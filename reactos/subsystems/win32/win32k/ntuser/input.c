/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsystems/win32/win32k/ntuser/input.c
 * PROGRAMERS:       Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                   Rafal Harabien (rafalh@reactos.org)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserInput);

extern NTSTATUS Win32kInitWin32Thread(PETHREAD Thread);
extern PPROCESSINFO ppiScrnSaver;

/* GLOBALS *******************************************************************/

PTHREADINFO ptiRawInput;
PTHREADINFO ptiKeyboard;
PTHREADINFO ptiMouse;
PKTIMER MasterTimer = NULL;
PATTACHINFO gpai = NULL;
HANDLE ghKeyboardDevice;

static DWORD LastInputTick = 0;
static HANDLE MouseDeviceHandle;
static HANDLE MouseThreadHandle;
static CLIENT_ID MouseThreadId;
static HANDLE KeyboardThreadHandle;
static CLIENT_ID KeyboardThreadId;
static HANDLE RawInputThreadHandle;
static CLIENT_ID RawInputThreadId;
static KEVENT InputThreadsStart;
static BOOLEAN InputThreadsRunning = FALSE;

/* FUNCTIONS *****************************************************************/

#define ClearMouseInput(mi) \
  mi.dx = 0; \
  mi.dy = 0; \
  mi.mouseData = 0; \
  mi.dwFlags = 0;

#define SendMouseEvent(mi) \
  if(mi.dx != 0 || mi.dy != 0) \
    mi.dwFlags |= MOUSEEVENTF_MOVE; \
  if(mi.dwFlags) \
    IntMouseInput(&mi,FALSE); \
  ClearMouseInput(mi);

static DWORD FASTCALL
IntLastInputTick(BOOL bUpdate)
{
    if (bUpdate)
    {
        LARGE_INTEGER TickCount;
        KeQueryTickCount(&TickCount);
        LastInputTick = MsqCalculateMessageTime(&TickCount);
        if (gpsi) gpsi->dwLastRITEventTickCount = LastInputTick;
    }
    return LastInputTick;
}

VOID FASTCALL
DoTheScreenSaver(VOID)
{
    LARGE_INTEGER TickCount;
    DWORD Test, TO;

    if (gspv.iScrSaverTimeout > 0) // Zero means Off.
    {
        KeQueryTickCount(&TickCount);
        Test = MsqCalculateMessageTime(&TickCount);
        Test = Test - LastInputTick;
        TO = 1000 * gspv.iScrSaverTimeout;
        if (Test > TO)
        {
            TRACE("Screensaver Message Start! Tick %d Timeout %d \n", Test, gspv.iScrSaverTimeout);

            if (ppiScrnSaver) // We are or we are not the screensaver, prevent reentry...
            {
                if (!(ppiScrnSaver->W32PF_flags & W32PF_IDLESCREENSAVER))
                {
                    ppiScrnSaver->W32PF_flags |= W32PF_IDLESCREENSAVER;
                    ERR("Screensaver is Idle\n");
                }
            }
            else
            {
                PUSER_MESSAGE_QUEUE ForegroundQueue = IntGetFocusMessageQueue();
                if (ForegroundQueue && ForegroundQueue->ActiveWindow)
                    UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_START_SCREENSAVE, 1); // lParam 1 == Secure
                else
                    UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_START_SCREENSAVE, 0);
            }
        }
    }
}

static VOID NTAPI
IntProcessMouseInputData(PMOUSE_INPUT_DATA Data, ULONG InputCount)
{
    PMOUSE_INPUT_DATA mid;
    MOUSEINPUT mi;
    ULONG i;

    ClearMouseInput(mi);
    mi.time = 0;
    mi.dwExtraInfo = 0;
    for(i = 0; i < InputCount; i++)
    {
        mid = (Data + i);
        mi.dx += mid->LastX;
        mi.dy += mid->LastY;

        /* Check if the mouse move is absolute */
        if (mid->Flags == MOUSE_MOVE_ABSOLUTE)
        {
            /* Set flag to convert to screen location */
            mi.dwFlags |= MOUSEEVENTF_ABSOLUTE;
        }

        if(mid->ButtonFlags)
        {
            if(mid->ButtonFlags & MOUSE_LEFT_BUTTON_DOWN)
            {
                mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_LEFT_BUTTON_UP)
            {
                mi.dwFlags |= MOUSEEVENTF_LEFTUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_MIDDLE_BUTTON_DOWN)
            {
                mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_MIDDLE_BUTTON_UP)
            {
                mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_RIGHT_BUTTON_DOWN)
            {
                mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_RIGHT_BUTTON_UP)
            {
                mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_BUTTON_4_DOWN)
            {
                mi.mouseData |= XBUTTON1;
                mi.dwFlags |= MOUSEEVENTF_XDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_BUTTON_4_UP)
            {
                mi.mouseData |= XBUTTON1;
                mi.dwFlags |= MOUSEEVENTF_XUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_BUTTON_5_DOWN)
            {
                mi.mouseData |= XBUTTON2;
                mi.dwFlags |= MOUSEEVENTF_XDOWN;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_BUTTON_5_UP)
            {
                mi.mouseData |= XBUTTON2;
                mi.dwFlags |= MOUSEEVENTF_XUP;
                SendMouseEvent(mi);
            }
            if(mid->ButtonFlags & MOUSE_WHEEL)
            {
                mi.mouseData = mid->ButtonData;
                mi.dwFlags |= MOUSEEVENTF_WHEEL;
                SendMouseEvent(mi);
            }
        }
    }

    SendMouseEvent(mi);
}

