// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.rogalis

#include "dro/flat-rb-tree.hpp"
#include "stl_tree_public.h"

#include <algorithm>
#include <bits/concept_check.h>
#include <bits/stl_function.h>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

namespace dro {

namespace details {

template <typename T> struct TreeBuilder {
  FlatRBTree<T, EmptyType> droRBTree;
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
      if (droRBTree.tree_[droRoot].key_ != gccRootKey ||
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

      if (droRBTree.tree_[droLeftNode].key_ != gccLeftKey ||
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

      if (droRBTree.tree_[droRightNode].key_ != gccRightKey ||
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
};

}// namespace details
}// namespace dro

int main() {
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
    for (int i {}; i < iters; ++i) {
      int h = randNum[i];
      rbtree.erase(h);
    }
    for (int i {}; i < iters; ++i) { rbtree.erase(i); }

  } catch (std::logic_error& e) {
    std::cerr << "Test Terminated with error: " << e.what() << "\n";
    return 1;
  }

  std::cout << "Test Completed! \n";
  return 0;
}
