/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/internal/debug.h
 * PURPOSE:         Useful debugging macros
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                28/05/98: Created
 */

/*
 * NOTES: Define DBG in configuration file for "checked" version
 *        Define NDEBUG before including this header to disable debugging
 *        macros
 *        Define NASSERT before including this header to disable assertions
 */

#include <reactos/debug.h>

#ifndef __NTOSKRNL_DEBUG
#define __NTOSKRNL_DEBUG

#include <reactos/debug.h>

#if defined(_MSC_VER) && (_MSC_VER < 1300)
/* TODO: Verify which version the MS compiler learned the __FUNCTION__ macro */
#define __FUNCTION__ "<unknown>"
#endif

#define CPRINT DPRINT1

/*
 * FUNCTION: Assert a maximum value for the current irql
 * ARGUMENTS:
 *        x = Maximum irql
 */
#define ASSERT_IRQL_LESS_OR_EQUAL(x) ASSERT(KeGetCurrentIrql()<=(x))
#define ASSERT_IRQL_EQUAL(x) ASSERT(KeGetCurrentIrql()==(x))
#define ASSERT_IRQL_LESS(x) ASSERT(KeGetCurrentIrql()<(x))

#endif /* __NTOSKRNL_DEBUG */
