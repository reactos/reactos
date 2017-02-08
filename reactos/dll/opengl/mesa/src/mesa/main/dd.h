/**
 * \file dd.h
 * Device driver interfaces.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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


#ifndef DD_INCLUDED
#define DD_INCLUDED

/* THIS FILE ONLY INCLUDED BY mtypes.h !!!!! */

#include "glheader.h"

struct gl_buffer_object;
struct gl_context;
struct gl_display_list;
struct gl_framebuffer;
struct gl_pixelstore_attrib;
struct gl_program;
struct gl_renderbuffer;
struct gl_renderbuffer_attachment;
struct gl_shader;
struct gl_shader_program;
struct gl_texture_image;
struct gl_texture_object;

/* GL_ARB_vertex_buffer_object */
/* Modifies GL_MAP_UNSYNCHRONIZED_BIT to allow driver to fail (return
 * NULL) if buffer is unavailable for immediate mapping.
 *
 * Does GL_MAP_INVALIDATE_RANGE_BIT do this?  It seems so, but it
 * would require more book-keeping in the driver than seems necessary
 * at this point.
 *
 * Does GL_MAP_INVALDIATE_BUFFER_BIT do this?  Not really -- we don't
 * want to provoke the driver to throw away the old storage, we will
 * respect the contents of already referenced data.
 */
#define MESA_MAP_NOWAIT_BIT       0x0040


/**
 * Device driver function table.
 * Core Mesa uses these function pointers to call into device drivers.
 * Most of these functions directly correspond to OpenGL state commands.
 * Core Mesa will call these functions after error checking has been done
 * so that the drivers don't have to worry about error testing.
 *
 * Vertex transformation/clipping/lighting is patched into the T&L module.
 * Rasterization functions are patched into the swrast module.
 *
 * Note: when new functions are added here, the drivers/common/driverfuncs.c
 * file should be updated too!!!
 */
struct dd_function_table {
   /**
    * Return a string as needed by glGetString().
    * Only the GL_RENDERER query must be implemented.  Otherwise, NULL can be
    * returned.
    */
   const GLubyte * (*GetString)( struct gl_context *ctx, GLenum name );

   /**
    * Notify the driver after Mesa has made some internal state changes.  
    *
    * This is in addition to any state change callbacks Mesa may already have
    * made.
    */
   void (*UpdateState)( struct gl_context *ctx, GLbitfield new_state );

   /**
    * Get the width and height of the named buffer/window.
    *
    * Mesa uses this to determine when the driver's window size has changed.
    * XXX OBSOLETE: this function will be removed in the future.
    */
   void (*GetBufferSize)( struct gl_framebuffer *buffer,
                          GLuint *width, GLuint *height );

   /**
    * Resize the given framebuffer to the given size.
    * XXX OBSOLETE: this function will be removed in the future.
    */
   void (*ResizeBuffers)( struct gl_context *ctx, struct gl_framebuffer *fb,
                          GLuint width, GLuint height);

   /**
    * Called whenever an error is generated.  
    * __struct gl_contextRec::ErrorValue contains the error value.
    */
   void (*Error)( struct gl_context *ctx );

   /**
    * This is called whenever glFinish() is called.
    */
   void (*Finish)( struct gl_context *ctx );

   /**
    * This is called whenever glFlush() is called.
    */
   void (*Flush)( struct gl_context *ctx );

   /**
    * Clear the color/depth/stencil/accum buffer(s).
    * \param buffers  a bitmask of BUFFER_BIT_* flags indicating which
    *                 renderbuffers need to be cleared.
    */
   void (*Clear)( struct gl_context *ctx, GLbitfield buffers );

   /**
    * Execute glAccum command.
    */
   void (*Accum)( struct gl_context *ctx, GLenum op, GLfloat value );


   /**
    * Execute glRasterPos, updating the ctx->Current.Raster fields
    */
   void (*RasterPos)( struct gl_context *ctx, const GLfloat v[4] );

   /**
    * \name Image-related functions
    */
   /*@{*/

   /**
    * Called by glDrawPixels().
    * \p unpack describes how to unpack the source image data.
    */
   void (*DrawPixels)( struct gl_context *ctx,
		       GLint x, GLint y, GLsizei width, GLsizei height,
		       GLenum format, GLenum type,
		       const struct gl_pixelstore_attrib *unpack,
		       const GLvoid *pixels );

   /**
    * Called by glReadPixels().
    */
   void (*ReadPixels)( struct gl_context *ctx,
		       GLint x, GLint y, GLsizei width, GLsizei height,
		       GLenum format, GLenum type,
		       const struct gl_pixelstore_attrib *unpack,
		       GLvoid *dest );

   /**
    * Called by glCopyPixels().  
    */
   void (*CopyPixels)( struct gl_context *ctx, GLint srcx, GLint srcy,
                       GLsizei width, GLsizei height,
                       GLint dstx, GLint dsty, GLenum type );

   /**
    * Called by glBitmap().  
    */
   void (*Bitmap)( struct gl_context *ctx,
		   GLint x, GLint y, GLsizei width, GLsizei height,
		   const struct gl_pixelstore_attrib *unpack,
		   const GLubyte *bitmap );
   /*@}*/

   
   /**
    * \name Texture image functions
    */
   /*@{*/

   /**
    * Choose actual hardware texture format given the user-provided source
    * image format and type and the desired internal format.  In some
    * cases, srcFormat and srcType can be GL_NONE.
    * Called by glTexImage(), etc.
    */
   gl_format (*ChooseTextureFormat)( struct gl_context *ctx, GLint internalFormat,
                                     GLenum srcFormat, GLenum srcType );

   /**
    * Called by glTexImage1D().  Simply copy the source texture data into the
    * destination texture memory.  The gl_texture_image fields, etc. will be
    * fully initialized.
    * The parameters are the same as glTexImage1D(), plus:
    * \param packing describes how to unpack the source data.
    * \param texImage is the destination texture image.
    */
   void (*TexImage1D)(struct gl_context *ctx,
                      struct gl_texture_image *texImage,
                      GLint internalFormat,
                      GLint width, GLint border,
                      GLenum format, GLenum type, const GLvoid *pixels,
                      const struct gl_pixelstore_attrib *packing);

   /**
    * Called by glTexImage2D().
    * 
    * \sa dd_function_table::TexImage1D.
    */
   void (*TexImage2D)(struct gl_context *ctx,
                      struct gl_texture_image *texImage,
                      GLint internalFormat,
                      GLint width, GLint height, GLint border,
                      GLenum format, GLenum type, const GLvoid *pixels,
                      const struct gl_pixelstore_attrib *packing);
   
