/*
 * Copyright (c) 2014, Alex Nichol and Alux contributors.
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "global-memory-x64.h"

namespace OS {

static GlobalMap map;

/******************
 * PhysRegionList *
 ******************/

void PhysRegionList::AddRegion(MemoryRegion & region) {
  if (regionCount == MaximumPhysicalRegions) {
    Panic("GlobalMap::AddRegion() - region overflow");
  }
  
  int insertIndex = regionCount;
  for (int i = 0; i < regionCount; i++) {
    if (regions[i].GetStart() > region.GetStart()) {
      insertIndex = i;
      break;
    }
  }
  for (int i = regionCount; i > insertIndex; i--) {
    regions[i] = regions[i - 1];
  }
  regions[insertIndex] = region;
  regionCount++;
}

PhysRegionList::PhysRegionList(void * mbootPtr) {
  regionCount = 0;
  MultibootBootInfo * multibootPtr = (MultibootBootInfo *)mbootPtr;
  
  // loop through and generate the regions
  uint32_t mmapLen = multibootPtr->mmap_length;
  uint32_t mmapAddr = multibootPtr->mmap_addr;
  if (mmapAddr + mmapLen > 0x100000) {
    Panic("GRUB memory map is placed beyond 1MB -- might be destroyed!");
  }
  
  uintptr_t longAddr = (uintptr_t)mmapAddr;
  MultibootMmapInfo * info = NULL;
  MultibootMmapInfo * next = (MultibootMmapInfo *)longAddr;
  while (mmapLen > 0) {
    info = next;
    next = (MultibootMmapInfo *)((uintptr_t)info + info->size + 4);
    mmapLen -= info->size + 4;
    
    if (info->type != 1) continue;
    
    // simplify the process by assuming that the first MB is contiguous (even)
    // though really it's not at all.
    if (info->base_addr + info->length <= 0x100000) continue;
    
    // get the range
    uint64_t start = info->base_addr;
    uint64_t len = info->length;
    
    // if this region is in the lower MB, we will stretch it out to the
    // beginning of the address space.
    if (start <= 0x100000) {
      len += start;
      start = 0;
    }
    
    MemoryRegion region((uintptr_t)start, (size_t)len);
    AddRegion(region);
  }
  if (!regionCount) Panic("GlobalMap() - no regions found");
}

MemoryRegion * PhysRegionList::GetRegions() {
  return regions;
}

int PhysRegionList::GetRegionCount() {
  return regionCount;
}

MemoryRegion * PhysRegionList::FindRegion(uintptr_t ptr) {
  for (int i = 0; i < regionCount; i++) {
    if (regions[i].start > ptr) continue;
    if (regions[i].start + regions[i].size <= ptr) continue;
    return &regions[i];
  }
  return NULL;
}

MemoryRegion * NextRegion(PhysRegionList::MemoryRegion * reg) {
  uintptr_t idx = ((uintptr_t)reg) / sizeof(void *);
  if (idx + 1 == regionCount) return NULL;
  return reg + 1;
}

/**************
 * MapCreator *
 **************/

void MapCreator::IncrementPhysOffset() {
  // find the region in which the current offset lies
  PhysRegionList::MemoryRegion * reg = regions->FindRegion(physOffset);
  if (!reg) {
    Panic("MapCreator::IncrementPhysOffset() - physOffset out of bounds.");
  }
  physOffset += 0x1000;
  while (physOffset + 0x1000 > reg->start + reg->size) {
    reg = regions->NextRegion(reg);
    if (!reg) {
      Panic("MapCreator::IncrementPhysOffset() - out of ranges.");
    }
    physOffset = reg->start;
  }
}

void MapCreator::InitializeTables() {
  pdtOffset = 0;
  pdptOffset = 0;
  
  uint64_t * physPDPT, * physPML4;
  AllocatePage(&physPML4, NULL);
  AllocatePage(&physPDPT, NULL);
  AllocatePage(&physPDT, &virtPDT);
  pdpt = (PhysAddr)physPDPT;
  pml4 = (PhysAddr)physPML4;
  
  physPML4[0] = pdpt | 3;
  physPDPT[0] = (uint64_t)physPDT | 3;
}

void MapCreator::MapNextPage() {
  if (pdtOffset == 0x200) {
    pdtOffset = 0;
    pdptOffset++;
    AllocatePage(&physPDT, &virtPDT);
    VisiblePDPT()[pdptOffset] = (uint64_t)physPDT | 3;
  }
  VisiblePDT()[pdtOffset++] = virtMapped | 3;
  virtMapped += 0x200000;
}

