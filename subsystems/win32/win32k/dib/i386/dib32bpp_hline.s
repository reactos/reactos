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

.globl _DIB_32BPP_HLine
.intel_syntax noprefix

.def   _DIB_32BPP_HLine;
.scl	2;
.type	32;
.endef

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

