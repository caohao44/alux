#include <arch/x64/scheduler/state.hpp>
#include <critical>

namespace OS {

State * State::NewKernel(void * call) {
  return new x64::State(call, NULL, true);
}

State * State::NewKernel(void * call, void * arg) {
  return new x64::State(call, arg, true);
}

State * State::NewUser(void * call) {
  return new x64::State(call, NULL, false);
}

namespace x64 {

State::State(void * call, void * arg, bool _kernel)
  : kernel(_kernel), rdi((uint64_t)arg) {
  AssertNoncritical();
  
  stack = new uint8_t[0x4000];
  
  if (kernel) {
    state.ss = 0;
    state.rsp = (uint64_t)kernStack + 0x4000;
    state.cs = 8;
  } else {
    state.ss = 0x1b;
    state.cs = 0x23;
  }
  state.rflags = 0x200;
  state.rip = (uint64_t)call;
}

State::~State() {
  delete stack;
}

void State::Load() {
  AssertCritical();
  if (!kernel) {
    CPU & cpu = CPU::GetCurrent();
    cpu.GetTSS()->rsp[0] = (uint64_t)stack + 0x4000;
  }
  __asm__ __volatile__(
    "sub $0x28, %%rsp\n"
    "mov $5, %%rcx\n"
    "mov %%rsp, %%rdi\n"
    "rep movsq\n"
    "mov %%rdx, %%rdi\n"
    "iretq"
    : : "S" (&state), "d" (rdi)
  );
}

void State::Delete() {
  delete this;
}

}

}
