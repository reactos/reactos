/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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

#include "mtypes.h"
#include "attrib.h"
#include "colormac.h"
#include "enums.h"
#include "formats.h"
#include "hash.h"
#include "imports.h"
#include "debug.h"
#include "get.h"
#include "pixelstore.h"
#include "readpix.h"
#include "texobj.h"


static const char *
tex_target_name(GLenum tgt)
{
   static const struct {
      GLenum target;
      const char *name;
   } tex_targets[] = {
      { GL_TEXTURE_1D, "GL_TEXTURE_1D" },
      { GL_TEXTURE_2D, "GL_TEXTURE_2D" },
      { GL_TEXTURE_3D, "GL_TEXTURE_3D" },
      { GL_TEXTURE_CUBE_MAP, "GL_TEXTURE_CUBE_MAP" },
      { GL_TEXTURE_RECTANGLE, "GL_TEXTURE_RECTANGLE" },
      { GL_TEXTURE_1D_ARRAY_EXT, "GL_TEXTURE_1D_ARRAY" },
      { GL_TEXTURE_2D_ARRAY_EXT, "GL_TEXTURE_2D_ARRAY" },
      { GL_TEXTURE_EXTERNAL_OES, "GL_TEXTURE_EXTERNAL_OES" }
   };
   GLuint i;
   for (i = 0; i < Elements(tex_targets); i++) {
      if (tex_targets[i].target == tgt)
         return tex_targets[i].name;
   }
   return "UNKNOWN TEX TARGET";
}


void
_mesa_print_state( const char *msg, GLuint state )
{
   _mesa_debug(NULL,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   msg,
	   state,
	   (state & _NEW_MODELVIEW)       ? "ctx->ModelView, " : "",
	   (state & _NEW_PROJECTION)      ? "ctx->Projection, " : "",
	   (state & _NEW_TEXTURE_MATRIX)  ? "ctx->TextureMatrix, " : "",
	   (state & _NEW_COLOR)           ? "ctx->Color, " : "",
	   (state & _NEW_DEPTH)           ? "ctx->Depth, " : "",
	   (state & _NEW_EVAL)            ? "ctx->Eval/EvalMap, " : "",
	   (state & _NEW_FOG)             ? "ctx->Fog, " : "",
	   (state & _NEW_HINT)            ? "ctx->Hint, " : "",
	   (state & _NEW_LIGHT)           ? "ctx->Light, " : "",
	   (state & _NEW_LINE)            ? "ctx->Line, " : "",
	   (state & _NEW_PIXEL)           ? "ctx->Pixel, " : "",
	   (state & _NEW_POINT)           ? "ctx->Point, " : "",
	   (state & _NEW_POLYGON)         ? "ctx->Polygon, " : "",
	   (state & _NEW_POLYGONSTIPPLE)  ? "ctx->PolygonStipple, " : "",
	   (state & _NEW_SCISSOR)         ? "ctx->Scissor, " : "",
	   (state & _NEW_STENCIL)         ? "ctx->Stencil, " : "",
	   (state & _NEW_TEXTURE)         ? "ctx->Texture, " : "",
	   (state & _NEW_TRANSFORM)       ? "ctx->Transform, " : "",
	   (state & _NEW_VIEWPORT)        ? "ctx->Viewport, " : "",
	   (state & _NEW_PACKUNPACK)      ? "ctx->Pack/Unpack, " : "",
	   (state & _NEW_ARRAY)           ? "ctx->Array, " : "",
	   (state & _NEW_RENDERMODE)      ? "ctx->RenderMode, " : "",
	   (state & _NEW_BUFFERS)         ? "ctx->Visual, ctx->DrawBuffer,, " : "");
}



