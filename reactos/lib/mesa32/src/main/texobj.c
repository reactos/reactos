/**
 * \file texobj.c
 * Texture object management.
 */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
#include "colortab.h"
#include "context.h"
#include "enums.h"
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
 *
 * Allocate and initialize a gl_texture_object structure, and insert in the
 * shared state texture list while holding its mutex.
 * If <tt>name > 0</tt> then also insert the new texture object into the hash
 * table.
 * 
 */
struct gl_texture_object *
_mesa_new_texture_object( GLcontext *ctx, GLuint name, GLenum target )
{
   struct gl_texture_object *obj;
   obj = CALLOC_STRUCT(gl_texture_object);
   _mesa_initialize_texture_object(obj, name, target);
   return obj;
}


/**
 * Initialize a texture object to default values.
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
          target == GL_TEXTURE_RECTANGLE_NV);

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
   _mesa_init_colortable(&obj->Palette);
}


/**
 * Deallocate a texture object struct.  It should have already been
 * removed from the texture object pool.
 *
 * \param shared the shared GL state to which the object belongs.
 * \param texOjb the texture object to delete.
 *
 * Unlink the texture object from the shared state texture linked list while
 * holding its lock. If the texture is a name number it's also removed from the
 * hash table. Finally frees the texture images and the object itself.
 */
void
_mesa_delete_texture_object( GLcontext *ctx, struct gl_texture_object *texObj )
{
   GLuint i;

   (void) ctx;

   assert(texObj);

   _mesa_free_colortable_data(&texObj->Palette);

   /* free the texture images */
   for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
      if (texObj->Image[i]) {
         _mesa_delete_texture_image( texObj->Image[i] );
      }
   }

   /* destroy the mutex -- it may have allocated memory (eg on bsd) */
   _glthread_DESTROY_MUTEX(texObj->Mutex);

   /* free this object */
   _mesa_free(texObj);
}


/**
 * Add the given texture object to the texture object pool.
 */
void
_mesa_save_texture_object( GLcontext *ctx, struct gl_texture_object *texObj )
{
   /* insert into linked list */
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   texObj->Next = ctx->Shared->TexObjectList;
   ctx->Shared->TexObjectList = texObj;
   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

   if (texObj->Name > 0) {
      /* insert into hash table */
      _mesa_HashInsert(ctx->Shared->TexObjects, texObj->Name, texObj);
   }
}


/**
 * Remove the given texture object from the texture object pool.
 * Do not deallocate the texture object though.
 */
void
_mesa_remove_texture_object( GLcontext *ctx, struct gl_texture_object *texObj )
{
   struct gl_texture_object *tprev, *tcurr;

   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   /* unlink from the linked list */
   tprev = NULL;
   tcurr = ctx->Shared->TexObjectList;
   while (tcurr) {
      if (tcurr == texObj) {
         if (tprev) {
            tprev->Next = texObj->Next;
         }
         else {
            ctx->Shared->TexObjectList = texObj->Next;
         }
         break;
      }
      tprev = tcurr;
      tcurr = tcurr->Next;
   }

   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

   if (texObj->Name > 0) {
      /* remove from hash table */
      _mesa_HashRemove(ctx->Shared->TexObjects, texObj->Name);
   }
}

/**
 * Copy texture object state from one texture object to another.
 *
 * \param dest destination texture object.
 * \param src source texture object.
 */
