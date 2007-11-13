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

/* fxsetup.c - 3Dfx VooDoo rendering mode setup functions */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "fxdrv.h"
#include "enums.h"
#include "tnl.h"
#include "tnl/t_context.h"
#include "swrast.h"
#include "texstore.h"


static void
fxTexValidate(GLcontext * ctx, struct gl_texture_object *tObj)
{
   tfxTexInfo *ti = fxTMGetTexInfo(tObj);
   GLint minl, maxl;

   if (ti->validated) {
      if (TDFX_DEBUG & VERBOSE_DRIVER) {
	 fprintf(stderr, "fxTexValidate(NOP)\n");
      }
      return;
   }

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxTexValidate(%p (%d))\n", (void *)tObj, tObj->Name);
   }

   ti->tObj = tObj;
   minl = ti->minLevel = tObj->BaseLevel;
   maxl = ti->maxLevel = MIN2(tObj->MaxLevel, tObj->Image[0][0]->MaxLog2);

#if FX_RESCALE_BIG_TEXURES_HACK
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   /* [dBorca]
    * Fake textures larger than HW supports:
    * 1) we have mipmaps. Then we just push up to the first supported
    *    LOD. A possible drawback is that Mesa will ignore the skipped
    *    LODs on further texture handling.
    *    Will this interfere with GL_TEXTURE_[MIN|BASE]_LEVEL? How?
    * 2) we don't have mipmaps. We need to rescale the big LOD in place.
    *    The above approach is somehow dumb! we might have rescaled
    *    once in TexImage2D to accomodate aspect ratio, and now we
    *    are rescaling again. The thing is, in TexImage2D we don't
    *    know whether we'll hit 1) or 2) by the time of validation.
    */
   if ((tObj->MinFilter == GL_NEAREST) || (tObj->MinFilter == GL_LINEAR)) {
      /* no mipmaps! */
      struct gl_texture_image *texImage = tObj->Image[0][minl];
      tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
      GLint _w, _h, maxSize = 1 << fxMesa->textureMaxLod;
      if ((mml->width > maxSize) || (mml->height > maxSize)) {
         /* need to rescale */
         GLint texelBytes = texImage->TexFormat->TexelBytes;
         GLvoid *texImage_Data = texImage->Data;
         _w = MIN2(texImage->Width, maxSize);
         _h = MIN2(texImage->Height, maxSize);
         if (TDFX_DEBUG & VERBOSE_TEXTURE) {
            fprintf(stderr, "fxTexValidate: rescaling %d x %d -> %d x %d\n",
                            texImage->Width, texImage->Height, _w, _h);
         }
         /* we should leave these as is and... (!) */
         texImage->Width = _w;
         texImage->Height = _h;
         fxTexGetInfo(_w, _h, NULL, NULL, NULL, NULL,
                      &(mml->wScale), &(mml->hScale));
         _w *= mml->wScale;
         _h *= mml->hScale;
         texImage->Data = _mesa_malloc(_w * _h * texelBytes);
         _mesa_rescale_teximage2d(texelBytes,
                                  mml->width,
                                  _w * texelBytes, /* dst stride */
                                  mml->width, mml->height, /* src */
                                  _w, _h, /* dst */
                                  texImage_Data /*src*/, texImage->Data /*dst*/ );
         _mesa_free(texImage_Data);
         mml->width = _w;
         mml->height = _h;
         /* (!) ... and set mml->wScale = _w / texImage->Width */
      }
   } else {
      /* mipmapping */
      if (maxl - minl > fxMesa->textureMaxLod) {
         /* skip a certain number of LODs */
         minl += maxl - fxMesa->textureMaxLod;
         if (TDFX_DEBUG & VERBOSE_TEXTURE) {
            fprintf(stderr, "fxTexValidate: skipping %d LODs\n", minl - ti->minLevel);
         }
         ti->minLevel = tObj->BaseLevel = minl;
      }
   }
}
#endif

   fxTexGetInfo(tObj->Image[0][minl]->Width, tObj->Image[0][minl]->Height,
		&(FX_largeLodLog2(ti->info)), &(FX_aspectRatioLog2(ti->info)),
		&(ti->sScale), &(ti->tScale),
		NULL, NULL);

   if ((tObj->MinFilter != GL_NEAREST) && (tObj->MinFilter != GL_LINEAR))
      fxTexGetInfo(tObj->Image[0][maxl]->Width, tObj->Image[0][maxl]->Height,
		   &(FX_smallLodLog2(ti->info)), NULL,
		   NULL, NULL, NULL, NULL);
   else
      FX_smallLodLog2(ti->info) = FX_largeLodLog2(ti->info);

   /* [dBorca] this is necessary because of fxDDCompressedTexImage2D */
   if (ti->padded) {
      struct gl_texture_image *texImage = tObj->Image[0][minl];
      tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);
      if (mml->wScale != 1 || mml->hScale != 1) {
         ti->sScale /= mml->wScale;
         ti->tScale /= mml->hScale;
      }
   }

   ti->baseLevelInternalFormat = tObj->Image[0][minl]->Format;

   ti->validated = GL_TRUE;

   ti->info.data = NULL;
}

static void
fxPrintUnitsMode(const char *msg, GLuint mode)
{
   fprintf(stderr,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   msg,
	   mode,
	   (mode & FX_UM_E0_REPLACE) ? "E0_REPLACE, " : "",
	   (mode & FX_UM_E0_MODULATE) ? "E0_MODULATE, " : "",
	   (mode & FX_UM_E0_DECAL) ? "E0_DECAL, " : "",
	   (mode & FX_UM_E0_BLEND) ? "E0_BLEND, " : "",
	   (mode & FX_UM_E1_REPLACE) ? "E1_REPLACE, " : "",
	   (mode & FX_UM_E1_MODULATE) ? "E1_MODULATE, " : "",
	   (mode & FX_UM_E1_DECAL) ? "E1_DECAL, " : "",
	   (mode & FX_UM_E1_BLEND) ? "E1_BLEND, " : "",
	   (mode & FX_UM_E0_ALPHA) ? "E0_ALPHA, " : "",
	   (mode & FX_UM_E0_LUMINANCE) ? "E0_LUMINANCE, " : "",
	   (mode & FX_UM_E0_LUMINANCE_ALPHA) ? "E0_LUMINANCE_ALPHA, " : "",
	   (mode & FX_UM_E0_INTENSITY) ? "E0_INTENSITY, " : "",
	   (mode & FX_UM_E0_RGB) ? "E0_RGB, " : "",
	   (mode & FX_UM_E0_RGBA) ? "E0_RGBA, " : "",
	   (mode & FX_UM_E1_ALPHA) ? "E1_ALPHA, " : "",
	   (mode & FX_UM_E1_LUMINANCE) ? "E1_LUMINANCE, " : "",
	   (mode & FX_UM_E1_LUMINANCE_ALPHA) ? "E1_LUMINANCE_ALPHA, " : "",
	   (mode & FX_UM_E1_INTENSITY) ? "E1_INTENSITY, " : "",
	   (mode & FX_UM_E1_RGB) ? "E1_RGB, " : "",
	   (mode & FX_UM_E1_RGBA) ? "E1_RGBA, " : "",
	   (mode & FX_UM_COLOR_ITERATED) ? "COLOR_ITERATED, " : "",
	   (mode & FX_UM_COLOR_CONSTANT) ? "COLOR_CONSTANT, " : "",
	   (mode & FX_UM_ALPHA_ITERATED) ? "ALPHA_ITERATED, " : "",
	   (mode & FX_UM_ALPHA_CONSTANT) ? "ALPHA_CONSTANT, " : "");
}

static GLuint
fxGetTexSetConfiguration(GLcontext * ctx,
			 struct gl_texture_object *tObj0,
			 struct gl_texture_object *tObj1)
{
   GLuint unitsmode = 0;
   GLuint envmode = 0;
   GLuint ifmt = 0;

   if ((ctx->Light.ShadeModel == GL_SMOOTH) || 1 ||
       (ctx->Point.SmoothFlag) ||
       (ctx->Line.SmoothFlag) ||
       (ctx->Polygon.SmoothFlag)) unitsmode |= FX_UM_ALPHA_ITERATED;
   else
      unitsmode |= FX_UM_ALPHA_CONSTANT;

   if (ctx->Light.ShadeModel == GL_SMOOTH || 1)
      unitsmode |= FX_UM_COLOR_ITERATED;
   else
      unitsmode |= FX_UM_COLOR_CONSTANT;



   /* 
      OpenGL Feeds Texture 0 into Texture 1
      Glide Feeds Texture 1 into Texture 0
    */
   if (tObj0) {
      tfxTexInfo *ti0 = fxTMGetTexInfo(tObj0);

      switch (ti0->baseLevelInternalFormat) {
      case GL_ALPHA:
	 ifmt |= FX_UM_E0_ALPHA;
	 break;
      case GL_LUMINANCE:
	 ifmt |= FX_UM_E0_LUMINANCE;
	 break;
      case GL_LUMINANCE_ALPHA:
	 ifmt |= FX_UM_E0_LUMINANCE_ALPHA;
	 break;
      case GL_INTENSITY:
	 ifmt |= FX_UM_E0_INTENSITY;
	 break;
      case GL_RGB:
	 ifmt |= FX_UM_E0_RGB;
	 break;
      case GL_RGBA:
	 ifmt |= FX_UM_E0_RGBA;
	 break;
      }

      switch (ctx->Texture.Unit[0].EnvMode) {
      case GL_DECAL:
	 envmode |= FX_UM_E0_DECAL;
	 break;
      case GL_MODULATE:
	 envmode |= FX_UM_E0_MODULATE;
	 break;
      case GL_REPLACE:
	 envmode |= FX_UM_E0_REPLACE;
	 break;
      case GL_BLEND:
	 envmode |= FX_UM_E0_BLEND;
	 break;
      case GL_ADD:
	 envmode |= FX_UM_E0_ADD;
	 break;
      default:
	 /* do nothing */
	 break;
      }
   }

