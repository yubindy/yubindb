#include "iterator.h"

#include <cstddef>
#include <string_view>

#include "cache.h"
namespace yubindb {
std::shared_ptr<Iterator> GetFileIterator(void* arg, const ReadOptions& options,
                                          std::string_view file_value) { //TODO should fix ?
  TableCache* cache = reinterpret_cast<TableCache*>(arg);
  if (file_value.size() != 16) {
    mlog->error("FileReader invoked with unexpected value");
    return nullptr;
  } else {
    return cache->NewIterator(options, DecodeFixed64(file_value.data()),
                              DecodeFixed64(file_value.data() + 8));
  }
}
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
      clear();
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
      clear();
      return;
    }
    index_iter.Prev();
    InitDataBlock();
    if (data_iter.iter() != nullptr) data_iter.SeekToLast();
  }
}
void TweLevelIterator::SetDataIterator(std::shared_ptr<Iterator>& data_iter_) {
  if (data_iter.iter() != nullptr) {
    data_iter.Set(data_iter_);
    mlog->error("SetDataIterator error");
  }
}
void TweLevelIterator::clear() {
  if (data_iter.iter() != nullptr) {
    data_iter.clear();
  }
}
void TweLevelIterator::InitDataBlock() {
  if (!index_iter.Valid()) {
    clear();
  } else {
    std::string_view handle = index_iter.value();
    if (data_iter.iter() != nullptr && handle.compare(data_block_handle) == 0) {
    } else {
      std::shared_ptr<Iterator> iter =
          (*block_function_)(arg_, options_, handle);
      data_block_handle.assign(handle.data(), handle.size());
      SetDataIterator(iter);
    }
  }
}
std::shared_ptr<Iterator> NewTwoLevelIterator(
    std::shared_ptr<Iterator> index_iter,
    std::shared_ptr<Iterator> (*block_function)(void* arg,
                                                const ReadOptions& options,
                                                std::string_view index_value),
    void* arg, const ReadOptions& options) {
  return std::make_shared<TweLevelIterator>(index_iter, block_function, arg,
                                            options);
}
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

std::shared_ptr<Iterator> NewMergingIterator(
    std::shared_ptr<Iterator>* children, int n) {
  if (n == 0) {
    return nullptr;
  } else if (n == 1) {
    return children[0];
  } else {
    return std::make_shared<Merageitor>(children, n);
  }
}
}  // namespace yubindb