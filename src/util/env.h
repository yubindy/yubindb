#ifndef YUBINDB_ENV_H_
#define YUBINDB_ENV_H_
#include <condition_variable>
#include <cstdio>
#include <fcntl.h>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <string_view>
#include <unistd.h>

#include "common.h"

constexpr const size_t kWritableFileBufferSize = 64 * 1024;
namespace yubindb {
// class Logger{

// }
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
                    std::unique_ptr<ReadFile>& result);
  // State NewRandomAccessFile(const std::string& filename,
  //                           RandomAccessFile** result);
  State NewWritableFile(const std::string& filename,
                        std::unique_ptr<WritableFile>& result);
  State NewAppendableFile(const std::string& filename,
                          std::unique_ptr<WritableFile>& result);
  State GetFileSize(const std::string& filename, uint64_t* size);
  State CreateDir(const std::string& dirname);
  State DeleteDir(const std::string& dirname);
  State DeleteFile(const std::string& filename);
  State RenameFile(const std::string& from, const std::string& to);
  State LockFile(const std::string& filename, std::unique_ptr<FileLock>& lock);
  State UnlockFile(std::unique_ptr<FileLock>& lock);
  void Schedule(void (*background_function)(void* background_arg),
                void* background_arg);
  void StartThread(void (*thread_main)(void* thread_main_arg),
                   void* thread_main_arg);
  // State NewLogger(const std::string& filename, std::unique_ptr<Logger>
  // result);
  bool FileExists(const std::string& filename) {
    return ::access(filename.c_str(), F_OK) == 0;
  }

 private:
  struct BackgroundWorkItem {
    explicit BackgroundWorkItem(void (*function)(void* arg), void* arg)
        : function(function), arg(arg) {}

    void (*const function)(void*);
    void* const arg;
  };
  // background
  void BackgroundThreadMain();
  static void BackgroundThread(PosixEnv* env) { env->BackgroundThreadMain(); }
  std::mutex background_work_mutex;
  std::condition_variable background_work_cond;
  std::queue<BackgroundWorkItem> background_work_queue;
  bool started_background_thread;

  // filemutex
  std::set<std::string> filelock;
  std::mutex filemutex;  // lock filelock
};
}  // namespace yubindb
#endif