/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     GPT bootsector for legacy BIOS systems
 * COPYRIGHT:   Copyright 2002-2003 Brian Palmer <brianp@sginet.com>
 *              Copyright 2024-2026 Daniel Victor <ilauncherdeveloper@gmail.com>
 */

#include <asm.inc>

#define CS_REL(x) [bp + (x - start)]

#define GPT_HEADER_SEGREG               fs
#define GPT_HEADER_ADDRESS              HEX(1000)
#define GPT_PARTITION_ENTRY_ADDRESS     HEX(2000)

#define GPT_HEADER_SIGNATURE_OFFSET             HEX(00)
#define GPT_HEADER_PARTITION_ARRAY_LBA_OFFSET   HEX(48)
#define GPT_HEADER_PARTITION_ENTRIES_OFFSET     HEX(50)
#define GPT_HEADER_PARTITION_ENTRY_SIZE_OFFSET  HEX(54)

#define GPT_PARTITION_ENTRY_TYPE_GUID_OFFSET    HEX(00)
#define GPT_PARTITION_ENTRY_START_LBA_OFFSET    HEX(20)

#define DAP_SECTOR_COUNT_OFFSET                 HEX(02)
#define DAP_BUFFER_OFFSET                       HEX(04)
#define DAP_SEGMENT_OFFSET                      HEX(06)
#define DAP_LBA_OFFSET                          HEX(08)

#define BOOTSECTOR_BASE_ADDRESS         HEX(7C00)
#define NEW_BOOTSECTOR_BASE_ADDRESS     HEX(7E00)
#define BOOTSECTOR_SIZE                 512

#define SECTOR_SIZE                     512
#define SECTOR_SIZE_IN_SHIFT            9

.code16
start:
    jmp short Entry
    nop

// Fields that will be changed by the installer
BootDrive:
    .byte HEX(FF)
GptSectorSize:
    .long 512

Entry:
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax

    /* Intialize GPT_HEADER_SEGREG */
    mov ax, GPT_HEADER_ADDRESS / 16
    mov GPT_HEADER_SEGREG, ax

    /* Setup bp and sp which will also be used on relocation */
    mov bp, NEW_BOOTSECTOR_BASE_ADDRESS
    mov sp, BOOTSECTOR_BASE_ADDRESS

    /* Relocate this code using bp and sp to save bytes */
    mov di, bp
    mov si, sp
    mov cx, BOOTSECTOR_SIZE
    rep movsb

    /* Jump to the relocated code */
    ljmp16 NEW_BOOTSECTOR_BASE_ADDRESS / 16, (NewSegment - start)

GptPartitionEntryOffset:
    .word 0

DAP:
    .byte 16            // Size of DAP (16 bytes)
    .byte 0             // Unused
    .word 0             // Number of sectors to read
    .word 0             // Offset of buffer (e.g., 0000h)
    .word 0             // Segment of buffer (e.g., 0000h)
    .quad 0             // LBA address to read (e.g., LBA 1)

NewSegment:
    cmp byte ptr CS_REL(BootDrive), HEX(0ff) // If they have specified a boot drive then use it
    jne CheckInt13hExtensions

    mov byte ptr CS_REL(BootDrive), dl       // Save the boot drive

// Now check if this computer supports extended reads. This boot sector will not work without it.
CheckInt13hExtensions:
    mov ah, HEX(41)             // AH = 41h
    mov bx, HEX(55aa)           // BX = 55AAh
    int HEX(13)                 // IBM/MS INT 13 Extensions - INSTALLATION CHECK
    jc PrintDiskError           // CF set on error (extensions not supported)
    cmp  bx, HEX(aa55)          // BX = AA55h if installed
    jne PrintDiskError
    test cl, 1                  // si = API subset support bitmap
    jz PrintDiskError           // Bit 0, extended disk access functions (AH=42h-44h,47h,48h) supported

LoadGptHeader:
    /* Setup output buffer */
    mov bx, GPT_HEADER_SEGREG
    mov es, bx
    xor bx, bx

    /* Set EDX:EAX to sector 1 */
    xor edx, edx
    xor eax, eax
    inc al

    /* Read GPT Header */
    call ReadSector

    /* Check GPT Header signature */
    mov di, GPT_HEADER_SIGNATURE_OFFSET
    mov si, offset GptSignatureStart
    mov cx, offset GptSignatureEnd - GptSignatureStart
    repe cmpsb

    /* Fail if GPT Header signature doesn't match GptSignature buffer */
    jne PrintInvalidGptError

CheckGptPartitionEntries:
    /* Clear partition entry index */
    xor eax, eax

