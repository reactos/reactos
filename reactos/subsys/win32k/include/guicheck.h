#ifndef __WIN32K_GUICHECK_H
#define __WIN32K_GUICHECK_H

#include <windows.h>
#include <ddk/ntddk.h>

VOID FASTCALL
W32kGuiCheck(VOID);
VOID FASTCALL
W32kGraphicsCheck(BOOL Create);

#endif /* __WIN32K_GUICHECK_H */

/* EOF */
