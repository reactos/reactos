
/*************************************************************************
*
* api.c
*
* WinStation Control API's for WIN32 subsystem.
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
*************************************************************************/

/*
 *  Includes
 */
#include "precomp.h"
#pragma hdrstop

#include "ntuser.h"

#include <winsta.h>
#include <wstmsg.h>

#define SESSION_ROOT L"\\Sessions"
#define MAX_SESSION_PATH 256

extern NTSTATUS CsrPopulateDosDevices(void);

NTSTATUS
CleanupSessionObjectDirectories(void);

HANDLE CsrQueryApiPort(void);
extern BOOL CtxInitUser32(VOID);

extern DrChangeDisplaySettings(WINSTATIONDORECONNECTMSG*);

USHORT gHRes = 0;
USHORT gVRes = 0;
USHORT gColorDepth = 0;

HANDLE ghportLPC = NULL;

BOOLEAN gbExitInProgress = FALSE;

#if DBG
ULONG  gulConnectCount = 0;
#endif // DBG

/*
 * The following are gotten from ICASRV.
 *
 */

HANDLE G_IcaVideoChannel = NULL;
HANDLE G_IcaMouseChannel = NULL;
HANDLE G_IcaKeyboardChannel = NULL;
HANDLE G_IcaBeepChannel = NULL;
HANDLE G_IcaCommandChannel = NULL;
HANDLE G_IcaThinwireChannel = NULL;
WCHAR G_WinStationName[WINSTATIONNAME_LENGTH];


/*
 * Definition for the WinStation control API's dispatch table
 */
typedef NTSTATUS (*PWIN32WINSTATION_API)(IN OUT PWINSTATION_APIMSG ApiMsg);

typedef struct _WIN32WINSTATION_DISPATCH {
    PWIN32WINSTATION_API pWin32ApiProc;
} WIN32WINSTATION_DISPATCH, *PWIN32WINSTATION_DISPATCH;

