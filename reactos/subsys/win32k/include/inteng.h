#ifndef __WIN32K_INTENG_H
#define __WIN32K_INTENG_H

/* Definitions of IntEngXxx functions */

extern BOOL STDCALL IntEngLineTo(SURFOBJ *Surface,
                                 CLIPOBJ *Clip,
                                 BRUSHOBJ *Brush,
                                 LONG x1,
                                 LONG y1,
                                 LONG x2,
                                 LONG y2,
                                 RECTL *RectBounds,
                                 MIX mix);

#endif
