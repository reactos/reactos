
#ifndef __WIN32K_PEN_H
#define __WIN32K_PEN_H

#include <win32k/gdiobj.h>

/* GDI logical pen object */
typedef struct
{
   LOGPEN    logpen;
} PENOBJ, *PPENOBJ;

/*  Internal interface  */

#define  PENOBJ_AllocPen()  \
  ((PPENOBJ) GDIOBJ_AllocObject (sizeof (PENOBJ), GO_PEN_MAGIC))
#define  PENOBJ_FreePen(hBMObj)  GDIOBJ_FreeObject((HGDIOBJ) hBMObj)
#define  PENOBJ_HandleToPtr(hBMObj)  \
  ((PPENOBJ) GDIOBJ_HandleToPtr ((HGDIOBJ) hBMObj, GO_PEN_MAGIC))
#define  PENOBJ_PtrToHandle(hBMObj)  \
  ((HPEN) GDIOBJ_PtrToHandle ((PGDIOBJ) hBMObj, GO_PEN_MAGIC))
#define  PENOBJ_LockPen(hBMObj) GDIOBJ_LockObject ((HGDIOBJ) hBMObj)
#define  PENOBJ_UnlockPen(hBMObj) GDIOBJ_UnlockObject ((HGDIOBJ) hBMObj)

HPEN STDCALL W32kCreatePen(INT  PenStyle,
                    INT  Width,
                    COLORREF  Color);

HPEN STDCALL W32kCreatePenIndirect(CONST PLOGPEN  lgpn);

HPEN STDCALL W32kExtCreatePen(DWORD  PenStyle,
                       DWORD  Width,
                       CONST PLOGBRUSH  lb,
                       DWORD  StyleCount,
                       CONST PDWORD  Style);

#endif

