/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          ntuser init. and main funcs.
 *  FILE:             subsystems/win32/win32k/ntuser/ntuser.c
 *  REVISION HISTORY:
 *       16 July 2005   Created (hardon)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

BOOL InitSysParams();

/* GLOBALS *******************************************************************/

ERESOURCE UserLock;
ATOM AtomMessage; // Window Message atom.
ATOM AtomWndObj;  // Window Object atom.
BOOL gbInitialized;
HINSTANCE hModClient = NULL;
BOOL ClientPfnInit = FALSE;

/* PRIVATE FUNCTIONS *********************************************************/

static
NTSTATUS FASTCALL
InitUserAtoms(VOID)
{

  gpsi->atomSysClass[ICLS_MENU]      = 32768;
  gpsi->atomSysClass[ICLS_DESKTOP]   = 32769;
  gpsi->atomSysClass[ICLS_DIALOG]    = 32770;
  gpsi->atomSysClass[ICLS_SWITCH]    = 32771;
  gpsi->atomSysClass[ICLS_ICONTITLE] = 32772;
  gpsi->atomSysClass[ICLS_TOOLTIPS]  = 32774;
  
  /* System Message Atom */
  AtomMessage = IntAddGlobalAtom(L"Message", TRUE);
  gpsi->atomSysClass[ICLS_HWNDMESSAGE] = AtomMessage;

  /* System Context Help Id Atom */
  gpsi->atomContextHelpIdProp = IntAddGlobalAtom(L"SysCH", TRUE);

  AtomWndObj = IntAddGlobalAtom(L"SysWNDO", TRUE);

  return STATUS_SUCCESS;
}

/* FUNCTIONS *****************************************************************/


NTSTATUS FASTCALL InitUserImpl(VOID)
{
   NTSTATUS Status;

   ExInitializeResourceLite(&UserLock);

   if (!UserCreateHandleTable())
   {
      DPRINT1("Failed creating handle table\n");
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   Status = InitSessionImpl();
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Error init session impl.\n");
      return Status;
   }

   if (!gpsi)
   {
      gpsi = UserHeapAlloc(sizeof(SERVERINFO));
      if (gpsi)
      {
         RtlZeroMemory(gpsi, sizeof(SERVERINFO));
         DPRINT("Global Server Data -> %x\n", gpsi);
      }
   }

   InitUserAtoms();

   InitSysParams();

   return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
UserInitialize(
  HANDLE  hPowerRequestEvent,
  HANDLE  hMediaRequestEvent)
{
    NTSTATUS Status;

// Set W32PF_Flags |= (W32PF_READSCREENACCESSGRANTED | W32PF_IOWINSTA)
// Create Object Directory,,, Looks like create workstation. "\\Windows\\WindowStations"
// Create Event for Diconnect Desktop.
// Initialize Video.
// {
//     DrvInitConsole.
//     DrvChangeDisplaySettings.
//     Update Shared Device Caps.
//     Initialize User Screen.
// }
// Create ThreadInfo for this Thread!
// {

    GetW32ThreadInfo();
   
//    Callback to User32 Client Thread Setup

    Status = co_IntClientThreadSetup();

// }
// Set Global SERVERINFO Error flags.
// Load Resources.

    NtUserUpdatePerUserSystemParameters(0, TRUE);

    return STATUS_SUCCESS;
}

/*
    Called from win32csr.
 */
NTSTATUS
APIENTRY
NtUserInitialize(
  DWORD   dwWinVersion,
  HANDLE  hPowerRequestEvent,
  HANDLE  hMediaRequestEvent)
{
    NTSTATUS Status;

    DPRINT("Enter NtUserInitialize(%lx, %p, %p)\n",
           dwWinVersion, hPowerRequestEvent, hMediaRequestEvent);

    /* Check the Windows version */
    if (dwWinVersion != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Acquire exclusive lock */
    UserEnterExclusive();

    /* Check if we are already initialized */
    if (gbInitialized)
    {
        UserLeave();
        return STATUS_UNSUCCESSFUL;
    }

// Initialize Power Request List.
// Initialize Media Change.
// InitializeGreCSRSS();
// {
//    Startup DxGraphics.
//    calls ** IntGdiGetLanguageID() and sets it **.
//    Enables Fonts drivers, Initialize Font table & Stock Fonts.
// }

    /* Initialize USER */
    Status = UserInitialize(hPowerRequestEvent, hMediaRequestEvent);

    /* Set us as initialized */
    gbInitialized = TRUE;

    /* Return */
    UserLeave();
    return Status;
}


/*
RETURN
   True if current thread owns the lock (possibly shared)
*/
BOOL FASTCALL UserIsEntered(VOID)
{
   return ExIsResourceAcquiredExclusiveLite(&UserLock)
      || ExIsResourceAcquiredSharedLite(&UserLock);
}

BOOL FASTCALL UserIsEnteredExclusive(VOID)
{
   return ExIsResourceAcquiredExclusiveLite(&UserLock);
}

VOID FASTCALL CleanupUserImpl(VOID)
{
   ExDeleteResourceLite(&UserLock);
}

VOID FASTCALL UserEnterShared(VOID)
{
   KeEnterCriticalRegion();
   ExAcquireResourceSharedLite(&UserLock, TRUE);
}

VOID FASTCALL UserEnterExclusive(VOID)
{
   KeEnterCriticalRegion();
   ExAcquireResourceExclusiveLite(&UserLock, TRUE);
}

VOID FASTCALL UserLeave(VOID)
{
   ExReleaseResourceLite(&UserLock);
   KeLeaveCriticalRegion();
}
