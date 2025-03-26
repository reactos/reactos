/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     EXTFS volume boot sector
 * COPYRIGHT:   Copyright 2002-2003 Brian Palmer <brianp@sginet.com>
 *              Copyright 2024-2025 Daniel Victor <ilauncherdeveloper@gmail.com>
 */

#include <asm.inc>
#include <freeldr/include/arch/pc/x86common.h>

// Boot sector constants
#define BOOTSECTOR_BASE_ADDRESS                 HEX(7C00)
#define EXTLDR_BOOTSECTOR_SIZE                  1024
#define EXT_POINTER_SIZE                        4
#define EXT_EXTENT_SIZE                         12

// Maximum extent values
#define EXT_EXTENT_MAX_LEVEL                    5
#define EXT_EXTENT_MAX_LENGTH                   32768

// Group descriptor offsets
#define EXT_GROUP_DESC_INODE_TABLE_OFFSET       8

// Extent offsets
#define EXT_EXTENT_HEADER_ENTRIES_OFFSET        2
#define EXT_EXTENT_HEADER_DEPTH_OFFSET          6
#define EXT_EXTENT_INDEX_LEAF_OFFSET            4
#define EXT_EXTENT_LENGTH_OFFSET                4
#define EXT_EXTENT_START_OFFSET                 8

// Inode offsets
#define EXT_INODE_SIZE_OFFSET                   4
#define EXT_INODE_FLAGS_OFFSET                  32
#define EXT_INODE_BLOCK_POINTER_OFFSET          40

// Directory entry offsets
#define EXT_DIRECTORY_ENTRY_SIZE_OFFSET         4
#define EXT_DIRECTORY_ENTRY_NAME_LENGTH_OFFSET  6
#define EXT_DIRECTORY_ENTRY_NAME_OFFSET         8

// Inode flags
#define EXT_INODE_FLAG_EXTENTS                  HEX(80000)

// Inode blocks constants
#define EXT_INODE_BLOCKS                        12
#define EXT_INODE_INDIRECT_BLOCKS               3

// Root Inode
#define EXT_ROOT_INODE                          2

// Inode address
#define EXT_INODE_ADDRESS                       HEX(9000)

// Data block addresses
#define EXT_BLOCK_ADDRESS                       HEX(1000)
#define EXT_BLOCK2_ADDRESS                      HEX(2000)
#define EXT_BLOCK3_ADDRESS                      HEX(3000)
#define EXT_BLOCK4_ADDRESS                      HEX(4000)
#define EXT_BLOCK5_ADDRESS                      HEX(5000)
#define EXT_BLOCK6_ADDRESS                      HEX(6000)
#define EXT_BLOCK7_ADDRESS                      HEX(A000)

// Inode types
#define EXT_INODE_TYPE_MASK                     HEX(F000)
#define EXT_INODE_TYPE_REGULAR                  HEX(8000)

// File size limit
#define EXT_INODE_DATA_SIZE_LIMIT               HEX(F000)

// Offset of functions addresses that will be used by the extldr.sys 3rd-stage bootsector
#define ExtReadBlockOffset                      2
#define ExtReadInodeOffset                      4
#define DisplayItAndRebootOffset                6
#define PutCharsOffset                          8

// Boot sector stack constants
#define BOOTSECTOR_STACK_TEMP_VARIABLES         2
#define BOOTSECTOR_STACK_TEMP_VARIABLES_SIZE    (4 * BOOTSECTOR_STACK_TEMP_VARIABLES)
#define BOOTSECTOR_STACK_OFFSET                 (8 + BOOTSECTOR_STACK_TEMP_VARIABLES_SIZE)
#define BOOTSECTOR_STACK_BASE                   (BOOTSECTOR_BASE_ADDRESS - BOOTSECTOR_STACK_OFFSET)
#define BP_REL(x)                               ss:[bp + (x - BOOTSECTOR_BASE_ADDRESS)]

// Temporary variables
#define ExtFileSizeState                        ((BOOTSECTOR_STACK_BASE + BOOTSECTOR_STACK_TEMP_VARIABLES_SIZE) - 4)
#define LBASectorsRead                          (ExtFileSizeState - 4)

.code16

#ifndef INCLUDED_ASM

