/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            server/main/misc.c
 * PURPOSE:         Misc stuff
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

//#define NDEBUG
#include <debug.h>

#include <ntstatus.h>
#include <shutdown.h>
#include <csr.h>

extern PEPROCESS CsrProcess;

NTSTATUS
APIENTRY
NtUserSetInformationThread(IN HANDLE ThreadHandle,
                           IN USERTHREADINFOCLASS ThreadInformationClass,
                           IN PVOID ThreadInformation,
                           IN ULONG ThreadInformationLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PETHREAD Thread;

    /* Allow only CSRSS to perform this operation */
    if (PsGetCurrentProcess() != CsrProcess)
        return STATUS_ACCESS_DENIED;

    UserEnterExclusive();

    /* Get the Thread */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SET_INFORMATION,
                                       *PsThreadType,
                                       UserMode,
                                       (PVOID)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status)) goto Quit;

    switch (ThreadInformationClass)
    {
        case UserThreadInitiateShutdown:
        {
            ULONG CapturedFlags = 0;

            DPRINT("Shutdown initiated\n");

            if (ThreadInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Capture the caller value */
            Status = STATUS_SUCCESS;
            _SEH2_TRY
            {
                ProbeForWrite(ThreadInformation, sizeof(CapturedFlags), sizeof(PVOID));
                CapturedFlags = *(PULONG)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            Status = UserInitiateShutdown(Thread, &CapturedFlags);

            /* Return the modified value to the caller */
            _SEH2_TRY
            {
                *(PULONG)ThreadInformation = CapturedFlags;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            break;
        }

        case UserThreadEndShutdown:
        {
            NTSTATUS ShutdownStatus;

            DPRINT("Shutdown ended\n");

            if (ThreadInformationLength != sizeof(ShutdownStatus))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Capture the caller value */
            Status = STATUS_SUCCESS;
            _SEH2_TRY
            {
                ProbeForRead(ThreadInformation, sizeof(ShutdownStatus), sizeof(PVOID));
                ShutdownStatus = *(NTSTATUS*)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            Status = UserEndShutdown(Thread, ShutdownStatus);
            break;
        }

        case UserThreadCsrApiPort:
        {
            HANDLE CsrPortHandle;

            DPRINT("Set CSR API Port for Win32k\n");

            if (ThreadInformationLength != sizeof(CsrPortHandle))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Capture the caller value */
            Status = STATUS_SUCCESS;
            _SEH2_TRY
            {
                ProbeForRead(ThreadInformation, sizeof(CsrPortHandle), sizeof(PVOID));
                CsrPortHandle = *(PHANDLE)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            Status = STATUS_NOT_IMPLEMENTED;//InitCsrApiPort(CsrPortHandle);
            UNIMPLEMENTED;
            break;
        }

        default:
        {
            UNIMPLEMENTED;
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }

    ObDereferenceObject(Thread);

Quit:
    UserLeave();
    return Status;
}
