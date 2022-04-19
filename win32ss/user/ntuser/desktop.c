/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS Win32k subsystem
 *  PURPOSE:          Desktops
 *  FILE:             subsystems/win32/win32k/ntuser/desktop.c
 *  PROGRAMMER:       Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserDesktop);

#include <reactos/buildno.h>

static NTSTATUS
UserInitializeDesktop(PDESKTOP pdesk, PUNICODE_STRING DesktopName, PWINSTATION_OBJECT pwinsta);

static NTSTATUS
IntMapDesktopView(IN PDESKTOP pdesk);

static NTSTATUS
IntUnmapDesktopView(IN PDESKTOP pdesk);

static VOID
IntFreeDesktopHeap(IN PDESKTOP pdesk);

/* GLOBALS *******************************************************************/

/* These can be changed via CSRSS startup, these are defaults */
DWORD gdwDesktopSectionSize = 512;
DWORD gdwNOIOSectionSize    = 128; // A guess, for one or more of the first three system desktops.

/* Currently active desktop */
PDESKTOP gpdeskInputDesktop = NULL;
HDC ScreenDeviceContext = NULL;
PTHREADINFO gptiDesktopThread = NULL;
HCURSOR gDesktopCursor = NULL;
PKEVENT gpDesktopThreadStartedEvent = NULL;

/* OBJECT CALLBACKS **********************************************************/

