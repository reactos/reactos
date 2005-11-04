/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_vtxfmt_x86.c,v 1.2 2002/12/16 16:18:56 dawes Exp $ */
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

#if defined(USE_X86_ASM)

#define EXTERN( FUNC )		\
extern const char *FUNC;	\
extern const char *FUNC##_end

EXTERN ( _x86_Attribute2fv );
EXTERN ( _x86_Attribute2f );
EXTERN ( _x86_Attribute3fv );
EXTERN ( _x86_Attribute3f );
EXTERN ( _x86_Vertex3fv_6 );
EXTERN ( _x86_Vertex3fv_8 );
EXTERN ( _x86_Vertex3fv );
EXTERN ( _x86_Vertex3f_4 );
EXTERN ( _x86_Vertex3f_6 );
EXTERN ( _x86_Vertex3f );
EXTERN ( _x86_Color4ubv_ub );
EXTERN ( _x86_Color4ubv_4f );
EXTERN ( _x86_Color4ub_ub );
EXTERN ( _x86_MultiTexCoord2fv );
EXTERN ( _x86_MultiTexCoord2fv_2 );
EXTERN ( _x86_MultiTexCoord2f );
EXTERN ( _x86_MultiTexCoord2f_2 );


/* Build specialized versions of the immediate calls on the fly for
 * the current state.  Generic x86 versions.
 */

struct dynfn *r200_makeX86Vertex3f( GLcontext *ctx, const int *key )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x 0x%08x %d\n", __FUNCTION__, 
	      key[0], key[1], rmesa->vb.vertex_size );

   switch (rmesa->vb.vertex_size) {
   case 4: {

      DFN ( _x86_Vertex3f_4, rmesa->vb.dfn_cache.Vertex3f );
      FIXUP(dfn->code, 2, 0x0, (int)&rmesa->vb.dmaptr);
      FIXUP(dfn->code, 25, 0x0, (int)&rmesa->vb.vertex[3]);
      FIXUP(dfn->code, 36, 0x0, (int)&rmesa->vb.counter);
      FIXUP(dfn->code, 46, 0x0, (int)&rmesa->vb.dmaptr);
      FIXUP(dfn->code, 51, 0x0, (int)&rmesa->vb.counter);
      FIXUP(dfn->code, 60, 0x0, (int)&rmesa->vb.notify);
      break;
   }
   case 6: {

      DFN ( _x86_Vertex3f_6, rmesa->vb.dfn_cache.Vertex3f );
      FIXUP(dfn->code, 3, 0x0, (int)&rmesa->vb.dmaptr);
      FIXUP(dfn->code, 28, 0x0, (int)&rmesa->vb.vertex[3]);
      FIXUP(dfn->code, 34, 0x0, (int)&rmesa->vb.vertex[4]);
      FIXUP(dfn->code, 40, 0x0, (int)&rmesa->vb.vertex[5]);
      FIXUP(dfn->code, 57, 0x0, (int)&rmesa->vb.counter);
      FIXUP(dfn->code, 63, 0x0, (int)&rmesa->vb.dmaptr);
      FIXUP(dfn->code, 70, 0x0, (int)&rmesa->vb.counter);
      FIXUP(dfn->code, 79, 0x0, (int)&rmesa->vb.notify);
      break;
   }
   default: {

      DFN ( _x86_Vertex3f, rmesa->vb.dfn_cache.Vertex3f );
      FIXUP(dfn->code, 3, 0x0, (int)&rmesa->vb.vertex[3]);
      FIXUP(dfn->code, 9, 0x0, (int)&rmesa->vb.dmaptr);
      FIXUP(dfn->code, 37, 0x0, rmesa->vb.vertex_size-3);
      FIXUP(dfn->code, 44, 0x0, (int)&rmesa->vb.counter);
      FIXUP(dfn->code, 50, 0x0, (int)&rmesa->vb.dmaptr);
      FIXUP(dfn->code, 56, 0x0, (int)&rmesa->vb.counter);
      FIXUP(dfn->code, 67, 0x0, (int)&rmesa->vb.notify);
   break;
   }
   }

   return dfn;
}



