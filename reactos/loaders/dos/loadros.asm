;
; Pmode setup stub
; (A20 enable code and PIC reprogram from linux bootsector)
;

;
; Base address of the kernel
;
KERNEL_BASE	equ     0c0000000h

;
; Segment selectors
;
;USER_CS		equ     08h
;USER_DS		equ     010h
;KERNEL_CS	equ     020h
;KERNEL_DS	equ     028h

KERNEL_CS       equ     08h
KERNEL_DS       equ     010h
                                     
;
; Space reserved in the gdt for tss descriptors
;
NR_TASKS        equ     128

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

%%str:		db	%1

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
        ;
        ; Load stack
        ;
        cli
        push    ds
        pop     ss
        push    ds
        pop     es
        mov     sp,real_stack_end
        sti

        ;
        ; Setup the loader space
        ;
        mov     ebx,0
        mov     eax,0
        mov     ecx,0
        mov     edx,0
        mov     esi,0
        mov     edi,0

        ;
        ; Calculate the end of this module
        ;
        mov     ax,ds
        movzx   ebx,ax
        shl     ebx,4
        add     ebx,_end

        ;
        ; Round up to the nearest page
        ;
        and     ebx,~0xfff
        add     ebx,01000h

        ;
        ; Set the start of the page directory
        ;
        mov     [kernel_page_directory_base],ebx

        ;
        ; Set the start of the continuous range of physical memory
        ; occupied by the kernel
        ;
        mov     [_start_mem],ebx
        add     ebx,01000h

        ;
        ; Calculate the start of the system page table (0xc0000000 upwards)
        ;
        mov     [system_page_table_base],ebx
        add     ebx,01000h

        ;
        ; Calculate the start of the page table to map the first 4mb
        ;
        mov     [lowmem_page_table_base],ebx
        add     ebx,01000h

        ;
        ; Set the position for the first module to be loaded
        ;
        mov     [next_load_base],ebx

        ;
        ; Set the address of the start of kernel code
        ;
        mov     [_start_kernel],ebx

        ;
        ; Make the argument list into a c string
        ;
        mov     di,081h
l1:
        cmp     byte [di],0dh
        je      l2
        cmp     byte [di],' '
        jne      l12
        mov     byte [di],0
l12:
        inc     di
        jmp     l1
l2:
        mov     byte [di],0
        mov     [end_cmd_line],di

        mov     dx,082h
l14:
        mov     bx,dx
        cmp     byte [bx],0
        je      l16

        ;
        ; Process the arguments
        ;
        mov     di,loading_msg
        call    print_string
        mov     di,dx
        call    print_string
        mov     ah,02h
        mov     dl,0dh
        int     021h
        mov     ah,02h
        mov     dl,0ah
        int     021h

        ;
        ; Load the file
        ;
        push    di
        mov     dx,di
;        call    _load_file
	call	pe_load_module
        pop     di

        ;
        ; Move onto the next module name in the command line
        ;
l15:
        cmp     di,[end_cmd_line]
        je      l16
        cmp     byte [di],0
        je      l17
        inc     di
        jmp     l15
l17:
        inc     di
        mov     dx,di
        jmp     l14
l16:

        ;
        ; Set the end of kernel memory 
        ;
        mov     eax,[next_load_base]
        mov     [_end_mem],eax

        ;
        ; Begin the pmode initalization
        ;
        jmp     _to_pmode

exit:
        mov     ax,04c00h
        int     21h

        ;
        ; Any errors detected jump here
        ;
_error:
        mov     di,err_msg
        call    print_string
        jmp     exit

end_cmd_line dw 0

;
; In: EDI = address
; Out: FS = segment
;      DI = base
;
convert_to_seg:
        push    eax

        mov     eax,edi
;        shr     eax,16
;        shl     eax,12
;        mov     fs,ax
	shr	eax, 4
	mov	fs, ax
	and	edi, 0xf

