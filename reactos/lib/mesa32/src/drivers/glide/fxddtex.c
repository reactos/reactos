/*
 * Mesa 3-D graphics library
 * Version:  4.0
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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

/* Authors:
 *    David Bucciarelli
 *    Brian Paul
 *    Daryll Strauss
 *    Keith Whitwell
 *    Daniel Borca
 *    Hiroshi Morii
 */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "fxdrv.h"
#include "enums.h"
#include "image.h"
#include "teximage.h"
#include "texformat.h"
#include "texobj.h"
#include "texstore.h"
#include "texutil.h"


void
fxPrintTextureData(tfxTexInfo * ti)
{
   fprintf(stderr, "Texture Data:\n");
   if (ti->tObj) {
      fprintf(stderr, "\tName: %d\n", ti->tObj->Name);
      fprintf(stderr, "\tBaseLevel: %d\n", ti->tObj->BaseLevel);
      fprintf(stderr, "\tSize: %d x %d\n",
	      ti->tObj->Image[ti->tObj->BaseLevel]->Width,
	      ti->tObj->Image[ti->tObj->BaseLevel]->Height);
   }
   else
      fprintf(stderr, "\tName: UNNAMED\n");
   fprintf(stderr, "\tLast used: %d\n", ti->lastTimeUsed);
   fprintf(stderr, "\tTMU: %ld\n", ti->whichTMU);
   fprintf(stderr, "\t%s\n", (ti->isInTM) ? "In TMU" : "Not in TMU");
   if (ti->tm[0])
      fprintf(stderr, "\tMem0: %x-%x\n", (unsigned) ti->tm[0]->startAddr,
	      (unsigned) ti->tm[0]->endAddr);
   if (ti->tm[1])
      fprintf(stderr, "\tMem1: %x-%x\n", (unsigned) ti->tm[1]->startAddr,
	      (unsigned) ti->tm[1]->endAddr);
   fprintf(stderr, "\tMipmaps: %d-%d\n", ti->minLevel, ti->maxLevel);
   fprintf(stderr, "\tFilters: min %d max %d\n",
	   (int) ti->minFilt, (int) ti->maxFilt);
   fprintf(stderr, "\tClamps: s %d t %d\n", (int) ti->sClamp,
	   (int) ti->tClamp);
   fprintf(stderr, "\tScales: s %f t %f\n", ti->sScale, ti->tScale);
   fprintf(stderr, "\t%s\n",
	   (ti->fixedPalette) ? "Fixed palette" : "Non fixed palette");
   fprintf(stderr, "\t%s\n", (ti->validated) ? "Validated" : "Not validated");
}


/************************************************************************/
/*************************** Texture Mapping ****************************/
/************************************************************************/

static void
fxTexInvalidate(GLcontext * ctx, struct gl_texture_object *tObj)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxTexInfo *ti;

   ti = fxTMGetTexInfo(tObj);
   if (ti->isInTM)
      fxTMMoveOutTM(fxMesa, tObj);	/* TO DO: SLOW but easy to write */

   ti->validated = GL_FALSE;
   fxMesa->new_state |= FX_NEW_TEXTURING;
}

static tfxTexInfo *
fxAllocTexObjData(fxMesaContext fxMesa)
{
   tfxTexInfo *ti;

   if (!(ti = CALLOC(sizeof(tfxTexInfo)))) {
      fprintf(stderr, "fxAllocTexObjData: ERROR: out of memory !\n");
      fxCloseHardware();
      exit(-1);
   }

   ti->validated = GL_FALSE;
   ti->isInTM = GL_FALSE;

   ti->whichTMU = FX_TMU_NONE;

   ti->tm[FX_TMU0] = NULL;
   ti->tm[FX_TMU1] = NULL;

   ti->minFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
   ti->maxFilt = GR_TEXTUREFILTER_BILINEAR;

   ti->sClamp = GR_TEXTURECLAMP_WRAP;
   ti->tClamp = GR_TEXTURECLAMP_WRAP;

   ti->mmMode = GR_MIPMAP_NEAREST;
   ti->LODblend = FXFALSE;

   return ti;
}

void
fxDDTexBind(GLcontext * ctx, GLenum target, struct gl_texture_object *tObj)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxTexInfo *ti;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxDDTexBind(%d, %x)\n", tObj->Name, (GLuint)tObj->DriverData);
   }

   if (target != GL_TEXTURE_2D)
      return;

   if (!tObj->DriverData) {
      tObj->DriverData = fxAllocTexObjData(fxMesa);
   }

   ti = fxTMGetTexInfo(tObj);

   fxMesa->texBindNumber++;
   ti->lastTimeUsed = fxMesa->texBindNumber;

   fxMesa->new_state |= FX_NEW_TEXTURING;
}

void
fxDDTexEnv(GLcontext * ctx, GLenum target, GLenum pname,
	   const GLfloat * param)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      if (param)
	 fprintf(stderr, "fxDDTexEnv(%x, %x)\n", pname, (GLint) (*param));
      else
	 fprintf(stderr, "fxDDTexEnv(%x)\n", pname);
   }

   /* apply any lod biasing right now */
   if (pname == GL_TEXTURE_LOD_BIAS_EXT) {
      grTexLodBiasValue(GR_TMU0, *param);

      if (fxMesa->haveTwoTMUs) {
	 grTexLodBiasValue(GR_TMU1, *param);
      }

   }

   fxMesa->new_state |= FX_NEW_TEXTURING;
}

