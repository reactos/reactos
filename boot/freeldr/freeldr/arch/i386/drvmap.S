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

.code16

PUBLIC _DriveMapInt13HandlerStart
_DriveMapInt13HandlerStart:
Int13Handler:

    push bp
    mov bp, sp
    push ax
    push cx
    push si

    cld

    /* Get callers flags from stack */
    mov ax, [bp + 6]
    mov word ptr cs:[CallersFlags - Int13Handler], ax

    /* Save the drive number they passed in */
    mov cs:[PassedInDriveNumber - Int13Handler], dl

    /* Now we need to perform the mapping */
    xor cx, cx
    mov si, offset Int13HandlerMapCount - Int13Handler

    /* Get the count of drives in the map list */
    mov cl, cs:[si]
    inc si

    /* If the map list is empty then just call the old int 13h handler */
    cmp cl, 0
    jz CallOldInt13Handler

GetMappedDriveNumberLoop:

    /* Get the next drive number in the list */
    lods ax, cs:[si]

    /* Check to see if it's the one they are calling int 13h for */
    cmp dl, al

    /* If not get the next one */
    jne GetMappedDriveNumberLoopNext

    /* If we get here then we have found a mapped drive */
    /* Send new drive number on to the old int 13h handler */
    mov dl, ah

    /* Call BIOS Int 13 Handler */
    jmp CallOldInt13Handler

GetMappedDriveNumberLoopNext:
    loop GetMappedDriveNumberLoop

CallOldInt13Handler:
    /* Restore the registers we changed off the stack */
    pop si
    pop cx
    pop ax

    /* Put flags onto stack */
    push word ptr cs:[CallersFlags - Int13Handler]


    /* Call old int 13h handler with new drive number */
    .byte HEX(9a) /* lcall */
PUBLIC _DriveMapOldInt13HandlerAddress
_DriveMapOldInt13HandlerAddress:
    .word   0
    .word   0

    /* Update the callers flags with the values the BIOS returned */
    push ax
    pushf
    pop ax
    mov [bp + 6], ax
    pop ax

    /* Restore the callers drive number */
    mov dl, cs:[PassedInDriveNumber - Int13Handler]

    pop bp

    iret

CallersFlags:
    .word   0

PassedInDriveNumber:
    .byte   0

PUBLIC _DriveMapInt13HandlerMapList
_DriveMapInt13HandlerMapList:
Int13HandlerMapCount:
    .byte   0

Int13HandlerDrive1:
    .byte   0
Int13HandlerDriveNew1:
    .byte   0

Int13HandlerDrive2:
    .byte   0
Int13HandlerDriveNew2:
    .byte   0

Int13HandlerDrive3:
    .byte   0
Int13HandlerDriveNew3:
    .byte   0

Int13HandlerDrive4:
    .byte   0
Int13HandlerDriveNew4:
    .byte   0

PUBLIC _DriveMapInt13HandlerEnd
_DriveMapInt13HandlerEnd:
