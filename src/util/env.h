#ifndef YUBINDB_ENV_H_
#define YUBINDB_ENV_H_
#include <condition_variable>
#include <cstdio>
#include <fcntl.h>
#include <functional>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <string_view>
#include <unistd.h>

#include "common.h"
#include "filename.h"

namespace yubindb {
const int kNumNonTableCacheFiles = 10;

class PosixEnv;

constexpr const uint32_t kWritableFileBufferSize = 64 * 1024;
static std::string_view Basename(const std::string& filename);
static bool Ismanifest(const std::string& filename);
static std::string Dirname(const std::string& filename);
State WriteStringToFile(PosixEnv* env, std::string_view data,
                        const std::string& fname, bool sync);
class FileLock {
 public:
  FileLock(int fd, std::string filename)
      : fd_(fd), filename_(std::move(filename)) {}

  int fd() const { return fd_; }
  const std::string& filename() const { return filename_; }

  const int fd_;
  const std::string filename_;
};
//顺序写入
class WritableFile {
 public:
  explicit WritableFile(std::string str_, int fd_)
      : offset(0),
        fd(fd_),
        filestr(std::move(str_)),
        dirstr(Dirname(filestr)),
        ismainifset(Ismanifest(filestr)) {}
  ~WritableFile() = default;
  WritableFile(const WritableFile&) = delete;
  WritableFile& operator=(const WritableFile&) = delete;

  State Append(std::string_view ptr);
  State Append(const char* ptr, uint32_t size);
  std::string_view Name() { return std::string_view(filestr); }
  State Close();
  State Flush();  // flush是将我们自己的缓冲写入文件
  State Sync() { return Sync(fd, filestr); }
  State Sync(int fd, const std::string& pt);
  State SyncDirmainifset();

 private:
  char buf_[kWritableFileBufferSize];
  uint32_t offset;
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
  ~ReadFile() = default;
  std::string_view Name() { return std::string_view(str); }
  State Read(uint32_t n, std::string_view result, char* scratch);
  State Skip(uint64_t n);

 private:
  std::string str;
  int fd;
};
class PosixEnv {  // TODO static
 public:
  typedef std::function<void()> backwork;
  PosixEnv() = default;
  ~PosixEnv() {
    static char msg[] = "PosixEnv destroyed!\n";
    std::fwrite(msg, 1, sizeof(msg), stderr);
    std::abort();
  }
  State NewReadFile(const std::string& filename,
                    std::unique_ptr<ReadFile>& result);
  // State NewRandomAccessFile(const std::string& filename,
  //                           RandomAccessFile** result);
  State NewWritableFile(const std::string& filename,
                        std::shared_ptr<WritableFile>& result);
  State NewWritableFile(const std::string& filename,
                        std::unique_ptr<WritableFile>& result);
  State NewAppendableFile(const std::string& filename,
                          std::unique_ptr<WritableFile>& result);
  State GetFileSize(const std::string& filename, uint64_t* size);
  State CreateDir(const std::string& dirname);
  State GetChildren(const std::string& directory_path,
                    std::vector<std::string>* result);
  State DeleteDir(const std::string& dirname);
  State DeleteFile(const std::string& filename);
  State RenameFile(const std::string& from, const std::string& to);
  State LockFile(const std::string& filename, std::unique_ptr<FileLock>& lock);
  State UnlockFile(std::unique_ptr<FileLock>& lock);
  void Schedule(backwork work);
  void StartThread(void (*thread_main)(void* thread_main_arg),
                   void* thread_main_arg);
  // State NewLogger(const std::string& filename, std::unique_ptr<Logger>
  // result);
  bool FileExists(const std::string& filename) {
    return ::access(filename.c_str(), F_OK) == 0;
  }

 private:
  // background
  void BackgroundThreadMain();
  std::mutex background_work_mutex;
  std::condition_variable background_work_cond;
  std::queue<backwork> background_work_queue;
  bool started_background_thread;

  // filemutex
  std::set<std::string> filelock;
  std::mutex filemutex;  // lock filelock
};
State SetCurrentFile(PosixEnv* env, const std::string& dbname,
                     uint64_t file_number);
}  // namespace yubindb
#endif