NTSTATUS
APIENTRY
IntDesktopObjectParse(IN PVOID ParseObject,
                      IN PVOID ObjectType,
                      IN OUT PACCESS_STATE AccessState,
                      IN KPROCESSOR_MODE AccessMode,
                      IN ULONG Attributes,
                      IN OUT PUNICODE_STRING CompleteName,
                      IN OUT PUNICODE_STRING RemainingName,
                      IN OUT PVOID Context OPTIONAL,
                      IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
                      OUT PVOID *Object)
{
    NTSTATUS Status;
    PDESKTOP Desktop;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PLIST_ENTRY NextEntry, ListHead;
    PWINSTATION_OBJECT WinStaObject = (PWINSTATION_OBJECT)ParseObject;
    UNICODE_STRING DesktopName;
    PBOOLEAN pContext = (PBOOLEAN) Context;

    if (pContext)
        *pContext = FALSE;

    /* Set the list pointers and loop the window station */
    ListHead = &WinStaObject->DesktopListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the current desktop */
        Desktop = CONTAINING_RECORD(NextEntry, DESKTOP, ListEntry);

        /* Get the desktop name */
        ASSERT(Desktop->pDeskInfo != NULL);
        RtlInitUnicodeString(&DesktopName, Desktop->pDeskInfo->szDesktopName);

        /* Compare the name */
        if (RtlEqualUnicodeString(RemainingName,
                                  &DesktopName,
                                  (Attributes & OBJ_CASE_INSENSITIVE) != 0))
        {
            /* We found a match. Did this come from a create? */
            if (Context)
            {
                /* Unless OPEN_IF was given, fail with an error */
                if (!(Attributes & OBJ_OPENIF))
                {
                    /* Name collision */
                    return STATUS_OBJECT_NAME_COLLISION;
                }
                else
                {
                    /* Otherwise, return with a warning only */
                    Status = STATUS_OBJECT_NAME_EXISTS;
                }
            }
            else
            {
                /* This was a real open, so this is OK */
                Status = STATUS_SUCCESS;
            }

            /* Reference the desktop and return it */
            ObReferenceObject(Desktop);
            *Object = Desktop;
            return Status;
        }

        /* Go to the next desktop */
        NextEntry = NextEntry->Flink;
    }

    /* If we got here but this isn't a create, just fail */
    if (!Context) return STATUS_OBJECT_NAME_NOT_FOUND;

    /* Create the desktop object */
    InitializeObjectAttributes(&ObjectAttributes, RemainingName, 0, NULL, NULL);
    Status = ObCreateObject(KernelMode,
                            ExDesktopObjectType,
                            &ObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(DESKTOP),
                            0,
                            0,
                            (PVOID*)&Desktop);
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the desktop */
    Status = UserInitializeDesktop(Desktop, RemainingName, WinStaObject);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(Desktop);
        return Status;
    }

    /* Set the desktop object and return success */
    *Object = Desktop;
    *pContext = TRUE;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IntDesktopObjectDelete(
    _In_ PVOID Parameters)
{
    PWIN32_DELETEMETHOD_PARAMETERS DeleteParameters = Parameters;
    PDESKTOP pdesk = (PDESKTOP)DeleteParameters->Object;

    TRACE("Deleting desktop object 0x%p\n", pdesk);

    if (pdesk->pDeskInfo &&
        pdesk->pDeskInfo->spwnd)
    {
        ASSERT(pdesk->pDeskInfo->spwnd->spwndChild == NULL);
        co_UserDestroyWindow(pdesk->pDeskInfo->spwnd);
    }

    if (pdesk->spwndMessage)
        co_UserDestroyWindow(pdesk->spwndMessage);

    /* Remove the desktop from the window station's list of associcated desktops */
    RemoveEntryList(&pdesk->ListEntry);

    /* Free the heap */
    IntFreeDesktopHeap(pdesk);

    ObDereferenceObject(pdesk->rpwinstaParent);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IntDesktopOkToClose(
    _In_ PVOID Parameters)
{
    PWIN32_OKAYTOCLOSEMETHOD_PARAMETERS OkToCloseParameters = Parameters;
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

    if (pti == NULL)
    {
        /* This happens when we leak desktop handles */
        return STATUS_SUCCESS;
    }

    /* Do not allow the current desktop or the initial desktop to be closed */
    if (OkToCloseParameters->Handle == pti->ppi->hdeskStartup ||
        OkToCloseParameters->Handle == pti->hdesk)
    {
        return STATUS_ACCESS_DENIED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IntDesktopObjectOpen(
    _In_ PVOID Parameters)
{
    NTSTATUS Ret;
    PWIN32_OPENMETHOD_PARAMETERS OpenParameters = Parameters;
    PPROCESSINFO ppi = PsGetProcessWin32Process(OpenParameters->Process);
    if (ppi == NULL)
        return STATUS_SUCCESS;

    UserEnterExclusive();
    Ret = IntMapDesktopView((PDESKTOP)OpenParameters->Object);
    UserLeave();
    return Ret;
}

NTSTATUS
NTAPI
IntDesktopObjectClose(
    _In_ PVOID Parameters)
{
    NTSTATUS Ret;
    PWIN32_CLOSEMETHOD_PARAMETERS CloseParameters = Parameters;
    PPROCESSINFO ppi = PsGetProcessWin32Process(CloseParameters->Process);
    if (ppi == NULL)
    {
        /* This happens when the process leaks desktop handles.
         * At this point the PPROCESSINFO is already destroyed */
        return STATUS_SUCCESS;
    }

    UserEnterExclusive();
    Ret = IntUnmapDesktopView((PDESKTOP)CloseParameters->Object);
    UserLeave();
    return Ret;
}


/* PRIVATE FUNCTIONS **********************************************************/

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitDesktopImpl(VOID)
{
    GENERIC_MAPPING IntDesktopMapping = { DESKTOP_READ,
                                          DESKTOP_WRITE,
                                          DESKTOP_EXECUTE,
                                          DESKTOP_ALL_ACCESS};

    /* Set Desktop Object Attributes */
    ExDesktopObjectType->TypeInfo.DefaultNonPagedPoolCharge = sizeof(DESKTOP);
    ExDesktopObjectType->TypeInfo.GenericMapping = IntDesktopMapping;
    ExDesktopObjectType->TypeInfo.ValidAccessMask = DESKTOP_ALL_ACCESS;

    /* Allocate memory for the event structure */
    gpDesktopThreadStartedEvent = ExAllocatePoolWithTag(NonPagedPool,
                                                        sizeof(KEVENT),
                                                        USERTAG_EVENT);
    if (!gpDesktopThreadStartedEvent)
    {
        ERR("Failed to allocate event!\n");
        return STATUS_NO_MEMORY;
    }

    /* Initialize the kernel event */
    KeInitializeEvent(gpDesktopThreadStartedEvent,
                      SynchronizationEvent,
                      FALSE);

    return STATUS_SUCCESS;
}

static NTSTATUS
GetSystemVersionString(OUT PWSTR pwszzVersion,
                       IN SIZE_T cchDest,
                       IN BOOLEAN InSafeMode,
                       IN BOOLEAN AppendNtSystemRoot)
{
    NTSTATUS Status;

    RTL_OSVERSIONINFOEXW VerInfo;
    UNICODE_STRING BuildLabString;
    UNICODE_STRING CSDVersionString;
    RTL_QUERY_REGISTRY_TABLE VersionConfigurationTable[] =
    {
        {
            NULL,
            RTL_QUERY_REGISTRY_DIRECT,
            L"BuildLab",
            &BuildLabString,
            REG_NONE, NULL, 0
        },
        {
            NULL,
            RTL_QUERY_REGISTRY_DIRECT,
            L"CSDVersion",
            &CSDVersionString,
            REG_NONE, NULL, 0
        },

        {0}
    };

    WCHAR BuildLabBuffer[256];
    WCHAR VersionBuffer[256];
    PWCHAR EndBuffer;

    VerInfo.dwOSVersionInfoSize = sizeof(VerInfo);

    /*
     * This call is uniquely used to retrieve the current CSD numbers.
     * All the rest (major, minor, ...) is either retrieved from the
     * SharedUserData structure, or from the registry.
     */
    RtlGetVersion((PRTL_OSVERSIONINFOW)&VerInfo);

    /*
     * - Retrieve the BuildLab string from the registry (set by the kernel).
     * - In kernel-mode, szCSDVersion is not initialized. Initialize it
     *   and query its value from the registry.
     */
    RtlZeroMemory(BuildLabBuffer, sizeof(BuildLabBuffer));
    RtlInitEmptyUnicodeString(&BuildLabString,
                              BuildLabBuffer,
                              sizeof(BuildLabBuffer));
    RtlZeroMemory(VerInfo.szCSDVersion, sizeof(VerInfo.szCSDVersion));
    RtlInitEmptyUnicodeString(&CSDVersionString,
                              VerInfo.szCSDVersion,
                              sizeof(VerInfo.szCSDVersion));
    Status = RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT,
                                    L"",
                                    VersionConfigurationTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Indicate nothing is there */
        BuildLabString.Length = 0;
        CSDVersionString.Length = 0;
    }
    /* NULL-terminate the strings */
    BuildLabString.Buffer[BuildLabString.Length / sizeof(WCHAR)] = UNICODE_NULL;
    CSDVersionString.Buffer[CSDVersionString.Length / sizeof(WCHAR)] = UNICODE_NULL;

    EndBuffer = VersionBuffer;
    if ( /* VerInfo.wServicePackMajor != 0 && */ CSDVersionString.Length)
    {
        /* Print the version string */
        Status = RtlStringCbPrintfExW(VersionBuffer,
                                      sizeof(VersionBuffer),
                                      &EndBuffer,
                                      NULL,
                                      0,
                                      L": %wZ",
                                      &CSDVersionString);
        if (!NT_SUCCESS(Status))
        {
            /* No version, NULL-terminate the string */
            *EndBuffer = UNICODE_NULL;
        }
    }
    else
    {
        /* No version, NULL-terminate the string */
        *EndBuffer = UNICODE_NULL;
    }

    if (InSafeMode)
    {
        /* String for Safe Mode */
        Status = RtlStringCchPrintfW(pwszzVersion,
                                     cchDest,
                                     L"ReactOS Version %S %wZ (NT %u.%u Build %u%s)\n",
                                     KERNEL_VERSION_STR,
                                     &BuildLabString,
                                     SharedUserData->NtMajorVersion,
                                     SharedUserData->NtMinorVersion,
                                     (VerInfo.dwBuildNumber & 0xFFFF),
                                     VersionBuffer);

        if (AppendNtSystemRoot && NT_SUCCESS(Status))
        {
            Status = RtlStringCbPrintfW(VersionBuffer,
                                        sizeof(VersionBuffer),
                                        L" - %s\n",
                                        SharedUserData->NtSystemRoot);
            if (NT_SUCCESS(Status))
            {
                /* Replace the last newline by a NULL, before concatenating */
                EndBuffer = wcsrchr(pwszzVersion, L'\n');
                if (EndBuffer) *EndBuffer = UNICODE_NULL;

                /* The concatenated string has a terminating newline */
                Status = RtlStringCchCatW(pwszzVersion,
                                          cchDest,
                                          VersionBuffer);
                if (!NT_SUCCESS(Status))
                {
                    /* Concatenation failed, put back the newline */
                    if (EndBuffer) *EndBuffer = L'\n';
                }
            }

            /* Override any failures as the NtSystemRoot string is optional */
            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        /* Multi-string for Normal Mode */
        Status = RtlStringCchPrintfW(pwszzVersion,
                                     cchDest,
                                     L"ReactOS Version %S\n"
                                     L"Build %wZ\n"
                                     L"Reporting NT %u.%u (Build %u%s)\n",
                                     KERNEL_VERSION_STR,
                                     &BuildLabString,
                                     SharedUserData->NtMajorVersion,
                                     SharedUserData->NtMinorVersion,
                                     (VerInfo.dwBuildNumber & 0xFFFF),
                                     VersionBuffer);

        if (AppendNtSystemRoot && NT_SUCCESS(Status))
        {
            Status = RtlStringCbPrintfW(VersionBuffer,
                                        sizeof(VersionBuffer),
                                        L"%s\n",
                                        SharedUserData->NtSystemRoot);
            if (NT_SUCCESS(Status))
            {
                Status = RtlStringCchCatW(pwszzVersion,
                                          cchDest,
                                          VersionBuffer);
            }

            /* Override any failures as the NtSystemRoot string is optional */
            Status = STATUS_SUCCESS;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        /* Fall-back string */
        Status = RtlStringCchPrintfW(pwszzVersion,
                                     cchDest,
                                     L"ReactOS Version %S %wZ\n",
                                     KERNEL_VERSION_STR,
                                     &BuildLabString);
        if (!NT_SUCCESS(Status))
        {
            /* General failure, NULL-terminate the string */
            pwszzVersion[0] = UNICODE_NULL;
        }
    }

    /*
     * Convert the string separators (newlines) into NULLs
     * and NULL-terminate the multi-string.
     */
    while (*pwszzVersion)
    {
        EndBuffer = wcschr(pwszzVersion, L'\n');
        if (!EndBuffer) break;
        pwszzVersion = EndBuffer;

        *pwszzVersion++ = UNICODE_NULL;
    }
    *pwszzVersion = UNICODE_NULL;

    return Status;
}


/*
 * IntResolveDesktop
 *
 * The IntResolveDesktop function attempts to retrieve valid handles to
 * a desktop and a window station suitable for the specified process.
 * The specified desktop path string is used only as a hint for the resolution.
 *
 * - If the process is already assigned to a window station and a desktop,
 *   handles to these objects are returned directly regardless of the specified
 *   desktop path string. This is what happens when this function is called for
 *   a process that has been already started and connected to the Win32 USER.
 *
 * - If the process is being connected to the Win32 USER, or is in a state
 *   where a window station is assigned to it but no desktop yet, the desktop
 *   path string is used as a hint for the resolution.
 *   A specified window station (if any, otherwise "WinSta0" is used as default)
 *   is tested for existence and accessibility. If the checks are OK a handle
 *   to it is returned. Otherwise we either fail (the window station does not
 *   exist) or, in case a default window station was used, we attempt to open
 *   or create a non-interactive Service-0xXXXX-YYYY$ window station. This is
 *   typically what happens when a non-interactive process is started while
 *   the WinSta0 window station was used as the default one.
 *   A specified desktop (if any, otherwise "Default" is used as default)
 *   is then tested for existence on the opened window station.
 *
 * - Rules for the choice of the default window station, when none is specified
 *   in the desktop path:
 *
 *   1. By default, a SYSTEM process connects to a non-interactive window
 *      station, either the Service-0x0-3e7$ (from the SYSTEM LUID) station,
 *      or one that has been inherited and that is non-interactive.
 *      Only when the interactive window station WinSta0 is specified that
 *      the process can connect to it (e.g. the case of interactive services).
 *
 *   2. An interactive process, i.e. a process whose LUID is the same as the
 *      one assigned to WinSta0 by Winlogon on user logon, connects by default
 *      to the WinSta0 window station, unless it has inherited from another
 *      interactive window station (which must be... none other than WinSta0).
 *
 *   3. A non-interactive (but not SYSTEM) process connects by default to
 *      a non-interactive Service-0xXXXX-YYYY$ window station (whose name
 *      is derived from the process' LUID), or to another non-interactive
 *      window station that has been inherited.
 *      Otherwise it may be able connect to the interactive WinSta0 only if
 *      it has explicit access rights to it.
 *
 * Parameters
 *    Process
 *       The user process object.
 *
 *    DesktopPath
 *       The desktop path string used as a hint for desktop resolution.
 *
 *    bInherit
 *       Whether or not the returned handles are inheritable.
 *
 *    phWinSta
 *       Pointer to a window station handle.
 *
 *    phDesktop
 *       Pointer to a desktop handle.
 *
 * Return Value
 *    Status code.
 */

NTSTATUS
FASTCALL
IntResolveDesktop(
    IN PEPROCESS Process,
    IN PUNICODE_STRING DesktopPath,
    IN BOOL bInherit,
    OUT HWINSTA* phWinSta,
    OUT HDESK* phDesktop)
{
    NTSTATUS Status;
    HWINSTA hWinSta = NULL, hWinStaDup = NULL;
    HDESK hDesktop = NULL, hDesktopDup = NULL;
    PPROCESSINFO ppi;
    HANDLE hProcess = NULL;
    LUID ProcessLuid;
    USHORT StrSize;
    SIZE_T MemSize;
    POBJECT_ATTRIBUTES ObjectAttributes = NULL;
    PUNICODE_STRING ObjectName;
    UNICODE_STRING WinStaName, DesktopName;
    const UNICODE_STRING WinSta0Name = RTL_CONSTANT_STRING(L"WinSta0");
    PWINSTATION_OBJECT WinStaObject;
    HWINSTA hTempWinSta = NULL;
    BOOLEAN bUseDefaultWinSta = FALSE;
    BOOLEAN bInteractive = FALSE;
    BOOLEAN bAccessAllowed = FALSE;

    ASSERT(UserIsEnteredExclusive());

    ASSERT(phWinSta);
    ASSERT(phDesktop);
    ASSERT(DesktopPath);

    *phWinSta  = NULL;
    *phDesktop = NULL;

    ppi = PsGetProcessWin32Process(Process);
    /* ppi is typically NULL for console applications that connect to Win32 USER */
    if (!ppi) TRACE("IntResolveDesktop: ppi is NULL!\n");

    if (ppi && ppi->hwinsta != NULL && ppi->hdeskStartup != NULL)
    {
        /*
         * If this process is the current one, just return the cached handles.
         * Otherwise, open the window station and desktop objects.
         */
        if (Process == PsGetCurrentProcess())
        {
            hWinSta  = ppi->hwinsta;
            hDesktop = ppi->hdeskStartup;
        }
        else
        {
            Status = ObOpenObjectByPointer(ppi->prpwinsta,
                                           0,
                                           NULL,
                                           MAXIMUM_ALLOWED,
                                           ExWindowStationObjectType,
                                           UserMode,
                                           (PHANDLE)&hWinSta);
            if (!NT_SUCCESS(Status))
            {
                ERR("IntResolveDesktop: Could not reference window station 0x%p\n", ppi->prpwinsta);
                SetLastNtError(Status);
                return Status;
            }

            Status = ObOpenObjectByPointer(ppi->rpdeskStartup,
                                           0,
                                           NULL,
                                           MAXIMUM_ALLOWED,
                                           ExDesktopObjectType,
                                           UserMode,
                                           (PHANDLE)&hDesktop);
            if (!NT_SUCCESS(Status))
            {
                ERR("IntResolveDesktop: Could not reference desktop 0x%p\n", ppi->rpdeskStartup);
                ObCloseHandle(hWinSta, UserMode);
                SetLastNtError(Status);
                return Status;
            }
        }

        *phWinSta  = hWinSta;
        *phDesktop = hDesktop;
        return STATUS_SUCCESS;
    }

    /* We will by default use the default window station and desktop */
    RtlInitEmptyUnicodeString(&WinStaName, NULL, 0);
    RtlInitEmptyUnicodeString(&DesktopName, NULL, 0);

    /*
     * Parse the desktop path string which can be of the form "WinSta\Desktop"
     * or just "Desktop". In the latter case we use the default window station
     * on which the process is attached to (or if none, "WinSta0").
     */
    if (DesktopPath->Buffer != NULL && DesktopPath->Length > sizeof(WCHAR))
    {
        DesktopName = *DesktopPath;

        /* Find the separator */
        while (DesktopName.Length > 0 && *DesktopName.Buffer &&
               *DesktopName.Buffer != OBJ_NAME_PATH_SEPARATOR)
        {
            DesktopName.Buffer++;
            DesktopName.Length -= sizeof(WCHAR);
            DesktopName.MaximumLength -= sizeof(WCHAR);
        }
        if (DesktopName.Length > 0)
        {
            RtlInitEmptyUnicodeString(&WinStaName, DesktopPath->Buffer,
                                      DesktopPath->Length - DesktopName.Length);
            // (USHORT)((ULONG_PTR)DesktopName.Buffer - (ULONG_PTR)DesktopPath->Buffer);
            WinStaName.Length = WinStaName.MaximumLength;

            /* Skip the separator */
            DesktopName.Buffer++;
            DesktopName.Length -= sizeof(WCHAR);
            DesktopName.MaximumLength -= sizeof(WCHAR);
        }
        else
        {
            RtlInitEmptyUnicodeString(&WinStaName, NULL, 0);
            DesktopName = *DesktopPath;
        }
    }

    TRACE("IntResolveDesktop: WinStaName:'%wZ' ; DesktopName:'%wZ'\n", &WinStaName, &DesktopName);

    /* Retrieve the process LUID */
    Status = GetProcessLuid(NULL, Process, &ProcessLuid);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntResolveDesktop: Failed to retrieve the process LUID, Status 0x%08lx\n", Status);
        SetLastNtError(Status);
        return Status;
    }

    /*
     * If this process is not the current one, obtain a temporary handle
     * to it so that we can perform handles duplication later.
     */
    if (Process != PsGetCurrentProcess())
    {
        Status = ObOpenObjectByPointer(Process,
                                       OBJ_KERNEL_HANDLE,
                                       NULL,
                                       0,
                                       *PsProcessType,
                                       KernelMode,
                                       &hProcess);
        if (!NT_SUCCESS(Status))
        {
            ERR("IntResolveDesktop: Failed to obtain a handle to process 0x%p, Status 0x%08lx\n", Process, Status);
            SetLastNtError(Status);
            return Status;
        }
        ASSERT(hProcess);
    }

    /*
     * If no window station has been specified, search the process handle table
     * for inherited window station handles, otherwise use a default one.
     */
    if (WinStaName.Buffer == NULL)
    {
        /*
         * We want to find a suitable default window station.
         * For applications that can be interactive, i.e. that have allowed
         * access to the single interactive window station on the system,
         * the default window station is 'WinSta0'.
         * For applications that cannot be interactive, i.e. that do not have
         * access to 'WinSta0' (e.g. non-interactive services), the default
         * window station is 'Service-0xXXXX-YYYY$' (created if needed).
         * Precedence will however be taken by any inherited window station
         * that possesses the required interactivity property.
         */
        bUseDefaultWinSta = TRUE;

        /*
         * Use the default 'WinSta0' window station. Whether we should
         * use 'Service-0xXXXX-YYYY$' instead will be determined later.
         */
        // RtlInitUnicodeString(&WinStaName, L"WinSta0");
        WinStaName = WinSta0Name;

        if (ObFindHandleForObject(Process,
                                  NULL,
                                  ExWindowStationObjectType,
                                  NULL,
                                  (PHANDLE)&hWinSta))
        {
            TRACE("IntResolveDesktop: Inherited window station is: 0x%p\n", hWinSta);
        }
    }

    /*
     * If no desktop has been specified, search the process handle table
     * for inherited desktop handles, otherwise use the Default desktop.
     * Note that the inherited desktop that we may use, may not belong
     * to the window station we will connect to.
     */
    if (DesktopName.Buffer == NULL)
    {
        /* Use a default desktop name */
        RtlInitUnicodeString(&DesktopName, L"Default");

        if (ObFindHandleForObject(Process,
                                  NULL,
                                  ExDesktopObjectType,
                                  NULL,
                                  (PHANDLE)&hDesktop))
        {
            TRACE("IntResolveDesktop: Inherited desktop is: 0x%p\n", hDesktop);
        }
    }


    /*
     * We are going to open either a window station or a desktop.
     * Even if this operation is done from kernel-mode, we should
     * "emulate" an opening from user-mode (i.e. using an ObjectAttributes
     * allocated in user-mode, with AccessMode == UserMode) for the
     * Object Manager to perform proper access validation to the
     * window station or desktop.
     */

    /*
     * Estimate the maximum size needed for the window station name
     * and desktop name to be given to ObjectAttributes->ObjectName.
     */
    StrSize = 0;

    /* Window station name */
    MemSize = _scwprintf(L"Service-0x%x-%x$", MAXULONG, MAXULONG) * sizeof(WCHAR);
    MemSize = gustrWindowStationsDir.Length + sizeof(OBJ_NAME_PATH_SEPARATOR)
              + max(WinStaName.Length, MemSize) + sizeof(UNICODE_NULL);
    if (MemSize > MAXUSHORT)
    {
        ERR("IntResolveDesktop: Window station name length is too long.\n");
        Status = STATUS_NAME_TOO_LONG;
        goto Quit;
    }
    StrSize = max(StrSize, (USHORT)MemSize);

    /* Desktop name */
    MemSize = max(DesktopName.Length + sizeof(UNICODE_NULL), sizeof(L"Default"));
    StrSize = max(StrSize, (USHORT)MemSize);

    /* Size for the OBJECT_ATTRIBUTES */
    MemSize = ALIGN_UP(sizeof(OBJECT_ATTRIBUTES), sizeof(PVOID));

    /* Add the string size */
    MemSize += ALIGN_UP(sizeof(UNICODE_STRING), sizeof(PVOID));
    MemSize += StrSize;

    /* Allocate the memory in user-mode */
    Status = ZwAllocateVirtualMemory(ZwCurrentProcess(),
                                     (PVOID*)&ObjectAttributes,
                                     0,
                                     &MemSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        ERR("ZwAllocateVirtualMemory() failed, Status 0x%08lx\n", Status);
        goto Quit;
    }

    ObjectName = (PUNICODE_STRING)((ULONG_PTR)ObjectAttributes +
                     ALIGN_UP(sizeof(OBJECT_ATTRIBUTES), sizeof(PVOID)));

    RtlInitEmptyUnicodeString(ObjectName,
                              (PWCHAR)((ULONG_PTR)ObjectName +
                                  ALIGN_UP(sizeof(UNICODE_STRING), sizeof(PVOID))),
                              StrSize);


    /* If we got an inherited window station handle, duplicate and use it */
    if (hWinSta)
    {
        ASSERT(bUseDefaultWinSta);

        /* Duplicate the handle if it belongs to another process than the current one */
        if (Process != PsGetCurrentProcess())
        {
            ASSERT(hProcess);
            Status = ZwDuplicateObject(hProcess,
                                       hWinSta,
                                       ZwCurrentProcess(),
                                       (PHANDLE)&hWinStaDup,
                                       0,
                                       0,
                                       DUPLICATE_SAME_ACCESS);
            if (!NT_SUCCESS(Status))
            {
                ERR("IntResolveDesktop: Failed to duplicate the window station handle, Status 0x%08lx\n", Status);
                /* We will use a default window station */
                hWinSta = NULL;
            }
            else
            {
                hWinSta = hWinStaDup;
            }
        }
    }

    /*
     * If we have an inherited window station, check whether
     * it is interactive and remember that for later.
     */
    if (hWinSta)
    {
        ASSERT(bUseDefaultWinSta);

        /* Reference the inherited window station */
        Status = ObReferenceObjectByHandle(hWinSta,
                                           0,
                                           ExWindowStationObjectType,
                                           KernelMode,
                                           (PVOID*)&WinStaObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to reference the inherited window station, Status 0x%08lx\n", Status);
            /* We will use a default window station */
            if (hWinStaDup)
            {
                ASSERT(hWinSta == hWinStaDup);
                ObCloseHandle(hWinStaDup, UserMode);
                hWinStaDup = NULL;
            }
            hWinSta = NULL;
        }
        else
        {
            ERR("Process LUID is: 0x%x-%x, inherited window station LUID is: 0x%x-%x\n",
                ProcessLuid.HighPart, ProcessLuid.LowPart,
                WinStaObject->luidUser.HighPart, WinStaObject->luidUser.LowPart);

            /* Check whether this window station is interactive, and remember it for later */
            bInteractive = !(WinStaObject->Flags & WSS_NOIO);

            /* Dereference the window station */
            ObDereferenceObject(WinStaObject);
        }
    }

    /* Build a valid window station name */
    Status = RtlStringCbPrintfW(ObjectName->Buffer,
                                ObjectName->MaximumLength,
                                L"%wZ\\%wZ",
                                &gustrWindowStationsDir,
                                &WinStaName);
    if (!NT_SUCCESS(Status))
    {
        ERR("Impossible to build a valid window station name, Status 0x%08lx\n", Status);
        goto Quit;
    }
    ObjectName->Length = (USHORT)(wcslen(ObjectName->Buffer) * sizeof(WCHAR));

    TRACE("Parsed initial window station: '%wZ'\n", ObjectName);

    /* Try to open the window station */
    InitializeObjectAttributes(ObjectAttributes,
                               ObjectName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    if (bInherit)
        ObjectAttributes->Attributes |= OBJ_INHERIT;

    Status = ObOpenObjectByName(ObjectAttributes,
                                ExWindowStationObjectType,
                                UserMode,
                                NULL,
                                WINSTA_ACCESS_ALL,
                                NULL,
                                (PHANDLE)&hTempWinSta);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to open the window station '%wZ', Status 0x%08lx\n", ObjectName, Status);
    }
    else
    {
        //
        // FIXME TODO: Perform a window station access check!!
        // If we fail AND bUseDefaultWinSta == FALSE we just quit.
        //

        /*
         * Check whether we are opening the (single) interactive
         * window station, and if so, perform an access check.
         */
        /* Check whether we are allowed to perform interactions */
        if (RtlEqualUnicodeString(&WinStaName, &WinSta0Name, TRUE))
        {
            LUID SystemLuid = SYSTEM_LUID;

            /* Interactive window station: check for user LUID */
            WinStaObject = InputWindowStation;

            Status = STATUS_ACCESS_DENIED;

            // TODO: Check also that we compare wrt. window station WinSta0
            // which is the only one that can be interactive on the system.
            if (((!bUseDefaultWinSta || bInherit) && RtlEqualLuid(&ProcessLuid, &SystemLuid)) ||
                 RtlEqualLuid(&ProcessLuid, &WinStaObject->luidUser))
            {
                /* We are interactive on this window station */
                bAccessAllowed = TRUE;
                Status = STATUS_SUCCESS;
            }
        }
        else
        {
            /* Non-interactive window station: we have access since we were able to open it */
            bAccessAllowed = TRUE;
            Status = STATUS_SUCCESS;
        }
    }

    /* If we failed, bail out if we were not trying to open the default window station */
    if (!NT_SUCCESS(Status) && !bUseDefaultWinSta) // if (!bAccessAllowed)
        goto Quit;

    if (/* bAccessAllowed && */ bInteractive || !bAccessAllowed)
    {
        /*
         * Close WinSta0 if the inherited window station is interactive so that
         * we can use it, or we do not have access to the interactive WinSta0.
         */
        ObCloseHandle(hTempWinSta, UserMode);
        hTempWinSta = NULL;
    }
    if (bInteractive == bAccessAllowed)
    {
        /* Keep using the inherited window station */
        NOTHING;
    }
    else // if (bInteractive != bAccessAllowed)
    {
        /*
         * Close the inherited window station, we will either keep using
         * the interactive WinSta0, or use Service-0xXXXX-YYYY$.
         */
        if (hWinStaDup)
        {
            ASSERT(hWinSta == hWinStaDup);
            ObCloseHandle(hWinStaDup, UserMode);
            hWinStaDup = NULL;
        }
        hWinSta = hTempWinSta; // hTempWinSta is NULL in case bAccessAllowed == FALSE
    }

    if (bUseDefaultWinSta)
    {
        if (hWinSta == NULL && !bInteractive)
        {
            /* Build a valid window station name from the LUID */
            Status = RtlStringCbPrintfW(ObjectName->Buffer,
                                        ObjectName->MaximumLength,
                                        L"%wZ\\Service-0x%x-%x$",
                                        &gustrWindowStationsDir,
                                        ProcessLuid.HighPart,
                                        ProcessLuid.LowPart);
            if (!NT_SUCCESS(Status))
            {
                ERR("Impossible to build a valid window station name, Status 0x%08lx\n", Status);
                goto Quit;
            }
            ObjectName->Length = (USHORT)(wcslen(ObjectName->Buffer) * sizeof(WCHAR));

            /*
             * Create or open the non-interactive window station.
             * NOTE: The non-interactive window station handle is never inheritable.
             */
            // FIXME: Set security!
            InitializeObjectAttributes(ObjectAttributes,
                                       ObjectName,
                                       OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                       NULL,
                                       NULL);

            Status = IntCreateWindowStation(&hWinSta,
                                            ObjectAttributes,
                                            UserMode,
                                            KernelMode,
                                            MAXIMUM_ALLOWED,
                                            0, 0, 0, 0, 0);
            if (!NT_SUCCESS(Status))
            {
                ASSERT(hWinSta == NULL);
                ERR("Failed to create or open the non-interactive window station '%wZ', Status 0x%08lx\n",
                    ObjectName, Status);
                goto Quit;
            }

            //
            // FIXME: We might not need to always create or open the "Default"
            // desktop on the Service-0xXXXX-YYYY$ window station; we may need
            // to use another one....
            //

            /* Create or open the Default desktop on the window station */
            Status = RtlStringCbCopyW(ObjectName->Buffer,
                                      ObjectName->MaximumLength,
                                      L"Default");
            if (!NT_SUCCESS(Status))
            {
                ERR("Impossible to build a valid desktop name, Status 0x%08lx\n", Status);
                goto Quit;
            }
            ObjectName->Length = (USHORT)(wcslen(ObjectName->Buffer) * sizeof(WCHAR));

            /* NOTE: The non-interactive desktop handle is never inheritable. */
            // FIXME: Set security!
            InitializeObjectAttributes(ObjectAttributes,
                                       ObjectName,
                                       OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                       hWinSta,
                                       NULL);

            Status = IntCreateDesktop(&hDesktop,
                                      ObjectAttributes,
                                      UserMode,
                                      NULL,
                                      NULL,
                                      0,
                                      MAXIMUM_ALLOWED);
            if (!NT_SUCCESS(Status))
            {
                ASSERT(hDesktop == NULL);
                ERR("Failed to create or open the desktop '%wZ' on window station 0x%p, Status 0x%08lx\n",
                    ObjectName, hWinSta, Status);
            }

            goto Quit;
        }
/*
        if (hWinSta == NULL)
        {
            Status = STATUS_UNSUCCESSFUL;
            goto Quit;
        }
*/
    }

    /*
     * If we got an inherited desktop handle, duplicate and use it,
     * otherwise open a new desktop.
     */
    if (hDesktop != NULL)
    {
        /* Duplicate the handle if it belongs to another process than the current one */
        if (Process != PsGetCurrentProcess())
        {
            ASSERT(hProcess);
            Status = ZwDuplicateObject(hProcess,
                                       hDesktop,
                                       ZwCurrentProcess(),
                                       (PHANDLE)&hDesktopDup,
                                       0,
                                       0,
                                       DUPLICATE_SAME_ACCESS);
            if (!NT_SUCCESS(Status))
            {
                ERR("IntResolveDesktop: Failed to duplicate the desktop handle, Status 0x%08lx\n", Status);
                /* We will use a default desktop */
                hDesktop = NULL;
            }
            else
            {
                hDesktop = hDesktopDup;
            }
        }
    }

    if ((hWinSta != NULL) && (hDesktop == NULL))
    {
        Status = RtlStringCbCopyNW(ObjectName->Buffer,
                                   ObjectName->MaximumLength,
                                   DesktopName.Buffer,
                                   DesktopName.Length);
        if (!NT_SUCCESS(Status))
        {
            ERR("Impossible to build a valid desktop name, Status 0x%08lx\n", Status);
            goto Quit;
        }
        ObjectName->Length = (USHORT)(wcslen(ObjectName->Buffer) * sizeof(WCHAR));

        TRACE("Parsed initial desktop: '%wZ'\n", ObjectName);

        /* Open the desktop object */
        InitializeObjectAttributes(ObjectAttributes,
                                   ObjectName,
                                   OBJ_CASE_INSENSITIVE,
                                   hWinSta,
                                   NULL);
        if (bInherit)
            ObjectAttributes->Attributes |= OBJ_INHERIT;

        Status = ObOpenObjectByName(ObjectAttributes,
                                    ExDesktopObjectType,
                                    UserMode,
                                    NULL,
                                    DESKTOP_ALL_ACCESS,
                                    NULL,
                                    (PHANDLE)&hDesktop);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to open the desktop '%wZ' on window station 0x%p, Status 0x%08lx\n",
                ObjectName, hWinSta, Status);
            goto Quit;
        }
    }

Quit:
    /* Release the object attributes */
    if (ObjectAttributes)
    {
        MemSize = 0;
        ZwFreeVirtualMemory(ZwCurrentProcess(),
                            (PVOID*)&ObjectAttributes,
                            &MemSize,
                            MEM_RELEASE);
    }

    /* Close the temporary process handle */
    if (hProcess) // if (Process != PsGetCurrentProcess())
        ObCloseHandle(hProcess, KernelMode);

    if (NT_SUCCESS(Status))
    {
        *phWinSta  = hWinSta;
        *phDesktop = hDesktop;
        return STATUS_SUCCESS;
    }
    else
    {
        ERR("IntResolveDesktop(%wZ) failed, Status 0x%08lx\n", DesktopPath, Status);

        if (hDesktopDup)
            ObCloseHandle(hDesktopDup, UserMode);
        if (hWinStaDup)
            ObCloseHandle(hWinStaDup, UserMode);

        if (hDesktop)
            ObCloseHandle(hDesktop, UserMode);
        if (hWinSta)
            ObCloseHandle(hWinSta, UserMode);

        SetLastNtError(Status);
        return Status;
    }
}

/*
 * IntValidateDesktopHandle
 *
 * Validates the desktop handle.
 *
 * Remarks
 *    If the function succeeds, the handle remains referenced. If the
 *    fucntion fails, last error is set.
 */

NTSTATUS FASTCALL
IntValidateDesktopHandle(
    HDESK Desktop,
    KPROCESSOR_MODE AccessMode,
    ACCESS_MASK DesiredAccess,
    PDESKTOP *Object)
{
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(Desktop,
                                       DesiredAccess,
                                       ExDesktopObjectType,
                                       AccessMode,
                                       (PVOID*)Object,
                                       NULL);

    TRACE("IntValidateDesktopHandle: handle:0x%p obj:0x%p access:0x%x Status:0x%lx\n",
          Desktop, *Object, DesiredAccess, Status);

    if (!NT_SUCCESS(Status))
        SetLastNtError(Status);

    return Status;
}

PDESKTOP FASTCALL
IntGetActiveDesktop(VOID)
{
    return gpdeskInputDesktop;
}

/*
 * Returns or creates a handle to the desktop object
 */
HDESK FASTCALL
IntGetDesktopObjectHandle(PDESKTOP DesktopObject)
{
    NTSTATUS Status;
    HDESK hDesk;

    ASSERT(DesktopObject);

    if (!ObFindHandleForObject(PsGetCurrentProcess(),
                               DesktopObject,
                               ExDesktopObjectType,
                               NULL,
                               (PHANDLE)&hDesk))
    {
        Status = ObOpenObjectByPointer(DesktopObject,
                                       0,
                                       NULL,
                                       0,
                                       ExDesktopObjectType,
                                       UserMode,
                                       (PHANDLE)&hDesk);
        if (!NT_SUCCESS(Status))
        {
            /* Unable to create a handle */
            ERR("Unable to create a desktop handle\n");
            return NULL;
        }
    }
    else
    {
        TRACE("Got handle: 0x%p\n", hDesk);
    }

    return hDesk;
}

PUSER_MESSAGE_QUEUE FASTCALL
IntGetFocusMessageQueue(VOID)
{
    PDESKTOP pdo = IntGetActiveDesktop();
    if (!pdo)
    {
        TRACE("No active desktop\n");
        return(NULL);
    }
    return (PUSER_MESSAGE_QUEUE)pdo->ActiveMessageQueue;
}

VOID FASTCALL
IntSetFocusMessageQueue(PUSER_MESSAGE_QUEUE NewQueue)
{
    PUSER_MESSAGE_QUEUE Old;
    PDESKTOP pdo = IntGetActiveDesktop();
    if (!pdo)
    {
        TRACE("No active desktop\n");
        return;
    }
    if (NewQueue != NULL)
    {
        if (NewQueue->Desktop != NULL)
        {
            TRACE("Message Queue already attached to another desktop!\n");
            return;
        }
        IntReferenceMessageQueue(NewQueue);
        (void)InterlockedExchangePointer((PVOID*)&NewQueue->Desktop, pdo);
    }
    Old = (PUSER_MESSAGE_QUEUE)InterlockedExchangePointer((PVOID*)&pdo->ActiveMessageQueue, NewQueue);
    if (Old != NULL)
    {
        (void)InterlockedExchangePointer((PVOID*)&Old->Desktop, 0);
        gpqForegroundPrev = Old;
        IntDereferenceMessageQueue(Old);
    }
    // Only one Q can have active foreground even when there are more than one desktop.
    if (NewQueue)
    {
        gpqForeground = pdo->ActiveMessageQueue;
    }
    else
    {
        gpqForeground = NULL;
        ERR("ptiLastInput is CLEARED!!\n");
        ptiLastInput = NULL; // ReactOS hacks... should check for process death.
    }
}

PWND FASTCALL
IntGetThreadDesktopWindow(PTHREADINFO pti)
{
    if (!pti) pti = PsGetCurrentThreadWin32Thread();
    if (pti->pDeskInfo) return pti->pDeskInfo->spwnd;
    return NULL;
}

PWND FASTCALL co_GetDesktopWindow(PWND pWnd)
{
    if (pWnd->head.rpdesk &&
        pWnd->head.rpdesk->pDeskInfo)
        return pWnd->head.rpdesk->pDeskInfo->spwnd;
    return NULL;
}

HWND FASTCALL IntGetDesktopWindow(VOID)
{
    PDESKTOP pdo = IntGetActiveDesktop();
    if (!pdo)
    {
        TRACE("No active desktop\n");
        return NULL;
    }
    return pdo->DesktopWindow;
}

// Win: _GetDesktopWindow
PWND FASTCALL UserGetDesktopWindow(VOID)
{
    PDESKTOP pdo = IntGetActiveDesktop();

    if (!pdo)
    {
        TRACE("No active desktop\n");
        return NULL;
    }
    // return pdo->pDeskInfo->spwnd;
    return UserGetWindowObject(pdo->DesktopWindow);
}

HWND FASTCALL IntGetMessageWindow(VOID)
{
    PDESKTOP pdo = IntGetActiveDesktop();

    if (!pdo)
    {
        TRACE("No active desktop\n");
        return NULL;
    }
    return pdo->spwndMessage->head.h;
}

// Win: _GetMessageWindow
PWND FASTCALL UserGetMessageWindow(VOID)
{
    PDESKTOP pdo = IntGetActiveDesktop();

    if (!pdo)
    {
        TRACE("No active desktop\n");
        return NULL;
    }
    return pdo->spwndMessage;
}

HWND FASTCALL IntGetCurrentThreadDesktopWindow(VOID)
{
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
    PDESKTOP pdo = pti->rpdesk;
    if (NULL == pdo)
    {
        ERR("Thread doesn't have a desktop\n");
        return NULL;
    }
    return pdo->DesktopWindow;
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOL FASTCALL
DesktopWindowProc(PWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
    PAINTSTRUCT Ps;
    ULONG Value;
    //ERR("DesktopWindowProc\n");

    *lResult = 0;

    switch (Msg)
    {
        case WM_NCCREATE:
            if (!Wnd->fnid)
            {
                Wnd->fnid = FNID_DESKTOP;
            }
            *lResult = (LRESULT)TRUE;
            return TRUE;

        case WM_CREATE:
            Value = HandleToULong(PsGetCurrentProcessId());
            // Save Process ID
            co_UserSetWindowLong(UserHMGetHandle(Wnd), DT_GWL_PROCESSID, Value, FALSE);
            Value = HandleToULong(PsGetCurrentThreadId());
            // Save Thread ID
            co_UserSetWindowLong(UserHMGetHandle(Wnd), DT_GWL_THREADID, Value, FALSE);
        case WM_CLOSE:
            return TRUE;

        case WM_DISPLAYCHANGE:
            co_WinPosSetWindowPos(Wnd, 0, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOACTIVATE);
            return TRUE;

        case WM_ERASEBKGND:
            IntPaintDesktop((HDC)wParam);
            *lResult = 1;
            return TRUE;

        case WM_PAINT:
        {
            if (IntBeginPaint(Wnd, &Ps))
            {
                IntEndPaint(Wnd, &Ps);
            }
            return TRUE;
        }
        case WM_SYSCOLORCHANGE:
            co_UserRedrawWindow(Wnd, NULL, NULL, RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
            return TRUE;

        case WM_SETCURSOR:
        {
            PCURICON_OBJECT pcurOld, pcurNew;
            pcurNew = UserGetCurIconObject(gDesktopCursor);
            if (!pcurNew)
            {
                return TRUE;
            }

            pcurNew->CURSORF_flags |= CURSORF_CURRENT;
            pcurOld = UserSetCursor(pcurNew, FALSE);
            if (pcurOld)
            {
                pcurOld->CURSORF_flags &= ~CURSORF_CURRENT;
                UserDereferenceObject(pcurOld);
            }
            return TRUE;
        }

        case WM_WINDOWPOSCHANGING:
        {
            PWINDOWPOS pWindowPos = (PWINDOWPOS)lParam;
            if ((pWindowPos->flags & SWP_SHOWWINDOW) != 0)
            {
                HDESK hdesk = UserOpenInputDesktop(0, FALSE, DESKTOP_ALL_ACCESS);
                IntSetThreadDesktop(hdesk, FALSE);
            }
            break;
        }
        default:
            TRACE("DWP calling IDWP Msg %d\n",Msg);
            //*lResult = IntDefWindowProc(Wnd, Msg, wParam, lParam, FALSE);
    }
    return TRUE; /* We are done. Do not do any callbacks to user mode */
}

BOOL FASTCALL
UserMessageWindowProc(PWND pwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
    *lResult = 0;

    switch(Msg)
    {
    case WM_NCCREATE:
        pwnd->fnid |= FNID_MESSAGEWND;
        *lResult = (LRESULT)TRUE;
        break;
    case WM_DESTROY:
        pwnd->fnid |= FNID_DESTROY;
        break;
    default:
        ERR("UMWP calling IDWP\n");
        *lResult = IntDefWindowProc(pwnd, Msg, wParam, lParam, FALSE);
    }

    return TRUE; /* We are done. Do not do any callbacks to user mode */
}

VOID NTAPI DesktopThreadMain(VOID)
{
    BOOL Ret;
    MSG Msg;

    gptiDesktopThread = PsGetCurrentThreadWin32Thread();

    UserEnterExclusive();

    /* Register system classes. This thread does not belong to any desktop so the
       classes will be allocated from the shared heap */
    UserRegisterSystemClasses();

    KeSetEvent(gpDesktopThreadStartedEvent, IO_NO_INCREMENT, FALSE);

    while (TRUE)
    {
        Ret = co_IntGetPeekMessage(&Msg, 0, 0, 0, PM_REMOVE, TRUE);
        if (Ret)
        {
            IntDispatchMessage(&Msg);
        }
    }

    UserLeave();
}

HDC FASTCALL
UserGetDesktopDC(ULONG DcType, BOOL bAltDc, BOOL ValidatehWnd)
{
    PWND DesktopObject = 0;
    HDC DesktopHDC = 0;

    /* This can be called from GDI/DX, so acquire the USER lock */
    UserEnterExclusive();

    if (DcType == DCTYPE_DIRECT)
    {
        DesktopObject = UserGetDesktopWindow();
        DesktopHDC = (HDC)UserGetWindowDC(DesktopObject);
    }
    else
    {
        PMONITOR pMonitor = UserGetPrimaryMonitor();
        DesktopHDC = IntGdiCreateDisplayDC(pMonitor->hDev, DcType, bAltDc);
    }

    UserLeave();

    return DesktopHDC;
}

VOID APIENTRY
UserRedrawDesktop(VOID)
{
    PWND Window = NULL;
    PREGION Rgn;

    Window = UserGetDesktopWindow();
    Rgn = IntSysCreateRectpRgnIndirect(&Window->rcWindow);

    IntInvalidateWindows(Window,
                         Rgn,
                         RDW_FRAME | RDW_ERASE |
                         RDW_INVALIDATE | RDW_ALLCHILDREN);

    REGION_Delete(Rgn);
}


NTSTATUS FASTCALL
co_IntShowDesktop(PDESKTOP Desktop, ULONG Width, ULONG Height, BOOL bRedraw)
{
    PWND pwnd = Desktop->pDeskInfo->spwnd;
    UINT flags = SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW;
    ASSERT(pwnd);

    if (!bRedraw)
        flags |= SWP_NOREDRAW;

    co_WinPosSetWindowPos(pwnd, NULL, 0, 0, Width, Height, flags);

    if (bRedraw)
        co_UserRedrawWindow( pwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_INVALIDATE );

    return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
IntHideDesktop(PDESKTOP Desktop)
{
    PWND DesktopWnd;

    DesktopWnd = IntGetWindowObject(Desktop->DesktopWindow);
    if (! DesktopWnd)
    {
        return ERROR_INVALID_WINDOW_HANDLE;
    }
    DesktopWnd->style &= ~WS_VISIBLE;

    return STATUS_SUCCESS;
}

static
HWND* FASTCALL
UserBuildShellHookHwndList(PDESKTOP Desktop)
{
    ULONG entries=0;
    PLIST_ENTRY ListEntry;
    PSHELL_HOOK_WINDOW Current;
    HWND* list;

    /* FIXME: If we save nb elements in desktop, we don't have to loop to find nb entries */
    ListEntry = Desktop->ShellHookWindows.Flink;
    while (ListEntry != &Desktop->ShellHookWindows)
    {
        ListEntry = ListEntry->Flink;
        entries++;
    }

    if (!entries) return NULL;

    list = ExAllocatePoolWithTag(PagedPool, sizeof(HWND) * (entries + 1), USERTAG_WINDOWLIST); /* alloc one extra for nullterm */
    if (list)
    {
        HWND* cursor = list;

        ListEntry = Desktop->ShellHookWindows.Flink;
        while (ListEntry != &Desktop->ShellHookWindows)
        {
            Current = CONTAINING_RECORD(ListEntry, SHELL_HOOK_WINDOW, ListEntry);
            ListEntry = ListEntry->Flink;
            *cursor++ = Current->hWnd;
        }

        *cursor = NULL; /* Nullterm list */
    }

    return list;
}

/*
 * Send the Message to the windows registered for ShellHook
 * notifications. The lParam contents depend on the Message. See
 * MSDN for more details (RegisterShellHookWindow)
 */
VOID co_IntShellHookNotify(WPARAM Message, WPARAM wParam, LPARAM lParam)
{
    PDESKTOP Desktop = IntGetActiveDesktop();
    HWND* HwndList;

    if (!gpsi->uiShellMsg)
    {
        gpsi->uiShellMsg = IntAddAtom(L"SHELLHOOK");

        TRACE("MsgType = %x\n", gpsi->uiShellMsg);
        if (!gpsi->uiShellMsg)
            ERR("LastError: %x\n", EngGetLastError());
    }

    if (!Desktop)
    {
        TRACE("IntShellHookNotify: No desktop!\n");
        return;
    }

    // Allow other devices have a shot at foreground.
    if (Message == HSHELL_APPCOMMAND) ptiLastInput = NULL;

    // FIXME: System Tray Support.

    HwndList = UserBuildShellHookHwndList(Desktop);
    if (HwndList)
    {
        HWND* cursor = HwndList;

        for (; *cursor; cursor++)
        {
            TRACE("Sending notify\n");
            UserPostMessage(*cursor,
                            gpsi->uiShellMsg,
                            Message,
                            (Message == HSHELL_LANGUAGE ? lParam : (LPARAM)wParam) );
/*            co_IntPostOrSendMessage(*cursor,
                                    gpsi->uiShellMsg,
                                    Message,
                                    (Message == HSHELL_LANGUAGE ? lParam : (LPARAM)wParam) );*/
        }

        ExFreePoolWithTag(HwndList, USERTAG_WINDOWLIST);
    }

    if (ISITHOOKED(WH_SHELL))
    {
        co_HOOK_CallHooks(WH_SHELL, Message, wParam, lParam);
    }
}

/*
 * Add the window to the ShellHookWindows list. The windows
 * on that list get notifications that are important to shell
 * type applications.
 *
 * TODO: Validate the window? I'm not sure if sending these messages to
 * an unsuspecting application that is not your own is a nice thing to do.
 */
BOOL IntRegisterShellHookWindow(HWND hWnd)
{
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
    PDESKTOP Desktop = pti->rpdesk;
    PSHELL_HOOK_WINDOW Entry;

    TRACE("IntRegisterShellHookWindow\n");

    /* First deregister the window, so we can be sure it's never twice in the
     * list.
     */
    IntDeRegisterShellHookWindow(hWnd);

    Entry = ExAllocatePoolWithTag(PagedPool,
                                  sizeof(SHELL_HOOK_WINDOW),
                                  TAG_WINSTA);

    if (!Entry)
        return FALSE;

    Entry->hWnd = hWnd;

    InsertTailList(&Desktop->ShellHookWindows, &Entry->ListEntry);

    return TRUE;
}

/*
 * Remove the window from the ShellHookWindows list. The windows
 * on that list get notifications that are important to shell
 * type applications.
 */
BOOL IntDeRegisterShellHookWindow(HWND hWnd)
{
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
    PDESKTOP Desktop = pti->rpdesk;
    PLIST_ENTRY ListEntry;
    PSHELL_HOOK_WINDOW Current;

    ListEntry = Desktop->ShellHookWindows.Flink;
    while (ListEntry != &Desktop->ShellHookWindows)
    {
        Current = CONTAINING_RECORD(ListEntry, SHELL_HOOK_WINDOW, ListEntry);
        ListEntry = ListEntry->Flink;
        if (Current->hWnd == hWnd)
        {
            RemoveEntryList(&Current->ListEntry);
            ExFreePoolWithTag(Current, TAG_WINSTA);
            return TRUE;
        }
    }

    return FALSE;
}

static VOID
IntFreeDesktopHeap(IN OUT PDESKTOP Desktop)
{
    /* FIXME: Disable until unmapping works in mm */
#if 0
    if (Desktop->pheapDesktop != NULL)
    {
        MmUnmapViewInSessionSpace(Desktop->pheapDesktop);
        Desktop->pheapDesktop = NULL;
    }

    if (Desktop->hsectionDesktop != NULL)
    {
        ObDereferenceObject(Desktop->hsectionDesktop);
        Desktop->hsectionDesktop = NULL;
    }
#endif
}

BOOL FASTCALL
IntPaintDesktop(HDC hDC)
{
    static WCHAR s_wszSafeMode[] = L"Safe Mode"; // FIXME: Localize!

    RECTL Rect;
    HBRUSH DesktopBrush, PreviousBrush;
    HWND hWndDesktop;
    BOOL doPatBlt = TRUE;
    PWND WndDesktop;
    BOOLEAN InSafeMode;

    if (GdiGetClipBox(hDC, &Rect) == ERROR)
        return FALSE;

    hWndDesktop = IntGetDesktopWindow(); // rpdesk->DesktopWindow;

    WndDesktop = UserGetWindowObject(hWndDesktop); // rpdesk->pDeskInfo->spwnd;
    if (!WndDesktop)
        return FALSE;

    /* Retrieve the current SafeMode state */
    InSafeMode = (UserGetSystemMetrics(SM_CLEANBOOT) != 0); // gpsi->aiSysMet[SM_CLEANBOOT];

    if (!InSafeMode)
    {
        DesktopBrush = (HBRUSH)WndDesktop->pcls->hbrBackground;

        /*
         * Paint desktop background
         */
        if (gspv.hbmWallpaper != NULL)
        {
            SIZE sz;
            int x, y;
            int scaledWidth, scaledHeight;
            int wallpaperX, wallpaperY, wallpaperWidth, wallpaperHeight;
            HDC hWallpaperDC;

            sz.cx = WndDesktop->rcWindow.right - WndDesktop->rcWindow.left;
            sz.cy = WndDesktop->rcWindow.bottom - WndDesktop->rcWindow.top;

            if (gspv.WallpaperMode == wmFit ||
                gspv.WallpaperMode == wmFill)
            {
                int scaleNum, scaleDen;

                // Precision improvement over ((sz.cx / gspv.cxWallpaper) > (sz.cy / gspv.cyWallpaper))
                if ((sz.cx * gspv.cyWallpaper) > (sz.cy * gspv.cxWallpaper))
                {
                    if (gspv.WallpaperMode == wmFit)
                    {
                        scaleNum = sz.cy;
                        scaleDen = gspv.cyWallpaper;
                    }
                    else
                    {
                        scaleNum = sz.cx;
                        scaleDen = gspv.cxWallpaper;
                    }
                }
                else
                {
                    if (gspv.WallpaperMode == wmFit)
                    {
                        scaleNum = sz.cx;
                        scaleDen = gspv.cxWallpaper;
                    }
                    else
                    {
                        scaleNum = sz.cy;
                        scaleDen = gspv.cyWallpaper;
                    }
                }

                scaledWidth = EngMulDiv(gspv.cxWallpaper, scaleNum, scaleDen);
                scaledHeight = EngMulDiv(gspv.cyWallpaper, scaleNum, scaleDen);

                if (gspv.WallpaperMode == wmFill)
                {
                    wallpaperX = (((scaledWidth - sz.cx) * gspv.cxWallpaper) / (2 * scaledWidth));
                    wallpaperY = (((scaledHeight - sz.cy) * gspv.cyWallpaper) / (2 * scaledHeight));

                    wallpaperWidth = (sz.cx * gspv.cxWallpaper) / scaledWidth;
                    wallpaperHeight = (sz.cy * gspv.cyWallpaper) / scaledHeight;
                }
            }

            if (gspv.WallpaperMode == wmStretch ||
                gspv.WallpaperMode == wmTile ||
                gspv.WallpaperMode == wmFill)
            {
                x = 0;
                y = 0;
            }
            else if (gspv.WallpaperMode == wmFit)
            {
                x = (sz.cx - scaledWidth) / 2;
                y = (sz.cy - scaledHeight) / 2;
            }
            else
            {
                /* Find the upper left corner, can be negative if the bitmap is bigger than the screen */
                x = (sz.cx / 2) - (gspv.cxWallpaper / 2);
                y = (sz.cy / 2) - (gspv.cyWallpaper / 2);
            }

            hWallpaperDC = NtGdiCreateCompatibleDC(hDC);
            if (hWallpaperDC != NULL)
            {
                HBITMAP hOldBitmap;

                /* Fill in the area that the bitmap is not going to cover */
                if (x > 0 || y > 0)
                {
                    /* FIXME: Clip out the bitmap
                       can be replaced with "NtGdiPatBlt(hDC, x, y, gspv.cxWallpaper, gspv.cyWallpaper, PATCOPY | DSTINVERT);"
                       once we support DSTINVERT */
                    PreviousBrush = NtGdiSelectBrush(hDC, DesktopBrush);
                    NtGdiPatBlt(hDC, Rect.left, Rect.top, Rect.right, Rect.bottom, PATCOPY);
                    NtGdiSelectBrush(hDC, PreviousBrush);
                }

                /* Do not fill the background after it is painted no matter the size of the picture */
                doPatBlt = FALSE;

                hOldBitmap = NtGdiSelectBitmap(hWallpaperDC, gspv.hbmWallpaper);

                if (gspv.WallpaperMode == wmStretch)
                {
                    if (Rect.right && Rect.bottom)
                        NtGdiStretchBlt(hDC,
                                        x,
                                        y,
                                        sz.cx,
                                        sz.cy,
                                        hWallpaperDC,
                                        0,
                                        0,
                                        gspv.cxWallpaper,
                                        gspv.cyWallpaper,
                                        SRCCOPY,
                                        0);
                }
                else if (gspv.WallpaperMode == wmTile)
                {
                    /* Paint the bitmap across the screen then down */
                    for (y = 0; y < Rect.bottom; y += gspv.cyWallpaper)
                    {
                        for (x = 0; x < Rect.right; x += gspv.cxWallpaper)
                        {
                            NtGdiBitBlt(hDC,
                                        x,
                                        y,
                                        gspv.cxWallpaper,
                                        gspv.cyWallpaper,
                                        hWallpaperDC,
                                        0,
                                        0,
                                        SRCCOPY,
                                        0,
                                        0);
                        }
                    }
                }
                else if (gspv.WallpaperMode == wmFit)
                {
                    if (Rect.right && Rect.bottom)
                    {
                        NtGdiStretchBlt(hDC,
                                        x,
                                        y,
                                        scaledWidth,
                                        scaledHeight,
                                        hWallpaperDC,
                                        0,
                                        0,
                                        gspv.cxWallpaper,
                                        gspv.cyWallpaper,
                                        SRCCOPY,
                                        0);
                    }
                }
                else if (gspv.WallpaperMode == wmFill)
                {
                    if (Rect.right && Rect.bottom)
                    {
                        NtGdiStretchBlt(hDC,
                                        x,
                                        y,
                                        sz.cx,
                                        sz.cy,
                                        hWallpaperDC,
                                        wallpaperX,
                                        wallpaperY,
                                        wallpaperWidth,
                                        wallpaperHeight,
                                        SRCCOPY,
                                        0);
                    }
                }
                else
                {
                    NtGdiBitBlt(hDC,
                                x,
                                y,
                                gspv.cxWallpaper,
                                gspv.cyWallpaper,
                                hWallpaperDC,
                                0,
                                0,
                                SRCCOPY,
                                0,
                                0);
                }
                NtGdiSelectBitmap(hWallpaperDC, hOldBitmap);
                NtGdiDeleteObjectApp(hWallpaperDC);
            }
        }
    }
    else
    {
        /* Black desktop background in Safe Mode */
        DesktopBrush = StockObjects[BLACK_BRUSH];
    }

    /* Background is set to none, clear the screen */
    if (doPatBlt)
    {
        PreviousBrush = NtGdiSelectBrush(hDC, DesktopBrush);
        NtGdiPatBlt(hDC, Rect.left, Rect.top, Rect.right, Rect.bottom, PATCOPY);
        NtGdiSelectBrush(hDC, PreviousBrush);
    }

    /*
     * Display the system version on the desktop background
     */
    if (InSafeMode || g_AlwaysDisplayVersion || g_PaintDesktopVersion)
    {
        NTSTATUS Status;
        static WCHAR wszzVersion[1024] = L"\0";

        /* Only used in normal mode */
        // We expect at most 4 strings (3 for version, 1 for optional NtSystemRoot)
        static POLYTEXTW VerStrs[4] = {{0},{0},{0},{0}};
        INT i = 0;
        SIZE_T len;

        HFONT hFont1 = NULL, hFont2 = NULL, hOldFont = NULL;
        COLORREF crText, color_old;
        UINT align_old;
        INT mode_old;
        PDC pdc;

        if (!UserSystemParametersInfo(SPI_GETWORKAREA, 0, &Rect, 0))
        {
            Rect.left = Rect.top = 0;
            Rect.right  = UserGetSystemMetrics(SM_CXSCREEN);
            Rect.bottom = UserGetSystemMetrics(SM_CYSCREEN);
        }
        else
        {
            RECTL_vOffsetRect(&Rect, -Rect.left, -Rect.top);
        }

        /*
         * Set up the fonts (otherwise use default ones)
         */

        /* Font for the principal version string */
        hFont1 = GreCreateFontIndirectW(&gspv.ncm.lfCaptionFont);
        /* Font for the secondary version strings */
        hFont2 = GreCreateFontIndirectW(&gspv.ncm.lfMenuFont);

        if (hFont1)
            hOldFont = NtGdiSelectFont(hDC, hFont1);

        if (gspv.hbmWallpaper == NULL)
        {
            /* Retrieve the brush fill colour */
            // TODO: The following code constitutes "GreGetBrushColor".
            PreviousBrush = NtGdiSelectBrush(hDC, DesktopBrush);
            pdc = DC_LockDc(hDC);
            if (pdc)
            {
                crText = pdc->eboFill.ulRGBColor;
                DC_UnlockDc(pdc);
            }
            else
            {
                crText = RGB(0, 0, 0);
            }
            NtGdiSelectBrush(hDC, PreviousBrush);

            /* Adjust text colour according to the brush */
            if (GetRValue(crText) + GetGValue(crText) + GetBValue(crText) > 128 * 3)
                crText = RGB(0, 0, 0);
            else
                crText = RGB(255, 255, 255);
        }
        else
        {
            /* Always use white when the text is displayed on top of a wallpaper */
            crText = RGB(255, 255, 255);
        }

        color_old = IntGdiSetTextColor(hDC, crText);
        align_old = IntGdiSetTextAlign(hDC, TA_RIGHT);
        mode_old  = IntGdiSetBkMode(hDC, TRANSPARENT);

        /* Display the system version information */
        if (!*wszzVersion)
        {
            Status = GetSystemVersionString(wszzVersion,
                                            ARRAYSIZE(wszzVersion),
                                            InSafeMode,
                                            g_AlwaysDisplayVersion);
            if (!InSafeMode && NT_SUCCESS(Status) && *wszzVersion)
            {
                PWCHAR pstr = wszzVersion;
                for (i = 0; (i < ARRAYSIZE(VerStrs)) && *pstr; ++i)
                {
                    VerStrs[i].n = lstrlenW(pstr);
                    VerStrs[i].lpstr = pstr;
                    pstr += (VerStrs[i].n + 1);
                }
            }
        }
        else
        {
            Status = STATUS_SUCCESS;
        }
        if (NT_SUCCESS(Status) && *wszzVersion)
        {
            if (!InSafeMode)
            {
                SIZE Size = {0, 0};
                LONG TotalHeight = 0;

                /* Normal Mode: multiple version information text separated by newlines */
                IntGdiSetTextAlign(hDC, TA_RIGHT | TA_BOTTOM);

                /* Compute the heights of the strings */
                if (hFont1) NtGdiSelectFont(hDC, hFont1);
                for (i = 0; i < ARRAYSIZE(VerStrs); ++i)
                {
                    if (!VerStrs[i].lpstr || !*VerStrs[i].lpstr || (VerStrs[i].n == 0))
                        break;

                    GreGetTextExtentW(hDC, VerStrs[i].lpstr, VerStrs[i].n, &Size, 1);
                    VerStrs[i].y = Size.cy; // Store the string height
                    TotalHeight += Size.cy;

                    /* While the first string was using hFont1, all the others use hFont2 */
                    if (hFont2) NtGdiSelectFont(hDC, hFont2);
                }
                /* The total height must not exceed the screen height */
                TotalHeight = min(TotalHeight, Rect.bottom);

                /* Display the strings */
                if (hFont1) NtGdiSelectFont(hDC, hFont1);
                for (i = 0; i < ARRAYSIZE(VerStrs); ++i)
                {
                    if (!VerStrs[i].lpstr || !*VerStrs[i].lpstr || (VerStrs[i].n == 0))
                        break;

                    TotalHeight -= VerStrs[i].y;
                    GreExtTextOutW(hDC,
                                   Rect.right - 5,
                                   Rect.bottom - TotalHeight - 5,
                                   0, NULL,
                                   VerStrs[i].lpstr,
                                   VerStrs[i].n,
                                   NULL, 0);

                    /* While the first string was using hFont1, all the others use hFont2 */
                    if (hFont2) NtGdiSelectFont(hDC, hFont2);
                }
            }
            else
            {
                if (hFont1) NtGdiSelectFont(hDC, hFont1);

                /* Safe Mode: single version information text in top center */
                len = wcslen(wszzVersion);

                IntGdiSetTextAlign(hDC, TA_CENTER | TA_TOP);
                GreExtTextOutW(hDC, (Rect.right + Rect.left)/2, Rect.top + 3, 0, NULL, wszzVersion, len, NULL, 0);
            }
        }

        if (InSafeMode)
        {
            if (hFont1) NtGdiSelectFont(hDC, hFont1);

            /* Print Safe Mode text in corners */
            len = wcslen(s_wszSafeMode);

            IntGdiSetTextAlign(hDC, TA_LEFT | TA_TOP);
            GreExtTextOutW(hDC, Rect.left, Rect.top + 3, 0, NULL, s_wszSafeMode, len, NULL, 0);
            IntGdiSetTextAlign(hDC, TA_RIGHT | TA_TOP);
            GreExtTextOutW(hDC, Rect.right, Rect.top + 3, 0, NULL, s_wszSafeMode, len, NULL, 0);
            IntGdiSetTextAlign(hDC, TA_LEFT | TA_BOTTOM);
            GreExtTextOutW(hDC, Rect.left, Rect.bottom - 5, 0, NULL, s_wszSafeMode, len, NULL, 0);
            IntGdiSetTextAlign(hDC, TA_RIGHT | TA_BOTTOM);
            GreExtTextOutW(hDC, Rect.right, Rect.bottom - 5, 0, NULL, s_wszSafeMode, len, NULL, 0);
        }

        IntGdiSetBkMode(hDC, mode_old);
        IntGdiSetTextAlign(hDC, align_old);
        IntGdiSetTextColor(hDC, color_old);

        if (hFont2)
            GreDeleteObject(hFont2);

        if (hFont1)
        {
            NtGdiSelectFont(hDC, hOldFont);
            GreDeleteObject(hFont1);
        }
    }

    return TRUE;
}

static NTSTATUS
UserInitializeDesktop(PDESKTOP pdesk, PUNICODE_STRING DesktopName, PWINSTATION_OBJECT pwinsta)
{
    PVOID DesktopHeapSystemBase = NULL;
    ULONG_PTR HeapSize = gdwDesktopSectionSize * 1024;
    SIZE_T DesktopInfoSize;
    ULONG i;

    TRACE("UserInitializeDesktop desktop 0x%p with name %wZ\n", pdesk, DesktopName);

    RtlZeroMemory(pdesk, sizeof(DESKTOP));

    /* Link the desktop with the parent window station */
    ObReferenceObject(pwinsta);
    pdesk->rpwinstaParent = pwinsta;
    InsertTailList(&pwinsta->DesktopListHead, &pdesk->ListEntry);

    /* Create the desktop heap */
    pdesk->hsectionDesktop = NULL;
    pdesk->pheapDesktop = UserCreateHeap(&pdesk->hsectionDesktop,
                                         &DesktopHeapSystemBase,
                                         HeapSize);
    if (pdesk->pheapDesktop == NULL)
    {
        ERR("Failed to create desktop heap!\n");
        return STATUS_NO_MEMORY;
    }

    /* Create DESKTOPINFO */
    DesktopInfoSize = sizeof(DESKTOPINFO) + DesktopName->Length + sizeof(WCHAR);
    pdesk->pDeskInfo = RtlAllocateHeap(pdesk->pheapDesktop,
                                       HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY,
                                       DesktopInfoSize);
    if (pdesk->pDeskInfo == NULL)
    {
        ERR("Failed to create the DESKTOP structure!\n");
        return STATUS_NO_MEMORY;
    }

    /* Initialize the DESKTOPINFO */
    pdesk->pDeskInfo->pvDesktopBase = DesktopHeapSystemBase;
    pdesk->pDeskInfo->pvDesktopLimit = (PVOID)((ULONG_PTR)DesktopHeapSystemBase + HeapSize);
    RtlCopyMemory(pdesk->pDeskInfo->szDesktopName,
                  DesktopName->Buffer,
                  DesktopName->Length + sizeof(WCHAR));
    for (i = 0; i < NB_HOOKS; i++)
    {
        InitializeListHead(&pdesk->pDeskInfo->aphkStart[i]);
    }

    InitializeListHead(&pdesk->ShellHookWindows);
    InitializeListHead(&pdesk->PtiList);

    return STATUS_SUCCESS;
}

/* SYSCALLS *******************************************************************/

/*
 * NtUserCreateDesktop
 *
 * Creates a new desktop.
 *
 * Parameters
 *    poaAttribs
 *       Object Attributes.
 *
 *    lpszDesktopDevice
 *       Name of the device.
 *
 *    pDeviceMode
 *       Device Mode.
 *
 *    dwFlags
 *       Interaction flags.
 *
 *    dwDesiredAccess
 *       Requested type of access.
 *
 *
 * Return Value
 *    If the function succeeds, the return value is a handle to the newly
 *    created desktop. If the specified desktop already exists, the function
 *    succeeds and returns a handle to the existing desktop. When you are
 *    finished using the handle, call the CloseDesktop function to close it.
 *    If the function fails, the return value is NULL.
 *
 * Status
 *    @implemented
 */

NTSTATUS
FASTCALL
IntCreateDesktop(
    OUT HDESK* phDesktop,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN KPROCESSOR_MODE AccessMode,
    IN PUNICODE_STRING lpszDesktopDevice OPTIONAL,
    IN LPDEVMODEW lpdmw OPTIONAL,
    IN DWORD dwFlags,
    IN ACCESS_MASK dwDesiredAccess)
{
    NTSTATUS Status;
    PDESKTOP pdesk = NULL;
    HDESK hDesk;
    BOOLEAN Context = FALSE;
    UNICODE_STRING ClassName;
    LARGE_STRING WindowName;
    BOOL NoHooks = FALSE;
    PWND pWnd = NULL;
    CREATESTRUCTW Cs;
    PTHREADINFO ptiCurrent;
    PCLS pcls;

    TRACE("Enter IntCreateDesktop\n");

    ASSERT(UserIsEnteredExclusive());

    ASSERT(phDesktop);
    *phDesktop = NULL;

    ptiCurrent = PsGetCurrentThreadWin32Thread();
    ASSERT(ptiCurrent);
    ASSERT(gptiDesktopThread);

    /* Turn off hooks when calling any CreateWindowEx from inside win32k */
    NoHooks = (ptiCurrent->TIF_flags & TIF_DISABLEHOOKS);
    ptiCurrent->TIF_flags |= TIF_DISABLEHOOKS;
    ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;

    /*
     * Try to open already existing desktop
     */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ExDesktopObjectType,
                                AccessMode,
                                NULL,
                                dwDesiredAccess,
                                (PVOID)&Context,
                                (PHANDLE)&hDesk);
    if (!NT_SUCCESS(Status))
    {
        ERR("ObOpenObjectByName failed to open/create desktop\n");
        goto Quit;
    }

    /* In case the object was not created (eg if it existed), return now */
    if (Context == FALSE)
    {
        TRACE("IntCreateDesktop opened desktop '%wZ'\n", ObjectAttributes->ObjectName);
        Status = STATUS_SUCCESS;
        goto Quit;
    }

    /* Reference the desktop */
    Status = ObReferenceObjectByHandle(hDesk,
                                       0,
                                       ExDesktopObjectType,
                                       KernelMode,
                                       (PVOID*)&pdesk,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to reference desktop object\n");
        goto Quit;
    }

    /* Get the desktop window class. The thread desktop does not belong to any desktop
     * so the classes created there (including the desktop class) are allocated in the shared heap
     * It would cause problems if we used a class that belongs to the caller
     */
    ClassName.Buffer = WC_DESKTOP;
    ClassName.Length = 0;
    pcls = IntGetAndReferenceClass(&ClassName, 0, TRUE);
    if (pcls == NULL)
    {
        ASSERT(FALSE);
        Status = STATUS_UNSUCCESSFUL;
        goto Quit;
    }

    RtlZeroMemory(&WindowName, sizeof(WindowName));
    RtlZeroMemory(&Cs, sizeof(Cs));
    Cs.x = UserGetSystemMetrics(SM_XVIRTUALSCREEN),
    Cs.y = UserGetSystemMetrics(SM_YVIRTUALSCREEN),
    Cs.cx = UserGetSystemMetrics(SM_CXVIRTUALSCREEN),
    Cs.cy = UserGetSystemMetrics(SM_CYVIRTUALSCREEN),
    Cs.style = WS_POPUP|WS_CLIPCHILDREN;
    Cs.hInstance = hModClient; // hModuleWin; // Server side winproc!
    Cs.lpszName = (LPCWSTR) &WindowName;
    Cs.lpszClass = (LPCWSTR) &ClassName;

    /* Use IntCreateWindow instead of co_UserCreateWindowEx because the later expects a thread with a desktop */
    pWnd = IntCreateWindow(&Cs, &WindowName, pcls, NULL, NULL, NULL, pdesk, WINVER);
    if (pWnd == NULL)
    {
        ERR("Failed to create desktop window for the new desktop\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Quit;
    }

    pdesk->dwSessionId = PsGetCurrentProcessSessionId();
    pdesk->DesktopWindow = pWnd->head.h;
    pdesk->pDeskInfo->spwnd = pWnd;
    pWnd->fnid = FNID_DESKTOP;

    ClassName.Buffer = MAKEINTATOM(gpsi->atomSysClass[ICLS_HWNDMESSAGE]);
    ClassName.Length = 0;
    pcls = IntGetAndReferenceClass(&ClassName, 0, TRUE);
    if (pcls == NULL)
    {
        ASSERT(FALSE);
        Status = STATUS_UNSUCCESSFUL;
        goto Quit;
    }

    RtlZeroMemory(&WindowName, sizeof(WindowName));
    RtlZeroMemory(&Cs, sizeof(Cs));
    Cs.cx = Cs.cy = 100;
    Cs.style = WS_POPUP|WS_CLIPCHILDREN;
    Cs.hInstance = hModClient; // hModuleWin; // Server side winproc!
    Cs.lpszName = (LPCWSTR)&WindowName;
    Cs.lpszClass = (LPCWSTR)&ClassName;
    pWnd = IntCreateWindow(&Cs, &WindowName, pcls, NULL, NULL, NULL, pdesk, WINVER);
    if (pWnd == NULL)
    {
        ERR("Failed to create message window for the new desktop\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Quit;
    }

    pdesk->spwndMessage = pWnd;
    pWnd->fnid = FNID_MESSAGEWND;

    /* Now...
       if !(WinStaObject->Flags & WSF_NOIO) is (not set) for desktop input output mode (see wiki)
       Create Tooltip. Saved in DesktopObject->spwndTooltip.
       Tooltip dwExStyle: WS_EX_TOOLWINDOW|WS_EX_TOPMOST
       hWndParent are spwndMessage. Use hModuleWin for server side winproc!
       The rest is same as message window.
       http://msdn.microsoft.com/en-us/library/bb760250(VS.85).aspx
    */
    Status = STATUS_SUCCESS;

Quit:
    if (pdesk != NULL)
    {
        ObDereferenceObject(pdesk);
    }
    if (!NT_SUCCESS(Status) && hDesk != NULL)
    {
        ObCloseHandle(hDesk, AccessMode);
        hDesk = NULL;
    }
    if (!NoHooks)
    {
        ptiCurrent->TIF_flags &= ~TIF_DISABLEHOOKS;
        ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;
    }

    TRACE("Leave IntCreateDesktop, Status 0x%08lx\n", Status);

    if (NT_SUCCESS(Status))
        *phDesktop = hDesk;
    else
        SetLastNtError(Status);
    return Status;
}

HDESK APIENTRY
NtUserCreateDesktop(
    POBJECT_ATTRIBUTES ObjectAttributes,
    PUNICODE_STRING lpszDesktopDevice,
    LPDEVMODEW lpdmw,
    DWORD dwFlags,
    ACCESS_MASK dwDesiredAccess)
{
    NTSTATUS Status;
    HDESK hDesk;

    DECLARE_RETURN(HDESK);

    TRACE("Enter NtUserCreateDesktop\n");
    UserEnterExclusive();

    Status = IntCreateDesktop(&hDesk,
                              ObjectAttributes,
                              UserMode,
                              lpszDesktopDevice,
                              lpdmw,
                              dwFlags,
                              dwDesiredAccess);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntCreateDesktop failed, Status 0x%08lx\n", Status);
        // SetLastNtError(Status);
        RETURN(NULL);
    }

    RETURN(hDesk);

CLEANUP:
    TRACE("Leave NtUserCreateDesktop, ret=0x%p\n", _ret_);
    UserLeave();
    END_CLEANUP;
}

/*
 * NtUserOpenDesktop
 *
 * Opens an existing desktop.
 *
 * Parameters
 *    lpszDesktopName
 *       Name of the existing desktop.
 *
 *    dwFlags
 *       Interaction flags.
 *
 *    dwDesiredAccess
 *       Requested type of access.
 *
 * Return Value
 *    Handle to the desktop or zero on failure.
 *
 * Status
 *    @implemented
 */

HDESK APIENTRY
NtUserOpenDesktop(
    POBJECT_ATTRIBUTES ObjectAttributes,
    DWORD dwFlags,
    ACCESS_MASK dwDesiredAccess)
{
    NTSTATUS Status;
    HDESK Desktop;

    Status = ObOpenObjectByName(
                 ObjectAttributes,
                 ExDesktopObjectType,
                 UserMode,
                 NULL,
                 dwDesiredAccess,
                 NULL,
                 (HANDLE*)&Desktop);

    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to open desktop\n");
        SetLastNtError(Status);
        return NULL;
    }

    TRACE("Opened desktop %S with handle 0x%p\n", ObjectAttributes->ObjectName->Buffer, Desktop);

    return Desktop;
}

HDESK UserOpenInputDesktop(DWORD dwFlags,
                           BOOL fInherit,
                           ACCESS_MASK dwDesiredAccess)
{
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
    NTSTATUS Status;
    ULONG HandleAttributes = 0;
    HDESK hdesk = NULL;

    if (!gpdeskInputDesktop)
    {
        return NULL;
    }

    if (pti->ppi->prpwinsta != InputWindowStation)
    {
        ERR("Tried to open input desktop from non interactive winsta!\n");
        EngSetLastError(ERROR_INVALID_FUNCTION);
        return NULL;
    }

    if (fInherit) HandleAttributes = OBJ_INHERIT;

    /* Create a new handle to the object */
    Status = ObOpenObjectByPointer(
                 gpdeskInputDesktop,
                 HandleAttributes,
                 NULL,
                 dwDesiredAccess,
                 ExDesktopObjectType,
                 UserMode,
                 (PHANDLE)&hdesk);

    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to open input desktop object\n");
        SetLastNtError(Status);
    }

    return hdesk;
}

/*
 * NtUserOpenInputDesktop
 *
 * Opens the input (interactive) desktop.
 *
 * Parameters
 *    dwFlags
 *       Interaction flags.
 *
 *    fInherit
 *       Inheritance option.
 *
 *    dwDesiredAccess
 *       Requested type of access.
 *
 * Return Value
 *    Handle to the input desktop or zero on failure.
 *
 * Status
 *    @implemented
 */

HDESK APIENTRY
NtUserOpenInputDesktop(
    DWORD dwFlags,
    BOOL fInherit,
    ACCESS_MASK dwDesiredAccess)
{
    HDESK hdesk;

    UserEnterExclusive();
    TRACE("Enter NtUserOpenInputDesktop gpdeskInputDesktop 0x%p\n", gpdeskInputDesktop);

    hdesk = UserOpenInputDesktop(dwFlags, fInherit, dwDesiredAccess);

    TRACE("NtUserOpenInputDesktop returning 0x%p\n", hdesk);
    UserLeave();
    return hdesk;
}

/*
 * NtUserCloseDesktop
 *
 * Closes a desktop handle.
 *
 * Parameters
 *    hDesktop
 *       Handle to the desktop.
 *
 * Return Value
 *   Status
 *
 * Remarks
 *   The desktop handle can be created with NtUserCreateDesktop or
 *   NtUserOpenDesktop. This function will fail if any thread in the calling
 *   process is using the specified desktop handle or if the handle refers
 *   to the initial desktop of the calling process.
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
NtUserCloseDesktop(HDESK hDesktop)
{
    PDESKTOP pdesk;
    NTSTATUS Status;
    DECLARE_RETURN(BOOL);

    TRACE("NtUserCloseDesktop(0x%p) called\n", hDesktop);
    UserEnterExclusive();

    if (hDesktop == gptiCurrent->hdesk || hDesktop == gptiCurrent->ppi->hdeskStartup)
    {
        ERR("Attempted to close thread desktop\n");
        EngSetLastError(ERROR_BUSY);
        RETURN(FALSE);
    }

    Status = IntValidateDesktopHandle(hDesktop, UserMode, 0, &pdesk);
    if (!NT_SUCCESS(Status))
    {
        ERR("Validation of desktop handle 0x%p failed\n", hDesktop);
        RETURN(FALSE);
    }

    ObDereferenceObject(pdesk);

    Status = ObCloseHandle(hDesktop, UserMode);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to close desktop handle 0x%p\n", hDesktop);
        SetLastNtError(Status);
        RETURN(FALSE);
    }

    RETURN(TRUE);

CLEANUP:
    TRACE("Leave NtUserCloseDesktop, ret=%i\n", _ret_);
    UserLeave();
    END_CLEANUP;
}

/*
 * NtUserPaintDesktop
 *
 * The NtUserPaintDesktop function fills the clipping region in the
 * specified device context with the desktop pattern or wallpaper. The
 * function is provided primarily for shell desktops.
 *
 * Parameters
 *    hDC
 *       Handle to the device context.
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
NtUserPaintDesktop(HDC hDC)
{
    BOOL Ret;

    UserEnterExclusive();
    TRACE("Enter NtUserPaintDesktop\n");

    Ret = IntPaintDesktop(hDC);

    TRACE("Leave NtUserPaintDesktop, ret=%i\n", Ret);
    UserLeave();
    return Ret;
}

/*
 * NtUserResolveDesktop
 *
 * The NtUserResolveDesktop function attempts to retrieve valid handles to
 * a desktop and a window station suitable for the specified process.
 * The specified desktop path string is used only as a hint for the resolution.
 *
 * See the description of IntResolveDesktop for more details.
 *
 * Parameters
 *    ProcessHandle
 *       Handle to a user process.
 *
 *    DesktopPath
 *       The desktop path string used as a hint for desktop resolution.
 *
 *    bInherit
 *       Whether or not the returned handles are inheritable.
 *
 *    phWinSta
 *       Pointer to a window station handle.
 *
 * Return Value
 *    Handle to the desktop (direct return value) and
 *    handle to the associated window station (by pointer).
 *    NULL in case of failure.
 *
 * Remarks
 *    Callable by CSRSS only.
 *
 * Status
 *    @implemented
 */

HDESK
NTAPI
NtUserResolveDesktop(
    IN HANDLE ProcessHandle,
    IN PUNICODE_STRING DesktopPath,
    IN BOOL bInherit,
    OUT HWINSTA* phWinSta)
{
    NTSTATUS Status;
    PEPROCESS Process;
    HWINSTA hWinSta = NULL;
    HDESK hDesktop  = NULL;
    UNICODE_STRING CapturedDesktopPath;

    /* Allow only the Console Server to perform this operation (via CSRSS) */
    if (PsGetCurrentProcess() != gpepCSRSS)
        return NULL;

    /* Get the process object the user handle was referencing */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_QUERY_INFORMATION,
                                       *PsProcessType,
                                       UserMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status))
        return NULL;

    UserEnterExclusive();

    _SEH2_TRY
    {
        /* Probe the handle pointer */
        // ProbeForWriteHandle
        ProbeForWrite(phWinSta, sizeof(HWINSTA), sizeof(HWINSTA));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        _SEH2_YIELD(goto Quit);
    }
    _SEH2_END;

    /* Capture the user desktop path string */
    Status = ProbeAndCaptureUnicodeString(&CapturedDesktopPath,
                                          UserMode,
                                          DesktopPath);
    if (!NT_SUCCESS(Status))
        goto Quit;

    /* Call the internal function */
    Status = IntResolveDesktop(Process,
                               &CapturedDesktopPath,
                               bInherit,
                               &hWinSta,
                               &hDesktop);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntResolveDesktop failed, Status 0x%08lx\n", Status);
        hWinSta  = NULL;
        hDesktop = NULL;
    }

    _SEH2_TRY
    {
        /* Return the window station handle */
        *phWinSta = hWinSta;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();

        /* We failed, close the opened desktop and window station */
        if (hDesktop) ObCloseHandle(hDesktop, UserMode);
        hDesktop = NULL;
        if (hWinSta) ObCloseHandle(hWinSta, UserMode);
    }
    _SEH2_END;

    /* Free the captured string */
    ReleaseCapturedUnicodeString(&CapturedDesktopPath, UserMode);

