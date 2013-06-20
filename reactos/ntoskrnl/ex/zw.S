
#include <asm.inc>
#include <syscalls.inc>

#ifdef _M_IX86
EXTERN _KiSystemService:PROC
#elif defined(_M_AMD64)
#include <ksamd64.inc>
EXTERN KiSystemService:PROC
EXTERN KiZwSystemService:PROC
#endif

.code

SyscallId = 0
#define SVC_(name, argcount) STUB_K name, argcount

#include <sysfuncs.h>

END
