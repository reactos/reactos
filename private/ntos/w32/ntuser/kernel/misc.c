/****************************** Module Header ******************************\
* Module Name: misc.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains citrix code.
*
\***************************************************************************/


#include "precomp.h"
#pragma hdrstop

/*
 * All of the following are gotten from ICASRV
 */

CACHE_STATISTICS ThinWireCache;

NTSTATUS RemoteConnect(
    IN PDOCONNECTDATA pDoConnectData,
    IN ULONG DisplayDriverNameLength,
    IN PWCHAR DisplayDriverName)
{
    NTSTATUS          Status = STATUS_SUCCESS;
    PFILE_OBJECT      pFileObject = NULL;
    PDEVICE_OBJECT    pDeviceObject = NULL;
    PWCHAR            pSep;

    TRACE_HYDAPI(("RemoteConnect: display %ws\n", DisplayDriverName));

    HYDRA_HINT(HH_REMOTECONNECT);

    UserAssert(ISCSRSS());

    gpThinWireCache = &ThinWireCache;

    if (pDoConnectData->fMouse) {
        ghRemoteMouseChannel = pDoConnectData->IcaMouseChannel;
    } else {
        ghRemoteMouseChannel = NULL;
    }

    ghRemoteVideoChannel = pDoConnectData->IcaVideoChannel;
    ghRemoteBeepChannel = pDoConnectData->IcaBeepChannel;
    ghRemoteKeyboardChannel = pDoConnectData->IcaKeyboardChannel;
    ghRemoteThinwireChannel = pDoConnectData->IcaThinwireChannel;

    gRemoteClientKeyboardType = pDoConnectData->ClientKeyboardType;

    gbClientDoubleClickSupport = pDoConnectData->fClientDoubleClickSupport;

    gfEnableWindowsKey = pDoConnectData->fEnableWindowsKey;

    RtlCopyMemory(gWinStationInfo.ProtocolName, pDoConnectData->ProtocolName,
                  WPROTOCOLNAME_LENGTH * sizeof(WCHAR));

    RtlCopyMemory(gWinStationInfo.AudioDriverName, pDoConnectData->AudioDriverName,
                  WAUDIONAME_LENGTH * sizeof(WCHAR));

    RtlZeroMemory(gstrBaseWinStationName,
                  WINSTATIONNAME_LENGTH * sizeof(WCHAR));

    RtlCopyMemory(gstrBaseWinStationName, pDoConnectData->WinStationName,
                  min(WINSTATIONNAME_LENGTH * sizeof(WCHAR), sizeof(pDoConnectData->WinStationName)));

    if (pSep = wcschr(gstrBaseWinStationName, L'#'))
        *pSep = UNICODE_NULL;

    /*
     * Create the event that is signaled when a desktop does away
     */
    gpevtDesktopDestroyed = CreateKernelEvent(SynchronizationEvent, FALSE);
    if (gpevtDesktopDestroyed == NULL) {
        RIPMSG0(RIP_WARNING, "RemoteConnect couldn't create gpevtDesktopDestroyed");
        return STATUS_NO_MEMORY;
    }

    gbConnected = TRUE;

    /*
     * WinStations must have the video device handle passed to them.
     */
    if (gbRemoteSession && !gVideoFileObject) {

        PFILE_OBJECT    pFileObject = NULL;
        PDEVICE_OBJECT  pDeviceObject = NULL;

        //
        // Dereference the file handle
        // and obtain a pointer to the device object for the handle.
        //

        Status = ObReferenceObjectByHandle(ghRemoteVideoChannel,
                                           0,
                                           NULL,
                                           KernelMode,
                                           (PVOID*)&pFileObject,
                                           NULL);
        if (NT_SUCCESS(Status)) {

            gVideoFileObject = pFileObject;

            //
            // Get a pointer to the device object for this file.
            //
            pDeviceObject = IoGetRelatedDeviceObject(pFileObject);

            Status = ObReferenceObjectByHandle(ghRemoteThinwireChannel,
                                               0,
                                               NULL,
                                               KernelMode,
                                               (PVOID*)&gThinwireFileObject,
                                               NULL);

            /*
             * This must be done before any thinwire data.
             */
            if (NT_SUCCESS(Status)) {

                if (!GreMultiUserInitSession(ghRemoteThinwireChannel,
                                             (PBYTE)gpThinWireCache,
                                             gVideoFileObject,
                                             gThinwireFileObject,
                                             DisplayDriverNameLength,
                                             DisplayDriverName)) {
                    RIPMSG0(RIP_WARNING, "UserInit: GreMultiUserInitSession failed");
                    Status = STATUS_UNSUCCESSFUL;
                } else {

                    DWORD BytesReturned;

                    Status = GreDeviceIoControl( pDeviceObject,
                                 IOCTL_VIDEO_ICA_ENABLE_GRAPHICS,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 &BytesReturned);
                    if (!NT_SUCCESS(Status)) {
                        RIPMSG1(RIP_WARNING, "UserInit: Enable graphics status %08lx",
                                Status);
                    }
                }
            }
        }
    }

    if (!NT_SUCCESS(Status)) {
        RIPMSG0(RIP_WARNING, "RemoteConnect failed");
        return Status;
    }

    if (InitVideo(FALSE) == NULL) {
        gbConnected = FALSE;
        RIPMSG0(RIP_WARNING, "InitVideo failed");
        return STATUS_UNSUCCESSFUL;
    }

    if (!LW_BrushInit()) {
        RIPMSG0(RIP_WARNING, "LW_BrushInit failed");
        return STATUS_NO_MEMORY;
    }

    InitLoadResources();

    Status = ObReferenceObjectByHandle(ghRemoteBeepChannel,
                                       0,
                                       NULL,
                                       KernelMode,
                                       (PVOID*)&gpRemoteBeepDevice,
                                       NULL);

    /*
     * video is initialized at this point
     */
    if (NT_SUCCESS(Status)) {
        gbVideoInitialized = TRUE;
    } else {
        RIPMSG0(RIP_WARNING, "Bad Remote Beep Channel");
    }

    return Status;
}


