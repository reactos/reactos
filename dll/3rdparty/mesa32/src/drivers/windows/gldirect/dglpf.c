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
*  ========================================================================
*
* Language:     ANSI C
* Environment:  Windows 9x (Win32)
*
* Description:  Pixel Formats.
*
****************************************************************************/

#include "dglpf.h"

#ifdef _USE_GLD3_WGL
#include "gld_driver.h"
#endif

// ***********************************************************************

char	szColorDepthWarning[] =
"GLDirect does not support the current desktop\n\
color depth.\n\n\
You may need to change the display resolution to\n\
16 bits per pixel or higher color depth using\n\
the Windows Display Settings control panel\n\
before running this OpenGL application.\n";

// ***********************************************************************
// This pixel format will be used as a template when compiling the list
// of pixel formats supported by the hardware. Many fields will be
// filled in at runtime.
// PFD flag defaults are upgraded to match ChoosePixelFormat() -- DaveM
DGL_pixelFormat pfTemplateHW =
{
    {
	sizeof(PIXELFORMATDESCRIPTOR),	// Size of the data structure
		1,							// Structure version - should be 1
									// Flags:
		PFD_DRAW_TO_WINDOW |		// The buffer can draw to a window or device surface.
		PFD_DRAW_TO_BITMAP |		// The buffer can draw to a bitmap. (DaveM)
		PFD_SUPPORT_GDI |			// The buffer supports GDI drawing. (DaveM)
		PFD_SUPPORT_OPENGL |		// The buffer supports OpenGL drawing.
		PFD_DOUBLEBUFFER |			// The buffer is double-buffered.
		0,							// Placeholder for easy commenting of above flags
		PFD_TYPE_RGBA,				// Pixel type RGBA.
		16,							// Total colour bitplanes (excluding alpha bitplanes)
		5, 0,						// Red bits, shift
		5, 5,						// Green bits, shift
		5, 10,						// Blue bits, shift
		0, 0,						// Alpha bits, shift (destination alpha)
		0,							// Accumulator bits (total)
		0, 0, 0, 0,					// Accumulator bits: Red, Green, Blue, Alpha
		0,							// Depth bits
		0,							// Stencil bits
		0,							// Number of auxiliary buffers
		0,							// Layer type
		0,							// Specifies the number of overlay and underlay planes.
		0,							// Layer mask
		0,							// Specifies the transparent color or index of an underlay plane.
		0							// Damage mask
	},
	-1,	// No depth/stencil buffer
};

// ***********************************************************************
// Return the count of the number of bits in a Bit Mask.
int BitCount(
	DWORD dw)
{
	int i;

	if (dw == 0)
		return 0;	// account for non-RGB mode

	for (i=0; dw; dw=dw>>1)
        i += (dw & 1);
    return i;
}

// ***********************************************************************

DWORD BitShift(
	DWORD dwMaskIn)
{
	DWORD dwShift, dwMask;

	if (dwMaskIn == 0)
		return 0;	// account for non-RGB mode

	for (dwShift=0, dwMask=dwMaskIn; !(dwMask&1); dwShift++, dwMask>>=1);

    return dwShift;
}

// ***********************************************************************

BOOL IsValidPFD(int iPFD)
{
	DGL_pixelFormat *lpPF;

	// Validate license
	if (!dglValidate())
		return FALSE;

	if ((glb.lpPF == NULL) ||
		(glb.nPixelFormatCount == 0))
		return FALSE;

	// Check PFD range
	if ( (iPFD < 1) || (iPFD > glb.nPixelFormatCount) ) {
		ddlogMessage(DDLOG_ERROR, "PFD out of range\n");
		return FALSE; // PFD is invalid
	}

	// Make a pointer to the pixel format
	lpPF = &glb.lpPF[iPFD-1];

	// Check size
	if (lpPF->pfd.nSize != sizeof(PIXELFORMATDESCRIPTOR)) {
		ddlogMessage(DDLOG_ERROR, "Bad PFD size\n");
		return FALSE; // PFD is invalid
	}

	// Check version
	if (lpPF->pfd.nVersion != 1) {
		ddlogMessage(DDLOG_ERROR, "PFD is not Version 1\n");
		return FALSE; // PFD is invalid
	}

	return TRUE; // PFD is valid
}