start:
    jmp short main
    nop

// Fields that will be changed by the installer
BootDrive:
    .byte HEX(FF)
ExtVolumeStartSector:
    .long 263088                // Start sector of the ext2 volume
ExtBlockSize:
    .long 2                     // Block size in sectors
ExtBlockSizeInBytes:
    .long 1024                  // Block size in bytes
ExtPointersPerBlock:
    .long 256                   // Number of block pointers that can be contained in one block
ExtGroupDescSize:
    .long 32                    // Size of Group Descriptor
ExtFirstDataBlock:
    .long 2                     // First data block (2 for 1024-byte blocks, 1 for bigger sizes)
ExtInodeSize:
    .long 128                   // Size of Inode
ExtInodesPerGroup:
    .long 2048                  // Number of inodes per group

// File variables
ExtFileSize:
    .long 0                     // File size in bytes
ExtFileAddress:
    .long FREELDR_BASE          // File address
ExtFileAddressOld:
    .long FREELDR_BASE          // Old file address

// Inode variables
ExtReadInodeGroup:
    .long 0
ExtReadInodeIndex:
    .long 0
ExtReadInodeGroupBlock:
    .long 0
ExtReadInodeIndexBlock:
    .long 0
ExtReadInodeGroupOffset:
    .word 0
ExtReadInodeIndexOffset:
    .word 0

main:
    xor ax, ax                      // Setup segment registers
    mov ds, ax                      // Make DS correct
    mov es, ax                      // Make ES correct
    mov ss, ax                      // Make SS correct
    mov bp, BOOTSECTOR_BASE_ADDRESS
    mov sp, bp                      // Setup a stack
    sub sp, BOOTSECTOR_STACK_OFFSET

    // Save the function addresses so the helper code knows where to call them
    mov word ptr ss:[bp-ExtReadBlockOffset],            offset ExtReadBlock
    mov word ptr ss:[bp-ExtReadInodeOffset],            offset ExtReadInode
    mov word ptr ss:[bp-DisplayItAndRebootOffset],      offset DisplayItAndReboot
    mov word ptr ss:[bp-PutCharsOffset],                offset PutChars

    mov si, offset BootDrive
    cmp byte ptr [si], HEX(0ff) // If they have specified a boot drive then use it
    jne CheckInt13hExtensions

    mov byte ptr [si], dl       // Save the boot drive

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

LoadExtraBootCode:
    // First we have to load our extra boot code at
    // sector 1 into memory at [0000:7e00h]
    xor eax, eax
    inc eax
    mov cx, 1
    xor bx, bx
    mov es, bx                  // Read sector to [0000:7e00h]
    mov bx, HEX(7e00)
    call ReadSectors

    jmp LoadRootDirectory

// Reads logical sectors into ES:[BX]
// EAX has logical sector number to read
// CX has number of sectors to read
ReadSectors:
    push es
    add  eax, dword ptr BP_REL(ExtVolumeStartSector)    // Add the start of the volume
                                                        // If at all possible we want to use LBA routines because
                                                        // they are optimized to read more than 1 sector per read

ReadSectorsLBA:
    pushad                                      // Save logical sector number & sector count

    cmp  cx, 64                                 // Since the LBA calls only support 0x7F sectors at a time we will limit ourselves to 64
    jbe  ReadSectorsSetupDiskAddressPacket      // If we are reading less than 65 sectors then just do the read
    mov  cx, 64                                 // Otherwise read only 64 sectors on this loop iteration

ReadSectorsSetupDiskAddressPacket:
    movzx ecx, cx
    mov dword ptr BP_REL(LBASectorsRead), ecx
    data32 push 0
    push eax                                // Put 64-bit logical block address on stack
    push es                                 // Put transfer segment on stack
    push bx                                 // Put transfer offset on stack
    push cx                                 // Set transfer count
    push 16                                 // Set size of packet to 10h
    mov  si, sp                             // Setup disk address packet on stack

    mov  dl, byte ptr BP_REL(BootDrive)     // Drive number
    mov  ah, HEX(42)                        // Int 13h, AH = 42h - Extended Read
    int  HEX(13)                            // Call BIOS
    jc   PrintDiskError                     // If the read failed then abort

    add  sp, 16                             // Remove disk address packet from stack

    popad                                   // Restore sector count & logical sector number

    push bx
    mov  ebx, dword ptr BP_REL(LBASectorsRead)
    add  eax, ebx                           // Increment sector to read
    shl  ebx, 5
    mov  dx, es
    add  dx, bx                             // Setup read buffer for next sector
    mov  es, dx
    pop  bx

    sub  cx, word ptr BP_REL(LBASectorsRead)
    jnz  ReadSectorsLBA                     // Read next sector

    pop es
    ret

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

