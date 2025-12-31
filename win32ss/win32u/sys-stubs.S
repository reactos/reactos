
#include <asm.inc>
#include <syscalls.inc>

.code

SyscallId = HEX(1000)
#define SVC_(name, argcount) STUB_U name, argcount

#ifdef _M_AMD64
#include <w32ksvc64.h>
#else
#include <w32ksvc32.h>
#endif

END
