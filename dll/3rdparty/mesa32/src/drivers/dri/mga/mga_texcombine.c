/*
 * Copyright (c) 2003 Ville Syrjala
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Ville Syrjala <syrjala@sci.fi>
 */

#include "glheader.h"

#include "mgacontext.h"
#include "mgatex.h"
#include "mgaregs.h"

/*
 * GL_ARB_texture_env_combine
 * GL_EXT_texture_env_combine
 * GL_ARB_texture_env_crossbar
 * GL_ATI_texture_env_combine3
 */

#define ARG_DISABLE 0xffffffff
#define MGA_ARG1  0
#define MGA_ARG2  1
#define MGA_ALPHA 2

GLboolean mgaUpdateTextureEnvCombine( GLcontext *ctx, int unit )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   const int source = mmesa->tmu_source[unit];
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[source];
   GLuint *reg = ((GLuint *)&mmesa->setup.tdualstage0 + unit);
   GLuint numColorArgs = 0, numAlphaArgs = 0;
   GLuint arg1[3], arg2[3], alpha[3];
   int args[3];
   int i;

   switch (texUnit->Combine.ModeRGB) {
   case GL_REPLACE:
      numColorArgs = 1;
      break;
   case GL_MODULATE:
   case GL_ADD:
   case GL_ADD_SIGNED:
   case GL_SUBTRACT:
      numColorArgs = 2;
      break;
   case GL_INTERPOLATE:
   case GL_MODULATE_ADD_ATI:
   case GL_MODULATE_SIGNED_ADD_ATI:
   case GL_MODULATE_SUBTRACT_ATI:
      numColorArgs = 3;
      break;
   default:
      return GL_FALSE;
   }

   switch (texUnit->Combine.ModeA) {
   case GL_REPLACE:
      numAlphaArgs = 1;
      break;
   case GL_MODULATE:
   case GL_ADD:
   case GL_ADD_SIGNED:
   case GL_SUBTRACT:
      numAlphaArgs = 2;
      break;
   default:
      return GL_FALSE;
   }

   /* Start fresh :) */
   *reg = 0;

   /* COLOR */
   for (i = 0; i < 3; i++) {
      arg1[i] = 0;
      arg2[i] = 0;
      alpha[i] = 0;
   }

   for (i = 0;i < numColorArgs; i++) {
      switch (texUnit->Combine.SourceRGB[i]) {
      case GL_TEXTURE:
         arg1[i] |= 0;
         arg2[i] |= ARG_DISABLE;
         alpha[i] |= TD0_color_alpha_currtex;
         break;
      case GL_TEXTURE0:
         if (source == 0) {
            arg1[i] |= 0;
            arg2[i] |= ARG_DISABLE;
            alpha[i] |= TD0_color_alpha_currtex;
         } else {
            if (ctx->Texture._EnabledUnits != 0x03) {
               /* disable texturing */
               mmesa->setup.dwgctl &= DC_opcod_MASK;
               mmesa->setup.dwgctl |= DC_opcod_trap;
               mmesa->hw.alpha_sel = AC_alphasel_diffused;
               /* return GL_TRUE since we don't need a fallback */
               return GL_TRUE;
            }
            arg1[i] |= ARG_DISABLE;
            arg2[i] |= ARG_DISABLE;
            alpha[i] |= TD0_color_alpha_prevtex;
         }
         break;
      case GL_TEXTURE1:
         if (source == 0) {
            if (ctx->Texture._EnabledUnits != 0x03) {
               /* disable texturing */
               mmesa->setup.dwgctl &= DC_opcod_MASK;
               mmesa->setup.dwgctl |= DC_opcod_trap;
               mmesa->hw.alpha_sel = AC_alphasel_diffused;
               /* return GL_TRUE since we don't need a fallback */
               return GL_TRUE;
            }
            arg1[i] |= ARG_DISABLE;
            /* G400 specs (TDUALSTAGE0) */
            arg2[i] |= TD0_color_arg2_prevstage;
            alpha[i] |= TD0_color_alpha_prevstage;
         } else {
            arg1[i] |= 0;
            arg2[i] |= ARG_DISABLE;
            alpha[i] |= TD0_color_alpha_currtex;
         }
         break;
      case GL_CONSTANT:
         if (mmesa->fcol_used &&
             mmesa->envcolor[source] != mmesa->envcolor[!source])
            return GL_FALSE;

         arg1[i] |= ARG_DISABLE;
         arg2[i] |= TD0_color_arg2_fcol;
         alpha[i] |= TD0_color_alpha_fcol;

         mmesa->setup.fcol = mmesa->envcolor[source];
         mmesa->fcol_used = GL_TRUE;
         break;
      case GL_PRIMARY_COLOR:
         arg1[i] |= ARG_DISABLE;
         /* G400 specs (TDUALSTAGE1) */
         if (unit == 0 || (mmesa->setup.tdualstage0 &
                           ((TD0_color_sel_mul & TD0_color_sel_add) |
                            (TD0_alpha_sel_mul & TD0_alpha_sel_add)))) {
            arg2[i] |= TD0_color_arg2_diffuse;
            alpha[i] |= TD0_color_alpha_diffuse;
         } else {
            arg2[i] |= ARG_DISABLE;
            alpha[i] |= ARG_DISABLE;
         }
         break;
      case GL_PREVIOUS:
         arg1[i] |= ARG_DISABLE;
         if (unit == 0) {
            arg2[i] |= TD0_color_arg2_diffuse;
            alpha[i] |= TD0_color_alpha_diffuse;
         } else {
            arg2[i] |= TD0_color_arg2_prevstage;
            alpha[i] |= TD0_color_alpha_prevstage;
         }
         break;
      default:
         return GL_FALSE;
      }

      switch (texUnit->Combine.OperandRGB[i]) {
      case GL_SRC_COLOR:
         arg1[i] |= 0;
         arg2[i] |= 0;
         if (texUnit->Combine.SourceRGB[i] == GL_CONSTANT &&
             RGBA_EQUAL( mmesa->envcolor[source] )) {
            alpha[i] |= 0;
         } else {
            alpha[i] |= ARG_DISABLE;
         }
         break;
      case GL_ONE_MINUS_SRC_COLOR:
         arg1[i] |= TD0_color_arg1_inv_enable;
         arg2[i] |= TD0_color_arg2_inv_enable;
         if (texUnit->Combine.SourceRGB[i] == GL_CONSTANT &&
             RGBA_EQUAL( mmesa->envcolor[source] )) {
            alpha[i] |= (TD0_color_alpha1inv_enable |
                         TD0_color_alpha2inv_enable);
         } else {
            alpha[i] |= ARG_DISABLE;
         }
         break;
      case GL_SRC_ALPHA:
         arg1[i] |= TD0_color_arg1_replicatealpha_enable;
         arg2[i] |= TD0_color_arg2_replicatealpha_enable;
         alpha[i] |= 0;
         break;
      case GL_ONE_MINUS_SRC_ALPHA:
         arg1[i] |= (TD0_color_arg1_replicatealpha_enable |
                     TD0_color_arg1_inv_enable);
         arg2[i] |= (TD0_color_arg2_replicatealpha_enable |
                     TD0_color_arg2_inv_enable);
         alpha[i] |= (TD0_color_alpha1inv_enable |
                      TD0_color_alpha2inv_enable);
         break;
      }
   }

   switch (texUnit->Combine.ModeRGB) {
   case GL_MODULATE_ADD_ATI:
   case GL_MODULATE_SIGNED_ADD_ATI:
      /* Special handling for ATI_texture_env_combine3.
       * If Arg1 == Arg0 or Arg1 == Arg2 we can use arg1 or arg2 as input for
       * both multiplier and adder.
       */
      /* Arg1 == arg1 */
      if (arg1[1] == arg1[0]) {
         if ((arg1[1] | arg2[2]) != ARG_DISABLE) {
            *reg |= arg1[1] | arg2[2];
            args[0] = MGA_ARG1; args[1] = MGA_ARG1; args[2] = MGA_ARG2;
            break;
         } else
         if ((arg1[1] | alpha[2]) != ARG_DISABLE) {
            *reg |= arg1[1] | alpha[2];
            args[0] = MGA_ARG1; args[1] = MGA_ARG1; args[2] = MGA_ALPHA;
            break;
         }
      }
      if (arg1[1] == arg1[2]) {
         if ((arg1[1] | arg2[0]) != ARG_DISABLE) {
            *reg |= arg1[1] | arg2[0];
            args[0] = MGA_ARG2; args[1] = MGA_ARG1; args[2] = MGA_ARG1;
            break;
         } else
         if ((arg1[1] | alpha[0]) != ARG_DISABLE) {
            *reg |= arg1[1] | alpha[0];
            args[0] = MGA_ALPHA; args[1] = MGA_ARG1; args[2] = MGA_ARG1;
            break;
         }
      }
      /* fallthrough */
   case GL_MODULATE_SUBTRACT_ATI:
      /* Arg1 == arg2 */
      if (arg2[1] == arg2[0]) {
         if ((arg2[1] | arg1[2]) != ARG_DISABLE) {
            *reg |= arg2[1] | arg1[2];
            args[0] = MGA_ARG2; args[1] = MGA_ARG2; args[2] = MGA_ARG1;
            break;
         } else
         if ((arg2[1] | alpha[2]) != ARG_DISABLE) {
            *reg |= arg2[1] | alpha[2];
            args[0] = MGA_ARG2; args[1] = MGA_ARG2; args[2] = MGA_ALPHA;
            break;
         }
      }
      if (arg2[1] == arg2[2]) {
         if ((arg2[1] | arg1[0]) != ARG_DISABLE) {
            *reg |= arg2[1] | arg1[0];
            args[0] = MGA_ARG1; args[1] = MGA_ARG2; args[2] = MGA_ARG2;
            break;
         } else
         if ((arg2[1] | alpha[0]) != ARG_DISABLE) {
            *reg |= arg2[1] | alpha[0];
            args[0] = MGA_ALPHA; args[1] = MGA_ARG2; args[2] = MGA_ARG2;
            break;
         }
      }
      /* fallthrough */
   default:
      /* Find working combo of arg1, arg2 and alpha.
       *
       * Keep the Arg0 != alpha cases first since there's
       * no way to get alpha out by itself (GL_REPLACE).
       *
       * Keep the Arg2 == alpha cases first because only alpha has the
       * capabilities to function as Arg2 (GL_INTERPOLATE). Also good for 
       * GL_ADD, GL_ADD_SIGNED, GL_SUBTRACT since we can't get alpha to the
       * adder.
       *
       * Keep the Arg1 == alpha cases last for GL_MODULATE_ADD_ATI,
       * GL_MODULATE_SIGNED_ADD_ATI. Again because we can't get alpha to the
       * adder.
       *
       * GL_MODULATE_SUBTRACT_ATI needs special treatment since it requires
       * that Arg1 == arg2. This requirement clashes with those of other modes.
       */
      if ((arg1[0] | arg2[1] | alpha[2]) != ARG_DISABLE) {
         *reg |= arg1[0] | arg2[1] | alpha[2];
         args[0] = MGA_ARG1; args[1] = MGA_ARG2; args[2] = MGA_ALPHA;
      } else
      if ((arg1[1] | arg2[0] | alpha[2]) != ARG_DISABLE &&
          texUnit->Combine.ModeRGB != GL_MODULATE_SUBTRACT_ATI) {
         *reg |= arg1[1] | arg2[0] | alpha[2];
         args[0] = MGA_ARG2; args[1] = MGA_ARG1; args[2] = MGA_ALPHA;
      } else
      if ((arg1[1] | arg2[2] | alpha[0]) != ARG_DISABLE &&
          texUnit->Combine.ModeRGB != GL_MODULATE_SUBTRACT_ATI) {
         *reg |= arg1[1] | arg2[2] | alpha[0];
         args[0] = MGA_ALPHA; args[1] = MGA_ARG1; args[2] = MGA_ARG2;
      } else
      if ((arg1[2] | arg2[1] | alpha[0]) != ARG_DISABLE) {
         *reg |= arg1[2] | arg2[1] | alpha[0];
         args[0] = MGA_ALPHA; args[1] = MGA_ARG2; args[2] = MGA_ARG1;
      } else
      if ((arg1[0] | arg2[2] | alpha[1]) != ARG_DISABLE) {
         *reg |= arg1[0] | arg2[2] | alpha[1];
         args[0] = MGA_ARG1; args[1] = MGA_ALPHA; args[2] = MGA_ARG2;
      } else
      if ((arg1[2] | arg2[0] | alpha[1]) != ARG_DISABLE) {
         *reg |= arg1[2] | arg2[0] | alpha[1];
         args[0] = MGA_ARG2; args[1] = MGA_ALPHA; args[2] = MGA_ARG1;
      } else {
         /* nothing suitable */
         return GL_FALSE;
      }
   }

   switch (texUnit->Combine.ModeRGB) {
   case GL_REPLACE:
      if (texUnit->Combine.ScaleShiftRGB) {
         return GL_FALSE;
      }

      if (args[0] == MGA_ARG1) {
         *reg |= TD0_color_sel_arg1;
      } else if (args[0] == MGA_ARG2) {
         *reg |= TD0_color_sel_arg2;
      } else if (args[0] == MGA_ALPHA) {
         /* Can't get alpha out by itself */
         return GL_FALSE;
      }
      break;
   case GL_MODULATE:
      if (texUnit->Combine.ScaleShiftRGB == 1) {
         *reg |= TD0_color_modbright_2x;
      } else if (texUnit->Combine.ScaleShiftRGB == 2) {
         *reg |= TD0_color_modbright_4x;
      }

      *reg |= TD0_color_sel_mul;

      if (args[0] == MGA_ALPHA || args[1] == MGA_ALPHA) {
         if (args[0] == MGA_ARG1 || args[1] == MGA_ARG1) {
            *reg |= TD0_color_arg2mul_alpha2;
         } else if (args[0] == MGA_ARG2 || args[1] == MGA_ARG2) {
            *reg |= TD0_color_arg1mul_alpha1;
         }
      }
      break;
   case GL_ADD_SIGNED:
      *reg |= TD0_color_addbias_enable;
      /* fallthrough */
   case GL_ADD:
      if (args[0] == MGA_ALPHA || args[1] == MGA_ALPHA) {
         /* Can't get alpha to the adder */
         return GL_FALSE;
      }
      if (texUnit->Combine.ScaleShiftRGB == 1) {
         *reg |= TD0_color_add2x_enable;
      } else if (texUnit->Combine.ScaleShiftRGB == 2) {
         return GL_FALSE;
      }

      *reg |= (TD0_color_add_add |
               TD0_color_sel_add);
      break;
   case GL_INTERPOLATE:
      if (args[2] != MGA_ALPHA) {
         /* Only alpha can function as Arg2 */
         return GL_FALSE;
      }
      if (texUnit->Combine.ScaleShiftRGB == 1) {
         *reg |= TD0_color_add2x_enable;
      } else if (texUnit->Combine.ScaleShiftRGB == 2) {
         return GL_FALSE;
      }

      *reg |= (TD0_color_arg1mul_alpha1 |
               TD0_color_blend_enable |
               TD0_color_arg1add_mulout |
               TD0_color_arg2add_mulout |
               TD0_color_add_add |
               TD0_color_sel_add);

      /* Have to do this with xor since GL_ONE_MINUS_SRC_ALPHA may have
       * already touched this bit.
       */
      *reg ^= TD0_color_alpha1inv_enable;

      if (args[0] == MGA_ARG2) {
         /* Swap arguments */
         *reg ^= (TD0_color_arg1mul_alpha1 |
                  TD0_color_arg2mul_alpha2 |
                  TD0_color_alpha1inv_enable |
                  TD0_color_alpha2inv_enable);
      }

      if (ctx->Texture._EnabledUnits != 0x03) {
         /* Linear blending mode needs dualtex enabled */
         *(reg+1) = (TD0_color_arg2_prevstage |
                     TD0_color_sel_arg2 |
                     TD0_alpha_arg2_prevstage |
                     TD0_alpha_sel_arg2);
         mmesa->force_dualtex = GL_TRUE;
      }
      break;
   case GL_SUBTRACT:
      if (args[0] == MGA_ALPHA || args[1] == MGA_ALPHA) {
         /* Can't get alpha to the adder */
         return GL_FALSE;
      }
      if (texUnit->Combine.ScaleShiftRGB == 1) {
         *reg |= TD0_color_add2x_enable;
      } else if (texUnit->Combine.ScaleShiftRGB == 2) {
         return GL_FALSE;
      }

      *reg |= (TD0_color_add_sub |
               TD0_color_sel_add);

      if (args[0] == MGA_ARG2) {
         /* Swap arguments */
         *reg ^= (TD0_color_arg1_inv_enable |
                  TD0_color_arg2_inv_enable);
      }
      break;
   case GL_MODULATE_SIGNED_ADD_ATI:
      *reg |= TD0_color_addbias_enable;
      /* fallthrough */
   case GL_MODULATE_ADD_ATI:
      if (args[1] == MGA_ALPHA) {
         /* Can't get alpha to the adder */
         return GL_FALSE;
      }
      if (texUnit->Combine.ScaleShiftRGB == 1) {
         *reg |= TD0_color_add2x_enable;
      } else if (texUnit->Combine.ScaleShiftRGB == 2) {
         return GL_FALSE;
      }

      *reg |= (TD0_color_add_add |
               TD0_color_sel_add);

      if (args[1] == args[0] || args[1] == args[2]) {
         *reg |= TD0_color_arg1add_mulout;
         if (args[0] == MGA_ALPHA || args[2] == MGA_ALPHA)
            *reg |= TD0_color_arg1mul_alpha1;

         if (args[1] == MGA_ARG1) {
            /* Swap adder arguments */
            *reg ^= (TD0_color_arg1add_mulout |
                     TD0_color_arg2add_mulout);
            if (args[0] == MGA_ALPHA || args[2] == MGA_ALPHA) {
               /* Swap multiplier arguments */
               *reg ^= (TD0_color_arg1mul_alpha1 |
                        TD0_color_arg2mul_alpha2);
            }
         }
      } else {
         *reg |= (TD0_color_arg2mul_alpha2 |
                  TD0_color_arg1add_mulout);

         if (args[1] == MGA_ARG1) {
            /* Swap arguments */
            *reg ^= (TD0_color_arg1mul_alpha1 |
                     TD0_color_arg2mul_alpha2 |
                     TD0_color_arg1add_mulout |
                     TD0_color_arg2add_mulout);
         }
      }
      break;
   case GL_MODULATE_SUBTRACT_ATI:
      if (args[1] != MGA_ARG2) {
         /* Can't swap arguments */
         return GL_FALSE;
      }
      if (texUnit->Combine.ScaleShiftRGB == 1) {
         *reg |= TD0_color_add2x_enable;
      } else if (texUnit->Combine.ScaleShiftRGB == 2) {
         return GL_FALSE;
      }

      *reg |= (TD0_color_add_sub |
               TD0_color_sel_add);

      if (args[1] == args[0] || args[1] == args[2]) {
         *reg |= TD0_color_arg1add_mulout;
         if (args[0] == MGA_ALPHA || args[2] == MGA_ALPHA)
            *reg |= TD0_color_arg1mul_alpha1;
      } else {
         *reg |= (TD0_color_arg2mul_alpha2 |
                  TD0_color_arg1add_mulout);
      }
      break;
   }


   /* ALPHA */
   for (i = 0; i < 2; i++) {
      arg1[i] = 0;
      arg2[i] = 0;
   }

   for (i = 0; i < numAlphaArgs; i++) {
      switch (texUnit->Combine.SourceA[i]) {
      case GL_TEXTURE:
         arg1[i] |= 0;
         arg2[i] |= ARG_DISABLE;
         break;
      case GL_TEXTURE0:
         if (source == 0) {
            arg1[i] |= 0;
            arg2[i] |= ARG_DISABLE;
         } else {
            if (ctx->Texture._EnabledUnits != 0x03) {
               /* disable texturing */
               mmesa->setup.dwgctl &= DC_opcod_MASK;
               mmesa->setup.dwgctl |= DC_opcod_trap;
               mmesa->hw.alpha_sel = AC_alphasel_diffused;
               /* return GL_TRUE since we don't need a fallback */
               return GL_TRUE;
            }
            arg1[i] |= ARG_DISABLE;
            arg2[i] |= TD0_alpha_arg2_prevtex;
         }
         break;
      case GL_TEXTURE1:
         if (source == 0) {
            if (ctx->Texture._EnabledUnits != 0x03) {
               /* disable texturing */
               mmesa->setup.dwgctl &= DC_opcod_MASK;
               mmesa->setup.dwgctl |= DC_opcod_trap;
               mmesa->hw.alpha_sel = AC_alphasel_diffused;
               /* return GL_TRUE since we don't need a fallback */
               return GL_TRUE;
            }
            arg1[i] |= ARG_DISABLE;
            /* G400 specs (TDUALSTAGE0) */
            arg2[i] |= TD0_alpha_arg2_prevstage;
         } else {
            arg1[i] |= 0;
            arg2[i] |= ARG_DISABLE;
         }
         break;
      case GL_CONSTANT:
         if (mmesa->fcol_used &&
             mmesa->envcolor[source] != mmesa->envcolor[!source])
            return GL_FALSE;

         arg1[i] |= ARG_DISABLE;
         arg2[i] |= TD0_alpha_arg2_fcol;

         mmesa->setup.fcol = mmesa->envcolor[source];
         mmesa->fcol_used = GL_TRUE;
         break;
      case GL_PRIMARY_COLOR:
         arg1[i] |= ARG_DISABLE;
         /* G400 specs (TDUALSTAGE1) */
         if (unit == 0 || (mmesa->setup.tdualstage0 &
                           ((TD0_color_sel_mul & TD0_color_sel_add) |
                            (TD0_alpha_sel_mul & TD0_alpha_sel_add)))) {
            arg2[i] |= TD0_alpha_arg2_diffuse;
         } else {
            arg2[i] |= ARG_DISABLE;
         }
         break;
      case GL_PREVIOUS:
         arg1[i] |= ARG_DISABLE;
         if (unit == 0) {
            arg2[i] |= TD0_alpha_arg2_diffuse;
         } else {
            arg2[i] |= TD0_alpha_arg2_prevstage;
         }
         break;
      default:
         return GL_FALSE;
      }

      switch (texUnit->Combine.OperandA[i]) {
      case GL_SRC_ALPHA:
         arg1[i] |= 0;
         arg2[i] |= 0;
         break;
      case GL_ONE_MINUS_SRC_ALPHA:
         arg1[i] |= TD0_alpha_arg1_inv_enable;
         arg2[i] |= TD0_alpha_arg2_inv_enable;
         break;
      }
   }

   /* Find a working combo of arg1 and arg2 */
   if ((arg1[0] | arg2[1]) != ARG_DISABLE) {
      *reg |= arg1[0] | arg2[1];
      args[0] = MGA_ARG1; args[1] = MGA_ARG2;
   } else
   if ((arg1[1] | arg2[0]) != ARG_DISABLE) {
      *reg |= arg1[1] | arg2[0];
      args[0] = MGA_ARG2; args[1] = MGA_ARG1;
   } else {
      /* nothing suitable */
      return GL_FALSE;
   }

   switch (texUnit->Combine.ModeA) {
   case GL_REPLACE:
      if (texUnit->Combine.ScaleShiftA) {
         return GL_FALSE;
      }

      if (args[0] == MGA_ARG1) {
         *reg |= TD0_alpha_sel_arg1;
      } else if (args[0] == MGA_ARG2) {
         *reg |= TD0_alpha_sel_arg2;
      }
      break;
   case GL_MODULATE:
      if (texUnit->Combine.ScaleShiftA == 1) {
         *reg |= TD0_alpha_modbright_2x;
      } else if (texUnit->Combine.ScaleShiftA == 2) {
         *reg |= TD0_alpha_modbright_4x;
      }

      *reg |= TD0_alpha_sel_mul;
      break;
   case GL_ADD_SIGNED:
      *reg |= TD0_alpha_addbias_enable;
      /* fallthrough */
   case GL_ADD:
      if (texUnit->Combine.ScaleShiftA == 1) {
         *reg |= TD0_alpha_add2x_enable;
      } else if (texUnit->Combine.ScaleShiftA == 2) {
         return GL_FALSE;
      }

      *reg |= (TD0_alpha_add_enable |
               TD0_alpha_sel_add);
      break;
   case GL_SUBTRACT:
      if (texUnit->Combine.ScaleShiftA == 1) {
         *reg |= TD0_alpha_add2x_enable;
      } else if (texUnit->Combine.ScaleShiftA == 2) {
         return GL_FALSE;
      }

      *reg |= (TD0_alpha_add_disable |
               TD0_alpha_sel_add);

      if (args[0] == MGA_ARG2) {
         /* Swap arguments */
         *reg ^= (TD0_alpha_arg1_inv_enable |
                  TD0_alpha_arg2_inv_enable);
      }
      break;
   }

   return GL_TRUE;
}
   
   
