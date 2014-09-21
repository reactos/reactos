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

#define D3DFORMAT_OP_DMAP                   0x00020000L

/* MSVC compile fix */
#ifndef D3DFORMAT_OP_NOTEXCOORDWRAPNORMIP
#define D3DFORMAT_OP_NOTEXCOORDWRAPNORMIP   0x01000000L
#endif

BOOL IsBackBufferFormat(D3DFORMAT Format);

BOOL IsExtendedFormat(D3DFORMAT Format);

BOOL IsZBufferFormat(D3DFORMAT Format);

BOOL IsMultiElementFormat(D3DFORMAT Format);

BOOL IsSupportedFormatOp(LPD3D9_DRIVERCAPS pDriverCaps, D3DFORMAT DisplayFormat, DWORD FormatOp);

HRESULT CheckDeviceType(LPD3D9_DRIVERCAPS pDriverCaps, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed);

HRESULT CheckDeviceFormat(LPD3D9_DRIVERCAPS pDriverCaps, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat);

HRESULT CheckDeviceFormatConversion(LPD3D9_DRIVERCAPS pDriverCaps, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat);

HRESULT CheckDepthStencilMatch(LPD3D9_DRIVERCAPS pDriverCaps, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat);

#endif // _FORMAT_H_
