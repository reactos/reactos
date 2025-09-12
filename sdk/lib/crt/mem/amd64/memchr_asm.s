/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     AMD64 optimized memchr implementation
 * COPYRIGHT:   Copyright 2025 ReactOS Team
 */

#include <asm.inc>
#include <ksamd64.inc>

.code64

/*
 * void *memchr(const void *buf, int c, size_t count)
 * 
 * Parameters:
 *   RCX - buffer pointer (buf)
 *   RDX - character to search for (c)
 *   R8  - number of bytes (count)
 *
 * Returns:
 *   RAX - pointer to first occurrence of c, or NULL if not found
 */

LEAF_ENTRY memchr, _TEXT

    /* Windows x64 ABI: RCX = buf, RDX = c, R8 = count */
    mov     r9, rcx         /* Save buf in R9 */
    movzx   eax, dl         /* Extract byte to search for */
    mov     rdx, r8         /* Move count to RDX */
    mov     rdi, rcx        /* Move buf to RDI for scasb */

    /* Check if count is 0 */
    test    rdx, rdx
    jz      .not_found

    /* Use SCASB for byte search */
    mov     rcx, rdx        /* Set counter */
    cld                     /* Clear direction flag (forward scan) */
    repne   scasb           /* Scan for AL in [RDI] */
    
    je      .found          /* If ZF=1, we found it */
    
.not_found:
    xor     rax, rax        /* Return NULL */
    ret

.found:
    /* RDI now points one byte past the match */
    lea     rax, [rdi - 1]  /* Adjust to point to the match */
    ret

LEAF_END memchr, _TEXT