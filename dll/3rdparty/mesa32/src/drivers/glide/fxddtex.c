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
#include "texcompress.h"
#include "texobj.h"
#include "texstore.h"


/* no borders! can't halve 1x1! (stride > width * comp) not allowed */
static void
_mesa_halve2x2_teximage2d ( GLcontext *ctx,
			    struct gl_texture_image *texImage,
			    GLuint bytesPerPixel,
			    GLint srcWidth, GLint srcHeight,
			    const GLvoid *srcImage, GLvoid *dstImage )
{
   GLint i, j, k;
   GLint dstWidth = srcWidth / 2;
   GLint dstHeight = srcHeight / 2;
   GLint srcRowStride = srcWidth * bytesPerPixel;
   GLubyte *src = (GLubyte *)srcImage;
   GLubyte *dst = dstImage;

   GLuint bpt = 0;
   GLubyte *_s = NULL;
   GLubyte *_d = NULL;
   GLenum _t = 0;

   if (texImage->TexFormat->MesaFormat == MESA_FORMAT_RGB565) {
      _t = GL_UNSIGNED_SHORT_5_6_5_REV;
      bpt = bytesPerPixel;
   } else if (texImage->TexFormat->MesaFormat == MESA_FORMAT_ARGB4444) {
      _t = GL_UNSIGNED_SHORT_4_4_4_4_REV;
      bpt = bytesPerPixel;
   } else if (texImage->TexFormat->MesaFormat == MESA_FORMAT_ARGB1555) {
      _t = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      bpt = bytesPerPixel;
   }
   if (bpt) {
      bytesPerPixel = 4;
      srcRowStride = srcWidth * bytesPerPixel;
      if (dstWidth == 0) {
         dstWidth = 1;
      }
      if (dstHeight == 0) {
         dstHeight = 1;
      }
      _s = src = MALLOC(srcRowStride * srcHeight);
      _d = dst = MALLOC(dstWidth * bytesPerPixel * dstHeight);
      _mesa_texstore_rgba8888(ctx, 2, GL_RGBA,
                              &_mesa_texformat_rgba8888_rev, src,
                              0, 0, 0, /* dstX/Y/Zoffset */
                              srcRowStride, /* dstRowStride */
                              0, /* dstImageStride */
                              srcWidth, srcHeight, 1,
                              texImage->_BaseFormat, _t,
                              srcImage, &ctx->DefaultPacking);
   }

   if (srcHeight == 1) {
      for (i = 0; i < dstWidth; i++) {
         for (k = 0; k < bytesPerPixel; k++) {
            dst[0] = (src[0] + src[bytesPerPixel] + 1) / 2;
            src++;
            dst++;
         }
         src += bytesPerPixel;
      }
   } else if (srcWidth == 1) {
      for (j = 0; j < dstHeight; j++) {
         for (k = 0; k < bytesPerPixel; k++) {
            dst[0] = (src[0] + src[srcRowStride] + 1) / 2;
            src++;
            dst++;
         }
         src += srcRowStride;
      }
   } else {
      for (j = 0; j < dstHeight; j++) {
         for (i = 0; i < dstWidth; i++) {
            for (k = 0; k < bytesPerPixel; k++) {
               dst[0] = (src[0] +
                         src[bytesPerPixel] +
                         src[srcRowStride] +
                         src[srcRowStride + bytesPerPixel] + 2) / 4;
               src++;
               dst++;
            }
            src += bytesPerPixel;
         }
         src += srcRowStride;
      }
   }

   if (bpt) {
      src = _s;
      dst = _d;
      texImage->TexFormat->StoreImage(ctx, 2, texImage->_BaseFormat,
                                      texImage->TexFormat, dstImage,
                                      0, 0, 0, /* dstX/Y/Zoffset */
                                      dstWidth * bpt,
                                      0, /* dstImageStride */
                                      dstWidth, dstHeight, 1,
                                      GL_BGRA, CHAN_TYPE, dst, &ctx->DefaultPacking);
      FREE(dst);
      FREE(src);
   }
}


