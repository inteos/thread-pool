/* -*- coding: UTF-8 -*-
 *
 *  Copyright (c) 2020 by Inteos Sp. z o.o.
 *  All rights reserved. See LICENSE file for details.
 */

/*
 * File:   ThreadPool.cpp
 *
 * This is a modified version of the ThreadPool class by Mariano Trebino
 * Author: mtrebi - https://github.com/mtrebi
 * Project: thread-pool - https://github.com/mtrebi/thread-pool
 * All thread-pool source code is available at MIT License.
 *
 * MIT License
 *
 * Copyright (c) 2016 Mariano Trebino
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#else
#error "signal.h required!"
#endif
#ifdef HAVE_SYSTYPES_H
#include <sys/types.h>
#else
#error "sys/types.h required!"
#endif
#if defined __sun__
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>
#include <unistd.h>
#elif defined __linux__
#include <sched.h>
#elif defined __APPLE__
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#endif
#include "ThreadPool.h"

/*
 *
 */
ThreadPool::ThreadWorker::ThreadWorker(ThreadPool * pool) : ptr(pool) {};

/*
 *
 */
void ThreadPool::ThreadWorker::operator()()
{
   std::function<void()> func;
   bool dequeued;

   while (!ptr->shut_flag)
   {
      ThreadPool * poolptr = ptr;      // a little local helper
      {
         std::unique_lock<std::mutex> lock(ptr->mutex);

         // wait on new task to execute or shutdown notification
         ptr->waitcv.wait(lock, [this, poolptr]
            {
               return !poolptr->job_queue.empty() || poolptr->shut_flag;
            });

         ptr->running_threads++;
         // get next task to complete
         dequeued = !ptr->shut_flag ? ptr->job_queue.dequeue(func) : false;
      }

      // got new task to run
      if (dequeued) {
         func();
      }
      ptr->running_threads--;
   }
};

/*
 *
 */
ThreadPool::ThreadPool(const std::size_t threads_num) : threads(std::vector<std::thread>(threads_num)) {};

/*
 *
 */
void ThreadPool::init(bool cpuaffinity)
{
#if defined __sun__ || defined __linux__ || defined __APPLE__
   if (cpuaffinity){
      // create threads and assign them to different cores
      std::size_t vcpu = 0;
      std::size_t vcpu_max = std::thread::hardware_concurrency() - 1;

#if defined __sun__
      std::vector<processorid_t> vcpuid;   /* Struct for CPU/core ID */

      processorid_t i, cpuid_max;
      cpuid_max = sysconf(_SC_CPUID_MAX);
      for (i = 0; i <= cpuid_max; i++) {
         if (p_online(i, P_STATUS) != -1){   /* Get only online cores ID */
            vcpuid.push_back(i);
         }
      }
#endif

      for (auto &t : threads) {

#if defined __sun__
      processor_bind(P_LWPID, P_MYID, vcpuid[vcpu], NULL);
#endif

         // get thread reference and spawn a working thread using ThreadWorker class
         t = std::thread(ThreadWorker(this));

#if defined __linux__
         cpu_set_t mask;
         CPU_ZERO(&mask);
         CPU_SET(vcpu, &mask);
         pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &mask);
#endif

#if defined __APPLE__
         thread_affinity_policy_data_t policy = { static_cast<integer_t>(vcpu) };
         thread_policy_set(pthread_mach_thread_np(t.native_handle()),
                           THREAD_AFFINITY_POLICY,
                           (thread_policy_t)&policy,
                           THREAD_AFFINITY_POLICY_COUNT);
#endif

         // handle cpu max index
         if (++vcpu > vcpu_max) {
            vcpu = 0;
         }
      }
   } else {
#endif
      // simple thread creation
      for (auto &t : threads) {
         // get thread reference and spawn a working thread using ThreadWorker class
         t = std::thread(ThreadWorker(this));
      }
#if defined __sun__ || defined __linux__ || defined __APPLE__
   }
#endif
}

/*
 *
 */
void ThreadPool::shutdown(bool abort)
{
   // flag shutdown state
   shut_flag = true;

   // iterate through all running threads in the pool
   for (auto &t: threads) {
      // notify shutdown
      waitcv.notify_all();

      if (abort){
         // interrupt all blocking syscalls so the thread can check shutdown
         raise(SIGUSR1);
      }

      // if the thread is ready and joinable then wait for it to finish
      if (t.joinable()) {
         t.join();
      }
   }
}
