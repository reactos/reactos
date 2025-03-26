/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Window event handlers
 * PROGRAMER:         James Tabor <james.tabor@reactos.org>
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserEvent);

typedef struct _EVENTPACK
{
  PEVENTHOOK pEH;
  LONG idObject;
  LONG idChild;
  LONG idThread;
} EVENTPACK, *PEVENTPACK;

static PEVENTTABLE GlobalEvents = NULL;

/* PRIVATE FUNCTIONS *********************************************************/

static
DWORD
FASTCALL
GetMaskFromEvent(DWORD Event)
{
  DWORD Ret = 0;

  if ( Event > EVENT_OBJECT_STATECHANGE )
  {
    if ( Event == EVENT_OBJECT_LOCATIONCHANGE ) return SRV_EVENT_LOCATIONCHANGE;
    if ( Event == EVENT_OBJECT_NAMECHANGE )     return SRV_EVENT_NAMECHANGE;
    if ( Event == EVENT_OBJECT_VALUECHANGE )    return SRV_EVENT_VALUECHANGE;
    return SRV_EVENT_CREATE;
  }

  if ( Event == EVENT_OBJECT_STATECHANGE ) return SRV_EVENT_STATECHANGE;

  Ret = SRV_EVENT_RUNNING;

  if ( Event < EVENT_SYSTEM_MENUSTART )    return SRV_EVENT_CREATE;

  if ( Event <= EVENT_SYSTEM_MENUPOPUPEND )
  {
    Ret = SRV_EVENT_MENU;
  }
  else
  {
    if ( Event <= EVENT_CONSOLE_CARET-1 )         return SRV_EVENT_CREATE;
    if ( Event <= EVENT_CONSOLE_END_APPLICATION ) return SRV_EVENT_END_APPLICATION;
    if ( Event != EVENT_OBJECT_FOCUS )            return SRV_EVENT_CREATE;
  }
  return Ret;
}

static
VOID
FASTCALL
IntSetSrvEventMask( UINT EventMin, UINT EventMax)
{
   UINT event;
   TRACE("SetSrvEventMask 1\n");
   for ( event = EventMin; event <= EventMax; event++)
   {
      if ((event >= EVENT_SYSTEM_SOUND && event <= EVENT_SYSTEM_MINIMIZEEND) ||
          (event >= EVENT_CONSOLE_CARET && event <= EVENT_CONSOLE_END_APPLICATION) ||
          (event >= EVENT_OBJECT_CREATE && event <= EVENT_OBJECT_ACCELERATORCHANGE))
      {
         gpsi->dwInstalledEventHooks |= GetMaskFromEvent(event);
      }
      if (event > EVENT_SYSTEM_MINIMIZEEND && event < EVENT_CONSOLE_CARET)
      {
          event = EVENT_CONSOLE_CARET-1;
          gpsi->dwInstalledEventHooks |= GetMaskFromEvent(event);
      }
      if (event > EVENT_CONSOLE_END_APPLICATION && event < EVENT_OBJECT_CREATE )
      {
          event = EVENT_OBJECT_CREATE-1;
          gpsi->dwInstalledEventHooks |= GetMaskFromEvent(event);
      }
      if (event > EVENT_OBJECT_ACCELERATORCHANGE && event < EVENT_MAX)
      {
          event = EVENT_MAX-1;
          gpsi->dwInstalledEventHooks |= GetMaskFromEvent(event);
          break;
      }
   }
   if (!gpsi->dwInstalledEventHooks)
      gpsi->dwInstalledEventHooks |= SRV_EVENT_RUNNING; // Set something.
   TRACE("SetSrvEventMask 2 : %x\n", gpsi->dwInstalledEventHooks);
}

static
LRESULT
FASTCALL
IntCallLowLevelEvent( PEVENTHOOK pEH,
                         DWORD event,
                           HWND hwnd,
                       LONG idObject,
                        LONG idChild,
                       LONG idThread)
{
   PEVENTPACK pEP;
   MSG Msg;

   pEP = ExAllocatePoolWithTag(NonPagedPool, sizeof(EVENTPACK), TAG_HOOK);
   if (!pEP) return 0;

   pEP->pEH = pEH;
   pEP->idObject = idObject;
   pEP->idChild = idChild;
   pEP->idThread = idThread;

   Msg.message = event;
   Msg.hwnd = hwnd;
   Msg.wParam = 0;
   Msg.lParam = POSTEVENT_NWE;
   Msg.time = 0;

   MsqPostMessage(pEH->head.pti, &Msg, FALSE, QS_EVENT, POSTEVENT_NWE, (LONG_PTR)pEP);
   return 0;
}

BOOLEAN
IntRemoveEvent(PVOID Object)
{
   PEVENTHOOK pEH = Object;
   if (pEH)
   {
      TRACE("IntRemoveEvent pEH %p\n", pEH);
      KeEnterCriticalRegion();
      RemoveEntryList(&pEH->Chain);
      GlobalEvents->Counts--;
      if (!GlobalEvents->Counts) gpsi->dwInstalledEventHooks = 0;
      UserDeleteObject(UserHMGetHandle(pEH), TYPE_WINEVENTHOOK);
      KeLeaveCriticalRegion();
      return TRUE;
   }
   return FALSE;
}

