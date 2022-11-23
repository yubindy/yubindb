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
class IteratorWrapper {
 public:
  IteratorWrapper() : iter_(nullptr), valid_(false) {}
  explicit IteratorWrapper(Iterator* iter) : iter_(nullptr) { Set(iter); }
  ~IteratorWrapper() { delete iter_; }
  Iterator* iter() const { return iter_; }

  void Set(Iterator* iter) {
    delete iter_;
    iter_ = iter;
    if (iter_ == nullptr) {
      valid_ = false;
    } else {
      Update();
    }
  }

  // Iterator interface methods
  bool Valid() const { return valid_; }
  std::string_view key() const {
    assert(Valid());
    return key_;
  }
  std::string_view value() const {
    assert(Valid());
    return iter_->value();
  }
  State state() const {
    assert(iter_);
    return iter_->state();
  }
  void Next() {
    assert(iter_);
    iter_->Next();
    Update();
  }
  void Prev() {
    assert(iter_);
    iter_->Prev();
    Update();
  }
  void Seek(std::string_view k) {
    assert(iter_);
    iter_->Seek(k);
    Update();
  }
  void SeekToFirst() {
    assert(iter_);
    iter_->SeekToFirst();
    Update();
  }
  void SeekToLast() {
    assert(iter_);
    iter_->SeekToLast();
    Update();
  }

 private:
  void Update() {
    valid_ = iter_->Valid();
    if (valid_) {
      key_ = iter_->key();
    }
  }

  Iterator* iter_;
  bool valid_;
  std::string_view key_;
};
typedef Iterator* (*BlockFunction)(void*, const ReadOptions&, std::string_view);
class TweLevelIterator : public Iterator {
 public:
  TweLevelIterator(Iterator* index_iter_, BlockFunction block_function,
                   void* arg, const ReadOptions& options)
      : block_function_(block_function),
        arg_(arg),
        options_(options),
        index_iter(index_iter_),
        data_iter(nullptr) {}

  ~TweLevelIterator() override{};

