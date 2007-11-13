/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_texstate.c,v 1.3 2003/02/15 22:18:47 dawes Exp $ */
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
#include "context.h"
#include "macros.h"
#include "texformat.h"
#include "enums.h"

#include "r200_context.h"
#include "r200_state.h"
#include "r200_ioctl.h"
#include "r200_swtcl.h"
#include "r200_tex.h"
#include "r200_tcl.h"


#define R200_TXFORMAT_A8        R200_TXFORMAT_I8
#define R200_TXFORMAT_L8        R200_TXFORMAT_I8
#define R200_TXFORMAT_AL88      R200_TXFORMAT_AI88
#define R200_TXFORMAT_YCBCR     R200_TXFORMAT_YVYU422
#define R200_TXFORMAT_YCBCR_REV R200_TXFORMAT_VYUY422
#define R200_TXFORMAT_RGB_DXT1  R200_TXFORMAT_DXT1
#define R200_TXFORMAT_RGBA_DXT1 R200_TXFORMAT_DXT1
#define R200_TXFORMAT_RGBA_DXT3 R200_TXFORMAT_DXT23
#define R200_TXFORMAT_RGBA_DXT5 R200_TXFORMAT_DXT45

#define _COLOR(f) \
    [ MESA_FORMAT_ ## f ] = { R200_TXFORMAT_ ## f, 0 }
#define _COLOR_REV(f) \
    [ MESA_FORMAT_ ## f ## _REV ] = { R200_TXFORMAT_ ## f, 0 }
#define _ALPHA(f) \
    [ MESA_FORMAT_ ## f ] = { R200_TXFORMAT_ ## f | R200_TXFORMAT_ALPHA_IN_MAP, 0 }
#define _ALPHA_REV(f) \
    [ MESA_FORMAT_ ## f ## _REV ] = { R200_TXFORMAT_ ## f | R200_TXFORMAT_ALPHA_IN_MAP, 0 }
#define _YUV(f) \
    [ MESA_FORMAT_ ## f ] = { R200_TXFORMAT_ ## f, R200_YUV_TO_RGB }
#define _INVALID(f) \
    [ MESA_FORMAT_ ## f ] = { 0xffffffff, 0 }
#define VALID_FORMAT(f) ( ((f) <= MESA_FORMAT_RGBA_DXT5) \
			     && (tx_table_le[f].format != 0xffffffff) )

static const struct {
   GLuint format, filter;
}
tx_table_be[] =
{
   [ MESA_FORMAT_RGBA8888 ] = { R200_TXFORMAT_ABGR8888 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
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

static const struct {
   GLuint format, filter;
}
tx_table_le[] =
{
   _ALPHA(RGBA8888),
   [ MESA_FORMAT_RGBA8888_REV ] = { R200_TXFORMAT_ABGR8888 | R200_TXFORMAT_ALPHA_IN_MAP, 0 },
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
static void r200SetTexImages( r200ContextPtr rmesa,
			      struct gl_texture_object *tObj )
{
   r200TexObjPtr t = (r200TexObjPtr)tObj->DriverData;
   const struct gl_texture_image *baseImage = tObj->Image[0][tObj->BaseLevel];
   GLint curOffset, blitWidth;
   GLint i, texelBytes;
   GLint numLevels;
   GLint log2Width, log2Height, log2Depth;
   const GLuint ui = 1;
   const GLubyte littleEndian = *((const GLubyte *) &ui);

   /* Set the hardware texture format
    */

   t->pp_txformat &= ~(R200_TXFORMAT_FORMAT_MASK |
		       R200_TXFORMAT_ALPHA_IN_MAP);
   t->pp_txfilter &= ~R200_YUV_TO_RGB;

   if ( VALID_FORMAT( baseImage->TexFormat->MesaFormat ) ) {
      if (littleEndian) {
	 t->pp_txformat |= tx_table_le[ baseImage->TexFormat->MesaFormat ].format;
	 t->pp_txfilter |= tx_table_le[ baseImage->TexFormat->MesaFormat ].filter;
      }
      else {
	 t->pp_txformat |= tx_table_be[ baseImage->TexFormat->MesaFormat ].format;
	 t->pp_txfilter |= tx_table_be[ baseImage->TexFormat->MesaFormat ].filter;
      }
   }
   else {
      _mesa_problem(NULL, "unexpected texture format in %s", __FUNCTION__);
      return;
   }

   texelBytes = baseImage->TexFormat->TexelBytes;

   /* Compute which mipmap levels we really want to send to the hardware.
    */

   driCalculateTextureFirstLastLevel( (driTextureObject *) t );
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
   if (texelBytes) {
      if (rmesa->texmicrotile  && (tObj->Target != GL_TEXTURE_RECTANGLE_NV) &&
      /* texrect might be able to use micro tiling too in theory? */
	 (baseImage->Height > 1)) {
	 /* allow 32 (bytes) x 1 mip (which will use two times the space
	 the non-tiled version would use) max if base texture is large enough */
	 if ((numLevels == 1) ||
	   (((baseImage->Width * texelBytes / baseImage->Height) <= 32) &&
	       (baseImage->Width * texelBytes > 64)) ||
	    ((baseImage->Width * texelBytes / baseImage->Height) <= 16)) {
	    t->tile_bits |= R200_TXO_MICRO_TILE;
	 }
      }
      if (tObj->Target != GL_TEXTURE_RECTANGLE_NV) {
	 /* we can set macro tiling even for small textures, they will be untiled anyway */
	 t->tile_bits |= R200_TXO_MACRO_TILE;
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
         if ((t->pp_txformat & R200_TXFORMAT_FORMAT_MASK) == R200_TXFORMAT_DXT1) {
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
      else if (t->tile_bits & R200_TXO_MICRO_TILE) {
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
   t->pp_txfilter &= ~R200_MAX_MIP_LEVEL_MASK;
   t->pp_txfilter |= (numLevels - 1) << R200_MAX_MIP_LEVEL_SHIFT;

   t->pp_txformat &= ~(R200_TXFORMAT_WIDTH_MASK |
		       R200_TXFORMAT_HEIGHT_MASK |
                       R200_TXFORMAT_CUBIC_MAP_ENABLE |
                       R200_TXFORMAT_F5_WIDTH_MASK |
                       R200_TXFORMAT_F5_HEIGHT_MASK);
   t->pp_txformat |= ((log2Width << R200_TXFORMAT_WIDTH_SHIFT) |
		      (log2Height << R200_TXFORMAT_HEIGHT_SHIFT));

   t->pp_txformat_x &= ~(R200_DEPTH_LOG2_MASK | R200_TEXCOORD_MASK);
   if (tObj->Target == GL_TEXTURE_3D) {
      t->pp_txformat_x |= (log2Depth << R200_DEPTH_LOG2_SHIFT);
      t->pp_txformat_x |= R200_TEXCOORD_VOLUME;
   }
   else if (tObj->Target == GL_TEXTURE_CUBE_MAP) {
      ASSERT(log2Width == log2Height);
      t->pp_txformat |= ((log2Width << R200_TXFORMAT_F5_WIDTH_SHIFT) |
                         (log2Height << R200_TXFORMAT_F5_HEIGHT_SHIFT) |
/* don't think we need this bit, if it exists at all - fglrx does not set it */
                         (R200_TXFORMAT_CUBIC_MAP_ENABLE));
      t->pp_txformat_x |= R200_TEXCOORD_CUBIC_ENV;
      t->pp_cubic_faces = ((log2Width << R200_FACE_WIDTH_1_SHIFT) |
                           (log2Height << R200_FACE_HEIGHT_1_SHIFT) |
                           (log2Width << R200_FACE_WIDTH_2_SHIFT) |
                           (log2Height << R200_FACE_HEIGHT_2_SHIFT) |
                           (log2Width << R200_FACE_WIDTH_3_SHIFT) |
                           (log2Height << R200_FACE_HEIGHT_3_SHIFT) |
                           (log2Width << R200_FACE_WIDTH_4_SHIFT) |
                           (log2Height << R200_FACE_HEIGHT_4_SHIFT));
   }
   else {
      /* If we don't in fact send enough texture coordinates, q will be 1,
       * making TEXCOORD_PROJ act like TEXCOORD_NONPROJ (Right?)
       */
      t->pp_txformat_x |= R200_TEXCOORD_PROJ;
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

   /* FYI: r200UploadTexImages( rmesa, t ) used to be called here */
}



/* ================================================================
 * Texture combine functions
 */

/* GL_ARB_texture_env_combine support
 */

/* The color tables have combine functions for GL_SRC_COLOR,
 * GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA and GL_ONE_MINUS_SRC_ALPHA.
 */
static GLuint r200_register_color[][R200_MAX_TEXTURE_UNITS] =
{
   {
      R200_TXC_ARG_A_R0_COLOR,
      R200_TXC_ARG_A_R1_COLOR,
      R200_TXC_ARG_A_R2_COLOR,
      R200_TXC_ARG_A_R3_COLOR,
      R200_TXC_ARG_A_R4_COLOR,
      R200_TXC_ARG_A_R5_COLOR
   },
   {
      R200_TXC_ARG_A_R0_COLOR | R200_TXC_COMP_ARG_A,
      R200_TXC_ARG_A_R1_COLOR | R200_TXC_COMP_ARG_A,
      R200_TXC_ARG_A_R2_COLOR | R200_TXC_COMP_ARG_A,
      R200_TXC_ARG_A_R3_COLOR | R200_TXC_COMP_ARG_A,
      R200_TXC_ARG_A_R4_COLOR | R200_TXC_COMP_ARG_A,
      R200_TXC_ARG_A_R5_COLOR | R200_TXC_COMP_ARG_A
   },
   {
      R200_TXC_ARG_A_R0_ALPHA,
      R200_TXC_ARG_A_R1_ALPHA,
      R200_TXC_ARG_A_R2_ALPHA,
      R200_TXC_ARG_A_R3_ALPHA,
      R200_TXC_ARG_A_R4_ALPHA,
      R200_TXC_ARG_A_R5_ALPHA
   },
   {
      R200_TXC_ARG_A_R0_ALPHA | R200_TXC_COMP_ARG_A,
      R200_TXC_ARG_A_R1_ALPHA | R200_TXC_COMP_ARG_A,
      R200_TXC_ARG_A_R2_ALPHA | R200_TXC_COMP_ARG_A,
      R200_TXC_ARG_A_R3_ALPHA | R200_TXC_COMP_ARG_A,
      R200_TXC_ARG_A_R4_ALPHA | R200_TXC_COMP_ARG_A,
      R200_TXC_ARG_A_R5_ALPHA | R200_TXC_COMP_ARG_A
   },
};

static GLuint r200_tfactor_color[] =
{
   R200_TXC_ARG_A_TFACTOR_COLOR,
   R200_TXC_ARG_A_TFACTOR_COLOR | R200_TXC_COMP_ARG_A,
   R200_TXC_ARG_A_TFACTOR_ALPHA,
   R200_TXC_ARG_A_TFACTOR_ALPHA | R200_TXC_COMP_ARG_A
};

static GLuint r200_tfactor1_color[] =
{
   R200_TXC_ARG_A_TFACTOR1_COLOR,
   R200_TXC_ARG_A_TFACTOR1_COLOR | R200_TXC_COMP_ARG_A,
   R200_TXC_ARG_A_TFACTOR1_ALPHA,
   R200_TXC_ARG_A_TFACTOR1_ALPHA | R200_TXC_COMP_ARG_A
};

static GLuint r200_primary_color[] =
{
   R200_TXC_ARG_A_DIFFUSE_COLOR,
   R200_TXC_ARG_A_DIFFUSE_COLOR | R200_TXC_COMP_ARG_A,
   R200_TXC_ARG_A_DIFFUSE_ALPHA,
   R200_TXC_ARG_A_DIFFUSE_ALPHA | R200_TXC_COMP_ARG_A
};

/* GL_ZERO table - indices 0-3
 * GL_ONE  table - indices 1-4
 */
static GLuint r200_zero_color[] =
{
   R200_TXC_ARG_A_ZERO,
   R200_TXC_ARG_A_ZERO | R200_TXC_COMP_ARG_A,
   R200_TXC_ARG_A_ZERO,
   R200_TXC_ARG_A_ZERO | R200_TXC_COMP_ARG_A,
   R200_TXC_ARG_A_ZERO
};

/* The alpha tables only have GL_SRC_ALPHA and GL_ONE_MINUS_SRC_ALPHA.
 */
static GLuint r200_register_alpha[][R200_MAX_TEXTURE_UNITS] =
{
   {
      R200_TXA_ARG_A_R0_ALPHA,
      R200_TXA_ARG_A_R1_ALPHA,
      R200_TXA_ARG_A_R2_ALPHA,
      R200_TXA_ARG_A_R3_ALPHA,
      R200_TXA_ARG_A_R4_ALPHA,
      R200_TXA_ARG_A_R5_ALPHA
   },
   {
      R200_TXA_ARG_A_R0_ALPHA | R200_TXA_COMP_ARG_A,
      R200_TXA_ARG_A_R1_ALPHA | R200_TXA_COMP_ARG_A,
      R200_TXA_ARG_A_R2_ALPHA | R200_TXA_COMP_ARG_A,
      R200_TXA_ARG_A_R3_ALPHA | R200_TXA_COMP_ARG_A,
      R200_TXA_ARG_A_R4_ALPHA | R200_TXA_COMP_ARG_A,
      R200_TXA_ARG_A_R5_ALPHA | R200_TXA_COMP_ARG_A
   },
};

static GLuint r200_tfactor_alpha[] =
{
   R200_TXA_ARG_A_TFACTOR_ALPHA,
   R200_TXA_ARG_A_TFACTOR_ALPHA | R200_TXA_COMP_ARG_A
};

static GLuint r200_tfactor1_alpha[] =
{
   R200_TXA_ARG_A_TFACTOR1_ALPHA,
   R200_TXA_ARG_A_TFACTOR1_ALPHA | R200_TXA_COMP_ARG_A
};

static GLuint r200_primary_alpha[] =
{
   R200_TXA_ARG_A_DIFFUSE_ALPHA,
   R200_TXA_ARG_A_DIFFUSE_ALPHA | R200_TXA_COMP_ARG_A
};

/* GL_ZERO table - indices 0-1
 * GL_ONE  table - indices 1-2
 */
static GLuint r200_zero_alpha[] =
{
   R200_TXA_ARG_A_ZERO,
   R200_TXA_ARG_A_ZERO | R200_TXA_COMP_ARG_A,
   R200_TXA_ARG_A_ZERO,
};


/* Extract the arg from slot A, shift it into the correct argument slot
 * and set the corresponding complement bit.
 */
#define R200_COLOR_ARG( n, arg )			\
do {							\
   color_combine |=					\
      ((color_arg[n] & R200_TXC_ARG_A_MASK)		\
       << R200_TXC_ARG_##arg##_SHIFT);			\
   color_combine |=					\
      ((color_arg[n] >> R200_TXC_COMP_ARG_A_SHIFT)	\
       << R200_TXC_COMP_ARG_##arg##_SHIFT);		\
} while (0)

#define R200_ALPHA_ARG( n, arg )			\
do {							\
   alpha_combine |=					\
      ((alpha_arg[n] & R200_TXA_ARG_A_MASK)		\
       << R200_TXA_ARG_##arg##_SHIFT);			\
   alpha_combine |=					\
      ((alpha_arg[n] >> R200_TXA_COMP_ARG_A_SHIFT)	\
       << R200_TXA_COMP_ARG_##arg##_SHIFT);		\
} while (0)


/* ================================================================
 * Texture unit state management
 */

static GLboolean r200UpdateTextureEnv( GLcontext *ctx, int unit, int slot, GLuint replaceargs )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   GLuint color_combine, alpha_combine;
   GLuint color_scale = rmesa->hw.pix[slot].cmd[PIX_PP_TXCBLEND2] &
      ~(R200_TXC_SCALE_MASK | R200_TXC_OUTPUT_REG_MASK | R200_TXC_TFACTOR_SEL_MASK |
	R200_TXC_TFACTOR1_SEL_MASK);
   GLuint alpha_scale = rmesa->hw.pix[slot].cmd[PIX_PP_TXABLEND2] &
      ~(R200_TXA_DOT_ALPHA | R200_TXA_SCALE_MASK | R200_TXA_OUTPUT_REG_MASK |
	R200_TXA_TFACTOR_SEL_MASK | R200_TXA_TFACTOR1_SEL_MASK);

   /* texUnit->_Current can be NULL if and only if the texture unit is
    * not actually enabled.
    */
   assert( (texUnit->_ReallyEnabled == 0)
	   || (texUnit->_Current != NULL) );

   if ( R200_DEBUG & DEBUG_TEXTURE ) {
      fprintf( stderr, "%s( %p, %d )\n", __FUNCTION__, (void *)ctx, unit );
   }

   /* Set the texture environment state.  Isn't this nice and clean?
    * The chip will automagically set the texture alpha to 0xff when
    * the texture format does not include an alpha component.  This
    * reduces the amount of special-casing we have to do, alpha-only
    * textures being a notable exception.
    */

   color_scale |= ((rmesa->state.texture.unit[unit].outputreg + 1) << R200_TXC_OUTPUT_REG_SHIFT) |
			(unit << R200_TXC_TFACTOR_SEL_SHIFT) |
			(replaceargs << R200_TXC_TFACTOR1_SEL_SHIFT);
   alpha_scale |= ((rmesa->state.texture.unit[unit].outputreg + 1) << R200_TXA_OUTPUT_REG_SHIFT) |
			(unit << R200_TXA_TFACTOR_SEL_SHIFT) |
			(replaceargs << R200_TXA_TFACTOR1_SEL_SHIFT);

   if ( !texUnit->_ReallyEnabled ) {
      assert( unit == 0);
      color_combine = R200_TXC_ARG_A_ZERO | R200_TXC_ARG_B_ZERO
	  | R200_TXC_ARG_C_DIFFUSE_COLOR | R200_TXC_OP_MADD;
      alpha_combine = R200_TXA_ARG_A_ZERO | R200_TXA_ARG_B_ZERO
	  | R200_TXA_ARG_C_DIFFUSE_ALPHA | R200_TXA_OP_MADD;
   }
   else {
      GLuint color_arg[3], alpha_arg[3];
      GLuint i;
      const GLuint numColorArgs = texUnit->_CurrentCombine->_NumArgsRGB;
      const GLuint numAlphaArgs = texUnit->_CurrentCombine->_NumArgsA;
      GLuint RGBshift = texUnit->_CurrentCombine->ScaleShiftRGB;
      GLuint Ashift = texUnit->_CurrentCombine->ScaleShiftA;


      const GLint replaceoprgb =
	 ctx->Texture.Unit[replaceargs]._CurrentCombine->OperandRGB[0] - GL_SRC_COLOR;
      const GLint replaceopa =
	 ctx->Texture.Unit[replaceargs]._CurrentCombine->OperandA[0] - GL_SRC_ALPHA;

      /* Step 1:
       * Extract the color and alpha combine function arguments.
       */
      for ( i = 0 ; i < numColorArgs ; i++ ) {
	 GLint op = texUnit->_CurrentCombine->OperandRGB[i] - GL_SRC_COLOR;
	 const GLint srcRGBi = texUnit->_CurrentCombine->SourceRGB[i];
	 assert(op >= 0);
	 assert(op <= 3);
	 switch ( srcRGBi ) {
	 case GL_TEXTURE:
	    color_arg[i] = r200_register_color[op][unit];
	    break;
	 case GL_CONSTANT:
	    color_arg[i] = r200_tfactor_color[op];
	    break;
	 case GL_PRIMARY_COLOR:
	    color_arg[i] = r200_primary_color[op];
	    break;
	 case GL_PREVIOUS:
	    if (replaceargs != unit) {
	       const GLint srcRGBreplace =
		  ctx->Texture.Unit[replaceargs]._CurrentCombine->SourceRGB[0];
	       if (op >= 2) {
		  op = op ^ replaceopa;
	       }
	       else {
		  op = op ^ replaceoprgb;
	       }
	       switch (srcRGBreplace) {
	       case GL_TEXTURE:
		  color_arg[i] = r200_register_color[op][replaceargs];
		  break;
	       case GL_CONSTANT:
		  color_arg[i] = r200_tfactor1_color[op];
		  break;
	       case GL_PRIMARY_COLOR:
		  color_arg[i] = r200_primary_color[op];
		  break;
	       case GL_PREVIOUS:
		  if (slot == 0)
		     color_arg[i] = r200_primary_color[op];
		  else
		     color_arg[i] = r200_register_color[op]
			[rmesa->state.texture.unit[replaceargs - 1].outputreg];
		  break;
	       case GL_ZERO:
		  color_arg[i] = r200_zero_color[op];
		  break;
	       case GL_ONE:
		  color_arg[i] = r200_zero_color[op+1];
		  break;
	       case GL_TEXTURE0:
	       case GL_TEXTURE1:
	       case GL_TEXTURE2:
	       case GL_TEXTURE3:
	       case GL_TEXTURE4:
	       case GL_TEXTURE5:
		  color_arg[i] = r200_register_color[op][srcRGBreplace - GL_TEXTURE0];
		  break;
	       default:
	       return GL_FALSE;
	       }
	    }
	    else {
	       if (slot == 0)
		  color_arg[i] = r200_primary_color[op];
	       else
		  color_arg[i] = r200_register_color[op]
		     [rmesa->state.texture.unit[unit - 1].outputreg];
            }
	    break;
	 case GL_ZERO:
	    color_arg[i] = r200_zero_color[op];
	    break;
	 case GL_ONE:
	    color_arg[i] = r200_zero_color[op+1];
	    break;
	 case GL_TEXTURE0:
	 case GL_TEXTURE1:
	 case GL_TEXTURE2:
	 case GL_TEXTURE3:
	 case GL_TEXTURE4:
	 case GL_TEXTURE5:
	    color_arg[i] = r200_register_color[op][srcRGBi - GL_TEXTURE0];
	    break;
	 default:
	    return GL_FALSE;
	 }
      }

      for ( i = 0 ; i < numAlphaArgs ; i++ ) {
	 GLint op = texUnit->_CurrentCombine->OperandA[i] - GL_SRC_ALPHA;
	 const GLint srcAi = texUnit->_CurrentCombine->SourceA[i];
	 assert(op >= 0);
	 assert(op <= 1);
	 switch ( srcAi ) {
	 case GL_TEXTURE:
	    alpha_arg[i] = r200_register_alpha[op][unit];
	    break;
	 case GL_CONSTANT:
	    alpha_arg[i] = r200_tfactor_alpha[op];
	    break;
	 case GL_PRIMARY_COLOR:
	    alpha_arg[i] = r200_primary_alpha[op];
	    break;
	 case GL_PREVIOUS:
	    if (replaceargs != unit) {
	       const GLint srcAreplace =
		  ctx->Texture.Unit[replaceargs]._CurrentCombine->SourceA[0];
	       op = op ^ replaceopa;
	       switch (srcAreplace) {
	       case GL_TEXTURE:
		  alpha_arg[i] = r200_register_alpha[op][replaceargs];
		  break;
	       case GL_CONSTANT:
		  alpha_arg[i] = r200_tfactor1_alpha[op];
		  break;
	       case GL_PRIMARY_COLOR:
		  alpha_arg[i] = r200_primary_alpha[op];
		  break;
	       case GL_PREVIOUS:
		  if (slot == 0)
		     alpha_arg[i] = r200_primary_alpha[op];
		  else
		     alpha_arg[i] = r200_register_alpha[op]
			[rmesa->state.texture.unit[replaceargs - 1].outputreg];
		  break;
	       case GL_ZERO:
		  alpha_arg[i] = r200_zero_alpha[op];
		  break;
	       case GL_ONE:
		  alpha_arg[i] = r200_zero_alpha[op+1];
		  break;
	       case GL_TEXTURE0:
	       case GL_TEXTURE1:
	       case GL_TEXTURE2:
	       case GL_TEXTURE3:
	       case GL_TEXTURE4:
	       case GL_TEXTURE5:
		  alpha_arg[i] = r200_register_alpha[op][srcAreplace - GL_TEXTURE0];
		  break;
	       default:
	       return GL_FALSE;
	       }
	    }
	    else {
	       if (slot == 0)
		  alpha_arg[i] = r200_primary_alpha[op];
	       else
		  alpha_arg[i] = r200_register_alpha[op]
		    [rmesa->state.texture.unit[unit - 1].outputreg];
            }
	    break;
	 case GL_ZERO:
	    alpha_arg[i] = r200_zero_alpha[op];
	    break;
	 case GL_ONE:
	    alpha_arg[i] = r200_zero_alpha[op+1];
	    break;
	 case GL_TEXTURE0:
	 case GL_TEXTURE1:
	 case GL_TEXTURE2:
	 case GL_TEXTURE3:
	 case GL_TEXTURE4:
	 case GL_TEXTURE5:
	    alpha_arg[i] = r200_register_alpha[op][srcAi - GL_TEXTURE0];
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
	 color_combine = (R200_TXC_ARG_A_ZERO |
			  R200_TXC_ARG_B_ZERO |
			  R200_TXC_OP_MADD);
	 R200_COLOR_ARG( 0, C );
	 break;
      case GL_MODULATE:
	 color_combine = (R200_TXC_ARG_C_ZERO |
			  R200_TXC_OP_MADD);
	 R200_COLOR_ARG( 0, A );
	 R200_COLOR_ARG( 1, B );
	 break;
      case GL_ADD:
	 color_combine = (R200_TXC_ARG_B_ZERO |
			  R200_TXC_COMP_ARG_B | 
			  R200_TXC_OP_MADD);
	 R200_COLOR_ARG( 0, A );
	 R200_COLOR_ARG( 1, C );
	 break;
      case GL_ADD_SIGNED:
	 color_combine = (R200_TXC_ARG_B_ZERO |
			  R200_TXC_COMP_ARG_B |
			  R200_TXC_BIAS_ARG_C |	/* new */
			  R200_TXC_OP_MADD); /* was ADDSIGNED */
	 R200_COLOR_ARG( 0, A );
	 R200_COLOR_ARG( 1, C );
	 break;
      case GL_SUBTRACT:
	 color_combine = (R200_TXC_ARG_B_ZERO |
			  R200_TXC_COMP_ARG_B | 
			  R200_TXC_NEG_ARG_C |
			  R200_TXC_OP_MADD);
	 R200_COLOR_ARG( 0, A );
	 R200_COLOR_ARG( 1, C );
	 break;
      case GL_INTERPOLATE:
	 color_combine = (R200_TXC_OP_LERP);
	 R200_COLOR_ARG( 0, B );
	 R200_COLOR_ARG( 1, A );
	 R200_COLOR_ARG( 2, C );
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
	 /* DOT3 works differently on R200 than on R100.  On R100, just
	  * setting the DOT3 mode did everything for you.  On R200, the
	  * driver has to enable the biasing and scale in the inputs to
	  * put them in the proper [-1,1] range.  This is what the 4x and
	  * the -0.5 in the DOT3 spec do.  The post-scale is then set
	  * normally.
	  */

	 color_combine = (R200_TXC_ARG_C_ZERO |
			  R200_TXC_OP_DOT3 |
			  R200_TXC_BIAS_ARG_A |
			  R200_TXC_BIAS_ARG_B |
			  R200_TXC_SCALE_ARG_A |
			  R200_TXC_SCALE_ARG_B);
	 R200_COLOR_ARG( 0, A );
	 R200_COLOR_ARG( 1, B );
	 break;

      case GL_MODULATE_ADD_ATI:
	 color_combine = (R200_TXC_OP_MADD);
	 R200_COLOR_ARG( 0, A );
	 R200_COLOR_ARG( 1, C );
	 R200_COLOR_ARG( 2, B );
	 break;
      case GL_MODULATE_SIGNED_ADD_ATI:
	 color_combine = (R200_TXC_BIAS_ARG_C |	/* new */
			  R200_TXC_OP_MADD); /* was ADDSIGNED */
	 R200_COLOR_ARG( 0, A );
	 R200_COLOR_ARG( 1, C );
	 R200_COLOR_ARG( 2, B );
	 break;
      case GL_MODULATE_SUBTRACT_ATI:
	 color_combine = (R200_TXC_NEG_ARG_C |
			  R200_TXC_OP_MADD);
	 R200_COLOR_ARG( 0, A );
	 R200_COLOR_ARG( 1, C );
	 R200_COLOR_ARG( 2, B );
	 break;
      default:
	 return GL_FALSE;
      }

      switch ( texUnit->_CurrentCombine->ModeA ) {
      case GL_REPLACE:
	 alpha_combine = (R200_TXA_ARG_A_ZERO |
			  R200_TXA_ARG_B_ZERO |
			  R200_TXA_OP_MADD);
	 R200_ALPHA_ARG( 0, C );
	 break;
      case GL_MODULATE:
	 alpha_combine = (R200_TXA_ARG_C_ZERO |
			  R200_TXA_OP_MADD);
	 R200_ALPHA_ARG( 0, A );
	 R200_ALPHA_ARG( 1, B );
	 break;
      case GL_ADD:
	 alpha_combine = (R200_TXA_ARG_B_ZERO |
			  R200_TXA_COMP_ARG_B |
			  R200_TXA_OP_MADD);
	 R200_ALPHA_ARG( 0, A );
	 R200_ALPHA_ARG( 1, C );
	 break;
      case GL_ADD_SIGNED:
	 alpha_combine = (R200_TXA_ARG_B_ZERO |
			  R200_TXA_COMP_ARG_B |
			  R200_TXA_BIAS_ARG_C |	/* new */
			  R200_TXA_OP_MADD); /* was ADDSIGNED */
	 R200_ALPHA_ARG( 0, A );
	 R200_ALPHA_ARG( 1, C );
	 break;
      case GL_SUBTRACT:
	 alpha_combine = (R200_TXA_ARG_B_ZERO |
			  R200_TXA_COMP_ARG_B |
			  R200_TXA_NEG_ARG_C |
			  R200_TXA_OP_MADD);
	 R200_ALPHA_ARG( 0, A );
	 R200_ALPHA_ARG( 1, C );
	 break;
      case GL_INTERPOLATE:
	 alpha_combine = (R200_TXA_OP_LERP);
	 R200_ALPHA_ARG( 0, B );
	 R200_ALPHA_ARG( 1, A );
	 R200_ALPHA_ARG( 2, C );
	 break;

      case GL_MODULATE_ADD_ATI:
	 alpha_combine = (R200_TXA_OP_MADD);
	 R200_ALPHA_ARG( 0, A );
	 R200_ALPHA_ARG( 1, C );
	 R200_ALPHA_ARG( 2, B );
	 break;
      case GL_MODULATE_SIGNED_ADD_ATI:
	 alpha_combine = (R200_TXA_BIAS_ARG_C |	/* new */
			  R200_TXA_OP_MADD); /* was ADDSIGNED */
	 R200_ALPHA_ARG( 0, A );
	 R200_ALPHA_ARG( 1, C );
	 R200_ALPHA_ARG( 2, B );
	 break;
      case GL_MODULATE_SUBTRACT_ATI:
	 alpha_combine = (R200_TXA_NEG_ARG_C |
			  R200_TXA_OP_MADD);
	 R200_ALPHA_ARG( 0, A );
	 R200_ALPHA_ARG( 1, C );
	 R200_ALPHA_ARG( 2, B );
	 break;
      default:
	 return GL_FALSE;
      }

      if ( (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA_EXT)
	   || (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA) ) {
	 alpha_scale |= R200_TXA_DOT_ALPHA;
	 Ashift = RGBshift;
      }

      /* Step 3:
       * Apply the scale factor.
       */
      color_scale |= (RGBshift << R200_TXC_SCALE_SHIFT);
      alpha_scale |= (Ashift   << R200_TXA_SCALE_SHIFT);

      /* All done!
       */
   }

   if ( rmesa->hw.pix[slot].cmd[PIX_PP_TXCBLEND] != color_combine ||
	rmesa->hw.pix[slot].cmd[PIX_PP_TXABLEND] != alpha_combine ||
	rmesa->hw.pix[slot].cmd[PIX_PP_TXCBLEND2] != color_scale ||
	rmesa->hw.pix[slot].cmd[PIX_PP_TXABLEND2] != alpha_scale) {
      R200_STATECHANGE( rmesa, pix[slot] );
      rmesa->hw.pix[slot].cmd[PIX_PP_TXCBLEND] = color_combine;
      rmesa->hw.pix[slot].cmd[PIX_PP_TXABLEND] = alpha_combine;
      rmesa->hw.pix[slot].cmd[PIX_PP_TXCBLEND2] = color_scale;
      rmesa->hw.pix[slot].cmd[PIX_PP_TXABLEND2] = alpha_scale;
   }

   return GL_TRUE;
}

#define REF_COLOR 1
#define REF_ALPHA 2

static GLboolean r200UpdateAllTexEnv( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLint i, j, currslot;
   GLint maxunitused = -1;
   GLboolean texregfree[6] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
   GLubyte stageref[7] = {0, 0, 0, 0, 0, 0, 0};
   GLint nextunit[R200_MAX_TEXTURE_UNITS] = {0, 0, 0, 0, 0, 0};
   GLint currentnext = -1;
   GLboolean ok;

   /* find highest used unit */
   for ( j = 0; j < R200_MAX_TEXTURE_UNITS; j++) {
      if (ctx->Texture.Unit[j]._ReallyEnabled) {
	 maxunitused = j;
      }
   }
   stageref[maxunitused + 1] = REF_COLOR | REF_ALPHA;

   for ( j = maxunitused; j >= 0; j-- ) {
      const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[j];

      rmesa->state.texture.unit[j].outputreg = -1;

      if (stageref[j + 1]) {

	 /* use the lowest available reg. That gets us automatically reg0 for the last stage.
	    need this even for disabled units, as it may get referenced due to the replace
	    optimization */
	 for ( i = 0 ; i < R200_MAX_TEXTURE_UNITS; i++ ) {
	    if (texregfree[i]) {
	       rmesa->state.texture.unit[j].outputreg = i;
	       break;
	    }
	 }
	 if (rmesa->state.texture.unit[j].outputreg == -1) {
	    /* no more free regs we can use. Need a fallback :-( */
	    return GL_FALSE;
         }

         nextunit[j] = currentnext;

         if (!texUnit->_ReallyEnabled) {
	 /* the not enabled stages are referenced "indirectly",
            must not cut off the lower stages */
	    stageref[j] = REF_COLOR | REF_ALPHA;
	    continue;
         }
	 currentnext = j;
 
	 const GLuint numColorArgs = texUnit->_CurrentCombine->_NumArgsRGB;
	 const GLuint numAlphaArgs = texUnit->_CurrentCombine->_NumArgsA;
	 const GLboolean isdot3rgba = (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA) ||
				      (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA_EXT);


	 /* check if we need the color part, special case for dot3_rgba
	    as if only the alpha part is referenced later on it still is using the color part */
	 if ((stageref[j + 1] & REF_COLOR) || isdot3rgba) {
	    for ( i = 0 ; i < numColorArgs ; i++ ) {
	       const GLuint srcRGBi = texUnit->_CurrentCombine->SourceRGB[i];
	       const GLuint op = texUnit->_CurrentCombine->OperandRGB[i];
	       switch ( srcRGBi ) {
	       case GL_PREVIOUS:
		  /* op 0/1 are referencing color, op 2/3 alpha */
		  stageref[j] |= (op >> 1) + 1;
	          break;
	       case GL_TEXTURE:
		  texregfree[j] = GL_FALSE;
		  break;
	       case GL_TEXTURE0:
	       case GL_TEXTURE1:
	       case GL_TEXTURE2:
	       case GL_TEXTURE3:
	       case GL_TEXTURE4:
	       case GL_TEXTURE5:
		  texregfree[srcRGBi - GL_TEXTURE0] = GL_FALSE;
	          break;
	       default: /* don't care about other sources here */
		  break;
	       }
	    }
	 }

	 /* alpha args are ignored for dot3_rgba */
	 if ((stageref[j + 1] & REF_ALPHA) && !isdot3rgba) {

	    for ( i = 0 ; i < numAlphaArgs ; i++ ) {
	       const GLuint srcAi = texUnit->_CurrentCombine->SourceA[i];
	       switch ( srcAi ) {
	       case GL_PREVIOUS:
		  stageref[j] |= REF_ALPHA;
		  break;
	       case GL_TEXTURE:
		  texregfree[j] = GL_FALSE;
		  break;
	       case GL_TEXTURE0:
	       case GL_TEXTURE1:
	       case GL_TEXTURE2:
	       case GL_TEXTURE3:
	       case GL_TEXTURE4:
	       case GL_TEXTURE5:
		  texregfree[srcAi - GL_TEXTURE0] = GL_FALSE;
		  break;
	       default: /* don't care about other sources here */
		  break;
	       }
	    }
	 }
      }
   }

   /* don't enable texture sampling for units if the result is not used */
   for (i = 0; i < R200_MAX_TEXTURE_UNITS; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled && !texregfree[i])
	 rmesa->state.texture.unit[i].unitneeded = ctx->Texture.Unit[i]._ReallyEnabled;
      else rmesa->state.texture.unit[i].unitneeded = 0;
   }

   ok = GL_TRUE;
   currslot = 0;
   rmesa->state.envneeded = 1;

   i = 0;
   while ((i <= maxunitused) && (i >= 0)) {
      /* only output instruction if the results are referenced */
      if (ctx->Texture.Unit[i]._ReallyEnabled && stageref[i+1]) {
         GLuint replaceunit = i;
	 /* try to optimize GL_REPLACE away (only one level deep though) */
	 if (	(ctx->Texture.Unit[i]._CurrentCombine->ModeRGB == GL_REPLACE) &&
		(ctx->Texture.Unit[i]._CurrentCombine->ModeA == GL_REPLACE) &&
		(ctx->Texture.Unit[i]._CurrentCombine->ScaleShiftRGB == 0) &&
		(ctx->Texture.Unit[i]._CurrentCombine->ScaleShiftA == 0) &&
		(nextunit[i] > 0) ) {
	    /* yippie! can optimize it away! */
	    replaceunit = i;
	    i = nextunit[i];
	 }

	 /* need env instruction slot */
	 rmesa->state.envneeded |= 1 << currslot;
	 ok = r200UpdateTextureEnv( ctx, i, currslot, replaceunit );
	 if (!ok) return GL_FALSE;
	 currslot++;
      }
      i = i + 1;
   }

   if (currslot == 0) {
      /* need one stage at least */
      rmesa->state.texture.unit[0].outputreg = 0;
      ok = r200UpdateTextureEnv( ctx, 0, 0, 0 );
   }

   R200_STATECHANGE( rmesa, ctx );
   rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~(R200_TEX_BLEND_ENABLE_MASK | R200_MULTI_PASS_ENABLE);
   rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= rmesa->state.envneeded << R200_TEX_BLEND_0_ENABLE_SHIFT;

   return ok;
}

#undef REF_COLOR
#undef REF_ALPHA


#define TEXOBJ_TXFILTER_MASK (R200_MAX_MIP_LEVEL_MASK |		\
			      R200_MIN_FILTER_MASK | 		\
			      R200_MAG_FILTER_MASK |		\
			      R200_MAX_ANISO_MASK |		\
			      R200_YUV_TO_RGB |			\
			      R200_YUV_TEMPERATURE_MASK |	\
			      R200_CLAMP_S_MASK | 		\
			      R200_CLAMP_T_MASK | 		\
			      R200_BORDER_MODE_D3D )

#define TEXOBJ_TXFORMAT_MASK (R200_TXFORMAT_WIDTH_MASK |	\
			      R200_TXFORMAT_HEIGHT_MASK |	\
			      R200_TXFORMAT_FORMAT_MASK |	\
			      R200_TXFORMAT_F5_WIDTH_MASK |	\
			      R200_TXFORMAT_F5_HEIGHT_MASK |	\
			      R200_TXFORMAT_ALPHA_IN_MAP |	\
			      R200_TXFORMAT_CUBIC_MAP_ENABLE |	\
			      R200_TXFORMAT_NON_POWER2)

#define TEXOBJ_TXFORMAT_X_MASK (R200_DEPTH_LOG2_MASK |		\
                                R200_TEXCOORD_MASK |		\
                                R200_CLAMP_Q_MASK | 		\
                                R200_VOLUME_FILTER_MASK)


static void import_tex_obj_state( r200ContextPtr rmesa,
				  int unit,
				  r200TexObjPtr texobj )
{
/* do not use RADEON_DB_STATE to avoid stale texture caches */
   int *cmd = &rmesa->hw.tex[unit].cmd[TEX_CMD_0];

   R200_STATECHANGE( rmesa, tex[unit] );

   cmd[TEX_PP_TXFILTER] &= ~TEXOBJ_TXFILTER_MASK;
   cmd[TEX_PP_TXFILTER] |= texobj->pp_txfilter & TEXOBJ_TXFILTER_MASK;
   cmd[TEX_PP_TXFORMAT] &= ~TEXOBJ_TXFORMAT_MASK;
   cmd[TEX_PP_TXFORMAT] |= texobj->pp_txformat & TEXOBJ_TXFORMAT_MASK;
   cmd[TEX_PP_TXFORMAT_X] &= ~TEXOBJ_TXFORMAT_X_MASK;
   cmd[TEX_PP_TXFORMAT_X] |= texobj->pp_txformat_x & TEXOBJ_TXFORMAT_X_MASK;
   cmd[TEX_PP_TXSIZE] = texobj->pp_txsize; /* NPOT only! */
   cmd[TEX_PP_TXPITCH] = texobj->pp_txpitch; /* NPOT only! */
   cmd[TEX_PP_BORDER_COLOR] = texobj->pp_border_color;
   if (rmesa->r200Screen->drmSupportsFragShader) {
      cmd[TEX_PP_TXOFFSET_NEWDRM] = texobj->pp_txoffset;
   }
   else {
      cmd[TEX_PP_TXOFFSET_OLDDRM] = texobj->pp_txoffset;
   }

   if (texobj->base.tObj->Target == GL_TEXTURE_CUBE_MAP) {
      int *cube_cmd = &rmesa->hw.cube[unit].cmd[CUBE_CMD_0];
      GLuint bytesPerFace = texobj->base.totalSize / 6;
      ASSERT(texobj->base.totalSize % 6 == 0);

      R200_STATECHANGE( rmesa, cube[unit] );
      cube_cmd[CUBE_PP_CUBIC_FACES] = texobj->pp_cubic_faces;
      if (rmesa->r200Screen->drmSupportsFragShader) {
	 /* that value is submitted twice. could change cube atom
	    to not include that command when new drm is used */
	 cmd[TEX_PP_CUBIC_FACES] = texobj->pp_cubic_faces;
      }
      cube_cmd[CUBE_PP_CUBIC_OFFSET_F1] = texobj->pp_txoffset + 1 * bytesPerFace;
      cube_cmd[CUBE_PP_CUBIC_OFFSET_F2] = texobj->pp_txoffset + 2 * bytesPerFace;
      cube_cmd[CUBE_PP_CUBIC_OFFSET_F3] = texobj->pp_txoffset + 3 * bytesPerFace;
      cube_cmd[CUBE_PP_CUBIC_OFFSET_F4] = texobj->pp_txoffset + 4 * bytesPerFace;
      cube_cmd[CUBE_PP_CUBIC_OFFSET_F5] = texobj->pp_txoffset + 5 * bytesPerFace;
   }

   texobj->dirty_state &= ~(1<<unit);
}


static void set_texgen_matrix( r200ContextPtr rmesa, 
			       GLuint unit,
			       const GLfloat *s_plane,
			       const GLfloat *t_plane,
			       const GLfloat *r_plane,
			       const GLfloat *q_plane )
{
   GLfloat m[16];

   m[0]  = s_plane[0];
   m[4]  = s_plane[1];
   m[8]  = s_plane[2];
   m[12] = s_plane[3];

   m[1]  = t_plane[0];
   m[5]  = t_plane[1];
   m[9]  = t_plane[2];
   m[13] = t_plane[3];

   m[2]  = r_plane[0];
   m[6]  = r_plane[1];
   m[10] = r_plane[2];
   m[14] = r_plane[3];

   m[3]  = q_plane[0];
   m[7]  = q_plane[1];
   m[11] = q_plane[2];
   m[15] = q_plane[3];

   _math_matrix_loadf( &(rmesa->TexGenMatrix[unit]), m);
   _math_matrix_analyse( &(rmesa->TexGenMatrix[unit]) );
   rmesa->TexGenEnabled |= R200_TEXMAT_0_ENABLE<<unit;
}


static GLuint r200_need_dis_texgen(const GLbitfield texGenEnabled,
				   const GLfloat *planeS,
				   const GLfloat *planeT,
				   const GLfloat *planeR,
				   const GLfloat *planeQ)
{
   GLuint needtgenable = 0;

   if (!(texGenEnabled & S_BIT)) {
      if (((texGenEnabled & T_BIT) && planeT[0] != 0.0) ||
	 ((texGenEnabled & R_BIT) && planeR[0] != 0.0) ||
	 ((texGenEnabled & Q_BIT) && planeQ[0] != 0.0)) {
	 needtgenable |= S_BIT;
      }
   }
   if (!(texGenEnabled & T_BIT)) {
      if (((texGenEnabled & S_BIT) && planeS[1] != 0.0) ||
	 ((texGenEnabled & R_BIT) && planeR[1] != 0.0) ||
	 ((texGenEnabled & Q_BIT) && planeQ[1] != 0.0)) {
	 needtgenable |= T_BIT;
     }
   }
   if (!(texGenEnabled & R_BIT)) {
      if (((texGenEnabled & S_BIT) && planeS[2] != 0.0) ||
	 ((texGenEnabled & T_BIT) && planeT[2] != 0.0) ||
	 ((texGenEnabled & Q_BIT) && planeQ[2] != 0.0)) {
	 needtgenable |= R_BIT;
      }
   }
   if (!(texGenEnabled & Q_BIT)) {
      if (((texGenEnabled & S_BIT) && planeS[3] != 0.0) ||
	 ((texGenEnabled & T_BIT) && planeT[3] != 0.0) ||
	 ((texGenEnabled & R_BIT) && planeR[3] != 0.0)) {
	 needtgenable |= Q_BIT;
      }
   }

   return needtgenable;
}


/*
 * Returns GL_FALSE if fallback required.  
 */
static GLboolean r200_validate_texgen( GLcontext *ctx, GLuint unit )
{  
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   GLuint inputshift = R200_TEXGEN_0_INPUT_SHIFT + unit*4;
   GLuint tgi, tgcm;
   GLuint mode = 0;
   GLboolean mixed_fallback = GL_FALSE;
   static const GLfloat I[16] = {
      1,  0,  0,  0,
      0,  1,  0,  0,
      0,  0,  1,  0,
      0,  0,  0,  1 };
   static const GLfloat reflect[16] = {
      -1,  0,  0,  0,
       0, -1,  0,  0,
       0,  0,  -1, 0,
       0,  0,  0,  1 };

   rmesa->TexGenCompSel &= ~(R200_OUTPUT_TEX_0 << unit);
   rmesa->TexGenEnabled &= ~(R200_TEXGEN_TEXMAT_0_ENABLE<<unit);
   rmesa->TexGenEnabled &= ~(R200_TEXMAT_0_ENABLE<<unit);
   rmesa->TexGenNeedNormals[unit] = GL_FALSE;
   tgi = rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_1] & ~(R200_TEXGEN_INPUT_MASK <<
						   inputshift);
   tgcm = rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_2] & ~(R200_TEXGEN_COMP_MASK <<
						    (unit * 4));

   if (0) 
      fprintf(stderr, "%s unit %d\n", __FUNCTION__, unit);

   if (texUnit->TexGenEnabled & S_BIT) {
      mode = texUnit->GenModeS;
   } else {
      tgcm |= R200_TEXGEN_COMP_S << (unit * 4);
   }

   if (texUnit->TexGenEnabled & T_BIT) {
      if (texUnit->GenModeT != mode)
	 mixed_fallback = GL_TRUE;
   } else {
      tgcm |= R200_TEXGEN_COMP_T << (unit * 4);
   }

   if (texUnit->TexGenEnabled & R_BIT) {
      if (texUnit->GenModeR != mode)
	 mixed_fallback = GL_TRUE;
   } else {
      tgcm |= R200_TEXGEN_COMP_R << (unit * 4);
   }

   if (texUnit->TexGenEnabled & Q_BIT) {
      if (texUnit->GenModeQ != mode)
	 mixed_fallback = GL_TRUE;
   } else {
      tgcm |= R200_TEXGEN_COMP_Q << (unit * 4);
   }

   if (mixed_fallback) {
      if (R200_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "fallback mixed texgen, 0x%x (0x%x 0x%x 0x%x 0x%x)\n",
		 texUnit->TexGenEnabled, texUnit->GenModeS, texUnit->GenModeT,
		 texUnit->GenModeR, texUnit->GenModeQ);
      return GL_FALSE;
   }

/* we CANNOT do mixed mode if the texgen mode requires a plane where the input
   is not enabled for texgen, since the planes are concatenated into texmat,
   and thus the input will come from texcoord rather than tex gen equation!
   Either fallback or just hope that those texcoords aren't really needed...
   Assuming the former will cause lots of unnecessary fallbacks, the latter will
   generate bogus results sometimes - it's pretty much impossible to really know
   when a fallback is needed, depends on texmat and what sort of texture is bound
   etc, - for now fallback if we're missing either S or T bits, there's a high
   probability we need the texcoords in that case.
   That's a lot of work for some obscure texgen mixed mode fixup - why oh why
   doesn't the chip just directly accept the plane parameters :-(. */
   switch (mode) {
   case GL_OBJECT_LINEAR: {
      GLuint needtgenable = r200_need_dis_texgen( texUnit->TexGenEnabled,
				texUnit->ObjectPlaneS, texUnit->ObjectPlaneT,
				texUnit->ObjectPlaneR, texUnit->ObjectPlaneQ );
      if (needtgenable & (S_BIT | T_BIT)) {
	 if (R200_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "fallback mixed texgen / obj plane, 0x%x\n",
		 texUnit->TexGenEnabled);
	 return GL_FALSE;
      }
      if (needtgenable & (R_BIT)) {
	 tgcm &= ~(R200_TEXGEN_COMP_R << (unit * 4));
      }
      if (needtgenable & (Q_BIT)) {
	 tgcm &= ~(R200_TEXGEN_COMP_Q << (unit * 4));
      }

      tgi |= R200_TEXGEN_INPUT_OBJ << inputshift;
      set_texgen_matrix( rmesa, unit, 
	 (texUnit->TexGenEnabled & S_BIT) ? texUnit->ObjectPlaneS : I,
	 (texUnit->TexGenEnabled & T_BIT) ? texUnit->ObjectPlaneT : I + 4,
	 (texUnit->TexGenEnabled & R_BIT) ? texUnit->ObjectPlaneR : I + 8,
	 (texUnit->TexGenEnabled & Q_BIT) ? texUnit->ObjectPlaneQ : I + 12);
      }
      break;

   case GL_EYE_LINEAR: {
      GLuint needtgenable = r200_need_dis_texgen( texUnit->TexGenEnabled,
				texUnit->EyePlaneS, texUnit->EyePlaneT,
				texUnit->EyePlaneR, texUnit->EyePlaneQ );
      if (needtgenable & (S_BIT | T_BIT)) {
	 if (R200_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "fallback mixed texgen / eye plane, 0x%x\n",
		 texUnit->TexGenEnabled);
	 return GL_FALSE;
      }
      if (needtgenable & (R_BIT)) {
	 tgcm &= ~(R200_TEXGEN_COMP_R << (unit * 4));
      }
      if (needtgenable & (Q_BIT)) {
	 tgcm &= ~(R200_TEXGEN_COMP_Q << (unit * 4));
      }
      tgi |= R200_TEXGEN_INPUT_EYE << inputshift;
      set_texgen_matrix( rmesa, unit,
	 (texUnit->TexGenEnabled & S_BIT) ? texUnit->EyePlaneS : I,
	 (texUnit->TexGenEnabled & T_BIT) ? texUnit->EyePlaneT : I + 4,
	 (texUnit->TexGenEnabled & R_BIT) ? texUnit->EyePlaneR : I + 8,
	 (texUnit->TexGenEnabled & Q_BIT) ? texUnit->EyePlaneQ : I + 12);
      }
      break;

   case GL_REFLECTION_MAP_NV:
      rmesa->TexGenNeedNormals[unit] = GL_TRUE;
      tgi |= R200_TEXGEN_INPUT_EYE_REFLECT << inputshift;
      /* pretty weird, must only negate when lighting is enabled? */
      if (ctx->Light.Enabled)
	 set_texgen_matrix( rmesa, unit, 
	    (texUnit->TexGenEnabled & S_BIT) ? reflect : I,
	    (texUnit->TexGenEnabled & T_BIT) ? reflect + 4 : I + 4,
	    (texUnit->TexGenEnabled & R_BIT) ? reflect + 8 : I + 8,
	    I + 12);
      break;

   case GL_NORMAL_MAP_NV:
      rmesa->TexGenNeedNormals[unit] = GL_TRUE;
      tgi |= R200_TEXGEN_INPUT_EYE_NORMAL<<inputshift;
      break;

   case GL_SPHERE_MAP:
      rmesa->TexGenNeedNormals[unit] = GL_TRUE;
      tgi |= R200_TEXGEN_INPUT_SPHERE<<inputshift;
      break;

   case 0:
      /* All texgen units were disabled, so just pass coords through. */
      tgi |= unit << inputshift;
      break;

   default:
      /* Unsupported mode, fallback:
       */
      if (R200_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "fallback unsupported texgen, %d\n",
		 texUnit->GenModeS);
      return GL_FALSE;
   }

   rmesa->TexGenEnabled |= R200_TEXGEN_TEXMAT_0_ENABLE << unit;
   rmesa->TexGenCompSel |= R200_OUTPUT_TEX_0 << unit;

   if (tgi != rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_1] || 
       tgcm != rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_2])
   {
      R200_STATECHANGE(rmesa, tcg);
      rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_1] = tgi;
      rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_2] = tgcm;
   }

   return GL_TRUE;
}


static void disable_tex( GLcontext *ctx, int unit )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   if (rmesa->hw.ctx.cmd[CTX_PP_CNTL] & (R200_TEX_0_ENABLE<<unit)) {
      /* Texture unit disabled */
      if ( rmesa->state.texture.unit[unit].texobj != NULL ) {
	 /* The old texture is no longer bound to this texture unit.
	  * Mark it as such.
	  */

	 rmesa->state.texture.unit[unit].texobj->base.bound &= ~(1UL << unit);
	 rmesa->state.texture.unit[unit].texobj = NULL;
      }

      R200_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~(R200_TEX_0_ENABLE << unit);
	 
      R200_STATECHANGE( rmesa, vtx );
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_1] &= ~(7 << (unit * 3));
	 
      if (rmesa->TclFallback & (R200_TCL_FALLBACK_TEXGEN_0<<unit)) {
	 TCL_FALLBACK( ctx, (R200_TCL_FALLBACK_TEXGEN_0<<unit), GL_FALSE);
      }

      /* Actually want to keep all units less than max active texture
       * enabled, right?  Fix this for >2 texunits.
       */

      {
	 GLuint tmp = rmesa->TexGenEnabled;

	 rmesa->TexGenEnabled &= ~(R200_TEXGEN_TEXMAT_0_ENABLE<<unit);
	 rmesa->TexGenEnabled &= ~(R200_TEXMAT_0_ENABLE<<unit);
	 rmesa->TexGenNeedNormals[unit] = GL_FALSE;
	 rmesa->TexGenCompSel &= ~(R200_OUTPUT_TEX_0 << unit);

	 if (tmp != rmesa->TexGenEnabled) {
	    rmesa->recheck_texgen[unit] = GL_TRUE;
	    rmesa->NewGLState |= _NEW_TEXTURE_MATRIX;
	 }
      }
   }
}

void set_re_cntl_d3d( GLcontext *ctx, int unit, GLboolean use_d3d )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   GLuint re_cntl;

   re_cntl = rmesa->hw.set.cmd[SET_RE_CNTL] & ~(R200_VTX_STQ0_D3D << (2 * unit));
   if (use_d3d)
      re_cntl |= R200_VTX_STQ0_D3D << (2 * unit);

   if ( re_cntl != rmesa->hw.set.cmd[SET_RE_CNTL] ) {
      R200_STATECHANGE( rmesa, set );
      rmesa->hw.set.cmd[SET_RE_CNTL] = re_cntl;
   }
}

static GLboolean enable_tex_2d( GLcontext *ctx, int unit )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   r200TexObjPtr t = (r200TexObjPtr) tObj->DriverData;

   /* Need to load the 2d images associated with this unit.
    */
   if (t->pp_txformat & R200_TXFORMAT_NON_POWER2) {
      t->pp_txformat &= ~R200_TXFORMAT_NON_POWER2;
      t->base.dirty_images[0] = ~0;
   }

   ASSERT(tObj->Target == GL_TEXTURE_2D || tObj->Target == GL_TEXTURE_1D);

   if ( t->base.dirty_images[0] ) {
      R200_FIREVERTICES( rmesa );
      r200SetTexImages( rmesa, tObj );
      r200UploadTexImages( rmesa, (r200TexObjPtr) tObj->DriverData, 0 );
      if ( !t->base.memBlock ) 
	 return GL_FALSE;
   }

   set_re_cntl_d3d( ctx, unit, GL_FALSE );

   return GL_TRUE;
}

#if ENABLE_HW_3D_TEXTURE
static GLboolean enable_tex_3d( GLcontext *ctx, int unit )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   r200TexObjPtr t = (r200TexObjPtr) tObj->DriverData;

   /* Need to load the 3d images associated with this unit.
    */
   if (t->pp_txformat & R200_TXFORMAT_NON_POWER2) {
      t->pp_txformat &= ~R200_TXFORMAT_NON_POWER2;
      t->base.dirty_images[0] = ~0;
   }

   ASSERT(tObj->Target == GL_TEXTURE_3D);

   /* R100 & R200 do not support mipmaps for 3D textures.
    */
   if ( (tObj->MinFilter != GL_NEAREST) && (tObj->MinFilter != GL_LINEAR) ) {
      return GL_FALSE;
   }

   if ( t->base.dirty_images[0] ) {
      R200_FIREVERTICES( rmesa );
      r200SetTexImages( rmesa, tObj );
      r200UploadTexImages( rmesa, (r200TexObjPtr) tObj->DriverData, 0 );
      if ( !t->base.memBlock ) 
	 return GL_FALSE;
   }

   set_re_cntl_d3d( ctx, unit, GL_TRUE );

   return GL_TRUE;
}
#endif

static GLboolean enable_tex_cube( GLcontext *ctx, int unit )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   r200TexObjPtr t = (r200TexObjPtr) tObj->DriverData;
   GLuint face;

   /* Need to load the 2d images associated with this unit.
    */
   if (t->pp_txformat & R200_TXFORMAT_NON_POWER2) {
      t->pp_txformat &= ~R200_TXFORMAT_NON_POWER2;
      for (face = 0; face < 6; face++)
         t->base.dirty_images[face] = ~0;
   }

   ASSERT(tObj->Target == GL_TEXTURE_CUBE_MAP);

   if ( t->base.dirty_images[0] || t->base.dirty_images[1] ||
        t->base.dirty_images[2] || t->base.dirty_images[3] ||
        t->base.dirty_images[4] || t->base.dirty_images[5] ) {
      /* flush */
      R200_FIREVERTICES( rmesa );
      /* layout memory space, once for all faces */
      r200SetTexImages( rmesa, tObj );
   }

   /* upload (per face) */
   for (face = 0; face < 6; face++) {
      if (t->base.dirty_images[face]) {
         r200UploadTexImages( rmesa, (r200TexObjPtr) tObj->DriverData, face );
      }
   }
      
   if ( !t->base.memBlock ) {
      /* texmem alloc failed, use s/w fallback */
      return GL_FALSE;
   }

   set_re_cntl_d3d( ctx, unit, GL_TRUE );

   return GL_TRUE;
}

static GLboolean enable_tex_rect( GLcontext *ctx, int unit )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   r200TexObjPtr t = (r200TexObjPtr) tObj->DriverData;

   if (!(t->pp_txformat & R200_TXFORMAT_NON_POWER2)) {
      t->pp_txformat |= R200_TXFORMAT_NON_POWER2;
      t->base.dirty_images[0] = ~0;
   }

   ASSERT(tObj->Target == GL_TEXTURE_RECTANGLE_NV);

   if ( t->base.dirty_images[0] ) {
      R200_FIREVERTICES( rmesa );
      r200SetTexImages( rmesa, tObj );
      r200UploadTexImages( rmesa, (r200TexObjPtr) tObj->DriverData, 0 );
      if ( !t->base.memBlock && !rmesa->prefer_gart_client_texturing ) 
	 return GL_FALSE;
   }

   set_re_cntl_d3d( ctx, unit, GL_FALSE );

   return GL_TRUE;
}


