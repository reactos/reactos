/*
 * PROJECT:     FreeLoader
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Add some helpers functions and flags necessary for the relocation.
 * COPYRIGHT:   Copyright 2024 Daniel Victor <ilauncherdeveloper@gmail.com>
 */

FirstBoot:
    .byte 1

GetBaseAddress:
    /* Clear necessary registers */
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx

    /* Call the sub function to get the base address */
    call .SubCall
.SubCall:
    /* Pop the return address into bx (lower 16 bits) */
    pop bx

    /* Calculate the base address by adjusting the offset */
    sub bx, offset .SubCall - FREELDR_BASE

    /* Load the current code segment */
    mov ax, cs

    /* Shift the segment address to get its physical address (real mode calculation) */
    shl eax, 4

    /* Add the physical segment base to the relative base address */
    add eax, ebx

    /* Store the calculated base address (low 32 bits) */
    mov dword ptr ds:[BSS_BaseAddress], eax

    /* Clear the high 32 bits (not used in 32-bit environments) */
    mov dword ptr ds:[BSS_BaseAddress + 4], ecx

    /* Return to the caller */
    ret

CheckNtldrBoot:
    /* Setup the extra segment to base address */
    mov eax, dword ptr ds:[BSS_BaseAddress]
    shr eax, 4
    mov es, ax

    /* Get the offset to the FirstBoot flag */
    mov si, offset FirstBoot - FREELDR_BASE

    /* Don't clear the necessary variables if it's not the first boot */
    cmp byte ptr es:[si], 0
    jz .IsNotTheFirstBoot

    /* Clear the necessary variables */
    xor eax, eax
    mov byte ptr ds:[BSS_BootedViaNtldr], al
    mov dword ptr ds:[BSS_NtldrAddress], eax
    mov dword ptr ds:[BSS_NtldrAddress + 4], eax
    mov byte ptr es:[si], al

.IsNotTheFirstBoot:
    /* Check if it's booted as NTLDR */
    mov eax, dword ptr ds:[BSS_BaseAddress]
    cmp eax, NTLDR_BASE
    jae RelocateFreeldr

    ret

RelocateFreeldr:
    /* Prepare the segments for the relocation */
    mov eax, FREELDR_BASE / 16
    mov es, ax
    mov eax, dword ptr ds:[BSS_BaseAddress]
    shr eax, 4
    mov ds, ax
    shl eax, 4

    /* Prepare the registers for 63KB relocation */
    xor si, si
    xor di, di
    mov cx, HEX(FFFF)

    /* Start the relocation */
    rep movsb

    /* Jump to the relocated code */
    ljmp16 0, .RelocatedFreeldr

.RelocatedFreeldr:
    /* Save the source register */
    mov esp, eax

    /* Clear the segments registers */
    xor eax, eax
    mov ds, ax
    mov es, ax

    /* Set the variables so Freeldr knows we booted via NTLDR */
    mov byte ptr ds:[BSS_BootedViaNtldr], 1
    mov dword ptr ds:[BSS_NtldrAddress], esp

    /* Jump to the relocated Entry Point */
    ljmp16 0, FREELDR_BASE
