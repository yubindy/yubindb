
#include "walog.h"

#include "crc32c/crc32c.h"
namespace yubindb {

State walWriter::Appendrecord(std::string_view str) {
  uint64_t size = str.size();
  const char* ptr = str.data();
  bool begin = true, end;
  RecordType type;
  State s;
  while (size > 0) {
    uint64_t blockleft = kBlockSize - block_offset;
    if (blockleft < kHeaderSize) {
      if (blockleft > 0) {
        file->Append(std::string("\x00\x00\x00\x00\x00\x00", blockleft));
      }
      block_offset = 0;
    }
    const size_t avail = kBlockSize - block_offset - kHeaderSize;
    bool end = avail > blockleft ? blockleft : avail;
    if (begin && end) {
      type = kFullType;
    } else if (begin) {
      type = kFirstType;
    } else if (end) {
      type = kLastType;
    } else {
      type = kMiddleType;
    }
    s = Flushphyrecord(type, ptr, std::min(blockleft, avail));
    ptr += std::min(blockleft, avail);
    size -= std::min(blockleft, avail);
  }
  return s;
}
State walWriter::Flushphyrecord(RecordType type, const char* buf_,
                                size_t size) {
  assert(block_offset + kHeaderSize + size <= kBlockSize);
  char buf[3];
  buf[0] = (size & 0xff);
  buf[1] = (size >> 8);
  buf[2] = type;
  uint32_t check_sum = crc32c::Crc32c(buf_, size); //SEGMENTATION FAULT use error
  State s;
  s = file->Append((char*)&check_sum, sizeof(check_sum));  // crc
  if (!s.ok()) {
    return s;
  }
  s = file->Append(buf, sizeof(buf));  // size + type
  if (!s.ok()) {
    return s;
  }
  s = file->Append(buf, size);
  if (!s.ok()) {
    return s;
  }
  file->Flush();
  block_offset += kHeaderSize + size;
  return s;
}
}  // namespace yubindb