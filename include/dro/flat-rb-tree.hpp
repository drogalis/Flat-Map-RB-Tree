// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.ndrew Drogalis

#ifndef DRO_FLAT_RED_BLACK_TREE
#define DRO_FLAT_RED_BLACK_TREE

#include <algorithm>  // for std::min
#include <concepts>   // for requires
#include <cstddef>    // for size_t, ptrdiff_t
#include <functional> // for less
#include <iterator>   // for pair, bidirectional_iterator_tag
#include <limits>     // for numeric_limits
#include <stdexcept>  // for out_of_range, runtime_error
#include <type_traits>// for std::is_default_constructible
#include <utility>    // for pair, forward
#include <vector>     // for vector, allocator

namespace dro {

namespace details {

template <typename T>
concept FlatTree_Type =
    std::is_default_constructible_v<T> &&
    (std::is_move_assignable_v<T> || std::is_copy_assignable_v<T>);

template <typename T, typename... Args>
concept FlatTree_NoThrow =
    std::is_nothrow_constructible_v<T, Args&&...> &&
    ((std::is_nothrow_copy_assignable_v<T> && std::is_copy_assignable_v<T>) ||
     (std::is_nothrow_move_assignable_v<T> && std::is_move_assignable_v<T>));

template <typename T>
concept Integral = std::is_integral_v<T>;

struct FlatSetEmptyType {};

template <FlatTree_Type Key> struct FlatSetPair {
  Key first;
  FlatSetEmptyType second [[no_unique_address]];
  FlatSetPair() = default;
  bool operator==(const FlatSetPair& other) { return first == other.first; }
  bool operator!=(const FlatSetPair& other) { return ! (*this == other); }
};

template <typename Pair, Integral Size = std::size_t> struct Node {
  using size_type = Size;
  // This empty_index_ definition goes against DRY. Not sure most elegant
  // solution that also enforces correctness
  constexpr static size_type empty_index_ =
      std::numeric_limits<size_type>::max();

  Pair pair_;
  Size parent_ {empty_index_};
  Size left_ {empty_index_};
  Size right_ {empty_index_};
  bool color_ {};

  Node() = default;
  bool operator==(const Node& other) {
    return (pair_ == other.pair_ && parent_ == other.parent_ &&
            left_ == other.left_ && right_ == other.right_ &&
            color_ == other.color_);
  }
  bool operator!=(const Node& other) { return ! (*this == other); }
};

template <typename Container> struct FlatTreeIterator {
  using key_type          = typename Container::key_type;
  using mapped_type       = typename Container::mapped_type;
  using value_type        = typename Container::value_type;
  using size_type         = typename Container::size_type;
  using key_compare       = typename Container::key_compare;
  using difference_type   = typename Container::difference_type;
  using pointer           = key_type*;
  using reference         = key_type&;
  using const_pointer     = const key_type*;
  using const_reference   = const key_type&;
  using iterator_category = std::bidirectional_iterator_tag;

  explicit FlatTreeIterator(Container* flatTree, size_type index,
                            bool reverse = false)
      : flatTree_(flatTree), index_(index), reverse_(reverse) {}

  bool operator==(const FlatTreeIterator& other) const {
    return other.flatTree_ == flatTree_ && other.index_ == index_ &&
           other.reverse_ == reverse_;
  }
  bool operator!=(const FlatTreeIterator& other) const {
    return ! (*this == other);
  }

  FlatTreeIterator& operator++() {
    if (reverse_) {
      index_ = flatTree_->_prev(index_);
    } else {
      index_ = flatTree_->_next(index_);
    }
    return *this;
  }

  FlatTreeIterator& operator--() {
    if (reverse_) {
      index_ = flatTree_->_next(index_);
    } else {
      index_ = flatTree_->_prev(index_);
    }
    return *this;
  }

  reference operator*() const
    requires(std::is_same_v<mapped_type, FlatSetEmptyType> &&
             ! std::is_const_v<Container>)
  {
    return flatTree_->tree_[index_].pair_.first;
  }

  const_reference operator*() const
    requires(std::is_same_v<mapped_type, FlatSetEmptyType> &&
             std::is_const_v<Container>)
  {
    return flatTree_->tree_[index_].pair_.first;
  }

  pointer operator->() const
    requires(std::is_same_v<mapped_type, FlatSetEmptyType> &&
             ! std::is_const_v<Container>)
  {
    return &flatTree_->tree_[index_].pair_.first;
  }

  const_pointer operator->() const
    requires(std::is_same_v<mapped_type, FlatSetEmptyType> &&
             std::is_const_v<Container>)
  {
    return &flatTree_->tree_[index_].pair_.first;
  }

  value_type& operator*() const
    requires(! std::is_same_v<mapped_type, FlatSetEmptyType> &&
             ! std::is_const_v<Container>)
  {
    return flatTree_->tree_[index_].pair_;
  }

  const value_type& operator*() const
    requires(! std::is_same_v<mapped_type, FlatSetEmptyType> &&
             std::is_const_v<Container>)
  {
    return flatTree_->tree_[index_].pair_;
  }

  value_type* operator->() const
    requires(! std::is_same_v<mapped_type, FlatSetEmptyType> &&
             ! std::is_const_v<Container>)
  {
    return &flatTree_->tree_[index_].pair_;
  }

  const value_type* operator->() const
    requires(! std::is_same_v<mapped_type, FlatSetEmptyType> &&
             std::is_const_v<Container>)
  {
    return &flatTree_->tree_[index_].pair_;
  }

private:
  Container* flatTree_ = nullptr;
  size_type index_ {};
  bool reverse_ {};
  friend Container;
};

template <FlatTree_Type Key, FlatTree_Type Value, typename Pair,
          Integral Size = std::size_t, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<Node<Pair, Size>>>
class FlatRBTree {

public:
  using key_type        = Key;
  using mapped_type     = Value;
  using value_type      = Pair;
  using size_type       = Size;
  using difference_type = std::ptrdiff_t;
  using key_compare     = Compare;
  using allocator_type  = Allocator;
  using node_type       = Node<Pair, Size>;
  using self_type      = FlatRBTree<Key, Value, Pair, Size, Compare, Allocator>;
  using iterator       = FlatTreeIterator<self_type>;
  using const_iterator = FlatTreeIterator<const self_type>;

private:
  // Constants
  constexpr static bool RED_   = false;
  constexpr static bool BLACK_ = true;

