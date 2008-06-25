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
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_texstate.c,v 1.2 2002/02/22 21:45:04 dawes Exp $ */

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

#include "tdfx_state.h"
#include "tdfx_tex.h"
#include "tdfx_texman.h"
#include "tdfx_texstate.h"


/* =============================================================
 * Texture
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



/*
 * Setup a texture environment on Voodoo5.
 * Return GL_TRUE for success, GL_FALSE for failure.
 * If we fail, we'll have to use software rendering.
 */
static GLboolean
SetupTexEnvNapalm(GLcontext *ctx, GLboolean useIteratedRGBA,
                  const struct gl_texture_unit *texUnit, GLenum baseFormat,
                  struct tdfx_texcombine_ext *env)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    GrTCCUColor_t incomingRGB, incomingAlpha;
    const GLenum envMode = texUnit->EnvMode;

    if (useIteratedRGBA) {
        incomingRGB = GR_CMBX_ITRGB;
        incomingAlpha = GR_CMBX_ITALPHA;
    }
    else {
        incomingRGB = GR_CMBX_OTHER_TEXTURE_RGB;
        incomingAlpha = GR_CMBX_OTHER_TEXTURE_ALPHA;
    }

    /* invariant: */
    env->Color.Shift = 0;
    env->Color.Invert = FXFALSE;
    env->Alpha.Shift = 0;
    env->Alpha.Invert = FXFALSE;

    switch (envMode) {
    case GL_REPLACE:
        /* -- Setup RGB combiner */
        if (baseFormat == GL_ALPHA) {
            /* Rv = Rf */
            env->Color.SourceA = incomingRGB;
        }
        else {
            /* Rv = Rt */
            env->Color.SourceA = GR_CMBX_LOCAL_TEXTURE_RGB;
        }
        env->Color.ModeA = GR_FUNC_MODE_X;
        env->Color.SourceB = GR_CMBX_ZERO;
        env->Color.ModeB = GR_FUNC_MODE_ZERO;
        env->Color.SourceC = GR_CMBX_ZERO;
        env->Color.InvertC = FXTRUE;
        env->Color.SourceD = GR_CMBX_ZERO;
        env->Color.InvertD = FXFALSE;
        /* -- Setup Alpha combiner */
        if (baseFormat == GL_LUMINANCE || baseFormat == GL_RGB) {
            /* Av = Af */
           env->Alpha.SourceD = incomingAlpha;
        }
        else {
            /* Av = At */
           env->Alpha.SourceD = GR_CMBX_LOCAL_TEXTURE_ALPHA;
        }
        env->Alpha.SourceA = GR_CMBX_ITALPHA;
        env->Alpha.ModeA = GR_FUNC_MODE_ZERO;
        env->Alpha.SourceB = GR_CMBX_ITALPHA;
        env->Alpha.ModeB = GR_FUNC_MODE_ZERO;
        env->Alpha.SourceC = GR_CMBX_ZERO;
        env->Alpha.InvertC = FXFALSE;
        env->Alpha.InvertD = FXFALSE;
        break;

    case GL_MODULATE:
        /* -- Setup RGB combiner */
        if (baseFormat == GL_ALPHA) {
            /* Rv = Rf */
           env->Color.SourceC = GR_CMBX_ZERO;
           env->Color.InvertC = FXTRUE;
        }
        else {
            /* Result = Frag * Tex */
           env->Color.SourceC = GR_CMBX_LOCAL_TEXTURE_RGB;
           env->Color.InvertC = FXFALSE;
        }
        env->Color.SourceA = incomingRGB;
        env->Color.ModeA = GR_FUNC_MODE_X;
        env->Color.SourceB = GR_CMBX_ZERO;
        env->Color.ModeB = GR_FUNC_MODE_ZERO;
        env->Color.SourceD = GR_CMBX_ZERO;
        env->Color.InvertD = FXFALSE;
        /* -- Setup Alpha combiner */
        if (baseFormat == GL_LUMINANCE || baseFormat == GL_RGB) {
            /* Av = Af */
           env->Alpha.SourceA = incomingAlpha;
           env->Alpha.SourceC = GR_CMBX_ZERO;
           env->Alpha.InvertC = FXTRUE;
        }
        else {
            /* Av = Af * At */
           env->Alpha.SourceA = GR_CMBX_LOCAL_TEXTURE_ALPHA;
           env->Alpha.SourceC = incomingAlpha;
           env->Alpha.InvertC = FXFALSE;
        }
        env->Alpha.ModeA = GR_FUNC_MODE_X;
        env->Alpha.SourceB = GR_CMBX_ITALPHA;
        env->Alpha.ModeB = GR_FUNC_MODE_ZERO;
        env->Alpha.SourceD = GR_CMBX_ZERO;
        env->Alpha.InvertD = FXFALSE;
        break;

    case GL_DECAL:
        /* -- Setup RGB combiner */
        if (baseFormat == GL_RGB) {
            /* Rv = Rt */
           env->Color.SourceB = GR_CMBX_ZERO;
           env->Color.ModeB = GR_FUNC_MODE_X;
           env->Color.SourceC = GR_CMBX_ZERO;
           env->Color.InvertC = FXTRUE;
           env->Color.SourceD = GR_CMBX_ZERO;
           env->Color.InvertD = FXFALSE;
        }
        else {
            /* Rv = Rf * (1 - At) + Rt * At */
           env->Color.SourceB = incomingRGB;
           env->Color.ModeB = GR_FUNC_MODE_NEGATIVE_X;
           env->Color.SourceC = GR_CMBX_LOCAL_TEXTURE_ALPHA;
           env->Color.InvertC = FXFALSE;
           env->Color.SourceD = GR_CMBX_B;
           env->Color.InvertD = FXFALSE;
        }
        env->Color.SourceA = GR_CMBX_LOCAL_TEXTURE_RGB;
        env->Color.ModeA = GR_FUNC_MODE_X;
        /* -- Setup Alpha combiner */
        /* Av = Af */
        env->Alpha.SourceA = incomingAlpha;
        env->Alpha.ModeA = GR_FUNC_MODE_X;
        env->Alpha.SourceB = GR_CMBX_ITALPHA;
        env->Alpha.ModeB = GR_FUNC_MODE_ZERO;
        env->Alpha.SourceC = GR_CMBX_ZERO;
        env->Alpha.InvertC = FXTRUE;
        env->Alpha.SourceD = GR_CMBX_ZERO;
        env->Alpha.InvertD = FXFALSE;
        break;

    case GL_BLEND:
        /* -- Setup RGB combiner */
        if (baseFormat == GL_ALPHA) {
            /* Rv = Rf */
            env->Color.SourceA = incomingRGB;
            env->Color.ModeA = GR_FUNC_MODE_X;
            env->Color.SourceB = GR_CMBX_ZERO;
            env->Color.ModeB = GR_FUNC_MODE_ZERO;
            env->Color.SourceC = GR_CMBX_ZERO;
            env->Color.InvertC = FXTRUE;
            env->Color.SourceD = GR_CMBX_ZERO;
            env->Color.InvertD = FXFALSE;
        }
        else {
            /* Rv = Rf * (1 - Rt) + Rc * Rt */
            env->Color.SourceA = GR_CMBX_TMU_CCOLOR;
            env->Color.ModeA = GR_FUNC_MODE_X;
            env->Color.SourceB = incomingRGB;
            env->Color.ModeB = GR_FUNC_MODE_NEGATIVE_X;
            env->Color.SourceC = GR_CMBX_LOCAL_TEXTURE_RGB;
            env->Color.InvertC = FXFALSE;
            env->Color.SourceD = GR_CMBX_B;
            env->Color.InvertD = FXFALSE;
        }
        /* -- Setup Alpha combiner */
        if (baseFormat == GL_LUMINANCE || baseFormat == GL_RGB) {
            /* Av = Af */
            env->Alpha.SourceA = incomingAlpha;
            env->Alpha.ModeA = GR_FUNC_MODE_X;
            env->Alpha.SourceB = GR_CMBX_ZERO;
            env->Alpha.ModeB = GR_FUNC_MODE_ZERO;
            env->Alpha.SourceC = GR_CMBX_ZERO;
            env->Alpha.InvertC = FXTRUE;
            env->Alpha.SourceD = GR_CMBX_ZERO;
            env->Alpha.InvertD = FXFALSE;
        }
        else if (baseFormat == GL_INTENSITY) {
            /* Av = Af * (1 - It) + Ac * It */
            env->Alpha.SourceA = GR_CMBX_TMU_CALPHA;
            env->Alpha.ModeA = GR_FUNC_MODE_X;
            env->Alpha.SourceB = incomingAlpha;
            env->Alpha.ModeB = GR_FUNC_MODE_NEGATIVE_X;
            env->Alpha.SourceC = GR_CMBX_LOCAL_TEXTURE_ALPHA;
            env->Alpha.InvertC = FXFALSE;
            env->Alpha.SourceD = GR_CMBX_B;
            env->Alpha.InvertD = FXFALSE;
        }
        else {
            /* Av = Af * At */
            env->Alpha.SourceA = GR_CMBX_LOCAL_TEXTURE_ALPHA;
            env->Alpha.ModeA = GR_FUNC_MODE_X;
            env->Alpha.SourceB = GR_CMBX_ITALPHA;
            env->Alpha.ModeB = GR_FUNC_MODE_ZERO;
            env->Alpha.SourceC = incomingAlpha;
            env->Alpha.InvertC = FXFALSE;
            env->Alpha.SourceD = GR_CMBX_ZERO;
            env->Alpha.InvertD = FXFALSE;
        }
        /* Also have to set up the tex env constant color */
        env->EnvColor = PACK_RGBA32(texUnit->EnvColor[0] * 255.0F,
                                    texUnit->EnvColor[1] * 255.0F,
                                    texUnit->EnvColor[2] * 255.0F,
                                    texUnit->EnvColor[3] * 255.0F);
        break;
    case GL_ADD:
        /* -- Setup RGB combiner */
        if (baseFormat == GL_ALPHA) {
            /* Rv = Rf */
           env->Color.SourceB = GR_CMBX_ZERO;
           env->Color.ModeB = GR_FUNC_MODE_ZERO;
        }
        else {
            /* Rv = Rf + Tt */
           env->Color.SourceB = GR_CMBX_LOCAL_TEXTURE_RGB;
           env->Color.ModeB = GR_FUNC_MODE_X;
        }
        env->Color.SourceA = incomingRGB;
        env->Color.ModeA = GR_FUNC_MODE_X;
        env->Color.SourceC = GR_CMBX_ZERO;
        env->Color.InvertC = FXTRUE;
        env->Color.SourceD = GR_CMBX_ZERO;
        env->Color.InvertD = FXFALSE;
        /* -- Setup Alpha combiner */
        if (baseFormat == GL_LUMINANCE || baseFormat == GL_RGB) {
            /* Av = Af */
           env->Alpha.SourceA = incomingAlpha;
           env->Alpha.SourceB = GR_CMBX_ITALPHA;
           env->Alpha.ModeB = GR_FUNC_MODE_ZERO;
           env->Alpha.SourceC = GR_CMBX_ZERO;
           env->Alpha.InvertC = FXTRUE;

        }
        else if (baseFormat == GL_INTENSITY) {
            /* Av = Af + It */
           env->Alpha.SourceA = incomingAlpha;
           env->Alpha.SourceB = GR_CMBX_LOCAL_TEXTURE_ALPHA;
           env->Alpha.ModeB = GR_FUNC_MODE_X;
           env->Alpha.SourceC = GR_CMBX_ZERO;
           env->Alpha.InvertC = FXTRUE;
        }
        else {
            /* Av = Af * At */
           env->Alpha.SourceA = GR_CMBX_LOCAL_TEXTURE_ALPHA;
           env->Alpha.SourceB = GR_CMBX_ITALPHA;
           env->Alpha.ModeB = GR_FUNC_MODE_ZERO;
           env->Alpha.SourceC = incomingAlpha;
           env->Alpha.InvertC = FXFALSE;
        }
        env->Alpha.ModeA = GR_FUNC_MODE_X;
        env->Alpha.SourceD = GR_CMBX_ZERO;
        env->Alpha.InvertD = FXFALSE;
        break;

    case GL_COMBINE_EXT:
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
            Shift_RGB = texUnit->Combine.ScaleShiftRGB;
            Shift_A = texUnit->Combine.ScaleShiftA;
            switch (texUnit->Combine.ModeRGB) {
            case GL_REPLACE:
               /*
                * The formula is: Arg0
                * We implement this by the formula:
                *   (Arg0 + 0(0))*(1-0) + 0
                */
                TEXENV_SETUP_ARG_RGB(A_RGB,
                                     texUnit->Combine.SourceRGB[0],
                                     texUnit->Combine.OperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Amode_RGB,
                                      texUnit->Combine.OperandRGB[0]);
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
                                     texUnit->Combine.SourceRGB[0],
                                     texUnit->Combine.OperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Amode_RGB,
                                      texUnit->Combine.OperandRGB[0]);
                B_RGB = GR_CMBX_ZERO;
                Bmode_RGB = GR_CMBX_ZERO;
                TEXENV_SETUP_ARG_RGB(C_RGB,
                                     texUnit->Combine.SourceRGB[1],
                                     texUnit->Combine.OperandRGB[1],
                                     incomingRGB, incomingAlpha);
                Cinv_RGB = TEXENV_OPERAND_INVERTED
                               (texUnit->Combine.OperandRGB[1]);
                D_RGB = GR_CMBX_ZERO;
                Dinv_RGB = Ginv_RGB = FXFALSE;
                break;
            case GL_ADD:
               /*
                * The formula is Arg0 + Arg1
                */
                TEXENV_SETUP_ARG_RGB(A_RGB,
                                     texUnit->Combine.SourceRGB[0],
                                     texUnit->Combine.OperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Amode_RGB,
                                      texUnit->Combine.OperandRGB[0]);
                TEXENV_SETUP_ARG_RGB(B_RGB,
                                     texUnit->Combine.SourceRGB[1],
                                     texUnit->Combine.OperandRGB[1],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Bmode_RGB,
                                      texUnit->Combine.OperandRGB[1]);
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
                                     texUnit->Combine.SourceRGB[0],
                                     texUnit->Combine.OperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_ARG_RGB(B_RGB,
                                     texUnit->Combine.SourceRGB[1],
                                     texUnit->Combine.OperandRGB[1],
                                     incomingRGB, incomingAlpha);
                if (!TEXENV_OPERAND_INVERTED(texUnit->Combine.OperandRGB[0])) {
                   /*
                    * A is not inverted.  So, choose it.
                    */
                    Amode_RGB = GR_FUNC_MODE_X_MINUS_HALF;
                    if (!TEXENV_OPERAND_INVERTED
                            (texUnit->Combine.OperandRGB[1])) {
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
                            (texUnit->Combine.OperandRGB[1])) {
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
                                     texUnit->Combine.SourceRGB[0],
                                     texUnit->Combine.OperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Amode_RGB,
                                      texUnit->Combine.OperandRGB[0]);
                TEXENV_SETUP_ARG_RGB(B_RGB,
                                     texUnit->Combine.SourceRGB[1],
                                     texUnit->Combine.OperandRGB[1],
                                     incomingRGB, incomingAlpha);
                if (TEXENV_OPERAND_INVERTED(texUnit->Combine.OperandRGB[1])) {
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
                                   texUnit->Combine.SourceRGB[2],
                                   texUnit->Combine.OperandRGB[2],
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
            switch (texUnit->Combine.ModeA) {
            case GL_REPLACE:
               /*
                * The formula is: Arg0
                * We implement this by the formula:
                *   (Arg0 + 0(0))*(1-0) + 0
                */
                TEXENV_SETUP_ARG_A(A_A,
                                   texUnit->Combine.SourceA[0],
                                   texUnit->Combine.OperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Amode_A,
                                    texUnit->Combine.OperandA[0]);
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
                                   texUnit->Combine.SourceA[0],
                                   texUnit->Combine.OperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Amode_A,
                                    texUnit->Combine.OperandA[0]);
                B_A = GR_CMBX_ZERO;
                Bmode_A = GR_CMBX_ZERO;
                TEXENV_SETUP_ARG_A(C_A,
                                   texUnit->Combine.SourceA[1],
                                   texUnit->Combine.OperandA[1],
                                   incomingAlpha);
                Cinv_A = TEXENV_OPERAND_INVERTED
                               (texUnit->Combine.OperandA[1]);
                D_A = GR_CMBX_ZERO;
                Dinv_A = Ginv_A = FXFALSE;
                break;
            case GL_ADD:
               /*
                * The formula is Arg0 + Arg1
                */
                TEXENV_SETUP_ARG_A(A_A,
                                   texUnit->Combine.SourceA[0],
                                   texUnit->Combine.OperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Amode_A,
                                    texUnit->Combine.OperandA[0]);
                TEXENV_SETUP_ARG_A(B_A,
                                   texUnit->Combine.SourceA[1],
                                   texUnit->Combine.OperandA[1],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Bmode_A,
                                    texUnit->Combine.OperandA[1]);
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
                                   texUnit->Combine.SourceA[0],
                                   texUnit->Combine.OperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_ARG_A(B_A,
                                   texUnit->Combine.SourceA[1],
                                   texUnit->Combine.OperandA[1],
                                   incomingAlpha);
                if (!TEXENV_OPERAND_INVERTED(texUnit->Combine.OperandA[0])) {
                   /*
                    * A is not inverted.  So, choose it.
                    */
                    Amode_A = GR_FUNC_MODE_X_MINUS_HALF;
                    if (!TEXENV_OPERAND_INVERTED
                            (texUnit->Combine.OperandA[1])) {
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
                            (texUnit->Combine.OperandA[1])) {
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
                                   texUnit->Combine.SourceA[0],
                                   texUnit->Combine.OperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Amode_A,
                                    texUnit->Combine.OperandA[0]);
                TEXENV_SETUP_ARG_A(B_A,
                                   texUnit->Combine.SourceA[1],
                                   texUnit->Combine.OperandA[1],
                                   incomingAlpha);
                if (!TEXENV_OPERAND_INVERTED(texUnit->Combine.OperandA[1])) {
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
                                   texUnit->Combine.SourceA[2],
                                   texUnit->Combine.OperandA[2],
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

    default:
        _mesa_problem(ctx, "%s: Bad envMode", __FUNCTION__);
    }

    fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_ENV;

    fxMesa->ColorCombineExt.SourceA = GR_CMBX_TEXTURE_RGB;
    fxMesa->ColorCombineExt.ModeA = GR_FUNC_MODE_X,
    fxMesa->ColorCombineExt.SourceB = GR_CMBX_ZERO;
    fxMesa->ColorCombineExt.ModeB = GR_FUNC_MODE_X;
    fxMesa->ColorCombineExt.SourceC = GR_CMBX_ZERO;
    fxMesa->ColorCombineExt.InvertC = FXTRUE;
    fxMesa->ColorCombineExt.SourceD = GR_CMBX_ZERO;
    fxMesa->ColorCombineExt.InvertD = FXFALSE;
    fxMesa->ColorCombineExt.Shift = 0;
    fxMesa->ColorCombineExt.Invert = FXFALSE;
    fxMesa->dirty |= TDFX_UPLOAD_COLOR_COMBINE;
    fxMesa->AlphaCombineExt.SourceA = GR_CMBX_TEXTURE_ALPHA;
    fxMesa->AlphaCombineExt.ModeA = GR_FUNC_MODE_X;
    fxMesa->AlphaCombineExt.SourceB = GR_CMBX_ZERO;
    fxMesa->AlphaCombineExt.ModeB = GR_FUNC_MODE_X;
    fxMesa->AlphaCombineExt.SourceC = GR_CMBX_ZERO;
    fxMesa->AlphaCombineExt.InvertC = FXTRUE;
    fxMesa->AlphaCombineExt.SourceD = GR_CMBX_ZERO;
    fxMesa->AlphaCombineExt.InvertD = FXFALSE;
    fxMesa->AlphaCombineExt.Shift = 0;
    fxMesa->AlphaCombineExt.Invert = FXFALSE;
    fxMesa->dirty |= TDFX_UPLOAD_ALPHA_COMBINE;
    return GL_TRUE; /* success */
}



/*
 * Setup the Voodoo3 texture environment for a single texture unit.
 * Return GL_TRUE for success, GL_FALSE for failure.
 * If failure, we'll use software rendering.
 */
static GLboolean
SetupSingleTexEnvVoodoo3(GLcontext *ctx, int unit,
                         GLenum envMode, GLenum baseFormat)
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   GrCombineLocal_t localc, locala;
   struct tdfx_combine alphaComb, colorComb;

   if (1 /*iteratedRGBA*/)
      localc = locala = GR_COMBINE_LOCAL_ITERATED;
   else
      localc = locala = GR_COMBINE_LOCAL_CONSTANT;

   switch (envMode) {
   case GL_DECAL:
      alphaComb.Function = GR_COMBINE_FUNCTION_LOCAL;
      alphaComb.Factor = GR_COMBINE_FACTOR_NONE;
      alphaComb.Local = locala;
      alphaComb.Other = GR_COMBINE_OTHER_NONE;
      alphaComb.Invert = FXFALSE;
      colorComb.Function = GR_COMBINE_FUNCTION_BLEND;
      colorComb.Factor = GR_COMBINE_FACTOR_TEXTURE_ALPHA;
      colorComb.Local = localc;
      colorComb.Other = GR_COMBINE_OTHER_TEXTURE;
      colorComb.Invert = FXFALSE;
      break;
   case GL_MODULATE:
      alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
      alphaComb.Factor = GR_COMBINE_FACTOR_LOCAL;
      alphaComb.Local = locala;
      alphaComb.Other = GR_COMBINE_OTHER_TEXTURE;
      alphaComb.Invert = FXFALSE;
      if (baseFormat == GL_ALPHA) {
         colorComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         colorComb.Factor = GR_COMBINE_FACTOR_NONE;
         colorComb.Local = localc;
         colorComb.Other = GR_COMBINE_OTHER_NONE;
         colorComb.Invert = FXFALSE;
      }
      else {
         colorComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         colorComb.Factor = GR_COMBINE_FACTOR_LOCAL;
         colorComb.Local = localc;
         colorComb.Other = GR_COMBINE_OTHER_TEXTURE;
         colorComb.Invert = FXFALSE;
      }
      break;

   case GL_BLEND:
      /*
       * XXX we can't do real GL_BLEND mode.  These settings assume that
       * the TexEnv color is black and incoming fragment color is white.
       */
      if (baseFormat == GL_LUMINANCE || baseFormat == GL_RGB) {
         /* Av = Af */
         alphaComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         alphaComb.Factor = GR_COMBINE_FACTOR_NONE;
         alphaComb.Local = locala;
         alphaComb.Other = GR_COMBINE_OTHER_NONE;
         alphaComb.Invert = FXFALSE;
      }
      else if (baseFormat == GL_INTENSITY) {
         /* Av = Af * (1 - It) + Ac * It */
         alphaComb.Function = GR_COMBINE_FUNCTION_BLEND;
         alphaComb.Factor = GR_COMBINE_FACTOR_TEXTURE_ALPHA;
         alphaComb.Local = locala;
         alphaComb.Other = GR_COMBINE_OTHER_CONSTANT;
         alphaComb.Invert = FXFALSE;
      }
      else {
         /* Av = Af * At */
         alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         alphaComb.Factor = GR_COMBINE_FACTOR_LOCAL;
         alphaComb.Local = locala;
         alphaComb.Other = GR_COMBINE_OTHER_TEXTURE;
         alphaComb.Invert = FXFALSE;
      }
      if (baseFormat == GL_ALPHA) {
         colorComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         colorComb.Factor = GR_COMBINE_FACTOR_NONE;
         colorComb.Local = localc;
         colorComb.Other = GR_COMBINE_OTHER_NONE;
         colorComb.Invert = FXFALSE;
      }
      else {
         colorComb.Function = GR_COMBINE_FUNCTION_BLEND;
         colorComb.Factor = GR_COMBINE_FACTOR_TEXTURE_RGB;
         colorComb.Local = localc;
         colorComb.Other = GR_COMBINE_OTHER_CONSTANT;
         colorComb.Invert = FXTRUE;
      }
      fxMesa->Color.MonoColor = PACK_RGBA32(
         ctx->Texture.Unit[unit].EnvColor[0] * 255.0f,
         ctx->Texture.Unit[unit].EnvColor[1] * 255.0f,
         ctx->Texture.Unit[unit].EnvColor[2] * 255.0f,
         ctx->Texture.Unit[unit].EnvColor[3] * 255.0f);
      fxMesa->dirty |= TDFX_UPLOAD_CONSTANT_COLOR;
      break;

   case GL_REPLACE:
      if ((baseFormat == GL_RGB) || (baseFormat == GL_LUMINANCE)) {
         alphaComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         alphaComb.Factor = GR_COMBINE_FACTOR_NONE;
         alphaComb.Local = locala;
         alphaComb.Other = GR_COMBINE_OTHER_NONE;
         alphaComb.Invert = FXFALSE;
      }
      else {
         alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         alphaComb.Factor = GR_COMBINE_FACTOR_ONE;
         alphaComb.Local = locala;
         alphaComb.Other = GR_COMBINE_OTHER_TEXTURE;
         alphaComb.Invert = FXFALSE;
      }
      if (baseFormat == GL_ALPHA) {
         colorComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         colorComb.Factor = GR_COMBINE_FACTOR_NONE;
         colorComb.Local = localc;
         colorComb.Other = GR_COMBINE_OTHER_NONE;
         colorComb.Invert = FXFALSE;
      }
      else {
         colorComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         colorComb.Factor = GR_COMBINE_FACTOR_ONE;
         colorComb.Local = localc;
         colorComb.Other = GR_COMBINE_OTHER_TEXTURE;
         colorComb.Invert = FXFALSE;
      }
      break;

   case GL_ADD:
      if (baseFormat == GL_ALPHA ||
          baseFormat == GL_LUMINANCE_ALPHA ||
          baseFormat == GL_RGBA) {
         /* product of texel and fragment alpha */
         alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         alphaComb.Factor = GR_COMBINE_FACTOR_LOCAL;
         alphaComb.Local = locala;
         alphaComb.Other = GR_COMBINE_OTHER_TEXTURE;
         alphaComb.Invert = FXFALSE;
      }
      else if (baseFormat == GL_LUMINANCE || baseFormat == GL_RGB) {
         /* fragment alpha is unchanged */
         alphaComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         alphaComb.Factor = GR_COMBINE_FACTOR_NONE;
         alphaComb.Local = locala;
         alphaComb.Other = GR_COMBINE_OTHER_NONE;
         alphaComb.Invert = FXFALSE;
      }
      else {
         ASSERT(baseFormat == GL_INTENSITY);
         /* sum of texel and fragment alpha */
         alphaComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,
         alphaComb.Factor = GR_COMBINE_FACTOR_ONE;
         alphaComb.Local = locala;
         alphaComb.Other = GR_COMBINE_OTHER_TEXTURE;
         alphaComb.Invert = FXFALSE;
      }
      if (baseFormat == GL_ALPHA) {
         /* rgb unchanged */
         colorComb.Function = GR_COMBINE_FUNCTION_LOCAL;
         colorComb.Factor = GR_COMBINE_FACTOR_NONE;
         colorComb.Local = localc;
         colorComb.Other = GR_COMBINE_OTHER_NONE;
         colorComb.Invert = FXFALSE;
      }
      else {
         /* sum of texel and fragment rgb */
         colorComb.Function = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,
         colorComb.Factor = GR_COMBINE_FACTOR_ONE;
         colorComb.Local = localc;
         colorComb.Other = GR_COMBINE_OTHER_TEXTURE;
         colorComb.Invert = FXFALSE;
      }
      break;

   default: {
      (void) memcpy(&colorComb, &fxMesa->ColorCombine, sizeof(colorComb));
      (void) memcpy(&alphaComb, &fxMesa->AlphaCombine, sizeof(alphaComb));
      _mesa_problem(ctx, "bad texture env mode in %s", __FUNCTION__);
   }
   }

   if (colorComb.Function != fxMesa->ColorCombine.Function ||
       colorComb.Factor != fxMesa->ColorCombine.Factor ||
       colorComb.Local != fxMesa->ColorCombine.Local ||
       colorComb.Other != fxMesa->ColorCombine.Other ||
       colorComb.Invert != fxMesa->ColorCombine.Invert) {
      fxMesa->ColorCombine = colorComb;
      fxMesa->dirty |= TDFX_UPLOAD_COLOR_COMBINE;
   }

   if (alphaComb.Function != fxMesa->AlphaCombine.Function ||
       alphaComb.Factor != fxMesa->AlphaCombine.Factor ||
       alphaComb.Local != fxMesa->AlphaCombine.Local ||
       alphaComb.Other != fxMesa->AlphaCombine.Other ||
       alphaComb.Invert != fxMesa->AlphaCombine.Invert) {
      fxMesa->AlphaCombine = alphaComb;
      fxMesa->dirty |= TDFX_UPLOAD_ALPHA_COMBINE;
   }
   return GL_TRUE;
}


/*
 * Setup the Voodoo3 texture environment for dual texture units.
 * Return GL_TRUE for success, GL_FALSE for failure.
 * If failure, we'll use software rendering.
 */
static GLboolean
SetupDoubleTexEnvVoodoo3(GLcontext *ctx, int tmu0,
                         GLenum envMode0, GLenum baseFormat0,
                         GLenum envMode1, GLenum baseFormat1)
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   const GrCombineLocal_t locala = GR_COMBINE_LOCAL_ITERATED;
   const GrCombineLocal_t localc = GR_COMBINE_LOCAL_ITERATED;
   const int tmu1 = 1 - tmu0;

   if (envMode0 == GL_MODULATE && envMode1 == GL_MODULATE) {
      GLboolean isalpha[TDFX_NUM_TMU];

      isalpha[tmu0] = (baseFormat0 == GL_ALPHA);
      isalpha[tmu1] = (baseFormat1 == GL_ALPHA);

      if (isalpha[TDFX_TMU1]) {
         fxMesa->TexCombine[1].FunctionRGB = GR_COMBINE_FUNCTION_ZERO;
         fxMesa->TexCombine[1].FactorRGB = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorAlpha = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].InvertRGB = FXTRUE;
         fxMesa->TexCombine[1].InvertAlpha = FXFALSE;
      }
      else {
         fxMesa->TexCombine[1].FunctionRGB = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorRGB = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorAlpha = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].InvertRGB = FXFALSE;
         fxMesa->TexCombine[1].InvertAlpha = FXFALSE;
      }
      if (isalpha[TDFX_TMU0]) {
         fxMesa->TexCombine[0].FunctionRGB = GR_COMBINE_FUNCTION_BLEND_OTHER;
         fxMesa->TexCombine[0].FactorRGB = GR_COMBINE_FACTOR_ONE;
         fxMesa->TexCombine[0].FunctionAlpha = GR_COMBINE_FUNCTION_BLEND_OTHER;
         fxMesa->TexCombine[0].FactorAlpha = GR_COMBINE_FACTOR_LOCAL;
         fxMesa->TexCombine[0].InvertRGB = FXFALSE;
         fxMesa->TexCombine[0].InvertAlpha = FXFALSE;
      }
      else {
         fxMesa->TexCombine[0].FunctionRGB = GR_COMBINE_FUNCTION_BLEND_OTHER;
         fxMesa->TexCombine[0].FactorRGB = GR_COMBINE_FACTOR_LOCAL;
         fxMesa->TexCombine[0].FunctionAlpha = GR_COMBINE_FUNCTION_BLEND_OTHER;
         fxMesa->TexCombine[0].FactorAlpha = GR_COMBINE_FACTOR_LOCAL;
         fxMesa->TexCombine[0].InvertRGB = FXFALSE;
         fxMesa->TexCombine[0].InvertAlpha = FXFALSE;
      }
      fxMesa->ColorCombine.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
      fxMesa->ColorCombine.Factor = GR_COMBINE_FACTOR_LOCAL;
      fxMesa->ColorCombine.Local = localc;
      fxMesa->ColorCombine.Other = GR_COMBINE_OTHER_TEXTURE;
      fxMesa->ColorCombine.Invert = FXFALSE;
      fxMesa->AlphaCombine.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
      fxMesa->AlphaCombine.Factor = GR_COMBINE_FACTOR_LOCAL;
      fxMesa->AlphaCombine.Local = locala;
      fxMesa->AlphaCombine.Other = GR_COMBINE_OTHER_TEXTURE;
      fxMesa->AlphaCombine.Invert = FXFALSE;
   }
   else if (envMode0 == GL_REPLACE && envMode1 == GL_BLEND) { /* Quake */
      if (tmu0 == TDFX_TMU1) {
         fxMesa->TexCombine[1].FunctionRGB = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorRGB = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorAlpha = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].InvertRGB = FXTRUE;
         fxMesa->TexCombine[1].InvertAlpha = FXFALSE;
         fxMesa->TexCombine[0].FunctionRGB = GR_COMBINE_FUNCTION_BLEND_OTHER;
         fxMesa->TexCombine[0].FactorRGB = GR_COMBINE_FACTOR_LOCAL;
         fxMesa->TexCombine[0].FunctionAlpha = GR_COMBINE_FUNCTION_BLEND_OTHER;
         fxMesa->TexCombine[0].FactorAlpha = GR_COMBINE_FACTOR_LOCAL;
         fxMesa->TexCombine[0].InvertRGB = FXFALSE;
         fxMesa->TexCombine[0].InvertAlpha = FXFALSE;
      }
      else {
         fxMesa->TexCombine[1].FunctionRGB = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorRGB = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorAlpha = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].InvertRGB = FXFALSE;
         fxMesa->TexCombine[1].InvertAlpha = FXFALSE;
         fxMesa->TexCombine[0].FunctionRGB = GR_COMBINE_FUNCTION_BLEND_OTHER;
         fxMesa->TexCombine[0].FactorRGB = GR_COMBINE_FACTOR_ONE_MINUS_LOCAL;
         fxMesa->TexCombine[0].FunctionAlpha = GR_COMBINE_FUNCTION_BLEND_OTHER;
         fxMesa->TexCombine[0].FactorAlpha = GR_COMBINE_FACTOR_ONE_MINUS_LOCAL;
         fxMesa->TexCombine[0].InvertRGB = FXFALSE;
         fxMesa->TexCombine[0].InvertAlpha = FXFALSE;
      }
      fxMesa->ColorCombine.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
      fxMesa->ColorCombine.Factor = GR_COMBINE_FACTOR_ONE;
      fxMesa->ColorCombine.Local = localc;
      fxMesa->ColorCombine.Other = GR_COMBINE_OTHER_TEXTURE;
      fxMesa->ColorCombine.Invert = FXFALSE;
      fxMesa->AlphaCombine.Function = GR_COMBINE_FUNCTION_LOCAL;
      fxMesa->AlphaCombine.Factor = GR_COMBINE_FACTOR_NONE;
      fxMesa->AlphaCombine.Local = locala;
      fxMesa->AlphaCombine.Other = GR_COMBINE_OTHER_NONE;
      fxMesa->AlphaCombine.Invert = FXFALSE;
   }
   else if (envMode0 == GL_REPLACE && envMode1 == GL_MODULATE) {
      /* Quake 2/3 */
      if (tmu1 == TDFX_TMU1) {
         fxMesa->TexCombine[1].FunctionRGB = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorRGB = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].FunctionAlpha = GR_COMBINE_FUNCTION_ZERO;
         fxMesa->TexCombine[1].FactorAlpha = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].InvertRGB = FXFALSE;
         fxMesa->TexCombine[1].InvertAlpha = FXTRUE;
         fxMesa->TexCombine[0].FunctionRGB = GR_COMBINE_FUNCTION_BLEND_OTHER;
         fxMesa->TexCombine[0].FactorRGB = GR_COMBINE_FACTOR_LOCAL;
         fxMesa->TexCombine[0].FunctionAlpha = GR_COMBINE_FUNCTION_BLEND_OTHER;
         fxMesa->TexCombine[0].FactorAlpha = GR_COMBINE_FACTOR_LOCAL;
         fxMesa->TexCombine[0].InvertRGB = FXFALSE;
         fxMesa->TexCombine[0].InvertAlpha = FXFALSE;
      }
      else {
         fxMesa->TexCombine[1].FunctionRGB = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorRGB = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorAlpha = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].InvertRGB = FXFALSE;
         fxMesa->TexCombine[1].InvertAlpha = FXFALSE;
         fxMesa->TexCombine[0].FunctionRGB = GR_COMBINE_FUNCTION_BLEND_OTHER;
         fxMesa->TexCombine[0].FactorRGB = GR_COMBINE_FACTOR_LOCAL;
         fxMesa->TexCombine[0].FunctionAlpha = GR_COMBINE_FUNCTION_BLEND_OTHER;
         fxMesa->TexCombine[0].FactorAlpha = GR_COMBINE_FACTOR_ONE;
         fxMesa->TexCombine[0].InvertRGB = FXFALSE;
         fxMesa->TexCombine[0].InvertAlpha = FXFALSE;
      }

      fxMesa->ColorCombine.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
      fxMesa->ColorCombine.Factor = GR_COMBINE_FACTOR_ONE;
      fxMesa->ColorCombine.Local = localc;
      fxMesa->ColorCombine.Other = GR_COMBINE_OTHER_TEXTURE;
      fxMesa->ColorCombine.Invert = FXFALSE;
      if (baseFormat0 == GL_RGB) {
         fxMesa->AlphaCombine.Function = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->AlphaCombine.Factor = GR_COMBINE_FACTOR_NONE;
         fxMesa->AlphaCombine.Local = locala;
         fxMesa->AlphaCombine.Other = GR_COMBINE_OTHER_NONE;
         fxMesa->AlphaCombine.Invert = FXFALSE;
      }
      else {
         fxMesa->AlphaCombine.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         fxMesa->AlphaCombine.Factor = GR_COMBINE_FACTOR_ONE;
         fxMesa->AlphaCombine.Local = locala;
         fxMesa->AlphaCombine.Other = GR_COMBINE_OTHER_NONE;
         fxMesa->AlphaCombine.Invert = FXFALSE;
      }
   }
   else if (envMode0 == GL_MODULATE && envMode1 == GL_ADD) {
      /* Quake 3 sky */
      GLboolean isalpha[TDFX_NUM_TMU];

      isalpha[tmu0] = (baseFormat0 == GL_ALPHA);
      isalpha[tmu1] = (baseFormat1 == GL_ALPHA);

      if (isalpha[TDFX_TMU1]) {
         fxMesa->TexCombine[1].FunctionRGB = GR_COMBINE_FUNCTION_ZERO;
         fxMesa->TexCombine[1].FactorRGB = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorAlpha = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].InvertRGB = FXTRUE;
         fxMesa->TexCombine[1].InvertAlpha = FXFALSE;
      }
      else {
         fxMesa->TexCombine[1].FunctionRGB = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorRGB = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorAlpha = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].InvertRGB = FXFALSE;
         fxMesa->TexCombine[1].InvertAlpha = FXFALSE;
      }
      if (isalpha[TDFX_TMU0]) {
         fxMesa->TexCombine[0].FunctionRGB = GR_COMBINE_FUNCTION_SCALE_OTHER;
         fxMesa->TexCombine[0].FactorRGB = GR_COMBINE_FACTOR_ONE;
         fxMesa->TexCombine[0].FunctionAlpha = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
         fxMesa->TexCombine[0].FactorAlpha = GR_COMBINE_FACTOR_ONE;
         fxMesa->TexCombine[0].InvertRGB = FXFALSE;
         fxMesa->TexCombine[0].InvertAlpha = FXFALSE;
      }
      else {
         fxMesa->TexCombine[0].FunctionRGB = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
         fxMesa->TexCombine[0].FactorRGB = GR_COMBINE_FACTOR_ONE;
         fxMesa->TexCombine[0].FunctionAlpha = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
         fxMesa->TexCombine[0].FactorAlpha = GR_COMBINE_FACTOR_ONE;
         fxMesa->TexCombine[0].InvertRGB = FXFALSE;
         fxMesa->TexCombine[0].InvertAlpha = FXFALSE;
      }
      fxMesa->ColorCombine.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
      fxMesa->ColorCombine.Factor = GR_COMBINE_FACTOR_LOCAL;
      fxMesa->ColorCombine.Local = localc;
      fxMesa->ColorCombine.Other = GR_COMBINE_OTHER_TEXTURE;
      fxMesa->ColorCombine.Invert = FXFALSE;
      fxMesa->AlphaCombine.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
      fxMesa->AlphaCombine.Factor = GR_COMBINE_FACTOR_LOCAL;
      fxMesa->AlphaCombine.Local = locala;
      fxMesa->AlphaCombine.Other = GR_COMBINE_OTHER_TEXTURE;
      fxMesa->AlphaCombine.Invert = FXFALSE;
   }
   else if (envMode0 == GL_REPLACE && envMode1 == GL_ADD) {
      /* Vulpine sky */
      GLboolean isalpha[TDFX_NUM_TMU];

      isalpha[tmu0] = (baseFormat0 == GL_ALPHA);
      isalpha[tmu1] = (baseFormat1 == GL_ALPHA);

      if (isalpha[TDFX_TMU1]) {
         fxMesa->TexCombine[1].FunctionRGB = GR_COMBINE_FUNCTION_ZERO;
         fxMesa->TexCombine[1].FactorRGB = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorAlpha = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].InvertRGB = FXTRUE;
         fxMesa->TexCombine[1].InvertAlpha = FXFALSE;
      } else {
         fxMesa->TexCombine[1].FunctionRGB = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorRGB = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorAlpha = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].InvertRGB = FXFALSE;
         fxMesa->TexCombine[1].InvertAlpha = FXFALSE;
      }

      if (isalpha[TDFX_TMU0]) {
         fxMesa->TexCombine[0].FunctionRGB = GR_COMBINE_FUNCTION_SCALE_OTHER;
         fxMesa->TexCombine[0].FactorRGB = GR_COMBINE_FACTOR_ONE;
         fxMesa->TexCombine[0].FunctionAlpha = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
         fxMesa->TexCombine[0].FactorAlpha = GR_COMBINE_FACTOR_ONE;
         fxMesa->TexCombine[0].InvertRGB = FXFALSE;
         fxMesa->TexCombine[0].InvertAlpha = FXFALSE;
      } else {
         fxMesa->TexCombine[0].FunctionRGB = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
         fxMesa->TexCombine[0].FactorRGB = GR_COMBINE_FACTOR_ONE;
         fxMesa->TexCombine[0].FunctionAlpha = GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
         fxMesa->TexCombine[0].FactorAlpha = GR_COMBINE_FACTOR_ONE;
         fxMesa->TexCombine[0].InvertRGB = FXFALSE;
         fxMesa->TexCombine[0].InvertAlpha = FXFALSE;
      }

      fxMesa->ColorCombine.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
      fxMesa->ColorCombine.Factor = GR_COMBINE_FACTOR_ONE;
      fxMesa->ColorCombine.Local = localc;
      fxMesa->ColorCombine.Other = GR_COMBINE_OTHER_TEXTURE;
      fxMesa->ColorCombine.Invert = FXFALSE;
      fxMesa->AlphaCombine.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
      fxMesa->AlphaCombine.Factor = GR_COMBINE_FACTOR_ONE;
      fxMesa->AlphaCombine.Local = locala;
      fxMesa->AlphaCombine.Other = GR_COMBINE_OTHER_TEXTURE;
      fxMesa->AlphaCombine.Invert = FXFALSE;
   }
   else if (envMode1 == GL_REPLACE) {
      /* Homeworld2 */

      fxMesa->TexCombine[1].FunctionRGB = GR_COMBINE_FUNCTION_ZERO;
      fxMesa->TexCombine[1].FactorRGB = GR_COMBINE_FACTOR_NONE;
      fxMesa->TexCombine[1].FunctionAlpha = GR_COMBINE_FUNCTION_ZERO;
      fxMesa->TexCombine[1].FactorAlpha = GR_COMBINE_FACTOR_NONE;
      fxMesa->TexCombine[1].InvertRGB = FXFALSE;
      fxMesa->TexCombine[1].InvertAlpha = FXFALSE;

      fxMesa->TexCombine[0].FunctionRGB = GR_COMBINE_FUNCTION_LOCAL;
      fxMesa->TexCombine[0].FactorRGB = GR_COMBINE_FACTOR_NONE;
      fxMesa->TexCombine[0].FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
      fxMesa->TexCombine[0].FactorAlpha = GR_COMBINE_FACTOR_NONE;
      fxMesa->TexCombine[0].InvertRGB = FXFALSE;
      fxMesa->TexCombine[0].InvertAlpha = FXFALSE;

      if ((baseFormat0 == GL_RGB) && (baseFormat0 == GL_LUMINANCE)) {
         fxMesa->AlphaCombine.Function = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->AlphaCombine.Factor = GR_COMBINE_FACTOR_NONE;
         fxMesa->AlphaCombine.Local = locala;
         fxMesa->AlphaCombine.Other = GR_COMBINE_OTHER_NONE;
         fxMesa->AlphaCombine.Invert = FXFALSE;
      } else {
         fxMesa->AlphaCombine.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         fxMesa->AlphaCombine.Factor = GR_COMBINE_FACTOR_ONE;
         fxMesa->AlphaCombine.Local = locala;
         fxMesa->AlphaCombine.Other = GR_COMBINE_OTHER_TEXTURE;
         fxMesa->AlphaCombine.Invert = FXFALSE;
      }
      if (baseFormat0 == GL_ALPHA) {
         fxMesa->ColorCombine.Function = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->ColorCombine.Factor = GR_COMBINE_FACTOR_NONE;
         fxMesa->ColorCombine.Local = localc;
         fxMesa->ColorCombine.Other = GR_COMBINE_OTHER_NONE;
         fxMesa->ColorCombine.Invert = FXFALSE;
      } else {
         fxMesa->ColorCombine.Function = GR_COMBINE_FUNCTION_SCALE_OTHER;
         fxMesa->ColorCombine.Factor = GR_COMBINE_FACTOR_ONE;
         fxMesa->ColorCombine.Local = localc;
         fxMesa->ColorCombine.Other = GR_COMBINE_OTHER_TEXTURE;
         fxMesa->ColorCombine.Invert = FXFALSE;
      }
   }
   else {
      _mesa_problem(ctx, "%s: Unexpected dual texture mode encountered", __FUNCTION__);
      return GL_FALSE;
   }

   fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_ENV;
   fxMesa->dirty |= TDFX_UPLOAD_COLOR_COMBINE;
   fxMesa->dirty |= TDFX_UPLOAD_ALPHA_COMBINE;
   return GL_TRUE;
}


