;
; Pmode setup stub
; (A20 enable code and PIC reprogram from linux bootsector)
;

;
; Base address of the kernel
;
LOAD_BASE	equ     0200000h

;
; Segment selectors
;
%define KERNEL_CS     (0x8)
%define KERNEL_DS     (0x10)
%define LOADER_CS     (0x18)
%define LOADER_DS     (0x20)

struc multiboot_module
mbm_mod_start:	resd	1
mbm_mod_end:	resd	1
mbm_string:	resd	1
mbm_reserved:	resd	1
endstruc

;
; We are a .com program
;
org 100h

;
; 16 bit code
;
BITS 16

%define NDEBUG 1

%macro	DPRINT	1+
%ifndef	NDEBUG
	jmp	%%end_str

%%str:	db	%1

%%end_str:
	push	di
	push	ds
	push	es
	pop	ds
	mov	di, %%str
	call	print_string
	pop	ds
	pop	di
%endif
%endmacro

entry:
	;;
	;; Load stack
	;;
	cli
	push	ds
	pop	ss
	push	ds
	pop	es
	mov	sp, real_stack_end
	sti

	;;
	;; Setup the 32-bit registers
	;;
	mov	ebx, 0
	mov	eax, 0
	mov	ecx, 0
	mov	edx, 0
	mov	esi, 0
	mov	edi, 0

	;;
	;; Set the position for the first module to be loaded
	;;
	mov	dword [next_load_base], LOAD_BASE

	;;
	;; Setup various variables
	;;
	mov	bx, ds
	movzx	eax, bx
	shl	eax, 4
	add	[gdt_base], eax

	;;
	;; Setup the loader code and data segments
	;;
	mov	eax, 0
	mov	ax, cs
	shl	eax, 4
	mov	[_loader_code_base_0_15], ax
	shr	eax, 16
	mov	byte [_loader_code_base_16_23], al

	mov	eax, 0
	mov	ax, ds
	shl	eax, 4
	mov	[_loader_data_base_0_15], ax
	shr	eax, 16
	mov	byte [_loader_data_base_16_23], al

	;;
	;; load gdt
	;;
	lgdt	[gdt_descr]

	;;
	;; Enable the A20 address line (to allow access to over 1mb)
	;;
	call	empty_8042
	mov	al, 0D1h		; command write
	out	064h, al
	call	empty_8042
	mov	al, 0DFh		; A20 on
	out	060h, al
	call	empty_8042

	;;
	;; Make the argument list into a c string
	;;
	mov	di, 081h
	mov	si, _multiboot_kernel_cmdline
.next_char
	mov	al, [di]
	mov	[si], al
	cmp	byte [di], 0dh
	je	.end_of_command_line
	inc	di
	inc	si
	jmp	.next_char
	
.end_of_command_line:
	mov	byte [di], 0
	mov	byte [si], 0
	mov	[end_cmd_line], di
	
	;;
	;; Make the argument list into a c string
	;;
	mov	di, 081h
.next_char2
	cmp	byte [di], 0
	je	.end_of_command_line2
	cmp	byte [di], ' '
	jne	.not_space
	mov	byte [di], 0
.not_space
	inc	di
	jmp	.next_char2
.end_of_command_line2

	;;
	;; Check if we want to skip the first character
	;;
	cmp	byte [081h], 0
	jne	.first_char_is_zero
	mov	dx, 082h
	jmp	.start_loading
.first_char_is_zero
	mov	dx, 081h

	;;
	;; Check if we have reached the end of the string
	;;
.start_loading
	mov	bx, dx
	cmp	byte [bx], 0
	jne	.more_modules
	jmp	.done_loading

