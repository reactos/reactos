// Copyright (c) 2004/2005 KJK::Hyperion

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to dos so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

.text
.intel_syntax noprefix

.globl __SEHCleanHandlerEnvironment
__SEHCleanHandlerEnvironment:
	cld
	ret

.globl __SEHCurrentRegistration
__SEHCurrentRegistration:
	mov rax, [fs:0]
	ret

.globl __SEHRegisterFrame
__SEHRegisterFrame:
	mov rcx, [rsp+4]
	mov rax, [fs:0]
	mov [rcx+0], eax
	mov [fs:0], ecx
	ret

.globl __SEHUnregisterFrame
__SEHUnregisterFrame:
	mov rcx, [fs:0]
	mov rcx, [rcx+0]
	mov [fs:0], rcx
	ret

.globl __SEHGlobalUnwind
__SEHGlobalUnwind:

.extern __SEHRtlUnwind

// RtlUnwind clobbers all the "don't clobber" registers, so we save them
	push rbx
	mov rbx, [rsp+8]
	push rsi
	push rdi

	push 0 // ReturnValue
	push 0 // ExceptionRecord
	push .RestoreRegisters // TargetIp
	push rbx // TargetFrame
	call [__SEHRtlUnwind]

.RestoreRegisters:
	pop rdi
	pop rsi
	pop rbx

	ret

// EOF