static VOID APIENTRY
MouseThreadMain(PVOID StartContext)
{
    UNICODE_STRING MouseDeviceName = RTL_CONSTANT_STRING(L"\\Device\\PointerClass0");
    OBJECT_ATTRIBUTES MouseObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    MOUSE_ATTRIBUTES MouseAttr;

    KeSetPriorityThread(&PsGetCurrentThread()->Tcb,
                        LOW_REALTIME_PRIORITY + 3);

    InitializeObjectAttributes(&MouseObjectAttributes,
                               &MouseDeviceName,
                               0,
                               NULL,
                               NULL);
    do
    {
        LARGE_INTEGER DueTime;
        KEVENT Event;
        DueTime.QuadPart = (LONGLONG)(-10000000);
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, &DueTime);
        Status = NtOpenFile(&MouseDeviceHandle,
                            FILE_ALL_ACCESS,
                            &MouseObjectAttributes,
                            &Iosb,
                            0,
                            FILE_SYNCHRONOUS_IO_ALERT);
    } while (!NT_SUCCESS(Status));

    /* Need to setup basic win32k for this thread to process WH_MOUSE_LL messages. */
    Status = Win32kInitWin32Thread(PsGetCurrentThread());
    if (!NT_SUCCESS(Status))
    {
        ERR("Win32K: Failed making mouse thread a win32 thread.\n");
        return; //(Status);
    }

    ptiMouse = PsGetCurrentThreadWin32Thread();
    ptiMouse->TIF_flags |= TIF_SYSTEMTHREAD;
    TRACE("Mouse Thread 0x%x \n", ptiMouse);

    KeSetPriorityThread(&PsGetCurrentThread()->Tcb,
                        LOW_REALTIME_PRIORITY + 3);

    for(;;)
    {
        /*
         * Wait to start input.
         */
        TRACE("Mouse Input Thread Waiting for start event\n");
        Status = KeWaitForSingleObject(&InputThreadsStart,
                                       0,
                                       KernelMode,
                                       TRUE,
                                       NULL);
        TRACE("Mouse Input Thread Starting...\n");

        /*FIXME: Does mouse attributes need to be used for anything */
        Status = NtDeviceIoControlFile(MouseDeviceHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &Iosb,
                                       IOCTL_MOUSE_QUERY_ATTRIBUTES,
                                       &MouseAttr, sizeof(MOUSE_ATTRIBUTES),
                                       NULL, 0);
        if(!NT_SUCCESS(Status))
        {
            TRACE("Failed to get mouse attributes\n");
        }

        /*
         * Receive and process mouse input.
         */
        while(InputThreadsRunning)
        {
            MOUSE_INPUT_DATA MouseInput;
            Status = NtReadFile(MouseDeviceHandle,
                                NULL,
                                NULL,
                                NULL,
                                &Iosb,
                                &MouseInput,
                                sizeof(MOUSE_INPUT_DATA),
                                NULL,
                                NULL);
            if(Status == STATUS_ALERTED && !InputThreadsRunning)
            {
                break;
            }
            if(Status == STATUS_PENDING)
            {
                NtWaitForSingleObject(MouseDeviceHandle, FALSE, NULL);
                Status = Iosb.Status;
            }
            if(!NT_SUCCESS(Status))
            {
                ERR("Win32K: Failed to read from mouse.\n");
                return; //(Status);
            }
            TRACE("MouseEvent\n");
            IntLastInputTick(TRUE);

            UserEnterExclusive();

            IntProcessMouseInputData(&MouseInput, Iosb.Information / sizeof(MOUSE_INPUT_DATA));

            UserLeave();
        }
        TRACE("Mouse Input Thread Stopped...\n");
    }
}