void
fxDDTexParam(GLcontext * ctx, GLenum target, struct gl_texture_object *tObj,
	     GLenum pname, const GLfloat * params)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLenum param = (GLenum) (GLint) params[0];
   tfxTexInfo *ti;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxDDTexParam(%d, %x, %s, %s)\n",
                      tObj->Name, (GLuint) tObj->DriverData,
                      _mesa_lookup_enum_by_nr(pname),
                      _mesa_lookup_enum_by_nr(param));
   }

   if (target != GL_TEXTURE_2D)
      return;

   if (!tObj->DriverData)
      tObj->DriverData = fxAllocTexObjData(fxMesa);

   ti = fxTMGetTexInfo(tObj);

   switch (pname) {

   case GL_TEXTURE_MIN_FILTER:
      switch (param) {
      case GL_NEAREST:
	 ti->mmMode = GR_MIPMAP_DISABLE;
	 ti->minFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
	 ti->LODblend = FXFALSE;
	 break;
      case GL_LINEAR:
	 ti->mmMode = GR_MIPMAP_DISABLE;
	 ti->minFilt = GR_TEXTUREFILTER_BILINEAR;
	 ti->LODblend = FXFALSE;
	 break;
      case GL_NEAREST_MIPMAP_LINEAR:
         /* [koolsmoky]
          * trilinear is bugged! mipmap blending produce
          * incorrect filtered colors for the smallest mipmap levels.
          * [dBorca]
          * currently Napalm can't do single-pass trilinear,
          * because the way its combiners are set. So we fall back
          * to GL_NEAREST_MIPMAP_NEAREST. We'll let true trilinear
          * enabled for V2, V3. If user shoots foot, not our problem!
          */
         if (!fxMesa->HaveCmbExt) {
	    if (fxMesa->haveTwoTMUs) {
	       ti->mmMode = GR_MIPMAP_NEAREST;
	       ti->LODblend = FXTRUE;
            } else {
	       ti->mmMode = GR_MIPMAP_NEAREST_DITHER;
	       ti->LODblend = FXFALSE;
            }
	    ti->minFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
	    break;
         }
      case GL_NEAREST_MIPMAP_NEAREST:
	 ti->mmMode = GR_MIPMAP_NEAREST;
	 ti->minFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
	 ti->LODblend = FXFALSE;
	 break;
      case GL_LINEAR_MIPMAP_LINEAR:
         /* [koolsmoky]
          * trilinear is bugged! mipmap blending produce
          * incorrect filtered colors for the smallest mipmap levels.
          * [dBorca]
          * currently Napalm can't do single-pass trilinear,
          * because the way its combiners are set. So we fall back
          * to GL_LINEAR_MIPMAP_NEAREST. We'll let true trilinear
          * enabled for V2, V3. If user shoots foot, not our problem!
          */
         if (!fxMesa->HaveCmbExt) {
            if (fxMesa->haveTwoTMUs) {
               ti->mmMode = GR_MIPMAP_NEAREST;
               ti->LODblend = FXTRUE;
            } else {
               ti->mmMode = GR_MIPMAP_NEAREST_DITHER;
               ti->LODblend = FXFALSE;
            }
            ti->minFilt = GR_TEXTUREFILTER_BILINEAR;
            break;
         }
      case GL_LINEAR_MIPMAP_NEAREST:
	 ti->mmMode = GR_MIPMAP_NEAREST;
	 ti->minFilt = GR_TEXTUREFILTER_BILINEAR;
	 ti->LODblend = FXFALSE;
	 break;
      default:
	 break;
      }
      fxTexInvalidate(ctx, tObj);
      break;

   case GL_TEXTURE_MAG_FILTER:
      switch (param) {
      case GL_NEAREST:
	 ti->maxFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
	 break;
      case GL_LINEAR:
	 ti->maxFilt = GR_TEXTUREFILTER_BILINEAR;
	 break;
      default:
	 break;
      }
      fxTexInvalidate(ctx, tObj);
      break;

   case GL_TEXTURE_WRAP_S:
      switch (param) {
      case GL_MIRRORED_REPEAT:
         ti->sClamp = GR_TEXTURECLAMP_MIRROR_EXT;
         break;
      case GL_CLAMP_TO_EDGE: /* CLAMP discarding border */
      case GL_CLAMP:
	 ti->sClamp = GR_TEXTURECLAMP_CLAMP;
	 break;
      case GL_REPEAT:
	 ti->sClamp = GR_TEXTURECLAMP_WRAP;
	 break;
      default:
	 break;
      }
      fxMesa->new_state |= FX_NEW_TEXTURING;
      break;

   case GL_TEXTURE_WRAP_T:
      switch (param) {
      case GL_MIRRORED_REPEAT:
         ti->tClamp = GR_TEXTURECLAMP_MIRROR_EXT;
         break;
      case GL_CLAMP_TO_EDGE: /* CLAMP discarding border */
      case GL_CLAMP:
	 ti->tClamp = GR_TEXTURECLAMP_CLAMP;
	 break;
      case GL_REPEAT:
	 ti->tClamp = GR_TEXTURECLAMP_WRAP;
	 break;
      default:
	 break;
      }
      fxMesa->new_state |= FX_NEW_TEXTURING;
      break;

   case GL_TEXTURE_BORDER_COLOR:
      /* TO DO */
      break;

   case GL_TEXTURE_MIN_LOD:
      /* TO DO */
      break;
   case GL_TEXTURE_MAX_LOD:
      /* TO DO */
      break;
   case GL_TEXTURE_BASE_LEVEL:
      fxTexInvalidate(ctx, tObj);
      break;
   case GL_TEXTURE_MAX_LEVEL:
      fxTexInvalidate(ctx, tObj);
      break;

   default:
      break;
   }
}

void
fxDDTexDel(GLcontext * ctx, struct gl_texture_object *tObj)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxTexInfo *ti = fxTMGetTexInfo(tObj);

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxDDTexDel(%d, %p)\n", tObj->Name, (void *) ti);
   }

   if (!ti)
      return;

   fxTMFreeTexture(fxMesa, tObj);

   FREE(ti);
   tObj->DriverData = NULL;

   /* Free mipmap images and the texture object itself */
   _mesa_delete_texture_object(ctx, tObj);
}

/*
 * Return true if texture is resident, false otherwise.
 */
GLboolean
fxDDIsTextureResident(GLcontext *ctx, struct gl_texture_object *tObj)
{
 tfxTexInfo *ti = fxTMGetTexInfo(tObj);
 return (ti && ti->isInTM);
}



/*
 * Convert a gl_color_table texture palette to Glide's format.
 */
static GrTexTable_t
convertPalette(const fxMesaContext fxMesa, FxU32 data[256], const struct gl_color_table *table)
{
   const GLubyte *tableUB = (const GLubyte *) table->Table;
   GLint width = table->Size;
   FxU32 r, g, b, a;
   GLint i;

   ASSERT(!table->FloatTable);

   switch (table->Format) {
   case GL_INTENSITY:
      for (i = 0; i < width; i++) {
	 r = tableUB[i];
	 g = tableUB[i];
	 b = tableUB[i];
	 a = tableUB[i];
	 data[i] = (a << 24) | (r << 16) | (g << 8) | b;
      }
      return fxMesa->HavePalExt ? GR_TEXTABLE_PALETTE_6666_EXT : GR_TEXTABLE_PALETTE;
   case GL_LUMINANCE:
      for (i = 0; i < width; i++) {
	 r = tableUB[i];
	 g = tableUB[i];
	 b = tableUB[i];
	 a = 255;
	 data[i] = (a << 24) | (r << 16) | (g << 8) | b;
      }
      return GR_TEXTABLE_PALETTE;
   case GL_ALPHA:
      for (i = 0; i < width; i++) {
	 r = g = b = 255;
	 a = tableUB[i];
	 data[i] = (a << 24) | (r << 16) | (g << 8) | b;
      }
      return fxMesa->HavePalExt ? GR_TEXTABLE_PALETTE_6666_EXT : GR_TEXTABLE_PALETTE;
   case GL_LUMINANCE_ALPHA:
      for (i = 0; i < width; i++) {
	 r = g = b = tableUB[i * 2 + 0];
	 a = tableUB[i * 2 + 1];
	 data[i] = (a << 24) | (r << 16) | (g << 8) | b;
      }
      return fxMesa->HavePalExt ? GR_TEXTABLE_PALETTE_6666_EXT : GR_TEXTABLE_PALETTE;
   default:
   case GL_RGB:
      for (i = 0; i < width; i++) {
	 r = tableUB[i * 3 + 0];
	 g = tableUB[i * 3 + 1];
	 b = tableUB[i * 3 + 2];
	 a = 255;
	 data[i] = (a << 24) | (r << 16) | (g << 8) | b;
      }
      return GR_TEXTABLE_PALETTE;
   case GL_RGBA:
      for (i = 0; i < width; i++) {
	 r = tableUB[i * 4 + 0];
	 g = tableUB[i * 4 + 1];
	 b = tableUB[i * 4 + 2];
	 a = tableUB[i * 4 + 3];
	 data[i] = (a << 24) | (r << 16) | (g << 8) | b;
      }
      return fxMesa->HavePalExt ? GR_TEXTABLE_PALETTE_6666_EXT : GR_TEXTABLE_PALETTE;
   }
}


