/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Device Functions
 * FILE:              win32ss/gdi/eng/device.c
 * PROGRAMER:         Jason Filby
 *                    Timo Kreuzer
 */

#include <win32k.h>
#include <ntddvdeo.h>

DBG_DEFAULT_CHANNEL(EngDev);

static PGRAPHICS_DEVICE gpPrimaryGraphicsDevice;
static PGRAPHICS_DEVICE gpVgaGraphicsDevice;

static PGRAPHICS_DEVICE gpGraphicsDeviceFirst = NULL;
static PGRAPHICS_DEVICE gpGraphicsDeviceLast = NULL;
static HSEMAPHORE ghsemGraphicsDeviceList;
static ULONG giDevNum = 1;

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitDeviceImpl(VOID)
{
    ghsemGraphicsDeviceList = EngCreateSemaphore();
    if (!ghsemGraphicsDeviceList)
        return STATUS_INSUFFICIENT_RESOURCES;

    return STATUS_SUCCESS;
}

static
BOOLEAN
EngpHasVgaDriver(
    _In_ PGRAPHICS_DEVICE pGraphicsDevice)
{
    WCHAR awcDeviceKey[256], awcServiceName[100];
    PWSTR lastBkSlash;
    NTSTATUS Status;
    ULONG cbValue;
    HKEY hkey;

    /* Open the key for the adapters */
    Status = RegOpenKey(L"\\Registry\\Machine\\HARDWARE\\DEVICEMAP\\VIDEO", &hkey);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not open HARDWARE\\DEVICEMAP\\VIDEO registry key: 0x%08lx\n", Status);
        return FALSE;
    }

    /* Read the name of the device key */
    cbValue = sizeof(awcDeviceKey);
    Status = RegQueryValue(hkey, pGraphicsDevice->szNtDeviceName, REG_SZ, awcDeviceKey, &cbValue);
    ZwClose(hkey);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not read '%S' registry value: 0x%08lx\n", Status);
        return FALSE;
    }

    /* Replace 'DeviceN' by 'Video' */
    lastBkSlash = wcsrchr(awcDeviceKey, L'\\');
    if (!lastBkSlash)
    {
        ERR("Invalid registry key '%S'\n", lastBkSlash);
        return FALSE;
    }
    if (!NT_SUCCESS(RtlStringCchCopyW(lastBkSlash + 1,
                                      ARRAYSIZE(awcDeviceKey) - (lastBkSlash + 1 - awcDeviceKey),
                                      L"Video")))
    {
        ERR("Failed to add 'Video' to registry key '%S'\n", awcDeviceKey);
        return FALSE;
    }

    /* Open device key */
    Status = RegOpenKey(awcDeviceKey, &hkey);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not open %S registry key: 0x%08lx\n", awcDeviceKey, Status);
        return FALSE;
    }

    /* Read service name */
    cbValue = sizeof(awcServiceName);
    Status = RegQueryValue(hkey, L"Service", REG_SZ, awcServiceName, &cbValue);
    ZwClose(hkey);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not read Service registry value in %S: 0x%08lx\n", awcDeviceKey, Status);
        return FALSE;
    }

    /* Device is using VGA driver if service name is 'VGASave' (case insensitive) */
    return (_wcsicmp(awcServiceName, L"VGASave") == 0);
}

/*
 * Add a device to gpGraphicsDeviceFirst/gpGraphicsDeviceLast list (if not already present).
 */
_Requires_lock_held_(ghsemGraphicsDeviceList)
static
VOID
EngpLinkGraphicsDevice(
    _In_ PGRAPHICS_DEVICE pToAdd)
{
    PGRAPHICS_DEVICE pGraphicsDevice;

    TRACE("EngLinkGraphicsDevice(%p)\n", pToAdd);

    /* Search if device is not already linked */
    for (pGraphicsDevice = gpGraphicsDeviceFirst;
         pGraphicsDevice;
         pGraphicsDevice = pGraphicsDevice->pNextGraphicsDevice)
    {
        if (pGraphicsDevice == pToAdd)
            return;
    }

    pToAdd->pNextGraphicsDevice = NULL;
    if (gpGraphicsDeviceLast)
        gpGraphicsDeviceLast->pNextGraphicsDevice = pToAdd;
    gpGraphicsDeviceLast = pToAdd;
    if (!gpGraphicsDeviceFirst)
        gpGraphicsDeviceFirst = pToAdd;
}

