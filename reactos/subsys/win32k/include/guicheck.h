#ifndef _WIN32K_GUICHECK_H
#define _WIN32K_GUICHECK_H

#include <windows.h>
#include <ddk/ntddk.h>

BOOL FASTCALL IntGraphicsCheck(BOOL Create);
BOOL FASTCALL IntCreatePrimarySurface();
VOID FASTCALL IntDestroyPrimarySurface();

NTSTATUS FASTCALL InitGuiCheckImpl (VOID);
BOOL FASTCALL IntIsGUIActive(VOID);

#endif /* _WIN32K_GUICHECK_H */

/* EOF */
