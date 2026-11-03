
#include <syscalls.inc>

#if defined(_M_ARM) || defined(_M_ARM64)

    TEXTAREA

#ifdef _M_ARM64
    IMPORT KiZwSystemService
#endif

#define SVC_(name, argcount) STUB_K name

#include <sysfuncs.h>

#else

#ifdef _M_IX86
EXTERN _KiSystemService:PROC
#elif defined(_M_AMD64)
#include <ksamd64.inc>
EXTERN KiSystemService:PROC
EXTERN KiZwSystemService:PROC
#endif

.code

#define SVC_(name, argcount) STUB_K name, argcount

#include <sysfuncs.h>

#endif

    END
