/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     EXTFS second stage loader
 * COPYRIGHT:   Copyright 2024-2025 Daniel Victor <ilauncherdeveloper@gmail.com>
 */

#define INCLUDED_ASM
#include "ext.S"

ExtLdrEntryPoint:
    // Clear segment registers
    xor eax, eax
    mov ds, ax
    mov es, ax

    // Read root directory Inode
    mov al, EXT_ROOT_INODE
    call ExtReadInode

    // Set file base address and search for freeldr
    mov dword ptr BP_REL(ExtFileAddress), FREELDR_BASE
    call ExtSetFileSegment

    // Swap ES with DS segment and clear SI
    call SwapESWithDS
    xor si, si

    // Set the directory entries size limit
    mov eax, EXT_INODE_DATA_SIZE_LIMIT

    // Get directory entries size
    mov edx, dword ptr BP_REL(ExtFileSize)

    // Set the buffer size with limit
    cmp edx, eax
    jbe .FLoop2
    mov edx, eax
.FLoop2:
    // Load directory Inode
    mov eax, dword ptr ds:[si]

    // If the buffer reached the end then abort the search
    cmp si, dx
    jae FreeLdrPrintFileNotFound

    // Save SI
    push si

    // If the Inode is zero then skip this entry
    test eax, eax
    jz .FSkip2

    mov di, offset FreeLdrFileName                                      // Load target filename address
    mov cx, offset FreeLdrFileNameEnd - FreeLdrFileName                 // Length of filename to compare
    cmp byte ptr ds:[si + EXT_DIRECTORY_ENTRY_NAME_LENGTH_OFFSET], cl   // Compare if both names have the same length
    jnz .FSkip2                                                         // Skip this entry if not
    add si, EXT_DIRECTORY_ENTRY_NAME_OFFSET                             // Move to filename in entry
    repe cmpsb                                                          // Compare filenames
    pop si                                                              // Restore SI
    jz LoadFreeLdr                                                      // Found matching file
    push si                                                             // Save SI

.FSkip2:
    // Restore SI
    pop si

    // Move to next directory entry
    add si, word ptr ds:[si + EXT_DIRECTORY_ENTRY_SIZE_OFFSET]

    jmp .FLoop2

ExtFileIndirectPointer:
    .long 0
ExtFileIndirectPointerDouble:
    .long 0

SwapESWithDS:
    // Swap ES and DS
    push es
    push ds
    pop es
    pop ds
    ret

ExtSetFileSegment:
    mov ebx, dword ptr BP_REL(ExtFileAddress)           // Get the EXT file address
    shr ebx, 4                                          // Shift four bits to the right to get segment
    jmp .ExtSkip
ExtSetInodeSegment:
    mov bx, EXT_INODE_ADDRESS / 16                      // Get the EXT inode address
    jmp .ExtSkip
ExtSetBlockSegment:
    mov bx, EXT_BLOCK_ADDRESS / 16                      // Get the EXT block address
    jmp .ExtSkip
ExtSetBlock2Segment:
    mov bx, EXT_BLOCK2_ADDRESS / 16                     // Get the EXT second block address
    jmp .ExtSkip
ExtSetBlock3Segment:
    mov bx, EXT_BLOCK3_ADDRESS / 16                     // Get the EXT third block address
    jmp .ExtSkip
ExtSetBlock4Segment:
    mov bx, EXT_BLOCK4_ADDRESS / 16                     // Get the EXT fourth block address
    jmp .ExtSkip
ExtSetBlock5Segment:
    mov bx, EXT_BLOCK5_ADDRESS / 16                     // Get the EXT fifth block address
    jmp .ExtSkip
ExtSetBlock6Segment:
    mov bx, EXT_BLOCK6_ADDRESS / 16                     // Get the EXT sixth block address
    jmp .ExtSkip
ExtSetBlock7Segment:
    mov bx, EXT_BLOCK7_ADDRESS / 16                     // Get the EXT seventh block address
.ExtSkip:
    mov es, bx                                          // Set ES
    xor bx, bx                                          // Clear BX
    ret

ExtUpdateFileSize:
    mov eax, dword ptr BP_REL(ExtBlockSizeInBytes)

