/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Window event handlers
 * FILE:              subsystem/win32/win32k/ntuser/event.c
 * PROGRAMER:         James Tabor (james.tabor@rectos.org)
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

typedef struct _EVENTPACK
{
  PEVENTHOOK pEH; 
  LONG idObject;
  LONG idChild;
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
   DPRINT("SetSrvEventMask 1\n");
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
   DPRINT("SetSrvEventMask 2 : %x\n", gpsi->dwInstalledEventHooks);
}

static
LRESULT
FASTCALL
IntCallLowLevelEvent( PEVENTHOOK pEH,
                         DWORD event,
                           HWND hwnd, 
                       LONG idObject,
                        LONG idChild)
{
   NTSTATUS Status;
   ULONG_PTR uResult;
   EVENTPACK EP;

   EP.pEH = pEH;
   EP.idObject = idObject;
   EP.idChild = idChild;

   /* FIXME should get timeout from
    * HKEY_CURRENT_USER\Control Panel\Desktop\LowLevelHooksTimeout */
   Status = co_MsqSendMessage( pEH->head.pti->MessageQueue,
                               hwnd,
                               event,
                               0,
                              (LPARAM)&EP,
                               5000,
                               TRUE,
                               MSQ_ISEVENT,
                              &uResult);

   return NT_SUCCESS(Status) ? uResult : 0;
}


static
BOOL
FASTCALL
IntRemoveEvent(PEVENTHOOK pEH)
{
   if (pEH)
   {
      DPRINT("IntRemoveEvent pEH 0x%x\n",pEH);
      KeEnterCriticalRegion();
      RemoveEntryList(&pEH->Chain);
      GlobalEvents->Counts--;
      if (!GlobalEvents->Counts) gpsi->dwInstalledEventHooks = 0;
      UserDeleteObject(UserHMGetHandle(pEH), otEvent);
      KeLeaveCriticalRegion();
      return TRUE;
   }
   return FALSE;
}

/* FUNCTIONS *****************************************************************/

LRESULT
FASTCALL
co_EVENT_CallEvents( DWORD event,
                       HWND hwnd, 
                    UINT_PTR idObject,
                    LONG_PTR idChild)
{
   PEVENTHOOK pEH;
   LRESULT Result;
   PEVENTPACK pEP = (PEVENTPACK)idChild;

   pEH = pEP->pEH;
   
   Result = co_IntCallEventProc( UserHMGetHandle(pEH),
                                 event,
                                 hwnd,
                                 pEP->idObject,
                                 pEP->idChild,
                                (DWORD_PTR)(NtCurrentTeb()->ClientId).UniqueThread,
                                (DWORD)EngGetTickCount(),
                                 pEH->Proc);
   return Result;
}

VOID
FASTCALL
IntNotifyWinEvent(
   DWORD Event,
   PWND  pWnd,
   LONG  idObject,
   LONG  idChild)
{
   PEVENTHOOK pEH;
   PLIST_ENTRY pLE;
   LRESULT Result;

   DPRINT("IntNotifyWinEvent GlobalEvents = 0x%x pWnd 0x%x\n",GlobalEvents, pWnd);

   if (!pWnd) return;

   if (pWnd && pWnd->state & WNDS_DESTROYED) return;

   if (!GlobalEvents || !GlobalEvents->Counts) return;

   pLE = GlobalEvents->Events.Flink;
   pEH = CONTAINING_RECORD(pLE, EVENTHOOK, Chain);
   do
   { 
     UserReferenceObject(pEH);
     // Must be inside the event window.
     if ( (pEH->eventMin <= Event) && (pEH->eventMax >= Event))
     {
        if (pEH->head.pti->pEThread != PsGetCurrentThread())
        { // if all process || all thread || other thread same process
           if (!(pEH->idProcess) || !(pEH->idThread) || 
               (NtCurrentTeb()->ClientId.UniqueProcess == (PVOID)(DWORD_PTR)pEH->idProcess))
           {
              Result = IntCallLowLevelEvent( pEH,
                                             Event,
                                             UserHMGetHandle(pWnd),
                                             idObject,
                                             idChild);
           }
        }// if ^skip own thread && ((Pid && CPid == Pid && ^skip own process) || all process)
        else if ( !(pEH->Flags & WINEVENT_SKIPOWNTHREAD) &&
                   ( ((pEH->idProcess &&
                     NtCurrentTeb()->ClientId.UniqueProcess == (PVOID)(DWORD_PTR)pEH->idProcess) &&
                     !(pEH->Flags & WINEVENT_SKIPOWNPROCESS)) ||
                     !pEH->idProcess ) )
        {
           Result = co_IntCallEventProc( UserHMGetHandle(pEH),
                                                        Event,
                                        UserHMGetHandle(pWnd),
                                                     idObject,
                                                      idChild,
             PtrToUint(NtCurrentTeb()->ClientId.UniqueThread),
                                     (DWORD)EngGetTickCount(),
                                                    pEH->Proc);
        }
     }
     UserDereferenceObject(pEH);
     pLE = pEH->Chain.Flink;
     pEH = CONTAINING_RECORD(pLE, EVENTHOOK, Chain);
   } while (pLE != &GlobalEvents->Events);
}            

