/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Support for logical devices
 * FILE:             win32ss/gdi/eng/ldevobj.c
 * PROGRAMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#define NDEBUG
#include <debug.h>
DBG_DEFAULT_CHANNEL(EngLDev);

#ifndef RVA_TO_ADDR
#define RVA_TO_ADDR(Base,Rva) ((PVOID)(((ULONG_PTR)(Base)) + (Rva)))
#endif

/** Globals *******************************************************************/

static HSEMAPHORE ghsemLDEVList;
static LIST_ENTRY gleLdevListHead;
static LDEVOBJ *gpldevWin32k = NULL;


/** Private functions *********************************************************/

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitLDEVImpl(VOID)
{
    ULONG cbSize;

    /* Initialize the LDEV list head */
    InitializeListHead(&gleLdevListHead);

    /* Initialize the loader lock */
    ghsemLDEVList = EngCreateSemaphore();
    if (!ghsemLDEVList)
    {
        ERR("Failed to create ghsemLDEVList\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate a LDEVOBJ for win32k */
    gpldevWin32k = ExAllocatePoolWithTag(PagedPool,
                                         sizeof(LDEVOBJ) +
                                         sizeof(SYSTEM_GDI_DRIVER_INFORMATION),
                                         GDITAG_LDEV);
    if (!gpldevWin32k)
    {
        return STATUS_NO_MEMORY;
    }

    /* Initialize the LDEVOBJ for win32k */
    gpldevWin32k->leLink.Flink = NULL;
    gpldevWin32k->leLink.Blink = NULL;
    gpldevWin32k->ldevtype = LDEV_DEVICE_DISPLAY;
    gpldevWin32k->cRefs = 1;
    gpldevWin32k->ulDriverVersion = GDI_ENGINE_VERSION;
    gpldevWin32k->pGdiDriverInfo = (PVOID)(gpldevWin32k + 1);
    RtlInitUnicodeString(&gpldevWin32k->pGdiDriverInfo->DriverName,
                         L"\\SystemRoot\\System32\\win32k.sys");
    gpldevWin32k->pGdiDriverInfo->ImageAddress = &__ImageBase;
    gpldevWin32k->pGdiDriverInfo->SectionPointer = NULL;
    gpldevWin32k->pGdiDriverInfo->EntryPoint = (PVOID)DriverEntry;
    gpldevWin32k->pGdiDriverInfo->ExportSectionPointer =
        RtlImageDirectoryEntryToData(&__ImageBase,
                                     TRUE,
                                     IMAGE_DIRECTORY_ENTRY_EXPORT,
                                     &cbSize);
    gpldevWin32k->pGdiDriverInfo->ImageLength = 0; // FIXME

    return STATUS_SUCCESS;
}

static
PLDEVOBJ
LDEVOBJ_AllocLDEV(
    _In_ LDEVTYPE ldevtype)
{
    PLDEVOBJ pldev;

    /* Allocate the structure from paged pool */
    pldev = ExAllocatePoolWithTag(PagedPool, sizeof(LDEVOBJ), GDITAG_LDEV);
    if (!pldev)
    {
        ERR("Failed to allocate LDEVOBJ.\n");
        return NULL;
    }

    /* Zero out the structure */
    RtlZeroMemory(pldev, sizeof(LDEVOBJ));

    /* Set the ldevtype */
    pldev->ldevtype = ldevtype;

    return pldev;
}

static
VOID
LDEVOBJ_vFreeLDEV(
    _In_ _Post_ptr_invalid_ PLDEVOBJ pldev)
{
    /* Make sure we don't have a driver loaded */
    ASSERT(pldev && pldev->pGdiDriverInfo == NULL);
    ASSERT(pldev->cRefs == 0);

    /* Free the memory */
    ExFreePoolWithTag(pldev, GDITAG_LDEV);
}

static
BOOL
LDEVOBJ_bLoadImage(
    _Inout_ PLDEVOBJ pldev,
    _In_ PUNICODE_STRING pustrPathName)
{
    PSYSTEM_GDI_DRIVER_INFORMATION pDriverInfo;
    NTSTATUS Status;
    ULONG cbSize;

    /* Make sure no image is loaded yet */
    ASSERT(pldev && pldev->pGdiDriverInfo == NULL);

    /* Allocate a SYSTEM_GDI_DRIVER_INFORMATION structure */
    cbSize = sizeof(SYSTEM_GDI_DRIVER_INFORMATION) + pustrPathName->Length;
    pDriverInfo = ExAllocatePoolWithTag(PagedPool, cbSize, GDITAG_LDEV);
    if (!pDriverInfo)
    {
        ERR("Failed to allocate SYSTEM_GDI_DRIVER_INFORMATION\n");
        return FALSE;
    }

    /* Initialize the UNICODE_STRING and copy the driver name */
    RtlInitEmptyUnicodeString(&pDriverInfo->DriverName,
                              (PWSTR)(pDriverInfo + 1),
                              pustrPathName->Length);
    RtlCopyUnicodeString(&pDriverInfo->DriverName, pustrPathName);

    /* Try to load the driver */
    Status = ZwSetSystemInformation(SystemLoadGdiDriverInformation,
                                    pDriverInfo,
                                    sizeof(SYSTEM_GDI_DRIVER_INFORMATION));
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to load a GDI driver: '%wZ', Status = 0x%lx\n",
            pustrPathName, Status);

        /* Free the allocated memory */
        ExFreePoolWithTag(pDriverInfo, GDITAG_LDEV);
        return FALSE;
    }

    /* Set the driver info */
    pldev->pGdiDriverInfo = pDriverInfo;

    /* Return success. */
    return TRUE;
}

