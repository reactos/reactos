#ifndef __WIN32K_MOUSE_H
#define __WIN32K_MOUSE_H

#include "../eng/misc.h"
#include <include/winsta.h>
//#include <ddk/ntddmou.h>

BOOL FASTCALL IntCheckClipCursor(LONG *x, LONG *y, PSYSTEM_CURSORINFO CurInfo);
BOOL FASTCALL IntSwapMouseButton(PWINSTATION_OBJECT WinStaObject, BOOL Swap);
INT  STDCALL  MouseSafetyOnDrawStart(SURFOBJ *SurfObj, SURFGDI *SurfGDI, LONG HazardX1, LONG HazardY1, LONG HazardX2, LONG HazardY2);
INT  FASTCALL MouseSafetyOnDrawEnd(SURFOBJ *SurfObj, SURFGDI *SurfGDI);
BOOL FASTCALL MouseMoveCursor(LONG X, LONG Y);
VOID FASTCALL EnableMouse(HDC hDisplayDC);
VOID          MouseGDICallBack(PMOUSE_INPUT_DATA Data, ULONG InputCount);


#endif /* __WIN32K_MOUSE_H */
