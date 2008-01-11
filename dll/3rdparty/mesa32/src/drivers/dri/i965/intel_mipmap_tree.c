/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "bufmgr.h"
#include "enums.h"
#include "imports.h"

static GLenum target_to_target( GLenum target )
{
   switch (target) {
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
      return GL_TEXTURE_CUBE_MAP_ARB;
   default:
      return target;
   }
}

struct intel_mipmap_tree *intel_miptree_create( struct intel_context *intel,
						GLenum target,
						GLenum internal_format,
						GLuint first_level,
						GLuint last_level,
						GLuint width0,
						GLuint height0,
						GLuint depth0,
						GLuint cpp,
						GLboolean compressed)
{
   GLboolean ok;
   struct intel_mipmap_tree *mt = calloc(sizeof(*mt), 1);

   if (INTEL_DEBUG & DEBUG_TEXTURE)
      _mesa_printf("%s target %s format %s level %d..%d\n", __FUNCTION__,
		   _mesa_lookup_enum_by_nr(target),
		   _mesa_lookup_enum_by_nr(internal_format),
		   first_level,
		   last_level);

   mt->target = target_to_target(target);
   mt->internal_format = internal_format;
   mt->first_level = first_level;
   mt->last_level = last_level;
   mt->width0 = width0;
   mt->height0 = height0;
   mt->depth0 = depth0;
   mt->cpp = compressed ? 2 : cpp;
   mt->compressed = compressed;

   switch (intel->intelScreen->deviceID) {
#if 0
   case PCI_CHIP_I945_G:
      ok = i945_miptree_layout( mt );
      break;
   case PCI_CHIP_I915_G:
   case PCI_CHIP_I915_GM:
      ok = i915_miptree_layout( mt );
      break;
#endif
   default:
      if (INTEL_DEBUG & DEBUG_TEXTURE)
	 _mesa_printf("assuming BRW texture layouts\n");
      ok = brw_miptree_layout( mt );
      break;
   }

   if (ok)
      mt->region = intel_region_alloc( intel, 
				       mt->cpp,
				       mt->pitch, 
				       mt->total_height );

   if (!mt->region) {
      free(mt);
      return NULL;
   }

   return mt;
}



void intel_miptree_destroy( struct intel_context *intel,
			    struct intel_mipmap_tree *mt )
{
   if (mt) {
      GLuint i;

      intel_region_release(intel, &(mt->region));

      for (i = 0; i < MAX_TEXTURE_LEVELS; i++)
	 if (mt->level[i].image_offset)
	    free(mt->level[i].image_offset);

      free(mt);
   }
}




void intel_miptree_set_level_info(struct intel_mipmap_tree *mt,
				  GLuint level,
				  GLuint nr_images,
				  GLuint x, GLuint y,
				  GLuint w, GLuint h, GLuint d)
{
   mt->level[level].width = w;
   mt->level[level].height = h;
   mt->level[level].depth = d;
   mt->level[level].level_offset = (x + y * mt->pitch) * mt->cpp;
   mt->level[level].nr_images = nr_images;

   if (INTEL_DEBUG & DEBUG_TEXTURE)
      _mesa_printf("%s level %d img size: %d,%d level_offset 0x%x\n", __FUNCTION__, level, w, h, 
		   mt->level[level].level_offset);

   /* Not sure when this would happen, but anyway: 
    */
   if (mt->level[level].image_offset) {
      free(mt->level[level].image_offset);
      mt->level[level].image_offset = NULL;
   }

   if (nr_images > 1) {
      mt->level[level].image_offset = malloc(nr_images * sizeof(GLuint));
      mt->level[level].image_offset[0] = 0;
   }
}



void intel_miptree_set_image_offset(struct intel_mipmap_tree *mt,
				    GLuint level,
				    GLuint img,
				    GLuint x, GLuint y)
{
   if (INTEL_DEBUG & DEBUG_TEXTURE)
      _mesa_printf("%s level %d img %d pos %d,%d\n", __FUNCTION__, level, img, x, y);

   if (img == 0)
      assert(x == 0 && y == 0);

   if (img > 0)
      mt->level[level].image_offset[img] = (x + y * mt->pitch) * mt->cpp;
}


/* Although we use the image_offset[] array to store relative offsets
 * to cube faces, Mesa doesn't know anything about this and expects
 * each cube face to be treated as a separate image.
 *
 * These functions present that view to mesa:
 */
const GLuint *intel_miptree_depth_offsets(struct intel_mipmap_tree *mt,
					  GLuint level)
{
   static const GLuint zero = 0;

   if (mt->target != GL_TEXTURE_3D ||
       mt->level[level].nr_images == 1)
      return &zero;
   else
      return mt->level[level].image_offset;
}


GLuint intel_miptree_image_offset(struct intel_mipmap_tree *mt,
				  GLuint face,
				  GLuint level)
{
   if (mt->target == GL_TEXTURE_CUBE_MAP_ARB)
      return (mt->level[level].level_offset +
	      mt->level[level].image_offset[face]);
   else
      return mt->level[level].level_offset;
}






/* Upload data for a particular image.
 */
GLboolean intel_miptree_image_data(struct intel_context *intel, 
				   struct intel_mipmap_tree *dst,
				   GLuint face,
				   GLuint level,
				   const void *src, 
				   GLuint src_row_pitch,
				   GLuint src_image_pitch)
{
   GLuint depth = dst->level[level].depth;
   GLuint dst_offset = intel_miptree_image_offset(dst, face, level);
   const GLuint *dst_depth_offset = intel_miptree_depth_offsets(dst, level);
   GLuint i;

   DBG("%s\n", __FUNCTION__);
   for (i = 0; i < depth; i++) {
      if (!intel_region_data(intel,
			     dst->region, 
			     dst_offset + dst_depth_offset[i],
			     0,
			     0,
			     src,
			     src_row_pitch,
			     0, 0,	/* source x,y */
			     dst->level[level].width,
			     dst->level[level].height))
	 return GL_FALSE;
      src += src_image_pitch;
   }
   return GL_TRUE;
}

