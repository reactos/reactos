/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        include/debug.h
 * PURPOSE:     Debugging support macros
 * DEFINES:     DBG     - Enable debug output
 *              NASSERT - Disable assertions
 */
#ifndef __DEBUG_H
#define __DEBUG_H

#define NORMAL_MASK    0x000000FF
#define SPECIAL_MASK   0xFFFFFF00
#define MIN_TRACE      0x00000001
#define MID_TRACE      0x00000002
#define MAX_TRACE      0x00000003

#define DEBUG_CHECK    0x00000100
#define DEBUG_ULTRA    0xFFFFFFFF

#ifdef DBG

extern DWORD DebugTraceLevel;

#define Print(_t_, _x_) \
    if (((DebugTraceLevel & NORMAL_MASK) >= _t_) || \
        ((DebugTraceLevel & _t_) > NORMAL_MASK)) { \
        DbgPrint("(%hS:%d)(%hS) ", __FILE__, __LINE__, __FUNCTION__); \
		DbgPrint _x_; \
    }

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef NASSERT
#define ASSERT(x)
#else /* NASSERT */
#define ASSERT(x) if (!(x)) { Print(MIN_TRACE, ("Assertion "#x" failed at %s:%d\n", __FILE__, __LINE__)); ExitProcess(0); }
#endif /* NASSERT */

#else /* DBG */

#define Print(_t_, _x_)

#define ASSERT_IRQL(x)
#define ASSERT(x)

#endif /* DBG */


#define assert(x) ASSERT(x)
#define assert_irql(x) ASSERT_IRQL(x)


#define UNIMPLEMENTED \
    Print(MIN_TRACE, ("is unimplemented, please try again later.\n"));

#define CHECKPOINT \
    Print(DEBUG_CHECK, ("\n"));

#define CP CHECKPOINT


static char GuidBufferSpace[40];

static inline PCHAR PRINT_GUID( const GUID *id )
{
    CHAR *str;

    if (!id) return "(null)";

    str = &GuidBufferSpace[0];

    if (!HIWORD(id))
    {
        sprintf( str, "<guid-0x%04x>", LOWORD((ULONG)id) );
    }
    else
    {
        sprintf( str, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 id->Data1, id->Data2, id->Data3,
                 id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                 id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
    }
    return str;
}

#endif /* __DEBUG_H */

/* EOF */
