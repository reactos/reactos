#ifndef __WIN32K_MOUSE_H
#define __WIN32K_MOUSE_H

INT MouseSafetyOnDrawStart(PSURFOBJ SurfObj, PSURFGDI SurfGDI, LONG HazardX1, LONG HazardY1, LONG HazardX2, LONG HazardY2);
INT MouseSafetyOnDrawEnd(PSURFOBJ SurfObj, PSURFGDI SurfGDI);
VOID EnableMouse(HDC hDisplayDC);

#endif /* __WIN32K_MOUSE_H */
