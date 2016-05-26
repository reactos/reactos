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

.code32

EXTERN SwitchToReal:PROC
EXTERN ContinueAddress:DWORD

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

END