/*
 * Remove a device from gpGraphicsDeviceFirst/gpGraphicsDeviceLast list.
 */
_Requires_lock_held_(ghsemGraphicsDeviceList)
static
VOID
EngpUnlinkGraphicsDevice(
    _In_ PGRAPHICS_DEVICE pToDelete)
{
    PGRAPHICS_DEVICE pPrevGraphicsDevice = NULL;
    PGRAPHICS_DEVICE pGraphicsDevice = gpGraphicsDeviceFirst;

    TRACE("EngpUnlinkGraphicsDevice('%S')\n", pToDelete->szNtDeviceName);

    while (pGraphicsDevice)
    {
        if (pGraphicsDevice != pToDelete)
        {
            /* Keep current device */
            pPrevGraphicsDevice = pGraphicsDevice;
            pGraphicsDevice = pGraphicsDevice->pNextGraphicsDevice;
        }
        else
        {
            /* At first, link again associated VGA Device */
            if (pGraphicsDevice->pVgaDevice)
                EngpLinkGraphicsDevice(pGraphicsDevice->pVgaDevice);

            /* We need to remove current device */
            pGraphicsDevice = pGraphicsDevice->pNextGraphicsDevice;

            /* Unlink chain */
            if (!pPrevGraphicsDevice)
                gpGraphicsDeviceFirst = pToDelete->pNextGraphicsDevice;
            else
                pPrevGraphicsDevice->pNextGraphicsDevice = pToDelete->pNextGraphicsDevice;
            if (gpGraphicsDeviceLast == pToDelete)
                gpGraphicsDeviceLast = pPrevGraphicsDevice;
        }
    }
}

