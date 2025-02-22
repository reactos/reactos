/*
 * PROJECT:     ReactOS Boot Sector for ISO file system (based on ISOLINUX)
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Booting ReactOS off a CD-ROM using the El Torito boot standard in "no emulation mode"
 * COPYRIGHT:   Copyright 1994-2009 H. Peter Anvin
 *              Copyright 2002 Michael K. Ter Louw
 *              Copyright 2002 Eric Kohl
 *              Copyright 2009 Intel Corporation *author: H. Peter Anvin
 *              Copyright 2011 Timo Kreuzer (timo.kreuzer@reactos.org)
 *              Copyright 2017 Colin Finck (colin@reactos.org)
 */

/* INCLUDES ******************************************************************/
#include <asm.inc>
#include <freeldr/include/arch/pc/x86common.h>

#ifndef ROS_REGTEST
#define WAIT_FOR_KEY
#endif


.code16
ASSUME CS:.text, DS:.text, ES:.text

/* CONSTANTS ******************************************************************/
BIOS_timer = HEX(046C)      // Timer ticks (1 word)
BIOS_magic = HEX(0472)      // BIOS reset magic (1 word)

// Memory below this point is reserved for the BIOS and the MBR
trackbuf = HEX(1000)        // Track buffer goes here (8192 bytes)
trackbufsize = 8192         // trackbuf ends at 3000h

// struct open_file_t
file_sector = 0             // Sector pointer (0 = structure free)
file_bytesleft = 4          // Number of bytes left
file_left = 8               // Number of sectors left
// Another unused DWORD follows here in ISOLINUX
#define open_file_t_size 16

// struct dir_t
dir_lba = 0                 // Directory start (LBA)
dir_len = 4                 // Length in bytes
dir_clust = 8               // Length in clusters
#define dir_t_size 12

MAX_OPEN_LG2 = 2            // log2(Max number of open files)
MAX_OPEN = 4
SECTOR_SHIFT = 11           // 2048 bytes/sector (El Torito requirement)
SECTOR_SIZE = 2048
retry_count = 6             // How patient are we with the BIOS?

/* UNINITIALIZED VARIABLES ****************************************************/
absolute HEX(5000)          // Here we keep our BSS stuff

resb ISOFileName, 64        // ISO filename canonicalization buffer
resb ISOFileNameEnd, 1
resb CurrentDir, dir_t_size // Current directory
resb RootDir, dir_t_size    // Root directory
resb DiskSys, 2             // Last INT 13h call
resb GetlinsecPtr, 2        // The sector-read pointer
resb DiskError, 1           // Error code for disk I/O
resb DriveNumber, 1         // CD-ROM BIOS drive number
resb ISOFlags, 1            // Flags for ISO directory search
resb RetryCount, 1          // Used for disk access retries

//align open_file_t_size
absolute HEX(5070)
resb Files, (MAX_OPEN * open_file_t_size)


/* ENTRY POINTS ***************************************************************/

// Entry point when booted from CD (El Torito standard)
start:
    mov bx, offset getlinsec_cdrom
    // Fall through

start_common:
    // Set up our stack and a flat addressing model.
    cli
    xor ax, ax
    mov ss, ax
    mov sp, offset start
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    sti

    // Our boot sector has been loaded to address 0x7C00.
    // Relocate our 2048 bytes boot sector to the given base address (should be 0x7000).
    cld
    mov cx, 2048 / 4
    mov si, HEX(7C00)
    mov di, offset start
    rep movsd

    ljmp16 0, relocated     // jump into relocated code

.org 64
hybrid_signature:
    .long HEX(7078c0fb)

// Entry point when booted through ISOMBR from a drive (isohybrid mode)
start_hybrid:
    mov bx, offset getlinsec_ebios
    jmp start_common

relocated:
    // Save our passed variables (BX from the entry point, DL from the BIOS) before anybody clobbers the registers.
    mov word ptr ds:[GetlinsecPtr], bx
    mov byte ptr ds:[DriveNumber], dl

    // Make sure the keyboard buffer is empty
    call pollchar_and_empty

    // If we're booting in hybrid mode and our boot drive is the first HDD (drive 80h),
    // we have no other option than booting into FREELDR.
    cmp word ptr ds:[GetlinsecPtr], offset getlinsec_ebios
    jne .read_mbr
    cmp byte ptr ds:[DriveNumber], HEX(80)
    je .boot_freeldr

