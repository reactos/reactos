/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
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
/*
 * FILE:            ntoskrnl/ke/i386/tskswitch.S
 * PURPOSE:         Microkernel thread support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 09/10/00
 */

/* INCLUDES ******************************************************************/

#include <internal/i386/segment.h>
#include <internal/ps.h>
#include <ddk/i386/tss.h>

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION:	 KeStackSwitchAndRet
 * PURPOSE:	 Switch to a new stack and return from the first frame on
 *               the new stack which was assumed to a stdcall function with
 *               8 bytes of arguments and which saved edi, esi and ebx.
 */
.globl _KeStackSwitchAndRet@4
_KeStackSwitchAndRet@4:
	pushl	%ebp
	movl	%esp, %ebp

	cli

	movl	8(%ebp), %esp

	sti

	popl	%edi
	popl	%esi
	popl	%ebx

	popl	%ebp
	ret	$8

.globl _KePushAndStackSwitchAndSysRet@8
_KePushAndStackSwitchAndSysRet@8:
	pushl	%ebp
	movl	%esp, %ebp

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	cli

	pushl	8(%ebp)

	movl	%fs:KPCR_CURRENT_THREAD, %ebx
	movl	%esp, KTHREAD_CALLBACK_STACK(%ebx)
	movl	12(%ebp), %esp

	sti

	push	$0
	call	_KeLowerIrql@4

	jmp	KeReturnFromSystemCall

