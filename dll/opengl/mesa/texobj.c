/* $Id: texobj.c,v 1.21 1998/01/16 01:09:44 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.6
 * Copyright (C) 1995-1997  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: texobj.c,v $
 * Revision 1.21  1998/01/16 01:09:44  brianp
 * glGenTextures() didn't reserve the returned texture IDs
 *
 * Revision 1.20  1997/12/07 17:34:05  brianp
 * added DavidB's v0.21 fxmesa driver patch
 *
 * Revision 1.19  1997/11/07 03:38:07  brianp
 * added stdio.h include for SunOS 4.x
 *
 * Revision 1.18  1997/10/13 23:57:59  brianp
 * added target parameter to Driver.BindTexture()
 *
 * Revision 1.17  1997/09/29 23:28:14  brianp
 * updated for new device driver texture functions
 *
 * Revision 1.16  1997/09/27 00:14:39  brianp
 * added GL_EXT_paletted_texture extension
 *
 * Revision 1.15  1997/09/23 00:58:15  brianp
 * now using hash table for texture objects
 *
 * Revision 1.14  1997/08/23 18:41:40  brianp
 * fixed bug in glBindTexture() when binding an incomplete texture image
 *
 * Revision 1.13  1997/08/23 17:14:44  brianp
 * fixed bug:  glBindTexture(target, 0) caused segfault
 *
 * Revision 1.12  1997/07/24 01:25:34  brianp
 * changed precompiled header symbol from PCH to PC_HEADER
 *
 * Revision 1.11  1997/05/28 03:26:49  brianp
 * added precompiled header (PCH) support
 *
 * Revision 1.10  1997/05/17 03:41:49  brianp
 * added code to update ctx->Texture.Current in gl_BindTexture()
 *
 * Revision 1.9  1997/05/03 00:52:52  brianp
 * removed a few unused variables
 *
 * Revision 1.8  1997/05/01 02:08:12  brianp
 * new implementation of gl_BindTexture()
 *
 * Revision 1.7  1997/04/28 23:38:45  brianp
 * added gl_test_texture_object_completeness()
 *
 * Revision 1.6  1997/04/14 02:03:05  brianp
 * added MinMagThresh to texture object
 *
 * Revision 1.5  1997/02/09 18:52:15  brianp
 * added GL_EXT_texture3D support
 *
 * Revision 1.4  1997/01/16 03:35:34  brianp
 * added calls to device driver DeleteTexture() and BindTexture() functions
 *
 * Revision 1.3  1997/01/09 19:49:47  brianp
 * added a check to switch rasterizers if needed in glBindTexture()
 *
 * Revision 1.2  1996/09/27 17:09:42  brianp
 * removed a redundant return statement
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "context.h"
#include "hash.h"
#include "macros.h"
#include "teximage.h"
#include "texobj.h"
#include "types.h"
#endif



/*
 * Allocate a new texture object and add it to the linked list of texture
 * objects.  If name>0 then also insert the new texture object into the hash
 * table.
 * Input:  shared - the shared GL state structure to contain the texture object
 *         name - integer name for the texture object
 *         dimensions - either 1, 2 or 3
 * Return:  pointer to new texture object
 */
struct gl_texture_object *
gl_alloc_texture_object( struct gl_shared_state *shared, GLuint name,
                         GLuint dimensions)
{
   struct gl_texture_object *obj;

   assert(dimensions >= 0 && dimensions <= 2);

   obj = (struct gl_texture_object *)
                     calloc(1,sizeof(struct gl_texture_object));
   if (obj) {
      /* init the non-zero fields */
      obj->Name = name;
      obj->Dimensions = dimensions;
      obj->WrapS = GL_REPEAT;
      obj->WrapT = GL_REPEAT;
      obj->MinFilter = GL_NEAREST_MIPMAP_LINEAR;
      obj->MagFilter = GL_LINEAR;
      obj->MinMagThresh = 0.0F;
      obj->Palette[0] = 255;
      obj->Palette[1] = 255;
      obj->Palette[2] = 255;
      obj->Palette[3] = 255;
      obj->PaletteSize = 1;
      obj->PaletteIntFormat = GL_RGBA;
      obj->PaletteFormat = GL_RGBA;

      /* insert into linked list */
      if (shared) {
         obj->Next = shared->TexObjectList;
         shared->TexObjectList = obj;
      }

      if (name > 0) {
         /* insert into hash table */
         HashInsert(shared->TexObjects, name, obj);
      }
   }
   return obj;
}


