// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.rogalis

#include "dro/flat-rb-tree.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

void testSet() {

  // Iterators
  {
    dro::FlatSet<int> flatset(10);
    const auto& cflatset = flatset;

    assert(flatset.begin() == flatset.end());
    assert(cflatset.begin() == cflatset.end());
    assert(flatset.cbegin() == flatset.cend());
    assert(flatset.cbegin() == cflatset.begin());
    assert(flatset.cend() == cflatset.end());

    assert(! (flatset.begin() != flatset.end()));
    assert(! (cflatset.begin() != cflatset.end()));
    assert(! (flatset.cbegin() != flatset.cend()));
    assert(! (flatset.cbegin() != cflatset.begin()));
    assert(! (flatset.cend() != cflatset.end()));

    for (int i = 1; i < 100; ++i) { flatset.insert(i); }

    int sum {};
    for (auto it : flatset) { sum += it; }
    assert(sum == 4950);
    assert(std::all_of(flatset.begin(), flatset.end(),
                       [](const auto& item) { return item > 0; }));
  }

  // Capacity
  {
    dro::FlatSet<int> flatset(10);
    const auto& cflatset = flatset;
    assert(cflatset.empty());
    assert(cflatset.size() == 0);
    assert(cflatset.max_size() > 0);
    flatset.insert(1);
    assert(! cflatset.empty());
    assert(cflatset.size() == 1);
  }

  // Modifiers
  {
    dro::FlatSet<int> flatset(10);
    flatset.insert(1);
    flatset.clear();
    assert(flatset.empty());
    assert(flatset.size() == 0);
    assert(flatset.begin() == flatset.end());
    assert(flatset.cbegin() == flatset.cend());
  }

  {
    dro::FlatSet<int> flatset(10);
    auto res = flatset.insert(1);
    assert(! flatset.empty());
    assert(flatset.size() == 1);
    assert(flatset.begin() != flatset.end());
    assert(flatset.cbegin() != flatset.cend());
    assert(res.first != flatset.end());
    assert(*(res.first) == 1);
    assert(res.second);
    auto res2 = flatset.insert(1);
    assert(flatset.size() == 1);
    assert(res2.first == res.first);
    assert(*(res2.first) == 1);
    assert(! res2.second);
  }

  {
    dro::FlatSet<int> flatset(10);
    auto res = flatset.emplace(1);
    assert(! flatset.empty());
    assert(flatset.size() == 1);
    assert(flatset.begin() != flatset.end());
    assert(flatset.cbegin() != flatset.cend());
    assert(res.first != flatset.end());
    assert(*(res.first) == 1);
    assert(res.second);
    auto res2 = flatset.emplace(1);
    assert(flatset.size() == 1);
    assert(res2.first == res.first);
    assert(*(res2.first) == 1);
    assert(! res2.second);
  }

  {
    dro::FlatSet<int> flatset(10);
    auto res = flatset.emplace(1);
    flatset.erase(res.first);
    assert(flatset.empty());
    assert(flatset.size() == 0);
    assert(flatset.begin() == flatset.end());
    assert(flatset.cbegin() == flatset.cend());
  }

  {
    dro::FlatSet<int> flatset(10);
    assert(flatset.erase(1) == 0);
    flatset.insert(1);
    assert(flatset.erase(1) == 1);
    assert(flatset.empty());
    assert(flatset.size() == 0);
    assert(flatset.begin() == flatset.end());
    assert(flatset.cbegin() == flatset.cend());
  }

  {
    dro::FlatSet<int> flatset1(10), flatset2(16);
    flatset1.insert(1);
    flatset2.swap(flatset1);
    assert(flatset1.empty());
    assert(flatset1.size() == 0);
    assert(flatset2.size() == 1);
    assert(*(flatset2.find(1)) == 1);
    std::swap(flatset1, flatset2);
    assert(flatset1.size() == 1);
    assert(*(flatset1.find(1)) == 1);
    assert(flatset2.empty());
    assert(flatset2.size() == 0);
  }

  {
    dro::FlatSet<int> flatset(10);
    const auto& cflatset = flatset;
    flatset.insert(1);
    assert(flatset.count(1) == 1);
    assert(flatset.count(2) == 0);
    assert(cflatset.count(1) == 1);
    assert(cflatset.count(2) == 0);
  }

  {
    dro::FlatSet<int> flatset(10);
    const auto& cflatset = flatset;
    flatset.insert(1);
    {
      auto it = flatset.find(1);
      assert(it != flatset.end());
      assert(*it == 1);
      it = flatset.find(2);
      assert(it == flatset.end());
    }
    {
      auto it = cflatset.find(1);
      assert(it != cflatset.end());
      assert(*it == 1);
      it = cflatset.find(2);
      assert(it == cflatset.end());
    }
  }
}
