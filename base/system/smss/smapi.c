/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/print.c
 * PURPOSE:         \SmApiPort LPC port message management.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"

#define NDEBUG
#include <debug.h>

/* GLOBAL VARIABLES *********************************************************/

static HANDLE SmApiPort = INVALID_HANDLE_VALUE;

/* SM API *******************************************************************/

SMAPI(SmInvalid)
{
    DPRINT("SM: %s called\n",__FUNCTION__);
    Request->SmHeader.Status = STATUS_NOT_IMPLEMENTED;
    return STATUS_SUCCESS;
}

/* SM API Table */
typedef NTSTATUS (FASTCALL * SM_PORT_API)(PSM_PORT_MESSAGE);

SM_PORT_API SmApi [] =
{
    SmInvalid,	/* unused */
    SmCompSes,	/* smapicomp.c */
    SmInvalid,	/* obsolete */
    SmInvalid,	/* unknown */
    SmExecPgm,	/* smapiexec.c */
    SmQryInfo	/* smapyqry.c */
};

/* TODO: optimize this address computation (it should be done
 * with a macro) */
PSM_CONNECT_DATA FASTCALL SmpGetConnectData (PSM_PORT_MESSAGE Request)
{
    PPORT_MESSAGE PortMessage = (PPORT_MESSAGE) Request;
    return (PSM_CONNECT_DATA)(PortMessage + 1);
}

NTSTATUS NTAPI
SmpHandleConnectionRequest (PSM_PORT_MESSAGE Request);

/**********************************************************************
 * SmpCallbackServer/2
 *
 * DESCRIPTION
 *	The SM calls back a previously connected subsystem process to
 *	authorize it to bootstrap (initialize). The SM connects to a
 *	named LPC port which name was sent in the connection data by
 *	the candidate subsystem server process.
 */
static NTSTATUS
SmpCallbackServer (PSM_PORT_MESSAGE Request,
           PSM_CLIENT_DATA ClientData)
{
    NTSTATUS          Status = STATUS_SUCCESS;
    PSM_CONNECT_DATA  ConnectData = SmpGetConnectData (Request);
    UNICODE_STRING    CallbackPortName;
    ULONG             CallbackPortNameLength = SM_SB_NAME_MAX_LENGTH; /* TODO: compute length */
    SB_CONNECT_DATA   SbConnectData;
    ULONG             SbConnectDataLength = sizeof SbConnectData;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;

    DPRINT("SM: %s called\n", __FUNCTION__);

    if (	((USHORT)-1 == ConnectData->SubSystemId) ||
        (IMAGE_SUBSYSTEM_NATIVE  == ConnectData->SubSystemId))
    {
        DPRINT("SM: %s: we do not need calling back SM!\n",
                __FUNCTION__);
        return STATUS_SUCCESS;
    }
    RtlCopyMemory (ClientData->SbApiPortName,
               ConnectData->SbName,
               CallbackPortNameLength);
    RtlInitUnicodeString (& CallbackPortName,
                  ClientData->SbApiPortName);

    SecurityQos.Length              = sizeof (SecurityQos);
    SecurityQos.ImpersonationLevel  = SecurityIdentification;
    SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQos.EffectiveOnly       = TRUE;

    SbConnectData.SmApiMax = (sizeof SmApi / sizeof SmApi[0]);
    Status = NtConnectPort (& ClientData->SbApiPort,
                & CallbackPortName,
                &SecurityQos,
                NULL,
                NULL,
                NULL,
                & SbConnectData,
                & SbConnectDataLength);
    return Status;
}

/**********************************************************************
 * NAME
 * 	SmpApiConnectedThread/1
 *
 * DESCRIPTION
 * 	Entry point for the listener thread of LPC port "\SmApiPort".
 */
