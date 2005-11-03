;/*++
;
;Copyright (c) 1998-2001 Klaus P. Gerlicher
;
;Module Name:
;
;    vga_utils.asm
;
;Abstract:
;
;    assembler function for directly programming standard VGA
;
;Environment:
;
;    LINUX 2.2.X
;    Kernel mode only
;
;Author:
;
;    Klaus P. Gerlicher
;    Reactos Port by Eugene Ingerman
;
;Revision History:
;
;    30-Oct-2001:	created
;
;Copyright notice:
;
;  This file may be distributed under the terms of the GNU Public License.
;
;--*/

global _pice_save_current_registers
global _pice_restore_current_registers
global _pice_set_mode_3_80x50
global _pice_set_mode_3_80x25

;****************************************************************************
;* some assign's ************************************************************
;****************************************************************************
%assign VGA_CRT_REGISTERS		24
%assign VGA_ATTRIBUTE_REGISTERS		21
%assign VGA_GRAPHIC_REGISTERS		9
%assign VGA_SEQUENCER_REGISTERS		5
%assign VGA_MISC_REGISTERS		1

%assign VGA_IO_BASE			03c0h
%assign VGA_IO_SIZE			020h

%assign VGA_ATTRIBUTE_INDEX		03c0h
%assign VGA_ATTRIBUTE_DATA_WRITE	03c0h
%assign VGA_ATTRIBUTE_DATA_READ		03c1h
%assign VGA_MISC_DATA_WRITE		03c2h
%assign VGA_SEQUENCER_INDEX		03c4h
%assign VGA_SEQUENCER_DATA		03c5h
%assign VGA_PEL_MASK			03c6h
%assign VGA_PEL_INDEX_READ		03c7h
%assign VGA_PEL_INDEX_WRITE		03c8h
%assign VGA_PEL_DATA			03c9h
%assign VGA_MISC_DATA_READ		03cch
%assign VGA_GRAPHIC_INDEX		03ceh
%assign VGA_GRAPHIC_DATA		03cfh
%assign VGA_CRT_INDEX			03d4h
%assign VGA_CRT_DATA			03d5h
%assign VGA_INPUT_STATUS		03dah

section .data
pice_mode3_80x50_registers:
;	offsets			0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18
.crt:			db	0x5f,0x4f,0x50,0x82,0x55,0x80,0xbf,0x1f,0x00,0x67,0x06,0x07,0x00,0x00,0x00,0x00,0x9c,0x8f,0x8f,0x28,0x1f,0x96,0xb9,0xa3,0xff
.attribute		db	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x08,0x00,0x0f,0x00,0x00
.graphic:		db	0x00,0x00,0x00,0x00,0x00,0x10,0x0e,0x00,0xff
.sequencer:		db	0x03,0x00,0x03,0x00,0x02 ; 9 bits per char
;.sequencer:		db	0x03,0x01,0x03,0x00,0x02 ; 8 bits per char
.misc:			db 	0x67

pice_mode3_80x25_registers:
;	offsets			0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18
.crt:			db	0x5f,0x4f,0x50,0x82,0x55,0x81,0xbf,0x1f,0x00,0x4f,0x0d,0x0e,0x00,0x00,0x30,0xe8,0x9c,0x0e,0x8f,0x28,0x1f,0x96,0xb9,0xa3
.attribute		db	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x0c,0x00,0x0f,0x08,0x00
.graphic:		db	0x00,0x00,0x00,0x00,0x00,0x10,0x0e,0x00,0xff
.sequencer:		db	0x03,0x00,0x03,0x00,0x02
.misc:			db	0x67



section .bss
pice_current_registers:
.crt:			resb	VGA_CRT_REGISTERS
.attribute:		resb	VGA_ATTRIBUTE_REGISTERS
.graphic:		resb	VGA_GRAPHIC_REGISTERS
.sequencer:		resb	VGA_SEQUENCER_REGISTERS
.misc:			resb	VGA_MISC_REGISTERS
			align	4
.colormap:		resd	256

