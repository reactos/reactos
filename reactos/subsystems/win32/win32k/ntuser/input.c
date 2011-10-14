/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          General input functions
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
static KEVENT InputThreadsStart;
static BOOLEAN InputThreadsRunning = FALSE;

/* FUNCTIONS *****************************************************************/

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
        Status = ZwOpenFile(&MouseDeviceHandle,
                            FILE_ALL_ACCESS,
                            &MouseObjectAttributes,
                            &Iosb,
                            0,
                            FILE_SYNCHRONOUS_IO_ALERT);
    } while (!NT_SUCCESS(Status));

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
        Status = ZwDeviceIoControlFile(MouseDeviceHandle,
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
            Status = ZwReadFile(MouseDeviceHandle,
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

            UserProcessMouseInput(&MouseInput, Iosb.Information / sizeof(MOUSE_INPUT_DATA));

            UserLeave();
        }
        TRACE("Mouse Input Thread Stopped...\n");
    }
}

VOID NTAPI
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
        DueTime.QuadPart = (LONGLONG)(-100000000);
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, &DueTime);
        Status = ZwOpenFile(&ghKeyboardDevice,
                            FILE_READ_ACCESS,//FILE_ALL_ACCESS,
                            &KeyboardObjectAttributes,
                            &Iosb,
                            0,
                            FILE_SYNCHRONOUS_IO_ALERT);
    } while (!NT_SUCCESS(Status));

    UserInitKeyboard(ghKeyboardDevice);

    ptiKeyboard = PsGetCurrentThreadWin32Thread();
    ptiKeyboard->TIF_flags |= TIF_SYSTEMTHREAD;
    TRACE("Keyboard Thread 0x%x \n", ptiKeyboard);

    KeSetPriorityThread(&PsGetCurrentThread()->Tcb,
                        LOW_REALTIME_PRIORITY + 3);

    for (;;)
    {
        /*
         * Wait to start input.
         */
        TRACE("Keyboard Input Thread Waiting for start event\n");
        Status = KeWaitForSingleObject(&InputThreadsStart,
                                       0,
                                       KernelMode,
                                       TRUE,
                                       NULL);

        TRACE("Keyboard Input Thread Starting...\n");
        /*
         * Receive and process keyboard input.
         */
        while (InputThreadsRunning)
        {
            KEYBOARD_INPUT_DATA KeyInput;

            TRACE("KeyInput @ %08x\n", &KeyInput);

            Status = ZwReadFile (ghKeyboardDevice,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &Iosb,
                                 &KeyInput,
                                 sizeof(KEYBOARD_INPUT_DATA),
                                 NULL,
                                 NULL);

            if (Status == STATUS_ALERTED && !InputThreadsRunning)
            {
                break;
            }
            if (Status == STATUS_PENDING)
            {
                NtWaitForSingleObject(ghKeyboardDevice, FALSE, NULL);
                Status = Iosb.Status;
            }
            if (!NT_SUCCESS(Status))
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

        TRACE("KeyboardInput Thread Stopped...\n");
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
        TRACE("Raw Input Thread Waiting for start event\n");

        Status = KeWaitForMultipleObjects( 2,
                                           Objects,
                                           WaitAll, //WaitAny,
                                           WrUserRequest,
                                           KernelMode,
                                           TRUE,
                                           NULL,
                                           NULL);
        TRACE("Raw Input Thread Starting...\n");

        ProcessTimers();
    }
    ERR("Raw Input Thread Exit!\n");
}

DWORD NTAPI
CreateSystemThreads(UINT Type)
{
    UserLeave();

    switch (Type)
    {
        case 0: KeyboardThreadMain(NULL); break;
        case 1: MouseThreadMain(NULL); break;
        case 2: RawInputThreadMain(NULL); break;
        default: ERR("Wrong type: %x\n", Type);
    }

    UserEnterShared();

    return 0;
}

INIT_FUNCTION
NTSTATUS
NTAPI
InitInputImpl(VOID)
{
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
    BOOL ret;

    TRACE("Enter NtUserBlockInput\n");
    UserEnterExclusive();

    ret = IntBlockInput(PsGetCurrentThreadWin32Thread(), BlockIt);

    UserLeave();
    TRACE("Leave NtUserBlockInput, ret=%i\n", ret);

    return ret;
}

BOOL FASTCALL
UserAttachThreadInput(PTHREADINFO pti, PTHREADINFO ptiTo, BOOL fAttach)
{
    PATTACHINFO pai;

    /* Can not be the same thread.*/
    if (pti == ptiTo) return FALSE;

    /* Do not attach to system threads or between different desktops. */
    if (pti->TIF_flags & TIF_DONTATTACHQUEUE ||
            ptiTo->TIF_flags & TIF_DONTATTACHQUEUE ||
            pti->rpdesk != ptiTo->rpdesk)
        return FALSE;

    /* If Attach set, allocate and link. */
    if (fAttach)
    {
        pai = ExAllocatePoolWithTag(PagedPool, sizeof(ATTACHINFO), USERTAG_ATTACHINFO);
        if (!pai) return FALSE;

        pai->paiNext = gpai;
        pai->pti1 = pti;
        pai->pti2 = ptiTo;
        gpai = pai;
    }
    else /* If clear, unlink and free it. */
    {
        PATTACHINFO paiprev = NULL;

        if (!gpai) return FALSE;

        pai = gpai;

        /* Search list and free if found or return false. */
        do
        {
            if (pai->pti2 == ptiTo && pai->pti1 == pti) break;
            paiprev = pai;
            pai = pai->paiNext;
        } while (pai);

        if (!pai) return FALSE;

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
    UINT uRet = 0;

    TRACE("Enter NtUserSendInput\n");
    UserEnterExclusive();

    pti = PsGetCurrentThreadWin32Thread();
    ASSERT(pti);

    if (!pti->rpdesk)
    {
        goto cleanup;
    }

    if (!nInputs || !pInput || cbSize != sizeof(INPUT))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    /*
     * FIXME - check access rights of the window station
     *         e.g. services running in the service window station cannot block input
     */
    if (!ThreadHasInputAccess(pti) ||
        !IntIsActiveDesktop(pti->rpdesk))
    {
        EngSetLastError(ERROR_ACCESS_DENIED);
        goto cleanup;
    }

    while (nInputs--)
    {
        INPUT SafeInput;
        NTSTATUS Status;

        Status = MmCopyFromCaller(&SafeInput, pInput++, sizeof(INPUT));
        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            goto cleanup;
        }

        switch (SafeInput.type)
        {
            case INPUT_MOUSE:
                if (IntMouseInput(&SafeInput.mi, TRUE))
                    uRet++;
                break;
            case INPUT_KEYBOARD:
                if (UserSendKeyboardInput(&SafeInput.ki, TRUE))
                    uRet++;
                break;
            case INPUT_HARDWARE:
                FIXME("INPUT_HARDWARE not supported!");
                break;
            default:
                ERR("SendInput(): Invalid input type: 0x%x\n", SafeInput.type);
                break;
        }
    }

cleanup:
    TRACE("Leave NtUserSendInput, ret=%i\n", uRet);
    UserLeave();
    return uRet;
}

/* EOF */
