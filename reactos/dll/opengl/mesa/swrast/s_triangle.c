/*
 * Mesa 3-D graphics library
 * Version:  7.3
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * When the device driver doesn't implement triangle rasterization it
 * can hook in _swrast_Triangle, which eventually calls one of these
 * functions to draw triangles.
 */

#include <precomp.h>

/**
 * Test if a triangle should be culled.  Used for feedback and selection mode.
 * \return GL_TRUE if the triangle is to be culled, GL_FALSE otherwise.
 */
GLboolean
_swrast_culltriangle( struct gl_context *ctx,
                      const SWvertex *v0,
                      const SWvertex *v1,
                      const SWvertex *v2 )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLfloat ex = v1->attrib[FRAG_ATTRIB_WPOS][0] - v0->attrib[FRAG_ATTRIB_WPOS][0];
   GLfloat ey = v1->attrib[FRAG_ATTRIB_WPOS][1] - v0->attrib[FRAG_ATTRIB_WPOS][1];
   GLfloat fx = v2->attrib[FRAG_ATTRIB_WPOS][0] - v0->attrib[FRAG_ATTRIB_WPOS][0];
   GLfloat fy = v2->attrib[FRAG_ATTRIB_WPOS][1] - v0->attrib[FRAG_ATTRIB_WPOS][1];
   GLfloat c = ex*fy-ey*fx;

   if (c * swrast->_BackfaceSign * swrast->_BackfaceCullSign <= 0.0F)
      return GL_FALSE;

   return GL_TRUE;
}



/*
 * Render a flat-shaded RGBA triangle.
 */
#define NAME flat_rgba_triangle
#define INTERP_Z 1
#define SETUP_CODE				\
   ASSERT(!ctx->Texture._EnabledCoord);\
   ASSERT(ctx->Light.ShadeModel==GL_FLAT);	\
   span.interpMask |= SPAN_RGBA;		\
   span.red = ChanToFixed(v2->color[0]);	\
   span.green = ChanToFixed(v2->color[1]);	\
   span.blue = ChanToFixed(v2->color[2]);	\
   span.alpha = ChanToFixed(v2->color[3]);	\
   span.redStep = 0;				\
   span.greenStep = 0;				\
   span.blueStep = 0;				\
   span.alphaStep = 0;
#define RENDER_SPAN( span )  _swrast_write_rgba_span(ctx, &span);
#include "s_tritemp.h"



/*
 * Render a smooth-shaded RGBA triangle.
 */
#define NAME smooth_rgba_triangle
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define SETUP_CODE				\
   {						\
      /* texturing must be off */		\
      ASSERT(!ctx->Texture._EnabledCoord);	\
      ASSERT(ctx->Light.ShadeModel==GL_SMOOTH);	\
   }
#define RENDER_SPAN( span )  _swrast_write_rgba_span(ctx, &span);
#include "s_tritemp.h"



/*
 * Render an RGB, GL_DECAL, textured triangle.
 * Interpolate S,T only w/out mipmapping or perspective correction.
 *
 * No fog.  No depth testing.
 */
#define NAME simple_textured_triangle
#define INTERP_INT_TEX 1
#define S_SCALE twidth
#define T_SCALE theight

#define SETUP_CODE							\
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffer;	\
   const struct gl_texture_object *obj = 				\
      ctx->Texture.Unit.CurrentTex[TEXTURE_2D_INDEX];		\
   const struct gl_texture_image *texImg =				\
      obj->Image[0][obj->BaseLevel];					\
   const struct swrast_texture_image *swImg =				\
      swrast_texture_image_const(texImg);				\
   const GLfloat twidth = (GLfloat) texImg->Width;			\
   const GLfloat theight = (GLfloat) texImg->Height;			\
   const GLint twidth_log2 = texImg->WidthLog2;				\
   const GLubyte *texture = (const GLubyte *) swImg->Map;		\
   const GLint smask = texImg->Width - 1;				\
   const GLint tmask = texImg->Height - 1;				\
   ASSERT(texImg->TexFormat == MESA_FORMAT_RGB888);			\
   if (!rb || !texture) {						\
      return;								\
   }

#define RENDER_SPAN( span )						\
   GLuint i;								\
   GLubyte rgba[MAX_WIDTH][4];						\
   span.intTex[0] -= FIXED_HALF; /* off-by-one error? */		\
   span.intTex[1] -= FIXED_HALF;					\
   for (i = 0; i < span.end; i++) {					\
      GLint s = FixedToInt(span.intTex[0]) & smask;			\
      GLint t = FixedToInt(span.intTex[1]) & tmask;			\
      GLint pos = (t << twidth_log2) + s;				\
      pos = pos + pos + pos;  /* multiply by 3 */			\
      rgba[i][RCOMP] = texture[pos+2];					\
      rgba[i][GCOMP] = texture[pos+1];					\
      rgba[i][BCOMP] = texture[pos+0];					\
      rgba[i][ACOMP] = 0xff;                                            \
      span.intTex[0] += span.intTexStep[0];				\
      span.intTex[1] += span.intTexStep[1];				\
   }									\
   _swrast_put_row(ctx, rb, GL_UNSIGNED_BYTE, span.end,                 \
                   span.x, span.y, rgba, NULL);

