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

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "gld_driver.h"
#include "ddlog.h"
#include "glheader.h"

// For glGetString().
#include "common_x86_asm.h"

//---------------------------------------------------------------------------

static char *szDriverError = "Driver used before initialisation!";

// This holds our dynamically created OpenGL renderer string.
// 256 chars should be plenty - remember that some apps display this.
static char _gldRendererString[256];

static char *szVendor = "SciTech Software, Inc.";

//---------------------------------------------------------------------------

extern BOOL gldGetDXErrorString_DX(HRESULT hr, char *buf, int nBufSize);

extern BOOL gldCreateDrawable_MesaSW(DGL_ctx *ctx, BOOL bPersistantInterface, BOOL bPersistantBuffers);
extern BOOL gldResizeDrawable_MesaSW(DGL_ctx *ctx, BOOL bDefaultDriver, BOOL bPersistantInterface, BOOL bPersistantBuffers);
extern BOOL gldDestroyDrawable_MesaSW(DGL_ctx *ctx);
extern BOOL gldCreatePrivateGlobals_MesaSW(void);
extern BOOL gldDestroyPrivateGlobals_MesaSW(void);
extern BOOL	gldBuildPixelformatList_MesaSW(void);
extern BOOL gldInitialiseMesa_MesaSW(DGL_ctx *ctx);
extern BOOL	gldSwapBuffers_MesaSW(DGL_ctx *ctx, HDC hDC, HWND hWnd);
extern PROC	gldGetProcAddress_MesaSW(LPCSTR a);
extern BOOL	gldGetDisplayMode_MesaSW(DGL_ctx *ctx, GLD_displayMode *glddm);

extern BOOL gldCreateDrawable_DX(DGL_ctx *ctx, BOOL bPersistantInterface, BOOL bPersistantBuffers);
extern BOOL gldResizeDrawable_DX(DGL_ctx *ctx, BOOL bDefaultDriver, BOOL bPersistantInterface, BOOL bPersistantBuffers);
extern BOOL gldDestroyDrawable_DX(DGL_ctx *ctx);
extern BOOL gldCreatePrivateGlobals_DX(void);
extern BOOL gldDestroyPrivateGlobals_DX(void);
extern BOOL	gldBuildPixelformatList_DX(void);
extern BOOL gldInitialiseMesa_DX(DGL_ctx *ctx);
extern BOOL	gldSwapBuffers_DX(DGL_ctx *ctx, HDC hDC, HWND hWnd);
extern PROC	gldGetProcAddress_DX(LPCSTR a);
extern BOOL	gldGetDisplayMode_DX(DGL_ctx *ctx, GLD_displayMode *glddm);

//---------------------------------------------------------------------------
// NOP functions. Called if proper driver functions are not set.
//---------------------------------------------------------------------------

static BOOL _gldDriverError(void)
{
	ddlogMessage(DDLOG_CRITICAL, szDriverError);
	return FALSE;
}

//---------------------------------------------------------------------------

static BOOL _GetDXErrorString_ERROR(
	HRESULT hr,
	char *buf,
	int nBufSize)
{
	return _gldDriverError();
}

//---------------------------------------------------------------------------

static BOOL _CreateDrawable_ERROR(
	DGL_ctx *ctx,
	BOOL bPersistantInterface,
	BOOL bPersistantBuffers)
{
	return _gldDriverError();
}

//---------------------------------------------------------------------------

static BOOL _ResizeDrawable_ERROR(
	DGL_ctx *ctx,
	BOOL bDefaultDriver,
	BOOL bPersistantInterface,
	BOOL bPersistantBuffers)
{
	return _gldDriverError();
}

//---------------------------------------------------------------------------

static BOOL _DestroyDrawable_ERROR(
	DGL_ctx *ctx)
{
	return _gldDriverError();
}

//---------------------------------------------------------------------------

static BOOL _CreatePrivateGlobals_ERROR(void)
{
	return _gldDriverError();
}

//---------------------------------------------------------------------------

static BOOL _DestroyPrivateGlobals_ERROR(void)
{
	return _gldDriverError();
}

//---------------------------------------------------------------------------

static BOOL _BuildPixelformatList_ERROR(void)
{
	return _gldDriverError();
}

//---------------------------------------------------------------------------


static BOOL _InitialiseMesa_ERROR(
	DGL_ctx *ctx)
{
	return _gldDriverError();
}

//---------------------------------------------------------------------------

