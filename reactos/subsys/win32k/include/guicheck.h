#ifndef _WIN32K_GUICHECK_H
#define _WIN32K_GUICHECK_H

BOOL FASTCALL IntGraphicsCheck(BOOL Create);
BOOL FASTCALL IntCreatePrimarySurface();
VOID FASTCALL IntDestroyPrimarySurface();

#endif /* _WIN32K_GUICHECK_H */

/* EOF */
