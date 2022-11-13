#include "version_set.h"

#include <algorithm>
namespace yubindb {
static int64_t TotalFileSize(
    const std::vector<std::shared_ptr<FileMate>>& files) {
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
class VersionSet::Builder {  // helper form edit+version=next version
 private:
  struct BySmallestKey {  // sort small of class
    bool operator()(FileMate* f1, FileMate* f2) const {
      int r = cmp(f1->smallest.getview(), f2->smallest.getview());
      if (r != 0) {
        return (r < 0);
      } else {
        // Break ties by file number
        return (f1->num < f2->num);
      }
    }
    bool operator()(const std::shared_ptr<FileMate>& f1,
                    const std::shared_ptr<FileMate>& f2) const {
      int r = cmp(f1->smallest.getview(), f2->smallest.getview());
      if (r != 0) {
        return (r < 0);
      } else {
        // Break ties by file number
        return (f1->num < f2->num);
      }
    }
  };
  typedef std::set<std::shared_ptr<FileMate>, BySmallestKey> FileSet;
  struct LevelState {
    std::set<uint64_t> deleted_files;
    std::shared_ptr<FileSet> added_files;
  };
  VersionSet* vset_;               // 待操作的VersionSet.
  std::shared_ptr<Version> base_;  // Version的基础版本
  LevelState levels_[config::kNumLevels];  //各level的变化过程，包括删、增的文件
 public:
  Builder(VersionSet* vset, std::shared_ptr<Version> base)
      : vset_(vset), base_(base) {
    BySmallestKey cmp;
    for (int level = 0; level < config::kNumLevels; level++) {
      levels_[level].added_files = std::make_shared<FileSet>(cmp);
    }
  }

  ~Builder() {}
  void Apply(VersionEdit* edit) {  // Version0 + VersionEdit
    // 更新VersionSet各层级的compact_pointers_。
    // Update compaction pointers
    for (size_t i = 0; i < edit->compact_pointers.size(); i++) {
      const int level = edit->compact_pointers[i].first;
      vset_->compact_pointer[level] =
          edit->compact_pointers[i].second.getString();
    }

    // 累加要删除的文件
    // Delete files
    for (const auto& deleted_file_set_kvp : edit->deleted_files) {
      const int level = deleted_file_set_kvp.first;
      const uint64_t number = deleted_file_set_kvp.second;
      levels_[level].deleted_files.insert(number);
    }

    // Add new files
    for (size_t i = 0; i < edit->new_files.size(); i++) {
      const int level = edit->new_files[i].first;
      const std::shared_ptr<FileMate> f =
          std::make_shared<FileMate>(edit->new_files[i].second);

      f->allowed_seeks = static_cast<int>(
          (f->file_size / 16384U));  // for seeks miss to compaction
      if (f->allowed_seeks < 100) f->allowed_seeks = 100;

      levels_[level].deleted_files.erase(f->num);
      levels_[level].added_files->insert(f);
    }
  }
  void SaveTo(std::unique_ptr<Version>& v) {
    BySmallestKey cmper;
    for (int level = 0; level < config::kNumLevels; level++) {
      // 将base_files、added_files排序后存储到Version*
      // v中，排序过程中需要丢弃的已删除的file。
      const std::vector<std::shared_ptr<FileMate>>& base_files =
          base_->files[level];  // now version
      std::vector<std::shared_ptr<FileMate>>::const_iterator base_iter =
          base_files.begin();
      std::vector<std::shared_ptr<FileMate>>::const_iterator base_end =
          base_files.end();
      const std::shared_ptr<FileSet> added_files = levels_[level].added_files;
      v->files[level].reserve(base_files.size() + added_files->size());
      for (const auto& added_file : *added_files) {
        // Add all smaller files listed in base_
        for (auto bpos =
                 std::upper_bound(base_iter, base_end, added_file, cmper);
             base_iter != bpos; ++base_iter) {
          MaybeAddFile(v, level, *base_iter);
        }

        MaybeAddFile(v, level, added_file);
      }

      // Add remaining base files
      for (; base_iter != base_end; ++base_iter) {
        MaybeAddFile(v, level, *base_iter);
      }

      // Make sure there is no overlap in levels > 0
      if (level > 0) {  // TODO level and teir
        for (uint32_t i = 1; i < v->files[level].size(); i++) {
          InternalKey& prev_end = v->files[level][i - 1]->largest;
          InternalKey& this_begin = v->files[level][i]->smallest;
          if (cmp(prev_end.getview(), this_begin.getview()) >= 0) {
            fprintf(stderr, "overlapping ranges in same level %s vs. %s\n",
                    prev_end.getString().c_str(),
                    this_begin.getString().c_str());
            abort();
          }
        }
      }
    }
  }
  void MaybeAddFile(std::unique_ptr<Version>& v, int level,
                    const std::shared_ptr<FileMate>& f) {
    if (levels_[level].deleted_files.count(f->num) > 0) {
      // File is deleted: do nothing
    } else {
      std::vector<std::shared_ptr<FileMate>>* files = &v->files[level];
      if (level > 0 && !files->empty()) {  // TODO level and teir
        // Must not overlap
        assert(cmp((*files)[files->size() - 1]->largest.getview(),
                   f->smallest.getview()) < 0);
      }
      files->push_back(f);
    }
  }
};
State VersionSet::Recover(bool* save_manifest) {
  *save_manifest = false;
}  // TODO
State VersionSet::LogAndApply(
    VersionEdit* edit, std::mutex* mu) {  // nowversion+versionedit=nextversion
  if (edit->has_log_number) {
    assert(edit->log_number >= log_number);
    assert(edit->log_number < next_file_number);
  } else {
    edit->SetLogNumber(log_number);
  }
  edit->SetNextFile(next_file_number);
  edit->SetLastSequence(last_sequence);

  std::unique_ptr<Version> v = std::make_unique<Version>(this);
  {
    Builder builder(this, nowversion);
    builder.Apply(edit);
    builder.SaveTo(v);
  }
  // TODO
}
int64_t VersionSet::NumLevelBytes(int level) const {
  return TotalFileSize(nowversion->files[level]);
}
void VersionSet::AddLiveFiles(std::set<uint64_t>* live) {
  for (auto v = versionlist.begin(); v != versionlist.end(); v++) {
    for (int level = 0; level < config::kNumLevels; level++) {
      std::vector<std::shared_ptr<FileMate>>& files = (*v)->files[level];
      for (size_t i = 0; i < files.size(); i++) {
        live->insert(files[i]->num);
      }
    }
  }
}
}  // namespace yubindb