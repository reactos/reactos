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
/* $Id$
 *
 * DC.C - Device context functions
 *
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* ROS Internal. Please deprecate */
NTHALAPI
BOOLEAN
NTAPI
HalQueryDisplayOwnership(
    VOID
);

#ifndef OBJ_COLORSPACE
#define OBJ_COLORSPACE	(14)
#endif

static GDIDEVICE PrimarySurface;
static KEVENT VideoDriverNeedsPreparation;
static KEVENT VideoDriverPrepared;


NTSTATUS FASTCALL
InitDcImpl(VOID)
{
  KeInitializeEvent(&VideoDriverNeedsPreparation, SynchronizationEvent, TRUE);
  KeInitializeEvent(&VideoDriverPrepared, NotificationEvent, FALSE);
  return STATUS_SUCCESS;
}

/* FIXME: DCs should probably be thread safe  */

//  ---------------------------------------------------------  File Statics

//  -----------------------------------------------------  Public Functions

BOOL STDCALL
NtGdiCancelDC(HDC  hDC)
{
  UNIMPLEMENTED;
  return FALSE;
}

HDC STDCALL
NtGdiCreateCompatibleDC(HDC hDC)
{
  PDC  NewDC, OrigDC;
  PDC_ATTR nDc_Attr, oDc_Attr;
  HBITMAP  hBitmap;
  HDC hNewDC, DisplayDC;
  HRGN hVisRgn;
  UNICODE_STRING DriverName;
  INT DC_Type = DC_TYPE_DIRECT;

  DisplayDC = NULL;
  if (hDC == NULL)
    {
      RtlInitUnicodeString(&DriverName, L"DISPLAY");
      DisplayDC = IntGdiCreateDC(&DriverName, NULL, NULL, NULL, TRUE);
      if (NULL == DisplayDC)
        {
          return NULL;
        }
      hDC = DisplayDC;
      DC_Type = DC_TYPE_MEMORY; // Null hDC == Memory DC.
    }

  /*  Allocate a new DC based on the original DC's device  */
  OrigDC = DC_LockDc(hDC);
  if (NULL == OrigDC)
    {
      if (NULL != DisplayDC)
        {
          NtGdiDeleteObjectApp(DisplayDC);
        }
      return NULL;
    }
  hNewDC = DC_AllocDC(&OrigDC->DriverName);
  if (NULL == hNewDC)
    {
      DC_UnlockDc(OrigDC);
      if (NULL != DisplayDC)
        {
          NtGdiDeleteObjectApp(DisplayDC);
        }
      return  NULL;
    }
  NewDC = DC_LockDc( hNewDC );

  oDc_Attr = OrigDC->pDc_Attr;
  if(!oDc_Attr) oDc_Attr = &OrigDC->Dc_Attr;
  nDc_Attr = NewDC->pDc_Attr;
  if(!nDc_Attr) nDc_Attr = &NewDC->Dc_Attr;

  /* Copy information from original DC to new DC  */
  NewDC->hSelf = hNewDC;
  NewDC->IsIC = FALSE;
  NewDC->DC_Type = DC_Type;

  NewDC->PDev = OrigDC->PDev;

  NewDC->w.bitsPerPixel = OrigDC->w.bitsPerPixel;

  /* DriverName is copied in the AllocDC routine  */
  nDc_Attr->ptlWindowOrg   = oDc_Attr->ptlWindowOrg;
  nDc_Attr->szlWindowExt   = oDc_Attr->szlWindowExt;
  nDc_Attr->ptlViewportOrg = oDc_Attr->ptlViewportOrg;
  nDc_Attr->szlViewportExt = oDc_Attr->szlViewportExt;

  /* Create default bitmap */
  if (!(hBitmap = IntGdiCreateBitmap( 1, 1, 1, NewDC->w.bitsPerPixel, NULL )))
    {
      DC_UnlockDc( OrigDC );
      DC_UnlockDc( NewDC );
      DC_FreeDC( hNewDC );
      if (NULL != DisplayDC)
        {
          NtGdiDeleteObjectApp(DisplayDC);
        }
      return NULL;
    }
  NewDC->w.flags        = DC_MEMORY;
  NewDC->w.hBitmap      = hBitmap;
  NewDC->w.hFirstBitmap = hBitmap;
  NewDC->pPDev          = OrigDC->pPDev;

  NewDC->PalIndexed = OrigDC->PalIndexed;
  NewDC->w.hPalette = OrigDC->w.hPalette;
  nDc_Attr->lTextAlign      = oDc_Attr->lTextAlign;
  nDc_Attr->ulForegroundClr = oDc_Attr->ulForegroundClr;
  nDc_Attr->ulBackgroundClr = oDc_Attr->ulBackgroundClr;
  nDc_Attr->lBkMode         = oDc_Attr->lBkMode;
  nDc_Attr->crForegroundClr = oDc_Attr->crForegroundClr;
  nDc_Attr->crBackgroundClr = oDc_Attr->crBackgroundClr;
  nDc_Attr->jBkMode         = oDc_Attr->jBkMode;
  nDc_Attr->jROP2           = oDc_Attr->jROP2;

  DC_UnlockDc(NewDC);
  DC_UnlockDc(OrigDC);
  if (NULL != DisplayDC)
    {
      NtGdiDeleteObjectApp(DisplayDC);
    }

  hVisRgn = NtGdiCreateRectRgn(0, 0, 1, 1);
  IntGdiSelectVisRgn(hNewDC, hVisRgn);
  NtGdiDeleteObject(hVisRgn);

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
  RtlFreeUnicodeString(&RegistryPath);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("No InstalledDisplayDrivers value in service entry found\n");
      return FALSE;
    }

  DPRINT("DriverFileNames %S\n", DriverFileNames->Buffer);

  return TRUE;
}

