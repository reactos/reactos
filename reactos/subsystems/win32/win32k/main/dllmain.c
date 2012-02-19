/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Driver entry and initialization of win32k
 * FILE:             subsystems/win32/win32k/main/main.c
 * PROGRAMER:
 */

#include <win32k.h>
#include <include/napi.h>

#define NDEBUG
#include <debug.h>

HANDLE hModuleWin;

PGDI_HANDLE_TABLE NTAPI GDIOBJ_iAllocHandleTable(OUT PSECTION_OBJECT *SectionObject);
BOOL NTAPI GDI_CleanupForProcess (struct _EPROCESS *Process);

HANDLE GlobalUserHeap = NULL;
PSECTION_OBJECT GlobalUserHeapSection = NULL;

PSERVERINFO gpsi = NULL; // Global User Server Information.

SHORT gusLanguageID;
PPROCESSINFO ppiScrnSaver;

extern ULONG_PTR Win32kSSDT[];
extern UCHAR Win32kSSPT[];
extern ULONG Win32kNumberOfSysCalls;

void
NTAPI
DbgPreServiceHook(ULONG ulSyscallId, PULONG_PTR pulArguments)
{
    GdiDbgPreServiceHook(ulSyscallId, pulArguments);
    UserDbgPreServiceHook(ulSyscallId, pulArguments);
}

ULONG_PTR
NTAPI
DbgPostServiceHook(ULONG ulSyscallId, ULONG_PTR ulResult)
{
    ulResult = GdiDbgPostServiceHook(ulSyscallId, ulResult);
    ulResult = UserDbgPostServiceHook(ulSyscallId, ulResult);
    return ulResult;
}