void
_mesa_copy_texture_object( struct gl_texture_object *dest,
                           const struct gl_texture_object *src )
{
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
   dest->Complete = src->Complete;
   dest->_IsPowerOfTwo = src->_IsPowerOfTwo;
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

   t->Complete = GL_TRUE;  /* be optimistic */
   t->_IsPowerOfTwo = GL_TRUE;  /* may be set FALSE below */

   /* Always need the base level image */
   if (!t->Image[baseLevel]) {
      char s[100];
      sprintf(s, "obj %p (%d) Image[baseLevel=%d] == NULL",
              (void *) t, t->Name, baseLevel);
      incomplete(t, s);
      t->Complete = GL_FALSE;
      return;
   }

   /* Check width/height/depth for zero */
   if (t->Image[baseLevel]->Width == 0 ||
       t->Image[baseLevel]->Height == 0 ||
       t->Image[baseLevel]->Depth == 0) {
      incomplete(t, "texture width = 0");
      t->Complete = GL_FALSE;
      return;
   }

   /* Compute _MaxLevel */
   if (t->Target == GL_TEXTURE_1D) {
      maxLog2 = t->Image[baseLevel]->WidthLog2;
      maxLevels = ctx->Const.MaxTextureLevels;
   }
   else if (t->Target == GL_TEXTURE_2D) {
      maxLog2 = MAX2(t->Image[baseLevel]->WidthLog2,
                     t->Image[baseLevel]->HeightLog2);
      maxLevels = ctx->Const.MaxTextureLevels;
   }
   else if (t->Target == GL_TEXTURE_3D) {
      GLint max = MAX2(t->Image[baseLevel]->WidthLog2,
                       t->Image[baseLevel]->HeightLog2);
      maxLog2 = MAX2(max, (GLint)(t->Image[baseLevel]->DepthLog2));
      maxLevels = ctx->Const.Max3DTextureLevels;
   }
   else if (t->Target == GL_TEXTURE_CUBE_MAP_ARB) {
      maxLog2 = MAX2(t->Image[baseLevel]->WidthLog2,
                     t->Image[baseLevel]->HeightLog2);
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
      const GLuint w = t->Image[baseLevel]->Width2;
      const GLuint h = t->Image[baseLevel]->Height2;
      if (!t->NegX[baseLevel] ||
          t->NegX[baseLevel]->Width2 != w ||
          t->NegX[baseLevel]->Height2 != h ||
          !t->PosY[baseLevel] ||
          t->PosY[baseLevel]->Width2 != w ||
          t->PosY[baseLevel]->Height2 != h ||
          !t->NegY[baseLevel] ||
          t->NegY[baseLevel]->Width2 != w ||
          t->NegY[baseLevel]->Height2 != h ||
          !t->PosZ[baseLevel] ||
          t->PosZ[baseLevel]->Width2 != w ||
          t->PosZ[baseLevel]->Height2 != h ||
          !t->NegZ[baseLevel] ||
          t->NegZ[baseLevel]->Width2 != w ||
          t->NegZ[baseLevel]->Height2 != h) {
         t->Complete = GL_FALSE;
         incomplete(t, "Non-quare cubemap image");
         return;
      }
   }

   /* check for non power of two */
   if (!t->Image[baseLevel]->_IsPowerOfTwo) {
      t->_IsPowerOfTwo = GL_FALSE;
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
         t->Complete = GL_FALSE;
         incomplete(t, "minLevel > maxLevel");
         return;
      }

      /* Test dimension-independent attributes */
      for (i = minLevel; i <= maxLevel; i++) {
         if (t->Image[i]) {
            if (t->Image[i]->TexFormat != t->Image[baseLevel]->TexFormat) {
               t->Complete = GL_FALSE;
               incomplete(t, "Format[i] != Format[baseLevel]");
               return;
            }
            if (t->Image[i]->Border != t->Image[baseLevel]->Border) {
               t->Complete = GL_FALSE;
               incomplete(t, "Border[i] != Border[baseLevel]");
               return;
            }
         }
      }

      /* Test things which depend on number of texture image dimensions */
      if (t->Target == GL_TEXTURE_1D) {
         /* Test 1-D mipmaps */
         GLuint width = t->Image[baseLevel]->Width2;
         for (i = baseLevel + 1; i < maxLevels; i++) {
            if (width > 1) {
               width /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
               if (!t->Image[i]) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "1D Image[i] == NULL");
                  return;
               }
               if (t->Image[i]->Width2 != width ) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "1D Image[i] bad width");
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
         GLuint width = t->Image[baseLevel]->Width2;
         GLuint height = t->Image[baseLevel]->Height2;
         for (i = baseLevel + 1; i < maxLevels; i++) {
            if (width > 1) {
               width /= 2;
            }
            if (height > 1) {
               height /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
               if (!t->Image[i]) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "2D Image[i] == NULL");
                  return;
               }
               if (t->Image[i]->Width2 != width) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "2D Image[i] bad width");
                  return;
               }
               if (t->Image[i]->Height2 != height) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "2D Image[i] bad height");
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
         GLuint width = t->Image[baseLevel]->Width2;
         GLuint height = t->Image[baseLevel]->Height2;
         GLuint depth = t->Image[baseLevel]->Depth2;
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
               if (!t->Image[i]) {
                  incomplete(t, "3D Image[i] == NULL");
                  t->Complete = GL_FALSE;
                  return;
               }
               if (t->Image[i]->Format == GL_DEPTH_COMPONENT) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "GL_DEPTH_COMPONENT only works with 1/2D tex");
                  return;
               }
               if (t->Image[i]->Width2 != width) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "3D Image[i] bad width");
                  return;
               }
               if (t->Image[i]->Height2 != height) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "3D Image[i] bad height");
                  return;
               }
               if (t->Image[i]->Depth2 != depth) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "3D Image[i] bad depth");
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
         GLuint width = t->Image[baseLevel]->Width2;
         GLuint height = t->Image[baseLevel]->Height2;
	 for (i = baseLevel + 1; i < maxLevels; i++) {
            if (width > 1) {
               width /= 2;
            }
            if (height > 1) {
               height /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
               /* check that we have images defined */
               if (!t->Image[i] || !t->NegX[i] ||
                   !t->PosY[i]  || !t->NegY[i] ||
                   !t->PosZ[i]  || !t->NegZ[i]) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "CubeMap Image[i] == NULL");
                  return;
               }
               /* Don't support GL_DEPTH_COMPONENT for cube maps */
               if (t->Image[i]->Format == GL_DEPTH_COMPONENT) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "GL_DEPTH_COMPONENT only works with 1/2D tex");
                  return;
               }
               /* check that all six images have same size */
               if (t->NegX[i]->Width2!=width || t->NegX[i]->Height2!=height ||
                   t->PosY[i]->Width2!=width || t->PosY[i]->Height2!=height ||
                   t->NegY[i]->Width2!=width || t->NegY[i]->Height2!=height ||
                   t->PosZ[i]->Width2!=width || t->PosZ[i]->Height2!=height ||
                   t->NegZ[i]->Width2!=width || t->NegZ[i]->Height2!=height) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "CubeMap Image[i] bad size");
                  return;
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
 * Texture name generation lock.
 *
 * Used by _mesa_GenTextures() to guarantee that the generation and allocation
 * of texture IDs is atomic.
 */