/*
 * This function makes sure that the correct mipmap levels are loaded
 * in the right places in memory and then makes the Glide calls to
 * setup the texture source pointers.
 */
static void
setupSingleTMU(tdfxContextPtr fxMesa, struct gl_texture_object *tObj)
{
   struct tdfxSharedState *shared = (struct tdfxSharedState *) fxMesa->glCtx->Shared->DriverData;
   tdfxTexInfo *ti = TDFX_TEXTURE_DATA(tObj);
   const GLcontext *ctx = fxMesa->glCtx;

   /* Make sure we're not loaded incorrectly */
   if (ti->isInTM && !shared->umaTexMemory) {
      /* if doing filtering between mipmap levels, alternate mipmap levels
       * must be in alternate TMUs.
       */
      if (ti->LODblend) {
         if (ti->whichTMU != TDFX_TMU_SPLIT)
            tdfxTMMoveOutTM_NoLock(fxMesa, tObj);
      }
      else {
         if (ti->whichTMU == TDFX_TMU_SPLIT)
            tdfxTMMoveOutTM_NoLock(fxMesa, tObj);
      }
   }

   /* Make sure we're loaded correctly */
   if (!ti->isInTM) {
      /* Have to download the texture */
      if (shared->umaTexMemory) {
         tdfxTMMoveInTM_NoLock(fxMesa, tObj, TDFX_TMU0);
      }
      else {
         /* Voodoo3 (split texture memory) */
         if (ti->LODblend) {
            tdfxTMMoveInTM_NoLock(fxMesa, tObj, TDFX_TMU_SPLIT);
         }
         else {
#if 0
            /* XXX putting textures into the second memory bank when the
             * first bank is full is not working at this time.
             */
            if (fxMesa->haveTwoTMUs) {
               GLint memReq = fxMesa->Glide.grTexTextureMemRequired(
                                       GR_MIPMAPLEVELMASK_BOTH, &(ti->info));
               if (shared->freeTexMem[TDFX_TMU0] > memReq) {
                  tdfxTMMoveInTM_NoLock(fxMesa, tObj, TDFX_TMU0);
               }
               else {
                  tdfxTMMoveInTM_NoLock(fxMesa, tObj, TDFX_TMU1);
               }
            }
            else
#endif
            {
               tdfxTMMoveInTM_NoLock(fxMesa, tObj, TDFX_TMU0);
            }
         }
      }
   }

   if (ti->LODblend && ti->whichTMU == TDFX_TMU_SPLIT) {
      /* mipmap levels split between texture banks */
      GLint u;

      if (ti->info.format == GR_TEXFMT_P_8 && !ctx->Texture.SharedPalette) {
         fxMesa->TexPalette.Type = ti->paltype;
         fxMesa->TexPalette.Data = &(ti->palette);
         fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_PALETTE;
      }

      for (u = 0; u < 2; u++) {
         fxMesa->TexParams[u].sClamp = ti->sClamp;
         fxMesa->TexParams[u].tClamp = ti->tClamp;
         fxMesa->TexParams[u].minFilt = ti->minFilt;
         fxMesa->TexParams[u].magFilt = ti->magFilt;
         fxMesa->TexParams[u].mmMode = ti->mmMode;
         fxMesa->TexParams[u].LODblend = ti->LODblend;
         fxMesa->TexParams[u].LodBias = ctx->Texture.Unit[u].LodBias;
      }
      fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_PARAMS;

      fxMesa->TexSource[0].StartAddress = ti->tm[TDFX_TMU0]->startAddr;
      fxMesa->TexSource[0].EvenOdd = GR_MIPMAPLEVELMASK_ODD;
      fxMesa->TexSource[0].Info = &(ti->info);
      fxMesa->TexSource[1].StartAddress = ti->tm[TDFX_TMU1]->startAddr;
      fxMesa->TexSource[1].EvenOdd = GR_MIPMAPLEVELMASK_EVEN;
      fxMesa->TexSource[1].Info = &(ti->info);
      fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_SOURCE;
   }
   else {
      FxU32 tmu;

      if (ti->whichTMU == TDFX_TMU_BOTH)
         tmu = TDFX_TMU0;
      else
         tmu = ti->whichTMU;

      if (shared->umaTexMemory) {
         assert(ti->whichTMU == TDFX_TMU0);
         assert(tmu == TDFX_TMU0);
      }

      if (ti->info.format == GR_TEXFMT_P_8 && !ctx->Texture.SharedPalette) {
         fxMesa->TexPalette.Type = ti->paltype;
         fxMesa->TexPalette.Data = &(ti->palette);
         fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_PALETTE;
      }

      /* KW: The alternative is to do the download to the other tmu.  If
       * we get to this point, I think it means we are thrashing the
       * texture memory, so perhaps it's not a good idea.
       */

      if (fxMesa->TexParams[tmu].sClamp != ti->sClamp ||
          fxMesa->TexParams[tmu].tClamp != ti->tClamp ||
          fxMesa->TexParams[tmu].minFilt != ti->minFilt ||
          fxMesa->TexParams[tmu].magFilt != ti->magFilt ||
          fxMesa->TexParams[tmu].mmMode != ti->mmMode ||
          fxMesa->TexParams[tmu].LODblend != FXFALSE ||
          fxMesa->TexParams[tmu].LodBias != ctx->Texture.Unit[tmu].LodBias) {
         fxMesa->TexParams[tmu].sClamp = ti->sClamp;
         fxMesa->TexParams[tmu].tClamp = ti->tClamp;
         fxMesa->TexParams[tmu].minFilt = ti->minFilt;
         fxMesa->TexParams[tmu].magFilt = ti->magFilt;
         fxMesa->TexParams[tmu].mmMode = ti->mmMode;
         fxMesa->TexParams[tmu].LODblend = FXFALSE;
         fxMesa->TexParams[tmu].LodBias = ctx->Texture.Unit[tmu].LodBias;
         fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_PARAMS;
      }

      /* Glide texture source info */
      fxMesa->TexSource[0].Info = NULL;
      fxMesa->TexSource[1].Info = NULL;
      if (ti->tm[tmu]) {
         fxMesa->TexSource[tmu].StartAddress = ti->tm[tmu]->startAddr;
         fxMesa->TexSource[tmu].EvenOdd = GR_MIPMAPLEVELMASK_BOTH;
         fxMesa->TexSource[tmu].Info = &(ti->info);
         fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_SOURCE;
      }
   }

   fxMesa->sScale0 = ti->sScale;
   fxMesa->tScale0 = ti->tScale;
}