NTSTATUS
EngpUpdateGraphicsDeviceList(VOID)
{
    ULONG iDevNum, iVGACompatible = -1, ulMaxObjectNumber = 0;
    WCHAR awcDeviceName[20], awcWinDeviceName[20];
    UNICODE_STRING ustrDeviceName;
    WCHAR awcBuffer[256];
    NTSTATUS Status;
    PGRAPHICS_DEVICE pGraphicsDevice;
    BOOLEAN bFoundNewDevice = FALSE;
    ULONG cbValue;
    HKEY hkey;

    /* Open the key for the adapters */
    Status = RegOpenKey(L"\\Registry\\Machine\\HARDWARE\\DEVICEMAP\\VIDEO", &hkey);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not open HARDWARE\\DEVICEMAP\\VIDEO registry key:0x%lx\n", Status);
        return Status;
    }

    /* Read the name of the VGA adapter */
    cbValue = sizeof(awcDeviceName);
    Status = RegQueryValue(hkey, L"VgaCompatible", REG_SZ, awcDeviceName, &cbValue);
    if (NT_SUCCESS(Status))
    {
        iVGACompatible = _wtoi(&awcDeviceName[sizeof("\\Device\\Video")-1]);
        ERR("VGA adapter = %lu\n", iVGACompatible);
    }

    /* Get the maximum number of adapters */
    if (!RegReadDWORD(hkey, L"MaxObjectNumber", &ulMaxObjectNumber))
    {
        ERR("Could not read MaxObjectNumber, defaulting to 0.\n");
    }

    TRACE("Found %lu devices\n", ulMaxObjectNumber + 1);

    /* Loop through all adapters */
    for (iDevNum = 0; iDevNum <= ulMaxObjectNumber; iDevNum++)
    {
        /* Create the adapter's key name */
        swprintf(awcDeviceName, L"\\Device\\Video%lu", iDevNum);

        /* Create the display device name */
        swprintf(awcWinDeviceName, L"\\\\.\\DISPLAY%lu", iDevNum + 1);
        RtlInitUnicodeString(&ustrDeviceName, awcWinDeviceName);

        /* Check if the device exists already */
        pGraphicsDevice = EngpFindGraphicsDevice(&ustrDeviceName, iDevNum);
        if (pGraphicsDevice != NULL)
        {
            continue;
        }

        /* Read the reg key name */
        cbValue = sizeof(awcBuffer);
        Status = RegQueryValue(hkey, awcDeviceName, REG_SZ, awcBuffer, &cbValue);
        if (!NT_SUCCESS(Status))
        {
            ERR("failed to query the registry path:0x%lx\n", Status);
            continue;
        }

        /* Initialize the driver for this device */
        pGraphicsDevice = InitDisplayDriver(awcDeviceName, awcBuffer);
        if (!pGraphicsDevice) continue;

        /* Check if this is a VGA compatible adapter */
        if (pGraphicsDevice->StateFlags & DISPLAY_DEVICE_VGA_COMPATIBLE)
        {
            /* Save this as the VGA adapter */
            if (!gpVgaGraphicsDevice || !EngpHasVgaDriver(gpVgaGraphicsDevice))
            {
                gpVgaGraphicsDevice = pGraphicsDevice;
                TRACE("gpVgaGraphicsDevice = %p\n", gpVgaGraphicsDevice);
            }
        }
        bFoundNewDevice = TRUE;

        /* Set the first one as primary device */
        if (!gpPrimaryGraphicsDevice || EngpHasVgaDriver(gpPrimaryGraphicsDevice))
        {
            gpPrimaryGraphicsDevice = pGraphicsDevice;
            TRACE("gpPrimaryGraphicsDevice = %p\n", gpPrimaryGraphicsDevice);
        }
    }

    /* Close the device map registry key */
    ZwClose(hkey);

    /* Can we link VGA device to primary device? */
    if (gpPrimaryGraphicsDevice &&
        gpVgaGraphicsDevice &&
        gpPrimaryGraphicsDevice != gpVgaGraphicsDevice &&
        !gpPrimaryGraphicsDevice->pVgaDevice)
    {
        /* Yes. Remove VGA device from global list, and attach it to primary device */
        TRACE("Linking VGA device %S to primary device %S\n", gpVgaGraphicsDevice->szNtDeviceName, gpPrimaryGraphicsDevice->szNtDeviceName);
        EngpUnlinkGraphicsDevice(gpVgaGraphicsDevice);
        gpPrimaryGraphicsDevice->pVgaDevice = gpVgaGraphicsDevice;
    }

    if (bFoundNewDevice && gbBaseVideo)
    {
        PGRAPHICS_DEVICE pToDelete;

        /* Lock list */
        EngAcquireSemaphore(ghsemGraphicsDeviceList);

        /* Remove every device from linked list, except base-video one */
        pGraphicsDevice = gpGraphicsDeviceFirst;
        while (pGraphicsDevice)
        {
            if (!EngpHasVgaDriver(pGraphicsDevice))
            {
                /* Not base-video device. Remove it */
                pToDelete = pGraphicsDevice;
                TRACE("Removing non-base-video device %S (%S)\n", pToDelete->szWinDeviceName, pToDelete->szNtDeviceName);

                EngpUnlinkGraphicsDevice(pGraphicsDevice);
                pGraphicsDevice = pGraphicsDevice->pNextGraphicsDevice;

                /* Free memory */
                ExFreePoolWithTag(pToDelete->pDiplayDrivers, GDITAG_DRVSUP);
                ExFreePoolWithTag(pToDelete, GDITAG_GDEVICE);
            }
            else
            {
                pGraphicsDevice = pGraphicsDevice->pNextGraphicsDevice;
            }
        }

        /* Unlock list */
        EngReleaseSemaphore(ghsemGraphicsDeviceList);
    }

    return STATUS_SUCCESS;
}

/* Open display settings registry key
 * Returns NULL in case of error. */
