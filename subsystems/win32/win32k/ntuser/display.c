/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Video initialization and display settings
 * FILE:             subsystems/win32/win32k/ntuser/display.c
 * PROGRAMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>

#include <intrin.h>

#define NDEBUG
#include <debug.h>

PDEVOBJ *gpdevPrimary;

const PWCHAR KEY_ROOT = L"";
const PWCHAR KEY_VIDEO = L"\\Registry\\Machine\\HARDWARE\\DEVICEMAP\\VIDEO";

NTSTATUS
NTAPI
UserEnumDisplayDevices(
    PUNICODE_STRING pustrDevice,
    DWORD iDevNum,
    PDISPLAY_DEVICEW pdispdev,
    DWORD dwFlags);

VOID
RegWriteSZ(HKEY hkey, PWSTR pwszValue, PWSTR pwszData)
{
    UNICODE_STRING ustrValue;
    UNICODE_STRING ustrData;

    RtlInitUnicodeString(&ustrValue, pwszValue);
    RtlInitUnicodeString(&ustrData, pwszData);
    ZwSetValueKey(hkey, &ustrValue, 0, REG_SZ, &ustrData, ustrData.Length + sizeof(WCHAR));
}

VOID
RegWriteDWORD(HKEY hkey, PWSTR pwszValue, DWORD dwData)
{
    UNICODE_STRING ustrValue;

    RtlInitUnicodeString(&ustrValue, pwszValue);
    ZwSetValueKey(hkey, &ustrValue, 0, REG_DWORD, &dwData, sizeof(DWORD));
}


BOOL
RegReadDWORD(HKEY hkey, PWSTR pwszValue, PDWORD pdwData)
{
    NTSTATUS Status;
    ULONG cbSize = sizeof(DWORD);
    Status = RegQueryValue(hkey, pwszValue, REG_DWORD, pdwData, &cbSize);
    return NT_SUCCESS(Status);
}

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


enum
{
    VF_USEVGA = 0x1,
};

BOOL
InitDisplayDriver(
    PUNICODE_STRING pustrRegPath,
    FLONG flags)
{
//    PWSTR pwszDriverName;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    NTSTATUS Status;

    /* Setup QueryTable for direct registry query */
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED|RTL_QUERY_REGISTRY_DIRECT;


    /* Check if vga mode is requested */
    if (flags & VF_USEVGA)
    {
        DWORD dwVgaCompatible;

        /*  */
        QueryTable[0].Name = L"VgaCompatible";
        QueryTable[0].EntryContext = &dwVgaCompatible;

        /* Check if the driver is vga */
        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        pustrRegPath->Buffer,
                                        QueryTable,
                                        NULL,
                                        NULL);

        if (!dwVgaCompatible)
        {
            /* This driver is not a vga driver */
            return FALSE;
        }
    }

#if 0

    /* Query the adapter's registry path */
    swprintf(awcBuffer, L"\\Device\\Video%lu", iDevNum);
    QueryTable[0].Name = pGraphicsDevice->szNtDeviceName;

    /* Set string for the registry key */
    ustrRegistryPath.Buffer = pdispdev->DeviceKey;
    ustrRegistryPath.Length = 128;
    ustrRegistryPath.MaximumLength = 128;
    QueryTable[0].EntryContext = &ustrRegistryPath;

    /* Query the registry */
    Status = RtlQueryRegistryValues(RTL_REGISTRY_DEVICEMAP,
                                    L"VIDEO",
                                    QueryTable,
                                    NULL,
                                    NULL);

    RegQueryValue(KEY_VIDEO, awcBuffer, REG_SZ, pdispdev->DeviceKey, 256);

    {
        HANDLE hmod;

        hmod = EngLoadImage(pwszDriverName);

        /* Jump to next name */
        pwszDriverName += wcslen(pwszDriverName) + 1;
    }
    while (pwszDriverName < 0);
#endif

    return 0;
}