static VOID APIENTRY
KeyboardThreadMain(PVOID StartContext)
{
    UNICODE_STRING KeyboardDeviceName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
    OBJECT_ATTRIBUTES KeyboardObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    InitializeObjectAttributes(&KeyboardObjectAttributes,
                               &KeyboardDeviceName,
                               0,
                               NULL,
                               NULL);
    do
    {
        LARGE_INTEGER DueTime;
        KEVENT Event;
        DueTime.QuadPart = (LONGLONG)(-10000000);
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, &DueTime);
        Status = NtOpenFile(&ghKeyboardDevice,
                            FILE_ALL_ACCESS,
                            &KeyboardObjectAttributes,
                            &Iosb,
                            0,
                            FILE_SYNCHRONOUS_IO_ALERT);
    } while (!NT_SUCCESS(Status));

    /* Not sure if converting this thread to a win32 thread is such
       a great idea. Since we're posting keyboard messages to the focus
       window message queue, we'll be (indirectly) doing sendmessage
       stuff from this thread (for WH_KEYBOARD_LL processing), which
       means we need our own message queue. If keyboard messages were
       instead queued to the system message queue, the thread removing
       the message from the system message queue would be responsible
       for WH_KEYBOARD_LL processing and we wouldn't need this thread
       to be a win32 thread. */
    Status = Win32kInitWin32Thread(PsGetCurrentThread());
    if (!NT_SUCCESS(Status))
    {
        ERR("Win32K: Failed making keyboard thread a win32 thread.\n");
        return; //(Status);
    }

    ptiKeyboard = PsGetCurrentThreadWin32Thread();
    ptiKeyboard->TIF_flags |= TIF_SYSTEMTHREAD;
    TRACE("Keyboard Thread 0x%x \n", ptiKeyboard);

    KeSetPriorityThread(&PsGetCurrentThread()->Tcb,
                        LOW_REALTIME_PRIORITY + 3);

    //IntKeyboardGetIndicatorTrans(ghKeyboardDevice,
   //                              &IndicatorTrans);

    for (;;)
    {
        /*
         * Wait to start input.
         */
        TRACE( "Keyboard Input Thread Waiting for start event\n" );
        Status = KeWaitForSingleObject(&InputThreadsStart,
                                       0,
                                       KernelMode,
                                       TRUE,
                                       NULL);

        TRACE( "Keyboard Input Thread Starting...\n" );
        /*
         * Receive and process keyboard input.
         */
        while (InputThreadsRunning)
        {
            KEYBOARD_INPUT_DATA KeyInput;

            TRACE("KeyInput @ %08x\n", &KeyInput);

            Status = NtReadFile (ghKeyboardDevice,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &Iosb,
                                 &KeyInput,
                                 sizeof(KEYBOARD_INPUT_DATA),
                                 NULL,
                                 NULL);

            if(Status == STATUS_ALERTED && !InputThreadsRunning)
            {
                break;
            }
            if(Status == STATUS_PENDING)
            {
                NtWaitForSingleObject(ghKeyboardDevice, FALSE, NULL);
                Status = Iosb.Status;
            }
            if(!NT_SUCCESS(Status))
            {
                ERR("Win32K: Failed to read from keyboard.\n");
                return; //(Status);
            }

            TRACE("KeyRaw: %s %04x\n",
                  (KeyInput.Flags & KEY_BREAK) ? "up" : "down",
                  KeyInput.MakeCode );

            if (Status == STATUS_ALERTED && !InputThreadsRunning)
                break;

            if (!NT_SUCCESS(Status))
            {
                ERR("Win32K: Failed to read from keyboard.\n");
                return; //(Status);
            }

            /* Set LastInputTick */
            IntLastInputTick(TRUE);

            /* Process data */
            UserEnterExclusive();
            UserProcessKeyboardInput(&KeyInput);
            UserLeave();
        }

        TRACE( "KeyboardInput Thread Stopped...\n" );
    }
}


static PVOID Objects[2];
/*
    Raw Input Thread.
    Since this relies on InputThreadsStart, just fake it.
 */
