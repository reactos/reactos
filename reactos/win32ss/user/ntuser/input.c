/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          General input functions
 * FILE:             subsystems/win32/win32k/ntuser/input.c
 * PROGRAMERS:       Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                   Rafal Harabien (rafalh@reactos.org)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserInput);

/* GLOBALS *******************************************************************/

PTHREADINFO ptiRawInput;
PTHREADINFO ptiKeyboard;
PTHREADINFO ptiMouse;
PKTIMER MasterTimer = NULL;
PATTACHINFO gpai = NULL;
HANDLE ghKeyboardDevice;

static DWORD LastInputTick = 0;
static HANDLE ghMouseDevice;

/* FUNCTIONS *****************************************************************/

/*
 * IntLastInputTick
 *
 * Updates or gets last input tick count
 */
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

/*
 * DoTheScreenSaver
 *
 * Check if scrensaver should be started and sends message to SAS window
 */
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
                if (ForegroundQueue && ForegroundQueue->spwndActive)
                    UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_START_SCREENSAVE, 1); // lParam 1 == Secure
                else
                    UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_START_SCREENSAVE, 0);
            }
        }
    }
}

/*
 * OpenInputDevice
 *
 * Opens input device for asynchronous access
 */
static
NTSTATUS NTAPI
OpenInputDevice(PHANDLE pHandle, PFILE_OBJECT *ppObject, CONST WCHAR *pszDeviceName)
{
    UNICODE_STRING DeviceName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    RtlInitUnicodeString(&DeviceName, pszDeviceName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               0,
                               NULL,
                               NULL);

    Status = ZwOpenFile(pHandle,
                        FILE_ALL_ACCESS,
                        &ObjectAttributes,
                        &Iosb,
                        0,
                        0);
    if (NT_SUCCESS(Status) && ppObject)
    {
        Status = ObReferenceObjectByHandle(*pHandle, SYNCHRONIZE, NULL, KernelMode, (PVOID*)ppObject, NULL);
        ASSERT(NT_SUCCESS(Status));
    }

    return Status;
}

/*
 * RawInputThreadMain
 *
 * Reads data from input devices and supports win32 timers
 */
