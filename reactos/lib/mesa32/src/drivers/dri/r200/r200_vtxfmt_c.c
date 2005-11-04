/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_vtxfmt_c.c,v 1.2 2002/12/16 16:18:56 dawes Exp $ */
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
#include "mtypes.h"
#include "colormac.h"
#include "simple_list.h"
#include "api_noop.h"
#include "vtxfmt.h"

#include "r200_vtxfmt.h"
#include "r200_tcl.h"

#include "dispatch.h"

/* Fallback versions of all the entrypoints for situations where
 * codegen isn't available.  This is still a lot faster than the
 * vb/pipeline implementation in Mesa.
 */
static void r200_Vertex3f( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   int i;

   *rmesa->vb.dmaptr++ = *(int *)&x;
   *rmesa->vb.dmaptr++ = *(int *)&y;
   *rmesa->vb.dmaptr++ = *(int *)&z;

   for (i = 3; i < rmesa->vb.vertex_size; i++)
      *rmesa->vb.dmaptr++ = rmesa->vb.vertex[i].i;
   
   if (--rmesa->vb.counter == 0)
      rmesa->vb.notify();
}


static void r200_Vertex3fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   int i;

   *rmesa->vb.dmaptr++ = *(int *)&v[0];
   *rmesa->vb.dmaptr++ = *(int *)&v[1];
   *rmesa->vb.dmaptr++ = *(int *)&v[2];

   for (i = 3; i < rmesa->vb.vertex_size; i++)
      *rmesa->vb.dmaptr++ = rmesa->vb.vertex[i].i;
   
   if (--rmesa->vb.counter == 0)
      rmesa->vb.notify();
}


static void r200_Vertex2f( GLfloat x, GLfloat y )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   int i;
   
   *rmesa->vb.dmaptr++ = *(int *)&x;
   *rmesa->vb.dmaptr++ = *(int *)&y;
   *rmesa->vb.dmaptr++ = 0;

   for (i = 3; i < rmesa->vb.vertex_size; i++)
      *rmesa->vb.dmaptr++ = rmesa->vb.vertex[i].i;

   if (--rmesa->vb.counter == 0)
      rmesa->vb.notify();
}


static void r200_Vertex2fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   int i;

   *rmesa->vb.dmaptr++ = *(int *)&v[0];
   *rmesa->vb.dmaptr++ = *(int *)&v[1];
   *rmesa->vb.dmaptr++ = 0;

   for (i = 3; i < rmesa->vb.vertex_size; i++)
      *rmesa->vb.dmaptr++ = rmesa->vb.vertex[i].i;
   
   if (--rmesa->vb.counter == 0)
      rmesa->vb.notify();
}



/* Color for ubyte (packed) color formats:
 */
#if 0
static void r200_Color3ub_ub( GLubyte r, GLubyte g, GLubyte b )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   r200_color_t *dest = rmesa->vb.colorptr;
   dest->red	= r;
   dest->green	= g;
   dest->blue	= b;
   dest->alpha	= 0xff;
}

static void r200_Color3ubv_ub( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   r200_color_t *dest = rmesa->vb.colorptr;
   dest->red	= v[0];
   dest->green	= v[1];
   dest->blue	= v[2];
   dest->alpha	= 0xff;
}

static void r200_Color4ub_ub( GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   r200_color_t *dest = rmesa->vb.colorptr;
   dest->red	= r;
   dest->green	= g;
   dest->blue	= b;
   dest->alpha	= a;
}

static void r200_Color4ubv_ub( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   *(GLuint *)rmesa->vb.colorptr = LE32_TO_CPU(*(GLuint *)v);
}
#endif /* 0 */

static void r200_Color3f_ub( GLfloat r, GLfloat g, GLfloat b )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   r200_color_t *dest = rmesa->vb.colorptr;
   UNCLAMPED_FLOAT_TO_UBYTE( dest->red,   r );
   UNCLAMPED_FLOAT_TO_UBYTE( dest->green, g );
   UNCLAMPED_FLOAT_TO_UBYTE( dest->blue,  b );
   dest->alpha = 255;
}

