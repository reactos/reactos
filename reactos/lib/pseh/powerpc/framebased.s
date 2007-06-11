# Copyright (c) 2004/2005 KJK::Hyperion

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to dos so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

	.section .text

	.globl _SEHCleanHandlerEnvironment
_SEHCleanHandlerEnvironment:
	blr

	.globl _SEHCurrentRegistration
_SEHCurrentRegistration:
	blr

	.globl _SEHRegisterFrame
_SEHRegisterFrame:
	blr

	.globl _SEHUnregisterFrame
__SEHUnregisterFrame:
	blr

	.globl _SEHGlobalUnwind
_SEHGlobalUnwind:

	.globl _SEHRtlUnwind
# RtlUnwind clobbers all the "don't clobber" registers, so we save them
	blr

	.globl _SEHUnregisterFrame
_SEHUnregisterFrame:
	blr
		
.RestoreRegisters:
	blr

# EOF
