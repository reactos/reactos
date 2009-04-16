/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Functions for creation and destruction of DCs
 * FILE:              subsystem/win32/win32k/objects/device.c
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@rectos.org)
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

// TODO: proper implementation of LDEVOBJ and PDEVOBJ interface

/*static*/ PDEVOBJ PrimarySurface;
PPDEVOBJ pPrimarySurface = &PrimarySurface;
static KEVENT VideoDriverNeedsPreparation;
static KEVENT VideoDriverPrepared;
PDC defaultDCstate = NULL;


NTSTATUS FASTCALL
InitDcImpl(VOID)
{
    KeInitializeEvent(&VideoDriverNeedsPreparation, SynchronizationEvent, TRUE);
    KeInitializeEvent(&VideoDriverPrepared, NotificationEvent, FALSE);
    return STATUS_SUCCESS;
}


static BOOLEAN FASTCALL
GetRegistryPath(PUNICODE_STRING RegistryPath, ULONG DisplayNumber)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    WCHAR DeviceNameBuffer[20];
    NTSTATUS Status;

    swprintf(DeviceNameBuffer, L"\\Device\\Video%lu", DisplayNumber);
    RtlInitUnicodeString(RegistryPath, NULL);
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].Name = DeviceNameBuffer;
    QueryTable[0].EntryContext = RegistryPath;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_DEVICEMAP,
                                    L"VIDEO",
                                    QueryTable,
                                    NULL,
                                    NULL);
    if (! NT_SUCCESS(Status))
    {
        DPRINT1("No \\Device\\Video%lu value in DEVICEMAP\\VIDEO found\n", DisplayNumber);
        return FALSE;
    }

    DPRINT("RegistryPath %wZ\n", RegistryPath);

    return TRUE;
}

static BOOL FASTCALL
FindDriverFileNames(PUNICODE_STRING DriverFileNames, ULONG DisplayNumber)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    UNICODE_STRING RegistryPath;
    NTSTATUS Status;

    if (! GetRegistryPath(&RegistryPath, DisplayNumber))
    {
        DPRINT("GetRegistryPath failed\n");
        return FALSE;
    }

    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].Name = L"InstalledDisplayDrivers";
    QueryTable[0].EntryContext = DriverFileNames;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    RegistryPath.Buffer,
                                    QueryTable,
                                    NULL,
                                    NULL);
    ExFreePoolWithTag(RegistryPath.Buffer, TAG_RTLREGISTRY);
    if (! NT_SUCCESS(Status))
    {
        DPRINT1("No InstalledDisplayDrivers value in service entry found\n");
        return FALSE;
    }

    DPRINT("DriverFileNames %S\n", DriverFileNames->Buffer);

    return TRUE;
}


static NTSTATUS APIENTRY
DevModeCallback(IN PWSTR ValueName,
                IN ULONG ValueType,
                IN PVOID ValueData,
                IN ULONG ValueLength,
                IN PVOID Context,
                IN PVOID EntryContext)
{
    PDEVMODEW DevMode = (PDEVMODEW) Context;

    DPRINT("Found registry value for name %S: type %d, length %d\n",
           ValueName, ValueType, ValueLength);

    if (REG_DWORD == ValueType && sizeof(DWORD) == ValueLength)
    {
        if (0 == _wcsicmp(ValueName, L"DefaultSettings.BitsPerPel"))
        {
            DevMode->dmBitsPerPel = *((DWORD *) ValueData);
        }
        else if (0 == _wcsicmp(ValueName, L"DefaultSettings.Flags"))
        {
            DevMode->dmDisplayFlags = *((DWORD *) ValueData);
        }
        else if (0 == _wcsicmp(ValueName, L"DefaultSettings.VRefresh"))
        {
            DevMode->dmDisplayFrequency = *((DWORD *) ValueData);
        }
        else if (0 == _wcsicmp(ValueName, L"DefaultSettings.XPanning"))
        {
            DevMode->dmPanningWidth = *((DWORD *) ValueData);
        }
        else if (0 == _wcsicmp(ValueName, L"DefaultSettings.XResolution"))
        {
            DevMode->dmPelsWidth = *((DWORD *) ValueData);
        }
        else if (0 == _wcsicmp(ValueName, L"DefaultSettings.YPanning"))
        {
            DevMode->dmPanningHeight = *((DWORD *) ValueData);
        }
        else if (0 == _wcsicmp(ValueName, L"DefaultSettings.YResolution"))
        {
            DevMode->dmPelsHeight = *((DWORD *) ValueData);
        }
    }

    return STATUS_SUCCESS;
}


static BOOL FASTCALL
SetupDevMode(PDEVMODEW DevMode, ULONG DisplayNumber)
{
    UNICODE_STRING RegistryPath;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    NTSTATUS Status;
    BOOLEAN Valid = TRUE;

    if (!GetRegistryPath(&RegistryPath, DisplayNumber))
    {
        DPRINT("GetRegistryPath failed\n");
        return FALSE;
    }

    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = DevModeCallback;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = NULL;
    QueryTable[0].EntryContext = NULL;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    RegistryPath.Buffer,
                                    QueryTable,
                                    DevMode,
                                    NULL);
    if (! NT_SUCCESS(Status))
    {
        DPRINT("RtlQueryRegistryValues for %wZ failed with status 0x%08x\n",
               &RegistryPath, Status);
        Valid = FALSE;
    }
    else
    {
        DPRINT("dmBitsPerPel %d dmDisplayFrequency %d dmPelsWidth %d dmPelsHeight %d\n",
               DevMode->dmBitsPerPel, DevMode->dmDisplayFrequency,
               DevMode->dmPelsWidth, DevMode->dmPelsHeight);
        if (0 == DevMode->dmBitsPerPel || 0 == DevMode->dmDisplayFrequency
                || 0 == DevMode->dmPelsWidth || 0 == DevMode->dmPelsHeight)
        {
            DPRINT("Not all required devmode members are set\n");
            Valid = FALSE;
        }
    }

    ExFreePoolWithTag(RegistryPath.Buffer, TAG_RTLREGISTRY);

    if (! Valid)
    {
        RtlZeroMemory(DevMode, sizeof(DEVMODEW));
    }

    return Valid;
}