   /**
    * Called by glTexImage3D().
    * 
    * \sa dd_function_table::TexImage1D.
    */
   void (*TexImage3D)(struct gl_context *ctx,
                      struct gl_texture_image *texImage,
                      GLint internalFormat,
                      GLint width, GLint height, GLint depth, GLint border,
                      GLenum format, GLenum type, const GLvoid *pixels,
                      const struct gl_pixelstore_attrib *packing);

   /**
    * Called by glTexSubImage1D().  Replace a subset of the target texture
    * with new texel data.
    * \sa dd_function_table::TexImage1D.
    */
   void (*TexSubImage1D)(struct gl_context *ctx,
                         struct gl_texture_image *texImage,
                         GLint xoffset, GLsizei width,
                         GLenum format, GLenum type,
                         const GLvoid *pixels,
                         const struct gl_pixelstore_attrib *packing);
   
   /**
    * Called by glTexSubImage2D().
    *
    * \sa dd_function_table::TexSubImage1D.
    */
   void (*TexSubImage2D)(struct gl_context *ctx,
                         struct gl_texture_image *texImage,
                         GLint xoffset, GLint yoffset,
                         GLsizei width, GLsizei height,
                         GLenum format, GLenum type,
                         const GLvoid *pixels,
                         const struct gl_pixelstore_attrib *packing);
   
   /**
    * Called by glTexSubImage3D().
    *
    * \sa dd_function_table::TexSubImage1D.
    */
   void (*TexSubImage3D)(struct gl_context *ctx,
                         struct gl_texture_image *texImage,
                         GLint xoffset, GLint yoffset, GLint zoffset,
                         GLsizei width, GLsizei height, GLint depth,
                         GLenum format, GLenum type,
                         const GLvoid *pixels,
                         const struct gl_pixelstore_attrib *packing);


   /**
    * Called by glGetTexImage().
    */
   void (*GetTexImage)( struct gl_context *ctx,
                        GLenum format, GLenum type, GLvoid *pixels,
                        struct gl_texture_image *texImage );

   /**
    * Called by glCopyTexSubImage1D() and glCopyTexImage1D().
    */
   void (*CopyTexSubImage1D)(struct gl_context *ctx,
                             struct gl_texture_image *texImage,
                             GLint xoffset,
                             struct gl_renderbuffer *rb,
                             GLint x, GLint y, GLsizei width);

   /**
    * Called by glCopyTexSubImage2D() and glCopyTexImage2D().
    */
   void (*CopyTexSubImage2D)(struct gl_context *ctx,
                             struct gl_texture_image *texImage,
                             GLint xoffset, GLint yoffset,
                             struct gl_renderbuffer *rb,
                             GLint x, GLint y,
                             GLsizei width, GLsizei height);

   /**
    * Called by glCopyTexSubImage3D() and glCopyTexImage3D().
    */
   void (*CopyTexSubImage3D)(struct gl_context *ctx,
                             struct gl_texture_image *texImage,
                             GLint xoffset, GLint yoffset, GLint zoffset,
                             struct gl_renderbuffer *rb,
                             GLint x, GLint y,
                             GLsizei width, GLsizei height);

   /**
    * Called by glGenerateMipmap() or when GL_GENERATE_MIPMAP_SGIS is enabled.
    */
   void (*GenerateMipmap)(struct gl_context *ctx, GLenum target,
                          struct gl_texture_object *texObj);

   /**
    * Called by glTexImage[123]D when user specifies a proxy texture
    * target.  
    *
    * \return GL_TRUE if the proxy test passes, or GL_FALSE if the test fails.
    */
   GLboolean (*TestProxyTexImage)(struct gl_context *ctx, GLenum target,
                                  GLint level, GLint internalFormat,
                                  GLenum format, GLenum type,
                                  GLint width, GLint height,
                                  GLint depth, GLint border);
   /*@}*/

   
   /**
    * \name Compressed texture functions
    */
   /*@{*/

   /**
    * Called by glCompressedTexImage1D().
    * The parameters are the same as for glCompressedTexImage1D(), plus a
    * pointer to the destination texure image.
    */
   void (*CompressedTexImage1D)(struct gl_context *ctx,
                                struct gl_texture_image *texImage,
                                GLint internalFormat,
                                GLsizei width, GLint border,
                                GLsizei imageSize, const GLvoid *data);
   /**
    * Called by glCompressedTexImage2D().
    *
    * \sa dd_function_table::CompressedTexImage1D.
    */
   void (*CompressedTexImage2D)(struct gl_context *ctx,
                                struct gl_texture_image *texImage,
                                GLint internalFormat,
                                GLsizei width, GLsizei height, GLint border,
                                GLsizei imageSize, const GLvoid *data);

   /**
    * Called by glCompressedTexImage3D().
    *
    * \sa dd_function_table::CompressedTexImage3D.
    */
   void (*CompressedTexImage3D)(struct gl_context *ctx,
                                struct gl_texture_image *texImage,
                                GLint internalFormat,
                                GLsizei width, GLsizei height, GLsizei depth,
                                GLint border,
                                GLsizei imageSize, const GLvoid *data);

   /**
    * Called by glCompressedTexSubImage1D().
    */
   void (*CompressedTexSubImage1D)(struct gl_context *ctx,
                                   struct gl_texture_image *texImage,
                                   GLint xoffset, GLsizei width,
                                   GLenum format,
                                   GLsizei imageSize, const GLvoid *data);

   /**
    * Called by glCompressedTexSubImage2D().
    */
   void (*CompressedTexSubImage2D)(struct gl_context *ctx,
                                   struct gl_texture_image *texImage,
                                   GLint xoffset, GLint yoffset,
                                   GLsizei width, GLint height,
                                   GLenum format,
                                   GLsizei imageSize, const GLvoid *data);

   /**
    * Called by glCompressedTexSubImage3D().
    */
   void (*CompressedTexSubImage3D)(struct gl_context *ctx,
                                   struct gl_texture_image *texImage,
                                   GLint xoffset, GLint yoffset, GLint zoffset,
                                   GLsizei width, GLint height, GLint depth,
                                   GLenum format,
                                   GLsizei imageSize, const GLvoid *data);

   /**
    * Called by glGetCompressedTexImage.
    */
   void (*GetCompressedTexImage)(struct gl_context *ctx,
                                 struct gl_texture_image *texImage,
                                 GLvoid *data);
   /*@}*/

