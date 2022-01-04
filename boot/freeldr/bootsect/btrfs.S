/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     BTRFS volume boot sector for FreeLoader
 * COPYRIGHT:   Copyright 2018 Victor Perevertkin (victor@perevertkin.ru)
 */

#include <asm.inc>
#include <freeldr/include/arch/pc/x86common.h>

.code16

CHUNK_MAP_OFFSET = HEX(8200) // max - 256 chunks (chunk is 24 bytes)
DATA_BUFFER_SEGMENT = HEX(9a0) // here we will store FS structures read from disk

FIRST_SUPERBLOCK_OFFSET = HEX(80) //in sectors

ROOT_TREE_OBJECTID = 1
EXTENT_TREE_OBJECTID = 2
CHUNK_TREE_OBJECTID = 3
DEV_TREE_OBJECTID = 4
FS_TREE_OBJECTID = 5
FIRST_CHUNK_TREE_OBJECTID = 256

DIR_ITEM_KEY = 84
EXTENT_DATA_KEY = 108
ROOT_ITEM_KEY = 132
EXTENT_ITEM_KEY = 168
CHUNK_ITEM_KEY = 228

BTRFS_FT_REG_FILE = 1

KEY_SIZE = 17
MAX_CHUNK_ITEMS = 255

start:
    jmp short main
    nop
// globals
ChunkMapSize:
    .byte 0
BootDrive:
    .byte 0
PartitionStartLBA:
    .quad HEX(3f) // default value. Setup tool have to overwrite it upon installation

TreeRootAddress:
    .quad 0
ChunkRootAddress:
    .quad 0
FsRootAddress:
    .quad 0

NodeSize:
    .long 0
LeafSize:
    .long 0
StripeSize:
    .long 0
SysChunkSize:
    .long 0

TreeRootLevel:
    .byte 0
ChunkRootLevel:
    .byte 0
FsRootLevel:
    .byte 0

main:
    xor eax, eax             // Setup segment registers
    mov ds, ax               // Make DS correct
    mov es, ax               // Make ES correct
    mov ss, ax               // Make SS correct
    mov sp, HEX(7c00)        // Setup a stack
    mov bp, sp

    mov byte ptr [BootDrive], dl

// Now check if this computer supports extended reads. This boot sector will not work without it
CheckInt13hExtensions:
    mov  ah, HEX(41)                        // AH = 41h
    mov  bx, HEX(55aa)                      // BX = 55AAh
    int  HEX(13)                            // IBM/MS INT 13 Extensions - INSTALLATION CHECK
    jc   PrintDiskError                     // CF set on error (extensions not supported)
    cmp  bx, HEX(aa55)                      // BX = AA55h if installed
    jne  PrintDiskError
    test cl, 1                              // si = API subset support bitmap
    jz   PrintDiskError                     // Bit 0, extended disk access functions (AH=42h-44h,47h,48h) supported

LoadExtraBootCode:
    // First we have to load our extra boot code into into memory at [0000:7e00h]
    xor edx, edx
    mov eax, 1 // read from the second sector - the first was already read
    mov  cx, 2
    mov  bx, HEX(7e00) // Read sector to [0000:7e00h]
    call ReadSectors

    mov ax, DATA_BUFFER_SEGMENT
    mov es, ax

// reading superblock
    xor edx, edx
    mov eax, FIRST_SUPERBLOCK_OFFSET
    mov cx, 8 // superblock is 0x1000 bytes - 8 sectors
    xor bx, bx
    call ReadSectors

    push es // swapping segments for memory operations with superblock
    push ds // ds must be DATA_BUFFER_SEGMENT
    pop es
    pop ds

    mov si, HEX(40)
    lea di, [btrfsSignature]
    mov cx, 4 // magic string size (words)
    repe cmpsw
    je SignatureOk

    mov si, wrongSignatureError
    call PrintCustomError

SignatureOk:
// signature is ok - reading superblock data
    add si, 8 // logical address of the root tree root
    lea di, [TreeRootAddress]
    mov cx, 8 // read both root tree root and chunk tree root
    rep movsw
// read sizes
    add si, HEX(34) // now pointing to nodesize field
    lea di, [NodeSize] // read all 4 values
    mov cx, 8
    rep movsw
// read both levels
    add si, 34
    movsw // di is on the right place

    push es
    push ds
    pop es
    pop ds

// read sys chunk array
    mov ax, HEX(32b) // sys_chunk_start
