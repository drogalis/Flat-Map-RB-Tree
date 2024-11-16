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

#ifndef DRO_HASH_FLAT_MAP_HPP
#define DRO_HASH_FLAT_MAP_HPP

#include <concepts>   // for requires
#include <cstddef>    // for size_t, ptrdiff_t
#include <cstdint>    // for uint64_t
#include <functional> // for less
#include <iterator>   // for pair, bidirectional_iterator_tag
#include <limits>     // for numeric_limits
#include <stdexcept>  // for out_of_range, runtime_error
#include <type_traits>// for std::is_default_constructible
#include <utility>    // for pair, forward
#include <vector>     // for vector, allocator

namespace dro {
namespace detail {

template <typename T>
concept hashflat_t =
    std::is_default_constructible_v<T> &&
    (std::is_move_assignable_v<T> || std::is_copy_assignable_v<T>);

template <typename T, typename... Args>
concept hashflat_nothrow =
    std::is_nothrow_constructible_v<T, Args&&...> &&
    ((std::is_nothrow_copy_assignable_v<T> && std::is_copy_assignable_v<T>) ||
     (std::is_nothrow_move_assignable_v<T> && std::is_move_assignable_v<T>));

template <typename T>
concept integral_t = std::is_integral_v<T>;

static constexpr bool RED_   = false;
static constexpr bool BLACK_ = true;

struct IsAFlatSet {};

template <hashflat_t Key> struct flatset_pair {
  Key first;
  IsAFlatSet second [[no_unique_address]];
};

template <typename Pair, integral_t SizeT = std::size_t> struct flatnode {
  Pair pair_;
  // Least significant bit: 1 - Full, 0 - Empty
  // 2nd Least significant bit: 1 - Black, 0 - Red
  uint64_t fingerprint_full_clr_ {};
  SizeT next_ {};  // Index of next element in collision chain
  SizeT parent_ {};// Parent in red black tree
  SizeT left_ {};  // Left child in red black tree
  SizeT right_ {}; // Right child in red black tree
};

template <typename Container, bool Reverse = false> struct flat_iterator {
  using key_type        = typename Container::key_type;
  using mapped_type     = typename Container::mapped_type;
  using value_type      = typename Container::value_type;
  using size_type       = typename Container::size_type;
  using key_equal       = typename Container::key_equal;
  using key_compare     = typename Container::key_compare;
  using difference_type = typename Container::difference_type;
  using reference = std::conditional_t<std::is_same_v<mapped_type, IsAFlatSet>,
                                       key_type&, value_type&>;
  using pointer   = std::conditional_t<std::is_same_v<mapped_type, IsAFlatSet>,
                                       key_type*, value_type*>;
  using const_reference =
      std::conditional_t<std::is_same_v<mapped_type, IsAFlatSet>,
                         const key_type&, const value_type&>;
  using const_pointer =
      std::conditional_t<std::is_same_v<mapped_type, IsAFlatSet>,
                         const key_type*, const value_type*>;
  using iterator_category = std::bidirectional_iterator_tag;

  explicit flat_iterator(Container* flatbase, size_type index)
      : flatbase_(flatbase), index_(index) {}

  bool operator==(const flat_iterator& other) const {
    return other.flatbase_ == flatbase_ && other.index_ == index_;
  }

  bool operator!=(const flat_iterator& other) const {
    return ! (*this == other);
  }

  flat_iterator& operator++()
    requires(! Reverse)
  {
    index_ = flatbase_->_next(index_);
    return *this;
  }

  flat_iterator& operator++()
    requires(Reverse)
  {
    index_ = flatbase_->_prev(index_);
    return *this;
  }

  flat_iterator& operator--()
    requires(! Reverse)
  {
    index_ = flatbase_->_prev(index_);
    return *this;
  }

  flat_iterator& operator--()
    requires(Reverse)
  {
    index_ = flatbase_->_next(index_);
    return *this;
  }

  reference operator*()
    requires(std::is_same_v<mapped_type, IsAFlatSet> &&
             ! std::is_const_v<Container>)
  {
    return flatbase_->tree_[index_].pair_.first;
  }

  const_reference operator*() const
    requires(std::is_same_v<mapped_type, IsAFlatSet> &&
             std::is_const_v<Container>)
  {
    return flatbase_->tree_[index_].pair_.first;
  }

  pointer operator->()
    requires(std::is_same_v<mapped_type, IsAFlatSet> &&
             ! std::is_const_v<Container>)
  {
    return &flatbase_->tree_[index_].pair_.first;
  }

  const_pointer operator->() const
    requires(std::is_same_v<mapped_type, IsAFlatSet> &&
             std::is_const_v<Container>)
  {
    return &flatbase_->tree_[index_].pair_.first;
  }

  reference operator*()
    requires(! std::is_same_v<mapped_type, IsAFlatSet> &&
             ! std::is_const_v<Container>)
  {
    return flatbase_->tree_[index_].pair_;
  }

  const_reference operator*() const
    requires(! std::is_same_v<mapped_type, IsAFlatSet> &&
             std::is_const_v<Container>)
  {
    return flatbase_->tree_[index_].pair_;
  }

  pointer operator->()
    requires(! std::is_same_v<mapped_type, IsAFlatSet> &&
             ! std::is_const_v<Container>)
  {
    return &flatbase_->tree_[index_].pair_;
  }

  const_pointer operator->() const
    requires(! std::is_same_v<mapped_type, IsAFlatSet> &&
             std::is_const_v<Container>)
  {
    return &flatbase_->tree_[index_].pair_;
  }

private:
  Container* flatbase_ = nullptr;
  size_type index_ {};
  friend Container;
};

template <hashflat_t Key, hashflat_t Value, typename Pair,
          integral_t SizeT = std::size_t, typename Compare = std::less<Key>,
          typename Hash      = std::hash<Key>,
          typename KeyEqual  = std::equal_to<Key>,
          typename Allocator = std::allocator<flatnode<Pair, SizeT>>>
class hash_flat_base {

public:
  using key_type        = Key;
  using mapped_type     = Value;
  using value_type      = Pair;
  using size_type       = SizeT;
  using difference_type = std::ptrdiff_t;
  using key_compare     = Compare;
  using key_equal       = KeyEqual;
  using hasher          = Hash;
  using allocator_type  = Allocator;
  using reference = std::conditional_t<std::is_same_v<mapped_type, IsAFlatSet>,
                                       key_type&, value_type&>;
  using pointer   = std::conditional_t<std::is_same_v<mapped_type, IsAFlatSet>,
                                       key_type*, value_type*>;
  using node_type = flatnode<value_type, size_type>;
  using tree_type = std::vector<node_type, Allocator>;
  using self_type =
      hash_flat_base<key_type, mapped_type, value_type, size_type, key_compare,
                     hasher, key_equal, allocator_type>;
  using iterator               = flat_iterator<self_type>;
  using const_iterator         = flat_iterator<const self_type>;
  using reverse_iterator       = flat_iterator<self_type, true>;
  using const_reverse_iterator = flat_iterator<const self_type, true>;

private:
  static constexpr bool hashflat_nothrow_v = hashflat_nothrow<key_type> &&
                                             hashflat_nothrow<mapped_type> &&
                                             hashflat_nothrow<value_type>;
  float load_factor_     = 1.0F;
  float hashable_ratio_  = 0.9F;
  float growth_multiple_ = 2.0F;
  size_type size_ {};
  size_type collision_head_ {};
  size_type collision_tail_ {};
  hasher hash_ {};
  key_compare compare_ {};
  key_equal equal_ {};
  size_type capacity_ {};
  size_type hashable_capacity_ {};