   if (tObj1) {
      tfxTexInfo *ti1 = fxTMGetTexInfo(tObj1);

      switch (ti1->baseLevelInternalFormat) {
      case GL_ALPHA:
	 ifmt |= FX_UM_E1_ALPHA;
	 break;
      case GL_LUMINANCE:
	 ifmt |= FX_UM_E1_LUMINANCE;
	 break;
      case GL_LUMINANCE_ALPHA:
	 ifmt |= FX_UM_E1_LUMINANCE_ALPHA;
	 break;
      case GL_INTENSITY:
	 ifmt |= FX_UM_E1_INTENSITY;
	 break;
      case GL_RGB:
	 ifmt |= FX_UM_E1_RGB;
	 break;
      case GL_RGBA:
	 ifmt |= FX_UM_E1_RGBA;
	 break;
      default:
	 /* do nothing */
	 break;
      }

      switch (ctx->Texture.Unit[1].EnvMode) {
      case GL_DECAL:
	 envmode |= FX_UM_E1_DECAL;
	 break;
      case GL_MODULATE:
	 envmode |= FX_UM_E1_MODULATE;
	 break;
      case GL_REPLACE:
	 envmode |= FX_UM_E1_REPLACE;
	 break;
      case GL_BLEND:
	 envmode |= FX_UM_E1_BLEND;
	 break;
      case GL_ADD:
	 envmode |= FX_UM_E1_ADD;
	 break;
      default:
	 /* do nothing */
	 break;
      }
   }

   unitsmode |= (ifmt | envmode);

   if (TDFX_DEBUG & (VERBOSE_DRIVER | VERBOSE_TEXTURE))
      fxPrintUnitsMode("fxGetTexSetConfiguration", unitsmode);

   return unitsmode;
}

/************************************************************************/
/************************* Rendering Mode SetUp *************************/
/************************************************************************/

/************************* Single Texture Set ***************************/

static void
fxSetupSingleTMU_NoLock(fxMesaContext fxMesa, struct gl_texture_object *tObj)
{
   tfxTexInfo *ti = fxTMGetTexInfo(tObj);
   int tmu;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupSingleTMU_NoLock(%p (%d))\n", (void *)tObj, tObj->Name);
   }

   ti->lastTimeUsed = fxMesa->texBindNumber;

   /* Make sure we're not loaded incorrectly */
   if (ti->isInTM) {
      if (ti->LODblend) {
	 if (ti->whichTMU != FX_TMU_SPLIT)
	    fxTMMoveOutTM(fxMesa, tObj);
      }
      else {
	 if (ti->whichTMU == FX_TMU_SPLIT)
	    fxTMMoveOutTM(fxMesa, tObj);
      }
   }

   /* Make sure we're loaded correctly */
   if (!ti->isInTM) {
      if (ti->LODblend)
	 fxTMMoveInTM_NoLock(fxMesa, tObj, FX_TMU_SPLIT);
      else {
	 if (fxMesa->haveTwoTMUs) {
            if (fxTMCheckStartAddr(fxMesa, FX_TMU0, ti)) {
	       fxTMMoveInTM_NoLock(fxMesa, tObj, FX_TMU0);
	    }
	    else {
	       fxTMMoveInTM_NoLock(fxMesa, tObj, FX_TMU1);
	    }
	 }
	 else
	    fxTMMoveInTM_NoLock(fxMesa, tObj, FX_TMU0);
      }
   }

   if (ti->LODblend && ti->whichTMU == FX_TMU_SPLIT) {
      /* broadcast */
      if ((ti->info.format == GR_TEXFMT_P_8)
	  && (!fxMesa->haveGlobalPaletteTexture)) {
	 if (TDFX_DEBUG & VERBOSE_DRIVER) {
	    fprintf(stderr, "fxSetupSingleTMU_NoLock: uploading texture palette\n");
	 }
	 grTexDownloadTable(ti->paltype, &(ti->palette));
      }

      grTexClampMode(GR_TMU0, ti->sClamp, ti->tClamp);
      grTexClampMode(GR_TMU1, ti->sClamp, ti->tClamp);
      grTexFilterMode(GR_TMU0, ti->minFilt, ti->maxFilt);
      grTexFilterMode(GR_TMU1, ti->minFilt, ti->maxFilt);
      grTexMipMapMode(GR_TMU0, ti->mmMode, ti->LODblend);
      grTexMipMapMode(GR_TMU1, ti->mmMode, ti->LODblend);

      grTexSource(GR_TMU0, ti->tm[FX_TMU0]->startAddr,
			    GR_MIPMAPLEVELMASK_ODD, &(ti->info));
      grTexSource(GR_TMU1, ti->tm[FX_TMU1]->startAddr,
			    GR_MIPMAPLEVELMASK_EVEN, &(ti->info));
   }
   else {
      if (ti->whichTMU == FX_TMU_BOTH)
	 tmu = FX_TMU0;
      else
	 tmu = ti->whichTMU;

      /* pointcast */
      if ((ti->info.format == GR_TEXFMT_P_8)
	  && (!fxMesa->haveGlobalPaletteTexture)) {
	 if (TDFX_DEBUG & VERBOSE_DRIVER) {
	    fprintf(stderr, "fxSetupSingleTMU_NoLock: uploading texture palette\n");
	 }
	 fxMesa->Glide.grTexDownloadTableExt(tmu, ti->paltype, &(ti->palette));
      }

      /* KW: The alternative is to do the download to the other tmu.  If
       * we get to this point, I think it means we are thrashing the
       * texture memory, so perhaps it's not a good idea.  
       */
      if (ti->LODblend && (TDFX_DEBUG & VERBOSE_DRIVER)) {
	 fprintf(stderr, "fxSetupSingleTMU_NoLock: not blending texture - only one tmu\n");
      }

      grTexClampMode(tmu, ti->sClamp, ti->tClamp);
      grTexFilterMode(tmu, ti->minFilt, ti->maxFilt);
      grTexMipMapMode(tmu, ti->mmMode, FXFALSE);

      grTexSource(tmu, ti->tm[tmu]->startAddr, GR_MIPMAPLEVELMASK_BOTH, &(ti->info));
   }
}

static void
fxSelectSingleTMUSrc_NoLock(fxMesaContext fxMesa, GLint tmu, FxBool LODblend)
{
   struct tdfx_texcombine tex0, tex1;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSelectSingleTMUSrc_NoLock(%d, %d)\n", tmu, LODblend);
   }

   tex0.InvertRGB     = FXFALSE;
   tex0.InvertAlpha   = FXFALSE;
   tex1.InvertRGB     = FXFALSE;
   tex1.InvertAlpha   = FXFALSE;

   if (LODblend) {
      tex0.FunctionRGB   = GR_COMBINE_FUNCTION_BLEND;
      tex0.FactorRGB     = GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION;
      tex0.FunctionAlpha = GR_COMBINE_FUNCTION_BLEND;
      tex0.FactorAlpha   = GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION;

      tex1.FunctionRGB   = GR_COMBINE_FUNCTION_LOCAL;
      tex1.FactorRGB     = GR_COMBINE_FACTOR_NONE;
      tex1.FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
      tex1.FactorAlpha   = GR_COMBINE_FACTOR_NONE;

      fxMesa->tmuSrc = FX_TMU_SPLIT;
   }
   else {
      if (tmu != FX_TMU1) {
         tex0.FunctionRGB   = GR_COMBINE_FUNCTION_LOCAL;
         tex0.FactorRGB     = GR_COMBINE_FACTOR_NONE;
         tex0.FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         tex0.FactorAlpha   = GR_COMBINE_FACTOR_NONE;

         tex1.FunctionRGB   = GR_COMBINE_FUNCTION_ZERO;
         tex1.FactorRGB     = GR_COMBINE_FACTOR_NONE;
         tex1.FunctionAlpha = GR_COMBINE_FUNCTION_ZERO;
         tex1.FactorAlpha   = GR_COMBINE_FACTOR_NONE;

	 fxMesa->tmuSrc = FX_TMU0;
      }
      else {
         tex1.FunctionRGB   = GR_COMBINE_FUNCTION_LOCAL;
         tex1.FactorRGB     = GR_COMBINE_FACTOR_NONE;
         tex1.FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         tex1.FactorAlpha   = GR_COMBINE_FACTOR_NONE;

	 /* correct values to set TMU0 in passthrough mode */
         tex0.FunctionRGB   = GR_COMBINE_FUNCTION_BLEND;
         tex0.FactorRGB     = GR_COMBINE_FACTOR_ONE;
         tex0.FunctionAlpha = GR_COMBINE_FUNCTION_BLEND;
         tex0.FactorAlpha   = GR_COMBINE_FACTOR_ONE;

	 fxMesa->tmuSrc = FX_TMU1;
      }
   }

   grTexCombine(GR_TMU0,
                tex0.FunctionRGB,
                tex0.FactorRGB,
                tex0.FunctionAlpha,
                tex0.FactorAlpha,
                tex0.InvertRGB,
                tex0.InvertAlpha);
   if (fxMesa->haveTwoTMUs) {
      grTexCombine(GR_TMU1,
                   tex1.FunctionRGB,
                   tex1.FactorRGB,
                   tex1.FunctionAlpha,
                   tex1.FactorAlpha,
                   tex1.InvertRGB,
                   tex1.InvertAlpha);
   }
}

static void
fxSetupTextureSingleTMU_NoLock(GLcontext * ctx, GLuint textureset)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   struct tdfx_combine alphaComb, colorComb;
   GrCombineLocal_t localc, locala;
   GLuint unitsmode;
   GLint ifmt;
   tfxTexInfo *ti;
   struct gl_texture_object *tObj = ctx->Texture.Unit[textureset]._Current;
   int tmu;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupTextureSingleTMU_NoLock(%d)\n", textureset);
   }

   ti = fxTMGetTexInfo(tObj);

   fxTexValidate(ctx, tObj);

   fxSetupSingleTMU_NoLock(fxMesa, tObj);

   if (ti->whichTMU == FX_TMU_BOTH)
      tmu = FX_TMU0;
   else
      tmu = ti->whichTMU;
   if (fxMesa->tmuSrc != tmu)
      fxSelectSingleTMUSrc_NoLock(fxMesa, tmu, ti->LODblend);

   if (textureset == 0 || !fxMesa->haveTwoTMUs)
      unitsmode = fxGetTexSetConfiguration(ctx, tObj, NULL);
   else
      unitsmode = fxGetTexSetConfiguration(ctx, NULL, tObj);