void
fxDDTexPalette(GLcontext * ctx, struct gl_texture_object *tObj)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   if (tObj) {
      /* per-texture palette */
      tfxTexInfo *ti;
      if (TDFX_DEBUG & VERBOSE_DRIVER) {
	 fprintf(stderr, "fxDDTexPalette(%d, %x)\n",
		 tObj->Name, (GLuint) tObj->DriverData);
      }
      if (!tObj->DriverData)
	 tObj->DriverData = fxAllocTexObjData(fxMesa);
      ti = fxTMGetTexInfo(tObj);
      ti->paltype = convertPalette(fxMesa, ti->palette.data, &tObj->Palette);
      fxTexInvalidate(ctx, tObj);
   }
   else {
      /* global texture palette */
      if (TDFX_DEBUG & VERBOSE_DRIVER) {
	 fprintf(stderr, "fxDDTexPalette(global)\n");
      }
      fxMesa->glbPalType = convertPalette(fxMesa, fxMesa->glbPalette.data, &ctx->Texture.Palette);
      fxMesa->new_state |= FX_NEW_TEXTURING;

      grTexDownloadTable(fxMesa->glbPalType, &(fxMesa->glbPalette));
   }
}


void
fxDDTexUseGlbPalette(GLcontext * ctx, GLboolean state)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxDDTexUseGlbPalette(%d)\n", state);
   }

   if (state) {
      fxMesa->haveGlobalPaletteTexture = 1;
   }
   else {
      fxMesa->haveGlobalPaletteTexture = 0;

      if ((ctx->Texture.Unit[0]._Current == ctx->Texture.Unit[0].Current2D) &&
	  (ctx->Texture.Unit[0]._Current != NULL)) {
	 struct gl_texture_object *tObj = ctx->Texture.Unit[0]._Current;

	 if (!tObj->DriverData)
	    tObj->DriverData = fxAllocTexObjData(fxMesa);

	 fxTexInvalidate(ctx, tObj);
      }
   }
}


static int
logbase2(int n)
{
   GLint i = 1;
   GLint log2 = 0;

   if (n < 0) {
      return -1;
   }

   while (n > i) {
      i *= 2;
      log2++;
   }
   if (i != n) {
      return -1;
   }
   else {
      return log2;
   }
}


/* fxTexGetInfo
 * w, h     - source texture width and height
 * lodlevel - Glide lod level token for the larger texture dimension
 * ar       - Glide aspect ratio token
 * sscale   - S scale factor used during triangle setup
 * tscale   - T scale factor used during triangle setup
 * wscale   - OpenGL -> Glide image width scale factor
 * hscale   - OpenGL -> Glide image height scale factor
 */
int
fxTexGetInfo(int w, int h, GrLOD_t * lodlevel, GrAspectRatio_t * ar,
	     float *sscale, float *tscale,
	     int *wscale, int *hscale)
{
   int logw, logh, ws, hs;
   GrLOD_t l;
   GrAspectRatio_t aspectratio;
   float s, t;

   logw = logbase2(w);
   logh = logbase2(h);

   l = MAX2(logw, logh);
   aspectratio = logw - logh;
   ws = hs = 1;
   s = t = 256.0f;

   /* hardware only allows a maximum aspect ratio of 8x1, so handle
    * |aspectratio| > 3 by scaling the image and using an 8x1 aspect
    * ratio
    */
   switch (aspectratio) {
   case 0:
      break;
   case 1:
      t = 128.0f;
      break;
   case 2:
      t = 64.0f;
      break;
   case 3:
      t = 32.0f;
      break;
   case -1:
      s = 128.0f;
      break;
   case -2:
      s = 64.0f;
      break;
   case -3:
      s = 32.0f;
      break;
   default:
      if (aspectratio > 3) {
         t = 32.0f;
         hs = 1 << (aspectratio - 3);
         aspectratio = GR_ASPECT_LOG2_8x1;
      } else /*if (aspectratio < -3)*/ {
         s = 32.0f;
         ws = 1 << (-aspectratio - 3);
         aspectratio = GR_ASPECT_LOG2_1x8;
      }
   }

   if (lodlevel)
      (*lodlevel) = l;

   if (ar)
      (*ar) = aspectratio;

   if (sscale)
      (*sscale) = s;

   if (tscale)
      (*tscale) = t;

   if (wscale)
      (*wscale) = ws;

   if (hscale)
      (*hscale) = hs;


   return 1;
}

static GLboolean
fxIsTexSupported(GLenum target, GLint internalFormat,
		 const struct gl_texture_image *image)
{
   if (target != GL_TEXTURE_2D)
      return GL_FALSE;

#if 0
   if (!fxTexGetInfo(image->Width, image->Height, NULL, NULL, NULL, NULL, NULL, NULL))
       return GL_FALSE;
#endif

   if (image->Border > 0)
      return GL_FALSE;

   return GL_TRUE;
}


/**********************************************************************/
/**** NEW TEXTURE IMAGE FUNCTIONS                                  ****/
/**********************************************************************/

/* Texel-fetch functions for software texturing and glGetTexImage().
 * We should have been able to use some "standard" fetch functions (which
 * may get defined in texutil.c) but we have to account for scaled texture
 * images on tdfx hardware (the 8:1 aspect ratio limit).
 * Hence, we need special functions here.
 *
 * [dBorca]
 * this better be right, if we will advertise GL_SGIS_generate_mipmap!
 */

