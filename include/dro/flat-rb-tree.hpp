// Andrew Drogalis

#ifndef DRO_FLAT_RED_BLACK_TREE
#define DRO_FLAT_RED_BLACK_TREE

#include <chrono>
#include <climits>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

namespace dro {

namespace details {

template <typename T>
concept FlatTree_Type =
    std::is_default_constructible<T>::value &&
    (std::is_move_assignable_v<T> || std::is_copy_assignable_v<T>);

template <typename T, typename... Args>
concept FlatTree_NoThrow_Type =
    std::is_nothrow_constructible_v<T, Args&&...> &&
    ((std::is_nothrow_copy_assignable_v<T> && std::is_copy_assignable_v<T>) ||
     (std::is_nothrow_move_assignable_v<T> && std::is_move_assignable_v<T>));

struct EmptyType {};

// Map Node
template <typename Key, typename Value, typename Size = std::size_t>
struct Node {
  Key key_;
  Value value_;
  Size parent_;
  Size left_;
  Size right_;
  bool color_ {};

  Node() = default;

  explicit Node(Key key, Value value, Size parent, Size left, Size right,
                bool color)
      : key_(key), value_(value), parent_(parent), left_(left), right_(right),
        color_(color) {}
};

// Set Node
template <typename Key, typename Size> struct Node<Key, EmptyType, Size> {
  Key key_;
  EmptyType value_ [[no_unique_address]];
  Size parent_;
  Size left_;
  Size right_;
  bool color_ {};

  Node() = default;

  explicit Node(Key key, EmptyType value, Size parent, Size left, Size right,
                bool color)
      : key_(key), value_(value), parent_(parent), left_(left), right_(right),
        color_(color) {}
};

template <typename ContT, typename IterVal, typename Size> struct set_iterator {
  using difference_type   = std::ptrdiff_t;
  using value_type        = IterVal;
  using size_type         = Size;
  using pointer           = value_type*;
  using reference         = value_type&;
  using iterator_category = std::forward_iterator_tag;

  bool operator==(const set_iterator& other) const {
    return other.flatTree_ == flatTree_ && other.idx_ == idx_;
  }
  bool operator!=(const set_iterator& other) const {
    return ! (other == *this);
  }

  set_iterator& operator++() {
    ++idx_;
    advance_past_empty();
    return *this;
  }

  reference operator*() const { return flatTree_->buckets_[idx_]; }
  pointer operator->() const { return &flatTree_->buckets_[idx_]; }

private:
  explicit set_iterator(ContT* flatTree) : flatTree_(flatTree) {
    advance_past_empty();
  }

  explicit set_iterator(ContT* flatTree, size_type idx)
      : flatTree_(flatTree), idx_(idx) {}

  template <typename OtherContT, typename OtherIterVal>
  explicit set_iterator(
      const set_iterator<OtherContT, OtherIterVal, Size>& other)
      : flatTree_(other.flatTree_), idx_(other.idx_) {}

  void advance_past_empty() {
    while (idx_ < flatTree_->size() &&
           (flatTree_->tree_[idx_].key_, flatTree_->empty_key_)) {
      ++idx_;
    }
  }

  ContT* flatTree_               = nullptr;
  typename ContT::size_type idx_ = 0;
  friend ContT;
};

template <typename Key, typename Value, typename Size = std::size_t,
          typename Compare   = std::less<Key>,
          typename Allocator = std::allocator<Node<Key, Value, Size>>>
class FlatRBTree {

  using key_type        = Key;
  using value_type      = Value;
  using size_type       = Size;
  using difference_type = std::ptrdiff_t;
  using key_compare     = Compare;
  using allocator_type  = Allocator;
  // using reference              = typename Container::reference;
  // using const_reference        = typename Container::const_reference;
  // using pointer                = typename Container::pointer;
  // using const_pointer          = typename Container::const_pointer;
  // using iterator               = typename Container::iterator;
  // using const_iterator         = typename Container::const_iterator;
  // using reverse_iterator       = typename Container::reverse_iterator;
  // using const_reverse_iterator = typename Container::const_reverse_iterator;

private:
  // Constants
  constexpr static bool RED_   = false;
  constexpr static bool BLACK_ = true;

