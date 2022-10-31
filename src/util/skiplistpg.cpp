#include "skiplistpg.h"
namespace yubindb {
void Skiplist::Insert(SkiplistKey key_) {
  node* p = (node*)arena->AllocAligned(sizeof(node));
  p->key = key_;
  skiplist_insert(&table, &p->snode);
}
bool Skiplist::KeyIsAfterNode(SkiplistKey& key, node* n) const {
  return (n != nullptr) && (cmp(n->key.getview(), key.getview()) < 0);
}
}  // namespace yubindb