/* $Id: trap.s,v 1.4 2000/10/11 20:50:34 dwelch Exp $
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
#include <internal/ps.h>
#include <ddk/defines.h>

/*
 *
 */

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
;     popl %fs
     movl $TEB_SELECTOR, %ax
     movl %ax, %fs
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

/*
 *
 */
.globl _old_interrupt_handler2e
_old_interrupt_handler2e:

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


/*
 *
 */

.globl _exception_handler0
_exception_handler0:
                pushl $0
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $0                        
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler1
_exception_handler1:
                pushl $0
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $1
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret


.globl _exception_handler2
_exception_handler2:
                pushl $0
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $2
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler3
_exception_handler3:
                pushl $0
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $3
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler4
_exception_handler4:
                pushl $0
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $4
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler5
_exception_handler5:
                pushl $0
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $5
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler6
_exception_handler6:
                pushl $0
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $6
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler7
_exception_handler7:
                pushl $0
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $7
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler8
_exception_handler8:
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $8
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler9
_exception_handler9:
                pushl $0
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $1
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler10
_exception_handler10:
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $10
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler11
_exception_handler11:
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $1
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler12
_exception_handler12:
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $1
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler13
_exception_handler13:
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $1
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler14
_exception_handler14:
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $14
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler15
_exception_handler15:
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $15
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret

.globl _exception_handler16
_exception_handler16:
                pushl %gs 
                pushl %fs 
                pushl %es 
                pushl %ds    
                pushl $16
                pusha                          
                movw $KERNEL_DS,%ax        
                movw %ax,%ds      
                movw %ax,%es      
                movw %ax,%fs      
                movw %ax,%gs      
                call _exception_handler        
                popa 
                addl $4,%esp                   
                popl %ds      
                popl %es      
                popl %fs      
                popl %gs      
                addl $4,%esp 
                iret
 
.globl _exception_handler_unknown
_exception_handler_unknown:
                pushl $0
                pushl %gs
                pushl %fs
                pushl %es
                pushl %ds
                pushl %ds
                pushl $0xff
                pusha                         
                movw $KERNEL_DS,%ax
                movw %ax,%ds
                movw %ax,%es
                movw %ax,%fs
                movw %ax,%gs
                call _exception_handler
                popa             
                addl $8,%esp
                iret


/* EOF */
