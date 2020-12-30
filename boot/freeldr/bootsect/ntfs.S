/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Bootsector
 * FILE:            boot/freeldr/bootsect/fat32.S
 * PURPOSE:
 * PROGRAMMERS:     Sylvain Deverre
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <freeldr/include/arch/pc/x86common.h>

#define BP_REL(x) [bp+x-offset start]

.code16

//ORG HEX(7c00)

start:
    jmp short main
    nop
OEMName:
    .ASCII "NTFS    "   // NTFS signature
BytesPerSector:
    .word 512          
SectsPerCluster:
    .byte 8
ReservedSectors:
    .word 0             // Unused by NTFS
NumberOfFats:
    .byte 0             // Unused by NTFS
MaxRootEntries:
    .word 0             // Unused by NTFS
TotalSectors:
    .word 0             // Unused by NTFS
MediaDescriptor:
    .byte HEX(0f8)
SectorsPerFat:
    .word 0             // Unused by NTFS
SectorsPerTrack:
    .word 0             // Should contain disk geometry
NumberOfHeads:
    .word 0             // Should contain disk geometry
HiddenSectors:
    .long 0
TotalSectorsBig:
    .long 0

// NTFS inserted info
BootDrive:
    .byte HEX(80)
CurrentHead:
    .byte 0
BootSignature:
    .byte HEX(80)
Unused:
    .byte 0
VolumeSectorCount:
    .long 0
    .long 0             // Must be patched by format program !
MftLocation:
    .long 0
    .long 0             // Must be patched by format program !
MftMirrorLocation:
    .long 0
    .long 0             // Must be patched by format program !
ClustersPerMftRecord:
    .long 0
ClustersPerIndexRecord:
    .long 0
VolumeSerialNumber:
    .long 0
    .long 0
Checksum:
    .long 0

main:
    // int 3
    xor ax,ax               // Setup segment registers
    mov ds,ax               // Make DS correct
    mov es,ax               // Make ES correct
    mov ss,ax               // Make SS correct
    mov bp, HEX(7c00)
    mov sp, HEX(7c00)       // Setup a stack

    cmp byte ptr [BootDrive], HEX(0ff)    // If they have specified a boot drive then use it
    jne GetDriveParameters

    mov byte ptr [BootDrive], dl          // Save the boot drive

GetDriveParameters:
    mov  ax, HEX(0800)
    mov  dl, byte ptr [BootDrive]               // Get boot drive in dl
    int  HEX(13)                                // Request drive parameters from the bios
    jnc  CalcDriveSize                          // If the call succeeded then calculate the drive size

    // If we get here then the call to the BIOS failed
    // so just set CHS equal to the maximum addressable
    // size
    mov  cx, HEX(0ffff)
    mov  dh, cl

CalcDriveSize:
    // Now that we have the drive geometry
    // lets calculate the drive size
    mov  bl, ch         // Put the low 8-bits of the cylinder count into BL
    mov  bh, cl         // Put the high 2-bits in BH
    shr  bh, 6          // Shift them into position, now BX contains the cylinder count
    and  cl, HEX(3f)    // Mask off cylinder bits from sector count
    // CL now contains sectors per track and DH contains head count
    movzx eax, dh       // Move the heads into EAX
    movzx ebx, bx       // Move the cylinders into EBX
    movzx ecx, cl       // Move the sectors per track into ECX
    inc   eax           // Make it one based because the bios returns it zero based
    inc   ebx           // Make the cylinder count one based also
    mul   ecx           // Multiply heads with the sectors per track, result in edx:eax
    mul   ebx           // Multiply the cylinders with (heads * sectors) [stored in edx:eax already]

    // We now have the total number of sectors as reported
    // by the bios in eax, so store it in our variable
    mov dword ptr ds:[BiosCHSDriveSize], eax

LoadExtraBootCode:
    // First we have to load our extra boot code at
    // next sector into memory at [0000:7e00h]
    mov  eax, HEX(1)
    add  eax, dword ptr [HiddenSectors]         // Add the number of hidden sectors
    mov  cx, 2
    xor  bx, bx
    mov  es, bx                                 // Read sector to [0000:7e00h]
    mov  bx, HEX(7e00)
    call ReadSectors
    jmp  StartSearch


// Reads logical sectors into [ES:BX]
// EAX has logical sector number to read
// CX has number of sectors to read
ReadSectors:
    push es
    cmp  eax, dword ptr ds:[BiosCHSDriveSize]   // Check if they are reading a sector outside CHS range
    jae  ReadSectorsLBA                         // Yes - go to the LBA routine
                                                // If at all possible we want to use LBA routines because
                                                // They are optimized to read more than 1 sector per read

    pushad                                      // Save logical sector number & sector count