VOID NTAPI
SmpApiConnectedThread(PVOID pConnectedPort)
{
    NTSTATUS	Status = STATUS_SUCCESS;
    PPORT_MESSAGE	Reply = NULL;
    SM_PORT_MESSAGE	Request;
    HANDLE          ConnectedPort = * (PHANDLE) pConnectedPort;

    DPRINT("SM: %s called\n", __FUNCTION__);
    RtlZeroMemory(&Request, sizeof(SM_PORT_MESSAGE));

    while (TRUE)
    {
        DPRINT("SM: %s: waiting for message\n",__FUNCTION__);

        Status = NtReplyWaitReceivePort(ConnectedPort,
                        NULL,
                        Reply,
                        (PPORT_MESSAGE) & Request);
        if (NT_SUCCESS(Status))
        {
            DPRINT("SM: %s: message received (type=%d)\n",
                __FUNCTION__,
                Request.Header.u2.s2.Type);

            switch (Request.Header.u2.s2.Type)
            {
            case LPC_CONNECTION_REQUEST:
                SmpHandleConnectionRequest (&Request);
                Reply = NULL;
                break;
            case LPC_DEBUG_EVENT:
//				DbgSsHandleKmApiMsg (&Request, 0);
                Reply = NULL;
                break;
            case LPC_PORT_CLOSED:
                  Reply = NULL;
                  continue;
            default:
                if ((Request.SmHeader.ApiIndex) &&
                    (Request.SmHeader.ApiIndex < (sizeof SmApi / sizeof SmApi[0])))
                {
                    Status = SmApi[Request.SmHeader.ApiIndex](&Request);
                        Reply = (PPORT_MESSAGE) & Request;
                } else {
                    Request.SmHeader.Status = STATUS_NOT_IMPLEMENTED;
                    Reply = (PPORT_MESSAGE) & Request;
                }
            }
        } else {
            /* LPC failed */
            DPRINT1("SM: %s: NtReplyWaitReceivePort() failed (Status=0x%08lx)\n",
                __FUNCTION__, Status);
            break;
        }
    }
    NtClose (ConnectedPort);
    DPRINT("SM: %s done\n", __FUNCTION__);
    NtTerminateThread (NtCurrentThread(), Status);
}

/**********************************************************************
 * NAME
 *	SmpHandleConnectionRequest/1
 *
 * ARGUMENTS
 * 	Request: LPC connection request message
 *
 * REMARKS
 * 	Quoted in http://support.microsoft.com/kb/258060/EN-US/
 */