Quit:
    UserLeave();

    /* Dereference the process object */
    ObDereferenceObject(Process);

    /* Return the desktop handle */
    return hDesktop;
}

/*
 * NtUserSwitchDesktop
 *
 * Sets the current input (interactive) desktop.
 *
 * Parameters
 *    hDesktop
 *       Handle to desktop.
 *
 * Return Value
 *    Status
 *
 * Status
 *    @unimplemented
 */

BOOL APIENTRY
NtUserSwitchDesktop(HDESK hdesk)
{
    PDESKTOP pdesk;
    NTSTATUS Status;
    BOOL bRedrawDesktop;
    DECLARE_RETURN(BOOL);

    UserEnterExclusive();
    TRACE("Enter NtUserSwitchDesktop(0x%p)\n", hdesk);

    Status = IntValidateDesktopHandle(hdesk, UserMode, 0, &pdesk);
    if (!NT_SUCCESS(Status))
    {
        ERR("Validation of desktop handle 0x%p failed\n", hdesk);
        RETURN(FALSE);
    }

    if (PsGetCurrentProcessSessionId() != pdesk->rpwinstaParent->dwSessionId)
    {
        ObDereferenceObject(pdesk);
        ERR("NtUserSwitchDesktop called for a desktop of a different session\n");
        RETURN(FALSE);
    }

    if (pdesk == gpdeskInputDesktop)
    {
        ObDereferenceObject(pdesk);
        WARN("NtUserSwitchDesktop called for active desktop\n");
        RETURN(TRUE);
    }

    /*
     * Don't allow applications switch the desktop if it's locked, unless the caller
     * is the logon application itself
     */
    if ((pdesk->rpwinstaParent->Flags & WSS_LOCKED) &&
        gpidLogon != PsGetCurrentProcessId())
    {
        ObDereferenceObject(pdesk);
        ERR("Switching desktop 0x%p denied because the window station is locked!\n", hdesk);
        RETURN(FALSE);
    }

    if (pdesk->rpwinstaParent != InputWindowStation)
    {
        ObDereferenceObject(pdesk);
        ERR("Switching desktop 0x%p denied because desktop doesn't belong to the interactive winsta!\n", hdesk);
        RETURN(FALSE);
    }

    /* FIXME: Fail if the process is associated with a secured
              desktop such as Winlogon or Screen-Saver */
    /* FIXME: Connect to input device */

    TRACE("Switching from desktop 0x%p to 0x%p\n", gpdeskInputDesktop, pdesk);

    bRedrawDesktop = FALSE;

    /* The first time SwitchDesktop is called, gpdeskInputDesktop is NULL */
    if (gpdeskInputDesktop != NULL)
    {
        if ((gpdeskInputDesktop->pDeskInfo->spwnd->style & WS_VISIBLE) == WS_VISIBLE)
            bRedrawDesktop = TRUE;

        /* Hide the previous desktop window */
        IntHideDesktop(gpdeskInputDesktop);
    }

    /* Set the active desktop in the desktop's window station. */
    InputWindowStation->ActiveDesktop = pdesk;

    /* Set the global state. */
    gpdeskInputDesktop = pdesk;

    /* Show the new desktop window */
    co_IntShowDesktop(pdesk, UserGetSystemMetrics(SM_CXSCREEN), UserGetSystemMetrics(SM_CYSCREEN), bRedrawDesktop);

    TRACE("SwitchDesktop gpdeskInputDesktop 0x%p\n", gpdeskInputDesktop);
    ObDereferenceObject(pdesk);

    RETURN(TRUE);

CLEANUP:
    TRACE("Leave NtUserSwitchDesktop, ret=%i\n", _ret_);
    UserLeave();
    END_CLEANUP;
}

