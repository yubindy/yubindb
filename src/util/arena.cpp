#include "arena.h"
namespace yubindb {
char* Arena::Alloc(size_t size) {
  assert(size > 0);
  if (size < alloc_remaining) {
    char* t = alloc_ptr;
    alloc_ptr += size;
    alloc_remaining -= size;
    return t;
  }
  return AllocateFallback(size);
}
char* Arena::AllocateFallback(size_t bytes) {
  if (bytes > BlockSize / 4) {
    return AllocateNewBlock(bytes);
  }
  alloc_ptr = AllocateNewBlock(bytes);
  alloc_remaining = BlockSize;

  char* rul = alloc_ptr;
  alloc_ptr += bytes;
  alloc_remaining -= bytes;
  return rul;
}
char* Arena::AllocateNewBlock(size_t block_bytes) {
  char* rul = new char[block_bytes];
  blocks.emplace_back(rul);
  memory_usage.fetch_add(block_bytes + sizeof(char*),
                         std::memory_order_relaxed);
  return rul;
}
char* Arena::AllocAligned(size_t size) {
  assert(size > 0);
  const int align = sizeof(char*);
  size_t current_mod = reinterpret_cast<uintptr_t>(alloc_ptr) & (align - 1);
  size_t shouldadd = (current_mod > 0 ? align - current_mod : 0);
  size_t needsize = size + shouldadd;
  char rul;
  if (size < alloc_remaining) {
    char* t = alloc_ptr + shouldadd;
    alloc_ptr += size;
    alloc_remaining -= size;
    return t;
  } else {
    return AllocateFallback(size);
  }
}
}  // namespace yubindb