/* -*- coding: UTF-8 -*-
 *
 *  Copyright (c) 2020 by Inteos Sp. z o.o.
 *  All rights reserved. See LICENSE file for details.
 */

/*
 * File:   ThreadPool.h
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

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <cstddef>      /* For std::size_t */
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "SafeQueue.h"

class ThreadPool {
private:
   std::atomic_bool shut_flag { false };
   SafeQueue<std::function<void()>> job_queue {};
   std::vector<std::thread> threads {};
   std::mutex mutex {};
   std::condition_variable waitcv {};
   std::atomic_size_t available_threads { 0 };
   std::atomic_size_t running_threads { 0 };

   class ThreadWorker {
   private:
      ThreadPool * ptr {};

   public:
      ThreadWorker(ThreadPool * pool);
      void operator()();
   };

public:
   // Default ctor
   ThreadPool(const std::size_t threads_num = std::thread::hardware_concurrency());
   // Remove copy ctors
   ThreadPool(const ThreadPool &) = delete;
   ThreadPool(ThreadPool &&) = delete;

   // Defeult dtor
   ~ThreadPool(){ shutdown(); };

   // Remove default operators
   ThreadPool & operator=(const ThreadPool &) = delete;
   ThreadPool & operator=(ThreadPool &&) = delete;

   // Inits thread pool
   void init(bool cpuaffinity = false);

   // Shutdowns the pool waiting for current tasks finish
   void shutdown(bool abort = false);

   // Submit a function to be executed asynchronously by the pool
   template<typename F, typename...Args>
   auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
      // Create a function with bounded parameters ready to execute
      std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
      // Encapsulate it into a shared ptr in order to be able to copy construct / assign
      auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

      // Wrap packaged task into void function
      std::function<void()> wrapper_func = [task_ptr]() {
         (*task_ptr)();
      };

      // Enqueue generic wrapper function
      job_queue.enqueue(wrapper_func);

      // Wake up one thread if its waiting
      waitcv.notify_one();

      // Return future from promise
      return task_ptr->get_future();
   }

   // Return the size of the pool
   inline std::size_t size() { return threads.size(); }

   // Return the size of the job queue
   inline std::size_t queue_size() { return job_queue.size(); }

   // Return the number of threads available for job execution
   inline std::size_t num_available() { return available_threads; }

   // Return the number of threads running and executing jobs
   inline std::size_t num_running() { return running_threads; }
};

#endif   /* THREADPOOL_H */