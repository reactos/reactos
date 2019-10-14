
Empty8042:
    .word HEX(00eb), HEX(00eb)      // jmp $+2, jmp $+2
    in al, HEX(64)
    cmp al, HEX(0ff)                // legacy-free machine without keyboard
    jz Empty8042_ret                // controllers on Intel Macs read back 0xFF
    test al, 2
    jnz Empty8042
Empty8042_ret:
    ret

EnableA20:
    pusha
    call Empty8042
    mov al, HEX(0D1)                // command write
    out HEX(064), al
    call Empty8042
    mov al, HEX(0DF)                // A20 on
    out HEX(060), al
    call Empty8042
    mov al, HEX(0FF)                // pulse output port
    out HEX(064), al
    call Empty8042
    popa
    ret

DisableA20:
    pusha
    call Empty8042
    mov al, HEX(0D1)                // command write
    out HEX(064), al
    call Empty8042
    mov al, HEX(0DD)                // A20 off
    out HEX(060), al
    call Empty8042
    mov al, HEX(0FF)                // pulse output port
    out HEX(064), al
    call Empty8042
    popa
    ret

/*
 * writestr
 * si = pointer to zero terminated string
 */
writestr:
    pushfd
    pushad
writestr_top:
    lodsb
    and al, al
    jz   writestr_end
    call writechr
    jmp  short writestr_top
writestr_end:
    popad
    popfd
    ret

/*
 * writechr
 * al = character to output
 */
writechr:
    pushf
    pusha
    mov ah, HEX(0E)
    xor bx, bx
    int HEX(10)
    popa
    popf
    ret

//
// writehex[248]: Write a hex number in (AL, AX, EAX) to the console
//
writehex2:
    pushfd
    pushad
    shl    eax, 24
    mov cx, 2
    jmp    short writehex_common
writehex4:
    pushfd
    pushad
    shl    eax, 16
    mov cx, 4
    jmp    short writehex_common
writehex8:
    pushfd
    pushad
    mov cx, 8
writehex_common:
.loop:
    rol    eax, 4
    push    eax
    and    al, HEX(0F)
    cmp al, 10
    jae    .high
.low:
    add    al, '0'
    jmp    short .ischar
.high:
    add    al, 'A'-10
.ischar:
    call writechr
    pop    eax
    loop    .loop
    popad
    popfd
    ret


Reboot:
    cli

    /* Disable A20 address line */
    call DisableA20

    /* Set the video back to 80x25 text mode */
    mov ax, HEX(0003)
    int HEX(10)

    /* Set the word at location 40h:72h to 0 (cold reboot) */
    mov word ptr ds:[HEX(0472)], HEX(0)

    /* and jump to location F000h:FFF0h in ROM */
    ljmp16 HEX(0F000), HEX(0FFF0)


Relocator16Boot:
    cli

    /* Disable A20 address line */
    call DisableA20

    /* Set the video back to 80x25 text mode */
    mov ax, HEX(0003)
    int HEX(10)

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
    // Don't setup ebp, we only use it as output! <-- FIXME!

    /* Jump to the new CS:IP (e.g. jump to bootsector code...) */
    .byte HEX(0EA) // ljmp16 segment:offset
Relocator16Address:
    .word HEX(7C00) // Default offset
    .word HEX(0000) // Default segment
    nop