;        and     edi,0ffffh

        pop     eax
        ret

;
; Print string in DS:DI
;
print_string:
	push	ebp
	mov	bp, sp
        push    eax
        push    edx
        push    edi

        mov     ax, 0x0200
.loop:
        mov     dl, [di]
        cmp     dl, 0
        je      .end_loop
	cmp	dl, '%'
	jne	.print_char
	inc	di
        mov     dl, [di]
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
        int     0x21

.next_char:
        inc     di
        jmp     .loop

.end_loop:
        pop     edi
        pop     edx
        pop     eax
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


STRUC	DOS_HDR
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

STRUC	NT_HDRS
nth_sig:	resd	1
ntf_mach:	resw	1    
ntf_num_secs:	resw	1
ntf_timestamp:	resd	1
ntf_symtab_ptr:	resd	1
ntf_num_syms:	resd	1
ntf_opt_hdr_sz:	resw	1
ntf_chars:	resw	1

nto_magic:	resw	1
nto_mjr_lnk_vr:	resb	1
nto_mnr_lnk_vr:	resb	1
nto_code_sz:	resd	1
nto_data_sz:	resd	1   
nto_bss_sz:	resd	1   
nto_entry_offs:	resd	1   
nto_code_offs:	resd	1   
nto_data_offs:	resd	1   
nto_image_base:	resd	1   
nto_sec_align:	resd	1   
nto_file_align:	resd	1   
nto_mjr_os_ver:	resw	1    
nto_Mnr_os_ver:	resw	1    
nto_mjr_img_vr:	resw	1    
nto_Mnr_img_vr:	resw	1    
nto_mjr_subsys:	resw	1    
nto_mnr_subsys:	resw	1    
nto_w32_ver:	resd	1   
nto_image_sz:	resd	1   
nto_hdr_sz:	resd	1   
nto_chksum:	resd	1   
nto_subsys:	resw	1    
nto_dll_chars:	resw	1    
nto_stk_res_sz:	resd	1   
nto_stk_cmt_sz:	resd	1   
nto_hp_res_sz:	resd	1   
nto_hp_cmt_sz:	resd	1   
nto_ldr_flags:	resd	1   
nto_dir_cnt:	resd	1   
nto_dir_ent:	resq	16
ENDSTRUC

STRUC	DATA_DIR
dd_rva:		resd	1
dd_sz:		resd	1
ENDSTRUC

STRUC  SCN_HDR
se_name:	resb	8
se_vsz:		resd	1
se_vaddr:	resd	1
se_rawsz:	resd	1
se_raw_ofs:	resd	1
se_reloc_ofs:	resd	1
se_lnum_ofs:	resd	1
se_num_relocs:	resw	1
se_num_lnums:	resw	1
se_chars:	resd	1
ENDSTRUC

;
; pe_load_module - load a PE module into memory
;
;	DI - Filename
;
;	[_nr_files] - total files loaded (incremented)
;	[next_load_base] - load base for file (updated to next loc)
;	[_module_lengths] - correct slot is set.
;

pe_load_module:
	push	dx
	push	ds

	push	ds
	pop	es

	mov	eax, [next_load_base]
	mov	[load_base], eax
DPRINT	'next_load_base %A', 13, 10, 0

        ;
        ; Open the file
        ;
        mov     ax, 0x3d00
        mov     dx, di
        int     0x21
        jnc	.open_good
	jmp	.error
.open_good:
        mov     [file_handle],ax
        
        ;
        ; Seek to beginning of file
        ;
        mov     ax,0x4200
        mov     bx, [file_handle]
        mov     cx, 0
        mov     dx, 0
        int     0x21
	jnc	.rewind_good
        jmp	.error