/* FUNCTIONS *****************************************************************/

//
// Dispatch MsgQueue Event Call processor!
//
LRESULT
APIENTRY
co_EVENT_CallEvents( DWORD event,
                     HWND hwnd,
                     UINT_PTR idObject,
                     LONG_PTR idChild)
{
   PEVENTHOOK pEH;
   LRESULT Result;
   PEVENTPACK pEP = (PEVENTPACK)idChild;

   pEH = pEP->pEH;
   TRACE("Dispatch Event 0x%lx, idObject %uI hwnd %p\n", event, idObject, hwnd);
   Result = co_IntCallEventProc( UserHMGetHandle(pEH),
                                 event,
                                 hwnd,
                                 pEP->idObject,
                                 pEP->idChild,
                                 pEP->idThread,
                                 EngGetTickCount32(),
                                 pEH->Proc,
                                 pEH->ihmod,
                                 pEH->offPfn);

   ExFreePoolWithTag(pEP, TAG_HOOK);
   return Result;
}

VOID
FASTCALL
IntNotifyWinEvent(
   DWORD Event,
   PWND  pWnd,
   LONG  idObject,
   LONG  idChild,
   DWORD flags)
{
   PEVENTHOOK pEH;
   PLIST_ENTRY ListEntry;
   PTHREADINFO pti, ptiCurrent;
   USER_REFERENCE_ENTRY Ref;

   TRACE("IntNotifyWinEvent GlobalEvents = %p pWnd %p\n", GlobalEvents, pWnd);

   if (!GlobalEvents || !GlobalEvents->Counts) return;

   if (pWnd && pWnd->state & WNDS_DESTROYED) return;

   ptiCurrent = PsGetCurrentThreadWin32Thread();

   if (pWnd && flags & WEF_SETBYWNDPTI)
      pti = pWnd->head.pti;
   else
      pti = ptiCurrent;

   ListEntry = GlobalEvents->Events.Flink;
   ASSERT(ListEntry != &GlobalEvents->Events);
   while (ListEntry != &GlobalEvents->Events)
   {
     pEH = CONTAINING_RECORD(ListEntry, EVENTHOOK, Chain);
     ListEntry = ListEntry->Flink;

     // Must be inside the event window.
     if ( Event >= pEH->eventMin && Event <= pEH->eventMax )
     {
     // if all process || all thread || other thread same process
     // if ^skip own thread && ((Pid && CPid == Pid && ^skip own process) || all process)
        if (!( (pEH->idProcess && pEH->idProcess != PtrToUint(pti->pEThread->Cid.UniqueProcess)) ||
               (pEH->Flags & WINEVENT_SKIPOWNPROCESS && pEH->head.pti->ppi == pti->ppi) ||
               (pEH->idThread && pEH->idThread != PtrToUint(pti->pEThread->Cid.UniqueThread)) ||
               (pEH->Flags & WINEVENT_SKIPOWNTHREAD && pEH->head.pti == pti) ||
                pEH->head.pti->rpdesk != ptiCurrent->rpdesk ) ) // Same as hooks.
        {
           UserRefObjectCo(pEH, &Ref);
           if (pEH->Flags & WINEVENT_INCONTEXT)
           {
              TRACE("In       Event 0x%x, idObject %d hwnd %p\n", Event, idObject, pWnd ? UserHMGetHandle(pWnd) : NULL);
              co_IntCallEventProc( UserHMGetHandle(pEH),
                                   Event,
                                   pWnd ? UserHMGetHandle(pWnd) : NULL,
                                   idObject,
                                   idChild,
                                   PtrToUint(NtCurrentTeb()->ClientId.UniqueThread),
                                   EngGetTickCount32(),
                                   pEH->Proc,
                                   pEH->ihmod,
                                   pEH->offPfn);
           }
           else
           {
              TRACE("Out      Event 0x%x, idObject %d hwnd %p\n", Event, idObject, pWnd ? UserHMGetHandle(pWnd) : NULL);
              IntCallLowLevelEvent( pEH,
                                    Event,
                                    pWnd ? UserHMGetHandle(pWnd) : NULL,
                                    idObject,
                                    idChild,
                                    PtrToUint(NtCurrentTeb()->ClientId.UniqueThread));
           }
           UserDerefObjectCo(pEH);
        }
     }
   }
}

