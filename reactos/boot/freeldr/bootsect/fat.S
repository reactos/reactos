// FAT.ASM
// FAT12/16 Boot Sector
// Copyright (c) 1998, 2001, 2002 Brian Palmer



// This is a FAT12/16 file system boot sector
// that searches the entire root directory
// for the file freeldr.sys and loads it into
// memory.
//
// The stack is set to 0000:7BF2 so that the first
// WORD pushed will be placed at 0000:7BF0
//
// The DWORD at 0000:7BFC or BP-04h is the logical
// sector number of the start of the data area.
//
// The DWORD at 0000:7BF8 or BP-08h is the total
// sector count of the boot drive as reported by
// the computers bios.
//
// The WORD at 0000:7BF6 or BP-0ah is the offset
// of the ReadSectors function in the boot sector.
//
// The WORD at 0000:7BF4 or BP-0ch is the offset
// of the ReadCluster function in the boot sector.
//
// The WORD at 0000:7BF2 or BP-0eh is the offset
// of the PutChars function in the boot sector.
//
// When it locates freeldr.sys on the disk it will
// load the first sector of the file to 0000:8000
// With the help of this sector we should be able
// to load the entire file off the disk, no matter
// how fragmented it is.
//
// We load the entire FAT table into memory at
// 7000:0000. This improves the speed of floppy disk
// boots dramatically.

#include <asm.inc>
#include <freeldr/include/arch/pc/x86common.h>

#define BP_REL(x) [bp+x-offset start]

DataAreaStartHigh		=		2
DataAreaStartLow		=		4
BiosCHSDriveSizeHigh	=		6
BiosCHSDriveSizeLow		=		8
BiosCHSDriveSize		=		8
ReadSectorsOffset		=		10
ReadClusterOffset		=		12
PutCharsOffset			=		14
BootSectorStackTop		=		HEX(7c00) - 16


// org 7c00h

.code16

start:
    jmp main
    nop

OEMName:
    .ascii "FrLdr1.0"
BytesPerSector:
    .word 512
SectsPerCluster:
    .byte 1
ReservedSectors:
    .word 1
NumberOfFats:
    .byte 2
MaxRootEntries:
    .word 224
TotalSectors:
    .word 2880
MediaDescriptor:
    .byte HEX(0f0)
SectorsPerFat:
    .word 9
SectorsPerTrack:
    .word 18
NumberOfHeads:
    .word 2
HiddenSectors:
    .long 0
TotalSectorsBig:
    .long 0
BootDrive:
    .byte HEX(0ff)
Reserved:
    .byte 0
ExtendSig:
    .byte HEX(29)
SerialNumber:
    .long 00000000
VolumeLabel:
    .ascii "NO NAME    "
FileSystem:
    .ascii "FAT12   "

main:
    xor ax, ax
    mov ss, ax
    mov bp, HEX(7c00)
    mov sp, BootSectorStackTop				// Setup a stack
    mov ds, ax								// Make DS correct
    mov es, ax								// Make ES correct

    cmp byte ptr BP_REL(BootDrive), HEX(0ff) // If they have specified a boot drive then use it
    jne GetDriveParameters

    mov byte ptr BP_REL(BootDrive), dl    // Save the boot drive


GetDriveParameters:
    mov ah, 8
    mov dl, byte ptr BP_REL(BootDrive)    // Get boot drive in dl
    int HEX(13)                             // Request drive parameters from the bios
    jnc CalcDriveSize                       // If the call succeeded then calculate the drive size

    // If we get here then the call to the BIOS failed
    // so just set CHS equal to the maximum addressable
    // size
    mov cx, HEX(0ffff)
    mov dh, cl