NTSTATUS
APIENTRY
Win32kProcessCallback(struct _EPROCESS *Process,
                      BOOLEAN Create)
{
    PPROCESSINFO ppiCurrent;
    DECLARE_RETURN(NTSTATUS);

    ASSERT(Process->Peb);

    DPRINT("Enter Win32kProcessCallback\n");
    UserEnterExclusive();

    if (Create)
    {
        SIZE_T ViewSize = 0;
        LARGE_INTEGER Offset;
        PVOID UserBase = NULL;
        PRTL_USER_PROCESS_PARAMETERS pParams = Process->Peb->ProcessParameters;
        NTSTATUS Status;

        ASSERT(PsGetProcessWin32Process(Process) == NULL);

        ppiCurrent = ExAllocatePoolWithTag(NonPagedPool,
                                           sizeof(PROCESSINFO),
                                           USERTAG_PROCESSINFO);

        if (ppiCurrent == NULL) 
            RETURN( STATUS_NO_MEMORY);

        RtlZeroMemory(ppiCurrent, sizeof(PROCESSINFO));

        PsSetProcessWin32Process(Process, ppiCurrent);

#ifdef DBG
        DbgInitDebugChannels();
#endif

        TRACE_PPI(ppiCurrent, UserProcess,"Allocated ppi for PID:%d\n", Process->UniqueProcessId);

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
                                    PAGE_EXECUTE_READ); /* would prefer PAGE_READONLY, but thanks to RTL heaps... */
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to map the global heap! 0x%x\n", Status);
            RETURN(Status);
        }
        ppiCurrent->HeapMappings.Next = NULL;
        ppiCurrent->HeapMappings.KernelMapping = (PVOID)GlobalUserHeap;
        ppiCurrent->HeapMappings.UserMapping = UserBase;
        ppiCurrent->HeapMappings.Count = 1;

        InitializeListHead(&ppiCurrent->MenuListHead);

        InitializeListHead(&ppiCurrent->GDIBrushAttrFreeList);
        InitializeListHead(&ppiCurrent->GDIDcAttrFreeList);

        InitializeListHead(&ppiCurrent->PrivateFontListHead);
        ExInitializeFastMutex(&ppiCurrent->PrivateFontListLock);

        InitializeListHead(&ppiCurrent->DriverObjListHead);
        ExInitializeFastMutex(&ppiCurrent->DriverObjListLock);

        ppiCurrent->KeyboardLayout = W32kGetDefaultKeyLayout();
        EngCreateEvent((PEVENT *)&ppiCurrent->InputIdleEvent);
        KeInitializeEvent(ppiCurrent->InputIdleEvent, NotificationEvent, FALSE);
        

        /* map the gdi handle table to user land */
        Process->Peb->GdiSharedHandleTable = GDI_MapHandleTable(Process);
        Process->Peb->GdiDCAttributeList = GDI_BATCH_LIMIT;
        pParams = Process->Peb->ProcessParameters;

        ppiCurrent->peProcess = Process;
        /* setup process flags */
        ppiCurrent->W32PF_flags = W32PF_THREADCONNECTED;

        if ( pParams &&
             pParams->WindowFlags & STARTF_SCRNSAVER )
        {
           ppiScrnSaver = ppiCurrent;
           ppiCurrent->W32PF_flags |= W32PF_SCREENSAVER;
        }

        /* Create pools for GDI object attributes */
        ppiCurrent->pPoolDcAttr = GdiPoolCreate(sizeof(DC_ATTR), 'acdG');
        ppiCurrent->pPoolBrushAttr = GdiPoolCreate(sizeof(BRUSH_ATTR), 'arbG');
        ppiCurrent->pPoolRgnAttr = GdiPoolCreate(sizeof(RGN_ATTR), 'agrG');
        ASSERT(ppiCurrent->pPoolDcAttr);
        ASSERT(ppiCurrent->pPoolBrushAttr);
        ASSERT(ppiCurrent->pPoolRgnAttr);
    }
    else
    {
        /* Get the Win32 Process */
        ppiCurrent = PsGetProcessWin32Process(Process);

        ASSERT(ppiCurrent);

        TRACE_CH(UserProcess, "Destroying W32 process PID:%d at IRQ level: %lu\n", Process->UniqueProcessId, KeGetCurrentIrql());
        ppiCurrent->W32PF_flags |= W32PF_TERMINATED;

        if (ppiScrnSaver == ppiCurrent) 
            ppiScrnSaver = NULL;

        if (ppiCurrent->InputIdleEvent)
        {
           EngFreeMem(ppiCurrent->InputIdleEvent);
           ppiCurrent->InputIdleEvent = NULL;
        }

        IntCleanupMenus(Process, ppiCurrent);
        IntCleanupCurIcons(Process, ppiCurrent);

        /* no process windows should exist at this point, or the function will assert! */
        DestroyProcessClasses(ppiCurrent);
        ppiCurrent->W32PF_flags &= ~W32PF_CLASSESREGISTERED;

        GDI_CleanupForProcess(Process);

        co_IntGraphicsCheck(FALSE);

        /*
         * Deregister logon application automatically
         */
        if(LogonProcess == ppiCurrent)
        {
            LogonProcess = NULL;
        }

        /* Close the startup desktop */
        if(ppiCurrent->rpdeskStartup)
            ObDereferenceObject(ppiCurrent->rpdeskStartup);
        if(ppiCurrent->hdeskStartup)
            ZwClose(ppiCurrent->hdeskStartup);

        /* Close the current window station */
        UserSetProcessWindowStation(NULL);

        /* Destroy GDI pools */
        GdiPoolDestroy(ppiCurrent->pPoolDcAttr);
        GdiPoolDestroy(ppiCurrent->pPoolBrushAttr);
        GdiPoolDestroy(ppiCurrent->pPoolRgnAttr);

        /* Ftee the PROCESSINFO */
        PsSetProcessWin32Process(Process, NULL);
        ExFreePoolWithTag(ppiCurrent, USERTAG_PROCESSINFO);
    }

    RETURN( STATUS_SUCCESS);

CLEANUP:
    UserLeave();
    DPRINT("Leave Win32kProcessCallback, ret=%i\n",_ret_);
    END_CLEANUP;
}


