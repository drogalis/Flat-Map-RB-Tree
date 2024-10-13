// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "dro/flat-rb-tree.hpp"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <random>
#include <vector>

#if __has_include(<boost/container/flat_map.hpp> )
#include <boost/container/flat_map.hpp>
#endif

struct alignas(4) Test {
  int x_;
  Test() = default;
  Test(int x) : x_(x) {}
  auto operator<=>(const Test&) const = default;
};

int main() {
  int iterations = 100'000;

  // Generate Vector of Random Ints
  std::vector<int> randInts(iterations);
  std::vector<std::pair<Test, Test>> findKeys(iterations);
  for (auto& i : randInts) { i = rand(); }

  std::cout << "Dro FlatMap: \n";

  // Insertion Benchmark
  dro::FlatMap<Test, Test, uint32_t> dro_;
  auto start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { dro_.emplace(Test(i), Test(i)); }
  auto stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average insertion time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Find Benchmark
  int idx {};
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) {
    findKeys[idx] = *(dro_.find(Test(i)));
    ++idx;
  }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Total find time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Remove Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { dro_.erase(Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average Erase time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations

            << " ns.\n";

  // ==============================================================================

  std::cout << "STL Map: \n";

  // Insertion Benchmark
  std::map<Test, Test> stl_;
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { stl_.emplace(Test(i), Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average insertion time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Find Benchmark
  idx   = 0;
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) {
    findKeys[idx] = *(stl_.find(Test(i)));
    ++idx;
  }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Total find time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Remove Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { stl_.erase(Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average Erase time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

#if __has_include(<boost/container/flat_map.hpp>)

  std::cout << "Boost FlatMap: \n";

  // Insertion Benchmark
  boost::container::flat_map<Test, Test> boost_;
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { boost_.emplace(Test(i), Test(i)); }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Average insertion time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Find Benchmark
  idx   = 0;
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) {
    findKeys[idx] = *(boost_.find(Test(i)));
    ++idx;
  }
  stop = std::chrono::high_resolution_clock::now();

  std::cout << "Total find time: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                    start)
                       .count() /
                   iterations
            << " ns.\n";

  // Remove Benchmark
  start = std::chrono::high_resolution_clock::now();
  for (auto i : randInts) { boost_.erase(Test(i)); }
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
