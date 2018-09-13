
/*************************************************************************
*
* termutil.c
*
* Terminal Server support utilities
*
* copyright notice: Copyright 1997, Microsoft
*
*
*************************************************************************/

#include "precomp.h"
#pragma hdrstop


/****************************************************************************\
*
* FUNCTION: InitializeMultiUserFunctions
*
* PURPOSE:  Load Winsta.dll and store function pointers
*
* HISTORY:
*
*
\****************************************************************************/
BOOL InitializeMultiUserFunctionsPtrs (void)
{

    HANDLE          dllHandle;

    //
    // Load winsta.dll
    //
    dllHandle = LoadLibraryW(L"winsta.dll");
    if (dllHandle == NULL) {
        return FALSE;
    }


    //_WinStationCallback
    gpfnWinStationCallback = (PWINSTATION_CALLBACK) GetProcAddress(
                                                            dllHandle,
                                                            "_WinStationCallback"
                                                            );
    if (gpfnWinStationCallback == NULL) {
        KdPrint(("InitializeMultiUserFunctions: Failed to get _WinStationCallback Proc %d\n",GetLastError()));
        FreeLibrary(dllHandle);
        return FALSE;
    }


    //WinStationDisconnect
    gpfnWinStationDisconnect = (PWINSTATION_DISCONNECT) GetProcAddress(
                                                            dllHandle,
                                                            "WinStationDisconnect"
                                                            );
    if (gpfnWinStationDisconnect == NULL) {
        KdPrint(("InitializeMultiUserFunctions: Failed to get WinStationDisconnect Proc %d\n",GetLastError()));
        FreeLibrary(dllHandle);
        return FALSE;
    }


    //WinStationConnectW
     gpfnWinStationConnect = (PWINSTATION_CONNECT) GetProcAddress(
                                                            dllHandle,
                                                            "WinStationConnectW"
                                                            );
    if (gpfnWinStationConnect == NULL) {
        KdPrint(("InitializeMultiUserFunctions: Failed to get WinStationConnectW Proc %d\n",GetLastError()));
        FreeLibrary(dllHandle);
        return FALSE;
    }


    //WinStationQueryInformationW
     gpfnWinStationQueryInformation = (PWINSTATION_QUERY_INFORMATION) GetProcAddress(
                                        dllHandle,
                                        "WinStationQueryInformationW"
                                        );
    if (gpfnWinStationQueryInformation == NULL) {
        KdPrint(("InitializeMultiUserFunctions: Failed to get WinStationQueryInformationW Proc %d\n",GetLastError()));
        FreeLibrary(dllHandle);
        return FALSE;
    }


    //ServerQueryInetConnectorInformationW
     gpfnServerQueryInetConnectorInformation = (PSERVER_QUERY_IC_INFORMATION) GetProcAddress(
                                                        dllHandle,
                                                        "ServerQueryInetConnectorInformationW"
                                                        );
    if (gpfnServerQueryInetConnectorInformation == NULL) {
        KdPrint(("InitializeMultiUserFunctions: Failed to get ServerQueryInetConnectorInformationW Proc %d\n",GetLastError()));
    }


    //WinStationEnumerate_IndexedW
    gpfnWinStationEnumerate_Indexed = (PWINSTATION_ENUMERATE_INDEXED) GetProcAddress(
                                                                        dllHandle,
                                                                        "WinStationEnumerate_IndexedW"
                                                                        );
    if (gpfnWinStationEnumerate_Indexed == NULL) {
        KdPrint(("InitializeMultiUserFunctions: Failed to get WinStationEnumerate_IndexedW Proc %d\n",GetLastError()));
        FreeLibrary(dllHandle);
        return FALSE;
    }


    //WinStationNameFromLogonIdW
     gpfnWinStationNameFromSessionId = (PWINSTATION_NAME_FROM_SESSIONID) GetProcAddress(
                                                            dllHandle,
                                                            "WinStationNameFromLogonIdW"
                                                            );
    if (gpfnWinStationNameFromSessionId == NULL) {
        KdPrint(("InitializeMultiUserFunctions: Failed to get WinStationNameFromLogonIdW Proc %d\n",GetLastError()));
        FreeLibrary(dllHandle);
        return FALSE;
    }


    //WinStationShutdownSystem
     gpfnWinStationShutdownSystem = (PWINSTATION_SHUTDOWN_SYSTEM) GetProcAddress(
                                                            dllHandle,
                                                            "WinStationShutdownSystem"
                                                            );
    if (gpfnWinStationShutdownSystem == NULL) {
        KdPrint(("InitializeMultiUserFunctions: Failed to get WinStationShutdownSystem Proc %d\n",GetLastError()));
        FreeLibrary(dllHandle);
        return FALSE;
    }


    //_WinStationWaitForConnect
     gpfnWinStationWaitForConnect = (PWINSTATION_WAIT_FOR_CONNECT) GetProcAddress(
                                                            dllHandle,
                                                            "_WinStationWaitForConnect"
                                                            );
    if (gpfnWinStationWaitForConnect == NULL) {
        KdPrint(("InitializeMultiUserFunctions: Failed to get _WinStationWaitForConnect Proc %d\n",GetLastError()));
        FreeLibrary(dllHandle);
        return FALSE;
    }


    //WinStationSetInformationW
     gpfnWinStationSetInformation = (PWINSTATION_SET_INFORMATION) GetProcAddress(
                                                            dllHandle,
                                                            "WinStationSetInformationW"
                                                            );
    if (gpfnWinStationSetInformation == NULL) {
        KdPrint(("InitializeMultiUserFunctions: Failed to get WinStationSetInformationW Proc %d\n",GetLastError()));
        FreeLibrary(dllHandle);
        return FALSE;
    }


    //_WinStationNotifyLogon
     gpfnWinStationNotifyLogon = (PWINSTATION_NOTIFY_LOGON) GetProcAddress(
                                                            dllHandle,
                                                            "_WinStationNotifyLogon"
                                                            );
    if (gpfnWinStationNotifyLogon == NULL) {
        KdPrint(("InitializeMultiUserFunctions: Failed to get _WinStationNotifyLogon Proc %d\n",GetLastError()));
        FreeLibrary(dllHandle);
        return FALSE;
    }

    //_WinStationInitNewSession
     gpfnWinStationNotifyNewSession = (PWINSTATION_NOTIFY_NEW_SESSION) GetProcAddress(
                                                            dllHandle,
                                                            "_WinStationNotifyNewSession"
                                                            );
    if (gpfnWinStationNotifyNewSession == NULL) {
        KdPrint(("InitializeMultiUserFunctions: Failed to get _WinStationNotifyNewSession Proc %d\n",GetLastError()));
    }

    return TRUE;

}

