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
                   

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

#include "macros.h"



/* Samplers aren't strictly wm state from the hardware's perspective,
 * but that is the only situation in which we use them in this driver.
 */



/* The brw (and related graphics cores) do not support GL_CLAMP.  The
 * Intel drivers for "other operating systems" implement GL_CLAMP as
 * GL_CLAMP_TO_EDGE, so the same is done here.
 */
static GLuint translate_wrap_mode( GLenum wrap )
{
   switch( wrap ) {
   case GL_REPEAT: 
      return BRW_TEXCOORDMODE_WRAP;
   case GL_CLAMP:  
      return BRW_TEXCOORDMODE_CLAMP_BORDER; /* conform likes it this way */
   case GL_CLAMP_TO_EDGE: 
      return BRW_TEXCOORDMODE_CLAMP; /* conform likes it this way */
   case GL_CLAMP_TO_BORDER: 
      return BRW_TEXCOORDMODE_CLAMP_BORDER;
   case GL_MIRRORED_REPEAT: 
      return BRW_TEXCOORDMODE_MIRROR;
   default: 
      return BRW_TEXCOORDMODE_WRAP;
   }
}


static GLuint U_FIXED(GLfloat value, GLuint frac_bits)
{
   value *= (1<<frac_bits);
   return value < 0 ? 0 : value;
}

static GLint S_FIXED(GLfloat value, GLuint frac_bits)
{
   return value * (1<<frac_bits);
}


static GLuint upload_default_color( struct brw_context *brw,
				    const GLfloat *color )
{
   struct brw_sampler_default_color sdc;

   COPY_4V(sdc.color, color); 
   
   return brw_cache_data( &brw->cache[BRW_SAMPLER_DEFAULT_COLOR], &sdc );
}


/*
 */
static void brw_update_sampler_state( struct gl_texture_unit *texUnit,
				      struct gl_texture_object *texObj,
				      GLuint sdc_gs_offset,
				      struct brw_sampler_state *sampler)
{   
   _mesa_memset(sampler, 0, sizeof(*sampler));

   switch (texObj->MinFilter) {
   case GL_NEAREST:
      sampler->ss0.min_filter = BRW_MAPFILTER_NEAREST;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NONE;
      break;
   case GL_LINEAR:
      sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NONE;
      break;
   case GL_NEAREST_MIPMAP_NEAREST:
      sampler->ss0.min_filter = BRW_MAPFILTER_NEAREST;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NEAREST;
      break;
   case GL_LINEAR_MIPMAP_NEAREST:
      sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NEAREST;
      break;
   case GL_NEAREST_MIPMAP_LINEAR:
      sampler->ss0.min_filter = BRW_MAPFILTER_NEAREST;
      sampler->ss0.mip_filter = BRW_MIPFILTER_LINEAR;
      break;
   case GL_LINEAR_MIPMAP_LINEAR:
      sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
      sampler->ss0.mip_filter = BRW_MIPFILTER_LINEAR;
      break;
   default:
      break;
   }

   /* Set Anisotropy: 
    */
   if ( texObj->MaxAnisotropy > 1.0 ) {
      sampler->ss0.min_filter = BRW_MAPFILTER_ANISOTROPIC; 
      sampler->ss0.mag_filter = BRW_MAPFILTER_ANISOTROPIC;

      if (texObj->MaxAnisotropy > 2.0) {
	 sampler->ss3.max_aniso = MAX2((texObj->MaxAnisotropy - 2) / 2,
				       BRW_ANISORATIO_16);
      }
   }
   else {
      switch (texObj->MagFilter) {
      case GL_NEAREST:
	 sampler->ss0.mag_filter = BRW_MAPFILTER_NEAREST;
	 break;
      case GL_LINEAR:
	 sampler->ss0.mag_filter = BRW_MAPFILTER_LINEAR;
	 break;
      default:
	 break;
      }  
   }