.more_modules
	;;
	;; Process the arguments
	;;
	cmp	byte [di], '/'
	je	.next_module

	;;
	;; Display a message saying we are loading the module
	;;
	mov	di, loading_msg
	call	print_string
	mov	di, dx
	call	print_string

	;;
	;; Save the filename
	;;
	mov	si, di
	mov	edx, 0

	mov	dx, [_multiboot_mods_count]
	shl	dx, 8
	add	dx, _multiboot_module_strings	
	mov	bx, [_multiboot_mods_count]
	imul	bx, bx, multiboot_module_size
	add	bx, _multiboot_modules
	mov	eax, 0
	mov	ax, ds
	shl	eax, 4
	add	eax, edx
	mov	[bx + mbm_string], eax
	
	mov	bx, dx
.copy_next_char
	mov	al, [si]
	mov	[bx], al
	inc	si
	inc	bx
	cmp	al, 0
	jne	.copy_next_char

	;;
	;; Load the file
	;;
	push	di
	mov	dx, di
	call	pe_load_module
	pop	di
	cmp	eax, 0
	jne	.load_success
	jmp	.exit
.load_success:
	mov	ah, 02h
	mov	dl, 0dh
	int	021h
	mov	ah, 02h
	mov	dl, 0ah
	int	021h

	;;
	;; Move onto the next module name in the command line
	;;
.next_module
	cmp	di, [end_cmd_line]
	je	.done_loading
	cmp	byte [di], 0
	je	.found_module_name
	inc	di
	jmp	.next_module
.found_module_name
	inc	di
	mov	dx, di
	jmp	.start_loading

.done_loading:

	;;
	;; Initialize the multiboot information
	;;
	mov	eax, 0
	mov	ax, ds
	shl	eax, 4
	
	mov	[_multiboot_info_base], eax
	add	dword [_multiboot_info_base], _multiboot_info
	
	mov	dword [_multiboot_flags], 0xc
	
	mov	[_multiboot_cmdline], eax
	add	dword [_multiboot_cmdline], _multiboot_kernel_cmdline
	
	;;
	;; Hide the kernel's entry in the list of modules
	;;
	mov	[_multiboot_mods_addr], eax
	mov	ebx, _multiboot_modules
	add	ebx, multiboot_module_size
	add	dword [_multiboot_mods_addr], ebx
	dec	dword [_multiboot_mods_count]

	;;
	;; get extended memory size in KB
	;;
	push	ebx
	xor	ebx,ebx
	mov	[_multiboot_mem_upper],ebx
	mov	[_multiboot_mem_lower],ebx

	mov	ax, 0xe801
	int	015h
	jc	.oldstylemem

	cmp	ax, 0
	je	.cmem

	and	ebx, 0xffff
	shl	ebx,6
	mov	[_multiboot_mem_upper],ebx
	and	eax,0xffff
	add	dword [_multiboot_mem_upper],eax
	jmp	.done_mem

.cmem:
	cmp	cx, 0
	je	.oldstylemem

	and	edx, 0xFFFF
	shl	edx, 6
	mov	[_multiboot_mem_upper], edx
	and	ecx, 0xFFFF
	add	dword [_multiboot_mem_upper], ecx
	jmp	.done_mem

.oldstylemem:
	;; int 15h opt e801 don't work , try int 15h, option 88h
	mov	ah, 088h
	int	015h
	cmp	ax, 0
	je	.cmosmem
	mov	[_multiboot_mem_upper],ax
	jmp	.done_mem
.cmosmem:
	;; int 15h opt 88h don't work , try read cmos
	xor	eax,eax
	mov	al, 0x31
	out	0x70, al
	in	al, 0x71
	and	eax, 0xffff	; clear carry
	shl	eax,8
	mov	[_multiboot_mem_upper],eax
	xor	eax,eax
	mov	al, 0x30
	out	0x70, al
	in	al, 0x71
	and	eax, 0xffff	; clear carry
	add	[_multiboot_mem_lower],eax

