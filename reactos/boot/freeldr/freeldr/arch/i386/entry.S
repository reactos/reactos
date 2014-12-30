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

EXTERN _BootMain:PROC
EXTERN _InitIdt:PROC
EXTERN _i386Idt:DWORD
//EXTERN _i386idtptr:FWORD

.code32

PUBLIC _RealEntryPoint
_RealEntryPoint:

    /* Setup segment selectors */
    mov ax, PMODE_DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    /* Setup protected mode stack */
    mov esp, dword ptr ds:[stack32]

    /* Load the IDT */
#ifdef _USE_ML
    lidt fword ptr ds:[i386idtptr]
#else
    lidt i386idtptr
#endif

    /* Continue execution */
    jmp dword ptr ds:[ContinueAddress]

ContinueAddress:
    .long _FrldrStartup


_FrldrStartup:

    /* Store BootDrive and BootPartition */
    mov byte ptr ds:[_FrldrBootDrive], dl
    xor eax, eax
    mov al, dh
    mov dword ptr ds:[_FrldrBootPartition], eax

    /* Patch long jump with real mode entry point */
    mov eax, dword ptr ds:[BSS_RealModeEntry]
    mov dword ptr ds:[SwitchToReal16Address], eax

    /* Initialize the idt */
    call _InitIdt

#ifndef _USE_ML
    /* Clean out bss */
    xor eax, eax
    mov edi, offset __bss_start__
    mov ecx, offset __bss_end__ + 3
    sub ecx, edi
    shr ecx, 2
    rep stosd

    /* Pass the command line to BootMain */
    mov eax, offset cmdline
#else
    xor eax, eax
#endif

    /* GO! */
    push eax
    call _BootMain

    /* We should never get here */
stop:
    jmp stop
    nop
    nop

Int386_regsin:
    .long 0
Int386_regsout:
    .long 0

/*
 * int Int386(int ivec, REGS* in, REGS* out);
 */
PUBLIC _Int386
_Int386:

    /* Get the function parameters */
    mov eax, dword ptr [esp + 4]
    mov dword ptr ds:[BSS_IntVector], eax
    mov eax, dword ptr [esp + 8]
    mov dword ptr [Int386_regsin], eax
    mov eax, dword ptr [esp + 12]
    mov dword ptr [Int386_regsout], eax

    /* Save all registers + segment registers */
    push ds
    push es
    push fs
    push gs
    pusha

    /* Copy input registers */
    mov esi, dword ptr [Int386_regsin]
    mov edi, BSS_RegisterSet
    mov ecx, REGS_SIZE / 4
    rep movsd

    /* Set the function ID */
    mov bx, FNID_Int386

    /* Set continue address and switch to real mode */
    mov dword ptr [ContinueAddress], offset Int386_return
    jmp SwitchToReal

Int386_return:

    /* Copy output registers */
    mov esi, BSS_RegisterSet
    mov edi, dword ptr [Int386_regsout]
    mov ecx, REGS_SIZE / 4
    rep movsd

    popa
    pop gs
    pop fs
    pop es
    pop ds
    ret


/*
 * U16 PxeCallApi(U16 Segment, U16 Offset, U16 Service, VOID *Parameter);
 *
 * RETURNS:
 */
PUBLIC _PxeCallApi
_PxeCallApi:
    push ebp
    mov ebp, esp

    pusha
    push es

    /* copy entry point */
    mov eax, [ebp + 8]
    shl eax, 16
    mov ax, [ebp + 12]
    mov dword ptr ds:[BSS_PxeEntryPoint], eax

    /* copy function */
    mov ax, [ebp + 16]
    mov word ptr ds:[BSS_PxeFunction], ax

    /* convert pointer to data buffer to segment/offset */
    mov eax, [ebp + 20]
    shr eax, 4
    and eax, HEX(0f000)
    mov word ptr ds:[BSS_PxeBufferSegment], ax
    mov eax, [ebp + 20]
    and eax, HEX(0ffff)
    mov word ptr ds:[BSS_PxeBufferOffset], ax

    /* Set the function ID and call realmode */
    mov bx, FNID_PxeCallApi
    call i386CallRealMode

    pop es
    popa

    mov esp, ebp
    pop ebp

    mov ax, word ptr [BSS_PxeResult]

    ret


PUBLIC _Reboot
_Reboot:
    /* Set the function ID */
    mov bx, FNID_Reboot

    /*Switch to real mode (We don't return) */
    jmp SwitchToReal


PUBLIC _ChainLoadBiosBootSectorCode
_ChainLoadBiosBootSectorCode:
    /* Set the boot drive */
    mov dl, byte ptr [_FrldrBootDrive]

    /* Set the function ID */
    mov bx, FNID_ChainLoadBiosBootSectorCode

    /*Switch to real mode (We don't return) */
    jmp SwitchToReal


PUBLIC i386CallRealMode
i386CallRealMode:
    /* Set continue address and switch to real mode */
    mov dword ptr [ContinueAddress], offset i386CallRealMode_return
    jmp SwitchToReal
i386CallRealMode_return:
    ret


/* Entrypoint for realmode function calls
 * ContinueAddress must be set to the return point from realmode
 * bx must be set to the ID of the realmode function to call. */
SwitchToReal:
    /* Set sane segments */
    mov ax, PMODE_DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    /* Save 32-bit stack pointer */
    mov dword ptr [stack32], esp

    /* jmp to 16-bit segment to set the limit correctly */
    .byte HEX(0ea) // jmp far RMODE_CS:switch_to_real16
SwitchToReal16Address:
    .long 0 // receives address of switch_to_real16
    .word RMODE_CS
    nop


    /* 16-bit stack pointer */
stack16:
	.word STACK16ADDR

    /* 32-bit stack pointer */
stack32:
    .long STACKADDR

    .align 4	/* force 4-byte alignment */
gdt:
    /* NULL Descriptor */
   .word HEX(0000)
   .word HEX(0000)
   .word HEX(0000)
   .word HEX(0000)

    /* 32-bit flat CS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9A00)
    .word HEX(00CF)

    /* 32-bit flat DS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9200)
    .word HEX(00CF)

    /* 16-bit real mode CS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9E00)
    .word HEX(0000)

    /* 16-bit real mode DS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9200)
    .word HEX(0000)

/* GDT table pointer */
gdtptr:
    .word HEX(27)		/* Limit */
    .long gdt			/* Base Address */

/* Real-mode IDT pointer */
rmode_idtptr:
    .word HEX(3ff)		/* Limit */
    .long 0			/* Base Address */

PUBLIC i386idtptr
i386idtptr:
    .word 255			/* Limit */
    .long _i386Idt		/* Base Address */

PUBLIC _FrldrBootDrive
_FrldrBootDrive:
    .byte 0

PUBLIC _FrldrBootPartition
_FrldrBootPartition:
    .long 0

END