static
BOOL
LDEVOBJ_bEnableDriver(
    _Inout_ PLDEVOBJ pldev,
    _In_ PFN_DrvEnableDriver pfnEnableDriver)
{
    DRVENABLEDATA ded;
    ULONG i;

    TRACE("LDEVOBJ_bEnableDriver('%wZ')\n", &pldev->pGdiDriverInfo->DriverName);

    ASSERT(pldev);
    ASSERT(pldev->cRefs == 0);

    if (pldev->ldevtype == LDEV_IMAGE)
        return TRUE;

    /* Call the drivers DrvEnableDriver function */
    RtlZeroMemory(&ded, sizeof(ded));
    if (!pfnEnableDriver(GDI_ENGINE_VERSION, sizeof(ded), &ded))
    {
        ERR("DrvEnableDriver failed\n");
        return FALSE;
    }

    /* Copy the returned driver version */
    pldev->ulDriverVersion = ded.iDriverVersion;

    /* Fill the driver function array */
    for (i = 0; i < ded.c; i++)
    {
        pldev->apfn[ded.pdrvfn[i].iFunc] = ded.pdrvfn[i].pfn;
    }

    /* Return success. */
    return TRUE;
}

static
VOID
LDEVOBJ_vDisableDriver(
    _Inout_ PLDEVOBJ pldev)
{
    ASSERT(pldev);
    ASSERT(pldev->cRefs == 0);

    TRACE("LDEVOBJ_vDisableDriver('%wZ')\n", &pldev->pGdiDriverInfo->DriverName);

    if (pldev->ldevtype == LDEV_IMAGE)
        return;

    if (pldev->pfn.DisableDriver)
    {
        /* Call the unload function */
        pldev->pfn.DisableDriver();
    }
}

static
PVOID
LDEVOBJ_pvFindImageProcAddress(
    _In_ PLDEVOBJ pldev,
    _In_z_ LPSTR pszProcName)
{
    PVOID pvImageBase;
    PIMAGE_EXPORT_DIRECTORY pExportDir;
    PVOID pvProcAdress = NULL;
    PUSHORT pOrdinals;
    PULONG pNames, pAddresses;
    ULONG i;

    /* Make sure we have a driver info */
    ASSERT(pldev && pldev->pGdiDriverInfo != NULL);

    /* Get the pointer to the export directory */
    pvImageBase = pldev->pGdiDriverInfo->ImageAddress;
    pExportDir = pldev->pGdiDriverInfo->ExportSectionPointer;
    if (!pExportDir)
    {
        ERR("LDEVOBJ_pvFindImageProcAddress: no export section found\n");
        return NULL;
    }

    /* Get pointers to some tables */
    pNames = RVA_TO_ADDR(pvImageBase, pExportDir->AddressOfNames);
    pOrdinals = RVA_TO_ADDR(pvImageBase, pExportDir->AddressOfNameOrdinals);
    pAddresses = RVA_TO_ADDR(pvImageBase, pExportDir->AddressOfFunctions);

    /* Loop the export table */
    for (i = 0; i < pExportDir->NumberOfNames; i++)
    {
        /* Compare the name */
        if (_stricmp(pszProcName, RVA_TO_ADDR(pvImageBase, pNames[i])) == 0)
        {
            /* Found! Calculate the procedure address */
            pvProcAdress = RVA_TO_ADDR(pvImageBase, pAddresses[pOrdinals[i]]);
            break;
        }
    }

    /* Return the address */
    return pvProcAdress;
}

