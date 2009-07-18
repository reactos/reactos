/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Functions for creation and destruction of DCs
 * FILE:              subsystem/win32/win32k/objects/device.c
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@rectos.org)
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/*static*/ PDEVOBJ PrimarySurface;
PPDEVOBJ pPrimarySurface = &PrimarySurface;
static KEVENT VideoDriverNeedsPreparation;
static KEVENT VideoDriverPrepared;


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

        /* Default display frequency to 60 if it's not specified */
        if (DevMode->dmDisplayFrequency == 0)
            DevMode->dmDisplayFrequency = 60;

        if (!DevMode->dmBitsPerPel ||
            !DevMode->dmPelsWidth ||
            !DevMode->dmPelsHeight)
        {
            DPRINT("Not all required devmode members are set\n");
            Valid = FALSE;
        }

        /* Set falgs for those fields we provide */
        PrimarySurface.DMW.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
    }

    ExFreePoolWithTag(RegistryPath.Buffer, TAG_RTLREGISTRY);

    if (!Valid) RtlZeroMemory(DevMode, sizeof(DEVMODEW));

    return Valid;
}

static BOOLEAN FASTCALL
IntPrepareDriver()
{
    PFN_DrvEnableDriver GDEnableDriver;
    DRVENABLEDATA DED;
    UNICODE_STRING DriverFileNames;
    PWSTR CurrentName;
    BOOLEAN GotDriver;
    BOOLEAN DoDefault = FALSE;
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

        /* Zero primary surface's device object */
        RtlZeroMemory(&PrimarySurface, sizeof(PrimarySurface));

        /* Open the miniport driver */
        PrimarySurface.VideoFileObject = DRIVER_FindMPDriver(DisplayNumber);

        if (!PrimarySurface.VideoFileObject)
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
            if (!GDEnableDriver)
            {
                DPRINT("FindDDIDriver failed for %S\n", CurrentName);
            }
            else
            {
                /*  Call DDI driver's EnableDriver function  */
                RtlZeroMemory(&DED, sizeof(DED));

                if (!GDEnableDriver(DDI_DRIVER_VERSION_NT5_01, sizeof(DED), &DED))
                {
                    DPRINT("DrvEnableDriver failed for %S\n", CurrentName);
                }
                else
                {
                    GotDriver = TRUE;
                }
            }

            if (!GotDriver)
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

        /* Allocate a physical device handle from the driver */
        // Support DMW.dmSize + DMW.dmDriverExtra & Alloc DMW then set prt pdmwDev.
        PrimarySurface.DMW.dmSize = sizeof (PrimarySurface.DMW);
        PrimarySurface.DMW.dmSpecVersion = DM_SPECVERSION;
        PrimarySurface.DMW.dmDriverVersion = DM_SPECVERSION;

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

            /* If no handle, then fallback to default */
            if (!PrimarySurface.hPDev)
            {
                DoDefault = TRUE;
                DPRINT1("DrvEnablePDev with registry parameters failed\n");
            }
        }
        else
        {
            DoDefault = TRUE;
        }

        /* Try default if requested so */
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

            if (!PrimarySurface.hPDev)
            {
                ObDereferenceObject(PrimarySurface.VideoFileObject);
                DPRINT1("DrvEnablePDEV with default parameters failed\n");
                DPRINT1("Perhaps DDI driver doesn't match miniport driver?\n");
                continue;
            }

            /* Update the primary surface with what we really got */
            PrimarySurface.DMW.dmPelsWidth = PrimarySurface.GDIInfo.ulHorzRes;
            PrimarySurface.DMW.dmPelsHeight = PrimarySurface.GDIInfo.ulVertRes;
            PrimarySurface.DMW.dmBitsPerPel = PrimarySurface.GDIInfo.cBitsPixel;
            PrimarySurface.DMW.dmDisplayFrequency = PrimarySurface.GDIInfo.ulVRefresh;
        }

        if (!PrimarySurface.DMW.dmDriverExtra)
        {
            /* Point pdwmDev to the in-structure storage since it fits */
            PrimarySurface.pdmwDev = &PrimarySurface.DMW;
        }
        else
        {
            /* DMW doesn't fit, unimplemented path */
            DPRINT1("WARNING!!! Need to Alloc DMW !!!!!!\n");
            DPRINT1("**** DMW extra = %u bytes. Please report to ros-dev@reactos.org ****\n", PrimarySurface.DMW.dmDriverExtra);
        }

        /* Set LogPixels defaults */
        if (!PrimarySurface.GDIInfo.ulLogPixelsX)
        {
            DPRINT("Adjusting GDIInfo.ulLogPixelsX\n");
            PrimarySurface.GDIInfo.ulLogPixelsX = 96;
        }
        if (!PrimarySurface.GDIInfo.ulLogPixelsY)
        {
            DPRINT("Adjusting GDIInfo.ulLogPixelsY\n");
            PrimarySurface.GDIInfo.ulLogPixelsY = 96;
        }

        /* Complete initialization of the physical device */
        PrimarySurface.DriverFunctions.CompletePDEV(
            PrimarySurface.hPDev,
            (HDEV)&PrimarySurface);

        /* Reference the driver object */
        DRIVER_ReferenceDriver(L"DISPLAY");

        /* Indicate success */
        PrimarySurface.PreparedDriver = TRUE;
        PrimarySurface.DisplayNumber = DisplayNumber;
        ret = TRUE;
        goto cleanup;
    }