ReadSysChunk:
    mov bx, ax
    add bx, KEY_SIZE
    push bx // start of the chunk
    call InsertChunk

    pop si
    mov bx, word ptr es:[si+44] // number of stripes
    shl bx, 5 // one stripe entry is 32 bytes
    lea si, [si+bx+48] // 48 - size of the chunk
    mov ax, si // save for next iteration
    sub si, HEX(32b)
    cmp si, word ptr [SysChunkSize] // the size cannot be more than 0xFFFF here so use word ptr
    jb ReadSysChunk

    jmp SetTreeRoots


// Reads logical sectors from disk
// INPUT:
// - ES:[BX] address at which the data will be stored
// - EDX:EAX logical sector number from which to read
// - CX number of sectors to read
// OUTPUT:
// - 512*CX bytes of memory at ES:[BX]
// LOCALS:
// - [bp-2] - LBASectorsRead
ReadSectors:
    push bp
    mov bp, sp
    sub sp, 2
    push es // we will spoil es register here

    add eax, dword ptr [PartitionStartLBA]
    adc edx, dword ptr [PartitionStartLBA+4]
ReadSectors_loop:
    pushad                                  // Save logical sector number & sector count

    cmp  cx, 8                              // Maximum sectors per call is 0x7F, but VirtualBox correctly works with only 8
    jbe  ReadSectorsSetupDiskAddressPacket  // If we are reading less than 9 sectors then just do the read
    mov  cx, 8                              // Otherwise read only 8 sectors on this loop iteration

ReadSectorsSetupDiskAddressPacket:
    mov  word ptr [bp-2], cx
    push edx
    push eax                                // Put 64-bit logical block address on stack
    push es                                 // Put transfer segment on stack
    push bx                                 // Put transfer offset on stack
    push cx                                 // Set transfer count
    push 16                                 // Set size of packet to 10h
    mov  si, sp                              // Setup disk address packet on stack

    mov  dl, byte ptr [BootDrive]                    // Drive number
    mov  ah, HEX(42)                                // Int 13h, AH = 42h - Extended Read
    int  HEX(13)                                // Call BIOS
    jc   PrintDiskError                     // If the read failed then abort

    add  sp, 16                             // Remove disk address packet from stack

    popad                                   // Restore sector count & logical sector number

    push bx
    movzx ebx, word ptr [bp-2]
    add  eax, ebx                            // Increment sector to read
    adc  edx, 0
    shl  ebx, 5                              // Multiplying by 512=2^9 here.
                                             // Shifting only by 5, because it goes to segment
                                             // (segment will be shifter by another 4 when converted to real addr)
    mov  si, es
    add  si, bx                              // Setup read buffer for next sector
    mov  es, si
    pop  bx

    sub  cx, word ptr [bp-2]
    jnz  ReadSectors_loop                        // Read next sector

    pop es
    leave
    ret

// Displays a disk error message
// And reboots
PrintDiskError:
    mov  si, msgDiskError            // Bad boot disk message
PrintCustomError:
    call PutChars                   // Display it

Reboot:
    lea  si, [msgAnyKey]             // Press any key message
    call PutChars                   // Display it
    xor ax,ax
    int HEX(16)                         // Wait for a keypress
    int HEX(19)                         // Reboot

PutChars:
    lodsb
    or al,al
    jz short Done
    call PutCharsCallBios
    jmp short PutChars
PutCharsCallBios:
    mov ah, HEX(0e)
    mov bx, HEX(07)
    int HEX(10)
    ret
Done:
    mov al, HEX(0d)
    call PutCharsCallBios
    mov al, HEX(0a)
    call PutCharsCallBios
    ret


msgDiskError:
    .asciz "Disk error"
msgAnyKey:
    .asciz "Press any key to restart"

.org 510
    .word HEX(aa55)       // BootSector signature

