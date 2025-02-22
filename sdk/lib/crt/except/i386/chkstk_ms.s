/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Stack checker
 * PROGRAMERS:        KJK::Hyperion <noog@libero.it>
 *                    Richard Henderson <rth@redhat.com>
 *                    Kai Tietz <kai.tietz@onevision.com>
 *                    Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>
#include <ks386.inc>

#define PAGE_SIZE 4096

PUBLIC ___chkstk_ms
.code

/* Special version, that does only probe and not allocate */
___chkstk_ms:

    /* EAX = size to be allocated */
    /* save the ECX and EAX register */
    push ecx
    push eax

    /* ECX = top of the previous stack frame */
    lea ecx, [esp + 12]

    /* probe the desired memory, page by page */
    cmp eax, PAGE_SIZE
    jl .l_LessThanAPage

.l_MoreThanAPage:

    /* raise the top of the stack by a page and probe */
    sub ecx, PAGE_SIZE
    test [ecx], eax

    /* loop if still more than a page must be probed */
    sub eax, PAGE_SIZE
    cmp eax, PAGE_SIZE
    jge .l_MoreThanAPage

.l_LessThanAPage:

    /* raise the top of the stack by EAX bytes (size % 4096) and probe */
    sub ecx, eax
    test [ecx], eax

    /* restore ECX and EAX */
    pop eax
    pop ecx

    /* return */
    ret

/* EOF */
END
