/*
 * Windows hook definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_HOOK_H
#define __WINE_HOOK_H

#include <windows.h>

#define HOOK_WINA	0x01
#define HOOK_WINW	0x02
#define HOOK_WIN32A	0x01
#define HOOK_WIN32W	0x02
#define HOOK_INUSE	0x80

#define HQUEUE HANDLE
/* hook type mask */
#define HOOK_MAPTYPE (HOOK_WIN32A | HOOK_WIN32W)


#define HOOK_MAGIC  ((int)'H' | (int)'K' << 8)  /* 'HK' */

/***** Window hooks *****/

  /* Hook values */
#define WH_MIN		    (-1)
#define WH_MAX              12

#define WH_MINHOOK          WH_MIN
#define WH_MAXHOOK          WH_MAX
#define WH_NB_HOOKS         (WH_MAXHOOK-WH_MINHOOK+1)


  /* Hook data (pointed to by a HHOOK) */
typedef struct
{
    HANDLE   next;               /* 00 Next hook in chain */
    HOOKPROC proc;               /* 02 Hook procedure */
    INT      id;                 /* 06 Hook id (WH_xxx) */
    HQUEUE   ownerQueue;         /* 08 Owner queue (0 for system hook) */
    HMODULE  ownerModule;        /* 0a Owner module */
    WORD     flags;              /* 0c flags */
} HOOKDATA;

HANDLE HOOK_SetHook( INT id, LPVOID proc, INT type,
	 HINSTANCE hInst, DWORD dwThreadId );

WINBOOL HOOK_RemoveHook( HANDLE hook );

WINBOOL HOOK_IsHooked( INT id );

LRESULT HOOK_CallHooks( INT id, INT code, WPARAM wParam,
                           LPARAM lParam ,WINBOOL bUnicode);

LRESULT HOOK_CallHooksA( INT id, INT code, WPARAM wParam,
				  LPARAM lParam );
LRESULT HOOK_CallHooksW( INT id, INT code, WPARAM wParam,
				  LPARAM lParam );

LRESULT HOOK_CallHook( HHOOK hook, INT fromtype, INT code,
                              WPARAM wParam, LPARAM lParam );

HANDLE HOOK_GetNextHook( HHOOK hook );

void HOOK_FreeModuleHooks( HMODULE hModule );
void HOOK_FreeQueueHooks( HQUEUE hQueue );
void HOOK_ResetQueueHooks( HQUEUE hQueue );
HOOKPROC HOOK_GetProc( HHOOK hook );

#endif  /* __WINE_HOOK_H */
