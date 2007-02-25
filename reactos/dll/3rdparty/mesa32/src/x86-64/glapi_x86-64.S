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
#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 303
#  pragma GCC visibility push(default)
#  define HIDDEN(x) .hidden x
#else
#  define HIDDEN(x)
#endif

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
	.globl	glNewList
	.type	glNewList, @function
glNewList:
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
	.size	glNewList, .-glNewList

	.p2align	4,,15
	.globl	glEndList
	.type	glEndList, @function
glEndList:
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
	.size	glEndList, .-glEndList

	.p2align	4,,15
	.globl	glCallList
	.type	glCallList, @function
glCallList:
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
	.size	glCallList, .-glCallList

	.p2align	4,,15
	.globl	glCallLists
	.type	glCallLists, @function
glCallLists:
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
	.size	glCallLists, .-glCallLists

	.p2align	4,,15
	.globl	glDeleteLists
	.type	glDeleteLists, @function
glDeleteLists:
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
	.size	glDeleteLists, .-glDeleteLists

	.p2align	4,,15
	.globl	glGenLists
	.type	glGenLists, @function
glGenLists:
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
	.size	glGenLists, .-glGenLists

	.p2align	4,,15
	.globl	glListBase
	.type	glListBase, @function
glListBase:
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
	.size	glListBase, .-glListBase

	.p2align	4,,15
	.globl	glBegin
	.type	glBegin, @function
glBegin:
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
	.size	glBegin, .-glBegin

	.p2align	4,,15
	.globl	glBitmap
	.type	glBitmap, @function
glBitmap:
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
	.size	glBitmap, .-glBitmap

	.p2align	4,,15
	.globl	glColor3b
	.type	glColor3b, @function
glColor3b:
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
	.size	glColor3b, .-glColor3b

	.p2align	4,,15
	.globl	glColor3bv
	.type	glColor3bv, @function
glColor3bv:
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
	.size	glColor3bv, .-glColor3bv

	.p2align	4,,15
	.globl	glColor3d
	.type	glColor3d, @function
glColor3d:
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
	.size	glColor3d, .-glColor3d

	.p2align	4,,15
	.globl	glColor3dv
	.type	glColor3dv, @function
glColor3dv:
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
	.size	glColor3dv, .-glColor3dv

	.p2align	4,,15
	.globl	glColor3f
	.type	glColor3f, @function
glColor3f:
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
	.size	glColor3f, .-glColor3f

	.p2align	4,,15
	.globl	glColor3fv
	.type	glColor3fv, @function
glColor3fv:
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
	.size	glColor3fv, .-glColor3fv

	.p2align	4,,15
	.globl	glColor3i
	.type	glColor3i, @function
glColor3i:
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
	.size	glColor3i, .-glColor3i

	.p2align	4,,15
	.globl	glColor3iv
	.type	glColor3iv, @function
glColor3iv:
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
	.size	glColor3iv, .-glColor3iv

	.p2align	4,,15
	.globl	glColor3s
	.type	glColor3s, @function
glColor3s:
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
	.size	glColor3s, .-glColor3s

	.p2align	4,,15
	.globl	glColor3sv
	.type	glColor3sv, @function
glColor3sv:
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
	.size	glColor3sv, .-glColor3sv

	.p2align	4,,15
	.globl	glColor3ub
	.type	glColor3ub, @function
glColor3ub:
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
	.size	glColor3ub, .-glColor3ub

	.p2align	4,,15
	.globl	glColor3ubv
	.type	glColor3ubv, @function
glColor3ubv:
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
	.size	glColor3ubv, .-glColor3ubv

	.p2align	4,,15
	.globl	glColor3ui
	.type	glColor3ui, @function
glColor3ui:
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
	.size	glColor3ui, .-glColor3ui

	.p2align	4,,15
	.globl	glColor3uiv
	.type	glColor3uiv, @function
glColor3uiv:
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
	.size	glColor3uiv, .-glColor3uiv

	.p2align	4,,15
	.globl	glColor3us
	.type	glColor3us, @function
glColor3us:
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
	.size	glColor3us, .-glColor3us

	.p2align	4,,15
	.globl	glColor3usv
	.type	glColor3usv, @function
glColor3usv:
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
	.size	glColor3usv, .-glColor3usv

	.p2align	4,,15
	.globl	glColor4b
	.type	glColor4b, @function
glColor4b:
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
	.size	glColor4b, .-glColor4b

	.p2align	4,,15
	.globl	glColor4bv
	.type	glColor4bv, @function
glColor4bv:
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
	.size	glColor4bv, .-glColor4bv

	.p2align	4,,15
	.globl	glColor4d
	.type	glColor4d, @function
glColor4d:
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
	.size	glColor4d, .-glColor4d

	.p2align	4,,15
	.globl	glColor4dv
	.type	glColor4dv, @function
glColor4dv:
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
	.size	glColor4dv, .-glColor4dv

	.p2align	4,,15
	.globl	glColor4f
	.type	glColor4f, @function
glColor4f:
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
	.size	glColor4f, .-glColor4f

	.p2align	4,,15
	.globl	glColor4fv
	.type	glColor4fv, @function
glColor4fv:
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
	.size	glColor4fv, .-glColor4fv

	.p2align	4,,15
	.globl	glColor4i
	.type	glColor4i, @function
glColor4i:
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
	.size	glColor4i, .-glColor4i

	.p2align	4,,15
	.globl	glColor4iv
	.type	glColor4iv, @function
glColor4iv:
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
	.size	glColor4iv, .-glColor4iv

	.p2align	4,,15
	.globl	glColor4s
	.type	glColor4s, @function
glColor4s:
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
	.size	glColor4s, .-glColor4s

	.p2align	4,,15
	.globl	glColor4sv
	.type	glColor4sv, @function
glColor4sv:
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
	.size	glColor4sv, .-glColor4sv

	.p2align	4,,15
	.globl	glColor4ub
	.type	glColor4ub, @function
glColor4ub:
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
	.size	glColor4ub, .-glColor4ub

	.p2align	4,,15
	.globl	glColor4ubv
	.type	glColor4ubv, @function
glColor4ubv:
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
	.size	glColor4ubv, .-glColor4ubv

	.p2align	4,,15
	.globl	glColor4ui
	.type	glColor4ui, @function
glColor4ui:
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
	.size	glColor4ui, .-glColor4ui

	.p2align	4,,15
	.globl	glColor4uiv
	.type	glColor4uiv, @function
glColor4uiv:
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
	.size	glColor4uiv, .-glColor4uiv

	.p2align	4,,15
	.globl	glColor4us
	.type	glColor4us, @function
glColor4us:
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
	.size	glColor4us, .-glColor4us

	.p2align	4,,15
	.globl	glColor4usv
	.type	glColor4usv, @function
glColor4usv:
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
	.size	glColor4usv, .-glColor4usv

	.p2align	4,,15
	.globl	glEdgeFlag
	.type	glEdgeFlag, @function
glEdgeFlag:
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
	.size	glEdgeFlag, .-glEdgeFlag

	.p2align	4,,15
	.globl	glEdgeFlagv
	.type	glEdgeFlagv, @function
glEdgeFlagv:
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
	.size	glEdgeFlagv, .-glEdgeFlagv

	.p2align	4,,15
	.globl	glEnd
	.type	glEnd, @function
glEnd:
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
	.size	glEnd, .-glEnd

	.p2align	4,,15
	.globl	glIndexd
	.type	glIndexd, @function
glIndexd:
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
	.size	glIndexd, .-glIndexd

	.p2align	4,,15
	.globl	glIndexdv
	.type	glIndexdv, @function
glIndexdv:
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
	.size	glIndexdv, .-glIndexdv

	.p2align	4,,15
	.globl	glIndexf
	.type	glIndexf, @function
glIndexf:
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
	.size	glIndexf, .-glIndexf

	.p2align	4,,15
	.globl	glIndexfv
	.type	glIndexfv, @function
glIndexfv:
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
	.size	glIndexfv, .-glIndexfv

	.p2align	4,,15
	.globl	glIndexi
	.type	glIndexi, @function
glIndexi:
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
	.size	glIndexi, .-glIndexi

	.p2align	4,,15
	.globl	glIndexiv
	.type	glIndexiv, @function
glIndexiv:
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
	.size	glIndexiv, .-glIndexiv

	.p2align	4,,15
	.globl	glIndexs
	.type	glIndexs, @function
glIndexs:
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
	.size	glIndexs, .-glIndexs

	.p2align	4,,15
	.globl	glIndexsv
	.type	glIndexsv, @function
glIndexsv:
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
	.size	glIndexsv, .-glIndexsv

	.p2align	4,,15
	.globl	glNormal3b
	.type	glNormal3b, @function
glNormal3b:
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
	.size	glNormal3b, .-glNormal3b

	.p2align	4,,15
	.globl	glNormal3bv
	.type	glNormal3bv, @function
glNormal3bv:
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
	.size	glNormal3bv, .-glNormal3bv

	.p2align	4,,15
	.globl	glNormal3d
	.type	glNormal3d, @function
glNormal3d:
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
	.size	glNormal3d, .-glNormal3d

	.p2align	4,,15
	.globl	glNormal3dv
	.type	glNormal3dv, @function
glNormal3dv:
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
	.size	glNormal3dv, .-glNormal3dv

	.p2align	4,,15
	.globl	glNormal3f
	.type	glNormal3f, @function
glNormal3f:
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
	.size	glNormal3f, .-glNormal3f

	.p2align	4,,15
	.globl	glNormal3fv
	.type	glNormal3fv, @function
glNormal3fv:
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
	.size	glNormal3fv, .-glNormal3fv

	.p2align	4,,15
	.globl	glNormal3i
	.type	glNormal3i, @function
glNormal3i:
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
	.size	glNormal3i, .-glNormal3i

	.p2align	4,,15
	.globl	glNormal3iv
	.type	glNormal3iv, @function
glNormal3iv:
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
	.size	glNormal3iv, .-glNormal3iv

	.p2align	4,,15
	.globl	glNormal3s
	.type	glNormal3s, @function
glNormal3s:
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
	.size	glNormal3s, .-glNormal3s

	.p2align	4,,15
	.globl	glNormal3sv
	.type	glNormal3sv, @function
glNormal3sv:
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
	.size	glNormal3sv, .-glNormal3sv

	.p2align	4,,15
	.globl	glRasterPos2d
	.type	glRasterPos2d, @function
glRasterPos2d:
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
	.size	glRasterPos2d, .-glRasterPos2d

	.p2align	4,,15
	.globl	glRasterPos2dv
	.type	glRasterPos2dv, @function
glRasterPos2dv:
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
	.size	glRasterPos2dv, .-glRasterPos2dv

	.p2align	4,,15
	.globl	glRasterPos2f
	.type	glRasterPos2f, @function
glRasterPos2f:
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
	.size	glRasterPos2f, .-glRasterPos2f

	.p2align	4,,15
	.globl	glRasterPos2fv
	.type	glRasterPos2fv, @function
glRasterPos2fv:
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
	.size	glRasterPos2fv, .-glRasterPos2fv

	.p2align	4,,15
	.globl	glRasterPos2i
	.type	glRasterPos2i, @function
glRasterPos2i:
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
	.size	glRasterPos2i, .-glRasterPos2i

	.p2align	4,,15
	.globl	glRasterPos2iv
	.type	glRasterPos2iv, @function
glRasterPos2iv:
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
	.size	glRasterPos2iv, .-glRasterPos2iv

	.p2align	4,,15
	.globl	glRasterPos2s
	.type	glRasterPos2s, @function
glRasterPos2s:
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
	.size	glRasterPos2s, .-glRasterPos2s

	.p2align	4,,15
	.globl	glRasterPos2sv
	.type	glRasterPos2sv, @function
glRasterPos2sv:
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
	.size	glRasterPos2sv, .-glRasterPos2sv

	.p2align	4,,15
	.globl	glRasterPos3d
	.type	glRasterPos3d, @function
glRasterPos3d:
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
	.size	glRasterPos3d, .-glRasterPos3d

	.p2align	4,,15
	.globl	glRasterPos3dv
	.type	glRasterPos3dv, @function
glRasterPos3dv:
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
	.size	glRasterPos3dv, .-glRasterPos3dv

	.p2align	4,,15
	.globl	glRasterPos3f
	.type	glRasterPos3f, @function
glRasterPos3f:
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
	.size	glRasterPos3f, .-glRasterPos3f

	.p2align	4,,15
	.globl	glRasterPos3fv
	.type	glRasterPos3fv, @function
glRasterPos3fv:
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
	.size	glRasterPos3fv, .-glRasterPos3fv

	.p2align	4,,15
	.globl	glRasterPos3i
	.type	glRasterPos3i, @function
glRasterPos3i:
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
	.size	glRasterPos3i, .-glRasterPos3i

	.p2align	4,,15
	.globl	glRasterPos3iv
	.type	glRasterPos3iv, @function
glRasterPos3iv:
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
	.size	glRasterPos3iv, .-glRasterPos3iv

	.p2align	4,,15
	.globl	glRasterPos3s
	.type	glRasterPos3s, @function
glRasterPos3s:
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
	.size	glRasterPos3s, .-glRasterPos3s

	.p2align	4,,15
	.globl	glRasterPos3sv
	.type	glRasterPos3sv, @function
glRasterPos3sv:
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
	.size	glRasterPos3sv, .-glRasterPos3sv

	.p2align	4,,15
	.globl	glRasterPos4d
	.type	glRasterPos4d, @function
glRasterPos4d:
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
	.size	glRasterPos4d, .-glRasterPos4d

	.p2align	4,,15
	.globl	glRasterPos4dv
	.type	glRasterPos4dv, @function
glRasterPos4dv:
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
	.size	glRasterPos4dv, .-glRasterPos4dv

	.p2align	4,,15
	.globl	glRasterPos4f
	.type	glRasterPos4f, @function
glRasterPos4f:
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
	.size	glRasterPos4f, .-glRasterPos4f

	.p2align	4,,15
	.globl	glRasterPos4fv
	.type	glRasterPos4fv, @function
glRasterPos4fv:
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
	.size	glRasterPos4fv, .-glRasterPos4fv

	.p2align	4,,15
	.globl	glRasterPos4i
	.type	glRasterPos4i, @function
glRasterPos4i:
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
	.size	glRasterPos4i, .-glRasterPos4i

	.p2align	4,,15
	.globl	glRasterPos4iv
	.type	glRasterPos4iv, @function
glRasterPos4iv:
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
	.size	glRasterPos4iv, .-glRasterPos4iv

	.p2align	4,,15
	.globl	glRasterPos4s
	.type	glRasterPos4s, @function
glRasterPos4s:
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
	.size	glRasterPos4s, .-glRasterPos4s

	.p2align	4,,15
	.globl	glRasterPos4sv
	.type	glRasterPos4sv, @function
glRasterPos4sv:
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
	.size	glRasterPos4sv, .-glRasterPos4sv

	.p2align	4,,15
	.globl	glRectd
	.type	glRectd, @function
glRectd:
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
	.size	glRectd, .-glRectd

	.p2align	4,,15
	.globl	glRectdv
	.type	glRectdv, @function
glRectdv:
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
	.size	glRectdv, .-glRectdv

	.p2align	4,,15
	.globl	glRectf
	.type	glRectf, @function
glRectf:
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
	.size	glRectf, .-glRectf

	.p2align	4,,15
	.globl	glRectfv
	.type	glRectfv, @function
glRectfv:
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
	.size	glRectfv, .-glRectfv

	.p2align	4,,15
	.globl	glRecti
	.type	glRecti, @function
glRecti:
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
	.size	glRecti, .-glRecti

	.p2align	4,,15
	.globl	glRectiv
	.type	glRectiv, @function
glRectiv:
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
	.size	glRectiv, .-glRectiv

	.p2align	4,,15
	.globl	glRects
	.type	glRects, @function
glRects:
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
	.size	glRects, .-glRects

	.p2align	4,,15
	.globl	glRectsv
	.type	glRectsv, @function
glRectsv:
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
	.size	glRectsv, .-glRectsv

	.p2align	4,,15
	.globl	glTexCoord1d
	.type	glTexCoord1d, @function
glTexCoord1d:
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
	.size	glTexCoord1d, .-glTexCoord1d

	.p2align	4,,15
	.globl	glTexCoord1dv
	.type	glTexCoord1dv, @function
glTexCoord1dv:
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
	.size	glTexCoord1dv, .-glTexCoord1dv

	.p2align	4,,15
	.globl	glTexCoord1f
	.type	glTexCoord1f, @function
glTexCoord1f:
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
	.size	glTexCoord1f, .-glTexCoord1f

	.p2align	4,,15
	.globl	glTexCoord1fv
	.type	glTexCoord1fv, @function
glTexCoord1fv:
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
	.size	glTexCoord1fv, .-glTexCoord1fv

	.p2align	4,,15
	.globl	glTexCoord1i
	.type	glTexCoord1i, @function
glTexCoord1i:
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
	.size	glTexCoord1i, .-glTexCoord1i

	.p2align	4,,15
	.globl	glTexCoord1iv
	.type	glTexCoord1iv, @function
glTexCoord1iv:
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
	.size	glTexCoord1iv, .-glTexCoord1iv

	.p2align	4,,15
	.globl	glTexCoord1s
	.type	glTexCoord1s, @function
glTexCoord1s:
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
	.size	glTexCoord1s, .-glTexCoord1s

	.p2align	4,,15
	.globl	glTexCoord1sv
	.type	glTexCoord1sv, @function
glTexCoord1sv:
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
	.size	glTexCoord1sv, .-glTexCoord1sv

	.p2align	4,,15
	.globl	glTexCoord2d
	.type	glTexCoord2d, @function
glTexCoord2d:
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
	.size	glTexCoord2d, .-glTexCoord2d

	.p2align	4,,15
	.globl	glTexCoord2dv
	.type	glTexCoord2dv, @function
glTexCoord2dv:
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
	.size	glTexCoord2dv, .-glTexCoord2dv

	.p2align	4,,15
	.globl	glTexCoord2f
	.type	glTexCoord2f, @function
glTexCoord2f:
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
	.size	glTexCoord2f, .-glTexCoord2f

	.p2align	4,,15
	.globl	glTexCoord2fv
	.type	glTexCoord2fv, @function
glTexCoord2fv:
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
	.size	glTexCoord2fv, .-glTexCoord2fv

	.p2align	4,,15
	.globl	glTexCoord2i
	.type	glTexCoord2i, @function
glTexCoord2i:
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
	.size	glTexCoord2i, .-glTexCoord2i

	.p2align	4,,15
	.globl	glTexCoord2iv
	.type	glTexCoord2iv, @function
glTexCoord2iv:
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
	.size	glTexCoord2iv, .-glTexCoord2iv

	.p2align	4,,15
	.globl	glTexCoord2s
	.type	glTexCoord2s, @function
glTexCoord2s:
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
	.size	glTexCoord2s, .-glTexCoord2s

	.p2align	4,,15
	.globl	glTexCoord2sv
	.type	glTexCoord2sv, @function
glTexCoord2sv:
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
	.size	glTexCoord2sv, .-glTexCoord2sv

	.p2align	4,,15
	.globl	glTexCoord3d
	.type	glTexCoord3d, @function
glTexCoord3d:
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
	.size	glTexCoord3d, .-glTexCoord3d

	.p2align	4,,15
	.globl	glTexCoord3dv
	.type	glTexCoord3dv, @function
glTexCoord3dv:
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
	.size	glTexCoord3dv, .-glTexCoord3dv

	.p2align	4,,15
	.globl	glTexCoord3f
	.type	glTexCoord3f, @function
glTexCoord3f:
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
	.size	glTexCoord3f, .-glTexCoord3f

	.p2align	4,,15
	.globl	glTexCoord3fv
	.type	glTexCoord3fv, @function
glTexCoord3fv:
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
	.size	glTexCoord3fv, .-glTexCoord3fv

	.p2align	4,,15
	.globl	glTexCoord3i
	.type	glTexCoord3i, @function
glTexCoord3i:
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
	.size	glTexCoord3i, .-glTexCoord3i

	.p2align	4,,15
	.globl	glTexCoord3iv
	.type	glTexCoord3iv, @function
glTexCoord3iv:
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
	.size	glTexCoord3iv, .-glTexCoord3iv

	.p2align	4,,15
	.globl	glTexCoord3s
	.type	glTexCoord3s, @function
glTexCoord3s:
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
	.size	glTexCoord3s, .-glTexCoord3s

	.p2align	4,,15
	.globl	glTexCoord3sv
	.type	glTexCoord3sv, @function
glTexCoord3sv:
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
	.size	glTexCoord3sv, .-glTexCoord3sv

	.p2align	4,,15
	.globl	glTexCoord4d
	.type	glTexCoord4d, @function
glTexCoord4d:
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
	.size	glTexCoord4d, .-glTexCoord4d

	.p2align	4,,15
	.globl	glTexCoord4dv
	.type	glTexCoord4dv, @function
glTexCoord4dv:
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
	.size	glTexCoord4dv, .-glTexCoord4dv

	.p2align	4,,15
	.globl	glTexCoord4f
	.type	glTexCoord4f, @function
glTexCoord4f:
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
	.size	glTexCoord4f, .-glTexCoord4f

	.p2align	4,,15
	.globl	glTexCoord4fv
	.type	glTexCoord4fv, @function
glTexCoord4fv:
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
	.size	glTexCoord4fv, .-glTexCoord4fv

	.p2align	4,,15
	.globl	glTexCoord4i
	.type	glTexCoord4i, @function
glTexCoord4i:
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
	.size	glTexCoord4i, .-glTexCoord4i

	.p2align	4,,15
	.globl	glTexCoord4iv
	.type	glTexCoord4iv, @function
glTexCoord4iv:
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
	.size	glTexCoord4iv, .-glTexCoord4iv

	.p2align	4,,15
	.globl	glTexCoord4s
	.type	glTexCoord4s, @function
glTexCoord4s:
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
	.size	glTexCoord4s, .-glTexCoord4s

	.p2align	4,,15
	.globl	glTexCoord4sv
	.type	glTexCoord4sv, @function
glTexCoord4sv:
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
	.size	glTexCoord4sv, .-glTexCoord4sv

	.p2align	4,,15
	.globl	glVertex2d
	.type	glVertex2d, @function
glVertex2d:
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
	.size	glVertex2d, .-glVertex2d

	.p2align	4,,15
	.globl	glVertex2dv
	.type	glVertex2dv, @function
glVertex2dv:
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
	.size	glVertex2dv, .-glVertex2dv

	.p2align	4,,15
	.globl	glVertex2f
	.type	glVertex2f, @function
glVertex2f:
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
	.size	glVertex2f, .-glVertex2f

	.p2align	4,,15
	.globl	glVertex2fv
	.type	glVertex2fv, @function
glVertex2fv:
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
	.size	glVertex2fv, .-glVertex2fv

	.p2align	4,,15
	.globl	glVertex2i
	.type	glVertex2i, @function
glVertex2i:
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
	.size	glVertex2i, .-glVertex2i

	.p2align	4,,15
	.globl	glVertex2iv
	.type	glVertex2iv, @function
glVertex2iv:
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
	.size	glVertex2iv, .-glVertex2iv

	.p2align	4,,15
	.globl	glVertex2s
	.type	glVertex2s, @function
glVertex2s:
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
	.size	glVertex2s, .-glVertex2s

	.p2align	4,,15
	.globl	glVertex2sv
	.type	glVertex2sv, @function
glVertex2sv:
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
	.size	glVertex2sv, .-glVertex2sv

	.p2align	4,,15
	.globl	glVertex3d
	.type	glVertex3d, @function
glVertex3d:
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
	.size	glVertex3d, .-glVertex3d

	.p2align	4,,15
	.globl	glVertex3dv
	.type	glVertex3dv, @function
glVertex3dv:
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
	.size	glVertex3dv, .-glVertex3dv

	.p2align	4,,15
	.globl	glVertex3f
	.type	glVertex3f, @function
glVertex3f:
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
	.size	glVertex3f, .-glVertex3f

	.p2align	4,,15
	.globl	glVertex3fv
	.type	glVertex3fv, @function
glVertex3fv:
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
	.size	glVertex3fv, .-glVertex3fv

	.p2align	4,,15
	.globl	glVertex3i
	.type	glVertex3i, @function
glVertex3i:
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
	.size	glVertex3i, .-glVertex3i

	.p2align	4,,15
	.globl	glVertex3iv
	.type	glVertex3iv, @function
glVertex3iv:
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
	.size	glVertex3iv, .-glVertex3iv

	.p2align	4,,15
	.globl	glVertex3s
	.type	glVertex3s, @function
glVertex3s:
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
	.size	glVertex3s, .-glVertex3s

	.p2align	4,,15
	.globl	glVertex3sv
	.type	glVertex3sv, @function