ExtAdjustFileSize:
    // Update the file size
    sub dword ptr BP_REL(ExtFileSizeState), eax
    add dword ptr BP_REL(ExtFileAddress), eax
    ret

ExtInodeDetectExtentsFlag:
    push eax
    mov eax, es:[si + EXT_INODE_FLAGS_OFFSET]
    test eax, EXT_INODE_FLAG_EXTENTS
    pop eax
    ret

ExtFileReadBlocks:
    // If the block count is zero then do nothing
    test bx, bx
    jz .FRDone

    // Read the block
    call ExtFileReadBlock

    // Go to the next block and decrement the block count
    inc eax
    dec bx

    // Loop until all blocks are read
    jmp ExtFileReadBlocks
.FRDone:
    ret

ExtSafeReadBlock:
    pushad

    // If the block is zero then just clear the block
    test eax, eax
    jz .NullBlk

    // Read the block safely
    call ExtReadBlock
    jmp .RBDone
.NullBlk:
    // If the block is 0 then just clear the block
    mov di, bx
    xor ax, ax
    mov cx, word ptr BP_REL(ExtBlockSizeInBytes)
    rep stosb
.RBDone:
    popad
    ret

// Reads a block from the EXT file
// EAX has the block to read
// ExtFileAddress has the address to write to
ExtFileReadBlock:
    push es
    pushad

    // Set the EXT file segment
    call ExtSetFileSegment

    // Read the block safely
    call ExtSafeReadBlock

    // Update the file size
    call ExtUpdateFileSize

    // Exit
    popad
    pop es
    ret

ExtReadEntireFileDone:
    push eax                                        // Save EAX register
    mov eax, dword ptr BP_REL(ExtBlockSizeInBytes)  // Load block size in bytes into EAX
    cmp dword ptr BP_REL(ExtFileSizeState), eax     // Compare file size state with block size
    pop eax                                         // Restore EAX register
    ret                                             // Return from procedure

// Loops over the block pointers and reads the blocks
// until the file size state is zero
ExtReadEntireFileLoop:
    call ExtReadEntireFileDone
    jb .RDone

    mov eax, dword ptr es:[si]

    // Read the block safely
    call ExtFileReadBlock

    add si, EXT_POINTER_SIZE
    loop ExtReadEntireFileLoop
.RDone:
    ret

// Reads the double indirect block data
// This function is used to read the blocks that are not contiguous
// to the blocks that are pointed by the indirect block
ExtReadEntireFileIndirectDouble:
    push es
    pushad

    // Check if there is no more blocks to read
    call ExtReadEntireFileDone
    jb .RDone4

    // Reset block offset
    xor si, si

    // Load the double indirect block address
    mov eax, dword ptr BP_REL(ExtFileIndirectPointerDouble)
    call ExtSetBlock2Segment
    call ExtSafeReadBlock

    // Load the number of pointers per block
    mov ecx, dword ptr BP_REL(ExtPointersPerBlock)
.RLoop4:
    // Check if there is no more blocks to read
    call ExtReadEntireFileDone
    jb .RDone4

    // Get the block address
    mov eax, dword ptr es:[si]

    // Read the indirect block
    call ExtReadEntireFileIndirect

    // Increment block offset
    add si, EXT_POINTER_SIZE

    // Loop until all blocks are read
    loop .RLoop4
.RDone4:
    popad
    pop es
    ret

// Reads the indirect block data
ExtReadEntireFileIndirect:
    push es
    pushad

    // Check if there is no more blocks to read
    call ExtReadEntireFileDone
    jb .RDone3

    // Reset block offset
    xor si, si

    // Set the block segment
    call ExtSetBlockSegment

    // Read the block safely
    call ExtSafeReadBlock

    // Read the blocks
    mov ecx, dword ptr BP_REL(ExtPointersPerBlock)
    call ExtReadEntireFileLoop

.RDone3:
    popad
    pop es
    ret

// Reads the direct block data
ExtReadEntireFileDirect:
    push es
    pushad

    // If there is no more blocks to read then abort it
    call ExtReadEntireFileDone
    jb .RDone2

    // Move to block pointer in Inode
    add si, EXT_INODE_BLOCK_POINTER_OFFSET
    xor ecx, ecx
    mov cl, EXT_INODE_BLOCKS

    // Set the correct segment
    call ExtSetInodeSegment

    // Read the blocks
    call ExtReadEntireFileLoop

