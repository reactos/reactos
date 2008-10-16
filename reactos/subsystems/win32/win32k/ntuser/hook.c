/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window hooks
 * FILE:             subsystem/win32/win32k/ntuser/hook.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 * NOTE:             Most of this code was adapted from Wine,
 *                   Copyright (C) 2002 Alexandre Julliard
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

static PHOOKTABLE GlobalHooks;


/* PRIVATE FUNCTIONS *********************************************************/


/* create a new hook table */
static PHOOKTABLE
IntAllocHookTable(void)
{
   PHOOKTABLE Table;
   UINT i;

   Table = ExAllocatePoolWithTag(PagedPool, sizeof(HOOKTABLE), TAG_HOOK);
   if (NULL != Table)
   {
      for (i = 0; i < NB_HOOKS; i++)
      {
         InitializeListHead(&Table->Hooks[i]);
         Table->Counts[i] = 0;
      }
   }

   return Table;
}


PHOOK FASTCALL IntGetHookObject(HHOOK hHook)
{
   PHOOK Hook;

   if (!hHook)
   {
      SetLastWin32Error(ERROR_INVALID_HOOK_HANDLE);
      return NULL;
   }

   Hook = (PHOOK)UserGetObject(gHandleTable, hHook, otHook);
   if (!Hook)
   {
      SetLastWin32Error(ERROR_INVALID_HOOK_HANDLE);
      return NULL;
   }

   ASSERT(USER_BODY_TO_HEADER(Hook)->RefCount >= 0);

   USER_BODY_TO_HEADER(Hook)->RefCount++;

   return Hook;
}



/* create a new hook and add it to the specified table */
static PHOOK
IntAddHook(PETHREAD Thread, int HookId, BOOLEAN Global, PWINSTATION_OBJECT WinStaObj)
{
   PTHREADINFO W32Thread;
   PHOOK Hook;
   PHOOKTABLE Table = Global ? GlobalHooks : MsqGetHooks(((PTHREADINFO)Thread->Tcb.Win32Thread)->MessageQueue);
   HANDLE Handle;

   if (NULL == Table)
   {
      Table = IntAllocHookTable();
      if (NULL == Table)
      {
         return NULL;
      }
      if (Global)
      {
         GlobalHooks = Table;
      }
      else
      {
         MsqSetHooks(((PTHREADINFO)Thread->Tcb.Win32Thread)->MessageQueue, Table);
      }
   }

   Hook = UserCreateObject(gHandleTable, &Handle, otHook, sizeof(HOOK));
   if (NULL == Hook)
   {
      return NULL;
   }

   Hook->Self = Handle;
   Hook->Thread = Thread;
   Hook->HookId = HookId;

   if (Thread)
   {
      W32Thread = ((PTHREADINFO)Thread->Tcb.Win32Thread);
      ASSERT(W32Thread != NULL);
      W32Thread->Hooks |= HOOKID_TO_FLAG(HookId);
      if (W32Thread->ThreadInfo != NULL)
          W32Thread->ThreadInfo->Hooks = W32Thread->Hooks;
   }

   RtlInitUnicodeString(&Hook->ModuleName, NULL);

   InsertHeadList(&Table->Hooks[HOOKID_TO_INDEX(HookId)], &Hook->Chain);

   return Hook;
}

/* get the hook table that a given hook belongs to */
static PHOOKTABLE FASTCALL
IntGetTable(PHOOK Hook)
{
   if (NULL == Hook->Thread || WH_KEYBOARD_LL == Hook->HookId ||
         WH_MOUSE_LL == Hook->HookId)
   {
      return GlobalHooks;
   }

   return MsqGetHooks(((PTHREADINFO)Hook->Thread->Tcb.Win32Thread)->MessageQueue);
}

/* get the first hook in the chain */
static PHOOK FASTCALL
IntGetFirstHook(PHOOKTABLE Table, int HookId)
{
   PLIST_ENTRY Elem = Table->Hooks[HOOKID_TO_INDEX(HookId)].Flink;
   return Elem == &Table->Hooks[HOOKID_TO_INDEX(HookId)]
          ? NULL : CONTAINING_RECORD(Elem, HOOK, Chain);
}

