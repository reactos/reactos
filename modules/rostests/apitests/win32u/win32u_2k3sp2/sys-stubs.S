
#include <asm.inc>
#include <syscalls.inc>

.code

SyscallId = HEX(1000)
#define SVC_(name, argcount) STUB_U name, argcount

#ifdef _M_AMD64
#include "win32ksvc-2k3sp2-64.h"
#else
#include "win32ksvc-2k3sp2.h"
#endif

END
