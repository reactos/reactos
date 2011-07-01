
#include <asm.inc>
#include <syscalls.inc>

.code

SyscallId = HEX(1000)
#define SVC_(name, argcount) STUB_U name, argcount

#include "w32ksvc.h"

END