   sampler->ss1.r_wrap_mode = translate_wrap_mode(texObj->WrapR);
   sampler->ss1.s_wrap_mode = translate_wrap_mode(texObj->WrapS);
   sampler->ss1.t_wrap_mode = translate_wrap_mode(texObj->WrapT);

   /* Fulsim complains if I don't do this.  Hardware doesn't mind:
    */
#if 0
   if (texObj->Target == GL_TEXTURE_CUBE_MAP_ARB) {
      sampler->ss1.r_wrap_mode = BRW_TEXCOORDMODE_CUBE;
      sampler->ss1.s_wrap_mode = BRW_TEXCOORDMODE_CUBE;
      sampler->ss1.t_wrap_mode = BRW_TEXCOORDMODE_CUBE;
   }
#endif

   /* Set shadow function: 
    */
   if (texObj->CompareMode == GL_COMPARE_R_TO_TEXTURE_ARB) {
      /* Shadowing is "enabled" by emitting a particular sampler
       * message (sample_c).  So need to recompile WM program when
       * shadow comparison is enabled on each/any texture unit.
       */
      sampler->ss0.shadow_function = intel_translate_shadow_compare_func(texObj->CompareFunc);
   }

   /* Set LOD bias: 
    */
   sampler->ss0.lod_bias = S_FIXED(CLAMP(texUnit->LodBias + texObj->LodBias, -16, 15), 6);

   sampler->ss0.lod_preclamp = 1; /* OpenGL mode */
   sampler->ss0.default_color_mode = 0; /* OpenGL/DX10 mode */

   /* Set BaseMipLevel, MaxLOD, MinLOD: 
    *
    * XXX: I don't think that using firstLevel, lastLevel works,
    * because we always setup the surface state as if firstLevel ==
    * level zero.  Probably have to subtract firstLevel from each of
    * these:
    */
   sampler->ss0.base_level = U_FIXED(0, 1);

   sampler->ss1.max_lod = U_FIXED(MIN2(MAX2(texObj->MaxLod, 0), 13), 6);
   sampler->ss1.min_lod = U_FIXED(MIN2(MAX2(texObj->MinLod, 0), 13), 6);
   
   sampler->ss2.default_color_pointer = sdc_gs_offset >> 5;
}



/* All samplers must be uploaded in a single contiguous array, which
 * complicates various things.  However, this is still too confusing -
 * FIXME: simplify all the different new texture state flags.
 */
static void upload_wm_samplers( struct brw_context *brw )
{
   GLuint unit;
   GLuint sampler_count = 0;

   /* _NEW_TEXTURE */
   for (unit = 0; unit < BRW_MAX_TEX_UNIT; unit++) {
      if (brw->attribs.Texture->Unit[unit]._ReallyEnabled) {	 
	 struct gl_texture_unit *texUnit = &brw->attribs.Texture->Unit[unit];
	 struct gl_texture_object *texObj = texUnit->_Current;

	 GLuint sdc_gs_offset = upload_default_color(brw, texObj->BorderColor);

	 brw_update_sampler_state(texUnit,
				  texObj, 
				  sdc_gs_offset,
				  &brw->wm.sampler[unit]);

	 sampler_count = unit + 1;
      }
   }
   
   if (brw->wm.sampler_count != sampler_count) {
      brw->wm.sampler_count = sampler_count;
      brw->state.dirty.cache |= CACHE_NEW_SAMPLER;
   }

   brw->wm.sampler_gs_offset = 0;

   if (brw->wm.sampler_count) 
      brw->wm.sampler_gs_offset = 
	 brw_cache_data_sz(&brw->cache[BRW_SAMPLER],
			   brw->wm.sampler,
			   sizeof(struct brw_sampler_state) * brw->wm.sampler_count);
}


const struct brw_tracked_state brw_wm_samplers = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = 0,
      .cache = 0
   },
   .update = upload_wm_samplers
};


