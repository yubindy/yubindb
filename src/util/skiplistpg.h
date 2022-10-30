#ifndef YUBINDB_SKIPLISTPG_H_
#define YUBINDB_SKIPLISTPG_H_
#include <functional>
#include <memory.h>
#include <string_view>

#include "arena.h"
#include "key.h"
#include "skiplist.h"
namespace yubindb {
struct my_node {
  // Metadata for skiplist node.
  skiplist_node snode;
  // My data here: {int, int} pair.
  InternalKey key;
  char* value;
};

// Define a comparison function for `my_node`.
static int my_cmp(skiplist_node* a, skiplist_node* b, void* aux) {
  // Get `my_node` from skiplist node `a` and `b`.
  struct my_node *aa, *bb;
  aa = _get_entry(a, struct my_node, snode);
  bb = _get_entry(b, struct my_node, snode);

  return cmp(aa->key.getview(), bb->key.getview());
}
class Skiplist {  // skiplist package
 public:
  explicit Skiplist() { skiplist_init(&table, my_cmp); }

  Skiplist(const Skiplist&) = delete;
  Skiplist& operator=(const Skiplist&) = delete;
  void Insert(std::string_view* key_);
  bool Equal(std::string_view a, std::string_view b) const {
    return (cmp(a, b) == 0);
  }

  bool KeyIsAfterNode(std::string_view key, my_node* n) const;

  my_node* FindGreaterOrEqual(std::string_view key, my_node** prev) const;

  my_node* FindLessThan(std::string_view key) const;

  my_node* FindLast() const;

 private:
  skiplist_raw table;
  std::unique_ptr<Arena> arena;
};
}  // namespace yubindb
#endif