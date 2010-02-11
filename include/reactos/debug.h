/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/internal/debug.h
 * PURPOSE:         Useful debugging macros
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                28/05/98: Created
 */

/*
 * NOTE: Define NDEBUG before including this header to disable debugging
 * macros
 */

#ifndef __INTERNAL_DEBUG
#define __INTERNAL_DEBUG

// #undef NDEBUG

/* Define DbgPrint/DbgPrintEx/RtlAssert unless the NDK is used */
#if !defined(_RTLFUNCS_H) && !defined(_NTDDK_)

/* Make sure we have basic types (some people include us *before* SDK... */
#if !defined(_NTDEF_) && !defined(_NTDEF_H) && !defined(_WINDEF_) && !defined(_WINDEF_H)
#error Please include SDK first.
#endif

ULONG
__cdecl
DbgPrint(
    IN PCCH  Format,
    IN ...
);

NTSYSAPI
ULONG
__cdecl
DbgPrintEx(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCCH  Format,
    IN ...
);

NTSYSAPI
VOID
NTAPI
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
);

#endif /* !defined(_RTLFUNCS_H) && !defined(_NTDDK_) */

#ifndef assert
#ifndef NASSERT
#define assert(x) if (!(x)) {RtlAssert("#x",__FILE__,__LINE__, ""); }
#else
#define assert(x)
#endif
#endif

#ifndef ASSERT
#ifndef NASSERT
#define ASSERT(x) if (!(x)) {RtlAssert("#x",__FILE__,__LINE__, ""); }
#else
#define ASSERT(x)
#endif
#endif

#ifndef ASSERTMSG
#ifndef NASSERT
#define ASSERTMSG(x,m) if (!(x)) {RtlAssert("#x",__FILE__,__LINE__, m); }
#else
#define ASSERTMSG(x)
#endif
#endif

/* Print stuff only on Debug Builds*/
#define DPFLTR_DEFAULT_ID -1
#if DBG

#define DPRINTEX(ch, lev, fmt, ...) DbgPrintEx(ch, lev, "(%s:%d:%s) " fmt, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

/* These are always printed */
#define DPRINT1(fmt, ...) DbgPrint("(%s:%d:%s) " fmt, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

/* These are printed only if NDEBUG is NOT defined */
#ifndef NDEBUG
#define DPRINT(fmt, ...) DPRINT1(fmt, __VA_ARGS__)
#else
#define DPRINT(...)
#endif
#define DPRINTT DPRINT

#define DPRINTC(v1, v2, fmt, ...) if(v1 & v2) DPRINT1(fmt, ...)

#define UNIMPLEMENTED DPRINT1("UNIMPLEMENTED!\n");
#define ERR_(ch, fmt, ...) DPRINTEX (DPFLTR_##ch##_ID, DPFLTR_ERROR_LEVEL, fmt, __VA_ARGS__)
#define WARN_(ch, fmt, ...) DPRINTEX (DPFLTR_##ch##_ID, DPFLTR_WARNING_LEVEL, fmt, __VA_ARGS__)
#define TRACE_(ch, fmt, ...) DPRINTEX (DPFLTR_##ch##_ID, DPFLTR_TRACE_LEVEL, fmt, __VA_ARGS__)
#define INFO_(ch, fmt, ...) DPRINTEX (DPFLTR_##ch##_ID, DPFLTR_INFO_LEVEL, fmt, __VA_ARGS__)

#else /* not DBG */
/* On non-debug builds, we never show these */
#define DPRINT1(...)
#define DPRINTEX(...)
#define DPRINT(...)
#define DPRINTC(...)
#define UNIMPLEMENTED
#define ERR_(ch, ...)
#define WARN_(ch, ...)
#define TRACE_(ch, ...)
#define INFO_(ch, ...)
#endif /* not DBG */

#define ASSERT_IRQL_LESS_OR_EQUAL(x) ASSERT(KeGetCurrentIrql()<=(x))
#define ASSERT_IRQL_EQUAL(x) ASSERT(KeGetCurrentIrql()==(x))
#define ASSERT_IRQL_LESS(x) ASSERT(KeGetCurrentIrql()<(x))

NTKERNELAPI VOID NTAPI KeQuerySystemTime(OUT PLARGE_INTEGER CurrentTime);
NTSYSCALLAPI NTSTATUS NTAPI NtYieldExecution(VOID);

_INLINE VOID DbgWait(i64 x)
{
	i64u t0;
	i64u t;
	KeQuerySystemTime((PLARGE_INTEGER)&t0);
	t0 += x;
	do
	{
		NtYieldExecution();
		KeQuerySystemTime((PLARGE_INTEGER)&t);
	} while (t < t0);
}

#endif /* __INTERNAL_DEBUG */

