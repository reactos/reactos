;
; Loads the kernel and any required modules
;

org 0

;
; Segment where we are loaded
;
LOADSEG equ 02000h

;
; Segment used for temporay storage
;
WORKSEG equ 01000h


KERNELBASE equ 05000h

;
; Offsets of work areas
;
FAT_CHAIN equ 0h

DIR_BUFFER equ 4000h
END_DIR_BUFFER equ 0ffe0h

FAT_SEG equ 03000h


;
; These are all on the stack
;
%define oem                  [bp+3]
%define bytesPerSector       [bp+0bh]
%define sectPerCluster       [bp+0dh]
%define resSectors           [bp+0eh]
%define nFats                [bp+10h]
%define nRootDir             [bp+11h]
%define nSectors             [bp+13h]
%define MID                  [bp+15h]
%define sectPerFat           [bp+16h]
%define sectPerTrack         [bp+18h]
%define nHeads               [bp+1ah]
%define nHidden              [bp+1ch]
%define nHidden_hi           [bp+1eh]
%define nSectorHuge          [bp+20h]
%define drive                [bp+24h]
%define extBoot              [bp+26h]
%define volid                [bp+27h]
%define vollabel             [bp+2bh]
%define filesys              36h

RETRYCOUNT equ 5

%define fat_start       [bp-4]          ; first FAT sector
%define fat_start_hi    [bp-2]
%define root_dir_start  [bp-8]          ; first root directory sector
%define root_dir_start_hi [bp-6]
%define data_start      [bp-12]         ; first data sector
%define data_start_hi   [bp-10]


entry:
        mov     drive,dl
        
        mov     ax,LOADSEG
        mov     ds,ax


        ;
        ; Print out a message
        ;
        mov     di,loadmsg
        call    printmsg


        ;
        ; Check here for shift pressed and if so display boot menu
        ;

        ;
        ; Load the entire fat
        ;
;        mov     ax,fat_start
;        mov     dx,fat_start_hi
;        mov     di,sectPerFat
;        mov     ax,FAT_SEG
;        mov     es,ax
;        mov     bx,0
;        call    readDisk


        ;
        ; Load root directory
        ;
        mov     ax,WORKSEG
        mov     es,ax

        mov     dx,root_dir_start_hi
        mov     ax,root_dir_start
        mov     bx,DIR_BUFFER
        mov     di,nRootDir
        shr     di,4
        mov     di,1
        call    readDisk
        jc      disk_error

        ;
        ; Look for a directory called boot
        ;        
        mov     di,DIR_BUFFER
        cld
        mov     cx,4
l1:
        mov     si,boot_dir_name
;        cmp     byte [di],0
;        je      boot_error
        repe    cmpsb
        je      found_it
        or      di,31
        inc     di
        cmp     di,END_DIR_BUFFER
        jge     boot_error
        jmp     l1


boot_error:
        mov     di,errormsg
        call    printmsg
l3:
        jmp     l3

disk_error:
        mov     di,errormsg1
        call    printmsg
        jmp     l3



found_it:
        mov     di,msg1
        call    printmsg

        ;
        ; Load the boot directory found above
        ;
        sub     di,4
        call    readFile

l2:
        jmp     l2

;
; readFile
;
%define file_length [di+01ch]
%define start_cluster [di+01ah]
readFile:
        cmp     byte  extBoot, 29h
        jne     fat_12
        cmp     byte  [bp+filesys+4], '6'  ; check for FAT-16 system
        je      fat_16

fat_12:
        mov     di,msg2
        call    printmsg
l4:
        jmp l4

fat_16:
        mov     di,msg3
        call    printmsg
        jmp     l4
        
        

;	readDisk:       Reads a number of sectors into memory.
;
;	Call with:      DX:AX = 32-bit DOS sector number
;	                DI = number of sectors to read
;	                ES:BX = destination buffer
;	                ES must be 64k aligned (1000h, 2000h etc).
;
;	Returns:        CF set on error
;	                ES:BX points one byte after the last byte read. 

readDisk:
                push    bp
		push    si
