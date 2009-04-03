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

.func _SEH2EnterHandleTrylevel
.globl __SEH2EnterHandleTrylevel
__SEH2EnterHandleTrylevel:
	mov eax, [esp+8]
	mov ecx, [esp+12]
	mov [eax+8], ecx
	mov [eax+16], ebp
	mov [eax+20], ebx
	mov [eax+24], esi
	mov [eax+28], edi

	mov edx, [esp+4]
	mov [edx+8], eax

	xor eax, eax
	ret
.endfunc

.func _SEH2EnterFrameAndHandleTrylevel
.globl __SEH2EnterFrameAndHandleTrylevel
__SEH2EnterFrameAndHandleTrylevel:
	mov eax, [esp+8]
	mov ecx, [esp+12]
	mov [eax+8], ecx
	mov [eax+16], ebp
	mov [eax+20], ebx
	mov [eax+24], esi
	mov [eax+28], edi

.extern __SEH2EnterFrameAndTrylevel
	jmp __SEH2EnterFrameAndTrylevel
.endfunc

.func __SEH2Handle
.globl ___SEH2Handle
___SEH2Handle:
	mov eax, [esp+4]
	mov ebp, [esp+12]
	mov ebx, [esp+16]
	mov esi, [esp+20]
	mov edi, [esp+24]
	mov esp, [esp+8]
	jmp eax
.endfunc

.func __SEH2FrameHandler
.globl ___SEH2FrameHandler
___SEH2FrameHandler:

.extern __SEH2FrameHandler

	cld
	jmp __SEH2FrameHandler
.endfunc

.func __SEH2UnwindHandler
.globl ___SEH2UnwindHandler
___SEH2UnwindHandler:

.extern __SEH2UnwindHandler

	cld
	jmp __SEH2UnwindHandler
.endfunc

// EOF