static void
selectSingleTMUSrc(tdfxContextPtr fxMesa, GLint tmu, FxBool LODblend)
{
   if (LODblend) {
      fxMesa->TexCombine[0].FunctionRGB = GR_COMBINE_FUNCTION_BLEND;
      fxMesa->TexCombine[0].FactorRGB = GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION;
      fxMesa->TexCombine[0].FunctionAlpha = GR_COMBINE_FUNCTION_BLEND;
      fxMesa->TexCombine[0].FactorAlpha = GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION;
      fxMesa->TexCombine[0].InvertRGB = FXFALSE;
      fxMesa->TexCombine[0].InvertAlpha = FXFALSE;

      if (fxMesa->haveTwoTMUs) {
         const struct gl_shared_state *mesaShared = fxMesa->glCtx->Shared;
         const struct tdfxSharedState *shared = (struct tdfxSharedState *) mesaShared->DriverData;
         int tmu;

         if (shared->umaTexMemory)
            tmu = GR_TMU0;
         else
            tmu = GR_TMU1;

         fxMesa->TexCombine[tmu].FunctionRGB = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[tmu].FactorRGB = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[tmu].FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[tmu].FactorAlpha = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[tmu].InvertRGB = FXFALSE;
         fxMesa->TexCombine[tmu].InvertAlpha = FXFALSE;
      }
      fxMesa->tmuSrc = TDFX_TMU_SPLIT;
   }
   else {
      if (tmu != TDFX_TMU1) {
         fxMesa->TexCombine[0].FunctionRGB = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[0].FactorRGB = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[0].FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[0].FactorAlpha = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[0].InvertRGB = FXFALSE;
         fxMesa->TexCombine[0].InvertAlpha = FXFALSE;
         if (fxMesa->haveTwoTMUs) {
            fxMesa->TexCombine[1].FunctionRGB = GR_COMBINE_FUNCTION_ZERO;
            fxMesa->TexCombine[1].FactorRGB = GR_COMBINE_FACTOR_NONE;
            fxMesa->TexCombine[1].FunctionAlpha = GR_COMBINE_FUNCTION_ZERO;
            fxMesa->TexCombine[1].FactorAlpha = GR_COMBINE_FACTOR_NONE;
            fxMesa->TexCombine[1].InvertRGB = FXFALSE;
            fxMesa->TexCombine[1].InvertAlpha = FXFALSE;
         }
         fxMesa->tmuSrc = TDFX_TMU0;
      }
      else {
         fxMesa->TexCombine[1].FunctionRGB = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorRGB = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].FunctionAlpha = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->TexCombine[1].FactorAlpha = GR_COMBINE_FACTOR_NONE;
         fxMesa->TexCombine[1].InvertRGB = FXFALSE;
         fxMesa->TexCombine[1].InvertAlpha = FXFALSE;
         /* GR_COMBINE_FUNCTION_SCALE_OTHER doesn't work ?!? */
         fxMesa->TexCombine[0].FunctionRGB = GR_COMBINE_FUNCTION_BLEND;
         fxMesa->TexCombine[0].FactorRGB = GR_COMBINE_FACTOR_ONE;
         fxMesa->TexCombine[0].FunctionAlpha = GR_COMBINE_FUNCTION_BLEND;
         fxMesa->TexCombine[0].FactorAlpha = GR_COMBINE_FACTOR_ONE;
         fxMesa->TexCombine[0].InvertRGB = FXFALSE;
         fxMesa->TexCombine[0].InvertAlpha = FXFALSE;
         fxMesa->tmuSrc = TDFX_TMU1;
      }
   }

   fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_ENV;
}