static GLboolean update_tex_common( GLcontext *ctx, int unit )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   r200TexObjPtr t = (r200TexObjPtr) tObj->DriverData;

   /* Fallback if there's a texture border */
   if ( tObj->Image[0][tObj->BaseLevel]->Border > 0 )
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
   if ( 1|| !(rmesa->hw.ctx.cmd[CTX_PP_CNTL] & (R200_TEX_0_ENABLE<<unit))) {
      R200_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= R200_TEX_0_ENABLE << unit;

      R200_STATECHANGE( rmesa, vtx );
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_1] &= ~(7 << (unit * 3));
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_1] |= 4 << (unit * 3);

      rmesa->recheck_texgen[unit] = GL_TRUE;
   }

   if (t->dirty_state & (1<<unit)) {
      import_tex_obj_state( rmesa, unit, t );
   }

   if (rmesa->recheck_texgen[unit]) {
      GLboolean fallback = !r200_validate_texgen( ctx, unit );
      TCL_FALLBACK( ctx, (R200_TCL_FALLBACK_TEXGEN_0<<unit), fallback);
      rmesa->recheck_texgen[unit] = 0;
      rmesa->NewGLState |= _NEW_TEXTURE_MATRIX;
   }

   FALLBACK( rmesa, R200_FALLBACK_BORDER_MODE, t->border_fallback );
   return !t->border_fallback;
}