/* find the first non-deleted hook in the chain */
static PHOOK FASTCALL
IntGetFirstValidHook(PHOOKTABLE Table, int HookId)
{
   PHOOK Hook;
   PLIST_ENTRY Elem;

   Hook = IntGetFirstHook(Table, HookId);
   while (NULL != Hook && NULL == Hook->Proc)
   {
      Elem = Hook->Chain.Flink;
      Hook = (Elem == &Table->Hooks[HOOKID_TO_INDEX(HookId)]
              ? NULL : CONTAINING_RECORD(Elem, HOOK, Chain));
   }

   return Hook;
}

/* find the next hook in the chain, skipping the deleted ones */
PHOOK
FASTCALL
IntGetNextHook(PHOOK Hook)
{
   PHOOKTABLE Table = IntGetTable(Hook);
   int HookId = Hook->HookId;
   PLIST_ENTRY Elem;

   Elem = Hook->Chain.Flink;
   while (Elem != &Table->Hooks[HOOKID_TO_INDEX(HookId)])
   {
      Hook = CONTAINING_RECORD(Elem, HOOK, Chain);
      if (NULL != Hook->Proc)
      {
         return Hook;
      }
   }

   if (NULL != GlobalHooks && Table != GlobalHooks)  /* now search through the global table */
   {
      return IntGetFirstValidHook(GlobalHooks, HookId);
   }

   return NULL;
}

/* free a hook, removing it from its chain */
static VOID FASTCALL
IntFreeHook(PHOOKTABLE Table, PHOOK Hook, PWINSTATION_OBJECT WinStaObj)
{
   RemoveEntryList(&Hook->Chain);
   RtlFreeUnicodeString(&Hook->ModuleName);

   /* Dereference thread if required */
   if (Hook->Flags & HOOK_THREAD_REFERENCED)
   {
      ObDereferenceObject(Hook->Thread);
   }

   /* Close handle */
   UserDeleteObject(Hook->Self, otHook);
}

/* remove a hook, freeing it if the chain is not in use */
static VOID
IntRemoveHook(PHOOK Hook, PWINSTATION_OBJECT WinStaObj, BOOL TableAlreadyLocked)
{
   PTHREADINFO W32Thread;
   PHOOKTABLE Table = IntGetTable(Hook);

   ASSERT(NULL != Table);
   if (NULL == Table)
   {
      return;
   }

   W32Thread = ((PTHREADINFO)Hook->Thread->Tcb.Win32Thread);
   ASSERT(W32Thread != NULL);
   W32Thread->Hooks &= ~HOOKID_TO_FLAG(Hook->HookId);
   if (W32Thread->ThreadInfo != NULL)
       W32Thread->ThreadInfo->Hooks = W32Thread->Hooks;

   if (0 != Table->Counts[HOOKID_TO_INDEX(Hook->HookId)])
   {
      Hook->Proc = NULL; /* chain is in use, just mark it and return */
   }
   else
   {
      IntFreeHook(Table, Hook, WinStaObj);
   }
}

/* release a hook chain, removing deleted hooks if the use count drops to 0 */
static VOID FASTCALL
IntReleaseHookChain(PHOOKTABLE Table, int HookId, PWINSTATION_OBJECT WinStaObj)
{
   PLIST_ENTRY Elem;
   PHOOK HookObj;

   if (NULL == Table)
   {
      return;
   }

   /* use count shouldn't already be 0 */
   ASSERT(0 != Table->Counts[HOOKID_TO_INDEX(HookId)]);
   if (0 == Table->Counts[HOOKID_TO_INDEX(HookId)])
   {
      return;
   }
   if (0 == --Table->Counts[HOOKID_TO_INDEX(HookId)])
   {
      Elem = Table->Hooks[HOOKID_TO_INDEX(HookId)].Flink;
      while (Elem != &Table->Hooks[HOOKID_TO_INDEX(HookId)])
      {
         HookObj = CONTAINING_RECORD(Elem, HOOK, Chain);
         Elem = Elem->Flink;
         if (NULL == HookObj->Proc)
         {
            IntFreeHook(Table, HookObj, WinStaObj);
         }
      }
   }
}

