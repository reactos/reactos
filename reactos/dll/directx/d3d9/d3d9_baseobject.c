/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_baseobject.c
 * PURPOSE:         Direct3D9's base object
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#include "d3d9_baseobject.h"
#include "d3d9_device.h"

#include <debug.h>
#include "d3d9_helpers.h"

VOID D3D9BaseObject_Destroy(D3D9BaseObject* pBaseObject, BOOL bFreeThis)
{
    if (bFreeThis)
    {
        AlignedFree((LPVOID*) pBaseObject);
    }
}

ID3D9BaseObjectVtbl D3D9BaseObject_Vtbl =
{
    /* D3D9BaseObject */
    D3D9BaseObject_Destroy
};

VOID InitD3D9BaseObject(D3D9BaseObject* pBaseObject, enum REF_TYPE RefType, IUnknown* pUnknown)
{
    pBaseObject->lpVtbl = &D3D9BaseObject_Vtbl;
    pBaseObject->RefType = RefType;
    pBaseObject->pUnknown = pUnknown;
}

ULONG D3D9BaseObject_AddRef(D3D9BaseObject* pBaseObject)
{
    if (pBaseObject->pUnknown)
    {
        pBaseObject->pUnknown->lpVtbl->AddRef((IUnknown*) &pBaseObject->pUnknown->lpVtbl);
    }

    return InterlockedIncrement(&pBaseObject->lRefCnt);
}

ULONG D3D9BaseObject_Release(D3D9BaseObject* pBaseObject)
{
    ULONG Ref = 0;

    if (pBaseObject)
    {
        Ref = InterlockedDecrement(&pBaseObject->lRefCnt);

        if (Ref == 0)
        {
            if (pBaseObject->pUnknown)
            {
                pBaseObject->pUnknown->lpVtbl->Release((IUnknown*) &pBaseObject->pUnknown->lpVtbl);
            }
        }
    }

    return Ref;
}
