/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_texstate.c,v 1.6 2002/12/16 16:18:59 dawes Exp $ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

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
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 */

#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "texformat.h"
#include "enums.h"

#include "radeon_context.h"
#include "radeon_state.h"
#include "radeon_ioctl.h"
#include "radeon_swtcl.h"
#include "radeon_tex.h"
#include "radeon_tcl.h"


#define RADEON_TXFORMAT_A8        RADEON_TXFORMAT_I8
#define RADEON_TXFORMAT_L8        RADEON_TXFORMAT_I8
#define RADEON_TXFORMAT_AL88      RADEON_TXFORMAT_AI88
#define RADEON_TXFORMAT_YCBCR     RADEON_TXFORMAT_YVYU422
#define RADEON_TXFORMAT_YCBCR_REV RADEON_TXFORMAT_VYUY422
#define RADEON_TXFORMAT_RGB_DXT1  RADEON_TXFORMAT_DXT1
#define RADEON_TXFORMAT_RGBA_DXT1 RADEON_TXFORMAT_DXT1
#define RADEON_TXFORMAT_RGBA_DXT3 RADEON_TXFORMAT_DXT23
#define RADEON_TXFORMAT_RGBA_DXT5 RADEON_TXFORMAT_DXT45

#define _COLOR(f) \
    [ MESA_FORMAT_ ## f ] = { RADEON_TXFORMAT_ ## f, 0 }
#define _COLOR_REV(f) \
    [ MESA_FORMAT_ ## f ## _REV ] = { RADEON_TXFORMAT_ ## f, 0 }
#define _ALPHA(f) \
    [ MESA_FORMAT_ ## f ] = { RADEON_TXFORMAT_ ## f | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 }
#define _ALPHA_REV(f) \
    [ MESA_FORMAT_ ## f ## _REV ] = { RADEON_TXFORMAT_ ## f | RADEON_TXFORMAT_ALPHA_IN_MAP, 0 }
#define _YUV(f) \
   [ MESA_FORMAT_ ## f ] = { RADEON_TXFORMAT_ ## f, RADEON_YUV_TO_RGB }
#define _INVALID(f) \
    [ MESA_FORMAT_ ## f ] = { 0xffffffff, 0 }
#define VALID_FORMAT(f) ( ((f) <= MESA_FORMAT_RGBA_DXT5) \
			     && (tx_table[f].format != 0xffffffff) )

static const struct {
   GLuint format, filter;
}
tx_table[] =
{
   _ALPHA(RGBA8888),
   _ALPHA_REV(RGBA8888),
   _ALPHA(ARGB8888),
   _ALPHA_REV(ARGB8888),
   _INVALID(RGB888),
   _COLOR(RGB565),
   _COLOR_REV(RGB565),
   _ALPHA(ARGB4444),
   _ALPHA_REV(ARGB4444),
   _ALPHA(ARGB1555),
   _ALPHA_REV(ARGB1555),
   _ALPHA(AL88),
   _ALPHA_REV(AL88),
   _ALPHA(A8),
   _COLOR(L8),
   _ALPHA(I8),
   _INVALID(CI8),
   _YUV(YCBCR),
   _YUV(YCBCR_REV),
   _INVALID(RGB_FXT1),
   _INVALID(RGBA_FXT1),
   _COLOR(RGB_DXT1),
   _ALPHA(RGBA_DXT1),
   _ALPHA(RGBA_DXT3),
   _ALPHA(RGBA_DXT5),
};

#undef _COLOR
#undef _ALPHA
#undef _INVALID

/**
 * This function computes the number of bytes of storage needed for
 * the given texture object (all mipmap levels, all cube faces).
 * The \c image[face][level].x/y/width/height parameters for upload/blitting
 * are computed here.  \c pp_txfilter, \c pp_txformat, etc. will be set here
 * too.
 * 
 * \param rmesa Context pointer
 * \param tObj GL texture object whose images are to be posted to
 *                 hardware state.
 */
static void radeonSetTexImages( radeonContextPtr rmesa,
				struct gl_texture_object *tObj )
{
   radeonTexObjPtr t = (radeonTexObjPtr)tObj->DriverData;
   const struct gl_texture_image *baseImage = tObj->Image[0][tObj->BaseLevel];
   GLint curOffset, blitWidth;
   GLint i, texelBytes;
   GLint numLevels;
   GLint log2Width, log2Height, log2Depth;

   /* Set the hardware texture format
    */

   t->pp_txformat &= ~(RADEON_TXFORMAT_FORMAT_MASK |
		       RADEON_TXFORMAT_ALPHA_IN_MAP);
   t->pp_txfilter &= ~RADEON_YUV_TO_RGB;

   if ( VALID_FORMAT( baseImage->TexFormat->MesaFormat ) ) {
      t->pp_txformat |= tx_table[ baseImage->TexFormat->MesaFormat ].format;
      t->pp_txfilter |= tx_table[ baseImage->TexFormat->MesaFormat ].filter;
   }
   else {
      _mesa_problem(NULL, "unexpected texture format in %s", __FUNCTION__);
      return;
   }

   texelBytes = baseImage->TexFormat->TexelBytes;

   /* Compute which mipmap levels we really want to send to the hardware.
    */

   if (tObj->Target != GL_TEXTURE_CUBE_MAP)
      driCalculateTextureFirstLastLevel( (driTextureObject *) t );
   else {
      /* r100 can't handle mipmaps for cube/3d textures, so don't waste
         memory for them */
      t->base.firstLevel = t->base.lastLevel = tObj->BaseLevel;
   }
   log2Width  = tObj->Image[0][t->base.firstLevel]->WidthLog2;
   log2Height = tObj->Image[0][t->base.firstLevel]->HeightLog2;
   log2Depth  = tObj->Image[0][t->base.firstLevel]->DepthLog2;

   numLevels = t->base.lastLevel - t->base.firstLevel + 1;

   assert(numLevels <= RADEON_MAX_TEXTURE_LEVELS);

   /* Calculate mipmap offsets and dimensions for blitting (uploading)
    * The idea is that we lay out the mipmap levels within a block of
    * memory organized as a rectangle of width BLIT_WIDTH_BYTES.
    */
   curOffset = 0;
   blitWidth = BLIT_WIDTH_BYTES;
   t->tile_bits = 0;

   /* figure out if this texture is suitable for tiling. */
   if (texelBytes && (tObj->Target != GL_TEXTURE_RECTANGLE_NV)) {
      if (rmesa->texmicrotile && (baseImage->Height > 1)) {
	 /* allow 32 (bytes) x 1 mip (which will use two times the space
	    the non-tiled version would use) max if base texture is large enough */
	 if ((numLevels == 1) ||
	   (((baseImage->Width * texelBytes / baseImage->Height) <= 32) &&
	       (baseImage->Width * texelBytes > 64)) ||
	    ((baseImage->Width * texelBytes / baseImage->Height) <= 16)) {
	    /* R100 has two microtile bits (only the txoffset reg, not the blitter)
	       weird: X2 + OPT: 32bit correct, 16bit completely hosed
		      X2: 32bit correct, 16bit correct
		      OPT: 32bit large mips correct, small mips hosed, 16bit completely hosed */
	    t->tile_bits |= RADEON_TXO_MICRO_TILE_X2 /*| RADEON_TXO_MICRO_TILE_OPT*/;
	 }
      }
      if ((baseImage->Width * texelBytes >= 256) && (baseImage->Height >= 16)) {
	 /* R100 disables macro tiling only if mip width is smaller than 256 bytes, and not
	    in the case if height is smaller than 16 (not 100% sure), as does the r200,
	    so need to disable macro tiling in that case */
	 if ((numLevels == 1) || ((baseImage->Width * texelBytes / baseImage->Height) <= 4)) {
	    t->tile_bits |= RADEON_TXO_MACRO_TILE;
	 }
      }
   }

   for (i = 0; i < numLevels; i++) {
      const struct gl_texture_image *texImage;
      GLuint size;

      texImage = tObj->Image[0][i + t->base.firstLevel];
      if ( !texImage )
	 break;

      /* find image size in bytes */
      if (texImage->IsCompressed) {
      /* need to calculate the size AFTER padding even though the texture is
         submitted without padding.
         Only handle pot textures currently - don't know if npot is even possible,
         size calculation would certainly need (trivial) adjustments.
         Align (and later pad) to 32byte, not sure what that 64byte blit width is
         good for? */
         if ((t->pp_txformat & RADEON_TXFORMAT_FORMAT_MASK) == RADEON_TXFORMAT_DXT1) {
            /* RGB_DXT1/RGBA_DXT1, 8 bytes per block */
            if ((texImage->Width + 3) < 8) /* width one block */
               size = texImage->CompressedSize * 4;
            else if ((texImage->Width + 3) < 16)
               size = texImage->CompressedSize * 2;
            else size = texImage->CompressedSize;
         }
         else /* DXT3/5, 16 bytes per block */
            if ((texImage->Width + 3) < 8)
               size = texImage->CompressedSize * 2;
            else size = texImage->CompressedSize;
      }
      else if (tObj->Target == GL_TEXTURE_RECTANGLE_NV) {
	 size = ((texImage->Width * texelBytes + 63) & ~63) * texImage->Height;
      }
      else if (t->tile_bits & RADEON_TXO_MICRO_TILE_X2) {
	 /* tile pattern is 16 bytes x2. mipmaps stay 32 byte aligned,
	    though the actual offset may be different (if texture is less than
	    32 bytes width) to the untiled case */
	 int w = (texImage->Width * texelBytes * 2 + 31) & ~31;
	 size = (w * ((texImage->Height + 1) / 2)) * texImage->Depth;
	 blitWidth = MAX2(texImage->Width, 64 / texelBytes);
      }
      else {
	 int w = (texImage->Width * texelBytes + 31) & ~31;
	 size = w * texImage->Height * texImage->Depth;
	 blitWidth = MAX2(texImage->Width, 64 / texelBytes);
      }
      assert(size > 0);

      /* Align to 32-byte offset.  It is faster to do this unconditionally
       * (no branch penalty).
       */

      curOffset = (curOffset + 0x1f) & ~0x1f;

      if (texelBytes) {
	 t->image[0][i].x = curOffset; /* fix x and y coords up later together with offset */
	 t->image[0][i].y = 0;
	 t->image[0][i].width = MIN2(size / texelBytes, blitWidth);
	 t->image[0][i].height = (size / texelBytes) / t->image[0][i].width;
      }
      else {
         t->image[0][i].x = curOffset % BLIT_WIDTH_BYTES;
         t->image[0][i].y = curOffset / BLIT_WIDTH_BYTES;
         t->image[0][i].width  = MIN2(size, BLIT_WIDTH_BYTES);
         t->image[0][i].height = size / t->image[0][i].width;     
      }

#if 0
      /* for debugging only and only  applicable to non-rectangle targets */
      assert(size % t->image[0][i].width == 0);
      assert(t->image[0][i].x == 0
             || (size < BLIT_WIDTH_BYTES && t->image[0][i].height == 1));
#endif

      if (0)
         fprintf(stderr,
                 "level %d: %dx%d x=%d y=%d w=%d h=%d size=%d at %d\n",
                 i, texImage->Width, texImage->Height,
                 t->image[0][i].x, t->image[0][i].y,
                 t->image[0][i].width, t->image[0][i].height, size, curOffset);

      curOffset += size;

   }

   /* Align the total size of texture memory block.
    */
   t->base.totalSize = (curOffset + RADEON_OFFSET_MASK) & ~RADEON_OFFSET_MASK;

   /* Setup remaining cube face blits, if needed */
   if (tObj->Target == GL_TEXTURE_CUBE_MAP) {
      const GLuint faceSize = t->base.totalSize;
      GLuint face;
      /* reuse face 0 x/y/width/height - just update the offset when uploading */
      for (face = 1; face < 6; face++) {
         for (i = 0; i < numLevels; i++) {
            t->image[face][i].x =  t->image[0][i].x;
            t->image[face][i].y =  t->image[0][i].y;
            t->image[face][i].width  = t->image[0][i].width;
            t->image[face][i].height = t->image[0][i].height;
         }
      }
      t->base.totalSize = 6 * faceSize; /* total texmem needed */
   }

   /* Hardware state:
    */
   t->pp_txfilter &= ~RADEON_MAX_MIP_LEVEL_MASK;
   t->pp_txfilter |= (numLevels - 1) << RADEON_MAX_MIP_LEVEL_SHIFT;

   t->pp_txformat &= ~(RADEON_TXFORMAT_WIDTH_MASK |
		       RADEON_TXFORMAT_HEIGHT_MASK |
                       RADEON_TXFORMAT_CUBIC_MAP_ENABLE |
                       RADEON_TXFORMAT_F5_WIDTH_MASK |
                       RADEON_TXFORMAT_F5_HEIGHT_MASK);
   t->pp_txformat |= ((log2Width << RADEON_TXFORMAT_WIDTH_SHIFT) |
		      (log2Height << RADEON_TXFORMAT_HEIGHT_SHIFT));

   if (tObj->Target == GL_TEXTURE_CUBE_MAP) {
      assert(log2Width == log2Height);
      t->pp_txformat |= ((log2Width << RADEON_TXFORMAT_F5_WIDTH_SHIFT) |
                         (log2Height << RADEON_TXFORMAT_F5_HEIGHT_SHIFT) |
                         (RADEON_TXFORMAT_CUBIC_MAP_ENABLE));
      t->pp_cubic_faces = ((log2Width << RADEON_FACE_WIDTH_1_SHIFT) |
                           (log2Height << RADEON_FACE_HEIGHT_1_SHIFT) |
                           (log2Width << RADEON_FACE_WIDTH_2_SHIFT) |
                           (log2Height << RADEON_FACE_HEIGHT_2_SHIFT) |
                           (log2Width << RADEON_FACE_WIDTH_3_SHIFT) |
                           (log2Height << RADEON_FACE_HEIGHT_3_SHIFT) |
                           (log2Width << RADEON_FACE_WIDTH_4_SHIFT) |
                           (log2Height << RADEON_FACE_HEIGHT_4_SHIFT));
   }

   t->pp_txsize = (((tObj->Image[0][t->base.firstLevel]->Width - 1) << 0) |
                   ((tObj->Image[0][t->base.firstLevel]->Height - 1) << 16));

   /* Only need to round to nearest 32 for textures, but the blitter
    * requires 64-byte aligned pitches, and we may/may not need the
    * blitter.   NPOT only!
    */
   if (baseImage->IsCompressed)
      t->pp_txpitch = (tObj->Image[0][t->base.firstLevel]->Width + 63) & ~(63);
   else
      t->pp_txpitch = ((tObj->Image[0][t->base.firstLevel]->Width * texelBytes) + 63) & ~(63);
   t->pp_txpitch -= 32;

   t->dirty_state = TEX_ALL;

   /* FYI: radeonUploadTexImages( rmesa, t ); used to be called here */
}



/* ================================================================
 * Texture combine functions
 */

/* GL_ARB_texture_env_combine support
 */

/* The color tables have combine functions for GL_SRC_COLOR,
 * GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA and GL_ONE_MINUS_SRC_ALPHA.
 */
static GLuint radeon_texture_color[][RADEON_MAX_TEXTURE_UNITS] =
{
   {
      RADEON_COLOR_ARG_A_T0_COLOR,
      RADEON_COLOR_ARG_A_T1_COLOR,
      RADEON_COLOR_ARG_A_T2_COLOR
   },
   {
      RADEON_COLOR_ARG_A_T0_COLOR | RADEON_COMP_ARG_A,
      RADEON_COLOR_ARG_A_T1_COLOR | RADEON_COMP_ARG_A,
      RADEON_COLOR_ARG_A_T2_COLOR | RADEON_COMP_ARG_A
   },
   {
      RADEON_COLOR_ARG_A_T0_ALPHA,
      RADEON_COLOR_ARG_A_T1_ALPHA,
      RADEON_COLOR_ARG_A_T2_ALPHA
   },
   {
      RADEON_COLOR_ARG_A_T0_ALPHA | RADEON_COMP_ARG_A,
      RADEON_COLOR_ARG_A_T1_ALPHA | RADEON_COMP_ARG_A,
      RADEON_COLOR_ARG_A_T2_ALPHA | RADEON_COMP_ARG_A
   },
};

static GLuint radeon_tfactor_color[] =
{
   RADEON_COLOR_ARG_A_TFACTOR_COLOR,
   RADEON_COLOR_ARG_A_TFACTOR_COLOR | RADEON_COMP_ARG_A,
   RADEON_COLOR_ARG_A_TFACTOR_ALPHA,
   RADEON_COLOR_ARG_A_TFACTOR_ALPHA | RADEON_COMP_ARG_A
};

static GLuint radeon_primary_color[] =
{
   RADEON_COLOR_ARG_A_DIFFUSE_COLOR,
   RADEON_COLOR_ARG_A_DIFFUSE_COLOR | RADEON_COMP_ARG_A,
   RADEON_COLOR_ARG_A_DIFFUSE_ALPHA,
   RADEON_COLOR_ARG_A_DIFFUSE_ALPHA | RADEON_COMP_ARG_A
};

static GLuint radeon_previous_color[] =
{
   RADEON_COLOR_ARG_A_CURRENT_COLOR,
   RADEON_COLOR_ARG_A_CURRENT_COLOR | RADEON_COMP_ARG_A,
   RADEON_COLOR_ARG_A_CURRENT_ALPHA,
   RADEON_COLOR_ARG_A_CURRENT_ALPHA | RADEON_COMP_ARG_A
};

/* GL_ZERO table - indices 0-3
 * GL_ONE  table - indices 1-4
 */
static GLuint radeon_zero_color[] =
{
   RADEON_COLOR_ARG_A_ZERO,
   RADEON_COLOR_ARG_A_ZERO | RADEON_COMP_ARG_A,
   RADEON_COLOR_ARG_A_ZERO,
   RADEON_COLOR_ARG_A_ZERO | RADEON_COMP_ARG_A,
   RADEON_COLOR_ARG_A_ZERO
};


/* The alpha tables only have GL_SRC_ALPHA and GL_ONE_MINUS_SRC_ALPHA.
 */
static GLuint radeon_texture_alpha[][RADEON_MAX_TEXTURE_UNITS] =
{
   {
      RADEON_ALPHA_ARG_A_T0_ALPHA,
      RADEON_ALPHA_ARG_A_T1_ALPHA,
      RADEON_ALPHA_ARG_A_T2_ALPHA
   },
   {
      RADEON_ALPHA_ARG_A_T0_ALPHA | RADEON_COMP_ARG_A,
      RADEON_ALPHA_ARG_A_T1_ALPHA | RADEON_COMP_ARG_A,
      RADEON_ALPHA_ARG_A_T2_ALPHA | RADEON_COMP_ARG_A
   },
};

static GLuint radeon_tfactor_alpha[] =
{
   RADEON_ALPHA_ARG_A_TFACTOR_ALPHA,
   RADEON_ALPHA_ARG_A_TFACTOR_ALPHA | RADEON_COMP_ARG_A
};

static GLuint radeon_primary_alpha[] =
{
   RADEON_ALPHA_ARG_A_DIFFUSE_ALPHA,
   RADEON_ALPHA_ARG_A_DIFFUSE_ALPHA | RADEON_COMP_ARG_A
};

static GLuint radeon_previous_alpha[] =
{
   RADEON_ALPHA_ARG_A_CURRENT_ALPHA,
   RADEON_ALPHA_ARG_A_CURRENT_ALPHA | RADEON_COMP_ARG_A
};

/* GL_ZERO table - indices 0-1
 * GL_ONE  table - indices 1-2
 */
static GLuint radeon_zero_alpha[] =
{
   RADEON_ALPHA_ARG_A_ZERO,
   RADEON_ALPHA_ARG_A_ZERO | RADEON_COMP_ARG_A,
   RADEON_ALPHA_ARG_A_ZERO
};


/* Extract the arg from slot A, shift it into the correct argument slot
 * and set the corresponding complement bit.
 */
#define RADEON_COLOR_ARG( n, arg )			\
do {							\
   color_combine |=					\
      ((color_arg[n] & RADEON_COLOR_ARG_MASK)		\
       << RADEON_COLOR_ARG_##arg##_SHIFT);		\
   color_combine |=					\
      ((color_arg[n] >> RADEON_COMP_ARG_SHIFT)		\
       << RADEON_COMP_ARG_##arg##_SHIFT);		\
} while (0)

#define RADEON_ALPHA_ARG( n, arg )			\
do {							\
   alpha_combine |=					\
      ((alpha_arg[n] & RADEON_ALPHA_ARG_MASK)		\
       << RADEON_ALPHA_ARG_##arg##_SHIFT);		\
   alpha_combine |=					\
      ((alpha_arg[n] >> RADEON_COMP_ARG_SHIFT)		\
       << RADEON_COMP_ARG_##arg##_SHIFT);		\
} while (0)


/* ================================================================
 * Texture unit state management
 */

static GLboolean radeonUpdateTextureEnv( GLcontext *ctx, int unit )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   GLuint color_combine, alpha_combine;
   const GLuint color_combine0 = RADEON_COLOR_ARG_A_ZERO | RADEON_COLOR_ARG_B_ZERO
         | RADEON_COLOR_ARG_C_CURRENT_COLOR | RADEON_BLEND_CTL_ADD
         | RADEON_SCALE_1X | RADEON_CLAMP_TX;
   const GLuint alpha_combine0 = RADEON_ALPHA_ARG_A_ZERO | RADEON_ALPHA_ARG_B_ZERO
         | RADEON_ALPHA_ARG_C_CURRENT_ALPHA | RADEON_BLEND_CTL_ADD
         | RADEON_SCALE_1X | RADEON_CLAMP_TX;


   /* texUnit->_Current can be NULL if and only if the texture unit is
    * not actually enabled.
    */
   assert( (texUnit->_ReallyEnabled == 0)
	   || (texUnit->_Current != NULL) );

   if ( RADEON_DEBUG & DEBUG_TEXTURE ) {
      fprintf( stderr, "%s( %p, %d )\n", __FUNCTION__, (void *)ctx, unit );
   }

   /* Set the texture environment state.  Isn't this nice and clean?
    * The chip will automagically set the texture alpha to 0xff when
    * the texture format does not include an alpha component. This
    * reduces the amount of special-casing we have to do, alpha-only
    * textures being a notable exception. Doesn't work for luminance
    * textures realized with I8 and ALPHA_IN_MAP not set neither (on r100).
    */
    /* Don't cache these results.
    */
   rmesa->state.texture.unit[unit].format = 0;
   rmesa->state.texture.unit[unit].envMode = 0;

   if ( !texUnit->_ReallyEnabled ) {
      color_combine = color_combine0;
      alpha_combine = alpha_combine0;
   }
   else {
      GLuint color_arg[3], alpha_arg[3];
      GLuint i;
      const GLuint numColorArgs = texUnit->_CurrentCombine->_NumArgsRGB;
      const GLuint numAlphaArgs = texUnit->_CurrentCombine->_NumArgsA;
      GLuint RGBshift = texUnit->_CurrentCombine->ScaleShiftRGB;
      GLuint Ashift = texUnit->_CurrentCombine->ScaleShiftA;


      /* Step 1:
       * Extract the color and alpha combine function arguments.
       */
      for ( i = 0 ; i < numColorArgs ; i++ ) {
	 const GLint op = texUnit->_CurrentCombine->OperandRGB[i] - GL_SRC_COLOR;
	 const GLuint srcRGBi = texUnit->_CurrentCombine->SourceRGB[i];
	 assert(op >= 0);
	 assert(op <= 3);
	 switch ( srcRGBi ) {
	 case GL_TEXTURE:
	    if (texUnit->_Current->Image[0][0]->_BaseFormat == GL_ALPHA)
	       color_arg[i] = radeon_zero_color[op];
	    else
	       color_arg[i] = radeon_texture_color[op][unit];
	    break;
	 case GL_CONSTANT:
	    color_arg[i] = radeon_tfactor_color[op];
	    break;
	 case GL_PRIMARY_COLOR:
	    color_arg[i] = radeon_primary_color[op];
	    break;
	 case GL_PREVIOUS:
	    color_arg[i] = radeon_previous_color[op];
	    break;
	 case GL_ZERO:
	    color_arg[i] = radeon_zero_color[op];
	    break;
	 case GL_ONE:
	    color_arg[i] = radeon_zero_color[op+1];
	    break;
	 case GL_TEXTURE0:
	 case GL_TEXTURE1:
	 case GL_TEXTURE2: {
	    GLuint txunit = srcRGBi - GL_TEXTURE0;
	    if (ctx->Texture.Unit[txunit]._Current->Image[0][0]->_BaseFormat == GL_ALPHA)
	       color_arg[i] = radeon_zero_color[op];
	    else
	 /* implement ogl 1.4/1.5 core spec here, not specification of
	  * GL_ARB_texture_env_crossbar (which would require disabling blending
	  * instead of undefined results when referencing not enabled texunit) */
	      color_arg[i] = radeon_texture_color[op][txunit];
	    }
	    break;
	 default:
	    return GL_FALSE;
	 }
      }

      for ( i = 0 ; i < numAlphaArgs ; i++ ) {
	 const GLint op = texUnit->_CurrentCombine->OperandA[i] - GL_SRC_ALPHA;
	 const GLuint srcAi = texUnit->_CurrentCombine->SourceA[i];
	 assert(op >= 0);
	 assert(op <= 1);
	 switch ( srcAi ) {
	 case GL_TEXTURE:
	    if (texUnit->_Current->Image[0][0]->_BaseFormat == GL_LUMINANCE)
	       alpha_arg[i] = radeon_zero_alpha[op+1];
	    else
	       alpha_arg[i] = radeon_texture_alpha[op][unit];
	    break;
	 case GL_CONSTANT:
	    alpha_arg[i] = radeon_tfactor_alpha[op];
	    break;
	 case GL_PRIMARY_COLOR:
	    alpha_arg[i] = radeon_primary_alpha[op];
	    break;
	 case GL_PREVIOUS:
	    alpha_arg[i] = radeon_previous_alpha[op];
	    break;
	 case GL_ZERO:
	    alpha_arg[i] = radeon_zero_alpha[op];
	    break;
	 case GL_ONE:
	    alpha_arg[i] = radeon_zero_alpha[op+1];
	    break;
	 case GL_TEXTURE0:
	 case GL_TEXTURE1:
	 case GL_TEXTURE2: {    
	    GLuint txunit = srcAi - GL_TEXTURE0;
	    if (ctx->Texture.Unit[txunit]._Current->Image[0][0]->_BaseFormat == GL_LUMINANCE)
	       alpha_arg[i] = radeon_zero_alpha[op+1];
	    else
	       alpha_arg[i] = radeon_texture_alpha[op][txunit];
	    }
	    break;
	 default:
	    return GL_FALSE;
	 }
      }

      /* Step 2:
       * Build up the color and alpha combine functions.
       */
      switch ( texUnit->_CurrentCombine->ModeRGB ) {
      case GL_REPLACE:
	 color_combine = (RADEON_COLOR_ARG_A_ZERO |
			  RADEON_COLOR_ARG_B_ZERO |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, C );
	 break;
      case GL_MODULATE:
	 color_combine = (RADEON_COLOR_ARG_C_ZERO |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, B );
	 break;
      case GL_ADD:
	 color_combine = (RADEON_COLOR_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, C );
	 break;
      case GL_ADD_SIGNED:
	 color_combine = (RADEON_COLOR_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_ADDSIGNED |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, C );
	 break;
      case GL_SUBTRACT:
	 color_combine = (RADEON_COLOR_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_SUBTRACT |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, C );
	 break;
      case GL_INTERPOLATE:
	 color_combine = (RADEON_BLEND_CTL_BLEND |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, B );
	 RADEON_COLOR_ARG( 1, A );
	 RADEON_COLOR_ARG( 2, C );
	 break;

      case GL_DOT3_RGB_EXT:
      case GL_DOT3_RGBA_EXT:
	 /* The EXT version of the DOT3 extension does not support the
	  * scale factor, but the ARB version (and the version in OpenGL
	  * 1.3) does.
	  */
	 RGBshift = 0;
	 /* FALLTHROUGH */

      case GL_DOT3_RGB:
      case GL_DOT3_RGBA:
	 /* The R100 / RV200 only support a 1X multiplier in hardware
	  * w/the ARB version.
	  */
	 if ( RGBshift != (RADEON_SCALE_1X >> RADEON_SCALE_SHIFT) ) {
	    return GL_FALSE;
	 }

	 RGBshift += 2;
	 if ( (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA_EXT)
	    || (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA) ) {
            /* is it necessary to set this or will it be ignored anyway? */
	    Ashift = RGBshift;
	 }

	 color_combine = (RADEON_COLOR_ARG_C_ZERO |
			  RADEON_BLEND_CTL_DOT3 |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, B );
	 break;

      case GL_MODULATE_ADD_ATI:
	 color_combine = (RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, C );
	 RADEON_COLOR_ARG( 2, B );
	 break;
      case GL_MODULATE_SIGNED_ADD_ATI:
	 color_combine = (RADEON_BLEND_CTL_ADDSIGNED |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, C );
	 RADEON_COLOR_ARG( 2, B );
	 break;
      case GL_MODULATE_SUBTRACT_ATI:
	 color_combine = (RADEON_BLEND_CTL_SUBTRACT |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, C );
	 RADEON_COLOR_ARG( 2, B );
	 break;
      default:
	 return GL_FALSE;
      }

      switch ( texUnit->_CurrentCombine->ModeA ) {
      case GL_REPLACE:
	 alpha_combine = (RADEON_ALPHA_ARG_A_ZERO |
			  RADEON_ALPHA_ARG_B_ZERO |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, C );
	 break;
      case GL_MODULATE:
	 alpha_combine = (RADEON_ALPHA_ARG_C_ZERO |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, B );
	 break;
      case GL_ADD:
	 alpha_combine = (RADEON_ALPHA_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, C );
	 break;
      case GL_ADD_SIGNED:
	 alpha_combine = (RADEON_ALPHA_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_ADDSIGNED |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, C );
	 break;
      case GL_SUBTRACT:
	 alpha_combine = (RADEON_COLOR_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_SUBTRACT |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, C );
	 break;
      case GL_INTERPOLATE:
	 alpha_combine = (RADEON_BLEND_CTL_BLEND |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, B );
	 RADEON_ALPHA_ARG( 1, A );
	 RADEON_ALPHA_ARG( 2, C );
	 break;

      case GL_MODULATE_ADD_ATI:
	 alpha_combine = (RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, C );
	 RADEON_ALPHA_ARG( 2, B );
	 break;
      case GL_MODULATE_SIGNED_ADD_ATI:
	 alpha_combine = (RADEON_BLEND_CTL_ADDSIGNED |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, C );
	 RADEON_ALPHA_ARG( 2, B );
	 break;
      case GL_MODULATE_SUBTRACT_ATI:
	 alpha_combine = (RADEON_BLEND_CTL_SUBTRACT |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, C );
	 RADEON_ALPHA_ARG( 2, B );
	 break;
      default:
	 return GL_FALSE;
      }

      if ( (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGB_EXT)
	   || (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGB) ) {
	 alpha_combine |= RADEON_DOT_ALPHA_DONT_REPLICATE;
      }

      /* Step 3:
       * Apply the scale factor.
       */
      color_combine |= (RGBshift << RADEON_SCALE_SHIFT);
      alpha_combine |= (Ashift   << RADEON_SCALE_SHIFT);

      /* All done!
       */
   }

   if ( rmesa->hw.tex[unit].cmd[TEX_PP_TXCBLEND] != color_combine ||
	rmesa->hw.tex[unit].cmd[TEX_PP_TXABLEND] != alpha_combine ) {
      RADEON_STATECHANGE( rmesa, tex[unit] );
      rmesa->hw.tex[unit].cmd[TEX_PP_TXCBLEND] = color_combine;
      rmesa->hw.tex[unit].cmd[TEX_PP_TXABLEND] = alpha_combine;
   }

   return GL_TRUE;
}

#define TEXOBJ_TXFILTER_MASK (RADEON_MAX_MIP_LEVEL_MASK |	\
			      RADEON_MIN_FILTER_MASK | 		\
			      RADEON_MAG_FILTER_MASK |		\
			      RADEON_MAX_ANISO_MASK |		\
			      RADEON_YUV_TO_RGB |		\
			      RADEON_YUV_TEMPERATURE_MASK |	\
			      RADEON_CLAMP_S_MASK | 		\
			      RADEON_CLAMP_T_MASK | 		\
			      RADEON_BORDER_MODE_D3D )

#define TEXOBJ_TXFORMAT_MASK (RADEON_TXFORMAT_WIDTH_MASK |	\
			      RADEON_TXFORMAT_HEIGHT_MASK |	\
			      RADEON_TXFORMAT_FORMAT_MASK |	\
                              RADEON_TXFORMAT_F5_WIDTH_MASK |	\
                              RADEON_TXFORMAT_F5_HEIGHT_MASK |	\
			      RADEON_TXFORMAT_ALPHA_IN_MAP |	\
			      RADEON_TXFORMAT_CUBIC_MAP_ENABLE |	\
                              RADEON_TXFORMAT_NON_POWER2)


static void import_tex_obj_state( radeonContextPtr rmesa,
				  int unit,
				  radeonTexObjPtr texobj )
{
/* do not use RADEON_DB_STATE to avoid stale texture caches */
   int *cmd = &rmesa->hw.tex[unit].cmd[TEX_CMD_0];
   GLuint se_coord_fmt = rmesa->hw.set.cmd[SET_SE_COORDFMT];

   RADEON_STATECHANGE( rmesa, tex[unit] );

   cmd[TEX_PP_TXFILTER] &= ~TEXOBJ_TXFILTER_MASK;
   cmd[TEX_PP_TXFILTER] |= texobj->pp_txfilter & TEXOBJ_TXFILTER_MASK;
   cmd[TEX_PP_TXFORMAT] &= ~TEXOBJ_TXFORMAT_MASK;
   cmd[TEX_PP_TXFORMAT] |= texobj->pp_txformat & TEXOBJ_TXFORMAT_MASK;
   cmd[TEX_PP_TXOFFSET] = texobj->pp_txoffset;
   cmd[TEX_PP_BORDER_COLOR] = texobj->pp_border_color;

   if (texobj->base.tObj->Target == GL_TEXTURE_RECTANGLE_NV) {
      GLuint *txr_cmd = RADEON_DB_STATE( txr[unit] );
      txr_cmd[TXR_PP_TEX_SIZE] = texobj->pp_txsize; /* NPOT only! */
      txr_cmd[TXR_PP_TEX_PITCH] = texobj->pp_txpitch; /* NPOT only! */
      RADEON_DB_STATECHANGE( rmesa, &rmesa->hw.txr[unit] );
      se_coord_fmt |= RADEON_VTX_ST0_NONPARAMETRIC << unit;
   }
   else {
      se_coord_fmt &= ~(RADEON_VTX_ST0_NONPARAMETRIC << unit);

      if (texobj->base.tObj->Target == GL_TEXTURE_CUBE_MAP) {
	 int *cube_cmd = &rmesa->hw.cube[unit].cmd[CUBE_CMD_0];
	 GLuint bytesPerFace = texobj->base.totalSize / 6;
	 ASSERT(texobj->base.totalSize % 6 == 0);

	 RADEON_STATECHANGE( rmesa, cube[unit] );
	 cube_cmd[CUBE_PP_CUBIC_FACES] = texobj->pp_cubic_faces;
	 /* dont know if this setup conforms to OpenGL.. 
	  * at least it matches the behavior of mesa software renderer
	  */
	 cube_cmd[CUBE_PP_CUBIC_OFFSET_0] = texobj->pp_txoffset; /* right */
	 cube_cmd[CUBE_PP_CUBIC_OFFSET_1] = texobj->pp_txoffset + 1 * bytesPerFace; /* left */
	 cube_cmd[CUBE_PP_CUBIC_OFFSET_2] = texobj->pp_txoffset + 2 * bytesPerFace; /* top */
	 cube_cmd[CUBE_PP_CUBIC_OFFSET_3] = texobj->pp_txoffset + 3 * bytesPerFace; /* bottom */
	 cube_cmd[CUBE_PP_CUBIC_OFFSET_4] = texobj->pp_txoffset + 4 * bytesPerFace; /* front */
	 cmd[TEX_PP_TXOFFSET] = texobj->pp_txoffset + 5 * bytesPerFace; /* back */
      }
   }

   if (se_coord_fmt != rmesa->hw.set.cmd[SET_SE_COORDFMT]) {
      RADEON_STATECHANGE( rmesa, set );
      rmesa->hw.set.cmd[SET_SE_COORDFMT] = se_coord_fmt;
   }

   texobj->dirty_state &= ~(1<<unit);
}




static void set_texgen_matrix( radeonContextPtr rmesa, 
			       GLuint unit,
			       const GLfloat *s_plane,
			       const GLfloat *t_plane,
			       const GLfloat *r_plane,
			       const GLfloat *q_plane )
{
   rmesa->TexGenMatrix[unit].m[0]  = s_plane[0];
   rmesa->TexGenMatrix[unit].m[4]  = s_plane[1];
   rmesa->TexGenMatrix[unit].m[8]  = s_plane[2];
   rmesa->TexGenMatrix[unit].m[12] = s_plane[3];

   rmesa->TexGenMatrix[unit].m[1]  = t_plane[0];
   rmesa->TexGenMatrix[unit].m[5]  = t_plane[1];
   rmesa->TexGenMatrix[unit].m[9]  = t_plane[2];
   rmesa->TexGenMatrix[unit].m[13] = t_plane[3];

   rmesa->TexGenMatrix[unit].m[2]  = r_plane[0];
   rmesa->TexGenMatrix[unit].m[6]  = r_plane[1];
   rmesa->TexGenMatrix[unit].m[10] = r_plane[2];
   rmesa->TexGenMatrix[unit].m[14] = r_plane[3];

   rmesa->TexGenMatrix[unit].m[3]  = q_plane[0];
   rmesa->TexGenMatrix[unit].m[7]  = q_plane[1];
   rmesa->TexGenMatrix[unit].m[11] = q_plane[2];
   rmesa->TexGenMatrix[unit].m[15] = q_plane[3];

   rmesa->TexGenEnabled |= RADEON_TEXMAT_0_ENABLE << unit;
   rmesa->NewGLState |= _NEW_TEXTURE_MATRIX;
}

/* Returns GL_FALSE if fallback required.
 */
static GLboolean radeon_validate_texgen( GLcontext *ctx, GLuint unit )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   GLuint inputshift = RADEON_TEXGEN_0_INPUT_SHIFT + unit*4;
   GLuint tmp = rmesa->TexGenEnabled;
   static const GLfloat reflect[16] = {
      -1,  0,  0,  0,
       0, -1,  0,  0,
       0,  0,  -1, 0,
       0,  0,  0,  1 };

   rmesa->TexGenEnabled &= ~(RADEON_TEXGEN_TEXMAT_0_ENABLE << unit);
   rmesa->TexGenEnabled &= ~(RADEON_TEXMAT_0_ENABLE << unit);
   rmesa->TexGenEnabled &= ~(RADEON_TEXGEN_INPUT_MASK << inputshift);
   rmesa->TexGenNeedNormals[unit] = 0;

   if ((texUnit->TexGenEnabled & (S_BIT|T_BIT|R_BIT|Q_BIT)) == 0) {
      /* Disabled, no fallback:
       */
      rmesa->TexGenEnabled |=
	 (RADEON_TEXGEN_INPUT_TEXCOORD_0 + unit) << inputshift;
      return GL_TRUE;
   }
   /* the r100 cannot do texgen for some coords and not for others
    * we do not detect such cases (certainly can't do it here) and just
    * ASSUME that when S and T are texgen enabled we do not need other
    * non-texgen enabled coords, no matter if the R and Q bits are texgen
    * enabled. Still check for mixed mode texgen for all coords.
    */
   else if ( (texUnit->TexGenEnabled & S_BIT) &&
	     (texUnit->TexGenEnabled & T_BIT) &&
	     (texUnit->GenModeS == texUnit->GenModeT) ) {
      if ( ((texUnit->TexGenEnabled & R_BIT) &&
	    (texUnit->GenModeS != texUnit->GenModeR)) ||
	   ((texUnit->TexGenEnabled & Q_BIT) &&
	    (texUnit->GenModeS != texUnit->GenModeQ)) ) {
	 /* Mixed modes, fallback:
	  */
	 if (RADEON_DEBUG & DEBUG_FALLBACKS)
	    fprintf(stderr, "fallback mixed texgen\n");
	 return GL_FALSE;
      }
      rmesa->TexGenEnabled |= RADEON_TEXGEN_TEXMAT_0_ENABLE << unit;
   }
   else {
   /* some texgen mode not including both S and T bits */
      if (RADEON_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "fallback mixed texgen/nontexgen\n");
      return GL_FALSE;
   }

   if ((texUnit->TexGenEnabled & (R_BIT | Q_BIT)) != 0) {
      /* need this here for vtxfmt presumably. Argh we need to set
         this from way too many places, would be much easier if we could leave
         tcl q coord always enabled as on r200) */
      RADEON_STATECHANGE( rmesa, tcl );
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_Q_BIT(unit);
   }

   switch (texUnit->GenModeS) {
   case GL_OBJECT_LINEAR:
      rmesa->TexGenEnabled |= RADEON_TEXGEN_INPUT_OBJ << inputshift;
      set_texgen_matrix( rmesa, unit,
			 texUnit->ObjectPlaneS,
			 texUnit->ObjectPlaneT,
			 texUnit->ObjectPlaneR,
			 texUnit->ObjectPlaneQ);
      break;

   case GL_EYE_LINEAR:
      rmesa->TexGenEnabled |= RADEON_TEXGEN_INPUT_EYE << inputshift;
      set_texgen_matrix( rmesa, unit,
			 texUnit->EyePlaneS,
			 texUnit->EyePlaneT,
			 texUnit->EyePlaneR,
			 texUnit->EyePlaneQ);
      break;

   case GL_REFLECTION_MAP_NV:
      rmesa->TexGenNeedNormals[unit] = GL_TRUE;
      rmesa->TexGenEnabled |= RADEON_TEXGEN_INPUT_EYE_REFLECT << inputshift;
      /* TODO: unknown if this is needed/correct */
      set_texgen_matrix( rmesa, unit, reflect, reflect + 4,
			reflect + 8, reflect + 12 );
      break;

   case GL_NORMAL_MAP_NV:
      rmesa->TexGenNeedNormals[unit] = GL_TRUE;
      rmesa->TexGenEnabled |= RADEON_TEXGEN_INPUT_EYE_NORMAL << inputshift;
      break;

   case GL_SPHERE_MAP:
      /* the mode which everyone uses :-( */
   default:
      /* Unsupported mode, fallback:
       */
      if (RADEON_DEBUG & DEBUG_FALLBACKS) 
	 fprintf(stderr, "fallback GL_SPHERE_MAP\n");
      return GL_FALSE;
   }

   if (tmp != rmesa->TexGenEnabled) {
      rmesa->NewGLState |= _NEW_TEXTURE_MATRIX;
   }

   return GL_TRUE;
}


static void disable_tex( GLcontext *ctx, int unit )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if (rmesa->hw.ctx.cmd[CTX_PP_CNTL] & (RADEON_TEX_0_ENABLE<<unit)) {
      /* Texture unit disabled */
      if ( rmesa->state.texture.unit[unit].texobj != NULL ) {
	 /* The old texture is no longer bound to this texture unit.
	  * Mark it as such.
	  */

	 rmesa->state.texture.unit[unit].texobj->base.bound &= ~(1UL << unit);
	 rmesa->state.texture.unit[unit].texobj = NULL;
      }

      RADEON_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= 
	  ~((RADEON_TEX_0_ENABLE | RADEON_TEX_BLEND_0_ENABLE) << unit);

      RADEON_STATECHANGE( rmesa, tcl );
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] &= ~(RADEON_ST_BIT(unit) |
						RADEON_Q_BIT(unit));

      if (rmesa->TclFallback & (RADEON_TCL_FALLBACK_TEXGEN_0<<unit)) {
	 TCL_FALLBACK( ctx, (RADEON_TCL_FALLBACK_TEXGEN_0<<unit), GL_FALSE);
	 rmesa->recheck_texgen[unit] = GL_TRUE;
      }

      if (rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT] & RADEON_TXFORMAT_CUBIC_MAP_ENABLE) {
      /* this seems to be a genuine (r100 only?) hw bug. Need to remove the
         cubic_map bit on unit 2 when the unit is disabled, otherwise every
	 2nd (2d) mipmap on unit 0 will be broken (may not be needed for other
	 units, better be safe than sorry though).*/
	 RADEON_STATECHANGE( rmesa, tex[unit] );
	 rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT] &= ~RADEON_TXFORMAT_CUBIC_MAP_ENABLE;
      }

      {
	 GLuint inputshift = RADEON_TEXGEN_0_INPUT_SHIFT + unit*4;
	 GLuint tmp = rmesa->TexGenEnabled;

	 rmesa->TexGenEnabled &= ~(RADEON_TEXGEN_TEXMAT_0_ENABLE<<unit);
	 rmesa->TexGenEnabled &= ~(RADEON_TEXMAT_0_ENABLE<<unit);
	 rmesa->TexGenEnabled &= ~(RADEON_TEXGEN_INPUT_MASK<<inputshift);
	 rmesa->TexGenNeedNormals[unit] = 0;
	 rmesa->TexGenEnabled |= 
	     (RADEON_TEXGEN_INPUT_TEXCOORD_0+unit) << inputshift;

	 if (tmp != rmesa->TexGenEnabled) {
	    rmesa->recheck_texgen[unit] = GL_TRUE;
	    rmesa->NewGLState |= _NEW_TEXTURE_MATRIX;
	 }
      }
   }
}

static GLboolean enable_tex_2d( GLcontext *ctx, int unit )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   radeonTexObjPtr t = (radeonTexObjPtr) tObj->DriverData;

   /* Need to load the 2d images associated with this unit.
    */
   if (t->pp_txformat & RADEON_TXFORMAT_NON_POWER2) {
      t->pp_txformat &= ~RADEON_TXFORMAT_NON_POWER2;
      t->base.dirty_images[0] = ~0;
   }

   ASSERT(tObj->Target == GL_TEXTURE_2D || tObj->Target == GL_TEXTURE_1D);

   if ( t->base.dirty_images[0] ) {
      RADEON_FIREVERTICES( rmesa );
      radeonSetTexImages( rmesa, tObj );
      radeonUploadTexImages( rmesa, (radeonTexObjPtr) tObj->DriverData, 0 );
      if ( !t->base.memBlock ) 
	return GL_FALSE;
   }

   return GL_TRUE;
}