static BOOL FASTCALL
IntPrepareDriver()
{
    PFN_DrvEnableDriver GDEnableDriver;
    DRVENABLEDATA DED;
    UNICODE_STRING DriverFileNames;
    PWSTR CurrentName;
    BOOL GotDriver;
    BOOL DoDefault;
    ULONG DisplayNumber;
    LARGE_INTEGER Zero;
    BOOLEAN ret = FALSE;

    Zero.QuadPart = 0;
    if (STATUS_SUCCESS != KeWaitForSingleObject(&VideoDriverNeedsPreparation, Executive, KernelMode, TRUE, &Zero))
    {
        /* Concurrent access. Wait for VideoDriverPrepared event */
        if (STATUS_SUCCESS == KeWaitForSingleObject(&VideoDriverPrepared, Executive, KernelMode, TRUE, NULL))
            ret = PrimarySurface.PreparedDriver;
        goto cleanup;
    }
    // HAX! Fixme so I can support more than one! So how many?
    for (DisplayNumber = 0; ; DisplayNumber++)
    {
        DPRINT("Trying to load display driver no. %d\n", DisplayNumber);

        RtlZeroMemory(&PrimarySurface, sizeof(PrimarySurface));

//      if (!pPrimarySurface) pPrimarySurface = ExAllocatePoolWithTag(PagedPool, sizeof(PDEVOBJ), TAG_GDIPDEV);

        PrimarySurface.VideoFileObject = DRIVER_FindMPDriver(DisplayNumber);

        /* Open the miniport driver  */
        if (PrimarySurface.VideoFileObject == NULL)
        {
            DPRINT1("FindMPDriver failed\n");
            goto cleanup;
        }

        /* Retrieve DDI driver names from registry */
        RtlInitUnicodeString(&DriverFileNames, NULL);
        if (!FindDriverFileNames(&DriverFileNames, DisplayNumber))
        {
            DPRINT1("FindDriverFileNames failed\n");
            continue;
        }

        /*
         * DriverFileNames may be a list of drivers in REG_SZ_MULTI format,
         * scan all of them until a good one found.
         */
        CurrentName = DriverFileNames.Buffer;
        GotDriver = FALSE;
        while (!GotDriver &&
                CurrentName < DriverFileNames.Buffer + (DriverFileNames.Length / sizeof (WCHAR)))
        {
            /* Get the DDI driver's entry point */
            GDEnableDriver = DRIVER_FindDDIDriver(CurrentName);
            if (NULL == GDEnableDriver)
            {
                DPRINT("FindDDIDriver failed for %S\n", CurrentName);
            }
            else
            {
                /*  Call DDI driver's EnableDriver function  */
                RtlZeroMemory(&DED, sizeof(DED));

                if (! GDEnableDriver(DDI_DRIVER_VERSION_NT5_01, sizeof(DED), &DED))
                {
                    DPRINT("DrvEnableDriver failed for %S\n", CurrentName);
                }
                else
                {
                    GotDriver = TRUE;
                }
            }

            if (! GotDriver)
            {
                /* Skip to the next name but never get past the Unicode string */
                while (L'\0' != *CurrentName &&
                        CurrentName < DriverFileNames.Buffer + (DriverFileNames.Length / sizeof (WCHAR)))
                {
                    CurrentName++;
                }
                if (CurrentName < DriverFileNames.Buffer + (DriverFileNames.Length / sizeof (WCHAR)))
                {
                    CurrentName++;
                }
            }
        }

        if (!GotDriver)
        {
            ObDereferenceObject(PrimarySurface.VideoFileObject);
            ExFreePoolWithTag(DriverFileNames.Buffer, TAG_RTLREGISTRY);
            DPRINT1("No suitable DDI driver found\n");
            continue;
        }

        DPRINT("Display driver %S loaded\n", CurrentName);

        ExFreePoolWithTag(DriverFileNames.Buffer, TAG_RTLREGISTRY);

        DPRINT("Building DDI Functions\n");

        /* Construct DDI driver function dispatch table */
        if (!DRIVER_BuildDDIFunctions(&DED, &PrimarySurface.DriverFunctions))
        {
            ObDereferenceObject(PrimarySurface.VideoFileObject);
            DPRINT1("BuildDDIFunctions failed\n");
            goto cleanup;
        }

        /* Allocate a phyical device handle from the driver */
        // Support DMW.dmSize + DMW.dmDriverExtra & Alloc DMW then set prt pdmwDev.
        PrimarySurface.DMW.dmSize = sizeof (PrimarySurface.DMW);
        if (SetupDevMode(&PrimarySurface.DMW, DisplayNumber))
        {
            PrimarySurface.hPDev = PrimarySurface.DriverFunctions.EnablePDEV(
                                       &PrimarySurface.DMW,
                                       L"",
                                       HS_DDI_MAX,
                                       PrimarySurface.FillPatterns,
                                       sizeof(PrimarySurface.GDIInfo),
                                       &PrimarySurface.GDIInfo,
                                       sizeof(PrimarySurface.DevInfo),
                                       &PrimarySurface.DevInfo,
                                       NULL,
                                       L"",
                                       (HANDLE) (PrimarySurface.VideoFileObject->DeviceObject));
            DoDefault = (NULL == PrimarySurface.hPDev);
            if (DoDefault)
            {
                DPRINT1("DrvEnablePDev with registry parameters failed\n");
            }
        }
        else
        {
            DoDefault = TRUE;
        }

        if (DoDefault)
        {
            RtlZeroMemory(&(PrimarySurface.DMW), sizeof(DEVMODEW));
            PrimarySurface.DMW.dmSize = sizeof (PrimarySurface.DMW);
            PrimarySurface.hPDev = PrimarySurface.DriverFunctions.EnablePDEV(
                                       &PrimarySurface.DMW,
                                       L"",
                                       HS_DDI_MAX,
                                       PrimarySurface.FillPatterns,
                                       sizeof(PrimarySurface.GDIInfo),
                                       &PrimarySurface.GDIInfo,
                                       sizeof(PrimarySurface.DevInfo),
                                       &PrimarySurface.DevInfo,
                                       NULL,
                                       L"",
                                       (HANDLE) (PrimarySurface.VideoFileObject->DeviceObject));

            if (NULL == PrimarySurface.hPDev)
            {
                ObDereferenceObject(PrimarySurface.VideoFileObject);
                DPRINT1("DrvEnablePDEV with default parameters failed\n");
                DPRINT1("Perhaps DDI driver doesn't match miniport driver?\n");
                continue;
            }

            // Update the primary surface with what we really got
            PrimarySurface.DMW.dmPelsWidth = PrimarySurface.GDIInfo.ulHorzRes;
            PrimarySurface.DMW.dmPelsHeight = PrimarySurface.GDIInfo.ulVertRes;
            PrimarySurface.DMW.dmBitsPerPel = PrimarySurface.GDIInfo.cBitsPixel;
            PrimarySurface.DMW.dmDisplayFrequency = PrimarySurface.GDIInfo.ulVRefresh;
        }

        if (!PrimarySurface.DMW.dmDriverExtra)
        {
            PrimarySurface.pdmwDev = &PrimarySurface.DMW; // HAX!
        }
        else
        {
            DPRINT1("WARNING!!! Need to Alloc DMW !!!!!!\n");
        }
        // Dont remove until we finish testing other drivers.
        if (PrimarySurface.DMW.dmDriverExtra != 0)
        {
            DPRINT1("**** DMW extra = %u bytes. Please report to ros-dev@reactos.org ****\n", PrimarySurface.DMW.dmDriverExtra);
        }

        if (0 == PrimarySurface.GDIInfo.ulLogPixelsX)
        {
            DPRINT("Adjusting GDIInfo.ulLogPixelsX\n");
            PrimarySurface.GDIInfo.ulLogPixelsX = 96;
        }
        if (0 == PrimarySurface.GDIInfo.ulLogPixelsY)
        {
            DPRINT("Adjusting GDIInfo.ulLogPixelsY\n");
            PrimarySurface.GDIInfo.ulLogPixelsY = 96;
        }

        PrimarySurface.Pointer.Exclude.right = -1;

        DPRINT("calling completePDev\n");

        /* Complete initialization of the physical device */
        PrimarySurface.DriverFunctions.CompletePDEV(
            PrimarySurface.hPDev,
            (HDEV)&PrimarySurface);

        DPRINT("calling DRIVER_ReferenceDriver\n");

        DRIVER_ReferenceDriver(L"DISPLAY");

        PrimarySurface.PreparedDriver = TRUE;
        PrimarySurface.DisplayNumber = DisplayNumber;
        PrimarySurface.flFlags = PDEV_DISPLAY; // Hard set,, add more flags.
        PrimarySurface.hsemDevLock = (PERESOURCE)EngCreateSemaphore();
        // Should be null,, but make sure for now.
        PrimarySurface.pvGammaRamp = NULL;
        PrimarySurface.ppdevNext = NULL;    // Fixme! We need to support more than display drvs.
        PrimarySurface.ppdevParent = NULL;  // Always NULL if primary.
        PrimarySurface.pGraphicsDev = NULL; // Fixme!
        PrimarySurface.pEDDgpl = ExAllocatePoolWithTag(PagedPool, sizeof(EDD_DIRECTDRAW_GLOBAL), TAG_EDDGBL);
        if (PrimarySurface.pEDDgpl)
        {
            RtlZeroMemory( PrimarySurface.pEDDgpl ,sizeof(EDD_DIRECTDRAW_GLOBAL));
            ret = TRUE;
        }
        goto cleanup;
    }

cleanup:
    KeSetEvent(&VideoDriverPrepared, 1, FALSE);
    return ret;
}

