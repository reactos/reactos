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
* Description:  OpenGL context handling.
*
****************************************************************************/

#ifndef __DGLCONTEXT_H
#define __DGLCONTEXT_H

// Disable compiler complaints about DLL linkage
#pragma warning (disable:4273)

// Macros to control compilation
#ifndef STRICT
#define STRICT
#endif // STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL\gl.h>

#ifdef _USE_GLD3_WGL
	#include "dglmacros.h"
	#include "dglglobals.h"
	#include "pixpack.h"
	#include "ddlog.h"
	#include "dglpf.h"
	#include "context.h"	// Mesa context
#else
	#include <ddraw.h>
	#include <d3d.h>

	#include "dglmacros.h"
	#include "dglglobals.h"
	#include "pixpack.h"
	#include "ddlog.h"
	#include "dglpf.h"
	#include "d3dvertex.h"

	#include "DirectGL.h"

	#include "context.h"	// Mesa context
	#include "vb.h"			// Mesa vertex buffer
#endif // _USE_GLD3_WGL

/*---------------------- Macros and type definitions ----------------------*/

// TODO: Use a list instead of this constant!
#define DGL_MAX_CONTEXTS 32

// Structure for describing an OpenGL context
#ifdef _USE_GLD3_WGL
typedef struct {
	BOOL				bHasBeenCurrent;
	DGL_pixelFormat		*lpPF;

	// Pointer to private driver data (this also contains the drawable).
	void				*glPriv;

	// Mesa vars:
	GLcontext			*glCtx;			// The core Mesa context
	GLvisual			*glVis;			// Describes the color buffer
	GLframebuffer		*glBuffer;		// Ancillary buffers

	GLuint				ClearIndex;
	GLuint				CurrentIndex;
	GLubyte				ClearColor[4];
	GLubyte				CurrentColor[4];

	BOOL				EmulateSingle;	// Emulate single-buffering
	BOOL				bDoubleBuffer;
	BOOL				bDepthBuffer;

	// Shared driver vars:
	BOOL				bAllocated;
    BOOL				bFullscreen;	// Is this a fullscreen context?
    BOOL				bSceneStarted;	// Has a lpDev->BeginScene been issued?
    BOOL				bCanRender;		// Flag: states whether rendering is OK
	BOOL				bFrameStarted;	// Has frame update started at all?
	BOOL				bStencil;		// TRUE if this context has stencil
	BOOL				bGDIEraseBkgnd; // GDI Erase Background command

	// Window information
	HWND				hWnd;			// Window handle
	HDC					hDC;			// Windows' Device Context of the window
	DWORD				dwWidth;		// Window width
	DWORD				dwHeight;		// Window height
	DWORD				dwBPP;			// Window bits-per-pixel;
	RECT				rcScreenRect;	// Screen rectangle
	DWORD				dwModeWidth;	// Display mode width
	DWORD				dwModeHeight;	// Display mode height
	float				dvClipX;
	float				dvClipY;
	LONG				lpfnWndProc;	// window message handler function

} DGL_ctx;

#define GLD_context			DGL_ctx
#define GLD_GET_CONTEXT(c)	(GLD_context*)(c)->DriverCtx

#else // _USE_GLD3_WGL