static BOOL	_SwapBuffers_ERROR(
	DGL_ctx *ctx,
	HDC hDC,
	HWND hWnd)
{
	return _gldDriverError();
}

//---------------------------------------------------------------------------

static PROC _GetProcAddress_ERROR(
	LPCSTR a)
{
	_gldDriverError();
	return NULL;
}

//---------------------------------------------------------------------------

static BOOL	_GetDisplayMode_ERROR(
	DGL_ctx *ctx,
	GLD_displayMode *glddm)
{
	return _gldDriverError();
}

//---------------------------------------------------------------------------
// Functions useful to all drivers
//---------------------------------------------------------------------------

const GLubyte* _gldGetStringGeneric(
	GLcontext *ctx,
	GLenum name)
{
	if (!ctx)
		return NULL;

	switch (name) {
	case GL_RENDERER:
		sprintf(_gldRendererString, "GLDirect 4.0 %s%s%s%s (%s %s)",
			_mesa_x86_cpu_features	? "/x86"		: "",
			cpu_has_mmx				? "/MMX"		: "",
			cpu_has_3dnow			? "/3DNow!"		: "",
			cpu_has_xmm				? "/SSE"		: "",
			__DATE__, __TIME__);
		return (const GLubyte *) _gldRendererString;
	case GL_VENDOR:
		return (const GLubyte *) szVendor;
	default:
		return NULL;
	}
}

//---------------------------------------------------------------------------
// Global driver function pointers, initially set to functions that
// will report an error when called.
//---------------------------------------------------------------------------

GLD_driver _gldDriver = {
	_GetDXErrorString_ERROR,
	_CreateDrawable_ERROR,
	_ResizeDrawable_ERROR,
	_DestroyDrawable_ERROR,
	_CreatePrivateGlobals_ERROR,
	_DestroyPrivateGlobals_ERROR,
	_BuildPixelformatList_ERROR,
	_InitialiseMesa_ERROR,
	_SwapBuffers_ERROR,
	_GetProcAddress_ERROR,
	_GetDisplayMode_ERROR
};

//---------------------------------------------------------------------------
// Init function. Should be called as soon as regkeys/ini-settings are read.
//---------------------------------------------------------------------------

BOOL gldInitDriverPointers(
	DWORD dwDriver)
{
	_gldDriver.GetDXErrorString	= gldGetDXErrorString_DX;

	if (dwDriver == GLDS_DRIVER_MESA_SW) {
		// Mesa Software driver
		_gldDriver.CreateDrawable			= gldCreateDrawable_MesaSW;
		_gldDriver.ResizeDrawable			= gldResizeDrawable_MesaSW;
		_gldDriver.DestroyDrawable			= gldDestroyDrawable_MesaSW;
		_gldDriver.CreatePrivateGlobals		= gldCreatePrivateGlobals_MesaSW;
		_gldDriver.DestroyPrivateGlobals	= gldDestroyPrivateGlobals_MesaSW;
		_gldDriver.BuildPixelformatList		= gldBuildPixelformatList_MesaSW;
		_gldDriver.InitialiseMesa			= gldInitialiseMesa_MesaSW;
		_gldDriver.SwapBuffers				= gldSwapBuffers_MesaSW;
		_gldDriver.wglGetProcAddress		= gldGetProcAddress_MesaSW;
		_gldDriver.GetDisplayMode			= gldGetDisplayMode_MesaSW;
		return TRUE;
	}
	
	if ((dwDriver == GLDS_DRIVER_REF) || (dwDriver == GLDS_DRIVER_HAL)) {
		// Direct3D driver, either HW or SW
		_gldDriver.CreateDrawable			= gldCreateDrawable_DX;
		_gldDriver.ResizeDrawable			= gldResizeDrawable_DX;
		_gldDriver.DestroyDrawable			= gldDestroyDrawable_DX;
		_gldDriver.CreatePrivateGlobals		= gldCreatePrivateGlobals_DX;
		_gldDriver.DestroyPrivateGlobals	= gldDestroyPrivateGlobals_DX;
		_gldDriver.BuildPixelformatList		= gldBuildPixelformatList_DX;
		_gldDriver.InitialiseMesa			= gldInitialiseMesa_DX;
		_gldDriver.SwapBuffers				= gldSwapBuffers_DX;
		_gldDriver.wglGetProcAddress		= gldGetProcAddress_DX;
		_gldDriver.GetDisplayMode			= gldGetDisplayMode_DX;
		return TRUE;
	};

	return FALSE;
}

//---------------------------------------------------------------------------
