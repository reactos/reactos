/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
 *  Portions from Linux video.S - Display adapter & video mode setup, version 2.13 (14-May-99)
 *  Copyright (C) 1995 -- 1999 Martin Mares <mj@ucw.cz>
 *  Based on the original setup.S code (C) Linus Torvalds and Mats Anderson
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



/*
 * VOID BiosSetVideoMode(ULONG VideoMode);
 */
BiosVideoMode:
	.long	0
EXTERN(_BiosSetVideoMode)
	.code32

	pushal

	/* Get BIOS video mode */
	movl	0x24(%esp),%eax
	movl	%eax,BiosVideoMode

	call	switch_to_real

	.code16

	/* Int 0x10, AH = 0x00 - Set Current Video Mode, also clears the screen */
	movb	$0x00,%ah
	movb	BiosVideoMode,%al
	int		$0x10

	call	switch_to_prot

	.code32

	popal
	ret

/*
 * VOID BiosSetVideoFont8x8(VOID);
 */
EXTERN(_BiosSetVideoFont8x8)
	.code32

	pushal

	call	switch_to_real

	.code16

	/* Int 0x10, AX = 0x1112 - Load 8x8 Font */
	movw	$0x1112,%ax
	xorb	%bl,%bl
	int		$0x10

	call	switch_to_prot

	.code32

	popal
	ret

/*
 * VOID BiosSetVideoFont8x14(VOID);
 */
EXTERN(_BiosSetVideoFont8x14)
	.code32

	pushal

	call	switch_to_real

	.code16

	/* Int 0x10, AX = 0x1111 - Load 8x16 Font */
	movw	$0x1111,%ax
	xorb	%bl,%bl
	int		$0x10

	call	switch_to_prot

	.code32

	popal
	ret

/*
 * VOID BiosSetVideoFont8x16(VOID);
 */
EXTERN(_BiosSetVideoFont8x16)
	.code32

	pushal

	call	switch_to_real

	.code16

	/* Int 0x10, AX = 0x1114 - Load 8x16 Font */
	movw	$0x1114,%ax
	xorb	%bl,%bl
	int		$0x10

	call	switch_to_prot

	.code32

	popal
	ret

/*
 * VOID BiosSelectAlternatePrintScreen(VOID);
 */
EXTERN(_BiosSelectAlternatePrintScreen)
	.code32

	pushal

	call	switch_to_real

	.code16

	/* Int 0x10, AH = 0x12 - Select alternate print screen routine */
	movb	$0x12,%ah
	movb	$0x20,%bl
	int		$0x10

	call	switch_to_prot

	.code32

	popal
	ret

/*
 * VOID BiosDisableCursorEmulation(VOID);
 */
EXTERN(_BiosDisableCursorEmulation)
	.code32

	pushal

	call	switch_to_real

	.code16

	/* Int 0x10, AH = 0x12 - Disable cursor emulation */
	movw	$0x1201,%ax
	movb	$0x34,%bl
	int		$0x10

	call	switch_to_prot

	.code32

	popal
	ret

/*
 * VOID BiosDefineCursor(ULONG StartScanLine, ULONG EndScanLine);
 */
BiosDefineCursorStartScanLine:
	.long	0
BiosDefineCursorEndScanLine:
	.long	0
EXTERN(_BiosDefineCursor)
	.code32

	pushal

	/* Get cursor scan line positions */
	movl	0x24(%esp),%eax
	movl	%eax,BiosDefineCursorStartScanLine
	movl	0x28(%esp),%eax
	movl	%eax,BiosDefineCursorEndScanLine

	call	switch_to_real

	.code16

	/* Int 0x10, AH = 0x01 - Set Text-Mode Cursor Shape */
	movb	$0x01,%ah
	movb	$0x03,%al		// Current video mode in AL for buggy AMI 386 BIOS
	movb	BiosDefineCursorStartScanLine,%ch
	movb	BiosDefineCursorEndScanLine,%cl
	int		$0x10

	call	switch_to_prot

	.code32

	popal
	ret

/*
 * ULONG BiosDetectVideoCard(VOID);
 */
BiosDetectVideoCardReturnValue:
	.long	0
EXTERN(_BiosDetectVideoCard)
	.code32

	pushal

	movl	$0x00,BiosDetectVideoCardReturnValue

	call	switch_to_real

	.code16

	/* Int 0x10, AH = 0x12 - Get EGA Info */
	movb	$0x12,%ah
	movb	$0x10,%bl
	int		$0x10

	movl	$0x00,BiosDetectVideoCardReturnValue
	cmpb	$0x10,%bl
	je		BiosDetectVideoCardDone

	/* Int 0x10, AX = 0x1A00 - Get Display Combination Code */
	movw	$0x1a00,%ax
	int		$0x10

	cmpb	$0x1a,%al
	je		BiosDetectVideoCardVga

	movl	$0x01,BiosDetectVideoCardReturnValue
	jmp		BiosDetectVideoCardDone

BiosDetectVideoCardVga:

	movl	$0x02,BiosDetectVideoCardReturnValue

BiosDetectVideoCardDone:

	call	switch_to_prot

	.code32

	popal

	movl	BiosDetectVideoCardReturnValue,%eax

	ret

/*
 * VOID BiosSet200ScanLines(VOID);
 */
EXTERN(_BiosSet200ScanLines)
	.code32

	pushal

	call	switch_to_real

	.code16

	/* Int 0x10, AX = 0x1200 - Set Vertical Resolution */
	movw	$0x1200,%ax
	movb	$0x30,%bl
	int		$0x10

	call	switch_to_prot

	.code32

	popal
	ret

/*
 * VOID BiosSet350ScanLines(VOID);
 */
