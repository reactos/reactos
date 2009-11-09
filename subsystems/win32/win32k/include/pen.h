#ifndef __WIN32K_PEN_H
#define __WIN32K_PEN_H

#include "gdiobj.h"
#include "brush.h"

/* Internal interface */

#define PEN_AllocPen() ((HPEN)GDIOBJ_AllocObj(GDIObjType_BRUSH_TYPE))
#define PEN_AllocPenWithHandle() ((PBRUSH)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_PEN))

#define PEN_FreePen(pBMObj) GDIOBJ_FreeObj((POBJ) pBMObj, GDIObjType_BRUSH_TYPE)
#define PEN_FreePenByHandle(hBMObj) GDIOBJ_FreeObjByHandle((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_PEN)

//#define PEN_LockPen(hBMObj) ((PBRUSH)GDIOBJ_LockObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_PEN))

#define PEN_AllocExtPen() ((PBRUSH)GDIOBJ_AllocObj(GDIObjType_BRUSH_TYPE))
#define PEN_AllocExtPenWithHandle() ((PBRUSH)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_EXTPEN))

#define PEN_FreeExtPen(pBMObj) GDIOBJ_FreeObj((POBJ) pBMObj, GDIObjType_BRUSH_TYPE)
#define PEN_FreeExtPenByHandle(hBMObj) GDIOBJ_FreeObjByHandle((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_EXTPEN)

//#define PEN_LockExtPen(hBMObj) ((PBRUSH)GDIOBJ_LockObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_EXTPEN))

#define PEN_UnlockPen(pPenObj) GDIOBJ_UnlockObjByPtr((POBJ)pPenObj)

#define  PEN_ShareUnlockPen(ppen) GDIOBJ_ShareUnlockObjByPtr((POBJ)ppen)


PBRUSH FASTCALL PEN_LockPen(HGDIOBJ);
PBRUSH FASTCALL PEN_ShareLockPen(HGDIOBJ);

INT APIENTRY PEN_GetObject(PBRUSH pPen, INT Count, PLOGPEN Buffer);

#endif
