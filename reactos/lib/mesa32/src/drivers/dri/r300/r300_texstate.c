/* $XFree86: xc/lib/GL/mesa/src/drv/r300/r300_texstate.c,v 1.3 2003/02/15 22:18:47 dawes Exp $ */
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

#include "r300_context.h"
#include "r300_state.h"
#include "r300_ioctl.h"
#include "radeon_ioctl.h"
//#include "r300_swtcl.h"
#include "r300_tex.h"
//#include "r300_tcl.h"
#include "r300_reg.h"

#define R200_TXFORMAT_A8        R200_TXFORMAT_I8
#define R200_TXFORMAT_L8        R200_TXFORMAT_I8
#define R200_TXFORMAT_AL88      R200_TXFORMAT_AI88
#define R200_TXFORMAT_YCBCR     R200_TXFORMAT_YVYU422
#define R200_TXFORMAT_YCBCR_REV R200_TXFORMAT_VYUY422

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
#define VALID_FORMAT(f) ( ((f) <= MESA_FORMAT_YCBCR_REV) \
			     && tx_table[f].flag )

#define _ASSIGN(entry, format)	\
	[ MESA_FORMAT_ ## entry ] = { format, 0, 1}

static const struct {
	GLuint format, filter;
} tx_table0[] = {
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
	    };

static const struct {
	GLuint format, filter, flag;
} tx_table[] = {
	/*
	 * Note that the _REV formats are the same as the non-REV formats.
	 * This is because the REV and non-REV formats are identical as a
	 * byte string, but differ when accessed as 16-bit or 32-bit words
	 * depending on the endianness of the host.  Since the textures are
	 * transferred to the R300 as a byte string (i.e. without any
	 * byte-swapping), the R300 sees the REV and non-REV formats
	 * identically.  -- paulus
	 */
	    _ASSIGN(RGBA8888, R300_EASY_TX_FORMAT(Y, Z, W, X, W8Z8Y8X8)),
	    _ASSIGN(RGBA8888_REV, R300_EASY_TX_FORMAT(Y, Z, W, X, W8Z8Y8X8)),
	    _ASSIGN(ARGB8888, R300_EASY_TX_FORMAT(X, Y, Z, W, W8Z8Y8X8)),
	    _ASSIGN(ARGB8888_REV, R300_EASY_TX_FORMAT(X, Y, Z, W, W8Z8Y8X8)),
	    _ASSIGN(RGB888, 0xffffffff),
	    _ASSIGN(RGB565, R300_EASY_TX_FORMAT(X, Y, Z, ONE, Z5Y6X5)),
	    _ASSIGN(RGB565_REV, R300_EASY_TX_FORMAT(X, Y, Z, ONE, Z5Y6X5)),
	    _ASSIGN(ARGB4444, R300_EASY_TX_FORMAT(X, Y, Z, W, W4Z4Y4X4)),
	    _ASSIGN(ARGB4444_REV, R300_EASY_TX_FORMAT(X, Y, Z, W, W4Z4Y4X4)),
	    _ASSIGN(ARGB1555, R300_EASY_TX_FORMAT(Z, Y, X, W, W1Z5Y5X5)),
	    _ASSIGN(ARGB1555_REV, R300_EASY_TX_FORMAT(Z, Y, X, W, W1Z5Y5X5)),
	    _ASSIGN(AL88, R300_EASY_TX_FORMAT(Y, Y, Y, X, Y8X8)),
	    _ASSIGN(AL88_REV, R300_EASY_TX_FORMAT(Y, Y, Y, X, Y8X8)),
	    _ASSIGN(RGB332, R300_EASY_TX_FORMAT(X, Y, Z, ONE, Z3Y3X2)),
	    _ASSIGN(A8, R300_EASY_TX_FORMAT(ZERO, ZERO, ZERO, X, X8)),
	    _ASSIGN(L8, R300_EASY_TX_FORMAT(X, X, X, ONE, X8)),
	    _ASSIGN(I8, R300_EASY_TX_FORMAT(X, X, X, X, X8)),
	    _ASSIGN(CI8, R300_EASY_TX_FORMAT(X, X, X, X, X8)),
	    _ASSIGN(YCBCR, R300_EASY_TX_FORMAT(X, Y, Z, ONE, G8R8_G8B8)|R300_TX_FORMAT_YUV_MODE ),
	    _ASSIGN(YCBCR_REV, R300_EASY_TX_FORMAT(X, Y, Z, ONE, G8R8_G8B8)|R300_TX_FORMAT_YUV_MODE),
	    };

#undef _COLOR
#undef _ALPHA
#undef _INVALID
#undef _ASSIGN


/**
 * This function computes the number of bytes of storage needed for
 * the given texture object (all mipmap levels, all cube faces).
 * The \c image[face][level].x/y/width/height parameters for upload/blitting
 * are computed here.  \c filter, \c format, etc. will be set here
 * too.
 *
 * \param rmesa Context pointer
 * \param tObj GL texture object whose images are to be posted to
 *                 hardware state.
 */
static void r300SetTexImages(r300ContextPtr rmesa,
			     struct gl_texture_object *tObj)
{
	r300TexObjPtr t = (r300TexObjPtr) tObj->DriverData;
	const struct gl_texture_image *baseImage =
	    tObj->Image[0][tObj->BaseLevel];
	GLint curOffset;
	GLint i;
	GLint numLevels;
	GLint log2Width, log2Height, log2Depth;

	/* Set the hardware texture format
	 */

	t->format &= ~(R200_TXFORMAT_FORMAT_MASK |
			    R200_TXFORMAT_ALPHA_IN_MAP);
#if 0
	t->filter &= ~R200_YUV_TO_RGB;
#endif
	if (VALID_FORMAT(baseImage->TexFormat->MesaFormat)) {
		t->format =
		    tx_table[baseImage->TexFormat->MesaFormat].format;
#if 1
		t->filter |=
		    tx_table[baseImage->TexFormat->MesaFormat].filter;
#endif
	} else {
		_mesa_problem(NULL, "unexpected texture format in %s",
			      __FUNCTION__);
		return;
	}

	/* Compute which mipmap levels we really want to send to the hardware.
	 */

	driCalculateTextureFirstLastLevel((driTextureObject *) t);
	log2Width = tObj->Image[0][t->base.firstLevel]->WidthLog2;
	log2Height = tObj->Image[0][t->base.firstLevel]->HeightLog2;
	log2Depth = tObj->Image[0][t->base.firstLevel]->DepthLog2;

	numLevels = t->base.lastLevel - t->base.firstLevel + 1;

	assert(numLevels <= RADEON_MAX_TEXTURE_LEVELS);

	/* Calculate mipmap offsets and dimensions for blitting (uploading)
	 * The idea is that we lay out the mipmap levels within a block of
	 * memory organized as a rectangle of width BLIT_WIDTH_BYTES.
	 */
	curOffset = 0;

	for (i = 0; i < numLevels; i++) {
		const struct gl_texture_image *texImage;
		GLuint size;

		texImage = tObj->Image[0][i + t->base.firstLevel];
		if (!texImage)
			break;

		/* find image size in bytes */
		if (texImage->IsCompressed) {
			size = texImage->CompressedSize;
		} else if (tObj->Target == GL_TEXTURE_RECTANGLE_NV) {
			size =
			    ((texImage->Width *
			      texImage->TexFormat->TexelBytes + 63)
			     & ~63) * texImage->Height;
		} else {
			int w =
			    texImage->Width * texImage->TexFormat->TexelBytes;
			if (w < 32)
				w = 32;
			size = w * texImage->Height * texImage->Depth;
		}
		assert(size > 0);

		if(0)
			fprintf(stderr, "w=%d h=%d d=%d tb=%d intFormat=%d\n", texImage->Width, texImage->Height,
				texImage->Depth, texImage->TexFormat->TexelBytes,
				texImage->IntFormat);

		/* Align to 32-byte offset.  It is faster to do this unconditionally
		 * (no branch penalty).
		 */

		curOffset = (curOffset + 0x1f) & ~0x1f;

		t->image[0][i].x = curOffset % BLIT_WIDTH_BYTES;
		t->image[0][i].y = curOffset / BLIT_WIDTH_BYTES;
		t->image[0][i].width = MIN2(size, BLIT_WIDTH_BYTES);
		t->image[0][i].height = size / t->image[0][i].width;

#if 0
		/* for debugging only and only  applicable to non-rectangle targets */
		assert(size % t->image[0][i].width == 0);
		assert(t->image[0][i].x == 0
		       || (size < BLIT_WIDTH_BYTES
			   && t->image[0][i].height == 1));
#endif

		if (0)
			fprintf(stderr,
				"level %d: %dx%d x=%d y=%d w=%d h=%d size=%d at %d\n",
				i, texImage->Width, texImage->Height,
				t->image[0][i].x, t->image[0][i].y,
				t->image[0][i].width, t->image[0][i].height,
				size, curOffset);

		curOffset += size;

	}

	/* Align the total size of texture memory block.
	 */
	t->base.totalSize =
	    (curOffset + RADEON_OFFSET_MASK) & ~RADEON_OFFSET_MASK;

	/* Setup remaining cube face blits, if needed */
	if (tObj->Target == GL_TEXTURE_CUBE_MAP) {
		/* Round totalSize up to multiple of BLIT_WIDTH_BYTES */
		const GLuint faceSize =
		    (t->base.totalSize + BLIT_WIDTH_BYTES - 1)
		    & ~(BLIT_WIDTH_BYTES - 1);
		const GLuint lines = faceSize / BLIT_WIDTH_BYTES;
		GLuint face;
		/* reuse face 0 x/y/width/height - just adjust y */
		for (face = 1; face < 6; face++) {
			for (i = 0; i < numLevels; i++) {
				t->image[face][i].x = t->image[0][i].x;
				t->image[face][i].y =
				    t->image[0][i].y + face * lines;
				t->image[face][i].width = t->image[0][i].width;
				t->image[face][i].height =
				    t->image[0][i].height;
			}
		}
		t->base.totalSize = 6 * faceSize;	/* total texmem needed */
	}

	/* Hardware state:
	 */
#if 0
	t->filter &= ~R200_MAX_MIP_LEVEL_MASK;
	t->filter |= (numLevels - 1) << R200_MAX_MIP_LEVEL_SHIFT;
#endif
#if 0
	t->format &= ~(R200_TXFORMAT_WIDTH_MASK |
			    R200_TXFORMAT_HEIGHT_MASK |
			    R200_TXFORMAT_CUBIC_MAP_ENABLE |
			    R200_TXFORMAT_F5_WIDTH_MASK |
			    R200_TXFORMAT_F5_HEIGHT_MASK);
	t->format |= ((log2Width << R200_TXFORMAT_WIDTH_SHIFT) |
			   (log2Height << R200_TXFORMAT_HEIGHT_SHIFT));
#endif

	t->format_x &= ~(R200_DEPTH_LOG2_MASK | R200_TEXCOORD_MASK);
	if (tObj->Target == GL_TEXTURE_3D) {
		t->format_x |= (log2Depth << R200_DEPTH_LOG2_SHIFT);
		t->format_x |= R200_TEXCOORD_VOLUME;
	} else if (tObj->Target == GL_TEXTURE_CUBE_MAP) {
		ASSERT(log2Width == log2Height);
		t->format |= ((log2Width << R200_TXFORMAT_F5_WIDTH_SHIFT) |
				   (log2Height << R200_TXFORMAT_F5_HEIGHT_SHIFT)
				   | (R200_TXFORMAT_CUBIC_MAP_ENABLE));
		t->format_x |= R200_TEXCOORD_CUBIC_ENV;
		t->pp_cubic_faces = ((log2Width << R200_FACE_WIDTH_1_SHIFT) |
				     (log2Height << R200_FACE_HEIGHT_1_SHIFT) |
				     (log2Width << R200_FACE_WIDTH_2_SHIFT) |
				     (log2Height << R200_FACE_HEIGHT_2_SHIFT) |
				     (log2Width << R200_FACE_WIDTH_3_SHIFT) |
				     (log2Height << R200_FACE_HEIGHT_3_SHIFT) |
				     (log2Width << R200_FACE_WIDTH_4_SHIFT) |
				     (log2Height << R200_FACE_HEIGHT_4_SHIFT));
	}

	t->size = (((tObj->Image[0][t->base.firstLevel]->Width - 1) << R300_TX_WIDTHMASK_SHIFT)
			|((tObj->Image[0][t->base.firstLevel]->Height - 1) << R300_TX_HEIGHTMASK_SHIFT)
		        |((log2Width>log2Height)?log2Width:log2Height)<<R300_TX_SIZE_SHIFT);

	/* Only need to round to nearest 32 for textures, but the blitter
	 * requires 64-byte aligned pitches, and we may/may not need the
	 * blitter.   NPOT only!
	 */
	if (baseImage->IsCompressed)
		t->pitch =
		    (tObj->Image[0][t->base.firstLevel]->Width + 63) & ~(63);
	else
		t->pitch =
		    ((tObj->Image[0][t->base.firstLevel]->Width *
		      baseImage->TexFormat->TexelBytes) + 63) & ~(63);
	t->pitch -= 32;

	t->dirty_state = TEX_ALL;

	/* FYI: r300UploadTexImages( rmesa, t ) used to be called here */
}

/* ================================================================
 * Texture combine functions
 */

/* GL_ARB_texture_env_combine support
 */

/* The color tables have combine functions for GL_SRC_COLOR,
 * GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA and GL_ONE_MINUS_SRC_ALPHA.
 */
static GLuint r300_register_color[][R200_MAX_TEXTURE_UNITS] = {
	{
	 R200_TXC_ARG_A_R0_COLOR,
	 R200_TXC_ARG_A_R1_COLOR,
	 R200_TXC_ARG_A_R2_COLOR,
	 R200_TXC_ARG_A_R3_COLOR,
	 R200_TXC_ARG_A_R4_COLOR,
	 R200_TXC_ARG_A_R5_COLOR},
	{
	 R200_TXC_ARG_A_R0_COLOR | R200_TXC_COMP_ARG_A,
	 R200_TXC_ARG_A_R1_COLOR | R200_TXC_COMP_ARG_A,
	 R200_TXC_ARG_A_R2_COLOR | R200_TXC_COMP_ARG_A,
	 R200_TXC_ARG_A_R3_COLOR | R200_TXC_COMP_ARG_A,
	 R200_TXC_ARG_A_R4_COLOR | R200_TXC_COMP_ARG_A,
	 R200_TXC_ARG_A_R5_COLOR | R200_TXC_COMP_ARG_A},
	{
	 R200_TXC_ARG_A_R0_ALPHA,
	 R200_TXC_ARG_A_R1_ALPHA,
	 R200_TXC_ARG_A_R2_ALPHA,
	 R200_TXC_ARG_A_R3_ALPHA,
	 R200_TXC_ARG_A_R4_ALPHA,
	 R200_TXC_ARG_A_R5_ALPHA},
	{
	 R200_TXC_ARG_A_R0_ALPHA | R200_TXC_COMP_ARG_A,
	 R200_TXC_ARG_A_R1_ALPHA | R200_TXC_COMP_ARG_A,
	 R200_TXC_ARG_A_R2_ALPHA | R200_TXC_COMP_ARG_A,
	 R200_TXC_ARG_A_R3_ALPHA | R200_TXC_COMP_ARG_A,
	 R200_TXC_ARG_A_R4_ALPHA | R200_TXC_COMP_ARG_A,
	 R200_TXC_ARG_A_R5_ALPHA | R200_TXC_COMP_ARG_A},
};

static GLuint r300_tfactor_color[] = {
	R200_TXC_ARG_A_TFACTOR_COLOR,
	R200_TXC_ARG_A_TFACTOR_COLOR | R200_TXC_COMP_ARG_A,
	R200_TXC_ARG_A_TFACTOR_ALPHA,
	R200_TXC_ARG_A_TFACTOR_ALPHA | R200_TXC_COMP_ARG_A
};

static GLuint r300_primary_color[] = {
	R200_TXC_ARG_A_DIFFUSE_COLOR,
	R200_TXC_ARG_A_DIFFUSE_COLOR | R200_TXC_COMP_ARG_A,
	R200_TXC_ARG_A_DIFFUSE_ALPHA,
	R200_TXC_ARG_A_DIFFUSE_ALPHA | R200_TXC_COMP_ARG_A
};

/* GL_ZERO table - indices 0-3
 * GL_ONE  table - indices 1-4
 */
static GLuint r300_zero_color[] = {
	R200_TXC_ARG_A_ZERO,
	R200_TXC_ARG_A_ZERO | R200_TXC_COMP_ARG_A,
	R200_TXC_ARG_A_ZERO,
	R200_TXC_ARG_A_ZERO | R200_TXC_COMP_ARG_A,
	R200_TXC_ARG_A_ZERO
};

/* The alpha tables only have GL_SRC_ALPHA and GL_ONE_MINUS_SRC_ALPHA.
 */
static GLuint r300_register_alpha[][R200_MAX_TEXTURE_UNITS] = {
	{
	 R200_TXA_ARG_A_R0_ALPHA,
	 R200_TXA_ARG_A_R1_ALPHA,
	 R200_TXA_ARG_A_R2_ALPHA,
	 R200_TXA_ARG_A_R3_ALPHA,
	 R200_TXA_ARG_A_R4_ALPHA,
	 R200_TXA_ARG_A_R5_ALPHA},
	{
	 R200_TXA_ARG_A_R0_ALPHA | R200_TXA_COMP_ARG_A,
	 R200_TXA_ARG_A_R1_ALPHA | R200_TXA_COMP_ARG_A,
	 R200_TXA_ARG_A_R2_ALPHA | R200_TXA_COMP_ARG_A,
	 R200_TXA_ARG_A_R3_ALPHA | R200_TXA_COMP_ARG_A,
	 R200_TXA_ARG_A_R4_ALPHA | R200_TXA_COMP_ARG_A,
	 R200_TXA_ARG_A_R5_ALPHA | R200_TXA_COMP_ARG_A},
};

static GLuint r300_tfactor_alpha[] = {
	R200_TXA_ARG_A_TFACTOR_ALPHA,
	R200_TXA_ARG_A_TFACTOR_ALPHA | R200_TXA_COMP_ARG_A
};

static GLuint r300_primary_alpha[] = {
	R200_TXA_ARG_A_DIFFUSE_ALPHA,
	R200_TXA_ARG_A_DIFFUSE_ALPHA | R200_TXA_COMP_ARG_A
};

/* GL_ZERO table - indices 0-1
 * GL_ONE  table - indices 1-2
 */
static GLuint r300_zero_alpha[] = {
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

static GLboolean r300UpdateTextureEnv(GLcontext * ctx, int unit)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
	GLuint color_combine, alpha_combine;

#if 0 /* disable for now.. */
	GLuint color_scale = rmesa->hw.pix[unit].cmd[PIX_PP_TXCBLEND2] &
	    ~(R200_TXC_SCALE_MASK);
	GLuint alpha_scale = rmesa->hw.pix[unit].cmd[PIX_PP_TXABLEND2] &
	    ~(R200_TXA_DOT_ALPHA | R200_TXA_SCALE_MASK);
#endif

	GLuint color_scale=0, alpha_scale=0;

	/* texUnit->_Current can be NULL if and only if the texture unit is
	 * not actually enabled.
	 */
	assert((texUnit->_ReallyEnabled == 0)
	       || (texUnit->_Current != NULL));

	if (RADEON_DEBUG & DEBUG_TEXTURE) {
		fprintf(stderr, "%s( %p, %d )\n", __FUNCTION__, (void *)ctx,
			unit);
	}

	/* Set the texture environment state.  Isn't this nice and clean?
	 * The chip will automagically set the texture alpha to 0xff when
	 * the texture format does not include an alpha component.  This
	 * reduces the amount of special-casing we have to do, alpha-only
	 * textures being a notable exception.
	 */
	/* Don't cache these results.
	 */
#if 0
	rmesa->state.texture.unit[unit].format = 0;
#endif
	rmesa->state.texture.unit[unit].envMode = 0;


	if (!texUnit->_ReallyEnabled) {
		if (unit == 0) {
			color_combine =
			    R200_TXC_ARG_A_ZERO | R200_TXC_ARG_B_ZERO |
			    R200_TXC_ARG_C_DIFFUSE_COLOR | R200_TXC_OP_MADD;
			alpha_combine =
			    R200_TXA_ARG_A_ZERO | R200_TXA_ARG_B_ZERO |
			    R200_TXA_ARG_C_DIFFUSE_ALPHA | R200_TXA_OP_MADD;
		} else {
			color_combine =
			    R200_TXC_ARG_A_ZERO | R200_TXC_ARG_B_ZERO |
			    R200_TXC_ARG_C_R0_COLOR | R200_TXC_OP_MADD;
			alpha_combine =
			    R200_TXA_ARG_A_ZERO | R200_TXA_ARG_B_ZERO |
			    R200_TXA_ARG_C_R0_ALPHA | R200_TXA_OP_MADD;
		}
	} else {
		GLuint color_arg[3], alpha_arg[3];
		GLuint i;
		const GLuint numColorArgs =
		    texUnit->_CurrentCombine->_NumArgsRGB;
		const GLuint numAlphaArgs = texUnit->_CurrentCombine->_NumArgsA;
		GLuint RGBshift = texUnit->_CurrentCombine->ScaleShiftRGB;
		GLuint Ashift = texUnit->_CurrentCombine->ScaleShiftA;

		/* Step 1:
		 * Extract the color and alpha combine function arguments.
		 */
		for (i = 0; i < numColorArgs; i++) {
			const GLint op =
			    texUnit->_CurrentCombine->OperandRGB[i] -
			    GL_SRC_COLOR;
			assert(op >= 0);
			assert(op <= 3);
			switch (texUnit->_CurrentCombine->SourceRGB[i]) {
			case GL_TEXTURE:
				color_arg[i] = r300_register_color[op][unit];
				break;
			case GL_CONSTANT:
				color_arg[i] = r300_tfactor_color[op];
				break;
			case GL_PRIMARY_COLOR:
				color_arg[i] = r300_primary_color[op];
				break;
			case GL_PREVIOUS:
				if (unit == 0)
					color_arg[i] = r300_primary_color[op];
				else
					color_arg[i] =
					    r300_register_color[op][0];
				break;
			case GL_ZERO:
				color_arg[i] = r300_zero_color[op];
				break;
			case GL_ONE:
				color_arg[i] = r300_zero_color[op + 1];
				break;
			default:
				return GL_FALSE;
			}
		}

		for (i = 0; i < numAlphaArgs; i++) {
			const GLint op =
			    texUnit->_CurrentCombine->OperandA[i] -
			    GL_SRC_ALPHA;
			assert(op >= 0);
			assert(op <= 1);
			switch (texUnit->_CurrentCombine->SourceA[i]) {
			case GL_TEXTURE:
				alpha_arg[i] = r300_register_alpha[op][unit];
				break;
			case GL_CONSTANT:
				alpha_arg[i] = r300_tfactor_alpha[op];
				break;
			case GL_PRIMARY_COLOR:
				alpha_arg[i] = r300_primary_alpha[op];
				break;
			case GL_PREVIOUS:
				if (unit == 0)
					alpha_arg[i] = r300_primary_alpha[op];
				else
					alpha_arg[i] =
					    r300_register_alpha[op][0];
				break;
			case GL_ZERO:
				alpha_arg[i] = r300_zero_alpha[op];
				break;
			case GL_ONE:
				alpha_arg[i] = r300_zero_alpha[op + 1];
				break;
			default:
				return GL_FALSE;
			}
		}

		/* Step 2:
		 * Build up the color and alpha combine functions.
		 */
		switch (texUnit->_CurrentCombine->ModeRGB) {
		case GL_REPLACE:
			color_combine = (R200_TXC_ARG_A_ZERO |
					 R200_TXC_ARG_B_ZERO |
					 R200_TXC_OP_MADD);
			R200_COLOR_ARG(0, C);
			break;
		case GL_MODULATE:
			color_combine = (R200_TXC_ARG_C_ZERO |
					 R200_TXC_OP_MADD);
			R200_COLOR_ARG(0, A);
			R200_COLOR_ARG(1, B);
			break;
		case GL_ADD:
			color_combine = (R200_TXC_ARG_B_ZERO |
					 R200_TXC_COMP_ARG_B |
					 R200_TXC_OP_MADD);
			R200_COLOR_ARG(0, A);
			R200_COLOR_ARG(1, C);
			break;
		case GL_ADD_SIGNED:
			color_combine = (R200_TXC_ARG_B_ZERO | R200_TXC_COMP_ARG_B | R200_TXC_BIAS_ARG_C |	/* new */
					 R200_TXC_OP_MADD);	/* was ADDSIGNED */
			R200_COLOR_ARG(0, A);
			R200_COLOR_ARG(1, C);
			break;
		case GL_SUBTRACT:
			color_combine = (R200_TXC_ARG_B_ZERO |
					 R200_TXC_COMP_ARG_B |
					 R200_TXC_NEG_ARG_C | R200_TXC_OP_MADD);
			R200_COLOR_ARG(0, A);
			R200_COLOR_ARG(1, C);
			break;
		case GL_INTERPOLATE:
			color_combine = (R200_TXC_OP_LERP);
			R200_COLOR_ARG(0, B);
			R200_COLOR_ARG(1, A);
			R200_COLOR_ARG(2, C);
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
			R200_COLOR_ARG(0, A);
			R200_COLOR_ARG(1, B);
			break;

		case GL_MODULATE_ADD_ATI:
			color_combine = (R200_TXC_OP_MADD);
			R200_COLOR_ARG(0, A);
			R200_COLOR_ARG(1, C);
			R200_COLOR_ARG(2, B);
			break;
		case GL_MODULATE_SIGNED_ADD_ATI:
			color_combine = (R200_TXC_BIAS_ARG_C |	/* new */
					 R200_TXC_OP_MADD);	/* was ADDSIGNED */
			R200_COLOR_ARG(0, A);
			R200_COLOR_ARG(1, C);
			R200_COLOR_ARG(2, B);
			break;
		case GL_MODULATE_SUBTRACT_ATI:
			color_combine = (R200_TXC_NEG_ARG_C | R200_TXC_OP_MADD);
			R200_COLOR_ARG(0, A);
			R200_COLOR_ARG(1, C);
			R200_COLOR_ARG(2, B);
			break;
		default:
			return GL_FALSE;
		}

		switch (texUnit->_CurrentCombine->ModeA) {
		case GL_REPLACE:
			alpha_combine = (R200_TXA_ARG_A_ZERO |
					 R200_TXA_ARG_B_ZERO |
					 R200_TXA_OP_MADD);
			R200_ALPHA_ARG(0, C);
			break;
		case GL_MODULATE:
			alpha_combine = (R200_TXA_ARG_C_ZERO |
					 R200_TXA_OP_MADD);
			R200_ALPHA_ARG(0, A);
			R200_ALPHA_ARG(1, B);
			break;
		case GL_ADD:
			alpha_combine = (R200_TXA_ARG_B_ZERO |
					 R200_TXA_COMP_ARG_B |
					 R200_TXA_OP_MADD);
			R200_ALPHA_ARG(0, A);
			R200_ALPHA_ARG(1, C);
			break;
		case GL_ADD_SIGNED:
			alpha_combine = (R200_TXA_ARG_B_ZERO | R200_TXA_COMP_ARG_B | R200_TXA_BIAS_ARG_C |	/* new */
					 R200_TXA_OP_MADD);	/* was ADDSIGNED */
			R200_ALPHA_ARG(0, A);
			R200_ALPHA_ARG(1, C);
			break;
		case GL_SUBTRACT:
			alpha_combine = (R200_TXA_ARG_B_ZERO |
					 R200_TXA_COMP_ARG_B |
					 R200_TXA_NEG_ARG_C | R200_TXA_OP_MADD);
			R200_ALPHA_ARG(0, A);
			R200_ALPHA_ARG(1, C);
			break;
		case GL_INTERPOLATE:
			alpha_combine = (R200_TXA_OP_LERP);
			R200_ALPHA_ARG(0, B);
			R200_ALPHA_ARG(1, A);
			R200_ALPHA_ARG(2, C);
			break;

		case GL_MODULATE_ADD_ATI:
			alpha_combine = (R200_TXA_OP_MADD);
			R200_ALPHA_ARG(0, A);
			R200_ALPHA_ARG(1, C);
			R200_ALPHA_ARG(2, B);
			break;
		case GL_MODULATE_SIGNED_ADD_ATI:
			alpha_combine = (R200_TXA_BIAS_ARG_C |	/* new */
					 R200_TXA_OP_MADD);	/* was ADDSIGNED */
			R200_ALPHA_ARG(0, A);
			R200_ALPHA_ARG(1, C);
			R200_ALPHA_ARG(2, B);
			break;
		case GL_MODULATE_SUBTRACT_ATI:
			alpha_combine = (R200_TXA_NEG_ARG_C | R200_TXA_OP_MADD);
			R200_ALPHA_ARG(0, A);
			R200_ALPHA_ARG(1, C);
			R200_ALPHA_ARG(2, B);
			break;
		default:
			return GL_FALSE;
		}

		if ((texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA_EXT)
		    || (texUnit->_CurrentCombine->ModeRGB == GL_DOT3_RGBA)) {
			alpha_scale |= R200_TXA_DOT_ALPHA;
			Ashift = RGBshift;
		}

		/* Step 3:
		 * Apply the scale factor.
		 */
		color_scale |= (RGBshift << R200_TXC_SCALE_SHIFT);
		alpha_scale |= (Ashift << R200_TXA_SCALE_SHIFT);

		/* All done!
		 */
	}

#if 0
	fprintf(stderr, "color_combine=%08x alpha_combine=%08x color_scale=%08x alpha_scale=%08x\n",
		color_combine, alpha_combine, color_scale, alpha_scale);
#endif

#if 0
	if (rmesa->hw.pix[unit].cmd[PIX_PP_TXCBLEND] != color_combine ||
	    rmesa->hw.pix[unit].cmd[PIX_PP_TXABLEND] != alpha_combine ||
	    rmesa->hw.pix[unit].cmd[PIX_PP_TXCBLEND2] != color_scale ||
	    rmesa->hw.pix[unit].cmd[PIX_PP_TXABLEND2] != alpha_scale) {
		R300_STATECHANGE(rmesa, pix[unit]);
		rmesa->hw.pix[unit].cmd[PIX_PP_TXCBLEND] = color_combine;
		rmesa->hw.pix[unit].cmd[PIX_PP_TXABLEND] = alpha_combine;
		rmesa->hw.pix[unit].cmd[PIX_PP_TXCBLEND2] = color_scale;
		rmesa->hw.pix[unit].cmd[PIX_PP_TXABLEND2] = alpha_scale;
	}

#endif

	return GL_TRUE;
}

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

static void import_tex_obj_state(r300ContextPtr rmesa,
				 int unit, r300TexObjPtr texobj)
{
#if 0 /* needs fixing.. or should be done elsewhere */
	GLuint *cmd = R300_DB_STATE(tex[unit]);

	cmd[TEX_PP_TXFILTER] &= ~TEXOBJ_TXFILTER_MASK;
	cmd[TEX_PP_TXFILTER] |= texobj->filter & TEXOBJ_TXFILTER_MASK;
	cmd[TEX_PP_TXFORMAT] &= ~TEXOBJ_TXFORMAT_MASK;
	cmd[TEX_PP_TXFORMAT] |= texobj->format & TEXOBJ_TXFORMAT_MASK;
	cmd[TEX_PP_TXFORMAT_X] &= ~TEXOBJ_TXFORMAT_X_MASK;
	cmd[TEX_PP_TXFORMAT_X] |=
	    texobj->format_x & TEXOBJ_TXFORMAT_X_MASK;
	cmd[TEX_PP_TXSIZE] = texobj->size;	/* NPOT only! */
	cmd[TEX_PP_TXPITCH] = texobj->pitch;	/* NPOT only! */
	cmd[TEX_PP_TXOFFSET] = texobj->pp_txoffset;
	cmd[TEX_PP_BORDER_COLOR] = texobj->pp_border_color;
	R200_DB_STATECHANGE(rmesa, &rmesa->hw.tex[unit]);

	if (texobj->base.tObj->Target == GL_TEXTURE_CUBE_MAP) {
		GLuint *cube_cmd = R200_DB_STATE(cube[unit]);
		GLuint bytesPerFace = texobj->base.totalSize / 6;
		ASSERT(texobj->totalSize % 6 == 0);
		cube_cmd[CUBE_PP_CUBIC_FACES] = texobj->pp_cubic_faces;
		cube_cmd[CUBE_PP_CUBIC_OFFSET_F1] =
		    texobj->pp_txoffset + 1 * bytesPerFace;
		cube_cmd[CUBE_PP_CUBIC_OFFSET_F2] =
		    texobj->pp_txoffset + 2 * bytesPerFace;
		cube_cmd[CUBE_PP_CUBIC_OFFSET_F3] =
		    texobj->pp_txoffset + 3 * bytesPerFace;
		cube_cmd[CUBE_PP_CUBIC_OFFSET_F4] =
		    texobj->pp_txoffset + 4 * bytesPerFace;
		cube_cmd[CUBE_PP_CUBIC_OFFSET_F5] =
		    texobj->pp_txoffset + 5 * bytesPerFace;
		R200_DB_STATECHANGE(rmesa, &rmesa->hw.cube[unit]);
	}

	texobj->dirty_state &= ~(1 << unit);
#endif
}

static void set_texgen_matrix(r300ContextPtr rmesa,
			      GLuint unit,
			      const GLfloat * s_plane,
			      const GLfloat * t_plane, const GLfloat * r_plane)
{
	static const GLfloat scale_identity[4] = { 1, 1, 1, 1 };

	if (!TEST_EQ_4V(s_plane, scale_identity) ||
	    !TEST_EQ_4V(t_plane, scale_identity) ||
	    !TEST_EQ_4V(r_plane, scale_identity)) {
		rmesa->TexGenEnabled |= R200_TEXMAT_0_ENABLE << unit;
		rmesa->TexGenMatrix[unit].m[0] = s_plane[0];
		rmesa->TexGenMatrix[unit].m[4] = s_plane[1];
		rmesa->TexGenMatrix[unit].m[8] = s_plane[2];
		rmesa->TexGenMatrix[unit].m[12] = s_plane[3];

		rmesa->TexGenMatrix[unit].m[1] = t_plane[0];
		rmesa->TexGenMatrix[unit].m[5] = t_plane[1];
		rmesa->TexGenMatrix[unit].m[9] = t_plane[2];
		rmesa->TexGenMatrix[unit].m[13] = t_plane[3];

		/* NOTE: r_plane goes in the 4th row, not 3rd! */
		rmesa->TexGenMatrix[unit].m[3] = r_plane[0];
		rmesa->TexGenMatrix[unit].m[7] = r_plane[1];
		rmesa->TexGenMatrix[unit].m[11] = r_plane[2];
		rmesa->TexGenMatrix[unit].m[15] = r_plane[3];

		//rmesa->NewGLState |= _NEW_TEXTURE_MATRIX;
	}
}

/* Need this special matrix to get correct reflection map coords */
static void set_texgen_reflection_matrix(r300ContextPtr rmesa, GLuint unit)
{
	static const GLfloat m[16] = {
		-1, 0, 0, 0,
		0, -1, 0, 0,
		0, 0, 0, -1,
		0, 0, -1, 0
	};
	_math_matrix_loadf(&(rmesa->TexGenMatrix[unit]), m);
	_math_matrix_analyse(&(rmesa->TexGenMatrix[unit]));
	rmesa->TexGenEnabled |= R200_TEXMAT_0_ENABLE << unit;
}

/* Need this special matrix to get correct normal map coords */
static void set_texgen_normal_map_matrix(r300ContextPtr rmesa, GLuint unit)
{
	static const GLfloat m[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1,
		0, 0, 1, 0
	};
	_math_matrix_loadf(&(rmesa->TexGenMatrix[unit]), m);
	_math_matrix_analyse(&(rmesa->TexGenMatrix[unit]));
	rmesa->TexGenEnabled |= R200_TEXMAT_0_ENABLE << unit;
}

/* Ignoring the Q texcoord for now.
 *
 * Returns GL_FALSE if fallback required.
 */
static GLboolean r300_validate_texgen(GLcontext * ctx, GLuint unit)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
	GLuint inputshift = R200_TEXGEN_0_INPUT_SHIFT + unit * 4;
	GLuint tmp = rmesa->TexGenEnabled;

	rmesa->TexGenCompSel &= ~(R200_OUTPUT_TEX_0 << unit);
	rmesa->TexGenEnabled &= ~(R200_TEXGEN_TEXMAT_0_ENABLE << unit);
	rmesa->TexGenEnabled &= ~(R200_TEXMAT_0_ENABLE << unit);
	rmesa->TexGenInputs &= ~(R200_TEXGEN_INPUT_MASK << inputshift);
	rmesa->TexGenNeedNormals[unit] = 0;

	if (0)
		fprintf(stderr, "%s unit %d\n", __FUNCTION__, unit);

	if ((texUnit->TexGenEnabled & (S_BIT | T_BIT | R_BIT)) == 0) {
		/* Disabled, no fallback:
		 */
		rmesa->TexGenInputs |=
		    (R200_TEXGEN_INPUT_TEXCOORD_0 + unit) << inputshift;
		return GL_TRUE;
	} else if (texUnit->TexGenEnabled & Q_BIT) {
		/* Very easy to do this, in fact would remove a fallback case
		 * elsewhere, but I haven't done it yet...  Fallback:
		 */
		/*fprintf(stderr, "fallback Q_BIT\n"); */
		return GL_FALSE;
	} else if (texUnit->TexGenEnabled == (S_BIT | T_BIT) &&
		   texUnit->GenModeS == texUnit->GenModeT) {
		/* OK */
		rmesa->TexGenEnabled |= R200_TEXGEN_TEXMAT_0_ENABLE << unit;
		/* continue */
	} else if (texUnit->TexGenEnabled == (S_BIT | T_BIT | R_BIT) &&
		   texUnit->GenModeS == texUnit->GenModeT &&
		   texUnit->GenModeT == texUnit->GenModeR) {
		/* OK */
		rmesa->TexGenEnabled |= R200_TEXGEN_TEXMAT_0_ENABLE << unit;
		/* continue */
	} else {
		/* Mixed modes, fallback:
		 */
		/* fprintf(stderr, "fallback mixed texgen\n"); */
		return GL_FALSE;
	}

	rmesa->TexGenEnabled |= R200_TEXGEN_TEXMAT_0_ENABLE << unit;

	switch (texUnit->GenModeS) {
	case GL_OBJECT_LINEAR:
		rmesa->TexGenInputs |= R200_TEXGEN_INPUT_OBJ << inputshift;
		set_texgen_matrix(rmesa, unit,
				  texUnit->ObjectPlaneS,
				  texUnit->ObjectPlaneT, texUnit->ObjectPlaneR);
		break;

	case GL_EYE_LINEAR:
		rmesa->TexGenInputs |= R200_TEXGEN_INPUT_EYE << inputshift;
		set_texgen_matrix(rmesa, unit,
				  texUnit->EyePlaneS,
				  texUnit->EyePlaneT, texUnit->EyePlaneR);
		break;

	case GL_REFLECTION_MAP_NV:
		rmesa->TexGenNeedNormals[unit] = GL_TRUE;
		rmesa->TexGenInputs |=
		    R200_TEXGEN_INPUT_EYE_REFLECT << inputshift;
		set_texgen_reflection_matrix(rmesa, unit);
		break;

	case GL_NORMAL_MAP_NV:
		rmesa->TexGenNeedNormals[unit] = GL_TRUE;
		rmesa->TexGenInputs |=
		    R200_TEXGEN_INPUT_EYE_NORMAL << inputshift;
		set_texgen_normal_map_matrix(rmesa, unit);
		break;

	case GL_SPHERE_MAP:
		rmesa->TexGenNeedNormals[unit] = GL_TRUE;
		rmesa->TexGenInputs |= R200_TEXGEN_INPUT_SPHERE << inputshift;
		break;

	default:
		/* Unsupported mode, fallback:
		 */
		/*  fprintf(stderr, "fallback unsupported texgen\n"); */
		return GL_FALSE;
	}

	rmesa->TexGenCompSel |= R200_OUTPUT_TEX_0 << unit;

	if (tmp != rmesa->TexGenEnabled) {
		//rmesa->NewGLState |= _NEW_TEXTURE_MATRIX;
	}

	return GL_TRUE;
}

