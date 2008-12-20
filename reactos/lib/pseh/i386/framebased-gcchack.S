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

.func _SEH2CurrentRegistration
.globl __SEH2CurrentRegistration
__SEH2CurrentRegistration:
	mov eax, [fs:0]
	ret
.endfunc

.func __SEH2EnterFrame
.globl ___SEH2EnterFrame
___SEH2EnterFrame:
	mov eax, [esp+4]
	mov ecx, [fs:0]
	mov [eax], ecx
	mov [fs:0], eax
	ret
.endfunc

.func __SEH2LeaveFrame
.globl ___SEH2LeaveFrame
___SEH2LeaveFrame:
	mov eax, [fs:0]
	mov eax, [eax]
	mov [fs:0], eax
	ret
.endfunc

.func __SEH2Handle
.globl ___SEH2Handle
___SEH2Handle:
	mov eax, [esp+4]
	mov ebp, [esp+8]
	mov esp, [esp+12]
	jmp eax
.endfunc

.func __SEH2Except
.globl ___SEH2Except
___SEH2Except:
	mov eax, [esp+4]
	mov ecx, [esp+8]

	call eax

	ret
.endfunc

.func __SEH2Finally
.globl ___SEH2Finally
___SEH2Finally:
	mov eax, [esp+4]
	mov ecx, [esp+8]

	call eax

	ret
.endfunc

.func __SEH2FrameHandler
.globl ___SEH2FrameHandler
___SEH2FrameHandler:

.extern __SEH2FrameHandler

	cld
	jmp __SEH2FrameHandler
.endfunc

.func __SEH2NestedHandler
.globl ___SEH2NestedHandler
___SEH2NestedHandler:

.extern __SEH2NestedHandler

	cld
	jmp __SEH2NestedHandler
.endfunc

// EOF