/*    if(fxMesa->lastUnitsMode==unitsmode) */
/*      return; */

   fxMesa->lastUnitsMode = unitsmode;

   fxMesa->stw_hint_state = 0;
   FX_grHints_NoLock(GR_HINT_STWHINT, 0);

   ifmt = ti->baseLevelInternalFormat;

   if (unitsmode & FX_UM_ALPHA_ITERATED)
      locala = GR_COMBINE_LOCAL_ITERATED;
   else
      locala = GR_COMBINE_LOCAL_CONSTANT;

   if (unitsmode & FX_UM_COLOR_ITERATED)
      localc = GR_COMBINE_LOCAL_ITERATED;
   else
      localc = GR_COMBINE_LOCAL_CONSTANT;

   if (TDFX_DEBUG & (VERBOSE_DRIVER | VERBOSE_TEXTURE))
      fprintf(stderr, "fxSetupTextureSingleTMU_NoLock: envmode is %s\n",
	      _mesa_lookup_enum_by_nr(ctx->Texture.Unit[textureset].EnvMode));

   alphaComb.Local    = locala;
   alphaComb.Invert   = FXFALSE;
   colorComb.Local    = localc;
   colorComb.Invert   = FXFALSE;

   switch (ctx->Texture.Unit[textureset].EnvMode) {
   case GL_DECAL:
      alphaComb.Function = GR_COMBINE_FUNCTION_LOCAL;
      alphaComb.Factor   = GR_COMBINE_FACTOR_NONE;
      alphaComb.Other    = GR_COMBINE_OTHER_NONE;

      colorComb.Function = GR_COMBINE_FUNCTION_BLEND;
      colorComb.Factor   = GR_COMBINE_FACTOR_TEXTURE_ALPHA;
      colorComb.Other    = GR_COMBINE_OTHER_TEXTURE;
      break;
   case GL_MODULATE:
      alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
      alphaComb.Factor   = GR_COMBINE_FACTOR_LOCAL;
      alphaComb.Other    = GR_COMBINE_OTHER_TEXTURE;

      if (ifmt == GL_ALPHA) {
         colorComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         colorComb.Factor   = GR_COMBINE_FACTOR_NONE;
         colorComb.Other    = GR_COMBINE_OTHER_NONE;
      } else {
         colorComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         colorComb.Factor   = GR_COMBINE_FACTOR_LOCAL;
         colorComb.Other    = GR_COMBINE_OTHER_TEXTURE;
      }
      break;
   case GL_BLEND:
      if (ifmt == GL_LUMINANCE || ifmt == GL_RGB) {
         /* Av = Af */
         alphaComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         alphaComb.Factor   = GR_COMBINE_FACTOR_NONE;
         alphaComb.Other    = GR_COMBINE_OTHER_NONE;
      }
      else if (ifmt == GL_INTENSITY) {
         /* Av = Af * (1 - It) + Ac * It */
         alphaComb.Function = GR_COMBINE_FUNCTION_BLEND;
         alphaComb.Factor   = GR_COMBINE_FACTOR_TEXTURE_ALPHA;
         alphaComb.Other    = GR_COMBINE_OTHER_CONSTANT;
      }
      else {
         /* Av = Af * At */
         alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         alphaComb.Factor   = GR_COMBINE_FACTOR_LOCAL;
         alphaComb.Other    = GR_COMBINE_OTHER_TEXTURE;
      }

      if (ifmt == GL_ALPHA) {
         colorComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         colorComb.Factor   = GR_COMBINE_FACTOR_NONE;
         colorComb.Other    = GR_COMBINE_OTHER_NONE;
      } else {
         if (fxMesa->type >= GR_SSTTYPE_Voodoo2) {
            colorComb.Function = GR_COMBINE_FUNCTION_BLEND;
            colorComb.Factor   = GR_COMBINE_FACTOR_TEXTURE_RGB;
            colorComb.Other    = GR_COMBINE_OTHER_CONSTANT;
         } else if (ifmt == GL_INTENSITY) {
            /* just a hack: RGB == ALPHA */
            colorComb.Function = GR_COMBINE_FUNCTION_BLEND;
            colorComb.Factor   = GR_COMBINE_FACTOR_TEXTURE_ALPHA;
            colorComb.Other    = GR_COMBINE_OTHER_CONSTANT;
         } else {
            /* Only Voodoo^2 can GL_BLEND (GR_COMBINE_FACTOR_TEXTURE_RGB)
             * These settings assume that the TexEnv color is black and
             * incoming fragment color is white.
             */
            colorComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
            colorComb.Factor   = GR_COMBINE_FACTOR_ONE;
            colorComb.Other    = GR_COMBINE_OTHER_TEXTURE;
            colorComb.Invert   = FXTRUE;
            _mesa_problem(NULL, "can't GL_BLEND with SST1");
         }
      }

      grConstantColorValue(
         (((GLuint)(ctx->Texture.Unit[textureset].EnvColor[0] * 255.0f))      ) |
         (((GLuint)(ctx->Texture.Unit[textureset].EnvColor[1] * 255.0f)) <<  8) |
         (((GLuint)(ctx->Texture.Unit[textureset].EnvColor[2] * 255.0f)) << 16) |
         (((GLuint)(ctx->Texture.Unit[textureset].EnvColor[3] * 255.0f)) << 24));
      break;
   case GL_REPLACE:
      if ((ifmt == GL_RGB) || (ifmt == GL_LUMINANCE)) {
         alphaComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         alphaComb.Factor   = GR_COMBINE_FACTOR_NONE;
         alphaComb.Other    = GR_COMBINE_OTHER_NONE;
      } else {
         alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         alphaComb.Factor   = GR_COMBINE_FACTOR_ONE;
         alphaComb.Other    = GR_COMBINE_OTHER_TEXTURE;
      }

      if (ifmt == GL_ALPHA) {
         colorComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         colorComb.Factor   = GR_COMBINE_FACTOR_NONE;
         colorComb.Other    = GR_COMBINE_OTHER_NONE;
      } else {
         colorComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         colorComb.Factor   = GR_COMBINE_FACTOR_ONE;
         colorComb.Other    = GR_COMBINE_OTHER_TEXTURE;
      }
      break;
   case GL_ADD:
      if (ifmt == GL_ALPHA ||
          ifmt == GL_LUMINANCE_ALPHA ||
          ifmt == GL_RGBA) {
         /* product of texel and fragment alpha */
         alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         alphaComb.Factor   = GR_COMBINE_FACTOR_LOCAL;
         alphaComb.Other    = GR_COMBINE_OTHER_TEXTURE;
      }
      else if (ifmt == GL_LUMINANCE || ifmt == GL_RGB) {
         /* fragment alpha is unchanged */
         alphaComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         alphaComb.Factor   = GR_COMBINE_FACTOR_NONE;
         alphaComb.Other    = GR_COMBINE_OTHER_NONE;
      }
      else {
         /* sum of texel and fragment alpha */
         alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
         alphaComb.Factor   = GR_COMBINE_FACTOR_ONE;
         alphaComb.Other    = GR_COMBINE_OTHER_TEXTURE;
      }

      if (ifmt == GL_ALPHA) {
         /* rgb unchanged */
         colorComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         colorComb.Factor   = GR_COMBINE_FACTOR_NONE;
         colorComb.Other    = GR_COMBINE_OTHER_NONE;
      }
      else {
         /* sum of texel and fragment rgb */
         colorComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
         colorComb.Factor   = GR_COMBINE_FACTOR_ONE;
         colorComb.Other    = GR_COMBINE_OTHER_TEXTURE;
      }
      break;
   default:
      if (TDFX_DEBUG & VERBOSE_DRIVER) {
	 fprintf(stderr, "fxSetupTextureSingleTMU_NoLock: %x Texture.EnvMode not yet supported\n",
		 ctx->Texture.Unit[textureset].EnvMode);
      }
      return;
   }

   grAlphaCombine(alphaComb.Function,
                  alphaComb.Factor,
                  alphaComb.Local,
                  alphaComb.Other,
                  alphaComb.Invert);
   grColorCombine(colorComb.Function,
                  colorComb.Factor,
                  colorComb.Local,
                  colorComb.Other,
                  colorComb.Invert);
}

#if 00
static void
fxSetupTextureSingleTMU(GLcontext * ctx, GLuint textureset)
{
   BEGIN_BOARD_LOCK();
   fxSetupTextureSingleTMU_NoLock(ctx, textureset);
   END_BOARD_LOCK();
}
#endif


/************************* Double Texture Set ***************************/

static void
fxSetupDoubleTMU_NoLock(fxMesaContext fxMesa,
			struct gl_texture_object *tObj0,
			struct gl_texture_object *tObj1)
{
#define T0_NOT_IN_TMU  0x01
#define T1_NOT_IN_TMU  0x02
#define T0_IN_TMU0     0x04
#define T1_IN_TMU0     0x08
#define T0_IN_TMU1     0x10
#define T1_IN_TMU1     0x20

