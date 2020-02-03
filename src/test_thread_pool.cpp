/* -*- coding: UTF-8 -*-
 *
 *  Copyright (c) 2015-2019 by Inteos Sp. z o.o.
 *  All rights reserved. See LICENSE file for details.
 *
 */

/*
 * File:   HashIndexValueTests.cpp
 * Author: radekk
 *
 * Created on 30 października 2019, 16:27
 */

#include <iostream>
#include <random>
#include "ThreadPool.h"
#include "catch.hpp"


std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<int> dist(-1000, 1000);
auto rnd = std::bind(dist, mt);


void simulate_hard_computation() {
  std::this_thread::sleep_for(std::chrono::milliseconds(2000 + rnd()));
}

// Simple function that adds multiplies two numbers and prints the result
void multiply(const int a, const int b) {
  simulate_hard_computation();
  const int res = a * b;
  std::cout << a << " * " << b << " = " << res << std::endl;
}

// Same as before but now we have an output parameter
void multiply_output(int & out, const int a, const int b) {
  simulate_hard_computation();
  out = a * b;
  std::cout << a << " * " << b << " = " << out << std::endl;
}

// Same as before but now we have an output parameter
int multiply_return(const int a, const int b) {
  simulate_hard_computation();
  const int res = a * b;
  std::cout << a << " * " << b << " = " << res << std::endl;
  return res;
}

TEST_CASE ("Execute simple tasks"){

};

void example() {
   // Create pool with 3 threads
   ThreadPool pool(3);

   // Initialize pool
   pool.init();

   // Submit (partial) multiplication table
   for (auto i = 1; i < 3; ++i) {
      for (auto j = 1; j < 10; ++j) {
         pool.submit(multiply, i, j);
      }
   }

   // Submit function with output parameter passed by ref
   int output_ref;
   auto future1 = pool.submit(multiply_output, std::ref(output_ref), 5, 6);

   // Wait for multiplication output to finish
   future1.get();
   std::cout << "Last operation result is equals to " << output_ref << std::endl;

   // Submit function with return parameter
   auto future2 = pool.submit(multiply_return, 5, 3);

   // Wait for multiplication output to finish
   int res = future2.get();
   std::cout << "Last operation result is equals to " << res << std::endl;

   pool.shutdown();
}