struct dynfn *r200_makeX86Vertex3fv( GLcontext *ctx, const int *key )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x 0x%08x %d\n", __FUNCTION__, 
	      key[0], key[1], rmesa->vb.vertex_size );

   switch (rmesa->vb.vertex_size) {
   case 6: {

      DFN ( _x86_Vertex3fv_6, rmesa->vb.dfn_cache.Vertex3fv );
      FIXUP(dfn->code, 1, 0x00000000, (int)&rmesa->vb.dmaptr);
      FIXUP(dfn->code, 27, 0x0000001c, (int)&rmesa->vb.vertex[3]);
      FIXUP(dfn->code, 33, 0x00000020, (int)&rmesa->vb.vertex[4]);
      FIXUP(dfn->code, 45, 0x00000024, (int)&rmesa->vb.vertex[5]);
      FIXUP(dfn->code, 56, 0x00000000, (int)&rmesa->vb.dmaptr);
      FIXUP(dfn->code, 61, 0x00000004, (int)&rmesa->vb.counter);
      FIXUP(dfn->code, 67, 0x00000004, (int)&rmesa->vb.counter);
      FIXUP(dfn->code, 76, 0x00000008, (int)&rmesa->vb.notify);
      break;
   }
   

   case 8: {

      DFN ( _x86_Vertex3fv_8, rmesa->vb.dfn_cache.Vertex3fv );
      FIXUP(dfn->code, 1, 0x00000000, (int)&rmesa->vb.dmaptr);
      FIXUP(dfn->code, 27, 0x0000001c, (int)&rmesa->vb.vertex[3]);
      FIXUP(dfn->code, 33, 0x00000020, (int)&rmesa->vb.vertex[4]);
      FIXUP(dfn->code, 45, 0x0000001c, (int)&rmesa->vb.vertex[5]);
      FIXUP(dfn->code, 51, 0x00000020, (int)&rmesa->vb.vertex[6]);
      FIXUP(dfn->code, 63, 0x00000024, (int)&rmesa->vb.vertex[7]);
      FIXUP(dfn->code, 74, 0x00000000, (int)&rmesa->vb.dmaptr);
      FIXUP(dfn->code, 79, 0x00000004, (int)&rmesa->vb.counter);
      FIXUP(dfn->code, 85, 0x00000004, (int)&rmesa->vb.counter);
      FIXUP(dfn->code, 94, 0x00000008, (int)&rmesa->vb.notify);
      break;
   }
   


   default: {

      DFN ( _x86_Vertex3fv, rmesa->vb.dfn_cache.Vertex3fv );
      FIXUP(dfn->code, 8, 0x01010101, (int)&rmesa->vb.dmaptr);
      FIXUP(dfn->code, 32, 0x00000006, rmesa->vb.vertex_size-3);
      FIXUP(dfn->code, 37, 0x00000058, (int)&rmesa->vb.vertex[3]);
      FIXUP(dfn->code, 45, 0x01010101, (int)&rmesa->vb.dmaptr);
      FIXUP(dfn->code, 50, 0x02020202, (int)&rmesa->vb.counter);
      FIXUP(dfn->code, 58, 0x02020202, (int)&rmesa->vb.counter);
      FIXUP(dfn->code, 67, 0x0, (int)&rmesa->vb.notify);
   break;
   }
   }

   return dfn;
}

static struct dynfn *
r200_makeX86Attribute2fv( struct dynfn * cache, const int *key,
			  const char * name, void * dest )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x\n", name, key[0] );

   DFN ( _x86_Attribute2fv, (*cache) );
   FIXUP(dfn->code, 11, 0x0, (int)dest); 
   FIXUP(dfn->code, 16, 0x4, 4+(int)dest); 

   return dfn;
}

static struct dynfn *
r200_makeX86Attribute2f( struct dynfn * cache, const int *key,
			 const char * name, void * dest )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x\n", name, key[0] );

   DFN ( _x86_Attribute2f, (*cache) );
   FIXUP(dfn->code, 1, 0x0, (int)dest); 

   return dfn;
}


static struct dynfn *
r200_makeX86Attribute3fv( struct dynfn * cache, const int *key,
			  const char * name, void * dest )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x\n", name, key[0] );

   DFN ( _x86_Attribute3fv, (*cache) );
   FIXUP(dfn->code, 14, 0x0, (int)dest); 
   FIXUP(dfn->code, 20, 0x4, 4+(int)dest); 
   FIXUP(dfn->code, 25, 0x8, 8+(int)dest);

   return dfn;
}

