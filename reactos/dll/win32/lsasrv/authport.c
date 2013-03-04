/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/authport.c
 * PURPOSE:     LsaAuthenticationPort server routines
 * COPYRIGHT:   Copyright 2009 Eric Kohl
 */

/* INCLUDES ****************************************************************/


#include "lsasrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);


typedef struct _LSAP_LOGON_CONTEXT
{
    LIST_ENTRY Entry;
    HANDLE ClientProcessHandle;
    HANDLE ConnectionHandle;
} LSAP_LOGON_CONTEXT, *PLSAP_LOGON_CONTEXT;


static LIST_ENTRY LsapLogonContextList;

static HANDLE PortThreadHandle = NULL;
static HANDLE AuthPortHandle = NULL;


/* FUNCTIONS ***************************************************************/

static NTSTATUS
LsapCallAuthenticationPackage(PLSA_API_MSG RequestMsg,
                              PLSAP_LOGON_CONTEXT LogonContext)
{
    TRACE("(%p %p)\n", RequestMsg, LogonContext);

    return STATUS_SUCCESS;
}


static NTSTATUS
LsapDeregisterLogonProcess(PLSA_API_MSG RequestMsg,
                           PLSAP_LOGON_CONTEXT LogonContext)
{
    TRACE("(%p %p)\n", RequestMsg, LogonContext);

    RemoveHeadList(&LogonContext->Entry);

    NtClose(LogonContext->ClientProcessHandle);
    NtClose(LogonContext->ConnectionHandle);

    RtlFreeHeap(RtlGetProcessHeap(), 0, LogonContext);

    return STATUS_SUCCESS;
}


