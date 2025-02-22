/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/debug.h
 * PURPOSE:     Debugging support macros
 * DEFINES:     DBG     - Enable debug output
 *              NASSERT - Disable assertions
 */

#pragma once

#define NORMAL_MASK    0x000000FF
#define SPECIAL_MASK   0xFFFFFF00
#define MIN_TRACE      0x00000001
#define MID_TRACE      0x00000002
#define MAX_TRACE      0x00000003

#define DEBUG_CHECK    0x00000100
#define DEBUG_MEMORY   0x00000200
#define DEBUG_PBUFFER  0x00000400
#define DEBUG_IRP      0x00000800
#define DEBUG_REFCOUNT 0x00001000
#define DEBUG_ADDRFILE 0x00002000
#define DEBUG_DATALINK 0x00004000
#define DEBUG_ARP      0x00008000
#define DEBUG_IP       0x00010000
#define DEBUG_UDP      0x00020000
#define DEBUG_TCP      0x00040000
#define DEBUG_ICMP     0x00080000
#define DEBUG_ROUTER   0x00100000
#define DEBUG_RCACHE   0x00200000
#define DEBUG_NCACHE   0x00400000
#define DEBUG_CPOINT   0x00800000
#define DEBUG_ULTRA    0xFFFFFFFF

#if DBG

extern DWORD DebugTraceLevel;

#ifdef _MSC_VER

#define LA_DbgPrint(_t_, _x_) \
    if ((_t_ > NORMAL_MASK) \
        ? (DebugTraceLevel & _t_) > NORMAL_MASK \
        : (DebugTraceLevel & NORMAL_MASK) >= _t_) { \
        DbgPrint("(%s:%d) ", __FILE__, __LINE__); \
        DbgPrint _x_ ; \
    }

#else /* _MSC_VER */

#define LA_DbgPrint(_t_, _x_) \
    if ((_t_ > NORMAL_MASK) \
        ? (DebugTraceLevel & _t_) > NORMAL_MASK \
        : (DebugTraceLevel & NORMAL_MASK) >= _t_) { \
        DbgPrint("(%s:%d)(%s) ", __FILE__, __LINE__, __FUNCTION__); \
        DbgPrint _x_ ; \
    }

#endif /* _MSC_VER */

#define ASSERT_IRQL(x) ASSERT(KeGetCurrentIrql() <= (x))

#else /* DBG */

#define LA_DbgPrint(_t_, _x_)

#if 0
#define ASSERT_IRQL(x)
#define ASSERT(x)
#endif

#endif /* DBG */


#define assert(x) ASSERT(x)
#define assert_irql(x) ASSERT_IRQL(x)


#ifdef _MSC_VER

#define UNIMPLEMENTED \
    LA_DbgPrint(MIN_TRACE, ("The function at %s:%d is unimplemented, \
        but come back another day.\n", __FILE__, __LINE__));

#else /* _MSC_VER */

#define UNIMPLEMENTED \
    LA_DbgPrint(MIN_TRACE, ("(%s:%d)(%s) is unimplemented, \
        but come back another day.\n", __FILE__, __LINE__, __FUNCTION__));

#endif /* _MSC_VER */


#define CHECKPOINT \
    do { LA_DbgPrint(DEBUG_CHECK, ("(%s:%d)\n", __FILE__, __LINE__)); } while(0);

#define CP CHECKPOINT

#include <memtrack.h>
