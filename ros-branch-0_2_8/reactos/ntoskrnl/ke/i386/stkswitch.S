/*
 * FILE:            ntoskrnl/ke/i386/tskswitch.S
 * PURPOSE:         Microkernel thread support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 09/10/00
 */

/* INCLUDES ******************************************************************/
#include <ndk/asm.h>

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

	jmp _KiServiceExit

