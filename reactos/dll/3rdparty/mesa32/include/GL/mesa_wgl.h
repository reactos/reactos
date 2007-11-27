/*
 * Mesa 3-D graphics library
 * Version:  3.1
 *
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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


/* prototypes for the Mesa WGL functions */
/* relocated here so that I could make GLUT get them properly */

#ifndef _mesa_wgl_h_
#define _mesa_wgl_h_

#if defined(__MINGW32__)
#  define __W32API_USE_DLLIMPORT__
#endif

#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef WGLAPI
#define WGLAPI GLAPI
#endif

#if defined(__MINGW32__)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN 1
#  endif
#  include <windows.h>
#endif


#if defined(_WIN32) && !defined(_WINGDI_) && !defined(_GNU_H_WINDOWS32_DEFINES) && !defined(OPENSTEP)
#ifndef _GNU_H_WINDOWS32_FUNCTIONS
#	ifdef UNICODE
#		define wglUseFontBitmaps  wglUseFontBitmapsW
#		define wglUseFontOutlines  wglUseFontOutlinesW
#	else
#		define wglUseFontBitmaps  wglUseFontBitmapsA
#		define wglUseFontOutlines  wglUseFontOutlinesA
#	endif /* !UNICODE */
#endif /* _GNU_H_WINDOWS32_FUNCTIONS */
typedef struct tagLAYERPLANEDESCRIPTOR LAYERPLANEDESCRIPTOR, *PLAYERPLANEDESCRIPTOR, *LPLAYERPLANEDESCRIPTOR;
typedef struct _GLYPHMETRICSFLOAT GLYPHMETRICSFLOAT, *PGLYPHMETRICSFLOAT, *LPGLYPHMETRICSFLOAT;
typedef struct tagPIXELFORMATDESCRIPTOR PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, *LPPIXELFORMATDESCRIPTOR;
#endif


#ifdef _MSC_VER
#  pragma warning( disable : 4615 ) /* pragma warning : unknown user warning type*/
#  pragma warning( push )
#  pragma warning( disable : 4273 ) /* 'function' : inconsistent DLL linkage. dllexport assumed. */
#endif


WGLAPI int   GLAPIENTRY wglSetPixelFormat(HDC, int, const PIXELFORMATDESCRIPTOR *);
WGLAPI int   GLAPIENTRY wglSwapBuffers(HDC hdc);
WGLAPI int   GLAPIENTRY wglChoosePixelFormat(HDC, const PIXELFORMATDESCRIPTOR *);
WGLAPI int   GLAPIENTRY wglDescribePixelFormat(HDC,int, unsigned int, LPPIXELFORMATDESCRIPTOR);
WGLAPI int   GLAPIENTRY wglGetPixelFormat(HDC hdc);

WGLAPI int   GLAPIENTRY wglCopyContext(HGLRC, HGLRC, unsigned int);
WGLAPI HGLRC GLAPIENTRY wglCreateContext(HDC);
WGLAPI HGLRC GLAPIENTRY wglCreateLayerContext(HDC,int);
WGLAPI int   GLAPIENTRY wglDeleteContext(HGLRC);
WGLAPI int   GLAPIENTRY wglDescribeLayerPlane(HDC, int, int, unsigned int,LPLAYERPLANEDESCRIPTOR);
WGLAPI HGLRC GLAPIENTRY wglGetCurrentContext(void);
WGLAPI HDC   GLAPIENTRY wglGetCurrentDC(void);
WGLAPI int   GLAPIENTRY wglGetLayerPaletteEntries(HDC, int, int, int,COLORREF *);
WGLAPI PROC  GLAPIENTRY wglGetProcAddress(const char*);
WGLAPI int   GLAPIENTRY wglMakeCurrent(HDC,HGLRC);
WGLAPI int   GLAPIENTRY wglRealizeLayerPalette(HDC, int, int);
WGLAPI int   GLAPIENTRY wglSetLayerPaletteEntries(HDC, int, int, int,const COLORREF *);
WGLAPI int   GLAPIENTRY wglShareLists(HGLRC, HGLRC);
WGLAPI int   GLAPIENTRY wglSwapLayerBuffers(HDC, unsigned int);
WGLAPI int   GLAPIENTRY wglUseFontBitmapsA(HDC, unsigned long, unsigned long, unsigned long);
WGLAPI int   GLAPIENTRY wglUseFontBitmapsW(HDC, unsigned long, unsigned long, unsigned long);
WGLAPI int   GLAPIENTRY wglUseFontOutlinesA(HDC, unsigned long, unsigned long, unsigned long, float,float, int, LPGLYPHMETRICSFLOAT);
WGLAPI int   GLAPIENTRY wglUseFontOutlinesW(HDC, unsigned long, unsigned long, unsigned long, float,float, int, LPGLYPHMETRICSFLOAT);

