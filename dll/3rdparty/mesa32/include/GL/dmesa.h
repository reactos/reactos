/*
 * Mesa 3-D graphics library
 * Version:  6.1
 * 
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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

/*
 * DOS/DJGPP device driver for Mesa
 *
 *  Author: Daniel Borca
 *  Email : dborca@users.sourceforge.net
 *  Web   : http://www.geocities.com/dborca
 */


#ifndef DMESA_H_included
#define DMESA_H_included

#define DMESA_MAJOR_VERSION 6
#define DMESA_MINOR_VERSION 5

/* Sample Usage:
 *
 * 1. Call DMesaCreateVisual() to initialize graphics.
 * 2. Call DMesaCreateContext() to create a DMesa rendering context.
 * 3. Call DMesaCreateBuffer() to define the window.
 * 4. Call DMesaMakeCurrent() to bind the DMesaBuffer to a DMesaContext.
 * 5. Make gl* calls to render your graphics.
 * 6. Use DMesaSwapBuffers() when double buffering to swap front/back buffers.
 * 7. Before exiting, destroy DMesaBuffer, DMesaContext and DMesaVisual.
 */

typedef struct dmesa_context *DMesaContext;
typedef struct dmesa_visual *DMesaVisual;
typedef struct dmesa_buffer *DMesaBuffer;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Create a new Visual and set graphics mode.
 */
DMesaVisual DMesaCreateVisual (GLint width,        /* X res */
                               GLint height,       /* Y res */
                               GLint colDepth,     /* BPP */
                               GLint refresh,      /* refresh rate: 0=default */
                               GLboolean dbFlag,   /* double-buffered */
                               GLboolean rgbFlag,  /* RGB mode */
                               GLint alphaSize,    /* requested bits/alpha */
                               GLint depthSize,    /* requested bits/depth */
                               GLint stencilSize,  /* requested bits/stencil */
                               GLint accumSize);   /* requested bits/accum */

/*
 * Destroy Visual and restore screen.
 */
void DMesaDestroyVisual (DMesaVisual v);



/*
 * Create a new Context for rendering.
 */
DMesaContext DMesaCreateContext (DMesaVisual visual, DMesaContext share);

/*
 * Destroy Context.
 */
void DMesaDestroyContext (DMesaContext c);

/*
 * Return a handle to the current context.
 */
DMesaContext DMesaGetCurrentContext (void);



/*
 * Create a new Buffer (window).
 */
DMesaBuffer DMesaCreateBuffer (DMesaVisual visual,
                               GLint xpos, GLint ypos,
                               GLint width, GLint height);

/*
 * Destroy Buffer.
 */
void DMesaDestroyBuffer (DMesaBuffer b);

/*
 * Return a handle to the current buffer.
 */
DMesaBuffer DMesaGetCurrentBuffer (void);

/*
 * Swap the front and back buffers for the given Buffer.
 * No action is taken if the buffer is not double buffered.
 */
void DMesaSwapBuffers (DMesaBuffer b);

/*
 * Bind Buffer to Context and make the Context the current one.
 */
GLboolean DMesaMakeCurrent (DMesaContext c, DMesaBuffer b);



/*
 * Move/Resize current Buffer.
 */
GLboolean DMesaMoveBuffer (GLint xpos, GLint ypos);
GLboolean DMesaResizeBuffer (GLint width, GLint height);

/*
 * Set palette index, using normalized values.
 */
void DMesaSetCI (int ndx, GLfloat red, GLfloat green, GLfloat blue);

/*
 * DMesa functions
 */
typedef void (*DMesaProc) ();
DMesaProc DMesaGetProcAddress (const char *name);

/*
 * DMesa state retrieval.
 */
#define DMESA_GET_SCREEN_SIZE 0x0100
#define DMESA_GET_DRIVER_CAPS 0x0200
#define DMESA_GET_VIDEO_MODES 0x0300
#define DMESA_GET_BUFFER_ADDR 0x0400

#define DMESA_DRIVER_DBL_BIT 0x1 /* double-buffered */
#define DMESA_DRIVER_YUP_BIT 0x2 /* lower-left window origin */
int DMesaGetIntegerv (GLenum pname, GLint *params);

#ifdef __cplusplus
}
#endif

#endif
