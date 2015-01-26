/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Bootsector
 * FILE:            fatx.S
 * PURPOSE:         Combined FAT16 and FAT32 boot sector
 * PROGRAMMERS:     Brian Palmer
 *                  Timo Kreuzer
 */

#define DISKREADBUFFER HEX(8E000)

/*
 * Layout of a FAT volume:
 *
 * |---------------------------------------------------------
 * | * BootSector                         |
 * | * FS Information Sector (FAT32 only) | ReservedSectors
 * | * ... more reserved sectors ...      |
 * |--------------------------------------------------------
 * | * FAT 1                              | NumberOfFats
 * | * FAT 2                              |      *
 * | * [more FATs]                        | SectorsPerFat
 * |---------------------------------------------------------
 * | * Root Directory (FAT12/FAT16 only)  | MaxRootEntries / 16
 * |---------------------------------------------------------
 * | * File data                          |
 * | ....                                 |
 * |----------------------------------------
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <freeldr/include/arch/pc/x86common.h>

#define ADDRESS_FOR_DIRENTRIES HEX(10000)

SizeOfDataArea        = 32

/* Put the stack below the data area */
BootSectorStackTop    = (HEX(7c00) - SizeOfDataArea)

/* Data area offsets for uninitialized data */
DataAreaStart         = BootSectorStackTop + 0  /* dword */
#ifndef FAT32
RootDirStartSector    = BootSectorStackTop + 4  /* dword */
#endif
BiosCHSDriveSize      = BootSectorStackTop + 8  /* dword */
LBASectorsRead        = BootSectorStackTop + 12 /* dword */
ReadSectorsOffset     = BootSectorStackTop + 16 /* word */
ReadClusterOffset     = BootSectorStackTop + 18 /* word */
PutCharsOffset        = BootSectorStackTop + 20 /* word */

/* Macro for bp relative memory access to reduce code size */
#define BP_REL(x) ss:[bp + x - BootSectorStackTop]

/* The code starts at 0x7c00 */
// org 7c00h

.code16

/******************************************************************************
 *                      BIOS Parameter Block (BPB)                            *
 ******************************************************************************/
/* We have 3 bytes at the entry point to jump over the data area */
start:
    jmp short main
    nop

/* Here starts the BIOS Parameter Block (BPB) data.
   The real data will be copied during install */
OEMName:
    .ascii "FrLdr1.0"
BytesPerSector:
    .word 512
SectorsPerCluster:
    .byte 0
ReservedSectors:
    .word 32
NumberOfFats:
    .byte 2
MaxRootEntries:
    .word 0            // Always zero for FAT32 volumes
TotalSectors:
    .word 0            // Always zero for FAT32 volumes
MediaDescriptor:
    .byte HEX(0f8)
SectorsPerFat:
    .word 0            // Always zero for FAT32 volumes
SectorsPerTrack:
    .word 0
NumberOfHeads:
    .word 0
HiddenSectors:
    .long 0
TotalSectorsBig:
    .long 0

/* Extra data for FAT32 volumes */
#ifdef FAT32
SectorsPerFatBig:
    .long    0
ExtendedFlags:
    .word    0
FSVersion:
    .word    0
RootDirStartCluster:
    .long    0
FSInfoSector:
    .word    0
BackupBootSector:
    .word    6
Reserved1:
    .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#endif // FAT32

BootDrive:
    .byte 0
Reserved:
    .byte 0
ExtendSig:
    .byte HEX(29)
SerialNumber:
    .long 0
VolumeLabel:
    .ascii "NO NAME    "
FileSystem:
    .ascii "FATxx   "


/******************************************************************************
 *                             String data                                    *
 ******************************************************************************/

filename:
    .ascii "FREELDR SYS"

msgBootFailure:
    .ascii "Load failed!", CR, LF, NUL

msgAnyKey:
    .ascii "Press any key to reboot...", NUL


/******************************************************************************
 *                           Main code entry                                  *
 * Input: DL = Boot drive                                                     *
 ******************************************************************************/