#include "s_tritemp.h"



/*
 * Render an RGB, GL_DECAL, textured triangle.
 * Interpolate S,T, GL_LESS depth test, w/out mipmapping or
 * perspective correction.
 * Depth buffer bits must be <= sizeof(DEFAULT_SOFTWARE_DEPTH_TYPE)
 *
 * No fog.
 */
#define NAME simple_z_textured_triangle
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_INT_TEX 1
#define S_SCALE twidth
#define T_SCALE theight

#define SETUP_CODE							\
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffer;	\
   const struct gl_texture_object *obj = 				\
      ctx->Texture.Unit.CurrentTex[TEXTURE_2D_INDEX];		\
   const struct gl_texture_image *texImg = 				\
       obj->Image[0][obj->BaseLevel]; 					\
   const struct swrast_texture_image *swImg =				\
      swrast_texture_image_const(texImg);				\
   const GLfloat twidth = (GLfloat) texImg->Width;			\
   const GLfloat theight = (GLfloat) texImg->Height;			\
   const GLint twidth_log2 = texImg->WidthLog2;				\
   const GLubyte *texture = (const GLubyte *) swImg->Map;		\
   const GLint smask = texImg->Width - 1;				\
   const GLint tmask = texImg->Height - 1;				\
   ASSERT(texImg->TexFormat == MESA_FORMAT_RGB888);			\
   if (!rb || !texture) {						\
      return;								\
   }

#define RENDER_SPAN( span )						\
   GLuint i;				    				\
   GLubyte rgba[MAX_WIDTH][4];						\
   span.intTex[0] -= FIXED_HALF; /* off-by-one error? */		\
   span.intTex[1] -= FIXED_HALF;					\
   for (i = 0; i < span.end; i++) {					\
      const GLuint z = FixedToDepth(span.z);				\
      if (z < zRow[i]) {						\
         GLint s = FixedToInt(span.intTex[0]) & smask;			\
         GLint t = FixedToInt(span.intTex[1]) & tmask;			\
         GLint pos = (t << twidth_log2) + s;				\
         pos = pos + pos + pos;  /* multiply by 3 */			\
         rgba[i][RCOMP] = texture[pos+2];				\
         rgba[i][GCOMP] = texture[pos+1];				\
         rgba[i][BCOMP] = texture[pos+0];				\
         rgba[i][ACOMP] = 0xff;          				\
         zRow[i] = z;							\
         span.array->mask[i] = 1;					\
      }									\
      else {								\
         span.array->mask[i] = 0;					\
      }									\
      span.intTex[0] += span.intTexStep[0];				\
      span.intTex[1] += span.intTexStep[1];				\
      span.z += span.zStep;						\
   }									\
   _swrast_put_row(ctx, rb, GL_UNSIGNED_BYTE,                           \
                   span.end, span.x, span.y, rgba, span.array->mask);

#include "s_tritemp.h"


#if CHAN_TYPE != GL_FLOAT

struct affine_info
{
   GLenum filter;
   GLenum format;
   GLenum envmode;
   GLint smask, tmask;
   GLint twidth_log2;
   const GLchan *texture;
   GLfixed er, eg, eb, ea;
   GLint tbytesline, tsize;
};


static inline GLint
ilerp(GLint t, GLint a, GLint b)
{
   return a + ((t * (b - a)) >> FIXED_SHIFT);
}

static inline GLint
ilerp_2d(GLint ia, GLint ib, GLint v00, GLint v10, GLint v01, GLint v11)
{
   const GLint temp0 = ilerp(ia, v00, v10);
   const GLint temp1 = ilerp(ia, v01, v11);
   return ilerp(ib, temp0, temp1);
}


/* This function can handle GL_NEAREST or GL_LINEAR sampling of 2D RGB or RGBA
 * textures with GL_REPLACE, GL_MODULATE, GL_BLEND, GL_DECAL or GL_ADD
 * texture env modes.
 */
static inline void
affine_span(struct gl_context *ctx, SWspan *span,
            struct affine_info *info)
{
   GLchan sample[4];  /* the filtered texture sample */
   const GLboolean texEnableSave = ctx->Texture._EnabledCoord;

