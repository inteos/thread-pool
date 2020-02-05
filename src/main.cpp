#include <iostream>
#include <random>
#include <ctime>
#include <ratio>
#include <chrono>
#include "ThreadPool.h"

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<int> dist(-1000, 1000);
auto rnd = std::bind(dist, mt);
std::atomic<int> counter = {0};
std::atomic<bool> shutdown = { false };

void simulate_hard_computation()
{
   std::this_thread::sleep_for(std::chrono::milliseconds(200 + rnd()));
}

void test_thread1()
{
   counter++;
   simulate_hard_computation();
}

void test_thread2()
{
   //counter++;
}

void display(ThreadPool * poolptr)
{
   std::size_t qs;
   std::size_t rt;

   while(!shutdown){
      qs = poolptr->queue_size();
      rt = poolptr->num_running();
      std::cout << "Queued " << qs << " jobs (utilization: " << rt << ")..." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
   }
}

int main()
{
   ThreadPool pool(4);
   ThreadPool *poolptr = &pool;
   const auto jobs = 100;
   const auto jobs2 = 10000000;

   std::thread infoth(display, poolptr);
#if 0
   for (auto i = 0; i < jobs; i++){
      pool.submit(test_thread1);
   }

   while (pool.queue_size() > 0){
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
   }

   if (counter != jobs){
      std::cout << "Error! counter=" << counter << std::endl;
      return 1;
   }

   counter = 0;
#endif

   std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

   for (auto i = 0; i < jobs2; i++){
      pool.submit(test_thread2);
   }

   std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

   std::cout << "submit end" << std::endl;
   std::this_thread::sleep_for(std::chrono::milliseconds(2000));
   pool.init(false);
   std::cout << "go!" << std::endl;

   std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();

   while (pool.queue_size() > 0){}

   std::chrono::high_resolution_clock::time_point t4 = std::chrono::high_resolution_clock::now();

   shutdown = true;
   infoth.join();

   pool.shutdown();

#if 0
   if (counter != jobs2){
      std::cout << "Error! counter=" << counter << std::endl;
      return 1;
   }
#endif

   std::cout << "done" << std::endl;

   std::chrono::duration<double> time_span1 = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
   std::chrono::duration<double> time_span2 = std::chrono::duration_cast<std::chrono::duration<double>>(t4 - t3);

   std::cout << "Task queue " << time_span1.count() << " seconds." << std::endl;;
   std::cout << "Task handling " << time_span2.count() << " seconds." << std::endl;;
   std::cout << std::endl;

   return 0;
};