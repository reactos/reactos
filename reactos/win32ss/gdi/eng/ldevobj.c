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

INIT_FUNCTION
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

PDEVMODEINFO
NTAPI
LDEVOBJ_pdmiGetModes(
    _In_ PLDEVOBJ pldev,
    _In_ HANDLE hDriver)
{
    ULONG cbSize, cbFull;
    PDEVMODEINFO pdminfo;

    TRACE("LDEVOBJ_pdmiGetModes(%p, %p)\n", pldev, hDriver);

    /* Call the driver to get the required size */
    cbSize = pldev->pfn.GetModes(hDriver, 0, NULL);
    if (!cbSize)
    {
        ERR("DrvGetModes returned 0\n");
        return NULL;
    }

    /* Add space for the header */
    cbFull = cbSize + FIELD_OFFSET(DEVMODEINFO, adevmode);

    /* Allocate a buffer for the DEVMODE array */
    pdminfo = ExAllocatePoolWithTag(PagedPool, cbFull, GDITAG_DEVMODE);
    if (!pdminfo)
    {
        ERR("Could not allocate devmodeinfo\n");
        return NULL;
    }

    pdminfo->pldev = pldev;
    pdminfo->cbdevmode = cbSize;

    /* Call the driver again to fill the buffer */
    cbSize = pldev->pfn.GetModes(hDriver, cbSize, pdminfo->adevmode);
    if (!cbSize)
    {
        /* Could not get modes */
        ERR("returned size %lu(%lu)\n", cbSize, pdminfo->cbdevmode);
        ExFreePoolWithTag(pdminfo, GDITAG_DEVMODE);
        pdminfo = NULL;
    }

    return pdminfo;
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
VOID
LDEVOBJ_vUnloadImage(
    _Inout_ PLDEVOBJ pldev)
{
    NTSTATUS Status;

    /* Make sure we have a driver info */
    ASSERT(pldev && pldev->pGdiDriverInfo != NULL);

    /* Check if we have loaded a driver */
    if (pldev->pfn.DisableDriver)
    {
        /* Call the unload function */
        pldev->pfn.DisableDriver();
    }

    /* Unload the driver */
    Status = ZwSetSystemInformation(SystemUnloadGdiDriverInformation,
                                    &pldev->pGdiDriverInfo->SectionPointer,
                                    sizeof(HANDLE));
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to unload the driver, this is bad.\n");
    }

    /* Free the driver info structure */
    ExFreePoolWithTag(pldev->pGdiDriverInfo, GDITAG_LDEV);
    pldev->pGdiDriverInfo = NULL;
}

static
BOOL
LDEVOBJ_bEnableDriver(
    _Inout_ PLDEVOBJ pldev)
{
    PFN_DrvEnableDriver pfnEnableDriver;
    DRVENABLEDATA ded;
    ULONG i;

    /* Make sure we have a driver info */
    ASSERT(pldev && pldev->pGdiDriverInfo != NULL);

    /* Call the drivers DrvEnableDriver function */
    RtlZeroMemory(&ded, sizeof(ded));
    pfnEnableDriver = pldev->pGdiDriverInfo->EntryPoint;
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

PLDEVOBJ
NTAPI
EngLoadImageEx(
    _In_z_ LPWSTR pwszDriverName,
    _In_ ULONG ldevtype)
{
    WCHAR acwBuffer[MAX_PATH];
    PLIST_ENTRY pleLink;
    PLDEVOBJ pldev;
    UNICODE_STRING strDriverName;
    SIZE_T cwcLength;
    LPWSTR pwsz;

    TRACE("EngLoadImageEx(%ls, %lu)\n", pwszDriverName, ldevtype);
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

        /* Shall we load a driver? */
        if (ldevtype != LDEV_IMAGE)
        {
            /* Load the driver */
            if (!LDEVOBJ_bEnableDriver(pldev))
            {
                ERR("LDEVOBJ_bEnableDriver failed\n");

                /* Unload the image. */
                LDEVOBJ_vUnloadImage(pldev);
                LDEVOBJ_vFreeLDEV(pldev);
                pldev = NULL;
                goto leave;
            }
        }

        /* Insert the LDEV into the global list */
        InsertHeadList(&gleLdevListHead, &pldev->leLink);
    }

    /* Increase ref count */
    pldev->cRefs++;

leave:
    /* Unlock loader */
    EngReleaseSemaphore(ghsemLDEVList);

    TRACE("EngLoadImageEx returning %p\n", pldev);
    return pldev;
}


/** Exported functions ********************************************************/

HANDLE
APIENTRY
EngLoadImage(
    _In_ LPWSTR pwszDriverName)
{
    return (HANDLE)EngLoadImageEx(pwszDriverName, LDEV_IMAGE);
}


VOID
APIENTRY
EngUnloadImage(
    _In_ HANDLE hModule)
{
    PLDEVOBJ pldev = (PLDEVOBJ)hModule;

    /* Make sure the LDEV is in the list */
    ASSERT((pldev->leLink.Flink != NULL) &&  (pldev->leLink.Blink != NULL));

    /* Lock loader */
    EngAcquireSemaphore(ghsemLDEVList);

    /* Decrement reference count */
    pldev->cRefs--;

    /* No more references left? */
    if (pldev->cRefs == 0)
    {
        /* Remove ldev from the list */
        RemoveEntryList(&pldev->leLink);

        /* Unload the image and free the LDEV */
        LDEVOBJ_vUnloadImage(pldev);
        LDEVOBJ_vFreeLDEV(pldev);
    }

    /* Unlock loader */
    EngReleaseSemaphore(ghsemLDEVList);
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