  friend iterator;
  friend const_iterator;
  friend reverse_iterator;
  friend const_reverse_iterator;

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

  size_type root_              = empty_index_;
  size_type first_index_cache_ = empty_index_;
  size_type last_index_cache_  = empty_index_;
  tree_type tree_;

public:
  explicit hash_flat_base(size_type capacity,
                          const Allocator& allocator = Allocator())
      : capacity_(capacity), tree_(allocator) {
    if (capacity < 1) {
      throw std::invalid_argument("Capacity must be positive number.");
    }
    if (capacity == empty_index_) {}
    // Additional slot for last collision_head_
    tree_.resize(capacity_ + 1);
    hashable_capacity_ = static_cast<float>(capacity_) * hashable_ratio_;
    collision_head_    = hashable_capacity_;
    collision_tail_    = hashable_capacity_;
  }

  // Member Function
  [[nodiscard]] allocator_type get_allocator() const {
    return tree_.get_allocator();
  }

  // Element Access

  mapped_type& at(const key_type& key)
    requires(! std::is_same_v<mapped_type, IsAFlatSet>)
  {
    size_type index = _find(key).first;
    if (index != empty_index_) {
      return tree_[index].pair_.second;
    }
    throw std::out_of_range("dro::hash_flat_base::at");
  }

  const mapped_type& at(const key_type& key) const
    requires(! std::is_same_v<mapped_type, IsAFlatSet>)
  {
    size_type index = _find(key).first;
    if (index != empty_index_) {
      return tree_[index].pair_.second;
    }
    throw std::out_of_range("dro::hash_flat_base::at");
  }

  mapped_type& operator[](const key_type& key)
    requires(! std::is_same_v<mapped_type, IsAFlatSet>)
  {
    size_type index = _emplace(key).first.index_;
    return tree_[index].pair_.second;
  }

  mapped_type& operator[](key_type&& key)
    requires(! std::is_same_v<mapped_type, IsAFlatSet>)
  {
    size_type index = _emplace(key).first.index_;
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

  reverse_iterator rbegin() { return reverse_iterator(this, _last()); }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(this, _last());
  }

  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(this, _last());
  }