static VOID APIENTRY
RawInputThreadMain(PVOID StartContext)
{
    NTSTATUS Status;
    LARGE_INTEGER DueTime;

    DueTime.QuadPart = (LONGLONG)(-10000000);

    do
    {
        KEVENT Event;
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, &DueTime);
    } while (!NT_SUCCESS(Status));


    Objects[0] = &InputThreadsStart;
    Objects[1] = MasterTimer;

    // This thread requires win32k!
    Status = Win32kInitWin32Thread(PsGetCurrentThread());
    if (!NT_SUCCESS(Status))
    {
        ERR("Win32K: Failed making Raw Input thread a win32 thread.\n");
        return; //(Status);
    }

    ptiRawInput = PsGetCurrentThreadWin32Thread();
    ptiRawInput->TIF_flags |= TIF_SYSTEMTHREAD;
    TRACE("Raw Input Thread 0x%x \n", ptiRawInput);

    KeSetPriorityThread(&PsGetCurrentThread()->Tcb,
                        LOW_REALTIME_PRIORITY + 3);

    UserEnterExclusive();
    StartTheTimers();
    UserLeave();

    //
    // ATM, we just have one job to handle, merge the other two later.
    //
    for(;;)
    {
        TRACE( "Raw Input Thread Waiting for start event\n" );

        Status = KeWaitForMultipleObjects( 2,
                                           Objects,
                                           WaitAll, //WaitAny,
                                           WrUserRequest,
                                           KernelMode,
                                           TRUE,
                                           NULL,
                                           NULL);
        TRACE( "Raw Input Thread Starting...\n" );

        ProcessTimers();
    }
    ERR("Raw Input Thread Exit!\n");
}

