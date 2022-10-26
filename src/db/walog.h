#ifndef YUBINDB_WALOG_H_
#define YUBINDB_WALOG_H_
#include <string_view>

#include "../util/common.h"
namespace yubindb {

//顺序写入
class WritableFile {
 public:
  explicit WritableFile();
  ~WritableFile();
  WritableFile(const WritableFile&) = delete;
  WritableFile& operator=(const WritableFile&) = delete;

  State Append(std::string_view ptr);
  State Close();
  State Flush();
  State Sync();

 private:
};
//顺序读取
class ReadFile {
 public:
  explicit ReadFile();
  ReadFile(const ReadFile&) = delete;
  ReadFile& operator=(const ReadFile&) = delete;
  ~ReadFile();
  State Read(size_t n, std::string_view* result, char* scratch);
  State Skip(uint64_t n);

 private:
};
class walWriter {
 public:
  explicit walWriter(WritableFile* file);
  ~walWriter();
  State Append(std::string_view str);

 private:
};
class walReader {
 public:
  explicit walReader();

 private:
};
}  // namespace yubindb
#endif