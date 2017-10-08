/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Device Functions
 * FILE:              win32ss/gdi/eng/device.c
 * PROGRAMER:         Jason Filby
 *                    Timo Kreuzer
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(EngDev)

PGRAPHICS_DEVICE gpPrimaryGraphicsDevice;
PGRAPHICS_DEVICE gpVgaGraphicsDevice;

static PGRAPHICS_DEVICE gpGraphicsDeviceFirst = NULL;
static PGRAPHICS_DEVICE gpGraphicsDeviceLast = NULL;
static HSEMAPHORE ghsemGraphicsDeviceList;
static ULONG giDevNum = 1;

INIT_FUNCTION
NTSTATUS
NTAPI
InitDeviceImpl(VOID)
{
    ghsemGraphicsDeviceList = EngCreateSemaphore();
    if (!ghsemGraphicsDeviceList)
        return STATUS_INSUFFICIENT_RESOURCES;

    return STATUS_SUCCESS;
}

BOOLEAN
EngpPopulateDeviceModeList(
    _Inout_ PGRAPHICS_DEVICE pGraphicsDevice,
    _In_ PDEVMODEW pdmDefault)
{
    PWSTR pwsz;
    PLDEVOBJ pldev;
    PDEVMODEINFO pdminfo;
    PDEVMODEW pdm, pdmEnd;
    ULONG i, cModes = 0;
    BOOLEAN bModeMatch = FALSE;

    ASSERT(pGraphicsDevice->pdevmodeInfo == NULL);
    ASSERT(pGraphicsDevice->pDevModeList == NULL);

    pwsz = pGraphicsDevice->pDiplayDrivers;

    /* Loop through the driver names
     * This is a REG_MULTI_SZ string */
    for (; *pwsz; pwsz += wcslen(pwsz) + 1)
    {
        TRACE("trying driver: %ls\n", pwsz);
        /* Try to load the display driver */
        pldev = EngLoadImageEx(pwsz, LDEV_DEVICE_DISPLAY);
        if (!pldev)
        {
            ERR("Could not load driver: '%ls'\n", pwsz);
            continue;
        }

        /* Get the mode list from the driver */
        pdminfo = LDEVOBJ_pdmiGetModes(pldev, pGraphicsDevice->DeviceObject);
        if (!pdminfo)
        {
            ERR("Could not get mode list for '%ls'\n", pwsz);
            continue;
        }

        /* Attach the mode info to the device */
        pdminfo->pdmiNext = pGraphicsDevice->pdevmodeInfo;
        pGraphicsDevice->pdevmodeInfo = pdminfo;

        /* Loop all DEVMODEs */
        pdmEnd = (DEVMODEW*)((PCHAR)pdminfo->adevmode + pdminfo->cbdevmode);
        for (pdm = pdminfo->adevmode;
             (pdm + 1 <= pdmEnd) && (pdm->dmSize != 0);
             pdm = (DEVMODEW*)((PCHAR)pdm + pdm->dmSize + pdm->dmDriverExtra))
        {
            /* Count this DEVMODE */
            cModes++;

            /* Some drivers like the VBox driver don't fill the dmDeviceName
               with the name of the display driver. So fix that here. */
            wcsncpy(pdm->dmDeviceName, pwsz, CCHDEVICENAME);
            pdm->dmDeviceName[CCHDEVICENAME - 1] = 0;
        }

        // FIXME: release the driver again until it's used?
    }

    if (!pGraphicsDevice->pdevmodeInfo || cModes == 0)
    {
        ERR("No devmodes\n");
        return FALSE;
    }

    /* Allocate an index buffer */
    pGraphicsDevice->cDevModes = cModes;
    pGraphicsDevice->pDevModeList = ExAllocatePoolWithTag(PagedPool,
                                                          cModes * sizeof(DEVMODEENTRY),
                                                          GDITAG_GDEVICE);
    if (!pGraphicsDevice->pDevModeList)
    {
        ERR("No devmode list\n");
        return FALSE;
    }

    TRACE("Looking for mode %lux%lux%lu(%lu Hz)\n",
        pdmDefault->dmPelsWidth,
        pdmDefault->dmPelsHeight,
        pdmDefault->dmBitsPerPel,
        pdmDefault->dmDisplayFrequency);

    /* Loop through all DEVMODEINFOs */
    for (pdminfo = pGraphicsDevice->pdevmodeInfo, i = 0;
         pdminfo;
         pdminfo = pdminfo->pdmiNext)
    {
        /* Calculate End of the DEVMODEs */
        pdmEnd = (DEVMODEW*)((PCHAR)pdminfo->adevmode + pdminfo->cbdevmode);

        /* Loop through the DEVMODEs */
        for (pdm = pdminfo->adevmode;
             (pdm + 1 <= pdmEnd) && (pdm->dmSize != 0);
             pdm = (PDEVMODEW)((PCHAR)pdm + pdm->dmSize + pdm->dmDriverExtra))
        {
            TRACE("    %S has mode %lux%lux%lu(%lu Hz)\n",
                  pdm->dmDeviceName,
                  pdm->dmPelsWidth,
                  pdm->dmPelsHeight,
                  pdm->dmBitsPerPel,
                  pdm->dmDisplayFrequency);
            /* Compare with the default entry */
            if (!bModeMatch &&
                pdm->dmBitsPerPel == pdmDefault->dmBitsPerPel &&
                pdm->dmPelsWidth == pdmDefault->dmPelsWidth &&
                pdm->dmPelsHeight == pdmDefault->dmPelsHeight)
            {
                pGraphicsDevice->iDefaultMode = i;
                pGraphicsDevice->iCurrentMode = i;
                TRACE("Found default entry: %lu '%ls'\n", i, pdm->dmDeviceName);
                if (pdm->dmDisplayFrequency == pdmDefault->dmDisplayFrequency)
                {
                    /* Uh oh, even the display frequency matches. */
                    bModeMatch = TRUE;
                }
            }

            /* Initialize the entry */
            pGraphicsDevice->pDevModeList[i].dwFlags = 0;
            pGraphicsDevice->pDevModeList[i].pdm = pdm;
            i++;
        }
    }
    return TRUE;
}