/*
 * NtUserGetThreadDesktop
 *
 * Status
 *    @implemented
 */

HDESK APIENTRY
NtUserGetThreadDesktop(DWORD dwThreadId, HDESK hConsoleDesktop)
{
    HDESK hDesk;
    NTSTATUS Status;
    PTHREADINFO pti;
    PEPROCESS Process;
    PDESKTOP DesktopObject;
    OBJECT_HANDLE_INFORMATION HandleInformation;

    UserEnterExclusive();
    TRACE("Enter NtUserGetThreadDesktop\n");

    if (!dwThreadId)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        hDesk = NULL;
        goto Quit;
    }

    /* Validate the Win32 thread and retrieve its information */
    pti = IntTID2PTI(UlongToHandle(dwThreadId));
    if (pti)
    {
        /* Get the desktop handle of the thread */
        hDesk = pti->hdesk;
        Process = pti->ppi->peProcess;
    }
    else if (hConsoleDesktop)
    {
        /*
         * The thread may belong to a console, so attempt to use the provided
         * console desktop handle as a fallback. Otherwise this means that the
         * thread is either not Win32 or invalid.
         */
        hDesk = hConsoleDesktop;
        Process = gpepCSRSS;
    }
    else
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        hDesk = NULL;
        goto Quit;
    }

    if (!hDesk)
    {
        ERR("Desktop information of thread 0x%x broken!?\n", dwThreadId);
        goto Quit;
    }

    if (Process == PsGetCurrentProcess())
    {
        /*
         * Just return the handle, since we queried the desktop handle
         * of a thread running in the same context.
         */
        goto Quit;
    }

    /*
     * We could just use the cached rpdesk instead of looking up the handle,
     * but it may actually be safer to validate the desktop and get a temporary
     * reference to it so that it does not disappear under us (e.g. when the
     * desktop is being destroyed) during the operation.
     */
    /*
     * Switch into the context of the thread we are trying to get
     * the desktop from, so we can use the handle.
     */
    KeAttachProcess(&Process->Pcb);
    Status = ObReferenceObjectByHandle(hDesk,
                                       0,
                                       ExDesktopObjectType,
                                       UserMode,
                                       (PVOID*)&DesktopObject,
                                       &HandleInformation);
    KeDetachProcess();

    if (NT_SUCCESS(Status))
    {
        /*
         * Lookup our handle table if we can find a handle to the desktop object.
         * If not, create one.
         * QUESTION: Do we really need to create a handle in case it doesn't exist??
         */
        hDesk = IntGetDesktopObjectHandle(DesktopObject);

        /* All done, we got a valid handle to the desktop */
        ObDereferenceObject(DesktopObject);
    }
    else
    {
        /* The handle could not be found, there is nothing to get... */
        hDesk = NULL;
    }

    if (!hDesk)
    {
        ERR("Could not retrieve or access desktop for thread 0x%x\n", dwThreadId);
        EngSetLastError(ERROR_ACCESS_DENIED);
    }

