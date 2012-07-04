/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Driver entry and initialization of win32k
 * FILE:             subsystems/win32/win32k/main/main.c
 * PROGRAMER:
 */

#include <win32k.h>
#include <napi.h>

#define NDEBUG
#include <debug.h>

HANDLE hModuleWin;

PGDI_HANDLE_TABLE NTAPI GDIOBJ_iAllocHandleTable(OUT PSECTION_OBJECT *SectionObject);
BOOL NTAPI GDI_CleanupForProcess (struct _EPROCESS *Process);
NTSTATUS NTAPI UserDestroyThreadInfo(struct _ETHREAD *Thread);

HANDLE GlobalUserHeap = NULL;
PSECTION_OBJECT GlobalUserHeapSection = NULL;

PSERVERINFO gpsi = NULL; // Global User Server Information.

SHORT gusLanguageID;
PPROCESSINFO ppiScrnSaver;

extern ULONG_PTR Win32kSSDT[];
extern UCHAR Win32kSSPT[];
extern ULONG Win32kNumberOfSysCalls;

#if DBG
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
#endif

NTSTATUS
APIENTRY
Win32kProcessCallback(struct _EPROCESS *Process,
                      BOOLEAN Create)
{
    PPROCESSINFO ppiCurrent;
    DECLARE_RETURN(NTSTATUS);

    ASSERT(Process->Peb);

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
        {
            ERR_CH(UserProcess, "Failed to allocate ppi for PID:%d\n", Process->UniqueProcessId);
            RETURN( STATUS_NO_MEMORY);
        }

        RtlZeroMemory(ppiCurrent, sizeof(PROCESSINFO));

        PsSetProcessWin32Process(Process, ppiCurrent);

#if DBG
        DbgInitDebugChannels();
#endif

        TRACE_CH(UserProcess,"Allocated ppi 0x%x for PID:%d\n", ppiCurrent, Process->UniqueProcessId);

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
            TRACE_CH(UserProcess,"Failed to map the global heap! 0x%x\n", Status);
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

        TRACE_CH(UserProcess, "Destroying ppi 0x%x\n", ppiCurrent);
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

        if (gppiInputProvider == ppiCurrent) gppiInputProvider = NULL;

        TRACE_CH(UserProcess,"Freeing ppi 0x%x\n", ppiCurrent);

        /* Ftee the PROCESSINFO */
        PsSetProcessWin32Process(Process, NULL);
        ExFreePoolWithTag(ppiCurrent, USERTAG_PROCESSINFO);
    }

    RETURN( STATUS_SUCCESS);

CLEANUP:
    UserLeave();
    END_CLEANUP;
}

