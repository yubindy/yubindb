#ifndef YUBINDB_SNAPSHOT_H_
#define YUBINDB_SHAPSHOT_H_
#include "../util/key.h"
namespace yubindb {
class Snapshot {
 public:
  explicit Snapshot(SequenceNum seq) : snap_seq(seq) {}
  ~Snapshot() = default;
  SequenceNum sequence() const { return snap_seq; }

 private:
  friend class SnapshotList;
  const SequenceNum snap_seq;
};
}  // namespace yubindb
#endif
