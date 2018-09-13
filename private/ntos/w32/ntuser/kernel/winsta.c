/**************************** Module Header ********************************\
* Module Name: winsta.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Windowstation Routines
*
* History:
* 01-14-91 JimA         Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* InitTerminal
*
* Creates the desktop thread for a terminal and also the RIT for the
* IO terminal
*
* History:
* 27-10-97 CLupu        Created.
\***************************************************************************/

NTSTATUS xxxInitTerminal(
    PTERMINAL pTerm)
{
    NTSTATUS Status;
    PKEVENT  pEventTermInit;
    HANDLE   hEventInputReady, hEventTermInit;
    HANDLE   hThreadDesktop;

    CheckCritIn();

    UserAssert(!(pTerm->dwTERMF_Flags & TERMF_INITIALIZED));

    if (pTerm->pEventInputReady != NULL) {

        /*
         * if we make it here it means that another thread is
         * executing xxxInitTerminal for the same terminal and it
         * left the critical section.
         */
        UserAssert(pTerm->pEventTermInit != NULL);

        /*
         * use a local variable so we can safely reset
         * pTerm->pEventTermInit when we're done with it
         */
        pEventTermInit = pTerm->pEventTermInit;

        ObReferenceObject(pEventTermInit);

        LeaveCrit();

        goto Wait;
    }

    /*
     * Create the input ready event. RIT and desktop thread will wait for it.
     * It will be set when the first desktop in this terminal will be created.
     */
    Status = ZwCreateEvent(
                     &hEventInputReady,
                     EVENT_ALL_ACCESS,
                     NULL,
                     NotificationEvent,
                     FALSE);

    if (!NT_SUCCESS(Status))
        return Status;

    Status = ObReferenceObjectByHandle(
                     hEventInputReady,
                     EVENT_ALL_ACCESS,
                     *ExEventObjectType,
                     KernelMode,
                     &pTerm->pEventInputReady, NULL);

    ZwClose(hEventInputReady);

    if (!NT_SUCCESS(Status))
        return Status;

    /*
     * Device and RIT initialization. Don't do it for
     * the system terminal.
     */
    if (!(pTerm->dwTERMF_Flags & TERMF_NOIO)) {
        if (!CreateTerminalInput(pTerm)) {
            ObDereferenceObject(pTerm->pEventInputReady);
            return STATUS_NO_MEMORY;
        }
    }

    /*
     * create an event to syncronize the terminal initialization
     */
    Status = ZwCreateEvent(
                     &hEventTermInit,
                     EVENT_ALL_ACCESS,
                     NULL,
                     NotificationEvent,
                     FALSE);

    if (!NT_SUCCESS(Status)) {
        ObDereferenceObject(pTerm->pEventInputReady);
        return Status;
    }

    Status = ObReferenceObjectByHandle(
                     hEventTermInit,
                     EVENT_ALL_ACCESS,
                     *ExEventObjectType,
                     KernelMode,
                     &pTerm->pEventTermInit, NULL);

    ZwClose(hEventTermInit);

    if (!NT_SUCCESS(Status)) {
        ObDereferenceObject(pTerm->pEventInputReady);
        return Status;
    }

    /*
     * use a local variable so we can safely reset
     * pTerm->pEventTermInit when we're done with it
     */
    pEventTermInit = pTerm->pEventTermInit;

    LeaveCrit();

    /*
     * Create the desktop thread.
     */
    Status = CreateSystemThread(
                     (PKSTART_ROUTINE)xxxDesktopThread,
                     pTerm,
                     &hThreadDesktop);

    if (!NT_SUCCESS(Status)) {
        EnterCrit();
        ObDereferenceObject(pTerm->pEventInputReady);
        ObDereferenceObject(pEventTermInit);
        return STATUS_NO_MEMORY;
    }

    ZwClose(hThreadDesktop);

Wait:
    KeWaitForSingleObject(pEventTermInit,
                          WrUserRequest,
                          KernelMode,
                          FALSE,
                          NULL);

    EnterCrit();

    /*
     * dereference the terminal init event. It will eventually
     * go away.
     */
    ObDereferenceObject(pEventTermInit);

    pTerm->pEventTermInit = NULL;

    if (pTerm->dwTERMF_Flags & TERMF_DTINITFAILED) {
        return STATUS_NO_MEMORY;
    }

    pTerm->dwTERMF_Flags |= TERMF_INITIALIZED;
    return STATUS_SUCCESS;
}


/***************************************************************************\
* xxxCreateWindowStation
*
* Creates the specified windowstation and starts a logon thread for the
* station.
*
* History:
* 01-15-91 JimA         Created.
\***************************************************************************/

static CONST LPCWSTR lpszStdFormats[] = {
    L"StdExit",
    L"StdNewDocument",
    L"StdOpenDocument",
    L"StdEditDocument",
    L"StdNewfromTemplate",
    L"StdCloseDocument",
    L"StdShowItem",
    L"StdDoVerbItem",
    L"System",
    L"OLEsystem",
    L"StdDocumentName",
    L"Protocols",
    L"Topics",
    L"Formats",
    L"Status",
    L"EditEnvItems",
    L"True",
    L"False",
    L"Change",
    L"Save",
    L"Close",
    L"MSDraw"
};

NTSTATUS CreateGlobalAtomTable(
    PVOID* ppAtomTable)
{
    NTSTATUS Status;
    RTL_ATOM Atom;
    ULONG    i;

    Status = RtlCreateAtomTable(0, ppAtomTable);

    if (!NT_SUCCESS(Status)) {
        RIPMSG0(RIP_WARNING, "Global atom table not created");
        return Status;
    }

    for (i = 0; i < ARRAY_SIZE(lpszStdFormats); i++) {
        Status = RtlAddAtomToAtomTable(*ppAtomTable,
                                       (PWSTR)lpszStdFormats[i],
                                       &Atom);
        if (!NT_SUCCESS(Status)) {
            RIPMSG1(RIP_WARNING, "RtlAddAtomToAtomTable failed to add atom %ws",
                    lpszStdFormats[i]);

            RtlDestroyAtomTable(*ppAtomTable);
            return Status;
        }

        RtlPinAtomInAtomTable(*ppAtomTable, Atom);
    }
    return Status;
}