;****************************************************************************
;* pice_save_current_charset ************************************************
;****************************************************************************
section .text
pice_address dd 0xc00a0000
pice_save_current_charset:
			xor	dword ebx, ebx
			call	pice_select_read_plane
			mov	dword ecx, 04000h
			mov	dword esi, [pice_address]
			mov	dword edi, pice_charset_saved
			cld
			rep movsd
			mov	dword ebx, 00100h
			call	pice_select_read_plane
			mov	dword ecx, 04000h
			mov	dword esi, [pice_address]
			mov	dword edi, (pice_charset_saved + 010000h)
			cld
			rep movsd
			mov	dword ebx, 00200h
			call	pice_select_read_plane
			mov	dword ecx, 04000h
			mov	dword esi, [pice_address]
			mov	dword edi, (pice_charset_saved + 020000h)
			cld
			rep movsd
			mov	dword ebx, 00300h
			call	pice_select_read_plane
			mov	dword ecx, 04000h
			mov	dword esi, [pice_address]
			mov	dword edi, (pice_charset_saved + 030000h)
			cld
			rep movsd
.end:			ret



;****************************************************************************
;* pice_restore_current_charset ****************************************************
;****************************************************************************
section .text
pice_restore_current_charset:
			mov	dword ebx, 00100h
			call	pice_select_write_plane
			mov	dword ecx, 04000h
			mov	dword esi, pice_charset_saved
			mov	dword edi, [pice_address]
			cld
			rep movsd
			mov	dword ebx, 00200h
			call	pice_select_write_plane
			mov	dword ecx, 04000h
			mov	dword esi, (pice_charset_saved + 010000h)
			mov	dword edi, [pice_address]
			cld
			rep movsd
			mov	dword ebx, 00400h
			call	pice_select_write_plane
			mov	dword ecx, 04000h
			mov	dword esi, (pice_charset_saved + 020000h)
			mov	dword edi, [pice_address]
			cld
			rep movsd
			mov	dword ebx, 00800h
			call	pice_select_write_plane
			mov	dword ecx, 04000h
			mov	dword esi, (pice_charset_saved + 030000h)
			mov	dword edi, [pice_address]
			cld
			rep movsd
.end:			ret

;****************************************************************************
;* pice_get_crt_registers **************************************************
;****************************************************************************
;* ebx=>  pointer where to store crt registers
;****************************************************************************
section .text
pice_get_crt_registers:
			xor	dword ecx, ecx
.loop:			mov	dword edx, VGA_CRT_INDEX
			mov	byte  al, cl
			out	word  dx, al
			mov	dword edx, VGA_CRT_DATA
			in	byte  al, dx
			mov	byte  [ebx + ecx], al
			inc	dword ecx
			cmp	dword ecx, VGA_CRT_REGISTERS
			jb	.loop
			ret



;****************************************************************************
;* pice_get_attribute_registers ********************************************
;****************************************************************************
;* ebx=>  pointer where to store attribute registers
;****************************************************************************
section .text
pice_get_attribute_registers:
			xor	dword ecx, ecx
.loop:			mov	dword edx, VGA_INPUT_STATUS
			in	byte  al, dx
			mov	dword edx, VGA_ATTRIBUTE_INDEX
			mov	byte  al, cl
			out	word  dx, al
			mov	dword edx, VGA_ATTRIBUTE_DATA_READ
			in	byte  al, dx
			mov	byte  [ebx + ecx], al
			inc	dword ecx
			cmp	dword ecx, VGA_ATTRIBUTE_REGISTERS
			jb	.loop
			ret



;****************************************************************************
;* pice_get_graphic_registers **********************************************
;****************************************************************************
;* ebx=>  pointer where to store graphics registers
;****************************************************************************
section .text
pice_get_graphic_registers:
			xor	dword ecx, ecx
.loop:			mov	dword edx, VGA_GRAPHIC_INDEX
			mov	byte  al, cl
			out	word  dx, al
			mov	dword edx, VGA_GRAPHIC_DATA
			in	byte  al, dx
			mov	byte  [ebx + ecx], al
			inc	dword ecx
			cmp	dword ecx, VGA_GRAPHIC_REGISTERS
			jb	.loop
			ret



;****************************************************************************
;* pice_get_sequencer_registers ********************************************
;****************************************************************************
;* ebx=>  pointer where to store sequencer registers
;****************************************************************************
section .text
pice_get_sequencer_registers:
			xor	dword ecx, ecx
.loop:			mov	dword edx, VGA_SEQUENCER_INDEX
			mov	byte  al, cl
			out	word  dx, al
			mov	dword edx, VGA_SEQUENCER_DATA
			in	byte  al, dx
			mov	byte  [ebx + ecx], al
			inc	dword ecx
			cmp	dword ecx, VGA_SEQUENCER_REGISTERS
			jb	.loop
			ret