static GLboolean enable_tex_cube( GLcontext *ctx, int unit )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   radeonTexObjPtr t = (radeonTexObjPtr) tObj->DriverData;
   GLuint face;

   /* Need to load the 2d images associated with this unit.
    */
   if (t->pp_txformat & RADEON_TXFORMAT_NON_POWER2) {
      t->pp_txformat &= ~RADEON_TXFORMAT_NON_POWER2;
      for (face = 0; face < 6; face++)
         t->base.dirty_images[face] = ~0;
   }

   ASSERT(tObj->Target == GL_TEXTURE_CUBE_MAP);

   if ( t->base.dirty_images[0] || t->base.dirty_images[1] ||
        t->base.dirty_images[2] || t->base.dirty_images[3] ||
        t->base.dirty_images[4] || t->base.dirty_images[5] ) {
      /* flush */
      RADEON_FIREVERTICES( rmesa );
      /* layout memory space, once for all faces */
      radeonSetTexImages( rmesa, tObj );
   }

   /* upload (per face) */
   for (face = 0; face < 6; face++) {
      if (t->base.dirty_images[face]) {
         radeonUploadTexImages( rmesa, (radeonTexObjPtr) tObj->DriverData, face );
      }
   }
      
   if ( !t->base.memBlock ) {
      /* texmem alloc failed, use s/w fallback */
      return GL_FALSE;
   }

   return GL_TRUE;
}