  void Seek(std::string_view target);
  void SeekToFirst() override;
  void SeekToLast() override;
  void Next() override;
  void Prev() override;
  std::string_view key() const override {
    assert(Valid());
    return data_iter.key();
  }
  std::string_view value() const override {
    assert(Valid());
    return data_iter.value();
  }
  bool Valid() const override { return data_iter.Valid(); }
  State state() override {
    if (!index_iter.state().ok()) {
      return index_iter.state();
    } else if (data_iter.iter() != nullptr && !data_iter.state().ok()) {
      return data_iter.state();
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
  IteratorWrapper index_iter;
  IteratorWrapper data_iter;
  std::string data_block_handle;
};
void TweLevelIterator::Seek(std::string_view target) {
  index_iter.Seek(target);
  InitDataBlock();
  if (data_iter.iter() != nullptr) data_iter.Seek(target);
  SkipEmptyDataBlocksForward();
}
void TweLevelIterator::SeekToFirst() {
  index_iter.SeekToFirst();
  InitDataBlock();
  if (data_iter.iter() != nullptr) data_iter.SeekToFirst();
  SkipEmptyDataBlocksForward();
}
void TweLevelIterator::SeekToLast() {
  index_iter.SeekToLast();
  InitDataBlock();
  if (data_iter.iter() != nullptr) data_iter.SeekToLast();
  SkipEmptyDataBlocksBackward();
}
void TweLevelIterator::Next() {
  assert(Valid());
  data_iter.Next();
  SkipEmptyDataBlocksForward();
}

void TweLevelIterator::Prev() {
  assert(Valid());
  data_iter.Prev();
  SkipEmptyDataBlocksBackward();
}
void TweLevelIterator::SkipEmptyDataBlocksForward() {
  while (data_iter.iter() == nullptr || !data_iter.Valid()) {
    if (!index_iter.Valid()) {
      SetDataIterator(nullptr);
      return;
    }
    index_iter.Next();
    InitDataBlock();
    if (data_iter.iter() != nullptr) data_iter.SeekToFirst();
  }
}

void TweLevelIterator::SkipEmptyDataBlocksBackward() {
  while (data_iter.iter() == nullptr || !data_iter.Valid()) {
    if (!index_iter.Valid()) {
      SetDataIterator(nullptr);
      return;
    }
    index_iter.Prev();
    InitDataBlock();
    if (data_iter.iter() != nullptr) data_iter.SeekToLast();
  }
}
void TweLevelIterator::SetDataIterator(Iterator* data_iter_) {
  if (data_iter.iter() != nullptr) {
    data_iter.Set(data_iter_);
    spdlog::error("SetDataIterator error");
  }
}
void TweLevelIterator::InitDataBlock() {
  if (!index_iter.Valid()) {
    SetDataIterator(nullptr);
  } else {
    std::string_view handle = index_iter.value();
    if (data_iter.iter() != nullptr && handle.compare(data_block_handle) == 0) {
    } else {
      Iterator* iter = (*block_function_)(arg_, options_, handle);
      data_block_handle.assign(handle.data(), handle.size());
      SetDataIterator(iter);
    }
  }
}
static Iterator* GetFileIterator(void* arg, const ReadOptions& options,
                                 std::string_view file_value);
Iterator* NewTwoLevelIterator(
    Iterator* index_iter,
    Iterator* (*block_function)(void* arg, const ReadOptions& options,
                                std::string_view index_value),
    void* arg, const ReadOptions& options) {
  return new TweLevelIterator(index_iter, block_function, arg, options);
}

class Merageitor : public Iterator {
 public:
  Merageitor(Iterator** children, int n)
      : children_(new IteratorWrapper[n]),
        n_(n),
        current_(nullptr),
        direction_(kForward) {
    for (int i = 0; i < n; i++) {
      children_[i].Set(children[i]);
    }
  }

  ~Merageitor() override { delete[] children_; }

  bool Valid() const override { return (current_ != nullptr); }

  void SeekToFirst() override {
    for (int i = 0; i < n_; i++) {
      children_[i].SeekToFirst();
    }
    FindSmallest();
    direction_ = kForward;
  }

  void SeekToLast() override {
    for (int i = 0; i < n_; i++) {
      children_[i].SeekToLast();
    }
    FindLargest();
    direction_ = kReverse;
  }

  void Seek(std::string_view target) override {
    for (int i = 0; i < n_; i++) {
      children_[i].Seek(target);
    }
    FindSmallest();
    direction_ = kForward;
  }

  void Next() override {
    assert(Valid());
    if (direction_ != kForward) {
      for (int i = 0; i < n_; i++) {
        IteratorWrapper* child = &children_[i];
        if (child != current_) {
          child->Seek(key());
          if (child->Valid() && cmp(key(), child->key()) == 0) {
            child->Next();
          }
        }
      }
      direction_ = kForward;
    }

    current_->Next();
    FindSmallest();
  }

  void Prev() override {
    assert(Valid());
    if (direction_ != kReverse) {
      for (int i = 0; i < n_; i++) {
        IteratorWrapper* child = &children_[i];
        if (child != current_) {
          child->Seek(key());
          if (child->Valid()) {
            child->Prev();
          } else {
            child->SeekToLast();
          }
        }
      }
      direction_ = kReverse;
    }

    current_->Prev();
    FindLargest();
  }

  std::string_view key() const override {
    assert(Valid());
    return current_->key();
  }

  std::string_view value() const override {
    assert(Valid());
    return current_->value();
  }

  State state() override {
    State s;
    for (int i = 0; i < n_; i++) {
      s = children_[i].state();
      if (!s.ok()) {
        break;
      }
    }
    return s;
  }

 private:
  enum Direction { kForward, kReverse };

  void FindSmallest();
  void FindLargest();

  IteratorWrapper* children_;
  int n_;
  IteratorWrapper* current_;
  Direction direction_;
};

void Merageitor::FindSmallest() {
  IteratorWrapper* smallest = nullptr;
  for (int i = 0; i < n_; i++) {
    IteratorWrapper* child = &children_[i];
    if (child->Valid()) {
      if (smallest == nullptr) {
        smallest = child;
      } else if (cmp(child->key(), smallest->key()) < 0) {
        smallest = child;
      }
    }
  }
  current_ = smallest;
}

void Merageitor::FindLargest() {
  IteratorWrapper* largest = nullptr;
  for (int i = n_ - 1; i >= 0; i--) {
    IteratorWrapper* child = &children_[i];
    if (child->Valid()) {
      if (largest == nullptr) {
        largest = child;
      } else if (cmp(child->key(), largest->key()) > 0) {
        largest = child;
      }
    }
  }
  current_ = largest;
}

Iterator* NewMergingIterator(Iterator** children, int n) {
  if (n == 0) {
    return nullptr;
  } else if (n == 1) {
    return children[0];
  } else {
    return new Merageitor(children, n);
  }
}
}  // namespace yubindb
#endif