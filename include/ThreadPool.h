#pragma once

#ifdef AFFINITY
#if defined __sun__
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>
#include <unistd.h>	/* For sysconf */
#elif __linux__
#include <cstdio>	/* For fprintf */
#include <sched.h>
#endif
#endif

#include <cstddef>	/* For std::size_t */
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
  class ThreadWorker {
  private:
    std::size_t m_id;
    ThreadPool * m_pool;
  public:
    ThreadWorker(ThreadPool * pool, const std::size_t id)
      : m_pool(pool), m_id(id) {
    }

    void operator()() {
      std::function<void()> func;
      bool dequeued;
      while (!m_pool->m_shutdown) {
        {
          std::unique_lock<std::mutex> lock(m_pool->m_conditional_mutex);
          if (m_pool->m_queue.empty()) {
            m_pool->m_conditional_lock.wait(lock);
          }
          dequeued = m_pool->m_queue.dequeue(func);
        }
        if (dequeued) {
          func();
        }
      }
    }
  };

  bool m_shutdown;
  SafeQueue<std::function<void()>> m_queue;
  std::vector<std::thread> m_threads;
  std::mutex m_conditional_mutex;
  std::condition_variable m_conditional_lock;
public:
  ThreadPool(const std::size_t n_threads)
    : m_threads(std::vector<std::thread>(n_threads)), m_shutdown(false) {
  }

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;

  ThreadPool & operator=(const ThreadPool &) = delete;
  ThreadPool & operator=(ThreadPool &&) = delete;

  // Inits thread pool
  void init() {
    #if (defined __sun__ || defined __linux__) && defined AFFINITY
    std::size_t v_cpu = 0;
    std::size_t v_cpu_max = std::thread::hardware_concurrency() - 1;
    #endif

    #if defined __sun__ && defined AFFINITY
    std::vector<processorid_t> v_cpu_id;	/* Struct for CPU/core ID */

    processorid_t i, cpuid_max;
    cpuid_max = sysconf(_SC_CPUID_MAX);
    for (i = 0; i <= cpuid_max; i++) {
        if (p_online(i, P_STATUS) != -1)	/* Get only online cores ID */
            v_cpu_id.push_back(i);
    }
    #endif

    for (std::size_t i = 0; i < m_threads.size(); ++i) {

	#if (defined __sun__ || defined __linux__) && defined AFFINITY
	if (v_cpu > v_cpu_max) {
		v_cpu = 0;
	}

	#ifdef __sun__
	processor_bind(P_LWPID, P_MYID, v_cpu_id[v_cpu], NULL);
	#elif __linux__
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(v_cpu, &mask);
	pthread_t thread = pthread_self();
	if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &mask) != 0) {
		fprintf(stderr, "Error setting thread affinity\n");
	}
	#endif

	++v_cpu;
	#endif

      m_threads[i] = std::thread(ThreadWorker(this, i));
    }
  }

  // Waits until threads finish their current task and shutdowns the pool
  void shutdown() {
    m_shutdown = true;
    m_conditional_lock.notify_all();
    
    for (int i = 0; i < m_threads.size(); ++i) {
      if(m_threads[i].joinable()) {
        m_threads[i].join();
      }
    }
  }

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
    m_queue.enqueue(wrapper_func);

    // Wake up one thread if its waiting
    m_conditional_lock.notify_one();

    // Return future from promise
    return task_ptr->get_future();
  }
};
