#ifndef _D3D9_BASEOBJECT_H_
#define _D3D9_BASEOBJECT_H_

#include "d3d9_common.h"

enum REF_TYPE
{
    RT_EXTERNAL,
    RT_BUILTIN,
    RT_INTERNAL,
};

typedef struct _D3D9BaseObjectVtbl
{
    VOID (*Destroy)();
} ID3D9BaseObjectVtbl;

typedef struct _D3D9BaseObject
{
/* 0x0000 */    ID3D9BaseObjectVtbl* lpVtbl;
/* 0x0004 */    DWORD dwUnknown0004;
/* 0x0008 */    DWORD dwUnknown0008;
/* 0x000c */    struct _Direct3DDevice9_INT* pBaseDevice;
/* 0x0010 */    DWORD dwUnknown0010;    // Index? Unique id?
/* 0x0014 */    HANDLE hKernelHandle;
/* 0x0018 */    LPDWORD dwUnknown0018;
/* 0x001c */    enum REF_TYPE RefType;
} D3D9BaseObject;

VOID InitD3D9BaseObject(D3D9BaseObject* pBaseObject, enum REF_TYPE RefType, struct _Direct3DDevice9_INT* pBaseDevice);

ULONG D3D9BaseObject_AddRef(D3D9BaseObject* pBaseObject);
ULONG D3D9BaseObject_Release(D3D9BaseObject* pBaseObject);

#endif // _D3D9_BASEOBJECT_H_