static GLboolean enable_tex_rect( GLcontext *ctx, int unit )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   radeonTexObjPtr t = (radeonTexObjPtr) tObj->DriverData;

   if (!(t->pp_txformat & RADEON_TXFORMAT_NON_POWER2)) {
      t->pp_txformat |= RADEON_TXFORMAT_NON_POWER2;
      t->base.dirty_images[0] = ~0;
   }

   ASSERT(tObj->Target == GL_TEXTURE_RECTANGLE_NV);

   if ( t->base.dirty_images[0] ) {
      RADEON_FIREVERTICES( rmesa );
      radeonSetTexImages( rmesa, tObj );
      radeonUploadTexImages( rmesa, (radeonTexObjPtr) tObj->DriverData, 0 );
      if ( !t->base.memBlock /* && !rmesa->prefer_gart_client_texturing  FIXME */ ) {
	 fprintf(stderr, "%s: upload failed\n", __FUNCTION__);
	 return GL_FALSE;
      }
   }

   return GL_TRUE;
}


static GLboolean update_tex_common( GLcontext *ctx, int unit )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   radeonTexObjPtr t = (radeonTexObjPtr) tObj->DriverData;
   GLenum format;

   /* Fallback if there's a texture border */
   if ( tObj->Image[0][tObj->BaseLevel]->Border > 0 ) {
      fprintf(stderr, "%s: border\n", __FUNCTION__);
      return GL_FALSE;
   }
   /* yuv conversion only works in first unit */
   if (unit != 0 && (t->pp_txfilter & RADEON_YUV_TO_RGB))
      return GL_FALSE;

   /* Update state if this is a different texture object to last
    * time.
    */
   if ( rmesa->state.texture.unit[unit].texobj != t ) {
      if ( rmesa->state.texture.unit[unit].texobj != NULL ) {
	 /* The old texture is no longer bound to this texture unit.
	  * Mark it as such.
	  */

	 rmesa->state.texture.unit[unit].texobj->base.bound &= 
	     ~(1UL << unit);
      }

      rmesa->state.texture.unit[unit].texobj = t;
      t->base.bound |= (1UL << unit);
      t->dirty_state |= 1<<unit;
      driUpdateTextureLRU( (driTextureObject *) t ); /* XXX: should be locked! */
   }


   /* Newly enabled?
    */
   if ( !(rmesa->hw.ctx.cmd[CTX_PP_CNTL] & (RADEON_TEX_0_ENABLE<<unit))) {
      RADEON_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= 
	  (RADEON_TEX_0_ENABLE | RADEON_TEX_BLEND_0_ENABLE) << unit;

      RADEON_STATECHANGE( rmesa, tcl );

      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_ST_BIT(unit);

      rmesa->recheck_texgen[unit] = GL_TRUE;
   }

   if (t->dirty_state & (1<<unit)) {
      import_tex_obj_state( rmesa, unit, t );
      /* may need to update texture matrix (for texrect adjustments) */
      rmesa->NewGLState |= _NEW_TEXTURE_MATRIX;
   }

   if (rmesa->recheck_texgen[unit]) {
      GLboolean fallback = !radeon_validate_texgen( ctx, unit );
      TCL_FALLBACK( ctx, (RADEON_TCL_FALLBACK_TEXGEN_0<<unit), fallback);
      rmesa->recheck_texgen[unit] = 0;
      rmesa->NewGLState |= _NEW_TEXTURE_MATRIX;
   }

   format = tObj->Image[0][tObj->BaseLevel]->_BaseFormat;
   if ( rmesa->state.texture.unit[unit].format != format ||
	rmesa->state.texture.unit[unit].envMode != texUnit->EnvMode ) {
      rmesa->state.texture.unit[unit].format = format;
      rmesa->state.texture.unit[unit].envMode = texUnit->EnvMode;
      if ( ! radeonUpdateTextureEnv( ctx, unit ) ) {
	 return GL_FALSE;
      }
   }

   FALLBACK( rmesa, RADEON_FALLBACK_BORDER_MODE, t->border_fallback );
   return !t->border_fallback;
}