static GLboolean r200UpdateTextureUnit( GLcontext *ctx, int unit )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint unitneeded = rmesa->state.texture.unit[unit].unitneeded;

   if ( unitneeded & (TEXTURE_RECT_BIT) ) {
      return (enable_tex_rect( ctx, unit ) &&
	      update_tex_common( ctx, unit ));
   }
   else if ( unitneeded & (TEXTURE_1D_BIT | TEXTURE_2D_BIT) ) {
      return (enable_tex_2d( ctx, unit ) &&
	      update_tex_common( ctx, unit ));
   }
#if ENABLE_HW_3D_TEXTURE
   else if ( unitneeded & (TEXTURE_3D_BIT) ) {
      return (enable_tex_3d( ctx, unit ) &&
	      update_tex_common( ctx, unit ));
   }
#endif
   else if ( unitneeded & (TEXTURE_CUBE_BIT) ) {
      return (enable_tex_cube( ctx, unit ) &&
	      update_tex_common( ctx, unit ));
   }
   else if ( unitneeded ) {
      return GL_FALSE;
   }
   else {
      disable_tex( ctx, unit );
      return GL_TRUE;
   }
}


void r200UpdateTextureState( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLboolean ok;
   GLuint dbg;

   if (ctx->ATIFragmentShader._Enabled) {
      GLuint i;
      for (i = 0; i < R200_MAX_TEXTURE_UNITS; i++) {
	 rmesa->state.texture.unit[i].unitneeded = ctx->Texture.Unit[i]._ReallyEnabled;
      }
      ok = GL_TRUE;
   }
   else {
      ok = r200UpdateAllTexEnv( ctx );
   }
   if (ok) {
      ok = (r200UpdateTextureUnit( ctx, 0 ) &&
	 r200UpdateTextureUnit( ctx, 1 ) &&
	 r200UpdateTextureUnit( ctx, 2 ) &&
	 r200UpdateTextureUnit( ctx, 3 ) &&
	 r200UpdateTextureUnit( ctx, 4 ) &&
	 r200UpdateTextureUnit( ctx, 5 ));
   }

   if (ok && ctx->ATIFragmentShader._Enabled) {
      r200UpdateFragmentShader(ctx);
   }

   FALLBACK( rmesa, R200_FALLBACK_TEXTURE, !ok );

   if (rmesa->TclFallback)
      r200ChooseVertexState( ctx );


   if (rmesa->r200Screen->chip_family == CHIP_FAMILY_R200) {

      /*
       * T0 hang workaround -------------
       * not needed for r200 derivatives
        */
      if ((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & R200_TEX_ENABLE_MASK) == R200_TEX_0_ENABLE &&
	 (rmesa->hw.tex[0].cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK) > R200_MIN_FILTER_LINEAR) {

	 R200_STATECHANGE(rmesa, ctx);
	 R200_STATECHANGE(rmesa, tex[1]);
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= R200_TEX_1_ENABLE;
	 if (!(rmesa->hw.cst.cmd[CST_PP_CNTL_X] & R200_PPX_TEX_1_ENABLE))
	    rmesa->hw.tex[1].cmd[TEX_PP_TXFORMAT] &= ~TEXOBJ_TXFORMAT_MASK;
	 rmesa->hw.tex[1].cmd[TEX_PP_TXFORMAT] |= R200_TXFORMAT_LOOKUP_DISABLE;
      }
      else if (!ctx->ATIFragmentShader._Enabled) {
	 if ((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & R200_TEX_1_ENABLE) &&
	    (rmesa->hw.tex[1].cmd[TEX_PP_TXFORMAT] & R200_TXFORMAT_LOOKUP_DISABLE)) {
	    R200_STATECHANGE(rmesa, tex[1]);
	    rmesa->hw.tex[1].cmd[TEX_PP_TXFORMAT] &= ~R200_TXFORMAT_LOOKUP_DISABLE;
         }
      }
      /* do the same workaround for the first pass of a fragment shader.
       * completely unknown if necessary / sufficient.
       */
      if ((rmesa->hw.cst.cmd[CST_PP_CNTL_X] & R200_PPX_TEX_ENABLE_MASK) == R200_PPX_TEX_0_ENABLE &&
	 (rmesa->hw.tex[0].cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK) > R200_MIN_FILTER_LINEAR) {

	 R200_STATECHANGE(rmesa, cst);
	 R200_STATECHANGE(rmesa, tex[1]);
	 rmesa->hw.cst.cmd[CST_PP_CNTL_X] |= R200_PPX_TEX_1_ENABLE;
	 if (!(rmesa->hw.ctx.cmd[CTX_PP_CNTL] & R200_TEX_1_ENABLE))
	    rmesa->hw.tex[1].cmd[TEX_PP_TXFORMAT] &= ~TEXOBJ_TXFORMAT_MASK;
	 rmesa->hw.tex[1].cmd[TEX_PP_TXMULTI_CTL] |= R200_PASS1_TXFORMAT_LOOKUP_DISABLE;
      }

      /* maybe needs to be done pairwise due to 2 parallel (physical) tex units ?
         looks like that's not the case, if 8500/9100 owners don't complain remove this...
      for ( i = 0; i < ctx->Const.MaxTextureUnits; i += 2) {
         if (((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & ((R200_TEX_0_ENABLE |
            R200_TEX_1_ENABLE ) << i)) == (R200_TEX_0_ENABLE << i)) &&
            ((rmesa->hw.tex[i].cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK) >
            R200_MIN_FILTER_LINEAR)) {
            R200_STATECHANGE(rmesa, ctx);
            R200_STATECHANGE(rmesa, tex[i+1]);
            rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= (R200_TEX_1_ENABLE << i);
            rmesa->hw.tex[i+1].cmd[TEX_PP_TXFORMAT] &= ~TEXOBJ_TXFORMAT_MASK;
            rmesa->hw.tex[i+1].cmd[TEX_PP_TXFORMAT] |= 0x08000000;
         }
         else {
            if ((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & (R200_TEX_1_ENABLE << i)) &&
               (rmesa->hw.tex[i+1].cmd[TEX_PP_TXFORMAT] & 0x08000000)) {
               R200_STATECHANGE(rmesa, tex[i+1]);
               rmesa->hw.tex[i+1].cmd[TEX_PP_TXFORMAT] &= ~0x08000000;
            }
         }
      } */

      /*
       * Texture cache LRU hang workaround -------------
       * not needed for r200 derivatives
       * hopefully this covers first pass of a shader as well
       */

      /* While the cases below attempt to only enable the workaround in the
       * specific cases necessary, they were insufficient.  See bugzilla #1519,
       * #729, #814.  Tests with quake3 showed no impact on performance.
       */
      dbg = 0x6;

      /*
      if (((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & (R200_TEX_0_ENABLE )) &&
         ((((rmesa->hw.tex[0].cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK)) &
         0x04) == 0)) ||
         ((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & R200_TEX_2_ENABLE) &&
         ((((rmesa->hw.tex[2].cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK)) &
         0x04) == 0)) ||
         ((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & R200_TEX_4_ENABLE) &&
         ((((rmesa->hw.tex[4].cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK)) &
         0x04) == 0)))
      {
         dbg |= 0x02;
      }

      if (((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & (R200_TEX_1_ENABLE )) &&
         ((((rmesa->hw.tex[1].cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK)) &
         0x04) == 0)) ||
         ((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & R200_TEX_3_ENABLE) &&
         ((((rmesa->hw.tex[3].cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK)) &
         0x04) == 0)) ||
         ((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & R200_TEX_5_ENABLE) &&
         ((((rmesa->hw.tex[5].cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK)) &
         0x04) == 0)))
      {
         dbg |= 0x04;
      }*/

      if (dbg != rmesa->hw.tam.cmd[TAM_DEBUG3]) {
         R200_STATECHANGE( rmesa, tam );
         rmesa->hw.tam.cmd[TAM_DEBUG3] = dbg;
         if (0) printf("TEXCACHE LRU HANG WORKAROUND %x\n", dbg);
      }
   }
}
