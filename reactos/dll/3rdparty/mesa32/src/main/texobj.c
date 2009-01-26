/**
 * \file texobj.c
 * Texture object management.
 */

/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "glheader.h"
#if FEATURE_colortable
#include "colortab.h"
#endif
#include "context.h"
#include "enums.h"
#include "fbobject.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "teximage.h"
#include "texstate.h"
#include "texobj.h"
#include "mtypes.h"


/**********************************************************************/
/** \name Internal functions */
/*@{*/


/**
 * Return the gl_texture_object for a given ID.
 */
struct gl_texture_object *
_mesa_lookup_texture(GLcontext *ctx, GLuint id)
{
   return (struct gl_texture_object *)
      _mesa_HashLookup(ctx->Shared->TexObjects, id);
}



/**
 * Allocate and initialize a new texture object.  But don't put it into the
 * texture object hash table.
 *
 * Called via ctx->Driver.NewTextureObject, unless overridden by a device
 * driver.
 * 
 * \param shared the shared GL state structure to contain the texture object
 * \param name integer name for the texture object
 * \param target either GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D,
 * GL_TEXTURE_CUBE_MAP_ARB or GL_TEXTURE_RECTANGLE_NV.  zero is ok for the sake
 * of GenTextures()
 *
 * \return pointer to new texture object.
 */
struct gl_texture_object *
_mesa_new_texture_object( GLcontext *ctx, GLuint name, GLenum target )
{
   struct gl_texture_object *obj;
   (void) ctx;
   obj = MALLOC_STRUCT(gl_texture_object);
   _mesa_initialize_texture_object(obj, name, target);
   return obj;
}


/**
 * Initialize a new texture object to default values.
 * \param obj  the texture object
 * \param name  the texture name
 * \param target  the texture target
 */
void
_mesa_initialize_texture_object( struct gl_texture_object *obj,
                                 GLuint name, GLenum target )
{
   ASSERT(target == 0 ||
          target == GL_TEXTURE_1D ||
          target == GL_TEXTURE_2D ||
          target == GL_TEXTURE_3D ||
          target == GL_TEXTURE_CUBE_MAP_ARB ||
          target == GL_TEXTURE_RECTANGLE_NV ||
          target == GL_TEXTURE_1D_ARRAY_EXT ||
          target == GL_TEXTURE_2D_ARRAY_EXT);

   _mesa_bzero(obj, sizeof(*obj));
   /* init the non-zero fields */
   _glthread_INIT_MUTEX(obj->Mutex);
   obj->RefCount = 1;
   obj->Name = name;
   obj->Target = target;
   obj->Priority = 1.0F;
   if (target == GL_TEXTURE_RECTANGLE_NV) {
      obj->WrapS = GL_CLAMP_TO_EDGE;
      obj->WrapT = GL_CLAMP_TO_EDGE;
      obj->WrapR = GL_CLAMP_TO_EDGE;
      obj->MinFilter = GL_LINEAR;
   }
   else {
      obj->WrapS = GL_REPEAT;
      obj->WrapT = GL_REPEAT;
      obj->WrapR = GL_REPEAT;
      obj->MinFilter = GL_NEAREST_MIPMAP_LINEAR;
   }
   obj->MagFilter = GL_LINEAR;
   obj->MinLod = -1000.0;
   obj->MaxLod = 1000.0;
   obj->LodBias = 0.0;
   obj->BaseLevel = 0;
   obj->MaxLevel = 1000;
   obj->MaxAnisotropy = 1.0;
   obj->CompareFlag = GL_FALSE;                      /* SGIX_shadow */
   obj->CompareOperator = GL_TEXTURE_LEQUAL_R_SGIX;  /* SGIX_shadow */
   obj->CompareMode = GL_NONE;         /* ARB_shadow */
   obj->CompareFunc = GL_LEQUAL;       /* ARB_shadow */
   obj->DepthMode = GL_LUMINANCE;      /* ARB_depth_texture */
   obj->ShadowAmbient = 0.0F;          /* ARB/SGIX_shadow_ambient */
}


/**
 * Some texture initialization can't be finished until we know which
 * target it's getting bound to (GL_TEXTURE_1D/2D/etc).
 */
static void
finish_texture_init(GLcontext *ctx, GLenum target,
                    struct gl_texture_object *obj)
{
   assert(obj->Target == 0);