EXTERN(_BiosSet350ScanLines)
	.code32

	pushal

	call	switch_to_real

	.code16

	/* Int 0x10, AX = 0x1201 - Set Vertical Resolution */
	movw	$0x1201,%ax
	movb	$0x30,%bl
	int		$0x10

	call	switch_to_prot

	.code32

	popal
	ret

/*
 * VOID BiosSet400ScanLines(VOID);
 */
EXTERN(_BiosSet400ScanLines)
	.code32

	pushal

	call	switch_to_real

	.code16

	/* Int 0x10, AX = 0x1202 - Set Vertical Resolution */
	movw	$0x1202,%ax
	movb	$0x30,%bl
	int		$0x10

	call	switch_to_prot

	.code32

	popal
	ret

/*
 * VOID BiosSet480ScanLines(VOID);
 */
EXTERN(_BiosSet480ScanLines)
	.code32

	pushal

	call	switch_to_real

	.code16

	movw	$0x03CC,%dx			// Get CRTC port
	inb		%dx,%al
	movb	$0xD4,%dl
	rorb	$0x01,%al
	jc		set48a
	movb	$0xB4,%dl

set48a:
	movw	$0x0C11,%ax			// Vertical sync end (also unlocks CR0-7)
	call	outidx
	movw	$0x0B06,%ax			// Vertical total
	call	outidx
	movw	$0x3E07,%ax			// (vertical) overflow
	call	outidx
	movw	$0xEA10,%ax			// Vertical sync start
	call	outidx
	movw	$0xDF12,%ax			// Vertical display end
	call	outidx
	movw	$0xE715,%ax			// Vertical blank start
	call	outidx
	movw	$0x0416,%ax			// Vertical blank end
	call	outidx

	push	%dx
	movb	$0xCC,%dl			// Misc output register (read)
	inb		%dx,%al
	movb	$0xC2,%dl			// (write)
	andb	$0x0D,%al			// Preserve clock select bits and color bit
	orb		$0xE2,%al			// Set correct sync polarity
	outb	%al,%dx
	pop		%dx

	call	switch_to_prot

	.code32

	popal
	ret

/*
 * VOID BiosSetVideoDisplayEnd(VOID);
 */
EXTERN(_BiosSetVideoDisplayEnd)
	.code32

	pushal

	call	switch_to_real

	.code16

	movw	$0x03CC,%dx			// Get CRTC port
	inb		%dx,%al
	movb	$0xD4,%dl
	rorb	$0x01,%al
	jc		set48a
	movb	$0xB4,%dl

setvde:
	movw	$0xDF12,%ax			// Vertical display end
	call	outidx

	call	switch_to_prot

	.code32

	popal
	ret

/*
 * Write to indexed VGA register (AL=index, AH=data, DX=index reg. port)
 */
outidx:
	.code16

	outb	%al,%dx
	push	%ax
	movb	%ah,%al
	incw	%dx
	outb	%al,%dx
	decw	%dx
	pop		%ax

	ret

/*
 * VOID VideoSetTextCursorPosition(ULONG X, ULONG Y);
 */
EXTERN(_VideoSetTextCursorPosition)
	.code32

	pushal

	/* Get cursor positions */
	movb	0x24(%esp),%dl
	movb	0x28(%esp),%dh

	call	switch_to_real

	.code16

	/* Update the cursor position */
	movb	$2,%ah
	movb	$0,%bh
	int		$0x10

	call	switch_to_prot

	.code32

	popal
	ret


/*
 * VOID VideoHideTextCursor(VOID);
 */
EXTERN(_VideoHideTextCursor)
	.code32

	pushal

	call	switch_to_real

	.code16

	/* Hide the cursor */
	movb	$1,%ah
	movw	$0x2000,%cx
	int		$0x10

	call	switch_to_prot

	.code32

	popal
	ret

/*
 * VOID VideoShowTextCursor(VOID);
 */
EXTERN(_VideoShowTextCursor)
	.code32

	pushal

	call	switch_to_real

	.code16

	/* Show the cursor */
	movb	$1,%ah
	movb	$0x0d,%ch
	movb	$0x0e,%cl
	int		$0x10

	call	switch_to_prot

	.code32

	popal
	ret

/*
 * ULONG VideoGetTextCursorPositionX(VOID);
 */
VideoGetTextCursorPositionXReturnValue:
	.long	0
EXTERN(_VideoGetTextCursorPositionX)
	.code32

	pushal

	movl	$0x00,VideoGetTextCursorPositionXReturnValue

	call	switch_to_real

	.code16

	/* Get the cursor position */
	movb	$3,%ah
	movb	$0,%bh
	int		$0x10

	/* Save return value */
	movzbl	%dl,%edx
	movl	%edx,VideoGetTextCursorPositionXReturnValue

	call	switch_to_prot

	.code32

	popal

	/* Restore return value */
	movl	VideoGetTextCursorPositionXReturnValue,%eax

	ret

/*
 * ULONG VideoGetTextCursorPositionY(VOID);
 */
VideoGetTextCursorPositionYReturnValue:
	.long	0
EXTERN(_VideoGetTextCursorPositionY)
	.code32

	pushal

	movl	$0x00,VideoGetTextCursorPositionYReturnValue

	call	switch_to_real

	.code16

	/* Get the cursor position */
	movb	$3,%ah
	movb	$0,%bh
	int		$0x10

	/* Save return value */
	movzbl	%dh,%edx
	movl	%edx,VideoGetTextCursorPositionYReturnValue

	call	switch_to_prot

	.code32

	popal

	/* Restore return value */
	movl	VideoGetTextCursorPositionYReturnValue,%eax

	ret



