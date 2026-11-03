
#include <syscalls.inc>

/* This file only has the NT5 syscalls */
#define SYSFUNCS_NT5_ONLY

#if defined(_M_ARM) || defined(_M_ARM64)

    TEXTAREA

#define SVC_(name, argcount) STUB_U name

#include <sysfuncs.h>

    END

#else

.code

#define SVC_(name, argcount) STUB_U name, argcount

#include <sysfuncs.h>

END
#endif
