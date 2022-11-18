#ifndef YUBINDB_FILENAME_H_
#define YUBINDB_FILENAME_H_
#include <assert.h>

#include <string>

#include "common.h"
namespace yubindb {
class PosixEnv;
enum FileType {
  kLogFile,
  kDBLockFile,
  kTableFile,
  kDescriptorFile,
  kCurrentFile,
  kTempFile,
  kInfoLogFile  // Either the current one, or an old one
};
static std::string MakeFileName(const std::string& dbname, uint64_t number,
                                const char* suffix);

std::string LogFileName(const std::string& dbname, uint64_t number);

std::string TableFileName(const std::string& dbname, uint64_t number);

std::string SSTTableFileName(const std::string& dbname, uint64_t number);
std::string VSSTTableFileName(const std::string& dbname, uint64_t number);
std::string DescriptorFileName(const std::string& dbname, uint64_t number);

std::string CurrentFileName(const std::string& dbname);

std::string LockFileName(const std::string& dbname);
std::string TempFileName(const std::string& dbname, uint64_t number);
std::string InfoLogFileName(const std::string& dbname);
std::string OldInfoLogFileName(const std::string& dbname);
bool ParsefileName(const std::string& filename, uint64_t number, FileType type);
}  // namespace yubindb
#endif