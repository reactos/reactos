; ****************************************************************************
;
;  isolinux.asm
;
;  A program to boot Linux kernels off a CD-ROM using the El Torito
;  boot standard in "no emulation" mode, making the entire filesystem
;  available.  It is based on the SYSLINUX boot loader for MS-DOS
;  floppies.
;
;   Copyright (C) 1994-2001  H. Peter Anvin
;
;  This program is free software; you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation, Inc., 675 Mass Ave, Cambridge MA 02139,
;  USA; either version 2 of the License, or (at your option) any later
;  version; incorporated herein by reference.
; 
; ****************************************************************************
;
; THIS FILE IS A MODIFIED VERSION OF ISOLINUX.ASM
; MODIFICATION DONE BY MICHAEL K TER LOUW
; LAST UPDATED 3-9-2002
; SEE "COPYING" FOR INFORMATION ABOUT THE LICENSE THAT APPLIES TO THIS RELEASE
;
; ****************************************************************************
;
; This file is a modified version of ISOLINUX.ASM.
; Modification done by Eric Kohl
; Last update 04-25-2002
;
; ****************************************************************************

; Note: The Makefile builds one version with DEBUG_MESSAGES automatically.
;%define DEBUG_MESSAGES                ; Uncomment to get debugging messages




; ---------------------------------------------------------------------------
;   BEGIN THE BIOS/CODE/DATA SEGMENT
; ---------------------------------------------------------------------------

		absolute 0400h
serial_base	resw 4			; Base addresses for 4 serial ports
		absolute 0413h
BIOS_fbm	resw 1			; Free Base Memory (kilobytes)
		absolute 046Ch
BIOS_timer	resw 1			; Timer ticks
		absolute 0472h
BIOS_magic	resw 1			; BIOS reset magic
		absolute 0484h
BIOS_vidrows	resb 1			; Number of screen rows

;
; Memory below this point is reserved for the BIOS and the MBR
;
		absolute 1000h
trackbuf	resb 8192		; Track buffer goes here
trackbufsize	equ $-trackbuf
;		trackbuf ends at 3000h

		struc open_file_t
file_sector	resd 1			; Sector pointer (0 = structure free)
file_left	resd 1			; Number of sectors left
		endstruc

		struc dir_t
dir_lba		resd 1			; Directory start (LBA)
dir_len		resd 1			; Length in bytes
dir_clust	resd 1			; Length in clusters
		endstruc


MAX_OPEN_LG2	equ 2			; log2(Max number of open files)
MAX_OPEN	equ (1 << MAX_OPEN_LG2)
SECTORSIZE_LG2	equ 11			; 2048 bytes/sector (El Torito requirement)
SECTORSIZE	equ (1 << SECTORSIZE_LG2)
CR		equ 13			; Carriage Return
LF		equ 10			; Line Feed



	absolute 5000h				; Here we keep our BSS stuff

DriveNo		resb 1			; CD-ROM BIOS drive number
DiskError	resb 1			; Error code for disk I/O
RetryCount	resb 1			; Used for disk access retries
TimeoutCount	resb 1			; Timeout counter
ISOFlags	resb 1			; Flags for ISO directory search
RootDir		resb dir_t_size		; Root directory
CurDir		resb dir_t_size		; Current directory
ISOFileName	resb 64			; ISO filename canonicalization buffer
ISOFileNameEnd	equ $


		alignb open_file_t_size
Files		resb MAX_OPEN*open_file_t_size



	section .text
	org 7000h

start:
	cli					; Disable interrupts
	xor	ax, ax				; ax = segment zero
	mov	ss, ax				; Initialize stack segment
	mov	sp, start			; Set up stack
	mov	ds, ax				; Initialize other segment registers
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	sti					; Enable interrupts
	cld					; Increment pointers

	mov	cx, 2048 >> 2			; Copy the bootsector
	mov	si, 0x7C00			; from 0000:7C00
	mov	di, 0x7000			; to 0000:7000
	rep	movsd				; copy the program
	jmp	0:relocate			; jump into relocated code

relocate:
	; Display the banner and copyright
%ifdef DEBUG_MESSAGES
	mov	si, isolinux_banner		; si points to hello message
	call	writestr			; display the message
	mov	si,copyright_str
	call	writestr
%endif


	; Make sure the keyboard buffer is empty
.kbd_buffer_test:
	call	pollchar
	jz	.kbd_buffer_empty
	call	getchar
	jmp	.kbd_buffer_test
