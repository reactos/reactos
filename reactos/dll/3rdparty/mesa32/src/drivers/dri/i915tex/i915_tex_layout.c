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

/* Code to layout images in a mipmap tree for i915 and i945
 * respectively.
 */

#include "intel_mipmap_tree.h"
#include "intel_tex_layout.h"
#include "macros.h"
#include "intel_context.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

static GLint initial_offsets[6][2] = { {0, 0},
{0, 2},
{1, 0},
{1, 2},
{1, 1},
{1, 3}
};


static GLint step_offsets[6][2] = { {0, 2},
{0, 2},
{-1, 2},
{-1, 2},
{-1, 1},
{-1, 1}
};

GLboolean
i915_miptree_layout(struct intel_mipmap_tree * mt)
{
   GLint level;

   switch (mt->target) {
   case GL_TEXTURE_CUBE_MAP:{
         const GLuint dim = mt->width0;
         GLuint face;
         GLuint lvlWidth = mt->width0, lvlHeight = mt->height0;

         assert(lvlWidth == lvlHeight); /* cubemap images are square */

         /* double pitch for cube layouts */
         mt->pitch = ((dim * mt->cpp * 2 + 3) & ~3) / mt->cpp;
         mt->total_height = dim * 4;

         for (level = mt->first_level; level <= mt->last_level; level++) {
            intel_miptree_set_level_info(mt, level, 6,
                                         0, 0,
                                         /*OLD: mt->pitch, mt->total_height,*/
                                         lvlWidth, lvlHeight,
                                         1);
            lvlWidth /= 2;
            lvlHeight /= 2;
         }

         for (face = 0; face < 6; face++) {
            GLuint x = initial_offsets[face][0] * dim;
            GLuint y = initial_offsets[face][1] * dim;
            GLuint d = dim;

            for (level = mt->first_level; level <= mt->last_level; level++) {
               intel_miptree_set_image_offset(mt, level, face, x, y);

               if (d == 0)
                  _mesa_printf("cube mipmap %d/%d (%d..%d) is 0x0\n",
                               face, level, mt->first_level, mt->last_level);

               d >>= 1;
               x += step_offsets[face][0] * d;
               y += step_offsets[face][1] * d;
            }
         }
         break;
      }
   case GL_TEXTURE_3D:{
         GLuint width = mt->width0;
         GLuint height = mt->height0;
         GLuint depth = mt->depth0;
         GLuint stack_height = 0;

         /* Calculate the size of a single slice. 
          */
         mt->pitch = ((mt->width0 * mt->cpp + 3) & ~3) / mt->cpp;

         /* XXX: hardware expects/requires 9 levels at minimum.
          */
         for (level = mt->first_level; level <= MAX2(8, mt->last_level);
              level++) {
            intel_miptree_set_level_info(mt, level, 1, 0, mt->total_height,
                                         width, height, depth);


            stack_height += MAX2(2, height);

            width = minify(width);
            height = minify(height);
            depth = minify(depth);
         }

         /* Fixup depth image_offsets: 
          */
         depth = mt->depth0;
         for (level = mt->first_level; level <= mt->last_level; level++) {
            GLuint i;
            for (i = 0; i < depth; i++) 
               intel_miptree_set_image_offset(mt, level, i,
                                              0, i * stack_height);

            depth = minify(depth);
         }


         /* Multiply slice size by texture depth for total size.  It's
          * remarkable how wasteful of memory the i915 texture layouts
          * are.  They are largely fixed in the i945.
          */
         mt->total_height = stack_height * mt->depth0;
         break;
      }

   default:{
         GLuint width = mt->width0;
         GLuint height = mt->height0;
	 GLuint img_height;

         mt->pitch = ((mt->width0 * mt->cpp + 3) & ~3) / mt->cpp;
         mt->total_height = 0;

         for (level = mt->first_level; level <= mt->last_level; level++) {
            intel_miptree_set_level_info(mt, level, 1,
                                         0, mt->total_height,
                                         width, height, 1);

            if (mt->compressed)
               img_height = MAX2(1, height / 4);
            else
               img_height = (MAX2(2, height) + 1) & ~1;

	    mt->total_height += img_height;

            width = minify(width);
            height = minify(height);
         }
         break;
      }
   }
   DBG("%s: %dx%dx%d - sz 0x%x\n", __FUNCTION__,
       mt->pitch,
       mt->total_height, mt->cpp, mt->pitch * mt->total_height * mt->cpp);

   return GL_TRUE;
}


