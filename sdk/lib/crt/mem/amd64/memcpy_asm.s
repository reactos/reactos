/* sdk/lib/crt/mem/amd64/memcpy_asm.s  (GAS, Intel syntax, Win64 ABI)
 *
 * void *memcpy(void *dest, const void *src, size_t count)
 * RCX=dest, RDX=src, R8=count
 * Returns dest in RAX
 * Note: memcpy has undefined behavior on overlap (use memmove).
 */

        .text
        .intel_syntax noprefix
        .globl  memcpy
        .def    memcpy; .scl 2; .type 32; .endef
        .p2align 4, 0x90

memcpy:
        mov     rax, rcx            # return value = dest
        mov     r10, rcx            # dst
        mov     r11, rdx            # src
        mov     rdx, r8             # count

        test    rdx, rdx
        jz      .Ldone

        # Large fast path: rep movsq/movsb (forward)
        cmp     rdx, 256
        jb      .Lsmallmed
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

.Lsmallmed:
        # Medium/small: align dst stores, unrolled qwords, then tail
        cmp     rdx, 32
        jb      .Lsmall

        # Align destination to 8 (unaligned loads are fine)
        mov     rcx, r10
        and     rcx, 7
        jz      .Laligned

        mov     r8, 8
        sub     r8, rcx
        cmp     r8, rdx
        cmova   r8, rdx
.Lalign_loop:
        mov     r9b, byte ptr [r11]
        mov     byte ptr [r10], r9b
        inc     r11
        inc     r10
        dec     r8
        dec     rdx
        jnz     .Lalign_loop

.Laligned:
        # 64B blocks
        mov     rcx, rdx
        shr     rcx, 6
        jz      .Lafter64

.Lblk64:
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
        jnz     .Lblk64

.Lafter64:
        and     rdx, 63
        cmp     rdx, 32
        jb      .Lqwords

        # 32B
        mov     r9,  qword ptr [r11 +  0]; mov qword ptr [r10 +  0], r9
        mov     r9,  qword ptr [r11 +  8]; mov qword ptr [r10 +  8], r9
        mov     r9,  qword ptr [r11 + 16]; mov qword ptr [r10 + 16], r9
        mov     r9,  qword ptr [r11 + 24]; mov qword ptr [r10 + 24], r9
        add     r11, 32
        add     r10, 32
        sub     rdx, 32

.Lqwords:
        mov     rcx, rdx
        shr     rcx, 3
        jz      .Ltail

.Lqw_loop:
        mov     r9, qword ptr [r11]
        mov     qword ptr [r10], r9
        add     r11, 8
        add     r10, 8
        dec     rcx
        jnz     .Lqw_loop

.Ltail:
        and     rdx, 7
        jz      .Ldone

.Lsmall:
        mov     rcx, rdx
.Lbyte_loop:
        mov     r9b, byte ptr [r11]
        mov     byte ptr [r10], r9b
        inc     r11
        inc     r10
        dec     rcx
        jnz     .Lbyte_loop

.Ldone:
        ret
