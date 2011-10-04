/****************************************************************************
*
*                        Mesa 3-D graphics library
*                        Direct3D Driver Interface
*
*  ========================================================================
*
*   Copyright (C) 1991-2004 SciTech Software, Inc. All rights reserved.
*
*   Permission is hereby granted, free of charge, to any person obtaining a
*   copy of this software and associated documentation files (the "Software"),
*   to deal in the Software without restriction, including without limitation
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,
*   and/or sell copies of the Software, and to permit persons to whom the
*   Software is furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included
*   in all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
*   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
*   SCITECH SOFTWARE INC BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
*   OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
*  ======================================================================
*
* Language:     ANSI C
* Environment:  Windows 9x (Win32)
*
* Description:  OpenGL window  functions (wgl*).
*
****************************************************************************/

#ifndef __DGLWGL_H
#define __DGLWGL_H

// Disable compiler complaints about DLL linkage
#pragma warning (disable:4273)

// Macros to control compilation
#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <GL\gl.h>

#include "dglcontext.h"
#include "dglglobals.h"
#include "dglmacros.h"
#include "ddlog.h"
#include "dglpf.h"

/*---------------------- Macros and type definitions ----------------------*/

typedef struct {
	PROC proc;
	char *name;
}  DGL_extension;

#ifndef __MINGW32__
/* XXX why is this here?
 * It should probaby be somewhere in src/mesa/drivers/windows/
 */
#if defined(_WIN32) && !defined(_WINGDI_) && !defined(_WINGDI_H) && !defined(_GNU_H_WINDOWS32_DEFINES) && !defined(OPENSTEP) && !defined(BUILD_FOR_SNAP) 
#	define WGL_FONT_LINES      0
#	define WGL_FONT_POLYGONS   1
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
#if !defined(GLX_USE_MESA)
#include <GL/mesa_wgl.h>
#endif
#endif
#endif /* !__MINGW32__ */

/*------------------------- Function Prototypes ---------------------------*/

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef _USE_GLD3_WGL
int		APIENTRY DGL_ChoosePixelFormat(HDC a, CONST PIXELFORMATDESCRIPTOR *ppfd);
BOOL	APIENTRY DGL_CopyContext(HGLRC a, HGLRC b, UINT c);
HGLRC	APIENTRY DGL_CreateContext(HDC a);
HGLRC	APIENTRY DGL_CreateLayerContext(HDC a, int b);
BOOL	APIENTRY DGL_DeleteContext(HGLRC a);
BOOL	APIENTRY DGL_DescribeLayerPlane(HDC a, int b, int c, UINT d, LPLAYERPLANEDESCRIPTOR e);
int		APIENTRY DGL_DescribePixelFormat(HDC a, int b, UINT c, LPPIXELFORMATDESCRIPTOR d);
HGLRC	APIENTRY DGL_GetCurrentContext(void);
HDC		APIENTRY DGL_GetCurrentDC(void);
PROC	APIENTRY DGL_GetDefaultProcAddress(LPCSTR a);
int		APIENTRY DGL_GetLayerPaletteEntries(HDC a, int b, int c, int d, COLORREF *e);
int		APIENTRY DGL_GetPixelFormat(HDC a);
PROC	APIENTRY DGL_GetProcAddress(LPCSTR a);
BOOL	APIENTRY DGL_MakeCurrent(HDC a, HGLRC b);
BOOL	APIENTRY DGL_RealizeLayerPalette(HDC a, int b, BOOL c);
int		APIENTRY DGL_SetLayerPaletteEntries(HDC a, int b, int c, int d, CONST COLORREF *e);
BOOL	APIENTRY DGL_SetPixelFormat(HDC a, int b, CONST PIXELFORMATDESCRIPTOR *c);
BOOL	APIENTRY DGL_ShareLists(HGLRC a, HGLRC b);
BOOL	APIENTRY DGL_SwapBuffers(HDC a);
BOOL	APIENTRY DGL_SwapLayerBuffers(HDC a, UINT b);
BOOL	APIENTRY DGL_UseFontBitmapsA(HDC a, DWORD b, DWORD c, DWORD d);
BOOL	APIENTRY DGL_UseFontBitmapsW(HDC a, DWORD b, DWORD c, DWORD d);
BOOL	APIENTRY DGL_UseFontOutlinesA(HDC a, DWORD b, DWORD c, DWORD d, FLOAT e, FLOAT f, int g, LPGLYPHMETRICSFLOAT h);
BOOL	APIENTRY DGL_UseFontOutlinesW(HDC a, DWORD b, DWORD c, DWORD d, FLOAT e, FLOAT f, int g, LPGLYPHMETRICSFLOAT h);
#endif //_USE_GLD3_WGL

BOOL	dglWglResizeBuffers(GLcontext *ctx, BOOL bDefaultDriver);

#ifdef  __cplusplus
}
#endif

#endif
