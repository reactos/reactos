/* -*- mode: c; c-basic-offset: 3 -*-
 *
 * Copyright 2000 VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_tex.c,v 1.7 2002/11/05 17:46:10 tsi Exp $ */

/*
 * New fixes:
 *	Daniel Borca <dborca@users.sourceforge.net>, 19 Jul 2004
 *
 * Original rewrite:
 *	Gareth Hughes <gareth@valinux.com>, 29 Sep - 1 Oct 2000
 *
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Brian Paul <brianp@valinux.com>
 *
 */


#include "enums.h"
#include "image.h"
#include "mipmap.h"
#include "texcompress.h"
#include "texformat.h"
#include "teximage.h"
#include "texstore.h"
#include "texobj.h"
#include "tdfx_context.h"
#include "tdfx_tex.h"
#include "tdfx_texman.h"


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
   GLuint dstImageOffsets = 0;

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
                              &dstImageOffsets,
                              srcWidth, srcHeight, 1,
                              texImage->_BaseFormat, _t, srcImage, &ctx->DefaultPacking);
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
                                      &dstImageOffsets,
                                      dstWidth, dstHeight, 1,
                                      GL_BGRA, CHAN_TYPE, dst, &ctx->DefaultPacking);
      FREE(dst);
      FREE(src);
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


/*
 * Compute various texture image parameters.
 * Input:  w, h - source texture width and height
 * Output:  lodlevel - Glide lod level token for the larger texture dimension
 *          aspectratio - Glide aspect ratio token
 *          sscale - S scale factor used during triangle setup
 *          tscale - T scale factor used during triangle setup
 *          wscale - OpenGL -> Glide image width scale factor
 *          hscale - OpenGL -> Glide image height scale factor
 *
 * Sample results:
 *      w    h       lodlevel               aspectRatio
 *     128  128  GR_LOD_LOG2_128 (=7)  GR_ASPECT_LOG2_1x1 (=0)
 *      64   64  GR_LOD_LOG2_64 (=6)   GR_ASPECT_LOG2_1x1 (=0)
 *      64   32  GR_LOD_LOG2_64 (=6)   GR_ASPECT_LOG2_2x1 (=1)
 *      32   64  GR_LOD_LOG2_64 (=6)   GR_ASPECT_LOG2_1x2 (=-1)
 *      32   32  GR_LOD_LOG2_32 (=5)   GR_ASPECT_LOG2_1x1 (=0)
 */
static void
tdfxTexGetInfo(const GLcontext *ctx, int w, int h,
               GrLOD_t *lodlevel, GrAspectRatio_t *aspectratio,
               float *sscale, float *tscale,
               int *wscale, int *hscale)
{
    int logw, logh, ar, lod, ws, hs;
    float s, t;

    ASSERT(w >= 1);
    ASSERT(h >= 1);

    logw = logbase2(w);
    logh = logbase2(h);
    ar = logw - logh;  /* aspect ratio = difference in log dimensions */
    s = t = 256.0;
    ws = hs = 1;

    /* Hardware only allows a maximum aspect ratio of 8x1, so handle
       |ar| > 3 by scaling the image and using an 8x1 aspect ratio */
    if (ar >= 0) {
        ASSERT(w >= h);
        lod = logw;
        if (ar <= GR_ASPECT_LOG2_8x1) {
            t = 256 >> ar;
        }
        else {
            /* have to stretch image height */
            t = 32.0;
            hs = 1 << (ar - 3);
            ar = GR_ASPECT_LOG2_8x1;
        }
    }
    else {
        ASSERT(w < h);
        lod = logh;
        if (ar >= GR_ASPECT_LOG2_1x8) {
            s = 256 >> -ar;
        }
        else {
            /* have to stretch image width */
            s = 32.0;
            ws = 1 << (-ar - 3);
            ar = GR_ASPECT_LOG2_1x8;
        }
    }

    if (lodlevel)
        *lodlevel = (GrLOD_t) lod;
    if (aspectratio)
        *aspectratio = (GrAspectRatio_t) ar;
    if (sscale)
        *sscale = s;
    if (tscale)
        *tscale = t;
    if (wscale)
        *wscale = ws;
    if (hscale)
        *hscale = hs;
}


/*
 * We need to call this when a texture object's minification filter
 * or texture image sizes change.
 */