HWINSTA xxxCreateWindowStation(
    POBJECT_ATTRIBUTES  ObjectAttributes,
    KPROCESSOR_MODE     OwnershipMode,
    DWORD               dwDesiredAccess,
    HANDLE              hKbdLayoutFile,
    DWORD               offTable,
    PCWSTR              pwszKLID,
    UINT                uKbdInputLocale)
{
    PWINDOWSTATION          pwinsta;
    PTHREADINFO             ptiCurrent;
    PDESKTOP                pdeskTemp;
    HDESK                   hdeskTemp;
    PSECURITY_DESCRIPTOR    psd;
    PSECURITY_DESCRIPTOR    psdCapture;
    PPROCESSINFO            ppiSave;
    NTSTATUS                Status;
    PACCESS_ALLOWED_ACE     paceList = NULL, pace;
    ULONG                   ulLength, ulLengthSid;
    HANDLE                  hEvent;
    HWINSTA                 hwinsta;
    DWORD                   dwDisableHooks;
    PTERMINAL               pTerm = NULL;
    PWND                    pwnd;
    WCHAR                   szBaseNamedObjectDirectory[MAX_SESSION_PATH];

    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     * Get the pointer to the security descriptor so we can
     * assign it to the new object later.
     */
    psdCapture = ObjectAttributes->SecurityDescriptor;

    /*
     * The first windowstation that gets created is Winsta0 and
     * it's the only interactive one.
     */
    if (grpWinStaList == NULL) {

        /*
         * Assert that winlogon is the first to call CreateWindowStation
         */
        UserAssert(PsGetCurrentProcess()->UniqueProcessId == gpidLogon);

        pTerm = &gTermIO;
    } else {
        pTerm = &gTermNOIO;

        UserAssert(grpWinStaList->rpwinstaNext == NULL ||
                   pTerm->dwTERMF_Flags & TERMF_NOIO);

        pTerm->dwTERMF_Flags |= TERMF_NOIO;
    }

    /*
     * Create the WindowStation object
     */
    Status = ObCreateObject(KernelMode, *ExWindowStationObjectType,
            ObjectAttributes, OwnershipMode, NULL, sizeof(WINDOWSTATION),
            0, 0, &pwinsta);

    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_WARNING, "Failed to create windowstation");
        return NULL;
    }

    /*
     * Initialize everything
     */
    RtlZeroMemory(pwinsta, sizeof(WINDOWSTATION));

    /*
     * Store the session id of the session who created the windowstation
     */
    pwinsta->dwSessionId = gSessionId;

    pwinsta->pTerm = pTerm;

    /*
     * All the windowstations in the system terminal are non-interactive.
     */
    if (pTerm->dwTERMF_Flags & TERMF_NOIO) {
        pwinsta->dwWSF_Flags = WSF_NOIO;
    }

    /*
     * Create the global atom table and populate it with the default OLE atoms
     * Pin each atom so they can't be deleted by bogus applications like Winword
     */
    Status = CreateGlobalAtomTable(&pwinsta->pGlobalAtomTable);

    if (pwinsta->pGlobalAtomTable == NULL) {
        UserAssert(!NT_SUCCESS(Status));
        RIPNTERR0(Status, RIP_WARNING, "CreateGlobalAtomTable failed");
        goto create_error;
    }

    /*
     * create the desktop thread
     * and the RIT (only for the IO terminal)
     */
    if (!(pTerm->dwTERMF_Flags & TERMF_INITIALIZED)) {

        Status = xxxInitTerminal(pTerm);

        if (!NT_SUCCESS(Status)) {
            RIPNTERR0(Status, RIP_WARNING, "xxxInitTerminal failed");
            goto create_error;
        }
    }

    if (!(pwinsta->dwWSF_Flags & WSF_NOIO)) {
        if (!xxxInitWindowStation(pwinsta)) {
            RIPNTERR0(STATUS_NO_MEMORY, RIP_WARNING, "xxxInitWindowStation failed");
            goto create_error;
        }
    }

    /*
     * Create only one desktop owner window per terminal.
     */
    if (pTerm->spwndDesktopOwner == NULL) {

        /*
         * Switch ppi values so window will be created using the
         * system's desktop window class.
         */
        ptiCurrent = PtiCurrent();
        ppiSave = ptiCurrent->ppi;
        ptiCurrent->ppi = pTerm->ptiDesktop->ppi;

        UserAssert(pTerm->ptiDesktop->ppi->W32PF_Flags & W32PF_CLASSESREGISTERED);

        pdeskTemp = ptiCurrent->rpdesk;            /* save current desktop */
        hdeskTemp = ptiCurrent->hdesk;
        if (pdeskTemp) {
            ObReferenceObject(pdeskTemp);
            LogDesktop(pdeskTemp, LD_REF_FN_CREATEWINDOWSTATION, TRUE, (ULONG_PTR)PtiCurrent());
        }

        /*
         * The following code is not supposed to leave the critical section because
         * CreateWindowStation is an API so the current thread can be on any state
         *  setting its pdesk to NULL it's kind of bogus
         */
        DeferWinEventNotify();
        BEGINATOMICCHECK();
        zzzSetDesktop(ptiCurrent, NULL, NULL);


        /*
         * HACK HACK HACK!!! (adams) In order to create the desktop window
         * with the correct desktop, we set the desktop of the current thread
         * to the new desktop. But in so doing we allow hooks on the current
         * thread to also hook this new desktop. This is bad, because we don't
         * want the desktop window to be hooked while it is created. So we
         * temporarily disable hooks of the current thread and desktop, and
         * reenable them after switching back to the original desktop.
         */

        dwDisableHooks = ptiCurrent->TIF_flags & TIF_DISABLEHOOKS;
        ptiCurrent->TIF_flags |= TIF_DISABLEHOOKS;

        /*
         * Create the desktop owner window
         *
         * CONSIDER (adams): Do we want to limit the desktop size so that the
         * width and height of a rect will fit in 16bit coordinates?
         *
         *         SHRT_MIN / 2, SHRT_MIN / 2, SHRT_MAX, SHRT_MAX,
         *
         * Or do we want to limit it so just any point has 16bit coordinates?
         *
         *         -SHRT_MIN, -SHRT_MIN, SHRT_MAX * 2, SHRT_MAX * 2
         */

        pwnd =  xxxCreateWindowEx(
                (DWORD)0,
                (PLARGE_STRING)DESKTOPCLASS,
                NULL,
                (WS_POPUP | WS_CLIPCHILDREN),
                SHRT_MIN / 2,
                SHRT_MIN / 2,
                SHRT_MAX,
                SHRT_MAX,
                NULL,
                NULL,
                hModuleWin,
                (LPWSTR)NULL,
                VER31
                );

        if (pwnd == NULL) {
            RIPMSG0(RIP_WARNING, "xxxCreateWindowStation: Failed to create mother desktop window");
            Status = STATUS_NO_MEMORY;
            EXITATOMICCHECK();
            zzzEndDeferWinEventNotify();
            /*
             * Restore caller's ppi
             */
            ptiCurrent->ppi = ppiSave;

            /*
             * Restore the previous desktop
             */
            zzzSetDesktop(ptiCurrent, pdeskTemp, hdeskTemp);

            goto create_error;
        }

        /*
         * Mark this handle entry that is allocated out of pool
         */
        {
            PHE phe;

            UserAssert(ptiCurrent->rpdesk == NULL);

            phe = HMPheFromObject(pwnd);
            phe->bFlags |= HANDLEF_POOL;
        }

        Lock(&(pTerm->spwndDesktopOwner), pwnd);

        UserAssert(ptiCurrent->TIF_flags & TIF_DISABLEHOOKS);
        ptiCurrent->TIF_flags = (ptiCurrent->TIF_flags & ~TIF_DISABLEHOOKS) | dwDisableHooks;

        SetVisible(pTerm->spwndDesktopOwner, SV_SET);
        HMChangeOwnerThread(pTerm->spwndDesktopOwner, pTerm->ptiDesktop);

        /*
         * Restore caller's ppi
         */
        ptiCurrent->ppi = ppiSave;

        /*
         * Restore the previous desktop
         */
        zzzSetDesktop(ptiCurrent, pdeskTemp, hdeskTemp);

        ENDATOMICCHECK();
        zzzEndDeferWinEventNotify();

        if (pdeskTemp) {
            LogDesktop(pdeskTemp, LD_DEREF_FN_CREATEWINDOWSTATION, FALSE, (ULONG_PTR)PtiCurrent());
            ObDereferenceObject(pdeskTemp);
        }
    }

    /*
     * If this is the visible windowstation, assign it to
     * the server and create the desktop switch notification
     * event.
     */
    if (!(pwinsta->dwWSF_Flags & WSF_NOIO)) {
        UNICODE_STRING strName;
        HANDLE hRootDir;
        OBJECT_ATTRIBUTES obja;

        /*
         * Create desktop switch notification event.
         */
        ulLengthSid = RtlLengthSid(SeExports->SeWorldSid);
        ulLength = ulLengthSid + sizeof(ACE_HEADER) + sizeof(ACCESS_MASK);

        /*
         * Allocate the ACE list
         */
        paceList = (PACCESS_ALLOWED_ACE)UserAllocPoolWithQuota(ulLength, TAG_SECURITY);

        if (paceList == NULL) {
            Status = STATUS_NO_MEMORY;
            goto create_error;
        }

        /*
         * Initialize ACE 0
         */
        pace = paceList;
        pace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        pace->Header.AceSize = (USHORT)ulLength;
        pace->Header.AceFlags = 0;
        pace->Mask = SYNCHRONIZE;
        RtlCopySid(ulLengthSid, &pace->SidStart, SeExports->SeWorldSid);

        /*
         * Create the SD
         */
        psd = CreateSecurityDescriptor(paceList, ulLength, FALSE);

        UserFreePool(paceList);

        if (psd == NULL) {
            Status = STATUS_NO_MEMORY;
            goto create_error;
        }

        /*
         * Create the named event.
         */
        UserAssert(ghEventSwitchDesktop == NULL);

        if (gbRemoteSession) {
            swprintf(szBaseNamedObjectDirectory, L"\\Sessions\\%ld\\BaseNamedObjects",
                     gSessionId);
            RtlInitUnicodeString(&strName, szBaseNamedObjectDirectory);
        } else {
            RtlInitUnicodeString(&strName, L"\\BaseNamedObjects");
        }

        InitializeObjectAttributes( &obja,
                                    &strName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                    );
        Status = ZwOpenDirectoryObject( &hRootDir,
                                        DIRECTORY_ALL_ACCESS &
                                            ~(DELETE | WRITE_DAC | WRITE_OWNER),
                                        &obja
                                    );
        if (NT_SUCCESS(Status)) {
            RtlInitUnicodeString(&strName, L"WinSta0_DesktopSwitch");
            InitializeObjectAttributes(&obja, &strName, OBJ_OPENIF, hRootDir, psd);
            Status = ZwCreateEvent(&hEvent, EVENT_ALL_ACCESS, &obja,
                    NotificationEvent, FALSE);
            ZwClose(hRootDir);

            if (NT_SUCCESS(Status)) {
                Status = ObReferenceObjectByHandle(hEvent, EVENT_ALL_ACCESS, *ExEventObjectType,
                        KernelMode, &gpEventSwitchDesktop, NULL);
                if (NT_SUCCESS(Status)) {

                    /*
                     * Attach to the system process and create a handle to the
                     * object.  This will ensure that the object name is retained
                     * when hEvent is closed.  This is simpler than creating a
                     * permanent object, which takes the
                     * SeCreatePermanentPrivilege.
                     */
                    KeAttachProcess(&gpepCSRSS->Pcb);

                    Status = ObOpenObjectByPointer(
                            gpEventSwitchDesktop,
                            0,
                            NULL,
                            EVENT_ALL_ACCESS,
                            NULL,
                            KernelMode,
                            &ghEventSwitchDesktop);
                    KeDetachProcess();
                }
                ZwClose(hEvent);
            }
        }
        if (!NT_SUCCESS(Status))
            goto create_error;

        UserFreePool(psd);
    }

    /*
     * Create a handle to the windowstation
     */
    Status = ObInsertObject(pwinsta, NULL, dwDesiredAccess, 1,
            &pwinsta, &hwinsta);

    if (Status == STATUS_OBJECT_NAME_EXISTS) {

        /*
         * The windowstation already exists, so deref and leave.
         */
        ObDereferenceObject(pwinsta);

    } else if (NT_SUCCESS(Status)) {
        PSECURITY_DESCRIPTOR psdParent, psdNew;
        SECURITY_SUBJECT_CONTEXT Context;
        POBJECT_DIRECTORY pParentDirectory;
        SECURITY_INFORMATION siNew;

        /*
         * Create security descriptor for the windowstation.
         * ObInsertObject only supports non-container
         * objects, so we must assign our own security descriptor.
         */
        SeCaptureSubjectContext(&Context);
        SeLockSubjectContext(&Context);

        pParentDirectory = OBJECT_HEADER_TO_NAME_INFO(
                OBJECT_TO_OBJECT_HEADER(pwinsta))->Directory;
        if (pParentDirectory != NULL)
            psdParent = OBJECT_TO_OBJECT_HEADER(pParentDirectory)->SecurityDescriptor;
        else
            psdParent = NULL;

        Status = SeAssignSecurity(
                psdParent,
                psdCapture,
                &psdNew,
                TRUE,
                &Context,
                (PGENERIC_MAPPING)&WinStaMapping,
                PagedPool);

        SeUnlockSubjectContext(&Context);
        SeReleaseSubjectContext(&Context);

        if (!NT_SUCCESS(Status)) {
#if DBG
            if (Status == STATUS_ACCESS_DENIED) {
                RIPNTERR0(Status, RIP_WARNING, "Access denied during object creation");
            } else {
                RIPNTERR1(Status, RIP_ERROR,
                            "Can't create security descriptor! Status = %#lx",
                            Status);
            }
#endif
        } else {

            /*
             * Call the security method to copy the security descriptor
             */
            siNew = (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                    DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION);
            Status = ObSetSecurityDescriptorInfo(
                    pwinsta,
                    &siNew,
                    psdNew,
                    &OBJECT_TO_OBJECT_HEADER(pwinsta)->SecurityDescriptor,
                    PagedPool,
                    (PGENERIC_MAPPING)&WinStaMapping);
            SeDeassignSecurity(&psdNew);

            if (NT_SUCCESS(Status)) {

                PWINDOWSTATION* ppwinsta;

                /*
                 * Put it on the tail of the global windowstation list
                 */
                ppwinsta = &grpWinStaList;
                while (*ppwinsta != NULL)
                    ppwinsta = &(*ppwinsta)->rpwinstaNext;
                LockWinSta(ppwinsta, pwinsta);

                /*
                 * For interactive window stations load the keyboard
                 * layout. !!!
                 */
                if (!(pwinsta->dwWSF_Flags & WSF_NOIO) && pwszKLID != NULL) {
                    if (xxxLoadKeyboardLayoutEx(
                                pwinsta,
                                hKbdLayoutFile,
                                (HKL)NULL,
                                offTable,
                                pwszKLID,
                                uKbdInputLocale,
                                KLF_ACTIVATE | KLF_INITTIME) == NULL) {
                        Status = STATUS_UNSUCCESSFUL;
                    }
                }
            }
        }
        ObDereferenceObject(pwinsta);
    }

    if (!NT_SUCCESS(Status)) {
        RIPNTERR1(Status,
                  RIP_WARNING,
                  "CreateWindowStation: Failed with Status 0x%x",
                  Status);
        return NULL;
    }

    return hwinsta;

    /*
     * Goto here if an error occurs so things can be cleaned up
     */