glVertex3sv:
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
	.size	glVertex3sv, .-glVertex3sv

	.p2align	4,,15
	.globl	glVertex4d
	.type	glVertex4d, @function
glVertex4d:
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
	.size	glVertex4d, .-glVertex4d

	.p2align	4,,15
	.globl	glVertex4dv
	.type	glVertex4dv, @function
glVertex4dv:
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
	.size	glVertex4dv, .-glVertex4dv

	.p2align	4,,15
	.globl	glVertex4f
	.type	glVertex4f, @function
glVertex4f:
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
	.size	glVertex4f, .-glVertex4f

	.p2align	4,,15
	.globl	glVertex4fv
	.type	glVertex4fv, @function
glVertex4fv:
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
	.size	glVertex4fv, .-glVertex4fv

	.p2align	4,,15
	.globl	glVertex4i
	.type	glVertex4i, @function
glVertex4i:
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
	.size	glVertex4i, .-glVertex4i

	.p2align	4,,15
	.globl	glVertex4iv
	.type	glVertex4iv, @function
glVertex4iv:
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
	.size	glVertex4iv, .-glVertex4iv

	.p2align	4,,15
	.globl	glVertex4s
	.type	glVertex4s, @function
glVertex4s:
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
	.size	glVertex4s, .-glVertex4s

	.p2align	4,,15
	.globl	glVertex4sv
	.type	glVertex4sv, @function
glVertex4sv:
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
	.size	glVertex4sv, .-glVertex4sv

	.p2align	4,,15
	.globl	glClipPlane
	.type	glClipPlane, @function
glClipPlane:
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
	.size	glClipPlane, .-glClipPlane

	.p2align	4,,15
	.globl	glColorMaterial
	.type	glColorMaterial, @function
glColorMaterial:
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
	.size	glColorMaterial, .-glColorMaterial

	.p2align	4,,15
	.globl	glCullFace
	.type	glCullFace, @function
glCullFace:
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
	.size	glCullFace, .-glCullFace

	.p2align	4,,15
	.globl	glFogf
	.type	glFogf, @function
glFogf:
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
	.size	glFogf, .-glFogf

	.p2align	4,,15
	.globl	glFogfv
	.type	glFogfv, @function
glFogfv:
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
	.size	glFogfv, .-glFogfv

	.p2align	4,,15
	.globl	glFogi
	.type	glFogi, @function
glFogi:
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
	.size	glFogi, .-glFogi

	.p2align	4,,15
	.globl	glFogiv
	.type	glFogiv, @function
glFogiv:
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
	.size	glFogiv, .-glFogiv

	.p2align	4,,15
	.globl	glFrontFace
	.type	glFrontFace, @function
glFrontFace:
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
	.size	glFrontFace, .-glFrontFace

	.p2align	4,,15
	.globl	glHint
	.type	glHint, @function
glHint:
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
	.size	glHint, .-glHint

	.p2align	4,,15
	.globl	glLightf
	.type	glLightf, @function
glLightf:
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
	.size	glLightf, .-glLightf

	.p2align	4,,15
	.globl	glLightfv
	.type	glLightfv, @function
glLightfv:
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
	.size	glLightfv, .-glLightfv

	.p2align	4,,15
	.globl	glLighti
	.type	glLighti, @function
glLighti:
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
	.size	glLighti, .-glLighti

	.p2align	4,,15
	.globl	glLightiv
	.type	glLightiv, @function
glLightiv:
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
	.size	glLightiv, .-glLightiv

	.p2align	4,,15
	.globl	glLightModelf
	.type	glLightModelf, @function
glLightModelf:
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
	.size	glLightModelf, .-glLightModelf

	.p2align	4,,15
	.globl	glLightModelfv
	.type	glLightModelfv, @function
glLightModelfv:
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
	.size	glLightModelfv, .-glLightModelfv

	.p2align	4,,15
	.globl	glLightModeli
	.type	glLightModeli, @function
glLightModeli:
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
	.size	glLightModeli, .-glLightModeli

	.p2align	4,,15
	.globl	glLightModeliv
	.type	glLightModeliv, @function
glLightModeliv:
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
	.size	glLightModeliv, .-glLightModeliv

	.p2align	4,,15
	.globl	glLineStipple
	.type	glLineStipple, @function
glLineStipple:
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
	.size	glLineStipple, .-glLineStipple

	.p2align	4,,15
	.globl	glLineWidth
	.type	glLineWidth, @function
glLineWidth:
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
	.size	glLineWidth, .-glLineWidth

	.p2align	4,,15
	.globl	glMaterialf
	.type	glMaterialf, @function
glMaterialf:
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
	.size	glMaterialf, .-glMaterialf

	.p2align	4,,15
	.globl	glMaterialfv
	.type	glMaterialfv, @function
glMaterialfv:
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
	.size	glMaterialfv, .-glMaterialfv

	.p2align	4,,15
	.globl	glMateriali
	.type	glMateriali, @function
glMateriali:
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
	.size	glMateriali, .-glMateriali

	.p2align	4,,15
	.globl	glMaterialiv
	.type	glMaterialiv, @function
glMaterialiv:
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
	.size	glMaterialiv, .-glMaterialiv

	.p2align	4,,15
	.globl	glPointSize
	.type	glPointSize, @function
glPointSize:
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
	.size	glPointSize, .-glPointSize

	.p2align	4,,15
	.globl	glPolygonMode
	.type	glPolygonMode, @function
glPolygonMode:
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
	.size	glPolygonMode, .-glPolygonMode

	.p2align	4,,15
	.globl	glPolygonStipple
	.type	glPolygonStipple, @function
glPolygonStipple:
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
	.size	glPolygonStipple, .-glPolygonStipple

	.p2align	4,,15
	.globl	glScissor
	.type	glScissor, @function
glScissor:
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
	.size	glScissor, .-glScissor

	.p2align	4,,15
	.globl	glShadeModel
	.type	glShadeModel, @function
glShadeModel:
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
	.size	glShadeModel, .-glShadeModel

	.p2align	4,,15
	.globl	glTexParameterf
	.type	glTexParameterf, @function
glTexParameterf:
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
	.size	glTexParameterf, .-glTexParameterf

	.p2align	4,,15
	.globl	glTexParameterfv
	.type	glTexParameterfv, @function
glTexParameterfv:
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
	.size	glTexParameterfv, .-glTexParameterfv

	.p2align	4,,15
	.globl	glTexParameteri
	.type	glTexParameteri, @function
glTexParameteri:
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
	.size	glTexParameteri, .-glTexParameteri

	.p2align	4,,15
	.globl	glTexParameteriv
	.type	glTexParameteriv, @function
glTexParameteriv:
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
	.size	glTexParameteriv, .-glTexParameteriv

	.p2align	4,,15
	.globl	glTexImage1D
	.type	glTexImage1D, @function
glTexImage1D:
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
	.size	glTexImage1D, .-glTexImage1D

	.p2align	4,,15
	.globl	glTexImage2D
	.type	glTexImage2D, @function
glTexImage2D:
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
	.size	glTexImage2D, .-glTexImage2D

	.p2align	4,,15
	.globl	glTexEnvf
	.type	glTexEnvf, @function
glTexEnvf:
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
	.size	glTexEnvf, .-glTexEnvf

	.p2align	4,,15
	.globl	glTexEnvfv
	.type	glTexEnvfv, @function
glTexEnvfv:
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
	.size	glTexEnvfv, .-glTexEnvfv

	.p2align	4,,15
	.globl	glTexEnvi
	.type	glTexEnvi, @function
glTexEnvi:
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
	.size	glTexEnvi, .-glTexEnvi

	.p2align	4,,15
	.globl	glTexEnviv
	.type	glTexEnviv, @function
glTexEnviv:
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
	.size	glTexEnviv, .-glTexEnviv

	.p2align	4,,15
	.globl	glTexGend
	.type	glTexGend, @function
glTexGend:
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
	.size	glTexGend, .-glTexGend

	.p2align	4,,15
	.globl	glTexGendv
	.type	glTexGendv, @function
glTexGendv:
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
	.size	glTexGendv, .-glTexGendv

	.p2align	4,,15
	.globl	glTexGenf
	.type	glTexGenf, @function
glTexGenf:
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
	.size	glTexGenf, .-glTexGenf

	.p2align	4,,15
	.globl	glTexGenfv
	.type	glTexGenfv, @function
glTexGenfv:
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
	.size	glTexGenfv, .-glTexGenfv

	.p2align	4,,15
	.globl	glTexGeni
	.type	glTexGeni, @function
glTexGeni:
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
	.size	glTexGeni, .-glTexGeni

	.p2align	4,,15
	.globl	glTexGeniv
	.type	glTexGeniv, @function
glTexGeniv:
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
	.size	glTexGeniv, .-glTexGeniv

	.p2align	4,,15
	.globl	glFeedbackBuffer
	.type	glFeedbackBuffer, @function
glFeedbackBuffer:
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
	.size	glFeedbackBuffer, .-glFeedbackBuffer

	.p2align	4,,15
	.globl	glSelectBuffer
	.type	glSelectBuffer, @function
glSelectBuffer:
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
	.size	glSelectBuffer, .-glSelectBuffer

	.p2align	4,,15
	.globl	glRenderMode
	.type	glRenderMode, @function
glRenderMode:
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
	.size	glRenderMode, .-glRenderMode

	.p2align	4,,15
	.globl	glInitNames
	.type	glInitNames, @function
glInitNames:
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
	.size	glInitNames, .-glInitNames

	.p2align	4,,15
	.globl	glLoadName
	.type	glLoadName, @function
glLoadName:
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
	.size	glLoadName, .-glLoadName

	.p2align	4,,15
	.globl	glPassThrough
	.type	glPassThrough, @function
glPassThrough:
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
	.size	glPassThrough, .-glPassThrough

	.p2align	4,,15
	.globl	glPopName
	.type	glPopName, @function
glPopName:
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
	.size	glPopName, .-glPopName

	.p2align	4,,15
	.globl	glPushName
	.type	glPushName, @function
glPushName:
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
	.size	glPushName, .-glPushName

	.p2align	4,,15
	.globl	glDrawBuffer
	.type	glDrawBuffer, @function
glDrawBuffer:
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
	.size	glDrawBuffer, .-glDrawBuffer

	.p2align	4,,15
	.globl	glClear
	.type	glClear, @function
glClear:
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
	.size	glClear, .-glClear

	.p2align	4,,15
	.globl	glClearAccum
	.type	glClearAccum, @function
glClearAccum:
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
	.size	glClearAccum, .-glClearAccum

	.p2align	4,,15
	.globl	glClearIndex
	.type	glClearIndex, @function
glClearIndex:
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
	.size	glClearIndex, .-glClearIndex

	.p2align	4,,15
	.globl	glClearColor
	.type	glClearColor, @function
glClearColor:
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
	.size	glClearColor, .-glClearColor

	.p2align	4,,15
	.globl	glClearStencil
	.type	glClearStencil, @function
glClearStencil:
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
	.size	glClearStencil, .-glClearStencil

	.p2align	4,,15
	.globl	glClearDepth
	.type	glClearDepth, @function
glClearDepth:
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
	.size	glClearDepth, .-glClearDepth

	.p2align	4,,15
	.globl	glStencilMask
	.type	glStencilMask, @function
glStencilMask:
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
	.size	glStencilMask, .-glStencilMask

	.p2align	4,,15
	.globl	glColorMask
	.type	glColorMask, @function
glColorMask:
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
	.size	glColorMask, .-glColorMask

	.p2align	4,,15
	.globl	glDepthMask
	.type	glDepthMask, @function
glDepthMask:
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
	.size	glDepthMask, .-glDepthMask

	.p2align	4,,15
	.globl	glIndexMask
	.type	glIndexMask, @function
glIndexMask:
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
	.size	glIndexMask, .-glIndexMask

	.p2align	4,,15
	.globl	glAccum
	.type	glAccum, @function
glAccum:
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
	.size	glAccum, .-glAccum

	.p2align	4,,15
	.globl	glDisable
	.type	glDisable, @function
glDisable:
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
	.size	glDisable, .-glDisable

	.p2align	4,,15
	.globl	glEnable
	.type	glEnable, @function
glEnable:
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
	.size	glEnable, .-glEnable

	.p2align	4,,15
	.globl	glFinish
	.type	glFinish, @function
glFinish:
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
	.size	glFinish, .-glFinish

	.p2align	4,,15
	.globl	glFlush
	.type	glFlush, @function
glFlush:
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
	.size	glFlush, .-glFlush

	.p2align	4,,15
	.globl	glPopAttrib
	.type	glPopAttrib, @function
glPopAttrib:
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
	.size	glPopAttrib, .-glPopAttrib

	.p2align	4,,15
	.globl	glPushAttrib
	.type	glPushAttrib, @function
glPushAttrib:
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
	.size	glPushAttrib, .-glPushAttrib

	.p2align	4,,15
	.globl	glMap1d
	.type	glMap1d, @function
glMap1d:
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
	.size	glMap1d, .-glMap1d

	.p2align	4,,15
	.globl	glMap1f
	.type	glMap1f, @function
glMap1f:
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
	.size	glMap1f, .-glMap1f

	.p2align	4,,15
	.globl	glMap2d
	.type	glMap2d, @function
glMap2d:
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
	.size	glMap2d, .-glMap2d

	.p2align	4,,15
	.globl	glMap2f
	.type	glMap2f, @function
glMap2f:
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
	.size	glMap2f, .-glMap2f

	.p2align	4,,15
	.globl	glMapGrid1d
	.type	glMapGrid1d, @function
glMapGrid1d:
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
	.size	glMapGrid1d, .-glMapGrid1d

	.p2align	4,,15
	.globl	glMapGrid1f
	.type	glMapGrid1f, @function
glMapGrid1f:
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
	.size	glMapGrid1f, .-glMapGrid1f

	.p2align	4,,15
	.globl	glMapGrid2d
	.type	glMapGrid2d, @function
glMapGrid2d:
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
	.size	glMapGrid2d, .-glMapGrid2d

	.p2align	4,,15
	.globl	glMapGrid2f
	.type	glMapGrid2f, @function
glMapGrid2f:
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
	.size	glMapGrid2f, .-glMapGrid2f

	.p2align	4,,15
	.globl	glEvalCoord1d
	.type	glEvalCoord1d, @function
glEvalCoord1d:
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
	.size	glEvalCoord1d, .-glEvalCoord1d

	.p2align	4,,15
	.globl	glEvalCoord1dv
	.type	glEvalCoord1dv, @function
glEvalCoord1dv:
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
	.size	glEvalCoord1dv, .-glEvalCoord1dv

	.p2align	4,,15
	.globl	glEvalCoord1f
	.type	glEvalCoord1f, @function
glEvalCoord1f:
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
	.size	glEvalCoord1f, .-glEvalCoord1f

	.p2align	4,,15
	.globl	glEvalCoord1fv
	.type	glEvalCoord1fv, @function
glEvalCoord1fv:
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
	.size	glEvalCoord1fv, .-glEvalCoord1fv

	.p2align	4,,15
	.globl	glEvalCoord2d
	.type	glEvalCoord2d, @function
glEvalCoord2d:
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
	.size	glEvalCoord2d, .-glEvalCoord2d

	.p2align	4,,15
	.globl	glEvalCoord2dv
	.type	glEvalCoord2dv, @function
glEvalCoord2dv:
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
	.size	glEvalCoord2dv, .-glEvalCoord2dv

	.p2align	4,,15
	.globl	glEvalCoord2f
	.type	glEvalCoord2f, @function
glEvalCoord2f:
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
	.size	glEvalCoord2f, .-glEvalCoord2f

	.p2align	4,,15
	.globl	glEvalCoord2fv
	.type	glEvalCoord2fv, @function
glEvalCoord2fv:
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
	.size	glEvalCoord2fv, .-glEvalCoord2fv

	.p2align	4,,15
	.globl	glEvalMesh1
	.type	glEvalMesh1, @function
glEvalMesh1:
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
	.size	glEvalMesh1, .-glEvalMesh1

	.p2align	4,,15
	.globl	glEvalPoint1
	.type	glEvalPoint1, @function
glEvalPoint1:
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
	.size	glEvalPoint1, .-glEvalPoint1

	.p2align	4,,15
	.globl	glEvalMesh2
	.type	glEvalMesh2, @function
glEvalMesh2:
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
	.size	glEvalMesh2, .-glEvalMesh2

	.p2align	4,,15
	.globl	glEvalPoint2
	.type	glEvalPoint2, @function
glEvalPoint2:
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
	.size	glEvalPoint2, .-glEvalPoint2

	.p2align	4,,15
	.globl	glAlphaFunc
	.type	glAlphaFunc, @function
glAlphaFunc:
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
	.size	glAlphaFunc, .-glAlphaFunc

	.p2align	4,,15
	.globl	glBlendFunc
	.type	glBlendFunc, @function
glBlendFunc:
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
	.size	glBlendFunc, .-glBlendFunc

	.p2align	4,,15
	.globl	glLogicOp
	.type	glLogicOp, @function
glLogicOp:
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
	.size	glLogicOp, .-glLogicOp

	.p2align	4,,15
	.globl	glStencilFunc
	.type	glStencilFunc, @function
glStencilFunc:
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
	.size	glStencilFunc, .-glStencilFunc

	.p2align	4,,15
	.globl	glStencilOp
	.type	glStencilOp, @function
glStencilOp:
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
	.size	glStencilOp, .-glStencilOp

	.p2align	4,,15
	.globl	glDepthFunc
	.type	glDepthFunc, @function
glDepthFunc:
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
	.size	glDepthFunc, .-glDepthFunc

	.p2align	4,,15
	.globl	glPixelZoom
	.type	glPixelZoom, @function
glPixelZoom:
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
	.size	glPixelZoom, .-glPixelZoom

	.p2align	4,,15
	.globl	glPixelTransferf
	.type	glPixelTransferf, @function
glPixelTransferf:
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
	.size	glPixelTransferf, .-glPixelTransferf

	.p2align	4,,15
	.globl	glPixelTransferi
	.type	glPixelTransferi, @function
glPixelTransferi:
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
	.size	glPixelTransferi, .-glPixelTransferi

	.p2align	4,,15
	.globl	glPixelStoref
	.type	glPixelStoref, @function
glPixelStoref:
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
	.size	glPixelStoref, .-glPixelStoref

	.p2align	4,,15
	.globl	glPixelStorei
	.type	glPixelStorei, @function
glPixelStorei:
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
	.size	glPixelStorei, .-glPixelStorei

	.p2align	4,,15
	.globl	glPixelMapfv
	.type	glPixelMapfv, @function
glPixelMapfv:
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
	.size	glPixelMapfv, .-glPixelMapfv

	.p2align	4,,15
	.globl	glPixelMapuiv
	.type	glPixelMapuiv, @function
glPixelMapuiv:
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
	.size	glPixelMapuiv, .-glPixelMapuiv

	.p2align	4,,15
	.globl	glPixelMapusv
	.type	glPixelMapusv, @function
glPixelMapusv:
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
	.size	glPixelMapusv, .-glPixelMapusv

	.p2align	4,,15
	.globl	glReadBuffer
	.type	glReadBuffer, @function
glReadBuffer:
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
	.size	glReadBuffer, .-glReadBuffer

	.p2align	4,,15
	.globl	glCopyPixels
	.type	glCopyPixels, @function
glCopyPixels:
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
	.size	glCopyPixels, .-glCopyPixels

	.p2align	4,,15
	.globl	glReadPixels
	.type	glReadPixels, @function
glReadPixels:
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
	.size	glReadPixels, .-glReadPixels

	.p2align	4,,15
	.globl	glDrawPixels
	.type	glDrawPixels, @function
glDrawPixels:
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
	.size	glDrawPixels, .-glDrawPixels

	.p2align	4,,15
	.globl	glGetBooleanv
	.type	glGetBooleanv, @function
glGetBooleanv:
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
	.size	glGetBooleanv, .-glGetBooleanv

	.p2align	4,,15
	.globl	glGetClipPlane
	.type	glGetClipPlane, @function
glGetClipPlane:
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
	.size	glGetClipPlane, .-glGetClipPlane

	.p2align	4,,15
	.globl	glGetDoublev
	.type	glGetDoublev, @function
glGetDoublev:
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
	.size	glGetDoublev, .-glGetDoublev

	.p2align	4,,15
	.globl	glGetError
	.type	glGetError, @function
glGetError:
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
	.size	glGetError, .-glGetError

	.p2align	4,,15
	.globl	glGetFloatv
	.type	glGetFloatv, @function
glGetFloatv:
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
	.size	glGetFloatv, .-glGetFloatv

	.p2align	4,,15
	.globl	glGetIntegerv
	.type	glGetIntegerv, @function
glGetIntegerv:
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
	.size	glGetIntegerv, .-glGetIntegerv

	.p2align	4,,15
	.globl	glGetLightfv
	.type	glGetLightfv, @function
glGetLightfv:
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
	.size	glGetLightfv, .-glGetLightfv

	.p2align	4,,15
	.globl	glGetLightiv
	.type	glGetLightiv, @function
glGetLightiv:
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
	.size	glGetLightiv, .-glGetLightiv

	.p2align	4,,15
	.globl	glGetMapdv
	.type	glGetMapdv, @function
glGetMapdv:
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
	.size	glGetMapdv, .-glGetMapdv

	.p2align	4,,15
	.globl	glGetMapfv
	.type	glGetMapfv, @function
glGetMapfv:
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
	.size	glGetMapfv, .-glGetMapfv

	.p2align	4,,15
	.globl	glGetMapiv
	.type	glGetMapiv, @function
glGetMapiv:
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
	.size	glGetMapiv, .-glGetMapiv

	.p2align	4,,15
	.globl	glGetMaterialfv
	.type	glGetMaterialfv, @function
glGetMaterialfv:
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
	.size	glGetMaterialfv, .-glGetMaterialfv

	.p2align	4,,15
	.globl	glGetMaterialiv
	.type	glGetMaterialiv, @function
glGetMaterialiv:
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
	.size	glGetMaterialiv, .-glGetMaterialiv

	.p2align	4,,15
	.globl	glGetPixelMapfv
	.type	glGetPixelMapfv, @function
glGetPixelMapfv:
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
	.size	glGetPixelMapfv, .-glGetPixelMapfv

	.p2align	4,,15
	.globl	glGetPixelMapuiv
	.type	glGetPixelMapuiv, @function
glGetPixelMapuiv:
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
	.size	glGetPixelMapuiv, .-glGetPixelMapuiv

	.p2align	4,,15
	.globl	glGetPixelMapusv
	.type	glGetPixelMapusv, @function
glGetPixelMapusv:
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
	.size	glGetPixelMapusv, .-glGetPixelMapusv

	.p2align	4,,15
	.globl	glGetPolygonStipple
	.type	glGetPolygonStipple, @function
glGetPolygonStipple:
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
	.size	glGetPolygonStipple, .-glGetPolygonStipple

	.p2align	4,,15
	.globl	glGetString
	.type	glGetString, @function
glGetString:
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
	.size	glGetString, .-glGetString

	.p2align	4,,15
	.globl	glGetTexEnvfv
	.type	glGetTexEnvfv, @function
glGetTexEnvfv:
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
	.size	glGetTexEnvfv, .-glGetTexEnvfv

	.p2align	4,,15
	.globl	glGetTexEnviv
	.type	glGetTexEnviv, @function
glGetTexEnviv:
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
	.size	glGetTexEnviv, .-glGetTexEnviv

	.p2align	4,,15
	.globl	glGetTexGendv
	.type	glGetTexGendv, @function
glGetTexGendv:
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
	.size	glGetTexGendv, .-glGetTexGendv

	.p2align	4,,15
	.globl	glGetTexGenfv
	.type	glGetTexGenfv, @function
glGetTexGenfv:
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
	.size	glGetTexGenfv, .-glGetTexGenfv

	.p2align	4,,15
	.globl	glGetTexGeniv
	.type	glGetTexGeniv, @function
glGetTexGeniv:
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
	.size	glGetTexGeniv, .-glGetTexGeniv

	.p2align	4,,15
	.globl	glGetTexImage
	.type	glGetTexImage, @function
glGetTexImage:
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
	.size	glGetTexImage, .-glGetTexImage

	.p2align	4,,15
	.globl	glGetTexParameterfv
	.type	glGetTexParameterfv, @function
glGetTexParameterfv:
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
	.size	glGetTexParameterfv, .-glGetTexParameterfv

	.p2align	4,,15
	.globl	glGetTexParameteriv
	.type	glGetTexParameteriv, @function
glGetTexParameteriv:
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
	.size	glGetTexParameteriv, .-glGetTexParameteriv

	.p2align	4,,15
	.globl	glGetTexLevelParameterfv
	.type	glGetTexLevelParameterfv, @function