   /**
    * \name Texture object / image functions
    */
   /*@{*/

   /**
    * Called by glBindTexture().
    */
   void (*BindTexture)( struct gl_context *ctx, GLenum target,
                        struct gl_texture_object *tObj );

   /**
    * Called to allocate a new texture object.  Drivers will usually
    * allocate/return a subclass of gl_texture_object.
    */
   struct gl_texture_object * (*NewTextureObject)(struct gl_context *ctx,
                                                  GLuint name, GLenum target);
   /**
    * Called to delete/free a texture object.  Drivers should free the
    * object and any image data it contains.
    */
   void (*DeleteTexture)(struct gl_context *ctx,
                         struct gl_texture_object *texObj);

   /** Called to allocate a new texture image object. */
   struct gl_texture_image * (*NewTextureImage)(struct gl_context *ctx);

   /** Called to free a texture image object returned by NewTextureImage() */
   void (*DeleteTextureImage)(struct gl_context *ctx,
                              struct gl_texture_image *);

   /** Called to allocate memory for a single texture image */
   GLboolean (*AllocTextureImageBuffer)(struct gl_context *ctx,
                                        struct gl_texture_image *texImage,
                                        gl_format format, GLsizei width,
                                        GLsizei height, GLsizei depth);

   /** Free the memory for a single texture image */
   void (*FreeTextureImageBuffer)(struct gl_context *ctx,
                                  struct gl_texture_image *texImage);

   /** Map a slice of a texture image into user space.
    * Note: for GL_TEXTURE_1D_ARRAY, height must be 1, y must be 0 and slice
    * indicates the 1D array index.
    * \param texImage  the texture image
    * \param slice  the 3D image slice or array texture slice
    * \param x, y, w, h  region of interest
    * \param mode  bitmask of GL_MAP_READ_BIT, GL_MAP_WRITE_BIT and
    *              GL_MAP_INVALIDATE_RANGE_BIT (if writing)
    * \param mapOut  returns start of mapping of region of interest
    * \param rowStrideOut  returns row stride (in bytes)
    */
   void (*MapTextureImage)(struct gl_context *ctx,
			   struct gl_texture_image *texImage,
			   GLuint slice,
			   GLuint x, GLuint y, GLuint w, GLuint h,
			   GLbitfield mode,
			   GLubyte **mapOut, GLint *rowStrideOut);

   void (*UnmapTextureImage)(struct gl_context *ctx,
			     struct gl_texture_image *texImage,
			     GLuint slice);

   /** For GL_ARB_texture_storage.  Allocate memory for whole mipmap stack.
    * All the gl_texture_images in the texture object will have their
    * dimensions, format, etc. initialized already.
    */
   GLboolean (*AllocTextureStorage)(struct gl_context *ctx,
                                    struct gl_texture_object *texObj,
                                    GLsizei levels, GLsizei width,
                                    GLsizei height, GLsizei depth);

   /**
    * Map a renderbuffer into user space.
    * \param mode  bitmask of GL_MAP_READ_BIT, GL_MAP_WRITE_BIT and
    *              GL_MAP_INVALIDATE_RANGE_BIT (if writing)
    */
   void (*MapRenderbuffer)(struct gl_context *ctx,
			   struct gl_renderbuffer *rb,
			   GLuint x, GLuint y, GLuint w, GLuint h,
			   GLbitfield mode,
			   GLubyte **mapOut, GLint *rowStrideOut);

   void (*UnmapRenderbuffer)(struct gl_context *ctx,
			     struct gl_renderbuffer *rb);

   /*@}*/


   /**
    * \name Vertex/fragment program functions
    */
   /*@{*/
   /** Bind a vertex/fragment program */
   void (*BindProgram)(struct gl_context *ctx, GLenum target, struct gl_program *prog);
   /** Allocate a new program */
   struct gl_program * (*NewProgram)(struct gl_context *ctx, GLenum target, GLuint id);
   /** Delete a program */
   void (*DeleteProgram)(struct gl_context *ctx, struct gl_program *prog);   
   /**
    * Notify driver that a program string (and GPU code) has been specified
    * or modified.  Return GL_TRUE or GL_FALSE to indicate if the program is
    * supported by the driver.
    */
   GLboolean (*ProgramStringNotify)(struct gl_context *ctx, GLenum target, 
                                    struct gl_program *prog);

   /** Query if program can be loaded onto hardware */
   GLboolean (*IsProgramNative)(struct gl_context *ctx, GLenum target, 
				struct gl_program *prog);
   
   /*@}*/

   /**
    * \name GLSL shader/program functions.
    */
   /*@{*/
   /**
    * Called when a shader program is linked.
    *
    * This gives drivers an opportunity to clone the IR and make their
    * own transformations on it for the purposes of code generation.
    */
   GLboolean (*LinkShader)(struct gl_context *ctx, struct gl_shader_program *shader);
   /*@}*/