.PutCharsLoop:
    mov ah, HEX(0E)
    mov bx, 7
    int HEX(10)
PutChars:
    lodsb
    or al, al
    jnz .PutCharsLoop
    ret

SwapESWithDS:
    // Swap ES and DS
    push es
    push ds
    pop es
    pop ds
    ret

ExtReadGroupDescriptor:
    mov eax, dword ptr BP_REL(ExtReadInodeGroupBlock)   // Get Inode group block
    add eax, dword ptr BP_REL(ExtFirstDataBlock)        // Add the Group Descriptor offset
    call ExtSetInodeSegment

ExtReadBlock:
    xor edx, edx
    mov ecx, dword ptr BP_REL(ExtBlockSize)
    mul ecx
    jmp ReadSectors

// EAX
ExtCalculateBlock:
    xor edx, edx                                        // Clear EDX before division
    div dword ptr BP_REL(ExtBlockSizeInBytes)           // Inode /= ExtBlockSizeInBytes
    mov dword ptr ds:[bp + si], eax                     // Store the Inode block
    ret

// SI, DI
ExtCalculateOffset:
    add bx, bp                                          // Sum BX with BP for absolute address
    xor edx, edx                                        // Clear EDX before multiplication
    mov eax, dword ptr ds:[bp + si]                     // Get the Inode block
    mul dword ptr BP_REL(ExtBlockSizeInBytes)           // Inode *= ExtBlockSizeInBytes
    mov ecx, dword ptr ds:[bx]                          // Get the Inode
    sub ecx, eax                                        // Subtract the original Inode with rounded down Inode
    mov word ptr ds:[bp + di], cx                       // Store the rounded down Inode
    ret

ExtSetOldFileSegment:
    mov ebx, dword ptr BP_REL(ExtFileAddressOld)        // Get the EXT old file address
    jmp .ExtSegSkip
ExtSetFileSegment:
    mov ebx, dword ptr BP_REL(ExtFileAddress)           // Get the EXT file address
.ExtSegSkip:
    shr ebx, 4                                          // Shift four bits to the right to get segment
    jmp .ExtSkip
ExtSetInodeSegment:
    mov bx, EXT_INODE_ADDRESS / 16                      // Get the EXT inode address
.ExtSkip:
    mov es, bx                                          // Set ES
    xor bx, bx                                          // Clear BX
    ret

// Read the Inode in EAX register
ExtReadInode:
    xor edx, edx                                        // Clear EDX before division
    dec eax                                             // Inode--
    div dword ptr BP_REL(ExtInodesPerGroup)             // Inode /= ExtInodesPerGroup
    mov dword ptr BP_REL(ExtReadInodeGroup), eax        // Store the Inode group
    mov dword ptr BP_REL(ExtReadInodeIndex), edx        // Store the Inode index

    xor edx, edx                                        // Clear EDX before multiplication
    mul dword ptr BP_REL(ExtGroupDescSize)              // Inode group *= ExtGroupDescSize
    mov dword ptr BP_REL(ExtReadInodeGroup), eax        // Store the precalculated Inode group

    xor edx, edx                                        // Clear EDX before multiplication
    mov eax, dword ptr BP_REL(ExtReadInodeIndex)        // Get the read Inode index
    mul dword ptr BP_REL(ExtInodeSize)                  // Inode group *= ExtInodeSize
    mov dword ptr BP_REL(ExtReadInodeIndex), eax        // Store the Inode index

    // Calculate the Inode index block
    mov si, offset ExtReadInodeIndexBlock - start
    call ExtCalculateBlock

    // Calculate the Inode group block
    mov eax, dword ptr BP_REL(ExtReadInodeGroup)
    mov si, offset ExtReadInodeGroupBlock - start
    call ExtCalculateBlock

    // Calculate the Inode group offset
    mov bx, offset ExtReadInodeGroup - start
    mov si, offset ExtReadInodeGroupBlock - start
    mov di, offset ExtReadInodeGroupOffset - start
    call ExtCalculateOffset

    // Calculate the Inode index offset
    mov bx, offset ExtReadInodeIndex - start
    mov si, offset ExtReadInodeIndexBlock - start
    mov di, offset ExtReadInodeIndexOffset - start
    call ExtCalculateOffset

    // Read group descriptor
    call ExtReadGroupDescriptor

    // Set the offset address
    mov si, word ptr BP_REL(ExtReadInodeGroupOffset)

    // Get InodeTable field from the ExtGroupDescriptor structure
    mov eax, dword ptr es:[si + EXT_GROUP_DESC_INODE_TABLE_OFFSET]

    // Sum EAX with Inode index block
    add eax, dword ptr BP_REL(ExtReadInodeIndexBlock)

    jmp ExtReadBlock