  size_type capacity_ {};
  size_type size_ {};

#ifndef NDEBUG

public:
#else

private:
#endif
  std::vector<Node<Key, Value, Size>> tree_;
  constexpr static size_type empty_key_ = std::numeric_limits<size_type>::max();
  size_type root_                       = empty_key_;

public:
  explicit FlatRBTree(
      size_type capacity  = 0,
      Allocator allocator = std::allocator<Node<Key, Value, Size>>())
      : capacity_(capacity), tree_(capacity_, allocator) {
    if (capacity_ == empty_key_) {
      throw std::invalid_argument(
          "Capacity must not be equal to empty_key_. Default empty_key_ = "
          "std::numeric_limits<size_type>::max()");
    }
  }

  ~FlatRBTree()                            = default;
  FlatRBTree(const FlatRBTree&)            = default;
  FlatRBTree& operator=(const FlatRBTree&) = default;
  FlatRBTree(FlatRBTree&&)                 = default;
  FlatRBTree& operator=(FlatRBTree&&)      = default;

  // Operators
  bool operator==(const FlatRBTree& lhs) {
    if (lhs.size_ != size_) {
      return false;
    }
    for (int i {}; i < size_; ++i) {
      if (tree_[i] != lhs.tree_[i]) {
        return false;
      }
    }
    return true;
  }

  // Member Function
  [[nodiscard]] size_type get_allocator() const {
    return tree_.get_allocator();
  }

  // Iterators
  void begin() {}

  void cbegin() const {}

  void end() {}

  void cend() const {}

  void rbegin() {}

  void crbegin() const {}

  void rend() {}

  void crend() const {}

  // Capacity
  [[nodiscard]] size_type size() const noexcept { return size_; }

  [[nodiscard]] size_type max_size() const noexcept { return empty_key_; }

  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

  [[nodiscard]] size_type capacity() const noexcept { return capacity_; }

  void reserve(size_type new_cap) { tree_.reserve(new_cap); }

  void shrink_to_fit() {
    while (capacity_ > size_) {
      tree_.pop_back();
      --capacity_;
    }
  }

  // Modifiers
  void clear() const noexcept { size_ = 0; }

  void insert(key_type key, value_type value) { _insert(key, value); }

  template <typename... Args>
  void emplace(key_type key, value_type value, Args&&... args) {
    _insert(key, value);
  }

  void erase(key_type key) { _erase(key); }

  void swap() {}

  void extract() {}

  void merge() {}

  // Lookup
  [[nodiscard]] size_type count(key_type key) const noexcept {
    return contains(key);
  }
  // [[nodiscard]] iterator find() const noexcept {
  // return capacity_;
  // }

  [[nodiscard]] bool contains(key_type key) const noexcept {
    return _find_index(key) != empty_key_;
  }

  // [[nodiscard]] iterator equal_range() const noexcept {
  // return capacity_;
  // }

  // // [[nodiscard]] iterator lower_bound() const noexcept {
  // return capacity_;
  // }

  // [[nodiscard]] iterator upper_bound() const noexcept {
  // return capacity_;
  // }// }

  // Observers
  [[nodiscard]] Compare key_comp() const noexcept { return Compare(); }

  [[nodiscard]] Compare value_comp() const noexcept { return Compare(); }

private:
  void _insert(key_type key, value_type value) {
    // Need 1 spot to represent emptiness
    if (size_ == empty_key_) {
      throw std::runtime_error("Size exceeds ");
    }
    size_type index  = size_;
    size_type parent = empty_key_;
    size_type node   = root_;
    // Find correct insertion location
    while (node != empty_key_) {
      parent          = node;
      auto& parentRef = tree_[parent];
      if (key == parentRef.key_) {
        return;
      }
      if (key < parentRef.key_) {
        node = parentRef.left_;
      } else {
        node = parentRef.right_;
      }
    }
    // Create Node in the end of the tree
    if (size_ == capacity_) {
      tree_.emplace_back(key, value, parent, empty_key_, empty_key_, RED_);
      ++capacity_;
    } else {
      Node<key_type, value_type, size_type> node(key, value, parent, empty_key_,
                                                 empty_key_, RED_);
      tree_[size_] = node;
    }
    ++size_;
    // Update root_
    if (! index) {
      root_               = index;
      tree_[root_].color_ = BLACK_;
      return;
    }
    // Update parent with new child
    auto& parentRef = tree_[parent];
    if (key < parentRef.key_) {
      parentRef.left_ = index;
    } else {
      parentRef.right_ = index;
    }
    _fixInsert(index);
  }

