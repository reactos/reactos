/* $Id: trap.s,v 1.2 2000/07/06 14:34:50 dwelch Exp $
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

/*
 *
 */

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