/*
 * Deallocate a texture object struct and remove it from the given
 * shared GL state.
 * Input:  shared - the shared GL state to which the object belongs
 *         t - the texture object to delete
 */
void gl_free_texture_object( struct gl_shared_state *shared,
                             struct gl_texture_object *t )
{
   struct gl_texture_object *tprev, *tcurr;

   assert(t);

   /* unlink t from the linked list */
   if (shared) {
      tprev = NULL;
      tcurr = shared->TexObjectList;
      while (tcurr) {
         if (tcurr==t) {
            if (tprev) {
               tprev->Next = t->Next;
            }
            else {
               shared->TexObjectList = t->Next;
            }
            break;
         }
         tprev = tcurr;
         tcurr = tcurr->Next;
      }
   }

   if (t->Name) {
      /* remove from hash table */
      HashRemove(shared->TexObjects, t->Name);
   }

   /* free texture image */
   {
      GLuint i;
      for (i=0;i<MAX_TEXTURE_LEVELS;i++) {
         if (t->Image[i]) {
            gl_free_texture_image( t->Image[i] );
         }
      }
   }
   /* free this object */
   free( t );
}



/*
 * Examine a texture object to determine if it is complete or not.
 * The t->Complete flag will be set to GL_TRUE or GL_FALSE accordingly.
 */
void gl_test_texture_object_completeness( struct gl_texture_object *t )
{
   t->Complete = GL_TRUE;  /* be optimistic */

   /* Always need level zero image */
   if (!t->Image[0] || !t->Image[0]->Data) {
      t->Complete = GL_FALSE;
      return;
   }

   if (t->MinFilter!=GL_NEAREST && t->MinFilter!=GL_LINEAR) {
      /*
       * Mipmapping: determine if we have a complete set of mipmaps
       */
      int i;

      /* Test dimension-independent attributes */
      for (i=1; i<MAX_TEXTURE_LEVELS; i++) {
         if (t->Image[i]) {
            if (!t->Image[i]->Data) {
               t->Complete = GL_FALSE;
               return;
            }
            if (t->Image[i]->Format != t->Image[0]->Format) {
               t->Complete = GL_FALSE;
               return;
            }
            if (t->Image[i]->Border != t->Image[0]->Border) {
               t->Complete = GL_FALSE;
               return;
            }
         }
      }

      /* Test things which depend on number of texture image dimensions */
      if (t->Dimensions==1) {
         /* Test 1-D mipmaps */
         GLuint width = t->Image[0]->Width2;
         for (i=1; i<MAX_TEXTURE_LEVELS; i++) {
            if (width>1) {
               width /= 2;
            }
            if (!t->Image[i]) {
               t->Complete = GL_FALSE;
               return;
            }
            if (!t->Image[i]->Data) {
               t->Complete = GL_FALSE;
               return;
            }
            if (t->Image[i]->Format != t->Image[0]->Format) {
               t->Complete = GL_FALSE;
               return;
            }
            if (t->Image[i]->Border != t->Image[0]->Border) {
               t->Complete = GL_FALSE;
               return;
            }
            if (t->Image[i]->Width2 != width ) {
               t->Complete = GL_FALSE;
               return;
            }
            if (width==1) {
               return;  /* found smallest needed mipmap, all done! */
            }
         }
      }
      else if (t->Dimensions==2) {
         /* Test 2-D mipmaps */
         GLuint width = t->Image[0]->Width2;
         GLuint height = t->Image[0]->Height2;
         for (i=1; i<MAX_TEXTURE_LEVELS; i++) {
            if (width>1) {
               width /= 2;
            }
            if (height>1) {
               height /= 2;
            }
            if (!t->Image[i]) {
               t->Complete = GL_FALSE;
               return;
            }
            if (t->Image[i]->Width2 != width) {
               t->Complete = GL_FALSE;
               return;
            }
            if (t->Image[i]->Height2 != height) {
               t->Complete = GL_FALSE;
               return;
            }
            if (width==1 && height==1) {
               return;  /* found smallest needed mipmap, all done! */
            }
         }
      }
      else {
         /* Dimensions = ??? */
         gl_problem(NULL, "Bug in gl_test_texture_object_completeness\n");
      }
   }
}