  void _erase(key_type key) {
    size_type matched_key = _find_index(key);

    if (matched_key == empty_key_) {
      return;
    }

    auto& matchedRef = tree_[matched_key];
    bool color       = matchedRef.color_;
    size_type parent = matchedRef.parent_;
    size_type child  = empty_key_;

    if (matchedRef.left_ == empty_key_ || matchedRef.right_ == empty_key_) {
      if (matchedRef.left_ == empty_key_) {
        child = matchedRef.right_;
      } else {
        child = matchedRef.left_;
      }
      if (child != empty_key_) {
        tree_[child].parent_ = matchedRef.parent_;
      }
      _updateParentChild(child, parent, matched_key);
      _swapOutOfTree(child, matched_key, child, parent);
    } else {
      size_type minNode = _minValueNode(matched_key);
      child             = tree_[minNode].right_;
      parent            = tree_[minNode].parent_;
      color             = tree_[minNode].color_;

      if (child != empty_key_) {
        tree_[child].parent_ = parent;
      }
      if (parent == matched_key) {
        tree_[parent].right_ = child;
        parent               = minNode;
      } else {
        tree_[parent].left_ = child;
      }
      _transferData(minNode, matched_key);
      _updateParentChild(minNode, matchedRef.parent_, matched_key);

      tree_[matchedRef.left_].parent_ = minNode;
      if (matchedRef.right_ != empty_key_) {
        tree_[matchedRef.right_].parent_ = minNode;
      }
      _swapOutOfTree(minNode, matched_key, child, parent);
    }
    if (color == BLACK_) {
      _fixErase(child, parent);
    }
    --size_;
  }

  size_type _find_index(key_type key) {
    size_type node = root_;
    // Find node with binary search
    while (node != empty_key_) {
      auto& nodeRef = tree_[node];
      if (nodeRef.key_ == key) {
        return node;
      }
      if (nodeRef.key_ < key) {
        node = nodeRef.right_;
      } else {
        node = nodeRef.left_;
      }
    }
    return empty_key_;
  }

  void _transferData(size_type nodeLeft, size_type nodeRight) {
    auto& nodeLeftRef   = tree_[nodeLeft];
    auto& nodeRightRef  = tree_[nodeRight];
    nodeLeftRef.parent_ = nodeRightRef.parent_;
    nodeLeftRef.color_  = nodeRightRef.color_;
    nodeLeftRef.left_   = nodeRightRef.left_;
    nodeLeftRef.right_  = nodeRightRef.right_;
  }

  void _updateParentChild(size_type child, size_type parent,
                          size_type matched_key) {
    if (parent != empty_key_) {
      auto& parentRef = tree_[parent];
      if (parentRef.left_ == matched_key) {
        parentRef.left_ = child;
      } else {
        parentRef.right_ = child;
      }
    } else {
      root_ = child;
    }
  }

  void _fixInsert(size_type node) {
    size_type parent;
    while ((parent = tree_[node].parent_) && tree_[parent].color_ == RED_) {

      auto& parentRef       = tree_[parent];
      size_type grandparent = parentRef.parent_;
      auto& grandparentRef  = tree_[grandparent];

      if (parent == grandparentRef.left_) {
        size_type uncle = grandparentRef.right_;
        auto& uncleRef  = tree_[uncle];
        if (uncle != empty_key_ && uncleRef.color_ == RED_) {
          _updateInsertColors(uncleRef, parentRef, grandparentRef);
          node = grandparent;
        } else {
          if (uncle != empty_key_) {
            _swapNodePosition(uncle, node);
            node = uncle;
          }
          if (node == parentRef.right_) {
            _rotateLeft(parent);
          }
          _rotateRight(grandparent);
          std::swap(parentRef.color_, grandparentRef.color_);
          node = parent;
        }
      } else {
        size_type uncle = grandparentRef.left_;
        auto& uncleRef  = tree_[uncle];
        if (uncle != empty_key_ && uncleRef.color_ == RED_) {
          _updateInsertColors(uncleRef, parentRef, grandparentRef);
          node = grandparent;
        } else {
          if (uncle != empty_key_) {
            _swapNodePosition(uncle, node);
            node = uncle;
          }
          if (node == parentRef.left_) {
            _rotateRight(parent);
          }
          _rotateLeft(grandparent);
          std::swap(parentRef.color_, grandparentRef.color_);
          node = parent;
        }
      }
    }
    tree_[root_].color_ = BLACK_;
  }

