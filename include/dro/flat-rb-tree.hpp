// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.ndrew Drogalis

#ifndef DRO_FLAT_RED_BLACK_TREE
#define DRO_FLAT_RED_BLACK_TREE

#include <charconv>
#include <chrono>
#include <climits>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <limits>
#include <locale>
#include <stdexcept>
#include <string>
#include <type_traits>
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
template <typename Value> struct NodeValue {
  Value value_;
  NodeValue() = default;
  explicit NodeValue(Value value) : value_(value) {}
};

// Set Node
template <> struct NodeValue<EmptyType> {
  EmptyType value_ [[no_unique_address]];
  NodeValue() = default;
  explicit NodeValue(EmptyType value) : value_(value) {}
};

template <typename Key, typename Value, typename Size = std::size_t>
struct Node : public NodeValue<Value> {
  Key key_;
  Size parent_;
  Size left_;
  Size right_;
  bool color_ {};

  Node() = default;

  explicit Node(Key key, Value value, Size parent, Size left, Size right,
                bool color)
      : NodeValue<Value>(value), key_(key), parent_(parent), left_(left),
        right_(right), color_(color) {}
};

template <typename Container> struct Iterator {
  using difference_type   = std::ptrdiff_t;
  using key_type          = Container::key_type;
  using value_type        = Container::value_type;
  using size_type         = Container::size_type;
  using pointer           = key_type*;
  using reference         = key_type&;
  using iterator_category = std::bidirectional_iterator_tag;

  explicit Iterator(Container* flatTree, size_type index, bool reverse = false)
      : flatTree_(flatTree), index_(index), reverse_(reverse) {}

  template <typename OtherContainer>
  explicit Iterator(const Iterator<OtherContainer>& lhs)
      : flatTree_(lhs.flatTree_), index_(lhs.index_), reverse_(lhs.reverse_) {}

  ~Iterator() { delete pair_pointer_; }
  Iterator(const Iterator& lhs) : flatTree_(lhs.flatTree_), index_(lhs.index_) {
    if (lhs.pair_pointer_) {
      pair_pointer_ =
          new Pair(lhs.pair_pointer_->first, lhs.pair_pointer_->second);
    }
  }
  Iterator& operator=(const Iterator& lhs) {
    if (this != lhs) {
      flatTree_ = lhs.flatTree_;
      index_    = lhs.index_;
      if (lhs.pair_pointer_) {
        pair_pointer_ =
            new Pair(lhs.pair_pointer_->first, lhs.pair_pointer_->second);
      }
    }
    return *this;
  }
  Iterator(Iterator&& lhs)
      : pair_pointer_(std::move(lhs.pair_pointer_)), flatTree_(lhs.flatTree_),
        index_(lhs.index_) {
    delete lhs.pair_pointer_;
  }
  Iterator& operator=(Iterator&& lhs) {
    if (this != lhs) {
      flatTree_     = lhs.flatTree_;
      index_        = lhs.index_;
      pair_pointer_ = std::move(lhs.pair_pointer_);
      delete lhs.pair_pointer_;
    }
    return *this;
  }

  struct Pair {
    key_type& first;
    value_type& second;
    Pair(key_type& key, value_type& val) : first(key), second(val) {}
  };

  bool operator==(const Iterator& lhs) const {
    return lhs.flatTree_ == flatTree_ && lhs.index_ == index_;
  }
  bool operator!=(const Iterator& lhs) const { return ! (lhs == *this); }

  Iterator& operator++() {
    if (reverse_) {
      index_ = flatTree_->_prev(index_);
    } else {
      index_ = flatTree_->_next(index_);
    }
    return *this;
  }

  Iterator& operator--() {
    if (reverse_) {
      index_ = flatTree_->_next(index_);
    } else {
      index_ = flatTree_->_prev(index_);
    }
    return *this;
  }

  reference operator*() const
    requires std::is_same_v<value_type, EmptyType>
  {
    return flatTree_->tree_[index_].key_;
  }
  pointer operator->() const
    requires std::is_same_v<value_type, EmptyType>
  {
    return &flatTree_->tree_[index_].key_;
  }

  Pair operator*() const
    requires(! std::is_same_v<value_type, EmptyType>)
  {
    return Pair(flatTree_->tree_[index_].key_, flatTree_->tree_[index_].value_);
  }

  Pair* operator->() const
    requires(! std::is_same_v<value_type, EmptyType>)
  {
    delete pair_pointer_;
    pair_pointer_ = new Pair(&flatTree_->tree_[index_].key_,
                             &flatTree_->tree_[index_].value_);
    return pair_pointer_;
  }

private:
  Pair* pair_pointer_  = nullptr;
  Container* flatTree_ = nullptr;
  size_type index_ {};
  bool reverse_ {};
  friend Container;
};