CheckInt13hExtensions:                          // Now check if this computer supports extended reads
    mov  ah, HEX(41)                            // AH = 41h
    mov  bx, HEX(55aa)                          // BX = 55AAh
    mov  dl, byte ptr [BootDrive]               // DL = drive (80h-FFh)
    int  HEX(13)                                // IBM/MS INT 13 Extensions - INSTALLATION CHECK
    jc   ReadSectorsCHS                         // CF set on error (extensions not supported)
    cmp  bx, HEX(0aa55)                         // BX = AA55h if installed
    jne  ReadSectorsCHS
    test cl,1                                   // CX = API subset support bitmap
    jz   ReadSectorsCHS                         // Bit 0, extended disk access functions (AH=42h-44h,47h,48h) supported

    popad                                       // Restore sector count & logical sector number

ReadSectorsLBA:
    pushad                                      // Save logical sector number & sector count

    cmp  cx, 64                                 // Since the LBA calls only support 0x7F sectors at a time we will limit ourselves to 64
    jbe  ReadSectorsSetupDiskAddressPacket      // If we are reading less than 65 sectors then just do the read
    mov  cx, 64                                 // Otherwise read only 64 sectors on this loop iteration

ReadSectorsSetupDiskAddressPacket:
    mov word ptr ds:[LBASectorsRead],cx
    push 0
    push 0
    push eax                                // Put 64-bit logical block address on stack
    push es                                 // Put transfer segment on stack
    push bx                                 // Put transfer offset on stack
    push cx                                 // Set transfer count
    push 16                                 // Set size of packet to 10h
    mov  si, sp                             // Setup disk address packet on stack

    mov  dl, byte ptr [BootDrive]           // Drive number
    mov  ah, HEX(42)                        // Int 13h, AH = 42h - Extended Read
    int  HEX(13)                            // Call BIOS
    jc   PrintDiskError                     // If the read failed then abort

    add  sp, 16                             // Remove disk address packet from stack

    popad                                   // Restore sector count & logical sector number

    push bx
    mov  ebx, dword ptr ds:[LBASectorsRead]
    add  eax, ebx                           // Increment sector to read
    shl  ebx, 5
    mov  dx, es
    add  dx, bx                             // Setup read buffer for next sector
    mov  es, dx
    pop  bx

    sub  cx, word ptr ds:[LBASectorsRead]
    jnz  ReadSectorsLBA                     // Read next sector

    pop es
    ret

LBASectorsRead:
    .long    0


// Reads logical sectors into [ES:BX]
// EAX has logical sector number to read
// CX has number of sectors to read
ReadSectorsCHS:
    popad                                        // Get logical sector number & sector count off stack

ReadSectorsCHSLoop:
    pushad
    xor  edx, edx
    movzx ecx, word ptr [SectorsPerTrack]
    div  ecx                                    // Divide logical by SectorsPerTrack
    inc  dl                                     // Sectors numbering starts at 1 not 0
    mov  cl, dl                                 // Sector in CL
    mov  edx, eax
    shr  edx, 16
    div  word ptr [NumberOfHeads]               // Divide logical by number of heads
    mov  dh, dl                                 // Head in DH
    mov  dl, byte ptr [BootDrive]               // Drive number in DL
    mov  ch, al                                 // Cylinder in CX
    ror  ah, 1                                  // Low 8 bits of cylinder in CH, high 2 bits
    ror  ah, 1                                  //  in CL shifted to bits 6 & 7
    or   cl, ah                                 // Or with sector number
    mov  ax, HEX(0201)
    int  HEX(13)    // DISK - READ SECTORS INTO MEMORY
                     // AL = number of sectors to read, CH = track, CL = sector
                     // DH = head, DL = drive, ES:BX -> buffer to fill
                     // Return: CF set on error, AH = status (see AH=01h), AL = number of sectors read

    jc   PrintDiskError                         // If the read failed then abort

    popad

    inc  eax                                    // Increment Sector to Read

    mov  dx, es
    add  dx, 32                                 // Increment read buffer for next sector
    mov  es, dx

    loop ReadSectorsCHSLoop                     // Read next sector

    pop es
    ret

// Displays a disk error message
// And reboots
PrintDiskError:
    mov  si, offset msgDiskError        // Bad boot disk message
    call PutChars                       // Display it

    jmp  Reboot

// Displays a file system error message
// And reboots
PrintFileSystemError:
    mov  si, offset msgFileSystemError  // FreeLdr not found message
    call PutChars                       // Display it

Reboot:
    mov  si, offset msgAnyKey           // Press any key message
    call PutChars                       // Display it
    xor  ax, ax
    int  HEX(16)                        // Wait for a keypress
    int  HEX(19)                        // Reboot

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
msgFileSystemError:
    .ascii "File system error", CR, LF, NUL
msgAnyKey:
    .ascii "Press any key to restart", CR, LF, NUL