static NTSTATUS STDCALL
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

  RtlFreeUnicodeString(&RegistryPath);

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

   for (DisplayNumber = 0; ; DisplayNumber++)
   {
      DPRINT("Trying to load display driver no. %d\n", DisplayNumber);

      RtlZeroMemory(&PrimarySurface, sizeof(PrimarySurface));

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
         RtlFreeUnicodeString(&DriverFileNames);
         DPRINT1("No suitable DDI driver found\n");
         continue;
      }

      DPRINT("Display driver %S loaded\n", CurrentName);

      RtlFreeUnicodeString(&DriverFileNames);

      DPRINT("Building DDI Functions\n");

      /* Construct DDI driver function dispatch table */
      if (!DRIVER_BuildDDIFunctions(&DED, &PrimarySurface.DriverFunctions))
      {
         ObDereferenceObject(PrimarySurface.VideoFileObject);
         DPRINT1("BuildDDIFunctions failed\n");
         goto cleanup;
      }

      /* Allocate a phyical device handle from the driver */
      PrimarySurface.DMW.dmSize = sizeof (PrimarySurface.DMW);
      if (SetupDevMode(&PrimarySurface.DMW, DisplayNumber))
      {
         PrimarySurface.PDev = PrimarySurface.DriverFunctions.EnablePDEV(
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
         DoDefault = (NULL == PrimarySurface.PDev);
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
         PrimarySurface.PDev = PrimarySurface.DriverFunctions.EnablePDEV(
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

         if (NULL == PrimarySurface.PDev)
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
         PrimarySurface.PDev,
         (HDEV)&PrimarySurface);

      DPRINT("calling DRIVER_ReferenceDriver\n");

      DRIVER_ReferenceDriver(L"DISPLAY");

      PrimarySurface.PreparedDriver = TRUE;
      PrimarySurface.DisplayNumber = DisplayNumber;
      PrimarySurface.flFlags = PDEV_DISPLAY; // Hard set,, add more flags.
      PrimarySurface.hsemDevLock = (PERESOURCE)EngCreateSemaphore();

      ret = TRUE;
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
   PrimarySurface.Handle =
      PrimarySurface.DriverFunctions.EnableSurface(PrimarySurface.PDev);
   if (NULL == PrimarySurface.Handle)
   {
/*      PrimarySurface.DriverFunctions.AssertMode(PrimarySurface.PDev, FALSE);*/
      PrimarySurface.DriverFunctions.DisablePDEV(PrimarySurface.PDev);
      ObDereferenceObject(PrimarySurface.VideoFileObject);
      DPRINT1("DrvEnableSurface failed\n");
      return FALSE;
   }

   PrimarySurface.DriverFunctions.AssertMode(PrimarySurface.PDev, TRUE);

   calledFromUser = UserIsEntered(); //fixme: possibly upgrade a shared lock
   if (!calledFromUser){
      UserEnterExclusive();
   }

   /* attach monitor */
   IntAttachMonitor(&PrimarySurface, PrimarySurface.DisplayNumber);

   SurfObj = EngLockSurface((HSURF)PrimarySurface.Handle);
   SurfObj->dhpdev = PrimarySurface.PDev;
   SurfSize = SurfObj->sizlBitmap;
   SurfaceRect.left = SurfaceRect.top = 0;
   SurfaceRect.right = SurfObj->sizlBitmap.cx;
   SurfaceRect.bottom = SurfObj->sizlBitmap.cy;
   /* FIXME - why does EngEraseSurface() sometimes crash?
     EngEraseSurface(SurfObj, &SurfaceRect, 0); */

   /* Put the pointer in the center of the screen */
   GDIDEV(SurfObj)->Pointer.Pos.x = (SurfaceRect.right - SurfaceRect.left) / 2;
   GDIDEV(SurfObj)->Pointer.Pos.y = (SurfaceRect.bottom - SurfaceRect.top) / 2;

   EngUnlockSurface(SurfObj);
   co_IntShowDesktop(IntGetActiveDesktop(), SurfSize.cx, SurfSize.cy);

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
    PrimarySurface.DriverFunctions.AssertMode(PrimarySurface.PDev, FALSE);
    PrimarySurface.DriverFunctions.DisableSurface(PrimarySurface.PDev);
    PrimarySurface.DriverFunctions.DisablePDEV(PrimarySurface.PDev);
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
  HDC      hNewDC;
  PDC      NewDC;
  PDC_ATTR nDc_Attr;
  HDC      hDC = NULL;
  HRGN     hVisRgn;
  UNICODE_STRING StdDriver;
  BOOL calledFromUser;

  RtlInitUnicodeString(&StdDriver, L"DISPLAY");

  if (Driver != NULL)
  {
    DPRINT("NAME Driver: %wZ\n", Driver);
  }
  else
  {
    DPRINT("NAME Driver: NULL\n", Driver);
  }


  if (Driver != NULL)
  {
    DPRINT("NAME Device: %wZ\n", Device);
  }
  else
  {
    DPRINT("NAME Device: NULL\n", Device);
  }


  if (NULL == Driver || 0 == RtlCompareUnicodeString(Driver, &StdDriver, TRUE))
    {
      if (CreateAsIC)
        {
          if (! IntPrepareDriverIfNeeded())
            {
              DPRINT1("Unable to prepare graphics driver, returning NULL ic\n");
              return NULL;
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
  if ((hNewDC = DC_FindOpenDC(Driver)) != NULL)
  {
    hDC = hNewDC;
    return  NtGdiCreateCompatibleDC(hDC);
  }

  if (Driver != NULL && Driver->Buffer != NULL)
  {
    if (Driver!=NULL)
        DPRINT("NAME: %wZ\n", Driver);
  }

  /*  Allocate a DC object  */
  if ((hNewDC = DC_AllocDC(Driver)) == NULL)
  {
    return  NULL;
  }

  NewDC = DC_LockDc( hNewDC );
  /* FIXME - NewDC can be NULL!!! Don't assert here! */
  if ( !NewDC )
  {
    DC_FreeDC( hNewDC );
    return NULL;
  }

  nDc_Attr = NewDC->pDc_Attr;
  if(!nDc_Attr) nDc_Attr = &NewDC->Dc_Attr;

  NewDC->DC_Type = DC_TYPE_DIRECT;
  NewDC->IsIC = CreateAsIC;

  NewDC->PDev = PrimarySurface.PDev;
  if(pUMdhpdev) pUMdhpdev = NewDC->PDev;
  NewDC->pPDev = (PVOID)&PrimarySurface;
  NewDC->w.hBitmap = PrimarySurface.Handle;

  NewDC->w.bitsPerPixel = ((PGDIDEVICE)NewDC->pPDev)->GDIInfo.cBitsPixel * 
                                     ((PGDIDEVICE)NewDC->pPDev)->GDIInfo.cPlanes;
  DPRINT("Bits per pel: %u\n", NewDC->w.bitsPerPixel);

  NewDC->flGraphics  = PrimarySurface.DevInfo.flGraphicsCaps;
  NewDC->flGraphics2 = PrimarySurface.DevInfo.flGraphicsCaps2;

  if (!CreateAsIC)
  {
    NewDC->PalIndexed = NtGdiGetStockObject(DEFAULT_PALETTE);
    NewDC->w.hPalette = PrimarySurface.DevInfo.hpalDefault;
    nDc_Attr->jROP2 = R2_COPYPEN;

    NewDC->erclWindow.top = NewDC->erclWindow.left = 0;
    NewDC->erclWindow.right  = ((PGDIDEVICE)NewDC->pPDev)->GDIInfo.ulHorzRes;
    NewDC->erclWindow.bottom = ((PGDIDEVICE)NewDC->pPDev)->GDIInfo.ulVertRes;

    DC_UnlockDc( NewDC );

    hVisRgn = NtGdiCreateRectRgn(0, 0, ((PGDIDEVICE)NewDC->pPDev)->GDIInfo.ulHorzRes,
                                 ((PGDIDEVICE)NewDC->pPDev)->GDIInfo.ulVertRes);
    IntGdiSelectVisRgn(hNewDC, hVisRgn);
    NtGdiDeleteObject(hVisRgn);

    /*  Initialize the DC state  */
    DC_InitDC(hNewDC);
    NtGdiSetTextColor(hNewDC, RGB(0, 0, 0));
    NtGdiSetTextAlign(hNewDC, TA_TOP);
    NtGdiSetBkColor(hNewDC, RGB(255, 255, 255));
    NtGdiSetBkMode(hNewDC, OPAQUE);
  }
  else
  {
    /* From MSDN2:
       The CreateIC function creates an information context for the specified device.
       The information context provides a fast way to get information about the
       device without creating a device context (DC). However, GDI drawing functions
       cannot accept a handle to an information context.
     */
    NewDC->DC_Type = DC_TYPE_INFO;
    DC_UnlockDc( NewDC );
  }
  return hNewDC;
}

HDC STDCALL
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
    _SEH_TRY
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
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
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

  Ret = IntGdiCreateDC(NULL == Device ? NULL : &SafeDevice,
                       NULL,
                       NULL == pUMdhpdev ? NULL : &Dhpdev,
                       NULL == InitData ? NULL : &SafeInitData,
                       (BOOL) iType); // FALSE 0 DCW, TRUE 1 ICW

  if (pUMdhpdev) pUMdhpdev = Dhpdev;

  return Ret;

}

BOOL
STDCALL
NtGdiDeleteObjectApp(HANDLE  DCHandle)
{
  PDC  DCToDelete;

  if (GDI_HANDLE_GET_TYPE(DCHandle) != GDI_OBJECT_TYPE_DC)
     return NtGdiDeleteObject((HGDIOBJ) DCHandle);

  if(IsObjectDead((HGDIOBJ)DCHandle)) return TRUE;

  if (!GDIOBJ_OwnedByCurrentProcess(GdiHandleTable, DCHandle))
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  DCToDelete = DC_LockDc(DCHandle);
  if (DCToDelete == NULL)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  /*  First delete all saved DCs  */
  while (DCToDelete->saveLevel)
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
    DCToDelete->saveLevel--;
    DC_UnlockDc( savedDC );
    NtGdiDeleteObjectApp (savedHDC);
  }

  /*  Free GDI resources allocated to this DC  */
  if (!(DCToDelete->w.flags & DC_SAVED))
  {
    /*
    NtGdiSelectPen (DCHandle, STOCK_BLACK_PEN);
    NtGdiSelectBrush (DCHandle, STOCK_WHITE_BRUSH);
    NtGdiSelectFont (DCHandle, STOCK_SYSTEM_FONT);
    DC_LockDC (DCHandle); NtGdiSelectXxx does not recognize stock objects yet  */
    if (DCToDelete->w.flags & DC_MEMORY)
    {
      NtGdiDeleteObject (DCToDelete->w.hFirstBitmap);
    }
    if (DCToDelete->XlateBrush != NULL)
      EngDeleteXlate(DCToDelete->XlateBrush);
    if (DCToDelete->XlatePen != NULL)
      EngDeleteXlate(DCToDelete->XlatePen);
  }
  if (DCToDelete->w.hClipRgn)
  {
    NtGdiDeleteObject (DCToDelete->w.hClipRgn);
  }
  if (DCToDelete->w.hVisRgn)
  {
    NtGdiDeleteObject (DCToDelete->w.hVisRgn);
  }
  if (NULL != DCToDelete->CombinedClip)
    {
      IntEngDeleteClipRegion(DCToDelete->CombinedClip);
    }
  if (DCToDelete->w.hGCClipRgn)
  {
    NtGdiDeleteObject (DCToDelete->w.hGCClipRgn);
  }
#if 0 /* FIXME */
  PATH_DestroyGdiPath (&DCToDelete->w.path);
#endif

  if (DCToDelete->emh)
  {
	 EngFreeMem(DCToDelete->emh);
  }

  DC_UnlockDc( DCToDelete );
  DC_FreeDC ( DCHandle );
  return TRUE;
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
STDCALL
NtGdiGetDCObject(HDC  hDC, INT  ObjectType)
{
  HGDIOBJ SelObject;
  DC *dc;
  PDC_ATTR Dc_Attr;

  /* From Wine: GetCurrentObject does not SetLastError() on a null object */
  if(!hDC) return NULL;

  if(!(dc = DC_LockDc(hDC)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return NULL;
  }
  Dc_Attr = dc->pDc_Attr;
  if (!Dc_Attr) Dc_Attr = &dc->Dc_Attr;
  switch(ObjectType)
  {
    case GDI_OBJECT_TYPE_EXTPEN:
    case GDI_OBJECT_TYPE_PEN:
      SelObject = Dc_Attr->hpen;
      break;
    case GDI_OBJECT_TYPE_BRUSH:
      SelObject = Dc_Attr->hbrush;
      break;
    case GDI_OBJECT_TYPE_PALETTE:
      SelObject = dc->w.hPalette;
      break;
    case GDI_OBJECT_TYPE_FONT:
      SelObject = Dc_Attr->hlfntNew;
      break;
    case GDI_OBJECT_TYPE_BITMAP:
      SelObject = dc->w.hBitmap;
      break;
    case GDI_OBJECT_TYPE_COLORSPACE:
      DPRINT1("FIXME: NtGdiGetCurrentObject() ObjectType OBJ_COLORSPACE not supported yet!\n");
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

BOOL FASTCALL
IntGdiGetDCOrgEx(DC *dc, LPPOINT  Point)
{
  Point->x = dc->w.DCOrgX;
  Point->y = dc->w.DCOrgY;

  return  TRUE;
}

BOOL STDCALL
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
      Ret = IntGdiGetDCOrgEx(dc, &SafePoint);
      break;
    case GdiGetAspectRatioFilter:
    default:
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      Ret = FALSE;
      break;
  }

  if (Ret)
  {
    _SEH_TRY
    {
      ProbeForWrite(Point,
                    sizeof(POINT),
                    1);
      *Point = SafePoint;
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
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
  PDC_ATTR Dc_Attr, nDc_Attr;

  Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;
  nDc_Attr = newdc->pDc_Attr;
  if(!nDc_Attr) nDc_Attr = &newdc->Dc_Attr;

  newdc->w.flags            = dc->w.flags | DC_SAVED;
  nDc_Attr->dwLayout        = Dc_Attr->dwLayout;
  nDc_Attr->hpen            = Dc_Attr->hpen;
  nDc_Attr->hbrush          = Dc_Attr->hbrush;
  nDc_Attr->hlfntNew        = Dc_Attr->hlfntNew;
  newdc->w.hBitmap          = dc->w.hBitmap;
  newdc->w.hFirstBitmap     = dc->w.hFirstBitmap;
#if 0
  newdc->w.hDevice          = dc->w.hDevice;
#endif
  newdc->PalIndexed         = dc->PalIndexed;
  newdc->w.hPalette         = dc->w.hPalette;
  newdc->w.totalExtent      = dc->w.totalExtent;
  newdc->w.bitsPerPixel     = dc->w.bitsPerPixel;
  nDc_Attr->jROP2           = Dc_Attr->jROP2;
  nDc_Attr->jFillMode       = Dc_Attr->jFillMode;
  nDc_Attr->jStretchBltMode = Dc_Attr->jStretchBltMode;
  nDc_Attr->lRelAbs         = Dc_Attr->lRelAbs;
  nDc_Attr->jBkMode         = Dc_Attr->jBkMode;
  nDc_Attr->lBkMode         = Dc_Attr->lBkMode;
  nDc_Attr->crBackgroundClr = Dc_Attr->crBackgroundClr;
  nDc_Attr->crForegroundClr = Dc_Attr->crForegroundClr;
  nDc_Attr->ulBackgroundClr = Dc_Attr->ulBackgroundClr;
  nDc_Attr->ulForegroundClr = Dc_Attr->ulForegroundClr;
  nDc_Attr->ptlBrushOrigin  = Dc_Attr->ptlBrushOrigin;
  nDc_Attr->lTextAlign      = Dc_Attr->lTextAlign;
  nDc_Attr->lTextExtra      = Dc_Attr->lTextExtra;
  nDc_Attr->cBreak          = Dc_Attr->cBreak;
  nDc_Attr->lBreakExtra     = Dc_Attr->lBreakExtra;
  nDc_Attr->iMapMode        = Dc_Attr->iMapMode;
  nDc_Attr->iGraphicsMode   = Dc_Attr->iGraphicsMode;
#if 0
  /* Apparently, the DC origin is not changed by [GS]etDCState */
  newdc->w.DCOrgX           = dc->w.DCOrgX;
  newdc->w.DCOrgY           = dc->w.DCOrgY;
#endif
  nDc_Attr->ptlCurrent      = Dc_Attr->ptlCurrent;
  nDc_Attr->ptfxCurrent     = Dc_Attr->ptfxCurrent;
  newdc->w.ArcDirection     = dc->w.ArcDirection;
  newdc->w.xformWorld2Wnd   = dc->w.xformWorld2Wnd;
  newdc->w.xformWorld2Vport = dc->w.xformWorld2Vport;
  newdc->w.xformVport2World = dc->w.xformVport2World;
  newdc->w.vport2WorldValid = dc->w.vport2WorldValid;
  nDc_Attr->ptlWindowOrg    = Dc_Attr->ptlWindowOrg;
  nDc_Attr->szlWindowExt    = Dc_Attr->szlWindowExt;
  nDc_Attr->ptlViewportOrg  = Dc_Attr->ptlViewportOrg;
  nDc_Attr->szlViewportExt  = Dc_Attr->szlViewportExt;

  newdc->saveLevel = 0;
  newdc->IsIC = dc->IsIC;

#if 0
  PATH_InitGdiPath( &newdc->w.path );
#endif

  /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */

  newdc->w.hGCClipRgn = newdc->w.hVisRgn = 0;
  if (dc->w.hClipRgn)
  {
    newdc->w.hClipRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
    NtGdiCombineRgn( newdc->w.hClipRgn, dc->w.hClipRgn, 0, RGN_COPY );
  }
}


VOID
FASTCALL
IntGdiCopyFromSaveState(PDC dc, PDC dcs, HDC hDC)
{
  PDC_ATTR Dc_Attr, sDc_Attr;

  Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;
  sDc_Attr = dcs->pDc_Attr;
  if(!sDc_Attr) sDc_Attr = &dcs->Dc_Attr;
  
  dc->w.flags              = dcs->w.flags & ~DC_SAVED;

  dc->w.hFirstBitmap       = dcs->w.hFirstBitmap;

#if 0
  dc->w.hDevice            = dcs->w.hDevice;
#endif

  Dc_Attr->dwLayout        = sDc_Attr->dwLayout;
  dc->w.totalExtent        = dcs->w.totalExtent;
  Dc_Attr->jROP2           = sDc_Attr->jROP2;
  Dc_Attr->jFillMode       = sDc_Attr->jFillMode;
  Dc_Attr->jStretchBltMode = sDc_Attr->jStretchBltMode;
  Dc_Attr->lRelAbs         = sDc_Attr->lRelAbs;
  Dc_Attr->jBkMode         = sDc_Attr->jBkMode;
  Dc_Attr->crBackgroundClr = sDc_Attr->crBackgroundClr;
  Dc_Attr->crForegroundClr = sDc_Attr->crForegroundClr;
  Dc_Attr->lBkMode         = sDc_Attr->lBkMode;
  Dc_Attr->ulBackgroundClr = sDc_Attr->ulBackgroundClr;
  Dc_Attr->ulForegroundClr = sDc_Attr->ulForegroundClr;
  Dc_Attr->ptlBrushOrigin  = sDc_Attr->ptlBrushOrigin;

  Dc_Attr->lTextAlign      = sDc_Attr->lTextAlign;
  Dc_Attr->lTextExtra      = sDc_Attr->lTextExtra;
  Dc_Attr->cBreak          = sDc_Attr->cBreak;
  Dc_Attr->lBreakExtra     = sDc_Attr->lBreakExtra;
  Dc_Attr->iMapMode        = sDc_Attr->iMapMode;
  Dc_Attr->iGraphicsMode   = sDc_Attr->iGraphicsMode;
#if 0
/* Apparently, the DC origin is not changed by [GS]etDCState */
  dc->w.DCOrgX             = dcs->w.DCOrgX;
  dc->w.DCOrgY             = dcs->w.DCOrgY;
#endif
  Dc_Attr->ptlCurrent      = sDc_Attr->ptlCurrent;
  Dc_Attr->ptfxCurrent     = sDc_Attr->ptfxCurrent;
  dc->w.ArcDirection       = dcs->w.ArcDirection;
  dc->w.xformWorld2Wnd     = dcs->w.xformWorld2Wnd;
  dc->w.xformWorld2Vport   = dcs->w.xformWorld2Vport;
  dc->w.xformVport2World   = dcs->w.xformVport2World;
  dc->w.vport2WorldValid   = dcs->w.vport2WorldValid;
  Dc_Attr->ptlWindowOrg    = sDc_Attr->ptlWindowOrg;
  Dc_Attr->szlWindowExt    = sDc_Attr->szlWindowExt;
  Dc_Attr->ptlViewportOrg  = sDc_Attr->ptlViewportOrg;
  Dc_Attr->szlViewportExt  = sDc_Attr->szlViewportExt;
  dc->PalIndexed           = dcs->PalIndexed;

  if (!(dc->w.flags & DC_MEMORY))
  {
     dc->w.bitsPerPixel = dcs->w.bitsPerPixel;
  }

#if 0
  if (dcs->w.hClipRgn)
  {
    if (!dc->w.hClipRgn)
    {
       dc->w.hClipRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
    }
    NtGdiCombineRgn( dc->w.hClipRgn, dcs->w.hClipRgn, 0, RGN_COPY );
  }
  else
  {
    if (dc->w.hClipRgn)
    {
       NtGdiDeleteObject( dc->w.hClipRgn );
    }
    dc->w.hClipRgn = 0;
  }
  {
    int res;
    res = CLIPPING_UpdateGCRegion( dc );
    ASSERT ( res != ERROR );
  }
  DC_UnlockDc ( dc );
#else
  IntGdiExtSelectClipRgn(dc, dcs->w.hClipRgn, RGN_COPY);
  DC_UnlockDc ( dc );
#endif
  if(!hDC) return; // Not a MemoryDC or SaveLevel DC, return.

  NtGdiSelectBitmap( hDC, dcs->w.hBitmap );
  NtGdiSelectBrush( hDC, sDc_Attr->hbrush );
  NtGdiSelectFont( hDC, sDc_Attr->hlfntNew );
  NtGdiSelectPen( hDC, sDc_Attr->hpen );

  NtGdiSetBkColor( hDC, sDc_Attr->crBackgroundClr);
  NtGdiSetTextColor( hDC, sDc_Attr->crForegroundClr);

  NtUserSelectPalette( hDC, dcs->w.hPalette, FALSE );

#if 0
  GDISelectPalette16( hDC, dcs->w.hPalette, FALSE );
#endif
}

HDC STDCALL
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

  newdc->hSelf = hnewdc;
  IntGdiCopyToSaveState( dc, newdc);

  DC_UnlockDc( newdc );
  DC_UnlockDc( dc );
  return  hnewdc;
}


VOID
STDCALL
IntGdiSetDCState ( HDC hDC, HDC hDCSave )
{
  PDC  dc, dcs;

  dc = DC_LockDc ( hDC );
  if ( dc )
  {
    dcs = DC_LockDc ( hDCSave );
    if ( dcs )
    {
      if ( dcs->w.flags & DC_SAVED )
      {
        IntGdiCopyFromSaveState( dc, dcs, dc->hSelf);
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

INT FASTCALL
IntGdiGetDeviceCaps(PDC dc, INT Index)
{
  INT ret = 0;
  POINT  pt;

  /* Retrieve capability */
  switch (Index)
  {
    case DRIVERVERSION:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVersion;
      break;

    case TECHNOLOGY:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulTechnology;
      break;

    case HORZSIZE:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulHorzSize;
      break;

    case VERTSIZE:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVertSize;
      break;

    case HORZRES:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulHorzRes;
      break;

    case VERTRES:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulVertRes;
      break;

    case LOGPIXELSX:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulLogPixelsX;
      break;

    case LOGPIXELSY:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulLogPixelsY;
      break;

    case BITSPIXEL:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.cBitsPixel;
      break;

    case PLANES:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.cPlanes;
      break;

    case NUMBRUSHES:
      UNIMPLEMENTED; /* FIXME */
      break;

    case NUMPENS:
      UNIMPLEMENTED; /* FIXME */
      break;

    case NUMFONTS:
      UNIMPLEMENTED; /* FIXME */
      break;

    case NUMCOLORS:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulNumColors;
      break;

    case ASPECTX:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulAspectX;
      break;

    case ASPECTY:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulAspectY;
      break;

    case ASPECTXY:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulAspectXY;
      break;

    case PDEVICESIZE:
      UNIMPLEMENTED; /* FIXME */
      break;

    case CLIPCAPS:
      UNIMPLEMENTED; /* FIXME */
      break;

    case SIZEPALETTE:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.ulNumPalReg; /* FIXME not sure */
      break;

    case NUMRESERVED:
      ret = 0;
      break;

    case COLORRES:
      UNIMPLEMENTED; /* FIXME */
      break;

    case PHYSICALWIDTH:
      if(IntGdiEscape(dc, GETPHYSPAGESIZE, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.x;
      }
      else
      {
        ret = 0;
      }
      break;

    case PHYSICALHEIGHT:
      if(IntGdiEscape(dc, GETPHYSPAGESIZE, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.y;
      }
      else
      {
        ret = 0;
      }
      break;

    case PHYSICALOFFSETX:
      if(IntGdiEscape(dc, GETPRINTINGOFFSET, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.x;
      }
      else
      {
        ret = 0;
      }
      break;

    case PHYSICALOFFSETY:
      if(IntGdiEscape(dc, GETPRINTINGOFFSET, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.y;
      }
      else
      {
        ret = 0;
      }
      break;

    case VREFRESH:
      UNIMPLEMENTED; /* FIXME */
      break;

    case SCALINGFACTORX:
      if(IntGdiEscape(dc, GETSCALINGFACTOR, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.x;
      }
      else
      {
        ret = 0;
      }
      break;

    case SCALINGFACTORY:
      if(IntGdiEscape(dc, GETSCALINGFACTOR, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.y;
      }
      else
      {
        ret = 0;
      }
      break;

    case RASTERCAPS:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.flRaster;
      break;

    case CURVECAPS:
      UNIMPLEMENTED; /* FIXME */
      break;

    case LINECAPS:
      UNIMPLEMENTED; /* FIXME */
      break;

    case POLYGONALCAPS:
      UNIMPLEMENTED; /* FIXME */
      break;

    case TEXTCAPS:
      ret = ((PGDIDEVICE)dc->pPDev)->GDIInfo.flTextCaps;
      break;

    default:
      ret = 0;
      break;
  }

  return ret;
}

INT STDCALL
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

INT
FASTCALL
IntGdiGetObject(IN HANDLE Handle,
                IN INT cbCount,
                IN LPVOID lpBuffer)
{
  PVOID pGdiObject;
  INT Result = 0;
  DWORD dwObjectType;

  pGdiObject = GDIOBJ_LockObj(GdiHandleTable, Handle, GDI_OBJECT_TYPE_DONTCARE);
  if (!pGdiObject)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }

  dwObjectType = GDIOBJ_GetObjectType(Handle);
  switch (dwObjectType)
    {
      case GDI_OBJECT_TYPE_PEN:
      case GDI_OBJECT_TYPE_EXTPEN:
        Result = PEN_GetObject((PGDIBRUSHOBJ) pGdiObject, cbCount, (PLOGPEN) lpBuffer); // IntGdiCreatePenIndirect
        break;

      case GDI_OBJECT_TYPE_BRUSH:
        Result = BRUSH_GetObject((PGDIBRUSHOBJ ) pGdiObject, cbCount, (LPLOGBRUSH)lpBuffer);
        break;

      case GDI_OBJECT_TYPE_BITMAP:
        Result = BITMAP_GetObject((BITMAPOBJ *) pGdiObject, cbCount, lpBuffer);
        break;
      case GDI_OBJECT_TYPE_FONT:
        Result = FontGetObject((PTEXTOBJ) pGdiObject, cbCount, lpBuffer);
#if 0
        // Fix the LOGFONT structure for the stock fonts
        if (FIRST_STOCK_HANDLE <= Handle && Handle <= LAST_STOCK_HANDLE)
          {
            FixStockFontSizeW(Handle, cbCount, lpBuffer);
          }
#endif
        break;

      case GDI_OBJECT_TYPE_PALETTE:
        Result = PALETTE_GetObject((PPALGDI) pGdiObject, cbCount, lpBuffer);
        break;

      default:
        DPRINT1("GDI object type 0x%08x not implemented\n", dwObjectType);
        break;
    }

  GDIOBJ_UnlockObjByPtr(GdiHandleTable, pGdiObject);

  return Result;
}

INT
NTAPI
NtGdiExtGetObjectW(IN HANDLE hGdiObj,
                   IN INT cbCount,
                   OUT LPVOID lpBuffer)
{
    INT iRetCount = 0;
    INT cbCopyCount;
    union
    {
        BITMAP bitmap;
        DIBSECTION dibsection;
        LOGPEN logpen;
        LOGBRUSH logbrush;
        LOGFONTW logfontw;
        EXTLOGFONTW extlogfontw;
        ENUMLOGFONTEXDVW enumlogfontexdvw;
    } Object;

    // Normalize to the largest supported object size
    cbCount = min((UINT)cbCount, sizeof(Object));

    // Now do the actual call
    iRetCount = IntGdiGetObject(hGdiObj, cbCount, lpBuffer ? &Object : NULL);
    cbCopyCount = min((UINT)cbCount, (UINT)iRetCount);

    // Make sure we have a buffer and a copy size
    if ((cbCopyCount) && (lpBuffer))
    {
        // Enter SEH for buffer transfer
        _SEH_TRY
        {
            // Probe the buffer and copy it
            ProbeForWrite(lpBuffer, cbCopyCount, sizeof(WORD));
            RtlCopyMemory(lpBuffer, &Object, cbCopyCount);
        }
        _SEH_HANDLE
        {
            // Clear the return value.
            // Do *NOT* set last error here!
            iRetCount = 0;
        }
        _SEH_END;
    }
    // Return the count
    return iRetCount;
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

BOOL STDCALL
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
      SaveLevel = dc->saveLevel + SaveLevel + 1;

  if(SaveLevel < 0 || dc->saveLevel<SaveLevel)
  {
    DC_UnlockDc(dc);
    return FALSE;
  }

  success=TRUE;
  while (dc->saveLevel >= SaveLevel)
  {
     HDC hdcs = DC_GetNextDC (dc);

     dcs = DC_LockDc (hdcs);
     if (dcs == NULL)
     {
        DC_UnlockDc(dc);
        return FALSE;
     }

     DC_SetNextDC (dc, DC_GetNextDC (dcs));
     dcs->hNext = 0;

     if (--dc->saveLevel < SaveLevel)
     {
         DC_UnlockDc( dc );
         DC_UnlockDc( dcs );

         IntGdiSetDCState(hDC, hdcs);

        if (!PATH_AssignGdiPath( &dc->w.path, &dcs->w.path ))
        {
          /* FIXME: This might not be quite right, since we're
           * returning FALSE but still destroying the saved DC state */
          success = FALSE;
        }
         dc = DC_LockDc(hDC);
         if(!dc)
         {
            return FALSE;
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


INT STDCALL
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

#if 0
    /* Copy path. The reason why path saving / restoring is in SaveDC/
     * RestoreDC and not in GetDCState/SetDCState is that the ...DCState
     * functions are only in Win16 (which doesn't have paths) and that
     * SetDCState doesn't allow us to signal an error (which can happen
     * when copying paths).
     */
  if (!PATH_AssignGdiPath (&dcs->w.path, &dc->w.path))
  {
    NtGdiDeleteObjectApp (hdcs);

    return 0;
  }
#endif

  DC_SetNextDC (dcs, DC_GetNextDC (dc));
  DC_SetNextDC (dc, hdcs);
  ret = ++dc->saveLevel;
  DC_UnlockDc( dcs );
  DC_UnlockDc( dc );

  return  ret;
}

 /*
 * @implemented
 */
HBITMAP
APIENTRY
NtGdiSelectBitmap(
    IN HDC hDC,
    IN HBITMAP hBmp)
{
    PDC pDC;
    PDC_ATTR pDc_Attr;
    HBITMAP hOrgBmp;
    PBITMAPOBJ pBmp;
    HRGN hVisRgn;
    BOOLEAN bFailed;
    PGDIBRUSHOBJ pBrush;

    if (hDC == NULL || hBmp == NULL) return NULL;

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        return NULL;
    }

    pDc_Attr = pDC->pDc_Attr;
    if(!pDc_Attr) pDc_Attr = &pDC->Dc_Attr;

    /* must be memory dc to select bitmap */
    if (!(pDC->w.flags & DC_MEMORY))
    {
        DC_UnlockDc(pDC);
        return NULL;
    }

    pBmp = BITMAPOBJ_LockBitmap(hBmp);
    if (!pBmp)
    {
        DC_UnlockDc(pDC);
        return NULL;
    }
    hOrgBmp = pDC->w.hBitmap;

    /* Release the old bitmap, lock the new one and convert it to a SURF */
    pDC->w.hBitmap = hBmp;

    // if we're working with a DIB, get the palette [fixme: only create if the selected palette is null]
    if(pBmp->dib)
    {
        pDC->w.bitsPerPixel = pBmp->dib->dsBmih.biBitCount;
        pDC->w.hPalette = pBmp->hDIBPalette;
    }
    else
    {
        pDC->w.bitsPerPixel = BitsPerFormat(pBmp->SurfObj.iBitmapFormat);
        pDC->w.hPalette = ((GDIDEVICE *)pDC->pPDev)->DevInfo.hpalDefault;
    }

    /* Regenerate the XLATEOBJs. */
    pBrush = BRUSHOBJ_LockBrush(pDc_Attr->hbrush);
    if (pBrush)
    {
        if (pDC->XlateBrush)
        {
            EngDeleteXlate(pDC->XlateBrush);
        }
        pDC->XlateBrush = IntGdiCreateBrushXlate(pDC, pBrush, &bFailed);
        BRUSHOBJ_UnlockBrush(pBrush);
    }

    pBrush = PENOBJ_LockPen(pDc_Attr->hpen);
    if (pBrush)
    {
        if (pDC->XlatePen)
        {
            EngDeleteXlate(pDC->XlatePen);
        }
        pDC->XlatePen = IntGdiCreateBrushXlate(pDC, pBrush, &bFailed);
        PENOBJ_UnlockPen(pBrush);
    }

    DC_UnlockDc(pDC);

    hVisRgn = NtGdiCreateRectRgn(0, 0, pBmp->SurfObj.sizlBitmap.cx, pBmp->SurfObj.sizlBitmap.cy);
    BITMAPOBJ_UnlockBitmap(pBmp);
    IntGdiSelectVisRgn(hDC, hVisRgn);
    NtGdiDeleteObject(hVisRgn);

    return hOrgBmp;
}

 /*
 * @implemented
 */
HBRUSH
APIENTRY
NtGdiSelectBrush(
    IN HDC hDC,
    IN HBRUSH hBrush)
{
    PDC pDC;
    PDC_ATTR pDc_Attr;
    HBRUSH hOrgBrush;
    PGDIBRUSHOBJ pBrush;
    XLATEOBJ *XlateObj;
    BOOLEAN bFailed;

    if (hDC == NULL || hBrush == NULL) return NULL;

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        return NULL;
    }

    pDc_Attr = pDC->pDc_Attr;
    if(!pDc_Attr) pDc_Attr = &pDC->Dc_Attr;

    pBrush = BRUSHOBJ_LockBrush(hBrush);
    if (pBrush == NULL)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return NULL;
    }

    XlateObj = IntGdiCreateBrushXlate(pDC, pBrush, &bFailed);
    BRUSHOBJ_UnlockBrush(pBrush);
    if(bFailed)
    {
        return NULL;
    }

    hOrgBrush = pDc_Attr->hbrush;
    pDc_Attr->hbrush = hBrush;
    if (pDC->XlateBrush != NULL)
    {
        EngDeleteXlate(pDC->XlateBrush);
    }
    pDC->XlateBrush = XlateObj;

    DC_UnlockDc(pDC);
    return hOrgBrush;
}

 /*
 * @implemented
 */
HFONT
APIENTRY
NtGdiSelectFont(
    IN HDC hDC,
    IN HFONT hFont)
{
    PDC pDC;
    PDC_ATTR pDc_Attr;
    HFONT hOrgFont = NULL;

    if (hDC == NULL || hFont == NULL) return NULL;

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        return NULL;
    }

    pDc_Attr = pDC->pDc_Attr;
    if(!pDc_Attr) pDc_Attr = &pDC->Dc_Attr;

    /* FIXME: what if not successful? */
    if(NT_SUCCESS(TextIntRealizeFont((HFONT)hFont)))
    {
        hOrgFont = pDc_Attr->hlfntNew;
        pDc_Attr->hlfntNew = hFont;
    }

    DC_UnlockDc(pDC);

    return hOrgFont;
}

 /*
 * @implemented
 */
HPEN
APIENTRY
NtGdiSelectPen(
    IN HDC hDC,
    IN HPEN hPen)
{
    PDC pDC;
    PDC_ATTR pDc_Attr;
    HPEN hOrgPen = NULL;
    PGDIBRUSHOBJ pPen;
    XLATEOBJ *XlateObj;
    BOOLEAN bFailed;

    if (hDC == NULL || hPen == NULL) return NULL;

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        return NULL;
    }

    pDc_Attr = pDC->pDc_Attr;
    if(!pDc_Attr) pDc_Attr = &pDC->Dc_Attr;

    pPen = PENOBJ_LockPen(hPen);
    if (pPen == NULL)
    {
        return NULL;
    }

    XlateObj = IntGdiCreateBrushXlate(pDC, pPen, &bFailed);
    PENOBJ_UnlockPen(pPen);
    if (bFailed)
    {
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        return NULL;
    }

    hOrgPen = pDc_Attr->hpen;
    pDc_Attr->hpen = hPen;
    if (pDC->XlatePen != NULL)
    {
        EngDeleteXlate(pDC->XlatePen);
    }
    pDC->XlatePen = XlateObj;

    DC_UnlockDc(pDC);

    return hOrgPen;
}

WORD STDCALL
IntGdiSetHookFlags(HDC hDC, WORD Flags)
{
  WORD wRet;
  DC *dc = DC_LockDc(hDC);

  if (NULL == dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }

  wRet = dc->w.flags & DC_DIRTY;

  /* "Undocumented Windows" info is slightly confusing.
   */

  DPRINT("DC %p, Flags %04x\n", hDC, Flags);

  if (Flags & DCHF_INVALIDATEVISRGN)
    {
      dc->w.flags |= DC_DIRTY;
    }
  else if (Flags & DCHF_VALIDATEVISRGN || 0 == Flags)
    {
      dc->w.flags &= ~DC_DIRTY;
    }

  DC_UnlockDc(dc);

  return wRet;
}


BOOL
STDCALL
NtGdiGetDCDword(
             HDC hDC,
             UINT u,
             DWORD *Result
               )
{
  BOOL Ret = TRUE;
  PDC dc;
  PDC_ATTR Dc_Attr;

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
  Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

  switch (u)
  {
    case GdiGetJournal:
      break;
    case GdiGetRelAbs:
      SafeResult = Dc_Attr->lRelAbs;
      break;
    case GdiGetBreakExtra:
      SafeResult = Dc_Attr->lBreakExtra;
      break;
    case GdiGerCharBreak:
      SafeResult = Dc_Attr->cBreak;
      break;
    case GdiGetArcDirection:
      SafeResult = dc->w.ArcDirection;
      break;
    case GdiGetEMFRestorDc:
      break;
    case GdiGetFontLanguageInfo:
      break;
    case GdiGetIsMemDc:
          SafeResult = dc->DC_Type;
      break;
    case GdiGetMapMode:
      SafeResult = Dc_Attr->iMapMode;
      break;
    case GdiGetTextCharExtra:
      SafeResult = Dc_Attr->lTextExtra;
      break;
    default:
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      Ret = FALSE;
      break;
  }

  if (Ret)
  {
    _SEH_TRY
    {
      ProbeForWrite(Result,
                    sizeof(DWORD),
                    1);
      *Result = SafeResult;
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
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
STDCALL
NtGdiGetAndSetDCDword(
                  HDC hDC,
                  UINT u,
                  DWORD dwIn,
                  DWORD *Result
                     )
{
  BOOL Ret = TRUE;
  PDC dc;
  PDC_ATTR Dc_Attr;

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
  Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

  switch (u)
  {
    case GdtGetSetCopyCount:
      break;
    case GdiGetSetTextAlign:
      SafeResult = Dc_Attr->lTextAlign;
      Dc_Attr->lTextAlign = dwIn;
      // Dc_Attr->flTextAlign = dwIn; // Flags!
      break;
    case GdiGetSetRelAbs:
      SafeResult = Dc_Attr->lRelAbs;
      Dc_Attr->lRelAbs = dwIn;
      break;
    case GdiGetSetTextCharExtra:
      SafeResult = Dc_Attr->lTextExtra;
      Dc_Attr->lTextExtra = dwIn;
      break;
    case GdiGetSetSelectFont:
      break;
    case GdiGetSetMapperFlagsInternal:
      break;
    case GdiGetSetMapMode:
      SafeResult = IntGdiSetMapMode( dc, dwIn);
      break;
    case GdiGetSetArcDirection:
      if (dwIn != AD_COUNTERCLOCKWISE && dwIn != AD_CLOCKWISE)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         Ret = FALSE;
      }
      SafeResult = dc->w.ArcDirection;
      dc->w.ArcDirection = dwIn;
      break;
    default:
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      Ret = FALSE;
      break;
  }

  if (Ret)
  {
    _SEH_TRY
    {
      ProbeForWrite(Result,
                    sizeof(DWORD),
                    1);
      *Result = SafeResult;
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
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
  PDC_ATTR Dc_Attr;
  HDC  hDC;
  PWSTR Buf = NULL;

  if (Driver != NULL)
  {
    Buf = ExAllocatePoolWithTag(PagedPool, Driver->MaximumLength, TAG_DC);
    if(!Buf)
    {
      return NULL;
    }
    RtlCopyMemory(Buf, Driver->Buffer, Driver->MaximumLength);
  }

  hDC = (HDC) GDIOBJ_AllocObj(GdiHandleTable, GDI_OBJECT_TYPE_DC);
  if (hDC == NULL)
  {
    if(Buf)
    {
      ExFreePool(Buf);
    }
    return  NULL;
  }

  DC_AllocateDcAttr(hDC);

  NewDC = DC_LockDc(hDC);
  /* FIXME - Handle NewDC == NULL! */
  if (Driver != NULL)
  {
    RtlCopyMemory(&NewDC->DriverName, Driver, sizeof(UNICODE_STRING));
    NewDC->DriverName.Buffer = Buf;
  }
  Dc_Attr = NewDC->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &NewDC->Dc_Attr;
  
  NewDC->hHmgr = (HGDIOBJ) hDC; // Save the handle for this DC object.
  NewDC->w.xformWorld2Wnd.eM11 = 1.0f;
  NewDC->w.xformWorld2Wnd.eM12 = 0.0f;
  NewDC->w.xformWorld2Wnd.eM21 = 0.0f;
  NewDC->w.xformWorld2Wnd.eM22 = 1.0f;
  NewDC->w.xformWorld2Wnd.eDx = 0.0f;
  NewDC->w.xformWorld2Wnd.eDy = 0.0f;
  NewDC->w.xformWorld2Vport = NewDC->w.xformWorld2Wnd;
  NewDC->w.xformVport2World = NewDC->w.xformWorld2Wnd;
  NewDC->w.vport2WorldValid = TRUE;

// Setup syncing bits for the dcattr data packets.
  Dc_Attr->flXform = DEVICE_TO_PAGE_INVALID;

  Dc_Attr->ulDirty_ = 0;  // Server side

  Dc_Attr->iMapMode = MM_TEXT;

  Dc_Attr->szlWindowExt.cx = 1; // Float to Int,,, WRONG!
  Dc_Attr->szlWindowExt.cy = 1;
  Dc_Attr->szlViewportExt.cx = 1;
  Dc_Attr->szlViewportExt.cy = 1;

  Dc_Attr->crForegroundClr = 0;
  Dc_Attr->ulForegroundClr = 0;

  Dc_Attr->ulBackgroundClr = 0xffffff;
  Dc_Attr->crBackgroundClr = 0xffffff;

  Dc_Attr->ulPenClr = RGB( 0, 0, 0 );
  Dc_Attr->crPenClr = RGB( 0, 0, 0 );

  Dc_Attr->ulBrushClr = RGB( 255, 255, 255 ); // Do this way too.
  Dc_Attr->crBrushClr = RGB( 255, 255, 255 );

  Dc_Attr->hlfntNew = NtGdiGetStockObject(SYSTEM_FONT);
  TextIntRealizeFont(Dc_Attr->hlfntNew);

  NewDC->w.hPalette = NtGdiGetStockObject(DEFAULT_PALETTE);

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

  NtGdiSelectBrush(DCHandle, NtGdiGetStockObject( WHITE_BRUSH ));
  NtGdiSelectPen(DCHandle, NtGdiGetStockObject( BLACK_PEN ));
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
  if(NewMem)
  {
     pDC->pDc_Attr = NewMem; // Store pointer
  }
  DC_UnlockDc(pDC);
}

VOID
FASTCALL
DC_FreeDcAttr(HDC  DCToFree )
{
  HANDLE Pid = NtCurrentProcess();
  PDC pDC = DC_LockDc(DCToFree);
  if (pDC->pDc_Attr == &pDC->Dc_Attr) return; // Internal DC object!
  pDC->pDc_Attr = NULL;
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
    if (!GDIOBJ_FreeObj(GdiHandleTable, DCToFree, GDI_OBJECT_TYPE_DC))
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
  RtlFreeUnicodeString(&pDC->DriverName);
  return TRUE;
}

HDC FASTCALL
DC_GetNextDC (PDC pDC)
{
  return pDC->hNext;
}

VOID FASTCALL
DC_SetNextDC (PDC pDC, HDC hNextDC)
{
  pDC->hNext = hNextDC;
}

VOID FASTCALL
DC_UpdateXforms(PDC  dc)
{
  XFORM  xformWnd2Vport;
  FLOAT  scaleX, scaleY;
  PDC_ATTR Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

  /* Construct a transformation to do the window-to-viewport conversion */
  scaleX = (Dc_Attr->szlWindowExt.cx ? (FLOAT)Dc_Attr->szlViewportExt.cx / (FLOAT)Dc_Attr->szlWindowExt.cx : 0.0f);
  scaleY = (Dc_Attr->szlWindowExt.cy ? (FLOAT)Dc_Attr->szlViewportExt.cy / (FLOAT)Dc_Attr->szlWindowExt.cy : 0.0f);
  xformWnd2Vport.eM11 = scaleX;
  xformWnd2Vport.eM12 = 0.0;
  xformWnd2Vport.eM21 = 0.0;
  xformWnd2Vport.eM22 = scaleY;
  xformWnd2Vport.eDx  = (FLOAT)Dc_Attr->ptlViewportOrg.x - scaleX * (FLOAT)Dc_Attr->ptlWindowOrg.x;
  xformWnd2Vport.eDy  = (FLOAT)Dc_Attr->ptlViewportOrg.y - scaleY * (FLOAT)Dc_Attr->ptlWindowOrg.y;

  /* Combine with the world transformation */
  IntGdiCombineTransform(&dc->w.xformWorld2Vport, &dc->w.xformWorld2Wnd, &xformWnd2Vport);

  /* Create inverse of world-to-viewport transformation */
  dc->w.vport2WorldValid = DC_InvertXform(&dc->w.xformWorld2Vport, &dc->w.xformVport2World);
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

VOID FASTCALL
DC_SetOwnership(HDC hDC, PEPROCESS Owner)
{
  PDC DC;

  GDIOBJ_SetOwnership(GdiHandleTable, hDC, Owner);
  DC = DC_LockDc(hDC);
  if (NULL != DC)
    {
      if (NULL != DC->w.hClipRgn)
        {
          GDIOBJ_CopyOwnership(GdiHandleTable, hDC, DC->w.hClipRgn);
        }
      if (NULL != DC->w.hVisRgn)
        {
          GDIOBJ_CopyOwnership(GdiHandleTable, hDC, DC->w.hVisRgn);
        }
      if (NULL != DC->w.hGCClipRgn)
        {
          GDIOBJ_CopyOwnership(GdiHandleTable, hDC, DC->w.hGCClipRgn);
        }
      DC_UnlockDc(DC);
    }
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
  Resource = ((PGDIDEVICE)dc->pPDev)->hsemDevLock;
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
  Resource = ((PGDIDEVICE)dc->pPDev)->hsemDevLock;
  DC_UnlockDc(dc);
  if (!Resource) return;
  ExReleaseResourceLite( Resource );
  KeLeaveCriticalRegion();
}

BOOL FASTCALL
IntIsPrimarySurface(SURFOBJ *SurfObj)
{
   if (PrimarySurface.Handle == NULL)
     {
       return FALSE;
     }
   return SurfObj->hsurf == PrimarySurface.Handle;
}

//
// Enumerate HDev
//
PGDIDEVICE FASTCALL
IntEnumHDev(VOID)
{
// I guess we will soon have more than one primary surface.
// This will do for now.
   return &PrimarySurface;
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

  if (pDeviceName && pDeviceName->Length <= DisplayString.Length)
    return STATUS_OBJECT_NAME_INVALID;

  if (pDeviceName == NULL || pDeviceName->Length == 0)
  {
    PWINDOW_OBJECT DesktopObject;
    HDC DesktopHDC;
    PDC pDC;

    DesktopObject = UserGetDesktopWindow();
    DesktopHDC = (HDC)UserGetWindowDC(DesktopObject);
    pDC = DC_LockDc(DesktopHDC);

    *DisplayNumber = ((GDIDEVICE *)pDC->pPDev)->DisplayNumber;

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
BOOL FASTCALL
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

  if (!NT_SUCCESS(GetDisplayNumberFromDeviceName(pDeviceName, &DisplayNumber)))
  {
    SetLastWin32Error(STATUS_NO_SUCH_DEVICE);
    return FALSE;
  }

  DPRINT("DevMode->dmSize = %d\n", pDevMode->dmSize);
  DPRINT("DevMode->dmExtraSize = %d\n", pDevMode->dmDriverExtra);
  if (pDevMode->dmSize != SIZEOF_DEVMODEW_300 &&
      pDevMode->dmSize != SIZEOF_DEVMODEW_400 &&
      pDevMode->dmSize != SIZEOF_DEVMODEW_500)
  {
    SetLastWin32Error(STATUS_INVALID_PARAMETER);
    return FALSE;
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
      SetLastWin32Error(0); /* FIXME: use error code */
      return FALSE;
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
        return FALSE;
      }

      IntPrepareDriverIfNeeded();

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

            SetLastWin32Error(STATUS_NO_MEMORY);
            return FALSE;
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

      RtlFreeUnicodeString(&DriverFileNames);
    }

    /* return cached info */
    CachedMode = CachedDevModes;
    if (CachedMode >= CachedDevModesEnd)
    {
      SetLastWin32Error(STATUS_NO_MORE_ENTRIES);
      return FALSE;
    }
    while (iModeNum-- > 0 && CachedMode < CachedDevModesEnd)
    {
      assert(CachedMode->dmSize > 0);
      CachedMode = (DEVMODEW *)((PCHAR)CachedMode + CachedMode->dmSize + CachedMode->dmDriverExtra);
    }
    if (CachedMode >= CachedDevModesEnd)
    {
      SetLastWin32Error(STATUS_NO_MORE_ENTRIES);
      return FALSE;
    }
  }

  ASSERT(CachedMode != NULL);

  RtlCopyMemory(pDevMode, CachedMode, min(pDevMode->dmSize, CachedMode->dmSize));
  RtlZeroMemory(pDevMode + pDevMode->dmSize, pDevMode->dmDriverExtra);
  RtlCopyMemory(pDevMode + min(pDevMode->dmSize, CachedMode->dmSize), CachedMode + CachedMode->dmSize, min(pDevMode->dmDriverExtra, CachedMode->dmDriverExtra));

  return TRUE;
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

      hDC = (HDC)UserGetWindowDC(Wnd);

      DC = DC_LockDc(hDC);
      if (NULL == DC)
      {
         return FALSE;
      }
      swprintf (szBuffer, L"\\\\.\\DISPLAY%lu", ((GDIDEVICE *)DC->pPDev)->DisplayNumber);
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
    Status = ZwOpenKey(&DevInstRegKey, GENERIC_READ | GENERIC_WRITE, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
      DPRINT1("Unable to open registry key %wZ (Status 0x%08lx)\n", &RegistryKey, Status);
      ExFreePoolWithTag(RegistryKey.Buffer, TAG_DC);
      return DISP_CHANGE_FAILED;
    }
    ExFreePoolWithTag(RegistryKey.Buffer, TAG_DC);

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
  DPRINT1("stub");
  return  DCB_RESET;   /* bounding rectangle always empty */
}

DWORD
APIENTRY
NtGdiSetBoundsRect(
    IN HDC hdc,
    IN LPRECT prc,
    IN DWORD f)
{
  DPRINT1("stub");
  return  DCB_DISABLE;   /* bounding rectangle always empty */
}

BOOL
STDCALL
NtGdiGetAspectRatioFilterEx(HDC  hDC,
                                 LPSIZE  AspectRatio)
{
  UNIMPLEMENTED;
  return FALSE;
}

/* EOF */
