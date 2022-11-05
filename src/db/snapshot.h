#ifndef YUBINDB_SNAPSHOT_H_
#define YUBINDB_SHAPSHOT_H_
#include "../util/key.h"
namespace yubindb {
class Snapshot {
 public:
  Snapshot() = default;
  virtual ~Snapshot();
};
class SnapshotImpl : public Snapshot {
 public:
  explicit SnapshotImpl(SequenceNum seq) : snap_seq(seq) {}
  ~SnapshotImpl();
  SequenceNum sequence() const { return snap_seq; }

 private:
  friend class SnapshotList;
  const SequenceNum snap_seq;
};
}  // namespace yubindb
#endif