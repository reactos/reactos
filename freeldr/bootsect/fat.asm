; FAT.ASM
; FAT12/16 Boot Sector
; Copyright (c) 1998, 2001, 2002 Brian Palmer



; This is a FAT12/16 file system boot sector
; that searches the entire root directory
; for the file freeldr.sys and loads it into
; memory.
;
; The stack is set to 0000:7BF2 so that the first
; WORD pushed will be placed at 0000:7BF0
;
; The DWORD at 0000:7BFC or BP-04h is the logical
; sector number of the start of the data area.
;
; The DWORD at 0000:7BF8 or BP-08h is the total
; sector count of the boot drive as reported by
; the computers bios.
;
; The WORD at 0000:7BF6 or BP-0ah is the offset
; of the ReadSectors function in the boot sector.
;
; The WORD at 0000:7BF4 or BP-0ch is the offset
; of the ReadCluster function in the boot sector.
;
; The WORD at 0000:7BF2 or BP-0eh is the offset
; of the PutChars function in the boot sector.
;
; When it locates freeldr.sys on the disk it will
; load the first sector of the file to 0000:8000
; With the help of this sector we should be able
; to load the entire file off the disk, no matter
; how fragmented it is.
;
; We load the entire FAT table into memory at
; 7000:0000. This improves the speed of floppy disk
; boots dramatically.


BootSectorStackTop		equ		0x7bf2
DataAreaStartHigh		equ		0x2
DataAreaStartLow		equ		0x4
BiosCHSDriveSizeHigh	equ		0x6
BiosCHSDriveSizeLow		equ		0x8
BiosCHSDriveSize		equ		0x8
ReadSectorsOffset		equ		0xa
ReadClusterOffset		equ		0xc
PutCharsOffset			equ		0xe


org 7c00h

segment .text

bits 16

start:
        jmp short main
        nop

OEMName         db 'FrLdr1.0'
BytesPerSector  dw 512
SectsPerCluster db 1
ReservedSectors dw 1
NumberOfFats    db 2
MaxRootEntries  dw 224
TotalSectors    dw 2880
MediaDescriptor db 0f0h
SectorsPerFat   dw 9
SectorsPerTrack dw 18
NumberOfHeads   dw 2
HiddenSectors   dd 0
TotalSectorsBig dd 0
BootDrive       db 0
Reserved        db 0
ExtendSig       db 29h
SerialNumber    dd 00000000h
VolumeLabel     db 'NO NAME    '
FileSystem      db 'FAT12   '

main:
        xor ax,ax
        mov ss,ax
        mov bp,7c00h
        mov sp,BootSectorStackTop				; Setup a stack
        mov ds,ax								; Make DS correct
        mov es,ax								; Make ES correct


		cmp BYTE [BYTE bp+BootDrive],BYTE 0xff	; If they have specified a boot drive then use it
		jne GetDriveParameters

        mov [BYTE bp+BootDrive],dl				; Save the boot drive


GetDriveParameters:
		mov  ah,08h
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
		mov   [BYTE bp-BiosCHSDriveSize],eax


        ; Now we must find our way to the first sector of the root directory
        xor ax,ax
		xor cx,cx
        mov al,[BYTE bp+NumberOfFats]			; Number of fats
        mul WORD [BYTE bp+SectorsPerFat]		; Times sectors per fat
        add ax,WORD [BYTE bp+HiddenSectors] 
        adc dx,WORD [BYTE bp+HiddenSectors+2]	; Add the number of hidden sectors 
        add ax,WORD [BYTE bp+ReservedSectors]	; Add the number of reserved sectors
        adc dx,cx								; Add carry bit
		mov WORD [BYTE bp-DataAreaStartLow],ax	; Save the starting sector of the root directory
		mov WORD [BYTE bp-DataAreaStartHigh],dx	; Save it in the first 4 bytes before the boot sector
		mov si,WORD [BYTE bp+MaxRootEntries]	; Get number of root dir entries in SI
        pusha									; Save 32-bit logical start sector of root dir
        ; DX:AX now has the number of the starting sector of the root directory

        ; Now calculate the size of the root directory
        mov ax,0020h							; Size of dir entry
        mul si									; Times the number of entries
        mov bx,[BYTE bp+BytesPerSector]
        add ax,bx
        dec ax
        div bx									; Divided by the size of a sector
		; AX now has the number of root directory sectors

		add [BYTE bp-DataAreaStartLow],ax		; Add the number of sectors of the root directory to our other value
		adc [BYTE bp-DataAreaStartHigh],cx		; Now the first 4 bytes before the boot sector contain the starting sector of the data area
        popa									; Restore root dir logical sector start to DX:AX

