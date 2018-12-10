/* $Id: dd.h,v 1.15 1997/12/05 04:38:28 brianp Exp $ */

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
 * $Log: dd.h,v $
 * Revision 1.15  1997/12/05 04:38:28  brianp
 * added ClearColorAndDepth() function pointer (David Bucciarelli)
 *
 * Revision 1.14  1997/11/25 03:39:41  brianp
 * added TexSubImage() function pointer (David Bucciarelli)
 *
 * Revision 1.13  1997/10/29 02:23:54  brianp
 * added UseGlobalTexturePalette() dd function (David Bucciarelli v20 3dfx)
 *
 * Revision 1.12  1997/10/16 02:27:53  brianp
 * more comments
 *
 * Revision 1.11  1997/10/14 00:07:40  brianp
 * added DavidB's fxmesa v19 changes
 *
 * Revision 1.10  1997/09/29 23:26:50  brianp
 * new texture functions
 *
 * Revision 1.9  1997/04/29 01:31:07  brianp
 * added RasterSetup() function to device driver
 *
 * Revision 1.8  1997/04/20 19:47:27  brianp
 * added RenderVB to device driver
 *
 * Revision 1.7  1997/04/20 16:31:08  brianp
 * added NearFar device driver function
 *
 * Revision 1.6  1997/04/12 12:27:31  brianp
 * added QuadFunc and RectFunc
 *
 * Revision 1.5  1997/03/21 01:57:07  brianp
 * added RendererString() function
 *
 * Revision 1.4  1997/02/10 19:22:47  brianp
 * added device driver Error() function
 *
 * Revision 1.3  1997/01/16 03:34:33  brianp
 * added preliminary texture mapping functions and cleaned up documentation
 *
 * Revision 1.2  1996/11/13 03:51:59  brianp
 * updated comments
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


#ifndef DD_INCLUDED
#define DD_INCLUDED


/* THIS FILE ONLY INCLUDED BY types.h !!!!! */


/*
 *                      Device Driver (DD) interface
 *
 *
 * All device driver functions are accessed through pointers in the
 * dd_function_table struct (defined below) which is stored in the GLcontext
 * struct.  Since the device driver is strictly accessed trough a table of
 * function pointers we can:
 *   1. switch between a number of different device drivers at runtime.
 *   2. use optimized functions dependant on current rendering state or
 *      frame buffer configuration.
 *
 * The function pointers in the dd_function_table struct are divided into
 * two groups:  mandatory and optional.
 * Mandatory functions have to be implemented by every device driver.
 * Optional functions may or may not be implemented by the device driver.
 * The optional functions provide ways to take advantage of special hardware
 * or optimized algorithms.
 *
 * The function pointers in the dd_function_table struct are first
 * initialized in the "MakeCurrent" function.  The "MakeCurrent" function
 * is a little different in each device driver.  See the X/Mesa, GLX, or
 * OS/Mesa drivers for examples.
 *
 * Later, Mesa may call the dd_function_table's UpdateState() function.
 * This function should initialize the dd_function_table's pointers again.
 * The UpdateState() function is called whenever the core (GL) rendering
 * state is changed in a way which may effect rasterization.  For example,
 * the TriangleFunc() pointer may have to point to different functions
 * depending on whether smooth or flat shading is enabled.
 *
 * Note that the first argument to every device driver function is a
 * GLcontext *.  In turn, the GLcontext->DriverCtx pointer points to
 * the driver-specific context struct.  See the X/Mesa or OS/Mesa interface
 * for an example.
 *
 * For more information about writing a device driver see the ddsample.c
 * file and other device drivers (xmesa[123].c, osmesa.c, etc) for examples.
 *
 *
 * Look below in the dd_function_table struct definition for descriptions
 * of each device driver function.
 * 
 *
 * In the future more function pointers may be added for glReadPixels
 * glCopyPixels, etc.
 *
 *
 * Notes:
 * ------
 *   RGBA = red/green/blue/alpha
 *   CI = color index (color mapped mode)
 *   mono = all pixels have the same color or index
 *
 *   The write_ functions all take an array of mask flags which indicate
 *   whether or not the pixel should be written.  One special case exists
 *   in the write_color_span function: if the mask array is NULL, then
 *   draw all pixels.  This is an optimization used for glDrawPixels().
 *
 * IN ALL CASES:
 *      X coordinates start at 0 at the left and increase to the right
 *      Y coordinates start at 0 at the bottom and increase upward
 *
 */





