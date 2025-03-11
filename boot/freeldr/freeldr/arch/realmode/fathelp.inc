/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FAT12/16 Boot Sector Helper Code
 * COPYRIGHT:   Copyright 1998-2003 Brian Palmer (brianp@reactos.org)
 * COPYRIGHT:   Copyright 2019 Dmitry Borisov (di.sean@protonmail.com)
 *              Copyright 2022 Hermès Bélusca-Maïto
 */

//#include <asm.inc>

//org 8000h

//.text

#define BootSectorStackTop      HEX(7bf2)
#define DataAreaStartHigh       2
#define DataAreaStartLow        4
#define BiosCHSDriveSizeHigh    6
#define BiosCHSDriveSizeLow     8
#define BiosCHSDriveSize        8
#define ReadSectorsOffset       10
#define ReadClusterOffset       12
#define PutCharsOffset          14

#define OEMName                 3
#define BytesPerSector          11
#define SectsPerCluster         13
#define ReservedSectors         14
#define NumberOfFats            16
#define MaxRootEntries          17
#define TotalSectors            19
#define MediaDescriptor         21
#define SectorsPerFat           22
#define SectorsPerTrack         24
#define NumberOfHeads           26
#define HiddenSectors           28
#define TotalSectorsBig         32
#define BootDrive               36
#define Reserved                37
#define ExtendSig               38
#define SerialNumber            39
#define VolumeLabel             43
#define FileSystem              54

#define BootPartition           HEX(7dfd)


// This code will be stored in the first 512 bytes
// of freeldr.sys. The first 3 bytes will be a jmp
// instruction to skip past the FAT helper code
// that is stored in the rest of the 512 bytes.
//

PUBLIC start
start:
    // This code is loaded at 0000:F800 so we have to
    // encode a jmp instruction to jump to 0000:FA00
    .byte HEX(e9), HEX(fd), HEX(01)

// Now starts the extra boot code that we will store
// in the first 512 bytes of freeldr.sys. This code
// allows the FAT12/16 bootsector to navigate the
// FAT table so that we can still load freeldr.sys
// even if it is fragmented.

FatHelperEntryPoint:
    /* First save AX - the start cluster of freeldr.sys */
    push ax

    /* Display "Loading FreeLoader..." message */
    mov si, offset msgLoading
    call word ptr [bp-PutCharsOffset]

    /* Restore AX (start cluster) */
    pop ax
    xor dx, dx

    mov  bx, FREELDR_BASE / 16
    mov  es, bx

LoadFile:
    push ax
    call IsFat12
    pop  ax
    jnc  LoadFile2
    cmp  ax, HEX(0ff8)      // Check to see if this is the last cluster in the chain
    jmp  LoadFile3

LoadFile2:
    cmp  ax, HEX(0fff8)
LoadFile3:
    jae  LoadFile_Done      // If so continue, if not then read then next one
    push ax
    xor  bx, bx             // Load ROSLDR starting at 0000:F800h
    push es
    call word ptr [bp-ReadClusterOffset]
    pop  es

    xor  bx, bx
    mov  bl, byte ptr [bp+SectsPerCluster]
    shl  bx, 5                          // BX = BX * 512 / 16
    mov  ax, es                         // Increment the load address by
    add  ax, bx                         // The size of a cluster
    mov  es, ax

    call IsFat12
    pop  ax
    push es
    jnc  LoadFile4
    call GetFatEntry12                  // Get the next entry
    jmp  LoadFile5
LoadFile4:
    call GetFatEntry16
LoadFile5:
    pop  es

    jmp  LoadFile                       // Load the next cluster (if any)

LoadFile_Done:
    mov  dl, byte ptr [bp+BootDrive]        // Load the boot drive into DL
    mov  dh, byte ptr ds:[BootPartition]    // Load the boot partition into DH

    /* Continue where other bootsectors start */
    jmp start


// Returns the FAT entry for a given cluster number for 16-bit FAT
// On entry DX:AX has cluster number
// On return AX has FAT entry for that cluster
GetFatEntry16:
    // NOTE: Do the DX:AX arithmetics via EAX instead.
    xchg ax, dx
    shl eax, 16     // HIWORD(EAX) = old DX
    mov ax, dx      // LOWORD(EAX) = DX == old AX.

    shl ax, 1                       // AX = AX * 2 (since FAT16 entries are 2 bytes)

    mov cx, ax                      // Save this for later in CX

    // mov bx, HEX(9000)
    // add bx, dx
    // mov es, bx

    xor edx, edx // Dividend
    movzx ebx, word ptr [bp+BytesPerSector] // FAT Sector Number = original AX / BytesPerSector
    div ebx

    // Now reconstruct DX:AX from EAX
    mov dx, ax
    shr eax, 16
    xchg dx, ax

    add ax, word ptr [bp+HiddenSectors]     // FAT Sector Number += HiddenSectors
    adc dx, word ptr [bp+HiddenSectors+2]
    add ax, word ptr [bp+ReservedSectors]   // FAT Sector Number += ReservedSectors
    adc dx, 0

    dec  bx
    and  cx, bx                     // FAT Offset Within Sector = CX % BytesPerSector
                                    // DX:AX holds logical FAT sector number
                                    // CX holds FAT entry offset

    call LoadFatSector              // Get FAT entry in AX

    ret


