; FAT.ASM
; FAT12/16 Boot Sector
; Copyright (c) 1998, 2001 Brian Palmer



; This is a FAT12/16 file system boot sector
; that searches the entire root directory
; for the file freeldr.sys and loads it into
; memory.
;
; The stack is set to 0000:7C00 so that the first
; DWORD pushed will be placed at 0000:7BFC
;
; When it locates freeldr.sys on the disk it will
; load the first sector of the file to 0000:7E00
; With the help of this sector we should be able
; to load the entire file off the disk, no matter
; how fragmented it is.
;
; We load the entire FAT table into memory at
; 7000:0000. This improves the speed of floppy disk
; boots dramatically.



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
        cli
        cld
        xor ax,ax
        mov ss,ax
        mov bp,7c00h
        mov sp,bp               ; Setup a stack
        mov ax,cs               ; Setup segment registers
        mov ds,ax               ; Make DS correct
        mov es,ax               ; Make ES correct


        sti                     ; Enable ints now
        mov [BYTE bp+BootDrive],dl      ; Save the boot drive
        xor ax,ax               ; Zero out AX

        ; Reset disk controller
        int 13h         
        jnc Continue1           
        jmp BadBoot             ; Reset failed...

Continue1:
        ; Now we must find our way to the first sector of the root directory
        xor ax,ax
        xor dx,dx
        mov al,[BYTE bp+NumberOfFats]    ; Number of fats
        mul WORD [BYTE bp+SectorsPerFat] ; Times sectors per fat
        add ax,WORD [BYTE bp+HiddenSectors] 
        adc dx,WORD [BYTE bp+HiddenSectors+2] ; Add the number of hidden sectors 
        add ax,WORD [BYTE bp+ReservedSectors] ; Add the number of reserved sectors
        adc dx,byte 0           ; Add carry bit
        push ax                 ; Store it on the stack
        push dx                 ; Save 32-bit logical start sector
        push ax
        push dx                 ; Save it for later use also
        ; DX:AX now has the number of the starting sector of the root directory

        ; Now calculate the size of the root directory
        mov ax,0020h            ; Size of dir entry
        mul WORD [BYTE bp+MaxRootEntries] ; Times the number of entries
        mov bx,[BYTE bp+BytesPerSector]
        add ax,bx
        dec ax
        div bx                  ; Divided by the size of a sector
        ; AX now has the number of root directory sectors

        xchg ax,cx              ; Now CX has number of sectors
        pop  dx
        pop  ax                 ; Restore logical sector start
        push cx                 ; Save number of root dir sectors for later use
        mov  bx,7c0h            ; We will load the root directory 
        add  bx,byte 20h        ; Right after the boot sector in memory
        mov  es,bx
        xor  bx,bx              ; We will load it to [0000:7e00h]
        call ReadSectors        ; Read the sectors


        ; Now we have to find our way through the root directory to
        ; The OSLOADER.SYS file
        mov  bx,[BYTE bp+MaxRootEntries]; Search entire root directory
        mov  ax,7e0h            ; We loaded at 07e0:0000
        mov  es,ax
        xor  di,di
        mov  si,filename
        mov  cx,11
        rep  cmpsb              ; Compare filenames
        jz   FoundFile          ; If same we found it
        dec  bx
        jnz  FindFile
        jmp  ErrBoot

FindFile:
        mov  ax,es              ; We didn't find it in the previous dir entry
        add  ax,byte 2          ; So lets move to the next one
        mov  es,ax              ; And search again
        xor  di,di
        mov  si,filename        
        mov  cx,11
        rep  cmpsb              ; Compare filenames
        jz   FoundFile          ; If same we found it
        dec  bx                 ; Keep searching till we run out of dir entries
        jnz  FindFile           ; Last entry?
        jmp  ErrBoot

