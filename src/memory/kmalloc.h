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

#ifndef __MEMORY_KMALLOC_H__
#define __MEMORY_KMALLOC_H__

#include <platform/failure.h>
#include <analloc2.h>

namespace OS {

void * Kmalloc(size_t size);
void Kfree(void * ptr);
void InitializeKmalloc();

class KmallocRegion {
private:
  typedef ANAlloc::BBTree Tree;
  typedef ANAlloc::Allocator<Tree> Allocator;
  const size_t PageSize = 0x40;
  
  void * start;
  size_t length;
  
  Tree tree;
  Allocator allocator;
  
protected:
  KmallocRegion * next;
  friend void * Kmalloc(size_t size);

public:
  KmallocRegion(void * start, size_t length);
  
  void * Allocate(size_t size);
  void Free(void * buff);
  
};

}

#endif