Quit:
    TRACE("Leave NtUserGetThreadDesktop, hDesk = 0x%p\n", hDesk);
    UserLeave();
    return hDesk;
}

static NTSTATUS
IntUnmapDesktopView(IN PDESKTOP pdesk)
{
    PPROCESSINFO ppi;
    PW32HEAP_USER_MAPPING HeapMapping, *PrevLink;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("IntUnmapDesktopView called for desktop object %p\n", pdesk);

    ppi = PsGetCurrentProcessWin32Process();

    /*
     * Unmap if we're the last thread using the desktop.
     * Start the search at the next mapping: skip the first entry
     * as it must be the global user heap mapping.
     */
    PrevLink = &ppi->HeapMappings.Next;
    HeapMapping = *PrevLink;
    while (HeapMapping != NULL)
    {
        if (HeapMapping->KernelMapping == (PVOID)pdesk->pheapDesktop)
        {
            if (--HeapMapping->Count == 0)
            {
                *PrevLink = HeapMapping->Next;

                TRACE("ppi 0x%p unmapped heap of desktop 0x%p\n", ppi, pdesk);
                Status = MmUnmapViewOfSection(PsGetCurrentProcess(),
                                              HeapMapping->UserMapping);

                ObDereferenceObject(pdesk);

                UserHeapFree(HeapMapping);
                break;
            }
        }

        PrevLink = &HeapMapping->Next;
        HeapMapping = HeapMapping->Next;
    }

    return Status;
}

