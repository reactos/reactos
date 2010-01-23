#ifndef _WIN32K_GUICHECK_H
#define _WIN32K_GUICHECK_H

BOOL FASTCALL co_IntGraphicsCheck(BOOL Create);
BOOL FASTCALL IntCreatePrimarySurface(VOID);
VOID FASTCALL IntDestroyPrimarySurface(VOID);

NTSTATUS FASTCALL InitGuiCheckImpl (VOID);

#endif /* _WIN32K_GUICHECK_H */

/* EOF */
