/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_resource.c
 * PURPOSE:         d3d9.dll internal resource functions
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#include "d3d9_resource.h"
#include "d3d9_device.h"

void InitDirect3DResource9(Direct3DResource9_INT* pResource, D3DPOOL Pool, LPDIRECT3DDEVICE9_INT pBaseDevice, enum REF_TYPE RefType)
{
    InitD3D9BaseObject(&pResource->BaseObject, RefType, (IUnknown*)&pBaseDevice->lpVtbl);

    pResource->Pool = Pool;
}