static void r200_Color3fv_ub( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   r200_color_t *dest = rmesa->vb.colorptr;
   UNCLAMPED_FLOAT_TO_UBYTE( dest->red,   v[0] );
   UNCLAMPED_FLOAT_TO_UBYTE( dest->green, v[1] );
   UNCLAMPED_FLOAT_TO_UBYTE( dest->blue,  v[2] );
   dest->alpha = 255;
}

static void r200_Color4f_ub( GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   r200_color_t *dest = rmesa->vb.colorptr;
   UNCLAMPED_FLOAT_TO_UBYTE( dest->red,   r );
   UNCLAMPED_FLOAT_TO_UBYTE( dest->green, g );
   UNCLAMPED_FLOAT_TO_UBYTE( dest->blue,  b );
   UNCLAMPED_FLOAT_TO_UBYTE( dest->alpha, a );
}

static void r200_Color4fv_ub( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   r200_color_t *dest = rmesa->vb.colorptr;
   UNCLAMPED_FLOAT_TO_UBYTE( dest->red,	  v[0] );
   UNCLAMPED_FLOAT_TO_UBYTE( dest->green, v[1] );
   UNCLAMPED_FLOAT_TO_UBYTE( dest->blue,  v[2] );
   UNCLAMPED_FLOAT_TO_UBYTE( dest->alpha, v[3] );
}


/* Color for float color+alpha formats:
 */
#if 0
static void r200_Color3ub_4f( GLubyte r, GLubyte g, GLubyte b )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = UBYTE_TO_FLOAT(r);
   dest[1] = UBYTE_TO_FLOAT(g);
   dest[2] = UBYTE_TO_FLOAT(b);
   dest[3] = 1.0;
}

static void r200_Color3ubv_4f( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = UBYTE_TO_FLOAT(v[0]);
   dest[1] = UBYTE_TO_FLOAT(v[1]);
   dest[2] = UBYTE_TO_FLOAT(v[2]);
   dest[3] = 1.0;
}

static void r200_Color4ub_4f( GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = UBYTE_TO_FLOAT(r);
   dest[1] = UBYTE_TO_FLOAT(g);
   dest[2] = UBYTE_TO_FLOAT(b);
   dest[3] = UBYTE_TO_FLOAT(a);
}

static void r200_Color4ubv_4f( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = UBYTE_TO_FLOAT(v[0]);
   dest[1] = UBYTE_TO_FLOAT(v[1]);
   dest[2] = UBYTE_TO_FLOAT(v[2]);
   dest[3] = UBYTE_TO_FLOAT(v[3]);
}
#endif /* 0 */


static void r200_Color3f_4f( GLfloat r, GLfloat g, GLfloat b )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = r;
   dest[1] = g;
   dest[2] = b;
   dest[3] = 1.0;		
}

static void r200_Color3fv_4f( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = v[0];
   dest[1] = v[1];
   dest[2] = v[2];
   dest[3] = 1.0;
}

static void r200_Color4f_4f( GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = r;
   dest[1] = g;
   dest[2] = b;
   dest[3] = a;
}

static void r200_Color4fv_4f( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = v[0];
   dest[1] = v[1];
   dest[2] = v[2];
   dest[3] = v[3];
}


/* Color for float color formats:
 */
#if 0
static void r200_Color3ub_3f( GLubyte r, GLubyte g, GLubyte b )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = UBYTE_TO_FLOAT(r);
   dest[1] = UBYTE_TO_FLOAT(g);
   dest[2] = UBYTE_TO_FLOAT(b);
}

static void r200_Color3ubv_3f( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = UBYTE_TO_FLOAT(v[0]);
   dest[1] = UBYTE_TO_FLOAT(v[1]);
   dest[2] = UBYTE_TO_FLOAT(v[2]);
}

static void r200_Color4ub_3f( GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = UBYTE_TO_FLOAT(r);
   dest[1] = UBYTE_TO_FLOAT(g);
   dest[2] = UBYTE_TO_FLOAT(b);
   ctx->Current.Attrib[VERT_ATTRIB_COLOR0][3] = UBYTE_TO_FLOAT(a);
}

static void r200_Color4ubv_3f( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = UBYTE_TO_FLOAT(v[0]);
   dest[1] = UBYTE_TO_FLOAT(v[1]);
   dest[2] = UBYTE_TO_FLOAT(v[2]);
   ctx->Current.Attrib[VERT_ATTRIB_COLOR0][3] = UBYTE_TO_FLOAT(v[3]);
}
#endif /* 0 */


