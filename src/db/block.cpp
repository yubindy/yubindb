#include "block.h"

#include "src/util/common.h"
#include "src/util/options.h"
namespace yubindb {
void Blockbuilder::Reset() {
  buffer.clear();
  restarts.clear();
  restarts.push_back(0);
  entrynum = 0;
  finished = false;
  last_key.clear();
}
size_t Blockbuilder::CurrentSizeEstimate() const {
  return (buffer.size() +                       // Raw data buffer
          restarts.size() * sizeof(uint32_t) +  // Restart array
          sizeof(uint32_t));                    // Restart array length
}
void Blockbuilder::Add(std::string_view key, std::string_view value) {
  std::string last_key_view(last_key);

  size_t shared = 0;
  if (entrynum < opt->block_restart_interval) {
    const size_t minlen = std::min(last_key.size(), key.size());
    while ((shared < minlen) && (last_key_view[shared] == key[shared])) {
      shared++;
    }
  } else {
    restarts.emplace_back(buffer.size());  // restart 前数据块偏移
    entrynum = 0;
  }
  const size_t non_shared = key.size() - shared;

  // Add "<shared><non_shared><value_size>" to buffer
  PutVarint32(&buffer, shared);
  PutVarint32(&buffer, non_shared);
  PutVarint32(&buffer, value.size());

  buffer.append(key.data() + shared, non_shared);
  buffer.append(value.data(), value.size());

  // Update state
  last_key.resize(shared);
  last_key.append(key.data() + shared, non_shared);
  assert(std::string_view(last_key) == key);
  entrynum++;
}
std::string_view Blockbuilder::Finish(){
    for(size_t i=0;i<restarts.size();i++){
        PutFixed32(&buffer,restarts[i]);
    }
    PutFixed32(&buffer,restarts.size());
    finished =true;
    return std::string(buffer);
}

};  // namespace yubindb