/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_baseobject.h
 * PURPOSE:         Direct3D9's base object
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#ifndef _D3D9_BASEOBJECT_H_
#define _D3D9_BASEOBJECT_H_

#include "d3d9_common.h"
#include <d3d9.h>

struct _D3D9BaseObject;
struct _Direct3DDevice9_INT;

enum REF_TYPE
{
    RT_EXTERNAL,
    RT_BUILTIN,
    RT_INTERNAL,
};

typedef struct _D3D9BaseObjectVtbl
{
    VOID (*Destroy)(struct _D3D9BaseObject* pBaseObject, BOOL bFreeThis);
} ID3D9BaseObjectVtbl;

typedef struct _D3D9BaseObject
{
/* 0x0000 */    ID3D9BaseObjectVtbl* lpVtbl;
/* 0x0004 */    LONG lRefCnt;
/* 0x0008 */    DWORD dwNumUsed;
/* 0x000c */    IUnknown* pUnknown;
/* 0x0010 */    DWORD dwUnknown0010;    // Index? Unique id?
/* 0x0014 */    HANDLE hKernelHandle;
/* 0x0018 */    LPDWORD dwUnknown0018;
/* 0x001c */    enum REF_TYPE RefType;
} D3D9BaseObject;

VOID InitD3D9BaseObject(D3D9BaseObject* pBaseObject, enum REF_TYPE RefType, IUnknown* pUnknown);

ULONG D3D9BaseObject_AddRef(D3D9BaseObject* pBaseObject);
ULONG D3D9BaseObject_Release(D3D9BaseObject* pBaseObject);
HRESULT D3D9BaseObject_GetDevice(D3D9BaseObject* pBaseObject, IDirect3DDevice9** ppDevice);
HRESULT D3D9BaseObject_GetDeviceInt(D3D9BaseObject* pBaseObject, struct _Direct3DDevice9_INT** ppDevice);
VOID D3D9BaseObject_LockDevice(D3D9BaseObject* pBaseObject);
VOID D3D9BaseObject_UnlockDevice(D3D9BaseObject* pBaseObject);

#endif // _D3D9_BASEOBJECT_H_