.rewind_good:

	;
	; Compute load address for PE headers
	;
        mov     edi,[load_base]
        call    convert_to_seg
        mov     dx,di
        push    fs
        pop     ds

	;
	; Load the headers
	;
        mov     ax, 0x3f00
        mov     bx, [es:file_handle]
        mov     cx, 0x1000
        int     0x21

	;
	;  Check DOS MZ Header
	;
	mov	bx, dx
	mov	ax, word [bx + e_magic]
	cmp	ax, 'MZ'
	je	.mz_hdr_good
        push    es
        pop     ds
	mov	dx, bad_mz_msg
	mov	di, dx
	call	print_string
	jmp	.error

.mz_hdr_good:
	;
	;  Check PE Header
	;
	mov	eax, dword [bx + e_lfanew]
DPRINT	'lfanew %A ', 0
	add	bx, ax
	mov	eax, dword [bx + nth_sig]
	cmp	eax, 0x00004550
	je	.pe_hdr_good
        push    es
        pop     ds
	mov	dx, bad_pe_msg
	mov	di, dx
	call	print_string
	jmp	.error
	
.pe_hdr_good:
	;
	;  Get header size and bump next_load_base
	;
	mov	eax, [bx + nto_hdr_sz]
DPRINT	'header size %A ', 0
	add	dword [es:next_load_base], eax

	;
	;  Setup section pointer
	;
	mov	ax, [bx + ntf_num_secs]
DPRINT	'num sections %a', 13, 10, 0
	mov	[es:num_sections], ax
	add	bx, NT_HDRS_size
	mov	[es:cur_section], bx
	mov	[es:cur_section + 2], ds
	;
	;  Load each section or fill with zeroes
	;
.scn_loop:
	;
	;  Compute size of data to load from file
	;
	mov	eax, [bx + se_rawsz]
DPRINT	'raw size %A ', 0
	cmp	eax, 0
	jne	.got_data
	jmp	.no_data
.got_data:
        mov     [es:size_mod_4k], ax
        and     word [es:size_mod_4k], 0x0fff
        shr     eax, 12
        mov     dword [es:size_div_4k], eax

	;
	;  Seek to section offset
	;
	mov	eax, [bx + se_raw_ofs]
DPRINT	'raw offset %A ', 0
	mov	dx, ax
	shr	eax, 16
	mov	cx, ax
        mov     ax,0x4200
        mov     bx, [es:file_handle]
        int     0x21
	jnc	.seek_good
        jmp	.error
.seek_good:

	;
	;  Load the base pointer
	;
        mov     edi,[es:next_load_base]
        call    convert_to_seg
        mov     dx, di
        push    fs
        pop     ds

        ;
        ;  Read data in 4k chunks
        ;
.do_chunk:
        ;
        ; Check if we have read it all
        ;
        mov     eax, [es:size_div_4k]
        cmp     eax, 0
        je      .chunk_done

        ;
        ; Make the call (dx was loaded above)
        ;
        mov     ax, 0x3f00
        mov     bx, [es:file_handle]
        mov     cx, 0x1000
        int     0x21               
	; FIXME: should check return status and count

        ;
        ; We move onto the next pointer by altering ds
        ;
        mov     ax, ds
        add     ax, 0x0100
        mov     ds, ax
        dec     word [es:size_div_4k]
        jmp     .do_chunk

.chunk_done:
        ;
        ; Read the last section
        ;
        mov     ax, 0x3f00
        mov     bx, [es:file_handle]
        mov     cx, [es:size_mod_4k]
        int     0x21
	jnc	.last_read_good
        jmp	.error
.last_read_good:

.no_data:
	;
	;  Zero out uninitialized data sections
	;
	lds	bx, [es:cur_section]