  void _updateInsertColors(auto& uncleRef, auto& parentRef,
                           auto& grandparentRef) {
    grandparentRef.color_ = RED_;
    parentRef.color_      = BLACK_;
    uncleRef.color_       = BLACK_;
  }

  void _fixErase(size_type node, size_type parent) {
    size_type sibling = empty_key_;
    while (node != root_ &&
           (node == empty_key_ || tree_[node].color_ == BLACK_)) {

      auto& nodeRef   = tree_[node];
      auto& parentRef = tree_[parent];

      if (node == parentRef.left_) {
        sibling = parentRef.right_;
        if (tree_[sibling].color_ == RED_) {
          tree_[sibling].color_ = BLACK_;
          parentRef.color_      = RED_;
          parent                = _rotateLeft(parent);
          sibling               = tree_[parent].right_;
        }
        auto& siblingRef = tree_[sibling];
        if ((siblingRef.left_ == empty_key_ ||
             tree_[siblingRef.left_].color_ == BLACK_) &&
            (siblingRef.right_ == empty_key_ ||
             tree_[siblingRef.right_].color_ == BLACK_)) {
          siblingRef.color_ = RED_;
          node              = parent;
          parent            = tree_[node].parent_;
        } else {
          if (siblingRef.right_ == empty_key_ ||
              tree_[siblingRef.right_].color_ == BLACK_) {
            if (siblingRef.left_ != empty_key_) {
              tree_[siblingRef.left_].color_ = BLACK_;
            }
            siblingRef.color_ = RED_;
            sibling           = _rotateRight(sibling);
            sibling           = tree_[parent].right_;
          }
          tree_[sibling].color_ = tree_[parent].color_;
          tree_[parent].color_  = BLACK_;
          if (tree_[sibling].right_ != empty_key_) {
            tree_[tree_[sibling].right_].color_ = BLACK_;
          }
          _rotateLeft(parent);
          node = root_;
          break;
        }
      } else {
        size_type sibling = parentRef.left_;
        if (tree_[sibling].color_ == RED_) {
          tree_[sibling].color_ = BLACK_;
          parentRef.color_      = RED_;
          parent                = _rotateRight(parent);
          sibling               = tree_[parent].left_;
        }
        auto& siblingRef = tree_[sibling];
        if ((siblingRef.left_ == empty_key_ ||
             tree_[siblingRef.left_].color_ == BLACK_) &&
            (siblingRef.right_ == empty_key_ ||
             tree_[siblingRef.right_].color_ == BLACK_)) {
          siblingRef.color_ = RED_;
          node              = parent;
          parent            = tree_[node].parent_;
        } else {
          if (siblingRef.left_ == empty_key_ ||
              tree_[siblingRef.left_].color_ == BLACK_) {
            if (siblingRef.right_ != empty_key_) {
              tree_[siblingRef.right_].color_ = BLACK_;
            }
            siblingRef.color_ = RED_;
            sibling           = _rotateLeft(sibling);
            sibling           = tree_[parent].left_;
          }
          tree_[sibling].color_ = tree_[parent].color_;
          tree_[parent].color_  = BLACK_;
          if (tree_[sibling].left_ != empty_key_) {
            tree_[tree_[sibling].left_].color_ = BLACK_;
          }
          _rotateRight(parent);
          node = root_;
          break;
        }
      }
    }
    if (node != empty_key_) {
      tree_[node].color_ = BLACK_;
    }
  }