template <typename Key, typename Value, typename Size = std::size_t,
          typename Compare   = std::less<Key>,
          typename Allocator = std::allocator<Node<Key, Value, Size>>>
class FlatRBTree {

public:
  using key_type        = Key;
  using value_type      = Value;
  using size_type       = Size;
  using difference_type = std::ptrdiff_t;
  using key_compare     = Compare;
  using allocator_type  = Allocator;
  using self_type       = FlatRBTree<Key, Value, Size, Compare, Allocator>;
  // using reference              = typename Container::reference;
  // using const_reference        = typename Container::const_reference;
  // using pointer                = typename Container::pointer;
  // using const_pointer          = typename Container::const_pointer;
  using iterator = Iterator<self_type>;
  using const_iterator =
      Iterator<const FlatRBTree<Key, Value, Size, Compare, Allocator>>;
  using pair_iterator       = std::pair<iterator, iterator>;
  using pair_const_iterator = std::pair<const_iterator, const_iterator>;
  // using reverse_iterator       = typename Container::reverse_iterator;
  // using const_reverse_iterator = typename Container::const_reverse_iterator;

private:
  // Constants
  constexpr static bool RED_   = false;
  constexpr static bool BLACK_ = true;

  size_type capacity_ {};
  size_type size_ {};

  friend iterator;

#ifndef NDEBUG

public:
#else

private:
#endif
  std::vector<Node<Key, Value, Size>> tree_;
  constexpr static size_type empty_index_ =
      std::numeric_limits<size_type>::max();
  size_type root_ = empty_index_;

public:
  explicit FlatRBTree(
      size_type capacity  = 0,
      Allocator allocator = std::allocator<Node<Key, Value, Size>>())
      : capacity_(capacity), tree_(capacity_, allocator) {
    if (capacity_ == empty_index_) {
      throw std::invalid_argument(
          "Capacity must not be equal to empty_index_. Default empty_index_ = "
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

  // Element Access
  value_type& at(const key_type& key)
    requires(! std::is_same_v<value_type, EmptyType>)
  {
    size_type index = _find_index(key);
    if (index == empty_index_) {
      throw std::out_of_range("Key not found");
    }
    return &tree_[index].value_;
  }

  const value_type& at(const key_type& key) const
    requires(! std::is_same_v<value_type, EmptyType>)
  {
    size_type index = _find_index(key);
    if (index == empty_index_) {
      throw std::out_of_range("Key not found");
    }
    return &tree_[index].value_;
  }

  value_type& operator[](const key_type& key)
    requires(! std::is_same_v<value_type, EmptyType>)
  {}

  // Iterators
  iterator begin() { return iterator(this, _first()); }

  const_iterator begin() const { return const_iterator(this, _first()); }

  const_iterator cbegin() const noexcept {
    return const_iterator(this, _first());
  }

  iterator end() { return iterator(this, empty_index_); }

  const_iterator end() const { return const_iterator(this, empty_index_); }

  const_iterator cend() const noexcept {
    return const_iterator(this, empty_index_);
  }

  iterator rbegin() { return iterator(this, _last(), true); }

  const_iterator rbegin() const { return const_iterator(this, _last(), true); }

  const_iterator crbegin() const noexcept {
    return const_iterator(this, _last(), true);
  }

  iterator rend() { return iterator(*this, empty_index_, true); }

  const_iterator rend() const {
    return const_iterator(this, empty_index_, true);
  }

  const_iterator crend() const noexcept {
    return const_iterator(this, empty_index_, true);
  }

  // Capacity
  [[nodiscard]] size_type size() const noexcept { return size_; }

  [[nodiscard]] size_type max_size() const noexcept { return empty_index_; }

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

  std::pair<iterator, bool> insert(key_type key, value_type value)
    requires(! std::is_same_v<value_type, EmptyType>)
  {
    bool success = _insert(key, value);
    return std::make_pair(find(key), success);
  }

  std::pair<iterator, bool> insert(const key_type& key)
    requires std::is_same_v<value_type, EmptyType>
  {
    bool success = _insert(key, EmptyType());
    return std::make_pair(find(key), success);
  }

  std::pair<iterator, bool> insert(key_type&& key)
    requires std::is_same_v<value_type, EmptyType>
  {
    bool success = _insert(key, EmptyType());
    return std::make_pair(find(key), success);
  }

  template <typename... Args>
  void emplace(key_type key, value_type value, Args&&... args) {
    _insert(key, value);
  }

  size_type erase(key_type key) { return _erase(key); }

  void swap(FlatRBTree& other) noexcept(FlatTree_NoThrow_Type<Key> &&
                                        FlatTree_NoThrow_Type<Value>) {
    std::swap(*this, other);
  }

  void extract() {}

  void merge() {}

  // Lookup
  [[nodiscard]] size_type count(const key_type& key) const {
    return contains(key);
  }
  template <typename K> [[nodiscard]] size_type count(const K& x) const {
    return contains(x);
  }

  [[nodiscard]] iterator find(const key_type& key) {
    return iterator(this, _find_index(key));
  }
  [[nodiscard]] const_iterator find(const key_type& key) const {
    return iterator(this, _find_index(key));
  }
  template <typename K> [[nodiscard]] iterator find(const K& x) {
    return iterator(this, _find_index(x));
  }
  template <typename K> [[nodiscard]] const_iterator find(const K& x) const {
    return iterator(this, _find_index(x));
  }

  [[nodiscard]] bool contains(const key_type& key) const {
    return _find_index(key) != empty_index_;
  }
  template <typename K> [[nodiscard]] bool contains(const K& x) const {
    return _find_index(x) != empty_index_;
  }

  [[nodiscard]] pair_iterator equal_range(const key_type& key) {
    return std::pair<iterator, iterator>(lower_bound(key), upper_bound(key));
  }
  [[nodiscard]] pair_const_iterator equal_range(const key_type& key) const {
    return std::pair<const_iterator, const_iterator>(lower_bound(key),
                                                     upper_bound(key));
  }
  template <typename K> [[nodiscard]] pair_iterator equal_range(const K& x) {
    return std::pair<iterator, iterator>(lower_bound(x), upper_bound(x));
  }
  template <typename K>
  [[nodiscard]] pair_const_iterator equal_range(const K& x) const {
    return std::pair<const_iterator, const_iterator>(lower_bound(x),
                                                     upper_bound(x));
  }

  [[nodiscard]] iterator lower_bound(const key_type& key) {
    return iterator(this, _notLessThan(key));
  }
  [[nodiscard]] const_iterator lower_bound(const key_type& key) const {
    return iterator(this, _notLessThan(key));
  }
  template <typename K> [[nodiscard]] iterator lower_bound(const K& x) {
    return iterator(this, _notLessThan(x));
  }
  template <typename K>
  [[nodiscard]] const_iterator lower_bound(const K& x) const {
    return iterator(this, _notLessThan(x));
  }

  [[nodiscard]] iterator upper_bound(const key_type& key) {
    return iterator(this, _greaterThan(key));
  }
  [[nodiscard]] const_iterator upper_bound(const key_type& key) const {
    return iterator(this, _greaterThan(key));
  }
  template <typename K> [[nodiscard]] iterator upper_bound(const K& x) {
    return iterator(this, _greaterThan(x));
  }
  template <typename K>
  [[nodiscard]] const_iterator upper_bound(const K& x) const {
    return iterator(this, _greaterThan(x));
  }

  // Observers
  [[nodiscard]] Compare key_comp() const noexcept { return Compare(); }

  [[nodiscard]] Compare value_comp() const noexcept { return Compare(); }

private:
  bool _insert(key_type key, value_type value) {
    // Need 1 spot to represent emptiness
    if (size_ == empty_index_) {
      throw std::runtime_error("Size exceeds ");
    }
    size_type index  = size_;
    size_type parent = empty_index_;
    size_type node   = root_;
    // Find correct insertion location
    while (node != empty_index_) {
      parent          = node;
      auto& parentRef = tree_[parent];
      if (key == parentRef.key_) {
        return false;
      }
      if (key < parentRef.key_) {
        node = parentRef.left_;
      } else {
        node = parentRef.right_;
      }
    }
    // Create Node in the end of the tree
    if (size_ == capacity_) {
      tree_.emplace_back(key, value, parent, empty_index_, empty_index_, RED_);
      ++capacity_;
    } else {
      Node<key_type, value_type, size_type> node(
          key, value, parent, empty_index_, empty_index_, RED_);
      tree_[size_] = node;
    }
    ++size_;
    // Update root_
    if (! index) {
      root_               = index;
      tree_[root_].color_ = BLACK_;
      return true;
    }
    // Update parent with new child
    auto& parentRef = tree_[parent];
    if (key < parentRef.key_) {
      parentRef.left_ = index;
    } else {
      parentRef.right_ = index;
    }
    _fixInsert(index);
    return true;
  }

  bool _erase(key_type key) {
    size_type matched_key = _find_index(key);

    if (matched_key == empty_index_) {
      return false;
    }

    auto& matchedRef = tree_[matched_key];
    bool color       = matchedRef.color_;
    size_type parent = matchedRef.parent_;
    size_type child  = empty_index_;

    if (matchedRef.left_ == empty_index_ || matchedRef.right_ == empty_index_) {
      if (matchedRef.left_ == empty_index_) {
        child = matchedRef.right_;
      } else {
        child = matchedRef.left_;
      }
      if (child != empty_index_) {
        tree_[child].parent_ = matchedRef.parent_;
      }
      _updateParentChild(child, parent, matched_key);
      _swapOutOfTree(child, matched_key, child, parent);
    } else {
      size_type minNode = _minValueNode(matched_key);
      child             = tree_[minNode].right_;
      parent            = tree_[minNode].parent_;
      color             = tree_[minNode].color_;

      if (child != empty_index_) {
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
      if (matchedRef.right_ != empty_index_) {
        tree_[matchedRef.right_].parent_ = minNode;
      }
      _swapOutOfTree(minNode, matched_key, child, parent);
    }
    if (color == BLACK_) {
      _fixErase(child, parent);
    }
    --size_;
    return true;
  }

  size_type _find_index(key_type key) {
    size_type node = root_;
    // Find node with binary search
    while (node != empty_index_) {
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
    return empty_index_;
  }

  size_type _greaterThan(key_type key) { return empty_index_; }

  size_type _notLessThan(key_type key) { return empty_index_; }

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
    if (parent != empty_index_) {
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
        if (uncle != empty_index_ && uncleRef.color_ == RED_) {
          _updateInsertColors(uncleRef, parentRef, grandparentRef);
          node = grandparent;
        } else {
          if (uncle != empty_index_) {
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
        if (uncle != empty_index_ && uncleRef.color_ == RED_) {
          _updateInsertColors(uncleRef, parentRef, grandparentRef);
          node = grandparent;
        } else {
          if (uncle != empty_index_) {
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
    size_type sibling = empty_index_;
    while (node != root_ &&
           (node == empty_index_ || tree_[node].color_ == BLACK_)) {

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
        if ((siblingRef.left_ == empty_index_ ||
             tree_[siblingRef.left_].color_ == BLACK_) &&
            (siblingRef.right_ == empty_index_ ||
             tree_[siblingRef.right_].color_ == BLACK_)) {
          siblingRef.color_ = RED_;
          node              = parent;
          parent            = tree_[node].parent_;
        } else {
          if (siblingRef.right_ == empty_index_ ||
              tree_[siblingRef.right_].color_ == BLACK_) {
            if (siblingRef.left_ != empty_index_) {
              tree_[siblingRef.left_].color_ = BLACK_;
            }
            siblingRef.color_ = RED_;
            sibling           = _rotateRight(sibling);
            sibling           = tree_[parent].right_;
          }
          tree_[sibling].color_ = tree_[parent].color_;
          tree_[parent].color_  = BLACK_;
          if (tree_[sibling].right_ != empty_index_) {
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
        if ((siblingRef.left_ == empty_index_ ||
             tree_[siblingRef.left_].color_ == BLACK_) &&
            (siblingRef.right_ == empty_index_ ||
             tree_[siblingRef.right_].color_ == BLACK_)) {
          siblingRef.color_ = RED_;
          node              = parent;
          parent            = tree_[node].parent_;
        } else {
          if (siblingRef.left_ == empty_index_ ||
              tree_[siblingRef.left_].color_ == BLACK_) {
            if (siblingRef.right_ != empty_index_) {
              tree_[siblingRef.right_].color_ = BLACK_;
            }
            siblingRef.color_ = RED_;
            sibling           = _rotateLeft(sibling);
            sibling           = tree_[parent].left_;
          }
          tree_[sibling].color_ = tree_[parent].color_;
          tree_[parent].color_  = BLACK_;
          if (tree_[sibling].left_ != empty_index_) {
            tree_[tree_[sibling].left_].color_ = BLACK_;
          }
          _rotateRight(parent);
          node = root_;
          break;
        }
      }
    }
    if (node != empty_index_) {
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
    if (childRight != empty_index_) {
      tree_[childRight].parent_ = node;
    }
    if (nodeLeft != empty_index_) {
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
    if (childLeft != empty_index_) {
      tree_[childLeft].parent_ = node;
    }
    if (nodeRight != empty_index_) {
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
    if (nodeAParent != empty_index_) {
      if (nodeAParentRef.left_ == nodeA) {
        nodeAParentRef.left_ = nodeB;
      } else {
        nodeAParentRef.right_ = nodeB;
      }
    }
    if (nodeBParent != empty_index_) {
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
    if (nodeALeft != empty_index_) {
      nodeALeftRef.parent_ = nodeB;
    }
    if (nodeARight != empty_index_) {
      nodeARightRef.parent_ = nodeB;
    }
    if (nodeBLeft != empty_index_) {
      nodeBLeftRef.parent_ = nodeA;
    }
    if (nodeBRight != empty_index_) {
      nodeBRightRef.parent_ = nodeA;
    }
    // Update Root if in swap
    root_ = (root_ == nodeA) ? nodeB : (root_ == nodeB) ? nodeA : root_;
    // Swap vector position
    std::swap(nodeARef, nodeBRef);
  }

  void _swapOutOfTree(size_type node, size_type removeNode, size_type& child,
                      size_type& parent) {
    if (node != empty_index_) {
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
    if (nodeAParent != empty_index_) {
      if (nodeAParentRef.left_ == nodeA) {
        nodeAParentRef.left_ = nodeB;
      } else {
        nodeAParentRef.right_ = nodeB;
      }
    }
    // Swap Children Index
    if (nodeALeft != empty_index_) {
      nodeALeftRef.parent_ = nodeB;
    }
    if (nodeARight != empty_index_) {
      nodeARightRef.parent_ = nodeB;
    }
    // Update Root if in swap
    root_ = (root_ == nodeA) ? nodeB : root_;
    // Swap vector position
    std::swap(nodeARef, tree_[nodeB]);
  }

  size_type _minValueNode(size_type node) {
    node           = tree_[node].right_;
    size_type left = empty_index_;
    while ((left = tree_[node].left_) != empty_index_) { node = left; }
    return node;
  }

  size_type _first() {
    size_type node = root_;
    size_type left = empty_index_;
    while ((left = tree_[node].left_) != empty_index_) { node = left; }
    return node;
  }

  size_type _last() {
    size_type node  = root_;
    size_type right = empty_index_;
    while ((right = tree_[node].right_) != empty_index_) { node = right; }
    return node;
  }

  size_type _next(size_type node) {
    if (node == size_ - 1 || node == empty_index_) {
      return empty_index_;
    }
    if (tree_[node].right_ != empty_index_) {
      node           = tree_[node].right_;
      size_type left = empty_index_;
      while ((left = tree_[node].left_) != empty_index_) { node = left; }
      return node;
    }

    size_type parent = empty_index_;
    while ((parent = tree_[node].parent_) && parent != empty_index_ &&
           node == tree_[parent].right_) {
      node = parent;
    }
    return parent;
  }

  size_type _prev(size_type node) {
    if (node == size_ - 1 || node == empty_index_) {
      return empty_index_;
    }
    if (tree_[node].left_ != empty_index_) {
      node            = tree_[node].left_;
      size_type right = empty_index_;
      while ((right = tree_[node].right_) != empty_index_) { node = right; }
      return node;
    }

    size_type parent = empty_index_;
    while ((parent = tree_[node].parent_) && parent != empty_index_ &&
           node == tree_[parent].left_) {
      node = parent;
    }
    return parent;
  }
};

}// namespace details

template <typename Key, typename Value, typename Size = std::size_t,
          typename Compare   = std::less<Key>,
          typename Allocator = std::allocator<details::Node<Key, Value, Size>>>
class FlatMap
    : public details::FlatRBTree<Key, Value, Size, Compare, Allocator> {
  using size_type = Size;
  using tree_type =
      details::FlatRBTree<Key, details::EmptyType, Size, Compare, Allocator>;
  using node_type = details::Node<Key, details::EmptyType, size_type>;

public:
  explicit FlatMap(size_type capacity  = 0,
                   Allocator allocator = std::allocator<node_type>())
      : tree_type(capacity, allocator) {}
};

template <typename Key, typename Size = std::size_t,
          typename Compare = std::less<Key>,
          typename Allocator =
              std::allocator<details::Node<Key, details::EmptyType, Size>>>
class FlatSet : public details::FlatRBTree<Key, details::EmptyType, Size,
                                           Compare, Allocator> {
  using size_type = Size;
  using tree_type =
      details::FlatRBTree<Key, details::EmptyType, Size, Compare, Allocator>;
  using node_type = details::Node<Key, details::EmptyType, size_type>;

public:
  explicit FlatSet(size_type capacity  = 0,
                   Allocator allocator = std::allocator<node_type>())
      : tree_type(capacity, allocator) {}
};
}// namespace dro
#endif