  size_type capacity_ {};
  size_type size_ {};

  friend iterator;
  friend const_iterator;

#ifndef NDEBUG

public:
#else
  // This macro is used to test the tree for correctness. A full tree traversal
  // is done against the STL tree for validation. I've seen several trees with
  // subtle errors due to lack of validation.

private:
#endif
  constexpr static size_type empty_index_ =
      std::numeric_limits<size_type>::max();

  size_type root_ = empty_index_;
  std::vector<node_type> tree_;

public:
  explicit FlatRBTree(size_type capacity = 1, Allocator allocator = Allocator())
      : capacity_(capacity), tree_(capacity_, allocator) {}

  // No memory allocated, this is redundant
  ~FlatRBTree()                            = default;
  FlatRBTree(const FlatRBTree&)            = default;
  FlatRBTree(FlatRBTree&&)                 = default;
  FlatRBTree& operator=(const FlatRBTree&) = default;
  FlatRBTree& operator=(FlatRBTree&&)      = default;

  // Operators
  bool operator==(const FlatRBTree& other) {
    if (size_ != other.size_) {
      return false;
    }
    for (int i {}; i < size_; ++i) {
      if (tree_[i] != other.tree_[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const FlatRBTree& other) { return ! (*this == other); }

  // Member Function
  [[nodiscard]] allocator_type get_allocator() const {
    return tree_.get_allocator();
  }

  // Element Access
  mapped_type& at(const key_type& key)
    requires(! std::is_same_v<mapped_type, FlatSetEmptyType>)
  {
    size_type index = _find_index(key);
    if (index == empty_index_) {
      throw std::out_of_range("Key not found");
    }
    return tree_[index].pair_.second;
  }

  const mapped_type& at(const key_type& key) const
    requires(! std::is_same_v<mapped_type, FlatSetEmptyType>)
  {
    size_type index = _find_index(key);
    if (index == empty_index_) {
      throw std::out_of_range("Key not found");
    }
    return tree_[index].pair_.second;
  }

  mapped_type& operator[](const key_type& key)
    requires(! std::is_same_v<mapped_type, FlatSetEmptyType>)
  {
    size_type index = _find_index(key);
    if (index == empty_index_) {
      index = _emplace(key).first.index_;
    }
    return tree_[index].pair_.second;
  }

  mapped_type& operator[](key_type&& key)
    requires(! std::is_same_v<mapped_type, FlatSetEmptyType>)
  {
    size_type index = _find_index(key);
    if (index == empty_index_) {
      index = _emplace(key).first.index_;
    }
    return tree_[index].pair_.second;
  }

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

  void reserve(size_type new_cap) { _resizeTree(new_cap); }

  void shrink_to_fit() {
    while (capacity_ > size_) {
      tree_.pop_back();
      --capacity_;
    }
  }

  // Modifiers
  void clear() noexcept {
    root_ = empty_index_;
    size_ = 0;
  }

  std::pair<iterator, bool> insert(const value_type& pair)
    requires(! std::is_same_v<mapped_type, FlatSetEmptyType>)
  {
    return _emplace(pair.first, pair.second);
  }

  std::pair<iterator, bool> insert(value_type&& pair)
    requires(! std::is_same_v<mapped_type, FlatSetEmptyType>)
  {
    return _emplace(pair.first, pair.second);
  }

  std::pair<iterator, bool> insert(const key_type& key)
    requires std::is_same_v<mapped_type, FlatSetEmptyType>
  {
    return _emplace(key);
  }

  std::pair<iterator, bool> insert(key_type&& key)
    requires std::is_same_v<mapped_type, FlatSetEmptyType>
  {
    return _emplace(key);
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    return _emplace(std::forward<Args>(args)...);
  }

  iterator erase(iterator pos) {
    key_type key = tree_[pos.index_].pair_.first;
    _erase(key);
    return upper_bound(key);
  }

  iterator erase(const_iterator pos) {
    key_type key = tree_[pos.index_].pair_.first;
    _erase(key);
    return upper_bound(key);
  }

  iterator erase(iterator first, iterator last) {
    key_type key;
    for (; first != last; ++first) {
      key = tree_[first.index_].pair_.first;
      _erase(key);
    }
    return upper_bound(key);
  }

  iterator erase(const_iterator first, const_iterator last) {
    key_type key;
    for (; first != last; ++first) {
      key = tree_[first.index_].pair_.first;
      _erase(key);
    }
    return upper_bound(key);
  }

  size_type erase(const key_type& key) { return _erase(key); }

  template <typename K>
  size_type erase(K&& x)
    requires std::is_convertible_v<K, key_type>
  {
    return _erase(x);
  }

  void swap(FlatRBTree& other) noexcept(FlatTree_NoThrow<Key> &&
                                        FlatTree_NoThrow<Value>) {
    std::swap(*this, other);
  }

  node_type extract(const_iterator position) { return tree_[position.index_]; }

  node_type extract(const key_type& key) { return tree_[_find_index(key)]; }

  template <typename K>
  node_type extract(K&& x)
    requires std::is_convertible_v<K, key_type>
  {
    return tree_[_find_index(x)];
  }

  void merge(self_type& source) {
    for (size_type i {}; i < source.size_; ++i) {
      _emplace(source.tree_[i].pair_.first, source.tree_[i].pair_.second);
    }
  }

  void merge(self_type&& source) {
    for (size_type i {}; i < source.size_; ++i) {
      _emplace(source.tree_[i].pair_.first, source.tree_[i].pair_.second);
    }
  }

  // Lookup
  [[nodiscard]] size_type count(const key_type& key) const {
    return contains(key);
  }

  template <typename K>
  [[nodiscard]] size_type count(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return contains(x);
  }

  [[nodiscard]] iterator find(const key_type& key) {
    return iterator(this, _find_index(key));
  }

  [[nodiscard]] const_iterator find(const key_type& key) const {
    return const_iterator(this, _find_index(key));
  }

  template <typename K>
  [[nodiscard]] iterator find(const K& x)
    requires std::is_convertible_v<K, key_type>
  {
    return iterator(this, _find_index(x));
  }

  template <typename K>
  [[nodiscard]] const_iterator find(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return const_iterator(this, _find_index(x));
  }

  [[nodiscard]] bool contains(const key_type& key) const {
    return _find_index(key) != empty_index_;
  }

  template <typename K>
  [[nodiscard]] bool contains(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return _find_index(x) != empty_index_;
  }

  [[nodiscard]] std::pair<iterator, iterator> equal_range(const key_type& key) {
    return {lower_bound(key), upper_bound(key)};
  }

  [[nodiscard]] std::pair<const_iterator, const_iterator>
  equal_range(const key_type& key) const {
    return {lower_bound(key), upper_bound(key)};
  }

  template <typename K>
  [[nodiscard]] std::pair<iterator, iterator> equal_range(const K& x)
    requires std::is_convertible_v<K, key_type>
  {
    return {lower_bound(x), upper_bound(x)};
  }
  template <typename K>
  [[nodiscard]] std::pair<const_iterator, const_iterator>
  equal_range(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return {lower_bound(x), upper_bound(x)};
  }

  [[nodiscard]] iterator lower_bound(const key_type& key) {
    return iterator(this, _notLessThan(key));
  }

  [[nodiscard]] const_iterator lower_bound(const key_type& key) const {
    return const_iterator(this, _notLessThan(key));
  }

  template <typename K>
  [[nodiscard]] iterator lower_bound(const K& x)
    requires std::is_convertible_v<K, key_type>
  {
    return iterator(this, _notLessThan(x));
  }
  template <typename K>
  [[nodiscard]] const_iterator lower_bound(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return const_iterator(this, _notLessThan(x));
  }

  [[nodiscard]] iterator upper_bound(const key_type& key) {
    return iterator(this, _greaterThan(key));
  }
  [[nodiscard]] const_iterator upper_bound(const key_type& key) const {
    return const_iterator(this, _greaterThan(key));
  }
  template <typename K>
  [[nodiscard]] iterator upper_bound(const K& x)
    requires std::is_convertible_v<K, key_type>
  {
    return iterator(this, _greaterThan(x));
  }
  template <typename K>
  [[nodiscard]] const_iterator upper_bound(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return const_iterator(this, _greaterThan(x));
  }

  // Observers
  [[nodiscard]] Compare key_comp() const noexcept { return Compare(); }

  [[nodiscard]] Compare value_comp() const noexcept { return Compare(); }

private:
  // For FlatMap
  template <typename K, typename... Args>
  std::pair<iterator, bool> _emplace(const K& key, Args&&... args)
    requires(std::is_convertible_v<K, key_type> &&
             ! std::is_same_v<mapped_type, FlatSetEmptyType> &&
             std::is_constructible_v<mapped_type, Args...>)
  {
    _validateSize();
    size_type insertIndex = size_;
    auto insertResult     = _findInsertLocation(key);
    if (! insertResult.second) {
      return {iterator(this, insertResult.first), false};
    }
    // Create Node in the end of the tree
    size_type parent = insertResult.first;
    _resizeTree();
    auto& newNodeRef        = tree_[size_];
    newNodeRef.pair_.first  = key;
    newNodeRef.pair_.second = mapped_type(std::forward<Args...>(args)...);
    newNodeRef.parent_      = parent;
    newNodeRef.left_        = empty_index_;
    newNodeRef.right_       = empty_index_;
    newNodeRef.color_       = RED_;
    ++size_;
    // Update root_
    if (! insertIndex) {
      root_               = insertIndex;
      tree_[root_].color_ = BLACK_;
      return {iterator(this, insertIndex), true};
    }
    _insertUpdateParentRoot(key, parent, insertIndex);
    _fixInsert(insertIndex);
    return {iterator(this, _find_index(key)), true};
  }

  // Overload For FlatSet
  template <typename... Args>
  std::pair<iterator, bool> _emplace(Args&&... args)
    requires(std::is_same_v<mapped_type, FlatSetEmptyType> &&
             std::is_constructible_v<key_type, Args...>)
  {
    _validateSize();
    size_type insertIndex = size_;
    key_type key          = key_type(std::forward<Args...>(args)...);
    auto insertResult     = _findInsertLocation(key);
    if (! insertResult.second) {
      return {iterator(this, insertResult.first), false};
    }
    // Create Node in the end of the tree
    size_type parent = insertResult.first;
    _resizeTree();
    auto& newNodeRef       = tree_[size_];
    newNodeRef.pair_.first = key_type(std::forward<Args...>(args)...);
    newNodeRef.parent_     = parent;
    newNodeRef.left_       = empty_index_;
    newNodeRef.right_      = empty_index_;
    newNodeRef.color_      = RED_;
    ++size_;
    // Update root_
    if (! insertIndex) {
      root_               = insertIndex;
      tree_[root_].color_ = BLACK_;
      return {iterator(this, insertIndex), true};
    }
    _insertUpdateParentRoot(key, parent, insertIndex);
    _fixInsert(insertIndex);
    return {iterator(this, _find_index(key)), true};
  }

  std::pair<size_type, bool> _findInsertLocation(key_type key) {
    size_type node   = root_;
    size_type parent = empty_index_;
    while (node != empty_index_) {
      parent          = node;
      auto& parentRef = tree_[parent];
      // Key found
      if (key == parentRef.pair_.first) {
        return {parent, false};
      }
      if (key_compare()(key, parentRef.pair_.first)) {
        node = parentRef.left_;
      } else {
        node = parentRef.right_;
      }
    }
    return {parent, true};
  }

  void _insertUpdateParentRoot(key_type key, size_type parent,
                               size_type insertIndex) {
    auto& parentRef = tree_[parent];
    if (key_compare()(key, parentRef.pair_.first)) {
      parentRef.left_ = insertIndex;
    } else {
      parentRef.right_ = insertIndex;
    }
  }

  bool _erase(key_type key) {
    size_type eraseIndex = _find_index(key);
    if (eraseIndex == empty_index_) {
      return false;
    }
    auto& eraseRef   = tree_[eraseIndex];
    bool color       = eraseRef.color_;
    size_type parent = eraseRef.parent_;
    size_type child  = empty_index_;
    // One or both children empty
    if (eraseRef.left_ == empty_index_ || eraseRef.right_ == empty_index_) {
      if (eraseRef.left_ == empty_index_) {
        child = eraseRef.right_;
      } else {
        child = eraseRef.left_;
      }
      _updateParent(child, eraseRef.parent_);
      _updateParentChild(child, parent, eraseIndex);
      _swapOutOfTree(child, eraseIndex, child, parent);
      // Both children full
    } else {
      size_type minNode = _minValueNode(eraseIndex);
      auto& minNodeRef  = tree_[minNode];
      child             = minNodeRef.right_;
      parent            = minNodeRef.parent_;
      color             = minNodeRef.color_;
      _updateParent(child, parent);
      if (parent == eraseIndex) {
        tree_[parent].right_ = child;
        parent               = minNode;
      } else {
        tree_[parent].left_ = child;
      }
      _transferData(minNode, eraseIndex);
      _updateParentChild(minNode, eraseRef.parent_, eraseIndex);
      tree_[eraseRef.left_].parent_ = minNode;
      _updateParent(eraseRef.right_, minNode);
      _swapOutOfTree(minNode, eraseIndex, child, parent);
    }
    if (color == BLACK_) {
      _fixErase(child, parent);
    }
    --size_;
    return true;
  }

  size_type _find_index(key_type key) const {
    size_type node = root_;
    // Find node with binary search
    while (node != empty_index_) {
      auto& nodeRef = tree_[node];
      if (nodeRef.pair_.first == key) {
        return node;
      }
      if (key_compare()(nodeRef.pair_.first, key)) {
        node = nodeRef.right_;
      } else {
        node = nodeRef.left_;
      }
    }
    return empty_index_;
  }

  void _updateParent(size_type node, size_type newParent) {
    if (node != empty_index_) {
      tree_[node].parent_ = newParent;
    }
  }

  size_type _greaterThan(key_type key) const {
    size_type node     = root_;
    size_type lastNode = empty_index_;
    // Find node with binary search
    while (node != empty_index_) {
      auto& nodeRef = tree_[node];
      if (key_compare()(nodeRef.pair_.first, key) ||
          nodeRef.pair_.first == key) {
        node = nodeRef.right_;
      } else {
        node = nodeRef.left_;
        if (node == empty_index_) {
          return lastNode;
        }
        auto& newNodeRef = tree_[node];
        if (lastNode != empty_index_ &&
            (key_compare()(newNodeRef.pair_.first, key) ||
             newNodeRef.pair_.first == key) &&
            ! (key_compare()(tree_[lastNode].pair_.first, key))) {
          return lastNode;
        }
      }
      lastNode = node;
    }
    return empty_index_;
  }

  size_type _notLessThan(key_type key) const {
    size_type node     = root_;
    size_type lastNode = empty_index_;
    // Find node with binary search
    while (node != empty_index_) {
      auto& nodeRef = tree_[node];
      if (nodeRef.pair_.first == key) {
        return node;
      }
      if (key_compare()(nodeRef.pair_.first, key)) {
        node = nodeRef.right_;
      } else {
        node = nodeRef.left_;
        if (node == empty_index_) {
          return lastNode;
        }
        auto& newNodeRef = tree_[node];
        if (lastNode != empty_index_ &&
            key_compare()(newNodeRef.pair_.first, key) &&
            (! key_compare()(tree_[lastNode].pair_.first, key) ||
             tree_[lastNode].pair_.first == key)) {
          return lastNode;
        }
      }
      lastNode = node;
    }
    return empty_index_;
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
                          size_type eraseIndex) {
    if (parent == empty_index_) {
      root_ = child;
      return;
    }
    auto& parentRef = tree_[parent];
    if (parentRef.left_ == eraseIndex) {
      parentRef.left_ = child;
    } else {
      parentRef.right_ = child;
    }
  }

  void _fixInsert(size_type node) {
    while (node != root_ && tree_[tree_[node].parent_].color_ == RED_) {
      size_type parent      = tree_[node].parent_;
      auto& parentRef       = tree_[parent];
      size_type grandparent = parentRef.parent_;
      auto& grandparentRef  = tree_[grandparent];
      size_type uncle = (parent == grandparentRef.left_) ? grandparentRef.right_
                                                         : grandparentRef.left_;
      auto& uncleRef  = tree_[uncle];
      // Update Uncle
      if (uncle != empty_index_ && uncleRef.color_ == RED_) {
        _updateInsertColors(uncleRef, parentRef, grandparentRef);
        node = grandparent;
      } else {
        if (uncle != empty_index_) {
          // Maintains a heap-like structure
          _swapNodePosition(uncle, node);
          node = uncle;
        }
        if (parent == grandparentRef.left_) {
          // Left Tree Insert
          if (node == parentRef.right_) {
            _rotateLeft(parent);
          }
          _rotateRight(grandparent);
        } else {
          // Right Tree Insert
          if (node == parentRef.left_) {
            _rotateRight(parent);
          }
          _rotateLeft(grandparent);
        }
        std::swap(parentRef.color_, grandparentRef.color_);
        node = parent;
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

  // THIS FUNCTION IS TERRIBLE
  // THE RB TREE LOGIC IS COMPLEX
  // NEEDS TO BE REFACTORED
  void _fixErase(size_type node, size_type parent) {
    size_type sibling = empty_index_;
    while (node != root_ &&
           (node == empty_index_ || tree_[node].color_ == BLACK_)) {
      auto& nodeRef   = tree_[node];
      auto& parentRef = tree_[parent];
      // Left Tree Erase
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
        // Right Tree Erase
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
    if (childRight != empty_index_) {
      tree_[childRight].parent_ = node;
    }
    // Update Parent
    size_type nodeLeft = nodeRef.left_;
    if (nodeLeft != empty_index_) {
      tree_[nodeLeft].parent_ = child;
    }
    // Touches less memory, more code, but less computation
    std::swap(nodeRef.pair_, childRef.pair_);
    std::swap(nodeRef.color_, childRef.color_);
    std::swap(nodeRef.left_, childRef.right_);
    std::swap(nodeRef.left_, nodeRef.right_);
    std::swap(childRef.left_, childRef.right_);
    return child;
  }

  size_type _rotateRight(size_type node) {
    auto& nodeRef   = tree_[node];
    size_type child = tree_[node].left_;
    auto& childRef  = tree_[child];
    // Update Children
    size_type childLeft = childRef.left_;
    if (childLeft != empty_index_) {
      tree_[childLeft].parent_ = node;
    }
    // Update Parent
    size_type nodeRight = nodeRef.right_;
    if (nodeRight != empty_index_) {
      tree_[nodeRight].parent_ = child;
    }
    // Touches less memory, more code, but less computation
    std::swap(nodeRef.pair_, childRef.pair_);
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
    // Update child and parent index for erase method
    if (nodeA == child) {
      child = nodeB;
    }
    if (nodeA == parent) {
      parent = nodeB;
    }
    // Saves computation time (~2-3 nanoseconds)
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

  size_type _minValueNode(size_type node) const {
    node           = tree_[node].right_;
    size_type left = empty_index_;
    while ((left = tree_[node].left_) != empty_index_) { node = left; }
    return node;
  }

  size_type _first() const {
    size_type node = root_;
    if (node != empty_index_) {
      size_type left = empty_index_;
      while ((left = tree_[node].left_) != empty_index_) { node = left; }
    }
    return node;
  }

  size_type _last() const {
    size_type node = root_;
    if (node != empty_index_) {
      size_type right = empty_index_;
      while ((right = tree_[node].right_) != empty_index_) { node = right; }
    }
    return node;
  }

  size_type _next(size_type node) const {
    if (node == size_ - 1 || node == empty_index_) {
      return empty_index_;
    }
    if (tree_[node].right_ != empty_index_) {
      node           = tree_[node].right_;
      size_type left = empty_index_;
      while ((left = tree_[node].left_) != empty_index_) { node = left; }
      return node;
    }
    // If no right child then backtrack
    size_type parent = empty_index_;
    while ((parent = tree_[node].parent_) && parent != empty_index_ &&
           node == tree_[parent].right_) {
      node = parent;
    }
    return parent;
  }

  size_type _prev(size_type node) const {
    if (node == size_ - 1 || node == empty_index_) {
      return empty_index_;
    }
    if (tree_[node].left_ != empty_index_) {
      node            = tree_[node].left_;
      size_type right = empty_index_;
      while ((right = tree_[node].right_) != empty_index_) { node = right; }
      return node;
    }
    // If no left child then backtrack
    size_type parent = empty_index_;
    while ((parent = tree_[node].parent_) && parent != empty_index_ &&
           node == tree_[parent].left_) {
      node = parent;
    }
    return parent;
  }

  void _validateSize() {
    if (size_ == empty_index_) {
      throw std::runtime_error("Size exceeds max capacity of size type. "
                               "Increase size type of tree.");
    }
  }

  void _resizeTree(size_type new_cap = 0) {
    if (new_cap > capacity_) {
      tree_.resize(new_cap);
      return;
    }
    if (size_ == capacity_) {
      capacity_ = std::min(empty_index_, capacity_ * 2);
      tree_.resize(capacity_);
    }
  }
};

}// namespace details

// Documentation:
// FlatMap<Key, Value, MaxSize, Compare, Allocator>
// Key: Must be copyable or moveable type
// Value: Must be copyable or moveable type
// MaxSize: Integral type used for tree size optimizations.
//          If you know the max size is less than default std::size_t, then
//          specify and the node will use the new type and save space
// Compare: Function used to compare keys, default std::less
// Allocator: Allocator passed to the vector, takes a dro::details::Node

template <details::FlatTree_Type Key, details::FlatTree_Type Value,
          details::Integral MaxSize = std::size_t,
          typename Compare          = std::less<Key>,
          typename Allocator =
              std::allocator<details::Node<std::pair<Key, Value>, MaxSize>>>
class FlatMap : public details::FlatRBTree<Key, Value, std::pair<Key, Value>,
                                           MaxSize, Compare, Allocator> {
  using size_type = MaxSize;
  using tree_type = details::FlatRBTree<Key, Value, std::pair<Key, Value>,
                                        MaxSize, Compare, Allocator>;

public:
  explicit FlatMap(size_type capacity = 1, Allocator allocator = Allocator())
      : tree_type(capacity, allocator) {}
};

// Documentation:
// FlatSet<Key, MaxSize, Compare, Allocator>
// Key: Must be copyable or moveable type
// MaxSize: Integral type used for tree size optimizations.
//          If you know the max size is less than default std::size_t, then
//          specify and the node will use the new type and save space
// Compare: Function used to compare keys, default std::less
// Allocator: Allocator passed to the vector, takes a dro::details::Node

template <details::FlatTree_Type Key, details::Integral MaxSize = std::size_t,
          typename Compare = std::less<Key>,
          typename Allocator =
              std::allocator<details::Node<details::FlatSetPair<Key>, MaxSize>>>
class FlatSet : public details::FlatRBTree<Key, details::FlatSetEmptyType,
                                           details::FlatSetPair<Key>, MaxSize,
                                           Compare, Allocator> {
  using size_type = MaxSize;
  using tree_type = details::FlatRBTree<Key, details::FlatSetEmptyType,
                                        details::FlatSetPair<Key>, MaxSize,
                                        Compare, Allocator>;

public:
  explicit FlatSet(size_type capacity = 1, Allocator allocator = Allocator())
      : tree_type(capacity, allocator) {}
};
}// namespace dro
#endif