   /**
    * \name State-changing functions.
    *
    * \note drawing functions are above.
    *
    * These functions are called by their corresponding OpenGL API functions.
    * They are \e also called by the gl_PopAttrib() function!!!
    * May add more functions like these to the device driver in the future.
    */
   /*@{*/
   /** Specify the alpha test function */
   void (*AlphaFunc)(struct gl_context *ctx, GLenum func, GLfloat ref);
   /** Set the blend color */
   void (*BlendColor)(struct gl_context *ctx, const GLfloat color[4]);
   /** Set the blend equation */
   void (*BlendEquationSeparate)(struct gl_context *ctx, GLenum modeRGB, GLenum modeA);
   void (*BlendEquationSeparatei)(struct gl_context *ctx, GLuint buffer,
                                  GLenum modeRGB, GLenum modeA);
   /** Specify pixel arithmetic */
   void (*BlendFuncSeparate)(struct gl_context *ctx,
                             GLenum sfactorRGB, GLenum dfactorRGB,
                             GLenum sfactorA, GLenum dfactorA);
   void (*BlendFuncSeparatei)(struct gl_context *ctx, GLuint buffer,
                              GLenum sfactorRGB, GLenum dfactorRGB,
                              GLenum sfactorA, GLenum dfactorA);
   /** Specify clear values for the color buffers */
   void (*ClearColor)(struct gl_context *ctx,
                      const union gl_color_union color);
   /** Specify the clear value for the depth buffer */
   void (*ClearDepth)(struct gl_context *ctx, GLclampd d);
   /** Specify the clear value for the stencil buffer */
   void (*ClearStencil)(struct gl_context *ctx, GLint s);
   /** Specify a plane against which all geometry is clipped */
   void (*ClipPlane)(struct gl_context *ctx, GLenum plane, const GLfloat *equation );
   /** Enable and disable writing of frame buffer color components */
   void (*ColorMask)(struct gl_context *ctx, GLboolean rmask, GLboolean gmask,
                     GLboolean bmask, GLboolean amask );
   void (*ColorMaskIndexed)(struct gl_context *ctx, GLuint buf, GLboolean rmask,
                            GLboolean gmask, GLboolean bmask, GLboolean amask);
   /** Cause a material color to track the current color */
   void (*ColorMaterial)(struct gl_context *ctx, GLenum face, GLenum mode);
   /** Specify whether front- or back-facing facets can be culled */
   void (*CullFace)(struct gl_context *ctx, GLenum mode);
   /** Define front- and back-facing polygons */
   void (*FrontFace)(struct gl_context *ctx, GLenum mode);
   /** Specify the value used for depth buffer comparisons */
   void (*DepthFunc)(struct gl_context *ctx, GLenum func);
   /** Enable or disable writing into the depth buffer */
   void (*DepthMask)(struct gl_context *ctx, GLboolean flag);
   /** Specify mapping of depth values from NDC to window coordinates */
   void (*DepthRange)(struct gl_context *ctx, GLclampd nearval, GLclampd farval);
   /** Specify the current buffer for writing */
   void (*DrawBuffer)( struct gl_context *ctx, GLenum buffer );
   /** Specify the buffers for writing for fragment programs*/
   void (*DrawBuffers)( struct gl_context *ctx, GLsizei n, const GLenum *buffers );
   /** Enable or disable server-side gl capabilities */
   void (*Enable)(struct gl_context *ctx, GLenum cap, GLboolean state);
   /** Specify fog parameters */
   void (*Fogfv)(struct gl_context *ctx, GLenum pname, const GLfloat *params);
   /** Specify implementation-specific hints */
   void (*Hint)(struct gl_context *ctx, GLenum target, GLenum mode);
   /** Set light source parameters.
    * Note: for GL_POSITION and GL_SPOT_DIRECTION, params will have already
    * been transformed to eye-space.
    */
   void (*Lightfv)(struct gl_context *ctx, GLenum light,
		   GLenum pname, const GLfloat *params );
   /** Set the lighting model parameters */
   void (*LightModelfv)(struct gl_context *ctx, GLenum pname, const GLfloat *params);
   /** Specify the line stipple pattern */
   void (*LineStipple)(struct gl_context *ctx, GLint factor, GLushort pattern );
   /** Specify the width of rasterized lines */
   void (*LineWidth)(struct gl_context *ctx, GLfloat width);
   /** Specify a logical pixel operation for color index rendering */
   void (*LogicOpcode)(struct gl_context *ctx, GLenum opcode);
   void (*PointParameterfv)(struct gl_context *ctx, GLenum pname,
                            const GLfloat *params);
   /** Specify the diameter of rasterized points */
   void (*PointSize)(struct gl_context *ctx, GLfloat size);
   /** Select a polygon rasterization mode */
   void (*PolygonMode)(struct gl_context *ctx, GLenum face, GLenum mode);
   /** Set the scale and units used to calculate depth values */
   void (*PolygonOffset)(struct gl_context *ctx, GLfloat factor, GLfloat units);
   /** Set the polygon stippling pattern */
   void (*PolygonStipple)(struct gl_context *ctx, const GLubyte *mask );
   /* Specifies the current buffer for reading */
   void (*ReadBuffer)( struct gl_context *ctx, GLenum buffer );
   /** Set rasterization mode */
   void (*RenderMode)(struct gl_context *ctx, GLenum mode );
   /** Define the scissor box */
   void (*Scissor)(struct gl_context *ctx, GLint x, GLint y, GLsizei w, GLsizei h);
   /** Select flat or smooth shading */
   void (*ShadeModel)(struct gl_context *ctx, GLenum mode);
   /** OpenGL 2.0 two-sided StencilFunc */
   void (*StencilFuncSeparate)(struct gl_context *ctx, GLenum face, GLenum func,
                               GLint ref, GLuint mask);
   /** OpenGL 2.0 two-sided StencilMask */
   void (*StencilMaskSeparate)(struct gl_context *ctx, GLenum face, GLuint mask);
   /** OpenGL 2.0 two-sided StencilOp */
   void (*StencilOpSeparate)(struct gl_context *ctx, GLenum face, GLenum fail,
                             GLenum zfail, GLenum zpass);
   /** Control the generation of texture coordinates */
   void (*TexGen)(struct gl_context *ctx, GLenum coord, GLenum pname,
		  const GLfloat *params);
   /** Set texture environment parameters */
   void (*TexEnv)(struct gl_context *ctx, GLenum target, GLenum pname,
                  const GLfloat *param);
   /** Set texture parameters */
   void (*TexParameter)(struct gl_context *ctx, GLenum target,
                        struct gl_texture_object *texObj,
                        GLenum pname, const GLfloat *params);
   /** Set the viewport */
   void (*Viewport)(struct gl_context *ctx, GLint x, GLint y, GLsizei w, GLsizei h);
   /*@}*/


   /**
    * \name Vertex/pixel buffer object functions
    */
   /*@{*/
   void (*BindBuffer)( struct gl_context *ctx, GLenum target,
		       struct gl_buffer_object *obj );

   struct gl_buffer_object * (*NewBufferObject)( struct gl_context *ctx, GLuint buffer,
						 GLenum target );
   
   void (*DeleteBuffer)( struct gl_context *ctx, struct gl_buffer_object *obj );

   GLboolean (*BufferData)( struct gl_context *ctx, GLenum target, GLsizeiptrARB size,
                            const GLvoid *data, GLenum usage,
                            struct gl_buffer_object *obj );

   void (*BufferSubData)( struct gl_context *ctx, GLintptrARB offset,
			  GLsizeiptrARB size, const GLvoid *data,
			  struct gl_buffer_object *obj );

   void (*GetBufferSubData)( struct gl_context *ctx,
			     GLintptrARB offset, GLsizeiptrARB size,
			     GLvoid *data, struct gl_buffer_object *obj );

   void (*CopyBufferSubData)( struct gl_context *ctx,
                              struct gl_buffer_object *src,
                              struct gl_buffer_object *dst,
                              GLintptr readOffset, GLintptr writeOffset,
                              GLsizeiptr size );

