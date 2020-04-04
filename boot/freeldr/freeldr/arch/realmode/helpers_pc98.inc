/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Real mode helper code for NEC PC-98 series
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/*
 * Enable the A20 address line
 */
EnableA20:
    push ax

    /* Unmask A20 line */
    xor ax, ax
    out HEX(0F2), al
    mov al, HEX(02)
    out HEX(0F6), al

    pop ax
    ret

/*
 * Disable the A20 address line
 */
DisableA20:
    push ax

    /* Mask A20 line */
    mov al, HEX(03)
    out HEX(0F6), al

    pop ax
    ret

/*
 * Prints a string
 * SI = pointer to zero terminated string
 */
writestr:
    pushfd
    pushad

.writestr_loop:
    lodsb

    /* Null test */
    or al, al
    jz short .writestr_end

    /* CR test */
    cmp al, HEX(0D)
    jz short .writestr_cr

    call writechr
    jmp short .writestr_loop

.writestr_cr:
    mov ax, word ptr VramOffset
    mov dl, 80 * 2
    div dl
    inc ax
    mul dl
    mov word ptr VramOffset, ax

    /* Skip the next LF character */
    inc si
    jmp short .writestr_loop

.writestr_end:
    popad
    popfd
    ret

/*
 * writechr
 * AL = character to output
 */
writechr:
    pushf
    pusha

    /* High-resolution mode check */
    test byte ptr ds:[HEX(501)], HEX(08)
    jz .writechr_normal
    push HEX(0E000)
    jmp short .writechr_test_done
.writechr_normal:
    push HEX(0A000)

.writechr_test_done:
    pop es
    mov di, word ptr VramOffset
    xor ah, ah
    stosw
    mov word ptr VramOffset, di

    popa
    popf
    ret

VramOffset:
    .word 0

/*
 * Writes a hex number in (AL, AX, EAX) to the console
 */
writehex2:
    pushfd
    pushad
    shl eax, 24
    mov cx, 2
    jmp short writehex_common
writehex4:
    pushfd
    pushad
    shl eax, 16
    mov cx, 4
    jmp short writehex_common
writehex8:
    pushfd
    pushad
    mov cx, 8
writehex_common:
.loop:
    rol eax, 4
    push eax
    and al, HEX(0F)
    cmp al, 10
    jae .high
.low:
    add al, '0'
    jmp short .ischar
.high:
    add al, 'A'-10
.ischar:
    call writechr
    pop eax
    loop .loop
    popad
    popfd
    ret

/*
 * Reboots the computer
 */
Reboot:
    cli

    /* Disable A20 address line */
    call DisableA20

    /* Enable SHUT0 */
    mov al, HEX(0F)
    out HEX(37), al

    /* Enable SHUT1 */
    mov al, HEX(0B)
    out HEX(37), al

    /* Activate the CPU reset line */
    xor ax, ax
    out HEX(0F0), al

.RebootLoop:
    hlt
    jmp short .RebootLoop

/*
 * Jumps to the bootsector code
 */
Relocator16Boot:
    cli

    /* Disable A20 address line */
    call DisableA20

    /* Stop displaying graphics */
    mov ax, HEX(4100)
    int HEX(18)

    /* Cursor off */
    mov ax, HEX(1200)
    int HEX(18)

    /* Clear text screen */
    mov ax, HEX(1600)
    mov dx, HEX(0E120)
    int HEX(18)

    /* Start displaying text */
    mov ax, HEX(0C00)
    int HEX(18)

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

    /* Setup the segment registers */
    mov ax, word ptr cs:[BSS_RegisterSet + REGS_DS]
    mov ds, ax
    mov ax, word ptr cs:[BSS_RegisterSet + REGS_ES]
    mov es, ax
    mov ax, word ptr cs:[BSS_RegisterSet + REGS_FS]
    mov fs, ax
    mov ax, word ptr cs:[BSS_RegisterSet + REGS_GS]
    mov gs, ax

    /* Patch the jump address (segment:offset) */
    mov eax, dword ptr cs:[BSS_RealModeEntry]
    mov dword ptr cs:[Relocator16Address], eax

    /* Switch the stack (segment:offset) */
    mov eax, dword ptr cs:[BSS_CallbackReturn]
    shr eax, 16
    mov ss, ax
    mov eax, dword ptr cs:[BSS_CallbackReturn]
    and eax, HEX(0FFFF)
    mov esp, eax

    /* Setup the registers */
    mov eax, dword ptr cs:[BSS_RegisterSet + REGS_EAX]
    mov ebx, dword ptr cs:[BSS_RegisterSet + REGS_EBX]
    mov ecx, dword ptr cs:[BSS_RegisterSet + REGS_ECX]
    mov edx, dword ptr cs:[BSS_RegisterSet + REGS_EDX]
    mov esi, dword ptr cs:[BSS_RegisterSet + REGS_ESI]
    mov edi, dword ptr cs:[BSS_RegisterSet + REGS_EDI]
    mov ebp, dword ptr cs:[BSS_RegisterSet + REGS_EBP]

    /* Jump to the new CS:IP */
    .byte HEX(0EA) /* ljmp16 segment:offset */
Relocator16Address:
    .word HEX(0000) /* Default offset */
    .word HEX(1FC0) /* Default segment */
    nop