.read_mbr:
    // Read the first sector (MBR) from the first hard disk (drive 80h) to 7C00h.
    // If we then decide to boot from HDD, we already have it at the right place.
    // In case of an error (indicated by the Carry Flag), just boot FREELDR from our ReactOS medium.
    mov ax, HEX(0201)
    mov dx, HEX(0080)
    mov cx, HEX(0001)
    mov bx, HEX(7C00)
    call int13
    jc .boot_freeldr

    // Verify the signature of the read MBR.
    // If it's invalid, there is probably no OS installed and we just boot FREELDR from our ReactOS medium.
    mov ax, word ptr ds:[HEX(7C00)+510]
    cmp ax, HEX(AA55)
    jne .boot_freeldr

#ifdef WAIT_FOR_KEY
    // We could either boot from the ReactOS medium or from hard disk. Let the user decide!
    // Display the 'Press key' message.
    call crlf_early
    mov si, offset presskey_msg
    call writestr_early

    // Count down 5 seconds.
    mov cx, 5

.next_second:
    // Count in seconds using the BIOS Timer, which runs roughly at 19 ticks per second.
    // Load its value plus one second into EAX for comparison later.
    mov eax, ds:[BIOS_timer]
    add eax, 19

.poll_again:
    // Check for a keypress, boot FREELDR from our ReactOS medium if a key was pressed.
    call pollchar_and_empty
    jnz .boot_freeldr

    // Check if another second has passed (in BIOS Timer ticks).
    mov ebx, ds:[BIOS_timer]
    cmp eax, ebx
    jnz .poll_again

    // Another second has passed, so print the dot and decrement the second counter.
    // If the user hasn't pressed a key after the entire 5 seconds have elapsed, just boot from the first hard disk.
    mov si, offset dot_msg
    call writestr_early
    dec cx
    jz .boot_harddisk
    jmp .next_second
#endif

.boot_harddisk:
    // Restore a clean context for the hard disk MBR and boot the already loaded MBR.
    call crlf_early
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov dx, HEX(0080)

    ljmp16 0, HEX(7C00)

.boot_freeldr:
#ifdef WAIT_FOR_KEY
    call crlf_early
    call crlf_early
#endif

    // The BIOS gave us a boot drive number, so in a perfect world we could just use that one now.
    // Unfortunately, there are many broken BIOSes around, which is why ISOLINUX verifies it and applies some hacks if the number is wrong.
    // Let's do exactly the same here to achieve maximum compatibility.

    // Don't do this if we are running in hybrid mode.
    cmp word ptr ds:[GetlinsecPtr], offset getlinsec_ebios
    je found_drive

    // Use the INT 13 function 4B01h (Get Disk Emulation Status) to fetch the El Torito Spec Packet.
    // We can use this information to verify that our passed boot drive number really belongs to our CD.
    mov ax, HEX(4B01)
    mov dl, byte ptr ds:[DriveNumber]
    mov si, offset spec_packet
    call int13

    // If this INT 13 function yields an error, we may be on a broken AWARD BIOS.
    // Check this and patch if possible.
    jc award_hack

    // Check that our passed boot drive number and the number in the Spec Packet match.
    // If not, try some workarounds to find our drive anyway.
    mov dl, byte ptr ds:[DriveNumber]
    cmp byte ptr ds:[sp_drive], dl
    jne spec_query_failed

found_drive:
    // Clear Files structures
    mov di, Files
    mov cx, (MAX_OPEN*open_file_t_size)/4
    xor eax, eax
    rep stosd

    // Read the entire 2K-sized ISO9660 Primary Volume Descriptor at sector 16 (32K).
    // This calculation only holds for single-session ISOs, but we should never encounter anything else.
    mov eax, 16
    mov bx, trackbuf
    call getonesec

    // Read the LBA address (offset 2 in the Directory Record) of the root directory (offset 156 in the Primary Volume Descriptor).
    mov eax, dword ptr ds:[trackbuf+156+2]
    mov dword ptr ds:[RootDir+dir_lba], eax
    mov dword ptr ds:[CurrentDir+dir_lba], eax

    // Read the data length (offset 10 in the Directory Record) of the root directory (offset 156 in the Primary Volume Descriptor).
    mov eax, dword ptr ds:[trackbuf+156+10]
    mov dword ptr ds:[RootDir+dir_len], eax
    mov dword ptr ds:[CurrentDir+dir_len], eax

    // Calculate the number of clusters and write that to our RootDir and CurrentDir structures.
    add eax, SECTOR_SIZE-1
    shr eax, SECTOR_SHIFT
    mov dword ptr ds:[RootDir+dir_clust],eax
    mov dword ptr ds:[CurrentDir+dir_clust],eax

    // Look for the "LOADER" directory (directory is indicated by AL = 2 when using searchdir_iso).
    mov di, offset loader_dir
    mov al, 2
    call searchdir_iso
    jnz .dir_found

    // No directory was found, so bail out with an error message.
    mov si, offset no_dir_msg
    call writemsg
    jmp kaboom

