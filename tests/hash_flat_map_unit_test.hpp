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

#include "dro/hash_flat_map.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <stdexcept>

void run_hash_flat_map_tests() {

  // Constructors
  {
    dro::hash_flat_map<int, int> flatmap(10);
    flatmap[1] = 1;
    dro::hash_flat_map<int, int> flatmap2(flatmap);
    assert(! flatmap2.empty());
    assert(flatmap2.size() == 1);
    assert(flatmap2[1] == 1);
  }

  {
    dro::hash_flat_map<int, int> flatmap(10);
    flatmap[1] = 1;
    dro::hash_flat_map<int, int> flatmap3(std::move(flatmap));
    assert(! flatmap3.empty());
    assert(flatmap3.size() == 1);
    assert(flatmap3[1] == 1);
  }

  {
    dro::hash_flat_map<int, int> flatmap(10);
    flatmap[1] = 1;
    dro::hash_flat_map<int, int> flatmap4(10);
    flatmap4 = flatmap;
    assert(! flatmap4.empty());
    assert(flatmap4.size() == 1);
    assert(flatmap4[1] == 1);
  }

  {
    dro::hash_flat_map<int, int> flatmap(10);
    flatmap[1] = 1;
    dro::hash_flat_map<int, int> flatmap5(10);
    flatmap5.operator=(std::move(flatmap));
    assert(! flatmap5.empty());
    assert(flatmap5.size() == 1);
    assert(flatmap5[1] == 1);
  }

  // Iterators
  {
    dro::hash_flat_map<int, int> flatmap(10);
    const auto& cflatmap = flatmap;

    assert(flatmap.begin() == flatmap.end());
    assert(cflatmap.begin() == cflatmap.end());
    assert(flatmap.cbegin() == flatmap.cend());
    assert(flatmap.cbegin() == cflatmap.begin());
    assert(flatmap.cend() == cflatmap.end());

    assert(! (flatmap.begin() != flatmap.end()));
    assert(! (cflatmap.begin() != cflatmap.end()));
    assert(! (flatmap.cbegin() != flatmap.cend()));
    assert(! (flatmap.cbegin() != cflatmap.begin()));
    assert(! (flatmap.cend() != cflatmap.end()));

    for (int i = 1; i < 100; ++i) { flatmap[i] = i; }

    int sum {};
    for (auto it : flatmap) { sum += it.first; }
    assert(sum == 4950);
    assert(std::all_of(flatmap.begin(), flatmap.end(),
                       [](const auto& item) { return item.second > 0; }));
  }

  // Capacity
  {
    dro::hash_flat_map<int, int> flatmap(10);
    const auto& cflatmap = flatmap;
    assert(cflatmap.empty());
    assert(cflatmap.size() == 0);
    assert(cflatmap.max_size() > 0);
    flatmap[1] = 1;
    assert(! cflatmap.empty());
    assert(cflatmap.size() == 1);
  }

  // Modifiers
  {
    dro::hash_flat_map<int, int> flatmap(10);
    flatmap[1] = 1;
    flatmap.clear();
    assert(flatmap.empty());
    assert(flatmap.size() == 0);
    assert(flatmap.begin() == flatmap.end());
    assert(flatmap.cbegin() == flatmap.cend());
  }

  {
    dro::hash_flat_map<int, int> flatmap(10);
    auto res = flatmap.insert({1, 1});
    assert(! flatmap.empty());
    assert(flatmap.size() == 1);
    assert(flatmap.begin() != flatmap.end());
    assert(flatmap.cbegin() != flatmap.cend());
    assert(res.first != flatmap.end());
    assert(res.first->first == 1);
    assert(res.first->second == 1);
    assert(res.second);
    const auto value = std::make_pair(1, 2);
    auto res2        = flatmap.insert(value);
    assert(flatmap.size() == 1);
    assert(res2.first == res.first);
    assert(res2.first->first == 1);
    assert(res2.first->second == 1);
    assert(! res2.second);
  }

  {
    dro::hash_flat_map<int, int> flatmap(10);
    auto res = flatmap.emplace(1, 1);
    assert(! flatmap.empty());
    assert(flatmap.size() == 1);
    assert(flatmap.begin() != flatmap.end());
    assert(flatmap.cbegin() != flatmap.cend());
    assert(res.first != flatmap.end());
    assert(res.first->first == 1);
    assert(res.first->second == 1);
    assert(res.second);
    auto res2 = flatmap.emplace(1, 2);
    assert(flatmap.size() == 1);
    assert(res2.first == res.first);
    assert(res2.first->first == 1);
    assert(res2.first->second == 1);
    assert(! res2.second);
  }

  {
    dro::hash_flat_map<int, int> flatmap(10);
    auto res = flatmap.emplace(1, 1);
    flatmap.erase(res.first);
    assert(flatmap.empty());
    assert(flatmap.size() == 0);
    assert(flatmap.begin() == flatmap.end());
    assert(flatmap.cbegin() == flatmap.cend());
  }

  {
    dro::hash_flat_map<int, int> flatmap(10);
    assert(flatmap.erase(1) == 0);
    flatmap[1] = 1;
    assert(flatmap.erase(1) == 1);
    assert(flatmap.empty());
    assert(flatmap.size() == 0);
    assert(flatmap.begin() == flatmap.end());
    assert(flatmap.cbegin() == flatmap.cend());
  }

  {
    // template <class K> erase(const K&)
    dro::hash_flat_map<int, int> flatmap(10);
    assert(flatmap.erase(1) == 0);
    flatmap[1] = 1;
    assert(flatmap.erase(1) == 1);
    assert(flatmap.empty());
    assert(flatmap.size() == 0);
    assert(flatmap.begin() == flatmap.end());
    assert(flatmap.cbegin() == flatmap.cend());
  }

  {
    dro::hash_flat_map<int, int> flatmap1(10), flatmap2(10);
    flatmap1[1] = 1;
    flatmap2.swap(flatmap1);
    assert(flatmap1.empty());
    assert(flatmap1.size() == 0);
    assert(flatmap2.size() == 1);
    assert(flatmap2[1] == 1);
    std::swap(flatmap1, flatmap2);
    assert(flatmap1.size() == 1);
    assert(flatmap1[1] == 1);
    assert(flatmap2.empty());
    assert(flatmap2.size() == 0);
  }

  // Lookup
  {
    dro::hash_flat_map<int, int> flatmap(10);
    const auto& cflatmap = flatmap;
    flatmap[1]           = 1;
    assert(flatmap.at(1) == 1);
    assert(cflatmap.at(1) == 1);
    flatmap.at(1) = 2;
    assert(flatmap.at(1) == 2);
    assert(cflatmap.at(1) == 2);
    try {
      flatmap.at(2);
      assert(false);// Should never reach
    } catch (std::out_of_range& e) {
      assert(true);// Should always reach
    } catch (...) {
      assert(false);// Should never reach
    }
    try {
      cflatmap.at(2);
      assert(false);// Should never reach
    } catch (std::out_of_range& e) {
      assert(true);// Should always reach
    } catch (...) {
      assert(false);// Should never reach
    }
  }

  {
    dro::hash_flat_map<int, int> flatmap(10);
    flatmap[1] = 1;
    assert(! flatmap.empty());
    assert(flatmap.size() == 1);
    assert(flatmap.begin() != flatmap.end());
    assert(flatmap.cbegin() != flatmap.cend());
    assert(flatmap[1] == 1);
  }

  {
    dro::hash_flat_map<int, int> flatmap(10);
    const auto& cflatmap = flatmap;
    flatmap[1]           = 1;
    assert(flatmap.count(1) == 1);
    assert(flatmap.count(2) == 0);
    assert(cflatmap.count(1) == 1);
    assert(cflatmap.count(2) == 0);
  }

  {
    dro::hash_flat_map<int, int> flatmap(10);
    const auto& cflatmap = flatmap;
    flatmap[1]           = 1;
    {
      auto it = flatmap.find(1);
      assert(it != flatmap.end());
      assert(it->first == 1);
      assert(it->second == 1);
      it = flatmap.find(2);
      assert(it == flatmap.end());
    }
    {
      auto it = cflatmap.find(1);
      assert(it != cflatmap.end());
      assert(it->first == 1);
      assert(it->second == 1);
      it = cflatmap.find(2);
      assert(it == cflatmap.end());
    }
  }


}
