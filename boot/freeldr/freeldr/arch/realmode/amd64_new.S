#include <asm.inc>
#include "../../include/arch/pc/x86common.h"

#define IMAGE_DOS_HEADER_e_lfanew 60
#define IMAGE_FILE_HEADER_SIZE 20
#define IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint 16

.code16

/* FAT helper code */
#include "fathelp.inc"

.org 512
Startup:
    cli
    
    /* Setup real mode segment registers */
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    /* Save boot drive and partition */
    mov byte ptr ds:[BSS_BootDrive], dl
    mov byte ptr ds:[BSS_BootPartition], dh
    
    /* Setup real mode stack */
    mov sp, word ptr ds:[stack16]
    
    /* Enable A20 */
    call EnableA20
    
    /* Check for 64-bit support */
    call CheckFor64BitSupport
    test al, al
    jz .NoLongModeSupport
    
    /* Build page tables */
    call BuildPageTables
    
    /* Get PE entry point */
    call GetEntryPoint
    
    /* Switch to long mode */
    call SwitchToLongMode
    
    /* Should never get here */
    jmp Reboot

.NoLongModeSupport:
    mov si, offset Msg_NoLongMode
    call writestr
    int HEX(16)
    jmp Reboot

Msg_NoLongMode:
    .ascii "64-bit CPU required!", CR, LF
    .ascii "Press any key to reboot...", NUL

/* Check for 64-bit CPU support */
CheckFor64BitSupport:
    pushad
    
    /* Check CPUID support */
    pushfd
    pop eax
    mov ebx, eax
    xor eax, HEX(00200000)
    push eax
    popfd
    pushfd
    pop eax
    cmp eax, ebx
    jz .NoCPUID
    
    /* Check for extended CPUID */
    mov eax, HEX(80000000)
    cpuid
    cmp eax, HEX(80000001)
    jb .NoLongMode
    
    /* Check for long mode */
    mov eax, HEX(80000001)
    cpuid
    test edx, HEX(20000000)
    jz .NoLongMode
    
    popad
    mov al, 1
    ret
    
.NoCPUID:
.NoLongMode:
    popad
    xor al, al
    ret

/* Build identity-mapped page tables for first 1GB */
BuildPageTables:
    pusha
    push es
    
    /* Clear page table area */
    mov ax, PML4_ADDRESS / 16
    mov es, ax
    xor di, di
    xor eax, eax
    mov cx, 3 * 1024  /* 3 pages * 1024 dwords */
    rep stosd
    
    /* PML4[0] -> PDP */
    mov di, 0
    mov eax, PDP_ADDRESS | HEX(03)  /* Present + RW */
    stosd
    
    /* PDP[0] -> PD */
    mov di, HEX(1000)
    mov eax, PD_ADDRESS | HEX(03)   /* Present + RW */
    stosd
    
    /* PD[0-511] -> 2MB pages */
    mov di, HEX(2000)
    mov eax, HEX(0083)  /* Present + RW + PS (2MB pages) */
    mov cx, 512
.FillPD:
    stosd
    add eax, HEX(200000)  /* Next 2MB */
    xor edx, edx
    stosd  /* High 32 bits = 0 */
    loop .FillPD
    
    pop es
    popa
    ret

/* Get PE entry point */
GetEntryPoint:
    push es
    
    /* Address PE image */
    mov ax, FREELDR_PE_BASE / 16
    mov es, ax
    
    /* Get e_lfanew */
    mov eax, dword ptr es:[IMAGE_DOS_HEADER_e_lfanew]
    
    /* Get optional header */
    add eax, 4 + IMAGE_FILE_HEADER_SIZE
    
    /* Get AddressOfEntryPoint */
    mov eax, dword ptr es:[eax + IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint]
    add eax, FREELDR_PE_BASE
    
    /* Store as 64-bit value */
    mov dword ptr ds:[EntryPoint64], eax
    mov dword ptr ds:[EntryPoint64 + 4], 0
    
    pop es
    ret

/* Switch to long mode */
SwitchToLongMode:
    /* Disable interrupts */
    cli
    
    /* Load GDT */
    lgdt ds:[gdtptr]
    
    /* Enable PAE and PGE */
    mov eax, cr4
    or eax, HEX(00A0)
    mov cr4, eax
    
    /* Load page table base */
    mov eax, PML4_ADDRESS
    mov cr3, eax
    
    /* Enable long mode in EFER */
    mov ecx, HEX(C0000080)  /* EFER MSR */
    rdmsr
    or eax, HEX(0100)       /* LME bit */
    wrmsr
    
    /* Enable paging and protection */
    mov eax, cr0
    or eax, HEX(80000001)
    mov cr0, eax
    
    /* Jump to 64-bit code */
    DB 0xEA  /* Far jump opcode */
    DD offset LongModeEntry
    DW HEX(10)  /* 64-bit code selector */

.code64
LongModeEntry:
    /* We're in 64-bit mode now! */
    
    /* Setup data segments */
    mov ax, HEX(18)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    /* Setup stack */
    mov rsp, STACKADDR
    
    /* Clear direction flag */
    cld
    
    /* Jump to PE entry point */
    mov rax, qword ptr [EntryPoint64]
    jmp rax

.code16

/* Enable A20 gate */
EnableA20:
    /* Try BIOS first */
    mov ax, HEX(2401)
    int HEX(15)
    jnc .A20Done
    
    /* Try keyboard controller */
    call .WaitKbdInput
    mov al, HEX(D1)
    out HEX(64), al
    call .WaitKbdInput
    mov al, HEX(DF)
    out HEX(60), al
    call .WaitKbdInput
    
.A20Done:
    ret

.WaitKbdInput:
    in al, HEX(64)
    test al, 2
    jnz .WaitKbdInput
    ret

/* Write string to screen */
writestr:
    lodsb
    test al, al
    jz .done
    mov ah, HEX(0E)
    mov bx, 7
    int HEX(10)
    jmp writestr
.done:
    ret

/* Reboot system */
Reboot:
    mov ax, HEX(40)
    mov ds, ax
    mov word ptr ds:[HEX(72)], HEX(1234)
    jmp HEX(F000):HEX(FFF0)

/* Data structures */
.align 8

/* GDT for long mode */
gdt:
    /* Null descriptor */
    .quad 0
    
    /* 64-bit code segment (0x10) */
    .word HEX(FFFF)     /* Limit low */
    .word 0             /* Base low */
    .byte 0             /* Base middle */
    .byte HEX(9A)       /* Access: Present, Code, Readable */
    .byte HEX(AF)       /* Flags: 64-bit, Granularity */
    .byte 0             /* Base high */
    
    /* 64-bit data segment (0x18) */
    .word HEX(FFFF)     /* Limit low */
    .word 0             /* Base low */
    .byte 0             /* Base middle */
    .byte HEX(92)       /* Access: Present, Data, Writable */
    .byte HEX(CF)       /* Flags: Granularity */
    .byte 0             /* Base high */
gdt_end:

gdtptr:
    .word gdt_end - gdt - 1
    .long gdt

/* Storage */
EntryPoint64:
    .quad 0

stack16:
    .word STACK16ADDR

#include "int386.inc"
#include "helpers.inc"
#include "pxe.inc"
#include "pnp.inc"

.org (FREELDR_PE_BASE - FREELDR_BASE - 1)
.byte 0
.endcode16

END