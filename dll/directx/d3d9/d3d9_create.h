/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_create.h
 * PURPOSE:         d3d9.dll internal create functions and data
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */
#ifndef _D3D9_CREATE_H_
#define _D3D9_CREATE_H_

#include <windows.h>
#include <d3d9types.h>
#include "d3d9_private.h"

/* Creates a Direct3D9 object */
HRESULT CreateD3D9(OUT LPDIRECT3D9 *ppDirect3D9, UINT SDKVersion);

BOOL CreateD3D9DeviceData(IN LPDIRECT3D9_DISPLAYADAPTER pDisplayAdapter, IN LPD3D9_DEVICEDATA pDeviceData);
VOID DestroyD3D9DeviceData(IN LPD3D9_DEVICEDATA pDeviceData);

#endif // _D3D9_CREATE_H_
