#include "reader.h"

#include <crc32c/crc32c.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "src/db/walog.h"
namespace yubindb {
Reader::Reader(std::shared_ptr<ReadFile> file_, bool checksum,
               uint64_t initial_offset)
    : file(file_),
      checksum_(checksum),
      backing_store_(new char[kBlockSize]),
      eof_(false),
      last_record_offset_(0),
      end_of_buffer_offset_(0),
      initial_offset_(initial_offset) {}
Reader::~Reader() {}
bool Reader::SkipToInitialBlock() {
  const size_t offset_in_block = initial_offset_ % kBlockSize;
  uint64_t block_start_location = initial_offset_ - offset_in_block;
  if (offset_in_block > kBlockSize - 6) {
    block_start_location += kBlockSize;
  }
  end_of_buffer_offset_ = block_start_location;
  if (block_start_location > 0) {
    State s = file->Skip(block_start_location);
    if (!s.ok()) {
      return false;
    }
  }
  return true;
}
uint64_t Reader::LastRecordOffset() { return last_record_offset_; }
bool Reader::ReadRecord(std::string* str) {
  if (last_record_offset_ < initial_offset_) {
    if (!SkipToInitialBlock()) {
      return false;
    }
  }
  str->clear();
  bool in_fragmented_record = false;
  uint64_t record_offset = 0;
  std::string_view frame;
  while (true) {
    const unsigned int record_type = ReadPhysicalRecord(&frame);
    uint64_t physical_recordoff =
        end_of_buffer_offset_ - backing_store_.size() - kHeaderSize - frame.size();
    switch (record_type) {
      case kFullType:
        if (in_fragmented_record) {
          if (!str->empty()) {
            mlog->error("partial record without end(1) size{}", str->size());
          }
        }
        record_offset = physical_recordoff;
        str->clear();
        *str = frame;
        last_record_offset_ = record_offset;
        return true;

      case kFirstType:
        if (in_fragmented_record) {
          if (!str->empty()) {
            mlog->error("partial record without end(2) size{}", str->size());
          }
        }
        record_offset = physical_recordoff;
        str->assign(frame.data(), frame.size());
        in_fragmented_record = true;
        break;

      case kMiddleType:
        if (!in_fragmented_record) {
          mlog->error("missing start of fragmented record(1) size{}",
                      str->size());
        } else {
          str->append(frame.data(), frame.size());
        }
        break;

      case kLastType:
        if (!in_fragmented_record) {
          mlog->error("missing start of fragmented record(2) size{}",
                      str->size());
        } else {
          str->append(frame.data(), frame.size());
          last_record_offset_ = record_offset;
          return true;
        }
        break;
      case kEof:
        if (in_fragmented_record) {
          str->clear();
        }
        return false;

      case kBadRecord:
        if (in_fragmented_record) {
          mlog->error("error in middle of record size{}", str->size());
          in_fragmented_record = false;
          str->clear();
        }
        break;

      default: {
        char buf[40];
        snprintf(buf, sizeof(buf), "unknown record type %u", record_type);
        mlog->error("{}", buf);
        in_fragmented_record = false;
        str->clear();
        break;
      }
    }
    return false;
  }
}
unsigned int Reader::ReadPhysicalRecord(std::string_view* result) {
  while (true) {
    if (backing_store_.size() < kHeaderSize) {
      if (!eof_) {
        backing_store_.clear();
        State status = file->Read(kBlockSize, &backing_store_);
        //当前Block结束位置的偏移
        end_of_buffer_offset_ += backing_store_.size();
        if (!status.ok()) {
          backing_store_.clear();
          eof_ = true;
          return kEof;
        } else if (backing_store_.size() < kBlockSize) {
          // 如果读到的数据小于kBlockSize，也说明到了文件结尾，eof_设为true
          eof_ = true;
        }
        continue;
      } else {
        backing_store_.clear();
        return kEof;
      }
    }
    const char* header = backing_store_.data();
    const uint32_t a = static_cast<uint32_t>(header[4]) & 0xff;
    const uint32_t b = static_cast<uint32_t>(header[5]) & 0xff;
    const unsigned int type = header[6];
    const uint32_t length = a | (b << 8);
    if (kHeaderSize + length > backing_store_.size()) {
      size_t drop_size = backing_store_.size();
      backing_store_.clear();
      if (!eof_) {
        mlog->error("bad record length size{}", drop_size);
        return kBadRecord;
      }
      return kEof;
    }
    if (type == kZeroType && length == 0) {
      backing_store_.clear();
      return kBadRecord;
    }
    // Check crc
    if (checksum_) {
      uint32_t expected_crc = DecodeFixed32(header);
      uint32_t actual_crc = crc32c::Crc32c(header + 6, 1 + length);
      if (actual_crc != expected_crc) {
        size_t drop_size = backing_store_.size();
        backing_store_.clear();
        mlog->error("checksum mismatch");
        return kBadRecord;
      }
    }
    // Skip physical record that started before initial_offset_
    if (end_of_buffer_offset_ - backing_store_.size() - kHeaderSize - length <
        initial_offset_) {
      return kBadRecord;
    }

    *result = std::string_view(header + kHeaderSize, length);
    return type;
  }
}
}  // namespace yubindb