static void r200_Color3f_3f( GLfloat r, GLfloat g, GLfloat b )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = r;
   dest[1] = g;
   dest[2] = b;
}

static void r200_Color3fv_3f( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = v[0];
   dest[1] = v[1];
   dest[2] = v[2];
}

static void r200_Color4f_3f( GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = r;
   dest[1] = g;
   dest[2] = b;
   ctx->Current.Attrib[VERT_ATTRIB_COLOR0][3] = a;
}

static void r200_Color4fv_3f( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatcolorptr;
   dest[0] = v[0];
   dest[1] = v[1];
   dest[2] = v[2];
   ctx->Current.Attrib[VERT_ATTRIB_COLOR0][3] = v[3]; 
}


/* Secondary Color:
 */
#if 0
static void r200_SecondaryColor3ubEXT_ub( GLubyte r, GLubyte g, GLubyte b )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   r200_color_t *dest = rmesa->vb.specptr;
   dest->red	= r;
   dest->green	= g;
   dest->blue	= b;
   dest->alpha	= 0xff;
}

static void r200_SecondaryColor3ubvEXT_ub( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   r200_color_t *dest = rmesa->vb.specptr;
   dest->red	= v[0];
   dest->green	= v[1];
   dest->blue	= v[2];
   dest->alpha	= 0xff;
}
#endif /* 0 */

static void r200_SecondaryColor3fEXT_ub( GLfloat r, GLfloat g, GLfloat b )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   r200_color_t *dest = rmesa->vb.specptr;
   UNCLAMPED_FLOAT_TO_UBYTE( dest->red,	  r );
   UNCLAMPED_FLOAT_TO_UBYTE( dest->green, g );
   UNCLAMPED_FLOAT_TO_UBYTE( dest->blue,  b );
   dest->alpha = 255;
}

static void r200_SecondaryColor3fvEXT_ub( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   r200_color_t *dest = rmesa->vb.specptr;
   UNCLAMPED_FLOAT_TO_UBYTE( dest->red,	  v[0] );
   UNCLAMPED_FLOAT_TO_UBYTE( dest->green, v[1] );
   UNCLAMPED_FLOAT_TO_UBYTE( dest->blue,  v[2] );
   dest->alpha = 255;
}

#if 0
static void r200_SecondaryColor3ubEXT_3f( GLubyte r, GLubyte g, GLubyte b )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatspecptr;
   dest[0] = UBYTE_TO_FLOAT(r);
   dest[1] = UBYTE_TO_FLOAT(g);
   dest[2] = UBYTE_TO_FLOAT(b);
   dest[3] = 1.0;
}

static void r200_SecondaryColor3ubvEXT_3f( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatspecptr;
   dest[0] = UBYTE_TO_FLOAT(v[0]);
   dest[1] = UBYTE_TO_FLOAT(v[1]);
   dest[2] = UBYTE_TO_FLOAT(v[2]);
   dest[3] = 1.0;
}
#endif /* 0 */

static void r200_SecondaryColor3fEXT_3f( GLfloat r, GLfloat g, GLfloat b )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatspecptr;
   dest[0] = r;
   dest[1] = g;
   dest[2] = b;
   dest[3] = 1.0;
}

static void r200_SecondaryColor3fvEXT_3f( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.floatspecptr;
   dest[0] = v[0];
   dest[1] = v[1];
   dest[2] = v[2];
   dest[3] = 1.0;
}



/* Normal
 */
static void r200_Normal3f( GLfloat n0, GLfloat n1, GLfloat n2 )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.normalptr;
   dest[0] = n0;
   dest[1] = n1;
   dest[2] = n2;
}

static void r200_Normal3fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.normalptr;
   dest[0] = v[0];
   dest[1] = v[1];
   dest[2] = v[2];
}


/* FogCoord
 */
static void r200_FogCoordfEXT( GLfloat f )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.fogptr;
   dest[0] = r200ComputeFogBlendFactor( ctx, f );
/*   ctx->Current.Attrib[VERT_ATTRIB_FOG][0] = f;*/
}