static HKEY
EngpGetRegistryHandleFromDeviceMap(
    _In_ PGRAPHICS_DEVICE pGraphicsDevice)
{
    static const PWCHAR KEY_VIDEO = L"\\Registry\\Machine\\HARDWARE\\DEVICEMAP\\VIDEO";
    HKEY hKey;
    WCHAR szDeviceKey[256];
    ULONG cbSize;
    NTSTATUS Status;

    /* Open the device map registry key */
    Status = RegOpenKey(KEY_VIDEO, &hKey);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not open HARDWARE\\DEVICEMAP\\VIDEO registry key: status 0x%08x\n", Status);
        return NULL;
    }

    /* Query the registry path */
    cbSize = sizeof(szDeviceKey);
    RegQueryValue(hKey,
                  pGraphicsDevice->szNtDeviceName,
                  REG_SZ,
                  szDeviceKey,
                  &cbSize);
    ZwClose(hKey);

    /* Open the registry key */
    Status = RegOpenKey(szDeviceKey, &hKey);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not open registry key '%S': status 0x%08x\n", szDeviceKey, Status);
        return NULL;
    }

    return hKey;
}

NTSTATUS
EngpGetDisplayDriverParameters(
    _In_ PGRAPHICS_DEVICE pGraphicsDevice,
    _Out_ PDEVMODEW pdm)
{
    HKEY hKey;
    NTSTATUS Status;
    RTL_QUERY_REGISTRY_TABLE DisplaySettingsTable[] =
    {
#define READ(field, str) \
        { \
            NULL, \
            RTL_QUERY_REGISTRY_DIRECT, \
            L ##str, \
            &pdm->field, \
            REG_NONE, NULL, 0 \
        },
    READ(dmBitsPerPel, "DefaultSettings.BitsPerPel")
    READ(dmPelsWidth, "DefaultSettings.XResolution")
    READ(dmPelsHeight, "DefaultSettings.YResolution")
    READ(dmDisplayFlags, "DefaultSettings.Flags")
    READ(dmDisplayFrequency, "DefaultSettings.VRefresh")
    READ(dmPanningWidth, "DefaultSettings.XPanning")
    READ(dmPanningHeight, "DefaultSettings.YPanning")
    READ(dmDisplayOrientation, "DefaultSettings.Orientation")
    READ(dmDisplayFixedOutput, "DefaultSettings.FixedOutput")
    READ(dmPosition.x, "Attach.RelativeX")
    READ(dmPosition.y, "Attach.RelativeY")
#undef READ
        {0}
    };

    hKey = EngpGetRegistryHandleFromDeviceMap(pGraphicsDevice);
    if (!hKey)
        return STATUS_UNSUCCESSFUL;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR)hKey,
                                    DisplaySettingsTable,
                                    NULL,
                                    NULL);

    ZwClose(hKey);
    return Status;
}

DWORD
EngpGetDisplayDriverAccelerationLevel(
    _In_ PGRAPHICS_DEVICE pGraphicsDevice)
{
    HKEY hKey;
    DWORD dwAccelerationLevel = 0;
    RTL_QUERY_REGISTRY_TABLE DisplaySettingsTable[] =
    {
        {
            NULL,
            RTL_QUERY_REGISTRY_DIRECT,
            L"Acceleration.Level",
            &dwAccelerationLevel,
            REG_NONE, NULL, 0
        },
        {0}
    };

    hKey = EngpGetRegistryHandleFromDeviceMap(pGraphicsDevice);
    if (!hKey)
        return 0;

    RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                           (PWSTR)hKey,
                           DisplaySettingsTable,
                           NULL,
                           NULL);
    ZwClose(hKey);

    return dwAccelerationLevel;
}

extern VOID
UserRefreshDisplay(IN PPDEVOBJ ppdev);

