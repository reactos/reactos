
/* 
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 2004, 2005, 2006 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: */
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
