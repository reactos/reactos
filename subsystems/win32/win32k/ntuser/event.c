
#include <w32k.h>

#define NDEBUG
#include <debug.h>

static PEVENTTABLE GlobalEvents = NULL;
static ULONG EventSys[EVENT_SYSTEM_MINIMIZEEND+1] = {0};
static ULONG EventObj[( EVENT_OBJECT_ACCELERATORCHANGE - EVENT_OBJECT_CREATE) +1] = {0};

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
IntEventUpCount(ULONG eventMin, ULONG eventMax)
{
   INT i, Min, Max;

   if ( eventMin >= EVENT_SYSTEM_SOUND && eventMax <= EVENT_SYSTEM_MINIMIZEEND)
   {
      for (i = eventMin; i < eventMax; i++)
      {
         gpsi->SrvEventActivity |= GetMaskFromEvent(i);
         EventSys[i]++;
      }
   }
   if ( eventMin >= EVENT_OBJECT_CREATE && eventMax <= EVENT_OBJECT_ACCELERATORCHANGE)
   {
      for (i = eventMin; i < eventMax; i++)
      {
         gpsi->SrvEventActivity |= GetMaskFromEvent(i);
         EventObj[i - EVENT_OBJECT_CREATE]++;
      }
   }
   if ( eventMin >= EVENT_SYSTEM_SOUND && eventMax <= EVENT_OBJECT_ACCELERATORCHANGE)
   {
      Max = EVENT_SYSTEM_MINIMIZEEND;
      for (i = eventMin; i < Max; i++)
      {
         gpsi->SrvEventActivity |= GetMaskFromEvent(i);
         EventSys[i]++;
      }
      Min = EVENT_OBJECT_CREATE;
      for (i = Min; i < eventMax; i++)
      {
         gpsi->SrvEventActivity |= GetMaskFromEvent(i);
         EventObj[i - EVENT_OBJECT_CREATE]++;
      }
   }
}

static
VOID
FASTCALL
IntEventDownCount(ULONG eventMin, ULONG eventMax)
{
   INT i, Min, Max;

   if ( eventMin >= EVENT_SYSTEM_SOUND && eventMax <= EVENT_SYSTEM_MINIMIZEEND)
   {
      for (i = eventMin; i < eventMax; i++)
      {
         EventSys[i]--;
         if (!EventSys[i]) gpsi->SrvEventActivity &= ~GetMaskFromEvent(i);
      }
   }
   if ( eventMin >= EVENT_OBJECT_CREATE && eventMax <= EVENT_OBJECT_ACCELERATORCHANGE)
   {
      for (i = eventMin; i < eventMax; i++)
      {
         EventObj[i - EVENT_OBJECT_CREATE]--;
         if (!EventObj[i - EVENT_OBJECT_CREATE])
            gpsi->SrvEventActivity &= ~GetMaskFromEvent(i);
      }
   }
   if ( eventMin >= EVENT_SYSTEM_SOUND && eventMax <= EVENT_OBJECT_ACCELERATORCHANGE)
   {
      Max = EVENT_SYSTEM_MINIMIZEEND;
      for (i = eventMin; i < Max; i++)
      {
         EventSys[i]--;
         if (!EventSys[i]) gpsi->SrvEventActivity &= ~GetMaskFromEvent(i);
      }
      Min = EVENT_OBJECT_CREATE;
      for (i = Min; i < eventMax; i++)
      {
         EventObj[i - EVENT_OBJECT_CREATE]--;
         if (!EventObj[i - EVENT_OBJECT_CREATE])
            gpsi->SrvEventActivity &= ~GetMaskFromEvent(i);
      }
   }
}


static
DWORD
FASTCALL
TimeStamp(VOID)
{
 return (DWORD)((ULONGLONG)SharedUserData->TickCountLowDeprecated * SharedUserData->TickCountMultiplier / 16777216);
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

   /* FIXME should get timeout from
    * HKEY_CURRENT_USER\Control Panel\Desktop\LowLevelHooksTimeout */
   Status = co_MsqSendMessage(((PW32THREAD)pEH->Thread->Tcb.Win32Thread)->MessageQueue,
                                           hwnd,
                                          event,
                                       idObject,
                                        idChild,
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
      RemoveEntryList(&pEH->Chain);
      GlobalEvents->Counts--;
      UserDeleteObject(pEH->Self, otEvent);
      return TRUE;
   }
   return FALSE;
}

/* FUNCTIONS *****************************************************************/

