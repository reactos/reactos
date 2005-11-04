/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_vtxfmt_sse.c,v 1.1 2002/10/30 12:51:53 alanh Exp $ */
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

#include "glheader.h"
#include "imports.h"
#include "simple_list.h" 
#include "r200_vtxfmt.h"

#if defined(USE_SSE_ASM)
#include "x86/common_x86_asm.h"

#define EXTERN( FUNC )		\
extern const char *FUNC;	\
extern const char *FUNC##_end

EXTERN( _sse_Attribute2fv );
EXTERN( _sse_Attribute2f );
EXTERN( _sse_Attribute3fv );
EXTERN( _sse_Attribute3f );
EXTERN( _sse_MultiTexCoord2fv );
EXTERN( _sse_MultiTexCoord2f );
EXTERN( _sse_MultiTexCoord2fv_2 );
EXTERN( _sse_MultiTexCoord2f_2 );

/* Build specialized versions of the immediate calls on the fly for
 * the current state.
 */

static struct dynfn *r200_makeSSEAttribute2fv( struct dynfn * cache, const int * key,
					       const char * name, void * dest)
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x\n", name, key[0] );

   DFN ( _sse_Attribute2fv, (*cache) );
   FIXUP(dfn->code, 10, 0x0, (int)dest);
   return dfn;
}

static struct dynfn *r200_makeSSEAttribute2f( struct dynfn * cache, const int * key,
					      const char * name, void * dest )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x\n", name, key[0] );

   DFN ( _sse_Attribute2f, (*cache) );
   FIXUP(dfn->code, 8, 0x0, (int)dest); 
   return dfn;
}

static struct dynfn *r200_makeSSEAttribute3fv( struct dynfn * cache, const int * key,
					       const char * name, void * dest)
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x\n", name, key[0] );

   DFN ( _sse_Attribute3fv, (*cache) );
   FIXUP(dfn->code, 13, 0x0, (int)dest);
   FIXUP(dfn->code, 18, 0x8, 8+(int)dest);
   return dfn;
}

static struct dynfn *r200_makeSSEAttribute3f( struct dynfn * cache, const int * key,
					      const char * name, void * dest )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x\n", name, key[0] );

   DFN ( _sse_Attribute3f, (*cache) );
   FIXUP(dfn->code, 12, 0x0, (int)dest); 
   FIXUP(dfn->code, 17, 0x8, 8+(int)dest); 
   return dfn;
}

static struct dynfn *r200_makeSSENormal3fv( GLcontext *ctx, const int *key )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   return r200_makeSSEAttribute3fv( & rmesa->vb.dfn_cache.Normal3fv, key,
				    __FUNCTION__, rmesa->vb.normalptr );
}

static struct dynfn *r200_makeSSENormal3f( GLcontext *ctx, const int * key )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   return r200_makeSSEAttribute3f( & rmesa->vb.dfn_cache.Normal3f, key,
				   __FUNCTION__, rmesa->vb.normalptr );
}

static struct dynfn *r200_makeSSEColor3fv( GLcontext *ctx, const int * key )
{
   if (VTX_COLOR(key[0],0) != R200_VTX_FP_RGB) 
      return NULL;
   else
   {
      r200ContextPtr rmesa = R200_CONTEXT(ctx);

      return r200_makeSSEAttribute3fv( & rmesa->vb.dfn_cache.Color3fv, key,
				       __FUNCTION__, rmesa->vb.floatcolorptr );
   }
}

static struct dynfn *r200_makeSSEColor3f( GLcontext *ctx, const int * key )
{
   if (VTX_COLOR(key[0],0) != R200_VTX_FP_RGB) 
      return NULL;
   else
   {
      r200ContextPtr rmesa = R200_CONTEXT(ctx);

      return r200_makeSSEAttribute3f( & rmesa->vb.dfn_cache.Color3f, key,
				      __FUNCTION__, rmesa->vb.floatcolorptr );
   }
}

#if 0 /* Temporarily disabled as it is broken w/the new cubemap code. - idr */
static struct dynfn *r200_makeSSETexCoord2fv( GLcontext *ctx, const int * key )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   return r200_makeSSEAttribute2fv( & rmesa->vb.dfn_cache.TexCoord2fv, key,
				    __FUNCTION__, rmesa->vb.texcoordptr[0] );
}

static struct dynfn *r200_makeSSETexCoord2f( GLcontext *ctx, const int * key )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   return r200_makeSSEAttribute2f( & rmesa->vb.dfn_cache.TexCoord2f, key,
				   __FUNCTION__, rmesa->vb.texcoordptr[0] );
}

static struct dynfn *r200_makeSSEMultiTexCoord2fv( GLcontext *ctx, const int * key )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x\n", __FUNCTION__, key[0] );

   if (rmesa->vb.texcoordptr[1] == rmesa->vb.texcoordptr[0]+4) {
      DFN ( _sse_MultiTexCoord2fv, rmesa->vb.dfn_cache.MultiTexCoord2fvARB );
      FIXUP(dfn->code, 18, 0xdeadbeef, (int)rmesa->vb.texcoordptr[0]);	
   } else {
      DFN ( _sse_MultiTexCoord2fv_2, rmesa->vb.dfn_cache.MultiTexCoord2fvARB );
      FIXUP(dfn->code, 14, 0x0, (int)rmesa->vb.texcoordptr);
   }
   return dfn;
}

static struct dynfn *r200_makeSSEMultiTexCoord2f( GLcontext *ctx, const int * key )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x\n", __FUNCTION__, key[0] );

   if (rmesa->vb.texcoordptr[1] == rmesa->vb.texcoordptr[0]+4) {
      DFN ( _sse_MultiTexCoord2f, rmesa->vb.dfn_cache.MultiTexCoord2fARB );
      FIXUP(dfn->code, 16, 0xdeadbeef, (int)rmesa->vb.texcoordptr[0]);	
   } else {
      DFN ( _sse_MultiTexCoord2f_2, rmesa->vb.dfn_cache.MultiTexCoord2fARB );
      FIXUP(dfn->code, 15, 0x0, (int)rmesa->vb.texcoordptr);
   }
   return dfn;
}
#endif

void r200InitSSECodegen( struct dfn_generators *gen )
{
   if ( cpu_has_xmm ) {
      gen->Normal3fv = (void *) r200_makeSSENormal3fv;
      gen->Normal3f = (void *) r200_makeSSENormal3f;
      gen->Color3fv = (void *) r200_makeSSEColor3fv;
      gen->Color3f = (void *) r200_makeSSEColor3f;
#if 0 /* Temporarily disabled as it is broken w/the new cubemap code. - idr */
      gen->TexCoord2fv = (void *) r200_makeSSETexCoord2fv;
      gen->TexCoord2f = (void *) r200_makeSSETexCoord2f;
      gen->MultiTexCoord2fvARB = (void *) r200_makeSSEMultiTexCoord2fv;
      gen->MultiTexCoord2fARB = (void *) r200_makeSSEMultiTexCoord2f;
#endif
   }
}

#else 

void r200InitSSECodegen( struct dfn_generators *gen )
{
   (void) gen;
}

#endif