void
fxPrintTextureData(tfxTexInfo * ti)
{
   fprintf(stderr, "Texture Data:\n");
   if (ti->tObj) {
      fprintf(stderr, "\tName: %d\n", ti->tObj->Name);
      fprintf(stderr, "\tBaseLevel: %d\n", ti->tObj->BaseLevel);
      fprintf(stderr, "\tSize: %d x %d\n",
	      ti->tObj->Image[0][ti->tObj->BaseLevel]->Width,
	      ti->tObj->Image[0][ti->tObj->BaseLevel]->Height);
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

   if ((target != GL_TEXTURE_1D) && (target != GL_TEXTURE_2D))
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
      GLfloat bias = *param;
      CLAMP_SELF(bias, -ctx->Const.MaxTextureLodBias,
                        ctx->Const.MaxTextureLodBias - 0.25);

      grTexLodBiasValue(GR_TMU0, bias);

      if (fxMesa->haveTwoTMUs) {
	 grTexLodBiasValue(GR_TMU1, bias);
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

   if ((target != GL_TEXTURE_1D) && (target != GL_TEXTURE_2D))
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
         /* [dBorca]
          * currently Napalm can't do single-pass trilinear,
          * because the way its combiners are set. So we fall back
          * to GL_NEAREST_MIPMAP_NEAREST. We'll let true trilinear
          * enabled for V2, V3.
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
         /* [dBorca]
          * currently Napalm can't do single-pass trilinear,
          * because the way its combiners are set. So we fall back
          * to GL_LINEAR_MIPMAP_NEAREST. We'll let true trilinear
          * enabled for V2, V3.
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
      fxMesa->new_state |= FX_NEW_TEXTURING;
      break;

   case GL_TEXTURE_WRAP_S:
      switch (param) {
      case GL_MIRRORED_REPEAT:
         ti->sClamp = GR_TEXTURECLAMP_MIRROR_EXT;
         break;
      case GL_CLAMP_TO_BORDER: /* no-no, but don't REPEAT, either */
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
      case GL_CLAMP_TO_BORDER: /* no-no, but don't REPEAT, either */
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


/**
 * Allocate a new texture object.
 * Called via ctx->Driver.NewTextureObject.
 * Note: this function will be called during context creation to
 * allocate the default texture objects.
 */
struct gl_texture_object *
fxDDNewTextureObject( GLcontext *ctx, GLuint name, GLenum target )
{
   struct gl_texture_object *obj;
   obj = _mesa_new_texture_object(ctx, name, target);
   return obj;
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

   ASSERT(table->Type == GL_UNSIGNED_BYTE);

   switch (table->_BaseFormat) {
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
      /* This might be a proxy texture. */
      if (!tObj->Palette.Table)
         return;
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

   fxMesa->haveGlobalPaletteTexture = state;
   fxMesa->new_state |= FX_NEW_TEXTURING;
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
   if ((target != GL_TEXTURE_1D) && (target != GL_TEXTURE_2D))
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
extern void
fxt1_decode_1 (const void *texture, int width,
               int i, int j, unsigned char *rgba);

/* Texel-fetch functions for software texturing and glGetTexImage().
 * We should have been able to use some "standard" fetch functions (which
 * may get defined in texutil.c) but we have to account for scaled texture
 * images on tdfx hardware (the 8:1 aspect ratio limit).
 * Hence, we need special functions here.
 */

static void
fetch_intensity8(const struct gl_texture_image *texImage,
		 GLint i, GLint j, GLint k, GLchan *rgba)
{
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
		 GLint i, GLint j, GLint k, GLchan *rgba)
{
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
	     GLint i, GLint j, GLint k, GLchan *rgba)
{
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
	     GLint i, GLint j, GLint k, GLchan *indexOut)
{
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLubyte *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLubyte *) texImage->Data) + j * mml->width + i;
   *indexOut = *texel;
}


static void
fetch_luminance8_alpha8(const struct gl_texture_image *texImage,
			GLint i, GLint j, GLint k, GLchan *rgba)
{
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
	     GLint i, GLint j, GLint k, GLchan *rgba)
{
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
	       GLint i, GLint j, GLint k, GLchan *rgba)
{
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLushort *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLushort *) texImage->Data) + j * mml->width + i;
   rgba[RCOMP] = FX_rgb_scale_4[(*texel >>  8) & 0xF];
   rgba[GCOMP] = FX_rgb_scale_4[(*texel >>  4) & 0xF];
   rgba[BCOMP] = FX_rgb_scale_4[ *texel        & 0xF];
   rgba[ACOMP] = FX_rgb_scale_4[(*texel >> 12) & 0xF];
}


static void
fetch_r5g5b5a1(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLchan *rgba)
{
   const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
   const GLushort *texel;

   i = i * mml->wScale;
   j = j * mml->hScale;

   texel = ((GLushort *) texImage->Data) + j * mml->width + i;
   rgba[RCOMP] = FX_rgb_scale_5[(*texel >> 10) & 0x1F];
   rgba[GCOMP] = FX_rgb_scale_5[(*texel >>  5) & 0x1F];
   rgba[BCOMP] = FX_rgb_scale_5[ *texel        & 0x1F];
   rgba[ACOMP] = (*texel >> 15) * 255;
}


static void
fetch_a8r8g8b8(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLchan *rgba)
{
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
fetch_rgb_fxt1(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLchan *rgba)
{
    const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);

    i = i * mml->wScale;
    j = j * mml->hScale;

    fxt1_decode_1(texImage->Data, mml->width, i, j, rgba);
    rgba[ACOMP] = 255;
}


static void
fetch_rgba_fxt1(const struct gl_texture_image *texImage,
		GLint i, GLint j, GLint k, GLchan *rgba)
{
    const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);

    i = i * mml->wScale;
    j = j * mml->hScale;

    fxt1_decode_1(texImage->Data, mml->width, i, j, rgba);
}


