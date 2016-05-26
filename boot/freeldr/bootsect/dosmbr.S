/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Bootsector for ISO file system
 * PURPOSE:         Normal DOS boot sector
 *                  Ported to NASM from FreeDOS fdisk 1.2.0 by:
 * PROGRAMMERS:     Casper Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

//-----------------------------------------------------------------------
//   ENTRY (copied from freedos bootsector)
//
// IN: DL = boot drive
// OUT: DL = boot drive
//
//-----------------------------------------------------------------------

.code16
real_start:
    cli
    cld
    xor ax, ax
    mov ss, ax          // initialize stack
    mov ds, ax
    mov bp, HEX(7c00)
    lea sp, [bp-32]
    sti

    /* Copy 512 bytes of MBR to 1fe0:7c00 */
    mov ax, HEX(1FE0)
    mov es, ax
    mov si, bp
    mov di, bp
    mov cx, 256
    rep movsw

    /* Jump into relocated code */
    ljmp16 HEX(1FE0), cont
cont:

    /* Setup segment registers */
    mov ds, ax
    mov ss, ax
    xor ax, ax
    mov es, ax

    /* Search for active partition */
    lea di, [bp + HEX(1be)] // start of partition table
test_next_for_active:
    test byte ptr ds:[di], HEX(80)
    jne active_partition_found
    add di, 16                    // next table
    cmp di, HEX(07c00) + HEX(1fe) // scanned beyond end of table ??
    jb test_next_for_active

/*****************************************************************/
    call print
    .asciz "No active partition found"

WAIT_FOR_REBOOT:
    jmp WAIT_FOR_REBOOT


/*****************************************************************/
trouble_reading_drive:
    call print
    .asciz "Read error while reading drive"
    jmp WAIT_FOR_REBOOT

/*****************************************************************/

invalid_partition_code:
    call print
    .asciz "Partition signature != 55AA"

    jmp WAIT_FOR_REBOOT

/*****************************************************************/

active_partition_found:
//  call print
//  .asciz "Loading active partition"

    call read_boot_sector

    jc  trouble_reading_drive

    cmp word ptr es:[HEX(7c00)+HEX(1fe)], HEX(0aa55)
    jne invalid_partition_code

    ljmp16 0, HEX(7c00)            // and jump to boot sector code

/*****************************
 * read_boot_sector
 *
 * IN: DI--> partition info
 * OUT:CARRY
 *****************************/
read_boot_sector:
    /* check for LBA support */
    mov bx, HEX(55aa)
    mov ah, HEX(41)
    int HEX(13)

    jc  StandardBios    //  if (regs.b.x != 0xaa55 || (regs.flags & 0x01))
    cmp bx, HEX(0aa55)  //    goto StandardBios;
    jne StandardBios

    /* if DAP cannot be used, don't use LBA
       if ((regs.c.x & 1) == 0)
           goto StandardBios; */
    test cl, 1
    jz StandardBios

    jmp short LBABios


_bios_LBA_address_packet:
    .byte 16
    .byte 0
    .byte 4         // read four sectors - why not
    .byte 0
    .word HEX(7c00) // fixed boot address for DOS sector
    .word HEX(0000)

_bios_LBA_low:
    .word 0
_bios_LBA_high:
    .word 0
    .word 0,0


LBABios:
    // copy start address of partition to DAP
    mov ax, [di + 8]
    mov word ptr ds:[_bios_LBA_low], ax
    mov ax,[di + 8 + 2]
    mov word ptr ds:[_bios_LBA_high], ax

    mov ax, HEX(4200)    //  regs.a.x = LBA_READ;
    mov si, offset _bios_LBA_address_packet // regs.si = FP_OFF(&dap);

    int HEX(13)
    ret

/*****************************************************************
 * read disk, using standard BIOS
 */
StandardBios:
    mov ax, HEX(0204)               //  regs.a.x = 0x0201;
    mov bx, HEX(7c00)               //  regs.b.x = FP_OFF(buffer);

    /* regs.c.x =
       ((chs.Cylinder & 0xff) << 8) + ((chs.Cylinder & 0x300) >> 2) +
        ; chs.Sector;
         that was easy ;-) */
    mov cx, word ptr ds:[di + 2]
    mov dh, byte ptr ds:[di + 1]    //  regs.d.b.h = chs.Head;
                                    //  regs.es = FP_SEG(buffer);
    int HEX(13)
    ret

/****** PRINT
 * prints text after call to this function.
 */
print_1char:
    xor bx, bx                   // video page 0
    mov ah, HEX(0E)              // else print it
    int HEX(10)                  // via TTY mode
print:
    pop si                       // this is the first character
print1:
    lodsb                        // get token
    push si                      // stack up potential return address
    cmp al, 0                    // end of string?
    jne print_1char              // until done
    ret                          // and jump to it

.org 510
    .byte HEX(55), HEX(0aa)

.endcode16

END
