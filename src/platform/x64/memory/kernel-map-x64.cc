#include "kernel-map-x64.hpp"

namespace OS {

namespace x64 {

static const VirtAddr ScratchStartAddr = 0x7FC0000000L;
static KernelMap globalMap;
static bool hasInitialized = false;

KernelMap & KernelMap::GetGlobal() {
  if (!hasInitialized) {
    hasInitialized = true;
    new(&globalMap) KernelMap();
  }
  return globalMap;
}

KernelMap::KernelMap() : manager() {
  scratchLock = 0;
  bzero(scratchBitmaps, sizeof(scratchBitmaps));
}

void KernelMap::Setup() {
  MapSetup setup(allocator);
  setup.Map();
  
  assert(ScratchPTCount <= 0x200);
  assert(setup.GetFirstUnmapped() <= ScratchStartAddr);
  
  PhysAddr scratchPDT = allocator->AllocPage();
  bzero((void *)scratchPDT, 0x1000);
  
  ((uint64_t *)setup.GetPDPT())[0x1ff] = scratchPDT | 3;
  uint64_t scratchStart = (uint64_t)ScratchPTStart();
  for (int i = 0; i < ScratchPTCount; i++) {
    PhysAddr scratchPT = scratchStart + (i << 12);
    ((uint64_t *)scratchPDT)[i] = scratchPT | 3;
  }
  
  TableMgr man(setup.GetPML4());
  manager = man;
}

PhysAddr KernelMap::GetPML4() {
  return manager.GetPML4();
}

void KernelMap::Set() {
  manager.Set();
}

VirtAddr KernelMap::Map(PhysAddr start, size_t size, bool largePages) {
  assert(!(size & (largePages ? 0x1fffff : 0xfff)));
  assert(!(start & (largePages ? 0x1fffff : 0xfff)));
  
  ScopeLock scope(&mapLock);
  
  // see if we can find a place in our biggest unused
  if (!CanFitRegion(size, largePages)) {
    manager.FindNewBU(buStart, buSize, ScratchStartAddr);
    if (!CanFitRegion(size, largePages)) {
      return 0;
    }
  }
  
  // align our BU
  if (largePages) {
    buSize -= 0x200000 - (buStart & 0x1fffff);
    buStart += 0x200000 - (buStart & 0x1fffff);
  }
  
  // map starting at buStart
  uint64_t flags = 0x103 | (largePages ? 0x80 : 0);
  manager.Map(buStart, start, size, largePages, flags, 3);
  VirtAddr result = buStart;
  buStart += size;
  buSize -= size;
  
  return result;
}

void KernelMap::MapAt(VirtAddr virt, PhysAddr start,
                      size_t size, bool largePages) {
  ScopeLock scope(&mapLock);
  uint64_t flags = 0x103 | (largePages ? 0x80 : 0);
  manager.Map(virt, start, size, largePages, flags, 3);
  
  // if we are in the BU, we need to modify the BU.
  if (start >= buStart && start < buStart + buSize) {
    if (start + size >= buStart + buSize) {
      buStart = 0;
      buSize = 0;
    } else {
      buSize = buStart + buSize - (start + size);
      buStart = start + size;
    }
  }
}

void KernelMap::ClearMap(VirtAddr virt, size_t size) {
  ScopeLock scope(&mapLock);
  manager.ClearMap(virt, size);
}

void KernelMap::Unmap(VirtAddr virt, size_t size) {
  ScopeLock scope(&mapLock);
  manager.Unmap(virt, size);
  
  // see what we should do about the biggest unmapped region
  if (size > buSize) {
    buStart = virt;
    buSize = size;
  } else if (buStart + buSize == virt) {
    buSize += size;
  } else if (virt + size == buStart) {
    buSize += size;
    buStart -= size;
  }
}

VirtAddr KernelMap::AllocScratch(PhysAddr start) {
  ScopeLock scope(&scratchLock);
  
  assert(!(start & 0xfff));
  
  int bitIndex = -1;
  for (int i = 0; i < ScratchPTCount * 8; i++) {
    if (!~scratchBitmaps[i]) continue;
    // find the first free NULL
    for (int j = 0; j < 0x40; j++) {
      if (scratchBitmaps[i] & (1L << j)) continue;
      
      bitIndex = j + (i * 0x40);
      scratchBitmaps[i] |= (1L << j);
      break;
    }
    break;
  }
  if (bitIndex < 0) return 0;
  
  VirtAddr virt = ScratchStartAddr + (bitIndex << 12);
  int scratchTableIdx = bitIndex >> 9;
  uint64_t * table = (uint64_t *)((uint64_t)ScratchPTStart()
    + (scratchTableIdx << 12));
  table[bitIndex & 0x1ff] = start | 3;
  __asm__("invlpg (%0)" : : "r" (virt));
  
  return virt;
}

void KernelMap::ReassignScratch(VirtAddr addr, PhysAddr newAddr) {
  int bitIndex = (int)((addr - ScratchStartAddr) >> 12);
  int scratchTableIdx = bitIndex >> 9;
  uint64_t * table = (uint64_t *)((uint64_t)ScratchPTStart()
    + (scratchTableIdx << 12));
  table[bitIndex & 0x1ff] = newAddr | 3;
  __asm__("invlpg (%0)" : : "r" (addr));
}

void KernelMap::FreeScratch(VirtAddr ptr) {
  ScopeLock scope(&scratchLock);
  
  int bitIndex = (int)((ptr - ScratchStartAddr) >> 12);
  int fieldIndex = bitIndex / 0x40;
  scratchBitmaps[fieldIndex] ^= 1L << (bitIndex & 0x3f);
}

/***********
 * PRIVATE *
 ***********/

bool KernelMap::CanFitRegion(size_t size, bool bigPages) {
  if (!bigPages) {
    return size <= buSize;
  }
  if (buSize < 0x200000) return false;
  size_t realSize = buSize - (0x200000 - (buStart & 0x1fffff));
  return size <= realSize;
}

}

}
