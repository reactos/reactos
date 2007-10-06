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

__readfsdword:
        add 7,13,3
        lwz 3,0(7)
        blr

__writefsdword:
        add 7,13,3
        stw 3,0(7)
        blr
        
.globl _SEHCleanHandlerEnvironment
_SEHCleanHandlerEnvironment:
        blr

.globl _SEHCurrentRegistration
_SEHCurrentRegistration:
        xor 3,0,0
        bl __readfsdword
        blr

.globl _SEHRegisterFrame
_SEHRegisterFrame:
        xor 3,0,0
        bl __readfsdword        # Get [fs:0]
        lwz 4,0(1)              # Get next stack frame into r4
        stw 3,0(1)              # put [fs:0] into old stack pos
        xor 3,0,0
        bl __writefsdword       # put old stack frame into [fs:0] completing
        blr                     # the chain and return

.globl _SEHUnregisterFrame
_SEHUnregisterFrame:
        xor 3,0,0
        bl __readfsdword        # Get old next frame ptr
        stw 3,0(1)              # Reset old stack frame
        blr

.globl _SEHGlobalUnwind
_SEHGlobalUnwind:

.extern _SEHRtlUnwind

// RtlUnwind clobbers all the "don't clobber" registers, so we save them
        lwz 3,4(1)
        stwu 1,-132(1)
        stmw 2,-128(1)
        
        xor 6,6,6
        xor 5,5,5
        lis 4,.RestoreRegisters@ha
        addi 4,4,.RestoreRegisters@l # Where to jump back to
        # We already have r3
        bl _SEHRtlUnwind

.RestoreRegisters:
        lmw 2,-128(1)
        addi 1,1,132
        blr

// EOF