static void r200_FogCoordfvEXT( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *dest = rmesa->vb.fogptr;
   dest[0] = r200ComputeFogBlendFactor( ctx, v[0] );
/*   ctx->Current.Attrib[VERT_ATTRIB_FOG][0] = v[0];*/
}


/* TexCoord
 */

/* \todo maybe (target & 4 ? target & 5 : target & 3) is more save than (target & 7) */
static void r200_MultiTexCoord1fARB(GLenum target, GLfloat s)
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLint unit = (target & 7);
   GLfloat * const dest = rmesa->vb.texcoordptr[unit];

   switch( ctx->Texture.Unit[unit]._ReallyEnabled ) {
   case TEXTURE_CUBE_BIT:
   case TEXTURE_3D_BIT:
      dest[2] = 0.0;
      /* FALLTHROUGH */
   case TEXTURE_2D_BIT:
   case TEXTURE_RECT_BIT:
      dest[1] = 0.0;
      /* FALLTHROUGH */
   case TEXTURE_1D_BIT:
      dest[0] = s;
   }
}

static void r200_MultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t)
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLint unit = (target & 7);
   GLfloat * const dest = rmesa->vb.texcoordptr[unit];

   switch( ctx->Texture.Unit[unit]._ReallyEnabled ) {
   case TEXTURE_CUBE_BIT:
   case TEXTURE_3D_BIT:
      dest[2] = 0.0;
      /* FALLTHROUGH */
   case TEXTURE_2D_BIT:
   case TEXTURE_RECT_BIT:
      dest[1] = t;
      dest[0] = s;
      break;
   default:
      VFMT_FALLBACK(__FUNCTION__);
      CALL_MultiTexCoord2fARB(GET_DISPATCH(), (target, s, t));
      return;	
   }
}

static void r200_MultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLint unit = (target & 7);
   GLfloat * const dest = rmesa->vb.texcoordptr[unit];

   switch( ctx->Texture.Unit[unit]._ReallyEnabled ) {
   case TEXTURE_CUBE_BIT:
   case TEXTURE_3D_BIT:
      dest[2] = r;
      dest[1] = t;
      dest[0] = s;
      break;
   default:
      VFMT_FALLBACK(__FUNCTION__);
      CALL_MultiTexCoord3fARB(GET_DISPATCH(), (target, s, t, r));
      return;	
   }
}

static void r200_TexCoord1f(GLfloat s)
{
   r200_MultiTexCoord1fARB(GL_TEXTURE0, s);
}

static void r200_TexCoord2f(GLfloat s, GLfloat t)
{
   r200_MultiTexCoord2fARB(GL_TEXTURE0, s, t);
}

static void r200_TexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
   r200_MultiTexCoord3fARB(GL_TEXTURE0, s, t, r);
}

static void r200_TexCoord1fv(const GLfloat *v)
{
   r200_MultiTexCoord1fARB(GL_TEXTURE0, v[0]);
}

static void r200_TexCoord2fv(const GLfloat *v)
{
   r200_MultiTexCoord2fARB(GL_TEXTURE0, v[0], v[1]);
}

static void r200_TexCoord3fv(const GLfloat *v)
{
   r200_MultiTexCoord3fARB(GL_TEXTURE0, v[0], v[1], v[2]);
}

static void r200_MultiTexCoord1fvARB(GLenum target, const GLfloat *v)
{
   r200_MultiTexCoord1fARB(target, v[0]);
}

static void r200_MultiTexCoord2fvARB(GLenum target, const GLfloat *v)
{
   r200_MultiTexCoord2fARB(target, v[0], v[1]);
}

static void r200_MultiTexCoord3fvARB(GLenum target, const GLfloat *v)
{
   r200_MultiTexCoord3fARB(target, v[0], v[1], v[2]);
}


static struct dynfn *lookup( struct dynfn *l, const int *key )
{
   struct dynfn *f;

   foreach( f, l ) {
      if (f->key[0] == key[0] && f->key[1] == key[1]) 
	 return f;
   }

   return NULL;
}

/* Can't use the loopback template for this:
 */

