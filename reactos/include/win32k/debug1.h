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

extern unsigned int old_idt[256][2];
//extern unsigned int idt;
extern unsigned int old_idt_valid;

#ifdef __NTOSKRNL__
//#define DPRINT_CHECKS ExAllocatePool(NonPagedPool,0); assert(old_idt_valid || (!memcmp(old_idt,KiIdt,256*2)));
//#define DPRINT_CHECKS ExAllocatePool(NonPagedPool,0);
#define DPRINT_CHECKS
#else
#define DPRINT_CHECKS
#endif

#ifndef NDEBUG
#define OLD_DPRINT(fmt,args...) do { DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint(fmt,args); } while(0);
#define DPRINT(args...) do { DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint(args); DPRINT_CHECKS  } while(0);
#define CHECKPOINT do { DbgPrint("%s:%d\n",__FILE__,__LINE__); ExAllocatePool(NonPagedPool,0); } while(0);
#else
//#define DPRINT(args...) do { DPRINT_CHECKS } while (0);
#define DPRINT(args...)
#define OLD_DPRINT(args...)
#define CHECKPOINT
#endif /* NDEBUG */

/*
 * FUNCTION: Assert a maximum value for the current irql
 * ARGUMENTS:
 *        x = Maximum irql
 */
#define ASSERT_IRQL(x) assert(KeGetCurrentIrql()<=(x))
#define assert_irql(x) assert(KeGetCurrentIrql()<=(x))

#define HBP_EXECUTE     (0)
#define HBP_WRITE       (1)
#define HBP_READWRITE   (3)

#define HBP_BYTE        (0)
#define HBP_WORD        (1)
#define HBP_DWORD       (3)

/*
 * FUNCTION: Sets a hardware breakpoint
 * ARGUMENTS:
 *          i = breakpoint to set (0 to 3)
 *          addr = linear address to break on
 *          type = Type of access to break on
 *          len = length of the variable to watch
 * NOTES:
 *       The variable to watch must be aligned to its length (i.e. a dword
 *	 breakpoint must be aligned to a dword boundary)
 *
 *       A fatal exception will be generated on the access to the variable.
 *       It is (at the moment) only really useful for catching undefined
 *       pointers if you know the variable effected but not the buggy
 *       routine.
 *
 * FIXME: Extend to call out to kernel debugger on breakpoint
 *        Add support for I/O breakpoints
 * REFERENCES: See the i386 programmer manual for more details
 */
void set_breakpoint(unsigned int i, unsigned int addr, unsigned int type,
		    unsigned int len);


#endif /* __INTERNAL_DEBUG */
