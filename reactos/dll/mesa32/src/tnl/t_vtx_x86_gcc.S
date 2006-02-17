/**************************************************************************

Copyright 2004 Tungsten Graphics Inc., Cedar Park, Texas.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Daniel Borca <dborca@yahoo.com>
 */

#if defined (__DJGPP__) || defined (__MINGW32__) || defined (__CYGWIN__)
#define GLOBL( x )	\
.globl _##x;		\
_##x:
#else  /* !defined (__DJGPP__) && !defined (__MINGW32__) && !defined (__CYGWIN__) */
#define GLOBL( x )	\
.globl x;		\
x:
#endif /* !defined (__DJGPP__) && !defined (__MINGW32__) && !defined (__CYGWIN__) */


#if !defined (STDCALL_API)
#define RETCLEAN( x )	ret
#else
#define RETCLEAN( x )	ret $x
#endif


#define _JMP(x)		\
.byte 0xe9;		\
.long x

#define _CALL(x)	\
.byte 0xe8;		\
.long x


/* Someone who knew a lot about this sort of thing would use this
 * macro to note current offsets, etc in a special region of the
 * object file & just make everything work out neat.  I don't know
 * enough to do that...
 */

#define SUBST( x ) (0x10101010 + x)


.data


/* [dBorca] TODO
 * Unfold functions for each vertex size?
 * Build super-specialized SSE versions?
 *
 * There is a trick in Vertex*fv: under certain conditions,
 * we tail to _tnl_wrap_filled_vertex(ctx). This means that
 * if Vertex*fv is STDCALL, then _tnl_wrap_filled_vertex must
 * be STDCALL as well, because (GLcontext *) and (GLfloat *)
 * have the same size.
 */
.align 4
GLOBL ( _tnl_x86_Vertex1fv )
	movl	4(%esp), %ecx
	push	%edi
	push	%esi
	movl	SUBST(0), %edi	/* 0x0 --> tnl->vtx.vbptr */
	movl	(%ecx), %edx	/* load v[0] */
	movl	%edx, (%edi)	/* tnl->vtx.vbptr[0] = v[0] */
	addl	$4, %edi	/* tnl->vtx.vbptr += 1 */
	movl	$SUBST(1), %ecx	/* 0x1 --> (tnl->vtx.vertex_size - 1) */
	movl	$SUBST(2), %esi	/* 0x2 --> (tnl->vtx.vertex + 1) */
	repz
	movsl	%ds:(%esi), %es:(%edi)
	movl	%edi, SUBST(0)	/* 0x0 --> tnl->vtx.vbptr */
	movl	SUBST(3), %edx	/* 0x3 --> counter */
	pop	%esi
	pop	%edi
	dec	%edx		/* counter-- */
	movl	%edx, SUBST(3)	/* 0x3 --> counter */
	je	.0		/* if (counter == 0) goto .0 */
	RETCLEAN(4)		/* return */
	.balign	16
.0:
	movl	$SUBST(4), %eax	/* load ctx */
	movl	%eax, 4(%esp)	/* push ctx */
	_JMP	(SUBST(5))	/* jmp _tnl_wrap_filled_vertex */
GLOBL ( _tnl_x86_Vertex1fv_end )

.align 4
GLOBL ( _tnl_x86_Vertex2fv )
	movl	4(%esp), %ecx
	push	%edi
	push	%esi
	movl	SUBST(0), %edi	/* load tnl->vtx.vbptr */
	movl	(%ecx), %edx	/* load v[0] */
	movl	4(%ecx), %eax	/* load v[1] */
	movl	%edx, (%edi)	/* tnl->vtx.vbptr[0] = v[0] */
	movl	%eax, 4(%edi)	/* tnl->vtx.vbptr[1] = v[1] */
	addl	$8, %edi	/* tnl->vtx.vbptr += 2 */
	movl	$SUBST(1), %ecx	/* vertex_size - 2 */
	movl	$SUBST(2), %esi	/* tnl->vtx.vertex + 2 */
	repz
	movsl	%ds:(%esi), %es:(%edi)
	movl	%edi, SUBST(0)	/* save tnl->vtx.vbptr */
	movl	SUBST(3), %edx	/* load counter */
	pop	%esi
	pop	%edi
	dec	%edx		/* counter-- */
	movl	%edx, SUBST(3)	/* save counter */
	je	.1		/* if (counter == 0) goto .1 */
	RETCLEAN(4)		/* return */
	.balign	16