   /* Instead of defining a function for each mode, a test is done
    * between the outer and inner loops. This is to reduce code size
    * and complexity. Observe that an optimizing compiler kills
    * unused variables (for instance tf,sf,ti,si in case of GL_NEAREST).
    */

#define NEAREST_RGB		\
   sample[RCOMP] = tex00[2];	\
   sample[GCOMP] = tex00[1];	\
   sample[BCOMP] = tex00[0];	\
   sample[ACOMP] = CHAN_MAX;

#define LINEAR_RGB							\
   sample[RCOMP] = ilerp_2d(sf, tf, tex00[2], tex01[2], tex10[2], tex11[2]);\
   sample[GCOMP] = ilerp_2d(sf, tf, tex00[1], tex01[1], tex10[1], tex11[1]);\
   sample[BCOMP] = ilerp_2d(sf, tf, tex00[0], tex01[0], tex10[0], tex11[0]);\
   sample[ACOMP] = CHAN_MAX;

#define NEAREST_RGBA  \
   sample[RCOMP] = tex00[3];	\
   sample[GCOMP] = tex00[2];	\
   sample[BCOMP] = tex00[1];	\
   sample[ACOMP] = tex00[0];

#define LINEAR_RGBA							\
   sample[RCOMP] = ilerp_2d(sf, tf, tex00[3], tex01[3], tex10[3], tex11[3]);\
   sample[GCOMP] = ilerp_2d(sf, tf, tex00[2], tex01[2], tex10[2], tex11[2]);\
   sample[BCOMP] = ilerp_2d(sf, tf, tex00[1], tex01[1], tex10[1], tex11[1]);\
   sample[ACOMP] = ilerp_2d(sf, tf, tex00[0], tex01[0], tex10[0], tex11[0])

#define MODULATE							  \
   dest[RCOMP] = span->red   * (sample[RCOMP] + 1u) >> (FIXED_SHIFT + 8); \
   dest[GCOMP] = span->green * (sample[GCOMP] + 1u) >> (FIXED_SHIFT + 8); \
   dest[BCOMP] = span->blue  * (sample[BCOMP] + 1u) >> (FIXED_SHIFT + 8); \
   dest[ACOMP] = span->alpha * (sample[ACOMP] + 1u) >> (FIXED_SHIFT + 8)

#define DECAL								\
   dest[RCOMP] = ((CHAN_MAX - sample[ACOMP]) * span->red +		\
               ((sample[ACOMP] + 1) * sample[RCOMP] << FIXED_SHIFT))	\
               >> (FIXED_SHIFT + 8);					\
   dest[GCOMP] = ((CHAN_MAX - sample[ACOMP]) * span->green +		\
               ((sample[ACOMP] + 1) * sample[GCOMP] << FIXED_SHIFT))	\
               >> (FIXED_SHIFT + 8);					\
   dest[BCOMP] = ((CHAN_MAX - sample[ACOMP]) * span->blue +		\
               ((sample[ACOMP] + 1) * sample[BCOMP] << FIXED_SHIFT))	\
               >> (FIXED_SHIFT + 8);					\
   dest[ACOMP] = FixedToInt(span->alpha)

#define BLEND								\
   dest[RCOMP] = ((CHAN_MAX - sample[RCOMP]) * span->red		\
               + (sample[RCOMP] + 1) * info->er) >> (FIXED_SHIFT + 8);	\
   dest[GCOMP] = ((CHAN_MAX - sample[GCOMP]) * span->green		\
               + (sample[GCOMP] + 1) * info->eg) >> (FIXED_SHIFT + 8);	\
   dest[BCOMP] = ((CHAN_MAX - sample[BCOMP]) * span->blue		\
               + (sample[BCOMP] + 1) * info->eb) >> (FIXED_SHIFT + 8);	\
   dest[ACOMP] = span->alpha * (sample[ACOMP] + 1) >> (FIXED_SHIFT + 8)

#define REPLACE  COPY_CHAN4(dest, sample)

#define ADD								\
   {									\
      GLint rSum = FixedToInt(span->red)   + (GLint) sample[RCOMP];	\
      GLint gSum = FixedToInt(span->green) + (GLint) sample[GCOMP];	\
      GLint bSum = FixedToInt(span->blue)  + (GLint) sample[BCOMP];	\
      dest[RCOMP] = MIN2(rSum, CHAN_MAX);				\
      dest[GCOMP] = MIN2(gSum, CHAN_MAX);				\
      dest[BCOMP] = MIN2(bSum, CHAN_MAX);				\
      dest[ACOMP] = span->alpha * (sample[ACOMP] + 1) >> (FIXED_SHIFT + 8); \
  }

/* shortcuts */

#define NEAREST_RGB_REPLACE		\
   NEAREST_RGB;				\
   dest[0] = sample[0];			\
   dest[1] = sample[1];			\
   dest[2] = sample[2];			\
   dest[3] = FixedToInt(span->alpha);

#define NEAREST_RGBA_REPLACE  \
   dest[RCOMP] = tex00[3]; \
   dest[GCOMP] = tex00[2]; \
   dest[BCOMP] = tex00[1]; \
   dest[ACOMP] = tex00[0]

#define SPAN_NEAREST(DO_TEX, COMPS)					\
	for (i = 0; i < span->end; i++) {				\
           /* Isn't it necessary to use FixedFloor below?? */		\
           GLint s = FixedToInt(span->intTex[0]) & info->smask;		\
           GLint t = FixedToInt(span->intTex[1]) & info->tmask;		\
           GLint pos = (t << info->twidth_log2) + s;			\
           const GLchan *tex00 = info->texture + COMPS * pos;		\
           DO_TEX;							\
           span->red += span->redStep;					\
	   span->green += span->greenStep;				\
           span->blue += span->blueStep;				\
	   span->alpha += span->alphaStep;				\
	   span->intTex[0] += span->intTexStep[0];			\
	   span->intTex[1] += span->intTexStep[1];			\
           dest += 4;							\
	}

#define SPAN_LINEAR(DO_TEX, COMPS)					\
	for (i = 0; i < span->end; i++) {				\
           /* Isn't it necessary to use FixedFloor below?? */		\
           const GLint s = FixedToInt(span->intTex[0]) & info->smask;	\
           const GLint t = FixedToInt(span->intTex[1]) & info->tmask;	\
           const GLfixed sf = span->intTex[0] & FIXED_FRAC_MASK;	\
           const GLfixed tf = span->intTex[1] & FIXED_FRAC_MASK;	\
           const GLint pos = (t << info->twidth_log2) + s;		\
           const GLchan *tex00 = info->texture + COMPS * pos;		\
           const GLchan *tex10 = tex00 + info->tbytesline;		\
           const GLchan *tex01 = tex00 + COMPS;				\
           const GLchan *tex11 = tex10 + COMPS;				\
           if (t == info->tmask) {					\
              tex10 -= info->tsize;					\
              tex11 -= info->tsize;					\
           }								\
           if (s == info->smask) {					\
              tex01 -= info->tbytesline;				\
              tex11 -= info->tbytesline;				\
           }								\
           DO_TEX;							\
           span->red += span->redStep;					\
	   span->green += span->greenStep;				\
           span->blue += span->blueStep;				\
	   span->alpha += span->alphaStep;				\
	   span->intTex[0] += span->intTexStep[0];			\
	   span->intTex[1] += span->intTexStep[1];			\
           dest += 4;							\
	}


