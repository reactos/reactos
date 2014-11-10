/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */
/*
** Author: Eric Veach, July 1994.
**
*/

#ifndef __geom_h_
#define __geom_h_

#include "mesh.h"

#ifdef NO_BRANCH_CONDITIONS
/* MIPS architecture has special instructions to evaluate boolean
 * conditions -- more efficient than branching, IF you can get the
 * compiler to generate the right instructions (SGI compiler doesn't)
 */
#define VertEq(u,v)	(((u)->s == (v)->s) & ((u)->t == (v)->t))
#define VertLeq(u,v)	(((u)->s < (v)->s) | \
                         ((u)->s == (v)->s & (u)->t <= (v)->t))
#else
#define VertEq(u,v)	((u)->s == (v)->s && (u)->t == (v)->t)
#define VertLeq(u,v)	(((u)->s < (v)->s) || \
                         ((u)->s == (v)->s && (u)->t <= (v)->t))
#endif

#define EdgeEval(u,v,w) __gl_edgeEval(u,v,w)
#define EdgeSign(u,v,w) __gl_edgeSign(u,v,w)

/* Versions of VertLeq, EdgeSign, EdgeEval with s and t transposed. */

#define TransLeq(u,v)	(((u)->t < (v)->t) || \
                         ((u)->t == (v)->t && (u)->s <= (v)->s))
#define TransEval(u,v,w)	__gl_transEval(u,v,w)
#define TransSign(u,v,w)	__gl_transSign(u,v,w)


#define EdgeGoesLeft(e) 	VertLeq( (e)->Dst, (e)->Org )
#define EdgeGoesRight(e)	VertLeq( (e)->Org, (e)->Dst )

#undef	ABS
#define ABS(x)	((x) < 0 ? -(x) : (x))
#define VertL1dist(u,v) (ABS(u->s - v->s) + ABS(u->t - v->t))

#define VertCCW(u,v,w)	__gl_vertCCW(u,v,w)

int		__gl_vertLeq( GLUvertex *u, GLUvertex *v );
GLdouble	__gl_edgeEval( GLUvertex *u, GLUvertex *v, GLUvertex *w );
GLdouble	__gl_edgeSign( GLUvertex *u, GLUvertex *v, GLUvertex *w );
GLdouble	__gl_transEval( GLUvertex *u, GLUvertex *v, GLUvertex *w );
GLdouble	__gl_transSign( GLUvertex *u, GLUvertex *v, GLUvertex *w );
int		__gl_vertCCW( GLUvertex *u, GLUvertex *v, GLUvertex *w );
void		__gl_edgeIntersect( GLUvertex *o1, GLUvertex *d1,
				    GLUvertex *o2, GLUvertex *d2,
				    GLUvertex *v );

#endif