.1:
	movl	$SUBST(4), %eax	/* load ctx */
	movl	%eax, 4(%esp)	/* push ctx */
	_JMP	(SUBST(5))	/* jmp _tnl_wrap_filled_vertex */
GLOBL ( _tnl_x86_Vertex2fv_end )

.align 4
GLOBL ( _tnl_x86_Vertex3fv )
	movl	4(%esp), %ecx
	push	%edi
	push	%esi
	movl	SUBST(0), %edi	/* load tnl->vtx.vbptr */
	movl	(%ecx), %edx	/* load v[0] */
	movl	4(%ecx), %eax	/* load v[1] */
	movl	8(%ecx), %esi	/* load v[2] */
	movl	%edx, (%edi)	/* tnl->vtx.vbptr[0] = v[0] */
	movl	%eax, 4(%edi)	/* tnl->vtx.vbptr[1] = v[1] */
	movl	%esi, 8(%edi)	/* tnl->vtx.vbptr[2] = v[2] */
	addl	$12, %edi	/* tnl->vtx.vbptr += 3 */
	movl	$SUBST(1), %ecx	/* vertex_size - 3 */
	movl	$SUBST(2), %esi	/* tnl->vtx.vertex + 3 */
	repz
	movsl	%ds:(%esi), %es:(%edi)
	movl	%edi, SUBST(0)	/* save tnl->vtx.vbptr */
	movl	SUBST(3), %edx	/* load counter */
	pop	%esi
	pop	%edi
	dec	%edx		/* counter-- */
	movl	%edx, SUBST(3)	/* save counter */
	je	.2		/* if (counter == 0) goto .2 */
	RETCLEAN(4)		/* return */
	.balign	16
.2:
	movl	$SUBST(4), %eax	/* load ctx */
	movl	%eax, 4(%esp)	/* push ctx */
	_JMP	(SUBST(5))	/* jmp _tnl_wrap_filled_vertex */
GLOBL ( _tnl_x86_Vertex3fv_end )

.align 4
GLOBL ( _tnl_x86_Vertex4fv )
	movl	4(%esp), %ecx
	push	%edi
	push	%esi
	movl	SUBST(0), %edi	/* load tnl->vtx.vbptr */
	movl	(%ecx), %edx	/* load v[0] */
	movl	4(%ecx), %eax	/* load v[1] */
	movl	8(%ecx), %esi	/* load v[2] */
	movl	12(%ecx), %ecx	/* load v[3] */
	movl	%edx, (%edi)	/* tnl->vtx.vbptr[0] = v[0] */
	movl	%eax, 4(%edi)	/* tnl->vtx.vbptr[1] = v[1] */
	movl	%esi, 8(%edi)	/* tnl->vtx.vbptr[2] = v[2] */
	movl	%ecx, 12(%edi)	/* tnl->vtx.vbptr[3] = v[3] */
	addl	$16, %edi	/* tnl->vtx.vbptr += 4 */
	movl	$SUBST(1), %ecx	/* vertex_size - 4 */
	movl	$SUBST(2), %esi	/* tnl->vtx.vertex + 4 */
	repz
	movsl	%ds:(%esi), %es:(%edi)
	movl	%edi, SUBST(0)	/* save tnl->vtx.vbptr */
	movl	SUBST(3), %edx	/* load counter */
	pop	%esi
	pop	%edi
	dec	%edx		/* counter-- */
	movl	%edx, SUBST(3)	/* save counter */
	je	.3		/* if (counter == 0) goto .3 */
	RETCLEAN(4)		/* return */
	.balign	16
.3:
	movl	$SUBST(4), %eax	/* load ctx */
	movl	%eax, 4(%esp)	/* push ctx */
	_JMP	(SUBST(5))	/* jmp _tnl_wrap_filled_vertex */
