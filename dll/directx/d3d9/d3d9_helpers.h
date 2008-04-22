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

#define LOCK_D3D9()     EnterCriticalSection(&This->d3d9_cs);
#define UNLOCK_D3D9()   LeaveCriticalSection(&This->d3d9_cs);

/* Convert a IDirect3D9 pointer safely to the internal implementation struct */
LPDIRECT3D9_INT impl_from_IDirect3D9(LPDIRECT3D9 iface);

/* Reads a registry value if it's of the correct value type */
BOOL ReadRegistryValue(IN DWORD ValueType, IN LPCSTR ValueName, OUT LPBYTE DataBuffer, IN OUT LPDWORD DataBufferSize);

/* Allocates memory and returns an aligned pointer */
HRESULT AlignedAlloc(IN OUT LPVOID *ppObject, IN SIZE_T dwSize);

/* Frees memory allocated with AlignedAlloc */
VOID AlignedFree(IN OUT LPVOID pObject);


#endif // _D3D9_HELPERS_H_