CalcDriveSize:
    // Now that we have the drive geometry
    // lets calculate the drive size
    mov bl, ch              // Put the low 8-bits of the cylinder count into BL
    mov bh, cl              // Put the high 2-bits in BH
    shr bh, 6               // Shift them into position, now BX contains the cylinder count
    and cl, HEX(3f)         // Mask off cylinder bits from sector count
    // CL now contains sectors per track and DH contains head count
    movzx eax, dh           // Move the heads into EAX
    movzx ebx, bx           // Move the cylinders into EBX
    movzx ecx, cl           // Move the sectors per track into ECX
    inc eax                 // Make it one based because the bios returns it zero based
    inc ebx                 // Make the cylinder count one based also
    mul ecx                 // Multiply heads with the sectors per track, result in edx:eax
    mul ebx                 // Multiply the cylinders with (heads * sectors) [stored in edx:eax already]

    // We now have the total number of sectors as reported
    // by the bios in eax, so store it in our variable
    mov dword ptr [bp - BiosCHSDriveSize], eax


    // Now we must find our way to the first sector of the root directory
    xor ax, ax
    xor cx, cx
    mov al, byte ptr BP_REL(NumberOfFats)         // Number of fats
    mul word ptr BP_REL(SectorsPerFat)            // Times sectors per fat
    add ax, word ptr BP_REL(HiddenSectors)
    adc dx, word ptr BP_REL(HiddenSectors+2)    // Add the number of hidden sectors
    add ax, word ptr BP_REL(ReservedSectors)      // Add the number of reserved sectors
    adc dx, cx                                      // Add carry bit
    mov word ptr [bp - DataAreaStartLow], ax     // Save the starting sector of the root directory
    mov word ptr [bp - DataAreaStartHigh], dx    // Save it in the first 4 bytes before the boot sector
    mov si, word ptr BP_REL(MaxRootEntries)       // Get number of root dir entries in SI
    pusha                                           // Save 32-bit logical start sector of root dir
    // DX:AX now has the number of the starting sector of the root directory

    // Now calculate the size of the root directory
    xor dx, dx
    mov ax, 32                       // Size of dir entry
    mul si                                  // Times the number of entries
    mov bx, word ptr BP_REL(BytesPerSector)
    add ax, bx
    dec ax
    div bx                                  // Divided by the size of a sector
    // AX now has the number of root directory sectors

    add word ptr [bp - DataAreaStartLow], ax    // Add the number of sectors of the root directory to our other value
    adc word ptr [bp - DataAreaStartHigh], cx   // Now the first 4 bytes before the boot sector contain the starting sector of the data area
    popa                                        // Restore root dir logical sector start to DX:AX

LoadRootDirSector:
    mov bx, HEX(7e0)							// We will load the root directory sector
    mov es, bx								// Right after the boot sector in memory
    xor bx, bx								// We will load it to [0000:7e00h]
    xor cx, cx								// Zero out CX
    inc cx									// Now increment it to 1, we are reading one sector
    xor di, di								// Zero out di
    push es									// Save ES because it will get incremented by 20h
    call ReadSectors						// Read the first sector of the root directory
    pop es									// Restore ES (ES:DI = 07E0:0000)

SearchRootDirSector:
    cmp byte ptr es:[di], ch							// If the first byte of the directory entry is zero then we have
    jz ErrBoot							// reached the end of the directory and FREELDR.SYS is not here so reboot
    pusha									// Save all registers
    mov cl, 11								// Put 11 in cl (length of filename in directory entry)
    mov si, offset filename						// Put offset of filename string in DS:SI
    repe cmpsb								// Compare this directory entry against 'FREELDR SYS'
    popa									// Restore all the registers
    jz FoundFreeLoader					// If we found it then jump
    dec si									// SI holds MaxRootEntries, subtract one
    jz ErrBoot							// If we are out of root dir entries then reboot
    add di, 32						// Increment DI by the size of a directory entry
    cmp di, HEX(0200)							// Compare DI to 512 (DI has offset to next dir entry, make sure we haven't gone over one sector)
    jc SearchRootDirSector				// If DI is less than 512 loop again
    jmp short LoadRootDirSector				// Didn't find FREELDR.SYS in this directory sector, try again

