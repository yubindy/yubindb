#include "version_edit.h"

#include <string_view>

#include "../util/common.h"
#include "db.h"
namespace yubindb {
void VersionEdit::Clear() {
  log_number = 0;
  last_sequence = 0;
  next_file_number = 0;
  has_log_number = false;
  has_next_file_number = false;
  has_last_sequence = false;
  deleted_files.clear();
  new_files.clear();
}

void VersionEdit::EncodeTo(std::string* dst) const {
  if (has_log_number) {
    PutVarint32(dst, kLogNumber);
    PutVarint64(dst, log_number);
  }
  if (has_next_file_number) {
    PutVarint32(dst, kNextFileNumber);
    PutVarint64(dst, next_file_number);
  }
  if (has_last_sequence) {
    PutVarint32(dst, kLastSequence);
    PutVarint64(dst, last_sequence);
  }

  for (uint32_t i = 0; i < compact_pointers.size(); i++) {
    PutVarint32(dst, kCompactPointer);
    PutVarint32(dst, compact_pointers[i].first);  // level
    PutLengthPrefixedview(dst, compact_pointers[i].second.getview());
  }

  for (const auto& deleted_file_kvp : deleted_files) {
    PutVarint32(dst, kDeletedFile);
    PutVarint32(dst, deleted_file_kvp.first);   // level
    PutVarint64(dst, deleted_file_kvp.second);  // file number
  }

  for (uint32_t i = 0; i < new_files.size(); i++) {
    const FileMate& f = new_files[i].second;
    PutVarint32(dst, kNewFile);
    PutVarint32(dst, new_files[i].first);  // level
    PutVarint64(dst, f.num);
    PutVarint64(dst, f.file_size);
    PutLengthPrefixedview(dst, f.smallest.getview());
    PutLengthPrefixedview(dst, f.largest.getview());
  }
}
static bool GetInternalKey(std::string_view* input, InternalKey* dst) {
  std::string_view str;
  if (GetLengthPrefixedview(input, &str)) {
    return dst->DecodeFrom(str);
  } else {
    return false;
  }
}
static bool GetLevel(std::string_view* input, int* level) {
  uint32_t v;
  if (GetVarint32(input, &v) && v < kNumLevels) {
    *level = v;
    return true;
  } else {
    return false;
  }
}
State VersionEdit::DecodeFrom(std::string_view src) {
  Clear();
  std::string_view input = src;
  const char* msg = nullptr;
  uint32_t tag;
  int level;
  uint64_t number;
  FileMate f;
  std::string_view str;
  InternalKey key;

  while (msg == nullptr && GetVarint32(&input, &tag)) {
    switch (tag) {
      case kLogNumber:
        if (GetVarint64(&input, &log_number)) {
          has_log_number = true;
        } else {
          msg = "VersionEdit : log number";
        }
        break;
      case kNextFileNumber:
        if (GetVarint64(&input, &next_file_number)) {
          has_next_file_number = true;
        } else {
          msg = "VersionEdit : next file number";
        }
        break;

      case kLastSequence:
        if (GetVarint64(&input, &last_sequence)) {
          has_last_sequence = true;
        } else {
          msg = "VersionEdit : last sequence number";
        }
        break;

      case kDeletedFile:
        if (GetLevel(&input, &level) && GetVarint64(&input, &number)) {
          deleted_files.insert(std::make_pair(level, number));
        } else {
          msg = "VersionEdit : deleted file";
        }
        break;

      case kNewFile:
        if (GetLevel(&input, &level) && GetVarint64(&input, &f.num) &&
            GetVarint64(&input, &f.file_size) &&
            GetInternalKey(&input, &f.smallest) &&
            GetInternalKey(&input, &f.largest)) {
          new_files.push_back(std::make_pair(level, f));
        } else {
          msg = "VersionEdit : new-file entry";
        }
        break;

      default:
        msg = "VersionEdit : unknown tag";
        break;
    }
  }

  if (msg == nullptr && !input.empty()) {
    msg = "VersionEdit : invalid tag";
  }

  State result;
  if (msg != nullptr) {
    result = State::Corruption(msg);
    spdlog::error(msg);
  }
  return result;
}
}  // namespace yubindb