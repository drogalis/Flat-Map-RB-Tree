// A

#include "dro/flat-rb-tree.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <set>
#include <vector>

#if __has_include(<boost/container/flat_set.hpp> )
#include <boost/container/flat_set.hpp>
#endif

int main() {
  int iterations = 1'000'000;
  // Generate Vector of Random Ints
  std::vector<int> randInts(iterations);
  // for (auto& i : randInts) { i = rand(); }
  for (int i {}; i < iterations; ++i) { randInts[i] = i; }

  std::cout << "Dro Flat RB Tree: \n";

  // Insertion Benchmark
  dro::FlatRBTree<int, int> rbTree;
  auto start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { rbTree.insert(i); }
  auto stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average insertion time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Find Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { rbTree.find_index(i); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Total find time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                   .count()
            << " ns.\n";

  // Remove Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { rbTree.remove(i); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average Erase time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations

            << " ns.\n";

  // Combo Insert and Remove Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (int i {1}; i < iterations; ++i) {
    if (i % 3) {
      rbTree.insert(randInts[i]);
    } else {
      rbTree.remove(randInts[i - 2]);
    }
  }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average Combo time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  std::cout << "STL Set (RB Tree): \n";

  // Insertion Benchmark
  std::set<int> stl_rbTree;
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { stl_rbTree.insert(i); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average insertion time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Find Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { stl_rbTree.count(i); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Total find time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                   .count()
            << " ns.\n";

  // Remove Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { stl_rbTree.erase(i); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average Erase time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Combo Insert and Remove Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (int i {1}; i < iterations; ++i) {
    if (i % 3) {
      stl_rbTree.insert(randInts[i]);
    } else {
      stl_rbTree.erase(randInts[i - 2]);
    }
  }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average Combo time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

#if __has_include(<boost/container/flat_set9.hpp>)// This is off

  std::cout << "Boost Flat Set (RB Tree): \n";

  // Insertion Benchmark
  boost::container::flat_set<int> boost_rbTree;
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { boost_rbTree.insert(i); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average insertion time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";
#endif

  return 0;
}