VOID
APIENTRY
NtUserNotifyWinEvent(
   DWORD Event,
   HWND  hWnd,
   LONG  idObject,
   LONG  idChild)
{
   PWND Window = NULL;
   USER_REFERENCE_ENTRY Ref;
   UserEnterExclusive();

   /* Validate input */
   if (hWnd && (hWnd != INVALID_HANDLE_VALUE))
   {
     Window = UserGetWindowObject(hWnd);
     if (!Window)
     {
       UserLeave();
       return;
     }
   }

   if (gpsi->dwInstalledEventHooks & GetMaskFromEvent(Event))
   {
      if (Window) UserRefObjectCo(Window, &Ref);
      IntNotifyWinEvent( Event, Window, idObject, idChild, WEF_SETBYWNDPTI);
      if (Window) UserDerefObjectCo(Window);
   }
   UserLeave();
}

HWINEVENTHOOK
APIENTRY
NtUserSetWinEventHook(
   UINT eventMin,
   UINT eventMax,
   HMODULE hmodWinEventProc,
   PUNICODE_STRING puString,
   WINEVENTPROC lpfnWinEventProc,
   DWORD idProcess,
   DWORD idThread,
   UINT dwflags)
{
   PEVENTHOOK pEH;
   HWINEVENTHOOK Ret = NULL;
   NTSTATUS Status;
   HANDLE Handle;
   PTHREADINFO pti;

   TRACE("NtUserSetWinEventHook hmod %p, pfn %p\n", hmodWinEventProc, lpfnWinEventProc);

   UserEnterExclusive();

   if ( !GlobalEvents )
   {
      GlobalEvents = ExAllocatePoolWithTag(PagedPool, sizeof(EVENTTABLE), TAG_HOOK);
      if (GlobalEvents == NULL)
      {
         EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
         goto SetEventExit;
      }
      GlobalEvents->Counts = 0;
      InitializeListHead(&GlobalEvents->Events);
   }

   if (eventMin > eventMax)
   {
      EngSetLastError(ERROR_INVALID_HOOK_FILTER);
      goto SetEventExit;
   }

   if (!lpfnWinEventProc)
   {
      EngSetLastError(ERROR_INVALID_FILTER_PROC);
      goto SetEventExit;
   }

   if (dwflags & WINEVENT_INCONTEXT)
   {
      if (!hmodWinEventProc)
      {
         ERR("Hook needs a module\n");
         EngSetLastError(ERROR_HOOK_NEEDS_HMOD);
         goto SetEventExit;
      }
      if (puString == NULL)
      {
         ERR("Dll not found\n");
         EngSetLastError(ERROR_DLL_NOT_FOUND);
         goto SetEventExit;
      }
   }
   else
   {
      TRACE("Out of Context\n");
      hmodWinEventProc = 0;
   }

   if (idThread)
   {
      PETHREAD Thread;
      Status = PsLookupThreadByThreadId(UlongToHandle(idThread), &Thread);
      if (!NT_SUCCESS(Status))
      {
         EngSetLastError(ERROR_INVALID_THREAD_ID);
         goto SetEventExit;
      }
      pti = PsGetThreadWin32Thread(Thread);
      ObDereferenceObject(Thread);
   }
   else
   {
       pti = PsGetCurrentThreadWin32Thread();
   }
   // Creator, pti is set here.
   pEH = UserCreateObject(gHandleTable, NULL, pti, &Handle, TYPE_WINEVENTHOOK, sizeof(EVENTHOOK));
   if (pEH)
   {
      InsertTailList(&GlobalEvents->Events, &pEH->Chain);
      GlobalEvents->Counts++;

      UserHMSetHandle(pEH, Handle);
      pEH->eventMin  = eventMin;
      pEH->eventMax  = eventMax;
      pEH->idProcess = idProcess; // These are cmp'ed
      pEH->idThread  = idThread;  //  "
      pEH->Flags     = dwflags;
    /*
       If WINEVENT_INCONTEXT, set offset from hmod and proc. Save ihmod from
       the atom index table where the hmod data is saved to be recalled later
       if fSync set by WINEVENT_INCONTEXT.
       If WINEVENT_OUTOFCONTEXT just use proc..
       Do this instead....
     */
      if (hmodWinEventProc != NULL)
      {
         pEH->offPfn = (ULONG_PTR)((char *)lpfnWinEventProc - (char *)hmodWinEventProc);
         pEH->ihmod = (INT_PTR)hmodWinEventProc;
         pEH->Proc = lpfnWinEventProc;
      }
      else
      {
         pEH->Proc = lpfnWinEventProc;
         pEH->offPfn = 0;
         pEH->ihmod = (INT_PTR)hmodWinEventProc;
      }

      UserDereferenceObject(pEH);

      Ret = Handle;
      IntSetSrvEventMask( eventMin, eventMax);
   }

SetEventExit:
   UserLeave();
   return Ret;
}

BOOL
APIENTRY
NtUserUnhookWinEvent(
   HWINEVENTHOOK hWinEventHook)
{
   PEVENTHOOK pEH;
   BOOL Ret = FALSE;

   UserEnterExclusive();

   pEH = (PEVENTHOOK)UserGetObject(gHandleTable, hWinEventHook, TYPE_WINEVENTHOOK);
   if (pEH)
   {
      Ret = IntRemoveEvent(pEH);
   }

   UserLeave();
   return Ret;
}

/* EOF */