  reverse_iterator rend() { return reverse_iterator(this, empty_index_); }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(this, empty_index_);
  }

  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(this, empty_index_);
  }

  // Capacity

  [[nodiscard]] size_type size() const noexcept { return size_; }

  [[nodiscard]] size_type max_size() const noexcept { return empty_index_; }

  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

  [[nodiscard]] size_type capacity() const noexcept { return capacity_; }

  // Modifiers

  void clear() noexcept {
    for (auto& node : tree_) {
      _set_empty(node.fingerprint_full_clr_);
      node.next_ = 0;
    }
    collision_head_    = hashable_capacity_;
    collision_tail_    = hashable_capacity_;
    first_index_cache_ = empty_index_;
    last_index_cache_  = empty_index_;
    root_              = empty_index_;
    size_              = 0;
  }

  std::pair<iterator, bool> insert(const value_type& pair)
    requires(! std::is_same_v<mapped_type, IsAFlatSet>)
  {
    return _emplace(pair.first, pair.second);
  }

  std::pair<iterator, bool> insert(value_type&& pair)
    requires(! std::is_same_v<mapped_type, IsAFlatSet>)
  {
    return _emplace(std::move(pair.first), std::move(pair.second));
  }

  std::pair<iterator, bool> insert(const key_type& key)
    requires std::is_same_v<mapped_type, IsAFlatSet>
  {
    return _emplace(key);
  }

  std::pair<iterator, bool> insert(key_type&& key)
    requires std::is_same_v<mapped_type, IsAFlatSet>
  {
    return _emplace(std::move(key));
  }

  template <class InputIt> void insert(InputIt first, InputIt last) {
    while (first != last) {
      insert(*first);
      ++first;
    }
  }

  void insert(std::initializer_list<value_type> ilist) {
    insert(ilist.begin(), ilist.end());
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    return _emplace(std::forward<Args>(args)...);
  }

  iterator erase(iterator pos) {
    return iterator(this, _erase(tree_[pos.index_].pair_.first).second);
  }

  iterator erase(const_iterator pos) {
    return iterator(this, _erase(tree_[pos.index_].pair_.first).second);
  }

  iterator erase(iterator first, iterator last) {
    size_type upper_index = empty_index_;
    for (; first != last && first != end(); ++first) {
      upper_index = _erase(tree_[first.index_].pair_.first).second;
    }
    return iterator(this, upper_index);
  }

  iterator erase(const_iterator first, const_iterator last) {
    size_type upper_index = empty_index_;
    for (; first != last && first != cend(); ++first) {
      upper_index = _erase(tree_[first.index_].pair_.first).second;
    }
    return iterator(this, upper_index);
  }

  size_type erase(const key_type& key) { return _erase(key).first; }

  template <typename K>
  size_type erase(K&& x)
    requires std::is_convertible_v<K, key_type>
  {
    return _erase(x).first;
  }

  void swap(self_type& other)
  // noexcept(FlatTree_NoThrow<Key> && FlatTree_NoThrow<Value>)
  {
    std::swap(*this, other);
  }

  node_type extract(const_iterator position) { return tree_[position.index_]; }

  node_type extract(const key_type& key) {
    size_type index = _find(key).first;
    if (index == empty_index_) {
      // TBD
    }
    return tree_[index];
  }

  template <typename K>
  node_type extract(K&& x)
    requires std::is_convertible_v<K, key_type>
  {
    size_type index = _find(x).first;
    if (index == empty_index_) {
      // TBD
    }
    return tree_[index];
  }

  void merge(self_type& source) {
    for (auto it = source.begin(); it != source.end(); ++it) { _insert(*it); }
  }

  void merge(self_type&& source) {
    for (auto it = source.begin(); it != source.end(); ++it) { _insert(*it); }
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
    return iterator(this, _find(key).first);
  }

  [[nodiscard]] const_iterator find(const key_type& key) const {
    return const_iterator(this, _find(key).first);
  }

  template <typename K>
  [[nodiscard]] iterator find(const K& x)
    requires std::is_convertible_v<K, key_type>
  {
    return iterator(this, _find(x).first);
  }

  template <typename K>
  [[nodiscard]] const_iterator find(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return const_iterator(this, _find(x).first);
  }

  [[nodiscard]] bool contains(const key_type& key) const {
    return _find(key).first != empty_index_;
  }

  template <typename K>
  [[nodiscard]] bool contains(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return _find(x).first != empty_index_;
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
    return iterator(this, _lower_bound(key));
  }

  [[nodiscard]] const_iterator lower_bound(const key_type& key) const {
    return const_iterator(this, _lower_bound(key));
  }

  template <typename K>
  [[nodiscard]] iterator lower_bound(const K& x)
    requires std::is_convertible_v<K, key_type>
  {
    return iterator(this, _lower_bound(x));
  }

  template <typename K>
  [[nodiscard]] const_iterator lower_bound(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return const_iterator(this, _lower_bound(x));
  }

  [[nodiscard]] iterator upper_bound(const key_type& key) {
    return iterator(this, _upper_bound(key));
  }

  [[nodiscard]] const_iterator upper_bound(const key_type& key) const {
    return const_iterator(this, _upper_bound(key));
  }

  template <typename K>
  [[nodiscard]] iterator upper_bound(const K& x)
    requires std::is_convertible_v<K, key_type>
  {
    return iterator(this, _upper_bound(x));
  }

  template <typename K>
  [[nodiscard]] const_iterator upper_bound(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return const_iterator(this, _upper_bound(x));
  }

  // Observers ///////

  [[nodiscard]] key_compare key_comp() const noexcept { return compare_; }

  [[nodiscard]] key_compare value_comp() const noexcept { return compare_; }

  [[nodiscard]] hasher hash_function() const { return hash_; }

  [[nodiscard]] key_equal key_eq() const { return equal_; }

