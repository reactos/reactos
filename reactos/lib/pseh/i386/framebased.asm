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

global __SEHCleanHandlerEnvironment
__SEHCleanHandlerEnvironment:
 cld
 ret

global __SEHRegisterFrame
__SEHRegisterFrame:
 mov ecx, [esp+4]
 mov eax, [fs:0]
 mov [ecx+0], eax
 mov [fs:0], ecx
 ret

global __SEHUnregisterFrame
__SEHUnregisterFrame:
 mov ecx, [esp+4]
 mov ecx, [ecx]
 mov [fs:0], ecx
 ret

global __SEHUnwind
__SEHUnwind:

 extern __SEHRtlUnwind

 mov ecx, [esp+4]

; RtlUnwind clobbers all the "don't clobber" registers, so we save them
 push esi
 push edi
 push ebx

 xor eax, eax
 push eax ; ReturnValue
 push eax ; ExceptionRecord
 push eax ; TargetIp
 push ecx ; TargetFrame
 call [__SEHRtlUnwind]

 pop ebx
 pop edi
 pop esi

 ret

; EOF