static void
fetch_rgb_dxt1(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLchan *rgba)
{
    const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);

    i = i * mml->wScale;
    j = j * mml->hScale;

    _mesa_texformat_rgb_dxt1.FetchTexel2D(texImage, i, j, k, rgba);
}


static void
fetch_rgba_dxt1(const struct gl_texture_image *texImage,
		GLint i, GLint j, GLint k, GLchan *rgba)
{
    const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);

    i = i * mml->wScale;
    j = j * mml->hScale;

    _mesa_texformat_rgba_dxt1.FetchTexel2D(texImage, i, j, k, rgba);
}


static void
fetch_rgba_dxt3(const struct gl_texture_image *texImage,
		GLint i, GLint j, GLint k, GLchan *rgba)
{
    const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);

    i = i * mml->wScale;
    j = j * mml->hScale;

    _mesa_texformat_rgba_dxt3.FetchTexel2D(texImage, i, j, k, rgba);
}


static void
fetch_rgba_dxt5(const struct gl_texture_image *texImage,
		GLint i, GLint j, GLint k, GLchan *rgba)
{
    const tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);

    i = i * mml->wScale;
    j = j * mml->hScale;

    _mesa_texformat_rgba_dxt5.FetchTexel2D(texImage, i, j, k, rgba);
}


#if 0 /* break glass in case of emergency */
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
#endif