NTSTATUS W32WinStationDoConnect( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationDoDisconnect( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationDoReconnect( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationExitWindows( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationTerminate( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationNtSecurity( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationDoMessage( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationThinwireStats( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationShadowSetup( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationShadowStart( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationShadowStop( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationShadowCleanup( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationPassthruEnable( IN OUT PWINSTATION_APIMSG );

NTSTATUS W32WinStationPassthruDisable( IN OUT PWINSTATION_APIMSG );

// This is the counter part to SMWinStationBroadcastSystemMessage  
NTSTATUS W32WinStationBroadcastSystemMessage( IN OUT PWINSTATION_APIMSG );
// This is the counter part to SMWinStationSendWindowMessage
NTSTATUS W32WinStationSendWindowMessage( IN OUT PWINSTATION_APIMSG );

/*
 * WinStation API Dispatch Table
 *
 * Only the API's that WIN32 implements as opposed to ICASRV
 * are entered here. The rest are NULL so that the same WinStation API
 * numbers may be used by ICASRV and WIN32.  If this table is
 * changed, the table below must be modified too, as well as the API
 * dispatch table in the ICASRV.
 */

/*
 * If you  ever change the index of the function pointers in this array
 * make sure API_W32WINSTATIONTERMINATE has the right index
 * for W32WinStationTerminate
 */

#define API_W32WINSTATIONTERMINATE  12  // includes -- W32WinStationSendWindowMessage 

WIN32WINSTATION_DISPATCH Win32WinStationDispatch[SMWinStationMaxApiNumber] = {
    NULL, // create
    NULL, // reset
    NULL, // disconnect
    NULL, // WCharLog
    NULL, // ApiWinStationGetSMCommand,
    NULL, // ApiWinStationBrokenConnection,
    NULL, // ApiWinStationIcaReplyMessage,
    NULL, // ApiWinStationIcaShadowHotkey,
    W32WinStationDoConnect,
    W32WinStationDoDisconnect,
    W32WinStationDoReconnect,
    W32WinStationExitWindows,
    W32WinStationTerminate,
    W32WinStationNtSecurity,
    W32WinStationDoMessage,
    NULL,
    W32WinStationThinwireStats,
    W32WinStationShadowSetup,
    W32WinStationShadowStart,
    W32WinStationShadowStop,
    W32WinStationShadowCleanup,
    W32WinStationPassthruEnable,
    W32WinStationPassthruDisable,
    NULL, // [AraBern] this was missing: SMWinStationInitialProgram
    NULL, // [AraBern] this was missing: SMWinStationNtsdDebug
    W32WinStationBroadcastSystemMessage,
    W32WinStationSendWindowMessage,
};

#if DBG
PSZ Win32WinStationAPIName[SMWinStationMaxApiNumber] = {
    "SmWinStationCreate",
    "SmWinStationReset",
    "SmWinStationDisconnect",
    "SmWinStationWCharLog",
    "SmWinStationGetSMCommand",
    "SmWinStationBrokenConnection",
    "SmWinStationIcaReplyMessage",
    "SmWinStationIcaShadowHotkey",
    "SmWinStationDoConnect",
    "SmWinStationDoDisconnect",
    "SmWinStationDoReconnect",
    "SmWinStationExitWindows",
    "SmWinStationTerminate",
    "SmWinStationNtSecurity",
    "SmWinStationDoMessage",
    "SmWinstationDoBreakPoint",
    "SmWinStationThinwireStats",
    "SmWinStationShadowSetup",
    "SmWinStationShadowStart",
    "SmWinStationShadowStop",
    "SmWinStationShadowCleanup",
    "SmWinStationPassthruEnable",
    "SmWinStationPassthruDisable",
    "SMWinStationInitialProgram",
    "SMWinStationNtsdDebug",
    "W32WinStationBroadcastSystemMessage",
    "W32WinStationSendWindowMessage",
};
#endif

NTSTATUS TerminalServerRequestThread( PVOID );

extern NTSTATUS Win32CommandChannelThread( PVOID );
extern NTSTATUS RemoteDoMessage( PWINSTATION_APIMSG pMsg );
extern NTSTATUS MultiUserSpoolerInit();
//extern BOOL     CtxGetCrossWinStationDebug( VOID );

extern HANDLE g_hDoMessageEvent;

extern NTSTATUS RemoteDoBroadcastSystemMessage( PWINSTATION_APIMSG pMsg );
extern NTSTATUS RemoteDoSendWindowMessage( PWINSTATION_APIMSG  pMsg);


/*****************************************************************************
 *
 *  WinStationAPIInit
 *
 *   Creates and initializes the WinStation API port and thread.
 *
 * ENTRY:
 *    No Parameters
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationAPIInit(
    VOID)
{
    NTSTATUS  Status;
    CLIENT_ID ClientId;
    HANDLE    ThreadHandle;
    KPRIORITY Priority;

#if DBG
    static BOOL Inited = FALSE;
#endif

    UserAssert(Inited == FALSE);

    gSessionId = NtCurrentPeb()->SessionId;

#if DBG
    if (Inited)
        DBGHYD(("WinStationAPIInit called twice !!!\n"));

    Inited = TRUE;
#endif

    Status = RtlCreateUserThread(NtCurrentProcess(),
                              NULL,
                              TRUE,
                              0L,
                              0L,
                              0L,
                              TerminalServerRequestThread,
                              NULL,
                              &ThreadHandle,
                              &ClientId);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("WinStationAPIInit: failed to create TerminalServerRequestThread\n"));
        goto Exit;
    }
    /*
     *  Add thread to server thread pool
     */
    CsrAddStaticServerThread(ThreadHandle, &ClientId, 0);

    /*
     * Boost priority of ICA SRV Request thread
     */
    Priority = THREAD_BASE_PRIORITY_MAX;

    Status = NtSetInformationThread(ThreadHandle, ThreadBasePriority,
                                 &Priority, sizeof(Priority));

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("WinStationAPIInit: failed to set thread priority\n"));
        goto Exit;
    }

    /*
     * Resume the thread now that we've initialized things.
     */
    NtResumeThread(ThreadHandle, NULL);


Exit:
    return Status;
}

NTSTATUS
TerminalServerRequestThread(
    IN PVOID ThreadParameter)
{
    PTEB                        Teb;
    UNICODE_STRING              PortName;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;
    WINSTATIONAPI_CONNECT_INFO  info;
    ULONG                       ConnectInfoLength;
    WINSTATION_APIMSG           ApiMsg;
    PWIN32WINSTATION_DISPATCH   pDispatch;
    NTSTATUS                    Status;
    REMOTE_PORT_VIEW            ServerView;
    HANDLE                      CsrStartHandle, hevtTermSrvInit;

    /*
     * Initialize GDI accelerators.  Identify this thread as a server thread.
     */
    Teb = NtCurrentTeb();
    Teb->GdiClientPID = 4; // PID_SERVERLPC
    Teb->GdiClientTID = HandleToUlong(Teb->ClientId.UniqueThread);

    /*
     *  Connect to user
     */

    hevtTermSrvInit = CreateEvent(NULL, TRUE, FALSE,
                                  L"Global\\TermSrvReadyEvent");

    UserAssert(hevtTermSrvInit != NULL);
#if DBG
    if (hevtTermSrvInit == NULL) {
        DBGHYD(("ERROR: could not create TermSrvReadyEvent !!!\n"));
    }
#endif

    Status = NtWaitForSingleObject(hevtTermSrvInit, FALSE, NULL);

    NtClose(hevtTermSrvInit);

    /*
     *  Connect to terminal server API port
     */
    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;

    RtlInitUnicodeString(&PortName, L"\\SmSsWinStationApiPort");

    /*
     * Init the REMOTE_VIEW structure
     */
    ServerView.Length = sizeof(ServerView);
    ServerView.ViewSize = 0;
    ServerView.ViewBase = 0;

    /*
     * Fill in the ConnectInfo structure with our access request mask
     */
    info.Version = CITRIX_WINSTATIONAPI_VERSION;
    info.RequestedAccess = 0; // BUGBUG should ask for ALL access
    ConnectInfoLength = sizeof(WINSTATIONAPI_CONNECT_INFO);

    Status = NtConnectPort( &ghportLPC,
                            &PortName,
                            &DynamicQos,
                            NULL, // ClientView
                            &ServerView,
                            NULL, // Max message length [select default]
                            (PVOID)&info,
                            &ConnectInfoLength);

    if (!NT_SUCCESS(Status)) {
        RIPMSG0(RIP_WARNING, "TerminalServerRequestThread: Failed to connect to LPC port");
        return Status;
    }

    RtlZeroMemory(&ApiMsg, sizeof(ApiMsg));

    //
    // Terminal Server calls into Session Manager to create a new Hydra session.
    // The session manager creates and resume a new session and returns to Terminal
    // server the session id of the new session. There is a race condition where
    // CSR can resume and call into terminal server before terminal server can
    // store the session id in its internal structure. To prevent this CSR will
    // wait here on a named event which will be set by Terminal server once it
    // gets the sessionid for the newly created session
    //

    if (NtCurrentPeb()->SessionId != 0) {

        CsrStartHandle = CreateEvent(NULL, TRUE, FALSE, L"CsrStartEvent");

        if (!CsrStartHandle) {

            RIPMSG1(RIP_WARNING, "TerminalServerRequestThread: Failed to create 'CsrStartEvent' with Status 0x%x",
                    Status);

            return Status;

        } else {

            Status = NtWaitForSingleObject(CsrStartHandle, FALSE, NULL);

            NtClose(CsrStartHandle);
            if (!NT_SUCCESS(Status)) {
                DBGHYD(("TerminalServerRequestThread: Wait for object 'CsrStartEvent' failed Status=0x%x\n",
                        Status));
            }
        }
    }

    for (;;) {

        /*
         * Initialize LPC message fields
         */
        ApiMsg.h.u1.s1.DataLength     = sizeof(ApiMsg) - sizeof(PORT_MESSAGE);
        ApiMsg.h.u1.s1.TotalLength    = sizeof(ApiMsg);
        ApiMsg.h.u2.s2.Type           = 0; // Kernel will fill in message type
        ApiMsg.h.u2.s2.DataInfoOffset = 0;
        ApiMsg.ApiNumber              = SMWinStationGetSMCommand;

        Status = NtRequestWaitReplyPort(ghportLPC,
                                        (PPORT_MESSAGE)&ApiMsg,
                                        (PPORT_MESSAGE)&ApiMsg);

        if (!NT_SUCCESS(Status)) {
            DBGHYD(("TerminalServerRequestThread wait failed with Status %lx\n",
                   Status));
            break;
        }

        if (ApiMsg.ApiNumber >= SMWinStationMaxApiNumber ) {

            DBGHYD(("TerminalServerRequestThread Bad API number %d\n",
                   ApiMsg.ApiNumber));

            ApiMsg.ReturnedStatus = STATUS_NOT_IMPLEMENTED;

        } else {

            /*
             * We must VALIDATE which ones are implemented here
             */
            pDispatch = &Win32WinStationDispatch[ApiMsg.ApiNumber];

            if (pDispatch->pWin32ApiProc) {

                BOOL bRestoreDesktop = FALSE;
                NTSTATUS Status;
                USERTHREAD_USEDESKTOPINFO utudi;

                /*
                 * For all the win32k callouts with the exception of
                 * terminate set this thread to the current desktop
                 */
                if (ApiMsg.ApiNumber != API_W32WINSTATIONTERMINATE) {
                    utudi.hThread = NULL;
                    utudi.drdRestore.pdeskRestore = NULL;
                    Status = NtUserSetInformationThread(NtCurrentThread(),
                                                        UserThreadUseActiveDesktop,
                                                        &utudi, sizeof(utudi));

                    if (NT_SUCCESS(Status)) {
                        bRestoreDesktop = TRUE;
                    }
                }

                /*
                 * Call the API
                 */
                ApiMsg.ReturnedStatus = (pDispatch->pWin32ApiProc)(&ApiMsg);

                if (bRestoreDesktop) {
                    NtUserSetInformationThread(NtCurrentThread(),
                                               UserThreadUseDesktop,
                                               &utudi,
                                               sizeof(utudi));
                }

            } else {
                // This control API is not implemented in WIN32
                ApiMsg.ReturnedStatus = STATUS_NOT_IMPLEMENTED;
            }
        }
    }

    ExitThread(0);

    return STATUS_UNSUCCESSFUL;

    UNREFERENCED_PARAMETER(ThreadParameter);
}

#if DBG
VOID
W32WinStationDumpReconnectInfo(
    WINSTATIONDORECONNECTMSG *pDoReconnect,
    BOOLEAN bReconnect)
{
    PSTR pCallerName;

    if (bReconnect) {
        pCallerName = "W32WinStationDoReconnect";
    } else {
        pCallerName = "W32WinStationDoConnect";
    }

    DbgPrint(pCallerName);
    DbgPrint(" - Display resolution information for session %d :\n", gSessionId);

    DbgPrint("\tProtocolType : %04d\n", pDoReconnect->ProtocolType);
    DbgPrint("\tHRes : %04d\n", pDoReconnect->HRes);
    DbgPrint("\tVRes : %04d\n", pDoReconnect->VRes);
    DbgPrint("\tColorDepth : %04d\n", pDoReconnect->ColorDepth);

    DbgPrint("\tKeyboardType : %d\n", pDoReconnect->KeyboardType);
    DbgPrint("\tKeyboardSubType : %d\n", pDoReconnect->KeyboardSubType);
    DbgPrint("\tKeyboardFunctionKey : %d\n", pDoReconnect->KeyboardFunctionKey);
}
#else
    #define W32WinStationDumpReconnectInfo(p, b)
#endif // DBG

NTSTATUS
W32WinStationDoConnect(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                Status = STATUS_SUCCESS;
    WINSTATIONDOCONNECTMSG* m = &pMsg->u.DoConnect;
    WCHAR                   DisplayDriverName[10];
    CLIENT_ID               ClientId;
    HANDLE                  ThreadHandle;
    KPRIORITY               Priority;
    DOCONNECTDATA           DoConnectData;
    WINSTATIONDORECONNECTMSG mDoReconnect;

    UserAssert(gulConnectCount == 0);

    /*
     * Populate the sessions \DosDevices from
     * the current consoles settings
     */
    Status = CsrPopulateDosDevices();
    if( !NT_SUCCESS(Status) ) {
        DBGHYD(("CsrPopulateDosDevices failed with Status %lx\n", Status));
        goto Exit;
    }


    G_IcaVideoChannel    = m->hIcaVideoChannel;
    G_IcaMouseChannel    = m->hIcaMouseChannel;
    G_IcaKeyboardChannel = m->hIcaKeyboardChannel;
    G_IcaBeepChannel     = m->hIcaBeepChannel;
    G_IcaCommandChannel  = m->hIcaCommandChannel;
    G_IcaThinwireChannel = m->hIcaThinwireChannel;

    memset(G_WinStationName, 0, sizeof(G_WinStationName));
    memcpy(G_WinStationName, m->WinStationName,
            min(sizeof(G_WinStationName), sizeof(m->WinStationName)));

    /*
     * This must be 8 unicode characters (file name) plus two zero wide characters.
     */
    memset(DisplayDriverName, 0, sizeof(DisplayDriverName));
    memcpy(DisplayDriverName, m->DisplayDriverName, sizeof(DisplayDriverName) - 2);

    /*
     * Give the information to the WIN32 driver.
     */
    memset(&DoConnectData, 0, sizeof(DoConnectData));

    DoConnectData.fMouse          = m->fMouse;
    DoConnectData.IcaBeepChannel  = G_IcaBeepChannel;
    DoConnectData.IcaVideoChannel = G_IcaVideoChannel;
    DoConnectData.IcaMouseChannel = G_IcaMouseChannel;
    DoConnectData.fEnableWindowsKey = m->fEnableWindowsKey;;

    DoConnectData.IcaKeyboardChannel        = G_IcaKeyboardChannel;
    DoConnectData.IcaThinwireChannel        = G_IcaThinwireChannel;
    DoConnectData.fClientDoubleClickSupport = m->fClientDoubleClickSupport;

    /*
     * Give the information to the keyboard type/subtype/number of functions.
     */
    DoConnectData.ClientKeyboardType.Type        = m->KeyboardType;
    DoConnectData.ClientKeyboardType.SubType     = m->KeyboardSubType;
    DoConnectData.ClientKeyboardType.FunctionKey = m->KeyboardFunctionKey;

    memcpy(DoConnectData.WinStationName, G_WinStationName,
            min(sizeof(G_WinStationName), sizeof(DoConnectData.WinStationName)));

    DoConnectData.drProtocolType = m->ProtocolType;
    DoConnectData.drPelsHeight = m->VRes;
    DoConnectData.drPelsWidth = m->HRes;
    DoConnectData.drBitsPerPel = m->ColorDepth;
    //BUGBUG DoConnectData.drDisplayFrequency is no yet setup

    // We set here the WINSTATIONDORECONNECTMSG that we will be using latter
    // In the call to DrChangeDisplaySettings(), because we want to trace the display
    // resolution information, prior to call NtUserRemoteConnect().

    mDoReconnect.ProtocolType = m->ProtocolType;
    mDoReconnect.HRes = m->HRes;
    mDoReconnect.VRes = m->VRes;
    mDoReconnect.ColorDepth = m->ColorDepth;

    W32WinStationDumpReconnectInfo(&mDoReconnect, FALSE);

    /* Give winstation protocol name */
    memset(DoConnectData.ProtocolName, 0, sizeof(DoConnectData.ProtocolName));
    memcpy(DoConnectData.ProtocolName, m->ProtocolName, sizeof(DoConnectData.ProtocolName) - 2);

    /* Give winstation audio drver name */
    memset(DoConnectData.AudioDriverName, 0, sizeof(DoConnectData.AudioDriverName));
    memcpy(DoConnectData.AudioDriverName, m->AudioDriverName, sizeof(DoConnectData.AudioDriverName) - 2);

    Status = NtUserRemoteConnect(&DoConnectData,
                                sizeof(DisplayDriverName),
                                DisplayDriverName);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("NtUserRemoteConnect failed with Status %lx\n", Status));
        goto Exit;
    }
    
    Status = RtlCreateUserThread( NtCurrentProcess(),
                                  NULL,
                                  TRUE,
                                  0L,
                                  0L,
                                  0L,
                                  Win32CommandChannelThread,
                                  NULL,
                                  &ThreadHandle,
                                  &ClientId );

    UserAssert(NT_SUCCESS(Status));
    
    if (!NT_SUCCESS(Status)) {
        DBGHYD(("RtlCreateUserThread failed with Status %lx\n", Status));
        goto Exit;
    }

    /*
     *  Add thread to server thread pool
     */
    if (CsrAddStaticServerThread(ThreadHandle, &ClientId, 0) == NULL) {
        DBGHYD(("CsrAddStaticServerThread failed\n"));
        goto Exit;
    }

    /*
     * Boost priority of thread
     */
    Priority = THREAD_BASE_PRIORITY_MAX;

    Status = NtSetInformationThread(ThreadHandle, ThreadBasePriority,
                                    &Priority, sizeof(Priority));

    UserAssert(NT_SUCCESS(Status));
    
    if (!NT_SUCCESS(Status)) {
        DBGHYD(("NtSetInformationThread failed with Status %lx\n", Status));
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    /*
     * Resume the thread now that we've initialized things.
     */
    NtResumeThread(ThreadHandle, NULL);

    if (CsrConnectToUser() == NULL) {
        DBGHYD(("CsrConnectToUser failed\n"));
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }
    
    if (!CtxInitUser32()) {
        DBGHYD(("CtxInitUser32 failed\n"));
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    /*
     * Create the Spooler service thread
     */
    if (gSessionId != 0) {
        Status = MultiUserSpoolerInit();
    }

    /*
     * Save the resolution
     */
    gHRes       = mDoReconnect.HRes;
    gVRes       = mDoReconnect.VRes;
    gColorDepth = mDoReconnect.ColorDepth;


Exit:

#if DBG
    if (NT_SUCCESS(Status)) {
        gulConnectCount++;
    }
#endif // DBG

    return Status;
}


NTSTATUS
W32WinStationDoDisconnect(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;

    memset(G_WinStationName, 0, sizeof(G_WinStationName));

    Status = (NTSTATUS)NtUserCallNoParam(SFI_XXXREMOTEDISCONNECT);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("xxxRemoteDisconnect failed with Status %lx\n", Status));
    }

#if DBG
    else {
        gulConnectCount--;
    }
#endif // DBG

    return Status;

    UNREFERENCED_PARAMETER(pMsg);
}


NTSTATUS
W32WinStationDoReconnect(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    DORECONNECTDATA             DoReconnectData;
    WINSTATIONDORECONNECTMSG*   m = &pMsg->u.DoReconnect;

    UserAssert(gulConnectCount == 0);


    memset(&DoReconnectData, 0, sizeof(DoReconnectData));

    DoReconnectData.fMouse = m->fMouse;
    DoReconnectData.fEnableWindowsKey = m->fEnableWindowsKey;
    DoReconnectData.fClientDoubleClickSupport = m->fClientDoubleClickSupport;

    memcpy(G_WinStationName, m->WinStationName,
           min(sizeof(G_WinStationName), sizeof(m->WinStationName)));

    memcpy(DoReconnectData.WinStationName, G_WinStationName,
           min(sizeof(G_WinStationName), sizeof(DoReconnectData.WinStationName)));

    DoReconnectData.drProtocolType = m->ProtocolType;
    DoReconnectData.drPelsHeight = m->VRes;
    DoReconnectData.drPelsWidth = m->HRes;
    DoReconnectData.drBitsPerPel = m->ColorDepth;
    if ((m->fDynamicReconnect) && (gHRes != m->HRes || gVRes != m->VRes || gColorDepth != m->ColorDepth ) ) {
       DoReconnectData.fChangeDisplaySettings = TRUE;
    }else{
       DoReconnectData.fChangeDisplaySettings = FALSE;
    }
    //BUGBUG DoReconnectData.drDisplayFrequency is no yet setup
    DoReconnectData.drDisplayFrequency = 0;

    /*
     * Give the information to the keyboard type/subtype/number of functions.
     */
    DoReconnectData.ClientKeyboardType.Type        = m->KeyboardType;
    DoReconnectData.ClientKeyboardType.SubType     = m->KeyboardSubType;
    DoReconnectData.ClientKeyboardType.FunctionKey = m->KeyboardFunctionKey;

    W32WinStationDumpReconnectInfo( m, TRUE);

    /*
     * Give the information to the WIN32 driver.
     */

    Status = (NTSTATUS)NtUserCallOneParam((ULONG_PTR)&DoReconnectData,
                                          SFI_XXXREMOTERECONNECT);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("xxxRemoteReconnect failed with Status %lx\n", Status));
    }else{

    /*
     * Save the resolution
     */
        gHRes       = m->HRes;
        gVRes       = m->VRes;
        gColorDepth = m->ColorDepth;

#if DBG
        gulConnectCount++;
#endif // DBG
    }


    return Status;
}


NTSTATUS
W32WinStationExitWindows(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    WINSTATIONEXITWINDOWSMSG*   m = &pMsg->u.ExitWindows;

    if (gSessionId == 0) {
        //BUGBUG v-nicbd: I am just wondering why it would be meaningless for the console. (?)
        DBGHYD(("W32WinStationExitWindows meaningless for main session\n"));
        return STATUS_INVALID_PARAMETER;
    }

    UserAssert(gulConnectCount <= 1);

    /*
     *  Tell winlogon to logoff
     */
    Status = (NTSTATUS)NtUserCallNoParam(SFI_REMOTELOGOFF);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("RemoteLogoff failed with Status %lx\n", Status));
    }

    return Status;
}


NTSTATUS
W32WinStationTerminate(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE   hevtShutDown;
    HANDLE   hevtRitExited, hevtRitStuck;

extern HANDLE WinStationIcaApiPort;

    gbExitInProgress = TRUE;

    /*
     * Get rid of hard error thread
     */

    
    if (gdwHardErrorThreadId != 0) {

        BoostHardError(-1, BHE_FORCE);

        /*
         * Poll (!!?) for hard error thread completion.  The thread does
         * not exit.
         */
        while (gdwHardErrorThreadId != 0) {
            DBGHYD(("Waiting for hard error thread to stop...\n"));

            Sleep(3 * 1000);
        }

        DBGHYD(("Stopped hard error thread\n"));
    }

    if (g_hDoMessageEvent)
        NtSetEvent(g_hDoMessageEvent, NULL);

    if (G_IcaMouseChannel) {
        CloseHandle(G_IcaMouseChannel);
        G_IcaMouseChannel = NULL;
    }

    if (G_IcaKeyboardChannel) {
        CloseHandle(G_IcaKeyboardChannel);
        G_IcaKeyboardChannel = NULL;
    }

    if (G_IcaCommandChannel) {
        CloseHandle(G_IcaCommandChannel);
        G_IcaCommandChannel = NULL;
    }

    if (G_IcaVideoChannel) {
        CloseHandle(G_IcaVideoChannel);
        G_IcaVideoChannel = NULL;
    }
    if (G_IcaBeepChannel) {
        CloseHandle(G_IcaBeepChannel);
        G_IcaBeepChannel = NULL;
    }
    if (G_IcaThinwireChannel) {
        CloseHandle(G_IcaThinwireChannel);
        G_IcaThinwireChannel = NULL;
    }
    
    hevtShutDown = OpenEvent(EVENT_ALL_ACCESS,
                             FALSE,
                             L"EventShutDownCSRSS");

    if (hevtShutDown == NULL) {
        /*
         * This case is for cached sessions where RIT and Destiop thread have
         * not been created.
         */
        DBGHYD(("W32WinStationTerminate terminating CSRSS ...\n"));

        Status = CleanupSessionObjectDirectories();

        if (ghportLPC) {
            NtClose(ghportLPC);
            ghportLPC = NULL;
        }

        return 0;
    }

    hevtRitExited = CreateEvent(NULL,
                                FALSE,
                                FALSE,
                                L"EventRitExited");

    UserAssert(hevtRitExited != NULL);

    hevtRitStuck = CreateEvent(NULL,
                               FALSE,
                               FALSE,
                               L"EventRitStuck");

    UserAssert(hevtRitStuck != NULL);
    
    /*
     * RIT is created. Signal this event that starts the
     * cleanup in win32k
     */
    SetEvent(hevtShutDown);

    DBGHYD(("EventShutDownCSRSS set in CSRSS ...\n"));

    
    while (1) {

        HANDLE arHandles[2] = {hevtRitExited, hevtRitStuck};
        DWORD  result;
        
        result = WaitForMultipleObjects(2, arHandles, FALSE, INFINITE);

        switch (result) {
        case WAIT_OBJECT_0:
            goto RITExited;
        
        case WAIT_OBJECT_0 + 1:
            
            /*
             * The RIT is stucked because there are still GUI threads
             * assigned to desktops. One reason for this is that winlogon
             * died w/o calling ExitWindowsEx.
             */
            UserAssert(ghportLPC != NULL);

            if (ghportLPC) {
                NtClose(ghportLPC);
                ghportLPC = NULL;
            }
            break;
        default:
            UserAssert(0);
            break;
        }
    }
    
RITExited:

    DBGHYD(("EventRitExited set in CSRSS ...\n"));

    CloseHandle(hevtRitExited);
    CloseHandle(hevtRitStuck);

    CloseHandle(hevtShutDown);

    Status = CleanupSessionObjectDirectories();

    if (ghportLPC) {
        NtClose(ghportLPC);
        ghportLPC = NULL;
    }

    return Status;
    UNREFERENCED_PARAMETER(pMsg);
}


NTSTATUS
W32WinStationNtSecurity(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = (NTSTATUS)NtUserCallNoParam(SFI_REMOTENTSECURITY);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("RemoteNtSecurity failed with Status %lx\n", Status));
    }

    return Status;
    UNREFERENCED_PARAMETER(pMsg);
}


NTSTATUS
W32WinStationDoMessage(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = RemoteDoMessage(pMsg);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("RemoteDoMessage failed with Status %lx\n", Status));
    }

    return Status;
}

 // This is the counter part to SMWinStationBroadcastSystemMessage  
