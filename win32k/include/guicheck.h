#ifndef _WIN32K_GUICHECK_H
#define _WIN32K_GUICHECK_H

BOOL FASTCALL IntCreatePrimarySurface();
VOID FASTCALL IntDestroyPrimarySurface();

NTSTATUS FASTCALL InitGuiCheckImpl (VOID);

BOOL FASTCALL
UserGraphicsCheck(BOOL Create);

#endif /* _WIN32K_GUICHECK_H */

/* EOF */