/*
 * Device Driver function table.
 */
struct dd_function_table {

   /**********************************************************************
    *** Mandatory functions:  these functions must be implemented by   ***
    *** every device driver.                                           ***
    **********************************************************************/

   const char * (*RendererString)(void);
   /*
    * Return a string which uniquely identifies this device driver.
    * The string should contain no whitespace.  Examples: "X11" "OffScreen"
    * "MSWindows" "SVGA".
    */

   void (*UpdateState)( GLcontext *ctx );
   /*
    * UpdateState() is called whenver Mesa thinks the device driver should
    * update its state and/or the other pointers (such as PointsFunc,
    * LineFunc, or TriangleFunc).
    */

   void (*ClearIndex)( GLcontext *ctx, GLuint index );
   /*
    * Called whenever glClearIndex() is called.  Set the index for clearing
    * the color buffer.
    */

   void (*ClearColor)( GLcontext *ctx, GLubyte red, GLubyte green,
                                        GLubyte blue, GLubyte alpha );
   /*
    * Called whenever glClearColor() is called.  Set the color for clearing
    * the color buffer.
    */

   void (*Clear)( GLcontext *ctx,
                  GLboolean all, GLint x, GLint y, GLint width, GLint height );
   /*
    * Clear the current color buffer.  If 'all' is set the clear the whole
    * buffer, else clear the region defined by (x,y,width,height).
    */

   void (*Index)( GLcontext *ctx, GLuint index );
   /*
    * Sets current color index for drawing flat-shaded primitives.
    */

   void (*Color)( GLcontext *ctx,
                  GLubyte red, GLubyte green, GLubyte glue, GLubyte alpha );
   /*
    * Sets current color for drawing flat-shaded primitives.
    */

   GLboolean (*SetBuffer)( GLcontext *ctx, GLenum mode );
   /*
    * Selects either the front or back color buffer for reading and writing.
    * mode is either GL_FRONT or GL_BACK.
    */

   void (*GetBufferSize)( GLcontext *ctx,
                          GLuint *width, GLuint *height );
   /*
    * Returns the width and height of the current color buffer.
    */


   /***
    *** Functions for writing pixels to the frame buffer:
    ***/
   void (*WriteColorSpan)( GLcontext *ctx,
                           GLuint n, GLint x, GLint y,
			   const GLubyte red[], const GLubyte green[],
			   const GLubyte blue[], const GLubyte alpha[],
			   const GLubyte mask[] );
   /*
    * Write a horizontal run of RGBA pixels.
    */

   void (*WriteMonocolorSpan)( GLcontext *ctx,
                               GLuint n, GLint x, GLint y,
			       const GLubyte mask[] );
   /*
    * Write a horizontal run of mono-RGBA pixels.
    */

   void (*WriteColorPixels)( GLcontext *ctx,
                             GLuint n, const GLint x[], const GLint y[],
			     const GLubyte red[], const GLubyte green[],
			     const GLubyte blue[], const GLubyte alpha[],
			     const GLubyte mask[] );
   /*
    * Write an array of RGBA pixels at random locations.
    */

   void (*WriteMonocolorPixels)( GLcontext *ctx,
                                 GLuint n, const GLint x[], const GLint y[],
				 const GLubyte mask[] );
   /*
    * Write an array of mono-RGBA pixels at random locations.
    */

   void (*WriteIndexSpan)( GLcontext *ctx,
                           GLuint n, GLint x, GLint y, const GLuint index[],
                           const GLubyte mask[] );
   /*
    * Write a horizontal run of CI pixels.
    */

   void (*WriteMonoindexSpan)( GLcontext *ctx,
                               GLuint n, GLint x, GLint y,
			       const GLubyte mask[] );
   /*
    * Write a horizontal run of mono-CI pixels.
    */

   void (*WriteIndexPixels)( GLcontext *ctx,
                             GLuint n, const GLint x[], const GLint y[],
                             const GLuint index[], const GLubyte mask[] );
   /*
    * Write a random array of CI pixels.
    */

   void (*WriteMonoindexPixels)( GLcontext *ctx,
                                 GLuint n, const GLint x[], const GLint y[],
				 const GLubyte mask[] );
   /*
    * Write a random array of mono-CI pixels.
    */

