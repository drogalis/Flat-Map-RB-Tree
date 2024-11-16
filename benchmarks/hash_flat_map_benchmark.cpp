// Copyright (c) 2024-2025 Andrew Drogalis
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

#include <chrono> // for high_resolution_clock, nanoseconds
#include <cstdint>// for uint32_t
#include <cstdlib>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <vector>

#include "dro/hash_flat_map.hpp"

#if __has_include(<boost/container/flat_map.hpp> )
#include <boost/container/flat_map.hpp>
#endif

#if __has_include(<folly/container/heap_vector_types.h> )
#include <folly/container/heap_vector_types.h>
#endif

struct alignas(4) Test {
  int x_;
  Test() = default;
  Test(int x) : x_(x) {}
  auto operator<=>(const Test&) const = default;
};

int main() {
  int iterations = 100'00;

  // Generate Vector of Random Ints
  std::vector<int> randInts(iterations);
  std::vector<std::pair<int, int>> findKeys(iterations);
  for (auto& i : randInts) { i = rand(); }

  std::cout << "dro::hashed_flat_map: \n";

  // Insertion Benchmark
  dro::hash_flat_map<int, int, uint32_t> dro_ {iterations * 2};

  auto start = std::chrono::high_resolution_clock::now();
  for (const auto& i : randInts) { dro_.emplace(i, i); }
  auto stop = std::chrono::high_resolution_clock::now();

  std::cout << "Mean insertion time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Find Benchmark
  int idx {};
  start = std::chrono::high_resolution_clock::now();
  for (const auto& i : randInts) {
    findKeys[idx] = *(dro_.find(i));
    ++idx;
  }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Mean find time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Remove Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (const auto& i : randInts) { dro_.erase(i); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Mean erase time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations

            << " ns.\n";

  // ==============================================================================

  std::cout << "std::map: \n";

  // Insertion Benchmark
  std::map<int, int> stl_;
  start = std::chrono::high_resolution_clock::now();
  for (const auto& i : randInts) { stl_.emplace(i, i); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Mean insertion time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Find Benchmark
  idx   = 0;
  start = std::chrono::high_resolution_clock::now();
  for (const auto& i : randInts) {
    findKeys[idx] = *(stl_.find(i));
    ++idx;
  }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Mean find time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Remove Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (const auto& i : randInts) { stl_.erase(i); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Mean erase time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

#if __has_include(<boost/container/flat_map.hpp---->)

  std::cout << "boost::flat_map: \n";

  // Insertion Benchmark
  boost::container::flat_map<Test, Test> boost_;
  start = std::chrono::high_resolution_clock::now();
  for (const auto& i : randInts) { boost_.emplace(Test(i), Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Mean insertion time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Find Benchmark
  idx   = 0;
  start = std::chrono::high_resolution_clock::now();
  for (const auto& i : randInts) {
    findKeys[idx] = *(boost_.find(Test(i)));
    ++idx;
  }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Mean find time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Remove Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (const auto& i : randInts) { boost_.erase(Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Mean erase time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations

            << " ns.\n";

#endif

#if __has_include(<folly/container/heap_vector_types.h>)

  std::cout << "folly::heap_vector_map: \n";

  // Insertion Benchmark
  folly::heap_vector_map<Test, Test> folly_;
  start = std::chrono::high_resolution_clock::now();
  for (const auto& i : randInts) { folly_.emplace(Test(i), Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Mean insertion time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Find Benchmark
  idx   = 0;
  start = std::chrono::high_resolution_clock::now();
  for (const auto& i : randInts) {
    findKeys[idx] = *(folly_.find(Test(i)));
    ++idx;
  }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Mean find time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Remove Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (const auto& i : randInts) { folly_.erase(Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Mean erase time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations

            << " ns.\n";

#endif

  return 0;
}