cleanup:
    /* Signal we're done */
    KeSetEvent(&VideoDriverPrepared, 1, FALSE);
    return ret;
}

BOOL FASTCALL
IntPrepareDriverIfNeeded()
{
    /* Lazy init the driver if it hasn't been initialized yet */
    if (PrimarySurface.PreparedDriver)
        return TRUE;
    else
        return IntPrepareDriver();
}

BOOL FASTCALL
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

    if (! IntPrepareDriverIfNeeded())
    {
        return FALSE;
    }

    /*if (! PrepareVideoPrt())
    {
        return FALSE;
    }*/

    /* Enable the drawing surface */
    PrimarySurface.pSurface =
        PrimarySurface.DriverFunctions.EnableSurface(PrimarySurface.hPDev);
    if (!PrimarySurface.pSurface)
    {
        /*      PrimarySurface.DriverFunctions.AssertMode(PrimarySurface.hPDev, FALSE);*/
        PrimarySurface.DriverFunctions.DisablePDEV(PrimarySurface.hPDev);
        ObDereferenceObject(PrimarySurface.VideoFileObject);
        DPRINT1("DrvEnableSurface failed\n");
        return FALSE;
    }

    /* Set the videomode */
    PrimarySurface.DriverFunctions.AssertMode(PrimarySurface.hPDev, TRUE);

    SurfObj = EngLockSurface(PrimarySurface.pSurface);
    SurfObj->dhpdev = PrimarySurface.hPDev;
    SurfSize = SurfObj->sizlBitmap;
    SurfaceRect.left = SurfaceRect.top = 0;
    SurfaceRect.right = SurfObj->sizlBitmap.cx;
    SurfaceRect.bottom = SurfObj->sizlBitmap.cy;
    //EngEraseSurface(SurfObj, &SurfaceRect, 0);

    /* Put the pointer in the center of the screen */
    //gpsi->ptCursor.x = (SurfaceRect.right - SurfaceRect.left) / 2;
    //gpsi->ptCursor.y = (SurfaceRect.bottom - SurfaceRect.top) / 2;

    /* Give the PDEV a MovePointer function */
    PrimarySurface.pfnMovePointer = PrimarySurface.DriverFunctions.MovePointer;
    if (!PrimarySurface.pfnMovePointer)
        PrimarySurface.pfnMovePointer = EngMovePointer;

    EngUnlockSurface(SurfObj);

    // Init Primary Displays Device Capabilities.
    //IntvGetDeviceCaps(&PrimarySurface, &GdiHandleTable->DevCaps);

    return TRUE;
}

VOID FASTCALL
IntDestroyPrimarySurface()
{
    DPRINT("Reseting display\n" );
    PrimarySurface.DriverFunctions.AssertMode(PrimarySurface.hPDev, FALSE);
    PrimarySurface.DriverFunctions.DisableSurface(PrimarySurface.hPDev);
    PrimarySurface.DriverFunctions.DisablePDEV(PrimarySurface.hPDev);
    PrimarySurface.PreparedDriver = FALSE;
    KeSetEvent(&VideoDriverNeedsPreparation, 1, FALSE);
    KeResetEvent(&VideoDriverPrepared);

    ObDereferenceObject(PrimarySurface.VideoFileObject);
}