static LRESULT FASTCALL
IntCallLowLevelHook(PHOOK Hook, INT Code, WPARAM wParam, LPARAM lParam)
{
   NTSTATUS Status;
   ULONG_PTR uResult;

   /* FIXME should get timeout from
    * HKEY_CURRENT_USER\Control Panel\Desktop\LowLevelHooksTimeout */
   Status = co_MsqSendMessage(((PTHREADINFO)Hook->Thread->Tcb.Win32Thread)->MessageQueue,
                                    (HWND) Code,
                                   Hook->HookId,
                                         wParam,
                                         lParam,
                                           5000,
                                           TRUE,
                                     MSQ_ISHOOK,
                                       &uResult);

   return NT_SUCCESS(Status) ? uResult : 0;
}

/*
  Called from inside kernel space.
 */
LRESULT
FASTCALL
co_HOOK_CallHooks(INT HookId, INT Code, WPARAM wParam, LPARAM lParam)
{
   PHOOK Hook, SaveHook;
   PTHREADINFO pti;
   PCLIENTINFO ClientInfo;
   PHOOKTABLE Table;
   LRESULT Result;
   PWINSTATION_OBJECT WinStaObj;
   NTSTATUS Status;

   ASSERT(WH_MINHOOK <= HookId && HookId <= WH_MAXHOOK);

   pti = PsGetCurrentThreadWin32Thread();
   if (!pti)
   {
      Table = NULL;
   }
   else
   {
      Table = MsqGetHooks(pti->MessageQueue);
   }

   if (NULL == Table || ! (Hook = IntGetFirstValidHook(Table, HookId)))
   {
      /* try global table */
      Table = GlobalHooks;
      if (NULL == Table || ! (Hook = IntGetFirstValidHook(Table, HookId)))
      {
         return 0;  /* no hook set */
      }
   }

   if ((Hook->Thread != PsGetCurrentThread()) && (Hook->Thread != NULL))
   {
      // Post it in message queue.
      return IntCallLowLevelHook(Hook, Code, wParam, lParam);
   }

   Table->Counts[HOOKID_TO_INDEX(HookId)]++;
   if (Table != GlobalHooks && GlobalHooks != NULL)
   {
      GlobalHooks->Counts[HOOKID_TO_INDEX(HookId)]++;
   }

   ClientInfo = GetWin32ClientInfo();
   SaveHook = ClientInfo->phkCurrent;
   ClientInfo->phkCurrent = Hook;     // Load the call.

   Result = co_IntCallHookProc( HookId,
                                  Code,
                                wParam,
                                lParam,
                            Hook->Proc,
                            Hook->Ansi,
                     &Hook->ModuleName);

   ClientInfo->phkCurrent = SaveHook;

   Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                           KernelMode,
                                           0,
                                           &WinStaObj);

   if (! NT_SUCCESS(Status))
   {
      DPRINT1("Invalid window station????\n");
   }
   else
   {
      IntReleaseHookChain(MsqGetHooks(pti->MessageQueue), HookId, WinStaObj);
      IntReleaseHookChain(GlobalHooks, HookId, WinStaObj);
      ObDereferenceObject(WinStaObj);
   }

   return Result;
}

VOID FASTCALL
HOOK_DestroyThreadHooks(PETHREAD Thread)
{
   int HookId;
   PLIST_ENTRY Elem;
   PHOOK HookObj;
   PWINSTATION_OBJECT WinStaObj;
   NTSTATUS Status;

   if (NULL != GlobalHooks)
   {
      Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                              KernelMode,
                                              0,
                                              &WinStaObj);

      if (! NT_SUCCESS(Status))
      {
         DPRINT1("Invalid window station????\n");
         return;
      }

      for (HookId = WH_MINHOOK; HookId <= WH_MAXHOOK; HookId++)
      {
         /* only low-level keyboard/mouse global hooks can be owned by a thread */
         switch(HookId)
         {
            case WH_KEYBOARD_LL:
            case WH_MOUSE_LL:
               Elem = GlobalHooks->Hooks[HOOKID_TO_INDEX(HookId)].Flink;
               while (Elem != &GlobalHooks->Hooks[HOOKID_TO_INDEX(HookId)])
               {
                  HookObj = CONTAINING_RECORD(Elem, HOOK, Chain);
                  Elem = Elem->Flink;
                  if (HookObj->Thread == Thread)
                  {
                     IntRemoveHook(HookObj, WinStaObj, TRUE);
                  }
               }
               break;
         }
      }
   }
}

