#pragma once

#define HOOK_THREAD_REFERENCED	(0x1)
#define NB_HOOKS (WH_MAXHOOK-WH_MINHOOK+1)
#define HOOKID_TO_INDEX(HookId) (HookId - WH_MINHOOK)
#define HOOKID_TO_FLAG(HookId) (1 << ((HookId) + 1))
#define ISITHOOKED(HookId) (((PTHREADINFO)PsGetCurrentThreadWin32Thread())->fsHooks & HOOKID_TO_FLAG(HookId))

typedef struct tagHOOKTABLE
{
  LIST_ENTRY Hooks[NB_HOOKS];  /* array of hook chains */
  UINT       Counts[NB_HOOKS]; /* use counts for each hook chain */
} HOOKTABLE, *PHOOKTABLE;

typedef struct tagEVENTHOOK
{
  THROBJHEAD     head;
  LIST_ENTRY     Chain;      /* Event chain entry */
  UINT           eventMin;
  UINT           eventMax; 
  DWORD          idProcess;
  DWORD          idThread;
  WINEVENTPROC   Proc;       /* Event function */
  ULONG          Flags;      /* Some internal flags */
  ULONG_PTR      offPfn;
  INT            ihmod;
} EVENTHOOK, *PEVENTHOOK;

typedef struct tagEVENTTABLE
{
  LIST_ENTRY Events;
  UINT       Counts;
} EVENTTABLE, *PEVENTTABLE;

LRESULT FASTCALL co_HOOK_CallHooks(INT HookId, INT Code, WPARAM wParam, LPARAM lParam);
LRESULT FASTCALL co_EVENT_CallEvents(DWORD, HWND, UINT_PTR, LONG_PTR);
VOID FASTCALL HOOK_DestroyThreadHooks(PETHREAD Thread);
PHOOK FASTCALL IntGetHookObject(HHOOK);
PHOOK FASTCALL IntGetNextHook(PHOOK Hook);
LRESULT FASTCALL UserCallNextHookEx( PHOOK pHook, int Code, WPARAM wParam, LPARAM lParam, BOOL Ansi);

/* EOF */
