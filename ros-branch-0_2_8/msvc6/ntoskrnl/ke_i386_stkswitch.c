/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
 *
 *  Moved to MSVC-compatible inline assembler by Mike Nordell, 2003-12-26
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
 * FILE:            ntoskrnl/ke/i386/vm86_sup.S
 * PURPOSE:         V86 mode support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 09/10/00
 */

/* INCLUDES ******************************************************************/

#pragma hdrstop

#include <ddk/ntddk.h>
#include <ddk/status.h>
#include <internal/i386/segment.h>
#include <internal/i386/fpu.h>
#include <internal/ps.h>
#include <ddk/defines.h>
#include <internal/v86m.h>
#include <ntos/tss.h>
//#include <ntos/service.h>
#include <internal/trap.h>
#include <internal/ps.h>

#include <roscfg.h>
#include <internal/ntoskrnl.h>
#include <internal/i386/segment.h>


void KeReturnFromSystemCall();

/*
 * FUNCTION:	 KeStackSwitchAndRet
 * PURPOSE:	 Switch to a new stack and return from the first frame on
 *               the new stack which was assumed to a stdcall function with
 *               8 bytes of arguments and which saved edi, esi and ebx.
 */
__declspec(naked)
VOID STDCALL
KeStackSwitchAndRet(PVOID NewStack)
{
	__asm
	{
		push	ebp
		mov		ebp, esp

		cli

		mov		esp, NewStack

		sti

		pop		edi
		pop		esi
		pop		ebx

		pop		ebp
		ret		8
	}
}

__declspec(naked)
VOID STDCALL
KePushAndStackSwitchAndSysRet(ULONG Push, PVOID NewStack)
{
	__asm
	{
		push	ebp
		mov		ebp, esp

		push	ebx
		push	esi
		push	edi

		cli

		push	8[ebp]

		mov		ebx, fs:KPCR_CURRENT_THREAD
		mov		KTHREAD_CALLBACK_STACK[ebx], esp
		mov		esp, 12[ebp]

		sti

		push	0
		call	KeLowerIrql

		jmp		KeReturnFromSystemCall
	}
}
