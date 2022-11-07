#include "version_set.h"
namespace yubindb {
VersionSet::VersionSet(const std::string& dbname_, const Options* options,
                       TableCache* table_cache)
    : env_(&options->env),
      dbname(dbname_),
      ops(options),
      next_file_number(2),
      manifest_file_number(0),
      last_sequence(0),
      log_number(0) {}
int VersionSet::NumLevelFiles(int level) const {
  return nowversion->files[level].size();
}
}  // namespace yubindb