.dir_found:
    // The directory was found, so update the information in our CurrentDir structure.
    // Free the file pointer entry at SI in the process.
    mov dword ptr ds:[CurrentDir+dir_len], eax
    mov eax, dword ptr ds:[si+file_left]
    mov dword ptr ds:[CurrentDir+dir_clust], eax
    xor eax, eax
    xchg eax, dword ptr ds:[si+file_sector]
    mov dword ptr ds:[CurrentDir+dir_lba], eax

    // Look for the "FREELDR.SYS" file.
    mov di, offset freeldr_sys
    call searchdir
    jnz .freeldr_found

    // The FREELDR file was not found, so bail out with an error message.
    mov si, offset no_freeldr_msg
    call writemsg
    jmp kaboom

.freeldr_found:
    // Calculate the rounded up number of 2K sectors that need to be read.
    mov ecx, eax
    shr ecx, SECTOR_SHIFT
    test eax, HEX(7FF)
    jz .load_freeldr
    inc ecx

.load_freeldr:
    // Load the entire FREELDR.SYS (parameter CX = FFFFh) to its designated base address FREELDR_BASE.
    // Using a high segment address with offset 0 instead of segment 0 with offset FREELDR_BASE apparently increases compatibility with some BIOSes.
    mov bx, FREELDR_BASE / 16
    mov es, bx
    xor ebx, ebx
    mov cx, HEX(FFFF)
    call getfssec

    // Pass two parameters to FREELDR:
    //    DL = BIOS Drive Number
    //    DH = Boot Partition (0 for HDD booting in hybrid mode, FFh for CD booting)
    movzx dx, byte ptr ds:[DriveNumber]
    cmp word ptr ds:[GetlinsecPtr], offset getlinsec_ebios
    je .jump_to_freeldr
    mov dh, HEX(FF)

.jump_to_freeldr:
    // Transfer execution to the bootloader.
    ljmp16 0, FREELDR_BASE


