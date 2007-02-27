; Copyright (c) 2004/2005 KJK::Hyperion

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

segment .text use32

global __SEHCleanHandlerEnvironment
__SEHCleanHandlerEnvironment:
	cld
	ret

global __SEHCurrentRegistration
__SEHCurrentRegistration:
	mov eax, [fs:0]
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
	mov ecx, [fs:0]
	mov ecx, [ecx+0]
	mov [fs:0], ecx
	ret

global __SEHGlobalUnwind
__SEHGlobalUnwind:

	extern __SEHRtlUnwind

; RtlUnwind clobbers all the "don't clobber" registers, so we save them
	push ebx
	mov ebx, [esp+8]
	push esi
	push edi

	push dword 0x0 ; ReturnValue
	push dword 0x0 ; ExceptionRecord
	push dword .RestoreRegisters ; TargetIp
	push ebx ; TargetFrame
	call [__SEHRtlUnwind]

.RestoreRegisters:
	pop edi
	pop esi
	pop ebx

	ret

; EOF
