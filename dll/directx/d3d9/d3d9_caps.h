/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_caps.h
 * PURPOSE:         d3d9.dll device/driver caps functions, defines and macros
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#ifndef _D3D9_CAPS_H_
#define _D3D9_CAPS_H_

#include "d3d9_private.h"
#include <d3dhal.h>

#define DX9_DDI_VERSION     4

void CreateDisplayModeList(LPCSTR lpszDeviceName, D3DDISPLAYMODE* pDisplayModes, DWORD* pNumDisplayModes, D3DFORMAT DisplayFormat, D3D9_Unknown6BC* pUnknown6BC);

BOOL GetDeviceData(LPD3D9_DEVICEDATA pDeviceData);

BOOL CanReenableDirectDrawObject(D3D9_Unknown6BC* ppUnknown);

BOOL GetD3D9DriverInfo( D3D9_Unknown6BC* pUnknown6BC,
                        LPD3D9_DRIVERCAPS pDriverCaps,
                        D3D9_CALLBACKS* pD3D9Callbacks,
                        LPCSTR lpszDeviceName,
                        HMODULE hD3dRefDll,
                        D3DHAL_GLOBALDRIVERDATA* pGblDriverData,
                        D3DHAL_D3DEXTENDEDCAPS* pD3dExtendedCaps,
                        LPDDSURFACEDESC puD3dTextureFormats,
                        DDPIXELFORMAT* pD3dZStencilFormatList,
                        D3DDISPLAYMODE* pD3dDisplayModeList,
                        D3DQUERYTYPE* pD3dQueryList,
                        LPDWORD pNumTextureFormats,
                        LPDWORD pNumZStencilFormats,
                        LPDWORD pNumExtendedFormats,
                        LPDWORD pNumQueries);

#endif // _D3D9_CAPS_H_
