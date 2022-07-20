/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          ntuser init. and main funcs.
 *  FILE:             win32ss/user/ntuser/ntuser.c
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMisc);

BOOL FASTCALL RegisterControlAtoms(VOID);

/* GLOBALS ********************************************************************/

PTHREADINFO gptiCurrent = NULL;
PPROCESSINFO gppiInputProvider = NULL;
BOOL g_AlwaysDisplayVersion = FALSE;
ERESOURCE UserLock;
ATOM AtomMessage;       // Window Message atom.
ATOM AtomWndObj;        // Window Object atom.
ATOM AtomLayer;         // Window Layer atom.
ATOM AtomFlashWndState; // Window Flash State atom.
ATOM AtomDDETrack;      // Window DDE Tracking atom.
ATOM AtomQOS;           // Window DDE Quality of Service atom.
HINSTANCE hModClient = NULL;
BOOL ClientPfnInit = FALSE;
ATOM gaGuiConsoleWndClass;
ATOM AtomImeLevel;

/* PRIVATE FUNCTIONS **********************************************************/

static
NTSTATUS FASTCALL
InitUserAtoms(VOID)
{
    RegisterControlAtoms();

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
    gpsi->atomIconProp   = IntAddGlobalAtom(L"SysIC", TRUE);

    gpsi->atomFrostedWindowProp = IntAddGlobalAtom(L"SysFrostedWindow", TRUE);

    AtomDDETrack = IntAddGlobalAtom(L"SysDT", TRUE);
    AtomQOS      = IntAddGlobalAtom(L"SysQOS", TRUE);
    AtomImeLevel = IntAddGlobalAtom(L"SysIMEL", TRUE);

    /*
     * FIXME: AddPropW uses the global kernel atom table, thus leading to conflicts if we use
     * the win32k atom table for this one. What is the right thing to do ?
     */
    // AtomWndObj = IntAddGlobalAtom(L"SysWNDO", TRUE);
    NtAddAtom(L"SysWNDO", 14, &AtomWndObj);

    AtomLayer = IntAddGlobalAtom(L"SysLayer", TRUE);
    AtomFlashWndState = IntAddGlobalAtom(L"FlashWState", TRUE);

    return STATUS_SUCCESS;
}

/* FUNCTIONS ******************************************************************/

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitUserImpl(VOID)
{
    NTSTATUS Status;
    HKEY hKey;

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

    Status = RegOpenKey(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows",
                        &hKey);
    if (NT_SUCCESS(Status))
    {
        DWORD dwValue = 0;
        RegReadDWORD(hKey, L"DisplayVersion", &dwValue);
        g_AlwaysDisplayVersion = !!dwValue;
        ZwClose(hKey);
    }

    InitSysParams();

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
InitVideo(VOID);

NTSTATUS
NTAPI
UserInitialize(VOID)
{
    static const DWORD wPattern55AA[] = /* 32 bit aligned */
    { 0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa,
      0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa };
    HBITMAP hPattern55AABitmap = NULL;
    NTSTATUS Status;

    NT_ASSERT(PsGetCurrentThreadWin32Thread() != NULL);

// Create Event for Disconnect Desktop.

    Status = UserCreateWinstaDirectory();
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the Video */
    Status = InitVideo();
    if (!NT_SUCCESS(Status))
    {
        /* We failed, bugcheck */
        KeBugCheckEx(VIDEO_DRIVER_INIT_FAILURE, Status, 0, 0, USER_VERSION);
    }

// {
//     DrvInitConsole.
//     DrvChangeDisplaySettings.
//     Update Shared Device Caps.
//     Initialize User Screen.
// }

// Set Global SERVERINFO Error flags.
// Load Resources.

    NtUserUpdatePerUserSystemParameters(0, TRUE);

    if (gpsi->hbrGray == NULL)
    {
        hPattern55AABitmap = GreCreateBitmap(8, 8, 1, 1, (LPBYTE)wPattern55AA);
        if (hPattern55AABitmap == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        gpsi->hbrGray = IntGdiCreatePatternBrush(hPattern55AABitmap);

        if (gpsi->hbrGray == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

/*
 * Called from usersrv.
 */
NTSTATUS
APIENTRY
NtUserInitialize(
    DWORD  dwWinVersion,
    HANDLE hPowerRequestEvent,
    HANDLE hMediaRequestEvent)
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
        /* No match, bugcheck */
        KeBugCheckEx(WIN32K_INIT_OR_RIT_FAILURE, 0, 0, dwWinVersion, USER_VERSION);
    }

    /* Acquire exclusive lock */
    UserEnterExclusive();

    /* Save the EPROCESS of CSRSS */
    InitCsrProcess(/*PsGetCurrentProcess()*/);

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
    return ExIsResourceAcquiredExclusiveLite(&UserLock) ||
           ExIsResourceAcquiredSharedLite(&UserLock);
}

BOOL FASTCALL UserIsEnteredExclusive(VOID)
{
    return ExIsResourceAcquiredExclusiveLite(&UserLock);
}

VOID FASTCALL CleanupUserImpl(VOID)
{
    ExDeleteResourceLite(&UserLock);
}

// Win: EnterSharedCrit
VOID FASTCALL UserEnterShared(VOID)
{
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&UserLock, TRUE);
}

// Win: EnterCrit
VOID FASTCALL UserEnterExclusive(VOID)
{
    ASSERT_NOGDILOCKS();
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&UserLock, TRUE);
    gptiCurrent = PsGetCurrentThreadWin32Thread();
}

// Win: LeaveCrit
VOID FASTCALL UserLeave(VOID)
{
    ASSERT_NOGDILOCKS();
    ASSERT(UserIsEntered());
    ExReleaseResourceLite(&UserLock);
    KeLeaveCriticalRegion();
}

/* EOF */
