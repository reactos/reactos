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

HMONITOR GetAdapterMonitor(LPCSTR lpszDeviceName);

#endif