   if (target == GL_TEXTURE_RECTANGLE_NV) {
      /* have to init wrap and filter state here - kind of klunky */
      obj->WrapS = GL_CLAMP_TO_EDGE;
      obj->WrapT = GL_CLAMP_TO_EDGE;
      obj->WrapR = GL_CLAMP_TO_EDGE;
      obj->MinFilter = GL_LINEAR;
      if (ctx->Driver.TexParameter) {
         static const GLfloat fparam_wrap[1] = {(GLfloat) GL_CLAMP_TO_EDGE};
         static const GLfloat fparam_filter[1] = {(GLfloat) GL_LINEAR};
         ctx->Driver.TexParameter(ctx, target, obj, GL_TEXTURE_WRAP_S, fparam_wrap);
         ctx->Driver.TexParameter(ctx, target, obj, GL_TEXTURE_WRAP_T, fparam_wrap);
         ctx->Driver.TexParameter(ctx, target, obj, GL_TEXTURE_WRAP_R, fparam_wrap);
         ctx->Driver.TexParameter(ctx, target, obj, GL_TEXTURE_MIN_FILTER, fparam_filter);
      }
   }
}


/**
 * Deallocate a texture object struct.  It should have already been
 * removed from the texture object pool.
 * Called via ctx->Driver.DeleteTexture() if not overriden by a driver.
 *
 * \param shared the shared GL state to which the object belongs.
 * \param texOjb the texture object to delete.
 */
void
_mesa_delete_texture_object( GLcontext *ctx, struct gl_texture_object *texObj )
{
   GLuint i, face;

   (void) ctx;

   /* Set Target to an invalid value.  With some assertions elsewhere
    * we can try to detect possible use of deleted textures.
    */
   texObj->Target = 0x99;

#if FEATURE_colortable
   _mesa_free_colortable_data(&texObj->Palette);
#endif

   /* free the texture images */
   for (face = 0; face < 6; face++) {
      for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
	 if (texObj->Image[face][i]) {
	    _mesa_delete_texture_image( ctx, texObj->Image[face][i] );
	 }
      }
   }

   /* destroy the mutex -- it may have allocated memory (eg on bsd) */
   _glthread_DESTROY_MUTEX(texObj->Mutex);

   /* free this object */
   _mesa_free(texObj);
}




/**
 * Copy texture object state from one texture object to another.
 * Use for glPush/PopAttrib.
 *
 * \param dest destination texture object.
 * \param src source texture object.
 */
void
_mesa_copy_texture_object( struct gl_texture_object *dest,
                           const struct gl_texture_object *src )
{
   dest->Target = src->Target;
   dest->Name = src->Name;
   dest->Priority = src->Priority;
   dest->BorderColor[0] = src->BorderColor[0];
   dest->BorderColor[1] = src->BorderColor[1];
   dest->BorderColor[2] = src->BorderColor[2];
   dest->BorderColor[3] = src->BorderColor[3];
   dest->WrapS = src->WrapS;
   dest->WrapT = src->WrapT;
   dest->WrapR = src->WrapR;
   dest->MinFilter = src->MinFilter;
   dest->MagFilter = src->MagFilter;
   dest->MinLod = src->MinLod;
   dest->MaxLod = src->MaxLod;
   dest->LodBias = src->LodBias;
   dest->BaseLevel = src->BaseLevel;
   dest->MaxLevel = src->MaxLevel;
   dest->MaxAnisotropy = src->MaxAnisotropy;
   dest->CompareFlag = src->CompareFlag;
   dest->CompareOperator = src->CompareOperator;
   dest->ShadowAmbient = src->ShadowAmbient;
   dest->CompareMode = src->CompareMode;
   dest->CompareFunc = src->CompareFunc;
   dest->DepthMode = src->DepthMode;
   dest->_MaxLevel = src->_MaxLevel;
   dest->_MaxLambda = src->_MaxLambda;
   dest->GenerateMipmap = src->GenerateMipmap;
   dest->Palette = src->Palette;
   dest->_Complete = src->_Complete;
}


/**
 * Check if the given texture object is valid by examining its Target field.
 * For debugging only.
 */
static GLboolean
valid_texture_object(const struct gl_texture_object *tex)
{
   switch (tex->Target) {
   case 0:
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_CUBE_MAP_ARB:
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_TEXTURE_1D_ARRAY_EXT:
   case GL_TEXTURE_2D_ARRAY_EXT:
      return GL_TRUE;
   case 0x99:
      _mesa_problem(NULL, "invalid reference to a deleted texture object");
      return GL_FALSE;
   default:
      _mesa_problem(NULL, "invalid texture object Target value");
      return GL_FALSE;
   }
}


/**
 * Reference (or unreference) a texture object.
 * If '*ptr', decrement *ptr's refcount (and delete if it becomes zero).
 * If 'tex' is non-null, increment its refcount.
 */