.kbd_buffer_empty:

	; Display the 'Press key' message and wait for a maximum of 5 seconds
	call	crlf
	mov	si, presskey_msg		; si points to 'Press key' message
	call	writestr			; display the message

	mov	byte [TimeoutCount], 5
.next_second:
	mov	eax, [BIOS_timer]		; load current tick counter
	add	eax, 19				; 

.poll_again:
	call	pollchar
	jnz	.boot_cdrom

	mov	ebx, [BIOS_timer]
	cmp	eax, ebx
	jnz	.poll_again

	mov	si, dot_msg			; print '.'
	call	writestr
	dec	byte [TimeoutCount]		; decrement timeout counter
	jz	.boot_harddisk
	jmp	.next_second

.boot_harddisk:
	; Boot first harddisk (drive 0x80)
	mov	ax, 0201h
	mov	dx, 0080h
	mov	cx, 0001h
	mov	bx, 7C00h
	int	13h
	jnc	.go_hd
	jmp	kaboom
.go_hd:
	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	mov	dx, 0080h

	jmp	0:0x7C00


.boot_cdrom:
	; Save and display the boot drive number
	mov	[DriveNo], dl
%ifdef DEBUG_MESSAGES
	mov	si, startup_msg
	call	writemsg
	mov	al, dl
	call	writehex2
	call	crlf
%endif

	; Now figure out what we're actually doing
	; Note: use passed-in DL value rather than 7Fh because
	; at least some BIOSes will get the wrong value otherwise
	mov	ax, 4B01h			; Get disk emulation status
	mov	dl, [DriveNo]
	mov	si, spec_packet
	int	13h
	jc	near spec_query_failed		; Shouldn't happen (BIOS bug)
	mov	dl, [DriveNo]
	cmp	[sp_drive], dl			; Should contain the drive number
	jne	near spec_query_failed

%ifdef DEBUG_MESSAGES
	mov	si, spec_ok_msg
	call	writemsg
	mov	al, byte [sp_drive]
	call	writehex2
	call	crlf
%endif

found_drive:
	; Get drive information
	mov	ah, 48h
	mov	dl, [DriveNo]
	mov	si, drive_params
	int	13h
	jnc	params_ok

	mov	si, nosecsize_msg
	call	writemsg

