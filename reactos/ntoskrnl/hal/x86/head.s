#define NR_TASKS 128

.globl _stext
.globl _idt
.globl _gdt
.globl _start
.globl _mainCRTStartup
.globl start
/*.globl _DllMainCRTStartup@12*/

/*_DllMainCRTStartup@12:*/
_stext:
_mainCRTStartup:
_start:
start:
        lidt    _idt_descr
        lgdt    _gdt_descr

        movw    $0x28,%ax
        movw    %ax,%ds

        popl    %eax
        popl    %eax
        movl    $_init_stack_top,%esp
        pushl   %eax
        pushl   $0

        jmp     __main

.data
_gdt:
        .word 0               
        .word 0
        .word 0
        .word 0
                                
        .word 0x0000        
        .word 0x0000         
        .word 0xfa00
        .word 0x00cc
                               
        .word 0x0000         
        .word 0x0000        
        .word 0xf200
        .word 0x00cc
                            
        .word 0xffff        
        .word 0x0000      
        .word 0x9200
        .word 0x00cf

        .word 0xffff       
        .word 0x0000        
        .word 0x9a00        
        .word 0x00cf
                               
        .word 0xffff          
        .word 0x0000          
        .word 0x9200        
        .word 0x00cf
        
         .fill 128,8,0

_idt_descr:
        .word (256*8)-1
        .long _idt

_gdt_descr:
        .word ((6+NR_TASKS)*8)-1
        .long _gdt

_idt:
        .fill 256,8,0

_init_stack:
        .fill 4096,1,0
_init_stack_top:

#if 0
_stext:
#endif