static NTSTATUS
IntMapDesktopView(IN PDESKTOP pdesk)
{
    PPROCESSINFO ppi;
    PW32HEAP_USER_MAPPING HeapMapping, *PrevLink;
    PVOID UserBase = NULL;
    SIZE_T ViewSize = 0;
    LARGE_INTEGER Offset;
    NTSTATUS Status;

    TRACE("IntMapDesktopView called for desktop object 0x%p\n", pdesk);

    ppi = PsGetCurrentProcessWin32Process();

    /*
     * Find out if another thread already mapped the desktop heap.
     * Start the search at the next mapping: skip the first entry
     * as it must be the global user heap mapping.
     */
    PrevLink = &ppi->HeapMappings.Next;
    HeapMapping = *PrevLink;
    while (HeapMapping != NULL)
    {
        if (HeapMapping->KernelMapping == (PVOID)pdesk->pheapDesktop)
        {
            HeapMapping->Count++;
            return STATUS_SUCCESS;
        }

        PrevLink = &HeapMapping->Next;
        HeapMapping = HeapMapping->Next;
    }

    /* We're the first, map the heap */
    Offset.QuadPart = 0;
    Status = MmMapViewOfSection(pdesk->hsectionDesktop,
                                PsGetCurrentProcess(),
                                &UserBase,
                                0,
                                0,
                                &Offset,
                                &ViewSize,
                                ViewUnmap,
                                SEC_NO_CHANGE,
                                PAGE_EXECUTE_READ); /* Would prefer PAGE_READONLY, but thanks to RTL heaps... */
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to map desktop\n");
        return Status;
    }

    TRACE("ppi 0x%p mapped heap of desktop 0x%p\n", ppi, pdesk);

    /* Add the mapping */
    HeapMapping = UserHeapAlloc(sizeof(*HeapMapping));
    if (HeapMapping == NULL)
    {
        MmUnmapViewOfSection(PsGetCurrentProcess(), UserBase);
        ERR("UserHeapAlloc() failed!\n");
        return STATUS_NO_MEMORY;
    }

    HeapMapping->Next = NULL;
    HeapMapping->KernelMapping = (PVOID)pdesk->pheapDesktop;
    HeapMapping->UserMapping = UserBase;
    HeapMapping->Limit = ViewSize;
    HeapMapping->Count = 1;
    *PrevLink = HeapMapping;

    ObReferenceObject(pdesk);

    return STATUS_SUCCESS;
}

