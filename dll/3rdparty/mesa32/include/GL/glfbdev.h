/*
 * Mesa 3-D graphics library
 * Version:  6.5
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


#ifndef GLFBDEV_H
#define GLFBDEV_H


/* avoid including linux/fb.h */
struct fb_fix_screeninfo;
struct fb_var_screeninfo;


/* public types */
typedef struct GLFBDevVisualRec *GLFBDevVisualPtr;
typedef struct GLFBDevBufferRec *GLFBDevBufferPtr;
typedef struct GLFBDevContextRec *GLFBDevContextPtr;


/* API version */
#define GLFBDEV_VERSION_1_0       1


/* For glFBDevCreateVisual */
#define GLFBDEV_DOUBLE_BUFFER   100
#define GLFBDEV_COLOR_INDEX     101
#define GLFBDEV_DEPTH_SIZE      102
#define GLFBDEV_STENCIL_SIZE    103
#define GLFBDEV_ACCUM_SIZE      104
#define GLFBDEV_LEVEL           105
#define GLFBDEV_MULTISAMPLE     106
#define GLFBDEV_NONE              0

/* For glFBDevGetString */
#define GLFBDEV_VERSION         200
#define GLFBDEV_VENDOR          201


/* Misc functions */

extern const char *
glFBDevGetString( int str );


typedef void (*GLFBDevProc)();


extern GLFBDevProc
glFBDevGetProcAddress( const char *procName );



/**
 * Create a GLFBDevVisual.
 * \param fixInfo - needed to get the visual types, etc.
 * \param varInfo - needed to get the bits_per_pixel, etc.
 * \param attribs - for requesting depth, stencil, accum buffers, etc.
 */
extern GLFBDevVisualPtr
glFBDevCreateVisual( const struct fb_fix_screeninfo *fixInfo,
                     const struct fb_var_screeninfo *varInfo,
                     const int *attribs );

extern void
glFBDevDestroyVisual( GLFBDevVisualPtr visual );

extern int
glFBDevGetVisualAttrib( const GLFBDevVisualPtr visual, int attrib);



/**
 * Create a GLFBDevBuffer.
 * \param fixInfo, varInfo - needed in order to get the screen size
 * (resolution), etc.
 * \param visual - as returned by glFBDevCreateVisual()
 * \param frontBuffer - address of front color buffer
 * \param backBuffer - address of back color buffer (may be NULL)
 * \param size - size of the color buffer(s) in bytes.
 */
extern GLFBDevBufferPtr
glFBDevCreateBuffer( const struct fb_fix_screeninfo *fixInfo,
                     const struct fb_var_screeninfo *varInfo,
                     const GLFBDevVisualPtr visual,
                     void *frontBuffer, void *backBuffer, size_t size );

extern void
glFBDevDestroyBuffer( GLFBDevBufferPtr buffer );

extern int
glFBDevGetBufferAttrib( const GLFBDevBufferPtr buffer, int attrib);

extern GLFBDevBufferPtr
glFBDevGetCurrentDrawBuffer( void );

extern GLFBDevBufferPtr
glFBDevGetCurrentReadBuffer( void );

extern void
glFBDevSwapBuffers( GLFBDevBufferPtr buffer );



/**
 * Create a GLFBDevContext.
 * \param visual - as created by glFBDevCreateVisual.
 * \param share - specifies another context with which to share textures,
 * display lists, etc. (may be NULL).
 */
extern GLFBDevContextPtr
glFBDevCreateContext( const GLFBDevVisualPtr visual, GLFBDevContextPtr share );

extern void
glFBDevDestroyContext( GLFBDevContextPtr context );

extern int
glFBDevGetContextAttrib( const GLFBDevContextPtr context, int attrib);

extern GLFBDevContextPtr
glFBDevGetCurrentContext( void );

extern int
glFBDevMakeCurrent( GLFBDevContextPtr context,
                    GLFBDevBufferPtr drawBuffer,
                    GLFBDevBufferPtr readBuffer );


#endif /* GLFBDEV_H */