msgDiskError:
    .ascii "Disk error", CR, LF, NUL
msgAnyKey:
    .ascii "Press any key", CR, LF, NUL

.org 509

BootPartition:
    .byte 0

.word HEX(AA55)       // BootSector signature

// End of bootsector
//
// Now starts the extra boot code that we will store
// at sector 1 on a EXT volume

LoadRootDirectory:
    mov al, EXT_ROOT_INODE          // Put the root directory inode number in AL
    movzx eax, al                   // Convert AL to EAX

    call ExtReadInode               // Read the inode
    call BasicReadFile              // Load the directory entries using basic function
    call SearchFile                 // Find the extended loader and run it

    jmp ExtLdrPrintFileNotFound     // If the extended loader wasn't found, display an error

ExtInodeDetectExtentsFlag:
    mov eax, es:[si + EXT_INODE_FLAGS_OFFSET]
    test eax, EXT_INODE_FLAG_EXTENTS
    ret

ExtUpdateFileSize:
    mov eax, dword ptr BP_REL(ExtBlockSizeInBytes)

ExtAdjustFileSize:
    // Update the file size
    sub dword ptr BP_REL(ExtFileSizeState), eax
    add dword ptr BP_REL(ExtFileAddress), eax
    ret

ExtReadFileDone:
    push eax
    mov eax, dword ptr BP_REL(ExtFileSizeState)
    cmp eax, dword ptr BP_REL(ExtBlockSizeInBytes)
    pop eax
    ret

ExtFileReadBlocks:
    push es
.FRLoop:
    // Check if there is no more blocks to read
    call ExtReadFileDone
    jb .FRDone

    // If the block count is zero then do nothing
    test bx, bx
    jz .FRDone

    // Read the block
    pushad
    call ExtSetFileSegment
    call ExtReadBlock
    popad

    // Update the file size
    call ExtUpdateFileSize

    // Go to the next block and decrement the block count
    inc eax
    dec bx

    // Loop until all blocks are read
    jmp .FRLoop
.FRDone:
    pop es
    ret

BasicReadFileExtents:
    // Add block pointer offset
    add si, EXT_INODE_BLOCK_POINTER_OFFSET

.DepthExtentsLoop:
    // Load extent header depth
    mov dx, word ptr es:[si + EXT_EXTENT_HEADER_DEPTH_OFFSET]

    // Check if depth is zero
    test dx, dx
    jz .DepthExtentsDone

    // Go to next extent
    add si, EXT_EXTENT_SIZE

    // Push all registers
    pushad

    // Read the extent block
    mov eax, dword ptr es:[si + EXT_EXTENT_INDEX_LEAF_OFFSET]
    call ExtSetInodeSegment
    call ExtReadBlock

    // Pop all registers
    popad

    // Reset SI
    xor si, si

    jmp .DepthExtentsLoop
.DepthExtentsDone:
    // Load extent header entries
    mov cx, word ptr es:[si + EXT_EXTENT_HEADER_ENTRIES_OFFSET]

