
;
; The BP register is initialized to 0x7c00, the start of
; the boot sector. The SP register is initialized to
; 0x7bf0, leaving 16 bytes of data storage space above
; the stack.
;
; The DWORD that gets stored at 0x7bfc is the logical
; sector number of the start of the data area.
;
; The DWORD that gets stored at 0x7bf8 is ????????
;
; The DWORD that gets stored at 0x7bf4 is ????????
;
; The DWORD that gets stored at 0x7bf0 is ????????
;


org 7c00h

segment .text

bits 16

start:
        jmp short main
        nop

OEMName				db 'MSWIN4.0'
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
BootDrive			db 80h
Reserved			db 0
ExtendSig			db 29h
SerialNumber		dd 00000000h
VolumeLabel			db 'NO NAME    '
FileSystem			db 'FAT16   '

main:
00007C3E  33C9              xor cx,cx
00007C40  8ED1              mov ss,cx					; Setup stack
00007C42  BCF07B            mov sp,0x7bf0				; Give us 16 bytes (4 dwords) of space above stack
00007C45  8ED9              mov ds,cx
00007C47  B80020            mov ax,0x2000
00007C4A  8EC0              mov es,ax					; Setup ES:0000 == 2000:0000
00007C4C  FC                cld
00007C4D  BD007C            mov bp,0x7c00
00007C50  384E24            cmp [bp+BootDrive],cl		; Compare the boot drive to zero (I think they are testing for a hard disk drive number)
00007C53  7D24              jnl floppy_boot				; Nope, it's a floppy, skip partition table tests
00007C55  8BC1              mov ax,cx					; Move zero to AX
00007C57  99                cwd							; DX:AX now contains zero
00007C58  E83C01            call read_one_sector		; Try to read in the MBR sector
00007C5B  721C              jc floppy_boot				; Read failed, continue
00007C5D  83EB3A            sub bx,byte +0x3a			; BX comes back with 512, make it equal to 454 (offset of partition table in MBR)
00007C60  66A11C7C          mov eax,[HiddenSectors]		; Put HiddenSectors in EAX
find_our_partition:
00007C64  26663B07          cmp eax,[es:bx]				; Compare partition table entry's start sector to HiddenSectors
00007C68  268A57FC          mov dl,[es:bx-0x4]			; Get partition type byte for this entry
00007C6C  7506              jnz next_partition_entry	; If partition start sector != HiddenSectors then skip this entry
00007C6E  80CA02            or dl,0x2					; Set the second bit in partition type?? I guess this makes types 4 & 6 identical
00007C71  885602            mov [bp+0x2],dl				; Save it on top of nop instruction (3rd byte of boot sector)
next_partition_entry:
00007C74  80C310            add bl,0x10					; Add 16 to bl (offset of next entry in partition table)
00007C77  73EB              jnc find_our_partition		; Jump back until we hit the end of the partition table

; We now have our partition type at 0000:7C02
; If the type was 4 or 6 then that byte is 6
; I can't imagine why the boot sector needs to store
; this information, but hopefully I will uncover it
; as I further disassemble this boot sector.


