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
Win32kProcessCallout(PEPROCESS Process,
                     BOOLEAN Create)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kThreadCallout(PETHREAD Thread,
                    PSW32THREADCALLOUTTYPE Type)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kGlobalAtomTableCallout(VOID)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kPowerEventCallout(PWIN32_POWEREVENT_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kPowerStateCallout(PWIN32_POWERSTATE_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kJobCallout(PWIN32_JOBCALLOUT_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kDesktopOpenProcedure(PWIN32_OPENMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kDesktopOkToCloseProcedure(PWIN32_OKAYTOCLOSEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kDesktopCloseProcedure(PWIN32_CLOSEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

VOID
APIENTRY
Win32kDesktopDeleteProcedure(PWIN32_DELETEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
}

NTSTATUS
APIENTRY
Win32kWindowStationOkToCloseProcedure(PWIN32_OKAYTOCLOSEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kWindowStationCloseProcedure(PWIN32_CLOSEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

VOID
APIENTRY
Win32kWindowStationDeleteProcedure(PWIN32_DELETEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
}

NTSTATUS
APIENTRY
Win32kWindowStationParseProcedure(PWIN32_PARSEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kWindowStationOpenProcedure(PWIN32_OPENMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kWin32DataCollectionProcedure(PEPROCESS Process,
                                   PVOID Callback,
                                   PVOID Context)
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
    WIN32_CALLOUTS_FPNS CalloutData;

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

    /* Set up Win32 callouts */
    CalloutData.ProcessCallout = Win32kProcessCallout;
    CalloutData.ThreadCallout = Win32kThreadCallout;
    CalloutData.GlobalAtomTableCallout = Win32kGlobalAtomTableCallout;
    CalloutData.PowerEventCallout = Win32kPowerEventCallout;
    CalloutData.PowerStateCallout = Win32kPowerStateCallout;
    CalloutData.JobCallout = Win32kJobCallout;
    CalloutData.BatchFlushRoutine = NtGdiFlushUserBatch;
    CalloutData.DesktopOpenProcedure = Win32kDesktopOpenProcedure;
    CalloutData.DesktopOkToCloseProcedure = Win32kDesktopOkToCloseProcedure;
    CalloutData.DesktopCloseProcedure = Win32kDesktopCloseProcedure;
    CalloutData.DesktopDeleteProcedure = Win32kDesktopDeleteProcedure;
    CalloutData.WindowStationOkToCloseProcedure = Win32kWindowStationOkToCloseProcedure;
    CalloutData.WindowStationCloseProcedure = Win32kWindowStationCloseProcedure;
    CalloutData.WindowStationDeleteProcedure = Win32kWindowStationDeleteProcedure;
    CalloutData.WindowStationParseProcedure = Win32kWindowStationParseProcedure;
    CalloutData.WindowStationOpenProcedure = Win32kWindowStationOpenProcedure;
    CalloutData.Win32DataCollectionProcedure = Win32kWin32DataCollectionProcedure;

    /* Register them */
    PsEstablishWin32Callouts(&CalloutData);

    return STATUS_SUCCESS;
}
