;
; Win2k FAT32 Boot Sector
;
; Brian Palmer <brianp@sginet.com>
;

;
; The BP register is initialized to 0x7c00, the start of
; the boot sector. The SP register is initialized to
; 0x7bf4, leaving 12 bytes of data storage space above
; the stack.
;
; The DWORD that gets stored at 0x7bf4 is 0xffffffff ??
;
; The DWORD that gets stored at 0x7bf8 is the count of
; total sectors of the volume, calculated from the BPB.
;
; The DWORD that gets stored at 0x7bfc is the logical
; sector number of the start of the data area.
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
; FAT32 Inserted Info
SectorsPerFatBig	dd	0
ExtendedFlags		dw	0
FSVersion			dw	0
RootDirStartCluster	dd	0
FSInfoSector		dw	0
BackupBootSector	dw	6
Reserved1			times 12 db 0
; End FAT32 Inserted Info
BootDrive			db 80h
Reserved			db 0
ExtendSig			db 29h
SerialNumber		dd 00000000h
VolumeLabel			db 'NO NAME    '
FileSystem			db 'FAT32   '

main:
00007C5A  33C9              xor		cx,cx
00007C5C  8ED1              mov		ss,cx			; Setup the stack
00007C5E  BCF47B            mov		sp,0x7bf4		; Give us 12 bytes of space above the stack
00007C61  8EC1              mov		es,cx
00007C63  8ED9              mov		ds,cx
00007C65  BD007C            mov		bp,0x7c00
00007C68  884E02            mov		[bp+0x2],cl		; Zero out the nop instruction?? (3rd byte of the boot sector)
00007C6B  8A5640            mov		dl,[bp+BootDrive]
00007C6E  B408              mov		ah,0x8
00007C70  CD13              int		0x13			; Int 13, func 8 - Get Drive Parameters
00007C72  7305              jnc		drive_param_ok	; If no error jmp

drive_param_error:
00007C74  B9FFFF            mov		cx,0xffff		; We couldn't determine the drive parameters
00007C77  8AF1              mov		dh,cl			; So just set the CHS to 0xff

drive_param_ok:
00007C79  660FB6C6          movzx	eax,dh			; Store the number of heads in eax
00007C7D  40                inc		ax				; Make it one-based because the bios returns it zero-based
00007C7E  660FB6D1          movzx	edx,cl			; Store the sectors per track in edx
00007C82  80E23F            and		dl,0x3f			; Mask off the cylinder bits
00007C85  F7E2              mul		dx				; Multiply the sectors per track with the heads, result in dx:ax
00007C87  86CD              xchg	cl,ch			; Switch the cylinder with the sectors
00007C89  C0ED06            shr		ch,0x6			; Move the top two cylinder bits down where they should be
00007C8C  41                inc		cx				; Make it one-based because the bios returns it zero-based
00007C8D  660FB7C9          movzx	ecx,cx
00007C91  66F7E1            mul		ecx				; Multiply the cylinders with (heads * sectors) [stored in dx:ax already]
00007C94  668946F8          mov		[bp-0x8],eax	; This value is the number of total sectors on the disk, so save it for later
00007C98  837E1600          cmp		word [bp+TotalSectors],byte +0x0	; Check the old 16-bit value of TotalSectors
00007C9C  7538              jnz		print_ntldr_error_message			; If it is non-zero then exit with an error

