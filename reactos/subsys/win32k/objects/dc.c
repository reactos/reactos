/* $Id: dc.c,v 1.59 2003/05/04 15:41:40 gvg Exp $
 *
 * DC.C - Device context functions
 *
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <ddk/ntddvid.h>

#include <win32k/bitmaps.h>
#include <win32k/coord.h>
#include <win32k/driver.h>
#include <win32k/dc.h>
#include <win32k/print.h>
#include <win32k/region.h>
#include <win32k/gdiobj.h>
#include <win32k/pen.h>
#include <win32k/text.h>
#include "../eng/handle.h"
#include <include/inteng.h>

#define NDEBUG
#include <win32k/debug1.h>

static GDIDEVICE PrimarySurface;
static BOOL PrimarySurfaceCreated = FALSE;

/* FIXME: DCs should probably be thread safe  */

/*
 * DC device-independent Get/SetXXX functions
 * (RJJ) swiped from WINE
 */

#define DC_GET_VAL( func_type, func_name, dc_field ) \
func_type STDCALL  func_name( HDC hdc ) \
{                                   \
  func_type  ft;                    \
  PDC  dc = DC_HandleToPtr( hdc );  \
  if (!dc)                          \
  {                                 \
    return 0;                       \
  }                                 \
  ft = dc->dc_field;                \
  DC_ReleasePtr( hdc );				\
  return ft;                        \
}

/* DC_GET_VAL_EX is used to define functions returning a POINT or a SIZE. It is
 * important that the function has the right signature, for the implementation
 * we can do whatever we want.
 */
#define DC_GET_VAL_EX( func_name, ret_x, ret_y, type ) \
BOOL STDCALL  func_name( HDC hdc, LP##type pt ) \
{                                   \
  PDC  dc = DC_HandleToPtr( hdc );  \
  if (!dc)                          \
  {                                 \
    return FALSE;                   \
  }                                 \
  ((LPPOINT)pt)->x = dc->ret_x;     \
  ((LPPOINT)pt)->y = dc->ret_y;     \
  DC_ReleasePtr( hdc );				\
  return  TRUE;                     \
}

#define DC_SET_MODE( func_name, dc_field, min_val, max_val ) \
INT STDCALL  func_name( HDC hdc, INT mode ) \
{                                           \
  INT  prevMode;                            \
  PDC  dc = DC_HandleToPtr( hdc );          \
  if(!dc)                                   \
  {                                         \
    return 0;                               \
  }                                         \
  if ((mode < min_val) || (mode > max_val)) \
  {                                         \
    return 0;                               \
  }                                         \
  prevMode = dc->dc_field;                  \
  dc->dc_field = mode;                      \
  DC_ReleasePtr( hdc );						\
  return prevMode;                          \
}


//  ---------------------------------------------------------  File Statics

static void  W32kSetDCState16(HDC  hDC, HDC  hDCSave);

//  -----------------------------------------------------  Public Functions

BOOL STDCALL  W32kCancelDC(HDC  hDC)
{
  UNIMPLEMENTED;
}

HDC STDCALL  W32kCreateCompatableDC(HDC  hDC)
{
  PDC  NewDC, OrigDC = NULL;
  HBITMAP  hBitmap;
  HDC hNewDC;

  OrigDC = DC_HandleToPtr(hDC);
  if (OrigDC == NULL)
  {
    hNewDC = DC_AllocDC(L"DISPLAY");
	if( hNewDC )
		NewDC = DC_HandleToPtr( hNewDC );
  }
  else {
    /*  Allocate a new DC based on the original DC's device  */
    hNewDC = DC_AllocDC(OrigDC->DriverName);
	if( hNewDC )
		NewDC = DC_HandleToPtr( hNewDC );
  }

  if (NewDC == NULL)
  {
    return  NULL;
  }

  /* Copy information from original DC to new DC  */
  NewDC->hSelf = NewDC;

  /* FIXME: Should this DC request its own PDEV?  */
  if(OrigDC == NULL) {
  } else {
    NewDC->PDev = OrigDC->PDev;
    NewDC->DMW = OrigDC->DMW;
    memcpy(NewDC->FillPatternSurfaces,
           OrigDC->FillPatternSurfaces,
           sizeof OrigDC->FillPatternSurfaces);
    NewDC->GDIInfo = OrigDC->GDIInfo;
    NewDC->DevInfo = OrigDC->DevInfo;
  }

  /* DriverName is copied in the AllocDC routine  */
  if(OrigDC == NULL) {
    NewDC->DeviceDriver = DRIVER_FindMPDriver(NewDC->DriverName);
  } else {
    NewDC->DeviceDriver = OrigDC->DeviceDriver;
    NewDC->wndOrgX = OrigDC->wndOrgX;
    NewDC->wndOrgY = OrigDC->wndOrgY;
    NewDC->wndExtX = OrigDC->wndExtX;
    NewDC->wndExtY = OrigDC->wndExtY;
    NewDC->vportOrgX = OrigDC->vportOrgX;
    NewDC->vportOrgY = OrigDC->vportOrgY;
    NewDC->vportExtX = OrigDC->vportExtX;
    NewDC->vportExtY = OrigDC->vportExtY;
  }

  /* Create default bitmap */
  if (!(hBitmap = W32kCreateBitmap( 1, 1, 1, 1, NULL )))
  {
    DC_ReleasePtr( hNewDC );
    DC_FreeDC( hNewDC );
    return NULL;
  }
  NewDC->w.flags        = DC_MEMORY;
  NewDC->w.bitsPerPixel = 1;
  NewDC->w.hBitmap      = hBitmap;
  NewDC->w.hFirstBitmap = hBitmap;
  NewDC->Surface = BitmapToSurf(BITMAPOBJ_HandleToPtr(hBitmap));

  if(OrigDC != NULL)
  {
    NewDC->w.hPalette = OrigDC->w.hPalette;
    NewDC->w.textColor = OrigDC->w.textColor;
    NewDC->w.textAlign = OrigDC->w.textAlign;
  }
  DC_ReleasePtr( hDC );
  DC_ReleasePtr( hNewDC );
  DC_InitDC(hNewDC);

  return  hNewDC;
}

static BOOL STDCALL FindDriverFileNames(PUNICODE_STRING DriverFileNames)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  UNICODE_STRING RegistryPath;
  NTSTATUS Status;

  RtlInitUnicodeString(&RegistryPath, NULL);
  RtlZeroMemory(QueryTable, sizeof(QueryTable));
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[0].Name = L"\\Device\\Video0";
  QueryTable[0].EntryContext = &RegistryPath;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_DEVICEMAP,
                                  L"VIDEO",
                                  QueryTable,
                                  NULL,
                                  NULL);
  if (! NT_SUCCESS(Status))
  {
    DPRINT1("No \\Device\\Video0 value in DEVICEMAP\\VIDEO found\n");
    return FALSE;
  }

  DPRINT("RegistryPath %S\n", RegistryPath.Buffer);

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


