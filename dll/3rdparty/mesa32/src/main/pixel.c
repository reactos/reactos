/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
#include "bufferobj.h"
#include "colormac.h"
#include "context.h"
#include "image.h"
#include "macros.h"
#include "pixel.h"
#include "mtypes.h"


/**********************************************************************/
/*****                    glPixelZoom                             *****/
/**********************************************************************/



void GLAPIENTRY
_mesa_PixelZoom( GLfloat xfactor, GLfloat yfactor )
{
   GET_CURRENT_CONTEXT(ctx);

   if (ctx->Pixel.ZoomX == xfactor &&
       ctx->Pixel.ZoomY == yfactor)
      return;

   FLUSH_VERTICES(ctx, _NEW_PIXEL);
   ctx->Pixel.ZoomX = xfactor;
   ctx->Pixel.ZoomY = yfactor;
}



/**********************************************************************/
/*****                    glPixelStore                            *****/
/**********************************************************************/


void GLAPIENTRY
_mesa_PixelStorei( GLenum pname, GLint param )
{
   /* NOTE: this call can't be compiled into the display list */
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (pname) {
      case GL_PACK_SWAP_BYTES:
	 if (param == (GLint)ctx->Pack.SwapBytes)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
         ctx->Pack.SwapBytes = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_PACK_LSB_FIRST:
	 if (param == (GLint)ctx->Pack.LsbFirst)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
         ctx->Pack.LsbFirst = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_PACK_ROW_LENGTH:
	 if (param<0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.RowLength == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.RowLength = param;
	 break;
      case GL_PACK_IMAGE_HEIGHT:
         if (param<0) {
            _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.ImageHeight == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.ImageHeight = param;
         break;
      case GL_PACK_SKIP_PIXELS:
	 if (param<0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.SkipPixels == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.SkipPixels = param;
	 break;
      case GL_PACK_SKIP_ROWS:
	 if (param<0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.SkipRows == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.SkipRows = param;
	 break;
      case GL_PACK_SKIP_IMAGES:
	 if (param<0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.SkipImages == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.SkipImages = param;
	 break;
      case GL_PACK_ALIGNMENT:
         if (param!=1 && param!=2 && param!=4 && param!=8) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Pack.Alignment == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Pack.Alignment = param;
	 break;
      case GL_PACK_INVERT_MESA:
         if (!ctx->Extensions.MESA_pack_invert) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glPixelstore(pname)" );
            return;
         }
         if (ctx->Pack.Invert == param)
            return;
         FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
         ctx->Pack.Invert = param;
         break;

      case GL_UNPACK_SWAP_BYTES:
	 if (param == (GLint)ctx->Unpack.SwapBytes)
	    return;
	 if ((GLint)ctx->Unpack.SwapBytes == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.SwapBytes = param ? GL_TRUE : GL_FALSE;
         break;
      case GL_UNPACK_LSB_FIRST:
	 if (param == (GLint)ctx->Unpack.LsbFirst)
	    return;
	 if ((GLint)ctx->Unpack.LsbFirst == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.LsbFirst = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_UNPACK_ROW_LENGTH:
	 if (param<0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.RowLength == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.RowLength = param;
	 break;
      case GL_UNPACK_IMAGE_HEIGHT:
         if (param<0) {
            _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.ImageHeight == param)
	    return;

	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.ImageHeight = param;
         break;
      case GL_UNPACK_SKIP_PIXELS:
	 if (param<0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.SkipPixels == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.SkipPixels = param;
	 break;
      case GL_UNPACK_SKIP_ROWS:
	 if (param<0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.SkipRows == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.SkipRows = param;
	 break;
      case GL_UNPACK_SKIP_IMAGES:
	 if (param < 0) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore(param)" );
	    return;
	 }
	 if (ctx->Unpack.SkipImages == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.SkipImages = param;
	 break;
      case GL_UNPACK_ALIGNMENT:
         if (param!=1 && param!=2 && param!=4 && param!=8) {
	    _mesa_error( ctx, GL_INVALID_VALUE, "glPixelStore" );
	    return;
	 }
	 if (ctx->Unpack.Alignment == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
	 ctx->Unpack.Alignment = param;
	 break;
      case GL_UNPACK_CLIENT_STORAGE_APPLE:
         if (param == (GLint)ctx->Unpack.ClientStorage)
            return;
         FLUSH_VERTICES(ctx, _NEW_PACKUNPACK);
         ctx->Unpack.ClientStorage = param ? GL_TRUE : GL_FALSE;
         break;
      default:
	 _mesa_error( ctx, GL_INVALID_ENUM, "glPixelStore" );
	 return;
   }
}


void GLAPIENTRY
_mesa_PixelStoref( GLenum pname, GLfloat param )
{
   _mesa_PixelStorei( pname, (GLint) param );
}



/**********************************************************************/
/*****                         glPixelMap                         *****/
/**********************************************************************/


/**
 * Helper routine used by the other _mesa_PixelMap() functions.
 */
static void
pixelmap(GLcontext *ctx, GLenum map, GLsizei mapsize, const GLfloat *values)
{
   GLint i;
   switch (map) {
      case GL_PIXEL_MAP_S_TO_S:
         ctx->Pixel.MapStoSsize = mapsize;
         for (i = 0; i < mapsize; i++) {
	    ctx->Pixel.MapStoS[i] = IROUND(values[i]);
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_I:
         ctx->Pixel.MapItoIsize = mapsize;
         for (i = 0; i < mapsize; i++) {
	    ctx->Pixel.MapItoI[i] = values[i];
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_R:
         ctx->Pixel.MapItoRsize = mapsize;
         for (i = 0; i < mapsize; i++) {
            GLfloat val = CLAMP( values[i], 0.0F, 1.0F );
	    ctx->Pixel.MapItoR[i] = val;
	    ctx->Pixel.MapItoR8[i] = (GLint) (val * 255.0F);
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_G:
         ctx->Pixel.MapItoGsize = mapsize;
         for (i = 0; i < mapsize; i++) {
            GLfloat val = CLAMP( values[i], 0.0F, 1.0F );
	    ctx->Pixel.MapItoG[i] = val;
	    ctx->Pixel.MapItoG8[i] = (GLint) (val * 255.0F);
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_B:
         ctx->Pixel.MapItoBsize = mapsize;
         for (i = 0; i < mapsize; i++) {
            GLfloat val = CLAMP( values[i], 0.0F, 1.0F );
	    ctx->Pixel.MapItoB[i] = val;
	    ctx->Pixel.MapItoB8[i] = (GLint) (val * 255.0F);
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_A:
         ctx->Pixel.MapItoAsize = mapsize;
         for (i = 0; i < mapsize; i++) {
            GLfloat val = CLAMP( values[i], 0.0F, 1.0F );
	    ctx->Pixel.MapItoA[i] = val;
	    ctx->Pixel.MapItoA8[i] = (GLint) (val * 255.0F);
	 }
	 break;
      case GL_PIXEL_MAP_R_TO_R:
         ctx->Pixel.MapRtoRsize = mapsize;
         for (i = 0; i < mapsize; i++) {
	    ctx->Pixel.MapRtoR[i] = CLAMP( values[i], 0.0F, 1.0F );
	 }
	 break;
      case GL_PIXEL_MAP_G_TO_G:
         ctx->Pixel.MapGtoGsize = mapsize;
         for (i = 0; i < mapsize; i++) {
	    ctx->Pixel.MapGtoG[i] = CLAMP( values[i], 0.0F, 1.0F );
	 }
	 break;
      case GL_PIXEL_MAP_B_TO_B:
         ctx->Pixel.MapBtoBsize = mapsize;
         for (i = 0; i < mapsize; i++) {
	    ctx->Pixel.MapBtoB[i] = CLAMP( values[i], 0.0F, 1.0F );
	 }
	 break;
      case GL_PIXEL_MAP_A_TO_A:
         ctx->Pixel.MapAtoAsize = mapsize;
         for (i = 0; i < mapsize; i++) {
	    ctx->Pixel.MapAtoA[i] = CLAMP( values[i], 0.0F, 1.0F );
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glPixelMap(map)" );
   }
}


void GLAPIENTRY
_mesa_PixelMapfv( GLenum map, GLsizei mapsize, const GLfloat *values )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   /* XXX someday, test against ctx->Const.MaxPixelMapTableSize */
   if (mapsize < 1 || mapsize > MAX_PIXEL_MAP_TABLE) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapfv(mapsize)" );
      return;
   }

   if (map >= GL_PIXEL_MAP_S_TO_S && map <= GL_PIXEL_MAP_I_TO_A) {
      /* test that mapsize is a power of two */
      if (_mesa_bitcount((GLuint) mapsize) != 1) {
	 _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapfv(mapsize)" );
         return;
      }
   }

   FLUSH_VERTICES(ctx, _NEW_PIXEL);

   if (ctx->Unpack.BufferObj->Name) {
      /* unpack pixelmap from PBO */
      GLubyte *buf;
      /* Note, need to use DefaultPacking and Unpack's buffer object */
      ctx->DefaultPacking.BufferObj = ctx->Unpack.BufferObj;
      if (!_mesa_validate_pbo_access(1, &ctx->DefaultPacking, mapsize, 1, 1,
                                     GL_INTENSITY, GL_FLOAT, values)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glPixelMapfv(invalid PBO access)");
         return;
      }
      /* restore */
      ctx->DefaultPacking.BufferObj = ctx->Array.NullBufferObj;
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                              GL_READ_ONLY_ARB,
                                              ctx->Unpack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glPixelMapfv(PBO is mapped)");
         return;
      }
      values = (const GLfloat *) ADD_POINTERS(buf, values);
   }
   else if (!values) {
      return;
   }

   pixelmap(ctx, map, mapsize, values);

   if (ctx->Unpack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              ctx->Unpack.BufferObj);
   }
}



void GLAPIENTRY
_mesa_PixelMapuiv(GLenum map, GLsizei mapsize, const GLuint *values )
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (mapsize < 1 || mapsize > MAX_PIXEL_MAP_TABLE) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapuiv(mapsize)" );
      return;
   }

   if (map >= GL_PIXEL_MAP_S_TO_S && map <= GL_PIXEL_MAP_I_TO_A) {
      /* test that mapsize is a power of two */
      if (_mesa_bitcount((GLuint) mapsize) != 1) {
	 _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapuiv(mapsize)" );
         return;
      }
   }

   FLUSH_VERTICES(ctx, _NEW_PIXEL);

   if (ctx->Unpack.BufferObj->Name) {
      /* unpack pixelmap from PBO */
      GLubyte *buf;
      /* Note, need to use DefaultPacking and Unpack's buffer object */
      ctx->DefaultPacking.BufferObj = ctx->Unpack.BufferObj;
      if (!_mesa_validate_pbo_access(1, &ctx->DefaultPacking, mapsize, 1, 1,
                                     GL_INTENSITY, GL_UNSIGNED_INT, values)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glPixelMapuiv(invalid PBO access)");
         return;
      }
      /* restore */
      ctx->DefaultPacking.BufferObj = ctx->Array.NullBufferObj;
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                              GL_READ_ONLY_ARB,
                                              ctx->Unpack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glPixelMapuiv(PBO is mapped)");
         return;
      }
      values = (const GLuint *) ADD_POINTERS(buf, values);
   }
   else if (!values) {
      return;
   }

   /* convert to floats */
   if (map == GL_PIXEL_MAP_I_TO_I || map == GL_PIXEL_MAP_S_TO_S) {
      GLint i;
      for (i = 0; i < mapsize; i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      GLint i;
      for (i = 0; i < mapsize; i++) {
         fvalues[i] = UINT_TO_FLOAT( values[i] );
      }
   }

   if (ctx->Unpack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              ctx->Unpack.BufferObj);
   }

   pixelmap(ctx, map, mapsize, fvalues);
}



void GLAPIENTRY
_mesa_PixelMapusv(GLenum map, GLsizei mapsize, const GLushort *values )
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (mapsize < 1 || mapsize > MAX_PIXEL_MAP_TABLE) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapusv(mapsize)" );
      return;
   }

   if (map >= GL_PIXEL_MAP_S_TO_S && map <= GL_PIXEL_MAP_I_TO_A) {
      /* test that mapsize is a power of two */
      if (_mesa_bitcount((GLuint) mapsize) != 1) {
	 _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapuiv(mapsize)" );
         return;
      }
   }

   FLUSH_VERTICES(ctx, _NEW_PIXEL);

   if (ctx->Unpack.BufferObj->Name) {
      /* unpack pixelmap from PBO */
      GLubyte *buf;
      /* Note, need to use DefaultPacking and Unpack's buffer object */
      ctx->DefaultPacking.BufferObj = ctx->Unpack.BufferObj;
      if (!_mesa_validate_pbo_access(1, &ctx->DefaultPacking, mapsize, 1, 1,
                                     GL_INTENSITY, GL_UNSIGNED_SHORT,
                                     values)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glPixelMapusv(invalid PBO access)");
         return;
      }
      /* restore */
      ctx->DefaultPacking.BufferObj = ctx->Array.NullBufferObj;
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                              GL_READ_ONLY_ARB,
                                              ctx->Unpack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glPixelMapusv(PBO is mapped)");
         return;
      }
      values = (const GLushort *) ADD_POINTERS(buf, values);
   }
   else if (!values) {
      return;
   }

    /* convert to floats */
   if (map == GL_PIXEL_MAP_I_TO_I || map == GL_PIXEL_MAP_S_TO_S) {
      GLint i;
      for (i = 0; i < mapsize; i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      GLint i;
      for (i = 0; i < mapsize; i++) {
         fvalues[i] = USHORT_TO_FLOAT( values[i] );
      }
   }

   if (ctx->Unpack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              ctx->Unpack.BufferObj);
   }

   pixelmap(ctx, map, mapsize, fvalues);
}


/**
 * Return size of the named map.
 */
static GLuint
get_map_size(GLcontext *ctx, GLenum map)
{
   switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
         return ctx->Pixel.MapItoIsize;
      case GL_PIXEL_MAP_S_TO_S:
         return ctx->Pixel.MapStoSsize;
      case GL_PIXEL_MAP_I_TO_R:
         return ctx->Pixel.MapItoRsize;
      case GL_PIXEL_MAP_I_TO_G:
         return ctx->Pixel.MapItoGsize;
      case GL_PIXEL_MAP_I_TO_B:
         return ctx->Pixel.MapItoBsize;
      case GL_PIXEL_MAP_I_TO_A:
         return ctx->Pixel.MapItoAsize;
      case GL_PIXEL_MAP_R_TO_R:
         return ctx->Pixel.MapRtoRsize;
      case GL_PIXEL_MAP_G_TO_G:
         return ctx->Pixel.MapGtoGsize;
      case GL_PIXEL_MAP_B_TO_B:
         return ctx->Pixel.MapBtoBsize;
      case GL_PIXEL_MAP_A_TO_A:
         return ctx->Pixel.MapAtoAsize;
      default:
         return 0;
   }
}


void GLAPIENTRY
_mesa_GetPixelMapfv( GLenum map, GLfloat *values )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint mapsize, i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   mapsize = get_map_size(ctx, map);

   if (ctx->Pack.BufferObj->Name) {
      /* pack pixelmap into PBO */
      GLubyte *buf;
      /* Note, need to use DefaultPacking and Pack's buffer object */
      ctx->DefaultPacking.BufferObj = ctx->Pack.BufferObj;
      if (!_mesa_validate_pbo_access(1, &ctx->DefaultPacking, mapsize, 1, 1,
                                     GL_INTENSITY, GL_FLOAT, values)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetPixelMapfv(invalid PBO access)");
         return;
      }
      /* restore */
      ctx->DefaultPacking.BufferObj = ctx->Array.NullBufferObj;
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                                              GL_WRITE_ONLY_ARB,
                                              ctx->Pack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetPixelMapfv(PBO is mapped)");
         return;
      }
      values = (GLfloat *) ADD_POINTERS(buf, values);
   }
   else if (!values) {
      return;
   }

   switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
         MEMCPY(values, ctx->Pixel.MapItoI, mapsize * sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_S_TO_S:
         for (i = 0; i < mapsize; i++) {
	    values[i] = (GLfloat) ctx->Pixel.MapStoS[i];
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_R:
         MEMCPY(values, ctx->Pixel.MapItoR, mapsize * sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_I_TO_G:
         MEMCPY(values, ctx->Pixel.MapItoG, mapsize * sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_I_TO_B:
         MEMCPY(values, ctx->Pixel.MapItoB, mapsize * sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_I_TO_A:
         MEMCPY(values, ctx->Pixel.MapItoA, mapsize * sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_R_TO_R:
         MEMCPY(values, ctx->Pixel.MapRtoR, mapsize * sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_G_TO_G:
         MEMCPY(values, ctx->Pixel.MapGtoG, mapsize * sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_B_TO_B:
         MEMCPY(values, ctx->Pixel.MapBtoB, mapsize * sizeof(GLfloat));
	 break;
      case GL_PIXEL_MAP_A_TO_A:
         MEMCPY(values, ctx->Pixel.MapAtoA, mapsize * sizeof(GLfloat));
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetPixelMapfv" );
   }

   if (ctx->Pack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                              ctx->Pack.BufferObj);
   }
}


void GLAPIENTRY
_mesa_GetPixelMapuiv( GLenum map, GLuint *values )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint mapsize, i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   mapsize = get_map_size(ctx, map);

   if (ctx->Pack.BufferObj->Name) {
      /* pack pixelmap into PBO */
      GLubyte *buf;
      /* Note, need to use DefaultPacking and Pack's buffer object */
      ctx->DefaultPacking.BufferObj = ctx->Pack.BufferObj;
      if (!_mesa_validate_pbo_access(1, &ctx->DefaultPacking, mapsize, 1, 1,
                                     GL_INTENSITY, GL_UNSIGNED_INT, values)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetPixelMapuiv(invalid PBO access)");
         return;
      }
      /* restore */
      ctx->DefaultPacking.BufferObj = ctx->Array.NullBufferObj;
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                                              GL_WRITE_ONLY_ARB,
                                              ctx->Pack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetPixelMapuiv(PBO is mapped)");
         return;
      }
      values = (GLuint *) ADD_POINTERS(buf, values);
   }
   else if (!values) {
      return;
   }

   switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapItoI[i] );
	 }
	 break;
      case GL_PIXEL_MAP_S_TO_S:
         MEMCPY(values, ctx->Pixel.MapStoS, mapsize * sizeof(GLint));
	 break;
      case GL_PIXEL_MAP_I_TO_R:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapItoR[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_G:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapItoG[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_B:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapItoB[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_A:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapItoA[i] );
	 }
	 break;
      case GL_PIXEL_MAP_R_TO_R:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapRtoR[i] );
	 }
	 break;
      case GL_PIXEL_MAP_G_TO_G:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapGtoG[i] );
	 }
	 break;
      case GL_PIXEL_MAP_B_TO_B:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapBtoB[i] );
	 }
	 break;
      case GL_PIXEL_MAP_A_TO_A:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_UINT( ctx->Pixel.MapAtoA[i] );
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetPixelMapfv" );
   }

   if (ctx->Pack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                              ctx->Pack.BufferObj);
   }
}


