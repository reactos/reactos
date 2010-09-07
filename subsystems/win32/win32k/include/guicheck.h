#pragma once

BOOL FASTCALL co_IntGraphicsCheck(BOOL Create);
BOOL FASTCALL IntCreatePrimarySurface(VOID);
VOID FASTCALL IntDestroyPrimarySurface(VOID);

NTSTATUS FASTCALL InitGuiCheckImpl (VOID);

/* EOF */