void
_mesa_reference_texobj(struct gl_texture_object **ptr,
                       struct gl_texture_object *tex)
{
   assert(ptr);
   if (*ptr == tex) {
      /* no change */
      return;
   }

   if (*ptr) {
      /* Unreference the old texture */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_texture_object *oldTex = *ptr;

      assert(valid_texture_object(oldTex));

      _glthread_LOCK_MUTEX(oldTex->Mutex);
      ASSERT(oldTex->RefCount > 0);
      oldTex->RefCount--;

      deleteFlag = (oldTex->RefCount == 0);
      _glthread_UNLOCK_MUTEX(oldTex->Mutex);

      if (deleteFlag) {
         GET_CURRENT_CONTEXT(ctx);
         if (ctx)
            ctx->Driver.DeleteTexture(ctx, oldTex);
         else
            _mesa_problem(NULL, "Unable to delete texture, no context");
      }

      *ptr = NULL;
   }
   assert(!*ptr);

   if (tex) {
      /* reference new texture */
      assert(valid_texture_object(tex));
      _glthread_LOCK_MUTEX(tex->Mutex);
      if (tex->RefCount == 0) {
         /* this texture's being deleted (look just above) */
         /* Not sure this can every really happen.  Warn if it does. */
         _mesa_problem(NULL, "referencing deleted texture object");
         *ptr = NULL;
      }
      else {
         tex->RefCount++;
         *ptr = tex;
      }
      _glthread_UNLOCK_MUTEX(tex->Mutex);
   }
}



/**
 * Report why a texture object is incomplete.  
 *
 * \param t texture object.
 * \param why string describing why it's incomplete.
 *
 * \note For debug purposes only.
 */
#if 0
static void
incomplete(const struct gl_texture_object *t, const char *why)
{
   _mesa_printf("Texture Obj %d incomplete because: %s\n", t->Name, why);
}
#else
#define incomplete(t, why)
#endif


/**
 * Examine a texture object to determine if it is complete.
 *
 * The gl_texture_object::Complete flag will be set to GL_TRUE or GL_FALSE
 * accordingly.
 *
 * \param ctx GL context.
 * \param t texture object.
 *
 * According to the texture target, verifies that each of the mipmaps is
 * present and has the expected size.
 */
void
_mesa_test_texobj_completeness( const GLcontext *ctx,
                                struct gl_texture_object *t )
{
   const GLint baseLevel = t->BaseLevel;
   GLint maxLog2 = 0, maxLevels = 0;

   t->_Complete = GL_TRUE;  /* be optimistic */

   /* Detect cases where the application set the base level to an invalid
    * value.
    */
   if ((baseLevel < 0) || (baseLevel > MAX_TEXTURE_LEVELS)) {
      char s[100];
      _mesa_sprintf(s, "obj %p (%d) base level = %d is invalid",
              (void *) t, t->Name, baseLevel);
      incomplete(t, s);
      t->_Complete = GL_FALSE;
      return;
   }

   /* Always need the base level image */
   if (!t->Image[0][baseLevel]) {
      char s[100];
      _mesa_sprintf(s, "obj %p (%d) Image[baseLevel=%d] == NULL",
              (void *) t, t->Name, baseLevel);
      incomplete(t, s);
      t->_Complete = GL_FALSE;
      return;
   }

   /* Check width/height/depth for zero */
   if (t->Image[0][baseLevel]->Width == 0 ||
       t->Image[0][baseLevel]->Height == 0 ||
       t->Image[0][baseLevel]->Depth == 0) {
      incomplete(t, "texture width = 0");
      t->_Complete = GL_FALSE;
      return;
   }

   /* Compute _MaxLevel */
   if ((t->Target == GL_TEXTURE_1D) ||
       (t->Target == GL_TEXTURE_1D_ARRAY_EXT)) {
      maxLog2 = t->Image[0][baseLevel]->WidthLog2;
      maxLevels = ctx->Const.MaxTextureLevels;
   }
   else if ((t->Target == GL_TEXTURE_2D) ||
	    (t->Target == GL_TEXTURE_2D_ARRAY_EXT)) {
      maxLog2 = MAX2(t->Image[0][baseLevel]->WidthLog2,
                     t->Image[0][baseLevel]->HeightLog2);
      maxLevels = ctx->Const.MaxTextureLevels;
   }
   else if (t->Target == GL_TEXTURE_3D) {
      GLint max = MAX2(t->Image[0][baseLevel]->WidthLog2,
                       t->Image[0][baseLevel]->HeightLog2);
      maxLog2 = MAX2(max, (GLint)(t->Image[0][baseLevel]->DepthLog2));
      maxLevels = ctx->Const.Max3DTextureLevels;
   }
   else if (t->Target == GL_TEXTURE_CUBE_MAP_ARB) {
      maxLog2 = MAX2(t->Image[0][baseLevel]->WidthLog2,
                     t->Image[0][baseLevel]->HeightLog2);
      maxLevels = ctx->Const.MaxCubeTextureLevels;
   }
   else if (t->Target == GL_TEXTURE_RECTANGLE_NV) {
      maxLog2 = 0;  /* not applicable */
      maxLevels = 1;  /* no mipmapping */
   }
   else {
      _mesa_problem(ctx, "Bad t->Target in _mesa_test_texobj_completeness");
      return;
   }