#if 0
static void print_state(tdfxContextPtr fxMesa)
{
   GLcontext *ctx = fxMesa->glCtx;
   struct gl_texture_object *tObj0 = ctx->Texture.Unit[0]._Current;
   struct gl_texture_object *tObj1 = ctx->Texture.Unit[1]._Current;
   GLenum base0 = tObj0->Image[0][tObj0->BaseLevel] ? tObj0->Image[0][tObj0->BaseLevel]->Format : 99;
   GLenum base1 = tObj1->Image[0][tObj1->BaseLevel] ? tObj1->Image[0][tObj1->BaseLevel]->Format : 99;

   printf("Unit 0: Enabled:  GL=%d   Gr=%d\n", ctx->Texture.Unit[0]._ReallyEnabled,
          fxMesa->TexState.Enabled[0]);
   printf("   EnvMode: GL=0x%x  Gr=0x%x\n", ctx->Texture.Unit[0].EnvMode,
          fxMesa->TexState.EnvMode[0]);
   printf("   BaseFmt: GL=0x%x  Gr=0x%x\n", base0, fxMesa->TexState.TexFormat[0]);


   printf("Unit 1: Enabled:  GL=%d  Gr=%d\n", ctx->Texture.Unit[1]._ReallyEnabled,
          fxMesa->TexState.Enabled[1]);
   printf("   EnvMode: GL=0x%x  Gr:0x%x\n", ctx->Texture.Unit[1].EnvMode,
          fxMesa->TexState.EnvMode[1]);
   printf("   BaseFmt: GL=0x%x  Gr:0x%x\n", base1, fxMesa->TexState.TexFormat[1]);
}
#endif

