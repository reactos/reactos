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

#ifndef __INTERNAL_DEBUG
#define __INTERNAL_DEBUG

#include <internal/ntoskrnl.h>
#include <internal/dbg.h>
#include <roscfg.h>

#if defined(_MSC_VER) && (_MSC_VER < 1300)
/* TODO: Verify which version the MS compiler learned the __FUNCTION__ macro */
#define __FUNCTION__ "<unknown>"
#endif
#define UNIMPLEMENTED do {DbgPrint("%s at %s:%d is unimplemented, have a nice day\n",__FUNCTION__,__FILE__,__LINE__); for(;;);  } while(0)

#ifdef DBG

/* Assert only on "checked" version */
#ifndef NASSERT
#ifdef assert
#undef assert
#endif
#define assert(x) if (!(x)) {DbgPrint("Assertion "#x" failed at %s:%d\n", __FILE__,__LINE__); KeBugCheck(0); }

#define assertmsg(_c_, _m_) \
  if (!(_c_)) { \
      DbgPrint("(%s:%d)(%s) ", __FILE__, __LINE__, __FUNCTION__); \
      DbgPrint _m_ ; \
      KeBugCheck(0); \
  }

#else

#ifdef assert
#undef assert
#endif
#define assert(x)
#define assertmsg(_c_, _m_)

#endif

/* Print if using a "checked" version */
#ifdef __GNUC__ /* using GNU C/C99 macro ellipsis */
#define CPRINT(args...) do { DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0)
#else
#define CPRINT DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint
#endif

#else /* DBG */

#define CPRINT(args...)
#ifdef assert
#undef assert
#endif
#define assert(x)
#define assertmsg(_c_, _m_)

#endif /* DBG */

#ifdef __GNUC__ /* using GNU C/C99 macro ellipsis */
#define DPRINT1(args...) do { DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0)
#else
#define DPRINT1 DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint
#endif

#define CHECKPOINT1 do { DbgPrint("%s:%d\n",__FILE__,__LINE__); } while(0)

#if defined(KDBG) && defined(NDEBUG) && defined(__NTOSKRNL__)

#define DPRINT(args...) do { \
  if (DbgShouldPrint(__FILE__)) { \
    DbgPrint("(%s:%d) ",__FILE__,__LINE__); \
    DbgPrint(args); \
  } \
} while(0)

#define CHECKPOINT

#else /* KDBG && NDEBUG && __NTOSKRNL__ */

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

#endif /* KDBG && NDEBUG */

/*
 * FUNCTION: Assert a maximum value for the current irql
 * ARGUMENTS:
 *        x = Maximum irql
 */
#define ASSERT_IRQL(x) assert(KeGetCurrentIrql()<=(x))
#define assert_irql(x) assert(KeGetCurrentIrql()<=(x))

#endif /* __INTERNAL_DEBUG */