   ASSERT(maxLevels > 0);

   t->_MaxLevel = baseLevel + maxLog2;
   t->_MaxLevel = MIN2(t->_MaxLevel, t->MaxLevel);
   t->_MaxLevel = MIN2(t->_MaxLevel, maxLevels - 1);

   /* Compute _MaxLambda = q - b (see the 1.2 spec) used during mipmapping */
   t->_MaxLambda = (GLfloat) (t->_MaxLevel - t->BaseLevel);

   if (t->Target == GL_TEXTURE_CUBE_MAP_ARB) {
      /* make sure that all six cube map level 0 images are the same size */
      const GLuint w = t->Image[0][baseLevel]->Width2;
      const GLuint h = t->Image[0][baseLevel]->Height2;
      GLuint face;
      for (face = 1; face < 6; face++) {
	 if (t->Image[face][baseLevel] == NULL ||
	     t->Image[face][baseLevel]->Width2 != w ||
	     t->Image[face][baseLevel]->Height2 != h) {
	    t->_Complete = GL_FALSE;
	    incomplete(t, "Non-quare cubemap image");
	    return;
	 }
      }
   }

   /* extra checking for mipmaps */
   if (t->MinFilter != GL_NEAREST && t->MinFilter != GL_LINEAR) {
      /*
       * Mipmapping: determine if we have a complete set of mipmaps
       */
      GLint i;
      GLint minLevel = baseLevel;
      GLint maxLevel = t->_MaxLevel;

      if (minLevel > maxLevel) {
         t->_Complete = GL_FALSE;
         incomplete(t, "minLevel > maxLevel");
         return;
      }

      /* Test dimension-independent attributes */
      for (i = minLevel; i <= maxLevel; i++) {
         if (t->Image[0][i]) {
            if (t->Image[0][i]->TexFormat != t->Image[0][baseLevel]->TexFormat) {
               t->_Complete = GL_FALSE;
               incomplete(t, "Format[i] != Format[baseLevel]");
               return;
            }
            if (t->Image[0][i]->Border != t->Image[0][baseLevel]->Border) {
               t->_Complete = GL_FALSE;
               incomplete(t, "Border[i] != Border[baseLevel]");
               return;
            }
         }
      }

      /* Test things which depend on number of texture image dimensions */
      if ((t->Target == GL_TEXTURE_1D) ||
          (t->Target == GL_TEXTURE_1D_ARRAY_EXT)) {
         /* Test 1-D mipmaps */
         GLuint width = t->Image[0][baseLevel]->Width2;
         for (i = baseLevel + 1; i < maxLevels; i++) {
            if (width > 1) {
               width /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
               if (!t->Image[0][i]) {
                  t->_Complete = GL_FALSE;
                  incomplete(t, "1D Image[0][i] == NULL");
                  return;
               }
               if (t->Image[0][i]->Width2 != width ) {
                  t->_Complete = GL_FALSE;
                  incomplete(t, "1D Image[0][i] bad width");
                  return;
               }
            }
            if (width == 1) {
               return;  /* found smallest needed mipmap, all done! */
            }
         }
      }
      else if ((t->Target == GL_TEXTURE_2D) ||
               (t->Target == GL_TEXTURE_2D_ARRAY_EXT)) {
         /* Test 2-D mipmaps */
         GLuint width = t->Image[0][baseLevel]->Width2;
         GLuint height = t->Image[0][baseLevel]->Height2;
         for (i = baseLevel + 1; i < maxLevels; i++) {
            if (width > 1) {
               width /= 2;
            }
            if (height > 1) {
               height /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
               if (!t->Image[0][i]) {
                  t->_Complete = GL_FALSE;
                  incomplete(t, "2D Image[0][i] == NULL");
                  return;
               }
               if (t->Image[0][i]->Width2 != width) {
                  t->_Complete = GL_FALSE;
                  incomplete(t, "2D Image[0][i] bad width");
                  return;
               }
               if (t->Image[0][i]->Height2 != height) {
                  t->_Complete = GL_FALSE;
                  incomplete(t, "2D Image[0][i] bad height");
                  return;
               }
               if (width==1 && height==1) {
                  return;  /* found smallest needed mipmap, all done! */
               }
            }
         }
      }
      else if (t->Target == GL_TEXTURE_3D) {
         /* Test 3-D mipmaps */
         GLuint width = t->Image[0][baseLevel]->Width2;
         GLuint height = t->Image[0][baseLevel]->Height2;
         GLuint depth = t->Image[0][baseLevel]->Depth2;
	 for (i = baseLevel + 1; i < maxLevels; i++) {
            if (width > 1) {
               width /= 2;
            }
            if (height > 1) {
               height /= 2;
            }
            if (depth > 1) {
               depth /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
               if (!t->Image[0][i]) {
                  incomplete(t, "3D Image[0][i] == NULL");
                  t->_Complete = GL_FALSE;
                  return;
               }
               if (t->Image[0][i]->_BaseFormat == GL_DEPTH_COMPONENT) {
                  t->_Complete = GL_FALSE;
                  incomplete(t, "GL_DEPTH_COMPONENT only works with 1/2D tex");
                  return;
               }
               if (t->Image[0][i]->Width2 != width) {
                  t->_Complete = GL_FALSE;
                  incomplete(t, "3D Image[0][i] bad width");
                  return;
               }
               if (t->Image[0][i]->Height2 != height) {
                  t->_Complete = GL_FALSE;
                  incomplete(t, "3D Image[0][i] bad height");
                  return;
               }
               if (t->Image[0][i]->Depth2 != depth) {
                  t->_Complete = GL_FALSE;
                  incomplete(t, "3D Image[0][i] bad depth");
                  return;
               }
            }
            if (width == 1 && height == 1 && depth == 1) {
               return;  /* found smallest needed mipmap, all done! */
            }
         }
      }
      else if (t->Target == GL_TEXTURE_CUBE_MAP_ARB) {
         /* make sure 6 cube faces are consistant */
         GLuint width = t->Image[0][baseLevel]->Width2;
         GLuint height = t->Image[0][baseLevel]->Height2;
	 for (i = baseLevel + 1; i < maxLevels; i++) {
            if (width > 1) {
               width /= 2;
            }
            if (height > 1) {
               height /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
	       GLuint face;
	       for (face = 0; face < 6; face++) {
		  /* check that we have images defined */
		  if (!t->Image[face][i]) {
		     t->_Complete = GL_FALSE;
		     incomplete(t, "CubeMap Image[n][i] == NULL");
		     return;
		  }
		  /* Don't support GL_DEPTH_COMPONENT for cube maps */
		  if (t->Image[face][i]->_BaseFormat == GL_DEPTH_COMPONENT) {
		     t->_Complete = GL_FALSE;
		     incomplete(t, "GL_DEPTH_COMPONENT only works with 1/2D tex");
		     return;
		  }
		  /* check that all six images have same size */
		  if (t->Image[face][i]->Width2!=width || 
		      t->Image[face][i]->Height2!=height) {
		     t->_Complete = GL_FALSE;
		     incomplete(t, "CubeMap Image[n][i] bad size");
		     return;
		  }
	       }
	    }
	    if (width == 1 && height == 1) {
	       return;  /* found smallest needed mipmap, all done! */
            }
         }
      }
      else if (t->Target == GL_TEXTURE_RECTANGLE_NV) {
         /* XXX special checking? */
      }
      else {
         /* Target = ??? */
         _mesa_problem(ctx, "Bug in gl_test_texture_object_completeness\n");
      }
   }
}

