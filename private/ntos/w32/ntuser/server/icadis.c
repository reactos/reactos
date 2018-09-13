
/*************************************************************************
*
* icadis.c
*
* Send notice of ICA disconnect
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* $Author:
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
#include <icadd.h>

HANDLE WinStationIcaApiPort = NULL;

/*******************************************************************************
 *
 *  ConnectToTerminalServer
 *
 * ENTRY:
 *    Access (input)
 *       security access
 *
 * EXIT:
 *    STATUS_SUCCESS - successful
 *
 ******************************************************************************/

NTSTATUS
ConnectToTerminalServer(
    ULONG Access,
    PHANDLE pPortHandle)
{
    UNICODE_STRING              PortName;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;
    WINSTATIONAPI_CONNECT_INFO  info;
    ULONG                       ConnectInfoLength;
    NTSTATUS                    Status;

    /*
     * Set up SM API port name
     */
    RtlInitUnicodeString(&PortName, L"\\SmSsWinStationApiPort");

    /*
     * Set up the security quality of service parameters to use over the
     * port.  Use the most efficient (least overhead) - which is dynamic
     * rather than static tracking.
     */
    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;

    /*
     * Fill in the ConnectInfo structure with our access request mask
     */
    info.Version = CITRIX_WINSTATIONAPI_VERSION;
    info.RequestedAccess = Access;
    ConnectInfoLength = sizeof(WINSTATIONAPI_CONNECT_INFO);

    // Attempt to connect to the Session Manager API port
    Status = NtConnectPort(pPortHandle,
                            &PortName,
                            &DynamicQos,
                            NULL,
                            NULL,
                            NULL, // Max message length [select default]
                            (PVOID)&info,
                            &ConnectInfoLength);
    if (!NT_SUCCESS(Status)) {
        // Look at the returned INFO to see why if desired
        *pPortHandle = NULL;
#if DBG
        if (ConnectInfoLength == sizeof(WINSTATIONAPI_CONNECT_INFO)) {
            DbgPrint("WinstationConnectToICASrv: connect failed, Reason 0x%x\n", info.AcceptStatus);
        }
        DbgPrint("WinstationConnectToICASrv: Connect failed 0x%x\n", Status);
#endif
        return (Status);
    }

    return (STATUS_SUCCESS);
}


/*******************************************************************************
 *
 *  BrokenConnection
 *
 * ENTRY:
 *    Reason code
 *
 * EXIT:
 *    STATUS_SUCCESS - successful
 *
 ******************************************************************************/


NTSTATUS
BrokenConnection(
    BROKENCLASS       Reason,
    BROKENSOURCECLASS Source)
{
    WINSTATION_APIMSG Msg;
    NTSTATUS          Status;

    /*
     * Connect to Session Mgr
     */
    if (WinStationIcaApiPort == NULL) {
        Status = ConnectToTerminalServer(0, &WinStationIcaApiPort);        //BUGBUG -- add access
        if (!NT_SUCCESS(Status)) {
            return (Status);
        }
    }


    Msg.h.u1.s1.DataLength = sizeof(Msg) - sizeof(PORT_MESSAGE);
    Msg.h.u1.s1.TotalLength = sizeof(Msg);
    Msg.h.u2.s2.Type = 0; // Kernel will fill in message type
    Msg.h.u2.s2.DataInfoOffset = 0;
    Msg.WaitForReply = TRUE;
    Msg.ApiNumber = SMWinStationBrokenConnection;
    Msg.ReturnedStatus = 0;

    Msg.u.Broken.Reason = Reason;
    Msg.u.Broken.Source = Source;

    Status = NtRequestWaitReplyPort(WinStationIcaApiPort, (PPORT_MESSAGE)&Msg, (PPORT_MESSAGE)&Msg);




#if DBG
    if (!NT_SUCCESS(Status)) {
        DbgPrint("BrokenConnection: rc=0x%x\n", Status);
    }
#endif

    return (Status);
}


/*******************************************************************************
 *
 *  ReplyMessageToTerminalServer
 *
 * ENTRY:
 *
 * EXIT:
 *    STATUS_SUCCESS - successful
 *
 ******************************************************************************/

NTSTATUS
ReplyMessageToTerminalServer(
    PCTXHARDERRORINFO pchi)
{
    WINSTATION_APIMSG Msg;
    NTSTATUS          Status;
    HANDLE            PortHandle;

    /*
     * Connect to Session Mgr
     */
    Status = ConnectToTerminalServer(0, &PortHandle);        //BUGBUG -- add access
    if (!NT_SUCCESS(Status)) {
        return (Status);
    }





    Msg.h.u1.s1.DataLength = sizeof(Msg) - sizeof(PORT_MESSAGE);
    Msg.h.u1.s1.TotalLength = sizeof(Msg);
    Msg.h.u2.s2.Type = 0; // Kernel will fill in message type
    Msg.h.u2.s2.DataInfoOffset = 0;
    Msg.WaitForReply = TRUE;
    Msg.ApiNumber = SMWinStationIcaReplyMessage;
    Msg.ReturnedStatus = 0;

    Msg.u.ReplyMessage.Response  = pchi->Response;
    Msg.u.ReplyMessage.pResponse = pchi->pResponse;
    Msg.u.ReplyMessage.hEvent    = pchi->hEvent;

    Status = NtRequestWaitReplyPort(PortHandle, (PPORT_MESSAGE)&Msg, (PPORT_MESSAGE)&Msg);





#if DBG
    if (!NT_SUCCESS(Status)) {
        DbgPrint("ReplyMessageToTerminalServer: rc=0x%x\n", Status);
    }
#endif
    NtClose(PortHandle);

    return (Status);
}

/*******************************************************************************
 *
 *  ShadowHotkey
 *
 * ENTRY:
 *    none
 *
 * EXIT:
 *    STATUS_SUCCESS - successful
 *
 ******************************************************************************/

NTSTATUS
ShadowHotkey()
{
    WINSTATION_APIMSG Msg;
    NTSTATUS Status;

    /*
     * Connect to Session Mgr
     */
    if (WinStationIcaApiPort == NULL) {
        Status = ConnectToTerminalServer(0, &WinStationIcaApiPort);        //BUGBUG -- add access
        if (!NT_SUCCESS(Status)) {
            return (Status);
        }
    }






    Msg.h.u1.s1.DataLength = sizeof(Msg) - sizeof(PORT_MESSAGE);
    Msg.h.u1.s1.TotalLength = sizeof(Msg);
    Msg.h.u2.s2.Type = 0; // Kernel will fill in message type
    Msg.h.u2.s2.DataInfoOffset = 0;
    Msg.WaitForReply = TRUE;
    Msg.ApiNumber = SMWinStationIcaShadowHotkey;
    Msg.ReturnedStatus = 0;

    Status = NtRequestWaitReplyPort(WinStationIcaApiPort, (PPORT_MESSAGE)&Msg, (PPORT_MESSAGE)&Msg);



#if DBG
    if (!NT_SUCCESS(Status)) {
        DbgPrint("ShadowHotkey: rc=0x%x\n", Status);
    }
#endif

    return (Status);
}
