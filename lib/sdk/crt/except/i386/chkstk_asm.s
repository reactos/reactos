/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Stack checker
 * FILE:              lib/ntdll/rtl/i386/chkstk.s
 * PROGRAMER:         KJK::Hyperion <noog@libero.it>
 */

#include <reactos/asm.h>
#include <ndk/asm.h>
#define PAGE_SIZE 4096

PUBLIC __chkstk
PUBLIC __alloca_probe
.code

/*
 _chkstk() is called by all stack allocations of more than 4 KB. It grows the
 stack in areas of 4 KB each, trying to access each area. This ensures that the
 guard page for the stack is hit, and the stack growing triggered
 */
__chkstk:
__alloca_probe:

    /* EAX = size to be allocated */
    /* save the ECX register */
    push ecx

    /* ECX = top of the previous stack frame */
    lea eax, [esp + 8]

    /* probe the desired memory, page by page */
    cmp eax, PAGE_SIZE
    jge .l_MoreThanAPage
    jmp .l_LessThanAPage

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

    /* EAX = top of the stack */
    mov eax, esp

    /* allocate the memory */
    mov esp, ecx

    /* restore ECX */
    mov ecx, [eax]

    /* restore the return address */
    mov eax, [eax + 4]
    push eax

    /* return */
    ret

/* EOF */
END
