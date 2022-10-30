#ifndef YUBINDB_SKIPLISTPG_H_
#define YUBINDB_SKIPLISTPG_H_
#include <functional>
#include <memory.h>
#include <string_view>

#include "arena.h"
#include "key.h"
#include "skiplist.h"
namespace yubindb {
typedef struct my_node {
  // Metadata for skiplist node.
  skiplist_node snode;
  // My data here: {int, int} pair.
  SkiplistKey key;
} node;

// Define a comparison function for `my_node`.
static int my_cmp(skiplist_node* a, skiplist_node* b, void* aux) {
  // Get `my_node` from skiplist node `a` and `b`.
  node *aa, *bb;
  aa = _get_entry(a, node, snode);
  bb = _get_entry(b, node, snode);

  return cmp(aa->key.getview(), bb->key.getview());
}
class Skiplist {  // skiplist package
 public:
  explicit Skiplist() { skiplist_init(&table, my_cmp); }

  Skiplist(const Skiplist&) = delete;
  Skiplist& operator=(const Skiplist&) = delete;
  void Insert(SkiplistKey key_);
  bool Equal(SkiplistKey& a, SkiplistKey& b) const {
    return (cmp(a.getview(), b.getview()) == 0);
  }

  bool KeyIsAfterNode(SkiplistKey& key, node* n) const;

  node* FindGreaterOrEqual(SkiplistKey& key, skiplist_node** prev) const;

  node* FindLessThan(SkiplistKey& key) const;

  node* FindLast() const;

 private:
  skiplist_raw table;
  std::unique_ptr<Arena> arena;
};
}  // namespace yubindb
#endif