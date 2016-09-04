/*
 *
 *  bpt:  B+ Tree Implementation
 *  Copyright (C) 20
  0-2016  Ami

  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.

 *  3. Neither the name of the copyright holder nor the names of its
 *  contributors may be used to endorse or promote products derived from this
 *  software without specific prior written permission.

 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.

 *  Author:  Amittai Aviram
 *    http://www.amittai.com
 *    amittai.aviram@gmail.edu or afa13@columbia.edu
 *  Original Date:  26 June 2010
 *  Last modified: 17 June 2016
 *
 *  This implementation demonstrates the B+ tree data structure
 *  for educational purposes, includin insertion, deletion, search, and display
 *  of the search path, the leaves, or the whole tree.
 *
 */
#include "btree.h"

int order = DEFAULT_ORDER;

/* Finds keys and their pointers, if present, in the range specified
 * by key_start and key_end, inclusive.  Places these in the arrays
 * returned_keys and returned_pointers, and returns the number of
 * entries found.
 */
int btree_find_range(btnode *root, int key_start, int key_end, bool verbose,
                     int returned_keys[], void *returned_pointers[]) {
  int i, num_found;
  num_found = 0;
  btnode *n = btree_find_leaf(root, key_start, verbose);
  if (n == NULL)
    return 0;
  for (i = 0; i < n->num_keys && n->keys[i] < key_start; i++)
    ;
  if (i == n->num_keys)
    return 0;
  while (n != NULL) {
    for (; i < n->num_keys && n->keys[i] <= key_end; i++) {
      returned_keys[num_found] = n->keys[i];
      returned_pointers[num_found] = n->pointers[i];
      num_found++;
    }
    n = n->pointers[order - 1];
    i = 0;
  }
  return num_found;
}

/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
btnode *btree_find_leaf(btnode *root, int key, bool verbose) {
  int i = 0;
  btnode *c = root;
  if (c == NULL) {
    if (verbose)
      printf("Empty tree.\n");
    return c;
  }
  while (!c->is_leaf) {
    if (verbose) {
      printf("[");
      for (i = 0; i < c->num_keys - 1; i++)
        printf("%d ", c->keys[i]);
      printf("%d] ", c->keys[i]);
    }
    i = 0;
    while (i < c->num_keys) {
      if (key >= c->keys[i])
        i++;
      else
        break;
    }
    if (verbose)
      printf("%d ->\n", i);
    c = (btnode *)c->pointers[i];
  }
  if (verbose) {
    printf("Leaf [");
    for (i = 0; i < c->num_keys - 1; i++)
      printf("%d ", c->keys[i]);
    printf("%d] ->\n", c->keys[i]);
  }
  return c;
}

/* Finds and returns the record to which
 * a key refers.
 */
record *btree_find(btnode *root, int key, bool verbose) {
  int i = 0;
  btnode *c = btree_find_leaf(root, key, verbose);
  if (c == NULL)
    return NULL;
  for (i = 0; i < c->num_keys; i++)
    if (c->keys[i] == key)
      break;
  if (i == c->num_keys)
    return NULL;
  else
    return (record *)c->pointers[i];
}

/* Finds the appropriate place to
 * split a btnode that is too big into two.
 */
int btree_cut(int length) {
  if (length % 2 == 0)
    return length / 2;
  else
    return length / 2 + 1;
}

// INSERTION

/* Creates a new record to hold the value
 * to which a key refers.
 */
record *btree_make_record(int value) {
  record *new_record = (record *)malloc(sizeof(record));
  if (new_record == NULL) {
    perror("Record creation.");
    exit(EXIT_FAILURE);
  } else {
    new_record->value = value;
  }
  return new_record;
}

/* Creates a new general btnode, which can be adapted
 * to serve as either a leaf or an internal btnode.
 */
