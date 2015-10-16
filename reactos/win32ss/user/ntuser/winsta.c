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

/* Currently active window station */
PWINSTATION_OBJECT InputWindowStation = NULL;

/* Winlogon SAS window */
HWND hwndSAS = NULL;

/* Full path to WindowStations directory */
UNICODE_STRING gustrWindowStationsDir;

/* INITIALIZATION FUNCTIONS ****************************************************/

INIT_FUNCTION
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
    PPEB Peb;
    NTSTATUS Status;
    WCHAR wstrWindowStationsDir[MAX_PATH];
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hWinstaDir;

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
        swprintf(wstrWindowStationsDir,
                 L"%ws\\%lu%ws",
                 SESSION_DIR,
                 Peb->SessionId,
                 WINSTA_OBJ_DIR);

        if (!RtlCreateUnicodeString(&gustrWindowStationsDir, wstrWindowStationsDir))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

   InitializeObjectAttributes(&ObjectAttributes,
                              &gustrWindowStationsDir,
                              0,
                              NULL,
                              NULL);
   Status = ZwCreateDirectoryObject(&hWinstaDir, 0, &ObjectAttributes);
   if (!NT_SUCCESS(Status))
   {
       ERR("Could not create %wZ directory (Status 0x%X)\n", &gustrWindowStationsDir,  Status);
      return Status;
   }

   TRACE("Created directory %wZ for session %lu\n", &gustrWindowStationsDir, Peb->SessionId);

   return Status;
}

/* OBJECT CALLBACKS  **********************************************************/

