#include "tree.h"
#include <stdlib.h>
#include <stdio.h>

SIString SI_WrapString(const char *s) {
  return (SIString){(char *)s, strlen(s)};
}
int main(int argc, char **argv) {

  TreeNode *root = NewTreeNode(SI_WrapString("hello"), "hello");
  TreeNode_Insert(root, SI_WrapString("foo"), "foo");
  TreeNode_Insert(root, SI_WrapString("bar"), "bar");
  TreeNode_Insert(root, SI_WrapString("zoo"), "zoo");
  TreeNode_Insert(root, SI_WrapString("boo"), "boo");
  TreeNode_Insert(root, SI_WrapString("goo"), "goo");

  TreeNode *n = TreeNode_FindGreater(root, SI_WrapString("foot"));

  // printf("%s\n", n ? n->val : NULL);
  if (n) {
    TreeIterator it = Tree_Iterate(n);
    // it.stack[0].state = 1;
    do {
      printf("%s\n", n ? n->val : NULL);
      n = TreeIterator_Next(&it);
    } while (n);
  }
  return 0;
}