static
BOOL
LDEVOBJ_bUnloadImage(
    _Inout_ PLDEVOBJ pldev)
{
    NTSTATUS Status;

    /* Make sure we have a driver info */
    ASSERT(pldev && pldev->pGdiDriverInfo != NULL);
    ASSERT(pldev->cRefs == 0);

    TRACE("LDEVOBJ_bUnloadImage('%wZ')\n", &pldev->pGdiDriverInfo->DriverName);

    /* Unload the driver */
#if 0
    Status = ZwSetSystemInformation(SystemUnloadGdiDriverInformation,
                                    &pldev->pGdiDriverInfo->SectionPointer,
                                    sizeof(HANDLE));
#else
    /* Unfortunately, ntoskrnl allows unloading a driver, but fails loading
     * it again with STATUS_IMAGE_ALREADY_LOADED. Prevent this problem by
     * never unloading any driver.
     */
    UNIMPLEMENTED;
    Status = STATUS_NOT_IMPLEMENTED;
#endif
    if (!NT_SUCCESS(Status))
        return FALSE;

    ExFreePoolWithTag(pldev->pGdiDriverInfo, GDITAG_LDEV);
    pldev->pGdiDriverInfo = NULL;

    return TRUE;
}

PLDEVOBJ
LDEVOBJ_pLoadInternal(
    _In_ PFN_DrvEnableDriver pfnEnableDriver,
    _In_ ULONG ldevtype)
{
    PLDEVOBJ pldev;

    TRACE("LDEVOBJ_pLoadInternal(%lu)\n", ldevtype);

    /* Lock loader */
    EngAcquireSemaphore(ghsemLDEVList);

    /* Allocate a new LDEVOBJ */
    pldev = LDEVOBJ_AllocLDEV(ldevtype);
    if (!pldev)
    {
        ERR("Could not allocate LDEV\n");
        goto leave;
    }

    /* Load the driver */
    if (!LDEVOBJ_bEnableDriver(pldev, pfnEnableDriver))
    {
        ERR("LDEVOBJ_bEnableDriver failed\n");
        LDEVOBJ_vFreeLDEV(pldev);
        pldev = NULL;
        goto leave;
    }

    /* Insert the LDEV into the global list */
    InsertHeadList(&gleLdevListHead, &pldev->leLink);

    /* Increase ref count */
    pldev->cRefs++;

leave:
    /* Unlock loader */
    EngReleaseSemaphore(ghsemLDEVList);

    TRACE("LDEVOBJ_pLoadInternal returning %p\n", pldev);
    return pldev;
}

