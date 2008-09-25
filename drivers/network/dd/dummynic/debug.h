/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     Dummy NIC Driver
 * FILE:        debug.h
 * PURPOSE:     Debugging support macros
 */
#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef DBG

#define ASSERT_IRQL_EQUAL(x) ASSERT(KeGetCurrentIrql() == (x))

#else /* DBG */

#define ASSERT_IRQL_EQUAL(x)

#endif /* DBG */

#endif /* __DEBUG_H */

/* EOF */
