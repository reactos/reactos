/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Bootsector for ISO file system
 * FILE:
 * PURPOSE:
 * PROGRAMMERS:     ?
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <freeldr/include/arch/pc/x86common.h>

.code16

// ****************************************************************************
//
//  isolinux.asm
//
//  A program to boot Linux kernels off a CD-ROM using the El Torito
//  boot standard in "no emulation" mode, making the entire filesystem
//  available.  It is based on the SYSLINUX boot loader for MS-DOS
//  floppies.
//
//   Copyright (C) 1994-2001  H. Peter Anvin
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, Inc., 675 Mass Ave, Cambridge MA 02139,
//  USA; either version 2 of the License, or (at your option) any later
//  version; incorporated herein by reference.
//
// ****************************************************************************
//
// THIS FILE IS A MODIFIED VERSION OF ISOLINUX.ASM
// MODIFICATION DONE BY MICHAEL K TER LOUW
// LAST UPDATED 3-9-2002
// SEE "COPYING" FOR INFORMATION ABOUT THE LICENSE THAT APPLIES TO THIS RELEASE
//
// ****************************************************************************
//
// This file is a modified version of ISOLINUX.ASM.
// Modification done by Eric Kohl
// Last update 04-25-2002
//
// ****************************************************************************

//#define DEBUG_MESSAGES                /* Uncomment to get debugging messages */
#ifndef ROS_REGTEST
#define WAIT_FOR_KEY
#endif

// ****************************************************************************
//    BEGIN THE BIOS/CODE/DATA SEGMENT
// ****************************************************************************
serial_base = HEX(0400)     // Base addresses for 4 serial ports (4 words)
BIOS_fbm = HEX(0413)        // Free Base Memory (kilobytes) (1 word)
BIOS_timer = HEX(046C)      // Timer ticks (1 word)
BIOS_magic = HEX(0472)      // BIOS reset magic (1 word)
BIOS_vidrows = HEX(0484)    // Number of screen rows (1 byte)

// Memory below this point is reserved for the BIOS and the MBR
trackbuf = HEX(1000)        // Track buffer goes here (8192 bytes)
trackbufsize = 8192         // trackbuf ends at 3000h

// struct open_file_t
file_sector = 0             // Sector pointer (0 = structure free)
file_left = 4               // Number of sectors left

//struct dir_t
dir_lba = 0                 // Directory start (LBA)
dir_len = 4                 // Length in bytes
dir_clust = 8               // Length in clusters

#define dir_t_size 12
#define open_file_t_size 8

MAX_OPEN_LG2 = 2            // log2(Max number of open files)
MAX_OPEN = 4
SECTORSIZE_LG2 = 11         // 2048 bytes/sector (El Torito requirement)
SECTORSIZE = 2048
retry_count    = 6          // How patient are we with the BIOS?

/******************************************************************************/
absolute HEX(5000)          // Here we keep our BSS stuff

resb DriveNo, 1             // CD-ROM BIOS drive number (BYTE)
resb DiskError, 1           // Error code for disk I/O (BYTE)
resb RetryCount, 1          // Used for disk access retries (BYTE)
resb TimeoutCount, 1        // Timeout counter (BYTE)
resb ISOFlags, 1            // Flags for ISO directory search (BYTE)
resb RootDir, dir_t_size    // Root directory (dir_t_size BYTES)
resb CurDir, dir_t_size     // Current directory (dir_t_size BYTES)
resb ISOFileName, 64        // ISO filename canonicalization buffer
resb ISOFileNameEnd, 1

//align open_file_t_size
absolute HEX(5060)
resb Files, (MAX_OPEN * open_file_t_size)


/******************************************************************************/


start:
    cli                     // Disable interrupts
    xor ax, ax              // ax = segment zero
    mov ss, ax              // Initialize stack segment
    mov sp, offset start    // Set up stack
    mov ds, ax              // Initialize other segment registers
    mov es, ax
    mov fs, ax
    mov gs, ax
    sti                     // Enable interrupts
    cld                     // Increment pointers

    mov cx, 2048 / 4        // Copy the bootsector
    mov si, HEX(7C00)       // from 0000:7C00
    mov di, HEX(7000)       // to 0000:7000
    rep movsd               // copy the program

    ljmp16 0, relocate      // jump into relocated code

