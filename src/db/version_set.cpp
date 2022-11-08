#include "version_set.h"
namespace yubindb {
VersionSet::VersionSet(const std::string& dbname_, const Options* options,
                       std::shared_ptr<TableCache>& table_cache_)
    : env_(&options->env),
      dbname(dbname_),
      ops(options),
      table_cache(table_cache_),
      next_file_number(2),
      manifest_file_number(0),
      last_sequence(0),
      log_number(0),
      descriptor_file(nullptr),
      descriptor_log(nullptr),
      nowversion(nullptr) {}
int VersionSet::NumLevelFiles(int level) const {
  return nowversion->files[level].size();
}
}  // namespace yubindb