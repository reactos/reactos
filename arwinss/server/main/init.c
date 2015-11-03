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

#include "object.h"
#include <handle.h>
#include <user.h>

//#define NDEBUG
#include <debug.h>

void init_directories(void);
NTSTATUS FASTCALL InitDcImpl(VOID);

/* GLOBALS *******************************************************************/

PGDI_HANDLE_TABLE GdiHandleTable = NULL;
PSECTION_OBJECT GdiTableSection = NULL;
LIST_ENTRY GlobalDriverListHead;
BOOL gbInitialized;

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
APIENTRY
UserCreateThreadInfo(PETHREAD Thread)
{
    struct _EPROCESS *Process;
    PTHREADINFO Win32Thread;
    PPROCESSINFO Win32Process;

    Process = Thread->ThreadsProcess;

    /* Get the Win32 Thread and Process */
    Win32Thread = PsGetThreadWin32Thread(Thread);
    Win32Process = PsGetProcessWin32Process(Process);
    DPRINT("Win32 thread %p, process %p\n", Win32Thread, Win32Process);

    DPRINT("Creating W32 thread TID:%d PID:%d at IRQ level: %lu. Win32Process %p, desktop %x\n",
        Thread->Tcb.Teb->ClientId.UniqueThread, Thread->Tcb.Teb->ClientId.UniqueProcess, KeGetCurrentIrql(), Win32Process, Win32Process->desktop);

    /* Allocate one if needed */
    if (!Win32Thread)
    {
        /* FIXME - lock the process */
        Win32Thread = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(THREADINFO),
                                            USERTAG_THREADINFO);

        if (!Win32Thread)
            return STATUS_NO_MEMORY;

        RtlZeroMemory(Win32Thread, sizeof(THREADINFO));

        PsSetThreadWin32Thread(Thread, Win32Thread, NULL);
        /* FIXME - unlock the process */
    }

    Win32Thread->process = Win32Process;
    Win32Thread->peThread = Thread;
    Win32Thread->desktop = Win32Process->desktop;
    Win32Thread->KeyboardLayout = UserGetDefaultKeyBoardLayout();

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
UserDestroyThreadInfo(PETHREAD Thread)
{
    struct _EPROCESS *Process;
    PTHREADINFO Win32Thread;
    PPROCESSINFO Win32Process;

    Process = Thread->ThreadsProcess;

    /* Get the Win32 Thread and Process */
    Win32Thread = PsGetThreadWin32Thread(Thread);
    Win32Process = PsGetProcessWin32Process(Process);
    DPRINT("Win32 thread %p, process %p\n", Win32Thread, Win32Process);

    DPRINT("Destroying W32 thread TID:%d at IRQ level: %lu\n", Thread->Tcb.Teb->ClientId.UniqueThread, KeGetCurrentIrql());

    /* USER thread-level cleanup */
    cleanup_clipboard_thread(Win32Thread);
    destroy_thread_windows(Win32Thread);
    free_msg_queue(Win32Thread);
    close_thread_desktop(Win32Thread);

    /* Free THREADINFO */
    PsSetThreadWin32Thread(Thread, NULL, Win32Thread);
    ExFreePoolWithTag(Win32Thread, USERTAG_THREADINFO);

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kProcessCallout(PEPROCESS Process,
                     BOOLEAN Create)
{
    PPROCESSINFO Win32Process;
    NTSTATUS Status;

    /* Get the Win32 Process */
    Win32Process = PsGetProcessWin32Process(Process);
    DPRINT("Win32kProcessCallback(): Win32Process %p, Create %d\n", Win32Process, Create);
    if (Create)
    {
        DPRINT("Creating W32 process PID:%d at IRQ level: %lu\n", Process->UniqueProcessId, KeGetCurrentIrql());

        /* We might be called with an already allocated win32 process */
        if (Win32Process)
        {
            /* There is no more to do for us (this is a success code!) */
            Status = STATUS_ALREADY_WIN32;
            return Status;
        }

        /* Allocate one if needed */
        /* FIXME - lock the process */
        Win32Process = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(PROCESSINFO),
                                             USERTAG_PROCESSINFO);

        if (!Win32Process) return STATUS_NO_MEMORY;

        RtlZeroMemory(Win32Process, sizeof(PROCESSINFO));

        PsSetProcessWin32Process(Process, Win32Process, NULL);
        Win32Process->peProcess = Process;
        /* FIXME - unlock the process */

        /* Create an idle event */
        Status = ZwCreateEvent(&Win32Process->idle_event_handle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, TRUE);
        if (!NT_SUCCESS(Status)) DPRINT1("Creating idle event failed with status 0x%08X\n", Status);

        /* Get a pointer to the object itself */
        Status = ObReferenceObjectByHandle(Win32Process->idle_event_handle,
                                           EVENT_ALL_ACCESS,
                                           NULL,
                                           KernelMode,
                                           (PVOID*)&Win32Process->idle_event,
                                           NULL);
        if (!NT_SUCCESS(Status)) ZwClose(Win32Process->idle_event_handle);

        list_init(&Win32Process->Classes);
        Win32Process->handles = alloc_handle_table(Win32Process, 0);
        connect_process_winstation(Win32Process);

        if(Process->Peb != NULL)
        {
            /* map the gdi handle table to user land */
            Process->Peb->GdiSharedHandleTable = GDI_MapHandleTable(GdiTableSection, Process);
            Process->Peb->GdiDCAttributeList = GDI_BATCH_LIMIT;
        }
    }
    else
    {
        DPRINT("Destroying W32 process PID:%d at IRQ level: %lu\n", Process->UniqueProcessId, KeGetCurrentIrql());

        UserEnterExclusive();

        /* Delete its handles table */
        close_process_handles(Win32Process);

        /* Destroy its classes */
        destroy_process_classes(Win32Process);

        /* Free allocated user handles */
        free_process_user_handles(Win32Process);

        /* Destroy idle event if it exists */
        if (Win32Process->idle_event)
        {
            ObDereferenceObject(Win32Process->idle_event);
            ZwClose(Win32Process->idle_event_handle);
        }

        /* Free the PROCESSINFO */
        PsSetProcessWin32Process(Process, NULL, Win32Process);
        ExFreePoolWithTag(Win32Process, USERTAG_PROCESSINFO);

        UserLeave();
    }

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kThreadCallout(PETHREAD Thread,
                    PSW32THREADCALLOUTTYPE Type)
{
    NTSTATUS Status;

    UserEnterExclusive();

    DPRINT("Enter Win32kThreadCallback, current thread id %d, process id %d, type %d\n",
        PsGetCurrentThread()->Tcb.Teb->ClientId.UniqueThread,
        PsGetCurrentThread()->Tcb.Teb->ClientId.UniqueProcess, Type);

    ASSERT(NtCurrentTeb());

    if (Type == PsW32ThreadCalloutInitialize)
    {
        Status = UserCreateThreadInfo(Thread);
    }
    else
    {
        Status = UserDestroyThreadInfo(Thread);
    }

    DPRINT("Leave Win32kThreadCallback\n");

    UserLeave();

    return Status;
}

NTSTATUS
APIENTRY
Win32kGlobalAtomTableCallout(VOID)
{
    DPRINT("Win32kGlobalAtomTableCallout() is UNIMPLEMENTED\n");
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
NtGdiFlushUserBatch(VOID)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
NtUserInitialize(
  DWORD   dwWinVersion,
  HANDLE  hPowerRequestEvent,
  HANDLE  hMediaRequestEvent)
{
    /* Check the Windows version */
    if (dwWinVersion != USER_VERSION)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Grab the lock exclusively */
    UserEnterExclusive();

    /* Check if already initialized */
    if (gbInitialized)
    {
        /* Release the lock and exit */
        UserLeave();
        return STATUS_UNSUCCESSFUL;
    }

    /* Save CSR process */
    CsrInit();

    /* Create win32 info for the current thread */
    UserCreateThreadInfo(PsGetCurrentThread());

    /* User server is initialized now */
    gbInitialized = TRUE;

    /* Release the lock */
    UserLeave();

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
    CalloutData.DesktopOpenProcedure = (PKWIN32_SESSION_CALLOUT)Win32kDesktopOpenProcedure;
    CalloutData.DesktopOkToCloseProcedure = (PKWIN32_SESSION_CALLOUT)Win32kDesktopOkToCloseProcedure;
    CalloutData.DesktopCloseProcedure = (PKWIN32_SESSION_CALLOUT)Win32kDesktopCloseProcedure;
    CalloutData.DesktopDeleteProcedure = (PKWIN32_SESSION_CALLOUT)Win32kDesktopDeleteProcedure;
    CalloutData.WindowStationOkToCloseProcedure = (PKWIN32_SESSION_CALLOUT)Win32kWindowStationOkToCloseProcedure;
    CalloutData.WindowStationCloseProcedure = (PKWIN32_SESSION_CALLOUT)Win32kWindowStationCloseProcedure;
    CalloutData.WindowStationDeleteProcedure = (PKWIN32_SESSION_CALLOUT)Win32kWindowStationDeleteProcedure;
    CalloutData.WindowStationParseProcedure = (PKWIN32_SESSION_CALLOUT)Win32kWindowStationParseProcedure;
    CalloutData.WindowStationOpenProcedure = (PKWIN32_SESSION_CALLOUT)Win32kWindowStationOpenProcedure;

    /* Register them */
    PsEstablishWin32Callouts(&CalloutData);

    /* Initialize a list of loaded drivers in Win32 subsystem */
    InitializeListHead(&GlobalDriverListHead);

    /* Initialize user implementation */
    UserInitialize();

    /* Init object directories implementation */
    init_directories();

    /* Initialize GDI objects implementation */
    GdiHandleTable = GDIOBJ_iAllocHandleTable(&GdiTableSection);
    if (GdiHandleTable == NULL)
    {
        DPRINT1("Failed to initialize the GDI handle table.\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Create stock objects */
    CreateStockObjects();

    /* Initialize timers */
    InitTimeThread();

    /* Init video driver implementation */
    InitDcImpl();

    /* Initialize window manager */
    SwmInitialize();
    //GrContextInit();

    return STATUS_SUCCESS;
}