floppy_boot:
00007C79  33C9              xor cx,cx					; Zero out CX
00007C7B  8A4610            mov al,[bp+NumberOfFats]	; Get the number of FATs in AL (usually 2)
00007C7E  98                cbw							; Sign extend it into AX (AX == 2)
00007C7F  F76616            mul word [bp+NumberOfFats]	; Multiply it with NumberOfFats PLUS the low byte of MaxRootEntries!!??
00007C82  03461C            add ax,[bp+HiddenSectors]	; Result is in DX:AX
00007C85  13561E            adc dx,[bp+HiddenSectors+2]	; Add HiddenSectors to DX:AX
00007C88  03460E            add ax,[bp+ReservedSectors]	; Add ReservedSectors to DX:AX
00007C8B  13D1              adc dx,cx					; CX still contains zero
00007C8D  8B7611            mov si,[bp+MaxRootEntries]	; Get MaxRootEntries in SI
00007C90  60                pusha						; Save all registers (right now DX:AX has starting sector of root dir)
00007C91  8946FC            mov [bp-0x4],ax				; Save the starting sector of the root directory
00007C94  8956FE            mov [bp-0x2],dx				; Save it in the first 4 bytes before the boot sector
00007C97  B82000            mov ax,0x20					; AX == 32 (size of a directory entry)
00007C9A  F7E6              mul si						; Multiply it with MaxRootEntries (DX:AX == length in bytes of root directory)
00007C9C  8B5E0B            mov bx,[bp+BytesPerSector]	; Get the BytesPerSector in BX
00007C9F  03C3              add ax,bx					; Add it to AX (what if this addition carries? MS should 'adc dx,0' shouldn't they?)
00007CA1  48                dec ax						; Subtract one (basically rounding up)
00007CA2  F7F3              div bx						; Divide DX:AX (length of root dir in bytes) by the size of a sector
00007CA4  0146FC            add [bp-0x4],ax				; Add the number of sectors of the root directory to our other value
00007CA7  114EFE            adc [bp-0x2],cx				; Now the first 4 bytes before the boot sector contain the starting sector of the data area
00007CAA  61                popa						; Restore all registers (DX:AX has start sector of root dir)
load_root_dir_sector:
00007CAB  BF0000            mov di,0x0					; Zero out di
00007CAE  E8E600            call read_one_sector		; Read the first sector of the root directory
00007CB1  7239              jc print_disk_error_message	; Read failed, print disk error and reboot
search_directory:
00007CB3  26382D            cmp [es:di],ch				; If the first byte of the directory entry is zero then we have reached the end
00007CB6  7417              jz print_ntldr_error_message; of the directory and NTLDR is not here so reboot
00007CB8  60                pusha						; Save all registers
00007CB9  B10B              mov cl,0xb					; Put 11 in cl (length of filename in directory entry)
00007CBB  BEA17D            mov si,NTLDR				; Put offset of filename string in DS:SI
00007CBE  F3A6              repe cmpsb					; Compare this directory entry against 'NTLDR      '
00007CC0  61                popa						; Restore all the registers
00007CC1  7432              jz found_ntldr				; If we found NTLDR then jump
00007CC3  4E                dec si						; SI holds MaxRootEntries, subtract one
00007CC4  7409              jz print_ntldr_error_message; If we are out of root dir entries then reboot
00007CC6  83C720            add di,byte +0x20			; Increment DI by the size of a directory entry
00007CC9  3BFB              cmp di,bx					; Compare DI to BX (DI has offset to next dir entry, BX has address of end of directory sector in memory)
00007CCB  72E6              jc search_directory			; If DI is less than BX loop again
00007CCD  EBDC              jmp short load_root_dir_sector	; Didn't find NTLDR in this directory sector, try again
print_ntldr_error_message:
00007CCF  A0FB7D            mov al,[NTLDR_ERR_offset_from_0x7d00]
putchars:
00007CD2  B47D              mov ah,0x7d
00007CD4  8BF0              mov si,ax
get_another_char:
00007CD6  AC                lodsb
00007CD7  98                cbw
00007CD8  40                inc ax
00007CD9  740C              jz print_reboot_message
00007CDB  48                dec ax
00007CDC  7413              jz reboot
00007CDE  B40E              mov ah,0xe
00007CE0  BB0700            mov bx,0x7
00007CE3  CD10              int 0x10
00007CE5  EBEF              jmp short get_another_char
print_reboot_message:
00007CE7  A0FD7D            mov al,[RESTART_ERR_offset_from_0x7d00]
00007CEA  EBE6              jmp short putchars
print_disk_error_message:
00007CEC  A0FC7D            mov al,[DISK_ERR_offset_from_0x7d00]
00007CEF  EBE1              jmp short putchars
reboot:
00007CF1  CD16              int 0x16
00007CF3  CD19              int 0x19
found_ntldr:
00007CF5  268B551A          mov dx,[es:di+0x1a]			; Get NTLDR start cluster in DX
00007CF9  52                push dx						; Save it on the stack
00007CFA  B001              mov al,0x1					; Read 1 cluster? Or is this one sector?
00007CFC  BB0000            mov bx,0x0					; ES:BX is the load address (2000:0000)
00007CFF  E83B00            call read_cluster			; Do the read
00007D02  72E8              jc print_disk_error_message	; If it failed then reboot
00007D04  5B                pop bx						; Get the start cluster of NTLDR in BX
00007D05  8A5624            mov dl,[bp+BootDrive]		; Get boot drive in DL
00007D08  BE0B7C            mov si,0x7c0b
00007D0B  8BFC              mov di,sp
00007D0D  C746F03D7D        mov word [bp-0x10],read_cluster
00007D12  C746F4297D        mov word [bp-0xc],0x7d29
00007D17  8CD9              mov cx,ds
00007D19  894EF2            mov [bp-0xe],cx
00007D1C  894EF6            mov [bp-0xa],cx
00007D1F  C606967DCB        mov byte [0x7d96],0xcb
00007D24  EA03000020        jmp 0x2000:0x3
00007D29  0FB6C8            movzx cx,al
00007D2C  668B46F8          mov eax,[bp-0x8]
00007D30  6603461C          add eax,[bp+HiddenSectors]
00007D34  668BD0            mov edx,eax
00007D37  66C1EA10          shr edx,0x10
00007D3B  EB5E              jmp short 0x7d9b
read_cluster:
00007D3D  0FB6C8            movzx cx,al
00007D40  4A                dec dx
00007D41  4A                dec dx
00007D42  8A460D            mov al,[bp+SectsPerCluster]
00007D45  32E4              xor ah,ah
00007D47  F7E2              mul dx
00007D49  0346FC            add ax,[bp-0x4]
00007D4C  1356FE            adc dx,[bp-0x2]
00007D4F  EB4A              jmp short 0x7d9b

read_sectors:
00007D51  52                push dx
00007D52  50                push ax
00007D53  06                push es
00007D54  53                push bx
00007D55  6A01              push byte +0x1
00007D57  6A10              push byte +0x10
00007D59  91                xchg ax,cx
00007D5A  8B4618            mov ax,[bp+SectorsPerTrack]
00007D5D  96                xchg ax,si
00007D5E  92                xchg ax,dx
00007D5F  33D2              xor dx,dx
00007D61  F7F6              div si
00007D63  91                xchg ax,cx
00007D64  F7F6              div si
00007D66  42                inc dx
00007D67  87CA              xchg cx,dx
00007D69  F7761A            div word [bp+NumberOfHeads]
00007D6C  8AF2              mov dh,dl
00007D6E  8AE8              mov ch,al
00007D70  C0CC02            ror ah,0x2
00007D73  0ACC              or cl,ah
00007D75  B80102            mov ax,0x201
00007D78  807E020E          cmp byte [bp+0x2],0xe
00007D7C  7504              jnz 0x7d82
00007D7E  B442              mov ah,0x42
00007D80  8BF4              mov si,sp
00007D82  8A5624            mov dl,[bp+BootDrive]
00007D85  CD13              int 0x13
00007D87  61                popa
00007D88  61                popa
00007D89  720B              jc 0x7d96
00007D8B  40                inc ax
00007D8C  7501              jnz 0x7d8f
00007D8E  42                inc dx
00007D8F  035E0B            add bx,[bp+BytesPerSector]
00007D92  49                dec cx
00007D93  7506              jnz 0x7d9b
00007D95  F8                clc
00007D96  C3                ret

read_one_sector:
00007D97  41                inc cx
00007D98  BB0000            mov bx,0x0
00007D9B  60                pusha
00007D9C  666A00            o32 push byte +0x0
00007D9F  EBB0              jmp short 0x7d51


NTLDR		db 'NTLDR      '


NTLDR_ERR	db 0dh,0ah,'NTLDR is missing',0ffh
DISK_ERR	db 0dh,0ah,'Disk error',0ffh
RESTART_ERR	db 0dh,0ah,'Press any key to restart',0dh,0ah

filler		times 18	db 0


NTLDR_offset_from_0x7d00						db 0
NTLDR_ERR_offset_from_0x7d00					db 0ach
DISK_ERR_offset_from_0x7d00						db 0bfh
RESTART_ERR_offset_from_0x7d00					db 0cch

						dw 0
						dw 0aa55h