static void
fetch_intensity8(const struct gl_texture_image *texImage,
		 GLint i, GLint j, GLint k, GLvoid * texelOut)
{
   GLchan *rgba = (GLchan *) texelOut;
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLubyte *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLubyte *) texImage->Data) + j * mml->width + i;
   rgba[RCOMP] = *texel;
   rgba[GCOMP] = *texel;
   rgba[BCOMP] = *texel;
   rgba[ACOMP] = *texel;
}


static void
fetch_luminance8(const struct gl_texture_image *texImage,
		 GLint i, GLint j, GLint k, GLvoid * texelOut)
{
   GLchan *rgba = (GLchan *) texelOut;
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLubyte *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLubyte *) texImage->Data) + j * mml->width + i;
   rgba[RCOMP] = *texel;
   rgba[GCOMP] = *texel;
   rgba[BCOMP] = *texel;
   rgba[ACOMP] = 255;
}


static void
fetch_alpha8(const struct gl_texture_image *texImage,
	     GLint i, GLint j, GLint k, GLvoid * texelOut)
{
   GLchan *rgba = (GLchan *) texelOut;
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLubyte *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLubyte *) texImage->Data) + j * mml->width + i;
   rgba[RCOMP] = 255;
   rgba[GCOMP] = 255;
   rgba[BCOMP] = 255;
   rgba[ACOMP] = *texel;
}


static void
fetch_index8(const struct gl_texture_image *texImage,
	     GLint i, GLint j, GLint k, GLvoid * texelOut)
{
   GLchan *indexOut = (GLchan *) texelOut;
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLubyte *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLubyte *) texImage->Data) + j * mml->width + i;
   *indexOut = *texel;
}


static void
fetch_luminance8_alpha8(const struct gl_texture_image *texImage,
			GLint i, GLint j, GLint k, GLvoid * texelOut)
{
   GLchan *rgba = (GLchan *) texelOut;
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLubyte *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLubyte *) texImage->Data) + (j * mml->width + i) * 2;
   rgba[RCOMP] = texel[0];
   rgba[GCOMP] = texel[0];
   rgba[BCOMP] = texel[0];
   rgba[ACOMP] = texel[1];
}


static void
fetch_r5g6b5(const struct gl_texture_image *texImage,
	     GLint i, GLint j, GLint k, GLvoid * texelOut)
{
   GLchan *rgba = (GLchan *) texelOut;
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLushort *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLushort *) texImage->Data) + j * mml->width + i;
   rgba[RCOMP] = FX_rgb_scale_5[(*texel >> 11) & 0x1F];
   rgba[GCOMP] = FX_rgb_scale_6[(*texel >>  5) & 0x3F];
   rgba[BCOMP] = FX_rgb_scale_5[ *texel        & 0x1F];
   rgba[ACOMP] = 255;
}


static void
fetch_r4g4b4a4(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLvoid * texelOut)
{
   GLchan *rgba = (GLchan *) texelOut;
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLushort *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLushort *) texImage->Data) + j * mml->width + i;
   rgba[RCOMP] = FX_rgb_scale_4[(*texel >> 12) & 0xF];
   rgba[GCOMP] = FX_rgb_scale_4[(*texel >>  8) & 0xF];
   rgba[BCOMP] = FX_rgb_scale_4[(*texel >>  4) & 0xF];
   rgba[ACOMP] = FX_rgb_scale_4[ *texel        & 0xF];
}


static void
fetch_r5g5b5a1(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLvoid * texelOut)
{
   GLchan *rgba = (GLchan *) texelOut;
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLushort *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLushort *) texImage->Data) + j * mml->width + i;
   rgba[RCOMP] = FX_rgb_scale_5[(*texel >> 11) & 0x1F];
   rgba[GCOMP] = FX_rgb_scale_5[(*texel >>  6) & 0x1F];
   rgba[BCOMP] = FX_rgb_scale_5[(*texel >>  1) & 0x1F];
   rgba[ACOMP] = ((*texel) & 0x01) * 255;
}


static void
fetch_a8r8g8b8(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLvoid * texelOut)
{
    GLchan *rgba = (GLchan *) texelOut;
    const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
    const GLuint *texel;

    i = i * mml->wScale;
    j = j * mml->hScale;

    texel = ((GLuint *) texImage->Data) + j * mml->width + i;
    rgba[RCOMP] = (((*texel) >> 16) & 0xff);
    rgba[GCOMP] = (((*texel) >>  8) & 0xff);
    rgba[BCOMP] = (((*texel)      ) & 0xff);
    rgba[ACOMP] = (((*texel) >> 24) & 0xff);
}


static void
PrintTexture(int w, int h, int c, const GLubyte * data)
{
   int i, j;
   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
	 if (c == 2)
	    fprintf(stderr, "%02x %02x  ", data[0], data[1]);
	 else if (c == 3)
	    fprintf(stderr, "%02x %02x %02x  ", data[0], data[1], data[2]);
	 data += c;
      }
      fprintf(stderr, "\n");
   }
}


