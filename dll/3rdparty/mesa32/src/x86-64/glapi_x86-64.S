/* DO NOT EDIT - This file generated automatically by gl_x86-64_asm.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2005
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* If we build with gcc's -fvisibility=hidden flag, we'll need to change
 * the symbol visibility mode to 'default'.
 */

#include "../x86/assyntax.h"

#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 303
#  pragma GCC visibility push(default)
#  define HIDDEN(x) .hidden x
#else
#  define HIDDEN(x)
#endif

# if defined(USE_MGL_NAMESPACE)
#  define GL_PREFIX(n) GLNAME(CONCAT(mgl,n))
#  define _glapi_Dispatch _mglapi_Dispatch
# else
#  define GL_PREFIX(n) GLNAME(CONCAT(gl,n))
# endif

#if defined(PTHREADS) || defined(USE_XTHREADS) || defined(SOLARIS_THREADS) || defined(WIN32_THREADS) || defined(BEOS_THREADS)
#  define THREADS
#endif

	.text

#ifdef GLX_USE_TLS

	.globl _x86_64_get_get_dispatch; HIDDEN(_x86_64_get_get_dispatch)
_x86_64_get_get_dispatch:
	lea	_x86_64_get_dispatch(%rip), %rax
	ret

	.p2align	4,,15
_x86_64_get_dispatch:
	movq	_glapi_tls_Dispatch@GOTTPOFF(%rip), %rax
	movq	%fs:(%rax), %rax
	ret
	.size	_x86_64_get_dispatch, .-_x86_64_get_dispatch

#elif defined(PTHREADS)

	.extern	_glapi_Dispatch
	.extern	_gl_DispatchTSD
	.extern	pthread_getspecific

	.p2align	4,,15
_x86_64_get_dispatch:
	movq	_gl_DispatchTSD(%rip), %rdi
	jmp	pthread_getspecific@PLT

#elif defined(THREADS)

	.extern	_glapi_get_dispatch

#endif

	.p2align	4,,15
	.globl	GL_PREFIX(NewList)
	.type	GL_PREFIX(NewList), @function
