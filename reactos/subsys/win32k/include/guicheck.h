#ifndef _WIN32K_GUICHECK_H
#define _WIN32K_GUICHECK_H

#include <windows.h>
#include <ddk/ntddk.h>

VOID FASTCALL
IntGraphicsCheck(BOOL Create);

#endif /* _WIN32K_GUICHECK_H */

/* EOF */