/*
 * Execute glGenTextures
 */
void gl_GenTextures( GLcontext *ctx, GLsizei n, GLuint *texName )
{
   GLuint first, i;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGenTextures" );
      return;
   }
   if (n<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glGenTextures" );
      return;
   }

   first = HashFindFreeKeyBlock(ctx->Shared->TexObjects, n);

   /* Return the texture names */
   for (i=0;i<n;i++) {
      texName[i] = first + i;
   }

   /* Allocate new, empty texture objects */
   for (i=0;i<n;i++) {
      GLuint name = first + i;
      GLuint dims = 0;
      struct gl_texture_object *newTexObj = gl_alloc_texture_object(ctx->Shared, name, dims);
      (void)newTexObj;
   }
}



/*
 * Execute glDeleteTextures
 */
void gl_DeleteTextures( GLcontext *ctx, GLsizei n, const GLuint *texName)
{
   GLuint i;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glAreTexturesResident" );
      return;
   }

   for (i=0;i<n;i++) {
      struct gl_texture_object *t;
      if (texName[i]>0) {
         t = (struct gl_texture_object *)
            HashLookup(ctx->Shared->TexObjects, texName[i]);
         if (t) {
            if (ctx->Texture.Current1D==t) {
               /* revert to default 1-D texture */
               ctx->Texture.Current1D = ctx->Shared->Default1D;
               t->RefCount--;
               assert( t->RefCount >= 0 );
            }
            else if (ctx->Texture.Current2D==t) {
               /* revert to default 2-D texture */
               ctx->Texture.Current2D = ctx->Shared->Default2D;
               t->RefCount--;
               assert( t->RefCount >= 0 );
            }

            /* tell device driver to delete texture */
            if (ctx->Driver.DeleteTexture) {
               (*ctx->Driver.DeleteTexture)( ctx, t );
            }

            if (t->RefCount==0) {
               gl_free_texture_object(ctx->Shared, t);
            }
         }
      }
   }
}



/*
 * Execute glBindTexture
 */