SetTreeRoots:
    // converting sizes to sectors. We dont need this sizes in bytes
    shr dword ptr [NodeSize], 9
    shr dword ptr [LeafSize], 9 // leafsize is deprecated

    // finding FS_TREE root

    // put the key on stack
    xor eax, eax
    push eax
    push eax
    dec sp
    mov byte ptr [esp], ROOT_ITEM_KEY
    push eax
    push 0
    push FS_TREE_OBJECTID

    mov eax, dword ptr [TreeRootAddress]
    mov edx, dword ptr [TreeRootAddress+4]
    mov cl, byte ptr [TreeRootLevel]

    call SearchTree
    add sp, 17 // setting stack back

    // bx - pointer to ROOT_ITEM
    mov al, byte ptr es:[bx+238] // moving level
    mov byte ptr [FsRootLevel], al
    cld

    mov eax, dword ptr es:[bx+176]
    mov dword ptr [FsRootAddress], eax
    mov eax, dword ptr es:[bx+176+4]
    mov dword ptr [FsRootAddress+4], eax

    // now we need to find DIR_ITEM_KEY with freeldr.sys hash
    xor eax, eax
    push eax
    push dword ptr [filenameCrc]
    dec sp
    mov byte ptr [esp], DIR_ITEM_KEY
    push eax
    push 0
    push 256 // root dir objectid

    mov eax, dword ptr [FsRootAddress]
    mov edx, dword ptr [FsRootAddress+4]
    mov cl, byte ptr [FsRootLevel]

    call SearchTree
    add sp, 17 // setting stack back

    // parsing DIR_ITEM
    // bx - item addr
    // cx - item length
    mov ax, cx

    push ds
    push es
    pop ds
    pop es
ParseDirItem_loop:
    cmp byte ptr [bx+29], BTRFS_FT_REG_FILE // checking dir_item type
    jne ParseDirItem_loop_2
    cmp word ptr [bx+27], 11 // checking name length
    jne ParseDirItem_loop_2
    lea si, [bx+30]
    lea di, [freeldrFilename]
    mov cx, 11
    repe cmpsb
    je FreeLdrFound

ParseDirItem_loop_2:
    mov dx, 30
    add dx, word ptr [bx+27]
    cmp dx, ax
    jae FreeLdrNotFound
    add bx, dx
    jmp ParseDirItem_loop

FreeLdrNotFound:
    lea si, [notFoundError]
    call PrintCustomError

FreeLdrFound:
    // freeldr objectid is the first qword of DIR_ITEM structure
    xor eax, eax
    push eax
    push eax
    dec sp
    mov byte ptr [esp], EXTENT_DATA_KEY
    push dword ptr [bx+4]
    push dword ptr [bx]

    push es
    pop ds

    mov eax, dword ptr [FsRootAddress]
    mov edx, dword ptr [FsRootAddress+4]
    mov cl, byte ptr [FsRootLevel]

    call SearchTree
    add sp, 17

    // here we have an EXTENT_ITEM with freeldr.sys
    mov eax, dword ptr es:[bx+29]
    shr eax, 9 // getting the number of clusters
    mov cx, ax
    push cx

    lea di, [bx+21]
    call ConvertAddress
    pop cx

    xor bx, bx
    mov ds, bx
    mov es, bx
    mov bx, FREELDR_BASE
    call ReadSectors

    mov  dl, byte ptr [BootDrive]     // Load boot drive into DL
    //mov  dh, 0    // Load boot partition into DH (not needed, FreeLbr detects it itself)

    /* Transfer execution to the bootloader */
    ljmp16 0, FREELDR_BASE


// Insert chunk into chunk map (located at DS:[CHUNK_MAP_OFFSET])
// INPUT:
// - ES:[AX] chunk key address
// - ES:[BX] chunk item address
// TODO: add max items checking
InsertChunk:
    push bx
    push ax
    push es

    xor ecx, ecx // index
InsertChunk_loop:
    std  // numbers are little-engian, going right-to-left
    mov si, CHUNK_MAP_OFFSET
    lea si, [esi+ecx*8]
    lea si, [esi+ecx*8]
    lea si, [esi+ecx*8]   // shift by 24 bytes

    inc cx
    cmp cl, byte ptr [ChunkMapSize]
    ja InsertChunk_insert

    lea si, [si+4]      // set to the high dword of the 8-byte number
    lea di, [eax+KEY_SIZE-4] // set to high dword of key's offset field (offset=logical addr)

    cmpsd
    jb InsertChunk_loop
    cmpsd
    jb InsertChunk_loop
    lea si, [si+4] // set back to the beginning of key

    // numbers are greater - need to insert Here
    // shifting all to right by one element
