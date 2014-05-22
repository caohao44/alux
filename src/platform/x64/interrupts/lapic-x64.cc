#include "lapic-x64.hpp"

namespace OS {

namespace x64 {

static XAPIC * xapic = NULL;
static X2APIC * x2apic = NULL;
static int LAPICDivide = 0x3;

LAPIC & LAPIC::GetCurrent() {
  AssertCritical();
  
  uint32_t ecx;
  CPUID(1, NULL, &ecx, NULL);
  if (ecx & (1 << 21)) {
    // setup x2APIC
    if (!x2apic) {
      x2apic = new X2APIC();
    }
    return *x2apic;
  }
  
  // setup xAPIC
  if (!xapic) {
    ACPI::MADT * madt = ACPI::GetMADT();
    xapic = new XAPIC((uint64_t)madt->GetHeader().lapicAddr);
  }
  return *xapic;
}

LAPIC::~LAPIC() {
}

void LAPIC::SetDefaults() {
  AssertCritical();
  WriteRegister(RegTASKPRIOR, 0x0);
  WriteRegister(RegLVT_TMR, 0x10000);
  WriteRegister(RegLVT_PERF, 0x10000);
  WriteRegister(RegLVT_LINT0, 0x8700);
  WriteRegister(RegLVT_LINT1, 0x400);
  WriteRegister(RegLVT_ERR, 0x10000);
  WriteRegister(RegSPURIOUS, 0x1ff);

  // reset might have shut them off
  WriteRegister(RegLVT_LINT0, 0x8700);
  WriteRegister(RegLVT_LINT1, 0x400);

  WriteRegister(RegTMRDIV, LAPICDivide);
}

void LAPIC::ClearErrors() {
  AssertCritical();
  WriteRegister(RegESR, 0);
}

void LAPIC::SendEOI() {
  AssertCritical();
  WriteRegister(RegEOI, 0);
}

void LAPIC::SetPriority(uint8_t vector) {
  AssertCritical();
  WriteRegister(RegTASKPRIOR, vector);
}

bool LAPIC::IsRequested(uint8_t vector) {
  AssertCritical();
  uint64_t regIndex = 0x20 + (vector >> 5);
  uint32_t mask = (1 << (vector & 0x1f));
  return 0 != (ReadRegister(regIndex) & mask);
}

bool LAPIC::IsInService(uint8_t vector) {
  AssertCritical();
  uint64_t regIndex = 0x10 + (vector >> 5);
  uint32_t mask = (1 << (vector & 0x1f));
  return 0 != (ReadRegister(regIndex) & mask);
}

XAPIC::XAPIC(uint64_t _base) : base(_base) {
  AssertCritical();
  VirtAddr addr;
  bool res = KernMap::Map(base, 0x1000, addr);
  assert(res);
  regs = (volatile uint32_t *)addr;
}

XAPIC::~XAPIC() {
  Panic("Destroying an XAPIC will cause a map leak");
  // KernMap::Unmap((VirtAddr)regs, 0x1000);
}

uint64_t XAPIC::ReadRegister(uint16_t reg) {
  AssertCritical();
  if (reg != 0x30) {
    return (uint64_t)regs[reg * 4];
  } else {
    uint32_t lower = regs[reg * 4];
    uint32_t higher = regs[(reg + 1) * 4];
    return (uint64_t)lower | ((uint64_t)higher << 0x20);
  }
}

void XAPIC::WriteRegister(uint16_t reg, uint64_t value) {
  AssertCritical();
  if (reg != 0x30) {
    assert(!(value & 0xFFFFFFFF00000000L));
    regs[reg * 4] = value;
  } else {
    regs[(reg + 1) * 4] = value >> 0x20;
    regs[reg * 4] = value & 0xFFFFFFFF;
  }
}

void XAPIC::Enable() {
  AssertCritical();
  uint64_t flags = ReadMSR(0x1b) & 0xf00;
  flags |= 1 << 11;
  WriteMSR(0x1b, base | flags);
}

uint32_t XAPIC::GetId() {
  AssertCritical();
  return ReadRegister(RegAPICID) >> 0x18;
}

void XAPIC::SendIPI(uint32_t cpu, uint8_t vector,
                    uint8_t mode, uint8_t level,
                    uint8_t trigger) {
  AssertCritical();
  uint64_t value = 0;
  value = (uint64_t)vector | ((uint64_t)mode << 8);
  value |= ((uint64_t)level << 0xe) | ((uint64_t)trigger << 0xf);
  value |= ((uint64_t)cpu << 0x38);
  WriteRegister(0x30, value);
}

uint64_t X2APIC::ReadRegister(uint16_t reg) {
  AssertCritical();
  return ReadMSR((uint32_t)reg + 0x800);
}

void X2APIC::WriteRegister(uint16_t reg, uint64_t value) {
  AssertCritical();
  WriteMSR((uint32_t)reg + 0x800, value);
}

void X2APIC::Enable() {
  AssertCritical();
  uint64_t flags = ReadMSR(0x1b) & 0xf00;
  flags |= 3 << 10;
  WriteMSR(0x1b, flags);
}

uint32_t X2APIC::GetId() {
  AssertCritical();
  return ReadRegister(RegAPICID);
}

void X2APIC::SendIPI(uint32_t cpu, uint8_t vector,
                     uint8_t mode, uint8_t level,
                     uint8_t trigger) {
  AssertCritical();
  uint64_t value = 0;
  value = (uint64_t)vector | ((uint64_t)mode << 8);
  value |= ((uint64_t)level << 0xe) | ((uint64_t)trigger << 0xf);
  value |= ((uint64_t)cpu << 0x20);
  WriteRegister(0x30, value);
}

}

}

