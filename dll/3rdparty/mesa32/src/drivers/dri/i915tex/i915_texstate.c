/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "mtypes.h"
#include "enums.h"
#include "texformat.h"
#include "dri_bufmgr.h"

#include "intel_mipmap_tree.h"
#include "intel_tex.h"

#include "i915_context.h"
#include "i915_reg.h"


static GLuint
translate_texture_format(GLuint mesa_format)
{
   switch (mesa_format) {
   case MESA_FORMAT_L8:
      return MAPSURF_8BIT | MT_8BIT_L8;
   case MESA_FORMAT_I8:
      return MAPSURF_8BIT | MT_8BIT_I8;
   case MESA_FORMAT_A8:
      return MAPSURF_8BIT | MT_8BIT_A8;
   case MESA_FORMAT_AL88:
      return MAPSURF_16BIT | MT_16BIT_AY88;
   case MESA_FORMAT_RGB565:
      return MAPSURF_16BIT | MT_16BIT_RGB565;
   case MESA_FORMAT_ARGB1555:
      return MAPSURF_16BIT | MT_16BIT_ARGB1555;
   case MESA_FORMAT_ARGB4444:
      return MAPSURF_16BIT | MT_16BIT_ARGB4444;
   case MESA_FORMAT_ARGB8888:
      return MAPSURF_32BIT | MT_32BIT_ARGB8888;
   case MESA_FORMAT_YCBCR_REV:
      return (MAPSURF_422 | MT_422_YCRCB_NORMAL);
   case MESA_FORMAT_YCBCR:
      return (MAPSURF_422 | MT_422_YCRCB_SWAPY);
   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_FXT1);
   case MESA_FORMAT_Z16:
      return (MAPSURF_16BIT | MT_16BIT_L16);
   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_RGB_DXT1:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT1);
   case MESA_FORMAT_RGBA_DXT3:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT2_3);
   case MESA_FORMAT_RGBA_DXT5:
      return (MAPSURF_COMPRESSED | MT_COMPRESS_DXT4_5);
   case MESA_FORMAT_Z24_S8:
      return (MAPSURF_32BIT | MT_32BIT_xL824);
   default:
      fprintf(stderr, "%s: bad image format %x\n", __FUNCTION__, mesa_format);
      abort();
      return 0;
   }
}




/* The i915 (and related graphics cores) do not support GL_CLAMP.  The
 * Intel drivers for "other operating systems" implement GL_CLAMP as
 * GL_CLAMP_TO_EDGE, so the same is done here.
 */
static GLuint
translate_wrap_mode(GLenum wrap)
{
   switch (wrap) {
   case GL_REPEAT:
      return TEXCOORDMODE_WRAP;
   case GL_CLAMP:
      return TEXCOORDMODE_CLAMP_EDGE;   /* not quite correct */
   case GL_CLAMP_TO_EDGE:
      return TEXCOORDMODE_CLAMP_EDGE;
   case GL_CLAMP_TO_BORDER:
      return TEXCOORDMODE_CLAMP_BORDER;
   case GL_MIRRORED_REPEAT:
      return TEXCOORDMODE_MIRROR;
   default:
      return TEXCOORDMODE_WRAP;
   }
}



/* Recalculate all state from scratch.  Perhaps not the most
 * efficient, but this has gotten complex enough that we need
 * something which is understandable and reliable.
 */