static struct dynfn *
r200_makeX86Attribute3f( struct dynfn * cache, const int *key,
			 const char * name, void * dest )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );

   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x\n", name, key[0] );

   DFN ( _x86_Attribute3f, (*cache) );
   FIXUP(dfn->code, 14, 0x0, (int)dest); 
   FIXUP(dfn->code, 20, 0x4, 4+(int)dest); 
   FIXUP(dfn->code, 25, 0x8, 8+(int)dest);

   return dfn;
}

struct dynfn *r200_makeX86Normal3fv( GLcontext *ctx, const int *key )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   return r200_makeX86Attribute3fv( & rmesa->vb.dfn_cache.Normal3fv, key,
				    __FUNCTION__, rmesa->vb.normalptr );
}

struct dynfn *r200_makeX86Normal3f( GLcontext *ctx, const int *key )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   return r200_makeX86Attribute3f( & rmesa->vb.dfn_cache.Normal3f, key,
				   __FUNCTION__, rmesa->vb.normalptr );
}

struct dynfn *r200_makeX86Color4ubv( GLcontext *ctx, const int *key )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );
   r200ContextPtr rmesa = R200_CONTEXT(ctx);


   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x\n", __FUNCTION__, key[0] );

   if (VTX_COLOR(key[0],0) == R200_VTX_PK_RGBA) {
      DFN ( _x86_Color4ubv_ub, rmesa->vb.dfn_cache.Color4ubv);
      FIXUP(dfn->code, 5, 0x12345678, (int)rmesa->vb.colorptr); 
      return dfn;
   } 
   else {

      DFN ( _x86_Color4ubv_4f, rmesa->vb.dfn_cache.Color4ubv);
      FIXUP(dfn->code, 2, 0x00000000, (int)_mesa_ubyte_to_float_color_tab); 
      FIXUP(dfn->code, 27, 0xdeadbeaf, (int)rmesa->vb.floatcolorptr); 
      FIXUP(dfn->code, 33, 0xdeadbeaf, (int)rmesa->vb.floatcolorptr+4); 
      FIXUP(dfn->code, 55, 0xdeadbeaf, (int)rmesa->vb.floatcolorptr+8); 
      FIXUP(dfn->code, 61, 0xdeadbeaf, (int)rmesa->vb.floatcolorptr+12); 
      return dfn;
   }
}

struct dynfn *r200_makeX86Color4ub( GLcontext *ctx, const int *key )
{
   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x\n", __FUNCTION__, key[0] );

   if (VTX_COLOR(key[0],0) == R200_VTX_PK_RGBA) {
      struct dynfn *dfn = MALLOC_STRUCT( dynfn );
      r200ContextPtr rmesa = R200_CONTEXT(ctx);

      DFN ( _x86_Color4ub_ub, rmesa->vb.dfn_cache.Color4ub );
      FIXUP(dfn->code, 18, 0x0, (int)rmesa->vb.colorptr); 
      FIXUP(dfn->code, 24, 0x0, (int)rmesa->vb.colorptr+1); 
      FIXUP(dfn->code, 30, 0x0, (int)rmesa->vb.colorptr+2); 
      FIXUP(dfn->code, 36, 0x0, (int)rmesa->vb.colorptr+3); 
      return dfn;
   }
   else
      return NULL;
}


struct dynfn *r200_makeX86Color3fv( GLcontext *ctx, const int *key )
{
   if (VTX_COLOR(key[0],0) != R200_VTX_FP_RGB) 
      return NULL;
   else
   {
      r200ContextPtr rmesa = R200_CONTEXT(ctx);

      return r200_makeX86Attribute3fv( & rmesa->vb.dfn_cache.Color3fv, key,
				       __FUNCTION__, rmesa->vb.floatcolorptr );
   }
}

struct dynfn *r200_makeX86Color3f( GLcontext *ctx, const int *key )
{
   if (VTX_COLOR(key[0],0) != R200_VTX_FP_RGB) 
      return NULL;
   else
   {
      r200ContextPtr rmesa = R200_CONTEXT(ctx);

      return r200_makeX86Attribute3f( & rmesa->vb.dfn_cache.Color3f, key,
				      __FUNCTION__, rmesa->vb.floatcolorptr );
   }
}



#if 0 /* Temporarily disabled as it is broken w/the new cubemap code. - idr */
struct dynfn *r200_makeX86TexCoord2fv( GLcontext *ctx, const int *key )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   return r200_makeX86Attribute2fv( & rmesa->vb.dfn_cache.TexCoord2fv, key,
				    __FUNCTION__, rmesa->vb.texcoordptr[0] );
}

