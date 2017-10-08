/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <asm.inc>
#include <arch/pc/x86common.h>

EXTERN i386CallRealMode:PROC

.code32

/*
 *  VOID BootOldLinuxKernel(ULONG KernelSize);
 */
PUBLIC _BootOldLinuxKernel
_BootOldLinuxKernel:

    /* First we have to copy the kernel down from 0x100000 to 0x10000 */
    /* The reason we can overwrite low memory is because this code */
    /* executes between 0000:8000 and 0000:FFFF. That leaves space for */
    /* 32k of code before we start interfering with Linux kernel address space. */

    /* Get KernelSize in ECX and move the kernel down */
    mov ecx, [esp + 4]
    mov esi, HEX(100000)
    mov edi, HEX(10000)
    rep movsb

    /* Fall through */

PUBLIC _BootNewLinuxKernel
_BootNewLinuxKernel:

    mov bx, FNID_BootLinuxKernel
    call i386CallRealMode

    /* We should never get here */
    int 3

END
