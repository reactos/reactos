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

NTSTATUS
APIENTRY
Win32kProcessCallback(struct _EPROCESS *Process,
                      BOOLEAN Create)
{
    PPROCESSINFO Win32Process;
    DECLARE_RETURN(NTSTATUS);

    DPRINT("Enter Win32kProcessCallback\n");
    UserEnterExclusive();

    /* Get the Win32 Process */
    Win32Process = PsGetProcessWin32Process(Process);

    /* Allocate one if needed */
    if (!Win32Process)
    {
        /* FIXME: Lock the process */
        Win32Process = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(PROCESSINFO),
                                             USERTAG_PROCESSINFO);

        if (Win32Process == NULL) RETURN( STATUS_NO_MEMORY);

        RtlZeroMemory(Win32Process, sizeof(PROCESSINFO));

        PsSetProcessWin32Process(Process, Win32Process);
        /* FIXME: Unlock the process */
    }

    if (Create)
    {
        SIZE_T ViewSize = 0;
        LARGE_INTEGER Offset;
        PVOID UserBase = NULL;
        PRTL_USER_PROCESS_PARAMETERS pParams = NULL;
        NTSTATUS Status;
        extern PSECTION_OBJECT GlobalUserHeapSection;

#ifdef DBG
        DbgInitDebugChannels();
#endif

        TRACE_PPI(Win32Process, UserProcess,"Allocated ppi for PID:%d\n", Process->UniqueProcessId);

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
                                    PAGE_EXECUTE_READ); /* would prefer PAGE_READONLY, but thanks to RTL heaps... */
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to map the global heap! 0x%x\n", Status);
            RETURN(Status);
        }
        Win32Process->HeapMappings.Next = NULL;
        Win32Process->HeapMappings.KernelMapping = (PVOID)GlobalUserHeap;
        Win32Process->HeapMappings.UserMapping = UserBase;
        Win32Process->HeapMappings.Count = 1;

        InitializeListHead(&Win32Process->MenuListHead);

        InitializeListHead(&Win32Process->GDIBrushAttrFreeList);
        InitializeListHead(&Win32Process->GDIDcAttrFreeList);

        InitializeListHead(&Win32Process->PrivateFontListHead);
        ExInitializeFastMutex(&Win32Process->PrivateFontListLock);

        InitializeListHead(&Win32Process->DriverObjListHead);
        ExInitializeFastMutex(&Win32Process->DriverObjListLock);

        Win32Process->KeyboardLayout = W32kGetDefaultKeyLayout();
        EngCreateEvent((PEVENT *)&Win32Process->InputIdleEvent);
        KeInitializeEvent(Win32Process->InputIdleEvent, NotificationEvent, FALSE);

        if(Process->Peb != NULL)
        {
            /* map the gdi handle table to user land */
            Process->Peb->GdiSharedHandleTable = GDI_MapHandleTable(Process);
            Process->Peb->GdiDCAttributeList = GDI_BATCH_LIMIT;
            pParams = Process->Peb->ProcessParameters;
        }

        Win32Process->peProcess = Process;
        /* setup process flags */
        Win32Process->W32PF_flags = W32PF_THREADCONNECTED;

        if ( pParams &&
             pParams->WindowFlags & STARTF_SCRNSAVER )
        {
           ppiScrnSaver = Win32Process;
           Win32Process->W32PF_flags |= W32PF_SCREENSAVER;
        }

        /* Create pools for GDI object attributes */
        Win32Process->pPoolDcAttr = GdiPoolCreate(sizeof(DC_ATTR), 'acdG');
        Win32Process->pPoolBrushAttr = GdiPoolCreate(sizeof(BRUSH_ATTR), 'arbG');
        Win32Process->pPoolRgnAttr = GdiPoolCreate(sizeof(RGN_ATTR), 'agrG');
        ASSERT(Win32Process->pPoolDcAttr);
        ASSERT(Win32Process->pPoolBrushAttr);
        ASSERT(Win32Process->pPoolRgnAttr);
    }
    else
    {
        DPRINT("Destroying W32 process PID:%d at IRQ level: %lu\n", Process->UniqueProcessId, KeGetCurrentIrql());
        Win32Process->W32PF_flags |= W32PF_TERMINATED;

        if (ppiScrnSaver == Win32Process) ppiScrnSaver = NULL;

        /* Notify logon application to restart shell if needed */
        if(Win32Process->rpdeskStartup->pDeskInfo)
        {
            if(Win32Process->rpdeskStartup->pDeskInfo->ppiShellProcess == Win32Process)
            {
                DWORD ExitCode;
                ExitCode = PsGetProcessExitStatus(Win32Process->peProcess);

                DPRINT1("Shell process is exiting (%d)\n", ExitCode);

                UserPostMessage(hwndSAS,
                                WM_LOGONNOTIFY,
                                LN_SHELL_EXITED,
                                ExitCode);
            }
        }

        if (Win32Process->InputIdleEvent)
        {
           EngFreeMem((PVOID)Win32Process->InputIdleEvent);
           Win32Process->InputIdleEvent = NULL;
        }

        IntCleanupMenus(Process, Win32Process);
        IntCleanupCurIcons(Process, Win32Process);

        /* no process windows should exist at this point, or the function will assert! */
        DestroyProcessClasses(Win32Process);
        Win32Process->W32PF_flags &= ~W32PF_CLASSESREGISTERED;

        GDI_CleanupForProcess(Process);

        co_IntGraphicsCheck(FALSE);

        /*
         * Deregister logon application automatically
         */
        if(LogonProcess == Win32Process)
        {
            LogonProcess = NULL;
        }

        /* Close the startup desktop */
        ASSERT(Win32Process->rpdeskStartup);
        ASSERT(Win32Process->hdeskStartup);
        ObDereferenceObject(Win32Process->rpdeskStartup);
        ZwClose(Win32Process->hdeskStartup);

        /* Close the current window station */
        UserSetProcessWindowStation(NULL);

        /* Destroy GDI pools */
        GdiPoolDestroy(Win32Process->pPoolDcAttr);
        GdiPoolDestroy(Win32Process->pPoolBrushAttr);
        GdiPoolDestroy(Win32Process->pPoolRgnAttr);

        /* Ftee the PROCESSINFO */
        PsSetProcessWin32Process(Process, NULL);
        ExFreePoolWithTag(Win32Process, USERTAG_PROCESSINFO);
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

    DPRINT("Enter Win32kThreadCallback\n");
    UserEnterExclusive();

    Process = Thread->ThreadsProcess;

    /* Get the Win32 Thread */
    ptiCurrent = PsGetThreadWin32Thread(Thread);

    /* Allocate one if needed */
    if (!ptiCurrent)
    {
        /* FIXME: Lock the process */
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
        /* FIXME: Unlock the process */
    }

    if (Type == PsW32ThreadCalloutInitialize)
    {
        HWINSTA hWinSta = NULL;
        PTEB pTeb;
        HDESK hDesk = NULL;
        PUNICODE_STRING DesktopPath;
        PDESKTOP pdesk;
        PRTL_USER_PROCESS_PARAMETERS ProcessParams = (Process->Peb ? Process->Peb->ProcessParameters : NULL);

        DPRINT("Creating W32 thread TID:%d at IRQ level: %lu\n", Thread->Cid.UniqueThread, KeGetCurrentIrql());

        InitializeListHead(&ptiCurrent->WindowListHead);
        InitializeListHead(&ptiCurrent->W32CallbackListHead);
        InitializeListHead(&ptiCurrent->PtiLink);
        for (i = 0; i < NB_HOOKS; i++)
        {
            InitializeListHead(&ptiCurrent->aphkStart[i]);
        }

        ptiCurrent->TIF_flags &= ~TIF_INCLEANUP;
        co_IntDestroyCaret(ptiCurrent);
        ptiCurrent->ppi = PsGetCurrentProcessWin32Process();
        ptiCurrent->ptiSibling = ptiCurrent->ppi->ptiList;
        ptiCurrent->ppi->ptiList = ptiCurrent;
        ptiCurrent->ppi->cThreads++;
        if (ptiCurrent->rpdesk && !ptiCurrent->pDeskInfo)
        {
           ptiCurrent->pDeskInfo = ptiCurrent->rpdesk->pDeskInfo;
        }
        ptiCurrent->MessageQueue = MsqCreateMessageQueue(Thread);
        ptiCurrent->KeyboardLayout = W32kGetDefaultKeyLayout();
        if (ptiCurrent->KeyboardLayout)
            UserReferenceObject(ptiCurrent->KeyboardLayout);
        ptiCurrent->pEThread = Thread;

        /* HAAAAAAAACK! This should go to Win32kProcessCallback */
        if(ptiCurrent->ppi->hdeskStartup == NULL)
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
            if(NT_SUCCESS(Status))
            {
                if(hWinSta != NULL)
                {
                    if(!UserSetProcessWindowStation(hWinSta))
                    {
                        DPRINT1("Failed to set process window station\n");
                    }
                }

                if (hDesk != NULL)
                {
                    /* Validate the new desktop. */
                    Status = IntValidateDesktopHandle(hDesk,
                                                      UserMode,
                                                      0,
                                                      &pdesk);

                    if(NT_SUCCESS(Status))
                    {
                        ptiCurrent->ppi->hdeskStartup = hDesk;
                        ptiCurrent->ppi->rpdeskStartup = pdesk;
                    }
                }
            }
            else
            {
               DPRINT1("No Desktop handle for this Thread!\n");
            }
        }

        if (ptiCurrent->ppi->hdeskStartup != NULL)
        {
            if (!IntSetThreadDesktop(ptiCurrent->ppi->hdeskStartup, FALSE))
            {
                DPRINT1("Unable to set thread desktop\n");
            }
        }

        pTeb = NtCurrentTeb();
        if (pTeb)
        { /* Attempt to startup client support which should have been initialized in IntSetThreadDesktop. */
           PCLIENTINFO pci = (PCLIENTINFO)pTeb->Win32ClientInfo;
           ptiCurrent->pClientInfo = pci;
           pci->ppi = ptiCurrent->ppi;
           pci->fsHooks = ptiCurrent->fsHooks;
           if (ptiCurrent->KeyboardLayout) pci->hKL = ptiCurrent->KeyboardLayout->hkl;
           pci->dwTIFlags = ptiCurrent->TIF_flags;

           /* CI may not have been initialized. */
           if (!pci->pDeskInfo && ptiCurrent->pDeskInfo)
           {
              if (!pci->ulClientDelta) pci->ulClientDelta = DesktopHeapGetUserDelta();

              pci->pDeskInfo = (PVOID)((ULONG_PTR)ptiCurrent->pDeskInfo - pci->ulClientDelta);
           }
           if (ptiCurrent->pcti && pci->pDeskInfo)
              pci->pClientThreadInfo = (PVOID)((ULONG_PTR)ptiCurrent->pcti - pci->ulClientDelta);
           else
              pci->pClientThreadInfo = NULL;
        }
        else
        {
           DPRINT1("No TEB for this Thread!\n");
           // System thread running! Now SendMessage should be okay.
           ptiCurrent->pcti = &ptiCurrent->cti;
        }
        GetW32ThreadInfo();
    }
    else
    {
        PTHREADINFO *ppti;
        PSINGLE_LIST_ENTRY psle;
        PPROCESSINFO ppiCurrent;

        DPRINT("Destroying W32 thread TID:%d at IRQ level: %lu\n", Thread->Cid.UniqueThread, KeGetCurrentIrql());

        ppiCurrent = ptiCurrent->ppi;
        ptiCurrent->TIF_flags |= TIF_INCLEANUP;

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
        }

        DceFreeThreadDCE(ptiCurrent);
        HOOK_DestroyThreadHooks(Thread);
        EVENT_DestroyThreadEvents(Thread);

        /* Cleanup timers */
        DestroyTimersForThread(ptiCurrent);
        KeSetEvent(ptiCurrent->MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
        UnregisterThreadHotKeys(Thread);

        /* what if this co_ func crash in umode? what will clean us up then? */
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
            DPRINT("thread clean: remove reference obj 0x%x\n",ref->obj);
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
    IN	PDRIVER_OBJECT	DriverObject,
    IN	PUNICODE_STRING	RegistryPath)
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

#if DBG_ENABLE_SERVICE_HOOKS
    /* Register service hook callbacks */
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
