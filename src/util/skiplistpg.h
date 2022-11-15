#ifndef YUBINDB_SKIPLISTPG_H_
#define YUBINDB_SKIPLISTPG_H_
#include <functional>
#include <memory.h>
#include <memory>
#include <string_view>

#include "arena.h"
#include "key.h"
#include "skiplist.h"
namespace yubindb {
struct node {
 public:
  node() = default;
  node(const SkiplistKey& key_) {key_.Key(key);key_.Val(val); }
  node(const InternalKey& key_) : key(key_) {}
  ~node() = default;

  friend class Skiplist;
  friend int my_cmp(skiplist_node* a, skiplist_node* b, void* aux);
  // Metadata for skiplist node.
  skiplist_node snode;
  // My data here: {int, int} pair.
  
  InternalKey key;
  std::string val; //value size (varint32) + value std::string
};

// Define a comparison function for `my_node`.
inline int my_cmp(skiplist_node* a, skiplist_node* b, void* aux) {
  // Get `my_node` from skiplist node `a` and `b`.
  node *aa, *bb;
  aa = _get_entry(a, node, snode);
  bb = _get_entry(b, node, snode);

  return cmp(aa->key, bb->key);
}
class Skiplist {  // skiplist package
 public:
  explicit Skiplist(std::shared_ptr<Arena>& arena_) : arena(arena_) {
    skiplist_init(&table, my_cmp);
  }

  Skiplist(const Skiplist&) = delete;
  Skiplist& operator=(const Skiplist&) = delete;
  void Insert(SkiplistKey skiplistkv) ; 
  bool Equal(SkiplistKey& a, SkiplistKey& b) const {
    InternalKey a1,b1;
    a.Key(a1);
    b.Key(b1);
    return (cmp(a1,b1) == 0);
  }
  skiplist_node* Seek(const InternalKey& key);
  skiplist_node* SeekToFirst();
  skiplist_node* SeekToLast();
  bool GreaterEqual(SkiplistKey& a, SkiplistKey& b);
  bool KeyIsAfterNode(SkiplistKey& key, node* n) const;

 private:
  skiplist_raw table;
  std::shared_ptr<Arena> arena;  // for new and delete
  std::vector<std::unique_ptr<node>> nodes;
};
}  // namespace yubindb
#endif