   /* May return NULL if MESA_MAP_NOWAIT_BIT is set in access:
    */
   void * (*MapBufferRange)( struct gl_context *ctx, GLintptr offset,
                             GLsizeiptr length, GLbitfield access,
                             struct gl_buffer_object *obj);

   void (*FlushMappedBufferRange)(struct gl_context *ctx,
                                  GLintptr offset, GLsizeiptr length,
                                  struct gl_buffer_object *obj);

   GLboolean (*UnmapBuffer)( struct gl_context *ctx,
			     struct gl_buffer_object *obj );
   /*@}*/

   /**
    * \name Functions for GL_APPLE_object_purgeable
    */
   /*@{*/
   /* variations on ObjectPurgeable */
   GLenum (*BufferObjectPurgeable)( struct gl_context *ctx, struct gl_buffer_object *obj, GLenum option );
   GLenum (*RenderObjectPurgeable)( struct gl_context *ctx, struct gl_renderbuffer *obj, GLenum option );
   GLenum (*TextureObjectPurgeable)( struct gl_context *ctx, struct gl_texture_object *obj, GLenum option );

   /* variations on ObjectUnpurgeable */
   GLenum (*BufferObjectUnpurgeable)( struct gl_context *ctx, struct gl_buffer_object *obj, GLenum option );
   GLenum (*RenderObjectUnpurgeable)( struct gl_context *ctx, struct gl_renderbuffer *obj, GLenum option );
   GLenum (*TextureObjectUnpurgeable)( struct gl_context *ctx, struct gl_texture_object *obj, GLenum option );
   /*@}*/

   /**
    * \name Functions for GL_EXT_framebuffer_{object,blit}.
    */
   /*@{*/
   struct gl_framebuffer * (*NewFramebuffer)(struct gl_context *ctx, GLuint name);
   struct gl_renderbuffer * (*NewRenderbuffer)(struct gl_context *ctx, GLuint name);
   void (*BindFramebuffer)(struct gl_context *ctx, GLenum target,
                           struct gl_framebuffer *drawFb,
                           struct gl_framebuffer *readFb);
   void (*FramebufferRenderbuffer)(struct gl_context *ctx, 
                                   struct gl_framebuffer *fb,
                                   GLenum attachment,
                                   struct gl_renderbuffer *rb);
   void (*RenderTexture)(struct gl_context *ctx,
                         struct gl_framebuffer *fb,
                         struct gl_renderbuffer_attachment *att);
   void (*FinishRenderTexture)(struct gl_context *ctx,
                               struct gl_renderbuffer_attachment *att);
   void (*ValidateFramebuffer)(struct gl_context *ctx,
                               struct gl_framebuffer *fb);
   /*@}*/
   void (*BlitFramebuffer)(struct gl_context *ctx,
                           GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                           GLbitfield mask, GLenum filter);

   /**
    * \name Query objects
    */
   /*@{*/
   struct gl_query_object * (*NewQueryObject)(struct gl_context *ctx, GLuint id);
   void (*DeleteQuery)(struct gl_context *ctx, struct gl_query_object *q);
   void (*BeginQuery)(struct gl_context *ctx, struct gl_query_object *q);
   void (*EndQuery)(struct gl_context *ctx, struct gl_query_object *q);
   void (*CheckQuery)(struct gl_context *ctx, struct gl_query_object *q);
   void (*WaitQuery)(struct gl_context *ctx, struct gl_query_object *q);
   /*@}*/


   /**
    * \name Vertex Array objects
    */
   /*@{*/
   struct gl_array_object * (*NewArrayObject)(struct gl_context *ctx, GLuint id);
   void (*DeleteArrayObject)(struct gl_context *ctx, struct gl_array_object *obj);
   void (*BindArrayObject)(struct gl_context *ctx, struct gl_array_object *obj);
   /*@}*/

   /**
    * \name GLSL-related functions (ARB extensions and OpenGL 2.x)
    */
   /*@{*/
   struct gl_shader *(*NewShader)(struct gl_context *ctx, GLuint name, GLenum type);
   void (*DeleteShader)(struct gl_context *ctx, struct gl_shader *shader);
   struct gl_shader_program *(*NewShaderProgram)(struct gl_context *ctx, GLuint name);
   void (*DeleteShaderProgram)(struct gl_context *ctx,
                               struct gl_shader_program *shProg);
   void (*UseProgram)(struct gl_context *ctx, struct gl_shader_program *shProg);
   /*@}*/


   /**
    * \name Support for multiple T&L engines
    */
   /*@{*/

   /**
    * Set by the driver-supplied T&L engine.  
    *
    * Set to PRIM_OUTSIDE_BEGIN_END when outside glBegin()/glEnd().
    */
   GLuint CurrentExecPrimitive;

   /**
    * Current state of an in-progress compilation.  
    *
    * May take on any of the additional values PRIM_OUTSIDE_BEGIN_END,
    * PRIM_INSIDE_UNKNOWN_PRIM or PRIM_UNKNOWN defined above.
    */
   GLuint CurrentSavePrimitive;


#define FLUSH_STORED_VERTICES 0x1
#define FLUSH_UPDATE_CURRENT  0x2
   /**
    * Set by the driver-supplied T&L engine whenever vertices are buffered
    * between glBegin()/glEnd() objects or __struct gl_contextRec::Current is not
    * updated.
    *
    * The dd_function_table::FlushVertices call below may be used to resolve
    * these conditions.
    */
   GLuint NeedFlush;
   GLuint SaveNeedFlush;


   /* Called prior to any of the GLvertexformat functions being
    * called.  Paired with Driver.FlushVertices().
    */
   void (*BeginVertices)( struct gl_context *ctx );

   /**
    * If inside glBegin()/glEnd(), it should ASSERT(0).  Otherwise, if
    * FLUSH_STORED_VERTICES bit in \p flags is set flushes any buffered
    * vertices, if FLUSH_UPDATE_CURRENT bit is set updates
    * __struct gl_contextRec::Current and gl_light_attrib::Material
    *
    * Note that the default T&L engine never clears the
    * FLUSH_UPDATE_CURRENT bit, even after performing the update.
    */
   void (*FlushVertices)( struct gl_context *ctx, GLuint flags );
   void (*SaveFlushVertices)( struct gl_context *ctx );

   /**
    * \brief Hook for drivers to prepare for a glBegin/glEnd block
    *
    * This hook is called in vbo_exec_Begin() before any action, including
    * state updates, occurs.
    */
   void (*PrepareExecBegin)( struct gl_context *ctx );

