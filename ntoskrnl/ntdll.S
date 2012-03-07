
#include <asm.inc>
#include <syscalls.inc>

.code

SyscallId = 0
#define SVC_(name, argcount) STUB_U name, argcount

#include <sysfuncs.h>

END