static void RevalidateTexture(GLcontext *ctx, struct gl_texture_object *tObj)
{
    tdfxTexInfo *ti = TDFX_TEXTURE_DATA(tObj);
    GLint minl, maxl;

    if (!ti)
       return;

    minl = maxl = tObj->BaseLevel;

    if (tObj->Image[0][minl]) {
       maxl = MIN2(tObj->MaxLevel, tObj->Image[0][minl]->MaxLog2);

       /* compute largeLodLog2, aspect ratio and texcoord scale factors */
       tdfxTexGetInfo(ctx, tObj->Image[0][minl]->Width, tObj->Image[0][minl]->Height,
                      &ti->info.largeLodLog2,
                      &ti->info.aspectRatioLog2,
                      &(ti->sScale), &(ti->tScale), NULL, NULL);
    }

    if (tObj->Image[0][maxl] && (tObj->MinFilter != GL_NEAREST) && (tObj->MinFilter != GL_LINEAR)) {
        /* mipmapping: need to compute smallLodLog2 */
        tdfxTexGetInfo(ctx, tObj->Image[0][maxl]->Width,
                       tObj->Image[0][maxl]->Height,
                       &ti->info.smallLodLog2, NULL,
                       NULL, NULL, NULL, NULL);
    }
    else {
        /* not mipmapping: smallLodLog2 = largeLodLog2 */
        ti->info.smallLodLog2 = ti->info.largeLodLog2;
        maxl = minl;
    }

    ti->minLevel = minl;
    ti->maxLevel = maxl;
    ti->info.data = NULL;

   /* this is necessary because of fxDDCompressedTexImage2D */
   if (ti->padded) {
      struct gl_texture_image *texImage = tObj->Image[0][minl];
      tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
      if (mml->wScale != 1 || mml->hScale != 1) {
         ti->sScale /= mml->wScale;
         ti->tScale /= mml->hScale;
      }
   }
}


static tdfxTexInfo *
fxAllocTexObjData(tdfxContextPtr fxMesa)
{
    tdfxTexInfo *ti;

    if (!(ti = CALLOC(sizeof(tdfxTexInfo)))) {
        _mesa_problem(NULL, "tdfx driver: out of memory");
        return NULL;
    }

    ti->isInTM = GL_FALSE;

    ti->whichTMU = TDFX_TMU_NONE;

    ti->tm[TDFX_TMU0] = NULL;
    ti->tm[TDFX_TMU1] = NULL;

    ti->minFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
    ti->magFilt = GR_TEXTUREFILTER_BILINEAR;

    ti->sClamp = GR_TEXTURECLAMP_WRAP;
    ti->tClamp = GR_TEXTURECLAMP_WRAP;

    ti->mmMode = GR_MIPMAP_NEAREST;
    ti->LODblend = FXFALSE;

    return ti;
}


/*
 * Called via glBindTexture.
 */
static void
tdfxBindTexture(GLcontext * ctx, GLenum target,
                  struct gl_texture_object *tObj)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    tdfxTexInfo *ti;

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxDDTexBind(%d,%p)\n", tObj->Name,
                tObj->DriverData);
    }

    if ((target != GL_TEXTURE_1D) && (target != GL_TEXTURE_2D))
        return;

    if (!tObj->DriverData) {
        tObj->DriverData = fxAllocTexObjData(fxMesa);
    }

    ti = TDFX_TEXTURE_DATA(tObj);
    ti->lastTimeUsed = fxMesa->texBindNumber++;

    fxMesa->new_state |= TDFX_NEW_TEXTURE;
}


/*
 * Called via glTexEnv.
 */
static void
tdfxTexEnv(GLcontext * ctx, GLenum target, GLenum pname,
             const GLfloat * param)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

    if ( TDFX_DEBUG & DEBUG_VERBOSE_API ) {
        if (param)
            fprintf(stderr, "fxmesa: texenv(%x,%x)\n", pname,
                    (GLint) (*param));
        else
            fprintf(stderr, "fxmesa: texenv(%x)\n", pname);
    }

    /* XXX this is a bit of a hack to force the Glide texture
     * state to be updated.
     */
    fxMesa->TexState.EnvMode[ctx->Texture.CurrentUnit]  = 0;

    fxMesa->new_state |= TDFX_NEW_TEXTURE;
}


/*
 * Called via glTexParameter.
 */
