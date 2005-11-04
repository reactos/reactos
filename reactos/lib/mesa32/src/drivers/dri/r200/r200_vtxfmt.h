/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_vtxfmt.h,v 1.1 2002/10/30 12:51:53 alanh Exp $ */
/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

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

#ifndef __R200_VTXFMT_H__
#define __R200_VTXFMT_H__

#include "r200_context.h"



extern void r200VtxfmtUpdate( GLcontext *ctx );
extern void r200VtxfmtInit( GLcontext *ctx, GLboolean useCodegen );
extern void r200VtxfmtInvalidate( GLcontext *ctx );
extern void r200VtxfmtDestroy( GLcontext *ctx );
extern void r200VtxfmtInitChoosers( GLvertexformat *vfmt );

extern void r200VtxfmtMakeCurrent( GLcontext *ctx );
extern void r200VtxfmtUnbindContext( GLcontext *ctx );

extern void r200_copy_to_current( GLcontext *ctx );
extern void VFMT_FALLBACK( const char *caller );

#define DFN( FUNC, CACHE)				\
do {							\
   char *start = (char *)&FUNC;				\
   char *end = (char *)&FUNC##_end;			\
   insert_at_head( &CACHE, dfn );			\
   dfn->key[0] = key[0];				\
   dfn->key[1] = key[1];				\
   dfn->code = _mesa_exec_malloc(end - start);		\
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
   /*fprintf(stderr, "%s/%d CVAL %x OFFSET %d VAL %x\n", __FUNCTION__,*/ \
   /*	   __LINE__, CHECKVAL, OFFSET, (int)(NEWVAL));*/		\
   *(int *)(CODE+OFFSET) = (int)(NEWVAL);				\
   OFFSET += 4;							\
} while (0)

/* 
 */
void r200InitCodegen( struct dfn_generators *gen, GLboolean useCodegen );
void r200InitX86Codegen( struct dfn_generators *gen );
void r200InitSSECodegen( struct dfn_generators *gen );



/* Defined in r200_vtxfmt_x86.c
 */
struct dynfn *r200_makeX86Vertex2f( GLcontext *, const int * );
struct dynfn *r200_makeX86Vertex2fv( GLcontext *, const int * );
struct dynfn *r200_makeX86Vertex3f( GLcontext *, const int * );
struct dynfn *r200_makeX86Vertex3fv( GLcontext *, const int * );
struct dynfn *r200_makeX86Color4ub( GLcontext *, const int * );
struct dynfn *r200_makeX86Color4ubv( GLcontext *, const int * );
struct dynfn *r200_makeX86Color3ub( GLcontext *, const int * );
struct dynfn *r200_makeX86Color3ubv( GLcontext *, const int * );
struct dynfn *r200_makeX86Color4f( GLcontext *, const int * );
struct dynfn *r200_makeX86Color4fv( GLcontext *, const int * );
struct dynfn *r200_makeX86Color3f( GLcontext *, const int * );
struct dynfn *r200_makeX86Color3fv( GLcontext *, const int * );
struct dynfn *r200_makeX86SecondaryColor3ubEXT( GLcontext *, const int * );
struct dynfn *r200_makeX86SecondaryColor3ubvEXT( GLcontext *, const int * );
struct dynfn *r200_makeX86SecondaryColor3fEXT( GLcontext *, const int * );
struct dynfn *r200_makeX86SecondaryColor3fvEXT( GLcontext *, const int * );
struct dynfn *r200_makeX86Normal3f( GLcontext *, const int * );
struct dynfn *r200_makeX86Normal3fv( GLcontext *, const int * );
struct dynfn *r200_makeX86TexCoord2f( GLcontext *, const int * );
struct dynfn *r200_makeX86TexCoord2fv( GLcontext *, const int * );
struct dynfn *r200_makeX86TexCoord1f( GLcontext *, const int * );
struct dynfn *r200_makeX86TexCoord1fv( GLcontext *, const int * );
struct dynfn *r200_makeX86MultiTexCoord2fARB( GLcontext *, const int * );
struct dynfn *r200_makeX86MultiTexCoord2fvARB( GLcontext *, const int * );
struct dynfn *r200_makeX86MultiTexCoord1fARB( GLcontext *, const int * );
struct dynfn *r200_makeX86MultiTexCoord1fvARB( GLcontext *, const int * );

#endif