.RDone2:
    popad
    pop es
    ret

ExtExtentsLevelSegmentFunctionTable:
    .word ExtSetBlockSegment
    .word ExtSetBlock2Segment
    .word ExtSetBlock3Segment
    .word ExtSetBlock4Segment
    .word ExtSetBlock5Segment
    .word ExtSetBlock6Segment

ExtExtentsLevelFunctionTable:
    .word ExtReadEntireFileExtentsLevel0
    .word ExtReadEntireFileExtentsLevelX
    .word ExtReadEntireFileExtentsLevelX
    .word ExtReadEntireFileExtentsLevelX
    .word ExtReadEntireFileExtentsLevelX
    .word ExtReadEntireFileExtentsLevelX

ExtReadEntireFileExtentsLevelX:
    push es
    pushad

    // Load extent header entries
    mov cx, word ptr es:[si + EXT_EXTENT_HEADER_ENTRIES_OFFSET]
.ExtentsLoopX:
    // Check if there is no more blocks to read
    call ExtReadEntireFileDone
    jb .ExtentsDoneX

    // Go to next extent
    add si, EXT_EXTENT_SIZE

    // Save current state
    push es
    pushad

    // Read the extent block
    mov eax, dword ptr es:[si + EXT_EXTENT_INDEX_LEAF_OFFSET]

    // Set block segment and read block safely
    call ExtSetBlock7Segment
    call ExtSafeReadBlock

    // Reset SI for calculations
    xor si, si

    // Calculate function table offset
    mov bp, word ptr es:[si + EXT_EXTENT_HEADER_DEPTH_OFFSET]
    shl bp, 1

    // Call the segment and level functions
    call word ptr cs:ExtExtentsLevelSegmentFunctionTable[bp]
    call ExtSafeReadBlock
    call word ptr cs:ExtExtentsLevelFunctionTable[bp]

    // Restore state and continue loop
    popad
    pop es

    loop .ExtentsLoopX
.ExtentsDoneX:
    popad
    pop es
    ret

ExtReadEntireFileExtentsLevel0:
    push es
    pushad

    // Load extent header entries
    mov cx, word ptr es:[si + EXT_EXTENT_HEADER_ENTRIES_OFFSET]

.ExtentsLoop0:
    // Check if there is no more blocks to read
    call ExtReadEntireFileDone
    jb .ExtentsDone0

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

    push es
.SparseLoop:
    // Check if the sparse blocks reached the end
    test bx, bx
    jz .SparseDone

    xor eax, eax
    call ExtFileReadBlock

    // Decrement the block count
    dec bx
    jmp .SparseLoop
.SparseDone:
    pop es
    jmp .ExtentsSkip0

.NotSparse:
    // Read blocks from extent start
    mov eax, dword ptr es:[si + EXT_EXTENT_START_OFFSET]
    call ExtFileReadBlocks

.ExtentsSkip0:
    loop .ExtentsLoop0
.ExtentsDone0:
    popad
    pop es
    ret

ExtReadEntireFileExtents:
    push es
    pushad

    // Add the extent header offset
    add si, EXT_INODE_BLOCK_POINTER_OFFSET

    // Load extent header depth
    mov ax, word ptr es:[si + EXT_EXTENT_HEADER_DEPTH_OFFSET]

    // Check if the file has more than the maximum allowed levels
    cmp ax, EXT_EXTENT_MAX_LEVEL
    ja FreeLdrPrintFileBig

    // If the extent header depth is zero then this is a level 0 extent
    test ax, ax
    jz .Level0

    // Call the recursive function
.LevelX:
    call ExtReadEntireFileExtentsLevelX

    // Jump to the end of the function
    jmp .FEDone

    // Level 0 extent
.Level0:
    // Call the level 0 extent function
    call ExtReadEntireFileExtentsLevel0

    // End of the function
.FEDone:
    popad
    pop es
    ret

