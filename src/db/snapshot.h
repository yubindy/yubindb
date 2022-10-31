#ifndef YUBINDB_SNAPSHOT_H_
#define YUBINDB_SHAPSHOT_H_
namespace yubindb {
class Snapshot {
  Snapshot() = default;
  virtual ~Snapshot();
};
class SnapshotImpl : public Snapshot {
  explicit SnapshotImpl() = default;
  ~SnapshotImpl();
};
}  // namespace yubindb
#endif