.done_mem:
	pop ebx
	
	;;
	;; Begin the pmode initalization
	;;
	
	;;
	;; Save cursor position
	;;
	mov	ax, 3		;! Reset video mode
	int	10h

	mov	bl, 10
	mov	ah, 12
	int	10h

	mov	ax, 1112h	;! Use 8x8 font
	xor	bl, bl
	int	10h
	mov	ax, 1200h	;! Use alternate print screen
	mov	bl, 20h
	int	10h
	mov	ah, 1h		;! Define cursor (scan lines 6 to 7)
	mov	cx, 0607h
	int	10h

	mov	ah, 1
	mov	cx, 0600h
	int	10h

	mov	ah, 6		; Scroll active page up
	mov	al, 32h		; Clear 50 lines
	mov	cx, 0		; Upper left of scroll
	mov	dx, 314fh	; Lower right of scroll
	mov	bh, 1*10h+1	; Use normal attribute on blanked lines
	int	10h

	mov	dx, 0
	mov	dh, 0

	mov	ah, 2
	mov	bh, 0
	int	10h

	mov	dx, 0
	mov	dh, 0

	mov	ah, 2
	mov	bh, 0
	int	10h

	mov	ah, 3
	mov	bh, 0
	int	10h
	movzx	eax, dl
;	mov	[_cursorx], eax
	movzx	eax, dh
;	mov	[_cursory], eax

	cli

	;;
	;; Load the absolute address of the multiboot information structure
	;;
	mov	ebx, [_multiboot_info_base]

	;;
	;; Enter pmode and clear prefetch queue
	;;
	mov	eax,cr0
	or	eax,0x10001
	mov	cr0,eax
	jmp	.next
.next:
	;;
	;; NOTE: This must be position independant (no references to
	;; non absolute variables)
	;;

	;;
	;; Initalize segment registers
	;;
	mov	ax,KERNEL_DS
	mov	ds,ax
	mov	ss,ax
	mov	es,ax
	mov	fs,ax
	mov	gs,ax

	;;
	;; Initalize eflags
	;;
	push	dword 0
	popf

	;;
	;; Load the multiboot magic value into eax
	;;	
	mov	eax, 0x2badb002

	;;
	;; Jump to start of the kernel
	;;
	jmp	dword KERNEL_CS:(LOAD_BASE+0x1000)

	;;
	;; Never get here
	;;

.exit:
	mov	ax,04c00h
	int	21h

end_cmd_line dw 0

;
; Print string in DS:DI
;
print_string:
	push	ebp
	mov	bp, sp
	push	eax
	push	edx
	push	edi

	mov	ax, 0x0200
.loop:
	mov	dl, [di]
	cmp	dl, 0
	je	.end_loop
	cmp	dl, '%'
	jne	.print_char
	inc	di
	mov	dl, [di]
	cmp	dl, 'a'
	jne	.not_ax
	push	eax
	mov	eax, [ss:bp - 4]
	call	print_ax
	pop	eax
	jmp	.next_char

.not_ax:
	cmp	dl, 'A'
	jne	.not_eax
	push	eax
	mov	eax, [ss:bp - 4]
	call	print_eax
	pop	eax
	jmp	.next_char

.not_eax:
	cmp	dl, 'c'
	jne	.not_cx
	push	ax
	mov	ax, cx
	call	print_ax
	pop	ax
	jmp	.next_char

.not_cx:

.print_char:
	int	0x21

.next_char:
	inc	di
	jmp	.loop

.end_loop:
	pop	edi
	pop	edx
	pop	eax
	pop	ebp
	ret

;
; print_ax - print the number in the ax register
;

print_ax:
	push	ax
	push	bx
	push	cx
	push	dx

	mov	bx, ax
	mov	ax, 0x0200
	mov	cx, 4
.loop:
	mov	dx, bx
	shr	dx, 12
	and	dl, 0x0f
	cmp	dl, 0x0a
	jge	.hex_val
	add	dl, '0'
	jmp	.not_hex

.hex_val:
	add	dl, 'a' - 10

.not_hex:	
	int	0x21
	shl	bx, 4
	dec	cx
	jnz	.loop

	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret

print_eax:
	push	eax
	push	ebx
	push	ecx
	push	edx

	mov	ebx, eax
	mov	ax, 0x0200
	mov	cx, 8
.loop:
	mov	edx, ebx
	shr	edx, 28
	and	dl, 0x0f
	cmp	dl, 0x0a
	jge	.hex_val
	add	dl, '0'
	jmp	.not_hex

.hex_val:
	add	dl, 'a' - 10

.not_hex:
	int	0x21
	shl	ebx, 4
	dec	cx
	jnz	.loop

	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
	ret

STRUC	pe_doshdr
e_magic:	resw	1
e_cblp:		resw	1
e_cp:		resw	1
e_crlc:		resw	1
e_cparhdr:	resw	1
e_minalloc:	resw	1
e_maxalloc:	resw	1
e_ss:		resw	1
e_sp:		resw	1
e_csum:		resw	1
e_ip:		resw	1
e_cs:		resw	1
e_lfarlc:	resw	1
e_ovno:		resw	1
e_res:		resw	4
e_oemid:	resw	1
e_oeminfo:	resw	1
e_res2:		resw	10
e_lfanew:	resd	1
ENDSTRUC


_mb_magic:
	dd 0
_mb_flags:
	dd 0
_mb_checksum:
	dd 0
_mb_header_addr:
	dd 0
_mb_load_addr:
	dd 0
_mb_load_end_addr:
	dd 0
_mb_bss_end_addr:
	dd 0
_mb_entry_addr:
	dd 0

_cpe_doshdr:
	times pe_doshdr_size db 0
_current_filehandle:
	dw 0
_current_size:
	dd 0

	;;
	;; Load a PE file
	;;	DS:DX = Filename
	;;
pe_load_module:
	;;
	;; Open file
	;;
	mov	ax, 0x3d00
	int	0x21
	jnc	.file_opened
	mov	dx, error_file_open_failed
	jmp	.error
.file_opened:

	;;
	;; Save the file handle
	;;
	mov	[_current_filehandle], ax

	;;
	;; Print space
	;;
	mov	ah,02h
	mov	dl,' '
	int	021h

	;;
	;; Seek to the start of the file
	;;
	mov	ax, 0x4200
	mov	bx, [_current_filehandle]
	mov	cx, 0
	mov	dx, 0
	int	0x21
	jnc	.seek_start
	mov	dx, error_file_seek_failed
	jmp	.error
.seek_start:

	;;
	;; Read in the DOS EXE header
	;;
	mov	ah, 0x3f
	mov	bx, [_current_filehandle]
	mov	cx, pe_doshdr_size
	mov	dx, _cpe_doshdr
	int	0x21
	jnc	.header_read
	mov	dx, error_file_read_failed
	jmp	.error
.header_read

	;;
	;; Check the DOS EXE magic
	;;
	mov	ax, word [_cpe_doshdr + e_magic]
	cmp	ax, 'MZ'
	je	.mz_hdr_good
	mov	dx, error_bad_mz
	jmp	.error
.mz_hdr_good

	;;
	;; Find the BSS size
	;;
	mov	ebx, dword [_multiboot_mods_count]
	cmp	ebx, 0
	jne	.not_first
	
	mov	edx, 0
	mov	ax, 0x4200
	mov	cx, 0
	mov	dx, 0x1004
	mov	bx, [_current_filehandle]
	int	0x21
	jnc	.start_seek1
	mov	dx, error_file_seek_failed
	jmp	.error
.start_seek1:
	mov	ah, 0x3F
	mov	bx, [_current_filehandle]
	mov	cx, 32
	mov	dx, _mb_magic
	int	0x21
	jnc	.mb_header_read
	mov	dx, error_file_read_failed
	jmp	.error
.mb_header_read:
	jmp	.first
	
.not_first:
	mov	dword [_mb_bss_end_addr], 0
