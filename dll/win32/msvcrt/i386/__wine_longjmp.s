/*
 * PROJECT:     ReactOS msvcrt.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     x86 asm implementation of ___wine_longjmp
 * COPYRIGHT:   Copyright 1999, 2010 Alexandre Julliard
 */

#include <asm.inc>

// See wine: dlls/winecrt0/setjmp.c

.code

PUBLIC ___wine_longjmp
___wine_longjmp:
    mov ecx, [esp + 4]       /* jmp_buf */
    mov eax, [esp + 8]       /* retval */
    mov ebp, [ecx + 0]       /* jmp_buf.Ebp */
    mov ebx, [ecx + 4]       /* jmp_buf.Ebx */
    mov edi, [ecx + 8]       /* jmp_buf.Edi */
    mov esi, [ecx + 12]      /* jmp_buf.Esi */
    mov esp, [ecx + 16]      /* jmp_buf.Esp */
    add esp, 4               /* get rid of return address */
    jmp dword ptr [ecx + 20] /* jmp_buf.Eip */

END