NTSTATUS
APIENTRY
Win32kThreadCallback(struct _ETHREAD *Thread,
                     PSW32THREADCALLOUTTYPE Type)
{
    struct _EPROCESS *Process;
    PTHREADINFO ptiCurrent;
    int i;
    NTSTATUS Status;
    PTEB pTeb;

    DPRINT("Enter Win32kThreadCallback\n");
    UserEnterExclusive();

    Process = Thread->ThreadsProcess;
    pTeb = NtCurrentTeb();

    ASSERT(pTeb);

    if (Type == PsW32ThreadCalloutInitialize)
    {
        HWINSTA hWinSta = NULL;
        PCLIENTINFO pci;
        HDESK hDesk = NULL;
        PUNICODE_STRING DesktopPath;
        PDESKTOP pdesk;
        PRTL_USER_PROCESS_PARAMETERS ProcessParams = Process->Peb->ProcessParameters;

        ASSERT(PsGetThreadWin32Thread(Thread)==NULL);

        ptiCurrent = ExAllocatePoolWithTag(NonPagedPool,
                                           sizeof(THREADINFO),
                                           USERTAG_THREADINFO);
        if (ptiCurrent == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto leave;
        }

        RtlZeroMemory(ptiCurrent, sizeof(THREADINFO));

        PsSetThreadWin32Thread(Thread, ptiCurrent);
        pTeb->Win32ThreadInfo = ptiCurrent;
        ptiCurrent->pClientInfo = (PCLIENTINFO)pTeb->Win32ClientInfo;

        TRACE_CH(UserThread, "Creating W32 thread TID:%d at IRQ level: %lu\n", Thread->Cid.UniqueThread, KeGetCurrentIrql());

        /* Initialize the THREADINFO */
        InitializeListHead(&ptiCurrent->WindowListHead);
        InitializeListHead(&ptiCurrent->W32CallbackListHead);
        InitializeListHead(&ptiCurrent->PtiLink);
        for (i = 0; i < NB_HOOKS; i++)
        {
            InitializeListHead(&ptiCurrent->aphkStart[i]);
        }
        ptiCurrent->pEThread = Thread;
        ptiCurrent->ppi = PsGetCurrentProcessWin32Process();
        ptiCurrent->ptiSibling = ptiCurrent->ppi->ptiList;
        ptiCurrent->ppi->ptiList = ptiCurrent;
        ptiCurrent->ppi->cThreads++;
        ptiCurrent->MessageQueue = MsqCreateMessageQueue(Thread);
        ptiCurrent->KeyboardLayout = W32kGetDefaultKeyLayout();
        if (ptiCurrent->KeyboardLayout)
            UserReferenceObject(ptiCurrent->KeyboardLayout);
        ptiCurrent->TIF_flags &= ~TIF_INCLEANUP;

        /* Initialize the CLIENTINFO */
        pci = (PCLIENTINFO)pTeb->Win32ClientInfo;
        RtlZeroMemory(pci, sizeof(CLIENTINFO));
        pci->ppi = ptiCurrent->ppi;
        pci->fsHooks = ptiCurrent->fsHooks;
        pci->dwTIFlags = ptiCurrent->TIF_flags;
        if (ptiCurrent->KeyboardLayout) 
            pci->hKL = ptiCurrent->KeyboardLayout->hkl;

        /* Assign a default window station and desktop to the process */
        /* Do not try to open a desktop or window station before winlogon initializes */
        if(ptiCurrent->ppi->hdeskStartup == NULL && LogonProcess != NULL)
        {
            /*
             * inherit the thread desktop and process window station (if not yet inherited) from the process startup
             * info structure. See documentation of CreateProcess()
             */
            DesktopPath = (ProcessParams ? ((ProcessParams->DesktopInfo.Length > 0) ? &ProcessParams->DesktopInfo : NULL) : NULL);
            Status = IntParseDesktopPath(Process,
                                         DesktopPath,
                                         &hWinSta,
                                         &hDesk);
            if(!NT_SUCCESS(Status))
            {
                ERR_CH(UserThread, "Failed to assign default dekstop and winsta to process\n");
                goto leave;
            }
            
            if(!UserSetProcessWindowStation(hWinSta))
            {
                Status = STATUS_UNSUCCESSFUL;
                goto leave;
            }

            /* Validate the new desktop. */
            Status = IntValidateDesktopHandle(hDesk, UserMode, 0, &pdesk);
            if(!NT_SUCCESS(Status))
            {
                goto leave;
            }

            /* Store the parsed desktop as the initial desktop */
            ptiCurrent->ppi->hdeskStartup = hDesk;
            ptiCurrent->ppi->rpdeskStartup = pdesk;
        }

        if (ptiCurrent->ppi->hdeskStartup != NULL)
        {
            if (!IntSetThreadDesktop(ptiCurrent->ppi->hdeskStartup, FALSE))
            {
                ERR_CH(UserThread,"Unable to set thread desktop\n");
                Status = STATUS_UNSUCCESSFUL;
                goto leave;
            }
        }
    }
    else
    {
        PTHREADINFO *ppti;
        PSINGLE_LIST_ENTRY psle;
        PPROCESSINFO ppiCurrent;

        /* Get the Win32 Thread */
        ptiCurrent = PsGetThreadWin32Thread(Thread);

        ASSERT(ptiCurrent);

        TRACE_CH(UserThread,"Destroying W32 thread TID:%d at IRQ level: %lu\n", Thread->Cid.UniqueThread, KeGetCurrentIrql());

        ppiCurrent = ptiCurrent->ppi;
        ptiCurrent->TIF_flags |= TIF_INCLEANUP;
        ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;

        /* Find the THREADINFO in the PROCESSINFO's list */
        ppti = &ppiCurrent->ptiList;
        while (*ppti != NULL && *ppti != ptiCurrent)
        {
            ppti = &((*ppti)->ptiSibling);
        }

        /* we must have found it */
        ASSERT(*ppti == ptiCurrent);

        /* Remove it from the list */
        *ppti = ptiCurrent->ptiSibling;

        /* Decrement thread count and check if its 0 */
        ppiCurrent->cThreads--;

        /* Do now some process cleanup that requires a valid win32 thread */
        if(ptiCurrent->ppi->cThreads == 0)
        {
            /* Check if we have registered the user api hook */
            if(ptiCurrent->ppi == ppiUahServer)
            {
                /* Unregister the api hook without blocking */
                UserUnregisterUserApiHook();
            }

            /* Notify logon application to restart shell if needed */
            if(ptiCurrent->pDeskInfo)
            {
                if(ptiCurrent->pDeskInfo->ppiShellProcess == ppiCurrent)
                {
                    DWORD ExitCode = PsGetProcessExitStatus(Process);

                    TRACE_PPI(ppiCurrent, UserProcess, "Shell process is exiting (%d)\n", ExitCode);

                    UserPostMessage(hwndSAS,
                                    WM_LOGONNOTIFY,
                                    LN_SHELL_EXITED,
                                    ExitCode);

                    ptiCurrent->pDeskInfo->ppiShellProcess = NULL;
                }
            }
        }

        DceFreeThreadDCE(ptiCurrent);
        HOOK_DestroyThreadHooks(Thread);
        EVENT_DestroyThreadEvents(Thread);

        /* Cleanup timers */
        DestroyTimersForThread(ptiCurrent);
        KeSetEvent(ptiCurrent->MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
        UnregisterThreadHotKeys(Thread);

        co_DestroyThreadWindows(Thread);
        IntBlockInput(ptiCurrent, FALSE);
        MsqDestroyMessageQueue(ptiCurrent->MessageQueue);
        IntCleanupThreadCallbacks(ptiCurrent);
        if (ptiCurrent->KeyboardLayout)
            UserDereferenceObject(ptiCurrent->KeyboardLayout);

        /* cleanup user object references stack */
        psle = PopEntryList(&ptiCurrent->ReferencesList);
        while (psle)
        {
            PUSER_REFERENCE_ENTRY ref = CONTAINING_RECORD(psle, USER_REFERENCE_ENTRY, Entry);
            TRACE_CH(UserThread,"thread clean: remove reference obj 0x%x\n",ref->obj);
            UserDereferenceObject(ref->obj);

            psle = PopEntryList(&ptiCurrent->ReferencesList);
        }

        IntSetThreadDesktop(NULL, TRUE);


        /* Free the THREADINFO */
        PsSetThreadWin32Thread(Thread, NULL);
        ExFreePoolWithTag(ptiCurrent, USERTAG_THREADINFO);
    }

    Status = STATUS_SUCCESS;

leave:
    UserLeave();
    DPRINT("Leave Win32kThreadCallback, Status=0x%lx\n",Status);

    return Status;
}

#ifdef _M_IX86
C_ASSERT(sizeof(SERVERINFO) <= PAGE_SIZE);
#endif

// Return on failure
#define NT_ROF(x) \
    Status = (x); \
    if (!NT_SUCCESS(Status)) \
    { \
        DPRINT1("Failed '%s' (0x%lx)\n", #x, Status); \
        return Status; \
    }

/*
 * This definition doesn't work
 */
INIT_FUNCTION
NTSTATUS
APIENTRY
DriverEntry(
    IN    PDRIVER_OBJECT    DriverObject,
    IN    PUNICODE_STRING    RegistryPath)
{
    NTSTATUS Status;
    BOOLEAN Result;
    WIN32_CALLOUTS_FPNS CalloutData = {0};
    PVOID GlobalUserHeapBase = NULL;

    /*
     * Register user mode call interface
     * (system service table index = 1)
     */
    Result = KeAddSystemServiceTable(Win32kSSDT,
                                     NULL,
                                     Win32kNumberOfSysCalls,
                                     Win32kSSPT,
                                     1);
    if (Result == FALSE)
    {
        DPRINT1("Adding system services failed!\n");
        return STATUS_UNSUCCESSFUL;
    }

    hModuleWin = MmPageEntireDriver(DriverEntry);
    DPRINT("Win32k hInstance 0x%x!\n",hModuleWin);

    /* Register Object Manager Callbacks */
    CalloutData.WindowStationParseProcedure = IntWinStaObjectParse;
    CalloutData.WindowStationDeleteProcedure = IntWinStaObjectDelete;
    CalloutData.DesktopDeleteProcedure = IntDesktopObjectDelete;
    CalloutData.ProcessCallout = Win32kProcessCallback;
    CalloutData.ThreadCallout = Win32kThreadCallback;
    CalloutData.BatchFlushRoutine = NtGdiFlushUserBatch;
    CalloutData.DesktopOkToCloseProcedure = IntDesktopOkToClose;
    CalloutData.WindowStationOkToCloseProcedure = IntWinstaOkToClose;

    /* Register our per-process and per-thread structures. */
    PsEstablishWin32Callouts((PWIN32_CALLOUTS_FPNS)&CalloutData);

    /* Register service hook callbacks */
    KdSystemDebugControl('CsoR', DbgPreServiceHook, ID_Win32PreServiceHook, 0, 0, 0, 0);
    KdSystemDebugControl('CsoR', DbgPostServiceHook, ID_Win32PostServiceHook, 0, 0, 0, 0);

    /* Create the global USER heap */
    GlobalUserHeap = UserCreateHeap(&GlobalUserHeapSection,
                                    &GlobalUserHeapBase,
                                    1 * 1024 * 1024); /* FIXME: 1 MB for now... */
    if (GlobalUserHeap == NULL)
    {
        DPRINT1("Failed to initialize the global heap!\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Allocate global server info structure */
    gpsi = UserHeapAlloc(sizeof(SERVERINFO));
    if (!gpsi)
    {
        DPRINT1("Failed allocate server info structure!\n");
        return STATUS_UNSUCCESSFUL;
    }

    RtlZeroMemory(gpsi, sizeof(SERVERINFO));
    DPRINT("Global Server Data -> %x\n", gpsi);

    NT_ROF(InitGdiHandleTable());
    NT_ROF(InitPaletteImpl());

    /* Create stock objects, ie. precreated objects commonly
       used by win32 applications */
    CreateStockObjects();
    CreateSysColorObjects();

    NT_ROF(InitPDEVImpl());
    NT_ROF(InitLDEVImpl());
    NT_ROF(InitDeviceImpl());
    NT_ROF(InitDcImpl());
    NT_ROF(InitUserImpl());
    NT_ROF(InitWindowStationImpl());
    NT_ROF(InitDesktopImpl());
    NT_ROF(InitInputImpl());
    NT_ROF(InitKeyboardImpl());
    NT_ROF(MsqInitializeImpl());
    NT_ROF(InitTimerImpl());

    /* Initialize FreeType library */
    if (!InitFontSupport())
    {
        DPRINT1("Unable to initialize font support\n");
        return Status;
    }

    gusLanguageID = UserGetLanguageID();

    return STATUS_SUCCESS;
}

/* EOF */
