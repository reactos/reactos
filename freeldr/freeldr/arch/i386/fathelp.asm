; FATHELP.ASM
; FAT12/16 Boot Sector Helper Code
; Copyright (c) 1998, 2001, 2002 Brian Palmer



;org 8000h

segment .text

bits 16


BootSectorStackTop		equ		0x7bf2
DataAreaStartHigh		equ		0x2
DataAreaStartLow		equ		0x4
BiosCHSDriveSizeHigh	equ		0x6
BiosCHSDriveSizeLow		equ		0x8
BiosCHSDriveSize		equ		0x8
ReadSectorsOffset		equ		0xa
ReadClusterOffset		equ		0xc
PutCharsOffset			equ		0xe

OEMName					equ		3
BytesPerSector			equ		11
SectsPerCluster			equ		13
ReservedSectors			equ		14
NumberOfFats			equ		16
MaxRootEntries			equ		17
TotalSectors			equ		19
MediaDescriptor			equ		21
SectorsPerFat			equ		22
SectorsPerTrack			equ		24
NumberOfHeads			equ		26
HiddenSectors			equ		28
TotalSectorsBig			equ		32
BootDrive				equ		36
Reserved				equ		37
ExtendSig				equ		38
SerialNumber			equ		39
VolumeLabel				equ		43
FileSystem				equ		54

BootPartition			equ		0x7dfd
        

; This code will be stored in the first 512 bytes
; of freeldr.sys. The first 3 bytes will be a jmp
; instruction to skip past the FAT helper code
; that is stored in the rest of the 512 bytes.
;
; This code is loaded at 0000:8000 so we have to
; encode a jmp instruction to jump to 0000:8200

global start
start:
        db	0xe9
		db	0xfd
		db	0x01

; Now starts the extra boot code that we will store
; in the first 512 bytes of freeldr.sys. This code
; allows the FAT12/16 bootsector to navigate the
; FAT table so that we can still load freeldr.sys
; even if it is fragmented.


FatHelperEntryPoint:

		push ax							; First save AX - the start cluster of freeldr.sys


		; Display "Loading FreeLoader..." message
        mov  esi,msgLoading				; Loading message
        call [bp-PutCharsOffset]		; Display it


		call ReadFatIntoMemory

		pop  ax							; Restore AX (start cluster)
		; AX has start cluster of freeldr.sys

        mov  bx,800h
        mov  es,bx

LoadFile:
		push ax
		call IsFat12
		pop  ax
		jnc  LoadFile2
		cmp  ax,0ff8h		    ; Check to see if this is the last cluster in the chain
		jmp  LoadFile3
LoadFile2:
		cmp  ax,0fff8h
LoadFile3:
		jae	 LoadFile_Done		; If so continue, if not then read then next one
		push ax
        xor  bx,bx              ; Load ROSLDR starting at 0000:8000h
		push es
		call [bp-ReadClusterOffset]
		pop  es

		xor  bx,bx
        mov  bl,BYTE [BYTE bp+SectsPerCluster]
		shl  bx,5							; BX = BX * 512 / 16
		mov  ax,es							; Increment the load address by
		add  ax,bx							; The size of a cluster
		mov  es,ax

		call IsFat12
		pop  ax
		push es
		jnc  LoadFile4
		call GetFatEntry12					; Get the next entry
		jmp  LoadFile5
LoadFile4:
		call GetFatEntry16
LoadFile5:
		pop  es

        jmp  LoadFile						; Load the next cluster (if any)

LoadFile_Done:
        mov  dl,BYTE [BYTE bp+BootDrive]	; Load the boot drive into DL
		mov  dh,[BootPartition]				; Load the boot partition into DH
        push WORD 0x0000
        push WORD 0x8000					; We will do a far return to 0000:8000h
        retf								; Transfer control to ROSLDR


; Reads the entire FAT into memory at 7000:0000
ReadFatIntoMemory:
        mov   ax,WORD [BYTE bp+HiddenSectors] 
        mov   dx,WORD [BYTE bp+HiddenSectors+2]
		add   ax,WORD [BYTE bp+ReservedSectors]
		adc   dx,byte 0
		mov   cx,WORD [BYTE bp+SectorsPerFat]
		mov   bx,7000h
		mov   es,bx
		xor   bx,bx
		call  [bp-ReadSectorsOffset]
		ret


; Returns the FAT entry for a given cluster number for 16-bit FAT
; On entry AX has cluster number
; On return AX has FAT entry for that cluster
GetFatEntry16:

		mov   cx,2						; AX = AX * 2 (since FAT16 entries are 2 bytes)
		mul   cx
		shl   dx,12

        mov   bx,7000h
		add   bx,dx
        mov   es,bx
		mov   bx,ax						; Restore FAT entry offset
		mov   ax,WORD [es:bx]	    	; Get FAT entry

		ret


; Returns the FAT entry for a given cluster number for 12-bit FAT
; On entry AX has cluster number
; On return AX has FAT entry for that cluster
GetFatEntry12:

		push  ax
		mov   cx,ax
		shr   ax,1
		add   ax,cx						; AX = AX * 1.5 (AX = AX + (AX / 2)) (since FAT12 entries are 12 bits)

        mov   bx,7000h
        mov   es,bx
		mov   bx,ax						; Put FAT entry offset into BX
		mov   ax,WORD [es:bx]	    	; Get FAT entry
		pop   cx						; Get cluster number from stack
		and   cx,1
		jz    UseLow12Bits
		and   ax,0fff0h
		shr   ax,4
		jmp   GetFatEntry12_Done

UseLow12Bits:
		and   ax,0fffh

GetFatEntry12_Done:

		ret


; Returns CF = 1 if this is a FAT12 file system
; Otherwise CF = 0 for FAT16
IsFat12:

		mov   bx,[BYTE bp-DataAreaStartLow]
		mov   cx,[BYTE bp-DataAreaStartHigh]
        ; CX:BX now has the number of the starting sector of the data area

		xor   dx,dx
		mov   ax,WORD [BYTE bp+TotalSectors]
		cmp   ax,byte 0
		jnz   IsFat12_2
		mov   ax,WORD [BYTE bp+TotalSectorsBig]
		mov   dx,WORD [BYTE bp+TotalSectorsBig+2]

		; DX:AX now contains the number of sectors on the volume
IsFat12_2:
		sub   ax,bx				; Subtract data area start sector
		sub   dx,cx				; from total sectors of volume

		; DX:AX now contains the number of data sectors on the volume
		movzx bx,BYTE [BYTE bp+SectsPerCluster]
		div   bx
		; AX now has the number of clusters on the volume
		stc
		cmp   ax,4085
		jb    IsFat12_Done
		clc

IsFat12_Done:
		ret



msgLoading	db 'Loading FreeLoader...',0dh,0ah,0

			times 510-($-$$) db 0	; Pad to 510 bytes
			dw 0aa55h				; BootSector signature
