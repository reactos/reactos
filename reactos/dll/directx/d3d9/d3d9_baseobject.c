#include "d3d9_baseobject.h"
#include "d3d9_device.h"

#include <debug.h>

VOID D3D9BaseObject_Destroy()
{
}

ID3D9BaseObjectVtbl D3D9BaseObject_Vtbl =
{
    /* D3D9BaseObject */
    D3D9BaseObject_Destroy
};

VOID InitD3D9BaseObject(D3D9BaseObject* pBaseObject, enum REF_TYPE RefType, struct _Direct3DDevice9_INT* pBaseDevice)
{
    // TODO: Add dtor to vtbl
    pBaseObject->lpVtbl = &D3D9BaseObject_Vtbl;
    pBaseObject->RefType = RefType;
    pBaseObject->pBaseDevice = pBaseDevice;
}

ULONG D3D9BaseObject_AddRef(D3D9BaseObject* pBaseObject)
{
    // TODO: Implement ref counting
    UNIMPLEMENTED
    return 1;
}

ULONG D3D9BaseObject_Release(D3D9BaseObject* pBaseObject)
{
    // TODO: Implement ref counting
    UNIMPLEMENTED
    return 0;
}
