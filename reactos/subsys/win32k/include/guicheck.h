#ifndef _WIN32K_GUICHECK_H
#define _WIN32K_GUICHECK_H

#include <windows.h>
#include <ddk/ntddk.h>

BOOL INTERNAL_CALL IntGraphicsCheck(BOOL Create);
BOOL INTERNAL_CALL IntCreatePrimarySurface();
VOID INTERNAL_CALL IntDestroyPrimarySurface();

#endif /* _WIN32K_GUICHECK_H */

/* EOF */
