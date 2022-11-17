#include "memtable.h"

#include <crc32c/crc32c.h>

#include <cassert>
#include <memory>
#include <string_view>

#include "src/db/block.h"
#include "src/util/common.h"
#include "src/util/options.h"
#include "src/util/skiplistpg.h"
namespace yubindb {
Memtable::Memtable()
    : arena(std::make_shared<Arena>()),
      table(std::make_unique<Skiplist>(arena)) {}
void Memtable::FindShortestSeparator(std::string* start,
                                     std::string_view limit) {}
void Memtable::FindShortSuccessor(std::string* key) {}
uint32_t Memtable::ApproximateMemoryUsage() { return table->Getsize(); }
// Iterator* NewIterator(); //TODO?
void Memtable::Add(SequenceNum seq, Valuetype type, std::string_view key,
                   std::string_view value) {
  uint32_t key_size = key.size();
  uint32_t val_size = value.size();
  uint32_t internal_size = key_size + 8;
  const uint32_t encond_len = VarintLength(internal_size) + internal_size +
                              VarintLength(val_size) + val_size;
  char* buf = arena->AllocateAligned(encond_len);
  char* p = EncodeVarint32(buf, internal_size);
  std::memcpy(p, key.data(), key_size);
  p += key_size;
  EncodeFixed64(p, (seq << 8 | type));
  p += 8;
  char* pt = EncodeVarint32(p, val_size);
  std::memcpy(pt, value.data(), val_size);
  table->Insert(SkiplistKey(buf));
}
bool Memtable::Get(const Lookey& key, std::string* value, State* s) {
  InternalKey keys(key.internal_key());
  skiplist_node* t = table->Seek(keys);
  if (t != nullptr) {
    node* found = _get_entry(t, node, snode);
    if ((found->key.Getag() & 0xf) == kTypeValue) {
      uint32_t val_size;
      getsize(found->val.data(), val_size);
      value->resize(val_size);
      memcpy(value->data(), found->val.c_str() + VarintLength(val_size),
             val_size);
      return true;
    } else if ((found->key.Getag() & 0xf) == kTypeDeletion) {
      return false;
    }
  } else {
    return false;
  }
}
State Memtable::Flushlevel0fromskip(FileMate& meta,
                                    std::unique_ptr<Tablebuilder>& builder) {
  node* p = table->SeekToFirst();
  meta.smallest = p->key;
  for (; table->Valid(p); p = table->Next(p)) {
    builder->Add(p->key, p->val);
  }
  while (!table->Valid(p)) {
    p = table->Prev(p);
  }
  meta.largest = p->key;
}
void Tablebuilder::Add(const InternalKey& key, const std::string& val) {
  assert(!closed);
  std::string_view keyview = key.getview();
  if (pending_index_entry) {
    assert(data_block.empty());
    std::string handle_entry;
    pending_handle.EncodeTo(&handle_entry);
    index_block.Add(keyview, std::string_view(handle_entry));
    pending_index_entry = false;
  }
  if (filter_block != nullptr) {
    filter_block->AddKey(keyview);
  }
  last_key.assign(keyview.data(), keyview.size());
  num_entries++;
  data_block.Add(keyview, val);
  if (data_block.CurrentSizeEstimate() >= options.block_size) {
    Flush();
  }
}
void Tablebuilder::Flush() {
  assert(!closed);
  if (data_block.empty()) return;
  assert(!pending_index_entry);
  WriteBlock(&data_block, &pending_handle);
  pending_index_entry = false;
  file->Flush();
  if (filter_block != nullptr) {
    filter_block->StartBlock(offset);
  }
}
void Tablebuilder::WriteBlock(Blockbuilder* block, BlockHandle* handle) {
  std::string_view raw = block->Finish();
  CompressionType type = options.compression;

  std::string_view block_;
  switch (type) {
    case kNoCompression:
      block_ = raw;
      break;
    case kSnappyCompression: {
      std::string* compress = &compressed_output;
      if (Snappy_Compress(raw.data(), raw.size(), compress) &&
          compress->size() < raw.size() - (raw.size() / 8u)) {  //压缩比太小
        block_ = *compress;
      } else {
        block_ = raw;
        type = kNoCompression;
      }
      compressed_output.clear();
      break;
    }
  }
  WriteRawBlock(block_, type, handle);
  block->Reset();
}
void Tablebuilder::WriteRawBlock(const std::string_view& block_contents,
                                 CompressionType type, BlockHandle* handle) {
  //    block_contents: char[n]
  //    type: uint8
  //    crc: uint32
  handle->set_offset(offset);
  handle->set_size(block_contents.size());
  state = file->Append(block_contents);
  if (state.ok()) {
    char ptr[kBlockBackSize];
    ptr[0] = type;
    uint32_t crc = crc32c::Crc32c(block_contents.data(), block_contents.size());
    crc = crc32c::Extend(crc, (uint8_t*)ptr, 1);
    EncodeFixed32(ptr, crc);
    state = file->Append(ptr);
    if (state.ok()) {
      offset += block_contents.size() + kBlockBackSize;
    }
  }
}
}  // namespace yubindb