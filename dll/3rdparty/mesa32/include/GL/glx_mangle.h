/*
 * Mesa 3-D graphics library
 * Version:  6.5
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


#ifndef GLX_MANGLE_H
#define GLX_MANGLE_H

#define glXChooseVisual mglXChooseVisual
#define glXCreateContext mglXCreateContext
#define glXDestroyContext mglXDestroyContext
#define glXMakeCurrent mglXMakeCurrent
#define glXCopyContext mglXCopyContext
#define glXSwapBuffers mglXSwapBuffers
#define glXCreateGLXPixmap mglXCreateGLXPixmap
#define glXDestroyGLXPixmap mglXDestroyGLXPixmap
#define glXQueryExtension mglXQueryExtension
#define glXQueryVersion mglXQueryVersion
#define glXIsDirect mglXIsDirect
#define glXGetConfig mglXGetConfig
#define glXGetCurrentContext mglXGetCurrentContext
#define glXGetCurrentDrawable mglXGetCurrentDrawable
#define glXWaitGL mglXWaitGL
#define glXWaitX mglXWaitX
#define glXUseXFont mglXUseXFont
#define glXQueryExtensionsString mglXQueryExtensionsString
#define glXQueryServerString mglXQueryServerString
#define glXGetClientString mglXGetClientString
#define glXCreateGLXPixmapMESA mglXCreateGLXPixmapMESA
#define glXReleaseBuffersMESA mglXReleaseBuffersMESA
#define glXCopySubBufferMESA mglXCopySubBufferMESA
#define glXGetVideoSyncSGI mglXGetVideoSyncSGI
#define glXWaitVideoSyncSGI mglXWaitVideoSyncSGI

/* GLX 1.2 */
#define glXGetCurrentDisplay mglXGetCurrentDisplay 

/* GLX 1.3 */
#define glXChooseFBConfig mglXChooseFBConfig          
#define glXGetFBConfigAttrib mglXGetFBConfigAttrib       
#define glXGetFBConfigs mglXGetFBConfigs            
#define glXGetVisualFromFBConfig mglXGetVisualFromFBConfig   
#define glXCreateWindow mglXCreateWindow            
#define glXDestroyWindow mglXDestroyWindow           
#define glXCreatePixmap mglXCreatePixmap            
#define glXDestroyPixmap mglXDestroyPixmap           
#define glXCreatePbuffer mglXCreatePbuffer           
#define glXDestroyPbuffer mglXDestroyPbuffer          
#define glXQueryDrawable mglXQueryDrawable           
#define glXCreateNewContext mglXCreateNewContext        
#define glXMakeContextCurrent mglXMakeContextCurrent      
#define glXGetCurrentReadDrawable mglXGetCurrentReadDrawable  
#define glXQueryContext mglXQueryContext            
#define glXSelectEvent mglXSelectEvent             
#define glXGetSelectedEvent mglXGetSelectedEvent    

/* GLX 1.4 */
#define glXGetProcAddress mglXGetProcAddress


#endif