/*@}*/


/***********************************************************************/
/** \name API functions */
/*@{*/


/**
 * Generate texture names.
 *
 * \param n number of texture names to be generated.
 * \param textures an array in which will hold the generated texture names.
 *
 * \sa glGenTextures().
 *
 * Calls _mesa_HashFindFreeKeyBlock() to find a block of free texture
 * IDs which are stored in \p textures.  Corresponding empty texture
 * objects are also generated.
 */ 
void GLAPIENTRY
_mesa_GenTextures( GLsizei n, GLuint *textures )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glGenTextures" );
      return;
   }

   if (!textures)
      return;

   /*
    * This must be atomic (generation and allocation of texture IDs)
    */
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->TexObjects, n);

   /* Allocate new, empty texture objects */
   for (i = 0; i < n; i++) {
      struct gl_texture_object *texObj;
      GLuint name = first + i;
      GLenum target = 0;
      texObj = (*ctx->Driver.NewTextureObject)( ctx, name, target);
      if (!texObj) {
         _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenTextures");
         return;
      }

      /* insert into hash table */
      _mesa_HashInsert(ctx->Shared->TexObjects, texObj->Name, texObj);

      textures[i] = name;
   }

   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
}


/**
 * Check if the given texture object is bound to the current draw or
 * read framebuffer.  If so, Unbind it.
 */
