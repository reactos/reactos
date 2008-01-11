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
        

/* Code to layout images in a mipmap tree for i965.
 */

#include "intel_mipmap_tree.h"
#include "intel_tex_layout.h"
#include "macros.h"


GLboolean brw_miptree_layout( struct intel_mipmap_tree *mt )
{
   /* XXX: these vary depending on image format: 
    */
/*    GLint align_w = 4; */

   switch (mt->target) {
   case GL_TEXTURE_CUBE_MAP: 
   case GL_TEXTURE_3D: {
      GLuint width  = mt->width0;
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

      for ( level = mt->first_level ; level <= mt->last_level ; level++ ) {
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

	 width  = minify(width);
	 height = minify(height);
	 depth  = minify(depth);
      }
      break;
   }

   default:
      i945_miptree_layout_2d(mt);
      break;
   }
   DBG("%s: %dx%dx%d - sz 0x%x\n", __FUNCTION__, 
		mt->pitch, 
		mt->total_height,
		mt->cpp,
		mt->pitch * mt->total_height * mt->cpp );
		
   return GL_TRUE;
}

