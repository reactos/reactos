/*
 * PROJECT:           ReactOS C Runtime Library
 * LICENSE:           MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:           memcpy implementation for amd64
 * COPYRIGHT:         Copyright 2026 Ahmed Arif <arif.ing@outlook.com>
 */

.intel_syntax noprefix

# Dispatch pointer, initialized to the one-time init stub
.section .data
.p2align 3
__memcpy_ptr:
    .quad __memcpy_init

.text

# Public entry: single indirect jump
.p2align 4, 0x90
.globl memcpy
.def memcpy; .scl 2; .type 32; .endef
memcpy:
    jmp     qword ptr [rip+__memcpy_ptr]

# One-time init: detect ERMS, update pointer, then dispatch
.p2align 4, 0x90
    .seh_proc __memcpy_init
__memcpy_init:
    push    rbx
    .seh_pushreg rbx
    .seh_endprologue

    # Save parameters (CPUID clobbers EAX, EBX, ECX, EDX)
    mov     r9,  rcx
    mov     r10, rdx
    # R8 is not clobbered by CPUID

    mov     eax, 7
    xor     ecx, ecx
    cpuid

    # Default to SSE2, upgrade to ERMS if available (EBX bit 9)
    lea     rax, [rip+__memcpy_sse2]
    bt      ebx, 9
    jnc     .Linit_done
    lea     rax, [rip+__memcpy_erms]
.Linit_done:
    mov     qword ptr [rip+__memcpy_ptr], rax

    # Restore parameters
    mov     rcx, r9
    mov     rdx, r10
    pop     rbx

    # Dispatch to chosen implementation
    jmp     qword ptr [rip+__memcpy_ptr]
    .seh_endproc

# =========================================================================
# SSE2 + non-temporal implementation
# =========================================================================
.p2align 4, 0x90
    .seh_proc __memcpy_sse2
__memcpy_sse2:
    .seh_endprologue
    .cfi_startproc
    # RCX=dst, RDX=src, R8=count
    mov     rax, rcx                # return value = dst

    cmp     r8, 32
    jb      .Lsmall_copy

    # Align dst to 16
    mov     r9, rcx
    and     r9, 15
    jz      .Lafter_align

    mov     r10, 16
    sub     r10, r9                 # bytes to align [1..15]
    sub     r8,  r10                # reduce remaining count

.Lalign_loop:
    mov     r11b, [rdx]
    mov     [rcx], r11b
    inc     rdx
    inc     rcx
    dec     r10
    jnz     .Lalign_loop

.Lafter_align:
    # Decide regular vs NT path
    cmp     r8, 262144              # 256 KiB
    jae     .Lnt_large

    # -------- Regular SSE2 path (aligned dst) --------
    # Choose aligned-loads when src is 16B aligned
    mov     r10, rdx
    and     r10, 15
    jz      .Lreg_aligned_loads

    # --- Unaligned-load main loop: 128B per iteration ---
    mov     r9, r8
    shr     r9, 7                   # r9 = number of 128B blocks
    jz      .Lreg_64tail
.p2align 4, 0x90
.Lreg128_unaligned:
    prefetchnta [rdx+256]
    prefetchnta [rdx+320]

    movdqu  xmm0, [rdx]
    movdqu  xmm1, [rdx+16]
    movdqu  xmm2, [rdx+32]
    movdqu  xmm3, [rdx+48]
    movdqa  [rcx],     xmm0
    movdqa  [rcx+16],  xmm1
    movdqa  [rcx+32],  xmm2
    movdqa  [rcx+48],  xmm3

    movdqu  xmm0, [rdx+64]
    movdqu  xmm1, [rdx+80]
    movdqu  xmm2, [rdx+96]
    movdqu  xmm3, [rdx+112]
    movdqa  [rcx+64],  xmm0
    movdqa  [rcx+80],  xmm1
    movdqa  [rcx+96],  xmm2
    movdqa  [rcx+112], xmm3

    add     rdx, 128
    add     rcx, 128
    dec     r9
    jnz     .Lreg128_unaligned

.Lreg_64tail:
    mov     r9, r8
    and     r9, 127                 # <128B remainder
    shr     r9, 6                   # # of 64B blocks
    jz      .Lreg_16tail

