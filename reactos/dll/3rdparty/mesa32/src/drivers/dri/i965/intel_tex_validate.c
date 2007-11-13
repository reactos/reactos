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

#include "mtypes.h"
#include "macros.h"

#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_tex.h"
#include "bufmgr.h"

/**
 * Compute which mipmap levels that really need to be sent to the hardware.
 * This depends on the base image size, GL_TEXTURE_MIN_LOD,
 * GL_TEXTURE_MAX_LOD, GL_TEXTURE_BASE_LEVEL, and GL_TEXTURE_MAX_LEVEL.
 */
static void intel_calculate_first_last_level( struct intel_texture_object *intelObj )
{
   struct gl_texture_object *tObj = &intelObj->base;
   const struct gl_texture_image * const baseImage =
       tObj->Image[0][tObj->BaseLevel];

   /* These must be signed values.  MinLod and MaxLod can be negative numbers,
    * and having firstLevel and lastLevel as signed prevents the need for
    * extra sign checks.
    */
   int   firstLevel;
   int   lastLevel;

   /* Yes, this looks overly complicated, but it's all needed.
    */
   switch (tObj->Target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_CUBE_MAP:
      if (tObj->MinFilter == GL_NEAREST || tObj->MinFilter == GL_LINEAR) {
         /* GL_NEAREST and GL_LINEAR only care about GL_TEXTURE_BASE_LEVEL.
          */
         firstLevel = lastLevel = tObj->BaseLevel;
      }
      else {
	 /* Currently not taking min/max lod into account here, those
	  * values are programmed as sampler state elsewhere and we
	  * upload the same mipmap levels regardless.  Not sure if
	  * this makes sense as it means it isn't possible for the app
	  * to use min/max lod to reduce texture memory pressure:
	  */
	 firstLevel = tObj->BaseLevel;
	 lastLevel = MIN2(tObj->BaseLevel + baseImage->MaxLog2, 
			  tObj->MaxLevel);
	 lastLevel = MAX2(firstLevel, lastLevel); /* need at least one level */
      }
      break;
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_TEXTURE_4D_SGIS:
      firstLevel = lastLevel = 0;
      break;
   default:
      return;
   }

   /* save these values */
   intelObj->firstLevel = firstLevel;
   intelObj->lastLevel = lastLevel;
}

static GLboolean copy_image_data_to_tree( struct intel_context *intel,
					  struct intel_texture_object *intelObj,
					  struct gl_texture_image *texImage,
					  GLuint face,
					  GLuint level)
{
   return intel_miptree_image_data(intel,
				   intelObj->mt,
				   face,
				   level,
				   texImage->Data,
				   texImage->RowStride,
				   (texImage->RowStride * 
				    texImage->Height * 
				    texImage->TexFormat->TexelBytes));
}

static void intel_texture_invalidate( struct intel_texture_object *intelObj )
{
   GLint nr_faces, face;
   intelObj->dirty = ~0;

   nr_faces = (intelObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   for (face = 0; face < nr_faces; face++) 
      intelObj->dirty_images[face] = ~0;
}

static void intel_texture_invalidate_cb( struct intel_context *intel,
					 void *ptr )
{
   intel_texture_invalidate( (struct intel_texture_object *) ptr );
}


/*  
 */
GLuint intel_finalize_mipmap_tree( struct intel_context *intel,
				   struct gl_texture_object *tObj )
{
   struct intel_texture_object *intelObj = intel_texture_object(tObj);
   GLuint face, i;
   GLuint nr_faces = 0;
   struct gl_texture_image *firstImage;

   if( tObj == intel->frame_buffer_texobj )
      return GL_FALSE;
   
   /* We know/require this is true by now: 
    */
   assert(intelObj->base.Complete);

   /* What levels must the tree include at a minimum?
    */
   if (intelObj->dirty) {
      intel_calculate_first_last_level( intelObj );
/*       intel_miptree_destroy(intel, intelObj->mt); */
/*       intelObj->mt = NULL; */
   }

   firstImage = intelObj->base.Image[0][intelObj->firstLevel];

   /* Fallback case:
    */
   if (firstImage->Border) {
      if (intelObj->mt) {
	 intel_miptree_destroy(intel, intelObj->mt);
	 intelObj->mt = NULL;
	 /* Set all images dirty:
	  */
	 intel_texture_invalidate(intelObj);
      }
      return GL_FALSE;
   }



   /* Check tree can hold all active levels.  Check tree matches
    * target, imageFormat, etc.
    */
   if (intelObj->mt &&
       (intelObj->mt->target != intelObj->base.Target ||
	intelObj->mt->internal_format != firstImage->InternalFormat ||
	intelObj->mt->first_level != intelObj->firstLevel ||
	intelObj->mt->last_level != intelObj->lastLevel ||
	intelObj->mt->width0 != firstImage->Width ||
	intelObj->mt->height0 != firstImage->Height ||
	intelObj->mt->depth0 != firstImage->Depth ||
	intelObj->mt->cpp != firstImage->TexFormat->TexelBytes ||
	intelObj->mt->compressed != firstImage->IsCompressed)) 
   {
      intel_miptree_destroy(intel, intelObj->mt);
      intelObj->mt = NULL;
      
      /* Set all images dirty:
       */
      intel_texture_invalidate(intelObj);
   }
      

   /* May need to create a new tree:
    */
   if (!intelObj->mt) {
      intelObj->mt = intel_miptree_create(intel,
					  intelObj->base.Target,
					  firstImage->InternalFormat,
					  intelObj->firstLevel,
					  intelObj->lastLevel,
					  firstImage->Width,
					  firstImage->Height,
					  firstImage->Depth,
					  firstImage->TexFormat->TexelBytes,
					  firstImage->IsCompressed);

      /* Tell the buffer manager that we will manage the backing
       * store, but we still want it to do fencing for us.
       */
      bmBufferSetInvalidateCB(intel, 
			      intelObj->mt->region->buffer,
			      intel_texture_invalidate_cb,
			      intelObj,
			      GL_FALSE);
   }

   /* Pull in any images not in the object's tree:
    */
   if (intelObj->dirty) {
      nr_faces = (intelObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
      for (face = 0; face < nr_faces; face++) {
	 if (intelObj->dirty_images[face]) {
	    for (i = intelObj->firstLevel; i <= intelObj->lastLevel; i++) {
	       struct gl_texture_image *texImage = intelObj->base.Image[face][i];

	       /* Need to import images in main memory or held in other trees.
		*/
	       if (intelObj->dirty_images[face] & (1<<i) &&
		   texImage) {

		  if (INTEL_DEBUG & DEBUG_TEXTURE)
		     _mesa_printf("copy data from image %d (%p) into object miptree\n",
				  i,
				  texImage->Data);

		  if (!copy_image_data_to_tree(intel,
					       intelObj,
					       texImage,
					       face,
					       i))
		     return GL_FALSE;

	       }
	    }
	 }
      }

      /* Only clear the dirty flags if everything went ok:
       */
      for (face = 0; face < nr_faces; face++) {
	 intelObj->dirty_images[face] = 0;
      }

      intelObj->dirty = 0;
   }

   return GL_TRUE;
}