  size_type _rotateLeft(size_type node) {
    auto& nodeRef   = tree_[node];
    size_type child = nodeRef.right_;
    auto& childRef  = tree_[child];
    // Update Children
    size_type childRight = childRef.right_;
    size_type nodeLeft   = nodeRef.left_;
    if (childRight != empty_key_) {
      tree_[childRight].parent_ = node;
    }
    if (nodeLeft != empty_key_) {
      tree_[nodeLeft].parent_ = child;
    }
    // Edge Case
    key_type temp = std::move(nodeRef.key_);
    nodeRef.key_  = std::move(childRef.key_);
    childRef.key_ = std::move(temp);

    std::swap(nodeRef.value_, childRef.value_);

    bool colorTemp  = std::move(nodeRef.color_);
    nodeRef.color_  = std::move(childRef.color_);
    childRef.color_ = std::move(colorTemp);

    size_type nodeLeftTemp = std::move(nodeRef.left_);
    nodeRef.left_          = std::move(childRef.right_);
    childRef.right_        = std::move(nodeLeftTemp);

    nodeLeftTemp   = std::move(nodeRef.left_);
    nodeRef.left_  = std::move(nodeRef.right_);
    nodeRef.right_ = std::move(nodeLeftTemp);

    size_type childLeftTemp = std::move(childRef.left_);
    childRef.left_          = std::move(childRef.right_);
    childRef.right_         = std::move(childLeftTemp);
    return child;
  }

  size_type _rotateRight(size_type node) {
    auto& nodeRef   = tree_[node];
    size_type child = tree_[node].left_;
    auto& childRef  = tree_[child];
    // Update Children
    size_type childLeft = childRef.left_;
    size_type nodeRight = nodeRef.right_;
    if (childLeft != empty_key_) {
      tree_[childLeft].parent_ = node;
    }
    if (nodeRight != empty_key_) {
      tree_[nodeRight].parent_ = child;
    }
    // Touches less memory, more code, less computation
    std::swap(nodeRef.key_, childRef.key_);
    std::swap(nodeRef.value_, childRef.value_);
    std::swap(nodeRef.color_, childRef.color_);
    std::swap(nodeRef.right_, childRef.left_);
    std::swap(nodeRef.left_, nodeRef.right_);
    std::swap(childRef.left_, childRef.right_);
    return child;
  }

  void _swapNodePosition(size_type nodeA, size_type nodeB) {
    if (nodeA == nodeB) {
      return;
    }
    // Saves computation time (~3-4 nanoseconds)
    auto& nodeARef        = tree_[nodeA];
    auto& nodeBRef        = tree_[nodeB];
    size_type nodeAParent = nodeARef.parent_;
    size_type nodeBParent = nodeBRef.parent_;
    size_type nodeALeft   = nodeARef.left_;
    size_type nodeARight  = nodeARef.right_;
    size_type nodeBLeft   = nodeBRef.left_;
    size_type nodeBRight  = nodeBRef.right_;
    auto& nodeAParentRef  = tree_[nodeAParent];
    auto& nodeBParentRef  = tree_[nodeBParent];
    auto& nodeALeftRef    = tree_[nodeALeft];
    auto& nodeARightRef   = tree_[nodeARight];
    auto& nodeBLeftRef    = tree_[nodeBLeft];
    auto& nodeBRightRef   = tree_[nodeBRight];

    // Swap Parent Index
    if (nodeAParent != empty_key_) {
      if (nodeAParentRef.left_ == nodeA) {
        nodeAParentRef.left_ = nodeB;
      } else {
        nodeAParentRef.right_ = nodeB;
      }
    }
    if (nodeBParent != empty_key_) {
      if (nodeBParentRef.left_ == nodeB) {
        nodeBParentRef.left_ = nodeA;
      } else {
        nodeBParentRef.right_ = nodeA;
      }
    }
    // Check if nodes have relationhip
    if (nodeAParent == nodeB) {
      nodeARef.parent_ = nodeA;
    }
    if (nodeBParent == nodeA) {
      nodeBRef.parent_ = nodeB;
    }
    // Swap Children Index
    if (nodeALeft != empty_key_) {
      nodeALeftRef.parent_ = nodeB;
    }
    if (nodeARight != empty_key_) {
      nodeARightRef.parent_ = nodeB;
    }
    if (nodeBLeft != empty_key_) {
      nodeBLeftRef.parent_ = nodeA;
    }
    if (nodeBRight != empty_key_) {
      nodeBRightRef.parent_ = nodeA;
    }
    // Update Root if in swap
    root_ = (root_ == nodeA) ? nodeB : (root_ == nodeB) ? nodeA : root_;
    // Swap vector position
    std::swap(nodeARef, nodeBRef);
  }

