/* $Id: trap.s,v 1.1 2000/07/04 08:52:41 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/trap.s
 * PURPOSE:         2E trap handler
 * PROGRAMMER:      David Welch (david.welch@seh.ox.ac.uk)
 * UPDATE HISTORY:
 *                  ???
 */

#include <ddk/status.h>
#include <internal/i386/segment.h>

.globl _PsBeginThreadWithContextInternal

_PsBeginThreadWithContextInternal:
     call _PiBeforeBeginThread
     popl %eax
     popl %eax
     popl %eax
     popl %eax
     popl %eax
     popl %eax
     popl %eax
     addl $112,%esp
     popl %gs
     popl %fs
     popl %es
     popl %ds
     popl %edi
     popl %esi
     popl %ebx
     popl %edx
     popl %ecx
     popl %eax
     popl %ebp
     iret

.globl _interrupt_handler2e
_interrupt_handler2e:

	   /* Save the user context */
	   pushl %ebp       /* Ebp */

	   pushl %eax       /* Eax */
	   pushl %ecx       /* Ecx */
	   pushl %edx       /* Edx */
	   pushl %ebx       /* Ebx */
	   pushl %esi       /* Esi */
	   pushl %edi       /* Edi */

	   pushl %ds        /* SegDs */
	   pushl %es        /* SegEs */
	   pushl %fs        /* SegFs */
	   pushl %gs        /* SegGs */

	   subl $112,%esp  /* FloatSave */

	   pushl $0         /* Dr7 */
	   pushl $0         /* Dr6 */
	   pushl $0         /* Dr3 */
	   pushl $0         /* Dr2 */
	   pushl $0         /* Dr1 */
	   pushl $0         /* Dr0 */
	   
	   pushl $0         /* ContextFlags */

           /*  Set ES to kernel segment  */
           movw $KERNEL_DS,%bx
           movw %bx,%es

	   /* Save pointer to user context as argument to system call */
	   pushl %esp

           /*  Allocate new Kernel stack frame  */
           movl %esp,%ebp

           /*  Users's current stack frame pointer is source  */
           movl %edx,%esi

           /*  Determine system service table to use  */
           cmpl  $0x0fff, %eax
           ja    useShadowTable

           /*  Check to see if EAX is valid/inrange  */
           cmpl  %es:_KeServiceDescriptorTable + 8, %eax
           jbe   serviceInRange
           movl  $STATUS_INVALID_SYSTEM_SERVICE, %eax
           jmp   done

serviceInRange:

           /*  Allocate room for argument list from kernel stack  */
           movl  %es:_KeServiceDescriptorTable + 12, %ecx
           movl  %es:(%ecx, %eax, 4), %ecx
           subl  %ecx, %esp

           /*  Copy the arguments from the user stack to the kernel stack  */
           movl %esp,%edi
           rep  movsb

           /*  DS is now also kernel segment  */
           movw %bx, %ds

	   /* Call system call hook */
	   pushl %eax
	   call _KiSystemCallHook
	   popl %eax

           /*  Make the system service call  */
           movl  %es:_KeServiceDescriptorTable, %ecx
           movl  %es:(%ecx, %eax, 4), %eax
           call  *%eax

#if CHECKED
           /*  Bump Service Counter  */
#endif

           /*  Deallocate the kernel stack frame  */
           movl %ebp,%esp

	   /* Call the post system call hook and deliver any pending APCs */
	   pushl %eax
	   call _KiAfterSystemCallHook
	   addl $8,%esp

           jmp  done

useShadowTable:

           subl  $0x1000, %eax

           /*  Check to see if EAX is valid/inrange  */
           cmpl  %es:_KeServiceDescriptorTableShadow + 24, %eax
           jbe   shadowServiceInRange
           movl  $STATUS_INVALID_SYSTEM_SERVICE, %eax
           jmp   done

shadowServiceInRange:

           /*  Allocate room for argument list from kernel stack  */
           movl  %es:_KeServiceDescriptorTableShadow + 28, %ecx
           movl  %es:(%ecx, %eax, 4), %ecx
           subl  %ecx, %esp

           /*  Copy the arguments from the user stack to the kernel stack  */
           movl %esp,%edi
           rep movsb

           /*  DS is now also kernel segment  */
           movw %bx,%ds

	   /* Call system call hook */
	   pushl %eax
	   call _KiSystemCallHook
           popl %eax
 
           /*  Make the system service call  */
           movl  %es:_KeServiceDescriptorTableShadow + 16, %ecx
           movl  %es:(%ecx, %eax, 4), %eax
           call  *%eax

#if CHECKED
           /*  Bump Service Counter  */
#endif

           /*  Deallocate the kernel stack frame  */
           movl %ebp,%esp

	   /* Call the post system call hook and deliver any pending APCs */
	   pushl %eax
	   call _KiAfterSystemCallHook
	   addl $8,%esp

done:

           /*  Restore the user context  */
	   addl $4,%esp    /* UserContext */
	   addl $24,%esp   /* Dr[0-3,6-7] */
	   addl $112,%esp  /* FloatingSave */
	   popl %gs        /* SegGs */
	   popl %fs        /* SegFs */
	   popl %es        /* SegEs */
	   popl %ds        /* SegDs */

	   popl %edi       /* Edi */
	   popl %esi       /* Esi */
	   popl %ebx       /* Ebx */
	   popl %edx       /* Edx */
	   popl %ecx       /* Ecx */
	   addl $4,%esp       /* Eax (Not restored) */

	   popl %ebp       /* Ebp */

           iret

/* EOF */
