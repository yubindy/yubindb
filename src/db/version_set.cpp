#include "version_set.h"
namespace yubindb {
static int64_t TotalFileSize(
    const std::vector<std::unique_ptr<FileMate>>& files) {
  int64_t sum = 0;
  for (size_t i = 0; i < files.size(); i++) {
    sum += files[i]->file_size;
  }
  return sum;
}
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
State VersionSet::Recover(bool* save_manifest) {}                    // TODO
State VersionSet::LogAndApply(VersionEdit* edit, std::mutex* mu) {}  // TODO

int64_t VersionSet::NumLevelBytes(int level) const {
  return TotalFileSize(nowversion->files[level]);
}
void VersionSet::AddLiveFiles(std::set<uint64_t>* live) {
  for (auto v = versionlist.begin(); v != versionlist.end(); v++) {
    for (int level = 0; level < config::kNumLevels; level++) {
      std::vector<std::unique_ptr<FileMate>>& files = (*v)->files[level];
      for (size_t i = 0; i < files.size(); i++) {
        live->insert(files[i]->num);
      }
    }
  }
}
}  // namespace yubindb