/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/init.c
 * PURPOSE:         Driver Initialization
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

/* System service call table */
#include <include/napi.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
APIENTRY
Win32kWinStaObjectParse(PWIN32_PARSEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

VOID
APIENTRY
Win32kWinStaObjectDelete(PWIN32_DELETEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

VOID
APIENTRY
Win32kDesktopObjectDelete(PWIN32_DELETEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
}


NTSTATUS
APIENTRY
Win32kProcessCallback(struct _EPROCESS *Process,
                      BOOLEAN Create)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kThreadCallback(struct _ETHREAD *Thread,
                     PSW32THREADCALLOUTTYPE Type)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

/* DRIVER ENTRYPOINT *********************************************************/

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    WIN32_CALLOUTS_FPNS CalloutData = {0};

    DPRINT1("Win32k initialization: DO %p, RegPath %wZ\n", DriverObject,
        RegistryPath);

    /* Add system service table */
    if (!KeAddSystemServiceTable(Win32kSSDT,
                                 NULL,
                                 Win32kNumberOfSysCalls,
                                 Win32kSSPT,
                                 1))
    {
        DPRINT1("Error adding system service table!\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Register Win32 callouts */
    CalloutData.WindowStationParseProcedure = Win32kWinStaObjectParse;
    CalloutData.WindowStationDeleteProcedure = Win32kWinStaObjectDelete;
    CalloutData.DesktopDeleteProcedure = Win32kDesktopObjectDelete;
    CalloutData.ProcessCallout = Win32kProcessCallback;
    CalloutData.ThreadCallout = Win32kThreadCallback;
    CalloutData.BatchFlushRoutine = NtGdiFlushUserBatch;

    PsEstablishWin32Callouts((PWIN32_CALLOUTS_FPNS)&CalloutData);

    return STATUS_SUCCESS;
}