/****************************************************************************\
*
* FUNCTION: HandleFailedLogon
*
* PURPOSE:  Tells the user why their logon attempt failed.
*
* RETURNS:  MSGINA_DLG_FAILURE - we told them what the problem was successfully.
*           DLG_INTERRUPTED() - a set of return values - see winlogon.h
*
* HISTORY:
*
*
\****************************************************************************/

int
HandleFailedLogon(
    PTERMINAL pTerm,
    HWND hDlg,
    NTSTATUS Status,
    NTSTATUS SubStatus,
    PWCHAR UserName,
    PWCHAR Domain
    )
{
    int Result;
    TCHAR    Buffer1[MAX_STRING_BYTES];
    TCHAR    Buffer2[MAX_STRING_BYTES];

    WlxSetTimeout( pTerm, TIMEOUT_NONE );

    switch (Status)
    {
        case ERROR_CTX_LOGON_DISABLED:

            Result = TimeoutMessageBox(pTerm, hDlg, IDS_MULTIUSER_LOGON_DISABLED,
                                             IDS_LOGON_MESSAGE,
                                             MB_OK | MB_ICONEXCLAMATION);
            break;

        case ERROR_CTX_WINSTATION_ACCESS_DENIED:

            Result = TimeoutMessageBox(pTerm, hDlg, IDS_MULTIUSER_WINSTATION_ACCESS_DENIED,
                                             IDS_LOGON_MESSAGE,
                                             MB_OK | MB_ICONEXCLAMATION);
            break;

        default:

#if DBG
            DbgPrint("Logon failure status = 0x%lx, sub-status = 0x%lx", Status, SubStatus);
#endif

            LoadString(NULL, IDS_UNKNOWN_LOGON_FAILURE, Buffer1, sizeof(Buffer1));
            _snwprintf(Buffer2, sizeof(Buffer2), Buffer1, Status);

            LoadString(NULL, IDS_LOGON_MESSAGE, Buffer1, sizeof(Buffer1));

            Result = WlxMessageBox(pTerm, hDlg, Buffer2,
                                                  Buffer1,
                                                  MB_OK | MB_ICONEXCLAMATION);
            break;
    }

    return(Result);

    UNREFERENCED_PARAMETER(UserName);
}

/***************************************************************************\
* FUNCTION: LogonDisabledDlgProc
*
* PURPOSE:  Processes messages for Disabled Logon dialog
*
* RETURNS:  MSGINA_DLG_SUCCESS     - the user was logged on successfully
*           MSGINA_DLG_FAILURE     - the logon failed,
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* HISTORY:
*
*
\***************************************************************************/