NTSTATUS
xxxRemoteDisconnect(
    VOID)
{
    NTSTATUS      Status = STATUS_SUCCESS;
    LARGE_INTEGER li;

    TRACE_HYDAPI(("xxxRemoteDisconnect\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    HYDRA_HINT(HH_REMOTEDISCONNECT);

    RtlZeroMemory(gstrBaseWinStationName,
                  WINSTATIONNAME_LENGTH * sizeof(WCHAR));

    UserAssert(gbConnected);

    if (gspdeskDisconnect == NULL) {
        /*
         * Convert dwMilliseconds to a relative-time(i.e.  negative)
         * LARGE_INTEGER.  NT Base calls take time values in 100 nanosecond
         * units. Timeout after 5 minutes.
         */
        li.QuadPart = Int32x32To64(-10000, 300000);

        KeWaitForSingleObject(gpEventDiconnectDesktop,
                              WrUserRequest,
                              KernelMode,
                              FALSE,
                              &li);
    }


    /*
     * If the disconnected desktop has not yet be setup.  Do not do any
     * disconnect processing.  It's better for the thinwire driver to try
     * and write rather than for the transmit buffers to be freed (trap).
     */
    if (gspdeskDisconnect) {

        /*
         * Blank the screen
         *
         * No need to stop graphics mode for disconnects
         */
        Status = xxxRemoteStopScreenUpdates(FALSE);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        /*
         * If there are any shadow connections, then redraw the screen now.
         */
        if (gnShadowers)
            RemoteRedrawScreen();
    } else {
        RIPMSG0(RIP_WARNING, "xxxRemoteDisconnect failed. The disconnect desktop was not created");
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * Tell thinwire driver about this
     */
    bDrvDisconnect(gpDispInfo->hDev, ghRemoteThinwireChannel,
                   gThinwireFileObject);

    
    /*
     * Tell winlogon about the disconnect
     */
    if (gspwndLogonNotify != NULL) {
        _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY, SESSION_DISCONNECTED, 0);
    }

    gbConnected = FALSE;

    return Status;
}

NTSTATUS
xxxRemoteReconnect(
    IN PDORECONNECTDATA pDoReconnectData)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL     fResult;
    PWCHAR   pSep;

    TRACE_HYDAPI(("xxxRemoteReconnect\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    HYDRA_HINT(HH_REMOTERECONNECT);

    UserAssert(ISCSRSS());

    gRemoteClientKeyboardType = pDoReconnectData->ClientKeyboardType;

    gbClientDoubleClickSupport = pDoReconnectData->fClientDoubleClickSupport;

    gfEnableWindowsKey = pDoReconnectData->fEnableWindowsKey;

    RtlCopyMemory(gstrBaseWinStationName, pDoReconnectData->WinStationName,
                  min(WINSTATIONNAME_LENGTH * sizeof(WCHAR), sizeof(pDoReconnectData->WinStationName)));

    if (pSep = wcschr(gstrBaseWinStationName, L'#'))
        *pSep = UNICODE_NULL;

    if (gnShadowers)
        xxxRemoteStopScreenUpdates(TRUE);


    /*
     * Call thinwire driver to check for thinwire mode compatibility
     */
    fResult = bDrvReconnect(gpDispInfo->hDev, ghRemoteThinwireChannel,
                            gThinwireFileObject);

    if (!fResult) {
        if (gnShadowers)
            RemoteRedrawScreen();

        return STATUS_CTX_BAD_VIDEO_MODE;
    }



    /*
     *  If instructed to do so, change Display mode before Reconnecting.
     *  Use display resolution information from Reconnect data.
     */

    if (pDoReconnectData->fChangeDisplaySettings) {
       DEVMODEW devmodew;
       UNICODE_STRING ustrDeviceName;
       PDEVMODEW pDevmodew = &devmodew;

       RtlZeroMemory(pDevmodew, sizeof(DEVMODEW));
       pDevmodew->dmPelsHeight = pDoReconnectData->drPelsHeight;
       pDevmodew->dmPelsWidth  = pDoReconnectData->drPelsWidth;
       pDevmodew->dmBitsPerPel = pDoReconnectData->drBitsPerPel;
       pDevmodew->dmFields     = DM_BITSPERPEL  | DM_PELSWIDTH  | DM_PELSHEIGHT;
       pDevmodew->dmSize       = sizeof(DEVMODEW);

       RtlInitUnicodeString(&ustrDeviceName, L"\\\\.\\DISPLAY1");


       Status = xxxUserChangeDisplaySettings(NULL, //&ustrDeviceName,
                                            &devmodew,
                                            NULL,
                                            grpdeskRitInput,
                                            0,
                                            NULL,
                                            KernelMode);
       /*
        * If Display settings change fails, let us disconnect the display driver
        * as the reconnect is going to be failed anyway.
        */


       if (!NT_SUCCESS(Status)) {
          bDrvDisconnect(gpDispInfo->hDev, ghRemoteThinwireChannel, gThinwireFileObject);
          return Status;
       }

       /*
        * Code in RemoteRedrawScreen() Expects gpDispInfo->pmdev to disabled, but
        * it has been enabled by xxxUserChangeDisplaySettings(), so let's disable it
        * before calling RemoteRedrawScreen().
        */
       DrvDisableMDEV(gpDispInfo->pmdev, TRUE);

    }

    /*
     * Now we can swicth out from disconnected desktop, to normal
     * desktop, in order to renable screen update.
     */

    RemoteRedrawScreen();

    if (gspwndLogonNotify != NULL) {
        _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY,
                     SESSION_RECONNECTED, 0);
    }

    /*
     * We need to re-init the mouse, especially since we may not have had one before.
     */
    UpdateMouseInfo();

    /*
     * Re-init'ing the keyboard may not be as neccessary.  Possibly the keyboard
     * attributes have changed.
     */
    InitKeyboard();

    /*
     * This is neccessary to sync up the client and the host.
     */
    UpdateKeyLights(FALSE);

    SetPointer(TRUE);

    gbConnected = TRUE;



    return Status;
}

/*
 * This allows ICASRV to cleanly logoff the user.  We send a message to winlogon and let
 * him do it.  We used to call ExitWindowsEx() directly, but that caused too many problems
 * when it was called from CSRSS.
 *
 */
NTSTATUS
RemoteLogoff(
    VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE_HYDAPI(("RemoteLogoff\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    HYDRA_HINT(HH_REMOTELOGOFF);

    UserAssert(ISCSRSS());

    /*
     * Tell winlogon about the logoff
     */
    if (gspwndLogonNotify != NULL) {
        _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY,
                     SESSION_LOGOFF, EWX_LOGOFF | EWX_FORCE);
    }

    return Status;
}


NTSTATUS
xxxRemoteStopScreenUpdates(
    BOOL fDisableGraphics)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SaveStatus = STATUS_SUCCESS;
    //ULONG BytesReturned;
    WORD NewButtonState;

    TRACE_HYDAPI(("xxxRemoteStopScreenUpdates fDisableGraphics %d\n", fDisableGraphics));

    CheckCritIn();

    UserAssert(ISCSRSS());

    /*
     * No need to do this multiple times.
     */
    if (gbFreezeScreenUpdates)
        return STATUS_SUCCESS;

    /*
     * This could be called directly from the command channel.
     */
    if (!gspdeskDisconnect)
        return STATUS_SUCCESS;

    /*
     * If not connected, forget it
     */
    if (ghRemoteVideoChannel == NULL)
        return STATUS_NO_SUCH_DEVICE;

    /*
     * Mouse buttons up.
     * (Ensures no mouse buttons are left in a down state)
     */
    NewButtonState = gwMKButtonState & ~gwMKCurrentButton;

    if ((NewButtonState & MOUSE_BUTTON_LEFT) != (gwMKButtonState & MOUSE_BUTTON_LEFT)) {
        xxxButtonEvent(MOUSE_BUTTON_LEFT, gptCursorAsync, TRUE, NtGetTickCount(),
                    0L, 0L, FALSE);
    }

    if ((NewButtonState & MOUSE_BUTTON_RIGHT) != (gwMKButtonState & MOUSE_BUTTON_RIGHT)) {
        xxxButtonEvent(MOUSE_BUTTON_RIGHT, gptCursorAsync, TRUE, NtGetTickCount(),
                    0L, 0L, FALSE);
    }
    gwMKButtonState = NewButtonState;

    /*
     * Send shift key breaks to win32
     * (Ensures no shift keys are left on)
     */

    // { 0, 0xb8, KEY_BREAK, 0, 0 },           // L alt
    xxxPushKeyEvent(VK_LMENU, 0xb8, KEYEVENTF_KEYUP, 0);

    // { 0, 0xb8, KEY_BREAK | KEY_E0, 0, 0 },  // R alt
    xxxPushKeyEvent(VK_RMENU, 0xb8, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);

    // { 0, 0x9d, KEY_BREAK, 0, 0 },           // L ctrl
    xxxPushKeyEvent(VK_LCONTROL, 0x9d, KEYEVENTF_KEYUP, 0);

    // { 0, 0x9d, KEY_BREAK | KEY_E0, 0, 0 },  // R ctrl
    xxxPushKeyEvent(VK_RCONTROL, 0x9d, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);

    // { 0, 0xaa, KEY_BREAK, 0, 0 },           // L shift
    xxxPushKeyEvent(VK_LSHIFT, 0xaa, KEYEVENTF_KEYUP, 0);

    // { 0, 0xb6, KEY_BREAK, 0, 0 }            // R shift
    xxxPushKeyEvent(VK_RSHIFT, 0xb6, KEYEVENTF_KEYUP, 0);

    Status = RemoteDisableScreen();

    if (!NT_SUCCESS(Status)) {
       return STATUS_NO_SUCH_DEVICE;
    }

    UserAssert(gspdeskDisconnect != NULL && grpdeskRitInput == gspdeskDisconnect);

    gbFreezeScreenUpdates = TRUE;

    return Status;
    UNREFERENCED_PARAMETER(fDisableGraphics);
}

/*
 * Taken from Internal Key Event.
 * Minus any permissions checking.
 */
VOID xxxPushKeyEvent(
    BYTE  bVk,
    BYTE  bScan,
    DWORD dwFlags,
    DWORD dwExtraInfo)
{
    USHORT usFlaggedVK;

    usFlaggedVK = (USHORT)bVk;

    if (dwFlags & KEYEVENTF_KEYUP)
        usFlaggedVK |= KBDBREAK;

    // IanJa: not all extended keys are numpad, but this seems to work.
    if (dwFlags & KEYEVENTF_EXTENDEDKEY)
        usFlaggedVK |= KBDNUMPAD | KBDEXT;

    xxxKeyEvent(usFlaggedVK, bScan, NtGetTickCount(), dwExtraInfo, FALSE);
}


NTSTATUS
RemoteThinwireStats(
    OUT PVOID Stats)
{
static DWORD sThinwireStatsLength = sizeof(CACHE_STATISTICS);

    TRACE_HYDAPI(("RemoteThinwireStats\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(ISCSRSS());
    if (gpThinWireCache != NULL) {
        RtlCopyMemory(Stats, gpThinWireCache, sThinwireStatsLength);
        return STATUS_SUCCESS;
    }
    return STATUS_NO_SUCH_DEVICE;
}


NTSTATUS
RemoteNtSecurity(
    VOID)
{
    TRACE_HYDAPI(("RemoteNtSecurity\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(ISCSRSS());

    UserAssert(gspwndLogonNotify != NULL);

    if (gspwndLogonNotify != NULL) {
        _PostMessage(gspwndLogonNotify, WM_HOTKEY, 0, 0);
    }
    return STATUS_SUCCESS;
}


NTSTATUS
xxxRemoteShadowSetup(
    VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE_HYDAPI(("xxxRemoteShadowSetup\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(ISCSRSS());

    /*
     * Blank the screen
     */
    if (gnShadowers || gbConnected)
        xxxRemoteStopScreenUpdates(FALSE);

    gnShadowers++;

    return Status;
}


NTSTATUS
RemoteShadowStart(
    IN PVOID pThinwireData,
    ULONG ThinwireDataLength)
{
    BOOL fResult;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE_HYDAPI(("RemoteShadowStart\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(ISCSRSS());

    /*
     * Call thinwire driver and check for thinwire mode compatibility
     */
    fResult = bDrvShadowConnect(gpDispInfo->hDev, pThinwireData,
                                ThinwireDataLength);

    // Although originally defined as BOOL, allow more meaningful return codes.
    if (!fResult) {
        return STATUS_CTX_BAD_VIDEO_MODE;
    }
    else if (fResult != TRUE) {
        return fResult;
    }

    RemoteRedrawScreen();

    return Status;
}


NTSTATUS
xxxRemoteShadowStop(
    VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE_HYDAPI(("xxxRemoteShadowStop\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(ISCSRSS());

    /*
     * Blank the screen
     */
    xxxRemoteStopScreenUpdates(FALSE);

    return Status;
}


NTSTATUS
RemoteShadowCleanup(
    IN PVOID pThinwireData,
    ULONG ThinwireDataLength)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE_HYDAPI(("RemoteShadowCleanup\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(ISCSRSS());

    /*
     * Tell the Thinwire driver about it
     */
    bDrvShadowDisconnect(gpDispInfo->hDev, pThinwireData,
                         ThinwireDataLength);

    if (gnShadowers > 0)
        gnShadowers--;

    if (gnShadowers || gbConnected) {
        RemoteRedrawScreen();
    }
    
    return Status;
}


NTSTATUS
xxxRemotePassthruEnable(
    VOID)
{
    IO_STATUS_BLOCK IoStatus;
static BOOL KeyboardType101;

    TRACE_HYDAPI(("xxxRemotePassthruEnable\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(gbConnected);
    UserAssert(gnShadowers == 0);
    UserAssert(ISCSRSS());

    KeyboardType101 = !(gapulCvt_VK == gapulCvt_VK_84);

    ZwDeviceIoControlFile(ghRemoteKeyboardChannel, NULL, NULL, NULL,
                          &IoStatus, IOCTL_KEYBOARD_ICA_TYPE,
                          &KeyboardType101, sizeof(KeyboardType101),
                          NULL, 0);

    if (guKbdTblSize != 0) {
        ZwDeviceIoControlFile(ghRemoteKeyboardChannel, NULL, NULL, NULL,
                              &IoStatus, IOCTL_KEYBOARD_ICA_LAYOUT,
                              ghKbdTblBase, guKbdTblSize,
                              gpKbdTbl, 0);
    }

    xxxRemoteStopScreenUpdates(FALSE);

    /*
     * Tell thinwire driver about this
     */
    bDrvDisconnect(gpDispInfo->hDev, ghRemoteThinwireChannel,
                   gThinwireFileObject);

    return STATUS_SUCCESS;
}


NTSTATUS
RemotePassthruDisable(
    VOID)
{
    BOOL fResult;

    TRACE_HYDAPI(("RemotePassthruDisable\n"));

    /*
     * Only allow CSRSS to do this
     */
    if (!ISCSRSS() || !ISTS()) {
        return STATUS_ACCESS_DENIED;
    }

    UserAssert(gnShadowers == 0);
    UserAssert(ISCSRSS());

    fResult = bDrvReconnect(gpDispInfo->hDev, ghRemoteThinwireChannel,
                            gThinwireFileObject);
    if (!fResult) {
        return STATUS_CTX_BAD_VIDEO_MODE;
    }

    if (gbConnected) {
        RemoteRedrawScreen();
        UpdateKeyLights(FALSE); // Make sure LED's are correct
    }

    return STATUS_SUCCESS;
}


NTSTATUS
CtxDisplayIOCtl(
    ULONG  DisplayIOCtlFlags,
    PUCHAR pDisplayIOCtlData,
    ULONG  cbDisplayIOCtlData)
{
    BOOL fResult;

    TRACE_HYDAPI(("CtxDisplayIOCtl\n"));

    fResult = bDrvDisplayIOCtl(gpDispInfo->hDev, pDisplayIOCtlData, cbDisplayIOCtlData);

    if (!fResult) {
        return STATUS_CTX_BAD_VIDEO_MODE;
    }

    if ((DisplayIOCtlFlags & DISPLAY_IOCTL_FLAG_REDRAW)) {
        RemoteRedrawRectangle(0,0,0xffff,0xffff);
    }

    return STATUS_SUCCESS;
}


/*
 * This is for things like user32.dll init routines that don't want to use
 * winsta.dll for queries.
 *
 */
DWORD
RemoteConnectState(
    VOID)
{
    DWORD state = 0;

    if (!gbRemoteSession) {

        state = CTX_W32_CONNECT_STATE_CONSOLE;

    } else if (!gbVideoInitialized) {

        state = CTX_W32_CONNECT_STATE_IDLE;

    } else if (gbExitInProgress) {

        state = CTX_W32_CONNECT_STATE_EXIT_IN_PROGRESS;

    } else if (gbConnected) {

        state = CTX_W32_CONNECT_STATE_CONNECTED;

    } else {
        state = CTX_W32_CONNECT_STATE_DISCONNECTED;
    }

    return state;
}

BOOL
_GetWinStationInfo(
    WSINFO* pWsInfo)
{
    CheckCritIn();

    try {

        ProbeForWrite(pWsInfo, sizeof(gWinStationInfo), DATAALIGN);
        RtlCopyMemory(pWsInfo, &gWinStationInfo, sizeof(gWinStationInfo));

    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        return FALSE;
    }

    return TRUE;
}
