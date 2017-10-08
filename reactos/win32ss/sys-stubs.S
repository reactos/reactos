
#include <syscalls.inc>

#ifdef _M_ARM

    TEXTAREA

#define SVC_(name, argcount) STUB_U name

#include "w32ksvc.h"

    END

#else

.code

SyscallId = HEX(1000)
#define SVC_(name, argcount) STUB_U name, argcount

#include "w32ksvc.h"

    END
#endif