create_error:

    RIPNTERR1(Status,
              RIP_WARNING,
              "CreateWindowStation: Failed with Status 0x%x",
              Status);

    ObDereferenceObject(pwinsta);

    return NULL;
}

/***************************************************************************\
* FreeWindowStation
*
* Called when last lock to the windowstation is removed.  Frees all
* resources owned by the windowstation.
*
* History:
* 12-22-93 JimA         Created.
\***************************************************************************/

VOID FreeWindowStation(
    PWINDOWSTATION pwinsta)
{
    UserAssert(OBJECT_TO_OBJECT_HEADER(pwinsta)->Type == *ExWindowStationObjectType);

    /*
     * Mark the windowstation as dying.  Make sure we're not recursing.
     */
    UserAssert(!(pwinsta->dwWSF_Flags & WSF_DYING));
    pwinsta->dwWSF_Flags |= WSF_DYING;

    UserAssert(pwinsta->rpdeskList == NULL);

    /*
     * Free up the other resources
     */

    if (!(pwinsta->dwWSF_Flags & WSF_NOIO) && (gpEventSwitchDesktop != NULL)) {
        KeSetEvent(gpEventSwitchDesktop, EVENT_INCREMENT, FALSE);
        ObDereferenceObject(gpEventSwitchDesktop);
        gpEventSwitchDesktop = NULL;
    }

    BEGIN_REENTERCRIT();

    RtlDestroyAtomTable(pwinsta->pGlobalAtomTable);

    ForceEmptyClipboard(pwinsta);

    /*
     * Free up keyboard layouts
     */
    if (!(pwinsta->dwWSF_Flags & WSF_NOIO) && pwinsta->spklList != NULL) {

        PKL pkl = pwinsta->spklList;
        PKL pklFirst = pkl;

        RIPMSG2(RIP_WARNING, "FreeWindowStation: pwinsta(%p)->spklList is not NULL, %p", pwinsta, pwinsta->spklList);

        do {
            PKL pklNext = pkl->pklNext;

            HMMarkObjectDestroy(pkl);
            pkl->dwKL_Flags |= KL_UNLOADED;

            Lock(&pwinsta->spklList, pklNext);

            pkl = pklNext;

        } while (pkl != pkl->pklNext && pkl != pklFirst);

        Unlock(&pwinsta->spklList);

        HYDRA_HINT(HH_KBDLYOUTFREEWINSTA);

        /*
         * make sure the logon notify window went away
         */
        UserAssert(gspwndLogonNotify == NULL);
    } else {
        UserAssert(pwinsta->spklList == NULL);
    }

    /*
     * Free the USER sid
     */
    if (pwinsta->psidUser != NULL) {
        UserFreePool(pwinsta->psidUser);
        pwinsta->psidUser = NULL;
    }

    END_REENTERCRIT();
}