/*
 * When we're only using a single texture unit, we always use the 0th
 * Glide/hardware unit, regardless if it's GL_TEXTURE0_ARB or GL_TEXTURE1_ARB
 * that's enalbed.
 * Input:  ctx - the context
 *         unit - the OpenGL texture unit to use.
 */
static void setupTextureSingleTMU(GLcontext * ctx, GLuint unit)
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   tdfxTexInfo *ti;
   struct gl_texture_object *tObj;
   int tmu;
   GLenum envMode, baseFormat;

   tObj = ctx->Texture.Unit[unit]._Current;
   if (tObj->Image[0][tObj->BaseLevel]->Border > 0) {
      FALLBACK(fxMesa, TDFX_FALLBACK_TEXTURE_BORDER, GL_TRUE);
      return;
   }

   setupSingleTMU(fxMesa, tObj);

   ti = TDFX_TEXTURE_DATA(tObj);
   if (ti->whichTMU == TDFX_TMU_BOTH)
      tmu = TDFX_TMU0;
   else
      tmu = ti->whichTMU;

   if (fxMesa->tmuSrc != tmu) {
      selectSingleTMUSrc(fxMesa, tmu, ti->LODblend);
   }

   if (ti->reloadImages)
      fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_IMAGES;

   /* Check if we really need to update the texenv state */
   envMode = ctx->Texture.Unit[unit].EnvMode;
   baseFormat = tObj->Image[0][tObj->BaseLevel]->_BaseFormat;

   if (TDFX_IS_NAPALM(fxMesa)) {
      /* see if we really need to update the unit */
      if (1/*fxMesa->TexState.Enabled[unit] != ctx->Texture.Unit[unit]._ReallyEnabled ||
          envMode != fxMesa->TexState.EnvMode[0] ||
          envMode == GL_COMBINE_EXT ||
          baseFormat != fxMesa->TexState.TexFormat[0]*/) {
         struct tdfx_texcombine_ext *otherEnv;
         if (!SetupTexEnvNapalm(ctx, GL_TRUE,
                                &ctx->Texture.Unit[unit], baseFormat,
                                &fxMesa->TexCombineExt[0])) {
            /* software fallback */
            FALLBACK(fxMesa, TDFX_FALLBACK_TEXTURE_ENV, GL_TRUE);
         }
         /* disable other unit */
         otherEnv = &fxMesa->TexCombineExt[1];
         otherEnv->Color.SourceA = GR_CMBX_ZERO;
         otherEnv->Color.ModeA = GR_FUNC_MODE_ZERO;
         otherEnv->Color.SourceB = GR_CMBX_ZERO;
         otherEnv->Color.ModeB = GR_FUNC_MODE_ZERO;
         otherEnv->Color.SourceC = GR_CMBX_ZERO;
         otherEnv->Color.InvertC = FXFALSE;
         otherEnv->Color.SourceD = GR_CMBX_ZERO;
         otherEnv->Color.InvertD = FXFALSE;
         otherEnv->Color.Shift = 0;
         otherEnv->Color.Invert = FXFALSE;
         otherEnv->Alpha.SourceA = GR_CMBX_ITALPHA;
         otherEnv->Alpha.ModeA = GR_FUNC_MODE_ZERO;
         otherEnv->Alpha.SourceB = GR_CMBX_ITALPHA;
         otherEnv->Alpha.ModeB = GR_FUNC_MODE_ZERO;
         otherEnv->Alpha.SourceC = GR_CMBX_ZERO;
         otherEnv->Alpha.InvertC = FXFALSE;
         otherEnv->Alpha.SourceD = GR_CMBX_ZERO;
         otherEnv->Alpha.InvertD = FXFALSE;
         otherEnv->Alpha.Shift = 0;
         otherEnv->Alpha.Invert = FXFALSE;

#if 0/*JJJ*/
         fxMesa->TexState.Enabled[unit] = ctx->Texture.Unit[unit]._ReallyEnabled;
         fxMesa->TexState.EnvMode[0] = envMode;
         fxMesa->TexState.TexFormat[0] = baseFormat;
         fxMesa->TexState.EnvMode[1] = 0;
         fxMesa->TexState.TexFormat[1] = 0;
#endif
      }
   }
   else {
      /* Voodoo3 */

      /* see if we really need to update the unit */
      if (1/*fxMesa->TexState.Enabled[unit] != ctx->Texture.Unit[unit]._ReallyEnabled ||
          envMode != fxMesa->TexState.EnvMode[0] ||
          envMode == GL_COMBINE_EXT ||
          baseFormat != fxMesa->TexState.TexFormat[0]*/) {
         if (!SetupSingleTexEnvVoodoo3(ctx, unit, envMode, baseFormat)) {
            /* software fallback */
            FALLBACK(fxMesa, TDFX_FALLBACK_TEXTURE_ENV, GL_TRUE);
         }
#if 0/*JJJ*/
         fxMesa->TexState.Enabled[unit] = ctx->Texture.Unit[unit]._ReallyEnabled;
         fxMesa->TexState.EnvMode[0] = envMode;
         fxMesa->TexState.TexFormat[0] = baseFormat;
         fxMesa->TexState.EnvMode[1] = 0;
         fxMesa->TexState.TexFormat[1] = 0;
#endif
      }
   }
}