   /***
    *** Functions to read pixels from frame buffer:
    ***/
   void (*ReadIndexSpan)( GLcontext *ctx,
                          GLuint n, GLint x, GLint y, GLuint index[] );
   /*
    * Read a horizontal run of color index pixels.
    */

   void (*ReadColorSpan)( GLcontext *ctx,
                          GLuint n, GLint x, GLint y,
			  GLubyte red[], GLubyte green[],
			  GLubyte blue[], GLubyte alpha[] );
   /*
    * Read a horizontal run of RGBA pixels.
    */

   void (*ReadIndexPixels)( GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
			    GLuint indx[], const GLubyte mask[] );
   /*
    * Read a random array of CI pixels.
    */

   void (*ReadColorPixels)( GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
			    GLubyte red[], GLubyte green[],
			    GLubyte blue[], GLubyte alpha[],
                            const GLubyte mask[] );
   /*
    * Read a random array of RGBA pixels.
    */


   /**********************************************************************
    *** Optional functions:  these functions may or may not be         ***
    *** implemented by the device driver.  If the device driver        ***
    *** doesn't implement them it should never touch these pointers    ***
    *** since Mesa will either set them to NULL or point them at a     ***
    *** fall-back function.                                            ***
    **********************************************************************/

   void (*Finish)( GLcontext *ctx );
   /*
    * Called whenever glFinish() is called.
    */

   void (*Flush)( GLcontext *ctx );
   /*
    * Called whenever glFlush() is called.
    */

   GLboolean (*IndexMask)( GLcontext *ctx, GLuint mask );
   /*
    * Implements glIndexMask() if possible, else return GL_FALSE.
    */

   GLboolean (*ColorMask)( GLcontext *ctx,
                           GLboolean rmask, GLboolean gmask,
                           GLboolean bmask, GLboolean amask );
   /*
    * Implements glColorMask() if possible, else return GL_FALSE.
    */

   GLboolean (*LogicOp)( GLcontext *ctx, GLenum op );
   /*
    * Implements glLogicOp() if possible, else return GL_FALSE.
    */

   void (*Dither)( GLcontext *ctx, GLboolean enable );
   /*
    * Enable/disable dithering.
    */

   void (*Error)( GLcontext *ctx );
   /*
    * Called whenever an error is generated.  ctx->ErrorValue contains
    * the error value.
    */

   void (*NearFar)( GLcontext *ctx, GLfloat nearVal, GLfloat farVal );
   /*
    * Called from glFrustum and glOrtho to tell device driver the
    * near and far clipping plane Z values.  The 3Dfx driver, for example,
    * uses this.
    */


   /***
    *** For supporting hardware Z buffers:
    ***/

   void (*AllocDepthBuffer)( GLcontext *ctx );
   /*
    * Called when the depth buffer must be allocated or possibly resized.
    */

   void (*ClearDepthBuffer)( GLcontext *ctx );
   /*
    * Clear the depth buffer to depth specified by CC.Depth.Clear value.
    */

   GLuint (*DepthTestSpan)( GLcontext *ctx,
                            GLuint n, GLint x, GLint y, const GLdepth z[],
                            GLubyte mask[] );
   void (*DepthTestPixels)( GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLdepth z[], GLubyte mask[] );
   /*
    * Apply the depth buffer test to an span/array of pixels and return
    * an updated pixel mask.  This function is not used when accelerated
    * point, line, polygon functions are used.
    */

   void (*ReadDepthSpanFloat)( GLcontext *ctx,
                               GLuint n, GLint x, GLint y, GLfloat depth[]);
   void (*ReadDepthSpanInt)( GLcontext *ctx,
                             GLuint n, GLint x, GLint y, GLdepth depth[] );
   /*
    * Return depth values as integers for glReadPixels.
    * Floats should be returned in the range [0,1].
    * Ints (GLdepth) values should be in the range [0,MAXDEPTH].
    */


   void (*ClearColorAndDepth)( GLcontext *ctx,
                               GLboolean all, GLint x, GLint y,
                               GLint width, GLint height );
   /*
    * If this function is implemented then it will be called when both the
    * color and depth buffers are to be cleared.
    */


   /***
    *** Accelerated point, line, polygon, glDrawPixels and glBitmap functions:
    ***/

   points_func PointsFunc;
   /*
    * Called to draw an array of points.
    */

   line_func LineFunc;
   /*
    * Called to draw a line segment.
    */