;****************************************************************************
;* pice_get_misc_registers *************************************************
;****************************************************************************
;* ebx=>  pointer where to store misc register
;****************************************************************************
section .text
pice_get_misc_registers:
			mov	dword edx, VGA_MISC_DATA_READ
			in	byte  al, dx
			mov	byte  [ebx], al
			ret



;****************************************************************************
;* pice_get_colormap *******************************************************
;****************************************************************************
;* ebx=>  pointer where to store colormap
;****************************************************************************
section .text
pice_get_colormap:
			xor	dword ecx, ecx
			xor	dword eax, eax
			mov	dword edx, VGA_PEL_INDEX_READ
			out	word  dx, al
			mov	dword edx, VGA_PEL_DATA
.loop:			in	byte  al, dx
			shl	dword eax, 8
			in	byte  al, dx
			shl	dword eax, 8
			in	byte  al, dx
			mov	dword [ebx + 4 * ecx], eax
			inc	dword ecx
			test	byte  cl, cl
			jnz	.loop
			ret



;****************************************************************************
;* pice_set_crt_registers **************************************************
;****************************************************************************
;* ebx=>  pointer to stored crt registers
;****************************************************************************
section .text
pice_set_crt_registers:

			;deprotect CRT registers 0 - 7

			mov	dword edx, VGA_CRT_INDEX
			mov	byte  al, 011h
			out	word  dx, al
			mov	dword edx, VGA_CRT_DATA
			in	byte  al, dx
			and	byte  al, 07fh
			out	word  dx, al

			;write to the registers

			xor	dword ecx, ecx
.loop:			mov	dword edx, VGA_CRT_INDEX
			mov	byte  al, cl
			out	word  dx, al
			mov	dword edx, VGA_CRT_DATA
			mov	byte  al, [ebx + ecx]
			out	word  dx, al
			inc	dword ecx
			cmp	dword ecx, VGA_CRT_REGISTERS
			jb	.loop
			ret



;****************************************************************************
;* pice_set_attribute_registers ********************************************
;****************************************************************************
;* ebx=>  pointer to stored attibute registers
;****************************************************************************
section .text
pice_set_attribute_registers:
			xor	dword ecx, ecx
.loop:			mov	dword edx, VGA_INPUT_STATUS
			in	byte  al, dx
			mov	dword edx, VGA_ATTRIBUTE_INDEX
			mov	byte  al, cl
			out	word  dx, al
			mov	dword edx, VGA_ATTRIBUTE_DATA_WRITE
			mov	byte  al, [ebx + ecx]
			out	word  dx, al
			inc	dword ecx
			cmp	dword ecx, VGA_ATTRIBUTE_REGISTERS
			jb	.loop
			ret



;****************************************************************************
;* pice_set_graphic_registers **********************************************
;****************************************************************************
;* ebx=>  pointer to stored graphic registers
;****************************************************************************
section .text
pice_set_graphic_registers:
			xor	dword ecx, ecx
.loop:			mov	dword edx, VGA_GRAPHIC_INDEX
			mov	byte  al, cl
			out	word  dx, al
			mov	dword edx, VGA_GRAPHIC_DATA
			mov	byte  al, [ebx + ecx]
			out	word  dx, al
			inc	dword ecx
			cmp	dword ecx, VGA_GRAPHIC_REGISTERS
			jb	.loop
			ret



;****************************************************************************
;* pice_set_sequencer_registers ********************************************
;****************************************************************************
;* ebx=>  pointer to stored sequencer registers
;****************************************************************************
section .text
pice_set_sequencer_registers:

			;synchronous reset on

			mov	dword edx, VGA_SEQUENCER_INDEX
			xor	dword eax, eax
			out	word  dx, al
			mov	dword edx, VGA_SEQUENCER_DATA
			inc	dword eax
			out	word  dx, al

			;write to the registers

			mov	dword edx, VGA_SEQUENCER_INDEX
			out	word  dx, al
			mov	dword edx, VGA_SEQUENCER_DATA
			mov	byte  al, [ebx + 1]
			or	byte  al, 020h
			out	word  dx, al
			mov	dword ecx, 2