static void
setupDoubleTMU(tdfxContextPtr fxMesa,
               struct gl_texture_object *tObj0,
               struct gl_texture_object *tObj1)
{
#define T0_NOT_IN_TMU  0x01
#define T1_NOT_IN_TMU  0x02
#define T0_IN_TMU0     0x04
#define T1_IN_TMU0     0x08
#define T0_IN_TMU1     0x10
#define T1_IN_TMU1     0x20

    const struct gl_shared_state *mesaShared = fxMesa->glCtx->Shared;
    const struct tdfxSharedState *shared = (struct tdfxSharedState *) mesaShared->DriverData;
    const GLcontext *ctx = fxMesa->glCtx;
    tdfxTexInfo *ti0 = TDFX_TEXTURE_DATA(tObj0);
    tdfxTexInfo *ti1 = TDFX_TEXTURE_DATA(tObj1);
    GLuint tstate = 0;
    int tmu0 = 0, tmu1 = 1;

    if (shared->umaTexMemory) {
       if (!ti0->isInTM) {
          tdfxTMMoveInTM_NoLock(fxMesa, tObj0, TDFX_TMU0);
          assert(ti0->isInTM);
       }
       if (!ti1->isInTM) {
          tdfxTMMoveInTM_NoLock(fxMesa, tObj1, TDFX_TMU0);
          assert(ti1->isInTM);
       }
    }
    else {
       /* We shouldn't need to do this. There is something wrong with
          multitexturing when the TMUs are swapped. So, we're forcing
          them to always be loaded correctly. !!! */
       if (ti0->whichTMU == TDFX_TMU1)
           tdfxTMMoveOutTM_NoLock(fxMesa, tObj0);
       if (ti1->whichTMU == TDFX_TMU0)
           tdfxTMMoveOutTM_NoLock(fxMesa, tObj1);

       if (ti0->isInTM) {
           switch (ti0->whichTMU) {
           case TDFX_TMU0:
               tstate |= T0_IN_TMU0;
               break;
           case TDFX_TMU1:
               tstate |= T0_IN_TMU1;
               break;
           case TDFX_TMU_BOTH:
               tstate |= T0_IN_TMU0 | T0_IN_TMU1;
               break;
           case TDFX_TMU_SPLIT:
               tstate |= T0_NOT_IN_TMU;
               break;
           }
       }
       else
           tstate |= T0_NOT_IN_TMU;

       if (ti1->isInTM) {
           switch (ti1->whichTMU) {
           case TDFX_TMU0:
               tstate |= T1_IN_TMU0;
               break;
           case TDFX_TMU1:
               tstate |= T1_IN_TMU1;
               break;
           case TDFX_TMU_BOTH:
               tstate |= T1_IN_TMU0 | T1_IN_TMU1;
               break;
           case TDFX_TMU_SPLIT:
               tstate |= T1_NOT_IN_TMU;
               break;
           }
       }
       else
           tstate |= T1_NOT_IN_TMU;

       /* Move texture maps into TMUs */

       if (!(((tstate & T0_IN_TMU0) && (tstate & T1_IN_TMU1)) ||
             ((tstate & T0_IN_TMU1) && (tstate & T1_IN_TMU0)))) {
           if (tObj0 == tObj1) {
              tdfxTMMoveInTM_NoLock(fxMesa, tObj1, TDFX_TMU_BOTH);
           }
           else {
               /* Find the minimal way to correct the situation */
               if ((tstate & T0_IN_TMU0) || (tstate & T1_IN_TMU1)) {
                   /* We have one in the standard order, setup the other */
                   if (tstate & T0_IN_TMU0) {
                      /* T0 is in TMU0, put T1 in TMU1 */
                      tdfxTMMoveInTM_NoLock(fxMesa, tObj1, TDFX_TMU1);
                   }
                   else {
                       tdfxTMMoveInTM_NoLock(fxMesa, tObj0, TDFX_TMU0);
                   }
                   /* tmu0 and tmu1 are setup */
               }
               else if ((tstate & T0_IN_TMU1) || (tstate & T1_IN_TMU0)) {
                   /* we have one in the reverse order, setup the other */
                   if (tstate & T1_IN_TMU0) {
                      /* T1 is in TMU0, put T0 in TMU1 */
                      tdfxTMMoveInTM_NoLock(fxMesa, tObj0, TDFX_TMU1);
                   }
                   else {
                       tdfxTMMoveInTM_NoLock(fxMesa, tObj1, TDFX_TMU0);
                   }
                   tmu0 = 1;
                   tmu1 = 0;
               }
               else {              /* Nothing is loaded */
                   tdfxTMMoveInTM_NoLock(fxMesa, tObj0, TDFX_TMU0);
                   tdfxTMMoveInTM_NoLock(fxMesa, tObj1, TDFX_TMU1);
                   /* tmu0 and tmu1 are setup */
               }
           }
       }
    }