mov	eax, dword [bx + se_chars]
DPRINT	'section chars %A', 13, 10, 0
	test	dword [bx + se_chars], 0x80
	jz	.no_fill
	
	;
	;  Compute size of section to zero fill
	;
	mov	eax, [bx + se_vsz]
	cmp	eax, 0
	je	.no_fill
        mov     [es:size_mod_4k], ax
        and     word [es:size_mod_4k], 0x0fff
        shr     eax, 12
        mov     [size_div_4k], eax

	;
	;  Load the base pointer
	;
        mov     edi,[es:next_load_base]
        call    convert_to_seg
        mov     dx, di
        push    fs
        pop     ds

.do_fill:
        ;
        ; Check if we have read it all
        ;
        mov     eax, [es:size_div_4k]
        cmp     eax, 0
        je      .fill_done

        ;
        ; Zero out a chunk
        ;
	mov	ax, 0x0000	
        mov     cx, 0x1000
	push	di
	push	es
	push	ds
	pop	es
rep	stosb
	pop	es
	pop	di

        ;
        ; We move onto the next pointer by altering ds
        ;
        mov     ax, ds
        add     ax, 0x0100
        mov     ds, ax
        dec     word [es:size_div_4k]
        jmp     .do_fill

.fill_done:
        ;
        ; Read the last section
        ;
	mov	ax, 0x0000	
        mov     cx, [es:size_mod_4k]
	push	di
	push	es
	push	ds
	pop	es
rep	stosb
	pop	es
	pop	di

.no_fill:

	;
	;  Update raw data offset in section header
	;
	lds	bx, [es:cur_section]
	mov	eax, [es:next_load_base]
	sub	eax, [es:load_base]
DPRINT	'new raw offset %A ', 0
	mov	[bx + se_raw_ofs], eax
	
	;
	;  Update next_load_base
	;
	mov	eax, [bx + se_vsz]
DPRINT	'scn virtual sz %A ', 0
	and	eax, 0xfffff000
	add	dword [es:next_load_base], eax
	test	dword [bx + se_vsz], 0xfff
	jz	.even_scn
	add	dword [es:next_load_base], 0x1000

.even_scn:
mov	eax, [es:next_load_base]
DPRINT	'next load base %A', 13, 10, 0

	;
	;  Setup for next section or exit loop
	;
	dec	word [es:num_sections]
	jz	.scn_done
	add	bx, SCN_HDR_size
	mov	[es:cur_section], bx
	jmp	.scn_loop

.scn_done:
	;
	;  Update module_length
	;
	mov	eax, [es:next_load_base]
	sub	eax, [es:load_base]
	mov	esi, [es:_nr_files]
	mov	[es:_module_lengths + esi * 4], eax

        inc     dword [es:_nr_files]

	pop	ds
	pop	dx
	ret

.error:
	push	es
	pop	ds
        mov     di, err_msg
        call    print_string
        jmp     exit

;
; Handle of the currently open file
;
file_handle dw 0

;
; Size of the current file mod 4k
;
size_mod_4k dw 0

;
; Size of the current file divided by 4k
;
size_div_4k dd 0

load_base	dd	0
num_sections	dw	0
cur_section	dd	0

;
;
;
last_addr dw 0

;
; Generic error message
;
err_msg		db	'Error during operation',10, 13, 0
bad_mz_msg	db	'Module has bad MZ header', 10, 13, 0
bad_pe_msg	db	'Module has bad PE header', 10, 13, 0
rostitle	db	'',0
loading_msg	db	'Loading: ',0
death_msg	db	'death', 0

filelength_lo	dw 0
filelength_hi	dw 0

kernel_page_directory_base dd 0
system_page_table_base dd 0
lowmem_page_table_base dd 0
next_load_base dd 0
_start_kernel dd 0

boot_param_struct_base dd 0

;
; These variables are passed to the kernel (as a structure)
;
align 4
_boot_param_struct:
_magic:
        dd 0
_cursorx:
        dd 0
_cursory:
        dd 0
_nr_files:
        dd 0
_start_mem:
        dd 0
_end_mem:
        dd 0
_module_lengths:
        times 64 dd 0
_end_boot_param_struct
             
