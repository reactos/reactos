/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Real mode helper code for all architectures
 * COPYRIGHT:   Copyright 2024-2025 Daniel Victor <ilauncherdeveloper@gmail.com>
 */

#define RELOCATOR_BLOCK_SIZE            HEX(200)
#define RELOCATOR_BLOCK_SIZE_SEGMENT    (RELOCATOR_BLOCK_SIZE / 16)

/* This function will Update BSS_CurrentBaseAddress, instruction pointer, code segment and relocate if necessary */
RelocateFreeLdr:
    /* Clear direction flag */
    cld

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
    sub bx, offset .SubCall - FREELDR_BASE

    /* Load the current code segment into eax */
    mov ax, cs

    /* Shift the segment address to get its physical address (real mode calculation) */
    shl eax, 4

    /* Add the physical segment base to the relative base address */
    add eax, ebx

    /* Store the calculated base address (low 32 bits) */
    mov dword ptr ds:[BSS_CurrentBaseAddress], eax

.FixBaseAddress:
    /* Don't relocate if it's not necessary */
    cmp eax, FREELDR_BASE
    jz .NotNecessaryRelocation

    /* Setup source segment */
    shr eax, 4
    mov ds, ax

    /* Move the realmode FREELDR code to TEMPCODE16_BASE */
    mov di, TEMPCODE16_BASE
    xor si, si
    mov cx, FREELDR_PE_BASE - FREELDR_BASE
    rep movsb

    ljmp16 TEMPCODE16_BASE / 16, (.FixBaseAddress2 - FREELDR_BASE)
.FixBaseAddress2:
    /* Save DX */
    push dx

    /* Prepare the registers for relocation */
    mov dx, (MEMORY_MARGIN - FREELDR_BASE) / 16
    mov eax, dword ptr es:[BSS_CurrentBaseAddress]
    shr eax, 4
    mov ds, ax
    mov ax, FREELDR_BASE / 16
    mov es, ax

    /* If current base address is lower than the final one then just copy it backwards */
    cmp dword ptr fs:[BSS_CurrentBaseAddress], FREELDR_BASE
    jb .PreFixLoopInverted
.FixLoop:
    /* Abort the loop if DX is below than `RELOCATOR_BLOCK_SIZE_SEGMENT` */
    cmp dx, RELOCATOR_BLOCK_SIZE_SEGMENT
    jb .FinishedRelocation

    /* Copy `RELOCATOR_BLOCK_SIZE` bytes from source to destination */
    xor di, di
    xor si, si
    mov cx, RELOCATOR_BLOCK_SIZE
    rep movsb

    /* Increment DS */
    mov ax, ds
    add ax, RELOCATOR_BLOCK_SIZE_SEGMENT
    mov ds, ax

    /* Increment ES */
    mov ax, es
    add ax, RELOCATOR_BLOCK_SIZE_SEGMENT
    mov es, ax

    /* Repeat the loop */
    sub dx, RELOCATOR_BLOCK_SIZE_SEGMENT
    jmp .FixLoop
.PreFixLoopInverted:
    /* Move DS to the end */
    mov ax, ds
    add ax, dx
    mov ds, ax

    /* Move ES to the end */
    mov ax, es
    add ax, dx
    mov es, ax
.FixLoopInverted:
    /* Abort the loop if DX is below than `RELOCATOR_BLOCK_SIZE_SEGMENT` */
    cmp dx, RELOCATOR_BLOCK_SIZE_SEGMENT
    jb .FinishedRelocation

    /* Decrement DS */
    mov ax, ds
    sub ax, RELOCATOR_BLOCK_SIZE_SEGMENT
    mov ds, ax

    /* Decrement ES */
    mov ax, es
    sub ax, RELOCATOR_BLOCK_SIZE_SEGMENT
    mov es, ax

    /* Copy `RELOCATOR_BLOCK_SIZE` bytes from source to destination */
    std
    mov di, RELOCATOR_BLOCK_SIZE
    mov si, di
    mov cx, di
    rep movsb

    /* Repeat the loop */
    sub dx, RELOCATOR_BLOCK_SIZE_SEGMENT
    jmp .FixLoopInverted

.FinishedRelocation:
    /* Restore DX and restart FreeLoader with new base address */
    pop dx
    ljmp16 FREELDR_BASE / 16, 0

.NotNecessaryRelocation:
    ret
