;
; Pmode setup stub
; (A20 enable code and PIC reprogram from linux bootsector)
;

;
; Base address of the kernel
;
KERNEL_BASE      equ     0c0000000h

;
; Segment selectors
;
USER_CS       equ     08h
USER_DS       equ     010h
KERNEL_CS     equ     020h
KERNEL_DS     equ     028h
                                     
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

entry:
        ;
        ; Load stack
        ;
        cli
        push    ds
        pop     ss
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
        call    _print_string
        mov     di,dx
        call    _print_string
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
        call    _load_file
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
        call    _print_string
        jmp     exit

end_cmd_line dw 0


;
; Read in a file to kernel_base, set kernel base to the end of the file in
; memory rounded up to the nearest page
;
; In:
;       DI = filename
;
_load_file:
        inc     dword [_nr_files]

        ;
        ; Open the file
        ;
        mov     ah,03dh
        mov     al,0
        mov     dx,di
        int     021h
        jc      _error
        mov     [file_handle],ax
        
        ;
        ; Find filelength
        ;
        mov     ah,042h
        mov     al,2
        mov     bx,[file_handle]
        mov     cx,0
        mov     dx,0
        int     021h

        ;
        ; Convert the filelength in DX:AX to a dword in EBX
        ;
        movzx   ebx,dx
        shl     ebx,16
        mov     bx,ax

        ;
        ; Record the length of the module in boot parameter table
        ;
        mov     esi,[_nr_files]
        dec     esi
        mov     [_module_lengths+esi*4],ebx

        ; 
        ; Convert the length into 
        ;
        mov     [size_mod_4k],bx
        and     word [size_mod_4k],0fffh

        shr     ebx,12
        mov     [size_div_4k],ebx


        ;
        ; Seek to beginning of file
        ;
        mov     ah,042h
        mov     al,0
        mov     bx,[file_handle]
        mov     cx,0
        mov     dx,0
        int     021h
        jc      _error

        ;
        ;  Read in the module
        ;
        push    ds

        ;
        ; Convert the linear point to the load address into a seg:off
        ;
        mov     edi,[next_load_base]
        call    convert_to_seg
        mov     dx,di

        ;
        ; Move onto the next position in prepartion for a future read
        ;
        mov     eax,[size_div_4k]
        mov     bx,[size_mod_4k]
        cmp     bx,0
        je      l20
        inc     eax        
l20:
        shl     eax,0ch
        add     [next_load_base],eax

        push    fs
        pop     ds
       
        ;
        ; We read the kernel in 4k chunks (because)
        ;
l6:
        ;
        ; Check if we have read it all
        ;
        mov     ax,[es:size_div_4k]
        cmp     ax,0
        je      l5

        ;
        ; Make the call (dx was loaded above)
        ;
        mov     ah,3fh
        mov     bx,[es:file_handle]
        mov     cx,01000h
        int     21h               

        ;
        ; We move onto the next pointer by altering ds
        ;
        mov     ax,ds
        add     ax,0100h
        mov     ds,ax
        dec     word [es:size_div_4k]
        jmp     l6

l5:
        ;
        ; Read the last section
        ;
        mov     ah,3fh
        mov     bx,[es:file_handle]
        mov     cx,[es:size_mod_4k]
        int     21h
        pop     ds
        jnc     _no_error
        jmp     _error

_no_error:
        ret

;
; In: EDI = address
; Out: FS = segment
;      DI = base
;
convert_to_seg:
        push    eax

        mov     eax,edi
        shr     eax,16
        shl     eax,12
        mov     fs,ax

        and     edi,0ffffh

        pop     eax
        ret

;
; Print string in DS:DI
;
_print_string:
        push    ax
        push    dx
        push    di
        mov     ah,02h
l3:
        mov     dl,[di]
        cmp     dl,0
        je      l4
        int     021h
        inc     di
        jmp     l3
l4:
        pop     di
        pop     dx
        pop     ax
        ret

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

;
;
;
last_addr dw 0

;
; Generic error message
;
err_msg db 'Error during operation',0

;
;
;
loading_msg db 'Loading: ',0

filelength_lo dw 0
filelength_hi dw 0

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
        dw (((6+NR_TASKS)*8)-1)
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
        mov     al,020h                ; start of hardware int's (0x20)
        out     021h,al
        dw   0x00eb,0x00eb
        mov     al,028h                ; start of hardware int's 2 (0x28)
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
        or      eax,0x80000001
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
        jmp     dword KERNEL_CS:KERNEL_BASE


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
                                
        dw 00000h          ; User code descriptor
        dw 00000h          ; base: 0h limit: 3gb
        dw 0fa00h
        dw 000cch
                               
        dw 00000h          ; User data descriptor
        dw 00000h          ; base: 0h limit: 3gb
        dw 0f200h
        dw 000cch
                            
        dw 00000h          
        dw 00000h         
        dw 00000h
        dw 00000h

        dw 0ffffh          ; Kernel code descriptor 
        dw 00000h          ; 
        dw 09a00h          ; base 0h limit 4gb
        dw 000cfh
                               
        dw 0ffffh          ; Kernel data descriptor
        dw 00000h          ; 
        dw 09200h          ; base 0h limit 4gb
        dw 000cfh

                                
        times NR_TASKS*8 db 0

_end:



