#ifndef _WIN32K_MOUSE_H
#define _WIN32K_MOUSE_H

#include "../eng/misc.h"
#include <include/winsta.h>
//#include <ddk/ntddmou.h>

BOOL FASTCALL IntCheckClipCursor(LONG *x, LONG *y, PSYSTEM_CURSORINFO CurInfo);
BOOL FASTCALL IntSwapMouseButton(PWINSTATION_OBJECT WinStaObject, BOOL Swap);
INT  FASTCALL MouseSafetyOnDrawStart(SURFOBJ *SurfObj, SURFGDI *SurfGDI, LONG HazardX1, LONG HazardY1, LONG HazardX2, LONG HazardY2);
INT  FASTCALL MouseSafetyOnDrawEnd(SURFOBJ *SurfObj, SURFGDI *SurfGDI);
BOOL FASTCALL MouseMoveCursor(LONG X, LONG Y);
VOID FASTCALL EnableMouse(HDC hDisplayDC);
VOID          MouseGDICallBack(PMOUSE_INPUT_DATA Data, ULONG InputCount);
VOID FASTCALL SetPointerRect(PSYSTEM_CURSORINFO CurInfo, PRECTL PointerRect);

#ifndef XBUTTON1
#define XBUTTON1	(0x01)
#endif
#ifndef XBUTTON2
#define XBUTTON2	(0x02)
#endif
#ifndef MOUSEEVENTF_XDOWN
#define MOUSEEVENTF_XDOWN	(0x80)
#endif
#ifndef MOUSEEVENTF_XUP
#define MOUSEEVENTF_XUP	(0x100)
#endif

#endif /* _WIN32K_MOUSE_H */