NTSTATUS NTAPI
UserCreateThreadInfo(struct _ETHREAD *Thread)
{
    struct _EPROCESS *Process;
    PCLIENTINFO pci;
    PTHREADINFO ptiCurrent;
    int i;
    NTSTATUS Status = STATUS_SUCCESS;
    PTEB pTeb;

    Process = Thread->ThreadsProcess;

    pTeb = NtCurrentTeb();

    ASSERT(pTeb);

    ptiCurrent = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(THREADINFO),
                                       USERTAG_THREADINFO);
    if (ptiCurrent == NULL)
    {
        ERR_CH(UserThread, "Failed to allocate pti for TID %d\n", Thread->Cid.UniqueThread);
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(ptiCurrent, sizeof(THREADINFO));

    PsSetThreadWin32Thread(Thread, ptiCurrent);
    pTeb->Win32ThreadInfo = ptiCurrent;
    ptiCurrent->pClientInfo = (PCLIENTINFO)pTeb->Win32ClientInfo;

    TRACE_CH(UserThread, "Allocated pti 0x%x for TID %d\n", ptiCurrent, Thread->Cid.UniqueThread);

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
    if(ptiCurrent->MessageQueue == NULL)
    {
        ERR_CH(UserThread,"Failed to allocate message loop\n");
        Status = STATUS_NO_MEMORY;
        goto error;
    }
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
    {
        pci->hKL = ptiCurrent->KeyboardLayout->hkl;
        pci->CodePage = ptiCurrent->KeyboardLayout->CodePage;
    }

    /* Assign a default window station and desktop to the process */
    /* Do not try to open a desktop or window station before winlogon initializes */
    if(ptiCurrent->ppi->hdeskStartup == NULL && LogonProcess != NULL)
    {
        HWINSTA hWinSta = NULL;
        HDESK hDesk = NULL;
        UNICODE_STRING DesktopPath;
        PDESKTOP pdesk;
        PRTL_USER_PROCESS_PARAMETERS ProcessParams;

        /*
         * inherit the thread desktop and process window station (if not yet inherited) from the process startup
         * info structure. See documentation of CreateProcess()
         */
        ProcessParams = pTeb->ProcessEnvironmentBlock->ProcessParameters;

        Status = STATUS_UNSUCCESSFUL;
        if(ProcessParams && ProcessParams->DesktopInfo.Length > 0)
        {
            Status = IntSafeCopyUnicodeStringTerminateNULL(&DesktopPath, &ProcessParams->DesktopInfo);
        }
        if(!NT_SUCCESS(Status))
        {
            RtlInitUnicodeString(&DesktopPath, NULL);
        }

        Status = IntParseDesktopPath(Process,
                                     &DesktopPath,
                                     &hWinSta,
                                     &hDesk);

        if (DesktopPath.Buffer)
            ExFreePoolWithTag(DesktopPath.Buffer, TAG_STRING);

        if(!NT_SUCCESS(Status))
        {
            ERR_CH(UserThread, "Failed to assign default dekstop and winsta to process\n");
            goto error;
        }

        if(!UserSetProcessWindowStation(hWinSta))
        {
            Status = STATUS_UNSUCCESSFUL;
            ERR_CH(UserThread,"Failed to set initial process winsta\n");
            goto error;
        }

        /* Validate the new desktop. */
        Status = IntValidateDesktopHandle(hDesk, UserMode, 0, &pdesk);
        if(!NT_SUCCESS(Status))
        {
            ERR_CH(UserThread,"Failed to validate initial desktop handle\n");
            goto error;
        }

        /* Store the parsed desktop as the initial desktop */
        ptiCurrent->ppi->hdeskStartup = hDesk;
        ptiCurrent->ppi->rpdeskStartup = pdesk;
    }

    if (ptiCurrent->ppi->hdeskStartup != NULL)
    {
        if (!IntSetThreadDesktop(ptiCurrent->ppi->hdeskStartup, FALSE))
        {
            ERR_CH(UserThread,"Failed to set thread desktop\n");
            Status = STATUS_UNSUCCESSFUL;
            goto error;
        }
    }

    /* mark the thread as fully initialized */
    ptiCurrent->TIF_flags |= TIF_GUITHREADINITIALIZED;
    ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;

    return STATUS_SUCCESS;

error:
    ERR_CH(UserThread,"UserCreateThreadInfo failed! Freeing pti 0x%x for TID %d\n", ptiCurrent, Thread->Cid.UniqueThread);
    UserDestroyThreadInfo(Thread);
    return Status;
}

