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
 * NOTE: Define NDEBUG before including this header to disable debugging
 * macros
 */

#ifndef __INTERNAL_DEBUG
#define __INTERNAL_DEBUG

#define UNIMPLEMENTED do {DbgPrint("%s at %s:%d is unimplemented, have a nice day\n",__FUNCTION__,__FILE__,__LINE__); for(;;);  } while(0);

/*  FIXME: should probably remove this later  */
#if !defined(CHECKED) && !defined(NDEBUG)
#define CHECKED
#endif

#ifndef NASSERT
#ifndef assert
#define assert(x) if (!(x)) {DbgPrint("Assertion "#x" failed at %s:%d\n", __FILE__,__LINE__); KeBugCheck(0); }
#endif
#ifndef ASSERT
#define ASSERT(x) assert(x)
#endif
#else
#ifndef assert
#define assert(x)
#endif
#ifndef ASSERT
#define ASSERT(x)
#endif
#endif

#define DPRINT1 DbgPrint("(%s:%d) ",__FILE__,__LINE__), DbgPrint
#define CHECKPOINT1 DbgPrint("%s:%d\n",__FILE__,__LINE__);


#ifdef __NTOSKRNL__
#define DPRINT_CHECKS
#else
#define DPRINT_CHECKS
#endif

#ifndef NDEBUG
#define DPRINT DbgPrint("(%s:%d) ",__FILE__,__LINE__), DbgPrint
#define CHECKPOINT DbgPrint("%s:%d\n",__FILE__,__LINE__);
#else
#define DPRINT(args...)
#define CHECKPOINT
#endif /* NDEBUG */

/*
 * FUNCTION: Assert a maximum value for the current irql
 * ARGUMENTS:
 *        x = Maximum irql
 */
#define ASSERT_IRQL(x) assert(KeGetCurrentIrql()<=(x))
#define assert_irql(x) assert(KeGetCurrentIrql()<=(x))

#endif /* __INTERNAL_DEBUG */