void gl_BindTexture( GLcontext *ctx, GLenum target, GLuint texName )
{
   struct gl_texture_object *oldTexObj;
   struct gl_texture_object *newTexObj;
   struct gl_texture_object **targetPointer;
   GLuint targetDimensions;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glAreTexturesResident" );
      return;
   }
   switch (target) {
      case GL_TEXTURE_1D:
         oldTexObj = ctx->Texture.Current1D;
         targetPointer = &ctx->Texture.Current1D;
         targetDimensions = 1;
         break;
      case GL_TEXTURE_2D:
         oldTexObj = ctx->Texture.Current2D;
         targetPointer = &ctx->Texture.Current2D;
         targetDimensions = 2;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glBindTexture" );
         return;
   }

   if (texName==0) {
      /* use default n-D texture */
      switch (target) {
         case GL_TEXTURE_1D:
            newTexObj = ctx->Shared->Default1D;
            break;
         case GL_TEXTURE_2D:
            newTexObj = ctx->Shared->Default2D;
            break;
         default:
            gl_problem(ctx, "Bad target in gl_BindTexture");
            return;
      }
   }
   else {
      newTexObj = (struct gl_texture_object *)
                             HashLookup(ctx->Shared->TexObjects, texName);
      if (newTexObj) {
         if (newTexObj->Dimensions == 0) {
            /* first time bound */
            newTexObj->Dimensions = targetDimensions;
         }
         else if (newTexObj->Dimensions != targetDimensions) {
            /* wrong dimensionality */
            gl_error( ctx, GL_INVALID_OPERATION, "glBindTextureEXT" );
            return;
         }
      }
      else {
         /* create new texture object */
         newTexObj = gl_alloc_texture_object(ctx->Shared, texName,
                                             targetDimensions);
      }
   }

   /* Update the Texture.Current[123]D pointer */
   *targetPointer = newTexObj;

   /* Tidy up reference counting */
   if (*targetPointer != oldTexObj && oldTexObj->Name>0) {
      /* decrement reference count of the prev texture object */
      oldTexObj->RefCount--;
      assert( oldTexObj->RefCount >= 0 );
   }

   if (newTexObj->Name>0) {
      newTexObj->RefCount++;
   }

   /* Check if we may have to use a new triangle rasterizer */
   if (   oldTexObj->WrapS != newTexObj->WrapS
       || oldTexObj->WrapT != newTexObj->WrapT
       || oldTexObj->WrapR != newTexObj->WrapR
       || oldTexObj->MinFilter != newTexObj->MinFilter
       || oldTexObj->MagFilter != newTexObj->MagFilter
       || (oldTexObj->Image[0] && newTexObj->Image[0] && 
	  (oldTexObj->Image[0]->Format!=newTexObj->Image[0]->Format))
       || !newTexObj->Complete) {
      ctx->NewState |= NEW_RASTER_OPS;
   }

   /* If we've changed the Current[123]D texture object then update the
    * ctx->Texture.Current pointer to point to the new texture object.
    */
   if (oldTexObj==ctx->Texture.Current) {
      ctx->Texture.Current = newTexObj;
   }

   /* The current n-D texture object can never be NULL! */
   assert(*targetPointer);
 
   /* Pass BindTexture call to device driver */
   if (ctx->Driver.BindTexture) {
      (*ctx->Driver.BindTexture)( ctx, target, newTexObj );
   }
}



/*
 * Execute glPrioritizeTextures
 */
void gl_PrioritizeTextures( GLcontext *ctx,
                            GLsizei n, const GLuint *texName,
                            const GLclampf *priorities )
{
   GLuint i;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glAreTexturesResident" );
      return;
   }
   if (n<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glAreTexturesResident(n)" );
      return;
   }

   for (i=0;i<n;i++) {
      struct gl_texture_object *t;
      if (texName[i]>0) {
         t = (struct gl_texture_object *)
            HashLookup(ctx->Shared->TexObjects, texName[i]);
         if (t) {
            t->Priority = CLAMP( priorities[i], 0.0F, 1.0F );
         }
      }
   }
}



/*
 * Execute glAreTexturesResident
 */
GLboolean gl_AreTexturesResident( GLcontext *ctx, GLsizei n,
                                  const GLuint *texName,
                                  GLboolean *residences )
{
   GLboolean resident = GL_TRUE;
   GLuint i;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glAreTexturesResident" );
      return GL_FALSE;
   }
   if (n<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glAreTexturesResident(n)" );
      return GL_FALSE;
   }

   for (i=0;i<n;i++) {
      struct gl_texture_object *t;
      if (texName[i]==0) {
         gl_error( ctx, GL_INVALID_VALUE, "glAreTexturesResident(textures)" );
         return GL_FALSE;
      }
      t = (struct gl_texture_object *)
         HashLookup(ctx->Shared->TexObjects, texName[i]);
      if (t) {
         /* we consider all valid texture objects to be resident */
         residences[i] = GL_TRUE;
      }
      else {
         gl_error( ctx, GL_INVALID_VALUE, "glAreTexturesResident(textures)" );
         return GL_FALSE;
      }
   }
   return resident;
}



/*
 * Execute glIsTexture
 */
GLboolean gl_IsTexture( GLcontext *ctx, GLuint texture )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glIsTextures" );
      return GL_FALSE;
   }
   if (texture>0 && HashLookup(ctx->Shared->TexObjects, texture)) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}

