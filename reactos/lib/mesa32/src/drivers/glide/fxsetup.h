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
/* [dBorca] Hack alert:
 * this code belongs to fxsetup.c, but I didn't want to clutter
 * the original code with Napalm specifics, in order to keep things
 * clear -- especially for backward compatibility. I should have
 * put it into another .c file, but I didn't want to export so many
 * things...
 * The point is, Napalm uses a different technique for texture env.
 * SST1 Single texturing:
 *      setup standard grTexCombine
 *      fiddle with grColorCombine/grAlphaCombine
 * SST1 Multi texturing:
 *      fiddle with grTexCombine/grColorCombine/grAlphaCombine
 * Napalm Single texturing:
 *      setup standard grColorCombineExt/grAlphaCombineExt
 *      fiddle with grTexColorCombine/grTexAlphaCombine
 * Napalm Multi texturing:
 *      setup standard grColorCombineExt/grAlphaCombineExt
 *      fiddle with grTexColorCombine/grTexAlphaCombine
 */

/*
 * These macros are used below when handling COMBINE_EXT.
 */
#define TEXENV_OPERAND_INVERTED(operand)                            \
  (((operand) == GL_ONE_MINUS_SRC_ALPHA)                            \
   || ((operand) == GL_ONE_MINUS_SRC_COLOR))
#define TEXENV_OPERAND_ALPHA(operand)                               \
  (((operand) == GL_SRC_ALPHA) || ((operand) == GL_ONE_MINUS_SRC_ALPHA))
#define TEXENV_SETUP_ARG_A(param, source, operand, iteratedAlpha)   \
    switch (source) {                                               \
    case GL_TEXTURE:                                                \
        param = GR_CMBX_LOCAL_TEXTURE_ALPHA;                        \
        break;                                                      \
    case GL_CONSTANT_EXT:                                           \
        param = GR_CMBX_TMU_CALPHA;                                 \
        break;                                                      \
    case GL_PRIMARY_COLOR_EXT:                                      \
        param = GR_CMBX_ITALPHA;                                    \
        break;                                                      \
    case GL_PREVIOUS_EXT:                                           \
        param = iteratedAlpha;                                      \
        break;                                                      \
    default:                                                        \
       /*                                                           \
        * This is here just to keep from getting                    \
        * compiler warnings.                                        \
        */                                                          \
        param = GR_CMBX_ZERO;                                       \
        break;                                                      \
    }

#define TEXENV_SETUP_ARG_RGB(param, source, operand, iteratedColor, iteratedAlpha) \
    if (!TEXENV_OPERAND_ALPHA(operand)) {                           \
        switch (source) {                                           \
        case GL_TEXTURE:                                            \
            param = GR_CMBX_LOCAL_TEXTURE_RGB;                      \
            break;                                                  \
        case GL_CONSTANT_EXT:                                       \
            param = GR_CMBX_TMU_CCOLOR;                             \
            break;                                                  \
        case GL_PRIMARY_COLOR_EXT:                                  \
            param = GR_CMBX_ITRGB;                                  \
            break;                                                  \
        case GL_PREVIOUS_EXT:                                       \
            param = iteratedColor;                                  \
            break;                                                  \
        default:                                                    \
           /*                                                       \
            * This is here just to keep from getting                \
            * compiler warnings.                                    \
            */                                                      \
            param = GR_CMBX_ZERO;                                   \
            break;                                                  \
        }                                                           \
    } else {                                                        \
        switch (source) {                                           \
        case GL_TEXTURE:                                            \
            param = GR_CMBX_LOCAL_TEXTURE_ALPHA;                    \
            break;                                                  \
        case GL_CONSTANT_EXT:                                       \
            param = GR_CMBX_TMU_CALPHA;                             \
            break;                                                  \
        case GL_PRIMARY_COLOR_EXT:                                  \
            param = GR_CMBX_ITALPHA;                                \
            break;                                                  \
        case GL_PREVIOUS_EXT:                                       \
            param = iteratedAlpha;                                  \
            break;                                                  \
        default:                                                    \
           /*                                                       \
            * This is here just to keep from getting                \
            * compiler warnings.                                    \
            */                                                      \
            param = GR_CMBX_ZERO;                                   \
            break;                                                  \
        }                                                           \
    }

#define TEXENV_SETUP_MODE_RGB(param, operand)                       \
    switch (operand) {                                              \
    case GL_SRC_COLOR:                                              \
    case GL_SRC_ALPHA:                                              \
        param = GR_FUNC_MODE_X;                                     \
        break;                                                      \
    case GL_ONE_MINUS_SRC_ALPHA:                                    \
    case GL_ONE_MINUS_SRC_COLOR:                                    \
        param = GR_FUNC_MODE_ONE_MINUS_X;                           \
        break;                                                      \
    default:                                                        \
        param = GR_FUNC_MODE_ZERO;                                  \
        break;                                                      \
    }

#define TEXENV_SETUP_MODE_A(param, operand)                         \
    switch (operand) {                                              \
    case GL_SRC_ALPHA:                                              \
        param = GR_FUNC_MODE_X;                                     \
        break;                                                      \
    case GL_ONE_MINUS_SRC_ALPHA:                                    \
        param = GR_FUNC_MODE_ONE_MINUS_X;                           \
        break;                                                      \
    default:                                                        \
        param = GR_FUNC_MODE_ZERO;                                  \
        break;                                                      \
    }