static void disable_tex(GLcontext * ctx, int unit)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

#if 0 /* This needs to be redone.. or done elsewhere */
	if (rmesa->hw.ctx.cmd[CTX_PP_CNTL] & (R200_TEX_0_ENABLE << unit)) {
		/* Texture unit disabled */
		if (rmesa->state.texture.unit[unit].texobj != NULL) {
			/* The old texture is no longer bound to this texture unit.
			 * Mark it as such.
			 */

			rmesa->state.texture.unit[unit].texobj->base.bound &=
			    ~(1UL << unit);
			rmesa->state.texture.unit[unit].texobj = NULL;
		}

		R300_STATECHANGE(rmesa, ctx);
		rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~((R200_TEX_0_ENABLE |
						     R200_TEX_BLEND_0_ENABLE) <<
						    unit);
		rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= R200_TEX_BLEND_0_ENABLE;

		R300_STATECHANGE(rmesa, tcl);
		rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_1] &=
		    ~(7 << (unit * 3));

		if (rmesa->radeon.TclFallback & (RADEON_TCL_FALLBACK_TEXGEN_0 << unit)) {
			TCL_FALLBACK(ctx, (RADEON_TCL_FALLBACK_TEXGEN_0 << unit),
				     GL_FALSE);
		}

		/* Actually want to keep all units less than max active texture
		 * enabled, right?  Fix this for >2 texunits.
		 */
		/* FIXME: What should happen here if r300UpdateTextureEnv fails? */
		if (unit == 0)
			r300UpdateTextureEnv(ctx, unit);

		{
			GLuint inputshift =
			    R200_TEXGEN_0_INPUT_SHIFT + unit * 4;
			GLuint tmp = rmesa->TexGenEnabled;

			rmesa->TexGenEnabled &=
			    ~(R200_TEXGEN_TEXMAT_0_ENABLE << unit);
			rmesa->TexGenEnabled &= ~(R200_TEXMAT_0_ENABLE << unit);
			rmesa->TexGenEnabled &=
			    ~(R200_TEXGEN_INPUT_MASK << inputshift);
			rmesa->TexGenNeedNormals[unit] = 0;
			rmesa->TexGenCompSel &= ~(R200_OUTPUT_TEX_0 << unit);
			rmesa->TexGenInputs &=
			    ~(R200_TEXGEN_INPUT_MASK << inputshift);

			if (tmp != rmesa->TexGenEnabled) {
				rmesa->recheck_texgen[unit] = GL_TRUE;
				rmesa->NewGLState |= _NEW_TEXTURE_MATRIX;
			}
		}
	}