static void
tdfxTexParameter(GLcontext * ctx, GLenum target,
                   struct gl_texture_object *tObj,
                   GLenum pname, const GLfloat * params)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    GLenum param = (GLenum) (GLint) params[0];
    tdfxTexInfo *ti;

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxDDTexParam(%d,%p,%x,%x)\n", tObj->Name,
                tObj->DriverData, pname, param);
    }

    if ((target != GL_TEXTURE_1D) && (target != GL_TEXTURE_2D))
        return;

    if (!tObj->DriverData)
        tObj->DriverData = fxAllocTexObjData(fxMesa);

    ti = TDFX_TEXTURE_DATA(tObj);

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
            if (!fxMesa->Glide.HaveCombineExt) {
                 if (fxMesa->haveTwoTMUs) {
                     ti->mmMode = GR_MIPMAP_NEAREST;
                     ti->LODblend = FXTRUE;
                 }
                 else {
                     ti->mmMode = GR_MIPMAP_NEAREST_DITHER;
                     ti->LODblend = FXFALSE;
                 }
                 ti->minFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
                 break;
            }
            /* XXX Voodoo3/Banshee mipmap blending seems to produce
             * incorrectly filtered colors for the smallest mipmap levels.
             * To work-around we fall-through here and use a different filter.
             */
        case GL_NEAREST_MIPMAP_NEAREST:
            ti->mmMode = GR_MIPMAP_NEAREST;
            ti->minFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
            ti->LODblend = FXFALSE;
            break;
        case GL_LINEAR_MIPMAP_LINEAR:
            if (!fxMesa->Glide.HaveCombineExt) {
                if (fxMesa->haveTwoTMUs) {
                    ti->mmMode = GR_MIPMAP_NEAREST;
                    ti->LODblend = FXTRUE;
                }
                else {
                    ti->mmMode = GR_MIPMAP_NEAREST_DITHER;
                    ti->LODblend = FXFALSE;
                }
                ti->minFilt = GR_TEXTUREFILTER_BILINEAR;
                break;
            }
            /* XXX Voodoo3/Banshee mipmap blending seems to produce
             * incorrectly filtered colors for the smallest mipmap levels.
             * To work-around we fall-through here and use a different filter.
             */
        case GL_LINEAR_MIPMAP_NEAREST:
            ti->mmMode = GR_MIPMAP_NEAREST;
            ti->minFilt = GR_TEXTUREFILTER_BILINEAR;
            ti->LODblend = FXFALSE;
            break;
        default:
            break;
        }
        ti->reloadImages = GL_TRUE;
        RevalidateTexture(ctx, tObj);
        fxMesa->new_state |= TDFX_NEW_TEXTURE;
        break;

    case GL_TEXTURE_MAG_FILTER:
        switch (param) {
        case GL_NEAREST:
            ti->magFilt = GR_TEXTUREFILTER_POINT_SAMPLED;
            break;
        case GL_LINEAR:
            ti->magFilt = GR_TEXTUREFILTER_BILINEAR;
            break;
        default:
            break;
        }
        fxMesa->new_state |= TDFX_NEW_TEXTURE;
        break;

    case GL_TEXTURE_WRAP_S:
        switch (param) {
        case GL_CLAMP_TO_BORDER:
        case GL_CLAMP_TO_EDGE:
        case GL_CLAMP:
            ti->sClamp = GR_TEXTURECLAMP_CLAMP;
            break;
        case GL_REPEAT:
            ti->sClamp = GR_TEXTURECLAMP_WRAP;
            break;
        case GL_MIRRORED_REPEAT:
            ti->sClamp = GR_TEXTURECLAMP_MIRROR_EXT;
            break;
        default:
            break;
        }
        fxMesa->new_state |= TDFX_NEW_TEXTURE;
        break;

    case GL_TEXTURE_WRAP_T:
        switch (param) {
        case GL_CLAMP_TO_BORDER:
        case GL_CLAMP_TO_EDGE:
        case GL_CLAMP:
            ti->tClamp = GR_TEXTURECLAMP_CLAMP;
            break;
        case GL_REPEAT:
            ti->tClamp = GR_TEXTURECLAMP_WRAP;
            break;
        case GL_MIRRORED_REPEAT:
            ti->tClamp = GR_TEXTURECLAMP_MIRROR_EXT;
            break;
        default:
            break;
        }
        fxMesa->new_state |= TDFX_NEW_TEXTURE;
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
        RevalidateTexture(ctx, tObj);
        break;
    case GL_TEXTURE_MAX_LEVEL:
        RevalidateTexture(ctx, tObj);
        break;

    default:
        break;
    }
}


/*
 * Called via glDeleteTextures to delete a texture object.
 * Here, we delete the Glide data associated with the texture.
 */
static void
tdfxDeleteTexture(GLcontext * ctx, struct gl_texture_object *tObj)
{
    if (ctx && ctx->DriverCtx) {
        tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
        tdfxTMFreeTexture(fxMesa, tObj);
        fxMesa->new_state |= TDFX_NEW_TEXTURE;
        /* Free mipmap images and the texture object itself */
        _mesa_delete_texture_object(ctx, tObj);
    }
}


/*
 * Return true if texture is resident, false otherwise.
 */
static GLboolean
tdfxIsTextureResident(GLcontext *ctx, struct gl_texture_object *tObj)
{
    tdfxTexInfo *ti = TDFX_TEXTURE_DATA(tObj);
    return (GLboolean) (ti && ti->isInTM);
}



/*
 * Convert a gl_color_table texture palette to Glide's format.
 */