void
_mesa_print_tri_caps( const char *name, GLuint flags )
{
   _mesa_debug(NULL,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   name,
	   flags,
	   (flags & DD_FLATSHADE)           ? "flat-shade, " : "",
	   (flags & DD_SEPARATE_SPECULAR)   ? "separate-specular, " : "",
	   (flags & DD_TRI_LIGHT_TWOSIDE)   ? "tri-light-twoside, " : "",
	   (flags & DD_TRI_TWOSTENCIL)      ? "tri-twostencil, " : "",
	   (flags & DD_TRI_UNFILLED)        ? "tri-unfilled, " : "",
	   (flags & DD_TRI_STIPPLE)         ? "tri-stipple, " : "",
	   (flags & DD_TRI_OFFSET)          ? "tri-offset, " : "",
	   (flags & DD_TRI_SMOOTH)          ? "tri-smooth, " : "",
	   (flags & DD_LINE_SMOOTH)         ? "line-smooth, " : "",
	   (flags & DD_LINE_STIPPLE)        ? "line-stipple, " : "",
	   (flags & DD_POINT_SMOOTH)        ? "point-smooth, " : "",
	   (flags & DD_POINT_ATTEN)         ? "point-atten, " : "",
	   (flags & DD_TRI_CULL_FRONT_BACK) ? "cull-all, " : ""
      );
}


/**
 * Print information about this Mesa version and build options.
 */
void _mesa_print_info( void )
{
   _mesa_debug(NULL, "Mesa GL_VERSION = %s\n",
	   (char *) _mesa_GetString(GL_VERSION));
   _mesa_debug(NULL, "Mesa GL_RENDERER = %s\n",
	   (char *) _mesa_GetString(GL_RENDERER));
   _mesa_debug(NULL, "Mesa GL_VENDOR = %s\n",
	   (char *) _mesa_GetString(GL_VENDOR));
   _mesa_debug(NULL, "Mesa GL_EXTENSIONS = %s\n",
	   (char *) _mesa_GetString(GL_EXTENSIONS));
#if defined(THREADS)
   _mesa_debug(NULL, "Mesa thread-safe: YES\n");
#else
   _mesa_debug(NULL, "Mesa thread-safe: NO\n");
#endif
#if defined(USE_X86_ASM)
   _mesa_debug(NULL, "Mesa x86-optimized: YES\n");
#else
   _mesa_debug(NULL, "Mesa x86-optimized: NO\n");
#endif
#if defined(USE_SPARC_ASM)
   _mesa_debug(NULL, "Mesa sparc-optimized: YES\n");
#else
   _mesa_debug(NULL, "Mesa sparc-optimized: NO\n");
#endif
}


/**
 * Set the debugging flags.
 *
 * \param debug debug string
 *
 * If compiled with debugging support then search for keywords in \p debug and
 * enables the verbose debug output of the respective feature.
 */
static void add_debug_flags( const char *debug )
{
#ifdef DEBUG
   struct debug_option {
      const char *name;
      GLbitfield flag;
   };
   static const struct debug_option debug_opt[] = {
      { "varray",    VERBOSE_VARRAY },
      { "tex",       VERBOSE_TEXTURE },
      { "mat",       VERBOSE_MATERIAL },
      { "pipe",      VERBOSE_PIPELINE },
      { "driver",    VERBOSE_DRIVER },
      { "state",     VERBOSE_STATE },
      { "api",       VERBOSE_API },
      { "list",      VERBOSE_DISPLAY_LIST },
      { "lighting",  VERBOSE_LIGHTING },
      { "disassem",  VERBOSE_DISASSEM },
      { "draw",      VERBOSE_DRAW },
      { "swap",      VERBOSE_SWAPBUFFERS }
   };
   GLuint i;

   MESA_VERBOSE = 0x0;
   for (i = 0; i < Elements(debug_opt); i++) {
      if (strstr(debug, debug_opt[i].name) || strcmp(debug, "all") == 0)
         MESA_VERBOSE |= debug_opt[i].flag;
   }

   /* Debug flag:
    */
   if (strstr(debug, "flush"))
      MESA_DEBUG_FLAGS |= DEBUG_ALWAYS_FLUSH;

#else
   (void) debug;
#endif
}


void 
_mesa_init_debug( struct gl_context *ctx )
{
   char *c;
   c = _mesa_getenv("MESA_DEBUG");
   if (c)
      add_debug_flags(c);

   c = _mesa_getenv("MESA_VERBOSE");
   if (c)
      add_debug_flags(c);
}


/*
 * Write ppm file
 */
