/*
 * PROJECT:         Win32 subsystem
 * LICENSE:         See COPYING in the top level directory
 * FILE:            win32ss/gdi/dib/i386/dib32bpp_hline.s
 * PURPOSE:         ASM optimised 32bpp HLine
 * PROGRAMMERS:     Magnus Olsen
 */

#include <asm.inc>

.code

PUBLIC _DIB_32BPP_HLine

_DIB_32BPP_HLine:
                  sub     esp, 12             // rember the base is not hex it is dec
                  mov     ecx, [esp+16]
                  mov     [esp+4], ebx
                  mov     edx, [esp+20]      // edx =  LONG x1
                  mov     [esp+8], edi
                  mov     edi, [esp+28]
                  mov     eax, [ecx+36]
                  mov     ebx, [esp+24]      // ebx =  LONG x2
                  imul    eax, edi
                  mov     edi, [ecx+32]
                  sub     ebx, edx            // cx  = (x2 - x1) ;
                  add     eax, edi
                  lea     edx, [eax+edx*4]
                  mov     [esp], edx
                  cld
                  mov     eax, [esp+32]
                  mov     edi, [esp]
                  test    edi, 3             // Align to fullword boundary
                  jnz     short _save_rest
                  mov     ecx, ebx           // Setup count of fullwords to fill
                  rep stosd

                  mov     ebx, [esp+4]
                  mov     edi, [esp+8]
                  add     esp, 12
                  ret
_save_rest:
                  stosw
                  ror     eax, 16
                  mov     ecx, ebx           // Setup count of fullwords to fill
                  dec     ecx
                  rep stosd                  // The actual fill
                  shr     eax, 16
                  stosw

                  mov     ebx, [esp+4]
                  mov     edi, [esp+8]
                  add     esp, 12
                  ret

END