#endif
}

static GLboolean enable_tex_2d(GLcontext * ctx, int unit)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
	struct gl_texture_object *tObj = texUnit->_Current;
	r300TexObjPtr t = (r300TexObjPtr) tObj->DriverData;

	/* Need to load the 2d images associated with this unit.
	 */
#if 0
	if (t->format & R200_TXFORMAT_NON_POWER2) {
		t->format &= ~R200_TXFORMAT_NON_POWER2;
		t->base.dirty_images[0] = ~0;
	}
#endif

	ASSERT(tObj->Target == GL_TEXTURE_2D || tObj->Target == GL_TEXTURE_1D);

	if (t->base.dirty_images[0]) {
		R300_FIREVERTICES(rmesa);
		r300SetTexImages(rmesa, tObj);
		r300UploadTexImages(rmesa, (r300TexObjPtr) tObj->DriverData, 0);
		if (!t->base.memBlock)
			return GL_FALSE;
	}

	return GL_TRUE;
}

#if ENABLE_HW_3D_TEXTURE
static GLboolean enable_tex_3d(GLcontext * ctx, int unit)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
	struct gl_texture_object *tObj = texUnit->_Current;
	r300TexObjPtr t = (r300TexObjPtr) tObj->DriverData;

	/* Need to load the 3d images associated with this unit.
	 */
	if (t->format & R200_TXFORMAT_NON_POWER2) {
		t->format &= ~R200_TXFORMAT_NON_POWER2;
		t->base.dirty_images[0] = ~0;
	}

	ASSERT(tObj->Target == GL_TEXTURE_3D);

	/* R100 & R200 do not support mipmaps for 3D textures.
	 */
	if ((tObj->MinFilter != GL_NEAREST) && (tObj->MinFilter != GL_LINEAR)) {
		return GL_FALSE;
	}

	if (t->base.dirty_images[0]) {
		R300_FIREVERTICES(rmesa);
		r300SetTexImages(rmesa, tObj);
		r300UploadTexImages(rmesa, (r300TexObjPtr) tObj->DriverData, 0);
		if (!t->base.memBlock)
			return GL_FALSE;
	}

	return GL_TRUE;
}
#endif

