/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Driver entry and initialization of win32k
 * FILE:             win32ss/user/ntuser/main.c
 * PROGRAMER:
 */

#include <win32k.h>
#include <napi.h>

#define NDEBUG
#include <debug.h>
#include <kdros.h>

HANDLE hModuleWin;

NTSTATUS ExitProcessCallback(PEPROCESS Process);
NTSTATUS NTAPI ExitThreadCallback(PETHREAD Thread);

// TODO: Should be moved to some GDI header
NTSTATUS GdiProcessCreate(PEPROCESS Process);
NTSTATUS GdiProcessDestroy(PEPROCESS Process);
NTSTATUS GdiThreadCreate(PETHREAD Thread);
NTSTATUS GdiThreadDestroy(PETHREAD Thread);

PSERVERINFO gpsi = NULL; // Global User Server Information.

USHORT gusLanguageID;
PPROCESSINFO ppiScrnSaver;
PPROCESSINFO gppiList = NULL;

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
AllocW32Process(IN  PEPROCESS Process,
                OUT PPROCESSINFO* W32Process)
{
    PPROCESSINFO ppiCurrent;

    TRACE_CH(UserProcess, "In AllocW32Process(0x%p)\n", Process);

    /* Check that we were not called with an already existing Win32 process info */
    ppiCurrent = PsGetProcessWin32Process(Process);
    if (ppiCurrent) return STATUS_SUCCESS;

    /* Allocate a new Win32 process info */
    ppiCurrent = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(*ppiCurrent),
                                       USERTAG_PROCESSINFO);
    if (ppiCurrent == NULL)
    {
        ERR_CH(UserProcess, "Failed to allocate ppi for PID:0x%lx\n",
               HandleToUlong(Process->UniqueProcessId));
        return STATUS_NO_MEMORY;
    }

    TRACE_CH(UserProcess, "Allocated ppi 0x%p for PID:0x%lx\n",
             ppiCurrent, HandleToUlong(Process->UniqueProcessId));

    RtlZeroMemory(ppiCurrent, sizeof(*ppiCurrent));

    PsSetProcessWin32Process(Process, ppiCurrent, NULL);
    IntReferenceProcessInfo(ppiCurrent);

    *W32Process = ppiCurrent;
    return STATUS_SUCCESS;
}

/*
 * Called from IntDereferenceProcessInfo
 */
VOID
UserDeleteW32Process(
    _Pre_notnull_ __drv_freesMem(Mem) PPROCESSINFO ppiCurrent)
{
    if (ppiCurrent->InputIdleEvent)
    {
        /* Free the allocated memory */
        ExFreePoolWithTag(ppiCurrent->InputIdleEvent, USERTAG_EVENT);
    }

    /* Close the startup desktop */
    if (ppiCurrent->rpdeskStartup)
        ObDereferenceObject(ppiCurrent->rpdeskStartup);

#if DBG
    if (DBG_IS_CHANNEL_ENABLED(ppiCurrent, DbgChUserObj, WARN_LEVEL))
    {
        TRACE_PPI(ppiCurrent, UserObj, "Dumping user handles now that process info %p is gets freed.\n", ppiCurrent);
        DbgUserDumpHandleTable();
    }
#endif

    /* Free the PROCESSINFO */
    ExFreePoolWithTag(ppiCurrent, USERTAG_PROCESSINFO);
}