BOOL FASTCALL
IntPrepareDriverIfNeeded()
{
    return (PrimarySurface.PreparedDriver ? TRUE : IntPrepareDriver());
}

static BOOL FASTCALL
PrepareVideoPrt()
{
    PIRP Irp;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    BOOL Prepare = TRUE;
    ULONG Length = sizeof(BOOL);
    PIO_STACK_LOCATION StackPtr;
    LARGE_INTEGER StartOffset;
    PFILE_OBJECT FileObject = PrimarySurface.VideoFileObject;
    PDEVICE_OBJECT DeviceObject = FileObject->DeviceObject;

    DPRINT("PrepareVideoPrt() called\n");

    KeClearEvent(&PrimarySurface.VideoFileObject->Event);

    ObReferenceObjectByPointer(FileObject, 0, IoFileObjectType, KernelMode);

    StartOffset.QuadPart = 0;
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
                                       DeviceObject,
                                       (PVOID) &Prepare,
                                       Length,
                                       &StartOffset,
                                       NULL,
                                       &Iosb);
    if (NULL == Irp)
    {
        return FALSE;
    }

    /* Set up IRP Data */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = KernelMode;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    Irp->Overlay.AsynchronousParameters.UserApcContext = NULL;
    Irp->Flags |= IRP_WRITE_OPERATION;

    /* Setup Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->FileObject = PrimarySurface.VideoFileObject;
    StackPtr->Parameters.Write.Key = 0;

    Status = IoCallDriver(DeviceObject, Irp);

    if (STATUS_PENDING == Status)
    {
        KeWaitForSingleObject(&FileObject->Event, Executive, KernelMode, TRUE, 0);
        Status = Iosb.Status;
    }

    return NT_SUCCESS(Status);
}


BOOL FASTCALL
IntCreatePrimarySurface()
{
    SIZEL SurfSize;
    RECTL SurfaceRect;
    SURFOBJ *SurfObj;
    BOOL calledFromUser;

    if (! IntPrepareDriverIfNeeded())
    {
        return FALSE;
    }

    if (! PrepareVideoPrt())
    {
        return FALSE;
    }

    DPRINT("calling EnableSurface\n");
    /* Enable the drawing surface */
    PrimarySurface.pSurface =
        PrimarySurface.DriverFunctions.EnableSurface(PrimarySurface.hPDev);
    if (NULL == PrimarySurface.pSurface)
    {
        /*      PrimarySurface.DriverFunctions.AssertMode(PrimarySurface.hPDev, FALSE);*/
        PrimarySurface.DriverFunctions.DisablePDEV(PrimarySurface.hPDev);
        ObDereferenceObject(PrimarySurface.VideoFileObject);
        DPRINT1("DrvEnableSurface failed\n");
        return FALSE;
    }

    PrimarySurface.DriverFunctions.AssertMode(PrimarySurface.hPDev, TRUE);

    calledFromUser = UserIsEntered(); //fixme: possibly upgrade a shared lock
    if (!calledFromUser)
    {
        UserEnterExclusive();
    }

    /* attach monitor */
    IntAttachMonitor(&PrimarySurface, PrimarySurface.DisplayNumber);

    SurfObj = EngLockSurface(PrimarySurface.pSurface);
    SurfObj->dhpdev = PrimarySurface.hPDev;
    SurfSize = SurfObj->sizlBitmap;
    SurfaceRect.left = SurfaceRect.top = 0;
    SurfaceRect.right = SurfObj->sizlBitmap.cx;
    SurfaceRect.bottom = SurfObj->sizlBitmap.cy;
    /* FIXME - why does EngEraseSurface() sometimes crash?
       EngEraseSurface(SurfObj, &SurfaceRect, 0); */

    /* Put the pointer in the center of the screen */
    gpsi->ptCursor.x = (SurfaceRect.right - SurfaceRect.left) / 2;
    gpsi->ptCursor.y = (SurfaceRect.bottom - SurfaceRect.top) / 2;

    /* Give the PDEV a MovePointer function */
    PrimarySurface.pfnMovePointer = PrimarySurface.DriverFunctions.MovePointer;
    if (!PrimarySurface.pfnMovePointer)
        PrimarySurface.pfnMovePointer = EngMovePointer;

    EngUnlockSurface(SurfObj);
    co_IntShowDesktop(IntGetActiveDesktop(), SurfSize.cx, SurfSize.cy);

    // Init Primary Displays Device Capabilities.
    IntvGetDeviceCaps(&PrimarySurface, &GdiHandleTable->DevCaps);

    if (!calledFromUser)
    {
        UserLeave();
    }

    return TRUE;
}

VOID FASTCALL
IntDestroyPrimarySurface()
{
    BOOL calledFromUser;

    DRIVER_UnreferenceDriver(L"DISPLAY");

    calledFromUser = UserIsEntered();
    if (!calledFromUser)
    {
        UserEnterExclusive();
    }

    /* detach monitor */
    IntDetachMonitor(&PrimarySurface);

    if (!calledFromUser)
    {
        UserLeave();
    }

    /*
     * FIXME: Hide a mouse pointer there. Also because we have to prevent
     * memory leaks with the Eng* mouse routines.
     */

    DPRINT("Reseting display\n" );
    PrimarySurface.DriverFunctions.AssertMode(PrimarySurface.hPDev, FALSE);
    PrimarySurface.DriverFunctions.DisableSurface(PrimarySurface.hPDev);
    PrimarySurface.DriverFunctions.DisablePDEV(PrimarySurface.hPDev);
    PrimarySurface.PreparedDriver = FALSE;
    KeSetEvent(&VideoDriverNeedsPreparation, 1, FALSE);
    KeResetEvent(&VideoDriverPrepared);

    DceEmptyCache();

    ObDereferenceObject(PrimarySurface.VideoFileObject);
}

INT
FASTCALL
IntcFonts(PPDEVOBJ pDevObj)
{
    ULONG_PTR Junk;
// Msdn DrvQueryFont:
// If the number of fonts in DEVINFO is -1 and iFace is zero, the driver
// should return the number of fonts it supports.
    if ( pDevObj->DevInfo.cFonts == -1)
    {
        if (pDevObj->DriverFunctions.QueryFont)
            pDevObj->DevInfo.cFonts =
                (ULONG)pDevObj->DriverFunctions.QueryFont(pDevObj->hPDev, 0, 0, &Junk);
        else
            pDevObj->DevInfo.cFonts = 0;
    }
    return pDevObj->DevInfo.cFonts;
}

//
// Support multi display/device locks.
//
VOID
FASTCALL
DC_LockDisplay(HDC hDC)
{
    PERESOURCE Resource;
    PDC dc = DC_LockDc(hDC);
    if (!dc) return;
    Resource = dc->ppdev->hsemDevLock;
    DC_UnlockDc(dc);
    if (!Resource) return;
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite( Resource , TRUE);
}

VOID
FASTCALL
DC_UnlockDisplay(HDC hDC)
{
    PERESOURCE Resource;
    PDC dc = DC_LockDc(hDC);
    if (!dc) return;
    Resource = dc->ppdev->hsemDevLock;
    DC_UnlockDc(dc);
    if (!Resource) return;
    ExReleaseResourceLite( Resource );
    KeLeaveCriticalRegion();
}

