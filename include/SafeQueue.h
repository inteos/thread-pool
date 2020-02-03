/* -*- coding: UTF-8 -*-
 *
 *  Copyright (c) 2020 by Inteos Sp. z o.o.
 *  All rights reserved. See LICENSE file for details.
 */

/*
 * File:   SafeQueue.h
 *
 * This is a modified version of the SafeQueue class by Mariano Trebino
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

#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H

#include <mutex>
#include <queue>


/*
 * Thread safe implementation of a Queue using a std::queue
 */
template <typename T>
class SafeQueue {
private:
   std::queue<T> queue;
   std::mutex mutex;

public:
/*
 * Standard class ctor/dtor
 */
   SafeQueue() {};
   SafeQueue(SafeQueue& other) = delete;
   ~SafeQueue() {};

/*
 * Checks if a queue is empty
 */
   inline bool empty() {
      std::lock_guard<std::mutex> l(mutex);
      return queue.empty();
   }

/*
 * Return the size of the queue
 */
   inline std::size_t size()
   {
      std::lock_guard<std::mutex> l(mutex);
      return queue.size();
   }

/*
 * Add an object to the queue
 */
   inline void enqueue(T& t)
   {
      std::lock_guard<std::mutex> l(mutex);
      queue.push(t);
   }

/*
 * Remove and return the object from the queue
 */
   inline bool dequeue(T& t)
   {
      std::lock_guard<std::mutex> l(mutex);

      if (queue.empty()) {
         return false;
      }

      t = std::move(queue.front());

      queue.pop();
      return true;
   }
};

#endif   /* SAFEQUEUE_H */