static LRESULT
FASTCALL
co_HOOK_CallHookNext(PHOOK Hook, INT Code, WPARAM wParam, LPARAM lParam)
{
   if ((Hook->Thread != PsGetCurrentThread()) && (Hook->Thread != NULL))
   {
      DPRINT1("CALLING HOOK from another Thread. %d\n",Hook->HookId);
      return IntCallLowLevelHook(Hook, Code, wParam, lParam);
   }
   DPRINT("CALLING HOOK %d\n",Hook->HookId);
   return co_IntCallHookProc(Hook->HookId,
                                     Code,
                                   wParam,
                                   lParam,
                               Hook->Proc,
                               Hook->Ansi,
                        &Hook->ModuleName);
}


LRESULT
FASTCALL
IntCallDebugHook(
   PHOOK Hook,
   int Code,
   WPARAM wParam,
   LPARAM lParam)
{
   LRESULT lResult = 0;
   ULONG Size;
   DEBUGHOOKINFO Debug;
   PVOID HooklParam = NULL;
   BOOL BadChk = FALSE;

   if (lParam)
   {
      _SEH_TRY
      {
          ProbeForRead((PVOID)lParam,
                       sizeof(DEBUGHOOKINFO),
                                   1);
          RtlCopyMemory( &Debug,
                  (PVOID)lParam,
                  sizeof(DEBUGHOOKINFO));
      }
      _SEH_HANDLE
      {
          BadChk = TRUE;
      }
      _SEH_END;
      if (BadChk)
      {
          DPRINT1("HOOK WH_DEBUG read from lParam ERROR!\n");
          return lResult;
      }
   }
   else
      return lResult; // Need lParam!

   switch (wParam)
   {
      case WH_CBT:
      {
         switch (Debug.code)
         {
            case HCBT_CLICKSKIPPED:
               Size = sizeof(MOUSEHOOKSTRUCTEX);
               break;
            case HCBT_MOVESIZE:
               Size = sizeof(RECT);
               break;
            case HCBT_ACTIVATE:
               Size = sizeof(CBTACTIVATESTRUCT); 
               break;
            case HCBT_CREATEWND: // Handle Ansi?
               Size = sizeof(CBT_CREATEWND);
               // What shall we do? Size += sizeof(CREATESTRUCTEX);
               break;
            default:
               Size = sizeof(LPARAM);
         }
      }
      break;

      case WH_MOUSE_LL:
         Size = sizeof(MSLLHOOKSTRUCT);
      break;

      case WH_KEYBOARD_LL:
         Size = sizeof(KBDLLHOOKSTRUCT);
      break;

      case WH_MSGFILTER:
      case WH_SYSMSGFILTER:
      case WH_GETMESSAGE:
         Size = sizeof(MSG);
      break;

      case WH_JOURNALPLAYBACK:
      case WH_JOURNALRECORD:
         Size = sizeof(EVENTMSG);
      break;

      case WH_FOREGROUNDIDLE:
      case WH_KEYBOARD:
      case WH_SHELL:
      default:
         Size = sizeof(LPARAM);
   }

   if (Size > sizeof(LPARAM))
      HooklParam = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_HOOK);

   if (HooklParam)
   {
      _SEH_TRY
      {
          ProbeForRead((PVOID)Debug.lParam,
                                      Size,
                                         1);
          RtlCopyMemory( HooklParam,
                (PVOID)Debug.lParam,
                               Size);
      }
      _SEH_HANDLE
      {
          BadChk = TRUE;
      }
      _SEH_END;
      if (BadChk)
      {
          DPRINT1("HOOK WH_DEBUG read from Debug.lParam ERROR!\n");
          ExFreePool(HooklParam);
          return lResult;
      }
   }

   if (HooklParam) Debug.lParam = (LPARAM)HooklParam;
   lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&Debug);
   if (HooklParam) ExFreePool(HooklParam);
   return lResult;
}

/*
   Called from user space via CallNextHook.
 */
