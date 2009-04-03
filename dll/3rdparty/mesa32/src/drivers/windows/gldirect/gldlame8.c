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
* Description:  GLDirect utility for determining lame boards/drivers.
*
****************************************************************************/

#define STRICT
#define	WIN32_LEAN_AND_MEAN
#include <d3d8.h>

/*
Ack. Broken out from gldlame.c because of broken D3D headers. KeithH
*/

/****************************************************************************
REMARKS:
Scans list of DirectDraw devices for specific device IDs.
****************************************************************************/

#define VENDORID_ATI 0x1002

static DWORD devATIRagePro[] = {
	0x4742, // 3D RAGE PRO BGA AGP 1X/2X
	0x4744, // 3D RAGE PRO BGA AGP 1X only
	0x4749, // 3D RAGE PRO BGA PCI 33 MHz
	0x4750, // 3D RAGE PRO PQFP PCI 33 MHz
	0x4751, // 3D RAGE PRO PQFP PCI 33 MHz limited 3D
	0x4C42, // 3D RAGE LT PRO BGA-312 AGP 133 MHz
	0x4C44, // 3D RAGE LT PRO BGA-312 AGP 66 MHz
	0x4C49, // 3D RAGE LT PRO BGA-312 PCI 33 MHz
	0x4C50, // 3D RAGE LT PRO BGA-256 PCI 33 MHz
	0x4C51, // 3D RAGE LT PRO BGA-256 PCI 33 MHz limited 3D
};

static DWORD devATIRageIIplus[] = {
	0x4755, // 3D RAGE II+
	0x4756, // 3D RAGE IIC PQFP PCI
	0x4757, // 3D RAGE IIC BGA AGP
	0x475A, // 3D RAGE IIC PQFP AGP
	0x4C47, // 3D RAGE LT-G
};

static __inline BOOL IsDevice(
	DWORD *lpDeviceIdList,
	DWORD dwDeviceId,
	int count)
{
	int i;

	for (i=0; i<count; i++)
		if (dwDeviceId == lpDeviceIdList[i])
			return TRUE;

	return FALSE;
}

/****************************************************************************
REMARKS:
Test the Direct3D8 device for "lameness" with respect to GLDirect.
This is done on per-chipset basis, as in GLD CAD driver (DGLCONTEXT.C).
If bTestForWHQL is set then the device is tested to see if it is
certified, and bIsWHQL is set to indicate TRUE or FALSE. Otherwise bIsWHQL
is not set. [WHQL = Windows Hardware Quality Labs]

NOTE: There is a one- or two-second time penalty incurred in determining
      the WHQL certification date.
****************************************************************************/
BOOL IsThisD3D8Lame(
	IDirect3D8 *pD3D,
	DWORD dwAdapter,
	BOOL bTestForWHQL,
	BOOL *bIsWHQL)
{
	DWORD					dwFlags = bTestForWHQL ? 0 : D3DENUM_NO_WHQL_LEVEL;
	D3DADAPTER_IDENTIFIER8	d3dai;
	HRESULT					hr;

	hr = IDirect3D8_GetAdapterIdentifier(pD3D, dwAdapter, dwFlags, &d3dai);
	if (FAILED(hr))
		return TRUE; // Definitely lame if adapter details can't be obtained!

	if (bTestForWHQL) {
		*bIsWHQL = d3dai.WHQLLevel ? TRUE : FALSE;
	}

	// Vendor 1: ATI
	if (d3dai.VendorId == VENDORID_ATI) {
		// Test A: ATI Rage PRO
		if (IsDevice(devATIRagePro, d3dai.DeviceId, sizeof(devATIRagePro)))
			return TRUE;	// bad mipmapping
		// Test B: ATI Rage II+
		if (IsDevice(devATIRageIIplus, d3dai.DeviceId, sizeof(devATIRageIIplus)))
			return TRUE; 	// bad HW alpha testing
	}

	return FALSE;
}

/****************************************************************************
REMARKS:
Test the Direct3DDevice8 device for "lameness" with respect to GLDirect.
This is done by querying for particular caps, as in GLD CPL (CPLMAIN.CPP).
****************************************************************************/
BOOL IsThisD3D8DeviceLame(
	IDirect3DDevice8 *pDev)
{
	D3DCAPS8	d3dCaps;
	HRESULT		hr;

	hr = IDirect3DDevice8_GetDeviceCaps(pDev, &d3dCaps);
	if (FAILED(hr))
		return TRUE;

	// Test 1: Perspective-correct textures
	// Any card that cannot do perspective-textures is *exceptionally* lame.
	if (!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_PERSPECTIVE)) {
		return TRUE; // Lame!
	}

	// Test 2: Bilinear filtering
	if (!(d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR)) {
		return TRUE; // Lame!
	}

	// Test 3: Mipmapping
	if (!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_MIPMAP)) {
		return TRUE; // Lame!
	}

	// Test 4: Depth-test modes (?)

	// Test 5: Blend Modes -- Based on DX7 D3DIM MTEXTURE.CPP caps test

	// Accept devices that can do multipass, alpha blending
	if( !((d3dCaps.DestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA) &&
			(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_SRCALPHA)) )
		return TRUE;

	// Accept devices that can do multipass, color blending
	if( !((d3dCaps.DestBlendCaps & D3DPBLENDCAPS_SRCCOLOR) &&
			(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_ZERO)) )
		return TRUE;

	// Accept devices that really support multiple textures.
	if( !((d3dCaps.MaxTextureBlendStages > 1 ) &&
			(d3dCaps.MaxSimultaneousTextures > 1 ) &&
			(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_MODULATE )) )
		return TRUE;

	return FALSE; // Not lame
}