GLOBL ( _tnl_x86_Vertex4fv_end )


/**
 * Generic handlers for vector format data. 
 */
GLOBL( _tnl_x86_Attribute1fv )
	movl	4(%esp), %ecx
	movl	(%ecx), %eax	/* load v[0] */
	movl	%eax, SUBST(0)	/* store v[0] to current vertex */
	RETCLEAN(4)
GLOBL ( _tnl_x86_Attribute1fv_end )

GLOBL( _tnl_x86_Attribute2fv )
	movl	4(%esp), %ecx
	movl	(%ecx), %eax	/* load v[0] */
	movl	4(%ecx), %edx	/* load v[1] */
	movl	%eax, SUBST(0)	/* store v[0] to current vertex */
	movl	%edx, SUBST(1)	/* store v[1] to current vertex */
	RETCLEAN(4)
GLOBL ( _tnl_x86_Attribute2fv_end )

GLOBL( _tnl_x86_Attribute3fv )
	movl	4(%esp), %ecx
	movl	(%ecx), %eax	/* load v[0] */
	movl	4(%ecx), %edx	/* load v[1] */
	movl	8(%ecx), %ecx	/* load v[2] */
	movl	%eax, SUBST(0)	/* store v[0] to current vertex */
	movl	%edx, SUBST(1)	/* store v[1] to current vertex */
	movl	%ecx, SUBST(2)	/* store v[2] to current vertex */
	RETCLEAN(4)
GLOBL ( _tnl_x86_Attribute3fv_end )

GLOBL( _tnl_x86_Attribute4fv )
	movl	4(%esp), %ecx
	movl	(%ecx), %eax	/* load v[0] */
	movl	4(%ecx), %edx	/* load v[1] */
	movl	%eax, SUBST(0)	/* store v[0] to current vertex */
	movl	%edx, SUBST(1)	/* store v[1] to current vertex */
	movl	8(%ecx), %eax	/* load v[2] */
	movl	12(%ecx), %edx	/* load v[3] */
	movl	%eax, SUBST(2)	/* store v[2] to current vertex */
	movl	%edx, SUBST(3)	/* store v[3] to current vertex */
	RETCLEAN(4)
GLOBL ( _tnl_x86_Attribute4fv_end )


/* Choosers:
 *
 * Must generate all of these ahead of first usage.  Generate at
 * compile-time?
 */
GLOBL( _tnl_x86_choose_fv )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	movl	$SUBST(0), (%esp)	/* arg 0 - attrib */
	movl	$SUBST(1), 4(%esp)	/* arg 1 - N */
	_CALL	(SUBST(2))		/* call do_choose */
	add	$12, %esp		/* tear down stack frame */
	jmp	*%eax			/* jump to new func */
GLOBL ( _tnl_x86_choose_fv_end )


/* FIRST LEVEL FUNCTIONS -- these are plugged directly into GL dispatch.
 *
 * In the 1st level dispatch functions, switch to a different
 * calling convention -- (const GLfloat *v) in %ecx.
 *
 * As with regular (x86) dispatch, don't create a new stack frame -
 * just let the 'ret' in the dispatched function return straight
 * back to the original caller.
 *
 * Vertex/Normal/Color, etc: the address of the function pointer
 * is known at codegen time.
 */

/* Unfortunately, have to play with the stack in the non-fv case:
 */
#if !defined (STDCALL_API)
GLOBL( _tnl_x86_dispatch_attrf1 )
GLOBL( _tnl_x86_dispatch_attrf2 )
GLOBL( _tnl_x86_dispatch_attrf3 )
GLOBL( _tnl_x86_dispatch_attrf4 )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	leal	16(%esp), %edx	/* address of first float on stack */
	movl	%edx, (%esp)	/* save as 'v' */
	call	*SUBST(0)	/* 0x0 --> tabfv[attr][n] */
	addl	$12, %esp	/* tear down frame */
	ret			/* return */
GLOBL( _tnl_x86_dispatch_attrf4_end )
GLOBL( _tnl_x86_dispatch_attrf3_end )
GLOBL( _tnl_x86_dispatch_attrf2_end )
GLOBL( _tnl_x86_dispatch_attrf1_end )