;
; Needed for enabling the a20 address line
;
empty_8042:
        jmp     $+3
        jmp     $+3
        in      al,064h
        test    al,02h
        jnz     empty_8042
	ret

;
; GDT descriptor
;
align 8
gdt_descr:
gdt_limit:
        dw (3*8)-1
gdt_base:
        dd gdt


_to_pmode:
        ;
        ; Setup kernel parameters
        ;
        mov     dword [_magic],0xdeadbeef

        ;
        ; Save cursor position
        ;
        mov     ax,3          ;! Reset video mode
        int     10h


        mov     bl,10
        mov     ah,12
        int     10h
        
	mov     ax,1112h      ;! Use 8x8 font
	xor	bl,bl
	int     10h
        mov     ax,1200h      ;! Use alternate print screen
        mov     bl,20h
        int     10h
        mov     ah,1h         ;! Define cursor (scan lines 6 to 7)
        mov     cx,0607h
        int     10h

        mov     ah,1
        mov     cx,0600h
        int     10h

        MOV       AH,6        ;SCROLL ACTIVE PAGE UP
        MOV       AL,32H     ;CLEAR 25 LINES
        MOV       CX,0H       ;UPPER LEFT OF SCROLL
        MOV       DX,314FH    ;LOWER RIGHT OF SCROLL
        MOV       BH,1*10h+1       ;USE NORMAL ATTRIBUTE ON BLANKED LINE
        INT       10H         ;VIDEO-IO


        mov     dx,0
        mov     dh,0

        mov     ah,02h
        mov     bh,0
        int     10h

        mov     dx,0
        mov     dh,0

        mov     ah,02h
        mov     bh,0
        int     010h
          
        mov     ah,03h
        mov     bh,0h
        int     010h
        movzx   eax,dl
        mov     [_cursorx],eax
        movzx   eax,dh
        mov     [_cursory],eax

        mov     bx,ds
        movzx   eax,bx
        shl     eax,4
        add     eax,_boot_param_struct
        mov     [boot_param_struct_base],eax        

        cli

        ;
        ; Zero out the kernel page directory
        ;
        ;
        mov     edi,[kernel_page_directory_base]
        call    convert_to_seg

        mov     cx,1024
l10:
        mov     dword [fs:di],0
        add     di,4
        loop    l10

        ;
        ; Map in the lowmem page table (and reuse it for the identity map)
        ;
        mov     edi,[kernel_page_directory_base]
        call    convert_to_seg

        mov     eax,[lowmem_page_table_base]
        add     eax,07h
        mov     [fs:di],eax
        mov     [fs:di+(0xd0000000/(1024*1024))],eax
	
	;
	; Map the page tables from the page table
	;
	mov     eax,[kernel_page_directory_base]
	add     eax,07h
	mov     [fs:di+(0xf0000000/(1024*1024))],eax
	
        ;
        ; Map in the kernel page table
        ;
        mov     eax,[system_page_table_base]
        add     eax,07h
        mov     [fs:di+3072],eax

        ;
        ; Setup the lowmem page table
        ;
        mov     edi,[lowmem_page_table_base]
        call    convert_to_seg

        mov     ebx,0
l9:
        mov     eax,ebx
        shl     eax,12        ; ebx = ebx * 4096
        add     eax,07h       ; user, rw, present
        mov     [fs:edi+ebx*4],eax
        inc     ebx
        cmp     ebx,1024
        jl      l9

        ;
        ; Setup the system page table
        ;
        mov     edi,[system_page_table_base]
        call    convert_to_seg

        mov     eax,07h