VOID NTAPI
RawInputThreadMain()
{
    NTSTATUS MouStatus = STATUS_UNSUCCESSFUL, KbdStatus = STATUS_UNSUCCESSFUL, Status;
    IO_STATUS_BLOCK MouIosb, KbdIosb;
    PFILE_OBJECT pKbdDevice, pMouDevice;
    LARGE_INTEGER ByteOffset;
    //LARGE_INTEGER WaitTimeout;
    PVOID WaitObjects[3], pSignaledObject = NULL;
    ULONG cWaitObjects = 0, cMaxWaitObjects = 1;
    MOUSE_INPUT_DATA MouseInput;
    KEYBOARD_INPUT_DATA KeyInput;

    ByteOffset.QuadPart = (LONGLONG)0;
    //WaitTimeout.QuadPart = (LONGLONG)(-10000000);

    ptiRawInput = GetW32ThreadInfo();
    ptiRawInput->TIF_flags |= TIF_SYSTEMTHREAD;
    ptiRawInput->pClientInfo->dwTIFlags = ptiRawInput->TIF_flags;

    TRACE("Raw Input Thread 0x%x\n", ptiRawInput);

    KeSetPriorityThread(&PsGetCurrentThread()->Tcb,
                        LOW_REALTIME_PRIORITY + 3);

    UserEnterExclusive();
    StartTheTimers();
    UserLeave();

    for(;;)
    {
        if (!ghMouseDevice)
        {
            /* Check if mouse device already exists */
            Status = OpenInputDevice(&ghMouseDevice, &pMouDevice, L"\\Device\\PointerClass0" );
            if (NT_SUCCESS(Status))
            {
                ++cMaxWaitObjects;
                TRACE("Mouse connected!\n");
            }
        }
        if (!ghKeyboardDevice)
        {
            /* Check if keyboard device already exists */
            Status = OpenInputDevice(&ghKeyboardDevice, &pKbdDevice, L"\\Device\\KeyboardClass0");
            if (NT_SUCCESS(Status))
            {
                ++cMaxWaitObjects;
                TRACE("Keyboard connected!\n");
            }
        }

        /* Reset WaitHandles array */
        cWaitObjects = 0;
        WaitObjects[cWaitObjects++] = MasterTimer;

        if (ghMouseDevice)
        {
            /* Try to read from mouse if previous reading is not pending */
            if (MouStatus != STATUS_PENDING)
            {
                MouStatus = ZwReadFile(ghMouseDevice,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &MouIosb,
                                       &MouseInput,
                                       sizeof(MOUSE_INPUT_DATA),
                                       &ByteOffset,
                                       NULL);
            }

            if (MouStatus == STATUS_PENDING)
                WaitObjects[cWaitObjects++] = &pMouDevice->Event;
        }

        if (ghKeyboardDevice)
        {
            /* Try to read from keyboard if previous reading is not pending */
            if (KbdStatus != STATUS_PENDING)
            {
                KbdStatus = ZwReadFile(ghKeyboardDevice,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &KbdIosb,
                                       &KeyInput,
                                       sizeof(KEYBOARD_INPUT_DATA),
                                       &ByteOffset,
                                       NULL);

            }
            if (KbdStatus == STATUS_PENDING)
                WaitObjects[cWaitObjects++] = &pKbdDevice->Event;
        }

        /* If all objects are pending, wait for them */
        if (cWaitObjects == cMaxWaitObjects)
        {
            Status = KeWaitForMultipleObjects(cWaitObjects,
                                              WaitObjects,
                                              WaitAny,
                                              UserRequest,
                                              KernelMode,
                                              TRUE,
                                              NULL,//&WaitTimeout,
                                              NULL);

            if ((Status >= STATUS_WAIT_0) &&
                (Status < (STATUS_WAIT_0 + (LONG)cWaitObjects)))
            {
                /* Some device has finished reading */
                pSignaledObject = WaitObjects[Status - STATUS_WAIT_0];

                /* Check if it is mouse or keyboard and update status */
                if (pSignaledObject == &pMouDevice->Event)
                    MouStatus = MouIosb.Status;
                else if (pSignaledObject == &pKbdDevice->Event)
                    KbdStatus = KbdIosb.Status;
                else if (pSignaledObject == MasterTimer)
                {
                    ProcessTimers();
                }
                else ASSERT(FALSE);
            }
        }

        /* Have we successed reading from mouse? */
        if (NT_SUCCESS(MouStatus) && MouStatus != STATUS_PENDING)
        {
            TRACE("MouseEvent\n");

            /* Set LastInputTick */
            IntLastInputTick(TRUE);

            /* Process data */
            UserEnterExclusive();
            UserProcessMouseInput(&MouseInput);
            UserLeave();
        }
        else if (MouStatus != STATUS_PENDING)
            ERR("Failed to read from mouse: %x.\n", MouStatus);

        /* Have we successed reading from keyboard? */
        if (NT_SUCCESS(KbdStatus) && KbdStatus != STATUS_PENDING)
        {
            TRACE("KeyboardEvent: %s %04x\n",
                  (KeyInput.Flags & KEY_BREAK) ? "up" : "down",
                  KeyInput.MakeCode);

            /* Set LastInputTick */
            IntLastInputTick(TRUE);

            /* Process data */
            UserEnterExclusive();
            UserProcessKeyboardInput(&KeyInput);
            UserLeave();
        }
        else if (KbdStatus != STATUS_PENDING)
            ERR("Failed to read from keyboard: %x.\n", KbdStatus);
    }
    ERR("Raw Input Thread Exit!\n");
}

/*
 * CreateSystemThreads
 *
 * Called form dedicated thread in CSRSS. RIT is started in context of this
 * thread because it needs valid Win32 process with TEB initialized
 */
DWORD NTAPI
CreateSystemThreads(UINT Type)
{
    UserLeave();

    switch (Type)
    {
        case 0: RawInputThreadMain(); break;
        default: ERR("Wrong type: %x\n", Type);
    }

    UserEnterShared();

    return 0;
}

/*
 * InitInputImpl
 *
 * Inits input implementation
 */
INIT_FUNCTION
NTSTATUS
NTAPI
InitInputImpl(VOID)
{
    MasterTimer = ExAllocatePoolWithTag(NonPagedPool, sizeof(KTIMER), USERTAG_SYSTEM);
    if (!MasterTimer)
    {
        ERR("Failed to allocate memory\n");
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }
    KeInitializeTimer(MasterTimer);

    return STATUS_SUCCESS;
}