NTSTATUS
UserProcessCreate(PEPROCESS Process)
{
    PPROCESSINFO ppiCurrent = PsGetProcessWin32Process(Process);
    ASSERT(ppiCurrent);

    InitializeListHead(&ppiCurrent->DriverObjListHead);
    ExInitializeFastMutex(&ppiCurrent->DriverObjListLock);

    {
        PKEVENT Event;

        /* Allocate memory for the event structure */
        Event = ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(*Event),
                                      USERTAG_EVENT);
        if (Event)
        {
            /* Initialize the kernel event */
            KeInitializeEvent(Event,
                              SynchronizationEvent,
                              FALSE);
        }
        else
        {
            /* Out of memory */
            DPRINT("CreateEvent() failed\n");
            KeBugCheck(0);
        }

        /* Set the event */
        ppiCurrent->InputIdleEvent = Event;
        KeInitializeEvent(ppiCurrent->InputIdleEvent, NotificationEvent, FALSE);
    }

    ppiCurrent->peProcess = Process;
    ppiCurrent->W32Pid = HandleToUlong(PsGetProcessId(Process));

    /* Setup process flags */
    ppiCurrent->W32PF_flags |= W32PF_PROCESSCONNECTED;
    if (Process->Peb->ProcessParameters &&
        (Process->Peb->ProcessParameters->WindowFlags & STARTF_SCREENSAVER))
    {
        ppiScrnSaver = ppiCurrent;
        ppiCurrent->W32PF_flags |= W32PF_SCREENSAVER;
    }

    // FIXME: check if this process is allowed.
    ppiCurrent->W32PF_flags |= W32PF_ALLOWFOREGROUNDACTIVATE; // Starting application will get it toggled off.

    return STATUS_SUCCESS;
}

NTSTATUS
UserProcessDestroy(PEPROCESS Process)
{
    PPROCESSINFO ppiCurrent = PsGetProcessWin32Process(Process);
    ASSERT(ppiCurrent);

    if (ppiScrnSaver == ppiCurrent)
        ppiScrnSaver = NULL;

    /* Destroy user objects */
    UserDestroyObjectsForOwner(gHandleTable, ppiCurrent);

    TRACE_CH(UserProcess, "Freeing ppi 0x%p\n", ppiCurrent);
#if DBG
    if (DBG_IS_CHANNEL_ENABLED(ppiCurrent, DbgChUserObj, WARN_LEVEL))
    {
        TRACE_CH(UserObj, "Dumping user handles at the end of the process %s (Info %p).\n",
            ppiCurrent->peProcess->ImageFileName, ppiCurrent);
        DbgUserDumpHandleTable();
    }
#endif

    /* Remove it from the list of GUI apps */
    co_IntGraphicsCheck(FALSE);

    /*
     * Deregister logon application automatically
     */
    if (gpidLogon == ppiCurrent->peProcess->UniqueProcessId)
        gpidLogon = 0;

    /* Close the current window station */
    UserSetProcessWindowStation(NULL);

    if (gppiInputProvider == ppiCurrent) gppiInputProvider = NULL;

    if (ppiCurrent->hdeskStartup)
    {
        ZwClose(ppiCurrent->hdeskStartup);
        ppiCurrent->hdeskStartup = NULL;
    }

    /* Clean up the process icon cache */
    IntCleanupCurIconCache(ppiCurrent);

    return STATUS_SUCCESS;
}

NTSTATUS
InitProcessCallback(PEPROCESS Process)
{
    NTSTATUS Status;
    PPROCESSINFO ppiCurrent;
    PVOID KernelMapping = NULL, UserMapping = NULL;

    /* We might be called with an already allocated win32 process */
    ppiCurrent = PsGetProcessWin32Process(Process);
    if (ppiCurrent != NULL)
    {
        /* There is no more to do for us (this is a success code!) */
        return STATUS_ALREADY_WIN32;
    }
    // if (ppiCurrent->W32PF_flags & W32PF_PROCESSCONNECTED)
        // return STATUS_ALREADY_WIN32;

    /* Allocate a new Win32 process info */
    Status = AllocW32Process(Process, &ppiCurrent);
    if (!NT_SUCCESS(Status))
    {
        ERR_CH(UserProcess, "Failed to allocate ppi for PID:0x%lx\n",
               HandleToUlong(Process->UniqueProcessId));
        return Status;
    }

#if DBG
    DbgInitDebugChannels();
#if defined(KDBG)
    KdRosRegisterCliCallback(DbgGdiKdbgCliCallback);
#endif
#endif

    /* Map the global user heap into the process */
    Status = MapGlobalUserHeap(Process, &KernelMapping, &UserMapping);
    if (!NT_SUCCESS(Status))
    {
        TRACE_CH(UserProcess, "Failed to map the global heap! 0x%x\n", Status);
        goto error;
    }

    TRACE_CH(UserProcess, "InitProcessCallback -- We have KernelMapping 0x%p and UserMapping 0x%p with delta = 0x%x\n",
           KernelMapping, UserMapping, (ULONG_PTR)KernelMapping - (ULONG_PTR)UserMapping);

    /* Initialize USER process info */
    Status = UserProcessCreate(Process);
    if (!NT_SUCCESS(Status))
    {
        ERR_CH(UserProcess, "UserProcessCreate failed, Status 0x%08lx\n", Status);
        goto error;
    }

    /* Initialize GDI process info */
    Status = GdiProcessCreate(Process);
    if (!NT_SUCCESS(Status))
    {
        ERR_CH(UserProcess, "GdiProcessCreate failed, Status 0x%08lx\n", Status);
        goto error;
    }

    /* Add the process to the global list */
    ppiCurrent->ppiNext = gppiList;
    gppiList = ppiCurrent;

    return STATUS_SUCCESS;

error:
    ERR_CH(UserProcess, "InitProcessCallback failed! Freeing ppi 0x%p for PID:0x%lx\n",
           ppiCurrent, HandleToUlong(Process->UniqueProcessId));
    ExitProcessCallback(Process);
    return Status;
}