BOOL STDCALL W32kCreatePrimarySurface(LPCWSTR Driver,
				      LPCWSTR Device)
{
  PGD_ENABLEDRIVER GDEnableDriver;
  HANDLE DeviceDriver;
  DRVENABLEDATA DED;
  PSURFOBJ SurfObj;
  UNICODE_STRING DriverFileNames;
  PWSTR CurrentName;
  BOOL GotDriver;

  /*  Open the miniport driver  */
  if ((DeviceDriver = DRIVER_FindMPDriver(Driver)) == NULL)
  {
    DPRINT("FindMPDriver failed\n");
    return(FALSE);
  }

  /*  Retrieve DDI driver names from registry */
  RtlInitUnicodeString(&DriverFileNames, NULL);
  if (! FindDriverFileNames(&DriverFileNames))
  {
    DPRINT("FindDriverFileNames failed\n");
    return(FALSE);
  }

  /* DriverFileNames may be a list of drivers in REG_SZ_MULTI format, scan all of
     them until a good one found */
  CurrentName = DriverFileNames.Buffer;
  GotDriver = FALSE;
  while (! GotDriver && CurrentName < DriverFileNames.Buffer + DriverFileNames.Length)
  {
    /*  Get the DDI driver's entry point  */
    GDEnableDriver = DRIVER_FindDDIDriver(CurrentName);
    if (NULL == GDEnableDriver)
    {
      DPRINT("FindDDIDriver failed for %S\n", CurrentName);
    }
    else
    {
      /*  Call DDI driver's EnableDriver function  */
      RtlZeroMemory(&DED, sizeof(DED));

      if (!GDEnableDriver(DDI_DRIVER_VERSION, sizeof(DED), &DED))
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
             CurrentName < DriverFileNames.Buffer + DriverFileNames.Length)
      {
	CurrentName++;
      }
      if (CurrentName < DriverFileNames.Buffer + DriverFileNames.Length)
      {
        CurrentName++;
      }
    }
  }
  RtlFreeUnicodeString(&DriverFileNames);
  if (! GotDriver)
  {
    DPRINT("No suitable driver found\n");
    return FALSE;
  }

  DPRINT("Display driver %S loaded\n", DriverName);

  DPRINT("Building DDI Functions\n");

  /*  Construct DDI driver function dispatch table  */
  if (!DRIVER_BuildDDIFunctions(&DED, &PrimarySurface.DriverFunctions))
  {
    DPRINT("BuildDDIFunctions failed\n");
    return(FALSE);
  }

  /*  Allocate a phyical device handle from the driver  */
  if (Device != NULL)
  {
    DPRINT("Device in u: %u\n", Device);
//    wcsncpy(NewDC->DMW.dmDeviceName, Device, DMMAXDEVICENAME); FIXME: this crashes everything?
  }

  DPRINT("Enabling PDev\n");

  PrimarySurface.PDev =
    PrimarySurface.DriverFunctions.EnablePDev(&PrimarySurface.DMW,
					     L"",
					     HS_DDI_MAX,
					     PrimarySurface.FillPatterns,
					     sizeof(PrimarySurface.GDIInfo),
					     (ULONG *) &PrimarySurface.GDIInfo,
					     sizeof(PrimarySurface.DevInfo),
					     &PrimarySurface.DevInfo,
					     NULL,
					     L"",
					     DeviceDriver);
  if (PrimarySurface.PDev == NULL)
  {
    DPRINT("DrvEnablePDEV failed\n");
    return(FALSE);
  }

  DPRINT("calling completePDev\n");

  /*  Complete initialization of the physical device  */
  PrimarySurface.DriverFunctions.CompletePDev(PrimarySurface.PDev,
					      &PrimarySurface);

  DPRINT("calling DRIVER_ReferenceDriver\n");

  DRIVER_ReferenceDriver (Driver);

  DPRINT("calling EnableSurface\n");

  /*  Enable the drawing surface  */
  PrimarySurface.Handle =
    PrimarySurface.DriverFunctions.EnableSurface(PrimarySurface.PDev);

  SurfObj = (PSURFOBJ)AccessUserObject((ULONG) PrimarySurface.Handle);
  SurfObj->dhpdev = PrimarySurface.PDev;

  return TRUE;
}