FoundFreeLoader:
    // We found freeldr.sys on the disk
    // so we need to load the first 512
    // bytes of it to 0000:F800
    // ES:DI has dir entry (ES:DI == 07E0:XXXX)
    mov ax, word ptr es:[di + HEX(1a)]				// Get start cluster
    push ax									// Save start cluster
    push FREELDR_BASE / 16					// Put load segment on the stack and load it
    pop es									// Into ES so that we load the cluster at 0000:F800
    call ReadCluster						// Read the cluster
    pop ax									// Restore start cluster of FreeLoader

    // Save the addresses of needed functions so
    // the helper code will know where to call them.
    mov word ptr [bp-ReadSectorsOffset], offset ReadSectors		// Save the address of ReadSectors
    mov word ptr [bp-ReadClusterOffset], offset ReadCluster		// Save the address of ReadCluster
    mov word ptr [bp-PutCharsOffset], offset PutChars	// Save the address of PutChars

    // Now AX has start cluster of FreeLoader and we
    // have loaded the helper code in the first 512 bytes
    // of FreeLoader to 0000:F800. Now transfer control
    // to the helper code. Skip the first three bytes
    // because they contain a jump instruction to skip
    // over the helper code in the FreeLoader image.
    ljmp16 0, FREELDR_BASE + 3




// Displays an error message
// And reboots
ErrBoot:
    mov si, offset msgFreeLdr      // FreeLdr not found message
    call PutChars           // Display it

Reboot:
//    mov si, offset msgAnyKey       // Press any key message
//    call PutChars           // Display it
    xor ax, ax
    int HEX(16)                 // Wait for a keypress
    int HEX(19)                 // Reboot

PutChars:
    lodsb
    or al,al
    jz short Done
    mov ah, HEX(0e)
    mov bx, 7
    int HEX(10)
    jmp short PutChars
Done:
    ret

// Displays a bad boot message
// And reboots
BadBoot:
    mov si, offset msgDiskError    // Bad boot disk message
    call PutChars           // Display it

    jmp short Reboot


// Reads cluster number in AX into [ES:0000]
ReadCluster:
    // StartSector = ((Cluster - 2) * SectorsPerCluster) + ReservedSectors + HiddenSectors;
    dec ax								// Adjust start cluster by 2
    dec ax								// Because the data area starts on cluster 2
    xor ch, ch
    mov cl, byte ptr BP_REL(SectsPerCluster)
    mul cx								// Times sectors per cluster
    add ax, [bp-DataAreaStartLow]		// Add start of data area
    adc dx, [bp-DataAreaStartHigh]	// Now we have DX:AX with the logical start sector of OSLOADER.SYS
    xor bx, bx								// We will load it to [ES:0000], ES loaded before function call
    //mov   cl,BYTE [BYTE bp+SectsPerCluster]// Sectors per cluster still in CX
    //call  ReadSectors
    //ret



// Reads logical sectors into [ES:BX]
// DX:AX has logical sector number to read
// CX has number of sectors to read
ReadSectors:

    // We can't just check if the start sector is
    // in the BIOS CHS range. We have to check if
    // the start sector + length is in that range.
    pusha
    dec cx
    add ax, cx
    adc dx, 0

    cmp dx, word ptr [bp-BiosCHSDriveSizeHigh]	// Check if they are reading a sector within CHS range
    ja  ReadSectorsLBA							// No - go to the LBA routine
    jb  ReadSectorsCHS							// Yes - go to the old CHS routine
    cmp ax, word ptr [bp-BiosCHSDriveSizeLow]	// Check if they are reading a sector within CHS range
    jbe ReadSectorsCHS							// Yes - go to the old CHS routine

ReadSectorsLBA:
    popa
ReadSectorsLBALoop:
    pusha									// Save logical sector number & sector count

    push 0
    push 0
    push dx									// Put 64-bit logical
    push ax									// block address on stack
    push es									// Put transfer segment on stack
    push bx									// Put transfer offset on stack
    push 1  								// Set transfer count to 1 sector
    push HEX(10)							// Set size of packet to 10h
    mov  si,sp								// Setup disk address packet on stack

