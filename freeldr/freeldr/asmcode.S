/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000, 2001  Brian Palmer  <brianp@sginet.com>
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
#include "asmcode.h"


EXTERN(start)

	cli

	/* Setup segment registers */
	xorw	%ax,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	/* Setup a stack */
	movw	stack16,%sp

	sti

	/* Init pmode */
	call	switch_to_prot

	.code32

	/* Store the boot drive */
	movb	%dl,(_BootDrive)

	/* GO! */
	call	_BootMain

	call	switch_to_real
	.code16

	int		$0x19

	/* We should never get here */
stop:
	jmp		stop


/*
 * Switches the processor to protected mode
 * it destroys eax
 */
EXTERN(switch_to_prot)

	.code16

	cli		/* None of these */

	/* Get the return address off the stack */
	popw	(code32ret)

	/* Save 16-bit stack pointer */
	movw	%sp,stack16

	/* Load the GDT */
	lgdt	gdtptr

	/* Enable Protected Mode */
	mov		%cr0,%eax
	orl		$CR0_PE_SET,%eax
	mov		%eax,%cr0

	/* Clear prefetch queue & correct CS */
	ljmp	$PMODE_CS, $inpmode


	.code32

inpmode:
	/* Setup segment selectors */
	movw	$PMODE_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	movl	stack32,%esp

	/* Put the return address back onto the stack */
	pushl	(code32ret)

	/* Now return in p-mode! */
	ret

/*
 * Switches the processor back to real mode
 * it destroys eax
 */
EXTERN(switch_to_real)

	.code32

	/* Get the return address off the stack */
	popl	(code16ret)

	/* Save 32-bit stack pointer */
	movl	%esp,stack32

	/* jmp to 16-bit segment to set the limit correctly */
	ljmp	$RMODE_CS, $switch_to_real16

switch_to_real16:
	.code16

	/* Restore segment registers to correct limit */
	movw	$RMODE_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss

	/* Disable Protected Mode */
	mov		%cr0,%eax
	andl	$CR0_PE_CLR,%eax
	mov		%eax,%cr0

	/* Clear prefetch queue & correct CS */
	ljmp	$0, $inrmode

inrmode:
	movw	%cs,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	movw	stack16,%sp

	/* Put the return address back onto the stack */
	pushw	(code16ret)

	sti		/* These are ok now */

	/* Now return in r-mode! */
	ret


/*
 * void putchar(int ch);
 */
EXTERN(_putchar)
	.code32

	push	%eax
	push	%ebx
	push	%ebp

	/* Get character to display */
	movb	0x10(%esp),%bl

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
	pop		%ebp
	pop		%ebx
	pop		%eax
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

	pop		%ebp
	pop		%ebx
	pop		%eax
	ret

/*
 * void clrscr(void);
 */
EXTERN(_clrscr)
	.code32

	push	%eax
	push	%ebp

	call	switch_to_real

	.code16

	/* Int 0x10, AH = 0x0F - Get Current Video Mode */
	movb	$0x0f,%ah
	int		$0x10

	/* Int 0x10, AH = 0x00 - Set Current Video Mode, also clears the screen */
	movb	$0x00,%ah
	int		$0x10

	call	switch_to_prot

	.code32

	pop		%ebp
	pop		%eax
	ret

/*
 * int kbhit(void);
 */
EXTERN(_kbhit)
	.code32

	push	%ebp
	push	%ebx

	call	switch_to_real

	.code16

	/* Int 0x16, AH = 0x01 - Get Keyboard Status */
	movb	$0x01,%ah
	int		$0x16
	jz		kbhit_1	// ZF=0 if no key is available

	/* Return value is non-zero if a key is available */
	movl	$1,%ebx
	jmp		kbhit_done

kbhit_1:
	/* Return value is zero if no key is available */
	movl	$0,%ebx

kbhit_done:
	
	call	switch_to_prot

	.code32

	/* Get return value from ebx */
	movl	%ebx,%eax

	pop		%ebx
	pop		%ebp
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
 * void gotoxy(int x, int y);
 */