// PVIDEO_WIN32K_CALLOUT
VOID
NTAPI
VideoPortCallout(
    _In_ PVOID Params)
{
/*
 * IMPORTANT NOTICE!! On Windows XP/2003 this function triggers the creation of
 * a specific VideoPortCalloutThread() system thread using the same mechanism
 * as the RIT/desktop/Ghost system threads.
 */

    PVIDEO_WIN32K_CALLBACKS_PARAMS CallbackParams = (PVIDEO_WIN32K_CALLBACKS_PARAMS)Params;

    TRACE("VideoPortCallout(0x%p, 0x%x)\n",
          CallbackParams, CallbackParams ? CallbackParams->CalloutType : -1);

    if (!CallbackParams)
        return;

    switch (CallbackParams->CalloutType)
    {
        case VideoFindAdapterCallout:
        {
            TRACE("VideoPortCallout: VideoFindAdapterCallout called - Param = %s\n",
                  CallbackParams->Param ? "TRUE" : "FALSE");
            if (CallbackParams->Param == TRUE)
            {
                /* Re-enable the display */
                UserRefreshDisplay(gpmdev->ppdevGlobal);
            }
            else
            {
                /* Disable the display */
                NOTHING; // Nothing to do for the moment...
            }

            CallbackParams->Status = STATUS_SUCCESS;
            break;
        }

        case VideoPowerNotifyCallout:
        case VideoDisplaySwitchCallout:
        case VideoEnumChildPdoNotifyCallout:
        case VideoWakeupCallout:
        case VideoChangeDisplaySettingsCallout:
        case VideoPnpNotifyCallout:
        case VideoDxgkDisplaySwitchCallout:
        case VideoDxgkMonitorEventCallout:
        case VideoDxgkFindAdapterTdrCallout:
            ERR("VideoPortCallout: CalloutType 0x%x is UNIMPLEMENTED!\n", CallbackParams->CalloutType);
            CallbackParams->Status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            ERR("VideoPortCallout: Unknown CalloutType 0x%x\n", CallbackParams->CalloutType);
            CallbackParams->Status = STATUS_UNSUCCESSFUL;
            break;
    }
}

