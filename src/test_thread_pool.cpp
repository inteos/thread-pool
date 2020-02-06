/* -*- coding: UTF-8 -*-
 *
 *  Copyright (c) 2015-2019 by Inteos Sp. z o.o.
 *  All rights reserved. See LICENSE file for details.
 *
 */

/*
 * File:   test_thread_pool.cpp
 * Author: radekk
 *
 * Created on 04 lutego 2020, 10:27
 *
 *
 */

#include <iostream>
#include <random>
#include <utility>
#include "ThreadPool.h"
#include "catch.hpp"


std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<int> dist(0, 10);
auto rnd = std::bind(dist, mt);
std::atomic<int> counter = {0};

// waits a random time which simulate random completition time
void simulate_hard_computation()
{
   std::this_thread::sleep_for(std::chrono::milliseconds(1 + rnd()));
}

// testing thread, no return, simulate computation time
void test_thread_void()
{
   counter++;
   simulate_hard_computation();
}

// testing thread, no return no computation time
void test_thread_none()
{
   counter++;
}

// testing thread, single parameter and return value, simulate computation time
int test_thread_p1r(const int a)
{
   simulate_hard_computation();
   return a;
}

// testing thread, double parameters and return value, simulate computation time
uint64_t test_thread_p2r(const int a, const int b)
{
   const auto res = a * b;
   simulate_hard_computation();
   return res;
}

// testing thread, triple parameters (including reference) and return value, simulate computation time
int test_thread_p3rr(uint64_t & out, const int a, const int b)
{
   out = a * b;
   simulate_hard_computation();
   return b;
}

void wait_for_pool_to_complete(ThreadPool &pool)
{
   do {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
   } while (pool.queue_size() > 0 || pool.num_running() > 0);
}

const auto test_vector1 = {1, 2, 3, 4, 6, 8, 16, 128, 256, 1024, 65536};
const auto test_vector2 = {1, 2, 4, 8, 16, 23, 32, 41, 89, 64, 127, 283, 353};
const std::vector<std::pair<int, int>> test_vector3 = {
   {45420, 30014},      // random set of numbers
   {56140, 6248},
   {13509, 36300},
   {2920,  20522},
   {27365, 23315},
   {40514, 37448},
   {34605, 54379},
   {10200, 12722},
};

TEST_CASE ("Default ctor", "defctor")
{
   SECTION ("default"){
      ThreadPool pool;
      CHECK ( pool.size() == std::thread::hardware_concurrency());
      CHECK_FALSE ( pool.num_available() > 0 );
   }

   SECTION ("zero")
   {
      ThreadPool pool(0);
      CHECK ( pool.size() == std::thread::hardware_concurrency() );
      CHECK_FALSE ( pool.num_available() > 0 );
   }
};

TEST_CASE ("Explicit ctor", "exctor")
{
   ThreadPool * pool;

   for (auto v : test_vector1){
      pool = new ThreadPool(v);
      REQUIRE ( pool != nullptr );
      CHECK ( pool->size() == v );
      CHECK_FALSE ( pool->num_available() > 0 );
      delete pool;
   }
};

TEST_CASE ("Job execution", "jobexec")
{
   ThreadPool pool;
   pool.init();

   for (auto v : test_vector2){
      counter = 0;
      for (auto n = 0; n < v; n++){
         pool.submit(test_thread_void);
      }
      wait_for_pool_to_complete(pool);
      CHECK ( counter == v );
      CHECK ( pool.num_available() > 0 );
      CHECK_FALSE ( pool.num_running() > 0 );
   }
};

TEST_CASE ("Job execution single param", "jobexecsingle")
{
   ThreadPool pool;
   pool.init();

   for (auto v : test_vector2){
      auto future = pool.submit(test_thread_p1r, v);
      // wait for result
      auto res = future.get();
      CHECK ( res == v );
      CHECK ( pool.num_available() > 0 );
      CHECK_FALSE ( pool.num_running() > 0 );
   }
};

TEST_CASE ("Job execution double param", "jobexecdouble")
{
   ThreadPool pool;
   pool.init();

   for (auto p : test_vector3){
      auto a = std::get<0>(p);
      auto b = std::get<1>(p);
      auto future = pool.submit(test_thread_p2r, a, b);
      // wait for result
      auto res = future.get();
      CHECK ( res == (a * b) );
      CHECK ( pool.num_available() > 0 );
      CHECK_FALSE ( pool.num_running() > 0 );
   }
};

TEST_CASE ("Job execution triple param", "jobexectriple")
{
   ThreadPool pool;
   pool.init();
   uint64_t out;

   for (auto p : test_vector3){
      auto a = std::get<0>(p);
      auto b = std::get<1>(p);
      out = 0;
      auto future = pool.submit(test_thread_p3rr, std::ref(out), a, b);
      // wait for result
      auto res = future.get();
      CHECK ( res == b );
      CHECK ( out == (a * b) );
      CHECK ( pool.num_available() > 0 );
      CHECK_FALSE ( pool.num_running() > 0 );
   }
};

TEST_CASE ("Init after shutdown", "shutinit")
{
   ThreadPool pool;

   // submit single job to execute
   pool.submit(test_thread_void);
   CHECK ( pool.queue_size() == 1 );

   // test shutdown without init
   pool.shutdown();
   CHECK_FALSE ( pool.num_available() > 0 );
   CHECK ( pool.queue_size() == 1 );

   // now do init and wait for job to complete
   pool.init();
   wait_for_pool_to_complete(pool);
   CHECK ( pool.num_available() > 0 );

   // test final shutdown
   pool.shutdown();
   CHECK_FALSE ( pool.num_available() > 0 );
}