LoadRootDirSector:
        mov  bx,7e0h							; We will load the root directory sector
        mov  es,bx								; Right after the boot sector in memory
        xor  bx,bx								; We will load it to [0000:7e00h]
		xor  cx,cx								; Zero out CX
		inc  cx									; Now increment it to 1, we are reading one sector
		xor  di,di								; Zero out di
		push es									; Save ES because it will get incremented by 20h
		call ReadSectors						; Read the first sector of the root directory
		pop  es									; Restore ES (ES:DI = 07E0:0000)

SearchRootDirSector:
		cmp  [es:di],ch							; If the first byte of the directory entry is zero then we have
		jz   ErrBoot							; reached the end of the directory and FREELDR.SYS is not here so reboot
		pusha									; Save all registers
		mov  cl,0xb								; Put 11 in cl (length of filename in directory entry)
		mov  si,filename						; Put offset of filename string in DS:SI
		repe cmpsb								; Compare this directory entry against 'FREELDR SYS'
		popa									; Restore all the registers
		jz   FoundFreeLoader					; If we found it then jump
		dec  si									; SI holds MaxRootEntries, subtract one
		jz   ErrBoot							; If we are out of root dir entries then reboot
		add  di,BYTE +0x20						; Increment DI by the size of a directory entry
		cmp  di,0200h							; Compare DI to 512 (DI has offset to next dir entry, make sure we haven't gone over one sector)
		jc   SearchRootDirSector				; If DI is less than 512 loop again
		jmp short LoadRootDirSector				; Didn't find FREELDR.SYS in this directory sector, try again

FoundFreeLoader:
		; We found freeldr.sys on the disk
		; so we need to load the first 512
		; bytes of it to 0000:8000
        ; ES:DI has dir entry (ES:DI == 07E0:XXXX)
        mov  ax,WORD [es:di+1ah]				; Get start cluster
		push ax									; Save start cluster
		push WORD 800h							; Put 800h on the stack and load it
		pop  es									; Into ES so that we load the cluster at 0000:8000
		call ReadCluster						; Read the cluster
		pop  ax									; Restore start cluster of FreeLoader

		; Save the addresses of needed functions so
		; the helper code will know where to call them.
		mov  WORD [BYTE bp-ReadSectorsOffset],ReadSectors		; Save the address of ReadSectors
		mov  WORD [BYTE bp-ReadClusterOffset],ReadCluster		; Save the address of ReadCluster
		mov  WORD [BYTE bp-PutCharsOffset],PutChars				; Save the address of PutChars

		; Now AX has start cluster of FreeLoader and we
		; have loaded the helper code in the first 512 bytes
		; of FreeLoader to 0000:8000. Now transfer control
		; to the helper code. Skip the first three bytes
		; because they contain a jump instruction to skip
		; over the helper code in the FreeLoader image.
		;jmp  0000:8003h
		jmp  8003h




; Displays an error message
; And reboots
ErrBoot:
        mov  si,msgFreeLdr      ; FreeLdr not found message
        call PutChars           ; Display it

Reboot:
        mov  si,msgAnyKey       ; Press any key message
        call PutChars           ; Display it
        xor ax,ax       
        int 16h                 ; Wait for a keypress
        int 19h                 ; Reboot

PutChars:
        lodsb
        or al,al
        jz short Done
        mov ah,0eh
        mov bx,07h
        int 10h
        jmp short PutChars
Done:
        retn

; Displays a bad boot message
; And reboots
BadBoot:
        mov  si,msgDiskError    ; Bad boot disk message
        call PutChars           ; Display it

		jmp short Reboot


; Reads cluster number in AX into [ES:0000]
ReadCluster:
		; StartSector = ((Cluster - 2) * SectorsPerCluster) + ReservedSectors + HiddenSectors;
        dec   ax								; Adjust start cluster by 2
        dec   ax								; Because the data area starts on cluster 2
        xor   ch,ch
        mov   cl,BYTE [BYTE bp+SectsPerCluster]
        mul   cx								; Times sectors per cluster
        add   ax,[BYTE bp-DataAreaStartLow]		; Add start of data area
        adc   dx,[BYTE bp-DataAreaStartHigh]	; Now we have DX:AX with the logical start sector of OSLOADER.SYS
        xor   bx,bx								; We will load it to [ES:0000], ES loaded before function call
		mov   cl,BYTE [BYTE bp+SectsPerCluster]
		;call  ReadSectors
		;ret



; Reads logical sectors into [ES:BX]
; DX:AX has logical sector number to read
; CX has number of sectors to read
ReadSectors:
		
		; We can't just check if the start sector is
		; in the BIOS CHS range. We have to check if
		; the start sector + length is in that range.
		pusha
		dec cx
		add ax,cx
		adc dx,byte 0

		cmp dx,WORD [BYTE bp-BiosCHSDriveSizeHigh]	; Check if they are reading a sector within CHS range
		jb ReadSectorsCHS							; Yes - go to the old CHS routine
		cmp ax,WORD [BYTE bp-BiosCHSDriveSizeLow]	; Check if they are reading a sector within CHS range
		jbe ReadSectorsCHS							; Yes - go to the old CHS routine

ReadSectorsLBA:
		popa
		pusha									; Save logical sector number & sector count

		o32 push byte 0
		push dx									; Put 64-bit logical
		push ax									; block address on stack
		push es									; Put transfer segment on stack
		push bx									; Put transfer offset on stack
		push byte 1								; Set transfer count to 1 sector
		push byte 0x10							; Set size of packet to 10h
		mov  si,sp								; Setup disk address packet on stack

; We are so totally out of space here that I am forced to
; comment out this very beautifully written piece of code
; It would have been nice to have had this check...
;CheckInt13hExtensions:							; Now make sure this computer supports extended reads
;		mov  ah,0x41							; AH = 41h
;		mov  bx,0x55aa							; BX = 55AAh
;		mov  dl,[BYTE bp+BootDrive]				; DL = drive (80h-FFh)
;		int  13h								; IBM/MS INT 13 Extensions - INSTALLATION CHECK
;		jc   PrintDiskError						; CF set on error (extensions not supported)
;		cmp  bx,0xaa55							; BX = AA55h if installed
;		jne  PrintDiskError
;		test cl,1								; CX = API subset support bitmap
;		jz   PrintDiskError						; Bit 0, extended disk access functions (AH=42h-44h,47h,48h) supported


												; Good, we're here so the computer supports LBA disk access
												; So finish the extended read
        mov  dl,[BYTE bp+BootDrive]				; Drive number
		mov  ah,42h								; Int 13h, AH = 42h - Extended Read
		int  13h								; Call BIOS
		jc   BadBoot							; If the read failed then abort

		add  sp,byte 0x10						; Remove disk address packet from stack

		popa									; Restore sector count & logical sector number

        inc  ax									; Increment Sector to Read
		adc  dx,byte 0

        mov  dx,es
        add  dx,byte 20h						; Increment read buffer for next sector
        mov  es,dx
												
        loop ReadSectorsLBA						; Read next sector

        ret   


; Reads logical sectors into [ES:BX]
; DX:AX has logical sector number to read
; CX has number of sectors to read
; CarryFlag set on error
ReadSectorsCHS:
		popa
        pusha
        xchg ax,cx
        xchg ax,dx
        xor  dx,dx
        div  WORD [BYTE bp+SectorsPerTrack]
        xchg ax,cx                    
        div  WORD [BYTE bp+SectorsPerTrack]    ; Divide logical by SectorsPerTrack
        inc  dx                        ; Sectors numbering starts at 1 not 0
        xchg cx,dx
        div  WORD [BYTE bp+NumberOfHeads]      ; Number of heads
        mov  dh,dl                     ; Head to DH, drive to DL
        mov  dl,[BYTE bp+BootDrive]            ; Drive number
        mov  ch,al                     ; Cylinder in CX
        ror  ah,1                      ; Low 8 bits of cylinder in CH, high 2 bits
        ror  ah,1                      ;  in CL shifted to bits 6 & 7
        or   cl,ah                     ; Or with sector number
        mov  ax,0201h
        int  13h     ; DISK - READ SECTORS INTO MEMORY
                     ; AL = number of sectors to read, CH = track, CL = sector
                     ; DH = head, DL    = drive, ES:BX -> buffer to fill
                     ; Return: CF set on error, AH =    status (see AH=01h), AL    = number of sectors read

        jc   BadBoot

        popa
        inc  ax       ;Increment Sector to Read
        jnz  NoCarryCHS
        inc  dx


NoCarryCHS:
        push bx
        mov  bx,es
        add  bx,byte 20h
        mov  es,bx
        pop  bx
                                        ; Increment read buffer for next sector
        loop ReadSectorsCHS             ; Read next sector

        ret   


msgDiskError db 'Disk error',0dh,0ah,0
msgFreeLdr   db 'freeldr.sys not found',0dh,0ah,0
; Sorry, need the space...
;msgAnyKey    db 'Press any key to restart',0dh,0ah,0
msgAnyKey    db 'Press any key',0dh,0ah,0
filename     db 'FREELDR SYS'

        times 509-($-$$) db 0   ; Pad to 509 bytes

BootPartition:
		db 0

BootSignature:
        dw 0aa55h       ; BootSector signature