EXTERN(_gotoxy)
	.code32

	push	%ebp
	push	%eax
	push	%ebx
	push	%edx

	/* Get cursor positions */
	movb	0x14(%esp),%dl
	movb	0x18(%esp),%dh

	call	switch_to_real

	.code16

	/* Update the cursor position */
	movb	$2,%ah
	movb	$0,%bh
	int		$0x10

	call	switch_to_prot

	.code32

	pop		%edx
	pop		%ebx
	pop		%eax
	pop		%ebp
	ret

/*
 * BOOL BiosInt13Read(ULONG Drive, ULONG Head, ULONG Track, ULONG Sector, ULONG SectorCount, PVOID Buffer);
 */
_biosdisk_drive:
	.long	0
_biosdisk_head:
	.long	0
_biosdisk_track:
	.long	0
_biosdisk_sector:
	.long	0
_biosdisk_nsects:
	.long	0
_biosdisk_buffer:
	.long	0
_biosdisk_retval:
	.long	0
_biosdisk_retrycount:
	.byte	0
EXTERN(_BiosInt13Read)
	.code32

	push	%ebp
	push	%esi
	push	%edi
	push	%ebx
	push	%ecx
	push	%edx

	/* Get parameters */
	movl	0x1c(%esp),%eax
	movl	%eax,_biosdisk_drive
	movl	0x20(%esp),%eax
	movl	%eax,_biosdisk_head
	movl	0x24(%esp),%eax
	movl	%eax,_biosdisk_track
	movl	0x28(%esp),%eax
	movl	%eax,_biosdisk_sector
	movl	0x2c(%esp),%eax
	movl	%eax,_biosdisk_nsects
	movl	0x30(%esp),%eax
	movl	%eax,_biosdisk_buffer

	call	switch_to_real

	.code16
	pushw	%es						// Save this just in case
	movb	$3,_biosdisk_retrycount	// Set the retry count to 3

_biosdisk_read:
	movl	_biosdisk_buffer,%eax	// Get buffer address in eax
	shrl	$4,%eax					// Make linear address into segment
	movw	%ax,%es					// Load ES with segment
	movl	_biosdisk_buffer,%ebx	//  and BX with offset
	andl	$0x0f,%ebx				//  so that data gets loaded to [ES:BX]
	movb	_biosdisk_sector,%cl	// Get the sector in CL
    movw	_biosdisk_track,%ax		// Cylinder in AX
	movb	%al,%ch					// Now put it in CH
	rorb	$1,%ah					// Low 8 bits of cylinder in CH, high 2 bits
	rorb	$1,%ah					//  in CL shifted to bits 6 & 7
	andb	$0xc0,%ah				// Clear out low six bits
	orb		%ah,%cl					// Or with sector number
	movb	_biosdisk_head,%dh		// Get the head
	movb	_biosdisk_drive,%dl		// Get the drive
	movb	$2,%ah					// BIOS int 0x13, function 2 - Read Disk Sectors
	movb	_biosdisk_nsects,%al	// Number of sectors to read
	int		$0x13					// Read a sector

	// I have recently learned that not all bioses return
	// the sector read count in the AL register (at least mine doesn't)
	// even if the sectors were read correctly. So instead
	// of checking the sector read count we will rely solely
	// on the carry flag being set on error

	//jmp		_biosdisk_done
	//cmpb	_biosdisk_nsects,%al	// See how many sectors we actually read
	//jne		_biosdisk_error			// Jump if no error

	movb	$1,%al					// Set the return value to be one (will be set to zero later if needed)
	jc		_biosdisk_error			// Jump if error (CF = 1 on error)
	jmp		_biosdisk_done


_biosdisk_error:
	cmpb	$0x11,%ah				// Check and see if it was a corrected ECC error
	je		_biosdisk_done			// If so then the data is still good, if not fail

	movb	_biosdisk_retrycount,%al// Get the current retry count
	decb	%al						// Decrement it
	movb	%al,_biosdisk_retrycount// Save it
	cmpb	$0,%al					// Is it zero?
	jz		_biosdisk_zero			// Yes, return zero

	movb	$0,%ah					// BIOS int 0x13, function 0 - Reset Disk System
	movb	_biosdisk_drive,%dl		// Get the drive
	int		$0x13					// Reset the disk system
	jmp		_biosdisk_read			// Try reading again

_biosdisk_zero:
	movb	$0,%al					// We will return zero

