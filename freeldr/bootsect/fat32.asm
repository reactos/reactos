; FAT32.ASM
; FAT32 Boot Sector
; Copyright (c) 1998, 2000, 2001, 2002 Brian Palmer

org 7c00h

segment .text

bits 16

start:
        jmp short main
        nop

OEMName				db 'FrLdr1.0'
BytesPerSector		dw 512
SectsPerCluster		db 0
ReservedSectors		dw 32
NumberOfFats		db 2
MaxRootEntries		dw 0			; Always zero for FAT32 volumes
TotalSectors		dw 0			; Always zero for FAT32 volumes
MediaDescriptor		db 0f8h
SectorsPerFat		dw 0			; Always zero for FAT32 volumes
SectorsPerTrack		dw 0
NumberOfHeads		dw 0
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
VolumeLabel			db 'NO NAME    '
FileSystem			db 'FAT32   '

main:
        xor ax,ax               ; Setup segment registers
        mov ds,ax               ; Make DS correct
        mov es,ax               ; Make ES correct
        mov ss,ax				; Make SS correct
		mov bp,7c00h
        mov sp,7c00h            ; Setup a stack



		cmp BYTE [BYTE bp+BootDrive],BYTE 0xff	; If they have specified a boot drive then use it
		jne CheckSectorsPerFat

        mov [BYTE bp+BootDrive],dl				; Save the boot drive



CheckSectorsPerFat:
        cmp	WORD [BYTE bp+SectorsPerFat],byte 0x00	; Check the old 16-bit value of SectorsPerFat
		jnz CheckFailed								; If it is non-zero then exit with an error
CheckTotalSectors:									; Check the old 16-bit value of TotalSectors & MaxRootEntries
        cmp	DWORD [BYTE bp+MaxRootEntries],byte 0x00; by comparing the DWORD at offset MaxRootEntries to zero
		jnz CheckFailed								; If it is non-zero then exit with an error
CheckFileSystemVersion:
        cmp WORD [BYTE bp+FSVersion],byte 0x00		; Check the file system version word
		jna GetDriveParameters						; It is zero, so continue
CheckFailed:
        jmp PrintFileSystemError					; If it is not zero then exit with an error


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
		mov   [BiosCHSDriveSize],eax


LoadExtraBootCode:
		; First we have to load our extra boot code at
		; sector 14 into memory at [0000:7e00h]
		mov  eax,0eh
        add  eax,DWORD [BYTE bp+HiddenSectors]	; Add the number of hidden sectors 
		mov  cx,1
        xor  bx,bx
        mov  es,bx								; Read sector to [0000:7e00h]
		mov  bx,7e00h
		call ReadSectors
		jmp  StartSearch



; Reads logical sectors into [ES:BX]
; EAX has logical sector number to read
; CX has number of sectors to read
ReadSectors:
		cmp  eax,DWORD [BiosCHSDriveSize]		; Check if they are reading a sector outside CHS range
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
		mov  [LBASectorsRead],cx
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

		add  sp,byte 0x10							; Remove disk address packet from stack

		popad									; Restore sector count & logical sector number

		push bx
		mov  ebx,DWORD [LBASectorsRead]
        add  eax,ebx							; Increment sector to read
		shl  ebx,5
        mov  dx,es
        add  dx,bx								; Setup read buffer for next sector
        mov  es,dx
		pop  bx

		sub  cx,[LBASectorsRead]
        jnz  ReadSectorsLBA						; Read next sector

        ret

LBASectorsRead:
	dd	0


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

		jmp  Reboot

; Displays a file system error message
; And reboots
PrintFileSystemError:
        mov  si,msgFileSystemError		; FreeLdr not found message
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
        mov ah,0eh
        mov bx,07h
        int 10h
        jmp short PutChars
Done:
        retn



BiosCHSDriveSize dd 0

