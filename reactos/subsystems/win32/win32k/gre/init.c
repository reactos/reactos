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

//#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

HANDLE GlobalUserHeap = NULL;
PSECTION_OBJECT GlobalUserHeapSection = NULL;

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
APIENTRY
Win32kProcessCallout(PEPROCESS Process,
                     BOOLEAN Create)
{
    PW32PROCESS Win32Process;

    DPRINT("Enter Win32kProcessCallback\n");

    /* Get the Win32 Process */
    Win32Process = PsGetProcessWin32Process(Process);

    /* Allocate one if needed */
    if (!Win32Process)
    {
        /* FIXME - lock the process */
        Win32Process = ExAllocatePoolWithTag(NonPagedPool,
            sizeof(PROCESSINFO),
            TAG('W', '3', '2', 'p'));

        if (!Win32Process)
            return STATUS_NO_MEMORY;

        RtlZeroMemory(Win32Process, sizeof(PROCESSINFO));

        PsSetProcessWin32Process(Process, Win32Process);
        /* FIXME - unlock the process */
    }

    if (Create)
    {
        SIZE_T ViewSize = 0;
        LARGE_INTEGER Offset;
        PVOID UserBase = NULL;
        NTSTATUS Status;
        extern PSECTION_OBJECT GlobalUserHeapSection;
        DPRINT("Creating W32 process PID:%d at IRQ level: %lu\n", Process->UniqueProcessId, KeGetCurrentIrql());

        /* map the global heap into the process */
        Offset.QuadPart = 0;
        Status = MmMapViewOfSection(GlobalUserHeapSection,
                                    PsGetCurrentProcess(),
                                    &UserBase,
                                    0,
                                    0,
                                    &Offset,
                                    &ViewSize,
                                    ViewUnmap,
                                    SEC_NO_CHANGE,
                                    PAGE_EXECUTE_READ);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to map the global heap! 0x%x\n", Status);
            return Status;
        }
        Win32Process->HeapMappings.Next = NULL;
        Win32Process->HeapMappings.KernelMapping = (PVOID)GlobalUserHeap;
        Win32Process->HeapMappings.UserMapping = UserBase;
        Win32Process->HeapMappings.Count = 1;

        InitializeListHead(&Win32Process->ClassList);
        InitializeListHead(&Win32Process->MenuListHead);
        InitializeListHead(&Win32Process->PrivateFontListHead);
        ExInitializeFastMutex(&Win32Process->PrivateFontListLock);
        InitializeListHead(&Win32Process->DriverObjListHead);
        ExInitializeFastMutex(&Win32Process->DriverObjListLock);

        if(Process->Peb != NULL)
        {
            /* map the gdi handle table to user land */
            //Process->Peb->GdiSharedHandleTable = GDI_MapHandleTable(GdiTableSection, Process);
            //Process->Peb->GdiDCAttributeList = GDI_BATCH_LIMIT;
        }

        /* setup process flags */
        Win32Process->W32PF_flags = 0;
    }
    else
    {
        DPRINT("Destroying W32 process PID:%d at IRQ level: %lu\n", Process->UniqueProcessId, KeGetCurrentIrql());
    }

    DPRINT("Leave Win32kProcessCallback\n");
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

NTSTATUS
APIENTRY
NtGdiFlushUserBatch(VOID)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/* DRIVER ENTRYPOINT *********************************************************/

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    WIN32_CALLOUTS_FPNS CalloutData;
    PVOID GlobalUserHeapBase = NULL;

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

    /* Create global heap */
    GlobalUserHeap = UserCreateHeap(&GlobalUserHeapSection,
                                    &GlobalUserHeapBase,
                                    1 * 1024 * 1024); /* FIXME - 1 MB for now... */
    if (GlobalUserHeap == NULL)
    {
        DPRINT1("Failed to initialize the global heap!\n");
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}