   triangle_func TriangleFunc;
   /* 
    * Called to draw a filled triangle.
    */

   quad_func QuadFunc;
   /* 
    * Called to draw a filled quadrilateral.
    */

   rect_func RectFunc;
   /* 
    * Called to draw a filled, screen-aligned 2-D rectangle.
    */

   GLboolean (*DrawPixels)( GLcontext *ctx,
                            GLint x, GLint y, GLsizei width, GLsizei height,
                            GLenum format, GLenum type, GLboolean packed,
                            const GLvoid *pixels );
   /*
    * Called from glDrawPixels().
    */

   GLboolean (*Bitmap)( GLcontext *ctx, GLsizei width, GLsizei height,
                        GLfloat xorig, GLfloat yorig,
                        GLfloat xmove, GLfloat ymove,
                        const struct gl_image *bitmap );
   /*
    * Called from glBitmap().
    */

   void (*Begin)( GLcontext *ctx, GLenum mode );
   void (*End)( GLcontext *ctx );
   /*
    * These are called whenever glBegin() or glEnd() are called.
    * The device driver may do some sort of window locking/unlocking here.
    */


   void (*RasterSetup)( GLcontext *ctx, GLuint start, GLuint end );
   /*
    * This function, if not NULL, is called whenever new window coordinates
    * are put in the vertex buffer.  The vertices in question are those n
    * such that start <= n < end.
    * The device driver can convert the window coords to its own specialized
    * format.  The 3Dfx driver uses this.
    */

   GLboolean (*RenderVB)( GLcontext *ctx, GLboolean allDone );
   /*
    * This function allows the device driver to rasterize an entire
    * buffer of primitives at once.  See the gl_render_vb() function
    * in vbrender.c for more details.
    * Return GL_TRUE if vertex buffer successfully rendered.
    * Return GL_FALSE if failure, let core Mesa render the vertex buffer.
    */

   /***
    *** Texture mapping functions:
    ***/

   void (*TexEnv)( GLcontext *ctx, GLenum pname, const GLfloat *param );
   /*
    * Called whenever glTexEnv*() is called.
    * Pname will be one of GL_TEXTURE_ENV_MODE or GL_TEXTURE_ENV_COLOR.
    * If pname is GL_TEXTURE_ENV_MODE then param will be one
    * of GL_MODULATE, GL_BLEND, GL_DECAL, or GL_REPLACE.
    */

   void (*TexImage)( GLcontext *ctx, GLenum target,
                     struct gl_texture_object *tObj, GLint level,
                     GLint internalFormat,
                     const struct gl_texture_image *image );
   /*
    * Called whenever a texture object's image is changed.
    *    texObject is the number of the texture object being changed.
    *    level indicates the mipmap level.
    *    internalFormat is the format in which the texture is to be stored.
    *    image is a pointer to a gl_texture_image struct which contains
    *       the actual image data.
    */

   void (*TexSubImage)( GLcontext *ctx, GLenum target,
                        struct gl_texture_object *tObj, GLint level,
                        GLint xoffset, GLint yoffset,
                        GLsizei width, GLsizei height,
                        GLint internalFormat,
                        const struct gl_texture_image *image );
   /*
    * Called from glTexSubImage() to define a sub-region of a texture.
    */

   void (*TexParameter)( GLcontext *ctx, GLenum target,
                         struct gl_texture_object *tObj,
                         GLenum pname, const GLfloat *params );
   /*
    * Called whenever glTexParameter*() is called.
    *    target is GL_TEXTURE_1D or GL_TEXTURE_2D
    *    texObject is the texture object to modify
    *    pname is one of GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER,
    *       GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, or GL_TEXTURE_BORDER_COLOR.
    *    params is dependant on pname.  See man glTexParameter.
    */

   void (*BindTexture)( GLcontext *ctx, GLenum target,
                        struct gl_texture_object *tObj );
   /*
    * Called whenever glBindTexture() is called.  This specifies which
    * texture is to be the current one.
    */

   void (*DeleteTexture)( GLcontext *ctx, struct gl_texture_object *tObj );
   /*
    * Called when a texture object can be deallocated.
    */

   void (*UpdateTexturePalette)( GLcontext *ctx,
                                 struct gl_texture_object *tObj );
   /*
    * Called when the texture's color lookup table is changed.
    * If tObj is NULL then the shared texture palette ctx->Texture.Palette
    * was changed.
    */
};



#endif

