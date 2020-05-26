/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FAT12 file system boot sector for NEC PC-98 series
 * NOTES:       The source code in this file is based on the Brian Palmer's work
 *              (boot/freeldr/bootsect/fat.S)
 * COPYRIGHT:   Copyright 2019 Dmitry Borisov (di.sean@protonmail.com)
 */

#include <asm.inc>
#include <freeldr/include/arch/pc/x86common.h>

#define BP_REL(x) [bp + x - offset start]
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

/*
 * See https://www.webtech.co.jp/company/doc/undocumented_mem/memsys.txt
 * At segment 0000h
 */
#define BOOT_DAUA HEX(0584)

DataAreaStartHigh    = 2
DataAreaStartLow     = 4
BiosCHSDriveSizeHigh = 6
BiosCHSDriveSizeLow  = 8
BiosCHSDriveSize     = 8
ReadSectorsOffset    = 10
ReadClusterOffset    = 12
PutCharsOffset       = 14
BootSectorStackTop   = HEX(7C00) - 16

if 0
.macro DEBUG_STOP
    hlt
    jmp short $ - 1
.endm

.macro DEBUG_STEP
    push ax
    xor ah, ah
    int HEX(18)                                 // Wait for a keypress
    pop ax
.endm
endif

// org 7C00h

.code16

start:
    jmp main
    nop

// After running fatten tool it overwrites (BPB & EBPB)
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

/*
 * Real-mode entry point
 */
main:
    xor ax, ax                                  // Setup segment registers:
    mov ds, ax                                  // Make DS correct
    mov es, ax                                  // Make ES correct
    mov ss, ax                                  // Make SS correct
    mov bp, HEX(7C00)
    mov sp, BootSectorStackTop                  // Stack grows downwards from BootSectorStackTop

if 0                                            // It would have been nice to have had this check...
    mov ax, HEX(1000)                           // Detecting hardware

    /*
     * INSTALLATION CHECK interrupt
     * See http://www.ctyme.com/intr/rb-2293.htm
     */
    int HEX(1A)
    cmp ax, HEX(1000)
    je HardwareError                            // An IBM-compatible PC
endif

    /*
     * IBM PC here:       NEC PC-98 here:
     * ESI   = 000E:0000  ESI   = 0000:0000
     * EDI   = 0000:070C  EDI   = 0000:0000
     * EBP   = 0000:7C00  EBP   = 0000:7C00
     * ESP   = 0000:7BF0  ESP   = 0000:7BF0
     * CS:IP = 0000:7C4E  CS:IP = 1FE0:004E
    */
    mov ax, word ptr ds:[BOOT_DAUA]             // Get the Device Address/Unit Address (DA/UA)
    mov si, ax

    push si
    push cs
    pop ds
    xor si, si
    mov ax, HEX(0000)
    mov es, ax
    mov di, HEX(7C00)
    mov cx, 512
    rep movsw                                   // Move our 512 bytes boot sector to the [0000:7C00]
    pop si

    ljmp16 0, relocated                         // Jump into relocated code

relocated:
    xor ax, ax                                  // Clean-up segments
    mov es, ax
    mov ds, ax

    test byte ptr ds:[HEX(501)], HEX(08)        // High-resolution mode check
    jz VideoTestNormalMode
    mov ax, HEX(0E000)
    jmp short VideoTestDone
VideoTestNormalMode:
    mov ax, HEX(0A000)
VideoTestDone:
    mov word ptr BP_REL(VramSegment), ax

    mov ax, si
    mov byte ptr BP_REL(BootDrive), al          // Save the boot drive

    /*
     * Now we must find our way to the first sector of the root directory
     *
     * LBA = NumberOfFats * SectorsPerFat + HiddenSectors + ReservedSectors
     */
    xor ax, ax
    xor cx, cx
    mov al, byte ptr BP_REL(NumberOfFats)       // Number of fats
    mul word ptr BP_REL(SectorsPerFat)          // Times sectors per fat
    add ax, word ptr BP_REL(HiddenSectors)
    adc dx, word ptr BP_REL(HiddenSectors + 2)  // Add the number of hidden sectors
    add ax, word ptr BP_REL(ReservedSectors)    // Add the number of reserved sectors
    adc dx, cx                                  // Add carry bit
    mov word ptr [bp - DataAreaStartLow], ax    // Save the starting sector of the root directory
    mov word ptr [bp - DataAreaStartHigh], dx   // Save it in the first 4 bytes before the boot sector
    mov si, word ptr BP_REL(MaxRootEntries)     // Get number of root dir entries in SI
    pusha                                       // Save 32-bit logical start sector of root dir
    // DX:AX now has the number of the starting sector of the root directory

    /*
     * Now calculate the size of the root directory
     *
     * Root directory sectors = (MaxRootEntries * 32 + BytesPerSector - 1) / BytesPerSector
     */
    xor dx, dx
    mov ax, 32                                  // Size of dir entry
    mul si                                      // Times the number of entries
    mov bx, word ptr BP_REL(BytesPerSector)
    add ax, bx
    dec ax
    div bx                                      // Divided by the size of a sector
    // AX now has the number of root directory sectors

    add word ptr [bp - DataAreaStartLow], ax    // Add the number of sectors of the root directory to our other value
    adc word ptr [bp - DataAreaStartHigh], cx   // Now the first 4 bytes before the boot sector contain the starting sector of the data area
    popa

