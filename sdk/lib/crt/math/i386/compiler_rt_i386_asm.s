/*
 * Copyright (c) 2026 Terascale Functionalists
 * SPDX-License-Identifier: MIT
 *
 * i386 Clang long-long helper wrappers for the ReactOS CRT math library.
 */

#include <asm.inc>

EXTERN __alldiv:PROC
EXTERN __aulldiv:PROC
EXTERN __allrem:PROC
EXTERN __aullrem:PROC
EXTERN __alldvrm:PROC
EXTERN __aulldvrm:PROC

PUBLIC ___divdi3
PUBLIC ___udivdi3
PUBLIC ___moddi3
PUBLIC ___umoddi3
PUBLIC ___divmoddi4
PUBLIC ___udivmoddi4

.code

___divdi3:
        push    dword ptr [esp + 16]
        push    dword ptr [esp + 16]
        push    dword ptr [esp + 16]
        push    dword ptr [esp + 16]
        call    __alldiv
        ret

___udivdi3:
        push    dword ptr [esp + 16]
        push    dword ptr [esp + 16]
        push    dword ptr [esp + 16]
        push    dword ptr [esp + 16]
        call    __aulldiv
        ret

___moddi3:
        push    dword ptr [esp + 16]
        push    dword ptr [esp + 16]
        push    dword ptr [esp + 16]
        push    dword ptr [esp + 16]
        call    __allrem
        ret

___umoddi3:
        push    dword ptr [esp + 16]
        push    dword ptr [esp + 16]
        push    dword ptr [esp + 16]
        push    dword ptr [esp + 16]
        call    __aullrem
        ret

___divmoddi4:
        push    ebx
        push    esi
        mov     esi, [esp + 28]
        push    dword ptr [esp + 24]
        push    dword ptr [esp + 24]
        push    dword ptr [esp + 24]
        push    dword ptr [esp + 24]
        call    __alldvrm
        test    esi, esi
        jz      short .Ldivmoddi4_done
        mov     [esi], ecx
        mov     [esi + 4], ebx
.Ldivmoddi4_done:
        pop     esi
        pop     ebx
        ret

___udivmoddi4:
        push    ebx
        push    esi
        mov     esi, [esp + 28]
        push    dword ptr [esp + 24]
        push    dword ptr [esp + 24]
        push    dword ptr [esp + 24]
        push    dword ptr [esp + 24]
        call    __aulldvrm
        test    esi, esi
        jz      short .Ludivmoddi4_done
        mov     [esi], ecx
        mov     [esi + 4], ebx
.Ludivmoddi4_done:
        pop     esi
        pop     ebx
        ret

END