glGetTexLevelParameterfv:
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
	.size	glGetTexLevelParameterfv, .-glGetTexLevelParameterfv

	.p2align	4,,15
	.globl	glGetTexLevelParameteriv
	.type	glGetTexLevelParameteriv, @function
glGetTexLevelParameteriv:
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
	.size	glGetTexLevelParameteriv, .-glGetTexLevelParameteriv

	.p2align	4,,15
	.globl	glIsEnabled
	.type	glIsEnabled, @function
glIsEnabled:
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
	.size	glIsEnabled, .-glIsEnabled

	.p2align	4,,15
	.globl	glIsList
	.type	glIsList, @function
glIsList:
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
	.size	glIsList, .-glIsList

	.p2align	4,,15
	.globl	glDepthRange
	.type	glDepthRange, @function
glDepthRange:
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
	.size	glDepthRange, .-glDepthRange

	.p2align	4,,15
	.globl	glFrustum
	.type	glFrustum, @function
glFrustum:
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
	.size	glFrustum, .-glFrustum

	.p2align	4,,15
	.globl	glLoadIdentity
	.type	glLoadIdentity, @function
glLoadIdentity:
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
	.size	glLoadIdentity, .-glLoadIdentity

	.p2align	4,,15
	.globl	glLoadMatrixf
	.type	glLoadMatrixf, @function
glLoadMatrixf:
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
	.size	glLoadMatrixf, .-glLoadMatrixf

	.p2align	4,,15
	.globl	glLoadMatrixd
	.type	glLoadMatrixd, @function
glLoadMatrixd:
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
	.size	glLoadMatrixd, .-glLoadMatrixd

	.p2align	4,,15
	.globl	glMatrixMode
	.type	glMatrixMode, @function
glMatrixMode:
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
	.size	glMatrixMode, .-glMatrixMode

	.p2align	4,,15
	.globl	glMultMatrixf
	.type	glMultMatrixf, @function
glMultMatrixf:
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
	.size	glMultMatrixf, .-glMultMatrixf

	.p2align	4,,15
	.globl	glMultMatrixd
	.type	glMultMatrixd, @function
glMultMatrixd:
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
	.size	glMultMatrixd, .-glMultMatrixd

	.p2align	4,,15
	.globl	glOrtho
	.type	glOrtho, @function
glOrtho:
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
	.size	glOrtho, .-glOrtho

	.p2align	4,,15
	.globl	glPopMatrix
	.type	glPopMatrix, @function
glPopMatrix:
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
	.size	glPopMatrix, .-glPopMatrix

	.p2align	4,,15
	.globl	glPushMatrix
	.type	glPushMatrix, @function
glPushMatrix:
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
	.size	glPushMatrix, .-glPushMatrix

	.p2align	4,,15
	.globl	glRotated
	.type	glRotated, @function
glRotated:
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
	.size	glRotated, .-glRotated

	.p2align	4,,15
	.globl	glRotatef
	.type	glRotatef, @function
glRotatef:
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
	.size	glRotatef, .-glRotatef

	.p2align	4,,15
	.globl	glScaled
	.type	glScaled, @function
glScaled:
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
	.size	glScaled, .-glScaled

	.p2align	4,,15
	.globl	glScalef
	.type	glScalef, @function
glScalef:
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
	.size	glScalef, .-glScalef

	.p2align	4,,15
	.globl	glTranslated
	.type	glTranslated, @function
glTranslated:
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
	.size	glTranslated, .-glTranslated

	.p2align	4,,15
	.globl	glTranslatef
	.type	glTranslatef, @function
glTranslatef:
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
	.size	glTranslatef, .-glTranslatef

	.p2align	4,,15
	.globl	glViewport
	.type	glViewport, @function
glViewport:
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
	.size	glViewport, .-glViewport

	.p2align	4,,15
	.globl	glArrayElement
	.type	glArrayElement, @function
glArrayElement:
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
	.size	glArrayElement, .-glArrayElement

	.p2align	4,,15
	.globl	glBindTexture
	.type	glBindTexture, @function
glBindTexture:
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
	.size	glBindTexture, .-glBindTexture

	.p2align	4,,15
	.globl	glColorPointer
	.type	glColorPointer, @function
glColorPointer:
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
	.size	glColorPointer, .-glColorPointer

	.p2align	4,,15
	.globl	glDisableClientState
	.type	glDisableClientState, @function
glDisableClientState:
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
	.size	glDisableClientState, .-glDisableClientState

	.p2align	4,,15
	.globl	glDrawArrays
	.type	glDrawArrays, @function
glDrawArrays:
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
	.size	glDrawArrays, .-glDrawArrays

	.p2align	4,,15
	.globl	glDrawElements
	.type	glDrawElements, @function
glDrawElements:
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
	.size	glDrawElements, .-glDrawElements

	.p2align	4,,15
	.globl	glEdgeFlagPointer
	.type	glEdgeFlagPointer, @function
glEdgeFlagPointer:
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
	.size	glEdgeFlagPointer, .-glEdgeFlagPointer

	.p2align	4,,15
	.globl	glEnableClientState
	.type	glEnableClientState, @function
glEnableClientState:
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
	.size	glEnableClientState, .-glEnableClientState

	.p2align	4,,15
	.globl	glIndexPointer
	.type	glIndexPointer, @function
glIndexPointer:
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
	.size	glIndexPointer, .-glIndexPointer

	.p2align	4,,15
	.globl	glIndexub
	.type	glIndexub, @function
glIndexub:
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
	.size	glIndexub, .-glIndexub

	.p2align	4,,15
	.globl	glIndexubv
	.type	glIndexubv, @function
glIndexubv:
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
	.size	glIndexubv, .-glIndexubv

	.p2align	4,,15
	.globl	glInterleavedArrays
	.type	glInterleavedArrays, @function
glInterleavedArrays:
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
	.size	glInterleavedArrays, .-glInterleavedArrays

	.p2align	4,,15
	.globl	glNormalPointer
	.type	glNormalPointer, @function
glNormalPointer:
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
	.size	glNormalPointer, .-glNormalPointer

	.p2align	4,,15
	.globl	glPolygonOffset
	.type	glPolygonOffset, @function
glPolygonOffset:
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
	.size	glPolygonOffset, .-glPolygonOffset

	.p2align	4,,15
	.globl	glTexCoordPointer
	.type	glTexCoordPointer, @function
glTexCoordPointer:
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
	.size	glTexCoordPointer, .-glTexCoordPointer

	.p2align	4,,15
	.globl	glVertexPointer
	.type	glVertexPointer, @function
glVertexPointer:
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
	.size	glVertexPointer, .-glVertexPointer

	.p2align	4,,15
	.globl	glAreTexturesResident
	.type	glAreTexturesResident, @function
glAreTexturesResident:
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
	.size	glAreTexturesResident, .-glAreTexturesResident

	.p2align	4,,15
	.globl	glCopyTexImage1D
	.type	glCopyTexImage1D, @function
glCopyTexImage1D:
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
	.size	glCopyTexImage1D, .-glCopyTexImage1D

	.p2align	4,,15
	.globl	glCopyTexImage2D
	.type	glCopyTexImage2D, @function
glCopyTexImage2D:
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
	.size	glCopyTexImage2D, .-glCopyTexImage2D

	.p2align	4,,15
	.globl	glCopyTexSubImage1D
	.type	glCopyTexSubImage1D, @function
glCopyTexSubImage1D:
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
	.size	glCopyTexSubImage1D, .-glCopyTexSubImage1D

	.p2align	4,,15
	.globl	glCopyTexSubImage2D
	.type	glCopyTexSubImage2D, @function
glCopyTexSubImage2D:
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
	.size	glCopyTexSubImage2D, .-glCopyTexSubImage2D

	.p2align	4,,15
	.globl	glDeleteTextures
	.type	glDeleteTextures, @function
glDeleteTextures:
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
	.size	glDeleteTextures, .-glDeleteTextures

	.p2align	4,,15
	.globl	glGenTextures
	.type	glGenTextures, @function
glGenTextures:
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
	.size	glGenTextures, .-glGenTextures

	.p2align	4,,15
	.globl	glGetPointerv
	.type	glGetPointerv, @function
glGetPointerv:
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
	.size	glGetPointerv, .-glGetPointerv

	.p2align	4,,15
	.globl	glIsTexture
	.type	glIsTexture, @function
glIsTexture:
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
	.size	glIsTexture, .-glIsTexture

	.p2align	4,,15
	.globl	glPrioritizeTextures
	.type	glPrioritizeTextures, @function
glPrioritizeTextures:
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
	.size	glPrioritizeTextures, .-glPrioritizeTextures

	.p2align	4,,15
	.globl	glTexSubImage1D
	.type	glTexSubImage1D, @function
glTexSubImage1D:
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
	.size	glTexSubImage1D, .-glTexSubImage1D

	.p2align	4,,15
	.globl	glTexSubImage2D
	.type	glTexSubImage2D, @function
glTexSubImage2D:
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
	.size	glTexSubImage2D, .-glTexSubImage2D

	.p2align	4,,15
	.globl	glPopClientAttrib
	.type	glPopClientAttrib, @function
glPopClientAttrib:
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
	.size	glPopClientAttrib, .-glPopClientAttrib

	.p2align	4,,15
	.globl	glPushClientAttrib
	.type	glPushClientAttrib, @function
glPushClientAttrib:
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
	.size	glPushClientAttrib, .-glPushClientAttrib

	.p2align	4,,15
	.globl	glBlendColor
	.type	glBlendColor, @function
glBlendColor:
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
	.size	glBlendColor, .-glBlendColor

	.p2align	4,,15
	.globl	glBlendEquation
	.type	glBlendEquation, @function
glBlendEquation:
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
	.size	glBlendEquation, .-glBlendEquation

	.p2align	4,,15
	.globl	glDrawRangeElements
	.type	glDrawRangeElements, @function
glDrawRangeElements:
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
	.size	glDrawRangeElements, .-glDrawRangeElements

	.p2align	4,,15
	.globl	glColorTable
	.type	glColorTable, @function
glColorTable:
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
	.size	glColorTable, .-glColorTable

	.p2align	4,,15
	.globl	glColorTableParameterfv
	.type	glColorTableParameterfv, @function
glColorTableParameterfv:
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
	.size	glColorTableParameterfv, .-glColorTableParameterfv

	.p2align	4,,15
	.globl	glColorTableParameteriv
	.type	glColorTableParameteriv, @function
glColorTableParameteriv:
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
	.size	glColorTableParameteriv, .-glColorTableParameteriv

	.p2align	4,,15
	.globl	glCopyColorTable
	.type	glCopyColorTable, @function
glCopyColorTable:
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
	.size	glCopyColorTable, .-glCopyColorTable

	.p2align	4,,15
	.globl	glGetColorTable
	.type	glGetColorTable, @function
glGetColorTable:
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
	.size	glGetColorTable, .-glGetColorTable

	.p2align	4,,15
	.globl	glGetColorTableParameterfv
	.type	glGetColorTableParameterfv, @function
glGetColorTableParameterfv:
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
	.size	glGetColorTableParameterfv, .-glGetColorTableParameterfv

	.p2align	4,,15
	.globl	glGetColorTableParameteriv
	.type	glGetColorTableParameteriv, @function
glGetColorTableParameteriv:
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
	.size	glGetColorTableParameteriv, .-glGetColorTableParameteriv

	.p2align	4,,15
	.globl	glColorSubTable
	.type	glColorSubTable, @function
glColorSubTable:
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
	.size	glColorSubTable, .-glColorSubTable

	.p2align	4,,15
	.globl	glCopyColorSubTable
	.type	glCopyColorSubTable, @function
glCopyColorSubTable:
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
	.size	glCopyColorSubTable, .-glCopyColorSubTable

	.p2align	4,,15
	.globl	glConvolutionFilter1D
	.type	glConvolutionFilter1D, @function
glConvolutionFilter1D:
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
	.size	glConvolutionFilter1D, .-glConvolutionFilter1D

	.p2align	4,,15
	.globl	glConvolutionFilter2D
	.type	glConvolutionFilter2D, @function
glConvolutionFilter2D:
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
	.size	glConvolutionFilter2D, .-glConvolutionFilter2D

	.p2align	4,,15
	.globl	glConvolutionParameterf
	.type	glConvolutionParameterf, @function
glConvolutionParameterf:
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
	.size	glConvolutionParameterf, .-glConvolutionParameterf

	.p2align	4,,15
	.globl	glConvolutionParameterfv
	.type	glConvolutionParameterfv, @function
glConvolutionParameterfv:
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
	.size	glConvolutionParameterfv, .-glConvolutionParameterfv

	.p2align	4,,15
	.globl	glConvolutionParameteri
	.type	glConvolutionParameteri, @function
glConvolutionParameteri:
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
	.size	glConvolutionParameteri, .-glConvolutionParameteri

	.p2align	4,,15
	.globl	glConvolutionParameteriv
	.type	glConvolutionParameteriv, @function
glConvolutionParameteriv:
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
	.size	glConvolutionParameteriv, .-glConvolutionParameteriv

	.p2align	4,,15
	.globl	glCopyConvolutionFilter1D
	.type	glCopyConvolutionFilter1D, @function
glCopyConvolutionFilter1D:
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
	.size	glCopyConvolutionFilter1D, .-glCopyConvolutionFilter1D

	.p2align	4,,15
	.globl	glCopyConvolutionFilter2D
	.type	glCopyConvolutionFilter2D, @function
glCopyConvolutionFilter2D:
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
	.size	glCopyConvolutionFilter2D, .-glCopyConvolutionFilter2D

	.p2align	4,,15
	.globl	glGetConvolutionFilter
	.type	glGetConvolutionFilter, @function
glGetConvolutionFilter:
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
	.size	glGetConvolutionFilter, .-glGetConvolutionFilter

	.p2align	4,,15
	.globl	glGetConvolutionParameterfv
	.type	glGetConvolutionParameterfv, @function
glGetConvolutionParameterfv:
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
	.size	glGetConvolutionParameterfv, .-glGetConvolutionParameterfv

	.p2align	4,,15
	.globl	glGetConvolutionParameteriv
	.type	glGetConvolutionParameteriv, @function
glGetConvolutionParameteriv:
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
	.size	glGetConvolutionParameteriv, .-glGetConvolutionParameteriv

	.p2align	4,,15
	.globl	glGetSeparableFilter
	.type	glGetSeparableFilter, @function
glGetSeparableFilter:
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
	.size	glGetSeparableFilter, .-glGetSeparableFilter

	.p2align	4,,15
	.globl	glSeparableFilter2D
	.type	glSeparableFilter2D, @function
glSeparableFilter2D:
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
	.size	glSeparableFilter2D, .-glSeparableFilter2D

	.p2align	4,,15
	.globl	glGetHistogram
	.type	glGetHistogram, @function
glGetHistogram:
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
	.size	glGetHistogram, .-glGetHistogram

	.p2align	4,,15
	.globl	glGetHistogramParameterfv
	.type	glGetHistogramParameterfv, @function
glGetHistogramParameterfv:
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
	.size	glGetHistogramParameterfv, .-glGetHistogramParameterfv

	.p2align	4,,15
	.globl	glGetHistogramParameteriv
	.type	glGetHistogramParameteriv, @function
glGetHistogramParameteriv:
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
	.size	glGetHistogramParameteriv, .-glGetHistogramParameteriv

	.p2align	4,,15
	.globl	glGetMinmax
	.type	glGetMinmax, @function
glGetMinmax:
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
	.size	glGetMinmax, .-glGetMinmax

	.p2align	4,,15
	.globl	glGetMinmaxParameterfv
	.type	glGetMinmaxParameterfv, @function
glGetMinmaxParameterfv:
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
	.size	glGetMinmaxParameterfv, .-glGetMinmaxParameterfv

	.p2align	4,,15
	.globl	glGetMinmaxParameteriv
	.type	glGetMinmaxParameteriv, @function
glGetMinmaxParameteriv:
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
	.size	glGetMinmaxParameteriv, .-glGetMinmaxParameteriv

	.p2align	4,,15
	.globl	glHistogram
	.type	glHistogram, @function
glHistogram:
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
	.size	glHistogram, .-glHistogram

	.p2align	4,,15
	.globl	glMinmax
	.type	glMinmax, @function
glMinmax:
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
	.size	glMinmax, .-glMinmax

	.p2align	4,,15
	.globl	glResetHistogram
	.type	glResetHistogram, @function
glResetHistogram:
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
	.size	glResetHistogram, .-glResetHistogram

	.p2align	4,,15
	.globl	glResetMinmax
	.type	glResetMinmax, @function
glResetMinmax:
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
	.size	glResetMinmax, .-glResetMinmax

	.p2align	4,,15
	.globl	glTexImage3D
	.type	glTexImage3D, @function
glTexImage3D:
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
	.size	glTexImage3D, .-glTexImage3D

	.p2align	4,,15
	.globl	glTexSubImage3D
	.type	glTexSubImage3D, @function
glTexSubImage3D:
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
	.size	glTexSubImage3D, .-glTexSubImage3D

	.p2align	4,,15
	.globl	glCopyTexSubImage3D
	.type	glCopyTexSubImage3D, @function
glCopyTexSubImage3D:
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
	.size	glCopyTexSubImage3D, .-glCopyTexSubImage3D

	.p2align	4,,15
	.globl	glActiveTextureARB
	.type	glActiveTextureARB, @function
glActiveTextureARB:
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
	.size	glActiveTextureARB, .-glActiveTextureARB

	.p2align	4,,15
	.globl	glClientActiveTextureARB
	.type	glClientActiveTextureARB, @function
glClientActiveTextureARB:
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
	.size	glClientActiveTextureARB, .-glClientActiveTextureARB

	.p2align	4,,15
	.globl	glMultiTexCoord1dARB
	.type	glMultiTexCoord1dARB, @function
glMultiTexCoord1dARB:
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
	.size	glMultiTexCoord1dARB, .-glMultiTexCoord1dARB

	.p2align	4,,15
	.globl	glMultiTexCoord1dvARB
	.type	glMultiTexCoord1dvARB, @function
glMultiTexCoord1dvARB:
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
	.size	glMultiTexCoord1dvARB, .-glMultiTexCoord1dvARB

	.p2align	4,,15
	.globl	glMultiTexCoord1fARB
	.type	glMultiTexCoord1fARB, @function
glMultiTexCoord1fARB:
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
	.size	glMultiTexCoord1fARB, .-glMultiTexCoord1fARB

	.p2align	4,,15
	.globl	glMultiTexCoord1fvARB
	.type	glMultiTexCoord1fvARB, @function
glMultiTexCoord1fvARB:
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
	.size	glMultiTexCoord1fvARB, .-glMultiTexCoord1fvARB

	.p2align	4,,15
	.globl	glMultiTexCoord1iARB
	.type	glMultiTexCoord1iARB, @function
glMultiTexCoord1iARB:
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
	.size	glMultiTexCoord1iARB, .-glMultiTexCoord1iARB

	.p2align	4,,15
	.globl	glMultiTexCoord1ivARB
	.type	glMultiTexCoord1ivARB, @function
glMultiTexCoord1ivARB:
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
	.size	glMultiTexCoord1ivARB, .-glMultiTexCoord1ivARB

	.p2align	4,,15
	.globl	glMultiTexCoord1sARB
	.type	glMultiTexCoord1sARB, @function
glMultiTexCoord1sARB:
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
	.size	glMultiTexCoord1sARB, .-glMultiTexCoord1sARB

	.p2align	4,,15
	.globl	glMultiTexCoord1svARB
	.type	glMultiTexCoord1svARB, @function
glMultiTexCoord1svARB:
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
	.size	glMultiTexCoord1svARB, .-glMultiTexCoord1svARB

	.p2align	4,,15
	.globl	glMultiTexCoord2dARB
	.type	glMultiTexCoord2dARB, @function
glMultiTexCoord2dARB:
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
	.size	glMultiTexCoord2dARB, .-glMultiTexCoord2dARB

	.p2align	4,,15
	.globl	glMultiTexCoord2dvARB
	.type	glMultiTexCoord2dvARB, @function
glMultiTexCoord2dvARB:
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
	.size	glMultiTexCoord2dvARB, .-glMultiTexCoord2dvARB

	.p2align	4,,15
	.globl	glMultiTexCoord2fARB
	.type	glMultiTexCoord2fARB, @function
glMultiTexCoord2fARB:
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
	.size	glMultiTexCoord2fARB, .-glMultiTexCoord2fARB

	.p2align	4,,15
	.globl	glMultiTexCoord2fvARB
	.type	glMultiTexCoord2fvARB, @function
glMultiTexCoord2fvARB:
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
	.size	glMultiTexCoord2fvARB, .-glMultiTexCoord2fvARB

	.p2align	4,,15
	.globl	glMultiTexCoord2iARB
	.type	glMultiTexCoord2iARB, @function
glMultiTexCoord2iARB:
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
	.size	glMultiTexCoord2iARB, .-glMultiTexCoord2iARB

	.p2align	4,,15
	.globl	glMultiTexCoord2ivARB
	.type	glMultiTexCoord2ivARB, @function
glMultiTexCoord2ivARB:
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
	.size	glMultiTexCoord2ivARB, .-glMultiTexCoord2ivARB

	.p2align	4,,15
	.globl	glMultiTexCoord2sARB
	.type	glMultiTexCoord2sARB, @function
glMultiTexCoord2sARB:
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
	.size	glMultiTexCoord2sARB, .-glMultiTexCoord2sARB

	.p2align	4,,15
	.globl	glMultiTexCoord2svARB
	.type	glMultiTexCoord2svARB, @function
glMultiTexCoord2svARB:
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
	.size	glMultiTexCoord2svARB, .-glMultiTexCoord2svARB

	.p2align	4,,15
	.globl	glMultiTexCoord3dARB
	.type	glMultiTexCoord3dARB, @function
glMultiTexCoord3dARB:
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
	.size	glMultiTexCoord3dARB, .-glMultiTexCoord3dARB

	.p2align	4,,15
	.globl	glMultiTexCoord3dvARB
	.type	glMultiTexCoord3dvARB, @function
glMultiTexCoord3dvARB:
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
	.size	glMultiTexCoord3dvARB, .-glMultiTexCoord3dvARB

	.p2align	4,,15
	.globl	glMultiTexCoord3fARB
	.type	glMultiTexCoord3fARB, @function
glMultiTexCoord3fARB:
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
	.size	glMultiTexCoord3fARB, .-glMultiTexCoord3fARB

	.p2align	4,,15
	.globl	glMultiTexCoord3fvARB
	.type	glMultiTexCoord3fvARB, @function
glMultiTexCoord3fvARB:
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
	.size	glMultiTexCoord3fvARB, .-glMultiTexCoord3fvARB

	.p2align	4,,15
	.globl	glMultiTexCoord3iARB
	.type	glMultiTexCoord3iARB, @function
glMultiTexCoord3iARB:
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
	.size	glMultiTexCoord3iARB, .-glMultiTexCoord3iARB

	.p2align	4,,15
	.globl	glMultiTexCoord3ivARB
	.type	glMultiTexCoord3ivARB, @function
glMultiTexCoord3ivARB:
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
	.size	glMultiTexCoord3ivARB, .-glMultiTexCoord3ivARB

	.p2align	4,,15
	.globl	glMultiTexCoord3sARB
	.type	glMultiTexCoord3sARB, @function
glMultiTexCoord3sARB:
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
	.size	glMultiTexCoord3sARB, .-glMultiTexCoord3sARB

	.p2align	4,,15
	.globl	glMultiTexCoord3svARB
	.type	glMultiTexCoord3svARB, @function
glMultiTexCoord3svARB:
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
	.size	glMultiTexCoord3svARB, .-glMultiTexCoord3svARB

	.p2align	4,,15
	.globl	glMultiTexCoord4dARB
	.type	glMultiTexCoord4dARB, @function
glMultiTexCoord4dARB:
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
	.size	glMultiTexCoord4dARB, .-glMultiTexCoord4dARB

	.p2align	4,,15
	.globl	glMultiTexCoord4dvARB
	.type	glMultiTexCoord4dvARB, @function
glMultiTexCoord4dvARB:
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
	.size	glMultiTexCoord4dvARB, .-glMultiTexCoord4dvARB

	.p2align	4,,15
	.globl	glMultiTexCoord4fARB
	.type	glMultiTexCoord4fARB, @function
glMultiTexCoord4fARB:
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
	.size	glMultiTexCoord4fARB, .-glMultiTexCoord4fARB

	.p2align	4,,15
	.globl	glMultiTexCoord4fvARB
	.type	glMultiTexCoord4fvARB, @function
glMultiTexCoord4fvARB:
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
	.size	glMultiTexCoord4fvARB, .-glMultiTexCoord4fvARB

	.p2align	4,,15
	.globl	glMultiTexCoord4iARB
	.type	glMultiTexCoord4iARB, @function
