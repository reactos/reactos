
#include <w32k.h>

#define NDEBUG
#include <debug.h>

//static PEVENTTABLE GlobalEvents;


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
DWORD
FASTCALL
TimeStamp(VOID)
{
 return (DWORD)((ULONGLONG)SharedUserData->TickCountLowDeprecated * SharedUserData->TickCountMultiplier / 16777216);
}

/* FUNCTIONS *****************************************************************/

LRESULT
FASTCALL
co_EVENT_CallEvents( DWORD event,
                       HWND hwnd, 
                   LONG idObject,
                    LONG idChild)
{

   PEVENTHOOK peh = UserHeapAlloc(sizeof(EVENTHOOK));

   if ((gpsi->SrvEventActivity & GetMaskFromEvent(event))) return 0; // No events to run.

   LRESULT Result = co_IntCallEventProc(peh->Self,
                                            event,
                                             hwnd,
                                         idObject,
                                          idChild,
        (DWORD)(NtCurrentTeb()->Cid).UniqueThread,
                                      TimeStamp(),
                                        peh->Proc);
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
   UNIMPLEMENTED
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
   gpsi->SrvEventActivity |= GetMaskFromEvent(eventMin);
   gpsi->SrvEventActivity &= ~GetMaskFromEvent(eventMin);

   UNIMPLEMENTED

   return 0;
}


BOOL
STDCALL
NtUserUnhookWinEvent(
   HWINEVENTHOOK hWinEventHook)
{
   UNIMPLEMENTED

   return FALSE;
}

/* EOF */
