; EXT2.ASM
; EXT2 Boot Sector
; Copyright (c) 2002 Brian Palmer


EXT2_ROOT_INO			equ 2
EXT2_S_IFMT				equ	0f0h
EXT2_S_IFREG			equ 080h


org 7c00h

segment .text

bits 16

start:
        jmp short main
        nop

BootDrive				db 0x80
SectorsPerTrack			dw 63
NumberOfHeads			dw 16
BiosCHSDriveSize		dd (1024 * 1024 * 63)
LBASectorsRead			dd 0

Ext2VolumeStartSector	dd 263088				; Start sector of the ext2 volume
Ext2BlockSize			dd 2					; Block size in sectors
Ext2BlockSizeInBytes	dd 1024					; Block size in bytes
Ext2PointersPerBlock	dd 256					; Number of block pointers that can be contained in one block
Ext2GroupDescPerBlock	dd 32					; Number of group descriptors per block
Ext2FirstDataBlock		dd 1					; First data block (1 for 1024-byte blocks, 0 for bigger sizes)
Ext2InodesPerGroup		dd 2048					; Number of inodes per group
Ext2InodesPerBlock		dd 8					; Number of inodes per block

Ext2ReadEntireFileLoadSegment:
	dw	0
Ext2InodeIndirectPointer:
	dd	0
Ext2InodeDoubleIndirectPointer:
	dd	0
Ext2BlocksLeftToRead:
	dd	0

main:
        xor ax,ax               ; Setup segment registers
        mov ds,ax               ; Make DS correct
        mov es,ax               ; Make ES correct
        mov ss,ax				; Make SS correct
		mov bp,7c00h
        mov sp,7c00h            ; Setup a stack


GetDriveParameters:
		mov  ax,0800h
		mov  dl,[BYTE bp+BootDrive]					; Get boot drive in dl
		int  13h									; Request drive parameters from the bios
		jnc  CalcDriveSize							; If the call succeeded then calculate the drive size

		; If we get here then the call to the BIOS failed
		; so just set CHS equal to the maximum addressable
		; size
		mov  cx,0ffffh
		mov  dh,cl

CalcDriveSize:
		; Now that we have the drive geometry
		; lets calculate the drive size
		mov  bl,ch			; Put the low 8-bits of the cylinder count into BL
		mov  bh,cl			; Put the high 2-bits in BH
		shr  bh,6			; Shift them into position, now BX contains the cylinder count
		and  cl,3fh			; Mask off cylinder bits from sector count
		; CL now contains sectors per track and DH contains head count
		movzx eax,dh		; Move the heads into EAX
		movzx ebx,bx		; Move the cylinders into EBX
		movzx ecx,cl		; Move the sectors per track into ECX
		inc   eax			; Make it one based because the bios returns it zero based
		inc   ebx			; Make the cylinder count one based also
		mul   ecx			; Multiply heads with the sectors per track, result in edx:eax
		mul   ebx			; Multiply the cylinders with (heads * sectors) [stored in edx:eax already]

		; We now have the total number of sectors as reported
		; by the bios in eax, so store it in our variable
		mov   [BYTE bp+BiosCHSDriveSize],eax


LoadExtraBootCode:
		; First we have to load our extra boot code at
		; sector 1 into memory at [0000:7e00h]
		mov  eax,01h
		mov  cx,1
		mov  bx,7e00h							; Read sector to [0000:7e00h]
		call ReadSectors

		jmp  LoadRootDirectory



; Reads ext2 group descriptor into [7000:8000]
; We read it to this arbitrary location so
; it will not cross a 64k boundary
; EAX has group descriptor number to read
Ext2ReadGroupDesc:
		shl   eax,5										; Group = (Group * sizeof(GROUP_DESCRIPTOR) /* 32 */)
		xor   edx,edx
		div   DWORD [BYTE bp+Ext2GroupDescPerBlock]		; Group = (Group / Ext2GroupDescPerBlock)
		add   eax,DWORD [BYTE bp+Ext2FirstDataBlock]	; Group = Group + Ext2FirstDataBlock + 1
		inc   eax										; EAX now has the group descriptor block number
														; EDX now has the group descriptor offset in the block

		; Adjust the read offset so that the
		; group descriptor is read to 7000:8000
		mov   ebx,78000h
		sub   ebx,edx
		shr   ebx,4
		mov   es,bx
		xor   bx,bx


		; Everything is now setup to call Ext2ReadBlock
		; Instead of using the call instruction we will
		; just put Ext2ReadBlock right after this routine