    ti0->lastTimeUsed = fxMesa->texBindNumber;
    ti1->lastTimeUsed = fxMesa->texBindNumber;


    if (!ctx->Texture.SharedPalette) {
        if (ti0->info.format == GR_TEXFMT_P_8) {
            fxMesa->TexPalette.Type = ti0->paltype;
            fxMesa->TexPalette.Data = &(ti0->palette);
            fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_PALETTE;
        }
        else if (ti1->info.format == GR_TEXFMT_P_8) {
            fxMesa->TexPalette.Type = ti1->paltype;
            fxMesa->TexPalette.Data = &(ti1->palette);
            fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_PALETTE;
        }
        else {
            fxMesa->TexPalette.Data = NULL;
        }
    }

    /*
     * Setup Unit 0
     */
    assert(ti0->isInTM);
    assert(ti0->tm[tmu0]);
    fxMesa->TexSource[tmu0].StartAddress = ti0->tm[tmu0]->startAddr;
    fxMesa->TexSource[tmu0].EvenOdd = GR_MIPMAPLEVELMASK_BOTH;
    fxMesa->TexSource[tmu0].Info = &(ti0->info);
    fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_SOURCE;

    if (fxMesa->TexParams[tmu0].sClamp != ti0->sClamp ||
        fxMesa->TexParams[tmu0].tClamp != ti0->tClamp ||
        fxMesa->TexParams[tmu0].minFilt != ti0->minFilt ||
        fxMesa->TexParams[tmu0].magFilt != ti0->magFilt ||
        fxMesa->TexParams[tmu0].mmMode != ti0->mmMode ||
        fxMesa->TexParams[tmu0].LODblend != FXFALSE ||
        fxMesa->TexParams[tmu0].LodBias != ctx->Texture.Unit[tmu0].LodBias) {
       fxMesa->TexParams[tmu0].sClamp = ti0->sClamp;
       fxMesa->TexParams[tmu0].tClamp = ti0->tClamp;
       fxMesa->TexParams[tmu0].minFilt = ti0->minFilt;
       fxMesa->TexParams[tmu0].magFilt = ti0->magFilt;
       fxMesa->TexParams[tmu0].mmMode = ti0->mmMode;
       fxMesa->TexParams[tmu0].LODblend = FXFALSE;
       fxMesa->TexParams[tmu0].LodBias = ctx->Texture.Unit[tmu0].LodBias;
       fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_PARAMS;
    }

    /*
     * Setup Unit 1
     */
    if (shared->umaTexMemory) {
        ASSERT(ti1->isInTM);
        ASSERT(ti1->tm[0]);
        fxMesa->TexSource[tmu1].StartAddress = ti1->tm[0]->startAddr;
        fxMesa->TexSource[tmu1].EvenOdd = GR_MIPMAPLEVELMASK_BOTH;
        fxMesa->TexSource[tmu1].Info = &(ti1->info);
    }
    else {
        ASSERT(ti1->isInTM);
        ASSERT(ti1->tm[tmu1]);
        fxMesa->TexSource[tmu1].StartAddress = ti1->tm[tmu1]->startAddr;
        fxMesa->TexSource[tmu1].EvenOdd = GR_MIPMAPLEVELMASK_BOTH;
        fxMesa->TexSource[tmu1].Info = &(ti1->info);
    }

    if (fxMesa->TexParams[tmu1].sClamp != ti1->sClamp ||
        fxMesa->TexParams[tmu1].tClamp != ti1->tClamp ||
        fxMesa->TexParams[tmu1].minFilt != ti1->minFilt ||
        fxMesa->TexParams[tmu1].magFilt != ti1->magFilt ||
        fxMesa->TexParams[tmu1].mmMode != ti1->mmMode ||
        fxMesa->TexParams[tmu1].LODblend != FXFALSE ||
        fxMesa->TexParams[tmu1].LodBias != ctx->Texture.Unit[tmu1].LodBias) {
       fxMesa->TexParams[tmu1].sClamp = ti1->sClamp;
       fxMesa->TexParams[tmu1].tClamp = ti1->tClamp;
       fxMesa->TexParams[tmu1].minFilt = ti1->minFilt;
       fxMesa->TexParams[tmu1].magFilt = ti1->magFilt;
       fxMesa->TexParams[tmu1].mmMode = ti1->mmMode;
       fxMesa->TexParams[tmu1].LODblend = FXFALSE;
       fxMesa->TexParams[tmu1].LodBias = ctx->Texture.Unit[tmu1].LodBias;
       fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_PARAMS;
    }

    fxMesa->sScale0 = ti0->sScale;
    fxMesa->tScale0 = ti0->tScale;
    fxMesa->sScale1 = ti1->sScale;
    fxMesa->tScale1 = ti1->tScale;

#undef T0_NOT_IN_TMU
#undef T1_NOT_IN_TMU
#undef T0_IN_TMU0
#undef T1_IN_TMU0
#undef T0_IN_TMU1
#undef T1_IN_TMU1
}

static void setupTextureDoubleTMU(GLcontext * ctx)
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   struct gl_texture_object *tObj0 = ctx->Texture.Unit[1]._Current;
   struct gl_texture_object *tObj1 = ctx->Texture.Unit[0]._Current;
   tdfxTexInfo *ti0 = TDFX_TEXTURE_DATA(tObj0);
   tdfxTexInfo *ti1 = TDFX_TEXTURE_DATA(tObj1);
   struct gl_texture_image *baseImage0 = tObj0->Image[0][tObj0->BaseLevel];
   struct gl_texture_image *baseImage1 = tObj1->Image[0][tObj1->BaseLevel];
#if 0/*JJJ*/
   const GLenum envMode0 = ctx->Texture.Unit[0].EnvMode;
   const GLenum envMode1 = ctx->Texture.Unit[1].EnvMode;
#endif

   if (baseImage0->Border > 0 || baseImage1->Border > 0) {
      FALLBACK(fxMesa, TDFX_FALLBACK_TEXTURE_BORDER, GL_TRUE);
      return;
   }

   setupDoubleTMU(fxMesa, tObj0, tObj1);

   if (ti0->reloadImages || ti1->reloadImages)
      fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_IMAGES;

   fxMesa->tmuSrc = TDFX_TMU_BOTH;

   if (TDFX_IS_NAPALM(fxMesa)) {
      /* Remember, Glide has its texture units numbered in backward
       * order compared to OpenGL.
       */
      GLboolean hw1 = GL_TRUE, hw2 = GL_TRUE;

      /* check if we really need to update glide unit 1 */
      if (1/*fxMesa->TexState.Enabled[0] != ctx->Texture.Unit[0]._ReallyEnabled ||
          envMode0 != fxMesa->TexState.EnvMode[1] ||
          envMode0 == GL_COMBINE_EXT ||
          baseImage0->Format != fxMesa->TexState.TexFormat[1] ||
          (fxMesa->Fallback & TDFX_FALLBACK_TEXTURE_ENV)*/) {
         hw1 = SetupTexEnvNapalm(ctx, GL_TRUE, &ctx->Texture.Unit[0],
                                baseImage0->_BaseFormat, &fxMesa->TexCombineExt[1]);
#if 0/*JJJ*/
         fxMesa->TexState.EnvMode[1] = envMode0;
         fxMesa->TexState.TexFormat[1] = baseImage0->_BaseFormat;
         fxMesa->TexState.Enabled[0] = ctx->Texture.Unit[0]._ReallyEnabled;
#endif
      }

      /* check if we really need to update glide unit 0 */
      if (1/*fxMesa->TexState.Enabled[1] != ctx->Texture.Unit[1]._ReallyEnabled ||
          envMode1 != fxMesa->TexState.EnvMode[0] ||
          envMode1 == GL_COMBINE_EXT ||
          baseImage1->_BaseFormat != fxMesa->TexState.TexFormat[0] ||
          (fxMesa->Fallback & TDFX_FALLBACK_TEXTURE_ENV)*/) {
         hw2 = SetupTexEnvNapalm(ctx, GL_FALSE, &ctx->Texture.Unit[1],
                                baseImage1->_BaseFormat, &fxMesa->TexCombineExt[0]);
#if 0/*JJJ*/
         fxMesa->TexState.EnvMode[0] = envMode1;
         fxMesa->TexState.TexFormat[0] = baseImage1->_BaseFormat;
         fxMesa->TexState.Enabled[1] = ctx->Texture.Unit[1]._ReallyEnabled;
#endif
      }


      if (!hw1 || !hw2) {
         FALLBACK(fxMesa, TDFX_FALLBACK_TEXTURE_ENV, GL_TRUE);
      }
   }
   else {
      int unit0, unit1;
      if ((ti0->whichTMU == TDFX_TMU1) || (ti1->whichTMU == TDFX_TMU0))
         unit0 = 1;
      else
         unit0 = 0;
      unit1 = 1 - unit0;

      if (1/*fxMesa->TexState.Enabled[0] != ctx->Texture.Unit[0]._ReallyEnabled ||
          fxMesa->TexState.Enabled[1] != ctx->Texture.Unit[1]._ReallyEnabled ||
          envMode0 != fxMesa->TexState.EnvMode[unit0] ||
          envMode0 == GL_COMBINE_EXT ||
          envMode1 != fxMesa->TexState.EnvMode[unit1] ||
          envMode1 == GL_COMBINE_EXT ||
          baseImage0->_BaseFormat != fxMesa->TexState.TexFormat[unit0] ||
          baseImage1->_BaseFormat != fxMesa->TexState.TexFormat[unit1] ||
          (fxMesa->Fallback & TDFX_FALLBACK_TEXTURE_ENV)*/) {

         if (!SetupDoubleTexEnvVoodoo3(ctx, unit0,
                         ctx->Texture.Unit[0].EnvMode, baseImage0->_BaseFormat,
                         ctx->Texture.Unit[1].EnvMode, baseImage1->_BaseFormat)) {
            FALLBACK(fxMesa, TDFX_FALLBACK_TEXTURE_ENV, GL_TRUE);
         }

#if 0/*JJJ*/
         fxMesa->TexState.EnvMode[unit0] = envMode0;
         fxMesa->TexState.TexFormat[unit0] = baseImage0->_BaseFormat;
         fxMesa->TexState.EnvMode[unit1] = envMode1;
         fxMesa->TexState.TexFormat[unit1] = baseImage1->_BaseFormat;
         fxMesa->TexState.Enabled[0] = ctx->Texture.Unit[0]._ReallyEnabled;
         fxMesa->TexState.Enabled[1] = ctx->Texture.Unit[1]._ReallyEnabled;
#endif
      }
   }
}