const struct gl_texture_format *
fxDDChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
                         GLenum srcFormat, GLenum srcType )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLboolean allow32bpt = fxMesa->HaveTexFmt;

   if (TDFX_DEBUG & VERBOSE_TEXTURE) {
      fprintf(stderr, "fxDDChooseTextureFormat(...)\n");
   }

   switch (internalFormat) {
   case GL_COMPRESSED_RGB:
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


static FetchTexelFuncC
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
      return &fetch_rgb_fxt1;
   case MESA_FORMAT_RGBA_FXT1:
      return &fetch_rgba_fxt1;
   case MESA_FORMAT_RGB_DXT1:
      return &fetch_rgb_dxt1;
   case MESA_FORMAT_RGBA_DXT1:
      return &fetch_rgba_dxt1;
   case MESA_FORMAT_RGBA_DXT3:
      return &fetch_rgba_dxt3;
   case MESA_FORMAT_RGBA_DXT5:
      return &fetch_rgba_dxt5;
   default:
      _mesa_problem(NULL, "Unexpected format in fxFetchFunction");
      return NULL;
   }
}


static GLboolean
adjust2DRatio (GLcontext *ctx,
	       GLint xoffset, GLint yoffset,
	       GLint width, GLint height,
	       GLenum format, GLenum type, const GLvoid *pixels,
	       const struct gl_pixelstore_attrib *packing,
	       tfxMipMapLevel *mml,
	       struct gl_texture_image *texImage,
	       GLint texelBytes,
	       GLint dstRowStride)
{
   const GLint newWidth = width * mml->wScale;
   const GLint newHeight = height * mml->hScale;
   GLvoid *tempImage;

   if (!texImage->IsCompressed) {
      GLubyte *destAddr;
      tempImage = MALLOC(width * height * texelBytes);
      if (!tempImage) {
         return GL_FALSE;
      }

      texImage->TexFormat->StoreImage(ctx, 2, texImage->_BaseFormat,
                                      texImage->TexFormat, tempImage,
                                      0, 0, 0, /* dstX/Y/Zoffset */
                                      width * texelBytes, /* dstRowStride */
                                      0, /* dstImageStride */
                                      width, height, 1,
                                      format, type, pixels, packing);

      /* now rescale */
      /* compute address of dest subimage within the overal tex image */
      destAddr = (GLubyte *) texImage->Data
         + (yoffset * mml->hScale * mml->width
            + xoffset * mml->wScale) * texelBytes;

      _mesa_rescale_teximage2d(texelBytes,
                               width,
                               dstRowStride, /* dst stride */
                               width, height,
                               newWidth, newHeight,
                               tempImage, destAddr);
   } else {
      const GLint rawBytes = 4;
      GLvoid *rawImage = MALLOC(width * height * rawBytes);
      if (!rawImage) {
         return GL_FALSE;
      }
      tempImage = MALLOC(newWidth * newHeight * rawBytes);
      if (!tempImage) {
         return GL_FALSE;
      }
      /* unpack image, apply transfer ops and store in rawImage */
      _mesa_texstore_rgba8888(ctx, 2, GL_RGBA,
                              &_mesa_texformat_rgba8888_rev, rawImage,
                              0, 0, 0, /* dstX/Y/Zoffset */
                              width * rawBytes, /* dstRowStride */
                              0, /* dstImageStride */
                              width, height, 1,
                              format, type, pixels, packing);
      _mesa_rescale_teximage2d(rawBytes,
                               width,
                               newWidth * rawBytes, /* dst stride */
                               width, height, /* src */
                               newWidth, newHeight, /* dst */
                               rawImage /*src*/, tempImage /*dst*/ );
      texImage->TexFormat->StoreImage(ctx, 2, texImage->_BaseFormat,
                                      texImage->TexFormat, texImage->Data,
                                      xoffset * mml->wScale, yoffset * mml->hScale, 0, /* dstX/Y/Zoffset */
                                      dstRowStride,
                                      0, /* dstImageStride */
                                      newWidth, newHeight, 1,
                                      GL_RGBA, CHAN_TYPE, tempImage, &ctx->DefaultPacking);
      FREE(rawImage);
   }

   FREE(tempImage);

   return GL_TRUE;
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
   GLint texelBytes, dstRowStride;

   if (TDFX_DEBUG & VERBOSE_TEXTURE) {
       fprintf(stderr, "fxDDTexImage2D: id=%d int 0x%x  format 0x%x  type 0x%x  %dx%d\n",
                       texObj->Name, texImage->InternalFormat, format, type,
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

#if FX_COMPRESS_S3TC_AS_FXT1_HACK
   /* [koolsmoky] substitute FXT1 for DXTn and Legacy S3TC */
   if (!ctx->Mesa_DXTn && texImage->IsCompressed) {
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
     texImage->InternalFormat = internalFormat;
   }
#endif
#if FX_TC_NAPALM
   if (fxMesa->type >= GR_SSTTYPE_Voodoo4) {
      GLenum texNapalm = 0;
      if (internalFormat == GL_COMPRESSED_RGB) {
         texNapalm = GL_COMPRESSED_RGB_FXT1_3DFX;
      } else if (internalFormat == GL_COMPRESSED_RGBA) {
         texNapalm = GL_COMPRESSED_RGBA_FXT1_3DFX;
      }
      if (texNapalm) {
         texImage->InternalFormat = internalFormat = texNapalm;
         texImage->IsCompressed = GL_TRUE;
      }
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

   /* allocate mipmap buffer */
   assert(!texImage->Data);
   if (texImage->IsCompressed) {
      texImage->CompressedSize = _mesa_compressed_texture_size(ctx,
                                                               mml->width,
                                                               mml->height,
                                                               1,
                                                               internalFormat);
      dstRowStride = _mesa_compressed_row_stride(internalFormat, mml->width);
      texImage->Data = _mesa_malloc(texImage->CompressedSize);
   } else {
      dstRowStride = mml->width * texelBytes;
      texImage->Data = _mesa_malloc(mml->width * mml->height * texelBytes);
   }
   if (!texImage->Data) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
      return;
   }

   if (pixels != NULL) {
      if (mml->wScale != 1 || mml->hScale != 1) {
         /* rescale image to overcome 1:8 aspect limitation */
         if (!adjust2DRatio(ctx,
			    0, 0,
			    width, height,
			    format, type, pixels,
			    packing,
			    mml,
			    texImage,
			    texelBytes,
			    dstRowStride)
            ) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
            return;
         }
      }
      else {
         /* no rescaling needed */
         /* unpack image, apply transfer ops and store in texImage->Data */
         texImage->TexFormat->StoreImage(ctx, 2, texImage->_BaseFormat,
                                         texImage->TexFormat, texImage->Data,
                                         0, 0, 0, /* dstX/Y/Zoffset */
                                         dstRowStride,
                                         0, /* dstImageStride */
                                         width, height, 1,
                                         format, type, pixels, packing);
      }

      /* GL_SGIS_generate_mipmap */
      if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
         GLint mipWidth, mipHeight;
         tfxMipMapLevel *mip;
         struct gl_texture_image *mipImage;
         const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
         const GLint maxLevels = _mesa_max_texture_levels(ctx, texObj->Target);

         assert(!texImage->IsCompressed);

         while (level < texObj->MaxLevel && level < maxLevels - 1) {
            mipWidth = width / 2;
            if (!mipWidth) {
               mipWidth = 1;
            }
            mipHeight = height / 2;
            if (!mipHeight) {
               mipHeight = 1;
            }
            if ((mipWidth == width) && (mipHeight == height)) {
               break;
            }
            _mesa_TexImage2D(target, ++level, internalFormat,
                             mipWidth, mipHeight, border,
                             format, type,
                             NULL);
            mipImage = _mesa_select_tex_image(ctx, texUnit, target, level);
            mip = FX_MIPMAP_DATA(mipImage);
            _mesa_halve2x2_teximage2d(ctx,
                                      texImage,
                                      texelBytes,
                                      mml->width, mml->height,
                                      texImage->Data, mipImage->Data);
            texImage = mipImage;
            mml = mip;
            width = mipWidth;
            height = mipHeight;
         }
      }
   }

   ti->info.format = mml->glideFormat;
   texImage->FetchTexelc = fxFetchFunction(texImage->TexFormat->MesaFormat);

   fxTexInvalidate(ctx, texObj);
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
   GLint texelBytes, dstRowStride;

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
   assert(texImage->_BaseFormat);

   texelBytes = texImage->TexFormat->TexelBytes;
   if (texImage->IsCompressed) {
      dstRowStride = _mesa_compressed_row_stride(texImage->InternalFormat, mml->width);
   } else {
      dstRowStride = mml->width * texelBytes;
   }

   if (mml->wScale != 1 || mml->hScale != 1) {
      /* need to rescale subimage to match mipmap level's rescale factors */
      if (!adjust2DRatio(ctx,
			 xoffset, yoffset,
			 width, height,
			 format, type, pixels,
			 packing,
			 mml,
			 texImage,
			 texelBytes,
			 dstRowStride)
         ) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexSubImage2D");
         return;
      }
   }
   else {
      /* no rescaling needed */
      texImage->TexFormat->StoreImage(ctx, 2, texImage->_BaseFormat,
                                      texImage->TexFormat, (GLubyte *) texImage->Data,
                                      xoffset, yoffset, 0, /* dstX/Y/Zoffset */
                                      dstRowStride,
                                      0, /* dstImageStride */
                                      width, height, 1,
                                      format, type, pixels, packing);
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      GLint mipWidth, mipHeight;
      tfxMipMapLevel *mip;
      struct gl_texture_image *mipImage;
      const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      const GLint maxLevels = _mesa_max_texture_levels(ctx, texObj->Target);

      assert(!texImage->IsCompressed);

      width = texImage->Width;
      height = texImage->Height;
      while (level < texObj->MaxLevel && level < maxLevels - 1) {
         mipWidth = width / 2;
         if (!mipWidth) {
            mipWidth = 1;
         }
         mipHeight = height / 2;
         if (!mipHeight) {
            mipHeight = 1;
         }
         if ((mipWidth == width) && (mipHeight == height)) {
            break;
         }
         ++level;
         mipImage = _mesa_select_tex_image(ctx, texUnit, target, level);
         mip = FX_MIPMAP_DATA(mipImage);
         _mesa_halve2x2_teximage2d(ctx,
                                   texImage,
                                   texelBytes,
                                   mml->width, mml->height,
                                   texImage->Data, mipImage->Data);
         texImage = mipImage;
         mml = mip;
         width = mipWidth;
         height = mipHeight;
      }
   }

   if (ti->validated && ti->isInTM && !texObj->GenerateMipmap)
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
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2D");
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

   /* allocate new storage for texture image, if needed */
   if (!texImage->Data) {
      texImage->CompressedSize = _mesa_compressed_texture_size(ctx,
                                                               mml->width,
                                                               mml->height,
                                                               1,
                                                               internalFormat);
      texImage->Data = _mesa_malloc(texImage->CompressedSize);
      if (!texImage->Data) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2D");
         return;
      }
   }

   /* save the texture data */
   if (mml->wScale != 1 || mml->hScale != 1) {
      /* [dBorca] Hack alert:
       * now we're screwed. We can't decompress,
       * unless we do it in HW (via textureBuffer).
       * We still have some chances:
       * 1) we got FXT1 textures - we CAN decompress, rescale for
       *    aspectratio, then compress back.
       * 2) there is a chance that MIN("s", "t") won't be overflowed.
       *    Thus, we don't care about textureclamp and we could lower
       *    MIN("uscale", "vscale") below 32. We still have to have
       *    our data aligned inside a 8:1 rectangle.
       * 3) just in case if MIN("s", "t") gets overflowed with GL_REPEAT,
       *    we replicate the data over the padded area.
       * For now, we take 2) + 3) but texelfetchers will be wrong!
       */
      GLuint srcRowStride = _mesa_compressed_row_stride(internalFormat, width);

      GLuint destRowStride = _mesa_compressed_row_stride(internalFormat,
                                                  mml->width);

      _mesa_upscale_teximage2d(srcRowStride, (height+3) / 4,
                               destRowStride, (mml->height+3) / 4,
                               1, data, srcRowStride,
                               texImage->Data);
      ti->padded = GL_TRUE;
   } else {
      MEMCPY(texImage->Data, data, texImage->CompressedSize);
   }

   ti->info.format = mml->glideFormat;
   texImage->FetchTexelc = fxFetchFunction(texImage->TexFormat->MesaFormat);

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      assert(!texImage->IsCompressed);
   }

   fxTexInvalidate(ctx, texObj);
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
   GLint destRowStride, srcRowStride;
   GLint i, rows;
   GLubyte *dest;

   if (TDFX_DEBUG & VERBOSE_TEXTURE) {
       fprintf(stderr, "fxDDCompressedTexSubImage2D: id=%d\n", texObj->Name);
   }

   ti = fxTMGetTexInfo(texObj);
   assert(ti);
   mml = FX_MIPMAP_DATA(texImage);
   assert(mml);

   srcRowStride = _mesa_compressed_row_stride(texImage->InternalFormat, width);

   destRowStride = _mesa_compressed_row_stride(texImage->InternalFormat,
                                               mml->width);
   dest = _mesa_compressed_image_address(xoffset, yoffset, 0,
                                         texImage->InternalFormat,
                                         mml->width,
                              (GLubyte*) texImage->Data);

   rows = height / 4; /* hardcoded 4, but works for FXT1/DXTC */

   for (i = 0; i < rows; i++) {
      MEMCPY(dest, data, srcRowStride);
      dest += destRowStride;
      data = (GLvoid *)((GLuint)data + (GLuint)srcRowStride);
   }

   /* [dBorca] Hack alert:
    * see fxDDCompressedTexImage2D for caveats
    */
   if (mml->wScale != 1 || mml->hScale != 1) {
      srcRowStride = _mesa_compressed_row_stride(texImage->InternalFormat, texImage->Width);

      destRowStride = _mesa_compressed_row_stride(texImage->InternalFormat,
                                               mml->width);
      _mesa_upscale_teximage2d(srcRowStride, texImage->Height / 4,
                               destRowStride, mml->height / 4,
                               1, texImage->Data, destRowStride,
                               texImage->Data);
   }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      assert(!texImage->IsCompressed);
   }

   if (ti->validated && ti->isInTM)
      fxTMReloadMipMapLevel(fxMesa, texObj, level);
   else
      fxTexInvalidate(ctx, texObj);
}


