// A

#include "dro/optimized-rb-tree.hpp"
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
  std::vector<int> randInts(0, iterations);
  // for (auto& i : randInts) { i = rand(); }
  for (int i {}; i < iterations; ++i) {
    randInts[i] = i; 
  } 

  std::cout << "Dro Flat RB Tree: \n";

  // Insertion Benchmark
  dro::OptimizedRBTree<int> rbTree;
  auto start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { rbTree.insert(i); }
  auto stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average insertion time: "
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

#if __has_include(<boost/container/flat_set.hpp> )

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
