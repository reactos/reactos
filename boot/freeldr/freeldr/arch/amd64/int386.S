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
.code64

EXTERN CallRealMode:PROC

/* int Int386(int ivec<ecx>, REGS* in<rdx>, REGS* out<r8>); */
PUBLIC Int386
Int386:

    /* Save home registers */
    mov r11, rsp
    mov qword ptr [r11 + 8], rcx
    mov qword ptr [r11 + 16], rdx
    mov qword ptr [r11 + 24], r8

    /* Save non-volatile registers */
    push rbx
    push rbp
    push rsi
    push rdi

    /* Alloc stack space for home registers */
    sub rsp, 40
    //.ENDPROLOG

int386_2:
    /* Copy the int vector to shared memory */
    mov dword ptr [BSS_IntVector], ecx

    /* Copy input registers */
    mov rsi, rdx
    mov rdi, BSS_RegisterSet
    mov rcx, REGS_SIZE / 4
    rep movsd

    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rbp, rbp
    xor rsi, rsi
    xor rdi, rdi

    /* Set the function ID and call real mode */
    mov bx, FNID_Int386
    call CallRealMode

    /* Copy output registers */
    mov rsi, BSS_RegisterSet
    mov rdi, [r11 + 24]
    mov rcx, REGS_SIZE / 4
    rep movsd

    /* cleanup and return */
    add rsp, 40
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret


END