00007C9E  837E2A00          cmp		word [bp+FSVersion],byte +0x0		; Check the file system version word
00007CA2  7732              ja		print_ntldr_error_message			; If it is not zero then exit with an error


		;
		; We are now ready to load our second sector of boot code
		; But first, a bit of undocumented information about how
		; Win2k stores it's second sector of boot code.
		;
		; The FAT32 filesystem was designed so that you can store
		; multiple sectors of boot code. The boot sector of a FAT32
		; volume is actually three sectors long. Microsoft extended
		; the BPB so much that you can't fit enough code in the
		; boot sector to make it work. So they extended it. Sector 0
		; is the traditional boot sector, sector 1 is the FSInfo sector,
		; and sector 2 is used to store extra boot code to make up
		; for the lost space the BPB takes.
		;
		; Now this creates an interesting problem. Suppose for example
		; that the user has Win98 and Win2k installed. The Win2k
		; boot sector is stored at sector 0 and the Win98 boot sector is
		; stored as BOOTSECT.DOS on the file system. Now if Win2k were
		; to store it's second sector of boot code in sector 2 like
		; the fat spec says to do then when you try to dual boot back
		; to Win98 the Win98 boot sector will load Win2k's second
		; sector of boot code. Understand? ;-)
		;
		; To get around this problem Win2k stores it's second sector
		; of boot code elsewhere. This sector is always stored at sector 13
		; on the file system. Now don't ask me what happens when you don't
		; have enough reserved sectors to store it, but I've never seen a
		; FAT32 volume that didn't have at least 32 reserved sectors.
		;

00007CA4  668B461C          mov		eax,[bp+HiddenSectors]	; Get the count of hidden sectors
00007CA8  6683C00C          add		eax,byte +0xc			; Add 12 to that value so that we are loading the 13th sector of the volume
00007CAC  BB0080            mov		bx,0x8000				; Read the sector to address 0x8000
00007CAF  B90100            mov		cx,0x1					; Read just one sector
00007CB2  E82B00            call	read_sectors			; Read it
00007CB5  E94803            jmp		0x8000					; Jump to the next sector of boot code

print_disk_error_message:
00007CB8  A0FA7D            mov		al,[DISK_ERR_offset_from_0x7d00]
putchars:
00007CBB  B47D              mov		ah,0x7d
00007CBD  8BF0              mov		si,ax
get_another_char:
00007CBF  AC                lodsb
00007CC0  84C0              test	al,al
00007CC2  7417              jz		reboot
00007CC4  3CFF              cmp		al,0xff
00007CC6  7409              jz		print_reboot_message
00007CC8  B40E              mov		ah,0xe
00007CCA  BB0700            mov		bx,0x7
00007CCD  CD10              int		0x10
00007CCF  EBEE              jmp		short get_another_char
print_reboot_message:
00007CD1  A0FB7D            mov		al,[RESTART_ERR_offset_from_0x7d00]
00007CD4  EBE5              jmp		short putchars
print_ntldr_error_message:
00007CD6  A0F97D            mov		al,[NTLDR_ERR_offset_from_0x7d00]
00007CD9  EBE0              jmp		short putchars
reboot:
00007CDB  98                cbw
00007CDC  CD16              int		0x16
00007CDE  CD19              int		0x19

