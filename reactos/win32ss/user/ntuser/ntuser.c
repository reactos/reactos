/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          ntuser init. and main funcs.
 *  FILE:             subsystems/win32/win32k/ntuser/ntuser.c
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMisc);

/* GLOBALS *******************************************************************/

PTHREADINFO gptiCurrent = NULL;
PPROCESSINFO gppiInputProvider = NULL;
ERESOURCE UserLock;
ATOM AtomMessage; // Window Message atom.
ATOM AtomWndObj;  // Window Object atom.
ATOM AtomLayer;   // Window Layer atom.
ATOM AtomFlashWndState; // Window Flash State atom.
HINSTANCE hModClient = NULL;
BOOL ClientPfnInit = FALSE;
PEPROCESS gpepCSRSS = NULL;
ATOM gaGuiConsoleWndClass;

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

  gpsi->atomIconSmProp = IntAddGlobalAtom(L"SysICS", TRUE);
  gpsi->atomIconProp = IntAddGlobalAtom(L"SysIC", TRUE);

  gpsi->atomFrostedWindowProp = IntAddGlobalAtom(L"SysFrostedWindow", TRUE);

  /*
   * FIXME: AddPropW uses the global kernel atom table, thus leading to conflicts if we use
   * the win32k atom table for this ones. What is the right thing to do ?
   */
  // AtomWndObj = IntAddGlobalAtom(L"SysWNDO", TRUE);
  NtAddAtom(L"SysWNDO", 14, &AtomWndObj);
  AtomLayer = IntAddGlobalAtom(L"SysLayer", TRUE);
  AtomFlashWndState = IntAddGlobalAtom(L"FlashWState", TRUE);

  return STATUS_SUCCESS;
}

/* FUNCTIONS *****************************************************************/

INIT_FUNCTION
NTSTATUS
NTAPI
InitUserImpl(VOID)
{
   NTSTATUS Status;

   ExInitializeResourceLite(&UserLock);

   if (!UserCreateHandleTable())
   {
      ERR("Failed creating handle table\n");
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   Status = InitSessionImpl();
   if (!NT_SUCCESS(Status))
   {
      ERR("Error init session impl.\n");
      return Status;
   }

   InitUserAtoms();

   InitSysParams();

   return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
InitVideo();

NTSTATUS
NTAPI
UserInitialize(VOID)
{
    static const DWORD wPattern55AA[] = /* 32 bit aligned */
    { 0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa,
      0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa };
    HBITMAP hPattern55AABitmap = NULL;
    NTSTATUS Status;

// Set W32PF_Flags |= (W32PF_READSCREENACCESSGRANTED | W32PF_IOWINSTA)
// Create Event for Diconnect Desktop.

    Status = UserCreateWinstaDirectory();
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize Video. */
    Status = InitVideo();
    if (!NT_SUCCESS(Status)) return Status;

// {
//     DrvInitConsole.
//     DrvChangeDisplaySettings.
//     Update Shared Device Caps.
//     Initialize User Screen.
// }
// Create ThreadInfo for this Thread!
// {

    /* Initialize the current thread. */
    Status = UserCreateThreadInfo(PsGetCurrentThread());
    if (!NT_SUCCESS(Status)) return Status;

// }
// Set Global SERVERINFO Error flags.
// Load Resources.

    NtUserUpdatePerUserSystemParameters(0, TRUE);

    if (gpsi->hbrGray == NULL)
    {
       hPattern55AABitmap = GreCreateBitmap(8, 8, 1, 1, (LPBYTE)wPattern55AA);
       gpsi->hbrGray = IntGdiCreatePatternBrush(hPattern55AABitmap);
       GreDeleteObject(hPattern55AABitmap);
       GreSetBrushOwner(gpsi->hbrGray, GDI_OBJ_HMGR_PUBLIC);
    }

    return STATUS_SUCCESS;
}

/*
 * Called from usersrv.
 */
NTSTATUS
APIENTRY
NtUserInitialize(
  DWORD   dwWinVersion,
  HANDLE  hPowerRequestEvent,
  HANDLE  hMediaRequestEvent)
{
    NTSTATUS Status;

    TRACE("Enter NtUserInitialize(%lx, %p, %p)\n",
          dwWinVersion, hPowerRequestEvent, hMediaRequestEvent);

    /* Check if we are already initialized */
    if (gpepCSRSS)
        return STATUS_UNSUCCESSFUL;

    /* Check Windows USER subsystem version */
    if (dwWinVersion != USER_VERSION)
    {
        // FIXME: Should bugcheck!
        return STATUS_UNSUCCESSFUL;
    }

    /* Acquire exclusive lock */
    UserEnterExclusive();

    /* Save the EPROCESS of CSRSS */
    gpepCSRSS = PsGetCurrentProcess();

// Initialize Power Request List (use hPowerRequestEvent).
// Initialize Media Change (use hMediaRequestEvent).

// InitializeGreCSRSS();
// {
//    Startup DxGraphics.
//    calls ** UserGetLanguageID() and sets it **.
//    Enables Fonts drivers, Initialize Font table & Stock Fonts.
// }

    /* Initialize USER */
    Status = UserInitialize();

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
   ASSERT_NOGDILOCKS();
   KeEnterCriticalRegion();
   ExAcquireResourceExclusiveLite(&UserLock, TRUE);
   gptiCurrent = PsGetCurrentThreadWin32Thread();
}

VOID FASTCALL UserLeave(VOID)
{
   ASSERT_NOGDILOCKS();
   ASSERT(UserIsEntered());
   ExReleaseResourceLite(&UserLock);
   KeLeaveCriticalRegion();
}
