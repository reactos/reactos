/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TDI test driver
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

#ifdef _MSC_VER

#define TDI_DbgPrint(_t_, _x_) \
    if (((DebugTraceLevel & NORMAL_MASK) >= _t_) || \
        ((DebugTraceLevel & _t_) > NORMAL_MASK)) { \
        DbgPrint("(%s:%d) ", __FILE__, __LINE__); \
        DbgPrint _x_ ; \
    }

#else /* _MSC_VER */

#define TDI_DbgPrint(_t_, _x_) \
    if (((DebugTraceLevel & NORMAL_MASK) >= _t_) || \
        ((DebugTraceLevel & _t_) > NORMAL_MASK)) { \
        DbgPrint("(%s:%d)(%s) ", __FILE__, __LINE__, __FUNCTION__); \
        DbgPrint _x_ ; \
    }

#endif /* _MSC_VER */

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef NASSERT
#define ASSERT(x)
#else /* NASSERT */
#define ASSERT(x) if (!(x)) { TDI_DbgPrint(MIN_TRACE, ("Assertion "#x" failed at %s:%d\n", __FILE__, __LINE__)); KeBugCheck(0); }
#endif /* NASSERT */

#define ASSERT_IRQL(x) ASSERT(KeGetCurrentIrql() <= (x))

#else /* DBG */

#define TDI_DbgPrint(_t_, _x_)

#define ASSERT_IRQL(x)
#define ASSERT(x)

#endif /* DBG */


#define assert(x) ASSERT(x)
#define assert_irql(x) ASSERT_IRQL(x)


#ifdef _MSC_VER

#define UNIMPLEMENTED \
    TDI_DbgPrint(MIN_TRACE, ("The function at %s:%d is unimplemented, \
        but come back another day.\n", __FILE__, __LINE__));

#else /* _MSC_VER */

#define UNIMPLEMENTED \
    TDI_DbgPrint(MIN_TRACE, ("%s at %s:%d is unimplemented, \
        but come back another day.\n", __FUNCTION__, __FILE__, __LINE__));

#endif /* _MSC_VER */


#define CHECKPOINT \
do { TDI_DbgPrint(MIN_TRACE, ("%s:%d\n", __FILE__, __LINE__)); } while(0);

#endif /* __DEBUG_H */

/* EOF */
