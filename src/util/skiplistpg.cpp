#include "skiplistpg.h"
namespace yubindb {
void Skiplist::Insert(SkiplistKey key_) {
  node* p = (node*)arena->AllocAligned(sizeof(node));
  p->key = key_;
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
skiplist_node* Skiplist::Seek(const SkiplistKey& key) {
  node p(key);
  skiplist_node* t = skiplist_find_greater_or_equal(&table, &p.snode);
  node* pp = _get_entry(t, node, snode);
  int r = strcmp(ExtractUserKey(key.getview()).data(),
                 ExtractUserKey(pp->key.getview().data()));
  if (r != 0) {
    t = nullptr;
  }
  return t;
}
skiplist_node* Skiplist::SeekToFirst() { return skiplist_begin(&table); }
skiplist_node* Skiplist::SeekToLast() { return skiplist_end(&table); }
bool Skiplist::KeyIsAfterNode(SkiplistKey& key, node* n) const {
  return (n != nullptr) && (cmp(n->key.getview(), key.getview()) < 0);
}
}  // namespace yubindb