_biosdisk_done:
	movzbl	%al,%eax				// Put the number of sectors read into EAX
	movl	%eax,_biosdisk_retval	// Save it as the return value

	popw	%es						// Restore ES
	call	switch_to_prot

	.code32

	movl	_biosdisk_retval,%eax	// Get return value
	//movl	$1,%eax

	pop		%edx
	pop		%ecx
	pop		%ebx
	pop		%edi
	pop		%esi
	pop		%ebp
	ret

/*
 * BOOL BiosInt13ReadExtended(ULONG Drive, ULONG Sector, ULONG SectorCount, PVOID Buffer);
 */
_disk_address_packet:
_packet_size:
	.byte	0x10
_packet_reserved:
	.byte	0
_packet_sector_count:
	.word	0
_packet_transfer_buffer_segment:
	.word	0
_packet_transfer_buffer_offset:
	.word	0
_packet_lba_sector_number:
	.quad	0
_packet_64bit_flat_address:
	.quad	0
_int13_extended_drive:
	.long	0
_int13_extended_sector_count:
	.long	0
_int13_extended_retval:
	.long	0
_int13_extended_retrycount:
	.byte	0
EXTERN(_BiosInt13ReadExtended)
	.code32

	push	%ebp
	push	%esi
	push	%edi
	push	%ebx
	push	%ecx
	push	%edx

	/* Get parameters */
	movl	0x1c(%esp),%eax
	movl	%eax,_int13_extended_drive
	movl	0x20(%esp),%eax
	movl	%eax,_packet_lba_sector_number
	movl	0x24(%esp),%eax
	movw	%ax,_packet_sector_count
	movl	%eax,_int13_extended_sector_count
	movl	0x28(%esp),%eax							// Get buffer address in eax
	shrl	$4,%eax									// Make linear address into segment
	movw	%ax,_packet_transfer_buffer_segment		// Save segment
	movl	0x28(%esp),%eax							// Get buffer address in eax
	andl	$0x0f,%eax								// Make linear address into offset
	movw	%ax,_packet_transfer_buffer_offset		// Save offset

	call	switch_to_real

	.code16
	pushw	%es										// Save this just in case
	movb	$3,_int13_extended_retrycount			// Set the retry count to 3

_int13_extended_read:
	movb	_int13_extended_drive,%dl				// Get the drive
	movb	$42,%ah									// BIOS int 0x13, function 42h - IBM/MS INT 13 Extensions - EXTENDED READ
	movw	$_disk_address_packet,%si				// DS:SI -> disk address packet
	int		$0x13									// Read sectors

	movb	$1,%al									// Set the return value to be one (will be set to zero later if needed)
	jc		_int13_extended_error					// Jump if error (CF = 1 on error)

	movl	_int13_extended_sector_count,%eax		// Get the sector count in eax
	cmpw	_packet_sector_count,%ax				// See how many sectors we actually read (returned in disk address packet sector count)
	jne		_int13_extended_error					// Jump if not equal

	jmp		_int13_extended_done


_int13_extended_error:
	cmpb	$0x11,%ah								// Check and see if it was a corrected ECC error
	je		_int13_extended_done					// If so then the data is still good, if not fail

	movb	_int13_extended_retrycount,%al			// Get the current retry count
	decb	%al										// Decrement it
	movb	%al,_int13_extended_retrycount			// Save it
	cmpb	$0,%al									// Is it zero?
	jz		_int13_extended_zero					// Yes, return zero

	movb	$0,%ah									// BIOS int 0x13, function 0 - Reset Disk System
	movb	_int13_extended_drive,%dl				// Get the drive
	int		$0x13									// Reset the disk system
	jmp		_int13_extended_read					// Try reading again

_int13_extended_zero:
	movb	$0,%al									// We will return zero

_int13_extended_done:
	movzbl	%al,%eax								// Put the number of sectors read into EAX
	movl	%eax,_int13_extended_retval				// Save it as the return value

	popw	%es										// Restore ES
	call	switch_to_prot

	.code32

	movl	_int13_extended_retval,%eax				// Get return value
	//movl	$1,%eax

	pop		%edx
	pop		%ecx
	pop		%ebx
	pop		%edi
	pop		%esi
	pop		%ebp
	ret

/*
 * BOOL BiosInt13ExtensionsSupported(ULONG Drive);
 */