static GrTexTable_t
convertPalette(FxU32 data[256], const struct gl_color_table *table)
{
    const GLubyte *tableUB = table->TableUB;
    GLint width = table->Size;
    FxU32 r, g, b, a;
    GLint i;

    switch (table->_BaseFormat) {
    case GL_INTENSITY:
        for (i = 0; i < width; i++) {
            r = tableUB[i];
            g = tableUB[i];
            b = tableUB[i];
            a = tableUB[i];
            data[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
        return GR_TEXTABLE_PALETTE_6666_EXT;
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
        return GR_TEXTABLE_PALETTE_6666_EXT;
    case GL_LUMINANCE_ALPHA:
        for (i = 0; i < width; i++) {
            r = g = b = tableUB[i * 2 + 0];
            a = tableUB[i * 2 + 1];
            data[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
        return GR_TEXTABLE_PALETTE_6666_EXT;
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
        return GR_TEXTABLE_PALETTE_6666_EXT;
    default:
        /* XXX fixme: how can this happen? */
        _mesa_error(NULL, GL_INVALID_ENUM, "convertPalette: table->Format == %s",
                                           _mesa_lookup_enum_by_nr(table->Format));
        return GR_TEXTABLE_PALETTE;
    }
}



static void
tdfxUpdateTexturePalette(GLcontext * ctx, struct gl_texture_object *tObj)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

    if (tObj) {
        /* per-texture palette */
        tdfxTexInfo *ti;
        
        /* This might be a proxy texture. */
        if (!tObj->Palette.TableUB)
            return;
            
        if (!tObj->DriverData)
            tObj->DriverData = fxAllocTexObjData(fxMesa);
        ti = TDFX_TEXTURE_DATA(tObj);
        assert(ti);
        ti->paltype = convertPalette(ti->palette.data, &tObj->Palette);
        /*tdfxTexInvalidate(ctx, tObj);*/
    }
    else {
        /* global texture palette */
        fxMesa->TexPalette.Type = convertPalette(fxMesa->glbPalette.data, &ctx->Texture.Palette);
	fxMesa->TexPalette.Data = &(fxMesa->glbPalette.data);
	fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_PALETTE;
    }
    fxMesa->new_state |= TDFX_NEW_TEXTURE; /* XXX too heavy-handed */
}


/**********************************************************************/
/**** NEW TEXTURE IMAGE FUNCTIONS                                  ****/
/**********************************************************************/

#if 000
static FxBool TexusFatalError = FXFALSE;
static FxBool TexusError = FXFALSE;

#define TX_DITHER_NONE                                  0x00000000

static void
fxTexusError(const char *string, FxBool fatal)
{
    _mesa_problem(NULL, string);
   /*
    * Just propagate the fatal value up.
    */
    TexusError = FXTRUE;
    TexusFatalError = fatal;
}
#endif


static const struct gl_texture_format *
tdfxChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
                           GLenum srcFormat, GLenum srcType )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   const GLboolean allow32bpt = TDFX_IS_NAPALM(fxMesa);

   switch (internalFormat) {
   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      return &_mesa_texformat_a8;
   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      return &_mesa_texformat_l8;
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
   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      return &_mesa_texformat_i8;
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
      return &_mesa_texformat_rgb565;
   case GL_COMPRESSED_RGB:
      /* intentional fall-through */
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
      /* intentional fall-through */
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
      return allow32bpt ? &_mesa_texformat_argb8888
                        : &_mesa_texformat_argb4444;
   case GL_RGB5_A1:
      return &_mesa_texformat_argb1555;
   case GL_COLOR_INDEX:
   case GL_COLOR_INDEX1_EXT:
   case GL_COLOR_INDEX2_EXT:
   case GL_COLOR_INDEX4_EXT:
   case GL_COLOR_INDEX8_EXT:
   case GL_COLOR_INDEX12_EXT:
   case GL_COLOR_INDEX16_EXT:
      return &_mesa_texformat_ci8;
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
      _mesa_problem(ctx, "unexpected format in tdfxChooseTextureFormat");
      return NULL;
   }
}


/*
 * Return the Glide format for the given mesa texture format.
 */
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


/* Texel-fetch functions for software texturing and glGetTexImage().
 * We should have been able to use some "standard" fetch functions (which
 * may get defined in texutil.c) but we have to account for scaled texture
 * images on tdfx hardware (the 8:1 aspect ratio limit).
 * Hence, we need special functions here.
 */
extern void
fxt1_decode_1 (const void *texture, int width,
               int i, int j, unsigned char *rgba);

static void
fetch_intensity8(const struct gl_texture_image *texImage,
		 GLint i, GLint j, GLint k, GLchan * rgba)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
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
		 GLint i, GLint j, GLint k, GLchan * rgba)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
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
	     GLint i, GLint j, GLint k, GLchan * rgba)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
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
	     GLint i, GLint j, GLint k, GLchan * indexOut)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
    const GLubyte *texel;

    i = i * mml->wScale;
    j = j * mml->hScale;

    texel = ((GLubyte *) texImage->Data) + j * mml->width + i;
    *indexOut = *texel;
}


static void
fetch_luminance8_alpha8(const struct gl_texture_image *texImage,
			GLint i, GLint j, GLint k, GLchan * rgba)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
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
	     GLint i, GLint j, GLint k, GLchan * rgba)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
    const GLushort *texel;

    i = i * mml->wScale;
    j = j * mml->hScale;

    texel = ((GLushort *) texImage->Data) + j * mml->width + i;
    rgba[RCOMP] = (((*texel) >> 11) & 0x1f) * 255 / 31;
    rgba[GCOMP] = (((*texel) >> 5) & 0x3f) * 255 / 63;
    rgba[BCOMP] = (((*texel) >> 0) & 0x1f) * 255 / 31;
    rgba[ACOMP] = 255;
}


static void
fetch_r4g4b4a4(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLchan * rgba)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
    const GLushort *texel;

    i = i * mml->wScale;
    j = j * mml->hScale;

    texel = ((GLushort *) texImage->Data) + j * mml->width + i;
    rgba[RCOMP] = (((*texel) >> 12) & 0xf) * 255 / 15;
    rgba[GCOMP] = (((*texel) >> 8) & 0xf) * 255 / 15;
    rgba[BCOMP] = (((*texel) >> 4) & 0xf) * 255 / 15;
    rgba[ACOMP] = (((*texel) >> 0) & 0xf) * 255 / 15;
}