GLboolean
i945_miptree_layout(struct intel_mipmap_tree * mt)
{
   GLint level;

   switch (mt->target) {
   case GL_TEXTURE_CUBE_MAP:{
         const GLuint dim = mt->width0;
         GLuint face;
         GLuint lvlWidth = mt->width0, lvlHeight = mt->height0;

         assert(lvlWidth == lvlHeight); /* cubemap images are square */

         /* Depending on the size of the largest images, pitch can be
          * determined either by the old-style packing of cubemap faces,
          * or the final row of 4x4, 2x2 and 1x1 faces below this. 
          */
         if (dim > 32)
            mt->pitch = ((dim * mt->cpp * 2 + 3) & ~3) / mt->cpp;
         else
            mt->pitch = 14 * 8;

         mt->total_height = dim * 4 + 4;

         /* Set all the levels to effectively occupy the whole rectangular region. 
          */
         for (level = mt->first_level; level <= mt->last_level; level++) {
            intel_miptree_set_level_info(mt, level, 6,
                                         0, 0,
                                         lvlWidth, lvlHeight, 1);
	    lvlWidth /= 2;
	    lvlHeight /= 2;
	 }


         for (face = 0; face < 6; face++) {
            GLuint x = initial_offsets[face][0] * dim;
            GLuint y = initial_offsets[face][1] * dim;
            GLuint d = dim;

            if (dim == 4 && face >= 4) {
               y = mt->total_height - 4;
               x = (face - 4) * 8;
            }
            else if (dim < 4 && (face > 0 || mt->first_level > 0)) {
               y = mt->total_height - 4;
               x = face * 8;
            }

            for (level = mt->first_level; level <= mt->last_level; level++) {
               intel_miptree_set_image_offset(mt, level, face, x, y);

               d >>= 1;

               switch (d) {
               case 4:
                  switch (face) {
                  case FACE_POS_X:
                  case FACE_NEG_X:
                     x += step_offsets[face][0] * d;
                     y += step_offsets[face][1] * d;
                     break;
                  case FACE_POS_Y:
                  case FACE_NEG_Y:
                     y += 12;
                     x -= 8;
                     break;
                  case FACE_POS_Z:
                  case FACE_NEG_Z:
                     y = mt->total_height - 4;
                     x = (face - 4) * 8;
                     break;
                  }

               case 2:
                  y = mt->total_height - 4;
                  x = 16 + face * 8;
                  break;

               case 1:
                  x += 48;
                  break;

               default:
                  x += step_offsets[face][0] * d;
                  y += step_offsets[face][1] * d;
                  break;
               }
            }
         }
         break;
      }
   case GL_TEXTURE_3D:{
         GLuint width = mt->width0;
         GLuint height = mt->height0;
         GLuint depth = mt->depth0;
         GLuint pack_x_pitch, pack_x_nr;
         GLuint pack_y_pitch;
         GLuint level;

         mt->pitch = ((mt->width0 * mt->cpp + 3) & ~3) / mt->cpp;
         mt->total_height = 0;

         pack_y_pitch = MAX2(mt->height0, 2);
         pack_x_pitch = mt->pitch;
         pack_x_nr = 1;

         for (level = mt->first_level; level <= mt->last_level; level++) {
            GLuint nr_images = mt->target == GL_TEXTURE_3D ? depth : 6;
            GLint x = 0;
            GLint y = 0;
            GLint q, j;

            intel_miptree_set_level_info(mt, level, nr_images,
                                         0, mt->total_height,
                                         width, height, depth);

            for (q = 0; q < nr_images;) {
               for (j = 0; j < pack_x_nr && q < nr_images; j++, q++) {
                  intel_miptree_set_image_offset(mt, level, q, x, y);
                  x += pack_x_pitch;
               }

               x = 0;
               y += pack_y_pitch;
            }


            mt->total_height += y;

            if (pack_x_pitch > 4) {
               pack_x_pitch >>= 1;
               pack_x_nr <<= 1;
               assert(pack_x_pitch * pack_x_nr <= mt->pitch);
            }

            if (pack_y_pitch > 2) {
               pack_y_pitch >>= 1;
            }

            width = minify(width);
            height = minify(height);
            depth = minify(depth);
         }
         break;
      }

   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_RECTANGLE_ARB:
         i945_miptree_layout_2d(mt);
         break;
   default:
      _mesa_problem(NULL, "Unexpected tex target in i945_miptree_layout()");
   }

   DBG("%s: %dx%dx%d - sz 0x%x\n", __FUNCTION__,
       mt->pitch,
       mt->total_height, mt->cpp, mt->pitch * mt->total_height * mt->cpp);

   return GL_TRUE;
}
