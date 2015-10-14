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
                                       sizeof(PrimarySurface.gdiinfo),
                                       (PULONG)&PrimarySurface.gdiinfo,
                                       sizeof(PrimarySurface.devinfo),
                                       &PrimarySurface.devinfo,
                                       NULL,
                                       L"",
                                       PrimarySurface.VideoFileObject->DeviceObject);

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
                                       sizeof(PrimarySurface.gdiinfo),
                                       (PULONG)&PrimarySurface.gdiinfo,
                                       sizeof(PrimarySurface.devinfo),
                                       &PrimarySurface.devinfo,
                                       NULL,
                                       L"",
                                       PrimarySurface.VideoFileObject->DeviceObject);

            if (!PrimarySurface.hPDev)
            {
                ObDereferenceObject(PrimarySurface.VideoFileObject);
                DPRINT1("DrvEnablePDEV with default parameters failed\n");
                DPRINT1("Perhaps DDI driver doesn't match miniport driver?\n");
                continue;
            }

            /* Update the primary surface with what we really got */
            PrimarySurface.DMW.dmPelsWidth = PrimarySurface.gdiinfo.ulHorzRes;
            PrimarySurface.DMW.dmPelsHeight = PrimarySurface.gdiinfo.ulVertRes;
            PrimarySurface.DMW.dmBitsPerPel = PrimarySurface.gdiinfo.cBitsPixel;
            PrimarySurface.DMW.dmDisplayFrequency = PrimarySurface.gdiinfo.ulVRefresh;
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
        if (!PrimarySurface.gdiinfo.ulLogPixelsX)
        {
            DPRINT("Adjusting GDIInfo.ulLogPixelsX\n");
            PrimarySurface.gdiinfo.ulLogPixelsX = 96;
        }
        if (!PrimarySurface.gdiinfo.ulLogPixelsY)
        {
            DPRINT("Adjusting GDIInfo.ulLogPixelsY\n");
            PrimarySurface.gdiinfo.ulLogPixelsY = 96;
        }

        PrimarySurface.Pointer.Exclude.right = -1;

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

    ObReferenceObjectByPointer(FileObject, 0, *IoFileObjectType, KernelMode);

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

    if (! PrepareVideoPrt())
    {
        return FALSE;
    }

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
    SwmUpdateRootWindow(SurfObj);
    //EngEraseSurface(SurfObj, &SurfaceRect, 0);

    /* Give the PDEV a MovePointer function */
    PrimarySurface.pfnMovePointer = PrimarySurface.DriverFunctions.MovePointer;
    if (!PrimarySurface.pfnMovePointer)
        PrimarySurface.pfnMovePointer = EngMovePointer;

    /* attach monitor */
    AttachMonitor(&PrimarySurface, PrimarySurface.DisplayNumber);

    /* Initialize mouse pointer */
    RosUserSetCursor(NULL);

    /* Put the pointer in the center of the screen */
    RosUserSetCursorPos((SurfaceRect.right - SurfaceRect.left) / 2,
                        (SurfaceRect.bottom - SurfaceRect.top) / 2);

    EngUnlockSurface(SurfObj);
    CsrNotifyShowDesktop(NULL, SurfSize.cx, SurfSize.cy);

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

NTSTATUS FASTCALL
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
GreChangeDisplaySettings(
    IN PUNICODE_STRING pDeviceName  OPTIONAL,
    IN LPDEVMODEW DevMode,
    IN DWORD dwflags,
    IN PVOID lParam  OPTIONAL)
{
    BOOLEAN Global = FALSE;
    BOOLEAN NoReset = FALSE;
    BOOLEAN Reset = FALSE;
    BOOLEAN SetPrimary = FALSE;
    LONG Ret = DISP_CHANGE_SUCCESSFUL;
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

    DPRINT1("Global: %d, SetPrimary: %d\n", Global, SetPrimary);

    if (Reset && NoReset)
        return DISP_CHANGE_BADFLAGS;

    if (dwflags == 0)
    {
        /* Dynamically change graphics mode */
        DPRINT1("flag 0 UNIMPLEMENTED\n");
        return DISP_CHANGE_FAILED;
        SetLastWin32Error(ERROR_CALL_NOT_IMPLEMENTED);
    }

    if ((dwflags & CDS_TEST) == CDS_TEST)
    {
        /* Test resolution */
        dwflags &= ~CDS_TEST;
        Status = GreEnumDisplaySettings(pDeviceName, ENUM_REGISTRY_SETTINGS, DevMode, 0);
        if (!NT_SUCCESS(Status))
            Ret = DISP_CHANGE_BADMODE;
        return Ret;
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

        Status = GreEnumDisplaySettings(pDeviceName, ENUM_CURRENT_SETTINGS, &lpDevMode, 0);
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
            SetLastWin32Error(ERROR_CALL_NOT_IMPLEMENTED);
        }

    }

    if ((dwflags & CDS_UPDATEREGISTRY) == CDS_UPDATEREGISTRY)
    {

        UNICODE_STRING DeviceName;
        UNICODE_STRING RegistryKey;
        OBJECT_ATTRIBUTES ObjectAttributes;
        HANDLE DevInstRegKey;
        ULONG NewValue;

        DPRINT1("set CDS_UPDATEREGISTRY\n");

        dwflags &= ~CDS_UPDATEREGISTRY;

        /* Check if pDeviceName is NULL, we need to retrieve it */
        if (pDeviceName == NULL)
        {
            // FIXME
            DPRINT1("fixme: pDeviceName is NULL\n");
            return DISP_CHANGE_FAILED;
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

    DPRINT("IntChangeDisplaySettings returning %x\n", Ret);
    return Ret;
}

NTSTATUS FASTCALL
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
        /* FIXME
        
        PWINDOW_OBJECT DesktopObject;
        HDC DesktopHDC;
        PDC pDC;

        DesktopObject = UserGetDesktopWindow();
        DesktopHDC = UserGetWindowDC(DesktopObject);
        pDC = DC_LockDc(DesktopHDC);

        *DisplayNumber = pDC->ppdev->DisplayNumber;

        DC_UnlockDc(pDC);
        UserReleaseDC(DesktopObject, DesktopHDC, FALSE);
        */
        *DisplayNumber = 1;
        DPRINT1("FIXME: DeviceName is NULL, trying display nr. 1\n");
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

NTSTATUS
FASTCALL
GreEnumDisplaySettings(
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
                        RtlCopyUnicodeString(&CachedDeviceName, pDeviceName);

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

INT APIENTRY
GreGetDeviceCaps(PDC pDC, INT cap)
{
    /* Get a pointer to the physical device */
    PPDEVOBJ ppDevObj = pDC->ppdev;
    GDIINFO *pGdiInfo = &ppDevObj->gdiinfo;
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
        return 0;
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