static GLboolean enable_tex_cube(GLcontext * ctx, int unit)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
	struct gl_texture_object *tObj = texUnit->_Current;
	r300TexObjPtr t = (r300TexObjPtr) tObj->DriverData;
	GLuint face;

	/* Need to load the 2d images associated with this unit.
	 */
	if (t->format & R200_TXFORMAT_NON_POWER2) {
		t->format &= ~R200_TXFORMAT_NON_POWER2;
		for (face = 0; face < 6; face++)
			t->base.dirty_images[face] = ~0;
	}

	ASSERT(tObj->Target == GL_TEXTURE_CUBE_MAP);

	if (t->base.dirty_images[0] || t->base.dirty_images[1] ||
	    t->base.dirty_images[2] || t->base.dirty_images[3] ||
	    t->base.dirty_images[4] || t->base.dirty_images[5]) {
		/* flush */
		R300_FIREVERTICES(rmesa);
		/* layout memory space, once for all faces */
		r300SetTexImages(rmesa, tObj);
	}

	/* upload (per face) */
	for (face = 0; face < 6; face++) {
		if (t->base.dirty_images[face]) {
			r300UploadTexImages(rmesa,
					    (r300TexObjPtr) tObj->DriverData,
					    face);
		}
	}

	if (!t->base.memBlock) {
		/* texmem alloc failed, use s/w fallback */
		return GL_FALSE;
	}

	return GL_TRUE;
}

