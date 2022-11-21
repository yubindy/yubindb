#include "version_set.h"

#include <algorithm>
#include <memory>
#include <string_view>

#include "src/db/version_edit.h"
#include "src/util/env.h"
#include "src/util/filename.h"
#include "src/util/key.h"
#include "src/util/options.h"
namespace yubindb {
static int64_t TotalFileSize(
    const std::vector<std::shared_ptr<FileMate>>& files) {
  int64_t sum = 0;
  for (size_t i = 0; i < files.size(); i++) {
    sum += files[i]->file_size;
  }
  return sum;
}
static double MaxBytesForLevel(const Options* options, int level) {
  double result = 10. * 1048576.0;
  while (level > 1) {
    result *= 10;
    level--;
  }
  return result;
}
void Version::GetOverlappFiles(
    int level, const InternalKey* begin, const InternalKey* end,
    std::vector<std::shared_ptr<FileMate>>*
        inputs) {  // get small and large and push inputs
  inputs->clear();
  InternalKey user_beg = *begin, user_end = *end;
  for (size_t i = 0; i < files[i].size(); i++) {
    std::shared_ptr<FileMate> p = files[0][i];
    if (begin != nullptr && cmp(user_beg, p->largest) > 0) {
    } else if (end != nullptr && cmp(*end, p->smallest) < 0) {
    } else {
      inputs->push_back(p);
      if (begin != nullptr && cmp(user_end, p->smallest) < 0) {
        user_beg = p->smallest;
        inputs->clear();
        i = 0;
      } else if (end != nullptr && cmp(p->largest, user_end) > 0) {
        user_end = p->largest;
        inputs->clear();
        i = 0;
      }
    }
  }
}
VersionSet::VersionSet(const std::string& dbname_, const Options* options,
                       std::shared_ptr<TableCache>& table_cache_,
                       std::shared_ptr<PosixEnv>& env)
    : env_(env),
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
      int r = cmp(f1->smallest, f2->smallest);
      if (r != 0) {
        return (r < 0);
      } else {
        // Break ties by file number
        return (f1->num < f2->num);
      }
      return true;
    }
    bool operator()(const std::shared_ptr<FileMate>& f1,
                    const std::shared_ptr<FileMate>& f2) const {
      int r = cmp(f1->smallest, f2->smallest);
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
          if (cmp(prev_end, this_begin) >= 0) {
            fprintf(stderr, "overlapping ranges in same level %s vs. %s\n",
                    prev_end.getString().c_str(),
                    this_begin.getString().c_str());
            abort();
          }
        }
      }
    }
  }
  void MaybeAddFile(std::unique_ptr<Version>& v,
                    int level,  //如果满足，则加入version 的 level filemate pair
                    const std::shared_ptr<FileMate>& f) {
    if (levels_[level].deleted_files.count(f->num) > 0) {
      // File is deleted: do nothing
    } else {
      std::vector<std::shared_ptr<FileMate>>* files = &v->files[level];
      if (level > 0 && !files->empty()) {  // TODO level and teir
        // Must not overlap
        assert(cmp((*files)[files->size() - 1]->largest, f->smallest) < 0);
      }
      files->push_back(f);
    }
  }
};
State VersionSet::Recover(bool* save_manifest) {
  *save_manifest = false;
  return State::Ok();
}  // TODO
State VersionSet::LogAndApply(
    VersionEdit* edit,
    std::mutex* mu) {  //将VersionEdit应用到Current Version,并且写入current文件
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
    builder.Apply(edit);  //+
    builder.SaveTo(v);    //=
  }
  Finalize(v);  // choose next compaction

  std::string new_mainifset_file;
  State s;
  if (descriptor_log == nullptr) {
    new_mainifset_file = DescriptorFileName(dbname, manifest_file_number);
    edit->SetNextFile(next_file_number);
    s = env_->NewWritableFile(new_mainifset_file, descriptor_file);
    if (s.ok()) {
      descriptor_log = std::make_unique<walWriter>(descriptor_file);
      s = WriteSnapshot(descriptor_log);  // 把nowversion版本写入MANIFEST
    }
  }
  {
    mu->unlock();
    if (s.ok()) {
      std::string record;
      edit->EncodeTo(&record);
      s = descriptor_log->Appendrecord(record);  //将edit写入MANIFEST
      if (s.ok()) {
        s = descriptor_file->Sync();
      }
    }
    if (s.ok() && !new_mainifset_file.empty()) {
      s = SetCurrentFile(env_.get(), dbname, manifest_file_number);
    }
    mu->lock();
  }
  if (s.ok()) {
    nowversion = std::make_shared<Version>(v.release());
    versionlist.emplace_front(nowversion);
    log_number = edit->log_number;
  } else {
    descriptor_log = nullptr;
    descriptor_file = nullptr;
    env_->DeleteFile(new_mainifset_file);
  }
  return s;
}
State VersionSet::WriteSnapshot(
    std::unique_ptr<walWriter>& log) {  //将nowversion版本写入MANIFEST
  VersionEdit edit;

  for (int i = 0; i < config::kNumLevels; i++) {  // save compaction pointer;
    if (!compact_pointer[i].empty()) {
      InternalKey key;
      key.DecodeFrom(compact_pointer[i]);
      edit.SetCompactPointer(i, key);
    }
  }

  for (int level = 0; level < config::kNumLevels; level++) {
    auto& p = nowversion->files[level];
    for (int i = 0; i < p.size(); i++) {
      std::shared_ptr<FileMate> f = p[i];
      edit.AddFile(level, f->num, f->file_size, f->smallest, f->largest);
    }
  }

  std::string record;
  edit.EncodeTo(&record);
  return log->Appendrecord(record);
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
void VersionSet::Finalize(std::unique_ptr<Version>& v) {
  int maxlevel = -1;
  double maxscore = -1;
  for (int i = 0; i < config::kNumLevels - 1; i++) {
    double score;
    if (i == 0) {
      score = v->files[i].size() / static_cast<double>(config::kL0_Compaction);
    } else {
      const uint64_t level_bytes = TotalFileSize(v->files[i]);
      score = static_cast<double>(level_bytes) / MaxBytesForLevel(ops, i);
      if (score > maxscore) {
        maxlevel = i;
        maxscore = score;
      }
    }
    nowversion->compaction_level = maxlevel;
    nowversion->compaction_score = maxscore;
  }
}
bool VersionSet::NeedsCompaction() {
  return (nowversion->compaction_score >= 1);
}
std::unique_ptr<Compaction>
VersionSet::PickCompaction() {  // choose compaction input file
  const bool size_compaction = (nowversion->compaction_score >= 1);
  std::unique_ptr<Compaction> comp;
  int level;
  if (size_compaction) {
    level = nowversion->compaction_level;
    comp = std::unique_ptr<Compaction>(ops, level);
    for (int i = 0; i < nowversion->files[level].size(); i++) {
      if (compact_pointer[level].empty() ||
          cmp(compact_pointer[level],
              nowversion->files[level][i]->largest.getview()) > 0) {
        comp->inputs_[0].emplace_back(nowversion->files[level][i]);
        break;
      }
    }
    if (comp->inputs_[0].empty()) {
      comp->inputs_[0].push_back(nowversion->files[level][0]);
    }
  } else {
    return nullptr;
  }
  comp->input_version_ = nowversion;

  if (level == 0) {
    InternalKey small, large;
    GetRange(comp->inputs_[0], &small, &large);
    nowversion->GetOverlappFiles(0, &small, &large,
                                 &comp->inputs_[0]);  // choose n+1 file
  }
  SetupOtherInputs(comp);  // set input[1] n+1;
  return comp;
}
void VersionSet::GetRange(const std::vector<std::shared_ptr<FileMate>>& inputs,
                          InternalKey* smallest, InternalKey* largest) {
  smallest->clear();
  largest->clear();
  *smallest = inputs[0]->smallest;
  *largest = inputs[0]->largest;
  for (int i = 1; i < inputs.size(); i++) {
    std::shared_ptr<FileMate> p = inputs[i];
    if (cmp(*smallest, p->smallest) < 0) {
      *smallest = p->smallest;
    }
    if (cmp(*largest, p->largest) > 0) {
      *largest = p->largest;
    }
  }
}
void VersionSet::GetRange2(
    const std::vector<std::shared_ptr<FileMate>>& inputs1,
    const std::vector<std::shared_ptr<FileMate>>& inputs2,
    InternalKey* smallest, InternalKey* largest) {
  std::vector<std::shared_ptr<FileMate>> all = inputs1;
  all.insert(all.end(), inputs2.begin(), inputs2.end());
  GetRange(all, smallest, largest);
}
void VersionSet::SetupOtherInputs(
    std::unique_ptr<Compaction>&
        cop) {  //尽可能扩大n的文件，确定n+1之后用，n+1的
                // large和其他n的largest 比较确定范围，扩大n
  InternalKey smallest, largest;
  GetRange(cop->inputs_[0], &smallest, &largest);
  nowversion->GetOverlappFiles(cop->level_ + 1, &smallest, &largest,
                               &cop->inputs_[1]);

  InternalKey all_start, all_limit;
  GetRange2(cop->inputs_[0], cop->inputs_[1], &all_start,
            &all_limit);  //在 n和n+1中确定范围

  if (!cop->inputs_[1].empty()) {
    std::vector<std::shared_ptr<FileMate>> expand;
    nowversion->GetOverlappFiles(cop->level_, &all_start, &all_limit, &expand);
    const int64_t inputs0_size = TotalFileSize(cop->inputs_[0]);
    const int64_t inputs1_size = TotalFileSize(cop->inputs_[1]);
    const int64_t expand_size = TotalFileSize(expand);
    if (expand.size() > cop->inputs_[0].size()) {
      InternalKey new_start, new_limit;
      GetRange(expand, &new_start, &new_limit);
      std::vector<std::shared_ptr<FileMate>> expanded1;
      // 取得level+1中与新的level中的输入文件overlap的文件
      nowversion->GetOverlappFiles(cop->level_ + 1, &new_start, &new_limit,
                                   &expanded1);
      if (expanded1.size() == cop->inputs_[1].size()) {
        spdlog::info(
            "Expanding@LEVEL{} FileNum: {} + {} ({} + {} bytes) to Expend "
            "FileNum {} + {} ({} + {} bytes)",
            cop->level_, int(cop->inputs_[0].size()),
            int(cop->inputs_[1].size()), long(inputs0_size), long(inputs1_size),
            int(expand.size()), int(expanded1.size()), long(expand_size),
            long(inputs1_size));
        smallest = new_start;
        largest = new_limit;
        cop->inputs_[0] = expand;
        cop->inputs_[1] = expanded1;
        GetRange2(cop->inputs_[0], cop->inputs_[1], &all_start, &all_limit);
      }
    }
  }
  if (cop->level_ + 2 < config::kNumLevels) {
    nowversion->GetOverlappFiles(
        cop->level_ + 2, &all_start, &all_limit,
        &cop->grandparents_);  // 取得level+2 中重叠的文件
    compact_pointer[cop->level_] = largest.getString();
    cop->edit_.SetCompactPointer(cop->level_, largest);
  }
}
std::unique_ptr<Merageitor> VersionSet::MakeInputIterator(Compaction* c) {
  ReadOptions options;
  const int space = (c->Level() == 0 ? c->inputs_[0].size() + 1 : 2);
  Iterator** list = new Iterator*[space];
  int num = 0;
  for (int i = 0; i < 2; i++) {
    if (c->inputs_[i].empty()) {
      if (c->Level() + i == 0) {
        const std::vector<std::shared_ptr<FileMate>>& files = c->inputs_[i];
        for (size_t i = 0; i < files.size(); i++) {
          list[num++] = table_cache->NewIterator(options, files[i]->num,
                                                 files[i]->file_size);
        }
      } else {
        list[num++];
      }
    }
  }
}
Compaction::Compaction(const Options* options, int level)
    : level_(level),
      max_output_file_size_(options->max_file_size),
      input_version_(nullptr),
      grand_index(0),
      seen_key(false),
      overlapbytes(0) {
  for (int i = 0; i < config::kNumLevels; i++) {
    level_ptrs[i] = 0;
  }
}
bool Compaction::IsTrivialMove() {
  return (inputs_[0].size() == 1 && inputs_[1].size() == 0 &&
          TotalFileSize(grandparents_) <=
              input_version_->vset->ops->max_file_size * 10);
}
}  // namespace yubindb