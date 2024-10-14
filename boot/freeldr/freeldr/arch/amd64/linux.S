/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <asm.inc>
#include <arch/pc/x86common.h>
#include <arch/pc/pcbios.h>

EXTERN DiskStopFloppyMotor:PROC
EXTERN Relocator16Boot:PROC
EXTERN FrldrBootDrive:BYTE
EXTERN FrldrBootPartition:DWORD

.code64

Regs:
    .space REGS_SIZE

/*
 * VOID __cdecl BootLinuxKernel(
 *     IN ULONG KernelSize<ecx>,
 *     IN PVOID KernelCurrentLoadAddress<rdx>,
 *     IN PVOID KernelTargetLoadAddress<r8>,
 *     IN UCHAR DriveNumber<r9b>,
 *     IN ULONG PartitionNumber<rsp+40>);
 */
PUBLIC BootLinuxKernel
BootLinuxKernel:

    /* Save home registers */
    mov r11, rsp
    mov dword ptr [r11 +  8], ecx
    mov qword ptr [r11 + 16], rdx
    mov qword ptr [r11 + 24], r8
    mov  byte ptr [r11 + 32], r9b

    /* Save non-volatile registers */
    push rsi
    push rdi

    /* Allocate stack space for home registers (+ alignment) */
    sub rsp, (8*4 + 8)
    //.ENDPROLOG

    /* Stop the floppy drive motor */
    call DiskStopFloppyMotor

    /* Set all segment registers to 0x9000 */
    mov ax, HEX(9000)
    mov word ptr [Regs + REGS_DS], ax
    mov word ptr [Regs + REGS_ES], ax
    mov word ptr [Regs + REGS_FS], ax
    mov word ptr [Regs + REGS_GS], ax

    /* Set the boot drive */
    xor edx, edx
    mov dl, byte ptr [r11 + 32]
    test dl, dl
    jnz set_part
    mov dl, byte ptr /*ds:*/[FrldrBootDrive]

    /* Set the boot partition */
set_part:
    mov eax, dword ptr [r11 + 40]
    test eax, eax
    jnz continue
    mov eax, dword ptr /*ds:*/[FrldrBootPartition]
continue:
    /* Store the 1-byte truncated partition number in DH */
    mov dh, al

    mov dword ptr [Regs + REGS_EDX], edx

    /*
     * Relocate the kernel image to its final destination (can be as low as 0x10000).
     * The reason we can overwrite low memory is because this code executes
     * between 0000:8000 and 0000:FFFF. That leaves space for 32k of code
     * before we start interfering with Linux kernel address space.
     */

    /* Get KernelSize in ECX */
    xor rcx, rcx    // Put the 64..32 higher bits to zero
    mov ecx, dword ptr [r11 + 8]
    test rcx, rcx   // If size is zero, do not perform relocations
    jz after_reloc

    /* Load the source and target addresses */
    mov rsi, qword ptr [r11 + 16] // HEX(100000) // LINUX_KERNEL_LOAD_ADDRESS
    mov rdi, qword ptr [r11 + 24] // HEX(10000)

//
// FIXME: Support relocating *upwards*, overlapping regions, aligned addresses,
// etc... !! See memmove code.
//
    /* Check how we should perform relocation */
    cmp rdi, rsi
    je after_reloc  // target == source: do not perform relocations
    ja reloc_up     // target  > source: relocate up
//  jb reloc_down   // target  < source: relocate down (default)

reloc_down:
    /* Move the kernel down - Start with low addresses and increment them */
    cld
#if 0
    rep movsb
#else
    mov edx, ecx            // Copy the total number of bytes in EDX
    and edx, HEX(0FFFFFFFC) // Number of bytes we copy using DWORDs
    xor edx, ecx            // Number of remaining bytes to copy after the DWORDs
    shr ecx, 2      // Count number of DWORDs
    rep movsd       // Move DWORDs
    mov ecx, edx    // Count number of remaining bytes
    rep movsb       // Move bytes
#endif
    jmp after_reloc

reloc_up:
    /* Move the kernel up - Start with high addresses and decrement them */
    std
    add rsi, rcx
    add rdi, rcx
    dec rsi
    dec rdi
    rep movsb
    // jmp after_reloc

after_reloc:

    mov word ptr [rsp-8 + 40], HEX(0000) // CodePointer
    mov r9w, HEX(9020) // CodeSegment
    mov r8w, HEX(9000) // StackPointer
    mov  dx, HEX(9000) // StackSegment
    mov rcx, offset Regs
    call Relocator16Boot

    /* Cleanup and return */
    add rsp, (8*4 + 8)
    pop rdi
    pop rsi

    /* We must never get there */
    int 3

END