void MapCreator::Switch() {
  hasSwitched = true;
  __asm__("mov %%cr3, %0" : : "r" (pml4));
  cout << "switched to new address space" << endl;
}

void MapCreator::AllocatePage(uint64_t ** phys, uint64_t ** virt) {
  if (virt) *virt = (uint64_t *)virtOffset;
  if (phys) *phys = (uint64_t *)physOffset;
  IncrementPhysOffset();
  bzero((void *)virtOffset, 0x1000);
  virtOffset += 0x1000;
}

uint64_t * MapCreator::VisiblePDT() {
  if (!hasSwitched) {
    return physPDT;
  }
  return virtPDT;
}

uint64_t * MapCreator::VisiblePDPT() {
  return (uint64_t *)pdpt;
}

MapCreator::MapCreator(PhysRegionList * r, AllocatorList * a) {
  regions = r;
  allocators = a;
  virtMapped = 0;
  hasSwitched = false;
}

void MapCreator::Map(uintptr_t total, uintptr_t current) {
  assert(!(total & 0x1fffff));
  assert(!(current & 0x1fffff));
  
  virtOffset = current;
  physOffset = current;
  
  InitializeTables();
  
  while (virtMapped < total) {
    
    MapNextPage();
    if (virtMapped >= virtOffset + 0x1000 && !hasSwitched) {
      Switch();
    }
    
  }
  
  if (!hasSwitched) {
    Switch();
  }
  
  cout << "mapped a total of " << virtMapped << " bytes" << endl;
  
  // create our scratch space
  uint64_t * physScratchPDT, uint64_t * physScratchPT;
  uint64_t * virtScratchPDT;
  AllocatePage(&physScratchPDT, &virtScratchPDT);
  AllocatePage(&physScratchPT, &virtScratchPT);
  virtScratchPDT[0x1ff] = (uint64_t)physScratchPT | 3;
  VisiblePDPT()[0x1ff] = (uint64_t)physScratchPDT | 3;
}

uint64_t * MapCreator::ScratchPageTable() {
  return virtScratchPT;
}

/*************
 * GlobalMap *
 *************/

GlobalMap & GlobalMap::GetGlobalMap() {
  return map;
}

void GlobalMap::InitializeGlobalMap(void * mbootPtr) {
  new(&map) GlobalMap(mbootPtr);
}

GlobalMap::GlobalMap(void * mbootPtr)
  : regions(mbootPtr),
    allocators(0x100000, 0x1000, 0x1000,
               regions.GetRegions(),
               regions.GetRegionCount()) {
}

static size_t PageTableSize(size_t size) {
  size_t ptCount = 1;
  size_t pdtCount = (size >> 21) + (size % (1L << 21) ? 1L : 0L);
  size_t pdptCount = (size >> 30) + (size % (1L << 30) ? 1L : 0L);
  size_t pml4Count = (size >> 39) + (size % (1L << 39) ? 1L : 0L);
  assert(pml4Count == 1);
  assert(pdptCount < 0x200);
  
  // we will also have a final PDT with a page table for temporary pages
  pdptCount++;
  pdtCount++;
  
  return (ptCount + pdtCount + pdptCount + pml4Count) << 12;
}

size_t GlobalMap::MemoryForKernel() {
  return 0x1000000; // TODO: use symbol here
}

size_t GlobalMap::MemoryForBitmaps() {
  size_t val = allocators.BitmapByteCount();
  if (val & 0xfff) {
    val += 0x1000 - (val & 0xfff);
  }
  return val;
}

size_t GlobalMap::MemoryForPageTables() {
  size_t size = MemoryForKernel() + MemoryForBitmaps();
  size_t pageTableSize = 0;
  while (1) {
    size_t newSize = PageTableSize(size + pageTableSize);
    if (newSize == pageTableSize) break;
    pageTableSize = newSize;
  }
  return pageTableSize;
}

void GlobalMap::Setup() {
  allocators.GenerateDescriptions();
  size_t physicalSize = MemoryForPageTables() + MemoryForBitmaps()
    + MemoryForKernel();
  if (physicalSize & 0x1fffff) {
    physicalSize += 0x200000 - (physicalSize & 0x1fffff);
  }
  cout << "reserving " << physicalSize
    << " bytes of physical memory..." << endl;
  
  MapCreator creator(&regions, &allocators);
  creator.Map((uintptr_t)physicalSize, (uintptr_t)MemoryForKernel());
  
  Panic("TODO: do something here");
}

}