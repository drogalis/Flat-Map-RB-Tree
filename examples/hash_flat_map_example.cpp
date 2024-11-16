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

#include <cstdint>              // for uint8_t
#include <dro/hash_flat_map.hpp>// for dro::hash_flat_map & dro::hash_flat_set
#include <iostream>

int main(int argc, char* argv[]) {
  dro::hash_flat_map<int, int> map {16};
  // Last template parameter in set is size type. So long as the set in not
  // larger than 255 elements (uint8_t limit), you can specify the size type to
  // save space in a node and improve performance
  dro::hash_flat_set<int, uint8_t> set {16};
  // Count num of elements in array
  std::array<int, 10> arr = {0, 0, 3, 3, 3, 4, 4, 5, 9, 4};
  for (auto& elem : arr) { map[elem]++; }

  // Can also build in place
  map.emplace(30, 2);
  // The emplace args only needs a key and it will generate the default value
  map.emplace(40);
  // Can use std::pair just like in std::map
  map.insert(std::make_pair(50, 3));

  for (const auto& elem : map) {
    std::cout << "Key: " << elem.first << " Value: " << elem.second << '\n';
  }

  // Erase element
  map.erase(9);
  bool success = map.contains(9);

  return 0;
}
