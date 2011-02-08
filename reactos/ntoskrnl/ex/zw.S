
#include <asm.inc>
#include <syscalls.inc>

.code

SyscallId = 0
#define SVC_(name, argcount) STUB_K name, argcount

#include "sysfuncs.h"

END