_glthread_DECLARE_STATIC_MUTEX(GenTexturesLock);

/**
 * Generate texture names.
 *
 * \param n number of texture names to be generated.
 * \param texName an array in which will hold the generated texture names.
 *
 * \sa glGenTextures().
 *
 * While holding the GenTexturesLock lock, calls _mesa_HashFindFreeKeyBlock()
 * to find a block of free texture IDs which are stored in \p texName.
 * Corresponding empty texture objects are also generated.
 */ 
void GLAPIENTRY
_mesa_GenTextures( GLsizei n, GLuint *texName )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glGenTextures" );
      return;
   }

   if (!texName)
      return;

   /*
    * This must be atomic (generation and allocation of texture IDs)
    */
   _glthread_LOCK_MUTEX(GenTexturesLock);

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->TexObjects, n);

   /* Allocate new, empty texture objects */
   for (i = 0; i < n; i++) {
      struct gl_texture_object *texObj;
      GLuint name = first + i;
      GLenum target = 0;
      texObj = (*ctx->Driver.NewTextureObject)( ctx, name, target);
      if (!texObj) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenTextures");
         return;
      }
      _mesa_save_texture_object(ctx, texObj);
      texName[i] = name;
   }

   _glthread_UNLOCK_MUTEX(GenTexturesLock);
}