btnode *btree_make_node(void) {
  btnode *new_node;
  new_node = malloc(sizeof(btnode));
  if (new_node == NULL) {
    perror("btnode creation.");
    exit(EXIT_FAILURE);
  }
  new_node->keys = malloc((order - 1) * sizeof(int));
  if (new_node->keys == NULL) {
    perror("New btnode keys array.");
    exit(EXIT_FAILURE);
  }
  new_node->pointers = malloc(order * sizeof(void *));
  if (new_node->pointers == NULL) {
    perror("New btnode pointers array.");
    exit(EXIT_FAILURE);
  }
  new_node->is_leaf = false;
  new_node->num_keys = 0;
  new_node->parent = NULL;
  new_node->next = NULL;
  return new_node;
}

/* Creates a new leaf by creating a btnode
 * and then adapting it appropriately.
 */
btnode *btree_make_leaf(void) {
  btnode *leaf = btree_make_node();
  leaf->is_leaf = true;
  return leaf;
}

/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to
 * the btnode to the left of the key to be inserted.
 */
int btree_get_left_index(btnode *parent, btnode *left) {

  int left_index = 0;
  while (left_index <= parent->num_keys && parent->pointers[left_index] != left)
    left_index++;
  return left_index;
}

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */
btnode *btree_insert_into_leaf(btnode *leaf, int key, record *pointer) {

  int i, insertion_point;

  insertion_point = 0;
  while (insertion_point < leaf->num_keys && leaf->keys[insertion_point] < key)
    insertion_point++;

  for (i = leaf->num_keys; i > insertion_point; i--) {
    leaf->keys[i] = leaf->keys[i - 1];
    leaf->pointers[i] = leaf->pointers[i - 1];
  }
  leaf->keys[insertion_point] = key;
  leaf->pointers[insertion_point] = pointer;
  leaf->num_keys++;
  return leaf;
}

/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
btnode *btree_insert_into_leaf_after_splitting(btnode *root, btnode *leaf,
                                               int key, record *pointer) {

  btnode *new_leaf;
  int *temp_keys;
  void **temp_pointers;
  int insertion_index, split, new_key, i, j;

  new_leaf = btree_make_leaf();

  temp_keys = malloc(order * sizeof(int));
  if (temp_keys == NULL) {
    perror("Temporary keys array.");
    exit(EXIT_FAILURE);
  }

  temp_pointers = malloc(order * sizeof(void *));
  if (temp_pointers == NULL) {
    perror("Temporary pointers array.");
    exit(EXIT_FAILURE);
  }

  insertion_index = 0;
  while (insertion_index < order - 1 && leaf->keys[insertion_index] < key)
    insertion_index++;

  for (i = 0, j = 0; i < leaf->num_keys; i++, j++) {
    if (j == insertion_index)
      j++;
    temp_keys[j] = leaf->keys[i];
    temp_pointers[j] = leaf->pointers[i];
  }

  temp_keys[insertion_index] = key;
  temp_pointers[insertion_index] = pointer;

  leaf->num_keys = 0;

  split = btree_cut(order - 1);

  for (i = 0; i < split; i++) {
    leaf->pointers[i] = temp_pointers[i];
    leaf->keys[i] = temp_keys[i];
    leaf->num_keys++;
  }

  for (i = split, j = 0; i < order; i++, j++) {
    new_leaf->pointers[j] = temp_pointers[i];
    new_leaf->keys[j] = temp_keys[i];
    new_leaf->num_keys++;
  }

  free(temp_pointers);
  free(temp_keys);

  new_leaf->pointers[order - 1] = leaf->pointers[order - 1];
  leaf->pointers[order - 1] = new_leaf;

  for (i = leaf->num_keys; i < order - 1; i++)
    leaf->pointers[i] = NULL;
  for (i = new_leaf->num_keys; i < order - 1; i++)
    new_leaf->pointers[i] = NULL;

  new_leaf->parent = leaf->parent;
  new_key = new_leaf->keys[0];

  return btree_insert_into_parent(root, leaf, new_key, new_leaf);
}

