/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/adapter.h
 * PURPOSE:         d3d9.dll adapter info functions
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#ifndef _ADAPTER_H_
#define _ADAPTER_H_

BOOL GetAdapterInfo(LPCSTR lpszDeviceName, D3DADAPTER_IDENTIFIER9* pIdentifier);

BOOL GetAdapterMode(LPCSTR lpszDeviceName, D3DDISPLAYMODE* pMode);

HRESULT GetAdapterCaps(const LPDIRECT3D9_DISPLAYADAPTER pDisplayAdapter, D3DDEVTYPE DeviceType, D3DCAPS9* pDstCaps);

HMONITOR GetAdapterMonitor(LPCSTR lpszDeviceName);

UINT GetDisplayFormatCount(D3DFORMAT Format, const D3DDISPLAYMODE* pSupportedDisplayModes, UINT NumDisplayModes);
const D3DDISPLAYMODE* FindDisplayFormat(D3DFORMAT Format, UINT ModeIndex, const D3DDISPLAYMODE* pSupportedDisplayModes, UINT NumDisplayModes);

#endif