static GLboolean enable_tex_rect(GLcontext * ctx, int unit)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
	struct gl_texture_object *tObj = texUnit->_Current;
	r300TexObjPtr t = (r300TexObjPtr) tObj->DriverData;

#if 0
	if (!(t->format & R200_TXFORMAT_NON_POWER2)) {
		t->format |= R200_TXFORMAT_NON_POWER2;
		t->base.dirty_images[0] = ~0;
	}
#endif

	ASSERT(tObj->Target == GL_TEXTURE_RECTANGLE_NV);

	if (t->base.dirty_images[0]) {
		R300_FIREVERTICES(rmesa);
		r300SetTexImages(rmesa, tObj);
		r300UploadTexImages(rmesa, (r300TexObjPtr) tObj->DriverData, 0);
		if (!t->base.memBlock && !rmesa->prefer_gart_client_texturing)
			return GL_FALSE;
	}

	return GL_TRUE;
}

static GLboolean update_tex_common(GLcontext * ctx, int unit)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
	struct gl_texture_object *tObj = texUnit->_Current;
	r300TexObjPtr t = (r300TexObjPtr) tObj->DriverData;
	GLenum format;

	/* Fallback if there's a texture border */
	if (tObj->Image[0][tObj->BaseLevel]->Border > 0)
		return GL_FALSE;

	/* Update state if this is a different texture object to last
	 * time.
	 */
	if (rmesa->state.texture.unit[unit].texobj != t) {
		if (rmesa->state.texture.unit[unit].texobj != NULL) {
			/* The old texture is no longer bound to this texture unit.
			 * Mark it as such.
			 */

			rmesa->state.texture.unit[unit].texobj->base.bound &=
			    ~(1UL << unit);
		}

		rmesa->state.texture.unit[unit].texobj = t;
		t->base.bound |= (1UL << unit);
		t->dirty_state |= 1 << unit;
		driUpdateTextureLRU((driTextureObject *) t);	/* XXX: should be locked! */
	}