#else  /* defined(STDCALL_API) */

GLOBL( _tnl_x86_dispatch_attrf1 )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	leal	16(%esp), %edx	/* address of first float on stack */
	movl	%edx, (%esp)	/* save as 'v' */
	call	*SUBST(0)	/* 0x0 --> tabfv[attr][n] */
	addl	$8, %esp	/* tear down frame (4 shaved off by the callee) */
	ret	$4		/* return */
GLOBL( _tnl_x86_dispatch_attrf1_end )

GLOBL( _tnl_x86_dispatch_attrf2 )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	leal	16(%esp), %edx	/* address of first float on stack */
	movl	%edx, (%esp)	/* save as 'v' */
	call	*SUBST(0)	/* 0x0 --> tabfv[attr][n] */
	addl	$8, %esp	/* tear down frame (4 shaved off by the callee) */
	ret	$8		/* return */
GLOBL( _tnl_x86_dispatch_attrf2_end )

GLOBL( _tnl_x86_dispatch_attrf3 )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	leal	16(%esp), %edx	/* address of first float on stack */
	movl	%edx, (%esp)	/* save as 'v' */
	call	*SUBST(0)	/* 0x0 --> tabfv[attr][n] */
	addl	$8, %esp	/* tear down frame (4 shaved off by the callee) */
	ret	$12		/* return */
GLOBL( _tnl_x86_dispatch_attrf3_end )

GLOBL( _tnl_x86_dispatch_attrf4 )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	leal	16(%esp), %edx	/* address of first float on stack */
	movl	%edx, (%esp)	/* save as 'v' */
	call	*SUBST(0)	/* 0x0 --> tabfv[attr][n] */
	addl	$8, %esp	/* tear down frame (4 shaved off by the callee) */
	ret	$16		/* return */
GLOBL( _tnl_x86_dispatch_attrf4_end )
#endif /* defined(STDCALL_API) */

/* The fv case is simpler:
 */
GLOBL( _tnl_x86_dispatch_attrfv )
	jmp	*SUBST(0)	/* 0x0 --> tabfv[attr][n] */
GLOBL( _tnl_x86_dispatch_attrfv_end )


/* MultiTexcoord: the address of the function pointer must be
 * calculated, but can use the index argument slot to hold 'v', and
 * avoid setting up a new stack frame.
 *
 * [dBorca]
 * right, this would be the preferred approach, but gcc does not
 * clean up the stack after each function call when optimizing (-fdefer-pop);
 * can it make assumptions about what's already on the stack?  I dunno,
 * but in this case, we can't mess with the caller's stack frame, and
 * we must use a model like `_x86_dispatch_attrfv' above.  Caveat emptor!
 */

/* Also, will only need a maximum of four of each of these per context:
 */
#if !defined (STDCALL_API)
GLOBL( _tnl_x86_dispatch_multitexcoordf1 )
GLOBL( _tnl_x86_dispatch_multitexcoordf2 )
GLOBL( _tnl_x86_dispatch_multitexcoordf3 )
GLOBL( _tnl_x86_dispatch_multitexcoordf4 )
	movl	4(%esp), %ecx
	leal	8(%esp), %edx
	andl	$7, %ecx
	movl	%edx, 4(%esp)
	sall	$4, %ecx
	jmp	*SUBST(0)(%ecx)	/* 0x0 - tabfv[tex0][n] */
GLOBL( _tnl_x86_dispatch_multitexcoordf4_end )
GLOBL( _tnl_x86_dispatch_multitexcoordf3_end )
GLOBL( _tnl_x86_dispatch_multitexcoordf2_end )
GLOBL( _tnl_x86_dispatch_multitexcoordf1_end )

GLOBL( _tnl_x86_dispatch_multitexcoordfv )
	movl	4(%esp), %ecx
	movl	8(%esp), %edx
	andl	$7, %ecx
	movl	%edx, 4(%esp)
	sall	$4, %ecx
	jmp	*SUBST(0)(%ecx)	/* 0x0 - tabfv[tex0][n] */
