; BOOTSECT.ASM
; FAT12/16 Boot Sector
; Copyright (c) 1998 Brian Palmer

org 7c00h

segment .text

bits 16

start:
        jmp short main
        nop

OEMName         db 'FreeLDR!'
BytesPerSector  dw 512
SectsPerCluster db 1
ReservedSectors dw 1
NumberOfFats    db 2
MaxRootEntries  dw 512
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
VolumeLabel     db 'FreeLoader!'
FileSystem      db 'FAT12   '

main:
        cli
        cld
        xor ax,ax
        mov ss,ax
        mov sp,7c00h            ; Setup a stack
        mov ax,cs               ; Setup segment registers
        mov ds,ax               ; Make DS correct
        mov es,ax               ; Make ES correct


        sti                     ; Enable ints now
        mov [BootDrive],dl      ; Save the boot drive
        xor ax,ax               ; Zero out AX

        ; Reset disk controller
        int 13h         
        jnc Continue1           
        jmp BadBoot             ; Reset failed...

Continue1:
        ; Now we must find our way to the first sector of the root directory
        xor ax,ax
        xor cx,cx
        mov al,[NumberOfFats]    ; Number of fats
        mul WORD [SectorsPerFat] ; Times sectors per fat
        add ax,WORD [HiddenSectors] 
        adc dx,WORD [HiddenSectors+2] ; Add the number of hidden sectors 
        add ax,[ReservedSectors]     ; Add the number of reserved sectors
        adc dx,cx               ; Add carry bit
        push ax                 ; Store it on the stack
        push dx                 ; Save 32-bit logical start sector
        push ax
        push dx                 ; Save it for later use also
        ; DX:AX now has the number of the starting sector of the root directory

        ; Now calculate the size of the root directory
        mov ax,0020h            ; Size of dir entry
        mul WORD [MaxRootEntries] ; Times the number of entries
        mov bx,[BytesPerSector]
        add ax,bx
        dec ax
        div bx                  ; Divided by the size of a sector
        ; AX now has the number of root directory sectors

        xchg ax,cx              ; Now CX has number of sectors
        pop  dx
        pop  ax                 ; Restore logical sector start
        push cx                 ; Save for later use
        mov  bx,7c0h            ; We will load the root directory 
        add  bx,20h             ; Right after the boot sector in memory
        mov  es,bx
        xor  bx,bx              ; We will load it to [0000:7e00h]
        call ReadSectors        ; Read the sectors
        jnc  Continue2          ; BadBoot on error
        jmp  BadBoot
Continue2:


        ; Now we have to find our way through the root directory to
        ; The OSLOADER.SYS file
        mov  bx,[MaxRootEntries]; Search entire root directory
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
        add  ax,2               ; So lets move to the next one
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
        xor  di,di              ; ES:DI has dir entry
        xor  dx,dx
        mov  ax,WORD [es:di+1ah]        ; Get start cluster
        dec  ax
        dec  ax
        xor  ch,ch
        mov  cl,BYTE [SectsPerCluster] ; Times sectors per cluster
        mul  cx
        pop  cx                 ; Get number of sectors for root dir
        add  ax,cx
        adc  dx,0
        pop  cx                 ; Get logical start sector of
        pop  bx                 ; Root directory
        add  ax,bx              ; Now we have DX:AX with the logical start
        adc  dx,cx              ; Sector of OSLOADER.SYS
        push ax
        push dx
        mov  ax,WORD [es:di+1ch]
        mov  dx,WORD [es:di+1eh]
        mov  bx,[BytesPerSector]
        dec  bx
        add  ax,bx
        adc  dx,0
        div  WORD [BytesPerSector]
        xchg ax,cx              ; Now CX has number of sectors of OSLOADER.SYS
        pop  dx
        pop  ax
        mov  bx,800h
        mov  es,bx
        xor  bx,bx              ; Load ROSLDR at 0000:8000h
        call ReadSectors        ; Load it
        jc   BadBoot
        mov  dl,[BootDrive]
        xor  ax,ax
        push ax
        mov  ax,8000h
        push ax                 ; We will do a far return to 0000:8000h
        retf                    ; Transfer control to ROSLDR



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
        div  WORD [SectorsPerTrack]
        xchg ax,cx                    
        div  WORD [SectorsPerTrack]    ; Divide logical by SectorsPerTrack
        inc  dx                        ; Sectors numbering starts at 1 not 0
        xchg cx,dx
        div  WORD [NumberOfHeads]      ; Number of heads
        mov  dh,dl                     ; Head to DH, drive to DL
        mov  dl,[BootDrive]            ; Drive number
        mov  ch,al                     ; Cylinder in CX
        ror  ah,1                      ; Low 8 bits of cylinder in CH, high 2 bits
        ror  ah,1                      ;  in CL shifted to bits 6 & 7
        or   cl,ah                     ; Or with sector number
        mov  ax,513
        int  13h     ; DISK - READ SECTORS INTO MEMORY
                     ; AL = number of sectors to read, CH = track, CL = sector
                     ; DH = head, DL    = drive, ES:BX -> buffer to fill
                     ; Return: CF set on error, AH =    status (see AH=01h), AL    = number of sectors read
        pop  cx
        pop  dx
        pop  ax
        jc   ReadFail
        inc  ax       ;Increment Sector to Read
        jnz  NoCarry
        inc  dx


NoCarry:
        push bx
        mov  bx,es
        add  bx,20h
        mov  es,bx
        pop  bx
                                        ; Increment read buffer for next sector
        loop ReadSectors                ; Read next sector


ReadFail:
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
msgAnyKey    db 'Press any key to continue.',0dh,0ah,0
filename     db 'FREELDR SYS'

        times 510-($-$$) db 0   ; Pad to 510 bytes
        dw 0aa55h       ; BootSector signature
        