   GLuint i;
   GLchan *dest = span->array->rgba[0];

   /* Disable tex units so they're not re-applied in swrast_write_rgba_span */
   ctx->Texture._EnabledCoord = GL_FALSE;

   span->intTex[0] -= FIXED_HALF;
   span->intTex[1] -= FIXED_HALF;
   switch (info->filter) {
   case GL_NEAREST:
      switch (info->format) {
      case MESA_FORMAT_RGB888:
         switch (info->envmode) {
         case GL_MODULATE:
            SPAN_NEAREST(NEAREST_RGB;MODULATE,3);
            break;
         case GL_DECAL:
         case GL_REPLACE:
            SPAN_NEAREST(NEAREST_RGB_REPLACE,3);
            break;
         case GL_BLEND:
            SPAN_NEAREST(NEAREST_RGB;BLEND,3);
            break;
         case GL_ADD:
            SPAN_NEAREST(NEAREST_RGB;ADD,3);
            break;
         default:
            _mesa_problem(ctx, "bad tex env mode in SPAN_LINEAR");
            return;
         }
         break;
      case MESA_FORMAT_RGBA8888:
         switch(info->envmode) {
         case GL_MODULATE:
            SPAN_NEAREST(NEAREST_RGBA;MODULATE,4);
            break;
         case GL_DECAL:
            SPAN_NEAREST(NEAREST_RGBA;DECAL,4);
            break;
         case GL_BLEND:
            SPAN_NEAREST(NEAREST_RGBA;BLEND,4);
            break;
         case GL_ADD:
            SPAN_NEAREST(NEAREST_RGBA;ADD,4);
            break;
         case GL_REPLACE:
            SPAN_NEAREST(NEAREST_RGBA_REPLACE,4);
            break;
         default:
            _mesa_problem(ctx, "bad tex env mode (2) in SPAN_LINEAR");
            return;
         }
         break;
      }
      break;

   case GL_LINEAR:
      span->intTex[0] -= FIXED_HALF;
      span->intTex[1] -= FIXED_HALF;
      switch (info->format) {
      case MESA_FORMAT_RGB888:
         switch (info->envmode) {
         case GL_MODULATE:
            SPAN_LINEAR(LINEAR_RGB;MODULATE,3);
            break;
         case GL_DECAL:
         case GL_REPLACE:
            SPAN_LINEAR(LINEAR_RGB;REPLACE,3);
            break;
         case GL_BLEND:
            SPAN_LINEAR(LINEAR_RGB;BLEND,3);
            break;
         case GL_ADD:
            SPAN_LINEAR(LINEAR_RGB;ADD,3);
            break;
         default:
            _mesa_problem(ctx, "bad tex env mode (3) in SPAN_LINEAR");
            return;
         }
         break;
      case MESA_FORMAT_RGBA8888:
         switch (info->envmode) {
         case GL_MODULATE:
            SPAN_LINEAR(LINEAR_RGBA;MODULATE,4);
            break;
         case GL_DECAL:
            SPAN_LINEAR(LINEAR_RGBA;DECAL,4);
            break;
         case GL_BLEND:
            SPAN_LINEAR(LINEAR_RGBA;BLEND,4);
            break;
         case GL_ADD:
            SPAN_LINEAR(LINEAR_RGBA;ADD,4);
            break;
         case GL_REPLACE:
            SPAN_LINEAR(LINEAR_RGBA;REPLACE,4);
            break;
         default:
            _mesa_problem(ctx, "bad tex env mode (4) in SPAN_LINEAR");
            return;
         }
         break;
      }
      break;
   }
   span->interpMask &= ~SPAN_RGBA;
   ASSERT(span->arrayMask & SPAN_RGBA);

