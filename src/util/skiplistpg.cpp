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
node* Skiplist::FindGreaterOrEqual(SkiplistKey& key,
                                   skiplist_node** prev) const {
  skiplist_node* head = skiplist_begin(&table);
  int level = GetMaxHeight() - 1;
  while (true) {
    Node* next = x->Next(level);
    if (KeyIsAfterNode(key, next)) {
      // Keep searching in this list
      x = next;
    } else {
      if (prev != nullptr) prev[level] = x;
      if (level == 0) {
        return next;
      } else {
        // Switch to next list
        level--;
      }
    }
  }
}
node* Skiplist::FindLessThan(SkiplistKey& key) const {}

node* Skiplist::FindLast() const {}
}  // namespace yubindb