#define CHOOSE(FN, FNTYPE, MASK0, MASK1, ARGS1, ARGS2 )			\
static void choose_##FN ARGS1						\
{									\
   GET_CURRENT_CONTEXT(ctx);						\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);				\
   int key[2];								\
   struct dynfn *dfn;							\
									\
   key[0] = rmesa->vb.vtxfmt_0 & MASK0;					\
   key[1] = rmesa->vb.vtxfmt_1 & MASK1;					\
									\
   dfn = lookup( &rmesa->vb.dfn_cache.FN, key );			\
   if (dfn == 0)							\
      dfn = rmesa->vb.codegen.FN( ctx, key );				\
   else if (R200_DEBUG & DEBUG_CODEGEN)					\
      fprintf(stderr, "%s -- cached codegen\n", __FUNCTION__ );		\
									\
   if (dfn)								\
      SET_ ## FN (ctx->Exec, (FNTYPE)(dfn->code));			\
   else {								\
      if (R200_DEBUG & DEBUG_CODEGEN)					\
	 fprintf(stderr, "%s -- generic version\n", __FUNCTION__ );	\
      SET_ ## FN (ctx->Exec, r200_##FN);				\
   }									\
									\
   ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;			\
   CALL_ ## FN (ctx->Exec, ARGS2);					\
}



/* For the _3f case, only allow one color function to be hooked in at
 * a time.  Eventually, use a similar mechanism to allow selecting the
 * color component of the vertex format based on client behaviour.  
 *
 * Note:  Perform these actions even if there is a codegen or cached 
 * codegen version of the chosen function.
 */
#define CHOOSE_COLOR(FN, FNTYPE, NR, MASK0, MASK1, ARGS1, ARGS2 )	\
static void choose_##FN ARGS1						\
{									\
   GET_CURRENT_CONTEXT(ctx);						\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);			\
   int key[2];								\
   struct dynfn *dfn;							\
									\
   key[0] = rmesa->vb.vtxfmt_0 & MASK0;					\
   key[1] = rmesa->vb.vtxfmt_1 & MASK1;					\
									\
   if (VTX_COLOR(rmesa->vb.vtxfmt_0,0) == R200_VTX_PK_RGBA) {		\
      SET_ ## FN (ctx->Exec, r200_##FN##_ub);				\
   }									\
   else if (VTX_COLOR(rmesa->vb.vtxfmt_0,0) == R200_VTX_FP_RGB) {	\
									\
      if (rmesa->vb.installed_color_3f_sz != NR) {			\
         rmesa->vb.installed_color_3f_sz = NR;				\
         if (NR == 3) ctx->Current.Attrib[VERT_ATTRIB_COLOR0][3] = 1.0;	\
         if (ctx->Driver.NeedFlush & FLUSH_UPDATE_CURRENT) {		\
            r200_copy_to_current( ctx );				\
            _mesa_install_exec_vtxfmt( ctx, &rmesa->vb.vtxfmt );	\
            CALL_ ## FN (ctx->Exec, ARGS2);				\
            return;							\
         }								\
      }									\
									\
      SET_ ## FN (ctx->Exec, r200_##FN##_3f);				\
   }									\
   else {								\
      SET_ ## FN (ctx->Exec, r200_##FN##_4f);				\
   }									\
									\
									\
   dfn = lookup( &rmesa->vb.dfn_cache.FN, key );			\
   if (!dfn) dfn = rmesa->vb.codegen.FN( ctx, key );			\
									\
   if (dfn) {								\
      if (R200_DEBUG & DEBUG_CODEGEN)					\
         fprintf(stderr, "%s -- codegen version\n", __FUNCTION__ );	\
      SET_ ## FN (ctx->Exec, (FNTYPE)dfn->code);			\
   }									\
   else if (R200_DEBUG & DEBUG_CODEGEN)					\
         fprintf(stderr, "%s -- 'c' version\n", __FUNCTION__ );		\
									\
   ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;			\
   CALL_ ## FN (ctx->Exec, ARGS2);					\
}



/* Right now there are both _ub and _3f versions of the secondary color
 * functions.  Currently, we only set-up the hardware to use the _ub versions.
 * The _3f versions are needed for the cases where secondary color isn't used
 * in the vertex format, but it still needs to be stored in the context
 * state vector.
 */
