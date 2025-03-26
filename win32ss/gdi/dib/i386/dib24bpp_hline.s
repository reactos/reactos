/*
 * PROJECT:         Win32 subsystem
 * LICENSE:         See COPYING in the top level directory
 * FILE:            win32ss/gdi/dib/i386/dib24bpp_hline.s
 * PURPOSE:         ASM optimised 24bpp HLine
 * PROGRAMMERS:     Magnus Olsen
 */

#include <asm.inc>

.code

PUBLIC _DIB_24BPP_HLine

      _DIB_24BPP_HLine:
                         push    edi
                         push    esi
                         push    ebx
                         sub     esp, 24
                         mov     ebx, [esp+40]
                         mov     edi, [esp+52]
                         mov     ecx, [esp+44]
                         mov     eax, [ebx+36]
                         mov     esi, [ebx+32]
                         mov     edx, [esp+48]
                         imul    eax, edi
                         sub     edx, ecx
                         mov     [esp], edx
                         add     eax, esi
                         lea     eax, [eax+ecx*2]
                         add     eax, ecx
                         cmp     edx, 7
                         mov     esi, edx
                         mov     [esp+4], eax
                         ja      Align4byte
                         lea     eax, [edx-1]
                         mov     [esp], eax
                         inc     eax
                         jnz     small_fill
                         add     esp, 24
                         pop     ebx
                         pop     esi
                         pop     edi
                         ret

                        /* For small fills, don't bother doing anything fancy */
               small_fill:
                         movzx   ecx, word ptr [esp+58]
                         mov     edx, [esp+4]
                         mov     esi, [esp+56]
                         lea     eax, [edx+2]
                         mov     [esp+4], eax
                         mov     [edx+2], cl
                         mov     eax, [esp]
                         inc     dword ptr [esp+4]
                         mov     [edx], si
                         dec     eax
                         mov     [esp], eax
                         inc     eax
                         jnz     small_fill
                         add     esp, 24
                         pop     ebx
                         pop     esi
                         pop     edi
                         ret

               Align4byte:
                         /* Align to 4-byte address */
                         test    al, 3
                         mov     ecx, eax
                         jz      loop1
                         lea     esi, [esi+0]
                         lea     edi, [edi+0]

           loopasmversion:
                        /* This is about 30% faster than the generic C code below */
                         movzx   edx, word ptr [esp+58]
                         lea     edi, [ecx+2]
                         mov     eax, [esp+56]
                         mov     [esp+4], edi
                         mov     [ecx+2], dl
                         mov     ebx, [esp+4]
                         mov     [ecx], ax
                         mov     edx, [esp]
                         inc     ebx
                         mov     [esp+4], ebx
                         dec     edx
                         test    bl, 3
                         mov     [esp], edx
                         mov     ecx, ebx
                         jnz     loopasmversion
                         mov     esi, edx

                   loop1:
                         mov     ecx, [esp+56]
                         and     ecx, 16777215
                         mov     ebx, ecx
                         shr     ebx, 8
                         mov     eax, ecx
                         shl     eax, 16
                         or      ebx, eax
                         mov     edx, ecx
                         shl     edx, 8
                         mov     eax, ecx
                         shr     eax, 16
                         or      edx, eax
                         mov     eax, ecx
                         shl     eax, 24
                         or      eax, ecx
                         mov     ecx, [esp]
                         shr     ecx, 2
                         mov     edi, [esp+4]
                    loop2:
                         mov     [edi], eax
                         mov     [edi+4], ebx
                         mov     [edi+8], edx
                         add     edi, 12
                         dec     ecx
                         jnz     loop2
                         mov     [esp+4], edi
                         and     esi, 3
                         lea     eax, [esi-1]
                         mov     [esp], eax
                         inc     eax
                         jnz     leftoverfromthemainloop
                         add     esp, 24
                         pop     ebx
                         pop     esi
                         pop     edi
                         ret

  leftoverfromthemainloop:

                        /*  Count = Count & 0x03; */
                         mov     ecx, [esp+4]
                         mov     ebx, [esp+56]
                         lea     esi, [ecx+2]
                         mov     [ecx], bx
                         shr     ebx, 16
                         mov     [esp+4], esi
                         mov     [ecx+2], bl
                         mov     eax, [esp]
                         inc     dword ptr [esp+4]
                         dec     eax
                         mov     [esp], eax
                         inc     eax
                         jnz     leftoverfromthemainloop
                         add     esp, 24
                         pop     ebx
                         pop     esi
                         pop     edi
                         ret

END