INIT_FUNCTION
NTSTATUS
NTAPI
InitInputImpl(VOID)
{
    NTSTATUS Status;

    KeInitializeEvent(&InputThreadsStart, NotificationEvent, FALSE);

    MasterTimer = ExAllocatePoolWithTag(NonPagedPool, sizeof(KTIMER), USERTAG_SYSTEM);
    if (!MasterTimer)
    {
        ERR("Win32K: Failed making Raw Input thread a win32 thread.\n");
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }
    KeInitializeTimer(MasterTimer);

    /* Initialize the default keyboard layout */
    if(!UserInitDefaultKeyboardLayout())
    {
        ERR("Failed to initialize default keyboard layout!\n");
    }

    Status = PsCreateSystemThread(&RawInputThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  NULL,
                                  NULL,
                                  &RawInputThreadId,
                                  RawInputThreadMain,
                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Win32K: Failed to create raw thread.\n");
    }

    Status = PsCreateSystemThread(&KeyboardThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  NULL,
                                  NULL,
                                  &KeyboardThreadId,
                                  KeyboardThreadMain,
                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Win32K: Failed to create keyboard thread.\n");
    }

    Status = PsCreateSystemThread(&MouseThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  NULL,
                                  NULL,
                                  &MouseThreadId,
                                  MouseThreadMain,
                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Win32K: Failed to create mouse thread.\n");
    }

    InputThreadsRunning = TRUE;
    KeSetEvent(&InputThreadsStart, IO_NO_INCREMENT, FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
CleanupInputImp(VOID)
{
    return(STATUS_SUCCESS);
}

BOOL FASTCALL
IntBlockInput(PTHREADINFO pti, BOOL BlockIt)
{
    PTHREADINFO OldBlock;
    ASSERT(pti);

    if(!pti->rpdesk || ((pti->TIF_flags & TIF_INCLEANUP) && BlockIt))
    {
        /*
         * fail blocking if exiting the thread
         */

        return FALSE;
    }

    /*
     * FIXME - check access rights of the window station
     *         e.g. services running in the service window station cannot block input
     */
    if(!ThreadHasInputAccess(pti) ||
            !IntIsActiveDesktop(pti->rpdesk))
    {
        EngSetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    ASSERT(pti->rpdesk);
    OldBlock = pti->rpdesk->BlockInputThread;
    if(OldBlock)
    {
        if(OldBlock != pti)
        {
            EngSetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
        pti->rpdesk->BlockInputThread = (BlockIt ? pti : NULL);
        return OldBlock == NULL;
    }

    pti->rpdesk->BlockInputThread = (BlockIt ? pti : NULL);
    return OldBlock == NULL;
}

BOOL
APIENTRY
NtUserBlockInput(
    BOOL BlockIt)
{
    DECLARE_RETURN(BOOLEAN);

    TRACE("Enter NtUserBlockInput\n");
    UserEnterExclusive();

    RETURN( IntBlockInput(PsGetCurrentThreadWin32Thread(), BlockIt));

CLEANUP:
    TRACE("Leave NtUserBlockInput, ret=%i\n", _ret_);
    UserLeave();
    END_CLEANUP;
}

BOOL FASTCALL
IntMouseInput(MOUSEINPUT *mi, BOOL Injected)
{
    const UINT SwapBtnMsg[2][2] =
    {
        {WM_LBUTTONDOWN, WM_RBUTTONDOWN},
        {WM_LBUTTONUP, WM_RBUTTONUP}
    };
    const WPARAM SwapBtn[2] =
    {
        MK_LBUTTON, MK_RBUTTON
    };
    POINT MousePos;
    PSYSTEM_CURSORINFO CurInfo;
    BOOL SwapButtons;
    MSG Msg;

    ASSERT(mi);

    CurInfo = IntGetSysCursorInfo();

    if(!mi->time)
    {
        LARGE_INTEGER LargeTickCount;
        KeQueryTickCount(&LargeTickCount);
        mi->time = MsqCalculateMessageTime(&LargeTickCount);
    }

    SwapButtons = gspv.bMouseBtnSwap;

    MousePos = gpsi->ptCursor;

    if(mi->dwFlags & MOUSEEVENTF_MOVE)
    {
        if(mi->dwFlags & MOUSEEVENTF_ABSOLUTE)
        {
            MousePos.x = mi->dx * UserGetSystemMetrics(SM_CXVIRTUALSCREEN) >> 16;
            MousePos.y = mi->dy * UserGetSystemMetrics(SM_CYVIRTUALSCREEN) >> 16;
        }
        else
        {
            MousePos.x += mi->dx;
            MousePos.y += mi->dy;
        }
    }

    /*
     * Insert the messages into the system queue
     */
    Msg.wParam = 0;
    Msg.lParam = MAKELPARAM(MousePos.x, MousePos.y);
    Msg.pt = MousePos;

    if (gafAsyncKeyState[VK_SHIFT] & KS_DOWN_BIT)
    {
        Msg.wParam |= MK_SHIFT;
    }

    if (gafAsyncKeyState[VK_CONTROL] & KS_DOWN_BIT)
    {
        Msg.wParam |= MK_CONTROL;
    }

    if(mi->dwFlags & MOUSEEVENTF_MOVE)
    {
        UserSetCursorPos(MousePos.x, MousePos.y, Injected, mi->dwExtraInfo, TRUE);
    }
    if(mi->dwFlags & MOUSEEVENTF_LEFTDOWN)
    {
        gafAsyncKeyState[VK_LBUTTON] |= KS_DOWN_BIT;
        Msg.message = SwapBtnMsg[0][SwapButtons];
        CurInfo->ButtonsDown |= SwapBtn[SwapButtons];
        Msg.wParam |= CurInfo->ButtonsDown;
        co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
    }
    else if(mi->dwFlags & MOUSEEVENTF_LEFTUP)
    {
        gafAsyncKeyState[VK_LBUTTON] &= ~KS_DOWN_BIT;
        Msg.message = SwapBtnMsg[1][SwapButtons];
        CurInfo->ButtonsDown &= ~SwapBtn[SwapButtons];
        Msg.wParam |= CurInfo->ButtonsDown;
        co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
    }
    if(mi->dwFlags & MOUSEEVENTF_MIDDLEDOWN)
    {
        gafAsyncKeyState[VK_MBUTTON] |= KS_DOWN_BIT;
        Msg.message = WM_MBUTTONDOWN;
        CurInfo->ButtonsDown |= MK_MBUTTON;
        Msg.wParam |= CurInfo->ButtonsDown;
        co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
    }
    else if(mi->dwFlags & MOUSEEVENTF_MIDDLEUP)
    {
        gafAsyncKeyState[VK_MBUTTON] &= ~KS_DOWN_BIT;
        Msg.message = WM_MBUTTONUP;
        CurInfo->ButtonsDown &= ~MK_MBUTTON;
        Msg.wParam |= CurInfo->ButtonsDown;
        co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
    }
    if(mi->dwFlags & MOUSEEVENTF_RIGHTDOWN)
    {
        gafAsyncKeyState[VK_RBUTTON] |= KS_DOWN_BIT;
        Msg.message = SwapBtnMsg[0][!SwapButtons];
        CurInfo->ButtonsDown |= SwapBtn[!SwapButtons];
        Msg.wParam |= CurInfo->ButtonsDown;
        co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
    }
    else if(mi->dwFlags & MOUSEEVENTF_RIGHTUP)
    {
        gafAsyncKeyState[VK_RBUTTON] &= ~KS_DOWN_BIT;
        Msg.message = SwapBtnMsg[1][!SwapButtons];
        CurInfo->ButtonsDown &= ~SwapBtn[!SwapButtons];
        Msg.wParam |= CurInfo->ButtonsDown;
        co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
    }

    if((mi->dwFlags & (MOUSEEVENTF_XDOWN | MOUSEEVENTF_XUP)) &&
            (mi->dwFlags & MOUSEEVENTF_WHEEL))
    {
        /* fail because both types of events use the mouseData field */
        return FALSE;
    }

    if(mi->dwFlags & MOUSEEVENTF_XDOWN)
    {
        Msg.message = WM_XBUTTONDOWN;
        if(mi->mouseData & XBUTTON1)
        {
            gafAsyncKeyState[VK_XBUTTON1] |= KS_DOWN_BIT;
            CurInfo->ButtonsDown |= MK_XBUTTON1;
            Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON1);
            co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
        }
        if(mi->mouseData & XBUTTON2)
        {
            gafAsyncKeyState[VK_XBUTTON2] |= KS_DOWN_BIT;
            CurInfo->ButtonsDown |= MK_XBUTTON2;
            Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON2);
            co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
        }
    }
    else if(mi->dwFlags & MOUSEEVENTF_XUP)
    {
        Msg.message = WM_XBUTTONUP;
        if(mi->mouseData & XBUTTON1)
        {
            gafAsyncKeyState[VK_XBUTTON1] &= ~KS_DOWN_BIT;
            CurInfo->ButtonsDown &= ~MK_XBUTTON1;
            Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON1);
            co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
        }
        if(mi->mouseData & XBUTTON2)
        {
            gafAsyncKeyState[VK_XBUTTON2] &= ~KS_DOWN_BIT;
            CurInfo->ButtonsDown &= ~MK_XBUTTON2;
            Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON2);
            co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
        }
    }
    if(mi->dwFlags & MOUSEEVENTF_WHEEL)
    {
        Msg.message = WM_MOUSEWHEEL;
        Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, mi->mouseData);
        co_MsqInsertMouseMessage(&Msg, Injected, mi->dwExtraInfo, TRUE);
    }

    return TRUE;
}