/***************************************************************************\
* DestroyWindowStation
*
* Removes the windowstation from the global list.  We can't release
* any resources until all locks have been removed.
* station.
*
* History:
* 01-17-91 JimA         Created.
\***************************************************************************/

VOID DestroyWindowStation(
    PEPROCESS Process,
    PVOID pobj,
    ACCESS_MASK amGranted,
    ULONG cProcessHandles,
    ULONG cSystemHandles)
{
    PWINDOWSTATION pwinsta = pobj;
    PWINDOWSTATION *ppwinsta;
    PDESKTOP pdesk;
    PDESKTOP pdeskLock = NULL;

    UserAssert(OBJECT_TO_OBJECT_HEADER(pobj)->Type == *ExWindowStationObjectType);

    /*
     * If this is not the last handle, leave
     */
    if (cSystemHandles != 1)
        return;

    BEGIN_REENTERCRIT();

    /*
     * If the window station was linked into the terminal's list,
     * go ahead and unlink it.
     */
    for (ppwinsta = &grpWinStaList;
            *ppwinsta != NULL && pwinsta != *ppwinsta;
            ppwinsta = &(*ppwinsta)->rpwinstaNext)
        ;
    if (*ppwinsta != NULL) {
        UnlockWinSta(ppwinsta);
        /*
         * Assert that unlocking it didn't destroy it.
         */
        UserAssert(OBJECT_TO_OBJECT_HEADER(pobj)->Type == *ExWindowStationObjectType);

        *ppwinsta = pwinsta->rpwinstaNext;
        /*
         * The instruction above transfered rpwinstaNext lock ownership to the previous
         *  element in the list. Hence the value in pwinsta can no longer be considered valid.
         */
        pwinsta->rpwinstaNext = NULL;
    }

    /*
     * Notify all console threads and wait for them to
     * terminate.
     */
    pdesk = pwinsta->rpdeskList;
    while (pdesk != NULL) {
        if (pdesk != grpdeskLogon && pdesk->dwConsoleThreadId) {
            LockDesktop(&pdeskLock, pdesk, LDL_FN_DESTROYWINDOWSTATION, 0);
            TerminateConsole(pdesk);

            /*
             * Restart scan in case desktop list has changed
             */
            pdesk = pwinsta->rpdeskList;
            UnlockDesktop(&pdeskLock, LDU_FN_DESTROYWINDOWSTATION, 0);
        } else
            pdesk = pdesk->rpdeskNext;
    }

    END_REENTERCRIT();

    UNREFERENCED_PARAMETER(Process);
    UNREFERENCED_PARAMETER(cProcessHandles);
    UNREFERENCED_PARAMETER(amGranted);
}