   tfxTexInfo *ti0 = fxTMGetTexInfo(tObj0);
   tfxTexInfo *ti1 = fxTMGetTexInfo(tObj1);
   GLuint tstate = 0;
   int tmu0 = 0, tmu1 = 1;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupDoubleTMU_NoLock(...)\n");
   }

   /* We shouldn't need to do this. There is something wrong with
      mutlitexturing when the TMUs are swapped. So, we're forcing
      them to always be loaded correctly. !!! */
   if (ti0->whichTMU == FX_TMU1)
      fxTMMoveOutTM_NoLock(fxMesa, tObj0);
   if (ti1->whichTMU == FX_TMU0)
      fxTMMoveOutTM_NoLock(fxMesa, tObj1);

   if (ti0->isInTM) {
      switch (ti0->whichTMU) {
      case FX_TMU0:
	 tstate |= T0_IN_TMU0;
	 break;
      case FX_TMU1:
	 tstate |= T0_IN_TMU1;
	 break;
      case FX_TMU_BOTH:
	 tstate |= T0_IN_TMU0 | T0_IN_TMU1;
	 break;
      case FX_TMU_SPLIT:
	 tstate |= T0_NOT_IN_TMU;
	 break;
      }
   }
   else
      tstate |= T0_NOT_IN_TMU;

   if (ti1->isInTM) {
      switch (ti1->whichTMU) {
      case FX_TMU0:
	 tstate |= T1_IN_TMU0;
	 break;
      case FX_TMU1:
	 tstate |= T1_IN_TMU1;
	 break;
      case FX_TMU_BOTH:
	 tstate |= T1_IN_TMU0 | T1_IN_TMU1;
	 break;
      case FX_TMU_SPLIT:
	 tstate |= T1_NOT_IN_TMU;
	 break;
      }
   }
   else
      tstate |= T1_NOT_IN_TMU;

   ti0->lastTimeUsed = fxMesa->texBindNumber;
   ti1->lastTimeUsed = fxMesa->texBindNumber;

   /* Move texture maps into TMUs */

   if (!(((tstate & T0_IN_TMU0) && (tstate & T1_IN_TMU1)) ||
	 ((tstate & T0_IN_TMU1) && (tstate & T1_IN_TMU0)))) {
      if (tObj0 == tObj1)
	 fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU_BOTH);
      else {
	 /* Find the minimal way to correct the situation */
	 if ((tstate & T0_IN_TMU0) || (tstate & T1_IN_TMU1)) {
	    /* We have one in the standard order, setup the other */
	    if (tstate & T0_IN_TMU0) {	/* T0 is in TMU0, put T1 in TMU1 */
	       fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU1);
	    }
	    else {
	       fxTMMoveInTM_NoLock(fxMesa, tObj0, FX_TMU0);
	    }
	    /* tmu0 and tmu1 are setup */
	 }
	 else if ((tstate & T0_IN_TMU1) || (tstate & T1_IN_TMU0)) {
	    /* we have one in the reverse order, setup the other */
	    if (tstate & T1_IN_TMU0) {	/* T1 is in TMU0, put T0 in TMU1 */
	       fxTMMoveInTM_NoLock(fxMesa, tObj0, FX_TMU1);
	    }
	    else {
	       fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU0);
	    }
	    tmu0 = 1;
	    tmu1 = 0;
	 }
	 else {			/* Nothing is loaded */
	    fxTMMoveInTM_NoLock(fxMesa, tObj0, FX_TMU0);
	    fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU1);
	    /* tmu0 and tmu1 are setup */
	 }
      }
   }

   /* [dBorca] Hack alert:
    * we put these in reverse order, so that if we can't
    * do _REAL_ pointcast, the TMU0 table gets broadcasted
    */
   if (!fxMesa->haveGlobalPaletteTexture) {
      /* pointcast */
      if (ti1->info.format == GR_TEXFMT_P_8) {
	 if (TDFX_DEBUG & VERBOSE_DRIVER) {
	    fprintf(stderr, "fxSetupDoubleTMU_NoLock: uploading texture palette for TMU1\n");
	 }
	 fxMesa->Glide.grTexDownloadTableExt(ti1->whichTMU, ti1->paltype, &(ti1->palette));
      }
      if (ti0->info.format == GR_TEXFMT_P_8) {
	 if (TDFX_DEBUG & VERBOSE_DRIVER) {
	    fprintf(stderr, "fxSetupDoubleTMU_NoLock: uploading texture palette for TMU0\n");
	 }
	 fxMesa->Glide.grTexDownloadTableExt(ti0->whichTMU, ti0->paltype, &(ti0->palette));
      }
   }

   grTexSource(tmu0, ti0->tm[tmu0]->startAddr,
			 GR_MIPMAPLEVELMASK_BOTH, &(ti0->info));
   grTexClampMode(tmu0, ti0->sClamp, ti0->tClamp);
   grTexFilterMode(tmu0, ti0->minFilt, ti0->maxFilt);
   grTexMipMapMode(tmu0, ti0->mmMode, FXFALSE);

   grTexSource(tmu1, ti1->tm[tmu1]->startAddr,
			 GR_MIPMAPLEVELMASK_BOTH, &(ti1->info));
   grTexClampMode(tmu1, ti1->sClamp, ti1->tClamp);
   grTexFilterMode(tmu1, ti1->minFilt, ti1->maxFilt);
   grTexMipMapMode(tmu1, ti1->mmMode, FXFALSE);

#undef T0_NOT_IN_TMU
#undef T1_NOT_IN_TMU
#undef T0_IN_TMU0
#undef T1_IN_TMU0
#undef T0_IN_TMU1
#undef T1_IN_TMU1
}

static void
fxSetupTextureDoubleTMU_NoLock(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   struct tdfx_combine alphaComb, colorComb;
   struct tdfx_texcombine tex0, tex1;
   GrCombineLocal_t localc, locala;
   tfxTexInfo *ti0, *ti1;
   struct gl_texture_object *tObj0 = ctx->Texture.Unit[1]._Current;
   struct gl_texture_object *tObj1 = ctx->Texture.Unit[0]._Current;
   GLuint envmode, ifmt, unitsmode;
   int tmu0 = 0, tmu1 = 1;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupTextureDoubleTMU_NoLock(...)\n");
   }

   ti0 = fxTMGetTexInfo(tObj0);
   fxTexValidate(ctx, tObj0);

   ti1 = fxTMGetTexInfo(tObj1);
   fxTexValidate(ctx, tObj1);

   fxSetupDoubleTMU_NoLock(fxMesa, tObj0, tObj1);

   unitsmode = fxGetTexSetConfiguration(ctx, tObj0, tObj1);