GLboolean fxDDIsCompressedFormat ( GLcontext *ctx, GLenum internalFormat )
{
 if ((internalFormat == GL_COMPRESSED_RGB_FXT1_3DFX) ||
     (internalFormat == GL_COMPRESSED_RGBA_FXT1_3DFX) ||
     (internalFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT) ||
     (internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ||
     (internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) ||
     (internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT) ||
     (internalFormat == GL_RGB_S3TC) ||
     (internalFormat == GL_RGB4_S3TC) ||
     (internalFormat == GL_RGBA_S3TC) ||
     (internalFormat == GL_RGBA4_S3TC)) {
    return GL_TRUE;
 }

/* [dBorca]
 * we are handling differently the above formats from the generic
 * GL_COMPRESSED_RGB[A]. For this, we will always have to separately
 * check for the ones below!
 */

#if FX_TC_NCC || FX_TC_NAPALM
 if ((internalFormat == GL_COMPRESSED_RGB) || (internalFormat == GL_COMPRESSED_RGBA)) {
    return GL_TRUE;
 }
#endif

 return GL_FALSE;
}


GLuint fxDDCompressedTextureSize (GLcontext *ctx,
                                  GLsizei width, GLsizei height, GLsizei depth,
                                  GLenum format)
{
 GLuint size;
 int wScale, hScale;

 ASSERT(depth == 1);

 /* Determine width and height scale factors for texture.
  * Remember, Glide is limited to 8:1 aspect ratios.
  */
 fxTexGetInfo(width, height,
              NULL,       /* lod level          */
              NULL,       /* aspect ratio       */
              NULL, NULL, /* sscale, tscale     */
              &wScale, &hScale);

 width *= wScale;
 height *= hScale;

 switch (format) {
 case GL_COMPRESSED_RGB_FXT1_3DFX:
 case GL_COMPRESSED_RGBA_FXT1_3DFX:
    /* round up width to next multiple of 8, height to next multiple of 4 */
    width = (width + 7) & ~7;
    height = (height + 3) & ~3;
    /* 16 bytes per 8x4 tile of RGB[A] texels */
    size = width * height / 2;
    /* Textures smaller than 8x4 will effectively be made into 8x4 and
     * take 16 bytes.
     */
    if (size < 16)
       size = 16;
    return size;
 case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
 case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
 case GL_RGB_S3TC:
 case GL_RGB4_S3TC:
    /* round up width, height to next multiple of 4 */
    width = (width + 3) & ~3;
    height = (height + 3) & ~3;
    /* 8 bytes per 4x4 tile of RGB[A] texels */
    size = width * height / 2;
    /* Textures smaller than 4x4 will effectively be made into 4x4 and
     * take 8 bytes.
     */
    if (size < 8)
       size = 8;
    return size;
 case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
 case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
 case GL_RGBA_S3TC:
 case GL_RGBA4_S3TC:
    /* round up width, height to next multiple of 4 */
    width = (width + 3) & ~3;
    height = (height + 3) & ~3;
    /* 16 bytes per 4x4 tile of RGBA texels */
    size = width * height; /* simple! */
    /* Textures smaller than 4x4 will effectively be made into 4x4 and
     * take 16 bytes.
     */
    if (size < 16)
       size = 16;
    return size;
 case GL_COMPRESSED_RGB:
#if FX_TC_NAPALM
    {
     fxMesaContext fxMesa = FX_CONTEXT(ctx);
     if (fxMesa->type >= GR_SSTTYPE_Voodoo4) {
        return fxDDCompressedTextureSize(ctx, width, height, 1, GL_COMPRESSED_RGB_FXT1_3DFX);
     }
    }
#endif
#if FX_TC_NCC
    return (width * height * 8 >> 3) + 12 * 4;
#endif
 case GL_COMPRESSED_RGBA:
#if FX_TC_NAPALM
    {
     fxMesaContext fxMesa = FX_CONTEXT(ctx);
     if (fxMesa->type >= GR_SSTTYPE_Voodoo4) {
        return fxDDCompressedTextureSize(ctx, width, height, 1, GL_COMPRESSED_RGBA_FXT1_3DFX);
     }
    }
#endif
#if FX_TC_NCC
    return (width * height * 16 >> 3) + 12 * 4;
#endif
 default:
    _mesa_problem(ctx, "bad texformat in fxDDCompressedTextureSize");
    return 0;
 }
}


const struct gl_texture_format *
fxDDChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
                         GLenum srcFormat, GLenum srcType )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLboolean allow32bpt = fxMesa->HaveTexFmt;

   /* [dBorca] Hack alert:
    * There is something wrong with this!!! Take an example:
    * 1) start HW rendering
    * 2) create a texture like this:
    *    glTexImage2D(GL_TEXTURE_2D, 0, 3, 16, 16, 0,
    *                 GL_RGB, GL_UNSIGNED_BYTE, floorTexture);
    *    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    * 3) we get here with internalFormat==3 and return either
    *    _mesa_texformat_rgb565 or _mesa_texformat_argb8888
    * 4) at some point, we encounter total rasterization fallback
    * 5) displaying a polygon with the above textures yield garbage on areas
    *    where pixel is larger than a texel, because our already set texel
    *    function doesn't match the real _mesa_texformat_argb888
    */

   if (TDFX_DEBUG & VERBOSE_TEXTURE) {
      fprintf(stderr, "fxDDChooseTextureFormat(...)\n");
   }

   switch (internalFormat) {
   case GL_COMPRESSED_RGB:
#if 0 && FX_TC_NAPALM /* [koolsmoky] */
     if (ctx->Extensions.TDFX_texture_compression_FXT1) {
       return &_mesa_texformat_rgb_fxt1;
     } else if (ctx->Extensions.EXT_texture_compression_s3tc) {
       return &_mesa_texformat_rgb_dxt1;
     }
#endif
     /* intentional fall through */
   case 3:
   case GL_RGB:
     if ( srcFormat == GL_RGB && srcType == GL_UNSIGNED_SHORT_5_6_5 ) {
       return &_mesa_texformat_rgb565;
     }
     /* intentional fall through */
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return (allow32bpt) ? &_mesa_texformat_argb8888
                          : &_mesa_texformat_rgb565;
   case GL_RGBA2:
   case GL_RGBA4:
      return &_mesa_texformat_argb4444;
   case GL_COMPRESSED_RGBA:
#if 0 && FX_TC_NAPALM /* [koolsmoky] */
     if (ctx->Extensions.TDFX_texture_compression_FXT1) {
       return &_mesa_texformat_rgba_fxt1;
     } else if (ctx->Extensions.EXT_texture_compression_s3tc) {
       return &_mesa_texformat_rgba_dxt3;
     }
#endif
     /* intentional fall through */
   case 4:
   case GL_RGBA:
     if ( srcFormat == GL_BGRA ) {
       if ( srcType == GL_UNSIGNED_INT_8_8_8_8_REV ) {
         return &_mesa_texformat_argb8888;
       }
       else if ( srcType == GL_UNSIGNED_SHORT_4_4_4_4_REV ) {
         return &_mesa_texformat_argb4444;
       }
       else if ( srcType == GL_UNSIGNED_SHORT_1_5_5_5_REV ) {
         return &_mesa_texformat_argb1555;
       }
     }
     /* intentional fall through */
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return (allow32bpt) ? &_mesa_texformat_argb8888
                          : &_mesa_texformat_argb4444;
   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      return &_mesa_texformat_i8;
   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      return &_mesa_texformat_l8;
   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      return &_mesa_texformat_a8;
   case GL_COLOR_INDEX:
   case GL_COLOR_INDEX1_EXT:
   case GL_COLOR_INDEX2_EXT:
   case GL_COLOR_INDEX4_EXT:
   case GL_COLOR_INDEX8_EXT:
   case GL_COLOR_INDEX12_EXT:
   case GL_COLOR_INDEX16_EXT:
      return &_mesa_texformat_ci8;
   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      return &_mesa_texformat_al88;
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
      return &_mesa_texformat_rgb565;
   case GL_RGB5_A1:
      return &_mesa_texformat_argb1555;
   /* GL_EXT_texture_compression_s3tc */
   /* GL_S3_s3tc */
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
      return &_mesa_texformat_rgb_dxt1;
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      return &_mesa_texformat_rgba_dxt1;
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
      return &_mesa_texformat_rgba_dxt3;
   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return &_mesa_texformat_rgba_dxt5;
   /* GL_3DFX_texture_compression_FXT1 */
   case GL_COMPRESSED_RGB_FXT1_3DFX:
      return &_mesa_texformat_rgb_fxt1;
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      return &_mesa_texformat_rgba_fxt1;
   default:
      _mesa_problem(NULL, "unexpected format in fxDDChooseTextureFormat");
      return NULL;
   }
}