InsertChunk_insert:
    push si // here we will store new chunk
    dec cx // because we increased it before comparison

    mov dx, ds
    mov es, dx

    movzx eax, byte ptr [ChunkMapSize] // number of elements

    mov si, CHUNK_MAP_OFFSET
    lea si, [esi+eax*8]
    lea si, [esi+eax*8]
    lea si, [esi+eax*8-4] // setting to the high dword of the last element

    mov di, si
    add di, 20 // last byte of the last + 1 element (-4 bytes)

    sub ax, cx
    mov bx, 6
    mul bx // 24/4 because of dwords
    mov cx, ax // number of elements to shift
    rep movsd

    // finally we can write the element
    cld
    pop di // here we will store new chunk

    pop ds // key-to-insert address segment
    pop si // key-to-insert address
    add si, 9 // move to 'offset' field
    movsd
    movsd // logical address loaded!

    // time for stripes
    pop si // chunk item address, length is the first field
    movsd
    movsd

    add si, 48 // move to offset of the first stripe (we read only first one)
    movsd
    movsd

    push es // swapping segments back to original
    push ds
    pop es
    pop ds

    inc byte ptr [ChunkMapSize]
    ret


// Searches a key in a BTRFS tree
// INPUT:
// - [bp+4] BTRFS key to find
// - EDX:EAX logical address of root header
// - CL leaf node level
// OUTPUT:
// - ES:[SI] pointer to found item's key
// - ES:[BX] pointer to found item
// - ECX item length
// LOCALS:
// - [bp-8] - block number/current node offset
// - [bp-9] - current level
SearchTree:
    push bp
    mov bp, sp
    sub sp, 9 // set stack frame

    mov dword ptr [bp-4], edx
    mov dword ptr [bp-8], eax
    mov byte ptr [bp-9], cl

SearchTree_readHeader:
    push ss
    pop es

    lea di, [bp-8]
    call ConvertAddress
    // LBA is in edx:eax now
    mov bx, DATA_BUFFER_SEGMENT
    mov es, bx
    xor bx, bx

    mov cl, byte ptr [bp-9]
    test cl, cl
    jz SearchTree_readLeaf
// read node
    mov cx, word ptr [NodeSize] // word btr because we cannot read more than 65536 bytes yet
    call ReadSectors // reading node to the memory

    // every node begins with header
    //mov ax, word ptr [DATA_BUFFER_OFFSET + HEX(60)] // number of items in this leaf
    mov cx, -1 // index
    // finding the key
SearchTree_findLoop_1:
    inc cx
    cmp word ptr es:[HEX(60)], cx
    je SearchTree_findLoop_1_equal // we are at the end - taking the last element

    lea si, [bp+4] // key to find
    mov di, HEX(65) // first key in leaf
    mov ax, 33
    call CompareKeys
    jb SearchTree_findLoop_1
    je SearchTree_findLoop_1_equal
    sub di, 33 // setting to previous element

    // we are here if key is equal or greater
    // (si points to the start of the key)
SearchTree_findLoop_1_equal:
    push ds
    push es
    pop ds
    pop es

    lea si, [di+17]
    lea di, [bp-8]
    movsd
    movsd

    push es
    pop ds

    dec byte ptr [bp-9] // decrement level
    jmp SearchTree_readHeader

SearchTree_readLeaf:
    mov cx, word ptr [LeafSize]
    call ReadSectors

    // every node begins with header
    //mov ax, word ptr [DATA_BUFFER_OFFSET + HEX(60)] // number of items in this leaf
    mov cx, -1 // index
    // finding the key
SearchTree_findLoop_2:
    inc cx
    cmp word ptr es:[HEX(60)], cx
    je SearchTree_foundEqual

    lea si, [bp+4] // key to find
    mov di, HEX(65) // first key in leaf
    mov ax, 25
    call CompareKeys
    jb SearchTree_findLoop_2
    je SearchTree_foundEqual

    // set pointer to previous element if greater
    sub di, 25

SearchTree_foundEqual:
    // found equal or greater key
    mov bx, word ptr es:[di+17] // data offset relative to end of header
    mov cx, word ptr es:[di+21] // data size
    add bx, HEX(65) // end of header
    mov si, di

    leave
    ret

SearchTree_notFound:
    xor ecx, ecx // return ecx=0 if nothing found

    leave
    ret


// Converts logical address into physical LBA addr using chunk map
// INPUT:
// - ES:[DI] pointer to logical addr
// OUTPUT:
// - EDX:EAX target LBA
// - sets ZF on failure
ConvertAddress:
    // NOTE: SearchTree will overwrite data buffer area and our logical addr will be erased!
    // so putting it on stack
    push dword ptr es:[di+4]
    push dword ptr es:[di]

    mov bl, 1 // indicates first try. On second try BL must be set to 0
ConvertAddress_secondTry:
    push ds
    pop es
    xor ecx, ecx // set index to 0