   /**
    * Give the driver the opportunity to hook in its own vtxfmt for
    * compiling optimized display lists.  This is called on each valid
    * glBegin() during list compilation.
    */
   GLboolean (*NotifySaveBegin)( struct gl_context *ctx, GLenum mode );

   /**
    * Notify driver that the special derived value _NeedEyeCoords has
    * changed.
    */
   void (*LightingSpaceChange)( struct gl_context *ctx );

   /**
    * Called by glNewList().
    *
    * Let the T&L component know what is going on with display lists
    * in time to make changes to dispatch tables, etc.
    */
   void (*NewList)( struct gl_context *ctx, GLuint list, GLenum mode );
   /**
    * Called by glEndList().
    *
    * \sa dd_function_table::NewList.
    */
   void (*EndList)( struct gl_context *ctx );

   /**
    * Called by glCallList(s).
    *
    * Notify the T&L component before and after calling a display list.
    */
   void (*BeginCallList)( struct gl_context *ctx, 
			  struct gl_display_list *dlist );
   /**
    * Called by glEndCallList().
    *
    * \sa dd_function_table::BeginCallList.
    */
   void (*EndCallList)( struct gl_context *ctx );

   /**@}*/

   /**
    * \name GL_ARB_sync interfaces
    */
   /*@{*/
   struct gl_sync_object * (*NewSyncObject)(struct gl_context *, GLenum);
   void (*FenceSync)(struct gl_context *, struct gl_sync_object *, GLenum, GLbitfield);
   void (*DeleteSyncObject)(struct gl_context *, struct gl_sync_object *);
   void (*CheckSync)(struct gl_context *, struct gl_sync_object *);
   void (*ClientWaitSync)(struct gl_context *, struct gl_sync_object *,
			  GLbitfield, GLuint64);
   void (*ServerWaitSync)(struct gl_context *, struct gl_sync_object *,
			  GLbitfield, GLuint64);
   /*@}*/

   /** GL_NV_conditional_render */
   void (*BeginConditionalRender)(struct gl_context *ctx, struct gl_query_object *q,
                                  GLenum mode);
   void (*EndConditionalRender)(struct gl_context *ctx, struct gl_query_object *q);

   /**
    * \name GL_OES_draw_texture interface
    */
   /*@{*/
   void (*DrawTex)(struct gl_context *ctx, GLfloat x, GLfloat y, GLfloat z,
                   GLfloat width, GLfloat height);
   /*@}*/

   /**
    * \name GL_OES_EGL_image interface
    */
   void (*EGLImageTargetTexture2D)(struct gl_context *ctx, GLenum target,
				   struct gl_texture_object *texObj,
				   struct gl_texture_image *texImage,
				   GLeglImageOES image_handle);
   void (*EGLImageTargetRenderbufferStorage)(struct gl_context *ctx,
					     struct gl_renderbuffer *rb,
					     void *image_handle);

   /**
    * \name GL_EXT_transform_feedback interface
    */
   struct gl_transform_feedback_object *
        (*NewTransformFeedback)(struct gl_context *ctx, GLuint name);
   void (*DeleteTransformFeedback)(struct gl_context *ctx,
                                   struct gl_transform_feedback_object *obj);
   void (*BeginTransformFeedback)(struct gl_context *ctx, GLenum mode,
                                  struct gl_transform_feedback_object *obj);
   void (*EndTransformFeedback)(struct gl_context *ctx,
                                struct gl_transform_feedback_object *obj);
   void (*PauseTransformFeedback)(struct gl_context *ctx,
                                  struct gl_transform_feedback_object *obj);
   void (*ResumeTransformFeedback)(struct gl_context *ctx,
                                   struct gl_transform_feedback_object *obj);

   /**
    * \name GL_NV_texture_barrier interface
    */
   void (*TextureBarrier)(struct gl_context *ctx);

   /**
    * \name GL_ARB_sampler_objects
    */
   struct gl_sampler_object * (*NewSamplerObject)(struct gl_context *ctx,
                                                  GLuint name);
   void (*DeleteSamplerObject)(struct gl_context *ctx,
                               struct gl_sampler_object *samp);
};


/**
 * Transform/Clip/Lighting interface
 *
 * Drivers present a reduced set of the functions possible in
 * glBegin()/glEnd() objects.  Core mesa provides translation stubs for the
 * remaining functions to map down to these entry points.
 *
 * These are the initial values to be installed into dispatch by
 * mesa.  If the T&L driver wants to modify the dispatch table
 * while installed, it must do so itself.  It would be possible for
 * the vertexformat to install its own initial values for these
 * functions, but this way there is an obvious list of what is
 * expected of the driver.
 *
 * If the driver wants to hook in entry points other than those
 * listed, it must restore them to their original values in
 * the disable() callback, below.
 */
