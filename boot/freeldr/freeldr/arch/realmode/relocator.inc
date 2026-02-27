/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Real-mode FreeLoader relocator
 * COPYRIGHT:   Copyright 2024-2025 Daniel Victor <ilauncherdeveloper@gmail.com>
 */

#define RELOCATOR_BLOCK_SIZE            HEX(200)
#define RELOCATOR_BLOCK_SIZE_SEGMENT    (RELOCATOR_BLOCK_SIZE / 16)

#define RELOCATOR_FUNCTION_BASE         FREELDR_BASE

#define RELOCATOR_PROGRAM_SIZE          (MEMORY_MARGIN - FREELDR_BASE)
#define RELOCATOR_PROGRAM_SIZE_BLOCKS   ((RELOCATOR_PROGRAM_SIZE + (RELOCATOR_BLOCK_SIZE - 1)) / RELOCATOR_BLOCK_SIZE)
#define RELOCATOR_PROGRAM_SIZE_ALIGNED  (RELOCATOR_PROGRAM_SIZE_BLOCKS * RELOCATOR_BLOCK_SIZE)

/* WARNING: This function must be called at the real-mode entry point since it restarts FreeLoader
 * This function will update BSS_CurrentSegmentBaseAddress and relocate if necessary */
RelocateFreeLdr:
    /* Clear direction flag */
    cld

    /* Call the sub-function to retrieve the base address */
    call .SubCall
.SubCall:
    /* Pop the return address into BX. Since the return address is consumed,
     * this function does not return to RelocateFreeLdr */
    pop bx

    /* Calculate IP base address and convert to segment value */
	sub bx, offset .SubCall - RELOCATOR_FUNCTION_BASE
	shr bx, 4

    /* Load CS base address into AX and add it with IP base address */
	mov ax, cs
	add ax, bx

    /* Store the calculated base address (low 16 bits) */
    mov word ptr fs:[BSS_CurrentSegmentBaseAddress], ax

.RelocateFrLdr16:
    /* If the current base address is the same as FREELDR_BASE then it's not necessary to relocate */
    cmp ax, FREELDR_BASE / 16
    jz .NotNecessaryRelocation

    /* Setup source segment */
    mov ds, ax

    /* Setup destination segment */
    xor ax, ax
    mov es, ax

    /* Copy the realmode part of FRLDR16 code to TEMPCODE16_BASE */
    mov di, TEMPCODE16_BASE
    mov si, offset RELOCATOR_FUNCTION_START - RELOCATOR_FUNCTION_BASE
    mov cx, offset RELOCATOR_FUNCTION_END - RELOCATOR_FUNCTION_START
    rep movsb

    /* Jump to relocated realmode part */
    ljmp16 TEMPCODE16_BASE / 16, 0

RELOCATOR_FUNCTION_START:

.SetupRelocation:
    /* Save DX (contains the boot drive and partition numbers) */
    push dx

    /* Prepare the copy size in 16-byte blocks */
    mov dx, RELOCATOR_PROGRAM_SIZE_ALIGNED / 16

    /* Prepare the destination register */
    mov ax, FREELDR_BASE / 16
    mov es, ax

    /* Prepare the source register */
    mov ax, word ptr fs:[BSS_CurrentSegmentBaseAddress]
    mov ds, ax

    /* If current base address is lower than the final one then just copy it backwards */
    cmp ax, FREELDR_BASE / 16
    jb .SetupRelocateLoopBackward
.RelocateLoopForward:
    /* Abort the loop if DX is below `RELOCATOR_BLOCK_SIZE_SEGMENT` */
    cmp dx, RELOCATOR_BLOCK_SIZE_SEGMENT
    jb .FinishedRelocation

    /* Copy `RELOCATOR_BLOCK_SIZE` bytes from source to destination */
    xor di, di
    xor si, si
    mov cx, RELOCATOR_BLOCK_SIZE
    rep movsb

    /* Increment ES */
    mov ax, es
    add ax, RELOCATOR_BLOCK_SIZE_SEGMENT
    mov es, ax

    /* Increment DS */
    mov ax, ds
    add ax, RELOCATOR_BLOCK_SIZE_SEGMENT
    mov ds, ax

    /* Repeat the loop */
    sub dx, RELOCATOR_BLOCK_SIZE_SEGMENT
    jmp .RelocateLoopForward

.SetupRelocateLoopBackward:
    /* Set direction flag for inverted copy */
    std

    /* Move destination register to the end */
    mov ax, es
    add ax, dx
    mov es, ax

    /* Move source register to the end */
    mov ax, ds
    add ax, dx
    mov ds, ax
.RelocateLoopBackward:
    /* Abort the loop if DX is below `RELOCATOR_BLOCK_SIZE_SEGMENT` */
    cmp dx, RELOCATOR_BLOCK_SIZE_SEGMENT
    jb .FinishedRelocation

    /* Decrement ES */
    mov ax, es
    sub ax, RELOCATOR_BLOCK_SIZE_SEGMENT
    mov es, ax

    /* Decrement DS */
    mov ax, ds
    sub ax, RELOCATOR_BLOCK_SIZE_SEGMENT
    mov ds, ax

    /* Copy `RELOCATOR_BLOCK_SIZE` bytes from source to destination */
    mov di, RELOCATOR_BLOCK_SIZE - 1
    mov si, di
    mov cx, RELOCATOR_BLOCK_SIZE
    rep movsb

    /* Repeat the loop */
    sub dx, RELOCATOR_BLOCK_SIZE_SEGMENT
    jmp .RelocateLoopBackward

.FinishedRelocation:
    /* Restore DX and restart FreeLoader with new base address */
    pop dx
    ljmp16 FREELDR_BASE / 16, 0

RELOCATOR_FUNCTION_END:

.NotNecessaryRelocation:
    ret