GL_PREFIX(NewList):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	0(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	0(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	0(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(NewList), .-GL_PREFIX(NewList)

	.p2align	4,,15
	.globl	GL_PREFIX(EndList)
	.type	GL_PREFIX(EndList), @function
GL_PREFIX(EndList):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	8(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	8(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	8(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EndList), .-GL_PREFIX(EndList)

	.p2align	4,,15
	.globl	GL_PREFIX(CallList)
	.type	GL_PREFIX(CallList), @function
GL_PREFIX(CallList):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	16(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	16(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	16(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CallList), .-GL_PREFIX(CallList)

	.p2align	4,,15
	.globl	GL_PREFIX(CallLists)
	.type	GL_PREFIX(CallLists), @function
GL_PREFIX(CallLists):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	24(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	24(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	24(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CallLists), .-GL_PREFIX(CallLists)

	.p2align	4,,15
	.globl	GL_PREFIX(DeleteLists)
	.type	GL_PREFIX(DeleteLists), @function
GL_PREFIX(DeleteLists):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	32(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	32(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	32(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	32(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DeleteLists), .-GL_PREFIX(DeleteLists)

	.p2align	4,,15
	.globl	GL_PREFIX(GenLists)
	.type	GL_PREFIX(GenLists), @function
GL_PREFIX(GenLists):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	40(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	40(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	40(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	40(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GenLists), .-GL_PREFIX(GenLists)

	.p2align	4,,15
	.globl	GL_PREFIX(ListBase)
	.type	GL_PREFIX(ListBase), @function
GL_PREFIX(ListBase):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	48(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	48(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	48(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	48(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ListBase), .-GL_PREFIX(ListBase)

	.p2align	4,,15
	.globl	GL_PREFIX(Begin)
	.type	GL_PREFIX(Begin), @function
GL_PREFIX(Begin):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	56(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	56(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	56(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	56(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Begin), .-GL_PREFIX(Begin)

	.p2align	4,,15
	.globl	GL_PREFIX(Bitmap)
	.type	GL_PREFIX(Bitmap), @function
GL_PREFIX(Bitmap):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	64(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	movq	%xmm1, 24(%rsp)
	movq	%xmm2, 32(%rsp)
	movq	%xmm3, 40(%rsp)
	movq	%rdx, 48(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	48(%rsp), %rdx
	movq	40(%rsp), %xmm3
	movq	32(%rsp), %xmm2
	movq	24(%rsp), %xmm1
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	64(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	64(%rax), %r11
	jmp	*%r11
1:
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	movq	%xmm1, 24(%rsp)
	movq	%xmm2, 32(%rsp)
	movq	%xmm3, 40(%rsp)
	movq	%rdx, 48(%rsp)
	call	_glapi_get_dispatch
	movq	48(%rsp), %rdx
	movq	40(%rsp), %xmm3
	movq	32(%rsp), %xmm2
	movq	24(%rsp), %xmm1
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	64(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Bitmap), .-GL_PREFIX(Bitmap)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3b)
	.type	GL_PREFIX(Color3b), @function
GL_PREFIX(Color3b):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	72(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	72(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	72(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	72(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3b), .-GL_PREFIX(Color3b)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3bv)
	.type	GL_PREFIX(Color3bv), @function
GL_PREFIX(Color3bv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	80(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	80(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	80(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	80(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3bv), .-GL_PREFIX(Color3bv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3d)
	.type	GL_PREFIX(Color3d), @function
GL_PREFIX(Color3d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	88(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	88(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	88(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	88(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3d), .-GL_PREFIX(Color3d)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3dv)
	.type	GL_PREFIX(Color3dv), @function
GL_PREFIX(Color3dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	96(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	96(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	96(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	96(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3dv), .-GL_PREFIX(Color3dv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3f)
	.type	GL_PREFIX(Color3f), @function
GL_PREFIX(Color3f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	104(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	104(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	104(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	104(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3f), .-GL_PREFIX(Color3f)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3fv)
	.type	GL_PREFIX(Color3fv), @function
GL_PREFIX(Color3fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	112(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	112(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	112(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	112(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3fv), .-GL_PREFIX(Color3fv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3i)
	.type	GL_PREFIX(Color3i), @function
GL_PREFIX(Color3i):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	120(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	120(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	120(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	120(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3i), .-GL_PREFIX(Color3i)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3iv)
	.type	GL_PREFIX(Color3iv), @function
GL_PREFIX(Color3iv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	128(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	128(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	128(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	128(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3iv), .-GL_PREFIX(Color3iv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3s)
	.type	GL_PREFIX(Color3s), @function
GL_PREFIX(Color3s):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	136(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	136(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	136(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	136(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3s), .-GL_PREFIX(Color3s)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3sv)
	.type	GL_PREFIX(Color3sv), @function
GL_PREFIX(Color3sv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	144(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	144(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	144(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	144(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3sv), .-GL_PREFIX(Color3sv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3ub)
	.type	GL_PREFIX(Color3ub), @function
GL_PREFIX(Color3ub):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	152(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	152(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	152(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	152(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3ub), .-GL_PREFIX(Color3ub)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3ubv)
	.type	GL_PREFIX(Color3ubv), @function
GL_PREFIX(Color3ubv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	160(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	160(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	160(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	160(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3ubv), .-GL_PREFIX(Color3ubv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3ui)
	.type	GL_PREFIX(Color3ui), @function
GL_PREFIX(Color3ui):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	168(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	168(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	168(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	168(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3ui), .-GL_PREFIX(Color3ui)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3uiv)
	.type	GL_PREFIX(Color3uiv), @function
GL_PREFIX(Color3uiv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	176(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	176(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	176(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	176(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3uiv), .-GL_PREFIX(Color3uiv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3us)
	.type	GL_PREFIX(Color3us), @function
GL_PREFIX(Color3us):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	184(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	184(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	184(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	184(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3us), .-GL_PREFIX(Color3us)

	.p2align	4,,15
	.globl	GL_PREFIX(Color3usv)
	.type	GL_PREFIX(Color3usv), @function
GL_PREFIX(Color3usv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	192(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	192(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	192(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	192(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color3usv), .-GL_PREFIX(Color3usv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4b)
	.type	GL_PREFIX(Color4b), @function
GL_PREFIX(Color4b):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	200(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	200(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	200(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	200(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4b), .-GL_PREFIX(Color4b)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4bv)
	.type	GL_PREFIX(Color4bv), @function
GL_PREFIX(Color4bv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	208(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	208(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	208(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	208(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4bv), .-GL_PREFIX(Color4bv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4d)
	.type	GL_PREFIX(Color4d), @function
GL_PREFIX(Color4d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	216(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	216(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	216(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	216(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4d), .-GL_PREFIX(Color4d)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4dv)
	.type	GL_PREFIX(Color4dv), @function
GL_PREFIX(Color4dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	224(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	224(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	224(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	224(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4dv), .-GL_PREFIX(Color4dv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4f)
	.type	GL_PREFIX(Color4f), @function
GL_PREFIX(Color4f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	232(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	232(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	232(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	232(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4f), .-GL_PREFIX(Color4f)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4fv)
	.type	GL_PREFIX(Color4fv), @function
GL_PREFIX(Color4fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	240(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	240(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	240(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	240(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4fv), .-GL_PREFIX(Color4fv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4i)
	.type	GL_PREFIX(Color4i), @function
GL_PREFIX(Color4i):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	248(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	248(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	248(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	248(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4i), .-GL_PREFIX(Color4i)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4iv)
	.type	GL_PREFIX(Color4iv), @function
GL_PREFIX(Color4iv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	256(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	256(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	256(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	256(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4iv), .-GL_PREFIX(Color4iv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4s)
	.type	GL_PREFIX(Color4s), @function
GL_PREFIX(Color4s):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	264(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	264(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	264(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	264(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4s), .-GL_PREFIX(Color4s)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4sv)
	.type	GL_PREFIX(Color4sv), @function
GL_PREFIX(Color4sv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	272(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	272(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	272(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	272(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4sv), .-GL_PREFIX(Color4sv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4ub)
	.type	GL_PREFIX(Color4ub), @function
GL_PREFIX(Color4ub):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	280(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	280(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	280(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	280(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4ub), .-GL_PREFIX(Color4ub)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4ubv)
	.type	GL_PREFIX(Color4ubv), @function
GL_PREFIX(Color4ubv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	288(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	288(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	288(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	288(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4ubv), .-GL_PREFIX(Color4ubv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4ui)
	.type	GL_PREFIX(Color4ui), @function
GL_PREFIX(Color4ui):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	296(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	296(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	296(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	296(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4ui), .-GL_PREFIX(Color4ui)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4uiv)
	.type	GL_PREFIX(Color4uiv), @function
GL_PREFIX(Color4uiv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	304(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	304(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	304(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	304(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4uiv), .-GL_PREFIX(Color4uiv)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4us)
	.type	GL_PREFIX(Color4us), @function
GL_PREFIX(Color4us):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	312(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	312(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	312(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	312(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4us), .-GL_PREFIX(Color4us)

	.p2align	4,,15
	.globl	GL_PREFIX(Color4usv)
	.type	GL_PREFIX(Color4usv), @function
GL_PREFIX(Color4usv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	320(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	320(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	320(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	320(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Color4usv), .-GL_PREFIX(Color4usv)

	.p2align	4,,15
	.globl	GL_PREFIX(EdgeFlag)
	.type	GL_PREFIX(EdgeFlag), @function
GL_PREFIX(EdgeFlag):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	328(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	328(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	328(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	328(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EdgeFlag), .-GL_PREFIX(EdgeFlag)

	.p2align	4,,15
	.globl	GL_PREFIX(EdgeFlagv)
	.type	GL_PREFIX(EdgeFlagv), @function
GL_PREFIX(EdgeFlagv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	336(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	336(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	336(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	336(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EdgeFlagv), .-GL_PREFIX(EdgeFlagv)

	.p2align	4,,15
	.globl	GL_PREFIX(End)
	.type	GL_PREFIX(End), @function
GL_PREFIX(End):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	344(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	344(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	344(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	344(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(End), .-GL_PREFIX(End)

	.p2align	4,,15
	.globl	GL_PREFIX(Indexd)
	.type	GL_PREFIX(Indexd), @function
GL_PREFIX(Indexd):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	352(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	352(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	352(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	352(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Indexd), .-GL_PREFIX(Indexd)

	.p2align	4,,15
	.globl	GL_PREFIX(Indexdv)
	.type	GL_PREFIX(Indexdv), @function
GL_PREFIX(Indexdv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	360(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	360(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	360(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	360(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Indexdv), .-GL_PREFIX(Indexdv)

	.p2align	4,,15
	.globl	GL_PREFIX(Indexf)
	.type	GL_PREFIX(Indexf), @function
GL_PREFIX(Indexf):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	368(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	368(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	368(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	368(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Indexf), .-GL_PREFIX(Indexf)

	.p2align	4,,15
	.globl	GL_PREFIX(Indexfv)
	.type	GL_PREFIX(Indexfv), @function
GL_PREFIX(Indexfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	376(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	376(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	376(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	376(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Indexfv), .-GL_PREFIX(Indexfv)

	.p2align	4,,15
	.globl	GL_PREFIX(Indexi)
	.type	GL_PREFIX(Indexi), @function
GL_PREFIX(Indexi):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	384(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	384(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	384(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	384(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Indexi), .-GL_PREFIX(Indexi)

	.p2align	4,,15
	.globl	GL_PREFIX(Indexiv)
	.type	GL_PREFIX(Indexiv), @function
GL_PREFIX(Indexiv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	392(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	392(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	392(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	392(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Indexiv), .-GL_PREFIX(Indexiv)

	.p2align	4,,15
	.globl	GL_PREFIX(Indexs)
	.type	GL_PREFIX(Indexs), @function
GL_PREFIX(Indexs):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	400(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	400(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	400(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	400(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Indexs), .-GL_PREFIX(Indexs)

	.p2align	4,,15
	.globl	GL_PREFIX(Indexsv)
	.type	GL_PREFIX(Indexsv), @function
GL_PREFIX(Indexsv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	408(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	408(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	408(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	408(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Indexsv), .-GL_PREFIX(Indexsv)

	.p2align	4,,15
	.globl	GL_PREFIX(Normal3b)
	.type	GL_PREFIX(Normal3b), @function
GL_PREFIX(Normal3b):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	416(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	416(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	416(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	416(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Normal3b), .-GL_PREFIX(Normal3b)

	.p2align	4,,15
	.globl	GL_PREFIX(Normal3bv)
	.type	GL_PREFIX(Normal3bv), @function
GL_PREFIX(Normal3bv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	424(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	424(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	424(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	424(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Normal3bv), .-GL_PREFIX(Normal3bv)

	.p2align	4,,15
	.globl	GL_PREFIX(Normal3d)
	.type	GL_PREFIX(Normal3d), @function
GL_PREFIX(Normal3d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	432(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	432(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	432(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	432(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Normal3d), .-GL_PREFIX(Normal3d)

	.p2align	4,,15
	.globl	GL_PREFIX(Normal3dv)
	.type	GL_PREFIX(Normal3dv), @function
GL_PREFIX(Normal3dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	440(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	440(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	440(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	440(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Normal3dv), .-GL_PREFIX(Normal3dv)

	.p2align	4,,15
	.globl	GL_PREFIX(Normal3f)
	.type	GL_PREFIX(Normal3f), @function
GL_PREFIX(Normal3f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	448(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	448(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	448(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	448(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Normal3f), .-GL_PREFIX(Normal3f)

	.p2align	4,,15
	.globl	GL_PREFIX(Normal3fv)
	.type	GL_PREFIX(Normal3fv), @function
GL_PREFIX(Normal3fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	456(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	456(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	456(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	456(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Normal3fv), .-GL_PREFIX(Normal3fv)

	.p2align	4,,15
	.globl	GL_PREFIX(Normal3i)
	.type	GL_PREFIX(Normal3i), @function
GL_PREFIX(Normal3i):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	464(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	464(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	464(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	464(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Normal3i), .-GL_PREFIX(Normal3i)

	.p2align	4,,15
	.globl	GL_PREFIX(Normal3iv)
	.type	GL_PREFIX(Normal3iv), @function
GL_PREFIX(Normal3iv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	472(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	472(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	472(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	472(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Normal3iv), .-GL_PREFIX(Normal3iv)

	.p2align	4,,15
	.globl	GL_PREFIX(Normal3s)
	.type	GL_PREFIX(Normal3s), @function
GL_PREFIX(Normal3s):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	480(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	480(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	480(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	480(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Normal3s), .-GL_PREFIX(Normal3s)

	.p2align	4,,15
	.globl	GL_PREFIX(Normal3sv)
	.type	GL_PREFIX(Normal3sv), @function
GL_PREFIX(Normal3sv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	488(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	488(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	488(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	488(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Normal3sv), .-GL_PREFIX(Normal3sv)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos2d)
	.type	GL_PREFIX(RasterPos2d), @function
GL_PREFIX(RasterPos2d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	496(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	496(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	496(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	496(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos2d), .-GL_PREFIX(RasterPos2d)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos2dv)
	.type	GL_PREFIX(RasterPos2dv), @function
GL_PREFIX(RasterPos2dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	504(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	504(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	504(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	504(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos2dv), .-GL_PREFIX(RasterPos2dv)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos2f)
	.type	GL_PREFIX(RasterPos2f), @function
GL_PREFIX(RasterPos2f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	512(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	512(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	512(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	512(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos2f), .-GL_PREFIX(RasterPos2f)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos2fv)
	.type	GL_PREFIX(RasterPos2fv), @function
GL_PREFIX(RasterPos2fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	520(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	520(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	520(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	520(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos2fv), .-GL_PREFIX(RasterPos2fv)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos2i)
	.type	GL_PREFIX(RasterPos2i), @function
GL_PREFIX(RasterPos2i):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	528(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	528(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	528(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	528(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos2i), .-GL_PREFIX(RasterPos2i)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos2iv)
	.type	GL_PREFIX(RasterPos2iv), @function
GL_PREFIX(RasterPos2iv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	536(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	536(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	536(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	536(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos2iv), .-GL_PREFIX(RasterPos2iv)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos2s)
	.type	GL_PREFIX(RasterPos2s), @function
GL_PREFIX(RasterPos2s):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	544(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	544(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	544(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	544(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos2s), .-GL_PREFIX(RasterPos2s)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos2sv)
	.type	GL_PREFIX(RasterPos2sv), @function
GL_PREFIX(RasterPos2sv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	552(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	552(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	552(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	552(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos2sv), .-GL_PREFIX(RasterPos2sv)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos3d)
	.type	GL_PREFIX(RasterPos3d), @function
GL_PREFIX(RasterPos3d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	560(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	560(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	560(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	560(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos3d), .-GL_PREFIX(RasterPos3d)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos3dv)
	.type	GL_PREFIX(RasterPos3dv), @function
GL_PREFIX(RasterPos3dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	568(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	568(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	568(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	568(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos3dv), .-GL_PREFIX(RasterPos3dv)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos3f)
	.type	GL_PREFIX(RasterPos3f), @function
GL_PREFIX(RasterPos3f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	576(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	576(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	576(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	576(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos3f), .-GL_PREFIX(RasterPos3f)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos3fv)
	.type	GL_PREFIX(RasterPos3fv), @function
GL_PREFIX(RasterPos3fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	584(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	584(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	584(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	584(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos3fv), .-GL_PREFIX(RasterPos3fv)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos3i)
	.type	GL_PREFIX(RasterPos3i), @function
GL_PREFIX(RasterPos3i):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	592(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	592(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	592(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	592(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos3i), .-GL_PREFIX(RasterPos3i)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos3iv)
	.type	GL_PREFIX(RasterPos3iv), @function
GL_PREFIX(RasterPos3iv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	600(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	600(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	600(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	600(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos3iv), .-GL_PREFIX(RasterPos3iv)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos3s)
	.type	GL_PREFIX(RasterPos3s), @function
GL_PREFIX(RasterPos3s):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	608(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	608(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	608(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	608(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos3s), .-GL_PREFIX(RasterPos3s)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos3sv)
	.type	GL_PREFIX(RasterPos3sv), @function
GL_PREFIX(RasterPos3sv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	616(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	616(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	616(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	616(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos3sv), .-GL_PREFIX(RasterPos3sv)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos4d)
	.type	GL_PREFIX(RasterPos4d), @function
GL_PREFIX(RasterPos4d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	624(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	624(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	624(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	624(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos4d), .-GL_PREFIX(RasterPos4d)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos4dv)
	.type	GL_PREFIX(RasterPos4dv), @function
GL_PREFIX(RasterPos4dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	632(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	632(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	632(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	632(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos4dv), .-GL_PREFIX(RasterPos4dv)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos4f)
	.type	GL_PREFIX(RasterPos4f), @function
GL_PREFIX(RasterPos4f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	640(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	640(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	640(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	640(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos4f), .-GL_PREFIX(RasterPos4f)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos4fv)
	.type	GL_PREFIX(RasterPos4fv), @function
GL_PREFIX(RasterPos4fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	648(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	648(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	648(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	648(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos4fv), .-GL_PREFIX(RasterPos4fv)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos4i)
	.type	GL_PREFIX(RasterPos4i), @function
GL_PREFIX(RasterPos4i):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	656(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	656(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	656(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	656(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos4i), .-GL_PREFIX(RasterPos4i)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos4iv)
	.type	GL_PREFIX(RasterPos4iv), @function
GL_PREFIX(RasterPos4iv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	664(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	664(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	664(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	664(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos4iv), .-GL_PREFIX(RasterPos4iv)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos4s)
	.type	GL_PREFIX(RasterPos4s), @function
GL_PREFIX(RasterPos4s):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	672(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	672(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	672(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	672(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos4s), .-GL_PREFIX(RasterPos4s)

	.p2align	4,,15
	.globl	GL_PREFIX(RasterPos4sv)
	.type	GL_PREFIX(RasterPos4sv), @function
GL_PREFIX(RasterPos4sv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	680(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	680(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	680(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	680(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RasterPos4sv), .-GL_PREFIX(RasterPos4sv)

	.p2align	4,,15
	.globl	GL_PREFIX(Rectd)
	.type	GL_PREFIX(Rectd), @function
GL_PREFIX(Rectd):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	688(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	688(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	688(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	688(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Rectd), .-GL_PREFIX(Rectd)

	.p2align	4,,15
	.globl	GL_PREFIX(Rectdv)
	.type	GL_PREFIX(Rectdv), @function
GL_PREFIX(Rectdv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	696(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	696(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	696(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	696(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Rectdv), .-GL_PREFIX(Rectdv)

	.p2align	4,,15
	.globl	GL_PREFIX(Rectf)
	.type	GL_PREFIX(Rectf), @function
GL_PREFIX(Rectf):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	704(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	704(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	704(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	704(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Rectf), .-GL_PREFIX(Rectf)

	.p2align	4,,15
	.globl	GL_PREFIX(Rectfv)
	.type	GL_PREFIX(Rectfv), @function
GL_PREFIX(Rectfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	712(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	712(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	712(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	712(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Rectfv), .-GL_PREFIX(Rectfv)

	.p2align	4,,15
	.globl	GL_PREFIX(Recti)
	.type	GL_PREFIX(Recti), @function
GL_PREFIX(Recti):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	720(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	720(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	720(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	720(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Recti), .-GL_PREFIX(Recti)

	.p2align	4,,15
	.globl	GL_PREFIX(Rectiv)
	.type	GL_PREFIX(Rectiv), @function
GL_PREFIX(Rectiv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	728(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	728(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	728(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	728(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Rectiv), .-GL_PREFIX(Rectiv)

	.p2align	4,,15
	.globl	GL_PREFIX(Rects)
	.type	GL_PREFIX(Rects), @function
GL_PREFIX(Rects):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	736(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	736(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	736(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	736(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Rects), .-GL_PREFIX(Rects)

	.p2align	4,,15
	.globl	GL_PREFIX(Rectsv)
	.type	GL_PREFIX(Rectsv), @function
GL_PREFIX(Rectsv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	744(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	744(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	744(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	744(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Rectsv), .-GL_PREFIX(Rectsv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord1d)
	.type	GL_PREFIX(TexCoord1d), @function
GL_PREFIX(TexCoord1d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	752(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	752(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	752(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	752(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord1d), .-GL_PREFIX(TexCoord1d)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord1dv)
	.type	GL_PREFIX(TexCoord1dv), @function
GL_PREFIX(TexCoord1dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	760(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	760(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	760(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	760(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord1dv), .-GL_PREFIX(TexCoord1dv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord1f)
	.type	GL_PREFIX(TexCoord1f), @function
GL_PREFIX(TexCoord1f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	768(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	768(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	768(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	768(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord1f), .-GL_PREFIX(TexCoord1f)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord1fv)
	.type	GL_PREFIX(TexCoord1fv), @function
GL_PREFIX(TexCoord1fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	776(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	776(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	776(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	776(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord1fv), .-GL_PREFIX(TexCoord1fv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord1i)
	.type	GL_PREFIX(TexCoord1i), @function
GL_PREFIX(TexCoord1i):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	784(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	784(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	784(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	784(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord1i), .-GL_PREFIX(TexCoord1i)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord1iv)
	.type	GL_PREFIX(TexCoord1iv), @function
GL_PREFIX(TexCoord1iv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	792(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	792(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	792(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	792(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord1iv), .-GL_PREFIX(TexCoord1iv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord1s)
	.type	GL_PREFIX(TexCoord1s), @function
GL_PREFIX(TexCoord1s):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	800(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	800(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	800(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	800(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord1s), .-GL_PREFIX(TexCoord1s)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord1sv)
	.type	GL_PREFIX(TexCoord1sv), @function
GL_PREFIX(TexCoord1sv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	808(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	808(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	808(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	808(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord1sv), .-GL_PREFIX(TexCoord1sv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord2d)
	.type	GL_PREFIX(TexCoord2d), @function
GL_PREFIX(TexCoord2d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	816(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	816(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	816(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	816(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord2d), .-GL_PREFIX(TexCoord2d)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord2dv)
	.type	GL_PREFIX(TexCoord2dv), @function
GL_PREFIX(TexCoord2dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	824(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	824(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	824(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	824(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord2dv), .-GL_PREFIX(TexCoord2dv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord2f)
	.type	GL_PREFIX(TexCoord2f), @function
GL_PREFIX(TexCoord2f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	832(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	832(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	832(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	832(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord2f), .-GL_PREFIX(TexCoord2f)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord2fv)
	.type	GL_PREFIX(TexCoord2fv), @function
GL_PREFIX(TexCoord2fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	840(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	840(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	840(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	840(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord2fv), .-GL_PREFIX(TexCoord2fv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord2i)
	.type	GL_PREFIX(TexCoord2i), @function
GL_PREFIX(TexCoord2i):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	848(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	848(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	848(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	848(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord2i), .-GL_PREFIX(TexCoord2i)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord2iv)
	.type	GL_PREFIX(TexCoord2iv), @function
GL_PREFIX(TexCoord2iv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	856(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	856(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	856(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	856(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord2iv), .-GL_PREFIX(TexCoord2iv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord2s)
	.type	GL_PREFIX(TexCoord2s), @function
GL_PREFIX(TexCoord2s):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	864(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	864(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	864(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	864(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord2s), .-GL_PREFIX(TexCoord2s)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord2sv)
	.type	GL_PREFIX(TexCoord2sv), @function
GL_PREFIX(TexCoord2sv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	872(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	872(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	872(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	872(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord2sv), .-GL_PREFIX(TexCoord2sv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord3d)
	.type	GL_PREFIX(TexCoord3d), @function
GL_PREFIX(TexCoord3d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	880(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	880(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	880(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	880(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord3d), .-GL_PREFIX(TexCoord3d)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord3dv)
	.type	GL_PREFIX(TexCoord3dv), @function
GL_PREFIX(TexCoord3dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	888(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	888(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	888(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	888(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord3dv), .-GL_PREFIX(TexCoord3dv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord3f)
	.type	GL_PREFIX(TexCoord3f), @function
GL_PREFIX(TexCoord3f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	896(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	896(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	896(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	896(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord3f), .-GL_PREFIX(TexCoord3f)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord3fv)
	.type	GL_PREFIX(TexCoord3fv), @function
GL_PREFIX(TexCoord3fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	904(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	904(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	904(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	904(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord3fv), .-GL_PREFIX(TexCoord3fv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord3i)
	.type	GL_PREFIX(TexCoord3i), @function
GL_PREFIX(TexCoord3i):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	912(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	912(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	912(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	912(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord3i), .-GL_PREFIX(TexCoord3i)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord3iv)
	.type	GL_PREFIX(TexCoord3iv), @function
GL_PREFIX(TexCoord3iv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	920(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	920(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	920(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	920(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord3iv), .-GL_PREFIX(TexCoord3iv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord3s)
	.type	GL_PREFIX(TexCoord3s), @function
GL_PREFIX(TexCoord3s):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	928(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	928(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	928(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	928(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord3s), .-GL_PREFIX(TexCoord3s)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord3sv)
	.type	GL_PREFIX(TexCoord3sv), @function
GL_PREFIX(TexCoord3sv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	936(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	936(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	936(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	936(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord3sv), .-GL_PREFIX(TexCoord3sv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord4d)
	.type	GL_PREFIX(TexCoord4d), @function
GL_PREFIX(TexCoord4d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	944(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	944(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	944(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	944(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord4d), .-GL_PREFIX(TexCoord4d)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord4dv)
	.type	GL_PREFIX(TexCoord4dv), @function
GL_PREFIX(TexCoord4dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	952(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	952(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	952(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	952(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord4dv), .-GL_PREFIX(TexCoord4dv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord4f)
	.type	GL_PREFIX(TexCoord4f), @function
GL_PREFIX(TexCoord4f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	960(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	960(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	960(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	960(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord4f), .-GL_PREFIX(TexCoord4f)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord4fv)
	.type	GL_PREFIX(TexCoord4fv), @function
GL_PREFIX(TexCoord4fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	968(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	968(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	968(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	968(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord4fv), .-GL_PREFIX(TexCoord4fv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord4i)
	.type	GL_PREFIX(TexCoord4i), @function
GL_PREFIX(TexCoord4i):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	976(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	976(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	976(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	976(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord4i), .-GL_PREFIX(TexCoord4i)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord4iv)
	.type	GL_PREFIX(TexCoord4iv), @function
GL_PREFIX(TexCoord4iv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	984(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	984(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	984(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	984(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord4iv), .-GL_PREFIX(TexCoord4iv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord4s)
	.type	GL_PREFIX(TexCoord4s), @function
GL_PREFIX(TexCoord4s):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	992(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	992(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	992(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	992(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord4s), .-GL_PREFIX(TexCoord4s)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoord4sv)
	.type	GL_PREFIX(TexCoord4sv), @function
GL_PREFIX(TexCoord4sv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1000(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1000(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1000(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1000(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoord4sv), .-GL_PREFIX(TexCoord4sv)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex2d)
	.type	GL_PREFIX(Vertex2d), @function
GL_PREFIX(Vertex2d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1008(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	1008(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1008(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	1008(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex2d), .-GL_PREFIX(Vertex2d)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex2dv)
	.type	GL_PREFIX(Vertex2dv), @function
GL_PREFIX(Vertex2dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1016(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1016(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1016(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1016(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex2dv), .-GL_PREFIX(Vertex2dv)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex2f)
	.type	GL_PREFIX(Vertex2f), @function
GL_PREFIX(Vertex2f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1024(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	1024(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1024(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	1024(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex2f), .-GL_PREFIX(Vertex2f)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex2fv)
	.type	GL_PREFIX(Vertex2fv), @function
GL_PREFIX(Vertex2fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1032(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1032(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1032(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1032(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex2fv), .-GL_PREFIX(Vertex2fv)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex2i)
	.type	GL_PREFIX(Vertex2i), @function
GL_PREFIX(Vertex2i):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1040(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1040(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1040(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1040(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex2i), .-GL_PREFIX(Vertex2i)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex2iv)
	.type	GL_PREFIX(Vertex2iv), @function
GL_PREFIX(Vertex2iv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1048(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1048(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1048(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1048(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex2iv), .-GL_PREFIX(Vertex2iv)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex2s)
	.type	GL_PREFIX(Vertex2s), @function
GL_PREFIX(Vertex2s):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1056(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1056(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1056(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1056(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex2s), .-GL_PREFIX(Vertex2s)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex2sv)
	.type	GL_PREFIX(Vertex2sv), @function
GL_PREFIX(Vertex2sv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1064(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1064(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1064(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1064(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex2sv), .-GL_PREFIX(Vertex2sv)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex3d)
	.type	GL_PREFIX(Vertex3d), @function
GL_PREFIX(Vertex3d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1072(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	1072(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1072(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	1072(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex3d), .-GL_PREFIX(Vertex3d)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex3dv)
	.type	GL_PREFIX(Vertex3dv), @function
GL_PREFIX(Vertex3dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1080(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1080(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1080(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1080(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex3dv), .-GL_PREFIX(Vertex3dv)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex3f)
	.type	GL_PREFIX(Vertex3f), @function
GL_PREFIX(Vertex3f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1088(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	1088(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1088(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	1088(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex3f), .-GL_PREFIX(Vertex3f)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex3fv)
	.type	GL_PREFIX(Vertex3fv), @function
GL_PREFIX(Vertex3fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1096(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1096(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1096(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1096(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex3fv), .-GL_PREFIX(Vertex3fv)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex3i)
	.type	GL_PREFIX(Vertex3i), @function
GL_PREFIX(Vertex3i):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1104(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1104(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1104(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1104(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex3i), .-GL_PREFIX(Vertex3i)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex3iv)
	.type	GL_PREFIX(Vertex3iv), @function
GL_PREFIX(Vertex3iv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1112(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1112(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1112(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1112(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex3iv), .-GL_PREFIX(Vertex3iv)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex3s)
	.type	GL_PREFIX(Vertex3s), @function
GL_PREFIX(Vertex3s):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1120(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1120(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1120(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1120(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex3s), .-GL_PREFIX(Vertex3s)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex3sv)
	.type	GL_PREFIX(Vertex3sv), @function
GL_PREFIX(Vertex3sv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1128(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1128(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1128(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1128(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex3sv), .-GL_PREFIX(Vertex3sv)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex4d)
	.type	GL_PREFIX(Vertex4d), @function
GL_PREFIX(Vertex4d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1136(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	1136(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1136(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	1136(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex4d), .-GL_PREFIX(Vertex4d)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex4dv)
	.type	GL_PREFIX(Vertex4dv), @function
GL_PREFIX(Vertex4dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1144(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1144(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1144(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1144(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex4dv), .-GL_PREFIX(Vertex4dv)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex4f)
	.type	GL_PREFIX(Vertex4f), @function
GL_PREFIX(Vertex4f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1152(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	1152(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1152(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	1152(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex4f), .-GL_PREFIX(Vertex4f)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex4fv)
	.type	GL_PREFIX(Vertex4fv), @function
GL_PREFIX(Vertex4fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1160(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1160(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1160(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1160(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex4fv), .-GL_PREFIX(Vertex4fv)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex4i)
	.type	GL_PREFIX(Vertex4i), @function
GL_PREFIX(Vertex4i):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1168(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1168(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1168(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1168(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex4i), .-GL_PREFIX(Vertex4i)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex4iv)
	.type	GL_PREFIX(Vertex4iv), @function
GL_PREFIX(Vertex4iv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1176(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1176(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1176(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1176(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex4iv), .-GL_PREFIX(Vertex4iv)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex4s)
	.type	GL_PREFIX(Vertex4s), @function
GL_PREFIX(Vertex4s):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1184(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1184(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1184(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1184(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex4s), .-GL_PREFIX(Vertex4s)

	.p2align	4,,15
	.globl	GL_PREFIX(Vertex4sv)
	.type	GL_PREFIX(Vertex4sv), @function
GL_PREFIX(Vertex4sv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1192(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1192(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1192(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1192(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Vertex4sv), .-GL_PREFIX(Vertex4sv)

	.p2align	4,,15
	.globl	GL_PREFIX(ClipPlane)
	.type	GL_PREFIX(ClipPlane), @function
GL_PREFIX(ClipPlane):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1200(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1200(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1200(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1200(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ClipPlane), .-GL_PREFIX(ClipPlane)

	.p2align	4,,15
	.globl	GL_PREFIX(ColorMaterial)
	.type	GL_PREFIX(ColorMaterial), @function
GL_PREFIX(ColorMaterial):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1208(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1208(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1208(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1208(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ColorMaterial), .-GL_PREFIX(ColorMaterial)

	.p2align	4,,15
	.globl	GL_PREFIX(CullFace)
	.type	GL_PREFIX(CullFace), @function
GL_PREFIX(CullFace):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1216(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1216(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1216(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1216(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CullFace), .-GL_PREFIX(CullFace)

	.p2align	4,,15
	.globl	GL_PREFIX(Fogf)
	.type	GL_PREFIX(Fogf), @function
GL_PREFIX(Fogf):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1224(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1224(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1224(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1224(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Fogf), .-GL_PREFIX(Fogf)

	.p2align	4,,15
	.globl	GL_PREFIX(Fogfv)
	.type	GL_PREFIX(Fogfv), @function
GL_PREFIX(Fogfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1232(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1232(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1232(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1232(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Fogfv), .-GL_PREFIX(Fogfv)

	.p2align	4,,15
	.globl	GL_PREFIX(Fogi)
	.type	GL_PREFIX(Fogi), @function
GL_PREFIX(Fogi):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1240(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1240(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1240(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1240(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Fogi), .-GL_PREFIX(Fogi)

	.p2align	4,,15
	.globl	GL_PREFIX(Fogiv)
	.type	GL_PREFIX(Fogiv), @function
GL_PREFIX(Fogiv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1248(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1248(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1248(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1248(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Fogiv), .-GL_PREFIX(Fogiv)

	.p2align	4,,15
	.globl	GL_PREFIX(FrontFace)
	.type	GL_PREFIX(FrontFace), @function
GL_PREFIX(FrontFace):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1256(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1256(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1256(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1256(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(FrontFace), .-GL_PREFIX(FrontFace)

	.p2align	4,,15
	.globl	GL_PREFIX(Hint)
	.type	GL_PREFIX(Hint), @function
GL_PREFIX(Hint):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1264(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1264(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1264(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1264(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Hint), .-GL_PREFIX(Hint)

	.p2align	4,,15
	.globl	GL_PREFIX(Lightf)
	.type	GL_PREFIX(Lightf), @function
GL_PREFIX(Lightf):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1272(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1272(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1272(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1272(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Lightf), .-GL_PREFIX(Lightf)

	.p2align	4,,15
	.globl	GL_PREFIX(Lightfv)
	.type	GL_PREFIX(Lightfv), @function
GL_PREFIX(Lightfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1280(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1280(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1280(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1280(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Lightfv), .-GL_PREFIX(Lightfv)

	.p2align	4,,15
	.globl	GL_PREFIX(Lighti)
	.type	GL_PREFIX(Lighti), @function
GL_PREFIX(Lighti):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1288(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1288(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1288(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1288(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Lighti), .-GL_PREFIX(Lighti)

	.p2align	4,,15
	.globl	GL_PREFIX(Lightiv)
	.type	GL_PREFIX(Lightiv), @function
GL_PREFIX(Lightiv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1296(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1296(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1296(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1296(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Lightiv), .-GL_PREFIX(Lightiv)

	.p2align	4,,15
	.globl	GL_PREFIX(LightModelf)
	.type	GL_PREFIX(LightModelf), @function
GL_PREFIX(LightModelf):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1304(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1304(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1304(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1304(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LightModelf), .-GL_PREFIX(LightModelf)

	.p2align	4,,15
	.globl	GL_PREFIX(LightModelfv)
	.type	GL_PREFIX(LightModelfv), @function
GL_PREFIX(LightModelfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1312(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1312(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1312(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1312(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LightModelfv), .-GL_PREFIX(LightModelfv)

	.p2align	4,,15
	.globl	GL_PREFIX(LightModeli)
	.type	GL_PREFIX(LightModeli), @function
GL_PREFIX(LightModeli):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1320(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1320(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1320(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1320(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LightModeli), .-GL_PREFIX(LightModeli)

	.p2align	4,,15
	.globl	GL_PREFIX(LightModeliv)
	.type	GL_PREFIX(LightModeliv), @function
GL_PREFIX(LightModeliv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1328(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1328(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1328(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1328(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LightModeliv), .-GL_PREFIX(LightModeliv)

	.p2align	4,,15
	.globl	GL_PREFIX(LineStipple)
	.type	GL_PREFIX(LineStipple), @function
GL_PREFIX(LineStipple):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1336(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1336(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1336(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1336(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LineStipple), .-GL_PREFIX(LineStipple)

	.p2align	4,,15
	.globl	GL_PREFIX(LineWidth)
	.type	GL_PREFIX(LineWidth), @function
GL_PREFIX(LineWidth):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1344(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	1344(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1344(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	1344(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LineWidth), .-GL_PREFIX(LineWidth)

	.p2align	4,,15
	.globl	GL_PREFIX(Materialf)
	.type	GL_PREFIX(Materialf), @function
GL_PREFIX(Materialf):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1352(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1352(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1352(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1352(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Materialf), .-GL_PREFIX(Materialf)

	.p2align	4,,15
	.globl	GL_PREFIX(Materialfv)
	.type	GL_PREFIX(Materialfv), @function
GL_PREFIX(Materialfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1360(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1360(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1360(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1360(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Materialfv), .-GL_PREFIX(Materialfv)

	.p2align	4,,15
	.globl	GL_PREFIX(Materiali)
	.type	GL_PREFIX(Materiali), @function
GL_PREFIX(Materiali):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1368(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1368(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1368(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1368(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Materiali), .-GL_PREFIX(Materiali)

	.p2align	4,,15
	.globl	GL_PREFIX(Materialiv)
	.type	GL_PREFIX(Materialiv), @function
GL_PREFIX(Materialiv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1376(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1376(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1376(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1376(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Materialiv), .-GL_PREFIX(Materialiv)

	.p2align	4,,15
	.globl	GL_PREFIX(PointSize)
	.type	GL_PREFIX(PointSize), @function
GL_PREFIX(PointSize):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1384(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	1384(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1384(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	1384(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PointSize), .-GL_PREFIX(PointSize)

	.p2align	4,,15
	.globl	GL_PREFIX(PolygonMode)
	.type	GL_PREFIX(PolygonMode), @function
GL_PREFIX(PolygonMode):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1392(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1392(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1392(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1392(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PolygonMode), .-GL_PREFIX(PolygonMode)

	.p2align	4,,15
	.globl	GL_PREFIX(PolygonStipple)
	.type	GL_PREFIX(PolygonStipple), @function
GL_PREFIX(PolygonStipple):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1400(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1400(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1400(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1400(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PolygonStipple), .-GL_PREFIX(PolygonStipple)

	.p2align	4,,15
	.globl	GL_PREFIX(Scissor)
	.type	GL_PREFIX(Scissor), @function
GL_PREFIX(Scissor):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1408(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1408(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1408(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1408(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Scissor), .-GL_PREFIX(Scissor)

	.p2align	4,,15
	.globl	GL_PREFIX(ShadeModel)
	.type	GL_PREFIX(ShadeModel), @function
GL_PREFIX(ShadeModel):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1416(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1416(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1416(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1416(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ShadeModel), .-GL_PREFIX(ShadeModel)

	.p2align	4,,15
	.globl	GL_PREFIX(TexParameterf)
	.type	GL_PREFIX(TexParameterf), @function
GL_PREFIX(TexParameterf):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1424(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1424(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1424(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1424(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexParameterf), .-GL_PREFIX(TexParameterf)

	.p2align	4,,15
	.globl	GL_PREFIX(TexParameterfv)
	.type	GL_PREFIX(TexParameterfv), @function
GL_PREFIX(TexParameterfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1432(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1432(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1432(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1432(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexParameterfv), .-GL_PREFIX(TexParameterfv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexParameteri)
	.type	GL_PREFIX(TexParameteri), @function
GL_PREFIX(TexParameteri):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1440(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1440(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1440(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1440(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexParameteri), .-GL_PREFIX(TexParameteri)

	.p2align	4,,15
	.globl	GL_PREFIX(TexParameteriv)
	.type	GL_PREFIX(TexParameteriv), @function
GL_PREFIX(TexParameteriv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1448(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1448(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1448(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1448(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexParameteriv), .-GL_PREFIX(TexParameteriv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexImage1D)
	.type	GL_PREFIX(TexImage1D), @function
GL_PREFIX(TexImage1D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1456(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1456(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1456(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1456(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexImage1D), .-GL_PREFIX(TexImage1D)

	.p2align	4,,15
	.globl	GL_PREFIX(TexImage2D)
	.type	GL_PREFIX(TexImage2D), @function
GL_PREFIX(TexImage2D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1464(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1464(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1464(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1464(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexImage2D), .-GL_PREFIX(TexImage2D)

	.p2align	4,,15
	.globl	GL_PREFIX(TexEnvf)
	.type	GL_PREFIX(TexEnvf), @function
GL_PREFIX(TexEnvf):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1472(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1472(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1472(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1472(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexEnvf), .-GL_PREFIX(TexEnvf)

	.p2align	4,,15
	.globl	GL_PREFIX(TexEnvfv)
	.type	GL_PREFIX(TexEnvfv), @function
GL_PREFIX(TexEnvfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1480(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1480(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1480(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1480(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexEnvfv), .-GL_PREFIX(TexEnvfv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexEnvi)
	.type	GL_PREFIX(TexEnvi), @function
GL_PREFIX(TexEnvi):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1488(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1488(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1488(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1488(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexEnvi), .-GL_PREFIX(TexEnvi)

	.p2align	4,,15
	.globl	GL_PREFIX(TexEnviv)
	.type	GL_PREFIX(TexEnviv), @function
GL_PREFIX(TexEnviv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1496(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1496(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1496(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1496(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexEnviv), .-GL_PREFIX(TexEnviv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexGend)
	.type	GL_PREFIX(TexGend), @function
GL_PREFIX(TexGend):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1504(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1504(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1504(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1504(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexGend), .-GL_PREFIX(TexGend)

	.p2align	4,,15
	.globl	GL_PREFIX(TexGendv)
	.type	GL_PREFIX(TexGendv), @function
GL_PREFIX(TexGendv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1512(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1512(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1512(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1512(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexGendv), .-GL_PREFIX(TexGendv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexGenf)
	.type	GL_PREFIX(TexGenf), @function
GL_PREFIX(TexGenf):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1520(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1520(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1520(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1520(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexGenf), .-GL_PREFIX(TexGenf)

	.p2align	4,,15
	.globl	GL_PREFIX(TexGenfv)
	.type	GL_PREFIX(TexGenfv), @function
GL_PREFIX(TexGenfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1528(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1528(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1528(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1528(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexGenfv), .-GL_PREFIX(TexGenfv)

	.p2align	4,,15
	.globl	GL_PREFIX(TexGeni)
	.type	GL_PREFIX(TexGeni), @function
GL_PREFIX(TexGeni):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1536(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1536(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1536(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1536(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexGeni), .-GL_PREFIX(TexGeni)

	.p2align	4,,15
	.globl	GL_PREFIX(TexGeniv)
	.type	GL_PREFIX(TexGeniv), @function
GL_PREFIX(TexGeniv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1544(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1544(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1544(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1544(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexGeniv), .-GL_PREFIX(TexGeniv)

	.p2align	4,,15
	.globl	GL_PREFIX(FeedbackBuffer)
	.type	GL_PREFIX(FeedbackBuffer), @function
GL_PREFIX(FeedbackBuffer):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1552(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1552(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1552(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1552(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(FeedbackBuffer), .-GL_PREFIX(FeedbackBuffer)

	.p2align	4,,15
	.globl	GL_PREFIX(SelectBuffer)
	.type	GL_PREFIX(SelectBuffer), @function
GL_PREFIX(SelectBuffer):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1560(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1560(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1560(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1560(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SelectBuffer), .-GL_PREFIX(SelectBuffer)

	.p2align	4,,15
	.globl	GL_PREFIX(RenderMode)
	.type	GL_PREFIX(RenderMode), @function
GL_PREFIX(RenderMode):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1568(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1568(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1568(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1568(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RenderMode), .-GL_PREFIX(RenderMode)

	.p2align	4,,15
	.globl	GL_PREFIX(InitNames)
	.type	GL_PREFIX(InitNames), @function
GL_PREFIX(InitNames):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1576(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	1576(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1576(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	1576(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(InitNames), .-GL_PREFIX(InitNames)

	.p2align	4,,15
	.globl	GL_PREFIX(LoadName)
	.type	GL_PREFIX(LoadName), @function
GL_PREFIX(LoadName):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1584(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1584(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1584(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1584(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LoadName), .-GL_PREFIX(LoadName)

	.p2align	4,,15
	.globl	GL_PREFIX(PassThrough)
	.type	GL_PREFIX(PassThrough), @function
GL_PREFIX(PassThrough):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1592(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	1592(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1592(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	1592(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PassThrough), .-GL_PREFIX(PassThrough)

	.p2align	4,,15
	.globl	GL_PREFIX(PopName)
	.type	GL_PREFIX(PopName), @function
GL_PREFIX(PopName):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1600(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	1600(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1600(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	1600(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PopName), .-GL_PREFIX(PopName)

	.p2align	4,,15
	.globl	GL_PREFIX(PushName)
	.type	GL_PREFIX(PushName), @function
GL_PREFIX(PushName):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1608(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1608(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1608(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1608(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PushName), .-GL_PREFIX(PushName)

	.p2align	4,,15
	.globl	GL_PREFIX(DrawBuffer)
	.type	GL_PREFIX(DrawBuffer), @function
GL_PREFIX(DrawBuffer):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1616(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1616(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1616(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1616(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DrawBuffer), .-GL_PREFIX(DrawBuffer)

	.p2align	4,,15
	.globl	GL_PREFIX(Clear)
	.type	GL_PREFIX(Clear), @function
GL_PREFIX(Clear):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1624(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1624(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1624(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1624(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Clear), .-GL_PREFIX(Clear)

	.p2align	4,,15
	.globl	GL_PREFIX(ClearAccum)
	.type	GL_PREFIX(ClearAccum), @function
GL_PREFIX(ClearAccum):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1632(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	1632(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1632(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	1632(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ClearAccum), .-GL_PREFIX(ClearAccum)

	.p2align	4,,15
	.globl	GL_PREFIX(ClearIndex)
	.type	GL_PREFIX(ClearIndex), @function
GL_PREFIX(ClearIndex):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1640(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	1640(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1640(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	1640(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ClearIndex), .-GL_PREFIX(ClearIndex)

	.p2align	4,,15
	.globl	GL_PREFIX(ClearColor)
	.type	GL_PREFIX(ClearColor), @function
GL_PREFIX(ClearColor):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1648(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1648(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1648(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1648(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ClearColor), .-GL_PREFIX(ClearColor)

	.p2align	4,,15
	.globl	GL_PREFIX(ClearStencil)
	.type	GL_PREFIX(ClearStencil), @function
GL_PREFIX(ClearStencil):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1656(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1656(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1656(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1656(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ClearStencil), .-GL_PREFIX(ClearStencil)

	.p2align	4,,15
	.globl	GL_PREFIX(ClearDepth)
	.type	GL_PREFIX(ClearDepth), @function
GL_PREFIX(ClearDepth):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1664(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1664(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1664(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1664(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ClearDepth), .-GL_PREFIX(ClearDepth)

	.p2align	4,,15
	.globl	GL_PREFIX(StencilMask)
	.type	GL_PREFIX(StencilMask), @function
GL_PREFIX(StencilMask):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1672(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1672(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1672(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1672(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(StencilMask), .-GL_PREFIX(StencilMask)

	.p2align	4,,15
	.globl	GL_PREFIX(ColorMask)
	.type	GL_PREFIX(ColorMask), @function
GL_PREFIX(ColorMask):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1680(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1680(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1680(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1680(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ColorMask), .-GL_PREFIX(ColorMask)

	.p2align	4,,15
	.globl	GL_PREFIX(DepthMask)
	.type	GL_PREFIX(DepthMask), @function
GL_PREFIX(DepthMask):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1688(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1688(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1688(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1688(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DepthMask), .-GL_PREFIX(DepthMask)

	.p2align	4,,15
	.globl	GL_PREFIX(IndexMask)
	.type	GL_PREFIX(IndexMask), @function
GL_PREFIX(IndexMask):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1696(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1696(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1696(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1696(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(IndexMask), .-GL_PREFIX(IndexMask)

	.p2align	4,,15
	.globl	GL_PREFIX(Accum)
	.type	GL_PREFIX(Accum), @function
GL_PREFIX(Accum):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1704(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1704(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1704(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1704(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Accum), .-GL_PREFIX(Accum)

	.p2align	4,,15
	.globl	GL_PREFIX(Disable)
	.type	GL_PREFIX(Disable), @function
GL_PREFIX(Disable):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1712(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1712(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1712(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1712(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Disable), .-GL_PREFIX(Disable)

	.p2align	4,,15
	.globl	GL_PREFIX(Enable)
	.type	GL_PREFIX(Enable), @function
GL_PREFIX(Enable):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1720(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1720(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1720(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1720(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Enable), .-GL_PREFIX(Enable)

	.p2align	4,,15
	.globl	GL_PREFIX(Finish)
	.type	GL_PREFIX(Finish), @function
GL_PREFIX(Finish):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1728(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	1728(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1728(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	1728(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Finish), .-GL_PREFIX(Finish)

	.p2align	4,,15
	.globl	GL_PREFIX(Flush)
	.type	GL_PREFIX(Flush), @function
GL_PREFIX(Flush):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1736(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	1736(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1736(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	1736(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Flush), .-GL_PREFIX(Flush)

	.p2align	4,,15
	.globl	GL_PREFIX(PopAttrib)
	.type	GL_PREFIX(PopAttrib), @function
GL_PREFIX(PopAttrib):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1744(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	1744(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1744(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	1744(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PopAttrib), .-GL_PREFIX(PopAttrib)

	.p2align	4,,15
	.globl	GL_PREFIX(PushAttrib)
	.type	GL_PREFIX(PushAttrib), @function
GL_PREFIX(PushAttrib):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1752(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1752(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1752(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1752(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PushAttrib), .-GL_PREFIX(PushAttrib)

	.p2align	4,,15
	.globl	GL_PREFIX(Map1d)
	.type	GL_PREFIX(Map1d), @function
GL_PREFIX(Map1d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1760(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%rsi, 24(%rsp)
	movq	%rdx, 32(%rsp)
	movq	%rcx, 40(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	40(%rsp), %rcx
	movq	32(%rsp), %rdx
	movq	24(%rsp), %rsi
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	1760(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1760(%rax), %r11
	jmp	*%r11
1:
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%rsi, 24(%rsp)
	movq	%rdx, 32(%rsp)
	movq	%rcx, 40(%rsp)
	call	_glapi_get_dispatch
	movq	40(%rsp), %rcx
	movq	32(%rsp), %rdx
	movq	24(%rsp), %rsi
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	1760(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Map1d), .-GL_PREFIX(Map1d)

	.p2align	4,,15
	.globl	GL_PREFIX(Map1f)
	.type	GL_PREFIX(Map1f), @function
GL_PREFIX(Map1f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1768(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%rsi, 24(%rsp)
	movq	%rdx, 32(%rsp)
	movq	%rcx, 40(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	40(%rsp), %rcx
	movq	32(%rsp), %rdx
	movq	24(%rsp), %rsi
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	1768(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1768(%rax), %r11
	jmp	*%r11
1:
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%rsi, 24(%rsp)
	movq	%rdx, 32(%rsp)
	movq	%rcx, 40(%rsp)
	call	_glapi_get_dispatch
	movq	40(%rsp), %rcx
	movq	32(%rsp), %rdx
	movq	24(%rsp), %rsi
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	1768(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Map1f), .-GL_PREFIX(Map1f)

	.p2align	4,,15
	.globl	GL_PREFIX(Map2d)
	.type	GL_PREFIX(Map2d), @function
GL_PREFIX(Map2d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1776(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$88, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%rsi, 24(%rsp)
	movq	%rdx, 32(%rsp)
	movq	%xmm2, 40(%rsp)
	movq	%xmm3, 48(%rsp)
	movq	%rcx, 56(%rsp)
	movq	%r8, 64(%rsp)
	movq	%r9, 72(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	72(%rsp), %r9
	movq	64(%rsp), %r8
	movq	56(%rsp), %rcx
	movq	48(%rsp), %xmm3
	movq	40(%rsp), %xmm2
	movq	32(%rsp), %rdx
	movq	24(%rsp), %rsi
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$88, %rsp
	movq	1776(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1776(%rax), %r11
	jmp	*%r11
1:
	subq	$88, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%rsi, 24(%rsp)
	movq	%rdx, 32(%rsp)
	movq	%xmm2, 40(%rsp)
	movq	%xmm3, 48(%rsp)
	movq	%rcx, 56(%rsp)
	movq	%r8, 64(%rsp)
	movq	%r9, 72(%rsp)
	call	_glapi_get_dispatch
	movq	72(%rsp), %r9
	movq	64(%rsp), %r8
	movq	56(%rsp), %rcx
	movq	48(%rsp), %xmm3
	movq	40(%rsp), %xmm2
	movq	32(%rsp), %rdx
	movq	24(%rsp), %rsi
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$88, %rsp
	movq	1776(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Map2d), .-GL_PREFIX(Map2d)

	.p2align	4,,15
	.globl	GL_PREFIX(Map2f)
	.type	GL_PREFIX(Map2f), @function
GL_PREFIX(Map2f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1784(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$88, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%rsi, 24(%rsp)
	movq	%rdx, 32(%rsp)
	movq	%xmm2, 40(%rsp)
	movq	%xmm3, 48(%rsp)
	movq	%rcx, 56(%rsp)
	movq	%r8, 64(%rsp)
	movq	%r9, 72(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	72(%rsp), %r9
	movq	64(%rsp), %r8
	movq	56(%rsp), %rcx
	movq	48(%rsp), %xmm3
	movq	40(%rsp), %xmm2
	movq	32(%rsp), %rdx
	movq	24(%rsp), %rsi
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$88, %rsp
	movq	1784(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1784(%rax), %r11
	jmp	*%r11
1:
	subq	$88, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%rsi, 24(%rsp)
	movq	%rdx, 32(%rsp)
	movq	%xmm2, 40(%rsp)
	movq	%xmm3, 48(%rsp)
	movq	%rcx, 56(%rsp)
	movq	%r8, 64(%rsp)
	movq	%r9, 72(%rsp)
	call	_glapi_get_dispatch
	movq	72(%rsp), %r9
	movq	64(%rsp), %r8
	movq	56(%rsp), %rcx
	movq	48(%rsp), %xmm3
	movq	40(%rsp), %xmm2
	movq	32(%rsp), %rdx
	movq	24(%rsp), %rsi
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$88, %rsp
	movq	1784(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Map2f), .-GL_PREFIX(Map2f)

	.p2align	4,,15
	.globl	GL_PREFIX(MapGrid1d)
	.type	GL_PREFIX(MapGrid1d), @function
GL_PREFIX(MapGrid1d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1792(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1792(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1792(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1792(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MapGrid1d), .-GL_PREFIX(MapGrid1d)

	.p2align	4,,15
	.globl	GL_PREFIX(MapGrid1f)
	.type	GL_PREFIX(MapGrid1f), @function
GL_PREFIX(MapGrid1f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1800(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1800(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1800(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1800(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MapGrid1f), .-GL_PREFIX(MapGrid1f)

	.p2align	4,,15
	.globl	GL_PREFIX(MapGrid2d)
	.type	GL_PREFIX(MapGrid2d), @function
GL_PREFIX(MapGrid2d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1808(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%rsi, 24(%rsp)
	movq	%xmm2, 32(%rsp)
	movq	%xmm3, 40(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	40(%rsp), %xmm3
	movq	32(%rsp), %xmm2
	movq	24(%rsp), %rsi
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	1808(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1808(%rax), %r11
	jmp	*%r11
1:
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%rsi, 24(%rsp)
	movq	%xmm2, 32(%rsp)
	movq	%xmm3, 40(%rsp)
	call	_glapi_get_dispatch
	movq	40(%rsp), %xmm3
	movq	32(%rsp), %xmm2
	movq	24(%rsp), %rsi
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	1808(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MapGrid2d), .-GL_PREFIX(MapGrid2d)

	.p2align	4,,15
	.globl	GL_PREFIX(MapGrid2f)
	.type	GL_PREFIX(MapGrid2f), @function
GL_PREFIX(MapGrid2f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1816(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%rsi, 24(%rsp)
	movq	%xmm2, 32(%rsp)
	movq	%xmm3, 40(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	40(%rsp), %xmm3
	movq	32(%rsp), %xmm2
	movq	24(%rsp), %rsi
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	1816(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1816(%rax), %r11
	jmp	*%r11
1:
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%rsi, 24(%rsp)
	movq	%xmm2, 32(%rsp)
	movq	%xmm3, 40(%rsp)
	call	_glapi_get_dispatch
	movq	40(%rsp), %xmm3
	movq	32(%rsp), %xmm2
	movq	24(%rsp), %rsi
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	1816(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MapGrid2f), .-GL_PREFIX(MapGrid2f)

	.p2align	4,,15
	.globl	GL_PREFIX(EvalCoord1d)
	.type	GL_PREFIX(EvalCoord1d), @function
GL_PREFIX(EvalCoord1d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1824(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	1824(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1824(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	1824(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EvalCoord1d), .-GL_PREFIX(EvalCoord1d)

	.p2align	4,,15
	.globl	GL_PREFIX(EvalCoord1dv)
	.type	GL_PREFIX(EvalCoord1dv), @function
GL_PREFIX(EvalCoord1dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1832(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1832(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1832(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1832(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EvalCoord1dv), .-GL_PREFIX(EvalCoord1dv)

	.p2align	4,,15
	.globl	GL_PREFIX(EvalCoord1f)
	.type	GL_PREFIX(EvalCoord1f), @function
GL_PREFIX(EvalCoord1f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1840(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	1840(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1840(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	1840(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EvalCoord1f), .-GL_PREFIX(EvalCoord1f)

	.p2align	4,,15
	.globl	GL_PREFIX(EvalCoord1fv)
	.type	GL_PREFIX(EvalCoord1fv), @function
GL_PREFIX(EvalCoord1fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1848(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1848(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1848(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1848(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EvalCoord1fv), .-GL_PREFIX(EvalCoord1fv)

	.p2align	4,,15
	.globl	GL_PREFIX(EvalCoord2d)
	.type	GL_PREFIX(EvalCoord2d), @function
GL_PREFIX(EvalCoord2d):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1856(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	1856(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1856(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	1856(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EvalCoord2d), .-GL_PREFIX(EvalCoord2d)

	.p2align	4,,15
	.globl	GL_PREFIX(EvalCoord2dv)
	.type	GL_PREFIX(EvalCoord2dv), @function
GL_PREFIX(EvalCoord2dv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1864(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1864(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1864(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1864(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EvalCoord2dv), .-GL_PREFIX(EvalCoord2dv)

	.p2align	4,,15
	.globl	GL_PREFIX(EvalCoord2f)
	.type	GL_PREFIX(EvalCoord2f), @function
GL_PREFIX(EvalCoord2f):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1872(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	1872(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1872(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	1872(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EvalCoord2f), .-GL_PREFIX(EvalCoord2f)

	.p2align	4,,15
	.globl	GL_PREFIX(EvalCoord2fv)
	.type	GL_PREFIX(EvalCoord2fv), @function
GL_PREFIX(EvalCoord2fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1880(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1880(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1880(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1880(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EvalCoord2fv), .-GL_PREFIX(EvalCoord2fv)

	.p2align	4,,15
	.globl	GL_PREFIX(EvalMesh1)
	.type	GL_PREFIX(EvalMesh1), @function
GL_PREFIX(EvalMesh1):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1888(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1888(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1888(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1888(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EvalMesh1), .-GL_PREFIX(EvalMesh1)

	.p2align	4,,15
	.globl	GL_PREFIX(EvalPoint1)
	.type	GL_PREFIX(EvalPoint1), @function
GL_PREFIX(EvalPoint1):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1896(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1896(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1896(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1896(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EvalPoint1), .-GL_PREFIX(EvalPoint1)

	.p2align	4,,15
	.globl	GL_PREFIX(EvalMesh2)
	.type	GL_PREFIX(EvalMesh2), @function
GL_PREFIX(EvalMesh2):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1904(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1904(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1904(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1904(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EvalMesh2), .-GL_PREFIX(EvalMesh2)

	.p2align	4,,15
	.globl	GL_PREFIX(EvalPoint2)
	.type	GL_PREFIX(EvalPoint2), @function
GL_PREFIX(EvalPoint2):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1912(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1912(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1912(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1912(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EvalPoint2), .-GL_PREFIX(EvalPoint2)

	.p2align	4,,15
	.globl	GL_PREFIX(AlphaFunc)
	.type	GL_PREFIX(AlphaFunc), @function
GL_PREFIX(AlphaFunc):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1920(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1920(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1920(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1920(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(AlphaFunc), .-GL_PREFIX(AlphaFunc)

	.p2align	4,,15
	.globl	GL_PREFIX(BlendFunc)
	.type	GL_PREFIX(BlendFunc), @function
GL_PREFIX(BlendFunc):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1928(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1928(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1928(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1928(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BlendFunc), .-GL_PREFIX(BlendFunc)

	.p2align	4,,15
	.globl	GL_PREFIX(LogicOp)
	.type	GL_PREFIX(LogicOp), @function
GL_PREFIX(LogicOp):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1936(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1936(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1936(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1936(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LogicOp), .-GL_PREFIX(LogicOp)

	.p2align	4,,15
	.globl	GL_PREFIX(StencilFunc)
	.type	GL_PREFIX(StencilFunc), @function
GL_PREFIX(StencilFunc):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1944(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1944(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1944(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1944(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(StencilFunc), .-GL_PREFIX(StencilFunc)

	.p2align	4,,15
	.globl	GL_PREFIX(StencilOp)
	.type	GL_PREFIX(StencilOp), @function
GL_PREFIX(StencilOp):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1952(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1952(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1952(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	1952(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(StencilOp), .-GL_PREFIX(StencilOp)

	.p2align	4,,15
	.globl	GL_PREFIX(DepthFunc)
	.type	GL_PREFIX(DepthFunc), @function
GL_PREFIX(DepthFunc):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1960(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	1960(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1960(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	1960(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DepthFunc), .-GL_PREFIX(DepthFunc)

	.p2align	4,,15
	.globl	GL_PREFIX(PixelZoom)
	.type	GL_PREFIX(PixelZoom), @function
GL_PREFIX(PixelZoom):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1968(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	1968(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1968(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	1968(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PixelZoom), .-GL_PREFIX(PixelZoom)

	.p2align	4,,15
	.globl	GL_PREFIX(PixelTransferf)
	.type	GL_PREFIX(PixelTransferf), @function
GL_PREFIX(PixelTransferf):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1976(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1976(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1976(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1976(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PixelTransferf), .-GL_PREFIX(PixelTransferf)

	.p2align	4,,15
	.globl	GL_PREFIX(PixelTransferi)
	.type	GL_PREFIX(PixelTransferi), @function
GL_PREFIX(PixelTransferi):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1984(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1984(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1984(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	1984(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PixelTransferi), .-GL_PREFIX(PixelTransferi)

	.p2align	4,,15
	.globl	GL_PREFIX(PixelStoref)
	.type	GL_PREFIX(PixelStoref), @function
GL_PREFIX(PixelStoref):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	1992(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1992(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	1992(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	1992(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PixelStoref), .-GL_PREFIX(PixelStoref)

	.p2align	4,,15
	.globl	GL_PREFIX(PixelStorei)
	.type	GL_PREFIX(PixelStorei), @function
GL_PREFIX(PixelStorei):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2000(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2000(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2000(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2000(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PixelStorei), .-GL_PREFIX(PixelStorei)

	.p2align	4,,15
	.globl	GL_PREFIX(PixelMapfv)
	.type	GL_PREFIX(PixelMapfv), @function
GL_PREFIX(PixelMapfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2008(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2008(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2008(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2008(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PixelMapfv), .-GL_PREFIX(PixelMapfv)

	.p2align	4,,15
	.globl	GL_PREFIX(PixelMapuiv)
	.type	GL_PREFIX(PixelMapuiv), @function
GL_PREFIX(PixelMapuiv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2016(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2016(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2016(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2016(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PixelMapuiv), .-GL_PREFIX(PixelMapuiv)

	.p2align	4,,15
	.globl	GL_PREFIX(PixelMapusv)
	.type	GL_PREFIX(PixelMapusv), @function
GL_PREFIX(PixelMapusv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2024(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2024(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2024(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2024(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PixelMapusv), .-GL_PREFIX(PixelMapusv)

	.p2align	4,,15
	.globl	GL_PREFIX(ReadBuffer)
	.type	GL_PREFIX(ReadBuffer), @function
GL_PREFIX(ReadBuffer):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2032(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2032(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2032(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2032(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ReadBuffer), .-GL_PREFIX(ReadBuffer)

	.p2align	4,,15
	.globl	GL_PREFIX(CopyPixels)
	.type	GL_PREFIX(CopyPixels), @function
GL_PREFIX(CopyPixels):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2040(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2040(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2040(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2040(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CopyPixels), .-GL_PREFIX(CopyPixels)

	.p2align	4,,15
	.globl	GL_PREFIX(ReadPixels)
	.type	GL_PREFIX(ReadPixels), @function
GL_PREFIX(ReadPixels):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2048(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2048(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2048(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2048(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ReadPixels), .-GL_PREFIX(ReadPixels)

	.p2align	4,,15
	.globl	GL_PREFIX(DrawPixels)
	.type	GL_PREFIX(DrawPixels), @function
GL_PREFIX(DrawPixels):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2056(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2056(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2056(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2056(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DrawPixels), .-GL_PREFIX(DrawPixels)

	.p2align	4,,15
	.globl	GL_PREFIX(GetBooleanv)
	.type	GL_PREFIX(GetBooleanv), @function
GL_PREFIX(GetBooleanv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2064(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2064(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2064(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2064(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetBooleanv), .-GL_PREFIX(GetBooleanv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetClipPlane)
	.type	GL_PREFIX(GetClipPlane), @function
GL_PREFIX(GetClipPlane):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2072(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2072(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2072(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2072(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetClipPlane), .-GL_PREFIX(GetClipPlane)

	.p2align	4,,15
	.globl	GL_PREFIX(GetDoublev)
	.type	GL_PREFIX(GetDoublev), @function
GL_PREFIX(GetDoublev):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2080(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2080(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2080(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2080(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetDoublev), .-GL_PREFIX(GetDoublev)

	.p2align	4,,15
	.globl	GL_PREFIX(GetError)
	.type	GL_PREFIX(GetError), @function
GL_PREFIX(GetError):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2088(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	2088(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2088(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	2088(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetError), .-GL_PREFIX(GetError)

	.p2align	4,,15
	.globl	GL_PREFIX(GetFloatv)
	.type	GL_PREFIX(GetFloatv), @function
GL_PREFIX(GetFloatv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2096(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2096(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2096(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2096(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetFloatv), .-GL_PREFIX(GetFloatv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetIntegerv)
	.type	GL_PREFIX(GetIntegerv), @function
GL_PREFIX(GetIntegerv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2104(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2104(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2104(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2104(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetIntegerv), .-GL_PREFIX(GetIntegerv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetLightfv)
	.type	GL_PREFIX(GetLightfv), @function
GL_PREFIX(GetLightfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2112(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2112(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2112(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2112(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetLightfv), .-GL_PREFIX(GetLightfv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetLightiv)
	.type	GL_PREFIX(GetLightiv), @function
GL_PREFIX(GetLightiv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2120(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2120(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2120(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2120(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetLightiv), .-GL_PREFIX(GetLightiv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetMapdv)
	.type	GL_PREFIX(GetMapdv), @function
GL_PREFIX(GetMapdv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2128(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2128(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2128(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2128(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetMapdv), .-GL_PREFIX(GetMapdv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetMapfv)
	.type	GL_PREFIX(GetMapfv), @function
GL_PREFIX(GetMapfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2136(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2136(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2136(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2136(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetMapfv), .-GL_PREFIX(GetMapfv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetMapiv)
	.type	GL_PREFIX(GetMapiv), @function
GL_PREFIX(GetMapiv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2144(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2144(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2144(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2144(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetMapiv), .-GL_PREFIX(GetMapiv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetMaterialfv)
	.type	GL_PREFIX(GetMaterialfv), @function
GL_PREFIX(GetMaterialfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2152(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2152(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2152(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2152(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetMaterialfv), .-GL_PREFIX(GetMaterialfv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetMaterialiv)
	.type	GL_PREFIX(GetMaterialiv), @function
GL_PREFIX(GetMaterialiv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2160(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2160(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2160(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2160(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetMaterialiv), .-GL_PREFIX(GetMaterialiv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetPixelMapfv)
	.type	GL_PREFIX(GetPixelMapfv), @function
GL_PREFIX(GetPixelMapfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2168(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2168(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2168(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2168(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetPixelMapfv), .-GL_PREFIX(GetPixelMapfv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetPixelMapuiv)
	.type	GL_PREFIX(GetPixelMapuiv), @function
GL_PREFIX(GetPixelMapuiv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2176(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2176(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2176(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2176(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetPixelMapuiv), .-GL_PREFIX(GetPixelMapuiv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetPixelMapusv)
	.type	GL_PREFIX(GetPixelMapusv), @function
GL_PREFIX(GetPixelMapusv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2184(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2184(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2184(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2184(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetPixelMapusv), .-GL_PREFIX(GetPixelMapusv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetPolygonStipple)
	.type	GL_PREFIX(GetPolygonStipple), @function
GL_PREFIX(GetPolygonStipple):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2192(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2192(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2192(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2192(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetPolygonStipple), .-GL_PREFIX(GetPolygonStipple)

	.p2align	4,,15
	.globl	GL_PREFIX(GetString)
	.type	GL_PREFIX(GetString), @function
GL_PREFIX(GetString):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2200(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2200(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2200(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2200(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetString), .-GL_PREFIX(GetString)

	.p2align	4,,15
	.globl	GL_PREFIX(GetTexEnvfv)
	.type	GL_PREFIX(GetTexEnvfv), @function
GL_PREFIX(GetTexEnvfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2208(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2208(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2208(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2208(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetTexEnvfv), .-GL_PREFIX(GetTexEnvfv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetTexEnviv)
	.type	GL_PREFIX(GetTexEnviv), @function
GL_PREFIX(GetTexEnviv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2216(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2216(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2216(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2216(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetTexEnviv), .-GL_PREFIX(GetTexEnviv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetTexGendv)
	.type	GL_PREFIX(GetTexGendv), @function
GL_PREFIX(GetTexGendv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2224(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2224(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2224(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2224(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetTexGendv), .-GL_PREFIX(GetTexGendv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetTexGenfv)
	.type	GL_PREFIX(GetTexGenfv), @function
GL_PREFIX(GetTexGenfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2232(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2232(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2232(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2232(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetTexGenfv), .-GL_PREFIX(GetTexGenfv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetTexGeniv)
	.type	GL_PREFIX(GetTexGeniv), @function
GL_PREFIX(GetTexGeniv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2240(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2240(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2240(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2240(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetTexGeniv), .-GL_PREFIX(GetTexGeniv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetTexImage)
	.type	GL_PREFIX(GetTexImage), @function
GL_PREFIX(GetTexImage):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2248(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2248(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2248(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2248(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetTexImage), .-GL_PREFIX(GetTexImage)

	.p2align	4,,15
	.globl	GL_PREFIX(GetTexParameterfv)
	.type	GL_PREFIX(GetTexParameterfv), @function
GL_PREFIX(GetTexParameterfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2256(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2256(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2256(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2256(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetTexParameterfv), .-GL_PREFIX(GetTexParameterfv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetTexParameteriv)
	.type	GL_PREFIX(GetTexParameteriv), @function
GL_PREFIX(GetTexParameteriv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2264(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2264(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2264(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2264(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetTexParameteriv), .-GL_PREFIX(GetTexParameteriv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetTexLevelParameterfv)
	.type	GL_PREFIX(GetTexLevelParameterfv), @function
GL_PREFIX(GetTexLevelParameterfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2272(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2272(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2272(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2272(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetTexLevelParameterfv), .-GL_PREFIX(GetTexLevelParameterfv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetTexLevelParameteriv)
	.type	GL_PREFIX(GetTexLevelParameteriv), @function
GL_PREFIX(GetTexLevelParameteriv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2280(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2280(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2280(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2280(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetTexLevelParameteriv), .-GL_PREFIX(GetTexLevelParameteriv)

	.p2align	4,,15
	.globl	GL_PREFIX(IsEnabled)
	.type	GL_PREFIX(IsEnabled), @function
GL_PREFIX(IsEnabled):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2288(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2288(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2288(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2288(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(IsEnabled), .-GL_PREFIX(IsEnabled)

	.p2align	4,,15
	.globl	GL_PREFIX(IsList)
	.type	GL_PREFIX(IsList), @function
GL_PREFIX(IsList):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2296(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2296(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2296(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2296(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(IsList), .-GL_PREFIX(IsList)

	.p2align	4,,15
	.globl	GL_PREFIX(DepthRange)
	.type	GL_PREFIX(DepthRange), @function
GL_PREFIX(DepthRange):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2304(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2304(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2304(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2304(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DepthRange), .-GL_PREFIX(DepthRange)

	.p2align	4,,15
	.globl	GL_PREFIX(Frustum)
	.type	GL_PREFIX(Frustum), @function
GL_PREFIX(Frustum):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2312(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$56, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	movq	%xmm4, 32(%rsp)
	movq	%xmm5, 40(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	40(%rsp), %xmm5
	movq	32(%rsp), %xmm4
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$56, %rsp
	movq	2312(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2312(%rax), %r11
	jmp	*%r11
1:
	subq	$56, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	movq	%xmm4, 32(%rsp)
	movq	%xmm5, 40(%rsp)
	call	_glapi_get_dispatch
	movq	40(%rsp), %xmm5
	movq	32(%rsp), %xmm4
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$56, %rsp
	movq	2312(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Frustum), .-GL_PREFIX(Frustum)

	.p2align	4,,15
	.globl	GL_PREFIX(LoadIdentity)
	.type	GL_PREFIX(LoadIdentity), @function
GL_PREFIX(LoadIdentity):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2320(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	2320(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2320(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	2320(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LoadIdentity), .-GL_PREFIX(LoadIdentity)

	.p2align	4,,15
	.globl	GL_PREFIX(LoadMatrixf)
	.type	GL_PREFIX(LoadMatrixf), @function
GL_PREFIX(LoadMatrixf):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2328(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2328(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2328(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2328(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LoadMatrixf), .-GL_PREFIX(LoadMatrixf)

	.p2align	4,,15
	.globl	GL_PREFIX(LoadMatrixd)
	.type	GL_PREFIX(LoadMatrixd), @function
GL_PREFIX(LoadMatrixd):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2336(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2336(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2336(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2336(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LoadMatrixd), .-GL_PREFIX(LoadMatrixd)

	.p2align	4,,15
	.globl	GL_PREFIX(MatrixMode)
	.type	GL_PREFIX(MatrixMode), @function
GL_PREFIX(MatrixMode):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2344(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2344(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2344(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2344(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MatrixMode), .-GL_PREFIX(MatrixMode)

	.p2align	4,,15
	.globl	GL_PREFIX(MultMatrixf)
	.type	GL_PREFIX(MultMatrixf), @function
GL_PREFIX(MultMatrixf):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2352(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2352(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2352(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2352(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultMatrixf), .-GL_PREFIX(MultMatrixf)

	.p2align	4,,15
	.globl	GL_PREFIX(MultMatrixd)
	.type	GL_PREFIX(MultMatrixd), @function
GL_PREFIX(MultMatrixd):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2360(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2360(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2360(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2360(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultMatrixd), .-GL_PREFIX(MultMatrixd)

	.p2align	4,,15
	.globl	GL_PREFIX(Ortho)
	.type	GL_PREFIX(Ortho), @function
GL_PREFIX(Ortho):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2368(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$56, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	movq	%xmm4, 32(%rsp)
	movq	%xmm5, 40(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	40(%rsp), %xmm5
	movq	32(%rsp), %xmm4
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$56, %rsp
	movq	2368(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2368(%rax), %r11
	jmp	*%r11
1:
	subq	$56, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	movq	%xmm4, 32(%rsp)
	movq	%xmm5, 40(%rsp)
	call	_glapi_get_dispatch
	movq	40(%rsp), %xmm5
	movq	32(%rsp), %xmm4
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$56, %rsp
	movq	2368(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Ortho), .-GL_PREFIX(Ortho)

	.p2align	4,,15
	.globl	GL_PREFIX(PopMatrix)
	.type	GL_PREFIX(PopMatrix), @function
GL_PREFIX(PopMatrix):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2376(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	2376(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2376(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	2376(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PopMatrix), .-GL_PREFIX(PopMatrix)

	.p2align	4,,15
	.globl	GL_PREFIX(PushMatrix)
	.type	GL_PREFIX(PushMatrix), @function
GL_PREFIX(PushMatrix):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2384(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	2384(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2384(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	2384(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PushMatrix), .-GL_PREFIX(PushMatrix)

	.p2align	4,,15
	.globl	GL_PREFIX(Rotated)
	.type	GL_PREFIX(Rotated), @function
GL_PREFIX(Rotated):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2392(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	2392(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2392(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	2392(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Rotated), .-GL_PREFIX(Rotated)

	.p2align	4,,15
	.globl	GL_PREFIX(Rotatef)
	.type	GL_PREFIX(Rotatef), @function
GL_PREFIX(Rotatef):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2400(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	2400(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2400(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	2400(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Rotatef), .-GL_PREFIX(Rotatef)

	.p2align	4,,15
	.globl	GL_PREFIX(Scaled)
	.type	GL_PREFIX(Scaled), @function
GL_PREFIX(Scaled):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2408(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	2408(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2408(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	2408(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Scaled), .-GL_PREFIX(Scaled)

	.p2align	4,,15
	.globl	GL_PREFIX(Scalef)
	.type	GL_PREFIX(Scalef), @function
GL_PREFIX(Scalef):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2416(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	2416(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2416(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	2416(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Scalef), .-GL_PREFIX(Scalef)

	.p2align	4,,15
	.globl	GL_PREFIX(Translated)
	.type	GL_PREFIX(Translated), @function
GL_PREFIX(Translated):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2424(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	2424(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2424(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	2424(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Translated), .-GL_PREFIX(Translated)

	.p2align	4,,15
	.globl	GL_PREFIX(Translatef)
	.type	GL_PREFIX(Translatef), @function
GL_PREFIX(Translatef):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2432(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	2432(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2432(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	2432(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Translatef), .-GL_PREFIX(Translatef)

	.p2align	4,,15
	.globl	GL_PREFIX(Viewport)
	.type	GL_PREFIX(Viewport), @function
GL_PREFIX(Viewport):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2440(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2440(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2440(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2440(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Viewport), .-GL_PREFIX(Viewport)

	.p2align	4,,15
	.globl	GL_PREFIX(ArrayElement)
	.type	GL_PREFIX(ArrayElement), @function
GL_PREFIX(ArrayElement):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2448(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2448(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2448(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2448(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ArrayElement), .-GL_PREFIX(ArrayElement)

	.p2align	4,,15
	.globl	GL_PREFIX(BindTexture)
	.type	GL_PREFIX(BindTexture), @function
GL_PREFIX(BindTexture):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2456(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2456(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2456(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2456(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BindTexture), .-GL_PREFIX(BindTexture)

	.p2align	4,,15
	.globl	GL_PREFIX(ColorPointer)
	.type	GL_PREFIX(ColorPointer), @function
GL_PREFIX(ColorPointer):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2464(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2464(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2464(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2464(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ColorPointer), .-GL_PREFIX(ColorPointer)

	.p2align	4,,15
	.globl	GL_PREFIX(DisableClientState)
	.type	GL_PREFIX(DisableClientState), @function
GL_PREFIX(DisableClientState):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2472(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2472(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2472(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2472(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DisableClientState), .-GL_PREFIX(DisableClientState)

	.p2align	4,,15
	.globl	GL_PREFIX(DrawArrays)
	.type	GL_PREFIX(DrawArrays), @function
GL_PREFIX(DrawArrays):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2480(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2480(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2480(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2480(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DrawArrays), .-GL_PREFIX(DrawArrays)

	.p2align	4,,15
	.globl	GL_PREFIX(DrawElements)
	.type	GL_PREFIX(DrawElements), @function
GL_PREFIX(DrawElements):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2488(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2488(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2488(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2488(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DrawElements), .-GL_PREFIX(DrawElements)

	.p2align	4,,15
	.globl	GL_PREFIX(EdgeFlagPointer)
	.type	GL_PREFIX(EdgeFlagPointer), @function
GL_PREFIX(EdgeFlagPointer):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2496(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2496(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2496(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2496(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EdgeFlagPointer), .-GL_PREFIX(EdgeFlagPointer)

	.p2align	4,,15
	.globl	GL_PREFIX(EnableClientState)
	.type	GL_PREFIX(EnableClientState), @function
GL_PREFIX(EnableClientState):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2504(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2504(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2504(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2504(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EnableClientState), .-GL_PREFIX(EnableClientState)

	.p2align	4,,15
	.globl	GL_PREFIX(IndexPointer)
	.type	GL_PREFIX(IndexPointer), @function
GL_PREFIX(IndexPointer):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2512(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2512(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2512(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2512(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(IndexPointer), .-GL_PREFIX(IndexPointer)

	.p2align	4,,15
	.globl	GL_PREFIX(Indexub)
	.type	GL_PREFIX(Indexub), @function
GL_PREFIX(Indexub):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2520(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2520(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2520(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2520(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Indexub), .-GL_PREFIX(Indexub)

	.p2align	4,,15
	.globl	GL_PREFIX(Indexubv)
	.type	GL_PREFIX(Indexubv), @function
GL_PREFIX(Indexubv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2528(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2528(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2528(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2528(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Indexubv), .-GL_PREFIX(Indexubv)

	.p2align	4,,15
	.globl	GL_PREFIX(InterleavedArrays)
	.type	GL_PREFIX(InterleavedArrays), @function
GL_PREFIX(InterleavedArrays):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2536(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2536(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2536(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2536(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(InterleavedArrays), .-GL_PREFIX(InterleavedArrays)

	.p2align	4,,15
	.globl	GL_PREFIX(NormalPointer)
	.type	GL_PREFIX(NormalPointer), @function
GL_PREFIX(NormalPointer):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2544(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2544(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2544(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2544(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(NormalPointer), .-GL_PREFIX(NormalPointer)

	.p2align	4,,15
	.globl	GL_PREFIX(PolygonOffset)
	.type	GL_PREFIX(PolygonOffset), @function
GL_PREFIX(PolygonOffset):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2552(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	2552(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2552(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	2552(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PolygonOffset), .-GL_PREFIX(PolygonOffset)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoordPointer)
	.type	GL_PREFIX(TexCoordPointer), @function
GL_PREFIX(TexCoordPointer):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2560(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2560(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2560(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2560(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoordPointer), .-GL_PREFIX(TexCoordPointer)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexPointer)
	.type	GL_PREFIX(VertexPointer), @function
GL_PREFIX(VertexPointer):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2568(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2568(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2568(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2568(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexPointer), .-GL_PREFIX(VertexPointer)

	.p2align	4,,15
	.globl	GL_PREFIX(AreTexturesResident)
	.type	GL_PREFIX(AreTexturesResident), @function
GL_PREFIX(AreTexturesResident):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2576(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2576(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2576(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2576(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(AreTexturesResident), .-GL_PREFIX(AreTexturesResident)

	.p2align	4,,15
	.globl	GL_PREFIX(CopyTexImage1D)
	.type	GL_PREFIX(CopyTexImage1D), @function
GL_PREFIX(CopyTexImage1D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2584(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2584(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2584(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2584(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CopyTexImage1D), .-GL_PREFIX(CopyTexImage1D)

	.p2align	4,,15
	.globl	GL_PREFIX(CopyTexImage2D)
	.type	GL_PREFIX(CopyTexImage2D), @function
GL_PREFIX(CopyTexImage2D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2592(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2592(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2592(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2592(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CopyTexImage2D), .-GL_PREFIX(CopyTexImage2D)

	.p2align	4,,15
	.globl	GL_PREFIX(CopyTexSubImage1D)
	.type	GL_PREFIX(CopyTexSubImage1D), @function
GL_PREFIX(CopyTexSubImage1D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2600(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2600(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2600(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2600(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CopyTexSubImage1D), .-GL_PREFIX(CopyTexSubImage1D)

	.p2align	4,,15
	.globl	GL_PREFIX(CopyTexSubImage2D)
	.type	GL_PREFIX(CopyTexSubImage2D), @function
GL_PREFIX(CopyTexSubImage2D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2608(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2608(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2608(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2608(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CopyTexSubImage2D), .-GL_PREFIX(CopyTexSubImage2D)

	.p2align	4,,15
	.globl	GL_PREFIX(DeleteTextures)
	.type	GL_PREFIX(DeleteTextures), @function
GL_PREFIX(DeleteTextures):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2616(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2616(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2616(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2616(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DeleteTextures), .-GL_PREFIX(DeleteTextures)

	.p2align	4,,15
	.globl	GL_PREFIX(GenTextures)
	.type	GL_PREFIX(GenTextures), @function
GL_PREFIX(GenTextures):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2624(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2624(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2624(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2624(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GenTextures), .-GL_PREFIX(GenTextures)

	.p2align	4,,15
	.globl	GL_PREFIX(GetPointerv)
	.type	GL_PREFIX(GetPointerv), @function
GL_PREFIX(GetPointerv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2632(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2632(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2632(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	2632(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetPointerv), .-GL_PREFIX(GetPointerv)

	.p2align	4,,15
	.globl	GL_PREFIX(IsTexture)
	.type	GL_PREFIX(IsTexture), @function
GL_PREFIX(IsTexture):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2640(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2640(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2640(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2640(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(IsTexture), .-GL_PREFIX(IsTexture)

	.p2align	4,,15
	.globl	GL_PREFIX(PrioritizeTextures)
	.type	GL_PREFIX(PrioritizeTextures), @function
GL_PREFIX(PrioritizeTextures):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2648(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2648(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2648(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2648(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PrioritizeTextures), .-GL_PREFIX(PrioritizeTextures)

	.p2align	4,,15
	.globl	GL_PREFIX(TexSubImage1D)
	.type	GL_PREFIX(TexSubImage1D), @function
GL_PREFIX(TexSubImage1D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2656(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2656(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2656(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2656(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexSubImage1D), .-GL_PREFIX(TexSubImage1D)

	.p2align	4,,15
	.globl	GL_PREFIX(TexSubImage2D)
	.type	GL_PREFIX(TexSubImage2D), @function
GL_PREFIX(TexSubImage2D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2664(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2664(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2664(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2664(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexSubImage2D), .-GL_PREFIX(TexSubImage2D)

	.p2align	4,,15
	.globl	GL_PREFIX(PopClientAttrib)
	.type	GL_PREFIX(PopClientAttrib), @function
GL_PREFIX(PopClientAttrib):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2672(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	2672(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2672(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	2672(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PopClientAttrib), .-GL_PREFIX(PopClientAttrib)

	.p2align	4,,15
	.globl	GL_PREFIX(PushClientAttrib)
	.type	GL_PREFIX(PushClientAttrib), @function
GL_PREFIX(PushClientAttrib):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2680(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2680(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2680(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2680(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PushClientAttrib), .-GL_PREFIX(PushClientAttrib)

	.p2align	4,,15
	.globl	GL_PREFIX(BlendColor)
	.type	GL_PREFIX(BlendColor), @function
GL_PREFIX(BlendColor):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2688(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2688(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2688(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2688(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BlendColor), .-GL_PREFIX(BlendColor)

	.p2align	4,,15
	.globl	GL_PREFIX(BlendEquation)
	.type	GL_PREFIX(BlendEquation), @function
GL_PREFIX(BlendEquation):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2696(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2696(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2696(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2696(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BlendEquation), .-GL_PREFIX(BlendEquation)

	.p2align	4,,15
	.globl	GL_PREFIX(DrawRangeElements)
	.type	GL_PREFIX(DrawRangeElements), @function
GL_PREFIX(DrawRangeElements):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2704(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2704(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2704(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2704(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DrawRangeElements), .-GL_PREFIX(DrawRangeElements)

	.p2align	4,,15
	.globl	GL_PREFIX(ColorTable)
	.type	GL_PREFIX(ColorTable), @function
GL_PREFIX(ColorTable):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2712(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2712(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2712(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2712(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ColorTable), .-GL_PREFIX(ColorTable)

	.p2align	4,,15
	.globl	GL_PREFIX(ColorTableParameterfv)
	.type	GL_PREFIX(ColorTableParameterfv), @function
GL_PREFIX(ColorTableParameterfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2720(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2720(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2720(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2720(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ColorTableParameterfv), .-GL_PREFIX(ColorTableParameterfv)

	.p2align	4,,15
	.globl	GL_PREFIX(ColorTableParameteriv)
	.type	GL_PREFIX(ColorTableParameteriv), @function
GL_PREFIX(ColorTableParameteriv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2728(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2728(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2728(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2728(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ColorTableParameteriv), .-GL_PREFIX(ColorTableParameteriv)

	.p2align	4,,15
	.globl	GL_PREFIX(CopyColorTable)
	.type	GL_PREFIX(CopyColorTable), @function
GL_PREFIX(CopyColorTable):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2736(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2736(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2736(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2736(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CopyColorTable), .-GL_PREFIX(CopyColorTable)

	.p2align	4,,15
	.globl	GL_PREFIX(GetColorTable)
	.type	GL_PREFIX(GetColorTable), @function
GL_PREFIX(GetColorTable):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2744(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2744(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2744(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2744(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetColorTable), .-GL_PREFIX(GetColorTable)

	.p2align	4,,15
	.globl	GL_PREFIX(GetColorTableParameterfv)
	.type	GL_PREFIX(GetColorTableParameterfv), @function
GL_PREFIX(GetColorTableParameterfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2752(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2752(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2752(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2752(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetColorTableParameterfv), .-GL_PREFIX(GetColorTableParameterfv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetColorTableParameteriv)
	.type	GL_PREFIX(GetColorTableParameteriv), @function
GL_PREFIX(GetColorTableParameteriv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2760(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2760(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2760(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2760(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetColorTableParameteriv), .-GL_PREFIX(GetColorTableParameteriv)

	.p2align	4,,15
	.globl	GL_PREFIX(ColorSubTable)
	.type	GL_PREFIX(ColorSubTable), @function
GL_PREFIX(ColorSubTable):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2768(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2768(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2768(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2768(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ColorSubTable), .-GL_PREFIX(ColorSubTable)

	.p2align	4,,15
	.globl	GL_PREFIX(CopyColorSubTable)
	.type	GL_PREFIX(CopyColorSubTable), @function
GL_PREFIX(CopyColorSubTable):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2776(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2776(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2776(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2776(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CopyColorSubTable), .-GL_PREFIX(CopyColorSubTable)

	.p2align	4,,15
	.globl	GL_PREFIX(ConvolutionFilter1D)
	.type	GL_PREFIX(ConvolutionFilter1D), @function
GL_PREFIX(ConvolutionFilter1D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2784(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2784(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2784(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2784(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ConvolutionFilter1D), .-GL_PREFIX(ConvolutionFilter1D)

	.p2align	4,,15
	.globl	GL_PREFIX(ConvolutionFilter2D)
	.type	GL_PREFIX(ConvolutionFilter2D), @function
GL_PREFIX(ConvolutionFilter2D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2792(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2792(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2792(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2792(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ConvolutionFilter2D), .-GL_PREFIX(ConvolutionFilter2D)

	.p2align	4,,15
	.globl	GL_PREFIX(ConvolutionParameterf)
	.type	GL_PREFIX(ConvolutionParameterf), @function
GL_PREFIX(ConvolutionParameterf):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2800(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	2800(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2800(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	2800(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ConvolutionParameterf), .-GL_PREFIX(ConvolutionParameterf)

	.p2align	4,,15
	.globl	GL_PREFIX(ConvolutionParameterfv)
	.type	GL_PREFIX(ConvolutionParameterfv), @function
GL_PREFIX(ConvolutionParameterfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2808(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2808(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2808(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2808(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ConvolutionParameterfv), .-GL_PREFIX(ConvolutionParameterfv)

	.p2align	4,,15
	.globl	GL_PREFIX(ConvolutionParameteri)
	.type	GL_PREFIX(ConvolutionParameteri), @function
GL_PREFIX(ConvolutionParameteri):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2816(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2816(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2816(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2816(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ConvolutionParameteri), .-GL_PREFIX(ConvolutionParameteri)

	.p2align	4,,15
	.globl	GL_PREFIX(ConvolutionParameteriv)
	.type	GL_PREFIX(ConvolutionParameteriv), @function
GL_PREFIX(ConvolutionParameteriv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2824(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2824(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2824(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2824(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ConvolutionParameteriv), .-GL_PREFIX(ConvolutionParameteriv)

	.p2align	4,,15
	.globl	GL_PREFIX(CopyConvolutionFilter1D)
	.type	GL_PREFIX(CopyConvolutionFilter1D), @function
GL_PREFIX(CopyConvolutionFilter1D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2832(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2832(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2832(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2832(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CopyConvolutionFilter1D), .-GL_PREFIX(CopyConvolutionFilter1D)

	.p2align	4,,15
	.globl	GL_PREFIX(CopyConvolutionFilter2D)
	.type	GL_PREFIX(CopyConvolutionFilter2D), @function
GL_PREFIX(CopyConvolutionFilter2D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2840(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2840(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2840(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2840(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CopyConvolutionFilter2D), .-GL_PREFIX(CopyConvolutionFilter2D)

	.p2align	4,,15
	.globl	GL_PREFIX(GetConvolutionFilter)
	.type	GL_PREFIX(GetConvolutionFilter), @function
GL_PREFIX(GetConvolutionFilter):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2848(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2848(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2848(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2848(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetConvolutionFilter), .-GL_PREFIX(GetConvolutionFilter)

	.p2align	4,,15
	.globl	GL_PREFIX(GetConvolutionParameterfv)
	.type	GL_PREFIX(GetConvolutionParameterfv), @function
GL_PREFIX(GetConvolutionParameterfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2856(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2856(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2856(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2856(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetConvolutionParameterfv), .-GL_PREFIX(GetConvolutionParameterfv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetConvolutionParameteriv)
	.type	GL_PREFIX(GetConvolutionParameteriv), @function
GL_PREFIX(GetConvolutionParameteriv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2864(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2864(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2864(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2864(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetConvolutionParameteriv), .-GL_PREFIX(GetConvolutionParameteriv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetSeparableFilter)
	.type	GL_PREFIX(GetSeparableFilter), @function
GL_PREFIX(GetSeparableFilter):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2872(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2872(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2872(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2872(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetSeparableFilter), .-GL_PREFIX(GetSeparableFilter)

	.p2align	4,,15
	.globl	GL_PREFIX(SeparableFilter2D)
	.type	GL_PREFIX(SeparableFilter2D), @function
GL_PREFIX(SeparableFilter2D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2880(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2880(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2880(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2880(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SeparableFilter2D), .-GL_PREFIX(SeparableFilter2D)

	.p2align	4,,15
	.globl	GL_PREFIX(GetHistogram)
	.type	GL_PREFIX(GetHistogram), @function
GL_PREFIX(GetHistogram):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2888(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2888(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2888(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2888(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetHistogram), .-GL_PREFIX(GetHistogram)

	.p2align	4,,15
	.globl	GL_PREFIX(GetHistogramParameterfv)
	.type	GL_PREFIX(GetHistogramParameterfv), @function
GL_PREFIX(GetHistogramParameterfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2896(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2896(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2896(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2896(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetHistogramParameterfv), .-GL_PREFIX(GetHistogramParameterfv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetHistogramParameteriv)
	.type	GL_PREFIX(GetHistogramParameteriv), @function
GL_PREFIX(GetHistogramParameteriv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2904(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2904(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2904(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2904(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetHistogramParameteriv), .-GL_PREFIX(GetHistogramParameteriv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetMinmax)
	.type	GL_PREFIX(GetMinmax), @function
GL_PREFIX(GetMinmax):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2912(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2912(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2912(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2912(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetMinmax), .-GL_PREFIX(GetMinmax)

	.p2align	4,,15
	.globl	GL_PREFIX(GetMinmaxParameterfv)
	.type	GL_PREFIX(GetMinmaxParameterfv), @function
GL_PREFIX(GetMinmaxParameterfv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2920(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2920(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2920(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2920(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetMinmaxParameterfv), .-GL_PREFIX(GetMinmaxParameterfv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetMinmaxParameteriv)
	.type	GL_PREFIX(GetMinmaxParameteriv), @function
GL_PREFIX(GetMinmaxParameteriv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2928(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2928(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2928(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2928(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetMinmaxParameteriv), .-GL_PREFIX(GetMinmaxParameteriv)

	.p2align	4,,15
	.globl	GL_PREFIX(Histogram)
	.type	GL_PREFIX(Histogram), @function
GL_PREFIX(Histogram):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2936(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2936(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2936(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2936(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Histogram), .-GL_PREFIX(Histogram)

	.p2align	4,,15
	.globl	GL_PREFIX(Minmax)
	.type	GL_PREFIX(Minmax), @function
GL_PREFIX(Minmax):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2944(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2944(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2944(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2944(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Minmax), .-GL_PREFIX(Minmax)

	.p2align	4,,15
	.globl	GL_PREFIX(ResetHistogram)
	.type	GL_PREFIX(ResetHistogram), @function
GL_PREFIX(ResetHistogram):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2952(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2952(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2952(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2952(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ResetHistogram), .-GL_PREFIX(ResetHistogram)

	.p2align	4,,15
	.globl	GL_PREFIX(ResetMinmax)
	.type	GL_PREFIX(ResetMinmax), @function
GL_PREFIX(ResetMinmax):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2960(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2960(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2960(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2960(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ResetMinmax), .-GL_PREFIX(ResetMinmax)

	.p2align	4,,15
	.globl	GL_PREFIX(TexImage3D)
	.type	GL_PREFIX(TexImage3D), @function
GL_PREFIX(TexImage3D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2968(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2968(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2968(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2968(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexImage3D), .-GL_PREFIX(TexImage3D)

	.p2align	4,,15
	.globl	GL_PREFIX(TexSubImage3D)
	.type	GL_PREFIX(TexSubImage3D), @function
GL_PREFIX(TexSubImage3D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2976(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2976(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2976(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2976(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexSubImage3D), .-GL_PREFIX(TexSubImage3D)

	.p2align	4,,15
	.globl	GL_PREFIX(CopyTexSubImage3D)
	.type	GL_PREFIX(CopyTexSubImage3D), @function
GL_PREFIX(CopyTexSubImage3D):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2984(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2984(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2984(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	2984(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CopyTexSubImage3D), .-GL_PREFIX(CopyTexSubImage3D)

	.p2align	4,,15
	.globl	GL_PREFIX(ActiveTextureARB)
	.type	GL_PREFIX(ActiveTextureARB), @function
GL_PREFIX(ActiveTextureARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	2992(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	2992(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	2992(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	2992(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ActiveTextureARB), .-GL_PREFIX(ActiveTextureARB)

	.p2align	4,,15
	.globl	GL_PREFIX(ClientActiveTextureARB)
	.type	GL_PREFIX(ClientActiveTextureARB), @function
GL_PREFIX(ClientActiveTextureARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3000(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3000(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3000(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3000(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ClientActiveTextureARB), .-GL_PREFIX(ClientActiveTextureARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord1dARB)
	.type	GL_PREFIX(MultiTexCoord1dARB), @function
GL_PREFIX(MultiTexCoord1dARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3008(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3008(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3008(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3008(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord1dARB), .-GL_PREFIX(MultiTexCoord1dARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord1dvARB)
	.type	GL_PREFIX(MultiTexCoord1dvARB), @function
GL_PREFIX(MultiTexCoord1dvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3016(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3016(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3016(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3016(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord1dvARB), .-GL_PREFIX(MultiTexCoord1dvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord1fARB)
	.type	GL_PREFIX(MultiTexCoord1fARB), @function
GL_PREFIX(MultiTexCoord1fARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3024(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3024(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3024(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3024(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord1fARB), .-GL_PREFIX(MultiTexCoord1fARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord1fvARB)
	.type	GL_PREFIX(MultiTexCoord1fvARB), @function
GL_PREFIX(MultiTexCoord1fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3032(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3032(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3032(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3032(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord1fvARB), .-GL_PREFIX(MultiTexCoord1fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord1iARB)
	.type	GL_PREFIX(MultiTexCoord1iARB), @function
GL_PREFIX(MultiTexCoord1iARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3040(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3040(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3040(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3040(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord1iARB), .-GL_PREFIX(MultiTexCoord1iARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord1ivARB)
	.type	GL_PREFIX(MultiTexCoord1ivARB), @function
GL_PREFIX(MultiTexCoord1ivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3048(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3048(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3048(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3048(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord1ivARB), .-GL_PREFIX(MultiTexCoord1ivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord1sARB)
	.type	GL_PREFIX(MultiTexCoord1sARB), @function
GL_PREFIX(MultiTexCoord1sARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3056(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3056(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3056(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3056(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord1sARB), .-GL_PREFIX(MultiTexCoord1sARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord1svARB)
	.type	GL_PREFIX(MultiTexCoord1svARB), @function
GL_PREFIX(MultiTexCoord1svARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3064(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3064(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3064(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3064(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord1svARB), .-GL_PREFIX(MultiTexCoord1svARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord2dARB)
	.type	GL_PREFIX(MultiTexCoord2dARB), @function
GL_PREFIX(MultiTexCoord2dARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3072(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3072(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3072(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3072(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord2dARB), .-GL_PREFIX(MultiTexCoord2dARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord2dvARB)
	.type	GL_PREFIX(MultiTexCoord2dvARB), @function
GL_PREFIX(MultiTexCoord2dvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3080(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3080(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3080(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3080(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord2dvARB), .-GL_PREFIX(MultiTexCoord2dvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord2fARB)
	.type	GL_PREFIX(MultiTexCoord2fARB), @function
GL_PREFIX(MultiTexCoord2fARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3088(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3088(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3088(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3088(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord2fARB), .-GL_PREFIX(MultiTexCoord2fARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord2fvARB)
	.type	GL_PREFIX(MultiTexCoord2fvARB), @function
GL_PREFIX(MultiTexCoord2fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3096(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3096(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3096(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3096(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord2fvARB), .-GL_PREFIX(MultiTexCoord2fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord2iARB)
	.type	GL_PREFIX(MultiTexCoord2iARB), @function
GL_PREFIX(MultiTexCoord2iARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3104(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3104(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3104(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3104(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord2iARB), .-GL_PREFIX(MultiTexCoord2iARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord2ivARB)
	.type	GL_PREFIX(MultiTexCoord2ivARB), @function
GL_PREFIX(MultiTexCoord2ivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3112(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3112(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3112(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3112(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord2ivARB), .-GL_PREFIX(MultiTexCoord2ivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord2sARB)
	.type	GL_PREFIX(MultiTexCoord2sARB), @function
GL_PREFIX(MultiTexCoord2sARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3120(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3120(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3120(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3120(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord2sARB), .-GL_PREFIX(MultiTexCoord2sARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord2svARB)
	.type	GL_PREFIX(MultiTexCoord2svARB), @function
GL_PREFIX(MultiTexCoord2svARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3128(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3128(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3128(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3128(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord2svARB), .-GL_PREFIX(MultiTexCoord2svARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord3dARB)
	.type	GL_PREFIX(MultiTexCoord3dARB), @function
GL_PREFIX(MultiTexCoord3dARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3136(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3136(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3136(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3136(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord3dARB), .-GL_PREFIX(MultiTexCoord3dARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord3dvARB)
	.type	GL_PREFIX(MultiTexCoord3dvARB), @function
GL_PREFIX(MultiTexCoord3dvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3144(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3144(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3144(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3144(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord3dvARB), .-GL_PREFIX(MultiTexCoord3dvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord3fARB)
	.type	GL_PREFIX(MultiTexCoord3fARB), @function
GL_PREFIX(MultiTexCoord3fARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3152(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3152(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3152(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3152(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord3fARB), .-GL_PREFIX(MultiTexCoord3fARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord3fvARB)
	.type	GL_PREFIX(MultiTexCoord3fvARB), @function
GL_PREFIX(MultiTexCoord3fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3160(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3160(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3160(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3160(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord3fvARB), .-GL_PREFIX(MultiTexCoord3fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord3iARB)
	.type	GL_PREFIX(MultiTexCoord3iARB), @function
GL_PREFIX(MultiTexCoord3iARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3168(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3168(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3168(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3168(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord3iARB), .-GL_PREFIX(MultiTexCoord3iARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord3ivARB)
	.type	GL_PREFIX(MultiTexCoord3ivARB), @function
GL_PREFIX(MultiTexCoord3ivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3176(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3176(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3176(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3176(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord3ivARB), .-GL_PREFIX(MultiTexCoord3ivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord3sARB)
	.type	GL_PREFIX(MultiTexCoord3sARB), @function
GL_PREFIX(MultiTexCoord3sARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3184(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3184(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3184(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3184(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord3sARB), .-GL_PREFIX(MultiTexCoord3sARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord3svARB)
	.type	GL_PREFIX(MultiTexCoord3svARB), @function
GL_PREFIX(MultiTexCoord3svARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3192(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3192(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3192(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3192(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord3svARB), .-GL_PREFIX(MultiTexCoord3svARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord4dARB)
	.type	GL_PREFIX(MultiTexCoord4dARB), @function
GL_PREFIX(MultiTexCoord4dARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3200(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	movq	%xmm3, 32(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	32(%rsp), %xmm3
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3200(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3200(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	movq	%xmm3, 32(%rsp)
	call	_glapi_get_dispatch
	movq	32(%rsp), %xmm3
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3200(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord4dARB), .-GL_PREFIX(MultiTexCoord4dARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord4dvARB)
	.type	GL_PREFIX(MultiTexCoord4dvARB), @function
GL_PREFIX(MultiTexCoord4dvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3208(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3208(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3208(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3208(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord4dvARB), .-GL_PREFIX(MultiTexCoord4dvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord4fARB)
	.type	GL_PREFIX(MultiTexCoord4fARB), @function
GL_PREFIX(MultiTexCoord4fARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3216(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	movq	%xmm3, 32(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	32(%rsp), %xmm3
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3216(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3216(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	movq	%xmm3, 32(%rsp)
	call	_glapi_get_dispatch
	movq	32(%rsp), %xmm3
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3216(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord4fARB), .-GL_PREFIX(MultiTexCoord4fARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord4fvARB)
	.type	GL_PREFIX(MultiTexCoord4fvARB), @function
GL_PREFIX(MultiTexCoord4fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3224(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3224(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3224(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3224(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord4fvARB), .-GL_PREFIX(MultiTexCoord4fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord4iARB)
	.type	GL_PREFIX(MultiTexCoord4iARB), @function
GL_PREFIX(MultiTexCoord4iARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3232(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3232(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3232(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3232(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord4iARB), .-GL_PREFIX(MultiTexCoord4iARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord4ivARB)
	.type	GL_PREFIX(MultiTexCoord4ivARB), @function
GL_PREFIX(MultiTexCoord4ivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3240(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3240(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3240(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3240(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord4ivARB), .-GL_PREFIX(MultiTexCoord4ivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord4sARB)
	.type	GL_PREFIX(MultiTexCoord4sARB), @function
GL_PREFIX(MultiTexCoord4sARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3248(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3248(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3248(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3248(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord4sARB), .-GL_PREFIX(MultiTexCoord4sARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiTexCoord4svARB)
	.type	GL_PREFIX(MultiTexCoord4svARB), @function
GL_PREFIX(MultiTexCoord4svARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3256(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3256(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3256(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3256(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiTexCoord4svARB), .-GL_PREFIX(MultiTexCoord4svARB)

	.p2align	4,,15
	.globl	GL_PREFIX(AttachShader)
	.type	GL_PREFIX(AttachShader), @function
GL_PREFIX(AttachShader):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3264(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3264(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3264(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3264(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(AttachShader), .-GL_PREFIX(AttachShader)

	.p2align	4,,15
	.globl	GL_PREFIX(CreateProgram)
	.type	GL_PREFIX(CreateProgram), @function
GL_PREFIX(CreateProgram):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3272(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	3272(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3272(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	3272(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CreateProgram), .-GL_PREFIX(CreateProgram)

	.p2align	4,,15
	.globl	GL_PREFIX(CreateShader)
	.type	GL_PREFIX(CreateShader), @function
GL_PREFIX(CreateShader):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3280(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3280(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3280(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3280(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CreateShader), .-GL_PREFIX(CreateShader)

	.p2align	4,,15
	.globl	GL_PREFIX(DeleteProgram)
	.type	GL_PREFIX(DeleteProgram), @function
GL_PREFIX(DeleteProgram):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3288(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3288(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3288(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3288(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DeleteProgram), .-GL_PREFIX(DeleteProgram)

	.p2align	4,,15
	.globl	GL_PREFIX(DeleteShader)
	.type	GL_PREFIX(DeleteShader), @function
GL_PREFIX(DeleteShader):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3296(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3296(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3296(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3296(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DeleteShader), .-GL_PREFIX(DeleteShader)

	.p2align	4,,15
	.globl	GL_PREFIX(DetachShader)
	.type	GL_PREFIX(DetachShader), @function
GL_PREFIX(DetachShader):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3304(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3304(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3304(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3304(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DetachShader), .-GL_PREFIX(DetachShader)

	.p2align	4,,15
	.globl	GL_PREFIX(GetAttachedShaders)
	.type	GL_PREFIX(GetAttachedShaders), @function
GL_PREFIX(GetAttachedShaders):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3312(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3312(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3312(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3312(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetAttachedShaders), .-GL_PREFIX(GetAttachedShaders)

	.p2align	4,,15
	.globl	GL_PREFIX(GetProgramInfoLog)
	.type	GL_PREFIX(GetProgramInfoLog), @function
GL_PREFIX(GetProgramInfoLog):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3320(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3320(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3320(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3320(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetProgramInfoLog), .-GL_PREFIX(GetProgramInfoLog)

	.p2align	4,,15
	.globl	GL_PREFIX(GetProgramiv)
	.type	GL_PREFIX(GetProgramiv), @function
GL_PREFIX(GetProgramiv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3328(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3328(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3328(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3328(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetProgramiv), .-GL_PREFIX(GetProgramiv)

	.p2align	4,,15
	.globl	GL_PREFIX(GetShaderInfoLog)
	.type	GL_PREFIX(GetShaderInfoLog), @function
GL_PREFIX(GetShaderInfoLog):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3336(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3336(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3336(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3336(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetShaderInfoLog), .-GL_PREFIX(GetShaderInfoLog)

	.p2align	4,,15
	.globl	GL_PREFIX(GetShaderiv)
	.type	GL_PREFIX(GetShaderiv), @function
GL_PREFIX(GetShaderiv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3344(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3344(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3344(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3344(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetShaderiv), .-GL_PREFIX(GetShaderiv)

	.p2align	4,,15
	.globl	GL_PREFIX(IsProgram)
	.type	GL_PREFIX(IsProgram), @function
GL_PREFIX(IsProgram):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3352(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3352(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3352(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3352(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(IsProgram), .-GL_PREFIX(IsProgram)

	.p2align	4,,15
	.globl	GL_PREFIX(IsShader)
	.type	GL_PREFIX(IsShader), @function
GL_PREFIX(IsShader):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3360(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3360(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3360(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3360(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(IsShader), .-GL_PREFIX(IsShader)

	.p2align	4,,15
	.globl	GL_PREFIX(StencilFuncSeparate)
	.type	GL_PREFIX(StencilFuncSeparate), @function
GL_PREFIX(StencilFuncSeparate):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3368(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3368(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3368(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3368(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(StencilFuncSeparate), .-GL_PREFIX(StencilFuncSeparate)

	.p2align	4,,15
	.globl	GL_PREFIX(StencilMaskSeparate)
	.type	GL_PREFIX(StencilMaskSeparate), @function
GL_PREFIX(StencilMaskSeparate):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3376(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3376(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3376(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3376(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(StencilMaskSeparate), .-GL_PREFIX(StencilMaskSeparate)

	.p2align	4,,15
	.globl	GL_PREFIX(StencilOpSeparate)
	.type	GL_PREFIX(StencilOpSeparate), @function
GL_PREFIX(StencilOpSeparate):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3384(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3384(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3384(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3384(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(StencilOpSeparate), .-GL_PREFIX(StencilOpSeparate)

	.p2align	4,,15
	.globl	GL_PREFIX(UniformMatrix2x3fv)
	.type	GL_PREFIX(UniformMatrix2x3fv), @function
GL_PREFIX(UniformMatrix2x3fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3392(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3392(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3392(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3392(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(UniformMatrix2x3fv), .-GL_PREFIX(UniformMatrix2x3fv)

	.p2align	4,,15
	.globl	GL_PREFIX(UniformMatrix2x4fv)
	.type	GL_PREFIX(UniformMatrix2x4fv), @function
GL_PREFIX(UniformMatrix2x4fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3400(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3400(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3400(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3400(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(UniformMatrix2x4fv), .-GL_PREFIX(UniformMatrix2x4fv)

	.p2align	4,,15
	.globl	GL_PREFIX(UniformMatrix3x2fv)
	.type	GL_PREFIX(UniformMatrix3x2fv), @function
GL_PREFIX(UniformMatrix3x2fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3408(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3408(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3408(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3408(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(UniformMatrix3x2fv), .-GL_PREFIX(UniformMatrix3x2fv)

	.p2align	4,,15
	.globl	GL_PREFIX(UniformMatrix3x4fv)
	.type	GL_PREFIX(UniformMatrix3x4fv), @function
GL_PREFIX(UniformMatrix3x4fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3416(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3416(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3416(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3416(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(UniformMatrix3x4fv), .-GL_PREFIX(UniformMatrix3x4fv)

	.p2align	4,,15
	.globl	GL_PREFIX(UniformMatrix4x2fv)
	.type	GL_PREFIX(UniformMatrix4x2fv), @function
GL_PREFIX(UniformMatrix4x2fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3424(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3424(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3424(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3424(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(UniformMatrix4x2fv), .-GL_PREFIX(UniformMatrix4x2fv)

	.p2align	4,,15
	.globl	GL_PREFIX(UniformMatrix4x3fv)
	.type	GL_PREFIX(UniformMatrix4x3fv), @function
GL_PREFIX(UniformMatrix4x3fv):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3432(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3432(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3432(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3432(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(UniformMatrix4x3fv), .-GL_PREFIX(UniformMatrix4x3fv)

	.p2align	4,,15
	.globl	GL_PREFIX(LoadTransposeMatrixdARB)
	.type	GL_PREFIX(LoadTransposeMatrixdARB), @function
GL_PREFIX(LoadTransposeMatrixdARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3440(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3440(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3440(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3440(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LoadTransposeMatrixdARB), .-GL_PREFIX(LoadTransposeMatrixdARB)

	.p2align	4,,15
	.globl	GL_PREFIX(LoadTransposeMatrixfARB)
	.type	GL_PREFIX(LoadTransposeMatrixfARB), @function
GL_PREFIX(LoadTransposeMatrixfARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3448(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3448(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3448(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3448(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LoadTransposeMatrixfARB), .-GL_PREFIX(LoadTransposeMatrixfARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultTransposeMatrixdARB)
	.type	GL_PREFIX(MultTransposeMatrixdARB), @function
GL_PREFIX(MultTransposeMatrixdARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3456(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3456(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3456(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3456(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultTransposeMatrixdARB), .-GL_PREFIX(MultTransposeMatrixdARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MultTransposeMatrixfARB)
	.type	GL_PREFIX(MultTransposeMatrixfARB), @function
GL_PREFIX(MultTransposeMatrixfARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3464(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3464(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3464(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3464(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultTransposeMatrixfARB), .-GL_PREFIX(MultTransposeMatrixfARB)

	.p2align	4,,15
	.globl	GL_PREFIX(SampleCoverageARB)
	.type	GL_PREFIX(SampleCoverageARB), @function
GL_PREFIX(SampleCoverageARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3472(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3472(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3472(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3472(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SampleCoverageARB), .-GL_PREFIX(SampleCoverageARB)

	.p2align	4,,15
	.globl	GL_PREFIX(CompressedTexImage1DARB)
	.type	GL_PREFIX(CompressedTexImage1DARB), @function
GL_PREFIX(CompressedTexImage1DARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3480(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3480(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3480(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3480(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CompressedTexImage1DARB), .-GL_PREFIX(CompressedTexImage1DARB)

	.p2align	4,,15
	.globl	GL_PREFIX(CompressedTexImage2DARB)
	.type	GL_PREFIX(CompressedTexImage2DARB), @function
GL_PREFIX(CompressedTexImage2DARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3488(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3488(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3488(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3488(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CompressedTexImage2DARB), .-GL_PREFIX(CompressedTexImage2DARB)

	.p2align	4,,15
	.globl	GL_PREFIX(CompressedTexImage3DARB)
	.type	GL_PREFIX(CompressedTexImage3DARB), @function
GL_PREFIX(CompressedTexImage3DARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3496(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3496(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3496(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3496(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CompressedTexImage3DARB), .-GL_PREFIX(CompressedTexImage3DARB)

	.p2align	4,,15
	.globl	GL_PREFIX(CompressedTexSubImage1DARB)
	.type	GL_PREFIX(CompressedTexSubImage1DARB), @function
GL_PREFIX(CompressedTexSubImage1DARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3504(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3504(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3504(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3504(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CompressedTexSubImage1DARB), .-GL_PREFIX(CompressedTexSubImage1DARB)

	.p2align	4,,15
	.globl	GL_PREFIX(CompressedTexSubImage2DARB)
	.type	GL_PREFIX(CompressedTexSubImage2DARB), @function
GL_PREFIX(CompressedTexSubImage2DARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3512(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3512(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3512(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3512(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CompressedTexSubImage2DARB), .-GL_PREFIX(CompressedTexSubImage2DARB)

	.p2align	4,,15
	.globl	GL_PREFIX(CompressedTexSubImage3DARB)
	.type	GL_PREFIX(CompressedTexSubImage3DARB), @function
GL_PREFIX(CompressedTexSubImage3DARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3520(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3520(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3520(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3520(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CompressedTexSubImage3DARB), .-GL_PREFIX(CompressedTexSubImage3DARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetCompressedTexImageARB)
	.type	GL_PREFIX(GetCompressedTexImageARB), @function
GL_PREFIX(GetCompressedTexImageARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3528(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3528(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3528(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3528(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetCompressedTexImageARB), .-GL_PREFIX(GetCompressedTexImageARB)

	.p2align	4,,15
	.globl	GL_PREFIX(DisableVertexAttribArrayARB)
	.type	GL_PREFIX(DisableVertexAttribArrayARB), @function
GL_PREFIX(DisableVertexAttribArrayARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3536(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3536(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3536(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3536(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DisableVertexAttribArrayARB), .-GL_PREFIX(DisableVertexAttribArrayARB)

	.p2align	4,,15
	.globl	GL_PREFIX(EnableVertexAttribArrayARB)
	.type	GL_PREFIX(EnableVertexAttribArrayARB), @function
GL_PREFIX(EnableVertexAttribArrayARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3544(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3544(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3544(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3544(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EnableVertexAttribArrayARB), .-GL_PREFIX(EnableVertexAttribArrayARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetProgramEnvParameterdvARB)
	.type	GL_PREFIX(GetProgramEnvParameterdvARB), @function
GL_PREFIX(GetProgramEnvParameterdvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3552(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3552(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3552(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3552(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetProgramEnvParameterdvARB), .-GL_PREFIX(GetProgramEnvParameterdvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetProgramEnvParameterfvARB)
	.type	GL_PREFIX(GetProgramEnvParameterfvARB), @function
GL_PREFIX(GetProgramEnvParameterfvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3560(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3560(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3560(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3560(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetProgramEnvParameterfvARB), .-GL_PREFIX(GetProgramEnvParameterfvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetProgramLocalParameterdvARB)
	.type	GL_PREFIX(GetProgramLocalParameterdvARB), @function
GL_PREFIX(GetProgramLocalParameterdvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3568(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3568(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3568(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3568(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetProgramLocalParameterdvARB), .-GL_PREFIX(GetProgramLocalParameterdvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetProgramLocalParameterfvARB)
	.type	GL_PREFIX(GetProgramLocalParameterfvARB), @function
GL_PREFIX(GetProgramLocalParameterfvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3576(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3576(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3576(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3576(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetProgramLocalParameterfvARB), .-GL_PREFIX(GetProgramLocalParameterfvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetProgramStringARB)
	.type	GL_PREFIX(GetProgramStringARB), @function
GL_PREFIX(GetProgramStringARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3584(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3584(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3584(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3584(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetProgramStringARB), .-GL_PREFIX(GetProgramStringARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetProgramivARB)
	.type	GL_PREFIX(GetProgramivARB), @function
GL_PREFIX(GetProgramivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3592(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3592(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3592(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3592(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetProgramivARB), .-GL_PREFIX(GetProgramivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetVertexAttribdvARB)
	.type	GL_PREFIX(GetVertexAttribdvARB), @function
GL_PREFIX(GetVertexAttribdvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3600(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3600(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3600(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3600(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetVertexAttribdvARB), .-GL_PREFIX(GetVertexAttribdvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetVertexAttribfvARB)
	.type	GL_PREFIX(GetVertexAttribfvARB), @function
GL_PREFIX(GetVertexAttribfvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3608(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3608(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3608(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3608(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetVertexAttribfvARB), .-GL_PREFIX(GetVertexAttribfvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetVertexAttribivARB)
	.type	GL_PREFIX(GetVertexAttribivARB), @function
GL_PREFIX(GetVertexAttribivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3616(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3616(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3616(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3616(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetVertexAttribivARB), .-GL_PREFIX(GetVertexAttribivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramEnvParameter4dARB)
	.type	GL_PREFIX(ProgramEnvParameter4dARB), @function
GL_PREFIX(ProgramEnvParameter4dARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3624(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	movq	%xmm1, 24(%rsp)
	movq	%xmm2, 32(%rsp)
	movq	%xmm3, 40(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	40(%rsp), %xmm3
	movq	32(%rsp), %xmm2
	movq	24(%rsp), %xmm1
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	3624(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3624(%rax), %r11
	jmp	*%r11
1:
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	movq	%xmm1, 24(%rsp)
	movq	%xmm2, 32(%rsp)
	movq	%xmm3, 40(%rsp)
	call	_glapi_get_dispatch
	movq	40(%rsp), %xmm3
	movq	32(%rsp), %xmm2
	movq	24(%rsp), %xmm1
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	3624(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramEnvParameter4dARB), .-GL_PREFIX(ProgramEnvParameter4dARB)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramEnvParameter4dvARB)
	.type	GL_PREFIX(ProgramEnvParameter4dvARB), @function
GL_PREFIX(ProgramEnvParameter4dvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3632(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3632(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3632(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3632(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramEnvParameter4dvARB), .-GL_PREFIX(ProgramEnvParameter4dvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramEnvParameter4fARB)
	.type	GL_PREFIX(ProgramEnvParameter4fARB), @function
GL_PREFIX(ProgramEnvParameter4fARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3640(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	movq	%xmm1, 24(%rsp)
	movq	%xmm2, 32(%rsp)
	movq	%xmm3, 40(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	40(%rsp), %xmm3
	movq	32(%rsp), %xmm2
	movq	24(%rsp), %xmm1
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	3640(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3640(%rax), %r11
	jmp	*%r11
1:
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	movq	%xmm1, 24(%rsp)
	movq	%xmm2, 32(%rsp)
	movq	%xmm3, 40(%rsp)
	call	_glapi_get_dispatch
	movq	40(%rsp), %xmm3
	movq	32(%rsp), %xmm2
	movq	24(%rsp), %xmm1
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	3640(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramEnvParameter4fARB), .-GL_PREFIX(ProgramEnvParameter4fARB)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramEnvParameter4fvARB)
	.type	GL_PREFIX(ProgramEnvParameter4fvARB), @function
GL_PREFIX(ProgramEnvParameter4fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3648(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3648(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3648(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3648(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramEnvParameter4fvARB), .-GL_PREFIX(ProgramEnvParameter4fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramLocalParameter4dARB)
	.type	GL_PREFIX(ProgramLocalParameter4dARB), @function
GL_PREFIX(ProgramLocalParameter4dARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3656(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	movq	%xmm1, 24(%rsp)
	movq	%xmm2, 32(%rsp)
	movq	%xmm3, 40(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	40(%rsp), %xmm3
	movq	32(%rsp), %xmm2
	movq	24(%rsp), %xmm1
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	3656(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3656(%rax), %r11
	jmp	*%r11
1:
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	movq	%xmm1, 24(%rsp)
	movq	%xmm2, 32(%rsp)
	movq	%xmm3, 40(%rsp)
	call	_glapi_get_dispatch
	movq	40(%rsp), %xmm3
	movq	32(%rsp), %xmm2
	movq	24(%rsp), %xmm1
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	3656(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramLocalParameter4dARB), .-GL_PREFIX(ProgramLocalParameter4dARB)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramLocalParameter4dvARB)
	.type	GL_PREFIX(ProgramLocalParameter4dvARB), @function
GL_PREFIX(ProgramLocalParameter4dvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3664(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3664(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3664(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3664(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramLocalParameter4dvARB), .-GL_PREFIX(ProgramLocalParameter4dvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramLocalParameter4fARB)
	.type	GL_PREFIX(ProgramLocalParameter4fARB), @function
GL_PREFIX(ProgramLocalParameter4fARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3672(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	movq	%xmm1, 24(%rsp)
	movq	%xmm2, 32(%rsp)
	movq	%xmm3, 40(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	40(%rsp), %xmm3
	movq	32(%rsp), %xmm2
	movq	24(%rsp), %xmm1
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	3672(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3672(%rax), %r11
	jmp	*%r11
1:
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%xmm0, 16(%rsp)
	movq	%xmm1, 24(%rsp)
	movq	%xmm2, 32(%rsp)
	movq	%xmm3, 40(%rsp)
	call	_glapi_get_dispatch
	movq	40(%rsp), %xmm3
	movq	32(%rsp), %xmm2
	movq	24(%rsp), %xmm1
	movq	16(%rsp), %xmm0
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	3672(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramLocalParameter4fARB), .-GL_PREFIX(ProgramLocalParameter4fARB)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramLocalParameter4fvARB)
	.type	GL_PREFIX(ProgramLocalParameter4fvARB), @function
GL_PREFIX(ProgramLocalParameter4fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3680(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3680(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3680(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3680(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramLocalParameter4fvARB), .-GL_PREFIX(ProgramLocalParameter4fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramStringARB)
	.type	GL_PREFIX(ProgramStringARB), @function
GL_PREFIX(ProgramStringARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3688(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3688(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3688(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3688(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramStringARB), .-GL_PREFIX(ProgramStringARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib1dARB)
	.type	GL_PREFIX(VertexAttrib1dARB), @function
GL_PREFIX(VertexAttrib1dARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3696(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3696(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3696(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3696(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib1dARB), .-GL_PREFIX(VertexAttrib1dARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib1dvARB)
	.type	GL_PREFIX(VertexAttrib1dvARB), @function
GL_PREFIX(VertexAttrib1dvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3704(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3704(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3704(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3704(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib1dvARB), .-GL_PREFIX(VertexAttrib1dvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib1fARB)
	.type	GL_PREFIX(VertexAttrib1fARB), @function
GL_PREFIX(VertexAttrib1fARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3712(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3712(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3712(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3712(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib1fARB), .-GL_PREFIX(VertexAttrib1fARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib1fvARB)
	.type	GL_PREFIX(VertexAttrib1fvARB), @function
GL_PREFIX(VertexAttrib1fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3720(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3720(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3720(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3720(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib1fvARB), .-GL_PREFIX(VertexAttrib1fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib1sARB)
	.type	GL_PREFIX(VertexAttrib1sARB), @function
GL_PREFIX(VertexAttrib1sARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3728(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3728(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3728(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3728(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib1sARB), .-GL_PREFIX(VertexAttrib1sARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib1svARB)
	.type	GL_PREFIX(VertexAttrib1svARB), @function
GL_PREFIX(VertexAttrib1svARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3736(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3736(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3736(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3736(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib1svARB), .-GL_PREFIX(VertexAttrib1svARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib2dARB)
	.type	GL_PREFIX(VertexAttrib2dARB), @function
GL_PREFIX(VertexAttrib2dARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3744(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3744(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3744(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3744(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib2dARB), .-GL_PREFIX(VertexAttrib2dARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib2dvARB)
	.type	GL_PREFIX(VertexAttrib2dvARB), @function
GL_PREFIX(VertexAttrib2dvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3752(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3752(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3752(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3752(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib2dvARB), .-GL_PREFIX(VertexAttrib2dvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib2fARB)
	.type	GL_PREFIX(VertexAttrib2fARB), @function
GL_PREFIX(VertexAttrib2fARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3760(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3760(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3760(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3760(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib2fARB), .-GL_PREFIX(VertexAttrib2fARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib2fvARB)
	.type	GL_PREFIX(VertexAttrib2fvARB), @function
GL_PREFIX(VertexAttrib2fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3768(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3768(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3768(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3768(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib2fvARB), .-GL_PREFIX(VertexAttrib2fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib2sARB)
	.type	GL_PREFIX(VertexAttrib2sARB), @function
GL_PREFIX(VertexAttrib2sARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3776(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3776(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3776(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3776(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib2sARB), .-GL_PREFIX(VertexAttrib2sARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib2svARB)
	.type	GL_PREFIX(VertexAttrib2svARB), @function
GL_PREFIX(VertexAttrib2svARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3784(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3784(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3784(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3784(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib2svARB), .-GL_PREFIX(VertexAttrib2svARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib3dARB)
	.type	GL_PREFIX(VertexAttrib3dARB), @function
GL_PREFIX(VertexAttrib3dARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3792(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3792(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3792(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3792(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib3dARB), .-GL_PREFIX(VertexAttrib3dARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib3dvARB)
	.type	GL_PREFIX(VertexAttrib3dvARB), @function
GL_PREFIX(VertexAttrib3dvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3800(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3800(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3800(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3800(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib3dvARB), .-GL_PREFIX(VertexAttrib3dvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib3fARB)
	.type	GL_PREFIX(VertexAttrib3fARB), @function
GL_PREFIX(VertexAttrib3fARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3808(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3808(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3808(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3808(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib3fARB), .-GL_PREFIX(VertexAttrib3fARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib3fvARB)
	.type	GL_PREFIX(VertexAttrib3fvARB), @function
GL_PREFIX(VertexAttrib3fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3816(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3816(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3816(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3816(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib3fvARB), .-GL_PREFIX(VertexAttrib3fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib3sARB)
	.type	GL_PREFIX(VertexAttrib3sARB), @function
GL_PREFIX(VertexAttrib3sARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3824(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3824(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3824(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3824(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib3sARB), .-GL_PREFIX(VertexAttrib3sARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib3svARB)
	.type	GL_PREFIX(VertexAttrib3svARB), @function
GL_PREFIX(VertexAttrib3svARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3832(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3832(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3832(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3832(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib3svARB), .-GL_PREFIX(VertexAttrib3svARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4NbvARB)
	.type	GL_PREFIX(VertexAttrib4NbvARB), @function
GL_PREFIX(VertexAttrib4NbvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3840(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3840(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3840(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3840(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4NbvARB), .-GL_PREFIX(VertexAttrib4NbvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4NivARB)
	.type	GL_PREFIX(VertexAttrib4NivARB), @function
GL_PREFIX(VertexAttrib4NivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3848(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3848(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3848(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3848(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4NivARB), .-GL_PREFIX(VertexAttrib4NivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4NsvARB)
	.type	GL_PREFIX(VertexAttrib4NsvARB), @function
GL_PREFIX(VertexAttrib4NsvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3856(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3856(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3856(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3856(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4NsvARB), .-GL_PREFIX(VertexAttrib4NsvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4NubARB)
	.type	GL_PREFIX(VertexAttrib4NubARB), @function
GL_PREFIX(VertexAttrib4NubARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3864(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3864(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3864(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3864(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4NubARB), .-GL_PREFIX(VertexAttrib4NubARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4NubvARB)
	.type	GL_PREFIX(VertexAttrib4NubvARB), @function
GL_PREFIX(VertexAttrib4NubvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3872(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3872(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3872(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3872(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4NubvARB), .-GL_PREFIX(VertexAttrib4NubvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4NuivARB)
	.type	GL_PREFIX(VertexAttrib4NuivARB), @function
GL_PREFIX(VertexAttrib4NuivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3880(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3880(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3880(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3880(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4NuivARB), .-GL_PREFIX(VertexAttrib4NuivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4NusvARB)
	.type	GL_PREFIX(VertexAttrib4NusvARB), @function
GL_PREFIX(VertexAttrib4NusvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3888(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3888(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3888(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3888(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4NusvARB), .-GL_PREFIX(VertexAttrib4NusvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4bvARB)
	.type	GL_PREFIX(VertexAttrib4bvARB), @function
GL_PREFIX(VertexAttrib4bvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3896(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3896(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3896(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3896(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4bvARB), .-GL_PREFIX(VertexAttrib4bvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4dARB)
	.type	GL_PREFIX(VertexAttrib4dARB), @function
GL_PREFIX(VertexAttrib4dARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3904(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	movq	%xmm3, 32(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	32(%rsp), %xmm3
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3904(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3904(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	movq	%xmm3, 32(%rsp)
	call	_glapi_get_dispatch
	movq	32(%rsp), %xmm3
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3904(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4dARB), .-GL_PREFIX(VertexAttrib4dARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4dvARB)
	.type	GL_PREFIX(VertexAttrib4dvARB), @function
GL_PREFIX(VertexAttrib4dvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3912(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3912(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3912(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3912(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4dvARB), .-GL_PREFIX(VertexAttrib4dvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4fARB)
	.type	GL_PREFIX(VertexAttrib4fARB), @function
GL_PREFIX(VertexAttrib4fARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3920(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	movq	%xmm3, 32(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	32(%rsp), %xmm3
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3920(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3920(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	movq	%xmm3, 32(%rsp)
	call	_glapi_get_dispatch
	movq	32(%rsp), %xmm3
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	3920(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4fARB), .-GL_PREFIX(VertexAttrib4fARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4fvARB)
	.type	GL_PREFIX(VertexAttrib4fvARB), @function
GL_PREFIX(VertexAttrib4fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3928(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3928(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3928(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3928(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4fvARB), .-GL_PREFIX(VertexAttrib4fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4ivARB)
	.type	GL_PREFIX(VertexAttrib4ivARB), @function
GL_PREFIX(VertexAttrib4ivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3936(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3936(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3936(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3936(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4ivARB), .-GL_PREFIX(VertexAttrib4ivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4sARB)
	.type	GL_PREFIX(VertexAttrib4sARB), @function
GL_PREFIX(VertexAttrib4sARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3944(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3944(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3944(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3944(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4sARB), .-GL_PREFIX(VertexAttrib4sARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4svARB)
	.type	GL_PREFIX(VertexAttrib4svARB), @function
GL_PREFIX(VertexAttrib4svARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3952(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3952(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3952(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3952(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4svARB), .-GL_PREFIX(VertexAttrib4svARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4ubvARB)
	.type	GL_PREFIX(VertexAttrib4ubvARB), @function
GL_PREFIX(VertexAttrib4ubvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3960(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3960(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3960(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3960(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4ubvARB), .-GL_PREFIX(VertexAttrib4ubvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4uivARB)
	.type	GL_PREFIX(VertexAttrib4uivARB), @function
GL_PREFIX(VertexAttrib4uivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3968(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3968(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3968(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3968(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4uivARB), .-GL_PREFIX(VertexAttrib4uivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4usvARB)
	.type	GL_PREFIX(VertexAttrib4usvARB), @function
GL_PREFIX(VertexAttrib4usvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3976(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3976(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3976(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3976(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4usvARB), .-GL_PREFIX(VertexAttrib4usvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribPointerARB)
	.type	GL_PREFIX(VertexAttribPointerARB), @function
GL_PREFIX(VertexAttribPointerARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3984(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3984(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3984(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3984(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribPointerARB), .-GL_PREFIX(VertexAttribPointerARB)

	.p2align	4,,15
	.globl	GL_PREFIX(BindBufferARB)
	.type	GL_PREFIX(BindBufferARB), @function
GL_PREFIX(BindBufferARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3992(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3992(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3992(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3992(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BindBufferARB), .-GL_PREFIX(BindBufferARB)

	.p2align	4,,15
	.globl	GL_PREFIX(BufferDataARB)
	.type	GL_PREFIX(BufferDataARB), @function
GL_PREFIX(BufferDataARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4000(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4000(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4000(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4000(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BufferDataARB), .-GL_PREFIX(BufferDataARB)

	.p2align	4,,15
	.globl	GL_PREFIX(BufferSubDataARB)
	.type	GL_PREFIX(BufferSubDataARB), @function
GL_PREFIX(BufferSubDataARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4008(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4008(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4008(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4008(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BufferSubDataARB), .-GL_PREFIX(BufferSubDataARB)

	.p2align	4,,15
	.globl	GL_PREFIX(DeleteBuffersARB)
	.type	GL_PREFIX(DeleteBuffersARB), @function
GL_PREFIX(DeleteBuffersARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4016(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4016(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4016(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4016(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DeleteBuffersARB), .-GL_PREFIX(DeleteBuffersARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GenBuffersARB)
	.type	GL_PREFIX(GenBuffersARB), @function
GL_PREFIX(GenBuffersARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4024(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4024(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4024(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4024(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GenBuffersARB), .-GL_PREFIX(GenBuffersARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetBufferParameterivARB)
	.type	GL_PREFIX(GetBufferParameterivARB), @function
GL_PREFIX(GetBufferParameterivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4032(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4032(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4032(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4032(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetBufferParameterivARB), .-GL_PREFIX(GetBufferParameterivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetBufferPointervARB)
	.type	GL_PREFIX(GetBufferPointervARB), @function
GL_PREFIX(GetBufferPointervARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4040(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4040(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4040(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4040(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetBufferPointervARB), .-GL_PREFIX(GetBufferPointervARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetBufferSubDataARB)
	.type	GL_PREFIX(GetBufferSubDataARB), @function
GL_PREFIX(GetBufferSubDataARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4048(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4048(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4048(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4048(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetBufferSubDataARB), .-GL_PREFIX(GetBufferSubDataARB)

	.p2align	4,,15
	.globl	GL_PREFIX(IsBufferARB)
	.type	GL_PREFIX(IsBufferARB), @function
GL_PREFIX(IsBufferARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4056(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4056(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4056(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4056(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(IsBufferARB), .-GL_PREFIX(IsBufferARB)

	.p2align	4,,15
	.globl	GL_PREFIX(MapBufferARB)
	.type	GL_PREFIX(MapBufferARB), @function
GL_PREFIX(MapBufferARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4064(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4064(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4064(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4064(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MapBufferARB), .-GL_PREFIX(MapBufferARB)

	.p2align	4,,15
	.globl	GL_PREFIX(UnmapBufferARB)
	.type	GL_PREFIX(UnmapBufferARB), @function
GL_PREFIX(UnmapBufferARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4072(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4072(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4072(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4072(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(UnmapBufferARB), .-GL_PREFIX(UnmapBufferARB)

	.p2align	4,,15
	.globl	GL_PREFIX(BeginQueryARB)
	.type	GL_PREFIX(BeginQueryARB), @function
GL_PREFIX(BeginQueryARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4080(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4080(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4080(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4080(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BeginQueryARB), .-GL_PREFIX(BeginQueryARB)

	.p2align	4,,15
	.globl	GL_PREFIX(DeleteQueriesARB)
	.type	GL_PREFIX(DeleteQueriesARB), @function
GL_PREFIX(DeleteQueriesARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4088(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4088(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4088(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4088(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DeleteQueriesARB), .-GL_PREFIX(DeleteQueriesARB)

	.p2align	4,,15
	.globl	GL_PREFIX(EndQueryARB)
	.type	GL_PREFIX(EndQueryARB), @function
GL_PREFIX(EndQueryARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4096(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4096(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4096(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4096(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EndQueryARB), .-GL_PREFIX(EndQueryARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GenQueriesARB)
	.type	GL_PREFIX(GenQueriesARB), @function
GL_PREFIX(GenQueriesARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4104(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4104(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4104(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4104(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GenQueriesARB), .-GL_PREFIX(GenQueriesARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetQueryObjectivARB)
	.type	GL_PREFIX(GetQueryObjectivARB), @function
GL_PREFIX(GetQueryObjectivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4112(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4112(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4112(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4112(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetQueryObjectivARB), .-GL_PREFIX(GetQueryObjectivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetQueryObjectuivARB)
	.type	GL_PREFIX(GetQueryObjectuivARB), @function
GL_PREFIX(GetQueryObjectuivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4120(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4120(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4120(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4120(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetQueryObjectuivARB), .-GL_PREFIX(GetQueryObjectuivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetQueryivARB)
	.type	GL_PREFIX(GetQueryivARB), @function
GL_PREFIX(GetQueryivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4128(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4128(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4128(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4128(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetQueryivARB), .-GL_PREFIX(GetQueryivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(IsQueryARB)
	.type	GL_PREFIX(IsQueryARB), @function
GL_PREFIX(IsQueryARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4136(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4136(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4136(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4136(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(IsQueryARB), .-GL_PREFIX(IsQueryARB)

	.p2align	4,,15
	.globl	GL_PREFIX(AttachObjectARB)
	.type	GL_PREFIX(AttachObjectARB), @function
GL_PREFIX(AttachObjectARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4144(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4144(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4144(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4144(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(AttachObjectARB), .-GL_PREFIX(AttachObjectARB)

	.p2align	4,,15
	.globl	GL_PREFIX(CompileShaderARB)
	.type	GL_PREFIX(CompileShaderARB), @function
GL_PREFIX(CompileShaderARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4152(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4152(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4152(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4152(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CompileShaderARB), .-GL_PREFIX(CompileShaderARB)

	.p2align	4,,15
	.globl	GL_PREFIX(CreateProgramObjectARB)
	.type	GL_PREFIX(CreateProgramObjectARB), @function
GL_PREFIX(CreateProgramObjectARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4160(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	4160(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4160(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	4160(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CreateProgramObjectARB), .-GL_PREFIX(CreateProgramObjectARB)

	.p2align	4,,15
	.globl	GL_PREFIX(CreateShaderObjectARB)
	.type	GL_PREFIX(CreateShaderObjectARB), @function
GL_PREFIX(CreateShaderObjectARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4168(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4168(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4168(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4168(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CreateShaderObjectARB), .-GL_PREFIX(CreateShaderObjectARB)

	.p2align	4,,15
	.globl	GL_PREFIX(DeleteObjectARB)
	.type	GL_PREFIX(DeleteObjectARB), @function
GL_PREFIX(DeleteObjectARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4176(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4176(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4176(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4176(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DeleteObjectARB), .-GL_PREFIX(DeleteObjectARB)

	.p2align	4,,15
	.globl	GL_PREFIX(DetachObjectARB)
	.type	GL_PREFIX(DetachObjectARB), @function
GL_PREFIX(DetachObjectARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4184(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4184(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4184(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4184(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DetachObjectARB), .-GL_PREFIX(DetachObjectARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetActiveUniformARB)
	.type	GL_PREFIX(GetActiveUniformARB), @function
GL_PREFIX(GetActiveUniformARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4192(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4192(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4192(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4192(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetActiveUniformARB), .-GL_PREFIX(GetActiveUniformARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetAttachedObjectsARB)
	.type	GL_PREFIX(GetAttachedObjectsARB), @function
GL_PREFIX(GetAttachedObjectsARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4200(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4200(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4200(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4200(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetAttachedObjectsARB), .-GL_PREFIX(GetAttachedObjectsARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetHandleARB)
	.type	GL_PREFIX(GetHandleARB), @function
GL_PREFIX(GetHandleARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4208(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4208(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4208(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4208(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetHandleARB), .-GL_PREFIX(GetHandleARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetInfoLogARB)
	.type	GL_PREFIX(GetInfoLogARB), @function
GL_PREFIX(GetInfoLogARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4216(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4216(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4216(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4216(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetInfoLogARB), .-GL_PREFIX(GetInfoLogARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetObjectParameterfvARB)
	.type	GL_PREFIX(GetObjectParameterfvARB), @function
GL_PREFIX(GetObjectParameterfvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4224(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4224(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4224(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4224(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetObjectParameterfvARB), .-GL_PREFIX(GetObjectParameterfvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetObjectParameterivARB)
	.type	GL_PREFIX(GetObjectParameterivARB), @function
GL_PREFIX(GetObjectParameterivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4232(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4232(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4232(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4232(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetObjectParameterivARB), .-GL_PREFIX(GetObjectParameterivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetShaderSourceARB)
	.type	GL_PREFIX(GetShaderSourceARB), @function
GL_PREFIX(GetShaderSourceARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4240(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4240(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4240(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4240(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetShaderSourceARB), .-GL_PREFIX(GetShaderSourceARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetUniformLocationARB)
	.type	GL_PREFIX(GetUniformLocationARB), @function
GL_PREFIX(GetUniformLocationARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4248(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4248(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4248(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4248(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetUniformLocationARB), .-GL_PREFIX(GetUniformLocationARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetUniformfvARB)
	.type	GL_PREFIX(GetUniformfvARB), @function
GL_PREFIX(GetUniformfvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4256(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4256(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4256(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4256(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetUniformfvARB), .-GL_PREFIX(GetUniformfvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetUniformivARB)
	.type	GL_PREFIX(GetUniformivARB), @function
GL_PREFIX(GetUniformivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4264(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4264(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4264(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4264(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetUniformivARB), .-GL_PREFIX(GetUniformivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(LinkProgramARB)
	.type	GL_PREFIX(LinkProgramARB), @function
GL_PREFIX(LinkProgramARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4272(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4272(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4272(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4272(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LinkProgramARB), .-GL_PREFIX(LinkProgramARB)

	.p2align	4,,15
	.globl	GL_PREFIX(ShaderSourceARB)
	.type	GL_PREFIX(ShaderSourceARB), @function
GL_PREFIX(ShaderSourceARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4280(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4280(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4280(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4280(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ShaderSourceARB), .-GL_PREFIX(ShaderSourceARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform1fARB)
	.type	GL_PREFIX(Uniform1fARB), @function
GL_PREFIX(Uniform1fARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4288(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4288(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4288(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4288(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform1fARB), .-GL_PREFIX(Uniform1fARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform1fvARB)
	.type	GL_PREFIX(Uniform1fvARB), @function
GL_PREFIX(Uniform1fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4296(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4296(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4296(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4296(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform1fvARB), .-GL_PREFIX(Uniform1fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform1iARB)
	.type	GL_PREFIX(Uniform1iARB), @function
GL_PREFIX(Uniform1iARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4304(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4304(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4304(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4304(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform1iARB), .-GL_PREFIX(Uniform1iARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform1ivARB)
	.type	GL_PREFIX(Uniform1ivARB), @function
GL_PREFIX(Uniform1ivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4312(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4312(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4312(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4312(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform1ivARB), .-GL_PREFIX(Uniform1ivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform2fARB)
	.type	GL_PREFIX(Uniform2fARB), @function
GL_PREFIX(Uniform2fARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4320(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4320(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4320(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4320(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform2fARB), .-GL_PREFIX(Uniform2fARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform2fvARB)
	.type	GL_PREFIX(Uniform2fvARB), @function
GL_PREFIX(Uniform2fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4328(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4328(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4328(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4328(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform2fvARB), .-GL_PREFIX(Uniform2fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform2iARB)
	.type	GL_PREFIX(Uniform2iARB), @function
GL_PREFIX(Uniform2iARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4336(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4336(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4336(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4336(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform2iARB), .-GL_PREFIX(Uniform2iARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform2ivARB)
	.type	GL_PREFIX(Uniform2ivARB), @function
GL_PREFIX(Uniform2ivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4344(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4344(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4344(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4344(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform2ivARB), .-GL_PREFIX(Uniform2ivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform3fARB)
	.type	GL_PREFIX(Uniform3fARB), @function
GL_PREFIX(Uniform3fARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4352(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	4352(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4352(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	4352(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform3fARB), .-GL_PREFIX(Uniform3fARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform3fvARB)
	.type	GL_PREFIX(Uniform3fvARB), @function
GL_PREFIX(Uniform3fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4360(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4360(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4360(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4360(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform3fvARB), .-GL_PREFIX(Uniform3fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform3iARB)
	.type	GL_PREFIX(Uniform3iARB), @function
GL_PREFIX(Uniform3iARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4368(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4368(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4368(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4368(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform3iARB), .-GL_PREFIX(Uniform3iARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform3ivARB)
	.type	GL_PREFIX(Uniform3ivARB), @function
GL_PREFIX(Uniform3ivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4376(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4376(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4376(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4376(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform3ivARB), .-GL_PREFIX(Uniform3ivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform4fARB)
	.type	GL_PREFIX(Uniform4fARB), @function
GL_PREFIX(Uniform4fARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4384(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	movq	%xmm3, 32(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	32(%rsp), %xmm3
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	4384(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4384(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	movq	%xmm3, 32(%rsp)
	call	_glapi_get_dispatch
	movq	32(%rsp), %xmm3
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	4384(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform4fARB), .-GL_PREFIX(Uniform4fARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform4fvARB)
	.type	GL_PREFIX(Uniform4fvARB), @function
GL_PREFIX(Uniform4fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4392(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4392(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4392(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4392(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform4fvARB), .-GL_PREFIX(Uniform4fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform4iARB)
	.type	GL_PREFIX(Uniform4iARB), @function
GL_PREFIX(Uniform4iARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4400(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4400(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4400(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4400(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform4iARB), .-GL_PREFIX(Uniform4iARB)

	.p2align	4,,15
	.globl	GL_PREFIX(Uniform4ivARB)
	.type	GL_PREFIX(Uniform4ivARB), @function
GL_PREFIX(Uniform4ivARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4408(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4408(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4408(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4408(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(Uniform4ivARB), .-GL_PREFIX(Uniform4ivARB)

	.p2align	4,,15
	.globl	GL_PREFIX(UniformMatrix2fvARB)
	.type	GL_PREFIX(UniformMatrix2fvARB), @function
GL_PREFIX(UniformMatrix2fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4416(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4416(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4416(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4416(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(UniformMatrix2fvARB), .-GL_PREFIX(UniformMatrix2fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(UniformMatrix3fvARB)
	.type	GL_PREFIX(UniformMatrix3fvARB), @function
GL_PREFIX(UniformMatrix3fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4424(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4424(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4424(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4424(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(UniformMatrix3fvARB), .-GL_PREFIX(UniformMatrix3fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(UniformMatrix4fvARB)
	.type	GL_PREFIX(UniformMatrix4fvARB), @function
GL_PREFIX(UniformMatrix4fvARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4432(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4432(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4432(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4432(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(UniformMatrix4fvARB), .-GL_PREFIX(UniformMatrix4fvARB)

	.p2align	4,,15
	.globl	GL_PREFIX(UseProgramObjectARB)
	.type	GL_PREFIX(UseProgramObjectARB), @function
GL_PREFIX(UseProgramObjectARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4440(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4440(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4440(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4440(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(UseProgramObjectARB), .-GL_PREFIX(UseProgramObjectARB)

	.p2align	4,,15
	.globl	GL_PREFIX(ValidateProgramARB)
	.type	GL_PREFIX(ValidateProgramARB), @function
GL_PREFIX(ValidateProgramARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4448(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4448(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4448(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4448(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ValidateProgramARB), .-GL_PREFIX(ValidateProgramARB)

	.p2align	4,,15
	.globl	GL_PREFIX(BindAttribLocationARB)
	.type	GL_PREFIX(BindAttribLocationARB), @function
GL_PREFIX(BindAttribLocationARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4456(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4456(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4456(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4456(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BindAttribLocationARB), .-GL_PREFIX(BindAttribLocationARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetActiveAttribARB)
	.type	GL_PREFIX(GetActiveAttribARB), @function
GL_PREFIX(GetActiveAttribARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4464(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4464(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4464(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4464(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetActiveAttribARB), .-GL_PREFIX(GetActiveAttribARB)

	.p2align	4,,15
	.globl	GL_PREFIX(GetAttribLocationARB)
	.type	GL_PREFIX(GetAttribLocationARB), @function
GL_PREFIX(GetAttribLocationARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4472(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4472(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4472(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4472(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetAttribLocationARB), .-GL_PREFIX(GetAttribLocationARB)

	.p2align	4,,15
	.globl	GL_PREFIX(DrawBuffersARB)
	.type	GL_PREFIX(DrawBuffersARB), @function
GL_PREFIX(DrawBuffersARB):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4480(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4480(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4480(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4480(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DrawBuffersARB), .-GL_PREFIX(DrawBuffersARB)

	.p2align	4,,15
	.globl	GL_PREFIX(PolygonOffsetEXT)
	.type	GL_PREFIX(PolygonOffsetEXT), @function
GL_PREFIX(PolygonOffsetEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4488(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	4488(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4488(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	4488(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PolygonOffsetEXT), .-GL_PREFIX(PolygonOffsetEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_562)
	.type	GL_PREFIX(_dispatch_stub_562), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_562))
GL_PREFIX(_dispatch_stub_562):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4496(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4496(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4496(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4496(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_562), .-GL_PREFIX(_dispatch_stub_562)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_563)
	.type	GL_PREFIX(_dispatch_stub_563), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_563))
GL_PREFIX(_dispatch_stub_563):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4504(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4504(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4504(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4504(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_563), .-GL_PREFIX(_dispatch_stub_563)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_564)
	.type	GL_PREFIX(_dispatch_stub_564), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_564))
GL_PREFIX(_dispatch_stub_564):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4512(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4512(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4512(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4512(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_564), .-GL_PREFIX(_dispatch_stub_564)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_565)
	.type	GL_PREFIX(_dispatch_stub_565), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_565))
GL_PREFIX(_dispatch_stub_565):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4520(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4520(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4520(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4520(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_565), .-GL_PREFIX(_dispatch_stub_565)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_566)
	.type	GL_PREFIX(_dispatch_stub_566), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_566))
GL_PREFIX(_dispatch_stub_566):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4528(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4528(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4528(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4528(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_566), .-GL_PREFIX(_dispatch_stub_566)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_567)
	.type	GL_PREFIX(_dispatch_stub_567), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_567))
GL_PREFIX(_dispatch_stub_567):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4536(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4536(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4536(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4536(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_567), .-GL_PREFIX(_dispatch_stub_567)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_568)
	.type	GL_PREFIX(_dispatch_stub_568), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_568))
GL_PREFIX(_dispatch_stub_568):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4544(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4544(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4544(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4544(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_568), .-GL_PREFIX(_dispatch_stub_568)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_569)
	.type	GL_PREFIX(_dispatch_stub_569), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_569))
GL_PREFIX(_dispatch_stub_569):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4552(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4552(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4552(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4552(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_569), .-GL_PREFIX(_dispatch_stub_569)

	.p2align	4,,15
	.globl	GL_PREFIX(ColorPointerEXT)
	.type	GL_PREFIX(ColorPointerEXT), @function
GL_PREFIX(ColorPointerEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4560(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4560(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4560(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4560(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ColorPointerEXT), .-GL_PREFIX(ColorPointerEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(EdgeFlagPointerEXT)
	.type	GL_PREFIX(EdgeFlagPointerEXT), @function
GL_PREFIX(EdgeFlagPointerEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4568(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4568(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4568(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4568(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EdgeFlagPointerEXT), .-GL_PREFIX(EdgeFlagPointerEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(IndexPointerEXT)
	.type	GL_PREFIX(IndexPointerEXT), @function
GL_PREFIX(IndexPointerEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4576(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4576(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4576(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4576(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(IndexPointerEXT), .-GL_PREFIX(IndexPointerEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(NormalPointerEXT)
	.type	GL_PREFIX(NormalPointerEXT), @function
GL_PREFIX(NormalPointerEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4584(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4584(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4584(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4584(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(NormalPointerEXT), .-GL_PREFIX(NormalPointerEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(TexCoordPointerEXT)
	.type	GL_PREFIX(TexCoordPointerEXT), @function
GL_PREFIX(TexCoordPointerEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4592(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4592(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4592(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4592(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TexCoordPointerEXT), .-GL_PREFIX(TexCoordPointerEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexPointerEXT)
	.type	GL_PREFIX(VertexPointerEXT), @function
GL_PREFIX(VertexPointerEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4600(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4600(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4600(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4600(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexPointerEXT), .-GL_PREFIX(VertexPointerEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(PointParameterfEXT)
	.type	GL_PREFIX(PointParameterfEXT), @function
GL_PREFIX(PointParameterfEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4608(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4608(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4608(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4608(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PointParameterfEXT), .-GL_PREFIX(PointParameterfEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(PointParameterfvEXT)
	.type	GL_PREFIX(PointParameterfvEXT), @function
GL_PREFIX(PointParameterfvEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4616(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4616(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4616(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4616(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PointParameterfvEXT), .-GL_PREFIX(PointParameterfvEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(LockArraysEXT)
	.type	GL_PREFIX(LockArraysEXT), @function
GL_PREFIX(LockArraysEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4624(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4624(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4624(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4624(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LockArraysEXT), .-GL_PREFIX(LockArraysEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(UnlockArraysEXT)
	.type	GL_PREFIX(UnlockArraysEXT), @function
GL_PREFIX(UnlockArraysEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4632(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	4632(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4632(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	4632(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(UnlockArraysEXT), .-GL_PREFIX(UnlockArraysEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_580)
	.type	GL_PREFIX(_dispatch_stub_580), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_580))
GL_PREFIX(_dispatch_stub_580):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4640(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4640(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4640(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4640(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_580), .-GL_PREFIX(_dispatch_stub_580)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_581)
	.type	GL_PREFIX(_dispatch_stub_581), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_581))
GL_PREFIX(_dispatch_stub_581):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4648(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4648(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4648(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4648(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_581), .-GL_PREFIX(_dispatch_stub_581)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3bEXT)
	.type	GL_PREFIX(SecondaryColor3bEXT), @function
GL_PREFIX(SecondaryColor3bEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4656(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4656(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4656(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4656(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3bEXT), .-GL_PREFIX(SecondaryColor3bEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3bvEXT)
	.type	GL_PREFIX(SecondaryColor3bvEXT), @function
GL_PREFIX(SecondaryColor3bvEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4664(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4664(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4664(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4664(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3bvEXT), .-GL_PREFIX(SecondaryColor3bvEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3dEXT)
	.type	GL_PREFIX(SecondaryColor3dEXT), @function
GL_PREFIX(SecondaryColor3dEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4672(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	4672(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4672(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	4672(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3dEXT), .-GL_PREFIX(SecondaryColor3dEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3dvEXT)
	.type	GL_PREFIX(SecondaryColor3dvEXT), @function
GL_PREFIX(SecondaryColor3dvEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4680(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4680(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4680(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4680(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3dvEXT), .-GL_PREFIX(SecondaryColor3dvEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3fEXT)
	.type	GL_PREFIX(SecondaryColor3fEXT), @function
GL_PREFIX(SecondaryColor3fEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4688(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	4688(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4688(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	4688(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3fEXT), .-GL_PREFIX(SecondaryColor3fEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3fvEXT)
	.type	GL_PREFIX(SecondaryColor3fvEXT), @function
GL_PREFIX(SecondaryColor3fvEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4696(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4696(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4696(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4696(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3fvEXT), .-GL_PREFIX(SecondaryColor3fvEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3iEXT)
	.type	GL_PREFIX(SecondaryColor3iEXT), @function
GL_PREFIX(SecondaryColor3iEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4704(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4704(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4704(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4704(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3iEXT), .-GL_PREFIX(SecondaryColor3iEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3ivEXT)
	.type	GL_PREFIX(SecondaryColor3ivEXT), @function
GL_PREFIX(SecondaryColor3ivEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4712(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4712(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4712(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4712(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3ivEXT), .-GL_PREFIX(SecondaryColor3ivEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3sEXT)
	.type	GL_PREFIX(SecondaryColor3sEXT), @function
GL_PREFIX(SecondaryColor3sEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4720(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4720(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4720(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4720(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3sEXT), .-GL_PREFIX(SecondaryColor3sEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3svEXT)
	.type	GL_PREFIX(SecondaryColor3svEXT), @function
GL_PREFIX(SecondaryColor3svEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4728(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4728(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4728(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4728(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3svEXT), .-GL_PREFIX(SecondaryColor3svEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3ubEXT)
	.type	GL_PREFIX(SecondaryColor3ubEXT), @function
GL_PREFIX(SecondaryColor3ubEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4736(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4736(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4736(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4736(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3ubEXT), .-GL_PREFIX(SecondaryColor3ubEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3ubvEXT)
	.type	GL_PREFIX(SecondaryColor3ubvEXT), @function
GL_PREFIX(SecondaryColor3ubvEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4744(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4744(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4744(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4744(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3ubvEXT), .-GL_PREFIX(SecondaryColor3ubvEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3uiEXT)
	.type	GL_PREFIX(SecondaryColor3uiEXT), @function
GL_PREFIX(SecondaryColor3uiEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4752(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4752(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4752(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4752(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3uiEXT), .-GL_PREFIX(SecondaryColor3uiEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3uivEXT)
	.type	GL_PREFIX(SecondaryColor3uivEXT), @function
GL_PREFIX(SecondaryColor3uivEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4760(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4760(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4760(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4760(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3uivEXT), .-GL_PREFIX(SecondaryColor3uivEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3usEXT)
	.type	GL_PREFIX(SecondaryColor3usEXT), @function
GL_PREFIX(SecondaryColor3usEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4768(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4768(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4768(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4768(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3usEXT), .-GL_PREFIX(SecondaryColor3usEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColor3usvEXT)
	.type	GL_PREFIX(SecondaryColor3usvEXT), @function
GL_PREFIX(SecondaryColor3usvEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4776(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4776(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4776(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4776(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColor3usvEXT), .-GL_PREFIX(SecondaryColor3usvEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(SecondaryColorPointerEXT)
	.type	GL_PREFIX(SecondaryColorPointerEXT), @function
GL_PREFIX(SecondaryColorPointerEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4784(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4784(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4784(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4784(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SecondaryColorPointerEXT), .-GL_PREFIX(SecondaryColorPointerEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiDrawArraysEXT)
	.type	GL_PREFIX(MultiDrawArraysEXT), @function
GL_PREFIX(MultiDrawArraysEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4792(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4792(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4792(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4792(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiDrawArraysEXT), .-GL_PREFIX(MultiDrawArraysEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(MultiDrawElementsEXT)
	.type	GL_PREFIX(MultiDrawElementsEXT), @function
GL_PREFIX(MultiDrawElementsEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4800(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4800(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4800(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4800(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(MultiDrawElementsEXT), .-GL_PREFIX(MultiDrawElementsEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(FogCoordPointerEXT)
	.type	GL_PREFIX(FogCoordPointerEXT), @function
GL_PREFIX(FogCoordPointerEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4808(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4808(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4808(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4808(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(FogCoordPointerEXT), .-GL_PREFIX(FogCoordPointerEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(FogCoorddEXT)
	.type	GL_PREFIX(FogCoorddEXT), @function
GL_PREFIX(FogCoorddEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4816(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	4816(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4816(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	4816(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(FogCoorddEXT), .-GL_PREFIX(FogCoorddEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(FogCoorddvEXT)
	.type	GL_PREFIX(FogCoorddvEXT), @function
GL_PREFIX(FogCoorddvEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4824(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4824(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4824(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4824(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(FogCoorddvEXT), .-GL_PREFIX(FogCoorddvEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(FogCoordfEXT)
	.type	GL_PREFIX(FogCoordfEXT), @function
GL_PREFIX(FogCoordfEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4832(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	4832(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4832(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	4832(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(FogCoordfEXT), .-GL_PREFIX(FogCoordfEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(FogCoordfvEXT)
	.type	GL_PREFIX(FogCoordfvEXT), @function
GL_PREFIX(FogCoordfvEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4840(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4840(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4840(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4840(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(FogCoordfvEXT), .-GL_PREFIX(FogCoordfvEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_606)
	.type	GL_PREFIX(_dispatch_stub_606), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_606))
GL_PREFIX(_dispatch_stub_606):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4848(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4848(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4848(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4848(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_606), .-GL_PREFIX(_dispatch_stub_606)

	.p2align	4,,15
	.globl	GL_PREFIX(BlendFuncSeparateEXT)
	.type	GL_PREFIX(BlendFuncSeparateEXT), @function
GL_PREFIX(BlendFuncSeparateEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4856(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4856(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4856(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4856(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BlendFuncSeparateEXT), .-GL_PREFIX(BlendFuncSeparateEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(FlushVertexArrayRangeNV)
	.type	GL_PREFIX(FlushVertexArrayRangeNV), @function
GL_PREFIX(FlushVertexArrayRangeNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4864(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	4864(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4864(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	4864(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(FlushVertexArrayRangeNV), .-GL_PREFIX(FlushVertexArrayRangeNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexArrayRangeNV)
	.type	GL_PREFIX(VertexArrayRangeNV), @function
GL_PREFIX(VertexArrayRangeNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4872(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4872(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4872(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4872(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexArrayRangeNV), .-GL_PREFIX(VertexArrayRangeNV)

	.p2align	4,,15
	.globl	GL_PREFIX(CombinerInputNV)
	.type	GL_PREFIX(CombinerInputNV), @function
GL_PREFIX(CombinerInputNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4880(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4880(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4880(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4880(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CombinerInputNV), .-GL_PREFIX(CombinerInputNV)

	.p2align	4,,15
	.globl	GL_PREFIX(CombinerOutputNV)
	.type	GL_PREFIX(CombinerOutputNV), @function
GL_PREFIX(CombinerOutputNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4888(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4888(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4888(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4888(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CombinerOutputNV), .-GL_PREFIX(CombinerOutputNV)

	.p2align	4,,15
	.globl	GL_PREFIX(CombinerParameterfNV)
	.type	GL_PREFIX(CombinerParameterfNV), @function
GL_PREFIX(CombinerParameterfNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4896(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4896(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4896(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4896(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CombinerParameterfNV), .-GL_PREFIX(CombinerParameterfNV)

	.p2align	4,,15
	.globl	GL_PREFIX(CombinerParameterfvNV)
	.type	GL_PREFIX(CombinerParameterfvNV), @function
GL_PREFIX(CombinerParameterfvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4904(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4904(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4904(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4904(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CombinerParameterfvNV), .-GL_PREFIX(CombinerParameterfvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(CombinerParameteriNV)
	.type	GL_PREFIX(CombinerParameteriNV), @function
GL_PREFIX(CombinerParameteriNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4912(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4912(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4912(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4912(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CombinerParameteriNV), .-GL_PREFIX(CombinerParameteriNV)

	.p2align	4,,15
	.globl	GL_PREFIX(CombinerParameterivNV)
	.type	GL_PREFIX(CombinerParameterivNV), @function
GL_PREFIX(CombinerParameterivNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4920(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4920(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4920(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4920(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CombinerParameterivNV), .-GL_PREFIX(CombinerParameterivNV)

	.p2align	4,,15
	.globl	GL_PREFIX(FinalCombinerInputNV)
	.type	GL_PREFIX(FinalCombinerInputNV), @function
GL_PREFIX(FinalCombinerInputNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4928(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4928(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4928(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4928(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(FinalCombinerInputNV), .-GL_PREFIX(FinalCombinerInputNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetCombinerInputParameterfvNV)
	.type	GL_PREFIX(GetCombinerInputParameterfvNV), @function
GL_PREFIX(GetCombinerInputParameterfvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4936(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4936(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4936(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4936(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetCombinerInputParameterfvNV), .-GL_PREFIX(GetCombinerInputParameterfvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetCombinerInputParameterivNV)
	.type	GL_PREFIX(GetCombinerInputParameterivNV), @function
GL_PREFIX(GetCombinerInputParameterivNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4944(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4944(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4944(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4944(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetCombinerInputParameterivNV), .-GL_PREFIX(GetCombinerInputParameterivNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetCombinerOutputParameterfvNV)
	.type	GL_PREFIX(GetCombinerOutputParameterfvNV), @function
GL_PREFIX(GetCombinerOutputParameterfvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4952(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4952(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4952(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4952(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetCombinerOutputParameterfvNV), .-GL_PREFIX(GetCombinerOutputParameterfvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetCombinerOutputParameterivNV)
	.type	GL_PREFIX(GetCombinerOutputParameterivNV), @function
GL_PREFIX(GetCombinerOutputParameterivNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4960(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4960(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4960(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4960(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetCombinerOutputParameterivNV), .-GL_PREFIX(GetCombinerOutputParameterivNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetFinalCombinerInputParameterfvNV)
	.type	GL_PREFIX(GetFinalCombinerInputParameterfvNV), @function
GL_PREFIX(GetFinalCombinerInputParameterfvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4968(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4968(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4968(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4968(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetFinalCombinerInputParameterfvNV), .-GL_PREFIX(GetFinalCombinerInputParameterfvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetFinalCombinerInputParameterivNV)
	.type	GL_PREFIX(GetFinalCombinerInputParameterivNV), @function
GL_PREFIX(GetFinalCombinerInputParameterivNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4976(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4976(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4976(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4976(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetFinalCombinerInputParameterivNV), .-GL_PREFIX(GetFinalCombinerInputParameterivNV)

	.p2align	4,,15
	.globl	GL_PREFIX(ResizeBuffersMESA)
	.type	GL_PREFIX(ResizeBuffersMESA), @function
GL_PREFIX(ResizeBuffersMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4984(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	4984(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4984(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	4984(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ResizeBuffersMESA), .-GL_PREFIX(ResizeBuffersMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos2dMESA)
	.type	GL_PREFIX(WindowPos2dMESA), @function
GL_PREFIX(WindowPos2dMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4992(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	4992(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4992(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	4992(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos2dMESA), .-GL_PREFIX(WindowPos2dMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos2dvMESA)
	.type	GL_PREFIX(WindowPos2dvMESA), @function
GL_PREFIX(WindowPos2dvMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5000(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5000(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5000(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5000(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos2dvMESA), .-GL_PREFIX(WindowPos2dvMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos2fMESA)
	.type	GL_PREFIX(WindowPos2fMESA), @function
GL_PREFIX(WindowPos2fMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5008(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	5008(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5008(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	5008(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos2fMESA), .-GL_PREFIX(WindowPos2fMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos2fvMESA)
	.type	GL_PREFIX(WindowPos2fvMESA), @function
GL_PREFIX(WindowPos2fvMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5016(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5016(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5016(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5016(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos2fvMESA), .-GL_PREFIX(WindowPos2fvMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos2iMESA)
	.type	GL_PREFIX(WindowPos2iMESA), @function
GL_PREFIX(WindowPos2iMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5024(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5024(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5024(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5024(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos2iMESA), .-GL_PREFIX(WindowPos2iMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos2ivMESA)
	.type	GL_PREFIX(WindowPos2ivMESA), @function
GL_PREFIX(WindowPos2ivMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5032(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5032(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5032(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5032(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos2ivMESA), .-GL_PREFIX(WindowPos2ivMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos2sMESA)
	.type	GL_PREFIX(WindowPos2sMESA), @function
GL_PREFIX(WindowPos2sMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5040(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5040(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5040(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5040(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos2sMESA), .-GL_PREFIX(WindowPos2sMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos2svMESA)
	.type	GL_PREFIX(WindowPos2svMESA), @function
GL_PREFIX(WindowPos2svMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5048(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5048(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5048(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5048(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos2svMESA), .-GL_PREFIX(WindowPos2svMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos3dMESA)
	.type	GL_PREFIX(WindowPos3dMESA), @function
GL_PREFIX(WindowPos3dMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5056(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	5056(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5056(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	5056(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos3dMESA), .-GL_PREFIX(WindowPos3dMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos3dvMESA)
	.type	GL_PREFIX(WindowPos3dvMESA), @function
GL_PREFIX(WindowPos3dvMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5064(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5064(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5064(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5064(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos3dvMESA), .-GL_PREFIX(WindowPos3dvMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos3fMESA)
	.type	GL_PREFIX(WindowPos3fMESA), @function
GL_PREFIX(WindowPos3fMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5072(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	5072(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5072(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	5072(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos3fMESA), .-GL_PREFIX(WindowPos3fMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos3fvMESA)
	.type	GL_PREFIX(WindowPos3fvMESA), @function
GL_PREFIX(WindowPos3fvMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5080(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5080(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5080(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5080(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos3fvMESA), .-GL_PREFIX(WindowPos3fvMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos3iMESA)
	.type	GL_PREFIX(WindowPos3iMESA), @function
GL_PREFIX(WindowPos3iMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5088(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5088(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5088(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5088(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos3iMESA), .-GL_PREFIX(WindowPos3iMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos3ivMESA)
	.type	GL_PREFIX(WindowPos3ivMESA), @function
GL_PREFIX(WindowPos3ivMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5096(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5096(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5096(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5096(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos3ivMESA), .-GL_PREFIX(WindowPos3ivMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos3sMESA)
	.type	GL_PREFIX(WindowPos3sMESA), @function
GL_PREFIX(WindowPos3sMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5104(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5104(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5104(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5104(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos3sMESA), .-GL_PREFIX(WindowPos3sMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos3svMESA)
	.type	GL_PREFIX(WindowPos3svMESA), @function
GL_PREFIX(WindowPos3svMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5112(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5112(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5112(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5112(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos3svMESA), .-GL_PREFIX(WindowPos3svMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos4dMESA)
	.type	GL_PREFIX(WindowPos4dMESA), @function
GL_PREFIX(WindowPos4dMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5120(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	5120(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5120(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	5120(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos4dMESA), .-GL_PREFIX(WindowPos4dMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos4dvMESA)
	.type	GL_PREFIX(WindowPos4dvMESA), @function
GL_PREFIX(WindowPos4dvMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5128(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5128(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5128(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5128(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos4dvMESA), .-GL_PREFIX(WindowPos4dvMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos4fMESA)
	.type	GL_PREFIX(WindowPos4fMESA), @function
GL_PREFIX(WindowPos4fMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5136(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	5136(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5136(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	movq	%xmm2, 16(%rsp)
	movq	%xmm3, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm3
	movq	16(%rsp), %xmm2
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$40, %rsp
	movq	5136(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos4fMESA), .-GL_PREFIX(WindowPos4fMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos4fvMESA)
	.type	GL_PREFIX(WindowPos4fvMESA), @function
GL_PREFIX(WindowPos4fvMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5144(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5144(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5144(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5144(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos4fvMESA), .-GL_PREFIX(WindowPos4fvMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos4iMESA)
	.type	GL_PREFIX(WindowPos4iMESA), @function
GL_PREFIX(WindowPos4iMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5152(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5152(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5152(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5152(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos4iMESA), .-GL_PREFIX(WindowPos4iMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos4ivMESA)
	.type	GL_PREFIX(WindowPos4ivMESA), @function
GL_PREFIX(WindowPos4ivMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5160(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5160(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5160(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5160(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos4ivMESA), .-GL_PREFIX(WindowPos4ivMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos4sMESA)
	.type	GL_PREFIX(WindowPos4sMESA), @function
GL_PREFIX(WindowPos4sMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5168(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5168(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5168(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5168(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos4sMESA), .-GL_PREFIX(WindowPos4sMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(WindowPos4svMESA)
	.type	GL_PREFIX(WindowPos4svMESA), @function
GL_PREFIX(WindowPos4svMESA):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5176(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5176(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5176(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5176(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(WindowPos4svMESA), .-GL_PREFIX(WindowPos4svMESA)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_648)
	.type	GL_PREFIX(_dispatch_stub_648), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_648))
GL_PREFIX(_dispatch_stub_648):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5184(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5184(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5184(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5184(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_648), .-GL_PREFIX(_dispatch_stub_648)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_649)
	.type	GL_PREFIX(_dispatch_stub_649), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_649))
GL_PREFIX(_dispatch_stub_649):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5192(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5192(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5192(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5192(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_649), .-GL_PREFIX(_dispatch_stub_649)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_650)
	.type	GL_PREFIX(_dispatch_stub_650), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_650))
GL_PREFIX(_dispatch_stub_650):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5200(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5200(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5200(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5200(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_650), .-GL_PREFIX(_dispatch_stub_650)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_651)
	.type	GL_PREFIX(_dispatch_stub_651), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_651))
GL_PREFIX(_dispatch_stub_651):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5208(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5208(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5208(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5208(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_651), .-GL_PREFIX(_dispatch_stub_651)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_652)
	.type	GL_PREFIX(_dispatch_stub_652), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_652))
GL_PREFIX(_dispatch_stub_652):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5216(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5216(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5216(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5216(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_652), .-GL_PREFIX(_dispatch_stub_652)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_653)
	.type	GL_PREFIX(_dispatch_stub_653), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_653))
GL_PREFIX(_dispatch_stub_653):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5224(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5224(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5224(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5224(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_653), .-GL_PREFIX(_dispatch_stub_653)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_654)
	.type	GL_PREFIX(_dispatch_stub_654), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_654))
GL_PREFIX(_dispatch_stub_654):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5232(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5232(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5232(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5232(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_654), .-GL_PREFIX(_dispatch_stub_654)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_655)
	.type	GL_PREFIX(_dispatch_stub_655), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_655))
GL_PREFIX(_dispatch_stub_655):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5240(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5240(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5240(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5240(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_655), .-GL_PREFIX(_dispatch_stub_655)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_656)
	.type	GL_PREFIX(_dispatch_stub_656), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_656))
GL_PREFIX(_dispatch_stub_656):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5248(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5248(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5248(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5248(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_656), .-GL_PREFIX(_dispatch_stub_656)

	.p2align	4,,15
	.globl	GL_PREFIX(AreProgramsResidentNV)
	.type	GL_PREFIX(AreProgramsResidentNV), @function
GL_PREFIX(AreProgramsResidentNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5256(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5256(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5256(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5256(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(AreProgramsResidentNV), .-GL_PREFIX(AreProgramsResidentNV)

	.p2align	4,,15
	.globl	GL_PREFIX(BindProgramNV)
	.type	GL_PREFIX(BindProgramNV), @function
GL_PREFIX(BindProgramNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5264(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5264(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5264(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5264(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BindProgramNV), .-GL_PREFIX(BindProgramNV)

	.p2align	4,,15
	.globl	GL_PREFIX(DeleteProgramsNV)
	.type	GL_PREFIX(DeleteProgramsNV), @function
GL_PREFIX(DeleteProgramsNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5272(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5272(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5272(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5272(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DeleteProgramsNV), .-GL_PREFIX(DeleteProgramsNV)

	.p2align	4,,15
	.globl	GL_PREFIX(ExecuteProgramNV)
	.type	GL_PREFIX(ExecuteProgramNV), @function
GL_PREFIX(ExecuteProgramNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5280(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5280(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5280(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5280(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ExecuteProgramNV), .-GL_PREFIX(ExecuteProgramNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GenProgramsNV)
	.type	GL_PREFIX(GenProgramsNV), @function
GL_PREFIX(GenProgramsNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5288(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5288(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5288(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5288(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GenProgramsNV), .-GL_PREFIX(GenProgramsNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetProgramParameterdvNV)
	.type	GL_PREFIX(GetProgramParameterdvNV), @function
GL_PREFIX(GetProgramParameterdvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5296(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5296(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5296(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5296(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetProgramParameterdvNV), .-GL_PREFIX(GetProgramParameterdvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetProgramParameterfvNV)
	.type	GL_PREFIX(GetProgramParameterfvNV), @function
GL_PREFIX(GetProgramParameterfvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5304(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5304(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5304(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5304(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetProgramParameterfvNV), .-GL_PREFIX(GetProgramParameterfvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetProgramStringNV)
	.type	GL_PREFIX(GetProgramStringNV), @function
GL_PREFIX(GetProgramStringNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5312(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5312(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5312(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5312(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetProgramStringNV), .-GL_PREFIX(GetProgramStringNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetProgramivNV)
	.type	GL_PREFIX(GetProgramivNV), @function
GL_PREFIX(GetProgramivNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5320(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5320(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5320(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5320(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetProgramivNV), .-GL_PREFIX(GetProgramivNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetTrackMatrixivNV)
	.type	GL_PREFIX(GetTrackMatrixivNV), @function
GL_PREFIX(GetTrackMatrixivNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5328(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5328(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5328(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5328(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetTrackMatrixivNV), .-GL_PREFIX(GetTrackMatrixivNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetVertexAttribPointervNV)
	.type	GL_PREFIX(GetVertexAttribPointervNV), @function
GL_PREFIX(GetVertexAttribPointervNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5336(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5336(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5336(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5336(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetVertexAttribPointervNV), .-GL_PREFIX(GetVertexAttribPointervNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetVertexAttribdvNV)
	.type	GL_PREFIX(GetVertexAttribdvNV), @function
GL_PREFIX(GetVertexAttribdvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5344(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5344(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5344(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5344(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetVertexAttribdvNV), .-GL_PREFIX(GetVertexAttribdvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetVertexAttribfvNV)
	.type	GL_PREFIX(GetVertexAttribfvNV), @function
GL_PREFIX(GetVertexAttribfvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5352(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5352(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5352(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5352(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetVertexAttribfvNV), .-GL_PREFIX(GetVertexAttribfvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetVertexAttribivNV)
	.type	GL_PREFIX(GetVertexAttribivNV), @function
GL_PREFIX(GetVertexAttribivNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5360(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5360(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5360(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5360(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetVertexAttribivNV), .-GL_PREFIX(GetVertexAttribivNV)

	.p2align	4,,15
	.globl	GL_PREFIX(IsProgramNV)
	.type	GL_PREFIX(IsProgramNV), @function
GL_PREFIX(IsProgramNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5368(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5368(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5368(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5368(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(IsProgramNV), .-GL_PREFIX(IsProgramNV)

	.p2align	4,,15
	.globl	GL_PREFIX(LoadProgramNV)
	.type	GL_PREFIX(LoadProgramNV), @function
GL_PREFIX(LoadProgramNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5376(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5376(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5376(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5376(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(LoadProgramNV), .-GL_PREFIX(LoadProgramNV)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramParameters4dvNV)
	.type	GL_PREFIX(ProgramParameters4dvNV), @function
GL_PREFIX(ProgramParameters4dvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5384(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5384(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5384(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5384(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramParameters4dvNV), .-GL_PREFIX(ProgramParameters4dvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramParameters4fvNV)
	.type	GL_PREFIX(ProgramParameters4fvNV), @function
GL_PREFIX(ProgramParameters4fvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5392(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5392(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5392(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5392(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramParameters4fvNV), .-GL_PREFIX(ProgramParameters4fvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(RequestResidentProgramsNV)
	.type	GL_PREFIX(RequestResidentProgramsNV), @function
GL_PREFIX(RequestResidentProgramsNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5400(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5400(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5400(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5400(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RequestResidentProgramsNV), .-GL_PREFIX(RequestResidentProgramsNV)

	.p2align	4,,15
	.globl	GL_PREFIX(TrackMatrixNV)
	.type	GL_PREFIX(TrackMatrixNV), @function
GL_PREFIX(TrackMatrixNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5408(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5408(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5408(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5408(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(TrackMatrixNV), .-GL_PREFIX(TrackMatrixNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib1dNV)
	.type	GL_PREFIX(VertexAttrib1dNV), @function
GL_PREFIX(VertexAttrib1dNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5416(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	5416(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5416(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	5416(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib1dNV), .-GL_PREFIX(VertexAttrib1dNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib1dvNV)
	.type	GL_PREFIX(VertexAttrib1dvNV), @function
GL_PREFIX(VertexAttrib1dvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5424(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5424(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5424(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5424(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib1dvNV), .-GL_PREFIX(VertexAttrib1dvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib1fNV)
	.type	GL_PREFIX(VertexAttrib1fNV), @function
GL_PREFIX(VertexAttrib1fNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5432(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	5432(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5432(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	5432(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib1fNV), .-GL_PREFIX(VertexAttrib1fNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib1fvNV)
	.type	GL_PREFIX(VertexAttrib1fvNV), @function
GL_PREFIX(VertexAttrib1fvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5440(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5440(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5440(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5440(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib1fvNV), .-GL_PREFIX(VertexAttrib1fvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib1sNV)
	.type	GL_PREFIX(VertexAttrib1sNV), @function
GL_PREFIX(VertexAttrib1sNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5448(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5448(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5448(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5448(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib1sNV), .-GL_PREFIX(VertexAttrib1sNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib1svNV)
	.type	GL_PREFIX(VertexAttrib1svNV), @function
GL_PREFIX(VertexAttrib1svNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5456(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5456(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5456(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5456(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib1svNV), .-GL_PREFIX(VertexAttrib1svNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib2dNV)
	.type	GL_PREFIX(VertexAttrib2dNV), @function
GL_PREFIX(VertexAttrib2dNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5464(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	5464(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5464(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	5464(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib2dNV), .-GL_PREFIX(VertexAttrib2dNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib2dvNV)
	.type	GL_PREFIX(VertexAttrib2dvNV), @function
GL_PREFIX(VertexAttrib2dvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5472(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5472(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5472(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5472(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib2dvNV), .-GL_PREFIX(VertexAttrib2dvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib2fNV)
	.type	GL_PREFIX(VertexAttrib2fNV), @function
GL_PREFIX(VertexAttrib2fNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5480(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	5480(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5480(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	call	_glapi_get_dispatch
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	5480(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib2fNV), .-GL_PREFIX(VertexAttrib2fNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib2fvNV)
	.type	GL_PREFIX(VertexAttrib2fvNV), @function
GL_PREFIX(VertexAttrib2fvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5488(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5488(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5488(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5488(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib2fvNV), .-GL_PREFIX(VertexAttrib2fvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib2sNV)
	.type	GL_PREFIX(VertexAttrib2sNV), @function
GL_PREFIX(VertexAttrib2sNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5496(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5496(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5496(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5496(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib2sNV), .-GL_PREFIX(VertexAttrib2sNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib2svNV)
	.type	GL_PREFIX(VertexAttrib2svNV), @function
GL_PREFIX(VertexAttrib2svNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5504(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5504(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5504(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5504(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib2svNV), .-GL_PREFIX(VertexAttrib2svNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib3dNV)
	.type	GL_PREFIX(VertexAttrib3dNV), @function
GL_PREFIX(VertexAttrib3dNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5512(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	5512(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5512(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	5512(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib3dNV), .-GL_PREFIX(VertexAttrib3dNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib3dvNV)
	.type	GL_PREFIX(VertexAttrib3dvNV), @function
GL_PREFIX(VertexAttrib3dvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5520(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5520(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5520(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5520(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib3dvNV), .-GL_PREFIX(VertexAttrib3dvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib3fNV)
	.type	GL_PREFIX(VertexAttrib3fNV), @function
GL_PREFIX(VertexAttrib3fNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5528(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	5528(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5528(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	call	_glapi_get_dispatch
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	5528(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib3fNV), .-GL_PREFIX(VertexAttrib3fNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib3fvNV)
	.type	GL_PREFIX(VertexAttrib3fvNV), @function
GL_PREFIX(VertexAttrib3fvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5536(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5536(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5536(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5536(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib3fvNV), .-GL_PREFIX(VertexAttrib3fvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib3sNV)
	.type	GL_PREFIX(VertexAttrib3sNV), @function
GL_PREFIX(VertexAttrib3sNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5544(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5544(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5544(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5544(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib3sNV), .-GL_PREFIX(VertexAttrib3sNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib3svNV)
	.type	GL_PREFIX(VertexAttrib3svNV), @function
GL_PREFIX(VertexAttrib3svNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5552(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5552(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5552(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5552(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib3svNV), .-GL_PREFIX(VertexAttrib3svNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4dNV)
	.type	GL_PREFIX(VertexAttrib4dNV), @function
GL_PREFIX(VertexAttrib4dNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5560(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	movq	%xmm3, 32(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	32(%rsp), %xmm3
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	5560(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5560(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	movq	%xmm3, 32(%rsp)
	call	_glapi_get_dispatch
	movq	32(%rsp), %xmm3
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	5560(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4dNV), .-GL_PREFIX(VertexAttrib4dNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4dvNV)
	.type	GL_PREFIX(VertexAttrib4dvNV), @function
GL_PREFIX(VertexAttrib4dvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5568(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5568(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5568(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5568(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4dvNV), .-GL_PREFIX(VertexAttrib4dvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4fNV)
	.type	GL_PREFIX(VertexAttrib4fNV), @function
GL_PREFIX(VertexAttrib4fNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5576(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	movq	%xmm3, 32(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	32(%rsp), %xmm3
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	5576(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5576(%rax), %r11
	jmp	*%r11
1:
	subq	$40, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	movq	%xmm1, 16(%rsp)
	movq	%xmm2, 24(%rsp)
	movq	%xmm3, 32(%rsp)
	call	_glapi_get_dispatch
	movq	32(%rsp), %xmm3
	movq	24(%rsp), %xmm2
	movq	16(%rsp), %xmm1
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$40, %rsp
	movq	5576(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4fNV), .-GL_PREFIX(VertexAttrib4fNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4fvNV)
	.type	GL_PREFIX(VertexAttrib4fvNV), @function
GL_PREFIX(VertexAttrib4fvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5584(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5584(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5584(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5584(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4fvNV), .-GL_PREFIX(VertexAttrib4fvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4sNV)
	.type	GL_PREFIX(VertexAttrib4sNV), @function
GL_PREFIX(VertexAttrib4sNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5592(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5592(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5592(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5592(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4sNV), .-GL_PREFIX(VertexAttrib4sNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4svNV)
	.type	GL_PREFIX(VertexAttrib4svNV), @function
GL_PREFIX(VertexAttrib4svNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5600(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5600(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5600(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5600(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4svNV), .-GL_PREFIX(VertexAttrib4svNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4ubNV)
	.type	GL_PREFIX(VertexAttrib4ubNV), @function
GL_PREFIX(VertexAttrib4ubNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5608(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5608(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5608(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5608(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4ubNV), .-GL_PREFIX(VertexAttrib4ubNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttrib4ubvNV)
	.type	GL_PREFIX(VertexAttrib4ubvNV), @function
GL_PREFIX(VertexAttrib4ubvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5616(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5616(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5616(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5616(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttrib4ubvNV), .-GL_PREFIX(VertexAttrib4ubvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribPointerNV)
	.type	GL_PREFIX(VertexAttribPointerNV), @function
GL_PREFIX(VertexAttribPointerNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5624(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5624(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5624(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5624(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribPointerNV), .-GL_PREFIX(VertexAttribPointerNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribs1dvNV)
	.type	GL_PREFIX(VertexAttribs1dvNV), @function
GL_PREFIX(VertexAttribs1dvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5632(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5632(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5632(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5632(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribs1dvNV), .-GL_PREFIX(VertexAttribs1dvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribs1fvNV)
	.type	GL_PREFIX(VertexAttribs1fvNV), @function
GL_PREFIX(VertexAttribs1fvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5640(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5640(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5640(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5640(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribs1fvNV), .-GL_PREFIX(VertexAttribs1fvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribs1svNV)
	.type	GL_PREFIX(VertexAttribs1svNV), @function
GL_PREFIX(VertexAttribs1svNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5648(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5648(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5648(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5648(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribs1svNV), .-GL_PREFIX(VertexAttribs1svNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribs2dvNV)
	.type	GL_PREFIX(VertexAttribs2dvNV), @function
GL_PREFIX(VertexAttribs2dvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5656(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5656(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5656(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5656(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribs2dvNV), .-GL_PREFIX(VertexAttribs2dvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribs2fvNV)
	.type	GL_PREFIX(VertexAttribs2fvNV), @function
GL_PREFIX(VertexAttribs2fvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5664(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5664(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5664(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5664(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribs2fvNV), .-GL_PREFIX(VertexAttribs2fvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribs2svNV)
	.type	GL_PREFIX(VertexAttribs2svNV), @function
GL_PREFIX(VertexAttribs2svNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5672(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5672(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5672(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5672(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribs2svNV), .-GL_PREFIX(VertexAttribs2svNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribs3dvNV)
	.type	GL_PREFIX(VertexAttribs3dvNV), @function
GL_PREFIX(VertexAttribs3dvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5680(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5680(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5680(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5680(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribs3dvNV), .-GL_PREFIX(VertexAttribs3dvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribs3fvNV)
	.type	GL_PREFIX(VertexAttribs3fvNV), @function
GL_PREFIX(VertexAttribs3fvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5688(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5688(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5688(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5688(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribs3fvNV), .-GL_PREFIX(VertexAttribs3fvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribs3svNV)
	.type	GL_PREFIX(VertexAttribs3svNV), @function
GL_PREFIX(VertexAttribs3svNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5696(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5696(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5696(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5696(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribs3svNV), .-GL_PREFIX(VertexAttribs3svNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribs4dvNV)
	.type	GL_PREFIX(VertexAttribs4dvNV), @function
GL_PREFIX(VertexAttribs4dvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5704(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5704(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5704(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5704(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribs4dvNV), .-GL_PREFIX(VertexAttribs4dvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribs4fvNV)
	.type	GL_PREFIX(VertexAttribs4fvNV), @function
GL_PREFIX(VertexAttribs4fvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5712(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5712(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5712(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5712(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribs4fvNV), .-GL_PREFIX(VertexAttribs4fvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribs4svNV)
	.type	GL_PREFIX(VertexAttribs4svNV), @function
GL_PREFIX(VertexAttribs4svNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5720(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5720(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5720(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5720(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribs4svNV), .-GL_PREFIX(VertexAttribs4svNV)

	.p2align	4,,15
	.globl	GL_PREFIX(VertexAttribs4ubvNV)
	.type	GL_PREFIX(VertexAttribs4ubvNV), @function
GL_PREFIX(VertexAttribs4ubvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5728(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5728(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5728(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5728(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(VertexAttribs4ubvNV), .-GL_PREFIX(VertexAttribs4ubvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(AlphaFragmentOp1ATI)
	.type	GL_PREFIX(AlphaFragmentOp1ATI), @function
GL_PREFIX(AlphaFragmentOp1ATI):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5736(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5736(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5736(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5736(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(AlphaFragmentOp1ATI), .-GL_PREFIX(AlphaFragmentOp1ATI)

	.p2align	4,,15
	.globl	GL_PREFIX(AlphaFragmentOp2ATI)
	.type	GL_PREFIX(AlphaFragmentOp2ATI), @function
GL_PREFIX(AlphaFragmentOp2ATI):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5744(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5744(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5744(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5744(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(AlphaFragmentOp2ATI), .-GL_PREFIX(AlphaFragmentOp2ATI)

	.p2align	4,,15
	.globl	GL_PREFIX(AlphaFragmentOp3ATI)
	.type	GL_PREFIX(AlphaFragmentOp3ATI), @function
GL_PREFIX(AlphaFragmentOp3ATI):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5752(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5752(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5752(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5752(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(AlphaFragmentOp3ATI), .-GL_PREFIX(AlphaFragmentOp3ATI)

	.p2align	4,,15
	.globl	GL_PREFIX(BeginFragmentShaderATI)
	.type	GL_PREFIX(BeginFragmentShaderATI), @function
GL_PREFIX(BeginFragmentShaderATI):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5760(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	5760(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5760(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	5760(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BeginFragmentShaderATI), .-GL_PREFIX(BeginFragmentShaderATI)

	.p2align	4,,15
	.globl	GL_PREFIX(BindFragmentShaderATI)
	.type	GL_PREFIX(BindFragmentShaderATI), @function
GL_PREFIX(BindFragmentShaderATI):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5768(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5768(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5768(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5768(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BindFragmentShaderATI), .-GL_PREFIX(BindFragmentShaderATI)

	.p2align	4,,15
	.globl	GL_PREFIX(ColorFragmentOp1ATI)
	.type	GL_PREFIX(ColorFragmentOp1ATI), @function
GL_PREFIX(ColorFragmentOp1ATI):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5776(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5776(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5776(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5776(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ColorFragmentOp1ATI), .-GL_PREFIX(ColorFragmentOp1ATI)

	.p2align	4,,15
	.globl	GL_PREFIX(ColorFragmentOp2ATI)
	.type	GL_PREFIX(ColorFragmentOp2ATI), @function
GL_PREFIX(ColorFragmentOp2ATI):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5784(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5784(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5784(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5784(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ColorFragmentOp2ATI), .-GL_PREFIX(ColorFragmentOp2ATI)

	.p2align	4,,15
	.globl	GL_PREFIX(ColorFragmentOp3ATI)
	.type	GL_PREFIX(ColorFragmentOp3ATI), @function
GL_PREFIX(ColorFragmentOp3ATI):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5792(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5792(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5792(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5792(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ColorFragmentOp3ATI), .-GL_PREFIX(ColorFragmentOp3ATI)

	.p2align	4,,15
	.globl	GL_PREFIX(DeleteFragmentShaderATI)
	.type	GL_PREFIX(DeleteFragmentShaderATI), @function
GL_PREFIX(DeleteFragmentShaderATI):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5800(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5800(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5800(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5800(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DeleteFragmentShaderATI), .-GL_PREFIX(DeleteFragmentShaderATI)

	.p2align	4,,15
	.globl	GL_PREFIX(EndFragmentShaderATI)
	.type	GL_PREFIX(EndFragmentShaderATI), @function
GL_PREFIX(EndFragmentShaderATI):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5808(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	5808(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5808(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	5808(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(EndFragmentShaderATI), .-GL_PREFIX(EndFragmentShaderATI)

	.p2align	4,,15
	.globl	GL_PREFIX(GenFragmentShadersATI)
	.type	GL_PREFIX(GenFragmentShadersATI), @function
GL_PREFIX(GenFragmentShadersATI):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5816(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5816(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5816(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5816(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GenFragmentShadersATI), .-GL_PREFIX(GenFragmentShadersATI)

	.p2align	4,,15
	.globl	GL_PREFIX(PassTexCoordATI)
	.type	GL_PREFIX(PassTexCoordATI), @function
GL_PREFIX(PassTexCoordATI):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5824(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5824(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5824(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5824(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PassTexCoordATI), .-GL_PREFIX(PassTexCoordATI)

	.p2align	4,,15
	.globl	GL_PREFIX(SampleMapATI)
	.type	GL_PREFIX(SampleMapATI), @function
GL_PREFIX(SampleMapATI):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5832(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5832(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5832(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5832(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SampleMapATI), .-GL_PREFIX(SampleMapATI)

	.p2align	4,,15
	.globl	GL_PREFIX(SetFragmentShaderConstantATI)
	.type	GL_PREFIX(SetFragmentShaderConstantATI), @function
GL_PREFIX(SetFragmentShaderConstantATI):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5840(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5840(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5840(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5840(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(SetFragmentShaderConstantATI), .-GL_PREFIX(SetFragmentShaderConstantATI)

	.p2align	4,,15
	.globl	GL_PREFIX(PointParameteriNV)
	.type	GL_PREFIX(PointParameteriNV), @function
GL_PREFIX(PointParameteriNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5848(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5848(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5848(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5848(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PointParameteriNV), .-GL_PREFIX(PointParameteriNV)

	.p2align	4,,15
	.globl	GL_PREFIX(PointParameterivNV)
	.type	GL_PREFIX(PointParameterivNV), @function
GL_PREFIX(PointParameterivNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5856(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5856(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5856(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5856(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(PointParameterivNV), .-GL_PREFIX(PointParameterivNV)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_733)
	.type	GL_PREFIX(_dispatch_stub_733), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_733))
GL_PREFIX(_dispatch_stub_733):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5864(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5864(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5864(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5864(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_733), .-GL_PREFIX(_dispatch_stub_733)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_734)
	.type	GL_PREFIX(_dispatch_stub_734), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_734))
GL_PREFIX(_dispatch_stub_734):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5872(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5872(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5872(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5872(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_734), .-GL_PREFIX(_dispatch_stub_734)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_735)
	.type	GL_PREFIX(_dispatch_stub_735), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_735))
GL_PREFIX(_dispatch_stub_735):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5880(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5880(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5880(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5880(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_735), .-GL_PREFIX(_dispatch_stub_735)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_736)
	.type	GL_PREFIX(_dispatch_stub_736), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_736))
GL_PREFIX(_dispatch_stub_736):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5888(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5888(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5888(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5888(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_736), .-GL_PREFIX(_dispatch_stub_736)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_737)
	.type	GL_PREFIX(_dispatch_stub_737), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_737))
GL_PREFIX(_dispatch_stub_737):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5896(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5896(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5896(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5896(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_737), .-GL_PREFIX(_dispatch_stub_737)

	.p2align	4,,15
	.globl	GL_PREFIX(GetProgramNamedParameterdvNV)
	.type	GL_PREFIX(GetProgramNamedParameterdvNV), @function
GL_PREFIX(GetProgramNamedParameterdvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5904(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5904(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5904(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5904(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetProgramNamedParameterdvNV), .-GL_PREFIX(GetProgramNamedParameterdvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(GetProgramNamedParameterfvNV)
	.type	GL_PREFIX(GetProgramNamedParameterfvNV), @function
GL_PREFIX(GetProgramNamedParameterfvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5912(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5912(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5912(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5912(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetProgramNamedParameterfvNV), .-GL_PREFIX(GetProgramNamedParameterfvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramNamedParameter4dNV)
	.type	GL_PREFIX(ProgramNamedParameter4dNV), @function
GL_PREFIX(ProgramNamedParameter4dNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5920(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%rdx, 16(%rsp)
	movq	%xmm0, 24(%rsp)
	movq	%xmm1, 32(%rsp)
	movq	%xmm2, 40(%rsp)
	movq	%xmm3, 48(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	48(%rsp), %xmm3
	movq	40(%rsp), %xmm2
	movq	32(%rsp), %xmm1
	movq	24(%rsp), %xmm0
	movq	16(%rsp), %rdx
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	5920(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5920(%rax), %r11
	jmp	*%r11
1:
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%rdx, 16(%rsp)
	movq	%xmm0, 24(%rsp)
	movq	%xmm1, 32(%rsp)
	movq	%xmm2, 40(%rsp)
	movq	%xmm3, 48(%rsp)
	call	_glapi_get_dispatch
	movq	48(%rsp), %xmm3
	movq	40(%rsp), %xmm2
	movq	32(%rsp), %xmm1
	movq	24(%rsp), %xmm0
	movq	16(%rsp), %rdx
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	5920(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramNamedParameter4dNV), .-GL_PREFIX(ProgramNamedParameter4dNV)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramNamedParameter4dvNV)
	.type	GL_PREFIX(ProgramNamedParameter4dvNV), @function
GL_PREFIX(ProgramNamedParameter4dvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5928(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5928(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5928(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5928(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramNamedParameter4dvNV), .-GL_PREFIX(ProgramNamedParameter4dvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramNamedParameter4fNV)
	.type	GL_PREFIX(ProgramNamedParameter4fNV), @function
GL_PREFIX(ProgramNamedParameter4fNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5936(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%rdx, 16(%rsp)
	movq	%xmm0, 24(%rsp)
	movq	%xmm1, 32(%rsp)
	movq	%xmm2, 40(%rsp)
	movq	%xmm3, 48(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	48(%rsp), %xmm3
	movq	40(%rsp), %xmm2
	movq	32(%rsp), %xmm1
	movq	24(%rsp), %xmm0
	movq	16(%rsp), %rdx
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	5936(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5936(%rax), %r11
	jmp	*%r11
1:
	subq	$56, %rsp
	movq	%rdi, (%rsp)
	movq	%rsi, 8(%rsp)
	movq	%rdx, 16(%rsp)
	movq	%xmm0, 24(%rsp)
	movq	%xmm1, 32(%rsp)
	movq	%xmm2, 40(%rsp)
	movq	%xmm3, 48(%rsp)
	call	_glapi_get_dispatch
	movq	48(%rsp), %xmm3
	movq	40(%rsp), %xmm2
	movq	32(%rsp), %xmm1
	movq	24(%rsp), %xmm0
	movq	16(%rsp), %rdx
	movq	8(%rsp), %rsi
	movq	(%rsp), %rdi
	addq	$56, %rsp
	movq	5936(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramNamedParameter4fNV), .-GL_PREFIX(ProgramNamedParameter4fNV)

	.p2align	4,,15
	.globl	GL_PREFIX(ProgramNamedParameter4fvNV)
	.type	GL_PREFIX(ProgramNamedParameter4fvNV), @function
GL_PREFIX(ProgramNamedParameter4fvNV):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5944(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5944(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5944(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5944(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(ProgramNamedParameter4fvNV), .-GL_PREFIX(ProgramNamedParameter4fvNV)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_744)
	.type	GL_PREFIX(_dispatch_stub_744), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_744))
GL_PREFIX(_dispatch_stub_744):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5952(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5952(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5952(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5952(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_744), .-GL_PREFIX(_dispatch_stub_744)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_745)
	.type	GL_PREFIX(_dispatch_stub_745), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_745))
GL_PREFIX(_dispatch_stub_745):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5960(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5960(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5960(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5960(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_745), .-GL_PREFIX(_dispatch_stub_745)

	.p2align	4,,15
	.globl	GL_PREFIX(BindFramebufferEXT)
	.type	GL_PREFIX(BindFramebufferEXT), @function
GL_PREFIX(BindFramebufferEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5968(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5968(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5968(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5968(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BindFramebufferEXT), .-GL_PREFIX(BindFramebufferEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(BindRenderbufferEXT)
	.type	GL_PREFIX(BindRenderbufferEXT), @function
GL_PREFIX(BindRenderbufferEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5976(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5976(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5976(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5976(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(BindRenderbufferEXT), .-GL_PREFIX(BindRenderbufferEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(CheckFramebufferStatusEXT)
	.type	GL_PREFIX(CheckFramebufferStatusEXT), @function
GL_PREFIX(CheckFramebufferStatusEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5984(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5984(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5984(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5984(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(CheckFramebufferStatusEXT), .-GL_PREFIX(CheckFramebufferStatusEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(DeleteFramebuffersEXT)
	.type	GL_PREFIX(DeleteFramebuffersEXT), @function
GL_PREFIX(DeleteFramebuffersEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5992(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5992(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5992(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5992(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DeleteFramebuffersEXT), .-GL_PREFIX(DeleteFramebuffersEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(DeleteRenderbuffersEXT)
	.type	GL_PREFIX(DeleteRenderbuffersEXT), @function
GL_PREFIX(DeleteRenderbuffersEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6000(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6000(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6000(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6000(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(DeleteRenderbuffersEXT), .-GL_PREFIX(DeleteRenderbuffersEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(FramebufferRenderbufferEXT)
	.type	GL_PREFIX(FramebufferRenderbufferEXT), @function
GL_PREFIX(FramebufferRenderbufferEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6008(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6008(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6008(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6008(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(FramebufferRenderbufferEXT), .-GL_PREFIX(FramebufferRenderbufferEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(FramebufferTexture1DEXT)
	.type	GL_PREFIX(FramebufferTexture1DEXT), @function
GL_PREFIX(FramebufferTexture1DEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6016(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6016(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6016(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6016(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(FramebufferTexture1DEXT), .-GL_PREFIX(FramebufferTexture1DEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(FramebufferTexture2DEXT)
	.type	GL_PREFIX(FramebufferTexture2DEXT), @function
GL_PREFIX(FramebufferTexture2DEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6024(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6024(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6024(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6024(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(FramebufferTexture2DEXT), .-GL_PREFIX(FramebufferTexture2DEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(FramebufferTexture3DEXT)
	.type	GL_PREFIX(FramebufferTexture3DEXT), @function
GL_PREFIX(FramebufferTexture3DEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6032(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6032(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6032(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6032(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(FramebufferTexture3DEXT), .-GL_PREFIX(FramebufferTexture3DEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(GenFramebuffersEXT)
	.type	GL_PREFIX(GenFramebuffersEXT), @function
GL_PREFIX(GenFramebuffersEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6040(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6040(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6040(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6040(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GenFramebuffersEXT), .-GL_PREFIX(GenFramebuffersEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(GenRenderbuffersEXT)
	.type	GL_PREFIX(GenRenderbuffersEXT), @function
GL_PREFIX(GenRenderbuffersEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6048(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6048(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6048(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6048(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GenRenderbuffersEXT), .-GL_PREFIX(GenRenderbuffersEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(GenerateMipmapEXT)
	.type	GL_PREFIX(GenerateMipmapEXT), @function
GL_PREFIX(GenerateMipmapEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6056(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	6056(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6056(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	6056(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GenerateMipmapEXT), .-GL_PREFIX(GenerateMipmapEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(GetFramebufferAttachmentParameterivEXT)
	.type	GL_PREFIX(GetFramebufferAttachmentParameterivEXT), @function
GL_PREFIX(GetFramebufferAttachmentParameterivEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6064(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6064(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6064(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6064(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetFramebufferAttachmentParameterivEXT), .-GL_PREFIX(GetFramebufferAttachmentParameterivEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(GetRenderbufferParameterivEXT)
	.type	GL_PREFIX(GetRenderbufferParameterivEXT), @function
GL_PREFIX(GetRenderbufferParameterivEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6072(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6072(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6072(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6072(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(GetRenderbufferParameterivEXT), .-GL_PREFIX(GetRenderbufferParameterivEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(IsFramebufferEXT)
	.type	GL_PREFIX(IsFramebufferEXT), @function
GL_PREFIX(IsFramebufferEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6080(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	6080(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6080(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	6080(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(IsFramebufferEXT), .-GL_PREFIX(IsFramebufferEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(IsRenderbufferEXT)
	.type	GL_PREFIX(IsRenderbufferEXT), @function
GL_PREFIX(IsRenderbufferEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6088(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	6088(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6088(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	6088(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(IsRenderbufferEXT), .-GL_PREFIX(IsRenderbufferEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(RenderbufferStorageEXT)
	.type	GL_PREFIX(RenderbufferStorageEXT), @function
GL_PREFIX(RenderbufferStorageEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6096(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6096(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6096(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6096(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(RenderbufferStorageEXT), .-GL_PREFIX(RenderbufferStorageEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_763)
	.type	GL_PREFIX(_dispatch_stub_763), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_763))
GL_PREFIX(_dispatch_stub_763):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6104(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6104(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6104(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6104(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_763), .-GL_PREFIX(_dispatch_stub_763)

	.p2align	4,,15
	.globl	GL_PREFIX(FramebufferTextureLayerEXT)
	.type	GL_PREFIX(FramebufferTextureLayerEXT), @function
GL_PREFIX(FramebufferTextureLayerEXT):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6112(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_x86_64_get_dispatch@PLT
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6112(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6112(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6112(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(FramebufferTextureLayerEXT), .-GL_PREFIX(FramebufferTextureLayerEXT)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_765)
	.type	GL_PREFIX(_dispatch_stub_765), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_765))
GL_PREFIX(_dispatch_stub_765):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6120(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6120(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6120(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6120(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_765), .-GL_PREFIX(_dispatch_stub_765)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_766)
	.type	GL_PREFIX(_dispatch_stub_766), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_766))
GL_PREFIX(_dispatch_stub_766):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6128(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6128(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6128(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6128(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_766), .-GL_PREFIX(_dispatch_stub_766)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_767)
	.type	GL_PREFIX(_dispatch_stub_767), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_767))
GL_PREFIX(_dispatch_stub_767):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6136(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6136(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6136(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6136(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_767), .-GL_PREFIX(_dispatch_stub_767)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_768)
	.type	GL_PREFIX(_dispatch_stub_768), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_768))
GL_PREFIX(_dispatch_stub_768):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6144(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6144(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6144(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6144(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_768), .-GL_PREFIX(_dispatch_stub_768)

	.p2align	4,,15
	.globl	GL_PREFIX(_dispatch_stub_769)
	.type	GL_PREFIX(_dispatch_stub_769), @function
	HIDDEN(GL_PREFIX(_dispatch_stub_769))
GL_PREFIX(_dispatch_stub_769):
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6152(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6152(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6152(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6152(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	GL_PREFIX(_dispatch_stub_769), .-GL_PREFIX(_dispatch_stub_769)

	.globl GL_PREFIX(ArrayElementEXT) ; .set GL_PREFIX(ArrayElementEXT), GL_PREFIX(ArrayElement)
	.globl GL_PREFIX(BindTextureEXT) ; .set GL_PREFIX(BindTextureEXT), GL_PREFIX(BindTexture)
	.globl GL_PREFIX(DrawArraysEXT) ; .set GL_PREFIX(DrawArraysEXT), GL_PREFIX(DrawArrays)
#ifndef GLX_INDIRECT_RENDERING
	.globl GL_PREFIX(AreTexturesResidentEXT) ; .set GL_PREFIX(AreTexturesResidentEXT), GL_PREFIX(AreTexturesResident)
#endif
	.globl GL_PREFIX(CopyTexImage1DEXT) ; .set GL_PREFIX(CopyTexImage1DEXT), GL_PREFIX(CopyTexImage1D)
	.globl GL_PREFIX(CopyTexImage2DEXT) ; .set GL_PREFIX(CopyTexImage2DEXT), GL_PREFIX(CopyTexImage2D)
	.globl GL_PREFIX(CopyTexSubImage1DEXT) ; .set GL_PREFIX(CopyTexSubImage1DEXT), GL_PREFIX(CopyTexSubImage1D)
	.globl GL_PREFIX(CopyTexSubImage2DEXT) ; .set GL_PREFIX(CopyTexSubImage2DEXT), GL_PREFIX(CopyTexSubImage2D)
#ifndef GLX_INDIRECT_RENDERING
	.globl GL_PREFIX(DeleteTexturesEXT) ; .set GL_PREFIX(DeleteTexturesEXT), GL_PREFIX(DeleteTextures)
#endif
#ifndef GLX_INDIRECT_RENDERING
	.globl GL_PREFIX(GenTexturesEXT) ; .set GL_PREFIX(GenTexturesEXT), GL_PREFIX(GenTextures)
#endif
	.globl GL_PREFIX(GetPointervEXT) ; .set GL_PREFIX(GetPointervEXT), GL_PREFIX(GetPointerv)
#ifndef GLX_INDIRECT_RENDERING
	.globl GL_PREFIX(IsTextureEXT) ; .set GL_PREFIX(IsTextureEXT), GL_PREFIX(IsTexture)
#endif
	.globl GL_PREFIX(PrioritizeTexturesEXT) ; .set GL_PREFIX(PrioritizeTexturesEXT), GL_PREFIX(PrioritizeTextures)
	.globl GL_PREFIX(TexSubImage1DEXT) ; .set GL_PREFIX(TexSubImage1DEXT), GL_PREFIX(TexSubImage1D)
	.globl GL_PREFIX(TexSubImage2DEXT) ; .set GL_PREFIX(TexSubImage2DEXT), GL_PREFIX(TexSubImage2D)
	.globl GL_PREFIX(BlendColorEXT) ; .set GL_PREFIX(BlendColorEXT), GL_PREFIX(BlendColor)
	.globl GL_PREFIX(BlendEquationEXT) ; .set GL_PREFIX(BlendEquationEXT), GL_PREFIX(BlendEquation)
	.globl GL_PREFIX(DrawRangeElementsEXT) ; .set GL_PREFIX(DrawRangeElementsEXT), GL_PREFIX(DrawRangeElements)
	.globl GL_PREFIX(ColorTableEXT) ; .set GL_PREFIX(ColorTableEXT), GL_PREFIX(ColorTable)
#ifndef GLX_INDIRECT_RENDERING
	.globl GL_PREFIX(GetColorTableEXT) ; .set GL_PREFIX(GetColorTableEXT), GL_PREFIX(GetColorTable)
#endif
#ifndef GLX_INDIRECT_RENDERING
	.globl GL_PREFIX(GetColorTableParameterfvEXT) ; .set GL_PREFIX(GetColorTableParameterfvEXT), GL_PREFIX(GetColorTableParameterfv)
#endif
#ifndef GLX_INDIRECT_RENDERING
	.globl GL_PREFIX(GetColorTableParameterivEXT) ; .set GL_PREFIX(GetColorTableParameterivEXT), GL_PREFIX(GetColorTableParameteriv)
#endif
	.globl GL_PREFIX(TexImage3DEXT) ; .set GL_PREFIX(TexImage3DEXT), GL_PREFIX(TexImage3D)
	.globl GL_PREFIX(TexSubImage3DEXT) ; .set GL_PREFIX(TexSubImage3DEXT), GL_PREFIX(TexSubImage3D)
	.globl GL_PREFIX(CopyTexSubImage3DEXT) ; .set GL_PREFIX(CopyTexSubImage3DEXT), GL_PREFIX(CopyTexSubImage3D)
	.globl GL_PREFIX(ActiveTexture) ; .set GL_PREFIX(ActiveTexture), GL_PREFIX(ActiveTextureARB)
	.globl GL_PREFIX(ClientActiveTexture) ; .set GL_PREFIX(ClientActiveTexture), GL_PREFIX(ClientActiveTextureARB)
	.globl GL_PREFIX(MultiTexCoord1d) ; .set GL_PREFIX(MultiTexCoord1d), GL_PREFIX(MultiTexCoord1dARB)
	.globl GL_PREFIX(MultiTexCoord1dv) ; .set GL_PREFIX(MultiTexCoord1dv), GL_PREFIX(MultiTexCoord1dvARB)
	.globl GL_PREFIX(MultiTexCoord1f) ; .set GL_PREFIX(MultiTexCoord1f), GL_PREFIX(MultiTexCoord1fARB)
	.globl GL_PREFIX(MultiTexCoord1fv) ; .set GL_PREFIX(MultiTexCoord1fv), GL_PREFIX(MultiTexCoord1fvARB)
	.globl GL_PREFIX(MultiTexCoord1i) ; .set GL_PREFIX(MultiTexCoord1i), GL_PREFIX(MultiTexCoord1iARB)
	.globl GL_PREFIX(MultiTexCoord1iv) ; .set GL_PREFIX(MultiTexCoord1iv), GL_PREFIX(MultiTexCoord1ivARB)
	.globl GL_PREFIX(MultiTexCoord1s) ; .set GL_PREFIX(MultiTexCoord1s), GL_PREFIX(MultiTexCoord1sARB)
	.globl GL_PREFIX(MultiTexCoord1sv) ; .set GL_PREFIX(MultiTexCoord1sv), GL_PREFIX(MultiTexCoord1svARB)
	.globl GL_PREFIX(MultiTexCoord2d) ; .set GL_PREFIX(MultiTexCoord2d), GL_PREFIX(MultiTexCoord2dARB)
	.globl GL_PREFIX(MultiTexCoord2dv) ; .set GL_PREFIX(MultiTexCoord2dv), GL_PREFIX(MultiTexCoord2dvARB)
	.globl GL_PREFIX(MultiTexCoord2f) ; .set GL_PREFIX(MultiTexCoord2f), GL_PREFIX(MultiTexCoord2fARB)
	.globl GL_PREFIX(MultiTexCoord2fv) ; .set GL_PREFIX(MultiTexCoord2fv), GL_PREFIX(MultiTexCoord2fvARB)
	.globl GL_PREFIX(MultiTexCoord2i) ; .set GL_PREFIX(MultiTexCoord2i), GL_PREFIX(MultiTexCoord2iARB)
	.globl GL_PREFIX(MultiTexCoord2iv) ; .set GL_PREFIX(MultiTexCoord2iv), GL_PREFIX(MultiTexCoord2ivARB)
	.globl GL_PREFIX(MultiTexCoord2s) ; .set GL_PREFIX(MultiTexCoord2s), GL_PREFIX(MultiTexCoord2sARB)
	.globl GL_PREFIX(MultiTexCoord2sv) ; .set GL_PREFIX(MultiTexCoord2sv), GL_PREFIX(MultiTexCoord2svARB)
	.globl GL_PREFIX(MultiTexCoord3d) ; .set GL_PREFIX(MultiTexCoord3d), GL_PREFIX(MultiTexCoord3dARB)
	.globl GL_PREFIX(MultiTexCoord3dv) ; .set GL_PREFIX(MultiTexCoord3dv), GL_PREFIX(MultiTexCoord3dvARB)
	.globl GL_PREFIX(MultiTexCoord3f) ; .set GL_PREFIX(MultiTexCoord3f), GL_PREFIX(MultiTexCoord3fARB)
	.globl GL_PREFIX(MultiTexCoord3fv) ; .set GL_PREFIX(MultiTexCoord3fv), GL_PREFIX(MultiTexCoord3fvARB)
	.globl GL_PREFIX(MultiTexCoord3i) ; .set GL_PREFIX(MultiTexCoord3i), GL_PREFIX(MultiTexCoord3iARB)
	.globl GL_PREFIX(MultiTexCoord3iv) ; .set GL_PREFIX(MultiTexCoord3iv), GL_PREFIX(MultiTexCoord3ivARB)
	.globl GL_PREFIX(MultiTexCoord3s) ; .set GL_PREFIX(MultiTexCoord3s), GL_PREFIX(MultiTexCoord3sARB)
	.globl GL_PREFIX(MultiTexCoord3sv) ; .set GL_PREFIX(MultiTexCoord3sv), GL_PREFIX(MultiTexCoord3svARB)
	.globl GL_PREFIX(MultiTexCoord4d) ; .set GL_PREFIX(MultiTexCoord4d), GL_PREFIX(MultiTexCoord4dARB)
	.globl GL_PREFIX(MultiTexCoord4dv) ; .set GL_PREFIX(MultiTexCoord4dv), GL_PREFIX(MultiTexCoord4dvARB)
	.globl GL_PREFIX(MultiTexCoord4f) ; .set GL_PREFIX(MultiTexCoord4f), GL_PREFIX(MultiTexCoord4fARB)
	.globl GL_PREFIX(MultiTexCoord4fv) ; .set GL_PREFIX(MultiTexCoord4fv), GL_PREFIX(MultiTexCoord4fvARB)
	.globl GL_PREFIX(MultiTexCoord4i) ; .set GL_PREFIX(MultiTexCoord4i), GL_PREFIX(MultiTexCoord4iARB)
	.globl GL_PREFIX(MultiTexCoord4iv) ; .set GL_PREFIX(MultiTexCoord4iv), GL_PREFIX(MultiTexCoord4ivARB)
	.globl GL_PREFIX(MultiTexCoord4s) ; .set GL_PREFIX(MultiTexCoord4s), GL_PREFIX(MultiTexCoord4sARB)
	.globl GL_PREFIX(MultiTexCoord4sv) ; .set GL_PREFIX(MultiTexCoord4sv), GL_PREFIX(MultiTexCoord4svARB)
	.globl GL_PREFIX(LoadTransposeMatrixd) ; .set GL_PREFIX(LoadTransposeMatrixd), GL_PREFIX(LoadTransposeMatrixdARB)
	.globl GL_PREFIX(LoadTransposeMatrixf) ; .set GL_PREFIX(LoadTransposeMatrixf), GL_PREFIX(LoadTransposeMatrixfARB)
	.globl GL_PREFIX(MultTransposeMatrixd) ; .set GL_PREFIX(MultTransposeMatrixd), GL_PREFIX(MultTransposeMatrixdARB)
	.globl GL_PREFIX(MultTransposeMatrixf) ; .set GL_PREFIX(MultTransposeMatrixf), GL_PREFIX(MultTransposeMatrixfARB)
	.globl GL_PREFIX(SampleCoverage) ; .set GL_PREFIX(SampleCoverage), GL_PREFIX(SampleCoverageARB)
	.globl GL_PREFIX(CompressedTexImage1D) ; .set GL_PREFIX(CompressedTexImage1D), GL_PREFIX(CompressedTexImage1DARB)
	.globl GL_PREFIX(CompressedTexImage2D) ; .set GL_PREFIX(CompressedTexImage2D), GL_PREFIX(CompressedTexImage2DARB)
	.globl GL_PREFIX(CompressedTexImage3D) ; .set GL_PREFIX(CompressedTexImage3D), GL_PREFIX(CompressedTexImage3DARB)
	.globl GL_PREFIX(CompressedTexSubImage1D) ; .set GL_PREFIX(CompressedTexSubImage1D), GL_PREFIX(CompressedTexSubImage1DARB)
	.globl GL_PREFIX(CompressedTexSubImage2D) ; .set GL_PREFIX(CompressedTexSubImage2D), GL_PREFIX(CompressedTexSubImage2DARB)
	.globl GL_PREFIX(CompressedTexSubImage3D) ; .set GL_PREFIX(CompressedTexSubImage3D), GL_PREFIX(CompressedTexSubImage3DARB)
	.globl GL_PREFIX(GetCompressedTexImage) ; .set GL_PREFIX(GetCompressedTexImage), GL_PREFIX(GetCompressedTexImageARB)
	.globl GL_PREFIX(DisableVertexAttribArray) ; .set GL_PREFIX(DisableVertexAttribArray), GL_PREFIX(DisableVertexAttribArrayARB)
	.globl GL_PREFIX(EnableVertexAttribArray) ; .set GL_PREFIX(EnableVertexAttribArray), GL_PREFIX(EnableVertexAttribArrayARB)
	.globl GL_PREFIX(GetVertexAttribdv) ; .set GL_PREFIX(GetVertexAttribdv), GL_PREFIX(GetVertexAttribdvARB)
	.globl GL_PREFIX(GetVertexAttribfv) ; .set GL_PREFIX(GetVertexAttribfv), GL_PREFIX(GetVertexAttribfvARB)
	.globl GL_PREFIX(GetVertexAttribiv) ; .set GL_PREFIX(GetVertexAttribiv), GL_PREFIX(GetVertexAttribivARB)
	.globl GL_PREFIX(ProgramParameter4dNV) ; .set GL_PREFIX(ProgramParameter4dNV), GL_PREFIX(ProgramEnvParameter4dARB)
	.globl GL_PREFIX(ProgramParameter4dvNV) ; .set GL_PREFIX(ProgramParameter4dvNV), GL_PREFIX(ProgramEnvParameter4dvARB)
	.globl GL_PREFIX(ProgramParameter4fNV) ; .set GL_PREFIX(ProgramParameter4fNV), GL_PREFIX(ProgramEnvParameter4fARB)
	.globl GL_PREFIX(ProgramParameter4fvNV) ; .set GL_PREFIX(ProgramParameter4fvNV), GL_PREFIX(ProgramEnvParameter4fvARB)
	.globl GL_PREFIX(VertexAttrib1d) ; .set GL_PREFIX(VertexAttrib1d), GL_PREFIX(VertexAttrib1dARB)
	.globl GL_PREFIX(VertexAttrib1dv) ; .set GL_PREFIX(VertexAttrib1dv), GL_PREFIX(VertexAttrib1dvARB)
	.globl GL_PREFIX(VertexAttrib1f) ; .set GL_PREFIX(VertexAttrib1f), GL_PREFIX(VertexAttrib1fARB)
	.globl GL_PREFIX(VertexAttrib1fv) ; .set GL_PREFIX(VertexAttrib1fv), GL_PREFIX(VertexAttrib1fvARB)
	.globl GL_PREFIX(VertexAttrib1s) ; .set GL_PREFIX(VertexAttrib1s), GL_PREFIX(VertexAttrib1sARB)
	.globl GL_PREFIX(VertexAttrib1sv) ; .set GL_PREFIX(VertexAttrib1sv), GL_PREFIX(VertexAttrib1svARB)
	.globl GL_PREFIX(VertexAttrib2d) ; .set GL_PREFIX(VertexAttrib2d), GL_PREFIX(VertexAttrib2dARB)
	.globl GL_PREFIX(VertexAttrib2dv) ; .set GL_PREFIX(VertexAttrib2dv), GL_PREFIX(VertexAttrib2dvARB)
	.globl GL_PREFIX(VertexAttrib2f) ; .set GL_PREFIX(VertexAttrib2f), GL_PREFIX(VertexAttrib2fARB)
	.globl GL_PREFIX(VertexAttrib2fv) ; .set GL_PREFIX(VertexAttrib2fv), GL_PREFIX(VertexAttrib2fvARB)
	.globl GL_PREFIX(VertexAttrib2s) ; .set GL_PREFIX(VertexAttrib2s), GL_PREFIX(VertexAttrib2sARB)
	.globl GL_PREFIX(VertexAttrib2sv) ; .set GL_PREFIX(VertexAttrib2sv), GL_PREFIX(VertexAttrib2svARB)
	.globl GL_PREFIX(VertexAttrib3d) ; .set GL_PREFIX(VertexAttrib3d), GL_PREFIX(VertexAttrib3dARB)
	.globl GL_PREFIX(VertexAttrib3dv) ; .set GL_PREFIX(VertexAttrib3dv), GL_PREFIX(VertexAttrib3dvARB)
	.globl GL_PREFIX(VertexAttrib3f) ; .set GL_PREFIX(VertexAttrib3f), GL_PREFIX(VertexAttrib3fARB)
	.globl GL_PREFIX(VertexAttrib3fv) ; .set GL_PREFIX(VertexAttrib3fv), GL_PREFIX(VertexAttrib3fvARB)
	.globl GL_PREFIX(VertexAttrib3s) ; .set GL_PREFIX(VertexAttrib3s), GL_PREFIX(VertexAttrib3sARB)
	.globl GL_PREFIX(VertexAttrib3sv) ; .set GL_PREFIX(VertexAttrib3sv), GL_PREFIX(VertexAttrib3svARB)
	.globl GL_PREFIX(VertexAttrib4Nbv) ; .set GL_PREFIX(VertexAttrib4Nbv), GL_PREFIX(VertexAttrib4NbvARB)
	.globl GL_PREFIX(VertexAttrib4Niv) ; .set GL_PREFIX(VertexAttrib4Niv), GL_PREFIX(VertexAttrib4NivARB)
	.globl GL_PREFIX(VertexAttrib4Nsv) ; .set GL_PREFIX(VertexAttrib4Nsv), GL_PREFIX(VertexAttrib4NsvARB)
	.globl GL_PREFIX(VertexAttrib4Nub) ; .set GL_PREFIX(VertexAttrib4Nub), GL_PREFIX(VertexAttrib4NubARB)
	.globl GL_PREFIX(VertexAttrib4Nubv) ; .set GL_PREFIX(VertexAttrib4Nubv), GL_PREFIX(VertexAttrib4NubvARB)
	.globl GL_PREFIX(VertexAttrib4Nuiv) ; .set GL_PREFIX(VertexAttrib4Nuiv), GL_PREFIX(VertexAttrib4NuivARB)
	.globl GL_PREFIX(VertexAttrib4Nusv) ; .set GL_PREFIX(VertexAttrib4Nusv), GL_PREFIX(VertexAttrib4NusvARB)
	.globl GL_PREFIX(VertexAttrib4bv) ; .set GL_PREFIX(VertexAttrib4bv), GL_PREFIX(VertexAttrib4bvARB)
	.globl GL_PREFIX(VertexAttrib4d) ; .set GL_PREFIX(VertexAttrib4d), GL_PREFIX(VertexAttrib4dARB)
	.globl GL_PREFIX(VertexAttrib4dv) ; .set GL_PREFIX(VertexAttrib4dv), GL_PREFIX(VertexAttrib4dvARB)
	.globl GL_PREFIX(VertexAttrib4f) ; .set GL_PREFIX(VertexAttrib4f), GL_PREFIX(VertexAttrib4fARB)
	.globl GL_PREFIX(VertexAttrib4fv) ; .set GL_PREFIX(VertexAttrib4fv), GL_PREFIX(VertexAttrib4fvARB)
	.globl GL_PREFIX(VertexAttrib4iv) ; .set GL_PREFIX(VertexAttrib4iv), GL_PREFIX(VertexAttrib4ivARB)
	.globl GL_PREFIX(VertexAttrib4s) ; .set GL_PREFIX(VertexAttrib4s), GL_PREFIX(VertexAttrib4sARB)
	.globl GL_PREFIX(VertexAttrib4sv) ; .set GL_PREFIX(VertexAttrib4sv), GL_PREFIX(VertexAttrib4svARB)
	.globl GL_PREFIX(VertexAttrib4ubv) ; .set GL_PREFIX(VertexAttrib4ubv), GL_PREFIX(VertexAttrib4ubvARB)
	.globl GL_PREFIX(VertexAttrib4uiv) ; .set GL_PREFIX(VertexAttrib4uiv), GL_PREFIX(VertexAttrib4uivARB)
	.globl GL_PREFIX(VertexAttrib4usv) ; .set GL_PREFIX(VertexAttrib4usv), GL_PREFIX(VertexAttrib4usvARB)
	.globl GL_PREFIX(VertexAttribPointer) ; .set GL_PREFIX(VertexAttribPointer), GL_PREFIX(VertexAttribPointerARB)
	.globl GL_PREFIX(BindBuffer) ; .set GL_PREFIX(BindBuffer), GL_PREFIX(BindBufferARB)
	.globl GL_PREFIX(BufferData) ; .set GL_PREFIX(BufferData), GL_PREFIX(BufferDataARB)
	.globl GL_PREFIX(BufferSubData) ; .set GL_PREFIX(BufferSubData), GL_PREFIX(BufferSubDataARB)
	.globl GL_PREFIX(DeleteBuffers) ; .set GL_PREFIX(DeleteBuffers), GL_PREFIX(DeleteBuffersARB)
	.globl GL_PREFIX(GenBuffers) ; .set GL_PREFIX(GenBuffers), GL_PREFIX(GenBuffersARB)
	.globl GL_PREFIX(GetBufferParameteriv) ; .set GL_PREFIX(GetBufferParameteriv), GL_PREFIX(GetBufferParameterivARB)
	.globl GL_PREFIX(GetBufferPointerv) ; .set GL_PREFIX(GetBufferPointerv), GL_PREFIX(GetBufferPointervARB)
	.globl GL_PREFIX(GetBufferSubData) ; .set GL_PREFIX(GetBufferSubData), GL_PREFIX(GetBufferSubDataARB)
	.globl GL_PREFIX(IsBuffer) ; .set GL_PREFIX(IsBuffer), GL_PREFIX(IsBufferARB)
	.globl GL_PREFIX(MapBuffer) ; .set GL_PREFIX(MapBuffer), GL_PREFIX(MapBufferARB)
	.globl GL_PREFIX(UnmapBuffer) ; .set GL_PREFIX(UnmapBuffer), GL_PREFIX(UnmapBufferARB)
	.globl GL_PREFIX(BeginQuery) ; .set GL_PREFIX(BeginQuery), GL_PREFIX(BeginQueryARB)
	.globl GL_PREFIX(DeleteQueries) ; .set GL_PREFIX(DeleteQueries), GL_PREFIX(DeleteQueriesARB)
	.globl GL_PREFIX(EndQuery) ; .set GL_PREFIX(EndQuery), GL_PREFIX(EndQueryARB)
	.globl GL_PREFIX(GenQueries) ; .set GL_PREFIX(GenQueries), GL_PREFIX(GenQueriesARB)
	.globl GL_PREFIX(GetQueryObjectiv) ; .set GL_PREFIX(GetQueryObjectiv), GL_PREFIX(GetQueryObjectivARB)
	.globl GL_PREFIX(GetQueryObjectuiv) ; .set GL_PREFIX(GetQueryObjectuiv), GL_PREFIX(GetQueryObjectuivARB)
	.globl GL_PREFIX(GetQueryiv) ; .set GL_PREFIX(GetQueryiv), GL_PREFIX(GetQueryivARB)
	.globl GL_PREFIX(IsQuery) ; .set GL_PREFIX(IsQuery), GL_PREFIX(IsQueryARB)
	.globl GL_PREFIX(CompileShader) ; .set GL_PREFIX(CompileShader), GL_PREFIX(CompileShaderARB)
	.globl GL_PREFIX(GetActiveUniform) ; .set GL_PREFIX(GetActiveUniform), GL_PREFIX(GetActiveUniformARB)
	.globl GL_PREFIX(GetShaderSource) ; .set GL_PREFIX(GetShaderSource), GL_PREFIX(GetShaderSourceARB)
	.globl GL_PREFIX(GetUniformLocation) ; .set GL_PREFIX(GetUniformLocation), GL_PREFIX(GetUniformLocationARB)
	.globl GL_PREFIX(GetUniformfv) ; .set GL_PREFIX(GetUniformfv), GL_PREFIX(GetUniformfvARB)
	.globl GL_PREFIX(GetUniformiv) ; .set GL_PREFIX(GetUniformiv), GL_PREFIX(GetUniformivARB)
	.globl GL_PREFIX(LinkProgram) ; .set GL_PREFIX(LinkProgram), GL_PREFIX(LinkProgramARB)
	.globl GL_PREFIX(ShaderSource) ; .set GL_PREFIX(ShaderSource), GL_PREFIX(ShaderSourceARB)
	.globl GL_PREFIX(Uniform1f) ; .set GL_PREFIX(Uniform1f), GL_PREFIX(Uniform1fARB)
	.globl GL_PREFIX(Uniform1fv) ; .set GL_PREFIX(Uniform1fv), GL_PREFIX(Uniform1fvARB)
	.globl GL_PREFIX(Uniform1i) ; .set GL_PREFIX(Uniform1i), GL_PREFIX(Uniform1iARB)
	.globl GL_PREFIX(Uniform1iv) ; .set GL_PREFIX(Uniform1iv), GL_PREFIX(Uniform1ivARB)
	.globl GL_PREFIX(Uniform2f) ; .set GL_PREFIX(Uniform2f), GL_PREFIX(Uniform2fARB)
	.globl GL_PREFIX(Uniform2fv) ; .set GL_PREFIX(Uniform2fv), GL_PREFIX(Uniform2fvARB)
	.globl GL_PREFIX(Uniform2i) ; .set GL_PREFIX(Uniform2i), GL_PREFIX(Uniform2iARB)
	.globl GL_PREFIX(Uniform2iv) ; .set GL_PREFIX(Uniform2iv), GL_PREFIX(Uniform2ivARB)
	.globl GL_PREFIX(Uniform3f) ; .set GL_PREFIX(Uniform3f), GL_PREFIX(Uniform3fARB)
	.globl GL_PREFIX(Uniform3fv) ; .set GL_PREFIX(Uniform3fv), GL_PREFIX(Uniform3fvARB)
	.globl GL_PREFIX(Uniform3i) ; .set GL_PREFIX(Uniform3i), GL_PREFIX(Uniform3iARB)
	.globl GL_PREFIX(Uniform3iv) ; .set GL_PREFIX(Uniform3iv), GL_PREFIX(Uniform3ivARB)
	.globl GL_PREFIX(Uniform4f) ; .set GL_PREFIX(Uniform4f), GL_PREFIX(Uniform4fARB)
	.globl GL_PREFIX(Uniform4fv) ; .set GL_PREFIX(Uniform4fv), GL_PREFIX(Uniform4fvARB)
	.globl GL_PREFIX(Uniform4i) ; .set GL_PREFIX(Uniform4i), GL_PREFIX(Uniform4iARB)
	.globl GL_PREFIX(Uniform4iv) ; .set GL_PREFIX(Uniform4iv), GL_PREFIX(Uniform4ivARB)
	.globl GL_PREFIX(UniformMatrix2fv) ; .set GL_PREFIX(UniformMatrix2fv), GL_PREFIX(UniformMatrix2fvARB)
	.globl GL_PREFIX(UniformMatrix3fv) ; .set GL_PREFIX(UniformMatrix3fv), GL_PREFIX(UniformMatrix3fvARB)
	.globl GL_PREFIX(UniformMatrix4fv) ; .set GL_PREFIX(UniformMatrix4fv), GL_PREFIX(UniformMatrix4fvARB)
	.globl GL_PREFIX(UseProgram) ; .set GL_PREFIX(UseProgram), GL_PREFIX(UseProgramObjectARB)
	.globl GL_PREFIX(ValidateProgram) ; .set GL_PREFIX(ValidateProgram), GL_PREFIX(ValidateProgramARB)
	.globl GL_PREFIX(BindAttribLocation) ; .set GL_PREFIX(BindAttribLocation), GL_PREFIX(BindAttribLocationARB)
	.globl GL_PREFIX(GetActiveAttrib) ; .set GL_PREFIX(GetActiveAttrib), GL_PREFIX(GetActiveAttribARB)
	.globl GL_PREFIX(GetAttribLocation) ; .set GL_PREFIX(GetAttribLocation), GL_PREFIX(GetAttribLocationARB)
	.globl GL_PREFIX(DrawBuffers) ; .set GL_PREFIX(DrawBuffers), GL_PREFIX(DrawBuffersARB)
	.globl GL_PREFIX(DrawBuffersATI) ; .set GL_PREFIX(DrawBuffersATI), GL_PREFIX(DrawBuffersARB)
	.globl GL_PREFIX(PointParameterf) ; .set GL_PREFIX(PointParameterf), GL_PREFIX(PointParameterfEXT)
	.globl GL_PREFIX(PointParameterfARB) ; .set GL_PREFIX(PointParameterfARB), GL_PREFIX(PointParameterfEXT)
	.globl GL_PREFIX(PointParameterfv) ; .set GL_PREFIX(PointParameterfv), GL_PREFIX(PointParameterfvEXT)
	.globl GL_PREFIX(PointParameterfvARB) ; .set GL_PREFIX(PointParameterfvARB), GL_PREFIX(PointParameterfvEXT)
	.globl GL_PREFIX(SecondaryColor3b) ; .set GL_PREFIX(SecondaryColor3b), GL_PREFIX(SecondaryColor3bEXT)
	.globl GL_PREFIX(SecondaryColor3bv) ; .set GL_PREFIX(SecondaryColor3bv), GL_PREFIX(SecondaryColor3bvEXT)
	.globl GL_PREFIX(SecondaryColor3d) ; .set GL_PREFIX(SecondaryColor3d), GL_PREFIX(SecondaryColor3dEXT)
	.globl GL_PREFIX(SecondaryColor3dv) ; .set GL_PREFIX(SecondaryColor3dv), GL_PREFIX(SecondaryColor3dvEXT)
	.globl GL_PREFIX(SecondaryColor3f) ; .set GL_PREFIX(SecondaryColor3f), GL_PREFIX(SecondaryColor3fEXT)
	.globl GL_PREFIX(SecondaryColor3fv) ; .set GL_PREFIX(SecondaryColor3fv), GL_PREFIX(SecondaryColor3fvEXT)
	.globl GL_PREFIX(SecondaryColor3i) ; .set GL_PREFIX(SecondaryColor3i), GL_PREFIX(SecondaryColor3iEXT)
	.globl GL_PREFIX(SecondaryColor3iv) ; .set GL_PREFIX(SecondaryColor3iv), GL_PREFIX(SecondaryColor3ivEXT)
	.globl GL_PREFIX(SecondaryColor3s) ; .set GL_PREFIX(SecondaryColor3s), GL_PREFIX(SecondaryColor3sEXT)
	.globl GL_PREFIX(SecondaryColor3sv) ; .set GL_PREFIX(SecondaryColor3sv), GL_PREFIX(SecondaryColor3svEXT)
	.globl GL_PREFIX(SecondaryColor3ub) ; .set GL_PREFIX(SecondaryColor3ub), GL_PREFIX(SecondaryColor3ubEXT)
	.globl GL_PREFIX(SecondaryColor3ubv) ; .set GL_PREFIX(SecondaryColor3ubv), GL_PREFIX(SecondaryColor3ubvEXT)
	.globl GL_PREFIX(SecondaryColor3ui) ; .set GL_PREFIX(SecondaryColor3ui), GL_PREFIX(SecondaryColor3uiEXT)
	.globl GL_PREFIX(SecondaryColor3uiv) ; .set GL_PREFIX(SecondaryColor3uiv), GL_PREFIX(SecondaryColor3uivEXT)
	.globl GL_PREFIX(SecondaryColor3us) ; .set GL_PREFIX(SecondaryColor3us), GL_PREFIX(SecondaryColor3usEXT)
	.globl GL_PREFIX(SecondaryColor3usv) ; .set GL_PREFIX(SecondaryColor3usv), GL_PREFIX(SecondaryColor3usvEXT)
	.globl GL_PREFIX(SecondaryColorPointer) ; .set GL_PREFIX(SecondaryColorPointer), GL_PREFIX(SecondaryColorPointerEXT)
	.globl GL_PREFIX(MultiDrawArrays) ; .set GL_PREFIX(MultiDrawArrays), GL_PREFIX(MultiDrawArraysEXT)
	.globl GL_PREFIX(MultiDrawElements) ; .set GL_PREFIX(MultiDrawElements), GL_PREFIX(MultiDrawElementsEXT)
	.globl GL_PREFIX(FogCoordPointer) ; .set GL_PREFIX(FogCoordPointer), GL_PREFIX(FogCoordPointerEXT)
	.globl GL_PREFIX(FogCoordd) ; .set GL_PREFIX(FogCoordd), GL_PREFIX(FogCoorddEXT)
	.globl GL_PREFIX(FogCoorddv) ; .set GL_PREFIX(FogCoorddv), GL_PREFIX(FogCoorddvEXT)
	.globl GL_PREFIX(FogCoordf) ; .set GL_PREFIX(FogCoordf), GL_PREFIX(FogCoordfEXT)
	.globl GL_PREFIX(FogCoordfv) ; .set GL_PREFIX(FogCoordfv), GL_PREFIX(FogCoordfvEXT)
	.globl GL_PREFIX(BlendFuncSeparate) ; .set GL_PREFIX(BlendFuncSeparate), GL_PREFIX(BlendFuncSeparateEXT)
	.globl GL_PREFIX(WindowPos2d) ; .set GL_PREFIX(WindowPos2d), GL_PREFIX(WindowPos2dMESA)
	.globl GL_PREFIX(WindowPos2dARB) ; .set GL_PREFIX(WindowPos2dARB), GL_PREFIX(WindowPos2dMESA)
	.globl GL_PREFIX(WindowPos2dv) ; .set GL_PREFIX(WindowPos2dv), GL_PREFIX(WindowPos2dvMESA)
	.globl GL_PREFIX(WindowPos2dvARB) ; .set GL_PREFIX(WindowPos2dvARB), GL_PREFIX(WindowPos2dvMESA)
	.globl GL_PREFIX(WindowPos2f) ; .set GL_PREFIX(WindowPos2f), GL_PREFIX(WindowPos2fMESA)
	.globl GL_PREFIX(WindowPos2fARB) ; .set GL_PREFIX(WindowPos2fARB), GL_PREFIX(WindowPos2fMESA)
	.globl GL_PREFIX(WindowPos2fv) ; .set GL_PREFIX(WindowPos2fv), GL_PREFIX(WindowPos2fvMESA)
	.globl GL_PREFIX(WindowPos2fvARB) ; .set GL_PREFIX(WindowPos2fvARB), GL_PREFIX(WindowPos2fvMESA)
	.globl GL_PREFIX(WindowPos2i) ; .set GL_PREFIX(WindowPos2i), GL_PREFIX(WindowPos2iMESA)
	.globl GL_PREFIX(WindowPos2iARB) ; .set GL_PREFIX(WindowPos2iARB), GL_PREFIX(WindowPos2iMESA)
	.globl GL_PREFIX(WindowPos2iv) ; .set GL_PREFIX(WindowPos2iv), GL_PREFIX(WindowPos2ivMESA)
	.globl GL_PREFIX(WindowPos2ivARB) ; .set GL_PREFIX(WindowPos2ivARB), GL_PREFIX(WindowPos2ivMESA)
	.globl GL_PREFIX(WindowPos2s) ; .set GL_PREFIX(WindowPos2s), GL_PREFIX(WindowPos2sMESA)
	.globl GL_PREFIX(WindowPos2sARB) ; .set GL_PREFIX(WindowPos2sARB), GL_PREFIX(WindowPos2sMESA)
	.globl GL_PREFIX(WindowPos2sv) ; .set GL_PREFIX(WindowPos2sv), GL_PREFIX(WindowPos2svMESA)
	.globl GL_PREFIX(WindowPos2svARB) ; .set GL_PREFIX(WindowPos2svARB), GL_PREFIX(WindowPos2svMESA)
	.globl GL_PREFIX(WindowPos3d) ; .set GL_PREFIX(WindowPos3d), GL_PREFIX(WindowPos3dMESA)
	.globl GL_PREFIX(WindowPos3dARB) ; .set GL_PREFIX(WindowPos3dARB), GL_PREFIX(WindowPos3dMESA)
	.globl GL_PREFIX(WindowPos3dv) ; .set GL_PREFIX(WindowPos3dv), GL_PREFIX(WindowPos3dvMESA)
	.globl GL_PREFIX(WindowPos3dvARB) ; .set GL_PREFIX(WindowPos3dvARB), GL_PREFIX(WindowPos3dvMESA)
	.globl GL_PREFIX(WindowPos3f) ; .set GL_PREFIX(WindowPos3f), GL_PREFIX(WindowPos3fMESA)
	.globl GL_PREFIX(WindowPos3fARB) ; .set GL_PREFIX(WindowPos3fARB), GL_PREFIX(WindowPos3fMESA)
	.globl GL_PREFIX(WindowPos3fv) ; .set GL_PREFIX(WindowPos3fv), GL_PREFIX(WindowPos3fvMESA)
	.globl GL_PREFIX(WindowPos3fvARB) ; .set GL_PREFIX(WindowPos3fvARB), GL_PREFIX(WindowPos3fvMESA)
	.globl GL_PREFIX(WindowPos3i) ; .set GL_PREFIX(WindowPos3i), GL_PREFIX(WindowPos3iMESA)
	.globl GL_PREFIX(WindowPos3iARB) ; .set GL_PREFIX(WindowPos3iARB), GL_PREFIX(WindowPos3iMESA)
	.globl GL_PREFIX(WindowPos3iv) ; .set GL_PREFIX(WindowPos3iv), GL_PREFIX(WindowPos3ivMESA)
	.globl GL_PREFIX(WindowPos3ivARB) ; .set GL_PREFIX(WindowPos3ivARB), GL_PREFIX(WindowPos3ivMESA)
	.globl GL_PREFIX(WindowPos3s) ; .set GL_PREFIX(WindowPos3s), GL_PREFIX(WindowPos3sMESA)
	.globl GL_PREFIX(WindowPos3sARB) ; .set GL_PREFIX(WindowPos3sARB), GL_PREFIX(WindowPos3sMESA)
	.globl GL_PREFIX(WindowPos3sv) ; .set GL_PREFIX(WindowPos3sv), GL_PREFIX(WindowPos3svMESA)
	.globl GL_PREFIX(WindowPos3svARB) ; .set GL_PREFIX(WindowPos3svARB), GL_PREFIX(WindowPos3svMESA)
	.globl GL_PREFIX(BindProgramARB) ; .set GL_PREFIX(BindProgramARB), GL_PREFIX(BindProgramNV)
	.globl GL_PREFIX(DeleteProgramsARB) ; .set GL_PREFIX(DeleteProgramsARB), GL_PREFIX(DeleteProgramsNV)
	.globl GL_PREFIX(GenProgramsARB) ; .set GL_PREFIX(GenProgramsARB), GL_PREFIX(GenProgramsNV)
	.globl GL_PREFIX(GetVertexAttribPointerv) ; .set GL_PREFIX(GetVertexAttribPointerv), GL_PREFIX(GetVertexAttribPointervNV)
	.globl GL_PREFIX(GetVertexAttribPointervARB) ; .set GL_PREFIX(GetVertexAttribPointervARB), GL_PREFIX(GetVertexAttribPointervNV)
	.globl GL_PREFIX(IsProgramARB) ; .set GL_PREFIX(IsProgramARB), GL_PREFIX(IsProgramNV)
	.globl GL_PREFIX(PointParameteri) ; .set GL_PREFIX(PointParameteri), GL_PREFIX(PointParameteriNV)
	.globl GL_PREFIX(PointParameteriv) ; .set GL_PREFIX(PointParameteriv), GL_PREFIX(PointParameterivNV)
	.globl GL_PREFIX(BlendEquationSeparate) ; .set GL_PREFIX(BlendEquationSeparate), GL_PREFIX(_dispatch_stub_745)

#if defined(GLX_USE_TLS) && defined(__linux__)
	.section ".note.ABI-tag", "a"
	.p2align 2
	.long	1f - 0f   /* name length */
	.long	3f - 2f   /* data length */
	.long	1         /* note length */
0:	.asciz "GNU"      /* vendor name */
1:	.p2align 2
2:	.long	0         /* note data: the ABI tag */
	.long	2,4,20    /* Minimum kernel version w/TLS */
3:	.p2align 2        /* pad out section */
#endif /* GLX_USE_TLS */

#if defined (__ELF__) && defined (__linux__)
	.section .note.GNU-stack,"",%progbits
#endif