void GLAPIENTRY
_mesa_GetPixelMapusv( GLenum map, GLushort *values )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint mapsize, i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   mapsize = get_map_size(ctx, map);

   if (ctx->Pack.BufferObj->Name) {
      /* pack pixelmap into PBO */
      GLubyte *buf;
      /* Note, need to use DefaultPacking and Pack's buffer object */
      ctx->DefaultPacking.BufferObj = ctx->Pack.BufferObj;
      if (!_mesa_validate_pbo_access(1, &ctx->DefaultPacking, mapsize, 1, 1,
                                     GL_INTENSITY, GL_UNSIGNED_SHORT,
                                     values)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetPixelMapusv(invalid PBO access)");
         return;
      }
      /* restore */
      ctx->DefaultPacking.BufferObj = ctx->Array.NullBufferObj;
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                                              GL_WRITE_ONLY_ARB,
                                              ctx->Pack.BufferObj);
      if (!buf) {
         /* buffer is already mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glGetPixelMapusv(PBO is mapped)");
         return;
      }
      values = (GLushort *) ADD_POINTERS(buf, values);
   }
   else if (!values) {
      return;
   }

   switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_USHORT(ctx->Pixel.MapItoI[i]);
	 }
	 break;
      case GL_PIXEL_MAP_S_TO_S:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = (GLushort) ctx->Pixel.MapStoS[i];
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_R:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapItoR[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_G:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapItoG[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_B:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapItoB[i] );
	 }
	 break;
      case GL_PIXEL_MAP_I_TO_A:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapItoA[i] );
	 }
	 break;
      case GL_PIXEL_MAP_R_TO_R:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapRtoR[i] );
	 }
	 break;
      case GL_PIXEL_MAP_G_TO_G:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapGtoG[i] );
	 }
	 break;
      case GL_PIXEL_MAP_B_TO_B:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapBtoB[i] );
	 }
	 break;
      case GL_PIXEL_MAP_A_TO_A:
	 for (i = 0; i < mapsize; i++) {
	    values[i] = FLOAT_TO_USHORT( ctx->Pixel.MapAtoA[i] );
	 }
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetPixelMapfv" );
   }

   if (ctx->Pack.BufferObj->Name) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                              ctx->Pack.BufferObj);
   }
}



/**********************************************************************/
/*****                       glPixelTransfer                      *****/
/**********************************************************************/


/*
 * Implements glPixelTransfer[fi] whether called immediately or from a
 * display list.
 */
void GLAPIENTRY
_mesa_PixelTransferf( GLenum pname, GLfloat param )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (pname) {
      case GL_MAP_COLOR:
         if (ctx->Pixel.MapColorFlag == (param ? GL_TRUE : GL_FALSE))
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.MapColorFlag = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_MAP_STENCIL:
         if (ctx->Pixel.MapStencilFlag == (param ? GL_TRUE : GL_FALSE))
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.MapStencilFlag = param ? GL_TRUE : GL_FALSE;
	 break;
      case GL_INDEX_SHIFT:
         if (ctx->Pixel.IndexShift == (GLint) param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.IndexShift = (GLint) param;
	 break;
      case GL_INDEX_OFFSET:
         if (ctx->Pixel.IndexOffset == (GLint) param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.IndexOffset = (GLint) param;
	 break;
      case GL_RED_SCALE:
         if (ctx->Pixel.RedScale == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.RedScale = param;
	 break;
      case GL_RED_BIAS:
         if (ctx->Pixel.RedBias == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.RedBias = param;
	 break;
      case GL_GREEN_SCALE:
         if (ctx->Pixel.GreenScale == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.GreenScale = param;
	 break;
      case GL_GREEN_BIAS:
         if (ctx->Pixel.GreenBias == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.GreenBias = param;
	 break;
      case GL_BLUE_SCALE:
         if (ctx->Pixel.BlueScale == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.BlueScale = param;
	 break;
      case GL_BLUE_BIAS:
         if (ctx->Pixel.BlueBias == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.BlueBias = param;
	 break;
      case GL_ALPHA_SCALE:
         if (ctx->Pixel.AlphaScale == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.AlphaScale = param;
	 break;
      case GL_ALPHA_BIAS:
         if (ctx->Pixel.AlphaBias == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.AlphaBias = param;
	 break;
      case GL_DEPTH_SCALE:
         if (ctx->Pixel.DepthScale == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.DepthScale = param;
	 break;
      case GL_DEPTH_BIAS:
         if (ctx->Pixel.DepthBias == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.DepthBias = param;
	 break;
      case GL_POST_COLOR_MATRIX_RED_SCALE:
         if (ctx->Pixel.PostColorMatrixScale[0] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixScale[0] = param;
	 break;
      case GL_POST_COLOR_MATRIX_RED_BIAS:
         if (ctx->Pixel.PostColorMatrixBias[0] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixBias[0] = param;
	 break;
      case GL_POST_COLOR_MATRIX_GREEN_SCALE:
         if (ctx->Pixel.PostColorMatrixScale[1] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixScale[1] = param;
	 break;
      case GL_POST_COLOR_MATRIX_GREEN_BIAS:
         if (ctx->Pixel.PostColorMatrixBias[1] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixBias[1] = param;
	 break;
      case GL_POST_COLOR_MATRIX_BLUE_SCALE:
         if (ctx->Pixel.PostColorMatrixScale[2] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixScale[2] = param;
	 break;
      case GL_POST_COLOR_MATRIX_BLUE_BIAS:
         if (ctx->Pixel.PostColorMatrixBias[2] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixBias[2] = param;
	 break;
      case GL_POST_COLOR_MATRIX_ALPHA_SCALE:
         if (ctx->Pixel.PostColorMatrixScale[3] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixScale[3] = param;
	 break;
      case GL_POST_COLOR_MATRIX_ALPHA_BIAS:
         if (ctx->Pixel.PostColorMatrixBias[3] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostColorMatrixBias[3] = param;
	 break;
      case GL_POST_CONVOLUTION_RED_SCALE:
         if (ctx->Pixel.PostConvolutionScale[0] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionScale[0] = param;
	 break;
      case GL_POST_CONVOLUTION_RED_BIAS:
         if (ctx->Pixel.PostConvolutionBias[0] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionBias[0] = param;
	 break;
      case GL_POST_CONVOLUTION_GREEN_SCALE:
         if (ctx->Pixel.PostConvolutionScale[1] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionScale[1] = param;
	 break;
      case GL_POST_CONVOLUTION_GREEN_BIAS:
         if (ctx->Pixel.PostConvolutionBias[1] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionBias[1] = param;
	 break;
      case GL_POST_CONVOLUTION_BLUE_SCALE:
         if (ctx->Pixel.PostConvolutionScale[2] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionScale[2] = param;
	 break;
      case GL_POST_CONVOLUTION_BLUE_BIAS:
         if (ctx->Pixel.PostConvolutionBias[2] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionBias[2] = param;
	 break;
      case GL_POST_CONVOLUTION_ALPHA_SCALE:
         if (ctx->Pixel.PostConvolutionScale[2] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionScale[2] = param;
	 break;
      case GL_POST_CONVOLUTION_ALPHA_BIAS:
         if (ctx->Pixel.PostConvolutionBias[2] == param)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_PIXEL);
         ctx->Pixel.PostConvolutionBias[2] = param;
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glPixelTransfer(pname)" );
         return;
   }
}


void GLAPIENTRY
_mesa_PixelTransferi( GLenum pname, GLint param )
{
   _mesa_PixelTransferf( pname, (GLfloat) param );
}



/**********************************************************************/
/*****                  Pixel processing functions               ******/
/**********************************************************************/

/*
 * Apply scale and bias factors to an array of RGBA pixels.
 */
void
_mesa_scale_and_bias_rgba(GLuint n, GLfloat rgba[][4],
                          GLfloat rScale, GLfloat gScale,
                          GLfloat bScale, GLfloat aScale,
                          GLfloat rBias, GLfloat gBias,
                          GLfloat bBias, GLfloat aBias)
{
   if (rScale != 1.0 || rBias != 0.0) {
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][RCOMP] = rgba[i][RCOMP] * rScale + rBias;
      }
   }
   if (gScale != 1.0 || gBias != 0.0) {
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][GCOMP] = rgba[i][GCOMP] * gScale + gBias;
      }
   }
   if (bScale != 1.0 || bBias != 0.0) {
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][BCOMP] = rgba[i][BCOMP] * bScale + bBias;
      }
   }
   if (aScale != 1.0 || aBias != 0.0) {
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][ACOMP] = rgba[i][ACOMP] * aScale + aBias;
      }
   }
}


/*
 * Apply pixel mapping to an array of floating point RGBA pixels.
 */
void
_mesa_map_rgba( const GLcontext *ctx, GLuint n, GLfloat rgba[][4] )
{
   const GLfloat rscale = (GLfloat) (ctx->Pixel.MapRtoRsize - 1);
   const GLfloat gscale = (GLfloat) (ctx->Pixel.MapGtoGsize - 1);
   const GLfloat bscale = (GLfloat) (ctx->Pixel.MapBtoBsize - 1);
   const GLfloat ascale = (GLfloat) (ctx->Pixel.MapAtoAsize - 1);
   const GLfloat *rMap = ctx->Pixel.MapRtoR;
   const GLfloat *gMap = ctx->Pixel.MapGtoG;
   const GLfloat *bMap = ctx->Pixel.MapBtoB;
   const GLfloat *aMap = ctx->Pixel.MapAtoA;
   GLuint i;
   for (i=0;i<n;i++) {
      GLfloat r = CLAMP(rgba[i][RCOMP], 0.0F, 1.0F);
      GLfloat g = CLAMP(rgba[i][GCOMP], 0.0F, 1.0F);
      GLfloat b = CLAMP(rgba[i][BCOMP], 0.0F, 1.0F);
      GLfloat a = CLAMP(rgba[i][ACOMP], 0.0F, 1.0F);
      rgba[i][RCOMP] = rMap[IROUND(r * rscale)];
      rgba[i][GCOMP] = gMap[IROUND(g * gscale)];
      rgba[i][BCOMP] = bMap[IROUND(b * bscale)];
      rgba[i][ACOMP] = aMap[IROUND(a * ascale)];
   }
}


/*
 * Apply the color matrix and post color matrix scaling and biasing.
 */
void
_mesa_transform_rgba(const GLcontext *ctx, GLuint n, GLfloat rgba[][4])
{
   const GLfloat rs = ctx->Pixel.PostColorMatrixScale[0];
   const GLfloat rb = ctx->Pixel.PostColorMatrixBias[0];
   const GLfloat gs = ctx->Pixel.PostColorMatrixScale[1];
   const GLfloat gb = ctx->Pixel.PostColorMatrixBias[1];
   const GLfloat bs = ctx->Pixel.PostColorMatrixScale[2];
   const GLfloat bb = ctx->Pixel.PostColorMatrixBias[2];
   const GLfloat as = ctx->Pixel.PostColorMatrixScale[3];
   const GLfloat ab = ctx->Pixel.PostColorMatrixBias[3];
   const GLfloat *m = ctx->ColorMatrixStack.Top->m;
   GLuint i;
   for (i = 0; i < n; i++) {
      const GLfloat r = rgba[i][RCOMP];
      const GLfloat g = rgba[i][GCOMP];
      const GLfloat b = rgba[i][BCOMP];
      const GLfloat a = rgba[i][ACOMP];
      rgba[i][RCOMP] = (m[0] * r + m[4] * g + m[ 8] * b + m[12] * a) * rs + rb;
      rgba[i][GCOMP] = (m[1] * r + m[5] * g + m[ 9] * b + m[13] * a) * gs + gb;
      rgba[i][BCOMP] = (m[2] * r + m[6] * g + m[10] * b + m[14] * a) * bs + bb;
      rgba[i][ACOMP] = (m[3] * r + m[7] * g + m[11] * b + m[15] * a) * as + ab;
   }
}


/**
 * Apply a color table lookup to an array of floating point RGBA colors.
 */
void
_mesa_lookup_rgba_float(const struct gl_color_table *table,
                        GLuint n, GLfloat rgba[][4])
{
   if (!table->Table || table->Size == 0)
      return;

   switch (table->Format) {
      case GL_INTENSITY:
         /* replace RGBA with I */
         if (table->Type == GL_FLOAT) {
            const GLint max = table->Size - 1;
            const GLfloat scale = (GLfloat) max;
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = IROUND(rgba[i][RCOMP] * scale);
               GLfloat c = lut[CLAMP(j, 0, max)];
               rgba[i][RCOMP] = rgba[i][GCOMP] =
                  rgba[i][BCOMP] = rgba[i][ACOMP] = c;
            }
         }
         else {
            const GLint max = table->Size - 1;
            const GLfloat scale = (GLfloat) max;
            const GLchan *lut = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = IROUND(rgba[i][RCOMP] * scale);
               GLfloat c = CHAN_TO_FLOAT(lut[CLAMP(j, 0, max)]);
               rgba[i][RCOMP] = rgba[i][GCOMP] =
                  rgba[i][BCOMP] = rgba[i][ACOMP] = c;
            }
         }
         break;
      case GL_LUMINANCE:
         /* replace RGB with L */
         if (table->Type == GL_FLOAT) {
            const GLint max = table->Size - 1;
            const GLfloat scale = (GLfloat) max;
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = IROUND(rgba[i][RCOMP] * scale);
               GLfloat c = lut[CLAMP(j, 0, max)];
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = c;
            }
         }
         else {
            const GLint max = table->Size - 1;
            const GLfloat scale = (GLfloat) max;
            const GLchan *lut = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = IROUND(rgba[i][RCOMP] * scale);
               GLfloat c = CHAN_TO_FLOAT(lut[CLAMP(j, 0, max)]);
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = c;
            }
         }
         break;
      case GL_ALPHA:
         /* replace A with A */
         if (table->Type == GL_FLOAT) {
            const GLint max = table->Size - 1;
            const GLfloat scale = (GLfloat) max;
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = IROUND(rgba[i][ACOMP] * scale);
               rgba[i][ACOMP] = lut[CLAMP(j, 0, max)];
            }
         }
         else {
            const GLint max = table->Size - 1;
            const GLfloat scale = (GLfloat) max;
            const GLchan *lut = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = IROUND(rgba[i][ACOMP] * scale);
               rgba[i][ACOMP] = CHAN_TO_FLOAT(lut[CLAMP(j, 0, max)]);
            }
         }
         break;
      case GL_LUMINANCE_ALPHA:
         /* replace RGBA with LLLA */
         if (table->Type == GL_FLOAT) {
            const GLint max = table->Size - 1;
            const GLfloat scale = (GLfloat) max;
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jL = IROUND(rgba[i][RCOMP] * scale);
               GLint jA = IROUND(rgba[i][ACOMP] * scale);
               GLfloat luminance, alpha;
               jL = CLAMP(jL, 0, max);
               jA = CLAMP(jA, 0, max);
               luminance = lut[jL * 2 + 0];
               alpha     = lut[jA * 2 + 1];
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = luminance;
               rgba[i][ACOMP] = alpha;;
            }
         }
         else {
            const GLint max = table->Size - 1;
            const GLfloat scale = (GLfloat) max;
            const GLchan *lut = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jL = IROUND(rgba[i][RCOMP] * scale);
               GLint jA = IROUND(rgba[i][ACOMP] * scale);
               GLfloat luminance, alpha;
               jL = CLAMP(jL, 0, max);
               jA = CLAMP(jA, 0, max);
               luminance = CHAN_TO_FLOAT(lut[jL * 2 + 0]);
               alpha     = CHAN_TO_FLOAT(lut[jA * 2 + 1]);
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = luminance;
               rgba[i][ACOMP] = alpha;;
            }
         }
         break;
      case GL_RGB:
         /* replace RGB with RGB */
         if (table->Type == GL_FLOAT) {
            const GLint max = table->Size - 1;
            const GLfloat scale = (GLfloat) max;
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jR = IROUND(rgba[i][RCOMP] * scale);
               GLint jG = IROUND(rgba[i][GCOMP] * scale);
               GLint jB = IROUND(rgba[i][BCOMP] * scale);
               jR = CLAMP(jR, 0, max);
               jG = CLAMP(jG, 0, max);
               jB = CLAMP(jB, 0, max);
               rgba[i][RCOMP] = lut[jR * 3 + 0];
               rgba[i][GCOMP] = lut[jG * 3 + 1];
               rgba[i][BCOMP] = lut[jB * 3 + 2];
            }
         }
         else {
            const GLint max = table->Size - 1;
            const GLfloat scale = (GLfloat) max;
            const GLchan *lut = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jR = IROUND(rgba[i][RCOMP] * scale);
               GLint jG = IROUND(rgba[i][GCOMP] * scale);
               GLint jB = IROUND(rgba[i][BCOMP] * scale);
               jR = CLAMP(jR, 0, max);
               jG = CLAMP(jG, 0, max);
               jB = CLAMP(jB, 0, max);
               rgba[i][RCOMP] = CHAN_TO_FLOAT(lut[jR * 3 + 0]);
               rgba[i][GCOMP] = CHAN_TO_FLOAT(lut[jG * 3 + 1]);
               rgba[i][BCOMP] = CHAN_TO_FLOAT(lut[jB * 3 + 2]);
            }
         }
         break;
      case GL_RGBA:
         /* replace RGBA with RGBA */
         if (table->Type == GL_FLOAT) {
            const GLint max = table->Size - 1;
            const GLfloat scale = (GLfloat) max;
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jR = IROUND(rgba[i][RCOMP] * scale);
               GLint jG = IROUND(rgba[i][GCOMP] * scale);
               GLint jB = IROUND(rgba[i][BCOMP] * scale);
               GLint jA = IROUND(rgba[i][ACOMP] * scale);
               jR = CLAMP(jR, 0, max);
               jG = CLAMP(jG, 0, max);
               jB = CLAMP(jB, 0, max);
               jA = CLAMP(jA, 0, max);
               rgba[i][RCOMP] = lut[jR * 4 + 0];
               rgba[i][GCOMP] = lut[jG * 4 + 1];
               rgba[i][BCOMP] = lut[jB * 4 + 2];
               rgba[i][ACOMP] = lut[jA * 4 + 3];
            }
         }
         else {
            const GLint max = table->Size - 1;
            const GLfloat scale = (GLfloat) max;
            const GLchan *lut = (const GLchan *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jR = IROUND(rgba[i][RCOMP] * scale);
               GLint jG = IROUND(rgba[i][GCOMP] * scale);
               GLint jB = IROUND(rgba[i][BCOMP] * scale);
               GLint jA = IROUND(rgba[i][ACOMP] * scale);
               jR = CLAMP(jR, 0, max);
               jG = CLAMP(jG, 0, max);
               jB = CLAMP(jB, 0, max);
               jA = CLAMP(jA, 0, max);
               rgba[i][RCOMP] = CHAN_TO_FLOAT(lut[jR * 4 + 0]);
               rgba[i][GCOMP] = CHAN_TO_FLOAT(lut[jG * 4 + 1]);
               rgba[i][BCOMP] = CHAN_TO_FLOAT(lut[jB * 4 + 2]);
               rgba[i][ACOMP] = CHAN_TO_FLOAT(lut[jA * 4 + 3]);
            }
         }
         break;
      default:
         _mesa_problem(NULL, "Bad format in _mesa_lookup_rgba_float");
         return;
   }
}



/**
 * Apply a color table lookup to an array of GLchan RGBA colors.
 */
void
_mesa_lookup_rgba_chan(const struct gl_color_table *table,
                       GLuint n, GLchan rgba[][4])
{
   if (!table->Table || table->Size == 0)
      return;

   switch (table->Format) {
      case GL_INTENSITY:
         /* replace RGBA with I */
         if (table->Type == GL_FLOAT) {
            const GLfloat scale = (GLfloat) (table->Size - 1) / CHAN_MAXF;
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = IROUND((GLfloat) rgba[i][RCOMP] * scale);
               GLchan c;
               CLAMPED_FLOAT_TO_CHAN(c, lut[j]);
               rgba[i][RCOMP] = rgba[i][GCOMP] =
                  rgba[i][BCOMP] = rgba[i][ACOMP] = c;
            }
         }
         else {
#if CHAN_TYPE == GL_UNSIGNED_BYTE
            if (table->Size == 256) {
               /* common case */
               const GLchan *lut = (const GLchan *) table->Table;
               GLuint i;
               for (i = 0; i < n; i++) {
                  const GLchan c = lut[rgba[i][RCOMP]];
                  rgba[i][RCOMP] = rgba[i][GCOMP] =
                     rgba[i][BCOMP] = rgba[i][ACOMP] = c;
               }
            }
            else
#endif
            {
               const GLfloat scale = (GLfloat) (table->Size - 1) / CHAN_MAXF;
               const GLchan *lut = (const GLchan *) table->Table;
               GLuint i;
               for (i = 0; i < n; i++) {
                  GLint j = IROUND((GLfloat) rgba[i][RCOMP] * scale);
                  rgba[i][RCOMP] = rgba[i][GCOMP] =
                     rgba[i][BCOMP] = rgba[i][ACOMP] = lut[j];
               }
            }
         }
         break;
      case GL_LUMINANCE:
         /* replace RGB with L */
         if (table->Type == GL_FLOAT) {
            const GLfloat scale = (GLfloat) (table->Size - 1) / CHAN_MAXF;
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = IROUND((GLfloat) rgba[i][RCOMP] * scale);
               GLchan c;
               CLAMPED_FLOAT_TO_CHAN(c, lut[j]);
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = c;
            }
         }
         else {
#if CHAN_TYPE == GL_UNSIGNED_BYTE
            if (table->Size == 256) {
               /* common case */
               const GLchan *lut = (const GLchan *) table->Table;
               GLuint i;
               for (i = 0; i < n; i++) {
                  const GLchan c = lut[rgba[i][RCOMP]];
                  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = c;
               }
            }
            else
#endif
            {
               const GLfloat scale = (GLfloat) (table->Size - 1) / CHAN_MAXF;
               const GLchan *lut = (const GLchan *) table->Table;
               GLuint i;
               for (i = 0; i < n; i++) {
                  GLint j = IROUND((GLfloat) rgba[i][RCOMP] * scale);
                  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = lut[j];
               }
            }
         }
         break;
      case GL_ALPHA:
         /* replace A with A */
         if (table->Type == GL_FLOAT) {
            const GLfloat scale = (GLfloat) (table->Size - 1) / CHAN_MAXF;
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint j = IROUND((GLfloat) rgba[i][ACOMP] * scale);
               GLchan c;
               CLAMPED_FLOAT_TO_CHAN(c, lut[j]);
               rgba[i][ACOMP] = c;
            }
         }
         else {
#if CHAN_TYPE == GL_UNSIGNED_BYTE
            if (table->Size == 256) {
               /* common case */
               const GLchan *lut = (const GLchan *) table->Table;
               GLuint i;
               for (i = 0; i < n; i++) {
                  rgba[i][ACOMP] = lut[rgba[i][ACOMP]];
               }
            }
            else
#endif
            {
               const GLfloat scale = (GLfloat) (table->Size - 1) / CHAN_MAXF;
               const GLchan *lut = (const GLchan *) table->Table;
               GLuint i;
               for (i = 0; i < n; i++) {
                  GLint j = IROUND((GLfloat) rgba[i][ACOMP] * scale);
                  rgba[i][ACOMP] = lut[j];
               }
            }
         }
         break;
      case GL_LUMINANCE_ALPHA:
         /* replace RGBA with LLLA */
         if (table->Type == GL_FLOAT) {
            const GLfloat scale = (GLfloat) (table->Size - 1) / CHAN_MAXF;
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jL = IROUND((GLfloat) rgba[i][RCOMP] * scale);
               GLint jA = IROUND((GLfloat) rgba[i][ACOMP] * scale);
               GLchan luminance, alpha;
               CLAMPED_FLOAT_TO_CHAN(luminance, lut[jL * 2 + 0]);
               CLAMPED_FLOAT_TO_CHAN(alpha, lut[jA * 2 + 1]);
               rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = luminance;
               rgba[i][ACOMP] = alpha;;
            }
         }
         else {
#if CHAN_TYPE == GL_UNSIGNED_BYTE
            if (table->Size == 256) {
               /* common case */
               const GLchan *lut = (const GLchan *) table->Table;
               GLuint i;
               for (i = 0; i < n; i++) {
                  GLchan l = lut[rgba[i][RCOMP] * 2 + 0];
                  GLchan a = lut[rgba[i][ACOMP] * 2 + 1];;
                  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = l;
                  rgba[i][ACOMP] = a;
               }
            }
            else
#endif
            {
               const GLfloat scale = (GLfloat) (table->Size - 1) / CHAN_MAXF;
               const GLchan *lut = (const GLchan *) table->Table;
               GLuint i;
               for (i = 0; i < n; i++) {
                  GLint jL = IROUND((GLfloat) rgba[i][RCOMP] * scale);
                  GLint jA = IROUND((GLfloat) rgba[i][ACOMP] * scale);
                  GLchan luminance = lut[jL * 2 + 0];
                  GLchan alpha     = lut[jA * 2 + 1];
                  rgba[i][RCOMP] = rgba[i][GCOMP] = rgba[i][BCOMP] = luminance;
                  rgba[i][ACOMP] = alpha;
               }
            }
         }
         break;
      case GL_RGB:
         /* replace RGB with RGB */
         if (table->Type == GL_FLOAT) {
            const GLfloat scale = (GLfloat) (table->Size - 1) / CHAN_MAXF;
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jR = IROUND((GLfloat) rgba[i][RCOMP] * scale);
               GLint jG = IROUND((GLfloat) rgba[i][GCOMP] * scale);
               GLint jB = IROUND((GLfloat) rgba[i][BCOMP] * scale);
               CLAMPED_FLOAT_TO_CHAN(rgba[i][RCOMP], lut[jR * 3 + 0]);
               CLAMPED_FLOAT_TO_CHAN(rgba[i][GCOMP], lut[jG * 3 + 1]);
               CLAMPED_FLOAT_TO_CHAN(rgba[i][BCOMP], lut[jB * 3 + 2]);
            }
         }
         else {
#if CHAN_TYPE == GL_UNSIGNED_BYTE
            if (table->Size == 256) {
               /* common case */
               const GLchan *lut = (const GLchan *) table->Table;
               GLuint i;
               for (i = 0; i < n; i++) {
                  rgba[i][RCOMP] = lut[rgba[i][RCOMP] * 3 + 0];
                  rgba[i][GCOMP] = lut[rgba[i][GCOMP] * 3 + 1];
                  rgba[i][BCOMP] = lut[rgba[i][BCOMP] * 3 + 2];
               }
            }
            else
#endif
            {
               const GLfloat scale = (GLfloat) (table->Size - 1) / CHAN_MAXF;
               const GLchan *lut = (const GLchan *) table->Table;
               GLuint i;
               for (i = 0; i < n; i++) {
                  GLint jR = IROUND((GLfloat) rgba[i][RCOMP] * scale);
                  GLint jG = IROUND((GLfloat) rgba[i][GCOMP] * scale);
                  GLint jB = IROUND((GLfloat) rgba[i][BCOMP] * scale);
                  rgba[i][RCOMP] = lut[jR * 3 + 0];
                  rgba[i][GCOMP] = lut[jG * 3 + 1];
                  rgba[i][BCOMP] = lut[jB * 3 + 2];
               }
            }
         }
         break;
      case GL_RGBA:
         /* replace RGBA with RGBA */
         if (table->Type == GL_FLOAT) {
            const GLfloat scale = (GLfloat) (table->Size - 1) / CHAN_MAXF;
            const GLfloat *lut = (const GLfloat *) table->Table;
            GLuint i;
            for (i = 0; i < n; i++) {
               GLint jR = IROUND((GLfloat) rgba[i][RCOMP] * scale);
               GLint jG = IROUND((GLfloat) rgba[i][GCOMP] * scale);
               GLint jB = IROUND((GLfloat) rgba[i][BCOMP] * scale);
               GLint jA = IROUND((GLfloat) rgba[i][ACOMP] * scale);
               CLAMPED_FLOAT_TO_CHAN(rgba[i][RCOMP], lut[jR * 4 + 0]);
               CLAMPED_FLOAT_TO_CHAN(rgba[i][GCOMP], lut[jG * 4 + 1]);
               CLAMPED_FLOAT_TO_CHAN(rgba[i][BCOMP], lut[jB * 4 + 2]);
               CLAMPED_FLOAT_TO_CHAN(rgba[i][ACOMP], lut[jA * 4 + 3]);
            }
         }
         else {
#if CHAN_TYPE == GL_UNSIGNED_BYTE
            if (table->Size == 256) {
               /* common case */
               const GLchan *lut = (const GLchan *) table->Table;
               GLuint i;
               for (i = 0; i < n; i++) {
                  rgba[i][RCOMP] = lut[rgba[i][RCOMP] * 4 + 0];
                  rgba[i][GCOMP] = lut[rgba[i][GCOMP] * 4 + 1];
                  rgba[i][BCOMP] = lut[rgba[i][BCOMP] * 4 + 2];
                  rgba[i][ACOMP] = lut[rgba[i][ACOMP] * 4 + 3];
               }
            }
            else
#endif
            {
               const GLfloat scale = (GLfloat) (table->Size - 1) / CHAN_MAXF;
               const GLfloat *lut = (const GLfloat *) table->Table;
               GLuint i;
               for (i = 0; i < n; i++) {
                  GLint jR = IROUND((GLfloat) rgba[i][RCOMP] * scale);
                  GLint jG = IROUND((GLfloat) rgba[i][GCOMP] * scale);
                  GLint jB = IROUND((GLfloat) rgba[i][BCOMP] * scale);
                  GLint jA = IROUND((GLfloat) rgba[i][ACOMP] * scale);
                  CLAMPED_FLOAT_TO_CHAN(rgba[i][RCOMP], lut[jR * 4 + 0]);
                  CLAMPED_FLOAT_TO_CHAN(rgba[i][GCOMP], lut[jG * 4 + 1]);
                  CLAMPED_FLOAT_TO_CHAN(rgba[i][BCOMP], lut[jB * 4 + 2]);
                  CLAMPED_FLOAT_TO_CHAN(rgba[i][ACOMP], lut[jA * 4 + 3]);
               }
            }
         }
         break;
      default:
         _mesa_problem(NULL, "Bad format in _mesa_lookup_rgba_chan");
         return;
   }
}



/*
 * Apply color index shift and offset to an array of pixels.
 */
void
_mesa_shift_and_offset_ci( const GLcontext *ctx, GLuint n, GLuint indexes[] )
{
   GLint shift = ctx->Pixel.IndexShift;
   GLint offset = ctx->Pixel.IndexOffset;
   GLuint i;
   if (shift > 0) {
      for (i=0;i<n;i++) {
         indexes[i] = (indexes[i] << shift) + offset;
      }
   }
   else if (shift < 0) {
      shift = -shift;
      for (i=0;i<n;i++) {
         indexes[i] = (indexes[i] >> shift) + offset;
      }
   }
   else {
      for (i=0;i<n;i++) {
         indexes[i] = indexes[i] + offset;
      }
   }
}


/*
 * Apply color index mapping to color indexes.
 */
void
_mesa_map_ci( const GLcontext *ctx, GLuint n, GLuint index[] )
{
   const GLuint mask = ctx->Pixel.MapItoIsize - 1;
   GLuint i;
   for (i = 0; i < n; i++) {
      const GLuint j = index[i] & mask;
      index[i] = IROUND(ctx->Pixel.MapItoI[j]);
   }
}


/*
 * Map color indexes to rgba values.
 */
void
_mesa_map_ci_to_rgba_chan( const GLcontext *ctx, GLuint n,
                           const GLuint index[], GLchan rgba[][4] )
{
#if CHAN_BITS == 8
   GLuint rmask = ctx->Pixel.MapItoRsize - 1;
   GLuint gmask = ctx->Pixel.MapItoGsize - 1;
   GLuint bmask = ctx->Pixel.MapItoBsize - 1;
   GLuint amask = ctx->Pixel.MapItoAsize - 1;
   const GLubyte *rMap = ctx->Pixel.MapItoR8;
   const GLubyte *gMap = ctx->Pixel.MapItoG8;
   const GLubyte *bMap = ctx->Pixel.MapItoB8;
   const GLubyte *aMap = ctx->Pixel.MapItoA8;
   GLuint i;
   for (i=0;i<n;i++) {
      rgba[i][RCOMP] = rMap[index[i] & rmask];
      rgba[i][GCOMP] = gMap[index[i] & gmask];
      rgba[i][BCOMP] = bMap[index[i] & bmask];
      rgba[i][ACOMP] = aMap[index[i] & amask];
   }
#else
   GLuint rmask = ctx->Pixel.MapItoRsize - 1;
   GLuint gmask = ctx->Pixel.MapItoGsize - 1;
   GLuint bmask = ctx->Pixel.MapItoBsize - 1;
   GLuint amask = ctx->Pixel.MapItoAsize - 1;
   const GLfloat *rMap = ctx->Pixel.MapItoR;
   const GLfloat *gMap = ctx->Pixel.MapItoG;
   const GLfloat *bMap = ctx->Pixel.MapItoB;
   const GLfloat *aMap = ctx->Pixel.MapItoA;
   GLuint i;
   for (i=0;i<n;i++) {
      CLAMPED_FLOAT_TO_CHAN(rgba[i][RCOMP], rMap[index[i] & rmask]);
      CLAMPED_FLOAT_TO_CHAN(rgba[i][GCOMP], gMap[index[i] & gmask]);
      CLAMPED_FLOAT_TO_CHAN(rgba[i][BCOMP], bMap[index[i] & bmask]);
      CLAMPED_FLOAT_TO_CHAN(rgba[i][ACOMP], aMap[index[i] & amask]);
   }
#endif
}


/*
 * Map color indexes to float rgba values.
 */
void
_mesa_map_ci_to_rgba( const GLcontext *ctx, GLuint n,
                      const GLuint index[], GLfloat rgba[][4] )
{
   GLuint rmask = ctx->Pixel.MapItoRsize - 1;
   GLuint gmask = ctx->Pixel.MapItoGsize - 1;
   GLuint bmask = ctx->Pixel.MapItoBsize - 1;
   GLuint amask = ctx->Pixel.MapItoAsize - 1;
   const GLfloat *rMap = ctx->Pixel.MapItoR;
   const GLfloat *gMap = ctx->Pixel.MapItoG;
   const GLfloat *bMap = ctx->Pixel.MapItoB;
   const GLfloat *aMap = ctx->Pixel.MapItoA;
   GLuint i;
   for (i=0;i<n;i++) {
      rgba[i][RCOMP] = rMap[index[i] & rmask];
      rgba[i][GCOMP] = gMap[index[i] & gmask];
      rgba[i][BCOMP] = bMap[index[i] & bmask];
      rgba[i][ACOMP] = aMap[index[i] & amask];
   }
}


/*
 * Map 8-bit color indexes to rgb values.
 */
void
_mesa_map_ci8_to_rgba( const GLcontext *ctx, GLuint n, const GLubyte index[],
                       GLchan rgba[][4] )
{
#if CHAN_BITS == 8
   GLuint rmask = ctx->Pixel.MapItoRsize - 1;
   GLuint gmask = ctx->Pixel.MapItoGsize - 1;
   GLuint bmask = ctx->Pixel.MapItoBsize - 1;
   GLuint amask = ctx->Pixel.MapItoAsize - 1;
   const GLubyte *rMap = ctx->Pixel.MapItoR8;
   const GLubyte *gMap = ctx->Pixel.MapItoG8;
   const GLubyte *bMap = ctx->Pixel.MapItoB8;
   const GLubyte *aMap = ctx->Pixel.MapItoA8;
   GLuint i;
   for (i=0;i<n;i++) {
      rgba[i][RCOMP] = rMap[index[i] & rmask];
      rgba[i][GCOMP] = gMap[index[i] & gmask];
      rgba[i][BCOMP] = bMap[index[i] & bmask];
      rgba[i][ACOMP] = aMap[index[i] & amask];
   }
#else
   GLuint rmask = ctx->Pixel.MapItoRsize - 1;
   GLuint gmask = ctx->Pixel.MapItoGsize - 1;
   GLuint bmask = ctx->Pixel.MapItoBsize - 1;
   GLuint amask = ctx->Pixel.MapItoAsize - 1;
   const GLfloat *rMap = ctx->Pixel.MapItoR;
   const GLfloat *gMap = ctx->Pixel.MapItoG;
   const GLfloat *bMap = ctx->Pixel.MapItoB;
   const GLfloat *aMap = ctx->Pixel.MapItoA;
   GLuint i;
   for (i=0;i<n;i++) {
      CLAMPED_FLOAT_TO_CHAN(rgba[i][RCOMP], rMap[index[i] & rmask]);
      CLAMPED_FLOAT_TO_CHAN(rgba[i][GCOMP], gMap[index[i] & gmask]);
      CLAMPED_FLOAT_TO_CHAN(rgba[i][BCOMP], bMap[index[i] & bmask]);
      CLAMPED_FLOAT_TO_CHAN(rgba[i][ACOMP], aMap[index[i] & amask]);
   }
#endif
}


void
_mesa_shift_and_offset_stencil( const GLcontext *ctx, GLuint n,
                                GLstencil stencil[] )
{
   GLuint i;
   GLint shift = ctx->Pixel.IndexShift;
   GLint offset = ctx->Pixel.IndexOffset;
   if (shift > 0) {
      for (i=0;i<n;i++) {
         stencil[i] = (stencil[i] << shift) + offset;
      }
   }
   else if (shift < 0) {
      shift = -shift;
      for (i=0;i<n;i++) {
         stencil[i] = (stencil[i] >> shift) + offset;
      }
   }
   else {
      for (i=0;i<n;i++) {
         stencil[i] = stencil[i] + offset;
      }
   }

}


void
_mesa_map_stencil( const GLcontext *ctx, GLuint n, GLstencil stencil[] )
{
   GLuint mask = ctx->Pixel.MapStoSsize - 1;
   GLuint i;
   for (i=0;i<n;i++) {
      stencil[i] = ctx->Pixel.MapStoS[ stencil[i] & mask ];
   }
}



/*
 * This function converts an array of GLchan colors to GLfloat colors.
 * Most importantly, it undoes the non-uniform quantization of pixel
 * values introduced when we convert shallow (< 8 bit) pixel values
 * to GLubytes in the ctx->Driver.ReadRGBASpan() functions.
 * This fixes a number of OpenGL conformance failures when running on
 * 16bpp displays, for example.
 */
void
_mesa_chan_to_float_span(const GLcontext *ctx, GLuint n,
                         CONST GLchan rgba[][4], GLfloat rgbaf[][4])
{
#if CHAN_TYPE == GL_FLOAT
   MEMCPY(rgbaf, rgba, n * 4 * sizeof(GLfloat));
#else
   const GLuint rShift = CHAN_BITS - ctx->Visual.redBits;
   const GLuint gShift = CHAN_BITS - ctx->Visual.greenBits;
   const GLuint bShift = CHAN_BITS - ctx->Visual.blueBits;
   GLuint aShift;
   const GLfloat rScale = 1.0F / (GLfloat) ((1 << ctx->Visual.redBits  ) - 1);
   const GLfloat gScale = 1.0F / (GLfloat) ((1 << ctx->Visual.greenBits) - 1);
   const GLfloat bScale = 1.0F / (GLfloat) ((1 << ctx->Visual.blueBits ) - 1);
   GLfloat aScale;
   GLuint i;

   if (ctx->Visual.alphaBits > 0) {
      aShift = CHAN_BITS - ctx->Visual.alphaBits;
      aScale = 1.0F / (GLfloat) ((1 << ctx->Visual.alphaBits) - 1);
   }
   else {
      aShift = 0;
      aScale = 1.0F / CHAN_MAXF;
   }

   for (i = 0; i < n; i++) {
      const GLint r = rgba[i][RCOMP] >> rShift;
      const GLint g = rgba[i][GCOMP] >> gShift;
      const GLint b = rgba[i][BCOMP] >> bShift;
      const GLint a = rgba[i][ACOMP] >> aShift;
      rgbaf[i][RCOMP] = (GLfloat) r * rScale;
      rgbaf[i][GCOMP] = (GLfloat) g * gScale;
      rgbaf[i][BCOMP] = (GLfloat) b * bScale;
      rgbaf[i][ACOMP] = (GLfloat) a * aScale;
   }
#endif
}

/**********************************************************************/
/*****                    State Management                        *****/
/**********************************************************************/

/*
 * Return a bitmask of IMAGE_*_BIT flags which to indicate which
 * pixel transfer operations are enabled.
 */
static void
update_image_transfer_state(GLcontext *ctx)
{
   GLuint mask = 0;

   if (ctx->Pixel.RedScale   != 1.0F || ctx->Pixel.RedBias   != 0.0F ||
       ctx->Pixel.GreenScale != 1.0F || ctx->Pixel.GreenBias != 0.0F ||
       ctx->Pixel.BlueScale  != 1.0F || ctx->Pixel.BlueBias  != 0.0F ||
       ctx->Pixel.AlphaScale != 1.0F || ctx->Pixel.AlphaBias != 0.0F)
      mask |= IMAGE_SCALE_BIAS_BIT;

   if (ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset)
      mask |= IMAGE_SHIFT_OFFSET_BIT;

   if (ctx->Pixel.MapColorFlag)
      mask |= IMAGE_MAP_COLOR_BIT;

   if (ctx->Pixel.ColorTableEnabled)
      mask |= IMAGE_COLOR_TABLE_BIT;

   if (ctx->Pixel.Convolution1DEnabled ||
       ctx->Pixel.Convolution2DEnabled ||
       ctx->Pixel.Separable2DEnabled) {
      mask |= IMAGE_CONVOLUTION_BIT;
      if (ctx->Pixel.PostConvolutionScale[0] != 1.0F ||
          ctx->Pixel.PostConvolutionScale[1] != 1.0F ||
          ctx->Pixel.PostConvolutionScale[2] != 1.0F ||
          ctx->Pixel.PostConvolutionScale[3] != 1.0F ||
          ctx->Pixel.PostConvolutionBias[0] != 0.0F ||
          ctx->Pixel.PostConvolutionBias[1] != 0.0F ||
          ctx->Pixel.PostConvolutionBias[2] != 0.0F ||
          ctx->Pixel.PostConvolutionBias[3] != 0.0F) {
         mask |= IMAGE_POST_CONVOLUTION_SCALE_BIAS;
      }
   }

   if (ctx->Pixel.PostConvolutionColorTableEnabled)
      mask |= IMAGE_POST_CONVOLUTION_COLOR_TABLE_BIT;

   if (ctx->ColorMatrixStack.Top->type != MATRIX_IDENTITY ||
       ctx->Pixel.PostColorMatrixScale[0] != 1.0F ||
       ctx->Pixel.PostColorMatrixBias[0]  != 0.0F ||
       ctx->Pixel.PostColorMatrixScale[1] != 1.0F ||
       ctx->Pixel.PostColorMatrixBias[1]  != 0.0F ||
       ctx->Pixel.PostColorMatrixScale[2] != 1.0F ||
       ctx->Pixel.PostColorMatrixBias[2]  != 0.0F ||
       ctx->Pixel.PostColorMatrixScale[3] != 1.0F ||
       ctx->Pixel.PostColorMatrixBias[3]  != 0.0F)
      mask |= IMAGE_COLOR_MATRIX_BIT;

   if (ctx->Pixel.PostColorMatrixColorTableEnabled)
      mask |= IMAGE_POST_COLOR_MATRIX_COLOR_TABLE_BIT;

   if (ctx->Pixel.HistogramEnabled)
      mask |= IMAGE_HISTOGRAM_BIT;

   if (ctx->Pixel.MinMaxEnabled)
      mask |= IMAGE_MIN_MAX_BIT;

   ctx->_ImageTransferState = mask;
}


void _mesa_update_pixel( GLcontext *ctx, GLuint new_state )
{
   if (new_state & _NEW_COLOR_MATRIX)
      _math_matrix_analyse( ctx->ColorMatrixStack.Top );

   /* References ColorMatrix.type (derived above).
    */
   if (new_state & _IMAGE_NEW_TRANSFER_STATE)
      update_image_transfer_state(ctx);
}


/**********************************************************************/
/*****                      Initialization                        *****/
/**********************************************************************/


/**
 * Initialize the context's PIXEL attribute group.
 */
void
_mesa_init_pixel( GLcontext *ctx )
{
   int i;

   /* Pixel group */
   ctx->Pixel.RedBias = 0.0;
   ctx->Pixel.RedScale = 1.0;
   ctx->Pixel.GreenBias = 0.0;
   ctx->Pixel.GreenScale = 1.0;
   ctx->Pixel.BlueBias = 0.0;
   ctx->Pixel.BlueScale = 1.0;
   ctx->Pixel.AlphaBias = 0.0;
   ctx->Pixel.AlphaScale = 1.0;
   ctx->Pixel.DepthBias = 0.0;
   ctx->Pixel.DepthScale = 1.0;
   ctx->Pixel.IndexOffset = 0;
   ctx->Pixel.IndexShift = 0;
   ctx->Pixel.ZoomX = 1.0;
   ctx->Pixel.ZoomY = 1.0;
   ctx->Pixel.MapColorFlag = GL_FALSE;
   ctx->Pixel.MapStencilFlag = GL_FALSE;
   ctx->Pixel.MapStoSsize = 1;
   ctx->Pixel.MapItoIsize = 1;
   ctx->Pixel.MapItoRsize = 1;
   ctx->Pixel.MapItoGsize = 1;
   ctx->Pixel.MapItoBsize = 1;
   ctx->Pixel.MapItoAsize = 1;
   ctx->Pixel.MapRtoRsize = 1;
   ctx->Pixel.MapGtoGsize = 1;
   ctx->Pixel.MapBtoBsize = 1;
   ctx->Pixel.MapAtoAsize = 1;
   ctx->Pixel.MapStoS[0] = 0;
   ctx->Pixel.MapItoI[0] = 0.0;
   ctx->Pixel.MapItoR[0] = 0.0;
   ctx->Pixel.MapItoG[0] = 0.0;
   ctx->Pixel.MapItoB[0] = 0.0;
   ctx->Pixel.MapItoA[0] = 0.0;
   ctx->Pixel.MapItoR8[0] = 0;
   ctx->Pixel.MapItoG8[0] = 0;
   ctx->Pixel.MapItoB8[0] = 0;
   ctx->Pixel.MapItoA8[0] = 0;
   ctx->Pixel.MapRtoR[0] = 0.0;
   ctx->Pixel.MapGtoG[0] = 0.0;
   ctx->Pixel.MapBtoB[0] = 0.0;
   ctx->Pixel.MapAtoA[0] = 0.0;
   ctx->Pixel.HistogramEnabled = GL_FALSE;
   ctx->Pixel.MinMaxEnabled = GL_FALSE;
   ctx->Pixel.PixelTextureEnabled = GL_FALSE;
   ctx->Pixel.FragmentRgbSource = GL_PIXEL_GROUP_COLOR_SGIS;
   ctx->Pixel.FragmentAlphaSource = GL_PIXEL_GROUP_COLOR_SGIS;
   ASSIGN_4V(ctx->Pixel.PostColorMatrixScale, 1.0, 1.0, 1.0, 1.0);
   ASSIGN_4V(ctx->Pixel.PostColorMatrixBias, 0.0, 0.0, 0.0, 0.0);
   ASSIGN_4V(ctx->Pixel.ColorTableScale, 1.0, 1.0, 1.0, 1.0);
   ASSIGN_4V(ctx->Pixel.ColorTableBias, 0.0, 0.0, 0.0, 0.0);
   ASSIGN_4V(ctx->Pixel.PCCTscale, 1.0, 1.0, 1.0, 1.0);
   ASSIGN_4V(ctx->Pixel.PCCTbias, 0.0, 0.0, 0.0, 0.0);
   ASSIGN_4V(ctx->Pixel.PCMCTscale, 1.0, 1.0, 1.0, 1.0);
   ASSIGN_4V(ctx->Pixel.PCMCTbias, 0.0, 0.0, 0.0, 0.0);
   ctx->Pixel.ColorTableEnabled = GL_FALSE;
   ctx->Pixel.PostConvolutionColorTableEnabled = GL_FALSE;
   ctx->Pixel.PostColorMatrixColorTableEnabled = GL_FALSE;
   ctx->Pixel.Convolution1DEnabled = GL_FALSE;
   ctx->Pixel.Convolution2DEnabled = GL_FALSE;
   ctx->Pixel.Separable2DEnabled = GL_FALSE;
   for (i = 0; i < 3; i++) {
      ASSIGN_4V(ctx->Pixel.ConvolutionBorderColor[i], 0.0, 0.0, 0.0, 0.0);
      ctx->Pixel.ConvolutionBorderMode[i] = GL_REDUCE;
      ASSIGN_4V(ctx->Pixel.ConvolutionFilterScale[i], 1.0, 1.0, 1.0, 1.0);
      ASSIGN_4V(ctx->Pixel.ConvolutionFilterBias[i], 0.0, 0.0, 0.0, 0.0);
   }
   for (i = 0; i < MAX_CONVOLUTION_WIDTH * MAX_CONVOLUTION_WIDTH * 4; i++) {
      ctx->Convolution1D.Filter[i] = 0.0;
      ctx->Convolution2D.Filter[i] = 0.0;
      ctx->Separable2D.Filter[i] = 0.0;
   }
   ASSIGN_4V(ctx->Pixel.PostConvolutionScale, 1.0, 1.0, 1.0, 1.0);
   ASSIGN_4V(ctx->Pixel.PostConvolutionBias, 0.0, 0.0, 0.0, 0.0);
   /* GL_SGI_texture_color_table */
   ASSIGN_4V(ctx->Pixel.TextureColorTableScale, 1.0, 1.0, 1.0, 1.0);
   ASSIGN_4V(ctx->Pixel.TextureColorTableBias, 0.0, 0.0, 0.0, 0.0);

   /* Pixel transfer */
   ctx->Pack.Alignment = 4;
   ctx->Pack.RowLength = 0;
   ctx->Pack.ImageHeight = 0;
   ctx->Pack.SkipPixels = 0;
   ctx->Pack.SkipRows = 0;
   ctx->Pack.SkipImages = 0;
   ctx->Pack.SwapBytes = GL_FALSE;
   ctx->Pack.LsbFirst = GL_FALSE;
   ctx->Pack.ClientStorage = GL_FALSE;
   ctx->Pack.Invert = GL_FALSE;
#if FEATURE_EXT_pixel_buffer_object
   ctx->Pack.BufferObj = ctx->Array.NullBufferObj;
#endif
   ctx->Unpack.Alignment = 4;
   ctx->Unpack.RowLength = 0;
   ctx->Unpack.ImageHeight = 0;
   ctx->Unpack.SkipPixels = 0;
   ctx->Unpack.SkipRows = 0;
   ctx->Unpack.SkipImages = 0;
   ctx->Unpack.SwapBytes = GL_FALSE;
   ctx->Unpack.LsbFirst = GL_FALSE;
   ctx->Unpack.ClientStorage = GL_FALSE;
   ctx->Unpack.Invert = GL_FALSE;
#if FEATURE_EXT_pixel_buffer_object
   ctx->Unpack.BufferObj = ctx->Array.NullBufferObj;
#endif

   /*
    * _mesa_unpack_image() returns image data in this format.  When we
    * execute image commands (glDrawPixels(), glTexImage(), etc) from
    * within display lists we have to be sure to set the current
    * unpacking parameters to these values!
    */
   ctx->DefaultPacking.Alignment = 1;
   ctx->DefaultPacking.RowLength = 0;
   ctx->DefaultPacking.SkipPixels = 0;
   ctx->DefaultPacking.SkipRows = 0;
   ctx->DefaultPacking.ImageHeight = 0;
   ctx->DefaultPacking.SkipImages = 0;
   ctx->DefaultPacking.SwapBytes = GL_FALSE;
   ctx->DefaultPacking.LsbFirst = GL_FALSE;
   ctx->DefaultPacking.ClientStorage = GL_FALSE;
   ctx->DefaultPacking.Invert = GL_FALSE;
#if FEATURE_EXT_pixel_buffer_object
   ctx->DefaultPacking.BufferObj = ctx->Array.NullBufferObj;
#endif

   if (ctx->Visual.doubleBufferMode) {
      ctx->Pixel.ReadBuffer = GL_BACK;
   }
   else {
      ctx->Pixel.ReadBuffer = GL_FRONT;
   }

   /* Miscellaneous */
   ctx->_ImageTransferState = 0;
}
