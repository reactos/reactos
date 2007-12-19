/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_helpers.h
 * PURPOSE:         d3d9.dll helper functions
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */
#ifndef _D3D9_HELPERS_H_
#define _D3D9_HELPERS_H_

#include "d3d9_private.h"

/* Convert a IDirect3D9 pointer safely to the internal implementation struct */
LPDIRECT3D9_INT impl_from_IDirect3D9(LPDIRECT3D9 iface);

/* Reads a registry value if it's of the correct value type */
BOOL ReadRegistryValue(IN DWORD ValueType, IN LPCSTR ValueName, OUT LPBYTE DataBuffer, IN OUT LPDWORD DataBufferSize);

/* Formats debug strings */
HRESULT FormatDebugString(IN OUT LPSTR Buffer, IN LONG BufferSize, IN LPCSTR FormatString, ... );

/* Creates a Direct3D9 object */
HRESULT CreateD3D9(OUT LPDIRECT3D9 *ppDirect3D9);

#endif // _D3D9_HELPERS_H_
