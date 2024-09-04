// Andrew Drogalis


#include <iostream>
#include "../src/optimized-rb-tree.hpp"

int main ()
{
  using RBTree = dro::OptimizedRBTree<int>;
  
  RBTree rbtree {};

  rbtree.insert(1);

  rbtree.insert(5);

  rbtree.insert(3);

  rbtree.remove(1);

  rbtree.remove(3);

  std::cout << "Test\n";

  return 0;
}
