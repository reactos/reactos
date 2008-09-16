/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


/* Allegro (DJGPP) driver by Bernhard Tschirren (bernie-t@geocities.com) */


#ifndef AMESA_H
#define AMESA_H


#define AMESA_MAJOR_VERSION 3
#define AMESA_MINOR_VERSION 3


typedef struct amesa_visual  *AMesaVisual;
typedef struct amesa_buffer  *AMesaBuffer;
typedef struct amesa_context *AMesaContext;


extern AMesaVisual AMesaCreateVisual(GLboolean dbFlag, GLint depth,
                                     GLint depthSize,
                                     GLint stencilSize,
                                     GLint accumSize);

extern void AMesaDestroyVisual(AMesaVisual visual);

extern AMesaBuffer AMesaCreateBuffer(AMesaVisual visual,
                                     GLint width, GLint height);

extern void AMesaDestroyBuffer(AMesaBuffer buffer);


extern AMesaContext AMesaCreateContext(AMesaVisual visual,
                                       AMesaContext sharelist);

extern void AMesaDestroyContext(AMesaContext context);

extern GLboolean AMesaMakeCurrent(AMesaContext context, AMesaBuffer buffer);

extern void AMesaSwapBuffers(AMesaBuffer buffer);


#endif /* AMESA_H */
