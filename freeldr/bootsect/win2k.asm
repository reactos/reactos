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
FileSystem			db 'FAT12   '

main:
00007C5A  33C9              xor		cx,cx
00007C5C  8ED1              mov		ss,cx
00007C5E  BCF47B            mov		sp,0x7bf4
00007C61  8EC1              mov		es,cx
00007C63  8ED9              mov		ds,cx
00007C65  BD007C            mov		bp,0x7c00
00007C68  884E02            mov		[bp+0x2],cl
00007C6B  8A5640            mov		dl,[bp+BootDrive]
00007C6E  B408              mov		ah,0x8
00007C70  CD13              int		0x13			; Int 13, func 8 - Get Drive Parameters
00007C72  7305              jnc		0x7c79			; If no error jmp

00007C74  B9FFFF            mov		cx,0xffff
00007C77  8AF1              mov		dh,cl

00007C79  660FB6C6          movzx	eax,dh
00007C7D  40                inc		ax
00007C7E  660FB6D1          movzx	edx,cl
00007C82  80E23F            and		dl,0x3f
00007C85  F7E2              mul		dx
00007C87  86CD              xchg	cl,ch
00007C89  C0ED06            shr		ch,0x6
00007C8C  41                inc		cx
00007C8D  660FB7C9          movzx	ecx,cx
00007C91  66F7E1            mul		ecx
00007C94  668946F8          mov		[bp-0x8],eax
00007C98  837E1600          cmp		word [bp+TotalSectors],byte +0x0
00007C9C  7538              jnz		print_ntldr_error_message

00007C9E  837E2A00          cmp		word [bp+FSVersion],byte +0x0
00007CA2  7732              ja		print_ntldr_error_message

00007CA4  668B461C          mov		eax,[bp+0x1c]
00007CA8  6683C00C          add		eax,byte +0xc
00007CAC  BB0080            mov		bx,0x8000
00007CAF  B90100            mov		cx,0x1
00007CB2  E82B00            call	read_sectors
00007CB5  E94803            jmp		0x8000

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