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

struct gl_pixelstore_attrib;
struct mesa_display_list;

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
   const GLubyte * (*GetString)( GLcontext *ctx, GLenum name );

   /**
    * Notify the driver after Mesa has made some internal state changes.  
    *
    * This is in addition to any state change callbacks Mesa may already have
    * made.
    */
   void (*UpdateState)( GLcontext *ctx, GLbitfield new_state );

   /**
    * Get the width and height of the named buffer/window.
    *
    * Mesa uses this to determine when the driver's window size has changed.
    * XXX OBSOLETE: this function will be removed in the future.
    */
   void (*GetBufferSize)( GLframebuffer *buffer,
                          GLuint *width, GLuint *height );

   /**
    * Resize the given framebuffer to the given size.
    * XXX OBSOLETE: this function will be removed in the future.
    */
   void (*ResizeBuffers)( GLcontext *ctx, GLframebuffer *fb,
                          GLuint width, GLuint height);

   /**
    * Called whenever an error is generated.  
    * __GLcontextRec::ErrorValue contains the error value.
    */
   void (*Error)( GLcontext *ctx );

   /**
    * This is called whenever glFinish() is called.
    */
   void (*Finish)( GLcontext *ctx );

   /**
    * This is called whenever glFlush() is called.
    */
   void (*Flush)( GLcontext *ctx );

   /**
    * Clear the color/depth/stencil/accum buffer(s).
    * \param buffers  a bitmask of BUFFER_BIT_* flags indicating which
    *                 renderbuffers need to be cleared.
    */
   void (*Clear)( GLcontext *ctx, GLbitfield buffers );

   /**
    * Execute glAccum command.
    */
   void (*Accum)( GLcontext *ctx, GLenum op, GLfloat value );


   /**
    * Execute glRasterPos, updating the ctx->Current.Raster fields
    */
   void (*RasterPos)( GLcontext *ctx, const GLfloat v[4] );

   /**
    * \name Image-related functions
    */
   /*@{*/

   /**
    * Called by glDrawPixels().
    * \p unpack describes how to unpack the source image data.
    */
   void (*DrawPixels)( GLcontext *ctx,
		       GLint x, GLint y, GLsizei width, GLsizei height,
		       GLenum format, GLenum type,
		       const struct gl_pixelstore_attrib *unpack,
		       const GLvoid *pixels );

   /**
    * Called by glReadPixels().
    */
   void (*ReadPixels)( GLcontext *ctx,
		       GLint x, GLint y, GLsizei width, GLsizei height,
		       GLenum format, GLenum type,
		       const struct gl_pixelstore_attrib *unpack,
		       GLvoid *dest );

   /**
    * Called by glCopyPixels().  
    */
   void (*CopyPixels)( GLcontext *ctx, GLint srcx, GLint srcy,
                       GLsizei width, GLsizei height,
                       GLint dstx, GLint dsty, GLenum type );

   /**
    * Called by glBitmap().  
    */
   void (*Bitmap)( GLcontext *ctx,
		   GLint x, GLint y, GLsizei width, GLsizei height,
		   const struct gl_pixelstore_attrib *unpack,
		   const GLubyte *bitmap );
   /*@}*/

   
   /**
    * \name Texture image functions
    */
   /*@{*/

   /**
    * Choose texture format.
    * 
    * This is called by the \c _mesa_store_tex[sub]image[123]d() fallback
    * functions.  The driver should examine \p internalFormat and return a
    * pointer to an appropriate gl_texture_format.
    */
   const struct gl_texture_format *(*ChooseTextureFormat)( GLcontext *ctx,
                      GLint internalFormat, GLenum srcFormat, GLenum srcType );

   /**
    * Called by glTexImage1D().
    * 
    * \param target user specified.
    * \param format user specified.
    * \param type user specified.
    * \param pixels user specified.
    * \param packing indicates the image packing of pixels.
    * \param texObj is the target texture object.
    * \param texImage is the target texture image.  It will have the texture \p
    * width, \p height, \p depth, \p border and \p internalFormat information.
    * 
    * \p retainInternalCopy is returned by this function and indicates whether
    * core Mesa should keep an internal copy of the texture image.
    *
    * Drivers should call a fallback routine from texstore.c if needed.
    */
   void (*TexImage1D)( GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint border,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage );

   /**
    * Called by glTexImage2D().
    * 
    * \sa dd_function_table::TexImage1D.
    */
   void (*TexImage2D)( GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint height, GLint border,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage );
   
   /**
    * Called by glTexImage3D().
    * 
    * \sa dd_function_table::TexImage1D.
    */
   void (*TexImage3D)( GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint height, GLint depth, GLint border,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage );

   /**
    * Called by glTexSubImage1D().
    *
    * \param target user specified.
    * \param level user specified.
    * \param xoffset user specified.
    * \param yoffset user specified.
    * \param zoffset user specified.
    * \param width user specified.
    * \param height user specified.
    * \param depth user specified.
    * \param format user specified.
    * \param type user specified.
    * \param pixels user specified.
    * \param packing indicates the image packing of pixels.
    * \param texObj is the target texture object.
    * \param texImage is the target texture image.  It will have the texture \p
    * width, \p height, \p border and \p internalFormat information.
    *
    * The driver should use a fallback routine from texstore.c if needed.
    */
   void (*TexSubImage1D)( GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLsizei width,
                          GLenum format, GLenum type,
                          const GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage );
   
   /**
    * Called by glTexSubImage2D().
    *
    * \sa dd_function_table::TexSubImage1D.
    */
   void (*TexSubImage2D)( GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLint yoffset,
                          GLsizei width, GLsizei height,
                          GLenum format, GLenum type,
                          const GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage );
   
   /**
    * Called by glTexSubImage3D().
    *
    * \sa dd_function_table::TexSubImage1D.
    */
   void (*TexSubImage3D)( GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLint yoffset, GLint zoffset,
                          GLsizei width, GLsizei height, GLint depth,
                          GLenum format, GLenum type,
                          const GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage );

   /**
    * Called by glGetTexImage().
    */
   void (*GetTexImage)( GLcontext *ctx, GLenum target, GLint level,
                        GLenum format, GLenum type, GLvoid *pixels,
                        struct gl_texture_object *texObj,
                        struct gl_texture_image *texImage );

   /**
    * Called by glCopyTexImage1D().
    * 
    * Drivers should use a fallback routine from texstore.c if needed.
    */
   void (*CopyTexImage1D)( GLcontext *ctx, GLenum target, GLint level,
                           GLenum internalFormat, GLint x, GLint y,
                           GLsizei width, GLint border );

   /**
    * Called by glCopyTexImage2D().
    * 
    * Drivers should use a fallback routine from texstore.c if needed.
    */
   void (*CopyTexImage2D)( GLcontext *ctx, GLenum target, GLint level,
                           GLenum internalFormat, GLint x, GLint y,
                           GLsizei width, GLsizei height, GLint border );

   /**
    * Called by glCopyTexSubImage1D().
    * 
    * Drivers should use a fallback routine from texstore.c if needed.
    */
   void (*CopyTexSubImage1D)( GLcontext *ctx, GLenum target, GLint level,
                              GLint xoffset,
                              GLint x, GLint y, GLsizei width );
   /**
    * Called by glCopyTexSubImage2D().
    * 
    * Drivers should use a fallback routine from texstore.c if needed.
    */
   void (*CopyTexSubImage2D)( GLcontext *ctx, GLenum target, GLint level,
                              GLint xoffset, GLint yoffset,
                              GLint x, GLint y,
                              GLsizei width, GLsizei height );
   /**
    * Called by glCopyTexSubImage3D().
    * 
    * Drivers should use a fallback routine from texstore.c if needed.
    */
   void (*CopyTexSubImage3D)( GLcontext *ctx, GLenum target, GLint level,
                              GLint xoffset, GLint yoffset, GLint zoffset,
                              GLint x, GLint y,
                              GLsizei width, GLsizei height );

   /**
    * Called by glGenerateMipmap() or when GL_GENERATE_MIPMAP_SGIS is enabled.
    */
   void (*GenerateMipmap)(GLcontext *ctx, GLenum target,
                          struct gl_texture_object *texObj);

   /**
    * Called by glTexImage[123]D when user specifies a proxy texture
    * target.  
    *
    * \return GL_TRUE if the proxy test passes, or GL_FALSE if the test fails.
    */
   GLboolean (*TestProxyTexImage)(GLcontext *ctx, GLenum target,
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
    *
    * \param target user specified.
    * \param format user specified.
    * \param type user specified.
    * \param pixels user specified.
    * \param packing indicates the image packing of pixels.
    * \param texObj is the target texture object.
    * \param texImage is the target texture image.  It will have the texture \p
    * width, \p height, \p depth, \p border and \p internalFormat information.
    *      
    * \a retainInternalCopy is returned by this function and indicates whether
    * core Mesa should keep an internal copy of the texture image.
    */
   void (*CompressedTexImage1D)( GLcontext *ctx, GLenum target,
                                 GLint level, GLint internalFormat,
                                 GLsizei width, GLint border,
                                 GLsizei imageSize, const GLvoid *data,
                                 struct gl_texture_object *texObj,
                                 struct gl_texture_image *texImage );
   /**
    * Called by glCompressedTexImage2D().
    *
    * \sa dd_function_table::CompressedTexImage1D.
    */
   void (*CompressedTexImage2D)( GLcontext *ctx, GLenum target,
                                 GLint level, GLint internalFormat,
                                 GLsizei width, GLsizei height, GLint border,
                                 GLsizei imageSize, const GLvoid *data,
                                 struct gl_texture_object *texObj,
                                 struct gl_texture_image *texImage );
   /**
    * Called by glCompressedTexImage3D().
    *
    * \sa dd_function_table::CompressedTexImage3D.
    */
   void (*CompressedTexImage3D)( GLcontext *ctx, GLenum target,
                                 GLint level, GLint internalFormat,
                                 GLsizei width, GLsizei height, GLsizei depth,
                                 GLint border,
                                 GLsizei imageSize, const GLvoid *data,
                                 struct gl_texture_object *texObj,
                                 struct gl_texture_image *texImage );

   /**
    * Called by glCompressedTexSubImage1D().
    * 
    * \param target user specified.
    * \param level user specified.
    * \param xoffset user specified.
    * \param yoffset user specified.
    * \param zoffset user specified.
    * \param width user specified.
    * \param height user specified.
    * \param depth user specified.
    * \param imageSize user specified.
    * \param data user specified.
    * \param texObj is the target texture object.
    * \param texImage is the target texture image.  It will have the texture \p
    * width, \p height, \p depth, \p border and \p internalFormat information.
    */
   void (*CompressedTexSubImage1D)(GLcontext *ctx, GLenum target, GLint level,
                                   GLint xoffset, GLsizei width,
                                   GLenum format,
                                   GLsizei imageSize, const GLvoid *data,
                                   struct gl_texture_object *texObj,
                                   struct gl_texture_image *texImage);
   /**
    * Called by glCompressedTexSubImage2D().
    *
    * \sa dd_function_table::CompressedTexImage3D.
    */
   void (*CompressedTexSubImage2D)(GLcontext *ctx, GLenum target, GLint level,
                                   GLint xoffset, GLint yoffset,
                                   GLsizei width, GLint height,
                                   GLenum format,
                                   GLsizei imageSize, const GLvoid *data,
                                   struct gl_texture_object *texObj,
                                   struct gl_texture_image *texImage);
   /**
    * Called by glCompressedTexSubImage3D().
    *
    * \sa dd_function_table::CompressedTexImage3D.
    */
   void (*CompressedTexSubImage3D)(GLcontext *ctx, GLenum target, GLint level,
                                   GLint xoffset, GLint yoffset, GLint zoffset,
                                   GLsizei width, GLint height, GLint depth,
                                   GLenum format,
                                   GLsizei imageSize, const GLvoid *data,
                                   struct gl_texture_object *texObj,
                                   struct gl_texture_image *texImage);


   /**
    * Called by glGetCompressedTexImage.
    */
   void (*GetCompressedTexImage)(GLcontext *ctx, GLenum target, GLint level,
                                 GLvoid *img,
                                 struct gl_texture_object *texObj,
                                 struct gl_texture_image *texImage);

   /**
    * Called to query number of bytes of storage needed to store the
    * specified compressed texture.
    */
   GLuint (*CompressedTextureSize)( GLcontext *ctx, GLsizei width,
                                    GLsizei height, GLsizei depth,
                                    GLenum format );
   /*@}*/

   /**
    * \name Texture object functions
    */
   /*@{*/

   /**
    * Called by glBindTexture().
    */
   void (*BindTexture)( GLcontext *ctx, GLenum target,
                        struct gl_texture_object *tObj );

   /**
    * Called to allocate a new texture object.
    * A new gl_texture_object should be returned.  The driver should
    * attach to it any device-specific info it needs.
    */
   struct gl_texture_object * (*NewTextureObject)( GLcontext *ctx, GLuint name,
                                                   GLenum target );
   /**
    * Called when a texture object is about to be deallocated.  
    *
    * Driver should delete the gl_texture_object object and anything
    * hanging off of it.
    */
   void (*DeleteTexture)( GLcontext *ctx, struct gl_texture_object *tObj );

   /**
    * Called to allocate a new texture image object.
    */
   struct gl_texture_image * (*NewTextureImage)( GLcontext *ctx );

   /** 
    * Called to free tImage->Data.
    */
   void (*FreeTexImageData)( GLcontext *ctx, struct gl_texture_image *tImage );

   /** Map texture image data into user space */
   void (*MapTexture)( GLcontext *ctx, struct gl_texture_object *tObj );
   /** Unmap texture images from user space */
   void (*UnmapTexture)( GLcontext *ctx, struct gl_texture_object *tObj );

   /**
    * Note: no context argument.  This function doesn't initially look
    * like it belongs here, except that the driver is the only entity
    * that knows for sure how the texture memory is allocated - via
    * the above callbacks.  There is then an argument that the driver
    * knows what memcpy paths might be fast.  Typically this is invoked with
    * 
    * to -- a pointer into texture memory allocated by NewTextureImage() above.
    * from -- a pointer into client memory or a mesa temporary.
    * sz -- nr bytes to copy.
    */
   void* (*TextureMemCpy)( void *to, const void *from, size_t sz );

   /**
    * Called by glAreTextureResident().
    */
   GLboolean (*IsTextureResident)( GLcontext *ctx,
                                   struct gl_texture_object *t );

   /**
    * Called by glPrioritizeTextures().
    */
   void (*PrioritizeTexture)( GLcontext *ctx,  struct gl_texture_object *t,
                              GLclampf priority );

   /**
    * Called by glActiveTextureARB() to set current texture unit.
    */
   void (*ActiveTexture)( GLcontext *ctx, GLuint texUnitNumber );

   /**
    * Called when the texture's color lookup table is changed.
    * 
    * If \p tObj is NULL then the shared texture palette
    * gl_texture_object::Palette is to be updated.
    */
   void (*UpdateTexturePalette)( GLcontext *ctx,
                                 struct gl_texture_object *tObj );
   /*@}*/

   
   /**
    * \name Imaging functionality
    */
   /*@{*/
   void (*CopyColorTable)( GLcontext *ctx,
			   GLenum target, GLenum internalformat,
			   GLint x, GLint y, GLsizei width );

   void (*CopyColorSubTable)( GLcontext *ctx,
			      GLenum target, GLsizei start,
			      GLint x, GLint y, GLsizei width );

   void (*CopyConvolutionFilter1D)( GLcontext *ctx, GLenum target,
				    GLenum internalFormat,
				    GLint x, GLint y, GLsizei width );

   void (*CopyConvolutionFilter2D)( GLcontext *ctx, GLenum target,
				    GLenum internalFormat,
				    GLint x, GLint y,
				    GLsizei width, GLsizei height );
   /*@}*/


   /**
    * \name Vertex/fragment program functions
    */
   /*@{*/
   /** Bind a vertex/fragment program */
   void (*BindProgram)(GLcontext *ctx, GLenum target, struct gl_program *prog);
   /** Allocate a new program */
   struct gl_program * (*NewProgram)(GLcontext *ctx, GLenum target, GLuint id);
   /** Delete a program */
   void (*DeleteProgram)(GLcontext *ctx, struct gl_program *prog);   
   /** Notify driver that a program string has been specified. */
   void (*ProgramStringNotify)(GLcontext *ctx, GLenum target, 
			       struct gl_program *prog);
   /** Get value of a program register during program execution. */
   void (*GetProgramRegister)(GLcontext *ctx, enum register_file file,
                              GLuint index, GLfloat val[4]);

   /** Query if program can be loaded onto hardware */
   GLboolean (*IsProgramNative)(GLcontext *ctx, GLenum target, 
				struct gl_program *prog);
   
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
   void (*AlphaFunc)(GLcontext *ctx, GLenum func, GLfloat ref);
   /** Set the blend color */
   void (*BlendColor)(GLcontext *ctx, const GLfloat color[4]);
   /** Set the blend equation */
   void (*BlendEquationSeparate)(GLcontext *ctx, GLenum modeRGB, GLenum modeA);
   /** Specify pixel arithmetic */
   void (*BlendFuncSeparate)(GLcontext *ctx,
                             GLenum sfactorRGB, GLenum dfactorRGB,
                             GLenum sfactorA, GLenum dfactorA);
   /** Specify clear values for the color buffers */
   void (*ClearColor)(GLcontext *ctx, const GLfloat color[4]);
   /** Specify the clear value for the depth buffer */
   void (*ClearDepth)(GLcontext *ctx, GLclampd d);
   /** Specify the clear value for the color index buffers */
   void (*ClearIndex)(GLcontext *ctx, GLuint index);
   /** Specify the clear value for the stencil buffer */
   void (*ClearStencil)(GLcontext *ctx, GLint s);
   /** Specify a plane against which all geometry is clipped */
   void (*ClipPlane)(GLcontext *ctx, GLenum plane, const GLfloat *equation );
   /** Enable and disable writing of frame buffer color components */
   void (*ColorMask)(GLcontext *ctx, GLboolean rmask, GLboolean gmask,
                     GLboolean bmask, GLboolean amask );
   /** Cause a material color to track the current color */
   void (*ColorMaterial)(GLcontext *ctx, GLenum face, GLenum mode);
   /** Specify whether front- or back-facing facets can be culled */
   void (*CullFace)(GLcontext *ctx, GLenum mode);
   /** Define front- and back-facing polygons */
   void (*FrontFace)(GLcontext *ctx, GLenum mode);
   /** Specify the value used for depth buffer comparisons */
   void (*DepthFunc)(GLcontext *ctx, GLenum func);
   /** Enable or disable writing into the depth buffer */
   void (*DepthMask)(GLcontext *ctx, GLboolean flag);
   /** Specify mapping of depth values from NDC to window coordinates */
   void (*DepthRange)(GLcontext *ctx, GLclampd nearval, GLclampd farval);
   /** Specify the current buffer for writing */
   void (*DrawBuffer)( GLcontext *ctx, GLenum buffer );
   /** Specify the buffers for writing for fragment programs*/
   void (*DrawBuffers)( GLcontext *ctx, GLsizei n, const GLenum *buffers );
   /** Enable or disable server-side gl capabilities */
   void (*Enable)(GLcontext *ctx, GLenum cap, GLboolean state);
   /** Specify fog parameters */
   void (*Fogfv)(GLcontext *ctx, GLenum pname, const GLfloat *params);
   /** Specify implementation-specific hints */
   void (*Hint)(GLcontext *ctx, GLenum target, GLenum mode);
   /** Control the writing of individual bits in the color index buffers */
   void (*IndexMask)(GLcontext *ctx, GLuint mask);
   /** Set light source parameters.
    * Note: for GL_POSITION and GL_SPOT_DIRECTION, params will have already
    * been transformed to eye-space.
    */
   void (*Lightfv)(GLcontext *ctx, GLenum light,
		   GLenum pname, const GLfloat *params );
   /** Set the lighting model parameters */
   void (*LightModelfv)(GLcontext *ctx, GLenum pname, const GLfloat *params);
   /** Specify the line stipple pattern */
   void (*LineStipple)(GLcontext *ctx, GLint factor, GLushort pattern );
   /** Specify the width of rasterized lines */
   void (*LineWidth)(GLcontext *ctx, GLfloat width);
   /** Specify a logical pixel operation for color index rendering */
   void (*LogicOpcode)(GLcontext *ctx, GLenum opcode);
   void (*PointParameterfv)(GLcontext *ctx, GLenum pname,
                            const GLfloat *params);
   /** Specify the diameter of rasterized points */
   void (*PointSize)(GLcontext *ctx, GLfloat size);
   /** Select a polygon rasterization mode */
   void (*PolygonMode)(GLcontext *ctx, GLenum face, GLenum mode);
   /** Set the scale and units used to calculate depth values */
   void (*PolygonOffset)(GLcontext *ctx, GLfloat factor, GLfloat units);
   /** Set the polygon stippling pattern */
   void (*PolygonStipple)(GLcontext *ctx, const GLubyte *mask );
   /* Specifies the current buffer for reading */
   void (*ReadBuffer)( GLcontext *ctx, GLenum buffer );
   /** Set rasterization mode */
   void (*RenderMode)(GLcontext *ctx, GLenum mode );
   /** Define the scissor box */
   void (*Scissor)(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h);
   /** Select flat or smooth shading */
   void (*ShadeModel)(GLcontext *ctx, GLenum mode);
   /** OpenGL 2.0 two-sided StencilFunc */
   void (*StencilFuncSeparate)(GLcontext *ctx, GLenum face, GLenum func,
                               GLint ref, GLuint mask);
   /** OpenGL 2.0 two-sided StencilMask */
   void (*StencilMaskSeparate)(GLcontext *ctx, GLenum face, GLuint mask);
   /** OpenGL 2.0 two-sided StencilOp */
   void (*StencilOpSeparate)(GLcontext *ctx, GLenum face, GLenum fail,
                             GLenum zfail, GLenum zpass);
   /** Control the generation of texture coordinates */
   void (*TexGen)(GLcontext *ctx, GLenum coord, GLenum pname,
		  const GLfloat *params);
   /** Set texture environment parameters */
   void (*TexEnv)(GLcontext *ctx, GLenum target, GLenum pname,
                  const GLfloat *param);
   /** Set texture parameters */
   void (*TexParameter)(GLcontext *ctx, GLenum target,
                        struct gl_texture_object *texObj,
                        GLenum pname, const GLfloat *params);
   void (*TextureMatrix)(GLcontext *ctx, GLuint unit, const GLmatrix *mat);
   /** Set the viewport */
   void (*Viewport)(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h);
   /*@}*/


   /**
    * \name Vertex array functions
    *
    * Called by the corresponding OpenGL functions.
    */
   /*@{*/
   void (*VertexPointer)(GLcontext *ctx, GLint size, GLenum type,
			 GLsizei stride, const GLvoid *ptr);
   void (*NormalPointer)(GLcontext *ctx, GLenum type,
			 GLsizei stride, const GLvoid *ptr);
   void (*ColorPointer)(GLcontext *ctx, GLint size, GLenum type,
			GLsizei stride, const GLvoid *ptr);
   void (*FogCoordPointer)(GLcontext *ctx, GLenum type,
			   GLsizei stride, const GLvoid *ptr);
   void (*IndexPointer)(GLcontext *ctx, GLenum type,
			GLsizei stride, const GLvoid *ptr);
   void (*SecondaryColorPointer)(GLcontext *ctx, GLint size, GLenum type,
				 GLsizei stride, const GLvoid *ptr);
   void (*TexCoordPointer)(GLcontext *ctx, GLint size, GLenum type,
			   GLsizei stride, const GLvoid *ptr);
   void (*EdgeFlagPointer)(GLcontext *ctx, GLsizei stride, const GLvoid *ptr);
   void (*VertexAttribPointer)(GLcontext *ctx, GLuint index, GLint size,
                               GLenum type, GLsizei stride, const GLvoid *ptr);
   void (*LockArraysEXT)( GLcontext *ctx, GLint first, GLsizei count );
   void (*UnlockArraysEXT)( GLcontext *ctx );
   /*@}*/


   /** 
    * \name State-query functions
    *
    * Return GL_TRUE if query was completed, GL_FALSE otherwise.
    */
   /*@{*/
   /** Return the value or values of a selected parameter */
   GLboolean (*GetBooleanv)(GLcontext *ctx, GLenum pname, GLboolean *result);
   /** Return the value or values of a selected parameter */
   GLboolean (*GetDoublev)(GLcontext *ctx, GLenum pname, GLdouble *result);
   /** Return the value or values of a selected parameter */
   GLboolean (*GetFloatv)(GLcontext *ctx, GLenum pname, GLfloat *result);
   /** Return the value or values of a selected parameter */
   GLboolean (*GetIntegerv)(GLcontext *ctx, GLenum pname, GLint *result);
   /** Return the value or values of a selected parameter */
   GLboolean (*GetPointerv)(GLcontext *ctx, GLenum pname, GLvoid **result);
   /*@}*/
   

   /**
    * \name Vertex/pixel buffer object functions
    */
#if FEATURE_ARB_vertex_buffer_object
   /*@{*/
   void (*BindBuffer)( GLcontext *ctx, GLenum target,
		       struct gl_buffer_object *obj );

   struct gl_buffer_object * (*NewBufferObject)( GLcontext *ctx, GLuint buffer,
						 GLenum target );
   
   void (*DeleteBuffer)( GLcontext *ctx, struct gl_buffer_object *obj );

   void (*BufferData)( GLcontext *ctx, GLenum target, GLsizeiptrARB size,
		       const GLvoid *data, GLenum usage,
		       struct gl_buffer_object *obj );

   void (*BufferSubData)( GLcontext *ctx, GLenum target, GLintptrARB offset,
			  GLsizeiptrARB size, const GLvoid *data,
			  struct gl_buffer_object *obj );

   void (*GetBufferSubData)( GLcontext *ctx, GLenum target,
			     GLintptrARB offset, GLsizeiptrARB size,
			     GLvoid *data, struct gl_buffer_object *obj );

   void * (*MapBuffer)( GLcontext *ctx, GLenum target, GLenum access,
			struct gl_buffer_object *obj );

   GLboolean (*UnmapBuffer)( GLcontext *ctx, GLenum target,
			     struct gl_buffer_object *obj );
   /*@}*/
#endif

   /**
    * \name Functions for GL_EXT_framebuffer_object
    */
#if FEATURE_EXT_framebuffer_object
   /*@{*/
   struct gl_framebuffer * (*NewFramebuffer)(GLcontext *ctx, GLuint name);
   struct gl_renderbuffer * (*NewRenderbuffer)(GLcontext *ctx, GLuint name);
   void (*BindFramebuffer)(GLcontext *ctx, GLenum target,
                           struct gl_framebuffer *fb, struct gl_framebuffer *fbread);
   void (*FramebufferRenderbuffer)(GLcontext *ctx, 
                                   struct gl_framebuffer *fb,
                                   GLenum attachment,
                                   struct gl_renderbuffer *rb);
   void (*RenderTexture)(GLcontext *ctx,
                         struct gl_framebuffer *fb,
                         struct gl_renderbuffer_attachment *att);
   void (*FinishRenderTexture)(GLcontext *ctx,
                               struct gl_renderbuffer_attachment *att);
   /*@}*/
#endif
#if FEATURE_EXT_framebuffer_blit
   void (*BlitFramebuffer)(GLcontext *ctx,
                           GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                           GLbitfield mask, GLenum filter);
#endif

   /**
    * \name Query objects
    */
   /*@{*/
   struct gl_query_object * (*NewQueryObject)(GLcontext *ctx, GLuint id);
   void (*DeleteQuery)(GLcontext *ctx, struct gl_query_object *q);
   void (*BeginQuery)(GLcontext *ctx, struct gl_query_object *q);
   void (*EndQuery)(GLcontext *ctx, struct gl_query_object *q);
   void (*CheckQuery)(GLcontext *ctx, struct gl_query_object *q);
   void (*WaitQuery)(GLcontext *ctx, struct gl_query_object *q);
   /*@}*/


   /**
    * \name Vertex Array objects
    */
   /*@{*/
   struct gl_array_object * (*NewArrayObject)(GLcontext *ctx, GLuint id);
   void (*DeleteArrayObject)(GLcontext *ctx, struct gl_array_object *obj);
   void (*BindArrayObject)(GLcontext *ctx, struct gl_array_object *obj);
   /*@}*/

   /**
    * \name GLSL-related functions (ARB extensions and OpenGL 2.x)
    */
   /*@{*/
   void (*AttachShader)(GLcontext *ctx, GLuint program, GLuint shader);
   void (*BindAttribLocation)(GLcontext *ctx, GLuint program, GLuint index,
                              const GLcharARB *name);
   void (*CompileShader)(GLcontext *ctx, GLuint shader);
   GLuint (*CreateShader)(GLcontext *ctx, GLenum type);
   GLuint (*CreateProgram)(GLcontext *ctx);
   void (*DeleteProgram2)(GLcontext *ctx, GLuint program);
   void (*DeleteShader)(GLcontext *ctx, GLuint shader);
   void (*DetachShader)(GLcontext *ctx, GLuint program, GLuint shader);
   void (*GetActiveAttrib)(GLcontext *ctx, GLuint program, GLuint index,
                           GLsizei maxLength, GLsizei * length, GLint * size,
                           GLenum * type, GLcharARB * name);
   void (*GetActiveUniform)(GLcontext *ctx, GLuint program, GLuint index,
                            GLsizei maxLength, GLsizei *length, GLint *size,
                            GLenum *type, GLcharARB *name);
   void (*GetAttachedShaders)(GLcontext *ctx, GLuint program, GLsizei maxCount,
                              GLsizei *count, GLuint *obj);
   GLint (*GetAttribLocation)(GLcontext *ctx, GLuint program,
                              const GLcharARB *name);
   GLuint (*GetHandle)(GLcontext *ctx, GLenum pname);
   void (*GetProgramiv)(GLcontext *ctx, GLuint program,
                        GLenum pname, GLint *params);
   void (*GetProgramInfoLog)(GLcontext *ctx, GLuint program, GLsizei bufSize,
                             GLsizei *length, GLchar *infoLog);
   void (*GetShaderiv)(GLcontext *ctx, GLuint shader,
                       GLenum pname, GLint *params);
   void (*GetShaderInfoLog)(GLcontext *ctx, GLuint shader, GLsizei bufSize,
                            GLsizei *length, GLchar *infoLog);
   void (*GetShaderSource)(GLcontext *ctx, GLuint shader, GLsizei maxLength,
                           GLsizei *length, GLcharARB *sourceOut);
   void (*GetUniformfv)(GLcontext *ctx, GLuint program, GLint location,
                        GLfloat *params);
   void (*GetUniformiv)(GLcontext *ctx, GLuint program, GLint location,
                        GLint *params);
   GLint (*GetUniformLocation)(GLcontext *ctx, GLuint program,
                               const GLcharARB *name);
   GLboolean (*IsProgram)(GLcontext *ctx, GLuint name);
   GLboolean (*IsShader)(GLcontext *ctx, GLuint name);
   void (*LinkProgram)(GLcontext *ctx, GLuint program);
   void (*ShaderSource)(GLcontext *ctx, GLuint shader, const GLchar *source);
   void (*Uniform)(GLcontext *ctx, GLint location, GLsizei count,
                   const GLvoid *values, GLenum type);
   void (*UniformMatrix)(GLcontext *ctx, GLint cols, GLint rows,
                         GLenum matrixType, GLint location, GLsizei count,
                         GLboolean transpose, const GLfloat *values);
   void (*UseProgram)(GLcontext *ctx, GLuint program);
   void (*ValidateProgram)(GLcontext *ctx, GLuint program);
   /* XXX many more to come */
   /*@}*/


   /**
    * \name Support for multiple T&L engines
    */
   /*@{*/

   /**
    * Bitmask of state changes that require the current T&L module to be
    * validated, using ValidateTnlModule() below.
    */
   GLuint NeedValidate;

   /**
    * Validate the current T&L module. 
    *
    * This is called directly after UpdateState() when a state change that has
    * occurred matches the dd_function_table::NeedValidate bitmask above.  This
    * ensures all computed values are up to date, thus allowing the driver to
    * decide if the current T&L module needs to be swapped out.
    *
    * This must be non-NULL if a driver installs a custom T&L module and sets
    * the dd_function_table::NeedValidate bitmask, but may be NULL otherwise.
    */
   void (*ValidateTnlModule)( GLcontext *ctx, GLuint new_state );


#define PRIM_OUTSIDE_BEGIN_END   (GL_POLYGON+1)
#define PRIM_INSIDE_UNKNOWN_PRIM (GL_POLYGON+2)
#define PRIM_UNKNOWN             (GL_POLYGON+3)

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
    * between glBegin()/glEnd() objects or __GLcontextRec::Current is not
    * updated.
    *
    * The dd_function_table::FlushVertices call below may be used to resolve
    * these conditions.
    */
   GLuint NeedFlush;
   GLuint SaveNeedFlush;

   /**
    * If inside glBegin()/glEnd(), it should ASSERT(0).  Otherwise, if
    * FLUSH_STORED_VERTICES bit in \p flags is set flushes any buffered
    * vertices, if FLUSH_UPDATE_CURRENT bit is set updates
    * __GLcontextRec::Current and gl_light_attrib::Material
    *
    * Note that the default T&L engine never clears the
    * FLUSH_UPDATE_CURRENT bit, even after performing the update.
    */
   void (*FlushVertices)( GLcontext *ctx, GLuint flags );
   void (*SaveFlushVertices)( GLcontext *ctx );

   /**
    * Give the driver the opportunity to hook in its own vtxfmt for
    * compiling optimized display lists.  This is called on each valid
    * glBegin() during list compilation.
    */
   GLboolean (*NotifySaveBegin)( GLcontext *ctx, GLenum mode );

   /**
    * Notify driver that the special derived value _NeedEyeCoords has
    * changed.
    */
   void (*LightingSpaceChange)( GLcontext *ctx );

   /**
    * Called by glNewList().
    *
    * Let the T&L component know what is going on with display lists
    * in time to make changes to dispatch tables, etc.
    */
   void (*NewList)( GLcontext *ctx, GLuint list, GLenum mode );
   /**
    * Called by glEndList().
    *
    * \sa dd_function_table::NewList.
    */
   void (*EndList)( GLcontext *ctx );

   /**
    * Called by glCallList(s).
    *
    * Notify the T&L component before and after calling a display list.
    */
   void (*BeginCallList)( GLcontext *ctx, 
			  struct mesa_display_list *dlist );
   /**
    * Called by glEndCallList().
    *
    * \sa dd_function_table::BeginCallList.
    */
   void (*EndCallList)( GLcontext *ctx );

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
 * the vertexformat to install it's own initial values for these
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
   void (GLAPIENTRYP ArrayElement)( GLint ); /* NOTE */
   void (GLAPIENTRYP Color3f)( GLfloat, GLfloat, GLfloat );
   void (GLAPIENTRYP Color3fv)( const GLfloat * );
   void (GLAPIENTRYP Color4f)( GLfloat, GLfloat, GLfloat, GLfloat );
   void (GLAPIENTRYP Color4fv)( const GLfloat * );
   void (GLAPIENTRYP EdgeFlag)( GLboolean );
   void (GLAPIENTRYP EvalCoord1f)( GLfloat );          /* NOTE */
   void (GLAPIENTRYP EvalCoord1fv)( const GLfloat * ); /* NOTE */
   void (GLAPIENTRYP EvalCoord2f)( GLfloat, GLfloat ); /* NOTE */
   void (GLAPIENTRYP EvalCoord2fv)( const GLfloat * ); /* NOTE */
   void (GLAPIENTRYP EvalPoint1)( GLint );             /* NOTE */
   void (GLAPIENTRYP EvalPoint2)( GLint, GLint );      /* NOTE */
   void (GLAPIENTRYP FogCoordfEXT)( GLfloat );
   void (GLAPIENTRYP FogCoordfvEXT)( const GLfloat * );
   void (GLAPIENTRYP Indexf)( GLfloat );
   void (GLAPIENTRYP Indexfv)( const GLfloat * );
   void (GLAPIENTRYP Materialfv)( GLenum face, GLenum pname, const GLfloat * ); /* NOTE */
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
   void (GLAPIENTRYP CallList)( GLuint );	/* NOTE */
   void (GLAPIENTRYP CallLists)( GLsizei, GLenum, const GLvoid * );	/* NOTE */
   void (GLAPIENTRYP Begin)( GLenum );
   void (GLAPIENTRYP End)( void );
   /* GL_NV_vertex_program */
   void (GLAPIENTRYP VertexAttrib1fNV)( GLuint index, GLfloat x );
   void (GLAPIENTRYP VertexAttrib1fvNV)( GLuint index, const GLfloat *v );
   void (GLAPIENTRYP VertexAttrib2fNV)( GLuint index, GLfloat x, GLfloat y );
   void (GLAPIENTRYP VertexAttrib2fvNV)( GLuint index, const GLfloat *v );
   void (GLAPIENTRYP VertexAttrib3fNV)( GLuint index, GLfloat x, GLfloat y, GLfloat z );
   void (GLAPIENTRYP VertexAttrib3fvNV)( GLuint index, const GLfloat *v );
   void (GLAPIENTRYP VertexAttrib4fNV)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
   void (GLAPIENTRYP VertexAttrib4fvNV)( GLuint index, const GLfloat *v );
#if FEATURE_ARB_vertex_program
   void (GLAPIENTRYP VertexAttrib1fARB)( GLuint index, GLfloat x );
   void (GLAPIENTRYP VertexAttrib1fvARB)( GLuint index, const GLfloat *v );
   void (GLAPIENTRYP VertexAttrib2fARB)( GLuint index, GLfloat x, GLfloat y );
   void (GLAPIENTRYP VertexAttrib2fvARB)( GLuint index, const GLfloat *v );
   void (GLAPIENTRYP VertexAttrib3fARB)( GLuint index, GLfloat x, GLfloat y, GLfloat z );
   void (GLAPIENTRYP VertexAttrib3fvARB)( GLuint index, const GLfloat *v );
   void (GLAPIENTRYP VertexAttrib4fARB)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
   void (GLAPIENTRYP VertexAttrib4fvARB)( GLuint index, const GLfloat *v );
#endif
   /*@}*/

   /*
    */
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