LRESULT
FASTCALL
UserCallNextHookEx(
   PHOOK Hook,
   int Code,
   WPARAM wParam,
   LPARAM lParam,
   BOOL Ansi)
{
  LRESULT lResult = 0;
  BOOL BadChk = FALSE;

// Handle this one first.
  if ((Hook->HookId == WH_MOUSE) ||
      (Hook->HookId == WH_CBT && Code == HCBT_CLICKSKIPPED))
  {
     MOUSEHOOKSTRUCTEX Mouse;
     if (lParam)
     {
        _SEH_TRY
        {
           ProbeForRead((PVOID)lParam,
                        sizeof(MOUSEHOOKSTRUCTEX),
                                    1);
           RtlCopyMemory( &Mouse,
                   (PVOID)lParam,
                   sizeof(MOUSEHOOKSTRUCTEX));
        }
        _SEH_HANDLE
        {
           BadChk = TRUE;
        }
        _SEH_END;
        if (BadChk)
        {
            DPRINT1("HOOK WH_MOUSE read from lParam ERROR!\n");
        }
     }
     if (!BadChk)
     {
        lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&Mouse);
     }
     return lResult;
  }

  switch(Hook->HookId)
  {
      case WH_MOUSE_LL:
      {
         MSLLHOOKSTRUCT Mouse;
         if (lParam)
         {
            _SEH_TRY
            {
                ProbeForRead((PVOID)lParam,
                             sizeof(MSLLHOOKSTRUCT),
                                         1);
                RtlCopyMemory( &Mouse,
                        (PVOID)lParam,
                        sizeof(MSLLHOOKSTRUCT));
            }
            _SEH_HANDLE
            {
               BadChk = TRUE;
            }
            _SEH_END;
            if (BadChk)
            {
                DPRINT1("HOOK WH_MOUSE_LL read from lParam ERROR!\n");
            }
         }
         if (!BadChk)
         {
            lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&Mouse);
         }
         break;
      }

      case WH_KEYBOARD_LL:
      {
         KBDLLHOOKSTRUCT Keyboard;
         if (lParam)
         {
            _SEH_TRY
            {
                ProbeForRead((PVOID)lParam,
                             sizeof(KBDLLHOOKSTRUCT),
                                         1);
                RtlCopyMemory( &Keyboard,
                        (PVOID)lParam,
                        sizeof(KBDLLHOOKSTRUCT));
            }
            _SEH_HANDLE
            {
               BadChk = TRUE;
            }
            _SEH_END;
            if (BadChk)
            {
                DPRINT1("HOOK WH_KEYBORD_LL read from lParam ERROR!\n");
            }
         }
         if (!BadChk)
         {
            lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&Keyboard);
         }
         break;
      }

      case WH_MSGFILTER:
      case WH_SYSMSGFILTER:
      case WH_GETMESSAGE:
      {
         MSG Msg;
         if (lParam)
         {
            _SEH_TRY
            {
               ProbeForRead((PVOID)lParam,
                               sizeof(MSG),
                                         1);
               RtlCopyMemory( &Msg,
                     (PVOID)lParam,
                       sizeof(MSG));
            }
            _SEH_HANDLE
            {
              BadChk = TRUE;
            }
            _SEH_END;
            if (BadChk)
            {
               DPRINT1("HOOK WH_XMESSAGEX read from lParam ERROR!\n");
            }
         }
         if (!BadChk)
         {
            lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&Msg);
            if (lParam && (Hook->HookId == WH_GETMESSAGE))
            {
               _SEH_TRY
               {
                  ProbeForWrite((PVOID)lParam,
                                  sizeof(MSG),
                                            1);
                  RtlCopyMemory((PVOID)lParam,
                                         &Msg,
                                  sizeof(MSG));
               }
               _SEH_HANDLE
               {
                 BadChk = TRUE;
               }
               _SEH_END;
               if (BadChk)
               {
                  DPRINT1("HOOK WH_GETMESSAGE write to lParam ERROR!\n");
               }
            }
         }
         break;
      }

      case WH_CBT:
         DPRINT1("HOOK WH_CBT!\n");
         switch (Code)
         {
            case HCBT_CREATEWND: // Use Ansi.
               DPRINT1("HOOK HCBT_CREATEWND\n");
//               lResult = co_HOOK_CallHookNext(Hook, Code, wParam, lParam);
               break;

            case HCBT_MOVESIZE:
            {
               RECT rt;
               DPRINT1("HOOK HCBT_MOVESIZE\n");
               if (lParam)
               {
                  _SEH_TRY
                  {
                      ProbeForRead((PVOID)lParam,
                                    sizeof(RECT),
                                              1);
                      RtlCopyMemory( &rt,
                           (PVOID)lParam,
                            sizeof(RECT));
                  }
                  _SEH_HANDLE
                  {
                     BadChk = TRUE;
                  }
                  _SEH_END;
                  if (BadChk)
                  {
                      DPRINT1("HOOK HCBT_MOVESIZE read from lParam ERROR!\n");
                   }
               }
               if (!BadChk)
               {
                   lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&rt);
               }
               break;
            }

            case HCBT_ACTIVATE:
            {
               CBTACTIVATESTRUCT CbAs;
               DPRINT1("HOOK HCBT_ACTIVATE\n");
               if (lParam)
               {
                  _SEH_TRY
                  {
                      ProbeForRead((PVOID)lParam,
                                   sizeof(CBTACTIVATESTRUCT),
                                              1);
                      RtlCopyMemory( &CbAs,
                             (PVOID)lParam,
                             sizeof(CBTACTIVATESTRUCT));
                  }
                  _SEH_HANDLE
                  {
                     BadChk = TRUE;
                  }
                  _SEH_END;
                  if (BadChk)
                  {
                      DPRINT1("HOOK HCBT_ACTIVATE read from lParam ERROR!\n");
                   }
               }
               if (!BadChk)
               {
                   lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)&CbAs);
               }
               break;
            }
            /*
                The rest just use default.
             */
            default:
               DPRINT1("HOOK HCBT_ %d\n",Code);
               lResult = co_HOOK_CallHookNext(Hook, Code, wParam, lParam);
               break;
         }
         break;

      case WH_JOURNALPLAYBACK:
      case WH_JOURNALRECORD:
      {
         EVENTMSG EventMsg;
         if (lParam)
         {
            _SEH_TRY
            {
                ProbeForRead((PVOID)lParam,
                             sizeof(EVENTMSG),
                                         1);
                RtlCopyMemory( &EventMsg,
                        (PVOID)lParam,
                        sizeof(EVENTMSG));
            }
            _SEH_HANDLE
            {
               BadChk = TRUE;
            }
            _SEH_END;
            if (BadChk)
            {
                DPRINT1("HOOK WH_JOURNAL read from lParam ERROR!\n");
            }
         }
         if (!BadChk) 
         {               
            lResult = co_HOOK_CallHookNext(Hook, Code, wParam, (LPARAM)(lParam ? &EventMsg : NULL));
            if (lParam)
            {
               _SEH_TRY
               {
                  ProbeForWrite((PVOID)lParam,
                                  sizeof(EVENTMSG),
                                            1);
                  RtlCopyMemory((PVOID)lParam,
                                         &EventMsg,
                                  sizeof(EVENTMSG));
               }
               _SEH_HANDLE
               {
                 BadChk = TRUE;
               }
               _SEH_END;
               if (BadChk)
               {
                  DPRINT1("HOOK WH_JOURNAL write to lParam ERROR!\n");
               }
            }
         }
         break;
      }

      case WH_DEBUG:
         lResult = IntCallDebugHook(Hook, Code, wParam, lParam);
         break;