read_sectors:
00007CE0  6660              pushad
00007CE2  663B46F8          cmp		eax,[bp-0x8]
00007CE6  0F824A00          jc		near 0x7d34
00007CEA  666A00            o32		push byte +0x0
00007CED  6650              push	eax
00007CEF  06                push	es
00007CF0  53                push	bx
00007CF1  666810000100      push	dword 0x10010
00007CF7  807E0200          cmp		byte [bp+0x2],0x0
00007CFB  0F852000          jnz		near 0x7d1f
00007CFF  B441              mov		ah,0x41
00007D01  BBAA55            mov		bx,0x55aa
00007D04  8A5640            mov		dl,[bp+BootDrive]
00007D07  CD13              int		0x13
00007D09  0F821C00          jc		near 0x7d29
00007D0D  81FB55AA          cmp		bx,0xaa55
00007D11  0F851400          jnz		near 0x7d29
00007D15  F6C101            test	cl,0x1
00007D18  0F840D00          jz		near 0x7d29
00007D1C  FE4602            inc		byte [bp+0x2]
00007D1F  B442              mov		ah,0x42
00007D21  8A5640            mov		dl,[bp+BootDrive]
00007D24  8BF4              mov		si,sp
00007D26  CD13              int		0x13
00007D28  B0F9              mov		al,0xf9
00007D2A  6658              pop		eax
00007D2C  6658              pop		eax
00007D2E  6658              pop		eax
00007D30  6658              pop		eax
00007D32  EB2A              jmp		short 0x7d5e
00007D34  6633D2            xor		edx,edx
00007D37  660FB74E18        movzx	ecx,word [bp+SectorsPerTrack]
00007D3C  66F7F1            div		ecx
00007D3F  FEC2              inc		dl
00007D41  8ACA              mov		cl,dl
00007D43  668BD0            mov		edx,eax
00007D46  66C1EA10          shr		edx,0x10
00007D4A  F7761A            div		word [bp+NumberOfHeads]
00007D4D  86D6              xchg	dl,dh
00007D4F  8A5640            mov		dl,[bp+BootDrive]
00007D52  8AE8              mov		ch,al
00007D54  C0E406            shl		ah,0x6
00007D57  0ACC              or		cl,ah
00007D59  B80102            mov		ax,0x201
00007D5C  CD13              int		0x13
00007D5E  6661              popad
00007D60  0F8254FF          jc		near print_disk_error_message
00007D64  81C30002          add		bx,0x200
00007D68  6640              inc		eax
00007D6A  49                dec		cx
00007D6B  0F8571FF          jnz		near read_sectors
00007D6F  C3                ret

NTLDR		db 'NTLDR      '

filler		times 49	db 0

NTLDR_ERR	db 0dh,0ah,'NTLDR is missing',0ffh
DISK_ERR	db 0dh,0ah,'Disk error',0ffh
RESTART_ERR	db 0dh,0ah,'Press any key to restart',0dh,0ah

more_filler	times 16	db 0

NTLDR_offset_from_0x7d00						db 0
NTLDR_ERR_offset_from_0x7d00					db 0ach
DISK_ERR_offset_from_0x7d00						db 0bfh
RESTART_ERR_offset_from_0x7d00					db 0cch

						dw 0
						dw 0aa55h






;
; And that ends the code that makes up the traditional boot sector
; From here on out is a disassembly of the extra sector of boot
; code required for a FAT32 volume. Win2k stores this code at
; sector 13 on the file system.
;




00008000  660FB64610        movzx eax,byte [bp+NumberOfFats]	; Put the number of fats into eax
00008005  668B4E24          mov ecx,[bp+SectorsPerFatBig]		; Put the count of sectors per fat into ecx
00008009  66F7E1            mul ecx								; Multiply them, edx:eax = (eax * ecx)
0000800C  6603461C          add eax,[bp+HiddenSectors]			; Add the hidden sectors to eax
00008010  660FB7560E        movzx edx,word [bp+ReservedSectors]	; Put the count of reserved sectors into edx
00008015  6603C2            add eax,edx							; Add it to eax
00008018  668946FC          mov [bp-0x4],eax					; eax now contains the start of the data area, so save it for later
0000801C  66C746F4FFFFFFFF  mov dword [bp-0xc],0xffffffff		; Save 0xffffffff for later??
00008024  668B462C          mov eax,[bp+RootDirStartCluster]	; Put the starting cluster of the root directory into eax
00008028  6683F802          cmp eax,byte +0x2					; Check and see if the root directory starts at cluster 2 or above
0000802C  0F82A6FC          jc near print_ntldr_error_message	; If not exit with error
00008030  663DF8FFFF0F      cmp eax,0xffffff8					; Check and see if the root directory start cluster is and end of cluster chain indicator
00008036  0F839CFC          jnc near print_ntldr_error_message	; If so exit with error

search_root_directory_cluster:
0000803A  6650              push eax							; Save root directory start cluster on stack
0000803C  6683E802          sub eax,byte +0x2					; Adjust it because the first two fat entries are unused so the third entry marks the first data area cluster
00008040  660FB65E0D        movzx ebx,byte [bp+SectsPerCluster]	; Put the number of sectors per cluster in ebx
00008045  8BF3              mov si,bx							; Now store it also in si register
00008047  66F7E3            mul ebx								; Multiply sectors per cluster with root directory start cluster
0000804A  660346FC          add eax,[bp-0x4]					; Add the start sector of the data area