/*
 * Reads root directory into [0000:7E00] and finds 'FREELDR SYS'
 *
 * Call with:
 *
 * DX:AX - LBA of the starting sector of the root directory
 */
LoadRootDirSector:
    mov bx, HEX(7E0)                            // We will load the root directory sector
    mov es, bx                                  // Right after the boot sector in memory
    xor bx, bx                                  // We will load it to [0000:7E00]
    xor cx, cx                                  // Zero out CX
    inc cx                                      // Now increment it to 1, we are reading one sector
    xor di, di                                  // Zero out di
    push es                                     // Save ES because it will get incremented by 20h
    call ReadSectors                            // Read the first sector of the root directory
    pop es                                      // Restore ES (ES:DI = 7E0:0000)

SearchRootDirSector:
    cmp byte ptr es:[di], ch                    // If the first byte of the directory entry is zero then we have
    jz PrintFileNotFound                        // reached the end of the directory and FREELDR.SYS is not here so reboot
    pusha                                       // Save all registers
    mov cl, 11                                  // Put 11 in cl (length of filename in directory entry)
    mov si, offset filename                     // Put offset of filename string in DS:SI
    repe cmpsb                                  // Compare this directory entry against 'FREELDR SYS'
    popa                                        // Restore all the registers
    jz FoundFreeLoader                          // If we found it then jump
    dec si                                      // SI holds MaxRootEntries, subtract one
    jz PrintFileNotFound                        // If we are out of root dir entries then reboot
    add di, 32                                  // Increment DI by the size of a directory entry
    cmp di, HEX(0200)                           // Compare DI to 512 (DI has offset to next dir entry, make sure we haven't gone over one sector)
    jc SearchRootDirSector                      // If DI is less than 512 loop again
    jmp short LoadRootDirSector                 // Didn't find FREELDR.SYS in this directory sector, try again

FoundFreeLoader:
    /*
     * We found freeldr.sys on the disk
     * so we need to load the first 512 bytes of it to [0000:F800]
     * ES:DI has dir entry (ES:DI == 07E0:XXXX)
     */
    mov ax, word ptr es:[di + HEX(1A)]          // Get start cluster
    push ax                                     // Save start cluster
    push FREELDR_BASE / 16                      // Put load segment on the stack and load it
    pop es                                      // Into ES so that we load the cluster at [0000:F800]
    call ReadCluster                            // Read the cluster
    pop ax                                      // Restore start cluster of FreeLoader

    /*
     * Save the addresses of needed functions so
     * the helper code will know where to call them
     */
    mov word ptr [bp - ReadSectorsOffset], offset ReadSectors   // Save the address of ReadSectors
    mov word ptr [bp - ReadClusterOffset], offset ReadCluster   // Save the address of ReadCluster
    mov word ptr [bp - PutCharsOffset], offset PrintString      // Save the address of PrintString

    /*
     * Now AX has start cluster of FreeLoader and we
     * have loaded the helper code in the first 512 bytes
     * of FreeLoader to 0000:F800. Now transfer control
     * to the helper code. Skip the first three bytes
     * because they contain a jump instruction to skip
     * over the helper code in the FreeLoader image
     */
    ljmp16 0, FREELDR_BASE + 3

/*
 * Reads cluster number in AX into [ES:BX]
 *
 * Call with:
 *
 * AX - cluster number
 * ES:BX - buffer to read data into
 */
ReadCluster:
    /*
     * StartSector = ((Cluster - 2) * SectorsPerCluster) + ReservedSectors + HiddenSectors
     */
    dec ax                                      // Adjust start cluster by 2
    dec ax                                      // Because the data area starts on cluster 2
    xor ch, ch
    mov cl, byte ptr BP_REL(SectsPerCluster)
    mul cx                                      // Times sectors per cluster
    add ax, [bp - DataAreaStartLow]             // Add start of data area
    adc dx, [bp - DataAreaStartHigh]            // Now we have DX:AX with the logical start sector of FREELDR.SYS
    xor bx, bx                                  // We will load it to [ES:0000], ES loaded before function call

/*
 * Reads logical sectors into [ES:BX]
 *
 * Call with:
 *
 * DX:AX - logical sector number to read (LBA value)
 * ES:BX - buffer to read data into
 * CX - number of sectors to read
 */