SectsPerMFT:
    .word 0
    
MFTStartSector:
    .long 0

BiosCHSDriveSize:
    .long 0

.org 510 // Pad to 509 bytes

BootFlagSignature:
    .word HEX(0aa55)    // BootSector signature

StartSearch:
    // Compute MFT start sector
    mov   eax, dword ptr [MftLocation]
    mov   cl, byte ptr [SectsPerCluster]
    movzx ecx, cx
    mul   ecx
    mov   dword ptr MFTStartSector, eax

    // Compute size of MFT entry in sectors
    mov   al, byte ptr [ClustersPerMftRecord]
    movzx ax, al
    test  al, HEX(80)
    jnz   NegativeOffset
    mov   cx, word ptr [SectsPerCluster]
    mul   cx
    mov   word ptr [SectsPerMFT], cx
    jmp   SearchNext
    NegativeOffset:
    not   al
    mov   cl, al
    sub   cl, 8
    xor   ax, ax
    inc   ax
    shl   ax, cl
    mov   word ptr [SectsPerMFT], ax

    SearchNext:
    mov   bp, sp
    sub   sp, HEX(10)

    // Read Root Directory MFT
    mov   ax, HEX(2000)
    mov   es, ax
    xor   bx, bx
    mov   eax, 5
    call  ReadInode
    cmp   dword ptr es:[0], HEX(454c4946)
    jnz   PrintFileSystemError

    // Valid MFT, check INDEX_ROOT attribute
    xor   ax, ax
    mov   ebx, HEX(90)
    call  FindAttributeHdr
    test  ax, ax
    jz    PrintFileSystemError
    mov   si, ax
    cmp   byte ptr es:[si + 8], HEX(0)     // Checks resident flag
    jnz   PrintFileSystemError
    add   si, word ptr es:[si + HEX(14)]   // Offset to resident data
    cmp   dword ptr es:[si], HEX(30)
    jnz   PrintFileSystemError

    // Save our INDEX_ROOT
    mov   word ptr [bp-2], si
    xor   ax, ax
    mov   ebx, HEX(A0)
    call  FindAttributeHdr
    test  ax, ax
    jz    PrintFileSystemError
    mov   si, ax
    cmp   byte ptr es:[si + 8], HEX(1)     // Check non-resident flag
    jnz   PrintFileSystemError
    add   si, word ptr es:[si + HEX(20)]   // Offset to run list
    mov   ax, es
    mov   ax, word ptr [bp-2]
    mov   bx, si
    call  LoopNodes
    jmp   Reboot

// Finds Freeldr.sys into the directory tree and returns the
// inode index
// [ES:AX] address of the INDEX_ROOT attribute data
// [ES:BX] address of the INDEX_ALLOCATION run list
LoopNodes:
    push  bp
    mov   bp, sp
    sub   sp, HEX(10)
    mov   word ptr [bp-2], es
    // TODO: read non-resident attribute data
    push  ax
    mov   ax, HEX(9000)
    mov   fs, ax
    mov   ax, bx
    xor   ecx, ecx
    xor   bx, bx
    call  ReadNonResidentAttribute
    test  ax, ax
    jz    PrintFileSystemError
    cmp   dword ptr fs:[0], HEX(58444e49) // Check if valid INDX
    jz    PrintFileSystemError
    pop   ax
    mov   sp, bp
    pop   bp
    ret

// Reads a NTFS cluster
// AX     : cluster number to read
// ES:BX  : address to read
ReadCluster:
    push  edx
    push  ecx
    mov   cl, byte ptr [SectsPerCluster]   // Convert sectors to clusters
    movzx ecx, cx
    mul   ecx
    add   eax, dword ptr [HiddenSectors]   // Add offset from the disk beginning
    call  ReadSectors
    pop   ecx
    pop   edx
    ret

// Reads a MFT entry
// AX     : MFT inode
// ES:BX  : address to read
ReadInode:
    push  edx
    push  ecx
    mov   cx, word ptr [SectsPerMFT]       // Get the correct number of sectors for the FILE entry
    mul   cx
    movzx eax, ax
    mov   ecx, dword ptr [MFTStartSector]  // Add it to the start of the MFT
    add   eax, ecx
    add   eax, dword ptr [HiddenSectors]   // Add disk offset
    mov   cx, word ptr [SectsPerMFT]
    movzx ecx, cx
    call  ReadSectors
    pop   ecx
    pop   edx
    ret