typedef struct {
	BOOL				bHasBeenCurrent;
	DGL_pixelFormat		*lpPF;
	//
	// Mesa context vars:
	//
	GLcontext			*glCtx;			// The core Mesa context
	GLvisual			*glVis;			// Describes the color buffer
	GLframebuffer		*glBuffer;		// Ancillary buffers

	GLuint				ClearIndex;
	GLuint				CurrentIndex;
	GLubyte				ClearColor[4];
	GLubyte				CurrentColor[4];

	BOOL				EmulateSingle;	// Emulate single-buffering
	BOOL				bDoubleBuffer;
	BOOL				bDepthBuffer;
	int					iZBufferPF;		// Index of Zbuffer pixel format

	// Vertex buffer: one-to-one correlation with Mesa's vertex buffer.
	// This will be filled by our setup function (see d3dvsetup.c)
	DGL_TLvertex		gWin[VB_SIZE];	// Transformed and lit vertices
//	DGL_Lvertex			gObj[VB_SIZE];	// Lit vertices in object coordinates.

	// Indices for DrawIndexedPrimitive.
	// Clipped quads are drawn seperately, so use VB_SIZE.
	// 6 indices are needed to make 2 triangles for each possible quad
//	WORD				wIndices[(VB_SIZE / 4) * 6];
	WORD				wIndices[32768];

	//
	// Device driver vars:
	//
	BOOL				bAllocated;
    BOOL				bFullscreen;	// Is this a fullscreen context?
    BOOL				bSceneStarted;	// Has a lpDev->BeginScene been issued?
    BOOL				bCanRender;		// Flag: states whether rendering is OK
	BOOL				bFrameStarted;	// Has frame update started at all?

    // DirectX COM interfaces, postfixed with the interface number
	IDirectDraw				*lpDD1;
	IDirectDraw4			*lpDD4;
	IDirect3D3				*lpD3D3;
	IDirect3DDevice3		*lpDev3;
	IDirect3DViewport3		*lpViewport3;
	IDirectDrawSurface4		*lpFront4;
	IDirectDrawSurface4		*lpBack4;
	IDirectDrawSurface4		*lpDepth4;

	// Vertex buffers
	BOOL					bD3DPipeline; // True if using D3D geometry pipeline
	IDirect3DVertexBuffer	*m_vbuf;	// Unprocessed vertices
	IDirect3DVertexBuffer	*m_pvbuf;	// Processed vertices ready to be rendered

	D3DTEXTUREOP		ColorOp[MAX_TEXTURE_UNITS]; // Used for re-enabling texturing
	D3DTEXTUREOP		AlphaOp[MAX_TEXTURE_UNITS]; // Used for re-enabling texturing
	struct gl_texture_object *tObj[MAX_TEXTURE_UNITS];

	DDCAPS				ddCaps;			// DirectDraw caps
	D3DDEVICEDESC		D3DDevDesc;		// Direct3D Device description

	DDPIXELFORMAT		ddpfRender;		// Pixel format of the render buffer
	DDPIXELFORMAT		ddpfDepth;		// Pixel format of the depth buffer

	BOOL				bStencil;		// TRUE is this context has stencil

	PX_packFunc			fnPackFunc;		// Pixel packing function for SW
	PX_unpackFunc		fnUnpackFunc;	// Pixel unpacking function for SW
	PX_packSpanFunc		fnPackSpanFunc;	// Pixel span packer

	D3DVIEWPORT2		d3dViewport;	// D3D Viewport object

	D3DCULL				cullmode;		// Direct3D cull mode
	D3DCOLOR			curcolor;		// Current color
	DWORD				dwColorPF;		// Current color, in format of target surface
	D3DCOLOR			d3dClearColor;	// Clear color
	D3DCOLOR			ConstantColor;	// For flat shading
	DWORD				dwClearColorPF;	// Clear color, in format of target surface
	BOOL				bGDIEraseBkgnd; // GDI Erase Background command

	// Primitive caches
//	DGL_vertex			LineCache[DGL_MAX_LINE_VERTS];
//	DGL_vertex			TriCache[DGL_MAX_TRI_VERTS];
//	DWORD				dwNextLineVert;
//	DWORD				dwNextTriVert;

	// Window information
	HWND				hWnd;			// Window handle
	HDC					hDC;			// Windows' Device Context of the window
	DWORD				dwWidth;		// Window width
	DWORD				dwHeight;		// Window height
	DWORD				dwBPP;			// Window bits-per-pixel;
	RECT				rcScreenRect;	// Screen rectangle
	DWORD				dwModeWidth;	// Display mode width
	DWORD				dwModeHeight;	// Display mode height
	float				dvClipX;
	float				dvClipY;
	LONG				lpfnWndProc;	// window message handler function

	// Shared texture palette
	IDirectDrawPalette	*lpGlobalPalette;

	// Usage counters.
	// One of these counters will be incremented when we choose
	// between hardware and software rendering functions.
//	DWORD				dwHWUsageCount;	// Hardware usage count
//	DWORD				dwSWUsageCount;	// Software usage count

	// Texture state flags.
//	BOOL				m_texturing;		// TRUE is texturing
//	BOOL				m_mtex;				// TRUE if multitexture
//	BOOL				m_texHandleValid;	// TRUE if tex state valid

	// Renderstate caches to ensure no redundant state changes
	DWORD				dwRS[256];		// Renderstates
	DWORD				dwTSS[2][24];	// Texture-stage states
	LPDIRECT3DTEXTURE2	lpTex[2];		// Texture (1 per stage)

	DWORD				dwMaxTextureSize;	// Max texture size:
											// clamped to 1024.

} DGL_ctx;
#endif // _USE_GLD3_WGL

/*------------------------- Function Prototypes ---------------------------*/

#ifdef  __cplusplus
extern "C" {
#endif

HHOOK	hKeyHook;
LRESULT CALLBACK dglKeyProc(int code,WPARAM wParam,LPARAM lParam);

void		dglInitContextState();
void		dglDeleteContextState();
BOOL 		dglIsValidContext(HGLRC a);
DGL_ctx*	dglGetContextAddress(const HGLRC a);
HDC 		dglGetCurrentDC(void);
HGLRC 		dglGetCurrentContext(void);
HGLRC		dglCreateContext(HDC a, const DGL_pixelFormat *lpPF);
BOOL		dglMakeCurrent(HDC a, HGLRC b);
BOOL		dglDeleteContext(HGLRC a);
BOOL		dglSwapBuffers(HDC hDC);

#ifdef  __cplusplus
}
#endif

#endif