static void
unbind_texobj_from_fbo(GLcontext *ctx, struct gl_texture_object *texObj)
{
   const GLuint n = (ctx->DrawBuffer == ctx->ReadBuffer) ? 1 : 2;
   GLuint i;

   for (i = 0; i < n; i++) {
      struct gl_framebuffer *fb = (i == 0) ? ctx->DrawBuffer : ctx->ReadBuffer;
      if (fb->Name) {
         GLuint j;
         for (j = 0; j < BUFFER_COUNT; j++) {
            if (fb->Attachment[j].Type == GL_TEXTURE &&
                fb->Attachment[j].Texture == texObj) {
               _mesa_remove_attachment(ctx, fb->Attachment + j);         
            }
         }
      }
   }
}


/**
 * Check if the given texture object is bound to any texture image units and
 * unbind it if so (revert to default textures).
 */
static void
unbind_texobj_from_texunits(GLcontext *ctx, struct gl_texture_object *texObj)
{
   GLuint u;

   for (u = 0; u < MAX_TEXTURE_IMAGE_UNITS; u++) {
      struct gl_texture_unit *unit = &ctx->Texture.Unit[u];
      if (texObj == unit->Current1D) {
         _mesa_reference_texobj(&unit->Current1D, ctx->Shared->Default1D);
         ASSERT(unit->Current1D);
      }
      else if (texObj == unit->Current2D) {
         _mesa_reference_texobj(&unit->Current2D, ctx->Shared->Default2D);
         ASSERT(unit->Current2D);
      }
      else if (texObj == unit->Current3D) {
         _mesa_reference_texobj(&unit->Current3D, ctx->Shared->Default3D);
         ASSERT(unit->Current3D);
      }
      else if (texObj == unit->CurrentCubeMap) {
         _mesa_reference_texobj(&unit->CurrentCubeMap, ctx->Shared->DefaultCubeMap);
         ASSERT(unit->CurrentCubeMap);
      }
      else if (texObj == unit->CurrentRect) {
         _mesa_reference_texobj(&unit->CurrentRect, ctx->Shared->DefaultRect);
         ASSERT(unit->CurrentRect);
      }
      else if (texObj == unit->Current1DArray) {
         _mesa_reference_texobj(&unit->Current1DArray, ctx->Shared->Default1DArray);
         ASSERT(unit->Current1DArray);
      }
      else if (texObj == unit->Current2DArray) {
         _mesa_reference_texobj(&unit->Current2DArray, ctx->Shared->Default2DArray);
         ASSERT(unit->Current2DArray);
      }
   }
}


/**
 * Delete named textures.
 *
 * \param n number of textures to be deleted.
 * \param textures array of texture IDs to be deleted.
 *
 * \sa glDeleteTextures().
 *
 * If we're about to delete a texture that's currently bound to any
 * texture unit, unbind the texture first.  Decrement the reference
 * count on the texture object and delete it if it's zero.
 * Recall that texture objects can be shared among several rendering
 * contexts.
 */
void GLAPIENTRY
_mesa_DeleteTextures( GLsizei n, const GLuint *textures)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx); /* too complex */

   if (!textures)
      return;

   for (i = 0; i < n; i++) {
      if (textures[i] > 0) {
         struct gl_texture_object *delObj
            = _mesa_lookup_texture(ctx, textures[i]);

         if (delObj) {
	    _mesa_lock_texture(ctx, delObj);

            /* Check if texture is bound to any framebuffer objects.
             * If so, unbind.
             * See section 4.4.2.3 of GL_EXT_framebuffer_object.
             */
            unbind_texobj_from_fbo(ctx, delObj);

            /* Check if this texture is currently bound to any texture units.
             * If so, unbind it.
             */
            unbind_texobj_from_texunits(ctx, delObj);

	    _mesa_unlock_texture(ctx, delObj);

            ctx->NewState |= _NEW_TEXTURE;

            /* The texture _name_ is now free for re-use.
             * Remove it from the hash table now.
             */
            _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
            _mesa_HashRemove(ctx->Shared->TexObjects, delObj->Name);
            _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

            /* Unreference the texobj.  If refcount hits zero, the texture
             * will be deleted.
             */
            _mesa_reference_texobj(&delObj, NULL);
         }
      }
   }
}


