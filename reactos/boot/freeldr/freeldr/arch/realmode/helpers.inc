
Empty8042:
    .word 0x00eb,0x00eb		// jmp $+2, jmp $+2
    in al, 0x64
    cmp al, 0xff				// legacy-free machine without keyboard
    jz empty_8042_ret			// controllers on Intel Macs read back 0xFF
    test al, 0x02
    jnz x86_16_Empty8042
empty_8042_ret:
	ret

EnableA20:
    pusha
    call x86_16_Empty8042
    mov al, 0xD1				// command write
    out 0x64, al
    call x86_16_Empty8042
    mov al, 0xDF				// A20 on
    out 0x60, al
    call x86_16_Empty8042
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
	and	al, al
	jz	writestr_end
	call	writechr
	jmp	short writestr_top
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
	mov	ah, 0x0E
	xor	bx, bx
	int	0x10
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

SoftReboot:
	mov ax, HEX(40)
	mov ds, ax
	mov si, HEX(72)

	/* Set the word at location 40:72 to 1234h */
	mov word ptr [si], HEX(1234)

	/* and jump to location FFFF:0 in ROM */
	ljmp16	HEX(0FFFF), HEX(0000)