NTSTATUS
ExitProcessCallback(PEPROCESS Process)
{
    PPROCESSINFO ppiCurrent, *pppi;

    /* Get the Win32 Process */
    ppiCurrent = PsGetProcessWin32Process(Process);
    ASSERT(ppiCurrent);
    ASSERT(ppiCurrent->peProcess == Process);

    TRACE_CH(UserProcess, "Destroying ppi 0x%p\n", ppiCurrent);
    ppiCurrent->W32PF_flags |= W32PF_TERMINATED;

    /* Remove it from the list */
    pppi = &gppiList;
    while (*pppi != NULL && *pppi != ppiCurrent)
    {
        pppi = &(*pppi)->ppiNext;
    }
    ASSERT(*pppi == ppiCurrent);
    *pppi = ppiCurrent->ppiNext;

    /* Cleanup GDI info */
    GdiProcessDestroy(Process);

    /* Cleanup USER info */
    UserProcessDestroy(Process);

    /* The process is dying */
    PsSetProcessWin32Process(Process, NULL, ppiCurrent);
    ppiCurrent->peProcess = NULL;

    /* Finally, dereference */
    IntDereferenceProcessInfo(ppiCurrent);

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kProcessCallback(PEPROCESS Process,
                      BOOLEAN Initialize)
{
    NTSTATUS Status;

    ASSERT(Process->Peb);

    TRACE_CH(UserProcess, "Win32kProcessCallback -->\n");

    UserEnterExclusive();

    if (Initialize)
    {
        Status = InitProcessCallback(Process);
    }
    else
    {
        Status = ExitProcessCallback(Process);
    }

    UserLeave();

    TRACE_CH(UserProcess, "<-- Win32kProcessCallback\n");

    return Status;
}



NTSTATUS
AllocW32Thread(IN  PETHREAD Thread,
               OUT PTHREADINFO* W32Thread)
{
    PTHREADINFO ptiCurrent;

    TRACE_CH(UserThread, "In AllocW32Thread(0x%p)\n", Thread);

    /* Check that we were not called with an already existing Win32 thread info */
    ptiCurrent = PsGetThreadWin32Thread(Thread);
    NT_ASSERT(ptiCurrent == NULL);

    /* Allocate a new Win32 thread info */
    ptiCurrent = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(*ptiCurrent),
                                       USERTAG_THREADINFO);
    if (ptiCurrent == NULL)
    {
        ERR_CH(UserThread, "Failed to allocate pti for TID:0x%lx\n",
               HandleToUlong(Thread->Cid.UniqueThread));
        return STATUS_NO_MEMORY;
    }

    TRACE_CH(UserThread, "Allocated pti 0x%p for TID:0x%lx\n",
             ptiCurrent, HandleToUlong(Thread->Cid.UniqueThread));

    RtlZeroMemory(ptiCurrent, sizeof(*ptiCurrent));

    PsSetThreadWin32Thread(Thread, ptiCurrent, NULL);
    ObReferenceObject(Thread);
    IntReferenceThreadInfo(ptiCurrent);

    *W32Thread = ptiCurrent;
    return STATUS_SUCCESS;
}

/*
 * Called from IntDereferenceThreadInfo
 */