void
fxDDTexImage1D (GLcontext *ctx, GLenum target, GLint level,
	        GLint internalFormat, GLint width, GLint border,
	        GLenum format, GLenum type, const GLvoid *pixels,
	        const struct gl_pixelstore_attrib *packing,
	        struct gl_texture_object *texObj,
	        struct gl_texture_image *texImage)
{
 fxDDTexImage2D(ctx, target, level,
	        internalFormat, width, 1, border,
	        format, type, pixels,
	        packing,
	        texObj,
	        texImage);
}


void
fxDDTexSubImage1D(GLcontext * ctx, GLenum target, GLint level,
		  GLint xoffset,
		  GLsizei width,
		  GLenum format, GLenum type, const GLvoid * pixels,
		  const struct gl_pixelstore_attrib *packing,
		  struct gl_texture_object *texObj,
		  struct gl_texture_image *texImage)
{
 fxDDTexSubImage2D(ctx, target, level,
		   xoffset, 0, width, 1,
		   format, type, pixels,
		   packing,
		   texObj,
		   texImage);
}


GLboolean
fxDDTestProxyTexImage (GLcontext *ctx, GLenum target,
                       GLint level, GLint internalFormat,
                       GLenum format, GLenum type,
                       GLint width, GLint height,
                       GLint depth, GLint border)
{
 /* XXX todo - maybe through fxTexValidate() */
 return _mesa_test_proxy_teximage(ctx, target,
                                  level, internalFormat,
                                  format, type,
                                  width, height,
                                  depth, border);
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