ConvertAddress_loop:
    cmp cl, byte ptr [ChunkMapSize]
    jae ConvertAddress_checkInclusion // greater chunk is not found in chunk map - checking the last one

    std  // numbers are little-engian, going right-to-left
    mov si, CHUNK_MAP_OFFSET
    lea si, [esi+ecx*8]
    lea si, [esi+ecx*8]
    lea si, [esi+ecx*8]   // shift by 24 bytes

    lea di, [esp+4]      // set to the second dword the 8-byte number
    lea si, [si+4]
    inc cl

    cmpsd
    jb ConvertAddress_loop
    cmpsd
    jb ConvertAddress_loop

ConvertAddress_checkInclusion:
    dec cl
    cld
    // found chunk map item, checking inclusion with length
    mov si, CHUNK_MAP_OFFSET
    lea si, [esi+ecx*8]
    lea si, [esi+ecx*8]
    lea si, [esi+ecx*8]   // shift by 24 bytes

    // logical_addr + length
    mov eax, dword ptr [si]   // low dword of address
    mov edx, dword ptr [si+4] // high dword of address
    add eax, dword ptr [si+8] // low dword of length
    adc edx, dword ptr [si+12] // high dword of length
    // edx:eax is the end of the chunk

    // (logical_addr + length) - addr_to_find
    cmp edx, dword ptr [esp+4]
    ja ConvertAddress_found
    cmp eax, dword ptr [esp]
    ja ConvertAddress_found // address is greater than end of the chunk
    test bl, bl
    jnz ConvertAddress_notFound
    ret 8

ConvertAddress_found:
    // found chunk. Calculating the address
    // addr_to_find - logical_addr
    pop eax // low dword of addr_to_find
    pop edx // high dword of addr_to_find
    sub eax, dword ptr [si]
    sbb edx, dword ptr [si+4]
    // (addr_to_find - logical_addr) + physical_addr
    add eax, dword ptr [si+16]
    adc edx, dword ptr [si+20]

    // edx:eax is physical address. Converting to LBA (shifting by 9 bits)
    shrd eax, edx, 9
    shr  edx, 9
    inc bl // clears ZF (bl is 0 or 1 here)
    ret

ConvertAddress_notFound:
    // offset is alredy on stack
    //push dword ptr [esp+4]
    //push dword ptr [esp]
    dec sp
    mov byte ptr [esp], CHUNK_ITEM_KEY
    data32 push 0
    push 0
    push FIRST_CHUNK_TREE_OBJECTID

    mov eax, dword ptr [ChunkRootAddress]
    mov edx, dword ptr [ChunkRootAddress+4]
    mov cl, byte ptr [ChunkRootLevel]

    call SearchTree
    add sp, 9 // setting stack back

    mov ax, si // ES:[SI] - found key pointer
    call InsertChunk

    mov bl, 0 // indicates second try
    jmp ConvertAddress_secondTry


// Compare key (key1) with key in array identified by base and index (key2)
// INPUT:
// - DS:[SI] key1 pointer
// - ES:[DI] start of the array
// - AX size of one key in array
// - CX key index
// OUTPUT:
// - DS:[SI] key1 pointer
// - ES:[DI] key2 pointer
// - sets flags according to comparison
CompareKeys:
    //xchg si, di

    mul cx
    add di, ax
    push ds
    push si
    push es
    push di

    // must be replaced for proper flags after cmps
    push ds
    push es
    pop ds
    pop es
    xchg si, di

    lea si, [si+4] // key in array
    lea di, [di+4] // searchable key
    std

    // comparing objectid
    cmpsd
    jne CompareKeys_end
    cmpsd
    jne CompareKeys_end
    // comparing type
    lea si, [si+12]
    lea di, [di+12]

    cmpsb
    jne CompareKeys_end
    // comparing offset
    lea si, [si+1+1+4]
    lea di, [di+1+1+4]

    cmpsd
    jne CompareKeys_end
    cmpsd
CompareKeys_end:
    cld
    pop di
    pop es
    pop si
    pop ds
    ret



btrfsSignature:
    .ascii "_BHRfS_M"

//.org 1022
//    .word HEX(aa55)


wrongSignatureError:
    .asciz "BTRFS read error"
MaxItemsError:
    .asciz "Max items error"
filenameCrc:
    .long HEX(68cba33d)          // computed hashsum of "freeldr.sys"
freeldrFilename:
    .ascii "freeldr.sys"
notFoundError:
    .asciz "NFE"
.org 1534
    .word HEX(aa55)
.endcode16

END
