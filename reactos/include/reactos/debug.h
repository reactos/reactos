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

/*  FIXME: should probably remove this later  */
#if !defined(CHECKED) && !defined(NDEBUG)
#define CHECKED
#endif

/* Define DbgPrint/DbgPrintEx/RtlAssert unless the NDK is used */
#if !defined(_RTLFUNCS_H) && (!defined(_NTDDK_) || !defined(__NTDDK_H))

/* Make sure we have basic types (some people include us *before* SDK... */
#if !defined(_NTDEF_) && !defined(_WINDEF_) && !defined(_WINDEF_H)
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
#ifdef DBG

    /* These are always printed */
    #define DPRINT1 DbgPrint("(%s:%d) ",__FILE__,__LINE__), DbgPrint
    #define CHECKPOINT1 do { DbgPrint("%s:%d\n",__FILE__,__LINE__); } while(0);

    /* These are printed only if NDEBUG is NOT defined */
    #ifndef NDEBUG

        #define DPRINT DbgPrint("(%s:%d) ",__FILE__,__LINE__), DbgPrint
        #define CHECKPOINT do { DbgPrint("%s:%d\n",__FILE__,__LINE__); } while(0);

    #else
        #ifdef _MSC_VER
            static __inline void DPRINT ( const char* fmt, ... )
            {
                UNREFERENCED_PARAMETER(fmt);
            }
        #else
            #define DPRINT(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
        #endif
        #define CHECKPOINT
    #endif

    #define UNIMPLEMENTED \
        DbgPrint("WARNING:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__);

    #if defined(__GNUC__)
        #define ERR_(ch, args...)   DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_ERROR_LEVEL, "(%s:%d)", __FILE__, __LINE__), \
                                    DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_ERROR_LEVEL, ##args)
        #define WARN_(ch, args...)  DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_WARNING_LEVEL, "(%s:%d)", __FILE__, __LINE__), \
                                    DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_WARNING_LEVEL, ##args)
        #define TRACE_(ch, args...) DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_TRACE_LEVEL, "(%s:%d)", __FILE__, __LINE__), \
                                    DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_TRACE_LEVEL, ##args)
        #define INFO_(ch, args...)  DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_INFO_LEVEL, "(%s:%d)", __FILE__, __LINE__), \
                                    DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_INFO_LEVEL, ##args)
    #elif defined(_MSC_VER)
        #define ERR_(ch, ...)       DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_ERROR_LEVEL, "(%s:%d)", __FILE__, __LINE__), \
                                    DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__)
        #define WARN_(ch, ...)      DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_WARNING_LEVEL, "(%s:%d)", __FILE__, __LINE__), \
                                    DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_WARNING_LEVEL, __VA_ARGS__)
        #define TRACE_(ch, ...)     DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_TRACE_LEVEL, "(%s:%d)", __FILE__, __LINE__), \
                                    DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_TRACE_LEVEL, __VA_ARGS__)
        #define INFO_(ch, ...)      DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_INFO_LEVEL, "(%s:%d)", __FILE__, __LINE__), \
                                    DbgPrintEx(DPFLTR_##ch##_ID, DPFLTR_INFO_LEVEL, __VA_ARGS__)
    #else
        #error Unknown compiler
    #endif

#else

    /* On non-debug builds, we never show these */
    #define DPRINT1(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define DPRINT(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)

    #define CHECKPOINT1
    #define CHECKPOINT
    #define UNIMPLEMENTED

    #define ERR_(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define WARN_(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define TRACE_(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define INFO_(ch, ...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

/*
 * FUNCTION: Assert a maximum value for the current irql
 * ARGUMENTS:
 *        x = Maximum irql
 */
#define ASSERT_IRQL(x) assert(KeGetCurrentIrql()<=(x))
#define assert_irql(x) assert(KeGetCurrentIrql()<=(x))

#ifndef KEBUGCHECK
#define KEBUGCHECK(a) DbgPrint("KeBugCheck (0x%X) at %s:%i\n", a, __FILE__,__LINE__), KeBugCheck(a)
#define KEBUGCHECKEX(a,b,c,d,e) DbgPrint("KeBugCheckEx (0x%X, 0x%X, 0x%X, 0x%X, 0x%X) at %s:%i\n", a, b, c, d, e, __FILE__,__LINE__), KeBugCheckEx(a,b,c,d,e)
#endif

#endif /* __INTERNAL_DEBUG */