NTSTATUS 
W32WinStationBroadcastSystemMessage( 
    PWINSTATION_APIMSG pMsg )
{

    NTSTATUS Status = STATUS_SUCCESS;
    
    Status = RemoteDoBroadcastSystemMessage(pMsg);
    
    if (!NT_SUCCESS(Status)) {
        DBGHYD(("RemoteDoBroadcastSystemMessage(): failed with status 0x%lx\n", Status));
    }

    return Status;
}
 // This is the counter part to SMWinStationSendWindowMessage
NTSTATUS 
W32WinStationSendWindowMessage( 
    PWINSTATION_APIMSG  pMsg)
{

    NTSTATUS Status = STATUS_SUCCESS;
    
    Status = RemoteDoSendWindowMessage(pMsg);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("RemoteDoSendWindowMessage failed with Status 0x%lx\n", Status));
    }

    return Status;
}


NTSTATUS
W32WinStationThinwireStats(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    WINSTATIONTHINWIRESTATSMSG* m = &pMsg->u.ThinwireStats;

    Status = (NTSTATUS)NtUserCallOneParam((ULONG_PTR)&m->Stats,
                                          SFI_REMOTETHINWIRESTATS);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("RemoteThinwireStats failed with Status %lx\n", Status));
    }

    return Status;
}


