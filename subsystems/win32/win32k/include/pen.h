#ifndef __WIN32K_PEN_H
#define __WIN32K_PEN_H

#include "gdiobj.h"
#include "brush.h"

/* Internal interface */

#define PENOBJ_AllocPen() ((HPEN)GDIOBJ_AllocObj(GDIObjType_BRUSH_TYPE))
#define PENOBJ_AllocPenWithHandle() ((PGDIBRUSHOBJ)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_PEN))

#define PENOBJ_FreePen(pBMObj) GDIOBJ_FreeObj((POBJ) pBMObj, GDIObjType_BRUSH_TYPE)
#define PENOBJ_FreePenByHandle(hBMObj) GDIOBJ_FreeObjByHandle((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_PEN)

//#define PENOBJ_LockPen(hBMObj) ((PGDIBRUSHOBJ)GDIOBJ_LockObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_PEN))

#define PENOBJ_AllocExtPen() ((PGDIBRUSHOBJ)GDIOBJ_AllocObj(GDIObjType_BRUSH_TYPE))
#define PENOBJ_AllocExtPenWithHandle() ((PGDIBRUSHOBJ)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_EXTPEN))

#define PENOBJ_FreeExtPen(pBMObj) GDIOBJ_FreeObj((POBJ) pBMObj, GDIObjType_BRUSH_TYPE)
#define PENOBJ_FreeExtPenByHandle(hBMObj) GDIOBJ_FreeObjByHandle((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_EXTPEN)

//#define PENOBJ_LockExtPen(hBMObj) ((PGDIBRUSHOBJ)GDIOBJ_LockObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_EXTPEN))

#define PENOBJ_UnlockPen(pPenObj) GDIOBJ_UnlockObjByPtr((POBJ)pPenObj)


PGDIBRUSHOBJ FASTCALL PENOBJ_LockPen(HGDIOBJ);
INT APIENTRY PEN_GetObject(PGDIBRUSHOBJ hPen, INT Count, PLOGPEN Buffer);

#endif