params_ok:
	; Check for the sector size (should be 2048, but
	; some BIOSes apparently think we're 512-byte media)
	;
	; FIX: We need to check what the proper behaviour
	; is for getlinsec when the BIOS thinks the sector
	; size is 512!!!  For that, we need such a BIOS, though...
%ifdef DEBUG_MESSAGES
	mov	si, secsize_msg
	call	writemsg
	mov	ax, [dp_secsize]
	call	writehex4
	call	crlf
%endif


	;
	; Clear Files structures
	;
	mov	di, Files
	mov	cx, (MAX_OPEN*open_file_t_size)/4
	xor	eax, eax
	rep	stosd

	;
	; Now, we need to sniff out the actual filesystem data structures.
	; mkisofs gave us a pointer to the primary volume descriptor
	; (which will be at 16 only for a single-session disk!); from the PVD
	; we should be able to find the rest of what we need to know.
	;
get_fs_structures:
	mov	eax, 16			; Primary Volume Descriptor (sector 16)
	mov	bx, trackbuf
	call	getonesec

	mov	eax, [trackbuf+156+2]
	mov	[RootDir+dir_lba],eax
	mov	[CurDir+dir_lba],eax
%ifdef DEBUG_MESSAGES
	mov	si, rootloc_msg
	call	writemsg
	call	writehex8
	call	crlf
%endif

	mov	eax,[trackbuf+156+10]
	mov	[RootDir+dir_len],eax
	mov	[CurDir+dir_len],eax
%ifdef DEBUG_MESSAGES
	mov	si, rootlen_msg
	call	writemsg
	call	writehex8
	call	crlf
%endif
	add	eax,SECTORSIZE-1
	shr	eax,SECTORSIZE_LG2
	mov	[RootDir+dir_clust],eax
	mov	[CurDir+dir_clust],eax
%ifdef DEBUG_MESSAGES
	mov	si, rootsect_msg
	call	writemsg
	call	writehex8
	call	crlf
%endif

	; Look for the "X86" directory, and if found,
	; make it the current directory instead of the root
	; directory.
	mov	di,isolinux_dir
	mov	al,02h				; Search for a directory
	call	searchdir_iso
	jnz	.dir_found
	mov	si,no_dir_msg
	call	writemsg
	jmp	kaboom

.dir_found:
	mov	[CurDir+dir_len],eax
	mov	eax,[si+file_left]
	mov	[CurDir+dir_clust],eax
	xor	eax,eax				; Free this file pointer entry
	xchg	eax,[si+file_sector]
	mov	[CurDir+dir_lba],eax


	mov	di, isolinux_bin		; di points to Isolinux filename
	call	searchdir			; look for the file
	jnz	.isolinux_opened		; got the file
	mov	si, no_isolinux_msg		; si points to error message
	call	writemsg			; display the message
	jmp	kaboom				; fail boot

.isolinux_opened:
	push	si				; save file pointer

%ifdef DEBUG_MESSAGES
	mov	si, filelen_msg
	call	writemsg
	call	writehex8
	call	crlf
%endif

	mov	bx, 0x8000			; bx = load address
	pop	si				; si = file pointer
	mov	cx, 0xFFFF			; load the whole file
	call	getfssec			; get the first sector
	mov	dl, [DriveNo]			; dl = boot drive
	jmp	0:0x8000			; jump into OSLoader



;
; searchdir:
;
; Open a file
;
;  On entry:
;	DS:DI	= filename
;  If successful:
;	ZF clear
;	SI		= file pointer
;	DX:AX or EAX	= file length in bytes
;  If unsuccessful
;	ZF set
;

;
; searchdir_iso is a special entry point for ISOLINUX only.  In addition
; to the above, searchdir_iso passes a file flag mask in AL.  This is useful
; for searching for directories.
;
alloc_failure:
	xor	ax,ax				; ZF <- 1
	ret

searchdir:
	xor	al,al
searchdir_iso:
	mov	[ISOFlags],al
	call	allocate_file			; Temporary file structure for directory
	jnz	alloc_failure
	push	es
	push	ds
	pop	es				; ES = DS
	mov	si,CurDir
	cmp	byte [di],'\'			; If filename begins with slash
	jne	.not_rooted
	inc	di				; Skip leading slash
	mov	si,RootDir			; Reference root directory instead
.not_rooted:
	mov	eax,[si+dir_clust]
	mov	[bx+file_left],eax
	mov	eax,[si+dir_lba]
	mov	[bx+file_sector],eax
	mov	edx,[si+dir_len]

.look_for_slash:
	mov	ax,di
.scan:
	mov	cl,[di]
	inc	di
	and	cl,cl
	jz	.isfile
	cmp	cl,'\'
	jne	.scan
	mov	[di-1],byte 0			; Terminate at directory name
	mov	cl,02h				; Search for directory
	xchg	cl,[ISOFlags]
	push	di
	push	cx
	push	word .resume			; Where to "return" to
	push	es
.isfile:
	xchg	ax,di

.getsome:
	; Get a chunk of the directory
	mov	si,trackbuf
	pushad
	xchg	bx,si
	mov	cx,1				; load one sector
	call	getfssec
	popad

.compare:
	movzx	eax, byte [si]			; Length of directory entry
	cmp	al, 33
	jb	.next_sector
	mov	cl, [si+25]
	xor	cl, [ISOFlags]
	test	cl, byte 8Eh			; Unwanted file attributes!
	jnz	.not_file
	pusha
	movzx	cx, byte [si+32]		; File identifier length
	add	si, byte 33			; File identifier offset
	call	iso_compare_names
	popa
	je	.success
.not_file:
	sub	edx, eax			; Decrease bytes left
	jbe	.failure
	add	si, ax				; Advance pointer

.check_overrun:
	; Did we finish the buffer?
	cmp	si, trackbuf+trackbufsize
	jb	.compare			; No, keep going

	jmp	short .getsome			; Get some more directory

.next_sector:
	; Advance to the beginning of next sector
	lea	ax, [si+SECTORSIZE-1]
	and	ax, ~(SECTORSIZE-1)
	sub	ax, si
	jmp	short .not_file			; We still need to do length checks

.failure:
%ifdef DEBUG_MESSAGES
	mov	si, findfail_msg
	call	writemsg
	call	crlf
%endif
	xor	eax, eax			; ZF = 1
	mov	[bx+file_sector], eax
	pop	es
	ret

.success:
	mov	eax, [si+2]			; Location of extent
	mov	[bx+file_sector], eax
	mov	eax, [si+10]			; Data length
	push	eax
	add	eax, SECTORSIZE-1
	shr	eax, SECTORSIZE_LG2
	mov	[bx+file_left], eax
	pop	eax
	mov	edx, eax
	shr	edx, 16
	and	bx, bx				; ZF = 0
	mov	si, bx
	pop	es
	ret

.resume:
	; We get here if we were only doing part of a lookup
	; This relies on the fact that .success returns bx == si
	xchg	edx, eax			; Directory length in edx
	pop	cx				; Old ISOFlags
	pop	di				; Next filename pointer

	mov	byte [di-1], '\'		; restore the backslash in the filename

	mov	[ISOFlags], cl			; Restore the flags
	jz	.failure			; Did we fail?  If so fail for real!
	jmp	.look_for_slash			; Otherwise, next level

;
; allocate_file: Allocate a file structure
;
;		If successful:
;		  ZF set
;		  BX = file pointer
;		In unsuccessful:
;		  ZF clear
;
allocate_file:
	push	cx
	mov	bx, Files
	mov	cx, MAX_OPEN
.check:
	cmp	dword [bx], byte 0
	je	.found
	add	bx, open_file_t_size		; ZF = 0
	loop	.check
	; ZF = 0 if we fell out of the loop
.found:
	pop	cx
	ret

;
; iso_compare_names:
;	Compare the names DS:SI and DS:DI and report if they are
;	equal from an ISO 9660 perspective.  SI is the name from
;	the filesystem; CX indicates its length, and ';' terminates.
;	DI is expected to end with a null.
;
;	Note: clobbers AX, CX, SI, DI; assumes DS == ES == base segment
;
iso_compare_names:
	; First, terminate and canonicalize input filename
	push	di
	mov	di, ISOFileName
.canon_loop:
	jcxz	.canon_end
	lodsb
	dec	cx
	cmp	al, ';'
	je	.canon_end
	and	al, al
	je	.canon_end
	stosb
	cmp	di, ISOFileNameEnd-1		; Guard against buffer overrun
	jb	.canon_loop
.canon_end:
	cmp	di, ISOFileName
	jbe	.canon_done
	cmp	byte [di-1], '.'		; Remove terminal dots
	jne	.canon_done
	dec	di
	jmp	short .canon_end
.canon_done:
	mov	[di], byte 0			; Null-terminate string
	pop	di
	mov	si, ISOFileName
.compare:
	lodsb
	mov	ah, [di]
	inc	di
	and	ax, ax
	jz	.success			; End of string for both
	and	al, al				; Is either one end of string?
	jz	.failure			; If so, failure
	and	ah, ah
	jz	.failure
	or	ax, 2020h			; Convert to lower case
	cmp	al, ah
	je	.compare
.failure:
	and	ax, ax				; ZF = 0 (at least one will be nonzero)
.success:
	ret







;
; getfssec: Get multiple clusters from a file, given the file pointer.
;
;  On entry:
;	ES:BX	-> Buffer
;	SI	-> File pointer
;	CX	-> Cluster count; 0FFFFh = until end of file
;  On exit:
;	SI	-> File pointer (or 0 on EOF)
;	CF = 1	-> Hit EOF
;
getfssec:
	cmp	cx, [si+file_left]
	jna	.ok_size
	mov	cx, [si+file_left]

.ok_size:
	mov	bp, cx
	push	cx
	push	si
	mov	eax, [si+file_sector]
	call	getlinsec
	xor	ecx, ecx
	pop	si
	pop	cx

	add	[si+file_sector], ecx
	sub	[si+file_left], ecx
	ja	.not_eof			; CF = 0

	xor	ecx, ecx
	mov	[si+file_sector], ecx		; Mark as unused
	xor	si,si
	stc

.not_eof:
	ret



; INT 13h, AX=4B01h, DL=<passed in value> failed.
; Try to scan the entire 80h-FFh from the end.
spec_query_failed:
	mov	si,spec_err_msg
	call	writemsg

	mov	dl, 0FFh
.test_loop:
	pusha
	mov	ax, 4B01h
	mov	si, spec_packet
	mov	byte [si], 13			; Size of buffer
	int	13h
	popa
	jc	.still_broken

	mov	si, maybe_msg
	call	writemsg
	mov	al, dl
	call	writehex2
	call	crlf

	cmp	byte [sp_drive], dl
	jne	.maybe_broken

	; Okay, good enough...
	mov	si, alright_msg
	call	writemsg
	mov	[DriveNo], dl
.found_drive:
	jmp	found_drive

	; Award BIOS 4.51 apparently passes garbage in sp_drive,
	; but if this was the drive number originally passed in
	; DL then consider it "good enough"
.maybe_broken:
	cmp	byte [DriveNo], dl
	je	.found_drive

.still_broken:	dec dx
	cmp	dl, 80h
	jnb	.test_loop

fatal_error:
	mov	si, nothing_msg
	call	writemsg

.norge:
	jmp	short .norge



	; Information message (DS:SI) output
	; Prefix with "isolinux: "
	;
writemsg:
	push	ax
	push	si
	mov	si, isolinux_str
	call	writestr
	pop	si
	call	writestr
	pop	ax
	ret

;
; crlf: Print a newline
;
crlf:
	mov	si, crlf_msg
	; Fall through

;
; writestr: write a null-terminated string to the console, saving
;           registers on entry.
;
writestr:
	pushfd
	pushad
.top:
	lodsb
	and	al, al
	jz	.end
	call	writechr
	jmp	short .top
.end:
	popad
	popfd
	ret


;
; writehex[248]: Write a hex number in (AL, AX, EAX) to the console
;
writehex2:
	pushfd
	pushad
	shl	eax, 24
	mov	cx, 2
	jmp	short writehex_common
writehex4:
	pushfd
	pushad
	shl	eax, 16
	mov	cx, 4
	jmp	short writehex_common
writehex8:
	pushfd
	pushad
	mov	cx, 8
writehex_common:
.loop:
	rol	eax, 4
	push	eax
	and	al, 0Fh
	cmp	al, 10
	jae	.high
.low:
	add	al, '0'
	jmp	short .ischar
.high:
	add	al, 'A'-10
.ischar:
	call	writechr
	pop	eax
	loop	.loop
	popad
	popfd
	ret

;
; Write a character to the screen.  There is a more "sophisticated"
; version of this in the subsequent code, so we patch the pointer
; when appropriate.
;

writechr:
	pushfd
	pushad
	mov	ah, 0Eh
	xor	bx, bx
	int	10h
	popad
	popfd
	ret

;
; Get one sector.  Convenience entry point.
;
getonesec:
	mov	bp, 1
	; Fall through to getlinsec

;
; Get linear sectors - EBIOS LBA addressing, 2048-byte sectors.
;
; Note that we can't always do this as a single request, because at least
; Phoenix BIOSes has a 127-sector limit.  To be on the safe side, stick
; to 32 sectors (64K) per request.
;
; Input:
;	EAX	- Linear sector number
;	ES:BX	- Target buffer
;	BP	- Sector count
;
getlinsec:
	mov si,dapa			; Load up the DAPA
	mov [si+4],bx
	mov bx,es
	mov [si+6],bx
	mov [si+8],eax
.loop:
	push bp				; Sectors left
	cmp bp,byte 32
	jbe .bp_ok
	mov bp,32
.bp_ok:
	mov [si+2],bp
	push si
	mov dl,[DriveNo]
	mov ah,42h			; Extended Read
	call xint13
	pop si
	pop bp
	movzx eax,word [si+2]		; Sectors we read
	add [si+8],eax			; Advance sector pointer
	sub bp,ax			; Sectors left
	shl ax,SECTORSIZE_LG2-4		; 2048-byte sectors -> segment
	add [si+6],ax			; Advance buffer pointer
	and bp,bp
	jnz .loop
	mov eax,[si+8]			; Next sector
	ret

	; INT 13h with retry
xint13:
	mov	byte [RetryCount], 6
.try:
	pushad
	int	13h
	jc	.error
	add	sp, byte 8*4			; Clean up stack
	ret
.error:
	mov	[DiskError], ah		; Save error code
	popad
	dec	byte [RetryCount]
	jnz	.try

.real_error:
	mov	si, diskerr_msg
	call	writemsg
	mov	al, [DiskError]
	call	writehex2
	mov	si, ondrive_str
	call	writestr
	mov	al, dl
	call	writehex2
	call	crlf
	; Fall through to kaboom

;
; kaboom: write a message and bail out.  Wait for a user keypress,
;	  then do a hard reboot.
;
kaboom:
	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	sti
	mov	si, err_bootfailed
	call	writestr
	call	getchar
	cli
	mov	word [BIOS_magic], 0	; Cold reboot
	jmp	0F000h:0FFF0h		; Reset vector address

getchar:
.again:
	mov	ah, 1		; Poll keyboard
	int	16h
	jz	.again
.kbd:
	xor	ax, ax		; Get keyboard input
	int	16h
.func_key:
	ret


;
; pollchar: check if we have an input character pending (ZF = 0)
;
pollchar:
		pushad
		mov ah,1		; Poll keyboard
		int 16h
		popad
		ret



isolinux_banner	db CR, LF, 'Loading IsoBoot...', CR, LF, 0
copyright_str	db ' Copyright (C) 1994-2002 H. Peter Anvin', CR, LF, 0
presskey_msg	db 'Press any key to boot from CD', 0
dot_msg		db '.',0

%ifdef DEBUG_MESSAGES
startup_msg:	db 'Starting up, DL = ', 0
spec_ok_msg:	db 'Loaded spec packet OK, drive = ', 0
secsize_msg:	db 'Sector size appears to be ', 0
rootloc_msg:	db 'Root directory location: ', 0
rootlen_msg:	db 'Root directory length: ', 0
rootsect_msg:	db 'Root directory length(sectors): ', 0
fileloc_msg:	db 'FreeLdr.sys location: ', 0
filelen_msg:	db 'FreeLdr.sys length: ', 0
filesect_msg:	db 'FreeLdr.sys length(sectors): ', 0
findfail_msg:	db 'Failed to find file!', 0
%endif

nosecsize_msg:	db 'Failed to get sector size, assuming 0800', CR, LF, 0
spec_err_msg:	db 'Loading spec packet failed, trying to wing it...', CR, LF, 0
maybe_msg:	db 'Found something at drive = ', 0
alright_msg:	db 'Looks like it might be right, continuing...', CR, LF, 0
nothing_msg:	db 'Failed to locate CD-ROM device; boot failed.', CR, LF, 0
isolinux_str	db 'IsoBoot: ', 0
crlf_msg	db CR, LF, 0
diskerr_msg:	db 'Disk error ', 0
ondrive_str:	db ', drive ', 0
err_bootfailed	db CR, LF, 'Boot failed: press a key to retry...'
isolinux_dir	db '\REACTOS', 0
no_dir_msg	db 'Could not find the REACTOS directory.', CR, LF, 0
isolinux_bin	db 'FREELDR.SYS', 0
no_isolinux_msg	db 'Could not find the file FREELDR.SYS.', CR, LF, 0

;
; El Torito spec packet
;
		align 8, db 0
spec_packet:	db 13h				; Size of packet
sp_media:	db 0				; Media type
sp_drive:	db 0				; Drive number
sp_controller:	db 0				; Controller index
sp_lba:		dd 0				; LBA for emulated disk image
sp_devspec:	dw 0				; IDE/SCSI information
sp_buffer:	dw 0				; User-provided buffer
sp_loadseg:	dw 0				; Load segment
sp_sectors:	dw 0				; Sector count
sp_chs:		db 0,0,0			; Simulated CHS geometry
sp_dummy:	db 0				; Scratch, safe to overwrite

;
; EBIOS drive parameter packet
;
		align 8, db 0
drive_params:	dw 30				; Buffer size
dp_flags:	dw 0				; Information flags
dp_cyl:		dd 0				; Physical cylinders
dp_head:	dd 0				; Physical heads
dp_sec:		dd 0				; Physical sectors/track
dp_totalsec:	dd 0,0				; Total sectors
dp_secsize:	dw 0				; Bytes per sector
dp_dpte:	dd 0				; Device Parameter Table
dp_dpi_key:	dw 0				; 0BEDDh if rest valid
dp_dpi_len:	db 0				; DPI len
		db 0
		dw 0
dp_bus:		times 4 db 0			; Host bus type
dp_interface:	times 8 db 0			; Interface type
db_i_path:	dd 0,0				; Interface path
db_d_path:	dd 0,0				; Device path
		db 0
db_dpi_csum:	db 0				; Checksum for DPI info

;
; EBIOS disk address packet
;
		align 8, db 0
dapa:		dw 16				; Packet size
.count:		dw 0				; Block count
.off:		dw 0				; Offset of buffer
.seg:		dw 0				; Segment of buffer
.lba:		dd 0				; LBA (LSW)
		dd 0				; LBA (MSW)

		times 2048-($-$$) nop		; Pad to file offset 2048









