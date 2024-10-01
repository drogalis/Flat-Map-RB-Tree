// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.rogalis

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include <bits/concept_check.h>
#include <bits/stl_function.h>

#include "dro/flat-rb-tree.hpp"
#include "flat-set-test.hpp"
#include "stl_tree_public.h"

namespace dro {

namespace details {

template <typename T> struct TreeBuilder {
  FlatRBTree<T, FlatSetEmptyType, details::PairFlatSet<T>> droRBTree;
  std::_Rb_tree<T, T, std::_Identity<T>, std::less<T>> gccRBTree;
  std::string error_message;

  void insert(T key) {
    droRBTree.insert(key);
    gccRBTree._M_emplace_unique(key);
    validateTree();
  }

  void erase(T key) {
    droRBTree.erase(key);
    gccRBTree.erase(key);
    validateTree();
  }

  void validateTree() {
    auto gccRoot = gccRBTree._M_root();
    auto droRoot = droRBTree.root_;
    if (gccRoot) {
      T gccRootKey =
          std::_Rb_tree<T, T, std::_Identity<T>, std::less<T>>::_S_key(gccRoot);
      if (droRBTree.tree_[droRoot].pair_.first != gccRootKey ||
          droRBTree.tree_[droRoot].color_ != gccRoot->_M_color) {
        throw std::logic_error("Tree Roots out of sync");
      }
      if (droRBTree.tree_[droRoot].parent_ != droRBTree.empty_index_) {
        throw std::logic_error("Tree Roots Parent out of sync");
      }
      traverseTree(droRoot, gccRoot);
    } else {
      if (droRoot != droRBTree.empty_index_) {
        throw std::logic_error("Tree Root should be empty_index");
      }
    }
  }

  void traverseTree(auto droNode, auto gccNode) {
    auto gccLeftNode = gccNode->_M_left;
    auto droLeftNode = droRBTree.tree_[droNode].left_;

    if (gccLeftNode) {
      T gccLeftKey =
          std::_Rb_tree<T, T, std::_Identity<T>, std::less<T>>::_S_key(
              gccLeftNode);

      if (droRBTree.tree_[droLeftNode].pair_.first != gccLeftKey ||
          droRBTree.tree_[droLeftNode].color_ != gccLeftNode->_M_color) {
        error_message +=
            "Left Tree error at index: " + std::to_string(droLeftNode);
        throw std::logic_error(error_message);
      }
      if (droRBTree.tree_[droLeftNode].parent_ != droNode) {
        error_message += "Left Tree parent out of sync at index: " +
                         std::to_string(droLeftNode);
        throw std::logic_error(error_message);
      }
      traverseTree(droLeftNode, gccLeftNode);
    } else {
      if (droLeftNode != droRBTree.empty_index_) {
        error_message += "Left Tree node should be empty at index: " +
                         std::to_string(droLeftNode);
        throw std::logic_error(error_message);
      }
    }

    auto gccRightNode = gccNode->_M_right;
    auto droRightNode = droRBTree.tree_[droNode].right_;

    if (gccRightNode) {
      T gccRightKey =
          std::_Rb_tree<T, T, std::_Identity<T>, std::less<T>>::_S_key(
              gccRightNode);

      if (droRBTree.tree_[droRightNode].pair_.first != gccRightKey ||
          droRBTree.tree_[droRightNode].color_ != gccRightNode->_M_color) {
        error_message +=
            "Right Tree error at index: " + std::to_string(droRightNode);
        throw std::logic_error(error_message);
      }
      if (droRBTree.tree_[droRightNode].parent_ != droNode) {
        error_message += "Right Tree parent out of sync at index: " +
                         std::to_string(droRightNode);
        throw std::logic_error(error_message);
      }
      traverseTree(droRightNode, gccRightNode);
    } else {
      if (droRightNode != droRBTree.empty_index_) {
        error_message += "Right Tree node should be empty at index: " +
                         std::to_string(droRightNode);
        throw std::logic_error(error_message);
      }
    }
  }

  void sortTest(std::vector<std::size_t> randNum, int iters) {
    for (int i {}; i < iters; ++i) { randNum.emplace_back(i); }
    std::sort(randNum.begin(), randNum.end());
    int i {};
    for (auto& elem : droRBTree) {
      if (elem != randNum[i]) {
        throw std::logic_error("Tree is not sorted correctly");
      }
      ++i;
    }
  }
};

}// namespace details
}// namespace dro

