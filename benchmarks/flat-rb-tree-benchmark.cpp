// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

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

struct alignas(4) Test {
  int x_;
  Test() = default;
  Test(int x) : x_(x) {}
  auto operator<=>(const Test&) const = default;
};

int main() {
  int iterations = 1'000'000;

  // Generate Vector of Random Ints
  std::vector<int> randInts(iterations);
  for (auto& i : randInts) { i = rand(); }

  std::cout << "Dro Flat RB Tree: \n";

  // Insertion Benchmark
  dro::FlatSet<Test, int> rbTree;
  auto start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { rbTree.insert(Test(i)); }
  auto stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average insertion time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Find Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { rbTree.find(Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Total find time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                   .count()
            << " ns.\n";

  // Remove Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { rbTree.erase(Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average Erase time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations

            << " ns.\n";

  // ==============================================================================

  std::cout << "STL Set (RB Tree): \n";

  // Insertion Benchmark
  std::set<Test> stl_rbTree;
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { stl_rbTree.insert(Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average insertion time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Find Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { stl_rbTree.count(Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Total find time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                   .count()
            << " ns.\n";

  // Remove Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { stl_rbTree.erase(Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average Erase time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

#if __has_include(<boost/container/flat_set.hpp>)

  std::cout << "Boost Flat Set (RB Tree): \n";

  // Insertion Benchmark
  boost::container::flat_set<Test> boost_rbTree;
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { boost_rbTree.insert(Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average insertion time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Find Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { boost_rbTree.find(Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Total find time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                   .count()
            << " ns.\n";

  // Remove Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { boost_rbTree.erase(Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average Erase time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations

            << " ns.\n";

#endif

  return 0;
}
