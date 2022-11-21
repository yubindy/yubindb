#ifndef YUBINDB_ITERATOR_H_
#define YUBINDB_ITERATOR_H_
#include <string_view>
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
  virtual std::string_view key() const = 0;
  virtual std::string_view value() const = 0;
};
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