/* Inserts a new key and pointer to a btnode
 * into a btnode into which these can fit
 * without violating the B+ tree properties.
 */
btnode *btree_insert_into_node(btnode *root, btnode *n, int left_index, int key,
                               btnode *right) {
  int i;

  for (i = n->num_keys; i > left_index; i--) {
    n->pointers[i + 1] = n->pointers[i];
    n->keys[i] = n->keys[i - 1];
  }
  n->pointers[left_index + 1] = right;
  n->keys[left_index] = key;
  n->num_keys++;
  return root;
}

/* Inserts a new key and pointer to a btnode
 * into a btnode, causing the btnode's size to exceed
 * the order, and causing the btnode to split into two.
 */
btnode *btree_insert_into_node_after_splitting(btnode *root, btnode *old_node,
                                               int left_index, int key,
                                               btnode *right) {

  int i, j, split, k_prime;
  btnode *new_node, *child;
  int *temp_keys;
  btnode **temp_pointers;

  /* First create a temporary set of keys and pointers
   * to hold everything in order, including
   * the new key and pointer, inserted in their
   * correct places.
   * Then create a new btnode and copy half of the
   * keys and pointers to the old btnode and
   * the other half to the new.
   */

  temp_pointers = malloc((order + 1) * sizeof(btnode *));
  if (temp_pointers == NULL) {
    perror("Temporary pointers array for splitting nodes.");
    exit(EXIT_FAILURE);
  }
  temp_keys = malloc(order * sizeof(int));
  if (temp_keys == NULL) {
    perror("Temporary keys array for splitting nodes.");
    exit(EXIT_FAILURE);
  }

  for (i = 0, j = 0; i < old_node->num_keys + 1; i++, j++) {
    if (j == left_index + 1)
      j++;
    temp_pointers[j] = old_node->pointers[i];
  }

  for (i = 0, j = 0; i < old_node->num_keys; i++, j++) {
    if (j == left_index)
      j++;
    temp_keys[j] = old_node->keys[i];
  }

  temp_pointers[left_index + 1] = right;
  temp_keys[left_index] = key;

  /* Create the new btnode and copy
   * half the keys and pointers to the
   * old and half to the new.
   */
  split = btree_cut(order);
  new_node = btree_make_node();
  old_node->num_keys = 0;
  for (i = 0; i < split - 1; i++) {
    old_node->pointers[i] = temp_pointers[i];
    old_node->keys[i] = temp_keys[i];
    old_node->num_keys++;
  }
  old_node->pointers[i] = temp_pointers[i];
  k_prime = temp_keys[split - 1];
  for (++i, j = 0; i < order; i++, j++) {
    new_node->pointers[j] = temp_pointers[i];
    new_node->keys[j] = temp_keys[i];
    new_node->num_keys++;
  }
  new_node->pointers[j] = temp_pointers[i];
  free(temp_pointers);
  free(temp_keys);
  new_node->parent = old_node->parent;
  for (i = 0; i <= new_node->num_keys; i++) {
    child = new_node->pointers[i];
    child->parent = new_node;
  }

  /* Insert a new key into the parent of the two
   * nodes resulting from the split, with
   * the old btnode to the left and the new to the right.
   */

  return btree_insert_into_parent(root, old_node, k_prime, new_node);
}

/* Inserts a new btnode (leaf or internal btnode) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
btnode *btree_insert_into_parent(btnode *root, btnode *left, int key,
                                 btnode *right) {

  int left_index;
  btnode *parent;

  parent = left->parent;

  /* Case: new root. */

  if (parent == NULL)
    return btree_insert_into_new_root(left, key, right);

  /* Case: leaf or btnode. (Remainder of
   * function body.)
   */

  /* Find the parent's pointer to the left
   * btnode.
   */

  left_index = btree_get_left_index(parent, left);

  /* Simple case: the new key fits into the btnode.
   */

  if (parent->num_keys < order - 1)
    return btree_insert_into_node(root, parent, left_index, key, right);

  /* Harder case:  split a btnode in order
   * to preserve the B+ tree properties.
   */

  return btree_insert_into_node_after_splitting(root, parent, left_index, key,
                                                right);
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
btnode *btree_insert_into_new_root(btnode *left, int key, btnode *right) {

  btnode *root = btree_make_node();
  root->keys[0] = key;
  root->pointers[0] = left;
  root->pointers[1] = right;
  root->num_keys++;
  root->parent = NULL;
  left->parent = root;
  right->parent = root;
  return root;
}

