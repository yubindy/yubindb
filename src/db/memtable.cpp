#include "memtable.h"
namespace yubindb {
Memtable::Memtable()
    : arena(std::make_shared<Arena>()),
      table(std::make_unique<Skiplist>(arena)) {}
void Memtable::FindShortestSeparator(std::string* start,
                                     std::string_view limit) {}
void Memtable::FindShortSuccessor(std::string* key) {}
size_t Memtable::ApproximateMemoryUsage() {}
// Iterator* NewIterator(); //TODO?
void Memtable::Add(SequenceNum seq, Valuetype type, std::string_view key,
                   std::string_view value) {
  size_t key_size = key.size();
  size_t val_size = value.size();
  size_t internal_size = key_size + 8;
  const size_t encond_len =
      internal_size + internal_size + VarintLength(val_size) + val_size;
  char* buf = arena->Alloc(encond_len);
  std::memcpy(buf, (char*)(&internal_size), sizeof(internal_size));
  char* p = buf + sizeof(internal_size);
  std::memcpy(p, key.data(), key_size);
  p += key_size;
  EncodeFixed64(p, (seq << 8 | type));
  p += 8;
  p = EncodeVarint32(p, val_size);
  std::memcpy(p, value.data(), val_size);
  table->Insert(SkiplistKey(buf, internal_size));
}
bool Memtable::Get(const Lookey& key, std::string* value, State* s) {
  SkiplistKey skipkey(key.skiplist_key().data(), key.getinterlen());
  skiplist_node* t = table->Seek(skipkey);
  if (t != nullptr) {
    node* found = _get_entry(t, node, snode);
    if ((found->key.Getag() & 0xf) == kTypeValue) {
      found->key.getString(value);
      return true;
    } else if ((found->key.Getag() & 0xf) == kTypeDeletion) {
      return false;
    }
  } else {
    return false;
  }
}

}  // namespace yubindb