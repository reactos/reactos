/* Copyright (c) Mark Harmstone 2020
 *
 * This file is part of WinBtrfs.
 *
 * WinBtrfs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public Licence as published by
 * the Free Software Foundation, either version 3 of the Licence, or
 * (at your option) any later version.
 *
 * WinBtrfs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public Licence for more details.
 *
 * You should have received a copy of the GNU Lesser General Public Licence
 * along with WinBtrfs.  If not, see <http://www.gnu.org/licenses/>. */

#include <asm.inc>

#ifdef __x86_64__

EXTERN crctable:QWORD

.code64

/* uint32_t __stdcall calc_crc32c_sw(uint32_t seed, uint8_t* msg, uint32_t msglen); */

PUBLIC calc_crc32c_sw
calc_crc32c_sw:

/* rax = crc / seed
 * rdx = buf
 * r8 = len
 * rcx = tmp
 * r10 = tmp2 */

mov rax, rcx

crcloop:
test r8, r8
jz crcend

mov rcx, rax
shr rcx, 8
mov r10b, byte ptr [rdx]
xor al, r10b
and rax, 255
shl rax, 2
lea r10, [rip+crctable]
mov eax, dword ptr [r10 + rax]
xor rax, rcx

inc rdx
dec r8

jmp crcloop

crcend:
ret

/****************************************************/

/* uint32_t __stdcall calc_crc32c_hw(uint32_t seed, uint8_t* msg, uint32_t msglen); */

PUBLIC calc_crc32c_hw

calc_crc32c_hw:

/* rax = crc / seed
 * rdx = buf
 * r8 = len */

mov rax, rcx

crchw_loop:
cmp r8, 8
jl crchw_stragglers

crc32 rax, qword ptr [rdx]

add rdx, 8
sub r8, 8
jmp crchw_loop

crchw_stragglers:
cmp r8, 4
jl crchw_stragglers2

crc32 eax, dword ptr [rdx]

add rdx, 4
sub r8, 4

crchw_stragglers2:
cmp r8, 2
jl crchw_stragglers3

crc32 eax, word ptr [rdx]

add rdx, 2
sub r8, 2

crchw_stragglers3:
test r8, r8
jz crchw_end

crc32 eax, byte ptr [rdx]
inc rdx
dec r8
jmp crchw_stragglers3

crchw_end:
ret

END
#elif defined(_X86_)

EXTERN _crctable:DWORD
.code

/* uint32_t __stdcall calc_crc32c_sw(uint32_t seed, uint8_t* msg, uint32_t msglen); */

PUBLIC _calc_crc32c_sw@12
_calc_crc32c_sw@12:

push ebp
mov ebp, esp

push esi
push ebx

mov eax, [ebp+8]
mov edx, [ebp+12]
mov ebx, [ebp+16]

/* eax = crc / seed
 * ebx = len
 * esi = tmp
 * edx = buf
 * ecx = tmp2 */

crcloop:
test ebx, ebx
jz crcend

mov esi, eax
shr esi, 8
mov cl, byte ptr [edx]
xor al, cl
and eax, 255
shl eax, 2
mov eax, [_crctable + eax]
xor eax, esi

inc edx
dec ebx

jmp crcloop

crcend:
pop ebx
pop esi

pop ebp

ret 12

/****************************************************/

/* uint32_t __stdcall calc_crc32c_hw(uint32_t seed, uint8_t* msg, uint32_t msglen); */

PUBLIC _calc_crc32c_hw@12
_calc_crc32c_hw@12:

push ebp
mov ebp, esp

mov eax, [ebp+8]
mov edx, [ebp+12]
mov ecx, [ebp+16]

/* eax = crc / seed
 * ecx = len
 * edx = buf */

crchw_loop:
cmp ecx, 4
jl crchw_stragglers

crc32 eax, dword ptr [edx]

add edx, 4
sub ecx, 4
jmp crchw_loop

crchw_stragglers:
cmp ecx, 2
jl crchw_stragglers2

crc32 eax, word ptr [edx]

add edx, 2
sub ecx, 2

crchw_stragglers2:
test ecx, ecx
jz crchw_end

crc32 eax, byte ptr [edx]
inc edx
dec ecx
jmp crchw_stragglers2

crchw_end:
pop ebp

ret 12

END
#endif