//
// Enumerate HDev
//
PPDEVOBJ FASTCALL
IntEnumHDev(VOID)
{
// I guess we will soon have more than one primary surface.
// This will do for now.
    return &PrimarySurface;
}


VOID FASTCALL
IntGdiReferencePdev(PPDEVOBJ ppdev)
{
    if (!hsemDriverMgmt) hsemDriverMgmt = EngCreateSemaphore(); // Hax, should be in dllmain.c
    IntGdiAcquireSemaphore(hsemDriverMgmt);
    ppdev->cPdevRefs++;
    IntGdiReleaseSemaphore(hsemDriverMgmt);
}

VOID FASTCALL
IntGdiUnreferencePdev(PPDEVOBJ ppdev, DWORD CleanUpType)
{
    IntGdiAcquireSemaphore(hsemDriverMgmt);
    ppdev->cPdevRefs--;
    if (!ppdev->cPdevRefs)
    {
        // Handle the destruction of ppdev or PDEVOBJ or PDEVOBJ or PDEV etc.
    }
    IntGdiReleaseSemaphore(hsemDriverMgmt);
}



INT
FASTCALL
IntGetColorManagementCaps(PPDEVOBJ pDevObj)
{
    INT ret = CM_NONE;

    if ( pDevObj->flFlags & PDEV_DISPLAY)
    {
        if (pDevObj->DevInfo.iDitherFormat == BMF_8BPP ||
            pDevObj->DevInfo.flGraphicsCaps2 & GCAPS2_CHANGEGAMMARAMP)
            ret = CM_GAMMA_RAMP;
    }
    if (pDevObj->DevInfo.flGraphicsCaps & GCAPS_CMYKCOLOR)
        ret |= CM_CMYK_COLOR;
    if (pDevObj->DevInfo.flGraphicsCaps & GCAPS_ICM)
        ret |= CM_DEVICE_ICM;
    return ret;
}

INT FASTCALL
IntGdiGetDeviceCaps(PDC dc, INT Index)
{
    INT ret = 0;
    PPDEVOBJ ppdev = dc->ppdev;
    /* Retrieve capability */
    switch (Index)
    {
        case DRIVERVERSION:
            ret = ppdev->GDIInfo.ulVersion;
            break;

        case TECHNOLOGY:
            ret = ppdev->GDIInfo.ulTechnology;
            break;

        case HORZSIZE:
            ret = ppdev->GDIInfo.ulHorzSize;
            break;

        case VERTSIZE:
            ret = ppdev->GDIInfo.ulVertSize;
            break;

        case HORZRES:
            ret = ppdev->GDIInfo.ulHorzRes;
            break;

        case VERTRES:
            ret = ppdev->GDIInfo.ulVertRes;
            break;

        case LOGPIXELSX:
            ret = ppdev->GDIInfo.ulLogPixelsX;
            break;

        case LOGPIXELSY:
            ret = ppdev->GDIInfo.ulLogPixelsY;
            break;

        case CAPS1:
            if ( ppdev->pGraphicsDev &&
                    (((PGRAPHICS_DEVICE)ppdev->pGraphicsDev)->StateFlags &
                     DISPLAY_DEVICE_MIRRORING_DRIVER))
                ret = C1_MIRRORING;
            break;

        case BITSPIXEL:
            ret = ppdev->GDIInfo.cBitsPixel;
            break;

        case PLANES:
            ret = ppdev->GDIInfo.cPlanes;
            break;

        case NUMBRUSHES:
            ret = -1;
            break;

        case NUMPENS:
            ret = ppdev->GDIInfo.ulNumColors;
            if ( ret != -1 ) ret *= 5;
            break;

        case NUMFONTS:
            ret = IntcFonts(ppdev);
            break;

        case NUMCOLORS:
            ret = ppdev->GDIInfo.ulNumColors;
            break;

        case ASPECTX:
            ret = ppdev->GDIInfo.ulAspectX;
            break;

        case ASPECTY:
            ret = ppdev->GDIInfo.ulAspectY;
            break;

        case ASPECTXY:
            ret = ppdev->GDIInfo.ulAspectXY;
            break;

        case CLIPCAPS:
            ret = CP_RECTANGLE;
            break;

        case SIZEPALETTE:
            ret = ppdev->GDIInfo.ulNumPalReg;
            break;

        case NUMRESERVED:
            ret = 20;
            break;

        case COLORRES:
            ret = ppdev->GDIInfo.ulDACRed +
                  ppdev->GDIInfo.ulDACGreen +
                  ppdev->GDIInfo.ulDACBlue;
            break;

        case DESKTOPVERTRES:
            ret = ppdev->GDIInfo.ulVertRes;
            break;

        case DESKTOPHORZRES:
            ret = ppdev->GDIInfo.ulHorzRes;
            break;

        case BLTALIGNMENT:
            ret = ppdev->GDIInfo.ulBltAlignment;
            break;

        case SHADEBLENDCAPS:
            ret = ppdev->GDIInfo.flShadeBlend;
            break;

        case COLORMGMTCAPS:
            ret = IntGetColorManagementCaps(ppdev);
            break;

        case PHYSICALWIDTH:
            ret = ppdev->GDIInfo.szlPhysSize.cx;
            break;

        case PHYSICALHEIGHT:
            ret = ppdev->GDIInfo.szlPhysSize.cy;
            break;

        case PHYSICALOFFSETX:
            ret = ppdev->GDIInfo.ptlPhysOffset.x;
            break;

        case PHYSICALOFFSETY:
            ret = ppdev->GDIInfo.ptlPhysOffset.y;
            break;

        case VREFRESH:
            ret = ppdev->GDIInfo.ulVRefresh;
            break;

        case RASTERCAPS:
            ret = ppdev->GDIInfo.flRaster;
            break;

        case CURVECAPS:
            ret = (CC_CIRCLES | CC_PIE | CC_CHORD | CC_ELLIPSES | CC_WIDE |
                   CC_STYLED | CC_WIDESTYLED | CC_INTERIORS | CC_ROUNDRECT);
            break;

        case LINECAPS:
            ret = (LC_POLYLINE | LC_MARKER | LC_POLYMARKER | LC_WIDE |
                   LC_STYLED | LC_WIDESTYLED | LC_INTERIORS);
            break;

        case POLYGONALCAPS:
            ret = (PC_POLYGON | PC_RECTANGLE | PC_WINDPOLYGON | PC_SCANLINE |
                   PC_WIDE | PC_STYLED | PC_WIDESTYLED | PC_INTERIORS);
            break;

        case TEXTCAPS:
            ret = ppdev->GDIInfo.flTextCaps;
            if (ppdev->GDIInfo.ulTechnology) ret |= TC_VA_ABLE;
            ret |= (TC_SO_ABLE|TC_UA_ABLE);
            break;

        case PDEVICESIZE:
        case SCALINGFACTORX:
        case SCALINGFACTORY:
        default:
            ret = 0;
            break;
    }

    return ret;
}

INT APIENTRY
NtGdiGetDeviceCaps(HDC  hDC,
                   INT  Index)
{
    PDC  dc;
    INT  ret;

    dc = DC_LockDc(hDC);
    if (dc == NULL)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }

    ret = IntGdiGetDeviceCaps(dc, Index);

    DPRINT("(%04x,%d): returning %d\n", hDC, Index, ret);

    DC_UnlockDc( dc );
    return ret;
}

VOID
FASTCALL
IntvGetDeviceCaps(
    PPDEVOBJ pDevObj,
    PDEVCAPS pDevCaps)
{
    ULONG Tmp = 0;
    PGDIINFO pGdiInfo = &pDevObj->GDIInfo;

    pDevCaps->ulVersion         = pGdiInfo->ulVersion;
    pDevCaps->ulTechnology      = pGdiInfo->ulTechnology;
    pDevCaps->ulHorzSizeM       = (pGdiInfo->ulHorzSize + 500) / 1000;
    pDevCaps->ulVertSizeM       = (pGdiInfo->ulVertSize + 500) / 1000;
    pDevCaps->ulHorzSize        = pGdiInfo->ulHorzSize;
    pDevCaps->ulVertSize        = pGdiInfo->ulVertSize;
    pDevCaps->ulHorzRes         = pGdiInfo->ulHorzRes;
    pDevCaps->ulVertRes         = pGdiInfo->ulVertRes;
    pDevCaps->ulVRefresh        = pGdiInfo->ulVRefresh;
    pDevCaps->ulDesktopHorzRes  = pGdiInfo->ulHorzRes;
    pDevCaps->ulDesktopVertRes  = pGdiInfo->ulVertRes;
    pDevCaps->ulBltAlignment    = pGdiInfo->ulBltAlignment;
    pDevCaps->ulPlanes          = pGdiInfo->cPlanes;

    pDevCaps->ulBitsPixel       = pGdiInfo->cBitsPixel;
    if (pGdiInfo->cBitsPixel == 15) pDevCaps->ulBitsPixel = 16;

    Tmp = pGdiInfo->ulNumColors;
    if ( Tmp != -1 ) Tmp *= 5;
    pDevCaps->ulNumPens = Tmp;
    pDevCaps->ulNumColors       = pGdiInfo->ulNumColors;

    pDevCaps->ulNumFonts        = IntcFonts(pDevObj);

    pDevCaps->ulRasterCaps      = pGdiInfo->flRaster;
    pDevCaps->ulShadeBlend      = pGdiInfo->flShadeBlend;
    pDevCaps->ulAspectX         = pGdiInfo->ulAspectX;
    pDevCaps->ulAspectY         = pGdiInfo->ulAspectY;
    pDevCaps->ulAspectXY        = pGdiInfo->ulAspectXY;
    pDevCaps->ulLogPixelsX      = pGdiInfo->ulLogPixelsX;
    pDevCaps->ulLogPixelsY      = pGdiInfo->ulLogPixelsY;
    pDevCaps->ulSizePalette     = pGdiInfo->ulNumPalReg;
    pDevCaps->ulColorRes        = pGdiInfo->ulDACRed + pGdiInfo->ulDACGreen + pGdiInfo->ulDACBlue;
    pDevCaps->ulPhysicalWidth   = pGdiInfo->szlPhysSize.cx;
    pDevCaps->ulPhysicalHeight  = pGdiInfo->szlPhysSize.cy;
    pDevCaps->ulPhysicalOffsetX = pGdiInfo->ptlPhysOffset.x;
    pDevCaps->ulPhysicalOffsetY = pGdiInfo->ptlPhysOffset.y;
    pDevCaps->ulPanningHorzRes  = pGdiInfo->ulPanningHorzRes;
    pDevCaps->ulPanningVertRes  = pGdiInfo->ulPanningVertRes;
    pDevCaps->xPanningAlignment = pGdiInfo->xPanningAlignment;
    pDevCaps->yPanningAlignment = pGdiInfo->yPanningAlignment;

    Tmp = 0;
    Tmp = pGdiInfo->flTextCaps | (TC_SO_ABLE|TC_UA_ABLE|TC_CP_STROKE|TC_OP_STROKE|TC_OP_CHARACTER);

    pDevCaps->ulTextCaps = pGdiInfo->flTextCaps | (TC_SO_ABLE|TC_UA_ABLE|TC_CP_STROKE|TC_OP_STROKE|TC_OP_CHARACTER);

    if (pGdiInfo->ulTechnology)
        pDevCaps->ulTextCaps = Tmp | TC_VA_ABLE;

    pDevCaps->ulColorMgmtCaps = IntGetColorManagementCaps(pDevObj);

    return;
}

/*
* @implemented
*/
BOOL
APIENTRY
NtGdiGetDeviceCapsAll (
    IN HDC hDC,
    OUT PDEVCAPS pDevCaps)
{
    PDC  dc;
    PDEVCAPS pSafeDevCaps;
    NTSTATUS Status = STATUS_SUCCESS;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pSafeDevCaps = ExAllocatePoolWithTag(PagedPool, sizeof(DEVCAPS), TAG_TEMP);

    if (!pSafeDevCaps)
    {
        DC_UnlockDc(dc);
        return FALSE;
    }

    IntvGetDeviceCaps(dc->ppdev, pSafeDevCaps);

    _SEH2_TRY
    {
        ProbeForWrite(pDevCaps,
        sizeof(DEVCAPS),
        1);
        RtlCopyMemory(pDevCaps, pSafeDevCaps, sizeof(DEVCAPS));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ExFreePoolWithTag(pSafeDevCaps, TAG_TEMP);
    DC_UnlockDc(dc);

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return FALSE;
    }
    return TRUE;
}


/*
 * @implemented
 */
DHPDEV
APIENTRY
NtGdiGetDhpdev(
    IN HDEV hdev)
{
    PPDEVOBJ ppdev, pGdiDevice = (PPDEVOBJ) hdev;
    if (!pGdiDevice) return NULL;
    if ( pGdiDevice < (PPDEVOBJ)MmSystemRangeStart) return NULL;
    ppdev = pPrimarySurface;
    IntGdiAcquireSemaphore(hsemDriverMgmt);
    do
    {
        if (pGdiDevice == ppdev) break;
        else
            ppdev = ppdev->ppdevNext;
    }
    while (ppdev != NULL);
    IntGdiReleaseSemaphore(hsemDriverMgmt);
    if (!ppdev) return NULL;
    return pGdiDevice->hPDev;
}

static NTSTATUS FASTCALL
GetVideoRegistryKey(
    OUT PUNICODE_STRING RegistryPath,
    IN PCUNICODE_STRING DeviceName) /* ex: "\Device\Video0" */
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    NTSTATUS Status;

    RtlInitUnicodeString(RegistryPath, NULL);
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].Name = DeviceName->Buffer;
    QueryTable[0].EntryContext = RegistryPath;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_DEVICEMAP,
                                    L"VIDEO",
                                    QueryTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("No %wZ value in DEVICEMAP\\VIDEO found (Status 0x%08lx)\n", DeviceName, Status);
        return STATUS_NO_SUCH_DEVICE;
    }

    DPRINT("RegistryPath %wZ\n", RegistryPath);
    return STATUS_SUCCESS;
}


static NTSTATUS FASTCALL
GetVideoDeviceName(
    OUT PUNICODE_STRING VideoDeviceName,
    IN PCUNICODE_STRING DisplayDevice) /* ex: "\.\DISPLAY1" or "\??\DISPLAY1" */
{
    UNICODE_STRING Prefix = RTL_CONSTANT_STRING(L"\\??\\");
    UNICODE_STRING ObjectName;
    UNICODE_STRING KernelModeName = { 0, };
    OBJECT_ATTRIBUTES ObjectAttributes;
    USHORT LastSlash;
    ULONG Length;
    HANDLE LinkHandle = NULL;
    NTSTATUS Status;

    RtlInitUnicodeString(VideoDeviceName, NULL);

    /* Get device name (DisplayDevice is "\.\xxx") */
    for (LastSlash = DisplayDevice->Length / sizeof(WCHAR); LastSlash > 0; LastSlash--)
    {
        if (DisplayDevice->Buffer[LastSlash - 1] == L'\\')
            break;
    }

    if (LastSlash == 0)
    {
        DPRINT1("Invalid device name '%wZ'\n", DisplayDevice);
        Status = STATUS_OBJECT_NAME_INVALID;
        goto cleanup;
    }
    ObjectName = *DisplayDevice;
    ObjectName.Length -= LastSlash * sizeof(WCHAR);
    ObjectName.MaximumLength -= LastSlash * sizeof(WCHAR);
    ObjectName.Buffer += LastSlash;

    /* Create "\??\xxx" (ex: "\??\DISPLAY1") */
    KernelModeName.MaximumLength = Prefix.Length + ObjectName.Length + sizeof(UNICODE_NULL);
    KernelModeName.Buffer = ExAllocatePoolWithTag(PagedPool,
                            KernelModeName.MaximumLength,
                            TAG_DC);
    if (!KernelModeName.Buffer)
    {
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }
    RtlCopyUnicodeString(&KernelModeName, &Prefix);
    Status = RtlAppendUnicodeStringToString(&KernelModeName, &ObjectName);
    if (!NT_SUCCESS(Status))
        goto cleanup;

    /* Open \??\xxx (ex: "\??\DISPLAY1") */
    InitializeObjectAttributes(&ObjectAttributes,
                               &KernelModeName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenSymbolicLinkObject(&LinkHandle,
                                      GENERIC_READ,
                                      &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to open symbolic link %wZ (Status 0x%08lx)\n", &KernelModeName, Status);
        Status = STATUS_NO_SUCH_DEVICE;
        goto cleanup;
    }

    Status = ZwQuerySymbolicLinkObject(LinkHandle, VideoDeviceName, &Length);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL)
    {
        DPRINT1("Unable to query symbolic link %wZ (Status 0x%08lx)\n", &KernelModeName, Status);
        Status = STATUS_NO_SUCH_DEVICE;
        goto cleanup;
    }
    VideoDeviceName->MaximumLength = Length;
    VideoDeviceName->Buffer = ExAllocatePoolWithTag(PagedPool,
                              VideoDeviceName->MaximumLength + sizeof(UNICODE_NULL),
                              TAG_DC);
    if (!VideoDeviceName->Buffer)
    {
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }
    Status = ZwQuerySymbolicLinkObject(LinkHandle, VideoDeviceName, NULL);
    VideoDeviceName->Buffer[VideoDeviceName->MaximumLength / sizeof(WCHAR) - 1] = UNICODE_NULL;
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to query symbolic link %wZ (Status 0x%08lx)\n", &KernelModeName, Status);
        Status = STATUS_NO_SUCH_DEVICE;
        goto cleanup;
    }
    Status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(Status) && VideoDeviceName->Buffer)
        ExFreePoolWithTag(VideoDeviceName->Buffer, TAG_DC);
    if (KernelModeName.Buffer)
        ExFreePoolWithTag(KernelModeName.Buffer, TAG_DC);
    if (LinkHandle)
        ZwClose(LinkHandle);
    return Status;
}

LONG
FASTCALL
IntChangeDisplaySettings(
    IN PUNICODE_STRING pDeviceName  OPTIONAL,
    IN LPDEVMODEW DevMode,
    IN DWORD dwflags,
    IN PVOID lParam  OPTIONAL)
{
    BOOLEAN Global = FALSE;
    BOOLEAN NoReset = FALSE;
    BOOLEAN Reset = FALSE;
    BOOLEAN SetPrimary = FALSE;
    LONG Ret=0;
    NTSTATUS Status ;

    DPRINT1("display flags : %x\n",dwflags);

    if ((dwflags & CDS_UPDATEREGISTRY) == CDS_UPDATEREGISTRY)
    {
        /* Check global, reset and noreset flags */
        if ((dwflags & CDS_GLOBAL) == CDS_GLOBAL)
            Global = TRUE;
        if ((dwflags & CDS_NORESET) == CDS_NORESET)
            NoReset = TRUE;
        dwflags &= ~(CDS_GLOBAL | CDS_NORESET);
    }
    if ((dwflags & CDS_RESET) == CDS_RESET)
        Reset = TRUE;
    if ((dwflags & CDS_SET_PRIMARY) == CDS_SET_PRIMARY)
        SetPrimary = TRUE;
    dwflags &= ~(CDS_RESET | CDS_SET_PRIMARY);

    if (Reset && NoReset)
        return DISP_CHANGE_BADFLAGS;

    if (dwflags == 0)
    {
        /* Dynamically change graphics mode */
        DPRINT1("flag 0 UNIMPLEMENTED\n");
        return DISP_CHANGE_FAILED;
    }

    if ((dwflags & CDS_TEST) == CDS_TEST)
    {
        /* Test reslution */
        dwflags &= ~CDS_TEST;
        DPRINT1("flag CDS_TEST UNIMPLEMENTED\n");
        Ret = DISP_CHANGE_FAILED;
    }

    if ((dwflags & CDS_FULLSCREEN) == CDS_FULLSCREEN)
    {
        DEVMODEW lpDevMode;
        /* Full Screen */
        dwflags &= ~CDS_FULLSCREEN;
        DPRINT1("flag CDS_FULLSCREEN partially implemented\n");
        Ret = DISP_CHANGE_FAILED;

        RtlZeroMemory(&lpDevMode, sizeof(DEVMODEW));
        lpDevMode.dmSize = sizeof(DEVMODEW);

        Status = IntEnumDisplaySettings(pDeviceName, ENUM_CURRENT_SETTINGS, &lpDevMode, 0);
        if (!NT_SUCCESS(Status))
            return DISP_CHANGE_FAILED;

        DPRINT1("Req Mode     : %d x %d x %d\n", DevMode->dmPelsWidth,DevMode->dmPelsHeight,DevMode->dmBitsPerPel);
        DPRINT1("Current Mode : %d x %d x %d\n", lpDevMode.dmPelsWidth,lpDevMode.dmPelsHeight, lpDevMode.dmBitsPerPel);


        if ((lpDevMode.dmBitsPerPel == DevMode->dmBitsPerPel) &&
                (lpDevMode.dmPelsWidth  == DevMode->dmPelsWidth) &&
                (lpDevMode.dmPelsHeight == DevMode->dmPelsHeight))
            Ret = DISP_CHANGE_SUCCESSFUL;
    }

    if ((dwflags & CDS_VIDEOPARAMETERS) == CDS_VIDEOPARAMETERS)
    {
        dwflags &= ~CDS_VIDEOPARAMETERS;
        if (lParam == NULL)
            Ret=DISP_CHANGE_BADPARAM;
        else
        {
            DPRINT1("flag CDS_VIDEOPARAMETERS UNIMPLEMENTED\n");
            Ret = DISP_CHANGE_FAILED;
        }

    }

    if ((dwflags & CDS_UPDATEREGISTRY) == CDS_UPDATEREGISTRY)
    {

        UNICODE_STRING DeviceName;
        UNICODE_STRING RegistryKey;
        UNICODE_STRING InDeviceName;
        OBJECT_ATTRIBUTES ObjectAttributes;
        HANDLE DevInstRegKey;
        ULONG NewValue;

        DPRINT1("set CDS_UPDATEREGISTRY\n");

        dwflags &= ~CDS_UPDATEREGISTRY;

        /* Check if pDeviceName is NULL, we need to retrieve it */
        if (pDeviceName == NULL)
        {
            WCHAR szBuffer[MAX_DRIVER_NAME];
            PDC DC;
            PWINDOW_OBJECT Wnd=NULL;
            HWND hWnd;
            HDC hDC;

            hWnd = IntGetDesktopWindow();
            if (!(Wnd = UserGetWindowObject(hWnd)))
            {
                return FALSE;
            }

            hDC = UserGetWindowDC(Wnd);

            DC = DC_LockDc(hDC);
            if (NULL == DC)
            {
                return FALSE;
            }
            swprintf (szBuffer, L"\\\\.\\DISPLAY%lu", DC->ppdev->DisplayNumber);
            DC_UnlockDc(DC);

            RtlInitUnicodeString(&InDeviceName, szBuffer);
            pDeviceName = &InDeviceName;
        }

        Status = GetVideoDeviceName(&DeviceName, pDeviceName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Unable to get destination of '%wZ' (Status 0x%08lx)\n", pDeviceName, Status);
            return DISP_CHANGE_FAILED;
        }
        Status = GetVideoRegistryKey(&RegistryKey, &DeviceName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Unable to get registry key for '%wZ' (Status 0x%08lx)\n", &DeviceName, Status);
            ExFreePoolWithTag(DeviceName.Buffer, TAG_DC);
            return DISP_CHANGE_FAILED;
        }
        ExFreePoolWithTag(DeviceName.Buffer, TAG_DC);

        InitializeObjectAttributes(&ObjectAttributes, &RegistryKey,
                                   OBJ_CASE_INSENSITIVE, NULL, NULL);
        Status = ZwOpenKey(&DevInstRegKey, KEY_SET_VALUE, &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Unable to open registry key %wZ (Status 0x%08lx)\n", &RegistryKey, Status);
            ExFreePoolWithTag(RegistryKey.Buffer, TAG_RTLREGISTRY);
            return DISP_CHANGE_FAILED;
        }
        ExFreePoolWithTag(RegistryKey.Buffer, TAG_RTLREGISTRY);

        /* Update needed fields */
        if (NT_SUCCESS(Status) && DevMode->dmFields & DM_BITSPERPEL)
        {
            RtlInitUnicodeString(&RegistryKey, L"DefaultSettings.BitsPerPel");
            NewValue = DevMode->dmBitsPerPel;
            Status = ZwSetValueKey(DevInstRegKey, &RegistryKey, 0, REG_DWORD, &NewValue, sizeof(NewValue));
        }

        if (NT_SUCCESS(Status) && DevMode->dmFields & DM_PELSWIDTH)
        {
            RtlInitUnicodeString(&RegistryKey, L"DefaultSettings.XResolution");
            NewValue = DevMode->dmPelsWidth;
            Status = ZwSetValueKey(DevInstRegKey, &RegistryKey, 0, REG_DWORD, &NewValue, sizeof(NewValue));
        }

        if (NT_SUCCESS(Status) && DevMode->dmFields & DM_PELSHEIGHT)
        {
            RtlInitUnicodeString(&RegistryKey, L"DefaultSettings.YResolution");
            NewValue = DevMode->dmPelsHeight;
            Status = ZwSetValueKey(DevInstRegKey, &RegistryKey, 0, REG_DWORD, &NewValue, sizeof(NewValue));
        }

        if (NT_SUCCESS(Status) && DevMode->dmFields & DM_DISPLAYFREQUENCY)
        {
            RtlInitUnicodeString(&RegistryKey, L"DefaultSettings.VRefresh");
            NewValue = DevMode->dmDisplayFrequency;
            Status = ZwSetValueKey(DevInstRegKey, &RegistryKey, 0, REG_DWORD, &NewValue, sizeof(NewValue));
        }

        ZwClose(DevInstRegKey);
        if (NT_SUCCESS(Status))
            Ret = DISP_CHANGE_RESTART;
        else
            /* return DISP_CHANGE_NOTUPDATED when we can save to reg only valid for NT */
            Ret = DISP_CHANGE_NOTUPDATED;
    }

    if (dwflags != 0)
        Ret = DISP_CHANGE_BADFLAGS;

    return Ret;
}