.Loop:
    /* If partition entry index reached the end then theres no bootable partition */
    cmp eax, dword ptr GPT_HEADER_SEGREG:[GPT_HEADER_PARTITION_ENTRIES_OFFSET]
    jnb PrintNoBootablePartition

    /* Self eplanatory */
    call GetGptPartition

    /* Check Partition Entry GUID */
    mov di, CS_REL(GptPartitionEntryOffset)
    add di, GPT_PARTITION_ENTRY_TYPE_GUID_OFFSET
    mov si, offset GptBootPartGuidStart
    mov cx, offset GptBootPartGuidEnd - GptBootPartGuidStart
    repe cmpsb

    /* Start booting if Partition Entry GUID match GptBootPartGuid buffer */
    je FoundActivePartition

    /* Advance the loop (eax++) */
    inc eax
    jmp .Loop

// EAX
GetGptPartition:
    pushad

    /* Convert the partition index to offset in bytes */
    xor edx, edx
    mov ebx, GPT_HEADER_SEGREG:[GPT_HEADER_PARTITION_ENTRY_SIZE_OFFSET]
    mul ebx

    /* Convert partition offset in bytes to sectors */
    mov ebx, CS_REL(GptSectorSize)
    div ebx

    /* Set the offset of which bytes partition entry will start at a sector */
    mov CS_REL(GptPartitionEntryOffset), dx

    /* Convert partition entry sector to absolute sector in disk */
    xor edx, edx
    add eax, GPT_HEADER_SEGREG:[GPT_HEADER_PARTITION_ARRAY_LBA_OFFSET]
    adc edx, 0

    /* Setup output buffer */
    mov bx, GPT_PARTITION_ENTRY_ADDRESS / 16
    mov es, bx
    xor bx, bx

    /* Read the partition entry sector */
    call ReadSector

    popad
    ret

FoundActivePartition:
    /* Setup partition entry offset at start_lba field */
    mov di, CS_REL(GptPartitionEntryOffset)
    add di, GPT_PARTITION_ENTRY_START_LBA_OFFSET

    /* Get 64 bits lba address */
    mov eax, es:[di]
    mov edx, es:[di + 4]

    /* Setup output buffer */
    mov bx, BOOTSECTOR_BASE_ADDRESS / 16
    mov es, bx
    xor bx, bx

    /* Read volume bootsector */
    call ReadSector

    /* Restore BootDrive and start volume bootsector */
    mov dl, CS_REL(BootDrive)
    ljmp16 0, BOOTSECTOR_BASE_ADDRESS

// Reads logical sector into ES:[BX]
// EAX has logical sector number to read
ReadSector:
    pushad

    mov ecx, CS_REL(GptSectorSize)
    shr ecx, SECTOR_SIZE_IN_SHIFT + 1

    shld edx, eax, cl
    shl eax, cl

    inc cl
    mov CS_REL((DAP + DAP_SECTOR_COUNT_OFFSET)), cl

    mov cx, es
    mov CS_REL((DAP + DAP_BUFFER_OFFSET)), bx
    mov CS_REL((DAP + DAP_SEGMENT_OFFSET)), cx
    mov CS_REL((DAP + DAP_LBA_OFFSET)), eax
    mov CS_REL((DAP + DAP_LBA_OFFSET + 4)), edx

    mov ah, HEX(42)             // Extended Read function
    mov dl, CS_REL(BootDrive)
    mov si, offset DAP          // DS:SI points to DAP
    int HEX(13)                 // Call BIOS
    jc PrintDiskError           // If carry set, handle error

    popad
    ret

PrintNoBootablePartition:
    mov si, offset msgNoBootablePartition
    call PutChars

    jmp Reboot

PrintInvalidGptError:
    mov si, offset msgInvalidGptHeader
    call PutChars

    jmp Reboot

// Displays a disk error message
// And reboots
PrintDiskError:
    mov si, offset msgDiskError         // Bad boot disk message
    call PutChars                       // Display it

Reboot:
    mov si, offset msgAnyKey            // Press any key message
    call PutChars                       // Display it
    xor ax, ax
    int HEX(16)                         // Wait for a keypress
    int HEX(19)                         // Reboot

PutChars:
    lodsb
    or   al, al
    jz   short Done
    mov  ah, HEX(0e)
    mov  bx, 7
    int  HEX(10)
    jmp  short PutChars
Done:
    ret

msgDiskError:
    .ascii "Disk error", CR, LF, NUL
msgAnyKey:
    .ascii "Press any key", CR, LF, NUL
msgInvalidGptHeader:
    .ascii "No GPT", CR, LF, NUL
msgNoBootablePartition:
    .ascii "No boot part", CR, LF, NUL

GptSignatureStart:
    .ascii "EFI PART"
GptSignatureEnd:

// EFI System Partition GUID
GptBootPartGuidStart:
    .long HEX(c12a7328)
    .word HEX(f81f)
    .word HEX(11d2)

    .byte HEX(ba), HEX(4b), HEX(00), HEX(a0), HEX(c9), HEX(3e), HEX(c9), HEX(3b)
GptBootPartGuidEnd:

.org 442
    .byte HEX(DE), HEX(AD), HEX(C0), HEX(DE)

.org 510
    .byte HEX(55), HEX(0aa)

.endcode16

END