// We are so totally out of space here that I am forced to
// comment out this very beautifully written piece of code
// It would have been nice to have had this check...
//CheckInt13hExtensions:							// Now make sure this computer supports extended reads
//		mov  ah,0x41							// AH = 41h
//		mov  bx,0x55aa							// BX = 55AAh
//		mov  dl,[BYTE bp+BootDrive]				// DL = drive (80h-FFh)
//		int  13h								// IBM/MS INT 13 Extensions - INSTALLATION CHECK
//		jc   PrintDiskError						// CF set on error (extensions not supported)
//		cmp  bx,0xaa55							// BX = AA55h if installed
//		jne  PrintDiskError
//		test cl,1								// CX = API subset support bitmap
//		jz   PrintDiskError						// Bit 0, extended disk access functions (AH=42h-44h,47h,48h) supported


												// Good, we're here so the computer supports LBA disk access
												// So finish the extended read
    mov dl, byte ptr BP_REL(BootDrive)				// Drive number
    mov ah, HEX(42)								// Int 13h, AH = 42h - Extended Read
    int HEX(13)								// Call BIOS
    jc BadBoot							// If the read failed then abort

    add sp, 16						// Remove disk address packet from stack

    popa									// Restore sector count & logical sector number

    inc ax									// Increment Sector to Read
    adc dx, 0

    push bx
    mov bx, es
    add bx, HEX(20)						// Increment read buffer for next sector
    mov es, bx
    pop bx

    loop ReadSectorsLBALoop					// Read next sector

    ret


// Reads logical sectors into [ES:BX]
// DX:AX has logical sector number to read
// CX has number of sectors to read
// CarryFlag set on error
ReadSectorsCHS:
    popa
ReadSectorsCHSLoop:
    pusha
    xchg ax, cx
    xchg ax, dx
    xor dx, dx
    div word ptr BP_REL(SectorsPerTrack)
    xchg ax, cx
    div  word ptr BP_REL(SectorsPerTrack)    // Divide logical by SectorsPerTrack
    inc  dx                        // Sectors numbering starts at 1 not 0
    xchg cx, dx
    div  word ptr BP_REL(NumberOfHeads)      // Number of heads
    mov  dh, dl                     // Head to DH, drive to DL
    mov  dl, byte ptr BP_REL(BootDrive)            // Drive number
    mov  ch, al                     // Cylinder in CX
    ror  ah, 2                      // Low 8 bits of cylinder in CH, high 2 bits
                                       //  in CL shifted to bits 6 & 7
    or   cl, ah                     // Or with sector number
    mov  ax, HEX(0201)
    int  HEX(13)     // DISK - READ SECTORS INTO MEMORY
                     // AL = number of sectors to read, CH = track, CL = sector
                     // DH = head, DL    = drive, ES:BX -> buffer to fill
                     // Return: CF set on error, AH =    status (see AH=01h), AL    = number of sectors read

    jc   BadBoot

    popa
    inc ax       //Increment Sector to Read
    jnz NoCarryCHS
    inc dx


NoCarryCHS:
    push bx
    mov  bx, es
    add  bx, HEX(20)
    mov  es, bx
    pop  bx
                                        // Increment read buffer for next sector
    loop ReadSectorsCHSLoop         // Read next sector

    ret


msgDiskError:
    .ascii "Disk error", CR, LF, NUL
msgFreeLdr:
    .ascii "ldr not found", CR, LF, NUL
// Sorry, need the space...
//msgAnyKey:
//  .ascii "Press any key to restart", CR, LF, NUL
filename:
    .ascii "FREELDR SYS"

    .org 509    // Pad to 509 bytes

BootPartition:
    .byte 0

BootSignature:
    .word HEX(0aa55)   // BootSector signature

.endcode16

END
