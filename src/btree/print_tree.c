
#include "btree.h"

/* The queue is used to print the tree in
 * level order, starting from the root
 * printing each entire rank on a separate
 * line, finishing with the leaves.
 */
btnode *queue = NULL;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
bool verbose_output = false;

/* Helper function for printing the
 * tree out.  See print_tree.
 */
void btree_enqueue(btnode *new_node) {
  btnode *c;
  if (queue == NULL) {
    queue = new_node;
    queue->next = NULL;
  } else {
    c = queue;
    while (c->next != NULL) {
      c = c->next;
    }
    c->next = new_node;
    new_node->next = NULL;
  }
}

/* Helper function for printing the
 * tree out.  See print_tree.
 */
btnode *btree_dequeue(void) {
  btnode *n = queue;
  queue = queue->next;
  n->next = NULL;
  return n;
}

/* Prints the bottom row of keys
 * of the tree (with their respective
 * pointers, if the verbose_output flag is set.
 */
void btree_print_leaves(btnode *root) {
  int i;
  btnode *c = root;
  if (root == NULL) {
    printf("Empty tree.\n");
    return;
  }
  while (!c->is_leaf)
    c = c->pointers[0];
  while (true) {
    for (i = 0; i < c->num_keys; i++) {
      if (verbose_output)
        printf("%lx ", (unsigned long)c->pointers[i]);
      printf("%d ", c->keys[i]);
    }
    if (verbose_output)
      printf("%lx ", (unsigned long)c->pointers[order - 1]);
    if (c->pointers[order - 1] != NULL) {
      printf(" | ");
      c = c->pointers[order - 1];
    } else
      break;
  }
  printf("\n");
}

/* Utility function to give the height
 * of the tree, which length in number of edges
 * of the path from the root to any leaf.
 */
int btree_height(btnode *root) {
  int h = 0;
  btnode *c = root;
  while (!c->is_leaf) {
    c = c->pointers[0];
    h++;
  }
  return h;
}

/* Utility function to give the length in edges
 * of the path from any btnode to the root.
 */
int btree_path_to_root(btnode *root, btnode *child) {
  int length = 0;
  btnode *c = child;
  while (c != root) {
    c = c->parent;
    length++;
  }
  return length;
}

/* Prints the B+ tree in the command
 * line in level (rank) order, with the
 * keys in each btnode and the '|' symbol
 * to separate nodes.
 * With the verbose_output flag set.
 * the values of the pointers corresponding
 * to the keys also appear next to their respective
 * keys, in hexadecimal notation.
 */
void btree_print(btnode *root) {

  btnode *n = NULL;
  int i = 0;
  int rank = 0;
  int new_rank = 0;

  if (root == NULL) {
    printf("Empty tree.\n");
    return;
  }
  queue = NULL;
  btree_enqueue(root);
  while (queue != NULL) {
    n = btree_dequeue();
    if (n->parent != NULL && n == n->parent->pointers[0]) {
      new_rank = btree_path_to_root(root, n);
      if (new_rank != rank) {
        rank = new_rank;
        printf("\n");
      }
    }
    if (verbose_output)
      printf("(%lx)", (unsigned long)n);
    for (i = 0; i < n->num_keys; i++) {
      if (verbose_output)
        printf("%lx ", (unsigned long)n->pointers[i]);
      printf("%d ", n->keys[i]);
    }
    if (!n->is_leaf)
      for (i = 0; i <= n->num_keys; i++)
        btree_enqueue(n->pointers[i]);
    if (verbose_output) {
      if (n->is_leaf)
        printf("%lx ", (unsigned long)n->pointers[order - 1]);
      else
        printf("%lx ", (unsigned long)n->pointers[n->num_keys]);
    }
    printf("| ");
  }
  printf("\n");
}

/* Finds the record under a given key and prints an
 * appropriate message to stdout.
 */
void btree_find_and_print(btnode *root, int key, bool verbose) {
  record *r = btree_find(root, key, verbose);
  if (r == NULL)
    printf("Record not found under key %d.\n", key);
  else
    printf("Record at %lx -- key %d, value %d.\n", (unsigned long)r, key,
           r->value);
}

/* Finds and prints the keys, pointers, and values within a range
 * of keys between key_start and key_end, including both bounds.
 */
void btree_find_and_print_range(btnode *root, int key_start, int key_end,
                                bool verbose) {
  int i;
  int array_size = key_end - key_start + 1;
  int returned_keys[array_size];
  void *returned_pointers[array_size];
  int num_found = btree_find_range(root, key_start, key_end, verbose,
                                   returned_keys, returned_pointers);
  if (!num_found)
    printf("None found.\n");
  else {
    for (i = 0; i < num_found; i++)
      printf("Key: %d   Location: %lx  Value: %d\n", returned_keys[i],
             (unsigned long)returned_pointers[i],
             ((record *)returned_pointers[i])->value);
  }
}