; Reads ext2 block into [ES:BX]
; EAX has logical block number to read
Ext2ReadBlock:
		mov   ecx,DWORD [BYTE bp+Ext2BlockSize]
		mul   ecx
		jmp   ReadSectors

; Reads ext2 inode into [6000:8000]
; We read it to this arbitrary location so
; it will not cross a 64k boundary
; EAX has inode number to read
Ext2ReadInode:
		dec   eax										; Inode = Inode - 1
		xor   edx,edx
		div   DWORD [BYTE bp+Ext2InodesPerGroup]		; Inode = (Inode / Ext2InodesPerGroup)
		mov   ebx,eax									; EBX now has the inode group number
		mov   eax,edx
		xor   edx,edx
		div   DWORD [BYTE bp+Ext2InodesPerBlock]		; Inode = (Inode / Ext2InodesPerBlock)
		shl   edx,7										; FIXME: InodeOffset *= 128 (make the array index a byte offset)
														; EAX now has the inode offset block number from inode table
														; EDX now has the inode offset in the block

		; Save the inode values and put the group
		; descriptor number in EAX and read it in
		push  edx
		push  eax
		mov   eax,ebx
		call  Ext2ReadGroupDesc

		; Group descriptor has been read, now
		; grab the inode table block number from it
		push  WORD 7000h
		pop   es
		mov   di,8008h
		pop   eax										; Restore inode offset block number from stack
		add   eax,DWORD [es:di]							; Add the inode table start block

		; Adjust the read offset so that the
		; inode we want is read to 6000:8000
		pop   edx										; Restore inode offset in the block from stack
		mov   ebx,68000h
		sub   ebx,edx
		shr   ebx,4
		mov   es,bx
		xor   bx,bx

		call  Ext2ReadBlock
		ret


; Reads logical sectors into [ES:BX]
; EAX has logical sector number to read
; CX has number of sectors to read
ReadSectors:
        add  eax,DWORD [BYTE bp+Ext2VolumeStartSector]	; Add the start of the volume
		cmp  eax,DWORD [BYTE bp+BiosCHSDriveSize]		; Check if they are reading a sector outside CHS range
		jae  ReadSectorsLBA						; Yes - go to the LBA routine
												; If at all possible we want to use LBA routines because
												; They are optimized to read more than 1 sector per read

		pushad									; Save logical sector number & sector count

CheckInt13hExtensions:							; Now check if this computer supports extended reads
		mov  ah,0x41							; AH = 41h
		mov  bx,0x55aa							; BX = 55AAh
		mov  dl,[BYTE bp+BootDrive]				; DL = drive (80h-FFh)
		int  13h								; IBM/MS INT 13 Extensions - INSTALLATION CHECK
		jc   ReadSectorsCHS						; CF set on error (extensions not supported)
		cmp  bx,0xaa55							; BX = AA55h if installed
		jne  ReadSectorsCHS
		test cl,1								; CX = API subset support bitmap
		jz   ReadSectorsCHS						; Bit 0, extended disk access functions (AH=42h-44h,47h,48h) supported

		popad									; Restore sector count & logical sector number

ReadSectorsLBA:
		pushad									; Save logical sector number & sector count

		cmp  cx,64								; Since the LBA calls only support 0x7F sectors at a time we will limit ourselves to 64
		jbe  ReadSectorsSetupDiskAddressPacket	; If we are reading less than 65 sectors then just do the read
		mov  cx,64								; Otherwise read only 64 sectors on this loop iteration

ReadSectorsSetupDiskAddressPacket:
		mov  [BYTE bp+LBASectorsRead],cx
		o32 push byte 0
		push eax								; Put 64-bit logical block address on stack
		push es									; Put transfer segment on stack
		push bx									; Put transfer offset on stack
		push cx									; Set transfer count
		push byte 0x10							; Set size of packet to 10h
		mov  si,sp								; Setup disk address packet on stack


        mov  dl,[BYTE bp+BootDrive]				; Drive number
		mov  ah,42h								; Int 13h, AH = 42h - Extended Read
		int  13h								; Call BIOS
		jc   PrintDiskError						; If the read failed then abort

		add  sp,byte 0x10						; Remove disk address packet from stack

		popad									; Restore sector count & logical sector number

		push bx
		mov  ebx,DWORD [BYTE bp+LBASectorsRead]
        add  eax,ebx							; Increment sector to read
		shl  ebx,5
        mov  dx,es
        add  dx,bx								; Setup read buffer for next sector
        mov  es,dx
		pop  bx

		sub  cx,[BYTE bp+LBASectorsRead]
        jnz  ReadSectorsLBA						; Read next sector

        ret