read_next:      push    dx
		push    ax

		;
		; translate sector number to BIOS parameters
		;

		;
		; abs = sector                          offset in track
		;     + head * sectPerTrack             offset in cylinder
		;     + track * sectPerTrack * nHeads   offset in platter
		; 
		; t1     = abs  /  sectPerTrack         (ax has t1)
		; sector = abs mod sectPerTrack         (cx has sector)
		;
                div     word sectPerTrack
		mov     cx, dx

		;
		; t1   = head + track * nHeads
		;
		; track = t1  /  nHeads                 (ax has track)
		; head  = t1 mod nHeads                 (dl has head)
		;
		xor     dx, dx
                div     word nHeads

		; the following manipulations are necessary in order to 
		; properly place parameters into registers.
		; ch = cylinder number low 8 bits
		; cl = 7-6: cylinder high two bits
		;      5-0: sector
		mov     dh, dl                  ; save head into dh for bios
		ror     ah, 1                   ; move track high bits into
		ror     ah, 1                   ; bits 7-6 (assumes top = 0)
		xchg    al, ah                  ; swap for later
                mov     dl, byte sectPerTrack
		sub     dl, cl
		inc     cl                      ; sector offset from 1
		or      cx, ax                  ; merge cylinder into sector
		mov     al, dl                  ; al has # of sectors left

		; Calculate how many sectors can be transfered in this read
		; due to dma boundary conditions.
		push    dx

		mov     si, di                  ; temp register save
		; this computes remaining bytes because of modulo 65536
		; nature of dma boundary condition
		mov     ax, bx                  ; get offset pointer
		neg     ax                      ; and convert to bytes
		jz      ax_min_1                ; started at seg:0, skip ahead

		xor     dx, dx                  ; convert to sectors
                div     word bytesPerSector
		
		cmp     ax, di                  ; check remainder vs. asked
		jb      ax_min_1                ; less, skip ahead
		mov     si, ax                  ; transfer only what we can

ax_min_1:       pop     dx

		; Check that request sectors do not exceed track boundary
		mov     si, sectPerTrack
		inc     si
		mov     ax, cx                  ; get the sector/cyl byte
		and     ax, 03fh                ; and mask out sector
		sub     si, ax                  ; si has how many we can read
		mov     ax, di
		cmp     si, di                  ; see if asked <= available
		jge     ax_min_2
		mov     ax, si                  ; get what can be xfered

ax_min_2:       mov     si, RETRYCOUNT
		mov     ah, 2
		mov     dl, drive

retry:          push    ax
		int     13h
		pop     ax
		jnc     read_ok
		push    ax
		xor     ax, ax          ; reset the drive
		int     13h
		pop     ax
		dec     si
		jnz     retry
		stc
		pop     ax
		pop     dx
		pop     si
                pop     bp
		ret

read_next_jmp:  jmp     short read_next
read_ok:        xor     ah, ah                          
		mov     si, ax                  ; AX = SI = number of sectors read      
                mul     word bytesPerSector ; AX = number of bytes read
		add     bx, ax                  ; add number of bytes read to BX
		jnc     no_incr_es              ; if overflow...

		mov     ax, es       
		add     ah, 10h                 ; ...add 1000h to ES
		mov     es, ax
		
no_incr_es:     pop     ax
		pop     dx                      ; DX:AX = last sector number

		add     ax, si
		adc     dx, 0                   ; DX:AX = next sector to read
		sub     di, si                  ; if there is anything left to read,
		jg      read_next_jmp           ; continue

		clc
		pop     si
                pop     bp
		ret

;
; Print string (DI = start) 
;
printmsg:
        push    ax
        push    bx
        push    di
        mov     ah,0eh
        mov     bh,0
        mov     bl,07h
.l1
        mov     al,[di]
        cmp     al,0
        je      .l2
        inc     di
        int     10h
        jmp     .l1
.l2
        pop     di
        pop     bx
        pop     ax
        ret



loadmsg db "Starting ReactOS...",0xd,0xa,0
boot_dir_name db 'BOOT'
errormsg db "Files missing on boot disk",0
errormsg1 db "Disk read error",0
msg1 db "Found boot directory",0xd,0xa,0
msg2 db 'FAT12',0
msg3 db 'FAT16',0