NTSTATUS NTAPI
SmpHandleConnectionRequest (PSM_PORT_MESSAGE Request)
{
    PSM_CONNECT_DATA ConnectData = SmpGetConnectData (Request);
    NTSTATUS         Status = STATUS_SUCCESS;
    BOOL             Accept = FALSE;
    PSM_CLIENT_DATA  ClientData = NULL;
    HANDLE           hClientDataApiPort = (HANDLE) 0;
    PHANDLE          ClientDataApiPort = & hClientDataApiPort;
    HANDLE           hClientDataApiPortThread = (HANDLE) 0;
    PHANDLE          ClientDataApiPortThread = & hClientDataApiPortThread;
    PVOID            Context = NULL;

    DPRINT("SM: %s called:\n  SubSystemID=%d\n  SbName=\"%S\"\n",
            __FUNCTION__, ConnectData->SubSystemId, ConnectData->SbName);

    if(sizeof (SM_CONNECT_DATA) == Request->Header.u1.s1.DataLength)
    {
        if(IMAGE_SUBSYSTEM_UNKNOWN == ConnectData->SubSystemId)
        {
            /*
             * This is not a call to register an image set,
             * but a simple connection request from a process
             * that will use the SM API.
             */
            DPRINT("SM: %s: simple request\n", __FUNCTION__);
            ClientDataApiPort = & hClientDataApiPort;
            ClientDataApiPortThread = & hClientDataApiPortThread;
            Accept = TRUE;
        } else {
            DPRINT("SM: %s: request to register an image set\n", __FUNCTION__);
            /*
             *  Reject GUIs classes: only odd subsystem IDs are
             *  allowed to register here (tty mode images).
             */
            if(1 == (ConnectData->SubSystemId % 2))
            {
                DPRINT("SM: %s: id = %d\n", __FUNCTION__, ConnectData->SubSystemId);
                /*
                 * SmBeginClientInitialization/2 will succeed only if there
                 * is a candidate client ready.
                 */
                Status = SmBeginClientInitialization (Request, & ClientData);
                if(STATUS_SUCCESS == Status)
                {
                    DPRINT("SM: %s: ClientData = %p\n",
                        __FUNCTION__, ClientData);
                    /*
                     * OK: the client is an environment subsystem
                     * willing to manage a free image type.
                     */
                    ClientDataApiPort = & ClientData->ApiPort;
                    ClientDataApiPortThread = & ClientData->ApiPortThread;
                    /*
                     * Call back the candidate environment subsystem
                     * server (use the port name sent in in the
                     * connection request message).
                     */
                    Status = SmpCallbackServer (Request, ClientData);
                    if(NT_SUCCESS(Status))
                    {
                        DPRINT("SM: %s: SmpCallbackServer OK\n",
                            __FUNCTION__);
                        Accept = TRUE;
                    } else {
                        DPRINT("SM: %s: SmpCallbackServer failed (Status=%08lx)\n",
                            __FUNCTION__, Status);
                        Status = SmDestroyClient (ConnectData->SubSystemId);
                    }
                }
            }
        }
    }
    DPRINT("SM: %s: before NtAcceptConnectPort\n", __FUNCTION__);

    Status = NtAcceptConnectPort (ClientDataApiPort,
                      Context,
                      (PPORT_MESSAGE) Request,
                      Accept,
                      NULL,
                      NULL);

    if(Accept)
    {
        if(!NT_SUCCESS(Status))
        {
            DPRINT1("SM: %s: NtAcceptConnectPort() failed (Status=0x%08lx)\n",
                __FUNCTION__, Status);
            return Status;
        } else {
            DPRINT("SM: %s: completing connection request\n", __FUNCTION__);
            Status = NtCompleteConnectPort (*ClientDataApiPort);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("SM: %s: NtCompleteConnectPort() failed (Status=0x%08lx)\n",
                    __FUNCTION__, Status);
                return Status;
            }
        }
        Status = STATUS_SUCCESS;
    }
    DPRINT("SM: %s done\n", __FUNCTION__);
    return Status;
}


/* LPC PORT INITIALIZATION **************************************************/


/**********************************************************************
 * NAME
 * 	SmCreateApiPort/0
 *
 * DECRIPTION
 */
NTSTATUS
SmCreateApiPort(VOID)
{
  OBJECT_ATTRIBUTES  ObjectAttributes = {0};
  UNICODE_STRING     UnicodeString = RTL_CONSTANT_STRING(SM_API_PORT_NAME);
  NTSTATUS           Status = STATUS_SUCCESS;

  InitializeObjectAttributes(&ObjectAttributes,
                 &UnicodeString,
                 0,
                 NULL,
                 NULL);

  Status = NtCreatePort(&SmApiPort,
            &ObjectAttributes,
            sizeof(SM_CONNECT_DATA),
            sizeof(SM_PORT_MESSAGE),
            0);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  /*
   * Create one thread for the named LPC
   * port \SmApiPort
   */
  RtlCreateUserThread(NtCurrentProcess(),
              NULL,
              FALSE,
              0,
              0,
              0,
              (PTHREAD_START_ROUTINE)SmpApiConnectedThread,
              &SmApiPort,
              NULL,
              NULL);

    /*
    * On NT LPC, we need a second thread to handle incoming connections
     * generated by incoming requests, otherwise the thread handling
     * the request will be busy sending the LPC message, without any other
     * thread being busy to receive the LPC message.
     */
    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 FALSE,
                                 0,
                                 0,
                                 0,
                                 (PTHREAD_START_ROUTINE)SmpApiConnectedThread,
                                 &SmApiPort,
                                 NULL,
                                 NULL);

  return(Status);
}

/* EOF */
