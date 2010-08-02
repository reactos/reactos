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

	.text
	.code16

#define ASM
#include <arch.h>


EXTERN(ChainLoadBiosBootSectorCode)
	.code64

	call x86_64_SwitchToReal
	.code16

	/* Set the boot drive */
	mov dl, BootDrive

	/* Load segment registers */
	cli
	mov bx, 0x0000
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx
	mov ss, bx
	mov sp, 0x7C00

//	ljmpl	$0x0000,$0x7C00
	jmp 0x7c00:0x0000

EXTERN(SoftReboot)
	.code64

	call	x86_64_SwitchToReal
	.code16

	mov ax, 0x40
	mov ds, ax
	mov si, 0x72

	// Set the word at location 40:72 to 1234h
	mov word ptr [si], 0x1234

	// and jump to location FFFF:0 in ROM
//	ljmpl	$0xFFFF,$0x0000
	jmp 0xffff: 0x0000