/* FUNCTIONS *****************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Start of BrokenAwardHack --- 10-nov-2002           Knut_Petersen@t-online.de
///////////////////////////////////////////////////////////////////////////////
//
// There is a problem with certain versions of the AWARD BIOS ...
// the boot sector will be loaded and executed correctly, but, because the
// int 13 vector points to the wrong code in the BIOS, every attempt to
// load the spec packet will fail. We scan for the equivalent of
//
//     mov ax,0201h
//     mov bx,7c00h
//     mov cx,0006h
//     mov dx,0180h
//     pushf
//     call <direct far>
//
// and use <direct far> as the new vector for int 13. The code above is
// used to load the boot code into ram, and there should be no reason
// for anybody to change it now or in the future. There are no opcodes
// that use encodings relativ to IP, so scanning is easy. If we find the
// code above in the BIOS code we can be pretty sure to run on a machine
// with an broken AWARD BIOS ...
//
///////////////////////////////////////////////////////////////////////////////
award_oldint13:
    .long 0
award_string:
    .byte HEX(0b8),1,2,HEX(0bb),0,HEX(7c),HEX(0b9),6,0,HEX(0ba),HEX(80),1,HEX(09c),HEX(09a)

award_hack:
    mov si, offset spec_err_msg                         // Moved to this place from
    call writemsg                                       // spec_query_failed

    mov eax, dword ptr ds:[HEX(13)*4]
    mov dword ptr ds:[award_oldint13], eax

    push es
    mov ax, HEX(F000)                                   // ES = BIOS Seg
    mov es, ax
    cld
    xor di, di                                          // start at ES:DI = f000:0
award_loop:
    push di                                             // save DI
    mov si, offset award_string                         // scan for award_string
    mov cx, 7                                           // length of award_string = 7dw
    repz cmpsw                                          // compare
    pop di                                              // restore DI
    jcxz award_found                                    // jmp if found
    inc di                                              // not found, inc di
    jno award_loop

award_failed:
    pop es                                              // No, not this way :-((
award_fail2:
    mov eax, dword ptr ds:[award_oldint13]              // restore the original int
    or eax, eax                                         // 13 vector if there is one
    jz spec_query_failed                                // and try other workarounds
    mov dword ptr ds:[HEX(13)*4], eax
    jmp spec_query_failed

award_found:
    mov eax, dword ptr es:[di+HEX(0e)]                  // load possible int 13 addr
    pop es                                              // restore ES

    cmp eax, dword ptr ds:[award_oldint13]              // give up if this is the
    jz award_failed                                     // active int 13 vector,
    mov dword ptr ds:[HEX(13)*4], eax                   // otherwise change 0:13h*4

    mov ax, HEX(4B01)                                   // try to read the spec packet
    mov dl, byte ptr ds:[DriveNumber]                   // now ... it should not fail
    mov si, offset spec_packet                          // any longer
    int HEX(13)
    jc award_fail2

    jmp found_drive                                     // and leave error recovery code
///////////////////////////////////////////////////////////////////////////////
// End of BrokenAwardHack ----            10-nov-2002 Knut_Petersen@t-online.de
///////////////////////////////////////////////////////////////////////////////


// INT 13h, AX=4B01h, DL=<passed in value> failed.
// Try to scan the entire 80h-FFh from the end.
spec_query_failed:
    // some code moved to BrokenAwardHack

    mov dl, HEX(FF)

.test_loop:
    pusha
    mov ax, HEX(4B01)
    mov si, offset spec_packet
    mov byte ptr ds:[si], HEX(13)                       // Size of buffer
    call int13
    popa
    jc .still_broken

    mov si, offset maybe_msg
    call writemsg
    mov al, dl
    call writehex2
    call crlf_early

    cmp byte ptr ds:[sp_drive], dl
    jne .maybe_broken

    // Okay, good enough...
    mov si, offset alright_msg
    call writemsg
.found_drive0:
    mov byte ptr ds:[DriveNumber], dl
.found_drive:
    jmp found_drive

    // Award BIOS 4.51 apparently passes garbage in sp_drive,
    // but if this was the drive number originally passed in
    // DL then consider it "good enough"
.maybe_broken:
    mov al, byte ptr ds:[DriveNumber]
    cmp al, dl
    je .found_drive

    // Intel Classic R+ computer with Adaptec 1542CP BIOS 1.02
    // passes garbage in sp_drive, and the drive number originally
    // passed in DL does not have 80h bit set.
    or al, HEX(80)
    cmp al, dl
    je .found_drive0

.still_broken:
    dec dx
    cmp dl, HEX(80)
    jnb .test_loop

    // No spec packet anywhere.  Some particularly pathetic
    // BIOSes apparently don't even implement function
    // 4B01h, so we can't query a spec packet no matter
    // what.  If we got a drive number in DL, then try to
    // use it, and if it works, then well...
    mov dl, byte ptr ds:[DriveNumber]
    cmp dl, HEX(81)                                     // Should be 81-FF at least
    jb fatal_error                                      // If not, it's hopeless

    // Write a warning to indicate we're on *very* thin ice now
    mov si, offset nospec_msg
    call writemsg
    mov al, dl
    call writehex2
    call crlf_early
    jmp .found_drive                                    // Pray that this works...

fatal_error:
    mov si, offset nothing_msg
    call writemsg

.norge:
    jmp short .norge

//
// searchdir:
//
//      Open a file
//
//          On entry:
//              DS:DI   = filename
//          If successful:
//              ZF clear
//              SI      = file pointer
//              EAX     = file length in bytes
//          If unsuccessful
//              ZF set
//
// Assumes CS == DS == ES, and trashes BX and CX.
//
// searchdir_iso is a special entry point for ISOLINUX only.  In addition
// to the above, searchdir_iso passes a file flag mask in AL.  This is useful
// for searching for directories.
//
alloc_failure:
    xor ax, ax                                  // ZF <- 1
    ret

searchdir:
    xor al, al
searchdir_iso:
    mov byte ptr ds:[ISOFlags], al
    call allocate_file                          // Temporary file structure for directory
    jnz alloc_failure
    push es
    push ds
    pop es                                      // ES = DS
    mov si, offset CurrentDir
    cmp byte ptr ds:[di], '/'                   // If filename begins with slash
    jne .not_rooted
    inc di                                      // Skip leading slash
    mov si, offset RootDir                      // Reference root directory instead
.not_rooted:
    mov eax, dword ptr ds:[si+dir_clust]
    mov dword ptr ds:[bx+file_left], eax
    shl eax, SECTOR_SHIFT
    mov dword ptr ds:[bx+file_bytesleft], eax
    mov eax, dword ptr ds:[si+dir_lba]
    mov dword ptr ds:[bx+file_sector], eax
    mov edx, dword ptr ds:[si+dir_len]

.look_for_slash:
    mov ax, di
.scan:
    mov cl, byte ptr ds:[di]
    inc di
    and cl, cl
    jz .isfile
    cmp cl, '/'
    jne .scan
    mov byte ptr ds:[di-1], 0                   // Terminate at directory name
    mov cl, 2                                   // Search for directory
    xchg cl, byte ptr ds:[ISOFlags]

    push di                                     // Save these...
    push cx

    // Create recursion stack frame...
    push offset .resume                         // Where to "return" to
    push es
.isfile:
    xchg ax, di

.getsome:
    // Get a chunk of the directory
    // This relies on the fact that ISOLINUX doesn't change SI
    mov si, trackbuf
    pushad
    xchg bx, si
    mov cx, word ptr ds:[BufSafe]
    call getfssec
    popad

.compare:
    movzx eax, byte ptr ds:[si]                 // Length of directory entry
    cmp al, 33
    jb .next_sector
    mov cl, byte ptr ds:[si+25]
    xor cl, byte ptr ds:[ISOFlags]
    test cl, HEX(8E)                            // Unwanted file attributes!
    jnz .not_file
    pusha
    movzx cx, byte ptr ds:[si+32]               // File identifier length
    add si, 33                                  // File identifier offset
    call iso_compare_names
    popa
    je .success
.not_file:
    sub edx, eax                                // Decrease bytes left
    jbe .failure
    add si, ax                                  // Advance pointer

.check_overrun:
    // Did we finish the buffer?
    cmp si, trackbuf+trackbufsize
    jb .compare                                 // No, keep going

    jmp short .getsome                          // Get some more directory

.next_sector:
    // Advance to the beginning of next sector
    lea ax, [si+SECTOR_SIZE-1]
    and ax, not (SECTOR_SIZE-1)
    sub ax, si
    jmp short .not_file                         // We still need to do length checks

.failure:
    xor eax, eax                                // ZF = 1
    mov dword ptr ds:[bx+file_sector], eax
    pop es
    ret

.success:
    mov eax, dword ptr ds:[si+2]                // Location of extent
    mov dword ptr ds:[bx+file_sector], eax
    mov eax, dword ptr ds:[si+10]               // Data length
    mov dword ptr ds:[bx+file_bytesleft], eax
    push eax
    add eax, SECTOR_SIZE-1
    shr eax, SECTOR_SHIFT
    mov dword ptr ds:[bx+file_left], eax
    pop eax
    jz .failure                                 // Empty file?
    // ZF = 0
    mov si, bx
    pop es
    ret

.resume:
    // We get here if we were only doing part of a lookup
    // This relies on the fact that .success returns bx == si
    xchg edx, eax                               // Directory length in edx
    pop cx                                      // Old ISOFlags
    pop di                                      // Next filename pointer
    mov byte ptr ds:[di-1], '/'                 // Restore slash
    mov byte ptr ds:[ISOFlags], cl              // Restore the flags
    jz .failure                                 // Did we fail?  If so fail for real!
    jmp .look_for_slash                         // Otherwise, next level

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
    je .found
    add bx, open_file_t_size                    // ZF = 0
    loop .check
    // ZF = 0 if we fell out of the loop
.found:
    pop cx
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
    push di
    mov di, offset ISOFileName
.canon_loop:
    jcxz .canon_end
    lodsb
    dec cx
    cmp al, ';'
    je .canon_end
    and al, al
    je .canon_end
    stosb
    cmp di, offset ISOFileNameEnd-1             // Guard against buffer overrun
    jb .canon_loop
.canon_end:
    cmp di, ISOFileName
    jbe .canon_done
    cmp byte ptr ds:[di-1], '.'                 // Remove terminal dots
    jne .canon_done
    dec di
    jmp short .canon_end
.canon_done:
    mov byte ptr ds:[di], 0                     // Null-terminate string
    pop di
    mov si, ISOFileName
.compare2:
    lodsb
    mov ah, byte ptr ds:[di]
    inc di
    and ax, ax
    jz .success2                                // End of string for both
    and al, al                                  // Is either one end of string?
    jz .failure2                                // If so, failure
    and ah, ah
    jz .failure2
    or ax, HEX(2020)                            // Convert to lower case
    cmp al, ah
    je .compare2
.failure2:
    and ax, ax                                  // ZF = 0 (at least one will be nonzero)
.success2:
    ret

//
// getfssec: Get multiple clusters from a file, given the file pointer.
//
//  On entry:
//       ES:BX   -> Buffer
//       SI      -> File pointer
//       CX      -> Cluster count
//  On exit:
//       SI      -> File pointer (or 0 on EOF)
//       CF = 1  -> Hit EOF
//       ECX     -> Bytes actually read
//
getfssec:
    push ds
    push cs
    pop ds                                      // DS <- CS

    movzx ecx, cx
    cmp ecx, dword ptr ds:[si+file_left]
    jna .ok_size
    mov ecx, dword ptr ds:[si+file_left]
.ok_size:
    pushad
    mov eax, dword ptr ds:[si+file_sector]
    mov bp, cx
    call getlinsec
    popad

    // ECX[31:16] == 0 here...
    add dword ptr ds:[si+file_sector], ecx
    sub dword ptr ds:[si+file_left], ecx
    shl ecx, SECTOR_SHIFT                       // Convert to bytes
    cmp ecx, dword ptr ds:[si+file_bytesleft]
    jb .not_all
    mov ecx, dword ptr ds:[si+file_bytesleft]
.not_all:
    sub dword ptr ds:[si+file_bytesleft], ecx
    jnz .ret                                    // CF = 0 in this case...
    push eax
    xor eax, eax
    mov dword ptr ds:[si+file_sector], eax      // Unused
    mov si, ax
    pop eax
    stc
.ret:
    pop ds
    ret

//
// Information message (DS:SI) output
// Prefix with "ISOBOOT: "
//
writemsg:
    push ax
    push si
    mov si, offset isoboot_str
    call writestr_early
    pop si
    call writestr_early
    pop ax
    ret

writestr_early:
    pushfd
    pushad
.top:
    lodsb
    and al, al
    jz .end_writestr
    call writechr
    jmp short .top
.end_writestr:
    popad
    popfd
    ret

crlf_early:
    push ax
    mov al, 13
    call writechr
    mov al, 10
    call writechr
    pop ax
    ret

//
// writechr: Write a character to the screen.
//
writechr:
    pushfd
    pushad
    mov ah, HEX(0E)
    xor bx, bx
    int HEX(10)
    popad
    popfd
    ret

//
// int13: save all the segment registers and call INT 13h.
// Some CD-ROM BIOSes have been found to corrupt segment registers
// and/or disable interrupts.
//
int13:
    pushf
    push bp
    push ds
    push es
    push fs
    push gs
    int HEX(13)
    mov bp, sp
    setc byte ptr ds:[bp+10]                    // Propagate CF to the caller
    pop gs
    pop fs
    pop es
    pop ds
    pop bp
    popf
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
getlinsec:
    jmp word ptr cs:[GetlinsecPtr]

//
// getlinsec_ebios:
//
// getlinsec implementation for floppy/HDD EBIOS (EDD)
//
getlinsec_ebios:
    xor edx, edx
    shld edx, eax, 2
    shl eax, 2                                  // Convert to HDD sectors
    shl bp, 2

.loop_ebios:
    push bp                                     // Sectors left
.retry2:
    call maxtrans                               // Enforce maximum transfer size
    movzx edi, bp                               // Sectors we are about to read
    mov cx, retry_count
.retry:
    // Form DAPA on stack
    push edx
    push eax
    push es
    push bx
    push di
    push 16
    mov si, sp
    pushad
    mov dl, byte ptr ds:[DriveNumber]
    push ds
    push ss
    pop ds                                      // DS <- SS
    mov ah, HEX(42)                             // Extended Read
    call int13
    pop ds
    popad
    lea sp, [si+16]                             // Remove DAPA
    jc .error_ebios
    pop bp
    add eax, edi                                // Advance sector pointer
    adc edx, 0
    sub bp, di                                  // Sectors left
    shl di, 9                                   // 512-byte sectors
    add bx, di                                  // Advance buffer pointer
    jnc .no_overflow                            // Check if we have read more than 64K and need to adjust ES
    mov di, es
    add di, HEX(1000)                           // Adjust segment by 64K (1000h * 16 = 10000h = 64K + 1)
    mov es, di
.no_overflow:
    and bp, bp
    jnz .loop_ebios

    ret

.error_ebios:
    pushad                                      // Try resetting the device
    xor ax, ax
    mov dl, byte ptr ds:[DriveNumber]
    call int13
    popad
    loop .retry                                 // CX-- and jump if not zero

    // Total failure.
    jmp kaboom

//
// Truncate BP to MaxTransfer
//
maxtrans:
    cmp bp, word ptr ds:[MaxTransfer]
    jna .ok
    mov bp, word ptr ds:[MaxTransfer]
.ok:
    ret

//
// This is the variant we use for real CD-ROMs:
// LBA, 2K sectors, some special error handling.
//
getlinsec_cdrom:
    mov si, offset dapa                         // Load up the DAPA
    mov word ptr ds:[si+4], bx
    mov word ptr ds:[si+6], es
    mov dword ptr ds:[si+8], eax
.loop_cdrom:
    push bp                                     // Sectors left
    cmp bp, word ptr ds:[MaxTransferCD]
    jbe .bp_ok
    mov bp, word ptr ds:[MaxTransferCD]
.bp_ok:
    mov word ptr ds:[si+2], bp
    push si
    mov dl, byte ptr ds:[DriveNumber]
    mov ah, HEX(42)                             // Extended Read
    call xint13
    pop si
    pop bp
    movzx eax, word ptr ds:[si+2]               // Sectors we read
    add dword ptr ds:[si+8], eax                // Advance sector pointer
    sub bp, ax                                  // Sectors left
    shl ax, SECTOR_SHIFT-4                      // 2048-byte sectors -> segment
    add word ptr ds:[si+6], ax                  // Advance buffer pointer
    and bp, bp
    jnz .loop_cdrom
    mov eax, dword ptr ds:[si+8]                // Next sector
    ret

    // INT 13h with retry
xint13:
    mov byte ptr ds:[RetryCount], retry_count
.try:
    pushad
    call int13
    jc .error_cdrom
    add sp, 8*4                                 // Clean up stack
    ret
.error_cdrom:
    mov byte ptr ds:[DiskError], ah             // Save error code
    popad
    mov word ptr ds:[DiskSys], ax               // Save system call number
    dec byte ptr ds:[RetryCount]
    jz .real_error
    push ax
    mov al, byte ptr ds:[RetryCount]
    mov ah, byte ptr ds:[dapa+2]                // Sector transfer count
    cmp al, 2                                   // Only 2 attempts left
    ja .nodanger
    mov ah, 1                                   // Drop transfer size to 1
    jmp short .setsize
.nodanger:
    cmp al, retry_count-2
    ja .again                                   // First time, just try again
    shr ah, 1                                   // Otherwise, try to reduce
    adc ah, 0                                   // the max transfer size, but not to 0
.setsize:
    mov byte ptr ds:[MaxTransferCD], ah
    mov byte ptr ds:[dapa+2], ah
.again:
    pop ax
    jmp .try

.real_error:
    mov si, offset diskerr_msg
    call writemsg
    mov al, byte ptr ds:[DiskError]
    call writehex2
    mov si, offset oncall_str
    call writestr_early
    mov ax, word ptr ds:[DiskSys]
    call writehex4
    mov si, offset ondrive_str
    call writestr_early
    mov al, dl
    call writehex2
    call crlf_early
    // Fall through to kaboom

//
// kaboom: write a message and bail out.  Wait for a user keypress,
//      then do a hard reboot.
//
kaboom:
    // Restore a clean context.
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    sti

    // Display the failure message.
    mov si, offset err_bootfailed
    call writestr_early

    // Wait for a keypress.
    xor ax, ax
    int HEX(16)

    // Disable interrupts and reset the system through a magic BIOS call.
    cli
    mov word ptr ds:[BIOS_magic], 0
    ljmp16 HEX(0F000), HEX(0FFF0)

//
// writehex[248]: Write a hex number in (AL, AX, EAX) to the console
//
writehex2:
    pushfd
    pushad
    rol eax, 24
    mov cx,2
    jmp short writehex_common
writehex4:
    pushfd
    pushad
    rol eax, 16
    mov cx, 4
    jmp short writehex_common
writehex8:
    pushfd
    pushad
    mov cx, 8
writehex_common:
.loop_writehex:
    rol eax, 4
    push eax
    and al, HEX(0F)
    cmp al, 10
    jae .high
.low:
    add al, '0'
    jmp short .ischar
.high:
    add al, 'A'-10
.ischar:
    call writechr
    pop eax
    loop .loop_writehex
    popad
    popfd
    ret

//
// pollchar_and_empty: Check if we have an input character pending (ZF = 0)
// and empty the input buffer afterwards.
//
pollchar_and_empty:
    pushad
    mov ah, 1                                   // Did the user press a key?
    int HEX(16)
    jz .end_pollchar                            // No, then we're done
    mov ah, 0                                   // Otherwise empty the buffer by reading it
    int HEX(16)
.end_pollchar:
    popad
    ret


/* INITIALIZED VARIABLES *****************************************************/
presskey_msg:
    .ascii "Press any key to boot from the ReactOS medium", NUL