HDC STDCALL  W32kCreateDC(LPCWSTR  Driver,
                  LPCWSTR  Device,
                  LPCWSTR  Output,
                  CONST PDEVMODEW  InitData)
{
  HDC      hNewDC;
  PDC      NewDC;
  HDC      hDC = NULL;
  PSURFGDI SurfGDI;

  /*  Check for existing DC object  */
  if ((hNewDC = DC_FindOpenDC(Driver)) != NULL)
  {
    hDC = hNewDC;
    return  W32kCreateCompatableDC(hDC);
  }

  DPRINT("NAME: %S\n", Driver); // FIXME: Should not crash if NULL

  /*  Allocate a DC object  */
  if ((hNewDC = DC_AllocDC(Driver)) == NULL)
  {
    return  NULL;
  }

  NewDC = DC_HandleToPtr( hNewDC );
  ASSERT( NewDC );

  if (!PrimarySurfaceCreated)
    {
      if (!W32kCreatePrimarySurface(Driver, Device))
	{
	  DC_ReleasePtr( hNewDC );
	  DC_FreeDC(hNewDC);
	  return  NULL;
	}
    }
  PrimarySurfaceCreated = TRUE;
  NewDC->DMW = PrimarySurface.DMW;
  NewDC->DevInfo = &PrimarySurface.DevInfo;
  NewDC->GDIInfo = &PrimarySurface.GDIInfo;
  memcpy(NewDC->FillPatternSurfaces, PrimarySurface.FillPatterns,
	 sizeof(NewDC->FillPatternSurfaces));
  NewDC->PDev = PrimarySurface.PDev;
  NewDC->Surface = PrimarySurface.Handle;
  NewDC->DriverFunctions = PrimarySurface.DriverFunctions;

  NewDC->DMW.dmSize = sizeof(NewDC->DMW);
  NewDC->DMW.dmFields = 0x000fc000;

  /* FIXME: get mode selection information from somewhere  */

  NewDC->DMW.dmLogPixels = 96;
  SurfGDI = (PSURFGDI)AccessInternalObject((ULONG) PrimarySurface.Handle);
  NewDC->DMW.dmBitsPerPel = SurfGDI->BitsPerPixel;
  NewDC->DMW.dmPelsWidth = SurfGDI->SurfObj.sizlBitmap.cx;
  NewDC->DMW.dmPelsHeight = SurfGDI->SurfObj.sizlBitmap.cy;
  NewDC->DMW.dmDisplayFlags = 0;
  NewDC->DMW.dmDisplayFrequency = 0;

  NewDC->w.bitsPerPixel = SurfGDI->BitsPerPixel; // FIXME: set this here??

  NewDC->w.hPalette = NewDC->DevInfo->hpalDefault;

  DPRINT("Bits per pel: %u\n", NewDC->w.bitsPerPixel);

  NewDC->w.hVisRgn = W32kCreateRectRgn(0, 0, SurfGDI->SurfObj.sizlBitmap.cx,
                                       SurfGDI->SurfObj.sizlBitmap.cy);
  DC_ReleasePtr( hNewDC );

  /*  Initialize the DC state  */
  DC_InitDC(hNewDC);
  W32kSetTextColor(hNewDC, RGB(0, 0, 0));
  W32kSetTextAlign(hNewDC, TA_TOP);

  return hNewDC;
}

HDC STDCALL W32kCreateIC(LPCWSTR  Driver,
                         LPCWSTR  Device,
                         LPCWSTR  Output,
                         CONST PDEVMODEW  DevMode)
{
  /* FIXME: this should probably do something else...  */
  return  W32kCreateDC(Driver, Device, Output, DevMode);
}

BOOL STDCALL W32kDeleteDC(HDC  DCHandle)
{
  PDC  DCToDelete;

  DCToDelete = DC_HandleToPtr(DCHandle);
  if (DCToDelete == NULL)
    {
      return  FALSE;
    }
  DPRINT( "Deleting DC\n" );
  if ((!(DCToDelete->w.flags & DC_MEMORY))) // Don't reset the display if its a memory DC
  {
    if (!DRIVER_UnreferenceDriver (DCToDelete->DriverName))
    {
      DPRINT( "No more references to driver, reseting display\n" );
      DCToDelete->DriverFunctions.DisableSurface(DCToDelete->PDev);
      CHECKPOINT;
      DCToDelete->DriverFunctions.AssertMode( DCToDelete->PDev, FALSE );
      CHECKPOINT;
      DCToDelete->DriverFunctions.DisablePDev(DCToDelete->PDev);
	  PrimarySurfaceCreated = FALSE;
	}
  }
  CHECKPOINT;
  /*  First delete all saved DCs  */
  while (DCToDelete->saveLevel)
  {
    PDC  savedDC;
    HDC  savedHDC;

    savedHDC = DC_GetNextDC (DCToDelete);
    savedDC = DC_HandleToPtr (savedHDC);
    if (savedDC == NULL)
    {
      break;
    }
    DC_SetNextDC (DCToDelete, DC_GetNextDC (savedDC));
    DCToDelete->saveLevel--;
	DC_ReleasePtr( savedHDC );
    W32kDeleteDC (savedHDC);
  }

  /*  Free GDI resources allocated to this DC  */
  if (!(DCToDelete->w.flags & DC_SAVED))
  {
    /*
    W32kSelectObject (DCHandle, STOCK_BLACK_PEN);
    W32kSelectObject (DCHandle, STOCK_WHITE_BRUSH);
    W32kSelectObject (DCHandle, STOCK_SYSTEM_FONT);
    DC_LockDC (DCHandle); W32kSelectObject does not recognize stock objects yet  */
    BITMAPOBJ_ReleasePtr(DCToDelete->w.hBitmap);
    if (DCToDelete->w.flags & DC_MEMORY)
    {
      EngDeleteSurface (DCToDelete->Surface);
      W32kDeleteObject (DCToDelete->w.hFirstBitmap);
    }
  }
  if (DCToDelete->w.hClipRgn)
  {
    W32kDeleteObject (DCToDelete->w.hClipRgn);
  }
  if (DCToDelete->w.hVisRgn)
  {
    W32kDeleteObject (DCToDelete->w.hVisRgn);
  }
  if (DCToDelete->w.hGCClipRgn)
  {
    W32kDeleteObject (DCToDelete->w.hGCClipRgn);
  }
#if 0 /* FIXME */
  PATH_DestroyGdiPath (&DCToDelete->w.path);
#endif
  DC_ReleasePtr( DCToDelete );
  DC_FreeDC (DCToDelete);

  return TRUE;
}

INT STDCALL W32kDrawEscape(HDC  hDC,
                    INT  nEscape,
                    INT  cbInput,
                    LPCSTR  lpszInData)
{
  UNIMPLEMENTED;
}

INT STDCALL W32kEnumObjects(HDC  hDC,
                     INT  ObjectType,
                     GOBJENUMPROC  ObjectFunc,
                     LPARAM  lParam)
{
  UNIMPLEMENTED;
}