private:
  // For FlatMap
  template <typename... Args>
  std::pair<iterator, bool> _emplace(Args&&... args) {
    key_type key {};
    _build_key(key, std::forward<Args>(args)...);
    auto hashmap_location  = _emplace_hashmap(key);
    size_type insert_index = hashmap_location.first;
    if (! hashmap_location.second) {
      return {iterator(this, insert_index), false};
    }
    _validate_size();
    size_type parent_index = _check_cached_extrema(key, insert_index);
    if (parent_index == empty_index_) {
      parent_index = _find_parent_location(key);
    }
    // Create Node
    auto& insert_node = tree_[insert_index];
    _emplace_key(insert_node, key);
    _emplace_value(insert_node, std::forward<Args>(args)...);
    insert_node.parent_ = parent_index;
    insert_node.left_   = empty_index_;
    insert_node.right_  = empty_index_;
    ++size_;
    // Update root_
    if (root_ == empty_index_) {
      root_ = insert_index;
      _set_black(tree_[root_].fingerprint_full_clr_);
    } else {
      _update_parent(key, parent_index, insert_index);
      _fix_insert(insert_index);
    }
    return {iterator(this, insert_index), true};
  }

  std::pair<size_type, bool> _emplace_hashmap(const key_type& key) {
    uint64_t key_hash      = _hash(key);
    uint64_t fingerprint   = _get_fingerprint(key_hash);
    size_type insert_index = _index_from_hash(key_hash);

    if (_get_full(tree_[insert_index].fingerprint_full_clr_)) {
      // Check the collision chain
      size_type chain_node {insert_index};
      size_type prev_index {};
      do {
        auto& bucket = tree_[chain_node];
        if (fingerprint == _get_fingerprint(bucket.fingerprint_full_clr_) &&
            equal_(bucket.pair_.first, key)) {
          return {insert_index, false};
        }
        prev_index = chain_node;
        chain_node = bucket.next_;
      } while (chain_node);
      // Not in the hashmap, will 100% insert.
      if (! _validate_load_factor_bounds()) {
        return _emplace_hashmap(key);
      }
      // Insert at the end of the collisions vector
      if (collision_tail_ == collision_head_) {
        if (! _validate_collision_space_bounds()) {
          return _emplace_hashmap(key);
        }
        insert_index = collision_head_;
        ++collision_head_;
        ++collision_tail_;
      }
      // Fill in the empty slots
      else {
        insert_index = tree_[collision_head_].next_;
        if (insert_index == collision_tail_) {
          collision_tail_ = collision_head_;
        } else {
          tree_[collision_head_].next_ = tree_[insert_index].next_;
        }
      }
      // Update chain
      tree_[prev_index].next_ = insert_index;
    } else if (! _validate_load_factor_bounds()) {
      return _emplace_hashmap(key);
    }
    // Good to go on insert, not in the tree
    _set_fingerprint(tree_[insert_index].fingerprint_full_clr_, key_hash);
    tree_[insert_index].next_ = 0;
    return {insert_index, true};
  }

  template <typename First, typename... Args>
  void _build_key(key_type& key, First&& first, Args&&... args)
    requires(! std::is_same_v<mapped_type, IsAFlatSet> &&
             std::is_move_assignable_v<key_type>)
  {
    key = first;
  }

  template <typename First, typename... Args>
  void _build_key(key_type& key, First&& first, Args&&... args)
    requires(! std::is_same_v<mapped_type, IsAFlatSet> &&
             ! std::is_move_assignable_v<key_type> &&
             std::is_copy_assignable_v<key_type>)
  {
    key_type non_movable(first);
    key = non_movable;
  }

  template <typename... Args>
  void _build_key(key_type& key, Args&&... args)
    requires(std::is_same_v<mapped_type, IsAFlatSet> &&
             std::is_constructible_v<key_type, Args && ...> &&
             std::is_move_assignable_v<key_type>)
  {
    key = key_type(std::forward<Args>(args)...);
  }

  template <typename... Args>
  void _build_key(key_type& key, Args&&... args)
    requires(std::is_same_v<mapped_type, IsAFlatSet> &&
             std::is_constructible_v<key_type, Args && ...> &&
             std::is_copy_assignable_v<key_type> &&
             ! std::is_move_assignable_v<key_type>)
  {
    key_type non_movable(std::forward<Args>(args)...);
    key = non_movable;
  }

  void _emplace_key(auto& newNodeRef,
                    key_type& key) noexcept(hashflat_nothrow_v)
    requires(std::is_move_assignable_v<key_type>)
  {
    newNodeRef.pair_.first = std::move(key);
  }

  void _emplace_key(auto& insert_node,
                    key_type& key) noexcept(hashflat_nothrow_v)
    requires(std::is_copy_assignable_v<key_type> &&
             ! std::is_move_assignable_v<key_type>)
  {
    insert_node.pair_.first = key;
  }

  template <typename... Args>
  void _emplace_value(auto& insert_node, Args&&... args) noexcept
    requires(std::is_same_v<mapped_type, IsAFlatSet>)
  {}// Intentionally blank for flat_set

  template <typename First, typename... Args>
  void _emplace_value(auto& insert_node, First&&,
                      Args&&... args) noexcept(hashflat_nothrow_v)
    requires(! std::is_same_v<mapped_type, IsAFlatSet> &&
             std::is_move_assignable_v<mapped_type> &&
             std::is_constructible_v<mapped_type, Args && ...>)
  {
    insert_node.pair_.second = mapped_type(std::forward<Args>(args)...);
  }

  template <typename First, typename... Args>
  void _emplace_value(auto& insert_node, First&&,
                      Args&&... args) noexcept(hashflat_nothrow_v)
    requires(! std::is_same_v<mapped_type, IsAFlatSet> &&
             std::is_copy_assignable_v<mapped_type> &&
             ! std::is_move_assignable_v<mapped_type> &&
             std::is_constructible_v<mapped_type, Args && ...>)
  {
    mapped_type non_movable(std::forward<Args>(args)...);
    insert_node.pair_.second = non_movable;
  }

  size_type _check_cached_extrema(const key_type& key,
                                  const size_type& insert_index) {
    size_type parent = empty_index_;
    if (last_index_cache_ == empty_index_) {
      first_index_cache_ = insert_index;
      last_index_cache_  = insert_index;
    } else if (compare_(key, tree_[first_index_cache_].pair_.first)) {
      parent             = first_index_cache_;
      first_index_cache_ = insert_index;
    } else if (compare_(tree_[last_index_cache_].pair_.first, key)) {
      parent            = last_index_cache_;
      last_index_cache_ = insert_index;
    }
    return parent;
  }

  size_type _find_parent_location(const key_type& key) {
    size_type node   = root_;
    size_type parent = empty_index_;
    while (node != empty_index_) {
      parent          = node;
      auto& parentRef = tree_[parent];
      _prefetch_binary_search(parentRef);
      bool compare = compare_(key, parentRef.pair_.first);
      node         = compare ? parentRef.left_ : parentRef.right_;
    }
    return parent;
  }

  void _update_parent(const key_type& key, const size_type& parent,
                      const size_type& insert_index) {
    auto& parent_node = tree_[parent];
    if (compare_(key, parent_node.pair_.first)) {
      parent_node.left_ = insert_index;
    } else {
      parent_node.right_ = insert_index;
    }
  }

  std::pair<bool, size_type> _erase(const key_type& key) {
    auto find_location    = _find(key);
    size_type erase_index = find_location.first;
    if (erase_index == empty_index_) {
      return {false, empty_index_};
    }
    // For return iterator
    size_type upper_index = _next(erase_index);
    size_type lower_index = _prev(erase_index);
    if (upper_index == empty_index_) {
      last_index_cache_ = lower_index;
    }
    if (lower_index == empty_index_) {
      first_index_cache_ = upper_index;
    }
    // Erase Node
    _erase_hashmap(erase_index, find_location.second);
    auto& erase_node = tree_[erase_index];
    bool color       = _get_color(erase_node.fingerprint_full_clr_);
    size_type parent = erase_node.parent_;
    size_type child  = empty_index_;
    // One or both children empty
    if (erase_node.left_ == empty_index_ || erase_node.right_ == empty_index_) {
      if (erase_node.left_ == empty_index_) {
        child = erase_node.right_;
      } else {
        child = erase_node.left_;
      }
      _assign_parent(child, erase_node.parent_);
      _update_children(child, parent, erase_index);
      // Both children full
    } else {
      size_type min_index = _min_value_index(erase_index);
      auto& min_node      = tree_[min_index];
      child               = min_node.right_;
      parent              = min_node.parent_;
      color               = _get_color(min_node.fingerprint_full_clr_);
      _assign_parent(child, parent);
      if (parent == erase_index) {
        tree_[parent].right_ = child;
        parent               = min_index;
      } else {
        tree_[parent].left_ = child;
      }
      _transfer_node_data(min_index, erase_index);
      _update_children(min_index, erase_node.parent_, erase_index);
      tree_[erase_node.left_].parent_ = min_index;
      _assign_parent(erase_node.right_, min_index);
    }
    if (color == BLACK_) {
      _fix_erase(child, parent);
    }
    --size_;
    return {true, upper_index};
  }

  void _erase_hashmap(size_type& erase_index, const size_type& prev_index) {
    auto& erase_node = tree_[erase_index];
    size_type next   = erase_node.next_;
    if (erase_index < hashable_capacity_) {
      if (next == 0) {
        _set_empty(erase_node.fingerprint_full_clr_);
        return;
      }
      _swap_node_position(next, erase_index);
      erase_index = next;
    } else {
      tree_[prev_index].next_ = next;
    }
    _set_empty(tree_[erase_index].fingerprint_full_clr_);
    tree_[erase_index].next_ = 0;
    // Add to collision linked list
    tree_[collision_tail_].next_ = erase_index;
    collision_tail_              = erase_index;
  }

  template <typename K>
  std::pair<size_type, size_type> _find(const K& key) const
    requires std::is_convertible_v<K, key_type>
  {
    // Find node with builtin hashmap
    uint64_t key_hash    = _hash(key);
    uint64_t fingerprint = _get_fingerprint(key_hash);
    // This is to avoid needing a doubly linked list
    size_type prev_index {};

    for (size_type index = _index_from_hash(key_hash);;) {
      auto& bucket                   = tree_[index];
      const auto& bucket_fingerprint = bucket.fingerprint_full_clr_;
      if (_get_full(bucket_fingerprint) &&
          fingerprint == _get_fingerprint(bucket_fingerprint) &&
          equal_(bucket.pair_.first, key)) {
        return {index, prev_index};
      }
      if (bucket.next_ == 0) {
        break;
      }
      prev_index = index;
      index      = bucket.next_;
    }
    return {empty_index_, prev_index};
  }

  void _prefetch_binary_search(auto& node) const {
    size_type next_left =
        (node.left_ == empty_index_) ? capacity_ - 1 : node.left_;
    size_type next_right =
        (node.right_ == empty_index_) ? capacity_ - 1 : node.right_;
    __builtin_prefetch(&tree_[next_left]);
    __builtin_prefetch(&tree_[next_right]);
  }

  void _assign_parent(size_type index, size_type parent) {
    if (index != empty_index_) {
      tree_[index].parent_ = parent;
    }
  }

  size_type _upper_bound(const key_type& key) const {
    size_type index      = root_;
    size_type last_index = empty_index_;
    // Find node with binary search
    while (index != empty_index_) {
      auto& node = tree_[index];
      _prefetch_binary_search(node);
      bool compare =
          (compare_(node.pair_.first, key) || equal_(node.pair_.first, key));
      last_index = compare ? last_index : index;
      index      = compare ? node.right_ : node.left_;
    }
    return last_index;
  }

  size_type _lower_bound(const key_type& key) const {
    size_type index      = root_;
    size_type last_index = empty_index_;
    // Find node with binary search
    while (index != empty_index_) {
      auto& node = tree_[index];
      _prefetch_binary_search(node);
      if (equal_(node.pair_.first, key)) {
        return index;
      }
      bool compare = compare_(node.pair_.first, key);
      last_index   = compare ? last_index : index;
      index        = compare ? node.right_ : node.left_;
    }
    return last_index;
  }

  void _transfer_node_data(const size_type& min_index,
                           const size_type& erase_index) {
    auto& min_node   = tree_[min_index];
    auto& erase_node = tree_[erase_index];
    min_node.parent_ = erase_node.parent_;
    bool erase_color = _get_color(erase_node.fingerprint_full_clr_);
    _set_color(min_node.fingerprint_full_clr_, erase_color);
    min_node.left_  = erase_node.left_;
    min_node.right_ = erase_node.right_;
  }

  void _update_children(size_type child, size_type parent,
                        size_type erase_index) {
    if (parent == empty_index_) {
      root_ = child;
      return;
    }
    auto& parent_node = tree_[parent];
    if (parent_node.left_ == erase_index) {
      parent_node.left_ = child;
    } else {
      parent_node.right_ = child;
    }
  }

  void _fix_insert(size_type node) {
    size_type parent = tree_[node].parent_;
    while (node != root_ &&
           _get_color(tree_[node].fingerprint_full_clr_) == RED_ &&
           _get_color(tree_[parent].fingerprint_full_clr_) == RED_) {
      auto& parent_node      = tree_[parent];
      size_type grandparent  = parent_node.parent_;
      auto& grandparent_node = tree_[grandparent];
      size_type uncle        = (parent == grandparent_node.left_)
                                   ? grandparent_node.right_
                                   : grandparent_node.left_;
      auto& uncle_node       = tree_[uncle];
      // Update Uncle
      if (uncle != empty_index_ &&
          _get_color(uncle_node.fingerprint_full_clr_) == RED_) {
        _update_colors(uncle_node, parent_node, grandparent_node);
        node = grandparent;
      } else {
        if (parent == grandparent_node.left_) {
          // Left Tree Insert
          if (node == parent_node.right_) {
            _rotate_left(parent);
            std::swap(node, parent);
          }
          _rotate_right(grandparent);
        } else {
          // Right Tree Insert
          if (node == parent_node.left_) {
            _rotate_right(parent);
            std::swap(node, parent);
          }
          _rotate_left(grandparent);
        }
        bool parent_color = _get_color(tree_[parent].fingerprint_full_clr_);
        bool grandparent_color =
            _get_color(grandparent_node.fingerprint_full_clr_);
        _set_color(grandparent_node.fingerprint_full_clr_, parent_color);
        _set_color(tree_[parent].fingerprint_full_clr_, grandparent_color);
        node = parent;
      }
      parent = tree_[node].parent_;
    }
    _set_black(tree_[root_].fingerprint_full_clr_);
  }

  void _update_colors(auto& uncle_node, auto& parent_node,
                      auto& grandparent_node) {
    _set_red(grandparent_node.fingerprint_full_clr_);
    _set_black(parent_node.fingerprint_full_clr_);
    _set_black(uncle_node.fingerprint_full_clr_);
  }

  void _fix_erase(size_type index, size_type parent) {
    size_type sibling = empty_index_;
    while (index != root_ &&
           (index == empty_index_ ||
            _get_color(tree_[index].fingerprint_full_clr_) == BLACK_)) {
      auto& parent_node = tree_[parent];
      bool is_left      = (index == parent_node.left_);
      // Analyze Sibling
      sibling = (is_left) ? parent_node.right_ : parent_node.left_;
      _check_sibling_red(sibling, parent, is_left);
      auto& sibling_node = tree_[sibling];
      if (_check_sibling_black(sibling_node, index, parent)) {
      } else {
        if (is_left) {
          _fix_erase_left(sibling_node, sibling, parent);
          _rotate_left(parent);
        } else {
          _fix_erase_right(sibling_node, sibling, parent);
          _rotate_right(parent);
        }
        index = root_;
        break;
      }
    }
    if (index != empty_index_) {
      _set_black(tree_[index].fingerprint_full_clr_);
    }
  }

  void _check_sibling_red(size_type& sibling, size_type& parent, bool is_left) {
    if (_get_color(tree_[sibling].fingerprint_full_clr_) == RED_) {
      _set_black(tree_[sibling].fingerprint_full_clr_);
      _set_red(tree_[parent].fingerprint_full_clr_);
      if (is_left) {
        _rotate_left(parent);
        sibling = tree_[parent].right_;
      } else {
        _rotate_right(parent);
        sibling = tree_[parent].left_;
      }
    }
  }

  bool _check_sibling_black(auto& sibling_node, size_type& index,
                            size_type& parent) {
    if ((sibling_node.left_ == empty_index_ ||
         _get_color(tree_[sibling_node.left_].fingerprint_full_clr_) ==
             BLACK_) &&
        (sibling_node.right_ == empty_index_ ||
         _get_color(tree_[sibling_node.right_].fingerprint_full_clr_) ==
             BLACK_)) {
      _set_red(sibling_node.fingerprint_full_clr_);
      index  = parent;
      parent = tree_[index].parent_;
      return true;
    }
    return false;
  }

  void _fix_erase_left(auto& sibling_node, size_type& sibling,
                       size_type& parent) {
    if (sibling_node.right_ == empty_index_ ||
        _get_color(tree_[sibling_node.right_].fingerprint_full_clr_) ==
            BLACK_) {
      if (sibling_node.left_ != empty_index_) {
        _set_black(tree_[sibling_node.left_].fingerprint_full_clr_);
      }
      _set_red(sibling_node.fingerprint_full_clr_);
      _rotate_right(sibling);
      sibling = tree_[parent].right_;
    }
    bool parent_color = _get_color(tree_[parent].fingerprint_full_clr_);
    _set_color(tree_[sibling].fingerprint_full_clr_, parent_color);
    _set_black(tree_[parent].fingerprint_full_clr_);
    if (tree_[sibling].right_ != empty_index_) {
      _set_black(tree_[tree_[sibling].right_].fingerprint_full_clr_);
    }
  }

  void _fix_erase_right(auto& sibling_node, size_type& sibling,
                        size_type& parent) {
    if (sibling_node.left_ == empty_index_ ||
        _get_color(tree_[sibling_node.left_].fingerprint_full_clr_) == BLACK_) {
      if (sibling_node.right_ != empty_index_) {
        _set_black(tree_[sibling_node.right_].fingerprint_full_clr_);
      }
      _set_red(sibling_node.fingerprint_full_clr_);
      _rotate_left(sibling);
      sibling = tree_[parent].left_;
    }
    _set_color(tree_[sibling].fingerprint_full_clr_,
               _get_color(tree_[parent].fingerprint_full_clr_));
    _set_black(tree_[parent].fingerprint_full_clr_);
    if (tree_[sibling].left_ != empty_index_) {
      _set_black(tree_[tree_[sibling].left_].fingerprint_full_clr_);
    }
  }

  void _rotate_left(const size_type& index) {
    auto& node            = tree_[index];
    size_type child       = node.right_;
    auto& child_node      = tree_[child];
    size_type index_right = node.right_ = child_node.left_;
    if (index_right != empty_index_) {
      tree_[index_right].parent_ = index;
    }
    _rotate_parents(node, child_node, index, child);
    child_node.left_ = index;
    node.parent_     = child;
  }

  void _rotate_right(const size_type& index) {
    auto& node           = tree_[index];
    size_type child      = node.left_;
    auto& child_node     = tree_[child];
    size_type index_left = node.left_ = child_node.right_;
    if (index_left != empty_index_) {
      tree_[index_left].parent_ = index;
    }
    _rotate_parents(node, child_node, index, child);
    child_node.right_ = index;
    node.parent_      = child;
  }

  void _rotate_parents(auto& node, auto& child_node, const size_type& index,
                       const size_type& child) {
    size_type parent = child_node.parent_ = node.parent_;
    auto& parent_node                     = tree_[parent];
    if (parent == empty_index_) {
      root_ = child;
    } else if (index == parent_node.left_) {
      parent_node.left_ = child;
    } else {
      parent_node.right_ = child;
    }
  }

  void _swap_node_position(size_type index_keep, size_type index_remove) {
    // Update cached indexes
    if (index_keep == first_index_cache_) {
      first_index_cache_ = index_remove;
    }
    if (index_keep == last_index_cache_) {
      last_index_cache_ = index_remove;
    }
    auto& node_keep         = tree_[index_keep];
    auto& node_remove       = tree_[index_remove];
    size_type parent_keep   = node_keep.parent_;
    size_type left_keep     = node_keep.left_;
    size_type right_keep    = node_keep.right_;
    size_type parent_remove = node_remove.parent_;
    size_type left_remove   = node_remove.left_;
    size_type right_remove  = node_remove.right_;
    // Swap Parent Index
    if (parent_keep != empty_index_) {
      if (tree_[parent_keep].left_ == index_keep) {
        tree_[parent_keep].left_ = index_remove;
      } else {
        tree_[parent_keep].right_ = index_remove;
      }
    }
    if (parent_remove != empty_index_) {
      if (tree_[parent_remove].left_ == index_remove) {
        tree_[parent_remove].left_ = index_keep;
      } else {
        tree_[parent_remove].right_ = index_keep;
      }
    }
    // Check if Nodes are related
    if (parent_keep == index_remove) {
      node_keep.parent_ = index_keep;
    }
    if (parent_remove == index_keep) {
      node_remove.parent_ = index_remove;
    }
    // Swap Children Index
    if (left_keep != empty_index_) {
      tree_[left_keep].parent_ = index_remove;
    }
    if (right_keep != empty_index_) {
      tree_[right_keep].parent_ = index_remove;
    }
    if (left_remove != empty_index_) {
      tree_[left_remove].parent_ = index_keep;
    }
    if (right_remove != empty_index_) {
      tree_[right_remove].parent_ = index_keep;
    }
    // Update Root if in swap
    if (root_ == index_keep) {
      root_ = index_remove;
    }
    if (root_ == index_remove) {
      root_ = index_keep;
    }
    // Swap vector position
    std::swap(node_keep, node_remove);
  }

  size_type _min_value_index(size_type index) const {
    index          = tree_[index].right_;
    size_type left = tree_[index].left_;
    while (left != empty_index_) {
      index = left;
      left  = tree_[index].left_;
    }
    return index;
  }

  size_type _first() const { return first_index_cache_; }

  size_type _last() const { return last_index_cache_; }

  size_type _next(size_type index) const {
    if (index == empty_index_) {
      return empty_index_;
    }
    // Check right sub-tree
    if (tree_[index].right_ != empty_index_) {
      index          = tree_[index].right_;
      size_type left = tree_[index].left_;

      while (left != empty_index_) {
        index = left;
        left  = tree_[index].left_;
      }
      return index;
    }
    // If no right sub-tree then backtrack
    size_type parent = tree_[index].parent_;
    while (parent != empty_index_ && index == tree_[parent].right_) {
      index  = parent;
      parent = tree_[index].parent_;
    }
    // End of tree
    if (parent == root_ && index == tree_[parent].right_) {
      return empty_index_;
    }
    return parent;
  }

  size_type _prev(size_type index) const {
    if (index == empty_index_) {
      return empty_index_;
    }
    // Check left sub-tree
    if (tree_[index].left_ != empty_index_) {
      index           = tree_[index].left_;
      size_type right = tree_[index].right_;
      while (right != empty_index_) {
        index = right;
        right = tree_[index].right_;
      }
      return index;
    }
    // If no left sub-tree then backtrack
    size_type parent = tree_[index].parent_;
    while (parent != empty_index_ && index == tree_[parent].left_) {
      index  = parent;
      parent = tree_[index].parent_;
    }
    // End of tree
    if (parent == root_ && index == tree_[parent].left_) {
      return empty_index_;
    }
    return parent;
  }

  void _validate_size() {
    if (size_ == empty_index_) {
      throw std::runtime_error("Size exceeds max capacity of size type. "
                               "Increase size type of tree.");
    }
  }

  [[nodiscard]] bool _validate_load_factor_bounds() {
    auto capacity_float    = static_cast<float>(capacity_);
    size_type max_capacity = capacity_float * load_factor_;
    if (size_ + 1 > max_capacity) {
      size_type new_capacity =
          (static_cast<float>(empty_index_) / growth_multiple_ < capacity_)
              ? empty_index_
              : capacity_float * growth_multiple_;
      _rehash(new_capacity);
      return false;
    }
    return true;
  }

  [[nodiscard]] bool _validate_collision_space_bounds() {
    if (collision_head_ >= capacity_) {
      auto capacity_float = static_cast<float>(capacity_);
      size_type new_capacity =
          (static_cast<float>(empty_index_) / growth_multiple_ < capacity_)
              ? empty_index_
              : capacity_float * growth_multiple_;
      _rehash(new_capacity);
      return false;
    }
    return true;
  }

  void _rehash(const size_type& count) {
    self_type other(count, get_allocator());
    for (auto it = begin(); it != end(); ++it) { other.insert(*it); }
    swap(other);
  }

  [[nodiscard]] size_type _index_from_hash(const uint64_t& hash) const {
    uint64_t mask   = ~0UL >> __builtin_clzl(hashable_capacity_);
    size_type index = hash & mask;
    return (index >= hashable_capacity_) ? index - hashable_capacity_ : index;
  }

  template <typename K>
  [[nodiscard]] uint64_t _hash(const K& key) const
      noexcept(noexcept(Hash()(key)))
    requires std::is_convertible_v<K, key_type>
  {
    return hash_(key);
  }

  void _set_fingerprint(uint64_t& fingerprint_full_clr,
                        const uint64_t& hash) noexcept {
    fingerprint_full_clr = hash;
    _set_red(fingerprint_full_clr);
    _set_full(fingerprint_full_clr);
  }

  [[nodiscard]] uint64_t
  _get_fingerprint(const uint64_t& fingerprint_full_clr) const noexcept {
    return fingerprint_full_clr >> 2;
  }

  void _set_color(uint64_t& fingerprint_full_clr, const bool& color) {
    _set_red(fingerprint_full_clr);
    fingerprint_full_clr |= color << 1;
  }

  void _set_red(uint64_t& fingerprint_full_clr) noexcept {
    uint64_t mask = ~2;
    fingerprint_full_clr &= mask;
  }

  void _set_black(uint64_t& fingerprint_full_clr) noexcept {
    fingerprint_full_clr |= 2;
  }

  [[nodiscard]] bool
  _get_color(const uint64_t& fingerprint_full_clr) const noexcept {
    return fingerprint_full_clr & 2;
  }

  void _set_empty(uint64_t& fingerprint_full_clr) noexcept {
    uint64_t mask = ~1;
    fingerprint_full_clr &= mask;
  }

  void _set_full(uint64_t& fingerprint_full_clr) noexcept {
    fingerprint_full_clr |= 1;
  }

  [[nodiscard]] bool
  _get_full(const uint64_t& fingerprint_full_clr) const noexcept {
    return fingerprint_full_clr & 1;
  }
};

}// namespace detail

