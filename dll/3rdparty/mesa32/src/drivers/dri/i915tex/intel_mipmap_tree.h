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

#ifndef INTEL_MIPMAP_TREE_H
#define INTEL_MIPMAP_TREE_H

#include "intel_regions.h"

/* A layer on top of the intel_regions code which adds:
 *
 * - Code to size and layout a region to hold a set of mipmaps.
 * - Query to determine if a new image fits in an existing tree.
 * - More refcounting 
 *     - maybe able to remove refcounting from intel_region?
 * - ?
 *
 * The fixed mipmap layout of intel hardware where one offset
 * specifies the position of all images in a mipmap hierachy
 * complicates the implementation of GL texture image commands,
 * compared to hardware where each image is specified with an
 * independent offset.
 *
 * In an ideal world, each texture object would be associated with a
 * single bufmgr buffer or 2d intel_region, and all the images within
 * the texture object would slot into the tree as they arrive.  The
 * reality can be a little messier, as images can arrive from the user
 * with sizes that don't fit in the existing tree, or in an order
 * where the tree layout cannot be guessed immediately.  
 * 
 * This structure encodes an idealized mipmap tree.  The GL image
 * commands build these where possible, otherwise store the images in
 * temporary system buffers.
 */


/**
 * Describes the location of each texture image within a texture region.
 */
struct intel_mipmap_level
{
   GLuint level_offset;
   GLuint width;
   GLuint height;
   GLuint depth;
   GLuint nr_images;

   /* Explicitly store the offset of each image for each cube face or
    * depth value.  Pretty much have to accept that hardware formats
    * are going to be so diverse that there is no unified way to
    * compute the offsets of depth/cube images within a mipmap level,
    * so have to store them as a lookup table:
    */
   GLuint *image_offset;
};

struct intel_mipmap_tree
{
   /* Effectively the key:
    */
   GLenum target;
   GLenum internal_format;

   GLuint first_level;
   GLuint last_level;

   GLuint width0, height0, depth0; /**< Level zero image dimensions */
   GLuint cpp;
   GLboolean compressed;

   /* Derived from the above:
    */
   GLuint pitch;
   GLuint depth_pitch;          /* per-image on i945? */
   GLuint total_height;

   /* Includes image offset tables:
    */
   struct intel_mipmap_level level[MAX_TEXTURE_LEVELS];

   /* The data is held here:
    */
   struct intel_region *region;

   /* These are also refcounted:
    */
   GLuint refcount;
};



struct intel_mipmap_tree *intel_miptree_create(struct intel_context *intel,
                                               GLenum target,
                                               GLenum internal_format,
                                               GLuint first_level,
                                               GLuint last_level,
                                               GLuint width0,
                                               GLuint height0,
                                               GLuint depth0,
                                               GLuint cpp,
                                               GLuint compress_byte);

void intel_miptree_reference(struct intel_mipmap_tree **dst,
                             struct intel_mipmap_tree *src);

void intel_miptree_release(struct intel_context *intel,
                           struct intel_mipmap_tree **mt);

/* Check if an image fits an existing mipmap tree layout
 */
GLboolean intel_miptree_match_image(struct intel_mipmap_tree *mt,
                                    struct gl_texture_image *image,
                                    GLuint face, GLuint level);

/* Return a pointer to an image within a tree.  Return image stride as
 * well.
 */
GLubyte *intel_miptree_image_map(struct intel_context *intel,
                                 struct intel_mipmap_tree *mt,
                                 GLuint face,
                                 GLuint level,
                                 GLuint * row_stride, GLuint * image_stride);

void intel_miptree_image_unmap(struct intel_context *intel,
                               struct intel_mipmap_tree *mt);


/* Return the linear offset of an image relative to the start of the
 * tree:
 */
GLuint intel_miptree_image_offset(struct intel_mipmap_tree *mt,
                                  GLuint face, GLuint level);

/* Return pointers to each 2d slice within an image.  Indexed by depth
 * value.
 */
const GLuint *intel_miptree_depth_offsets(struct intel_mipmap_tree *mt,
                                          GLuint level);


void intel_miptree_set_level_info(struct intel_mipmap_tree *mt,
                                  GLuint level,
                                  GLuint nr_images,
                                  GLuint x, GLuint y,
                                  GLuint w, GLuint h, GLuint d);

void intel_miptree_set_image_offset(struct intel_mipmap_tree *mt,
                                    GLuint level,
                                    GLuint img, GLuint x, GLuint y);


/* Upload an image into a tree
 */
void intel_miptree_image_data(struct intel_context *intel,
                              struct intel_mipmap_tree *dst,
                              GLuint face,
                              GLuint level,
                              void *src,
                              GLuint src_row_pitch, GLuint src_image_pitch);

/* Copy an image between two trees
 */
void intel_miptree_image_copy(struct intel_context *intel,
                              struct intel_mipmap_tree *dst,
                              GLuint face, GLuint level,
                              struct intel_mipmap_tree *src);

/* i915_mipmap_tree.c:
 */
GLboolean i915_miptree_layout(struct intel_mipmap_tree *mt);
GLboolean i945_miptree_layout(struct intel_mipmap_tree *mt);



#endif