.first:
	
	;;
	;; Seek to the end of the file to get the file size
	;;
	mov	edx, 0
	mov	ax, 0x4202
	mov	dx, 0
	mov	cx, 0
	mov	bx, [_current_filehandle]
	int	0x21
	jnc	.start_end
	mov	dx, error_file_seek_failed
	jmp	.error
.start_end
	shl	edx, 16
	mov	dx, ax
	mov	[_current_size], edx
	
	mov	edx, 0
	mov	ax, 0x4200
	mov	dx, 0
	mov	cx, 0
	mov	bx, [_current_filehandle]
	int	0x21
	jnc	.start_seek
	mov	dx, error_file_seek_failed
	jmp	.error
.start_seek
	
	mov	edi, [next_load_base]
	
.read_next:
	cmp	dword [_current_size], 32768
	jle	.read_tail

	;;
	;; Read in the file data
	;;
	push	ds
	mov	ah, 0x3f
	mov	bx, [_current_filehandle]
	mov	cx, 32768
	mov	dx, 0x9000
	mov	ds, dx
	mov	dx, 0
	int	0x21
	jnc	.read_data_succeeded
	pop	ds
	mov	dx, error_file_read_failed
	jmp	.error
.read_data_succeeded:
%ifndef NDEBUG
	mov	ah,02h
	mov	dl,'#'
	int	021h
%endif

	;;
	;; Copy the file data just read in to high memory
	;;
	pop	ds
	mov	esi, 0x90000
	mov	ecx, 32768
	call	_himem_copy
%ifndef NDEBUG
	mov	ah,02h
	mov	dl,'$'
	int	021h
%else
	mov	ah,02h
	mov	dl,'.'
	int	021h
%endif

	sub	dword [_current_size], 32768
	jmp	.read_next

.read_tail
	;;
	;; Read in the tailing part of the file data
	;;
	push	ds
	mov	eax, [_current_size]
	mov	cx, ax
	mov	ah, 0x3f
	mov	bx, [_current_filehandle]
	mov	dx, 0x9000
	mov	ds, dx
	mov	dx, 0
	int	0x21
	jnc	.read_last_data_succeeded
	pop	ds
	mov	dx, error_file_read_failed
	jmp	.error
.read_last_data_succeeded:
%ifndef NDEBUG
	mov	ah,02h
	mov	dl,'#'
	int	021h
%endif

	;;
	;; Copy the tailing part to high memory
	;;
	pop	ds
	mov	ecx, [_current_size]
	mov	esi, 0x90000
	call	_himem_copy
%ifndef NDEBUG
	mov	ah,02h
	mov	dl,'$'
	int	021h
%else
	mov	ah,02h
	mov	dl,'.'
	int	021h
%endif

	mov	edx, [_mb_bss_end_addr]
	cmp	edx, 0
	je	.no_bss
	mov	edi, edx	
.no_bss:		
	test	di, 0xfff
	jz	.no_round
	and	di, 0xf000
	add	edi, 0x1000
.no_round:

	mov	bx, [_multiboot_mods_count]
	imul	bx, bx, multiboot_module_size
	add	bx, _multiboot_modules
	
	mov	edx, [next_load_base]
	mov	[bx + mbm_mod_start], edx
	mov	[bx + mbm_mod_end], edi
	mov	[next_load_base], edi
	mov	dword [bx + mbm_reserved], 0
	
	inc	dword [_multiboot_mods_count]

	mov	eax, 1
	
	ret

	;;
	;; On error print a message and return zero
	;;
.error:
	mov	ah, 0x9
	int	0x21
	mov	eax, 0
	ret

	;;
	;; Copy to high memory
	;; ARGUMENTS
	;;	ESI = Source address
	;;	EDI = Destination address
	;;	ECX = Byte count
	;; RETURNS
	;;	EDI = End of the destination region
	;;	ECX = 0
	;; 
_himem_copy:
	push	ds		; Save DS
	push	es		; Save ES
	push	eax
	push	esi

	cmp	eax, 0
	je	.l3
	
	cli			; No interrupts during pmode
	
	mov	eax, cr0	; Entered protected mode
	or	eax, 0x1
	mov	cr0, eax

	jmp	.l1		; Flush prefetch queue
