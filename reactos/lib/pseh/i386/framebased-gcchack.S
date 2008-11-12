// Copyright (c) 2008 KJK::Hyperion

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

.globl ___SEH2Except
___SEH2Except:
	mov eax, [esp+4]
	mov ecx, [esp+8]

	push [esp+12]
	call eax

	pop ecx

	ret

.globl ___SEH2Finally
___SEH2Finally:
	mov eax, [esp+4]
	mov ecx, [esp+8]

	call eax

	ret

.globl __SEH2GlobalUnwind
__SEH2GlobalUnwind:

.extern _RtlUnwind@16

	push ebx
	mov ebx, [esp+8]
	push esi
	push edi

	push 0 // ReturnValue
	push 0 // ExceptionRecord
	push .RestoreRegisters // TargetIp
	push ebx // TargetFrame
	call _RtlUnwind@16

.RestoreRegisters:
	pop edi
	pop esi
	pop ebx

	ret

// EOF
