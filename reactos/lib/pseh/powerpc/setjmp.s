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

# Note: the undecorated names are for Borland C++ (and possibly other compilers
# using the OMF format)
	.globl _SEHSetJmp
	.globl __SEHSetJmp
_SEHSetJmp:
__SEHSetJmp:
	blr

	.globl _SEHLongJmp
	.globl __SEHLongJmp
_SEHLongJmp:
__SEHLongJmp:
	blr

	.globl _SEHLongJmp_KeepEsp
	.globl __SEHLongJmp_KeepEsp
_SEHLongJmp_KeepEsp:
__SEHLongJmp_KeepEsp:
	blr

# EOF