NTSTATUS
W32WinStationShadowSetup(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                  Status = STATUS_SUCCESS;
    WINSTATIONSHADOWSETUPMSG* m = &pMsg->u.ShadowSetup;

    Status = (NTSTATUS)NtUserCallNoParam(SFI_XXXREMOTESHADOWSETUP);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("xxxRemoteShadowSetup failed with Status %lx\n", Status));
    }

    return Status;
}


NTSTATUS
W32WinStationShadowStart(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                  Status = STATUS_SUCCESS;
    WINSTATIONSHADOWSTARTMSG* m = &pMsg->u.ShadowStart;

    Status = (NTSTATUS)NtUserCallTwoParam((ULONG_PTR)m->pThinwireData,
                                          (ULONG_PTR)m->ThinwireDataLength,
                                          SFI_REMOTESHADOWSTART);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("RemoteShadowStart failed with Status %lx\n", Status));
    }

    return Status;
}


NTSTATUS
W32WinStationShadowStop(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                 Status = STATUS_SUCCESS;
    WINSTATIONSHADOWSTOPMSG* m = &pMsg->u.ShadowStop;

    Status = (NTSTATUS)NtUserCallNoParam(SFI_XXXREMOTESHADOWSTOP);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("xxxRemoteShadowStop failed with Status %lx\n", Status));
    }

    return Status;
}


