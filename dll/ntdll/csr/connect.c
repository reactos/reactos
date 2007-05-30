/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/connect.c
 * PURPOSE:         Routines for connecting and calling CSR
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

HANDLE CsrApiPort;
HANDLE CsrProcessId;
HANDLE CsrPortHeap;
ULONG_PTR CsrPortMemoryDelta;
BOOLEAN InsideCsrProcess = FALSE;
BOOLEAN UsingOldCsr = TRUE;

typedef NTSTATUS
(NTAPI *PCSR_SERVER_API_ROUTINE)(IN PPORT_MESSAGE Request,
                                 IN PPORT_MESSAGE Reply);

PCSR_SERVER_API_ROUTINE CsrServerApiRoutine;

#define UNICODE_PATH_SEP L"\\"
#define CSR_PORT_NAME L"ApiPort"

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
HANDLE
NTAPI
CsrGetProcessId(VOID)
{
    return CsrProcessId;
}

/*
 * @implemented
 */
NTSTATUS 
NTAPI
CsrClientCallServer(PCSR_API_MESSAGE ApiMessage,
                    PCSR_CAPTURE_BUFFER CaptureBuffer OPTIONAL,
                    CSR_API_NUMBER ApiNumber,
                    ULONG RequestLength)
{
    NTSTATUS Status;
    ULONG PointerCount;
    PULONG_PTR Pointers;
    ULONG_PTR CurrentPointer;
    DPRINT("CsrClientCallServer\n");

    /* Fill out the Port Message Header */
    ApiMessage->Header.u2.ZeroInit = 0;
    ApiMessage->Header.u1.s1.DataLength = RequestLength - sizeof(PORT_MESSAGE);
    ApiMessage->Header.u1.s1.TotalLength = RequestLength;

    /* Fill out the CSR Header */
    ApiMessage->Type = ApiNumber;
    //ApiMessage->Opcode = ApiNumber; <- Activate with new CSR
    ApiMessage->CsrCaptureData = NULL;

    DPRINT("API: %lx, u1.s1.DataLength: %x, u1.s1.TotalLength: %x\n", 
           ApiNumber,
           ApiMessage->Header.u1.s1.DataLength,
           ApiMessage->Header.u1.s1.TotalLength);
                
    /* Check if we are already inside a CSR Server */
    if (!InsideCsrProcess)
    {
        /* Check if we got a a Capture Buffer */
        if (CaptureBuffer)
        {
            /* We have to convert from our local view to the remote view */
            ApiMessage->CsrCaptureData = (PVOID)((ULONG_PTR)CaptureBuffer +
                                                 CsrPortMemoryDelta);

            /* Lock the buffer */
            CaptureBuffer->BufferEnd = 0;

            /* Get the pointer information */
            PointerCount = CaptureBuffer->PointerCount;
            Pointers = CaptureBuffer->PointerArray;

            /* Loop through every pointer and convert it */
            DPRINT("PointerCount: %lx\n", PointerCount);
            while (PointerCount--)
            {
                /* Get this pointer and check if it's valid */
                DPRINT("Array Address: %p. This pointer: %p. Data: %lx\n",
                        &Pointers, Pointers, *Pointers);
                if ((CurrentPointer = *Pointers++))
                {
                    /* Update it */
                    DPRINT("CurrentPointer: %lx.\n", *(PULONG_PTR)CurrentPointer);
                    *(PULONG_PTR)CurrentPointer += CsrPortMemoryDelta;
                    Pointers[-1] = CurrentPointer - (ULONG_PTR)ApiMessage;
                    DPRINT("CurrentPointer: %lx.\n", *(PULONG_PTR)CurrentPointer);
                }
            }
        }

        /* Send the LPC Message */
        Status = NtRequestWaitReplyPort(CsrApiPort,
                                        &ApiMessage->Header,
                                        &ApiMessage->Header);

        /* Check if we got a a Capture Buffer */
        if (CaptureBuffer)
        {
            /* We have to convert from the remote view to our remote view */
            DPRINT("Reconverting CaptureBuffer\n");
            ApiMessage->CsrCaptureData = (PVOID)((ULONG_PTR)
                                                 ApiMessage->CsrCaptureData -
                                                 CsrPortMemoryDelta);

            /* Get the pointer information */
            PointerCount = CaptureBuffer->PointerCount;
            Pointers = CaptureBuffer->PointerArray;

            /* Loop through every pointer and convert it */
            while (PointerCount--)
            {
                /* Get this pointer and check if it's valid */
                if ((CurrentPointer = *Pointers++))
                {
                    /* Update it */
                    CurrentPointer += (ULONG_PTR)ApiMessage;
                    Pointers[-1] = CurrentPointer;
                    *(PULONG_PTR)CurrentPointer -= CsrPortMemoryDelta;
                }
            }
        }

        /* Check for success */
        if (!NT_SUCCESS(Status))
        {
            /* We failed. Overwrite the return value with the failure */
            DPRINT1("LPC Failed: %lx\n", Status);
            ApiMessage->Status = Status;
        }
    }
    else
    {
        /* This is a server-to-server call. Save our CID and do a direct call */
        DbgBreakPoint();
        ApiMessage->Header.ClientId = NtCurrentTeb()->Cid;
        Status = CsrServerApiRoutine(&ApiMessage->Header,
                                     &ApiMessage->Header);
       
        /* Check for success */
        if (!NT_SUCCESS(Status))
        {
            /* We failed. Overwrite the return value with the failure */
            ApiMessage->Status = Status;
        }
    }

    /* Return the CSR Result */
    DPRINT("Got back: 0x%lx\n", ApiMessage->Status);
    return ApiMessage->Status;
}