static void
fetch_r5g5b5a1(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLchan * rgba)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
    const GLushort *texel;

    i = i * mml->wScale;
    j = j * mml->hScale;

    texel = ((GLushort *) texImage->Data) + j * mml->width + i;
    rgba[RCOMP] = (((*texel) >> 11) & 0x1f) * 255 / 31;
    rgba[GCOMP] = (((*texel) >> 6) & 0x1f) * 255 / 31;
    rgba[BCOMP] = (((*texel) >> 1) & 0x1f) * 255 / 31;
    rgba[ACOMP] = (((*texel) >> 0) & 0x01) * 255;
}


static void
fetch_a8r8g8b8(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLchan * rgba)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);
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
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);

    i = i * mml->wScale;
    j = j * mml->hScale;

    fxt1_decode_1(texImage->Data, mml->width, i, j, rgba);
    rgba[ACOMP] = 255;
}


static void
fetch_rgba_fxt1(const struct gl_texture_image *texImage,
		GLint i, GLint j, GLint k, GLchan *rgba)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);

    i = i * mml->wScale;
    j = j * mml->hScale;

    fxt1_decode_1(texImage->Data, mml->width, i, j, rgba);
}


static void
fetch_rgb_dxt1(const struct gl_texture_image *texImage,
	       GLint i, GLint j, GLint k, GLchan *rgba)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);

    i = i * mml->wScale;
    j = j * mml->hScale;

    _mesa_texformat_rgb_dxt1.FetchTexel2D(texImage, i, j, k, rgba);
}


static void
fetch_rgba_dxt1(const struct gl_texture_image *texImage,
		GLint i, GLint j, GLint k, GLchan *rgba)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);

    i = i * mml->wScale;
    j = j * mml->hScale;

    _mesa_texformat_rgba_dxt1.FetchTexel2D(texImage, i, j, k, rgba);
}


static void
fetch_rgba_dxt3(const struct gl_texture_image *texImage,
		GLint i, GLint j, GLint k, GLchan *rgba)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);

    i = i * mml->wScale;
    j = j * mml->hScale;

    _mesa_texformat_rgba_dxt3.FetchTexel2D(texImage, i, j, k, rgba);
}


static void
fetch_rgba_dxt5(const struct gl_texture_image *texImage,
		GLint i, GLint j, GLint k, GLchan *rgba)
{
    const tdfxMipMapLevel *mml = TDFX_TEXIMAGE_DATA(texImage);

    i = i * mml->wScale;
    j = j * mml->hScale;

    _mesa_texformat_rgba_dxt5.FetchTexel2D(texImage, i, j, k, rgba);
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
	       tdfxMipMapLevel *mml,
	       struct gl_texture_image *texImage,
	       GLint texelBytes,
	       GLint dstRowStride)
{
   const GLint newWidth = width * mml->wScale;
   const GLint newHeight = height * mml->hScale;
   GLvoid *tempImage;
   GLuint dstImageOffsets = 0;

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
                                      &dstImageOffsets,
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
	 FREE(rawImage);
         return GL_FALSE;
      }
      /* unpack image, apply transfer ops and store in rawImage */
      _mesa_texstore_rgba8888(ctx, 2, GL_RGBA,
                              &_mesa_texformat_rgba8888_rev, rawImage,
                              0, 0, 0, /* dstX/Y/Zoffset */
                              width * rawBytes, /* dstRowStride */
                              &dstImageOffsets,
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
                                      &dstImageOffsets,
                                      newWidth, newHeight, 1,
                                      GL_RGBA, CHAN_TYPE, tempImage, &ctx->DefaultPacking);
      FREE(rawImage);
   }

   FREE(tempImage);

   return GL_TRUE;
}


static void
tdfxTexImage2D(GLcontext *ctx, GLenum target, GLint level,
               GLint internalFormat, GLint width, GLint height, GLint border,
               GLenum format, GLenum type, const GLvoid *pixels,
               const struct gl_pixelstore_attrib *packing,
               struct gl_texture_object *texObj,
               struct gl_texture_image *texImage)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    tdfxTexInfo *ti;
    tdfxMipMapLevel *mml;
    GLint texelBytes, dstRowStride;
    GLuint mesaFormat;

    /*
    printf("TexImage id=%d int 0x%x  format 0x%x  type 0x%x  %dx%d\n",
           texObj->Name, texImage->InternalFormat, format, type,
           texImage->Width, texImage->Height);
    */

    ti = TDFX_TEXTURE_DATA(texObj);
    if (!ti) {
        texObj->DriverData = fxAllocTexObjData(fxMesa);
        if (!texObj->DriverData) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
            return;
        }
        ti = TDFX_TEXTURE_DATA(texObj);
    }
    assert(ti);

    mml = TDFX_TEXIMAGE_DATA(texImage);
    if (!mml) {
        texImage->DriverData = CALLOC(sizeof(tdfxMipMapLevel));
        if (!texImage->DriverData) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
            return;
        }
        mml = TDFX_TEXIMAGE_DATA(texImage);
    }

    /* Determine width and height scale factors for texture.
     * Remember, Glide is limited to 8:1 aspect ratios.
     */
    tdfxTexGetInfo(ctx,
                   texImage->Width, texImage->Height,
                   NULL,       /* lod level          */
                   NULL,       /* aspect ratio       */
                   NULL, NULL, /* sscale, tscale     */
                   &mml->wScale, &mml->hScale);

    /* rescaled size: */
    mml->width = width * mml->wScale;
    mml->height = height * mml->hScale;