BOOL FASTCALL
UserAttachThreadInput(PTHREADINFO pti, PTHREADINFO ptiTo, BOOL fAttach)
{
    PATTACHINFO pai;

    /* Can not be the same thread.*/
    if (pti == ptiTo) return FALSE;

    /* Do not attach to system threads or between different desktops. */
    if ( pti->TIF_flags & TIF_DONTATTACHQUEUE ||
            ptiTo->TIF_flags & TIF_DONTATTACHQUEUE ||
            pti->rpdesk != ptiTo->rpdesk )
        return FALSE;

    /* If Attach set, allocate and link. */
    if ( fAttach )
    {
        pai = ExAllocatePoolWithTag(PagedPool, sizeof(ATTACHINFO), USERTAG_ATTACHINFO);
        if ( !pai ) return FALSE;

        pai->paiNext = gpai;
        pai->pti1 = pti;
        pai->pti2 = ptiTo;
        gpai = pai;
    }
    else /* If clear, unlink and free it. */
    {
        PATTACHINFO paiprev = NULL;

        if ( !gpai ) return FALSE;

        pai = gpai;

        /* Search list and free if found or return false. */
        do
        {
            if ( pai->pti2 == ptiTo && pai->pti1 == pti ) break;
            paiprev = pai;
            pai = pai->paiNext;
        } while (pai);

        if ( !pai ) return FALSE;

        if (paiprev) paiprev->paiNext = pai->paiNext;

        ExFreePoolWithTag(pai, USERTAG_ATTACHINFO);
    }

    return TRUE;
}

UINT
APIENTRY
NtUserSendInput(
    UINT nInputs,
    LPINPUT pInput,
    INT cbSize)
{
    PTHREADINFO pti;
    UINT cnt;
    DECLARE_RETURN(UINT);

    TRACE("Enter NtUserSendInput\n");
    UserEnterExclusive();

    pti = PsGetCurrentThreadWin32Thread();
    ASSERT(pti);

    if (!pti->rpdesk)
    {
        RETURN( 0);
    }

    if (!nInputs || !pInput || cbSize != sizeof(INPUT))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        RETURN( 0);
    }

    /*
     * FIXME - check access rights of the window station
     *         e.g. services running in the service window station cannot block input
     */
    if (!ThreadHasInputAccess(pti) ||
            !IntIsActiveDesktop(pti->rpdesk))
    {
        EngSetLastError(ERROR_ACCESS_DENIED);
        RETURN( 0);
    }

    cnt = 0;
    while (nInputs--)
    {
        INPUT SafeInput;
        NTSTATUS Status;

        Status = MmCopyFromCaller(&SafeInput, pInput++, sizeof(INPUT));
        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            RETURN( cnt);
        }

        switch (SafeInput.type)
        {
            case INPUT_MOUSE:
                if (IntMouseInput(&SafeInput.mi, TRUE))
                    cnt++;
                break;
            case INPUT_KEYBOARD:
                if (UserSendKeyboardInput(&SafeInput.ki, TRUE))
                    cnt++;
                break;
            case INPUT_HARDWARE:
                break;
            default:
                ERR("SendInput(): Invalid input type: 0x%x\n", SafeInput.type);
                break;
        }
    }

    RETURN( cnt);