#if 0 /* do elsewhere ? */
	/* Newly enabled?
	 */
	if (1
	    || !(rmesa->hw.ctx.
		 cmd[CTX_PP_CNTL] & (R200_TEX_0_ENABLE << unit))) {
		R300_STATECHANGE(rmesa, ctx);
		rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= (R200_TEX_0_ENABLE |
						   R200_TEX_BLEND_0_ENABLE) <<
		    unit;

		R300_STATECHANGE(rmesa, vtx);
		rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_1] |= 4 << (unit * 3);

		rmesa->recheck_texgen[unit] = GL_TRUE;
	}

	if (t->dirty_state & (1 << unit)) {
		import_tex_obj_state(rmesa, unit, t);
	}

	if (rmesa->recheck_texgen[unit]) {
		GLboolean fallback = !r300_validate_texgen(ctx, unit);
		TCL_FALLBACK(ctx, (RADEON_TCL_FALLBACK_TEXGEN_0 << unit),
			     fallback);
		rmesa->recheck_texgen[unit] = 0;
		rmesa->NewGLState |= _NEW_TEXTURE_MATRIX;
	}
#endif

	format = tObj->Image[0][tObj->BaseLevel]->Format;
	if (rmesa->state.texture.unit[unit].format != format ||
	    rmesa->state.texture.unit[unit].envMode != texUnit->EnvMode) {
		//rmesa->state.texture.unit[unit].format = format;
		rmesa->state.texture.unit[unit].envMode = texUnit->EnvMode;
		if (!r300UpdateTextureEnv(ctx, unit)) {
			return GL_FALSE;
		}
	}

