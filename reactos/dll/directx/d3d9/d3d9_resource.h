/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_resource.h
 * PURPOSE:         d3d9.dll internal resource structures
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#ifndef _D3D9_RESOURCE_H_
#define _D3D9_RESOURCE_H_

#include "d3d9_baseobject.h"

typedef struct _Direct3DResource9_INT
{
/* 0x0000 */    D3D9BaseObject BaseObject;
/* 0x0020 */    DWORD dwUnknown20;
/* 0x0024 */    DWORD dwUnknown24;
/* 0x0028 */    DWORD dwUnknown28;
/* 0x002c */    DWORD dwUnknown2c;
/* 0x0030 */    DWORD dwUnknown30;
/* 0x0034 */    DWORD dwUnknown34;
/* 0x0038 */    D3DPOOL Pool;
/* 0x003c */    LPDWORD dwUnknown3c;
/* 0x0040 */    LPDWORD dwUnknown40;
} Direct3DResource9_INT;

void InitDirect3DResource9(Direct3DResource9_INT* pResource, D3DPOOL Pool, struct _Direct3DDevice9_INT* pDevice, enum REF_TYPE RefType);

#endif // _D3D9_RESOURCE_H_
