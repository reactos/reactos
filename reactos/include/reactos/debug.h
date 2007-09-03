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

/* Define DbgPrint/RtlAssert unless the NDK is used */
#if !defined(_RTLFUNCS_H) && (!defined(_NTDDK_) || !defined(__NTDDK_H))

/* Make sure we have basic types (some people include us *before* SDK... */
#if defined(_NTDEF_) || (defined _WINDEF_) || (defined _WINDEF_H)
ULONG
__cdecl
DbgPrint(
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

#else

    /* On non-debug builds, we never show these */
    #define DPRINT1(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define DPRINT(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)

    #define CHECKPOINT1
    #define CHECKPOINT
    #define UNIMPLEMENTED
#endif

/*
 * FUNCTION: Assert a maximum value for the current irql
 * ARGUMENTS:
 *        x = Maximum irql
 */
#define ASSERT_IRQL(x) assert(KeGetCurrentIrql()<=(x))
#define assert_irql(x) assert(KeGetCurrentIrql()<=(x))

/* Macros expanding to the appropriate inline assembly to raise a breakpoint */
#if defined(_M_IX86)
#define ASM_BREAKPOINT "\nint $3\n"
#elif defined(_M_ALPHA)
#define ASM_BREAKPOINT "\ncall_pal bpt\n"
#elif defined(_M_MIPS)
#define ASM_BREAKPOINT "\nbreak\n"
#elif defined(__x86_64__)
#define ASM_BREAKPOINT "\nint $3\n"
#elif defined(_M_PPC)
#define ASM_BREAKPOINT "\ntwi 1\n"
#else
#error Unsupported architecture.
#endif

#ifndef KEBUGCHECK
#define KEBUGCHECK(a) DbgPrint("KeBugCheck (0x%X) at %s:%i\n", a, __FILE__,__LINE__), KeBugCheck(a)
#define KEBUGCHECKEX(a,b,c,d,e) DbgPrint("KeBugCheckEx (0x%X, 0x%X, 0x%X, 0x%X, 0x%X) at %s:%i\n", a, b, c, d, e, __FILE__,__LINE__), KeBugCheckEx(a,b,c,d,e)
#endif

#endif /* __INTERNAL_DEBUG */