/*
    Default the rest like, WH_FOREGROUNDIDLE, WH_KEYBOARD and WH_SHELL.
 */
      case WH_FOREGROUNDIDLE:
      case WH_KEYBOARD:
      case WH_SHELL:
         lResult = co_HOOK_CallHookNext(Hook, Code, wParam, lParam);      
         break;

      default:
         DPRINT1("Unsupported HOOK Id -> %d\n",Hook->HookId);
         break;
  }
  return lResult; 
}

LRESULT
STDCALL
NtUserCallNextHookEx(
   int Code,
   WPARAM wParam,
   LPARAM lParam,
   BOOL Ansi)
{
   PHOOK HookObj, NextObj;
   PCLIENTINFO ClientInfo;
   PWINSTATION_OBJECT WinStaObj;
   NTSTATUS Status;
   DECLARE_RETURN(LRESULT);

   DPRINT("Enter NtUserCallNextHookEx\n");
   UserEnterExclusive();

   Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                           KernelMode,
                                           0,
                                           &WinStaObj);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( 0);
   }

   ObDereferenceObject(WinStaObj);

   ClientInfo = GetWin32ClientInfo();

   if (!ClientInfo) RETURN( 0);

   HookObj = ClientInfo->phkCurrent;

   if (!HookObj) RETURN( 0);

   UserReferenceObject(HookObj);

   Ansi = HookObj->Ansi;

   if (NULL != HookObj->Thread && (HookObj->Thread != PsGetCurrentThread()))
   {
      DPRINT1("Thread mismatch\n");
      UserDereferenceObject(HookObj);
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      RETURN( 0);
   }
   
   NextObj = IntGetNextHook(HookObj);
   ClientInfo->phkCurrent = NextObj; // Preset next hook from list.
   UserCallNextHookEx( HookObj, Code, wParam, lParam, Ansi);
   UserDereferenceObject(HookObj);

   RETURN( (LRESULT)NextObj);