PGRAPHICS_DEVICE
NTAPI
EngpRegisterGraphicsDevice(
    _In_ PUNICODE_STRING pustrDeviceName,
    _In_ PUNICODE_STRING pustrDiplayDrivers,
    _In_ PUNICODE_STRING pustrDescription)
{
    PGRAPHICS_DEVICE pGraphicsDevice;
    PDEVICE_OBJECT pDeviceObject;
    PFILE_OBJECT pFileObject;
    NTSTATUS Status;
    VIDEO_WIN32K_CALLBACKS Win32kCallbacks;
    ULONG ulReturn;
    PWSTR pwsz;
    ULONG cj;

    TRACE("EngpRegisterGraphicsDevice(%wZ)\n", pustrDeviceName);

    /* Allocate a GRAPHICS_DEVICE structure */
    pGraphicsDevice = ExAllocatePoolZero(PagedPool,
                                         sizeof(GRAPHICS_DEVICE),
                                         GDITAG_GDEVICE);
    if (!pGraphicsDevice)
    {
        ERR("ExAllocatePoolWithTag failed\n");
        return NULL;
    }

    /* Try to open and enable the device */
    Status = IoGetDeviceObjectPointer(pustrDeviceName,
                                      FILE_READ_DATA | FILE_WRITE_DATA,
                                      &pFileObject,
                                      &pDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not open device %wZ, 0x%lx\n", pustrDeviceName, Status);
        ExFreePoolWithTag(pGraphicsDevice, GDITAG_GDEVICE);
        return NULL;
    }

    /* Copy the device and file object pointers */
    pGraphicsDevice->DeviceObject = pDeviceObject;
    pGraphicsDevice->FileObject = pFileObject;

    /* Initialize and register the device with videoprt for Win32k callbacks */
    Win32kCallbacks.PhysDisp = pGraphicsDevice;
    Win32kCallbacks.Callout = VideoPortCallout;
    // Reset the data being returned prior to the call.
    Win32kCallbacks.bACPI = FALSE;
    Win32kCallbacks.pPhysDeviceObject = NULL;
    Win32kCallbacks.DualviewFlags = 0;
    Status = (NTSTATUS)EngDeviceIoControl((HANDLE)pDeviceObject,
                                          IOCTL_VIDEO_INIT_WIN32K_CALLBACKS,
                                          &Win32kCallbacks,
                                          sizeof(Win32kCallbacks),
                                          &Win32kCallbacks,
                                          sizeof(Win32kCallbacks),
                                          &ulReturn);
    if (Status != ERROR_SUCCESS)
    {
        ERR("EngDeviceIoControl(0x%p, IOCTL_VIDEO_INIT_WIN32K_CALLBACKS) failed, Status 0x%lx\n",
            pDeviceObject, Status);
    }
    // TODO: Set flags according to the results.
    // if (Win32kCallbacks.bACPI)
    // if (Win32kCallbacks.DualviewFlags & ???)
    pGraphicsDevice->PhysDeviceHandle = Win32kCallbacks.pPhysDeviceObject;

    /* FIXME: Enumerate children monitor devices for this video adapter
     *
     * - Force the adapter to re-enumerate its monitors:
     *   IoSynchronousInvalidateDeviceRelations(pdo, BusRelations)
     *
     * - Retrieve all monitor PDOs from VideoPrt:
     *   EngDeviceIoControl(0x%p, IOCTL_VIDEO_ENUM_MONITOR_PDO)
     *
     * - Initialize these fields and structures accordingly:
     *   pGraphicsDevice->dwMonCnt
     *   pGraphicsDevice->pvMonDev[0..dwMonCnt-1]
     */

    /* Copy the device name */
    RtlStringCbCopyNW(pGraphicsDevice->szNtDeviceName,
                      sizeof(pGraphicsDevice->szNtDeviceName),
                      pustrDeviceName->Buffer,
                      pustrDeviceName->Length);

    /* Create a Win32 device name (FIXME: virtual devices!) */
    RtlStringCbPrintfW(pGraphicsDevice->szWinDeviceName,
                       sizeof(pGraphicsDevice->szWinDeviceName),
                       L"\\\\.\\DISPLAY%d",
                       (int)giDevNum);

    /* Allocate a buffer for the strings */
    cj = pustrDiplayDrivers->Length + pustrDescription->Length + sizeof(WCHAR);
    pwsz = ExAllocatePoolWithTag(PagedPool, cj, GDITAG_DRVSUP);
    if (!pwsz)
    {
        ERR("Could not allocate string buffer\n");
        ASSERT(FALSE); // FIXME
        ExFreePoolWithTag(pGraphicsDevice, GDITAG_GDEVICE);
        return NULL;
    }

    /* Copy the display driver names */
    pGraphicsDevice->pDiplayDrivers = pwsz;
    RtlCopyMemory(pGraphicsDevice->pDiplayDrivers,
                  pustrDiplayDrivers->Buffer,
                  pustrDiplayDrivers->Length);

    /* Copy the description */
    pGraphicsDevice->pwszDescription = pwsz + pustrDiplayDrivers->Length / sizeof(WCHAR);
    RtlCopyMemory(pGraphicsDevice->pwszDescription,
                  pustrDescription->Buffer,
                  pustrDescription->Length);
    pGraphicsDevice->pwszDescription[pustrDescription->Length/sizeof(WCHAR)] = 0;

    /* Lock loader */
    EngAcquireSemaphore(ghsemGraphicsDeviceList);

    /* Insert the device into the global list */
    EngpLinkGraphicsDevice(pGraphicsDevice);

    /* Increment the device number */
    giDevNum++;

    /* Unlock loader */
    EngReleaseSemaphore(ghsemGraphicsDeviceList);
    TRACE("Prepared %lu modes for %ls\n", pGraphicsDevice->cDevModes, pGraphicsDevice->pwszDescription);

    /* HACK: already in graphic mode; display wallpaper on this new display */
    if (ScreenDeviceContext)
    {
        UNICODE_STRING DriverName = RTL_CONSTANT_STRING(L"DISPLAY");
        UNICODE_STRING DisplayName;
        HDC hdc;
        RtlInitUnicodeString(&DisplayName, pGraphicsDevice->szWinDeviceName);
        hdc = IntGdiCreateDC(&DriverName, &DisplayName, NULL, NULL, FALSE);
        IntPaintDesktop(hdc);
    }

    return pGraphicsDevice;
}

