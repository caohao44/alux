# Alux

Alux is a small Hobby Operating System. The purpose of this project is to create a lightweight microkernel on top of which you may place a language interpreter like V8.

General architectual ideas that I am playing around with:

 * A unified address space to reduce latency due to TLB misses
 * CPU pinning for tasks/threads
 * Cross-platform abstractions
 * Memory allocation and VMM completely in user-space

## To-do

I have SMP working, so now I'm going to focus on finishing up the boot process and tightening up the standards for post-boot execution. Here's some things I need to do:

 * Move the GetLocalAPIC() method to LAPIC::GetCurrent()
 * Change GetBaseIOAPIC() to IOAPIC::GetBase()
 * Create an HPET implementation
 * Add custom interrupt handler for Panic IPI; simply `cli` & `hlt`
 * Create object-send IPI mechanism
 * Custom one-time interrupt handler for CPU APIC timer calibration
 * Calibrate CPU real time clock if it is available; otherwise, keep the PIT enabled for time-keeping purposes.

Also, there's some miscellaneous stuff I want to do:

 * Implement ACPI shutdown
 * Look into Translation Cache Extension to increase invlpg speed
 * Tighten up header docs about what is critical and noncritical and what is synchronous and what is not.

## License

Alux is licensed under the BSD 2-clause license. See [LICENSE](https://github.com/unixpickle/alux/blob/master/LICENSE).

```
Copyright (c) 2014, Alex Nichol and Alux contributors.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```
