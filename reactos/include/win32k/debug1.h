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
#define assert(x) if (!(x)) {DbgPrint("Assertion "#x" failed at %s:%d\n", __FILE__,__LINE__); KeBugCheck(0); }
#define ASSERT(x) assert(x)
#else
#define assert(x)
#define ASSERT(x)
#endif

#define DPRINT1(args...) do { DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint(args); ExAllocatePool(NonPagedPool,0); } while(0);
#define CHECKPOINT1 do { DbgPrint("%s:%d\n",__FILE__,__LINE__); ExAllocatePool(NonPagedPool,0); } while(0);


#ifdef __NTOSKRNL__
#define DPRINT_CHECKS
#else
#define DPRINT_CHECKS
#endif

#ifndef NDEBUG
#define DPRINT(args...) do { DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#define CHECKPOINT do { DbgPrint("%s:%d\n",__FILE__,__LINE__); ExAllocatePool(NonPagedPool,0); } while(0);
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