/* First insertion:
 * start a new tree.
 */
btnode *btree_new(int key, record *pointer) {

  btnode *root = btree_make_leaf();
  root->keys[0] = key;
  root->pointers[0] = pointer;
  root->pointers[order - 1] = NULL;
  root->parent = NULL;
  root->num_keys++;
  return root;
}

/* Master insertion function.
 * Inserts a key and an associated value into
 * the B+ tree, causing the tree to be adjusted
 * however necessary to maintain the B+ tree
 * properties.
 */
btnode *btree_insert(btnode *root, int key, int value) {

  record *pointer;
  btnode *leaf;

  /* The current implementation ignores
   * duplicates.
   */
  if (btree_find(root, key, false) != NULL)
    return root;

  /* Create a new record for the
   * value.
   */
  pointer = btree_make_record(value);

  /* Case: the tree does not exist yet.
   * Start a new tree.
   */

  if (root == NULL)
    return btree_new(key, pointer);

  /* Case: the tree already exists.
   * (Rest of function body.)
   */

  leaf = btree_find_leaf(root, key, false);

  /* Case: leaf has room for key and pointer.
   */
  if (leaf->num_keys < order - 1) {
    leaf = btree_insert_into_leaf(leaf, key, pointer);
    return root;
  }

  /* Case:  leaf must be split.
   */

  return btree_insert_into_leaf_after_splitting(root, leaf, key, pointer);
}

// DELETION.

/* Utility function for deletion.  Retrieves
 * the index of a btnode's nearest neighbor (sibling)
 * to the left if one exists.  If not (the btnode
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int btree_get_neighbor_index(btnode *n) {

  int i;

  /* Return the index of the key to the left
   * of the pointer in the parent pointing
   * to n.
   * If n is the leftmost child, this means
   * return -1.
   */
  for (i = 0; i <= n->parent->num_keys; i++)
    if (n->parent->pointers[i] == n)
      return i - 1;

  // Error state.
  printf("Search for nonexistent pointer to btnode in parent.\n");
  printf("btnode:  %#lx\n", (unsigned long)n);
  exit(EXIT_FAILURE);
}

btnode *btree_remove_entry_from_node(btnode *n, int key, btnode *pointer) {

  int i, num_pointers;

  // Remove the key and shift other keys accordingly.
  i = 0;
  while (n->keys[i] != key)
    i++;
  for (++i; i < n->num_keys; i++)
    n->keys[i - 1] = n->keys[i];

  // Remove the pointer and shift other pointers accordingly.
  // First determine number of pointers.
  num_pointers = n->is_leaf ? n->num_keys : n->num_keys + 1;
  i = 0;
  while (n->pointers[i] != pointer)
    i++;
  for (++i; i < num_pointers; i++)
    n->pointers[i - 1] = n->pointers[i];

  // One key fewer.
  n->num_keys--;

  // Set the other pointers to NULL for tidiness.
  // A leaf uses the last pointer to point to the next leaf.
  if (n->is_leaf)
    for (i = n->num_keys; i < order - 1; i++)
      n->pointers[i] = NULL;
  else
    for (i = n->num_keys + 1; i < order; i++)
      n->pointers[i] = NULL;

  return n;
}

