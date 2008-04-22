#include "mtypes.h"
#include "macros.h"

#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_mipmap_tree.h"
#include "intel_tex.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

/**
 * Compute which mipmap levels that really need to be sent to the hardware.
 * This depends on the base image size, GL_TEXTURE_MIN_LOD,
 * GL_TEXTURE_MAX_LOD, GL_TEXTURE_BASE_LEVEL, and GL_TEXTURE_MAX_LEVEL.
 */
static void
intel_calculate_first_last_level(struct intel_texture_object *intelObj)
{
   struct gl_texture_object *tObj = &intelObj->base;
   const struct gl_texture_image *const baseImage =
      tObj->Image[0][tObj->BaseLevel];

   /* These must be signed values.  MinLod and MaxLod can be negative numbers,
    * and having firstLevel and lastLevel as signed prevents the need for
    * extra sign checks.
    */
   int firstLevel;
   int lastLevel;

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
         firstLevel = tObj->BaseLevel + (GLint) (tObj->MinLod + 0.5);
         firstLevel = MAX2(firstLevel, tObj->BaseLevel);
         lastLevel = tObj->BaseLevel + (GLint) (tObj->MaxLod + 0.5);
         lastLevel = MAX2(lastLevel, tObj->BaseLevel);
         lastLevel = MIN2(lastLevel, tObj->BaseLevel + baseImage->MaxLog2);
         lastLevel = MIN2(lastLevel, tObj->MaxLevel);
         lastLevel = MAX2(firstLevel, lastLevel);       /* need at least one level */
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

static void
copy_image_data_to_tree(struct intel_context *intel,
                        struct intel_texture_object *intelObj,
                        struct intel_texture_image *intelImage)
{
   if (intelImage->mt) {
      /* Copy potentially with the blitter:
       */
      intel_miptree_image_copy(intel,
                               intelObj->mt,
                               intelImage->face,
                               intelImage->level, intelImage->mt);

      intel_miptree_release(intel, &intelImage->mt);
   }
   else {
      assert(intelImage->base.Data != NULL);

      /* More straightforward upload.  
       */
      intel_miptree_image_data(intel,
                               intelObj->mt,
                               intelImage->face,
                               intelImage->level,
                               intelImage->base.Data,
                               intelImage->base.RowStride,
                               intelImage->base.RowStride *
                               intelImage->base.Height);
      _mesa_align_free(intelImage->base.Data);
      intelImage->base.Data = NULL;
   }

   intel_miptree_reference(&intelImage->mt, intelObj->mt);
}


/*  
 */
GLuint
intel_finalize_mipmap_tree(struct intel_context *intel, GLuint unit)
{
   struct gl_texture_object *tObj = intel->ctx.Texture.Unit[unit]._Current;
   struct intel_texture_object *intelObj = intel_texture_object(tObj);
   int comp_byte = 0;
   int cpp;

   GLuint face, i;
   GLuint nr_faces = 0;
   struct intel_texture_image *firstImage;

   GLboolean need_flush = GL_FALSE;

   /* We know/require this is true by now: 
    */
   assert(intelObj->base.Complete);

   /* What levels must the tree include at a minimum?
    */
   intel_calculate_first_last_level(intelObj);
   firstImage =
      intel_texture_image(intelObj->base.Image[0][intelObj->firstLevel]);

   /* Fallback case:
    */
   if (firstImage->base.Border) {
      if (intelObj->mt) {
         intel_miptree_release(intel, &intelObj->mt);
      }
      return GL_FALSE;
   }


   /* If both firstImage and intelObj have a tree which can contain
    * all active images, favour firstImage.  Note that because of the
    * completeness requirement, we know that the image dimensions
    * will match.
    */
   if (firstImage->mt &&
       firstImage->mt != intelObj->mt &&
       firstImage->mt->first_level <= intelObj->firstLevel &&
       firstImage->mt->last_level >= intelObj->lastLevel) {

      if (intelObj->mt)
         intel_miptree_release(intel, &intelObj->mt);

      intel_miptree_reference(&intelObj->mt, firstImage->mt);
   }

   if (firstImage->base.IsCompressed) {
      comp_byte = intel_compressed_num_bytes(firstImage->base.TexFormat->MesaFormat);
      cpp = comp_byte;
   }
   else cpp = firstImage->base.TexFormat->TexelBytes;

   /* Check tree can hold all active levels.  Check tree matches
    * target, imageFormat, etc.
    * 
    * XXX: For some layouts (eg i945?), the test might have to be
    * first_level == firstLevel, as the tree isn't valid except at the
    * original start level.  Hope to get around this by
    * programming minLod, maxLod, baseLevel into the hardware and
    * leaving the tree alone.
    */
   if (intelObj->mt &&
       (intelObj->mt->target != intelObj->base.Target ||
	intelObj->mt->internal_format != firstImage->base.InternalFormat ||
	intelObj->mt->first_level != intelObj->firstLevel ||
	intelObj->mt->last_level != intelObj->lastLevel ||
	intelObj->mt->width0 != firstImage->base.Width ||
	intelObj->mt->height0 != firstImage->base.Height ||
	intelObj->mt->depth0 != firstImage->base.Depth ||
	intelObj->mt->cpp != cpp ||
	intelObj->mt->compressed != firstImage->base.IsCompressed)) {
      intel_miptree_release(intel, &intelObj->mt);
   }


   /* May need to create a new tree:
    */
   if (!intelObj->mt) {
      intelObj->mt = intel_miptree_create(intel,
                                          intelObj->base.Target,
                                          firstImage->base.InternalFormat,
                                          intelObj->firstLevel,
                                          intelObj->lastLevel,
                                          firstImage->base.Width,
                                          firstImage->base.Height,
                                          firstImage->base.Depth,
                                          cpp,
                                          comp_byte);
   }

   /* Pull in any images not in the object's tree:
    */
   nr_faces = (intelObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   for (face = 0; face < nr_faces; face++) {
      for (i = intelObj->firstLevel; i <= intelObj->lastLevel; i++) {
         struct intel_texture_image *intelImage =
            intel_texture_image(intelObj->base.Image[face][i]);

         /* Need to import images in main memory or held in other trees.
          */
         if (intelObj->mt != intelImage->mt) {
            copy_image_data_to_tree(intel, intelObj, intelImage);
	    need_flush = GL_TRUE;
         }
      }
   }

   if (need_flush)
      intel_batchbuffer_flush(intel->batch);

   return GL_TRUE;
}



void
intel_tex_map_images(struct intel_context *intel,
                     struct intel_texture_object *intelObj)
{
   GLuint nr_faces = (intelObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   GLuint face, i;

   DBG("%s\n", __FUNCTION__);

   for (face = 0; face < nr_faces; face++) {
      for (i = intelObj->firstLevel; i <= intelObj->lastLevel; i++) {
         struct intel_texture_image *intelImage =
            intel_texture_image(intelObj->base.Image[face][i]);

         if (intelImage->mt) {
            intelImage->base.Data =
               intel_miptree_image_map(intel,
                                       intelImage->mt,
                                       intelImage->face,
                                       intelImage->level,
                                       &intelImage->base.RowStride,
                                       intelImage->base.ImageOffsets);
            /* convert stride to texels, not bytes */
            intelImage->base.RowStride /= intelImage->mt->cpp;
/*             intelImage->base.ImageStride /= intelImage->mt->cpp; */
         }
      }
   }
}



void
intel_tex_unmap_images(struct intel_context *intel,
                       struct intel_texture_object *intelObj)
{
   GLuint nr_faces = (intelObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   GLuint face, i;

   for (face = 0; face < nr_faces; face++) {
      for (i = intelObj->firstLevel; i <= intelObj->lastLevel; i++) {
         struct intel_texture_image *intelImage =
            intel_texture_image(intelObj->base.Image[face][i]);

         if (intelImage->mt) {
            intel_miptree_image_unmap(intel, intelImage->mt);
            intelImage->base.Data = NULL;
         }
      }
   }
}
