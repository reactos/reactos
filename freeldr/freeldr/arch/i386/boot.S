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
#include "multiboot.h"


	.code32
EXTERN(_JumpToBootCode)
	call	switch_to_real
	.code16

	/* Set the boot drive */
	movb	(_BootDrive),%dl

	/* Load segment registers */
	cli
	movw	$0x0000,%bx
	movw	%bx,%ds
	movw	%bx,%es
	movw	%bx,%fs
	movw	%bx,%gs
	movw	%bx,%ss
	movw	$0x7C00,%sp

	ljmpl	$0x0000,$0x7C00


	.code32
EXTERN(_JumpToLinuxBootCode)
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
	movw	$0x8FF4,%sp

	ljmpl	$0x9020,$0x0000
