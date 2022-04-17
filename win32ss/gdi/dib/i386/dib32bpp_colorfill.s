/*
 * PROJECT:         Win32 subsystem
 * LICENSE:         See COPYING in the top level directory
 * FILE:            win32ss/gdi/dib/i386/dib32bpp_colorfill.s
 * PURPOSE:         ASM optimised 32bpp ColorFill
 * PROGRAMMERS:     Magnus Olsen
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <asm.inc>

.code
/*
 * BOOLEAN
 * _cdecl
 * DIB_32BPP_ColorFill(SURFOBJ* pso, RECTL* prcl, ULONG iColor);
*/

PUBLIC _DIB_32BPP_ColorFill
_DIB_32BPP_ColorFill:
        push    ebp
        mov     ebp, esp
        push    ebx
        push    esi
        push    edi
        sub     esp, 4            /* Space for lDelta */

        mov     edx, [ebp+12]     /* edx = prcl */
        mov     ecx, [ebp+8]      /* ecx = pso */

        mov     ebx, [ecx+36]   /* ebx = pso->lDelta; */
        mov     [esp], ebx        /* lDelta = pso->lDelta; */
        mov     edi, [edx+4]      /* edi = prcl->top; */
        mov     eax, edi          /* eax = prcl->top; */
        imul    eax, ebx          /* eax = prcl->top * pso->lDelta; */
        add     eax, [ecx+32]   /* eax += pso->pvScan0; */
        mov     ebx, [edx]        /* ebx = prcl->left; */
        lea     esi, [eax+ebx*4]  /* esi = pvLine0 = eax + 4 * prcl->left; */

        mov     ebx, [edx+8]      /* ebx = prcl->right; */
        sub     ebx, [edx]        /* ebx = prcl->right - prcl->left; */
        jle     .end               /* if (ebx <= 0) goto end; */

        mov     edx, [edx+12]     /* edx = prcl->bottom; */
        sub     edx, edi          /* edx -= prcl->top; */
        jle     .end               /* if (eax <= 0) goto end; */

        mov     eax, [ebp+16]     /* eax = iColor; */
        cld

for_loop:                         /* do { */
        mov     edi, esi          /*   edi = pvLine0; */
        mov     ecx, ebx          /*   ecx = cx; */
        rep stosd                 /*   memset(pvLine0, iColor, cx); */
        add     esi, [esp]        /*   pvLine0 += lDelta; */
        dec     edx               /*   cy--; */
        jnz     for_loop          /* } while (cy > 0); */

.end:
        mov     eax, 1
        add     esp, 4
        pop     edi
        pop     esi
        pop     ebx
        pop     ebp
        ret

END