int main() {
  {
    dro::details::TreeBuilder<int> rbtree;
    const int loopCount = 20;
    const int maxIters  = 1000;

    const int iters = 5000;
    std::vector<std::size_t> randNum(iters, 0);

    try {
      for (int i {}; i < iters; ++i) { rbtree.insert(i); }
      for (int i {}; i < iters; ++i) {
        int h = rand();
        rbtree.insert(h);
        randNum[i] = h;
      }
      rbtree.sortTest(randNum, iters);
      for (int i {}; i < iters; ++i) {
        int h = randNum[i];
        rbtree.erase(h);
      }
      for (int i {}; i < iters; ++i) { rbtree.erase(i); }

    } catch (std::logic_error& e) {
      std::cerr << "Test Terminated with error: " << e.what() << "\n";
      return 1;
    }
  }

  // FlatSet
  testSet();

  // Constructors
  {
    dro::FlatMap<int, int> flatmap(10);
    flatmap[1] = 1;
    dro::FlatMap<int, int> flatmap2(flatmap);
    assert(! flatmap2.empty());
    assert(flatmap2.size() == 1);
    assert(flatmap2[1] == 1);
  }

  {
    dro::FlatMap<int, int> flatmap(10);
    flatmap[1] = 1;
    dro::FlatMap<int, int> flatmap3(std::move(flatmap));
    assert(! flatmap3.empty());
    assert(flatmap3.size() == 1);
    assert(flatmap3[1] == 1);
  }

  {
    dro::FlatMap<int, int> flatmap(10);
    flatmap[1] = 1;
    dro::FlatMap<int, int> flatmap4(10);
    flatmap4 = flatmap;
    assert(! flatmap4.empty());
    assert(flatmap4.size() == 1);
    assert(flatmap4[1] == 1);
  }

  {
    dro::FlatMap<int, int> flatmap(10);
    flatmap[1] = 1;
    dro::FlatMap<int, int> flatmap5(10);
    flatmap5.operator=(std::move(flatmap));
    assert(! flatmap5.empty());
    assert(flatmap5.size() == 1);
    assert(flatmap5[1] == 1);
  }

  // Iterators
  {
    dro::FlatMap<int, int> flatmap(10);
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
    dro::FlatMap<int, int> flatmap(10);
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
    dro::FlatMap<int, int> flatmap(10);
    flatmap[1] = 1;
    flatmap.clear();
    assert(flatmap.empty());
    assert(flatmap.size() == 0);
    assert(flatmap.begin() == flatmap.end());
    assert(flatmap.cbegin() == flatmap.cend());
  }

  {
    dro::FlatMap<int, int> flatmap(10);
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
    dro::FlatMap<int, int> flatmap(10);
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
    dro::FlatMap<int, int> flatmap(10);
    auto res = flatmap.emplace(1, 1);
    flatmap.erase(res.first);
    assert(flatmap.empty());
    assert(flatmap.size() == 0);
    assert(flatmap.begin() == flatmap.end());
    assert(flatmap.cbegin() == flatmap.cend());
  }

  {
    dro::FlatMap<int, int> flatmap(10);
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
    dro::FlatMap<int, int> flatmap(10);
    assert(flatmap.erase(1) == 0);
    flatmap[1] = 1;
    assert(flatmap.erase(1) == 1);
    assert(flatmap.empty());
    assert(flatmap.size() == 0);
    assert(flatmap.begin() == flatmap.end());
    assert(flatmap.cbegin() == flatmap.cend());
  }

  {
    dro::FlatMap<int, int> flatmap1(10), flatmap2(10);
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
    dro::FlatMap<int, int> flatmap(10);
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
      assert(true);// Should always reach;
    } catch (...) {
      assert(false);// Should never reach
    }
    try {
      cflatmap.at(2);
      assert(false);// Should never reach
    } catch (std::out_of_range& e) {
      assert(true);// Should always reach;
    } catch (...) {
      assert(false);// Should never reach
    }
  }

  {
    dro::FlatMap<int, int> flatmap(10);
    flatmap[1] = 1;
    assert(! flatmap.empty());
    assert(flatmap.size() == 1);
    assert(flatmap.begin() != flatmap.end());
    assert(flatmap.cbegin() != flatmap.cend());
    assert(flatmap[1] == 1);
  }

  {
    dro::FlatMap<int, int> flatmap(10);
    const auto& cflatmap = flatmap;
    flatmap[1]           = 1;
    assert(flatmap.count(1) == 1);
    assert(flatmap.count(2) == 0);
    assert(cflatmap.count(1) == 1);
    assert(cflatmap.count(2) == 0);
  }

  {
    dro::FlatMap<int, int> flatmap(10);
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

  std::cout << "Test Completed! \n";
  return 0;
}
