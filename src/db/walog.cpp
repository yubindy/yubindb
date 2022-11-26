
#include "walog.h"

#include "crc32c/crc32c.h"
namespace yubindb {

State walWriter::Appendrecord(std::string_view str) {
  uint32_t size = str.size();
  const char* ptr = str.data();
  bool begin = true, end;
  RecordType type;
  State s;
  while (size > 0) {
    uint32_t blockleft = kBlockSize - block_offset;
    if (blockleft < kHeaderSize) {
      if (blockleft > 0) {
        file->Append(std::string("\x00\x00\x00\x00\x00\x00", blockleft));
      }
      block_offset = 0;
    }
    const uint32_t avail = kBlockSize - block_offset - kHeaderSize;
    bool end = avail > size ? size : avail;
    if (begin && end) {
      type = kFullType;
    } else if (begin) {
      type = kFirstType;
    } else if (end) {
      type = kLastType;
    } else {
      type = kMiddleType;
    }
    s = Flushphyrecord(type, ptr, std::min(size, avail));
    ptr += std::min(size, avail);
    size -= std::min(size, avail);
  }
  return s;
}
State walWriter::Flushphyrecord(RecordType type, const char* buf_,
                                uint32_t size) {
  assert(block_offset + kHeaderSize + size <= kBlockSize);
  mlog->info("wal write flushrecord type: {} msg: {} size: {}", type, buf_,
               size);
  char buf[kHeaderSize];
  buf[4] = (size & 0xff);
  buf[5] = (size >> 8);
  buf[6] = type;
  uint32_t check_sum = crc32c::Extend(type_crc[type], (uint8_t*)(buf_), size);
  EncodeFixed32(buf, check_sum);
  State s;
  s = file->Append(buf, kHeaderSize);  // crc+size + type
  if (!s.ok()) {
    return s;
  }
  s = file->Append(buf_, size);
  if (!s.ok()) {
    return s;
  }
  file->Flush();
  block_offset += kHeaderSize + size;
  return s;
}
}  // namespace yubindb