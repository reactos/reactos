/* $Id: fiber.S,v 1.2 2003/06/09 22:39:48 hyperion Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS system libraries
 * FILE:       lib/kernel32/thread/i386/fiber.S
 * PURPOSE:    Fiber context switch code for the x86 architecture
 * PROGRAMMER: KJK::Hyperion <noog@libero.it>
 *
 * UPDATE HISTORY:
 *             28/05/2003 - created
 *
 */

.extern _DbgPrint

.globl _SwitchToFiber@4

ErrStr:
 .ascii \
"(KERNEL32:" __FILE__ ") Saving and restoring the floating point context \
currently unimplemented\n\0"

_SwitchToFiber@4:

 movl %fs:0x18, %ecx           /* Teb = NtCurrentTeb() */

 /* get the current fiber */
 movl 0x10(%ecx), %eax         /* Fiber = Teb->Tib.Fib.FiberData */

 /* store the volatile context of the current fiber */
 movl 0x0(%ecx),   %edx
 movl %edx,        0x4(%eax)   /* Fiber->ExceptionList = Teb->ExceptionList */
 movl 0x4(%ecx),   %edx
 movl %edx,        0x8(%eax)   /* Fiber->StackBase = Teb->StackBase */
 movl 0x8(%ecx),   %edx
 movl %edx,        0xC(%eax)   /* Fiber->StackLimit = Teb->StackLimit */
 movl 0xE0C(%ecx), %edx
 movl %edx,        0x10(%eax)  /* Fiber->StackBottom = Teb->DeallocationStack */
 movl 0x0(%esp),   %edx
 movl %edx,        0x18(%eax)  /* Fiber->Eip = [esp + 0] */
 movl %esp,        %edx
 addl $0x8,        %edx        
 movl %edx,        0x1C(%eax)  /* Fiber->Esp = esp + 8 */
 movl %ebp,        0x20(%eax)  /* Fiber->Ebp = ebp */
 movl %ebx,        0x24(%eax)  /* Fiber->Ebx = ebx */
 movl %esi,        0x28(%eax)  /* Fiber->Esi = edi */
 movl %edi,        0x2C(%eax)  /* Fiber->Edi = esi */

 testl $1, 0x14(%eax)
 jz l_NoFloatSave
 
 /* save the floating point context */
 /* TODO */
 pushl ErrStr
 call _DbgPrint
 popl %ecx
 
l_NoFloatSave:

 /* switch to the specified fiber */
 movl 0x4(%esp), %eax          /* Fiber = lpFiber */
 movl %eax, 0x10(%ecx)         /* Teb->Tib.FiberData = Fiber */

 /* restore the volatile context of the specified fiber */
 movl 0x4(%eax),   %edx
 movl %edx,        0x0(%ecx)   /* Teb->ExceptionList = Fiber->ExceptionList */
 movl 0x8(%eax),   %edx
 movl %edx,        0x4(%ecx)   /* Teb->StackBase = Fiber->StackBase */
 movl 0xC(%eax),   %edx
 movl %edx,        0x8(%ecx)   /* Teb->StackLimit = Fiber->StackLimit */
 movl 0x10(%eax),  %edx
 movl %edx,        0xE0C(%ecx) /* Teb->StackBottom = Fiber->DeallocationStack */
 movl 0x18(%eax),  %edx        /* edx = Fiber->Eip */
 movl 0x1C(%eax),  %esp        /* esp = Fiber->Esp */
 movl 0x20(%eax),  %ebp        /* ebp = Fiber->Ebp */
 movl 0x24(%eax),  %ebx        /* ebx = Fiber->Ebx */
 movl 0x28(%eax),  %esi        /* esi = Fiber->Esi */
 movl 0x2C(%eax),  %edi        /* edi = Fiber->Edi */

 testb $1, 0x14(%eax)
 jz l_NoFloatSave
 
 /* restore the floating point context */
 /* TODO */
 pushl ErrStr
 call _DbgPrint
 popl %ecx
 
l_NoFloatRestore:

 /* jump to the saved program counter */
 jmp *%edx

/* EOF */
