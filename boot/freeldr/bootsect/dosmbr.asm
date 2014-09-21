;
; normal DOS boot sector
;
; Ported to nasm from FreeDOS fdisk 1.2.0 by:
; Casper Hornstrup (chorns@users.sourceforge.net)
;

                align 2, db 0

  global  _bootnormal_code
_bootnormal_code:

;-----------------------------------------------------------------------
;   ENTRY (copied from freedos bootsector)
;
; IN: DL = boot drive
;OUT: DL = boot drive
;
;-----------------------------------------------------------------------

real_start:     cli
                cld
                xor     ax, ax
                mov     ss, ax          ; initialize stack
                mov     ds, ax
                mov     bp, 0x7c00
                lea     sp, [bp-0x20]
                sti
                
                mov     ax, 0x1FE0
                mov     es, ax
                mov     si, bp
                mov     di, bp
                mov     cx, 0x0100
                rep     movsw

                jmp     word 0x1FE0:0x7c00+ cont-real_start

cont:           mov     ds, ax
                mov     ss, ax
                xor     ax,ax
                mov     es,ax


										 ; search for active partition
                lea di, [bp+0x1be] ; start of partition table
test_next_for_active:				
                test byte [di],0x80
                jne  active_partition_found
                add  di,0x10                    ; next table
                cmp  di, 07c00h+0x1fe; scanned beyond end of table ??
                jb  test_next_for_active

;*****************************************************************				
                call print
                db 'no active partition found',0
				
WAIT_FOR_REBOOT:
                jmp $


;*****************************************************************
trouble_reading_drive:
                call print
                db 'read error while reading drive',0
                jmp WAIT_FOR_REBOOT

;*****************************************************************

invalid_partition_code:
                call print
                db 'partition signature != 55AA',0
			
                jmp WAIT_FOR_REBOOT

								
;*****************************************************************

active_partition_found:
;				call print
;				db 'loading active partition',0

                call read_boot_sector                    
				
                jc  trouble_reading_drive
			
                cmp word [es:0x7c00+0x1fe],0xaa55
                jne invalid_partition_code
			
                jmp word 0x0:0x7c00             ; and jump to boot sector code


;*****************************
; read_boot_sector				
;
; IN: DI--> partition info
;OUT:CARRY
;*****************************

read_boot_sector:
                ;  /* check for LBA support */
		mov bx,0x55aa		
		mov ah,0x41
		int 0x13

		jc  StandardBios    ;  if (regs.b.x != 0xaa55 || (regs.flags & 0x01))
		cmp bx,0xaa55       ;    goto StandardBios;
		jne StandardBios

                              ;  /* if DAP cannot be used, don't use LBA */
                              ;  if ((regs.c.x & 1) == 0)
                              ;    goto StandardBios;
 		test cl,1
 		jz StandardBios
 
        jmp short LBABios


_bios_LBA_address_packet:
	db 0x10
	db 0
	db 4				; read four sectors - why not
	db 0
	dw 0x7c00			; fixed boot address for DOS sector
	dw 0x0000	
_bios_LBA_low  dw 0
_bios_LBA_high dw 0
	dw 0,0


LBABios:
						; copy start address of partition to DAP
	mov ax,[di+8]
	mov [0x7c00+ (_bios_LBA_low-real_start)],ax
	mov ax,[di+8+2]
	mov [0x7c00+ (_bios_LBA_high-real_start)],ax

    mov ax,0x4200		;  regs.a.x = LBA_READ;
    mov si,0x7c00+ (_bios_LBA_address_packet-real_start); regs.si = FP_OFF(&dap);

	int 0x13
	ret

;*****************************************************************
; read disk, using standard BIOS
;
StandardBios:
    mov ax,0x0204			;  regs.a.x = 0x0201;
    mov bx,0x7c00			;  regs.b.x = FP_OFF(buffer);
	mov cx,[di+2]			;      regs.c.x =
          					; ((chs.Cylinder & 0xff) << 8) + ((chs.Cylinder & 0x300) >> 2) +
          					; chs.Sector;
          					; that was easy ;-)
    mov dh,[di+1]			;  regs.d.b.h = chs.Head;
    						;  regs.es = FP_SEG(buffer);
	int 0x13
	ret	
	


;****** PRINT
; prints text after call to this function.

print_1char:        
                xor   bx, bx                   ; video page 0
                mov   ah, 0x0E                 ; else print it
                int   0x10                     ; via TTY mode
print:          pop   si                       ; this is the first character
print1:         lodsb                          ; get token
                push  si                       ; stack up potential return address
                cmp   al, 0                    ; end of string?
                jne   print_1char              ; until done
                ret                            ; and jump to it



		times	0x1fe-$+$$ db 0
		db 0x55,0xaa
		