static GrTextureFormat_t
fxGlideFormat(GLint mesaFormat)
{
   switch (mesaFormat) {
   case MESA_FORMAT_I8:
      return GR_TEXFMT_ALPHA_8;
   case MESA_FORMAT_A8:
      return GR_TEXFMT_ALPHA_8;
   case MESA_FORMAT_L8:
      return GR_TEXFMT_INTENSITY_8;
   case MESA_FORMAT_CI8:
      return GR_TEXFMT_P_8;
   case MESA_FORMAT_AL88:
      return GR_TEXFMT_ALPHA_INTENSITY_88;
   case MESA_FORMAT_RGB565:
      return GR_TEXFMT_RGB_565;
   case MESA_FORMAT_ARGB4444:
      return GR_TEXFMT_ARGB_4444;
   case MESA_FORMAT_ARGB1555:
      return GR_TEXFMT_ARGB_1555;
   case MESA_FORMAT_ARGB8888:
      return GR_TEXFMT_ARGB_8888;
   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
     return GR_TEXFMT_ARGB_CMP_FXT1;
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_RGBA_DXT1:
     return GR_TEXFMT_ARGB_CMP_DXT1;
   case MESA_FORMAT_RGBA_DXT3:
     return GR_TEXFMT_ARGB_CMP_DXT3;
   case MESA_FORMAT_RGBA_DXT5:
     return GR_TEXFMT_ARGB_CMP_DXT5;
   default:
      _mesa_problem(NULL, "Unexpected format in fxGlideFormat");
      return 0;
   }
}


static FetchTexelFunc
fxFetchFunction(GLint mesaFormat)
{
   switch (mesaFormat) {
   case MESA_FORMAT_I8:
      return &fetch_intensity8;
   case MESA_FORMAT_A8:
      return &fetch_alpha8;
   case MESA_FORMAT_L8:
      return &fetch_luminance8;
   case MESA_FORMAT_CI8:
      return &fetch_index8;
   case MESA_FORMAT_AL88:
      return &fetch_luminance8_alpha8;
   case MESA_FORMAT_RGB565:
      return &fetch_r5g6b5;
   case MESA_FORMAT_ARGB4444:
      return &fetch_r4g4b4a4;
   case MESA_FORMAT_ARGB1555:
      return &fetch_r5g5b5a1;
   case MESA_FORMAT_ARGB8888:
      return &fetch_a8r8g8b8;
   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_RGBA_DXT3:
   case MESA_FORMAT_RGBA_DXT5:
     return &fetch_r4g4b4a4;
   default:
      _mesa_problem(NULL, "Unexpected format in fxFetchFunction");
      return NULL;
   }
}

void
fxDDTexImage2D(GLcontext * ctx, GLenum target, GLint level,
	       GLint internalFormat, GLint width, GLint height, GLint border,
	       GLenum format, GLenum type, const GLvoid * pixels,
	       const struct gl_pixelstore_attrib *packing,
	       struct gl_texture_object *texObj,
	       struct gl_texture_image *texImage)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxTexInfo *ti;
   tfxMipMapLevel *mml;
   GLint texelBytes;

   GLvoid *_final_texImage_Data;
   const struct gl_texture_format *_final_texImage_TexFormat;

   if (TDFX_DEBUG & VERBOSE_TEXTURE) {
       fprintf(stderr, "fxDDTexImage2D: id=%d int 0x%x  format 0x%x  type 0x%x  %dx%d\n",
                       texObj->Name, texImage->IntFormat, format, type,
                       texImage->Width, texImage->Height);
   }

   if (!fxIsTexSupported(target, internalFormat, texImage)) {
      _mesa_problem(NULL, "fx Driver: unsupported texture in fxDDTexImg()\n");
      return;
   }

   if (!texObj->DriverData) {
      texObj->DriverData = fxAllocTexObjData(fxMesa);
      if (!texObj->DriverData) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   }
   ti = fxTMGetTexInfo(texObj);

   if (!texImage->DriverData) {
      texImage->DriverData = CALLOC(sizeof(tfxMipMapLevel));
      if (!texImage->DriverData) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   }
   mml = FX_MIPMAP_DATA(texImage);

   fxTexGetInfo(width, height, NULL, NULL, NULL, NULL,
		&mml->wScale, &mml->hScale);

   mml->width = width * mml->wScale;
   mml->height = height * mml->hScale;

#if 0 && FX_COMPRESS_S3TC_AS_FXT1_HACK
   /* [koolsmoky] substitute FXT1 for DXTn and Legacy S3TC */
   /* [dBorca] we should update texture's attribute, then,
    * because if the application asks us to decompress, we
    * have to know the REAL format! Also, DXT3/5 might not
    * be correct, since it would mess with "compressedSize".
    * Ditto for GL_RGBA[4]_S3TC, which is always mapped to DXT3.
    */
   if (texImage->IsCompressed) {
     switch (internalFormat) {
     case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
     case GL_RGB_S3TC:
     case GL_RGB4_S3TC:
       internalFormat = GL_COMPRESSED_RGB_FXT1_3DFX;
       break;
     case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
     case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
     case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
     case GL_RGBA_S3TC:
     case GL_RGBA4_S3TC:
       internalFormat = GL_COMPRESSED_RGBA_FXT1_3DFX;
     }
   }
#endif
#if 1 || FX_COMPRESS_DXT5_AS_DXT3_HACK
   /* [dBorca] either VSA is stupid at DXT5, 
    * or our compression tool is broken. See
    * above for caveats.
    */
   if ((texImage->IsCompressed) &&
       (internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)) {
       internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
   }
#endif

   /* choose the texture format */
   assert(ctx->Driver.ChooseTextureFormat);
   texImage->TexFormat = (*ctx->Driver.ChooseTextureFormat)(ctx,
                                          internalFormat, format, type);
   assert(texImage->TexFormat);
   texelBytes = texImage->TexFormat->TexelBytes;
   /*if (!fxMesa->HaveTexFmt) assert(texelBytes == 1 || texelBytes == 2);*/

   mml->glideFormat = fxGlideFormat(texImage->TexFormat->MesaFormat);
   /* dirty trick: will thrash CopyTex[Sub]Image */
