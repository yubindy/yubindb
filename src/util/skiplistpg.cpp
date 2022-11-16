#include "skiplistpg.h"
#include <memory>
#include "src/util/key.h"
namespace yubindb {
void Skiplist::Insert(SkiplistKey skiplistkv) {
  std::unique_ptr<node> p = std::make_unique<node>();
  skiplist_init_node(&p->snode);
  size_t nodesize(sizeof(node));
  skiplistkv.Key(p->key);
  skiplistkv.Val(p->val);
  nodesize+=p->val.size()+p->key.size();
  skiplist_insert(&table, &p->snode);
  size +=nodesize;
  nodes.emplace_back(std::move(p));
}
bool Skiplist::GreaterEqual(SkiplistKey& a, SkiplistKey& b) {
  node p(a);
  skiplist_node* cursor = skiplist_find_greater_or_equal(&table, &p.snode);
  if (!cursor) {
    return false;
  }
  return true;
}
skiplist_node* Skiplist::Seek(const InternalKey& key) {
  node p(key);
  skiplist_node* t = skiplist_find_greater_or_equal(&table, &p.snode);
  node* pp = _get_entry(t, node, snode);
  int r = key.ExtractUserKey().compare(pp->key.ExtractUserKey());
  if (r != 0) {
    t = nullptr;
  }
  return t;
}
bool Skiplist::KeyIsAfterNode(SkiplistKey& key, node* n) const {
  InternalKey p;
  key.Key(p);
  return (n != nullptr) && (cmp(n->key, p) < 0);
}
State Skiplist::Flushlevel0(FileMate& meta){
  node* beg=SeekToFirst();
  meta.smallest.DecodeFrom(beg->key.getview());
  node* end=SeekToFirst();
  meta.largest.DecodeFrom(end->key.getview());
  while(Valid(beg)){
    
  }
}
}  // namespace yubindb