.p2align 4, 0x90
.Lreg64_unaligned:
    movdqu  xmm0, [rdx]
    movdqu  xmm1, [rdx+16]
    movdqu  xmm2, [rdx+32]
    movdqu  xmm3, [rdx+48]
    movdqa  [rcx],     xmm0
    movdqa  [rcx+16],  xmm1
    movdqa  [rcx+32],  xmm2
    movdqa  [rcx+48],  xmm3
    add     rdx, 64
    add     rcx, 64
    dec     r9
    jnz     .Lreg64_unaligned
    jmp     .Lreg_16tail

    # --- Aligned-load fast path ---
.p2align 4, 0x90
.Lreg_aligned_loads:
    mov     r9, r8
    shr     r9, 7                   # 128B blocks
    jz      .Lreg_64tail_al

.p2align 4, 0x90
.Lreg128_aligned:
    prefetchnta [rdx+256]
    prefetchnta [rdx+320]

    movdqa  xmm0, [rdx]
    movdqa  xmm1, [rdx+16]
    movdqa  xmm2, [rdx+32]
    movdqa  xmm3, [rdx+48]
    movdqa  [rcx],     xmm0
    movdqa  [rcx+16],  xmm1
    movdqa  [rcx+32],  xmm2
    movdqa  [rcx+48],  xmm3

    movdqa  xmm0, [rdx+64]
    movdqa  xmm1, [rdx+80]
    movdqa  xmm2, [rdx+96]
    movdqa  xmm3, [rdx+112]
    movdqa  [rcx+64],  xmm0
    movdqa  [rcx+80],  xmm1
    movdqa  [rcx+96],  xmm2
    movdqa  [rcx+112], xmm3

    add     rdx, 128
    add     rcx, 128
    dec     r9
    jnz     .Lreg128_aligned

.Lreg_64tail_al:
    mov     r9, r8
    and     r9, 127
    shr     r9, 6
    jz      .Lreg_16tail

.p2align 4, 0x90
.Lreg64_aligned:
    movdqa  xmm0, [rdx]
    movdqa  xmm1, [rdx+16]
    movdqa  xmm2, [rdx+32]
    movdqa  xmm3, [rdx+48]
    movdqa  [rcx],     xmm0
    movdqa  [rcx+16],  xmm1
    movdqa  [rcx+32],  xmm2
    movdqa  [rcx+48],  xmm3
    add     rdx, 64
    add     rcx, 64
    dec     r9
    jnz     .Lreg64_aligned

    # Common tails
.Lreg_16tail:
    mov     r9, r8
    and     r9, 63                  # <64
    shr     r9, 4                   # 16B chunks
    jz      .Lreg_bytetail
.p2align 4, 0x90
.Lreg_16body:
    movdqu  xmm0, [rdx]
    movdqa  [rcx], xmm0
    add     rdx, 16
    add     rcx, 16
    dec     r9
    jnz     .Lreg_16body

.Lreg_bytetail:
    and     r8, 15
    jz      .Ldone
.p2align 4, 0x90
.Lreg_byteloop:
    mov     r9b, [rdx]
    mov     [rcx], r9b
    inc     rdx
    inc     rcx
    dec     r8
    jnz     .Lreg_byteloop
    jmp     .Ldone

    # -------- Non-temporal large copy (>= 256 KiB) --------
.p2align 4, 0x90
.Lnt_large:
    # 64B-align destination for NT stores
    mov     r10, rcx
    and     r10, 63                 # misalignment [0..63]
    jz      .Lnt_after_align64
    mov     r11, 64
    sub     r11, r10                # bytes to reach 64B alignment
    cmp     r11, r8
    cmova   r11, r8
    mov     r10, r11                # total preface

    # 16B chunks to reach 64B boundary
    mov     r9, r11
    shr     r9, 4
    jz      .Lnt_align64_tail
.p2align 4, 0x90
.Lnt_align64_16:
    movdqu  xmm0, [rdx]
    movdqa  [rcx], xmm0
    add     rdx, 16
    add     rcx, 16
    dec     r9
    jnz     .Lnt_align64_16

.Lnt_align64_tail:
    and     r11, 15
    jz      .Lnt_align64_done
.p2align 4, 0x90
.Lnt_align64_tail_loop:
    mov     r9b, [rdx]
    mov     [rcx], r9b
    inc     rdx
    inc     rcx
    dec     r11
    jnz     .Lnt_align64_tail_loop

.Lnt_align64_done:
    sub     r8, r10

.Lnt_after_align64:
    # 128B-unrolled NT main loop
    mov     r9, r8
    shr     r9, 7                   # 128B blocks
    jz      .Lnt_64tail

