#ifndef _WIN32K_GUICHECK_H
#define _WIN32K_GUICHECK_H

#include <windows.h>
#include <ddk/ntddk.h>

BOOL FASTCALL IntGraphicsCheck(BOOL Create);
BOOL FASTCALL IntCreatePrimarySurface();
VOID FASTCALL IntDestroyPrimarySurface();

#endif /* _WIN32K_GUICHECK_H */

/* EOF */