; Reads logical sectors into [ES:BX]
; EAX has logical sector number to read
; CX has number of sectors to read
ReadSectorsCHS:
		popad										; Get logical sector number & sector count off stack

ReadSectorsCHSLoop:
        pushad
        xor   edx,edx
		movzx ecx,WORD [BYTE bp+SectorsPerTrack]
		div   ecx									; Divide logical by SectorsPerTrack
        inc   dl									; Sectors numbering starts at 1 not 0
		mov   cl,dl									; Sector in CL
		mov   edx,eax
		shr   edx,16
        div   WORD [BYTE bp+NumberOfHeads]			; Divide logical by number of heads
        mov   dh,dl									; Head in DH
        mov   dl,[BYTE bp+BootDrive]				; Drive number in DL
        mov   ch,al									; Cylinder in CX
        ror   ah,1									; Low 8 bits of cylinder in CH, high 2 bits
        ror   ah,1									;  in CL shifted to bits 6 & 7
        or    cl,ah									; Or with sector number
        mov   ax,0201h
        int   13h    ; DISK - READ SECTORS INTO MEMORY
                     ; AL = number of sectors to read, CH = track, CL = sector
                     ; DH = head, DL    = drive, ES:BX -> buffer to fill
                     ; Return: CF set on error, AH =    status (see AH=01h), AL    = number of sectors read

        jc    PrintDiskError						; If the read failed then abort

        popad

        inc   eax									; Increment Sector to Read

        mov   dx,es
        add   dx,byte 20h							; Increment read buffer for next sector
        mov   es,dx

        loop  ReadSectorsCHSLoop					; Read next sector

        ret   




; Displays a disk error message
; And reboots
PrintDiskError:
        mov  si,msgDiskError			; Bad boot disk message
        call PutChars					; Display it

Reboot:
        mov  si,msgAnyKey				; Press any key message
        call PutChars					; Display it
        xor ax,ax       
        int 16h							; Wait for a keypress
        int 19h							; Reboot

PutChars:
        lodsb
        or al,al
        jz short Done
		call PutCharsCallBios
        jmp short PutChars
PutCharsCallBios:
        mov ah,0eh
        mov bx,07h
        int 10h
		retn
Done:
		mov al,0dh
		call PutCharsCallBios
		mov al,0ah
		call PutCharsCallBios
        retn



msgDiskError		db 'Disk error',0
msgAnyKey			db 'Press any key to restart',0

        times 510-($-$$) db 0   ; Pad to 510 bytes
        dw 0aa55h       ; BootSector signature
        

; End of bootsector
;
; Now starts the extra boot code that we will store
; at sector 1 on a EXT2 volume



LoadRootDirectory:

		mov  eax,EXT2_ROOT_INO			; Put the root directory inode number in EAX
		call Ext2ReadInode				; Read in the inode

		; Point ES:DI to the inode structure at 6000:8000
		push WORD 6000h
		pop  es
		mov  di,8000h
		push di
		push es							; Save these for later

		; Get root directory size from inode structure
		mov  eax,DWORD [es:di+4]
		push eax

		; Now that the inode has been read in load
		; the root directory file data to 0000:8000
		call Ext2ReadEntireFile

		; Since the root directory was loaded to 0000:8000
		; then add 8000h to the root directory's size
		pop  eax
		mov  edx,8000h					; Set EDX to the current offset in the root directory
		add  eax,edx					; Initially add 8000h to the size of the root directory

SearchRootDirectory:
		push edx						; Save current offset in root directory
		push eax						; Save the size of the root directory

		; Now we have to convert the current offset
		; in the root directory to a SEGMENT:OFFSET pair
		mov  eax,edx
		xor  edx,edx
		mov  ecx,16
		div  ecx						; Now AX:DX has segment & offset
		mov  es,ax
		mov  di,dx
		push di							; Save the start of the directory entry
		add  di,byte 8					; Add the offset to the filename
		mov  si,filename
		mov  cl,11
		rep  cmpsb						; Compare the file names
		pop  di
		pop  eax
		pop  edx
		jz   FoundFile

		; Nope, didn't find it in this entry, keep looking
		movzx ecx,WORD [es:di+4]
		add   edx,ecx

		; Check to see if we have reached the
		; end of the root directory
		cmp  edx,eax
		jb   SearchRootDirectory
		jmp  PrintFileNotFound