GLOBL( _tnl_x86_dispatch_multitexcoordfv_end )

#else  /* defined (STDCALL_API) */

GLOBL( _tnl_x86_dispatch_multitexcoordf1 )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	movl	16(%esp), %ecx
	leal	20(%esp), %edx
	andl	$7, %ecx
	movl	%edx, (%esp)
	sall	$4, %ecx
	call	*SUBST(0)(%ecx)	/* 0x0 - tabfv[tex0][n] */
	addl	$8, %esp	/* tear down frame (4 shaved off by the callee) */
	ret	$8		/* return */
GLOBL( _tnl_x86_dispatch_multitexcoordf1_end )

GLOBL( _tnl_x86_dispatch_multitexcoordf2 )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	movl	16(%esp), %ecx
	leal	20(%esp), %edx
	andl	$7, %ecx
	movl	%edx, (%esp)
	sall	$4, %ecx
	call	*SUBST(0)(%ecx)	/* 0x0 - tabfv[tex0][n] */
	addl	$8, %esp	/* tear down frame (4 shaved off by the callee) */
	ret	$12		/* return */
GLOBL( _tnl_x86_dispatch_multitexcoordf2_end )

GLOBL( _tnl_x86_dispatch_multitexcoordf3 )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	movl	16(%esp), %ecx
	leal	20(%esp), %edx
	andl	$7, %ecx
	movl	%edx, (%esp)
	sall	$4, %ecx
	call	*SUBST(0)(%ecx)	/* 0x0 - tabfv[tex0][n] */
	addl	$8, %esp	/* tear down frame (4 shaved off by the callee) */
	ret	$16		/* return */
GLOBL( _tnl_x86_dispatch_multitexcoordf3_end )

GLOBL( _tnl_x86_dispatch_multitexcoordf4 )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	movl	16(%esp), %ecx
	leal	20(%esp), %edx
	andl	$7, %ecx
	movl	%edx, (%esp)
	sall	$4, %ecx
	call	*SUBST(0)(%ecx)	/* 0x0 - tabfv[tex0][n] */
	addl	$8, %esp	/* tear down frame (4 shaved off by the callee) */
	ret	$20		/* return */
GLOBL( _tnl_x86_dispatch_multitexcoordf4_end )

GLOBL( _tnl_x86_dispatch_multitexcoordfv )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	movl	16(%esp), %ecx
	movl	20(%esp), %edx
	andl	$7, %ecx
	movl	%edx, (%esp)
	sall	$4, %ecx
	call	*SUBST(0)(%ecx)	/* 0x0 - tabfv[tex0][n] */
	addl	$8, %esp	/* tear down frame (4 shaved off by the callee) */
	ret	$8		/* return */
GLOBL( _tnl_x86_dispatch_multitexcoordfv_end )
#endif /* defined (STDCALL_API) */


/* VertexAttrib: the address of the function pointer must be
 * calculated.
 */
#if !defined (STDCALL_API)
GLOBL( _tnl_x86_dispatch_vertexattribf1 )
GLOBL( _tnl_x86_dispatch_vertexattribf2 )
GLOBL( _tnl_x86_dispatch_vertexattribf3 )
GLOBL( _tnl_x86_dispatch_vertexattribf4 )
	movl	4(%esp), %eax
	cmpl	$16, %eax
	jb	.8		/* "cmovge" is not supported on all CPUs */
	movl	$16, %eax
.8:
	leal	8(%esp), %ecx	/* calculate 'v' */
	movl	%ecx, 4(%esp)	/* save in 1st arg slot */
	sall	$4, %eax
	jmp	*SUBST(0)(%eax)	/* 0x0 - tabfv[0][n] */
GLOBL( _tnl_x86_dispatch_vertexattribf4_end )
GLOBL( _tnl_x86_dispatch_vertexattribf3_end )
GLOBL( _tnl_x86_dispatch_vertexattribf2_end )
GLOBL( _tnl_x86_dispatch_vertexattribf1_end )

