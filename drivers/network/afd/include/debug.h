/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver
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
#define DEBUG_IRP      0x00000200
#define DEBUG_ULTRA    0xFFFFFFFF

#if DBG

extern DWORD DebugTraceLevel;

#ifdef _MSC_VER

#define AFD_DbgPrint(_t_, _x_) \
    if ((_t_ > NORMAL_MASK) \
        ? (DebugTraceLevel & _t_) > NORMAL_MASK \
        : (DebugTraceLevel & NORMAL_MASK) >= _t_) { \
        DbgPrint("(%s:%d) ", __FILE__, __LINE__); \
        DbgPrint _x_ ; \
    }

#else /* _MSC_VER */

#define AFD_DbgPrint(_t_, _x_) \
    if ((_t_ > NORMAL_MASK) \
        ? (DebugTraceLevel & _t_) > NORMAL_MASK \
        : (DebugTraceLevel & NORMAL_MASK) >= _t_) { \
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
#define ASSERT(x) if (!(x)) { AFD_DbgPrint(MIN_TRACE, ("Assertion "#x" failed at %s:%d\n", __FILE__, __LINE__)); DbgBreakPoint(); }
#endif /* NASSERT */

#else /* DBG */

#define AFD_DbgPrint(_t_, _x_)

#define ASSERTKM(x)
#ifndef ASSERT
#define ASSERT(x)
#endif

#endif /* DBG */


#undef assert
#define assert(x) ASSERT(x)


#ifdef _MSC_VER

#define UNIMPLEMENTED \
    AFD_DbgPrint(MIN_TRACE, ("The function at %s:%d is unimplemented, \
        but come back another day.\n", __FILE__, __LINE__));

#else /* _MSC_VER */

#define UNIMPLEMENTED \
    AFD_DbgPrint(MIN_TRACE, ("%s at %s:%d is unimplemented, " \
        "but come back another day.\n", __FUNCTION__, __FILE__, __LINE__));

#endif /* _MSC_VER */


#define CHECKPOINT \
    AFD_DbgPrint(DEBUG_CHECK, ("\n"));

#define CP CHECKPOINT

/* EOF */
