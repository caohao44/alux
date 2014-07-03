#ifndef __MEMORY_WIRED_HPP__
#define __MEMORY_WIRED_HPP__

#include <address.h>
#include <cstddef>
#include <cstdint>

namespace std {

class WiredMemory {
  static uint64_t PhysicalUsed();
  static uint64_t PhysicalAvailable();
  static uint64_t PhysicalTotal();
  static int GetPageSizeCount();
  static uint64_t GetPageSize(int idx);
  static uint64_t GetPageAlignment(int idx);
  
  static bool Allocate(PhysAddr & res, uint64_t size, uint64_t align);
  static void Free(PhysAddr addr);
  
  static bool Map(void *& res, PhysAddr start, uint64_t pageSize,
                  uint64_t pageCount, bool exec = true, bool write = true);
  static void Unmap(void * virt, uint64_t pageSize, uint64_t pageCount);
};

}

#endif