NTSTATUS
NTAPI
CsrConnectToServer(IN PWSTR ObjectDirectory)
{
    ULONG PortNameLength;
    UNICODE_STRING PortName;
    LARGE_INTEGER CsrSectionViewSize;
    NTSTATUS Status;
    HANDLE CsrSectionHandle;
    PORT_VIEW LpcWrite;
    REMOTE_PORT_VIEW LpcRead;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    SID_IDENTIFIER_AUTHORITY NtSidAuthority = {SECURITY_NT_AUTHORITY};
    PSID SystemSid = NULL;
    CSR_CONNECTION_INFO ConnectionInfo;
    ULONG ConnectionInfoLength = sizeof(CSR_CONNECTION_INFO);

    DPRINT("%s(%S)\n", __FUNCTION__, ObjectDirectory);

    /* Binary compatibility with MS KERNEL32 */
    if (NULL == ObjectDirectory)
    {
        ObjectDirectory = L"\\Windows";
    }

    /* Calculate the total port name size */
    PortNameLength = ((wcslen(ObjectDirectory) + 1) * sizeof(WCHAR)) +
                     sizeof(CSR_PORT_NAME);

    /* Set the port name */
    PortName.Length = 0;
    PortName.MaximumLength = PortNameLength;

    /* Allocate a buffer for it */
    PortName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, PortNameLength);
    if (PortName.Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Create the name */
    RtlAppendUnicodeToString(&PortName, ObjectDirectory );
    RtlAppendUnicodeToString(&PortName, UNICODE_PATH_SEP);
    RtlAppendUnicodeToString(&PortName, CSR_PORT_NAME);

    /* Create a section for the port memory */
    CsrSectionViewSize.QuadPart = CSR_CSRSS_SECTION_SIZE;
    Status = NtCreateSection(&CsrSectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &CsrSectionViewSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failure allocating CSR Section\n");
        return Status;
    }

    /* Set up the port view structures to match them with the section */
    LpcWrite.Length = sizeof(PORT_VIEW);
    LpcWrite.SectionHandle = CsrSectionHandle;
    LpcWrite.SectionOffset = 0;
    LpcWrite.ViewSize = CsrSectionViewSize.u.LowPart;
    LpcWrite.ViewBase = 0;
    LpcWrite.ViewRemoteBase = 0;
    LpcRead.Length = sizeof(REMOTE_PORT_VIEW);
    LpcRead.ViewSize = 0;
    LpcRead.ViewBase = 0;

    /* Setup the QoS */
    SecurityQos.ImpersonationLevel = SecurityImpersonation;
    SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQos.EffectiveOnly = TRUE;

    /* Setup the connection info */
    ConnectionInfo.Version = 0x10000;

    /* Create a SID for us */
    Status = RtlAllocateAndInitializeSid(&NtSidAuthority,
                                          1,
                                          SECURITY_LOCAL_SYSTEM_RID,
                                          0,
                                          0,
                                          0,
                                          0,
                                          0,
                                          0,
                                          0,
                                          &SystemSid);
    if (!NT_SUCCESS(Status))
    {
        /* Failure */
        DPRINT1("Couldn't allocate SID\n");
        NtClose(CsrSectionHandle);
        return Status;
    }

    /* Connect to the port */
    Status = NtSecureConnectPort(&CsrApiPort,
                                 &PortName,
                                 &SecurityQos,
                                 &LpcWrite,
                                 SystemSid,
                                 &LpcRead,
                                 NULL,
                                 &ConnectionInfo,
                                 &ConnectionInfoLength);
    NtClose(CsrSectionHandle);
    if (!NT_SUCCESS(Status))
    {
        /* Failure */
        DPRINT1("Couldn't connect to CSR port\n");
        return Status;
    }

    /* Save the delta between the sections, for capture usage later */
    CsrPortMemoryDelta = (ULONG_PTR)LpcWrite.ViewRemoteBase -
                         (ULONG_PTR)LpcWrite.ViewBase;

    /* Save the Process */
    CsrProcessId = ConnectionInfo.ProcessId;

    /* Save CSR Section data */
    NtCurrentPeb()->ReadOnlySharedMemoryBase = ConnectionInfo.SharedSectionBase;
    NtCurrentPeb()->ReadOnlySharedMemoryHeap = ConnectionInfo.SharedSectionHeap;
    NtCurrentPeb()->ReadOnlyStaticServerData = ConnectionInfo.SharedSectionData;

    /* Create the port heap */
    CsrPortHeap = RtlCreateHeap(0,
                                LpcWrite.ViewBase,
                                LpcWrite.ViewSize,
                                PAGE_SIZE,
                                0,
                                0);
    if (CsrPortHeap == NULL)
    {
        NtClose(CsrApiPort);
        CsrApiPort = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Return success */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
CsrClientConnectToServer(PWSTR ObjectDirectory,
                         ULONG ServerId,
                         PVOID ConnectionInfo,
                         PULONG ConnectionInfoSize,
                         PBOOLEAN ServerToServerCall)
{
    NTSTATUS Status;
    PIMAGE_NT_HEADERS NtHeader;
    UNICODE_STRING CsrSrvName;
    HANDLE hCsrSrv;
    ANSI_STRING CsrServerRoutineName;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    CSR_API_MESSAGE RosApiMessage;
    CSR_API_MESSAGE2 ApiMessage;
    PCSR_CLIENT_CONNECT ClientConnect = &ApiMessage.ClientConnect;

    /* Validate the Connection Info */
    DPRINT("CsrClientConnectToServer: %lx %p\n", ServerId, ConnectionInfo);
    if (ConnectionInfo && (!ConnectionInfoSize || !*ConnectionInfoSize))
    {
        DPRINT1("Connection info given, but no length\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if we're inside a CSR Process */
    if (InsideCsrProcess)
    {
        /* Tell the client that we're already inside CSR */
        if (ServerToServerCall) *ServerToServerCall = TRUE;
        return STATUS_SUCCESS;
    }

    /*
     * We might be in a CSR Process but not know it, if this is the first call.
     * So let's find out.
     */
    if (!(NtHeader = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress)))
    {
        /* The image isn't valid */
        DPRINT1("Invalid image\n");
        return STATUS_INVALID_IMAGE_FORMAT;
    }
    InsideCsrProcess = (NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE);

    /* Now we can check if we are inside or not */
    if (InsideCsrProcess && !UsingOldCsr)
    {
        /* We're inside, so let's find csrsrv */
        DbgBreakPoint();
        RtlInitUnicodeString(&CsrSrvName, L"csrsrv");
        Status = LdrGetDllHandle(NULL,
                                 NULL,
                                 &CsrSrvName,
                                 &hCsrSrv);
        RtlFreeUnicodeString(&CsrSrvName);

        /* Now get the Server to Server routine */
        RtlInitAnsiString(&CsrServerRoutineName, "CsrCallServerFromServer");
        Status = LdrGetProcedureAddress(hCsrSrv,
                                        &CsrServerRoutineName,
                                        0L,
                                        (PVOID*)&CsrServerApiRoutine);

        /* Use the local heap as port heap */
        CsrPortHeap = RtlGetProcessHeap();

        /* Tell the caller we're inside the server */
        *ServerToServerCall = InsideCsrProcess;
        return STATUS_SUCCESS;
    }

    /* Now check if connection info is given */
    if (ConnectionInfo)
    {
        /* Well, we're defintely in a client now */
        InsideCsrProcess = FALSE;

        /* Do we have a connection to CSR yet? */
        if (!CsrApiPort)
        {
            /* No, set it up now */
            if (!NT_SUCCESS(Status = CsrConnectToServer(ObjectDirectory)))
            {
                /* Failed */
                DPRINT1("Failure to connect to CSR\n");
                return Status;
            }
        }

        /* Setup the connect message header */
        ClientConnect->ServerId = ServerId;
        ClientConnect->ConnectionInfoSize = *ConnectionInfoSize;

        /* Setup a buffer for the connection info */
        CaptureBuffer = CsrAllocateCaptureBuffer(1,
                                                 ClientConnect->ConnectionInfoSize);
        if (CaptureBuffer == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Allocate a pointer for the connection info*/
        CsrAllocateMessagePointer(CaptureBuffer,
                                  ClientConnect->ConnectionInfoSize,
                                  &ClientConnect->ConnectionInfo);

        /* Copy the data into the buffer */
        RtlMoveMemory(ClientConnect->ConnectionInfo,
                      ConnectionInfo,
                      ClientConnect->ConnectionInfoSize);

        /* Return the allocated length */
        *ConnectionInfoSize = ClientConnect->ConnectionInfoSize;

        /* Call CSR */
#if 0
        Status = CsrClientCallServer(&ApiMessage,
                                     CaptureBuffer,
                                     CSR_MAKE_OPCODE(CsrSrvClientConnect,
                                                     CSR_SRV_DLL),
                                     sizeof(CSR_CLIENT_CONNECT));
#endif
        Status = CsrClientCallServer(&RosApiMessage,
                                     NULL,
                                     MAKE_CSR_API(CONNECT_PROCESS, CSR_NATIVE),
                                     sizeof(CSR_API_MESSAGE));
    }
    else
    {
        /* No connection info, just return */
        Status = STATUS_SUCCESS;
    }

    /* Let the caller know if this was server to server */
    DPRINT("Status was: 0x%lx. Are we in server: 0x%x\n", Status, InsideCsrProcess);
    if (ServerToServerCall) *ServerToServerCall = InsideCsrProcess;
    return Status;
}

/* EOF */
