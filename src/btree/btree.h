#ifndef __BTREE_H__
#define __BTREE_H__

/*
 *
 *  bpt:  B+ Tree Implementation
 *  Copyright (C) 2010-2016  Amittai Aviram  http://www.amittai.com
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Default order is 4.
#define DEFAULT_ORDER 4

// Minimum order is necessarily 3.  We set the maximum
// order arbitrarily.  You may change the maximum order.
#define MIN_ORDER 3
#define MAX_ORDER 20

/* Type representing the record
 * to which a given key refers.
 * In a real B+ tree system, the
 * record would hold data (in a database)
 * or a file (in an operating system)
 * or some other information.
 * Users can rewrite this part of the code
 * to change the type and content
 * of the value field.
 */
typedef struct record { int value; } record;

/* Type representing a btnode in the B+ tree.
 * This type is general enough to serve for both
 * the leaf and the internal btnode.
 * The heart of the btnode is the array
 * of keys and the array of corresponding
 * pointers.  The relation between keys
 * and pointers differs between leaves and
 * internal nodes.  In a leaf, the index
 * of each key equals the index of its corresponding
 * pointer, with a maximum of order - 1 key-pointer
 * pairs.  The last pointer points to the
 * leaf to the right (or NULL in the case
 * of the rightmost leaf).
 * In an internal btnode, the first pointer
 * refers to lower nodes with keys less than
 * the smallest key in the keys array.  Then,
 * with indices i starting at 0, the pointer
 * at i + 1 points to the subtree with keys
 * greater than or equal to the key in this
 * btnode at index i.
 * The num_keys field is used to keep
 * track of the number of valid keys.
 * In an internal btnode, the number of valid
 * pointers is always num_keys + 1.
 * In a leaf, the number of valid pointers
 * to data is always num_keys.  The
 * last leaf pointer points to the next leaf.
 */
typedef struct btnode {
  void **pointers;
  void **keys;
  struct btnode *parent;
  bool is_leaf;
  int num_keys;
  struct btnode *next; // Used for queue.
} btnode;

// GLOBALS.

/* The order determines the maximum and minimum
 * number of entries (keys and pointers) in any
 * btnode.  Every btnode has at most order - 1 keys and
 * at least (roughly speaking) half that number.
 * Every leaf has as many pointers to data as keys,
 * and every internal btnode has one more pointer
 * to a subtree than the number of keys.
 * This global variable is initialized to the
 * default value.
 */
extern int order;

// FUNCTION PROTOTYPES.

// Output and utility.
void btree_enqueue(btnode *new_node);
btnode *btree_dequeue(void);
int btree_height(btnode *root);
int btree_path_to_root(btnode *root, btnode *child);
void btree_print_leaves(btnode *root);
void btree_print(btnode *root);
void btree_find_and_print(btnode *root, int key, bool verbose);
void btree_find_and_print_range(btnode *root, int range1, int range2,
                                bool verbose);
int btree_find_range(btnode *root, int key_start, int key_end, bool verbose,
                     int returned_keys[], void *returned_pointers[]);
btnode *btree_find_leaf(btnode *root, int key, bool verbose);
record *btree_find(btnode *root, int key, bool verbose);
int btree_cut(int length);

// Insertion.

record *btree_make_record(int value);
btnode *btree_make_node(void);
btnode *btree_make_leaf(void);
int btree_get_left_index(btnode *parent, btnode *left);
btnode *btree_insert_into_leaf(btnode *leaf, int key, record *pointer);
btnode *btree_insert_into_leaf_after_splitting(btnode *root, btnode *leaf,
                                               int key, record *pointer);
btnode *btree_insert_into_node(btnode *root, btnode *parent, int left_index,
                               int key, btnode *right);
btnode *btree_insert_into_node_after_splitting(btnode *root, btnode *parent,
                                               int left_index, int key,
                                               btnode *right);
btnode *btree_insert_into_parent(btnode *root, btnode *left, int key,
                                 btnode *right);
btnode *btree_insert_into_new_root(btnode *left, int key, btnode *right);
btnode *btree_new(int key, record *pointer);
btnode *btree_insert(btnode *root, int key, int value);

// Deletion.

int btree_get_neighbor_index(btnode *n);
btnode *btree_adjust_root(btnode *root);
btnode *btree_coalesce_nodes(btnode *root, btnode *n, btnode *neighbor,
                             int neighbor_index, int k_prime);
btnode *btree_redistribute_nodes(btnode *root, btnode *n, btnode *neighbor,
                                 int neighbor_index, int k_prime_index,
                                 int k_prime);
btnode *btree_delete_entry(btnode *root, btnode *n, int key, void *pointer);
btnode *btree_delete(btnode *root, int key);

#endif