static GLboolean
i915_update_tex_unit(struct intel_context *intel, GLuint unit, GLuint ss3)
{
   GLcontext *ctx = &intel->ctx;
   struct i915_context *i915 = i915_context(ctx);
   struct gl_texture_object *tObj = ctx->Texture.Unit[unit]._Current;
   struct intel_texture_object *intelObj = intel_texture_object(tObj);
   struct gl_texture_image *firstImage;
   GLuint *state = i915->state.Tex[unit], format, pitch;

   memset(state, 0, sizeof(state));

   /*We need to refcount these. */

   if (i915->state.tex_buffer[unit] != NULL) {
       driBOUnReference(i915->state.tex_buffer[unit]);
       i915->state.tex_buffer[unit] = NULL;
   }

   if (!intelObj->imageOverride && !intel_finalize_mipmap_tree(intel, unit))
      return GL_FALSE;

   /* Get first image here, since intelObj->firstLevel will get set in
    * the intel_finalize_mipmap_tree() call above.
    */
   firstImage = tObj->Image[0][intelObj->firstLevel];

   if (intelObj->imageOverride) {
      i915->state.tex_buffer[unit] = NULL;
      i915->state.tex_offset[unit] = intelObj->textureOffset;

      switch (intelObj->depthOverride) {
      case 32:
	 format = MAPSURF_32BIT | MT_32BIT_ARGB8888;
	 break;
      case 24:
      default:
	 format = MAPSURF_32BIT | MT_32BIT_XRGB8888;
	 break;
      case 16:
	 format = MAPSURF_16BIT | MT_16BIT_RGB565;
	 break;
      }

      pitch = intelObj->pitchOverride;
   } else {
      i915->state.tex_buffer[unit] = driBOReference(intelObj->mt->region->
						    buffer);
      i915->state.tex_offset[unit] =  intel_miptree_image_offset(intelObj->mt,
								 0, intelObj->
								 firstLevel);

      format = translate_texture_format(firstImage->TexFormat->MesaFormat);
      pitch = intelObj->mt->pitch * intelObj->mt->cpp;
   }

   state[I915_TEXREG_MS3] =
      (((firstImage->Height - 1) << MS3_HEIGHT_SHIFT) |
       ((firstImage->Width - 1) << MS3_WIDTH_SHIFT) | format |
       MS3_USE_FENCE_REGS);

   state[I915_TEXREG_MS4] =
     ((((pitch / 4) - 1) << MS4_PITCH_SHIFT) | MS4_CUBE_FACE_ENA_MASK |
       ((((intelObj->lastLevel - intelObj->firstLevel) * 4)) <<
	MS4_MAX_LOD_SHIFT) | ((firstImage->Depth - 1) <<
			      MS4_VOLUME_DEPTH_SHIFT));


   {
      GLuint minFilt, mipFilt, magFilt;

      switch (tObj->MinFilter) {
      case GL_NEAREST:
         minFilt = FILTER_NEAREST;
         mipFilt = MIPFILTER_NONE;
         break;
      case GL_LINEAR:
         minFilt = FILTER_LINEAR;
         mipFilt = MIPFILTER_NONE;
         break;
      case GL_NEAREST_MIPMAP_NEAREST:
         minFilt = FILTER_NEAREST;
         mipFilt = MIPFILTER_NEAREST;
         break;
      case GL_LINEAR_MIPMAP_NEAREST:
         minFilt = FILTER_LINEAR;
         mipFilt = MIPFILTER_NEAREST;
         break;
      case GL_NEAREST_MIPMAP_LINEAR:
         minFilt = FILTER_NEAREST;
         mipFilt = MIPFILTER_LINEAR;
         break;
      case GL_LINEAR_MIPMAP_LINEAR:
         minFilt = FILTER_LINEAR;
         mipFilt = MIPFILTER_LINEAR;
         break;
      default:
         return GL_FALSE;
      }

      if (tObj->MaxAnisotropy > 1.0) {
         minFilt = FILTER_ANISOTROPIC;
         magFilt = FILTER_ANISOTROPIC;
      }
      else {
         switch (tObj->MagFilter) {
         case GL_NEAREST:
            magFilt = FILTER_NEAREST;
            break;
         case GL_LINEAR:
            magFilt = FILTER_LINEAR;
            break;
         default:
            return GL_FALSE;
         }
      }

      state[I915_TEXREG_SS2] = i915->lodbias_ss2[unit];

      /* YUV conversion:
       */
      if (firstImage->TexFormat->MesaFormat == MESA_FORMAT_YCBCR ||
          firstImage->TexFormat->MesaFormat == MESA_FORMAT_YCBCR_REV)
         state[I915_TEXREG_SS2] |= SS2_COLORSPACE_CONVERSION;

      /* Shadow:
       */
      if (tObj->CompareMode == GL_COMPARE_R_TO_TEXTURE_ARB &&
          tObj->Target != GL_TEXTURE_3D) {

         state[I915_TEXREG_SS2] |=
            (SS2_SHADOW_ENABLE |
             intel_translate_compare_func(tObj->CompareFunc));

         minFilt = FILTER_4X4_FLAT;
         magFilt = FILTER_4X4_FLAT;
      }

      state[I915_TEXREG_SS2] |= ((minFilt << SS2_MIN_FILTER_SHIFT) |
                                 (mipFilt << SS2_MIP_FILTER_SHIFT) |
                                 (magFilt << SS2_MAG_FILTER_SHIFT));
   }

   {
      GLenum ws = tObj->WrapS;
      GLenum wt = tObj->WrapT;
      GLenum wr = tObj->WrapR;


      /* 3D textures don't seem to respect the border color.
       * Fallback if there's ever a danger that they might refer to
       * it.  
       * 
       * Effectively this means fallback on 3D clamp or
       * clamp_to_border.
       */
      if (tObj->Target == GL_TEXTURE_3D &&
          (tObj->MinFilter != GL_NEAREST ||
           tObj->MagFilter != GL_NEAREST) &&
          (ws == GL_CLAMP ||
           wt == GL_CLAMP ||
           wr == GL_CLAMP ||
           ws == GL_CLAMP_TO_BORDER ||
           wt == GL_CLAMP_TO_BORDER || wr == GL_CLAMP_TO_BORDER))
         return GL_FALSE;


      state[I915_TEXREG_SS3] = ss3;     /* SS3_NORMALIZED_COORDS */

      state[I915_TEXREG_SS3] |=
         ((translate_wrap_mode(ws) << SS3_TCX_ADDR_MODE_SHIFT) |
          (translate_wrap_mode(wt) << SS3_TCY_ADDR_MODE_SHIFT) |
          (translate_wrap_mode(wr) << SS3_TCZ_ADDR_MODE_SHIFT));

      state[I915_TEXREG_SS3] |= (unit << SS3_TEXTUREMAP_INDEX_SHIFT);
   }


   state[I915_TEXREG_SS4] = INTEL_PACKCOLOR8888(tObj->_BorderChan[0],
                                                tObj->_BorderChan[1],
                                                tObj->_BorderChan[2],
                                                tObj->_BorderChan[3]);


   I915_ACTIVESTATE(i915, I915_UPLOAD_TEX(unit), GL_TRUE);
   /* memcmp was already disabled, but definitely won't work as the
    * region might now change and that wouldn't be detected:
    */
   I915_STATECHANGE(i915, I915_UPLOAD_TEX(unit));


#if 0
   DBG(TEXTURE, "state[I915_TEXREG_SS2] = 0x%x\n", state[I915_TEXREG_SS2]);
   DBG(TEXTURE, "state[I915_TEXREG_SS3] = 0x%x\n", state[I915_TEXREG_SS3]);
   DBG(TEXTURE, "state[I915_TEXREG_SS4] = 0x%x\n", state[I915_TEXREG_SS4]);
   DBG(TEXTURE, "state[I915_TEXREG_MS2] = 0x%x\n", state[I915_TEXREG_MS2]);
   DBG(TEXTURE, "state[I915_TEXREG_MS3] = 0x%x\n", state[I915_TEXREG_MS3]);
   DBG(TEXTURE, "state[I915_TEXREG_MS4] = 0x%x\n", state[I915_TEXREG_MS4]);
#endif

   return GL_TRUE;
}