.p2align 4, 0x90
.Lnt128:
    prefetchnta [rdx+256]
    prefetchnta [rdx+320]

    movdqu  xmm0, [rdx]
    movdqu  xmm1, [rdx+16]
    movdqu  xmm2, [rdx+32]
    movdqu  xmm3, [rdx+48]
    movntdq [rcx],     xmm0
    movntdq [rcx+16],  xmm1
    movntdq [rcx+32],  xmm2
    movntdq [rcx+48],  xmm3

    movdqu  xmm0, [rdx+64]
    movdqu  xmm1, [rdx+80]
    movdqu  xmm2, [rdx+96]
    movdqu  xmm3, [rdx+112]
    movntdq [rcx+64],  xmm0
    movntdq [rcx+80],  xmm1
    movntdq [rcx+96],  xmm2
    movntdq [rcx+112], xmm3

    add     rdx, 128
    add     rcx, 128
    dec     r9
    jnz     .Lnt128

.Lnt_64tail:
    mov     r9, r8
    and     r9, 127
    shr     r9, 6                   # 64B blocks
    jz      .Lnt_16tail
.p2align 4, 0x90
.Lnt64:
    movdqu  xmm0, [rdx]
    movdqu  xmm1, [rdx+16]
    movdqu  xmm2, [rdx+32]
    movdqu  xmm3, [rdx+48]
    movntdq [rcx],     xmm0
    movntdq [rcx+16],  xmm1
    movntdq [rcx+32],  xmm2
    movntdq [rcx+48],  xmm3
    add     rdx, 64
    add     rcx, 64
    dec     r9
    jnz     .Lnt64

.Lnt_16tail:
    mov     r9, r8
    and     r9, 63
    shr     r9, 4                   # 16B chunks
    jz      .Lnt_bytetail
.p2align 4, 0x90
.Lnt_16body:
    movdqu  xmm0, [rdx]
    movntdq [rcx], xmm0
    add     rdx, 16
    add     rcx, 16
    dec     r9
    jnz     .Lnt_16body

.Lnt_bytetail:
    and     r8, 15
    jz      .Lnt_finish
.p2align 4, 0x90
.Lnt_byteloop:
    mov     r9b, [rdx]
    mov     [rcx], r9b
    inc     rdx
    inc     rcx
    dec     r8
    jnz     .Lnt_byteloop

.Lnt_finish:
    sfence                          # ensure NT stores reach memory
    jmp     .Ldone

    # -------- Tiny copies (< 32 bytes) --------
.p2align 4, 0x90
.Lsmall_copy:
    mov     r9, r8
    cmp     r9, 16
    jb      .Lsmall_8

    mov     r10, [rdx]
    mov     r11, [rdx+8]
    mov     [rcx],   r10
    mov     [rcx+8], r11
    add     rcx, 16
    add     rdx, 16
    sub     r9, 16

.Lsmall_8:
    cmp     r9, 8
    jb      .Lsmall_4
    mov     r10, [rdx]
    mov     [rcx], r10
    add     rcx, 8
    add     rdx, 8
    sub     r9, 8

.Lsmall_4:
    cmp     r9, 4
    jb      .Lsmall_2
    mov     r10d, [rdx]
    mov     [rcx], r10d
    add     rcx, 4
    add     rdx, 4
    sub     r9, 4

.Lsmall_2:
    cmp     r9, 2
    jb      .Lsmall_1
    mov     r10w, [rdx]
    mov     [rcx], r10w
    add     rcx, 2
    add     rdx, 2
    sub     r9, 2

.Lsmall_1:
    test    r9, r9
    jz      .Ldone
    mov     r10b, [rdx]
    mov     [rcx], r10b

.Ldone:
    ret
    .seh_endproc
    .cfi_endproc

# =========================================================================
# ERMS implementation: rep movsb
# =========================================================================
.p2align 4, 0x90
    .seh_proc __memcpy_erms
__memcpy_erms:
    push    rdi
    .seh_pushreg rdi
    push    rsi
    .seh_pushreg rsi
    .seh_endprologue
    .cfi_startproc

    mov     rax, rcx                # return value = dst
    mov     rdi, rcx                # RDI = dst
    mov     rsi, rdx                # RSI = src
    mov     rcx, r8                 # RCX = count
    test    rcx, rcx
    jz      .Lerms_done
    cld
    rep movsb

.Lerms_done:
    pop     rsi
    pop     rdi
    ret
    .seh_endproc
    .cfi_endproc