PGRAPHICS_DEVICE
NTAPI
EngpFindGraphicsDevice(
    _In_opt_ PUNICODE_STRING pustrDevice,
    _In_ ULONG iDevNum)
{
    UNICODE_STRING ustrCurrent;
    PGRAPHICS_DEVICE pGraphicsDevice;
    ULONG i;
    TRACE("EngpFindGraphicsDevice('%wZ', %lu)\n",
           pustrDevice, iDevNum);

    /* Lock list */
    EngAcquireSemaphore(ghsemGraphicsDeviceList);

    if (pustrDevice && pustrDevice->Buffer)
    {
        /* Find specified video adapter by name */
        for (pGraphicsDevice = gpGraphicsDeviceFirst;
             pGraphicsDevice;
             pGraphicsDevice = pGraphicsDevice->pNextGraphicsDevice)
        {
            /* Compare the device name */
            RtlInitUnicodeString(&ustrCurrent, pGraphicsDevice->szWinDeviceName);
            if (RtlEqualUnicodeString(&ustrCurrent, pustrDevice, FALSE))
            {
                break;
            }
        }

        if (pGraphicsDevice)
        {
            /* Validate selected monitor number */
#if 0
            if (iDevNum >= pGraphicsDevice->dwMonCnt)
                pGraphicsDevice = NULL;
#else
            /* FIXME: dwMonCnt not initialized, see EngpRegisterGraphicsDevice */
#endif
        }
    }
    else
    {
        /* Select video adapter by device number */
        for (pGraphicsDevice = gpGraphicsDeviceFirst, i = 0;
             pGraphicsDevice && i < iDevNum;
             pGraphicsDevice = pGraphicsDevice->pNextGraphicsDevice, i++);
    }

    /* Unlock list */
    EngReleaseSemaphore(ghsemGraphicsDeviceList);

    return pGraphicsDevice;
}

static
NTSTATUS
EngpFileIoRequest(
    _In_ PFILE_OBJECT pFileObject,
    _In_ ULONG ulMajorFunction,
    _In_reads_(nBufferSize) PVOID lpBuffer,
    _In_ SIZE_T nBufferSize,
    _In_ ULONGLONG ullStartOffset,
    _Out_ PULONG_PTR lpInformation)
{
    PDEVICE_OBJECT pDeviceObject;
    KEVENT Event;
    PIRP pIrp;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    LARGE_INTEGER liStartOffset;

    /* Get corresponding device object */
    pDeviceObject = IoGetRelatedDeviceObject(pFileObject);
    if (!pDeviceObject)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Initialize an event */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    /* Build IRP */
    liStartOffset.QuadPart = ullStartOffset;
    pIrp = IoBuildSynchronousFsdRequest(ulMajorFunction,
                                        pDeviceObject,
                                        lpBuffer,
                                        (ULONG)nBufferSize,
                                        &liStartOffset,
                                        &Event,
                                        &Iosb);
    if (!pIrp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Call the driver */
    Status = IoCallDriver(pDeviceObject, pIrp);

    /* Wait if neccessary */
    if (STATUS_PENDING == Status)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);
        Status = Iosb.Status;
    }

    /* Return information to the caller about the operation. */
    *lpInformation = Iosb.Information;

    /* Return NTSTATUS */
    return Status;
}

VOID
APIENTRY
EngFileWrite(
    _In_ PFILE_OBJECT pFileObject,
    _In_reads_(nLength) PVOID lpBuffer,
    _In_ SIZE_T nLength,
    _Out_ PSIZE_T lpBytesWritten)
{
    NTSTATUS status;

    status = EngpFileIoRequest(pFileObject,
                               IRP_MJ_WRITE,
                               lpBuffer,
                               nLength,
                               0,
                               lpBytesWritten);
    if (!NT_SUCCESS(status))
    {
        *lpBytesWritten = 0;
    }
}

