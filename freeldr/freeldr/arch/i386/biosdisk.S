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
_biosdisk_error_code:
	.byte	0
EXTERN(_BiosInt13Read)
	.code32

	pushal

	/* Get parameters */
	movl	0x24(%esp),%eax
	movl	%eax,_biosdisk_drive
	movl	0x28(%esp),%eax
	movl	%eax,_biosdisk_head
	movl	0x2c(%esp),%eax
	movl	%eax,_biosdisk_track
	movl	0x30(%esp),%eax
	movl	%eax,_biosdisk_sector
	movl	0x34(%esp),%eax
	movl	%eax,_biosdisk_nsects
	movl	0x38(%esp),%eax
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
	movb	%ah,_biosdisk_error_code// Save the error code

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

	popal

	movl	_biosdisk_retval,%eax	// Get return value
	//movl	$1,%eax

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
_packet_transfer_buffer_offset:
	.word	0
_packet_transfer_buffer_segment:
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

	pushal

	/* Get parameters */
	movl	0x24(%esp),%eax
	movl	%eax,_int13_extended_drive
	movl	0x28(%esp),%eax
	movl	%eax,_packet_lba_sector_number
	movl	0x2c(%esp),%eax
	movw	%ax,_packet_sector_count
	movl	%eax,_int13_extended_sector_count
	movl	0x30(%esp),%eax							// Get buffer address in eax
	shrl	$4,%eax									// Make linear address into segment
	movw	%ax,_packet_transfer_buffer_segment		// Save segment
	movl	0x30(%esp),%eax							// Get buffer address in eax
	andl	$0x0f,%eax								// Make linear address into offset
	movw	%ax,_packet_transfer_buffer_offset		// Save offset

	call	switch_to_real

	.code16
	pushw	%es										// Save this just in case
	movb	$3,_int13_extended_retrycount			// Set the retry count to 3

_int13_extended_read:
	movb	_int13_extended_drive,%dl				// Get the drive
	movb	$0x42,%ah								// BIOS int 0x13, function 42h - IBM/MS INT 13 Extensions - EXTENDED READ
	movw	$_disk_address_packet,%si				// DS:SI -> disk address packet
	int		$0x13									// Read sectors

	jc		_int13_extended_error					// Jump if error (CF = 1 on error)

	movl	_int13_extended_sector_count,%eax		// Get the sector count in eax
	cmpw	_packet_sector_count,%ax				// See how many sectors we actually read (returned in disk address packet sector count)
	jne		_int13_extended_error					// Jump if not equal

	jmp		_int13_extended_done


_int13_extended_error:
	movb	%ah,_biosdisk_error_code				// Save the error code

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

	popal

	movl	_int13_extended_retval,%eax				// Get return value

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

	pushal

	/* Get parameters */
	movl	0x24(%esp),%eax
	movl	%eax,_int13_extension_check_drive

	call	switch_to_real

	.code16
												// Now make sure this computer supports extended reads
	movb	$0x41,%ah							// AH = 41h
	movw	$0x55aa,%bx							// BX = 55AAh
	movb	_int13_extension_check_drive,%dl	// DL = drive (80h-FFh)
	int		$0x13								// IBM/MS INT 13 Extensions - INSTALLATION CHECK
	jc		_int13_extension_check_error		// CF set on error (extensions not supported)
	cmpw	$0xaa55,%bx							// BX = AA55h if installed
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

	popal

	movl	_int13_extension_check_retval,%eax				// Get return value

	ret

/*
 * ULONG BiosInt13GetLastErrorCode(VOID);
 */
EXTERN(_BiosInt13GetLastErrorCode)
	.code32

	movzbl	_biosdisk_error_code,%eax				// Get return value

	ret


/*
 * void StopFloppyMotor(void);
 *
 * Stops the floppy drive from spinning, so that other software is
 * jumped to with a known state.
 */
EXTERN(_StopFloppyMotor)
	.code32

	pushal

	call	switch_to_real

	.code16

	movw	$0x3F2, %dx
	xorb	%al, %al
	outb	%al, %dx

	call	switch_to_prot

	.code32

	popal

	ret

/*
 * int get_heads(int drive);
 */
EXTERN(_get_heads)
	.code32

	pushal
	push	%es

	/* Get drive */
	movl	0x28(%esp),%eax
	movl	%eax,_biosdisk_drive

	call	switch_to_real

	.code16

	movb	$0x08,%ah
	movb	_biosdisk_drive,%dl
	int		$0x13
	jc		_get_heads_error

	movzbl	%dh,%edx
	incl	%edx
	movl	%edx,_biosdisk_retval
	jmp		_get_heads_done

_get_heads_error:
	movl	$0xff,_biosdisk_retval

_get_heads_done:

	call	switch_to_prot

	.code32

	pop		%es
	popal

	movl	_biosdisk_retval,%eax	// Get return value

	ret

/*
 * int get_cylinders(int drive);
 */
EXTERN(_get_cylinders)
	.code32

	pushal
	push	%es

	/* Get drive */
	movl	0x28(%esp),%eax
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

	pop		%es
	popal

	movl	_biosdisk_retval,%eax	// Get return value

	ret

/*
 * int get_sectors(int drive);
 */
EXTERN(_get_sectors)
	.code32

	pushal
	push	%es

	/* Get drive */
	movl	0x28(%esp),%eax
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

	pop		%es
	popal

	movl	_biosdisk_retval,%eax	// Get return value

	ret


/*
 * BOOL BiosInt13GetDriveParameters(ULONG Drive, PGEOMETRY Geometry);
 */
_bios_int13_cylinders:
	.long	0
_bios_int13_heads:
	.long	0
_bios_int13_sectors:
	.long	0
_bios_int13_bytes_per_sector:
	.long	0
_bios_int13_drive_parameters_struct_address:
	.long	0
EXTERN(_BiosInt13GetDriveParameters)
	.code32

	pushal
	push	%es

	/* Get drive */
	movl	0x28(%esp),%eax
	movl	%eax,_biosdisk_drive
	movl	0x2c(%esp),%eax
	movl	%eax,_bios_int13_drive_parameters_struct_address

	call	switch_to_real

	.code16

	movb	$0x08,%ah
	movb	_biosdisk_drive,%dl
	int		$0x13
	jc		_BiosInt13GetDriveParameters_Error

	// Get the heads
	movzbl	%dh,%eax
	incl	%eax
	movl	%eax,_bios_int13_heads

	// Get the sectors
	movw	%cx,%dx
	andb	$0x3f,%dl
	movzbl	%dl,%edx
	movl	%edx,_bios_int13_sectors

	// Get the cylinders
	xorl	%edx,%edx
	andb	$0xc0,%cl
	shrb	$0x06,%cl
	movb	%cl,%dh
	movb	%ch,%dl
	incl	%edx
	movl	%edx,_bios_int13_cylinders

	// Get the bytes per sector
	movl	$512,_bios_int13_bytes_per_sector		// Just assume 512 bytes per sector
	movl	$0x01,_biosdisk_retval
	jmp		_BiosInt13GetDriveParameters_Done

_BiosInt13GetDriveParameters_Error:
	movl	$0x00,_biosdisk_retval

_BiosInt13GetDriveParameters_Done:

	call	switch_to_prot

	.code32

	// Copy drive parameters to structure
	movl	$_bios_int13_cylinders,%esi
	movl	_bios_int13_drive_parameters_struct_address,%edi
	movl	$0x04,%ecx
	cld
	rep movsl

	pop		%es
	popal

	movl	_biosdisk_retval,%eax	// Get return value

	ret