relocate:
#ifdef DEBUG_MESSAGES
    // Display the banner and copyright
    mov si, offset isolinux_banner // si points to hello message
    call writestr            // display the message
    mov si, offset copyright_str
    call writestr
#endif

    // Make sure the keyboard buffer is empty
    call pollchar_and_empty

    // Check for MBR on harddisk
    pusha
    mov ax, HEX(0201)
    mov dx, HEX(0080)
    mov cx, HEX(0001)
    mov bx, trackbuf
    int HEX(13)
    popa
    jc .boot_cdrom          // could not read hdd

    push ax
#ifdef ROS_REGTEST // this change is taken from the original isobtrt.asm
    mov ax, word ptr ds:[trackbuf+510]
#else
    mov ax, word ptr ds:[trackbuf]
#endif
    cmp ax, 0
    je .boot_cdrom // no boot sector found (hopefully there are no weird bootsectors which begin with 0)
    pop ax

#ifdef WAIT_FOR_KEY
    // Display the 'Press key' message and wait for a maximum of 5 seconds
    call crlf
    mov si, offset presskey_msg // si points to 'Press key' message
    call writestr               // display the message

    mov byte ptr ds:[TimeoutCount], 5
.next_second:
    mov eax, ds:[BIOS_timer]        // load current tick counter
    add eax, 19

.poll_again:
    call pollchar_and_empty
    jnz .boot_cdrom

    mov ebx, ds:[BIOS_timer]
    cmp eax, ebx
    jnz .poll_again

    mov si, offset dot_msg            // print '.'
    call writestr
    dec byte ptr ds:[TimeoutCount]        // decrement timeout counter
    jz .boot_harddisk
    jmp .next_second
#endif

.boot_harddisk:
    call crlf

    // Boot first harddisk (drive 0x80)
    mov ax, HEX(0201)
    mov dx, HEX(0080)
    mov cx, HEX(0001)
    mov bx, HEX(7C00)
    int HEX(13)
    jnc .go_hd
    jmp kaboom
.go_hd:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov dx, HEX(0080)

    ljmp16 0, HEX(7C00)

.boot_cdrom:
#ifdef WAIT_FOR_KEY
    call crlf
    call crlf
#endif

    // Save and display the boot drive number
    mov byte ptr ds:[DriveNo], dl
#ifdef DEBUG_MESSAGES
    mov si, offset startup_msg
    call writemsg
    mov al, dl
    call writehex2
    call crlf
#endif

    // Now figure out what we're actually doing
    // Note: use passed-in DL value rather than 7Fh because
    // at least some BIOSes will get the wrong value otherwise
    mov ax, HEX(4B01)            // Get disk emulation status
    mov dl, byte ptr ds:[DriveNo]
    mov si, offset spec_packet
    int HEX(13)
    jc    spec_query_failed        // Shouldn't happen (BIOS bug)
    mov dl, byte ptr ds:[DriveNo]
    cmp byte ptr ds:[sp_drive], dl            // Should contain the drive number
    jne    spec_query_failed

#ifdef DEBUG_MESSAGES
    mov si, offset spec_ok_msg
    call writemsg
    mov al, byte ptr ds:[sp_drive]
    call writehex2
    call crlf
#endif

found_drive:
    // Get drive information
    mov ah, HEX(48)
    mov dl, byte ptr ds:[DriveNo]
    mov si, offset drive_params
    int HEX(13)
    jnc params_ok

    // mov si, nosecsize_msg    No use in reporting this
    // call writemsg

params_ok:
    // Check for the sector size (should be 2048, but
    // some BIOSes apparently think we're 512-byte media)
    //
    // FIX: We need to check what the proper behaviour
    // is for getlinsec when the BIOS thinks the sector
    // size is 512!!!  For that, we need such a BIOS, though...
#ifdef DEBUG_MESSAGES
    mov si, offset secsize_msg
    call writemsg
    mov ax, word ptr ds:[dp_secsize]
    call writehex4
    call crlf
#endif


    //
    // Clear Files structures
    //
    mov di, Files
    mov cx, (MAX_OPEN*open_file_t_size)/4
    xor    eax, eax
    rep    stosd

    //
    // Now, we need to sniff out the actual filesystem data structures.
    // mkisofs gave us a pointer to the primary volume descriptor
    // (which will be at 16 only for a single-session disk!); from the PVD
    // we should be able to find the rest of what we need to know.
    //
