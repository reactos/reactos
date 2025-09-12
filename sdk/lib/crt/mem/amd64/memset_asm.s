/* sdk/lib/crt/mem/amd64/memset_asm.s  (GAS, Intel syntax, Win64 ABI)
 *
 * void *memset(void *dest, int val, size_t count)
 * RCX=dest, RDX=val, R8=count
 * Returns dest in RAX
 */

        .text
        .intel_syntax noprefix
        .globl  memset
        .def    memset; .scl 2; .type 32; .endef
        .p2align 4, 0x90

memset:
        mov     rax, rcx            # return value = dest
        movzx   r9,  dl             # r9b = byte value
        mov     rdx, r8             # rdx = count
        mov     r10, rcx            # r10 = dst (volatile)

        test    rdx, rdx
        jz      .Ldone

        # Large fast path: rep stosb (simple & robust in boot)
        cmp     rdx, 256
        jb      .Lsmallmed
        push    rdi                 # preserve non-volatile
        cld
        mov     rdi, r10
        mov     al,  r9b
        mov     rcx, rdx
        rep stosb
        pop     rdi
        ret

.Lsmallmed:
        # Medium/small path: align to 8, then qwords, then tail
        cmp     rdx, 32
        jb      .Lsmall

        # Build 64-bit pattern in r11
        movzx   r11, r9b
        mov     rcx, 0x0101010101010101
        imul    r11, rcx

        # Align destination to 8-byte boundary
        mov     rcx, r10
        and     rcx, 7
        jz      .Laligned

        mov     r8, 8
        sub     r8, rcx                 # r8 = bytes to reach 8B align
        cmp     r8, rdx
        cmova   r8, rdx
.Lalign_loop:
        mov     byte ptr [r10], r9b
        inc     r10
        dec     r8
        dec     rdx
        jnz     .Lalign_loop

.Laligned:
        mov     rcx, rdx
        shr     rcx, 3                  # qword count
        jz      .Ltail

.Lqw_loop:
        mov     qword ptr [r10], r11
        add     r10, 8
        dec     rcx
        jnz     .Lqw_loop

.Ltail:
        and     rdx, 7
        jz      .Ldone

.Lsmall:
        mov     rcx, rdx
.Lbyte_loop:
        mov     byte ptr [r10], r9b
        inc     r10
        dec     rcx
        jnz     .Lbyte_loop

.Ldone:
        ret
