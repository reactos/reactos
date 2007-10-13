
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

/*
 * DC device-independent Get/SetXXX functions
 * (RJJ) swiped from WINE
 */

#define DC_GET_VAL( func_type, func_name, dc_field ) \
func_type STDCALL  func_name( HDC hdc ) \
{                                   \
  func_type  ft;                    \
  PDC  dc = DC_LockDc( hdc );       \
  if (!dc)                          \
  {                                 \
    SetLastWin32Error(ERROR_INVALID_HANDLE); \
    return 0;                       \
  }                                 \
  ft = dc->dc_field;                \
  DC_UnlockDc(dc);                  \
  return ft;                        \
}

/* DC_GET_VAL_EX is used to define functions returning a POINT or a SIZE. It is
 * important that the function has the right signature, for the implementation
 * we can do whatever we want.
 */
#define DC_GET_VAL_EX( FuncName, ret_x, ret_y, type, ax, ay ) \
VOID FASTCALL Int##FuncName ( PDC dc, LP##type pt) \
{ \
  ASSERT(dc); \
  ASSERT(pt); \
  pt->ax = dc->ret_x; \
  pt->ay = dc->ret_y; \
} \
BOOL STDCALL NtGdi##FuncName ( HDC hdc, LP##type pt ) \
{ \
  NTSTATUS Status = STATUS_SUCCESS; \
  type Safept; \
  PDC dc; \
  if(!pt) \
  { \
    SetLastWin32Error(ERROR_INVALID_PARAMETER); \
    return FALSE; \
  } \
  if(!(dc = DC_LockDc(hdc))) \
  { \
    SetLastWin32Error(ERROR_INVALID_HANDLE); \
    return FALSE; \
  } \
  Int##FuncName( dc, &Safept); \
  DC_UnlockDc(dc); \
  _SEH_TRY \
  { \
    ProbeForWrite(pt, \
                  sizeof( type ), \
                  1); \
    *pt = Safept; \
  } \
  _SEH_HANDLE \
  { \
    Status = _SEH_GetExceptionCode(); \
  } \
  _SEH_END; \
  if(!NT_SUCCESS(Status)) \
  { \
    SetLastNtError(Status); \
    return FALSE; \
  } \
  return TRUE; \
}

