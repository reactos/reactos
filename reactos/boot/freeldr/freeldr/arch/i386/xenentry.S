/*
 *  FreeLoader
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
 *
 *  Based on XenoLinux arch/xen/i386/kernel/entry.S
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#define ASM
#define __ASSEMBLY__
#include <arch.h>
#include <xen.h>

#define ALIGN .align 16,0x90

EBX             = 0x00
ECX             = 0x04
EDX             = 0x08
ESI             = 0x0C
EDI             = 0x10
EBP             = 0x14
EAX             = 0x18
DS              = 0x1C
ES              = 0x20
EIP             = 0x24
CS              = 0x28
EFLAGS          = 0x2C
OLDESP          = 0x30
OLDSS           = 0x34

/* Offsets into shared_info_t. */
#define evtchn_upcall_pending           /* 0 */
#define evtchn_upcall_mask              1

#define XEN_GET_VCPU_INFO(reg)	movl XenSharedInfo,reg
#define XEN_BLOCK_EVENTS(reg)	movb $1,evtchn_upcall_mask(reg)
#define XEN_UNBLOCK_EVENTS(reg)	movb $0,evtchn_upcall_mask(reg)

#define XEN_TEST_PENDING(reg)   testb $0xFF,evtchn_upcall_pending(reg)

/*
 * A note on the "critical region" in our callback handler.
 * We want to avoid stacking callback handlers due to events occurring
 * during handling of the last event. To do this, we keep events disabled
 * until we've done all processing. HOWEVER, we must enable events before
 * popping the stack frame (can't be done atomically) and so it would still
 * be possible to get enough handler activations to overflow the stack.
 * Although unlikely, bugs of that kind are hard to track down, so we'd
 * like to avoid the possibility.
 * So, on entry to the handler we detect whether we interrupted an
 * existing activation in its critical region -- if so, we pop the current
 * activation and restart the handler using the previous one.
 */
EXTERN(XenHypervisorCallback)
	cld
	pushl %es
	pushl %ds
	pushl %eax
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	pushl %ecx
	pushl %ebx
	movl $(FLAT_RING1_DS), %edx
	movl %edx, %ds
	movl %edx, %es
	movl EIP(%esp),%eax
	cmpl $scrit,%eax
	jb   11f
	cmpl $ecrit,%eax
	jb   critical_region_fixup
11:	XEN_GET_VCPU_INFO(%esi)
	push %esp
	call XenEvtchnDoUpcall
	add  $4,%esp

	ALIGN
	XEN_UNBLOCK_EVENTS(%esi)
scrit:	/**** START OF CRITICAL REGION ****/
	XEN_TEST_PENDING(%esi)
	jnz  14f		/* process more events if necessary... */
	popl %ebx
	popl %ecx
	popl %edx
	popl %esi
	popl %edi
	popl %ebp
	popl %eax
	popl %ds
	popl %es
	iret

14:	XEN_BLOCK_EVENTS(%esi)
	jmp  11b
ecrit:  /**** END OF CRITICAL REGION ****/

/*
 * [How we do the fixup]. We want to merge the current stack frame with the
 * just-interrupted frame. How we do this depends on where in the critical
 * region the interrupted handler was executing, and so how many saved
 * registers are in each frame. We do this quickly using the lookup table
 * 'critical_fixup_table'. For each byte offset in the critical region, it
 * provides the number of bytes which have already been popped from the
 * interrupted stack frame. 
 */
critical_region_fixup:
	addl $critical_fixup_table-scrit,%eax
	movzbl (%eax),%eax		/* %eax contains num bytes popped */
    	mov  %esp,%esi
	add  %eax,%esi			/* %esi points at end of src region */
	mov  %esp,%edi
	add  $0x34,%edi			/* %edi points at end of dst region */
	mov  %eax,%ecx
	shr  $2,%ecx			/* convert words to bytes */
	je   17f			/* skip loop if nothing to copy */
16:	subl $4,%esi			/* pre-decrementing copy loop */
	subl $4,%edi
	movl (%esi),%eax
	movl %eax,(%edi)
	loop 16b
17:	movl %edi,%esp			/* final %edi is top of merged stack */
	jmp  11b

critical_fixup_table:
	.byte 0x00,0x00,0x00		/* testb $0xff,(%esi) = XEN_TEST_PENDING */
	.byte 0x00,0x00			/* jnz  14f */
	.byte 0x00			/* pop  %ebx */
	.byte 0x04			/* pop  %ecx */
	.byte 0x08			/* pop  %edx */
	.byte 0x0c			/* pop  %esi */
	.byte 0x10			/* pop  %edi */
	.byte 0x14			/* pop  %ebp */
	.byte 0x18			/* pop  %eax */
	.byte 0x1c			/* pop  %ds */
	.byte 0x20			/* pop  %es */
	.byte 0x24			/* iret */
	.byte 0x00,0x00,0x00,0x00	/* movb $1,1(%esi) */
	.byte 0x00,0x00			/* jmp  11b */

/* Hypervisor uses this for application faults while it executes. */
EXTERN(XenFailsafeCallback)
	pop  %ds
	pop  %es
	pop  %fs 
	pop  %gs
	iret

/* EOF */