.FinalExtentsLoop:
    // Check if there is no more blocks to read
    call ExtReadFileDone
    jb .FinalExtentsDone

    // Go to next extent
    add si, EXT_EXTENT_SIZE

    // Load extent length
    mov bx, word ptr es:[si + EXT_EXTENT_LENGTH_OFFSET]
    and ebx, HEX(FFFF)

    // Check if extent is sparse
    cmp bx, EXT_EXTENT_MAX_LENGTH
    jbe .NotSparse

    // Adjust sparse extent length
    sub bx, EXT_EXTENT_MAX_LENGTH

    // Adjust extent length to byte count
    // by multiplying extent length to block size
    xor edx, edx
    mov eax, dword ptr BP_REL(ExtBlockSizeInBytes)
    mul ebx

    // Adjust file size for sparse extent
    call ExtAdjustFileSize

    jmp .FinalExtentsSkip

.NotSparse:
    // Read blocks from extent start
    mov eax, dword ptr es:[si + EXT_EXTENT_START_OFFSET]
    call ExtFileReadBlocks

.FinalExtentsSkip:
    // Loop to process next extent
    loop .FinalExtentsLoop

.FinalExtentsDone:
    ret

BasicReadFile:
    push es
    pushad
    call ExtSetInodeSegment

    // Set the correct Inode offset
    mov si, word ptr BP_REL(ExtReadInodeIndexOffset)

    // Set the old file address
    mov eax, dword ptr BP_REL(ExtFileAddress)
    mov dword ptr BP_REL(ExtFileAddressOld), eax

    // Set the file size limit
    mov eax, EXT_INODE_DATA_SIZE_LIMIT

    // Load file size from Inode
    mov ebx, dword ptr es:[si + EXT_INODE_SIZE_OFFSET]

    // Compare and limit file size
    cmp ebx, eax
    jbe .BelowOrEqualSize
    mov ebx, eax
.BelowOrEqualSize:
    // Store the file size in the ExtFileSize variable
    mov dword ptr BP_REL(ExtFileSize), ebx

    // Set rounded up file size
    add ebx, dword ptr BP_REL(ExtBlockSizeInBytes)
    dec ebx
    mov dword ptr BP_REL(ExtFileSizeState), ebx

    // Don't use the extents method if theres no extents flag
    call ExtInodeDetectExtentsFlag
    jz .NoExtents

    // If this Inode use Extents mapping then use the extents method and skip the entire classic method
    call BasicReadFileExtents
    jmp .LDone

.NoExtents:
    // Set up for reading direct block addresses
    xor ecx, ecx
    mov cl, EXT_INODE_BLOCKS
    add si, EXT_INODE_BLOCK_POINTER_OFFSET
.LLoop:
    call ExtSetInodeSegment

    call ExtReadFileDone
    jb .LDone

    // Get the block address
    mov eax, dword ptr es:[si]

    // If the block address is zero, skip the block
    test eax, eax
    jz .LSkipBlock

    // Set the file segment
    call ExtSetFileSegment

    // Read the block
    call ExtReadBlock
.LSkipBlock:
    call ExtUpdateFileSize

    // Increment block
    add si, EXT_POINTER_SIZE

    // Loop until all blocks are loaded
    loop .LLoop
.LDone:
    popad
    pop es
    ret

SearchFile:
    call ExtSetOldFileSegment
    call SwapESWithDS

    xor si, si
    mov dx, word ptr BP_REL(ExtFileSize)
.FLoop:
    mov eax, dword ptr ds:[si]                                          // Load directory Inode

    cmp si, dx                                                          // End of buffer reached?
    jae .Done                                                           // Abort the search if yes

    // Save SI
    push si

    test eax, eax                                                       // Check if Inode is zero
    jz .Skip                                                            // Skip this entry if yes

    mov di, offset ExtLdrFileName                                       // Load target filename address
    mov cx, offset ExtLdrFileNameEnd - ExtLdrFileName                   // Length of filename to compare
    cmp byte ptr ds:[si + EXT_DIRECTORY_ENTRY_NAME_LENGTH_OFFSET], cl   // Compare if both names have the same length
    jnz .Skip                                                           // Skip this entry if not
    add si, EXT_DIRECTORY_ENTRY_NAME_OFFSET                             // Move to filename in entry
    repe cmpsb                                                          // Compare filenames
    pop si                                                              // Restore SI
    jz LoadExtLdr                                                       // Found matching file
    push si                                                             // Save SI