  void _swapOutOfTree(size_type node, size_type removeNode, size_type& child,
                      size_type& parent) {
    if (node != empty_key_) {
      _swapNodePositionRemove(node, removeNode, child, parent);
      removeNode = node;
    }
    _swapNodePositionRemove(size_ - 1, removeNode, child, parent);
  }

  void _swapNodePositionRemove(size_type nodeA, size_type nodeB,
                               size_type& child, size_type& parent) {
    if (nodeA == nodeB) {
      return;
    }
    if (nodeA == child) {
      child = nodeB;
    }
    if (nodeA == parent) {
      parent = nodeB;
    }
    auto& nodeARef        = tree_[nodeA];
    size_type nodeAParent = nodeARef.parent_;
    size_type nodeALeft   = nodeARef.left_;
    size_type nodeARight  = nodeARef.right_;
    auto& nodeAParentRef  = tree_[nodeAParent];
    auto& nodeALeftRef    = tree_[nodeALeft];
    auto& nodeARightRef   = tree_[nodeARight];

    // Swap Parent Index
    if (nodeAParent != empty_key_) {
      if (nodeAParentRef.left_ == nodeA) {
        nodeAParentRef.left_ = nodeB;
      } else {
        nodeAParentRef.right_ = nodeB;
      }
    }
    // Swap Children Index
    if (nodeALeft != empty_key_) {
      nodeALeftRef.parent_ = nodeB;
    }
    if (nodeARight != empty_key_) {
      nodeARightRef.parent_ = nodeB;
    }
    // Update Root if in swap
    root_ = (root_ == nodeA) ? nodeB : root_;
    // Swap vector position
    std::swap(nodeARef, tree_[nodeB]);
  }

  size_type _minValueNode(size_type node) {
    node           = tree_[node].right_;
    size_type left = empty_key_;
    while ((left = tree_[node].left_) != empty_key_) { node = left; }
    return node;
  }

  size_type _first() {
    size_type node = root_;
    size_type left = empty_key_;
    while ((left = tree_[node].left_) != empty_key_) { node = left; }
    return node;
  }

  size_type _last() {
    size_type node  = root_;
    size_type right = empty_key_;
    while ((right = tree_[node].right_) != empty_key_) { node = right; }
    return node;
  }

  size_type _next(size_type node) {

    if (node == empty_key_) {
      return node;
    }
    if (tree_[node].right_ != empty_key_) {
      node           = tree_[node].right_;
      size_type left = empty_key_;
      while ((left = tree_[node].left_) != empty_key_) { node = left; }
      return node;
    }

    size_type parent = empty_key_;
    while ((parent = tree_[node].parent_) && node == tree_[parent].right_) {
      node = parent;
    }
    return parent;
  }

  size_type _prev(size_type node) {

    if (node == empty_key_) {
      return node;
    }
    if (tree_[node].left_ != empty_key_) {
      node            = tree_[node].left_;
      size_type right = empty_key_;
      while ((right = tree_[node].right_) != empty_key_) { node = right; }
      return node;
    }

    size_type parent = empty_key_;
    while ((parent = tree_[node].parent_) && node == tree_[parent].left_) {
      node = parent;
    }
    return parent;
  }
};

}// namespace details

template <typename Key, typename Value, typename Size = std::size_t,
          typename Compare   = std::less<Key>,
          typename Allocator = std::allocator<details::Node<Key, Value, Size>>>
class FlatMap {};

template <typename Key, typename Size = std::size_t,
          typename Compare = std::less<Key>,
          typename Allocator =
              std::allocator<details::Node<Key, details::EmptyType, Size>>>
class FlatSet {};

}// namespace dro
#endif