template <detail::hashflat_t Key, detail::hashflat_t Value,
          detail::integral_t SizeT = std::size_t,
          typename Compare = std::less<Key>, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator =
              std::allocator<detail::flatnode<std::pair<Key, Value>, SizeT>>>
class hash_flat_map
    : public detail::hash_flat_base<Key, Value, std::pair<Key, Value>, SizeT,
                                    Compare, Hash, KeyEqual, Allocator> {
  using size_type = SizeT;
  using base_type =
      detail::hash_flat_base<Key, Value, std::pair<Key, Value>, SizeT, Compare,
                             Hash, KeyEqual, Allocator>;

public:
  explicit hash_flat_map(size_type capacity         = 2,
                         const Allocator& allocator = Allocator())
      : base_type(capacity, allocator) {}
};

template <detail::hashflat_t Key, detail::integral_t SizeT = std::size_t,
          typename Compare = std::less<Key>, typename Hash = std::hash<Key>,
          typename KeyEqual  = std::equal_to<Key>,
          typename Allocator = std::allocator<
              detail::flatnode<detail::flatset_pair<Key>, SizeT>>>
class hash_flat_set
    : public detail::hash_flat_base<Key, detail::IsAFlatSet,
                                    detail::flatset_pair<Key>, SizeT, Compare,
                                    Hash, KeyEqual, Allocator> {
  using size_type = SizeT;
  using base_type =
      detail::hash_flat_base<Key, detail::IsAFlatSet, detail::flatset_pair<Key>,
                             SizeT, Compare, Hash, KeyEqual, Allocator>;

public:
  explicit hash_flat_set(size_type capacity         = 2,
                         const Allocator& allocator = Allocator())
      : base_type(capacity, allocator) {}
};

}// namespace dro
#endif
