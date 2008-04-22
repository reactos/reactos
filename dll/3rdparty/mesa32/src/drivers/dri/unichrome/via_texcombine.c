/*
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file via_texcombine.c
 * Calculate texture combine hardware state.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <stdio.h>

#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "colormac.h"
#include "enums.h"

#include "via_context.h"
#include "via_state.h"
#include "via_tex.h"
#include "via_3d_reg.h"


#define VIA_USE_ALPHA (HC_XTC_Adif - HC_XTC_Dif)

#define INPUT_A_SHIFT     14
#define INPUT_B_SHIFT     7
#define INPUT_C_SHIFT     0
#define INPUT_CBias_SHIFT 14

#define CONST_ONE         (HC_XTC_0 | HC_XTC_InvTOPC)

static const unsigned color_operand_modifier[4] = {
   0,
   HC_XTC_InvTOPC,
   VIA_USE_ALPHA,
   VIA_USE_ALPHA | HC_XTC_InvTOPC,
};

static const unsigned alpha_operand_modifier[2] = {
   0, HC_XTA_InvTOPA
};

static const unsigned bias_alpha_operand_modifier[2] = {
   0, HC_HTXnTBLAbias_Inv
};


static const unsigned c_shift_table[3] = {
   HC_HTXnTBLCshift_No, HC_HTXnTBLCshift_1, HC_HTXnTBLCshift_2
};

static const unsigned  a_shift_table[3] = {
   HC_HTXnTBLAshift_No, HC_HTXnTBLAshift_1, HC_HTXnTBLAshift_2
};


/**
 * Calculate the hardware state for the specified texture combine mode
 *
 * \bug
 * All forms of DOT3 bumpmapping are completely untested, and are most
 * likely wrong.  KW: Looks like it will never be quite right as the
 * hardware seems to experience overflow in color calculation at the
 * 4x shift levels, which need to be programed for DOT3.  Maybe newer
 * hardware fixes these issues.
 *
 * \bug 
 * KW: needs attention to the case where texunit 1 is enabled but
 * texunit 0 is not.
 */
