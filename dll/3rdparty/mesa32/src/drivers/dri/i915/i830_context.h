/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef I830CONTEXT_INC
#define I830CONTEXT_INC

#include "intel_context.h"

#define I830_FALLBACK_TEXTURE		 0x1000
#define I830_FALLBACK_COLORMASK		 0x2000
#define I830_FALLBACK_STENCIL		 0x4000
#define I830_FALLBACK_STIPPLE		 0x8000
#define I830_FALLBACK_LOGICOP		 0x10000

#define I830_UPLOAD_CTX              0x1
#define I830_UPLOAD_BUFFERS          0x2
#define I830_UPLOAD_STIPPLE          0x4
#define I830_UPLOAD_INVARIENT        0x8
#define I830_UPLOAD_TEX(i)           (0x10<<(i))
#define I830_UPLOAD_TEXBLEND(i)      (0x100<<(i))
#define I830_UPLOAD_TEX_ALL          (0x0f0)
#define I830_UPLOAD_TEXBLEND_ALL     (0xf00)

/* State structure offsets - these will probably disappear.
 */
#define I830_DESTREG_CBUFADDR0 0
#define I830_DESTREG_CBUFADDR1 1
#define I830_DESTREG_CBUFADDR2 2
#define I830_DESTREG_DBUFADDR0 3
#define I830_DESTREG_DBUFADDR1 4
#define I830_DESTREG_DBUFADDR2 5
#define I830_DESTREG_DV0 6
#define I830_DESTREG_DV1 7
#define I830_DESTREG_SENABLE 8
#define I830_DESTREG_SR0 9
#define I830_DESTREG_SR1 10
#define I830_DESTREG_SR2 11
#define I830_DEST_SETUP_SIZE 12

#define I830_CTXREG_STATE1		0
#define I830_CTXREG_STATE2		1
#define I830_CTXREG_STATE3		2
#define I830_CTXREG_STATE4		3
#define I830_CTXREG_STATE5		4
#define I830_CTXREG_IALPHAB		5
#define I830_CTXREG_STENCILTST		6
#define I830_CTXREG_ENABLES_1		7
#define I830_CTXREG_ENABLES_2		8
#define I830_CTXREG_AA			9
#define I830_CTXREG_FOGCOLOR		10
#define I830_CTXREG_BLENDCOLOR0		11
#define I830_CTXREG_BLENDCOLOR1		12 
#define I830_CTXREG_VF			13
#define I830_CTXREG_VF2			14
#define I830_CTXREG_MCSB0		15
#define I830_CTXREG_MCSB1		16
#define I830_CTX_SETUP_SIZE		17

#define I830_STPREG_ST0        0
#define I830_STPREG_ST1        1
#define I830_STP_SETUP_SIZE    2

#define I830_TEXREG_TM0LI      0 /* load immediate 2 texture map n */
#define I830_TEXREG_TM0S0      1
#define I830_TEXREG_TM0S1      2
#define I830_TEXREG_TM0S2      3
#define I830_TEXREG_TM0S3      4
#define I830_TEXREG_TM0S4      5
#define I830_TEXREG_MCS	       6	/* _3DSTATE_MAP_COORD_SETS */
#define I830_TEXREG_CUBE       7	/* _3DSTATE_MAP_SUBE */
#define I830_TEX_SETUP_SIZE    8

#define I830_TEXBLEND_SIZE	12	/* (4 args + op) * 2 + COLOR_FACTOR */

struct i830_texture_object
{
   struct intel_texture_object intel;
   GLuint Setup[I830_TEX_SETUP_SIZE];
};

#define I830_TEX_UNITS 4

struct i830_hw_state {
   GLuint Ctx[I830_CTX_SETUP_SIZE];
   GLuint Buffer[I830_DEST_SETUP_SIZE];
   GLuint Stipple[I830_STP_SETUP_SIZE];
   GLuint Tex[I830_TEX_UNITS][I830_TEX_SETUP_SIZE];
   GLuint TexBlend[I830_TEX_UNITS][I830_TEXBLEND_SIZE];
   GLuint TexBlendWordsUsed[I830_TEX_UNITS];
   GLuint emitted;		/* I810_UPLOAD_* */
   GLuint active;
};

struct i830_context 
{
   struct intel_context intel;
   
   DECLARE_RENDERINPUTS(last_index_bitset);

   struct i830_hw_state meta, initial, state, *current;
};

typedef struct i830_context *i830ContextPtr;
typedef struct i830_texture_object *i830TextureObjectPtr;

#define I830_CONTEXT(ctx)	((i830ContextPtr)(ctx))



#define I830_STATECHANGE(i830, flag)				\
do {								\
   INTEL_FIREVERTICES( &i830->intel );				\
   i830->state.emitted &= ~flag;					\
} while (0)

#define I830_ACTIVESTATE(i830, flag, mode)	\
do {						\
   INTEL_FIREVERTICES( &i830->intel );		\
   if (mode)					\
      i830->state.active |= flag;		\
   else						\
      i830->state.active &= ~flag;		\
} while (0)

/* i830_vtbl.c
 */
extern void 
i830InitVtbl( i830ContextPtr i830 );

/* i830_context.c
 */
extern GLboolean 
i830CreateContext( const __GLcontextModes *mesaVis,
		   __DRIcontextPrivate *driContextPriv,
		   void *sharedContextPrivate);

/* i830_tex.c, i830_texstate.c
 */
extern void 
i830UpdateTextureState( intelContextPtr intel );

extern void 
i830InitTextureFuncs( struct dd_function_table *functions );

extern intelTextureObjectPtr
i830AllocTexObj( struct gl_texture_object *tObj );

/* i830_texblend.c
 */
extern GLuint i830SetTexEnvCombine(i830ContextPtr i830,
    const struct gl_tex_env_combine_state * combine, GLint blendUnit,
     GLuint texel_op, GLuint *state, const GLfloat *factor );

extern void 
i830EmitTextureBlend( i830ContextPtr i830 );


/* i830_state.c
 */
extern void 
i830InitStateFuncs( struct dd_function_table *functions );

extern void 
i830EmitState( i830ContextPtr i830 );

extern void 
i830InitState( i830ContextPtr i830 );

/* i830_metaops.c
 */
extern GLboolean
i830TryTextureReadPixels( GLcontext *ctx,
			  GLint x, GLint y, GLsizei width, GLsizei height,
			  GLenum format, GLenum type,
			  const struct gl_pixelstore_attrib *pack,
			  GLvoid *pixels );

extern GLboolean
i830TryTextureDrawPixels( GLcontext *ctx,
			  GLint x, GLint y, GLsizei width, GLsizei height,
			  GLenum format, GLenum type,
			  const struct gl_pixelstore_attrib *unpack,
			  const GLvoid *pixels );

extern void 
i830ClearWithTris( intelContextPtr intel, GLbitfield mask,
		   GLboolean all, GLint cx, GLint cy, GLint cw, GLint ch);

extern void
i830RotateWindow(intelContextPtr intel, __DRIdrawablePrivate *dPriv,
                 GLuint srcBuf);

#endif