.loop:			mov	dword edx, VGA_SEQUENCER_INDEX
			mov	byte  al, cl
			out	word  dx, al
			mov	dword edx, VGA_SEQUENCER_DATA
			mov	byte  al, [ebx + ecx]
			out	word  dx, al
			inc	dword ecx
			cmp	dword ecx, VGA_SEQUENCER_REGISTERS
			jb	.loop

			;synchronous reset off

			mov	dword edx, VGA_SEQUENCER_INDEX
			xor	dword eax, eax
			out	word  dx, al
			mov	dword edx, VGA_SEQUENCER_DATA
			mov	byte  al, 3
			out	word  dx, al
			ret



;****************************************************************************
;* pice_set_misc_registers *************************************************
;****************************************************************************
;* ebx=>  pointer to stored misc register
;****************************************************************************
section .text
pice_set_misc_registers:
			mov	dword edx, VGA_MISC_DATA_WRITE
			mov	byte  al, [ebx]
			out	word dx, al
			ret



;****************************************************************************
;* pice_set_colormap *******************************************************
;****************************************************************************
;* ebx=>  pointer to stored colormap
;****************************************************************************
section .text
pice_set_colormap:
			xor	dword ecx, ecx
			xor	dword eax, eax
			mov	dword edx, VGA_PEL_INDEX_WRITE
			out	word  dx, al
			mov	dword edx, VGA_PEL_DATA
.loop:			mov	dword eax, [ebx + 4 * ecx]
			rol	dword eax, 16
			out	word  dx, al
			rol	dword eax, 8
			out	word  dx, al
			rol	dword eax, 8
			out	word  dx, al
			inc	dword ecx
			test	byte  cl, cl
			jnz	.loop
			ret



;****************************************************************************
;* pice_screen_on **********************************************************
;****************************************************************************
section .text
pice_screen_on:

			;turn on the screen

			mov	dword edx, VGA_SEQUENCER_INDEX
			mov	byte  al, 1
			out	word  dx, al
			mov	dword edx, VGA_SEQUENCER_DATA
			in	byte  al, dx
			and	byte  al, 0dfh
			out	word  dx, al

			;enable video output

			mov	dword edx, VGA_INPUT_STATUS
			in	byte  al, dx
			mov	dword edx, VGA_ATTRIBUTE_DATA_WRITE
			mov	byte  al, 020h
			out	word  dx, al
			ret

;****************************************************************************
;* pice_select_write_plane *************************************************
;****************************************************************************
;* bl==>  write mode
;* bh==>  write plane
;****************************************************************************
section .text
pice_select_write_plane:
			and	dword ebx, 00f03h

			;enable set/reset = 0

			mov	dword edx, VGA_GRAPHIC_INDEX
			mov	byte  al, 1
			out	word  dx, al
			mov	dword edx, VGA_GRAPHIC_DATA
			xor	dword eax, eax
			out	word  dx, al

			;logical operation = none, rotate = 0

			mov	dword edx, VGA_GRAPHIC_INDEX
			mov	byte  al, 3
			out	word  dx, al
			mov	dword edx, VGA_GRAPHIC_DATA
			xor	dword eax, eax
			out	word  dx, al

			;select write mode

			mov	dword edx, VGA_GRAPHIC_INDEX
			mov	byte  al, 5
			out	word  dx, al
			mov	dword edx, VGA_GRAPHIC_DATA
			in	byte  al, dx
			and	byte  al, 0fch
			or	byte  al, bl
			out	word  dx, al

			;bitmask = 0ffh

			mov	dword edx, VGA_GRAPHIC_INDEX
			mov	byte  al, 8
			out	word  dx, al
			mov	dword edx, VGA_GRAPHIC_DATA
			mov	byte  al, 0ffh
			out	word  dx, al

			;select write plane

			mov	dword edx, VGA_SEQUENCER_INDEX
			mov	byte  al, 2
			out	word  dx, al
			mov	dword edx, VGA_SEQUENCER_DATA
			mov	byte  al, bh
			out	word  dx, al
			ret



;****************************************************************************
;* pice_select_read_plane **************************************************
;****************************************************************************
;* bl==>  read mode
;* bh==>  read plane
;****************************************************************************
section .text
pice_select_read_plane:
			and	dword ebx, 00301h
			shl	byte  bl, 3

			;select read mode

			mov	dword edx, VGA_GRAPHIC_INDEX
			mov	byte  al, 5
			out	word  dx, al
			mov	dword edx, VGA_GRAPHIC_DATA
			in	byte  al, dx
			and	byte  al, 0f7h
			or	byte  al, bl
			out	word  dx, al

			;select read plane

			mov	dword edx, VGA_GRAPHIC_INDEX
			mov	byte  al, 4
			out	word  dx, al
			mov	dword edx, VGA_GRAPHIC_DATA
			mov	byte  al, bh
			out	word  dx, al
			ret