read_directory_sector:
0000804E  BB0082            mov bx,0x8200						; We now have the start sector of the root directory, so load it to 0x8200
00008051  8BFB              mov di,bx							; Put the address of the root directory sector in di also
00008053  B90100            mov cx,0x1							; Read one sector
00008056  E887FC            call read_sectors					; Perform the read

check_entry_for_ntldr:
00008059  382D              cmp [di],ch							; Check the first byte of the root directory entry for zero
0000805B  741E              jz ntldr_not_found					; If so then NTLDR is missing so exit with error
0000805D  B10B              mov cl,0xb							; Put the value 11 in cl so we can compare an 11-byte filename
0000805F  56                push si								; Save si (which contains the number of sectors per cluster)
00008060  BE707D            mov si,NTLDR ;0x7d70				; Check and see if "NTLDR" is the first file entry
00008063  F3A6              repe cmpsb							; Do the compare
00008065  5E                pop si								; Restore sectors per cluster into si
00008066  741B              jz ntldr_found						; If we found it then continue, else check next entry
00008068  03F9              add di,cx							; Add 0 to di? the next entry is 0x15 bytes away
0000806A  83C715            add di,byte +0x15					; Add 0x15 to di
0000806D  3BFB              cmp di,bx							; Check to see if we have reached the end of our sector we loaded, read_sectors sets bx = end address of data loaded
0000806F  72E8              jc check_entry_for_ntldr			; If we haven't reached the end then check the next entry
00008071  4E                dec si								; decrement si, si holds the number of sectors per cluster
00008072  75DA              jnz read_directory_sector			; If it's not zero then search the next sector for NTLDR
00008074  6658              pop eax								; If we got here that means we didn't find NTLDR in the previous root directory cluster, so restore eax with the start cluster
00008076  E86500            call get_fat_entry					; Get the next cluster in the fat chain
00008079  72BF              jc search_root_directory_cluster	; If we reached end-of-file marker then don't jump, otherwise continue search

ntldr_not_found:
0000807B  83C404            add sp,byte +0x4
0000807E  E955FC            jmp print_ntldr_error_message

ntldr_load_segment_address	dw	0x2000

ntldr_found:
00008083  83C404            add sp,byte +0x4					; Adjust stack to remove root directory start cluster
00008086  8B7509            mov si,[di+0x9]						; Put start cluster high word in si
00008089  8B7D0F            mov di,[di+0xf]						; Put start cluster low word in di
0000808C  8BC6              mov ax,si							; Put high word in ax
0000808E  66C1E010          shl eax,0x10						; Shift it into position
00008092  8BC7              mov ax,di							; Put low word in ax, now eax contains start cluster of NTLDR
00008094  6683F802          cmp eax,byte +0x2					; Check and see if the start cluster of NTLDR starts at cluster 2 or above
00008098  0F823AFC          jc near print_ntldr_error_message	; If not exit with error
0000809C  663DF8FFFF0F      cmp eax,0xffffff8					; Check and see if the start cluster of NTLDR is and end of cluster chain indicator
000080A2  0F8330FC          jnc near print_ntldr_error_message	; If so exit with error