main:
    /* First setup the segment registers */
    xor ax, ax
    mov ds, ax
    mov ss, ax

    /* Load the stack pointer */
    mov sp, BootSectorStackTop

    /* Load bp for relative memory access, which saves us some bytes of code
       size, when used with 32 bit instructions */
    mov bp, sp

    /* Load the boot drive from the BPB into al */
    mov al, byte ptr ds:[BootDrive]

    /* Check if it's valid */
    cmp al, HEX(0ff)
    je .SaveBootDrive

    /* Copy it into dl */
    mov dl, al

.SaveBootDrive:
    /* Save the bootdrive in the BPB */
    mov byte ptr ds:[BootDrive], dl


/******************************************************************************
 *                        Get drive parameters                                *
 ******************************************************************************/

    /* Call INT 13 to get the drive parameters:
       AH = 08h
       DL = drive (bit 7 set for hard disk)
       ES:DI = 0000h:0000h to guard against BIOS bugs */
    xor di, di
    mov ah, 8
    int HEX(13)

    /* Return from INT 13h/08h:
        CF set on error -> AH = status (07h)
        CF clear if successful -> AH = 00h
        AL = 00h on at least some BIOSes
        BL = drive type (AT/PS2 floppies only)
        CH = low eight bits of maximum cylinder number
        CL = bits 0:5 maximum sector number, bits 7:8 high two bits of maximum cylinder number
        DH = maximum head number
        DL = number of drives
        ES:DI -> drive parameter table (floppies only) */

    /* Check for failure */
    jc BootFailure


/******************************************************************************
 *                         Calculate drive size                               *
 ******************************************************************************/

    movzx ebx, ch           // Put the low 8-bits of the cylinder count into EBX
    mov bh, cl              // Put the high 2-bits in BH
    shr bh, 6               // Shift them into position, now BX contains the cylinder count

    and cl, HEX(3f)         // Mask off cylinder bits from sector count
    movzx ecx, cl           // Move the sectors per track into ECX

    movzx eax, dh           // Move the heads into EAX

    inc eax                 // Make it one based because the bios returns it zero based
    inc ebx                 // Make the cylinder count one based also
    mul ecx                 // Multiply heads with the sectors per track, result in edx:eax
    mul ebx                 // Multiply the cylinders with (heads * sectors) [stored in edx:eax already]

    // We now have the total number of sectors as reported
    // by the bios in eax, so store it in our variable
    mov dword ptr BP_REL(BiosCHSDriveSize), eax


/******************************************************************************
 *                             Load the FAT                                   *
 ******************************************************************************/

    /* Load the number of first sector of the FAT into eax */
    movzx eax, word ptr BP_REL(ReservedSectors)
    add eax, dword ptr BP_REL(HiddenSectors)

    /* Load sector count into ecx */
#if FAT32
    mov ecx, BP_REL(SectorsPerFatBig)
#else
    movzx ecx, word ptr BP_REL(SectorsPerFat)
#endif

    /* Save FAT sector and size for later use */
    pushad

    /* Point ES:DI to the memory that is later the disk read buffer for freeldr.
       This way we cannot overwrite our FAT with freeldr data */
    mov bx, DISKREADBUFFER / 16
    mov es,bx
    xor di, di

    /* Read the sectors */
    call ReadSectors

    /* Restore FAT sector and size */
    popad


/******************************************************************************
 *                 Get root directory / data area start                       *
 ******************************************************************************/

    /* Copy reserved + hidden sectors to EBX */
    mov ebx, eax

    /* Calculate (NumberOfFats * SectorsPerFat) */
    movzx eax, byte ptr BP_REL(NumberOfFats)
    mul ecx

    /* Add reserved sectors and hidden sectors */
    add eax, ebx

#ifndef FAT32
    /* Save the starting sector of the root directory */
    mov dword ptr BP_REL(RootDirStartSector), eax

    /* Calculate number of sectors for the root dir:
       sectors = MaxRootEntries * 32 / 512 (rounded up!) */
    movzx ebx, word ptr BP_REL(MaxRootEntries)
    add ebx, 15
    shr ebx, 4

    /* Add the root dir start sector and save it as DataAreaStart */
    add ebx, eax
    mov dword ptr BP_REL(DataAreaStart), ebx