VOID
UserDeleteW32Thread(PTHREADINFO pti)
{
   PPROCESSINFO ppi = pti->ppi;

   TRACE_CH(UserThread, "UserDeleteW32Thread pti 0x%p\n",pti);

   /* Free the message queue */
   if (pti->MessageQueue)
   {
      MsqDestroyMessageQueue(pti);
   }

   MsqCleanupThreadMsgs(pti);

   ObDereferenceObject(pti->pEThread);

   ExFreePoolWithTag(pti, USERTAG_THREADINFO);

   IntDereferenceProcessInfo(ppi);

   {
      // Find another queue for mouse cursor.
      MSG msg;
      msg.message = WM_MOUSEMOVE;
      msg.wParam = UserGetMouseButtonsState();
      msg.lParam = MAKELPARAM(gpsi->ptCursor.x, gpsi->ptCursor.y);
      msg.pt = gpsi->ptCursor;
      co_MsqInsertMouseMessage(&msg, 0, 0, TRUE);
   }
}

NTSTATUS
UserThreadCreate(PETHREAD Thread)
{
    return STATUS_SUCCESS;
}

NTSTATUS
UserThreadDestroy(PETHREAD Thread)
{
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
InitThreadCallback(PETHREAD Thread)
{
    PEPROCESS Process;
    PCLIENTINFO pci;
    PTHREADINFO ptiCurrent;
    int i;
    NTSTATUS Status = STATUS_SUCCESS;
    PTEB pTeb;
    PRTL_USER_PROCESS_PARAMETERS ProcessParams;

    Process = Thread->ThreadsProcess;

    pTeb = NtCurrentTeb();
    ASSERT(pTeb);

    ProcessParams = pTeb->ProcessEnvironmentBlock->ProcessParameters;

    /* Allocate a new Win32 thread info */
    Status = AllocW32Thread(Thread, &ptiCurrent);
    if (!NT_SUCCESS(Status))
    {
        ERR_CH(UserThread, "Failed to allocate pti for TID:0x%lx\n",
               HandleToUlong(Thread->Cid.UniqueThread));
        return Status;
    }

    /* Initialize the THREADINFO */
    ptiCurrent->pEThread = Thread;
    ptiCurrent->ppi = PsGetProcessWin32Process(Process);
    IntReferenceProcessInfo(ptiCurrent->ppi);
    pTeb->Win32ThreadInfo = ptiCurrent;
    ptiCurrent->pClientInfo = (PCLIENTINFO)pTeb->Win32ClientInfo;
    ptiCurrent->pcti = &ptiCurrent->cti;

    /* Mark the process as having threads */
    ptiCurrent->ppi->W32PF_flags |= W32PF_THREADCONNECTED;

    InitializeListHead(&ptiCurrent->WindowListHead);
    InitializeListHead(&ptiCurrent->W32CallbackListHead);
    InitializeListHead(&ptiCurrent->PostedMessagesListHead);
    InitializeListHead(&ptiCurrent->SentMessagesListHead);
    InitializeListHead(&ptiCurrent->PtiLink);
    for (i = 0; i < NB_HOOKS; i++)
    {
        InitializeListHead(&ptiCurrent->aphkStart[i]);
    }
    ptiCurrent->ptiSibling = ptiCurrent->ppi->ptiList;
    ptiCurrent->ppi->ptiList = ptiCurrent;
    ptiCurrent->ppi->cThreads++;

    ptiCurrent->hEventQueueClient = NULL;
    Status = ZwCreateEvent(&ptiCurrent->hEventQueueClient, EVENT_ALL_ACCESS,
                            NULL, SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(Status))
    {
        ERR_CH(UserThread, "Event creation failed, Status 0x%08x.\n", Status);
        goto error;
    }
    Status = ObReferenceObjectByHandle(ptiCurrent->hEventQueueClient, 0,
                                       *ExEventObjectType, UserMode,
                                       (PVOID*)&ptiCurrent->pEventQueueServer, NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR_CH(UserThread, "Failed referencing the event object, Status 0x%08x.\n", Status);
        ObCloseHandle(ptiCurrent->hEventQueueClient, UserMode);
        ptiCurrent->hEventQueueClient = NULL;
        goto error;
    }

    ptiCurrent->pcti->timeLastRead = EngGetTickCount32();

    ptiCurrent->MessageQueue = MsqCreateMessageQueue(ptiCurrent);
    if (ptiCurrent->MessageQueue == NULL)
    {
        ERR_CH(UserThread, "Failed to allocate message loop\n");
        Status = STATUS_NO_MEMORY;
        goto error;
    }

    ptiCurrent->KeyboardLayout = W32kGetDefaultKeyLayout();
    if (ptiCurrent->KeyboardLayout)
        UserReferenceObject(ptiCurrent->KeyboardLayout);

    ptiCurrent->TIF_flags &= ~TIF_INCLEANUP;

    // FIXME: Flag SYSTEM threads with... TIF_SYSTEMTHREAD !!

    /* CSRSS threads have some special features */
    if (Process == gpepCSRSS || !gpepCSRSS)
        ptiCurrent->TIF_flags = TIF_CSRSSTHREAD | TIF_DONTATTACHQUEUE;

    /* Initialize the CLIENTINFO */
    pci = (PCLIENTINFO)pTeb->Win32ClientInfo;
    RtlZeroMemory(pci, sizeof(*pci));
    pci->ppi = ptiCurrent->ppi;
    pci->fsHooks = ptiCurrent->fsHooks;
    pci->dwTIFlags = ptiCurrent->TIF_flags;
    if (ptiCurrent->KeyboardLayout)
    {
        pci->hKL = ptiCurrent->KeyboardLayout->hkl;
        pci->CodePage = ptiCurrent->KeyboardLayout->CodePage;
    }

    /* Need to pass the user Startup Information to the current process. */
    if ( ProcessParams )
    {
       if ( ptiCurrent->ppi->usi.cb == 0 )      // Not initialized yet.
       {
          if ( ProcessParams->WindowFlags != 0 ) // Need window flags set.
          {
             ptiCurrent->ppi->usi.cb          = sizeof(USERSTARTUPINFO);
             ptiCurrent->ppi->usi.dwX         = ProcessParams->StartingX;
             ptiCurrent->ppi->usi.dwY         = ProcessParams->StartingY;
             ptiCurrent->ppi->usi.dwXSize     = ProcessParams->CountX;
             ptiCurrent->ppi->usi.dwYSize     = ProcessParams->CountY;
             ptiCurrent->ppi->usi.dwFlags     = ProcessParams->WindowFlags;
             ptiCurrent->ppi->usi.wShowWindow = (WORD)ProcessParams->ShowWindowFlags;
          }
       }
    }

    /*
     * Assign a default window station and desktop to the process.
     * Do not try to open a desktop or window station before the very first
     * (interactive) window station has been created by Winlogon.
     */
    if (!(ptiCurrent->TIF_flags & (TIF_SYSTEMTHREAD | TIF_CSRSSTHREAD)) &&
        ptiCurrent->ppi->hdeskStartup == NULL &&
        InputWindowStation != NULL)
    {
        HWINSTA hWinSta = NULL;
        HDESK hDesk = NULL;
        UNICODE_STRING DesktopPath;
        PDESKTOP pdesk;

        /*
         * Inherit the thread desktop and process window station (if not yet inherited)
         * from the process startup info structure. See documentation of CreateProcess().
         */
        Status = STATUS_UNSUCCESSFUL;
        if (ProcessParams && ProcessParams->DesktopInfo.Length > 0)
        {
            Status = IntSafeCopyUnicodeStringTerminateNULL(&DesktopPath, &ProcessParams->DesktopInfo);
        }
        if (!NT_SUCCESS(Status))
        {
            RtlInitUnicodeString(&DesktopPath, NULL);
        }

        Status = IntResolveDesktop(Process,
                                   &DesktopPath,
                                   !!(ProcessParams->WindowFlags & STARTF_INHERITDESKTOP),
                                   &hWinSta,
                                   &hDesk);

        if (DesktopPath.Buffer)
            ExFreePoolWithTag(DesktopPath.Buffer, TAG_STRING);

        if (!NT_SUCCESS(Status))
        {
            ERR_CH(UserThread, "Failed to assign default desktop and winsta to process\n");
            goto error;
        }

        if (!UserSetProcessWindowStation(hWinSta))
        {
            Status = STATUS_UNSUCCESSFUL;
            ERR_CH(UserThread, "Failed to set initial process winsta\n");
            goto error;
        }

        /* Validate the new desktop */
        Status = IntValidateDesktopHandle(hDesk, UserMode, 0, &pdesk);
        if (!NT_SUCCESS(Status))
        {
            ERR_CH(UserThread, "Failed to validate initial desktop handle\n");
            goto error;
        }

        /* Store the parsed desktop as the initial desktop */
        ASSERT(ptiCurrent->ppi->hdeskStartup == NULL);
        ASSERT(Process->UniqueProcessId != gpidLogon);
        ptiCurrent->ppi->hdeskStartup = hDesk;
        ptiCurrent->ppi->rpdeskStartup = pdesk;
    }

    if (ptiCurrent->ppi->hdeskStartup != NULL)
    {
        if (!IntSetThreadDesktop(ptiCurrent->ppi->hdeskStartup, FALSE))
        {
            ERR_CH(UserThread, "Failed to set thread desktop\n");
            Status = STATUS_UNSUCCESSFUL;
            goto error;
        }
    }

    /* Mark the thread as fully initialized */
    ptiCurrent->TIF_flags |= TIF_GUITHREADINITIALIZED;

    if (!(ptiCurrent->ppi->W32PF_flags & (W32PF_ALLOWFOREGROUNDACTIVATE | W32PF_APPSTARTING)) &&
         (gptiForeground && gptiForeground->ppi == ptiCurrent->ppi ))
    {
        ptiCurrent->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
    }
    ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;

    /* Create the default input context */
    if (IS_IMM_MODE())
    {
        (VOID)UserCreateInputContext(0);
    }

    /* Last things to do only if we are not a SYSTEM or CSRSS thread */
    if (!(ptiCurrent->TIF_flags & (TIF_SYSTEMTHREAD | TIF_CSRSSTHREAD)))
    {
        /* Callback to User32 Client Thread Setup */
        TRACE_CH(UserThread, "Call co_IntClientThreadSetup...\n");
        Status = co_IntClientThreadSetup();
        if (!NT_SUCCESS(Status))
        {
            ERR_CH(UserThread, "ClientThreadSetup failed with Status 0x%08lx\n", Status);
            goto error;
        }
        TRACE_CH(UserThread, "co_IntClientThreadSetup succeeded!\n");
    }
    else
    {
        TRACE_CH(UserThread, "co_IntClientThreadSetup cannot be called...\n");
    }

    TRACE_CH(UserThread, "UserCreateW32Thread pti 0x%p\n", ptiCurrent);
    return STATUS_SUCCESS;

error:
    ERR_CH(UserThread, "InitThreadCallback failed! Freeing pti 0x%p for TID:0x%lx\n",
           ptiCurrent, HandleToUlong(Thread->Cid.UniqueThread));
    ExitThreadCallback(Thread);
    return Status;
}

VOID
UserDisplayNotifyShutdown(PPROCESSINFO ppiCurrent);

NTSTATUS
NTAPI
ExitThreadCallback(PETHREAD Thread)
{
    PTHREADINFO *ppti;
    PSINGLE_LIST_ENTRY psle;
    PPROCESSINFO ppiCurrent;
    PEPROCESS Process;
    PTHREADINFO ptiCurrent;

    Process = Thread->ThreadsProcess;

    /* Get the Win32 Thread */
    ptiCurrent = PsGetThreadWin32Thread(Thread);
    ASSERT(ptiCurrent);

    TRACE_CH(UserThread, "Destroying pti 0x%p eThread 0x%p\n", ptiCurrent, Thread);

    ptiCurrent->TIF_flags |= TIF_INCLEANUP;
    ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;

    ppiCurrent = ptiCurrent->ppi;
    ASSERT(ppiCurrent);

    IsRemoveAttachThread(ptiCurrent);

    ptiCurrent->TIF_flags |= TIF_DONTATTACHQUEUE;
    ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;

    UserCloseClipboard();

    /* Decrement thread count and check if its 0 */
    ppiCurrent->cThreads--;

    if (ptiCurrent->TIF_flags & TIF_GUITHREADINITIALIZED)
    {
        /* Do now some process cleanup that requires a valid win32 thread */
        if (ptiCurrent->ppi->cThreads == 0)
        {
            /* Check if we have registered the user api hook */
            if (ptiCurrent->ppi == ppiUahServer)
            {
                /* Unregister the api hook */
                UserUnregisterUserApiHook();
            }

            /* Notify logon application to restart shell if needed */
            if (ptiCurrent->pDeskInfo)
            {
                if (ptiCurrent->pDeskInfo->ppiShellProcess == ppiCurrent)
                {
                    DWORD ExitCode = PsGetProcessExitStatus(Process);

                   TRACE_CH(UserProcess, "Shell process is exiting (%lu)\n", ExitCode);

                    UserPostMessage(hwndSAS,
                                    WM_LOGONNOTIFY,
                                    LN_SHELL_EXITED,
                                    ExitCode);

                    ptiCurrent->pDeskInfo->ppiShellProcess = NULL;
                }
            }
        }

        DceFreeThreadDCE(ptiCurrent);
        DestroyTimersForThread(ptiCurrent);
        KeSetEvent(ptiCurrent->pEventQueueServer, IO_NO_INCREMENT, FALSE);
        UnregisterThreadHotKeys(ptiCurrent);

        if (!UserDestroyObjectsForOwner(gHandleTable, ptiCurrent))
        {
            DPRINT1("Failed to delete objects belonging to thread %p. This is VERY BAD!.\n", ptiCurrent);
            ASSERT(FALSE);
            return STATUS_UNSUCCESSFUL;
        }

        if (ppiCurrent && ppiCurrent->ptiList == ptiCurrent && !ptiCurrent->ptiSibling &&
            ppiCurrent->W32PF_flags & W32PF_CLASSESREGISTERED)
        {
            TRACE_CH(UserThread, "DestroyProcessClasses\n");
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
            TRACE_CH(UserThread, "thread clean: remove reference obj 0x%p\n",ref->obj);
            UserDereferenceObject(ref->obj);

            psle = PopEntryList(&ptiCurrent->ReferencesList);
        }
    }

    if (ptiCurrent->cEnterCount)
    {
       KeSetKernelStackSwapEnable(TRUE);
       ptiCurrent->cEnterCount = 0;
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

    if (gptiForeground == ptiCurrent)
    {
//       IntNotifyWinEvent(EVENT_OBJECT_FOCUS, NULL, OBJID_CLIENT, CHILDID_SELF, 0);
//       IntNotifyWinEvent(EVENT_SYSTEM_FOREGROUND, NULL, OBJID_WINDOW, CHILDID_SELF, 0);

       gptiForeground = NULL;
    }

    /* Restore display mode when we are the last thread, and we changed the display mode */
    if (ppiCurrent->cThreads == 0)
        UserDisplayNotifyShutdown(ppiCurrent);


    // Fixes CORE-6384 & CORE-7030.
/*    if (ptiLastInput == ptiCurrent)
    {
       if (!ppiCurrent->ptiList)
          ptiLastInput = gptiForeground;
       else
          ptiLastInput = ppiCurrent->ptiList;
       ERR_CH(UserThread, "DTI: ptiLastInput is Cleared!!\n");
    }
*/
    TRACE_CH(UserThread, "Freeing pti 0x%p\n", ptiCurrent);

    IntSetThreadDesktop(NULL, TRUE);

    if (ptiCurrent->hEventQueueClient != NULL)
    {
       ObCloseHandle(ptiCurrent->hEventQueueClient, UserMode);
       ObDereferenceObject(ptiCurrent->pEventQueueServer);
    }
    ptiCurrent->hEventQueueClient = NULL;

    /* The thread is dying */
    PsSetThreadWin32Thread(Thread /*ptiCurrent->pEThread*/, NULL, ptiCurrent);

    /* Dereference the THREADINFO */
    IntDereferenceThreadInfo(ptiCurrent);

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kThreadCallback(PETHREAD Thread,
                     PSW32THREADCALLOUTTYPE Type)
{
    NTSTATUS Status;

    ASSERT(NtCurrentTeb());

    UserEnterExclusive();

    if (Type == PsW32ThreadCalloutInitialize)
    {
        ASSERT(PsGetThreadWin32Thread(Thread) == NULL);
        Status = InitThreadCallback(Thread);
    }
    else // if (Type == PsW32ThreadCalloutExit)
    {
        ASSERT(PsGetThreadWin32Thread(Thread) != NULL);
        Status = ExitThreadCallback(Thread);
    }

    UserLeave();

    return Status;
}

_Function_class_(DRIVER_UNLOAD)
VOID NTAPI
DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
    // TODO: Do more cleanup!

    ResetCsrApiPort();
    ResetCsrProcess();
}

// Return on failure
#define NT_ROF(x) \
{ \
    Status = (x); \
    if (!NT_SUCCESS(Status)) \
    { \
        DPRINT1("Failed '%s' (0x%lx)\n", #x, Status); \
        return Status; \
    } \
}

// Lock & return on failure
#define USERLOCK_AND_ROF(x)         \
{                                   \
    UserEnterExclusive();           \
    Status = (x);                   \
    UserLeave();                    \
    if (!NT_SUCCESS(Status))        \
    { \
        DPRINT1("Failed '%s' (0x%lx)\n", #x, Status); \
        return Status; \
    } \
}



/*
 * This definition doesn't work
 */
CODE_SEG("INIT")
NTSTATUS
APIENTRY
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath)
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
    DPRINT("Win32k hInstance 0x%p!\n", hModuleWin);

    DriverObject->DriverUnload = DriverUnload;

    /* Register Object Manager Callbacks */
    CalloutData.ProcessCallout = Win32kProcessCallback;
    CalloutData.ThreadCallout = Win32kThreadCallback;
    // CalloutData.GlobalAtomTableCallout = NULL;
    // CalloutData.PowerEventCallout = NULL;
    // CalloutData.PowerStateCallout = NULL;
    // CalloutData.JobCallout = NULL;
    CalloutData.BatchFlushRoutine = NtGdiFlushUserBatch;
    CalloutData.DesktopOpenProcedure = IntDesktopObjectOpen;
    CalloutData.DesktopOkToCloseProcedure = IntDesktopOkToClose;
    CalloutData.DesktopCloseProcedure = IntDesktopObjectClose;
    CalloutData.DesktopDeleteProcedure = IntDesktopObjectDelete;
    CalloutData.WindowStationOkToCloseProcedure = IntWinStaOkToClose;
    // CalloutData.WindowStationCloseProcedure = NULL;
    CalloutData.WindowStationDeleteProcedure = IntWinStaObjectDelete;
    CalloutData.WindowStationParseProcedure = IntWinStaObjectParse;
    // CalloutData.WindowStationOpenProcedure = NULL;

    /* Register our per-process and per-thread structures. */
    PsEstablishWin32Callouts(&CalloutData);

    /* Register service hook callbacks */
#if DBG && defined(KDBG)
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

    /* Init the global user lock */
    ExInitializeResourceLite(&UserLock);

    /* Lock while we use the heap (UserHeapAlloc asserts on this) */
    UserEnterExclusive();

    /* Allocate global server info structure */
    gpsi = UserHeapAlloc(sizeof(*gpsi));
    UserLeave();
    if (!gpsi)
    {
        DPRINT1("Failed allocate server info structure!\n");
        return STATUS_UNSUCCESSFUL;
    }

    RtlZeroMemory(gpsi, sizeof(*gpsi));
    DPRINT("Global Server Data -> %p\n", gpsi);

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
    USERLOCK_AND_ROF(InitUserImpl());
    NT_ROF(InitWindowStationImpl());
    NT_ROF(InitDesktopImpl());
    NT_ROF(InitInputImpl());
    NT_ROF(InitKeyboardImpl());
    NT_ROF(MsqInitializeImpl());
    NT_ROF(InitTimerImpl());
    NT_ROF(InitDCEImpl());

    gusLanguageID = UserGetLanguageID();

    /* Initialize FreeType library */
    if (!InitFontSupport())
    {
        DPRINT1("Unable to initialize font support\n");
        return Status;
    }

    return STATUS_SUCCESS;
}

/* EOF */