btnode *btree_adjust_root(btnode *root) {

  btnode *new_root;

  /* Case: nonempty root.
   * Key and pointer have already been deleted,
   * so nothing to be done.
   */

  if (root->num_keys > 0)
    return root;

  /* Case: empty root.
   */

  // If it has a child, promote
  // the first (only) child
  // as the new root.

  if (!root->is_leaf) {
    new_root = root->pointers[0];
    new_root->parent = NULL;
  }

  // If it is a leaf (has no children),
  // then the whole tree is empty.

  else
    new_root = NULL;

  free(root->keys);
  free(root->pointers);
  free(root);

  return new_root;
}

/* Coalesces a btnode that has become
 * too small after deletion
 * with a neighboring btnode that
 * can accept the additional entries
 * without exceeding the maximum.
 */
btnode *btree_coalesce_nodes(btnode *root, btnode *n, btnode *neighbor,
                             int neighbor_index, int k_prime) {

  int i, j, neighbor_insertion_index, n_end;
  btnode *tmp;

  /* Swap neighbor with btnode if btnode is on the
   * extreme left and neighbor is to its right.
   */

  if (neighbor_index == -1) {
    tmp = n;
    n = neighbor;
    neighbor = tmp;
  }

  /* Starting point in the neighbor for copying
   * keys and pointers from n.
   * Recall that n and neighbor have swapped places
   * in the special case of n being a leftmost child.
   */

  neighbor_insertion_index = neighbor->num_keys;

  /* Case:  nonleaf btnode.
   * Append k_prime and the following pointer.
   * Append all pointers and keys from the neighbor.
   */

  if (!n->is_leaf) {

    /* Append k_prime.
     */

    neighbor->keys[neighbor_insertion_index] = k_prime;
    neighbor->num_keys++;

    n_end = n->num_keys;

    for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
      neighbor->keys[i] = n->keys[j];
      neighbor->pointers[i] = n->pointers[j];
      neighbor->num_keys++;
      n->num_keys--;
    }

    /* The number of pointers is always
     * one more than the number of keys.
     */

    neighbor->pointers[i] = n->pointers[j];

    /* All children must now point up to the same parent.
     */

    for (i = 0; i < neighbor->num_keys + 1; i++) {
      tmp = (btnode *)neighbor->pointers[i];
      tmp->parent = neighbor;
    }
  }

  /* In a leaf, append the keys and pointers of
   * n to the neighbor.
   * Set the neighbor's last pointer to point to
   * what had been n's right neighbor.
   */

  else {
    for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
      neighbor->keys[i] = n->keys[j];
      neighbor->pointers[i] = n->pointers[j];
      neighbor->num_keys++;
    }
    neighbor->pointers[order - 1] = n->pointers[order - 1];
  }

  root = btree_delete_entry(root, n->parent, k_prime, n);
  free(n->keys);
  free(n->pointers);
  free(n);
  return root;
}

/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small btnode's entries without exceeding the
 * maximum
 */