/*    if(fxMesa->lastUnitsMode==unitsmode) */
/*      return; */

   fxMesa->lastUnitsMode = unitsmode;

   fxMesa->stw_hint_state |= GR_STWHINT_ST_DIFF_TMU1;
   FX_grHints_NoLock(GR_HINT_STWHINT, fxMesa->stw_hint_state);

   envmode = unitsmode & FX_UM_E_ENVMODE;
   ifmt = unitsmode & FX_UM_E_IFMT;

   if (unitsmode & FX_UM_ALPHA_ITERATED)
      locala = GR_COMBINE_LOCAL_ITERATED;
   else
      locala = GR_COMBINE_LOCAL_CONSTANT;

   if (unitsmode & FX_UM_COLOR_ITERATED)
      localc = GR_COMBINE_LOCAL_ITERATED;
   else
      localc = GR_COMBINE_LOCAL_CONSTANT;


   if (TDFX_DEBUG & (VERBOSE_DRIVER | VERBOSE_TEXTURE))
      fprintf(stderr, "fxSetupTextureDoubleTMU_NoLock: envmode is %s/%s\n",
	      _mesa_lookup_enum_by_nr(ctx->Texture.Unit[0].EnvMode),
	      _mesa_lookup_enum_by_nr(ctx->Texture.Unit[1].EnvMode));


   if ((ti0->whichTMU == FX_TMU1) || (ti1->whichTMU == FX_TMU0)) {
      tmu0 = 1;
      tmu1 = 0;
   }
   fxMesa->tmuSrc = FX_TMU_BOTH;

   tex0.InvertRGB     = FXFALSE;
   tex0.InvertAlpha   = FXFALSE;
   tex1.InvertRGB     = FXFALSE;
   tex1.InvertAlpha   = FXFALSE;
   alphaComb.Local    = locala;
   alphaComb.Invert   = FXFALSE;
   colorComb.Local    = localc;
   colorComb.Invert   = FXFALSE;

   switch (envmode) {
   case (FX_UM_E0_MODULATE | FX_UM_E1_MODULATE):
      {
	 GLboolean isalpha[FX_NUM_TMU];

	 isalpha[tmu0] = (ti0->baseLevelInternalFormat == GL_ALPHA);
	 isalpha[tmu1] = (ti1->baseLevelInternalFormat == GL_ALPHA);

	 if (isalpha[FX_TMU1]) {
            tex1.FunctionRGB   = GR_COMBINE_FUNCTION_ZERO;
            tex1.FactorRGB     = GR_COMBINE_FACTOR_NONE;
            tex1.FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
            tex1.FactorAlpha   = GR_COMBINE_FACTOR_NONE;
            tex1.InvertRGB     = FXTRUE;
	 } else {
            tex1.FunctionRGB   = GR_COMBINE_FUNCTION_LOCAL;
            tex1.FactorRGB     = GR_COMBINE_FACTOR_NONE;
            tex1.FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
            tex1.FactorAlpha   = GR_COMBINE_FACTOR_NONE;
         }

	 if (isalpha[FX_TMU0]) {
            tex0.FunctionRGB   = GR_COMBINE_FUNCTION_BLEND_OTHER;
            tex0.FactorRGB     = GR_COMBINE_FACTOR_ONE;
            tex0.FunctionAlpha = GR_COMBINE_FUNCTION_BLEND_OTHER;
            tex0.FactorAlpha   = GR_COMBINE_FACTOR_LOCAL;
	 } else {
            tex0.FunctionRGB   = GR_COMBINE_FUNCTION_BLEND_OTHER;
            tex0.FactorRGB     = GR_COMBINE_FACTOR_LOCAL;
            tex0.FunctionAlpha = GR_COMBINE_FUNCTION_BLEND_OTHER;
            tex0.FactorAlpha   = GR_COMBINE_FACTOR_LOCAL;
         }

         colorComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         colorComb.Factor   = GR_COMBINE_FACTOR_LOCAL;
         colorComb.Other    = GR_COMBINE_OTHER_TEXTURE;

         alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         alphaComb.Factor   = GR_COMBINE_FACTOR_LOCAL;
         alphaComb.Other    = GR_COMBINE_OTHER_TEXTURE;
	 break;
      }
   case (FX_UM_E0_REPLACE | FX_UM_E1_BLEND):	/* Only for GLQuake */
      if (tmu0 == FX_TMU1) {
         tex1.FunctionRGB   = GR_COMBINE_FUNCTION_LOCAL;
         tex1.FactorRGB     = GR_COMBINE_FACTOR_NONE;
         tex1.FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         tex1.FactorAlpha   = GR_COMBINE_FACTOR_NONE;
         tex1.InvertRGB     = FXTRUE;

         tex0.FunctionRGB   = GR_COMBINE_FUNCTION_BLEND_OTHER;
         tex0.FactorRGB     = GR_COMBINE_FACTOR_LOCAL;
         tex0.FunctionAlpha = GR_COMBINE_FUNCTION_BLEND_OTHER;
         tex0.FactorAlpha   = GR_COMBINE_FACTOR_LOCAL;
      }
      else {
         tex1.FunctionRGB   = GR_COMBINE_FUNCTION_LOCAL;
         tex1.FactorRGB     = GR_COMBINE_FACTOR_NONE;
         tex1.FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         tex1.FactorAlpha   = GR_COMBINE_FACTOR_NONE;

         tex0.FunctionRGB   = GR_COMBINE_FUNCTION_BLEND_OTHER;
         tex0.FactorRGB     = GR_COMBINE_FACTOR_ONE_MINUS_LOCAL;
         tex0.FunctionAlpha = GR_COMBINE_FUNCTION_BLEND_OTHER;
         tex0.FactorAlpha   = GR_COMBINE_FACTOR_ONE_MINUS_LOCAL;
      }

      alphaComb.Function = GR_COMBINE_FUNCTION_LOCAL;
      alphaComb.Factor   = GR_COMBINE_FACTOR_NONE;
      alphaComb.Other    = GR_COMBINE_OTHER_NONE;

      colorComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
      colorComb.Factor   = GR_COMBINE_FACTOR_ONE;
      colorComb.Other    = GR_COMBINE_OTHER_TEXTURE;
      break;
   case (FX_UM_E0_REPLACE | FX_UM_E1_MODULATE):	/* Quake 2 and 3 */
      if (tmu1 == FX_TMU1) {
         tex1.FunctionRGB   = GR_COMBINE_FUNCTION_LOCAL;
         tex1.FactorRGB     = GR_COMBINE_FACTOR_NONE;
         tex1.FunctionAlpha = GR_COMBINE_FUNCTION_ZERO;
         tex1.FactorAlpha   = GR_COMBINE_FACTOR_NONE;
         tex1.InvertAlpha   = FXTRUE;

         tex0.FunctionRGB   = GR_COMBINE_FUNCTION_BLEND_OTHER;
         tex0.FactorRGB     = GR_COMBINE_FACTOR_LOCAL;
         tex0.FunctionAlpha = GR_COMBINE_FUNCTION_BLEND_OTHER;
         tex0.FactorAlpha   = GR_COMBINE_FACTOR_LOCAL;
      }
      else {
         tex1.FunctionRGB   = GR_COMBINE_FUNCTION_LOCAL;
         tex1.FactorRGB     = GR_COMBINE_FACTOR_NONE;
         tex1.FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         tex1.FactorAlpha   = GR_COMBINE_FACTOR_NONE;

         tex0.FunctionRGB   = GR_COMBINE_FUNCTION_BLEND_OTHER;
         tex0.FactorRGB     = GR_COMBINE_FACTOR_LOCAL;
         tex0.FunctionAlpha = GR_COMBINE_FUNCTION_BLEND_OTHER;
         tex0.FactorAlpha   = GR_COMBINE_FACTOR_ONE;
      }

      if (ti0->baseLevelInternalFormat == GL_RGB) {
         alphaComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         alphaComb.Factor   = GR_COMBINE_FACTOR_NONE;
         alphaComb.Other    = GR_COMBINE_OTHER_NONE;
      } else {
         alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         alphaComb.Factor   = GR_COMBINE_FACTOR_ONE;
         alphaComb.Other    = GR_COMBINE_OTHER_NONE;
      }

      colorComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
      colorComb.Factor   = GR_COMBINE_FACTOR_ONE;
      colorComb.Other    = GR_COMBINE_OTHER_TEXTURE;
      break;


   case (FX_UM_E0_MODULATE | FX_UM_E1_ADD):	/* Quake 3 Sky */
      {
	 GLboolean isalpha[FX_NUM_TMU];

	 isalpha[tmu0] = (ti0->baseLevelInternalFormat == GL_ALPHA);
	 isalpha[tmu1] = (ti1->baseLevelInternalFormat == GL_ALPHA);

	 if (isalpha[FX_TMU1]) {
            tex1.FunctionRGB   = GR_COMBINE_FUNCTION_ZERO;
            tex1.FactorRGB     = GR_COMBINE_FACTOR_NONE;
            tex1.FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
            tex1.FactorAlpha   = GR_COMBINE_FACTOR_NONE;
            tex1.InvertRGB     = FXTRUE;
	 } else {
            tex1.FunctionRGB   = GR_COMBINE_FUNCTION_LOCAL;
            tex1.FactorRGB     = GR_COMBINE_FACTOR_NONE;
            tex1.FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
            tex1.FactorAlpha   = GR_COMBINE_FACTOR_NONE;
         }

	 if (isalpha[FX_TMU0]) {
            tex0.FunctionRGB   = GR_COMBINE_FUNCTION_SCALE_OTHER;
            tex0.FactorRGB     = GR_COMBINE_FACTOR_ONE;
            tex0.FunctionAlpha = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
            tex0.FactorAlpha   = GR_COMBINE_FACTOR_ONE;
	 } else {
            tex0.FunctionRGB   = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
            tex0.FactorRGB     = GR_COMBINE_FACTOR_ONE;
            tex0.FunctionAlpha = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
            tex0.FactorAlpha   = GR_COMBINE_FACTOR_ONE;
         }

         colorComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         colorComb.Factor   = GR_COMBINE_FACTOR_LOCAL;
         colorComb.Other    = GR_COMBINE_OTHER_TEXTURE;

         alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         alphaComb.Factor   = GR_COMBINE_FACTOR_LOCAL;
         alphaComb.Other    = GR_COMBINE_OTHER_TEXTURE;
	 break;
      }

   case (FX_UM_E0_REPLACE | FX_UM_E1_ADD):	/* Vulpine Sky */
      {
	 GLboolean isalpha[FX_NUM_TMU];

	 isalpha[tmu0] = (ti0->baseLevelInternalFormat == GL_ALPHA);
	 isalpha[tmu1] = (ti1->baseLevelInternalFormat == GL_ALPHA);

	 if (isalpha[FX_TMU1]) {
            tex1.FunctionRGB   = GR_COMBINE_FUNCTION_ZERO;
            tex1.FactorRGB     = GR_COMBINE_FACTOR_NONE;
            tex1.FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
            tex1.FactorAlpha   = GR_COMBINE_FACTOR_NONE;
            tex1.InvertRGB     = FXTRUE;
	 } else {
            tex1.FunctionRGB   = GR_COMBINE_FUNCTION_LOCAL;
            tex1.FactorRGB     = GR_COMBINE_FACTOR_NONE;
            tex1.FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
            tex1.FactorAlpha   = GR_COMBINE_FACTOR_NONE;
         }

	 if (isalpha[FX_TMU0]) {
            tex0.FunctionRGB   = GR_COMBINE_FUNCTION_SCALE_OTHER;
            tex0.FactorRGB     = GR_COMBINE_FACTOR_ONE;
            tex0.FunctionAlpha = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
            tex0.FactorAlpha   = GR_COMBINE_FACTOR_ONE;
	 } else {
            tex0.FunctionRGB   = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
            tex0.FactorRGB     = GR_COMBINE_FACTOR_ONE;
            tex0.FunctionAlpha = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
            tex0.FactorAlpha   = GR_COMBINE_FACTOR_ONE;
         }

         colorComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         colorComb.Factor   = GR_COMBINE_FACTOR_ONE;
         colorComb.Other    = GR_COMBINE_OTHER_TEXTURE;

         alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         alphaComb.Factor   = GR_COMBINE_FACTOR_ONE;
         alphaComb.Other    = GR_COMBINE_OTHER_TEXTURE;
	 break;
      }

   case (FX_UM_E0_MODULATE | FX_UM_E1_REPLACE): /* Homeworld2 */
      {
         tex1.FunctionRGB   = GR_COMBINE_FUNCTION_ZERO;
         tex1.FactorRGB     = GR_COMBINE_FACTOR_NONE;
         tex1.FunctionAlpha = GR_COMBINE_FUNCTION_ZERO;
         tex1.FactorAlpha   = GR_COMBINE_FACTOR_NONE;

         tex0.FunctionRGB   = GR_COMBINE_FUNCTION_LOCAL;
         tex0.FactorRGB     = GR_COMBINE_FACTOR_NONE;
         tex0.FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         tex0.FactorAlpha   = GR_COMBINE_FACTOR_NONE;

         if (ifmt & (FX_UM_E0_RGB | FX_UM_E0_LUMINANCE)) {
            alphaComb.Function = GR_COMBINE_FUNCTION_LOCAL;
            alphaComb.Factor   = GR_COMBINE_FACTOR_NONE;
            alphaComb.Other    = GR_COMBINE_OTHER_NONE;
         } else {
            alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
            alphaComb.Factor   = GR_COMBINE_FACTOR_ONE;
            alphaComb.Other    = GR_COMBINE_OTHER_TEXTURE;
         }

         if (ifmt & FX_UM_E0_ALPHA) {
            colorComb.Function = GR_COMBINE_FUNCTION_LOCAL;
            colorComb.Factor   = GR_COMBINE_FACTOR_NONE;
            colorComb.Other    = GR_COMBINE_OTHER_NONE;
         } else {
            colorComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
            colorComb.Factor   = GR_COMBINE_FACTOR_ONE;
            colorComb.Other    = GR_COMBINE_OTHER_TEXTURE;
         }
         break;
      }
   default:
      fprintf(stderr, "fxSetupTextureDoubleTMU_NoLock: Unexpected dual texture mode encountered\n");
      return;
   }

   grAlphaCombine(alphaComb.Function,
                  alphaComb.Factor,
                  alphaComb.Local,
                  alphaComb.Other,
                  alphaComb.Invert);
   grColorCombine(colorComb.Function,
                  colorComb.Factor,
                  colorComb.Local,
                  colorComb.Other,
                  colorComb.Invert);
   grTexCombine(GR_TMU0,
                tex0.FunctionRGB,
                tex0.FactorRGB,
                tex0.FunctionAlpha,
                tex0.FactorAlpha,
                tex0.InvertRGB,
                tex0.InvertAlpha);
   grTexCombine(GR_TMU1,
                tex1.FunctionRGB,
                tex1.FactorRGB,
                tex1.FunctionAlpha,
                tex1.FactorAlpha,
                tex1.InvertRGB,
                tex1.InvertAlpha);
}

/************************* No Texture ***************************/

static void
fxSetupTextureNone_NoLock(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GrCombineLocal_t localc, locala;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupTextureNone_NoLock(...)\n");
   }

   if ((ctx->Light.ShadeModel == GL_SMOOTH) || 1 ||
       (ctx->Point.SmoothFlag) ||
       (ctx->Line.SmoothFlag) ||
       (ctx->Polygon.SmoothFlag)) locala = GR_COMBINE_LOCAL_ITERATED;
   else
      locala = GR_COMBINE_LOCAL_CONSTANT;

   if (ctx->Light.ShadeModel == GL_SMOOTH || 1)
      localc = GR_COMBINE_LOCAL_ITERATED;
   else
      localc = GR_COMBINE_LOCAL_CONSTANT;

   grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,
                  GR_COMBINE_FACTOR_NONE,
                  locala,
                  GR_COMBINE_OTHER_NONE,
                  FXFALSE);

   grColorCombine(GR_COMBINE_FUNCTION_LOCAL,
                  GR_COMBINE_FACTOR_NONE,
                  localc,
                  GR_COMBINE_OTHER_NONE,
                  FXFALSE);

   fxMesa->lastUnitsMode = FX_UM_NONE;
}

#include "fxsetup.h"

/************************************************************************/
/************************** Texture Mode SetUp **************************/
/************************************************************************/

