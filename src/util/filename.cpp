#include "filename.h"

#include <string_view>
namespace yubindb {
static std::string MakeFileName(const std::string& dbname, uint64_t number,
                                const char* suffix) {
  char buf[100];
  snprintf(buf, sizeof(buf), "/%06llu.%s",
           static_cast<unsigned long long>(number), suffix);
  return dbname + buf;
}

std::string LogFileName(const std::string& dbname, uint64_t number) {
  assert(number > 0);
  return MakeFileName(dbname, number, "log");
}

std::string TableFileName(const std::string& dbname, uint64_t number) {
  assert(number > 0);
  return MakeFileName(dbname, number, "ldb");
}

std::string SSTTableFileName(const std::string& dbname, uint64_t number) {
  assert(number > 0);
  return MakeFileName(dbname, number, "sst");
}
std::string VSSTTableFileName(const std::string& dbname, uint64_t number) {
  assert(number > 0);
  return MakeFileName(dbname, number, "v_sst");  // for key sum value division
}
std::string DescriptorFileName(const std::string& dbname, uint64_t number) {
  assert(number > 0);
  char buf[100];
  snprintf(buf, sizeof(buf), "/MANIFEST-%06llu",
           static_cast<unsigned long long>(number));
  return dbname + buf;
}

std::string CurrentFileName(const std::string& dbname) {
  return dbname + "/CURRENT";
}

std::string LockFileName(const std::string& dbname) { return dbname + "/LOCK"; }

std::string TempFileName(const std::string& dbname, uint64_t number) {
  assert(number > 0);
  return MakeFileName(dbname, number, "dbtmp");
}

std::string InfoLogFileName(const std::string& dbname) {
  return dbname + "/LOG";
}
std::string OldInfoLogFileName(const std::string& dbname) {
  return dbname + "/LOG.old";
}
bool ParsefileName(const std::string& filename, uint64_t* number,
                   FileType* type) {
  std::string_view pfile(filename);
  if (pfile == "CURRENT") {
    *number = 0;
    *type = kCurrentFile;
  } else if (pfile == "LOG" || pfile == "LOG.old") {
    *number = 0;
    *type = kInfoLogFile;
  } else if (pfile.starts_with("MANIFEST-")) {
    pfile.remove_prefix(strlen("MANIFEST-"));
    uint64_t num;
    num = strtoull(pfile.data(), NULL, 0);
    if (!pfile.empty()) {
      return false;
    }
    *type = kDescriptorFile;
    *number = num;
  } else {
     uint64_t num;
    std::string_view suffix = pfile;
    if (suffix == std::string_view(".log")) {
      *type = kLogFile;
    } else if (suffix == std::string_view(".sst") || suffix == std::string_view(".ldb")) {
      *type = kTableFile;
    } else if (suffix == std::string_view(".dbtmp")) {
      *type = kTempFile;
    } else {
      return false;
    }
    *number = num;
  }
  return true;
}
}  // namespace yubindb