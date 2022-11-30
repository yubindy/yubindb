#include "memtable.h"

#include <crc32c/crc32c.h>
#include <snappy.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view> 

#include "src/db/block.h"
#include "src/util/common.h"
#include "src/util/options.h"
#include "src/util/skiplistpg.h"
namespace yubindb {
void Footer::EncodeTo(std::string* dst) const {
  metaindex_handle_.EncodeTo(dst);
  index_handle_.EncodeTo(dst);
  dst->resize(2 * BlockHandle::kMaxEncodedLength);
  PutFixed32(dst, static_cast<uint32_t>(kTableMagicNumber));
  PutFixed32(dst, static_cast<uint32_t>(kTableMagicNumber>>32));
  assert(dst->size() == kEncodedLength);
}
State Footer::DecodeFrom(std::string_view* input) {
  const char* magic_ptr = input->data() + kEncodedLength - 8;
  const uint32_t magic_lo = DecodeFixed32(magic_ptr);
  const uint32_t magic_hi = DecodeFixed32(magic_ptr + 4);
  const uint64_t magic = ((static_cast<uint64_t>(magic_hi) << 32) |
                          (static_cast<uint64_t>(magic_lo)));
  if (magic != kTableMagicNumber) {
    mlog->warn("not an sstable (bad magic number {0:x}",magic);
    mlog->warn("truemagic {0:x}",kTableMagicNumber);
    //mlog->info("{0:x}",std::stol(std::string(magic_ptr,8)));
    return State::Corruption();
  }
  State rul = metaindex_handle_.DecodeFrom(input);
  if (rul.ok()) {
    rul = index_handle_.DecodeFrom(input);
  }
  if (rul.ok()) {
    const char* end = magic_ptr + 8;
    *input = std::string_view(end, input->data() + input->size() - end);
  }
  return rul;
}
Memtable::Memtable()
    : arena(std::make_shared<Arena>()),
      table(std::make_unique<Skiplist>(arena)) {}
void Memtable::FindShortestSeparator(std::string* start,
                                     std::string_view limit) {}
void Memtable::FindShortSuccessor(std::string* key) {}
uint32_t Memtable::ApproximateMemoryUsage() { return table->Getsize(); }
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
    } else if ((found->key.Getag() & 0xf) == kTypeDeletion) {
      *s = State::Notfound();
    }
    mlog->debug("Get Key {} Value {} Seq {} Type {}", keys.getusrkeyview(),
                *value, found->key.Getag() >> 8, found->key.Getag() & 0xf);
    return true;
  } else {
    return false;
  }
}
State Memtable::Flushlevel0fromskip(FileMate& meta,
                                    std::unique_ptr<Tablebuilder>& builder) {
  node* p = table->SeekToFirst();
  meta.smallest = p->key;
  for (; table->Valid(p); p = table->Next(p)) {
    builder->Add(p->key.getview(), p->val);
    mlog->trace("builder add {}", p->key.getusrkeyview());
  }
  if (p = table->SeekToLast()) {
    if (table->Valid(p)) meta.largest = p->key;
  }
  State s = builder->Finish();
  return s;
}
void Tablebuilder::Add(const std::string_view& key,
                       const std::string_view& val) {
  assert(!closed);
  if (pending_index_entry) {
    assert(data_block.empty());
    std::string handle_entry;
    pending_handle.EncodeTo(&handle_entry);
    index_block.Add(key, std::string_view(handle_entry));
    pending_index_entry = false;
  }
  if (filter_block != nullptr) {
    filter_block->AddKey(key);
  }
  last_key.assign(key.data(), key.size());
  num_entries++;
  data_block.Add(key, val);
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
State Tablebuilder::Finish() {
  Flush();
  closed = true;
  BlockHandle filter_block_handle, metaindex_block_handle, index_block_handle;
  if (state.ok() && filter_block != nullptr) {  // Write filter block
    WriteRawBlock(filter_block->Finish(), kNoCompression, &filter_block_handle);
  }
  if (state.ok() && filter_block != nullptr) {  // Write metaindex block
    Blockbuilder meta_index_block(&options);
    if (filter_block != nullptr) {
      std::string key = "filter.bloom";
      std::string handle_encoding;
      filter_block_handle.EncodeTo(&handle_encoding);
      meta_index_block.Add(key, handle_encoding);
    }
    WriteBlock(&meta_index_block, &metaindex_block_handle);  // meta_index_block
  }
  if (state.ok()) {
    if (pending_index_entry) {
      std::string handle_encoding;
      pending_handle.EncodeTo(&handle_encoding);
      index_block.Add(last_key, handle_encoding);
      pending_index_entry = false;
    }
    WriteBlock(&index_block, &index_block_handle);
  }
  if (state.ok()) {
    Footer footer;
    footer.set_metaindex_handle(metaindex_block_handle);
    footer.set_index_handle(index_block_handle);
    std::string footer_encoding;
    mlog->info(
        "sstable flooter metaindex offset {} size {} index offset {} size {}",
        metaindex_block_handle.offset(), metaindex_block_handle.size(),
        index_block_handle.offset(), index_block_handle.size());
    footer.EncodeTo(&footer_encoding);
    state = file->Append(footer_encoding);
    if (state.ok()) {
      offset += footer_encoding.size();
    }
  }
  return state;
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
      if (snappy::Compress(raw.data(), raw.size(), compress)) {  //压缩比太小
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
    state = file->Append(std::string_view(ptr, kBlockBackSize));
    if (state.ok()) {
      offset += block_contents.size() + kBlockBackSize;
    }
  }
}
}  // namespace yubindb