static NTSTATUS
LsapLogonUser(PLSA_API_MSG RequestMsg,
              PLSAP_LOGON_CONTEXT LogonContext)
{
    PVOID LocalAuthInfo = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("(%p %p)\n", RequestMsg, LogonContext);

    TRACE("LogonType: %lu\n", RequestMsg->LogonUser.Request.LogonType);
    TRACE("AuthenticationPackage: 0x%08lx\n", RequestMsg->LogonUser.Request.AuthenticationPackage);
    TRACE("AuthenticationInformation: %p\n", RequestMsg->LogonUser.Request.AuthenticationInformation);
    TRACE("AuthenticationInformationLength: %lu\n", RequestMsg->LogonUser.Request.AuthenticationInformationLength);

    LocalAuthInfo = RtlAllocateHeap(RtlGetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    RequestMsg->LogonUser.Request.AuthenticationInformationLength);
    if (LocalAuthInfo == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    /* Read the authentication info from the callers adress space */
    Status = NtReadVirtualMemory(LogonContext->ClientProcessHandle,
                                 RequestMsg->LogonUser.Request.AuthenticationInformation,
                                 LocalAuthInfo,
                                 RequestMsg->LogonUser.Request.AuthenticationInformationLength,
                                 NULL);
    if (!NT_SUCCESS(Status))
        goto done;

    if (RequestMsg->LogonUser.Request.LogonType == Interactive ||
        RequestMsg->LogonUser.Request.LogonType == Batch ||
        RequestMsg->LogonUser.Request.LogonType == Service)
    {
        PMSV1_0_INTERACTIVE_LOGON LogonInfo;
        ULONG_PTR PtrOffset;

        LogonInfo = (PMSV1_0_INTERACTIVE_LOGON)LocalAuthInfo;

        /* Fix-up pointers in the authentication info */
        PtrOffset = (ULONG_PTR)LocalAuthInfo - (ULONG_PTR)RequestMsg->LogonUser.Request.AuthenticationInformation;

        LogonInfo->LogonDomainName.Buffer = (PWSTR)((ULONG_PTR)LogonInfo->LogonDomainName.Buffer + PtrOffset);
        LogonInfo->UserName.Buffer = (PWSTR)((ULONG_PTR)LogonInfo->UserName.Buffer + PtrOffset);
        LogonInfo->Password.Buffer = (PWSTR)((ULONG_PTR)LogonInfo->Password.Buffer + PtrOffset);

        TRACE("Domain: %S\n", LogonInfo->LogonDomainName.Buffer);
        TRACE("User: %S\n", LogonInfo->UserName.Buffer);
        TRACE("Password: %S\n", LogonInfo->Password.Buffer);
    }
    else
    {
        FIXME("LogonType %lu is not supported yet!\n", RequestMsg->LogonUser.Request.LogonType);
    }



    RequestMsg->LogonUser.Reply.ProfileBuffer = NULL;
    RequestMsg->LogonUser.Reply.ProfileBufferLength = 0;
//     LUID LogonId;
    RequestMsg->LogonUser.Reply.Token = NULL;
//     QUOTA_LIMITS Quotas;
    RequestMsg->LogonUser.Reply.SubStatus = STATUS_SUCCESS;

done:
    if (LocalAuthInfo != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, LocalAuthInfo);

    return Status;
}


static NTSTATUS
LsapLookupAuthenticationPackage(PLSA_API_MSG RequestMsg,
                                PLSAP_LOGON_CONTEXT LogonContext)
{
    STRING PackageName;
    ULONG PackageId;
    NTSTATUS Status;

    TRACE("(%p %p)\n", RequestMsg, LogonContext);
    TRACE("PackageName: %s\n", RequestMsg->LookupAuthenticationPackage.Request.PackageName);

    PackageName.Length = RequestMsg->LookupAuthenticationPackage.Request.PackageNameLength;
    PackageName.MaximumLength = LSASS_MAX_PACKAGE_NAME_LENGTH + 1;
    PackageName.Buffer = RequestMsg->LookupAuthenticationPackage.Request.PackageName;

    Status = LsapLookupAuthenticationPackageByName(&PackageName,
                                                   &PackageId);
    if (NT_SUCCESS(Status))
    {
        RequestMsg->LookupAuthenticationPackage.Reply.Package = PackageId;
    }

    return Status;
}


static NTSTATUS
LsapCheckLogonProcess(PLSA_API_MSG RequestMsg,
                      PLSAP_LOGON_CONTEXT *LogonContext)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ProcessHandle = NULL;
    PLSAP_LOGON_CONTEXT Context = NULL;
    NTSTATUS Status;

    TRACE("(%p)\n", RequestMsg);

    TRACE("Client ID: %p %p\n", RequestMsg->h.ClientId.UniqueProcess, RequestMsg->h.ClientId.UniqueThread);

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenProcess(&ProcessHandle,
                           PROCESS_VM_READ,
                           &ObjectAttributes,
                           &RequestMsg->h.ClientId);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtOpenProcess() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Allocate the logon context */
    Context = RtlAllocateHeap(RtlGetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              sizeof(LSAP_LOGON_CONTEXT));
    if (Context == NULL)
    {
        NtClose(ProcessHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TRACE("New LogonContext: %p\n", Context);

    Context->ClientProcessHandle = ProcessHandle;

    *LogonContext = Context;

    return STATUS_SUCCESS;
}


static NTSTATUS
LsapHandlePortConnection(PLSA_API_MSG RequestMsg)
{
    PLSAP_LOGON_CONTEXT LogonContext = NULL;
    HANDLE ConnectionHandle = NULL;
    BOOLEAN Accept;
    REMOTE_PORT_VIEW RemotePortView;
    NTSTATUS Status;

    TRACE("(%p)\n", RequestMsg);

    TRACE("Logon Process Name: %s\n", RequestMsg->ConnectInfo.LogonProcessNameBuffer);

    Status = LsapCheckLogonProcess(RequestMsg,
                                   &LogonContext);

    RequestMsg->ConnectInfo.OperationalMode = 0x43218765;

    RequestMsg->ConnectInfo.Status = Status;

    if (NT_SUCCESS(Status))
    {
        Accept = TRUE;
    }
    else
    {
        Accept = FALSE;
    }

    RemotePortView.Length = sizeof(REMOTE_PORT_VIEW);
    Status = NtAcceptConnectPort(&ConnectionHandle,
                                 (PVOID*)LogonContext,
                                 &RequestMsg->h,
                                 Accept,
                                 NULL,
                                 &RemotePortView);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtAcceptConnectPort failed (Status 0x%lx)\n", Status);
        return Status;
    }

    if (Accept == TRUE)
    {
        LogonContext->ConnectionHandle = ConnectionHandle;

        InsertHeadList(&LsapLogonContextList,
                       &LogonContext->Entry);

        Status = NtCompleteConnectPort(ConnectionHandle);
        if (!NT_SUCCESS(Status))
        {
            ERR("NtCompleteConnectPort failed (Status 0x%lx)\n", Status);
            return Status;
        }
    }

    return Status;
}


NTSTATUS WINAPI
AuthPortThreadRoutine(PVOID Param)
{
    PLSAP_LOGON_CONTEXT LogonContext;
    PLSA_API_MSG ReplyMsg = NULL;
    LSA_API_MSG RequestMsg;
    NTSTATUS Status;

    TRACE("AuthPortThreadRoutine() called\n");

    Status = STATUS_SUCCESS;

    for (;;)
    {
        Status = NtReplyWaitReceivePort(AuthPortHandle,
                                        (PVOID*)&LogonContext,
                                        &ReplyMsg->h,
                                        &RequestMsg.h);
        if (!NT_SUCCESS(Status))
        {
            TRACE("NtReplyWaitReceivePort() failed (Status %lx)\n", Status);
            break;
        }

        TRACE("Received message\n");

        switch (RequestMsg.h.u2.s2.Type)
        {
            case LPC_CONNECTION_REQUEST:
                TRACE("Port connection request\n");
                Status = LsapHandlePortConnection(&RequestMsg);
                ReplyMsg = NULL;
                break;

            case LPC_PORT_CLOSED:
                TRACE("Port closed\n");
                ReplyMsg = NULL;
                break;

            case LPC_CLIENT_DIED:
                TRACE("Client died\n");
                ReplyMsg = NULL;
                break;

            default:
                TRACE("Received request (ApiNumber: %lu)\n", RequestMsg.ApiNumber);

                switch (RequestMsg.ApiNumber)
                {
                    case LSASS_REQUEST_CALL_AUTHENTICATION_PACKAGE:
                        RequestMsg.Status = LsapCallAuthenticationPackage(&RequestMsg,
                                                                          LogonContext);
                        ReplyMsg = &RequestMsg;
                        break;

                    case LSASS_REQUEST_DEREGISTER_LOGON_PROCESS:

                        ReplyMsg = &RequestMsg;
                        RequestMsg.Status = STATUS_SUCCESS;
                        Status = NtReplyPort(AuthPortHandle,
                                             &ReplyMsg->h);

                        LsapDeregisterLogonProcess(&RequestMsg,
                                                   LogonContext);

                        ReplyMsg = NULL;
                        break;

                    case LSASS_REQUEST_LOGON_USER:
                        RequestMsg.Status = LsapLogonUser(&RequestMsg,
                                                          LogonContext);
                        ReplyMsg = &RequestMsg;
                        break;

                    case LSASS_REQUEST_LOOKUP_AUTHENTICATION_PACKAGE:
                        RequestMsg.Status = LsapLookupAuthenticationPackage(&RequestMsg,
                                                                            LogonContext);
                        ReplyMsg = &RequestMsg;
                        break;

                    default:
                        RequestMsg.Status = STATUS_SUCCESS; /* FIXME */
                        ReplyMsg = &RequestMsg;
                        break;
                }

                break;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
StartAuthenticationPort(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PortName;
    DWORD ThreadId;
    NTSTATUS Status;

    /* Initialize the logon context list */
    InitializeListHead(&LsapLogonContextList);

    RtlInitUnicodeString(&PortName,
                         L"\\LsaAuthenticationPort");

    InitializeObjectAttributes(&ObjectAttributes,
                               &PortName,
                               0,
                               NULL,
                               NULL);

    Status = NtCreatePort(&AuthPortHandle,
                          &ObjectAttributes,
                          sizeof(LSA_CONNECTION_INFO),
                          sizeof(LSA_API_MSG),
                          sizeof(LSA_API_MSG) * 32);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtCreatePort() failed (Status %lx)\n", Status);
        return Status;
    }

    PortThreadHandle = CreateThread(NULL,
                                    0x1000,
                                    (LPTHREAD_START_ROUTINE)AuthPortThreadRoutine,
                                    NULL,
                                    0,
                                    &ThreadId);


    return STATUS_SUCCESS;
}

/* EOF */