#if FX_TC_NCC || FX_TC_NAPALM
   if (internalFormat == GL_COMPRESSED_RGB) {
#if FX_TC_NCC
      mml->glideFormat = GR_TEXFMT_YIQ_422;
#endif
#if FX_TC_NAPALM
      if (fxMesa->type >= GR_SSTTYPE_Voodoo4) {
         mml->glideFormat = GR_TEXFMT_ARGB_CMP_FXT1;
      }
#endif
   } else if (internalFormat == GL_COMPRESSED_RGBA) {
#if FX_TC_NCC
      mml->glideFormat = GR_TEXFMT_AYIQ_8422;
#endif
#if FX_TC_NAPALM
      if (fxMesa->type >= GR_SSTTYPE_Voodoo4) {
         mml->glideFormat = GR_TEXFMT_ARGB_CMP_FXT1;
      }
#endif
   }
#endif

   /* allocate mipmap buffer */
   assert(!texImage->Data);
   if (texImage->IsCompressed) {
      texImage->Data = MESA_PBUFFER_ALLOC(texImage->CompressedSize);
      texelBytes = 4;
      _final_texImage_TexFormat = &_mesa_texformat_argb8888;
      _final_texImage_Data = MALLOC(mml->width * mml->height * 4);
      if (!_final_texImage_Data) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   } else {
      texImage->Data = MESA_PBUFFER_ALLOC(mml->width * mml->height * texelBytes);
      _final_texImage_TexFormat = texImage->TexFormat;
      _final_texImage_Data = texImage->Data;
   }
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
      return;
   }

   if (mml->wScale != 1 || mml->hScale != 1) {
      /* rescale image to overcome 1:8 aspect limitation */
      GLvoid *tempImage;
      tempImage = MALLOC(width * height * texelBytes);
      if (!tempImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
      /* unpack image, apply transfer ops and store in tempImage */
      _mesa_transfer_teximage(ctx, 2, texImage->Format,
                              _final_texImage_TexFormat,
                              tempImage,
                              width, height, 1, 0, 0, 0, /* src */
                              width * texelBytes, /* dstRowStride */
                              0, /* dstImageStride */
                              format, type, pixels, packing);
      _mesa_rescale_teximage2d(texelBytes,
                               mml->width * texelBytes, /* dst stride */
                               width, height, /* src */
                               mml->width, mml->height, /* dst */
                               tempImage /*src*/, _final_texImage_Data /*dst*/ );
      FREE(tempImage);
   }
   else {
      /* no rescaling needed */
      /* unpack image, apply transfer ops and store in texImage->Data */
      _mesa_transfer_teximage(ctx, 2, texImage->Format,
                              _final_texImage_TexFormat, _final_texImage_Data,
                              width, height, 1, 0, 0, 0,
                              mml->width * texelBytes,
                              0, /* dstImageStride */
                              format, type, pixels, packing);
   }

   /* now compress */
   if (texImage->IsCompressed) {
#if FX_TC_NCC
      if ((mml->glideFormat == GR_TEXFMT_AYIQ_8422) ||
          (mml->glideFormat == GR_TEXFMT_YIQ_422)) {
         TxMip txMip, pxMip;
         txMip.width = mml->width;
         txMip.height = mml->height;
         txMip.depth = 1;
         txMip.data[0] = _final_texImage_Data;
         pxMip.data[0] = texImage->Data;
         fxMesa->Glide.txMipQuantize(&pxMip, &txMip, mml->glideFormat, TX_DITHER_ERR, TX_COMPRESSION_STATISTICAL);
         fxMesa->Glide.txPalToNcc((GuNccTable *)(&(ti->palette)), pxMip.pal);
         MEMCPY((char *)texImage->Data + texImage->CompressedSize - 12 * 4, &(ti->palette.data[16]), 12 * 4);
      } else
#endif
      fxMesa->Glide.txImgQuantize(texImage->Data, _final_texImage_Data, mml->width, mml->height, mml->glideFormat, TX_DITHER_NONE);
      FREE(_final_texImage_Data);
   }

   ti->info.format = mml->glideFormat;
   texImage->FetchTexel = fxFetchFunction(texImage->TexFormat->MesaFormat);

   /* [dBorca]
    * Hack alert: unsure...
    */
   if (0 && ti->validated && ti->isInTM) {
      /*fprintf(stderr, "reloadmipmaplevels\n"); */
      fxTMReloadMipMapLevel(fxMesa, texObj, level);
   }
   else {
      /*fprintf(stderr, "invalidate2\n"); */
      fxTexInvalidate(ctx, texObj);
   }
}


void
fxDDTexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
		  GLint xoffset, GLint yoffset,
		  GLsizei width, GLsizei height,
		  GLenum format, GLenum type, const GLvoid * pixels,
		  const struct gl_pixelstore_attrib *packing,
		  struct gl_texture_object *texObj,
		  struct gl_texture_image *texImage)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxTexInfo *ti;
   tfxMipMapLevel *mml;
   GLint texelBytes;

   /* [dBorca] Hack alert:
    * fix the goddamn texture compression here
    */

   if (TDFX_DEBUG & VERBOSE_TEXTURE) {
       fprintf(stderr, "fxDDTexSubImage2D: id=%d\n", texObj->Name);
   }

   if (!texObj->DriverData) {
      _mesa_problem(ctx, "problem in fxDDTexSubImage2D");
      return;
   }

   ti = fxTMGetTexInfo(texObj);
   assert(ti);
   mml = FX_MIPMAP_DATA(texImage);
   assert(mml);

   assert(texImage->Data);	/* must have an existing texture image! */
   assert(texImage->Format);

   texelBytes = texImage->TexFormat->TexelBytes;

   if (mml->wScale != 1 || mml->hScale != 1) {
      /* need to rescale subimage to match mipmap level's rescale factors */
      const GLint newWidth = width * mml->wScale;
      const GLint newHeight = height * mml->hScale;
      GLvoid *tempImage;
      GLubyte *destAddr;
      tempImage = MALLOC(width * height * texelBytes);
      if (!tempImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage2D");
         return;
      }

      _mesa_transfer_teximage(ctx, 2, texImage->Format,/* Tex int format */
                              texImage->TexFormat,     /* dest format */
                              (GLubyte *) tempImage,   /* dest */
                              width, height, 1,        /* subimage size */
                              0, 0, 0,                 /* subimage pos */
                              width * texelBytes,      /* dest row stride */
                              0,                       /* dst image stride */
                              format, type, pixels, packing);

      /* now rescale */
      /* compute address of dest subimage within the overal tex image */
      destAddr = (GLubyte *) texImage->Data
         + (yoffset * mml->hScale * mml->width
            + xoffset * mml->wScale) * texelBytes;

      _mesa_rescale_teximage2d(texelBytes,
                               mml->width * texelBytes, /* dst stride */
                               width, height,
                               newWidth, newHeight,
                               tempImage, destAddr);

      FREE(tempImage);
   }
   else {
      /* no rescaling needed */
      _mesa_transfer_teximage(ctx, 2, texImage->Format,  /* Tex int format */
                              texImage->TexFormat,       /* dest format */
                              (GLubyte *) texImage->Data,/* dest */
                              width, height, 1,          /* subimage size */
                              xoffset, yoffset, 0,       /* subimage pos */
                              mml->width * texelBytes,   /* dest row stride */
                              0,                         /* dst image stride */
                              format, type, pixels, packing);
   }

   /* [dBorca]
    * Hack alert: unsure...
    */
   if (0 && ti->validated && ti->isInTM)
      fxTMReloadMipMapLevel(fxMesa, texObj, level);
   else
      fxTexInvalidate(ctx, texObj);
}


