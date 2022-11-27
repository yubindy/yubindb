#ifndef YUBINDB_ITERATOR_H_
#define YUBINDB_ITERATOR_H_


#include <cstddef>
#include <memory>
#include <string_view>

#include "../util/options.h"
#include "src/util/common.h"
namespace yubindb {
class Iterator {
 public:
  Iterator()=default;

  Iterator(const Iterator&) = delete;
  Iterator& operator=(const Iterator&) = delete;
  virtual ~Iterator()=default;
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
  explicit IteratorWrapper(std::shared_ptr<Iterator>& iter) {
    Set(iter);
    iter = nullptr;
  }
  ~IteratorWrapper() {}
  std::shared_ptr<Iterator> iter() const { return iter_; }

  void Set(std::shared_ptr<Iterator>& iter) {
    iter_ = iter;
    if (iter_ == nullptr) {
      valid_ = false;
    } else {
      Update();
    }
  }
  void clear() {
    iter_ = nullptr;
    valid_ = false;
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

  std::shared_ptr<Iterator> iter_;
  bool valid_;
  std::string_view key_;
};
typedef std::shared_ptr<Iterator> (*BlockFunction)(void*, const ReadOptions&,
                                                   std::string_view);
class TweLevelIterator : public Iterator {
 public:
  TweLevelIterator(std::shared_ptr<Iterator>& index_iter_,
                   BlockFunction block_function, void* arg,
                   const ReadOptions& options)
      : block_function_(block_function),
        arg_(arg),
        options_(options),
        index_iter(index_iter_),
        data_iter() {}

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
  void clear();
  void SkipEmptyDataBlocksForward();
  void SkipEmptyDataBlocksBackward();
  void SetDataIterator(std::shared_ptr<Iterator>& data_iter);
  void InitDataBlock();
  BlockFunction block_function_;
  void* arg_;
  const ReadOptions options_;
  State s;
  IteratorWrapper index_iter;
  IteratorWrapper data_iter;
  std::string data_block_handle;
};
std::shared_ptr<Iterator> GetFileIterator(void* arg,
                                                 const ReadOptions& options,
                                                 std::string_view file_value);
std::shared_ptr<Iterator> NewTwoLevelIterator(
    std::shared_ptr<Iterator> index_iter,
    std::shared_ptr<Iterator> (*block_function)(void* arg,
                                                const ReadOptions& options,
                                                std::string_view index_value),
    void* arg, const ReadOptions& options);
class Merageitor : public Iterator {
 public:
  Merageitor(std::shared_ptr<Iterator>* children, int n)
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
std::shared_ptr<Iterator> NewMergingIterator(
    std::shared_ptr<Iterator>* children, int n);
}  // namespace yubindb
#endif