/**
 * Bind a named texture to a texturing target.
 * 
 * \param target texture target.
 * \param texName texture name.
 * 
 * \sa glBindTexture().
 *
 * Determines the old texture object bound and returns immediately if rebinding
 * the same texture.  Get the current texture which is either a default texture
 * if name is null, a named texture from the hash, or a new texture if the
 * given texture name is new. Increments its reference count, binds it, and
 * calls dd_function_table::BindTexture. Decrements the old texture reference
 * count and deletes it if it reaches zero.
 */
void GLAPIENTRY
_mesa_BindTexture( GLenum target, GLuint texName )
{
   GET_CURRENT_CONTEXT(ctx);
   const GLuint unit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *newTexObj = NULL, *defaultTexObj = NULL;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glBindTexture %s %d\n",
                  _mesa_lookup_enum_by_nr(target), (GLint) texName);

   switch (target) {
   case GL_TEXTURE_1D:
      defaultTexObj = ctx->Shared->Default1D;
      break;
   case GL_TEXTURE_2D:
      defaultTexObj = ctx->Shared->Default2D;
      break;
   case GL_TEXTURE_3D:
      defaultTexObj = ctx->Shared->Default3D;
      break;
   case GL_TEXTURE_CUBE_MAP_ARB:
      defaultTexObj = ctx->Shared->DefaultCubeMap;
      break;
   case GL_TEXTURE_RECTANGLE_NV:
      defaultTexObj = ctx->Shared->DefaultRect;
      break;
   case GL_TEXTURE_1D_ARRAY_EXT:
      defaultTexObj = ctx->Shared->Default1DArray;
      break;
   case GL_TEXTURE_2D_ARRAY_EXT:
      defaultTexObj = ctx->Shared->Default2DArray;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindTexture(target)");
      return;
   }

   /*
    * Get pointer to new texture object (newTexObj)
    */
   if (texName == 0) {
      newTexObj = defaultTexObj;
   }
   else {
      /* non-default texture object */
      newTexObj = _mesa_lookup_texture(ctx, texName);
      if (newTexObj) {
         /* error checking */
         if (newTexObj->Target != 0 && newTexObj->Target != target) {
            /* the named texture object's target doesn't match the given target */
            _mesa_error( ctx, GL_INVALID_OPERATION,
                         "glBindTexture(target mismatch)" );
            return;
         }
         if (newTexObj->Target == 0) {
            finish_texture_init(ctx, target, newTexObj);
         }
      }
      else {
         /* if this is a new texture id, allocate a texture object now */
	 newTexObj = (*ctx->Driver.NewTextureObject)(ctx, texName, target);
         if (!newTexObj) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindTexture");
            return;
         }

         /* and insert it into hash table */
         _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
         _mesa_HashInsert(ctx->Shared->TexObjects, texName, newTexObj);
         _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
      }
      newTexObj->Target = target;
   }

   assert(valid_texture_object(newTexObj));

   /* flush before changing binding */
   FLUSH_VERTICES(ctx, _NEW_TEXTURE);

   /* Do the actual binding.  The refcount on the previously bound
    * texture object will be decremented.  It'll be deleted if the
    * count hits zero.
    */
   switch (target) {
      case GL_TEXTURE_1D:
         _mesa_reference_texobj(&texUnit->Current1D, newTexObj);
         ASSERT(texUnit->Current1D);
         break;
      case GL_TEXTURE_2D:
         _mesa_reference_texobj(&texUnit->Current2D, newTexObj);
         ASSERT(texUnit->Current2D);
         break;
      case GL_TEXTURE_3D:
         _mesa_reference_texobj(&texUnit->Current3D, newTexObj);
         ASSERT(texUnit->Current3D);
         break;
      case GL_TEXTURE_CUBE_MAP_ARB:
         _mesa_reference_texobj(&texUnit->CurrentCubeMap, newTexObj);
         ASSERT(texUnit->CurrentCubeMap);
         break;
      case GL_TEXTURE_RECTANGLE_NV:
         _mesa_reference_texobj(&texUnit->CurrentRect, newTexObj);
         ASSERT(texUnit->CurrentRect);
         break;
      case GL_TEXTURE_1D_ARRAY_EXT:
         texUnit->Current1DArray = newTexObj;
         ASSERT(texUnit->Current1DArray);
         break;
      case GL_TEXTURE_2D_ARRAY_EXT:
         texUnit->Current2DArray = newTexObj;
         ASSERT(texUnit->Current2DArray);
         break;
      default:
         /* Bad target should be caught above */
         _mesa_problem(ctx, "bad target in BindTexture");
         return;
   }

   /* Pass BindTexture call to device driver */
   if (ctx->Driver.BindTexture)
      (*ctx->Driver.BindTexture)( ctx, target, newTexObj );
}


