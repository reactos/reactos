
#ifndef __WIN32K_PEN_H
#define __WIN32K_PEN_H

#include <win32k/gdiobj.h>

/* GDI logical pen object */
typedef struct
{
   LOGPEN logpen;
   ULONG  iSolidColor;
} PENOBJ, *PPENOBJ;

/*  Internal interface  */

#define  PENOBJ_AllocPen()  \
  ((HPEN) GDIOBJ_AllocObj (sizeof (PENOBJ), GDI_OBJECT_TYPE_PEN, NULL))
#define  PENOBJ_FreePen(hBMObj)  GDIOBJ_FreeObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_PEN, GDIOBJFLAG_DEFAULT)
#define  PENOBJ_LockPen(hBMObj) ((PPENOBJ)GDIOBJ_LockObj ((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_PEN))
#define  PENOBJ_UnlockPen(hBMObj) GDIOBJ_UnlockObj ((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_PEN)

HPEN STDCALL NtGdiCreatePen(INT  PenStyle,
                    INT  Width,
                    COLORREF  Color);

HPEN STDCALL NtGdiCreatePenIndirect(CONST PLOGPEN  lgpn);

HPEN STDCALL NtGdiExtCreatePen(DWORD  PenStyle,
                       DWORD  Width,
                       CONST PLOGBRUSH  lb,
                       DWORD  StyleCount,
                       CONST PDWORD  Style);

#endif