static void
write_ppm(const char *filename, const GLubyte *buffer, int width, int height,
          int comps, int rcomp, int gcomp, int bcomp, GLboolean invert)
{
   FILE *f = fopen( filename, "w" );
   if (f) {
      int x, y;
      const GLubyte *ptr = buffer;
      fprintf(f,"P6\n");
      fprintf(f,"# ppm-file created by osdemo.c\n");
      fprintf(f,"%i %i\n", width,height);
      fprintf(f,"255\n");
      fclose(f);
      f = fopen( filename, "ab" );  /* reopen in binary append mode */
      for (y=0; y < height; y++) {
         for (x = 0; x < width; x++) {
            int yy = invert ? (height - 1 - y) : y;
            int i = (yy * width + x) * comps;
            fputc(ptr[i+rcomp], f); /* write red */
            fputc(ptr[i+gcomp], f); /* write green */
            fputc(ptr[i+bcomp], f); /* write blue */
         }
      }
      fclose(f);
   }
   else {
      fprintf(stderr, "Unable to create %s in write_ppm()\n", filename);
   }
}


/**
 * Write a texture image to a ppm file.
 * \param face  cube face in [0,5]
 * \param level  mipmap level
 */
static void
write_texture_image(struct gl_texture_object *texObj,
                    GLuint face, GLuint level)
{
   struct gl_texture_image *img = texObj->Image[face][level];
   if (img) {
      GET_CURRENT_CONTEXT(ctx);
      struct gl_pixelstore_attrib store;
      GLubyte *buffer;
      char s[100];

      buffer = (GLubyte *) malloc(img->Width * img->Height
                                        * img->Depth * 4);

      store = ctx->Pack; /* save */
      ctx->Pack = ctx->DefaultPacking;

      ctx->Driver.GetTexImage(ctx, GL_RGBA, GL_UNSIGNED_BYTE, buffer, img);

      /* make filename */
      _mesa_snprintf(s, sizeof(s), "/tmp/tex%u.l%u.f%u.ppm", texObj->Name, level, face);

      printf("  Writing image level %u to %s\n", level, s);
      write_ppm(s, buffer, img->Width, img->Height, 4, 0, 1, 2, GL_FALSE);

      ctx->Pack = store; /* restore */

      free(buffer);
   }
}


/**
 * Write renderbuffer image to a ppm file.
 */
void
_mesa_write_renderbuffer_image(const struct gl_renderbuffer *rb)
{
   GET_CURRENT_CONTEXT(ctx);
   GLubyte *buffer;
   char s[100];
   GLenum format, type;

   if (rb->_BaseFormat == GL_RGB || 
       rb->_BaseFormat == GL_RGBA) {
      format = GL_RGBA;
      type = GL_UNSIGNED_BYTE;
   }
   else if (rb->_BaseFormat == GL_DEPTH_STENCIL) {
      format = GL_DEPTH_STENCIL;
      type = GL_UNSIGNED_INT_24_8;
   }
   else {
      _mesa_debug(NULL,
                  "Unsupported BaseFormat 0x%x in "
                  "_mesa_write_renderbuffer_image()\n",
                  rb->_BaseFormat);
      return;
   }

   buffer = (GLubyte *) malloc(rb->Width * rb->Height * 4);

   ctx->Driver.ReadPixels(ctx, 0, 0, rb->Width, rb->Height,
                          format, type, &ctx->DefaultPacking, buffer);

   /* make filename */
   _mesa_snprintf(s, sizeof(s), "/tmp/renderbuffer%u.ppm", rb->Name);
   _mesa_snprintf(s, sizeof(s), "C:\\renderbuffer%u.ppm", rb->Name);

   printf("  Writing renderbuffer image to %s\n", s);

   _mesa_debug(NULL, "  Writing renderbuffer image to %s\n", s);

   write_ppm(s, buffer, rb->Width, rb->Height, 4, 0, 1, 2, GL_TRUE);

   free(buffer);
}


/** How many texture images (mipmap levels, faces) to write to files */
#define WRITE_NONE 0
#define WRITE_ONE  1
#define WRITE_ALL  2

static GLuint WriteImages;


static void
dump_texture(struct gl_texture_object *texObj, GLuint writeImages)
{
   const GLuint numFaces = texObj->Target == GL_TEXTURE_CUBE_MAP ? 6 : 1;
   GLboolean written = GL_FALSE;
   GLuint i, j;

   printf("Texture %u\n", texObj->Name);
   printf("  Target %s\n", tex_target_name(texObj->Target));
   for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
      for (j = 0; j < numFaces; j++) {
         struct gl_texture_image *texImg = texObj->Image[j][i];
         if (texImg) {
            printf("  Face %u level %u: %d x %d x %d, format %s\n",
		   j, i,
		   texImg->Width, texImg->Height, texImg->Depth,
		   _mesa_get_format_name(texImg->TexFormat));
            if (writeImages == WRITE_ALL ||
                (writeImages == WRITE_ONE && !written)) {
               write_texture_image(texObj, j, i);
               written = GL_TRUE;
            }
         }
      }
   }
}