#define DC_SET_MODE( func_name, dc_field, min_val, max_val ) \
INT STDCALL  func_name( HDC hdc, INT mode ) \
{                                           \
  INT  prevMode;                            \
  PDC  dc;                                  \
  if ((mode < min_val) || (mode > max_val)) \
  { \
    SetLastWin32Error(ERROR_INVALID_PARAMETER); \
    return 0;                               \
  } \
  dc = DC_LockDc ( hdc );              \
  if ( !dc )                                \
  { \
    SetLastWin32Error(ERROR_INVALID_HANDLE); \
    return 0;                               \
  } \
  prevMode = dc->dc_field;                  \
  dc->dc_field = mode;                      \
  DC_UnlockDc ( dc );                    \
  return prevMode;                          \
}


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

  /* Copy information from original DC to new DC  */
  NewDC->hSelf = hNewDC;
  NewDC->IsIC = FALSE;
  NewDC->DC_Type = DC_Type;
  
  NewDC->PDev = OrigDC->PDev;
  memcpy(NewDC->FillPatternSurfaces,
         OrigDC->FillPatternSurfaces,
         sizeof OrigDC->FillPatternSurfaces);
  NewDC->GDIInfo = OrigDC->GDIInfo;
  NewDC->DevInfo = OrigDC->DevInfo;
  NewDC->w.bitsPerPixel = OrigDC->w.bitsPerPixel;

  /* DriverName is copied in the AllocDC routine  */
  NewDC->DeviceDriver = OrigDC->DeviceDriver;
  NewDC->Dc_Attr.ptlWindowOrg.x = OrigDC->Dc_Attr.ptlWindowOrg.x;
  NewDC->Dc_Attr.ptlWindowOrg.y = OrigDC->Dc_Attr.ptlWindowOrg.y;
  NewDC->Dc_Attr.szlWindowExt.cx = OrigDC->Dc_Attr.szlWindowExt.cx;
  NewDC->Dc_Attr.szlWindowExt.cy = OrigDC->Dc_Attr.szlWindowExt.cy;
  NewDC->Dc_Attr.ptlViewportOrg.x = OrigDC->Dc_Attr.ptlViewportOrg.x;
  NewDC->Dc_Attr.ptlViewportOrg.y = OrigDC->Dc_Attr.ptlViewportOrg.y;
  NewDC->Dc_Attr.szlViewportExt.cx = OrigDC->Dc_Attr.szlViewportExt.cx;
  NewDC->Dc_Attr.szlViewportExt.cy = OrigDC->Dc_Attr.szlViewportExt.cy;

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
  NewDC->GDIDevice      = OrigDC->GDIDevice;

  NewDC->PalIndexed = OrigDC->PalIndexed;
  NewDC->w.hPalette = OrigDC->w.hPalette;
  NewDC->Dc_Attr.crForegroundClr = OrigDC->Dc_Attr.crForegroundClr;
  NewDC->Dc_Attr.lTextAlign = OrigDC->Dc_Attr.lTextAlign;
  NewDC->Dc_Attr.crBackgroundClr = OrigDC->Dc_Attr.crBackgroundClr;
  NewDC->Dc_Attr.jBkMode = OrigDC->Dc_Attr.jBkMode;
  NewDC->Dc_Attr.jROP2 = OrigDC->Dc_Attr.jROP2;

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
  DCU_SynchDcAttrtoUser(hNewDC, -1);

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

      RtlFreeUnicodeString(&DriverFileNames);

      if (!GotDriver)
      {
         ObDereferenceObject(PrimarySurface.VideoFileObject);
         DPRINT1("No suitable DDI driver found\n");
         continue;
      }

      DPRINT("Display driver %S loaded\n", CurrentName);

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

  NewDC->DC_Type = DC_TYPE_DIRECT;
  NewDC->IsIC = CreateAsIC;
  NewDC->DevInfo = &PrimarySurface.DevInfo;
  NewDC->GDIInfo = &PrimarySurface.GDIInfo;
  memcpy(NewDC->FillPatternSurfaces, PrimarySurface.FillPatterns,
  sizeof(NewDC->FillPatternSurfaces));
  NewDC->PDev = PrimarySurface.PDev;
  if(pUMdhpdev) pUMdhpdev = NewDC->PDev;
  NewDC->GDIDevice = (HDEV)&PrimarySurface;
  NewDC->DriverFunctions = PrimarySurface.DriverFunctions;
  NewDC->w.hBitmap = PrimarySurface.Handle;

  NewDC->w.bitsPerPixel = NewDC->GDIInfo->cBitsPixel * NewDC->GDIInfo->cPlanes;
  DPRINT("Bits per pel: %u\n", NewDC->w.bitsPerPixel);

  if (! CreateAsIC)
  {
    NewDC->PalIndexed = NtGdiGetStockObject(DEFAULT_PALETTE);
    NewDC->w.hPalette = NewDC->DevInfo->hpalDefault;
    NewDC->Dc_Attr.jROP2 = R2_COPYPEN;

    DC_UnlockDc( NewDC );

    hVisRgn = NtGdiCreateRectRgn(0, 0, NewDC->GDIInfo->ulHorzRes,
                                 NewDC->GDIInfo->ulVertRes);
    IntGdiSelectVisRgn(hNewDC, hVisRgn);
    NtGdiDeleteObject(hVisRgn);

    /*  Initialize the DC state  */
    DC_InitDC(hNewDC);
    NtGdiSetTextColor(hNewDC, RGB(0, 0, 0));
    NtGdiSetTextAlign(hNewDC, TA_TOP);
    NtGdiSetBkColor(hNewDC, RGB(255, 255, 255));
    NtGdiSetBkMode(hNewDC, OPAQUE);
    DCU_SynchDcAttrtoUser(hNewDC, -1);
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

BOOL STDCALL
NtGdiDeleteObjectApp(HANDLE  DCHandle)
{
  PDC  DCToDelete;

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
    NtGdiSelectObject (DCHandle, STOCK_BLACK_PEN);
    NtGdiSelectObject (DCHandle, STOCK_WHITE_BRUSH);
    NtGdiSelectObject (DCHandle, STOCK_SYSTEM_FONT);
    DC_LockDC (DCHandle); NtGdiSelectObject does not recognize stock objects yet  */
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

DC_GET_VAL( COLORREF, NtGdiGetBkColor, Dc_Attr.crBackgroundClr )
DC_GET_VAL( INT, NtGdiGetBkMode, Dc_Attr.jBkMode )
DC_GET_VAL_EX( GetBrushOrgEx, Dc_Attr.ptlBrushOrigin.x, Dc_Attr.ptlBrushOrigin.y, POINT, x, y )

HANDLE
STDCALL
NtGdiGetDCObject(HDC  hDC, INT  ObjectType)
{
  HGDIOBJ SelObject;
  DC *dc;

  /* From Wine: GetCurrentObject does not SetLastError() on a null object */
  if(!hDC) return NULL;

  if(!(dc = DC_LockDc(hDC)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return NULL;
  }

  switch(ObjectType)
  {
    case GDI_OBJECT_TYPE_EXTPEN:
    case GDI_OBJECT_TYPE_PEN:         
      SelObject = dc->Dc_Attr.hpen;
      break;
    case GDI_OBJECT_TYPE_BRUSH:
      SelObject = dc->Dc_Attr.hbrush;
      break;
    case GDI_OBJECT_TYPE_PALETTE:
      SelObject = dc->w.hPalette;
      break;
    case GDI_OBJECT_TYPE_FONT:
      SelObject = dc->Dc_Attr.hlfntNew;
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

DC_GET_VAL_EX ( GetCurrentPositionEx, Dc_Attr.ptlCurrent.x, Dc_Attr.ptlCurrent.y, POINT, x, y )

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
  POINT SafePoint;
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

COLORREF STDCALL
NtGdiSetBkColor(HDC hDC, COLORREF color)
{
  COLORREF oldColor;
  PDC dc;
  HBRUSH hBrush;

  if (!(dc = DC_LockDc(hDC)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return CLR_INVALID;
  }

  oldColor = dc->Dc_Attr.crBackgroundClr;
  dc->Dc_Attr.crBackgroundClr = color;
  hBrush = dc->Dc_Attr.hbrush;
  DC_UnlockDc(dc);
  NtGdiSelectObject(hDC, hBrush);
  return oldColor;
}


VOID
FASTCALL
IntGdiCopyToSaveState(PDC dc, PDC newdc)
{
  newdc->w.flags                   = dc->w.flags | DC_SAVED;
  newdc->Dc_Attr.hpen              = dc->Dc_Attr.hpen;
  newdc->Dc_Attr.hbrush            = dc->Dc_Attr.hbrush;
  newdc->Dc_Attr.hlfntNew          = dc->Dc_Attr.hlfntNew;
  newdc->w.hBitmap                 = dc->w.hBitmap;
  newdc->w.hFirstBitmap            = dc->w.hFirstBitmap;
#if 0
  newdc->w.hDevice                 = dc->w.hDevice;
#endif
  newdc->PalIndexed                = dc->PalIndexed;
  newdc->w.hPalette                = dc->w.hPalette;
  newdc->w.totalExtent             = dc->w.totalExtent;
  newdc->w.bitsPerPixel            = dc->w.bitsPerPixel;
  newdc->Dc_Attr.jROP2             = dc->Dc_Attr.jROP2;
  newdc->Dc_Attr.jFillMode         = dc->Dc_Attr.jFillMode;
  newdc->Dc_Attr.jStretchBltMode   = dc->Dc_Attr.jStretchBltMode;
  newdc->Dc_Attr.lRelAbs           = dc->Dc_Attr.lRelAbs;
  newdc->Dc_Attr.jBkMode           = dc->Dc_Attr.jBkMode;
  newdc->Dc_Attr.crBackgroundClr   = dc->Dc_Attr.crBackgroundClr;
  newdc->Dc_Attr.crForegroundClr   = dc->Dc_Attr.crForegroundClr;
  newdc->Dc_Attr.ptlBrushOrigin.x  = dc->Dc_Attr.ptlBrushOrigin.x;
  newdc->Dc_Attr.ptlBrushOrigin.y  = dc->Dc_Attr.ptlBrushOrigin.y;
  newdc->Dc_Attr.lTextAlign        = dc->Dc_Attr.lTextAlign;
  newdc->Dc_Attr.lTextExtra        = dc->Dc_Attr.lTextExtra;
  newdc->Dc_Attr.cBreak            = dc->Dc_Attr.cBreak;
  newdc->Dc_Attr.lBreakExtra       = dc->Dc_Attr.lBreakExtra;
  newdc->Dc_Attr.iMapMode          = dc->Dc_Attr.iMapMode;
  newdc->Dc_Attr.iGraphicsMode     = dc->Dc_Attr.iGraphicsMode;
#if 0
  /* Apparently, the DC origin is not changed by [GS]etDCState */
  newdc->w.DCOrgX                  = dc->w.DCOrgX;
  newdc->w.DCOrgY                  = dc->w.DCOrgY;
#endif
  newdc->Dc_Attr.ptlCurrent.x      = dc->Dc_Attr.ptlCurrent.x;
  newdc->Dc_Attr.ptlCurrent.y      = dc->Dc_Attr.ptlCurrent.y;
  newdc->w.ArcDirection            = dc->w.ArcDirection;
  newdc->w.xformWorld2Wnd          = dc->w.xformWorld2Wnd;
  newdc->w.xformWorld2Vport        = dc->w.xformWorld2Vport;
  newdc->w.xformVport2World        = dc->w.xformVport2World;
  newdc->w.vport2WorldValid        = dc->w.vport2WorldValid;
  newdc->Dc_Attr.ptlWindowOrg.x    = dc->Dc_Attr.ptlWindowOrg.x;
  newdc->Dc_Attr.ptlWindowOrg.y    = dc->Dc_Attr.ptlWindowOrg.y;
  newdc->Dc_Attr.szlWindowExt.cx   = dc->Dc_Attr.szlWindowExt.cx;
  newdc->Dc_Attr.szlWindowExt.cy   = dc->Dc_Attr.szlWindowExt.cy;
  newdc->Dc_Attr.ptlViewportOrg.x  = dc->Dc_Attr.ptlViewportOrg.x;
  newdc->Dc_Attr.ptlViewportOrg.y  = dc->Dc_Attr.ptlViewportOrg.y;
  newdc->Dc_Attr.szlViewportExt.cx = dc->Dc_Attr.szlViewportExt.cx;
  newdc->Dc_Attr.szlViewportExt.cy = dc->Dc_Attr.szlViewportExt.cy;

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
  dc->w.flags                   = dcs->w.flags & ~DC_SAVED;

  dc->w.hFirstBitmap            = dcs->w.hFirstBitmap;

#if 0
  dc->w.hDevice                 = dcs->w.hDevice;
#endif

  dc->w.totalExtent             = dcs->w.totalExtent;
  dc->Dc_Attr.jROP2             = dcs->Dc_Attr.jROP2;
  dc->Dc_Attr.jFillMode         = dcs->Dc_Attr.jFillMode;
  dc->Dc_Attr.jStretchBltMode   = dcs->Dc_Attr.jStretchBltMode;
  dc->Dc_Attr.lRelAbs           = dcs->Dc_Attr.lRelAbs;
  dc->Dc_Attr.jBkMode           = dcs->Dc_Attr.jBkMode;
  dc->Dc_Attr.crBackgroundClr   = dcs->Dc_Attr.crBackgroundClr;
  dc->Dc_Attr.crForegroundClr   = dcs->Dc_Attr.crForegroundClr;
  dc->Dc_Attr.ptlBrushOrigin.x  = dcs->Dc_Attr.ptlBrushOrigin.x;
  dc->Dc_Attr.ptlBrushOrigin.y  = dcs->Dc_Attr.ptlBrushOrigin.y;
  dc->Dc_Attr.lTextAlign        = dcs->Dc_Attr.lTextAlign;
  dc->Dc_Attr.lTextExtra        = dcs->Dc_Attr.lTextExtra;
  dc->Dc_Attr.cBreak            = dcs->Dc_Attr.cBreak;
  dc->Dc_Attr.lBreakExtra       = dcs->Dc_Attr.lBreakExtra;
  dc->Dc_Attr.iMapMode          = dcs->Dc_Attr.iMapMode;
  dc->Dc_Attr.iGraphicsMode     = dcs->Dc_Attr.iGraphicsMode;
#if 0
/* Apparently, the DC origin is not changed by [GS]etDCState */
  dc->w.DCOrgX                  = dcs->w.DCOrgX;
  dc->w.DCOrgY                  = dcs->w.DCOrgY;
#endif
  dc->Dc_Attr.ptlCurrent.x      = dcs->Dc_Attr.ptlCurrent.x;
  dc->Dc_Attr.ptlCurrent.y      = dcs->Dc_Attr.ptlCurrent.y;
  dc->w.ArcDirection            = dcs->w.ArcDirection;

  dc->w.xformWorld2Wnd          = dcs->w.xformWorld2Wnd;
  dc->w.xformWorld2Vport        = dcs->w.xformWorld2Vport;
  dc->w.xformVport2World        = dcs->w.xformVport2World;
  dc->w.vport2WorldValid        = dcs->w.vport2WorldValid;
  dc->Dc_Attr.ptlWindowOrg.x    = dcs->Dc_Attr.ptlWindowOrg.x;
  dc->Dc_Attr.ptlWindowOrg.y    = dcs->Dc_Attr.ptlWindowOrg.y;
  dc->Dc_Attr.szlWindowExt.cx   = dcs->Dc_Attr.szlWindowExt.cx;
  dc->Dc_Attr.szlWindowExt.cy   = dcs->Dc_Attr.szlWindowExt.cy;
  dc->Dc_Attr.ptlViewportOrg.x  = dcs->Dc_Attr.ptlViewportOrg.x;
  dc->Dc_Attr.ptlViewportOrg.y  = dcs->Dc_Attr.ptlViewportOrg.y;
  dc->Dc_Attr.szlViewportExt.cx = dcs->Dc_Attr.szlViewportExt.cx;
  dc->Dc_Attr.szlViewportExt.cy = dcs->Dc_Attr.szlViewportExt.cy;
  dc->PalIndexed                = dcs->PalIndexed;

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
  
  NtGdiSelectObject( hDC, dcs->w.hBitmap );
  NtGdiSelectObject( hDC, dcs->Dc_Attr.hbrush );
  NtGdiSelectObject( hDC, dcs->Dc_Attr.hlfntNew );
  NtGdiSelectObject( hDC, dcs->Dc_Attr.hpen );

  NtGdiSetBkColor( hDC, dcs->Dc_Attr.crBackgroundClr);
  NtGdiSetTextColor( hDC, dcs->Dc_Attr.crForegroundClr);

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

//  DCU_SyncDcAttrtoUser(newdc, -1);
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
      ret = dc->GDIInfo->ulVersion;
      break;

    case TECHNOLOGY:
      ret = dc->GDIInfo->ulTechnology;
      break;

    case HORZSIZE:
      ret = dc->GDIInfo->ulHorzSize;
      break;

    case VERTSIZE:
      ret = dc->GDIInfo->ulVertSize;
      break;

    case HORZRES:
      ret = dc->GDIInfo->ulHorzRes;
      break;

    case VERTRES:
      ret = dc->GDIInfo->ulVertRes;
      break;

    case LOGPIXELSX:
      ret = dc->GDIInfo->ulLogPixelsX;
      break;

    case LOGPIXELSY:
      ret = dc->GDIInfo->ulLogPixelsY;
      break;

    case BITSPIXEL:
      ret = dc->GDIInfo->cBitsPixel;
      break;

    case PLANES:
      ret = dc->GDIInfo->cPlanes;
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
      ret = dc->GDIInfo->ulNumColors;
      break;

    case ASPECTX:
      ret = dc->GDIInfo->ulAspectX;
      break;

    case ASPECTY:
      ret = dc->GDIInfo->ulAspectY;
      break;

    case ASPECTXY:
      ret = dc->GDIInfo->ulAspectXY;
      break;

    case PDEVICESIZE:
      UNIMPLEMENTED; /* FIXME */
      break;

    case CLIPCAPS:
      UNIMPLEMENTED; /* FIXME */
      break;

    case SIZEPALETTE:
      ret = dc->GDIInfo->ulNumPalReg; /* FIXME not sure */
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
      ret = dc->GDIInfo->flRaster;
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
      ret = dc->GDIInfo->flTextCaps;
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

DC_GET_VAL( INT, NtGdiGetMapMode, Dc_Attr.iMapMode )
DC_GET_VAL( INT, NtGdiGetPolyFillMode, Dc_Attr.jFillMode )

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

DC_GET_VAL( INT, NtGdiGetROP2, Dc_Attr.jROP2 )
DC_GET_VAL( INT, NtGdiGetStretchBltMode, Dc_Attr.jStretchBltMode )
DC_GET_VAL( UINT, NtGdiGetTextAlign, Dc_Attr.lTextAlign )
DC_GET_VAL( COLORREF, NtGdiGetTextColor, Dc_Attr.crForegroundClr )
DC_GET_VAL_EX( GetViewportExtEx, Dc_Attr.szlViewportExt.cx, Dc_Attr.szlViewportExt.cy, SIZE, cx, cy )
DC_GET_VAL_EX( GetViewportOrgEx, Dc_Attr.ptlViewportOrg.x, Dc_Attr.ptlViewportOrg.y, POINT, x, y )
DC_GET_VAL_EX( GetWindowExtEx, Dc_Attr.szlWindowExt.cx, Dc_Attr.szlWindowExt.cy, SIZE, cx, cy )
DC_GET_VAL_EX( GetWindowOrgEx, Dc_Attr.ptlWindowOrg.x, Dc_Attr.ptlWindowOrg.y, POINT, x, y )

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

HGDIOBJ
STDCALL
NtGdiSelectObject(HDC  hDC, HGDIOBJ  hGDIObj)
{
  HGDIOBJ objOrg = NULL; // default to failure
  BITMAPOBJ *pb;
  PDC dc;
  PGDIBRUSHOBJ pen;
  PGDIBRUSHOBJ brush;
  XLATEOBJ *XlateObj;
  DWORD objectType;
  HRGN hVisRgn;
  BOOLEAN Failed;

  if (!hDC || !hGDIObj)
    {
    /* From Wine:
     * SelectObject() with a NULL DC returns 0 and sets ERROR_INVALID_HANDLE.
     * Note: Under XP at least invalid ptrs can also be passed, not just NULL;
     *       Don't test that here in case it crashes earlier win versions.
     */
       if (!hDC) SetLastWin32Error(ERROR_INVALID_HANDLE);
       return NULL;
    }

  dc = DC_LockDc(hDC);
  if (NULL == dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return NULL;
    }

  objectType = GDIOBJ_GetObjectType(hGDIObj);

  switch (objectType)
  {
    case GDI_OBJECT_TYPE_PEN:
      pen = PENOBJ_LockPen((HPEN) hGDIObj);
      if (pen == NULL)
      {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        break;
      }

      XlateObj = IntGdiCreateBrushXlate(dc, pen, &Failed);
      PENOBJ_UnlockPen(pen);
      if (Failed)
      {
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        break;
      }

      objOrg = (HGDIOBJ)dc->Dc_Attr.hpen;
      dc->Dc_Attr.hpen = hGDIObj;
      if (dc->XlatePen != NULL)
        EngDeleteXlate(dc->XlatePen);
      dc->XlatePen = XlateObj;
      break;

    case GDI_OBJECT_TYPE_BRUSH:
      brush = BRUSHOBJ_LockBrush((HPEN) hGDIObj);
      if (brush == NULL)
      {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        break;
      }

      XlateObj = IntGdiCreateBrushXlate(dc, brush, &Failed);
      BRUSHOBJ_UnlockBrush(brush);
      if (Failed)
      {
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        break;
      }

      objOrg = (HGDIOBJ)dc->Dc_Attr.hbrush;
      dc->Dc_Attr.hbrush = hGDIObj;
      if (dc->XlateBrush != NULL)
        EngDeleteXlate(dc->XlateBrush);
      dc->XlateBrush = XlateObj;
      break;

    case GDI_OBJECT_TYPE_FONT:
      if(NT_SUCCESS(TextIntRealizeFont((HFONT)hGDIObj)))
      {
        objOrg = (HGDIOBJ)dc->Dc_Attr.hlfntNew;
        dc->Dc_Attr.hlfntNew = (HFONT) hGDIObj;
      }
      break;

    case GDI_OBJECT_TYPE_BITMAP:
      // must be memory dc to select bitmap
      if (!(dc->w.flags & DC_MEMORY))
        {
          DC_UnlockDc(dc);
          return NULL;
        }
      pb = BITMAPOBJ_LockBitmap(hGDIObj);
      if (NULL == pb)
        {
          SetLastWin32Error(ERROR_INVALID_HANDLE);
          DC_UnlockDc(dc);
          return NULL;
        }
      objOrg = (HGDIOBJ)dc->w.hBitmap;

      /* Release the old bitmap, lock the new one and convert it to a SURF */
      dc->w.hBitmap = hGDIObj;

      // if we're working with a DIB, get the palette [fixme: only create if the selected palette is null]
      if(pb->dib)
      {
        dc->w.bitsPerPixel = pb->dib->dsBmih.biBitCount;
        dc->w.hPalette = pb->hDIBPalette;
      }
      else
      {
        dc->w.bitsPerPixel = BitsPerFormat(pb->SurfObj.iBitmapFormat);
        dc->w.hPalette = dc->DevInfo->hpalDefault;
      }

      /* Reselect brush and pen to regenerate the XLATEOBJs. */
      NtGdiSelectObject ( hDC, dc->Dc_Attr.hbrush );
      NtGdiSelectObject ( hDC, dc->Dc_Attr.hpen );

      DC_UnlockDc ( dc );
      hVisRgn = NtGdiCreateRectRgn ( 0, 0, pb->SurfObj.sizlBitmap.cx, pb->SurfObj.sizlBitmap.cy );
      BITMAPOBJ_UnlockBitmap( pb );
      IntGdiSelectVisRgn ( hDC, hVisRgn );
      NtGdiDeleteObject ( hVisRgn );

      return objOrg;

    case GDI_OBJECT_TYPE_REGION:
      /*
       * The return value is one of the following values:
       *  SIMPLEREGION
       *  COMPLEXREGION
       *  NULLREGION
       */
      objectType = IntGdiExtSelectClipRgn(dc, (HRGN)hGDIObj, RGN_COPY);
      DC_UnlockDc (dc);
      return (HGDIOBJ)objectType;

    default:
      break;
  }
//  DCU_SyncDcAttrtoUser(dc, -1);
  DC_UnlockDc( dc );
  return objOrg;
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
  DC *dc;
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

  switch (u)
  {
    case GdiGetJournal:
      break;
    case GdiGetRelAbs:
      SafeResult = dc->Dc_Attr.lRelAbs;
      break;
    case GdiGetBreakExtra:
      SafeResult = dc->Dc_Attr.lBreakExtra;
      break;
    case GdiGerCharBreak:
      SafeResult = dc->Dc_Attr.cBreak;
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
      SafeResult = dc->Dc_Attr.iMapMode;
      break;
    case GdiGetTextCharExtra:
      SafeResult = dc->Dc_Attr.lTextExtra;
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
  DC *dc;
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

  switch (u)
  {
    case GdtGetSetCopyCount:
      break;
    case GdiGetSetTextAlign:
      SafeResult = dc->Dc_Attr.lTextAlign;
      dc->Dc_Attr.lTextAlign = dwIn;
      dc->Dc_Attr.flTextAlign = dwIn;
      break;
    case GdiGetSetRelAbs:
      SafeResult = dc->Dc_Attr.lRelAbs;
      dc->Dc_Attr.lRelAbs = dwIn;
      break;
    case GdiGetSetTextCharExtra:
      SafeResult = dc->Dc_Attr.lTextExtra;
      dc->Dc_Attr.lTextExtra = dwIn;
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


DC_SET_MODE( NtGdiSetBkMode, Dc_Attr.jBkMode, TRANSPARENT, OPAQUE )
DC_SET_MODE( NtGdiSetPolyFillMode, Dc_Attr.jFillMode, ALTERNATE, WINDING )
DC_SET_MODE( NtGdiSetROP2, Dc_Attr.jROP2, R2_BLACK, R2_WHITE )
DC_SET_MODE( NtGdiSetStretchBltMode, Dc_Attr.jStretchBltMode, BLACKONWHITE, HALFTONE )

//  ----------------------------------------------------  Private Interface

HDC FASTCALL
DC_AllocDC(PUNICODE_STRING Driver)
{
  PDC  NewDC;
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
  NewDC->Dc_Attr.flXform = DEVICE_TO_PAGE_INVALID;

  NewDC->Dc_Attr.ulDirty_ = 0;  // Server side

  NewDC->Dc_Attr.iMapMode = MM_TEXT;

  NewDC->Dc_Attr.szlWindowExt.cx = 1; // Float to Int,,, WRONG!
  NewDC->Dc_Attr.szlWindowExt.cy = 1;
  NewDC->Dc_Attr.szlViewportExt.cx = 1;
  NewDC->Dc_Attr.szlViewportExt.cy = 1;

  NewDC->Dc_Attr.crForegroundClr = 0;
  NewDC->Dc_Attr.ulForegroundClr = 0;

  NewDC->Dc_Attr.ulBackgroundClr = 0xffffff;
  NewDC->Dc_Attr.crBackgroundClr = 0xffffff;

  NewDC->Dc_Attr.ulPenClr = RGB( 0, 0, 0 );
  NewDC->Dc_Attr.crPenClr = RGB( 0, 0, 0 );

  NewDC->Dc_Attr.ulBrushClr = RGB( 255, 255, 255 ); // Do this way too.
  NewDC->Dc_Attr.crBrushClr = RGB( 255, 255, 255 );

  NewDC->Dc_Attr.hlfntNew = NtGdiGetStockObject(SYSTEM_FONT);
  TextIntRealizeFont(NewDC->Dc_Attr.hlfntNew);
  
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

  NtGdiSelectObject(DCHandle, NtGdiGetStockObject( WHITE_BRUSH ));
  NtGdiSelectObject(DCHandle, NtGdiGetStockObject( BLACK_PEN ));
  //NtGdiSelectObject(DCHandle, hFont);

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
  PDC pDC = DC_LockDc(hDC);
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
                                   MEM_DECOMMIT);
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

  if (!GDIOBJ_FreeObj(GdiHandleTable, DCToFree, GDI_OBJECT_TYPE_DC))
  {
    DPRINT("DC_FreeDC failed\n");
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

  /* Construct a transformation to do the window-to-viewport conversion */
  scaleX = (dc->Dc_Attr.szlWindowExt.cx ? (FLOAT)dc->Dc_Attr.szlViewportExt.cx / (FLOAT)dc->Dc_Attr.szlWindowExt.cx : 0.0f);
  scaleY = (dc->Dc_Attr.szlWindowExt.cy ? (FLOAT)dc->Dc_Attr.szlViewportExt.cy / (FLOAT)dc->Dc_Attr.szlWindowExt.cy : 0.0f);
  xformWnd2Vport.eM11 = scaleX;
  xformWnd2Vport.eM12 = 0.0;
  xformWnd2Vport.eM21 = 0.0;
  xformWnd2Vport.eM22 = scaleY;
  xformWnd2Vport.eDx  = (FLOAT)dc->Dc_Attr.ptlViewportOrg.x - scaleX * (FLOAT)dc->Dc_Attr.ptlWindowOrg.x;
  xformWnd2Vport.eDy  = (FLOAT)dc->Dc_Attr.ptlViewportOrg.y - scaleY * (FLOAT)dc->Dc_Attr.ptlWindowOrg.y;

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

BOOL FASTCALL
IntIsPrimarySurface(SURFOBJ *SurfObj)
{
   if (PrimarySurface.Handle == NULL)
     {
       return FALSE;
     }
   return SurfObj->hsurf == PrimarySurface.Handle;
}

/*
 * Returns the color of the brush or pen that is currently selected into the DC.
 * This function is called from GetDCBrushColor() and GetDCPenColor()
 */
COLORREF FASTCALL
IntGetDCColor(HDC hDC, ULONG Object)
{
   /*
    * The previous implementation was completly incorrect. It modified the
    * brush that was currently selected into the device context, but in fact
    * the DC pen/brush color should be stored directly in the device context
    * (at address 0x2C of the user mode DC object memory on Windows 2K/XP).
    * The actual color is then used when DC_BRUSH/DC_PEN object is selected
    * into the device context and BRUSHOBJ for drawing is composed (belongs
    * to IntGdiInitBrushInstance in the current ReactOS implementation). Also
    * the implementation should be moved to user mode GDI32.dll when UM
    * mapped GDI objects will be implemented.
    */

   DPRINT("WIN32K:IntGetDCColor is unimplemented\n");
   return 0xFFFFFF; /* The default DC color. */
}

/*
 * Changes the color of the brush or pen that is currently selected into the DC.
 * This function is called from SetDCBrushColor() and SetDCPenColor()
 */
COLORREF FASTCALL
IntSetDCColor(HDC hDC, ULONG Object, COLORREF Color)
{
   /* See the comment in IntGetDCColor. */

   DPRINT("WIN32K:IntSetDCColor is unimplemented\n");
   return CLR_INVALID;
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

    *DisplayNumber = ((GDIDEVICE *)pDC->GDIDevice)->DisplayNumber;

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
      swprintf (szBuffer, L"\\\\.\\DISPLAY%lu", ((GDIDEVICE *)DC->GDIDevice)->DisplayNumber);
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
