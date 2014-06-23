#ifndef __SYSCALL_SCHEDULER_HPP__
#define __SYSCALL_SCHEDULER_HPP__

namespace OS {

void SyscallLaunchThread(void * address, void * argument);
void SyscallFork(void * address, void * argument);
void SyscallExit(bool wasError);
void SyscallThreadExit();

}

#endif