FoundFile:
		mov  eax,[es:di]				; Get inode number from directory entry
		call Ext2ReadInode				; Read in the inode

		; Point ES:DI to the inode structure at 6000:8000
		pop  es
		pop  di							; These were saved earlier

		mov  cx,[es:di]					; Get the file mode so we can make sure it's a regular file
		and  ch,EXT2_S_IFMT				; Mask off everything but the file type
		cmp  ch,EXT2_S_IFREG			; Make sure it's a regular file
		je   LoadFreeLoader
		jmp  PrintRegFileError

LoadFreeLoader:
        mov  si,msgLoading				; "Loading FreeLoader..." message
        call PutChars					; Display it

		call Ext2ReadEntireFile			; Read freeldr.sys to 0000:8000

        mov  dl,[BYTE bp+BootDrive]
        push byte 0						; We loaded at 0000:8000
        push WORD 8000h					; We will do a far return to 0000:8000h
        retf							; Transfer control to FreeLoader





; Reads ext2 file data into [0000:8000]
; This function assumes that the file's
; inode has been read in to 6000:8000 *and*
; ES:DI points to 6000:8000
; This will load all the blocks up to
; and including the double-indirect pointers.
; This should be sufficient because it
; allows for ~64MB which is much bigger
; than we need for a boot loader.
Ext2ReadEntireFile:

		; Reset the load segment
		mov  WORD [BYTE bp+Ext2ReadEntireFileLoadSegment],800h

		; Now we must calculate how
		; many blocks to read in
		; We will do this by rounding the
		; file size up to the next block
		; size and then dividing by the block size
		mov  eax,DWORD [bp+Ext2BlockSizeInBytes]			; Get the block size in bytes
		push eax
		dec  eax											; Ext2BlockSizeInBytes -= 1
		add  eax,DWORD [es:di+4]							; Add the file size
		xor  edx,edx
		pop  ecx											; Divide by the block size in bytes
		div  ecx											; EAX now contains the number of blocks to load
		push eax

		; Make sure the file size isn't zero
		cmp  eax,0
		jnz  Ext2ReadEntireFile2
		jmp  PrintFileSizeError

Ext2ReadEntireFile2:
		; Save the indirect & double indirect pointers
		mov  edx,DWORD [es:di+0x58]							; Get indirect pointer
		mov  [BYTE bp+Ext2InodeIndirectPointer],edx			; Save indirect pointer
		mov  edx,DWORD [es:di+0x5c]							; Get double indirect pointer
		mov  [BYTE bp+Ext2InodeDoubleIndirectPointer],edx	; Save double indirect pointer

		; Now copy the direct pointers to 7000:0000
		; so that we can call Ext2ReadDirectBlocks
		push ds												; Save DS
		push es
		push WORD 7000h
		pop  es
		pop  ds
		mov  si,8028h
		xor  di,di											; DS:SI = 6000:8028 ES:DI = 7000:0000
		mov  cx,24											; Moving 24 words of data
		rep  movsw
		pop  ds												; Restore DS

		; Now we have all the block pointers in the
		; right location so read them in
		pop  eax											; Restore the total number of blocks in this file
		xor  ecx,ecx										; Set the max count of blocks to read to 12
		mov  cl,12											; which is the number of direct block pointers in the inode
		call Ext2ReadDirectBlockList

		; Check to see if we actually have
		; blocks left to read
		cmp  eax,0
		jz   Ext2ReadEntireFileDone

		; Now we have read all the direct blocks in
		; the inode. So now we have to read the indirect
		; block and read all it's direct blocks
		push eax											; Save the total block count
		mov  eax,DWORD [bp+Ext2InodeIndirectPointer]		; Get the indirect block pointer
		push WORD 7000h
		pop  es
		xor  bx,bx											; Set the load address to 7000:0000
		call Ext2ReadBlock									; Read the block

		; Now we have all the block pointers from the
		; indirect block in the right location so read them in
		pop  eax											; Restore the total block count
		mov  ecx,DWORD [bp+Ext2PointersPerBlock]			; Get the number of block pointers that one block contains
		call Ext2ReadDirectBlockList

		; Check to see if we actually have
		; blocks left to read
		cmp  eax,0
		jz   Ext2ReadEntireFileDone

		; Now we have read all the direct blocks from
		; the inode's indirect block pointer. So now
		; we have to read the double indirect block
		; and read all it's indirect blocks
		; (whew, it's a good thing I don't support triple indirect blocks)
		mov  [bp+Ext2BlocksLeftToRead],eax					; Save the total block count
		mov  eax,DWORD [bp+Ext2InodeDoubleIndirectPointer]	; Get the double indirect block pointer
		push WORD 7800h
		pop  es
		push es												; Save an extra copy of this value on the stack
		xor  bx,bx											; Set the load address to 7000:8000
		call Ext2ReadBlock									; Read the block

		pop  es												; Put 7800h into ES (saved on the stack already)
		xor  di,di

