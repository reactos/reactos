
#include <syscalls.inc>

/* Skip NT5 syscalls, so we get the right IDs for the NT6 ones */
#define SYSFUNCS_NT5_ONLY
#ifdef _M_ARM64
#define SVC_(name, argcount) SKIP_STUB name
#else
#define SVC_(name, argcount) SyscallId = SyscallId + 1
#endif
#include <sysfuncs.h>
#undef SVC_
#undef SYSFUNCS_NT5_ONLY

/* Now do the NT6 ones proper */
#define SYSFUNCS_NT6_ONLY

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