NTSTATUS
NTAPI
DisplayDriverQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext)
{
    PWSTR pwszRegKey = ValueData;
    PGRAPHICS_DEVICE pGraphicsDevice;
    UNICODE_STRING ustrDeviceName, ustrDisplayDrivers, ustrDescription;
    NTSTATUS Status;
    WCHAR awcBuffer[128];
    ULONG cbSize;
    HKEY hkey;
    DEVMODEW dmDefault;

    UNREFERENCED_PARAMETER(ValueLength);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(EntryContext);

    DPRINT1("DisplayDriverQueryRoutine(%S, %S);\n",
            ValueName, pwszRegKey);

    /* Check if we have a correct entry */
    if (ValueType != REG_SZ || ValueName[0] != '\\')
    {
        /* Something else, just skip it */
        return STATUS_SUCCESS;
    }

    /* Open the driver's registry key */
    Status = RegOpenKey(pwszRegKey, &hkey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open registry key\n");
        return STATUS_SUCCESS;
    }

// HACK: only use 1st adapter
//if (ValueName[13] != '0')
//    return STATUS_SUCCESS;

    /* Query the diplay drivers */
    cbSize = sizeof(awcBuffer) - 10;
    Status = RegQueryValue(hkey,
                           L"InstalledDisplayDrivers",
                           REG_MULTI_SZ,
                           awcBuffer,
                           &cbSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Didn't find 'InstalledDisplayDrivers', status = 0x%lx\n", Status);
        ZwClose(hkey);
        return STATUS_SUCCESS;
    }

    /* Initialize the UNICODE_STRING */
    ustrDisplayDrivers.Buffer = awcBuffer;
    ustrDisplayDrivers.MaximumLength = cbSize;
    ustrDisplayDrivers.Length = cbSize;

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
        ustrDescription.MaximumLength = cbSize;
        ustrDescription.Length = cbSize;
    }
    else
    {
        RtlInitUnicodeString(&ustrDescription, L"<unknown>");
    }

    /* Query the default settings */
    RegReadDisplaySettings(hkey, &dmDefault);

    /* Close the registry key */
    ZwClose(hkey);

    /* Register the device with GDI */
    RtlInitUnicodeString(&ustrDeviceName, ValueName);
    pGraphicsDevice = EngpRegisterGraphicsDevice(&ustrDeviceName,
                                                 &ustrDisplayDrivers,
                                                 &ustrDescription,
                                                 &dmDefault);

    // FIXME: what to do with pGraphicsDevice?

    return STATUS_SUCCESS;
}

BOOL InitSysParams();

