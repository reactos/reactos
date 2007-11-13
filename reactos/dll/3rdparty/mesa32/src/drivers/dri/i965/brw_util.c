/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
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
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
         

#include "mtypes.h"
#include "shader/prog_parameter.h"
#include "brw_util.h"
#include "brw_defines.h"

GLuint brw_count_bits( GLuint val )
{
   GLuint i;
   for (i = 0; val ; val >>= 1)
      if (val & 1)
	 i++;
   return i;
}


static GLuint brw_parameter_state_flags(const gl_state_index state[])
{
   switch (state[0]) {
   case STATE_MATERIAL:
   case STATE_LIGHT:
   case STATE_LIGHTMODEL_AMBIENT:
   case STATE_LIGHTMODEL_SCENECOLOR:
   case STATE_LIGHTPROD:
      return _NEW_LIGHT;

   case STATE_TEXGEN:
   case STATE_TEXENV_COLOR:
      return _NEW_TEXTURE;

   case STATE_FOG_COLOR:
   case STATE_FOG_PARAMS:
      return _NEW_FOG;

   case STATE_CLIPPLANE:
      return _NEW_TRANSFORM;

   case STATE_POINT_SIZE:
   case STATE_POINT_ATTENUATION:
      return _NEW_POINT;

   case STATE_MODELVIEW_MATRIX:
      return _NEW_MODELVIEW;
   case STATE_PROJECTION_MATRIX:
      return _NEW_PROJECTION;
   case STATE_MVP_MATRIX:
      return _NEW_MODELVIEW | _NEW_PROJECTION;
   case STATE_TEXTURE_MATRIX:
      return _NEW_TEXTURE_MATRIX;
   case STATE_PROGRAM_MATRIX:
      return _NEW_TRACK_MATRIX;

   case STATE_DEPTH_RANGE:
      return _NEW_VIEWPORT;

   case STATE_FRAGMENT_PROGRAM:
   case STATE_VERTEX_PROGRAM:
      return _NEW_PROGRAM;

   case STATE_INTERNAL:
      switch (state[1]) {
      case STATE_NORMAL_SCALE:
	 return _NEW_MODELVIEW;
      case STATE_TEXRECT_SCALE:
	 return _NEW_TEXTURE;
      default:
	 assert(0);
	 return 0;
      }

   default:
      assert(0);
      return 0;
   }
}


GLuint
brw_parameter_list_state_flags(struct gl_program_parameter_list *paramList)
{
   GLuint i;
   GLuint result = 0;

   if (!paramList)
      return 0;

   for (i = 0; i < paramList->NumParameters; i++) {
      if (paramList->Parameters[i].Type == PROGRAM_STATE_VAR) {
         result |= brw_parameter_state_flags(paramList->Parameters[i].StateIndexes);
      }
   }

   return result;
}


GLuint brw_translate_blend_equation( GLenum mode )
{
   switch (mode) {
   case GL_FUNC_ADD: 
      return BRW_BLENDFUNCTION_ADD; 
   case GL_MIN: 
      return BRW_BLENDFUNCTION_MIN; 
   case GL_MAX: 
      return BRW_BLENDFUNCTION_MAX; 
   case GL_FUNC_SUBTRACT: 
      return BRW_BLENDFUNCTION_SUBTRACT; 
   case GL_FUNC_REVERSE_SUBTRACT: 
      return BRW_BLENDFUNCTION_REVERSE_SUBTRACT; 
   default: 
      assert(0);
      return BRW_BLENDFUNCTION_ADD;
   }
}

GLuint brw_translate_blend_factor( GLenum factor )
{
   switch(factor) {
   case GL_ZERO: 
      return BRW_BLENDFACTOR_ZERO; 
   case GL_SRC_ALPHA: 
      return BRW_BLENDFACTOR_SRC_ALPHA; 
   case GL_ONE: 
      return BRW_BLENDFACTOR_ONE; 
   case GL_SRC_COLOR: 
      return BRW_BLENDFACTOR_SRC_COLOR; 
   case GL_ONE_MINUS_SRC_COLOR: 
      return BRW_BLENDFACTOR_INV_SRC_COLOR; 
   case GL_DST_COLOR: 
      return BRW_BLENDFACTOR_DST_COLOR; 
   case GL_ONE_MINUS_DST_COLOR: 
      return BRW_BLENDFACTOR_INV_DST_COLOR; 
   case GL_ONE_MINUS_SRC_ALPHA:
      return BRW_BLENDFACTOR_INV_SRC_ALPHA; 
   case GL_DST_ALPHA: 
      return BRW_BLENDFACTOR_DST_ALPHA; 
   case GL_ONE_MINUS_DST_ALPHA:
      return BRW_BLENDFACTOR_INV_DST_ALPHA; 
   case GL_SRC_ALPHA_SATURATE: 
      return BRW_BLENDFACTOR_SRC_ALPHA_SATURATE;
   case GL_CONSTANT_COLOR:
      return BRW_BLENDFACTOR_CONST_COLOR; 
   case GL_ONE_MINUS_CONSTANT_COLOR:
      return BRW_BLENDFACTOR_INV_CONST_COLOR;
   case GL_CONSTANT_ALPHA:
      return BRW_BLENDFACTOR_CONST_ALPHA; 
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      return BRW_BLENDFACTOR_INV_CONST_ALPHA;
   default:
      assert(0);
      return BRW_BLENDFACTOR_ZERO;
   }   
}