;****************************************************************************
;* pice_save_current_registers **********************************************
;****************************************************************************
section .text
_pice_save_current_registers:
			push esi
			push edi
			push ebx

;			call	pice_save_current_charset

.crt:		mov	dword ebx, pice_current_registers.crt
			call	pice_get_crt_registers

.attribute:	mov	dword ebx, pice_current_registers.attribute
			call	pice_get_attribute_registers

.graphic:	mov	dword ebx, pice_current_registers.graphic
			call	pice_get_graphic_registers

.sequencer:	mov	dword ebx, pice_current_registers.sequencer
			call	pice_get_sequencer_registers

.misc:		mov	dword ebx, pice_current_registers.misc
			call	pice_get_misc_registers

.colormap:	mov	dword ebx, pice_current_registers.colormap
			call	pice_get_colormap

			pop ebx
			pop edi
			pop esi
.end:		ret

;****************************************************************************
;* pice_restore_current_registers *******************************************
;****************************************************************************
section .text
_pice_restore_current_registers:
			push esi
			push edi
			push ebx

;			call	pice_restore_current_charset

.misc:		mov	dword ebx, pice_current_registers.misc
			call	pice_set_misc_registers

.crt:		mov	dword ebx, pice_current_registers.crt
			call	pice_set_crt_registers

.attribute:	mov	dword ebx, pice_current_registers.attribute
			call	pice_set_attribute_registers

.graphic:	mov	dword ebx, pice_current_registers.graphic
			call	pice_set_graphic_registers

.sequencer:	mov	dword ebx, pice_current_registers.sequencer
			call	pice_set_sequencer_registers

.screen_on:	call	pice_screen_on

.colormap:	mov	dword ebx, pice_current_registers.colormap
			call	pice_set_colormap

			pop ebx
			pop edi
			pop esi

.end:		ret


;****************************************************************************
;* pice_set_mode_3_80x50*****************************************************
;****************************************************************************
section .text
_pice_set_mode_3_80x50:
			push esi
			push edi
			push ebx

.crt:		mov	dword ebx, pice_mode3_80x50_registers.crt
			call	pice_set_crt_registers

.attribute:	mov	dword ebx, pice_mode3_80x50_registers.attribute
			call	pice_set_attribute_registers

.graphic:	mov	dword ebx, pice_mode3_80x50_registers.graphic
			call	pice_set_graphic_registers

.sequencer:	mov	dword ebx, pice_mode3_80x50_registers.sequencer
			call	pice_set_sequencer_registers

.misc:		mov	dword ebx, pice_mode3_80x50_registers.misc
			call	pice_set_misc_registers

.screen_on:	call	pice_screen_on

;.colormap:	mov	dword ebx, pice_current_registers.colormap
;			call	pice_set_colormap

			pop ebx
			pop edi
			pop esi

.end:		ret

;****************************************************************************
;* pice_set_mode_3_80x25*****************************************************
;****************************************************************************
section .text
_pice_set_mode_3_80x25:
			push esi
			push edi
			push ebx

.crt:		mov	dword ebx, pice_mode3_80x25_registers.crt
			call	pice_set_crt_registers

.attribute:	mov	dword ebx, pice_mode3_80x25_registers.attribute
			call	pice_set_attribute_registers

.graphic:	mov	dword ebx, pice_mode3_80x25_registers.graphic
			call	pice_set_graphic_registers

.sequencer:	mov	dword ebx, pice_mode3_80x25_registers.sequencer
			call	pice_set_sequencer_registers

.misc:		mov	dword ebx, pice_mode3_80x25_registers.misc
			call	pice_set_misc_registers

.screen_on:	call	pice_screen_on

;.colormap:	mov	dword ebx, pice_current_registers.colormap
;			call	pice_set_colormap

			pop ebx
			pop edi
			pop esi

.end:		ret

;****************************************************************************
;* uninitialized data *******************************************************
;****************************************************************************
section .bss
			alignb	4
pice_charset_saved:	resb	040000h