struct dynfn *r200_makeX86TexCoord2f( GLcontext *ctx, const int *key )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   return r200_makeX86Attribute2f( & rmesa->vb.dfn_cache.TexCoord2f, key,
				   __FUNCTION__, rmesa->vb.texcoordptr[0] );
}

struct dynfn *r200_makeX86MultiTexCoord2fvARB( GLcontext *ctx, const int *key )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x 0x%08x\n", __FUNCTION__, key[0], key[1] );

   if (rmesa->vb.texcoordptr[1] == rmesa->vb.texcoordptr[0]+4) {
      DFN ( _x86_MultiTexCoord2fv, rmesa->vb.dfn_cache.MultiTexCoord2fvARB );
      FIXUP(dfn->code, 21, 0xdeadbeef, (int)rmesa->vb.texcoordptr[0]);	
      FIXUP(dfn->code, 27, 0xdeadbeef, (int)rmesa->vb.texcoordptr[0]+4);
   } else {
      DFN ( _x86_MultiTexCoord2fv_2, rmesa->vb.dfn_cache.MultiTexCoord2fvARB );
      FIXUP(dfn->code, 14, 0x0, (int)rmesa->vb.texcoordptr);
   }
   return dfn;
}

struct dynfn *r200_makeX86MultiTexCoord2fARB( GLcontext *ctx, 
					      const int *key )
{
   struct dynfn *dfn = MALLOC_STRUCT( dynfn );
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   if (R200_DEBUG & DEBUG_CODEGEN)
      fprintf(stderr, "%s 0x%08x 0x%08x\n", __FUNCTION__, key[0], key[1] );

   if (rmesa->vb.texcoordptr[1] == rmesa->vb.texcoordptr[0]+4) {
      DFN ( _x86_MultiTexCoord2f, rmesa->vb.dfn_cache.MultiTexCoord2fARB );
      FIXUP(dfn->code, 20, 0xdeadbeef, (int)rmesa->vb.texcoordptr[0]); 
      FIXUP(dfn->code, 26, 0xdeadbeef, (int)rmesa->vb.texcoordptr[0]+4); 
   }
   else {
      /* Note: this might get generated multiple times, even though the
       * actual emitted code is the same.
       */
      DFN ( _x86_MultiTexCoord2f_2, rmesa->vb.dfn_cache.MultiTexCoord2fARB );
      FIXUP(dfn->code, 18, 0x0, (int)rmesa->vb.texcoordptr); 
   }      
   return dfn;
}
#endif

void r200InitX86Codegen( struct dfn_generators *gen )
{
   gen->Vertex3f = r200_makeX86Vertex3f;
   gen->Vertex3fv = r200_makeX86Vertex3fv;
   gen->Color4ub = r200_makeX86Color4ub; /* PKCOLOR only */
   gen->Color4ubv = r200_makeX86Color4ubv; /* PKCOLOR only */
   gen->Normal3f = r200_makeX86Normal3f;
   gen->Normal3fv = r200_makeX86Normal3fv;
#if 0 /* Temporarily disabled as it is broken w/the new cubemap code. - idr */
   gen->TexCoord2f = r200_makeX86TexCoord2f;
   gen->TexCoord2fv = r200_makeX86TexCoord2fv;
   gen->MultiTexCoord2fARB = r200_makeX86MultiTexCoord2fARB;
   gen->MultiTexCoord2fvARB = r200_makeX86MultiTexCoord2fvARB;
#endif
   gen->Color3f = r200_makeX86Color3f;
   gen->Color3fv = r200_makeX86Color3fv;

   /* Not done:
    */
/*     gen->Vertex2f = r200_makeX86Vertex2f; */
/*     gen->Vertex2fv = r200_makeX86Vertex2fv; */
/*     gen->Color3ub = r200_makeX86Color3ub; */
/*     gen->Color3ubv = r200_makeX86Color3ubv; */
/*     gen->Color4f = r200_makeX86Color4f; */
/*     gen->Color4fv = r200_makeX86Color4fv; */
/*     gen->TexCoord1f = r200_makeX86TexCoord1f; */
/*     gen->TexCoord1fv = r200_makeX86TexCoord1fv; */
/*     gen->MultiTexCoord1fARB = r200_makeX86MultiTexCoord1fARB; */
/*     gen->MultiTexCoord1fvARB = r200_makeX86MultiTexCoord1fvARB; */
}


#else 

void r200InitX86Codegen( struct dfn_generators *gen )
{
   (void) gen;
}

#endif