#if FX_COMPRESS_S3TC_AS_FXT1_HACK
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
    mesaFormat = texImage->TexFormat->MesaFormat;
    mml->glideFormat = fxGlideFormat(mesaFormat);
    ti->info.format = mml->glideFormat;
    texImage->FetchTexelc = fxFetchFunction(mesaFormat);
    texelBytes = texImage->TexFormat->TexelBytes;

    if (texImage->IsCompressed) {
       texImage->CompressedSize = _mesa_compressed_texture_size(ctx,
                                                        	mml->width,
                                                        	mml->height,
                                                        	1,
                                                        	mesaFormat);
       dstRowStride = _mesa_compressed_row_stride(texImage->TexFormat->MesaFormat, mml->width);
       texImage->Data = _mesa_alloc_texmemory(texImage->CompressedSize);
    } else {
       dstRowStride = mml->width * texelBytes;
       texImage->Data = _mesa_alloc_texmemory(mml->width * mml->height * texelBytes);
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
                                          texImage->ImageOffsets,
                                          width, height, 1,
                                          format, type, pixels, packing);
       }

      /* GL_SGIS_generate_mipmap */
      if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
         GLint mipWidth, mipHeight;
         tdfxMipMapLevel *mip;
         struct gl_texture_image *mipImage;
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
            mipImage = _mesa_select_tex_image(ctx, texObj, target, level);
            mip = TDFX_TEXIMAGE_DATA(mipImage);
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

    RevalidateTexture(ctx, texObj);

    ti->reloadImages = GL_TRUE;
    fxMesa->new_state |= TDFX_NEW_TEXTURE;
}


static void
tdfxTexSubImage2D(GLcontext *ctx, GLenum target, GLint level,
                    GLint xoffset, GLint yoffset,
                    GLsizei width, GLsizei height,
                    GLenum format, GLenum type,
                    const GLvoid *pixels,
                    const struct gl_pixelstore_attrib *packing,
                    struct gl_texture_object *texObj,
                    struct gl_texture_image *texImage )
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    tdfxTexInfo *ti;
    tdfxMipMapLevel *mml;
    GLint texelBytes, dstRowStride;

    if (!texObj->DriverData) {
        _mesa_problem(ctx, "problem in fxDDTexSubImage2D");
        return;
    }

    ti = TDFX_TEXTURE_DATA(texObj);
    assert(ti);
    mml = TDFX_TEXIMAGE_DATA(texImage);
    assert(mml);

    assert(texImage->Data);	/* must have an existing texture image! */
    assert(texImage->_BaseFormat);

    texelBytes = texImage->TexFormat->TexelBytes;
    if (texImage->IsCompressed) {
       dstRowStride = _mesa_compressed_row_stride(texImage->TexFormat->MesaFormat, mml->width);
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
                                        texImage->TexFormat, texImage->Data,
                                        xoffset, yoffset, 0,
                                        dstRowStride,
                                        texImage->ImageOffsets,
                                        width, height, 1,
                                        format, type, pixels, packing);
    }

   /* GL_SGIS_generate_mipmap */
   if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
      GLint mipWidth, mipHeight;
      tdfxMipMapLevel *mip;
      struct gl_texture_image *mipImage;
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
         mipImage = _mesa_select_tex_image(ctx, texObj, target, level);
         mip = TDFX_TEXIMAGE_DATA(mipImage);
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

    ti->reloadImages = GL_TRUE; /* signal the image needs to be reloaded */
    fxMesa->new_state |= TDFX_NEW_TEXTURE;  /* XXX this might be a bit much */
}


static void
tdfxTexImage1D(GLcontext *ctx, GLenum target, GLint level,
               GLint internalFormat, GLint width, GLint border,
               GLenum format, GLenum type, const GLvoid *pixels,
               const struct gl_pixelstore_attrib *packing,
               struct gl_texture_object *texObj,
               struct gl_texture_image *texImage)
{
 tdfxTexImage2D(ctx, target, level,
                internalFormat, width, 1, border,
                format, type, pixels,
                packing,
                texObj,
                texImage);
}

static void
tdfxTexSubImage1D(GLcontext *ctx, GLenum target, GLint level,
                    GLint xoffset,
                    GLsizei width,
                    GLenum format, GLenum type,
                    const GLvoid *pixels,
                    const struct gl_pixelstore_attrib *packing,
                    struct gl_texture_object *texObj,
                    struct gl_texture_image *texImage )
{
 tdfxTexSubImage2D(ctx, target, level,
                    xoffset, 0,
                    width, 1,
                    format, type,
                    pixels,
                    packing,
                    texObj,
                    texImage);
}

/**********************************************************************/
/**** COMPRESSED TEXTURE IMAGE FUNCTIONS                           ****/
/**********************************************************************/