/***************************************************************************\
* ParseWindowStation
*
* Parse a windowstation path.
*
* History:
* 06-14-95 JimA         Created.
\***************************************************************************/

NTSTATUS ParseWindowStation(
    PVOID pContainerObject,
    POBJECT_TYPE pObjectType,
    PACCESS_STATE pAccessState,
    KPROCESSOR_MODE AccessMode,
    ULONG Attributes,
    PUNICODE_STRING pstrCompleteName,
    PUNICODE_STRING pstrRemainingName,
    PVOID Context OPTIONAL,
    PSECURITY_QUALITY_OF_SERVICE pqos,
    PVOID *pObject)
{
    PWINDOWSTATION pwinsta = pContainerObject;

    UserAssert(OBJECT_TO_OBJECT_HEADER(pContainerObject)->Type == *ExWindowStationObjectType);

    /*
     * If nothing remains to be parsed, return the windowstation.
     */
    *pObject = NULL;
    if (pstrRemainingName->Length == 0) {
        if (pObjectType != *ExWindowStationObjectType)
            return STATUS_OBJECT_TYPE_MISMATCH;

        ObReferenceObject(pwinsta);
        *pObject = pwinsta;
        return STATUS_SUCCESS;
    }

    /*
     * Skip leading path separator, if present.
     */
    if (*(pstrRemainingName->Buffer) == OBJ_NAME_PATH_SEPARATOR) {
        pstrRemainingName->Buffer++;
        pstrRemainingName->Length -= sizeof(WCHAR);
        pstrRemainingName->MaximumLength -= sizeof(WCHAR);
    }

    /*
     * Validate the desktop name.
     */
    if (wcschr(pstrRemainingName->Buffer, L'\\'))
        return STATUS_OBJECT_PATH_INVALID;
    if (pObjectType == *ExDesktopObjectType) {
        return ParseDesktop(
                pContainerObject,
                pObjectType,
                pAccessState,
                AccessMode,
                Attributes,
                pstrCompleteName,
                pstrRemainingName,
                Context,
                pqos,
                pObject);
    }

    return STATUS_OBJECT_TYPE_MISMATCH;
}


