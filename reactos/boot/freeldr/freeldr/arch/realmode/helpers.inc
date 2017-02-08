
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

    /* Set the word at location 40h:72h to 0 (cold reboot) */
    mov word ptr ds:[HEX(0472)], HEX(0)

    /* and jump to location F000h:FFF0h in ROM */
    ljmp16 HEX(0F000), HEX(0FFF0)


ChainLoadBiosBootSectorCode:
    /* Load segment registers */
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, HEX(7C00)

    /* Jump to the bootsector code */
    ljmp16 HEX(0000), HEX(7C00)
