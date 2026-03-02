/*
 * PROJECT:           ReactOS C Runtime Library
 * LICENSE:           MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:           memmove implementation for amd64
 * COPYRIGHT:         Copyright 2026 Ahmed Arif <arif.ing@outlook.com>
 */

.intel_syntax noprefix
.text
.p2align 4, 0x90

.globl memmove
.def memmove; .scl 2; .type 32; .endef
    .seh_proc memmove
memmove:
    .seh_endprologue
    .cfi_startproc
    # RCX = dst, RDX = src, R8 = count
    mov     rax, rcx                 # return value = dst

    test    r8, r8
    jz      .Ldone

    cmp     rcx, rdx
    je      .Ldone                   # dst == src

    # If dst <= src, or dst >= src+count, forward is safe
    jb      .Lcopy_forward           # dst < src
    mov     r9, rdx
    add     r9, r8                   # r9 = src + count (may overflow)
    jc      .Lcopy_backward          # overflow => pick backward to be safe
    cmp     rcx, r9
    jae     .Lcopy_forward           # dst >= src+count => no overlap => forward

    # -------------------------
    # Backward copy (overlap)
    # -------------------------
.Lcopy_backward:
    # Set pointers one past the end
    add     rdx, r8
    add     rcx, r8
    mov     r9,  r8

    # 8-byte chunks
    mov     r10, r9
    shr     r10, 3                   # r10 = r9 / 8
    jz      .Lbwd_tail

.p2align 4, 0x90
.Lbwd_qword:
    sub     rdx, 8
    sub     rcx, 8
    mov     r11, [rdx]
    mov     [rcx], r11
    dec     r10
    jnz     .Lbwd_qword

    and     r9, 7                    # remaining < 8

.Lbwd_tail:
    # 4-byte
    test    r9, 4
    jz      .Lbwd_2
    sub     rdx, 4
    sub     rcx, 4
    mov     r11d, [rdx]
    mov     [rcx], r11d
    sub     r9, 4

.Lbwd_2:
    # 2-byte
    test    r9, 2
    jz      .Lbwd_1
    sub     rdx, 2
    sub     rcx, 2
    mov     r11w, [rdx]
    mov     [rcx], r11w
    sub     r9, 2

.Lbwd_1:
    # 1-byte
    test    r9, 1
    jz      .Ldone
    dec     rdx
    dec     rcx
    mov     r11b, [rdx]
    mov     [rcx], r11b
    jmp     .Ldone

    # -------------------------
    # Forward copy (no overlap)
    # -------------------------
.Lcopy_forward:
    mov     r9, r8

    # 8-byte chunks
    mov     r10, r9
    shr     r10, 3                   # r10 = r9 / 8
    jz      .Lfwd_tail

.p2align 4, 0x90
.Lfwd_qword:
    mov     r11, [rdx]
    mov     [rcx], r11
    add     rdx, 8
    add     rcx, 8
    dec     r10
    jnz     .Lfwd_qword

    and     r9, 7                    # remaining < 8

.Lfwd_tail:
    # 4-byte
    test    r9, 4
    jz      .Lfwd_2
    mov     r11d, [rdx]
    mov     [rcx], r11d
    add     rdx, 4
    add     rcx, 4
    sub     r9, 4

.Lfwd_2:
    # 2-byte
    test    r9, 2
    jz      .Lfwd_1
    mov     r11w, [rdx]
    mov     [rcx], r11w
    add     rdx, 2
    add     rcx, 2
    sub     r9, 2

.Lfwd_1:
    # 1-byte
    test    r9, 1
    jz      .Ldone
    mov     r11b, [rdx]
    mov     [rcx], r11b

.Ldone:
    ret
    .seh_endproc
    .cfi_endproc
