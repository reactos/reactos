#ifndef _WIN32K_HOOK_H
#define _WIN32K_HOOK_H

#include <windows.h>
#include <internal/ps.h>

typedef struct tagHOOK
{
  LIST_ENTRY Chain;          /* Hook chain entry */
  HHOOK      Self;           /* user handle for this hook */
  PETHREAD   Thread;         /* Thread owning the hook */
  int        HookId;         /* Hook table index */
  HOOKPROC   Proc;           /* Hook function */
  BOOLEAN    Ansi;           /* Is it an Ansi hook? */
  UNICODE_STRING ModuleName; /* Module name for global hooks */
} HOOK, *PHOOK;

#define NB_HOOKS (WH_MAXHOOK-WH_MINHOOK+1)

typedef struct tagHOOKTABLE
{
  FAST_MUTEX Lock;
  LIST_ENTRY Hooks[NB_HOOKS];  /* array of hook chains */
  UINT       Counts[NB_HOOKS]; /* use counts for each hook chain */
} HOOKTABLE, *PHOOKTABLE;

LRESULT FASTCALL HOOK_CallHooks(INT HookId, INT Code, WPARAM wParam, LPARAM lParam);
VOID FASTCALL HOOK_DestroyThreadHooks(PETHREAD Thread);

#define IntLockHookTable(HookTable) \
  ExAcquireFastMutex(&HookTable->Lock)

#define IntUnLockHookTable(HookTable) \
  ExReleaseFastMutex(&HookTable->Lock)

#endif /* _WIN32K_HOOK_H */

/* EOF */
