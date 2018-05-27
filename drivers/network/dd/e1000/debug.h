/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Intel PRO/1000 Driver
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

#define DEBUG_MEMORY   0x00000100
#define DEBUG_ULTRA    0xFFFFFFFF

#if DBG

extern ULONG DebugTraceLevel;


#define NDIS_DbgPrint(_t_, _x_) \
    if ((_t_ > NORMAL_MASK) \
        ? (DebugTraceLevel & _t_) > NORMAL_MASK \
        : (DebugTraceLevel & NORMAL_MASK) >= _t_) { \
        DbgPrint("(%s:%d)(%s) ", __FILE__, __LINE__, __FUNCTION__); \
        DbgPrint _x_ ; \
    }


#define ASSERT_IRQL(x) ASSERT(KeGetCurrentIrql() <= (x))
#define ASSERT_IRQL_EQUAL(x) ASSERT(KeGetCurrentIrql() == (x))

#else /* DBG */

#define NDIS_DbgPrint(_t_, _x_)

#define ASSERT_IRQL(x)
#define ASSERT_IRQL_EQUAL(x)
/* #define ASSERT(x) */  /* ndis.h */

#endif /* DBG */


#define assert(x) ASSERT(x)
#define assert_irql(x) ASSERT_IRQL(x)


#define UNIMPLEMENTED \
    NDIS_DbgPrint(MIN_TRACE, ("UNIMPLEMENTED.\n"));


#define UNIMPLEMENTED_DBGBREAK(...) \
    do { \
        NDIS_DbgPrint(MIN_TRACE, ("UNIMPLEMENTED.\n")); \
        DbgPrint("" __VA_ARGS__); \
        DbgBreakPoint(); \
    } while (0)


const char* Oid2Str(IN NDIS_OID Oid);

