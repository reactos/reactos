#ifndef _WIN32K_DRIVEROBJ_H
#define _WIN32K_DRIVEROBJ_H

#include "gdiobj.h"

/* Object structure */
typedef struct _EDRIVEROBJ
{
    BASEOBJECT baseobj;
    DRIVEROBJ drvobj;
    PVOID reserved;
} EDRIVEROBJ, *PEDRIVEROBJ;

/* Cleanup function */
BOOL INTERNAL_CALL DRIVEROBJ_Cleanup(PVOID pObject);


#define DRIVEROBJ_AllocObjectWithHandle()  ((PEDRIVEROBJ)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_DRIVEROBJ))
#define DRIVEROBJ_FreeObjectByHandle(hdo) GDIOBJ_FreeObjByHandle((HGDIOBJ)hdo, GDI_OBJECT_TYPE_DRIVEROBJ)
#define DRIVEROBJ_LockObject(hdo) ((PEDRIVEROBJ)GDIOBJ_LockObj((HGDIOBJ)hdo, GDI_OBJECT_TYPE_DRIVEROBJ))
#define DRIVEROBJ_UnlockObject(pdo) GDIOBJ_UnlockObjByPtr((POBJ)pdo)

#endif /* !_WIN32K_DRIVEROBJ_H */