l8:
        mov     edx,eax
        add     edx,[_start_kernel]
        mov     [fs:edi],edx
        add     edi,4
        add     eax,1000h
        cmp     eax,100007h
        jl      l8
        
        ;
        ; Load the page directory into cr3
        ;
        mov     eax,[kernel_page_directory_base]
        mov     cr3,eax

        ;
        ; Setup various variables
        ;
        mov     bx,ds
        movzx   eax,bx
        shl     eax,4
        add     [gdt_base],eax

        ;
        ; Enable the A20 address line (to allow access to over 1mb)
        ;
	call	empty_8042
        mov     al,0D1h                ; command write
        out     064h,al
	call	empty_8042
        mov     al,0DFh                ; A20 on
        out     060h,al
	call	empty_8042

        ;
        ; Reprogram the PIC because they overlap the Intel defined
        ; exceptions 
        ;
        mov     al,011h                ; initialization sequence
        out     020h,al                ; send it to 8259A-1
        dw   0x00eb,0x00eb           ; jmp $+2, jmp $+2
        out     0A0h,al                ; and to 8259A-2
        dw   0x00eb,0x00eb
        mov     al,040h                ; start of hardware int's (0x20)
        out     021h,al
        dw   0x00eb,0x00eb
        mov     al,048h                ; start of hardware int's 2 (0x28)
        out     0A1h,al
        dw   0x00eb,0x00eb
        mov     al,04h                ; 8259-1 is master
        out     021h,al
        dw  0x00eb,0x00eb
        mov     al,002h                ; 8259-2 is slave
        out     0A1h,al
        dw   0x00eb,0x00eb
        mov     al,01h                ; 8086 mode for both
        out     021h,al
        dw  0x00eb,0x00eb
        out     0A1h,al
        dw   0x00eb,0x00eb
        mov     al,0FFh                ; mask off all interrupts for now
        out     021h,al
        dw   0x00eb,0x00eb
        out     0A1h,al

        ;
        ; Load stack
        ;
        mov     bx,ds
        movzx   eax,bx
        shl     eax,4
        add     eax,real_stack_end
        mov     [real_stack_base],eax
        mov     esp,[real_stack_base]
        mov     edx,[boot_param_struct_base]

        ;
        ; load gdt
        ;
        lgdt    [gdt_descr]

        ;       
        ; Enter pmode and clear prefetch queue
        ;
        mov     eax,cr0
        or      eax,0x80010001
        mov     cr0,eax
        jmp     next
next:
        ;
        ; NOTE: This must be position independant (no references to
        ; non absolute variables)
        ;

        ;
        ; Initalize segment registers
        ;
        mov     ax,KERNEL_DS
        mov     ds,ax
        mov     ss,ax        
        mov     es,ax
        mov     fs,ax
        mov     gs,ax

        ;
        ; Initalize eflags
        ;
        push    dword 0
        popf

        ;
        ; Jump to start of 32 bit code at c0000000
        ;
        push    edx
        push    dword 0
        jmp     dword KERNEL_CS:(KERNEL_BASE+0x1000)


;
; Our initial stack
;
real_stack times 1024 db 0
real_stack_end:

real_stack_base dd 0


;
; Global descriptor table
;
align 8
gdt:
        dw 0               ; Zero descriptor
        dw 0
        dw 0
        dw 0
                                
        ;dw 00000h          ; User code descriptor
        ;dw 00000h          ; base: 0h limit: 3gb
        ;dw 0fa00h
        ;dw 000cch
                               
        ;dw 00000h          ; User data descriptor
        ;dw 00000h          ; base: 0h limit: 3gb
        ;dw 0f200h
        ;dw 000cch
                            
        ;dw 00000h          
        ;dw 00000h         
        ;dw 00000h
        ;dw 00000h

        dw 0ffffh          ; Kernel code descriptor 
        dw 00000h          ; 
        dw 09a00h          ; base 0h limit 4gb
        dw 000cfh
                               
        dw 0ffffh          ; Kernel data descriptor
        dw 00000h          ; 
        dw 09200h          ; base 0h limit 4gb
        dw 000cfh

                                
        ;times NR_TASKS*8 db 0

_end:



