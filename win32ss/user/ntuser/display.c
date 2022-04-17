/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Video initialization and display settings
 * FILE:             win32ss/user/ntuser/display.c
 * PROGRAMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserDisplay);

BOOL gbBaseVideo = FALSE;
static PPROCESSINFO gpFullscreen = NULL;

static const PWCHAR KEY_VIDEO = L"\\Registry\\Machine\\HARDWARE\\DEVICEMAP\\VIDEO";

VOID
RegWriteDisplaySettings(HKEY hkey, PDEVMODEW pdm)
{
    RegWriteDWORD(hkey, L"DefaultSettings.BitsPerPel", pdm->dmBitsPerPel);
    RegWriteDWORD(hkey, L"DefaultSettings.XResolution", pdm->dmPelsWidth);
    RegWriteDWORD(hkey, L"DefaultSettings.YResolution", pdm->dmPelsHeight);
    RegWriteDWORD(hkey, L"DefaultSettings.Flags", pdm->dmDisplayFlags);
    RegWriteDWORD(hkey, L"DefaultSettings.VRefresh", pdm->dmDisplayFrequency);
    RegWriteDWORD(hkey, L"DefaultSettings.XPanning", pdm->dmPanningWidth);
    RegWriteDWORD(hkey, L"DefaultSettings.YPanning", pdm->dmPanningHeight);
    RegWriteDWORD(hkey, L"DefaultSettings.Orientation", pdm->dmDisplayOrientation);
    RegWriteDWORD(hkey, L"DefaultSettings.FixedOutput", pdm->dmDisplayFixedOutput);
    RegWriteDWORD(hkey, L"Attach.RelativeX", pdm->dmPosition.x);
    RegWriteDWORD(hkey, L"Attach.RelativeY", pdm->dmPosition.y);
//    RegWriteDWORD(hkey, L"Attach.ToDesktop, pdm->dmBitsPerPel", pdm->);
}

