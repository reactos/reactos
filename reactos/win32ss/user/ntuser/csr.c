/* 
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Interface between Win32k and USERSRV
 * FILE:             subsystems/win32/win32k/ntuser/csr.c
 * PROGRAMER:        Hermes Belusca-Maito (hermes.belusca@sfr.fr), based on
 *                   the original code by Ge van Geldorp (ge@gse.nl) and by
 *                   the CSR code in NTDLL.
 */

#include <win32k.h>

DBG_DEFAULT_CHANNEL(UserCsr);

PEPROCESS gpepCSRSS = NULL;
PVOID CsrApiPort = NULL;

VOID
InitCsrProcess(VOID /*IN PEPROCESS CsrProcess*/)
{
    /* Save the EPROCESS of CSRSS */
    gpepCSRSS = PsGetCurrentProcess();
    // gpepCSRSS = CsrProcess;
    ObReferenceObject(gpepCSRSS);
}

VOID
ResetCsrProcess(VOID)
{
    if (gpepCSRSS)
        ObDereferenceObject(gpepCSRSS);

    gpepCSRSS = NULL;
}

NTSTATUS
InitCsrApiPort(IN HANDLE CsrPortHandle)
{
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(CsrPortHandle,
                                       0,
                                       /* * */LpcPortObjectType, // or NULL,
                                       UserMode,
                                       &CsrApiPort,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        CsrApiPort = NULL;
        ERR("Failed to set CSR API Port.\n");
    }

    return Status;
}

VOID
ResetCsrApiPort(VOID)
{
    if (CsrApiPort)
        ObDereferenceObject(CsrApiPort);

    CsrApiPort = NULL;
}

/*
 * Function copied from ntdll/csr/connect.c::CsrClientCallServer
 * and adapted for kernel-mode.
 *
 * NOTE: This is really a co_* function!
 */
NTSTATUS
NTAPI
CsrClientCallServer(IN OUT PCSR_API_MESSAGE ApiMessage,
                    IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer OPTIONAL,
                    IN CSR_API_NUMBER ApiNumber,
                    IN ULONG DataLength)
{
    NTSTATUS Status;
#if 0
    ULONG PointerCount;
    PULONG_PTR OffsetPointer;
#endif

    /* Do we have a connection to CSR yet? */
    if (!CsrApiPort)
        return STATUS_INVALID_PORT_HANDLE;

    /* Fill out the Port Message Header */
    ApiMessage->Header.u2.ZeroInit = 0;
    ApiMessage->Header.u1.s1.TotalLength = DataLength +
        sizeof(CSR_API_MESSAGE) - sizeof(ApiMessage->Data); // FIELD_OFFSET(CSR_API_MESSAGE, Data) + DataLength;
    ApiMessage->Header.u1.s1.DataLength = DataLength +
        FIELD_OFFSET(CSR_API_MESSAGE, Data) - sizeof(ApiMessage->Header); // ApiMessage->Header.u1.s1.TotalLength - sizeof(PORT_MESSAGE);

    /* Fill out the CSR Header */
    ApiMessage->ApiNumber = ApiNumber;
    ApiMessage->CsrCaptureData = NULL;

    TRACE("API: %lx, u1.s1.DataLength: %x, u1.s1.TotalLength: %x\n",
          ApiNumber,
          ApiMessage->Header.u1.s1.DataLength,
          ApiMessage->Header.u1.s1.TotalLength);

#if 0
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
#endif

    UserLeaveCo();

    /* Send the LPC Message */

    // The wait logic below is subject to change in the future. One can
    // imagine adding an external parameter to CsrClientCallServer, or write
    // two versions of CsrClientCallServer, synchronous and asynchronous.
    if (PsGetCurrentProcess() == gpepCSRSS)
    {
        Status = LpcRequestPort(CsrApiPort,
                                &ApiMessage->Header);
    }
    else
    {
        Status = LpcRequestWaitReplyPort(CsrApiPort,
                                         &ApiMessage->Header,
                                         &ApiMessage->Header);
    }

    UserEnterCo();

#if 0
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
#endif

    /* Check for success */
    if (!NT_SUCCESS(Status))
    {
        /* We failed. Overwrite the return value with the failure. */
        ERR("LPC Failed: %lx\n", Status);
        ApiMessage->Status = Status;
    }

    /* Return the CSR Result */
    TRACE("Got back: 0x%lx\n", ApiMessage->Status);
    return ApiMessage->Status;
}

/* EOF */