#if R200_MERGED
	FALLBACK(&rmesa->radeon, RADEON_FALLBACK_BORDER_MODE, t->border_fallback);
#endif
		
	return !t->border_fallback;
}

static GLboolean r300UpdateTextureUnit(GLcontext * ctx, int unit)
{
	struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

	if (texUnit->_ReallyEnabled & (TEXTURE_RECT_BIT)) {
		return (enable_tex_rect(ctx, unit) &&
			update_tex_common(ctx, unit));
	} else if (texUnit->_ReallyEnabled & (TEXTURE_1D_BIT | TEXTURE_2D_BIT)) {
		return (enable_tex_2d(ctx, unit) &&
			update_tex_common(ctx, unit));
	}
#if ENABLE_HW_3D_TEXTURE
	else if (texUnit->_ReallyEnabled & (TEXTURE_3D_BIT)) {
		return (enable_tex_3d(ctx, unit) &&
			update_tex_common(ctx, unit));
	}
#endif
	else if (texUnit->_ReallyEnabled & (TEXTURE_CUBE_BIT)) {
		return (enable_tex_cube(ctx, unit) &&
			update_tex_common(ctx, unit));
	} else if (texUnit->_ReallyEnabled) {
		return GL_FALSE;
	} else {
		disable_tex(ctx, unit);
		return GL_TRUE;
	}
}