load_next_ntldr_cluster:
000080A6  6650              push eax							; Save NTLDR start cluster for later
000080A8  6683E802          sub eax,byte +0x2					; Adjust it because the first two fat entries are unused so the third entry marks the first data area cluster
000080AC  660FB64E0D        movzx ecx,byte [bp+SectsPerCluster]	; Put the sectors per cluster into ecx
000080B1  66F7E1            mul ecx								; Multiply sectors per cluster by the start cluster, we now have the logical start sector
000080B4  660346FC          add eax,[bp-0x4]					; Add the start of the data area logical sector
000080B8  BB0000            mov bx,0x0							; Load NTLDR to offset zero
000080BB  06                push es								; Save es
000080BC  8E068180          mov es,[ntldr_load_segment_address]	; Get the segment address to load NTLDR to
000080C0  E81DFC            call read_sectors					; Load the first cluster
000080C3  07                pop es								; Restore es
000080C4  6658              pop eax								; Restore eax to NTLDR start cluster
000080C6  C1EB04            shr bx,0x4							; bx contains the amount of data we transferred, so divide it by 16
000080C9  011E8180          add [ntldr_load_segment_address],bx	; Add that value to the segment
000080CD  E80E00            call get_fat_entry					; Get the next cluster in eax
000080D0  0F830200          jnc near jump_to_ntldr				; If we have reached the end of file then lets get to NTLDR
000080D4  72D0              jc load_next_ntldr_cluster			; If not, then load another cluster

jump_to_ntldr:
000080D6  8A5640            mov dl,[bp+BootDrive]				; Put the boot drive in dl
000080D9  EA00000020        jmp 0x2000:0x0						; Jump to NTLDR

get_fat_entry:
000080DE  66C1E002          shl eax,0x2							; Multiply cluster by 4
000080E2  E81100            call load_fat_sector				; Load the fat sector
000080E5  26668B01          mov eax,[es:bx+di]					; Get the fat entry
000080E9  6625FFFFFF0F      and eax,0xfffffff					; Mask off the most significant 4 bits
000080EF  663DF8FFFF0F      cmp eax,0xffffff8					; Compare it to end of file marker to set the flags correctly
000080F5  C3                ret									; Return to caller

load_fat_sector:
000080F6  BF007E            mov di,0x7e00						; We will load the fat sector to 0x7e00
000080F9  660FB74E0B        movzx ecx,word [bp+SectsPerCluster]	; Get the sectors per cluster
000080FE  6633D2            xor edx,edx							; We will divide (cluster * 4) / sectorspercluster
00008101  66F7F1            div ecx								; eax is already set before we get to this routine
00008104  663B46F4          cmp eax,[bp-0xc]					; Compare eax to 0xffffffff (initially, we set this value later)
00008108  743A              jz load_fat_sector_end				; If it is the same return
0000810A  668946F4          mov [bp-0xc],eax					; Update that value
0000810E  6603461C          add eax,[bp+HiddenSectors]			; Add the hidden sectors
00008112  660FB74E0E        movzx ecx,word [bp+ReservedSectors]	; Add the reserved sectors
00008117  6603C1            add eax,ecx							; To the hidden sectors + the value we computed earlier
0000811A  660FB75E28        movzx ebx,word [bp+ExtendedFlags]	; Get extended flags and put into ebx
0000811F  83E30F            and bx,byte +0xf					; Mask off upper 8 bits
00008122  7416              jz load_fat_sector_into_memory		; If fat is mirrored then skip fat calcs
00008124  3A5E10            cmp bl,[bp+NumberOfFats]			; Compare bl to number of fats
00008127  0F83ABFB          jnc near print_ntldr_error_message	; If bl is bigger than numfats exit with error
0000812B  52                push dx								; Save dx
0000812C  668BC8            mov ecx,eax							; Put the current fat sector offset into ecx
0000812F  668B4624          mov eax,[bp+SectorsPerFatBig]		; Get the number of sectors occupied by one fat
00008133  66F7E3            mul ebx								; Multiplied by the active fat index
00008136  6603C1            add eax,ecx							; Add the current fat sector offset
00008139  5A                pop dx								; Restore dx
load_fat_sector_into_memory:
0000813A  52                push dx								; Save dx, what is so important in dx??
0000813B  8BDF              mov bx,di							; Put 0x7e00 in bx
0000813D  B90100            mov cx,0x1							; Load one sector
00008140  E89DFB            call read_sectors					; Perform the read
00008143  5A                pop dx								; Restore dx
load_fat_sector_end:
00008144  8BDA              mov bx,dx							; Put it into bx, what is this value??
00008146  C3                ret									; Return


