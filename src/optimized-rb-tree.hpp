// Andrew Drogalis

#ifndef DRO_SPSC_QUEUE
#define DRO_SPSC_QUEUE

#include <climits>
#include <cstddef>
#include <limits>
#include <vector>

namespace dro
{

template <typename T> class OptimizedRBTree
{
  using size_type = std::size_t;

private:
  constexpr static size_type empty_value_ =
      std::numeric_limits<size_type>::max();
  constexpr static bool RED_   = false;
  constexpr static bool BLACK_ = true;

  struct Node
  {
    T data_;
    size_type parent_;
    size_type left_  = empty_value_;
    size_type right_ = empty_value_;
    bool color_      = RED_;

    explicit Node(T data, size_type parent = empty_value_)
        : data_(data), parent_(parent)
    {
    }
  };

  std::vector<Node> tree_;
  size_type root_ = empty_value_;

public:
  OptimizedRBTree() {}

  // Public function: Insert a value into Red-Black Tree
  void insert(T key)
  {
    size_type node    = tree_.size();
    size_type parent  = empty_value_;
    size_type current = root_;
    while (current != empty_value_)
    {
      parent = current;
      if (key < tree_[current].data_)
      {
        current = tree_[current].left_;
      }
      else
      {
        current = tree_[current].right_;
      }
    }
    tree_.emplace_back(key, parent);
    if (parent == empty_value_)
    {
      root_ = node;
    }
    else if (key < tree_[parent].data_)
    {
      tree_[parent].left_ = node;
    }
    else
    {
      tree_[parent].right_ = node;
    }
    fixInsert(node);
  }

  // Public function: Remove a value from Red-Black Tree
  void remove(T key)
  {
    size_type node = root_;
    size_type z    = empty_value_;
    size_type x    = empty_value_;
    size_type y    = empty_value_;
    while (node != empty_value_)
    {
      if (tree_[node].data_ == key)
      {
        z = node;
      }

      if (tree_[node].data_ <= key)
      {
        node = tree_[node].right_;
      }
      else
      {
        node = tree_[node].left_;
      }
    }

    if (z == empty_value_)
    {
      // cout << "Key not found in the tree" << endl;
      return;
    }

    y                   = z;
    bool yOriginalColor = tree_[y].color_;
    if (tree_[z].left_ == empty_value_)
    {
      x = tree_[z].right_;
      transplant(root_, z, tree_[z].right_);
    }
    else if (tree_[z].right_ == empty_value_)
    {
      x = tree_[z].left_;
      transplant(root_, z, tree_[z].left_);
    }
    else
    {
      y              = minValueNode(tree_[z].right_);
      yOriginalColor = tree_[y].color_;
      x              = tree_[y].right_;
      if (tree_[y].parent_ == z)
      {
        if (x != empty_value_)
        {
          tree_[x].parent_ = y;
        }
      }
      else
      {
        transplant(root_, y, tree_[y].right_);
        size_type right_idx = tree_[y].right_ = tree_[z].right_;
        tree_[right_idx].parent_              = y;
      }
      transplant(root_, z, y);
      size_type left_idx = tree_[y].left_ = tree_[z].left_;
      tree_[left_idx].parent_             = y;
      tree_[y].color_                     = tree_[z].color_;
    }
    // TODO Handle std::swaps for contig memory
    // tree_.pop_back()
    // delete z;
    if (yOriginalColor == BLACK_)
    {
      fixDelete(x);
    }
  }

private:
  // Utility function: Left Rotation
  void rotateLeft(size_type& node)
  {
    size_type child          = tree_[node].right_;
    size_type node_right_idx = tree_[node].right_ = tree_[child].left_;
    if (node_right_idx != empty_value_)
    {
      tree_[node_right_idx].parent_ = node;
    }
    size_type node_parent_idx = tree_[child].parent_ = tree_[node].parent_;
    if (node_parent_idx == empty_value_)
    {
      root_ = child;
    }
    else if (node == tree_[node_parent_idx].left_)
    {
      tree_[node_parent_idx].left_ = child;
    }
    else
    {
      tree_[node_parent_idx].right_ = child;
      tree_[child].left_            = node;
      tree_[node].parent_           = child;
    }
  }

  // Utility function: Right Rotation
  void rotateRight(size_type& node)
  {
    size_type child         = tree_[node].left_;
    size_type node_left_idx = tree_[node].left_ = tree_[child].right_;
    if (node_left_idx != empty_value_)
    {
      tree_[node_left_idx].parent_ = node;
    }
    size_type node_parent_idx = tree_[child].parent_ = tree_[node].parent_;
    if (node_parent_idx == empty_value_)
    {
      root_ = child;
    }
    else if (node == tree_[node_parent_idx].left_)
    {
      tree_[node_parent_idx].left_ = child;
    }
    else
    {
      tree_[node_parent_idx].right_ = child;
      tree_[child].right_           = node;
      tree_[node].parent_           = child;
    }
  }

  // Utility function: Fixing Insertion Violation
  void fixInsert(size_type& node)
  {
    size_type parent      = empty_value_;
    size_type grandparent = empty_value_;
    while (node != root_ && tree_[node].color_ == RED_ &&
           tree_[tree_[node].parent_].color_ == RED_)
    {
      parent      = tree_[node].parent_;
      grandparent = tree_[parent].parent_;
      if (parent == tree_[grandparent].left_)
      {
        size_type uncle = tree_[grandparent].right_;
        if (uncle != empty_value_ && tree_[uncle].color_ == RED_)
        {
          tree_[grandparent].color_ = RED_;
          tree_[parent].color_      = BLACK_;
          tree_[uncle].color_       = BLACK_;
          node                      = grandparent;
        }
        else
        {
          if (node == tree_[parent].right_)
          {
            rotateLeft(parent);
            node   = parent;
            parent = tree_[node].parent_;
          }
          rotateRight(grandparent);
          std::swap(tree_[parent].color_, tree_[grandparent].color_);
          node = parent;
        }
      }
      else
      {
        size_type uncle = tree_[grandparent].left_;
        if (uncle != empty_value_ && tree_[uncle].color_ == RED_)
        {
          tree_[grandparent].color_ = RED_;
          tree_[parent].color_      = BLACK_;
          tree_[uncle].color_       = BLACK_;
          node                      = grandparent;
        }
        else
        {
          if (node == tree_[parent].left_)
          {
            rotateRight(parent);
            node   = parent;
            parent = tree_[node].parent_;
          }
          rotateLeft(grandparent);
          std::swap(tree_[parent].color_, tree_[grandparent].color_);
          node = parent;
        }
      }
    }
    tree_[root_].color_ = BLACK_;
  }

  // Utility function: Fixing Deletion Violation
  void fixDelete(size_type& node)
  {
    while (node != root_ && tree_[node].color_ == BLACK_)
    {
      size_type node_parent_idx = tree_[node].parent_;
      if (node == tree_[node_parent_idx].left_)
      {
        size_type sibling = tree_[node_parent_idx].right_;
        if (tree_[sibling].color_ == RED_)
        {
          tree_[sibling].color_         = BLACK_;
          tree_[node_parent_idx].color_ = RED_;
          rotateLeft(node_parent_idx);
          sibling = tree_[node_parent_idx].right_;
        }
        if ((tree_[sibling].left_ == empty_value_ ||
             tree_[tree_[sibling].left_].color_ == BLACK_) &&
            (tree_[sibling].right_ == empty_value_ ||
             tree_[tree_[sibling].right_].color_ == BLACK_))
        {
          tree_[sibling].color_ = RED_;
          node                  = node_parent_idx;
        }
        else
        {
          if (tree_[sibling].right_ == empty_value_ ||
              tree_[tree_[sibling].right_].color_ == BLACK_)
          {
            if (tree_[sibling].left_ != empty_value_)
            {
              tree_[tree_[sibling].left_].color_ = BLACK_;
            }
            tree_[sibling].color_ = RED_;
            rotateRight(sibling);
            sibling = tree_[node_parent_idx].right_;
          }
          tree_[sibling].color_         = tree_[node_parent_idx].color_;
          tree_[node_parent_idx].color_ = BLACK_;
          if (tree_[sibling].right_ != empty_value_)
          {
            tree_[tree_[sibling].right_].color_ = BLACK_;
          }
          rotateLeft(node_parent_idx);
          node = root_;
        }
      }
      else
      {
        size_type sibling = tree_[node_parent_idx].left_;
        if (tree_[sibling].color_ == RED_)
        {
          tree_[sibling].color_         = BLACK_;
          tree_[node_parent_idx].color_ = RED_;
          rotateRight(node_parent_idx);
          sibling = tree_[node_parent_idx].left_;
        }
        if ((tree_[sibling].left_ == empty_value_ ||
             tree_[tree_[sibling].left_].color_ == BLACK_) &&
            (tree_[sibling].right_ == empty_value_ ||
             tree_[tree_[sibling].right_].color_ == BLACK_))
        {
          tree_[sibling].color_ = RED_;
          node                  = node_parent_idx;
        }
        else
        {
          if (tree_[sibling].left_ == empty_value_ ||
              tree_[tree_[sibling].left_].color_ == BLACK_)
          {
            if (tree_[sibling].right_ != empty_value_)
            {
              tree_[tree_[sibling].right_].color_ = BLACK_;
            }
            tree_[sibling].color_ = RED_;
            rotateLeft(sibling);
            sibling = tree_[node_parent_idx].left_;
          }
          tree_[sibling].color_         = tree_[node_parent_idx].color_;
          tree_[node_parent_idx].color_ = BLACK_;
          if (tree_[sibling].left_ != empty_value_)
          {
            tree_[tree_[sibling].left_].color_ = BLACK_;
          }
          rotateRight(node_parent_idx);
          node = root_;
        }
      }
    }
    tree_[node].color_ = BLACK_;
  }

  // Utility function: Find Node with Minimum Value
  size_type minValueNode(size_type node)
  {
    size_type current = node;
    while (tree_[current].left_ != empty_value_)
    {
      current = tree_[current].left_;
    }
    return current;
  }

  // Utility function: Transplant nodes in Red-Black Tree
  void transplant(size_type& root, size_type& u, size_type& v)
  {
    size_type u_parent_idx = tree_[u].parent_;
    if (u_parent_idx == empty_value_)
    {
      root = v;
    }
    else if (u == tree_[u_parent_idx].left_)
    {
      tree_[u_parent_idx].left_ = v;
    }
    else
    {
      tree_[u_parent_idx].right_ = v;
    }
    if (v != empty_value_)
    {
      tree_[v].parent_ = tree_[u].parent_;
    }
  }
};

}// namespace dro
#endif