get_fs_structures:
    mov eax, 16            // Primary Volume Descriptor (sector 16)
    mov bx, trackbuf
    call getonesec

    mov eax, dword ptr ds:[trackbuf+156+2]
    mov dword ptr ds:[RootDir+dir_lba],eax
    mov dword ptr ds:[CurDir+dir_lba],eax
#ifdef DEBUG_MESSAGES
    mov si, offset rootloc_msg
    call writemsg
    call writehex8
    call crlf
#endif

    mov eax, dword ptr ds:[trackbuf+156+10]
    mov dword ptr ds:[RootDir+dir_len],eax
    mov dword ptr ds:[CurDir+dir_len],eax
#ifdef DEBUG_MESSAGES
    mov si, offset rootlen_msg
    call writemsg
    call writehex8
    call crlf
#endif
    add    eax,SECTORSIZE-1
    shr    eax,SECTORSIZE_LG2
    mov dword ptr ds:[RootDir+dir_clust],eax
    mov dword ptr ds:[CurDir+dir_clust],eax
#ifdef DEBUG_MESSAGES
    mov si, offset rootsect_msg
    call writemsg
    call writehex8
    call crlf
#endif

    // Look for the "REACTOS" directory, and if found,
    // make it the current directory instead of the root
    // directory.
    mov di, offset isolinux_dir
    mov al, 2                // Search for a directory
    call searchdir_iso
    jnz .dir_found
    mov si, offset no_dir_msg
    call writemsg
    jmp    kaboom

.dir_found:
    mov dword ptr ds:[CurDir+dir_len],eax
    mov eax, dword ptr ds:[si+file_left]
    mov dword ptr ds:[CurDir+dir_clust],eax
    xor    eax,eax                // Free this file pointer entry
    xchg    eax,dword ptr ds:[si+file_sector]
    mov dword ptr ds:[CurDir+dir_lba],eax


    mov di, offset isolinux_bin        // di points to Isolinux filename
    call searchdir            // look for the file
    jnz .isolinux_opened        // got the file
    mov si, offset no_isolinux_msg        // si points to error message
    call writemsg            // display the message
    jmp    kaboom                // fail boot

.isolinux_opened:
    mov di, si                // save file pointer

#ifdef DEBUG_MESSAGES
    mov si, offset filelen_msg
    call writemsg
    call writehex8
    call crlf
#endif

    mov ecx, eax            // calculate sector count
    shr ecx, 11
    test eax, HEX(7FF)
    jz .full_sector
    inc ecx
.full_sector:

#ifdef DEBUG_MESSAGES
    mov eax, ecx
    mov si, offset filesect_msg
    call writemsg
    call writehex8
    call crlf
#endif

// use high segment, as some bios can fail, when offset is too big
    mov bx, FREELDR_BASE / 16 // es = load segment
    mov es, bx
    xor ebx, ebx              // bx = load offset
    mov si, di                // restore file pointer
    mov cx, HEX(0FFFF)        // load the whole file
    call getfssec             // get the whole file

#ifdef DEBUG_MESSAGES
    mov si, offset startldr_msg
    call writemsg
    call crlf
#endif

    mov dl, byte ptr ds:[DriveNo]            // dl = boot drive
    mov dh, 0                    // dh = boot partition

    /* Transfer execution to the bootloader */
    ljmp16 0, FREELDR_BASE

//
// searchdir:
//
// Open a file
//
//  On entry:
//    DS:DI    = filename
//  If successful:
//    ZF clear
//    SI        = file pointer
//    DX:AX or EAX    = file length in bytes
//  If unsuccessful
//    ZF set
//

//
// searchdir_iso is a special entry point for ISOLINUX only.  In addition
// to the above, searchdir_iso passes a file flag mask in AL.  This is useful
// for searching for directories.
//
alloc_failure:
    xor    ax,ax                // ZF <- 1
    ret

searchdir:
    xor    al, al
searchdir_iso:
    mov byte ptr ds:[ISOFlags],al
    call allocate_file            // Temporary file structure for directory
    jnz alloc_failure
    push    es
    push    ds
    pop    es                // ES = DS
    mov si, offset CurDir
    cmp byte ptr ds:[di], 92 //'\'            // If filename begins with slash
    jne    .not_rooted
    inc    di                // Skip leading slash
    mov si, offset RootDir            // Reference root directory instead
