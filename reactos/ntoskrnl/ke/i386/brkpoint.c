/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/brkpoints.c
 * PURPOSE:         Handles breakpoints
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID STDCALL 
DbgBreakPoint(VOID)
{
#if defined(__GNUC__)
   __asm__("int $3\n\t");
#elif defined(_MSC_VER)
   __asm int 3;
#else
#error Unknown compiler for inline assembler
#endif
}

/*
 * @implemented
 */
#if defined(__GNUC__)
__asm__(".globl _DbgBreakPointNoBugCheck@0\n\t"
	"_DbgBreakPointNoBugCheck@0:\n\t"
	"int $3\n\t"
	"ret\n\t");
#endif

/*
 * @implemented
 */
VOID STDCALL 
DbgBreakPointWithStatus(ULONG Status)
{
#if defined(__GNUC__)
   __asm__("mov %0, %%eax\n\t"
           "int $3\n\t"
           ::"m"(Status));
#elif defined(_MSC_VER)
   __asm mov eax, Status
   __asm int 3;
#else
#error Unknown compiler for inline assembler
#endif
}
