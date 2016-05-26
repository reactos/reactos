
#include <asm.inc>
#include "../../include/arch/pc/x86common.h"

#define IMAGE_DOS_HEADER_e_lfanew 60
#define IMAGE_FILE_HEADER_SIZE 20
#define IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint 16


.code16

/* fat helper code */
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

    /* Save the boot drive and partition */
    mov byte ptr ds:[BSS_BootDrive], dl
    mov byte ptr ds:[BSS_BootPartition], dh

    /* Setup a real mode stack */
    mov sp, word ptr ds:[stack16]

    /* Output first status */
    mov si, offset Msg_Starting
    call writestr

    /* Enable A20 address line */
    call EnableA20

    /* Check the CPU */
    call CheckFor64BitSupport
    test al, al
    jnz .LongModeSupported

    /* Output failure message */
    mov si, offset Msg_Unsupported
    call writestr

    /* Wait for a keypress */
    int HEX(16)
    jmp Reboot

Msg_Unsupported:
    .ascii "This CPU is not supported.", CR, LF
    .ascii "Press any key to reboot...", NUL

Msg_Starting:
    .ascii "Starting FreeLoader...", CR, LF, NUL

Msg_LongModeSupported:
    .ascii "Long mode support detected.", CR, LF, NUL

.LongModeSupported:
    /* Output status */
    mov si, offset Msg_LongModeSupported
    call writestr

    /* Load the GDT */
    lgdt fword ptr [gdtptr]

    /* Build the startup page tables */
    call BuildPageTables

    /* Store real mode entry point in shared memory */
    mov dword ptr ds:[BSS_RealModeEntry], offset RealModeEntryPoint

    /* Address the image with es segment */
    mov ax, FREELDR_PE_BASE / 16
    mov es, ax

    /* Get address of optional header */
    mov eax, dword ptr es:[IMAGE_DOS_HEADER_e_lfanew]
    add eax, 4 + IMAGE_FILE_HEADER_SIZE

    /* Get address of entry point */
    mov eax, dword ptr es:[eax + IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint]
    add eax, FREELDR_PE_BASE

    /* Save entry point */
    mov dword ptr ds:[LongModeEntryPoint], eax

    /* Restore es */
    xor ax, ax
    mov es, ax

    /* Output status */
    mov si, offset Msg_SwitchToLongMode
    call writestr

    jmp ExitToLongMode

Msg_SwitchToLongMode:
    .ascii "Switching to long mode....", CR, LF, NUL

.align 8
gdt:
    .word HEX(0000), HEX(0000), HEX(0000), HEX(0000) /* 00: NULL descriptor */
    .word HEX(0000), HEX(0000), HEX(0000), HEX(0000) /* 08:  */
    .word HEX(0000), HEX(0000), HEX(9800), HEX(0020) /* 10: long mode cs */
    .word HEX(ffff), HEX(0000), HEX(f300), HEX(00cf) /* 18: long mode ds */
    .word HEX(FFFF), HEX(0000), HEX(9E00), HEX(0000) /* 20: 16-bit real mode CS */
    .word HEX(FFFF), HEX(0000), HEX(9200), HEX(0000) /* 28: 16-bit real mode DS */
    .word HEX(FFFF), HEX(0000), HEX(9B00), HEX(00CF) /* 30: compat mode cs */

/* GDT table pointer */
gdtptr:
    .word HEX(37) /* Limit */
    .long offset gdt   /* Base Address */


CheckFor64BitSupport:
    /* Check if CPU supports CPUID */
    pushad
    pushfd
    pop eax
    mov ebx, eax
    xor eax, HEX(00200000)
    push eax
    popfd
    pushfd
    pop eax
    cmp eax,ebx
    jnz .CheckForPAE

    mov si, offset .Msg_NoCpuidSupport
    call writestr
    popad
    xor al, al
    ret

.Msg_NoCpuidSupport:
    .ascii "The system doesn't support CPUID.", CR, LF, NUL

.CheckForPAE:
    /* CPUID support detected - getting the PAE/PGE */
    mov eax,1 // Fn0000_0001 - PAE in EDX[6]
    cpuid
    and edx, HEX(00a0)
    cmp edx, HEX(00a0)
    je .CheckForLongMode

    mov si, offset .Msg_NoPAE
    call writestr
    popad
    xor al, al
    ret

.Msg_NoPAE:
    .ascii "PAE or PGE not set.", CR, LF, NUL

.CheckForLongMode:
    xor edx, edx
    mov eax, HEX(80000001)
    cpuid
    and edx, HEX(20000000)
    test edx,edx
    jnz .Success

    mov si, offset .Msg_NoLongMode
    call writestr
    popad
    xor al, al
    ret

