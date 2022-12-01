#ifndef YUBINDB_READER_H_
#define YUBINDB_READER_H_
#include <memory>
#include <string_view>

#include "../util/env.h"
#include "../util/loger.h"
#include "walog.h"
namespace yubindb {
class Reader {
 public:
  Reader(std::shared_ptr<ReadFile> file, bool checksum,
         uint64_t initial_offset);

  Reader(const Reader&) = delete;
  Reader& operator=(const Reader&) = delete;

  ~Reader();
  bool ReadRecord(std::string* str);
  uint64_t LastRecordOffset();

 private:
  // Extend record types with the following special values
  enum {
    kEof = kLastType + 1,
    kBadRecord = kLastType + 2
  };
  bool SkipToInitialBlock();
  unsigned int ReadPhysicalRecord(std::string* result);

  std::shared_ptr<ReadFile>  file;
  bool const checksum_;       //是否进行数据校验
  std::string backing_store_;
  bool eof_;
  uint64_t last_record_offset_;
  uint64_t end_of_buffer_offset_;
  // 初始Offset
  uint64_t const initial_offset_;
};
}  // namespace yubindb
#endif 