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

#define NORMAL_MASK    0x000000FF
#define SPECIAL_MASK   0xFFFFFF00
#define MIN_TRACE      0x00000001
#define MID_TRACE      0x00000002
#define MAX_TRACE      0x00000003

#define DEBUG_ADAPTER  0x00000100
#define DEBUG_ULTRA    0xFFFFFFFF

#if DBG

extern unsigned long debug_trace_level;

#ifdef _MSC_VER

#define DH_DbgPrint(_t_, _x_) \
    if (((debug_trace_level & NORMAL_MASK) >= _t_) || \
        ((debug_trace_level & _t_) > NORMAL_MASK)) { \
        DbgPrint("(%s:%d) ", __FILE__, __LINE__); \
        DbgPrint _x_ ; \
    }

#else /* _MSC_VER */

#define DH_DbgPrint(_t_, _x_) \
    if (((debug_trace_level & NORMAL_MASK) >= _t_) || \
        ((debug_trace_level & _t_) > NORMAL_MASK)) { \
        DbgPrint("(%s:%d)(%s) ", __FILE__, __LINE__, __FUNCTION__); \
        DbgPrint _x_ ; \
    }

#endif /* _MSC_VER */

#else /* DBG */

#define DH_DbgPrint(_t_, _x_)

#endif /* DBG */

#endif /* __DEBUG_H */

/* EOF */