GLOBL( _tnl_x86_dispatch_vertexattribfv )
	movl	4(%esp), %eax
	cmpl	$16, %eax
	jb	.9		/* "cmovge" is not supported on all CPUs */
	movl	$16, %eax
.9:
	movl	8(%esp), %ecx	/* load 'v' */
	movl	%ecx, 4(%esp)	/* save in 1st arg slot */
	sall	$4, %eax
	jmp	*SUBST(0)(%eax)	/* 0x0 - tabfv[0][n] */
GLOBL( _tnl_x86_dispatch_vertexattribfv_end )

#else  /* defined (STDCALL_API) */

GLOBL( _tnl_x86_dispatch_vertexattribf1 )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	movl	16(%esp), %eax
	cmpl	$16, %eax
	jb	.81		/* "cmovge" is not supported on all CPUs */
	movl	$16, %eax
.81:
	leal	20(%esp), %ecx	/* load 'v' */
	movl	%ecx, (%esp)	/* save in 1st arg slot */
	sall	$4, %eax
	call	*SUBST(0)(%eax)	/* 0x0 - tabfv[0][n] */
	addl	$8, %esp	/* tear down frame (4 shaved off by the callee) */
	ret	$8		/* return */
GLOBL( _tnl_x86_dispatch_vertexattribf1_end )

GLOBL( _tnl_x86_dispatch_vertexattribf2 )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	movl	16(%esp), %eax
	cmpl	$16, %eax
	jb	.82		/* "cmovge" is not supported on all CPUs */
	movl	$16, %eax
.82:
	leal	20(%esp), %ecx	/* load 'v' */
	movl	%ecx, (%esp)	/* save in 1st arg slot */
	sall	$4, %eax
	call	*SUBST(0)(%eax)	/* 0x0 - tabfv[0][n] */
	addl	$8, %esp	/* tear down frame (4 shaved off by the callee) */
	ret	$12		/* return */
GLOBL( _tnl_x86_dispatch_vertexattribf2_end )

GLOBL( _tnl_x86_dispatch_vertexattribf3 )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	movl	16(%esp), %eax
	cmpl	$16, %eax
	jb	.83		/* "cmovge" is not supported on all CPUs */
	movl	$16, %eax
.83:
	leal	20(%esp), %ecx	/* load 'v' */
	movl	%ecx, (%esp)	/* save in 1st arg slot */
	sall	$4, %eax
	call	*SUBST(0)(%eax)	/* 0x0 - tabfv[0][n] */
	addl	$8, %esp	/* tear down frame (4 shaved off by the callee) */
	ret	$16		/* return */
GLOBL( _tnl_x86_dispatch_vertexattribf3_end )

GLOBL( _tnl_x86_dispatch_vertexattribf4 )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	movl	16(%esp), %eax
	cmpl	$16, %eax
	jb	.84		/* "cmovge" is not supported on all CPUs */
	movl	$16, %eax
.84:
	leal	20(%esp), %ecx	/* load 'v' */
	movl	%ecx, (%esp)	/* save in 1st arg slot */
	sall	$4, %eax
	call	*SUBST(0)(%eax)	/* 0x0 - tabfv[0][n] */
	addl	$8, %esp	/* tear down frame (4 shaved off by the callee) */
	ret	$20		/* return */
GLOBL( _tnl_x86_dispatch_vertexattribf4_end )

GLOBL( _tnl_x86_dispatch_vertexattribfv )
	subl	$12, %esp	/* gcc does 16 byte alignment of stack frames? */
	movl	16(%esp), %eax
	cmpl	$16, %eax
	jb	.9		/* "cmovge" is not supported on all CPUs */
	movl	$16, %eax
.9:
	movl	20(%esp), %ecx	/* load 'v' */
	movl	%ecx, (%esp)	/* save in 1st arg slot */
	sall	$4, %eax
	call	*SUBST(0)(%eax)	/* 0x0 - tabfv[0][n] */
	addl	$8, %esp	/* tear down frame (4 shaved off by the callee) */
	ret	$8		/* return */
GLOBL( _tnl_x86_dispatch_vertexattribfv_end )
#endif /* defined (STDCALL_API) */
