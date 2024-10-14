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

EXTERN _DiskStopFloppyMotor:PROC
EXTERN _Relocator16Boot:PROC
EXTERN _FrldrBootDrive:BYTE
EXTERN _FrldrBootPartition:DWORD

.code32

Regs:
    .space REGS_SIZE

/*
 * VOID __cdecl BootLinuxKernel(
 *     IN ULONG KernelSize,
 *     IN PVOID KernelCurrentLoadAddress,
 *     IN PVOID KernelTargetLoadAddress,
 *     IN UCHAR DriveNumber,
 *     IN ULONG PartitionNumber);
 */
PUBLIC _BootLinuxKernel
_BootLinuxKernel:

    /* Stop the floppy drive motor */
    call _DiskStopFloppyMotor

    /* Set all segment registers to 0x9000 */
    mov ax, HEX(9000)
    mov word ptr [Regs + REGS_DS], ax
    mov word ptr [Regs + REGS_ES], ax
    mov word ptr [Regs + REGS_FS], ax
    mov word ptr [Regs + REGS_GS], ax

    /* Set the boot drive */
    xor edx, edx
    mov dl, byte ptr [esp + 16]
    test dl, dl
    jnz set_part
    mov dl, byte ptr ds:[_FrldrBootDrive]

    /* Set the boot partition */
set_part:
    mov eax, dword ptr [esp + 20]
    test eax, eax
    jnz continue
    mov eax, dword ptr ds:[_FrldrBootPartition]
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
    mov ecx, dword ptr [esp + 4]
    test ecx, ecx   // If size is zero, do not perform relocations
    jz after_reloc

    /* Load the source and target addresses */
    mov esi, dword ptr [esp +  8] // HEX(100000) // LINUX_KERNEL_LOAD_ADDRESS
    mov edi, dword ptr [esp + 12] // HEX(10000)

//
// FIXME: Support relocating *upwards*, overlapping regions, aligned addresses,
// etc... !! See memmove code.
//
    /* Check how we should perform relocation */
    cmp edi, esi
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
    add esi, ecx
    add edi, ecx
    dec esi
    dec edi
    rep movsb
    // jmp after_reloc

after_reloc:

    push HEX(0000) // CodePointer
    push HEX(9020) // CodeSegment
    push HEX(9000) // StackPointer
    push HEX(9000) // StackSegment
    mov eax, offset Regs
    push eax
    call _Relocator16Boot

    /* We must never get there */
    int 3

END
