/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/format.h
 * PURPOSE:         d3d9.dll D3DFORMAT helper functions
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#ifndef _FORMAT_H_
#define _FORMAT_H_

#include <d3d9.h>
#include "d3d9_private.h"

BOOL IsBackBufferFormat(D3DFORMAT Format);

BOOL IsExtendedFormat(D3DFORMAT Format);

BOOL IsSupportedFormatOp(LPD3D9_DRIVERCAPS pDriverCaps, D3DFORMAT DisplayFormat, DWORD FormatOp);

HRESULT CheckDeviceFormat(LPD3D9_DRIVERCAPS pDriverCaps, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed);

#endif // _FORMAT_H_