static void
fxSetupTextureEnvNapalm_NoLock(GLcontext * ctx, GLuint textureset, GLuint tmu, GLboolean iterated)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[textureset];
   struct tdfx_combine_alpha_ext alphaComb;
   struct tdfx_combine_color_ext colorComb;
   const GLfloat *envColor = texUnit->EnvColor;
   GrCombineLocal_t localc, locala; /* fragmentColor/Alpha */
   GLint ifmt;
   tfxTexInfo *ti;
   struct gl_texture_object *tObj = texUnit->Current2D;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupTextureEnvNapalm_NoLock(unit %u, TMU %u, iterated %d)\n",
                      textureset, tmu, iterated);
   }

   ti = fxTMGetTexInfo(tObj);

   ifmt = ti->baseLevelInternalFormat;

   if (iterated) {
      /* we don't have upstream TMU */
      locala = GR_CMBX_ITALPHA;
      localc = GR_CMBX_ITRGB;
   } else {
      /* we have upstream TMU */
      locala = GR_CMBX_OTHER_TEXTURE_ALPHA;
      localc = GR_CMBX_OTHER_TEXTURE_RGB;
   }

   alphaComb.InvertD = FXFALSE;
   alphaComb.Shift   = 0;
   alphaComb.Invert  = FXFALSE;
   colorComb.InvertD = FXFALSE;
   colorComb.Shift   = 0;
   colorComb.Invert  = FXFALSE;

   switch (texUnit->EnvMode) {
   case GL_DECAL:
      alphaComb.SourceA = locala;
      alphaComb.ModeA   = GR_FUNC_MODE_X;
      alphaComb.SourceB = GR_CMBX_ZERO;
      alphaComb.ModeB   = GR_FUNC_MODE_X;
      alphaComb.SourceC = GR_CMBX_ZERO;
      alphaComb.InvertC = FXTRUE;
      alphaComb.SourceD = GR_CMBX_ZERO;

      colorComb.SourceA = GR_CMBX_LOCAL_TEXTURE_RGB;
      colorComb.ModeA   = GR_FUNC_MODE_X;
      colorComb.SourceB = localc;
      colorComb.ModeB   = GR_FUNC_MODE_NEGATIVE_X;
      colorComb.SourceC = GR_CMBX_LOCAL_TEXTURE_ALPHA;
      colorComb.InvertC = FXFALSE;
      colorComb.SourceD = GR_CMBX_B;
      break;
   case GL_MODULATE:
      if (ifmt == GL_LUMINANCE || ifmt == GL_RGB) {
         alphaComb.SourceA = locala;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_ZERO;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_ZERO;
         alphaComb.InvertC = FXTRUE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      } else {
         alphaComb.SourceA = locala;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_ZERO;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         alphaComb.InvertC = FXFALSE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      }

      if (ifmt == GL_ALPHA) {
         colorComb.SourceA = localc;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = GR_CMBX_ZERO;
         colorComb.ModeB   = GR_FUNC_MODE_X;
         colorComb.SourceC = GR_CMBX_ZERO;
         colorComb.InvertC = FXTRUE;
         colorComb.SourceD = GR_CMBX_ZERO;
      } else {
         colorComb.SourceA = localc;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = GR_CMBX_ZERO;
         colorComb.ModeB   = GR_FUNC_MODE_X;
         colorComb.SourceC = GR_CMBX_LOCAL_TEXTURE_RGB;
         colorComb.InvertC = FXFALSE;
         colorComb.SourceD = GR_CMBX_ZERO;
      }
      break;
   case GL_BLEND:
      if (ifmt == GL_INTENSITY) {
         alphaComb.SourceA = GR_CMBX_TMU_CALPHA;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = locala;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         alphaComb.InvertC = FXFALSE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      } else {
         alphaComb.SourceA = locala;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_ZERO;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         alphaComb.InvertC = FXFALSE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      }

      if (ifmt == GL_ALPHA) {
         colorComb.SourceA = localc;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = GR_CMBX_ZERO;
         colorComb.ModeB   = GR_FUNC_MODE_X;
         colorComb.SourceC = GR_CMBX_ZERO;
         colorComb.InvertC = FXTRUE;
         colorComb.SourceD = GR_CMBX_ZERO;
      } else {
         colorComb.SourceA = GR_CMBX_TMU_CCOLOR;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = localc;
         colorComb.ModeB   = GR_FUNC_MODE_NEGATIVE_X;
         colorComb.SourceC = GR_CMBX_LOCAL_TEXTURE_RGB;
         colorComb.InvertC = FXFALSE;
         colorComb.SourceD = GR_CMBX_B;
      }

      fxMesa->Glide.grConstantColorValueExt(tmu,
         (((GLuint)(envColor[0] * 255.0f))      ) |
         (((GLuint)(envColor[1] * 255.0f)) <<  8) |
         (((GLuint)(envColor[2] * 255.0f)) << 16) |
         (((GLuint)(envColor[3] * 255.0f)) << 24));
      break;
   case GL_REPLACE:
      if (ifmt == GL_LUMINANCE || ifmt == GL_RGB) {
         alphaComb.SourceA = locala;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_ZERO;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_ZERO;
         alphaComb.InvertC = FXTRUE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      } else {
         alphaComb.SourceA = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_ZERO;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_ZERO;
         alphaComb.InvertC = FXTRUE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      }

      if (ifmt == GL_ALPHA) {
         colorComb.SourceA = localc;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = GR_CMBX_ZERO;
         colorComb.ModeB   = GR_FUNC_MODE_X;
         colorComb.SourceC = GR_CMBX_ZERO;
         colorComb.InvertC = FXTRUE;
         colorComb.SourceD = GR_CMBX_ZERO;
      } else {
         colorComb.SourceA = GR_CMBX_LOCAL_TEXTURE_RGB;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = GR_CMBX_ZERO;
         colorComb.ModeB   = GR_FUNC_MODE_X;
         colorComb.SourceC = GR_CMBX_ZERO;
         colorComb.InvertC = FXTRUE;
         colorComb.SourceD = GR_CMBX_ZERO;
      }
      break;
   case GL_ADD:
      if (ifmt == GL_LUMINANCE || ifmt == GL_RGB) {
         alphaComb.SourceA = locala;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_ZERO;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_ZERO;
         alphaComb.InvertC = FXTRUE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      } else if (ifmt == GL_INTENSITY) {
         alphaComb.SourceA = locala;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_ZERO;
         alphaComb.InvertC = FXTRUE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      } else {
         alphaComb.SourceA = locala;
         alphaComb.ModeA   = GR_FUNC_MODE_X;
         alphaComb.SourceB = GR_CMBX_ZERO;
         alphaComb.ModeB   = GR_FUNC_MODE_X;
         alphaComb.SourceC = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         alphaComb.InvertC = FXFALSE;
         alphaComb.SourceD = GR_CMBX_ZERO;
      }

      if (ifmt == GL_ALPHA) {
         colorComb.SourceA = localc;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = GR_CMBX_ZERO;
         colorComb.ModeB   = GR_FUNC_MODE_X;
         colorComb.SourceC = GR_CMBX_ZERO;
         colorComb.InvertC = FXTRUE;
         colorComb.SourceD = GR_CMBX_ZERO;
      } else {
         colorComb.SourceA = localc;
         colorComb.ModeA   = GR_FUNC_MODE_X;
         colorComb.SourceB = GR_CMBX_LOCAL_TEXTURE_RGB;
         colorComb.ModeB   = GR_FUNC_MODE_X;
         colorComb.SourceC = GR_CMBX_ZERO;
         colorComb.InvertC = FXTRUE;
         colorComb.SourceD = GR_CMBX_ZERO;
      }
      break;
    /* COMBINE_EXT */
    case GL_COMBINE_EXT:
      /* [dBorca] Hack alert:
       * INCOMPLETE!!!
       */
      if (TDFX_DEBUG & (VERBOSE_DRIVER | VERBOSE_TEXTURE)) {
#if 1
         fprintf(stderr, "COMBINE_EXT: %s + %s\n",
	      _mesa_lookup_enum_by_nr(texUnit->CombineModeRGB),
	      _mesa_lookup_enum_by_nr(texUnit->CombineModeA));
#else
         fprintf(stderr, "Texture Unit %d\n", textureset);
         fprintf(stderr, "  GL_TEXTURE_ENV_MODE = %s\n", _mesa_lookup_enum_by_nr(texUnit->EnvMode));
         fprintf(stderr, "  GL_COMBINE_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineModeRGB));
         fprintf(stderr, "  GL_COMBINE_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineModeA));
         fprintf(stderr, "  GL_SOURCE0_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineSourceRGB[0]));
         fprintf(stderr, "  GL_SOURCE1_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineSourceRGB[1]));
         fprintf(stderr, "  GL_SOURCE2_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineSourceRGB[2]));
         fprintf(stderr, "  GL_SOURCE0_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineSourceA[0]));
         fprintf(stderr, "  GL_SOURCE1_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineSourceA[1]));
         fprintf(stderr, "  GL_SOURCE2_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineSourceA[2]));
         fprintf(stderr, "  GL_OPERAND0_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineOperandRGB[0]));
         fprintf(stderr, "  GL_OPERAND1_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineOperandRGB[1]));
         fprintf(stderr, "  GL_OPERAND2_RGB = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineOperandRGB[2]));
         fprintf(stderr, "  GL_OPERAND0_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineOperandA[0]));
         fprintf(stderr, "  GL_OPERAND1_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineOperandA[1]));
         fprintf(stderr, "  GL_OPERAND2_ALPHA = %s\n", _mesa_lookup_enum_by_nr(texUnit->CombineOperandA[2]));
         fprintf(stderr, "  GL_RGB_SCALE = %d\n", 1 << texUnit->CombineScaleShiftRGB);
         fprintf(stderr, "  GL_ALPHA_SCALE = %d\n", 1 << texUnit->CombineScaleShiftA);
         fprintf(stderr, "  GL_TEXTURE_ENV_COLOR = (%f, %f, %f, %f)\n", envColor[0], envColor[1], envColor[2], envColor[3]);
#endif
      }

      alphaComb.Shift   = texUnit->CombineScaleShiftA;
      colorComb.Shift   = texUnit->CombineScaleShiftRGB;

      switch (texUnit->CombineModeRGB) {
             case GL_MODULATE:
                  /* Arg0 * Arg1 == (A + 0) * C + 0 */
                  TEXENV_SETUP_ARG_RGB(colorComb.SourceA,
                                       texUnit->CombineSourceRGB[0],
                                       texUnit->CombineOperandRGB[0],
                                       localc, locala);
                  TEXENV_SETUP_MODE_RGB(colorComb.ModeA,
                                        texUnit->CombineOperandRGB[0]);
                  colorComb.SourceB = GR_CMBX_ZERO;
                  colorComb.ModeB   = GR_FUNC_MODE_ZERO;
                  TEXENV_SETUP_ARG_RGB(colorComb.SourceC,
                                       texUnit->CombineSourceRGB[1],
                                       texUnit->CombineOperandRGB[1],
                                       localc, locala);
                  colorComb.InvertC = TEXENV_OPERAND_INVERTED(
                                       texUnit->CombineOperandRGB[1]);
                  colorComb.SourceD = GR_CMBX_ZERO;
                  break;
             case GL_REPLACE:
                  /* Arg0 == (A + 0) * 1 + 0 */
                  TEXENV_SETUP_ARG_RGB(colorComb.SourceA,
                                       texUnit->CombineSourceRGB[0],
                                       texUnit->CombineOperandRGB[0],
                                       localc, locala);
                  TEXENV_SETUP_MODE_RGB(colorComb.ModeA,
                                        texUnit->CombineOperandRGB[0]);
                  colorComb.SourceB = GR_CMBX_ZERO;
                  colorComb.ModeB   = GR_FUNC_MODE_ZERO;
                  colorComb.SourceC = GR_CMBX_ZERO;
                  colorComb.InvertC = FXTRUE;
                  colorComb.SourceD = GR_CMBX_ZERO;
                  break;
             case GL_ADD:
                  /* Arg0 + Arg1 = (A + B) * 1 + 0 */
                  TEXENV_SETUP_ARG_RGB(colorComb.SourceA,
                                       texUnit->CombineSourceRGB[0],
                                       texUnit->CombineOperandRGB[0],
                                       localc, locala);
                  TEXENV_SETUP_MODE_RGB(colorComb.ModeA,
                                        texUnit->CombineOperandRGB[0]);
                  TEXENV_SETUP_ARG_RGB(colorComb.SourceB,
                                       texUnit->CombineSourceRGB[1],
                                       texUnit->CombineOperandRGB[1],
                                       localc, locala);
                  TEXENV_SETUP_MODE_RGB(colorComb.ModeB,
                                        texUnit->CombineOperandRGB[1]);
                  colorComb.SourceC = GR_CMBX_ZERO;
                  colorComb.InvertC = FXTRUE;
                  colorComb.SourceD = GR_CMBX_ZERO;
                  break;
             case GL_INTERPOLATE_EXT:
                  /* Arg0 * Arg2 + Arg1 * (1 - Arg2) ==
                   * (Arg0 - Arg1) * Arg2 + Arg1 == (A - B) * C + D
                   */
                  TEXENV_SETUP_ARG_RGB(colorComb.SourceA,
                                       texUnit->CombineSourceRGB[0],
                                       texUnit->CombineOperandRGB[0],
                                       localc, locala);
                  TEXENV_SETUP_MODE_RGB(colorComb.ModeA,
                                        texUnit->CombineOperandRGB[0]);
                  TEXENV_SETUP_ARG_RGB(colorComb.SourceB,
                                       texUnit->CombineSourceRGB[1],
                                       texUnit->CombineOperandRGB[1],
                                       localc, locala);
                  if (TEXENV_OPERAND_INVERTED(texUnit->CombineOperandRGB[1])) {
                     /* Hack alert!!! This case is wrong!!! */
                     fprintf(stderr, "COMBINE_EXT_color: WRONG!!!\n");
                     colorComb.ModeB = GR_FUNC_MODE_NEGATIVE_X;
                  } else {
                     colorComb.ModeB = GR_FUNC_MODE_NEGATIVE_X;
                  }
                  /*
                   * The Source/Operand for the C value must
                   * specify some kind of alpha value.
                   */
                  TEXENV_SETUP_ARG_A(colorComb.SourceC,
                                     texUnit->CombineSourceRGB[2],
                                     texUnit->CombineOperandRGB[2],
                                     locala);
                  colorComb.InvertC = FXFALSE;
                  colorComb.SourceD = GR_CMBX_B;
                  break;
             default:
                  fprintf(stderr, "COMBINE_EXT_color: %s\n",
                                  _mesa_lookup_enum_by_nr(texUnit->CombineModeRGB));
      }

      switch (texUnit->CombineModeA) {
             case GL_MODULATE:
                  /* Arg0 * Arg1 == (A + 0) * C + 0 */
                  TEXENV_SETUP_ARG_A(alphaComb.SourceA,
                                     texUnit->CombineSourceA[0],
                                     texUnit->CombineOperandA[0],
                                     locala);
                  TEXENV_SETUP_MODE_A(alphaComb.ModeA,
                                      texUnit->CombineOperandA[0]);
                  alphaComb.SourceB = GR_CMBX_ZERO;
                  alphaComb.ModeB   = GR_FUNC_MODE_ZERO;
                  TEXENV_SETUP_ARG_A(alphaComb.SourceC,
                                     texUnit->CombineSourceA[1],
                                     texUnit->CombineOperandA[1],
                                     locala);
                  alphaComb.InvertC = TEXENV_OPERAND_INVERTED(
                                       texUnit->CombineOperandA[1]);
                  alphaComb.SourceD = GR_CMBX_ZERO;
                  break;
             case GL_REPLACE:
                 /* Arg0 == (A + 0) * 1 + 0 */
                  TEXENV_SETUP_ARG_A(alphaComb.SourceA,
                                     texUnit->CombineSourceA[0],
                                     texUnit->CombineOperandA[0],
                                     locala);
                  TEXENV_SETUP_MODE_A(alphaComb.ModeA,
                                      texUnit->CombineOperandA[0]);
                  alphaComb.SourceB = GR_CMBX_ZERO;
                  alphaComb.ModeB   = GR_FUNC_MODE_ZERO;
                  alphaComb.SourceC = GR_CMBX_ZERO;
                  alphaComb.InvertC = FXTRUE;
                  alphaComb.SourceD = GR_CMBX_ZERO;
                  break;
             case GL_ADD:
                  /* Arg0 + Arg1 = (A + B) * 1 + 0 */
                  TEXENV_SETUP_ARG_A(alphaComb.SourceA,
                                     texUnit->CombineSourceA[0],
                                     texUnit->CombineOperandA[0],
                                     locala);
                  TEXENV_SETUP_MODE_A(alphaComb.ModeA,
                                      texUnit->CombineOperandA[0]);
                  TEXENV_SETUP_ARG_A(alphaComb.SourceB,
                                     texUnit->CombineSourceA[1],
                                     texUnit->CombineOperandA[1],
                                     locala);
                  TEXENV_SETUP_MODE_A(alphaComb.ModeB,
                                      texUnit->CombineOperandA[1]);
                  alphaComb.SourceC = GR_CMBX_ZERO;
                  alphaComb.InvertC = FXTRUE;
                  alphaComb.SourceD = GR_CMBX_ZERO;
                  break;
             default:
                  fprintf(stderr, "COMBINE_EXT_alpha: %s\n",
                                  _mesa_lookup_enum_by_nr(texUnit->CombineModeA));
      }

      fxMesa->Glide.grConstantColorValueExt(tmu,
         (((GLuint)(envColor[0] * 255.0f))      ) |
         (((GLuint)(envColor[1] * 255.0f)) <<  8) |
         (((GLuint)(envColor[2] * 255.0f)) << 16) |
         (((GLuint)(envColor[3] * 255.0f)) << 24));
      break;
#if 0
        {
            FxU32 A_RGB, B_RGB, C_RGB, D_RGB;
            FxU32 Amode_RGB, Bmode_RGB;
            FxBool Cinv_RGB, Dinv_RGB, Ginv_RGB;
            FxU32 Shift_RGB;
            FxU32 A_A, B_A, C_A, D_A;
            FxU32 Amode_A, Bmode_A;
            FxBool Cinv_A, Dinv_A, Ginv_A;
            FxU32 Shift_A;

           /*
            *
            * In the formulas below, we write:
            *  o "1(x)" for the identity function applied to x,
            *    so 1(x) = x.
            *  o "0(x)" for the constant function 0, so
            *    0(x) = 0 for all values of x.
            *
            * Calculate the color combination.
            */
            Shift_RGB = texUnit->CombineScaleShiftRGB;
            Shift_A = texUnit->CombineScaleShiftA;
            switch (texUnit->CombineModeRGB) {
            case GL_REPLACE:
               /*
                * The formula is: Arg0
                * We implement this by the formula:
                *   (Arg0 + 0(0))*(1-0) + 0
                */
                TEXENV_SETUP_ARG_RGB(A_RGB,
                                     texUnit->CombineSourceRGB[0],
                                     texUnit->CombineOperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Amode_RGB,
                                      texUnit->CombineOperandRGB[0]);
                B_RGB = C_RGB = D_RGB = GR_CMBX_ZERO;
                Bmode_RGB = GR_FUNC_MODE_ZERO;
                Cinv_RGB = FXTRUE;
                Dinv_RGB = Ginv_RGB = FXFALSE;
                break;
            case GL_MODULATE:
               /*
                * The formula is: Arg0 * Arg1
                *
                * We implement this by the formula
                *   (Arg0 + 0(0)) * Arg1 + 0(0)
                */
                TEXENV_SETUP_ARG_RGB(A_RGB,
                                     texUnit->CombineSourceRGB[0],
                                     texUnit->CombineOperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Amode_RGB,
                                      texUnit->CombineOperandRGB[0]);
                B_RGB = GR_CMBX_ZERO;
                Bmode_RGB = GR_CMBX_ZERO;
                TEXENV_SETUP_ARG_RGB(C_RGB,
                                     texUnit->CombineSourceRGB[1],
                                     texUnit->CombineOperandRGB[1],
                                     incomingRGB, incomingAlpha);
                Cinv_RGB = TEXENV_OPERAND_INVERTED
                               (texUnit->CombineOperandRGB[1]);
                D_RGB = GR_CMBX_ZERO;
                Dinv_RGB = Ginv_RGB = FXFALSE;
                break;
            case GL_ADD:
               /*
                * The formula is Arg0 + Arg1
                */
                TEXENV_SETUP_ARG_RGB(A_RGB,
                                     texUnit->CombineSourceRGB[0],
                                     texUnit->CombineOperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Amode_RGB,
                                      texUnit->CombineOperandRGB[0]);
                TEXENV_SETUP_ARG_RGB(B_RGB,
                                     texUnit->CombineSourceRGB[1],
                                     texUnit->CombineOperandRGB[1],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Bmode_RGB,
                                      texUnit->CombineOperandRGB[1]);
                C_RGB = D_RGB = GR_CMBX_ZERO;
                Cinv_RGB = FXTRUE;
                Dinv_RGB = Ginv_RGB = FXFALSE;
                break;
            case GL_ADD_SIGNED_EXT:
               /*
                * The formula is: Arg0 + Arg1 - 0.5.
                * We compute this by calculating:
                *      (Arg0 - 1/2) + Arg1         if op0 is SRC_{COLOR,ALPHA}
                *      Arg0 + (Arg1 - 1/2)         if op1 is SRC_{COLOR,ALPHA}
                * If both op0 and op1 are ONE_MINUS_SRC_{COLOR,ALPHA}
                * we cannot implement the formula properly.
                */
                TEXENV_SETUP_ARG_RGB(A_RGB,
                                     texUnit->CombineSourceRGB[0],
                                     texUnit->CombineOperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_ARG_RGB(B_RGB,
                                     texUnit->CombineSourceRGB[1],
                                     texUnit->CombineOperandRGB[1],
                                     incomingRGB, incomingAlpha);
                if (!TEXENV_OPERAND_INVERTED(texUnit->CombineOperandRGB[0])) {
                   /*
                    * A is not inverted.  So, choose it.
                    */
                    Amode_RGB = GR_FUNC_MODE_X_MINUS_HALF;
                    if (!TEXENV_OPERAND_INVERTED
                            (texUnit->CombineOperandRGB[1])) {
                        Bmode_RGB = GR_FUNC_MODE_X;
                    }
                    else {
                        Bmode_RGB = GR_FUNC_MODE_ONE_MINUS_X;
                    }
                }
                else {
                   /*
                    * A is inverted, so try to subtract 1/2
                    * from B.
                    */
                    Amode_RGB = GR_FUNC_MODE_ONE_MINUS_X;
                    if (!TEXENV_OPERAND_INVERTED
                            (texUnit->CombineOperandRGB[1])) {
                        Bmode_RGB = GR_FUNC_MODE_X_MINUS_HALF;
                    }
                    else {
                       /*
                        * Both are inverted.  This is the case
                        * we cannot handle properly.  We just
                        * choose to not add the - 1/2.
                        */
                        Bmode_RGB = GR_FUNC_MODE_ONE_MINUS_X;
                        return GL_FALSE;
                    }
                }
                C_RGB = D_RGB = GR_CMBX_ZERO;
                Cinv_RGB = FXTRUE;
                Dinv_RGB = Ginv_RGB = FXFALSE;
                break;
            case GL_INTERPOLATE_EXT:
               /*
                * The formula is: Arg0 * Arg2 + Arg1 * (1 - Arg2).
                * We compute this by the formula:
                *            (Arg0 - Arg1) * Arg2 + Arg1
                *               == Arg0 * Arg2 - Arg1 * Arg2 + Arg1
                *               == Arg0 * Arg2 + Arg1 * (1 - Arg2)
                * However, if both Arg1 is ONE_MINUS_X, the HW does
                * not support it properly.
                */
                TEXENV_SETUP_ARG_RGB(A_RGB,
                                     texUnit->CombineSourceRGB[0],
                                     texUnit->CombineOperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Amode_RGB,
                                      texUnit->CombineOperandRGB[0]);
                TEXENV_SETUP_ARG_RGB(B_RGB,
                                     texUnit->CombineSourceRGB[1],
                                     texUnit->CombineOperandRGB[1],
                                     incomingRGB, incomingAlpha);
                if (TEXENV_OPERAND_INVERTED(texUnit->CombineOperandRGB[1])) {
                   /*
                    * This case is wrong.
                    */
                   Bmode_RGB = GR_FUNC_MODE_NEGATIVE_X;
                   return GL_FALSE;
                }
                else {
                    Bmode_RGB = GR_FUNC_MODE_NEGATIVE_X;
                }
               /*
                * The Source/Operand for the C value must
                * specify some kind of alpha value.
                */
                TEXENV_SETUP_ARG_A(C_RGB,
                                   texUnit->CombineSourceRGB[2],
                                   texUnit->CombineOperandRGB[2],
                                   incomingAlpha);
                Cinv_RGB = FXFALSE;
                D_RGB = GR_CMBX_B;
                Dinv_RGB = Ginv_RGB = FXFALSE;
                break;
            default:
               /*
                * This is here mostly to keep from getting
                * a compiler warning about these not being set.
                * However, this should set all the texture values
                * to zero.
                */
                A_RGB = B_RGB = C_RGB = D_RGB = GR_CMBX_ZERO;
                Amode_RGB = Bmode_RGB = GR_FUNC_MODE_X;
                Cinv_RGB = Dinv_RGB = Ginv_RGB = FXFALSE;
                break;
            }
           /*
            * Calculate the alpha combination.
            */
            switch (texUnit->CombineModeA) {
            case GL_REPLACE:
               /*
                * The formula is: Arg0
                * We implement this by the formula:
                *   (Arg0 + 0(0))*(1-0) + 0
                */
                TEXENV_SETUP_ARG_A(A_A,
                                   texUnit->CombineSourceA[0],
                                   texUnit->CombineOperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Amode_A,
                                    texUnit->CombineOperandA[0]);
                B_A = GR_CMBX_ITALPHA;
                Bmode_A = GR_FUNC_MODE_ZERO;
                C_A = D_A = GR_CMBX_ZERO;
                Cinv_A = FXTRUE;
                Dinv_A = Ginv_A = FXFALSE;
                break;
            case GL_MODULATE:
               /*
                * The formula is: Arg0 * Arg1
                *
                * We implement this by the formula
                *   (Arg0 + 0(0)) * Arg1 + 0(0)
                */
                TEXENV_SETUP_ARG_A(A_A,
                                   texUnit->CombineSourceA[0],
                                   texUnit->CombineOperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Amode_A,
                                    texUnit->CombineOperandA[0]);
                B_A = GR_CMBX_ZERO;
                Bmode_A = GR_CMBX_ZERO;
                TEXENV_SETUP_ARG_A(C_A,
                                   texUnit->CombineSourceA[1],
                                   texUnit->CombineOperandA[1],
                                   incomingAlpha);
                Cinv_A = TEXENV_OPERAND_INVERTED
                               (texUnit->CombineOperandA[1]);
                D_A = GR_CMBX_ZERO;
                Dinv_A = Ginv_A = FXFALSE;
                break;
            case GL_ADD:
               /*
                * The formula is Arg0 + Arg1
                */
                TEXENV_SETUP_ARG_A(A_A,
                                   texUnit->CombineSourceA[0],
                                   texUnit->CombineOperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Amode_A,
                                    texUnit->CombineOperandA[0]);
                TEXENV_SETUP_ARG_A(B_A,
                                   texUnit->CombineSourceA[1],
                                   texUnit->CombineOperandA[1],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Bmode_A,
                                    texUnit->CombineOperandA[1]);
                C_A = D_A = GR_CMBX_ZERO;
                Cinv_A = FXTRUE;
                Dinv_A = Ginv_A = FXFALSE;
                break;
            case GL_ADD_SIGNED_EXT:
               /*
                * The formula is: Arg0 + Arg1 - 0.5.
                * We compute this by calculating:
                *      (Arg0 - 1/2) + Arg1         if op0 is SRC_{COLOR,ALPHA}
                *      Arg0 + (Arg1 - 1/2)         if op1 is SRC_{COLOR,ALPHA}
                * If both op0 and op1 are ONE_MINUS_SRC_{COLOR,ALPHA}
                * we cannot implement the formula properly.
                */
                TEXENV_SETUP_ARG_A(A_A,
                                   texUnit->CombineSourceA[0],
                                   texUnit->CombineOperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_ARG_A(B_A,
                                   texUnit->CombineSourceA[1],
                                   texUnit->CombineOperandA[1],
                                   incomingAlpha);
                if (!TEXENV_OPERAND_INVERTED(texUnit->CombineOperandA[0])) {
                   /*
                    * A is not inverted.  So, choose it.
                    */
                    Amode_A = GR_FUNC_MODE_X_MINUS_HALF;
                    if (!TEXENV_OPERAND_INVERTED
                            (texUnit->CombineOperandA[1])) {
                        Bmode_A = GR_FUNC_MODE_X;
                    } else {
                        Bmode_A = GR_FUNC_MODE_ONE_MINUS_X;
                    }
                } else {
                   /*
                    * A is inverted, so try to subtract 1/2
                    * from B.
                    */
                    Amode_A = GR_FUNC_MODE_ONE_MINUS_X;
                    if (!TEXENV_OPERAND_INVERTED
                            (texUnit->CombineOperandA[1])) {
                        Bmode_A = GR_FUNC_MODE_X_MINUS_HALF;
                    } else {
                       /*
                        * Both are inverted.  This is the case
                        * we cannot handle properly.  We just
                        * choose to not add the - 1/2.
                        */
                        Bmode_A = GR_FUNC_MODE_ONE_MINUS_X;
                        return GL_FALSE;
                    }
                }
                C_A = D_A = GR_CMBX_ZERO;
                Cinv_A = FXTRUE;
                Dinv_A = Ginv_A = FXFALSE;
                break;
            case GL_INTERPOLATE_EXT:
               /*
                * The formula is: Arg0 * Arg2 + Arg1 * (1 - Arg2).
                * We compute this by the formula:
                *            (Arg0 - Arg1) * Arg2 + Arg1
                *               == Arg0 * Arg2 - Arg1 * Arg2 + Arg1
                *               == Arg0 * Arg2 + Arg1 * (1 - Arg2)
                * However, if both Arg1 is ONE_MINUS_X, the HW does
                * not support it properly.
                */
                TEXENV_SETUP_ARG_A(A_A,
                                   texUnit->CombineSourceA[0],
                                   texUnit->CombineOperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Amode_A,
                                    texUnit->CombineOperandA[0]);
                TEXENV_SETUP_ARG_A(B_A,
                                   texUnit->CombineSourceA[1],
                                   texUnit->CombineOperandA[1],
                                   incomingAlpha);
                if (!TEXENV_OPERAND_INVERTED(texUnit->CombineOperandA[1])) {
                    Bmode_A = GR_FUNC_MODE_NEGATIVE_X;
                }
                else {
                   /*
                    * This case is wrong.
                    */
                    Bmode_A = GR_FUNC_MODE_NEGATIVE_X;
                    return GL_FALSE;
                }
               /*
                * The Source/Operand for the C value must
                * specify some kind of alpha value.
                */
                TEXENV_SETUP_ARG_A(C_A,
                                   texUnit->CombineSourceA[2],
                                   texUnit->CombineOperandA[2],
                                   incomingAlpha);
                Cinv_A = FXFALSE;
                D_A = GR_CMBX_B;
                Dinv_A = Ginv_A = FXFALSE;
                break;
            default:
               /*
                * This is here mostly to keep from getting
                * a compiler warning about these not being set.
                * However, this should set all the alpha values
                * to one.
                */
                A_A = B_A = C_A = D_A = GR_CMBX_ZERO;
                Amode_A = Bmode_A = GR_FUNC_MODE_X;
                Cinv_A = Dinv_A = FXFALSE;
                Ginv_A = FXTRUE;
                break;
            }
           /*
            * Save the parameters.
            */
            env->Color.SourceA = A_RGB;
            env->Color.ModeA = Amode_RGB;
            env->Color.SourceB = B_RGB;
            env->Color.ModeB = Bmode_RGB;
            env->Color.SourceC = C_RGB;
            env->Color.InvertC = Cinv_RGB;
            env->Color.SourceD = D_RGB;
            env->Color.InvertD = Dinv_RGB;
            env->Color.Shift = Shift_RGB;
            env->Color.Invert = Ginv_RGB;
            env->Alpha.SourceA = A_A;
            env->Alpha.ModeA = Amode_A;
            env->Alpha.SourceB = B_A;
            env->Alpha.ModeB = Bmode_A;
            env->Alpha.SourceC = C_A;
            env->Alpha.InvertC = Cinv_A;
            env->Alpha.SourceD = D_A;
            env->Alpha.InvertD = Dinv_A;
            env->Alpha.Shift = Shift_A;
            env->Alpha.Invert = Ginv_A;
            env->EnvColor = PACK_RGBA32(texUnit->EnvColor[0] * 255.0F,
                                        texUnit->EnvColor[1] * 255.0F,
                                        texUnit->EnvColor[2] * 255.0F,
                                        texUnit->EnvColor[3] * 255.0F);
        }
        break;
#endif
   default:
      if (TDFX_DEBUG & VERBOSE_DRIVER) {
	 fprintf(stderr, "fxSetupTextureEnvNapalm_NoLock: %x Texture.EnvMode not yet supported\n",
		 texUnit->EnvMode);
      }
      return;
   }

   /* On Napalm we simply put the color combine unit into passthrough mode
    * and do everything we need with the texture combine units. */
   fxMesa->Glide.grColorCombineExt(GR_CMBX_TEXTURE_RGB,
                                   GR_FUNC_MODE_X,
                                   GR_CMBX_ZERO,
                                   GR_FUNC_MODE_X,
                                   GR_CMBX_ZERO,
                                   FXTRUE,
                                   GR_CMBX_ZERO,
                                   FXFALSE,
                                   0,
                                   FXFALSE);
   fxMesa->Glide.grAlphaCombineExt(GR_CMBX_TEXTURE_ALPHA,
                                   GR_FUNC_MODE_X,
                                   GR_CMBX_ZERO,
                                   GR_FUNC_MODE_X,
                                   GR_CMBX_ZERO,
                                   FXTRUE,
                                   GR_CMBX_ZERO,
                                   FXFALSE,
                                   0,
                                   FXFALSE);

   fxMesa->Glide.grTexAlphaCombineExt(tmu,
                                      alphaComb.SourceA,
                                      alphaComb.ModeA,
                                      alphaComb.SourceB,
                                      alphaComb.ModeB,
                                      alphaComb.SourceC,
                                      alphaComb.InvertC,
                                      alphaComb.SourceD,
                                      alphaComb.InvertD,
                                      alphaComb.Shift,
                                      alphaComb.Invert);
   fxMesa->Glide.grTexColorCombineExt(tmu,
                                      colorComb.SourceA,
                                      colorComb.ModeA,
                                      colorComb.SourceB,
                                      colorComb.ModeB,
                                      colorComb.SourceC,
                                      colorComb.InvertC,
                                      colorComb.SourceD,
                                      colorComb.InvertD,
                                      colorComb.Shift,
                                      colorComb.Invert);
}


/************************* Single Texture Set ***************************/

static void
fxSelectSingleTMUSrcNapalm_NoLock(fxMesaContext fxMesa, GLint tmu, FxBool LODblend)
{
   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSelectSingleTMUSrcNapalm_NoLock(%d, %d)\n", tmu, LODblend);
   }

   if (LODblend) {
      /* [dBorca] Hack alert:
       * TODO: GR_CMBX_LOD_FRAC
       */

      fxMesa->tmuSrc = FX_TMU_SPLIT;
   }
   else {
      if (tmu != FX_TMU1) {
         /* disable tex1 */
         if (fxMesa->haveTwoTMUs) {
            fxMesa->Glide.grTexAlphaCombineExt(FX_TMU1,
                                               GR_CMBX_ZERO,
                                               GR_FUNC_MODE_ZERO,
                                               GR_CMBX_ZERO,
                                               GR_FUNC_MODE_ZERO,
                                               GR_CMBX_ZERO,
                                               FXTRUE,
                                               GR_CMBX_ZERO,
                                               FXFALSE,
                                               0,
                                               FXFALSE);
            fxMesa->Glide.grTexColorCombineExt(FX_TMU1,
                                               GR_CMBX_ZERO,
                                               GR_FUNC_MODE_ZERO,
                                               GR_CMBX_ZERO,
                                               GR_FUNC_MODE_ZERO,
                                               GR_CMBX_ZERO,
                                               FXTRUE,
                                               GR_CMBX_ZERO,
                                               FXFALSE,
                                               0,
                                               FXFALSE);
         }

	 fxMesa->tmuSrc = FX_TMU0;
      }
      else {
#if 1
         grTexCombine(GR_TMU0,
                      GR_COMBINE_FUNCTION_BLEND,
                      GR_COMBINE_FACTOR_ONE,
                      GR_COMBINE_FUNCTION_BLEND,
                      GR_COMBINE_FACTOR_ONE,
                      FXFALSE,
                      FXFALSE);
#else
         /* [dBorca] why, oh why? doesn't work! stupid Glide? */
         fxMesa->Glide.grTexAlphaCombineExt(FX_TMU0,
                                            GR_CMBX_OTHER_TEXTURE_ALPHA,
                                            GR_FUNC_MODE_X,
                                            GR_CMBX_ZERO,
                                            GR_FUNC_MODE_X,
                                            GR_CMBX_ZERO,
                                            FXTRUE,
                                            GR_CMBX_ZERO,
                                            FXFALSE,
                                            0,
                                            FXFALSE);
         fxMesa->Glide.grTexColorCombineExt(FX_TMU0,
                                            GR_CMBX_OTHER_TEXTURE_RGB,
                                            GR_FUNC_MODE_X,
                                            GR_CMBX_ZERO,
                                            GR_FUNC_MODE_X,
                                            GR_CMBX_ZERO,
                                            FXTRUE,
                                            GR_CMBX_ZERO,
                                            FXFALSE,
                                            0,
                                            FXFALSE);
#endif

	 fxMesa->tmuSrc = FX_TMU1;
      }
   }
}

static void
fxSetupTextureSingleTMUNapalm_NoLock(GLcontext * ctx, GLuint textureset)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   GLuint unitsmode;
   tfxTexInfo *ti;
   struct gl_texture_object *tObj = ctx->Texture.Unit[textureset].Current2D;
   int tmu;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupTextureSingleTMUNapalm_NoLock(%d)\n", textureset);
   }

   ti = fxTMGetTexInfo(tObj);

   fxTexValidate(ctx, tObj);

   fxSetupSingleTMU_NoLock(fxMesa, tObj);

   if (ti->whichTMU == FX_TMU_BOTH)
      tmu = FX_TMU0;
   else
      tmu = ti->whichTMU;
   if (fxMesa->tmuSrc != tmu)
      fxSelectSingleTMUSrcNapalm_NoLock(fxMesa, tmu, ti->LODblend);

   if (textureset == 0 || !fxMesa->haveTwoTMUs)
      unitsmode = fxGetTexSetConfiguration(ctx, tObj, NULL);
   else
      unitsmode = fxGetTexSetConfiguration(ctx, NULL, tObj);

/*    if(fxMesa->lastUnitsMode==unitsmode) */
/*      return; */

   fxMesa->lastUnitsMode = unitsmode;

   fxMesa->stw_hint_state = 0;
   FX_grHints_NoLock(GR_HINT_STWHINT, 0);

   if (TDFX_DEBUG & (VERBOSE_DRIVER | VERBOSE_TEXTURE))
      fprintf(stderr, "fxSetupTextureSingleTMUNapalm_NoLock: envmode is %s\n",
	      _mesa_lookup_enum_by_nr(ctx->Texture.Unit[textureset].EnvMode));

   /* [dBorca] Hack alert:
    * what if we're in split mode? (LODBlend)
    * also should we update BOTH TMUs in FX_TMU_BOTH mode?
    */
   fxSetupTextureEnvNapalm_NoLock(ctx, textureset, tmu, GL_TRUE);
}


/************************* Double Texture Set ***************************/

static void
fxSetupTextureDoubleTMUNapalm_NoLock(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   tfxTexInfo *ti0, *ti1;
   struct gl_texture_object *tObj0 = ctx->Texture.Unit[1].Current2D;
   struct gl_texture_object *tObj1 = ctx->Texture.Unit[0].Current2D;
   GLuint unitsmode;
   int tmu0 = 0, tmu1 = 1;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupTextureDoubleTMUNapalm_NoLock(...)\n");
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

   if (TDFX_DEBUG & (VERBOSE_DRIVER | VERBOSE_TEXTURE))
      fprintf(stderr, "fxSetupTextureDoubleTMUNapalm_NoLock: envmode is %s/%s\n",
	      _mesa_lookup_enum_by_nr(ctx->Texture.Unit[0].EnvMode),
	      _mesa_lookup_enum_by_nr(ctx->Texture.Unit[1].EnvMode));


   if ((ti0->whichTMU == FX_TMU1) || (ti1->whichTMU == FX_TMU0)) {
      tmu0 = 1;
      tmu1 = 0;
   }
   fxMesa->tmuSrc = FX_TMU_BOTH;

   /* OpenGL vs Glide texture pipeline */
   fxSetupTextureEnvNapalm_NoLock(ctx, 0, 1, GL_TRUE);
   fxSetupTextureEnvNapalm_NoLock(ctx, 1, 0, GL_FALSE);
}

/************************* No Texture ***************************/

static void
fxSetupTextureNoneNapalm_NoLock(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxSetupTextureNoneNapalm_NoLock(...)\n");
   }

   /* the combiner formula is: (A + B) * C + D
   **
   ** a = tc_otherselect
   ** a_mode = tc_invert_other
   ** b = tc_localselect
   ** b_mode = tc_invert_local
   ** c = (tc_mselect, tc_mselect_7)
   ** d = (tc_add_clocal, tc_add_alocal)
   ** shift = tc_outshift
   ** invert = tc_invert_output
   */

   fxMesa->Glide.grColorCombineExt(GR_CMBX_ITRGB,
                                   GR_FUNC_MODE_X,
                                   GR_CMBX_ZERO,
                                   GR_FUNC_MODE_ZERO,
                                   GR_CMBX_ZERO,
                                   FXTRUE,
                                   GR_CMBX_ZERO,
                                   FXFALSE,
                                   0,
                                   FXFALSE);
   fxMesa->Glide.grAlphaCombineExt(GR_CMBX_ITALPHA,
                                   GR_FUNC_MODE_X,
                                   GR_CMBX_ZERO,
                                   GR_FUNC_MODE_ZERO,
                                   GR_CMBX_ZERO,
                                   FXTRUE,
                                   GR_CMBX_ZERO,
                                   FXFALSE,
                                   0,
                                   FXFALSE);

   fxMesa->lastUnitsMode = FX_UM_NONE;
}
