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
* Description:  Globals.
*
****************************************************************************/

#ifndef __DGLGLOBALS_H
#define __DGLGLOBALS_H

#include "dglcontext.h"
#include "dglpf.h"		// Pixel format
#ifndef _USE_GLD3_WGL
#include "d3dtexture.h"
#endif

/*---------------------- Macros and type definitions ----------------------*/

typedef enum {
	DGL_RENDER_MESASW		= 0,
	DGL_RENDER_D3D			= 1,
	DGL_RENDER_FORCE_DWORD	= 0x7ffffff,
} DGL_renderType;

#ifdef _USE_GLD3_WGL

// Same as DGL_renderType? KeithH
typedef enum {
	GLDS_DRIVER_MESA_SW			= 0,	// Mesa SW rendering
	GLDS_DRIVER_REF				= 1,	// Direct3D Reference Rasteriser
	GLDS_DRIVER_HAL				= 2,	// Direct3D HW rendering
} GLDS_DRIVER;

typedef enum {
	GLDS_TNL_DEFAULT			= 0,	// Choose best TnL method
	GLDS_TNL_MESA				= 1,	// Mesa TnL
	GLDS_TNL_D3DSW				= 2,	// D3D Software TnL
	GLDS_TNL_D3DHW				= 3,	// D3D Hardware TnL
} GLDS_TNL;

typedef enum {
	GLDS_MULTISAMPLE_NONE		= 0,
	GLDS_MULTISAMPLE_FASTEST	= 1,
	GLDS_MULTISAMPLE_NICEST		= 2,
} GLDS_MULTISAMPLE;
#endif

typedef struct {
	// Registry settings
	char				szDDName[MAX_DDDEVICEID_STRING]; // DirectDraw device name
	char				szD3DName[MAX_DDDEVICEID_STRING]; // Direct3D driver name
	BOOL				bPrimary; // Is ddraw device the Primary device?
	BOOL				bHardware; // Is the d3d driver a Hardware driver?
#ifndef _USE_GLD3_WGL
	GUID				ddGuid; // GUID of the ddraw device 
	GUID				d3dGuid; // GUID of the direct3d driver
#endif // _USE_GLD3_WGL
//	BOOL				bFullscreen; // Force fullscreen - only useful for primary adaptors.
	BOOL				bSquareTextures; // Does this driver require square textures?
	DWORD               dwRendering; // Type of rendering required

	BOOL				bWaitForRetrace; // Sync to vertical retrace
	BOOL				bFullscreenBlit; // Use Blt instead of Flip in fullscreen modes

	// Multitexture
	BOOL				bMultitexture;

	BOOL				bUseMipmaps;

	DWORD				dwMemoryType; // Sysmem or Vidmem

	// Global palette
	BOOL				bPAL8;
	DDPIXELFORMAT		ddpfPAL8;

	// Multitexture
	WORD				wMaxSimultaneousTextures;

	// Win32 internals
	BOOL				bAppActive; // Keep track of Alt-Tab
	LONG				lpfnWndProc; // WndProc of calling app

	// Pixel Format Descriptior list.
	int					nPixelFormatCount;
	DGL_pixelFormat		*lpPF;
#ifndef _USE_GLD3_WGL
	// ZBuffer pixel formats
	int					nZBufferPFCount; // Count of Zbuffer pixel formats
	DDPIXELFORMAT		*lpZBufferPF; // ZBuffer pixel formats

	// Display modes (for secondary devices)
	int					nDisplayModeCount;
	DDSURFACEDESC2		*lpDisplayModes;

	// Texture pixel formats
	int					nTextureFormatCount;
	DGL_textureFormat	*lpTextureFormat;
#endif // _USE_GLD3_WGL
	// Alpha emulation via chroma key
	BOOL				bEmulateAlphaTest;

	// Geom pipeline override.
	// If this is set TRUE then the D3D pipeline will never be used,
	// and the Mesa pipline will always be used.
	BOOL				bForceMesaPipeline;

#ifdef _USE_GLD3_WGL
	BOOL				bPixelformatsDirty;	// Signal a list rebuild
#endif

	// Additional globals to support multiple GL rendering contexts, GLRCs
	BOOL				bDirectDraw;			// DirectDraw interface exists ?
	BOOL				bDirectDrawPrimary;		// DirectDraw primary surface exists ?
	BOOL				bDirect3D;				// Direct3D interface exists ?
	BOOL				bDirect3DDevice;		// Direct3D device exists ?

	BOOL 				bDirectDrawStereo;		// DirectDraw Stereo driver started ?
	int 				iDirectDrawStereo;		// DirectDraw Stereo driver reference count
	HWND				hWndActive;				// copy of active window

    // Copies of DirectX COM interfaces for re-referencing across multiple GLRCs
//	IDirectDraw4			*lpDD4;				// copy of DirectDraw interface
//	IDirectDrawSurface4		*lpPrimary4;		// copy of DirectDraw primary surface
//	IDirectDrawSurface4		*lpBack4;
//	IDirectDrawSurface4		*lpDepth4;
//	IDirectDrawPalette		*lpGlobalPalette;

	// Aids for heavy-duty MFC-windowed OGL apps, like AutoCAD
	BOOL				bMessageBoxWarnings;	// popup message box warning
	BOOL				bDirectDrawPersistant;  // DirectDraw is persisitant
	BOOL				bPersistantBuffers;  	// DirectDraw buffers persisitant

	// FPU setup option for CAD precision (AutoCAD) vs GAME speed (Quake)
	BOOL				bFastFPU;				// single-precision-only FPU ?

	// Hot-Key support, like for real-time stereo parallax adjustments
	BOOL				bHotKeySupport;			// hot-key support ?

	// Multi-threaded support, for apps like 3DStudio
	BOOL				bMultiThreaded;			// multi-threaded ?

	// Detect and use app-specific customizations for apps like 3DStudio
	BOOL				bAppCustomizations;		// app customizations ?

#ifdef _USE_GLD3_WGL
	DWORD				dwAdapter;				// Primary DX8 adapter
	DWORD				dwTnL;					// MesaSW TnL
	DWORD				dwMultisample;			// Multisample Off
	DWORD				dwDriver;				// Direct3D HW
	void				*pDrvPrivate;			// Driver-specific data
#endif

} DGL_globals;

/*------------------------- Function Prototypes ---------------------------*/

#ifdef  __cplusplus
extern "C" {
#endif

DGL_globals	glb;

void		dglInitGlobals();

#ifdef  __cplusplus
}
#endif

#endif
