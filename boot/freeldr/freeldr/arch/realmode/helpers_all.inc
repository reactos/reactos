/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Real mode helper code for all architectures
 * COPYRIGHT:   Copyright 2024-2025 Daniel Victor <ilauncherdeveloper@gmail.com>
 */

Relocator16SS:
    .word 0
Relocator16EAX:
    .long 0
Relocator16ESP:
    .long 0

cs_writestr:
    /* Save necessary registers */
    push ax
    push ds

    /* Prepare ds before writestr call */
    mov ax, cs
    mov ds, ax

    /* Display the message */
    call writestr

    /* Restore necessary registers and return */
    pop ds
    pop ax
    ret

/* This function will Update BSS_CurrentBaseAddress, instruction pointer and code segment */
UpdateCurrentCodeSegment:
    /* Clear necessary registers */
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx

    /* Call the sub function to get the base address */
    call .SubCall
.SubCall:
    /* Pop the return address into bx. Since the return address is consumed,
     * this function does not return to UpdateCurrentCodeSegment */
    pop bx

    /* Calculate the base address by adjusting the offset */
    sub bx, offset .SubCall

    /* Load the current code segment into eax */
    mov ax, cs

    /* Shift the segment address to get its physical address (real mode calculation) */
    shl eax, 4

    /* Add the physical segment base to the relative base address */
    add eax, ebx

    /* Store the calculated base address (low 32 bits) */
    mov dword ptr ds:[BSS_CurrentBaseAddress], eax

    /* Clear the high 32 bits (not used in 32-bit environments) */
    mov dword ptr ds:[BSS_CurrentBaseAddress + 4], ecx

    /* Save the return address offset */
    mov ecx, ebx

    /* Convert the base address to segment and set the offset */
    mov ebx, eax
    shr ebx, 4
    mov ax, offset .Return

    /* Change the CS and IP using retf method */
    push bx
    push ax
    retf
.Return:
    /* Relocate the return value to get offset */
    pop ax
    sub ax, cx
    push ax

    /* Return to the caller */
    ret