CLEANUP:
    TRACE("Leave NtUserSendInput, ret=%i\n", _ret_);
    UserLeave();
    END_CLEANUP;
}

BOOL
FASTCALL
IntQueryTrackMouseEvent(
    LPTRACKMOUSEEVENT lpEventTrack)
{
    PDESKTOP pDesk;
    PTHREADINFO pti;

    pti = PsGetCurrentThreadWin32Thread();
    pDesk = pti->rpdesk;

    /* Always cleared with size set and return true. */
    RtlZeroMemory(lpEventTrack , sizeof(TRACKMOUSEEVENT));
    lpEventTrack->cbSize = sizeof(TRACKMOUSEEVENT);

    if ( pDesk->dwDTFlags & (DF_TME_LEAVE | DF_TME_HOVER) &&
            pDesk->spwndTrack &&
            pti->MessageQueue == pDesk->spwndTrack->head.pti->MessageQueue )
    {
        if ( pDesk->htEx != HTCLIENT )
            lpEventTrack->dwFlags |= TME_NONCLIENT;

        if ( pDesk->dwDTFlags & DF_TME_LEAVE )
            lpEventTrack->dwFlags |= TME_LEAVE;

        if ( pDesk->dwDTFlags & DF_TME_HOVER )
        {
            lpEventTrack->dwFlags |= TME_HOVER;
            lpEventTrack->dwHoverTime = pDesk->dwMouseHoverTime;
        }
        lpEventTrack->hwndTrack = UserHMGetHandle(pDesk->spwndTrack);
    }
    return TRUE;
}

BOOL
FASTCALL
IntTrackMouseEvent(
    LPTRACKMOUSEEVENT lpEventTrack)
{
    PDESKTOP pDesk;
    PTHREADINFO pti;
    PWND pWnd;
    POINT point;

    pti = PsGetCurrentThreadWin32Thread();
    pDesk = pti->rpdesk;

    if (!(pWnd = UserGetWindowObject(lpEventTrack->hwndTrack)))
        return FALSE;

    /* Tracking spwndTrack same as pWnd */
    if ( lpEventTrack->dwFlags & TME_CANCEL ) // Canceled mode.
    {
        if ( lpEventTrack->dwFlags & TME_LEAVE )
            pDesk->dwDTFlags &= ~DF_TME_LEAVE;

        if ( lpEventTrack->dwFlags & TME_HOVER )
        {
            if ( pDesk->dwDTFlags & DF_TME_HOVER )
            {   // Kill hover timer.
                IntKillTimer(pWnd, ID_EVENT_SYSTIMER_MOUSEHOVER, TRUE);
                pDesk->dwDTFlags &= ~DF_TME_HOVER;
            }
        }
    }
    else // Not Canceled.
    {
       pDesk->spwndTrack = pWnd;
        if ( lpEventTrack->dwFlags & TME_LEAVE )
            pDesk->dwDTFlags |= DF_TME_LEAVE;

        if ( lpEventTrack->dwFlags & TME_HOVER )
        {
            pDesk->dwDTFlags |= DF_TME_HOVER;

            if ( !lpEventTrack->dwHoverTime || lpEventTrack->dwHoverTime == HOVER_DEFAULT )
                pDesk->dwMouseHoverTime = gspv.iMouseHoverTime; // use the system default hover time-out.
            else
                pDesk->dwMouseHoverTime = lpEventTrack->dwHoverTime;
            // Start timer for the hover period.
            IntSetTimer( pWnd, ID_EVENT_SYSTIMER_MOUSEHOVER, pDesk->dwMouseHoverTime, SystemTimerProc, TMRF_SYSTEM);
            // Get windows thread message points.
            point = pWnd->head.pti->ptLast;
            // Set desktop mouse hover from the system default hover rectangle.
            RECTL_vSetRect(&pDesk->rcMouseHover,
                           point.x - gspv.iMouseHoverWidth  / 2,
                           point.y - gspv.iMouseHoverHeight / 2,
                           point.x + gspv.iMouseHoverWidth  / 2,
                           point.y + gspv.iMouseHoverHeight / 2);
        }
    }
    return TRUE;
}