CLEANUP:
   DPRINT("Leave NtUserCallNextHookEx, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

HHOOK
STDCALL
NtUserSetWindowsHookAW(
   int idHook, 
   HOOKPROC lpfn,
   BOOL Ansi)
{
   UNICODE_STRING USModuleName;
   RtlInitUnicodeString(&USModuleName, NULL);
   return NtUserSetWindowsHookEx(NULL, &USModuleName, 0, idHook, lpfn, Ansi);
}

HHOOK
STDCALL
NtUserSetWindowsHookEx(
   HINSTANCE Mod,
   PUNICODE_STRING UnsafeModuleName,
   DWORD ThreadId,
   int HookId,
   HOOKPROC HookProc,
   BOOL Ansi)
{
   PWINSTATION_OBJECT WinStaObj;
   PCLIENTINFO ClientInfo;
   BOOLEAN Global;
   PETHREAD Thread;
   PHOOK Hook;
   UNICODE_STRING ModuleName;
   NTSTATUS Status;
   HHOOK Handle;
   DECLARE_RETURN(HHOOK);

   DPRINT("Enter NtUserSetWindowsHookEx\n");
   UserEnterExclusive();

   if (HookId < WH_MINHOOK || WH_MAXHOOK < HookId )
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( NULL);
   }

   if (!HookProc)
   {
      SetLastWin32Error(ERROR_INVALID_FILTER_PROC);
      RETURN( NULL);
   }

   ClientInfo = GetWin32ClientInfo();

   if (ThreadId)  /* thread-local hook */
   {
      if (HookId == WH_JOURNALRECORD ||
            HookId == WH_JOURNALPLAYBACK ||
            HookId == WH_KEYBOARD_LL ||
            HookId == WH_MOUSE_LL ||
            HookId == WH_SYSMSGFILTER)
      {
         /* these can only be global */
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         RETURN( NULL);
      }
      Mod = NULL;
      Global = FALSE;
      if (! NT_SUCCESS(PsLookupThreadByThreadId((HANDLE) ThreadId, &Thread)))
      {
         DPRINT1("Invalid thread id 0x%x\n", ThreadId);
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         RETURN( NULL);
      }
      if (Thread->ThreadsProcess != PsGetCurrentProcess())
      {
         ObDereferenceObject(Thread);
         DPRINT1("Can't specify thread belonging to another process\n");
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         RETURN( NULL);
      }
   }
   else  /* system-global hook */
   {
      if (HookId == WH_KEYBOARD_LL || HookId == WH_MOUSE_LL)
      {
         Mod = NULL;
         Thread = PsGetCurrentThread();
         Status = ObReferenceObjectByPointer(Thread,
                                             THREAD_ALL_ACCESS,
                                             PsThreadType,
                                             KernelMode);

         if (! NT_SUCCESS(Status))
         {
            SetLastNtError(Status);
            RETURN( (HANDLE) NULL);
         }
      }
      else if (NULL ==  Mod)
      {
         SetLastWin32Error(ERROR_HOOK_NEEDS_HMOD);
         RETURN( NULL);
      }
      else
      {
         Thread = NULL;
      }
      Global = TRUE;
   }

   if ( ( Global && (HookId != WH_KEYBOARD_LL || HookId != WH_MOUSE_LL) ) ||
        WH_DEBUG == HookId ||
        WH_JOURNALPLAYBACK == HookId ||
        WH_JOURNALRECORD == HookId)
   {
#if 0 /* Removed to get winEmbed working again */
      UNIMPLEMENTED
#else
      DPRINT1("Not implemented: HookId %d Global %s\n", HookId, Global ? "TRUE" : "FALSE");
#endif

      if (NULL != Thread)
      {
         ObDereferenceObject(Thread);
      }
      SetLastWin32Error(ERROR_NOT_SUPPORTED);
      RETURN( NULL);
   }

   Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                           KernelMode,
                                           0,
                                           &WinStaObj);

   if (! NT_SUCCESS(Status))
   {
      if (NULL != Thread)
      {
         ObDereferenceObject(Thread);
      }
      SetLastNtError(Status);
      RETURN( (HANDLE) NULL);
   }

   Hook = IntAddHook(Thread, HookId, Global, WinStaObj);
   if (NULL == Hook)
   {
      if (NULL != Thread)
      {
         ObDereferenceObject(Thread);
      }
      ObDereferenceObject(WinStaObj);
      RETURN( NULL);
   }

   if (NULL != Thread)
   {
      Hook->Flags |= HOOK_THREAD_REFERENCED;
   }

   if (NULL != Mod)
   {
      Status = MmCopyFromCaller(&ModuleName, UnsafeModuleName, sizeof(UNICODE_STRING));
      if (! NT_SUCCESS(Status))
      {
         UserDereferenceObject(Hook);
         IntRemoveHook(Hook, WinStaObj, FALSE);
         if (NULL != Thread)
         {
            ObDereferenceObject(Thread);
         }
         ObDereferenceObject(WinStaObj);
         SetLastNtError(Status);
         RETURN( NULL);
      }
      Hook->ModuleName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                ModuleName.MaximumLength,
                                TAG_HOOK);
      if (NULL == Hook->ModuleName.Buffer)
      {
         UserDereferenceObject(Hook);
         IntRemoveHook(Hook, WinStaObj, FALSE);
         if (NULL != Thread)
         {
            ObDereferenceObject(Thread);
         }
         ObDereferenceObject(WinStaObj);
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         RETURN( NULL);
      }
      Hook->ModuleName.MaximumLength = ModuleName.MaximumLength;
      Status = MmCopyFromCaller(Hook->ModuleName.Buffer,
                                ModuleName.Buffer,
                                ModuleName.MaximumLength);
      if (! NT_SUCCESS(Status))
      {
         ExFreePool(Hook->ModuleName.Buffer);
         UserDereferenceObject(Hook);
         IntRemoveHook(Hook, WinStaObj, FALSE);
         if (NULL != Thread)
         {
            ObDereferenceObject(Thread);
         }
         ObDereferenceObject(WinStaObj);
         SetLastNtError(Status);
         RETURN( NULL);
      }
      Hook->ModuleName.Length = ModuleName.Length;
      /* make proc relative to the module base */
      Hook->Proc = (void *)((char *)HookProc - (char *)Mod);
   }
   else
     Hook->Proc = HookProc;

   Hook->Ansi = Ansi;
   Handle = Hook->Self;