/***************************************************************************\
* OkayToCloseWindowStation
*
* We can only close windowstation handles if they're not in use.
*
* History:
* 08-Feb-1999 JerrySh   Created.
\***************************************************************************/

BOOLEAN OkayToCloseWindowStation(
    PEPROCESS Process OPTIONAL,
    PVOID Object,
    HANDLE Handle)
{
    PWINDOWSTATION pwinsta = (PWINDOWSTATION)Object;

    UNREFERENCED_PARAMETER(Process);

    UserAssert(OBJECT_TO_OBJECT_HEADER(Object)->Type == *ExWindowStationObjectType);

    /*
     * Kernel mode code can close anything.
     */
    if (KeGetPreviousMode() == KernelMode) {
        return TRUE;
    }

    /*
     * We can't close a windowstation that's being used.
     */
    if (CheckHandleInUse(Handle) || CheckHandleFlag(Handle, HF_PROTECTED)) {
        RIPMSG1(RIP_WARNING, "Trying to close windowstation %#p while still in use", pwinsta);
        return FALSE;
    }

    return TRUE;
}

/***************************************************************************\
* _OpenWindowStation
*
* Open a windowstation for the calling process
*
* History:
* 03-19-91 JimA         Created.
\***************************************************************************/

HWINSTA _OpenWindowStation(
    POBJECT_ATTRIBUTES pObjA,
    DWORD dwDesiredAccess,
    KPROCESSOR_MODE AccessMode)
{
    HWINSTA hwinsta;
    NTSTATUS Status;

    /*
     * Obja is client-side.  Ob interfaces protect and capture is
     * appropriate.
     */
    Status = ObOpenObjectByName(
            pObjA,
            *ExWindowStationObjectType,
            AccessMode,
            NULL,
            dwDesiredAccess,
            NULL,
            &hwinsta);
    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_VERBOSE, "");
        hwinsta = NULL;
    }
    return hwinsta;
}

/***************************************************************************\
* _CloseWindowStation
*
* Closes a windowstation for the calling process
*
* History:
* 15-Jun-1999 JerrySh   Created.
\***************************************************************************/

BOOL _CloseWindowStation(
    HWINSTA hwinsta)
{
    HWINSTA hwinstaCurrent;

    _GetProcessWindowStation(&hwinstaCurrent);
    if (hwinsta != hwinstaCurrent) {
        return NT_SUCCESS(ZwClose(hwinsta));
    }
    return FALSE;
}

/***************************************************************************\
* xxxSetProcessWindowStation (API)
*
* Sets the windowstation of the calling process to the windowstation
* specified by pwinsta.
*
* History:
* 01-14-91 JimA         Created.
\***************************************************************************/

BOOL xxxSetProcessWindowStation(
    HWINSTA         hwinsta,
    KPROCESSOR_MODE AccessMode)
{
    PETHREAD                    Thread = PsGetCurrentThread();
    PEPROCESS                   Process = PsGetCurrentProcess();
    HWINSTA                     hwinstaDup;
    NTSTATUS                    Status;
    PPROCESSINFO                ppi;
    PWINDOWSTATION              pwinsta;
    PWINDOWSTATION              pwinstaOld;
    OBJECT_HANDLE_INFORMATION   ohi;
    OBJECT_HANDLE_INFORMATION   ohiOld;

    if (Process == NULL) {
        UserAssert(Process);
        return FALSE;
    }

    if (Thread == NULL) {
        UserAssert(Thread);
        return FALSE;
    }

    ppi = PpiFromProcess(THREAD_TO_PROCESS(Thread));

    if (!NT_SUCCESS(ObReferenceObjectByHandle(
            hwinsta,
            0,
            *ExWindowStationObjectType,
            AccessMode,
            &pwinsta,
            &ohi))) {
        return FALSE;
    }

   /*
    * Bug 38780. Lock the handle to window station so that an app cannot free the
    * this handle by calling  GetProcessWindowStation() & CloseHandle()
    */

    /*
     * Unprotect the old hwinsta
     */
    if (ppi->hwinsta) {
        SetHandleFlag(ppi->hwinsta, HF_PROTECTED, FALSE);
    }

    /*
     * Save the WindowStation information
     */
    LockWinSta(&ppi->rpwinsta, pwinsta);
    ObDereferenceObject(pwinsta);
    ppi->hwinsta = hwinsta;

    /*
     * Protect the new Window Station Handle
     */
    SetHandleFlag(ppi->hwinsta, HF_PROTECTED, TRUE);

    /*
     * Check the old Atom Manager WindowStation to see if we are
     * changing this process' WindowStation.
     */
    if (Process->Win32WindowStation) {
        /*
         * Get a pointer to the old WindowStation object to see if it's
         * the same WindowStation that we are setting.
         */
        Status = ObReferenceObjectByHandle(
            Process->Win32WindowStation,
            0,
            *ExWindowStationObjectType,
            AccessMode,
            &pwinstaOld,
            &ohiOld);
        if (NT_SUCCESS(Status)) {
            /*
             * Are they different WindowStations?  If so, NULL out the
             * atom manager cache so we will reset it below.
             */
            if (pwinsta != pwinstaOld) {
                ZwClose(Process->Win32WindowStation);
                Process->Win32WindowStation = NULL;
            }
            ObDereferenceObject(pwinstaOld);

        } else {
            /*
             * Their Atom Manager handle is bad?  Give them a new one.
             */
            Process->Win32WindowStation = NULL;
#if DBG
            RIPMSG2(RIP_WARNING,
                    "SetProcessWindowStation: Couldn't reference old WindowStation (0x%X) Status=0x%X",
                    Process->Win32WindowStation,
                    Status);
#endif
        }
    }

    /*
     * Duplicate the WindowStation handle and stash it in the atom
     * manager's cache (Process->Win32WindowStation).  We duplicate
     * the handle in case
     */
    if (Process->Win32WindowStation == NULL) {
        Status = xxxUserDuplicateObject(
                     NtCurrentProcess(),
                     hwinsta,
                     NtCurrentProcess(),
                     &hwinstaDup,
                     0,
                     0,
                     DUPLICATE_SAME_ACCESS);

        if (NT_SUCCESS(Status)) {
            Process->Win32WindowStation = hwinstaDup;
        }
#if DBG
        else {
            RIPMSG2(RIP_WARNING,
                    "SetProcessWindowStation: Couldn't duplicate WindowStation handle (0x%X) Status=0x%X",
                    hwinsta,
                    Status);
        }
#endif
    }

    ppi->amwinsta = ohi.GrantedAccess;

    /*
     * Cache WSF_NOIO flag in the W32PROCESS so that GDI can access it.
     */
    if (pwinsta->dwWSF_Flags & WSF_NOIO) {
        ppi->W32PF_Flags &= ~W32PF_IOWINSTA;
    } else {
        ppi->W32PF_Flags |= W32PF_IOWINSTA;
    }

    /*
     * Do the access check now for readscreen so that
     * blts off of the display will be as fast as possible.
     */
    if (RtlAreAllAccessesGranted(ohi.GrantedAccess, WINSTA_READSCREEN)) {
        ppi->W32PF_Flags |= W32PF_READSCREENACCESSGRANTED;
    } else {
        ppi->W32PF_Flags &= ~W32PF_READSCREENACCESSGRANTED;
    }

    return TRUE;
}


