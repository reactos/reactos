/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/debug.h
 * PURPOSE:     Debugging support macros
 * DEFINES:     DBG     - Enable debug output
 *              NASSERT - Disable assertions
 */
#ifndef __DEBUG_H
#define __DEBUG_H

#define MIN_TRACE      ((1 << DPFLTR_WARNING_LEVEL))
#define MID_TRACE      ((1 << DPFLTR_WARNING_LEVEL) | (1 << DPFLTR_TRACE_LEVEL))
#define MAX_TRACE      ((1 << DPFLTR_WARNING_LEVEL) | (1 << DPFLTR_TRACE_LEVEL) | (1 << DPFLTR_INFO_LEVEL))

#define DEBUG_CHECK    0x00000100
#define DEBUG_MEMORY   0x00000200
#define DEBUG_PBUFFER  0x00000400
#define DEBUG_IRP      0x00000800
#define DEBUG_TCPIF    0x00001000
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
#define DEBUG_LOCK     0x01000000
#define DEBUG_INFO     0x02000000
#define DEBUG_ULTRA    0x7FFFFFFF

#ifdef DBG

#define REMOVE_PARENS(...) __VA_ARGS__
#define TI_DbgPrint(_t_, _x_) \
    DbgPrintEx(DPFLTR_TCPIP_ID, (_t_) | DPFLTR_MASK, "(%s:%d) ", __FILE__, __LINE__), \
    DbgPrintEx(DPFLTR_TCPIP_ID, (_t_) | DPFLTR_MASK, REMOVE_PARENS _x_)

#else /* DBG */

#define TI_DbgPrint(_t_, _x_)

#endif /* DBG */


#define assert(x) ASSERT(x)
#define assert_irql(x) ASSERT_IRQL(x)


#ifdef _MSC_VER

#define UNIMPLEMENTED \
    TI_DbgPrint(MIN_TRACE, ("The function at %s:%d is unimplemented, \
        but come back another day.\n", __FILE__, __LINE__));

#else /* _MSC_VER */

#define UNIMPLEMENTED \
    TI_DbgPrint(MIN_TRACE, ("(%s:%d)(%s) is unimplemented, \
        but come back another day.\n", __FILE__, __LINE__, __FUNCTION__));

#endif /* _MSC_VER */


#define CHECKPOINT \
    do { TI_DbgPrint(DEBUG_CHECK, ("(%s:%d)\n", __FILE__, __LINE__)); } while(0);

#define CP CHECKPOINT

#include <memtrack.h>

#define ASSERT_KM_POINTER(_x) \
   ASSERT(((PVOID)_x) != (PVOID)0xcccccccc); \
   ASSERT(((PVOID)_x) >= (PVOID)0x80000000);

#endif /* __DEBUG_H */

/* EOF */