/**
 * Set texture priorities.
 * 
 * \param n number of textures.
 * \param texName texture names.
 * \param priorities corresponding texture priorities.
 * 
 * \sa glPrioritizeTextures().
 * 
 * Looks up each texture in the hash, clamps the corresponding priority between
 * 0.0 and 1.0, and calls dd_function_table::PrioritizeTexture.
 */
void GLAPIENTRY
_mesa_PrioritizeTextures( GLsizei n, const GLuint *texName,
                          const GLclampf *priorities )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (n < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glPrioritizeTextures" );
      return;
   }

   if (!priorities)
      return;

   for (i = 0; i < n; i++) {
      if (texName[i] > 0) {
         struct gl_texture_object *t = _mesa_lookup_texture(ctx, texName[i]);
         if (t) {
            t->Priority = CLAMP( priorities[i], 0.0F, 1.0F );
	    if (ctx->Driver.PrioritizeTexture)
	       ctx->Driver.PrioritizeTexture( ctx, t, t->Priority );
         }
      }
   }

   ctx->NewState |= _NEW_TEXTURE;
}

/**
 * See if textures are loaded in texture memory.
 * 
 * \param n number of textures to query.
 * \param texName array with the texture names.
 * \param residences array which will hold the residence status.
 *
 * \return GL_TRUE if all textures are resident and \p residences is left unchanged, 
 * 
 * \sa glAreTexturesResident().
 *
 * Looks up each texture in the hash and calls
 * dd_function_table::IsTextureResident.
 */
GLboolean GLAPIENTRY
_mesa_AreTexturesResident(GLsizei n, const GLuint *texName,
                          GLboolean *residences)
{
   GET_CURRENT_CONTEXT(ctx);
   GLboolean allResident = GL_TRUE;
   GLint i, j;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glAreTexturesResident(n)");
      return GL_FALSE;
   }

   if (!texName || !residences)
      return GL_FALSE;

   for (i = 0; i < n; i++) {
      struct gl_texture_object *t;
      if (texName[i] == 0) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glAreTexturesResident");
         return GL_FALSE;
      }
      t = _mesa_lookup_texture(ctx, texName[i]);
      if (!t) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glAreTexturesResident");
         return GL_FALSE;
      }
      if (!ctx->Driver.IsTextureResident ||
          ctx->Driver.IsTextureResident(ctx, t)) {
         /* The texture is resident */
	 if (!allResident)
	    residences[i] = GL_TRUE;
      }
      else {
         /* The texture is not resident */
         if (allResident) {
	    allResident = GL_FALSE;
	    for (j = 0; j < i; j++)
	       residences[j] = GL_TRUE;
	 }
	 residences[i] = GL_FALSE;
      }
   }
   
   return allResident;
}

/**
 * See if a name corresponds to a texture.
 *
 * \param texture texture name.
 *
 * \return GL_TRUE if texture name corresponds to a texture, or GL_FALSE
 * otherwise.
 * 
 * \sa glIsTexture().
 *
 * Calls _mesa_HashLookup().
 */
GLboolean GLAPIENTRY
_mesa_IsTexture( GLuint texture )
{
   struct gl_texture_object *t;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (!texture)
      return GL_FALSE;

   t = _mesa_lookup_texture(ctx, texture);

   /* IsTexture is true only after object has been bound once. */
   return t && t->Target;
}


/**
 * Simplest implementation of texture locking: Grab the a new mutex in
 * the shared context.  Examine the shared context state timestamp and
 * if there has been a change, set the appropriate bits in
 * ctx->NewState.
 *
 * This is used to deal with synchronizing things when a texture object
 * is used/modified by different contexts (or threads) which are sharing
 * the texture.
 *
 * See also _mesa_lock/unlock_texture() in teximage.h
 */
void
_mesa_lock_context_textures( GLcontext *ctx )
{
   _glthread_LOCK_MUTEX(ctx->Shared->TexMutex);

   if (ctx->Shared->TextureStateStamp != ctx->TextureStateTimestamp) {
      ctx->NewState |= _NEW_TEXTURE;
      ctx->TextureStateTimestamp = ctx->Shared->TextureStateStamp;
   }
}


void
_mesa_unlock_context_textures( GLcontext *ctx )
{
   assert(ctx->Shared->TextureStateStamp == ctx->TextureStateTimestamp);
   _glthread_UNLOCK_MUTEX(ctx->Shared->TexMutex);
}

/*@}*/