INT_PTR WINAPI
LogonDisabledDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    DLG_RETURN_TYPE Result;

    switch (message)
    {

        case WM_INITDIALOG:

            // Centre the window on the screen and bring it to the front
            CentreWindow(hDlg);
            return( TRUE );

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {

                default:

                    switch (LOWORD(wParam))
                    {

                        case IDOK:
                        case IDCANCEL:
                           if (!g_Console) {
                              // Allow logon screen to go away if not at console
                              EndDialog(hDlg, TRUE);
                              return(TRUE);
                           }
                            EndDialog(hDlg, FALSE);
                            return(TRUE);

                    }
                    break;

            }
            break;

        case WLX_WM_SAS:

            if ((wParam == WLX_SAS_TYPE_TIMEOUT) ||
                (wParam == WLX_SAS_TYPE_SCRNSVR_TIMEOUT) )
            {
                //
                // If this was a timeout, return false, and let winlogon
                // kill us later
                //

                return(FALSE);
            }
            return(TRUE);
    }

    return(FALSE);
}


//
// BUGBUG This will go away once we have these properties on UserObject
//
/*******************************************************************************
 *
 *  DefaultUserConfigQueryW (UNICODE)
 *
 *   Fill the Default User Configuration structure
 *
 * ENTRY:
 *
 * EXIT:
 *    Always will return ERROR_SUCCESS, unless UserConfigLength is incorrect.
 *
 ******************************************************************************/

LONG
DefaultUserConfigQuery( WCHAR * pServerName,
                            PUSERCONFIGW pUserConfig,
                            ULONG UserConfigLength,
                            PULONG pReturnLength )
{
    UNREFERENCED_PARAMETER( pServerName );

    /*
     * Validate length and zero-initialize the destination
     * USERCONFIGW buffer.
     */
    if ( UserConfigLength < sizeof(USERCONFIGW) )
        return( ERROR_INSUFFICIENT_BUFFER );

    /*
     * Initialize to an initial default.
     */
    memset(pUserConfig, 0, UserConfigLength);

    pUserConfig->fInheritAutoLogon = TRUE;

    pUserConfig->fInheritResetBroken = TRUE;

    pUserConfig->fInheritReconnectSame = TRUE;

    pUserConfig->fInheritInitialProgram = TRUE;

    pUserConfig->fInheritCallback = FALSE;

    pUserConfig->fInheritCallbackNumber = TRUE;

    pUserConfig->fInheritShadow = TRUE;

    pUserConfig->fInheritMaxSessionTime = TRUE;

    pUserConfig->fInheritMaxDisconnectionTime = TRUE;

    pUserConfig->fInheritMaxIdleTime = TRUE;

    pUserConfig->fInheritAutoClient = TRUE;

    pUserConfig->fInheritSecurity = FALSE;

    pUserConfig->fPromptForPassword = FALSE;

    pUserConfig->fResetBroken = FALSE;

    pUserConfig->fReconnectSame = FALSE;

    pUserConfig->fLogonDisabled = FALSE;

    pUserConfig->fAutoClientDrives = TRUE;

    pUserConfig->fAutoClientLpts = TRUE;

    pUserConfig->fForceClientLptDef = TRUE;

    pUserConfig->fDisableEncryption = TRUE;

    pUserConfig->fHomeDirectoryMapRoot = FALSE;

    pUserConfig->fUseDefaultGina = FALSE;

    pUserConfig->fDisableCpm = FALSE;

    pUserConfig->fDisableCdm = FALSE;

    pUserConfig->fDisableCcm = FALSE;

    pUserConfig->fDisableLPT = FALSE;

    pUserConfig->fDisableClip = FALSE;

    pUserConfig->fDisableExe = FALSE;

    pUserConfig->fDisableCam = FALSE;

    pUserConfig->UserName[0] = 0;

    pUserConfig->Domain[0] = 0;

    pUserConfig->Password[0] = 0;

    pUserConfig->WorkDirectory[0] = 0;

    pUserConfig->InitialProgram[0] = 0;

    pUserConfig->CallbackNumber[0] = 0;

    pUserConfig->Callback = Callback_Disable;

    pUserConfig->Shadow = Shadow_EnableInputNotify;

    pUserConfig->MaxConnectionTime = 0;

    pUserConfig->MaxDisconnectionTime = 0;

    pUserConfig->MaxIdleTime = 0;

    pUserConfig->KeyboardLayout = 0;

    pUserConfig->MinEncryptionLevel = 1;

    pUserConfig->fWallPaperDisabled = FALSE;

    pUserConfig->NWLogonServer[0] = 0;

    pUserConfig->WFProfilePath[0] = 0;

    pUserConfig->WFHomeDir[0] = 0;

    pUserConfig->WFHomeDirDrive[0] = 0;


    *pReturnLength = sizeof(USERCONFIGW);

    return( ERROR_SUCCESS );
}


