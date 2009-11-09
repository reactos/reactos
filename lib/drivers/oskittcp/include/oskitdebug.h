/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/debug.h
 * PURPOSE:     Debugging support macros
 * DEFINES:     DBG     - Enable debug output
 *              NASSERT - Disable assertions
 */
#ifndef __OSKITDEBUG_H
#define __OSKITDEBUG_H

#define OSK_NORMAL_MASK     0x000000FF
#define OSK_SPECIAL_MASK    0xFFFFFF00
#define OSK_MIN_TRACE       0x00000001
#define OSK_MID_TRACE       0x00000002
#define OSK_MAX_TRACE       0x00000003

#define OSK_DEBUG_CHECK     0x00000100
#define OSK_DEBUG_MEMORY    0x00000200
#define OSK_DEBUG_BUFFER    0x00000400
#define OSK_DEBUG_IRP       0x00000800
#define OSK_DEBUG_REFCOUNT  0x00001000
#define OSK_DEBUG_ADDRFILE  0x00002000
#define OSK_DEBUG_DATALINK  0x00004000
#define OSK_DEBUG_ARP       0x00008000
#define OSK_DEBUG_IP        0x00010000
#define OSK_DEBUG_UDP       0x00020000
#define OSK_DEBUG_TCP       0x00040000
#define OSK_DEBUG_ICMP      0x00080000
#define OSK_DEBUG_ROUTER    0x00100000
#define OSK_DEBUG_RCACHE    0x00200000
#define OSK_DEBUG_NCACHE    0x00400000
#define OSK_DEBUG_CPOINT    0x00800000
#define OSK_DEBUG_ULTRA     0xFFFFFFFF

#include <oskittypes.h>

#if DBG

extern OSK_UINT OskitDebugTraceLevel;

#ifdef _MSC_VER

#define OS_DbgPrint(_t_, _x_) \
    if (((OskitDebugTraceLevel & OSK_NORMAL_MASK) >= _t_) || \
        ((OskitDebugTraceLevel & _t_) > OSK_NORMAL_MASK)) { \
        DbgPrint("(%s:%d) ", __FILE__, __LINE__); \
        DbgPrint _x_ ; \
    }

#else /* _MSC_VER */

#define OS_DbgPrint(_t_, _x_) \
    if (((OskitDebugTraceLevel & OSK_NORMAL_MASK) >= _t_) || \
        ((OskitDebugTraceLevel & _t_) > OSK_NORMAL_MASK)) { \
        DbgPrint("(%s:%d)(%s) ", __FILE__, __LINE__, __FUNCTION__); \
        DbgPrint _x_ ; \
    }

#endif /* _MSC_VER */

#if 0
#ifdef ASSERT
#undef ASSERT
#endif

#ifdef NASSERT
#define ASSERT(x)
#else /* NASSERT */
#define ASSERT(x) if (!(x)) { OS_DbgPrint(MIN_TRACE, ("Assertion "#x" failed at %s:%d\n", __FILE__, __LINE__)); KeBugCheck(0); }
#endif /* NASSERT */
#endif

#define ASSERT_IRQL(x) ASSERT(KeGetCurrentIrql() <= (x))

#else /* DBG */

#define OS_DbgPrint(_t_, _x_)

#if 0
#define ASSERT_IRQL(x)
#define ASSERT(x)
#endif

#endif /* DBG */

#ifndef _MSC_VER
#define assert(x) ASSERT(x)
#endif//_MSC_VER
#define assert_irql(x) ASSERT_IRQL(x)

#endif /* __OSKITDEBUG_H */

/* EOF */
