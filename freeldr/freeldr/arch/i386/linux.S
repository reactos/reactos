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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

	.text
	.code16

#define ASM
#include "arch.h"


	.code32
EXTERN(_BootNewLinuxKernel)
	call	switch_to_real
	.code16

	/* Set the boot drive */
	movb	(_BootDrive),%dl

	/* Load segment registers */
	cli
	movw	$0x9000,%bx
	movw	%bx,%ds
	movw	%bx,%es
	movw	%bx,%fs
	movw	%bx,%gs
	movw	%bx,%ss
	movw	$0x9000,%sp

	ljmpl	$0x9020,$0x0000


	.code32
/*
 *  VOID BootOldLinuxKernel(ULONG KernelSize);
 */
EXTERN(_BootOldLinuxKernel)

	/* First we have to copy the kernel down from 0x100000 to 0x10000 */
	/* The reason we can overwrite low memory is because this code */
	/* executes between 0000:8000 and 0000:FFFF. That leaves space for */
	/* 32k of code before we start interfering with Linux kernel address space. */

	/* Get KernelSize in ECX and move the kernel down */
	movl	0x04(%esp),%ecx
	movl	$0x100000,%esi
	movl	$0x10000,%edi
	rep movsb

	call	switch_to_real
	.code16

	/* Set the boot drive */
	movb	(_BootDrive),%dl

	/* Load segment registers */
	cli
	movw	$0x9000,%bx
	movw	%bx,%ds
	movw	%bx,%es
	movw	%bx,%fs
	movw	%bx,%gs
	movw	%bx,%ss
	movw	$0x9000,%sp

	ljmpl	$0x9020,$0x0000
