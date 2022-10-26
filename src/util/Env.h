#ifndef YUBINDB_ENV_H_
#define YUBINDB_ENV_H_
#include <cstdio>
#include <string_view>

#include "common.h"
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
class PosixEnv {
 public:
  PosixEnv() = default;
  ~PosixEnv() {
    static char msg[] = "PosixEnv destroyed!\n";
    std::fwrite(msg, 1, sizeof(msg), stderr);
    std::abort();
  }
  State NewReadFile(const std::string& filename, ReadFile** result);
  State NewRandomAccessFile(const std::string& filename,
                            RandomAccessFile** result);
  State NewWritableFile(const std::string& filename, WritableFile** result);
  State NewAppendableFile(const std::string& filename, WritableFile** result);
  State DeleteFile(const std::string& filename);
  State RenameFile(const std::string& from, const std::string& to);
  State LockFile(const std::string& filename, FileLock** lock);
  State UnlockFile(FileLock* lock);

 private:
};
}  // namespace yubindb
#endif