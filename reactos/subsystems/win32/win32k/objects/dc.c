/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * DC.C - Device context functions
 *
 */

#include <w32k.h>
#include <bugcodes.h>

#define NDEBUG
#include <debug.h>

//  ---------------------------------------------------------  File Statics

static PDEVOBJ PrimarySurface;
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

/* FIXME: DCs should probably be thread safe  */

//  -----------------------------------------------------  Public Functions

BOOL APIENTRY
NtGdiCancelDC(HDC  hDC)
{
  UNIMPLEMENTED;
  return FALSE;
}

HDC APIENTRY
NtGdiCreateCompatibleDC(HDC hDC)
{
  PDC  NewDC, OrigDC;
  PDC_ATTR pdcattrNew, pdcattrOld;
  HDC hNewDC, DisplayDC = NULL;
  HRGN hVisRgn;
  UNICODE_STRING DriverName;
  DWORD Layout = 0;

  if (hDC == NULL)
    {
      RtlInitUnicodeString(&DriverName, L"DISPLAY");
      DisplayDC = IntGdiCreateDC(&DriverName, NULL, NULL, NULL, TRUE);
      if (NULL == DisplayDC)
        {
          DPRINT1("Failed to create DisplayDC\n");
          return NULL;
        }
      hDC = DisplayDC;
    }

  /*  Allocate a new DC based on the original DC's device  */
  OrigDC = DC_LockDc(hDC);
  if (NULL == OrigDC)
    {
      if (NULL != DisplayDC)
        {
          NtGdiDeleteObjectApp(DisplayDC);
        }
      DPRINT1("Failed to lock hDC\n");
      return NULL;
    }
  hNewDC = DC_AllocDC(&OrigDC->rosdc.DriverName);
  if (NULL == hNewDC)
    {
      DPRINT1("Failed to create hNewDC\n");
      DC_UnlockDc(OrigDC);
      if (NULL != DisplayDC)
        {
          NtGdiDeleteObjectApp(DisplayDC);
        }
      return  NULL;
    }
  NewDC = DC_LockDc( hNewDC );

  if(!NewDC)
  {
    DPRINT1("Failed to lock hNewDC\n");
    NtGdiDeleteObjectApp(hNewDC);
    return NULL;
  }

  pdcattrOld = OrigDC->pdcattr;
  pdcattrNew = NewDC->pdcattr;

  /* Copy information from original DC to new DC  */
  NewDC->dclevel.hdcSave = hNewDC;

  NewDC->dhpdev = OrigDC->dhpdev;

  NewDC->rosdc.bitsPerPixel = OrigDC->rosdc.bitsPerPixel;

  /* DriverName is copied in the AllocDC routine  */
  pdcattrNew->ptlWindowOrg   = pdcattrOld->ptlWindowOrg;
  pdcattrNew->szlWindowExt   = pdcattrOld->szlWindowExt;
  pdcattrNew->ptlViewportOrg = pdcattrOld->ptlViewportOrg;
  pdcattrNew->szlViewportExt = pdcattrOld->szlViewportExt;

  NewDC->dctype        = DC_TYPE_MEMORY; // Always!
  NewDC->rosdc.hBitmap      = NtGdiGetStockObject(DEFAULT_BITMAP);
  NewDC->ppdev          = OrigDC->ppdev;
  NewDC->dclevel.hpal    = OrigDC->dclevel.hpal;

  pdcattrNew->lTextAlign      = pdcattrOld->lTextAlign;
  pdcattrNew->lBkMode         = pdcattrOld->lBkMode;
  pdcattrNew->jBkMode         = pdcattrOld->jBkMode;
  pdcattrNew->jROP2           = pdcattrOld->jROP2;
  pdcattrNew->dwLayout        = pdcattrOld->dwLayout;
  if (pdcattrOld->dwLayout & LAYOUT_ORIENTATIONMASK) Layout = pdcattrOld->dwLayout;
  NewDC->dclevel.flPath     = OrigDC->dclevel.flPath;
  pdcattrNew->ulDirty_        = pdcattrOld->ulDirty_;
  pdcattrNew->iCS_CP          = pdcattrOld->iCS_CP;

  NewDC->erclWindow = (RECTL){0,0,1,1};

  DC_UnlockDc(NewDC);
  DC_UnlockDc(OrigDC);
  if (NULL != DisplayDC)
  {
     NtGdiDeleteObjectApp(DisplayDC);
  }

  hVisRgn = NtGdiCreateRectRgn(0, 0, 1, 1);
  if (hVisRgn)
  {
    GdiSelectVisRgn(hNewDC, hVisRgn);
    NtGdiDeleteObject(hVisRgn);
  }
  if (Layout) NtGdiSetLayout( hNewDC, -1, Layout);

  DC_InitDC(hNewDC);
  return hNewDC;
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
   PGD_ENABLEDRIVER GDEnableDriver;
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
            (ULONG *) &PrimarySurface.GDIInfo,
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
            (ULONG *) &PrimarySurface.GDIInfo,
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

static BOOL FASTCALL
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
   if (!calledFromUser){
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

   EngUnlockSurface(SurfObj);
   co_IntShowDesktop(IntGetActiveDesktop(), SurfSize.cx, SurfSize.cy);

   // Init Primary Displays Device Capabilities.
   IntvGetDeviceCaps(&PrimarySurface, &GdiHandleTable->DevCaps);

   if (!calledFromUser){
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
    if (!calledFromUser){
       UserEnterExclusive();
    }

    /* detach monitor */
    IntDetachMonitor(&PrimarySurface);

    if (!calledFromUser){
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

HDC FASTCALL
IntGdiCreateDC(PUNICODE_STRING Driver,
               PUNICODE_STRING Device,
               PVOID pUMdhpdev,
               CONST PDEVMODEW InitData,
               BOOL CreateAsIC)
{
  HDC      hdc;
  PDC      pdc;
  PDC_ATTR pdcattr;
  HRGN     hVisRgn;
  UNICODE_STRING StdDriver;
  BOOL calledFromUser;

  RtlInitUnicodeString(&StdDriver, L"DISPLAY");

  DPRINT("DriverName: %wZ, DeviceName: %wZ\n", Driver, Device);

  if (NULL == Driver || 0 == RtlCompareUnicodeString(Driver, &StdDriver, TRUE))
    {
      if (CreateAsIC)
        {
          if (! IntPrepareDriverIfNeeded())
            {
              /* Here, we have two possibilities:
               * a) return NULL, and hope that the caller
               *    won't call us in a loop
               * b) bugcheck, but caller is unable to
               *    react on the problem
               */
              /*DPRINT1("Unable to prepare graphics driver, returning NULL ic\n");
              return NULL;*/
              KeBugCheck(VIDEO_DRIVER_INIT_FAILURE);
            }
        }
      else
        {
          calledFromUser = UserIsEntered();
          if (!calledFromUser){
             UserEnterExclusive();
          }

          if (! co_IntGraphicsCheck(TRUE))
          {
            if (!calledFromUser){
               UserLeave();
            }
            DPRINT1("Unable to initialize graphics, returning NULL dc\n");
            return NULL;
          }

          if (!calledFromUser){
            UserLeave();
          }

        }
    }

  /*  Check for existing DC object  */
  if ((hdc = DC_FindOpenDC(Driver)) != NULL)
  {
    hdc = NtGdiCreateCompatibleDC(hdc);
    if (!hdc)
       DPRINT1("NtGdiCreateCompatibleDC() failed\n");
    return hdc;
  }

  /*  Allocate a DC object  */
  if ((hdc = DC_AllocDC(Driver)) == NULL)
  {
    DPRINT1("DC_AllocDC() failed\n");
    return  NULL;
  }

  pdc = DC_LockDc( hdc );
  if ( !pdc )
  {
    DC_FreeDC( hdc );
    DPRINT1("DC_LockDc() failed\n");
    return NULL;
  }

  pdcattr = pdc->pdcattr;

  pdc->dctype = DC_TYPE_DIRECT;

  pdc->dhpdev = PrimarySurface.hPDev;
  if(pUMdhpdev) pUMdhpdev = pdc->dhpdev; // set DHPDEV for device.
  pdc->ppdev = (PVOID)&PrimarySurface;
  pdc->rosdc.hBitmap = (HBITMAP)PrimarySurface.pSurface;
  // ATM we only have one display.
  pdcattr->ulDirty_ |= DC_PRIMARY_DISPLAY;

  pdc->rosdc.bitsPerPixel = pdc->ppdev->GDIInfo.cBitsPixel *
                                     pdc->ppdev->GDIInfo.cPlanes;
  DPRINT("Bits per pel: %u\n", pdc->rosdc.bitsPerPixel);

  pdc->flGraphicsCaps  = PrimarySurface.DevInfo.flGraphicsCaps;
  pdc->flGraphicsCaps2 = PrimarySurface.DevInfo.flGraphicsCaps2;

  pdc->dclevel.hpal = NtGdiGetStockObject(DEFAULT_PALETTE);

  pdcattr->jROP2 = R2_COPYPEN;

  pdc->erclWindow.top = pdc->erclWindow.left = 0;
  pdc->erclWindow.right  = pdc->ppdev->GDIInfo.ulHorzRes;
  pdc->erclWindow.bottom = pdc->ppdev->GDIInfo.ulVertRes;
  pdc->dclevel.flPath &= ~DCPATH_CLOCKWISE; // Default is CCW.

  pdcattr->iCS_CP = ftGdiGetTextCharsetInfo(pdc,NULL,0);

  hVisRgn = NtGdiCreateRectRgn(0, 0, pdc->ppdev->GDIInfo.ulHorzRes,
                                     pdc->ppdev->GDIInfo.ulVertRes);

  if (!CreateAsIC)
  {
    pdc->pSurfInfo = NULL;
//    pdc->dclevel.pSurface = 
    DC_UnlockDc( pdc );

    /*  Initialize the DC state  */
    DC_InitDC(hdc);
    IntGdiSetTextColor(hdc, RGB(0, 0, 0));
    IntGdiSetBkColor(hdc, RGB(255, 255, 255));
  }
  else
  {
    /* From MSDN2:
       The CreateIC function creates an information context for the specified device.
       The information context provides a fast way to get information about the
       device without creating a device context (DC). However, GDI drawing functions
       cannot accept a handle to an information context.
     */
    pdc->dctype = DC_TYPE_INFO;
//    pdc->pSurfInfo = 
    pdc->dclevel.pSurface = NULL;
    pdcattr->crBackgroundClr = pdcattr->ulBackgroundClr = RGB(255, 255, 255);
    pdcattr->crForegroundClr = RGB(0, 0, 0);
    DC_UnlockDc( pdc );
  }

  if (hVisRgn)
  {
     GdiSelectVisRgn(hdc, hVisRgn);
     NtGdiDeleteObject(hVisRgn);
  }

  IntGdiSetTextAlign(hdc, TA_TOP);
  IntGdiSetBkMode(hdc, OPAQUE);

  return hdc;
}

HDC APIENTRY
NtGdiOpenDCW( PUNICODE_STRING Device,
              DEVMODEW *InitData,
              PUNICODE_STRING pustrLogAddr,
              ULONG iType,
              HANDLE hspool,
              VOID *pDriverInfo2,
              VOID *pUMdhpdev )
{
  UNICODE_STRING SafeDevice;
  DEVMODEW SafeInitData;
  PVOID Dhpdev;
  HDC Ret;
  NTSTATUS Status = STATUS_SUCCESS;

  if(InitData)
  {
    _SEH2_TRY
    {
      if (pUMdhpdev)
      {
        ProbeForWrite(pUMdhpdev,
                   sizeof(PVOID),
                   1);
      }
      ProbeForRead(InitData,
                   sizeof(DEVMODEW),
                   1);
      RtlCopyMemory(&SafeInitData,
                    InitData,
                    sizeof(DEVMODEW));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
      Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    if(!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return NULL;
    }
    /* FIXME - InitData can have some more bytes! */
  }

  if(Device)
  {
    Status = IntSafeCopyUnicodeString(&SafeDevice, Device);
    if(!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return NULL;
    }
  }

  Ret = IntGdiCreateDC(Device ? &SafeDevice : NULL,
                       NULL,
                       pUMdhpdev ? &Dhpdev : NULL,
                       InitData ? &SafeInitData : NULL,
                       (BOOL) iType); // FALSE 0 DCW, TRUE 1 ICW

  if (pUMdhpdev) pUMdhpdev = Dhpdev;

  return Ret;

}


HDC FASTCALL
IntGdiCreateDisplayDC(HDEV hDev, ULONG DcType, BOOL EmptyDC)
{
  HDC hDC;
  UNICODE_STRING DriverName;
  RtlInitUnicodeString(&DriverName, L"DISPLAY");

  if (DcType != DC_TYPE_MEMORY)
     hDC = IntGdiCreateDC(&DriverName, NULL, NULL, NULL, (DcType == DC_TYPE_INFO));
  else
     hDC = NtGdiCreateCompatibleDC(NULL); // OH~ Yuck! I think I taste vomit in my mouth!
//
// There is room to grow here~
//

//
// If NULL, first time through! Build the default (was window) dc!
//
  if (hDC && !defaultDCstate) // Ultra HAX! Dedicated to GvG!
  { // This is a cheesy way to do this.
      PDC dc = DC_LockDc ( hDC );
      defaultDCstate = ExAllocatePoolWithTag(PagedPool, sizeof(DC), TAG_DC);
      if (!defaultDCstate)
      {
          DC_UnlockDc( dc );
          return NULL;
      }
      RtlZeroMemory(defaultDCstate, sizeof(DC));
      defaultDCstate->pdcattr = &defaultDCstate->dcattr;
      IntGdiCopyToSaveState(dc, defaultDCstate);
      DC_UnlockDc( dc );
  }
  return hDC;
}

BOOL FASTCALL
IntGdiCleanDC(HDC hDC)
{
  PDC dc;
  if (!hDC) return FALSE;
  dc = DC_LockDc ( hDC );
  if (!dc) return FALSE;
  // Clean the DC
  if (defaultDCstate) IntGdiCopyFromSaveState(dc, defaultDCstate, hDC );
  return TRUE;
}

//
//
//
BOOL
FASTCALL
IntGdiDeleteDC(HDC hDC, BOOL Force)
{
  PDC  DCToDelete = DC_LockDc(hDC);

  if (DCToDelete == NULL)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  if(!Force)
  {
    if (DCToDelete->fs & DC_FLAG_PERMANENT)
    {
         DPRINT1("No! You Naughty Application!\n");
         DC_UnlockDc( DCToDelete );
         return UserReleaseDC(NULL, hDC, FALSE);
    }
  }

  /*  First delete all saved DCs  */
  while (DCToDelete->dclevel.lSaveDepth)
  {
    PDC  savedDC;
    HDC  savedHDC;

    savedHDC = DC_GetNextDC (DCToDelete);
    savedDC = DC_LockDc (savedHDC);
    if (savedDC == NULL)
    {
      break;
    }
    DC_SetNextDC (DCToDelete, DC_GetNextDC (savedDC));
    DCToDelete->dclevel.lSaveDepth--;
    DC_UnlockDc( savedDC );
    IntGdiDeleteDC(savedHDC, Force);
  }

  /*  Free GDI resources allocated to this DC  */
  if (!(DCToDelete->dclevel.flPath & DCPATH_SAVESTATE))
  {
    /*
    NtGdiSelectPen (DCHandle, STOCK_BLACK_PEN);
    NtGdiSelectBrush (DCHandle, STOCK_WHITE_BRUSH);
    NtGdiSelectFont (DCHandle, STOCK_SYSTEM_FONT);
    DC_LockDC (DCHandle); NtGdiSelectXxx does not recognize stock objects yet  */
    if (DCToDelete->rosdc.XlateBrush != NULL)
      EngDeleteXlate(DCToDelete->rosdc.XlateBrush);
    if (DCToDelete->rosdc.XlatePen != NULL)
      EngDeleteXlate(DCToDelete->rosdc.XlatePen);
  }
  if (DCToDelete->rosdc.hClipRgn)
  {
    NtGdiDeleteObject (DCToDelete->rosdc.hClipRgn);
  }
  if (DCToDelete->rosdc.hVisRgn)
  {
    NtGdiDeleteObject (DCToDelete->rosdc.hVisRgn);
  }
  if (NULL != DCToDelete->rosdc.CombinedClip)
  {
    IntEngDeleteClipRegion(DCToDelete->rosdc.CombinedClip);
  }
  if (DCToDelete->rosdc.hGCClipRgn)
  {
    NtGdiDeleteObject (DCToDelete->rosdc.hGCClipRgn);
  }
  PATH_Delete(DCToDelete->dclevel.hPath);

  DC_UnlockDc( DCToDelete );
  DC_FreeDC ( hDC );
  return TRUE;
}

BOOL
APIENTRY
NtGdiDeleteObjectApp(HANDLE  DCHandle)
{
  /* Complete all pending operations */
  NtGdiFlushUserBatch();

  if (GDI_HANDLE_IS_STOCKOBJ(DCHandle)) return TRUE;

  if (GDI_HANDLE_GET_TYPE(DCHandle) != GDI_OBJECT_TYPE_DC)
     return NtGdiDeleteObject((HGDIOBJ) DCHandle);

  if(IsObjectDead((HGDIOBJ)DCHandle)) return TRUE;

  if (!GDIOBJ_OwnedByCurrentProcess(DCHandle))
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  return IntGdiDeleteDC(DCHandle, FALSE);
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

ULONG
APIENTRY
NtGdiEnumObjects(
    IN HDC hdc,
    IN INT iObjectType,
    IN ULONG cjBuf,
    OUT OPTIONAL PVOID pvBuf)
{
  UNIMPLEMENTED;
  return 0;
}

HANDLE
APIENTRY
NtGdiGetDCObject(HDC  hDC, INT  ObjectType)
{
  HGDIOBJ SelObject;
  DC *dc;
  PDC_ATTR pdcattr;

  /* From Wine: GetCurrentObject does not SetLastError() on a null object */
  if(!hDC) return NULL;

  if(!(dc = DC_LockDc(hDC)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return NULL;
  }
  pdcattr = dc->pdcattr;

  if (pdcattr->ulDirty_ & DC_BRUSH_DIRTY)
     IntGdiSelectBrush(dc,pdcattr->hbrush);

  if (pdcattr->ulDirty_ & DC_PEN_DIRTY)
     IntGdiSelectPen(dc,pdcattr->hpen);

  switch(ObjectType)
  {
    case GDI_OBJECT_TYPE_EXTPEN:
    case GDI_OBJECT_TYPE_PEN:
      SelObject = pdcattr->hpen;
      break;
    case GDI_OBJECT_TYPE_BRUSH:
      SelObject = pdcattr->hbrush;
      break;
    case GDI_OBJECT_TYPE_PALETTE:
      SelObject = dc->dclevel.hpal;
      break;
    case GDI_OBJECT_TYPE_FONT:
      SelObject = pdcattr->hlfntNew;
      break;
    case GDI_OBJECT_TYPE_BITMAP:
      SelObject = dc->rosdc.hBitmap;
      break;
    case GDI_OBJECT_TYPE_COLORSPACE:
      DPRINT1("FIXME: NtGdiGetCurrentObject() ObjectType OBJ_COLORSPACE not supported yet!\n");
      // SelObject = dc->dclevel.pColorSpace.BaseObject.hHmgr; ?
      SelObject = NULL;
      break;
    default:
      SelObject = NULL;
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      break;
  }

  DC_UnlockDc(dc);
  return SelObject;
}

LONG FASTCALL
IntCalcFillOrigin(PDC pdc)
{
  pdc->ptlFillOrigin.x = pdc->dclevel.ptlBrushOrigin.x + pdc->ptlDCOrig.x;
  pdc->ptlFillOrigin.y = pdc->dclevel.ptlBrushOrigin.y + pdc->ptlDCOrig.y;

  return pdc->ptlFillOrigin.y;
}

VOID
APIENTRY
GdiSetDCOrg(HDC hDC, LONG Left, LONG Top, PRECTL prc)
{
  PDC pdc;

  pdc = DC_LockDc(hDC);
  if (!pdc) return;

  pdc->ptlDCOrig.x = Left;
  pdc->ptlDCOrig.y = Top;

  IntCalcFillOrigin(pdc);

  if (prc) pdc->erclWindow = *prc;

  DC_UnlockDc(pdc);
}


BOOL FASTCALL
IntGdiGetDCOrg(PDC pDc, PPOINTL ppt)
{
  *ppt = pDc->ptlDCOrig;
  return TRUE;
}

BOOL APIENTRY
GdiGetDCOrgEx(HDC hDC, PPOINTL ppt, PRECTL prc)
{
  PDC pdc;

  pdc = DC_LockDc(hDC);
  if (!pdc) return FALSE;

  *prc = pdc->erclWindow;
  *ppt = pdc->ptlDCOrig;

  DC_UnlockDc(pdc);
  return TRUE;
}

BOOL FASTCALL
IntGetAspectRatioFilter(PDC pDC,
                        LPSIZE AspectRatio)
{
  PDC_ATTR pdcattr;

  pdcattr = pDC->pdcattr;

  if ( pdcattr->flFontMapper & 1 ) // TRUE assume 1.
  {
   // "This specifies that Windows should only match fonts that have the
   // same aspect ratio as the display.", Programming Windows, Fifth Ed.
     AspectRatio->cx = pDC->ppdev->GDIInfo.ulLogPixelsX;
     AspectRatio->cy = pDC->ppdev->GDIInfo.ulLogPixelsY;
  }
  else
  {
     AspectRatio->cx = 0;
     AspectRatio->cy = 0;
  }
  return TRUE;
}

VOID
FASTCALL
IntGetViewportExtEx(PDC pdc, LPSIZE pSize)
{
    PDC_ATTR pdcattr;

    /* Get a pointer to the dc attribute */
    pdcattr = pdc->pdcattr;

    /* Check if we need to recalculate */
    if (pdcattr->flXform & PAGE_EXTENTS_CHANGED)
    {
        /* Check if we need to do isotropic fixup */
        if (pdcattr->iMapMode == MM_ISOTROPIC)
        {
            IntFixIsotropicMapping(pdc);
        }

        /* Update xforms, CHECKME: really done here? */
        DC_UpdateXforms(pdc);
    }

    /* Copy the viewport extension */
    *pSize = pdcattr->szlViewportExt;
}

BOOL APIENTRY
NtGdiGetDCPoint( HDC hDC, UINT iPoint, PPOINTL Point)
{
  BOOL Ret = TRUE;
  DC *dc;
  POINTL SafePoint;
  SIZE Size;
  NTSTATUS Status = STATUS_SUCCESS;

  if(!Point)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  RtlZeroMemory(&SafePoint, sizeof(POINT));

  dc = DC_LockDc(hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  switch (iPoint)
  {
    case GdiGetViewPortExt:
      IntGetViewportExtEx(dc, &Size);
      SafePoint.x = Size.cx;
      SafePoint.y = Size.cy;
      break;
    case GdiGetWindowExt:
      IntGetWindowExtEx(dc, &Size);
      SafePoint.x = Size.cx;
      SafePoint.y = Size.cy;
      break;
    case GdiGetViewPortOrg:
      IntGetViewportOrgEx(dc, &SafePoint);
      break;
    case GdiGetWindowOrg:
      IntGetWindowOrgEx(dc, &SafePoint);
      break;
    case GdiGetDCOrg:
      Ret = IntGdiGetDCOrg(dc, &SafePoint);
      break;
    case GdiGetAspectRatioFilter:
      Ret = IntGetAspectRatioFilter(dc, &Size);
      SafePoint.x = Size.cx;
      SafePoint.y = Size.cy;
      break;
    default:
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      Ret = FALSE;
      break;
  }

  if (Ret)
  {
    _SEH2_TRY
    {
      ProbeForWrite(Point,
                    sizeof(POINT),
                    1);
      *Point = SafePoint;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
      Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
  }

  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    DC_UnlockDc(dc);
    return FALSE;
  }

  DC_UnlockDc(dc);
  return Ret;
}

VOID
FASTCALL
IntGdiCopyToSaveState(PDC dc, PDC newdc)
{
  PDC_ATTR pdcattr, nDc_Attr;

  pdcattr = dc->pdcattr;
  nDc_Attr = newdc->pdcattr;

  newdc->dclevel.flPath     = dc->dclevel.flPath | DCPATH_SAVESTATE;

  nDc_Attr->dwLayout        = pdcattr->dwLayout;
  nDc_Attr->hpen            = pdcattr->hpen;
  nDc_Attr->hbrush          = pdcattr->hbrush;
  nDc_Attr->hlfntNew        = pdcattr->hlfntNew;
  newdc->rosdc.hBitmap          = dc->rosdc.hBitmap;
  newdc->dclevel.hpal       = dc->dclevel.hpal;
  newdc->rosdc.bitsPerPixel     = dc->rosdc.bitsPerPixel;
  nDc_Attr->jROP2           = pdcattr->jROP2;
  nDc_Attr->jFillMode       = pdcattr->jFillMode;
  nDc_Attr->jStretchBltMode = pdcattr->jStretchBltMode;
  nDc_Attr->lRelAbs         = pdcattr->lRelAbs;
  nDc_Attr->jBkMode         = pdcattr->jBkMode;
  nDc_Attr->lBkMode         = pdcattr->lBkMode;
  nDc_Attr->crBackgroundClr = pdcattr->crBackgroundClr;
  nDc_Attr->crForegroundClr = pdcattr->crForegroundClr;
  nDc_Attr->ulBackgroundClr = pdcattr->ulBackgroundClr;
  nDc_Attr->ulForegroundClr = pdcattr->ulForegroundClr;
  nDc_Attr->ptlBrushOrigin  = pdcattr->ptlBrushOrigin;
  nDc_Attr->lTextAlign      = pdcattr->lTextAlign;
  nDc_Attr->lTextExtra      = pdcattr->lTextExtra;
  nDc_Attr->cBreak          = pdcattr->cBreak;
  nDc_Attr->lBreakExtra     = pdcattr->lBreakExtra;
  nDc_Attr->iMapMode        = pdcattr->iMapMode;
  nDc_Attr->iGraphicsMode   = pdcattr->iGraphicsMode;
#if 0
  /* Apparently, the DC origin is not changed by [GS]etDCState */
  newdc->ptlDCOrig.x           = dc->ptlDCOrig.x;
  newdc->ptlDCOrig.y           = dc->ptlDCOrig.y;
#endif
  nDc_Attr->ptlCurrent      = pdcattr->ptlCurrent;
  nDc_Attr->ptfxCurrent     = pdcattr->ptfxCurrent;
  newdc->dclevel.mxWorldToDevice = dc->dclevel.mxWorldToDevice;
  newdc->dclevel.mxDeviceToWorld = dc->dclevel.mxDeviceToWorld;
  newdc->dclevel.mxWorldToPage   = dc->dclevel.mxWorldToPage;
  nDc_Attr->flXform         = pdcattr->flXform;
  nDc_Attr->ptlWindowOrg    = pdcattr->ptlWindowOrg;
  nDc_Attr->szlWindowExt    = pdcattr->szlWindowExt;
  nDc_Attr->ptlViewportOrg  = pdcattr->ptlViewportOrg;
  nDc_Attr->szlViewportExt  = pdcattr->szlViewportExt;

  newdc->dclevel.lSaveDepth = 0;
  newdc->dctype = dc->dctype;

#if 0
  PATH_InitGdiPath( &newdc->dclevel.hPath );
#endif

  /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */

  newdc->rosdc.hGCClipRgn = newdc->rosdc.hVisRgn = 0;
  if (dc->rosdc.hClipRgn)
  {
    newdc->rosdc.hClipRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
    NtGdiCombineRgn( newdc->rosdc.hClipRgn, dc->rosdc.hClipRgn, 0, RGN_COPY );
  }
}


VOID
FASTCALL
IntGdiCopyFromSaveState(PDC dc, PDC dcs, HDC hDC)
{
  PDC_ATTR pdcattr, sDc_Attr;

  pdcattr = dc->pdcattr;
  sDc_Attr = dcs->pdcattr;

  dc->dclevel.flPath       = dcs->dclevel.flPath & ~DCPATH_SAVESTATE;

  pdcattr->dwLayout        = sDc_Attr->dwLayout;
  pdcattr->jROP2           = sDc_Attr->jROP2;
  pdcattr->jFillMode       = sDc_Attr->jFillMode;
  pdcattr->jStretchBltMode = sDc_Attr->jStretchBltMode;
  pdcattr->lRelAbs         = sDc_Attr->lRelAbs;
  pdcattr->jBkMode         = sDc_Attr->jBkMode;
  pdcattr->crBackgroundClr = sDc_Attr->crBackgroundClr;
  pdcattr->crForegroundClr = sDc_Attr->crForegroundClr;
  pdcattr->lBkMode         = sDc_Attr->lBkMode;
  pdcattr->ulBackgroundClr = sDc_Attr->ulBackgroundClr;
  pdcattr->ulForegroundClr = sDc_Attr->ulForegroundClr;
  pdcattr->ptlBrushOrigin  = sDc_Attr->ptlBrushOrigin;

  pdcattr->lTextAlign      = sDc_Attr->lTextAlign;
  pdcattr->lTextExtra      = sDc_Attr->lTextExtra;
  pdcattr->cBreak          = sDc_Attr->cBreak;
  pdcattr->lBreakExtra     = sDc_Attr->lBreakExtra;
  pdcattr->iMapMode        = sDc_Attr->iMapMode;
  pdcattr->iGraphicsMode   = sDc_Attr->iGraphicsMode;
#if 0
/* Apparently, the DC origin is not changed by [GS]etDCState */
  dc->ptlDCOrig.x             = dcs->ptlDCOrig.x;
  dc->ptlDCOrig.y             = dcs->ptlDCOrig.y;
#endif
  pdcattr->ptlCurrent      = sDc_Attr->ptlCurrent;
  pdcattr->ptfxCurrent     = sDc_Attr->ptfxCurrent;
  dc->dclevel.mxWorldToDevice = dcs->dclevel.mxWorldToDevice;
  dc->dclevel.mxDeviceToWorld = dcs->dclevel.mxDeviceToWorld;
  dc->dclevel.mxWorldToPage   = dcs->dclevel.mxWorldToPage;
  pdcattr->flXform         = sDc_Attr->flXform;
  pdcattr->ptlWindowOrg    = sDc_Attr->ptlWindowOrg;
  pdcattr->szlWindowExt    = sDc_Attr->szlWindowExt;
  pdcattr->ptlViewportOrg  = sDc_Attr->ptlViewportOrg;
  pdcattr->szlViewportExt  = sDc_Attr->szlViewportExt;

  if (dc->dctype != DC_TYPE_MEMORY)
  {
     dc->rosdc.bitsPerPixel = dcs->rosdc.bitsPerPixel;
  }

#if 0
  if (dcs->rosdc.hClipRgn)
  {
    if (!dc->rosdc.hClipRgn)
    {
       dc->rosdc.hClipRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
    }
    NtGdiCombineRgn( dc->rosdc.hClipRgn, dcs->rosdc.hClipRgn, 0, RGN_COPY );
  }
  else
  {
    if (dc->rosdc.hClipRgn)
    {
       NtGdiDeleteObject( dc->rosdc.hClipRgn );
    }
    dc->rosdc.hClipRgn = 0;
  }
  {
    int res;
    res = CLIPPING_UpdateGCRegion( dc );
    ASSERT ( res != ERROR );
  }
  DC_UnlockDc ( dc );
#else
  GdiExtSelectClipRgn(dc, dcs->rosdc.hClipRgn, RGN_COPY);
  DC_UnlockDc ( dc );
#endif
  if(!hDC) return; // Not a MemoryDC or SaveLevel DC, return.

  NtGdiSelectBitmap( hDC, dcs->rosdc.hBitmap );
  NtGdiSelectBrush( hDC, sDc_Attr->hbrush );
  NtGdiSelectFont( hDC, sDc_Attr->hlfntNew );
  NtGdiSelectPen( hDC, sDc_Attr->hpen );

  IntGdiSetBkColor( hDC, sDc_Attr->crBackgroundClr);
  IntGdiSetTextColor( hDC, sDc_Attr->crForegroundClr);

  GdiSelectPalette( hDC, dcs->dclevel.hpal, FALSE );

#if 0
  GDISelectPalette16( hDC, dcs->dclevel.hpal, FALSE );
#endif
}

HDC APIENTRY
IntGdiGetDCState(HDC  hDC)
{
  PDC  newdc, dc;
  HDC hnewdc;

  dc = DC_LockDc(hDC);
  if (dc == NULL)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }

  hnewdc = DC_AllocDC(NULL);
  if (hnewdc == NULL)
  {
    DC_UnlockDc(dc);
    return 0;
  }
  newdc = DC_LockDc( hnewdc );
  /* FIXME - newdc can be NULL!!!! Don't assert here!!! */
  ASSERT( newdc );

  newdc->dclevel.hdcSave = hnewdc;
  IntGdiCopyToSaveState( dc, newdc);

  DC_UnlockDc( newdc );
  DC_UnlockDc( dc );
  return  hnewdc;
}


VOID
APIENTRY
IntGdiSetDCState ( HDC hDC, HDC hDCSave )
{
  PDC  dc, dcs;

  dc = DC_LockDc ( hDC );
  if ( dc )
  {
    dcs = DC_LockDc ( hDCSave );
    if ( dcs )
    {
      if ( dcs->dclevel.flPath & DCPATH_SAVESTATE )
      {
        IntGdiCopyFromSaveState( dc, dcs, dc->dclevel.hdcSave);
      }
      else
      {
        DC_UnlockDc(dc);
      }
      DC_UnlockDc ( dcs );
    }
    else
    {
      DC_UnlockDc ( dc );
      SetLastWin32Error(ERROR_INVALID_HANDLE);
    }
  }
  else
    SetLastWin32Error(ERROR_INVALID_HANDLE);
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

INT
FASTCALL
IntGetColorManagementCaps(PPDEVOBJ pDevObj)
{
  INT ret = CM_NONE;

  if ( pDevObj->flFlags & PDEV_DISPLAY)
  {
     if ( pDevObj->DevInfo.iDitherFormat == BMF_8BPP ||
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



BOOL
APIENTRY
NtGdiResetDC(
    IN HDC hdc,
    IN LPDEVMODEW pdm,
    OUT PBOOL pbBanding,
    IN OPTIONAL VOID *pDriverInfo2,
    OUT VOID *ppUMdhpdev)
{
  UNIMPLEMENTED;
  return 0;
}

BOOL APIENTRY
NtGdiRestoreDC(HDC  hDC, INT  SaveLevel)
{
  PDC dc, dcs;
  BOOL success;

  DPRINT("NtGdiRestoreDC(%lx, %d)\n", hDC, SaveLevel);

  dc = DC_LockDc(hDC);
  if (!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  if (SaveLevel < 0)
      SaveLevel = dc->dclevel.lSaveDepth + SaveLevel + 1;

  if(SaveLevel < 0 || dc->dclevel.lSaveDepth<SaveLevel)
  {
    DC_UnlockDc(dc);
    return FALSE;
  }

  success=TRUE;
  while (dc->dclevel.lSaveDepth >= SaveLevel)
  {
     HDC hdcs = DC_GetNextDC (dc);

     dcs = DC_LockDc (hdcs);
     if (dcs == NULL)
     {
        DC_UnlockDc(dc);
        return FALSE;
     }

     DC_SetNextDC (dc, DC_GetNextDC (dcs));
     dcs->hdcNext = 0;

     if (--dc->dclevel.lSaveDepth < SaveLevel)
     {
         DC_UnlockDc( dc );
         DC_UnlockDc( dcs );

         IntGdiSetDCState(hDC, hdcs);

         dc = DC_LockDc(hDC);
         if(!dc)
         {
            return FALSE;
         }
         // Restore Path by removing it, if the Save flag is set.
         // BeginPath will takecare of the rest.
         if ( dc->dclevel.hPath && dc->dclevel.flPath & DCPATH_SAVE)
         {
            PATH_Delete(dc->dclevel.hPath);
            dc->dclevel.hPath = 0;
            dc->dclevel.flPath &= ~DCPATH_SAVE;
         }
       }
       else
       {
         DC_UnlockDc( dcs );
       }
       NtGdiDeleteObjectApp (hdcs);
  }
  DC_UnlockDc( dc );
  return  success;
}


INT APIENTRY
NtGdiSaveDC(HDC  hDC)
{
  HDC  hdcs;
  PDC  dc, dcs;
  INT  ret;

  DPRINT("NtGdiSaveDC(%lx)\n", hDC);

  if (!(hdcs = IntGdiGetDCState(hDC)))
  {
    return 0;
  }

  dcs = DC_LockDc (hdcs);
  if (dcs == NULL)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }
  dc = DC_LockDc (hDC);
  if (dc == NULL)
  {
    DC_UnlockDc(dcs);
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }

  /* 
   * Copy path.
   */
  dcs->dclevel.hPath = dc->dclevel.hPath;
  if (dcs->dclevel.hPath) dcs->dclevel.flPath |= DCPATH_SAVE;

  DC_SetNextDC (dcs, DC_GetNextDC (dc));
  DC_SetNextDC (dc, hdcs);
  ret = ++dc->dclevel.lSaveDepth;
  DC_UnlockDc( dcs );
  DC_UnlockDc( dc );

  return  ret;
}


HPALETTE 
FASTCALL 
GdiSelectPalette(HDC  hDC,
           HPALETTE  hpal,
    BOOL  ForceBackground)
{
    PDC dc;
    HPALETTE oldPal = NULL;
    PPALGDI PalGDI;

    // FIXME: mark the palette as a [fore\back]ground pal
    dc = DC_LockDc(hDC);
    if (!dc)
    {
        return NULL;
    }

    /* Check if this is a valid palette handle */
    PalGDI = PALETTE_LockPalette(hpal);
    if (!PalGDI)
    {
        DC_UnlockDc(dc);
        return NULL;
    }

    /* Is this a valid palette for this depth? */
    if ((dc->rosdc.bitsPerPixel <= 8 && PalGDI->Mode == PAL_INDEXED) ||
        (dc->rosdc.bitsPerPixel > 8  && PalGDI->Mode != PAL_INDEXED))
    {
        oldPal = dc->dclevel.hpal;
        dc->dclevel.hpal = hpal;
    }
    else if (8 < dc->rosdc.bitsPerPixel && PAL_INDEXED == PalGDI->Mode)
    {
        oldPal = dc->dclevel.hpal;
        dc->dclevel.hpal = hpal;
    }

    PALETTE_UnlockPalette(PalGDI);
    DC_UnlockDc(dc);

    return oldPal;
}

WORD APIENTRY
IntGdiSetHookFlags(HDC hDC, WORD Flags)
{
  WORD wRet;
  DC *dc = DC_LockDc(hDC);

  if (NULL == dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }

  wRet = dc->fs & DC_FLAG_DIRTY_RAO; // Fixme wrong flag!

  /* "Undocumented Windows" info is slightly confusing.
   */

  DPRINT("DC %p, Flags %04x\n", hDC, Flags);

  if (Flags & DCHF_INVALIDATEVISRGN)
    { /* hVisRgn has to be updated */
      dc->fs |= DC_FLAG_DIRTY_RAO;
    }
  else if (Flags & DCHF_VALIDATEVISRGN || 0 == Flags)
    {
      dc->fs &= ~DC_FLAG_DIRTY_RAO;
    }

  DC_UnlockDc(dc);

  return wRet;
}


BOOL
APIENTRY
NtGdiGetDCDword(
             HDC hDC,
             UINT u,
             DWORD *Result
               )
{
  BOOL Ret = TRUE;
  PDC dc;
  PDC_ATTR pdcattr;

  DWORD SafeResult = 0;
  NTSTATUS Status = STATUS_SUCCESS;

  if(!Result)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  dc = DC_LockDc(hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  pdcattr = dc->pdcattr;

  switch (u)
  {
    case GdiGetJournal:
      break;
    case GdiGetRelAbs:
      SafeResult = pdcattr->lRelAbs;
      break;
    case GdiGetBreakExtra:
      SafeResult = pdcattr->lBreakExtra;
      break;
    case GdiGerCharBreak:
      SafeResult = pdcattr->cBreak;
      break;
    case GdiGetArcDirection:
      if (pdcattr->dwLayout & LAYOUT_RTL)
          SafeResult = AD_CLOCKWISE - ((dc->dclevel.flPath & DCPATH_CLOCKWISE) != 0);
      else
          SafeResult = ((dc->dclevel.flPath & DCPATH_CLOCKWISE) != 0) + AD_COUNTERCLOCKWISE;
      break;
    case GdiGetEMFRestorDc:
      break;
    case GdiGetFontLanguageInfo:
          SafeResult = IntGetFontLanguageInfo(dc);
      break;
    case GdiGetIsMemDc:
          SafeResult = dc->dctype;
      break;
    case GdiGetMapMode:
      SafeResult = pdcattr->iMapMode;
      break;
    case GdiGetTextCharExtra:
      SafeResult = pdcattr->lTextExtra;
      break;
    default:
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      Ret = FALSE;
      break;
  }

  if (Ret)
  {
    _SEH2_TRY
    {
      ProbeForWrite(Result,
                    sizeof(DWORD),
                    1);
      *Result = SafeResult;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
      Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
  }

  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    DC_UnlockDc(dc);
    return FALSE;
  }

  DC_UnlockDc(dc);
  return Ret;
}

BOOL
APIENTRY
NtGdiGetAndSetDCDword(
                  HDC hDC,
                  UINT u,
                  DWORD dwIn,
                  DWORD *Result
                     )
{
  BOOL Ret = TRUE;
  PDC dc;
  PDC_ATTR pdcattr;

  DWORD SafeResult = 0;
  NTSTATUS Status = STATUS_SUCCESS;

  if(!Result)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  dc = DC_LockDc(hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  pdcattr = dc->pdcattr;

  switch (u)
  {
    case GdiGetSetCopyCount:
      SafeResult = dc->ulCopyCount;
      dc->ulCopyCount = dwIn;
      break;
    case GdiGetSetTextAlign:
      SafeResult = pdcattr->lTextAlign;
      pdcattr->lTextAlign = dwIn;
      // pdcattr->flTextAlign = dwIn; // Flags!
      break;
    case GdiGetSetRelAbs:
      SafeResult = pdcattr->lRelAbs;
      pdcattr->lRelAbs = dwIn;
      break;
    case GdiGetSetTextCharExtra:
      SafeResult = pdcattr->lTextExtra;
      pdcattr->lTextExtra = dwIn;
      break;
    case GdiGetSetSelectFont:
      break;
    case GdiGetSetMapperFlagsInternal:
      if (dwIn & ~1)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         Ret = FALSE;
         break;
      }
      SafeResult = pdcattr->flFontMapper;
      pdcattr->flFontMapper = dwIn;
      break;
    case GdiGetSetMapMode:
      SafeResult = IntGdiSetMapMode( dc, dwIn);
      break;
    case GdiGetSetArcDirection:
      if (dwIn != AD_COUNTERCLOCKWISE && dwIn != AD_CLOCKWISE)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         Ret = FALSE;
         break;
      }
      if ( pdcattr->dwLayout & LAYOUT_RTL ) // Right to Left
      {
         SafeResult = AD_CLOCKWISE - ((dc->dclevel.flPath & DCPATH_CLOCKWISE) != 0);
         if ( dwIn == AD_CLOCKWISE )
         {
            dc->dclevel.flPath &= ~DCPATH_CLOCKWISE;
            break;
         }
         dc->dclevel.flPath |= DCPATH_CLOCKWISE;
      }
      else // Left to Right
      {
         SafeResult = ((dc->dclevel.flPath & DCPATH_CLOCKWISE) != 0) + AD_COUNTERCLOCKWISE;
         if ( dwIn == AD_COUNTERCLOCKWISE)
         {
            dc->dclevel.flPath &= ~DCPATH_CLOCKWISE;
            break;
         }
         dc->dclevel.flPath |= DCPATH_CLOCKWISE;
      }
      break;
    default:
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      Ret = FALSE;
      break;
  }

  if (Ret)
  {
    _SEH2_TRY
    {
      ProbeForWrite(Result,
                    sizeof(DWORD),
                    1);
      *Result = SafeResult;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
      Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
  }

  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    DC_UnlockDc(dc);
    return FALSE;
  }

  DC_UnlockDc(dc);
  return Ret;
}

//  ----------------------------------------------------  Private Interface

HDC FASTCALL
DC_AllocDC(PUNICODE_STRING Driver)
{
  PDC  NewDC;
  PDC_ATTR pdcattr;
  HDC  hDC;
  PWSTR Buf = NULL;
  XFORM xformTemplate;

  if (Driver != NULL)
  {
    Buf = ExAllocatePoolWithTag(PagedPool, Driver->MaximumLength, TAG_DC);
    if(!Buf)
    {
      DPRINT1("ExAllocatePoolWithTag failed\n");
      return NULL;
    }
    RtlCopyMemory(Buf, Driver->Buffer, Driver->MaximumLength);
  }

  NewDC = (PDC)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_DC);
  if(!NewDC)
  {
    if(Buf)
    {
      ExFreePoolWithTag(Buf, TAG_DC);
    }
    DPRINT1("GDIOBJ_AllocObjWithHandle failed\n");
    return NULL;
  }

  hDC = NewDC->BaseObject.hHmgr;

  NewDC->pdcattr = &NewDC->dcattr;
  DC_AllocateDcAttr(hDC);

  if (Driver != NULL)
  {
    RtlCopyMemory(&NewDC->rosdc.DriverName, Driver, sizeof(UNICODE_STRING));
    NewDC->rosdc.DriverName.Buffer = Buf;
  }
  pdcattr = NewDC->pdcattr;

  NewDC->BaseObject.hHmgr = (HGDIOBJ) hDC; // Save the handle for this DC object.
  
  xformTemplate.eM11 = 1.0f;
  xformTemplate.eM12 = 0.0f;
  xformTemplate.eM21 = 0.0f;
  xformTemplate.eM22 = 1.0f;
  xformTemplate.eDx = 0.0f;
  xformTemplate.eDy = 0.0f;
  XForm2MatrixS(&NewDC->dclevel.mxWorldToDevice, &xformTemplate);
  XForm2MatrixS(&NewDC->dclevel.mxDeviceToWorld, &xformTemplate);
  XForm2MatrixS(&NewDC->dclevel.mxWorldToPage, &xformTemplate);

// Setup syncing bits for the dcattr data packets.
  pdcattr->flXform = DEVICE_TO_PAGE_INVALID;

  pdcattr->ulDirty_ = 0;  // Server side

  pdcattr->iMapMode = MM_TEXT;
  pdcattr->iGraphicsMode = GM_COMPATIBLE;
  pdcattr->jFillMode = ALTERNATE;

  pdcattr->szlWindowExt.cx = 1; // Float to Int,,, WRONG!
  pdcattr->szlWindowExt.cy = 1;
  pdcattr->szlViewportExt.cx = 1;
  pdcattr->szlViewportExt.cy = 1;

  pdcattr->crForegroundClr = 0;
  pdcattr->ulForegroundClr = 0;

  pdcattr->ulBackgroundClr = 0xffffff;
  pdcattr->crBackgroundClr = 0xffffff;

  pdcattr->ulPenClr = RGB( 0, 0, 0 );
  pdcattr->crPenClr = RGB( 0, 0, 0 );

  pdcattr->ulBrushClr = RGB( 255, 255, 255 ); // Do this way too.
  pdcattr->crBrushClr = RGB( 255, 255, 255 );

//// This fixes the default brush and pen settings. See DC_InitDC.
  pdcattr->hbrush = NtGdiGetStockObject( WHITE_BRUSH );
  pdcattr->hpen = NtGdiGetStockObject( BLACK_PEN );
////
  pdcattr->hlfntNew = NtGdiGetStockObject(SYSTEM_FONT);
  TextIntRealizeFont(pdcattr->hlfntNew,NULL);

  NewDC->dclevel.hpal = NtGdiGetStockObject(DEFAULT_PALETTE);
  NewDC->dclevel.laPath.eMiterLimit = 10.0;

  DC_UnlockDc(NewDC);

  return  hDC;
}

HDC FASTCALL
DC_FindOpenDC(PUNICODE_STRING  Driver)
{
  return NULL;
}

/*!
 * Initialize some common fields in the Device Context structure.
*/
VOID FASTCALL
DC_InitDC(HDC  DCHandle)
{
//  NtGdiRealizeDefaultPalette(DCHandle);

////  Removed for now.. See above brush and pen.
//  NtGdiSelectBrush(DCHandle, NtGdiGetStockObject( WHITE_BRUSH ));
//  NtGdiSelectPen(DCHandle, NtGdiGetStockObject( BLACK_PEN ));
////
  //NtGdiSelectFont(DCHandle, hFont);

/*
  {
    int res;
    res = CLIPPING_UpdateGCRegion(DCToInit);
    ASSERT ( res != ERROR );
  }
*/
}

VOID
FASTCALL
DC_AllocateDcAttr(HDC hDC)
{
  PVOID NewMem = NULL;
  PDC pDC;
  HANDLE Pid = NtCurrentProcess();
  ULONG MemSize = sizeof(DC_ATTR); //PAGE_SIZE it will allocate that size

  NTSTATUS Status = ZwAllocateVirtualMemory(Pid,
                                        &NewMem,
                                              0,
                                       &MemSize,
                         MEM_COMMIT|MEM_RESERVE,
                                 PAGE_READWRITE);
  KeEnterCriticalRegion();
  {
    INT Index = GDI_HANDLE_GET_INDEX((HGDIOBJ)hDC);
    PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
    // FIXME: dc could have been deleted!!! use GDIOBJ_InsertUserData
    if (NT_SUCCESS(Status))
    {
      RtlZeroMemory(NewMem, MemSize);
      Entry->UserData  = NewMem;
      DPRINT("DC_ATTR allocated! 0x%x\n",NewMem);
    }
    else
    {
       DPRINT("DC_ATTR not allocated!\n");
    }
  }
  KeLeaveCriticalRegion();
  pDC = DC_LockDc(hDC);
  ASSERT(pDC->pdcattr == &pDC->dcattr);
  if(NewMem)
  {
     pDC->pdcattr = NewMem; // Store pointer
  }
  DC_UnlockDc(pDC);
}

VOID
FASTCALL
DC_FreeDcAttr(HDC  DCToFree )
{
  HANDLE Pid = NtCurrentProcess();
  PDC pDC = DC_LockDc(DCToFree);
  if (pDC->pdcattr == &pDC->dcattr) return; // Internal DC object!
  pDC->pdcattr = &pDC->dcattr;
  DC_UnlockDc(pDC);

  KeEnterCriticalRegion();
  {
    INT Index = GDI_HANDLE_GET_INDEX((HGDIOBJ)DCToFree);
    PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
    if(Entry->UserData)
    {
      ULONG MemSize = sizeof(DC_ATTR); //PAGE_SIZE;
      NTSTATUS Status = ZwFreeVirtualMemory(Pid,
                               &Entry->UserData,
                                       &MemSize,
                                   MEM_RELEASE);
      if (NT_SUCCESS(Status))
      {
        DPRINT("DC_FreeDC DC_ATTR 0x%x\n", Entry->UserData);
        Entry->UserData = NULL;
      }
    }
  }
  KeLeaveCriticalRegion();
}

VOID FASTCALL
DC_FreeDC(HDC  DCToFree)
{
  DC_FreeDcAttr(DCToFree);
  if(!IsObjectDead( DCToFree ))
  {
    if (!GDIOBJ_FreeObjByHandle(DCToFree, GDI_OBJECT_TYPE_DC))
    {
       DPRINT1("DC_FreeDC failed\n");
    }
  }
  else
  {
    DPRINT1("Attempted to Delete 0x%x currently being destroyed!!!\n",DCToFree);
  }
}

BOOL INTERNAL_CALL
DC_Cleanup(PVOID ObjectBody)
{
  PDC pDC = (PDC)ObjectBody;
  if (pDC->rosdc.DriverName.Buffer)
    ExFreePoolWithTag(pDC->rosdc.DriverName.Buffer, TAG_DC);
  return TRUE;
}

HDC FASTCALL
DC_GetNextDC (PDC pDC)
{
  return pDC->hdcNext;
}

VOID FASTCALL
DC_SetNextDC (PDC pDC, HDC hNextDC)
{
  pDC->hdcNext = hNextDC;
}

VOID FASTCALL
DC_UpdateXforms(PDC  dc)
{
  XFORM  xformWnd2Vport;
  FLOAT  scaleX, scaleY;
  PDC_ATTR pdcattr = dc->pdcattr;
  XFORM xformWorld2Vport, xformWorld2Wnd, xformVport2World;

  /* Construct a transformation to do the window-to-viewport conversion */
  scaleX = (pdcattr->szlWindowExt.cx ? (FLOAT)pdcattr->szlViewportExt.cx / (FLOAT)pdcattr->szlWindowExt.cx : 0.0f);
  scaleY = (pdcattr->szlWindowExt.cy ? (FLOAT)pdcattr->szlViewportExt.cy / (FLOAT)pdcattr->szlWindowExt.cy : 0.0f);
  xformWnd2Vport.eM11 = scaleX;
  xformWnd2Vport.eM12 = 0.0;
  xformWnd2Vport.eM21 = 0.0;
  xformWnd2Vport.eM22 = scaleY;
  xformWnd2Vport.eDx  = (FLOAT)pdcattr->ptlViewportOrg.x - scaleX * (FLOAT)pdcattr->ptlWindowOrg.x;
  xformWnd2Vport.eDy  = (FLOAT)pdcattr->ptlViewportOrg.y - scaleY * (FLOAT)pdcattr->ptlWindowOrg.y;

  /* Combine with the world transformation */
  MatrixS2XForm(&xformWorld2Vport, &dc->dclevel.mxWorldToDevice);
  MatrixS2XForm(&xformWorld2Wnd, &dc->dclevel.mxWorldToPage);
  IntGdiCombineTransform(&xformWorld2Vport, &xformWorld2Wnd, &xformWnd2Vport);

  /* Create inverse of world-to-viewport transformation */
  MatrixS2XForm(&xformVport2World, &dc->dclevel.mxDeviceToWorld);
  if (DC_InvertXform(&xformWorld2Vport, &xformVport2World))
  {
      pdcattr->flXform &= ~DEVICE_TO_WORLD_INVALID;
  }
  else
  {
      pdcattr->flXform |= DEVICE_TO_WORLD_INVALID;
  }
  
  XForm2MatrixS(&dc->dclevel.mxWorldToDevice, &xformWorld2Vport);

}

BOOL FASTCALL
DC_InvertXform(const XFORM *xformSrc,
               XFORM *xformDest)
{
  FLOAT  determinant;

  determinant = xformSrc->eM11*xformSrc->eM22 - xformSrc->eM12*xformSrc->eM21;
  if (determinant > -1e-12 && determinant < 1e-12)
  {
    return  FALSE;
  }

  xformDest->eM11 =  xformSrc->eM22 / determinant;
  xformDest->eM12 = -xformSrc->eM12 / determinant;
  xformDest->eM21 = -xformSrc->eM21 / determinant;
  xformDest->eM22 =  xformSrc->eM11 / determinant;
  xformDest->eDx  = -xformSrc->eDx * xformDest->eM11 - xformSrc->eDy * xformDest->eM21;
  xformDest->eDy  = -xformSrc->eDx * xformDest->eM12 - xformSrc->eDy * xformDest->eM22;

  return  TRUE;
}

BOOL
FASTCALL
DC_SetOwnership(HDC hDC, PEPROCESS Owner)
{
    PDC pDC;

    if(!GDIOBJ_SetOwnership(hDC, Owner)) return FALSE;
    pDC = DC_LockDc(hDC);
    if (pDC)
    {
        if (pDC->rosdc.hClipRgn)
        {
            if(!GDIOBJ_SetOwnership(pDC->rosdc.hClipRgn, Owner)) return FALSE;
        }
        if (pDC->rosdc.hVisRgn)
        {
           if(!GDIOBJ_SetOwnership(pDC->rosdc.hVisRgn, Owner)) return FALSE;
        }
        if (pDC->rosdc.hGCClipRgn)
        {
            if(!GDIOBJ_SetOwnership(pDC->rosdc.hGCClipRgn, Owner)) return FALSE;
        }
        if (pDC->dclevel.hPath)
        {
           if(!GDIOBJ_SetOwnership(pDC->dclevel.hPath, Owner)) return FALSE;
        }
        DC_UnlockDc(pDC);
    }
    return TRUE;
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

BOOL FASTCALL
IntIsPrimarySurface(SURFOBJ *SurfObj)
{
   if (PrimarySurface.pSurface == NULL)
     {
       return FALSE;
     }
   return SurfObj->hsurf == PrimarySurface.pSurface;
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
  if(!hsemDriverMgmt) hsemDriverMgmt = EngCreateSemaphore(); // Hax, should be in dllmain.c
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
        PGD_ENABLEDRIVER GDEnableDriver;
        PGD_GETMODES GetModes = NULL;
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
            GetModes = (PGD_GETMODES)DrvFn->pfn;
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

   if (!IntEnumDisplaySettings(pDeviceName, ENUM_CURRENT_SETTINGS, &lpDevMode, 0))
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

DWORD
APIENTRY
NtGdiGetBoundsRect(
    IN HDC hdc,
    OUT LPRECT prc,
    IN DWORD f)
{
  DPRINT1("stub\n");
  return  DCB_RESET;   /* bounding rectangle always empty */
}

DWORD
APIENTRY
NtGdiSetBoundsRect(
    IN HDC hdc,
    IN LPRECT prc,
    IN DWORD f)
{
  DPRINT1("stub\n");
  return  DCB_DISABLE;   /* bounding rectangle always empty */
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
  } while (ppdev != NULL);
  IntGdiReleaseSemaphore(hsemDriverMgmt);
  if (!ppdev) return NULL;
  return pGdiDevice->hPDev;
}

 /*
 * @unimplemented
 */
BOOL
APIENTRY
NtGdiMakeInfoDC(
    IN HDC hdc,
    IN BOOL bSet)
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */
