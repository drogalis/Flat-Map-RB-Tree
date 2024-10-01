// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <dro/flat-rb-tree.hpp>// for dro::FlatMap & dro::FlatSet
#include <iostream>

int main(int argc, char* argv[]) {
  dro::FlatMap<int, int> map;
  dro::FlatSet<int> set;

  // Count num of elements in array
  std::array<int, 10> arr = {0, 0, 3, 3, 3, 4, 4, 5, 9, 4};
  for (auto& elem : arr) { map[elem]++; }

  // Can also build in place
  map.emplace(30, 2);
  map.insert(std::make_pair(50, 3));

  for (const auto& elem : map) {
    std::cout << "Key: " << elem.first << " Value: " << elem.second << '\n';
  }

  // Erase element
  map.erase(9);
  bool success = map.contains(9);

  return 0;
}