// ***********************************************************************

#ifndef _USE_GLD3_WGL

int		iEnumCount;			// Enumeration count
DWORD	dwDisplayBitDepth;	// Bit depth of current display mode

// ***********************************************************************

HRESULT WINAPI EnumDisplayModesCallback(
	DDSURFACEDESC2* pddsd,
	void *pvContext)
{
	DWORD			dwModeDepth;
	DDSURFACEDESC2	*lpDisplayMode;
	char			buf[32];

    // Check parameters
	if (pddsd == NULL)
		return DDENUMRET_CANCEL;

    dwModeDepth = pddsd->ddpfPixelFormat.dwRGBBitCount;
	lpDisplayMode = (DDSURFACEDESC2 *)pvContext;

	// Check mode for compatability with device.
	if (dwModeDepth != dwDisplayBitDepth)
		return DDENUMRET_OK;

	if (lpDisplayMode != NULL) {
		memcpy(&lpDisplayMode[iEnumCount], pddsd, sizeof(DDSURFACEDESC2));
		sprintf(buf, TEXT("Mode: %ld x %ld x %ld\n"),
				pddsd->dwWidth, pddsd->dwHeight, dwModeDepth);
		ddlogMessage(DDLOG_INFO, buf);
	}

	iEnumCount++;

	return DDENUMRET_OK;
}

// ***********************************************************************

HRESULT CALLBACK d3dEnumZBufferFormatsCallback(
	DDPIXELFORMAT* pddpf,
	VOID* lpZBufferPF )
{
	char buf[64];

	if(pddpf == NULL)
		return D3DENUMRET_CANCEL;

	if (pddpf->dwFlags & DDPF_ZBUFFER) {
		if (lpZBufferPF == NULL) {
			// Pass 1. Merely counting the PF
			glb.nZBufferPFCount++;
		} else {
			// Pass 2. Save the PF
			if (pddpf->dwFlags & DDPF_STENCILBUFFER) {
				sprintf(buf, " %d: Z=%d S=%d\n",
					iEnumCount,
					pddpf->dwZBufferBitDepth,
					pddpf->dwStencilBitDepth);
			} else {
				sprintf(buf, " %d: Z=%d S=0\n",
					iEnumCount,
					pddpf->dwZBufferBitDepth);
			}
			ddlogMessage(DDLOG_INFO, buf);

			memcpy(&glb.lpZBufferPF[iEnumCount++],
				pddpf,
				sizeof(DDPIXELFORMAT));
		}
	}

	return D3DENUMRET_OK;
}
#endif // _USE_GLD3_WGL

// ***********************************************************************

BOOL IsStencilSupportBroken(LPDIRECTDRAW4 lpDD4)
{
	DDDEVICEIDENTIFIER	dddi; // DX6 device identifier
	BOOL				bBroken = FALSE;

	// Microsoft really fucked up with the GetDeviceIdentifier function
	// on Windows 2000, since it locks up on stock driers on the CD. Updated
	// drivers from vendors appear to work, but we can't identify the drivers
	// without this function!!! For now we skip these tests on Windows 2000.
	if ((GetVersion() & 0x80000000UL) == 0)
		return FALSE;

	// Obtain device info
	if (FAILED(IDirectDraw4_GetDeviceIdentifier(lpDD4, &dddi, 0)))
		return FALSE;

	// Matrox G400 stencil buffer support does not draw anything in AutoCAD,
	// but ordinary Z buffers draw shaded models fine. (DaveM)
	if (dddi.dwVendorId == 0x102B) {		// Matrox
		if (dddi.dwDeviceId == 0x0525) {	// G400
			bBroken = TRUE;
		}
	}

	return bBroken;
}

// ***********************************************************************