   _swrast_write_rgba_span(ctx, span);

   /* re-enable texture units */
   ctx->Texture._EnabledCoord = texEnableSave;

#undef SPAN_NEAREST
#undef SPAN_LINEAR
}



/*
 * Render an RGB/RGBA textured triangle without perspective correction.
 */
#define NAME affine_textured_triangle
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_INT_TEX 1
#define S_SCALE twidth
#define T_SCALE theight

#define SETUP_CODE							\
   struct affine_info info;						\
   struct gl_texture_unit *unit = &ctx->Texture.Unit;			\
   const struct gl_texture_object *obj = 				\
      ctx->Texture.Unit.CurrentTex[TEXTURE_2D_INDEX];		\
   const struct gl_texture_image *texImg = 				\
      obj->Image[0][obj->BaseLevel]; 					\
   const struct swrast_texture_image *swImg =				\
      swrast_texture_image_const(texImg);				\
   const GLfloat twidth = (GLfloat) texImg->Width;			\
   const GLfloat theight = (GLfloat) texImg->Height;			\
   info.texture = (const GLchan *) swImg->Map;				\
   info.twidth_log2 = texImg->WidthLog2;				\
   info.smask = texImg->Width - 1;					\
   info.tmask = texImg->Height - 1;					\
   info.format = texImg->TexFormat;					\
   info.filter = obj->Sampler.MinFilter;				\
   info.envmode = unit->EnvMode;					\
   info.er = 0;					\
   info.eg = 0;					\
   info.eb = 0;					\
   span.arrayMask |= SPAN_RGBA;						\
									\
   if (info.envmode == GL_BLEND) {					\
      /* potential off-by-one error here? (1.0f -> 2048 -> 0) */	\
      info.er = FloatToFixed(unit->EnvColor[RCOMP] * CHAN_MAXF);	\
      info.eg = FloatToFixed(unit->EnvColor[GCOMP] * CHAN_MAXF);	\
      info.eb = FloatToFixed(unit->EnvColor[BCOMP] * CHAN_MAXF);	\
      info.ea = FloatToFixed(unit->EnvColor[ACOMP] * CHAN_MAXF);	\
   }									\
   if (!info.texture) {							\
      /* this shouldn't happen */					\
      return;								\
   }									\
									\
   switch (info.format) {						\
   case MESA_FORMAT_RGB888:						\
      info.tbytesline = texImg->Width * 3;				\
      break;								\
   case MESA_FORMAT_RGBA8888:						\
      info.tbytesline = texImg->Width * 4;				\
      break;								\
   default:								\
      _mesa_problem(NULL, "Bad texture format in affine_texture_triangle");\
      return;								\
   }									\
   info.tsize = texImg->Height * info.tbytesline;

#define RENDER_SPAN( span )   affine_span(ctx, &span, &info);

#include "s_tritemp.h"



struct persp_info
{
   GLenum filter;
   GLenum format;
   GLenum envmode;
   GLint smask, tmask;
   GLint twidth_log2;
   const GLchan *texture;
   GLfixed er, eg, eb, ea;   /* texture env color */
   GLint tbytesline, tsize;
};


static inline void
fast_persp_span(struct gl_context *ctx, SWspan *span,
		struct persp_info *info)
{
   GLchan sample[4];  /* the filtered texture sample */