GLboolean
viaTexCombineState( struct via_context *vmesa,
		    const struct gl_tex_env_combine_state * combine,
		    unsigned unit )
{
   unsigned color_arg[3];
   unsigned alpha_arg[3];
   unsigned bias_alpha_arg[3];
   unsigned color = HC_HTXnTBLCsat_MASK;
   unsigned alpha = HC_HTXnTBLAsat_MASK;
   unsigned bias = 0;
   unsigned op = 0;
   unsigned a_shift = combine->ScaleShiftA;
   unsigned c_shift = combine->ScaleShiftRGB;
   unsigned i;
   unsigned constant_color[3];
   unsigned ordered_constant_color[4];
   unsigned constant_alpha[3];
   unsigned bias_alpha = 0;
   unsigned abc_alpha = 0;
   const struct gl_texture_unit * texUnit = 
      &vmesa->glCtx->Texture.Unit[unit];
   unsigned env_color[4];

   /* It seems that the color clamping can be overwhelmed at the 4x
    * scale settings, necessitating this fallback:
    */
   if (c_shift == 2 || a_shift == 2) {
      return GL_FALSE;
   }

   CLAMPED_FLOAT_TO_UBYTE(env_color[0], texUnit->EnvColor[0]);
   CLAMPED_FLOAT_TO_UBYTE(env_color[1], texUnit->EnvColor[1]);
   CLAMPED_FLOAT_TO_UBYTE(env_color[2], texUnit->EnvColor[2]);
   CLAMPED_FLOAT_TO_UBYTE(env_color[3], texUnit->EnvColor[3]);

   (void) memset( constant_color, 0, sizeof( constant_color ) );
   (void) memset( ordered_constant_color, 0, sizeof( ordered_constant_color ) );
   (void) memset( constant_alpha, 0, sizeof( constant_alpha ) );

   for ( i = 0 ; i < combine->_NumArgsRGB ; i++ ) {
      const GLint op = combine->OperandRGB[i] - GL_SRC_COLOR;

      switch ( combine->SourceRGB[i] ) {
      case GL_TEXTURE:
	 color_arg[i] = HC_XTC_Tex;
	 color_arg[i] += color_operand_modifier[op];
	 break;
      case GL_CONSTANT:
	 color_arg[i] = HC_XTC_HTXnTBLRC;

	 switch( op ) {
	 case 0:		/* GL_SRC_COLOR */
	    constant_color[i] = ((env_color[0] << 16) | 
				 (env_color[1] << 8) | 
				 env_color[2]);
	    break;
	 case 1:		/* GL_ONE_MINUS_SRC_COLOR */
	    constant_color[i] = ~((env_color[0] << 16) | 
				  (env_color[1] << 8) | 
				  env_color[2]) & 0x00ffffff;
	    break;
	 case 2:		/* GL_SRC_ALPHA */
	    constant_color[i] = ((env_color[3] << 16) | 
				 (env_color[3] << 8) | 
				 env_color[3]);
	    break;
	 case 3:		/* GL_ONE_MINUS_SRC_ALPHA */
	    constant_color[i] = ~((env_color[3] << 16) | 
				  (env_color[3] << 8) | 
				  env_color[3]) & 0x00ffffff;
	    break;
	 }
	 break;
      case GL_PRIMARY_COLOR:
	 color_arg[i] = HC_XTC_Dif;
	 color_arg[i] += color_operand_modifier[op];
	 break;
      case GL_PREVIOUS:
	 color_arg[i] = (unit == 0) ? HC_XTC_Dif : HC_XTC_Cur;
	 color_arg[i] += color_operand_modifier[op];
	 break;
      }
   }
	
   
   /* On the Unichrome, all combine operations take on some form of:
    *
    *     (xA * (xB op xC) + xBias) << xShift
    * 
    * 'op' can be selected as add, subtract, min, max, or mask.  The min, max
    * and mask modes are currently unused.  With the exception of DOT3, all
    * standard GL_COMBINE modes can be implemented simply by selecting the
    * correct inputs for A, B, C, and Bias and the correct operation for op.
    *
    * NOTE: xBias (when read from the constant registers) is signed,
    * and scaled to fit -255..255 in 8 bits, ie 0x1 == 2.
    */

   switch( combine->ModeRGB ) {
   /* Ca = 1.0, Cb = arg0, Cc = 0, Cbias = 0
    */
   case GL_REPLACE:
      color |= ((CONST_ONE << INPUT_A_SHIFT) |
		(color_arg[0] << INPUT_B_SHIFT));
		
      ordered_constant_color[1] = constant_color[0];
      break;
      
   /* Ca = arg[0], Cb = arg[1], Cc = 0, Cbias = 0
    */
   case GL_MODULATE:
      color |= ((color_arg[0] << INPUT_A_SHIFT) | 
		(color_arg[1] << INPUT_B_SHIFT));

      ordered_constant_color[0] = constant_color[0];
      ordered_constant_color[1] = constant_color[1];
      break;

   /* Ca = 1.0, Cb = arg[0], Cc = arg[1], Cbias = 0
    */
   case GL_ADD:
   case GL_SUBTRACT:
      if ( combine->ModeRGB == GL_SUBTRACT ) {
	 op |= HC_HTXnTBLCop_Sub;
      }

      color |= ((CONST_ONE << INPUT_A_SHIFT) |
		(color_arg[0] << INPUT_B_SHIFT) |
		(color_arg[1] << INPUT_C_SHIFT));

      ordered_constant_color[1] = constant_color[0];
      ordered_constant_color[2] = constant_color[1];
      break;

   /* Ca = 1.0, Cb = arg[0], Cc = arg[1], Cbias = -0.5
    */
   case GL_ADD_SIGNED:
      color |= ((CONST_ONE << INPUT_A_SHIFT) |
		(color_arg[0] << INPUT_B_SHIFT) | 
		(color_arg[1] << INPUT_C_SHIFT));

      bias |= HC_HTXnTBLCbias_HTXnTBLRC;

      ordered_constant_color[1] = constant_color[0];
      ordered_constant_color[2] = constant_color[1];
      ordered_constant_color[3] = 0x00bfbfbf; /* -.5 */
      break;

   /* Ca = arg[2], Cb = arg[0], Cc = arg[1], Cbias = arg[1]
    */
   case GL_INTERPOLATE:
      op |= HC_HTXnTBLCop_Sub;

      color |= ((color_arg[2] << INPUT_A_SHIFT) |
		(color_arg[0] << INPUT_B_SHIFT) |
		(color_arg[1] << INPUT_C_SHIFT));

      bias |= (color_arg[1] << INPUT_CBias_SHIFT);

      ordered_constant_color[0] = constant_color[2];
      ordered_constant_color[1] = constant_color[0];
      ordered_constant_color[2] = constant_color[1];
      ordered_constant_color[3] = (constant_color[1] >> 1) & 0x7f7f7f;
      break;

#if 0
   /* At this point this code is completely untested.  It appears that the
    * Unichrome has the same limitation as the Radeon R100.  The only
    * supported post-scale when doing DOT3 bumpmapping is 1x.
    */
   case GL_DOT3_RGB_EXT:
   case GL_DOT3_RGBA_EXT:
   case GL_DOT3_RGB:
   case GL_DOT3_RGBA:
      c_shift = 2;
      a_shift = 2;
      color |= ((color_arg[0] << INPUT_A_SHIFT) |
		(color_arg[1] << INPUT_B_SHIFT));
      op |= HC_HTXnTBLDOT4;
      break;
#endif

   default:
      assert(0);
      break;
   }




   /* The alpha blend stage has the annoying quirk of not having a
    * hard-wired 0 input, like the color stage.  As a result, we have
    * to program the constant register with 0 and use that as our
    * 0 input.
    *
    *     (xA * (xB op xC) + xBias) << xShift
    *
    */

   for ( i = 0 ; i < combine->_NumArgsA ; i++ ) {
      const GLint op = combine->OperandA[i] - GL_SRC_ALPHA;

      switch ( combine->SourceA[i] ) {
      case GL_TEXTURE:
	 alpha_arg[i] = HC_XTA_Atex;
	 alpha_arg[i] += alpha_operand_modifier[op];
	 bias_alpha_arg[i] = HC_HTXnTBLAbias_Atex;
	 bias_alpha_arg[i] += bias_alpha_operand_modifier[op];
	 break;
      case GL_CONSTANT:
	 alpha_arg[i] = HC_XTA_HTXnTBLRA;
	 bias_alpha_arg[i] = HC_HTXnTBLAbias_HTXnTBLRAbias;
	 constant_alpha[i] = (op == 0) ? env_color[3] : (~env_color[3] & 0xff);
	 break;
      case GL_PRIMARY_COLOR:
	 alpha_arg[i] = HC_XTA_Adif;
	 alpha_arg[i] += alpha_operand_modifier[op];
	 bias_alpha_arg[i] = HC_HTXnTBLAbias_Adif;
	 bias_alpha_arg[i] += bias_alpha_operand_modifier[op];
	 break;
      case GL_PREVIOUS:
	 alpha_arg[i] = (unit == 0) ? HC_XTA_Adif : HC_XTA_Acur;
	 alpha_arg[i] += alpha_operand_modifier[op];
	 bias_alpha_arg[i] = (unit == 0 ? 
			      HC_HTXnTBLAbias_Adif : 
			      HC_HTXnTBLAbias_Acur);
	 bias_alpha_arg[i] += bias_alpha_operand_modifier[op];
	 break;
      }
   }

   switch( combine->ModeA ) {
   /* Aa = 0, Ab = 0, Ac = 0, Abias = arg0
    */
   case GL_REPLACE:
      alpha |= ((HC_XTA_HTXnTBLRA << INPUT_A_SHIFT) |
		(HC_XTA_HTXnTBLRA << INPUT_B_SHIFT) |
		(HC_XTA_HTXnTBLRA << INPUT_C_SHIFT));
      abc_alpha = 0;

      bias |= bias_alpha_arg[0];
      bias_alpha = constant_alpha[0] >> 1;
      break;
      
   /* Aa = arg[0], Ab = arg[1], Ac = 0, Abias = 0
    */
   case GL_MODULATE:
      alpha |= ((alpha_arg[1] << INPUT_A_SHIFT) | 
		(alpha_arg[0] << INPUT_B_SHIFT) | 
		(HC_XTA_HTXnTBLRA << INPUT_C_SHIFT));

      abc_alpha = ((constant_alpha[1] << HC_HTXnTBLRAa_SHIFT) |
		   (constant_alpha[0] << HC_HTXnTBLRAb_SHIFT) |
		   (0 << HC_HTXnTBLRAc_SHIFT));

      bias |= HC_HTXnTBLAbias_HTXnTBLRAbias;
      bias_alpha = 0;
      break;

   /* Aa = 1.0, Ab = arg[0], Ac = arg[1], Abias = 0
    */
   case GL_ADD:
   case GL_SUBTRACT:
      if ( combine->ModeA == GL_SUBTRACT ) {
	 op |= HC_HTXnTBLAop_Sub;
      }

      alpha |= ((HC_XTA_HTXnTBLRA << INPUT_A_SHIFT) |
		(alpha_arg[0] << INPUT_B_SHIFT) |
		(alpha_arg[1] << INPUT_C_SHIFT));

      abc_alpha = ((0xff << HC_HTXnTBLRAa_SHIFT) |
		   (constant_alpha[0] << HC_HTXnTBLRAb_SHIFT) |
		   (constant_alpha[1] << HC_HTXnTBLRAc_SHIFT));

      bias |= HC_HTXnTBLAbias_HTXnTBLRAbias;
      bias_alpha = 0;
      break;

   /* Aa = 1.0, Ab = arg[0], Ac = arg[1], Abias = -0.5
    */
   case GL_ADD_SIGNED:
      alpha |= ((HC_XTA_HTXnTBLRA << INPUT_A_SHIFT) |
		(alpha_arg[0] << INPUT_B_SHIFT) | 
		(alpha_arg[1] << INPUT_C_SHIFT));
      abc_alpha = ((0xff << HC_HTXnTBLRAa_SHIFT) |
		   (constant_alpha[0] << HC_HTXnTBLRAb_SHIFT) |
		   (constant_alpha[1] << HC_HTXnTBLRAc_SHIFT));

      bias |= HC_HTXnTBLAbias_HTXnTBLRAbias;
      bias_alpha = 0xbf;
      break;

   /* Aa = arg[2], Ab = arg[0], Ac = arg[1], Abias = arg[1]
    */
   case GL_INTERPOLATE:
      op |= HC_HTXnTBLAop_Sub;

      alpha |= ((alpha_arg[2] << INPUT_A_SHIFT) |
		(alpha_arg[0] << INPUT_B_SHIFT) |
		(alpha_arg[1] << INPUT_C_SHIFT));
      abc_alpha = ((constant_alpha[2] << HC_HTXnTBLRAa_SHIFT) |
		   (constant_alpha[0] << HC_HTXnTBLRAb_SHIFT) |
		   (constant_alpha[1] << HC_HTXnTBLRAc_SHIFT));

      bias |= bias_alpha_arg[1];
      bias_alpha = constant_alpha[1] >> 1;
      break;
   }
   

   op |= c_shift_table[ c_shift ] | a_shift_table[ a_shift ];


   vmesa->regHTXnTBLMPfog[unit] = HC_HTXnTBLMPfog_Fog;

   vmesa->regHTXnTBLCsat[unit] = color;
   vmesa->regHTXnTBLAsat[unit] = alpha;
   vmesa->regHTXnTBLCop[unit] = op | bias;
   vmesa->regHTXnTBLRAa[unit] = abc_alpha;
   vmesa->regHTXnTBLRFog[unit] = bias_alpha;

   vmesa->regHTXnTBLRCa[unit] = ordered_constant_color[0];
   vmesa->regHTXnTBLRCb[unit] = ordered_constant_color[1];
   vmesa->regHTXnTBLRCc[unit] = ordered_constant_color[2];
   vmesa->regHTXnTBLRCbias[unit] = ordered_constant_color[3];

   return GL_TRUE;
}

