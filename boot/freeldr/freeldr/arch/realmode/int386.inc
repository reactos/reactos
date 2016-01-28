
#include "../../include/arch/pc/pcbios.h"

#define EFLAGS_CF HEX(01)
#define EFLAGS_ZF HEX(40)
#define EFLAGS_SF HEX(80)

Int386:
    /* Save all registers + segment registers */
    push ds
    push es
    push fs
    push gs
    pushad

    /* Get the interupt vector and patch the opcode */
    mov al, byte ptr ds:[BSS_IntVector]
    mov byte ptr ds:[Int386_vector_opcode], al

    /* Get current EFLAGS and mask CF, ZF and SF */
    pushf
    pop cx
    and cx, not (EFLAGS_CF or EFLAGS_ZF or EFLAGS_SF)

    /* Get flags CF, ZF and SF from the REGS structure */
    mov ax, word ptr cs:[BSS_RegisterSet + REGS_EFLAGS]
    and ax, (EFLAGS_CF or EFLAGS_ZF or EFLAGS_SF)

    /* Combine flags and set them */
    or ax, cx
    push ax
    popf

    /* Setup the registers */
    mov ax, word ptr cs:[BSS_RegisterSet + REGS_DS]
    mov ds, ax
    mov ax, word ptr cs:[BSS_RegisterSet + REGS_ES]
    mov es, ax
    mov ax, word ptr cs:[BSS_RegisterSet + REGS_FS]
    mov fs, ax
    mov ax, word ptr cs:[BSS_RegisterSet + REGS_GS]
    mov gs, ax

    mov eax, dword ptr cs:[BSS_RegisterSet + REGS_EAX]
    mov ebx, dword ptr cs:[BSS_RegisterSet + REGS_EBX]
    mov ecx, dword ptr cs:[BSS_RegisterSet + REGS_ECX]
    mov edx, dword ptr cs:[BSS_RegisterSet + REGS_EDX]
    mov esi, dword ptr cs:[BSS_RegisterSet + REGS_ESI]
    mov edi, dword ptr cs:[BSS_RegisterSet + REGS_EDI]
    // Don't setup ebp, we only use it as output!

    /* Call the interrupt vector */
    /*int       Int386_vector*/
    .byte HEX(0cd)
Int386_vector_opcode:
    .byte 00

    /* Save the registers */
    mov dword ptr cs:[BSS_RegisterSet + REGS_EAX], eax
    mov dword ptr cs:[BSS_RegisterSet + REGS_EBX], ebx
    mov dword ptr cs:[BSS_RegisterSet + REGS_ECX], ecx
    mov dword ptr cs:[BSS_RegisterSet + REGS_EDX], edx
    mov dword ptr cs:[BSS_RegisterSet + REGS_ESI], esi
    mov dword ptr cs:[BSS_RegisterSet + REGS_EDI], edi
    mov dword ptr cs:[BSS_RegisterSet + REGS_EBP], ebp

    mov ax, ds
    mov word ptr cs:[BSS_RegisterSet + REGS_DS], ax
    mov ax, es
    mov word ptr cs:[BSS_RegisterSet + REGS_ES], ax
    mov ax, fs
    mov word ptr cs:[BSS_RegisterSet + REGS_FS], ax
    mov ax, gs
    mov word ptr cs:[BSS_RegisterSet + REGS_GS], ax

    pushfd
    pop dword ptr cs:[BSS_RegisterSet + REGS_EFLAGS]

    popad
    pop gs
    pop fs
    pop es
    pop ds

    ret



