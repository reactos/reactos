/*
 * Windows hook definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_HOOK_H
#define __WINE_HOOK_H

#include <windows.h>

#define HOOK_WIN32A	0x01
#define HOOK_WIN32W	0x02
#define HOOK_INUSE	0x80


/* hook type mask */
#define HOOK_MAPTYPE (HOOK_WIN32A | HOOK_WIN32W)

HOOKPROC HOOK_GetProc( HHOOK hhook );
WINBOOL HOOK_IsHooked( INT id );

LRESULT HOOK_CallHooksA( INT id, INT code, WPARAM wParam,
				  LPARAM lParam );
LRESULT HOOK_CallHooksW( INT id, INT code, WPARAM wParam,
				  LPARAM lParam );
void HOOK_FreeModuleHooks( HMODULE hModule );
void HOOK_FreeQueueHooks( HQUEUE hQueue );
void HOOK_ResetQueueHooks( HQUEUE hQueue );
HOOKPROC HOOK_GetProc( HHOOK hook );

#endif  /* __WINE_HOOK_H */
