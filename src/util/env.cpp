#include "env.h"

#include <fcntl.h>
#include <memory.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "spdlog/spdlog.h"

namespace yubindb {
State WritableFile::Append(std::string_view ptr) {
  return Append(ptr.data(), ptr.size());
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
  if (::fdatasync(fd) == 0) {
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
    return State::IoError("open error");
  } else {
    ::close(fd);
    if (::fdatasync(fd) == 0) {
      return State::Ok();
    }
    spdlog::error("error sync: filename: {} err: {}", dirstr.c_str(),
                  strerror(errno));
    return State::IoError("sync");
  }
  return State::Ok();
}
State PosixEnv::NewReadFile(const std::string& filename,
                            std::unique_ptr<ReadFile> result) {
  int fd = ::open(filename.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    result = nullptr;
    spdlog::error("error open: filename: {} err: {}", filename,
                  strerror(errno));
    return State::IoError(filename.c_str());
  }
  result = std::make_unique<ReadFile>(ReadFile(filename, fd));
  return State::Ok();
}
State PosixEnv::NewWritableFile(const std::string& filename,
                                std::unique_ptr<WritableFile> result) {
  int fd =
      ::open(filename.c_str(), O_TRUNC | O_WRONLY | O_CREAT | O_CLOEXEC, 0644);
  if (fd < 0) {
    result = nullptr;
    spdlog::error("error open: filename: {} err: {}", filename,
                  strerror(errno));
    return State::IoError(filename.c_str());
  }
  result = std::make_unique<WritableFile>(WritableFile(filename, fd));
  return State::Ok();
}
State PosixEnv::NewAppendableFile(const std::string& filename,
                                  std::unique_ptr<WritableFile> result) {
  int fd =
      ::open(filename.c_str(), O_APPEND | O_WRONLY | O_CREAT | O_CLOEXEC, 0644);
  if (fd < 0) {
    result = nullptr;
    spdlog::error("error open: filename: {} err: {}", filename,
                  strerror(errno));
    return State::IoError(filename.c_str());
  }
  result = std::make_unique<WritableFile>(WritableFile(filename, fd));
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
// State PosixEnv::LockFile(const std::string& filename) {
//   int fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
//   if (fd < 0) {
//     spdlog::error("error open : filename: {} err: {}", filename,
//                   strerror(errno));
//     return State::IoError(filename.c_str());
//   }
//   {
//     filemutex.lock();
//     filelock[fd] = filename;
//     filemutex.unlock();
//   }
//   struct ::flock file_lock_info;
//   std::memset(&file_lock_info, 0, sizeof(file_lock_info));
//   file_lock_info.l_type = F_WRLCK;
//   file_lock_info.l_whence = SEEK_SET;
//   file_lock_info.l_start = 0;
//   file_lock_info.l_len = 0;
//   errno = 0;
//   if (::fcntl(fd, F_SETLK, &file_lock_info) == -1) {
//     ::close(fd);
//     filemutex.lock();
//     filelock.erase(fd);
//     filemutex.unlock();
//     spdlog::error("error fcntl : filename: {} err: {}", filename,
//                   strerror(errno));
//     return State::IoError(filename);
//   }

//   return State::Ok();
// }
// State PosixEnv::UnlockFile(const std::string& filename) {
//   struct ::flock file_lock_info;
//   std::memset(&file_lock_info, 0, sizeof(file_lock_info));
//   file_lock_info.l_type = F_WRLCK;
//   file_lock_info.l_whence = SEEK_SET;
//   file_lock_info.l_start = 0;
//   file_lock_info.l_len = 0;
//   errno = 0;
//   if (::fcntl(fd, F_SETLK, &file_lock_info) == -1) {
//     ::close(fd);
//     filemutex.lock();
//     filelock.erase(filename);
//     filemutex.unlock();
//     spdlog::error("error fcntl : filename: {} err: {}", filename,
//                   strerror(errno));
//     return State::IoError(filename);
//   }

// return State::Ok();
//}
}  // namespace yubindb