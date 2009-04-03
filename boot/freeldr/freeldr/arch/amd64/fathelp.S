// fathelp.S
// FAT12/16 Boot Sector Helper Code
// Copyright (c) 1998, 2001, 2002, 2003 Brian Palmer

.intel_syntax noprefix

.text

.code16

#define BootSectorStackTop		0x7bf2
#define DataAreaStartHigh		0x2
#define DataAreaStartLow		0x4
#define BiosCHSDriveSizeHigh	0x6
#define BiosCHSDriveSizeLow		0x8
#define BiosCHSDriveSize		0x8
#define ReadSectorsOffset		0xa
#define ReadClusterOffset		0xc
#define PutCharsOffset			0xe

#define OEMName					3
#define BytesPerSector			11
#define SectsPerCluster			13
#define ReservedSectors			14
#define NumberOfFats			16
#define MaxRootEntries			17
#define TotalSectors			19
#define MediaDescriptor			21
#define SectorsPerFat			22
#define SectorsPerTrack			24
#define NumberOfHeads			26
#define HiddenSectors			28
#define TotalSectorsBig			32
#define BootDrive				36
#define Reserved				37
#define ExtendSig				38
#define SerialNumber			39
#define VolumeLabel				43
#define FileSystem				54


// This code will be stored in the first 512 bytes
// of freeldr.sys. The first 3 bytes will be a jmp
// instruction to skip past the FAT helper code
// that is stored in the rest of the 512 bytes.
//
// This code is loaded at 0000:8000 so we have to
// encode a jmp instruction to jump to 0000:8200
//.org 0x8000

.global _mainCRTStartup	// For Mingw32 builds where the linker looks for this symbol
_mainCRTStartup:
.global start
start:
        .byte	0xe9
		.byte	0xfd
		.byte	0x01

// Now starts the extra boot code that we will store
// in the first 512 bytes of freeldr.sys. This code
// allows the FAT12/16 bootsector to navigate the
// FAT table so that we can still load freeldr.sys
// even if it is fragmented.


FatHelperEntryPoint:

		push ax							// First save AX - the start cluster of freeldr.sys


		// Display "Loading FreeLoader..." message
        mov  esi, offset msgLoading			// Loading message
        call [bp-PutCharsOffset]		// Display it


		call ReadFatIntoMemory

		pop  ax							// Restore AX (start cluster)
		// AX has start cluster of freeldr.sys

        mov  bx,0x800
        mov  es,bx

LoadFile:
		push ax
		call IsFat12
		pop  ax
		jnc  LoadFile2
		cmp  ax,0x0ff8		    // Check to see if this is the last cluster in the chain
		jmp  LoadFile3
LoadFile2:
		cmp  ax,0x0fff8
LoadFile3:
		jae	 LoadFile_Done		// If so continue, if not then read then next one
		push ax
        xor  bx,bx              // Load ROSLDR starting at 0000:8000h
		push es
		call [bp-ReadClusterOffset]
		pop  es

		xor  bx,bx
        mov  bl, [bp+SectsPerCluster]
		shl  bx,5							// BX = BX * 512 / 16
		mov  ax,es							// Increment the load address by
		add  ax,bx							// The size of a cluster
		mov  es,ax

		call IsFat12
		pop  ax
		push es
		jnc  LoadFile4
		call GetFatEntry12					// Get the next entry
		jmp  LoadFile5
LoadFile4:
		call GetFatEntry16
LoadFile5:
		pop  es

        jmp  LoadFile						// Load the next cluster (if any)

LoadFile_Done:
        mov  dl, [bp+BootDrive]	// Load the boot drive into DL
		mov  dh, BootPartition				// Load the boot partition into DH
        push word ptr 0x0000
        push word ptr 0x8000					// We will do a far return to 0000:8000h

//        retf								// Transfer control to ROSLDR
	.byte 0xcb // == retf


// Reads the entire FAT into memory at 7000:0000
ReadFatIntoMemory:
        mov   ax, [bp+HiddenSectors] 
        mov   dx, [bp+HiddenSectors+2]
		add   ax, [bp+ReservedSectors]
		adc   dx, 0
		mov   cx, [bp+SectorsPerFat]
		mov   bx,0x7000
		mov   es,bx
		xor   bx,bx
		call  [bp-ReadSectorsOffset]
		ret


// Returns the FAT entry for a given cluster number for 16-bit FAT
// On entry AX has cluster number
// On return AX has FAT entry for that cluster
GetFatEntry16:

		mov   cx,2						// AX = AX * 2 (since FAT16 entries are 2 bytes)
		mul   cx
		shl   dx,12

        mov   bx,0x7000
		add   bx,dx
        mov   es,bx
		mov   bx,ax						// Restore FAT entry offset
		mov   ax, [es:bx]	    	// Get FAT entry

		ret


// Returns the FAT entry for a given cluster number for 12-bit FAT
// On entry AX has cluster number
// On return AX has FAT entry for that cluster
GetFatEntry12:

		push  ax
		mov   cx,ax
		shr   ax,1
		add   ax,cx						// AX = AX * 1.5 (AX = AX + (AX / 2)) (since FAT12 entries are 12 bits)

        mov   bx,0x7000
        mov   es,bx
		mov   bx,ax						// Put FAT entry offset into BX
		mov   ax, [es:bx]	    	// Get FAT entry
		pop   cx						// Get cluster number from stack
		and   cx,1
		jz    UseLow12Bits
		and   ax,0x0fff0
		shr   ax,4
		jmp   GetFatEntry12_Done

UseLow12Bits:
		and   ax,0x0fff

GetFatEntry12_Done:

		ret


// Returns CF = 1 if this is a FAT12 file system
// Otherwise CF = 0 for FAT16
IsFat12:

		mov   ebx, [bp-DataAreaStartLow]
        // EBX now has the number of the starting sector of the data area
		// starting from the beginning of the disk, so subtrace hidden sectors
		sub   ebx, [bp+HiddenSectors]


		xor   eax,eax
		mov   ax, [bp+TotalSectors]
		cmp   ax, 0
		jnz   IsFat12_2
		mov   eax, [bp+TotalSectorsBig]

		// EAX now contains the number of sectors on the volume

IsFat12_2:
		sub   eax,ebx				// Subtract data area start sector
		xor   edx,edx				// from total sectors of volume

		// EDX:EAX now contains the number of data sectors on the volume
		movzx ebx, byte ptr [bp+SectsPerCluster]
		div   ebx
		// EAX now has the number of clusters on the volume
		stc
		cmp   eax,4085
		jb    IsFat12_Done
		clc

IsFat12_Done:
		ret



msgLoading:
		.ascii "Loading FreeLoader..."
		.byte 0x0d,0x0a,0

//			times 510-($-$$) db 0	// Pad to 510 bytes
.org 0x1fe
			.word 0x0aa55				// BootSector signature


// pseudo adresses
//.org 0x7dfd
BootPartition:
