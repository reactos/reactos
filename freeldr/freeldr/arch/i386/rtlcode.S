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


/*
 * void putchar(int ch);
 */
EXTERN(_putchar)
	.code32

	pushal

	/* Get character to display */
	movb	0x24(%esp),%bl

	/* If we are displaying a CR '\n' then do a LF also */
	cmpb	$0x0a,%bl
	jnz		putchar_1

	/* Display the LF */
	pushl	$0x0d
	call	_putchar
	popl	%eax

putchar_1:
	/* If we are displaying a TAB '\t' then display 8 spaces ' ' */
	cmpb	$0x09,%bl
	jnz		putchar_2

	/* Display the 8 spaces ' ' */
	pushl	$0x20
	call	_putchar
	call	_putchar
	call	_putchar
	call	_putchar
	call	_putchar
	call	_putchar
	call	_putchar
	call	_putchar
	popl	%eax
	popal

	ret

putchar_2:
	call	switch_to_real

	.code16

	/* Display the character via BIOS int 0x10 function 0x0e */
	movb	$0x0e,%ah
	movb	%bl,%al
	movw	$1,%bx
	int		$0x10

	call	switch_to_prot

	.code32

	popal

	ret

/*
 * int kbhit(void);
 */
_kbhit_retval:
	.long	0
EXTERN(_kbhit)
	.code32

	pushal

	movl	$0x00,_kbhit_retval

	call	switch_to_real

	.code16

	/* Int 0x16, AH = 0x01 - Get Keyboard Status */
	movb	$0x01,%ah
	int		$0x16
	jz		kbhit_1	// ZF=0 if no key is available

	/* Return value is non-zero if a key is available */
	movl	$1,_kbhit_retval
	jmp		kbhit_done

kbhit_1:
	/* Return value is zero if no key is available */
	movl	$0,_kbhit_retval

kbhit_done:
	
	call	switch_to_prot

	.code32

	popal

	/* Get return value */
	movl	_kbhit_retval,%eax

	ret

/*
 * int getch(void);
 */
extended_scancode:
	.byte	0
EXTERN(_getch)
	.code32

	push	%ebp
	push	%ebx

	call	switch_to_real

	.code16

	/* Check and see if we have an extended scancode to return */
	movb	extended_scancode,%al
	movb	$0,extended_scancode
	movzbl	%al,%ebx
	cmpb	$0,%al
	jnz		getch_done

	/* Int 0x16, AH = 0x00 - Wait for keypress */
	movb	$0,%ah
	int		$0x16

	/* If al is zero then it is an extended key */
	cmp		$0,%al
	jnz		getch_1

	/* Save the scan code to be returned on the next call to getch() */
	movb	%ah,extended_scancode

getch_1:
	/* Store character in ebx */
	movzbl	%al,%ebx

getch_done:
	call	switch_to_prot

	.code32

	/* Get return value from ebx */
	movl	%ebx,%eax

	pop		%ebx
	pop		%ebp
	ret

/*
 * int getyear(void);
 */
EXTERN(_getyear)
	.code32

	push	%ebp
	push	%ebx
	push	%ecx
	push	%edx

	call	switch_to_real

	.code16

	/* Get the date */
	movb	$4,%ah
	int		$0x1a

	/* Convert from BCD to normal */
	movb	%ch,%al
	andb	$0x0f,%al
	movb	%al,%dl
	movb	%ch,%al
	shrb	$0x04,%al
	andb	$0x0f,%al
	movb	$0x0a,%bl
	mulb	%bl
	addb	%al,%dl
	movb	%dl,%dh

	movb	%cl,%al
	andb	$0x0f,%al
	movb	%al,%dl
	movb	%cl,%al
	shrb	$0x04,%al
	andb	$0x0f,%al
	movb	$0x0a,%bl
	mulb	%bl
	addb	%al,%dl

	movb	%dl,%cl

	movzbl	%dh,%eax
	movl	$100,%ebx
	mull	%ebx
	movl	%eax,%edx
	addb	%cl,%dl

	/* Save return value */
	movl	%edx,%edx

	call	switch_to_prot

	.code32

	/* Restore return value */
	movl	%edx,%eax

	pop		%edx
	pop		%ecx
	pop		%ebx
	pop		%ebp
	ret