// Clear the client threads next hook.
   ClientInfo->phkCurrent = 0;
   
   UserDereferenceObject(Hook);
   ObDereferenceObject(WinStaObj);

   RETURN( Handle);

CLEANUP:
   DPRINT("Leave NtUserSetWindowsHookEx, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
STDCALL
NtUserUnhookWindowsHookEx(
   HHOOK Hook)
{
   PWINSTATION_OBJECT WinStaObj;
   PHOOK HookObj;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserUnhookWindowsHookEx\n");
   UserEnterExclusive();

   Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                                           KernelMode,
                                           0,
                                           &WinStaObj);

   if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   //  Status = UserReferenceObjectByHandle(gHandleTable, Hook,
   //                                      otHookProc, (PVOID *) &HookObj);
   if (!(HookObj = IntGetHookObject(Hook)))
   {
      DPRINT1("Invalid handle passed to NtUserUnhookWindowsHookEx\n");
      ObDereferenceObject(WinStaObj);
      //      SetLastNtError(Status);
      RETURN( FALSE);
   }
   ASSERT(Hook == HookObj->Self);

   IntRemoveHook(HookObj, WinStaObj, FALSE);

   UserDereferenceObject(HookObj);
   ObDereferenceObject(WinStaObj);

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserUnhookWindowsHookEx, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}
 
/* EOF */