/***************************************************************************\
* _GetProcessWindowStation (API)
*
* Returns a pointer to the windowstation of the calling process.
*
* History:
* 01-14-91 JimA         Created.
\***************************************************************************/

PWINDOWSTATION _GetProcessWindowStation(
    HWINSTA *phwinsta)
{
    PPROCESSINFO ppi;

    ppi = PpiCurrent();
    UserAssert(ppi);

    if (phwinsta)
        *phwinsta = ppi->hwinsta;
    return ppi->rpwinsta;
}


/***************************************************************************\
* _BuildNameList
*
* Builds a list of windowstation or desktop names.
*
* History:
* 05-17-94 JimA         Created.
* 10-21-96 CLupu        Added TERMINAL enumeration
\***************************************************************************/

NTSTATUS _BuildNameList(
    PWINDOWSTATION pwinsta,
    PNAMELIST      ccxpNameList,
    UINT           cbNameList,
    PUINT          pcbNeeded)
{
    PBYTE                    pobj;
    PWCHAR                   ccxpwchDest, ccxpwchMax;
    ACCESS_MASK              amDesired;
    POBJECT_HEADER           pHead;
    POBJECT_HEADER_NAME_INFO pNameInfo;
    DWORD                    iNext;
    NTSTATUS                 Status;
    CONST GENERIC_MAPPING *pGenericMapping;

/*
 * Note -- NameList is client-side, and so must be protected.
 */

    try {
        ccxpNameList->cNames = 0;
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        return STATUS_ACCESS_VIOLATION;
    }

    ccxpwchDest = ccxpNameList->awchNames;
    ccxpwchMax = (PWCHAR)((PBYTE)ccxpNameList + cbNameList - sizeof(WCHAR));

    /*
     * If we're enumerating windowstations, pwinsta is NULL.  Otherwise,
     * we're enumerating desktops.
     */
    if (pwinsta == NULL) {
        pobj  = (PBYTE)grpWinStaList;
        amDesired = WINSTA_ENUMERATE;
        pGenericMapping = &WinStaMapping;
        iNext = FIELD_OFFSET(WINDOWSTATION, rpwinstaNext);
    } else {
        pobj = (PBYTE)pwinsta->rpdeskList;
        amDesired = DESKTOP_ENUMERATE;
        pGenericMapping = &DesktopMapping;
        iNext = FIELD_OFFSET(DESKTOP, rpdeskNext);
    }

    Status = STATUS_SUCCESS;
    *pcbNeeded = 0;
    while (pobj != NULL) {

        if (AccessCheckObject(pobj, amDesired, KernelMode, pGenericMapping)) {

            /*
             * Find object name
             */
            pHead = OBJECT_TO_OBJECT_HEADER(pobj);
            pNameInfo = OBJECT_HEADER_TO_NAME_INFO(pHead);

            /*
             * If we run out of space, reset the buffer
             * and continue so we can compute the needed
             * space.
             */
            if ((PWCHAR)((PBYTE)ccxpwchDest + pNameInfo->Name.Length +
                    sizeof(WCHAR)) >= ccxpwchMax) {
                *pcbNeeded += (UINT)((PBYTE)ccxpwchDest - (PBYTE)ccxpNameList);
                ccxpwchDest = ccxpNameList->awchNames;
                Status = STATUS_BUFFER_TOO_SMALL;
            }

            try {
                ccxpNameList->cNames++;

                /*
                 * Copy and terminate the string
                 */
                RtlCopyMemory(ccxpwchDest, pNameInfo->Name.Buffer,
                    pNameInfo->Name.Length);
                (PBYTE)ccxpwchDest += pNameInfo->Name.Length;
                *ccxpwchDest++ = 0;
            } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
                return STATUS_ACCESS_VIOLATION;
            }
        }

        pobj = *(PBYTE*)(pobj + iNext);
    }

    /*
     * Put an empty string on the end.
     */
    try {
        *ccxpwchDest++ = 0;

        ccxpNameList->cb = (UINT)((PBYTE)ccxpwchDest - (PBYTE)ccxpNameList);
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        return STATUS_ACCESS_VIOLATION;
    }

    *pcbNeeded += (UINT)((PBYTE)ccxpwchDest - (PBYTE)ccxpNameList);

    return Status;
}