_Success_(return>=0)
NTSTATUS
APIENTRY
EngFileIoControl(
    _In_ PFILE_OBJECT pFileObject,
    _In_ DWORD dwIoControlCode,
    _In_reads_(nInBufferSize) PVOID lpInBuffer,
    _In_ SIZE_T nInBufferSize,
    _Out_writes_(nOutBufferSize) PVOID lpOutBuffer,
    _In_ SIZE_T nOutBufferSize,
    _Out_ PULONG_PTR lpInformation)
{
    PDEVICE_OBJECT pDeviceObject;
    KEVENT Event;
    PIRP pIrp;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    /* Get corresponding device object */
    pDeviceObject = IoGetRelatedDeviceObject(pFileObject);
    if (!pDeviceObject)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Initialize an event */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    /* Build IO control IRP */
    pIrp = IoBuildDeviceIoControlRequest(dwIoControlCode,
                                         pDeviceObject,
                                         lpInBuffer,
                                         (ULONG)nInBufferSize,
                                         lpOutBuffer,
                                         (ULONG)nOutBufferSize,
                                         FALSE,
                                         &Event,
                                         &Iosb);
    if (!pIrp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Call the driver */
    Status = IoCallDriver(pDeviceObject, pIrp);

    /* Wait if neccessary */
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);
        Status = Iosb.Status;
    }

    /* Return information to the caller about the operation. */
    *lpInformation = Iosb.Information;

    /* This function returns NTSTATUS */
    return Status;
}

/*
 * @implemented
 */
_Success_(return==0)
DWORD
APIENTRY
EngDeviceIoControl(
    _In_ HANDLE hDevice,
    _In_ DWORD dwIoControlCode,
    _In_reads_bytes_opt_(cjInBufferSize) LPVOID lpInBuffer,
    _In_ DWORD cjInBufferSize,
    _Out_writes_bytes_opt_(cjOutBufferSize) LPVOID lpOutBuffer,
    _In_ DWORD cjOutBufferSize,
    _Out_ LPDWORD lpBytesReturned)
{
    PIRP Irp;
    NTSTATUS Status;
    KEVENT Event;
    IO_STATUS_BLOCK Iosb;
    PDEVICE_OBJECT DeviceObject;

    TRACE("EngDeviceIoControl() called\n");

    if (!hDevice)
    {
        return ERROR_INVALID_HANDLE;
    }

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    DeviceObject = (PDEVICE_OBJECT) hDevice;

    Irp = IoBuildDeviceIoControlRequest(dwIoControlCode,
                                        DeviceObject,
                                        lpInBuffer,
                                        cjInBufferSize,
                                        lpOutBuffer,
                                        cjOutBufferSize,
                                        FALSE,
                                        &Event,
                                        &Iosb);
    if (!Irp) return ERROR_NOT_ENOUGH_MEMORY;

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        (VOID)KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);
        Status = Iosb.Status;
    }

    TRACE("EngDeviceIoControl(): Returning %X/%X\n", Iosb.Status,
           Iosb.Information);

    /* Return information to the caller about the operation. */
    *lpBytesReturned = (DWORD)Iosb.Information;

    /* Convert NT status values to win32 error codes. */
    switch (Status)
    {
        case STATUS_INSUFFICIENT_RESOURCES:
            return ERROR_NOT_ENOUGH_MEMORY;

        case STATUS_BUFFER_OVERFLOW:
            return ERROR_MORE_DATA;

        case STATUS_NOT_IMPLEMENTED:
            return ERROR_INVALID_FUNCTION;

        case STATUS_INVALID_PARAMETER:
            return ERROR_INVALID_PARAMETER;

        case STATUS_BUFFER_TOO_SMALL:
            return ERROR_INSUFFICIENT_BUFFER;

        case STATUS_DEVICE_DOES_NOT_EXIST:
            return ERROR_DEV_NOT_EXIST;

        case STATUS_PENDING:
            return ERROR_IO_PENDING;
    }

    return Status;
}

/* EOF */
