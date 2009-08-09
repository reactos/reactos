/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_helpers.h
 * PURPOSE:         d3d9.dll helper functions
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */
#ifndef _D3D9_HELPERS_H_
#define _D3D9_HELPERS_H_

#include "d3d9_common.h"
#include "d3d9_private.h"


/* Reads a registry value if it's of the correct value type */
BOOL ReadRegistryValue(IN DWORD ValueType, IN LPCSTR ValueName, OUT LPBYTE DataBuffer, IN OUT LPDWORD DataBufferSize);

/* Safe string formatting */
HRESULT SafeFormatString(IN OUT LPSTR Buffer, IN DWORD BufferSize, IN LPCSTR FormatString, ... );
HRESULT SafeCopyString(OUT LPSTR Dst, IN DWORD DstSize, IN LPCSTR Src);
HRESULT SafeAppendString(IN OUT LPSTR Dst, IN DWORD DstSize, IN LPCSTR Src);

/* Allocates memory and returns an aligned pointer */
HRESULT AlignedAlloc(IN OUT LPVOID *ppObject, IN SIZE_T dwSize);

/* Frees memory allocated with AlignedAlloc */
VOID AlignedFree(IN OUT LPVOID pObject);


#endif // _D3D9_HELPERS_H_