DC_GET_VAL( COLORREF, W32kGetBkColor, w.backgroundColor )
DC_GET_VAL( INT, W32kGetBkMode, w.backgroundMode )
DC_GET_VAL_EX( W32kGetBrushOrgEx, w.brushOrgX, w.brushOrgY, POINT )
DC_GET_VAL( HRGN, W32kGetClipRgn, w.hClipRgn )

HGDIOBJ STDCALL W32kGetCurrentObject(HDC  hDC,
                              UINT  ObjectType)
{
  UNIMPLEMENTED;
}

DC_GET_VAL_EX( W32kGetCurrentPositionEx, w.CursPosX, w.CursPosY, POINT )

BOOL STDCALL W32kGetDCOrgEx(HDC  hDC,
                     LPPOINT  Point)
{
  PDC dc;

  if (!Point)
  {
    return FALSE;
  }
  dc = DC_HandleToPtr(hDC);
  if (dc == NULL)
  {
    return FALSE;
  }

  Point->x = Point->y = 0;

  Point->x += dc->w.DCOrgX;
  Point->y += dc->w.DCOrgY;
  DC_ReleasePtr( hDC );
  return  TRUE;
}

HDC STDCALL W32kGetDCState16(HDC  hDC)
{
  PDC  newdc, dc;
  HDC hnewdc;

  dc = DC_HandleToPtr(hDC);
  if (dc == NULL)
  {
    return 0;
  }

  hnewdc = DC_AllocDC(NULL);
  if (hnewdc == NULL)
  {
	DC_ReleasePtr( hDC );
    return 0;
  }
  newdc = DC_HandleToPtr( hnewdc );
  ASSERT( newdc );

  newdc->w.flags            = dc->w.flags | DC_SAVED;
  newdc->w.hPen             = dc->w.hPen;
  newdc->w.hBrush           = dc->w.hBrush;
  newdc->w.hFont            = dc->w.hFont;
  newdc->w.hBitmap          = dc->w.hBitmap;
  newdc->w.hFirstBitmap     = dc->w.hFirstBitmap;
#if 0
  newdc->w.hDevice          = dc->w.hDevice;
  newdc->w.hPalette         = dc->w.hPalette;
#endif
  newdc->w.totalExtent      = dc->w.totalExtent;
  newdc->w.bitsPerPixel     = dc->w.bitsPerPixel;
  newdc->w.ROPmode          = dc->w.ROPmode;
  newdc->w.polyFillMode     = dc->w.polyFillMode;
  newdc->w.stretchBltMode   = dc->w.stretchBltMode;
  newdc->w.relAbsMode       = dc->w.relAbsMode;
  newdc->w.backgroundMode   = dc->w.backgroundMode;
  newdc->w.backgroundColor  = dc->w.backgroundColor;
  newdc->w.textColor        = dc->w.textColor;
  newdc->w.brushOrgX        = dc->w.brushOrgX;
  newdc->w.brushOrgY        = dc->w.brushOrgY;
  newdc->w.textAlign        = dc->w.textAlign;
  newdc->w.charExtra        = dc->w.charExtra;
  newdc->w.breakTotalExtra  = dc->w.breakTotalExtra;
  newdc->w.breakCount       = dc->w.breakCount;
  newdc->w.breakExtra       = dc->w.breakExtra;
  newdc->w.breakRem         = dc->w.breakRem;
  newdc->w.MapMode          = dc->w.MapMode;
  newdc->w.GraphicsMode     = dc->w.GraphicsMode;
#if 0
  /* Apparently, the DC origin is not changed by [GS]etDCState */
  newdc->w.DCOrgX           = dc->w.DCOrgX;
  newdc->w.DCOrgY           = dc->w.DCOrgY;
#endif
  newdc->w.CursPosX         = dc->w.CursPosX;
  newdc->w.CursPosY         = dc->w.CursPosY;
  newdc->w.ArcDirection     = dc->w.ArcDirection;
#if 0
  newdc->w.xformWorld2Wnd   = dc->w.xformWorld2Wnd;
  newdc->w.xformWorld2Vport = dc->w.xformWorld2Vport;
  newdc->w.xformVport2World = dc->w.xformVport2World;
  newdc->w.vport2WorldValid = dc->w.vport2WorldValid;
#endif
  newdc->wndOrgX            = dc->wndOrgX;
  newdc->wndOrgY            = dc->wndOrgY;
  newdc->wndExtX            = dc->wndExtX;
  newdc->wndExtY            = dc->wndExtY;
  newdc->vportOrgX          = dc->vportOrgX;
  newdc->vportOrgY          = dc->vportOrgY;
  newdc->vportExtX          = dc->vportExtX;
  newdc->vportExtY          = dc->vportExtY;

  newdc->hSelf = hnewdc;
  newdc->saveLevel = 0;

#if 0
  PATH_InitGdiPath( &newdc->w.path );
#endif

  /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */

#if 0
  newdc->w.hGCClipRgn = newdc->w.hVisRgn = 0;
#endif
  if (dc->w.hClipRgn)
  {
    newdc->w.hClipRgn = W32kCreateRectRgn( 0, 0, 0, 0 );
    W32kCombineRgn( newdc->w.hClipRgn, dc->w.hClipRgn, 0, RGN_COPY );
  }
  else
  {
    newdc->w.hClipRgn = 0;
  }
  DC_ReleasePtr( hnewdc );
  return  hnewdc;
}