// Reads the entire file in EXTFS
ExtReadEntireFile:
    // Set the correct Inode offset
    mov si, word ptr BP_REL(ExtReadInodeIndexOffset)

    // Set file size
    mov ecx, dword ptr es:[si + EXT_INODE_SIZE_OFFSET]
    mov dword ptr BP_REL(ExtFileSize), ecx

    // If file size is zero then abort it
    test ecx, ecx
    jz FreeLdrPrintFileZero

    // Set rounded up file size
    add ecx, dword ptr BP_REL(ExtBlockSizeInBytes)
    dec ecx
    mov dword ptr BP_REL(ExtFileSizeState), ecx

    // Use extents method if necessary
    call ExtInodeDetectExtentsFlag
    jz .NoExtents

    // Call the extents method
    call ExtReadEntireFileExtents
    ret

.NoExtents:
    // Set Indirect pointer
    mov eax, dword ptr es:[si + (EXT_INODE_BLOCK_POINTER_OFFSET + (EXT_INODE_BLOCKS * EXT_POINTER_SIZE))]
    mov dword ptr BP_REL(ExtFileIndirectPointer), eax

    // Set Double indirect pointer
    mov eax, dword ptr es:[si + (EXT_INODE_BLOCK_POINTER_OFFSET + (EXT_INODE_BLOCKS * EXT_POINTER_SIZE) + EXT_POINTER_SIZE)]
    mov dword ptr BP_REL(ExtFileIndirectPointerDouble), eax

    call ExtReadEntireFileDirect                                // Read the direct blocks

    mov eax, dword ptr BP_REL(ExtFileIndirectPointer)           // Load the simple indirect pointer
    call ExtReadEntireFileIndirect                              // Read the simple indirect blocks

    call ExtReadEntireFileIndirectDouble                        // Read the double indirect blocks
    ret

LoadFreeLdr:
    // Swap ES and DS
    call SwapESWithDS

    // Show output loading status
    push si
    mov si, offset msgLoadingFreeLdr
    call PutChars
    pop si

    // Load Inode at directory entry
    mov eax, dword ptr es:[si]
    call ExtReadInode
    mov si, word ptr BP_REL(ExtReadInodeIndexOffset)

    // Get Inode type
    mov ax, word ptr es:[si]
    and ax, EXT_INODE_TYPE_MASK

    // If it's not a regular file then abort it
    cmp ax, EXT_INODE_TYPE_REGULAR
    jnz FreeLdrPrintRegFileError

    // Load Freeldr at FREELDR_BASE
    mov dword ptr BP_REL(ExtFileAddress), FREELDR_BASE
    call ExtReadEntireFile

    // Restore the boot drive and partition
    mov dl, byte ptr BP_REL(BootDrive)
    mov dh, byte ptr BP_REL(BootPartition)

    // Transfer execution to the bootloader
    ljmp16 FREELDR_BASE / 16, 0

FreeLdrPrintFileBig:
    // Make DS correct, display it and reboot
    call SwapESWithDS
    mov si, offset msgFreeLdrBig
    jmp DisplayItAndReboot

FreeLdrPrintFileZero:
    // Make DS correct, display it and reboot
    call SwapESWithDS
    mov si, offset msgFreeLdrZero
    jmp DisplayItAndReboot

FreeLdrPrintFileNotFound:
    // Make DS correct, display it and reboot
    call SwapESWithDS
    mov si, offset msgFreeLdr
    jmp DisplayItAndReboot

FreeLdrPrintRegFileError:
    // Make DS correct, display it and reboot
    call SwapESWithDS
    mov si, offset msgFreeLdrNotRegularFile     // FreeLdr not found message
    jmp DisplayItAndReboot

FreeLdrFileName:
    .ascii "freeldr.sys"
FreeLdrFileNameEnd:

msgFreeLdrZero:
    .ascii "freeldr.sys size is zero", CR, LF, NUL
msgFreeLdrBig:
    .ascii "freeldr.sys has many extent levels", CR, LF, NUL
msgFreeLdr:
    .ascii "freeldr.sys not found", CR, LF, NUL
msgFreeLdrNotRegularFile:
    .ascii "freeldr.sys is not a regular file", CR, LF, NUL
msgLoadingFreeLdr:
    .ascii "Loading FreeLoader...", CR, LF, NUL

.org (EXTLDR_BOOTSECTOR_SIZE - 2)
.word HEX(AA55)       // BootSector signature

.endcode16

END
