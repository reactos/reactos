/*
 *  ReactOS kernel
 *  Copyright (C) 2000  David Welch <welch@cwcom.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: syscall.S,v 1.1 2000/12/10 23:42:00 dwelch Exp $
 *
 * FILE:            ntoskrnl/hal/x86/syscall.s
 * PURPOSE:         2E trap handler
 * PROGRAMMER:      David Welch (david.welch@seh.ox.ac.uk)
 * UPDATE HISTORY:
 *                  ???
 */

#include <ddk/status.h>
#include <internal/i386/segment.h>
#include <internal/ps.h>
#include <ddk/defines.h>

/*
 *
 */
.globl _interrupt_handler2e
_interrupt_handler2e:

           /* Construct a trap frame on the stack */

	   /* Error code */
	   pushl	$0     
	   pushl	%ebp
	   pushl	%ebx
	   pushl	%esi
	   pushl	%edi
	   pushl	%fs
	   /* Load PCR selector into fs */
	   movl		$PCR_SELECTOR, %ebx 
	   movl		%ebx, %fs

	   /* Save the old exception list */
	   movl         %fs:KPCR_EXCEPTION_LIST, %ebx
	   pushl	%ebx
	   /* Put the exception handler chain terminator */
	   movl         $0xffffffff, %fs:KPCR_EXCEPTION_LIST
	   /* Get a pointer to the current thread */
	   movl         %fs:KPCR_CURRENT_THREAD, %esi
	   /* Save the old previous mode */
	   movl         $0, %ebx
	   movb         %ss:KTHREAD_PREVIOUS_MODE(%esi), %bl
	   pushl        %ebx
           /* Set the new previous mode based on the saved CS selector */
	   movl	        0x24(%esp), %ebx
	   cmpl         $KERNEL_CS, %ebx
	   jne          L1
	   movb         $KernelMode, %ss:KTHREAD_PREVIOUS_MODE(%esi)
	   jmp          L3
L1:
	   movb         $UserMode, %ss:KTHREAD_PREVIOUS_MODE(%esi)
L3:

	   /* Save other registers */	   
	   pushl %eax
	   pushl %ecx
	   pushl %edx
	   pushl %ds
	   pushl %es
	   pushl %gs
	   pushl $0     /* DR7 */
	   pushl $0     /* DR6 */
	   pushl $0     /* DR3 */
	   pushl $0     /* DR2 */
	   pushl $0     /* DR1 */
	   pushl $0     /* DR0 */
	   pushl $0     /* XXX: TempESP */
	   pushl $0     /* XXX: TempCS */
	   pushl $0     /* XXX: DebugPointer */
	   pushl $0     /* XXX: DebugArgMark */
	   pushl $0     /* XXX: DebugEIP */
	   pushl $0     /* XXX: DebugEBP */

           /* Load the segment registers */
	   movl  $KERNEL_DS, %ebx
	   movl  %ebx, %ds
	   movl  %ebx, %es
	   movl  %ebx, %gs

           /* Save the old trap frame pointer (over the EDX register??) */
           movl KTHREAD_TRAP_FRAME(%esi), %ebx
	   movl %ebx, 0x3C(%esp)
	
	   /* Save a pointer to the trap frame in the TCB */
	   movl	%esp, KTHREAD_TRAP_FRAME(%esi)
	 
           /*  Set ES to kernel segment  */
           movw $KERNEL_DS,%bx
           movw %bx,%es

           /*  Allocate new Kernel stack frame  */
           movl %esp,%ebp

           /*  Users's current stack frame pointer is source  */
           movl %edx,%esi

           /*  Determine system service table to use  */
           cmpl  $0x0fff, %eax
           ja    new_useShadowTable

           /*  Check to see if EAX is valid/inrange  */
           cmpl  %es:_KeServiceDescriptorTable + 8, %eax
           jbe   new_serviceInRange
           movl  $STATUS_INVALID_SYSTEM_SERVICE, %eax
           jmp   new_done

new_serviceInRange:

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
	   pushl %ebp
	   pushl %eax
	   call _KiAfterSystemCallHook
	   addl $8,%esp

           jmp  new_done

new_useShadowTable:

           subl  $0x1000, %eax

           /*  Check to see if EAX is valid/inrange  */
           cmpl  %es:_KeServiceDescriptorTableShadow + 24, %eax
           jbe   new_shadowServiceInRange
           movl  $STATUS_INVALID_SYSTEM_SERVICE, %eax
           jmp   new_done

new_shadowServiceInRange:

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
	   pushl %esp
	   pushl %eax
	   call _KiAfterSystemCallHook
	   addl $8,%esp

new_done:

           /* Restore the user context */
	   /* Get a pointer to the current thread */
           movl %fs:0x124, %esi
	
           /* Restore the old trap frame pointer */
           movl 0x3c(%esp), %ebx
	   movl %ebx, KTHREAD_TRAP_FRAME(%esi)
	
	   /* Skip debug information and unsaved registers */
	   addl	$0x30, %esp
	   popl %gs
	   popl %es
	   popl %ds
	   popl %edx
	   popl %ecx
	   addl $0x4, %esp   /* Don't restore eax */

	   /* Restore the old previous mode */
	   popl %ebx
	   movb %bl, %ss:KTHREAD_PREVIOUS_MODE(%esi)

	   /* Restore the old exception handler list */
	   popl %ebx
	   movl %ebx, %fs:KPCR_EXCEPTION_LIST
	
	   popl %fs 
	   popl %edi
	   popl %esi
	   popl %ebx
	   popl %ebp
	   addl $0x4, %esp  /* Ignore error code */
		
           iret
