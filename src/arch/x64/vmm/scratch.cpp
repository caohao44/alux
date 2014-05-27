#include <arch/x64/vmm/scratch.hpp>
#include <arch/x64/vmm/kernel-layout.hpp>
#include <utilities/lock.hpp>
#include <new>
#include <cassert>
#include <cstring>

namespace OS {

namespace x64 {

static Scratch globalScratch;

void Scratch::Initialize(PhysAddr pdpt, PageAllocator & alloc) {
  new(&globalScratch) Scratch();
  globalScratch.Setup(pdpt, alloc);
}

Scratch & Scratch::GetGlobal() {
  return globalScratch;
}
  
VirtAddr Scratch::Alloc(PhysAddr page) {
  assert(!(page & 0xfff));
  ScopeCriticalLock scope(&lock);
  
  int bitIndex = -1;
  for (int i = 0; i < PTCount * 0x200; i++) {
    if (!GetBitmap(i)) {
      FlipBitmap(i);
      bitIndex = i;
      break;
    }
  }
  if (bitIndex < 0) return 0;
  
  uint64_t * table = PageTableAddress(bitIndex);
  VirtAddr virt = ScratchAddress(bitIndex);
  table[bitIndex & 0x1ff] = page | 3;
  __asm__ volatile("invlpg (%0)" : : "r" (virt) : "memory");
  return virt;
}

void Scratch::Reassign(VirtAddr addr, PhysAddr newAddr) {
  assert(!(addr & 0xfff));
  assert(!(newAddr & 0xfff));
  ScopeCriticalLock scope(&lock);
  
  int bitIndex = (int)((addr - StartAddr) / 0x1000);
  uint64_t * table = PageTableAddress(bitIndex);
  table[bitIndex & 0x1ff] = newAddr | 3;
  __asm__ volatile("invlpg (%0)" : : "r" (addr) : "memory");
}

void Scratch::Free(VirtAddr addr) {
  ScopeCriticalLock scope(&lock);
  
  int bitIndex = (int)((addr - StartAddr) / 0x1000);
  assert(GetBitmap(bitIndex));
  FlipBitmap(bitIndex);
}

void Scratch::Setup(PhysAddr pdpt, PageAllocator & alloc) {
  PhysAddr scratchPDT = alloc.AllocPage();
  bzero((void *)scratchPDT, 0x1000);
  
  // put the scratch PDT into the kernel PDPT
  ((uint64_t *)pdpt)[0x1ff] = scratchPDT | 3;
  
  // setup the page tables and put them in the scratch PDT
  uint64_t scratchStart = ScratchPTStart();
  for (int i = 0; i < PTCount; i++) {
    PhysAddr scratchPT = scratchStart + (0x1000 * i);
    ((uint64_t *)scratchPDT)[i] = scratchPT | 3;
  }
}

bool Scratch::GetBitmap(int idx) {
  return (bitmaps[idx / 0x40] & (1L << (idx & 0x3f))) != 0;
}

void Scratch::FlipBitmap(int idx) {
  bitmaps[idx / 0x40] ^= (1L << (idx & 0x3f));
}

VirtAddr Scratch::ScratchPTStart() {
  return KernelSize() - (0x1000 * PTCount);
}

VirtAddr Scratch::ScratchAddress(int cellIndex) {
  return StartAddr + (cellIndex << 12);
}

uint64_t * Scratch::PageTableAddress(int cellIndex) {
  return (uint64_t *)(ScratchPTStart() + ((cellIndex >> 9) * 0x1000));
}

}

}