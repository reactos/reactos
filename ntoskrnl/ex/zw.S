
#include <asm.inc>
#include <syscalls.inc>

EXTERN _KiSystemService

.code

SyscallId = 0
#define SVC_(name, argcount) STUB_K name, argcount

#include <sysfuncs.h>

END