PLDEVOBJ
NTAPI
LDEVOBJ_pLoadDriver(
    _In_z_ LPWSTR pwszDriverName,
    _In_ ULONG ldevtype)
{
    WCHAR acwBuffer[MAX_PATH];
    PLIST_ENTRY pleLink;
    PLDEVOBJ pldev;
    UNICODE_STRING strDriverName;
    SIZE_T cwcLength;
    LPWSTR pwsz;

    TRACE("LDEVOBJ_pLoadDriver(%ls, %lu)\n", pwszDriverName, ldevtype);
    ASSERT(pwszDriverName);

    /* Initialize buffer for the the driver name */
    RtlInitEmptyUnicodeString(&strDriverName, acwBuffer, sizeof(acwBuffer));

    /* Start path with systemroot */
    RtlAppendUnicodeToString(&strDriverName, L"\\SystemRoot\\System32\\");

    /* Get Length of given string */
    cwcLength = wcslen(pwszDriverName);

    /* Check if we have a system32 path given */
    pwsz = pwszDriverName + cwcLength;
    while (pwsz > pwszDriverName)
    {
        if ((*pwsz == L'\\') && (_wcsnicmp(pwsz, L"\\system32\\", 10) == 0))
        {
            /* Driver name starts after system32 */
            pwsz += 10;
            break;
        }
        pwsz--;
    }

    /* Append the driver name */
    RtlAppendUnicodeToString(&strDriverName, pwsz);

    /* MSDN says "The driver must include this suffix in the pwszDriver string."
       But in fact it's optional. The function can also load .sys files without
       appending the .dll extension. */
    if ((cwcLength < 4) ||
        ((_wcsnicmp(pwszDriverName + cwcLength - 4, L".dll", 4) != 0) &&
         (_wcsnicmp(pwszDriverName + cwcLength - 4, L".sys", 4) != 0)) )
    {
        /* Append the .dll suffix */
        RtlAppendUnicodeToString(&strDriverName, L".dll");
    }

    /* Lock loader */
    EngAcquireSemaphore(ghsemLDEVList);

    /* Search the List of LDEVS for the driver name */
    for (pleLink = gleLdevListHead.Flink;
         pleLink != &gleLdevListHead;
         pleLink = pleLink->Flink)
    {
        pldev = CONTAINING_RECORD(pleLink, LDEVOBJ, leLink);

        /* Check if the ldev is associated with a file */
        if (pldev->pGdiDriverInfo)
        {
            /* Check for match (case insensative) */
            if (RtlEqualUnicodeString(&pldev->pGdiDriverInfo->DriverName, &strDriverName, TRUE))
            {
                /* Image found in LDEV list */
                break;
            }
        }
    }

    /* Did we find one? */
    if (pleLink == &gleLdevListHead)
    {
        /* No, allocate a new LDEVOBJ */
        pldev = LDEVOBJ_AllocLDEV(ldevtype);
        if (!pldev)
        {
            ERR("Could not allocate LDEV\n");
            goto leave;
        }

        /* Load the image */
        if (!LDEVOBJ_bLoadImage(pldev, &strDriverName))
        {
            LDEVOBJ_vFreeLDEV(pldev);
            pldev = NULL;
            ERR("LDEVOBJ_bLoadImage failed\n");
            goto leave;
        }

        /* Load the driver */
        if (!LDEVOBJ_bEnableDriver(pldev, pldev->pGdiDriverInfo->EntryPoint))
        {
            ERR("LDEVOBJ_bEnableDriver failed\n");

            /* Unload the image. */
            LDEVOBJ_bUnloadImage(pldev);
            LDEVOBJ_vFreeLDEV(pldev);
            pldev = NULL;
            goto leave;
        }

        /* Insert the LDEV into the global list */
        InsertHeadList(&gleLdevListHead, &pldev->leLink);
    }

    /* Increase ref count */
    pldev->cRefs++;

leave:
    /* Unlock loader */
    EngReleaseSemaphore(ghsemLDEVList);

    TRACE("LDEVOBJ_pLoadDriver returning %p\n", pldev);
    return pldev;
}

static
VOID
LDEVOBJ_vDereference(
    _Inout_ PLDEVOBJ pldev)
{
    /* Lock loader */
    EngAcquireSemaphore(ghsemLDEVList);

    /* Decrement reference count */
    ASSERT(pldev->cRefs > 0);
    pldev->cRefs--;

    /* More references left? */
    if (pldev->cRefs > 0)
    {
        EngReleaseSemaphore(ghsemLDEVList);
        return;
    }

    LDEVOBJ_vDisableDriver(pldev);

    if (LDEVOBJ_bUnloadImage(pldev))
    {
        /* Remove ldev from the list */
        RemoveEntryList(&pldev->leLink);

        /* Free the driver info structure */
        LDEVOBJ_vFreeLDEV(pldev);
    }
    else
    {
        WARN("Failed to unload driver '%wZ', trying to re-enable it.\n", &pldev->pGdiDriverInfo->DriverName);
        LDEVOBJ_bEnableDriver(pldev, pldev->pGdiDriverInfo->EntryPoint);

        /* Increment again reference count */
        pldev->cRefs++;
    }

    /* Unlock loader */
    EngReleaseSemaphore(ghsemLDEVList);
}