.not_rooted:
    mov eax, dword ptr ds:[si+dir_clust]
    mov dword ptr ds:[bx+file_left],eax
    mov eax,dword ptr ds:[si+dir_lba]
    mov dword ptr ds:[bx+file_sector],eax
    mov edx,dword ptr ds:[si+dir_len]

.look_for_slash:
    mov ax,di
.scan:
    mov cl, byte ptr ds:[di]
    inc    di
    and    cl,cl
    jz    .isfile
    cmp cl, 92 // '\'
    jne    .scan
    mov byte ptr ds:[di-1], 0            // Terminate at directory name
    mov cl,2                // Search for directory
    xchg    cl, byte ptr ds:[ISOFlags]
    push    di
    push    cx
    push    offset .resume            // Where to "return" to
    push    es
.isfile:
    xchg    ax,di

.getsome:
    // Get a chunk of the directory
    mov si,trackbuf
    pushad
    xchg    bx,si
    mov cx,1                // load one sector
    call getfssec
    popad

.compare:
    movzx    eax, byte ptr ds:[si]            // Length of directory entry
    cmp al, 33
    jb    .next_sector
    mov cl, byte ptr ds:[si+25]
    xor    cl, byte ptr ds:[ISOFlags]
    test    cl, HEX(8E)            // Unwanted file attributes!
    jnz .not_file
    pusha
    movzx    cx, byte ptr ds:[si+32]        // File identifier length
    add    si, 33            // File identifier offset
    call iso_compare_names
    popa
    je    .success
.not_file:
    sub    edx, eax            // Decrease bytes left
    jbe    .failure
    add    si, ax                // Advance pointer

.check_overrun:
    // Did we finish the buffer?
    cmp si, trackbuf+trackbufsize
    jb    .compare            // No, keep going

    jmp    .getsome            // Get some more directory

.next_sector:
    // Advance to the beginning of next sector
    lea    ax, [si+SECTORSIZE-1]
    and    ax, not (SECTORSIZE-1)
    sub    ax, si
    jmp    .not_file            // We still need to do length checks

.failure:
#ifdef DEBUG_MESSAGES
    mov si, offset findfail_msg
    call writemsg
    call crlf
#endif
    xor    eax, eax            // ZF = 1
    mov dword ptr ds:[bx+file_sector], eax
    pop    es
    ret

.success:
    mov eax, dword ptr ds:[si+2]            // Location of extent
    mov dword ptr ds:[bx+file_sector], eax
    mov eax, dword ptr ds:[si+10]            // Data length
    push    eax
    add    eax, SECTORSIZE-1
    shr    eax, SECTORSIZE_LG2
    mov dword ptr ds:[bx+file_left], eax
    pop    eax
    mov edx, eax
    shr    edx, 16
    and    bx, bx                // ZF = 0
    mov si, bx
    pop    es
    ret

.resume:
    // We get here if we were only doing part of a lookup
    // This relies on the fact that .success returns bx == si
    xchg    edx, eax            // Directory length in edx
    pop    cx                // Old ISOFlags
    pop    di                // Next filename pointer

    // restore the backslash in the filename
    mov byte ptr ds:[di-1], 92 // '\'

    mov byte ptr ds:[ISOFlags], cl            // Restore the flags
    jz    .failure            // Did we fail?  If so fail for real!
    jmp    .look_for_slash            // Otherwise, next level

//
// allocate_file: Allocate a file structure
//
//        If successful:
//          ZF set
//          BX = file pointer
//        In unsuccessful:
//          ZF clear
//
allocate_file:
    push cx
    mov bx, Files
    mov cx, MAX_OPEN
.check:
    cmp dword ptr ds:[bx], 0
    je    .found
    add    bx, open_file_t_size        // ZF = 0
    loop    .check
    // ZF = 0 if we fell out of the loop
.found:
    pop    cx
    ret

//
// iso_compare_names:
//    Compare the names DS:SI and DS:DI and report if they are
//    equal from an ISO 9660 perspective.  SI is the name from
//    the filesystem; CX indicates its length, and ';' terminates.
//    DI is expected to end with a null.
//
//    Note: clobbers AX, CX, SI, DI; assumes DS == ES == base segment
//
iso_compare_names:
    // First, terminate and canonicalize input filename
    push    di
    mov di, offset ISOFileName
.canon_loop:
    jcxz    .canon_end
    lodsb
    dec    cx
    cmp al, ';'
    je    .canon_end
    and    al, al
    je    .canon_end
    stosb
    cmp di, offset ISOFileNameEnd-1        // Guard against buffer overrun
    jb    .canon_loop