_int13_extension_check_drive:
	.long	0
_int13_extension_check_retval:
	.long	0
EXTERN(_BiosInt13ExtensionsSupported)
	.code32

	push	%ebp
	push	%esi
	push	%edi
	push	%ebx
	push	%ecx
	push	%edx

	/* Get parameters */
	movl	0x1c(%esp),%eax
	movl	%eax,_int13_extension_check_drive

	call	switch_to_real

	.code16
												// Now make sure this computer supports extended reads
	movb	$0x41,%ah							// AH = 41h
	movw	$0x55aa,%bx							// BX = 55AAh
	movb	_int13_extension_check_drive,%dl	// DL = drive (80h-FFh)
	int		$0x13								// IBM/MS INT 13 Extensions - INSTALLATION CHECK
	jc		_int13_extension_check_error		// CF set on error (extensions not supported)
	cmpw	$0x55aa,%bx							// BX = AA55h if installed
	jne		_int13_extension_check_error
	testb	$1,%cl								// CX = API subset support bitmap
	jz		_int13_extension_check_error		// Bit 0, extended disk access functions (AH=42h-44h,47h,48h) supported

												// If we get here then we passed all the int13 extension tests
	movl	$1,_int13_extension_check_retval	// Set return value to TRUE
	jmp		_int13_extension_check_done

_int13_extension_check_error:

	movl	$0,_int13_extension_check_retval	// The tests failed so return FALSE

_int13_extension_check_done:

	call	switch_to_prot

	.code32

	movl	_int13_extension_check_retval,%eax				// Get return value

	pop		%edx
	pop		%ecx
	pop		%ebx
	pop		%edi
	pop		%esi
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

/*
 * void hidecursor(void);
 */
EXTERN(_hidecursor)
	.code32

	push	%ebp
	push	%ebx
	push	%ecx
	push	%edx

	call	switch_to_real

	.code16

	/* Hide the cursor */
	movb	$1,%ah
	movw	$0x2000,%cx
	int		$0x10

	call	switch_to_prot

	.code32

	pop		%edx
	pop		%ecx
	pop		%ebx
	pop		%ebp
	ret

/*
 * void showcursor(void);
 */
EXTERN(_showcursor)
	.code32

	push	%ebp
	push	%ebx
	push	%ecx
	push	%edx

	call	switch_to_real

	.code16

	/* Show the cursor */
	movb	$1,%ah
	movb	$0x0d,%ch
	movb	$0x0e,%cl
	int		$0x10

	call	switch_to_prot

	.code32

	pop		%edx
	pop		%ecx
	pop		%ebx
	pop		%ebp
	ret

/*
 * int wherex(void);
 */
EXTERN(_wherex)
	.code32

	push	%ebp
	push	%ebx
	push	%ecx
	push	%edx

	call	switch_to_real

	.code16

	/* Get the cursor position */
	movb	$3,%ah
	movb	$0,%bh
	int		$0x10

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
 * int wherey(void);
 */
EXTERN(_wherey)
	.code32

	push	%ebp
	push	%ebx
	push	%ecx
	push	%edx

	call	switch_to_real

	.code16

	/* Get the cursor position */
	movb	$3,%ah
	movb	$0,%bh
	int		$0x10

	/* Save return value */
	movzbl	%dh,%edx

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
 * void stop_floppy(void);
 *
 * Stops the floppy drive from spinning, so that other software is
 * jumped to with a known state.
 */
EXTERN(_stop_floppy)
	.code32

	push	%eax
	push	%edx

	call	switch_to_real

	.code16

	movw	$0x3F2, %dx
	xorb	%al, %al
	outb	%al, %dx

	call	switch_to_prot

	.code32

	pop		%edx
	pop		%eax
	ret

/*
 * int get_heads(int drive);
 */
EXTERN(_get_heads)
	.code32

	push	%ebx
	push	%ecx
	push	%edx
	push	%edi
	push	%es

	/* Get drive */
	movl	0x18(%esp),%eax
	movl	%eax,_biosdisk_drive

	call	switch_to_real

	.code16

	movb	$0x08,%ah
	movb	_biosdisk_drive,%dl
	int		$0x13
	jc		_get_heads_error

	incb	%dh
	movzbl	%dh,%edx
	movl	%edx,_biosdisk_retval
	jmp		_get_heads_done