glMultiTexCoord4iARB:
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
	.size	glMultiTexCoord4iARB, .-glMultiTexCoord4iARB

	.p2align	4,,15
	.globl	glMultiTexCoord4ivARB
	.type	glMultiTexCoord4ivARB, @function
glMultiTexCoord4ivARB:
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
	.size	glMultiTexCoord4ivARB, .-glMultiTexCoord4ivARB

	.p2align	4,,15
	.globl	glMultiTexCoord4sARB
	.type	glMultiTexCoord4sARB, @function
glMultiTexCoord4sARB:
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
	.size	glMultiTexCoord4sARB, .-glMultiTexCoord4sARB

	.p2align	4,,15
	.globl	glMultiTexCoord4svARB
	.type	glMultiTexCoord4svARB, @function
glMultiTexCoord4svARB:
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
	.size	glMultiTexCoord4svARB, .-glMultiTexCoord4svARB

	.p2align	4,,15
	.globl	glLoadTransposeMatrixfARB
	.type	glLoadTransposeMatrixfARB, @function
glLoadTransposeMatrixfARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3264(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3264(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glLoadTransposeMatrixfARB, .-glLoadTransposeMatrixfARB

	.p2align	4,,15
	.globl	glLoadTransposeMatrixdARB
	.type	glLoadTransposeMatrixdARB, @function
glLoadTransposeMatrixdARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3272(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3272(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3272(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3272(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glLoadTransposeMatrixdARB, .-glLoadTransposeMatrixdARB

	.p2align	4,,15
	.globl	glMultTransposeMatrixfARB
	.type	glMultTransposeMatrixfARB, @function
glMultTransposeMatrixfARB:
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
	.size	glMultTransposeMatrixfARB, .-glMultTransposeMatrixfARB

	.p2align	4,,15
	.globl	glMultTransposeMatrixdARB
	.type	glMultTransposeMatrixdARB, @function
glMultTransposeMatrixdARB:
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
	.size	glMultTransposeMatrixdARB, .-glMultTransposeMatrixdARB

	.p2align	4,,15
	.globl	glSampleCoverageARB
	.type	glSampleCoverageARB, @function
glSampleCoverageARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3296(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
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
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3296(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSampleCoverageARB, .-glSampleCoverageARB

	.p2align	4,,15
	.globl	glDrawBuffersARB
	.type	glDrawBuffersARB, @function
glDrawBuffersARB:
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
	.size	glDrawBuffersARB, .-glDrawBuffersARB

	.p2align	4,,15
	.globl	glPolygonOffsetEXT
	.type	glPolygonOffsetEXT, @function
glPolygonOffsetEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3312(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	3312(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3312(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	3312(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glPolygonOffsetEXT, .-glPolygonOffsetEXT

	.p2align	4,,15
	.globl	glGetTexFilterFuncSGIS
	.type	glGetTexFilterFuncSGIS, @function
glGetTexFilterFuncSGIS:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3320(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3320(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetTexFilterFuncSGIS, .-glGetTexFilterFuncSGIS

	.p2align	4,,15
	.globl	glTexFilterFuncSGIS
	.type	glTexFilterFuncSGIS, @function
glTexFilterFuncSGIS:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3328(%rax), %r11
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
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3328(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glTexFilterFuncSGIS, .-glTexFilterFuncSGIS

	.p2align	4,,15
	.globl	glGetHistogramEXT
	.type	glGetHistogramEXT, @function
glGetHistogramEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3336(%rax), %r11
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
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3336(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetHistogramEXT, .-glGetHistogramEXT

	.p2align	4,,15
	.globl	glGetHistogramParameterfvEXT
	.type	glGetHistogramParameterfvEXT, @function
glGetHistogramParameterfvEXT:
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
	.size	glGetHistogramParameterfvEXT, .-glGetHistogramParameterfvEXT

	.p2align	4,,15
	.globl	glGetHistogramParameterivEXT
	.type	glGetHistogramParameterivEXT, @function
glGetHistogramParameterivEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3352(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3352(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetHistogramParameterivEXT, .-glGetHistogramParameterivEXT

	.p2align	4,,15
	.globl	glGetMinmaxEXT
	.type	glGetMinmaxEXT, @function
glGetMinmaxEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3360(%rax), %r11
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
	movq	3360(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetMinmaxEXT, .-glGetMinmaxEXT

	.p2align	4,,15
	.globl	glGetMinmaxParameterfvEXT
	.type	glGetMinmaxParameterfvEXT, @function
glGetMinmaxParameterfvEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3368(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3368(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetMinmaxParameterfvEXT, .-glGetMinmaxParameterfvEXT

	.p2align	4,,15
	.globl	glGetMinmaxParameterivEXT
	.type	glGetMinmaxParameterivEXT, @function
glGetMinmaxParameterivEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3376(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3376(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetMinmaxParameterivEXT, .-glGetMinmaxParameterivEXT

	.p2align	4,,15
	.globl	glGetConvolutionFilterEXT
	.type	glGetConvolutionFilterEXT, @function
glGetConvolutionFilterEXT:
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
	.size	glGetConvolutionFilterEXT, .-glGetConvolutionFilterEXT

	.p2align	4,,15
	.globl	glGetConvolutionParameterfvEXT
	.type	glGetConvolutionParameterfvEXT, @function
glGetConvolutionParameterfvEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3392(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3392(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetConvolutionParameterfvEXT, .-glGetConvolutionParameterfvEXT

	.p2align	4,,15
	.globl	glGetConvolutionParameterivEXT
	.type	glGetConvolutionParameterivEXT, @function
glGetConvolutionParameterivEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3400(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3400(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetConvolutionParameterivEXT, .-glGetConvolutionParameterivEXT

	.p2align	4,,15
	.globl	glGetSeparableFilterEXT
	.type	glGetSeparableFilterEXT, @function
glGetSeparableFilterEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3408(%rax), %r11
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
	movq	3408(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetSeparableFilterEXT, .-glGetSeparableFilterEXT

	.p2align	4,,15
	.globl	glGetColorTableSGI
	.type	glGetColorTableSGI, @function
glGetColorTableSGI:
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
	.size	glGetColorTableSGI, .-glGetColorTableSGI

	.p2align	4,,15
	.globl	glGetColorTableParameterfvSGI
	.type	glGetColorTableParameterfvSGI, @function
glGetColorTableParameterfvSGI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3424(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3424(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetColorTableParameterfvSGI, .-glGetColorTableParameterfvSGI

	.p2align	4,,15
	.globl	glGetColorTableParameterivSGI
	.type	glGetColorTableParameterivSGI, @function
glGetColorTableParameterivSGI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3432(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3432(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetColorTableParameterivSGI, .-glGetColorTableParameterivSGI

	.p2align	4,,15
	.globl	glPixelTexGenSGIX
	.type	glPixelTexGenSGIX, @function
glPixelTexGenSGIX:
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
	.size	glPixelTexGenSGIX, .-glPixelTexGenSGIX

	.p2align	4,,15
	.globl	glPixelTexGenParameteriSGIS
	.type	glPixelTexGenParameteriSGIS, @function
glPixelTexGenParameteriSGIS:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3448(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
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
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3448(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glPixelTexGenParameteriSGIS, .-glPixelTexGenParameteriSGIS

	.p2align	4,,15
	.globl	glPixelTexGenParameterivSGIS
	.type	glPixelTexGenParameterivSGIS, @function
glPixelTexGenParameterivSGIS:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3456(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
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
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3456(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glPixelTexGenParameterivSGIS, .-glPixelTexGenParameterivSGIS

	.p2align	4,,15
	.globl	glPixelTexGenParameterfSGIS
	.type	glPixelTexGenParameterfSGIS, @function
glPixelTexGenParameterfSGIS:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3464(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3464(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3464(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3464(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glPixelTexGenParameterfSGIS, .-glPixelTexGenParameterfSGIS

	.p2align	4,,15
	.globl	glPixelTexGenParameterfvSGIS
	.type	glPixelTexGenParameterfvSGIS, @function
glPixelTexGenParameterfvSGIS:
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
	.size	glPixelTexGenParameterfvSGIS, .-glPixelTexGenParameterfvSGIS

	.p2align	4,,15
	.globl	glGetPixelTexGenParameterivSGIS
	.type	glGetPixelTexGenParameterivSGIS, @function
glGetPixelTexGenParameterivSGIS:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3480(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3480(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetPixelTexGenParameterivSGIS, .-glGetPixelTexGenParameterivSGIS

	.p2align	4,,15
	.globl	glGetPixelTexGenParameterfvSGIS
	.type	glGetPixelTexGenParameterfvSGIS, @function
glGetPixelTexGenParameterfvSGIS:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3488(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3488(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetPixelTexGenParameterfvSGIS, .-glGetPixelTexGenParameterfvSGIS

	.p2align	4,,15
	.globl	glTexImage4DSGIS
	.type	glTexImage4DSGIS, @function
glTexImage4DSGIS:
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
	.size	glTexImage4DSGIS, .-glTexImage4DSGIS

	.p2align	4,,15
	.globl	glTexSubImage4DSGIS
	.type	glTexSubImage4DSGIS, @function
glTexSubImage4DSGIS:
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
	.size	glTexSubImage4DSGIS, .-glTexSubImage4DSGIS

	.p2align	4,,15
	.globl	glAreTexturesResidentEXT
	.type	glAreTexturesResidentEXT, @function
glAreTexturesResidentEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3512(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3512(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glAreTexturesResidentEXT, .-glAreTexturesResidentEXT

	.p2align	4,,15
	.globl	glGenTexturesEXT
	.type	glGenTexturesEXT, @function
glGenTexturesEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3520(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3520(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGenTexturesEXT, .-glGenTexturesEXT

	.p2align	4,,15
	.globl	glIsTextureEXT
	.type	glIsTextureEXT, @function
glIsTextureEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3528(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3528(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glIsTextureEXT, .-glIsTextureEXT

	.p2align	4,,15
	.globl	glDetailTexFuncSGIS
	.type	glDetailTexFuncSGIS, @function
glDetailTexFuncSGIS:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3536(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3536(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glDetailTexFuncSGIS, .-glDetailTexFuncSGIS

	.p2align	4,,15
	.globl	glGetDetailTexFuncSGIS
	.type	glGetDetailTexFuncSGIS, @function
glGetDetailTexFuncSGIS:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3544(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
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
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3544(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetDetailTexFuncSGIS, .-glGetDetailTexFuncSGIS

	.p2align	4,,15
	.globl	glSharpenTexFuncSGIS
	.type	glSharpenTexFuncSGIS, @function
glSharpenTexFuncSGIS:
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
	.size	glSharpenTexFuncSGIS, .-glSharpenTexFuncSGIS

	.p2align	4,,15
	.globl	glGetSharpenTexFuncSGIS
	.type	glGetSharpenTexFuncSGIS, @function
glGetSharpenTexFuncSGIS:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3560(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3560(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetSharpenTexFuncSGIS, .-glGetSharpenTexFuncSGIS

	.p2align	4,,15
	.globl	glSampleMaskSGIS
	.type	glSampleMaskSGIS, @function
glSampleMaskSGIS:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3568(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3568(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSampleMaskSGIS, .-glSampleMaskSGIS

	.p2align	4,,15
	.globl	glSamplePatternSGIS
	.type	glSamplePatternSGIS, @function
glSamplePatternSGIS:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3576(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3576(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSamplePatternSGIS, .-glSamplePatternSGIS

	.p2align	4,,15
	.globl	glColorPointerEXT
	.type	glColorPointerEXT, @function
glColorPointerEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3584(%rax), %r11
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
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3584(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glColorPointerEXT, .-glColorPointerEXT

	.p2align	4,,15
	.globl	glEdgeFlagPointerEXT
	.type	glEdgeFlagPointerEXT, @function
glEdgeFlagPointerEXT:
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
	.size	glEdgeFlagPointerEXT, .-glEdgeFlagPointerEXT

	.p2align	4,,15
	.globl	glIndexPointerEXT
	.type	glIndexPointerEXT, @function
glIndexPointerEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3600(%rax), %r11
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
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3600(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glIndexPointerEXT, .-glIndexPointerEXT

	.p2align	4,,15
	.globl	glNormalPointerEXT
	.type	glNormalPointerEXT, @function
glNormalPointerEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3608(%rax), %r11
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
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3608(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glNormalPointerEXT, .-glNormalPointerEXT

	.p2align	4,,15
	.globl	glTexCoordPointerEXT
	.type	glTexCoordPointerEXT, @function
glTexCoordPointerEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3616(%rax), %r11
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
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3616(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glTexCoordPointerEXT, .-glTexCoordPointerEXT

	.p2align	4,,15
	.globl	glVertexPointerEXT
	.type	glVertexPointerEXT, @function
glVertexPointerEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3624(%rax), %r11
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
	movq	3624(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3624(%rax), %r11
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
	movq	3624(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexPointerEXT, .-glVertexPointerEXT

	.p2align	4,,15
	.globl	glSpriteParameterfSGIX
	.type	glSpriteParameterfSGIX, @function
glSpriteParameterfSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3632(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3632(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3632(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3632(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSpriteParameterfSGIX, .-glSpriteParameterfSGIX

	.p2align	4,,15
	.globl	glSpriteParameterfvSGIX
	.type	glSpriteParameterfvSGIX, @function
glSpriteParameterfvSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3640(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3640(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3640(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3640(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSpriteParameterfvSGIX, .-glSpriteParameterfvSGIX

	.p2align	4,,15
	.globl	glSpriteParameteriSGIX
	.type	glSpriteParameteriSGIX, @function
glSpriteParameteriSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3648(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3648(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSpriteParameteriSGIX, .-glSpriteParameteriSGIX

	.p2align	4,,15
	.globl	glSpriteParameterivSGIX
	.type	glSpriteParameterivSGIX, @function
glSpriteParameterivSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3656(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3656(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3656(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3656(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSpriteParameterivSGIX, .-glSpriteParameterivSGIX

	.p2align	4,,15
	.globl	glPointParameterfEXT
	.type	glPointParameterfEXT, @function
glPointParameterfEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3664(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3664(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3664(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3664(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glPointParameterfEXT, .-glPointParameterfEXT

	.p2align	4,,15
	.globl	glPointParameterfvEXT
	.type	glPointParameterfvEXT, @function
glPointParameterfvEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3672(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3672(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3672(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3672(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glPointParameterfvEXT, .-glPointParameterfvEXT

	.p2align	4,,15
	.globl	glGetInstrumentsSGIX
	.type	glGetInstrumentsSGIX, @function
glGetInstrumentsSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3680(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	3680(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3680(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	3680(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetInstrumentsSGIX, .-glGetInstrumentsSGIX

	.p2align	4,,15
	.globl	glInstrumentsBufferSGIX
	.type	glInstrumentsBufferSGIX, @function
glInstrumentsBufferSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3688(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3688(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glInstrumentsBufferSGIX, .-glInstrumentsBufferSGIX

	.p2align	4,,15
	.globl	glPollInstrumentsSGIX
	.type	glPollInstrumentsSGIX, @function
glPollInstrumentsSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3696(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3696(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3696(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3696(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glPollInstrumentsSGIX, .-glPollInstrumentsSGIX

	.p2align	4,,15
	.globl	glReadInstrumentsSGIX
	.type	glReadInstrumentsSGIX, @function
glReadInstrumentsSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3704(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3704(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glReadInstrumentsSGIX, .-glReadInstrumentsSGIX

	.p2align	4,,15
	.globl	glStartInstrumentsSGIX
	.type	glStartInstrumentsSGIX, @function
glStartInstrumentsSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3712(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	3712(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3712(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	3712(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glStartInstrumentsSGIX, .-glStartInstrumentsSGIX

	.p2align	4,,15
	.globl	glStopInstrumentsSGIX
	.type	glStopInstrumentsSGIX, @function
glStopInstrumentsSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3720(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3720(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glStopInstrumentsSGIX, .-glStopInstrumentsSGIX

	.p2align	4,,15
	.globl	glFrameZoomSGIX
	.type	glFrameZoomSGIX, @function
glFrameZoomSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3728(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3728(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFrameZoomSGIX, .-glFrameZoomSGIX

	.p2align	4,,15
	.globl	glTagSampleBufferSGIX
	.type	glTagSampleBufferSGIX, @function
glTagSampleBufferSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3736(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	3736(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3736(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	3736(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glTagSampleBufferSGIX, .-glTagSampleBufferSGIX

	.p2align	4,,15
	.globl	glReferencePlaneSGIX
	.type	glReferencePlaneSGIX, @function
glReferencePlaneSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3744(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	3744(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3744(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3744(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glReferencePlaneSGIX, .-glReferencePlaneSGIX

	.p2align	4,,15
	.globl	glFlushRasterSGIX
	.type	glFlushRasterSGIX, @function
glFlushRasterSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3752(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	3752(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3752(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	3752(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFlushRasterSGIX, .-glFlushRasterSGIX

	.p2align	4,,15
	.globl	glGetListParameterfvSGIX
	.type	glGetListParameterfvSGIX, @function
glGetListParameterfvSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3760(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3760(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3760(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3760(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetListParameterfvSGIX, .-glGetListParameterfvSGIX

	.p2align	4,,15
	.globl	glGetListParameterivSGIX
	.type	glGetListParameterivSGIX, @function
glGetListParameterivSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3768(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3768(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetListParameterivSGIX, .-glGetListParameterivSGIX

	.p2align	4,,15
	.globl	glListParameterfSGIX
	.type	glListParameterfSGIX, @function
glListParameterfSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3776(%rax), %r11
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
	movq	3776(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3776(%rax), %r11
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
	movq	3776(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glListParameterfSGIX, .-glListParameterfSGIX

	.p2align	4,,15
	.globl	glListParameterfvSGIX
	.type	glListParameterfvSGIX, @function
glListParameterfvSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3784(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3784(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glListParameterfvSGIX, .-glListParameterfvSGIX

	.p2align	4,,15
	.globl	glListParameteriSGIX
	.type	glListParameteriSGIX, @function
glListParameteriSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3792(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3792(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3792(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3792(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glListParameteriSGIX, .-glListParameteriSGIX

	.p2align	4,,15
	.globl	glListParameterivSGIX
	.type	glListParameterivSGIX, @function
glListParameterivSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3800(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3800(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glListParameterivSGIX, .-glListParameterivSGIX

	.p2align	4,,15
	.globl	glFragmentColorMaterialSGIX
	.type	glFragmentColorMaterialSGIX, @function
glFragmentColorMaterialSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3808(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3808(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3808(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3808(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFragmentColorMaterialSGIX, .-glFragmentColorMaterialSGIX

	.p2align	4,,15
	.globl	glFragmentLightfSGIX
	.type	glFragmentLightfSGIX, @function
glFragmentLightfSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3816(%rax), %r11
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
	movq	3816(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3816(%rax), %r11
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
	movq	3816(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFragmentLightfSGIX, .-glFragmentLightfSGIX

	.p2align	4,,15
	.globl	glFragmentLightfvSGIX
	.type	glFragmentLightfvSGIX, @function
glFragmentLightfvSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3824(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3824(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFragmentLightfvSGIX, .-glFragmentLightfvSGIX

	.p2align	4,,15
	.globl	glFragmentLightiSGIX
	.type	glFragmentLightiSGIX, @function
glFragmentLightiSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3832(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3832(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFragmentLightiSGIX, .-glFragmentLightiSGIX

	.p2align	4,,15
	.globl	glFragmentLightivSGIX
	.type	glFragmentLightivSGIX, @function
glFragmentLightivSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3840(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3840(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFragmentLightivSGIX, .-glFragmentLightivSGIX

	.p2align	4,,15
	.globl	glFragmentLightModelfSGIX
	.type	glFragmentLightModelfSGIX, @function
glFragmentLightModelfSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3848(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3848(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3848(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	3848(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFragmentLightModelfSGIX, .-glFragmentLightModelfSGIX

	.p2align	4,,15
	.globl	glFragmentLightModelfvSGIX
	.type	glFragmentLightModelfvSGIX, @function
glFragmentLightModelfvSGIX:
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
	.size	glFragmentLightModelfvSGIX, .-glFragmentLightModelfvSGIX

	.p2align	4,,15
	.globl	glFragmentLightModeliSGIX
	.type	glFragmentLightModeliSGIX, @function
glFragmentLightModeliSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3864(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3864(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFragmentLightModeliSGIX, .-glFragmentLightModeliSGIX

	.p2align	4,,15
	.globl	glFragmentLightModelivSGIX
	.type	glFragmentLightModelivSGIX, @function
glFragmentLightModelivSGIX:
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
	.size	glFragmentLightModelivSGIX, .-glFragmentLightModelivSGIX

	.p2align	4,,15
	.globl	glFragmentMaterialfSGIX
	.type	glFragmentMaterialfSGIX, @function
glFragmentMaterialfSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3880(%rax), %r11
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
	movq	3880(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3880(%rax), %r11
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
	movq	3880(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFragmentMaterialfSGIX, .-glFragmentMaterialfSGIX

	.p2align	4,,15
	.globl	glFragmentMaterialfvSGIX
	.type	glFragmentMaterialfvSGIX, @function
glFragmentMaterialfvSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3888(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3888(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFragmentMaterialfvSGIX, .-glFragmentMaterialfvSGIX

	.p2align	4,,15
	.globl	glFragmentMaterialiSGIX
	.type	glFragmentMaterialiSGIX, @function
glFragmentMaterialiSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3896(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3896(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFragmentMaterialiSGIX, .-glFragmentMaterialiSGIX

	.p2align	4,,15
	.globl	glFragmentMaterialivSGIX
	.type	glFragmentMaterialivSGIX, @function
glFragmentMaterialivSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3904(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3904(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3904(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3904(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFragmentMaterialivSGIX, .-glFragmentMaterialivSGIX

	.p2align	4,,15
	.globl	glGetFragmentLightfvSGIX
	.type	glGetFragmentLightfvSGIX, @function
glGetFragmentLightfvSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3912(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3912(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetFragmentLightfvSGIX, .-glGetFragmentLightfvSGIX

	.p2align	4,,15
	.globl	glGetFragmentLightivSGIX
	.type	glGetFragmentLightivSGIX, @function
glGetFragmentLightivSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3920(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3920(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3920(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3920(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetFragmentLightivSGIX, .-glGetFragmentLightivSGIX

	.p2align	4,,15
	.globl	glGetFragmentMaterialfvSGIX
	.type	glGetFragmentMaterialfvSGIX, @function
glGetFragmentMaterialfvSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3928(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3928(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetFragmentMaterialfvSGIX, .-glGetFragmentMaterialfvSGIX

	.p2align	4,,15
	.globl	glGetFragmentMaterialivSGIX
	.type	glGetFragmentMaterialivSGIX, @function
glGetFragmentMaterialivSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3936(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3936(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetFragmentMaterialivSGIX, .-glGetFragmentMaterialivSGIX

	.p2align	4,,15
	.globl	glLightEnviSGIX
	.type	glLightEnviSGIX, @function
glLightEnviSGIX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3944(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3944(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glLightEnviSGIX, .-glLightEnviSGIX

	.p2align	4,,15
	.globl	glVertexWeightfEXT
	.type	glVertexWeightfEXT, @function
glVertexWeightfEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3952(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	3952(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3952(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	3952(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexWeightfEXT, .-glVertexWeightfEXT

	.p2align	4,,15
	.globl	glVertexWeightfvEXT
	.type	glVertexWeightfvEXT, @function
glVertexWeightfvEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3960(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	3960(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexWeightfvEXT, .-glVertexWeightfvEXT

	.p2align	4,,15
	.globl	glVertexWeightPointerEXT
	.type	glVertexWeightPointerEXT, @function
glVertexWeightPointerEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3968(%rax), %r11
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
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	3968(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexWeightPointerEXT, .-glVertexWeightPointerEXT

	.p2align	4,,15
	.globl	glFlushVertexArrayRangeNV
	.type	glFlushVertexArrayRangeNV, @function
glFlushVertexArrayRangeNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3976(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	3976(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	3976(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	3976(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFlushVertexArrayRangeNV, .-glFlushVertexArrayRangeNV

	.p2align	4,,15
	.globl	glVertexArrayRangeNV
	.type	glVertexArrayRangeNV, @function
glVertexArrayRangeNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	3984(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	3984(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexArrayRangeNV, .-glVertexArrayRangeNV

	.p2align	4,,15
	.globl	glCombinerParameterfvNV
	.type	glCombinerParameterfvNV, @function
glCombinerParameterfvNV:
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
	.size	glCombinerParameterfvNV, .-glCombinerParameterfvNV

	.p2align	4,,15
	.globl	glCombinerParameterfNV
	.type	glCombinerParameterfNV, @function
glCombinerParameterfNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4000(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4000(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4000(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4000(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCombinerParameterfNV, .-glCombinerParameterfNV

	.p2align	4,,15
	.globl	glCombinerParameterivNV
	.type	glCombinerParameterivNV, @function
glCombinerParameterivNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4008(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4008(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCombinerParameterivNV, .-glCombinerParameterivNV

	.p2align	4,,15
	.globl	glCombinerParameteriNV
	.type	glCombinerParameteriNV, @function
glCombinerParameteriNV:
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
	.size	glCombinerParameteriNV, .-glCombinerParameteriNV

	.p2align	4,,15
	.globl	glCombinerInputNV
	.type	glCombinerInputNV, @function
glCombinerInputNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4024(%rax), %r11
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
	movq	4024(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCombinerInputNV, .-glCombinerInputNV

	.p2align	4,,15
	.globl	glCombinerOutputNV
	.type	glCombinerOutputNV, @function
glCombinerOutputNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4032(%rax), %r11
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
	movq	4032(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCombinerOutputNV, .-glCombinerOutputNV

	.p2align	4,,15
	.globl	glFinalCombinerInputNV
	.type	glFinalCombinerInputNV, @function
glFinalCombinerInputNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4040(%rax), %r11
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
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4040(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFinalCombinerInputNV, .-glFinalCombinerInputNV

	.p2align	4,,15
	.globl	glGetCombinerInputParameterfvNV
	.type	glGetCombinerInputParameterfvNV, @function
glGetCombinerInputParameterfvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4048(%rax), %r11
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
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4048(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetCombinerInputParameterfvNV, .-glGetCombinerInputParameterfvNV

	.p2align	4,,15
	.globl	glGetCombinerInputParameterivNV
	.type	glGetCombinerInputParameterivNV, @function
glGetCombinerInputParameterivNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4056(%rax), %r11
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
	movq	4056(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetCombinerInputParameterivNV, .-glGetCombinerInputParameterivNV

	.p2align	4,,15
	.globl	glGetCombinerOutputParameterfvNV
	.type	glGetCombinerOutputParameterfvNV, @function
glGetCombinerOutputParameterfvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4064(%rax), %r11
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
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4064(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetCombinerOutputParameterfvNV, .-glGetCombinerOutputParameterfvNV

	.p2align	4,,15
	.globl	glGetCombinerOutputParameterivNV
	.type	glGetCombinerOutputParameterivNV, @function
glGetCombinerOutputParameterivNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4072(%rax), %r11
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
	movq	4072(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetCombinerOutputParameterivNV, .-glGetCombinerOutputParameterivNV

	.p2align	4,,15
	.globl	glGetFinalCombinerInputParameterfvNV
	.type	glGetFinalCombinerInputParameterfvNV, @function
glGetFinalCombinerInputParameterfvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4080(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4080(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetFinalCombinerInputParameterfvNV, .-glGetFinalCombinerInputParameterfvNV

	.p2align	4,,15
	.globl	glGetFinalCombinerInputParameterivNV
	.type	glGetFinalCombinerInputParameterivNV, @function
glGetFinalCombinerInputParameterivNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4088(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4088(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetFinalCombinerInputParameterivNV, .-glGetFinalCombinerInputParameterivNV

	.p2align	4,,15
	.globl	glResizeBuffersMESA
	.type	glResizeBuffersMESA, @function
glResizeBuffersMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4096(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	4096(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4096(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	4096(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glResizeBuffersMESA, .-glResizeBuffersMESA

	.p2align	4,,15
	.globl	glWindowPos2dMESA
	.type	glWindowPos2dMESA, @function
glWindowPos2dMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4104(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	4104(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4104(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	4104(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos2dMESA, .-glWindowPos2dMESA

	.p2align	4,,15
	.globl	glWindowPos2dvMESA
	.type	glWindowPos2dvMESA, @function
glWindowPos2dvMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4112(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4112(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos2dvMESA, .-glWindowPos2dvMESA

	.p2align	4,,15
	.globl	glWindowPos2fMESA
	.type	glWindowPos2fMESA, @function
glWindowPos2fMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4120(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	4120(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4120(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%xmm0, (%rsp)
	movq	%xmm1, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm1
	movq	(%rsp), %xmm0
	addq	$24, %rsp
	movq	4120(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos2fMESA, .-glWindowPos2fMESA

	.p2align	4,,15
	.globl	glWindowPos2fvMESA
	.type	glWindowPos2fvMESA, @function
glWindowPos2fvMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4128(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4128(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos2fvMESA, .-glWindowPos2fvMESA

	.p2align	4,,15
	.globl	glWindowPos2iMESA
	.type	glWindowPos2iMESA, @function
glWindowPos2iMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4136(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
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
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4136(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos2iMESA, .-glWindowPos2iMESA

	.p2align	4,,15
	.globl	glWindowPos2ivMESA
	.type	glWindowPos2ivMESA, @function
glWindowPos2ivMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4144(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4144(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos2ivMESA, .-glWindowPos2ivMESA

	.p2align	4,,15
	.globl	glWindowPos2sMESA
	.type	glWindowPos2sMESA, @function
glWindowPos2sMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4152(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
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
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4152(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos2sMESA, .-glWindowPos2sMESA

	.p2align	4,,15
	.globl	glWindowPos2svMESA
	.type	glWindowPos2svMESA, @function
glWindowPos2svMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4160(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4160(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4160(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4160(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos2svMESA, .-glWindowPos2svMESA

	.p2align	4,,15
	.globl	glWindowPos3dMESA
	.type	glWindowPos3dMESA, @function
glWindowPos3dMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4168(%rax), %r11
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
	movq	4168(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4168(%rax), %r11
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
	movq	4168(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos3dMESA, .-glWindowPos3dMESA

	.p2align	4,,15
	.globl	glWindowPos3dvMESA
	.type	glWindowPos3dvMESA, @function
glWindowPos3dvMESA:
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
	.size	glWindowPos3dvMESA, .-glWindowPos3dvMESA

	.p2align	4,,15
	.globl	glWindowPos3fMESA
	.type	glWindowPos3fMESA, @function
glWindowPos3fMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4184(%rax), %r11
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
	movq	4184(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4184(%rax), %r11
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
	movq	4184(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos3fMESA, .-glWindowPos3fMESA

	.p2align	4,,15
	.globl	glWindowPos3fvMESA
	.type	glWindowPos3fvMESA, @function
glWindowPos3fvMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4192(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4192(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos3fvMESA, .-glWindowPos3fvMESA

	.p2align	4,,15
	.globl	glWindowPos3iMESA
	.type	glWindowPos3iMESA, @function
glWindowPos3iMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4200(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4200(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos3iMESA, .-glWindowPos3iMESA

	.p2align	4,,15
	.globl	glWindowPos3ivMESA
	.type	glWindowPos3ivMESA, @function
glWindowPos3ivMESA:
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
	.size	glWindowPos3ivMESA, .-glWindowPos3ivMESA

	.p2align	4,,15
	.globl	glWindowPos3sMESA
	.type	glWindowPos3sMESA, @function
glWindowPos3sMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4216(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4216(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos3sMESA, .-glWindowPos3sMESA

	.p2align	4,,15
	.globl	glWindowPos3svMESA
	.type	glWindowPos3svMESA, @function
glWindowPos3svMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4224(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4224(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos3svMESA, .-glWindowPos3svMESA

	.p2align	4,,15
	.globl	glWindowPos4dMESA
	.type	glWindowPos4dMESA, @function
glWindowPos4dMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4232(%rax), %r11
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
	movq	4232(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4232(%rax), %r11
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
	movq	4232(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos4dMESA, .-glWindowPos4dMESA

	.p2align	4,,15
	.globl	glWindowPos4dvMESA
	.type	glWindowPos4dvMESA, @function
glWindowPos4dvMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4240(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4240(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos4dvMESA, .-glWindowPos4dvMESA

	.p2align	4,,15
	.globl	glWindowPos4fMESA
	.type	glWindowPos4fMESA, @function
glWindowPos4fMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4248(%rax), %r11
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
	movq	4248(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4248(%rax), %r11
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
	movq	4248(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos4fMESA, .-glWindowPos4fMESA

	.p2align	4,,15
	.globl	glWindowPos4fvMESA
	.type	glWindowPos4fvMESA, @function
glWindowPos4fvMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4256(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4256(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos4fvMESA, .-glWindowPos4fvMESA

	.p2align	4,,15
	.globl	glWindowPos4iMESA
	.type	glWindowPos4iMESA, @function
glWindowPos4iMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4264(%rax), %r11
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
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4264(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos4iMESA, .-glWindowPos4iMESA

	.p2align	4,,15
	.globl	glWindowPos4ivMESA
	.type	glWindowPos4ivMESA, @function
glWindowPos4ivMESA:
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
	.size	glWindowPos4ivMESA, .-glWindowPos4ivMESA

	.p2align	4,,15
	.globl	glWindowPos4sMESA
	.type	glWindowPos4sMESA, @function
glWindowPos4sMESA:
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
	.size	glWindowPos4sMESA, .-glWindowPos4sMESA

	.p2align	4,,15
	.globl	glWindowPos4svMESA
	.type	glWindowPos4svMESA, @function
glWindowPos4svMESA:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4288(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4288(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4288(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4288(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glWindowPos4svMESA, .-glWindowPos4svMESA

	.p2align	4,,15
	.globl	glBlendFuncSeparateEXT
	.type	glBlendFuncSeparateEXT, @function
glBlendFuncSeparateEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4296(%rax), %r11
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
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4296(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glBlendFuncSeparateEXT, .-glBlendFuncSeparateEXT

	.p2align	4,,15
	.globl	glIndexMaterialEXT
	.type	glIndexMaterialEXT, @function
glIndexMaterialEXT:
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
	.size	glIndexMaterialEXT, .-glIndexMaterialEXT

	.p2align	4,,15
	.globl	glIndexFuncEXT
	.type	glIndexFuncEXT, @function
glIndexFuncEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4312(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4312(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glIndexFuncEXT, .-glIndexFuncEXT

	.p2align	4,,15
	.globl	glLockArraysEXT
	.type	glLockArraysEXT, @function
glLockArraysEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4320(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4320(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4320(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4320(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glLockArraysEXT, .-glLockArraysEXT

	.p2align	4,,15
	.globl	glUnlockArraysEXT
	.type	glUnlockArraysEXT, @function
glUnlockArraysEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4328(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	4328(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4328(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	4328(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUnlockArraysEXT, .-glUnlockArraysEXT

	.p2align	4,,15
	.globl	glCullParameterdvEXT
	.type	glCullParameterdvEXT, @function
glCullParameterdvEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4336(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4336(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCullParameterdvEXT, .-glCullParameterdvEXT

	.p2align	4,,15
	.globl	glCullParameterfvEXT
	.type	glCullParameterfvEXT, @function
glCullParameterfvEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4344(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4344(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCullParameterfvEXT, .-glCullParameterfvEXT

	.p2align	4,,15
	.globl	glHintPGI
	.type	glHintPGI, @function
glHintPGI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4352(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4352(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4352(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4352(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glHintPGI, .-glHintPGI

	.p2align	4,,15
	.globl	glFogCoordfEXT
	.type	glFogCoordfEXT, @function
glFogCoordfEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4360(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	4360(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4360(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	4360(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFogCoordfEXT, .-glFogCoordfEXT

	.p2align	4,,15
	.globl	glFogCoordfvEXT
	.type	glFogCoordfvEXT, @function
glFogCoordfvEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4368(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4368(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFogCoordfvEXT, .-glFogCoordfvEXT

	.p2align	4,,15
	.globl	glFogCoorddEXT
	.type	glFogCoorddEXT, @function
glFogCoorddEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4376(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	4376(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4376(%rax), %r11
	jmp	*%r11
1:
	subq	$8, %rsp
	movq	%xmm0, (%rsp)
	call	_glapi_get_dispatch
	movq	(%rsp), %xmm0
	addq	$8, %rsp
	movq	4376(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFogCoorddEXT, .-glFogCoorddEXT

	.p2align	4,,15
	.globl	glFogCoorddvEXT
	.type	glFogCoorddvEXT, @function
glFogCoorddvEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4384(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4384(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4384(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4384(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFogCoorddvEXT, .-glFogCoorddvEXT

	.p2align	4,,15
	.globl	glFogCoordPointerEXT
	.type	glFogCoordPointerEXT, @function
glFogCoordPointerEXT:
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
	.size	glFogCoordPointerEXT, .-glFogCoordPointerEXT

	.p2align	4,,15
	.globl	glGetColorTableEXT
	.type	glGetColorTableEXT, @function
glGetColorTableEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4400(%rax), %r11
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4400(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetColorTableEXT, .-glGetColorTableEXT

	.p2align	4,,15
	.globl	glGetColorTableParameterivEXT
	.type	glGetColorTableParameterivEXT, @function
glGetColorTableParameterivEXT:
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
	.size	glGetColorTableParameterivEXT, .-glGetColorTableParameterivEXT

	.p2align	4,,15
	.globl	glGetColorTableParameterfvEXT
	.type	glGetColorTableParameterfvEXT, @function
glGetColorTableParameterfvEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4416(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4416(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetColorTableParameterfvEXT, .-glGetColorTableParameterfvEXT

	.p2align	4,,15
	.globl	glTbufferMask3DFX
	.type	glTbufferMask3DFX, @function
glTbufferMask3DFX:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4424(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4424(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glTbufferMask3DFX, .-glTbufferMask3DFX

	.p2align	4,,15
	.globl	glCompressedTexImage3DARB
	.type	glCompressedTexImage3DARB, @function
glCompressedTexImage3DARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4432(%rax), %r11
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
	movq	4432(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCompressedTexImage3DARB, .-glCompressedTexImage3DARB

	.p2align	4,,15
	.globl	glCompressedTexImage2DARB
	.type	glCompressedTexImage2DARB, @function
glCompressedTexImage2DARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4440(%rax), %r11
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
	movq	4440(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCompressedTexImage2DARB, .-glCompressedTexImage2DARB

	.p2align	4,,15
	.globl	glCompressedTexImage1DARB
	.type	glCompressedTexImage1DARB, @function
glCompressedTexImage1DARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4448(%rax), %r11
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
	movq	4448(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCompressedTexImage1DARB, .-glCompressedTexImage1DARB

	.p2align	4,,15
	.globl	glCompressedTexSubImage3DARB
	.type	glCompressedTexSubImage3DARB, @function
glCompressedTexSubImage3DARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4456(%rax), %r11
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
	movq	4456(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCompressedTexSubImage3DARB, .-glCompressedTexSubImage3DARB

	.p2align	4,,15
	.globl	glCompressedTexSubImage2DARB
	.type	glCompressedTexSubImage2DARB, @function
glCompressedTexSubImage2DARB:
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
	.size	glCompressedTexSubImage2DARB, .-glCompressedTexSubImage2DARB

	.p2align	4,,15
	.globl	glCompressedTexSubImage1DARB
	.type	glCompressedTexSubImage1DARB, @function
glCompressedTexSubImage1DARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4472(%rax), %r11
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
	movq	4472(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCompressedTexSubImage1DARB, .-glCompressedTexSubImage1DARB

	.p2align	4,,15
	.globl	glGetCompressedTexImageARB
	.type	glGetCompressedTexImageARB, @function
glGetCompressedTexImageARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4480(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4480(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetCompressedTexImageARB, .-glGetCompressedTexImageARB

	.p2align	4,,15
	.globl	glSecondaryColor3bEXT
	.type	glSecondaryColor3bEXT, @function
glSecondaryColor3bEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4488(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4488(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4488(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4488(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3bEXT, .-glSecondaryColor3bEXT

	.p2align	4,,15
	.globl	glSecondaryColor3bvEXT
	.type	glSecondaryColor3bvEXT, @function
glSecondaryColor3bvEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4496(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4496(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3bvEXT, .-glSecondaryColor3bvEXT

	.p2align	4,,15
	.globl	glSecondaryColor3dEXT
	.type	glSecondaryColor3dEXT, @function
glSecondaryColor3dEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4504(%rax), %r11
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
	movq	4504(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4504(%rax), %r11
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
	movq	4504(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3dEXT, .-glSecondaryColor3dEXT

	.p2align	4,,15
	.globl	glSecondaryColor3dvEXT
	.type	glSecondaryColor3dvEXT, @function
glSecondaryColor3dvEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4512(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4512(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4512(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4512(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3dvEXT, .-glSecondaryColor3dvEXT

	.p2align	4,,15
	.globl	glSecondaryColor3fEXT
	.type	glSecondaryColor3fEXT, @function
glSecondaryColor3fEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4520(%rax), %r11
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
	movq	4520(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4520(%rax), %r11
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
	movq	4520(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3fEXT, .-glSecondaryColor3fEXT

	.p2align	4,,15
	.globl	glSecondaryColor3fvEXT
	.type	glSecondaryColor3fvEXT, @function
glSecondaryColor3fvEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4528(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4528(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3fvEXT, .-glSecondaryColor3fvEXT

	.p2align	4,,15
	.globl	glSecondaryColor3iEXT
	.type	glSecondaryColor3iEXT, @function
glSecondaryColor3iEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4536(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4536(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3iEXT, .-glSecondaryColor3iEXT

	.p2align	4,,15
	.globl	glSecondaryColor3ivEXT
	.type	glSecondaryColor3ivEXT, @function
glSecondaryColor3ivEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4544(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4544(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3ivEXT, .-glSecondaryColor3ivEXT

	.p2align	4,,15
	.globl	glSecondaryColor3sEXT
	.type	glSecondaryColor3sEXT, @function
glSecondaryColor3sEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4552(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4552(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3sEXT, .-glSecondaryColor3sEXT

	.p2align	4,,15
	.globl	glSecondaryColor3svEXT
	.type	glSecondaryColor3svEXT, @function
glSecondaryColor3svEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4560(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4560(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3svEXT, .-glSecondaryColor3svEXT

	.p2align	4,,15
	.globl	glSecondaryColor3ubEXT
	.type	glSecondaryColor3ubEXT, @function
glSecondaryColor3ubEXT:
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
	.size	glSecondaryColor3ubEXT, .-glSecondaryColor3ubEXT

	.p2align	4,,15
	.globl	glSecondaryColor3ubvEXT
	.type	glSecondaryColor3ubvEXT, @function
glSecondaryColor3ubvEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4576(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4576(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3ubvEXT, .-glSecondaryColor3ubvEXT

	.p2align	4,,15
	.globl	glSecondaryColor3uiEXT
	.type	glSecondaryColor3uiEXT, @function
glSecondaryColor3uiEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4584(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4584(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3uiEXT, .-glSecondaryColor3uiEXT

	.p2align	4,,15
	.globl	glSecondaryColor3uivEXT
	.type	glSecondaryColor3uivEXT, @function
glSecondaryColor3uivEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4592(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4592(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3uivEXT, .-glSecondaryColor3uivEXT

	.p2align	4,,15
	.globl	glSecondaryColor3usEXT
	.type	glSecondaryColor3usEXT, @function
glSecondaryColor3usEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4600(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4600(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3usEXT, .-glSecondaryColor3usEXT

	.p2align	4,,15
	.globl	glSecondaryColor3usvEXT
	.type	glSecondaryColor3usvEXT, @function
glSecondaryColor3usvEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4608(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	4608(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4608(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4608(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColor3usvEXT, .-glSecondaryColor3usvEXT

	.p2align	4,,15
	.globl	glSecondaryColorPointerEXT
	.type	glSecondaryColorPointerEXT, @function
glSecondaryColorPointerEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4616(%rax), %r11
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
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4616(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSecondaryColorPointerEXT, .-glSecondaryColorPointerEXT

	.p2align	4,,15
	.globl	glAreProgramsResidentNV
	.type	glAreProgramsResidentNV, @function
glAreProgramsResidentNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4624(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4624(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glAreProgramsResidentNV, .-glAreProgramsResidentNV

	.p2align	4,,15
	.globl	glBindProgramNV
	.type	glBindProgramNV, @function
glBindProgramNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4632(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4632(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4632(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4632(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glBindProgramNV, .-glBindProgramNV

	.p2align	4,,15
	.globl	glDeleteProgramsNV
	.type	glDeleteProgramsNV, @function
glDeleteProgramsNV:
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
	.size	glDeleteProgramsNV, .-glDeleteProgramsNV

	.p2align	4,,15
	.globl	glExecuteProgramNV
	.type	glExecuteProgramNV, @function
glExecuteProgramNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4648(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4648(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glExecuteProgramNV, .-glExecuteProgramNV

	.p2align	4,,15
	.globl	glGenProgramsNV
	.type	glGenProgramsNV, @function
glGenProgramsNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4656(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4656(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGenProgramsNV, .-glGenProgramsNV

	.p2align	4,,15
	.globl	glGetProgramParameterdvNV
	.type	glGetProgramParameterdvNV, @function
glGetProgramParameterdvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4664(%rax), %r11
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
	movq	4664(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetProgramParameterdvNV, .-glGetProgramParameterdvNV

	.p2align	4,,15
	.globl	glGetProgramParameterfvNV
	.type	glGetProgramParameterfvNV, @function
glGetProgramParameterfvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4672(%rax), %r11
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
	movq	4672(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4672(%rax), %r11
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
	movq	4672(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetProgramParameterfvNV, .-glGetProgramParameterfvNV

	.p2align	4,,15
	.globl	glGetProgramivNV
	.type	glGetProgramivNV, @function
glGetProgramivNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4680(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4680(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetProgramivNV, .-glGetProgramivNV

	.p2align	4,,15
	.globl	glGetProgramStringNV
	.type	glGetProgramStringNV, @function
glGetProgramStringNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4688(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4688(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4688(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4688(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetProgramStringNV, .-glGetProgramStringNV

	.p2align	4,,15
	.globl	glGetTrackMatrixivNV
	.type	glGetTrackMatrixivNV, @function
glGetTrackMatrixivNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4696(%rax), %r11
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
	movq	4696(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetTrackMatrixivNV, .-glGetTrackMatrixivNV

	.p2align	4,,15
	.globl	glGetVertexAttribdvARB
	.type	glGetVertexAttribdvARB, @function
glGetVertexAttribdvARB:
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
	.size	glGetVertexAttribdvARB, .-glGetVertexAttribdvARB

	.p2align	4,,15
	.globl	glGetVertexAttribfvARB
	.type	glGetVertexAttribfvARB, @function
glGetVertexAttribfvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4712(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4712(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetVertexAttribfvARB, .-glGetVertexAttribfvARB

	.p2align	4,,15
	.globl	glGetVertexAttribivARB
	.type	glGetVertexAttribivARB, @function
glGetVertexAttribivARB:
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
	.size	glGetVertexAttribivARB, .-glGetVertexAttribivARB

	.p2align	4,,15
	.globl	glGetVertexAttribPointervNV
	.type	glGetVertexAttribPointervNV, @function
glGetVertexAttribPointervNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4728(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4728(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetVertexAttribPointervNV, .-glGetVertexAttribPointervNV

	.p2align	4,,15
	.globl	glIsProgramNV
	.type	glIsProgramNV, @function
glIsProgramNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4736(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	4736(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glIsProgramNV, .-glIsProgramNV

	.p2align	4,,15
	.globl	glLoadProgramNV
	.type	glLoadProgramNV, @function
glLoadProgramNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4744(%rax), %r11
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
	movq	4744(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glLoadProgramNV, .-glLoadProgramNV

	.p2align	4,,15
	.globl	glProgramParameter4dNV
	.type	glProgramParameter4dNV, @function
glProgramParameter4dNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4752(%rax), %r11
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
	movq	4752(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4752(%rax), %r11
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
	movq	4752(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramParameter4dNV, .-glProgramParameter4dNV

	.p2align	4,,15
	.globl	glProgramParameter4dvNV
	.type	glProgramParameter4dvNV, @function
glProgramParameter4dvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4760(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4760(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramParameter4dvNV, .-glProgramParameter4dvNV

	.p2align	4,,15
	.globl	glProgramParameter4fNV
	.type	glProgramParameter4fNV, @function
glProgramParameter4fNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4768(%rax), %r11
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
	movq	4768(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4768(%rax), %r11
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
	movq	4768(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramParameter4fNV, .-glProgramParameter4fNV

	.p2align	4,,15
	.globl	glProgramParameter4fvNV
	.type	glProgramParameter4fvNV, @function
glProgramParameter4fvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4776(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4776(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramParameter4fvNV, .-glProgramParameter4fvNV

	.p2align	4,,15
	.globl	glProgramParameters4dvNV
	.type	glProgramParameters4dvNV, @function
glProgramParameters4dvNV:
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
	.size	glProgramParameters4dvNV, .-glProgramParameters4dvNV

	.p2align	4,,15
	.globl	glProgramParameters4fvNV
	.type	glProgramParameters4fvNV, @function
glProgramParameters4fvNV:
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
	.size	glProgramParameters4fvNV, .-glProgramParameters4fvNV

	.p2align	4,,15
	.globl	glRequestResidentProgramsNV
	.type	glRequestResidentProgramsNV, @function
glRequestResidentProgramsNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4800(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4800(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glRequestResidentProgramsNV, .-glRequestResidentProgramsNV

	.p2align	4,,15
	.globl	glTrackMatrixNV
	.type	glTrackMatrixNV, @function
glTrackMatrixNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4808(%rax), %r11
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
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4808(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glTrackMatrixNV, .-glTrackMatrixNV

	.p2align	4,,15
	.globl	glVertexAttribPointerNV
	.type	glVertexAttribPointerNV, @function
glVertexAttribPointerNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4816(%rax), %r11
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
	movq	4816(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4816(%rax), %r11
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
	movq	4816(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttribPointerNV, .-glVertexAttribPointerNV

	.p2align	4,,15
	.globl	glVertexAttrib1dARB
	.type	glVertexAttrib1dARB, @function
glVertexAttrib1dARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4824(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4824(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4824(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4824(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib1dARB, .-glVertexAttrib1dARB

	.p2align	4,,15
	.globl	glVertexAttrib1dvARB
	.type	glVertexAttrib1dvARB, @function
glVertexAttrib1dvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4832(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4832(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4832(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4832(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib1dvARB, .-glVertexAttrib1dvARB

	.p2align	4,,15
	.globl	glVertexAttrib1fARB
	.type	glVertexAttrib1fARB, @function
glVertexAttrib1fARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4840(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4840(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4840(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	4840(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib1fARB, .-glVertexAttrib1fARB

	.p2align	4,,15
	.globl	glVertexAttrib1fvARB
	.type	glVertexAttrib1fvARB, @function
glVertexAttrib1fvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4848(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
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
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4848(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib1fvARB, .-glVertexAttrib1fvARB

	.p2align	4,,15
	.globl	glVertexAttrib1sARB
	.type	glVertexAttrib1sARB, @function
glVertexAttrib1sARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4856(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4856(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib1sARB, .-glVertexAttrib1sARB

	.p2align	4,,15
	.globl	glVertexAttrib1svARB
	.type	glVertexAttrib1svARB, @function
glVertexAttrib1svARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4864(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4864(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4864(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4864(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib1svARB, .-glVertexAttrib1svARB

	.p2align	4,,15
	.globl	glVertexAttrib2dARB
	.type	glVertexAttrib2dARB, @function
glVertexAttrib2dARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4872(%rax), %r11
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
	movq	4872(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4872(%rax), %r11
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
	movq	4872(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib2dARB, .-glVertexAttrib2dARB

	.p2align	4,,15
	.globl	glVertexAttrib2dvARB
	.type	glVertexAttrib2dvARB, @function
glVertexAttrib2dvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4880(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4880(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib2dvARB, .-glVertexAttrib2dvARB

	.p2align	4,,15
	.globl	glVertexAttrib2fARB
	.type	glVertexAttrib2fARB, @function
glVertexAttrib2fARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4888(%rax), %r11
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
	movq	4888(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4888(%rax), %r11
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
	movq	4888(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib2fARB, .-glVertexAttrib2fARB

	.p2align	4,,15
	.globl	glVertexAttrib2fvARB
	.type	glVertexAttrib2fvARB, @function
glVertexAttrib2fvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4896(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4896(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4896(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4896(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib2fvARB, .-glVertexAttrib2fvARB

	.p2align	4,,15
	.globl	glVertexAttrib2sARB
	.type	glVertexAttrib2sARB, @function
glVertexAttrib2sARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4904(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	4904(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib2sARB, .-glVertexAttrib2sARB

	.p2align	4,,15
	.globl	glVertexAttrib2svARB
	.type	glVertexAttrib2svARB, @function
glVertexAttrib2svARB:
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
	.size	glVertexAttrib2svARB, .-glVertexAttrib2svARB

	.p2align	4,,15
	.globl	glVertexAttrib3dARB
	.type	glVertexAttrib3dARB, @function
glVertexAttrib3dARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4920(%rax), %r11
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
	movq	4920(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4920(%rax), %r11
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
	movq	4920(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib3dARB, .-glVertexAttrib3dARB

	.p2align	4,,15
	.globl	glVertexAttrib3dvARB
	.type	glVertexAttrib3dvARB, @function
glVertexAttrib3dvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4928(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4928(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib3dvARB, .-glVertexAttrib3dvARB

	.p2align	4,,15
	.globl	glVertexAttrib3fARB
	.type	glVertexAttrib3fARB, @function
glVertexAttrib3fARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4936(%rax), %r11
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
	movq	4936(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4936(%rax), %r11
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
	movq	4936(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib3fARB, .-glVertexAttrib3fARB

	.p2align	4,,15
	.globl	glVertexAttrib3fvARB
	.type	glVertexAttrib3fvARB, @function
glVertexAttrib3fvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4944(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4944(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib3fvARB, .-glVertexAttrib3fvARB

	.p2align	4,,15
	.globl	glVertexAttrib3sARB
	.type	glVertexAttrib3sARB, @function
glVertexAttrib3sARB:
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
	.size	glVertexAttrib3sARB, .-glVertexAttrib3sARB

	.p2align	4,,15
	.globl	glVertexAttrib3svARB
	.type	glVertexAttrib3svARB, @function
glVertexAttrib3svARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4960(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4960(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib3svARB, .-glVertexAttrib3svARB

	.p2align	4,,15
	.globl	glVertexAttrib4dARB
	.type	glVertexAttrib4dARB, @function
glVertexAttrib4dARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4968(%rax), %r11
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
	movq	4968(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4968(%rax), %r11
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
	movq	4968(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4dARB, .-glVertexAttrib4dARB

	.p2align	4,,15
	.globl	glVertexAttrib4dvARB
	.type	glVertexAttrib4dvARB, @function
glVertexAttrib4dvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4976(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4976(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4dvARB, .-glVertexAttrib4dvARB

	.p2align	4,,15
	.globl	glVertexAttrib4fARB
	.type	glVertexAttrib4fARB, @function
glVertexAttrib4fARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4984(%rax), %r11
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
	movq	4984(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4984(%rax), %r11
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
	movq	4984(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4fARB, .-glVertexAttrib4fARB

	.p2align	4,,15
	.globl	glVertexAttrib4fvARB
	.type	glVertexAttrib4fvARB, @function
glVertexAttrib4fvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	4992(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4992(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	4992(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	4992(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4fvARB, .-glVertexAttrib4fvARB

	.p2align	4,,15
	.globl	glVertexAttrib4sARB
	.type	glVertexAttrib4sARB, @function
glVertexAttrib4sARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5000(%rax), %r11
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
	movq	5000(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4sARB, .-glVertexAttrib4sARB

	.p2align	4,,15
	.globl	glVertexAttrib4svARB
	.type	glVertexAttrib4svARB, @function
glVertexAttrib4svARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5008(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5008(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5008(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5008(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4svARB, .-glVertexAttrib4svARB

	.p2align	4,,15
	.globl	glVertexAttrib4NubARB
	.type	glVertexAttrib4NubARB, @function
glVertexAttrib4NubARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5016(%rax), %r11
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
	movq	5016(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4NubARB, .-glVertexAttrib4NubARB

	.p2align	4,,15
	.globl	glVertexAttrib4NubvARB
	.type	glVertexAttrib4NubvARB, @function
glVertexAttrib4NubvARB:
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
	.size	glVertexAttrib4NubvARB, .-glVertexAttrib4NubvARB

	.p2align	4,,15
	.globl	glVertexAttribs1dvNV
	.type	glVertexAttribs1dvNV, @function
glVertexAttribs1dvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5032(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5032(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttribs1dvNV, .-glVertexAttribs1dvNV

	.p2align	4,,15
	.globl	glVertexAttribs1fvNV
	.type	glVertexAttribs1fvNV, @function
glVertexAttribs1fvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5040(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5040(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttribs1fvNV, .-glVertexAttribs1fvNV

	.p2align	4,,15
	.globl	glVertexAttribs1svNV
	.type	glVertexAttribs1svNV, @function
glVertexAttribs1svNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5048(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5048(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttribs1svNV, .-glVertexAttribs1svNV

	.p2align	4,,15
	.globl	glVertexAttribs2dvNV
	.type	glVertexAttribs2dvNV, @function
glVertexAttribs2dvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5056(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5056(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5056(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5056(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttribs2dvNV, .-glVertexAttribs2dvNV

	.p2align	4,,15
	.globl	glVertexAttribs2fvNV
	.type	glVertexAttribs2fvNV, @function
glVertexAttribs2fvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5064(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5064(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttribs2fvNV, .-glVertexAttribs2fvNV

	.p2align	4,,15
	.globl	glVertexAttribs2svNV
	.type	glVertexAttribs2svNV, @function
glVertexAttribs2svNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5072(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5072(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5072(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5072(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttribs2svNV, .-glVertexAttribs2svNV

	.p2align	4,,15
	.globl	glVertexAttribs3dvNV
	.type	glVertexAttribs3dvNV, @function
glVertexAttribs3dvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5080(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5080(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttribs3dvNV, .-glVertexAttribs3dvNV

	.p2align	4,,15
	.globl	glVertexAttribs3fvNV
	.type	glVertexAttribs3fvNV, @function
glVertexAttribs3fvNV:
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
	.size	glVertexAttribs3fvNV, .-glVertexAttribs3fvNV

	.p2align	4,,15
	.globl	glVertexAttribs3svNV
	.type	glVertexAttribs3svNV, @function
glVertexAttribs3svNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5096(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5096(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttribs3svNV, .-glVertexAttribs3svNV

	.p2align	4,,15
	.globl	glVertexAttribs4dvNV
	.type	glVertexAttribs4dvNV, @function
glVertexAttribs4dvNV:
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
	.size	glVertexAttribs4dvNV, .-glVertexAttribs4dvNV

	.p2align	4,,15
	.globl	glVertexAttribs4fvNV
	.type	glVertexAttribs4fvNV, @function
glVertexAttribs4fvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5112(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5112(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttribs4fvNV, .-glVertexAttribs4fvNV

	.p2align	4,,15
	.globl	glVertexAttribs4svNV
	.type	glVertexAttribs4svNV, @function
glVertexAttribs4svNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5120(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5120(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5120(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5120(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttribs4svNV, .-glVertexAttribs4svNV

	.p2align	4,,15
	.globl	glVertexAttribs4ubvNV
	.type	glVertexAttribs4ubvNV, @function
glVertexAttribs4ubvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5128(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5128(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttribs4ubvNV, .-glVertexAttribs4ubvNV

	.p2align	4,,15
	.globl	glPointParameteriNV
	.type	glPointParameteriNV, @function
glPointParameteriNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5136(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5136(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5136(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5136(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glPointParameteriNV, .-glPointParameteriNV

	.p2align	4,,15
	.globl	glPointParameterivNV
	.type	glPointParameterivNV, @function
glPointParameterivNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5144(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
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
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5144(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glPointParameterivNV, .-glPointParameterivNV

	.p2align	4,,15
	.globl	glMultiDrawArraysEXT
	.type	glMultiDrawArraysEXT, @function
glMultiDrawArraysEXT:
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
	.size	glMultiDrawArraysEXT, .-glMultiDrawArraysEXT

	.p2align	4,,15
	.globl	glMultiDrawElementsEXT
	.type	glMultiDrawElementsEXT, @function
glMultiDrawElementsEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5160(%rax), %r11
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
	movq	5160(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glMultiDrawElementsEXT, .-glMultiDrawElementsEXT

	.p2align	4,,15
	.globl	glActiveStencilFaceEXT
	.type	glActiveStencilFaceEXT, @function
glActiveStencilFaceEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5168(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5168(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glActiveStencilFaceEXT, .-glActiveStencilFaceEXT

	.p2align	4,,15
	.globl	glDeleteFencesNV
	.type	glDeleteFencesNV, @function
glDeleteFencesNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5176(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
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
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5176(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glDeleteFencesNV, .-glDeleteFencesNV

	.p2align	4,,15
	.globl	glGenFencesNV
	.type	glGenFencesNV, @function
glGenFencesNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5184(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5184(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGenFencesNV, .-glGenFencesNV

	.p2align	4,,15
	.globl	glIsFenceNV
	.type	glIsFenceNV, @function
glIsFenceNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5192(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5192(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glIsFenceNV, .-glIsFenceNV

	.p2align	4,,15
	.globl	glTestFenceNV
	.type	glTestFenceNV, @function
glTestFenceNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5200(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5200(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glTestFenceNV, .-glTestFenceNV

	.p2align	4,,15
	.globl	glGetFenceivNV
	.type	glGetFenceivNV, @function
glGetFenceivNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5208(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5208(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetFenceivNV, .-glGetFenceivNV

	.p2align	4,,15
	.globl	glFinishFenceNV
	.type	glFinishFenceNV, @function
glFinishFenceNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5216(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5216(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFinishFenceNV, .-glFinishFenceNV

	.p2align	4,,15
	.globl	glSetFenceNV
	.type	glSetFenceNV, @function
glSetFenceNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5224(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5224(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSetFenceNV, .-glSetFenceNV

	.p2align	4,,15
	.globl	glVertexAttrib4bvARB
	.type	glVertexAttrib4bvARB, @function
glVertexAttrib4bvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5232(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
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
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5232(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4bvARB, .-glVertexAttrib4bvARB

	.p2align	4,,15
	.globl	glVertexAttrib4ivARB
	.type	glVertexAttrib4ivARB, @function
glVertexAttrib4ivARB:
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
	.size	glVertexAttrib4ivARB, .-glVertexAttrib4ivARB

	.p2align	4,,15
	.globl	glVertexAttrib4ubvARB
	.type	glVertexAttrib4ubvARB, @function
glVertexAttrib4ubvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5248(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
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
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5248(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4ubvARB, .-glVertexAttrib4ubvARB

	.p2align	4,,15
	.globl	glVertexAttrib4usvARB
	.type	glVertexAttrib4usvARB, @function
glVertexAttrib4usvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5256(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5256(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4usvARB, .-glVertexAttrib4usvARB

	.p2align	4,,15
	.globl	glVertexAttrib4uivARB
	.type	glVertexAttrib4uivARB, @function
glVertexAttrib4uivARB:
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
	.size	glVertexAttrib4uivARB, .-glVertexAttrib4uivARB

	.p2align	4,,15
	.globl	glVertexAttrib4NbvARB
	.type	glVertexAttrib4NbvARB, @function
glVertexAttrib4NbvARB:
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
	.size	glVertexAttrib4NbvARB, .-glVertexAttrib4NbvARB

	.p2align	4,,15
	.globl	glVertexAttrib4NsvARB
	.type	glVertexAttrib4NsvARB, @function
glVertexAttrib4NsvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5280(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5280(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4NsvARB, .-glVertexAttrib4NsvARB

	.p2align	4,,15
	.globl	glVertexAttrib4NivARB
	.type	glVertexAttrib4NivARB, @function
glVertexAttrib4NivARB:
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
	.size	glVertexAttrib4NivARB, .-glVertexAttrib4NivARB

	.p2align	4,,15
	.globl	glVertexAttrib4NusvARB
	.type	glVertexAttrib4NusvARB, @function
glVertexAttrib4NusvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5296(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5296(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4NusvARB, .-glVertexAttrib4NusvARB

	.p2align	4,,15
	.globl	glVertexAttrib4NuivARB
	.type	glVertexAttrib4NuivARB, @function
glVertexAttrib4NuivARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5304(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5304(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4NuivARB, .-glVertexAttrib4NuivARB

	.p2align	4,,15
	.globl	glVertexAttribPointerARB
	.type	glVertexAttribPointerARB, @function
glVertexAttribPointerARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5312(%rax), %r11
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
	movq	5312(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttribPointerARB, .-glVertexAttribPointerARB

	.p2align	4,,15
	.globl	glEnableVertexAttribArrayARB
	.type	glEnableVertexAttribArrayARB, @function
glEnableVertexAttribArrayARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5320(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5320(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glEnableVertexAttribArrayARB, .-glEnableVertexAttribArrayARB

	.p2align	4,,15
	.globl	glDisableVertexAttribArrayARB
	.type	glDisableVertexAttribArrayARB, @function
glDisableVertexAttribArrayARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5328(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5328(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glDisableVertexAttribArrayARB, .-glDisableVertexAttribArrayARB

	.p2align	4,,15
	.globl	glProgramStringARB
	.type	glProgramStringARB, @function
glProgramStringARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5336(%rax), %r11
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
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5336(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramStringARB, .-glProgramStringARB

	.p2align	4,,15
	.globl	glProgramEnvParameter4dARB
	.type	glProgramEnvParameter4dARB, @function
glProgramEnvParameter4dARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5344(%rax), %r11
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
	movq	5344(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5344(%rax), %r11
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
	movq	5344(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramEnvParameter4dARB, .-glProgramEnvParameter4dARB

	.p2align	4,,15
	.globl	glProgramEnvParameter4dvARB
	.type	glProgramEnvParameter4dvARB, @function
glProgramEnvParameter4dvARB:
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
	.size	glProgramEnvParameter4dvARB, .-glProgramEnvParameter4dvARB

	.p2align	4,,15
	.globl	glProgramEnvParameter4fARB
	.type	glProgramEnvParameter4fARB, @function
glProgramEnvParameter4fARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5360(%rax), %r11
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
	movq	5360(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5360(%rax), %r11
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
	movq	5360(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramEnvParameter4fARB, .-glProgramEnvParameter4fARB

	.p2align	4,,15
	.globl	glProgramEnvParameter4fvARB
	.type	glProgramEnvParameter4fvARB, @function
glProgramEnvParameter4fvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5368(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5368(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramEnvParameter4fvARB, .-glProgramEnvParameter4fvARB

	.p2align	4,,15
	.globl	glProgramLocalParameter4dARB
	.type	glProgramLocalParameter4dARB, @function
glProgramLocalParameter4dARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5376(%rax), %r11
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
	movq	5376(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5376(%rax), %r11
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
	movq	5376(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramLocalParameter4dARB, .-glProgramLocalParameter4dARB

	.p2align	4,,15
	.globl	glProgramLocalParameter4dvARB
	.type	glProgramLocalParameter4dvARB, @function
glProgramLocalParameter4dvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5384(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5384(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramLocalParameter4dvARB, .-glProgramLocalParameter4dvARB

	.p2align	4,,15
	.globl	glProgramLocalParameter4fARB
	.type	glProgramLocalParameter4fARB, @function
glProgramLocalParameter4fARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5392(%rax), %r11
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
	movq	5392(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5392(%rax), %r11
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
	movq	5392(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramLocalParameter4fARB, .-glProgramLocalParameter4fARB

	.p2align	4,,15
	.globl	glProgramLocalParameter4fvARB
	.type	glProgramLocalParameter4fvARB, @function
glProgramLocalParameter4fvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5400(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5400(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramLocalParameter4fvARB, .-glProgramLocalParameter4fvARB

	.p2align	4,,15
	.globl	glGetProgramEnvParameterdvARB
	.type	glGetProgramEnvParameterdvARB, @function
glGetProgramEnvParameterdvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5408(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5408(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetProgramEnvParameterdvARB, .-glGetProgramEnvParameterdvARB

	.p2align	4,,15
	.globl	glGetProgramEnvParameterfvARB
	.type	glGetProgramEnvParameterfvARB, @function
glGetProgramEnvParameterfvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5416(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5416(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5416(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5416(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetProgramEnvParameterfvARB, .-glGetProgramEnvParameterfvARB

	.p2align	4,,15
	.globl	glGetProgramLocalParameterdvARB
	.type	glGetProgramLocalParameterdvARB, @function
glGetProgramLocalParameterdvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5424(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5424(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetProgramLocalParameterdvARB, .-glGetProgramLocalParameterdvARB

	.p2align	4,,15
	.globl	glGetProgramLocalParameterfvARB
	.type	glGetProgramLocalParameterfvARB, @function
glGetProgramLocalParameterfvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5432(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5432(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5432(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5432(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetProgramLocalParameterfvARB, .-glGetProgramLocalParameterfvARB

	.p2align	4,,15
	.globl	glGetProgramivARB
	.type	glGetProgramivARB, @function
glGetProgramivARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5440(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5440(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetProgramivARB, .-glGetProgramivARB

	.p2align	4,,15
	.globl	glGetProgramStringARB
	.type	glGetProgramStringARB, @function
glGetProgramStringARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5448(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5448(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetProgramStringARB, .-glGetProgramStringARB

	.p2align	4,,15
	.globl	glProgramNamedParameter4fNV
	.type	glProgramNamedParameter4fNV, @function
glProgramNamedParameter4fNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5456(%rax), %r11
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
	movq	5456(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5456(%rax), %r11
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
	movq	5456(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramNamedParameter4fNV, .-glProgramNamedParameter4fNV

	.p2align	4,,15
	.globl	glProgramNamedParameter4dNV
	.type	glProgramNamedParameter4dNV, @function
glProgramNamedParameter4dNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5464(%rax), %r11
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
	movq	5464(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5464(%rax), %r11
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
	movq	5464(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramNamedParameter4dNV, .-glProgramNamedParameter4dNV

	.p2align	4,,15
	.globl	glProgramNamedParameter4fvNV
	.type	glProgramNamedParameter4fvNV, @function
glProgramNamedParameter4fvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5472(%rax), %r11
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
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5472(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramNamedParameter4fvNV, .-glProgramNamedParameter4fvNV

	.p2align	4,,15
	.globl	glProgramNamedParameter4dvNV
	.type	glProgramNamedParameter4dvNV, @function
glProgramNamedParameter4dvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5480(%rax), %r11
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
	movq	5480(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5480(%rax), %r11
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
	movq	5480(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glProgramNamedParameter4dvNV, .-glProgramNamedParameter4dvNV

	.p2align	4,,15
	.globl	glGetProgramNamedParameterfvNV
	.type	glGetProgramNamedParameterfvNV, @function
glGetProgramNamedParameterfvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5488(%rax), %r11
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
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5488(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetProgramNamedParameterfvNV, .-glGetProgramNamedParameterfvNV

	.p2align	4,,15
	.globl	glGetProgramNamedParameterdvNV
	.type	glGetProgramNamedParameterdvNV, @function
glGetProgramNamedParameterdvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5496(%rax), %r11
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
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5496(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetProgramNamedParameterdvNV, .-glGetProgramNamedParameterdvNV

	.p2align	4,,15
	.globl	glBindBufferARB
	.type	glBindBufferARB, @function
glBindBufferARB:
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
	.size	glBindBufferARB, .-glBindBufferARB

	.p2align	4,,15
	.globl	glBufferDataARB
	.type	glBufferDataARB, @function
glBufferDataARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5512(%rax), %r11
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
	movq	5512(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5512(%rax), %r11
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
	movq	5512(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glBufferDataARB, .-glBufferDataARB

	.p2align	4,,15
	.globl	glBufferSubDataARB
	.type	glBufferSubDataARB, @function
glBufferSubDataARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5520(%rax), %r11
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
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5520(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glBufferSubDataARB, .-glBufferSubDataARB

	.p2align	4,,15
	.globl	glDeleteBuffersARB
	.type	glDeleteBuffersARB, @function
glDeleteBuffersARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5528(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5528(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5528(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5528(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glDeleteBuffersARB, .-glDeleteBuffersARB

	.p2align	4,,15
	.globl	glGenBuffersARB
	.type	glGenBuffersARB, @function
glGenBuffersARB:
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
	.size	glGenBuffersARB, .-glGenBuffersARB

	.p2align	4,,15
	.globl	glGetBufferParameterivARB
	.type	glGetBufferParameterivARB, @function
glGetBufferParameterivARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5544(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5544(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetBufferParameterivARB, .-glGetBufferParameterivARB

	.p2align	4,,15
	.globl	glGetBufferPointervARB
	.type	glGetBufferPointervARB, @function
glGetBufferPointervARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5552(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5552(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetBufferPointervARB, .-glGetBufferPointervARB

	.p2align	4,,15
	.globl	glGetBufferSubDataARB
	.type	glGetBufferSubDataARB, @function
glGetBufferSubDataARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5560(%rax), %r11
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
	movq	5560(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5560(%rax), %r11
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
	movq	5560(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetBufferSubDataARB, .-glGetBufferSubDataARB

	.p2align	4,,15
	.globl	glIsBufferARB
	.type	glIsBufferARB, @function
glIsBufferARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5568(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5568(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glIsBufferARB, .-glIsBufferARB

	.p2align	4,,15
	.globl	glMapBufferARB
	.type	glMapBufferARB, @function
glMapBufferARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5576(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5576(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5576(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5576(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glMapBufferARB, .-glMapBufferARB

	.p2align	4,,15
	.globl	glUnmapBufferARB
	.type	glUnmapBufferARB, @function
glUnmapBufferARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5584(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5584(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUnmapBufferARB, .-glUnmapBufferARB

	.p2align	4,,15
	.globl	glDepthBoundsEXT
	.type	glDepthBoundsEXT, @function
glDepthBoundsEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5592(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5592(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glDepthBoundsEXT, .-glDepthBoundsEXT

	.p2align	4,,15
	.globl	glGenQueriesARB
	.type	glGenQueriesARB, @function
glGenQueriesARB:
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
	.size	glGenQueriesARB, .-glGenQueriesARB

	.p2align	4,,15
	.globl	glDeleteQueriesARB
	.type	glDeleteQueriesARB, @function
glDeleteQueriesARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5608(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5608(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glDeleteQueriesARB, .-glDeleteQueriesARB

	.p2align	4,,15
	.globl	glIsQueryARB
	.type	glIsQueryARB, @function
glIsQueryARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5616(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5616(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glIsQueryARB, .-glIsQueryARB

	.p2align	4,,15
	.globl	glBeginQueryARB
	.type	glBeginQueryARB, @function
glBeginQueryARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5624(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5624(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glBeginQueryARB, .-glBeginQueryARB

	.p2align	4,,15
	.globl	glEndQueryARB
	.type	glEndQueryARB, @function
glEndQueryARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5632(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5632(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glEndQueryARB, .-glEndQueryARB

	.p2align	4,,15
	.globl	glGetQueryivARB
	.type	glGetQueryivARB, @function
glGetQueryivARB:
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
	.size	glGetQueryivARB, .-glGetQueryivARB

	.p2align	4,,15
	.globl	glGetQueryObjectivARB
	.type	glGetQueryObjectivARB, @function
glGetQueryObjectivARB:
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
	.size	glGetQueryObjectivARB, .-glGetQueryObjectivARB

	.p2align	4,,15
	.globl	glGetQueryObjectuivARB
	.type	glGetQueryObjectuivARB, @function
glGetQueryObjectuivARB:
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
	.size	glGetQueryObjectuivARB, .-glGetQueryObjectuivARB

	.p2align	4,,15
	.globl	glMultiModeDrawArraysIBM
	.type	glMultiModeDrawArraysIBM, @function
glMultiModeDrawArraysIBM:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5664(%rax), %r11
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
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5664(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glMultiModeDrawArraysIBM, .-glMultiModeDrawArraysIBM

	.p2align	4,,15
	.globl	glMultiModeDrawElementsIBM
	.type	glMultiModeDrawElementsIBM, @function
glMultiModeDrawElementsIBM:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5672(%rax), %r11
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
	movq	5672(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glMultiModeDrawElementsIBM, .-glMultiModeDrawElementsIBM

	.p2align	4,,15
	.globl	glBlendEquationSeparateEXT
	.type	glBlendEquationSeparateEXT, @function
glBlendEquationSeparateEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5680(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5680(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glBlendEquationSeparateEXT, .-glBlendEquationSeparateEXT

	.p2align	4,,15
	.globl	glDeleteObjectARB
	.type	glDeleteObjectARB, @function
glDeleteObjectARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5688(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5688(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glDeleteObjectARB, .-glDeleteObjectARB

	.p2align	4,,15
	.globl	glGetHandleARB
	.type	glGetHandleARB, @function
glGetHandleARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5696(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5696(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetHandleARB, .-glGetHandleARB

	.p2align	4,,15
	.globl	glDetachObjectARB
	.type	glDetachObjectARB, @function
glDetachObjectARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5704(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5704(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glDetachObjectARB, .-glDetachObjectARB

	.p2align	4,,15
	.globl	glCreateShaderObjectARB
	.type	glCreateShaderObjectARB, @function
glCreateShaderObjectARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5712(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5712(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCreateShaderObjectARB, .-glCreateShaderObjectARB

	.p2align	4,,15
	.globl	glShaderSourceARB
	.type	glShaderSourceARB, @function
glShaderSourceARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5720(%rax), %r11
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
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5720(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glShaderSourceARB, .-glShaderSourceARB

	.p2align	4,,15
	.globl	glCompileShaderARB
	.type	glCompileShaderARB, @function
glCompileShaderARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5728(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5728(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCompileShaderARB, .-glCompileShaderARB

	.p2align	4,,15
	.globl	glCreateProgramObjectARB
	.type	glCreateProgramObjectARB, @function
glCreateProgramObjectARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5736(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	5736(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5736(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	5736(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCreateProgramObjectARB, .-glCreateProgramObjectARB

	.p2align	4,,15
	.globl	glAttachObjectARB
	.type	glAttachObjectARB, @function
glAttachObjectARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5744(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5744(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glAttachObjectARB, .-glAttachObjectARB

	.p2align	4,,15
	.globl	glLinkProgramARB
	.type	glLinkProgramARB, @function
glLinkProgramARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5752(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5752(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glLinkProgramARB, .-glLinkProgramARB

	.p2align	4,,15
	.globl	glUseProgramObjectARB
	.type	glUseProgramObjectARB, @function
glUseProgramObjectARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5760(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	5760(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5760(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	5760(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUseProgramObjectARB, .-glUseProgramObjectARB

	.p2align	4,,15
	.globl	glValidateProgramARB
	.type	glValidateProgramARB, @function
glValidateProgramARB:
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
	.size	glValidateProgramARB, .-glValidateProgramARB

	.p2align	4,,15
	.globl	glUniform1fARB
	.type	glUniform1fARB, @function
glUniform1fARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5776(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	5776(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5776(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	5776(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform1fARB, .-glUniform1fARB

	.p2align	4,,15
	.globl	glUniform2fARB
	.type	glUniform2fARB, @function
glUniform2fARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5784(%rax), %r11
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
	movq	5784(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5784(%rax), %r11
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
	movq	5784(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform2fARB, .-glUniform2fARB

	.p2align	4,,15
	.globl	glUniform3fARB
	.type	glUniform3fARB, @function
glUniform3fARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5792(%rax), %r11
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
	movq	5792(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5792(%rax), %r11
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
	movq	5792(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform3fARB, .-glUniform3fARB

	.p2align	4,,15
	.globl	glUniform4fARB
	.type	glUniform4fARB, @function
glUniform4fARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5800(%rax), %r11
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
	movq	5800(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5800(%rax), %r11
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
	movq	5800(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform4fARB, .-glUniform4fARB

	.p2align	4,,15
	.globl	glUniform1iARB
	.type	glUniform1iARB, @function
glUniform1iARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5808(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5808(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5808(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	5808(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform1iARB, .-glUniform1iARB

	.p2align	4,,15
	.globl	glUniform2iARB
	.type	glUniform2iARB, @function
glUniform2iARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5816(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5816(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform2iARB, .-glUniform2iARB

	.p2align	4,,15
	.globl	glUniform3iARB
	.type	glUniform3iARB, @function
glUniform3iARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5824(%rax), %r11
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
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5824(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform3iARB, .-glUniform3iARB

	.p2align	4,,15
	.globl	glUniform4iARB
	.type	glUniform4iARB, @function
glUniform4iARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5832(%rax), %r11
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
	pushq	%rcx
	pushq	%r8
	call	_glapi_get_dispatch
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5832(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform4iARB, .-glUniform4iARB

	.p2align	4,,15
	.globl	glUniform1fvARB
	.type	glUniform1fvARB, @function
glUniform1fvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5840(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5840(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform1fvARB, .-glUniform1fvARB

	.p2align	4,,15
	.globl	glUniform2fvARB
	.type	glUniform2fvARB, @function
glUniform2fvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5848(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5848(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform2fvARB, .-glUniform2fvARB

	.p2align	4,,15
	.globl	glUniform3fvARB
	.type	glUniform3fvARB, @function
glUniform3fvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5856(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5856(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform3fvARB, .-glUniform3fvARB

	.p2align	4,,15
	.globl	glUniform4fvARB
	.type	glUniform4fvARB, @function
glUniform4fvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5864(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5864(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform4fvARB, .-glUniform4fvARB

	.p2align	4,,15
	.globl	glUniform1ivARB
	.type	glUniform1ivARB, @function
glUniform1ivARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5872(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5872(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform1ivARB, .-glUniform1ivARB

	.p2align	4,,15
	.globl	glUniform2ivARB
	.type	glUniform2ivARB, @function
glUniform2ivARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5880(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5880(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform2ivARB, .-glUniform2ivARB

	.p2align	4,,15
	.globl	glUniform3ivARB
	.type	glUniform3ivARB, @function
glUniform3ivARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5888(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5888(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform3ivARB, .-glUniform3ivARB

	.p2align	4,,15
	.globl	glUniform4ivARB
	.type	glUniform4ivARB, @function
glUniform4ivARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5896(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5896(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniform4ivARB, .-glUniform4ivARB

	.p2align	4,,15
	.globl	glUniformMatrix2fvARB
	.type	glUniformMatrix2fvARB, @function
glUniformMatrix2fvARB:
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
	.size	glUniformMatrix2fvARB, .-glUniformMatrix2fvARB

	.p2align	4,,15
	.globl	glUniformMatrix3fvARB
	.type	glUniformMatrix3fvARB, @function
glUniformMatrix3fvARB:
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
	.size	glUniformMatrix3fvARB, .-glUniformMatrix3fvARB

	.p2align	4,,15
	.globl	glUniformMatrix4fvARB
	.type	glUniformMatrix4fvARB, @function
glUniformMatrix4fvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5920(%rax), %r11
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
	movq	5920(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5920(%rax), %r11
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
	movq	5920(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glUniformMatrix4fvARB, .-glUniformMatrix4fvARB

	.p2align	4,,15
	.globl	glGetObjectParameterfvARB
	.type	glGetObjectParameterfvARB, @function
glGetObjectParameterfvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5928(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5928(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetObjectParameterfvARB, .-glGetObjectParameterfvARB

	.p2align	4,,15
	.globl	glGetObjectParameterivARB
	.type	glGetObjectParameterivARB, @function
glGetObjectParameterivARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5936(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5936(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	5936(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5936(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetObjectParameterivARB, .-glGetObjectParameterivARB

	.p2align	4,,15
	.globl	glGetInfoLogARB
	.type	glGetInfoLogARB, @function
glGetInfoLogARB:
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
	.size	glGetInfoLogARB, .-glGetInfoLogARB

	.p2align	4,,15
	.globl	glGetAttachedObjectsARB
	.type	glGetAttachedObjectsARB, @function
glGetAttachedObjectsARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5952(%rax), %r11
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
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5952(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetAttachedObjectsARB, .-glGetAttachedObjectsARB

	.p2align	4,,15
	.globl	glGetUniformLocationARB
	.type	glGetUniformLocationARB, @function
glGetUniformLocationARB:
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
	.size	glGetUniformLocationARB, .-glGetUniformLocationARB

	.p2align	4,,15
	.globl	glGetActiveUniformARB
	.type	glGetActiveUniformARB, @function
glGetActiveUniformARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5968(%rax), %r11
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
	movq	5968(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetActiveUniformARB, .-glGetActiveUniformARB

	.p2align	4,,15
	.globl	glGetUniformfvARB
	.type	glGetUniformfvARB, @function
glGetUniformfvARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5976(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5976(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetUniformfvARB, .-glGetUniformfvARB

	.p2align	4,,15
	.globl	glGetUniformivARB
	.type	glGetUniformivARB, @function
glGetUniformivARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5984(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
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
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5984(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetUniformivARB, .-glGetUniformivARB

	.p2align	4,,15
	.globl	glGetShaderSourceARB
	.type	glGetShaderSourceARB, @function
glGetShaderSourceARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	5992(%rax), %r11
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
	pushq	%rdx
	pushq	%rcx
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	5992(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetShaderSourceARB, .-glGetShaderSourceARB

	.p2align	4,,15
	.globl	glBindAttribLocationARB
	.type	glBindAttribLocationARB, @function
glBindAttribLocationARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6000(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6000(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glBindAttribLocationARB, .-glBindAttribLocationARB

	.p2align	4,,15
	.globl	glGetActiveAttribARB
	.type	glGetActiveAttribARB, @function
glGetActiveAttribARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6008(%rax), %r11
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
	movq	6008(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetActiveAttribARB, .-glGetActiveAttribARB

	.p2align	4,,15
	.globl	glGetAttribLocationARB
	.type	glGetAttribLocationARB, @function
glGetAttribLocationARB:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6016(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6016(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetAttribLocationARB, .-glGetAttribLocationARB

	.p2align	4,,15
	.globl	glGetVertexAttribdvNV
	.type	glGetVertexAttribdvNV, @function
glGetVertexAttribdvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6024(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6024(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetVertexAttribdvNV, .-glGetVertexAttribdvNV

	.p2align	4,,15
	.globl	glGetVertexAttribfvNV
	.type	glGetVertexAttribfvNV, @function
glGetVertexAttribfvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6032(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6032(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetVertexAttribfvNV, .-glGetVertexAttribfvNV

	.p2align	4,,15
	.globl	glGetVertexAttribivNV
	.type	glGetVertexAttribivNV, @function
glGetVertexAttribivNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6040(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
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
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6040(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetVertexAttribivNV, .-glGetVertexAttribivNV

	.p2align	4,,15
	.globl	glVertexAttrib1dNV
	.type	glVertexAttrib1dNV, @function
glVertexAttrib1dNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6048(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	6048(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6048(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	6048(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib1dNV, .-glVertexAttrib1dNV

	.p2align	4,,15
	.globl	glVertexAttrib1dvNV
	.type	glVertexAttrib1dvNV, @function
glVertexAttrib1dvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6056(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
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
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6056(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib1dvNV, .-glVertexAttrib1dvNV

	.p2align	4,,15
	.globl	glVertexAttrib1fNV
	.type	glVertexAttrib1fNV, @function
glVertexAttrib1fNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6064(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_x86_64_get_dispatch@PLT
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	6064(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6064(%rax), %r11
	jmp	*%r11
1:
	subq	$24, %rsp
	movq	%rdi, (%rsp)
	movq	%xmm0, 8(%rsp)
	call	_glapi_get_dispatch
	movq	8(%rsp), %xmm0
	movq	(%rsp), %rdi
	addq	$24, %rsp
	movq	6064(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib1fNV, .-glVertexAttrib1fNV

	.p2align	4,,15
	.globl	glVertexAttrib1fvNV
	.type	glVertexAttrib1fvNV, @function
glVertexAttrib1fvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6072(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6072(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib1fvNV, .-glVertexAttrib1fvNV

	.p2align	4,,15
	.globl	glVertexAttrib1sNV
	.type	glVertexAttrib1sNV, @function
glVertexAttrib1sNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6080(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
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
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6080(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib1sNV, .-glVertexAttrib1sNV

	.p2align	4,,15
	.globl	glVertexAttrib1svNV
	.type	glVertexAttrib1svNV, @function
glVertexAttrib1svNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6088(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
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
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6088(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib1svNV, .-glVertexAttrib1svNV

	.p2align	4,,15
	.globl	glVertexAttrib2dNV
	.type	glVertexAttrib2dNV, @function
glVertexAttrib2dNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6096(%rax), %r11
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
	movq	6096(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6096(%rax), %r11
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
	movq	6096(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib2dNV, .-glVertexAttrib2dNV

	.p2align	4,,15
	.globl	glVertexAttrib2dvNV
	.type	glVertexAttrib2dvNV, @function
glVertexAttrib2dvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6104(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6104(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib2dvNV, .-glVertexAttrib2dvNV

	.p2align	4,,15
	.globl	glVertexAttrib2fNV
	.type	glVertexAttrib2fNV, @function
glVertexAttrib2fNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6112(%rax), %r11
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
	movq	6112(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6112(%rax), %r11
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
	movq	6112(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib2fNV, .-glVertexAttrib2fNV

	.p2align	4,,15
	.globl	glVertexAttrib2fvNV
	.type	glVertexAttrib2fvNV, @function
glVertexAttrib2fvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6120(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6120(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib2fvNV, .-glVertexAttrib2fvNV

	.p2align	4,,15
	.globl	glVertexAttrib2sNV
	.type	glVertexAttrib2sNV, @function
glVertexAttrib2sNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6128(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
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
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6128(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib2sNV, .-glVertexAttrib2sNV

	.p2align	4,,15
	.globl	glVertexAttrib2svNV
	.type	glVertexAttrib2svNV, @function
glVertexAttrib2svNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6136(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6136(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib2svNV, .-glVertexAttrib2svNV

	.p2align	4,,15
	.globl	glVertexAttrib3dNV
	.type	glVertexAttrib3dNV, @function
glVertexAttrib3dNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6144(%rax), %r11
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
	movq	6144(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6144(%rax), %r11
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
	movq	6144(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib3dNV, .-glVertexAttrib3dNV

	.p2align	4,,15
	.globl	glVertexAttrib3dvNV
	.type	glVertexAttrib3dvNV, @function
glVertexAttrib3dvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6152(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
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
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6152(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib3dvNV, .-glVertexAttrib3dvNV

	.p2align	4,,15
	.globl	glVertexAttrib3fNV
	.type	glVertexAttrib3fNV, @function
glVertexAttrib3fNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6160(%rax), %r11
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
	movq	6160(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6160(%rax), %r11
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
	movq	6160(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib3fNV, .-glVertexAttrib3fNV

	.p2align	4,,15
	.globl	glVertexAttrib3fvNV
	.type	glVertexAttrib3fvNV, @function
glVertexAttrib3fvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6168(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6168(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6168(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6168(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib3fvNV, .-glVertexAttrib3fvNV

	.p2align	4,,15
	.globl	glVertexAttrib3sNV
	.type	glVertexAttrib3sNV, @function
glVertexAttrib3sNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6176(%rax), %r11
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
	movq	6176(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6176(%rax), %r11
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
	movq	6176(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib3sNV, .-glVertexAttrib3sNV

	.p2align	4,,15
	.globl	glVertexAttrib3svNV
	.type	glVertexAttrib3svNV, @function
glVertexAttrib3svNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6184(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6184(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6184(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6184(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib3svNV, .-glVertexAttrib3svNV

	.p2align	4,,15
	.globl	glVertexAttrib4dNV
	.type	glVertexAttrib4dNV, @function
glVertexAttrib4dNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6192(%rax), %r11
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
	movq	6192(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6192(%rax), %r11
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
	movq	6192(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4dNV, .-glVertexAttrib4dNV

	.p2align	4,,15
	.globl	glVertexAttrib4dvNV
	.type	glVertexAttrib4dvNV, @function
glVertexAttrib4dvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6200(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6200(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6200(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6200(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4dvNV, .-glVertexAttrib4dvNV

	.p2align	4,,15
	.globl	glVertexAttrib4fNV
	.type	glVertexAttrib4fNV, @function
glVertexAttrib4fNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6208(%rax), %r11
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
	movq	6208(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6208(%rax), %r11
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
	movq	6208(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4fNV, .-glVertexAttrib4fNV

	.p2align	4,,15
	.globl	glVertexAttrib4fvNV
	.type	glVertexAttrib4fvNV, @function
glVertexAttrib4fvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6216(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6216(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6216(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6216(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4fvNV, .-glVertexAttrib4fvNV

	.p2align	4,,15
	.globl	glVertexAttrib4sNV
	.type	glVertexAttrib4sNV, @function
glVertexAttrib4sNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6224(%rax), %r11
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
	movq	6224(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6224(%rax), %r11
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
	movq	6224(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4sNV, .-glVertexAttrib4sNV

	.p2align	4,,15
	.globl	glVertexAttrib4svNV
	.type	glVertexAttrib4svNV, @function
glVertexAttrib4svNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6232(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6232(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6232(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6232(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4svNV, .-glVertexAttrib4svNV

	.p2align	4,,15
	.globl	glVertexAttrib4ubNV
	.type	glVertexAttrib4ubNV, @function
glVertexAttrib4ubNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6240(%rax), %r11
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
	movq	6240(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6240(%rax), %r11
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
	movq	6240(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4ubNV, .-glVertexAttrib4ubNV

	.p2align	4,,15
	.globl	glVertexAttrib4ubvNV
	.type	glVertexAttrib4ubvNV, @function
glVertexAttrib4ubvNV:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6248(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6248(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6248(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6248(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glVertexAttrib4ubvNV, .-glVertexAttrib4ubvNV

	.p2align	4,,15
	.globl	glGenFragmentShadersATI
	.type	glGenFragmentShadersATI, @function
glGenFragmentShadersATI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6256(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	6256(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6256(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	6256(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGenFragmentShadersATI, .-glGenFragmentShadersATI

	.p2align	4,,15
	.globl	glBindFragmentShaderATI
	.type	glBindFragmentShaderATI, @function
glBindFragmentShaderATI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6264(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	6264(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6264(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	6264(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glBindFragmentShaderATI, .-glBindFragmentShaderATI

	.p2align	4,,15
	.globl	glDeleteFragmentShaderATI
	.type	glDeleteFragmentShaderATI, @function
glDeleteFragmentShaderATI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6272(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	6272(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6272(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	6272(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glDeleteFragmentShaderATI, .-glDeleteFragmentShaderATI

	.p2align	4,,15
	.globl	glBeginFragmentShaderATI
	.type	glBeginFragmentShaderATI, @function
glBeginFragmentShaderATI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6280(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	6280(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6280(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	6280(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glBeginFragmentShaderATI, .-glBeginFragmentShaderATI

	.p2align	4,,15
	.globl	glEndFragmentShaderATI
	.type	glEndFragmentShaderATI, @function
glEndFragmentShaderATI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6288(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	movq	6288(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6288(%rax), %r11
	jmp	*%r11
1:
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	movq	6288(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glEndFragmentShaderATI, .-glEndFragmentShaderATI

	.p2align	4,,15
	.globl	glPassTexCoordATI
	.type	glPassTexCoordATI, @function
glPassTexCoordATI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6296(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6296(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6296(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6296(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glPassTexCoordATI, .-glPassTexCoordATI

	.p2align	4,,15
	.globl	glSampleMapATI
	.type	glSampleMapATI, @function
glSampleMapATI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6304(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6304(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6304(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6304(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSampleMapATI, .-glSampleMapATI

	.p2align	4,,15
	.globl	glColorFragmentOp1ATI
	.type	glColorFragmentOp1ATI, @function
glColorFragmentOp1ATI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6312(%rax), %r11
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
	movq	6312(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6312(%rax), %r11
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
	movq	6312(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glColorFragmentOp1ATI, .-glColorFragmentOp1ATI

	.p2align	4,,15
	.globl	glColorFragmentOp2ATI
	.type	glColorFragmentOp2ATI, @function
glColorFragmentOp2ATI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6320(%rax), %r11
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
	movq	6320(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6320(%rax), %r11
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
	movq	6320(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glColorFragmentOp2ATI, .-glColorFragmentOp2ATI

	.p2align	4,,15
	.globl	glColorFragmentOp3ATI
	.type	glColorFragmentOp3ATI, @function
glColorFragmentOp3ATI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6328(%rax), %r11
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
	movq	6328(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6328(%rax), %r11
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
	movq	6328(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glColorFragmentOp3ATI, .-glColorFragmentOp3ATI

	.p2align	4,,15
	.globl	glAlphaFragmentOp1ATI
	.type	glAlphaFragmentOp1ATI, @function
glAlphaFragmentOp1ATI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6336(%rax), %r11
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
	movq	6336(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6336(%rax), %r11
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
	movq	6336(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glAlphaFragmentOp1ATI, .-glAlphaFragmentOp1ATI

	.p2align	4,,15
	.globl	glAlphaFragmentOp2ATI
	.type	glAlphaFragmentOp2ATI, @function
glAlphaFragmentOp2ATI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6344(%rax), %r11
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
	movq	6344(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6344(%rax), %r11
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
	movq	6344(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glAlphaFragmentOp2ATI, .-glAlphaFragmentOp2ATI

	.p2align	4,,15
	.globl	glAlphaFragmentOp3ATI
	.type	glAlphaFragmentOp3ATI, @function
glAlphaFragmentOp3ATI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6352(%rax), %r11
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
	movq	6352(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6352(%rax), %r11
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
	movq	6352(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glAlphaFragmentOp3ATI, .-glAlphaFragmentOp3ATI

	.p2align	4,,15
	.globl	glSetFragmentShaderConstantATI
	.type	glSetFragmentShaderConstantATI, @function
glSetFragmentShaderConstantATI:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6360(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6360(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6360(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6360(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glSetFragmentShaderConstantATI, .-glSetFragmentShaderConstantATI

	.p2align	4,,15
	.globl	glIsRenderbufferEXT
	.type	glIsRenderbufferEXT, @function
glIsRenderbufferEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6368(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	6368(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6368(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	6368(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glIsRenderbufferEXT, .-glIsRenderbufferEXT

	.p2align	4,,15
	.globl	glBindRenderbufferEXT
	.type	glBindRenderbufferEXT, @function
glBindRenderbufferEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6376(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6376(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6376(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6376(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glBindRenderbufferEXT, .-glBindRenderbufferEXT

	.p2align	4,,15
	.globl	glDeleteRenderbuffersEXT
	.type	glDeleteRenderbuffersEXT, @function
glDeleteRenderbuffersEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6384(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6384(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6384(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6384(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glDeleteRenderbuffersEXT, .-glDeleteRenderbuffersEXT

	.p2align	4,,15
	.globl	glGenRenderbuffersEXT
	.type	glGenRenderbuffersEXT, @function
glGenRenderbuffersEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6392(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6392(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6392(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6392(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGenRenderbuffersEXT, .-glGenRenderbuffersEXT

	.p2align	4,,15
	.globl	glRenderbufferStorageEXT
	.type	glRenderbufferStorageEXT, @function
glRenderbufferStorageEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6400(%rax), %r11
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
	movq	6400(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6400(%rax), %r11
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
	movq	6400(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glRenderbufferStorageEXT, .-glRenderbufferStorageEXT

	.p2align	4,,15
	.globl	glGetRenderbufferParameterivEXT
	.type	glGetRenderbufferParameterivEXT, @function
glGetRenderbufferParameterivEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6408(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_x86_64_get_dispatch@PLT
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6408(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6408(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	call	_glapi_get_dispatch
	popq	%rdx
	popq	%rsi
	popq	%rdi
	movq	6408(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetRenderbufferParameterivEXT, .-glGetRenderbufferParameterivEXT

	.p2align	4,,15
	.globl	glIsFramebufferEXT
	.type	glIsFramebufferEXT, @function
glIsFramebufferEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6416(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	6416(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6416(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	6416(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glIsFramebufferEXT, .-glIsFramebufferEXT

	.p2align	4,,15
	.globl	glBindFramebufferEXT
	.type	glBindFramebufferEXT, @function
glBindFramebufferEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6424(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6424(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6424(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6424(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glBindFramebufferEXT, .-glBindFramebufferEXT

	.p2align	4,,15
	.globl	glDeleteFramebuffersEXT
	.type	glDeleteFramebuffersEXT, @function
glDeleteFramebuffersEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6432(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6432(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6432(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6432(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glDeleteFramebuffersEXT, .-glDeleteFramebuffersEXT

	.p2align	4,,15
	.globl	glGenFramebuffersEXT
	.type	glGenFramebuffersEXT, @function
glGenFramebuffersEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6440(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6440(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6440(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6440(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGenFramebuffersEXT, .-glGenFramebuffersEXT

	.p2align	4,,15
	.globl	glCheckFramebufferStatusEXT
	.type	glCheckFramebufferStatusEXT, @function
glCheckFramebufferStatusEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6448(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	6448(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6448(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	6448(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glCheckFramebufferStatusEXT, .-glCheckFramebufferStatusEXT

	.p2align	4,,15
	.globl	glFramebufferTexture1DEXT
	.type	glFramebufferTexture1DEXT, @function
glFramebufferTexture1DEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6456(%rax), %r11
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
	movq	6456(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6456(%rax), %r11
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
	movq	6456(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFramebufferTexture1DEXT, .-glFramebufferTexture1DEXT

	.p2align	4,,15
	.globl	glFramebufferTexture2DEXT
	.type	glFramebufferTexture2DEXT, @function
glFramebufferTexture2DEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6464(%rax), %r11
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
	movq	6464(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6464(%rax), %r11
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
	movq	6464(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFramebufferTexture2DEXT, .-glFramebufferTexture2DEXT

	.p2align	4,,15
	.globl	glFramebufferTexture3DEXT
	.type	glFramebufferTexture3DEXT, @function
glFramebufferTexture3DEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6472(%rax), %r11
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
	movq	6472(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6472(%rax), %r11
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
	movq	6472(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFramebufferTexture3DEXT, .-glFramebufferTexture3DEXT

	.p2align	4,,15
	.globl	glFramebufferRenderbufferEXT
	.type	glFramebufferRenderbufferEXT, @function
glFramebufferRenderbufferEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6480(%rax), %r11
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
	movq	6480(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6480(%rax), %r11
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
	movq	6480(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glFramebufferRenderbufferEXT, .-glFramebufferRenderbufferEXT

	.p2align	4,,15
	.globl	glGetFramebufferAttachmentParameterivEXT
	.type	glGetFramebufferAttachmentParameterivEXT, @function
glGetFramebufferAttachmentParameterivEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6488(%rax), %r11
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
	movq	6488(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6488(%rax), %r11
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
	movq	6488(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGetFramebufferAttachmentParameterivEXT, .-glGetFramebufferAttachmentParameterivEXT

	.p2align	4,,15
	.globl	glGenerateMipmapEXT
	.type	glGenerateMipmapEXT, @function
glGenerateMipmapEXT:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6496(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	call	_x86_64_get_dispatch@PLT
	popq	%rdi
	movq	6496(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6496(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	call	_glapi_get_dispatch
	popq	%rdi
	movq	6496(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glGenerateMipmapEXT, .-glGenerateMipmapEXT

	.p2align	4,,15
	.globl	glStencilFuncSeparate
	.type	glStencilFuncSeparate, @function
glStencilFuncSeparate:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6504(%rax), %r11
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
	movq	6504(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6504(%rax), %r11
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
	movq	6504(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glStencilFuncSeparate, .-glStencilFuncSeparate

	.p2align	4,,15
	.globl	glStencilOpSeparate
	.type	glStencilOpSeparate, @function
glStencilOpSeparate:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6512(%rax), %r11
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
	movq	6512(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6512(%rax), %r11
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
	movq	6512(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glStencilOpSeparate, .-glStencilOpSeparate

	.p2align	4,,15
	.globl	glStencilMaskSeparate
	.type	glStencilMaskSeparate, @function
glStencilMaskSeparate:
#if defined(GLX_USE_TLS)
	call	_x86_64_get_dispatch@PLT
	movq	6520(%rax), %r11
	jmp	*%r11
#elif defined(PTHREADS)
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_x86_64_get_dispatch@PLT
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6520(%rax), %r11
	jmp	*%r11
#else
	movq	_glapi_Dispatch(%rip), %rax
	testq	%rax, %rax
	je	1f
	movq	6520(%rax), %r11
	jmp	*%r11
1:
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	call	_glapi_get_dispatch
	popq	%rbp
	popq	%rsi
	popq	%rdi
	movq	6520(%rax), %r11
	jmp	*%r11
#endif /* defined(GLX_USE_TLS) */
	.size	glStencilMaskSeparate, .-glStencilMaskSeparate

	.globl glArrayElementEXT ; .set glArrayElementEXT, glArrayElement
	.globl glBindTextureEXT ; .set glBindTextureEXT, glBindTexture
	.globl glDrawArraysEXT ; .set glDrawArraysEXT, glDrawArrays
	.globl glCopyTexImage1DEXT ; .set glCopyTexImage1DEXT, glCopyTexImage1D
	.globl glCopyTexImage2DEXT ; .set glCopyTexImage2DEXT, glCopyTexImage2D
	.globl glCopyTexSubImage1DEXT ; .set glCopyTexSubImage1DEXT, glCopyTexSubImage1D
	.globl glCopyTexSubImage2DEXT ; .set glCopyTexSubImage2DEXT, glCopyTexSubImage2D
	.globl glDeleteTexturesEXT ; .set glDeleteTexturesEXT, glDeleteTextures
	.globl glGetPointervEXT ; .set glGetPointervEXT, glGetPointerv
	.globl glPrioritizeTexturesEXT ; .set glPrioritizeTexturesEXT, glPrioritizeTextures
	.globl glTexSubImage1DEXT ; .set glTexSubImage1DEXT, glTexSubImage1D
	.globl glTexSubImage2DEXT ; .set glTexSubImage2DEXT, glTexSubImage2D
	.globl glBlendColorEXT ; .set glBlendColorEXT, glBlendColor
	.globl glBlendEquationEXT ; .set glBlendEquationEXT, glBlendEquation
	.globl glDrawRangeElementsEXT ; .set glDrawRangeElementsEXT, glDrawRangeElements
	.globl glColorTableSGI ; .set glColorTableSGI, glColorTable
	.globl glColorTableEXT ; .set glColorTableEXT, glColorTable
	.globl glColorTableParameterfvSGI ; .set glColorTableParameterfvSGI, glColorTableParameterfv
	.globl glColorTableParameterivSGI ; .set glColorTableParameterivSGI, glColorTableParameteriv
	.globl glCopyColorTableSGI ; .set glCopyColorTableSGI, glCopyColorTable
	.globl glColorSubTableEXT ; .set glColorSubTableEXT, glColorSubTable
	.globl glCopyColorSubTableEXT ; .set glCopyColorSubTableEXT, glCopyColorSubTable
	.globl glConvolutionFilter1DEXT ; .set glConvolutionFilter1DEXT, glConvolutionFilter1D
	.globl glConvolutionFilter2DEXT ; .set glConvolutionFilter2DEXT, glConvolutionFilter2D
	.globl glConvolutionParameterfEXT ; .set glConvolutionParameterfEXT, glConvolutionParameterf
	.globl glConvolutionParameterfvEXT ; .set glConvolutionParameterfvEXT, glConvolutionParameterfv
	.globl glConvolutionParameteriEXT ; .set glConvolutionParameteriEXT, glConvolutionParameteri
	.globl glConvolutionParameterivEXT ; .set glConvolutionParameterivEXT, glConvolutionParameteriv
	.globl glCopyConvolutionFilter1DEXT ; .set glCopyConvolutionFilter1DEXT, glCopyConvolutionFilter1D
	.globl glCopyConvolutionFilter2DEXT ; .set glCopyConvolutionFilter2DEXT, glCopyConvolutionFilter2D
	.globl glSeparableFilter2DEXT ; .set glSeparableFilter2DEXT, glSeparableFilter2D
	.globl glHistogramEXT ; .set glHistogramEXT, glHistogram
	.globl glMinmaxEXT ; .set glMinmaxEXT, glMinmax
	.globl glResetHistogramEXT ; .set glResetHistogramEXT, glResetHistogram
	.globl glResetMinmaxEXT ; .set glResetMinmaxEXT, glResetMinmax
	.globl glTexImage3DEXT ; .set glTexImage3DEXT, glTexImage3D
	.globl glTexSubImage3DEXT ; .set glTexSubImage3DEXT, glTexSubImage3D
	.globl glCopyTexSubImage3DEXT ; .set glCopyTexSubImage3DEXT, glCopyTexSubImage3D
	.globl glActiveTexture ; .set glActiveTexture, glActiveTextureARB
	.globl glClientActiveTexture ; .set glClientActiveTexture, glClientActiveTextureARB
	.globl glMultiTexCoord1d ; .set glMultiTexCoord1d, glMultiTexCoord1dARB
	.globl glMultiTexCoord1dv ; .set glMultiTexCoord1dv, glMultiTexCoord1dvARB
	.globl glMultiTexCoord1f ; .set glMultiTexCoord1f, glMultiTexCoord1fARB
	.globl glMultiTexCoord1fv ; .set glMultiTexCoord1fv, glMultiTexCoord1fvARB
	.globl glMultiTexCoord1i ; .set glMultiTexCoord1i, glMultiTexCoord1iARB
	.globl glMultiTexCoord1iv ; .set glMultiTexCoord1iv, glMultiTexCoord1ivARB
	.globl glMultiTexCoord1s ; .set glMultiTexCoord1s, glMultiTexCoord1sARB
	.globl glMultiTexCoord1sv ; .set glMultiTexCoord1sv, glMultiTexCoord1svARB
	.globl glMultiTexCoord2d ; .set glMultiTexCoord2d, glMultiTexCoord2dARB
	.globl glMultiTexCoord2dv ; .set glMultiTexCoord2dv, glMultiTexCoord2dvARB
	.globl glMultiTexCoord2f ; .set glMultiTexCoord2f, glMultiTexCoord2fARB
	.globl glMultiTexCoord2fv ; .set glMultiTexCoord2fv, glMultiTexCoord2fvARB
	.globl glMultiTexCoord2i ; .set glMultiTexCoord2i, glMultiTexCoord2iARB
	.globl glMultiTexCoord2iv ; .set glMultiTexCoord2iv, glMultiTexCoord2ivARB
	.globl glMultiTexCoord2s ; .set glMultiTexCoord2s, glMultiTexCoord2sARB
	.globl glMultiTexCoord2sv ; .set glMultiTexCoord2sv, glMultiTexCoord2svARB
	.globl glMultiTexCoord3d ; .set glMultiTexCoord3d, glMultiTexCoord3dARB
	.globl glMultiTexCoord3dv ; .set glMultiTexCoord3dv, glMultiTexCoord3dvARB
	.globl glMultiTexCoord3f ; .set glMultiTexCoord3f, glMultiTexCoord3fARB
	.globl glMultiTexCoord3fv ; .set glMultiTexCoord3fv, glMultiTexCoord3fvARB
	.globl glMultiTexCoord3i ; .set glMultiTexCoord3i, glMultiTexCoord3iARB
	.globl glMultiTexCoord3iv ; .set glMultiTexCoord3iv, glMultiTexCoord3ivARB
	.globl glMultiTexCoord3s ; .set glMultiTexCoord3s, glMultiTexCoord3sARB
	.globl glMultiTexCoord3sv ; .set glMultiTexCoord3sv, glMultiTexCoord3svARB
	.globl glMultiTexCoord4d ; .set glMultiTexCoord4d, glMultiTexCoord4dARB
	.globl glMultiTexCoord4dv ; .set glMultiTexCoord4dv, glMultiTexCoord4dvARB
	.globl glMultiTexCoord4f ; .set glMultiTexCoord4f, glMultiTexCoord4fARB
	.globl glMultiTexCoord4fv ; .set glMultiTexCoord4fv, glMultiTexCoord4fvARB
	.globl glMultiTexCoord4i ; .set glMultiTexCoord4i, glMultiTexCoord4iARB
	.globl glMultiTexCoord4iv ; .set glMultiTexCoord4iv, glMultiTexCoord4ivARB
	.globl glMultiTexCoord4s ; .set glMultiTexCoord4s, glMultiTexCoord4sARB
	.globl glMultiTexCoord4sv ; .set glMultiTexCoord4sv, glMultiTexCoord4svARB
	.globl glLoadTransposeMatrixf ; .set glLoadTransposeMatrixf, glLoadTransposeMatrixfARB
	.globl glLoadTransposeMatrixd ; .set glLoadTransposeMatrixd, glLoadTransposeMatrixdARB
	.globl glMultTransposeMatrixf ; .set glMultTransposeMatrixf, glMultTransposeMatrixfARB
	.globl glMultTransposeMatrixd ; .set glMultTransposeMatrixd, glMultTransposeMatrixdARB
	.globl glSampleCoverage ; .set glSampleCoverage, glSampleCoverageARB
	.globl glDrawBuffersATI ; .set glDrawBuffersATI, glDrawBuffersARB
	.globl glSampleMaskEXT ; .set glSampleMaskEXT, glSampleMaskSGIS
	.globl glSamplePatternEXT ; .set glSamplePatternEXT, glSamplePatternSGIS
	.globl glPointParameterf ; .set glPointParameterf, glPointParameterfEXT
	.globl glPointParameterfARB ; .set glPointParameterfARB, glPointParameterfEXT
	.globl glPointParameterfSGIS ; .set glPointParameterfSGIS, glPointParameterfEXT
	.globl glPointParameterfv ; .set glPointParameterfv, glPointParameterfvEXT
	.globl glPointParameterfvARB ; .set glPointParameterfvARB, glPointParameterfvEXT
	.globl glPointParameterfvSGIS ; .set glPointParameterfvSGIS, glPointParameterfvEXT
	.globl glWindowPos2d ; .set glWindowPos2d, glWindowPos2dMESA
	.globl glWindowPos2dARB ; .set glWindowPos2dARB, glWindowPos2dMESA
	.globl glWindowPos2dv ; .set glWindowPos2dv, glWindowPos2dvMESA
	.globl glWindowPos2dvARB ; .set glWindowPos2dvARB, glWindowPos2dvMESA
	.globl glWindowPos2f ; .set glWindowPos2f, glWindowPos2fMESA
	.globl glWindowPos2fARB ; .set glWindowPos2fARB, glWindowPos2fMESA
	.globl glWindowPos2fv ; .set glWindowPos2fv, glWindowPos2fvMESA
	.globl glWindowPos2fvARB ; .set glWindowPos2fvARB, glWindowPos2fvMESA
	.globl glWindowPos2i ; .set glWindowPos2i, glWindowPos2iMESA
	.globl glWindowPos2iARB ; .set glWindowPos2iARB, glWindowPos2iMESA
	.globl glWindowPos2iv ; .set glWindowPos2iv, glWindowPos2ivMESA
	.globl glWindowPos2ivARB ; .set glWindowPos2ivARB, glWindowPos2ivMESA
	.globl glWindowPos2s ; .set glWindowPos2s, glWindowPos2sMESA
	.globl glWindowPos2sARB ; .set glWindowPos2sARB, glWindowPos2sMESA
	.globl glWindowPos2sv ; .set glWindowPos2sv, glWindowPos2svMESA
	.globl glWindowPos2svARB ; .set glWindowPos2svARB, glWindowPos2svMESA
	.globl glWindowPos3d ; .set glWindowPos3d, glWindowPos3dMESA
	.globl glWindowPos3dARB ; .set glWindowPos3dARB, glWindowPos3dMESA
	.globl glWindowPos3dv ; .set glWindowPos3dv, glWindowPos3dvMESA
	.globl glWindowPos3dvARB ; .set glWindowPos3dvARB, glWindowPos3dvMESA
	.globl glWindowPos3f ; .set glWindowPos3f, glWindowPos3fMESA
	.globl glWindowPos3fARB ; .set glWindowPos3fARB, glWindowPos3fMESA
	.globl glWindowPos3fv ; .set glWindowPos3fv, glWindowPos3fvMESA
	.globl glWindowPos3fvARB ; .set glWindowPos3fvARB, glWindowPos3fvMESA
	.globl glWindowPos3i ; .set glWindowPos3i, glWindowPos3iMESA
	.globl glWindowPos3iARB ; .set glWindowPos3iARB, glWindowPos3iMESA
	.globl glWindowPos3iv ; .set glWindowPos3iv, glWindowPos3ivMESA
	.globl glWindowPos3ivARB ; .set glWindowPos3ivARB, glWindowPos3ivMESA
	.globl glWindowPos3s ; .set glWindowPos3s, glWindowPos3sMESA
	.globl glWindowPos3sARB ; .set glWindowPos3sARB, glWindowPos3sMESA
	.globl glWindowPos3sv ; .set glWindowPos3sv, glWindowPos3svMESA
	.globl glWindowPos3svARB ; .set glWindowPos3svARB, glWindowPos3svMESA
	.globl glBlendFuncSeparate ; .set glBlendFuncSeparate, glBlendFuncSeparateEXT
	.globl glBlendFuncSeparateINGR ; .set glBlendFuncSeparateINGR, glBlendFuncSeparateEXT
	.globl glFogCoordf ; .set glFogCoordf, glFogCoordfEXT
	.globl glFogCoordfv ; .set glFogCoordfv, glFogCoordfvEXT
	.globl glFogCoordd ; .set glFogCoordd, glFogCoorddEXT
	.globl glFogCoorddv ; .set glFogCoorddv, glFogCoorddvEXT
	.globl glFogCoordPointer ; .set glFogCoordPointer, glFogCoordPointerEXT
	.globl glCompressedTexImage3D ; .set glCompressedTexImage3D, glCompressedTexImage3DARB
	.globl glCompressedTexImage2D ; .set glCompressedTexImage2D, glCompressedTexImage2DARB
	.globl glCompressedTexImage1D ; .set glCompressedTexImage1D, glCompressedTexImage1DARB
	.globl glCompressedTexSubImage3D ; .set glCompressedTexSubImage3D, glCompressedTexSubImage3DARB
	.globl glCompressedTexSubImage2D ; .set glCompressedTexSubImage2D, glCompressedTexSubImage2DARB
	.globl glCompressedTexSubImage1D ; .set glCompressedTexSubImage1D, glCompressedTexSubImage1DARB
	.globl glGetCompressedTexImage ; .set glGetCompressedTexImage, glGetCompressedTexImageARB
	.globl glSecondaryColor3b ; .set glSecondaryColor3b, glSecondaryColor3bEXT
	.globl glSecondaryColor3bv ; .set glSecondaryColor3bv, glSecondaryColor3bvEXT
	.globl glSecondaryColor3d ; .set glSecondaryColor3d, glSecondaryColor3dEXT
	.globl glSecondaryColor3dv ; .set glSecondaryColor3dv, glSecondaryColor3dvEXT
	.globl glSecondaryColor3f ; .set glSecondaryColor3f, glSecondaryColor3fEXT
	.globl glSecondaryColor3fv ; .set glSecondaryColor3fv, glSecondaryColor3fvEXT
	.globl glSecondaryColor3i ; .set glSecondaryColor3i, glSecondaryColor3iEXT
	.globl glSecondaryColor3iv ; .set glSecondaryColor3iv, glSecondaryColor3ivEXT
	.globl glSecondaryColor3s ; .set glSecondaryColor3s, glSecondaryColor3sEXT
	.globl glSecondaryColor3sv ; .set glSecondaryColor3sv, glSecondaryColor3svEXT
	.globl glSecondaryColor3ub ; .set glSecondaryColor3ub, glSecondaryColor3ubEXT
	.globl glSecondaryColor3ubv ; .set glSecondaryColor3ubv, glSecondaryColor3ubvEXT
	.globl glSecondaryColor3ui ; .set glSecondaryColor3ui, glSecondaryColor3uiEXT
	.globl glSecondaryColor3uiv ; .set glSecondaryColor3uiv, glSecondaryColor3uivEXT
	.globl glSecondaryColor3us ; .set glSecondaryColor3us, glSecondaryColor3usEXT
	.globl glSecondaryColor3usv ; .set glSecondaryColor3usv, glSecondaryColor3usvEXT
	.globl glSecondaryColorPointer ; .set glSecondaryColorPointer, glSecondaryColorPointerEXT
	.globl glBindProgramARB ; .set glBindProgramARB, glBindProgramNV
	.globl glDeleteProgramsARB ; .set glDeleteProgramsARB, glDeleteProgramsNV
	.globl glGenProgramsARB ; .set glGenProgramsARB, glGenProgramsNV
	.globl glGetVertexAttribPointervARB ; .set glGetVertexAttribPointervARB, glGetVertexAttribPointervNV
	.globl glIsProgramARB ; .set glIsProgramARB, glIsProgramNV
	.globl glPointParameteri ; .set glPointParameteri, glPointParameteriNV
	.globl glPointParameteriv ; .set glPointParameteriv, glPointParameterivNV
	.globl glMultiDrawArrays ; .set glMultiDrawArrays, glMultiDrawArraysEXT
	.globl glMultiDrawElements ; .set glMultiDrawElements, glMultiDrawElementsEXT
	.globl glBindBuffer ; .set glBindBuffer, glBindBufferARB
	.globl glBufferData ; .set glBufferData, glBufferDataARB
	.globl glBufferSubData ; .set glBufferSubData, glBufferSubDataARB
	.globl glDeleteBuffers ; .set glDeleteBuffers, glDeleteBuffersARB
	.globl glGenBuffers ; .set glGenBuffers, glGenBuffersARB
	.globl glGetBufferParameteriv ; .set glGetBufferParameteriv, glGetBufferParameterivARB
	.globl glGetBufferPointerv ; .set glGetBufferPointerv, glGetBufferPointervARB
	.globl glGetBufferSubData ; .set glGetBufferSubData, glGetBufferSubDataARB
	.globl glIsBuffer ; .set glIsBuffer, glIsBufferARB
	.globl glMapBuffer ; .set glMapBuffer, glMapBufferARB
	.globl glUnmapBuffer ; .set glUnmapBuffer, glUnmapBufferARB
	.globl glGenQueries ; .set glGenQueries, glGenQueriesARB
	.globl glDeleteQueries ; .set glDeleteQueries, glDeleteQueriesARB
	.globl glIsQuery ; .set glIsQuery, glIsQueryARB
	.globl glBeginQuery ; .set glBeginQuery, glBeginQueryARB
	.globl glEndQuery ; .set glEndQuery, glEndQueryARB
	.globl glGetQueryiv ; .set glGetQueryiv, glGetQueryivARB
	.globl glGetQueryObjectiv ; .set glGetQueryObjectiv, glGetQueryObjectivARB
	.globl glGetQueryObjectuiv ; .set glGetQueryObjectuiv, glGetQueryObjectuivARB
	.globl glBlendEquationSeparateATI ; .set glBlendEquationSeparateATI, glBlendEquationSeparateEXT

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
