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
#include <arch.h>


EXTERN(_DriveMapInt13HandlerStart)
Int13Handler:

	pushw	%bp
	movw	%sp,%bp
	pushw	%ax
	pushw	%cx
	pushw	%si

	cld

	/* Get callers flags from stack */
	movw	0x06(%bp),%ax
	movw	%ax,%cs:(CallersFlags - Int13Handler)

	/* Save the drive number they passed in */
	movb	%dl,%cs:(PassedInDriveNumber - Int13Handler)

	/* Now we need to perform the mapping */
	xorw	%cx,%cx
	movw	$(Int13HandlerMapCount - Int13Handler),%si
	/* Get the count of drives in the map list */
	movb	%cs:(%si),%cl
	incw	%si

	/* If the map list is empty then just call the old int 13h handler */
	cmpb	$0,%cl
	jz		CallOldInt13Handler

GetMappedDriveNumberLoop:

	/* Get the next drive number in the list */
	lodsw	%cs:(%si),%ax
	/* Check to see if it's the one they are calling int 13h for */
	cmpb	%al,%dl
	/* If not get the next one */
	jne		GetMappedDriveNumberLoopNext

	/* If we get here then we have found a mapped drive */
	/* Send new drive number on to the old int 13h handler */
	movb	%ah,%dl
	/* Call BIOS Int 13 Handler */
	jmp		CallOldInt13Handler

GetMappedDriveNumberLoopNext:
	loop	GetMappedDriveNumberLoop

CallOldInt13Handler:
	/* Restore the registers we changed off the stack */
	popw	%si
	popw	%cx
	popw	%ax

	/* Put flags onto stack */
	pushw	%cs:(CallersFlags - Int13Handler)

	/* Call old int 13h handler with new drive number */
	.byte	0x9a /* lcall */
EXTERN(_DriveMapOldInt13HandlerAddress)
	.word	0
	.word	0

	/* Update the callers flags with the values the BIOS returned */
	pushw	%ax
	pushf
	popw	%ax
	movw	%ax,0x06(%bp)
	popw	%ax
	/* Restore the callers drive number */
	movb	%cs:(PassedInDriveNumber - Int13Handler),%dl

	popw	%bp

	iret

CallersFlags:
	.word	0

PassedInDriveNumber:
	.byte	0

EXTERN(_DriveMapInt13HandlerMapList)
Int13HandlerMapCount:
	.byte	0

Int13HandlerDrive1:
	.byte	0
Int13HandlerDriveNew1:
	.byte	0

Int13HandlerDrive2:
	.byte	0
Int13HandlerDriveNew2:
	.byte	0

Int13HandlerDrive3:
	.byte	0
Int13HandlerDriveNew3:
	.byte	0

Int13HandlerDrive4:
	.byte	0
Int13HandlerDriveNew4:
	.byte	0

EXTERN(_DriveMapInt13HandlerEnd)