_get_heads_error:
	movl	$0xff,_biosdisk_retval

_get_heads_done:

	call	switch_to_prot

	.code32

	movl	_biosdisk_retval,%eax	// Get return value

	pop		%es
	pop		%edi
	pop		%edx
	pop		%ecx
	pop		%ebx

	ret

/*
 * int get_cylinders(int drive);
 */
EXTERN(_get_cylinders)
	.code32

	push	%ebx
	push	%ecx
	push	%edx
	push	%edi
	push	%es

	/* Get drive */
	movl	0x18(%esp),%eax
	movl	%eax,_biosdisk_drive

	call	switch_to_real

	.code16

	movb	$0x08,%ah
	movb	_biosdisk_drive,%dl
	int		$0x13
	jc		_get_cylinders_error

	xorl	%edx,%edx
	andb	$0xc0,%cl
	shrb	$0x06,%cl
	movb	%cl,%dh
	movb	%ch,%dl
	incl	%edx
	movl	%edx,_biosdisk_retval
	jmp		_get_cylinders_done

_get_cylinders_error:
	movl	$0xff,_biosdisk_retval

_get_cylinders_done:

	call	switch_to_prot

	.code32

	movl	_biosdisk_retval,%eax	// Get return value

	pop		%es
	pop		%edi
	pop		%edx
	pop		%ecx
	pop		%ebx

	ret

/*
 * int get_sectors(int drive);
 */
EXTERN(_get_sectors)
	.code32

	push	%ebx
	push	%ecx
	push	%edx
	push	%edi
	push	%es

	/* Get drive */
	movl	0x18(%esp),%eax
	movl	%eax,_biosdisk_drive

	call	switch_to_real

	.code16

	movb	$0x08,%ah
	movb	_biosdisk_drive,%dl
	int		$0x13
	jc		_get_sectors_error

	andb	$0x3f,%cl
	movzbl	%cl,%ecx
	movl	%ecx,_biosdisk_retval
	jmp		_get_sectors_done

_get_sectors_error:
	movl	$0xff,_biosdisk_retval

_get_sectors_done:

	call	switch_to_prot

	.code32

	movl	_biosdisk_retval,%eax	// Get return value

	pop		%es
	pop		%edi
	pop		%edx
	pop		%ecx
	pop		%ebx

	ret


	/*
	 * Needed for enabling the a20 address line
	 */
	.code16
empty_8042:
	.word	0x00eb,0x00eb            // jmp $+2, jmp $+2
	inb     $0x64,%al
    testb   $0x02,%al
    jnz     empty_8042
	ret

	/*
	 * Enable the A20 address line (to allow access to over 1mb)
	 */
	.code32
EXTERN(_enable_a20)
	call	switch_to_real
	.code16

	call	empty_8042
    movb	$0xD1,%al                // command write
    outb	%al,$0x64
	call	empty_8042
    mov		$0xDF,%al                // A20 on
    out		%al,$0x60
	call	empty_8042
	call	switch_to_prot
	.code32

	ret




	/* 16-bit stack pointer */
stack16:
	.word	STACK16ADDR

	/* 32-bit stack pointer */
stack32:
	.long	STACK32ADDR

	/* 16-bit return address */
code16ret:
	.long	0

	/* 32-bit return address */
code32ret:
	.long	0


	.p2align	2	/* force 4-byte alignment */
gdt:
	/* NULL Descriptor */
	.word	0x0000
	.word	0x0000
	.word	0x0000
	.word	0x0000

	/* 32-bit flat CS */
	.word	0xFFFF
	.word	0x0000
	.word	0x9A00
	.word	0x00CF

	/* 32-bit flat DS */
	.word	0xFFFF
	.word	0x0000
	.word	0x9200
	.word	0x00CF

	/* 16-bit real mode CS */
	.word	0xFFFF
	.word	0x0000
	.word	0x9E00
	.word	0x0000

	/* 16-bit real mode DS */
	.word	0xFFFF
	.word	0x0000
	.word	0x9200
	.word	0x0000

/* GDT table pointer */
gdtptr:
	.word	0x27		/* Limit */
	.long	gdt			/* Base Address */

EXTERN(_BootDrive)
	.long	0