static void
fxSetupTexture_NoLock(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupTexture_NoLock(...)\n");
   }

   if (fxMesa->HaveCmbExt) {
      /* Texture Combine, Color Combine and Alpha Combine. */
      if ((ctx->Texture.Unit[0]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT)) &&
          (ctx->Texture.Unit[1]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT)) &&
          fxMesa->haveTwoTMUs) {
         fxSetupTextureDoubleTMUNapalm_NoLock(ctx);
      }
      else if (ctx->Texture.Unit[0]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT)) {
         fxSetupTextureSingleTMUNapalm_NoLock(ctx, 0);
      }
      else if (ctx->Texture.Unit[1]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT)) {
         fxSetupTextureSingleTMUNapalm_NoLock(ctx, 1);
      }
      else {
         fxSetupTextureNoneNapalm_NoLock(ctx);
      }
   } else {
      /* Texture Combine, Color Combine and Alpha Combine. */
      if ((ctx->Texture.Unit[0]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT)) &&
          (ctx->Texture.Unit[1]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT)) &&
          fxMesa->haveTwoTMUs) {
         fxSetupTextureDoubleTMU_NoLock(ctx);
      }
      else if (ctx->Texture.Unit[0]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT)) {
         fxSetupTextureSingleTMU_NoLock(ctx, 0);
      }
      else if (ctx->Texture.Unit[1]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT)) {
         fxSetupTextureSingleTMU_NoLock(ctx, 1);
      }
      else {
         fxSetupTextureNone_NoLock(ctx);
      }
   }
}

void
fxSetupTexture(GLcontext * ctx)
{
   BEGIN_BOARD_LOCK();
   fxSetupTexture_NoLock(ctx);
   END_BOARD_LOCK();
}

/************************************************************************/
/**************************** Blend SetUp *******************************/
/************************************************************************/

void
fxDDBlendFuncSeparate(GLcontext * ctx, GLenum sfactor, GLenum dfactor, GLenum asfactor, GLenum adfactor)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxUnitsState *us = &fxMesa->unitsState;
   GLboolean isNapalm = (fxMesa->type >= GR_SSTTYPE_Voodoo4);
   GLboolean have32bpp = (fxMesa->colDepth == 32);
   GLboolean haveAlpha = fxMesa->haveHwAlpha;
   GrAlphaBlendFnc_t sfact, dfact, asfact, adfact;

   /*
    * 15/16 BPP alpha channel alpha blending modes
    *   0x0	AZERO		Zero
    *   0x4	AONE		One
    *
    * 32 BPP alpha channel alpha blending modes
    *   0x0	AZERO		Zero
    *   0x1	ASRC_ALPHA	Source alpha
    *   0x3	ADST_ALPHA	Destination alpha
    *   0x4	AONE		One
    *   0x5	AOMSRC_ALPHA	1 - Source alpha
    *   0x7	AOMDST_ALPHA	1 - Destination alpha
    *
    * If we don't have HW alpha buffer:
    *   DST_ALPHA == 1
    *   ONE_MINUS_DST_ALPHA == 0
    * Unsupported modes are:
    *   1 if used as src blending factor
    *   0 if used as dst blending factor
    */

   switch (sfactor) {
   case GL_ZERO:
      sfact = GR_BLEND_ZERO;
      break;
   case GL_ONE:
      sfact = GR_BLEND_ONE;
      break;
   case GL_DST_COLOR:
      sfact = GR_BLEND_DST_COLOR;
      break;
   case GL_ONE_MINUS_DST_COLOR:
      sfact = GR_BLEND_ONE_MINUS_DST_COLOR;
      break;
   case GL_SRC_ALPHA:
      sfact = GR_BLEND_SRC_ALPHA;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      sfact = GR_BLEND_ONE_MINUS_SRC_ALPHA;
      break;
   case GL_DST_ALPHA:
      sfact = haveAlpha ? GR_BLEND_DST_ALPHA : GR_BLEND_ONE/*bad*/;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      sfact = haveAlpha ? GR_BLEND_ONE_MINUS_DST_ALPHA : GR_BLEND_ZERO/*bad*/;
      break;
   case GL_SRC_ALPHA_SATURATE:
      sfact = GR_BLEND_ALPHA_SATURATE;
      break;
   case GL_SRC_COLOR:
      if (isNapalm) {
         sfact = GR_BLEND_SAME_COLOR_EXT;
         break;
      }
   case GL_ONE_MINUS_SRC_COLOR:
      if (isNapalm) {
         sfact = GR_BLEND_ONE_MINUS_SAME_COLOR_EXT;
         break;
      }
   default:
      sfact = GR_BLEND_ONE;
      break;
   }

   switch (asfactor) {
   case GL_ZERO:
      asfact = GR_BLEND_ZERO;
      break;
   case GL_ONE:
      asfact = GR_BLEND_ONE;
      break;
   case GL_SRC_COLOR:
   case GL_SRC_ALPHA:
      asfact = have32bpp ? GR_BLEND_SRC_ALPHA : GR_BLEND_ONE/*bad*/;
      break;
   case GL_ONE_MINUS_SRC_COLOR:
   case GL_ONE_MINUS_SRC_ALPHA:
      asfact = have32bpp ? GR_BLEND_ONE_MINUS_SRC_ALPHA : GR_BLEND_ONE/*bad*/;
      break;
   case GL_DST_COLOR:
   case GL_DST_ALPHA:
      asfact = (have32bpp && haveAlpha) ? GR_BLEND_DST_ALPHA : GR_BLEND_ONE/*bad*/;
      break;
   case GL_ONE_MINUS_DST_COLOR:
   case GL_ONE_MINUS_DST_ALPHA:
      asfact = (have32bpp && haveAlpha) ? GR_BLEND_ONE_MINUS_DST_ALPHA : GR_BLEND_ZERO/*bad*/;
      break;
   case GL_SRC_ALPHA_SATURATE:
      asfact = GR_BLEND_ONE;
      break;
   default:
      asfact = GR_BLEND_ONE;
      break;
   }

   switch (dfactor) {
   case GL_ZERO:
      dfact = GR_BLEND_ZERO;
      break;
   case GL_ONE:
      dfact = GR_BLEND_ONE;
      break;
   case GL_SRC_COLOR:
      dfact = GR_BLEND_SRC_COLOR;
      break;
   case GL_ONE_MINUS_SRC_COLOR:
      dfact = GR_BLEND_ONE_MINUS_SRC_COLOR;
      break;
   case GL_SRC_ALPHA:
      dfact = GR_BLEND_SRC_ALPHA;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      dfact = GR_BLEND_ONE_MINUS_SRC_ALPHA;
      break;
   case GL_DST_ALPHA:
      dfact = haveAlpha ? GR_BLEND_DST_ALPHA : GR_BLEND_ONE/*bad*/;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      dfact = haveAlpha ? GR_BLEND_ONE_MINUS_DST_ALPHA : GR_BLEND_ZERO/*bad*/;
      break;
   case GL_DST_COLOR:
      if (isNapalm) {
         dfact = GR_BLEND_SAME_COLOR_EXT;
         break;
      }
   case GL_ONE_MINUS_DST_COLOR:
      if (isNapalm) {
         dfact = GR_BLEND_ONE_MINUS_SAME_COLOR_EXT;
         break;
      }
   default:
      dfact = GR_BLEND_ZERO;
      break;
   }

   switch (adfactor) {
   case GL_ZERO:
      adfact = GR_BLEND_ZERO;
      break;
   case GL_ONE:
      adfact = GR_BLEND_ONE;
      break;
   case GL_SRC_COLOR:
   case GL_SRC_ALPHA:
      adfact = have32bpp ? GR_BLEND_SRC_ALPHA : GR_BLEND_ZERO/*bad*/;
      break;
   case GL_ONE_MINUS_SRC_COLOR:
   case GL_ONE_MINUS_SRC_ALPHA:
      adfact = have32bpp ? GR_BLEND_ONE_MINUS_SRC_ALPHA : GR_BLEND_ZERO/*bad*/;
      break;
   case GL_DST_COLOR:
   case GL_DST_ALPHA:
      adfact = (have32bpp && haveAlpha) ? GR_BLEND_DST_ALPHA : GR_BLEND_ONE/*bad*/;
      break;
   case GL_ONE_MINUS_DST_COLOR:
   case GL_ONE_MINUS_DST_ALPHA:
      adfact = (have32bpp && haveAlpha) ? GR_BLEND_ONE_MINUS_DST_ALPHA : GR_BLEND_ZERO/*bad*/;
      break;
   default:
      adfact = GR_BLEND_ZERO;
      break;
   }

   if ((sfact != us->blendSrcFuncRGB) || (asfact != us->blendSrcFuncAlpha)) {
      us->blendSrcFuncRGB = sfact;
      us->blendSrcFuncAlpha = asfact;
      fxMesa->new_state |= FX_NEW_BLEND;
   }

   if ((dfact != us->blendDstFuncRGB) || (adfact != us->blendDstFuncAlpha)) {
      us->blendDstFuncRGB = dfact;
      us->blendDstFuncAlpha = adfact;
      fxMesa->new_state |= FX_NEW_BLEND;
   }
}

void
fxDDBlendEquationSeparate(GLcontext * ctx, GLenum modeRGB, GLenum modeA)
{
 fxMesaContext fxMesa = FX_CONTEXT(ctx);
 tfxUnitsState *us = &fxMesa->unitsState;
 GrAlphaBlendOp_t q;

 switch (modeRGB) {
        case GL_FUNC_ADD:
             q = GR_BLEND_OP_ADD;
             break;
        case GL_FUNC_SUBTRACT:
             q = GR_BLEND_OP_SUB;
             break;
        case GL_FUNC_REVERSE_SUBTRACT:
             q = GR_BLEND_OP_REVSUB;
             break;
        default:
             q = us->blendEqRGB;
 }
 if (q != us->blendEqRGB) {
    us->blendEqRGB = q;
    fxMesa->new_state |= FX_NEW_BLEND;
 }

 switch (modeA) {
        case GL_FUNC_ADD:
             q = GR_BLEND_OP_ADD;
             break;
        case GL_FUNC_SUBTRACT:
             q = GR_BLEND_OP_SUB;
             break;
        case GL_FUNC_REVERSE_SUBTRACT:
             q = GR_BLEND_OP_REVSUB;
             break;
        default:
             q = us->blendEqAlpha;
 }
 if (q != us->blendEqAlpha) {
    us->blendEqAlpha = q;
    fxMesa->new_state |= FX_NEW_BLEND;
 }
}