#define CHOOSE_SECONDARY_COLOR(FN, FNTYPE, MASK0, MASK1, ARGS1, ARGS2 )	\
static void choose_##FN ARGS1						\
{									\
   GET_CURRENT_CONTEXT(ctx);						\
   r200ContextPtr rmesa = R200_CONTEXT(ctx);			\
   int key[2];								\
   struct dynfn *dfn;							\
									\
   key[0] = rmesa->vb.vtxfmt_0 & MASK0;					\
   key[1] = rmesa->vb.vtxfmt_1 & MASK1;					\
									\
   dfn = lookup( &rmesa->vb.dfn_cache.FN, key );			\
   if (dfn == 0)							\
      dfn = rmesa->vb.codegen.FN( ctx, key );			\
   else  if (R200_DEBUG & DEBUG_CODEGEN)				\
      fprintf(stderr, "%s -- cached version\n", __FUNCTION__ );		\
									\
   if (dfn)								\
      SET_ ## FN (ctx->Exec, (FNTYPE)(dfn->code));			\
   else {								\
      if (R200_DEBUG & DEBUG_CODEGEN)					\
         fprintf(stderr, "%s -- generic version\n", __FUNCTION__ );	\
      SET_ ## FN (ctx->Exec, (VTX_COLOR(rmesa->vb.vtxfmt_0,1) == R200_VTX_PK_RGBA) \
	  ? r200_##FN##_ub : r200_##FN##_3f);				\
   }									\
									\
   ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;		\
   CALL_ ## FN (ctx->Exec, ARGS2);					\
}







/* VTXFMT_0
 */
#define MASK_XYZW  (R200_VTX_W0|R200_VTX_Z0)
#define MASK_NORM  (MASK_XYZW|R200_VTX_N0)
#define MASK_FOG   (MASK_NORM |R200_VTX_DISCRETE_FOG)
#define MASK_COLOR (MASK_FOG |(R200_VTX_COLOR_MASK<<R200_VTX_COLOR_0_SHIFT))
#define MASK_SPEC  (MASK_COLOR|(R200_VTX_COLOR_MASK<<R200_VTX_COLOR_1_SHIFT))

/* VTXFMT_1
 */
#define MASK_ST0 (0x7 << R200_VTX_TEX0_COMP_CNT_SHIFT)
/* FIXME: maybe something like in the radeon driver is needed here? */


typedef void (*p4f)( GLfloat, GLfloat, GLfloat, GLfloat );
typedef void (*p3f)( GLfloat, GLfloat, GLfloat );
typedef void (*p2f)( GLfloat, GLfloat );
typedef void (*p1f)( GLfloat );
typedef void (*pe3f)( GLenum, GLfloat, GLfloat, GLfloat );
typedef void (*pe2f)( GLenum, GLfloat, GLfloat );
typedef void (*pe1f)( GLenum, GLfloat );
typedef void (*p4ub)( GLubyte, GLubyte, GLubyte, GLubyte );
typedef void (*p3ub)( GLubyte, GLubyte, GLubyte );
typedef void (*pfv)( const GLfloat * );
typedef void (*pefv)( GLenum, const GLfloat * );
typedef void (*pubv)( const GLubyte * );


CHOOSE(Normal3f, p3f, MASK_NORM, 0, 
       (GLfloat a,GLfloat b,GLfloat c), (a,b,c))
CHOOSE(Normal3fv, pfv, MASK_NORM, 0, 
       (const GLfloat *v), (v))

#if 0
CHOOSE_COLOR(Color4ub, p4ub, 4, MASK_COLOR, 0,
	(GLubyte a,GLubyte b, GLubyte c, GLubyte d), (a,b,c,d))
CHOOSE_COLOR(Color4ubv, pubv, 4, MASK_COLOR, 0, 
	(const GLubyte *v), (v))
CHOOSE_COLOR(Color3ub, p3ub, 3, MASK_COLOR, 0, 
	(GLubyte a,GLubyte b, GLubyte c), (a,b,c))
CHOOSE_COLOR(Color3ubv, pubv, 3, MASK_COLOR, 0, 
	(const GLubyte *v), (v))
