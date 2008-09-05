/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/device.h
 * PURPOSE:         Direct3D9's device creation
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "d3d9_haldevice.h"

HRESULT CreateD3D9HalDevice(LPDIRECT3D9_INT pDirect3D9, UINT Adapter,
                            HWND hFocusWindow, DWORD BehaviourFlags,
                            D3DPRESENT_PARAMETERS* pPresentationParameters,
                            DWORD NumAdaptersToCreate,
                            struct IDirect3DDevice9** ppReturnedDeviceInterface);

#endif // _DEVICE_H_
