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

#include <precomp.h>

/**********************************************************************/
/** \name Internal functions */
/*@{*/


/**
 * Return the gl_texture_object for a given ID.
 */
struct gl_texture_object *
_mesa_lookup_texture(struct gl_context *ctx, GLuint id)
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
_mesa_new_texture_object( struct gl_context *ctx, GLuint name, GLenum target )
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
          target == GL_TEXTURE_CUBE_MAP_ARB);

   memset(obj, 0, sizeof(*obj));
   /* init the non-zero fields */
   _glthread_INIT_MUTEX(obj->Mutex);
   obj->RefCount = 1;
   obj->Name = name;
   obj->Target = target;
   obj->Priority = 1.0F;
   obj->BaseLevel = 0;
   obj->MaxLevel = 1000;

   /* sampler state */
   obj->Sampler.WrapS = GL_REPEAT;
   obj->Sampler.WrapT = GL_REPEAT;
   obj->Sampler.WrapR = GL_REPEAT;
   obj->Sampler.MinFilter = GL_NEAREST_MIPMAP_LINEAR;

   obj->Sampler.MagFilter = GL_LINEAR;
   obj->Sampler.MaxAnisotropy = 1.0;
}


/**
 * Deallocate a texture object struct.  It should have already been
 * removed from the texture object pool.
 * Called via ctx->Driver.DeleteTexture() if not overriden by a driver.
 *
 * \param shared the shared GL state to which the object belongs.
 * \param texObj the texture object to delete.
 */
void
_mesa_delete_texture_object(struct gl_context *ctx,
                            struct gl_texture_object *texObj)
{
   GLuint i, face;

   /* Set Target to an invalid value.  With some assertions elsewhere
    * we can try to detect possible use of deleted textures.
    */
   texObj->Target = 0x99;

   /* free the texture images */
   for (face = 0; face < 6; face++) {
      for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
         if (texObj->Image[face][i]) {
            ctx->Driver.DeleteTextureImage(ctx, texObj->Image[face][i]);
         }
      }
   }

   /* destroy the mutex -- it may have allocated memory (eg on bsd) */
   _glthread_DESTROY_MUTEX(texObj->Mutex);

   /* free this object */
   free(texObj);
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
   dest->Sampler.BorderColor.f[0] = src->Sampler.BorderColor.f[0];
   dest->Sampler.BorderColor.f[1] = src->Sampler.BorderColor.f[1];
   dest->Sampler.BorderColor.f[2] = src->Sampler.BorderColor.f[2];
   dest->Sampler.BorderColor.f[3] = src->Sampler.BorderColor.f[3];
   dest->Sampler.WrapS = src->Sampler.WrapS;
   dest->Sampler.WrapT = src->Sampler.WrapT;
   dest->Sampler.WrapR = src->Sampler.WrapR;
   dest->Sampler.MinFilter = src->Sampler.MinFilter;
   dest->Sampler.MagFilter = src->Sampler.MagFilter;
   dest->BaseLevel = src->BaseLevel;
   dest->MaxLevel = src->MaxLevel;
   dest->Sampler.MaxAnisotropy = src->Sampler.MaxAnisotropy;
   dest->_MaxLevel = src->_MaxLevel;
   dest->_MaxLambda = src->_MaxLambda;
   dest->_Complete = src->_Complete;
}


/**
 * Free all texture images of the given texture object.
 *
 * \param ctx GL context.
 * \param t texture object.
 *
 * \sa _mesa_clear_texture_image().
 */
void
_mesa_clear_texture_object(struct gl_context *ctx,
                           struct gl_texture_object *texObj)
{
   GLuint i, j;

   if (texObj->Target == 0)
      return;

   for (i = 0; i < MAX_FACES; i++) {
      for (j = 0; j < MAX_TEXTURE_LEVELS; j++) {
         struct gl_texture_image *texImage = texObj->Image[i][j];
         if (texImage)
            _mesa_clear_texture_image(ctx, texImage);
      }
   }
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
   case GL_TEXTURE_CUBE_MAP_ARB:
      return GL_TRUE;
   case 0x99:
      _mesa_problem(NULL, "invalid reference to a deleted texture object");
      return GL_FALSE;
   default:
      _mesa_problem(NULL, "invalid texture object Target 0x%x, Id = %u",
                    tex->Target, tex->Name);
      return GL_FALSE;
   }
}