Ext2ReadIndirectBlock:
		mov  eax,DWORD [es:di]								; Get indirect block pointer
		add  di,4											; Update DI for next array index
		push es
		push di

		push WORD 7000h
		pop  es
		xor  bx,bx											; Set the load address to 7000:0000
		call Ext2ReadBlock									; Read the indirect block

		; Now we have all the block pointers from the
		; indirect block in the right location so read them in
		mov  eax,DWORD [bp+Ext2BlocksLeftToRead]			; Restore the total block count
		mov  ecx,DWORD [bp+Ext2PointersPerBlock]			; Get the number of block pointers that one block contains
		call Ext2ReadDirectBlockList
		mov  [bp+Ext2BlocksLeftToRead],eax					; Save the total block count
		pop  di
		pop  es

		; Check to see if we actually have
		; blocks left to read
		cmp  eax,0
		jnz  Ext2ReadIndirectBlock

Ext2ReadEntireFileDone:
		ret

; Reads a maximum number of blocks
; from an array at 7000:0000
; and updates the total count
; ECX contains the max number of blocks to read
; EAX contains the number of blocks left to read
; On return:
;  EAX contians the new number of blocks left to read
Ext2ReadDirectBlockList:
		cmp  eax,ecx										; Compare it to the maximum number of blocks to read
		ja   CallExt2ReadDirectBlocks						; If it will take more blocks then just read all of the blocks
		mov  cx,ax											; Otherwise adjust the block count accordingly

CallExt2ReadDirectBlocks:
		sub  eax,ecx										; Subtract the number of blocks being read from the total count
		push eax											; Save the new total count
		call Ext2ReadDirectBlocks
		pop  eax											; Restore the total count
		ret


; Reads a specified number of blocks
; from an array at 7000:0000
; CX contains the number of blocks to read
Ext2ReadDirectBlocks:

		push WORD 7000h
		pop  es
		xor  di,di											; Set ES:DI = 7000:0000

Ext2ReadDirectBlocksLoop:
		mov  eax,[es:di]									; Get direct block pointer from array
		add  di,4											; Update DI for next array index

		push cx												; Save number of direct blocks left
		push es												; Save array segment
		push di												; Save array offset
		mov  es,[BYTE bp+Ext2ReadEntireFileLoadSegment]
		xor  bx,bx											; Setup load address for next read

		call Ext2ReadBlock									; Read the block (this updates ES for the next read)

		mov  [BYTE bp+Ext2ReadEntireFileLoadSegment],es		; Save updated ES

		pop  di												; Restore the array offset
		pop  es												; Restore the array segment
		pop  cx												; Restore the number of blocks left

		loop Ext2ReadDirectBlocksLoop

		; At this point all the direct blocks should
		; be loaded and ES (Ext2ReadEntireFileLoadSegment)
		; should be ready for the next read.
		ret



; Displays a file not found error message
; And reboots
PrintFileNotFound:
        mov  si,msgFreeLdr      ; FreeLdr not found message
		jmp short DisplayAndRebootIt

; Displays a file size is 0 error
; And reboots
PrintFileSizeError:
        mov  si,msgFileSize     ; Error message
		jmp short DisplayAndRebootIt

; Displays a file is not a regular file error
; And reboots
PrintRegFileError:
        mov  si,msgRegFile      ; Error message
DisplayAndRebootIt:
        call PutChars           ; Display it
		jmp  Reboot

msgFreeLdr   db 'freeldr.sys not found',0
msgFileSize  db 'File size is 0',0
msgRegFile   db 'freeldr.sys isnt a regular file',0
filename     db 'freeldr.sys'
msgLoading   db 'Loading FreeLoader...',0

        times 1022-($-$$) db 0   ; Pad to 1022 bytes

        dw 0aa55h       ; BootSector signature