.l1:
	
	mov	eax, KERNEL_DS	; Load DS with a suitable selector
	mov	ds, ax
	mov	eax, KERNEL_DS
	mov	es, ax

	cld
	a32 rep	movsb
;.l2:
;	mov	al, [esi]	; Copy the data
;	mov	[edi], al
;	dec	ecx
;	inc	esi
;	inc	edi
;	cmp	ecx, 0
;	jne	.l2

	mov	eax, cr0	; Leave protected mode
	and	eax, 0xfffffffe
	mov	cr0, eax
	
	jmp	.l3
.l3:	
	sti
	pop	esi
	pop	eax
	pop	es
	pop	ds
	ret

;
; Loading message
;
loading_msg	db	'Loading: ',0

;;
;; Next free address in high memory
;;
next_load_base dd 0

;
; Needed for enabling the a20 address line
;
empty_8042:
	jmp	$+3
	jmp	$+3
	in	al,064h
	test	al,02h
	jnz	empty_8042
	ret

;
; GDT descriptor
;
align 8
gdt_descr:
gdt_limit:
	dw	(5*8)-1
gdt_base:
	dd	_gdt

	;;
	;; Our initial stack
	;;
real_stack times 1024 db 0
real_stack_end:

	;;
	;; Boot information structure
	;;
_multiboot_info_base:
	dd	0x0

_multiboot_info:
_multiboot_flags:
	dd	0x0
_multiboot_mem_lower:
	dd	0x0
_multiboot_mem_upper:
	dd	0x0
_multiboot_boot_device:
	dd	0x0
_multiboot_cmdline:	
	dd	0x0
_multiboot_mods_count:
	dd	0x0
_multiboot_mods_addr:
	dd	0x0
_multiboot_syms:
	times 12 db 0
_multiboot_mmap_length:
	dd	0x0
_multiboot_mmap_addr:
	dd	0x0
_multiboot_drives_count:
	dd	0x0
_multiboot_drives_addr:
	dd	0x0
_multiboot_config_table:
	dd	0x0
_multiboot_boot_loader_name:
	dd	0x0
_multiboot_apm_table:
	dd	0x0

_multiboot_modules:	
	times (64*multiboot_module_size) db 0
_multiboot_module_strings:
	times (64*256) db 0
	
_multiboot_kernel_cmdline:
	times 255 db 0

	;;
	;; Global descriptor table
	;;
_gdt:	
	dw	0x0		; Zero descriptor
	dw	0x0
	dw	0x0
	dw	0x0

	dw	0xffff		; Kernel code descriptor
	dw	0x0000
	dw	0x9a00
	dw	0x00cf

	dw	0xffff		;  Kernel data descriptor
	dw	0x0000
	dw	0x9200
	dw	0x00cf

	dw	0xffff		;  Loader code descriptor
_loader_code_base_0_15:
	dw	0x0000
_loader_code_base_16_23:
	db	0x00
	db	0x9a
	dw	0x0000
	
	dw	0xffff		;  Loader data descriptor
_loader_data_base_0_15:
	dw	0x0000
_loader_data_base_16_23:
	db	0x00
	db	0x92
	dw	0x0000

error_pmode_already:
	db	'Error: The processor is already in protected mode'
	db	0xa, 0xd, '$'
error_file_open_failed:
	db	'Error: Failed to open file'
	db	0xa, 0xd, '$'
error_file_seek_failed:
	db	'Error: File seek failed'
	db	0xa, 0xd, '$'
error_file_read_failed:
	db	'Error: File read failed'
	db	0xa, 0xd, '$'
error_coff_load_failed:
	db	'Error: Failed to load COFF file'
	db	0xa, 0xd, '$'
error_bad_mz:
	db	'Error: Bad DOS EXE magic'
	db	0xa, 0xd, '$'

