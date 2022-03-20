/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS Win32k subsystem
 *  PURPOSE:          Window stations
 *  FILE:             win32ss/user/ntuser/winsta.c
 *  PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 *  TODO:             The process window station is created on
 *                    the first USER32/GDI32 call not related
 *                    to window station/desktop handling
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserWinsta);

/* GLOBALS *******************************************************************/

/*
 * The currently active window station. This is the
 * only one interactive window station on the system.
 */
PWINSTATION_OBJECT InputWindowStation = NULL;

/* Winlogon SAS window */
HWND hwndSAS = NULL;

/* Full path to WindowStations directory */
UNICODE_STRING gustrWindowStationsDir;

/* INITIALIZATION FUNCTIONS ****************************************************/

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitWindowStationImpl(VOID)
{
    GENERIC_MAPPING IntWindowStationMapping = { WINSTA_READ,
                                                WINSTA_WRITE,
                                                WINSTA_EXECUTE,
                                                WINSTA_ACCESS_ALL};

    /* Set Winsta Object Attributes */
    ExWindowStationObjectType->TypeInfo.DefaultNonPagedPoolCharge = sizeof(WINSTATION_OBJECT);
    ExWindowStationObjectType->TypeInfo.GenericMapping = IntWindowStationMapping;
    ExWindowStationObjectType->TypeInfo.ValidAccessMask = WINSTA_ACCESS_ALL;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
UserCreateWinstaDirectory(VOID)
{
    NTSTATUS Status;
    PPEB Peb;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hWinstaDir;
    WCHAR wstrWindowStationsDir[MAX_PATH];

    /* Create the WindowStations directory and cache its path for later use */
    Peb = NtCurrentPeb();
    if(Peb->SessionId == 0)
    {
        if (!RtlCreateUnicodeString(&gustrWindowStationsDir, WINSTA_OBJ_DIR))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        Status = RtlStringCbPrintfW(wstrWindowStationsDir,
                                    sizeof(wstrWindowStationsDir),
                                    L"%ws\\%lu%ws",
                                    SESSION_DIR,
                                    Peb->SessionId,
                                    WINSTA_OBJ_DIR);
        if (!NT_SUCCESS(Status))
            return Status;

        if (!RtlCreateUnicodeString(&gustrWindowStationsDir, wstrWindowStationsDir))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &gustrWindowStationsDir,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwCreateDirectoryObject(&hWinstaDir, DIRECTORY_CREATE_OBJECT, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not create %wZ directory (Status 0x%X)\n", &gustrWindowStationsDir,  Status);
        return Status;
    }

    TRACE("Created directory %wZ for session %lu\n", &gustrWindowStationsDir, Peb->SessionId);

    return Status;
}

/* OBJECT CALLBACKS ***********************************************************/

NTSTATUS
NTAPI
IntWinStaObjectDelete(
    _In_ PVOID Parameters)
{
    PWIN32_DELETEMETHOD_PARAMETERS DeleteParameters = Parameters;
    PWINSTATION_OBJECT WinSta = (PWINSTATION_OBJECT)DeleteParameters->Object;

    TRACE("Deleting window station 0x%p\n", WinSta);

    if (WinSta == InputWindowStation)
    {
        ERR("WARNING: Deleting the interactive window station '%wZ'!\n",
            &(OBJECT_HEADER_TO_NAME_INFO(OBJECT_TO_OBJECT_HEADER(InputWindowStation))->Name));

        /* Only Winlogon can close and delete the interactive window station */
        ASSERT(gpidLogon == PsGetCurrentProcessId());

        InputWindowStation = NULL;
    }

    WinSta->Flags |= WSS_DYING;

    UserEmptyClipboardData(WinSta);

    RtlDestroyAtomTable(WinSta->AtomTable);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IntWinStaObjectParse(
    _In_ PVOID Parameters)
{
    PWIN32_PARSEMETHOD_PARAMETERS ParseParameters = Parameters;
    PUNICODE_STRING RemainingName = ParseParameters->RemainingName;

    /* Assume we don't find anything */
    *ParseParameters->Object = NULL;

    /* Check for an empty name */
    if (!RemainingName->Length)
    {
        /* Make sure this is a window station, can't parse a desktop now */
        if (ParseParameters->ObjectType != ExWindowStationObjectType)
        {
            /* Fail */
            return STATUS_OBJECT_TYPE_MISMATCH;
        }

        /* Reference the window station and return */
        ObReferenceObject(ParseParameters->ParseObject);
        *ParseParameters->Object = ParseParameters->ParseObject;
        return STATUS_SUCCESS;
    }

    /* Check for leading slash */
    if (RemainingName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
    {
        /* Skip it */
        RemainingName->Buffer++;
        RemainingName->Length -= sizeof(WCHAR);
        RemainingName->MaximumLength -= sizeof(WCHAR);
    }

    /* Check if there is still a slash */
    if (wcschr(RemainingName->Buffer, OBJ_NAME_PATH_SEPARATOR))
    {
        /* In this case, fail */
        return STATUS_OBJECT_PATH_INVALID;
    }

    /*
     * Check if we are parsing a desktop.
     */
    if (ParseParameters->ObjectType == ExDesktopObjectType)
    {
        /* Then call the desktop parse routine */
        return IntDesktopObjectParse(ParseParameters->ParseObject,
                                     ParseParameters->ObjectType,
                                     ParseParameters->AccessState,
                                     ParseParameters->AccessMode,
                                     ParseParameters->Attributes,
                                     ParseParameters->CompleteName,
                                     RemainingName,
                                     ParseParameters->Context,
                                     ParseParameters->SecurityQos,
                                     ParseParameters->Object);
    }

    /* Should hopefully never get here */
    return STATUS_OBJECT_TYPE_MISMATCH;
}

NTSTATUS
NTAPI
IntWinStaOkToClose(
    _In_ PVOID Parameters)
{
    PWIN32_OKAYTOCLOSEMETHOD_PARAMETERS OkToCloseParameters = Parameters;
    PPROCESSINFO ppi;

    ppi = PsGetCurrentProcessWin32Process();

    if (ppi && (OkToCloseParameters->Handle == ppi->hwinsta))
    {
        return STATUS_ACCESS_DENIED;
    }

    return STATUS_SUCCESS;
}

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * IntValidateWindowStationHandle
 *
 * Validates the window station handle.
 *
 * Remarks
 *    If the function succeeds, the handle remains referenced. If the
 *    fucntion fails, last error is set.
 */

NTSTATUS FASTCALL
IntValidateWindowStationHandle(
    HWINSTA WindowStation,
    KPROCESSOR_MODE AccessMode,
    ACCESS_MASK DesiredAccess,
    PWINSTATION_OBJECT *Object,
    POBJECT_HANDLE_INFORMATION pObjectHandleInfo)
{
    NTSTATUS Status;

    if (WindowStation == NULL)
    {
        ERR("Invalid window station handle\n");
        EngSetLastError(ERROR_INVALID_HANDLE);
        return STATUS_INVALID_HANDLE;
    }

    Status = ObReferenceObjectByHandle(WindowStation,
                                       DesiredAccess,
                                       ExWindowStationObjectType,
                                       AccessMode,
                                       (PVOID*)Object,
                                       pObjectHandleInfo);

    if (!NT_SUCCESS(Status))
        SetLastNtError(Status);

    return Status;
}

BOOL FASTCALL
co_IntInitializeDesktopGraphics(VOID)
{
    TEXTMETRICW tmw;
    UNICODE_STRING DriverName = RTL_CONSTANT_STRING(L"DISPLAY");
    PDESKTOP pdesk;

    if (PDEVOBJ_lChangeDisplaySettings(NULL, NULL, NULL, &gpmdev, TRUE) != DISP_CHANGE_SUCCESSFUL)
    {
        ERR("PDEVOBJ_lChangeDisplaySettings() failed.\n");
        return FALSE;
    }

    ScreenDeviceContext = IntGdiCreateDC(&DriverName, NULL, NULL, NULL, FALSE);
    if (NULL == ScreenDeviceContext)
    {
        IntDestroyPrimarySurface();
        return FALSE;
    }
    GreSetDCOwner(ScreenDeviceContext, GDI_OBJ_HMGR_PUBLIC);

    if (!IntCreatePrimarySurface())
    {
        return FALSE;
    }

    hSystemBM = NtGdiCreateCompatibleDC(ScreenDeviceContext);

    NtGdiSelectFont(hSystemBM, NtGdiGetStockObject(SYSTEM_FONT));
    GreSetDCOwner(hSystemBM, GDI_OBJ_HMGR_PUBLIC);

    /* Update the system metrics */
    InitMetrics();

    /* Set new size of the monitor */
    UserUpdateMonitorSize((HDEV)gpmdev->ppdevGlobal);

    /* Update the SERVERINFO */
    gpsi->aiSysMet[SM_CXSCREEN] = gpmdev->ppdevGlobal->gdiinfo.ulHorzRes;
    gpsi->aiSysMet[SM_CYSCREEN] = gpmdev->ppdevGlobal->gdiinfo.ulVertRes;
    gpsi->Planes        = NtGdiGetDeviceCaps(ScreenDeviceContext, PLANES);
    gpsi->BitsPixel     = NtGdiGetDeviceCaps(ScreenDeviceContext, BITSPIXEL);
    gpsi->BitCount      = gpsi->Planes * gpsi->BitsPixel;
    gpsi->dmLogPixels   = NtGdiGetDeviceCaps(ScreenDeviceContext, LOGPIXELSY);
    if (NtGdiGetDeviceCaps(ScreenDeviceContext, RASTERCAPS) & RC_PALETTE)
    {
        gpsi->PUSIFlags |= PUSIF_PALETTEDISPLAY;
    }
    else
    {
        gpsi->PUSIFlags &= ~PUSIF_PALETTEDISPLAY;
    }
    // Font is realized and this dc was previously set to internal DC_ATTR.
    gpsi->cxSysFontChar = IntGetCharDimensions(hSystemBM, &tmw, (DWORD*)&gpsi->cySysFontChar);
    gpsi->tmSysFont     = tmw;

    /* Put the pointer in the center of the screen */
    gpsi->ptCursor.x = gpsi->aiSysMet[SM_CXSCREEN] / 2;
    gpsi->ptCursor.y = gpsi->aiSysMet[SM_CYSCREEN] / 2;

    /* Attach monitor */
    UserAttachMonitor((HDEV)gpmdev->ppdevGlobal);

    /* Setup the cursor */
    co_IntLoadDefaultCursors();

    /* Setup the icons */
    co_IntSetWndIcons();

    /* Setup Menu */
    MenuInit();

    /* Show the desktop */
    pdesk = IntGetActiveDesktop();
    ASSERT(pdesk);
    co_IntShowDesktop(pdesk, gpsi->aiSysMet[SM_CXSCREEN], gpsi->aiSysMet[SM_CYSCREEN], TRUE);

    /* HACK: display wallpaper on all secondary displays */
    {
        PGRAPHICS_DEVICE pGraphicsDevice;
        UNICODE_STRING DriverName = RTL_CONSTANT_STRING(L"DISPLAY");
        UNICODE_STRING DisplayName;
        HDC hdc;
        ULONG iDevNum;

        for (iDevNum = 1; (pGraphicsDevice = EngpFindGraphicsDevice(NULL, iDevNum)) != NULL; iDevNum++)
        {
            RtlInitUnicodeString(&DisplayName, pGraphicsDevice->szWinDeviceName);
            hdc = IntGdiCreateDC(&DriverName, &DisplayName, NULL, NULL, FALSE);
            IntPaintDesktop(hdc);
        }
    }

    return TRUE;
}

VOID FASTCALL
IntEndDesktopGraphics(VOID)
{
    if (NULL != ScreenDeviceContext)
    {  // No need to allocate a new dcattr.
        GreSetDCOwner(ScreenDeviceContext, GDI_OBJ_HMGR_POWNED);
        GreDeleteObject(ScreenDeviceContext);
        ScreenDeviceContext = NULL;
    }
    IntHideDesktop(IntGetActiveDesktop());
    IntDestroyPrimarySurface();
}

HDC FASTCALL
IntGetScreenDC(VOID)
{
    return ScreenDeviceContext;
}

BOOL FASTCALL
CheckWinstaAttributeAccess(ACCESS_MASK DesiredAccess)
{
    PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();
    if ( gpidLogon != PsGetCurrentProcessId() )
    {
        if (!(ppi->W32PF_flags & W32PF_IOWINSTA))
        {
            ERR("Requires Interactive Window Station\n");
            EngSetLastError(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
            return FALSE;
        }
        if (!RtlAreAllAccessesGranted(ppi->amwinsta, DesiredAccess))
        {
            ERR("Access Denied\n");
            EngSetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }
    return TRUE;
}


/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * NtUserCreateWindowStation
 *
 * Creates a new window station.
 *
 * Parameters
 *    lpszWindowStationName
 *       Pointer to a null-terminated string specifying the name of the
 *       window station to be created. Window station names are
 *       case-insensitive and cannot contain backslash characters (\).
 *       Only members of the Administrators group are allowed to specify a
 *       name.
 *
 *    dwDesiredAccess
 *       Requested type of access
 *
 *    lpSecurity
 *       Security descriptor
 *
 *    Unknown3, Unknown4, Unknown5, Unknown6
 *       Unused
 *
 * Return Value
 *    If the function succeeds, the return value is a handle to the newly
 *    created window station. If the specified window station already
 *    exists, the function succeeds and returns a handle to the existing
 *    window station. If the function fails, the return value is NULL.
 *
 * Status
 *    @implemented
 */

NTSTATUS
FASTCALL
IntCreateWindowStation(
    OUT HWINSTA* phWinSta,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN KPROCESSOR_MODE AccessMode,
    IN KPROCESSOR_MODE OwnerMode,
    IN ACCESS_MASK dwDesiredAccess,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4,
    DWORD Unknown5,
    DWORD Unknown6)
{
    NTSTATUS Status;
    HWINSTA hWinSta;
    PWINSTATION_OBJECT WindowStation;

    TRACE("IntCreateWindowStation called\n");

    ASSERT(phWinSta);
    *phWinSta = NULL;

    Status = ObOpenObjectByName(ObjectAttributes,
                                ExWindowStationObjectType,
                                AccessMode,
                                NULL,
                                dwDesiredAccess,
                                NULL,
                                (PVOID*)&hWinSta);
    if (NT_SUCCESS(Status))
    {
        TRACE("IntCreateWindowStation opened window station '%wZ'\n",
              ObjectAttributes->ObjectName);
        *phWinSta = hWinSta;
        return Status;
    }

    /*
     * No existing window station found, try to create a new one.
     */

    /* Create the window station object */
    Status = ObCreateObject(AccessMode,
                            ExWindowStationObjectType,
                            ObjectAttributes,
                            OwnerMode,
                            NULL,
                            sizeof(WINSTATION_OBJECT),
                            0,
                            0,
                            (PVOID*)&WindowStation);
    if (!NT_SUCCESS(Status))
    {
        ERR("ObCreateObject failed for window station '%wZ', Status 0x%08lx\n",
            ObjectAttributes->ObjectName, Status);
        SetLastNtError(Status);
        return Status;
    }

    /* Initialize the window station */
    RtlZeroMemory(WindowStation, sizeof(WINSTATION_OBJECT));

    InitializeListHead(&WindowStation->DesktopListHead);
    WindowStation->dwSessionId = NtCurrentPeb()->SessionId;
    Status = RtlCreateAtomTable(37, &WindowStation->AtomTable);
    if (!NT_SUCCESS(Status))
    {
        ERR("RtlCreateAtomTable failed for window station '%wZ', Status 0x%08lx\n",
            ObjectAttributes->ObjectName, Status);
        ObDereferenceObject(WindowStation);
        SetLastNtError(Status);
        return Status;
    }

    Status = ObInsertObject(WindowStation,
                            NULL,
                            dwDesiredAccess,
                            0,
                            NULL,
                            (PVOID*)&hWinSta);
    if (!NT_SUCCESS(Status))
    {
        ERR("ObInsertObject failed for window station, Status 0x%08lx\n", Status);
        SetLastNtError(Status);
        return Status;
    }

    // FIXME! TODO: Add this new window station to a linked list

    if (InputWindowStation == NULL)
    {
        ERR("Initializing input window station\n");

        /* Only Winlogon can create the interactive window station */
        ASSERT(gpidLogon == PsGetCurrentProcessId());

        InputWindowStation = WindowStation;
        WindowStation->Flags &= ~WSS_NOIO;

        InitCursorImpl();

        UserCreateSystemThread(ST_DESKTOP_THREAD);
        UserCreateSystemThread(ST_RIT);

        /* Desktop functions require the desktop thread running so wait for it to initialize */
        UserLeaveCo();
        KeWaitForSingleObject(gpDesktopThreadStartedEvent,
                              UserRequest,
                              UserMode,
                              FALSE,
                              NULL);
        UserEnterCo();
    }
    else
    {
        WindowStation->Flags |= WSS_NOIO;
    }

    TRACE("IntCreateWindowStation created window station '%wZ' object 0x%p handle 0x%p\n",
          ObjectAttributes->ObjectName, WindowStation, hWinSta);

    *phWinSta = hWinSta;
    EngSetLastError(ERROR_SUCCESS);

    return STATUS_SUCCESS;
}

static VOID
FreeUserModeWindowStationName(
    IN OUT PUNICODE_STRING WindowStationName,
    IN PUNICODE_STRING TebStaticUnicodeString,
    IN OUT POBJECT_ATTRIBUTES UserModeObjectAttributes OPTIONAL,
    IN POBJECT_ATTRIBUTES LocalObjectAttributes OPTIONAL)
{
    SIZE_T MemSize = 0;

    /* Try to restore the user's UserModeObjectAttributes */
    if (UserModeObjectAttributes && LocalObjectAttributes)
    {
        _SEH2_TRY
        {
            ProbeForWrite(UserModeObjectAttributes, sizeof(OBJECT_ATTRIBUTES), sizeof(ULONG));
            *UserModeObjectAttributes = *LocalObjectAttributes;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            NOTHING;
        }
        _SEH2_END;
    }

    /* Free the user-mode memory */
    if (WindowStationName && (WindowStationName != TebStaticUnicodeString))
    {
        ZwFreeVirtualMemory(ZwCurrentProcess(),
                            (PVOID*)&WindowStationName,
                            &MemSize,
                            MEM_RELEASE);
    }
}

static NTSTATUS
BuildUserModeWindowStationName(
    IN OUT POBJECT_ATTRIBUTES UserModeObjectAttributes,
    IN OUT POBJECT_ATTRIBUTES LocalObjectAttributes,
    OUT PUNICODE_STRING* WindowStationName,
    OUT PUNICODE_STRING* TebStaticUnicodeString)
{
    NTSTATUS Status;
    SIZE_T MemSize;

    LUID CallerLuid;
    PTEB Teb;
    USHORT StrSize;

    *WindowStationName = NULL;
    *TebStaticUnicodeString = NULL;

    /* Retrieve the current process LUID */
    Status = GetProcessLuid(NULL, NULL, &CallerLuid);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to retrieve the caller LUID, Status 0x%08lx\n", Status);
        return Status;
    }

    /* Compute the needed string size */
    MemSize = _scwprintf(L"%wZ\\Service-0x%x-%x$",
                         &gustrWindowStationsDir,
                         CallerLuid.HighPart,
                         CallerLuid.LowPart);
    MemSize = MemSize * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    if (MemSize > MAXUSHORT)
    {
        ERR("Window station name length is too long.\n");
        return STATUS_NAME_TOO_LONG;
    }
    StrSize = (USHORT)MemSize;

    /*
     * Check whether it's short enough so that we can use the static buffer
     * in the TEB. Otherwise continue with virtual memory allocation.
     */
    Teb = NtCurrentTeb();
    if (Teb && (StrSize <= sizeof(Teb->StaticUnicodeBuffer)))
    {
        /* We can use the TEB's static unicode string */
        ASSERT(Teb->StaticUnicodeString.Buffer == Teb->StaticUnicodeBuffer);
        ASSERT(Teb->StaticUnicodeString.MaximumLength == sizeof(Teb->StaticUnicodeBuffer));

        /* Remember the TEB's static unicode string address for later */
        *TebStaticUnicodeString = &Teb->StaticUnicodeString;

        *WindowStationName = *TebStaticUnicodeString;
        (*WindowStationName)->Length = 0;
    }
    else
    {
        /* The TEB's static unicode string is too small, allocate some user-mode virtual memory */
        MemSize += ALIGN_UP(sizeof(UNICODE_STRING), sizeof(PVOID));

        /* Allocate the memory in user-mode */
        Status = ZwAllocateVirtualMemory(ZwCurrentProcess(),
                                         (PVOID*)WindowStationName,
                                         0,
                                         &MemSize,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            ERR("ZwAllocateVirtualMemory() failed, Status 0x%08lx\n", Status);
            return Status;
        }

        RtlInitEmptyUnicodeString(*WindowStationName,
                                  (PWCHAR)((ULONG_PTR)*WindowStationName +
                                      ALIGN_UP(sizeof(UNICODE_STRING), sizeof(PVOID))),
                                  StrSize);
    }

    /* Build a valid window station name from the LUID */
    Status = RtlStringCbPrintfW((*WindowStationName)->Buffer,
                                (*WindowStationName)->MaximumLength,
                                L"%wZ\\Service-0x%x-%x$",
                                &gustrWindowStationsDir,
                                CallerLuid.HighPart,
                                CallerLuid.LowPart);
    if (!NT_SUCCESS(Status))
    {
        ERR("Impossible to build a valid window station name, Status 0x%08lx\n", Status);
        goto Quit;
    }
    (*WindowStationName)->Length = (USHORT)(wcslen((*WindowStationName)->Buffer) * sizeof(WCHAR));

    /* Try to update the user's UserModeObjectAttributes */
    _SEH2_TRY
    {
        ProbeForWrite(UserModeObjectAttributes, sizeof(OBJECT_ATTRIBUTES), sizeof(ULONG));
        *LocalObjectAttributes = *UserModeObjectAttributes;

        UserModeObjectAttributes->ObjectName = *WindowStationName;
        UserModeObjectAttributes->RootDirectory = NULL;

        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

Quit:
    if (!NT_SUCCESS(Status))
    {
        /* Release the window station name */
        FreeUserModeWindowStationName(*WindowStationName,
                                      *TebStaticUnicodeString,
                                      NULL, NULL);
    }

    return Status;
}

HWINSTA
APIENTRY
NtUserCreateWindowStation(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK dwDesiredAccess,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4,
    DWORD Unknown5,
    DWORD Unknown6)
{
    NTSTATUS Status = STATUS_SUCCESS;
    HWINSTA hWinSta = NULL;
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    PUNICODE_STRING WindowStationName = NULL;
    PUNICODE_STRING TebStaticUnicodeString = NULL;
    KPROCESSOR_MODE OwnerMode = UserMode;

    TRACE("NtUserCreateWindowStation called\n");

    /* Capture the object attributes and the window station name */
    _SEH2_TRY
    {
        ProbeForRead(ObjectAttributes, sizeof(OBJECT_ATTRIBUTES), sizeof(ULONG));
        LocalObjectAttributes = *ObjectAttributes;
        if (LocalObjectAttributes.Length != sizeof(OBJECT_ATTRIBUTES))
        {
            ERR("Invalid ObjectAttributes length!\n");
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        /*
         * Check whether the caller provided a window station name together
         * with a RootDirectory handle.
         *
         * If the caller did not provide a window station name, build a new one
         * based on the logon session identifier for the calling process.
         * The new name is allocated in user-mode, as the rest of ObjectAttributes
         * already is, so that the validation performed by the Object Manager
         * can be done adequately.
         */
        if ((LocalObjectAttributes.ObjectName == NULL ||
             LocalObjectAttributes.ObjectName->Buffer == NULL ||
             LocalObjectAttributes.ObjectName->Length == 0 ||
             LocalObjectAttributes.ObjectName->Buffer[0] == UNICODE_NULL)
            /* &&
            LocalObjectAttributes.RootDirectory == NULL */)
        {
            /* No, build the new window station name */
            Status = BuildUserModeWindowStationName(ObjectAttributes,
                                                    &LocalObjectAttributes,
                                                    &WindowStationName,
                                                    &TebStaticUnicodeString);
            if (!NT_SUCCESS(Status))
            {
                ERR("BuildUserModeWindowStationName() failed, Status 0x%08lx\n", Status);
                _SEH2_LEAVE;
            }
            OwnerMode = KernelMode;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status =_SEH2_GetExceptionCode();
        ERR("ObjectAttributes capture failed, Status 0x%08lx\n", Status);
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return NULL;
    }

    UserEnterExclusive();

    /* Create the window station */
    Status = IntCreateWindowStation(&hWinSta,
                                    ObjectAttributes,
                                    UserMode,
                                    OwnerMode,
                                    dwDesiredAccess,
                                    Unknown2,
                                    Unknown3,
                                    Unknown4,
                                    Unknown5,
                                    Unknown6);
    UserLeave();

    if (NT_SUCCESS(Status))
    {
        TRACE("NtUserCreateWindowStation created window station '%wZ' with handle 0x%p\n",
              ObjectAttributes->ObjectName, hWinSta);
    }
    else
    {
        ASSERT(hWinSta == NULL);
        ERR("NtUserCreateWindowStation failed to create window station '%wZ', Status 0x%08lx\n",
            ObjectAttributes->ObjectName, Status);
    }

    /* Try to restore the user's ObjectAttributes and release the window station name */
    FreeUserModeWindowStationName(WindowStationName,
                                  TebStaticUnicodeString,
                                  (OwnerMode == KernelMode ? ObjectAttributes : NULL),
                                  &LocalObjectAttributes);

    if (!NT_SUCCESS(Status))
    {
        ASSERT(hWinSta == NULL);
        SetLastNtError(Status);
    }

    return hWinSta;
}

/*
 * NtUserOpenWindowStation
 *
 * Opens an existing window station.
 *
 * Parameters
 *    lpszWindowStationName
 *       Name of the existing window station.
 *
 *    dwDesiredAccess
 *       Requested type of access.
 *
 * Return Value
 *    If the function succeeds, the return value is the handle to the
 *    specified window station. If the function fails, the return value
 *    is NULL.
 *
 * Remarks
 *    The returned handle can be closed with NtUserCloseWindowStation.
 *
 * Status
 *    @implemented
 */

HWINSTA
APIENTRY
NtUserOpenWindowStation(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK dwDesiredAccess)
{
    NTSTATUS Status = STATUS_SUCCESS;
    HWINSTA hWinSta = NULL;
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    PUNICODE_STRING WindowStationName = NULL;
    PUNICODE_STRING TebStaticUnicodeString = NULL;
    KPROCESSOR_MODE OwnerMode = UserMode;

    TRACE("NtUserOpenWindowStation called\n");

    /* Capture the object attributes and the window station name */
    _SEH2_TRY
    {
        ProbeForRead(ObjectAttributes, sizeof(OBJECT_ATTRIBUTES), sizeof(ULONG));
        LocalObjectAttributes = *ObjectAttributes;
        if (LocalObjectAttributes.Length != sizeof(OBJECT_ATTRIBUTES))
        {
            ERR("Invalid ObjectAttributes length!\n");
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        /*
         * Check whether the caller did not provide a window station name,
         * or provided the special "Service-0x00000000-00000000$" name.
         *
         * NOTE: On Windows, the special "Service-0x00000000-00000000$" string
         * is used instead of an empty name (observed when API-monitoring
         * OpenWindowStation() called with an empty window station name).
         */
        if ((LocalObjectAttributes.ObjectName == NULL ||
             LocalObjectAttributes.ObjectName->Buffer == NULL ||
             LocalObjectAttributes.ObjectName->Length == 0 ||
             LocalObjectAttributes.ObjectName->Buffer[0] == UNICODE_NULL)
            /* &&
            LocalObjectAttributes.RootDirectory == NULL */)
        {
            /* No, remember that for later */
            LocalObjectAttributes.ObjectName = NULL;
        }
        if (LocalObjectAttributes.ObjectName &&
            LocalObjectAttributes.ObjectName->Length ==
                sizeof(L"Service-0x00000000-00000000$") - sizeof(UNICODE_NULL) &&
            _wcsnicmp(LocalObjectAttributes.ObjectName->Buffer,
                      L"Service-0x00000000-00000000$",
                      LocalObjectAttributes.ObjectName->Length / sizeof(WCHAR)) == 0)
        {
            /* No, remember that for later */
            LocalObjectAttributes.ObjectName = NULL;
        }

        /*
         * If the caller did not provide a window station name, build a new one
         * based on the logon session identifier for the calling process.
         * The new name is allocated in user-mode, as the rest of ObjectAttributes
         * already is, so that the validation performed by the Object Manager
         * can be done adequately.
         */
        if (!LocalObjectAttributes.ObjectName)
        {
            /* No, build the new window station name */
            Status = BuildUserModeWindowStationName(ObjectAttributes,
                                                    &LocalObjectAttributes,
                                                    &WindowStationName,
                                                    &TebStaticUnicodeString);
            if (!NT_SUCCESS(Status))
            {
                ERR("BuildUserModeWindowStationName() failed, Status 0x%08lx\n", Status);
                _SEH2_LEAVE;
            }
            OwnerMode = KernelMode;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status =_SEH2_GetExceptionCode();
        ERR("ObjectAttributes capture failed, Status 0x%08lx\n", Status);
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return NULL;
    }

    /* Open the window station */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ExWindowStationObjectType,
                                UserMode,
                                NULL,
                                dwDesiredAccess,
                                NULL,
                                (PVOID*)&hWinSta);
    if (NT_SUCCESS(Status))
    {
        TRACE("NtUserOpenWindowStation opened window station '%wZ' with handle 0x%p\n",
              ObjectAttributes->ObjectName, hWinSta);
    }
    else
    {
        ASSERT(hWinSta == NULL);
        ERR("NtUserOpenWindowStation failed to open window station '%wZ', Status 0x%08lx\n",
            ObjectAttributes->ObjectName, Status);
    }

    /* Try to restore the user's ObjectAttributes and release the window station name */
    FreeUserModeWindowStationName(WindowStationName,
                                  TebStaticUnicodeString,
                                  (OwnerMode == KernelMode ? ObjectAttributes : NULL),
                                  &LocalObjectAttributes);

    if (!NT_SUCCESS(Status))
    {
        ASSERT(hWinSta == NULL);
        SetLastNtError(Status);
    }

    return hWinSta;
}

/*
 * NtUserCloseWindowStation
 *
 * Closes a window station handle.
 *
 * Parameters
 *    hWinSta
 *       Handle to the window station.
 *
 * Return Value
 *    Status
 *
 * Remarks
 *    The window station handle can be created with NtUserCreateWindowStation
 *    or NtUserOpenWindowStation. Attempts to close a handle to the window
 *    station assigned to the calling process will fail.
 *
 * Status
 *    @implemented
 */

BOOL
APIENTRY
NtUserCloseWindowStation(
    HWINSTA hWinSta)
{
    PWINSTATION_OBJECT Object;
    NTSTATUS Status;

    TRACE("NtUserCloseWindowStation called (%p)\n", hWinSta);

    if (hWinSta == UserGetProcessWindowStation())
    {
        ERR("Attempted to close process window station\n");
        return FALSE;
    }

    Status = IntValidateWindowStationHandle(hWinSta,
                                            UserMode,
                                            0,
                                            &Object,
                                            NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Validation of window station handle (%p) failed\n", hWinSta);
        return FALSE;
    }

    ObDereferenceObject(Object);

    TRACE("Closing window station handle (%p)\n", hWinSta);

    Status = ObCloseHandle(hWinSta, UserMode);
    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * NtUserGetObjectInformation
 *
 * The NtUserGetObjectInformation function retrieves information about a
 * window station or desktop object.
 *
 * Parameters
 *    hObj
 *       Handle to the window station or desktop object for which to
 *       return information. This can be a handle of type HDESK or HWINSTA
 *       (for example, a handle returned by NtUserCreateWindowStation,
 *       NtUserOpenWindowStation, NtUserCreateDesktop, or NtUserOpenDesktop).
 *
 *    nIndex
 *       Specifies the object information to be retrieved.
 *
 *    pvInfo
 *       Pointer to a buffer to receive the object information.
 *
 *    nLength
 *       Specifies the size, in bytes, of the buffer pointed to by the
 *       pvInfo parameter.
 *
 *    lpnLengthNeeded
 *       Pointer to a variable receiving the number of bytes required to
 *       store the requested information. If this variable's value is
 *       greater than the value of the nLength parameter when the function
 *       returns, the function returns FALSE, and none of the information
 *       is copied to the pvInfo buffer. If the value of the variable pointed
 *       to by lpnLengthNeeded is less than or equal to the value of nLength,
 *       the entire information block is copied.
 *
 * Return Value
 *    If the function succeeds, the return value is nonzero. If the function
 *    fails, the return value is zero.
 *
 * Status
 *    @unimplemented
 */

BOOL APIENTRY
NtUserGetObjectInformation(
    HANDLE hObject,
    DWORD nIndex,
    PVOID pvInformation,
    DWORD nLength,
    PDWORD nLengthNeeded)
{
    NTSTATUS Status;
    PWINSTATION_OBJECT WinStaObject = NULL;
    PDESKTOP DesktopObject = NULL;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;
    OBJECT_HANDLE_INFORMATION HandleInfo;
    USEROBJECTFLAGS ObjectFlags;
    PUNICODE_STRING pStrNameU = NULL;
    PVOID pvData = NULL;
    SIZE_T nDataSize = 0;

    _SEH2_TRY
    {
        if (nLengthNeeded)
            ProbeForWrite(nLengthNeeded, sizeof(*nLengthNeeded), 1);
        ProbeForWrite(pvInformation, nLength, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        return FALSE;
    }
    _SEH2_END;

    /* Try window station */
    TRACE("Trying to open window station 0x%p\n", hObject);
    Status = ObReferenceObjectByHandle(hObject,
                                       0,
                                       ExWindowStationObjectType,
                                       UserMode,
                                       (PVOID*)&WinStaObject,
                                       &HandleInfo);

    if (Status == STATUS_OBJECT_TYPE_MISMATCH)
    {
        /* Try desktop */
        TRACE("Trying to open desktop %p\n", hObject);
        WinStaObject = NULL;
        Status = IntValidateDesktopHandle(hObject,
                                          UserMode,
                                          0,
                                          &DesktopObject);
    }

    if (!NT_SUCCESS(Status))
    {
        ERR("Failed: 0x%x\n", Status);
        goto Exit;
    }

    TRACE("WinSta or Desktop opened!\n");

    /* Get data */
    switch (nIndex)
    {
        case UOI_FLAGS:
        {
            ObjectFlags.fReserved = FALSE;
            ObjectFlags.fInherit = !!(HandleInfo.HandleAttributes & OBJ_INHERIT);

            ObjectFlags.dwFlags = 0;
            if (WinStaObject != NULL)
            {
                if (!(WinStaObject->Flags & WSS_NOIO))
                    ObjectFlags.dwFlags |= WSF_VISIBLE;
            }
            else if (DesktopObject != NULL)
            {
                FIXME("Setting DF_ALLOWOTHERACCOUNTHOOK is unimplemented.\n");
            }
            else
            {
                ERR("No associated WinStaObject nor DesktopObject!\n");
            }

            pvData = &ObjectFlags;
            nDataSize = sizeof(ObjectFlags);
            Status = STATUS_SUCCESS;
            break;
        }

        case UOI_NAME:
        {
            if (WinStaObject != NULL)
            {
                ObjectHeader = OBJECT_TO_OBJECT_HEADER(WinStaObject);
                NameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);

                if (NameInfo && (NameInfo->Name.Length > 0))
                {
                    /* Named window station */
                    pStrNameU = &NameInfo->Name;
                    nDataSize = pStrNameU->Length + sizeof(UNICODE_NULL);
                }
                else
                {
                    /* Unnamed window station (should never happen!) */
                    ASSERT(FALSE);
                    pStrNameU = NULL;
                    nDataSize = sizeof(UNICODE_NULL);
                }
                Status = STATUS_SUCCESS;
            }
            else if (DesktopObject != NULL)
            {
                pvData = DesktopObject->pDeskInfo->szDesktopName;
                nDataSize = (wcslen(DesktopObject->pDeskInfo->szDesktopName) + 1) * sizeof(WCHAR);
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;
        }

        case UOI_TYPE:
        {
            if (WinStaObject != NULL)
            {
                ObjectHeader = OBJECT_TO_OBJECT_HEADER(WinStaObject);
                pStrNameU = &ObjectHeader->Type->Name;
                nDataSize = pStrNameU->Length + sizeof(UNICODE_NULL);
                Status = STATUS_SUCCESS;
            }
            else if (DesktopObject != NULL)
            {
                ObjectHeader = OBJECT_TO_OBJECT_HEADER(DesktopObject);
                pStrNameU = &ObjectHeader->Type->Name;
                nDataSize = pStrNameU->Length + sizeof(UNICODE_NULL);
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;
        }

        case UOI_USER_SID:
            Status = STATUS_NOT_IMPLEMENTED;
            ERR("UOI_USER_SID unimplemented!\n");
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

Exit:
    if ((Status == STATUS_SUCCESS) && (nLength < nDataSize))
        Status = STATUS_BUFFER_TOO_SMALL;

    _SEH2_TRY
    {
        if (nLengthNeeded)
            *nLengthNeeded = nDataSize;

        /* Try to copy data to caller */
        if (Status == STATUS_SUCCESS && (nDataSize > 0))
        {
            TRACE("Trying to copy data to caller (len = %lu, len needed = %lu)\n", nLength, nDataSize);
            if (pvData)
            {
                /* Copy the data */
                RtlCopyMemory(pvInformation, pvData, nDataSize);
            }
            else if (pStrNameU)
            {
                /* Copy and NULL-terminate the string */
                RtlCopyMemory(pvInformation, pStrNameU->Buffer, pStrNameU->Length);
                ((PWCHAR)pvInformation)[pStrNameU->Length / sizeof(WCHAR)] = UNICODE_NULL;
            }
            else
            {
                /* Zero the memory */
                RtlZeroMemory(pvInformation, nDataSize);
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Release objects */
    if (DesktopObject != NULL)
        ObDereferenceObject(DesktopObject);
    if (WinStaObject != NULL)
        ObDereferenceObject(WinStaObject);

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * NtUserSetObjectInformation
 *
 * The NtUserSetObjectInformation function sets information about a
 * window station or desktop object.
 *
 * Parameters
 *    hObj
 *       Handle to the window station or desktop object for which to set
 *       object information. This value can be a handle of type HDESK or
 *       HWINSTA.
 *
 *    nIndex
 *       Specifies the object information to be set.
 *
 *    pvInfo
 *       Pointer to a buffer containing the object information.
 *
 *    nLength
 *       Specifies the size, in bytes, of the information contained in the
 *       buffer pointed to by pvInfo.
 *
 * Return Value
 *    If the function succeeds, the return value is nonzero. If the function
 *    fails the return value is zero.
 *
 * Status
 *    @unimplemented
 */

BOOL
APIENTRY
NtUserSetObjectInformation(
    HANDLE hObject,
    DWORD nIndex,
    PVOID pvInformation,
    DWORD nLength)
{
    /* FIXME: ZwQueryObject */
    /* FIXME: ZwSetInformationObject */
    SetLastNtError(STATUS_UNSUCCESSFUL);
    return FALSE;
}


HWINSTA FASTCALL
UserGetProcessWindowStation(VOID)
{
    PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();

    return ppi->hwinsta;
}


/*
 * NtUserGetProcessWindowStation
 *
 * Returns a handle to the current process window station.
 *
 * Return Value
 *    If the function succeeds, the return value is handle to the window
 *    station assigned to the current process. If the function fails, the
 *    return value is NULL.
 *
 * Status
 *    @implemented
 */

HWINSTA APIENTRY
NtUserGetProcessWindowStation(VOID)
{
    return UserGetProcessWindowStation();
}

BOOL FASTCALL
UserSetProcessWindowStation(HWINSTA hWindowStation)
{
    NTSTATUS Status;
    PPROCESSINFO ppi;
    OBJECT_HANDLE_INFORMATION ObjectHandleInfo;
    PWINSTATION_OBJECT NewWinSta = NULL, OldWinSta;
    HWINSTA hCacheWinSta;

    ppi = PsGetCurrentProcessWin32Process();

    /* Reference the new window station */
    if (hWindowStation != NULL)
    {
        Status = IntValidateWindowStationHandle(hWindowStation,
                                                UserMode,
                                                0,
                                                &NewWinSta,
                                                &ObjectHandleInfo);
        if (!NT_SUCCESS(Status))
        {
            TRACE("Validation of window station handle 0x%p failed\n", hWindowStation);
            SetLastNtError(Status);
            return FALSE;
        }
    }

    OldWinSta = ppi->prpwinsta;
    hCacheWinSta = PsGetProcessWin32WindowStation(ppi->peProcess);

    /* Dereference the previous window station */
    if (OldWinSta != NULL)
    {
        ObDereferenceObject(OldWinSta);
    }

    /*
     * FIXME: Don't allow changing the window station if there are threads that are attached to desktops and own GUI objects?
     */

    /* Close the cached EPROCESS window station handle if needed */
    if (hCacheWinSta != NULL)
    {
        /* Reference the window station */
        Status = ObReferenceObjectByHandle(hCacheWinSta,
                                           0,
                                           ExWindowStationObjectType,
                                           UserMode,
                                           (PVOID*)&OldWinSta,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to reference the inherited window station, Status 0x%08lx\n", Status);
            /* We failed, reset the cache */
            hCacheWinSta = NULL;
            PsSetProcessWindowStation(ppi->peProcess, hCacheWinSta);
        }
        else
        {
            /*
             * Close the old handle and reset the cache only
             * if we are setting a different window station.
             */
            if (NewWinSta != OldWinSta)
            {
                ObCloseHandle(hCacheWinSta, UserMode);
                hCacheWinSta = NULL;
                PsSetProcessWindowStation(ppi->peProcess, hCacheWinSta);
            }

            /* Dereference the window station */
            ObDereferenceObject(OldWinSta);
        }
    }

    /* Duplicate and save a new cached EPROCESS window station handle */
    if ((hCacheWinSta == NULL) && (hWindowStation != NULL))
    {
        Status = ZwDuplicateObject(ZwCurrentProcess(),
                                   hWindowStation,
                                   ZwCurrentProcess(),
                                   (PHANDLE)&hCacheWinSta,
                                   0,
                                   0,
                                   DUPLICATE_SAME_ACCESS);
        if (!NT_SUCCESS(Status))
        {
            ERR("UserSetProcessWindowStation: Failed to duplicate the window station handle, Status 0x%08lx\n", Status);
        }
        else
        {
            PsSetProcessWindowStation(ppi->peProcess, hCacheWinSta);
        }
    }

    ppi->prpwinsta = NewWinSta;
    ppi->hwinsta = hWindowStation;
    ppi->amwinsta = hWindowStation != NULL ? ObjectHandleInfo.GrantedAccess : 0;
    TRACE("WS : Granted Access 0x%08lx\n",ppi->amwinsta);

    if (RtlAreAllAccessesGranted(ppi->amwinsta, WINSTA_READSCREEN))
    {
        ppi->W32PF_flags |= W32PF_READSCREENACCESSGRANTED;
    }
    else
    {
        ppi->W32PF_flags &= ~W32PF_READSCREENACCESSGRANTED;
    }

    if (NewWinSta && !(NewWinSta->Flags & WSS_NOIO))
    {
        ppi->W32PF_flags |= W32PF_IOWINSTA;
    }
    else /* Might be closed if the handle is NULL */
    {
        ppi->W32PF_flags &= ~W32PF_IOWINSTA;
    }
    return TRUE;
}

/*
 * NtUserSetProcessWindowStation
 *
 * Assigns a window station to the current process.
 *
 * Parameters
 *    hWinSta
 *       Handle to the window station.
 *
 * Return Value
 *    Status
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
NtUserSetProcessWindowStation(HWINSTA hWindowStation)
{
    BOOL ret;

    UserEnterExclusive();

    ret = UserSetProcessWindowStation(hWindowStation);

    UserLeave();

    return ret;
}

/*
 * NtUserLockWindowStation
 *
 * Locks switching desktops. Only the logon application is allowed to call this function.
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
NtUserLockWindowStation(HWINSTA hWindowStation)
{
    PWINSTATION_OBJECT Object;
    NTSTATUS Status;

    TRACE("About to set process window station with handle (%p)\n",
          hWindowStation);

    if (gpidLogon != PsGetCurrentProcessId())
    {
        ERR("Unauthorized process attempted to lock the window station!\n");
        EngSetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    Status = IntValidateWindowStationHandle(hWindowStation,
                                            UserMode,
                                            0,
                                            &Object,
                                            NULL);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Validation of window station handle (%p) failed\n",
              hWindowStation);
        SetLastNtError(Status);
        return FALSE;
    }

    Object->Flags |= WSS_LOCKED;

    ObDereferenceObject(Object);
    return TRUE;
}

/*
 * NtUserUnlockWindowStation
 *
 * Unlocks switching desktops. Only the logon application is allowed to call this function.
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
NtUserUnlockWindowStation(HWINSTA hWindowStation)
{
    PWINSTATION_OBJECT Object;
    NTSTATUS Status;
    BOOL Ret;

    TRACE("About to set process window station with handle (%p)\n",
          hWindowStation);

    if (gpidLogon != PsGetCurrentProcessId())
    {
        ERR("Unauthorized process attempted to unlock the window station!\n");
        EngSetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    Status = IntValidateWindowStationHandle(hWindowStation,
                                            UserMode,
                                            0,
                                            &Object,
                                            NULL);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Validation of window station handle (%p) failed\n",
              hWindowStation);
        SetLastNtError(Status);
        return FALSE;
    }

    Ret = (Object->Flags & WSS_LOCKED) == WSS_LOCKED;
    Object->Flags &= ~WSS_LOCKED;

    ObDereferenceObject(Object);
    return Ret;
}

static NTSTATUS FASTCALL
BuildWindowStationNameList(
    ULONG dwSize,
    PVOID lpBuffer,
    PULONG pRequiredSize)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE DirectoryHandle;
    char InitialBuffer[256], *Buffer;
    ULONG Context, ReturnLength, BufferSize;
    DWORD EntryCount;
    POBJECT_DIRECTORY_INFORMATION DirEntry;
    WCHAR NullWchar;

    //
    // FIXME: Fully wrong! Since, by calling NtUserCreateWindowStation
    // with judicious parameters one can create window stations elsewhere
    // than in Windows\WindowStations directory, Win32k definitely MUST
    // maintain a list of window stations it has created, and not rely
    // on the enumeration of Windows\WindowStations !!!
    //

    /*
     * Try to open the directory.
     */
    InitializeObjectAttributes(&ObjectAttributes,
                               &gustrWindowStationsDir,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwOpenDirectoryObject(&DirectoryHandle,
                                   DIRECTORY_QUERY,
                                   &ObjectAttributes);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* First try to query the directory using a fixed-size buffer */
    Context = 0;
    Buffer = NULL;
    Status = ZwQueryDirectoryObject(DirectoryHandle,
                                    InitialBuffer,
                                    sizeof(InitialBuffer),
                                    FALSE,
                                    TRUE,
                                    &Context,
                                    &ReturnLength);
    if (NT_SUCCESS(Status))
    {
        if (STATUS_NO_MORE_ENTRIES == ZwQueryDirectoryObject(DirectoryHandle, NULL, 0, FALSE,
                                                             FALSE, &Context, NULL))
        {
            /* Our fixed-size buffer is large enough */
            Buffer = InitialBuffer;
        }
    }

    if (NULL == Buffer)
    {
        /* Need a larger buffer, check how large exactly */
        Status = ZwQueryDirectoryObject(DirectoryHandle, NULL, 0, FALSE, TRUE, &Context,
                                        &ReturnLength);
        if (!NT_SUCCESS(Status))
        {
            ERR("ZwQueryDirectoryObject failed\n");
            ZwClose(DirectoryHandle);
            return Status;
        }

        BufferSize = ReturnLength;
        Buffer = ExAllocatePoolWithTag(PagedPool, BufferSize, TAG_WINSTA);
        if (NULL == Buffer)
        {
            ZwClose(DirectoryHandle);
            return STATUS_NO_MEMORY;
        }

        /* We should have a sufficiently large buffer now */
        Context = 0;
        Status = ZwQueryDirectoryObject(DirectoryHandle, Buffer, BufferSize,
                                        FALSE, TRUE, &Context, &ReturnLength);
        if (! NT_SUCCESS(Status) ||
              STATUS_NO_MORE_ENTRIES != ZwQueryDirectoryObject(DirectoryHandle, NULL, 0, FALSE,
                                                               FALSE, &Context, NULL))
        {
            /* Something went wrong, maybe someone added a directory entry? Just give up. */
            ExFreePoolWithTag(Buffer, TAG_WINSTA);
            ZwClose(DirectoryHandle);
            return NT_SUCCESS(Status) ? STATUS_INTERNAL_ERROR : Status;
        }
    }

    ZwClose(DirectoryHandle);

    /*
     * Count the required size of buffer.
     */
    ReturnLength = sizeof(DWORD);
    EntryCount = 0;
    for (DirEntry = (POBJECT_DIRECTORY_INFORMATION) Buffer;
         0 != DirEntry->Name.Length;
         DirEntry++)
    {
        ReturnLength += DirEntry->Name.Length + sizeof(WCHAR);
        EntryCount++;
    }
    TRACE("Required size: %lu Entry count: %lu\n", ReturnLength, EntryCount);
    if (NULL != pRequiredSize)
    {
        Status = MmCopyToCaller(pRequiredSize, &ReturnLength, sizeof(ULONG));
        if (! NT_SUCCESS(Status))
        {
            if (Buffer != InitialBuffer)
            {
                ExFreePoolWithTag(Buffer, TAG_WINSTA);
            }
            return STATUS_BUFFER_TOO_SMALL;
        }
    }

    /*
     * Check if the supplied buffer is large enough.
     */
    if (dwSize < ReturnLength)
    {
        if (Buffer != InitialBuffer)
        {
            ExFreePoolWithTag(Buffer, TAG_WINSTA);
        }
        return STATUS_BUFFER_TOO_SMALL;
    }

    /*
     * Generate the resulting buffer contents.
     */
    Status = MmCopyToCaller(lpBuffer, &EntryCount, sizeof(DWORD));
    if (! NT_SUCCESS(Status))
    {
        if (Buffer != InitialBuffer)
        {
            ExFreePoolWithTag(Buffer, TAG_WINSTA);
        }
        return Status;
    }
    lpBuffer = (PVOID) ((PCHAR) lpBuffer + sizeof(DWORD));

    NullWchar = L'\0';
    for (DirEntry = (POBJECT_DIRECTORY_INFORMATION) Buffer;
         0 != DirEntry->Name.Length;
         DirEntry++)
    {
        Status = MmCopyToCaller(lpBuffer, DirEntry->Name.Buffer, DirEntry->Name.Length);
        if (! NT_SUCCESS(Status))
        {
            if (Buffer != InitialBuffer)
            {
                ExFreePoolWithTag(Buffer, TAG_WINSTA);
            }
            return Status;
        }
        lpBuffer = (PVOID) ((PCHAR) lpBuffer + DirEntry->Name.Length);
        Status = MmCopyToCaller(lpBuffer, &NullWchar, sizeof(WCHAR));
        if (! NT_SUCCESS(Status))
        {
            if (Buffer != InitialBuffer)
            {
                ExFreePoolWithTag(Buffer, TAG_WINSTA);
            }
            return Status;
        }
        lpBuffer = (PVOID) ((PCHAR) lpBuffer + sizeof(WCHAR));
    }

    /*
     * Clean up
     */
    if (Buffer != InitialBuffer)
    {
        ExFreePoolWithTag(Buffer, TAG_WINSTA);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS FASTCALL
BuildDesktopNameList(
    HWINSTA hWindowStation,
    ULONG dwSize,
    PVOID lpBuffer,
    PULONG pRequiredSize)
{
    NTSTATUS Status;
    PWINSTATION_OBJECT WindowStation;
    PLIST_ENTRY DesktopEntry;
    PDESKTOP DesktopObject;
    DWORD EntryCount;
    ULONG ReturnLength;
    WCHAR NullWchar;
    UNICODE_STRING DesktopName;

    Status = IntValidateWindowStationHandle(hWindowStation,
                                            UserMode,
                                            0,
                                            &WindowStation,
                                            NULL);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    /*
     * Count the required size of buffer.
     */
    ReturnLength = sizeof(DWORD);
    EntryCount = 0;
    for (DesktopEntry = WindowStation->DesktopListHead.Flink;
         DesktopEntry != &WindowStation->DesktopListHead;
         DesktopEntry = DesktopEntry->Flink)
    {
        DesktopObject = CONTAINING_RECORD(DesktopEntry, DESKTOP, ListEntry);
        RtlInitUnicodeString(&DesktopName, DesktopObject->pDeskInfo->szDesktopName);
        ReturnLength += DesktopName.Length + sizeof(WCHAR);
        EntryCount++;
    }
    TRACE("Required size: %lu Entry count: %lu\n", ReturnLength, EntryCount);
    if (NULL != pRequiredSize)
    {
        Status = MmCopyToCaller(pRequiredSize, &ReturnLength, sizeof(ULONG));
        if (! NT_SUCCESS(Status))
        {
            ObDereferenceObject(WindowStation);
            return STATUS_BUFFER_TOO_SMALL;
        }
    }

    /*
     * Check if the supplied buffer is large enough.
     */
    if (dwSize < ReturnLength)
    {
        ObDereferenceObject(WindowStation);
        return STATUS_BUFFER_TOO_SMALL;
    }

    /*
     * Generate the resulting buffer contents.
     */
    Status = MmCopyToCaller(lpBuffer, &EntryCount, sizeof(DWORD));
    if (! NT_SUCCESS(Status))
    {
        ObDereferenceObject(WindowStation);
        return Status;
    }
    lpBuffer = (PVOID) ((PCHAR) lpBuffer + sizeof(DWORD));

    NullWchar = L'\0';
    for (DesktopEntry = WindowStation->DesktopListHead.Flink;
         DesktopEntry != &WindowStation->DesktopListHead;
         DesktopEntry = DesktopEntry->Flink)
    {
        DesktopObject = CONTAINING_RECORD(DesktopEntry, DESKTOP, ListEntry);
        RtlInitUnicodeString(&DesktopName, DesktopObject->pDeskInfo->szDesktopName);
        Status = MmCopyToCaller(lpBuffer, DesktopName.Buffer, DesktopName.Length);
        if (! NT_SUCCESS(Status))
        {
            ObDereferenceObject(WindowStation);
            return Status;
        }
        lpBuffer = (PVOID) ((PCHAR)lpBuffer + DesktopName.Length);
        Status = MmCopyToCaller(lpBuffer, &NullWchar, sizeof(WCHAR));
        if (! NT_SUCCESS(Status))
        {
            ObDereferenceObject(WindowStation);
            return Status;
        }
        lpBuffer = (PVOID) ((PCHAR) lpBuffer + sizeof(WCHAR));
    }

    /*
     * Clean up and return
     */
    ObDereferenceObject(WindowStation);
    return STATUS_SUCCESS;
}

/*
 * NtUserBuildNameList
 *
 * Function used for enumeration of desktops or window stations.
 *
 * Parameters
 *    hWinSta
 *       For enumeration of window stations this parameter must be set to
 *       zero. Otherwise it's handle for window station.
 *
 *    dwSize
 *       Size of buffer passed by caller.
 *
 *    lpBuffer
 *       Buffer passed by caller. If the function succeeds, the buffer is
 *       filled with window station/desktop count (in first DWORD) and
 *       NULL-terminated window station/desktop names.
 *
 *    pRequiredSize
 *       If the function succeeds, this is the number of bytes copied.
 *       Otherwise it's size of buffer needed for function to succeed.
 *
 * Status
 *    @implemented
 */

NTSTATUS APIENTRY
NtUserBuildNameList(
    HWINSTA hWindowStation,
    ULONG dwSize,
    PVOID lpBuffer,
    PULONG pRequiredSize)
{
    /* The WindowStation name list and desktop name list are build in completely
       different ways. Call the appropriate function */
    return NULL == hWindowStation ? BuildWindowStationNameList(dwSize, lpBuffer, pRequiredSize) :
           BuildDesktopNameList(hWindowStation, dwSize, lpBuffer, pRequiredSize);
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetLogonNotifyWindow(HWND hWnd)
{
    if (gpidLogon != PsGetCurrentProcessId())
    {
        return FALSE;
    }

    if (!IntIsWindow(hWnd))
    {
        return FALSE;
    }

    hwndSAS = hWnd;

    return TRUE;
}

BOOL
APIENTRY
NtUserLockWorkStation(VOID)
{
    BOOL ret;
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

    UserEnterExclusive();

    if (pti->rpdesk == IntGetActiveDesktop())
    {
        ret = UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_LOCK_WORKSTATION, 0);
    }
    else
    {
        ret = FALSE;
    }

    UserLeave();

    return ret;
}

BOOL
NTAPI
NtUserSetWindowStationUser(
    IN HWINSTA hWindowStation,
    IN PLUID pluid,
    IN PSID psid OPTIONAL,
    IN DWORD size)
{
    BOOL Ret = FALSE;
    NTSTATUS Status;
    PWINSTATION_OBJECT WindowStation = NULL;
    LUID luidUser;

    UserEnterExclusive();

    if (gpidLogon != PsGetCurrentProcessId())
    {
        EngSetLastError(ERROR_ACCESS_DENIED);
        goto Leave;
    }

    /* Validate the window station */
    Status = IntValidateWindowStationHandle(hWindowStation,
                                            UserMode,
                                            0,
                                            &WindowStation,
                                            NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Leave;
    }

    /* Capture the user LUID */
    _SEH2_TRY
    {
        ProbeForRead(pluid, sizeof(LUID), 1);
        luidUser = *pluid;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        _SEH2_YIELD(goto Leave);
    }
    _SEH2_END;

    /* Reset the window station user LUID */
    RtlZeroMemory(&WindowStation->luidUser, sizeof(LUID));

    /* Reset the window station user SID */
    if (WindowStation->psidUser)
    {
        ExFreePoolWithTag(WindowStation->psidUser, USERTAG_SECURITY);
        WindowStation->psidUser = NULL;
    }

    /* Copy the new user SID if one has been provided */
    if (psid)
    {
        WindowStation->psidUser = ExAllocatePoolWithTag(PagedPool, size, USERTAG_SECURITY);
        if (WindowStation->psidUser == NULL)
        {
            EngSetLastError(ERROR_OUTOFMEMORY);
            goto Leave;
        }

        Status = STATUS_SUCCESS;
        _SEH2_TRY
        {
            ProbeForRead(psid, size, 1);
            RtlCopyMemory(WindowStation->psidUser, psid, size);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(WindowStation->psidUser, USERTAG_SECURITY);
            WindowStation->psidUser = NULL;
            goto Leave;
        }
    }

    /* Copy the new user LUID */
    WindowStation->luidUser = luidUser;

    Ret = TRUE;

Leave:
    if (WindowStation)
        ObDereferenceObject(WindowStation);

    UserLeave();
    return Ret;
}

/* EOF */