/**
 * Dump a single texture.
 */
void
_mesa_dump_texture(GLuint texture, GLuint writeImages)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_object *texObj = _mesa_lookup_texture(ctx, texture);
   if (texObj) {
      dump_texture(texObj, writeImages);
   }
}


static void
dump_texture_cb(GLuint id, void *data, void *userData)
{
   struct gl_texture_object *texObj = (struct gl_texture_object *) data;
   (void) userData;
   dump_texture(texObj, WriteImages);
}


/**
 * Print basic info about all texture objext to stdout.
 * If dumpImages is true, write PPM of level[0] image to a file.
 */
void
_mesa_dump_textures(GLuint writeImages)
{
   GET_CURRENT_CONTEXT(ctx);
   WriteImages = writeImages;
   _mesa_HashWalk(ctx->Shared->TexObjects, dump_texture_cb, ctx);
}


static void
dump_renderbuffer(const struct gl_renderbuffer *rb, GLboolean writeImage)
{
   printf("Renderbuffer %u: %u x %u  IntFormat = %s\n",
	  rb->Name, rb->Width, rb->Height,
	  _mesa_lookup_enum_by_nr(rb->InternalFormat));
   if (writeImage) {
      _mesa_write_renderbuffer_image(rb);
   }
}


static void
dump_renderbuffer_cb(GLuint id, void *data, void *userData)
{
   const struct gl_renderbuffer *rb = (const struct gl_renderbuffer *) data;
   (void) userData;
   dump_renderbuffer(rb, WriteImages);
}


/**
 * Print basic info about all renderbuffers to stdout.
 * If dumpImages is true, write PPM of level[0] image to a file.
 */
void
_mesa_dump_renderbuffers(GLboolean writeImages)
{
   GET_CURRENT_CONTEXT(ctx);
   WriteImages = writeImages;
   _mesa_HashWalk(ctx->Shared->RenderBuffers, dump_renderbuffer_cb, ctx);
}



void
_mesa_dump_color_buffer(const char *filename)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLuint w = ctx->DrawBuffer->Width;
   const GLuint h = ctx->DrawBuffer->Height;
   GLubyte *buf;

   buf = (GLubyte *) malloc(w * h * 4);

   _mesa_PushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
   _mesa_PixelStorei(GL_PACK_ALIGNMENT, 1);
   _mesa_PixelStorei(GL_PACK_INVERT_MESA, GL_TRUE);

   _mesa_ReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);

   printf("ReadBuffer %p 0x%x  DrawBuffer %p 0x%x\n",
	  (void *) ctx->ReadBuffer->_ColorReadBuffer,
	  ctx->ReadBuffer->ColorReadBuffer,
	  (void *) ctx->DrawBuffer->_ColorDrawBuffers[0],
	  ctx->DrawBuffer->ColorDrawBuffer[0]);
   printf("Writing %d x %d color buffer to %s\n", w, h, filename);
   write_ppm(filename, buf, w, h, 4, 0, 1, 2, GL_TRUE);

   _mesa_PopClientAttrib();

   free(buf);
}


void
_mesa_dump_depth_buffer(const char *filename)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLuint w = ctx->DrawBuffer->Width;
   const GLuint h = ctx->DrawBuffer->Height;
   GLuint *buf;
   GLubyte *buf2;
   GLuint i;

   buf = (GLuint *) malloc(w * h * 4);  /* 4 bpp */
   buf2 = (GLubyte *) malloc(w * h * 3); /* 3 bpp */

   _mesa_PushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
   _mesa_PixelStorei(GL_PACK_ALIGNMENT, 1);
   _mesa_PixelStorei(GL_PACK_INVERT_MESA, GL_TRUE);

   _mesa_ReadPixels(0, 0, w, h, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, buf);

   /* spread 24 bits of Z across R, G, B */
   for (i = 0; i < w * h; i++) {
      buf2[i*3+0] = (buf[i] >> 24) & 0xff;
      buf2[i*3+1] = (buf[i] >> 16) & 0xff;
      buf2[i*3+2] = (buf[i] >>  8) & 0xff;
   }

   printf("Writing %d x %d depth buffer to %s\n", w, h, filename);
   write_ppm(filename, buf2, w, h, 3, 0, 1, 2, GL_TRUE);

   _mesa_PopClientAttrib();

   free(buf);
   free(buf2);
}


