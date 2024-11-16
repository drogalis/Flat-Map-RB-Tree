// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.ndrew Drogalis

#ifndef DRO_FLAT_RED_BLACK_TREE
#define DRO_FLAT_RED_BLACK_TREE

#include <concepts>   // for requires
#include <cstddef>    // for size_t, ptrdiff_t
#include <functional> // for less
#include <iterator>   // for pair, bidirectional_iterator_tag
#include <limits>     // for numeric_limits
#include <stdexcept>  // for out_of_range, runtime_error
#include <type_traits>// for std::is_default_constructible
#include <utility>    // for pair, forward
#include <vector>     // for vector, allocator

#include <iostream>
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

template <typename Pair, Integral MaxSize = std::size_t> struct Node {
  using size_type = MaxSize;
  // This empty_index_ definition goes against DRY. Not sure most elegant
  // solution that also enforces correctness
  constexpr static size_type empty_index_ =
      std::numeric_limits<size_type>::max();

  Pair pair_;
  size_type parent_ {empty_index_};
  size_type left_ {empty_index_};
  size_type right_ {empty_index_};
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
          Integral MaxSize = std::size_t, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<Node<Pair, MaxSize>>>
class FlatRBTree {

public:
  using key_type        = Key;
  using mapped_type     = Value;
  using value_type      = Pair;
  using size_type       = MaxSize;
  using difference_type = std::ptrdiff_t;
  using key_compare     = Compare;
  using allocator_type  = Allocator;
  using node_type       = Node<value_type, size_type>;
  using self_type = FlatRBTree<key_type, mapped_type, value_type, size_type,
                               key_compare, allocator_type>;
  using iterator  = FlatTreeIterator<self_type>;
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

  size_type root_            = empty_index_;
  size_type firstIndexCache_ = empty_index_;
  size_type lastIndexCache_  = empty_index_;
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
    size_type index = _findIndex(key);
    if (index == empty_index_) {
      throw std::out_of_range("Key not found");
    }
    return tree_[index].pair_.second;
  }

  const mapped_type& at(const key_type& key) const
    requires(! std::is_same_v<mapped_type, FlatSetEmptyType>)
  {
    size_type index = _findIndex(key);
    if (index == empty_index_) {
      throw std::out_of_range("Key not found");
    }
    return tree_[index].pair_.second;
  }

  mapped_type& operator[](const key_type& key)
    requires(! std::is_same_v<mapped_type, FlatSetEmptyType>)
  {
    size_type index = _findIndex(key);
    if (index == empty_index_) {
      index = _emplace(key).first.index_;
    }
    return tree_[index].pair_.second;
  }

  mapped_type& operator[](key_type&& key)
    requires(! std::is_same_v<mapped_type, FlatSetEmptyType>)
  {
    size_type index = _findIndex(key);
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

  iterator rend() { return iterator(this, empty_index_, true); }

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
    firstIndexCache_ = empty_index_;
    lastIndexCache_  = empty_index_;
    root_            = empty_index_;
    size_            = 0;
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
  std::pair<iterator, bool> emplace(Args&&... args)
    requires(! std::is_same_v<mapped_type, FlatSetEmptyType>)
  {
    return _emplace(std::forward<Args>(args)...);
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args)
    requires std::is_same_v<mapped_type, FlatSetEmptyType>
  {
    return _emplaceSet(std::forward<Args>(args)...);
  }

  iterator erase(iterator pos) {
    if (pos == end()) {
      return end();
    }
    size_type index = pos.index_;
    key_type key    = tree_[index].pair_.first;
    return iterator(this, _erase(key, index).second);
  }

  iterator erase(const_iterator pos) {
    if (pos == cend()) {
      return end();
    }
    size_type index = pos.index_;
    key_type key    = tree_[index].pair_.first;
    return iterator(this, _erase(key, index).second);
  }

  iterator erase(iterator first, iterator last) {
    key_type key;
    size_type upperIndex = empty_index_;
    for (; first != last && first != end(); ++first) {
      size_type index = first.index_;
      key             = tree_[index].pair_.first;
      upperIndex      = _erase(key, index).second;
    }
    return iterator(this, upperIndex);
  }

  iterator erase(const_iterator first, const_iterator last) {
    key_type key;
    size_type upperIndex = empty_index_;
    for (; first != last && first != cend(); ++first) {
      size_type index = first.index_;
      key             = tree_[index].pair_.first;
      upperIndex      = _erase(key, index).second;
    }
    return iterator(this, upperIndex);
  }

  size_type erase(const key_type& key) { return _erase(key).first; }

  template <typename K>
  size_type erase(K&& x)
    requires std::is_convertible_v<K, key_type>
  {
    return _erase(x).first;
  }

  void swap(FlatRBTree& other) noexcept(FlatTree_NoThrow<Key> &&
                                        FlatTree_NoThrow<Value>) {
    std::swap(*this, other);
  }

  node_type extract(const_iterator position) { return tree_[position.index_]; }

  node_type extract(const key_type& key) { return tree_[_findIndex(key)]; }

  template <typename K>
  node_type extract(K&& x)
    requires std::is_convertible_v<K, key_type>
  {
    return tree_[_findIndex(x)];
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
    return iterator(this, _findIndex(key));
  }

  [[nodiscard]] const_iterator find(const key_type& key) const {
    return const_iterator(this, _findIndex(key));
  }

  template <typename K>
  [[nodiscard]] iterator find(const K& x)
    requires std::is_convertible_v<K, key_type>
  {
    return iterator(this, _findIndex(x));
  }

  template <typename K>
  [[nodiscard]] const_iterator find(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return const_iterator(this, _findIndex(x));
  }

  [[nodiscard]] bool contains(const key_type& key) const {
    return _findIndex(key) != empty_index_;
  }

  template <typename K>
  [[nodiscard]] bool contains(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return _findIndex(x) != empty_index_;
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
    return iterator(this, _lowerBound(key));
  }

  [[nodiscard]] const_iterator lower_bound(const key_type& key) const {
    return const_iterator(this, _lowerBound(key));
  }

  template <typename K>
  [[nodiscard]] iterator lower_bound(const K& x)
    requires std::is_convertible_v<K, key_type>
  {
    return iterator(this, _lowerBound(x));
  }
  template <typename K>
  [[nodiscard]] const_iterator lower_bound(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return const_iterator(this, _lowerBound(x));
  }

  [[nodiscard]] iterator upper_bound(const key_type& key) {
    return iterator(this, _upperBound(key));
  }
  [[nodiscard]] const_iterator upper_bound(const key_type& key) const {
    return const_iterator(this, _upperBound(key));
  }
  template <typename K>
  [[nodiscard]] iterator upper_bound(const K& x)
    requires std::is_convertible_v<K, key_type>
  {
    return iterator(this, _upperBound(x));
  }
  template <typename K>
  [[nodiscard]] const_iterator upper_bound(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return const_iterator(this, _upperBound(x));
  }

  // Observers
  [[nodiscard]] Compare key_comp() const noexcept { return Compare(); }

  [[nodiscard]] Compare value_comp() const noexcept { return Compare(); }

private:
  // For FlatMap
  template <typename K, typename... Args>
  std::pair<iterator, bool> _emplace(const K& key, Args&&... args)
    requires(std::is_convertible_v<K, key_type> &&
             std::is_constructible_v<mapped_type, Args && ...>)
  {
    _validateSize();
    size_type insertIndex = size_;
    size_type extremaCase {};
    auto isExtrema = _checkCachedExtrema(key, extremaCase);
    auto insertResult =
        (! isExtrema.second) ? _findInsertLocation(key) : isExtrema;
    if (! insertResult.second) {
      return {iterator(this, insertResult.first), false};
    }
    // Create Node in the end of the tree
    size_type parent = insertResult.first;
    _resizeTree();
    auto& newNodeRef        = tree_[size_];
    newNodeRef.pair_.first  = key;
    newNodeRef.pair_.second = mapped_type(std::forward<Args>(args)...);
    newNodeRef.parent_      = parent;
    newNodeRef.left_        = empty_index_;
    newNodeRef.right_       = empty_index_;
    newNodeRef.color_       = RED_;
    ++size_;
    // Update root_
    if (! insertIndex) {
      root_               = insertIndex;
      tree_[root_].color_ = BLACK_;
      _insertUpdateCachedExtrema(extremaCase, insertIndex);
      return {iterator(this, insertIndex), true};
    }
    _insertUpdateParentRoot(key, parent, insertIndex);
    insertIndex = _fixInsert(insertIndex, key);
    _insertUpdateCachedExtrema(extremaCase, insertIndex);
    return {iterator(this, insertIndex), true};
  }

  // Overload For FlatSet
  template <typename... Args>
  std::pair<iterator, bool> _emplaceSet(Args&&... args)
    requires(std::is_same_v<mapped_type, FlatSetEmptyType> &&
             std::is_constructible_v<key_type, Args && ...>)
  {
    key_type key = key_type(std::forward<Args>(args)...);
    return _emplace(key);
  }

  std::pair<size_type, bool> _checkCachedExtrema(const key_type& key,
                                                 size_type& extremaCase) {
    if (firstIndexCache_ != empty_index_ &&
        key_compare()(key, tree_[firstIndexCache_].pair_.first)) {
      extremaCase = 2;
      return {firstIndexCache_, true};
    }
    if (lastIndexCache_ != empty_index_ &&
        key_compare()(tree_[lastIndexCache_].pair_.first, key)) {
      extremaCase = 1;
      return {lastIndexCache_, true};
    }
    return {empty_index_, false};
  }

  void _insertUpdateCachedExtrema(size_type extremaCase,
                                  size_type insertIndex) {
    if (firstIndexCache_ == empty_index_ || extremaCase == 2) {
      firstIndexCache_ = insertIndex;
    }
    if (lastIndexCache_ == empty_index_ || extremaCase == 1) {
      lastIndexCache_ = insertIndex;
    }
  }

  std::pair<size_type, bool> _findInsertLocation(const key_type& key) {
    size_type node   = root_;
    size_type parent = empty_index_;
    while (node != empty_index_) {
      parent          = node;
      auto& parentRef = tree_[parent];
      _prefetchBinarySearch(parentRef);
      // Key found
      if (key == parentRef.pair_.first) {
        return {parent, false};
      }
      bool compare = key_compare()(key, parentRef.pair_.first);
      node         = compare ? parentRef.left_ : parentRef.right_;
    }
    return {parent, true};
  }

  void _insertUpdateParentRoot(const key_type& key, size_type parent,
                               size_type insertIndex) {
    auto& parentRef = tree_[parent];
    if (key_compare()(key, parentRef.pair_.first)) {
      parentRef.left_ = insertIndex;
    } else {
      parentRef.right_ = insertIndex;
    }
  }

  std::pair<bool, size_type> _erase(const key_type& key,
                                    size_type index = empty_index_) {
    size_type eraseIndex = (index == empty_index_) ? _findIndex(key) : index;
    if (eraseIndex == empty_index_) {
      return {false, empty_index_};
    }
    std::cout << "old erase idx: " << eraseIndex << '\n';
    // For return iterator
    size_type upperIndex    = _next(eraseIndex);
    size_type lowerIndex    = _prev(eraseIndex);
    const bool largestElem  = (upperIndex == empty_index_);
    const bool smallestElem = (lowerIndex == empty_index_);
    upperIndex              = largestElem ? size_ - 1 : upperIndex;
    lowerIndex              = smallestElem ? size_ - 1 : lowerIndex;
    // Erase Node
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
      _swapOutOfTree(child, eraseIndex, child, parent, upperIndex, lowerIndex);
      // Both children full
    } else {
      size_type minNode = _minValueNode(eraseIndex);
      std::cout << "_min-index-v: " << tree_[minNode].pair_.first << '\n';
      auto& minNodeRef = tree_[minNode];
      child            = minNodeRef.right_;
      parent           = minNodeRef.parent_;
      color            = minNodeRef.color_;
      _updateParent(child, parent);
      if (parent == eraseIndex) {
        tree_[parent].right_ = child;
        parent               = minNode;
      } else {
        tree_[parent].left_ = child;
      }
      _transferData(minNode, eraseIndex);
      std::cout << "_erase index 2 value " << tree_[eraseIndex].pair_.first
              << '\n';
      _updateParentChild(minNode, eraseRef.parent_, eraseIndex);
      tree_[eraseRef.left_].parent_ = minNode;
      _updateParent(eraseRef.right_, minNode);
      std::cout << "_erase index 3 value " << tree_[eraseIndex].pair_.first
              << '\n';
      _swapOutOfTree(minNode, eraseIndex, child, parent, upperIndex,
                     lowerIndex);
    }
    std::cout << "_erase index l value " << tree_[eraseIndex].pair_.first
              << '\n';
    if (color == BLACK_) {
      _fixErase(child, parent, upperIndex, lowerIndex);
    } else {
      std::cout << "NO FIX\n";
    }
    upperIndex       = largestElem ? empty_index_ : upperIndex;
    lowerIndex       = smallestElem ? empty_index_ : lowerIndex;
    firstIndexCache_ = smallestElem ? upperIndex : firstIndexCache_;
    lastIndexCache_  = largestElem ? lowerIndex : lastIndexCache_;
    --size_;
    return {true, upperIndex};
  }

  size_type _findIndex(const key_type& key) const {
    size_type node = root_;
    // Find node with binary search
    while (node != empty_index_) {
      auto& nodeRef = tree_[node];
      _prefetchBinarySearch(nodeRef);
      if (nodeRef.pair_.first == key) {
        return node;
      }
      bool compare = key_compare()(nodeRef.pair_.first, key);
      node         = compare ? nodeRef.right_ : nodeRef.left_;
    }
    return empty_index_;
  }

  void _prefetchBinarySearch(auto& nodeRef) const {
    size_type nextLeft =
        (nodeRef.left_ == empty_index_) ? size_ - 1 : nodeRef.left_;
    size_type nextRight =
        (nodeRef.right_ == empty_index_) ? size_ - 1 : nodeRef.right_;
    __builtin_prefetch(&tree_[nextLeft]);
    __builtin_prefetch(&tree_[nextRight]);
  }

  void _updateParent(size_type node, size_type newParent) {
    if (node != empty_index_) {
      tree_[node].parent_ = newParent;
    }
  }

  size_type _upperBound(const key_type& key) const {
    size_type node     = root_;
    size_type lastNode = empty_index_;
    // Find node with binary search
    while (node != empty_index_) {
      auto& nodeRef = tree_[node];
      _prefetchBinarySearch(nodeRef);
      bool compare = (key_compare()(nodeRef.pair_.first, key) ||
                      nodeRef.pair_.first == key);
      lastNode     = compare ? lastNode : node;
      node         = compare ? nodeRef.right_ : nodeRef.left_;
    }
    return lastNode;
  }

  size_type _lowerBound(const key_type& key) const {
    size_type node     = root_;
    size_type lastNode = empty_index_;
    // Find node with binary search
    while (node != empty_index_) {
      auto& nodeRef = tree_[node];
      _prefetchBinarySearch(nodeRef);
      if (nodeRef.pair_.first == key) {
        return node;
      }
      bool compare = key_compare()(nodeRef.pair_.first, key);
      lastNode     = compare ? lastNode : node;
      node         = compare ? nodeRef.right_ : nodeRef.left_;
    }
    return lastNode;
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

  size_type _fixInsert(size_type node, const key_type& key) {
    size_type baseNode = node;
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
        if (tree_[grandparent].pair_.first == key) {
          baseNode = grandparent;
        }
        node = parent;
      }
    }
    tree_[root_].color_ = BLACK_;
    return baseNode;
  }

  void _updateInsertColors(auto& uncleRef, auto& parentRef,
                           auto& grandparentRef) {
    grandparentRef.color_ = RED_;
    parentRef.color_      = BLACK_;
    uncleRef.color_       = BLACK_;
  }

  void _fixErase(size_type node, size_type parent, size_type& upperIndex,
                 size_type& lowerIndex) {
    // Cannot be a reference
    key_type upperKey = tree_[upperIndex].pair_.first;
    key_type lowerKey = tree_[lowerIndex].pair_.first;
    size_type sibling = empty_index_;
    while (node != root_ &&
           (node == empty_index_ || tree_[node].color_ == BLACK_)) {
      auto& nodeRef   = tree_[node];
      auto& parentRef = tree_[parent];
      bool isLeftTree = (node == parentRef.left_);
      // Analyze Sibling
      sibling = (isLeftTree) ? parentRef.right_ : parentRef.left_;
      std::cout << "_value: " << tree_[parent].pair_.first << ' '
                << tree_[sibling].pair_.first << '\n';
      std::cout << parent << ' ' << sibling << "--- sib here\n";
      _checkSiblingRed(sibling, parent, isLeftTree);
      std::cout << parent << ' ' << sibling << "--- sib here\n";
      if (parent != empty_index_ && tree_[parent].pair_.first == upperKey) {
        upperIndex = parent;
      }
      if (parent != empty_index_ && tree_[parent].pair_.first == lowerKey) {
        lowerIndex = parent;
      }
      auto& siblingRef = tree_[sibling];
      if (_checkSiblingChildColor(siblingRef, node, parent)) {
        // Fix Side of Tree
      } else {
        if (isLeftTree) {
          std::cout << "Left\n";
          _fixEraseLeftTree(siblingRef, sibling, parent);
          _rotateLeft(parent);
        } else {
          _fixEraseRightTree(siblingRef, sibling, parent);
          _rotateRight(parent);
        }
        if (sibling != empty_index_ && tree_[sibling].pair_.first == upperKey) {
          upperIndex = sibling;
        }
        if (sibling != empty_index_ && tree_[sibling].pair_.first == lowerKey) {
          lowerIndex = sibling;
        }
        node = root_;
        break;
      }
    }
    std::cout << "-- end\n";
    if (node != empty_index_) {
      tree_[node].color_ = BLACK_;
    }
  }

  void _checkSiblingRed(size_type& sibling, size_type& parent,
                        bool isLeftTree) {
    if (tree_[sibling].color_ == RED_) {
      tree_[sibling].color_ = BLACK_;
      tree_[parent].color_  = RED_;
      if (isLeftTree) {
        std::cout << "_ltest: " << tree_[parent].parent_ << '\n';
        std::cout << "_ltest: " << tree_[parent].left_ << '\n';
        std::cout << "_ltest: " << tree_[parent].right_ << '\n';
        parent = _rotateLeft(parent);
        std::cout << "_ltest: " << tree_[parent].parent_ << '\n';
        std::cout << "_ltest: " << tree_[tree_[parent].parent_].left_ << '\n';
        std::cout << "_ltest: " << tree_[tree_[parent].parent_].right_ << '\n';
        sibling = tree_[parent].right_;
        std::cout << "value: " << tree_[parent].pair_.first << ' '
                  << tree_[sibling].pair_.first << '\n';
        std::cout << "color: " << tree_[parent].color_ << ' '
                  << tree_[sibling].color_ << '\n';
      } else {
        parent  = _rotateRight(parent);
        sibling = tree_[parent].left_;
      }
    }
  }

  bool _checkSiblingChildColor(auto& siblingRef, size_type& node,
                               size_type& parent) {
    if ((siblingRef.left_ == empty_index_ ||
         tree_[siblingRef.left_].color_ == BLACK_) &&
        (siblingRef.right_ == empty_index_ ||
         tree_[siblingRef.right_].color_ == BLACK_)) {
      siblingRef.color_ = RED_;
      node              = parent;
      parent            = tree_[node].parent_;
      return true;
    }
    return false;
  }

  void _fixEraseLeftTree(auto& siblingRef, size_type& sibling,
                         size_type& parent) {
    if (siblingRef.right_ == empty_index_ ||
        tree_[siblingRef.right_].color_ == BLACK_) {
      if (siblingRef.left_ != empty_index_) {
        tree_[siblingRef.left_].color_ = BLACK_;
      }
      siblingRef.color_ = RED_;
      sibling           = _rotateRight(sibling);
      sibling           = tree_[parent].right_;
      std::cout << "_value-sib: " << tree_[sibling].pair_.first << '\n';
    }
    tree_[sibling].color_ = tree_[parent].color_;
    tree_[parent].color_  = BLACK_;
    if (tree_[sibling].right_ != empty_index_) {
      tree_[tree_[sibling].right_].color_ = BLACK_;
    }
  }

  void _fixEraseRightTree(auto& siblingRef, size_type& sibling,
                          size_type& parent) {
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
  }

  size_type _rotateLeft(size_type node) {
    auto& nodeRef   = tree_[node];
    size_type child = nodeRef.right_;
    auto& childRef  = tree_[child];
    // Update Cached Extrema
    _updateExtrema(node, child);
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
    // Update Cached Extrema
    _updateExtrema(node, child);
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

  void _updateExtrema(size_type nodeA, size_type nodeB) {
    if (nodeA == firstIndexCache_) {
      firstIndexCache_ = nodeB;
    } else if (nodeB == firstIndexCache_) {
      firstIndexCache_ = nodeA;
    }
    if (nodeA == lastIndexCache_) {
      lastIndexCache_ = nodeB;
    } else if (nodeB == lastIndexCache_) {
      lastIndexCache_ = nodeA;
    }
  }

  void _swapNodePosition(size_type nodeA, size_type nodeB) {
    if (nodeA == nodeB) {
      return;
    }
    _updateExtrema(nodeA, nodeB);
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
                      size_type& parent, size_type& upperIndex,
                      size_type& lowerIndex) {
    if (node != empty_index_) {
      _swapNodePositionRemove(node, removeNode, child, parent, upperIndex,
                              lowerIndex);
      removeNode = node;
    }
    _swapNodePositionRemove(size_ - 1, removeNode, child, parent, upperIndex,
                            lowerIndex);
  }

  void _swapNodePositionRemove(size_type nodeA, size_type nodeB,
                               size_type& child, size_type& parent,
                               size_type& upperIndex, size_type& lowerIndex) {
    if (nodeA == nodeB) {
      return;
    }
    _updateExtrema(nodeA, nodeB);
    // Update upperIndex for return iterator
    if (tree_[nodeA].pair_.first == tree_[upperIndex].pair_.first) {
      upperIndex = nodeB;
    }
    if (tree_[nodeA].pair_.first == tree_[lowerIndex].pair_.first) {
      lowerIndex = nodeB;
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

  size_type _first() const { return firstIndexCache_; }

  size_type _last() const { return lastIndexCache_; }

  size_type _next(size_type node) const {
    if (node == empty_index_) {
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
    if (parent == root_ && node == tree_[parent].right_) {
      return empty_index_;
    }
    return parent;
  }

  size_type _prev(size_type node) const {
    if (node == empty_index_) {
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
    if (parent == root_ && node == tree_[parent].left_) {
      return empty_index_;
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
      capacity_ = (empty_index_ / 2 < capacity_) ? empty_index_ : capacity_ * 2;
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