/**
 * Reference (or unreference) a texture object.
 * If '*ptr', decrement *ptr's refcount (and delete if it becomes zero).
 * If 'tex' is non-null, increment its refcount.
 * This is normally only called from the _mesa_reference_texobj() macro
 * when there's a real pointer change.
 */
void
_mesa_reference_texobj_(struct gl_texture_object **ptr,
                        struct gl_texture_object *tex)
{
   assert(ptr);

   if (*ptr) {
      /* Unreference the old texture */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_texture_object *oldTex = *ptr;

      ASSERT(valid_texture_object(oldTex));
      (void) valid_texture_object; /* silence warning in release builds */

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
      ASSERT(valid_texture_object(tex));
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
 * Mark a texture object as incomplete.
 * \param t  texture object
 * \param fmt...  string describing why it's incomplete (for debugging).
 */
static void
incomplete(struct gl_texture_object *t, const char *fmt, ...)
{
#if 0
   va_list args;
   char s[100];

   va_start(args, fmt);
   vsnprintf(s, sizeof(s), fmt, args);
   va_end(args);

   printf("Texture Obj %d incomplete because: %s\n", t->Name, s);
#endif
   t->_Complete = GL_FALSE;
}


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
_mesa_test_texobj_completeness( const struct gl_context *ctx,
                                struct gl_texture_object *t )
{
   const GLint baseLevel = t->BaseLevel;
   GLint maxLog2 = 0, maxLevels = 0;

   t->_Complete = GL_TRUE;  /* be optimistic */

   /* Detect cases where the application set the base level to an invalid
    * value.
    */
   if ((baseLevel < 0) || (baseLevel >= MAX_TEXTURE_LEVELS)) {
      incomplete(t, "base level = %d is invalid", baseLevel);
      return;
   }

   /* Always need the base level image */
   if (!t->Image[0][baseLevel]) {
      incomplete(t, "Image[baseLevel=%d] == NULL", baseLevel);
      return;
   }

   /* Check width/height/depth for zero */
   if (t->Image[0][baseLevel]->Width == 0 ||
       t->Image[0][baseLevel]->Height == 0 ||
       t->Image[0][baseLevel]->Depth == 0) {
      incomplete(t, "texture width = 0");
      return;
   }

   /* Compute _MaxLevel */
   if (t->Target == GL_TEXTURE_1D) {
      maxLog2 = t->Image[0][baseLevel]->WidthLog2;
      maxLevels = ctx->Const.MaxTextureLevels;
   }
   else if (t->Target == GL_TEXTURE_2D) {
      maxLog2 = MAX2(t->Image[0][baseLevel]->WidthLog2,
                     t->Image[0][baseLevel]->HeightLog2);
      maxLevels = ctx->Const.MaxTextureLevels;
   }
   else if (t->Target == GL_TEXTURE_CUBE_MAP_ARB) {
      maxLog2 = MAX2(t->Image[0][baseLevel]->WidthLog2,
                     t->Image[0][baseLevel]->HeightLog2);
      maxLevels = ctx->Const.MaxCubeTextureLevels;
   }
   else {
      _mesa_problem(ctx, "Bad t->Target in _mesa_test_texobj_completeness");
      return;
   }

   ASSERT(maxLevels > 0);

   if (t->MaxLevel < t->BaseLevel) {
      incomplete(t, "MAX_LEVEL (%d) < BASE_LEVEL (%d)",
		 t->MaxLevel, t->BaseLevel);
      return;
   }

   t->_MaxLevel = baseLevel + maxLog2;
   t->_MaxLevel = MIN2(t->_MaxLevel, t->MaxLevel);
   t->_MaxLevel = MIN2(t->_MaxLevel, maxLevels - 1);

   /* Compute _MaxLambda = q - b (see the 1.2 spec) used during mipmapping */
   t->_MaxLambda = (GLfloat) (t->_MaxLevel - t->BaseLevel);

   if (t->Immutable) {
      /* This texture object was created with glTexStorage1/2/3D() so we
       * know that all the mipmap levels are the right size and all cube
       * map faces are the same size.
       * We don't need to do any of the additional checks below.
       */
      return;
   }

   if (t->Target == GL_TEXTURE_CUBE_MAP_ARB) {
      /* make sure that all six cube map level 0 images are the same size */
      const GLuint w = t->Image[0][baseLevel]->Width2;
      const GLuint h = t->Image[0][baseLevel]->Height2;
      GLuint face;
      for (face = 1; face < 6; face++) {
         if (t->Image[face][baseLevel] == NULL ||
             t->Image[face][baseLevel]->Width2 != w ||
             t->Image[face][baseLevel]->Height2 != h) {
            incomplete(t, "Cube face missing or mismatched size");
            return;
         }
      }
   }

   /* extra checking for mipmaps */
   if (t->Sampler.MinFilter != GL_NEAREST && t->Sampler.MinFilter != GL_LINEAR) {
      /*
       * Mipmapping: determine if we have a complete set of mipmaps
       */
      GLint i;
      GLint minLevel = baseLevel;
      GLint maxLevel = t->_MaxLevel;

      if (minLevel > maxLevel) {
         incomplete(t, "minLevel > maxLevel");
         return;
      }

      /* Test dimension-independent attributes */
      for (i = minLevel; i <= maxLevel; i++) {
         if (t->Image[0][i]) {
            if (t->Image[0][i]->TexFormat != t->Image[0][baseLevel]->TexFormat) {
               incomplete(t, "Format[i] != Format[baseLevel]");
               return;
            }
            if (t->Image[0][i]->Border != t->Image[0][baseLevel]->Border) {
               incomplete(t, "Border[i] != Border[baseLevel]");
               return;
            }
         }
      }

      /* Test things which depend on number of texture image dimensions */
      if (t->Target == GL_TEXTURE_1D) {
         /* Test 1-D mipmaps */
         GLuint width = t->Image[0][baseLevel]->Width2;
         for (i = baseLevel + 1; i < maxLevels; i++) {
            if (width > 1) {
               width /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
               const struct gl_texture_image *img = t->Image[0][i];
               if (!img) {
                  incomplete(t, "1D Image[%d] is missing", i);
                  return;
               }
               if (img->Width2 != width ) {
                  incomplete(t, "1D Image[%d] bad width %u", i, img->Width2);
                  return;
               }
            }
            if (width == 1) {
               return;  /* found smallest needed mipmap, all done! */
            }
         }
      }
      else if (t->Target == GL_TEXTURE_2D) {
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
               const struct gl_texture_image *img = t->Image[0][i];
               if (!img) {
                  incomplete(t, "2D Image[%d of %d] is missing", i, maxLevel);
                  return;
               }
               if (img->Width2 != width) {
                  incomplete(t, "2D Image[%d] bad width %u", i, img->Width2);
                  return;
               }
               if (img->Height2 != height) {
                  incomplete(t, "2D Image[i] bad height %u", i, img->Height2);
                  return;
               }
               if (width==1 && height==1) {
                  return;  /* found smallest needed mipmap, all done! */
               }
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
		     incomplete(t, "CubeMap Image[n][i] == NULL");
		     return;
		  }
		  /* Don't support GL_DEPTH_COMPONENT for cube maps */
                  if (ctx->VersionMajor < 3) {
                     if (t->Image[face][i]->_BaseFormat == GL_DEPTH_COMPONENT) {
                        incomplete(t, "GL_DEPTH_COMPONENT only works with 1/2D tex");
                        return;
                     }
		  }
		  /* check that all six images have same size */
                  if (t->Image[face][i]->Width2 != width || 
                      t->Image[face][i]->Height2 != height) {
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
      else {
         /* Target = ??? */
         _mesa_problem(ctx, "Bug in gl_test_texture_object_completeness\n");
      }
   }
}


/**
 * Check if the given cube map texture is "cube complete" as defined in
 * the OpenGL specification.
 */
GLboolean
_mesa_cube_complete(const struct gl_texture_object *texObj)
{
   const GLint baseLevel = texObj->BaseLevel;
   const struct gl_texture_image *img0, *img;
   GLuint face;

   if (texObj->Target != GL_TEXTURE_CUBE_MAP)
      return GL_FALSE;

   if ((baseLevel < 0) || (baseLevel >= MAX_TEXTURE_LEVELS))
      return GL_FALSE;

   /* check first face */
   img0 = texObj->Image[0][baseLevel];
   if (!img0 ||
       img0->Width < 1 ||
       img0->Width != img0->Height)
      return GL_FALSE;

   /* check remaining faces vs. first face */
   for (face = 1; face < 6; face++) {
      img = texObj->Image[face][baseLevel];
      if (!img ||
          img->Width != img0->Width ||
          img->Height != img0->Height ||
          img->TexFormat != img0->TexFormat)
         return GL_FALSE;
   }

   return GL_TRUE;
}


/**
 * Mark a texture object dirty.  It forces the object to be incomplete
 * and optionally forces the context to re-validate its state.
 *
 * \param ctx GL context.
 * \param texObj texture object.
 * \param invalidate_state also invalidate context state.
 */
void
_mesa_dirty_texobj(struct gl_context *ctx, struct gl_texture_object *texObj,
                   GLboolean invalidate_state)
{
   texObj->_Complete = GL_FALSE;
   if (invalidate_state)
      ctx->NewState |= _NEW_TEXTURE;
}


/**
 * Return pointer to a default/fallback texture.
 * The texture is a 2D 8x8 RGBA texture with all texels = (0,0,0,1).
 * That's the value a sampler should get when sampling from an
 * incomplete texture.
 */
struct gl_texture_object *
_mesa_get_fallback_texture(struct gl_context *ctx)
{
   if (!ctx->Shared->FallbackTex) {
      /* create fallback texture now */
      static GLubyte texels[8 * 8][4];
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;
      gl_format texFormat;
      GLuint i;

      for (i = 0; i < 8 * 8; i++) {
         texels[i][0] =
         texels[i][1] =
         texels[i][2] = 0x0;
         texels[i][3] = 0xff;
      }

      /* create texture object */
      texObj = ctx->Driver.NewTextureObject(ctx, 0, GL_TEXTURE_2D);
      assert(texObj->RefCount == 1);
      texObj->Sampler.MinFilter = GL_NEAREST;
      texObj->Sampler.MagFilter = GL_NEAREST;

      /* create level[0] texture image */
      texImage = _mesa_get_tex_image(ctx, texObj, GL_TEXTURE_2D, 0);

      texFormat = ctx->Driver.ChooseTextureFormat(ctx, GL_RGBA, GL_RGBA,
                                                  GL_UNSIGNED_BYTE);

      /* init the image fields */
      _mesa_init_teximage_fields(ctx, texImage,
                                 8, 8, 1, 0, GL_RGBA, texFormat); 

      ASSERT(texImage->TexFormat != MESA_FORMAT_NONE);

      /* set image data */
      ctx->Driver.TexImage2D(ctx, texImage, GL_RGBA,
                             8, 8, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, texels,
                             &ctx->DefaultPacking);

      _mesa_test_texobj_completeness(ctx, texObj);
      assert(texObj->_Complete);

      ctx->Shared->FallbackTex = texObj;
   }
   return ctx->Shared->FallbackTex;
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
      texObj = ctx->Driver.NewTextureObject(ctx, name, target);
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
 * Check if the given texture object is bound to any texture image units and
 * unbind it if so (revert to default textures).
 */
static void
unbind_texobj_from_texunits(struct gl_context *ctx,
                            struct gl_texture_object *texObj)
{
   GLuint tex;
   struct gl_texture_unit *unit = &ctx->Texture.Unit;
   for (tex = 0; tex < NUM_TEXTURE_TARGETS; tex++) {
      if (texObj == unit->CurrentTex[tex]) {
         _mesa_reference_texobj(&unit->CurrentTex[tex],
                                ctx->Shared->DefaultTex[tex]);
         ASSERT(unit->CurrentTex[tex]);
         break;
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
 * Convert a GL texture target enum such as GL_TEXTURE_2D or GL_TEXTURE_3D
 * into the corresponding Mesa texture target index.
 * Note that proxy targets are not valid here.
 * \return TEXTURE_x_INDEX or -1 if target is invalid
 */
static GLint
target_enum_to_index(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
      return TEXTURE_1D_INDEX;
   case GL_TEXTURE_2D:
      return TEXTURE_2D_INDEX;
   case GL_TEXTURE_CUBE_MAP_ARB:
      return TEXTURE_CUBE_INDEX;
   default:
      return -1;
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
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit;
   struct gl_texture_object *newTexObj = NULL;
   GLint targetIndex;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glBindTexture %s %d\n",
                  _mesa_lookup_enum_by_nr(target), (GLint) texName);

   targetIndex = target_enum_to_index(target);
   if (targetIndex < 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindTexture(target)");
      return;
   }
   assert(targetIndex < NUM_TEXTURE_TARGETS);

   /*
    * Get pointer to new texture object (newTexObj)
    */
   if (texName == 0) {
      /* Use a default texture object */
      newTexObj = ctx->Shared->DefaultTex[targetIndex];
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
      }
      else {
         /* if this is a new texture id, allocate a texture object now */
         newTexObj = ctx->Driver.NewTextureObject(ctx, texName, target);
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

   /* Check if this texture is only used by this context and is already bound.
    * If so, just return.
    */
   {
      GLboolean early_out;
      _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
      early_out = ((ctx->Shared->RefCount == 1)
                   && (newTexObj == texUnit->CurrentTex[targetIndex]));
      _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
      if (early_out) {
         return;
      }
   }

   /* flush before changing binding */
   FLUSH_VERTICES(ctx, _NEW_TEXTURE);

   /* Do the actual binding.  The refcount on the previously bound
    * texture object will be decremented.  It'll be deleted if the
    * count hits zero.
    */
   _mesa_reference_texobj(&texUnit->CurrentTex[targetIndex], newTexObj);
   ASSERT(texUnit->CurrentTex[targetIndex]);

   /* Pass BindTexture call to device driver */
   if (ctx->Driver.BindTexture)
      ctx->Driver.BindTexture(ctx, target, newTexObj);
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
 * Note: we assume all textures are always resident
 */
GLboolean GLAPIENTRY
_mesa_AreTexturesResident(GLsizei n, const GLuint *texName,
                          GLboolean *residences)
{
   GET_CURRENT_CONTEXT(ctx);
   GLboolean allResident = GL_TRUE;
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glAreTexturesResident(n)");
      return GL_FALSE;
   }

   if (!texName || !residences)
      return GL_FALSE;

   /* We only do error checking on the texture names */
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
 * Simplest implementation of texture locking: grab the shared tex
 * mutex.  Examine the shared context state timestamp and if there has
 * been a change, set the appropriate bits in ctx->NewState.
 *
 * This is used to deal with synchronizing things when a texture object
 * is used/modified by different contexts (or threads) which are sharing
 * the texture.
 *
 * See also _mesa_lock/unlock_texture() in teximage.h
 */
void
_mesa_lock_context_textures( struct gl_context *ctx )
{
   _glthread_LOCK_MUTEX(ctx->Shared->TexMutex);

   if (ctx->Shared->TextureStateStamp != ctx->TextureStateTimestamp) {
      ctx->NewState |= _NEW_TEXTURE;
      ctx->TextureStateTimestamp = ctx->Shared->TextureStateStamp;
   }
}


void
_mesa_unlock_context_textures( struct gl_context *ctx )
{
   assert(ctx->Shared->TextureStateStamp == ctx->TextureStateTimestamp);
   _glthread_UNLOCK_MUTEX(ctx->Shared->TexMutex);
}

/*@}*/
