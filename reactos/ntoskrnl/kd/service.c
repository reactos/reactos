/* $Id: service.c,v 1.8 2004/01/08 18:54:12 jfilby Exp $
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

#if defined(__GNUC__)

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

#elif defined(_MSC_VER)

__declspec(naked)
void interrupt_handler2d()
{
	__asm
	{
		/* Save the user context */
		push ebp
		push eax
		push ecx
		push edx
		push ebx
		push esi
		push edi

		push ds
		push es
		push fs
		push gs

		sub esp, 112  /* FloatSave */

		mov ebx, eax
		mov eax, dr7 __asm push eax
		mov eax, dr6 __asm push eax
		mov eax, dr3 __asm push eax
		mov eax, dr2 __asm push eax
		mov eax, dr1 __asm push eax
		mov eax, dr0 __asm push eax
		mov eax, ebx

		push 0		/* ContextFlags */

		/*  Set ES to kernel segment  */
		mov bx, KERNEL_DS
		mov es, bx

		/* FIXME: check to see if SS is valid/inrange */

		mov ds, bx	/*  DS is now also kernel segment */

			/* Call debug service dispatcher */
		push edx
		push ecx
		push eax
		call KdpServiceDispatcher
		add esp, 12		/* restore stack pointer */

		/*  Restore the user context  */
		add esp, 4			/* UserContext */
		pop eax __asm mov dr0, eax
		pop eax __asm mov dr1, eax
		pop eax __asm mov dr2, eax
		pop eax __asm mov dr3, eax
		pop eax __asm mov dr6, eax
		pop eax __asm mov dr7, eax
		add esp, 112		/* FloatingSave */
		pop gs
		pop fs
		pop es
		pop ds

		pop edi
		pop esi
		pop ebx
		pop edx
		pop ecx
		add esp, 4		/* Eax Not restored */

		pop ebp

		iretd
	}
}

#else
#error Unknown compiler for inline assembler
#endif

/* EOF */