CHOOSE_SECONDARY_COLOR(SecondaryColor3ubEXT, p3ub, MASK_SPEC, 0, 
	(GLubyte a,GLubyte b, GLubyte c), (a,b,c))
CHOOSE_SECONDARY_COLOR(SecondaryColor3ubvEXT, pubv, MASK_SPEC, 0, 
	(const GLubyte *v), (v))
#endif

CHOOSE_COLOR(Color4f, p4f, 4, MASK_COLOR, 0, 
	(GLfloat a,GLfloat b, GLfloat c, GLfloat d), (a,b,c,d))
CHOOSE_COLOR(Color4fv, pfv, 4, MASK_COLOR, 0, 
	(const GLfloat *v), (v))
CHOOSE_COLOR(Color3f, p3f, 3, MASK_COLOR, 0,
	(GLfloat a,GLfloat b, GLfloat c), (a,b,c))
CHOOSE_COLOR(Color3fv, pfv, 3, MASK_COLOR, 0,
	(const GLfloat *v), (v))


CHOOSE_SECONDARY_COLOR(SecondaryColor3fEXT, p3f, MASK_SPEC, 0,
	(GLfloat a,GLfloat b, GLfloat c), (a,b,c))
CHOOSE_SECONDARY_COLOR(SecondaryColor3fvEXT, pfv, MASK_SPEC, 0,
	(const GLfloat *v), (v))

CHOOSE(TexCoord3f, p3f, ~0, MASK_ST0, 
       (GLfloat a,GLfloat b,GLfloat c), (a,b,c))
CHOOSE(TexCoord3fv, pfv, ~0, MASK_ST0, 
       (const GLfloat *v), (v))
CHOOSE(TexCoord2f, p2f, ~0, MASK_ST0, 
       (GLfloat a,GLfloat b), (a,b))
CHOOSE(TexCoord2fv, pfv, ~0, MASK_ST0, 
       (const GLfloat *v), (v))
CHOOSE(TexCoord1f, p1f, ~0, MASK_ST0, 
       (GLfloat a), (a))
CHOOSE(TexCoord1fv, pfv, ~0, MASK_ST0, 
       (const GLfloat *v), (v))

CHOOSE(MultiTexCoord3fARB, pe3f, ~0, ~0,
	 (GLenum u,GLfloat a,GLfloat b,GLfloat c), (u,a,b,c))
CHOOSE(MultiTexCoord3fvARB, pefv, ~0, ~0,
	(GLenum u,const GLfloat *v), (u,v))
CHOOSE(MultiTexCoord2fARB, pe2f, ~0, ~0,
	 (GLenum u,GLfloat a,GLfloat b), (u,a,b))
CHOOSE(MultiTexCoord2fvARB, pefv, ~0, ~0,
	(GLenum u,const GLfloat *v), (u,v))
CHOOSE(MultiTexCoord1fARB, pe1f, ~0, ~0,
	 (GLenum u,GLfloat a), (u,a))
CHOOSE(MultiTexCoord1fvARB, pefv, ~0, ~0,
	(GLenum u,const GLfloat *v), (u,v))

CHOOSE(Vertex3f, p3f, ~0, ~0, 
       (GLfloat a,GLfloat b,GLfloat c), (a,b,c))
CHOOSE(Vertex3fv, pfv, ~0, ~0, 
       (const GLfloat *v), (v))
CHOOSE(Vertex2f, p2f, ~0, ~0, 
       (GLfloat a,GLfloat b), (a,b))
CHOOSE(Vertex2fv, pfv, ~0, ~0, 
       (const GLfloat *v), (v))

CHOOSE(FogCoordfEXT, p1f, MASK_FOG, ~0, 
       (GLfloat f), (f))
CHOOSE(FogCoordfvEXT, pfv, MASK_FOG, ~0, 
       (const GLfloat *f), (f))




