
BootLinuxKernel:
    // dl must be set to the boot drive

    /* Load segment registers */
    cli
    mov bx, HEX(9000)
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    mov ss, bx
    mov sp, HEX(9000)

    ljmp16  HEX(9020), HEX(0000)