#define SIZEOF_DEVMODEW_300 188
#define SIZEOF_DEVMODEW_400 212
#define SIZEOF_DEVMODEW_500 220

static NTSTATUS FASTCALL
GetDisplayNumberFromDeviceName(
    IN PUNICODE_STRING pDeviceName  OPTIONAL,
    OUT ULONG *DisplayNumber)
{
    UNICODE_STRING DisplayString = RTL_CONSTANT_STRING(L"\\\\.\\DISPLAY");
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Length;
    ULONG Number;
    ULONG i;

    if (DisplayNumber == NULL)
        return STATUS_INVALID_PARAMETER_2;

    /* Check if DeviceName is valid */
    if (pDeviceName &&
            pDeviceName->Length > 0 && pDeviceName->Length <= DisplayString.Length)
        return STATUS_OBJECT_NAME_INVALID;

    if (pDeviceName == NULL || pDeviceName->Length == 0)
    {
        PWINDOW_OBJECT DesktopObject;
        HDC DesktopHDC;
        PDC pDC;

        DesktopObject = UserGetDesktopWindow();
        DesktopHDC = UserGetWindowDC(DesktopObject);
        pDC = DC_LockDc(DesktopHDC);

        *DisplayNumber = pDC->ppdev->DisplayNumber;

        DC_UnlockDc(pDC);
        UserReleaseDC(DesktopObject, DesktopHDC, FALSE);

        return STATUS_SUCCESS;
    }

    /* Hack to check if the first parts are equal, by faking the device name length */
    Length = pDeviceName->Length;
    pDeviceName->Length = DisplayString.Length;
    if (RtlEqualUnicodeString(&DisplayString, pDeviceName, FALSE) == FALSE)
        Status = STATUS_OBJECT_NAME_INVALID;
    pDeviceName->Length = Length;

    if (NT_SUCCESS(Status))
    {
        /* Convert the last part of pDeviceName to a number */
        Number = 0;
        Length = pDeviceName->Length / sizeof(WCHAR);
        for (i = DisplayString.Length / sizeof(WCHAR); i < Length; i++)
        {
            WCHAR Char = pDeviceName->Buffer[i];
            if (Char >= L'0' && Char <= L'9')
                Number = Number * 10 + Char - L'0';
            else if (Char != L'\0')
                return STATUS_OBJECT_NAME_INVALID;
        }

        *DisplayNumber = Number - 1;
    }

    return Status;
}

/*! \brief Enumerate possible display settings for the given display...
 *
 * \todo Make thread safe!?
 * \todo Don't ignore pDeviceName
 * \todo Implement non-raw mode (only return settings valid for driver and monitor)
 */