FoundFile:
		; We found freeldr.sys on the disk
		; so we need to load the first 512
		; bytes of it to 0000:7E00
        xor  di,di              ; ES:DI has dir entry
        xor  dx,dx
        mov  ax,WORD [es:di+1ah]; Get start cluster
        dec  ax					; Adjust start cluster by 2
        dec  ax					; Because the data area starts on cluster 2
        xor  ch,ch
        mov  cl,BYTE [BYTE bp+SectsPerCluster] ; Times sectors per cluster
        mul  cx
        pop  cx                 ; Get number of sectors for root dir
        add  ax,cx				; Add it to the start sector of freeldr.sys
        adc  dx,byte 0
        pop  cx                 ; Get logical start sector of
        pop  bx                 ; Root directory
        add  ax,bx              ; Now we have DX:AX with the logical start
        adc  dx,cx              ; Sector of OSLOADER.SYS
		mov  cx,1				; We will load 1 sector
        push WORD [es:di+1ah]   ; Save start cluster
        mov  bx,7e0h
        mov  es,bx
        xor  bx,bx
        call ReadSectors        ; Load it
		pop  ax					; Restore start cluster
		jmp  LoadFile



; Reads logical sectors into [ES:BX]
; DX:AX has logical sector number to read
; CX has number of sectors to read
; CarryFlag set on error
ReadSectors:
        push ax
        push dx
        push cx
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

        pop  cx
        pop  dx
        pop  ax
        inc  ax       ;Increment Sector to Read
        jnz  NoCarry
        inc  dx


NoCarry:
        push bx
        mov  bx,es
        add  bx,byte 20h
        mov  es,bx
        pop  bx
                                        ; Increment read buffer for next sector
        loop ReadSectors                ; Read next sector

        ret   



; Displays a bad boot message
; And reboots
BadBoot:
        mov  si,msgDiskError    ; Bad boot disk message
        call PutChars           ; Display it
        mov  si,msgAnyKey       ; Press any key message
        call PutChars           ; Display it

		jmp  Reboot

; Displays an error message
; And reboots
ErrBoot:
        mov  si,msgFreeLdr      ; FreeLdr not found message
        call PutChars           ; Display it
        mov  si,msgAnyKey       ; Press any key message
        call PutChars           ; Display it

Reboot:
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

msgDiskError db 'Disk error',0dh,0ah,0
msgFreeLdr   db 'FREELDR.SYS not found',0dh,0ah,0
msgAnyKey    db 'Press any key to restart',0dh,0ah,0
filename     db 'FREELDR SYS'

        times 510-($-$$) db 0   ; Pad to 510 bytes
        dw 0aa55h       ; BootSector signature
        
        

; End of bootsector
;
; Now starts the extra boot code that we will store
; in the first 512 bytes of freeldr.sys



LoadFile:

		push ax							; First save AX - the start cluster of freeldr.sys


		; Lets save the contents of the screen
		; from B800:0000 to 9000:8000
		push ds
		mov  ax,0b800h
		mov  ds,ax
		xor  si,si
		mov  ax,9800h
		mov  es,ax
		xor  di,di
		mov  cx,2000					; Copy 2000 characters [words] (screen is 80x25)
		rep  movsw						; 2 bytes a character (one is the attribute byte)
		pop  ds

		mov  ah,03h						; AH = 03h
		xor  bx,bx						; BH = video page
		int  10h						; BIOS Int 10h Func 3 - Read Cursor Position and Size
		mov  [es:di],dx					; DH = row, DL = column

		; Display "Loading FreeLoader..." message
        mov  si,msgLoading				; Loading message
        call PutChars					; Display it


		pop  ax							; Restore AX

		; AX has start cluster of freeldr.sys
		push ax
		call ReadFatIntoMemory
		pop  ax

        mov  bx,7e0h
        mov  es,bx

LoadFile2:
		push ax
		call IsFat12
		pop  ax
		jnc  LoadFile3
		cmp  ax,0ff8h		    ; Check to see if this is the last cluster in the chain
		jmp  LoadFile4
LoadFile3:
		cmp  ax,0fff8h
LoadFile4:
		jae	 LoadFile_Done		; If so continue, if not then read then next one
		push ax
        xor  bx,bx              ; Load ROSLDR starting at 0000:8000h
		push es
		call ReadCluster
		pop  es

		xor  bx,bx
        mov  bl,BYTE [BYTE bp+SectsPerCluster]
		shl  bx,5				; BX = BX * 512 / 16
		mov  ax,es				; Increment the load address by
		add  ax,bx				; The size of a cluster
		mov  es,ax

		call IsFat12
		pop  ax
		push es
		jnc  LoadFile5
		call GetFatEntry12		; Get the next entry
		jmp  LoadFile6
LoadFile5:
		call GetFatEntry16