/**
 * Delete named textures.
 *
 * \param n number of textures to be deleted.
 * \param texName array of textures names to be deleted.
 *
 * \sa glDeleteTextures().
 *
 * For each texture checks if its bound to any of the texture units, unbinding
 * it and decrementing the reference count if so. If the texture reference
 * count is zero, delete its object.
 */
void GLAPIENTRY
_mesa_DeleteTextures( GLsizei n, const GLuint *texName)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx); /* too complex */

   if (!texName)
      return;

   for (i=0;i<n;i++) {
      if (texName[i] > 0) {
         struct gl_texture_object *delObj = (struct gl_texture_object *)
            _mesa_HashLookup(ctx->Shared->TexObjects, texName[i]);
         if (delObj) {
            /* First check if this texture is currently bound.
             * If so, unbind it and decrement the reference count.
             */
            GLuint u;
            for (u = 0; u < MAX_TEXTURE_IMAGE_UNITS; u++) {
               struct gl_texture_unit *unit = &ctx->Texture.Unit[u];
               if (delObj == unit->Current1D) {
                  unit->Current1D = ctx->Shared->Default1D;
                  ctx->Shared->Default1D->RefCount++;
                  delObj->RefCount--;
                  if (delObj == unit->_Current)
                     unit->_Current = unit->Current1D;
               }
               else if (delObj == unit->Current2D) {
                  unit->Current2D = ctx->Shared->Default2D;
                  ctx->Shared->Default2D->RefCount++;
                  delObj->RefCount--;
                  if (delObj == unit->_Current)
                     unit->_Current = unit->Current2D;
               }
               else if (delObj == unit->Current3D) {
                  unit->Current3D = ctx->Shared->Default3D;
                  ctx->Shared->Default3D->RefCount++;
                  delObj->RefCount--;
                  if (delObj == unit->_Current)
                     unit->_Current = unit->Current3D;
               }
               else if (delObj == unit->CurrentCubeMap) {
                  unit->CurrentCubeMap = ctx->Shared->DefaultCubeMap;
                  ctx->Shared->DefaultCubeMap->RefCount++;
                  delObj->RefCount--;
                  if (delObj == unit->_Current)
                     unit->_Current = unit->CurrentCubeMap;
               }
               else if (delObj == unit->CurrentRect) {
                  unit->CurrentRect = ctx->Shared->DefaultRect;
                  ctx->Shared->DefaultRect->RefCount++;
                  delObj->RefCount--;
                  if (delObj == unit->_Current)
                     unit->_Current = unit->CurrentRect;
               }
            }
            ctx->NewState |= _NEW_TEXTURE;

            /* Decrement reference count and delete if zero */
            delObj->RefCount--;
            ASSERT(delObj->RefCount >= 0);

            if (delObj->RefCount == 0) {
               ASSERT(delObj->Name != 0);
               _mesa_remove_texture_object(ctx, delObj);
               ASSERT(ctx->Driver.DeleteTexture);
               (*ctx->Driver.DeleteTexture)(ctx, delObj);
            }
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
   GLuint unit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *oldTexObj;
   struct gl_texture_object *newTexObj = 0;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glBindTexture %s %d\n",
                  _mesa_lookup_enum_by_nr(target), (GLint) texName);

   switch (target) {
      case GL_TEXTURE_1D:
         oldTexObj = texUnit->Current1D;
         break;
      case GL_TEXTURE_2D:
         oldTexObj = texUnit->Current2D;
         break;
      case GL_TEXTURE_3D:
         oldTexObj = texUnit->Current3D;
         break;
      case GL_TEXTURE_CUBE_MAP_ARB:
         if (!ctx->Extensions.ARB_texture_cube_map) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glBindTexture(target)" );
            return;
         }
         oldTexObj = texUnit->CurrentCubeMap;
         break;
      case GL_TEXTURE_RECTANGLE_NV:
         if (!ctx->Extensions.NV_texture_rectangle) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glBindTexture(target)" );
            return;
         }
         oldTexObj = texUnit->CurrentRect;
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glBindTexture(target)" );
         return;
   }

   if (oldTexObj->Name == texName)
      return;   /* rebinding the same texture- no change */

   /*
    * Get pointer to new texture object (newTexObj)
    */
   if (texName == 0) {
      /* newTexObj = a default texture object */
      switch (target) {
         case GL_TEXTURE_1D:
            newTexObj = ctx->Shared->Default1D;
            break;
         case GL_TEXTURE_2D:
            newTexObj = ctx->Shared->Default2D;
            break;
         case GL_TEXTURE_3D:
            newTexObj = ctx->Shared->Default3D;
            break;
         case GL_TEXTURE_CUBE_MAP_ARB:
            newTexObj = ctx->Shared->DefaultCubeMap;
            break;
         case GL_TEXTURE_RECTANGLE_NV:
            newTexObj = ctx->Shared->DefaultRect;
            break;
         default:
            ; /* Bad targets are caught above */
      }
   }
   else {
      /* non-default texture object */
      const struct _mesa_HashTable *hash = ctx->Shared->TexObjects;
      newTexObj = (struct gl_texture_object *) _mesa_HashLookup(hash, texName);
      if (newTexObj) {
         /* error checking */
         if (newTexObj->Target != 0 && newTexObj->Target != target) {
            /* the named texture object's dimensions don't match the target */
            _mesa_error( ctx, GL_INVALID_OPERATION,
                         "glBindTexture(wrong dimensionality)" );
            return;
         }
         if (newTexObj->Target == 0 && target == GL_TEXTURE_RECTANGLE_NV) {
            /* have to init wrap and filter state here - kind of klunky */
            newTexObj->WrapS = GL_CLAMP_TO_EDGE;
            newTexObj->WrapT = GL_CLAMP_TO_EDGE;
            newTexObj->WrapR = GL_CLAMP_TO_EDGE;
            newTexObj->MinFilter = GL_LINEAR;
         }
      }
      else {
         /* if this is a new texture id, allocate a texture object now */
	 newTexObj = (*ctx->Driver.NewTextureObject)(ctx, texName, target);
         if (!newTexObj) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindTexture");
            return;
         }
         _mesa_save_texture_object(ctx, newTexObj);
      }
      newTexObj->Target = target;
   }

   newTexObj->RefCount++;

   /* do the actual binding, but first flush outstanding vertices:
    */
   FLUSH_VERTICES(ctx, _NEW_TEXTURE);

   switch (target) {
      case GL_TEXTURE_1D:
         texUnit->Current1D = newTexObj;
         break;
      case GL_TEXTURE_2D:
         texUnit->Current2D = newTexObj;
         break;
      case GL_TEXTURE_3D:
         texUnit->Current3D = newTexObj;
         break;
      case GL_TEXTURE_CUBE_MAP_ARB:
         texUnit->CurrentCubeMap = newTexObj;
         break;
      case GL_TEXTURE_RECTANGLE_NV:
         texUnit->CurrentRect = newTexObj;
         break;
      default:
         _mesa_problem(ctx, "bad target in BindTexture");
         return;
   }

   /* Pass BindTexture call to device driver */
   if (ctx->Driver.BindTexture)
      (*ctx->Driver.BindTexture)( ctx, target, newTexObj );

   oldTexObj->RefCount--;
   assert(oldTexObj->RefCount >= 0);
   if (oldTexObj->RefCount == 0) {
      assert(oldTexObj->Name != 0);
      _mesa_remove_texture_object(ctx, oldTexObj);
      ASSERT(ctx->Driver.DeleteTexture);
      (*ctx->Driver.DeleteTexture)( ctx, oldTexObj );
   }
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
         struct gl_texture_object *t = (struct gl_texture_object *)
            _mesa_HashLookup(ctx->Shared->TexObjects, texName[i]);
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
      t = (struct gl_texture_object *)
         _mesa_HashLookup(ctx->Shared->TexObjects, texName[i]);
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
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);
   return texture > 0 && _mesa_HashLookup(ctx->Shared->TexObjects, texture);
}

/*@}*/
