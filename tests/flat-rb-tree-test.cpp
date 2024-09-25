// Andrew Drogalis

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
#include <vector>

template <typename T> struct TreeBuilder {
  dro::FlatRBTree<T> droRBTree;
  std::_Rb_tree<T, T, std::_Identity<T>, std::less<T>> gccRBTree;

  void insert(T key) {
    droRBTree.insert(key);
    gccRBTree._M_emplace_unique(key);
    validateTree();
  }

  void erase(T key) {
    droRBTree.remove(key);
    gccRBTree.erase(key);
    validateTree();
  }

  void print_vals() { droRBTree.print_vals(); }

  void validateTree() {
    auto gccRoot = gccRBTree._M_root();
    auto droRoot = droRBTree.root_;
    if (gccRoot) {
      T gccRootKey =
          std::_Rb_tree<T, T, std::_Identity<T>, std::less<T>>::_S_key(gccRoot);
      if (droRBTree.tree_[droRoot].data_ != gccRootKey ||
          droRBTree.tree_[droRoot].color_ != gccRoot->_M_color) {
        throw std::logic_error("Tree Roots out of sync");
      }
      if (droRBTree.tree_[droRoot].parent_ != droRBTree.empty_value_) {
        throw std::logic_error("Tree Roots Parent out of sync");
      }
      // std::cout << "\nRoot: " << gccRootKey << " " << gccRoot->_M_color << '\n';
      traverseTree(droRoot, gccRoot);
    } else {
      if (droRoot != droRBTree.empty_value_) {
        throw std::logic_error("Sync Error at Root");
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
      // std::cout << "Left: " << gccLeftKey << ' ' << gccLeftNode->_M_color
      //           << " Dro: " << droRBTree.tree_[droLeftNode].data_ << " "
      //           << droRBTree.tree_[droLeftNode].color_ << " Index: " << droLeftNode << '\n';

      if (droRBTree.tree_[droLeftNode].data_ != gccLeftKey ||
          droRBTree.tree_[droLeftNode].color_ != gccLeftNode->_M_color) {
        throw std::logic_error("Error at Left Tree Index");
      }
      if (droRBTree.tree_[droLeftNode].parent_ != droNode) {
        throw std::logic_error("Error at Left Tree Parent");
      }
      traverseTree(droLeftNode, gccLeftNode);
    } else {
      if (droLeftNode != droRBTree.empty_value_) {
         throw std::logic_error("Sync Error at Left Tree Index");
      }
    }

    auto gccRightNode = gccNode->_M_right;
    auto droRightNode = droRBTree.tree_[droNode].right_;

    if (gccRightNode) {
      T gccRightKey =
          std::_Rb_tree<T, T, std::_Identity<T>, std::less<T>>::_S_key(
              gccRightNode);
      // std::cout << "Right: " << gccRightKey << ' ' << gccRightNode->_M_color
      //           << " Dro: " << droRBTree.tree_[droRightNode].data_ << " "
      //           << droRBTree.tree_[droRightNode].color_ << '\n';
      if (droRBTree.tree_[droRightNode].data_ != gccRightKey ||
          droRBTree.tree_[droRightNode].color_ != gccRightNode->_M_color) {
         throw std::logic_error("Error at Right Tree Index");
      }
      if (droRBTree.tree_[droRightNode].parent_ != droNode) {
        throw std::logic_error("Error at Right Tree Parent");
      }
      traverseTree(droRightNode, gccRightNode);
    } else {
      if (droRightNode != droRBTree.empty_value_) {
        throw std::logic_error("Sync Error at Right Tree Index");
      }
    }
  }
};

int main() {
  TreeBuilder<int> rbtree;
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
      std::cout << i << ' ' << h << '\n';
      rbtree.erase(h);
    }
    for (int i {}; i < iters; ++i) {
      std::cout << i << '\n';
      rbtree.erase(i);
    }

  } catch (std::logic_error& e) {
    std::cerr << e.what() << "\nERROR !!!!!!!!\n\n\n";
  }
  std::cout << "Final Vals: \n";
  //
  //
  // rbtree.print_vals();

  std::cout << "Test\n";

  return 0;
}