  /* Instead of defining a function for each mode, a test is done
   * between the outer and inner loops. This is to reduce code size
   * and complexity. Observe that an optimizing compiler kills
   * unused variables (for instance tf,sf,ti,si in case of GL_NEAREST).
   */
#define SPAN_NEAREST(DO_TEX,COMP)					\
	for (i = 0; i < span->end; i++) {				\
           GLdouble invQ = tex_coord[2] ?				\
                                 (1.0 / tex_coord[2]) : 1.0;            \
           GLfloat s_tmp = (GLfloat) (tex_coord[0] * invQ);		\
           GLfloat t_tmp = (GLfloat) (tex_coord[1] * invQ);		\
           GLint s = IFLOOR(s_tmp) & info->smask;	        	\
           GLint t = IFLOOR(t_tmp) & info->tmask;	        	\
           GLint pos = (t << info->twidth_log2) + s;			\
           const GLchan *tex00 = info->texture + COMP * pos;		\
           DO_TEX;							\
           span->red += span->redStep;					\
	   span->green += span->greenStep;				\
           span->blue += span->blueStep;				\
	   span->alpha += span->alphaStep;				\
	   tex_coord[0] += tex_step[0];					\
	   tex_coord[1] += tex_step[1];					\
	   tex_coord[2] += tex_step[2];					\
           dest += 4;							\
	}

#define SPAN_LINEAR(DO_TEX,COMP)					\
	for (i = 0; i < span->end; i++) {				\
           GLdouble invQ = tex_coord[2] ?				\
                                 (1.0 / tex_coord[2]) : 1.0;            \
           const GLfloat s_tmp = (GLfloat) (tex_coord[0] * invQ);	\
           const GLfloat t_tmp = (GLfloat) (tex_coord[1] * invQ);	\
           const GLfixed s_fix = FloatToFixed(s_tmp) - FIXED_HALF;	\
           const GLfixed t_fix = FloatToFixed(t_tmp) - FIXED_HALF;      \
           const GLint s = FixedToInt(FixedFloor(s_fix)) & info->smask;	\
           const GLint t = FixedToInt(FixedFloor(t_fix)) & info->tmask;	\
           const GLfixed sf = s_fix & FIXED_FRAC_MASK;			\
           const GLfixed tf = t_fix & FIXED_FRAC_MASK;			\
           const GLint pos = (t << info->twidth_log2) + s;		\
           const GLchan *tex00 = info->texture + COMP * pos;		\
           const GLchan *tex10 = tex00 + info->tbytesline;		\
           const GLchan *tex01 = tex00 + COMP;				\
           const GLchan *tex11 = tex10 + COMP;				\
           if (t == info->tmask) {					\
              tex10 -= info->tsize;					\
              tex11 -= info->tsize;					\
           }								\
           if (s == info->smask) {					\
              tex01 -= info->tbytesline;				\
              tex11 -= info->tbytesline;				\
           }								\
           DO_TEX;							\
           span->red   += span->redStep;				\
	   span->green += span->greenStep;				\
           span->blue  += span->blueStep;				\
	   span->alpha += span->alphaStep;				\
	   tex_coord[0] += tex_step[0];					\
	   tex_coord[1] += tex_step[1];					\
	   tex_coord[2] += tex_step[2];					\
           dest += 4;							\
	}

   GLuint i;
   GLfloat tex_coord[3], tex_step[3];
   GLchan *dest = span->array->rgba[0];

   const GLboolean texEnableSave = ctx->Texture._EnabledCoord;
   ctx->Texture._EnabledCoord = GL_FALSE;

   tex_coord[0] = span->attrStart[FRAG_ATTRIB_TEX][0]  * (info->smask + 1);
   tex_step[0] = span->attrStepX[FRAG_ATTRIB_TEX][0] * (info->smask + 1);
   tex_coord[1] = span->attrStart[FRAG_ATTRIB_TEX][1] * (info->tmask + 1);
   tex_step[1] = span->attrStepX[FRAG_ATTRIB_TEX][1] * (info->tmask + 1);
   /* span->attrStart[FRAG_ATTRIB_TEX0][2] only if 3D-texturing, here only 2D */
   tex_coord[2] = span->attrStart[FRAG_ATTRIB_TEX][3];
   tex_step[2] = span->attrStepX[FRAG_ATTRIB_TEX][3];

   switch (info->filter) {
   case GL_NEAREST:
      switch (info->format) {
      case MESA_FORMAT_RGB888:
         switch (info->envmode) {
         case GL_MODULATE:
            SPAN_NEAREST(NEAREST_RGB;MODULATE,3);
            break;
         case GL_DECAL:
         case GL_REPLACE:
            SPAN_NEAREST(NEAREST_RGB_REPLACE,3);
            break;
         case GL_BLEND:
            SPAN_NEAREST(NEAREST_RGB;BLEND,3);
            break;
         case GL_ADD:
            SPAN_NEAREST(NEAREST_RGB;ADD,3);
            break;
         default:
            _mesa_problem(ctx, "bad tex env mode (5) in SPAN_LINEAR");
            return;
         }
         break;
      case MESA_FORMAT_RGBA8888:
         switch(info->envmode) {
         case GL_MODULATE:
            SPAN_NEAREST(NEAREST_RGBA;MODULATE,4);
            break;
         case GL_DECAL:
            SPAN_NEAREST(NEAREST_RGBA;DECAL,4);
            break;
         case GL_BLEND:
            SPAN_NEAREST(NEAREST_RGBA;BLEND,4);
            break;
         case GL_ADD:
            SPAN_NEAREST(NEAREST_RGBA;ADD,4);
            break;
         case GL_REPLACE:
            SPAN_NEAREST(NEAREST_RGBA_REPLACE,4);
            break;
         default:
            _mesa_problem(ctx, "bad tex env mode (6) in SPAN_LINEAR");
            return;
         }
         break;
      }
      break;

   case GL_LINEAR:
      switch (info->format) {
      case MESA_FORMAT_RGB888:
         switch (info->envmode) {
         case GL_MODULATE:
            SPAN_LINEAR(LINEAR_RGB;MODULATE,3);
            break;
         case GL_DECAL:
         case GL_REPLACE:
            SPAN_LINEAR(LINEAR_RGB;REPLACE,3);
            break;
         case GL_BLEND:
            SPAN_LINEAR(LINEAR_RGB;BLEND,3);
            break;
         case GL_ADD:
            SPAN_LINEAR(LINEAR_RGB;ADD,3);
            break;
         default:
            _mesa_problem(ctx, "bad tex env mode (7) in SPAN_LINEAR");
            return;
         }
         break;
      case MESA_FORMAT_RGBA8888:
         switch (info->envmode) {
         case GL_MODULATE:
            SPAN_LINEAR(LINEAR_RGBA;MODULATE,4);
            break;
         case GL_DECAL:
            SPAN_LINEAR(LINEAR_RGBA;DECAL,4);
            break;
         case GL_BLEND:
            SPAN_LINEAR(LINEAR_RGBA;BLEND,4);
            break;
         case GL_ADD:
            SPAN_LINEAR(LINEAR_RGBA;ADD,4);
            break;
         case GL_REPLACE:
            SPAN_LINEAR(LINEAR_RGBA;REPLACE,4);
            break;
         default:
            _mesa_problem(ctx, "bad tex env mode (8) in SPAN_LINEAR");
            return;
         }
         break;
      }
      break;
   }
   
