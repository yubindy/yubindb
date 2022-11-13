#include "env.h"

#include <dirent.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#include "filename.h"
#include "spdlog/spdlog.h"
namespace yubindb {
void Logv(const char* format, va_list ap) {}
State WritableFile::Append(std::string_view ptr) {
  return Append(ptr.data(), ptr.size());
}
static std::string_view Basename(const std::string& filename) {
  auto rul = filename.rfind('/');
  if (rul == std::string::npos) {
    return std::string_view(filename);
  }
  assert(filename.find('/', rul + 1) == std::string::npos);

  return std::string_view(filename.data() + rul + 1,
                          filename.length() - rul - 1);
}
static bool Ismanifest(const std::string& filename) {
  std::string_view p(Basename(filename).substr(0, 8));
  return p.compare("MANIFEST");
}
static std::string Dirname(const std::string& filename) {
  std::string::size_type separator_pos = filename.rfind('/');
  if (separator_pos == std::string::npos) {
    return std::string(".");
  }
  assert(filename.find('/', separator_pos + 1) == std::string::npos);

  return filename.substr(0, separator_pos);
}
State WriteStringToFile(PosixEnv* env, std::string_view data,
                        const std::string& fname, bool sync) {
  std::unique_ptr<WritableFile> file;
  State s = env->NewWritableFile(fname, file);
  if (!s.ok()) {
    return s;
  }
  s = file->Append(data);
  if (s.ok() && sync) {
    s = file->Sync();
  }
  file.reset();
  if (!s.ok()) {
    env->DeleteFile(fname);
  }
  return s;
}
State WritableFile::Append(const char* ptr, size_t size) {
  size_t wrsize = std::min(size, kWritableFileBufferSize - offset);
  const char* wrptr = ptr;
  std::memcpy(buf_, ptr, wrsize);
  size -= wrsize;
  wrptr += wrsize;
  offset += wrsize;
  if (size == 0) {
    return State::Ok();
  }
  Flush();
  if (size < kWritableFileBufferSize) {
    std::memcpy(buf_, ptr, size);
    offset += size;
    return State::Ok();
  }
  return Flush();
}
State WritableFile::Close() {
  State s = Flush();
  errno = 0;
  const int result = ::close(fd);
  if (result < 0 && s.ok()) {
    spdlog::error("error close: filename: {} err: {}", filestr.c_str(),
                  strerror(errno));
    s = State::IoError("error close");
  }
  fd = -1;
  return s;
}
State WritableFile::Flush() {
  char* t = buf_;
  uint64_t size = offset;
  ssize_t n = 0;
  while (size > 0) {
    n = ::write(fd, buf_, offset);
    if (n < 0) {
      spdlog::error("error write: filename: {} err: {}", Name(),
                    strerror(errno));
      if (errno == EINTR) {
        continue;
      }
      return State::IoError("error write");
    }
    t += n;
    size -= n;
  }
  offset = 0;
  return State::Ok();
}
State WritableFile::Sync(int fd, const std::string& pt) {
  if (ismainifset) {
    return SyncDirmainifset();
  }
  State p = Flush();
  if (!p.ok()) {
    return p;
  }
  if (::fsync(fd) == 0) {
    return State::Ok();
  }
  spdlog::error("error sync: filename: {} err: {}", pt.c_str(),
                strerror(errno));
  return State::IoError("sync");
}
State WritableFile::SyncDirmainifset() {
  int fd = ::open(dirstr.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    spdlog::error("error open: filename: {} err: {}", dirstr.c_str(),
                  strerror(errno));
    ::close(fd);
    return State::IoError("open error");
  } else {
    if (::fsync(fd) == 0) {
      return State::Ok();
    }
    spdlog::error("error SyncDirmainifset: filename: {} err: {}",
                  dirstr.c_str(), strerror(errno));
    return State::IoError("sync");
  }
  return State::Ok();
}
State PosixEnv::NewReadFile(const std::string& filename,
                            std::unique_ptr<ReadFile>& result) {
  int fd = ::open(filename.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    result = nullptr;
    spdlog::error("error open: filename: {} err: {}", filename,
                  strerror(errno));
    return State::IoError(filename.c_str());
  }
  result = std::make_unique<ReadFile>(filename, fd);
  return State::Ok();
}
State PosixEnv::NewWritableFile(const std::string& filename,
                                std::shared_ptr<WritableFile>& result) {
  int fd =
      ::open(filename.c_str(), O_TRUNC | O_WRONLY | O_CREAT | O_CLOEXEC, 0644);
  if (fd < 0) {
    result = nullptr;
    spdlog::error("error open: filename: {} err: {}", filename,
                  strerror(errno));
    return State::IoError(filename.c_str());
  }
  result = std::make_shared<WritableFile>(filename, fd);
  return State::Ok();
}
State PosixEnv::NewWritableFile(const std::string& filename,
                                std::unique_ptr<WritableFile>& result) {
  int fd =
      ::open(filename.c_str(), O_TRUNC | O_WRONLY | O_CREAT | O_CLOEXEC, 0644);
  if (fd < 0) {
    result = nullptr;
    spdlog::error("error open: filename: {} err: {}", filename,
                  strerror(errno));
    return State::IoError(filename.c_str());
  }
  result = std::make_unique<WritableFile>(filename, fd);
  return State::Ok();
}
State PosixEnv::NewAppendableFile(const std::string& filename,
                                  std::unique_ptr<WritableFile>& result) {
  int fd =
      ::open(filename.c_str(), O_APPEND | O_WRONLY | O_CREAT | O_CLOEXEC, 0644);
  if (fd < 0) {
    result = nullptr;
    spdlog::error("error open: filename: {} err: {}", filename,
                  strerror(errno));
    return State::IoError(filename.c_str());
  }
  result = std::make_unique<WritableFile>(filename, fd);
  return State::Ok();
}
State PosixEnv::CreateDir(const std::string& dirname) {
  if (::mkdir(dirname.c_str(), 0755) != 0) {
    spdlog::warn("warn createdir: dirname: {} err: {}", dirname,
                 strerror(errno));
    return State::IoError(dirname.c_str());
  }
  return State::Ok();
}
State PosixEnv::GetChildren(const std::string& directory_path,
                            std::vector<std::string>* result) {
  result->clear();
  ::DIR* dir = ::opendir(directory_path.c_str());
  if (dir == nullptr) {
    spdlog::error("error delterdir: dirname: {} err: {}", directory_path,
                  strerror(errno));
    return State::IoError(directory_path.c_str());
  }
  struct ::dirent* entry;
  while ((entry = ::readdir(dir)) != nullptr) {
    result->emplace_back(entry->d_name);
  }
  ::closedir(dir);
  return State::Ok();
}
State PosixEnv::DeleteDir(const std::string& dirname) {
  if (::rmdir(dirname.c_str()) != 0) {
    spdlog::error("error delterdir: dirname: {} err: {}", dirname,
                  strerror(errno));
    return State::IoError(dirname.c_str());
  }
  return State::Ok();
}
State PosixEnv::DeleteFile(const std::string& filename) {
  if (::unlink(filename.c_str()) != 0) {
    spdlog::error("error unlink: filename: {} err: {}", filename,
                  strerror(errno));
    return State::IoError(filename.c_str());
  }
  return State::Ok();
}
State PosixEnv::RenameFile(const std::string& from, const std::string& to) {
  if (::rename(from.c_str(), to.c_str()) != 0) {
    spdlog::error("error rename: filename: {} err: {}", from, strerror(errno));
    return State::IoError(from.c_str());
  }
  return State::Ok();
}
State PosixEnv::LockFile(const std::string& filename,
                         std::unique_ptr<FileLock>& lock) {
  int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
  if (fd < 0) {
    spdlog::error("error open : filename: {} err: {}", filename,
                  strerror(errno));
    return State::IoError(filename.c_str());
  }
  {
    filemutex.lock();
    filelock.insert(filename);
    filemutex.unlock();
  }
  struct ::flock file_lock_info;
  std::memset(&file_lock_info, 0, sizeof(file_lock_info));
  file_lock_info.l_type = F_WRLCK;
  file_lock_info.l_whence = SEEK_SET;
  file_lock_info.l_start = 0;
  file_lock_info.l_len = 0;
  errno = 0;
  if (::fcntl(fd, F_SETLK, &file_lock_info) == -1) {
    ::close(fd);
    filemutex.lock();
    filelock.erase(filename);
    filemutex.unlock();
    spdlog::error("error fcntl : filename: {lock+} err: {}", filename,
                  strerror(errno));
    return State::IoError(filename);
  }

  lock = std::make_unique<FileLock>(fd, filename);
  return State::Ok();
}
State PosixEnv::UnlockFile(std::unique_ptr<FileLock>& lock) {
  struct ::flock file_lock_info;
  std::memset(&file_lock_info, 0, sizeof(file_lock_info));
  file_lock_info.l_type = F_WRLCK;
  file_lock_info.l_whence = SEEK_SET;
  file_lock_info.l_start = 0;
  file_lock_info.l_len = 0;
  errno = 0;
  if (::fcntl((*lock).fd_, F_SETLK, &file_lock_info) == -1) {
    ::close((*lock).fd_);
    filemutex.lock();
    filelock.erase((*lock).filename_);
    filemutex.unlock();
    spdlog::error("error fcntl : filename: {} err: {}", (*lock).filename_,
                  strerror(errno));
    return State::IoError((*lock).filename_);
  }
  lock.reset();
  return State::Ok();
}
void PosixEnv::Schedule(backwork work) {
  background_work_mutex.lock();

  if (!started_background_thread) {
    started_background_thread = true;
    std::thread background_thread(work);
    background_thread.detach();
  }

  if (background_work_queue.empty()) {
    background_work_cond.notify_one();
  }
  background_work_queue.emplace(work);
  background_work_mutex.unlock();
}
void PosixEnv::StartThread(void (*thread_main)(void* thread_main_arg),
                           void* thread_main_arg) {
  std::thread threads(thread_main, thread_main_arg);
  threads.detach();
}
void PosixEnv::BackgroundThreadMain() {
  std::unique_lock<std::mutex> backwork(background_work_mutex);
  while (true) {
    backwork.lock();
    while (background_work_queue.empty()) {
      background_work_cond.wait(backwork);
    }
    std::function<void()> background_work_function =
        background_work_queue.front();
    background_work_queue.pop();

    backwork.unlock();
    background_work_function();
  }
}
}  // namespace yubindb