NTSTATUS
NTAPI
UserDestroyThreadInfo(struct _ETHREAD *Thread)
{
    PTHREADINFO *ppti;
    PSINGLE_LIST_ENTRY psle;
    PPROCESSINFO ppiCurrent;
    struct _EPROCESS *Process;
    PTHREADINFO ptiCurrent;

    Process = Thread->ThreadsProcess;

    /* Get the Win32 Thread */
    ptiCurrent = PsGetThreadWin32Thread(Thread);

    ASSERT(ptiCurrent);

    TRACE_CH(UserThread,"Destroying pti 0x%x\n", ptiCurrent);

    ppiCurrent = ptiCurrent->ppi;
    ptiCurrent->TIF_flags |= TIF_INCLEANUP;
    ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;


    /* Decrement thread count and check if its 0 */
    ppiCurrent->cThreads--;

    if(ptiCurrent->TIF_flags & TIF_GUITHREADINITIALIZED)
    {
        /* Do now some process cleanup that requires a valid win32 thread */
        if(ptiCurrent->ppi->cThreads == 0)
        {
            /* Check if we have registered the user api hook */
            if(ptiCurrent->ppi == ppiUahServer)
            {
                /* Unregister the api hook */
                UserUnregisterUserApiHook();
            }

            /* Notify logon application to restart shell if needed */
            if(ptiCurrent->pDeskInfo)
            {
                if(ptiCurrent->pDeskInfo->ppiShellProcess == ppiCurrent)
                {
                    DWORD ExitCode = PsGetProcessExitStatus(Process);

                   TRACE_CH(UserProcess, "Shell process is exiting (%d)\n", ExitCode);

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
        DestroyTimersForThread(ptiCurrent);
        KeSetEvent(ptiCurrent->MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
        UnregisterThreadHotKeys(Thread);
/*
        if (IsListEmpty(&ptiCurrent->WindowListHead))
        {
           ERR_CH(UserThread,"Thread Window List is Empty!\n");
        }
*/
        co_DestroyThreadWindows(Thread);

        if (ppiCurrent && ppiCurrent->ptiList == ptiCurrent && !ptiCurrent->ptiSibling)
        {
           //ERR_CH(UserThread,"DestroyProcessClasses\n");
          /* no process windows should exist at this point, or the function will assert! */
           DestroyProcessClasses(ppiCurrent);
           ppiCurrent->W32PF_flags &= ~W32PF_CLASSESREGISTERED;
        }

        IntBlockInput(ptiCurrent, FALSE);
        IntCleanupThreadCallbacks(ptiCurrent);

        /* cleanup user object references stack */
        psle = PopEntryList(&ptiCurrent->ReferencesList);
        while (psle)
        {
            PUSER_REFERENCE_ENTRY ref = CONTAINING_RECORD(psle, USER_REFERENCE_ENTRY, Entry);
            TRACE_CH(UserThread,"thread clean: remove reference obj 0x%x\n",ref->obj);
            UserDereferenceObject(ref->obj);

            psle = PopEntryList(&ptiCurrent->ReferencesList);
        }
    }

    if (ptiCurrent->pqAttach && ptiCurrent->MessageQueue)
    {
       PTHREADINFO ptiTo;
       ptiTo = PsGetThreadWin32Thread(ptiCurrent->MessageQueue->Thread);
       TRACE_CH(UserThread,"Attached Thread is getting switched!\n");
       UserAttachThreadInput( ptiCurrent, ptiTo, FALSE);
    }

    /* Free the message queue */
    if(ptiCurrent->MessageQueue)
    {
       MsqDestroyMessageQueue(ptiCurrent->MessageQueue);
    }

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

    if (ptiCurrent->KeyboardLayout)
        UserDereferenceObject(ptiCurrent->KeyboardLayout);

    IntSetThreadDesktop(NULL, TRUE);

    TRACE_CH(UserThread,"Freeing pti 0x%x\n", ptiCurrent);

    /* Free the THREADINFO */
    PsSetThreadWin32Thread(Thread, NULL);
    ExFreePoolWithTag(ptiCurrent, USERTAG_THREADINFO);

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kThreadCallback(struct _ETHREAD *Thread,
                     PSW32THREADCALLOUTTYPE Type)
{
    NTSTATUS Status;

    UserEnterExclusive();

    ASSERT(NtCurrentTeb());

    if (Type == PsW32ThreadCalloutInitialize)
    {
        ASSERT(PsGetThreadWin32Thread(Thread) == NULL);
        Status = UserCreateThreadInfo(Thread);
    }
    else
    {
        ASSERT(PsGetThreadWin32Thread(Thread) != NULL);
        Status = UserDestroyThreadInfo(Thread);
    }

    UserLeave();

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
#if DBG
    KdSystemDebugControl('CsoR', DbgPreServiceHook, ID_Win32PreServiceHook, 0, 0, 0, 0);
    KdSystemDebugControl('CsoR', DbgPostServiceHook, ID_Win32PostServiceHook, 0, 0, 0, 0);
#endif

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

    NT_ROF(InitBrushImpl());
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
    NT_ROF(InitDCEImpl());

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
