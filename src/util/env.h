#ifndef YUBINDB_ENV_H_
#define YUBINDB_ENV_H_
#include <cstdio>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "common.h"

constexpr const size_t kWritableFileBufferSize = 64 * 1024;
namespace yubindb {
//顺序写入
class WritableFile {
 public:
  explicit WritableFile(std::string str_, int fd_)
      : offset(0),
        filestr(std::move(str_)),
        dirstr(Dirname(filestr)),
        fd(fd_),
        ismainifset(Ismainset(filestr)) {}
  ~WritableFile();
  WritableFile(const WritableFile&) = delete;
  WritableFile& operator=(const WritableFile&) = delete;

  State Append(std::string_view ptr);
  State Append(const char* ptr, size_t size);
  std::string_view Name() { return std::string_view(filestr); }
  bool Ismainset(std::string_view s);
  State Close();
  State Flush();  // flush是将我们自己的缓冲写入文件
  State Sync() { return Sync(fd, filestr); }
  State Sync(int fd, const std::string& pt);
  State SyncDirmainifset();
  static std::string Dirname(const std::string& filename) {
    auto pos = filename.rfind('/');
    if (pos == std::string::npos) {
      return std::string(".");
    }

    assert(filename.find('/', pos + 1) == std::string::npos);

    return filename.substr(0, pos);
  }

 private:
  char buf_[kWritableFileBufferSize];
  size_t offset;
  int fd;
  const std::string filestr;
  const std::string dirstr;
  const bool ismainifset;
};
//顺序读取
class ReadFile {
 public:
  explicit ReadFile(std::string str_, int fd_) : str(str_), fd(fd_) {}
  ReadFile(const ReadFile&) = delete;
  ReadFile& operator=(const ReadFile&) = delete;
  ~ReadFile();
  std::string_view Name() { return std::string_view(str); }
  State Read(size_t n, std::string_view result, char* scratch);
  State Skip(uint64_t n);

 private:
  std::string str;
  int fd;
};
class PosixEnv {
 public:
  PosixEnv() = default;
  ~PosixEnv() {
    static char msg[] = "PosixEnv destroyed!\n";
    std::fwrite(msg, 1, sizeof(msg), stderr);
    std::abort();
  }
  State NewReadFile(const std::string& filename,
                    std::unique_ptr<ReadFile> result);
  // State NewRandomAccessFile(const std::string& filename,
  //                           RandomAccessFile** result);
  State NewWritableFile(const std::string& filename,
                        std::unique_ptr<WritableFile> result);
  State NewAppendableFile(const std::string& filename,
                          std::unique_ptr<WritableFile> result);
  State DeleteFile(const std::string& filename);
  State RenameFile(const std::string& from, const std::string& to);
  // State LockFile(const std::string& filename);
  // State UnlockFile(const std::string& filename);

 private:
  std::unordered_map<int, std::string> filelock;
  std::mutex filemutex;  // lock filelock
};
}  // namespace yubindb
#endif