NTSTATUS
FASTCALL
IntEnumDisplaySettings(
    IN PUNICODE_STRING pDeviceName  OPTIONAL,
    IN DWORD iModeNum,
    IN OUT LPDEVMODEW pDevMode,
    IN DWORD dwFlags)
{
    static DEVMODEW *CachedDevModes = NULL, *CachedDevModesEnd = NULL;
    static DWORD SizeOfCachedDevModes = 0;
    static UNICODE_STRING CachedDeviceName;
    PDEVMODEW CachedMode = NULL;
    DEVMODEW DevMode;
    ULONG DisplayNumber;
    NTSTATUS Status;

    Status = GetDisplayNumberFromDeviceName(pDeviceName, &DisplayNumber);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    DPRINT("DevMode->dmSize = %d\n", pDevMode->dmSize);
    DPRINT("DevMode->dmExtraSize = %d\n", pDevMode->dmDriverExtra);
    if (pDevMode->dmSize != SIZEOF_DEVMODEW_300 &&
            pDevMode->dmSize != SIZEOF_DEVMODEW_400 &&
            pDevMode->dmSize != SIZEOF_DEVMODEW_500)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (iModeNum == ENUM_CURRENT_SETTINGS)
    {
        CachedMode = &PrimarySurface.DMW;
        ASSERT(CachedMode->dmSize > 0);
    }
    else if (iModeNum == ENUM_REGISTRY_SETTINGS)
    {
        RtlZeroMemory(&DevMode, sizeof (DevMode));
        DevMode.dmSize = sizeof (DevMode);
        DevMode.dmDriverExtra = 0;
        if (SetupDevMode(&DevMode, DisplayNumber))
            CachedMode = &DevMode;
        else
        {
            return STATUS_UNSUCCESSFUL; // FIXME: what status?
        }
        /* FIXME: Maybe look for the matching devmode supplied by the
         *        driver so we can provide driver private/extra data?
         */
    }
    else
    {
        BOOL IsCachedDevice = (CachedDevModes != NULL);

        if (CachedDevModes &&
                ((pDeviceName == NULL && CachedDeviceName.Length > 0) ||
                 (pDeviceName != NULL && pDeviceName->Buffer != NULL && CachedDeviceName.Length == 0) ||
                 (pDeviceName != NULL && pDeviceName->Buffer != NULL && CachedDeviceName.Length > 0 && RtlEqualUnicodeString(pDeviceName, &CachedDeviceName, TRUE) == FALSE)))
        {
            IsCachedDevice = FALSE;
        }

        if (iModeNum == 0 || IsCachedDevice == FALSE) /* query modes from drivers */
        {
            UNICODE_STRING DriverFileNames;
            LPWSTR CurrentName;
            DRVENABLEDATA DrvEnableData;

            /* Free resources from last driver cache */
            if (IsCachedDevice == FALSE && CachedDeviceName.Buffer != NULL)
            {
                RtlFreeUnicodeString(&CachedDeviceName);
            }

            /* Retrieve DDI driver names from registry */
            RtlInitUnicodeString(&DriverFileNames, NULL);
            if (!FindDriverFileNames(&DriverFileNames, DisplayNumber))
            {
                DPRINT1("FindDriverFileNames failed\n");
                return STATUS_UNSUCCESSFUL;
            }

            if (!IntPrepareDriverIfNeeded())
            {
                DPRINT1("IntPrepareDriverIfNeeded failed\n");
                return STATUS_UNSUCCESSFUL;
            }

            /*
             * DriverFileNames may be a list of drivers in REG_SZ_MULTI format,
             * scan all of them until a good one found.
             */
            CurrentName = DriverFileNames.Buffer;
            for (;CurrentName < DriverFileNames.Buffer + (DriverFileNames.Length / sizeof (WCHAR));
                    CurrentName += wcslen(CurrentName) + 1)
            {
                INT i;
                PFN_DrvEnableDriver GDEnableDriver;
                PFN_DrvGetModes GetModes = NULL;
                INT SizeNeeded, SizeUsed;

                /* Get the DDI driver's entry point */
                //GDEnableDriver = DRIVER_FindDDIDriver(CurrentName);
                GDEnableDriver = DRIVER_FindExistingDDIDriver(L"DISPLAY");
                if (NULL == GDEnableDriver)
                {
                    DPRINT("FindDDIDriver failed for %S\n", CurrentName);
                    continue;
                }

                /*  Call DDI driver's EnableDriver function  */
                RtlZeroMemory(&DrvEnableData, sizeof(DrvEnableData));

                if (!GDEnableDriver(DDI_DRIVER_VERSION_NT5_01, sizeof (DrvEnableData), &DrvEnableData))
                {
                    DPRINT("DrvEnableDriver failed for %S\n", CurrentName);
                    continue;
                }

                CachedDevModesEnd = CachedDevModes;

                /* find DrvGetModes function */
                for (i = 0; i < DrvEnableData.c; i++)
                {
                    PDRVFN DrvFn = DrvEnableData.pdrvfn + i;

                    if (DrvFn->iFunc == INDEX_DrvGetModes)
                    {
                        GetModes = (PFN_DrvGetModes)DrvFn->pfn;
                        break;
                    }
                }

                if (GetModes == NULL)
                {
                    DPRINT("DrvGetModes doesn't exist for %S\n", CurrentName);
                    continue;
                }

                /* make sure we have enough memory to hold the modes */
                SizeNeeded = GetModes((HANDLE)(PrimarySurface.VideoFileObject->DeviceObject), 0, NULL);
                if (SizeNeeded <= 0)
                {
                    DPRINT("DrvGetModes failed for %S\n", CurrentName);
                    break;
                }

                SizeUsed = (PCHAR)CachedDevModesEnd - (PCHAR)CachedDevModes;
                if (SizeOfCachedDevModes < SizeUsed + SizeNeeded)
                {
                    PVOID NewBuffer;

                    SizeOfCachedDevModes += SizeNeeded;
                    NewBuffer = ExAllocatePool(PagedPool, SizeOfCachedDevModes);
                    if (NewBuffer == NULL)
                    {
                        /* clean up */
                        ExFreePool(CachedDevModes);
                        CachedDevModes = NULL;
                        CachedDevModesEnd = NULL;
                        SizeOfCachedDevModes = 0;

                        if (CachedDeviceName.Buffer != NULL)
                            RtlFreeUnicodeString(&CachedDeviceName);

                        return STATUS_NO_MEMORY;
                    }
                    if (CachedDevModes != NULL)
                    {
                        RtlCopyMemory(NewBuffer, CachedDevModes, SizeUsed);
                        ExFreePool(CachedDevModes);
                    }
                    CachedDevModes = NewBuffer;
                    CachedDevModesEnd = (DEVMODEW *)((PCHAR)NewBuffer + SizeUsed);
                }

                if (!IsCachedDevice)
                {
                    if (CachedDeviceName.Buffer != NULL)
                        RtlFreeUnicodeString(&CachedDeviceName);

                    if (pDeviceName)
                        IntSafeCopyUnicodeString(&CachedDeviceName, pDeviceName);

                    IsCachedDevice = TRUE;
                }

                /* query modes */
                SizeNeeded = GetModes((HANDLE)(PrimarySurface.VideoFileObject->DeviceObject),
                                      SizeNeeded,
                                      CachedDevModesEnd);
                if (SizeNeeded <= 0)
                {
                    DPRINT("DrvGetModes failed for %S\n", CurrentName);
                }
                else
                {
                    CachedDevModesEnd = (DEVMODEW *)((PCHAR)CachedDevModesEnd + SizeNeeded);
                }
            }

            ExFreePoolWithTag(DriverFileNames.Buffer, TAG_RTLREGISTRY);
        }

        /* return cached info */
        CachedMode = CachedDevModes;
        if (CachedMode >= CachedDevModesEnd)
        {
            return STATUS_NO_MORE_ENTRIES;
        }
        while (iModeNum-- > 0 && CachedMode < CachedDevModesEnd)
        {
            assert(CachedMode->dmSize > 0);
            CachedMode = (DEVMODEW *)((PCHAR)CachedMode + CachedMode->dmSize + CachedMode->dmDriverExtra);
        }
        if (CachedMode >= CachedDevModesEnd)
        {
            return STATUS_NO_MORE_ENTRIES;
        }
    }

    ASSERT(CachedMode != NULL);

    RtlCopyMemory(pDevMode, CachedMode, min(pDevMode->dmSize, CachedMode->dmSize));
    RtlZeroMemory(pDevMode + pDevMode->dmSize, pDevMode->dmDriverExtra);
    RtlCopyMemory(pDevMode + min(pDevMode->dmSize, CachedMode->dmSize), CachedMode + CachedMode->dmSize, min(pDevMode->dmDriverExtra, CachedMode->dmDriverExtra));

    return STATUS_SUCCESS;
}

INT
APIENTRY
NtGdiDrawEscape(
    IN HDC hdc,
    IN INT iEsc,
    IN INT cjIn,
    IN OPTIONAL LPSTR pjIn)
{
    UNIMPLEMENTED;
    return 0;
}