BOOL
IntSetThreadDesktop(IN HDESK hDesktop,
                    IN BOOL FreeOnFailure)
{
    PDESKTOP pdesk = NULL, pdeskOld;
    PTHREADINFO pti;
    NTSTATUS Status;
    PCLIENTTHREADINFO pctiOld, pctiNew = NULL;
    PCLIENTINFO pci;

    ASSERT(NtCurrentTeb());

    TRACE("IntSetThreadDesktop hDesktop:0x%p, FOF:%i\n",hDesktop, FreeOnFailure);

    pti = PsGetCurrentThreadWin32Thread();
    pci = pti->pClientInfo;

    /* If the caller gave us a desktop, ensure it is valid */
    if (hDesktop != NULL)
    {
        /* Validate the new desktop. */
        Status = IntValidateDesktopHandle(hDesktop, UserMode, 0, &pdesk);
        if (!NT_SUCCESS(Status))
        {
            ERR("Validation of desktop handle 0x%p failed\n", hDesktop);
            return FALSE;
        }

        if (pti->rpdesk == pdesk)
        {
            /* Nothing to do */
            ObDereferenceObject(pdesk);
            return TRUE;
        }
    }

    /* Make sure that we don't own any window in the current desktop */
    if (!IsListEmpty(&pti->WindowListHead))
    {
        if (pdesk)
            ObDereferenceObject(pdesk);
        ERR("Attempted to change thread desktop although the thread has windows!\n");
        EngSetLastError(ERROR_BUSY);
        return FALSE;
    }

    /* Desktop is being re-set so clear out foreground. */
    if (pti->rpdesk != pdesk && pti->MessageQueue == gpqForeground)
    {
        // Like above, there shouldn't be any windows, hooks or anything active on this threads desktop!
        IntSetFocusMessageQueue(NULL);
    }

    /* Before doing the switch, map the new desktop heap and allocate the new pcti */
    if (pdesk != NULL)
    {
        Status = IntMapDesktopView(pdesk);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to map desktop heap!\n");
            ObDereferenceObject(pdesk);
            SetLastNtError(Status);
            return FALSE;
        }

        pctiNew = DesktopHeapAlloc(pdesk, sizeof(CLIENTTHREADINFO));
        if (pctiNew == NULL)
        {
            ERR("Failed to allocate new pcti\n");
            IntUnmapDesktopView(pdesk);
            ObDereferenceObject(pdesk);
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    /*
     * Processes, in particular Winlogon.exe, that manage window stations
     * (especially the interactive WinSta0 window station) and desktops,
     * may not be able to connect at startup to a window station and have
     * an associated desktop as well, if none exists on the system already.
     * Because creating a new window station does not affect the window station
     * associated to the process, and because neither by associating a window
     * station to the process nor creating a new desktop on it does associate
     * a startup desktop to that process, the process has to actually assigns
     * one of its threads to a desktop so that it gets automatically an assigned
     * startup desktop.
     *
     * This is what actually happens for Winlogon.exe, which is started without
     * any window station and desktop. By creating the first (and therefore
     * interactive) WinSta0 window station, then assigning WinSta0 to itself
     * and creating the Default desktop on it, and then assigning this desktop
     * to its main thread, Winlogon.exe basically does the similar steps that
     * would have been done automatically at its startup if there were already
     * an existing WinSta0 window station and Default desktop.
     *
     * Of course all this must not be done if we are a SYSTEM or CSRSS thread.
     */
    // if (pti->ppi->peProcess != gpepCSRSS)
    if (!(pti->TIF_flags & (TIF_SYSTEMTHREAD | TIF_CSRSSTHREAD)) &&
        pti->ppi->rpdeskStartup == NULL && hDesktop != NULL)
    {
        ERR("The process 0x%p '%s' didn't have an assigned startup desktop before, assigning it now!\n",
            pti->ppi->peProcess, pti->ppi->peProcess->ImageFileName);

        pti->ppi->hdeskStartup = hDesktop;
        pti->ppi->rpdeskStartup = pdesk;
    }

    /* free all classes or move them to the shared heap */
    if (pti->rpdesk != NULL)
    {
        if (!IntCheckProcessDesktopClasses(pti->rpdesk, FreeOnFailure))
        {
            ERR("Failed to move process classes to shared heap!\n");
            if (pdesk)
            {
                DesktopHeapFree(pdesk, pctiNew);
                IntUnmapDesktopView(pdesk);
                ObDereferenceObject(pdesk);
            }
            return FALSE;
        }
    }

    pdeskOld = pti->rpdesk;
    if (pti->pcti != &pti->cti)
        pctiOld = pti->pcti;
    else
        pctiOld = NULL;

    /* do the switch */
    if (pdesk != NULL)
    {
        pti->rpdesk = pdesk;
        pti->hdesk = hDesktop;
        pti->pDeskInfo = pti->rpdesk->pDeskInfo;
        pti->pcti = pctiNew;

        pci->ulClientDelta = DesktopHeapGetUserDelta();
        pci->pDeskInfo = (PVOID)((ULONG_PTR)pti->pDeskInfo - pci->ulClientDelta);
        pci->pClientThreadInfo = (PVOID)((ULONG_PTR)pti->pcti - pci->ulClientDelta);

        /* initialize the new pcti */
        if (pctiOld != NULL)
        {
            RtlCopyMemory(pctiNew, pctiOld, sizeof(CLIENTTHREADINFO));
        }
        else
        {
            RtlZeroMemory(pctiNew, sizeof(CLIENTTHREADINFO));
            pci->fsHooks = pti->fsHooks;
            pci->dwTIFlags = pti->TIF_flags;
        }
    }
    else
    {
        pti->rpdesk = NULL;
        pti->hdesk = NULL;
        pti->pDeskInfo = NULL;
        pti->pcti = &pti->cti; // Always point inside so there will be no crash when posting or sending msg's!
        pci->ulClientDelta = 0;
        pci->pDeskInfo = NULL;
        pci->pClientThreadInfo = NULL;
    }

    /* clean up the old desktop */
    if (pdeskOld != NULL)
    {
        RemoveEntryList(&pti->PtiLink);
        if (pctiOld) DesktopHeapFree(pdeskOld, pctiOld);
        IntUnmapDesktopView(pdeskOld);
        ObDereferenceObject(pdeskOld);
    }

    if (pdesk)
    {
        InsertTailList(&pdesk->PtiList, &pti->PtiLink);
    }

    TRACE("IntSetThreadDesktop: pti 0x%p ppi 0x%p switched from object 0x%p to 0x%p\n", pti, pti->ppi, pdeskOld, pdesk);

    return TRUE;
}

/*
 * NtUserSetThreadDesktop
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
NtUserSetThreadDesktop(HDESK hDesktop)
{
    BOOL ret = FALSE;

    UserEnterExclusive();

    // FIXME: IntSetThreadDesktop validates the desktop handle, it should happen
    // here too and set the NT error level. Q. Is it necessary to have the validation
    // in IntSetThreadDesktop? Is it needed there too?
    if (hDesktop || (!hDesktop && PsGetCurrentProcess() == gpepCSRSS))
        ret = IntSetThreadDesktop(hDesktop, FALSE);

    UserLeave();

    return ret;
}

/* EOF */
