#ifndef YUBINDB_ITERATOR_H_
#define YUBINDB_ITERATOR_H_
#include <spdlog/spdlog.h>

#include <string_view>

#include "options.h"
#include "src/util/common.h"
namespace yubindb {
class Iterator {
 public:
  Iterator();

  Iterator(const Iterator&) = delete;
  Iterator& operator=(const Iterator&) = delete;
  virtual ~Iterator();
  virtual bool Valid() const = 0;
  virtual void SeekToFirst() = 0;
  virtual void SeekToLast() = 0;
  virtual void Seek(std::string_view target) = 0;
  virtual void Next() = 0;
  virtual void Prev() = 0;
  virtual State state() = 0;
  virtual std::string_view key() const = 0;
  virtual std::string_view value() const = 0;
};
typedef Iterator* (*BlockFunction)(void*, const ReadOptions&, std::string_view);
class TweLevelIterator : public Iterator {
 public:
  TweLevelIterator(Iterator* index_iter, BlockFunction block_function,
                   void* arg, const ReadOptions& options)
      : block_function_(block_function),
        arg_(arg),
        options_(options),
        index_iter(index_iter),
        data_iter(nullptr) {}

  ~TweLevelIterator() override{};

  void Seek(std::string_view target);
  void SeekToFirst() override;
  void SeekToLast() override;
  void Next() override;
  void Prev() override;
  std::string_view key() const override {
    assert(Valid());
    return data_iter->key();
  }
  std::string_view value() const override {
    assert(Valid());
    return data_iter->value();
  }
  bool Valid() const override { return data_iter->Valid(); }
  State state() override {
    // It'd be nice if status() returned a const Status& instead of a Status
    if (!index_iter->state().ok()) {
      return index_iter->state();
    } else if (data_iter != nullptr && !data_iter->state().ok()) {
      return data_iter->state();
    } else {
      return s;
    }
  }

 private:
  void SkipEmptyDataBlocksForward();
  void SkipEmptyDataBlocksBackward();
  void SetDataIterator(Iterator* data_iter);
  void InitDataBlock();
  BlockFunction block_function_;
  void* arg_;
  const ReadOptions options_;
  State s;
  Iterator* index_iter;
  Iterator* data_iter;
  std::string_view index;
  std::string_view data;
  std::string data_block_handle;
};
void TweLevelIterator::Seek(std::string_view target) {
  index_iter->Seek(target);
  InitDataBlock();
  if (data_iter != nullptr) data_iter->Seek(target);
  SkipEmptyDataBlocksForward();
}
void TweLevelIterator::SeekToFirst() {
  index_iter->SeekToFirst();
  InitDataBlock();
  if (data_iter != nullptr) data_iter->SeekToFirst();
  SkipEmptyDataBlocksForward();
}
void TweLevelIterator::SeekToLast() {
  index_iter->SeekToLast();
  InitDataBlock();
  if (data_iter != nullptr) data_iter->SeekToLast();
  SkipEmptyDataBlocksBackward();
}
void TweLevelIterator::Next() {
  assert(Valid());
  data_iter->Next();
  SkipEmptyDataBlocksForward();
}

void TweLevelIterator::Prev() {
  assert(Valid());
  data_iter->Prev();
  SkipEmptyDataBlocksBackward();
}
void TweLevelIterator::SkipEmptyDataBlocksForward() {
  while (data_iter == nullptr || !data_iter->Valid()) {
    // Move to next block
    if (!index_iter->Valid()) {
      SetDataIterator(nullptr);
      return;
    }
    index_iter->Next();
    InitDataBlock();
    if (data_iter != nullptr) data_iter->SeekToFirst();
  }
}

void TweLevelIterator::SkipEmptyDataBlocksBackward() {
  while (data_iter == nullptr || !data_iter->Valid()) {
    if (!index_iter->Valid()) {
      SetDataIterator(nullptr);
      return;
    }
    index_iter->Prev();
    InitDataBlock();
    if (data_iter != nullptr) data_iter->SeekToLast();
  }
}
void TweLevelIterator::SetDataIterator(Iterator* data_iter_) {
  if (data_iter != nullptr) {
    delete data_iter;
    data_iter = data_iter;
    if (data_iter->Valid()) {
      index = data_iter->key();
    } else {
      spdlog::error("SetDataIterator error");
    }
  }
}
void TweLevelIterator::InitDataBlock() {
  if (!index_iter->Valid()) {
    SetDataIterator(nullptr);
  } else {
    std::string_view handle = index_iter->value();
    if (data_iter != nullptr && handle.compare(data_block_handle_) == 0) {
      // data_iter_ is already constructed with this iterator, so
      // no need to change anything
    } else {
      Iterator* iter = (*block_function_)(arg_, options_, handle);
      data_block_handle_.assign(handle.data(), handle.size());
      SetDataIterator(iter);
    }
  }
}
Iterator* NewTwoLevelIterator(
    Iterator* index_iter,
    Iterator* (*block_function)(void* arg, const ReadOptions& options,
                                std::string_view index_value),
    void* arg, const ReadOptions& options) {}

class Merageitor {
 public:
 private:
};
// Iterator* NewMergingIterator(Iterator** children, int n) {
//   if (n == 0) {
//     return NewEmptyIterator();
//   } else if (n == 1) {
//     return children[0];
//   } else {
//     return new Merageitor(children,n);
//   }
// }
}  // namespace yubindb
#endif