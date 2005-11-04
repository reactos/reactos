/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_vtxfmt.h,v 1.3 2002/12/21 17:02:16 dawes Exp $ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     Tungsten Graphics Inc., Cedar Park, Texas.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef __RADEON_VTXFMT_H__
#define __RADEON_VTXFMT_H__

#include "radeon_context.h"


extern void radeonVtxfmtUpdate( GLcontext *ctx );
extern void radeonVtxfmtInit( GLcontext *ctx, GLboolean useCodegen );
extern void radeonVtxfmtInvalidate( GLcontext *ctx );
extern void radeonVtxfmtDestroy( GLcontext *ctx );
extern void radeonVtxfmtInitChoosers( GLvertexformat *vfmt );

extern void radeonVtxfmtMakeCurrent( GLcontext *ctx );
extern void radeonVtxfmtUnbindContext( GLcontext *ctx );

extern void radeon_copy_to_current( GLcontext *ctx );

#define DFN( FUNC, CACHE)				\
do {							\
   char *start = (char *)&FUNC;				\
   char *end = (char *)&FUNC##_end;			\
   insert_at_head( &CACHE, dfn );			\
   dfn->key = key;					\
   dfn->code = _mesa_exec_malloc( end - start );	\
   _mesa_memcpy(dfn->code, start, end - start);		\
}							\
while ( 0 )

#define FIXUP( CODE, OFFSET, CHECKVAL, NEWVAL )	\
do {						\
   int *icode = (int *)(CODE+OFFSET);		\
   assert (*icode == CHECKVAL);			\
   *icode = (int)NEWVAL;			\
} while (0)


/* Useful for figuring out the offsets:
 */
#define FIXUP2( CODE, OFFSET, CHECKVAL, NEWVAL )		\
do {								\
   while (*(int *)(CODE+OFFSET) != CHECKVAL) OFFSET++;		\
   fprintf(stderr, "%s/%d CVAL %x OFFSET %d VAL %x\n", __FUNCTION__,	\
	   __LINE__, CHECKVAL, OFFSET, (int)(NEWVAL));			\
   *(int *)(CODE+OFFSET) = (int)(NEWVAL);				\
   OFFSET += 4;							\
} while (0)

/* 
 */
void radeonInitCodegen( struct dfn_generators *gen, GLboolean useCodegen );
void radeonInitX86Codegen( struct dfn_generators *gen );
void radeonInitSSECodegen( struct dfn_generators *gen );



/* Defined in radeon_vtxfmt_x86.c
 */
struct dynfn *radeon_makeX86Vertex2f( GLcontext *, int );
struct dynfn *radeon_makeX86Vertex2fv( GLcontext *, int );
struct dynfn *radeon_makeX86Vertex3f( GLcontext *, int );
struct dynfn *radeon_makeX86Vertex3fv( GLcontext *, int );
struct dynfn *radeon_makeX86Color4ub( GLcontext *, int );
struct dynfn *radeon_makeX86Color4ubv( GLcontext *, int );
struct dynfn *radeon_makeX86Color3ub( GLcontext *, int );
struct dynfn *radeon_makeX86Color3ubv( GLcontext *, int );
struct dynfn *radeon_makeX86Color4f( GLcontext *, int );
struct dynfn *radeon_makeX86Color4fv( GLcontext *, int );
struct dynfn *radeon_makeX86Color3f( GLcontext *, int );
struct dynfn *radeon_makeX86Color3fv( GLcontext *, int );
struct dynfn *radeon_makeX86SecondaryColor3ubEXT( GLcontext *, int );
struct dynfn *radeon_makeX86SecondaryColor3ubvEXT( GLcontext *, int );
struct dynfn *radeon_makeX86SecondaryColor3fEXT( GLcontext *, int );
struct dynfn *radeon_makeX86SecondaryColor3fvEXT( GLcontext *, int );
struct dynfn *radeon_makeX86Normal3f( GLcontext *, int );
struct dynfn *radeon_makeX86Normal3fv( GLcontext *, int );
struct dynfn *radeon_makeX86TexCoord2f( GLcontext *, int );
struct dynfn *radeon_makeX86TexCoord2fv( GLcontext *, int );
struct dynfn *radeon_makeX86TexCoord1f( GLcontext *, int );
struct dynfn *radeon_makeX86TexCoord1fv( GLcontext *, int );
struct dynfn *radeon_makeX86MultiTexCoord2fARB( GLcontext *, int );
struct dynfn *radeon_makeX86MultiTexCoord2fvARB( GLcontext *, int );
struct dynfn *radeon_makeX86MultiTexCoord1fARB( GLcontext *, int );
struct dynfn *radeon_makeX86MultiTexCoord1fvARB( GLcontext *, int );

#endif
