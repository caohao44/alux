#ifndef __ALUX_SYSCALL_HANDLER_HPP__
#define __ALUX_SYSCALL_HANDLER_HPP__

#include <anarch/api/syscall-module>

namespace Alux {

anarch::SyscallRet SyscallHandler(uint16_t number, anarch::SyscallArgs & args);

}

#endif
