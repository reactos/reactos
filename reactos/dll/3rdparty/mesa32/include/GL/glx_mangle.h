/*
 * Mesa 3-D graphics library
 * Version:  4.1
 * Copyright (C) 1995-1998  Brian Paul
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

/* GLX 1.4 */
#define glXGetProcAddress mglXGetProcAddress


#endif
