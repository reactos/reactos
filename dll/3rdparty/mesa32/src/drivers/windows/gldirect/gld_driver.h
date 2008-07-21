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
* Environment:  Windows 9x/2000/XP/XBox (Win32)
*
* Description:  Driver functions and interfaces
*
****************************************************************************/

#ifndef _GLD_DRIVER_H
#define _GLD_DRIVER_H

// This file is only useful is we're using the new GLD3 WGL code.
#ifdef _USE_GLD3_WGL

#include "dglcontext.h"

// Same as DX8 D3DDISPLAYMODE
typedef struct {
	DWORD	Width;
	DWORD	Height;
	DWORD	Refresh;
	DWORD	BPP;
} GLD_displayMode;

typedef struct {
	// Returns a string for a given HRESULT error code.
	BOOL	(*GetDXErrorString)(HRESULT hr, char *buf, int nBufSize);

	// Driver functions for managing drawables.
	// Functions must respect persistant buffers / persistant interface.
	// NOTE: Persistant interface is: DirectDraw, pre-DX8; Direct3D, DX8 and above.
	BOOL	(*CreateDrawable)(DGL_ctx *ctx, BOOL bPersistantInterface, BOOL bPersistantBuffers);
	BOOL	(*ResizeDrawable)(DGL_ctx *ctx, BOOL bDefaultDriver, BOOL bPersistantInterface, BOOL bPersistantBuffers);
	BOOL	(*DestroyDrawable)(DGL_ctx *ctx);

	// Create/Destroy private globals belonging to driver
	BOOL	(*CreatePrivateGlobals)(void);
	BOOL	(*DestroyPrivateGlobals)(void);

	// Build pixelformat list
	BOOL	(*BuildPixelformatList)(void);

	// Initialise Mesa's driver pointers
	BOOL	(*InitialiseMesa)(DGL_ctx *ctx);

	// Swap buffers
	BOOL	(*SwapBuffers)(DGL_ctx *ctx, HDC hDC, HWND hWnd);

	// wglGetProcAddress()
	PROC	(*wglGetProcAddress)(LPCSTR a);

	BOOL	(*GetDisplayMode)(DGL_ctx *ctx, GLD_displayMode *glddm);
} GLD_driver;

extern GLD_driver _gldDriver;

BOOL gldInitDriverPointers(DWORD dwDriver);
const GLubyte* _gldGetStringGeneric(GLcontext *ctx, GLenum name);

#endif // _USE_GLD3_WGL

#endif // _GLD_DRIVER_H
