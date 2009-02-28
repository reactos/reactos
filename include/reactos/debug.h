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

#endif

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
#ifdef DBG

    /* These are always printed */
    #define DPRINT1 DbgPrint("(%s:%d) ",__FILE__,__LINE__), DbgPrint

    /* These are printed only if NDEBUG is NOT defined */
    #ifndef NDEBUG

        #define DPRINT DbgPrint("(%s:%d) ",__FILE__,__LINE__), DbgPrint

    #else
        #ifdef _MSC_VER
            static __inline void DPRINT ( const char* fmt, ... )
            {
                UNREFERENCED_PARAMETER(fmt);
            }
        #else
            #define DPRINT(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
        #endif
    #endif

    #define UNIMPLEMENTED       DbgPrint("WARNING:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__);

    /* The ##__VA_ARGS__ syntax is not a standard and was only tested with MSVC and GCC. If other compilers support them as well, add them to this #if block. */
    #if defined(_MSC_VER) || defined(__GNUC__)
        #define ERR_(ch, fmt, ...)    DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_ERROR_LEVEL, "(%s:%d) " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
        #define WARN_(ch, fmt, ...)   DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_WARNING_LEVEL, "(%s:%d) " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
        #define TRACE_(ch, fmt, ...)  DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_TRACE_LEVEL, "(%s:%d) " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
        #define INFO_(ch, fmt, ...)   DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_INFO_LEVEL, "(%s:%d) " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
    #endif
#else

    /* On non-debug builds, we never show these */
    #define DPRINT1(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define DPRINT(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)

    #define UNIMPLEMENTED

    #define ERR_(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define WARN_(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define TRACE_(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define INFO_(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

#define ASSERT_IRQL_LESS_OR_EQUAL(x) ASSERT(KeGetCurrentIrql()<=(x))
#define ASSERT_IRQL_EQUAL(x) ASSERT(KeGetCurrentIrql()==(x))
#define ASSERT_IRQL_LESS(x) ASSERT(KeGetCurrentIrql()<(x))

#endif /* __INTERNAL_DEBUG */
