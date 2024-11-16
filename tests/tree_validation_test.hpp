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

#include "dro/hash_flat_map.hpp"
#include "flat-rb-tree.hpp"

#include <bits/concept_check.h>
#include <bits/stl_function.h>

#include "stl_tree_public.h"

namespace dro::detail {

template <typename T, typename Compare> struct TreeBuilder {
  hash_flat_base<T, IsAFlatSet, flatset_pair<T>, uint32_t, Compare> droRBTree {
      20000};
  std::_Rb_tree<T, T, std::_Identity<T>, Compare> gccRBTree;
  std::string error_message;
  details::FlatRBTree<T, details::FlatSetEmptyType, details::FlatSetPair<T>,
                      uint32_t, Compare>
      droRBTree2;

  void insert(T key) {
    auto iter = droRBTree.emplace(key).first;
    if (*iter != key) {
      throw std::logic_error("Insert Iterator is not correct");
    }
    gccRBTree._M_emplace_unique(key);
    validateTree();
    checkFirstLastIter();
  }

  void erase(T key) {
    auto iterDroFind = droRBTree.find(key);
    auto iterGccFind = gccRBTree.find(key);
    if (iterDroFind == droRBTree.end()) {
      if (iterGccFind != gccRBTree.end()) {
        throw std::logic_error("Find Iterator is not correct");
      }
      return;
    }
    auto droIter = droRBTree.erase(iterDroFind);
    auto gccIter = gccRBTree.erase(iterGccFind);
    if (droIter == droRBTree.end()) {
      if (gccIter != gccRBTree.end()) {
        throw std::logic_error("Erase Iterator is not correct");
      }
      return;
    }
    if (*droIter != *gccIter) {
      throw std::logic_error("Erase Iterator is not correct");
    }
    validateTree();
    checkFirstLastIter();
  }

  void checkFirstLastIter() {
    auto iterDro = droRBTree.begin();
    auto iterGCC = gccRBTree.begin();
    for (; iterDro != droRBTree.end() && iterGCC != gccRBTree.end();
         ++iterDro, ++iterGCC) {
      if (*iterDro != *iterGCC) {
        throw std::logic_error("Forward Iterator is not correct");
      }
    }
    auto iterDroR = droRBTree.rbegin();
    auto iterGCCR = gccRBTree.rbegin();
    for (; iterDroR != droRBTree.rend() && iterGCCR != gccRBTree.rend();
         ++iterDroR, ++iterGCCR) {
      if (*iterDroR != *iterGCCR) {
        throw std::logic_error("Reverse Iterator is not correct");
      }
    }
  }

  void validateTree() {
    auto droRoot  = droRBTree.root_;
    auto gccRoot  = gccRBTree._M_root();
    auto droRoot2 = droRBTree2.root_;
    // debug_print_tree_GCC(gccRoot, "", true);
    // debug_print_tree_Dro(droRoot, "", true);
    // debug_print_tree_Dro2(droRoot2, "", true);
    if (gccRoot) {
      T gccRootKey =
          std::_Rb_tree<T, T, std::_Identity<T>, std::less<T>>::_S_key(gccRoot);
      // Confirm tree roots are equal
      if (droRBTree.tree_[droRoot].pair_.first != gccRootKey ||
          _getColor(droRBTree.tree_[droRoot].fingerprint_full_clr_) !=
              gccRoot->_M_color) {
        throw std::logic_error("Tree root out of sync");
      }
      if (droRBTree.tree_[droRoot].parent_ != droRBTree.empty_index_) {
        throw std::logic_error("Tree root parent out of sync");
      }
      traverseTree(droRoot, gccRoot);
    } else {
      if (droRoot != droRBTree.empty_index_) {
        throw std::logic_error("Dro tree root should be empty_index");
      }
    }
  }

  void traverseTree(auto droNode, auto gccNode) {
    // Left Tree
    auto droLeftNode = droRBTree.tree_[droNode].left_;
    auto gccLeftNode = gccNode->_M_left;
    if (gccLeftNode) {
      T gccLeftKey =
          std::_Rb_tree<T, T, std::_Identity<T>, std::less<T>>::_S_key(
              gccLeftNode);
      // Confirm left tree nodes are equal
      if (droRBTree.tree_[droLeftNode].pair_.first != gccLeftKey ||
          _getColor(droRBTree.tree_[droLeftNode].fingerprint_full_clr_) !=
              gccLeftNode->_M_color) {
        std::cout << "parent- " << droLeftNode << ' ' << gccLeftKey << '\n'; 
        std::cout << "error- " << droRBTree.tree_[droLeftNode].pair_.first << ' ' << gccLeftKey << '\n'; 
        error_message +=
            "Left tree out of sync at index: " + std::to_string(droLeftNode);
        throw std::logic_error(error_message);
      }
      if (droRBTree.tree_[droLeftNode].parent_ != droNode) {
        error_message += "Left tree parent out of sync at index: " +
                         std::to_string(droLeftNode);
        throw std::logic_error(error_message);
      }
      traverseTree(droLeftNode, gccLeftNode);
    } else {
      if (droLeftNode != droRBTree.empty_index_) {
        error_message += "Left tree node should be empty_index_ at index: " +
                         std::to_string(droLeftNode);
        throw std::logic_error(error_message);
      }
    }
    // Right Tree
    auto droRightNode = droRBTree.tree_[droNode].right_;
    auto gccRightNode = gccNode->_M_right;
    if (gccRightNode) {
      T gccRightKey =
          std::_Rb_tree<T, T, std::_Identity<T>, std::less<T>>::_S_key(
              gccRightNode);
      // Confirm right tree nodes are equal
      if (droRBTree.tree_[droRightNode].pair_.first != gccRightKey ||
          _getColor(droRBTree.tree_[droRightNode].fingerprint_full_clr_) !=
              gccRightNode->_M_color) {
        error_message +=
            "Right tree out of sync at index: " + std::to_string(droRightNode);
        throw std::logic_error(error_message);
      }
      if (droRBTree.tree_[droRightNode].parent_ != droNode) {
        error_message += "Right tree parent out of sync at index: " +
                         std::to_string(droRightNode);
        throw std::logic_error(error_message);
      }
      traverseTree(droRightNode, gccRightNode);
    } else {
      if (droRightNode != droRBTree.empty_index_) {
        error_message += "Right tree node should be empty_index_ at index: " +
                         std::to_string(droRightNode);
        throw std::logic_error(error_message);
      }
    }
  }

  void debug_print_tree_Dro(auto& node, std::string indent, bool last) {
    if (indent.empty()) {
      std::cout << "dro::rb_tree:\n";
    }
    if (node != droRBTree.empty_index_) {
      std::cout << indent;
      if (last) {
        std::cout << "R----";
        indent += "   ";
      } else {
        std::cout << "L----";
        indent += "|  ";
      }
      std::string sColor =
          (_getColor(droRBTree.tree_[node].fingerprint_full_clr_)) ? "BLACK"
                                                                   : "RED";
      std::cout << droRBTree.tree_[node].pair_.first << " (" << sColor << ")"
                << '\n';
      debug_print_tree_Dro(droRBTree.tree_[node].left_, indent, false);
      debug_print_tree_Dro(droRBTree.tree_[node].right_, indent, true);
    }
  }

  void debug_print_tree_Dro2(auto& node, std::string indent, bool last) {
    if (indent.empty()) {
      std::cout << "dro::rb_tree2:\n";
    }
    if (node != droRBTree2.empty_index_) {
      std::cout << indent;
      if (last) {
        std::cout << "R----";
        indent += "   ";
      } else {
        std::cout << "L----";
        indent += "|  ";
      }
      std::string sColor = (droRBTree2.tree_[node].color_) ? "BLACK" : "RED";
      std::cout << droRBTree2.tree_[node].pair_.first << " (" << sColor << ")"
                << '\n';
      debug_print_tree_Dro2(droRBTree2.tree_[node].left_, indent, false);
      debug_print_tree_Dro2(droRBTree2.tree_[node].right_, indent, true);
    }
  }

  void debug_print_tree_GCC(auto& node, std::string indent, bool last) {
    if (indent.empty()) {
      std::cout << "gcc::rb_tree:\n";
    }
    if (node) {
      std::cout << indent;
      if (last) {
        std::cout << "R----";
        indent += "   ";
      } else {
        std::cout << "L----";
        indent += "|  ";
      }
      std::string sColor = (node->_M_color) ? "BLACK" : "RED";
      T Key =
          std::_Rb_tree<T, T, std::_Identity<T>, std::less<T>>::_S_key(node);
      std::cout << Key << " (" << sColor << ")" << '\n';
      debug_print_tree_GCC(node->_M_left, indent, false);
      debug_print_tree_GCC(node->_M_right, indent, true);
    }
  }

  [[nodiscard]] bool
  _getColor(const uint64_t& fingerprint_full_clr) const noexcept {
    return fingerprint_full_clr & 2;
  }
};

}// namespace dro::detail

