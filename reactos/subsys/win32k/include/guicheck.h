#ifndef __WIN32K_GUICHECK_H
#define __WIN32K_GUICHECK_H

#include <windows.h>
#include <ddk/ntddk.h>

VOID FASTCALL
IntGraphicsCheck(BOOL Create);

#endif /* __WIN32K_GUICHECK_H */

/* EOF */