ULONG
LDEVOBJ_ulGetDriverModes(
    _In_ LPWSTR pwszDriverName,
    _In_ HANDLE hDriver,
    _Out_ PDEVMODEW *ppdm)
{
    PLDEVOBJ pldev = NULL;
    ULONG cbSize = 0;
    PDEVMODEW pdm = NULL;

    TRACE("LDEVOBJ_ulGetDriverModes('%ls', %p)\n", pwszDriverName, hDriver);

    pldev = LDEVOBJ_pLoadDriver(pwszDriverName, LDEV_DEVICE_DISPLAY);
    if (!pldev)
        goto cleanup;

    /* Mirror drivers may omit this function */
    if (!pldev->pfn.GetModes)
        goto cleanup;

    /* Call the driver to get the required size */
    cbSize = pldev->pfn.GetModes(hDriver, 0, NULL);
    if (!cbSize)
    {
        ERR("DrvGetModes returned 0\n");
        goto cleanup;
    }

    /* Allocate a buffer for the DEVMODE array */
    pdm = ExAllocatePoolWithTag(PagedPool, cbSize, GDITAG_DEVMODE);
    if (!pdm)
    {
        ERR("Could not allocate devmodeinfo\n");
        goto cleanup;
    }

    /* Call the driver again to fill the buffer */
    cbSize = pldev->pfn.GetModes(hDriver, cbSize, pdm);
    if (!cbSize)
    {
        /* Could not get modes */
        ERR("DrvrGetModes returned 0 on second call\n");
        ExFreePoolWithTag(pdm, GDITAG_DEVMODE);
        pdm = NULL;
    }

cleanup:
    if (pldev)
        LDEVOBJ_vDereference(pldev);

    *ppdm = pdm;
    return cbSize;
}

BOOL
LDEVOBJ_bBuildDevmodeList(
    _Inout_ PGRAPHICS_DEVICE pGraphicsDevice)
{
    PWSTR pwsz;
    PDEVMODEINFO pdminfo;
    PDEVMODEW pdm, pdmEnd;
    ULONG i, cModes = 0;
    ULONG cbSize, cbFull;

    if (pGraphicsDevice->pdevmodeInfo)
        return TRUE;
    ASSERT(pGraphicsDevice->pDevModeList == NULL);

    pwsz = pGraphicsDevice->pDiplayDrivers;

    /* Loop through the driver names
     * This is a REG_MULTI_SZ string */
    for (; *pwsz; pwsz += wcslen(pwsz) + 1)
    {
        /* Get the mode list from the driver */
        TRACE("Trying driver: %ls\n", pwsz);
        cbSize = LDEVOBJ_ulGetDriverModes(pwsz, pGraphicsDevice->DeviceObject, &pdm);
        if (!cbSize)
        {
            WARN("Driver %ls returned no valid mode\n", pwsz);
            continue;
        }

        /* Add space for the header */
        cbFull = cbSize + FIELD_OFFSET(DEVMODEINFO, adevmode);

        /* Allocate a buffer for the DEVMODE array */
        pdminfo = ExAllocatePoolWithTag(PagedPool, cbFull, GDITAG_DEVMODE);
        if (!pdminfo)
        {
            ERR("Could not allocate devmodeinfo\n");
            ExFreePoolWithTag(pdm, GDITAG_DEVMODE);
            continue;
        }

        pdminfo->cbdevmode = cbSize;
        RtlCopyMemory(pdminfo->adevmode, pdm, cbSize);
        ExFreePoolWithTag(pdm, GDITAG_DEVMODE);

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
            RtlStringCbCopyW(pdm->dmDeviceName, sizeof(pdm->dmDeviceName), pwsz);
        }
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

            /* Initialize the entry */
            pGraphicsDevice->pDevModeList[i].dwFlags = 0;
            pGraphicsDevice->pDevModeList[i].pdm = pdm;
            i++;
        }
    }
    return TRUE;
}

static
BOOL
LDEVOBJ_bGetClosestMode(
    _Inout_ PGRAPHICS_DEVICE pGraphicsDevice,
    _In_ PDEVMODEW RequestedMode,
    _Out_ PDEVMODEW *pSelectedMode)
{
    if (pGraphicsDevice->cDevModes == 0)
        return FALSE;

    /* Search a 32bit mode (if not already specified) */
    if (!(RequestedMode->dmFields & DM_BITSPERPEL))
    {
        RequestedMode->dmBitsPerPel = 32;
        RequestedMode->dmFields |= DM_BITSPERPEL;
    }
    if (LDEVOBJ_bProbeAndCaptureDevmode(pGraphicsDevice, RequestedMode, pSelectedMode, FALSE))
        return TRUE;

    /* Fall back to first mode */
    WARN("Fall back to first available mode\n");
    *pSelectedMode = pGraphicsDevice->pDevModeList[0].pdm;
    return TRUE;
}