template <typename TreeBuilder> int runTreeTraversal(TreeBuilder rbTree) {
  const int iters = 5'000;
  std::vector<std::size_t> randNum(iters, 0);
  // Tree Functional Test
  try {
    for (int i {}; i < iters; ++i) { rbTree.insert(i); }
    for (int i {}; i < iters; ++i) {
      int rd = rand();
      rbTree.insert(rd);
      randNum[i] = rd;
    }
    std::cout << "Tree insertions completed...\n";

    for (int i {}; i < iters; ++i) {
      int rd = randNum[i];
      rbTree.erase(rd);
    }
    for (int i {}; i < iters; ++i) { rbTree.erase(i); }
    std::cout << "Tree erasions completed...\n";

    for (int i {}; i < iters; ++i) {
      int rd = rand() % (iters * 2);
      if (rd % 3) {
        rbTree.insert(rd);
      } else {
        rbTree.erase(rd);
      }
    }
    for (int i {}; i < iters; ++i) {
      int rd = rand() % (iters * 2);
      if (rd % 3) {
        rbTree.erase(rd);
      } else {
        rbTree.insert(rd);
      }
    }
    std::cout << "Tree combinations completed...\n";
    // Print Error and Terminate
  } catch (std::logic_error& e) {
    std::cerr << "Test Terminated with error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}

bool run_tree_validation() {
  std::cout << "Starting Tree Traversal Test... Runtime ~45 seconds.\n";

  using lessCompareTreeBuilder = dro::detail::TreeBuilder<int, std::less<int>>;
  using greaterCompareTreeBuilder =
      dro::detail::TreeBuilder<int, std::greater<int>>;
  lessCompareTreeBuilder rbTreeL;
  greaterCompareTreeBuilder rbTreeG;

  if (runTreeTraversal(rbTreeL)) {
    return true;
  }
  if (runTreeTraversal(rbTreeG)) {
    return true;
  }
  return false;
}