void r300UpdateTextureState(GLcontext * ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	GLboolean ok;
	GLuint dbg;
	int i;

	ok = (r300UpdateTextureUnit(ctx, 0) &&
	      r300UpdateTextureUnit(ctx, 1) &&
	      r300UpdateTextureUnit(ctx, 2) &&
	      r300UpdateTextureUnit(ctx, 3) &&
	      r300UpdateTextureUnit(ctx, 4) &&
	      r300UpdateTextureUnit(ctx, 5) &&
	      r300UpdateTextureUnit(ctx, 6) &&
	      r300UpdateTextureUnit(ctx, 7)
	      );

#if R200_MERGED
	FALLBACK(&rmesa->radeon, RADEON_FALLBACK_TEXTURE, !ok);
#endif	

	/* This needs correction, or just be done elsewhere
	if (rmesa->radeon.TclFallback)
		r300ChooseVertexState(ctx);
	*/

#if 0 /* Workaround - disable.. */
	if (GET_CHIP(rmesa->radeon.radeonScreen) == RADEON_CHIP_REAL_R200) {
		/*
		 * T0 hang workaround -------------
		 * not needed for r200 derivatives?
		 */
		if ((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & R200_TEX_ENABLE_MASK) ==
		    R200_TEX_0_ENABLE
		    && (rmesa->hw.tex[0].
			cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK) >
		    R200_MIN_FILTER_LINEAR) {

			R300_STATECHANGE(rmesa, ctx);
			R300_STATECHANGE(rmesa, tex[1]);
			rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= R200_TEX_1_ENABLE;
			rmesa->hw.tex[1].cmd[TEX_PP_TXFORMAT] &=
			    ~TEXOBJ_TXFORMAT_MASK;
			rmesa->hw.tex[1].cmd[TEX_PP_TXFORMAT] |= 0x08000000;
		} else {
			if ((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & R200_TEX_1_ENABLE)
			    && (rmesa->hw.tex[1].
				cmd[TEX_PP_TXFORMAT] & 0x08000000)) {
				R300_STATECHANGE(rmesa, tex[1]);
				rmesa->hw.tex[1].cmd[TEX_PP_TXFORMAT] &=
				    ~0x08000000;
			}
		}

		/* maybe needs to be done pairwise due to 2 parallel (physical) tex units ?
		   looks like that's not the case, if 8500/9100 owners don't complain remove this...
		   for ( i = 0; i < ctx->Const.MaxTextureUnits; i += 2) {
		   if (((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & ((R200_TEX_0_ENABLE |
		   R200_TEX_1_ENABLE ) << i)) == (R200_TEX_0_ENABLE << i)) &&
		   ((rmesa->hw.tex[i].cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK) >
		   R200_MIN_FILTER_LINEAR)) {
		   R300_STATECHANGE(rmesa, ctx);
		   R300_STATECHANGE(rmesa, tex[i+1]);
		   rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= (R200_TEX_1_ENABLE << i);
		   rmesa->hw.tex[i+1].cmd[TEX_PP_TXFORMAT] &= ~TEXOBJ_TXFORMAT_MASK;
		   rmesa->hw.tex[i+1].cmd[TEX_PP_TXFORMAT] |= 0x08000000;
		   }
		   else {
		   if ((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & (R200_TEX_1_ENABLE << i)) &&
		   (rmesa->hw.tex[i+1].cmd[TEX_PP_TXFORMAT] & 0x08000000)) {
		   R300_STATECHANGE(rmesa, tex[i+1]);
		   rmesa->hw.tex[i+1].cmd[TEX_PP_TXFORMAT] &= ~0x08000000;
		   }
		   }
		   } */

		/*
		 * Texture cache LRU hang workaround -------------
		 * not needed for r200 derivatives?
		 */
		dbg = 0x0;

		if (((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & (R200_TEX_0_ENABLE)) &&
		     ((((rmesa->hw.tex[0].
			 cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK)) & 0x04)
		      == 0))
		    || ((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & R200_TEX_2_ENABLE)
			&&
			((((rmesa->hw.tex[2].
			    cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK)) &
			  0x04) == 0))
		    || ((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & R200_TEX_4_ENABLE)
			&&
			((((rmesa->hw.tex[4].
			    cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK)) &
			  0x04) == 0))) {
			dbg |= 0x02;
		}

		if (((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & (R200_TEX_1_ENABLE)) &&
		     ((((rmesa->hw.tex[1].
			 cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK)) & 0x04)
		      == 0))
		    || ((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & R200_TEX_3_ENABLE)
			&&
			((((rmesa->hw.tex[3].
			    cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK)) &
			  0x04) == 0))
		    || ((rmesa->hw.ctx.cmd[CTX_PP_CNTL] & R200_TEX_5_ENABLE)
			&&
			((((rmesa->hw.tex[5].
			    cmd[TEX_PP_TXFILTER] & R200_MIN_FILTER_MASK)) &
			  0x04) == 0))) {
			dbg |= 0x04;
		}

		if (dbg != rmesa->hw.tam.cmd[TAM_DEBUG3]) {
			R300_STATECHANGE(rmesa, tam);
			rmesa->hw.tam.cmd[TAM_DEBUG3] = dbg;
			if (0)
				printf("TEXCACHE LRU HANG WORKAROUND %x\n",
				       dbg);
		}
	}
#endif
}