VOID
APIENTRY
NtUserNotifyWinEvent(
   DWORD Event,
   HWND  hWnd,
   LONG  idObject,
   LONG  idChild)
{
   PWINDOW_OBJECT Window = NULL;
   USER_REFERENCE_ENTRY Ref;
   UserEnterExclusive();

   /* Validate input */
   if (hWnd && (hWnd != INVALID_HANDLE_VALUE) && !(Window = UserGetWindowObject(hWnd)))
   {
      UserLeave();
      return;
   }

   if (gpsi->dwInstalledEventHooks & GetMaskFromEvent(Event))
   {
      UserRefObjectCo(Window, &Ref);
      IntNotifyWinEvent( Event, Window->Wnd, idObject, idChild);
      UserDerefObjectCo(Window);
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
   PETHREAD Thread = NULL;

   DPRINT("NtUserSetWinEventHook hmod 0x%x, pfn 0x%x\n",hmodWinEventProc, lpfnWinEventProc);

   UserEnterExclusive();

   if ( !GlobalEvents )
   {
      GlobalEvents = ExAllocatePoolWithTag(PagedPool, sizeof(EVENTTABLE), TAG_HOOK);
      if (GlobalEvents == NULL)
      {
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         goto SetEventExit;
      }
      GlobalEvents->Counts = 0;      
      InitializeListHead(&GlobalEvents->Events);
   }

   if (eventMin > eventMax)
   {
      SetLastWin32Error(ERROR_INVALID_HOOK_FILTER);
      goto SetEventExit;
   }

   if (!lpfnWinEventProc)
   {
      SetLastWin32Error(ERROR_INVALID_FILTER_PROC);
      goto SetEventExit;
   }

   if ((dwflags & WINEVENT_INCONTEXT) && !hmodWinEventProc)
   {
      SetLastWin32Error(ERROR_HOOK_NEEDS_HMOD);
      goto SetEventExit;
   }

   if (idThread)
   {
      Status = PsLookupThreadByThreadId((HANDLE)(DWORD_PTR)idThread, &Thread);
      if (!NT_SUCCESS(Status))
      {   
         SetLastWin32Error(ERROR_INVALID_THREAD_ID);
         goto SetEventExit;
      }
   }

   pEH = UserCreateObject(gHandleTable, NULL, &Handle, otEvent, sizeof(EVENTHOOK));
   if (pEH)
   {
      InsertTailList(&GlobalEvents->Events, &pEH->Chain);
      GlobalEvents->Counts++;

      UserHMGetHandle(pEH) = Handle;
      if (Thread)
         pEH->head.pti = Thread->Tcb.Win32Thread;
      else
         pEH->head.pti = GetW32ThreadInfo();
      pEH->eventMin  = eventMin;
      pEH->eventMax  = eventMax;
      pEH->idProcess = idProcess;
      pEH->idThread  = idThread;
      pEH->Flags     = dwflags;

      if (NULL != hmodWinEventProc)
      {
         pEH->offPfn = (ULONG_PTR)((char *)lpfnWinEventProc - (char *)hmodWinEventProc);
         pEH->ihmod = (INT)hmodWinEventProc;
         pEH->Proc = lpfnWinEventProc;
      }
      else
         pEH->Proc = lpfnWinEventProc;

      UserDereferenceObject(pEH);

      Ret = Handle;
      IntSetSrvEventMask( eventMin, eventMax);
   }

SetEventExit:
   if (Thread) ObDereferenceObject(Thread);
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

   pEH = (PEVENTHOOK)UserGetObject(gHandleTable, hWinEventHook, otEvent);
   if (pEH) 
   {
      Ret = IntRemoveEvent(pEH);
   }

   UserLeave();
   return Ret;
}

/* EOF */