dot_msg:
    .ascii ".", NUL
isoboot_str:
    .ascii "ISOBOOT: ", NUL
spec_err_msg:
    .ascii "Loading spec packet failed, trying to wing it...", CR, LF, NUL
maybe_msg:
    .ascii "Found something at drive = ", NUL
alright_msg:
    .ascii "Looks reasonable, continuing...", CR, LF, NUL
nospec_msg:
    .ascii "Extremely broken BIOS detected, last attempt with drive = ", NUL
nothing_msg:
    .ascii "Failed to locate CD-ROM device; boot failed.", CR, LF, NUL
diskerr_msg:
    .ascii "Disk error ", NUL
oncall_str:
    .ascii ", AX = ", NUL
ondrive_str:
    .ascii ", drive ", NUL
err_bootfailed:
    .ascii CR, LF, "Boot failed: press a key to retry...", NUL
loader_dir:
    .ascii "/LOADER", NUL
no_dir_msg:
    .ascii "LOADER dir not found.", CR, LF, NUL
freeldr_sys:
    .ascii "FREELDR.SYS", NUL
no_freeldr_msg:
    .ascii "FREELDR.SYS not found.", CR, LF, NUL

.align 4
BufSafe:
    .word trackbufsize/SECTOR_SIZE              // Clusters we can load into trackbuf