NTSTATUS
W32WinStationShadowCleanup(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    WINSTATIONSHADOWCLEANUPMSG* m = &pMsg->u.ShadowCleanup;

    Status = (NTSTATUS)NtUserCallTwoParam((ULONG_PTR)m->pThinwireData,
                                          (ULONG_PTR)m->ThinwireDataLength,
                                          SFI_REMOTESHADOWCLEANUP);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("RemoteShadowCleanup failed with Status %lx\n", Status));
    }

    return Status;
}


NTSTATUS
W32WinStationPassthruEnable(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = (NTSTATUS)NtUserCallNoParam(SFI_XXXREMOTEPASSTHRUENABLE);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("xxxRemotePassthruEnable failed with Status %lx\n", Status));
    }

    return Status;
    UNREFERENCED_PARAMETER(pMsg);
}


NTSTATUS
W32WinStationPassthruDisable(
    PWINSTATION_APIMSG pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = (NTSTATUS)NtUserCallNoParam(SFI_REMOTEPASSTHRUDISABLE);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("RemotePassthruDisable failed with Status %lx\n", Status));
    }

    return Status;
    UNREFERENCED_PARAMETER(pMsg);
}


NTSTATUS
CleanupSessionObjectDirectories(
    VOID)
{
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING    UnicodeString;
    HANDLE            LinkHandle;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    BOOLEAN           RestartScan;
    UCHAR             DirInfoBuffer[ 4096 ];
    WCHAR             szSessionString [ MAX_SESSION_PATH ];
    ULONG             Context = 0;
    ULONG             ReturnedLength;
    HANDLE            DosDevicesDirectory;
    HANDLE            *HandleArray;
    ULONG             Size = 100;
    ULONG             i, Count = 0;

    swprintf(szSessionString,L"%ws\\%ld\\DosDevices",SESSION_ROOT,NtCurrentPeb()->SessionId);

    RtlInitUnicodeString(&UnicodeString, szSessionString);

    InitializeObjectAttributes(&Attributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenDirectoryObject(&DosDevicesDirectory,
                                   DIRECTORY_ALL_ACCESS,
                                   &Attributes);

    if (!NT_SUCCESS(Status)) {
        DBGHYD(("NtOpenDirectoryObject failed with Status %lx\n", Status));
        return Status;
    }

Restart:
    HandleArray = (HANDLE *)LocalAlloc(LPTR, Size * sizeof(HANDLE));

    if (HandleArray == NULL) {

        NtClose(DosDevicesDirectory);
        return STATUS_NO_MEMORY;
    }

    RestartScan = TRUE;
    DirInfo = (POBJECT_DIRECTORY_INFORMATION)&DirInfoBuffer;

    while (TRUE) {
        Status = NtQueryDirectoryObject( DosDevicesDirectory,
                                         (PVOID)DirInfo,
                                         sizeof(DirInfoBuffer),
                                         TRUE,
                                         RestartScan,
                                         &Context,
                                         &ReturnedLength);

        /*
         *  Check the status of the operation.
         */
        if (!NT_SUCCESS(Status)) {

            if (Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }
            break;
        }

        if (!wcscmp(DirInfo->TypeName.Buffer, L"SymbolicLink")) {

            if ( Count >= Size ) {

                for (i = 0; i < Count ; i++) {
                    NtClose (HandleArray[i]);
                }
                Size += 20;
                Count = 0;
                LocalFree(HandleArray);
                goto Restart;

            }

            InitializeObjectAttributes( &Attributes,
                                        &DirInfo->Name,
                                        OBJ_CASE_INSENSITIVE,
                                        DosDevicesDirectory,
                                        NULL);

            Status = NtOpenSymbolicLinkObject( &LinkHandle,
                                               SYMBOLIC_LINK_ALL_ACCESS,
                                               &Attributes);

            if (NT_SUCCESS(Status)) {

                Status = NtMakeTemporaryObject( LinkHandle );

                if (NT_SUCCESS( Status )) {
                    HandleArray[Count] = LinkHandle;
                    Count++;
                }
            }

        }
        RestartScan = FALSE;
     }

     for (i = 0; i < Count ; i++) {

         NtClose (HandleArray[i]);

     }

     LocalFree(HandleArray);

     NtClose(DosDevicesDirectory);

     return Status;
}