msgDiskError		db 'Disk error',0dh,0ah,0
msgFileSystemError	db 'File system error',0dh,0ah,0
msgAnyKey			db 'Press any key to restart',0dh,0ah,0

        times 509-($-$$) db 0   ; Pad to 510 bytes

BootPartition:
		db 0

BootSignature:
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



StartSearch:
        ; Now we must get the first cluster of the root directory
		mov  eax,DWORD [BYTE bp+RootDirStartCluster]
		cmp  eax,0ffffff8h		; Check to see if this is the last cluster in the chain
		jb	 ContinueSearch		; If not continue, if so then we didn't find freeldr.sys
		jmp  PrintFileNotFound
ContinueSearch:
        mov  bx,2000h
        mov  es,bx				; Read cluster to [2000:0000h]
        call ReadCluster        ; Read the cluster


        ; Now we have to find our way through the root directory to
        ; The OSLOADER.SYS file
		xor  bx,bx
        mov  bl,[BYTE bp+SectsPerCluster]
		shl  bx,4				; BX = BX * 512 / 32
        mov  ax,2000h            ; We loaded at 2000:0000
        mov  es,ax
        xor  di,di
        mov  si,filename
        mov  cx,11
        rep  cmpsb              ; Compare filenames
        jz   FoundFile          ; If same we found it
        dec  bx
        jnz  FindFile
        jmp  PrintFileNotFound

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
		mov  eax,DWORD [BYTE bp+RootDirStartCluster]
		call GetFatEntry
		mov  [BYTE bp+RootDirStartCluster],eax
        jmp  StartSearch

FoundFile:

										; Display "Loading FreeLoader..." message
        mov  si,msgLoading				; Loading message
        call PutChars					; Display it

        xor  di,di						; ES:DI has dir entry
        xor  dx,dx
        mov  ax,WORD [es:di+14h]        ; Get start cluster high word
		shl  eax,16
        mov  ax,WORD [es:di+1ah]        ; Get start cluster low word

CheckStartCluster:
		cmp  eax,2						; Check and see if the start cluster starts at cluster 2 or above
		jnb  CheckEndCluster			; If so then continue
		jmp  PrintFileSystemError		; If not exit with error
CheckEndCluster:
		cmp  eax,0ffffff8h				; Check and see if the start cluster is and end of cluster chain indicator
		jb   InitializeLoadSegment		; If not then continue
		jmp  PrintFileSystemError		; If so exit with error

InitializeLoadSegment:
        mov  bx,800h
        mov  es,bx

LoadFile:
		cmp  eax,0ffffff8h		; Check to see if this is the last cluster in the chain
		jae	 LoadFileDone		; If so continue, if not then read the next one
		push eax
        xor  bx,bx              ; Load ROSLDR starting at 0000:8000h
		push es
		call ReadCluster
		pop  es

		xor  bx,bx
        mov  bl,[BYTE bp+SectsPerCluster]
		shl  bx,5				; BX = BX * 512 / 16
		mov  ax,es				; Increment the load address by
		add  ax,bx				; The size of a cluster
		mov  es,ax

		pop  eax
		push es
		call GetFatEntry		; Get the next entry
		pop  es

        jmp  LoadFile			; Load the next cluster (if any)

LoadFileDone:
        mov  dl,[BYTE bp+BootDrive]		; Load boot drive into DL
		mov  dh,[BootPartition]			; Load boot partition into DH
        xor  ax,ax
        push ax					; We loaded at 0000:8000
        push WORD 8000h			; We will do a far return to 0000:8000h
        retf                    ; Transfer control to ROSLDR