BOOL
APIENTRY
NtUserTrackMouseEvent(
    LPTRACKMOUSEEVENT lpEventTrack)
{
    TRACKMOUSEEVENT saveTME;
    BOOL Ret = FALSE;

    TRACE("Enter NtUserTrackMouseEvent\n");
    UserEnterExclusive();

    _SEH2_TRY
    {
        ProbeForRead(lpEventTrack, sizeof(TRACKMOUSEEVENT), 1);
        RtlCopyMemory(&saveTME, lpEventTrack, sizeof(TRACKMOUSEEVENT));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(goto Exit;)
    }
    _SEH2_END;

    if ( saveTME.cbSize != sizeof(TRACKMOUSEEVENT) )
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    if (saveTME.dwFlags & ~(TME_CANCEL | TME_QUERY | TME_NONCLIENT | TME_LEAVE | TME_HOVER) )
    {
        EngSetLastError(ERROR_INVALID_FLAGS);
        goto Exit;
    }

    if ( saveTME.dwFlags & TME_QUERY )
    {
        Ret = IntQueryTrackMouseEvent(&saveTME);
        _SEH2_TRY
        {
            ProbeForWrite(lpEventTrack, sizeof(TRACKMOUSEEVENT), 1);
            RtlCopyMemory(lpEventTrack, &saveTME, sizeof(TRACKMOUSEEVENT));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
            Ret = FALSE;
        }
        _SEH2_END;
    }
    else
    {
        Ret = IntTrackMouseEvent(&saveTME);
    }

Exit:
    TRACE("Leave NtUserTrackMouseEvent, ret=%i\n", Ret);
    UserLeave();
    return Ret;
}

extern MOUSEMOVEPOINT MouseHistoryOfMoves[];
extern INT gcur_count;

DWORD
APIENTRY
NtUserGetMouseMovePointsEx(
    UINT cbSize,
    LPMOUSEMOVEPOINT lpptIn,
    LPMOUSEMOVEPOINT lpptOut,
    int nBufPoints,
    DWORD resolution)
{
    MOUSEMOVEPOINT Safeppt;
    //BOOL Hit;
    INT Count = -1;
    DECLARE_RETURN(DWORD);

    TRACE("Enter NtUserGetMouseMovePointsEx\n");
    UserEnterExclusive();

    if ((cbSize != sizeof(MOUSEMOVEPOINT)) || (nBufPoints < 0) || (nBufPoints > 64))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        RETURN( -1);
    }

    if (!lpptIn || (!lpptOut && nBufPoints))
    {
        EngSetLastError(ERROR_NOACCESS);
        RETURN( -1);
    }

    _SEH2_TRY
    {
        ProbeForRead( lpptIn, cbSize, 1);
        RtlCopyMemory(&Safeppt, lpptIn, cbSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(RETURN( -1))
    }
    _SEH2_END;

    // http://msdn.microsoft.com/en-us/library/ms646259(v=vs.85).aspx
    // This explains the math issues in transforming points.
    Count = gcur_count; // FIFO is forward so retrieve backward.
    //Hit = FALSE;
    do
    {
        if (Safeppt.x == 0 && Safeppt.y == 0)
            break; // No test.
        // Finds the point, it returns the last nBufPoints prior to and including the supplied point.
        if (MouseHistoryOfMoves[Count].x == Safeppt.x && MouseHistoryOfMoves[Count].y == Safeppt.y)
        {
            if ( Safeppt.time ) // Now test time and it seems to be absolute.
            {
                if (Safeppt.time == MouseHistoryOfMoves[Count].time)
                {
                    //Hit = TRUE;
                    break;
                }
                else
                {
                    if (--Count < 0) Count = 63;
                    continue;
                }
            }
            //Hit = TRUE;
            break;
        }
        if (--Count < 0) Count = 63;
    }
    while ( Count != gcur_count);

    switch(resolution)
    {
        case GMMP_USE_DISPLAY_POINTS:
            if (nBufPoints)
            {
                _SEH2_TRY
                {
                    ProbeForWrite(lpptOut, cbSize, 1);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    SetLastNtError(_SEH2_GetExceptionCode());
                    _SEH2_YIELD(RETURN( -1))
                }
                _SEH2_END;
            }
            Count = nBufPoints;
            break;
        case GMMP_USE_HIGH_RESOLUTION_POINTS:
            break;
        default:
            EngSetLastError(ERROR_POINT_NOT_FOUND);
            RETURN( -1);
    }

    RETURN( Count);

CLEANUP:
    TRACE("Leave NtUserGetMouseMovePointsEx, ret=%i\n", _ret_);
    UserLeave();
    END_CLEANUP;
}

/* EOF */
