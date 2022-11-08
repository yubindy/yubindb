#ifndef YUBINDB_FILENAME_H_
#define YUBINDB_FILENAME_H_
#include <assert.h>
#include <string>

#include "common.h"
#include "env.h"
namespace yubindb {
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
State SetCurrentFile(PosixEnv* env, const std::string& dbname,
                     uint64_t file_number);
}  // namespace yubindb
#endif