   ASSERT(span->arrayMask & SPAN_RGBA);
   _swrast_write_rgba_span(ctx, span);

#undef SPAN_NEAREST
#undef SPAN_LINEAR

   /* restore state */
   ctx->Texture._EnabledCoord = texEnableSave;
}


/*
 * Render an perspective corrected RGB/RGBA textured triangle.
 * The Q (aka V in Mesa) coordinate must be zero such that the divide
 * by interpolated Q/W comes out right.
 *
 */
#define NAME persp_textured_triangle
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_ATTRIBS 1

#define SETUP_CODE							\
   struct persp_info info;						\
   const struct gl_texture_unit *unit = &ctx->Texture.Unit;		\
   const struct gl_texture_object *obj = 				\
      ctx->Texture.Unit.CurrentTex[TEXTURE_2D_INDEX];		\
   const struct gl_texture_image *texImg = 				\
      obj->Image[0][obj->BaseLevel];			 		\
   const struct swrast_texture_image *swImg =				\
      swrast_texture_image_const(texImg);				\
   info.texture = (const GLchan *) swImg->Map;				\
   info.twidth_log2 = texImg->WidthLog2;				\
   info.smask = texImg->Width - 1;					\
   info.tmask = texImg->Height - 1;					\
   info.format = texImg->TexFormat;					\
   info.filter = obj->Sampler.MinFilter;				\
   info.envmode = unit->EnvMode;					\
   info.er = 0;					\
   info.eg = 0;					\
   info.eb = 0;					\
									\
   if (info.envmode == GL_BLEND) {					\
      /* potential off-by-one error here? (1.0f -> 2048 -> 0) */	\
      info.er = FloatToFixed(unit->EnvColor[RCOMP] * CHAN_MAXF);	\
      info.eg = FloatToFixed(unit->EnvColor[GCOMP] * CHAN_MAXF);	\
      info.eb = FloatToFixed(unit->EnvColor[BCOMP] * CHAN_MAXF);	\
      info.ea = FloatToFixed(unit->EnvColor[ACOMP] * CHAN_MAXF);	\
   }									\
   if (!info.texture) {							\
      /* this shouldn't happen */					\
      return;								\
   }									\
									\
   switch (info.format) {						\
   case MESA_FORMAT_RGB888:						\
      info.tbytesline = texImg->Width * 3;				\
      break;								\
   case MESA_FORMAT_RGBA8888:						\
      info.tbytesline = texImg->Width * 4;				\
      break;								\
   default:								\
      _mesa_problem(NULL, "Bad texture format in persp_textured_triangle");\
      return;								\
   }									\
   info.tsize = texImg->Height * info.tbytesline;

#define RENDER_SPAN( span )			\
   span.interpMask &= ~SPAN_RGBA;		\
   span.arrayMask |= SPAN_RGBA;			\
   fast_persp_span(ctx, &span, &info);

#include "s_tritemp.h"

#endif /*CHAN_TYPE != GL_FLOAT*/



/*
 * Render an RGBA triangle with arbitrary attributes.
 */
#define NAME general_triangle
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_ATTRIBS 1
#define RENDER_SPAN( span )   _swrast_write_rgba_span(ctx, &span);
#include "s_tritemp.h"



static void
nodraw_triangle( struct gl_context *ctx,
                 const SWvertex *v0,
                 const SWvertex *v1,
                 const SWvertex *v2 )
{
   (void) (ctx && v0 && v1 && v2);
}



#ifdef DEBUG

/* record the current triangle function name */
const char *_mesa_triFuncName = NULL;

#define USE(triFunc)				\
do {						\
    _mesa_triFuncName = #triFunc;		\
    /*printf("%s\n", _mesa_triFuncName);*/	\
    swrast->Triangle = triFunc;			\
} while (0)

