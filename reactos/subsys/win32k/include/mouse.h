#ifndef __WIN32K_MOUSE_H
#define __WIN32K_MOUSE_H

#include "../eng/objects.h"

INT MouseSafetyOnDrawStart(SURFOBJ *SurfObj, SURFGDI *SurfGDI, LONG HazardX1, LONG HazardY1, LONG HazardX2, LONG HazardY2);
INT MouseSafetyOnDrawEnd(SURFOBJ *SurfObj, SURFGDI *SurfGDI);

#endif /* __WIN32K_MOUSE_H */