void r200VtxfmtInitChoosers( GLvertexformat *vfmt )
{
   vfmt->Color3f = choose_Color3f;
   vfmt->Color3fv = choose_Color3fv;
   vfmt->Color4f = choose_Color4f;
   vfmt->Color4fv = choose_Color4fv;
   vfmt->SecondaryColor3fEXT = choose_SecondaryColor3fEXT;
   vfmt->SecondaryColor3fvEXT = choose_SecondaryColor3fvEXT;
   vfmt->MultiTexCoord1fARB = choose_MultiTexCoord1fARB;
   vfmt->MultiTexCoord1fvARB = choose_MultiTexCoord1fvARB;
   vfmt->MultiTexCoord2fARB = choose_MultiTexCoord2fARB;
   vfmt->MultiTexCoord2fvARB = choose_MultiTexCoord2fvARB;
   vfmt->MultiTexCoord3fARB = choose_MultiTexCoord3fARB;
   vfmt->MultiTexCoord3fvARB = choose_MultiTexCoord3fvARB;
   vfmt->Normal3f = choose_Normal3f;
   vfmt->Normal3fv = choose_Normal3fv;
   vfmt->TexCoord1f = choose_TexCoord1f;
   vfmt->TexCoord1fv = choose_TexCoord1fv;
   vfmt->TexCoord2f = choose_TexCoord2f;
   vfmt->TexCoord2fv = choose_TexCoord2fv;
   vfmt->TexCoord3f = choose_TexCoord3f;
   vfmt->TexCoord3fv = choose_TexCoord3fv;
   vfmt->Vertex2f = choose_Vertex2f;
   vfmt->Vertex2fv = choose_Vertex2fv;
   vfmt->Vertex3f = choose_Vertex3f;
   vfmt->Vertex3fv = choose_Vertex3fv;
/*   vfmt->FogCoordfEXT = choose_FogCoordfEXT;
   vfmt->FogCoordfvEXT = choose_FogCoordfvEXT;*/

   /* TODO: restore ubyte colors to vtxfmt.
    */
#if 0
   vfmt->Color3ub = choose_Color3ub;
   vfmt->Color3ubv = choose_Color3ubv;
   vfmt->Color4ub = choose_Color4ub;
   vfmt->Color4ubv = choose_Color4ubv;
   vfmt->SecondaryColor3ubEXT = choose_SecondaryColor3ubEXT;
   vfmt->SecondaryColor3ubvEXT = choose_SecondaryColor3ubvEXT;
#endif
}


static struct dynfn *codegen_noop( GLcontext *ctx, const int *key )
{
   (void) ctx; (void) key;
   return NULL;
}

void r200InitCodegen( struct dfn_generators *gen, GLboolean useCodegen )
{
   gen->Vertex3f = codegen_noop;
   gen->Vertex3fv = codegen_noop;
   gen->Color4ub = codegen_noop;
   gen->Color4ubv = codegen_noop;
   gen->Normal3f = codegen_noop;
   gen->Normal3fv = codegen_noop;

   gen->TexCoord3f = codegen_noop;
   gen->TexCoord3fv = codegen_noop;
   gen->TexCoord2f = codegen_noop;
   gen->TexCoord2fv = codegen_noop;
   gen->TexCoord1f = codegen_noop;
   gen->TexCoord1fv = codegen_noop;

   gen->MultiTexCoord3fARB = codegen_noop;
   gen->MultiTexCoord3fvARB = codegen_noop;
   gen->MultiTexCoord2fARB = codegen_noop;
   gen->MultiTexCoord2fvARB = codegen_noop;
   gen->MultiTexCoord1fARB = codegen_noop;
   gen->MultiTexCoord1fvARB = codegen_noop;
/*   gen->FogCoordfEXT = codegen_noop;
   gen->FogCoordfvEXT = codegen_noop;*/

   gen->Vertex2f = codegen_noop;
   gen->Vertex2fv = codegen_noop;
   gen->Color3ub = codegen_noop;
   gen->Color3ubv = codegen_noop;
   gen->Color4f = codegen_noop;
   gen->Color4fv = codegen_noop;
   gen->Color3f = codegen_noop;
   gen->Color3fv = codegen_noop;
   gen->SecondaryColor3fEXT = codegen_noop;
   gen->SecondaryColor3fvEXT = codegen_noop;
   gen->SecondaryColor3ubEXT = codegen_noop;
   gen->SecondaryColor3ubvEXT = codegen_noop;

   if (useCodegen) {
#if defined(USE_X86_ASM)
      r200InitX86Codegen( gen );
#endif

#if defined(USE_SSE_ASM)
      r200InitSSECodegen( gen );
#endif
   }
}