#else

#define USE(triFunc)  swrast->Triangle = triFunc;

#endif




/*
 * Determine which triangle rendering function to use given the current
 * rendering context.
 *
 * Please update the summary flag _SWRAST_NEW_TRIANGLE if you add or
 * remove tests to this code.
 */
void
_swrast_choose_triangle( struct gl_context *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (ctx->Polygon.CullFlag &&
       ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK) {
      USE(nodraw_triangle);
      return;
   }

   if (ctx->RenderMode==GL_RENDER) {

      if (ctx->Polygon.SmoothFlag) {
         _swrast_set_aa_triangle_function(ctx);
         ASSERT(swrast->Triangle);
         return;
      }

      /*
       * XXX should examine swrast->_ActiveAttribMask to determine what
       * needs to be interpolated.
       */
      if (ctx->Texture._EnabledCoord ||
          swrast->_FogEnabled) {
         /* Ugh, we do a _lot_ of tests to pick the best textured tri func */
         const struct gl_texture_object *texObj2D;
         const struct gl_texture_image *texImg;
         const struct swrast_texture_image *swImg;
         GLenum minFilter, magFilter, envMode;
         gl_format format;
         texObj2D = ctx->Texture.Unit.CurrentTex[TEXTURE_2D_INDEX];

         texImg = texObj2D ? texObj2D->Image[0][texObj2D->BaseLevel] : NULL;
         swImg = swrast_texture_image_const(texImg);

         format = texImg ? texImg->TexFormat : MESA_FORMAT_NONE;
         minFilter = texObj2D ? texObj2D->Sampler.MinFilter : GL_NONE;
         magFilter = texObj2D ? texObj2D->Sampler.MagFilter : GL_NONE;
         envMode = ctx->Texture.Unit.EnvMode;

         /* First see if we can use an optimized 2-D texture function */
         if (ctx->Texture._EnabledCoord
             && ctx->Texture._Enabled
             && ctx->Texture.Unit._ReallyEnabled == TEXTURE_2D_BIT
             && texObj2D->Sampler.WrapS == GL_REPEAT
             && texObj2D->Sampler.WrapT == GL_REPEAT
             && swImg->_IsPowerOfTwo
             && texImg->Border == 0
             && texImg->Width == swImg->RowStride
             && (format == MESA_FORMAT_RGB888 || format == MESA_FORMAT_RGBA8888)
             && minFilter == magFilter
             && !swrast->_FogEnabled
             && ctx->Texture.Unit.EnvMode != GL_COMBINE_EXT
             && ctx->Texture.Unit.EnvMode != GL_COMBINE4_NV) {
	    if (ctx->Hint.PerspectiveCorrection==GL_FASTEST) {
	       if (minFilter == GL_NEAREST
		   && format == MESA_FORMAT_RGB888
		   && (envMode == GL_REPLACE || envMode == GL_DECAL)
		   && ((swrast->_RasterMask == (DEPTH_BIT | TEXTURE_BIT)
			&& ctx->Depth.Func == GL_LESS
			&& ctx->Depth.Mask == GL_TRUE)
		       || swrast->_RasterMask == TEXTURE_BIT)
		   && ctx->Polygon.StippleFlag == GL_FALSE
                   && ctx->DrawBuffer->Visual.depthBits <= 16) {
		  if (swrast->_RasterMask == (DEPTH_BIT | TEXTURE_BIT)) {
		     USE(simple_z_textured_triangle);
		  }
		  else {
		     USE(simple_textured_triangle);
		  }
	       }
	       else {
#if CHAN_BITS != 8
                  USE(general_triangle);
#else
                  if (format == MESA_FORMAT_RGBA8888 && !_mesa_little_endian()) {
                     /* We only handle RGBA8888 correctly on little endian
                      * in the optimized code above.
                      */
                     USE(general_triangle);
                  }
                  else {
                     USE(affine_textured_triangle);
                 }
#endif
	       }
	    }
	    else {
#if CHAN_BITS != 8
               USE(general_triangle);
#else
               USE(persp_textured_triangle);
#endif
	    }
	 }
         else {
            /* general case textured triangles */
            USE(general_triangle);
         }
      }
      else {
         ASSERT(!swrast->_FogEnabled);
         ASSERT(!_mesa_need_secondary_color(ctx));
	 if (ctx->Light.ShadeModel==GL_SMOOTH) {
	    /* smooth shaded, no texturing, stippled or some raster ops */
#if CHAN_BITS != 8
               USE(general_triangle);
#else
               USE(smooth_rgba_triangle);
#endif
	 }
	 else {
	    /* flat shaded, no texturing, stippled or some raster ops */
#if CHAN_BITS != 8
            USE(general_triangle);
#else
            USE(flat_rgba_triangle);
#endif
	 }
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      USE(_swrast_feedback_triangle);
   }
   else {
      /* GL_SELECT mode */
      USE(_swrast_select_triangle);
   }
}