.Msg_NoLongMode:
    .ascii "Long mode is not supported.", CR, LF, NUL

.Success:
    popad
    xor al, al
    inc al
    ret


BuildPageTables:
    pusha
    push es

    /* Get segment of the PML4 */
    mov eax, PML4_ADDRESS / 16
    mov es, ax
    cld
    xor di, di

    /* One entry in the PML4 pointing to PDP */
    mov eax, PDP_ADDRESS
    or eax, HEX(0f)
    stosd

    /* clear rest */
    xor eax, eax
    mov cx, 1023
    rep stosd

    /* One entry in the PDP pointing to PD */
    mov eax, PD_ADDRESS
    or eax, HEX(0f)
    stosd

    /* clear rest */
    xor eax, eax
    mov ecx, 1023
    rep stosd

    /* 512 entries in the PD, each defining a 2MB page each */
    mov ecx, 512
    mov eax, HEX(008f)

.Bpt2:
    mov es: [di], eax
    mov dword ptr es: [di + 4], 0
    add eax, 512 * 4096 // add 512 4k pages
    add di, 8

    /* Loop all PDEs */
    dec cx
    jnz .Bpt2

    /* Return */
    pop es
    popa
    ret


/******************************************************************************/

#define MSR_EFER HEX(C0000080)
#define LMODE_CS HEX(10)

/* This is the entry point from long mode */
RealModeEntryPoint:
    /* Disable Protected Mode */
    mov eax, cr0
    and eax, HEX(0fffffffe) // ~0x00000001
    mov cr0, eax

    /* Clear prefetch queue & correct CS */
    ljmp16 0, offset InRealMode

InRealMode:

//    mov ax, HEX(0b800)
//    mov es, ax
//    mov word ptr es:[12], HEX(0e00) + '6'

    /* Set real mode segments */
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    /* Clear out the high 16-bits of ESP */
    /* This is needed because I have one */
    /* machine that hangs when booted to dos if */
    /* anything other than 0x0000 is in the high */
    /* 16-bits of ESP. Even though real-mode */
    /* code should only use SP and not ESP. */
    xor esp, esp

    /* Restore real mode stack */
    mov sp, word ptr ds:[stack16]

   // sti /* These are ok now */

    /* Do the callback, specified by bx */
    shl bx, 1
    call word ptr ds:CallbackTable[bx]

ExitToLongMode:
    /* Disable interrupts */
    cli

    /* Set correct segment registers */
    xor ax,ax
    mov ds,ax
    mov es,ax
    mov fs,ax
    mov gs,ax
    mov ss,ax

    /* Safe current stack pointer */
    mov word ptr ds:[stack16], sp

    /* Set PAE and PGE: 10100000b */
    mov eax, HEX(00a0)
    mov cr4, eax

    /* Point cr3 at the PML4 */
    mov eax, PML4_ADDRESS
    mov cr3, eax

    /* Enable long mode */
    mov ecx, MSR_EFER
    rdmsr
    or eax, HEX(00000100)
    wrmsr

    /* Activate long mode by enabling paging and protection simultaneously,
       skipping protected mode entirely */
    mov eax, cr0
    or eax, HEX(80000001)
    mov cr0, eax

    /* Clear prefetch queue & correct CS */
    ljmp16 LMODE_CS, InLongMode
InLongMode:
    //DB 66h, 0B8h, 18h, 00h // mov ax, LMODE_DS
    //DB 66h, 8Eh, 0D8h // mov ds, ax
    //DB 66h, 66h, 0C7h, 04h, 25h, 00h, 80h, 0Bh, 00h, 31h, 0Eh
    //mov word ptr [HEX(b8000)], HEX(0e00) + '1'

    .byte HEX(0ff), HEX(25) // opcode of 64bit indirect jump
    .long 1 // relative address of LongModeEntryPoint
    nop
LongModeEntryPoint:
    .long 0, 0

    int HEX(16)
    jmp Reboot

CallbackTable:
    .word Int386
    .word Reboot
    .word ChainLoadBiosBootSectorCode
    .word PxeCallApi
    .word PnpBiosGetDeviceNodeCount
    .word PnpBiosGetDeviceNode
    .word 0 // BootLinuxKernel

    /* 16-bit stack pointer */
stack16:
    .word STACK16ADDR


#include "int386.inc"
#include "pxe.inc"
#include "pnp.inc"
#include "helpers.inc"

.org (FREELDR_PE_BASE - FREELDR_BASE - 1)
.byte 0
.endcode16

END