void
i915UpdateTextureState(struct intel_context *intel)
{
   GLboolean ok = GL_TRUE;
   GLuint i;

   for (i = 0; i < I915_TEX_UNITS && ok; i++) {
      switch (intel->ctx.Texture.Unit[i]._ReallyEnabled) {
      case TEXTURE_1D_BIT:
      case TEXTURE_2D_BIT:
      case TEXTURE_CUBE_BIT:
      case TEXTURE_3D_BIT:
         ok = i915_update_tex_unit(intel, i, SS3_NORMALIZED_COORDS);
         break;
      case TEXTURE_RECT_BIT:
         ok = i915_update_tex_unit(intel, i, 0);
         break;
      case 0:{
            struct i915_context *i915 = i915_context(&intel->ctx);
            if (i915->state.active & I915_UPLOAD_TEX(i))
               I915_ACTIVESTATE(i915, I915_UPLOAD_TEX(i), GL_FALSE);

	    if (i915->state.tex_buffer[i] != NULL) {
	       driBOUnReference(i915->state.tex_buffer[i]);
	       i915->state.tex_buffer[i] = NULL;
	    }

            break;
         }
      default:
         ok = GL_FALSE;
         break;
      }
   }

   FALLBACK(intel, I915_FALLBACK_TEXTURE, !ok);
}