NTSTATUS ReferenceWindowStation(
    PETHREAD Thread,
    HWINSTA hwinsta,
    ACCESS_MASK amDesiredAccess,
    PWINDOWSTATION *ppwinsta,
    BOOL fUseDesktop)
{
    PPROCESSINFO ppi;
    PTHREADINFO pti;
    PWINDOWSTATION pwinsta = NULL;
    NTSTATUS Status;

    /*
     * We prefer to use the thread's desktop to dictate which
     * windowstation/Atom table to use rather than the process.
     * This allows NetDDE, which has threads running under
     * different desktops on different windowstations but whos
     * process is set to only one of these windowstations, to
     * get global atoms properly without having to change its
     * process windowstation a billion times and synchronize.
     */
    ppi = PpiFromProcess(Thread->ThreadsProcess);
    pti = PtiFromThread(Thread);

    /*
     * First, try to get the windowstation from the pti, and then
     * from the ppi.
     */
    if (ppi != NULL) {
        if (!fUseDesktop || pti == NULL || pti->rpdesk == NULL ||
                ppi->rpwinsta == pti->rpdesk->rpwinstaParent) {

            /*
             * Use the windowstation assigned to the process.
             */
            pwinsta = ppi->rpwinsta;
            if (pwinsta != NULL) {
                RETURN_IF_ACCESS_DENIED(ppi->amwinsta, amDesiredAccess,
                        STATUS_ACCESS_DENIED);
            }
        }

        /*
         * If we aren't using the process' windowstation, try to
         * go through the thread's desktop.
         */
        if (pwinsta == NULL && pti != NULL && pti->rpdesk != NULL) {

            /*
             * Perform access check the parent windowstation.  This
             * is an expensive operation.
             */
            pwinsta = pti->rpdesk->rpwinstaParent;
            if (!AccessCheckObject(pwinsta, amDesiredAccess, KernelMode, &WinStaMapping))
                return STATUS_ACCESS_DENIED;
        }
    }

    /*
     * If we still don't have a windowstation and a handle was
     * passed in, use it.
     */
    if (pwinsta == NULL) {
        if (hwinsta != NULL) {
            Status = ObReferenceObjectByHandle(
                    hwinsta,
                    amDesiredAccess,
                    *ExWindowStationObjectType,
                    KernelMode,
                    &pwinsta,
                    NULL);
            if (!NT_SUCCESS(Status))
                return Status;
            ObDereferenceObject(pwinsta);
        } else {
            return STATUS_NOT_FOUND;
        }
    }

    *ppwinsta = pwinsta;

    return STATUS_SUCCESS;
}

/***************************************************************************\
* _SetWindowStationUser
*
* Private API for winlogon to associate a windowstation with a user.
*
* History:
* 06-27-94 JimA         Created.
\***************************************************************************/

UINT _SetWindowStationUser(
    PWINDOWSTATION pwinsta,
    PLUID pluidUser,
    PSID ccxpsidUser,
    DWORD cbsidUser)
{

    /*
     * Make sure the caller is the logon process
     */
    if (GetCurrentProcessId() != gpidLogon) {
        RIPERR0(ERROR_ACCESS_DENIED,
                RIP_WARNING,
                "Access denied in _SetWindowStationUser: caller must be in the logon process");

        return FALSE;
    }

    if (pwinsta->psidUser != NULL)
        UserFreePool(pwinsta->psidUser);

    if (ccxpsidUser != NULL) {
        pwinsta->psidUser = UserAllocPoolWithQuota(cbsidUser, TAG_SECURITY);
        if (pwinsta->psidUser == NULL) {
            RIPERR0(ERROR_OUTOFMEMORY,
                    RIP_WARNING,
                    "Memory allocation failed in _SetWindowStationUser");

            return FALSE;
        }
        try {
            RtlCopyMemory(pwinsta->psidUser, ccxpsidUser, cbsidUser);
        } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {

            UserFreePool(pwinsta->psidUser);
            pwinsta->psidUser = NULL;
            return FALSE;
        }
    } else {
        pwinsta->psidUser = NULL;
    }

    pwinsta->luidUser = *pluidUser;

    return TRUE;
}


/***************************************************************************\
* _LockWorkStation (API)
*
* locks the workstation. This API just posts a message to winlogon
* and winlogon does all the work
*
* History:
* 06-11-97 CLupu        Created.
\***************************************************************************/

BOOL _LockWorkStation(
    VOID)
{
    UserAssert(gspwndLogonNotify != NULL);

    _PostMessage(gspwndLogonNotify,
                 WM_LOGONNOTIFY, LOGON_LOCKWORKSTATION, 0);

    return TRUE;
}

#ifdef LATER    // HY
BOOL _IsIoDesktop(
    HDESK hdesk)
{
    BOOL fRet = FALSE;
    NTSTATUS Status;
    PDESKTOP pdesk = NULL;

    Status = ValidateHdesk(hdesk, UserMode, 0, &pdesk);
    if (NT_SUCCESS(Status)) {
        UserAssert(pdesk && pdesk->rpwinstaParent);
        fRet = (pdesk->rpwinstaParent->dwWSF_Flags & WSF_NOIO) == 0;
        ObDereferenceObject(pdesk);
    }
    return fRet;
}
#endif