VOID
RegReadDisplaySettings(HKEY hkey, PDEVMODEW pdm)
{
    DWORD dwValue;

    /* Zero out the structure */
    RtlZeroMemory(pdm, sizeof(DEVMODEW));

/* Helper macro */
#define READ(field, str, flag) \
    if (RegReadDWORD(hkey, L##str, &dwValue)) \
    { \
        pdm->field = dwValue; \
        pdm->dmFields |= flag; \
    }

    /* Read all present settings */
    READ(dmBitsPerPel, "DefaultSettings.BitsPerPel", DM_BITSPERPEL);
    READ(dmPelsWidth, "DefaultSettings.XResolution", DM_PELSWIDTH);
    READ(dmPelsHeight, "DefaultSettings.YResolution", DM_PELSHEIGHT);
    READ(dmDisplayFlags, "DefaultSettings.Flags", DM_DISPLAYFLAGS);
    READ(dmDisplayFrequency, "DefaultSettings.VRefresh", DM_DISPLAYFREQUENCY);
    READ(dmPanningWidth, "DefaultSettings.XPanning", DM_PANNINGWIDTH);
    READ(dmPanningHeight, "DefaultSettings.YPanning", DM_PANNINGHEIGHT);
    READ(dmDisplayOrientation, "DefaultSettings.Orientation", DM_DISPLAYORIENTATION);
    READ(dmDisplayFixedOutput, "DefaultSettings.FixedOutput", DM_DISPLAYFIXEDOUTPUT);
    READ(dmPosition.x, "Attach.RelativeX", DM_POSITION);
    READ(dmPosition.y, "Attach.RelativeY", DM_POSITION);
}

PGRAPHICS_DEVICE
NTAPI
InitDisplayDriver(
    IN PWSTR pwszDeviceName,
    IN PWSTR pwszRegKey)
{
    PGRAPHICS_DEVICE pGraphicsDevice;
    UNICODE_STRING ustrDeviceName, ustrDisplayDrivers, ustrDescription;
    NTSTATUS Status;
    WCHAR awcBuffer[128];
    ULONG cbSize;
    HKEY hkey;
    DWORD dwVga;

    TRACE("InitDisplayDriver(%S, %S);\n",
          pwszDeviceName, pwszRegKey);

    /* Open the driver's registry key */
    Status = RegOpenKey(pwszRegKey, &hkey);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to open registry key: %ls\n", pwszRegKey);
        return NULL;
    }

    /* Query the diplay drivers */
    cbSize = sizeof(awcBuffer) - 10;
    Status = RegQueryValue(hkey,
                           L"InstalledDisplayDrivers",
                           REG_MULTI_SZ,
                           awcBuffer,
                           &cbSize);
    if (!NT_SUCCESS(Status))
    {
        ERR("Didn't find 'InstalledDisplayDrivers', status = 0x%lx\n", Status);
        ZwClose(hkey);
        return NULL;
    }

    /* Initialize the UNICODE_STRING */
    ustrDisplayDrivers.Buffer = awcBuffer;
    ustrDisplayDrivers.MaximumLength = (USHORT)cbSize;
    ustrDisplayDrivers.Length = (USHORT)cbSize;

    /* Set Buffer for description and size of remaining buffer */
    ustrDescription.Buffer = awcBuffer + (cbSize / sizeof(WCHAR));
    cbSize = sizeof(awcBuffer) - cbSize;

    /* Query the device string */
    Status = RegQueryValue(hkey,
                           L"Device Description",
                           REG_SZ,
                           ustrDescription.Buffer,
                           &cbSize);
    if (NT_SUCCESS(Status))
    {
        ustrDescription.MaximumLength = (USHORT)cbSize;
        ustrDescription.Length = (USHORT)cbSize;
    }
    else
    {
        RtlInitUnicodeString(&ustrDescription, L"<unknown>");
    }

    /* Query if this is a VGA compatible driver */
    cbSize = sizeof(DWORD);
    Status = RegQueryValue(hkey, L"VgaCompatible", REG_DWORD, &dwVga, &cbSize);
    if (!NT_SUCCESS(Status)) dwVga = 0;

    /* Close the registry key */
    ZwClose(hkey);

    /* Register the device with GDI */
    RtlInitUnicodeString(&ustrDeviceName, pwszDeviceName);
    pGraphicsDevice = EngpRegisterGraphicsDevice(&ustrDeviceName,
                                                 &ustrDisplayDrivers,
                                                 &ustrDescription);
    if (pGraphicsDevice && dwVga)
    {
        pGraphicsDevice->StateFlags |= DISPLAY_DEVICE_VGA_COMPATIBLE;
    }

    return pGraphicsDevice;
}

NTSTATUS
NTAPI
InitVideo(VOID)
{
    NTSTATUS Status;
    HKEY hkey;

    TRACE("----------------------------- InitVideo() -------------------------------\n");

    /* Check if VGA mode is requested, by finding the special volatile key created by VIDEOPRT */
    Status = RegOpenKey(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\GraphicsDrivers\\BaseVideo", &hkey);
    if (NT_SUCCESS(Status))
        ZwClose(hkey);
    gbBaseVideo = NT_SUCCESS(Status);
    if (gbBaseVideo)
        ERR("VGA mode requested.\n");

    /* Initialize all display devices */
    Status = EngpUpdateGraphicsDeviceList();
    if (!NT_SUCCESS(Status))
        return Status;

    /* Check if we had any success */
    if (!gpPrimaryGraphicsDevice)
    {
        /* Check if there is a VGA device we skipped */
        if (gpVgaGraphicsDevice)
        {
            /* There is, use the VGA device */
            gpPrimaryGraphicsDevice = gpVgaGraphicsDevice;
        }
        else
        {
            ERR("No usable display driver was found.\n");
            return STATUS_UNSUCCESSFUL;
        }
    }

    InitSysParams();

    return STATUS_SUCCESS;
}

VOID
UserRefreshDisplay(IN PPDEVOBJ ppdev)
{
    ULONG_PTR ulResult;
    // PVOID pvOldCursor;

    // TODO: Re-enable the cursor reset code once this function becomes called
    // from within a Win32 thread... Indeed UserSetCursor() requires this, but
    // at the moment this function is directly called from a separate thread
    // from within videoprt, instead of by a separate win32k system thread.

    if (!ppdev)
        return;

    PDEVOBJ_vReference(ppdev);

    /* Remove mouse pointer */
    // pvOldCursor = UserSetCursor(NULL, TRUE);

    /* Do the mode switch -- Use the actual same current mode */
    ulResult = PDEVOBJ_bSwitchMode(ppdev, ppdev->pdmwDev);
    ASSERT(ulResult);

    /* Restore mouse pointer, no hooks called */
    // pvOldCursor = UserSetCursor(pvOldCursor, TRUE);
    // ASSERT(pvOldCursor == NULL);

    /* Update the system metrics */
    InitMetrics();

    /* Set new size of the monitor */
    // UserUpdateMonitorSize((HDEV)ppdev);

    //co_IntShowDesktop(pdesk, ppdev->gdiinfo.ulHorzRes, ppdev->gdiinfo.ulVertRes);
    UserRedrawDesktop();

    PDEVOBJ_vRelease(ppdev);
}

NTSTATUS
NTAPI
UserEnumDisplayDevices(
    PUNICODE_STRING pustrDevice,
    DWORD iDevNum,
    PDISPLAY_DEVICEW pdispdev,
    DWORD dwFlags)
{
    PGRAPHICS_DEVICE pGraphicsDevice;
    ULONG cbSize;
    HKEY hkey;
    NTSTATUS Status;

    if (!pustrDevice)
    {
        /* Check if some devices have been added since last time */
        EngpUpdateGraphicsDeviceList();
    }

    /* Ask gdi for the GRAPHICS_DEVICE */
    pGraphicsDevice = EngpFindGraphicsDevice(pustrDevice, iDevNum);
    if (!pGraphicsDevice)
    {
        /* No device found */
        ERR("No GRAPHICS_DEVICE found for '%wZ', iDevNum %lu\n", pustrDevice, iDevNum);
        return STATUS_UNSUCCESSFUL;
    }

    /* Open the device map registry key */
    Status = RegOpenKey(KEY_VIDEO, &hkey);
    if (!NT_SUCCESS(Status))
    {
        /* No device found */
        ERR("Could not open reg key\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Query the registry path */
    cbSize = sizeof(pdispdev->DeviceKey);
    RegQueryValue(hkey,
                  pGraphicsDevice->szNtDeviceName,
                  REG_SZ,
                  pdispdev->DeviceKey,
                  &cbSize);

    /* Close registry key */
    ZwClose(hkey);

    /* Copy device name, device string and StateFlags */
    RtlStringCbCopyW(pdispdev->DeviceName, sizeof(pdispdev->DeviceName), pGraphicsDevice->szWinDeviceName);
    RtlStringCbCopyW(pdispdev->DeviceString, sizeof(pdispdev->DeviceString), pGraphicsDevice->pwszDescription);
    pdispdev->StateFlags = pGraphicsDevice->StateFlags;
    // FIXME: fill in DEVICE ID
    pdispdev->DeviceID[0] = UNICODE_NULL;

    return STATUS_SUCCESS;
}

//NTSTATUS
BOOL
NTAPI
NtUserEnumDisplayDevices(
    PUNICODE_STRING pustrDevice,
    DWORD iDevNum,
    PDISPLAY_DEVICEW pDisplayDevice,
    DWORD dwFlags)
{
    UNICODE_STRING ustrDevice;
    WCHAR awcDevice[CCHDEVICENAME];
    DISPLAY_DEVICEW dispdev;
    NTSTATUS Status;

    TRACE("Enter NtUserEnumDisplayDevices(%wZ, %lu)\n",
          pustrDevice, iDevNum);

    dispdev.cb = sizeof(dispdev);

    if (pustrDevice)
    {
        /* Initialize destination string */
        RtlInitEmptyUnicodeString(&ustrDevice, awcDevice, sizeof(awcDevice));

        _SEH2_TRY
        {
            /* Probe the UNICODE_STRING and the buffer */
            ProbeForRead(pustrDevice, sizeof(UNICODE_STRING), 1);
            ProbeForRead(pustrDevice->Buffer, pustrDevice->Length, 1);

            /* Copy the string */
            RtlCopyUnicodeString(&ustrDevice, pustrDevice);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
//            _SEH2_YIELD(return _SEH2_GetExceptionCode());
            _SEH2_YIELD(return NT_SUCCESS(_SEH2_GetExceptionCode()));
        }
        _SEH2_END

        if (ustrDevice.Length > 0)
            pustrDevice = &ustrDevice;
        else
            pustrDevice = NULL;
   }

    /* If name is given only iDevNum==0 gives results */
    if (pustrDevice && iDevNum != 0)
        return FALSE;

    /* Acquire global USER lock */
    UserEnterShared();

    /* Call the internal function */
    Status = UserEnumDisplayDevices(pustrDevice, iDevNum, &dispdev, dwFlags);

    /* Release lock */
    UserLeave();

    /* On success copy data to caller */
    if (NT_SUCCESS(Status))
    {
        /* Enter SEH */
        _SEH2_TRY
        {
            /* First probe the cb field */
            ProbeForWrite(&pDisplayDevice->cb, sizeof(DWORD), 1);

            /* Check the buffer size */
            if (pDisplayDevice->cb)
            {
                /* Probe the output buffer */
                pDisplayDevice->cb = min(pDisplayDevice->cb, sizeof(dispdev));
                ProbeForWrite(pDisplayDevice, pDisplayDevice->cb, 1);

                /* Copy as much as the given buffer allows */
                RtlCopyMemory(pDisplayDevice, &dispdev, pDisplayDevice->cb);
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END
    }

    TRACE("Leave NtUserEnumDisplayDevices, Status = 0x%lx\n", Status);
    /* Return the result */
//    return Status;
    return NT_SUCCESS(Status); // FIXME
}

NTSTATUS
NTAPI
UserEnumCurrentDisplaySettings(
    PUNICODE_STRING pustrDevice,
    PDEVMODEW *ppdm)
{
    PPDEVOBJ ppdev;

    /* Get the PDEV for the device */
    ppdev = EngpGetPDEV(pustrDevice);
    if (!ppdev)
    {
        /* No device found */
        ERR("No PDEV found!\n");
        return STATUS_INVALID_PARAMETER_1;
    }

    *ppdm = ppdev->pdmwDev;
    PDEVOBJ_vRelease(ppdev);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
UserEnumDisplaySettings(
   PUNICODE_STRING pustrDevice,
   DWORD iModeNum,
   LPDEVMODEW *ppdm,
   DWORD dwFlags)
{
    PGRAPHICS_DEVICE pGraphicsDevice;
    PDEVMODEENTRY pdmentry;
    ULONG i, iFoundMode;
    PPDEVOBJ ppdev;

    TRACE("Enter UserEnumDisplaySettings('%wZ', %lu)\n",
          pustrDevice, iModeNum);

    /* Ask GDI for the GRAPHICS_DEVICE */
    pGraphicsDevice = EngpFindGraphicsDevice(pustrDevice, 0);
    ppdev = EngpGetPDEV(pustrDevice);

    if (!pGraphicsDevice || !ppdev)
    {
        /* No device found */
        ERR("No device found!\n");
        return STATUS_INVALID_PARAMETER_1;
    }

    /* let's politely ask the driver for an updated mode list,
       just in case there's something new in there (vbox) */

    PDEVOBJ_vRefreshModeList(ppdev);
    PDEVOBJ_vRelease(ppdev);

    iFoundMode = 0;
    for (i = 0; i < pGraphicsDevice->cDevModes; i++)
    {
        pdmentry = &pGraphicsDevice->pDevModeList[i];

        /* FIXME: Consider EDS_RAWMODE */
#if 0
        if ((!(dwFlags & EDS_RAWMODE) && (pdmentry->dwFlags & 1)) ||!
            (dwFlags & EDS_RAWMODE))
#endif
        {
            /* Is this the one we want? */
            if (iFoundMode == iModeNum)
            {
                *ppdm = pdmentry->pdm;
                return STATUS_SUCCESS;
            }

            /* Increment number of found modes */
            iFoundMode++;
        }
    }

    /* Nothing was found */
    return STATUS_INVALID_PARAMETER_2;
}

NTSTATUS
NTAPI
UserOpenDisplaySettingsKey(
    OUT PHKEY phkey,
    IN PUNICODE_STRING pustrDevice,
    IN BOOL bGlobal)
{
    HKEY hkey;
    DISPLAY_DEVICEW dispdev;
    NTSTATUS Status;

    /* Get device info */
    Status = UserEnumDisplayDevices(pustrDevice, 0, &dispdev, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    if (bGlobal)
    {
        // FIXME: Need to fix the registry key somehow
    }

    /* Open the registry key */
    Status = RegOpenKey(dispdev.DeviceKey, &hkey);
    if (!NT_SUCCESS(Status))
        return Status;

    *phkey = hkey;

    return Status;
}

NTSTATUS
NTAPI
UserEnumRegistryDisplaySettings(
    IN PUNICODE_STRING pustrDevice,
    OUT LPDEVMODEW pdm)
{
    HKEY hkey;
    NTSTATUS Status = UserOpenDisplaySettingsKey(&hkey, pustrDevice, 0);
    if(NT_SUCCESS(Status))
    {
        RegReadDisplaySettings(hkey, pdm);
        ZwClose(hkey);
        return STATUS_SUCCESS;
    }
    return Status;
}

NTSTATUS
APIENTRY
NtUserEnumDisplaySettings(
    IN PUNICODE_STRING pustrDevice,
    IN DWORD iModeNum,
    OUT LPDEVMODEW lpDevMode,
    IN DWORD dwFlags)
{
    UNICODE_STRING ustrDeviceUser;
    UNICODE_STRING ustrDevice;
    WCHAR awcDevice[CCHDEVICENAME];
    NTSTATUS Status;
    ULONG cbSize, cbExtra;
    DEVMODEW dmReg, *pdm;

    TRACE("Enter NtUserEnumDisplaySettings(%wZ, %lu, %p, 0x%lx)\n",
          pustrDevice, iModeNum, lpDevMode, dwFlags);

    _SEH2_TRY
    {
        ProbeForRead(lpDevMode, sizeof(DEVMODEW), sizeof(UCHAR));

        cbSize = lpDevMode->dmSize;
        cbExtra = lpDevMode->dmDriverExtra;

        ProbeForWrite(lpDevMode, cbSize + cbExtra, sizeof(UCHAR));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    if (cbSize != sizeof(DEVMODEW))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (pustrDevice)
    {
        /* Initialize destination string */
        RtlInitEmptyUnicodeString(&ustrDevice, awcDevice, sizeof(awcDevice));

        _SEH2_TRY
        {
            /* Probe the UNICODE_STRING and the buffer */
            ustrDeviceUser = ProbeForReadUnicodeString(pustrDevice);

            if (!ustrDeviceUser.Length || !ustrDeviceUser.Buffer)
                ExRaiseStatus(STATUS_NO_MEMORY);

            ProbeForRead(ustrDeviceUser.Buffer,
                         ustrDeviceUser.Length,
                         sizeof(UCHAR));

            /* Copy the string */
            RtlCopyUnicodeString(&ustrDevice, &ustrDeviceUser);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return STATUS_INVALID_PARAMETER_1);
        }
        _SEH2_END;

        pustrDevice = &ustrDevice;
    }

    /* Acquire global USER lock */
    UserEnterShared();

    if (iModeNum == ENUM_REGISTRY_SETTINGS)
    {
        /* Get the registry settings */
        Status = UserEnumRegistryDisplaySettings(pustrDevice, &dmReg);
        pdm = &dmReg;
        pdm->dmSize = sizeof(DEVMODEW);
    }
    else if (iModeNum == ENUM_CURRENT_SETTINGS)
    {
        /* Get the current settings */
        Status = UserEnumCurrentDisplaySettings(pustrDevice, &pdm);
    }
    else
    {
        /* Get specified settings */
        Status = UserEnumDisplaySettings(pustrDevice, iModeNum, &pdm, dwFlags);
    }

    /* Release lock */
    UserLeave();

    /* Did we succeed? */
    if (NT_SUCCESS(Status))
    {
        /* Copy some information back */
        _SEH2_TRY
        {
            /* Output what we got */
            RtlCopyMemory(lpDevMode, pdm, min(cbSize, pdm->dmSize));

            /* Output private/extra driver data */
            if (cbExtra > 0 && pdm->dmDriverExtra > 0)
            {
                RtlCopyMemory((PCHAR)lpDevMode + cbSize,
                              (PCHAR)pdm + pdm->dmSize,
                              min(cbExtra, pdm->dmDriverExtra));
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    return Status;
}

VOID
UserUpdateFullscreen(
    DWORD flags)
{
    if (flags & CDS_FULLSCREEN)
        gpFullscreen = gptiCurrent->ppi;
    else
        gpFullscreen = NULL;
}

LONG
APIENTRY
UserChangeDisplaySettings(
   PUNICODE_STRING pustrDevice,
   LPDEVMODEW pdm,
   DWORD flags,
   LPVOID lParam)
{
    DEVMODEW dm;
    LONG lResult = DISP_CHANGE_SUCCESSFUL;
    HKEY hkey;
    NTSTATUS Status;
    PPDEVOBJ ppdev;
    WORD OrigBC;
    //PDESKTOP pdesk;
    PDEVMODEW newDevMode = NULL;

    /* If no DEVMODE is given, use registry settings */
    if (!pdm)
    {
        /* Get the registry settings */
        Status = UserEnumRegistryDisplaySettings(pustrDevice, &dm);
        if (!NT_SUCCESS(Status))
        {
            ERR("Could not load registry settings\n");
            return DISP_CHANGE_BADPARAM;
        }
    }
    else if (pdm->dmSize < FIELD_OFFSET(DEVMODEW, dmFields))
    {
        return DISP_CHANGE_BADMODE; /* This is what WinXP SP3 returns */
    }
    else
    {
        dm = *pdm;
    }

    /* Save original bit count */
    OrigBC = gpsi->BitCount;

    /* Check params */
    if ((dm.dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT)) != (DM_PELSWIDTH | DM_PELSHEIGHT))
    {
        ERR("Devmode doesn't specify the resolution.\n");
        return DISP_CHANGE_BADMODE;
    }

    /* Get the PDEV */
    ppdev = EngpGetPDEV(pustrDevice);
    if (!ppdev)
    {
        ERR("Failed to get PDEV\n");
        return DISP_CHANGE_BADPARAM;
    }

    /* Fixup values */
    if (dm.dmBitsPerPel == 0 || !(dm.dmFields & DM_BITSPERPEL))
    {
        dm.dmBitsPerPel = ppdev->pdmwDev->dmBitsPerPel;
        dm.dmFields |= DM_BITSPERPEL;
    }

    if ((dm.dmFields & DM_DISPLAYFREQUENCY) && (dm.dmDisplayFrequency == 0))
        dm.dmDisplayFrequency = ppdev->pdmwDev->dmDisplayFrequency;

    /* Look for the requested DEVMODE */
    if (!LDEVOBJ_bProbeAndCaptureDevmode(ppdev->pGraphicsDevice, &dm, &newDevMode, FALSE))
    {
        ERR("Could not find a matching DEVMODE\n");
        lResult = DISP_CHANGE_BADMODE;
        goto leave;
    }
    else if (flags & CDS_TEST)
    {
        /* It's possible, go ahead! */
        lResult = DISP_CHANGE_SUCCESSFUL;
        goto leave;
    }

    /* Shall we update the registry? */
    if (flags & CDS_UPDATEREGISTRY)
    {
        /* Open the local or global settings key */
        Status = UserOpenDisplaySettingsKey(&hkey, pustrDevice, flags & CDS_GLOBAL);
        if (NT_SUCCESS(Status))
        {
            /* Store the settings */
            RegWriteDisplaySettings(hkey, newDevMode);

            /* Close the registry key */
            ZwClose(hkey);
        }
        else
        {
            ERR("Could not open registry key\n");
            lResult = DISP_CHANGE_NOTUPDATED;
        }
    }

    /* Check if DEVMODE matches the current mode */
    if (newDevMode->dmSize == ppdev->pdmwDev->dmSize &&
        RtlCompareMemory(newDevMode, ppdev->pdmwDev, newDevMode->dmSize) == newDevMode->dmSize &&
        !(flags & CDS_RESET))
    {
        ERR("DEVMODE matches, nothing to do\n");
        goto leave;
    }

    /* Shall we apply the settings? */
    if (!(flags & CDS_NORESET))
    {
        ULONG_PTR ulResult;
        PVOID pvOldCursor;
        TEXTMETRICW tmw;

        /* Remove mouse pointer */
        pvOldCursor = UserSetCursor(NULL, TRUE);

        /* Do the mode switch */
        ulResult = PDEVOBJ_bSwitchMode(ppdev, newDevMode);

        /* Restore mouse pointer, no hooks called */
        pvOldCursor = UserSetCursor(pvOldCursor, TRUE);
        ASSERT(pvOldCursor == NULL);

        /* Check for success or failure */
        if (!ulResult)
        {
            /* Setting mode failed */
            ERR("Failed to set mode\n");

            /* Set the correct return value */
            if ((flags & CDS_UPDATEREGISTRY) && (lResult != DISP_CHANGE_NOTUPDATED))
                lResult = DISP_CHANGE_RESTART;
            else
                lResult = DISP_CHANGE_FAILED;
        }
        else
        {
            /* Setting mode succeeded */
            lResult = DISP_CHANGE_SUCCESSFUL;
            ExFreePoolWithTag(ppdev->pdmwDev, GDITAG_DEVMODE);
            ppdev->pdmwDev = newDevMode;

            UserUpdateFullscreen(flags);

            /* Update the system metrics */
            InitMetrics();

            /* Set new size of the monitor */
            UserUpdateMonitorSize((HDEV)ppdev);

            /* Update the SERVERINFO */
            gpsi->dmLogPixels = ppdev->gdiinfo.ulLogPixelsY;
            gpsi->Planes      = ppdev->gdiinfo.cPlanes;
            gpsi->BitsPixel   = ppdev->gdiinfo.cBitsPixel;
            gpsi->BitCount    = gpsi->Planes * gpsi->BitsPixel;
            gpsi->aiSysMet[SM_CXSCREEN] = ppdev->gdiinfo.ulHorzRes;
            gpsi->aiSysMet[SM_CYSCREEN] = ppdev->gdiinfo.ulVertRes;
            if (ppdev->gdiinfo.flRaster & RC_PALETTE)
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
        }

        /*
         * Refresh the display on success and even on failure,
         * since the display may have been messed up.
         */

        /* Remove all cursor clipping */
        UserClipCursor(NULL);

        //pdesk = IntGetActiveDesktop();
        //IntHideDesktop(pdesk);

        /* Send WM_DISPLAYCHANGE to all toplevel windows */
        co_IntSendMessageTimeout( HWND_BROADCAST,
                                  WM_DISPLAYCHANGE,
                                  gpsi->BitCount,
                                  MAKELONG(gpsi->aiSysMet[SM_CXSCREEN], gpsi->aiSysMet[SM_CYSCREEN]),
                                  SMTO_NORMAL,
                                  100,
                                  &ulResult );

        ERR("BitCount New %d Orig %d ChkNew %d\n",gpsi->BitCount,OrigBC,ppdev->gdiinfo.cBitsPixel);

        /* Not full screen and different bit count, send messages */
        if (!(flags & CDS_FULLSCREEN) &&
            gpsi->BitCount != OrigBC)
        {
            ERR("Detect settings changed.\n");
            UserSendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);
            UserSendNotifyMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
        }

        //co_IntShowDesktop(pdesk, ppdev->gdiinfo.ulHorzRes, ppdev->gdiinfo.ulVertRes);

        UserRedrawDesktop();
    }

leave:
    if (newDevMode && newDevMode != ppdev->pdmwDev)
        ExFreePoolWithTag(newDevMode, GDITAG_DEVMODE);

    /* Release the PDEV */
    PDEVOBJ_vRelease(ppdev);

    return lResult;
}

VOID
UserDisplayNotifyShutdown(
    PPROCESSINFO ppiCurrent)
{
    if (ppiCurrent == gpFullscreen)
    {
        UserChangeDisplaySettings(NULL, NULL, 0, NULL);
        if (gpFullscreen)
            ERR("Failed to restore display mode!\n");
    }
}

LONG
APIENTRY
NtUserChangeDisplaySettings(
    PUNICODE_STRING pustrDevice,
    LPDEVMODEW lpDevMode,
    DWORD dwflags,
    LPVOID lParam)
{
    WCHAR awcDevice[CCHDEVICENAME];
    UNICODE_STRING ustrDevice;
    DEVMODEW dmLocal;
    LONG lRet;

    /* Check arguments */
    if ((dwflags != CDS_VIDEOPARAMETERS) && (lParam != NULL))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return DISP_CHANGE_BADPARAM;
    }

    /* Check flags */
    if ((dwflags & (CDS_GLOBAL|CDS_NORESET)) && !(dwflags & CDS_UPDATEREGISTRY))
    {
        return DISP_CHANGE_BADFLAGS;
    }

    if ((dwflags & CDS_RESET) && (dwflags & CDS_NORESET))
    {
        return DISP_CHANGE_BADFLAGS;
    }

    /* Copy the device name */
    if (pustrDevice)
    {
        /* Initialize destination string */
        RtlInitEmptyUnicodeString(&ustrDevice, awcDevice, sizeof(awcDevice));

        _SEH2_TRY
        {
            /* Probe the UNICODE_STRING and the buffer */
            ProbeForRead(pustrDevice, sizeof(UNICODE_STRING), 1);
            ProbeForRead(pustrDevice->Buffer, pustrDevice->Length, 1);

            /* Copy the string */
            RtlCopyUnicodeString(&ustrDevice, pustrDevice);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Set and return error */
            SetLastNtError(_SEH2_GetExceptionCode());
            _SEH2_YIELD(return DISP_CHANGE_BADPARAM);
        }
        _SEH2_END

        pustrDevice = &ustrDevice;
   }

    /* Copy devmode */
    if (lpDevMode)
    {
        _SEH2_TRY
        {
            /* Probe the size field of the structure */
            ProbeForRead(lpDevMode, sizeof(dmLocal.dmSize), 1);

            /* Calculate usable size */
            dmLocal.dmSize = min(sizeof(dmLocal), lpDevMode->dmSize);

            /* Probe and copy the full DEVMODE */
            ProbeForRead(lpDevMode, dmLocal.dmSize, 1);
            RtlCopyMemory(&dmLocal, lpDevMode, dmLocal.dmSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Set and return error */
            SetLastNtError(_SEH2_GetExceptionCode());
            _SEH2_YIELD(return DISP_CHANGE_BADPARAM);
        }
        _SEH2_END

        /* Check for extra parameters */
        if (dmLocal.dmDriverExtra > 0)
        {
            /* FIXME: TODO */
            ERR("lpDevMode->dmDriverExtra is IGNORED!\n");
            dmLocal.dmDriverExtra = 0;
        }

        /* Use the local structure */
        lpDevMode = &dmLocal;
    }

    // FIXME: Copy videoparameters

    /* Acquire global USER lock */
    UserEnterExclusive();

    /* Call internal function */
    lRet = UserChangeDisplaySettings(pustrDevice, lpDevMode, dwflags, NULL);

    /* Release lock */
    UserLeave();

    return lRet;
}
