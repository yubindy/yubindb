#include "skiplistpg.h"
namespace yubindb {
void Skiplist::Insert(SkiplistKey skiplistkv)  {
  node* p = (node*)arena->AllocAligned(sizeof(node));
  skiplist_init_node(&p->snode);
  p->key = skiplistkv.Key();
  p->val = skiplistkv.Val();
  skiplist_insert(&table, &p->snode);
}
bool Skiplist::GreaterEqual(SkiplistKey& a, SkiplistKey& b) {
  node p(a);
  skiplist_node* cursor = skiplist_find_greater_or_equal(&table, &p.snode);
  if (!cursor) {
    return false;
  }
  return true;
}
skiplist_node* Skiplist::Seek(const InternalKey& key)  {
  node p(key);
  skiplist_node* t = skiplist_find_greater_or_equal(&table, &p.snode);
  node* pp = _get_entry(t, node, snode);
  int r =
      key.ExtractUserKey().compare(pp->key.ExtractUserKey());
  if (r != 0) {
    t = nullptr;
  }
  return t;
}
skiplist_node* Skiplist::SeekToFirst() { return skiplist_begin(&table); }
skiplist_node* Skiplist::SeekToLast() { return skiplist_end(&table); }
bool Skiplist::KeyIsAfterNode(SkiplistKey& key, node* n) const {
  return (n != nullptr) && (cmp(n->key, key.Key()) < 0);
}
}  // namespace yubindb