// Maximum transfer size
.align 4
MaxTransfer:
    .word 127                                   // Hard disk modes
MaxTransferCD:
    .word 32                                    // CD mode

//
// El Torito spec packet
//
.align 8
spec_packet:
    .byte HEX(13)                               // Size of packet
sp_media:
    .byte 0                                     // Media type
sp_drive:
    .byte 0                                     // Drive number
sp_controller:
    .byte 0                                     // Controller index
sp_lba:
    .long 0                                     // LBA for emulated disk image
sp_devspec:
    .word 0                                     // IDE/SCSI information
sp_buffer:
    .word 0                                     // User-provided buffer
sp_loadseg:
    .word 0                                     // Load segment
sp_sectors:
    .word 0                                     // Sector count
sp_chs:
    .byte 0,0,0                                 // Simulated CHS geometry
sp_dummy:
    .byte 0                                     // Scratch, safe to overwrite

//
// EBIOS disk address packet
//
.align 8
dapa:
    .word 16                                    // Packet size
.count:
    .word 0                                     // Block count
.off:
    .word 0                                     // Offset of buffer
.seg:
    .word 0                                     // Segment of buffer
.lba:
    .long 0                                     // LBA (LSW)
    .long 0                                     // LBA (MSW)


// Extend the size to cover one 2K-sized sector
.org 2046
    .word HEX(0aa55)    // BootSector signature

.endcode16

END
