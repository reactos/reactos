/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock Helper DLL for TCP/IP
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

#define DEBUG_ULTRA    0xFFFFFFFF

#ifdef DBG

extern DWORD DebugTraceLevel;

#define Get_DbgPrint(quote...) L##quote

#define WSH_DbgPrint(_t_, _x_) \
    if (((DebugTraceLevel & NORMAL_MASK) >= _t_) || \
        ((DebugTraceLevel & _t_) > NORMAL_MASK)) { \
        WCHAR _buffer[256]; \
        swprintf(_buffer, L"(%s:%d)(%s) ", __FILE__, __LINE__, __FUNCTION__); \
        OutputDebugStringW(_buffer); \
        swprintf(_buffer, Get_DbgPrint _x_); \
        OutputDebugStringW(_buffer); \
    }

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef NASSERT
#define ASSERT(x)
#else /* NASSERT */
#define ASSERT(x) if (!(x)) { WSH_DbgPrint(MIN_TRACE, ("Assertion "#x" failed at %s:%d\n", __FILE__, __LINE__)); KeBugCheck(0); }
#endif /* NASSERT */

#define ASSERT_IRQL(x) ASSERT(KeGetCurrentIrql() <= (x))

#else /* DBG */

#define WSH_DbgPrint(_t_, _x_)

#define ASSERT_IRQL(x)
#define ASSERT(x)

#endif /* DBG */


#define assert(x) ASSERT(x)
#define assert_irql(x) ASSERT_IRQL(x)


#define UNIMPLEMENTED \
    WSH_DbgPrint(MIN_TRACE, ("(%s:%d)(%s) is unimplemented, \
        please try again later.\n", __FILE__, __LINE__, __FUNCTION__));

#define CHECKPOINT \
    do { WSH_DbgPrint(MIN_TRACE, ("(%s:%d)\n", __FILE__, __LINE__)); } while(0);

#endif /* __DEBUG_H */

/* EOF */
