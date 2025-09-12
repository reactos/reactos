/* sdk/lib/crt/mem/amd64/memmove_asm.s  (GAS, Intel syntax, Win64 ABI)
 *
 * void *memmove(void *dest, const void *src, size_t count)
 * RCX=dest, RDX=src, R8=count
 * Returns dest in RAX
 */

        .text
        .intel_syntax noprefix
        .globl  memmove
        .def    memmove; .scl 2; .type 32; .endef
        .p2align 4, 0x90

memmove:
        mov     rax, rcx            # return value = dest
        mov     r10, rcx            # dst
        mov     r11, rdx            # src
        mov     rdx, r8             # count

        test    rdx, rdx
        jz      .Ldone

        # Overlap check: forward if (dst <= src) || (dst >= src+count)
        cmp     r10, r11
        jbe     .Lfwd
        mov     rcx, r11
        add     rcx, rdx
        cmp     r10, rcx
        jae     .Lfwd

        # ---------- Backward copy (overlap) ----------
        # Large backward: std + rep movsb (simple, robust)
        cmp     rdx, 256
        jb      .Lb_smallmed
        push    rdi
        push    rsi
        std
        lea     rdi, [r10 + rdx - 1]
        lea     rsi, [r11 + rdx - 1]
        mov     rcx, rdx
        rep movsb
        cld
        pop     rsi
        pop     rdi
        ret

.Lb_smallmed:
        # Manual backward qwords, then bytes
        add     r10, rdx
        add     r11, rdx

        mov     rcx, rdx
        shr     rcx, 3              # qword count
        jz      .Lb_tail

.Lb_qw:
        sub     r11, 8
        sub     r10, 8
        mov     r8,  qword ptr [r11]
        mov     qword ptr [r10], r8
        dec     rcx
        jnz     .Lb_qw

.Lb_tail:
        and     rdx, 7
        jz      .Ldone

.Lb_byte:
        dec     r11
        dec     r10
        mov     r8b, byte ptr [r11]
        mov     byte ptr [r10], r8b
        dec     rdx
        jnz     .Lb_byte
        ret

        # ---------- Forward copy (no overlap or safe) ----------
.Lfwd:
        cmp     rdx, 256
        jb      .Lf_smallmed

        # Large forward: rep movsq/movsb
        push    rdi
        push    rsi
        cld
        mov     rdi, r10
        mov     rsi, r11
        mov     rcx, rdx
        shr     rcx, 3
        rep movsq
        mov     rcx, rdx
        and     rcx, 7
        rep movsb
        pop     rsi
        pop     rdi
        ret

.Lf_smallmed:
        # Align destination to 8 (loads may be unaligned)
        cmp     rdx, 32
        jb      .Lf_small

        mov     rcx, r10
        and     rcx, 7
        jz      .Lf_aligned

        mov     r8, 8
        sub     r8, rcx
        cmp     r8, rdx
        cmova   r8, rdx
.Lf_align_loop:
        mov     r9b, byte ptr [r11]
        mov     byte ptr [r10], r9b
        inc     r11
        inc     r10
        dec     r8
        dec     rdx
        jnz     .Lf_align_loop

.Lf_aligned:
        # 64B blocks
        mov     rcx, rdx
        shr     rcx, 6
        jz      .Lf_after64

.Lf_64:
        mov     r9,  qword ptr [r11 +  0]; mov qword ptr [r10 +  0], r9
        mov     r9,  qword ptr [r11 +  8]; mov qword ptr [r10 +  8], r9
        mov     r9,  qword ptr [r11 + 16]; mov qword ptr [r10 + 16], r9
        mov     r9,  qword ptr [r11 + 24]; mov qword ptr [r10 + 24], r9
        mov     r9,  qword ptr [r11 + 32]; mov qword ptr [r10 + 32], r9
        mov     r9,  qword ptr [r11 + 40]; mov qword ptr [r10 + 40], r9
        mov     r9,  qword ptr [r11 + 48]; mov qword ptr [r10 + 48], r9
        mov     r9,  qword ptr [r11 + 56]; mov qword ptr [r10 + 56], r9
        add     r11, 64
        add     r10, 64
        dec     rcx
        jnz     .Lf_64

.Lf_after64:
        and     rdx, 63
        cmp     rdx, 32
        jb      .Lf_qwords

        # 32B
        mov     r9,  qword ptr [r11 +  0]; mov qword ptr [r10 +  0], r9
        mov     r9,  qword ptr [r11 +  8]; mov qword ptr [r10 +  8], r9
        mov     r9,  qword ptr [r11 + 16]; mov qword ptr [r10 + 16], r9
        mov     r9,  qword ptr [r11 + 24]; mov qword ptr [r10 + 24], r9
        add     r11, 32
        add     r10, 32
        sub     rdx, 32

.Lf_qwords:
        mov     rcx, rdx
        shr     rcx, 3
        jz      .Lf_tail

.Lf_qw:
        mov     r9, qword ptr [r11]
        mov     qword ptr [r10], r9
        add     r11, 8
        add     r10, 8
        dec     rcx
        jnz     .Lf_qw

.Lf_tail:
        and     rdx, 7
        jz      .Ldone

.Lf_small:
        mov     rcx, rdx
.Lf_byte:
        mov     r9b, byte ptr [r11]
        mov     byte ptr [r10], r9b
        inc     r11
        inc     r10
        dec     rcx
        jnz     .Lf_byte

.Ldone:
        ret