NTSTATUS
APIENTRY
IntWinStaObjectDelete(
    _In_ PVOID Parameters)
{
    PWIN32_DELETEMETHOD_PARAMETERS DeleteParameters = Parameters;
   PWINSTATION_OBJECT WinSta = (PWINSTATION_OBJECT)DeleteParameters->Object;

   TRACE("Deleting window station (0x%p)\n", WinSta);

   WinSta->Flags |= WSS_DYING;

   UserEmptyClipboardData(WinSta);

   RtlDestroyAtomTable(WinSta->AtomTable);

   RtlFreeUnicodeString(&WinSta->Name);

   return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
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
IntWinstaOkToClose(
    _In_ PVOID Parameters)
{
    PWIN32_OKAYTOCLOSEMETHOD_PARAMETERS OkToCloseParameters = Parameters;
    PPROCESSINFO ppi;

    ppi = PsGetCurrentProcessWin32Process();

    if(ppi && (OkToCloseParameters->Handle == ppi->hwinsta))
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

   Status = ObReferenceObjectByHandle(
               WindowStation,
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

   ScreenDeviceContext = IntGdiCreateDC(&DriverName, NULL, NULL, NULL, FALSE);
   if (NULL == ScreenDeviceContext)
   {
      IntDestroyPrimarySurface();
      return FALSE;
   }
   GreSetDCOwner(ScreenDeviceContext, GDI_OBJ_HMGR_PUBLIC);

   if (! IntCreatePrimarySurface())
   {
      return FALSE;
   }

   hSystemBM = NtGdiCreateCompatibleDC(ScreenDeviceContext);

   NtGdiSelectFont(hSystemBM, NtGdiGetStockObject(SYSTEM_FONT));
   GreSetDCOwner(hSystemBM, GDI_OBJ_HMGR_PUBLIC);

   /* Update the SERVERINFO */
   gpsi->aiSysMet[SM_CXSCREEN] = gppdevPrimary->gdiinfo.ulHorzRes;
   gpsi->aiSysMet[SM_CYSCREEN] = gppdevPrimary->gdiinfo.ulVertRes;
   gpsi->Planes        = NtGdiGetDeviceCaps(ScreenDeviceContext, PLANES);
   gpsi->BitsPixel     = NtGdiGetDeviceCaps(ScreenDeviceContext, BITSPIXEL);
   gpsi->BitCount      = gpsi->Planes * gpsi->BitsPixel;
   gpsi->dmLogPixels   = NtGdiGetDeviceCaps(ScreenDeviceContext, LOGPIXELSY);
   if (NtGdiGetDeviceCaps(ScreenDeviceContext, RASTERCAPS) & RC_PALETTE)
   {
      gpsi->PUSIFlags |= PUSIF_PALETTEDISPLAY;
   }
   else
      gpsi->PUSIFlags &= ~PUSIF_PALETTEDISPLAY;
   // Font is realized and this dc was previously set to internal DC_ATTR.
   gpsi->cxSysFontChar = IntGetCharDimensions(hSystemBM, &tmw, (DWORD*)&gpsi->cySysFontChar);
   gpsi->tmSysFont     = tmw;

   /* Put the pointer in the center of the screen */
   gpsi->ptCursor.x = gpsi->aiSysMet[SM_CXSCREEN] / 2;
   gpsi->ptCursor.y = gpsi->aiSysMet[SM_CYSCREEN] / 2;

   /* Attach monitor */
   UserAttachMonitor((HDEV)gppdevPrimary);

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
 *    Unknown3, Unknown4, Unknown5
 *       Unused
 *
 * Return Value
 *    If the function succeeds, the return value is a handle to the newly
 *    created window station. If the specified window station already
 *    exists, the function succeeds and returns a handle to the existing
 *    window station. If the function fails, the return value is NULL.
 *
 * Todo
 *    Correct the prototype to match the Windows one (with 7 parameters
 *    on Windows XP).
 *
 * Status
 *    @implemented
 */

HWINSTA APIENTRY
NtUserCreateWindowStation(
   POBJECT_ATTRIBUTES ObjectAttributes,
   ACCESS_MASK dwDesiredAccess,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4,
   DWORD Unknown5,
   DWORD Unknown6)
{
   UNICODE_STRING WindowStationName;
   PWINSTATION_OBJECT WindowStationObject;
   HWINSTA WindowStation;
   NTSTATUS Status;

   TRACE("NtUserCreateWindowStation called\n");

   Status = ObOpenObjectByName(
               ObjectAttributes,
               ExWindowStationObjectType,
               UserMode,
               NULL,
               dwDesiredAccess,
               NULL,
               (PVOID*)&WindowStation);

   if (NT_SUCCESS(Status))
   {
       TRACE("NtUserCreateWindowStation opened window station %wZ\n", ObjectAttributes->ObjectName);
       return (HWINSTA)WindowStation;
   }

   /*
    * No existing window station found, try to create new one
    */

   /* Capture window station name */
   _SEH2_TRY
   {
      ProbeForRead( ObjectAttributes, sizeof(OBJECT_ATTRIBUTES), 1);
      Status = IntSafeCopyUnicodeStringTerminateNULL(&WindowStationName, ObjectAttributes->ObjectName);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status =_SEH2_GetExceptionCode();
   }
   _SEH2_END

   if (! NT_SUCCESS(Status))
   {
      ERR("Failed reading capturing window station name\n");
      SetLastNtError(Status);
      return NULL;
   }

   /* Create the window station object */
   Status = ObCreateObject(
               UserMode,
               ExWindowStationObjectType,
               ObjectAttributes,
               UserMode,
               NULL,
               sizeof(WINSTATION_OBJECT),
               0,
               0,
               (PVOID*)&WindowStationObject);

   if (!NT_SUCCESS(Status))
   {
      ERR("ObCreateObject failed for window station %wZ\n", &WindowStationName);
      ExFreePoolWithTag(WindowStationName.Buffer, TAG_STRING);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return 0;
   }

   Status = ObInsertObject(
               (PVOID)WindowStationObject,
               NULL,
               dwDesiredAccess,
               0,
               NULL,
               (PVOID*)&WindowStation);

   if (!NT_SUCCESS(Status))
   {
      ERR("ObInsertObject failed for window station %wZ\n", &WindowStationName);
      ExFreePoolWithTag(WindowStationName.Buffer, TAG_STRING);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      ObDereferenceObject(WindowStationObject);
      return 0;
   }

   /* Initialize the window station */
   RtlZeroMemory(WindowStationObject, sizeof(WINSTATION_OBJECT));

   InitializeListHead(&WindowStationObject->DesktopListHead);
   Status = RtlCreateAtomTable(37, &WindowStationObject->AtomTable);
   WindowStationObject->Name = WindowStationName;
   WindowStationObject->dwSessionId = NtCurrentPeb()->SessionId;

   if (InputWindowStation == NULL)
   {
      ERR("Initializing input window station\n");
      InputWindowStation = WindowStationObject;

      WindowStationObject->Flags &= ~WSS_NOIO;

      InitCursorImpl();
   }
   else
   {
      WindowStationObject->Flags |= WSS_NOIO;
   }

   TRACE("NtUserCreateWindowStation created object %p with name %wZ handle %p\n",
          WindowStation, &WindowStationObject->Name, WindowStation);
   return WindowStation;
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

HWINSTA APIENTRY
NtUserOpenWindowStation(
   POBJECT_ATTRIBUTES ObjectAttributes,
   ACCESS_MASK dwDesiredAccess)
{
   HWINSTA hwinsta;
   NTSTATUS Status;

   Status = ObOpenObjectByName(
               ObjectAttributes,
               ExWindowStationObjectType,
               UserMode,
               NULL,
               dwDesiredAccess,
               NULL,
               (PVOID*)&hwinsta);

   if (!NT_SUCCESS(Status))
   {
       ERR("NtUserOpenWindowStation failed\n");
      SetLastNtError(Status);
      return 0;
   }

   TRACE("Opened window station %wZ with handle %p\n", ObjectAttributes->ObjectName, hwinsta);

   return hwinsta;
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
 *    or NtUserOpenWindowStation. Attemps to close a handle to the window
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

   Status = IntValidateWindowStationHandle(
               hWinSta,
               KernelMode,
               0,
               &Object,
               0);

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
   PWINSTATION_OBJECT WinStaObject;
   PDESKTOP DesktopObject = NULL;
   NTSTATUS Status;
   PVOID pvData = NULL;
   DWORD nDataSize = 0;

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

   /* try windowstation */
   TRACE("Trying to open window station %p\n", hObject);
   Status = ObReferenceObjectByHandle(
               hObject,
               0,
               ExWindowStationObjectType,
               UserMode,
               (PVOID*)&WinStaObject,
               NULL);

   if (Status == STATUS_OBJECT_TYPE_MISMATCH)
   {
      /* try desktop */
      TRACE("Trying to open desktop %p\n", hObject);
      WinStaObject = NULL;
      Status = IntValidateDesktopHandle(
                  hObject,
                  UserMode,
                  0,
                  &DesktopObject);
   }

   if (!NT_SUCCESS(Status))
   {
      ERR("Failed: 0x%x\n", Status);
      goto Exit;
   }

   TRACE("WinSta or Desktop opened!!\n");

   /* get data */
   switch (nIndex)
   {
      case UOI_FLAGS:
         Status = STATUS_NOT_IMPLEMENTED;
         ERR("UOI_FLAGS unimplemented!\n");
         break;

      case UOI_NAME:
         if (WinStaObject != NULL)
         {
             pvData = WinStaObject->Name.Buffer;
             nDataSize = WinStaObject->Name.Length + sizeof(WCHAR);
            Status = STATUS_SUCCESS;
         }
         else if (DesktopObject != NULL)
         {
             pvData = DesktopObject->pDeskInfo->szDesktopName;
            nDataSize = (wcslen(DesktopObject->pDeskInfo->szDesktopName) + 1) * sizeof(WCHAR);
            Status = STATUS_SUCCESS;
         }
         else
            Status = STATUS_INVALID_PARAMETER;
         break;

      case UOI_TYPE:
         if (WinStaObject != NULL)
         {
            pvData = L"WindowStation";
            nDataSize = sizeof(L"WindowStation");
            Status = STATUS_SUCCESS;
         }
         else if (DesktopObject != NULL)
         {
            pvData = L"Desktop";
            nDataSize = sizeof(L"Desktop");
            Status = STATUS_SUCCESS;
         }
         else
            Status = STATUS_INVALID_PARAMETER;
         break;

      case UOI_USER_SID:
         Status = STATUS_NOT_IMPLEMENTED;
         ERR("UOI_USER_SID unimplemented!\n");
         break;

      default:
         Status = STATUS_INVALID_PARAMETER;
         break;
   }

Exit:
   if (Status == STATUS_SUCCESS && nLength < nDataSize)
      Status = STATUS_BUFFER_TOO_SMALL;

   _SEH2_TRY
   {
      if (nLengthNeeded)
         *nLengthNeeded = nDataSize;

      /* try to copy data to caller */
      if (Status == STATUS_SUCCESS)
      {
         TRACE("Trying to copy data to caller (len = %lu, len needed = %lu)\n", nLength, nDataSize);
         RtlCopyMemory(pvInformation, pvData, nDataSize);
      }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END;

   /* release objects */
   if (WinStaObject != NULL)
      ObDereferenceObject(WinStaObject);
   if (DesktopObject != NULL)
      ObDereferenceObject(DesktopObject);

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
    PPROCESSINFO ppi;
    NTSTATUS Status;
    HWINSTA hwinstaOld;
    OBJECT_HANDLE_INFORMATION ObjectHandleInfo;
    PWINSTATION_OBJECT NewWinSta = NULL, OldWinSta;

    ppi = PsGetCurrentProcessWin32Process();

    /* Reference the new window station */
    if(hWindowStation !=NULL)
    {
        Status = IntValidateWindowStationHandle( hWindowStation,
                                                 KernelMode,
                                                 0,
                                                 &NewWinSta,
                                                 &ObjectHandleInfo);
       if (!NT_SUCCESS(Status))
       {
          TRACE("Validation of window station handle (%p) failed\n",
                hWindowStation);
          SetLastNtError(Status);
          return FALSE;
       }
    }

   OldWinSta = ppi->prpwinsta;
   hwinstaOld = PsGetProcessWin32WindowStation(ppi->peProcess);

   /* Dereference the previous window station */
   if(OldWinSta != NULL)
   {
       ObDereferenceObject(OldWinSta);
   }

   /* Check if we have a stale handle (it should happen for console apps) */
   if(hwinstaOld != ppi->hwinsta)
   {
       ObCloseHandle(hwinstaOld, UserMode);
   }

   /*
    * FIXME: Don't allow changing the window station if there are threads that are attached to desktops and own GUI objects.
    */

   PsSetProcessWindowStation(ppi->peProcess, hWindowStation);

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

   if (NewWinSta && !(NewWinSta->Flags & WSS_NOIO) )
   {
      ppi->W32PF_flags |= W32PF_IOWINSTA;
   }
   else // Might be closed if the handle is null.
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

   Status = IntValidateWindowStationHandle(
               hWindowStation,
               KernelMode,
               0,
               &Object,
               0);
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

   Status = IntValidateWindowStationHandle(
               hWindowStation,
               KernelMode,
               0,
               &Object,
               0);
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

   /*
    * Try to open the directory.
    */
   InitializeObjectAttributes(
      &ObjectAttributes,
      &gustrWindowStationsDir,
      OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
      NULL,
      NULL);

   Status = ZwOpenDirectoryObject(
               &DirectoryHandle,
               DIRECTORY_QUERY,
               &ObjectAttributes);

   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   /* First try to query the directory using a fixed-size buffer */
   Context = 0;
   Buffer = NULL;
   Status = ZwQueryDirectoryObject(DirectoryHandle, InitialBuffer, sizeof(InitialBuffer),
                                   FALSE, TRUE, &Context, &ReturnLength);
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
   for (DirEntry = (POBJECT_DIRECTORY_INFORMATION) Buffer; 0 != DirEntry->Name.Length;
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
   for (DirEntry = (POBJECT_DIRECTORY_INFORMATION) Buffer; 0 != DirEntry->Name.Length;
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
                                           KernelMode,
                                           0,
                                           &WindowStation,
                                           0);
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

BOOL APIENTRY
NtUserSetWindowStationUser(
   HWINSTA hWindowStation,
   PLUID pluid,
   PSID psid,
   DWORD size)
{
   NTSTATUS Status;
   PWINSTATION_OBJECT WindowStation = NULL;
   BOOL Ret = FALSE;

   UserEnterExclusive();

   if (gpidLogon != PsGetCurrentProcessId())
   {
       EngSetLastError(ERROR_ACCESS_DENIED);
       goto Leave;
   }

   Status = IntValidateWindowStationHandle(hWindowStation,
                                           KernelMode,
                                           0,
                                           &WindowStation,
                                           0);
   if (!NT_SUCCESS(Status))
   {
      goto Leave;
   }

   if (WindowStation->psidUser)
   {
      ExFreePoolWithTag(WindowStation->psidUser, USERTAG_SECURITY);
   }

   WindowStation->psidUser = ExAllocatePoolWithTag(PagedPool, size, USERTAG_SECURITY);
   if (WindowStation->psidUser == NULL)
   {
      EngSetLastError(ERROR_OUTOFMEMORY);
      goto Leave;
   }

   _SEH2_TRY
   {
      ProbeForRead( psid, size, 1);
      ProbeForRead( pluid, sizeof(LUID), 1);

      RtlCopyMemory(WindowStation->psidUser, psid, size);
      WindowStation->luidUser = *pluid;
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END;

   if (!NT_SUCCESS(Status))
   {
      ExFreePoolWithTag(WindowStation->psidUser, USERTAG_SECURITY);
      WindowStation->psidUser = 0;
      goto Leave;
   }

   Ret = TRUE;

Leave:
   if (WindowStation) ObDereferenceObject(WindowStation);
   UserLeave();
   return Ret;
}


/* EOF */
