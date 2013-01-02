/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef STW_WGL_H_
#define STW_WGL_H_


#include <windows.h>

#include <GL/gl.h>


/*
 * Undeclared APIs exported by opengl32.dll
 */

WINGDIAPI BOOL WINAPI
wglSwapBuffers(HDC hdc);

WINGDIAPI int WINAPI
wglChoosePixelFormat(HDC hdc,
                     CONST PIXELFORMATDESCRIPTOR *ppfd);

WINGDIAPI int WINAPI
wglDescribePixelFormat(HDC hdc,
                       int iPixelFormat,
                       UINT nBytes,
                       LPPIXELFORMATDESCRIPTOR ppfd);

WINGDIAPI int WINAPI
wglGetPixelFormat(HDC hdc);

WINGDIAPI BOOL WINAPI
wglSetPixelFormat(HDC hdc,
                  int iPixelFormat,
                  CONST PIXELFORMATDESCRIPTOR *ppfd);

#ifndef WGL_SWAPMULTIPLE_MAX

typedef struct _WGLSWAP
{
   HDC hdc;
   UINT uiFlags;
} WGLSWAP;

#define WGL_SWAPMULTIPLE_MAX 16

WINGDIAPI DWORD WINAPI
wglSwapMultipleBuffers(UINT n,
                       CONST WGLSWAP *ps);

#endif /* !WGL_SWAPMULTIPLE_MAX */


#endif /* STW_WGL_H_ */
