/* $Id: chkstk.s,v 1.2 2003/07/06 23:04:19 hyperion Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Stack checker
 * FILE:              lib/ntdll/rtl/i386/chkstk.s
 * PROGRAMER:         KJK::Hyperion <noog@libero.it>
 */

.globl __chkstk
.globl __alloca_probe

/*
 _chkstk() is called by all stack allocations of more than 4 KB. It grows the
 stack in areas of 4 KB each, trying to access each area. This ensures that the
 guard page for the stack is hit, and the stack growing triggered
 */
__chkstk:
__alloca_probe:

/* EAX = size to be allocated */
/* save the ECX register */
 pushl %ecx

/* ECX = top of the previous stack frame */
 leal 8(%esp), %ecx

/* probe the desired memory, page by page */
 cmpl $0x1000, %eax
 jge .l_MoreThanAPage
 jmp .l_LessThanAPage

.l_MoreThanAPage:

/* raise the top of the stack by a page and probe */
 subl $0x1000, %ecx
 testl %eax, 0(%ecx)

/* loop if still more than a page must be probed */
 subl $0x1000, %eax
 cmpl $0x1000, %eax
 jge .l_MoreThanAPage

.l_LessThanAPage:

/* raise the top of the stack by EAX bytes (size % 4096) and probe */
 subl %eax, %ecx
 testl %eax, 0(%ecx)

/* EAX = top of the stack */
 movl %esp, %eax

/* allocate the memory */
 movl %ecx, %esp

/* restore ECX */
 movl 0(%eax), %ecx

/* restore the return address */
 movl 4(%eax), %eax
 pushl %eax

/* return */
 ret

/* EOF */
