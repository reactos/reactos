; BTSECT32.ASM
; FAT32 Boot Sector
; Copyright (c) 1998, 2000 Brian Palmer

org 7c00h

segment .text

bits 16

start:
        jmp short main
        nop

OEMName				db 'FreeLDR!'
BytesPerSector		dw 512
SectsPerCluster		db 1
ReservedSectors		dw 1
NumberOfFats		db 2
MaxRootEntries		dw 0			;512 - Always zero for FAT32 volumes
TotalSectors		dw 0			;2880 - Always zero for FAT32 volumes
MediaDescriptor		db 0f8h
SectorsPerFat		dw 0			;9 - Always zero for FAT32 volumes
SectorsPerTrack		dw 18
NumberOfHeads		dw 2
HiddenSectors		dd 0
TotalSectorsBig		dd 0
; FAT32 Inserted Info
SectorsPerFatBig	dd	0
ExtendedFlags		dw	0
FSVersion			dw	0
RootDirStartCluster	dd	0
FSInfoSector		dw	0
BackupBootSector	dw	6
Reserved1			times 12 db 0
; End FAT32 Inserted Info
BootDrive			db 0
Reserved			db 0
ExtendSig			db 29h
SerialNumber		dd 00000000h
VolumeLabel			db 'FreeLoader!'
FileSystem			db 'FAT32   '

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
        jnc Continue
        jmp BadBoot             ; Reset failed...

Continue:
		; First we have to load our extra boot code at
		; sector 14 into memory at [0000:7e00h]
		xor  dx,dx
		mov  ax,0eh
        add  ax,WORD [HiddenSectors] 
        adc  dx,WORD [HiddenSectors+2] ; Add the number of hidden sectors 
		mov  cx,1
        mov  bx,7e0h
        mov  es,bx				; Read sector to [0000:7e00h]
		xor  bx,bx
		call ReadSectors
		jnc  Continue1
		jmp  BadBoot


Continue1:
        ; Now we must get the first cluster of the root directory
		mov  eax,DWORD [RootDirStartCluster]
		cmp  eax,0ffffff8h		; Check to see if this is the last cluster in the chain
		jb	 Continue2			; If not continue, if so BadBoot
		jmp  ErrBoot
Continue2:
        mov  bx,800h
        mov  es,bx				; Read cluster to [0000:8000h]
        call ReadCluster        ; Read the cluster


        ; Now we have to find our way through the root directory to
        ; The OSLOADER.SYS file
		xor  bx,bx
        mov  bl,[SectsPerCluster]
		shl  bx,4				; BX = BX * 512 / 32
        mov  ax,800h            ; We loaded at 0800:0000
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

		; Get the next root dir cluster and try again until we run out of clusters
		mov  eax,DWORD [RootDirStartCluster]
		call GetFatEntry
		mov  [RootDirStartCluster],eax
        jmp  Continue1

FoundFile:
        xor  di,di              ; ES:DI has dir entry
        xor  dx,dx
        mov  ax,WORD [es:di+14h]        ; Get start cluster high word
		shl  eax,16
        mov  ax,WORD [es:di+1ah]        ; Get start cluster low word

        mov  bx,800h
        mov  es,bx

FoundFile2:
		cmp  eax,0ffffff8h		; Check to see if this is the last cluster in the chain
		jae	 FoundFile3			; If so continue, if not then read then next one
		push eax
        xor  bx,bx              ; Load ROSLDR starting at 0000:8000h
		push es
		call ReadCluster
		pop  es

		xor  bx,bx
        mov  bl,[SectsPerCluster]
		shl  bx,5				; BX = BX * 512 / 16
		mov  ax,es				; Increment the load address by
		add  ax,bx				; The size of a cluster
		mov  es,ax

		pop  eax
		push es
		call GetFatEntry		; Get the next entry
		pop  es

        jmp  FoundFile2			; Load the next cluster (if any)

FoundFile3:
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
        

; End of bootsector
;
; Now starts the extra boot code that we will store
; at sector 14 on a FAT32 volume
;
; To remain multi-boot compatible with other operating
; systems we must not overwrite anything other than
; the bootsector which means we will have to use
; a different sector like 14 to store our extra boot code
;
; Note: Win2k uses sector 12 for this purpose


; Returns the FAT entry for a given cluster number
; On entry EAX has cluster number
; On return EAX has FAT entry for that cluster
GetFatEntry:

		shl   eax,2						; EAX = EAX * 4 (since FAT32 entries are 4 bytes)
		mov   ecx,eax					; Save this for later in ECX
		xor   edx,edx
		movzx ebx,WORD [BytesPerSector]
		push  ebx
		div   ebx						; FAT Sector Number = EAX / BytesPerSector
		movzx ebx,WORD [ReservedSectors]
		add   eax,ebx					; FAT Sector Number += ReservedSectors
		movzx ebx,WORD [HiddenSectors]
		add   eax,ebx					; FAT Sector Number += HiddenSectors
		pop   ebx
		dec   ebx
		and   ecx,ebx					; FAT Offset Within Sector = ECX % BytesPerSector
		; EAX holds logical FAT sector number
		; ECX holds FAT entry offset
		push  ecx
		ror   eax,16
		mov   dx,ax
		ror   eax,16
		; DX:AX holds logical FAT sector number
        mov  bx,7000h
        mov  es,bx
        xor  bx,bx              ; We will load it to [7000:0000h]
		mov  cx,1
		call ReadSectors
		jnc  GetFatEntry1
		jmp  BadBoot
GetFatEntry1:
        mov  bx,7000h
        mov  es,bx
		pop  ecx
		mov  eax,DWORD [es:ecx]		; Get FAT entry
		and  eax,0fffffffh			; Mask off reserved bits

		ret


; Reads cluster number in EAX into [ES:0000]
ReadCluster:
		; StartSector = ((Cluster - 2) * SectorsPerCluster) + ReservedSectors + HiddenSectors;

		dec   eax
		dec   eax
		xor   edx,edx
		movzx ebx,BYTE [SectsPerCluster]
		mul   ebx
		push  eax
		xor   edx,edx
		movzx eax,BYTE [NumberOfFats]
		mul   DWORD [SectorsPerFatBig]
		movzx ebx,WORD [ReservedSectors]
		add   eax,ebx
		add   eax,DWORD [HiddenSectors]
		pop   ebx
		add   eax,ebx			; EAX now contains the logical sector number of the cluster
		ror   eax,16
		mov   dx,ax
		ror   eax,16
        xor   bx,bx				; We will load it to [ES:0000], ES loaded before function call
		movzx cx,BYTE [SectsPerCluster]
		call  ReadSectors
		jnc   ReadCluster1
		jmp   BadBoot

ReadCluster1:
		ret

        times 1022-($-$$) db 0   ; Pad to 1022 bytes
        dw 0aa55h       ; BootSector signature
