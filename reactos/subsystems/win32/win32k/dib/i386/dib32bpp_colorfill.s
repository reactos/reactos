/*
 * PROJECT:         Win32 subsystem
 * LICENSE:         See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/dib/i386/dib32bpp_colorfill.c
 * PURPOSE:         ASM optimised 32bpp ColorFill
 * PROGRAMMERS:     Magnus Olsen
 */

  .globl _DIB_32BPP_ColorFill
  .intel_syntax noprefix

  .def   _DIB_32BPP_ColorFill;
  .scl	2;
  .type	32;
  .endef
  
  _DIB_32BPP_ColorFill:
                        sub     esp, 24
                        mov     ecx, [esp+32]
                        mov     [esp+8], ebx
                        mov     ebx, [esp+28]
                        mov     [esp+20], ebp
                        mov     ebp, [esp+36]
                        mov     [esp+12], esi
                        mov     [esp+16], edi
                        mov     edi, [ecx]
                        mov     esi, [ecx+8]
                        mov     edx, [ebx+36]
                        sub     esi, edi
                        mov     edi, [ecx+4]
                        mov     eax, edi
                        imul    eax, edx
                        add     eax, [ebx+32]
                        mov     ebx, [ecx]
                        lea     eax, [eax+ebx*4]
                        mov     [esp+4], eax
                        mov     eax, [ecx+12]
                        cmp     eax, edi
                        jbe     end
                        sub     eax, edi
                        mov     [esp], eax
                        lea     esi, [esi+0]

               for_loop:
                        mov     eax, ebp
                        cld
                        mov     ebx, esi
                        mov     edi, [esp+4]
                        test    edi, 3
                        jnz     algin_draw
                        mov     ecx, esi
                        rep stosd
                        add     [esp+4], edx
                        dec     dword ptr [esp]
                        jnz     for_loop
               end:
                        mov     ebx, [esp+8]
                        mov     eax, 1
                        mov     esi, [esp+12]
                        mov     edi, [esp+16]
                        mov     ebp, [esp+20]
                        add     esp, 24
                        ret

               algin_draw:
                        stosd
                        dec     ebx
                        mov     ecx, ebx
                        rol     eax, 16
                        stosd
                        add     [esp+4], edx
                        dec     dword ptr [esp]
                        jnz     for_loop

                        mov     ebx, [esp+8]
                        mov     eax, 1
                        mov     esi, [esp+12]
                        mov     edi, [esp+16]
                        mov     ebp, [esp+20]
                        add     esp, 24
                        ret
