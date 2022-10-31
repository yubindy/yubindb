#include <stdio.h>
#include <stdlib.h>

#include "skiplist.h"
struct my_node {
  // Metadata for skiplist node.
  skiplist_node snode;
  // My data here: {int, int} pair.
  int key;
  int value;
};

// Define a comparison function for `my_node`.
static int my_cmp(skiplist_node* a, skiplist_node* b, void* aux) {
  // Get `my_node` from skiplist node `a` and `b`.
  struct my_node *aa, *bb;
  aa = _get_entry(a, struct my_node, snode);
  bb = _get_entry(b, struct my_node, snode);

  // aa  < bb: return neg
  // aa == bb: return 0
  // aa  > bb: return pos
  if (aa->key < bb->key) return -1;
  if (aa->key > bb->key) return 1;
  return 0;
}

int main() {
  skiplist_raw slist;

  // Initialize skiplist.
  skiplist_init(&slist, my_cmp);

  //   << Insertion >>
  // Allocate & insert 3 KV pairs: {0, 0}, {1, 10}, {2, 20}.
  struct my_node* nodes[3];
  for (int i = 0; i < 3; ++i) {
    // Allocate memory.
    nodes[i] = (struct my_node*)malloc(sizeof(struct my_node));
    // Initialize node.
    skiplist_init_node(&nodes[i]->snode);
    // Assign key and value.
    nodes[i]->key = i;
    nodes[i]->value = i * 10;
    // Insert into skiplist.
    skiplist_insert(&slist, &nodes[i]->snode);
  }
  printf("size %d", skiplist_get_size(&slist));
  for (int i = 0; i < 3; ++i) {
    // Define a query.
    struct my_node query;
    query.key = i;
    // Find a skiplist node `cursor`.
    skiplist_node* cursor = skiplist_find(&slist, &query.snode);
    // If `cursor` is NULL, key doesn't exist.
    if (!cursor) continue;
    // Get `my_node` from `cursor`.
    // Note: found->snode == *cursor
    struct my_node* found = _get_entry(cursor, struct my_node, snode);
    printf("[point lookup] key: %d, value: %d\n", found->key, found->value);
    // Release `cursor` (== &found->snode).
    // Other thread cannot free `cursor` until `cursor` is released.
    skiplist_release_node(cursor);
  }
    for (int i = 0; i < 3; ++i) {
    // Define a query.
    struct my_node query;
    query.key = i;
    // Find a skiplist node `cursor`.
    skiplist_node* cursor = skiplist_find(&slist, &query.snode);
    // If `cursor` is NULL, key doesn't exist.
    if (!cursor) continue;
    // Get `my_node` from `cursor`.
    // Note: found->snode == *cursor
    struct my_node* found = _get_entry(cursor, struct my_node, snode);
    printf("[point lookup] key: %d, value: %d\n", found->key, found->value);
    // Release `cursor` (== &found->snode).
    // Other thread cannot free `cursor` until `cursor` is released.
    skiplist_release_node(cursor);
  }
  return 0;
}
