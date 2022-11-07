#include "version_set.h"
namespace yubindb {
int VersionSet::NumLevelFiles(int level) const {
  return nowversion->files[level].size();
}
}  // namespace yubindb