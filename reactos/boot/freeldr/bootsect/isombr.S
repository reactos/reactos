/*
 * PROJECT:     ReactOS MBR Boot Sector for ISO file system ("isohybrid mode")
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * COPYRIGHT:   Copyright 2017 Colin Finck <colin@reactos.org>
 */

#include <asm.inc>

.code16

// The "isohybrid" tool patching our ISO will write the LBA of the El Torito BIOS Boot Sector to this address.
pBootSectorLBA = start+432

// Our BIOS Boot Sector contains this signature at the specified position and our third-party imported "isohybrid" patching tool checks for it.
// This is also why it's the same signature ISOLINUX uses.
HybridSignature = HEX(7078C0FB)
pBootSectorHybridSignature = HEX(7C00)+64
pBootSectorHybridEntryPoint = HEX(7C00)+64+4

start:
    // The MBR needs to start with 33h, because some dumb BIOSes check for that (cf. syslinux commit d0f275981c9289dc4b8df64e72cd9902bf85aebe).
    // The following line encodes a "xor ax, ax" (but it's not the only way to encode it, so we can't just write that instruction here).
    .byte HEX(33), HEX(C0)

    // Set up our stack and a flat addressing model.
    cli
    mov ss, ax
    mov sp, offset start
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    sti

    // Our boot sector has been loaded to address 0x7C00.
    // Relocate our 512 bytes boot sector to the given base address (should be 0x7000).
    cld
    mov cx, 512 / 4
    mov si, HEX(7C00)
    mov di, offset start
    rep movsd

    // Jump to the relocated code.
    ljmp16 0, relocated

relocated:
    // Prepare the Disk Access Packet (DAP) for INT 13h, Function 42h: Extended Read Sectors from Drive.
    // Read 4 sectors to address 7C00h, which is exactly our 2K-sized BIOS Boot Sector.
    push 0
    push 0
    push dword ptr es:[pBootSectorLBA]
    push es
    push HEX(7C00)
    push 4
    push HEX(10)

    // Call the BIOS function. Note that we haven't clobbered DL up to this point, so the Drive Number passed by the BIOS is still there.
    // Read errors are indicated by the Carry Flag.
    mov ah, HEX(42)
    mov si, sp
    int HEX(13)
    jc read_error

    // Verify the Hybrid Signature.
    cmp dword ptr es:[pBootSectorHybridSignature], HybridSignature
    jne invalid_signature

    // Signature is valid, so jump to the entry point for the hybrid code.
    ljmp16 0, pBootSectorHybridEntryPoint

read_error:
    call die_with_error
    .ascii "ISOMBR: Read Error!", CR, LF

invalid_signature:
    call die_with_error
    .ascii "ISOMBR: Invalid Boot Sector Hybrid Signature!", CR, LF

die_with_error:
    // Fetch the message to output stored at the return address on the stack.
    pop si

    // Call BIOS INT 10h, Function 0Eh to output a single character.
    // Do this in a loop and stop after we have printed the newline LF character.
next_character:
    lodsb
    mov ah, HEX(0E)
    xor bx, bx
    int HEX(10)
    cmp al, 10
    jne next_character

    // Die gracefully, that means in an infinite HLT loop to not put any stress on the CPU.
die:
    hlt
    jmp die

// The "isohybrid" tool will add the remaining information, including the infamous 0xAA55 MBR signature.
.org 431
    .byte 0

.endcode16

END