BOOL FASTCALL
IntBlockInput(PTHREADINFO pti, BOOL BlockIt)
{
    PTHREADINFO OldBlock;
    ASSERT(pti);

    if(!pti->rpdesk || ((pti->TIF_flags & TIF_INCLEANUP) && BlockIt))
    {
        /*
         * Fail blocking if exiting the thread
         */

        return FALSE;
    }

    /*
     * FIXME: Check access rights of the window station
     *        e.g. services running in the service window station cannot block input
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

PTHREADINFO FASTCALL
IsThreadAttach(PTHREADINFO ptiTo)
{
    PATTACHINFO pai;

    if (!gpai) return NULL;

    pai = gpai;
    do
    {
        if (pai->pti2 == ptiTo) break;
        pai = pai->paiNext;
    } while (pai);

    if (!pai) return NULL;

    // Return ptiFrom.
    return pai->pti1;
}

BOOL FASTCALL
UserAttachThreadInput(PTHREADINFO ptiFrom, PTHREADINFO ptiTo, BOOL fAttach)
{
    MSG msg;
    PATTACHINFO pai;

    /* Can not be the same thread. */
    if (ptiFrom == ptiTo) return FALSE;

    /* Do not attach to system threads or between different desktops. */
    if (ptiFrom->TIF_flags & TIF_DONTATTACHQUEUE ||
            ptiTo->TIF_flags & TIF_DONTATTACHQUEUE ||
            ptiFrom->rpdesk != ptiTo->rpdesk)
        return FALSE;

    /* MSDN Note:
       Keyboard and mouse events received by both threads are processed by the thread specified by the idAttachTo.
     */

    /* If Attach set, allocate and link. */
    if (fAttach)
    {
        pai = ExAllocatePoolWithTag(PagedPool, sizeof(ATTACHINFO), USERTAG_ATTACHINFO);
        if (!pai) return FALSE;

        pai->paiNext = gpai;
        pai->pti1 = ptiFrom;
        pai->pti2 = ptiTo;
        gpai = pai;
        ERR("Attach Allocated! ptiFrom 0x%p  ptiTo 0x%p\n",ptiFrom,ptiTo);

        ptiTo->MessageQueue->iCursorLevel -= ptiFrom->iCursorLevel;
        ptiFrom->pqAttach = ptiFrom->MessageQueue;
        ptiFrom->MessageQueue = ptiTo->MessageQueue;
        // FIXME: conditions?
        if (ptiFrom->pqAttach == gpqForeground)
        {
        ptiFrom->MessageQueue->spwndActive = ptiFrom->pqAttach->spwndActive;
        ptiFrom->MessageQueue->spwndFocus = ptiFrom->pqAttach->spwndFocus;
        ptiFrom->MessageQueue->CursorObject = ptiFrom->pqAttach->CursorObject;
        ptiFrom->MessageQueue->CaptureWindow = ptiFrom->pqAttach->CaptureWindow;
        ptiFrom->MessageQueue->spwndCapture = ptiFrom->pqAttach->spwndCapture;
        ptiFrom->MessageQueue->QF_flags ^= ((ptiFrom->MessageQueue->QF_flags ^ ptiFrom->pqAttach->QF_flags) & QF_CAPTURELOCKED);
        ptiFrom->MessageQueue->CaretInfo = ptiFrom->pqAttach->CaretInfo;
        }
    }
    else /* If clear, unlink and free it. */
    {
        PATTACHINFO paiprev = NULL;

        if (!gpai) return FALSE;

        pai = gpai;

        /* Search list and free if found or return false. */
        do
        {
            if (pai->pti2 == ptiTo && pai->pti1 == ptiFrom) break;
            paiprev = pai;
            pai = pai->paiNext;
        } while (pai);

        if (!pai) return FALSE;

        if (paiprev) paiprev->paiNext = pai->paiNext;
        else if (!pai->paiNext) gpai = NULL;

        ExFreePoolWithTag(pai, USERTAG_ATTACHINFO);
        ERR("Attach Free! ptiFrom 0x%p  ptiTo 0x%p\n",ptiFrom,ptiTo);

        ptiFrom->MessageQueue = ptiFrom->pqAttach;
        // FIXME: conditions?
        ptiFrom->MessageQueue->CursorObject = NULL;
        ptiFrom->MessageQueue->spwndActive = NULL;
        ptiFrom->MessageQueue->spwndFocus = NULL;
        ptiFrom->pqAttach = NULL;
        ptiTo->MessageQueue->iCursorLevel -= ptiFrom->iCursorLevel;
    }
    /* Note that key state, which can be ascertained by calls to the GetKeyState
       or GetKeyboardState function, is reset after a call to AttachThreadInput.
       ATM which one?
     */
    RtlCopyMemory(ptiTo->MessageQueue->afKeyState, gafAsyncKeyState, sizeof(gafAsyncKeyState));

    /* Generate mouse move message */
    msg.message = WM_MOUSEMOVE;
    msg.wParam = UserGetMouseButtonsState();
    msg.lParam = MAKELPARAM(gpsi->ptCursor.x, gpsi->ptCursor.y);
    msg.pt = gpsi->ptCursor;
    co_MsqInsertMouseMessage(&msg, 0, 0, TRUE);

    return TRUE;
}

/*
 * NtUserSendInput
 *
 * Generates input events from software
 */
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
     * FIXME: Check access rights of the window station
     *        e.g. services running in the service window station cannot block input
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
                if (UserSendMouseInput(&SafeInput.mi, TRUE))
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