LRESULT
FASTCALL
co_EVENT_CallEvents( DWORD event,
                       HWND hwnd, 
                   LONG idObject,
                    LONG idChild)
{

   PEVENTHOOK pEH = UserHeapAlloc(sizeof(EVENTHOOK));

   if ((gpsi->SrvEventActivity & GetMaskFromEvent(event))) return 0; // No events to run.


   if ((pEH->Thread != PsGetCurrentThread()) && (pEH->Thread != NULL))
   {
      // Post it in message queue.
      return IntCallLowLevelEvent(pEH, event, hwnd, idObject, idChild);
   }


   LRESULT Result = co_IntCallEventProc(pEH->Self,
                                            event,
                                             hwnd,
                                         idObject,
                                          idChild,
        (DWORD)(NtCurrentTeb()->Cid).UniqueThread,
                                      TimeStamp(),
                                        pEH->Proc);
   return Result;
}


VOID
STDCALL
NtUserNotifyWinEvent(
   DWORD Event,
   HWND  hWnd,
   LONG  idObject,
   LONG  idChild)
{
   UserEnterExclusive();
   UNIMPLEMENTED
   UserLeave();
}

HWINEVENTHOOK
STDCALL
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
   UNICODE_STRING ModuleName;
   NTSTATUS Status;
   HANDLE Handle;

   UserEnterExclusive();

   DPRINT1("WARNING! Use at your own risk! Function is UNIMPLEMENTED!\n");

   if ( !GlobalEvents )
   {
      GlobalEvents = ExAllocatePoolWithTag(PagedPool, sizeof(EVENTTABLE), TAG_HOOK);
      GlobalEvents->Counts = 0;      
      InitializeListHead(&GlobalEvents->Events);
   }

   pEH = UserCreateObject(gHandleTable, &Handle, otEvent, sizeof(EVENTHOOK));
   if (pEH)
   {
      InsertTailList(&GlobalEvents->Events, &pEH->Chain);
      GlobalEvents->Counts++;

      pEH->Self      = Handle;
      pEH->Thread    = PsGetCurrentThread();
      pEH->eventMin  = eventMin;
      pEH->eventMax  = eventMax;
      pEH->idProcess = idProcess;
      pEH->idThread  = idThread;
      pEH->Ansi      = FALSE;
      pEH->Flags     = dwflags;

      if ((dwflags & WINEVENT_INCONTEXT) && !hmodWinEventProc)
      {
         SetLastWin32Error(ERROR_HOOK_NEEDS_HMOD);
         goto SetEventExit;
      }
                        
      if (eventMin > eventMax)
      {
         SetLastWin32Error(ERROR_INVALID_HOOK_FILTER);
         goto SetEventExit;
      }

      if (NULL != hmodWinEventProc)
      {
         Status = MmCopyFromCaller(&ModuleName, puString, sizeof(UNICODE_STRING));
         if (! NT_SUCCESS(Status))
         {
            UserDereferenceObject(pEH);
            IntRemoveEvent(pEH);
            SetLastNtError(Status);
            goto SetEventExit;
         }
         pEH->ModuleName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                   ModuleName.MaximumLength,
                                   TAG_HOOK);
         if (NULL == pEH->ModuleName.Buffer)
         {
            UserDereferenceObject(pEH);
            IntRemoveEvent(pEH);
            SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
            goto SetEventExit;
          }
         pEH->ModuleName.MaximumLength = ModuleName.MaximumLength;
         Status = MmCopyFromCaller(pEH->ModuleName.Buffer,
                                   ModuleName.Buffer,
                                   ModuleName.MaximumLength);
         if (! NT_SUCCESS(Status))
         {
            ExFreePool(pEH->ModuleName.Buffer);
            UserDereferenceObject(pEH);
            IntRemoveEvent(pEH);
            SetLastNtError(Status);
            goto SetEventExit;
         }
         pEH->ModuleName.Length = ModuleName.Length;
         pEH->Proc = (void *)((char *)lpfnWinEventProc - (char *)hmodWinEventProc);
      }
      else
         pEH->Proc = lpfnWinEventProc;

      Ret = Handle;
      /*
         Now we are good, set the Events and counts.
       */
      IntEventUpCount(eventMin, eventMax);
   }

SetEventExit:
   UserLeave();
   return Ret;
}


BOOL
STDCALL
NtUserUnhookWinEvent(
   HWINEVENTHOOK hWinEventHook)
{
   PEVENTHOOK pEH;
   BOOL Ret = FALSE;

   UserEnterExclusive();

   DPRINT1("WARNING! Use at your own risk! Function is UNIMPLEMENTED!\n");

   pEH = (PEVENTHOOK)UserGetObject(gHandleTable, hWinEventHook, otEvent);
   if (pEH) 
   {
      IntEventDownCount(pEH->eventMin, pEH->eventMax);
      Ret = IntRemoveEvent(pEH);
   }

   UserLeave();
   return Ret;
}

/* EOF */