; Returns the FAT entry for a given cluster number
; On entry EAX has cluster number
; On return EAX has FAT entry for that cluster
GetFatEntry:

		shl   eax,2								; EAX = EAX * 4 (since FAT32 entries are 4 bytes)
		mov   ecx,eax							; Save this for later in ECX
		xor   edx,edx
		movzx ebx,WORD [BYTE bp+BytesPerSector]
		push  ebx
		div   ebx								; FAT Sector Number = EAX / BytesPerSector
		movzx ebx,WORD [BYTE bp+ReservedSectors]
		add   eax,ebx							; FAT Sector Number += ReservedSectors
		mov   ebx,DWORD [BYTE bp+HiddenSectors]
		add   eax,ebx							; FAT Sector Number += HiddenSectors
		pop   ebx
		dec   ebx
		and   ecx,ebx							; FAT Offset Within Sector = ECX % BytesPerSector
												; EAX holds logical FAT sector number
												; ECX holds FAT entry offset

												; Now we have to check the extended flags
												; to see which FAT is the active one
												; and use it, or if they are mirrored then
												; no worries
		movzx ebx,WORD [BYTE bp+ExtendedFlags]	; Get extended flags and put into ebx
		and   bx,0x0f							; Mask off upper 8 bits, now we have active fat in bl
		jz    LoadFatSector						; If fat is mirrored then skip fat calcs
		cmp   bl,[BYTE bp+NumberOfFats]			; Compare bl to number of fats
		jb    GetActiveFatOffset
		jmp   PrintFileSystemError				; If bl is bigger than numfats exit with error
GetActiveFatOffset:
		push  eax								; Save logical FAT sector number
		mov   eax,[BYTE bp+SectorsPerFatBig]	; Get the number of sectors occupied by one fat in eax
		mul   ebx								; Multiplied by the active FAT index we have in ebx
		pop   edx								; Get logical FAT sector number
		add   eax,edx							; Add the current FAT sector offset

LoadFatSector:
		push  ecx
		; EAX holds logical FAT sector number
		; Check if we have already loaded it
		cmp  eax,DWORD [FatSectorInCache]
		je   LoadFatSectorAlreadyLoaded

		mov  DWORD [FatSectorInCache],eax
        mov  bx,7000h
        mov  es,bx
        xor  bx,bx								; We will load it to [7000:0000h]
		mov  cx,1
		call ReadSectors

LoadFatSectorAlreadyLoaded:
        mov  bx,7000h
        mov  es,bx
		pop  ecx
		mov  eax,DWORD [es:ecx]					; Get FAT entry
		and  eax,0fffffffh						; Mask off reserved bits

		ret

FatSectorInCache:								; This variable tells us which sector we currently have in memory
	dd	0ffffffffh								; There is no need to re-read the same sector if we don't have to


; Reads cluster number in EAX into [ES:0000]
ReadCluster:
		; StartSector = ((Cluster - 2) * SectorsPerCluster) + ReservedSectors + HiddenSectors;

		dec   eax
		dec   eax
		xor   edx,edx
		movzx ebx,BYTE [BYTE bp+SectsPerCluster]
		mul   ebx
		push  eax
		xor   edx,edx
		movzx eax,BYTE [BYTE bp+NumberOfFats]
		mul   DWORD [BYTE bp+SectorsPerFatBig]
		movzx ebx,WORD [BYTE bp+ReservedSectors]
		add   eax,ebx
		add   eax,DWORD [BYTE bp+HiddenSectors]
		pop   ebx
		add   eax,ebx			; EAX now contains the logical sector number of the cluster
        xor   bx,bx				; We will load it to [ES:0000], ES loaded before function call
		movzx cx,BYTE [BYTE bp+SectsPerCluster]
		call  ReadSectors
		ret


; Displays a file not found error message
; And reboots
PrintFileNotFound:
        mov  si,msgFreeLdr      ; FreeLdr not found message
        call PutChars           ; Display it
        mov  si,msgAnyKey       ; Press any key message
        call PutChars           ; Display it

		jmp  Reboot

msgFreeLdr   db 'freeldr.sys not found',0dh,0ah,0
filename     db 'FREELDR SYS'
msgLoading   db 'Loading FreeLoader...',0dh,0ah,0


        times 1022-($-$$) db 0   ; Pad to 1022 bytes

        dw 0aa55h       ; BootSector signature