static void
tdfxCompressedTexImage2D (GLcontext *ctx, GLenum target,
                          GLint level, GLint internalFormat,
                          GLsizei width, GLsizei height, GLint border,
                          GLsizei imageSize, const GLvoid *data,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    tdfxTexInfo *ti;
    tdfxMipMapLevel *mml;
    GLuint mesaFormat;

    if (TDFX_DEBUG & DEBUG_VERBOSE_DRI) {
        fprintf(stderr, "tdfxCompressedTexImage2D: id=%d int 0x%x  %dx%d\n",
                        texObj->Name, internalFormat,
                        width, height);
    }

    if ((target != GL_TEXTURE_1D && target != GL_TEXTURE_2D) || texImage->Border > 0) {
       _mesa_problem(NULL, "tdfx: unsupported texture in tdfxCompressedTexImg()\n");
       return;
    }

    assert(texImage->IsCompressed);

    ti = TDFX_TEXTURE_DATA(texObj);
    if (!ti) {
        texObj->DriverData = fxAllocTexObjData(fxMesa);
        if (!texObj->DriverData) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2D");
            return;
        }
        ti = TDFX_TEXTURE_DATA(texObj);
    }
    assert(ti);

    mml = TDFX_TEXIMAGE_DATA(texImage);
    if (!mml) {
        texImage->DriverData = CALLOC(sizeof(tdfxMipMapLevel));
        if (!texImage->DriverData) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2D");
            return;
        }
        mml = TDFX_TEXIMAGE_DATA(texImage);
    }

    tdfxTexGetInfo(ctx, width, height, NULL, NULL, NULL, NULL,
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
    mesaFormat = texImage->TexFormat->MesaFormat;
    mml->glideFormat = fxGlideFormat(mesaFormat);
    ti->info.format = mml->glideFormat;
    texImage->FetchTexelc = fxFetchFunction(mesaFormat);

    /* allocate new storage for texture image, if needed */
    if (!texImage->Data) {
       texImage->CompressedSize = _mesa_compressed_texture_size(ctx,
                                                                mml->width,
                                                                mml->height,
                                                                1,
                                                                mesaFormat);
       texImage->Data = _mesa_alloc_texmemory(texImage->CompressedSize);
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
       const GLuint mesaFormat = texImage->TexFormat->MesaFormat;
       GLuint srcRowStride = _mesa_compressed_row_stride(mesaFormat, width);
 
       GLuint destRowStride = _mesa_compressed_row_stride(mesaFormat,
                                                   mml->width);
 
       _mesa_upscale_teximage2d(srcRowStride, (height+3) / 4,
                                destRowStride, (mml->height+3) / 4,
                                1, data, srcRowStride,
                                texImage->Data);
       ti->padded = GL_TRUE;
    } else {
       MEMCPY(texImage->Data, data, texImage->CompressedSize);
    }

    /* GL_SGIS_generate_mipmap */
    if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
       assert(!texImage->IsCompressed);
    }

    RevalidateTexture(ctx, texObj);

    ti->reloadImages = GL_TRUE;
    fxMesa->new_state |= TDFX_NEW_TEXTURE;
}


static void
tdfxCompressedTexSubImage2D( GLcontext *ctx, GLenum target,
                             GLint level, GLint xoffset,
                             GLint yoffset, GLsizei width,
                             GLint height, GLenum format,
                             GLsizei imageSize, const GLvoid *data,
                             struct gl_texture_object *texObj,
                             struct gl_texture_image *texImage )
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    tdfxTexInfo *ti;
    tdfxMipMapLevel *mml;
    GLint destRowStride, srcRowStride;
    GLint i, rows;
    GLubyte *dest;
    const GLuint mesaFormat = texImage->TexFormat->MesaFormat;

    if (TDFX_DEBUG & DEBUG_VERBOSE_DRI) {
        fprintf(stderr, "tdfxCompressedTexSubImage2D: id=%d\n", texObj->Name);
    }

    ti = TDFX_TEXTURE_DATA(texObj);
    assert(ti);
    mml = TDFX_TEXIMAGE_DATA(texImage);
    assert(mml);

    srcRowStride = _mesa_compressed_row_stride(mesaFormat, width);

    destRowStride = _mesa_compressed_row_stride(mesaFormat, mml->width);
    dest = _mesa_compressed_image_address(xoffset, yoffset, 0,
                                          mesaFormat,
                                          mml->width,
                               (GLubyte*) texImage->Data);

    rows = height / 4; /* [dBorca] hardcoded 4, but works for FXT1/DXTC */

    for (i = 0; i < rows; i++) {
       MEMCPY(dest, data, srcRowStride);
       dest += destRowStride;
       data = (GLvoid *)((intptr_t)data + (intptr_t)srcRowStride);
    }

    /* [dBorca] Hack alert:
     * see fxDDCompressedTexImage2D for caveats
     */
    if (mml->wScale != 1 || mml->hScale != 1) {
       srcRowStride = _mesa_compressed_row_stride(mesaFormat, texImage->Width);
 
       destRowStride = _mesa_compressed_row_stride(mesaFormat, mml->width);
       _mesa_upscale_teximage2d(srcRowStride, texImage->Height / 4,
                                destRowStride, mml->height / 4,
                                1, texImage->Data, destRowStride,
                                texImage->Data);
    }

    /* GL_SGIS_generate_mipmap */
    if (level == texObj->BaseLevel && texObj->GenerateMipmap) {
       assert(!texImage->IsCompressed);
    }

    RevalidateTexture(ctx, texObj);

    ti->reloadImages = GL_TRUE;
    fxMesa->new_state |= TDFX_NEW_TEXTURE;
}


