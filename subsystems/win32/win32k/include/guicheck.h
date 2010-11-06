#pragma once

BOOL FASTCALL co_IntGraphicsCheck(BOOL Create);
BOOL FASTCALL IntCreatePrimarySurface(VOID);
VOID FASTCALL IntDestroyPrimarySurface(VOID);

INIT_FUNCTION
NTSTATUS
NTAPI
InitGuiCheckImpl (VOID);

/* EOF */
