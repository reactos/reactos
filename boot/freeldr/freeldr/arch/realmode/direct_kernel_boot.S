#include <asm.inc>
#include "../../include/arch/pc/x86common.h"

#define IMAGE_DOS_HEADER_e_lfanew 60
#define IMAGE_FILE_HEADER_SIZE 20
#define IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint 16

.code16

/* FAT helper code */
#include "fathelp.inc"

.org 512
DirectBoot:
    cli
    
    /* Setup segments */
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    /* Set stack */
    mov sp, HEX(7C00)
    
    /* Output status */
    mov dx, HEX(03F8)
    mov al, 'D'
    out dx, al
    
    /* Enable A20 */
    in al, HEX(92)
    or al, 2
    out HEX(92), al
    
    /* Load kernel directly from CD at sector 2048 (assuming kernel is there) */
    /* For now, just try to jump to kernel entry if it exists */
    
    /* Build minimal page tables for long mode */
    mov edi, HEX(1000)
    xor eax, eax
    mov ecx, HEX(1000)
    rep stosd
    
    /* PML4 at 0x1000 */
    mov dword ptr [HEX(1000)], HEX(2003)
    
    /* PDP at 0x2000 */
    mov dword ptr [HEX(2000)], HEX(3003)
    
    /* PD at 0x3000 - identity map first 2MB */
    mov dword ptr [HEX(3000)], HEX(0083)
    
    /* Load GDT */
    lgdt gdtptr
    
    /* Enter protected mode */
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    /* Jump to protected mode */
    .byte HEX(66), HEX(EA)
    .long InProtectedMode
    .word HEX(08)
    
.code32
InProtectedMode:
    /* Set segments */
    mov ax, HEX(10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    /* Set PAE and PGE */
    mov eax, cr4
    or eax, HEX(A0)
    mov cr4, eax
    
    /* Set page table */
    mov eax, HEX(1000)
    mov cr3, eax
    
    /* Enable long mode */
    mov ecx, HEX(C0000080)
    rdmsr
    or eax, HEX(100)
    wrmsr
    
    /* Enable paging */
    mov eax, cr0
    or eax, HEX(80000000)
    mov cr0, eax
    
    /* Jump to long mode */
    .byte HEX(EA)
    .long InLongMode
    .word HEX(18)
    
.code64
InLongMode:
    /* Output 'L' */
    mov dx, HEX(3F8)
    mov al, 'L'
    out dx, al
    
    /* Set segments */
    mov ax, HEX(20)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    /* Set stack */
    mov rsp, HEX(90000)
    
    /* Try to load kernel */
    /* For ReactOS, kernel is usually at 0x800000 */
    /* But we need to load it first - for now just output message */
    
    mov dx, HEX(3F8)
    mov al, 'K'
    out dx, al
    
    /* Jump to kernel entry if it exists */
    mov rax, HEX(800000)
    mov rbx, qword ptr [rax]
    test rbx, rbx
    jz no_kernel
    
    /* Found something at kernel address */
    mov dx, HEX(3F8)
    mov al, '!'
    out dx, al
    
    /* Jump to kernel */
    jmp rax
    
no_kernel:
    /* No kernel found */
    mov dx, HEX(3F8)
    mov al, 'N'
    out dx, al
    
halt:
    hlt
    jmp halt

.align 8
gdt:
    .quad 0                         /* NULL */
    .quad HEX(00CF9A000000FFFF)    /* 32-bit code */
    .quad HEX(00CF92000000FFFF)    /* 32-bit data */
    .quad HEX(00209A0000000000)    /* 64-bit code */
    .quad HEX(0000920000000000)    /* 64-bit data */
    
gdtptr:
    .word 39
    .long gdt

.org (FREELDR_PE_BASE - FREELDR_BASE - 1)
.byte 0
.endcode16

END