.canon_end:
    cmp di, ISOFileName
    jbe    .canon_done
    cmp byte ptr ds:[di-1], '.'        // Remove terminal dots
    jne    .canon_done
    dec    di
    jmp    short .canon_end
.canon_done:
    mov byte ptr ds:[di], 0            // Null-terminate string
    pop    di
    mov si, ISOFileName
.compare2:
    lodsb
    mov ah, byte ptr ds:[di]
    inc    di
    and    ax, ax
    jz    .success2            // End of string for both
    and    al, al                // Is either one end of string?
    jz    .failure2            // If so, failure
    and    ah, ah
    jz    .failure2
    or    ax, HEX(2020)            // Convert to lower case
    cmp al, ah
    je    .compare2
.failure2:
    and    ax, ax                // ZF = 0 (at least one will be nonzero)
.success2:
    ret







//
// getfssec: Get multiple clusters from a file, given the file pointer.
//
//  On entry:
//    ES:BX    -> Buffer
//    SI    -> File pointer
//    CX    -> Cluster count; 0FFFFh = until end of file
//  On exit:
//    SI    -> File pointer (or 0 on EOF)
//    CF = 1    -> Hit EOF
//
getfssec:
    cmp cx, word ptr ds:[si+file_left]
    jna    .ok_size
    mov cx, word ptr ds:[si+file_left]

.ok_size:
    mov bp, cx
    push    cx
    push    si
    mov eax, dword ptr ds:[si+file_sector]
    call getlinsec
    xor    ecx, ecx
    pop    si
    pop    cx

    add    dword ptr ds:[si+file_sector], ecx
    sub    dword ptr ds:[si+file_left], ecx
    ja    .not_eof            // CF = 0

    xor    ecx, ecx
    mov dword ptr ds:[si+file_sector], ecx        // Mark as unused
    xor    si,si
    stc

.not_eof:
    ret


// INT 13h, AX=4B01h, DL=<passed in value> failed.
// Try to scan the entire 80h-FFh from the end.
spec_query_failed:
    mov si, offset spec_err_msg
    call writemsg

    mov dl, HEX(0FF)
.test_loop:
    pusha
    mov ax, HEX(4B01)
    mov si, offset spec_packet
    mov byte ptr ds:[si], 13            // Size of buffer
    int HEX(13)
    popa
    jc    .still_broken

    mov si, offset maybe_msg
    call writemsg
    mov al, dl
    call writehex2
    call crlf

    cmp byte ptr ds:[sp_drive], dl
    jne    .maybe_broken

    // Okay, good enough...
    mov si, offset alright_msg
    call writemsg
    mov byte ptr ds:[DriveNo], dl
.found_drive:
    jmp    found_drive

    // Award BIOS 4.51 apparently passes garbage in sp_drive,
    // but if this was the drive number originally passed in
    // DL then consider it "good enough"
.maybe_broken:
    cmp byte ptr ds:[DriveNo], dl
    je .found_drive

.still_broken:
    dec dx
    cmp dl, HEX(80)
    jnb    .test_loop

fatal_error:
    mov si, offset nothing_msg
    call writemsg

.norge:
    jmp    .norge



// Information message (DS:SI) output
// Prefix with "isolinux: "
writemsg:
    push    ax
    push    si
    mov si, offset isolinux_str
    call writestr
    pop    si
    call writestr
    pop    ax
    ret

//
// crlf: Print a newline
crlf:
    mov si, offset crlf_msg
    // Fall through

//
// writestr: write a null-terminated string to the console, saving
//           registers on entry.
//
writestr:
    pushfd
    pushad
writestr_top:
    lodsb
    and    al, al
    jz    writestr_end
    call writechr
    jmp    short writestr_top
writestr_end:
    popad
    popfd
    ret

//
// writehex[248]: Write a hex number in (AL, AX, EAX) to the console
//
writehex2:
    pushfd
    pushad
    shl    eax, 24
    mov cx, 2
    jmp    short writehex_common
writehex4:
    pushfd
    pushad
    shl    eax, 16
    mov cx, 4
    jmp    short writehex_common
writehex8:
    pushfd
    pushad
    mov cx, 8
writehex_common:
.loop:
    rol    eax, 4
    push    eax
    and    al, HEX(0F)
    cmp al, 10
    jae    .high
.low:
    add    al, '0'
    jmp    short .ischar
.high:
    add    al, 'A'-10
.ischar:
    call writechr
    pop    eax
    loop    .loop
    popad
    popfd
    ret

//
// writechr: Write a character to the screen.  There is a more "sophisticated"
// version of this in the subsequent code, so we patch the pointer
// when appropriate.
writechr:
    pushfd
    pushad
    mov ah, HEX(0E)
    xor    bx, bx
    int HEX(10)
    popad
    popfd
    ret

//
// Get one sector.  Convenience entry point.
//
getonesec:
    mov bp, 1
    // Fall through to getlinsec

//
// Get linear sectors - EBIOS LBA addressing, 2048-byte sectors.
//
// Note that we can't always do this as a single request, because at least
// Phoenix BIOSes has a 127-sector limit.  To be on the safe side, stick
// to 32 sectors (64K) per request.
//
// Input:
//    EAX    - Linear sector number
//    ES:BX    - Target buffer
//    BP    - Sector count
//
getlinsec:
    push es // save es, we reset it later to 0

    mov si, offset dapa            // Load up the DAPA
    mov word ptr ds:[si+4], bx
    mov bx, es
    mov word ptr ds:[si+6], bx
    xor bx, bx // reset es to 0, some bioses (KVM) require that
    mov es, bx
    mov dword ptr ds:[si+8], eax
.loop2:
    push bp                // Sectors left
    cmp bp, word ptr ds:[MaxTransfer]
    jbe .bp_ok
    mov bp, word ptr ds:[MaxTransfer]
.bp_ok:
    mov word ptr ds:[si+2], bp
    push si
    mov dl, byte ptr ds:[DriveNo]
    mov ah, HEX(42)            // Extended Read
    call xint13
    pop si
    pop bp
    movzx eax,word ptr ds:[si+2]        // Sectors we read
    add dword ptr ds:[si+8], eax        // Advance sector pointer
    sub bp, ax            // Sectors left
    shl ax, SECTORSIZE_LG2-4        // 2048-byte sectors -> segment
    add word ptr ds:[si+6], ax            // Advance buffer pointer
    and bp, bp
    jnz .loop2
    mov eax, dword ptr ds:[si+8]            // Next sector

    pop es
    ret

    // INT 13h with retry
xint13:
    mov byte ptr ds:[RetryCount], retry_count
.try:
    pushad
    int HEX(13)
    jc    .error
    add    sp, 8*4            // Clean up stack
    ret
.error:
    mov byte ptr ds:[DiskError], ah        // Save error code
    popad
    dec byte ptr ds:[RetryCount]
    jz .real_error
    push ax
    mov al, byte ptr ds:[RetryCount]
    mov ah, byte ptr ds:[dapa+2]            // Sector transfer count
    cmp al,2            // Only 2 attempts left
    ja .nodanger
    mov ah,1            // Drop transfer size to 1
    jmp short .setsize
.nodanger:
    cmp al, retry_count-2
    ja .again            // First time, just try again
    shr ah,1            // Otherwise, try to reduce
    adc ah,0            // the max transfer size, but not to 0
.setsize:
    mov byte ptr ds:[MaxTransfer],ah
    mov byte ptr ds:[dapa+2],ah
.again:
    pop ax
    jmp .try

.real_error:
    mov si, offset diskerr_msg
    call writemsg
    mov al, byte ptr ds:[DiskError]
    call writehex2
    mov si, offset ondrive_str
    call writestr
    mov al, dl
    call writehex2
    call crlf
    // Fall through to kaboom

//
// kaboom: write a message and bail out.  Wait for a user keypress,
//      then do a hard reboot.
//
kaboom:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    sti
    mov si, offset err_bootfailed
    call writestr
    xor    ax, ax        // Wait for keypress
    int HEX(16)
    cli
    mov word ptr ds:[BIOS_magic], 0    // Cold reboot
    ljmp16 HEX(0F000), HEX(0FFF0)        // Reset vector address


//
// pollchar_and_empty: check if we have an input character pending (ZF = 0) and empty the input buffer afterwards
//
pollchar_and_empty:
    pushad
    mov ah, 1        // Did the user press a key?
    int HEX(16)
    jz    .end        // No, then we're done
    mov ah, 0        // Otherwise empty the buffer by reading it
    int HEX(16)
.end:
    popad
    ret