BOOL
LDEVOBJ_bProbeAndCaptureDevmode(
    _Inout_ PGRAPHICS_DEVICE pGraphicsDevice,
    _In_ PDEVMODEW RequestedMode,
    _Out_ PDEVMODEW *pSelectedMode,
    _In_ BOOL bSearchClosestMode)
{
    PDEVMODEW pdmCurrent, pdm, pdmSelected = NULL;
    ULONG i;
    DWORD dwFields;

    if (!LDEVOBJ_bBuildDevmodeList(pGraphicsDevice))
        return FALSE;

    if (bSearchClosestMode)
    {
        /* Search the closest mode */
        if (!LDEVOBJ_bGetClosestMode(pGraphicsDevice, RequestedMode, &pdmSelected))
            return FALSE;
        ASSERT(pdmSelected);
    }
    else
    {
        /* Search if requested mode exists */
        for (i = 0; i < pGraphicsDevice->cDevModes; i++)
        {
            pdmCurrent = pGraphicsDevice->pDevModeList[i].pdm;

            /* Compare asked DEVMODE fields
             * Only compare those that are valid in both DEVMODE structs */
            dwFields = pdmCurrent->dmFields & RequestedMode->dmFields;

            /* For now, we only need those */
            if ((dwFields & DM_BITSPERPEL) &&
                (pdmCurrent->dmBitsPerPel != RequestedMode->dmBitsPerPel)) continue;
            if ((dwFields & DM_PELSWIDTH) &&
                (pdmCurrent->dmPelsWidth != RequestedMode->dmPelsWidth)) continue;
            if ((dwFields & DM_PELSHEIGHT) &&
                (pdmCurrent->dmPelsHeight != RequestedMode->dmPelsHeight)) continue;
            if ((dwFields & DM_DISPLAYFREQUENCY) &&
                (pdmCurrent->dmDisplayFrequency != RequestedMode->dmDisplayFrequency)) continue;

            pdmSelected = pdmCurrent;
            break;
        }

        if (!pdmSelected)
        {
            WARN("Requested mode not found (%dx%dx%d %d Hz)\n",
                RequestedMode->dmFields & DM_PELSWIDTH ? RequestedMode->dmPelsWidth : 0,
                RequestedMode->dmFields & DM_PELSHEIGHT ? RequestedMode->dmPelsHeight : 0,
                RequestedMode->dmFields & DM_BITSPERPEL ? RequestedMode->dmBitsPerPel : 0,
                RequestedMode->dmFields & DM_DISPLAYFREQUENCY ? RequestedMode->dmDisplayFrequency : 0);
            return FALSE;
        }
    }

    /* Allocate memory for output */
    pdm = ExAllocatePoolZero(PagedPool, pdmSelected->dmSize + pdmSelected->dmDriverExtra, GDITAG_DEVMODE);
    if (!pdm)
        return FALSE;

    /* Copy selected mode */
    RtlCopyMemory(pdm, pdmSelected, pdmSelected->dmSize);
    RtlCopyMemory((PVOID)((ULONG_PTR)pdm + pdm->dmSize),
                  (PVOID)((ULONG_PTR)pdmSelected + pdmSelected->dmSize),
                  pdmSelected->dmDriverExtra);

    *pSelectedMode = pdm;
    return TRUE;
}

/** Exported functions ********************************************************/

HANDLE
APIENTRY
EngLoadImage(
    _In_ LPWSTR pwszDriverName)
{
    return (HANDLE)LDEVOBJ_pLoadDriver(pwszDriverName, LDEV_IMAGE);
}


VOID
APIENTRY
EngUnloadImage(
    _In_ HANDLE hModule)
{
    PLDEVOBJ pldev = (PLDEVOBJ)hModule;

    /* Make sure the LDEV is in the list */
    ASSERT((pldev->leLink.Flink != NULL) &&  (pldev->leLink.Blink != NULL));

    LDEVOBJ_vDereference(pldev);
}


PVOID
APIENTRY
EngFindImageProcAddress(
    _In_ HANDLE hModule,
    _In_ LPSTR  lpProcName)
{
    PLDEVOBJ pldev = (PLDEVOBJ)hModule;

    ASSERT(gpldevWin32k != NULL);

    /* Check if win32k is requested */
    if (!pldev)
    {
        pldev = gpldevWin32k;
    }

    /* Check if the drivers entry point is requested */
    if (_strnicmp(lpProcName, "DrvEnableDriver", 15) == 0)
    {
        return pldev->pGdiDriverInfo->EntryPoint;
    }

    /* Try to find the address */
    return LDEVOBJ_pvFindImageProcAddress(pldev, lpProcName);
}

/* EOF */
