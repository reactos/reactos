/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/reactos/debug.h
 * PURPOSE:         Useful debugging macros
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/*
 * NOTE: Define NDEBUG before including this header
 * to disable debugging macros.
 */

#pragma once

#include <builddir.h>

#if !defined(__RELFILE__)
#define __RELFILE__ __FILE__
#endif

/* Define DbgPrint/DbgPrintEx/RtlAssert unless the NDK is used */
#if !defined(_RTLFUNCS_H) && !defined(_NTDDK_)

/* Make sure we have basic types (some people include us *before* SDK)... */
#if !defined(_NTDEF_) && !defined(_NTDEF_H) && !defined(_WINDEF_) && !defined(_WINDEF_H)
#error Please include SDK first.
#endif

#ifdef __cplusplus
extern "C" {
#endif

ULONG
__cdecl
DbgPrint(
    _In_z_ _Printf_format_string_ PCSTR Format,
    ...
);

NTSYSAPI
ULONG
__cdecl
DbgPrintEx(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_z_ _Printf_format_string_ PCSTR Format,
    ...
);

__analysis_noreturn
NTSYSAPI
VOID
NTAPI
RtlAssert(
    _In_ PVOID FailedAssertion,
    _In_ PVOID FileName,
    _In_ ULONG LineNumber,
    _In_opt_z_ PCHAR Message
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !defined(_RTLFUNCS_H) && !defined(_NTDDK_) */

#ifndef assert
#if DBG && !defined(NASSERT)
#define assert(x) if (!(x)) { RtlAssert((PVOID)#x, (PVOID)__RELFILE__, __LINE__, ""); }
#else
#define assert(x) ((VOID) 0)
#endif
#endif

#ifndef ASSERT
#if DBG && !defined(NASSERT)
#define ASSERT(x) if (!(x)) { RtlAssert((PVOID)#x, (PVOID)__RELFILE__, __LINE__, ""); }
#else
#define ASSERT(x) ((VOID) 0)
#endif
#endif

#ifndef ASSERTMSG
#if DBG && !defined(NASSERT)
#define ASSERTMSG(m, x) if (!(x)) { RtlAssert((PVOID)#x, __RELFILE__, __LINE__, m); }
#else
#define ASSERTMSG(m, x) ((VOID) 0)
#endif
#endif

/* For internal purposes only */
#define __NOTICE(level, fmt, ...)   DbgPrint(#level ":  %s at %s:%d " fmt, __FUNCTION__, __RELFILE__, __LINE__, ##__VA_ARGS__)

/* Print stuff only on Debug Builds*/
#define DPFLTR_DEFAULT_ID -1
#if DBG

    /* These are always printed */
    #define DPRINT1(fmt, ...) do { \
        if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
            DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

    /* These are printed only if NDEBUG is NOT defined */
    #ifndef NDEBUG

        #define DPRINT(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)

    #else

#if defined(_MSC_VER)
        #define DPRINT   __noop
#else
        #define DPRINT(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

    #endif

    #define UNIMPLEMENTED         __NOTICE(WARNING, "is UNIMPLEMENTED!\n");

    #define ERR_(ch, fmt, ...)    DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_ERROR_LEVEL, "(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__)
    #define WARN_(ch, fmt, ...)   DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_WARNING_LEVEL, "(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__)
    #define TRACE_(ch, fmt, ...)  DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_TRACE_LEVEL, "(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__)
    #define INFO_(ch, fmt, ...)   DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_INFO_LEVEL, "(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__)

    #define ERR__(ch, fmt, ...)    DbgPrintEx(ch, DPFLTR_ERROR_LEVEL, "(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__)
    #define WARN__(ch, fmt, ...)   DbgPrintEx(ch, DPFLTR_WARNING_LEVEL, "(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__)
    #define TRACE__(ch, fmt, ...)  DbgPrintEx(ch, DPFLTR_TRACE_LEVEL, "(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__)
    #define INFO__(ch, fmt, ...)   DbgPrintEx(ch, DPFLTR_INFO_LEVEL, "(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__)

#else /* not DBG */

    /* On non-debug builds, we never show these */
#if defined(_MSC_VER)
    #define DPRINT1   __noop
    #define DPRINT    __noop

    #define UNIMPLEMENTED

    #define ERR_      __noop
    #define WARN_     __noop
    #define TRACE_    __noop
    #define INFO_     __noop

    #define ERR__     __noop
    #define WARN__    __noop
    #define TRACE__   __noop
    #define INFO__    __noop
#else
    #define DPRINT1(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define DPRINT(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)

    #define UNIMPLEMENTED

    #define ERR_(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define WARN_(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define TRACE_(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define INFO_(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)

    #define ERR__(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define WARN__(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define TRACE__(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define INFO__(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif /* _MSC_VER */

#endif /* not DBG */

/******************************************************************************/
/*
 * Declare a target-dependent process termination procedure.
 */
#ifndef _NTDDK_             /* User-Mode */
    #ifndef NTOS_MODE_USER  /* Should be Win32 */
        #ifndef _WIN32
            #error "Unsupported target."
        #else
            #define TerminateCurrentProcess(Status) TerminateProcess(GetCurrentProcess(), (Status))
        #endif
    #else   /* Native */
        #ifndef _PSFUNCS_H
            NTSYSCALLAPI
            NTSTATUS
            NTAPI
            NtTerminateProcess(
                IN HANDLE ProcessHandle,
                IN NTSTATUS ExitStatus
            );
        #endif
        #ifndef NtCurrentProcess
            #define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)
        #endif
        #define TerminateCurrentProcess(Status) NtTerminateProcess(NtCurrentProcess(), (Status))
    #endif
#else   /* Kernel-Mode */
    #include <bugcodes.h>
    #define TerminateCurrentProcess(Status) KeBugCheckEx(CRITICAL_SERVICE_FAILED, (Status), 0, 0, 0)
#endif


/* For internal purposes only */
#define __ERROR_DBGBREAK(...)   \
do {                            \
    DbgPrint("" __VA_ARGS__);   \
    DbgBreakPoint();            \
} while (0)

/* For internal purposes only */
#define __ERROR_FATAL(Status, ...)      \
do {                                    \
    DbgPrint("" __VA_ARGS__);           \
    DbgBreakPoint();                    \
    TerminateCurrentProcess(Status);    \
} while (0)

/*
 * These macros are designed to display an optional printf-like
 * user-defined message and to break into the debugger.
 * After that they allow to continue the program execution.
 */
#define ERROR_DBGBREAK(...)         \
do {                                \
    __NOTICE(ERROR, "\n");          \
    __ERROR_DBGBREAK(__VA_ARGS__);  \
} while (0)

#define UNIMPLEMENTED_DBGBREAK(...)         \
do {                                        \
    __NOTICE(ERROR, "is UNIMPLEMENTED!\n"); \
    __ERROR_DBGBREAK(__VA_ARGS__);          \
} while (0)

/*
 * These macros are designed to display an optional printf-like
 * user-defined message and to break into the debugger.
 * After that they halt the execution of the current thread.
 */
#define ERROR_FATAL(...)                                    \
do {                                                        \
    __NOTICE(UNRECOVERABLE ERROR, "\n");                    \
    __ERROR_FATAL(STATUS_ASSERTION_FAILURE, __VA_ARGS__);   \
} while (0)

#define UNIMPLEMENTED_FATAL(...)                            \
do {                                                        \
    __NOTICE(UNRECOVERABLE ERROR, "is UNIMPLEMENTED!\n");   \
    __ERROR_FATAL(STATUS_NOT_IMPLEMENTED, __VA_ARGS__);     \
} while (0)
/******************************************************************************/

#define ASSERT_IRQL_LESS_OR_EQUAL(x) ASSERT(KeGetCurrentIrql()<=(x))
#define ASSERT_IRQL_EQUAL(x) ASSERT(KeGetCurrentIrql()==(x))
#define ASSERT_IRQL_LESS(x) ASSERT(KeGetCurrentIrql()<(x))

#define __STRING2__(x) #x
#define __STRING__(x) __STRING2__(x)
#define __STRLINE__ __STRING__(__LINE__)

#if !defined(_MSC_VER) && !defined(__pragma)
#define __pragma(x) _Pragma(#x)
#endif

#define _WARN(msg) __pragma(message("WARNING! Line " __STRLINE__ ": " msg))

/* EOF */