isolinux_banner:
    .ascii CR, LF, "Loading IsoBoot...", CR, LF, NUL
copyright_str:
    .ascii " (C) 1994-2002 H. Peter Anvin", CR, LF, NUL
presskey_msg:
    .ascii "Press any key to boot from CD", NUL
dot_msg:
    .ascii ".", NUL

#ifdef DEBUG_MESSAGES
startup_msg:
    .ascii "Startup, DL = '", NUL
spec_ok_msg:
    .ascii "packet OK, drive = ", NUL
secsize_msg:
    .ascii "size appears to be ", NUL
rootloc_msg:
    .ascii "Root dir loc: ", NUL
rootlen_msg:
    .ascii "Root dir len: ", NUL
rootsect_msg:
    .ascii "Root dir len(sect): ", NUL
fileloc_msg:
    .ascii "SETUPLDR loc: ", NUL
filelen_msg:
    .ascii "SETUPLDR len: ", NUL
filesect_msg:
    .ascii "SETUPLDR len(sect): ", NUL
findfail_msg:
    .ascii "Failed to find file!", NUL
startldr_msg:
    .ascii "Starting SETUPLDR.SYS", NUL
#endif

spec_err_msg:
    .ascii "Load spec failed, trying wing ...", CR, LF, NUL
maybe_msg:
    .ascii "Found smth at drive = ", NUL
alright_msg:
    .ascii "might be ok, continuing...", CR, LF, NUL
nothing_msg:
    .ascii "Failed locate CD-ROM; boot failed.", CR, LF, NUL

isolinux_str:
    .ascii "IsoBoot: ", NUL
crlf_msg:
    .ascii CR, LF, NUL
diskerr_msg:
    .ascii "Disk error ", NUL
ondrive_str:
    .ascii ", drive ", NUL

err_bootfailed:
    .ascii CR, LF, "failed..", NUL
isolinux_dir:
    .ascii "\\LOADER", NUL
no_dir_msg:
    .ascii "LOADER dir not found.", CR, LF, NUL
isolinux_bin:
    .ascii "SETUPLDR.SYS", NUL
no_isolinux_msg:
    .ascii "SETUPLDR not found.", CR, LF, NUL


//
// El Torito spec packet
//
.align 8
spec_packet:
    .byte HEX(13)          // Size of packet
sp_media:
    .byte 0                // Media type
sp_drive:
    .byte 0                // Drive number
sp_controller:
    .byte 0                // Controller index
sp_lba:
    .long 0                // LBA for emulated disk image
sp_devspec:
    .word 0                // IDE/SCSI information
sp_buffer:
    .word 0                // User-provided buffer
sp_loadseg:
    .word 0                // Load segment
sp_sectors:
    .word 0                // Sector count
sp_chs:
    .byte 0,0,0            // Simulated CHS geometry
sp_dummy:
    .byte 0                // Scratch, safe to overwrite

//
// EBIOS drive parameter packet
//
.align 8
drive_params:
    .word 30                // Buffer size
dp_flags:
    .word 0                // Information flags
dp_cyl:
    .long 0                // Physical cylinders
dp_head:
    .long 0                // Physical heads
dp_sec:
    .long 0                // Physical sectors/track
dp_totalsec:
    .long 0,0                // Total sectors
dp_secsize:
    .word 0                // Bytes per sector
dp_dpte:
    .long 0                // Device Parameter Table
dp_dpi_key:
    .word 0                // 0BEDDh if rest valid
dp_dpi_len:
    .byte 0                // DPI len
    .byte 0
    .word 0
dp_bus:
    .byte 0,0,0,0            // Host bus type
dp_interface:
    .byte 0,0,0,0,0,0,0,0            // Interface type
db_i_path:
    .long 0,0                // Interface path
db_d_path:
    .long 0,0                // Device path
    .byte 0
db_dpi_csum:
    .byte 0                // Checksum for DPI info

//
// EBIOS disk address packet
//
.align 8
dapa:
    .word 16                // Packet size
.count:
    .word 0                // Block count
.off:
    .word 0                // Offset of buffer
.seg:
    .word 0                // Segment of buffer
.lba:
    .long 0                // LBA (LSW)
    .long 0                // LBA (MSW)

.align 4
MaxTransfer:
    .word 2 //32                // Max sectors per transfer

.org 2046        // Pad to file offset 2046
.word HEX(0aa55)    // BootSector signature

.endcode16

END
