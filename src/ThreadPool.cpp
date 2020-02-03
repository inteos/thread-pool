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

#include "ThreadPool.h"

/*
 *
 */
ThreadPool::ThreadWorker::ThreadWorker(ThreadPool * pool)
   :ptr(pool) {};

/*
 *
 */
void ThreadPool::ThreadWorker::operator()()
{
   std::function<void()> func;
   bool dequeued;

   while (!ptr->shut_flag)
   {
      ThreadPool * poolptr = ptr;
      {
         std::unique_lock<std::mutex> lock(ptr->mutex);
         ptr->waitcv.wait(lock, [this, poolptr]{ return poolptr->job_queue.empty();});
         dequeued = ptr->job_queue.dequeue(func);
      }
      if (dequeued) {
         func();
      }
   }
};

/*
 *
 */
ThreadPool::ThreadPool(const std::size_t threads_num)
   :threads(std::vector<std::thread>(threads_num)) {};

/*
 *
 */
void ThreadPool::init() {
#if defined __sun__ || defined __linux__ || defined __APPLE__
   std::size_t vcpu = 0;
   std::size_t vcpu_max = std::thread::hardware_concurrency() - 1;
#endif

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
#ifdef __sun__
      processor_bind(P_LWPID, P_MYID, vcpuid[vcpu], NULL);
#endif

      t = std::thread(ThreadWorker(this));

#ifdef __linux__
      cpu_set_t mask;
      CPU_ZERO(&mask);
      CPU_SET(v_cpu, &mask);
      pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &mask);
#endif

#ifdef __APPLE__
   thread_affinity_policy_data_t policy = { static_cast<integer_t>(vcpu) };
   thread_policy_set(pthread_mach_thread_np(t.native_handle()),
                     THREAD_AFFINITY_POLICY,
                     (thread_policy_t)&policy,
                     THREAD_AFFINITY_POLICY_COUNT);
#endif

#if defined __sun__ || defined __linux__ || defined __APPLE__
      if (++vcpu > vcpu_max) {
         vcpu = 0;
      }
#endif
   }
}

/*
 *
 */
void ThreadPool::shutdown()
{
   shut_flag = true;
   waitcv.notify_all();

   for (auto i = 0; i < threads.size(); ++i) {
      if(threads[i].joinable()) {
         threads[i].join();
      }
   }
}
