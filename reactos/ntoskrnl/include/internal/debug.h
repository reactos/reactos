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

#ifdef CHECKPOINT
#undef CHECKPOINT
#endif

#ifdef DPRINT
#undef DPRINT
#endif

#ifndef NDEBUG
#ifdef __GNUC__ /* using GNU C/C99 macro ellipsis */
#define DPRINT(args...) do { DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0)
#else
#define DPRINT DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint
#endif
#define CHECKPOINT do { DbgPrint("%s:%d\n",__FILE__,__LINE__); } while(0)
#else /* NDEBUG */
#ifdef __GNUC__ /* using GNU C/C99 macro ellipsis */
#define DPRINT(args...)
#else
#define DPRINT
#endif
#define CHECKPOINT
#endif /* NDEBUG */

#ifndef __INTERNAL_DEBUG
#define __INTERNAL_DEBUG

#if defined(_MSC_VER) && (_MSC_VER < 1300)
/* TODO: Verify which version the MS compiler learned the __FUNCTION__ macro */
#define __FUNCTION__ "<unknown>"
#endif
#define UNIMPLEMENTED DbgPrint("%s at %s:%d is unimplemented, have a nice day\n",__FUNCTION__,__FILE__,__LINE__);


#ifdef assert
#undef assert
#endif

#ifdef DBG

/* Print if using a "checked" version */
#ifdef __GNUC__ /* using GNU C/C99 macro ellipsis */
#define CPRINT(args...) do { DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0)
#else
#define CPRINT DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint
#endif

#ifdef __GNUC__ /* using GNU C/C99 macro ellipsis */
#define DPRINT1(args...) do { DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0)
#else
#define DPRINT1 DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint
#endif

#else /* DBG */

#ifdef __GNUC__ /* using GNU C/C99 macro ellipsis */
#define CPRINT(args...)
#define DPRINT1(args...)
#else
#define CPRINT
#define DPRINT1
#endif

#endif /* DBG */

#define CHECKPOINT1 do { DbgPrint("%s:%d\n",__FILE__,__LINE__); } while(0)

/*
 * FUNCTION: Assert a maximum value for the current irql
 * ARGUMENTS:
 *        x = Maximum irql
 */
#define ASSERT_IRQL_LESS_OR_EQUAL(x) ASSERT(KeGetCurrentIrql()<=(x))
#define ASSERT_IRQL(x) ASSERT_IRQL_LESS_OR_EQUAL(x)
#define ASSERT_IRQL_EQUAL(x) ASSERT(KeGetCurrentIrql()==(x))
#define ASSERT_IRQL_LESS(x) ASSERT(KeGetCurrentIrql()<(x))
#define assert_irql(x) assert(KeGetCurrentIrql()<=(x))

#endif /* __INTERNAL_DEBUG */
