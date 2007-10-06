#ifndef __WIN32K_PEN_H
#define __WIN32K_PEN_H

#include "gdiobj.h"
#include "brush.h"

/* Internal interface */

#define PENOBJ_AllocPen() ((HPEN)GDIOBJ_AllocObj(GdiHandleTable, GDI_OBJECT_TYPE_PEN))
#define PENOBJ_FreePen(hBMObj) GDIOBJ_FreeObj(GdiHandleTable, (HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_PEN)
#define PENOBJ_LockPen(hBMObj) ((PGDIBRUSHOBJ)GDIOBJ_LockObj(GdiHandleTable, (HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_PEN))
#define PENOBJ_UnlockPen(pPenObj) GDIOBJ_UnlockObjByPtr(GdiHandleTable, pPenObj)

#endif