void
fxDDCompressedTexImage2D (GLcontext *ctx, GLenum target,
                          GLint level, GLint internalFormat,
                          GLsizei width, GLsizei height, GLint border,
                          GLsizei imageSize, const GLvoid *data,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxTexInfo *ti;
   tfxMipMapLevel *mml;

   if (TDFX_DEBUG & VERBOSE_TEXTURE) {
       fprintf(stderr, "fxDDCompressedTexImage2D: id=%d int 0x%x  %dx%d\n",
                       texObj->Name, internalFormat,
                       width, height);
   }

   assert(texImage->IsCompressed);

   if (!fxIsTexSupported(target, internalFormat, texImage)) {
      _mesa_problem(NULL, "fx Driver: unsupported texture in fxDDCompressedTexImg()\n");
      return;
   }

   if (!texObj->DriverData) {
      texObj->DriverData = fxAllocTexObjData(fxMesa);
      if (!texObj->DriverData) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2D");
         return;
      }
   }
   ti = fxTMGetTexInfo(texObj);

   if (!texImage->DriverData) {
      texImage->DriverData = CALLOC(sizeof(tfxMipMapLevel));
      if (!texImage->DriverData) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   }
   mml = FX_MIPMAP_DATA(texImage);

   fxTexGetInfo(width, height, NULL, NULL, NULL, NULL,
		&mml->wScale, &mml->hScale);

   mml->width = width * mml->wScale;
   mml->height = height * mml->hScale;


   /* choose the texture format */
   assert(ctx->Driver.ChooseTextureFormat);
   texImage->TexFormat = (*ctx->Driver.ChooseTextureFormat)(ctx,
                                          internalFormat, -1/*format*/, -1/*type*/);
   assert(texImage->TexFormat);

   /* Determine the appropriate Glide texel format,
    * given the user's internal texture format hint.
    */
   mml->glideFormat = fxGlideFormat(texImage->TexFormat->MesaFormat);
#if FX_TC_NCC || FX_TC_NAPALM
   if (internalFormat == GL_COMPRESSED_RGB) {
#if FX_TC_NCC
      mml->glideFormat = GR_TEXFMT_YIQ_422;
#endif
#if FX_TC_NAPALM
      if (fxMesa->type >= GR_SSTTYPE_Voodoo4) {
         mml->glideFormat = GR_TEXFMT_ARGB_CMP_FXT1;
      }
#endif
   } else if (internalFormat == GL_COMPRESSED_RGBA) {
#if FX_TC_NCC
      mml->glideFormat = GR_TEXFMT_AYIQ_8422;
#endif
#if FX_TC_NAPALM
      if (fxMesa->type >= GR_SSTTYPE_Voodoo4) {
         mml->glideFormat = GR_TEXFMT_ARGB_CMP_FXT1;
      }
#endif
   }
#endif

   /* allocate new storage for texture image, if needed */
   if (!texImage->Data) {
      texImage->Data = MESA_PBUFFER_ALLOC(imageSize);
      if (!texImage->Data) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   }

   /* save the texture data */
   MEMCPY(texImage->Data, data, imageSize);
#if FX_TC_NCC
   if ((mml->glideFormat == GR_TEXFMT_AYIQ_8422) ||
       (mml->glideFormat == GR_TEXFMT_YIQ_422)) {
      MEMCPY(&(ti->palette.data[16]), (char *)data + imageSize - 12 * 4, 12 * 4);
   }
#endif

   ti->info.format = mml->glideFormat;
   texImage->FetchTexel = fxFetchFunction(texImage->TexFormat->MesaFormat);

   /* [dBorca] Hack alert:
    * what about different size/texel? other anomalies? SW rescaling?
    */

   /* [dBorca]
    * Hack alert: unsure...
    */
   if (0 && ti->validated && ti->isInTM) {
      /*fprintf(stderr, "reloadmipmaplevels\n"); */
      fxTMReloadMipMapLevel(fxMesa, texObj, level);
   }
   else {
      /*fprintf(stderr, "invalidate2\n"); */
      fxTexInvalidate(ctx, texObj);
   }
}


void
fxDDCompressedTexSubImage2D( GLcontext *ctx, GLenum target,
                             GLint level, GLint xoffset,
                             GLint yoffset, GLsizei width,
                             GLint height, GLenum format,
                             GLsizei imageSize, const GLvoid *data,
                             struct gl_texture_object *texObj,
                             struct gl_texture_image *texImage )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxTexInfo *ti;
   tfxMipMapLevel *mml;

   if (TDFX_DEBUG & VERBOSE_TEXTURE) {
       fprintf(stderr, "fxDDCompressedTexSubImage2D: id=%d\n", texObj->Name);
   }

   ti = fxTMGetTexInfo(texObj);
   assert(ti);
   mml = FX_MIPMAP_DATA(texImage);
   assert(mml);

   /*
    * We punt if we are not replacing the entire image.  This
    * is allowed by the spec.
    *
    * [dBorca] Hack alert:
    * ohwell, we should NOT! Look into _mesa_store_compressed_texsubimage2d
    * on how to calculate the sub-image.
    */
   if ((xoffset != 0) && (yoffset != 0)
       && (width != texImage->Width)
       && (height != texImage->Height)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCompressedTexSubImage2D(CHICKEN)");
      return;
   }

   /* [dBorca] Hack alert:
    * what about different size/texel? other anomalies? SW rescaling?
    */
   MEMCPY(texImage->Data, data, imageSize);

   /* [dBorca]
    * Hack alert: unsure...
    */
   if (0 && ti->validated && ti->isInTM)
      fxTMReloadMipMapLevel(fxMesa, texObj, level);
   else
      fxTexInvalidate(ctx, texObj);
}


#else /* FX */

/*
 * Need this to provide at least one external definition.
 */

extern int gl_fx_dummy_function_ddtex(void);
int
gl_fx_dummy_function_ddtex(void)
{
   return 0;
}

#endif /* FX */