static GLboolean radeonUpdateTextureUnit( GLcontext *ctx, int unit )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   if ( texUnit->_ReallyEnabled & (TEXTURE_RECT_BIT) ) {
      return (enable_tex_rect( ctx, unit ) &&
	      update_tex_common( ctx, unit ));
   }
   else if ( texUnit->_ReallyEnabled & (TEXTURE_1D_BIT | TEXTURE_2D_BIT) ) {
      return (enable_tex_2d( ctx, unit ) &&
	      update_tex_common( ctx, unit ));
   }
   else if ( texUnit->_ReallyEnabled & (TEXTURE_CUBE_BIT) ) {
      return (enable_tex_cube( ctx, unit ) &&
	      update_tex_common( ctx, unit ));
   }
   else if ( texUnit->_ReallyEnabled ) {
      return GL_FALSE;
   }
   else {
      disable_tex( ctx, unit );
      return GL_TRUE;
   }
}

void radeonUpdateTextureState( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLboolean ok;

   ok = (radeonUpdateTextureUnit( ctx, 0 ) &&
	 radeonUpdateTextureUnit( ctx, 1 ) &&
	 radeonUpdateTextureUnit( ctx, 2 ));

   FALLBACK( rmesa, RADEON_FALLBACK_TEXTURE, !ok );

   if (rmesa->TclFallback)
      radeonChooseVertexState( ctx );
}