// Returns the FAT entry for a given cluster number for 12-bit FAT
// On entry AX has cluster number
// On return AX has FAT entry for that cluster
GetFatEntry12:
    push  ax                        // Save original cluster number
    mov   cx, ax
    shr   ax, 1
    add   ax, cx                    // AX = AX * 1.5 (AX = AX + (AX / 2)) (since FAT12 entries are 12 bits)

    mov cx, ax                      // Save this for later in CX

    xor dx, dx // Dividend
    mov bx, word ptr [bp+BytesPerSector]    // FAT Sector Number = original AX / BytesPerSector
    div bx

    xor dx, dx
    add ax, word ptr [bp+HiddenSectors]     // FAT Sector Number += HiddenSectors
    adc dx, word ptr [bp+HiddenSectors+2]
    add ax, word ptr [bp+ReservedSectors]   // FAT Sector Number += ReservedSectors
    adc dx, 0

    dec  bx
    and  cx, bx                     // FAT Offset Within Sector = CX % BytesPerSector
                                    // DX:AX holds logical FAT sector number
                                    // CX holds FAT entry offset

    call LoadFatSector              // Get FAT entry in AX

    pop   cx                        // Get cluster number from stack
    and   cx, 1
    jz    UseLow12Bits
    and   ax, HEX(0fff0)
    shr   ax, 4
    jmp   GetFatEntry12_Done

UseLow12Bits:
    and   ax, HEX(0fff)

GetFatEntry12_Done:
    ret


/*
 * This code is directly adapted from fat32.S
 * BUT: Use DX:AX instead of EAX for the FAT logical sector number.
 * CX: FAT entry offset.
 */
LoadFatSector:
    push  cx                        // Save FAT entry offset

//
// FIXME: Why loading the sectors at 7000h do fail?
// In the meantime, do as with FAT32 and load the sectors
// at 9000h instead; this seems to work fine...
//
    mov   bx, HEX(9000)             // We will load it to [9000:0000h]
    mov   es, bx

    // DX:AX holds logical FAT sector number
    // Check if we have already loaded it
//
// FIXME: Why using cs:[...] does not work?
//
    cmp   ax, word ptr [bp+FatSectorInCache]
    jne   DoLoad
    cmp   dx, word ptr [bp+FatSectorInCache+2]
    je    LoadFatSectorAlreadyLoaded

DoLoad:
    mov word ptr [bp+FatSectorInCache], ax
    mov word ptr [bp+FatSectorInCache+2], dx
    xor bx, bx
    // Read 2 consecutive sectors, so that, in FAT12, entry data
    // that may cross sector boundaries, can be correctly read.
    // This is not needed in FAT16 and FAT32 where entry data is
    // aligned on word boundaries.
    mov cx, 2       // mov cx, word ptr [bp+SectorsPerFat]

    push es // The FAT12/16 bootsector ReadSectors destroys ES.
    call word ptr [bp-ReadSectorsOffset]
    pop es

LoadFatSectorAlreadyLoaded:
    pop   bx                        // Put FAT entry offset (originally in CX) into BX
    mov   ax, word ptr es:[bx]      // Get FAT entry
    ret

FatSectorInCache:                   // This variable tells us which sector we currently have in memory
    .long   HEX(0ffffffff)          // There is no need to re-read the same sector if we don't have to


// Returns CF = 1 if this is a FAT12 file system
// Otherwise CF = 0 for FAT16
IsFat12:
    mov   ebx, dword ptr [bp-DataAreaStartLow]
    // EBX now has the number of the starting sector of the data area
    // starting from the beginning of the disk, so subtract hidden sectors
    sub   ebx, dword ptr [bp+HiddenSectors]

    xor   eax,eax
    mov   ax, word ptr [bp+TotalSectors]
    cmp   ax, 0
    jnz   IsFat12_2
    mov   eax, dword ptr [bp+TotalSectorsBig]

    // EAX now contains the number of sectors on the volume

IsFat12_2:
    sub   eax, ebx                  // Subtract data area start sector
    xor   edx, edx                  // from total sectors of volume

    // EDX:EAX now contains the number of data sectors on the volume
    movzx ebx, byte ptr [bp+SectsPerCluster]
    div   ebx
    // EAX now has the number of clusters on the volume
    stc
    cmp   eax, 4085
    jb    IsFat12_Done
    clc

IsFat12_Done:
    ret


msgLoading:
    .ascii "Loading FreeLoader...", CR, LF, NUL

    .org 510                // Pad to 510 bytes
    .word HEX(0aa55)        // BootSector signature