#else
    mov dword ptr BP_REL(DataAreaStart), eax

    /* On FAT32 volumes the root dir start cluster is stored in the BPB */
    mov eax, dword ptr BP_REL(RootDirStartCluster)
#endif


/******************************************************************************
 *                 Search the root directory for freeldr                      *
 ******************************************************************************/
.SearchForFreeldr:

    /* Load ES with the segment where we put the dir entries */
    mov bx, ADDRESS_FOR_DIRENTRIES / 16
    mov es, bx

    /* Set the address offset to 0 */
    xor di, di

#ifdef FAT32
    /* Read the dir cluster. This loads the next cluster into EAX */
    call ReadCluster

    /* Calculate the numer of dir entries in this cluster:
       dx = SectorsPerCluster * 512 / 32 */
    movzx dx, byte ptr ds:[SectorsPerCluster]
    shl dx, 4
#else
    /* Set the number of sectors to read to 1 */
    xor cx, cx
    inc cx

    /* Read the sector, but preserve ES */
    push es
    call ReadSectors
    pop es

    /* Set entry count to entries per sector */
    mov dx, (512 / 32)
#endif

    /* Load the start offset of the dir entries into ebx */
    xor bx, bx

.CheckDirEntry:
    /* Load the address of the name into di */
    mov di, bx

    /* If the first byte of the entry is 0 then we have reached the end */
    cmp byte ptr es:[di], ch
    jz BootFailure

    /* Compare with freeldr file name */
    mov si, offset filename
    mov cx, 11
    repe cmpsb

    /* Check if we found the file */
    jz .FoundFreeLoader

    /* File didn't match, go to next entry */
    add bx, 32

    /* Decrement entry count and check if we reached the end */
    dec dx
    jnz .CheckDirEntry

#if FAT32
    /* Check to see if this was the last cluster in the chain */
    cmp eax, HEX(0ffffff8)
    jnb BootFailure
#endif

    /* Repeat the search process with the next sector / cluster.
       eax is already incremented in ReadSectors / ReadCluster */
    jmp .SearchForFreeldr


/******************************************************************************
 *                            Load freeldr                                    *
 ******************************************************************************/
.FoundFreeLoader:

    /* Load the cluster number of freeldr into eax */
#if FAT32
#error unsupported
#else
    movzx eax, word ptr es:[bx + HEX(1A)]
#endif

    /* Load es:di with the freeldr start address */
    mov dx, FREELDR_BASE / 16
    mov es, dx
    xor di, di

.LoadNextCluster:
    /* Load the cluster to the current address. EAX is adjusted to the next
       cluster and ES is adjusted for the next read */
    call ReadCluster

    /* Check if this is the last cluster in the chain */
#if FAT32
    cmp eax, HEX(0ffffff8)
#elif FAT12
    cmp ax, HEX(0ff8)
#else
    cmp ax, HEX(0fff8)
#endif
    jb .LoadNextCluster

    /* Load boot drive into DL, boot partition into DH */
    mov  dl, byte ptr ds:[BootDrive]
    mov  dh, byte ptr ds:[BootPartition]

    /* Now the complete freeldr imag is loaded.
       Jump to the realmode entry point. */
    ljmp16 0, FREELDR_BASE



BootFailure:
    mov  si, offset msgBootFailure
    call PutChars


Reboot:
    /* Output "Press any key to reboot" message */
    mov  si, offset msgAnyKey
    call PutChars

    /* Wait for a keypress */
    xor  ax, ax
    int  HEX(16)

    /* Reboot */
    int  HEX(19)


/******************************************************************************
 * PROCEDURE ReadCluster                                                      *
 * Input: EAX = Cluster number, ES:DI = Target                                *
 * Modifies: EAX (next cluster number), BX, DX (undefined)                    *
 ******************************************************************************/
