#include "env.h"

#include <memory.h>

#include "spdlog/spdlog.h"

namespace yubindb {
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