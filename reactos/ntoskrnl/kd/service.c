/* $Id: service.c,v 1.3 2000/03/04 22:02:13 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kd/service.c
 * PURPOSE:         Debug service dispatcher
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  17/01/2000: Created
 */

#include <ddk/ntddk.h>
#include <internal/i386/segment.h>
#include <internal/kd.h>

/* FUNCTIONS ***************************************************************/

/*
 * Note: DON'T CHANGE THIS FUNCTION!!!
 *       DON'T CALL HalDisplayString OR SOMETING ELSE!!!
 *       You'll only break the serial/bochs debugging feature!!!
 */

ULONG
KdpServiceDispatcher (
        ULONG Service,
        PVOID Context1,
        PVOID Context2)
{
    ULONG Result = 0;

    switch (Service)
    {
        case 1: /* DbgPrint */
            Result = KdpPrintString ((PANSI_STRING)Context1);
            break;

        default:
            HalDisplayString ("Invalid debug service call!\n");
            break;
    }

    return Result;
}


#define _STR(x) #x
#define STR(x) _STR(x)

void interrupt_handler2d(void);
   __asm__("\n\t.global _interrupt_handler2d\n\t"
	   "_interrupt_handler2d:\n\t"
	   
	   /* Save the user context */
	   "pushl %ebp\n\t"       /* Ebp */
	   
	   "pushl %eax\n\t"       /* Eax */
	   "pushl %ecx\n\t"       /* Ecx */
	   "pushl %edx\n\t"       /* Edx */
	   "pushl %ebx\n\t"       /* Ebx */
	   "pushl %esi\n\t"       /* Esi */
	   "pushl %edi\n\t"       /* Edi */
	   
	   "pushl %ds\n\t"        /* SegDs */
	   "pushl %es\n\t"        /* SegEs */
	   "pushl %fs\n\t"        /* SegFs */
	   "pushl %gs\n\t"        /* SegGs */
	   
	   "subl $112,%esp\n\t"   /* FloatSave */
	   
	   "pushl $0\n\t"         /* Dr7 */
	   "pushl $0\n\t"         /* Dr6 */
	   "pushl $0\n\t"         /* Dr3 */
	   "pushl $0\n\t"         /* Dr2 */
	   "pushl $0\n\t"         /* Dr1 */
	   "pushl $0\n\t"         /* Dr0 */
	   
	   "pushl $0\n\t"         /* ContextFlags */
	   
	   /*  Set ES to kernel segment  */
	   "movw  $"STR(KERNEL_DS)",%bx\n\t"
	   "movw %bx,%es\n\t"
	   
           /* FIXME: check to see if SS is valid/inrange */
           
           /*  DS is now also kernel segment */
           "movw %bx,%ds\n\t"
           
           /* Call debug service dispatcher */
           "pushl %edx\n\t"
           "pushl %ecx\n\t"
           "pushl %eax\n\t"
	   "call _KdpServiceDispatcher\n\t"
	   "addl $12,%esp\n\t"   /* restore stack pointer */

	   /*  Restore the user context  */
	   "addl $4,%esp\n\t"    /* UserContext */
	   "addl $24,%esp\n\t"   /* Dr[0-3,6-7] */
	   "addl $112,%esp\n\t"  /* FloatingSave */
	   "popl %gs\n\t"        /* SegGs */
	   "popl %fs\n\t"        /* SegFs */
	   "popl %es\n\t"        /* SegEs */
	   "popl %ds\n\t"        /* SegDs */
	   
	   "popl %edi\n\t"       /* Edi */
	   "popl %esi\n\t"       /* Esi */
	   "popl %ebx\n\t"       /* Ebx */
	   "popl %edx\n\t"       /* Edx */
	   "popl %ecx\n\t"       /* Ecx */
	   "addl $4,%esp\n\t"    /* Eax (Not restored) */
	   
	   "popl %ebp\n\t"       /* Ebp */
	   
	   "iret\n\t");

/* EOF */
