#ifndef YUBINDB_ARENA_H_
#define YUBINDB_ARENA_H_
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace yubindb {
static const int kBlockSize = 4096;
class Arena {
 public:
  Arena() : alloc_ptr(nullptr), alloc_remaining(0), memory_usage(0) {}
  ~Arena() {
    for (size_t i = 0; i < blocks.size(); i++) {
      delete[] blocks[i];
    }
  }
  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;
  char* Alloc(size_t size);
  char* AllocAligned(size_t size);
  size_t MemoryUsage() const {
    return memory_usage.load(std::memory_order_relaxed);
  }

 private:
  char* AllocateFallback(size_t bytes);
  char* AllocateNewBlock(size_t block_bytes);

  char* alloc_ptr;         // now block
  size_t alloc_remaining;  // now block

  // Array of new[] allocated memory blocks
  std::vector<char*> blocks;  // for delete
  std::atomic<size_t> memory_usage;
};
}  // namespace yubindb
#endif