void dglBuildPixelFormatList()
{
	int				i;
	char			buf[128];
	char			cat[8];
	DGL_pixelFormat	*lpPF;

#ifdef _USE_GLD3_WGL
	_gldDriver.BuildPixelformatList();
#else
	HRESULT			hRes;
	IDirectDraw		*lpDD1 = NULL;
	IDirectDraw4	*lpDD4 = NULL;
	IDirect3D3		*lpD3D3 = NULL;
	DDSURFACEDESC2	ddsdDisplayMode;

	DWORD			dwRb, dwGb, dwBb, dwAb; // Bit counts
	DWORD			dwRs, dwGs, dwBs, dwAs; // Bit shifts
	DWORD			dwPixelType;			// RGB or color index

	// Set defaults
	glb.nPixelFormatCount	= 0;
	glb.lpPF				= NULL;
	glb.nZBufferPFCount		= 0;
	glb.lpZBufferPF			= NULL;
	glb.nDisplayModeCount	= 0;
	glb.lpDisplayModes		= NULL;

	//
	// Examine the hardware for depth and stencil
	//

	if (glb.bPrimary)
		hRes = DirectDrawCreate(NULL, &lpDD1, NULL);
	else
		hRes = DirectDrawCreate(&glb.ddGuid, &lpDD1, NULL);
		
	if (FAILED(hRes)) {
		ddlogError(DDLOG_ERROR, "dglBPFL: DirectDrawCreate failed", hRes);
		return;
	}

	// Query for DX6 IDirectDraw4.
	hRes = IDirectDraw_QueryInterface(
				lpDD1,
				&IID_IDirectDraw4,
				(void**)&lpDD4);
	if (FAILED(hRes)) {
		ddlogError(DDLOG_ERROR, "dglBPFL: QueryInterface (DD4) failed", hRes);
		goto clean_up;
	}


	// Retrieve caps of current display mode
	ZeroMemory(&ddsdDisplayMode, sizeof(ddsdDisplayMode));
	ddsdDisplayMode.dwSize = sizeof(ddsdDisplayMode);
	hRes = IDirectDraw4_GetDisplayMode(lpDD4, &ddsdDisplayMode);
	if (FAILED(hRes))
		goto clean_up;

	dwDisplayBitDepth = ddsdDisplayMode.ddpfPixelFormat.dwRGBBitCount;
	dwPixelType = (dwDisplayBitDepth <= 8) ? PFD_TYPE_COLORINDEX : PFD_TYPE_RGBA;
	dwRb = BitCount(ddsdDisplayMode.ddpfPixelFormat.dwRBitMask);
	dwGb = BitCount(ddsdDisplayMode.ddpfPixelFormat.dwGBitMask);
	dwBb = BitCount(ddsdDisplayMode.ddpfPixelFormat.dwBBitMask);
	dwRs = BitShift(ddsdDisplayMode.ddpfPixelFormat.dwRBitMask);
	dwGs = BitShift(ddsdDisplayMode.ddpfPixelFormat.dwGBitMask);
	dwBs = BitShift(ddsdDisplayMode.ddpfPixelFormat.dwBBitMask);

	if (BitCount(ddsdDisplayMode.ddpfPixelFormat.dwRGBAlphaBitMask)) {
		dwAb = BitCount(ddsdDisplayMode.ddpfPixelFormat.dwRGBAlphaBitMask);
		dwAs = BitShift(ddsdDisplayMode.ddpfPixelFormat.dwRGBAlphaBitMask);
	} else {
		dwAb = 0;
		dwAs = 0;
	}

	// Query for available display modes
	ddlogMessage(DDLOG_INFO, "\n");
	ddlogMessage(DDLOG_INFO, "Display Modes:\n");

	// Pass 1: Determine count
	iEnumCount = 0;
	hRes = IDirectDraw4_EnumDisplayModes(
				lpDD4,
				0,
				NULL,
				NULL,
				EnumDisplayModesCallback);
	if (FAILED(hRes)) {
		ddlogError(DDLOG_ERROR, "dglBPFL: EnumDisplayModes failed", hRes);
		goto clean_up;
	}
	if (iEnumCount == 0) {
		ddlogMessage(DDLOG_ERROR, "dglBPFL: No display modes found");
		goto clean_up;
	}
	glb.lpDisplayModes = (DDSURFACEDESC2 *)calloc(iEnumCount,
												sizeof(DDSURFACEDESC2));
	if (glb.lpDisplayModes == NULL) {
		ddlogMessage(DDLOG_ERROR, "dglBPFL: DDSURFACEDESC2 calloc failed");
		goto clean_up;
	}
	glb.nDisplayModeCount = iEnumCount;
	// Pass 2: Save modes
	iEnumCount = 0;
	hRes = IDirectDraw4_EnumDisplayModes(
				lpDD4,
				0,
				NULL,
				(void *)glb.lpDisplayModes,
				EnumDisplayModesCallback);
	if (FAILED(hRes)) {
		ddlogError(DDLOG_ERROR, "dglBPFL: EnumDisplayModes failed", hRes);
		goto clean_up;
	}
							  // Query for IDirect3D3 interface
	hRes = IDirectDraw4_QueryInterface(
				lpDD4,
				&IID_IDirect3D3,
				(void**)&lpD3D3);
	if (FAILED(hRes)) {
		ddlogError(DDLOG_ERROR, "dglBPFL: QueryInterface (D3D3) failed", hRes);
		goto clean_up;
	}

	ddlogMessage(DDLOG_INFO, "\n");
	ddlogMessage(DDLOG_INFO, "ZBuffer formats:\n");

	// Pass 1. Count the ZBuffer pixel formats
	hRes = IDirect3D3_EnumZBufferFormats(
				lpD3D3,
				&glb.d3dGuid,
				d3dEnumZBufferFormatsCallback,
				NULL);

	if (FAILED(hRes))
		goto clean_up;

	if (glb.nZBufferPFCount) {
		glb.lpZBufferPF = (DDPIXELFORMAT *)calloc(glb.nZBufferPFCount,
												sizeof(DDPIXELFORMAT));
		if(glb.lpZBufferPF == NULL)
			goto clean_up;

		// Pass 2. Cache the ZBuffer pixel formats
		iEnumCount = 0; // (Used by the enum function)
		hRes = IDirect3D3_EnumZBufferFormats(
					lpD3D3,
					&glb.d3dGuid,
					d3dEnumZBufferFormatsCallback,
					glb.lpZBufferPF);

		if (FAILED(hRes))
			goto clean_up;
	}

	// Remove stencil support for boards which don't work for AutoCAD;
	// Matrox G400 does not work, but NVidia TNT2 and ATI Rage128 do... (DaveM)
	if (IsStencilSupportBroken(lpDD4)) {
		for (i=0; i<iEnumCount; i++)
			if (glb.lpZBufferPF[i].dwFlags & DDPF_STENCILBUFFER)
				glb.nZBufferPFCount--;
	}

	// One each for every ZBuffer pixel format (including no depth buffer)
	// Times-two because duplicated for single buffering (as opposed to double)
	glb.nPixelFormatCount = 2 * (glb.nZBufferPFCount + 1);
	glb.lpPF = (DGL_pixelFormat *)calloc(glb.nPixelFormatCount,
										sizeof(DGL_pixelFormat));
	if (glb.lpPF == NULL)
		goto clean_up;
	//
	// Fill in the pixel formats
	// Note: Depth buffer bits are really (dwZBufferBitDepth-dwStencilBitDepth)
	//		 but this will pass wierd numbers to the OpenGL app. (?)
	//

	pfTemplateHW.pfd.iPixelType		= dwPixelType;
	pfTemplateHW.pfd.cColorBits		= dwDisplayBitDepth;
	pfTemplateHW.pfd.cRedBits		= dwRb;
	pfTemplateHW.pfd.cGreenBits		= dwGb;
	pfTemplateHW.pfd.cBlueBits		= dwBb;
	pfTemplateHW.pfd.cAlphaBits		= dwAb;
	pfTemplateHW.pfd.cRedShift		= dwRs;
	pfTemplateHW.pfd.cGreenShift	= dwGs;
	pfTemplateHW.pfd.cBlueShift		= dwBs;
	pfTemplateHW.pfd.cAlphaShift	= dwAs;

	lpPF = glb.lpPF;

	// Fill in the double-buffered pixel formats
	for (i=0; i<(glb.nZBufferPFCount + 1); i++, lpPF++) {
		memcpy(lpPF, &pfTemplateHW, sizeof(DGL_pixelFormat));
		if (i) {
			lpPF->iZBufferPF		= i-1;
			lpPF->pfd.cDepthBits	= glb.lpZBufferPF[i-1].dwZBufferBitDepth;
			lpPF->pfd.cStencilBits	= glb.lpZBufferPF[i-1].dwStencilBitDepth;
		}
	}
	// Fill in the single-buffered pixel formats
	for (i=0; i<(glb.nZBufferPFCount + 1); i++, lpPF++) {
		memcpy(lpPF, &pfTemplateHW, sizeof(DGL_pixelFormat));
		if (i) {
			lpPF->iZBufferPF		= i-1;
			lpPF->pfd.cDepthBits	= glb.lpZBufferPF[i-1].dwZBufferBitDepth;
			lpPF->pfd.cStencilBits	= glb.lpZBufferPF[i-1].dwStencilBitDepth;
		}
		// Remove double-buffer flag. Could use XOR instead...
		lpPF->pfd.dwFlags &= (~(PFD_DOUBLEBUFFER));
		// Insert GDI flag for single buffered format only.
		lpPF->pfd.dwFlags |= PFD_SUPPORT_GDI;
	}
#endif // _USE_GLD3_WGL

	// Lets dump the list to the log
	// ** Based on "wglinfo" by Nate Robins **
	ddlogMessage(DDLOG_INFO, "\n");
	ddlogMessage(DDLOG_INFO, "Pixel Formats:\n");
	ddlogMessage(DDLOG_INFO,
		"   visual  x  bf lv rg d st  r  g  b a  ax dp st accum buffs  ms\n");
	ddlogMessage(DDLOG_INFO,
		" id dep cl sp sz l  ci b ro sz sz sz sz bf th cl  r  g  b  a ns b\n");
	ddlogMessage(DDLOG_INFO,
		"-----------------------------------------------------------------\n");
	for (i=0, lpPF = glb.lpPF; i<glb.nPixelFormatCount; i++, lpPF++) {
		sprintf(buf, "0x%02x ", i+1);

		sprintf(cat, "%2d ", lpPF->pfd.cColorBits);
		strcat(buf, cat);
		if(lpPF->pfd.dwFlags & PFD_DRAW_TO_WINDOW)      sprintf(cat, "wn ");
		else if(lpPF->pfd.dwFlags & PFD_DRAW_TO_BITMAP) sprintf(cat, "bm ");
		else sprintf(cat, ".  ");
		strcat(buf, cat);

		/* should find transparent pixel from LAYERPLANEDESCRIPTOR */
		sprintf(cat, " . "); 
		strcat(buf, cat);

		sprintf(cat, "%2d ", lpPF->pfd.cColorBits);
		strcat(buf, cat);

		/* bReserved field indicates number of over/underlays */
		if(lpPF->pfd.bReserved) sprintf(cat, " %d ", lpPF->pfd.bReserved);
		else sprintf(cat, " . "); 
		strcat(buf, cat);

		sprintf(cat, " %c ", lpPF->pfd.iPixelType == PFD_TYPE_RGBA ? 'r' : 'c');
		strcat(buf, cat);

		sprintf(cat, "%c ", lpPF->pfd.dwFlags & PFD_DOUBLEBUFFER ? 'y' : '.');
		strcat(buf, cat);

		sprintf(cat, " %c ", lpPF->pfd.dwFlags & PFD_STEREO ? 'y' : '.');
		strcat(buf, cat);

		if(lpPF->pfd.cRedBits && lpPF->pfd.iPixelType == PFD_TYPE_RGBA) 
		    sprintf(cat, "%2d ", lpPF->pfd.cRedBits);
		else sprintf(cat, " . ");
		strcat(buf, cat);

		if(lpPF->pfd.cGreenBits && lpPF->pfd.iPixelType == PFD_TYPE_RGBA) 
		    sprintf(cat, "%2d ", lpPF->pfd.cGreenBits);
		else sprintf(cat, " . ");
		strcat(buf, cat);

		if(lpPF->pfd.cBlueBits && lpPF->pfd.iPixelType == PFD_TYPE_RGBA) 
		    sprintf(cat, "%2d ", lpPF->pfd.cBlueBits);
		else sprintf(cat, " . ");
		strcat(buf, cat);
	
		if(lpPF->pfd.cAlphaBits && lpPF->pfd.iPixelType == PFD_TYPE_RGBA) 
			sprintf(cat, "%2d ", lpPF->pfd.cAlphaBits);
		else sprintf(cat, " . ");
		strcat(buf, cat);
	
		if(lpPF->pfd.cAuxBuffers)     sprintf(cat, "%2d ", lpPF->pfd.cAuxBuffers);
		else sprintf(cat, " . ");
		strcat(buf, cat);
	
		if(lpPF->pfd.cDepthBits)      sprintf(cat, "%2d ", lpPF->pfd.cDepthBits);
		else sprintf(cat, " . ");
		strcat(buf, cat);
	
		if(lpPF->pfd.cStencilBits)    sprintf(cat, "%2d ", lpPF->pfd.cStencilBits);
		else sprintf(cat, " . ");
		strcat(buf, cat);
	
		if(lpPF->pfd.cAccumRedBits)   sprintf(cat, "%2d ", lpPF->pfd.cAccumRedBits);
		else sprintf(cat, " . ");
		strcat(buf, cat);

		if(lpPF->pfd.cAccumGreenBits) sprintf(cat, "%2d ", lpPF->pfd.cAccumGreenBits);
		else sprintf(cat, " . ");
		strcat(buf, cat);
	
		if(lpPF->pfd.cAccumBlueBits)  sprintf(cat, "%2d ", lpPF->pfd.cAccumBlueBits);
		else sprintf(cat, " . ");
		strcat(buf, cat);
	
		if(lpPF->pfd.cAccumAlphaBits) sprintf(cat, "%2d ", lpPF->pfd.cAccumAlphaBits);
		else sprintf(cat, " . ");
		strcat(buf, cat);
	
		/* no multisample in Win32 */
		sprintf(cat, " . .\n");
		strcat(buf, cat);

		ddlogMessage(DDLOG_INFO, buf);
	}
	ddlogMessage(DDLOG_INFO,
		"-----------------------------------------------------------------\n");
	ddlogMessage(DDLOG_INFO, "\n");

#ifndef _USE_GLD3_WGL
clean_up:
	// Release COM objects
	RELEASE(lpD3D3);
	RELEASE(lpDD4);
	RELEASE(lpDD1);

	// Popup warning message if non RGB color mode
	if (dwDisplayBitDepth <= 8) {
		ddlogPrintf(DDLOG_WARN, "Current Color Depth %d bpp is not supported", dwDisplayBitDepth);
		MessageBox(NULL, szColorDepthWarning, "GLDirect", MB_OK | MB_ICONWARNING);
	}
#endif // _USE_GLD3_WGL
}

// ***********************************************************************

void dglReleasePixelFormatList()
{
	glb.nPixelFormatCount = 0;
	if (glb.lpPF) {
		free(glb.lpPF);
		glb.lpPF = NULL;
	}
#ifndef _USE_GLD3_WGL
	glb.nZBufferPFCount = 0;
	if (glb.lpZBufferPF) {
		free(glb.lpZBufferPF);
		glb.lpZBufferPF = NULL;
	}
	glb.nDisplayModeCount = 0;
	if (glb.lpDisplayModes) {
		free(glb.lpDisplayModes);
		glb.lpDisplayModes = NULL;
	}
#endif // _USE_GLD3_WGL
}

// ***********************************************************************