PGRAPHICS_DEVICE
NTAPI
EngpRegisterGraphicsDevice(
    _In_ PUNICODE_STRING pustrDeviceName,
    _In_ PUNICODE_STRING pustrDiplayDrivers,
    _In_ PUNICODE_STRING pustrDescription,
    _In_ PDEVMODEW pdmDefault)
{
    PGRAPHICS_DEVICE pGraphicsDevice;
    PDEVICE_OBJECT pDeviceObject;
    PFILE_OBJECT pFileObject;
    NTSTATUS Status;
    PWSTR pwsz;
    ULONG cj;
    SIZE_T cjWritten;
    BOOL bEnable = TRUE;

    TRACE("EngpRegisterGraphicsDevice(%wZ)\n", pustrDeviceName);

    /* Allocate a GRAPHICS_DEVICE structure */
    pGraphicsDevice = ExAllocatePoolWithTag(PagedPool,
                                            sizeof(GRAPHICS_DEVICE),
                                            GDITAG_GDEVICE);
    if (!pGraphicsDevice)
    {
        ERR("ExAllocatePoolWithTag failed\n");
        return NULL;
    }

    /* Try to open the driver */
    Status = IoGetDeviceObjectPointer(pustrDeviceName,
                                      FILE_READ_DATA | FILE_WRITE_DATA,
                                      &pFileObject,
                                      &pDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not open driver %wZ, 0x%lx\n", pustrDeviceName, Status);
        ExFreePoolWithTag(pGraphicsDevice, GDITAG_GDEVICE);
        return NULL;
    }

    /* Enable the device */
    EngFileWrite(pFileObject, &bEnable, sizeof(BOOL), &cjWritten);

    /* Copy the device and file object pointers */
    pGraphicsDevice->DeviceObject = pDeviceObject;
    pGraphicsDevice->FileObject = pFileObject;

    /* Copy device name */
    RtlStringCbCopyNW(pGraphicsDevice->szNtDeviceName,
                     sizeof(pGraphicsDevice->szNtDeviceName),
                     pustrDeviceName->Buffer,
                     pustrDeviceName->Length);

    /* Create a win device name (FIXME: virtual devices!) */
    swprintf(pGraphicsDevice->szWinDeviceName, L"\\\\.\\DISPLAY%d", (int)giDevNum);

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

    /* Copy display driver names */
    pGraphicsDevice->pDiplayDrivers = pwsz;
    RtlCopyMemory(pGraphicsDevice->pDiplayDrivers,
                  pustrDiplayDrivers->Buffer,
                  pustrDiplayDrivers->Length);

    /* Copy description */
    pGraphicsDevice->pwszDescription = pwsz + pustrDiplayDrivers->Length / sizeof(WCHAR);
    RtlCopyMemory(pGraphicsDevice->pwszDescription,
                  pustrDescription->Buffer,
                  pustrDescription->Length);
    pGraphicsDevice->pwszDescription[pustrDescription->Length/sizeof(WCHAR)] = 0;

    /* Initialize the pdevmodeInfo list and default index  */
    pGraphicsDevice->pdevmodeInfo = NULL;
    pGraphicsDevice->iDefaultMode = 0;
    pGraphicsDevice->iCurrentMode = 0;

    // FIXME: initialize state flags
    pGraphicsDevice->StateFlags = 0;

    /* Create the mode list */
    pGraphicsDevice->pDevModeList = NULL;
    if (!EngpPopulateDeviceModeList(pGraphicsDevice, pdmDefault))
    {
        ExFreePoolWithTag(pGraphicsDevice, GDITAG_GDEVICE);
        return NULL;
    }

    /* Lock loader */
    EngAcquireSemaphore(ghsemGraphicsDeviceList);

    /* Insert the device into the global list */
    pGraphicsDevice->pNextGraphicsDevice = NULL;
    if (gpGraphicsDeviceLast)
        gpGraphicsDeviceLast->pNextGraphicsDevice = pGraphicsDevice;
    gpGraphicsDeviceLast = pGraphicsDevice;
    if (!gpGraphicsDeviceFirst)
        gpGraphicsDeviceFirst = pGraphicsDevice;

    /* Increment device number */
    giDevNum++;

    /* Unlock loader */
    EngReleaseSemaphore(ghsemGraphicsDeviceList);
    TRACE("Prepared %lu modes for %ls\n", pGraphicsDevice->cDevModes, pGraphicsDevice->pwszDescription);

    return pGraphicsDevice;
}


PGRAPHICS_DEVICE
NTAPI
EngpFindGraphicsDevice(
    _In_opt_ PUNICODE_STRING pustrDevice,
    _In_ ULONG iDevNum,
    _In_ DWORD dwFlags)
{
    UNICODE_STRING ustrCurrent;
    PGRAPHICS_DEVICE pGraphicsDevice;
    ULONG i;
    TRACE("EngpFindGraphicsDevice('%wZ', %lu, 0x%lx)\n",
           pustrDevice, iDevNum, dwFlags);

    /* Lock list */
    EngAcquireSemaphore(ghsemGraphicsDeviceList);

    if (pustrDevice && pustrDevice->Buffer)
    {
        /* Loop through the list of devices */
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
    }
    else
    {
        /* Loop through the list of devices */
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
