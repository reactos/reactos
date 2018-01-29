/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of _chkstk and _alloca_probe
 * PROGRAMMERS        Richard Henderson <rth@redhat.com>
 *                    Kai Tietz <kai.tietz@onevision.com>
 *                    Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#define PAGE_SIZE 4096

/* CODE **********************************************************************/
.code64

PUBLIC __chkstk
PUBLIC __alloca_probe

__alloca_probe:
.PROC __chkstk

    push rcx                    /* save temps */
    .pushreg rcx
    push rax
    .pushreg rax
    .endprolog

    cmp rax, PAGE_SIZE          /* > 4k ?*/
    lea rcx, [rsp + 24]         /* point past return addr */
    jb l_LessThanAPage

l_MoreThanAPage:
    sub rcx, PAGE_SIZE          /* yes, move pointer down 4k */
    or byte ptr [rcx], 0        /* probe there */
    sub rax, PAGE_SIZE          /* decrement count */

    cmp rax, PAGE_SIZE
    ja l_MoreThanAPage          /* and do it again */

l_LessThanAPage:
    sub rcx, rax
    or byte ptr [rcx], 0        /* less than 4k, just peek here */

    pop rax
    pop rcx
    ret

.ENDP

END
/* EOF */
