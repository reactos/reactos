
#ifndef __WIN32K_PEN_H
#define __WIN32K_PEN_H

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

