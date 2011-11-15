#pragma once

/* Object structure */
typedef struct _EDRIVEROBJ
{
    BASEOBJECT baseobj;
    DRIVEROBJ drvobj;
    PVOID reserved;
} EDRIVEROBJ, *PEDRIVEROBJ;

typedef DRIVEROBJ *PDRIVEROBJ;

/* Cleanup function */
BOOL NTAPI DRIVEROBJ_Cleanup(PVOID pObject);


#define DRIVEROBJ_AllocObjectWithHandle()  ((PEDRIVEROBJ)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_DRIVEROBJ, sizeof(DRIVEROBJ)))
#define DRIVEROBJ_FreeObjectByHandle(hdo) GDIOBJ_FreeObjByHandle((HGDIOBJ)hdo, GDI_OBJECT_TYPE_DRIVEROBJ)
#define DRIVEROBJ_UnlockObject(pdo) GDIOBJ_vUnlockObject((POBJ)pdo)

FORCEINLINE
PEDRIVEROBJ
DRIVEROBJ_LockObject(HDRVOBJ hdo)
{
    return GDIOBJ_LockObject(hdo, GDIObjType_DRVOBJ_TYPE);
}