ReadSectors:

    .ReadSectorsLoop:
        pusha

        /*
         * Converting LBA (Linear Block Address) into a format CHS (Cylinder:Head:Sector)
         *
         * C = (LBA / SPT) / HPC
         * H = (LBA / SPT) % HPC
         * S = (LBA % SPT) + 1
         */
        xchg ax, cx
        xchg ax, dx
        xor dx, dx
        div word ptr BP_REL(SectorsPerTrack)
        xchg ax, cx
        div word ptr BP_REL(SectorsPerTrack)    // Divide logical by SectorsPerTrack
        inc dx                                  // Sectors numbering starts at 1 not 0
        xchg cx, dx
        div word ptr BP_REL(NumberOfHeads)      // Number of heads

        mov dh, dl                              // DH - head number (0-1)
        mov dl, cl                              // DL - sector number (1-26)
        mov cl, al                              // CL - cylinder number (0-76)

        // TODO: This should be calculated using the equation: BytesPerSector = (CH + 1) * 128
        mov ch, 2                               // CH - sector size (0-4): 0 (128), 1 (256), 2 (512), 3 (1024), 4 (2048)

        mov al, byte ptr BP_REL(BootDrive)      // AL - DA/UA
        push bp
        push bx
        mov bx, word ptr BP_REL(BytesPerSector) // BX - bytes to read
        pop bp                                  // ES:BP - buffer to read data into
        mov ah, HEX(56)                         // AH - read sectors from a floppy disk with SEEK, and use double-density format (MFM)

        /*
         * Disk BIOS interrupt
         * See http://radioc.web.fc2.com/column/pc98bas/bios/int1b_06.htm
         */
        int HEX(1b)

        pop bp
        jc PrintDiskError                       // CF set on failure

        popa

        inc ax                                  // Increment sector to read
        jnz .NoCarryCHS
        inc dx

    .NoCarryCHS:
        push bx
        mov bx, es
        add bx, HEX(20)                         // Add size of dir entry to the buffer address for the next sector
        mov es, bx
        pop bx
    loop .ReadSectorsLoop                       // Increment read buffer for next sector, read next sector

    ret

/*
 * Prints a character
 *
 * Call with:
 *
 * AL - ASCII code
 */
PutChar:
    push di
    push es

    push word ptr BP_REL(VramSegment)
    pop es
    mov di, word ptr BP_REL(VramOffset)         // ES:DI = VramSegment:VramOffset
    .PutCharWrite:
        xor ah, ah
        stosw                                   // Write ASCII directly to the VRAM

        mov word ptr BP_REL(VramOffset), di
    pop es
    pop di

    ret

/*
 * Prints a null-terminated string
 *
 * Call with:
 *
 * DS:SI - pointer to a string
 */
PrintString:
    xor ah, ah
    lodsb                                       // Get a single char from a ptr

    or al, al
    jz short .PrintEnd                          // Found NULL

    cmp al, HEX(0D)
    jz short .PrintStringHandleCR               // Found CR

    call PutChar
    jmp short PrintString

    .PrintStringHandleCR:
        mov ax, word ptr BP_REL(VramOffset)
        mov dl, VGA_WIDTH * 2
        div dl
        inc ax
        mul dl
        mov word ptr BP_REL(VramOffset), ax
        inc si                                  // Skip the next LF character
     jmp short PrintString

.PrintEnd:
    ret

if 0
/*
 * Displays a hardware error message and reboots
 */
HardwareError:
    mov si, offset msgHardwareError

    .PrintStringVGA:
        lodsb                                   // Get a single char from a ptr

        or al, al
        jz short .HardwareErrorDone             // Found NULL

        mov ah, HEX(0E)                         // Teletype output
        mov bx, 7                               // BH - video page number, BL - foreground color
        int HEX(10)                             // Display a character via TTY mode
    jmp short .PrintStringVGA

.HardwareErrorDone:
    xor ax, ax
    int HEX(16)                                 // Wait for a keypress
    int HEX(19)                                 // Reboot
endif

/*
 * Displays a disk error message and reboots
 */
PrintDiskError:
    mov si, offset msgDiskError                 // Disk error message
    call PrintString                            // Display it

    jmp short Reboot

/*
 * Displays a file not found error message and reboots
 */
PrintFileNotFound:
    mov si, offset msgNotFoundError             // FreeLdr not found message
    call PrintString                            // Display it

    jmp short Reboot

/*
 * Reboots the computer after keypress
 */
Reboot:
    mov si, offset msgAnyKey                    // Press any key message
    call PrintString                            // Display it

    xor ax, ax
    int HEX(18)                                 // Wait for a keypress

    /*
     * Activate the CPU reset line
     * See https://people.freebsd.org/~kato/pc98-arch.html#cpureset
     * and http://www.webtech.co.jp/company/doc/undocumented_mem/io_cpu.txt
     */
    xor ax, ax
    out HEX(0F0), al

    hlt
Halt:
    jmp short Halt                              // Spin

VramSegment:
    .word 0
VramOffset:
    .word 0
msgDiskError:
    .ascii "ERR", CR, LF, NUL
msgNotFoundError:
    .ascii "NFE", CR, LF, NUL
msgAnyKey:
    .ascii "Press any key", NUL
filename:
    .ascii "FREELDR SYS"

if 0                                            // So totally out of space here...
msgHardwareError:
    .ascii "It's not PC-98", NUL
endif

    .org 509                                    // Pad to 509 bytes

BootPartition:
    .byte 0

BootSignature:
    .word HEX(0AA55)                            // BootSector signature

.endcode16

END