void
fxSetupBlend(GLcontext * ctx)
{
 fxMesaContext fxMesa = FX_CONTEXT(ctx);
 tfxUnitsState *us = &fxMesa->unitsState;

 if (fxMesa->HavePixExt) {
    if (us->blendEnabled) {
       fxMesa->Glide.grAlphaBlendFunctionExt(us->blendSrcFuncRGB, us->blendDstFuncRGB,
                                             us->blendEqRGB,
                                             us->blendSrcFuncAlpha, us->blendDstFuncAlpha,
                                             us->blendEqAlpha);
    } else {
       fxMesa->Glide.grAlphaBlendFunctionExt(GR_BLEND_ONE, GR_BLEND_ZERO,
                                             GR_BLEND_OP_ADD,
                                             GR_BLEND_ONE, GR_BLEND_ZERO,
                                             GR_BLEND_OP_ADD);
    }
 } else {
    if (us->blendEnabled) {
       grAlphaBlendFunction(us->blendSrcFuncRGB, us->blendDstFuncRGB,
                            us->blendSrcFuncAlpha, us->blendDstFuncAlpha);
    } else {
       grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO,
                            GR_BLEND_ONE, GR_BLEND_ZERO);
    }
 }
}

/************************************************************************/
/************************** Alpha Test SetUp ****************************/
/************************************************************************/

void
fxDDAlphaFunc(GLcontext * ctx, GLenum func, GLfloat ref)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxUnitsState *us = &fxMesa->unitsState;

   if (
       (us->alphaTestFunc != func)
       ||
       (us->alphaTestRefValue != ref)
      ) {
      us->alphaTestFunc = func;
      us->alphaTestRefValue = ref;
      fxMesa->new_state |= FX_NEW_ALPHA;
   }
}

static void
fxSetupAlphaTest(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxUnitsState *us = &fxMesa->unitsState;

   if (us->alphaTestEnabled) {
      GrAlpha_t ref = (GLint) (us->alphaTestRefValue * 255.0);
      grAlphaTestFunction(us->alphaTestFunc - GL_NEVER + GR_CMP_NEVER);
      grAlphaTestReferenceValue(ref);
   }
   else
      grAlphaTestFunction(GR_CMP_ALWAYS);
}

/************************************************************************/
/************************** Depth Test SetUp ****************************/
/************************************************************************/

void
fxDDDepthFunc(GLcontext * ctx, GLenum func)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxUnitsState *us = &fxMesa->unitsState;

   if (us->depthTestFunc != func) {
      us->depthTestFunc = func;
      fxMesa->new_state |= FX_NEW_DEPTH;
   }
}

void
fxDDDepthMask(GLcontext * ctx, GLboolean flag)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxUnitsState *us = &fxMesa->unitsState;

   if (flag != us->depthMask) {
      us->depthMask = flag;
      fxMesa->new_state |= FX_NEW_DEPTH;
   }
}

void
fxSetupDepthTest(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxUnitsState *us = &fxMesa->unitsState;

   if (us->depthTestEnabled) {
      grDepthBufferFunction(us->depthTestFunc - GL_NEVER + GR_CMP_NEVER);
      grDepthMask(us->depthMask);
   }
   else {
      grDepthBufferFunction(GR_CMP_ALWAYS);
      grDepthMask(FXFALSE);
   }
}

/************************************************************************/
/************************** Stencil SetUp *******************************/
/************************************************************************/

static GrStencil_t convertGLStencilOp( GLenum op )
{
   switch ( op ) {
   case GL_KEEP:
      return GR_STENCILOP_KEEP;
   case GL_ZERO:
      return GR_STENCILOP_ZERO;
   case GL_REPLACE:
      return GR_STENCILOP_REPLACE;
   case GL_INCR:
      return GR_STENCILOP_INCR_CLAMP;
   case GL_DECR:
      return GR_STENCILOP_DECR_CLAMP;
   case GL_INVERT:
      return GR_STENCILOP_INVERT;
   case GL_INCR_WRAP_EXT:
      return GR_STENCILOP_INCR_WRAP;
   case GL_DECR_WRAP_EXT:
      return GR_STENCILOP_DECR_WRAP;
   default:
      _mesa_problem( NULL, "bad stencil op in convertGLStencilOp" );
   }
   return GR_STENCILOP_KEEP;   /* never get, silence compiler warning */
}

void
fxDDStencilFuncSeparate (GLcontext *ctx, GLenum face, GLenum func,
                         GLint ref, GLuint mask)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxUnitsState *us = &fxMesa->unitsState;

   if (ctx->Stencil.ActiveFace) {
      return;
   }

   if (
       (us->stencilFunction != func)
       ||
       (us->stencilRefValue != ref)
       ||
       (us->stencilValueMask != mask)
      ) {
      us->stencilFunction = func;
      us->stencilRefValue = ref;
      us->stencilValueMask = mask;
      fxMesa->new_state |= FX_NEW_STENCIL;
   }
}

void
fxDDStencilMaskSeparate (GLcontext *ctx, GLenum face, GLuint mask)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxUnitsState *us = &fxMesa->unitsState;

   if (ctx->Stencil.ActiveFace) {
      return;
   }

   if (us->stencilWriteMask != mask) {
      us->stencilWriteMask = mask;
      fxMesa->new_state |= FX_NEW_STENCIL;
   }
}

void
fxDDStencilOpSeparate (GLcontext *ctx, GLenum face, GLenum sfail,
                       GLenum zfail, GLenum zpass)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxUnitsState *us = &fxMesa->unitsState;

   if (ctx->Stencil.ActiveFace) {
      return;
   }

   if (
       (us->stencilFailFunc != sfail)
       ||
       (us->stencilZFailFunc != zfail)
       ||
       (us->stencilZPassFunc != zpass)
      ) {
      us->stencilFailFunc = sfail;
      us->stencilZFailFunc = zfail;
      us->stencilZPassFunc = zpass;
      fxMesa->new_state |= FX_NEW_STENCIL;
   }
}

void
fxSetupStencil (GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxUnitsState *us = &fxMesa->unitsState;

   if (us->stencilEnabled) {
      GrCmpFnc_t stencilFailFunc = GR_STENCILOP_KEEP;
      GrCmpFnc_t stencilZFailFunc = GR_STENCILOP_KEEP;
      GrCmpFnc_t stencilZPassFunc = GR_STENCILOP_KEEP;
      if (!fxMesa->multipass) {
         stencilFailFunc = convertGLStencilOp(us->stencilFailFunc);
         stencilZFailFunc = convertGLStencilOp(us->stencilZFailFunc);
         stencilZPassFunc = convertGLStencilOp(us->stencilZPassFunc);
      }
      grEnable(GR_STENCIL_MODE_EXT);
      fxMesa->Glide.grStencilOpExt(stencilFailFunc,
                                   stencilZFailFunc,
                                   stencilZPassFunc);
      fxMesa->Glide.grStencilFuncExt(us->stencilFunction - GL_NEVER + GR_CMP_NEVER,
                                     us->stencilRefValue,
                                     us->stencilValueMask);
      fxMesa->Glide.grStencilMaskExt(us->stencilWriteMask);
   } else {
      grDisable(GR_STENCIL_MODE_EXT);
   }
}

void
fxSetupStencilFace (GLcontext * ctx, GLint face)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxUnitsState *us = &fxMesa->unitsState;

   if (us->stencilEnabled) {
      GrCmpFnc_t stencilFailFunc = GR_STENCILOP_KEEP;
      GrCmpFnc_t stencilZFailFunc = GR_STENCILOP_KEEP;
      GrCmpFnc_t stencilZPassFunc = GR_STENCILOP_KEEP;
      if (!fxMesa->multipass) {
         stencilFailFunc = convertGLStencilOp(ctx->Stencil.FailFunc[face]);
         stencilZFailFunc = convertGLStencilOp(ctx->Stencil.ZFailFunc[face]);
         stencilZPassFunc = convertGLStencilOp(ctx->Stencil.ZPassFunc[face]);
      }
      grEnable(GR_STENCIL_MODE_EXT);
      fxMesa->Glide.grStencilOpExt(stencilFailFunc,
                                   stencilZFailFunc,
                                   stencilZPassFunc);
      fxMesa->Glide.grStencilFuncExt(ctx->Stencil.Function[face] - GL_NEVER + GR_CMP_NEVER,
                                     ctx->Stencil.Ref[face],
                                     ctx->Stencil.ValueMask[face]);
      fxMesa->Glide.grStencilMaskExt(ctx->Stencil.WriteMask[face]);
   } else {
      grDisable(GR_STENCIL_MODE_EXT);
   }
}

/************************************************************************/
/**************************** Color Mask SetUp **************************/
/************************************************************************/

void
fxDDColorMask(GLcontext * ctx,
	      GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   fxMesa->new_state |= FX_NEW_COLOR_MASK;
   (void) r;
   (void) g;
   (void) b;
   (void) a;
}

void
fxSetupColorMask(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   if (fxMesa->colDepth == 32) {
      /* 32bpp mode */
      fxMesa->Glide.grColorMaskExt(ctx->Color.ColorMask[RCOMP],
                                   ctx->Color.ColorMask[GCOMP],
                                   ctx->Color.ColorMask[BCOMP],
                                   ctx->Color.ColorMask[ACOMP] && fxMesa->haveHwAlpha);
   }
   else {
      /* 15/16 bpp mode */
      grColorMask(ctx->Color.ColorMask[RCOMP] |
                  ctx->Color.ColorMask[GCOMP] |
                  ctx->Color.ColorMask[BCOMP],
                  ctx->Color.ColorMask[ACOMP] && fxMesa->haveHwAlpha);
   }
}




/************************************************************************/
/**************************** Fog Mode SetUp ****************************/
/************************************************************************/

/*
 * This is called during state update in order to update the Glide fog state.
 */