#if	0
static void
PrintTexture(int w, int h, int c, const GLubyte * data)
{
    int i, j;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            if (c == 2)
                printf("%02x %02x  ", data[0], data[1]);
            else if (c == 3)
                printf("%02x %02x %02x  ", data[0], data[1], data[2]);
            data += c;
        }
        printf("\n");
    }
}
#endif


GLboolean
tdfxTestProxyTexImage(GLcontext *ctx, GLenum target,
                        GLint level, GLint internalFormat,
                        GLenum format, GLenum type,
                        GLint width, GLint height,
                        GLint depth, GLint border)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    struct gl_shared_state *mesaShared = fxMesa->glCtx->Shared;
    struct tdfxSharedState *shared = (struct tdfxSharedState *) mesaShared->DriverData;

    switch (target) {
    case GL_PROXY_TEXTURE_1D:
	/*JJJ wrong*/
    case GL_PROXY_TEXTURE_2D:
        {
            struct gl_texture_object *tObj;
            tdfxTexInfo *ti;
            int memNeeded;

            tObj = ctx->Texture.Proxy2D;
            if (!tObj->DriverData)
                tObj->DriverData = fxAllocTexObjData(fxMesa);
            ti = TDFX_TEXTURE_DATA(tObj);
            assert(ti);

            /* assign the parameters to test against */
            tObj->Image[0][level]->Width = width;
            tObj->Image[0][level]->Height = height;
            tObj->Image[0][level]->Border = border;
#if 0
            tObj->Image[0][level]->InternalFormat = internalFormat;
#endif
            if (level == 0) {
               /* don't use mipmap levels > 0 */
               tObj->MinFilter = tObj->MagFilter = GL_NEAREST;
            }
            else {
               /* test with all mipmap levels */
               tObj->MinFilter = GL_LINEAR_MIPMAP_LINEAR;
               tObj->MagFilter = GL_NEAREST;
            }
            RevalidateTexture(ctx, tObj);

            /*
            printf("small lodlog2 0x%x\n", ti->info.smallLodLog2);
            printf("large lodlog2 0x%x\n", ti->info.largeLodLog2);
            printf("aspect ratio 0x%x\n", ti->info.aspectRatioLog2);
            printf("glide format 0x%x\n", ti->info.format);
            printf("data %p\n", ti->info.data);
            printf("lodblend %d\n", (int) ti->LODblend);
            */

            /* determine where texture will reside */
            if (ti->LODblend && !shared->umaTexMemory) {
                /* XXX GR_MIPMAPLEVELMASK_BOTH might not be right, but works */
                memNeeded = fxMesa->Glide.grTexTextureMemRequired(
                                        GR_MIPMAPLEVELMASK_BOTH, &(ti->info));
            }
            else {
                /* XXX GR_MIPMAPLEVELMASK_BOTH might not be right, but works */
                memNeeded = fxMesa->Glide.grTexTextureMemRequired(
                                        GR_MIPMAPLEVELMASK_BOTH, &(ti->info));
            }
            /*
            printf("Proxy test %d > %d\n", memNeeded, shared->totalTexMem[0]);
            */
            if (memNeeded > shared->totalTexMem[0])
                return GL_FALSE;
            else
                return GL_TRUE;
        }
    case GL_PROXY_TEXTURE_3D:
        return GL_TRUE;  /* software rendering */
    default:
        return GL_TRUE;  /* never happens, silence compiler */
    }
}


/**
 * Allocate a new texture object.
 * Called via ctx->Driver.NewTextureObject.
 * Note: this function will be called during context creation to
 * allocate the default texture objects.
 * Note: we could use containment here to 'derive' the driver-specific
 * texture object from the core mesa gl_texture_object.  Not done at this time.
 */
static struct gl_texture_object *
tdfxNewTextureObject( GLcontext *ctx, GLuint name, GLenum target )
{
   struct gl_texture_object *obj;
   obj = _mesa_new_texture_object(ctx, name, target);
   return obj;
}


void tdfxInitTextureFuncs( struct dd_function_table *functions )
{
   functions->BindTexture		= tdfxBindTexture;
   functions->NewTextureObject		= tdfxNewTextureObject;
   functions->DeleteTexture		= tdfxDeleteTexture;
   functions->TexEnv			= tdfxTexEnv;
   functions->TexParameter		= tdfxTexParameter;
   functions->ChooseTextureFormat       = tdfxChooseTextureFormat;
   functions->TexImage1D		= tdfxTexImage1D;
   functions->TexSubImage1D		= tdfxTexSubImage1D;
   functions->TexImage2D		= tdfxTexImage2D;
   functions->TexSubImage2D		= tdfxTexSubImage2D;
   functions->IsTextureResident		= tdfxIsTextureResident;
   functions->CompressedTexImage2D	= tdfxCompressedTexImage2D;
   functions->CompressedTexSubImage2D	= tdfxCompressedTexSubImage2D;
   functions->UpdateTexturePalette      = tdfxUpdateTexturePalette;
}