LoadFile6:
		pop  es

        jmp  LoadFile2			; Load the next cluster (if any)

LoadFile_Done:
        mov  dl,BYTE [BYTE bp+BootDrive]
        xor  ax,ax
        push ax
        mov  ax,8000h
        push ax                 ; We will do a far return to 0000:8000h
        retf                    ; Transfer control to ROSLDR


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
		call  ReadSectors
		ret


; Returns the FAT entry for a given cluster number for 16-bit FAT
; On entry AX has cluster number
; On return AX has FAT entry for that cluster
GetFatEntry16:

		xor   dx,dx
		mov   cx,2						; AX = AX * 2 (since FAT16 entries are 2 bytes)
		mul   cx
		shl   dx,0fh

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


; Reads cluster number in AX into [ES:0000]
ReadCluster:
		; StartSector = ((Cluster - 2) * SectorsPerCluster) + + ReservedSectors + HiddenSectors;

		dec   ax
		dec   ax
		xor   dx,dx
		movzx bx,BYTE [BYTE bp+SectsPerCluster]
		mul   bx
		push  ax
		push  dx
        ; Now calculate the size of the root directory
        mov   ax,0020h            ; Size of dir entry
        mul   WORD [BYTE bp+MaxRootEntries] ; Times the number of entries
        mov   bx,WORD [BYTE bp+BytesPerSector]
        add   ax,bx
        dec   ax
        div   bx                  ; Divided by the size of a sector
		mov   cx,ax
        ; CX now has the number of root directory sectors
		xor   dx,dx
		movzx ax,BYTE [BYTE bp+NumberOfFats]
		mul   WORD [BYTE bp+SectorsPerFat]
		add   ax,WORD [BYTE bp+ReservedSectors]
		adc   dx,byte 0
		add   ax,WORD [BYTE bp+HiddenSectors]
		adc   dx,WORD [BYTE bp+HiddenSectors+2]
		add   ax,cx
		adc   dx,byte 0
		pop   cx
		pop   bx
		add   ax,bx
		adc   dx,cx
        xor   bx,bx				; We will load it to [ES:0000], ES loaded before function call
		movzx cx,BYTE [BYTE bp+SectsPerCluster]
		call  ReadSectors
		ret

; Returns CF = 1 if this is a FAT12 file system
; Otherwise CF = 0 for FAT16
IsFat12:

        ; Now calculate the size of the root directory
        mov   ax,0020h            ; Size of dir entry
        mul   WORD [BYTE bp+MaxRootEntries] ; Times the number of entries
        mov   bx,WORD [BYTE bp+BytesPerSector]
        add   ax,bx				  ; Plus (BytesPerSector - 1)
        dec   ax
        div   bx                  ; Divided by the size of a sector
        ; AX now has the number of root directory sectors

		mov   bx,ax
        ; Now we must find our way to the first sector of the root directory
        xor   ax,ax
        xor   dx,dx
        mov   al,BYTE [BYTE bp+NumberOfFats]    ; Number of fats
        mul   WORD [BYTE bp+SectorsPerFat] ; Times sectors per fat
        add   ax,WORD [BYTE bp+HiddenSectors] 
        adc   dx,WORD [BYTE bp+HiddenSectors+2] ; Add the number of hidden sectors 
        add   ax,[BYTE bp+ReservedSectors]     ; Add the number of reserved sectors
        adc   dx,byte 0               ; Add carry bit
		add   ax,bx
		adc   dx,byte 0				  ; Add carry bit
        ; DX:AX now has the number of the starting sector of the data area

		xor   cx,cx
		mov   bx,WORD [BYTE bp+TotalSectors]
		cmp   bx,byte 0
		jnz   IsFat12_2
		mov   bx,WORD [BYTE bp+TotalSectorsBig]
		mov   cx,WORD [BYTE bp+TotalSectorsBig+2]

		; CX:BX now contains the number of sectors on the volume
IsFat12_2:
		sub   bx,ax				; Subtract data area start sector
		sub   cx,dx				; from total sectors of volume
		mov   ax,bx
		mov   dx,cx

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



        times 998-($-$$) db 0   ; Pad to 998 bytes

msgLoading   db 'Loading FreeLoader...',0dh,0ah,0

        dw 0aa55h       ; BootSector signature
