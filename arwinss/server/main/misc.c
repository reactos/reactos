/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            server/main/misc.c
 * PURPOSE:         Misc stuff
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

#include <ntstatus.h>
#include <shutdown.h>
#include <csr.h>

extern PEPROCESS CsrProcess;
extern HWND hwndSAS;

/* Registered logon process ID */
HANDLE gpidLogon = 0;

BOOL
APIENTRY
NtUserSetLogonNotifyWindow(HWND hWnd)
{
    if (gpidLogon != PsGetCurrentProcessId())
        return FALSE;

    DPRINT("Logon hwnd %x\n", hWnd);

    hwndSAS = hWnd;

    return TRUE;
}


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

BOOL
UserRegisterLogonProcess(HANDLE ProcessId, BOOL Register)
{
    NTSTATUS Status;
    PEPROCESS Process;

    Status = PsLookupProcessByProcessId(ProcessId, &Process);
    if (!NT_SUCCESS(Status))
    {
        EngSetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    ProcessId = Process->UniqueProcessId;
    ObDereferenceObject(Process);

    if (Register)
    {
        /* Register the logon process */
        if (gpidLogon != 0) return FALSE;
        gpidLogon = ProcessId;
    }
    else
    {
        /* Deregister the logon process */
        if (gpidLogon != ProcessId) return FALSE;
        gpidLogon = 0;
    }

    return TRUE;
}

DWORD_PTR
APIENTRY
NtUserCallTwoParam(
    DWORD_PTR Param1,
    DWORD_PTR Param2,
    DWORD Routine)
{
    DWORD_PTR ReturnValue;

    DPRINT("Enter NtUserCallTwoParam\n");
    UserEnterExclusive();

    switch(Routine)
    {
        case TWOPARAM_ROUTINE_REGISTERLOGONPROCESS:
            ReturnValue = (DWORD_PTR)UserRegisterLogonProcess((HANDLE)Param1, (BOOL)Param2);
            break;

        default:
            DPRINT1("Calling invalid routine number 0x%x in NtUserCallTwoParam(), Param1=0x%x Parm2=0x%x\n",
                Routine, Param1, Param2);
            EngSetLastError(ERROR_INVALID_PARAMETER);
    }

    DPRINT("Leave NtUserCallTwoParam, ret=%p\n", ReturnValue);
    UserLeave();
    return ReturnValue;
}