INT APIENTRY
GreGetDeviceCaps(PDC pDC, INT cap)
{
    /* Get a pointer to the physical device */
    PPDEVOBJ ppDevObj = pDC->pPDevice;
    GDIINFO *pGdiInfo = &ppDevObj->GDIInfo;
    ULONG palette_size=0;

    switch(cap)
    {
    case DRIVERVERSION:
        return 0x300;
    case TECHNOLOGY:
        return DT_RASDISPLAY;
    case HORZSIZE:
        return pGdiInfo->ulHorzSize;
    case VERTSIZE:
        return pGdiInfo->ulVertSize;
    case HORZRES:
        return pGdiInfo->ulHorzRes;
    case VERTRES:
        return pGdiInfo->ulVertRes;
    case DESKTOPHORZRES:
        return pGdiInfo->ulHorzRes;
    case DESKTOPVERTRES:
        return pGdiInfo->ulVertRes;
    case BITSPIXEL:
        return pGdiInfo->cBitsPixel;
    case PLANES:
        return 1;
    case NUMBRUSHES:
        return -1;
    case NUMPENS:
        return -1;
    case NUMMARKERS:
        return 0;
    case NUMFONTS:
        return 0;
    case NUMCOLORS:
        /* MSDN: Number of entries in the device's color table, if the device has
         * a color depth of no more than 8 bits per pixel.For devices with greater
         * color depths, -1 is returned. */
        return (pGdiInfo->cBitsPixel > 8) ? -1 : (1 << pGdiInfo->cBitsPixel);
        //return pGdiInfo->ulNumColors;
    case PDEVICESIZE:
        return sizeof(NTDRV_PDEVICE);
    case CURVECAPS:
        return (CC_CIRCLES | CC_PIE | CC_CHORD | CC_ELLIPSES | CC_WIDE |
                CC_STYLED | CC_WIDESTYLED | CC_INTERIORS | CC_ROUNDRECT);
    case LINECAPS:
        return (LC_POLYLINE | LC_MARKER | LC_POLYMARKER | LC_WIDE |
                LC_STYLED | LC_WIDESTYLED | LC_INTERIORS);
    case POLYGONALCAPS:
        return (PC_POLYGON | PC_RECTANGLE | PC_WINDPOLYGON | PC_SCANLINE |
                PC_WIDE | PC_STYLED | PC_WIDESTYLED | PC_INTERIORS);
    case TEXTCAPS:
        return pGdiInfo->flTextCaps | TC_VA_ABLE;
    case CLIPCAPS:
        return CP_REGION;
    case RASTERCAPS:
        return (RC_BITBLT | RC_BANDING | RC_SCALING | RC_BITMAP64 | RC_DI_BITMAP |
                RC_DIBTODEV | RC_BIGFONT | RC_STRETCHBLT | RC_STRETCHDIB | RC_DEVBITS |
                (palette_size ? RC_PALETTE : 0));
    case SHADEBLENDCAPS:
        return (SB_GRAD_RECT | SB_GRAD_TRI | SB_CONST_ALPHA | SB_PIXEL_ALPHA);
    case ASPECTX:
    case ASPECTY:
        return 36;
    case ASPECTXY:
        return 51;
    case LOGPIXELSX:
        return pGdiInfo->ulLogPixelsX;
    case LOGPIXELSY:
        return pGdiInfo->ulLogPixelsY;
    case CAPS1:
        DPRINT1("(%p): CAPS1 is unimplemented, will return 0\n", pDC );
        /* please see wingdi.h for the possible bit-flag values that need
           to be returned. */
        return 0;
    case SIZEPALETTE:
        return palette_size;
    case NUMRESERVED:
    case COLORRES:
    case PHYSICALWIDTH:
    case PHYSICALHEIGHT:
    case PHYSICALOFFSETX:
    case PHYSICALOFFSETY:
    case SCALINGFACTORX:
    case SCALINGFACTORY:
    case VREFRESH:
    case BLTALIGNMENT:
        return 0;
    default:
        DPRINT1("(%p): unsupported capability %d, will return 0\n", pDC, cap );
        return 0;
    }
}

/* EOF */