typedef struct {
   /**
    * \name Vertex
    */
   /*@{*/
   void (GLAPIENTRYP ArrayElement)( GLint );
   void (GLAPIENTRYP Color3f)( GLfloat, GLfloat, GLfloat );
   void (GLAPIENTRYP Color3fv)( const GLfloat * );
   void (GLAPIENTRYP Color4f)( GLfloat, GLfloat, GLfloat, GLfloat );
   void (GLAPIENTRYP Color4fv)( const GLfloat * );
   void (GLAPIENTRYP EdgeFlag)( GLboolean );
   void (GLAPIENTRYP EvalCoord1f)( GLfloat );
   void (GLAPIENTRYP EvalCoord1fv)( const GLfloat * );
   void (GLAPIENTRYP EvalCoord2f)( GLfloat, GLfloat );
   void (GLAPIENTRYP EvalCoord2fv)( const GLfloat * );
   void (GLAPIENTRYP EvalPoint1)( GLint );
   void (GLAPIENTRYP EvalPoint2)( GLint, GLint );
   void (GLAPIENTRYP FogCoordfEXT)( GLfloat );
   void (GLAPIENTRYP FogCoordfvEXT)( const GLfloat * );
   void (GLAPIENTRYP Indexf)( GLfloat );
   void (GLAPIENTRYP Indexfv)( const GLfloat * );
   void (GLAPIENTRYP Materialfv)( GLenum face, GLenum pname, const GLfloat * );
   void (GLAPIENTRYP MultiTexCoord1fARB)( GLenum, GLfloat );
   void (GLAPIENTRYP MultiTexCoord1fvARB)( GLenum, const GLfloat * );
   void (GLAPIENTRYP MultiTexCoord2fARB)( GLenum, GLfloat, GLfloat );
   void (GLAPIENTRYP MultiTexCoord2fvARB)( GLenum, const GLfloat * );
   void (GLAPIENTRYP MultiTexCoord3fARB)( GLenum, GLfloat, GLfloat, GLfloat );
   void (GLAPIENTRYP MultiTexCoord3fvARB)( GLenum, const GLfloat * );
   void (GLAPIENTRYP MultiTexCoord4fARB)( GLenum, GLfloat, GLfloat, GLfloat, GLfloat );
   void (GLAPIENTRYP MultiTexCoord4fvARB)( GLenum, const GLfloat * );
   void (GLAPIENTRYP Normal3f)( GLfloat, GLfloat, GLfloat );
   void (GLAPIENTRYP Normal3fv)( const GLfloat * );
   void (GLAPIENTRYP SecondaryColor3fEXT)( GLfloat, GLfloat, GLfloat );
   void (GLAPIENTRYP SecondaryColor3fvEXT)( const GLfloat * );
   void (GLAPIENTRYP TexCoord1f)( GLfloat );
   void (GLAPIENTRYP TexCoord1fv)( const GLfloat * );
   void (GLAPIENTRYP TexCoord2f)( GLfloat, GLfloat );
   void (GLAPIENTRYP TexCoord2fv)( const GLfloat * );
   void (GLAPIENTRYP TexCoord3f)( GLfloat, GLfloat, GLfloat );
   void (GLAPIENTRYP TexCoord3fv)( const GLfloat * );
   void (GLAPIENTRYP TexCoord4f)( GLfloat, GLfloat, GLfloat, GLfloat );
   void (GLAPIENTRYP TexCoord4fv)( const GLfloat * );
   void (GLAPIENTRYP Vertex2f)( GLfloat, GLfloat );
   void (GLAPIENTRYP Vertex2fv)( const GLfloat * );
   void (GLAPIENTRYP Vertex3f)( GLfloat, GLfloat, GLfloat );
   void (GLAPIENTRYP Vertex3fv)( const GLfloat * );
   void (GLAPIENTRYP Vertex4f)( GLfloat, GLfloat, GLfloat, GLfloat );
   void (GLAPIENTRYP Vertex4fv)( const GLfloat * );
   void (GLAPIENTRYP CallList)( GLuint );
   void (GLAPIENTRYP CallLists)( GLsizei, GLenum, const GLvoid * );
   void (GLAPIENTRYP Begin)( GLenum );
   void (GLAPIENTRYP End)( void );
   void (GLAPIENTRYP PrimitiveRestartNV)( void );
   /* GL_NV_vertex_program */
   void (GLAPIENTRYP VertexAttrib1fNV)( GLuint index, GLfloat x );
   void (GLAPIENTRYP VertexAttrib1fvNV)( GLuint index, const GLfloat *v );
   void (GLAPIENTRYP VertexAttrib2fNV)( GLuint index, GLfloat x, GLfloat y );
   void (GLAPIENTRYP VertexAttrib2fvNV)( GLuint index, const GLfloat *v );
   void (GLAPIENTRYP VertexAttrib3fNV)( GLuint index, GLfloat x, GLfloat y, GLfloat z );
   void (GLAPIENTRYP VertexAttrib3fvNV)( GLuint index, const GLfloat *v );
   void (GLAPIENTRYP VertexAttrib4fNV)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
   void (GLAPIENTRYP VertexAttrib4fvNV)( GLuint index, const GLfloat *v );
   /* GL_ARB_vertex_program */
   void (GLAPIENTRYP VertexAttrib1fARB)( GLuint index, GLfloat x );
   void (GLAPIENTRYP VertexAttrib1fvARB)( GLuint index, const GLfloat *v );
   void (GLAPIENTRYP VertexAttrib2fARB)( GLuint index, GLfloat x, GLfloat y );
   void (GLAPIENTRYP VertexAttrib2fvARB)( GLuint index, const GLfloat *v );
   void (GLAPIENTRYP VertexAttrib3fARB)( GLuint index, GLfloat x, GLfloat y, GLfloat z );
   void (GLAPIENTRYP VertexAttrib3fvARB)( GLuint index, const GLfloat *v );
   void (GLAPIENTRYP VertexAttrib4fARB)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
   void (GLAPIENTRYP VertexAttrib4fvARB)( GLuint index, const GLfloat *v );

   /* GL_EXT_gpu_shader4 / GL 3.0 */
   void (GLAPIENTRYP VertexAttribI1i)( GLuint index, GLint x);
   void (GLAPIENTRYP VertexAttribI2i)( GLuint index, GLint x, GLint y);
   void (GLAPIENTRYP VertexAttribI3i)( GLuint index, GLint x, GLint y, GLint z);
   void (GLAPIENTRYP VertexAttribI4i)( GLuint index, GLint x, GLint y, GLint z, GLint w);
   void (GLAPIENTRYP VertexAttribI2iv)( GLuint index, const GLint *v);
   void (GLAPIENTRYP VertexAttribI3iv)( GLuint index, const GLint *v);
   void (GLAPIENTRYP VertexAttribI4iv)( GLuint index, const GLint *v);

   void (GLAPIENTRYP VertexAttribI1ui)( GLuint index, GLuint x);
   void (GLAPIENTRYP VertexAttribI2ui)( GLuint index, GLuint x, GLuint y);
   void (GLAPIENTRYP VertexAttribI3ui)( GLuint index, GLuint x, GLuint y, GLuint z);
   void (GLAPIENTRYP VertexAttribI4ui)( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
   void (GLAPIENTRYP VertexAttribI2uiv)( GLuint index, const GLuint *v);
   void (GLAPIENTRYP VertexAttribI3uiv)( GLuint index, const GLuint *v);
   void (GLAPIENTRYP VertexAttribI4uiv)( GLuint index, const GLuint *v);

   /* GL_ARB_vertex_type_10_10_10_2_rev / GL3.3 */
   void (GLAPIENTRYP VertexP2ui)( GLenum type, GLuint value );
   void (GLAPIENTRYP VertexP2uiv)( GLenum type, const GLuint *value);

   void (GLAPIENTRYP VertexP3ui)( GLenum type, GLuint value );
   void (GLAPIENTRYP VertexP3uiv)( GLenum type, const GLuint *value);

   void (GLAPIENTRYP VertexP4ui)( GLenum type, GLuint value );
   void (GLAPIENTRYP VertexP4uiv)( GLenum type, const GLuint *value);

   void (GLAPIENTRYP TexCoordP1ui)( GLenum type, GLuint coords );
   void (GLAPIENTRYP TexCoordP1uiv)( GLenum type, const GLuint *coords );

   void (GLAPIENTRYP TexCoordP2ui)( GLenum type, GLuint coords );
   void (GLAPIENTRYP TexCoordP2uiv)( GLenum type, const GLuint *coords );

   void (GLAPIENTRYP TexCoordP3ui)( GLenum type, GLuint coords );
   void (GLAPIENTRYP TexCoordP3uiv)( GLenum type, const GLuint *coords );

   void (GLAPIENTRYP TexCoordP4ui)( GLenum type, GLuint coords );
   void (GLAPIENTRYP TexCoordP4uiv)( GLenum type, const GLuint *coords );

   void (GLAPIENTRYP MultiTexCoordP1ui)( GLenum texture, GLenum type, GLuint coords );
   void (GLAPIENTRYP MultiTexCoordP1uiv)( GLenum texture, GLenum type, const GLuint *coords );
   void (GLAPIENTRYP MultiTexCoordP2ui)( GLenum texture, GLenum type, GLuint coords );
   void (GLAPIENTRYP MultiTexCoordP2uiv)( GLenum texture, GLenum type, const GLuint *coords );
   void (GLAPIENTRYP MultiTexCoordP3ui)( GLenum texture, GLenum type, GLuint coords );
   void (GLAPIENTRYP MultiTexCoordP3uiv)( GLenum texture, GLenum type, const GLuint *coords );
   void (GLAPIENTRYP MultiTexCoordP4ui)( GLenum texture, GLenum type, GLuint coords );
   void (GLAPIENTRYP MultiTexCoordP4uiv)( GLenum texture, GLenum type, const GLuint *coords );

   void (GLAPIENTRYP NormalP3ui)( GLenum type, GLuint coords );
   void (GLAPIENTRYP NormalP3uiv)( GLenum type, const GLuint *coords );

   void (GLAPIENTRYP ColorP3ui)( GLenum type, GLuint color );
   void (GLAPIENTRYP ColorP3uiv)( GLenum type, const GLuint *color );

   void (GLAPIENTRYP ColorP4ui)( GLenum type, GLuint color );
   void (GLAPIENTRYP ColorP4uiv)( GLenum type, const GLuint *color );

   void (GLAPIENTRYP SecondaryColorP3ui)( GLenum type, GLuint color );
   void (GLAPIENTRYP SecondaryColorP3uiv)( GLenum type, const GLuint *color );

   void (GLAPIENTRYP VertexAttribP1ui)( GLuint index, GLenum type,
					GLboolean normalized, GLuint value);
   void (GLAPIENTRYP VertexAttribP2ui)( GLuint index, GLenum type,
					GLboolean normalized, GLuint value);
   void (GLAPIENTRYP VertexAttribP3ui)( GLuint index, GLenum type,
					GLboolean normalized, GLuint value);
   void (GLAPIENTRYP VertexAttribP4ui)( GLuint index, GLenum type,
					GLboolean normalized, GLuint value);
   void (GLAPIENTRYP VertexAttribP1uiv)( GLuint index, GLenum type,
					GLboolean normalized,
					 const GLuint *value);
   void (GLAPIENTRYP VertexAttribP2uiv)( GLuint index, GLenum type,
					GLboolean normalized,
					 const GLuint *value);
   void (GLAPIENTRYP VertexAttribP3uiv)( GLuint index, GLenum type,
					GLboolean normalized,
					 const GLuint *value);
   void (GLAPIENTRYP VertexAttribP4uiv)( GLuint index, GLenum type,
					 GLboolean normalized,
					 const GLuint *value);

   /*@}*/

   void (GLAPIENTRYP Rectf)( GLfloat, GLfloat, GLfloat, GLfloat );

   /**
    * \name Array
    */
   /*@{*/
   void (GLAPIENTRYP DrawArrays)( GLenum mode, GLint start, GLsizei count );
   void (GLAPIENTRYP DrawElements)( GLenum mode, GLsizei count, GLenum type,
			 const GLvoid *indices );
   void (GLAPIENTRYP DrawRangeElements)( GLenum mode, GLuint start,
			      GLuint end, GLsizei count,
			      GLenum type, const GLvoid *indices );
   void (GLAPIENTRYP MultiDrawElementsEXT)( GLenum mode, const GLsizei *count,
					    GLenum type,
					    const GLvoid **indices,
					    GLsizei primcount);
   void (GLAPIENTRYP DrawElementsBaseVertex)( GLenum mode, GLsizei count,
					      GLenum type,
					      const GLvoid *indices,
					      GLint basevertex );
   void (GLAPIENTRYP DrawRangeElementsBaseVertex)( GLenum mode, GLuint start,
						   GLuint end, GLsizei count,
						   GLenum type,
						   const GLvoid *indices,
						   GLint basevertex);
   void (GLAPIENTRYP MultiDrawElementsBaseVertex)( GLenum mode,
						   const GLsizei *count,
						   GLenum type,
						   const GLvoid **indices,
						   GLsizei primcount,
						   const GLint *basevertex);
   void (GLAPIENTRYP DrawArraysInstanced)(GLenum mode, GLint first,
                                          GLsizei count, GLsizei primcount);
   void (GLAPIENTRYP DrawElementsInstanced)(GLenum mode, GLsizei count,
                                            GLenum type, const GLvoid *indices,
                                            GLsizei primcount);
   void (GLAPIENTRYP DrawElementsInstancedBaseVertex)(GLenum mode, GLsizei count,
                                            GLenum type, const GLvoid *indices,
                                            GLsizei primcount, GLint basevertex);
   void (GLAPIENTRYP DrawTransformFeedback)(GLenum mode, GLuint name);
   /*@}*/

   /**
    * \name Eval
    *
    * If you don't support eval, fallback to the default vertex format
    * on receiving an eval call and use the pipeline mechanism to
    * provide partial T&L acceleration.
    *
    * Mesa will provide a set of helper functions to do eval within
    * accelerated vertex formats, eventually...
    */
   /*@{*/
   void (GLAPIENTRYP EvalMesh1)( GLenum mode, GLint i1, GLint i2 );
   void (GLAPIENTRYP EvalMesh2)( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 );
   /*@}*/

} GLvertexformat;


#endif /* DD_INCLUDED */