ReadCluster:

    pushad

    // StartSector = ((Cluster - 2) * SectorsPerCluster) + SectorsForFat + ReservedSectors + HiddenSectors
    // StartSector = ((Cluster - 2) * SectorsPerCluster) + DataAreaStart

    /* Substract 2 */
    dec eax
    dec eax

    /* Multiply with SectorsPerCluster */
    movzx ecx, byte ptr BP_REL(SectorsPerCluster)
    mul ecx

    /* Add DataAreaStart */
    add eax, dword ptr BP_REL(DataAreaStart)

    /* Call ReadSectors. EAX = SectorNumber, ECX = SectorsPerCluster */
    call ReadSectors

    /* Restore the cluster number */
    popad

    /* Save ES */
    push es

#if FAT32
#error FAT23 not implemented
#elif FAT12
#error FAT12 not implemented
#else
    /* DX:AX = AX * 2 (since FAT16 entries are 2 bytes) */
    mov bx, 2
    mul bx

    /* Shift DX, so that it is the segment offset: DX = DX * (64K / 16) */
    shl dx, 12

    /* Put segment address of FAT into ES */
    add dx, DISKREADBUFFER / 16
    mov es, dx

    /* Put the FAT entry offset into EBX for indirect mov */
    mov bx, ax

    /* Put the content of the FAT entry into AX */
    mov ax, es:[bx]
#endif

    /* Restore ES and return */
    pop  es
    ret


/******************************************************************************
 * PROCEDURE ReadSectors                                                      *
 * Input: EAX = Sector start number, ECX = number of sectors, ES:DI = Target  *
 * Modifies: EAX (incremented by sector count), CX = 0, ES (incremented),     *
 *           EBX undefined                                                    *
 ******************************************************************************/
ReadSectors:
    /* We could possibly also implement CHS, but it's currently unimplemented */
//jmp $
ReadSectorsLBA:

    /* Copy number of sectors to ebx */
    movzx ebx, cx

    /* Since the LBA calls only support 0x7F sectors at a time,
       we will limit ourselves to 64 */
    cmp bx, 64
    jbe .ReadSectorsLBA2
    mov bx, 64

.ReadSectorsLBA2:

    /* Save logical sector number & sector count */
    pushad

    /* Setup the disk address packet on the stack */
    .byte HEX(66) // size overwrite prefix for next push
    push 0    // Put 64-bit logical block address (high part) on stack
    push eax  // Put 64-bit logical block address (low part) on stack
    push es   // Put transfer segment on stack
    push di   // Put transfer offset on stack
    push bx   // Set transfer count (for this round)
    push 16   // Set size of packet to 16

    /* Point si to the disk address packet on stack */
    mov si, sp

    /* Set the drive number */
    mov dl, byte ptr ds:[BootDrive]
//jmp $
    /* Call INT 13h, AH = 42h - Extended Read
       Input: ...
       Modifies: ... */
    mov ah, HEX(42)
    int HEX(13)
//jmp $
    /* Check for failure */
    jc BootFailure

    /* Remove disk address packet from stack */
    add sp, 16

    /* Adjust ES to point to the next sector */
    shl bx, 5
    mov ax, es
    add ax, bx
    mov es, ax

    /* Restore sector count & logical sector number */
    popad

    /* Adjust the sector number to the next sector we need to read
       by adding the number of sectors that we read */
    add eax, ebx

    /* Adjust remaining sectors */
    sub cx, bx
    jnz ReadSectorsLBA

    /* return */
    ret



/******************************************************************************
 * PROCEDURE PutChars                                                         *
 * Input: ESI = Points to string to be printed                                *
 * Modifies: AL, AH, SI                                                       *
 ******************************************************************************/
PutChars2:
    mov ah, HEX(0e)
    mov bx, 7
    int HEX(10)
PutChars:
    lodsb
    or al, al
    jnz short PutChars2
    ret


/******************************************************************************
 *               Padding and boot sector signature                            *
 ******************************************************************************/
    /* Pad to 509 bytes */
    .org 509

BootPartition:
    .byte 0

BootSignature:
    .word HEX(0aa55)   // BootSector signature

.endcode16

END