INT STDCALL W32kGetDeviceCaps(HDC  hDC,
                       INT  Index)
{
  PDC  dc;
  INT  ret;
  POINT  pt;

  dc = DC_HandleToPtr(hDC);
  if (dc == NULL)
  {
    return 0;
  }

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
      UNIMPLEMENTED; /* FIXME */
      break;

    case COLORRES:
      UNIMPLEMENTED; /* FIXME */
      break;

    case PHYSICALWIDTH:
      if(W32kEscape(hDC, GETPHYSPAGESIZE, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.x;
      }
      else
      {
	ret = 0;
      }
      break;

    case PHYSICALHEIGHT:
      if(W32kEscape(hDC, GETPHYSPAGESIZE, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.y;
      }
      else
      {
	ret = 0;
      }
      break;

    case PHYSICALOFFSETX:
      if(W32kEscape(hDC, GETPRINTINGOFFSET, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.x;
      }
      else
      {
	ret = 0;
      }
      break;

    case PHYSICALOFFSETY:
      if(W32kEscape(hDC, GETPRINTINGOFFSET, 0, NULL, (LPVOID)&pt) > 0)
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
      if(W32kEscape(hDC, GETSCALINGFACTOR, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.x;
      }
      else
      {
	ret = 0;
      }
      break;

    case SCALINGFACTORY:
      if(W32kEscape(hDC, GETSCALINGFACTOR, 0, NULL, (LPVOID)&pt) > 0)
      {
        ret = pt.y;
      }
      else
      {
	ret = 0;
      }
      break;

    case RASTERCAPS:
      UNIMPLEMENTED; /* FIXME */
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

  DPRINT("(%04x,%d): returning %d\n", hDC, Index, ret);

  DC_ReleasePtr( hDC );
  return ret;
}

DC_GET_VAL( INT, W32kGetMapMode, w.MapMode )
DC_GET_VAL( INT, W32kGetPolyFillMode, w.polyFillMode )

INT STDCALL W32kGetObjectA(HANDLE handle, INT count, LPVOID buffer)
{
  PGDIOBJ  gdiObject;
  INT  result = 0;
  WORD  magic;

  if (!count)
    return  0;
  gdiObject = GDIOBJ_LockObj (handle, GO_MAGIC_DONTCARE);
  if (gdiObject == 0)
    return  0;

  magic = GDIOBJ_GetHandleMagic (handle);
  switch(magic)
  {
/*    case GO_PEN_MAGIC:
      result = PEN_GetObject((PENOBJ *)gdiObject, count, buffer);
      break;
    case GO_BRUSH_MAGIC:
      result = BRUSH_GetObject((BRUSHOBJ *)gdiObject, count, buffer);
      break; */
    case GO_BITMAP_MAGIC:
      result = BITMAP_GetObject((BITMAPOBJ *)gdiObject, count, buffer);
      break;
/*    case GO_FONT_MAGIC:
      result = FONT_GetObjectA((FONTOBJ *)gdiObject, count, buffer);

      // FIXME: Fix the LOGFONT structure for the stock fonts

      if ( (handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE) )
        FixStockFontSizeA(handle, count, buffer);
      break;
    case GO_PALETTE_MAGIC:
      result = PALETTE_GetObject((PALETTEOBJ *)gdiObject, count, buffer);
      break; */

    case GO_REGION_MAGIC:
    case GO_DC_MAGIC:
    case GO_DISABLED_DC_MAGIC:
    case GO_META_DC_MAGIC:
    case GO_METAFILE_MAGIC:
    case GO_METAFILE_DC_MAGIC:
    case GO_ENHMETAFILE_MAGIC:
    case GO_ENHMETAFILE_DC_MAGIC:
      // FIXME("Magic %04x not implemented\n", magic);
      break;

    default:
      DbgPrint("Invalid GDI Magic %04x\n", magic);
      break;
  }
  GDIOBJ_UnlockObj (handle, GO_MAGIC_DONTCARE);
  return  result;
}

INT STDCALL W32kGetObjectW(HANDLE handle, INT count, LPVOID buffer)
{
  PGDIOBJHDR  gdiObject;
  INT  result = 0;
  WORD  magic;

  if (!count)
    return 0;
  gdiObject = GDIOBJ_LockObj(handle, GO_MAGIC_DONTCARE);
  if (gdiObject == 0)
    return 0;

  magic = GDIOBJ_GetHandleMagic (handle);
  switch(magic)
  {
/*    case GO_PEN_MAGIC:
      result = PEN_GetObject((PENOBJ *)gdiObject, count, buffer);
      break;
    case GO_BRUSH_MAGIC:
      result = BRUSH_GetObject((BRUSHOBJ *)gdiObject, count, buffer);
       break; */
    case GO_BITMAP_MAGIC:
      result = BITMAP_GetObject((BITMAPOBJ *)gdiObject, count, buffer);
      break;
/*    case GO_FONT_MAGIC:
      result = FONT_GetObjectW((FONTOBJ *)gdiObject, count, buffer);

      // Fix the LOGFONT structure for the stock fonts

      if ( (handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE) )
      FixStockFontSizeW(handle, count, buffer);
    break;
    case GO_PALETTE_MAGIC:
      result = PALETTE_GetObject((PALETTEOBJ *)gdiObject, count, buffer);
      break; */
    default:
      // FIXME("Magic %04x not implemented\n", gdiObject->magic);
      break;
  }
  GDIOBJ_UnlockObj(handle, GO_MAGIC_DONTCARE);
  return  result;
}

INT STDCALL W32kGetObject(HANDLE handle, INT count, LPVOID buffer)
{
  return W32kGetObjectW(handle, count, buffer);
}

DWORD STDCALL W32kGetObjectType(HANDLE handle)
{
  GDIOBJHDR * ptr;
  INT result = 0;
  WORD  magic;

  ptr = GDIOBJ_LockObj(handle, GO_MAGIC_DONTCARE);
  if (ptr == 0)
    return 0;

  magic = GDIOBJ_GetHandleMagic (handle);
  switch(magic)
  {
    case GO_PEN_MAGIC:
      result = OBJ_PEN;
      break;
    case GO_BRUSH_MAGIC:
      result = OBJ_BRUSH;
      break;
    case GO_BITMAP_MAGIC:
      result = OBJ_BITMAP;
      break;
    case GO_FONT_MAGIC:
      result = OBJ_FONT;
      break;
    case GO_PALETTE_MAGIC:
      result = OBJ_PAL;
      break;
    case GO_REGION_MAGIC:
      result = OBJ_REGION;
      break;
    case GO_DC_MAGIC:
      result = OBJ_DC;
      break;
    case GO_META_DC_MAGIC:
      result = OBJ_METADC;
      break;
    case GO_METAFILE_MAGIC:
      result = OBJ_METAFILE;
      break;
    case GO_METAFILE_DC_MAGIC:
     result = OBJ_METADC;
      break;
    case GO_ENHMETAFILE_MAGIC:
      result = OBJ_ENHMETAFILE;
      break;
    case GO_ENHMETAFILE_DC_MAGIC:
      result = OBJ_ENHMETADC;
      break;
    default:
      // FIXME("Magic %04x not implemented\n", magic);
      break;
  }
  GDIOBJ_UnlockObj(handle, GO_MAGIC_DONTCARE);
  return result;
}

DC_GET_VAL( INT, W32kGetRelAbs, w.relAbsMode )
DC_GET_VAL( INT, W32kGetROP2, w.ROPmode )
DC_GET_VAL( INT, W32kGetStretchBltMode, w.stretchBltMode )
DC_GET_VAL( UINT, W32kGetTextAlign, w.textAlign )
DC_GET_VAL( COLORREF, W32kGetTextColor, w.textColor )
DC_GET_VAL_EX( W32kGetViewportExtEx, vportExtX, vportExtY, SIZE )
DC_GET_VAL_EX( W32kGetViewportOrgEx, vportOrgX, vportOrgY, POINT )
DC_GET_VAL_EX( W32kGetWindowExtEx, wndExtX, wndExtY, SIZE )
DC_GET_VAL_EX( W32kGetWindowOrgEx, wndOrgX, wndOrgY, POINT )

HDC STDCALL W32kResetDC(HDC  hDC, CONST DEVMODEW *InitData)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kRestoreDC(HDC  hDC, INT  SaveLevel)
{
  PDC  dc, dcs;
  BOOL  success;

  dc = DC_HandleToPtr(hDC);
  if(!dc)
  {
    return FALSE;
  }

  if (SaveLevel == -1)
  {
    SaveLevel = dc->saveLevel;
  }

  if ((SaveLevel < 1) || (SaveLevel > dc->saveLevel))
  {
    return FALSE;
  }

  success = TRUE;
  while (dc->saveLevel >= SaveLevel)
  {
    HDC hdcs = DC_GetNextDC (dc);

    dcs = DC_HandleToPtr (hdcs);
    if (dcs == NULL)
    {
      return FALSE;
    }
    DC_SetNextDC (dcs, DC_GetNextDC (dcs));
    if (--dc->saveLevel < SaveLevel)
      {
        W32kSetDCState16 (hDC, hdcs);
#if 0
        if (!PATH_AssignGdiPath( &dc->w.path, &dcs->w.path ))
        {
          /* FIXME: This might not be quite right, since we're
           * returning FALSE but still destroying the saved DC state */
          success = FALSE;
        }
#endif
      }
	  DC_ReleasePtr( hdcs );
    W32kDeleteDC (hdcs);
  }
  DC_ReleasePtr( hDC );
  return  success;
}

INT STDCALL W32kSaveDC(HDC  hDC)
{
  HDC  hdcs;
  PDC  dc, dcs;
  INT  ret;

  dc = DC_HandleToPtr (hDC);
  if (dc == NULL)
  {
    return 0;
  }

  if (!(hdcs = W32kGetDCState16 (hDC)))
  {
    return 0;
  }
  dcs = DC_HandleToPtr (hdcs);

#if 0
    /* Copy path. The reason why path saving / restoring is in SaveDC/
     * RestoreDC and not in GetDCState/SetDCState is that the ...DCState
     * functions are only in Win16 (which doesn't have paths) and that
     * SetDCState doesn't allow us to signal an error (which can happen
     * when copying paths).
     */
  if (!PATH_AssignGdiPath (&dcs->w.path, &dc->w.path))
  {
    W32kDeleteDC (hdcs);
    return 0;
  }
#endif

  DC_SetNextDC (dcs, DC_GetNextDC (dc));
  DC_SetNextDC (dc, hdcs);
  ret = ++dc->saveLevel;
  DC_ReleasePtr( hdcs );
  DC_ReleasePtr( hDC );

  return  ret;
}

HGDIOBJ STDCALL W32kSelectObject(HDC  hDC, HGDIOBJ  hGDIObj)
{
  HGDIOBJ   objOrg;
  BITMAPOBJ *pb;
  PDC dc;
  PPENOBJ pen;
  PBRUSHOBJ brush;
  PXLATEOBJ XlateObj;
  PPALGDI PalGDI;
  WORD  objectMagic;
  COLORREF *ColorMap;
  ULONG NumColors, Index;

  if(!hDC || !hGDIObj) return NULL;

  dc = DC_HandleToPtr(hDC);
  objectMagic = GDIOBJ_GetHandleMagic (hGDIObj);
//  GdiObjHdr = hGDIObj;

  // FIXME: Get object handle from GDIObj and use it instead of GDIObj below?

  switch(objectMagic) {
    case GO_PEN_MAGIC:
      objOrg = (HGDIOBJ)dc->w.hPen;
      dc->w.hPen = hGDIObj;

      // Convert the color of the pen to the format of the DC
      PalGDI = (PPALGDI)AccessInternalObject((ULONG) dc->w.hPalette);
      if( PalGDI ){
      	XlateObj = (PXLATEOBJ)IntEngCreateXlate(PalGDI->Mode, PAL_RGB, dc->w.hPalette, NULL);
      	pen = GDIOBJ_LockObj(dc->w.hPen, GO_PEN_MAGIC);
	if( pen ){
      	  pen->logpen.lopnColor = XLATEOBJ_iXlate(XlateObj, pen->logpen.lopnColor);
	}
	GDIOBJ_UnlockObj( dc->w.hPen, GO_PEN_MAGIC);
	EngDeleteXlate(XlateObj);
      }
      break;

    case GO_BRUSH_MAGIC:
      objOrg = (HGDIOBJ)dc->w.hBrush;
      dc->w.hBrush = (HBRUSH) hGDIObj;

      // Convert the color of the brush to the format of the DC
      PalGDI = (PPALGDI)AccessInternalObject((ULONG) dc->w.hPalette);
      if( PalGDI ){
      	XlateObj = (PXLATEOBJ)IntEngCreateXlate(PalGDI->Mode, PAL_RGB, dc->w.hPalette, NULL);
      	brush = GDIOBJ_LockObj(dc->w.hBrush, GO_BRUSH_MAGIC);
	if( brush ){
      	  brush->iSolidColor = XLATEOBJ_iXlate(XlateObj, brush->logbrush.lbColor);
	}
	GDIOBJ_UnlockObj( dc->w.hBrush, GO_BRUSH_MAGIC);
	EngDeleteXlate(XlateObj);
      }
      break;

    case GO_FONT_MAGIC:
      objOrg = (HGDIOBJ)dc->w.hFont;
      dc->w.hFont = (HFONT) hGDIObj;
      TextIntRealizeFont(dc->w.hFont);
      break;

    case GO_BITMAP_MAGIC:
      // must be memory dc to select bitmap
      if (!(dc->w.flags & DC_MEMORY)) return NULL;
      objOrg = (HGDIOBJ)dc->w.hBitmap;

      /* Release the old bitmap, lock the new one and convert it to a SURF */
      EngDeleteSurface(dc->Surface);
      BITMAPOBJ_ReleasePtr(objOrg);
      dc->w.hBitmap = hGDIObj;
      pb = BITMAPOBJ_HandleToPtr(hGDIObj);
      dc->Surface = BitmapToSurf(pb);

      // if we're working with a DIB, get the palette [fixme: only create if the selected palette is null]
      if(pb->dib)
      {
        dc->w.bitsPerPixel = pb->dib->dsBmih.biBitCount;

        if(pb->dib->dsBmih.biBitCount <= 8)
        {
          if(pb->dib->dsBmih.biBitCount == 1) { NumColors = 2; } else
          if(pb->dib->dsBmih.biBitCount == 4) { NumColors = 16; } else
          if(pb->dib->dsBmih.biBitCount == 8) { NumColors = 256; }

          ColorMap = ExAllocatePool(PagedPool, sizeof(COLORREF) * NumColors);
          for (Index = 0; Index < NumColors; Index++)
            {
            ColorMap[Index] = RGB(pb->ColorMap[Index].rgbRed,
                                  pb->ColorMap[Index].rgbGreen,
                                  pb->ColorMap[Index].rgbBlue);
          }
          dc->w.hPalette = EngCreatePalette(PAL_INDEXED, NumColors, (ULONG *) ColorMap, 0, 0, 0);
          ExFreePool(ColorMap);
        } else
        if(16 == pb->dib->dsBmih.biBitCount)
        {
          dc->w.hPalette = EngCreatePalette(PAL_BITFIELDS, pb->dib->dsBmih.biClrUsed, NULL, 0x7c00, 0x03e0, 0x001f);
        } else
        if(pb->dib->dsBmih.biBitCount >= 24)
        {
          dc->w.hPalette = EngCreatePalette(PAL_RGB, pb->dib->dsBmih.biClrUsed, NULL, 0, 0, 0);
        }
      } else {
        dc->w.bitsPerPixel = pb->bitmap.bmBitsPixel;
      }
      break;

#if UPDATEREGIONS
    case GO_REGION_MAGIC:
      /* objOrg = (HGDIOBJ)hDC->region; */
      objOrg = NULL; /* FIXME? hDC->region is destroyed below */
      SelectClipRgn(hDC, (HRGN)hGDIObj);
      break;
#endif
    default:
      return NULL;
  }
  DC_ReleasePtr( hDC );
  return objOrg;
}

DC_SET_MODE( W32kSetBkMode, w.backgroundMode, TRANSPARENT, OPAQUE )
DC_SET_MODE( W32kSetPolyFillMode, w.polyFillMode, ALTERNATE, WINDING )
// DC_SET_MODE( W32kSetRelAbs, w.relAbsMode, ABSOLUTE, RELATIVE )
DC_SET_MODE( W32kSetROP2, w.ROPmode, R2_BLACK, R2_WHITE )
DC_SET_MODE( W32kSetStretchBltMode, w.stretchBltMode, BLACKONWHITE, HALFTONE )

COLORREF STDCALL W32kSetBkColor(HDC hDC, COLORREF color)
{
  COLORREF  oldColor;
  PDC  dc = DC_HandleToPtr(hDC);

  if (!dc)
  {
    return 0x80000000;
  }

  oldColor = dc->w.backgroundColor;
  dc->w.backgroundColor = color;
  DC_ReleasePtr( hDC );
  return  oldColor;
}

static void  W32kSetDCState16(HDC  hDC, HDC  hDCSave)
{
  PDC  dc, dcs;

  dc = DC_HandleToPtr(hDC);
  if (dc == NULL)
  {
    return;
  }

  dcs = DC_HandleToPtr(hDCSave);
  if (dcs == NULL)
  {
	DC_ReleasePtr( hDC );
    return;
  }
  if (!dcs->w.flags & DC_SAVED)
  {
    return;
  }

  dc->w.flags            = dcs->w.flags & ~DC_SAVED;

  dc->w.hFirstBitmap     = dcs->w.hFirstBitmap;

#if 0
  dc->w.hDevice          = dcs->w.hDevice;
#endif

  dc->w.totalExtent      = dcs->w.totalExtent;
  dc->w.ROPmode          = dcs->w.ROPmode;
  dc->w.polyFillMode     = dcs->w.polyFillMode;
  dc->w.stretchBltMode   = dcs->w.stretchBltMode;
  dc->w.relAbsMode       = dcs->w.relAbsMode;
  dc->w.backgroundMode   = dcs->w.backgroundMode;
  dc->w.backgroundColor  = dcs->w.backgroundColor;
  dc->w.textColor        = dcs->w.textColor;
  dc->w.brushOrgX        = dcs->w.brushOrgX;
  dc->w.brushOrgY        = dcs->w.brushOrgY;
  dc->w.textAlign        = dcs->w.textAlign;
  dc->w.charExtra        = dcs->w.charExtra;
  dc->w.breakTotalExtra  = dcs->w.breakTotalExtra;
  dc->w.breakCount       = dcs->w.breakCount;
  dc->w.breakExtra       = dcs->w.breakExtra;
  dc->w.breakRem         = dcs->w.breakRem;
  dc->w.MapMode          = dcs->w.MapMode;
  dc->w.GraphicsMode     = dcs->w.GraphicsMode;
#if 0
  /* Apparently, the DC origin is not changed by [GS]etDCState */
  dc->w.DCOrgX           = dcs->w.DCOrgX;
  dc->w.DCOrgY           = dcs->w.DCOrgY;
#endif
  dc->w.CursPosX         = dcs->w.CursPosX;
  dc->w.CursPosY         = dcs->w.CursPosY;
  dc->w.ArcDirection     = dcs->w.ArcDirection;

#if 0
  dc->w.xformWorld2Wnd   = dcs->w.xformWorld2Wnd;
  dc->w.xformWorld2Vport = dcs->w.xformWorld2Vport;
  dc->w.xformVport2World = dcs->w.xformVport2World;
  dc->w.vport2WorldValid = dcs->w.vport2WorldValid;
#endif

  dc->wndOrgX            = dcs->wndOrgX;
  dc->wndOrgY            = dcs->wndOrgY;
  dc->wndExtX            = dcs->wndExtX;
  dc->wndExtY            = dcs->wndExtY;
  dc->vportOrgX          = dcs->vportOrgX;
  dc->vportOrgY          = dcs->vportOrgY;
  dc->vportExtX          = dcs->vportExtX;
  dc->vportExtY          = dcs->vportExtY;

  if (!(dc->w.flags & DC_MEMORY))
  {
    dc->w.bitsPerPixel = dcs->w.bitsPerPixel;
  }

#if 0
  if (dcs->w.hClipRgn)
  {
    if (!dc->w.hClipRgn)
    {
      dc->w.hClipRgn = W32kCreateRectRgn( 0, 0, 0, 0 );
    }
    W32kCombineRgn( dc->w.hClipRgn, dcs->w.hClipRgn, 0, RGN_COPY );
  }
  else
  {
    if (dc->w.hClipRgn)
    {
      W32kDeleteObject( dc->w.hClipRgn );
    }

    dc->w.hClipRgn = 0;
  }
  CLIPPING_UpdateGCRegion( dc );
#endif

  W32kSelectObject( hDC, dcs->w.hBitmap );
  W32kSelectObject( hDC, dcs->w.hBrush );
  W32kSelectObject( hDC, dcs->w.hFont );
  W32kSelectObject( hDC, dcs->w.hPen );
  W32kSetBkColor( hDC, dcs->w.backgroundColor);
  W32kSetTextColor( hDC, dcs->w.textColor);

#if 0
  GDISelectPalette16( hDC, dcs->w.hPalette, FALSE );
#endif

  DC_ReleasePtr( hDCSave );
  DC_ReleasePtr( hDC );
}

//  ----------------------------------------------------  Private Interface

HDC  DC_AllocDC(LPCWSTR  Driver)
{
  	PDC  NewDC;
  	HDC  hDC;

  	hDC = (HDC) GDIOBJ_AllocObj(sizeof(DC), GO_DC_MAGIC);
  	if (hDC == NULL)
  	{
  	  return  NULL;
  	}

	NewDC = (PDC) GDIOBJ_LockObj( hDC, GO_DC_MAGIC );

  	if (Driver != NULL)
  	{
  	  NewDC->DriverName = ExAllocatePool(PagedPool, (wcslen(Driver) + 1) * sizeof(WCHAR));
  	  wcscpy(NewDC->DriverName, Driver);
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

	NewDC->w.hFont = W32kGetStockObject(SYSTEM_FONT);

	GDIOBJ_UnlockObj( hDC, GO_DC_MAGIC );
  	return  hDC;
}

HDC  DC_FindOpenDC(LPCWSTR  Driver)
{
  return NULL;
}

/*!
 * Initialize some common fields in the Device Context structure.
*/
void  DC_InitDC(HDC  DCHandle)
{
//  W32kRealizeDefaultPalette(DCHandle);

  W32kSelectObject(DCHandle, W32kGetStockObject( WHITE_BRUSH ));
  W32kSelectObject(DCHandle, W32kGetStockObject( BLACK_PEN ));
  //W32kSelectObject(DCHandle, hFont);

//  CLIPPING_UpdateGCRegion(DCToInit);

}

void  DC_FreeDC(HDC  DCToFree)
{
  if (!GDIOBJ_FreeObj(DCToFree, GO_DC_MAGIC, GDIOBJFLAG_DEFAULT))
  {
    DPRINT("DC_FreeDC failed\n");
  }
}

BOOL DC_InternalDeleteDC( PDC DCToDelete )
{
	if( DCToDelete->DriverName )
		ExFreePool(DCToDelete->DriverName);
	return TRUE;
}

HDC  DC_GetNextDC (PDC pDC)
{
  return pDC->hNext;
}

void  DC_SetNextDC (PDC pDC, HDC hNextDC)
{
  pDC->hNext = hNextDC;
}

void
DC_UpdateXforms(PDC  dc)
{
  XFORM  xformWnd2Vport;
  FLOAT  scaleX, scaleY;

  /* Construct a transformation to do the window-to-viewport conversion */
  scaleX = (FLOAT)dc->vportExtX / (FLOAT)dc->wndExtX;
  scaleY = (FLOAT)dc->vportExtY / (FLOAT)dc->wndExtY;
  xformWnd2Vport.eM11 = scaleX;
  xformWnd2Vport.eM12 = 0.0;
  xformWnd2Vport.eM21 = 0.0;
  xformWnd2Vport.eM22 = scaleY;
  xformWnd2Vport.eDx  = (FLOAT)dc->vportOrgX - scaleX * (FLOAT)dc->wndOrgX;
  xformWnd2Vport.eDy  = (FLOAT)dc->vportOrgY - scaleY * (FLOAT)dc->wndOrgY;

  /* Combine with the world transformation */
  W32kCombineTransform(&dc->w.xformWorld2Vport, &dc->w.xformWorld2Wnd, &xformWnd2Vport);

  /* Create inverse of world-to-viewport transformation */
  dc->w.vport2WorldValid = DC_InvertXform(&dc->w.xformWorld2Vport, &dc->w.xformVport2World);
}

BOOL
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