void
tdfxUpdateTextureState( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   FALLBACK(fxMesa, TDFX_FALLBACK_TEXTURE_BORDER, GL_FALSE);
   FALLBACK(fxMesa, TDFX_FALLBACK_TEXTURE_ENV, GL_FALSE);

   if (ctx->Texture.Unit[0]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT) &&
       ctx->Texture.Unit[1]._ReallyEnabled == 0) {
      LOCK_HARDWARE( fxMesa );  /* XXX remove locking eventually */
      setupTextureSingleTMU(ctx, 0);
      UNLOCK_HARDWARE( fxMesa );
   }
   else if (ctx->Texture.Unit[0]._ReallyEnabled == 0 && 
            ctx->Texture.Unit[1]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT)) {
      LOCK_HARDWARE( fxMesa );
      setupTextureSingleTMU(ctx, 1);
      UNLOCK_HARDWARE( fxMesa );
   }
   else if (ctx->Texture.Unit[0]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT) &&
            ctx->Texture.Unit[1]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT)) {
      LOCK_HARDWARE( fxMesa );
      setupTextureDoubleTMU(ctx);
      UNLOCK_HARDWARE( fxMesa );
   }
   else {
      /* disable hardware texturing */
      if (TDFX_IS_NAPALM(fxMesa)) {
         fxMesa->ColorCombineExt.SourceA = GR_CMBX_ITRGB;
         fxMesa->ColorCombineExt.ModeA = GR_FUNC_MODE_X;
         fxMesa->ColorCombineExt.SourceB = GR_CMBX_ZERO;
         fxMesa->ColorCombineExt.ModeB = GR_FUNC_MODE_ZERO;
         fxMesa->ColorCombineExt.SourceC = GR_CMBX_ZERO;
         fxMesa->ColorCombineExt.InvertC = FXTRUE;
         fxMesa->ColorCombineExt.SourceD = GR_CMBX_ZERO;
         fxMesa->ColorCombineExt.InvertD = FXFALSE;
         fxMesa->ColorCombineExt.Shift = 0;
         fxMesa->ColorCombineExt.Invert = FXFALSE;
         fxMesa->AlphaCombineExt.SourceA = GR_CMBX_ITALPHA;
         fxMesa->AlphaCombineExt.ModeA = GR_FUNC_MODE_X;
         fxMesa->AlphaCombineExt.SourceB = GR_CMBX_ZERO;
         fxMesa->AlphaCombineExt.ModeB = GR_FUNC_MODE_ZERO;
         fxMesa->AlphaCombineExt.SourceC = GR_CMBX_ZERO;
         fxMesa->AlphaCombineExt.InvertC = FXTRUE;
         fxMesa->AlphaCombineExt.SourceD = GR_CMBX_ZERO;
         fxMesa->AlphaCombineExt.InvertD = FXFALSE;
         fxMesa->AlphaCombineExt.Shift = 0;
         fxMesa->AlphaCombineExt.Invert = FXFALSE;
      }
      else {
         /* Voodoo 3*/
         fxMesa->ColorCombine.Function = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->ColorCombine.Factor = GR_COMBINE_FACTOR_NONE;
         fxMesa->ColorCombine.Local = GR_COMBINE_LOCAL_ITERATED;
         fxMesa->ColorCombine.Other = GR_COMBINE_OTHER_NONE;
         fxMesa->ColorCombine.Invert = FXFALSE;
         fxMesa->AlphaCombine.Function = GR_COMBINE_FUNCTION_LOCAL;
         fxMesa->AlphaCombine.Factor = GR_COMBINE_FACTOR_NONE;
         fxMesa->AlphaCombine.Local = GR_COMBINE_LOCAL_ITERATED;
         fxMesa->AlphaCombine.Other = GR_COMBINE_OTHER_NONE;
         fxMesa->AlphaCombine.Invert = FXFALSE;
      }

      fxMesa->TexState.Enabled[0] = 0;
      fxMesa->TexState.Enabled[1] = 0;
      fxMesa->TexState.EnvMode[0] = 0;
      fxMesa->TexState.EnvMode[1] = 0;

      fxMesa->dirty |= TDFX_UPLOAD_COLOR_COMBINE;
      fxMesa->dirty |= TDFX_UPLOAD_ALPHA_COMBINE;

      if (ctx->Texture.Unit[0]._ReallyEnabled != 0 ||
          ctx->Texture.Unit[1]._ReallyEnabled != 0) {
         /* software texture (cube map, rect tex, etc */
         FALLBACK(fxMesa, TDFX_FALLBACK_TEXTURE_ENV, GL_TRUE);
      }
   }
}



/*
 * This is a special case of texture state update.
 * It's used when we've simply bound a new texture to a texture
 * unit and the new texture has the exact same attributes as the
 * previously bound texture.
 * This is very common in Quake3.
 */
void
tdfxUpdateTextureBinding( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   struct gl_texture_object *tObj0 = ctx->Texture.Unit[0]._Current;
   struct gl_texture_object *tObj1 = ctx->Texture.Unit[1]._Current;
   tdfxTexInfo *ti0 = TDFX_TEXTURE_DATA(tObj0);
   tdfxTexInfo *ti1 = TDFX_TEXTURE_DATA(tObj1);

    const struct gl_shared_state *mesaShared = fxMesa->glCtx->Shared;
    const struct tdfxSharedState *shared = (struct tdfxSharedState *) mesaShared->DriverData;

   if (ti0) {
      fxMesa->sScale0 = ti0->sScale;
      fxMesa->tScale0 = ti0->tScale;
      if (ti0->info.format == GR_TEXFMT_P_8) {
         fxMesa->TexPalette.Type = ti0->paltype;
         fxMesa->TexPalette.Data = &(ti0->palette);
         fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_PALETTE;
      }
      else if (ti1 && ti1->info.format == GR_TEXFMT_P_8) {
         fxMesa->TexPalette.Type = ti1->paltype;
         fxMesa->TexPalette.Data = &(ti1->palette);
         fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_PALETTE;
      }
   }
   if (ti1) {
      fxMesa->sScale1 = ti1->sScale;
      fxMesa->tScale1 = ti1->tScale;
   }

   if (ctx->Texture.Unit[0]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT) &&
       ctx->Texture.Unit[0]._ReallyEnabled == 0) {
      /* Only unit 0 2D enabled */
      if (shared->umaTexMemory) {
         fxMesa->TexSource[0].StartAddress = ti0->tm[0]->startAddr;
         fxMesa->TexSource[0].EvenOdd = GR_MIPMAPLEVELMASK_BOTH;
         fxMesa->TexSource[0].Info = &(ti0->info);
      }
      else {
         if (ti0->LODblend && ti0->whichTMU == TDFX_TMU_SPLIT) {
            fxMesa->TexSource[0].StartAddress = ti0->tm[TDFX_TMU0]->startAddr;
            fxMesa->TexSource[0].EvenOdd = GR_MIPMAPLEVELMASK_ODD;
            fxMesa->TexSource[0].Info = &(ti0->info);
            fxMesa->TexSource[1].StartAddress = ti0->tm[TDFX_TMU1]->startAddr;
            fxMesa->TexSource[1].EvenOdd = GR_MIPMAPLEVELMASK_EVEN;
            fxMesa->TexSource[1].Info = &(ti0->info);
         }
         else {
            FxU32 tmu;
            if (ti0->whichTMU == TDFX_TMU_BOTH)
               tmu = TDFX_TMU0;
            else
               tmu = ti0->whichTMU;
            fxMesa->TexSource[0].Info = NULL;
            fxMesa->TexSource[1].Info = NULL;
            if (ti0->tm[tmu]) {
               fxMesa->TexSource[tmu].StartAddress = ti0->tm[tmu]->startAddr;
               fxMesa->TexSource[tmu].EvenOdd = GR_MIPMAPLEVELMASK_BOTH;
               fxMesa->TexSource[tmu].Info = &(ti0->info);
            }
         }
      }
   }
   else if (ctx->Texture.Unit[0]._ReallyEnabled == 0 && 
            ctx->Texture.Unit[0]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT)) {
      /* Only unit 1 2D enabled */
      if (shared->umaTexMemory) {
         fxMesa->TexSource[0].StartAddress = ti1->tm[0]->startAddr;
         fxMesa->TexSource[0].EvenOdd = GR_MIPMAPLEVELMASK_BOTH;
         fxMesa->TexSource[0].Info = &(ti1->info);
      }
   }
   else if (ctx->Texture.Unit[0]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT) && 
            ctx->Texture.Unit[0]._ReallyEnabled & (TEXTURE_1D_BIT|TEXTURE_2D_BIT)) {
      /* Both 2D enabled */
      if (shared->umaTexMemory) {
         const FxU32 tmu0 = 0, tmu1 = 1;
         fxMesa->TexSource[tmu0].StartAddress = ti0->tm[0]->startAddr;
         fxMesa->TexSource[tmu0].EvenOdd = GR_MIPMAPLEVELMASK_BOTH;
         fxMesa->TexSource[tmu0].Info = &(ti0->info);

         fxMesa->TexSource[tmu1].StartAddress = ti1->tm[0]->startAddr;
         fxMesa->TexSource[tmu1].EvenOdd = GR_MIPMAPLEVELMASK_BOTH;
         fxMesa->TexSource[tmu1].Info = &(ti1->info);
      }
      else {
         const FxU32 tmu0 = 0, tmu1 = 1;
         fxMesa->TexSource[tmu0].StartAddress = ti0->tm[tmu0]->startAddr;
         fxMesa->TexSource[tmu0].EvenOdd = GR_MIPMAPLEVELMASK_BOTH;
         fxMesa->TexSource[tmu0].Info = &(ti0->info);

         fxMesa->TexSource[tmu1].StartAddress = ti1->tm[tmu1]->startAddr;
         fxMesa->TexSource[tmu1].EvenOdd = GR_MIPMAPLEVELMASK_BOTH;
         fxMesa->TexSource[tmu1].Info = &(ti1->info);
      }
   }


   fxMesa->dirty |= TDFX_UPLOAD_TEXTURE_SOURCE;
}