BOOL
InitVideo(FLONG flags)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    NTSTATUS Status;

    DPRINT1("----------------------------- InitVideo() -------------------------------\n");

    /* Setup QueryTable for registry query */
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = DisplayDriverQueryRoutine;

    /* Query the registry */
    Status = RtlQueryRegistryValues(RTL_REGISTRY_DEVICEMAP,
                                    L"VIDEO",
                                    QueryTable,
                                    NULL,
                                    NULL);

    InitSysParams();

    return 0;
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

    /* Ask gdi for the GRAPHICS_DEVICE */
    pGraphicsDevice = EngpFindGraphicsDevice(pustrDevice, iDevNum, 0);
    if (!pGraphicsDevice)
    {
        /* No device found */
        DPRINT1("No GRAPHICS_DEVICE found\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Open thhe device map registry key */
    Status = RegOpenKey(KEY_VIDEO, &hkey);
    if (!NT_SUCCESS(Status))
    {
        /* No device found */
        DPRINT1("Could not open reg key\n");
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
    wcsncpy(pdispdev->DeviceName, pGraphicsDevice->szWinDeviceName, 32);
    wcsncpy(pdispdev->DeviceString, pGraphicsDevice->pwszDescription, 128);
    pdispdev->StateFlags = pGraphicsDevice->StateFlags;

    // FIXME: fill in DEVICE ID

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

    DPRINT1("Enter NtUserEnumDisplayDevices(%p, %ls, %ld)\n",
            pustrDevice, pustrDevice ? pustrDevice->Buffer : 0, iDevNum);

    // FIXME: HACK, desk.cpl passes broken crap
    if (pustrDevice && iDevNum != 0)
        return FALSE;

    dispdev.cb = sizeof(DISPLAY_DEVICEW);

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

    /* Acquire global USER lock */
    UserEnterExclusive();

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

    DPRINT1("Leave NtUserEnumDisplayDevices, Status = 0x%lx\n", Status);
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
        DPRINT1("No PDEV found!\n");
        return STATUS_UNSUCCESSFUL;
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

    DPRINT1("Enter UserEnumDisplaySettings('%ls', %ld)\n",
            pustrDevice ? pustrDevice->Buffer : NULL, iModeNum);

    /* Ask gdi for the GRAPHICS_DEVICE */
    pGraphicsDevice = EngpFindGraphicsDevice(pustrDevice, 0, 0);
    if (!pGraphicsDevice)
    {
        /* No device found */
        DPRINT1("No device found!\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (iModeNum == 0)
    {
        DPRINT1("Should initialize modes somehow\n");
        // Update DISPLAY_DEVICEs?
    }

    iFoundMode = 0;
    for (i = 0; i < pGraphicsDevice->cDevModes; i++)
    {
        pdmentry = &pGraphicsDevice->pDevModeList[i];

//        if ((!(dwFlags & EDS_RAWMODE) && (pdmentry->dwFlags & 1)) || // FIXME!
//            (dwFlags & EDS_RAWMODE))
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
    return STATUS_INVALID_PARAMETER;
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
        // FIXME: need to fix the registry key somehow
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
    return Status ;
}


NTSTATUS
APIENTRY
NtUserEnumDisplaySettings(
    IN PUNICODE_STRING pustrDevice,
    IN DWORD iModeNum,
    OUT LPDEVMODEW lpDevMode,
    IN DWORD dwFlags)
{
    UNICODE_STRING ustrDevice;
    WCHAR awcDevice[CCHDEVICENAME];
    NTSTATUS Status;
    ULONG cbSize, cbExtra;
    DEVMODEW dmReg, *pdm;

    DPRINT1("Enter NtUserEnumDisplaySettings(%ls, %ld)\n",
            pustrDevice ? pustrDevice->Buffer:0, iModeNum);

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
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END

        pustrDevice = &ustrDevice;
   }

    /* Acquire global USER lock */
    UserEnterExclusive();

    if (iModeNum == ENUM_REGISTRY_SETTINGS)
    {
        /* Get the registry settings */
        Status = UserEnumRegistryDisplaySettings(pustrDevice, &dmReg);
        pdm = &dmReg;
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
            ProbeForRead(lpDevMode, sizeof(DEVMODEW), 1);
            cbSize = lpDevMode->dmSize;
            cbExtra = lpDevMode->dmDriverExtra;

            ProbeForWrite(lpDevMode, cbSize + cbExtra, 1);
            /* Output what we got */
            RtlCopyMemory(lpDevMode, pdm, min(cbSize, pdm->dmSize));

            /* output private/extra driver data */
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

BOOL APIENTRY UserClipCursor(RECTL *prcl);
VOID APIENTRY UserRedrawDesktop();
HCURSOR FASTCALL UserSetCursor(PCURICON_OBJECT NewCursor, BOOL ForceChange);

LONG
APIENTRY
UserChangeDisplaySettings(
   PUNICODE_STRING pustrDevice,
   LPDEVMODEW pdm,
   HWND hwnd,
   DWORD flags,
   LPVOID lParam)
{
    DEVMODEW dm;
    LONG lResult = DISP_CHANGE_SUCCESSFUL;
    HKEY hkey;
    NTSTATUS Status;
    PPDEVOBJ ppdev;
    PDESKTOP pdesk;

    /* If no DEVMODE is given, use registry settings */
    if (!pdm)
    {
        /* Get the registry settings */
        Status = UserEnumRegistryDisplaySettings(pustrDevice, &dm);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Could not load registry settings\n");
            return DISP_CHANGE_BADPARAM;
        }
    }
    else if (pdm->dmSize < FIELD_OFFSET(DEVMODEW, dmFields))
        return DISP_CHANGE_BADMODE; /* This is what winXP SP3 returns */
    else
        dm = *pdm;

    /* Check params */
    if ((dm.dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT)) != (DM_PELSWIDTH | DM_PELSHEIGHT))
    {
        DPRINT1("devmode doesn't specify the resolution.\n");
        return DISP_CHANGE_BADMODE;
    }

    /* Get the PDEV */
    ppdev = EngpGetPDEV(pustrDevice);
    if (!ppdev)
    {
        DPRINT1("failed to get PDEV\n");
        return DISP_CHANGE_BADPARAM;
    }

    /* Fixup values */
    if((dm.dmFields & DM_BITSPERPEL) && (dm.dmBitsPerPel == 0))
        dm.dmBitsPerPel = ppdev->pdmwDev->dmBitsPerPel;
    if((dm.dmFields & DM_DISPLAYFREQUENCY) && (dm.dmDisplayFrequency == 0))
        dm.dmDisplayFrequency = ppdev->pdmwDev->dmDisplayFrequency;

    /* Look for the requested DEVMODE */
    pdm = PDEVOBJ_pdmMatchDevMode(ppdev, &dm);
    if (!pdm)
    {
        DPRINT1("Could not find a matching DEVMODE\n");
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
            RegWriteDisplaySettings(hkey, pdm);

            /* Close the registry key */
            ZwClose(hkey);
        }
        else
        {
            DPRINT1("Could not open registry key\n");
            lResult = DISP_CHANGE_NOTUPDATED;
        }
    }

    /* Check if DEVMODE matches the current mode */
    if (pdm == ppdev->pdmwDev && !(flags & CDS_RESET))
    {
        DPRINT1("DEVMODE matches, nothing to do\n");
        goto leave;
    }

    /* Shall we apply the settings? */
    if (!(flags & CDS_NORESET))
    {
        ULONG ulResult;

        /* Remove mouse pointer */
        UserSetCursor(NULL, TRUE);

        /* Do the mode switch */
        ulResult = PDEVOBJ_bSwitchMode(ppdev, pdm);

        /* Restore mouse pointer, no hooks called */
        UserSetCursorPos(gpsi->ptCursor.x, gpsi->ptCursor.y, FALSE);

        /* Check for failure */
        if (!ulResult)
        {
            DPRINT1("failed to set mode\n");
            lResult = (lResult == DISP_CHANGE_NOTUPDATED) ?
                DISP_CHANGE_FAILED : DISP_CHANGE_RESTART;

            goto leave;
        }

        /* Update the system metrics */
        InitMetrics();

        //IntvGetDeviceCaps(&PrimarySurface, &GdiHandleTable->DevCaps);

        /* Remove all cursor clipping */
        UserClipCursor(NULL);

        pdesk = IntGetActiveDesktop();
        //IntHideDesktop(pdesk);

        /* Send WM_DISPLAYCHANGE to all toplevel windows */
        co_IntSendMessageTimeout(HWND_BROADCAST,
                                 WM_DISPLAYCHANGE,
                                 (WPARAM)ppdev->gdiinfo.cBitsPixel,
                                 (LPARAM)(ppdev->gdiinfo.ulHorzRes + (ppdev->gdiinfo.ulVertRes << 16)),
                                 SMTO_NORMAL,
                                 100,
                                 &ulResult);

        //co_IntShowDesktop(pdesk, ppdev->gdiinfo.ulHorzRes, ppdev->gdiinfo.ulVertRes);

        UserRedrawDesktop();
    }

leave:
    /* Release the PDEV */
    PDEVOBJ_vRelease(ppdev);

    return lResult;
}

LONG
APIENTRY
NtUserChangeDisplaySettings(
    PUNICODE_STRING pustrDevice,
    LPDEVMODEW lpDevMode,
    HWND hwnd,
    DWORD dwflags,
    LPVOID lParam)
{
    WCHAR awcDevice[CCHDEVICENAME];
    UNICODE_STRING ustrDevice;
    DEVMODEW dmLocal;
    LONG lRet;

    /* Check arguments */
    if ((dwflags != CDS_VIDEOPARAMETERS && lParam != NULL) ||
        (hwnd != NULL))
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return DISP_CHANGE_BADPARAM;
    }

    /* Check flags */
    if ((dwflags & (CDS_GLOBAL|CDS_NORESET)) && !(dwflags & CDS_UPDATEREGISTRY))
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
            DPRINT1("lpDevMode->dmDriverExtra is IGNORED!\n");
            dmLocal.dmDriverExtra = 0;
        }

        /* Use the local structure */
        lpDevMode = &dmLocal;
    }

    // FIXME: Copy videoparameters

    /* Acquire global USER lock */
    UserEnterExclusive();

    /* Call internal function */
    lRet = UserChangeDisplaySettings(pustrDevice, lpDevMode, hwnd, dwflags, NULL);

    /* Release lock */
    UserLeave();

    return lRet;
}