.Skip:
    // Restore SI
    pop si

    // Move to next directory entry and continue looping
    add si, word ptr ds:[si + EXT_DIRECTORY_ENTRY_SIZE_OFFSET]
    jmp .FLoop
.Done:
    ret

LoadExtLdr:
    // Swap ES and DS
    call SwapESWithDS

    push si                                                 // Save SI
    mov si, offset msgLoadingExtLdr                         // Point SI to a loading message
    call PutChars                                           // Show the message
    pop si                                                  // Restore SI

    mov eax, dword ptr es:[si]                              // Load directory Inode
    call ExtReadInode                                       // Read the inode
    mov si, word ptr BP_REL(ExtReadInodeIndexOffset)        // Set the correct offset

    // Get Inode type
    mov ax, word ptr es:[si]
    and ax, EXT_INODE_TYPE_MASK

    cmp ax, EXT_INODE_TYPE_REGULAR                          // Check if regular file
    jnz ExtLdrPrintRegFileError                             // If not, handle error

    call BasicReadFile                                      // Load the file using basic function
    call ExtSetOldFileSegment                               // Set old file segment
    call SwapESWithDS                                       // Swap ES with DS before copy

    // Copy the loaded file to 1KB ahead of this bootsector
    xor si, si
    mov di, offset ExtLdrEntryPoint
    mov cx, EXTLDR_BOOTSECTOR_SIZE
    rep movsb

    ljmp16 0, ExtLdrEntryPoint

ExtLdrPrintFileNotFound:
    // Make DS correct, display it and reboot
    call SwapESWithDS
    mov si, offset msgExtLdr
    jmp DisplayItAndReboot

ExtLdrPrintRegFileError:
    mov si, offset msgExtLdrNotRegularFile      // ExtLdr not found message
DisplayItAndReboot:
    call PutChars                               // Display it
    jmp Reboot

ExtLdrFileName:
    .ascii "extldr.sys"
ExtLdrFileNameEnd:

msgExtLdr:
    .ascii "extldr.sys not found", CR, LF, NUL
msgExtLdrNotRegularFile:
    .ascii "extldr.sys is not a regular file", CR, LF, NUL
msgLoadingExtLdr:
    .ascii "Loading ExtLoader...", CR, LF, NUL

.org 1022

.word HEX(AA55)       // BootSector signature

ExtLdrEntryPoint:
// ExtLdr is loaded here

.endcode16

END

#else

#define start BOOTSECTOR_BASE_ADDRESS

#define BootDrive                   (start + 3)
#define ExtVolumeStartSector        (BootDrive + 1)
#define ExtBlockSize                (ExtVolumeStartSector + 4)
#define ExtBlockSizeInBytes         (ExtBlockSize + 4)
#define ExtPointersPerBlock         (ExtBlockSizeInBytes + 4)
#define ExtGroupDescSize            (ExtPointersPerBlock + 4)
#define ExtFirstDataBlock           (ExtGroupDescSize + 4)
#define ExtInodeSize                (ExtFirstDataBlock + 4)
#define ExtInodesPerGroup           (ExtInodeSize + 4)

#define ExtFileSize                 (ExtInodesPerGroup + 4)
#define ExtFileAddress              (ExtFileSize + 4)
#define ExtFileAddressOld           (ExtFileAddress + 4)

#define ExtReadInodeGroup           (ExtFileAddressOld + 4)
#define ExtReadInodeIndex           (ExtReadInodeGroup + 4)
#define ExtReadInodeGroupBlock      (ExtReadInodeIndex + 4)
#define ExtReadInodeIndexBlock      (ExtReadInodeGroupBlock + 4)
#define ExtReadInodeGroupOffset     (ExtReadInodeIndexBlock + 4)
#define ExtReadInodeIndexOffset     (ExtReadInodeGroupOffset + 2)

#define BootPartition               (BootDrive + 506)

#define ExtReadBlock                word ptr ss:[bp-ExtReadBlockOffset]
#define ExtReadInode                word ptr ss:[bp-ExtReadInodeOffset]
#define DisplayItAndReboot          word ptr ss:[bp-DisplayItAndRebootOffset]
#define PutChars                    word ptr ss:[bp-PutCharsOffset]

#endif