00008147  0000              add [bx+si],al
00008149  0000              add [bx+si],al
0000814B  0000              add [bx+si],al
0000814D  0000              add [bx+si],al
0000814F  0000              add [bx+si],al
00008151  0000              add [bx+si],al
00008153  0000              add [bx+si],al
00008155  0000              add [bx+si],al
00008157  0000              add [bx+si],al
00008159  0000              add [bx+si],al
0000815B  0000              add [bx+si],al
0000815D  0000              add [bx+si],al
0000815F  0000              add [bx+si],al
00008161  0000              add [bx+si],al
00008163  0000              add [bx+si],al
00008165  0000              add [bx+si],al
00008167  0000              add [bx+si],al
00008169  0000              add [bx+si],al
0000816B  0000              add [bx+si],al
0000816D  0000              add [bx+si],al
0000816F  0000              add [bx+si],al
00008171  0000              add [bx+si],al
00008173  0000              add [bx+si],al
00008175  0000              add [bx+si],al
00008177  0000              add [bx+si],al
00008179  0000              add [bx+si],al
0000817B  0000              add [bx+si],al
0000817D  0000              add [bx+si],al
0000817F  0000              add [bx+si],al
00008181  0000              add [bx+si],al
00008183  0000              add [bx+si],al
00008185  0000              add [bx+si],al
00008187  0000              add [bx+si],al
00008189  0000              add [bx+si],al
0000818B  0000              add [bx+si],al
0000818D  0000              add [bx+si],al
0000818F  0000              add [bx+si],al
00008191  0000              add [bx+si],al
00008193  0000              add [bx+si],al
00008195  0000              add [bx+si],al
00008197  0000              add [bx+si],al
00008199  0000              add [bx+si],al
0000819B  0000              add [bx+si],al
0000819D  0000              add [bx+si],al
0000819F  0000              add [bx+si],al
000081A1  0000              add [bx+si],al
000081A3  0000              add [bx+si],al
000081A5  0000              add [bx+si],al
000081A7  0000              add [bx+si],al
000081A9  0000              add [bx+si],al
000081AB  0000              add [bx+si],al
000081AD  0000              add [bx+si],al
000081AF  0000              add [bx+si],al
000081B1  0000              add [bx+si],al
000081B3  0000              add [bx+si],al
000081B5  0000              add [bx+si],al
000081B7  0000              add [bx+si],al
000081B9  0000              add [bx+si],al
000081BB  0000              add [bx+si],al
000081BD  0000              add [bx+si],al
000081BF  0000              add [bx+si],al
000081C1  0000              add [bx+si],al
000081C3  0000              add [bx+si],al
000081C5  0000              add [bx+si],al
000081C7  0000              add [bx+si],al
000081C9  0000              add [bx+si],al
000081CB  0000              add [bx+si],al
000081CD  0000              add [bx+si],al
000081CF  0000              add [bx+si],al
000081D1  0000              add [bx+si],al
000081D3  0000              add [bx+si],al
000081D5  0000              add [bx+si],al
000081D7  0000              add [bx+si],al
000081D9  0000              add [bx+si],al
000081DB  0000              add [bx+si],al
000081DD  0000              add [bx+si],al
000081DF  0000              add [bx+si],al
000081E1  0000              add [bx+si],al
000081E3  0000              add [bx+si],al
000081E5  0000              add [bx+si],al
000081E7  0000              add [bx+si],al
000081E9  0000              add [bx+si],al
000081EB  0000              add [bx+si],al
000081ED  0000              add [bx+si],al
000081EF  0000              add [bx+si],al
000081F1  0000              add [bx+si],al
000081F3  0000              add [bx+si],al
000081F5  0000              add [bx+si],al
000081F7  0000              add [bx+si],al
000081F9  0000              add [bx+si],al
000081FB  0000              add [bx+si],al
000081FD  0055AA            add [di-0x56],dl		; We can't forget the infamous boot signature