/*
 * int getday(void);
 */
EXTERN(_getday)
	.code32

	push	%ebp
	push	%ebx
	push	%ecx
	push	%edx

	call	switch_to_real

	.code16

	/* Get the date */
	movb	$4,%ah
	int		$0x1a

	/* Convert from BCD to normal */
	movb	%dl,%al
	andb	$0x0f,%al
	movb	%al,%cl
	movb	%dl,%al
	shrb	$0x04,%al
	andb	$0x0f,%al
	movb	$0x0a,%bl
	mulb	%bl
	addb	%al,%cl

	/* Save return value */
	movzbl	%cl,%edx

	call	switch_to_prot

	.code32

	/* Restore return value */
	movl	%edx,%eax

	pop		%edx
	pop		%ecx
	pop		%ebx
	pop		%ebp
	ret

/*
 * int getmonth(void);
 */
EXTERN(_getmonth)
	.code32

	push	%ebp
	push	%ebx
	push	%ecx
	push	%edx

	call	switch_to_real

	.code16

	/* Get the date */
	movb	$4,%ah
	int		$0x1a

	/* Convert from BCD to normal */
	movb	%dh,%al
	andb	$0x0f,%al
	movb	%al,%dl
	movb	%dh,%al
	shrb	$0x04,%al
	andb	$0x0f,%al
	movb	$0x0a,%bl
	mulb	%bl
	addb	%al,%dl

	/* Save return value */
	movzbl	%dl,%edx

	call	switch_to_prot

	.code32

	/* Restore return value */
	movl	%edx,%eax

	pop		%edx
	pop		%ecx
	pop		%ebx
	pop		%ebp
	ret

/*
 * int gethour(void);
 */
EXTERN(_gethour)
	.code32

	push	%ebp
	push	%ebx
	push	%ecx
	push	%edx

	call	switch_to_real

	.code16

	/* Get the time */
	movb	$2,%ah
	int		$0x1a

	/* Convert from BCD to normal */
	movb	%ch,%al
	andb	$0x0f,%al
	movb	%al,%dl
	movb	%ch,%al
	shrb	$0x04,%al
	andb	$0x0f,%al
	movb	$0x0a,%bl
	mulb	%bl
	addb	%al,%dl

	/* Save return value */
	movzbl	%dl,%edx

	call	switch_to_prot

	.code32

	/* Restore return value */
	movl	%edx,%eax

	pop		%edx
	pop		%ecx
	pop		%ebx
	pop		%ebp
	ret

/*
 * int getminute(void);
 */
EXTERN(_getminute)
	.code32

	push	%ebp
	push	%ebx
	push	%ecx
	push	%edx

	call	switch_to_real

	.code16

	/* Get the time */
	movb	$2,%ah
	int		$0x1a

	/* Convert from BCD to normal */
	movb	%cl,%al
	andb	$0x0f,%al
	movb	%al,%dl
	movb	%cl,%al
	shrb	$0x04,%al
	andb	$0x0f,%al
	movb	$0x0a,%bl
	mulb	%bl
	addb	%al,%dl

	/* Save return value */
	movzbl	%dl,%edx

	call	switch_to_prot

	.code32

	/* Restore return value */
	movl	%edx,%eax

	pop		%edx
	pop		%ecx
	pop		%ebx
	pop		%ebp
	ret

/*
 * int getsecond(void);
 */
EXTERN(_getsecond)
	.code32

	push	%ebp
	push	%ebx
	push	%ecx
	push	%edx

	call	switch_to_real

	.code16

	/* Get the time */
	movb	$2,%ah
	int		$0x1a

	/* Convert from BCD to normal */
	movb	%dh,%al
	andb	$0x0f,%al
	movb	%al,%dl
	movb	%dh,%al
	shrb	$0x04,%al
	andb	$0x0f,%al
	movb	$0x0a,%bl
	mulb	%bl
	addb	%al,%dl

	/* Save return value */
	movzbl	%dl,%edx

	call	switch_to_prot

	.code32

	/* Restore return value */
	movl	%edx,%eax

	pop		%edx
	pop		%ecx
	pop		%ebx
	pop		%ebp
	ret