void
_mesa_dump_stencil_buffer(const char *filename)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLuint w = ctx->DrawBuffer->Width;
   const GLuint h = ctx->DrawBuffer->Height;
   GLubyte *buf;
   GLubyte *buf2;
   GLuint i;

   buf = (GLubyte *) malloc(w * h);  /* 1 bpp */
   buf2 = (GLubyte *) malloc(w * h * 3); /* 3 bpp */

   _mesa_PushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
   _mesa_PixelStorei(GL_PACK_ALIGNMENT, 1);
   _mesa_PixelStorei(GL_PACK_INVERT_MESA, GL_TRUE);

   _mesa_ReadPixels(0, 0, w, h, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, buf);

   for (i = 0; i < w * h; i++) {
      buf2[i*3+0] = buf[i];
      buf2[i*3+1] = (buf[i] & 127) * 2;
      buf2[i*3+2] = (buf[i] - 128) * 2;
   }

   printf("Writing %d x %d stencil buffer to %s\n", w, h, filename);
   write_ppm(filename, buf2, w, h, 3, 0, 1, 2, GL_TRUE);

   _mesa_PopClientAttrib();

   free(buf);
   free(buf2);
}


void
_mesa_dump_image(const char *filename, const void *image, GLuint w, GLuint h,
                 GLenum format, GLenum type)
{
   GLboolean invert = GL_TRUE;

   if (format == GL_RGBA && type == GL_UNSIGNED_BYTE) {
      write_ppm(filename, image, w, h, 4, 0, 1, 2, invert);
   }
   else if (format == GL_BGRA && type == GL_UNSIGNED_BYTE) {
      write_ppm(filename, image, w, h, 4, 2, 1, 0, invert);
   }
   else if (format == GL_LUMINANCE_ALPHA && type == GL_UNSIGNED_BYTE) {
      write_ppm(filename, image, w, h, 2, 1, 0, 0, invert);
   }
   else {
      _mesa_problem(NULL, "Unsupported format/type in _mesa_dump_image()");
   }
}


/**
 * Quick and dirty function to "print" a texture to stdout.
 */
void
_mesa_print_texture(struct gl_context *ctx, struct gl_texture_image *img)
{
   const GLint slice = 0;
   GLint srcRowStride;
   GLuint i, j, c;
   GLubyte *data;

   ctx->Driver.MapTextureImage(ctx, img, slice,
                               0, 0, img->Width, img->Height, GL_MAP_READ_BIT,
                               &data, &srcRowStride);

   if (!data) {
      printf("No texture data\n");
   }
   else {
      /* XXX add more formats or make into a new format utility function */
      switch (img->TexFormat) {
         case MESA_FORMAT_A8:
         case MESA_FORMAT_L8:
         case MESA_FORMAT_I8:
            c = 1;
            break;
         case MESA_FORMAT_AL88:
         case MESA_FORMAT_AL88_REV:
            c = 2;
            break;
         case MESA_FORMAT_RGB888:
         case MESA_FORMAT_BGR888:
            c = 3;
            break;
         case MESA_FORMAT_RGBA8888:
         case MESA_FORMAT_ARGB8888:
            c = 4;
            break;
         default:
            _mesa_problem(NULL, "error in PrintTexture\n");
            return;
      }

      for (i = 0; i < img->Height; i++) {
         for (j = 0; j < img->Width; j++) {
            if (c==1)
               printf("%02x  ", data[0]);
            else if (c==2)
               printf("%02x%02x  ", data[0], data[1]);
            else if (c==3)
               printf("%02x%02x%02x  ", data[0], data[1], data[2]);
            else if (c==4)
               printf("%02x%02x%02x%02x  ", data[0], data[1], data[2], data[3]);
            data += (srcRowStride - img->Width) * c;
         }
         /* XXX use img->ImageStride here */
         printf("\n");

      }
   }

   ctx->Driver.UnmapTextureImage(ctx, img, slice);
}
