; Copyright (c) 2004 KJK::Hyperion

; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to dos so, subject to the following conditions:

; The above copyright notice and this permission notice shall be included in all
; copies or substantial portions of the Software.

; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.

cpu 486
segment .text use32

; Note: the undecorated names are for Borland C++ (and possibly other compilers
; using the OMF format)
global SEHSetJmp
global __SEHSetJmp@4
SEHSetJmp:
__SEHSetJmp@4:
 ; jump buffer
 mov eax, [esp+4]

 ; program counter
 mov ecx, [esp+0]

 ; stack pointer
 lea edx, [esp+4]

 ; fill the jump buffer
 mov [eax+0], ebp
 mov [eax+4], edx
 mov [eax+8], ecx
 mov [eax+12], ebx
 mov [eax+16], esi
 mov [eax+20], edi

 xor eax, eax
 ret 4

global SEHLongJmp
global __SEHLongJmp@8
SEHLongJmp:
__SEHLongJmp@8:
 ; return value
 mov eax, [esp+8]

 ; jump buffer
 mov ecx, [esp+4]

 ; restore the saved context
 mov ebp, [ecx+0]
 mov esp, [ecx+4]
 mov edx, [ecx+8]
 mov ebx, [ecx+12]
 mov esi, [ecx+16]
 mov edi, [ecx+20]
 jmp edx

; EOF