btnode *btree_redistribute_nodes(btnode *root, btnode *n, btnode *neighbor,
                                 int neighbor_index, int k_prime_index,
                                 int k_prime) {

  int i;
  btnode *tmp;

  /* Case: n has a neighbor to the left.
   * Pull the neighbor's last key-pointer pair over
   * from the neighbor's right end to n's left end.
   */

  if (neighbor_index != -1) {
    if (!n->is_leaf)
      n->pointers[n->num_keys + 1] = n->pointers[n->num_keys];
    for (i = n->num_keys; i > 0; i--) {
      n->keys[i] = n->keys[i - 1];
      n->pointers[i] = n->pointers[i - 1];
    }
    if (!n->is_leaf) {
      n->pointers[0] = neighbor->pointers[neighbor->num_keys];
      tmp = (btnode *)n->pointers[0];
      tmp->parent = n;
      neighbor->pointers[neighbor->num_keys] = NULL;
      n->keys[0] = k_prime;
      n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
    } else {
      n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
      neighbor->pointers[neighbor->num_keys - 1] = NULL;
      n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
      n->parent->keys[k_prime_index] = n->keys[0];
    }
  }

  /* Case: n is the leftmost child.
   * Take a key-pointer pair from the neighbor to the right.
   * Move the neighbor's leftmost key-pointer pair
   * to n's rightmost position.
   */

  else {
    if (n->is_leaf) {
      n->keys[n->num_keys] = neighbor->keys[0];
      n->pointers[n->num_keys] = neighbor->pointers[0];
      n->parent->keys[k_prime_index] = neighbor->keys[1];
    } else {
      n->keys[n->num_keys] = k_prime;
      n->pointers[n->num_keys + 1] = neighbor->pointers[0];
      tmp = (btnode *)n->pointers[n->num_keys + 1];
      tmp->parent = n;
      n->parent->keys[k_prime_index] = neighbor->keys[0];
    }
    for (i = 0; i < neighbor->num_keys - 1; i++) {
      neighbor->keys[i] = neighbor->keys[i + 1];
      neighbor->pointers[i] = neighbor->pointers[i + 1];
    }
    if (!n->is_leaf)
      neighbor->pointers[i] = neighbor->pointers[i + 1];
  }

  /* n now has one more key and one more pointer;
   * the neighbor has one fewer of each.
   */

  n->num_keys++;
  neighbor->num_keys--;

  return root;
}

/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
btnode *btree_delete_entry(btnode *root, btnode *n, int key, void *pointer) {

  int min_keys;
  btnode *neighbor;
  int neighbor_index;
  int k_prime_index, k_prime;
  int capacity;

  // Remove key and pointer from btnode.

  n = btree_remove_entry_from_node(n, key, pointer);

  /* Case:  deletion from the root.
   */

  if (n == root)
    return btree_adjust_root(root);

  /* Case:  deletion from a btnode below the root.
   * (Rest of function body.)
   */

  /* Determine minimum allowable size of btnode,
   * to be preserved after deletion.
   */

  min_keys = n->is_leaf ? btree_cut(order - 1) : btree_cut(order) - 1;

  /* Case:  btnode stays at or above minimum.
   * (The simple case.)
   */

  if (n->num_keys >= min_keys)
    return root;

  /* Case:  btnode falls below minimum.
   * Either coalescence or redistribution
   * is needed.
   */

  /* Find the appropriate neighbor btnode with which
   * to coalesce.
   * Also find the key (k_prime) in the parent
   * between the pointer to btnode n and the pointer
   * to the neighbor.
   */

  neighbor_index = btree_get_neighbor_index(n);
  k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
  k_prime = n->parent->keys[k_prime_index];
  neighbor = neighbor_index == -1 ? n->parent->pointers[1]
                                  : n->parent->pointers[neighbor_index];

  capacity = n->is_leaf ? order : order - 1;

  /* Coalescence. */

  if (neighbor->num_keys + n->num_keys < capacity)
    return btree_coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);

  /* Redistribution. */

  else
    return btree_redistribute_nodes(root, n, neighbor, neighbor_index,
                                    k_prime_index, k_prime);
}

/* Master deletion function.
 */
btnode *btree_delete(btnode *root, int key) {

  btnode *key_leaf;
  record *key_record;

  key_record = btree_find(root, key, false);
  key_leaf = btree_find_leaf(root, key, false);
  if (key_record != NULL && key_leaf != NULL) {
    root = btree_delete_entry(root, key_leaf, key, key_record);
    free(key_record);
  }
  return root;
}

void btree_destroy_tree_nodes(btnode *root) {
  int i;
  if (root->is_leaf)
    for (i = 0; i < root->num_keys; i++)
      free(root->pointers[i]);
  else
    for (i = 0; i < root->num_keys + 1; i++)
      btree_destroy_tree_nodes(root->pointers[i]);
  free(root->pointers);
  free(root->keys);
  free(root);
}

btnode *btree_destroy_tree(btnode *root) {
  btree_destroy_tree_nodes(root);
  return NULL;
}
