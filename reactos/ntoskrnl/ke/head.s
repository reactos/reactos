#include <internal/ntoskrnl.h>

#define NR_TASKS 128

.globl _start
.globl _NtProcessStartup
.globl _init_stack
.globl _init_stack_top

_NtProcessStartup:
_start:
        lidt    _idt_descr
        lgdt    _gdt_descr

        movw    $0x10,%ax
        movw    %ax,%ds

        popl    %eax
        popl    %eax
        movl    $_init_stack_top,%esp
        pushl   %eax
        pushl   $0

        jmp     __main

.data

_idt_descr:
        .word (256*8)-1
        .long _KiIdt

_gdt_descr:
        .word ((8+NR_TASKS)*8)-1
        .long _KiGdt

.align 8
_init_stack:
        .fill MM_STACK_SIZE,1,0
_init_stack_top:



