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
* Description:  Global variables.
*
****************************************************************************/

#include "dglglobals.h"

// =======================================================================
// Global Variables
// =======================================================================

char szCopyright[]	= "Copyright (c) 1998 SciTech Software, Inc.";
char szDllName[]	= "Scitech GLDirect";
char szErrorTitle[]	= "GLDirect Error";

DGL_globals glb;

// Shared result variable
HRESULT hResult;

// ***********************************************************************

// Patch function for missing function in Mesa
int finite(
	double x)
{
	return _finite(x);
};

// ***********************************************************************

void dglInitGlobals()
{
    // Zero all fields just in case
    memset(&glb, 0, sizeof(glb));

	// Set the global defaults
	glb.bPrimary			= FALSE;		// Not the primary device
	glb.bHardware			= FALSE;		// Not a hardware device
//	glb.bFullscreen			= FALSE;		// Not running fullscreen
	glb.bSquareTextures		= FALSE;		// Device does not need sq
	glb.bPAL8				= FALSE;		// Device cannot do 8bit
	glb.dwMemoryType		= DDSCAPS_SYSTEMMEMORY;
	glb.dwRendering			= DGL_RENDER_D3D;

	glb.bWaitForRetrace		= TRUE;			// Sync to vertical retrace
	glb.bFullscreenBlit		= FALSE;

	glb.nPixelFormatCount	= 0;
	glb.lpPF				= NULL;			// Pixel format list
#ifndef _USE_GLD3_WGL
	glb.nZBufferPFCount		= 0;
	glb.lpZBufferPF			= NULL;
	glb.nDisplayModeCount	= 0;
	glb.lpDisplayModes		= NULL;
	glb.nTextureFormatCount	= 0;
	glb.lpTextureFormat		= NULL;
#endif // _USE_GLD3_WGL

	glb.wMaxSimultaneousTextures = 1;

	// Enable support for multitexture, if available.
	glb.bMultitexture		= TRUE;

	// Enable support for mipmapping
	glb.bUseMipmaps			= TRUE;

	// Alpha emulation via chroma key
	glb.bEmulateAlphaTest	= FALSE;

	// Use Mesa pipeline always (for compatibility)
	glb.bForceMesaPipeline	= FALSE;

	// Init support for multiple GLRCs
	glb.bDirectDraw			= FALSE;
	glb.bDirectDrawPrimary	= FALSE;
	glb.bDirect3D			= FALSE;
	glb.bDirect3DDevice		= FALSE;
	glb.bDirectDrawStereo	= FALSE;
	glb.iDirectDrawStereo	= 0;
	glb.hWndActive			= NULL;
	// Init DirectX COM interfaces for multiple GLRCs
//	glb.lpDD4				= NULL;
//	glb.lpPrimary4			= NULL;
//	glb.lpBack4				= NULL;
//	glb.lpDepth4			= NULL;
//	glb.lpGlobalPalette		= NULL;

	// Init special support options
	glb.bMessageBoxWarnings = TRUE;
	glb.bDirectDrawPersistant = FALSE;
	glb.bPersistantBuffers	= FALSE;

	// Do not assume single-precision-only FPU (for compatibility)
	glb.bFastFPU			= FALSE;

	// Allow hot-key support
	glb.bHotKeySupport		= TRUE;

	// Default to single-threaded support (for simplicity)
	glb.bMultiThreaded		= FALSE;

	// Use application-specific customizations (for end-user convenience)
	glb.bAppCustomizations	= TRUE;

#ifdef _USE_GLD3_WGL
	// Registry/ini-file settings for GLDirect 3.x
	glb.dwAdapter				= 0;	// Primary DX8 adapter
	glb.dwTnL					= 1;	// MesaSW TnL
	glb.dwMultisample			= 0;	// Multisample Off
	glb.dwDriver				= 2;	// Direct3D HW

	// Signal a pixelformat list rebuild
	glb.bPixelformatsDirty		= TRUE;
#endif
}

// ***********************************************************************
