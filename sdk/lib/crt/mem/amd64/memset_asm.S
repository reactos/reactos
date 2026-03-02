/*
 * PROJECT:           ReactOS C Runtime Library
 * LICENSE:           MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:           memset implementation for amd64
 * COPYRIGHT:         Copyright 2026 Ahmed Arif <arif.ing@outlook.com>
 */

.intel_syntax noprefix
.text
.p2align 4, 0x90

.globl memset
.def memset; .scl 2; .type 32; .endef
    .seh_proc memset
memset:
    .seh_endprologue
    .cfi_startproc
    # RCX = dst, EDX = byte value, R8 = count
    mov     rax, rcx                # return value = dst

    test    r8, r8
    jz      .Ldone

    # Build 64-bit fill pattern in R11 from DL
    movzx   r11d, dl                # r11 = 0..255
    mov     r10, 0x0101010101010101
    imul    r11, r10                # r11 = byte replicated across 8 bytes

    # If count < 16, go to small tail
    cmp     r8, 16
    jb      .Lsmall

    # -------------------------
    # Align destination to 8B
    # -------------------------
    mov     r9, rcx
    and     r9, 7                   # misalignment [0..7]
    jz      .Lqwords

    mov     r10, 8
    sub     r10, r9                 # bytes to align [1..7]
    cmp     r10, r8
    cmova   r10, r8                 # cap by remaining count
    sub     r8,  r10                # reduce remaining

.p2align 4, 0x90
.Lalign_bytes:
    mov     byte ptr [rcx], r11b
    inc     rcx
    dec     r10
    jnz     .Lalign_bytes

    test    r8, r8
    jz      .Ldone

    # -------------------------
    # 8-byte bulk stores
    # -------------------------
.Lqwords:
    mov     r9, r8
    shr     r9, 3                   # r9 = # of qwords
    jz      .Ltail

.p2align 4, 0x90
.Lqw_loop:
    mov     qword ptr [rcx], r11
    add     rcx, 8
    dec     r9
    jnz     .Lqw_loop

    and     r8, 7                   # remaining < 8

    # -------------------------
    # 4/2/1 tail
    # -------------------------
.Ltail:
    test    r8, 4
    jz      .Ltail_2
    mov     dword ptr [rcx], r11d
    add     rcx, 4
    sub     r8, 4

.Ltail_2:
    test    r8, 2
    jz      .Ltail_1
    mov     word ptr [rcx], r11w
    add     rcx, 2
    sub     r8, 2

.Ltail_1:
    test    r8, 1
    jz      .Ldone
    mov     byte ptr [rcx], r11b
    jmp     .Ldone

    # -------------------------
    # Small sizes (< 16)
    # -------------------------
.Lsmall:
    # Try 8,4,2,1 in sequence
    test    r8, 8
    jz      .Lsm_4
    mov     qword ptr [rcx], r11
    add     rcx, 8
    sub     r8, 8

.Lsm_4:
    test    r8, 4
    jz      .Lsm_2
    mov     dword ptr [rcx], r11d
    add     rcx, 4
    sub     r8, 4

.Lsm_2:
    test    r8, 2
    jz      .Lsm_1
    mov     word ptr [rcx], r11w
    add     rcx, 2
    sub     r8, 2

.Lsm_1:
    test    r8, 1
    jz      .Ldone
    mov     byte ptr [rcx], r11b

.Ldone:
    ret
    .seh_endproc
    .cfi_endproc