#ifndef __MINGW32__

typedef void *HPBUFFERARB;

WGLAPI int   GLAPIENTRY SwapBuffers(HDC);
WGLAPI int   GLAPIENTRY ChoosePixelFormat(HDC,const PIXELFORMATDESCRIPTOR *);
WGLAPI int   GLAPIENTRY DescribePixelFormat(HDC,int,unsigned int,LPPIXELFORMATDESCRIPTOR);
WGLAPI int   GLAPIENTRY GetPixelFormat(HDC);
WGLAPI int   GLAPIENTRY SetPixelFormat(HDC,int,const PIXELFORMATDESCRIPTOR *);

GLAPI const char * GLAPIENTRY wglGetExtensionsStringEXT (void);
GLAPI BOOL GLAPIENTRY wglChoosePixelFormatARB (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
GLAPI BOOL GLAPIENTRY wglSwapIntervalEXT (int interval);
GLAPI int GLAPIENTRY wglGetSwapIntervalEXT (void);
GLAPI BOOL GLAPIENTRY wglGetPixelFormatAttribivARB (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, int *piValues);
GLAPI BOOL GLAPIENTRY wglGetPixelFormatAttribfvARB (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, FLOAT *pfValues);
GLAPI BOOL GLAPIENTRY wglMakeContextCurrentARB(HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
GLAPI HANDLE GLAPIENTRY wglGetCurrentReadDCARB(void);
GLAPI HPBUFFERARB GLAPIENTRY wglCreatePbufferARB (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
GLAPI HDC GLAPIENTRY wglGetPbufferDCARB (HPBUFFERARB hPbuffer);
GLAPI int GLAPIENTRY wglReleasePbufferDCARB (HPBUFFERARB hPbuffer, HDC hDC);
GLAPI BOOL GLAPIENTRY wglDestroyPbufferARB (HPBUFFERARB hPbuffer);
GLAPI BOOL GLAPIENTRY wglQueryPbufferARB (HPBUFFERARB hPbuffer, int iAttribute, int *piValue);
GLAPI HANDLE GLAPIENTRY wglCreateBufferRegionARB(HDC hDC, int iLayerPlane, UINT uType);
GLAPI VOID GLAPIENTRY wglDeleteBufferRegionARB(HANDLE hRegion);
GLAPI BOOL GLAPIENTRY wglSaveBufferRegionARB(HANDLE hRegion, int x, int y, int width, int height);
GLAPI BOOL GLAPIENTRY wglRestoreBufferRegionARB(HANDLE hRegion, int x, int y, int width, int height, int xSrc, int ySrc);
GLAPI BOOL GLAPIENTRY wglSetPbufferAttribARB (HPBUFFERARB hPbuffer, const int *piAttribList);
GLAPI BOOL GLAPIENTRY wglBindTexImageARB (HPBUFFERARB hPbuffer, int iBuffer);
GLAPI BOOL GLAPIENTRY wglReleaseTexImageARB (HPBUFFERARB hPbuffer, int iBuffer);
#endif

#ifndef WGL_ARB_extensions_string
#define WGL_ARB_extensions_string 1

WGLAPI const char * GLAPIENTRY wglGetExtensionsStringARB(HDC hdc);
#endif /* WGL_ARB_extensions_string */


#ifdef _MSC_VER
#  pragma warning( pop )
#endif

#ifdef __cplusplus
}
#endif


#endif /* _mesa_wgl_h_ */