// Reads a non-resident attribute
// ES:AX : Address of the data runs
// FS:BX : Address to write to
// ECX    : LCN to read
ReadNonResidentAttribute:
    push  bp
    mov   bp, sp
    sub   sp, HEX(10)
    push  dx
    push  si
    mov   dword ptr [bp - 4], 0          // Store the current LCN
    mov   word ptr [bp - 8], bx
    mov   dword ptr [bp - 12], ecx
    mov   si, ax
    xor   dx, dx
    RunLoop:
        mov   al, byte ptr es:[si]
        test  al, al
        jz    NotFound
        call  UnpackRun
        push  ecx
        call  PrintInt
        add   dword ptr [bp - 4], eax
        cmp   dword ptr [bp - 12], ecx
        jb    FoundRun
        sub   dword ptr [bp - 12], ecx  // Decrement the cluster
        jmp   RunLoop
FoundRun:
    mov   ebx, dword ptr [bp - 4]
    mov   ecx, dword ptr [bp - 12]
    add   ebx, ecx

    push es
    mov  ax, fs
    mov  es, ax
    mov  eax, ebx
    mov  bx, word ptr [bp - 8]
    call ReadCluster
    pop  es
    jmp RunSearchEnd
NotFound:
    xor ax, ax
RunSearchEnd:
    pop si
    pop dx
    mov sp, bp
    pop bp
    ret

// Function to unpack a run
// ES:SI   : address of the run
// returns unpacked LCN in EAX, unpaked len in ECX
// SI points to the next run
UnpackRun:
    push bp
    mov  bp, sp
    sub  sp, HEX(10)
    push ebx

    // Unpack run header
    mov bl, byte ptr es:[si]
    inc si

    mov bh, bl
    shr bh, 4 
    and bl, 7 
    mov byte ptr [bp-2], bh     // BH contains the LCN length
    mov byte ptr [bp-1], bl     // BL contains the number of cluster length
    
    mov al, bl
    call UnpackLen
    mov dword ptr [bp - 8], ebx

    mov al, byte ptr [bp-2]
    call UnpackLen
    mov cl, byte ptr es:[si-1]  // Fixup sign if last byte is > 255
    test cl, HEX(80)
    jz NoSign
    not eax
    add ebx, eax

    NoSign:
    mov eax, ebx
    mov ecx, dword ptr [bp - 8]
    pop ebx
    mov sp, bp
    pop bp
    ret

UnpackLen:
    push cx
    movzx ax, al
    // Read length
    // We read the whole DWORD and then compute a mask to remove
    // unneeded bytes
    xor ebx, ebx
    mov ebx, dword ptr es:[si]
    add si, ax

    // Compute 2^(8*len) - 1                 
    mov cl, al
    shl cl, 3
    xor eax, eax
    inc eax
    shl eax, cl
    dec eax
    // Apply this mask to what we read
    and ebx, eax
    pop cx
    ret

// Finds an attribute header
// ES:AX : pointer to the MFT entry
// EBX   : type to find
// returns: offset to the attribute header or 0 if not found
FindAttributeHdr:
    push  cx
    push  si
    push  edx
    mov   si, ax
    mov   cx, word ptr es:[si+HEX(14)]  // Get offset attribute
    add   si, cx                        // si = offset + size
    FindAttributeHdrLoop:
        mov   edx, dword ptr es:[si]    // Get attribute type 
        cmp   edx, ebx                
        jz    AttrFound
        cmp   edx, HEX(ffffffff)
        jz    AttrNotFound
        add   cx, word ptr es:[si + 4]  // Add size of the attribute
        add   si, word ptr es:[si + 4] 
        jmp   FindAttributeHdrLoop
    
    AttrNotFound:
    // Attribute not found, reset the offset
    xor   cx, cx
    AttrFound:
    mov   ax, cx
    pop   edx
    pop   si
    pop   cx
    ret

offs:
    .ascii "0123456789abcdef"

PrintInt:
    push bp
    mov  bp, sp
    sub  sp, 2
    pushad
    mov  word ptr [bp - 2], 32
    IntLoop:
        mov eax, dword ptr [bp + 4]
        sub word ptr [bp - 2], 4
        mov cx, word ptr [bp - 2]
        shr eax, cl
        and ax, HEX(f)
        mov si, offset offs
        add si, ax
        mov al, [si]
        mov ah, HEX(0e)
        mov bx, 7
        int HEX(10)
        mov cx, word ptr [bp - 2]
        test cx, cx
        jnz IntLoop
    mov al, HEX(0a)
    mov ah, HEX(0e)
    mov bx, 7
    int HEX(10)
    mov al, HEX(0d)
    mov ah, HEX(0e)
    mov bx, 7
    int HEX(10)
    popad
    mov sp, bp
    pop bp
    ret 4

PutWChars:
    lodsw
    or   al, al
    jz   short WDone
    mov  ah, HEX(0e)
    mov  bx, 7
    int  HEX(10)
    jmp  short PutWChars
WDone:
    ret

FreeLdr:
.word 'F', 'R', 'E', 'E', 'L', 'D', 'R', '.', 'S', 'Y', 'S', NUL