static void
fxSetupFog(GLcontext * ctx)
{
   if (ctx->Fog.Enabled /*&& ctx->FogMode==FOG_FRAGMENT */ ) {
      fxMesaContext fxMesa = FX_CONTEXT(ctx);

      /* update fog color */
      GLubyte col[4];
      col[0] = (unsigned int) (255 * ctx->Fog.Color[0]);
      col[1] = (unsigned int) (255 * ctx->Fog.Color[1]);
      col[2] = (unsigned int) (255 * ctx->Fog.Color[2]);
      col[3] = (unsigned int) (255 * ctx->Fog.Color[3]);
      grFogColorValue(FXCOLOR4(col));

      if (fxMesa->fogTableMode != ctx->Fog.Mode ||
	  fxMesa->fogDensity != ctx->Fog.Density ||
	  fxMesa->fogStart != ctx->Fog.Start ||
	  fxMesa->fogEnd != ctx->Fog.End) {
	 /* reload the fog table */
	 switch (ctx->Fog.Mode) {
	 case GL_LINEAR:
	    guFogGenerateLinear(fxMesa->fogTable, ctx->Fog.Start,
				ctx->Fog.End);
	    if (fxMesa->fogTable[0] > 63) {
	       /* [dBorca] Hack alert:
	        * As per Glide3 Programming Guide:
	        * The difference between consecutive fog values
	        * must be less than 64.
	        */
	       fxMesa->fogTable[0] = 63;
	    }
	    break;
	 case GL_EXP:
	    guFogGenerateExp(fxMesa->fogTable, ctx->Fog.Density);
	    break;
	 case GL_EXP2:
	    guFogGenerateExp2(fxMesa->fogTable, ctx->Fog.Density);
	    break;
	 default:
	    ;
	 }
	 fxMesa->fogTableMode = ctx->Fog.Mode;
	 fxMesa->fogDensity = ctx->Fog.Density;
	 fxMesa->fogStart = ctx->Fog.Start;
	 fxMesa->fogEnd = ctx->Fog.End;
      }

      grFogTable(fxMesa->fogTable);
      if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT) {
         grVertexLayout(GR_PARAM_FOG_EXT, GR_VERTEX_FOG_OFFSET << 2,
                                          GR_PARAM_ENABLE);
         grFogMode(GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT);
      } else {
         grVertexLayout(GR_PARAM_FOG_EXT, GR_VERTEX_FOG_OFFSET << 2,
                                          GR_PARAM_DISABLE);
         grFogMode(GR_FOG_WITH_TABLE_ON_Q);
      }
   }
   else {
      grFogMode(GR_FOG_DISABLE);
   }
}

void
fxDDFogfv(GLcontext * ctx, GLenum pname, const GLfloat * params)
{
   FX_CONTEXT(ctx)->new_state |= FX_NEW_FOG;
   switch (pname) {
      case GL_FOG_COORDINATE_SOURCE_EXT: {
         GLenum p = (GLenum)*params;
         if (p == GL_FOG_COORDINATE_EXT) {
            _swrast_allow_vertex_fog(ctx, GL_TRUE);
            _swrast_allow_pixel_fog(ctx, GL_FALSE);
            _tnl_allow_vertex_fog( ctx, GL_TRUE);
            _tnl_allow_pixel_fog( ctx, GL_FALSE);
         } else {
            _swrast_allow_vertex_fog(ctx, GL_FALSE);
            _swrast_allow_pixel_fog(ctx, GL_TRUE);
            _tnl_allow_vertex_fog( ctx, GL_FALSE);
            _tnl_allow_pixel_fog( ctx, GL_TRUE);
         }
         break;
      }
      default:
         ;
   }
}

/************************************************************************/
/************************** Scissor Test SetUp **************************/
/************************************************************************/

/* This routine is used in managing the lock state, and therefore can't lock */
void
fxSetScissorValues(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   int xmin, xmax;
   int ymin, ymax;

   if (ctx->Scissor.Enabled) {
      xmin = ctx->Scissor.X;
      xmax = ctx->Scissor.X + ctx->Scissor.Width;
      ymin = ctx->Scissor.Y;
      ymax = ctx->Scissor.Y + ctx->Scissor.Height;

      if (xmin < 0)
         xmin = 0;
      if (xmax > fxMesa->width)
         xmax = fxMesa->width;
      if (ymin < fxMesa->screen_height - fxMesa->height)
         ymin = fxMesa->screen_height - fxMesa->height;
      if (ymax > fxMesa->screen_height - 0)
         ymax = fxMesa->screen_height - 0;
   }
   else {
      xmin = 0;
      ymin = 0;
      xmax = fxMesa->width;
      ymax = fxMesa->height;
   }

   fxMesa->clipMinX = xmin;
   fxMesa->clipMinY = ymin;
   fxMesa->clipMaxX = xmax;
   fxMesa->clipMaxY = ymax;
   grClipWindow(xmin, ymin, xmax, ymax);
}

void
fxSetupScissor(GLcontext * ctx)
{
   BEGIN_BOARD_LOCK();
   fxSetScissorValues(ctx);
   END_BOARD_LOCK();
}

void
fxDDScissor(GLcontext * ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   FX_CONTEXT(ctx)->new_state |= FX_NEW_SCISSOR;
}

/************************************************************************/
/*************************** Cull mode setup ****************************/
/************************************************************************/


void
fxDDCullFace(GLcontext * ctx, GLenum mode)
{
   (void) mode;
   FX_CONTEXT(ctx)->new_state |= FX_NEW_CULL;
}

void
fxDDFrontFace(GLcontext * ctx, GLenum mode)
{
   (void) mode;
   FX_CONTEXT(ctx)->new_state |= FX_NEW_CULL;
}


void
fxSetupCull(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GrCullMode_t mode = GR_CULL_DISABLE;

   if (ctx->Polygon.CullFlag && (fxMesa->raster_primitive == GL_TRIANGLES)) {
      switch (ctx->Polygon.CullFaceMode) {
      case GL_BACK:
	 if (ctx->Polygon.FrontFace == GL_CCW)
	    mode = GR_CULL_NEGATIVE;
	 else
	    mode = GR_CULL_POSITIVE;
	 break;
      case GL_FRONT:
	 if (ctx->Polygon.FrontFace == GL_CCW)
	    mode = GR_CULL_POSITIVE;
	 else
	    mode = GR_CULL_NEGATIVE;
	 break;
      case GL_FRONT_AND_BACK:
	 /* Handled as a fallback on triangles in tdfx_tris.c */
	 return;
      default:
	 ASSERT(0);
	 break;
      }
   }

   if (fxMesa->cullMode != mode) {
      fxMesa->cullMode = mode;
      grCullMode(mode);
   }
}


/************************************************************************/
/****************************** DD Enable ******************************/
/************************************************************************/

void
fxDDEnable(GLcontext * ctx, GLenum cap, GLboolean state)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxUnitsState *us = &fxMesa->unitsState;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "%s(%s)\n", state ? "fxDDEnable" : "fxDDDisable",
	      _mesa_lookup_enum_by_nr(cap));
   }

   switch (cap) {
   case GL_ALPHA_TEST:
      if (state != us->alphaTestEnabled) {
	 us->alphaTestEnabled = state;
	 fxMesa->new_state |= FX_NEW_ALPHA;
      }
      break;
   case GL_BLEND:
      if (state != us->blendEnabled) {
	 us->blendEnabled = state;
	 fxMesa->new_state |= FX_NEW_BLEND;
      }
      break;
   case GL_DEPTH_TEST:
      if (state != us->depthTestEnabled) {
	 us->depthTestEnabled = state;
	 fxMesa->new_state |= FX_NEW_DEPTH;
      }
      break;
   case GL_STENCIL_TEST:
      if (fxMesa->haveHwStencil && state != us->stencilEnabled) {
	 us->stencilEnabled = state;
	 fxMesa->new_state |= FX_NEW_STENCIL;
      }
      break;
   case GL_DITHER:
      if (state) {
	 grDitherMode(GR_DITHER_4x4);
      }
      else {
	 grDitherMode(GR_DITHER_DISABLE);
      }
      break;
   case GL_SCISSOR_TEST:
      fxMesa->new_state |= FX_NEW_SCISSOR;
      break;
   case GL_SHARED_TEXTURE_PALETTE_EXT:
      fxDDTexUseGlbPalette(ctx, state);
      break;
   case GL_FOG:
      fxMesa->new_state |= FX_NEW_FOG;
      break;
   case GL_CULL_FACE:
      fxMesa->new_state |= FX_NEW_CULL;
      break;
   case GL_LINE_SMOOTH:
   case GL_LINE_STIPPLE:
   case GL_POINT_SMOOTH:
   case GL_POLYGON_SMOOTH:
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
      fxMesa->new_state |= FX_NEW_TEXTURING;
      break;
   default:
      ;				/* XXX no-op? */
   }
}




/************************************************************************/
/************************** Changes to units state **********************/
/************************************************************************/


/* All units setup is handled under texture setup.
 */
void
fxDDShadeModel(GLcontext * ctx, GLenum mode)
{
   FX_CONTEXT(ctx)->new_state |= FX_NEW_TEXTURING;
}



/************************************************************************/
/****************************** Units SetUp *****************************/
/************************************************************************/
static void
fx_print_state_flags(const char *msg, GLuint flags)
{
   fprintf(stderr,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s\n",
	   msg,
	   flags,
	   (flags & FX_NEW_TEXTURING) ? "texture, " : "",
	   (flags & FX_NEW_BLEND) ? "blend, " : "",
	   (flags & FX_NEW_ALPHA) ? "alpha, " : "",
	   (flags & FX_NEW_FOG) ? "fog, " : "",
	   (flags & FX_NEW_SCISSOR) ? "scissor, " : "",
	   (flags & FX_NEW_COLOR_MASK) ? "colormask, " : "",
	   (flags & FX_NEW_CULL) ? "cull, " : "",
	   (flags & FX_NEW_STENCIL) ? "stencil, " : "");
}

void
fxSetupFXUnits(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLuint newstate = fxMesa->new_state;

   if (TDFX_DEBUG & VERBOSE_DRIVER)
      fx_print_state_flags("fxSetupFXUnits", newstate);

   if (newstate) {
      if (newstate & FX_NEW_TEXTURING)
	 fxSetupTexture(ctx);

      if (newstate & FX_NEW_BLEND)
	 fxSetupBlend(ctx);

      if (newstate & FX_NEW_ALPHA)
	 fxSetupAlphaTest(ctx);

      if (newstate & FX_NEW_DEPTH)
	 fxSetupDepthTest(ctx);

      if (newstate & FX_NEW_STENCIL)
	 fxSetupStencil(ctx);

      if (newstate & FX_NEW_FOG)
	 fxSetupFog(ctx);

      if (newstate & FX_NEW_SCISSOR)
	 fxSetupScissor(ctx);

      if (newstate & FX_NEW_COLOR_MASK)
	 fxSetupColorMask(ctx);

      if (newstate & FX_NEW_CULL)
	 fxSetupCull(ctx);

      fxMesa->new_state = 0;
   }
}



#else


/*
 * Need this to provide at least one external definition.
 */

extern int gl_fx_dummy_function_setup(void);
int
gl_fx_dummy_function_setup(void)
{
   return 0;
}

#endif /* FX */
