/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            dll/ntdll/csr/connect.c
 * PURPOSE:         Routines for connecting and calling CSR
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *******************************************************************/

#include <ntdll.h>

#include <ndk/lpcfuncs.h>
#include <csr/csrsrv.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

HANDLE CsrApiPort;
HANDLE CsrProcessId;
HANDLE CsrPortHeap;
ULONG_PTR CsrPortMemoryDelta;
BOOLEAN InsideCsrProcess = FALSE;

typedef NTSTATUS
(NTAPI *PCSR_SERVER_API_ROUTINE)(IN PPORT_MESSAGE Request,
                                 IN PPORT_MESSAGE Reply);

PCSR_SERVER_API_ROUTINE CsrServerApiRoutine;

#define UNICODE_PATH_SEP L"\\"

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
CsrpConnectToServer(IN PWSTR ObjectDirectory)
{
    NTSTATUS Status;
    ULONG PortNameLength;
    UNICODE_STRING PortName;
    LARGE_INTEGER CsrSectionViewSize;
    HANDLE CsrSectionHandle;
    PORT_VIEW LpcWrite;
    REMOTE_PORT_VIEW LpcRead;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    SID_IDENTIFIER_AUTHORITY NtSidAuthority = {SECURITY_NT_AUTHORITY};
    PSID SystemSid = NULL;
    CSR_API_CONNECTINFO ConnectionInfo;
    ULONG ConnectionInfoLength = sizeof(CSR_API_CONNECTINFO);

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
    PortName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, PortNameLength);
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
                             SEC_RESERVE,
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
    ConnectionInfo.DebugFlags = 0;

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
    RtlFreeSid(SystemSid);
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
    CsrProcessId = ConnectionInfo.ServerProcessId;

    /* Save CSR Section data */
    NtCurrentPeb()->ReadOnlySharedMemoryBase = ConnectionInfo.SharedSectionBase;
    NtCurrentPeb()->ReadOnlySharedMemoryHeap = ConnectionInfo.SharedSectionHeap;
    NtCurrentPeb()->ReadOnlyStaticServerData = ConnectionInfo.SharedStaticServerData;

    /* Create the port heap */
    CsrPortHeap = RtlCreateHeap(0,
                                LpcWrite.ViewBase,
                                LpcWrite.ViewSize,
                                PAGE_SIZE,
                                0,
                                0);
    if (CsrPortHeap == NULL)
    {
        /* Failure */
        DPRINT1("Couldn't create heap for CSR port\n");
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
CsrClientConnectToServer(IN PWSTR ObjectDirectory,
                         IN ULONG ServerId,
                         IN PVOID ConnectionInfo,
                         IN OUT PULONG ConnectionInfoSize,
                         OUT PBOOLEAN ServerToServerCall)
{
    NTSTATUS Status;
    PIMAGE_NT_HEADERS NtHeader;
    UNICODE_STRING CsrSrvName;
    HANDLE hCsrSrv;
    ANSI_STRING CsrServerRoutineName;
    CSR_API_MESSAGE ApiMessage;
    PCSR_CLIENT_CONNECT ClientConnect = &ApiMessage.Data.CsrClientConnect;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    DPRINT("CsrClientConnectToServer: %lx %p\n", ServerId, ConnectionInfo);

    /* Validate the Connection Info */
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
    if (InsideCsrProcess)
    {
        /* We're inside, so let's find csrsrv */
        DPRINT("Next-GEN CSRSS support\n");
        RtlInitUnicodeString(&CsrSrvName, L"csrsrv");
        Status = LdrGetDllHandle(NULL,
                                 NULL,
                                 &CsrSrvName,
                                 &hCsrSrv);

        /* Now get the Server to Server routine */
        RtlInitAnsiString(&CsrServerRoutineName, "CsrCallServerFromServer");
        Status = LdrGetProcedureAddress(hCsrSrv,
                                        &CsrServerRoutineName,
                                        0L,
                                        (PVOID*)&CsrServerApiRoutine);

        /* Use the local heap as port heap */
        CsrPortHeap = RtlGetProcessHeap();

        /* Tell the caller we're inside the server */
        if (ServerToServerCall) *ServerToServerCall = InsideCsrProcess;
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
            Status = CsrpConnectToServer(ObjectDirectory);
            if (!NT_SUCCESS(Status))
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
        CaptureBuffer = CsrAllocateCaptureBuffer(1, ClientConnect->ConnectionInfoSize);
        if (CaptureBuffer == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Capture the connection info data */
        CsrCaptureMessageBuffer(CaptureBuffer,
                                ConnectionInfo,
                                ClientConnect->ConnectionInfoSize,
                                &ClientConnect->ConnectionInfo);

        /* Return the allocated length */
        *ConnectionInfoSize = ClientConnect->ConnectionInfoSize;

        /* Call CSR */
        Status = CsrClientCallServer(&ApiMessage,
                                     CaptureBuffer,
                                     CSR_CREATE_API_NUMBER(CSRSRV_SERVERDLL_INDEX, CsrpClientConnect),
                                     sizeof(CSR_CLIENT_CONNECT));

        /* Copy the updated connection info data back into the user buffer */
        RtlMoveMemory(ConnectionInfo,
                      ClientConnect->ConnectionInfo,
                      *ConnectionInfoSize);

        /* Free the capture buffer */
        CsrFreeCaptureBuffer(CaptureBuffer);
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

#if 0
//
// Structures can be padded at the end, causing the size of the entire structure
// minus the size of the last field, not to be equal to the offset of the last
// field.
//
typedef struct _TEST_EMBEDDED
{
    ULONG One;
    ULONG Two;
    ULONG Three;
} TEST_EMBEDDED;

typedef struct _TEST
{
    PORT_MESSAGE h;
    TEST_EMBEDDED Three;
} TEST;

C_ASSERT(sizeof(PORT_MESSAGE) == 0x18);
C_ASSERT(FIELD_OFFSET(TEST, Three) == 0x18);
C_ASSERT(sizeof(TEST_EMBEDDED) == 0xC);

C_ASSERT(sizeof(TEST) != (sizeof(TEST_EMBEDDED) + sizeof(PORT_MESSAGE)));
C_ASSERT((sizeof(TEST) - sizeof(TEST_EMBEDDED)) != FIELD_OFFSET(TEST, Three));
#endif

/*
 * @implemented
 */
NTSTATUS
NTAPI
CsrClientCallServer(IN OUT PCSR_API_MESSAGE ApiMessage,
                    IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer OPTIONAL,
                    IN CSR_API_NUMBER ApiNumber,
                    IN ULONG DataLength)
{
    NTSTATUS Status;
    ULONG PointerCount;
    PULONG_PTR OffsetPointer;

    /* Fill out the Port Message Header */
    ApiMessage->Header.u2.ZeroInit = 0;
    ApiMessage->Header.u1.s1.TotalLength = DataLength +
        sizeof(CSR_API_MESSAGE) - sizeof(ApiMessage->Data); // FIELD_OFFSET(CSR_API_MESSAGE, Data) + DataLength;
    ApiMessage->Header.u1.s1.DataLength = DataLength +
        FIELD_OFFSET(CSR_API_MESSAGE, Data) - sizeof(ApiMessage->Header); // ApiMessage->Header.u1.s1.TotalLength - sizeof(PORT_MESSAGE);

    /* Fill out the CSR Header */
    ApiMessage->ApiNumber = ApiNumber;
    ApiMessage->CsrCaptureData = NULL;

    DPRINT("API: %lx, u1.s1.DataLength: %x, u1.s1.TotalLength: %x\n",
           ApiNumber,
           ApiMessage->Header.u1.s1.DataLength,
           ApiMessage->Header.u1.s1.TotalLength);

    /* Check if we are already inside a CSR Server */
    if (!InsideCsrProcess)
    {
        /* Check if we got a Capture Buffer */
        if (CaptureBuffer)
        {
            /*
             * We have to convert from our local (client) view
             * to the remote (server) view.
             */
            ApiMessage->CsrCaptureData = (PCSR_CAPTURE_BUFFER)
                ((ULONG_PTR)CaptureBuffer + CsrPortMemoryDelta);

            /* Lock the buffer. */
            CaptureBuffer->BufferEnd = NULL;

            /*
             * Each client pointer inside the CSR message is converted into
             * a server pointer, and each pointer to these message pointers
             * is converted into an offset.
             */
            PointerCount  = CaptureBuffer->PointerCount;
            OffsetPointer = CaptureBuffer->PointerOffsetsArray;
            while (PointerCount--)
            {
                if (*OffsetPointer != 0)
                {
                    *(PULONG_PTR)*OffsetPointer += CsrPortMemoryDelta;
                    *OffsetPointer -= (ULONG_PTR)ApiMessage;
                }
                ++OffsetPointer;
            }
        }

        /* Send the LPC Message */
        Status = NtRequestWaitReplyPort(CsrApiPort,
                                        &ApiMessage->Header,
                                        &ApiMessage->Header);

        /* Check if we got a Capture Buffer */
        if (CaptureBuffer)
        {
            /*
             * We have to convert back from the remote (server) view
             * to our local (client) view.
             */
            ApiMessage->CsrCaptureData = (PCSR_CAPTURE_BUFFER)
                ((ULONG_PTR)ApiMessage->CsrCaptureData - CsrPortMemoryDelta);

            /*
             * Convert back the offsets into pointers to CSR message
             * pointers, and convert back these message server pointers
             * into client pointers.
             */
            PointerCount  = CaptureBuffer->PointerCount;
            OffsetPointer = CaptureBuffer->PointerOffsetsArray;
            while (PointerCount--)
            {
                if (*OffsetPointer != 0)
                {
                    *OffsetPointer += (ULONG_PTR)ApiMessage;
                    *(PULONG_PTR)*OffsetPointer -= CsrPortMemoryDelta;
                }
                ++OffsetPointer;
            }
        }

        /* Check for success */
        if (!NT_SUCCESS(Status))
        {
            /* We failed. Overwrite the return value with the failure. */
            DPRINT1("LPC Failed: %lx\n", Status);
            ApiMessage->Status = Status;
        }
    }
    else
    {
        /* This is a server-to-server call. Save our CID and do a direct call. */
        DPRINT("Next gen server-to-server call\n");

        /* We check this equality inside CsrValidateMessageBuffer */
        ApiMessage->Header.ClientId = NtCurrentTeb()->ClientId;

        Status = CsrServerApiRoutine(&ApiMessage->Header,
                                     &ApiMessage->Header);

        /* Check for success */
        if (!NT_SUCCESS(Status))
        {
            /* We failed. Overwrite the return value with the failure. */
            ApiMessage->Status = Status;
        }
    }

    /* Return the CSR Result */
    DPRINT("Got back: 0x%lx\n", ApiMessage->Status);
    return ApiMessage->Status;
}

/*
 * @implemented
 */
HANDLE
NTAPI
CsrGetProcessId(VOID)
{
    return CsrProcessId;
}

/* EOF */
