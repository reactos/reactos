/*
 * DC.C - Device context functions
 * 
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/bitmaps.h>
#include <win32k/coord.h>
#include <win32k/driver.h>
#include <win32k/dc.h>
#include <win32k/print.h>
#include <win32k/region.h>

// #define NDEBUG
#include <internal/debug.h>

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
    {                               \
      return 0;                     \
    }                               \
  ft = dc->dc_field;                \
  DC_UnlockDC(dc);                  \
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
    {                               \
      return FALSE;                 \
    }                               \
  ((LPPOINT)pt)->x = dc->ret_x;     \
  ((LPPOINT)pt)->y = dc->ret_y;     \
  DC_UnlockDC(dc);                  \
  return  TRUE;                     \
}

#define DC_SET_MODE( func_name, dc_field, min_val, max_val ) \
INT STDCALL  func_name( HDC hdc, INT mode ) \
{                                           \
  INT  prevMode;                            \
  PDC  dc = DC_HandleToPtr( hdc );          \
  if(!dc)                                   \
    {                                       \
      return 0;                             \
    }                                       \
  if ((mode < min_val) || (mode > max_val))  \
    {                                       \
      return 0;                             \
    }                                       \
    prevMode = dc->dc_field;                \
    dc->dc_field = mode;                    \
    DC_UnlockDC(hdc);                          \
    return prevMode;                        \
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
  PDC  NewDC, OrigDC;
  HBITMAP  hBitmap;

  OrigDC = DC_HandleToPtr(hDC);

  /*  Allocate a new DC based on the original DC's device  */
  NewDC = DC_AllocDC(OrigDC->DriverName);
  if (NewDC == NULL) 
    {
      DC_UnlockDC(OrigDC);
      
      return  NULL;
    }

  /* Copy information from original DC to new DC  */
  NewDC->hSelf = NewDC;

  /* FIXME: Should this DC request its own PDEV?  */
  NewDC->PDev = OrigDC->PDev;

  NewDC->DMW = OrigDC->DMW;
  memcpy(NewDC->FillPatternSurfaces, 
         OrigDC->FillPatternSurfaces,
         sizeof OrigDC->FillPatternSurfaces);
  NewDC->GDIInfo = OrigDC->GDIInfo;
  NewDC->DevInfo = OrigDC->DevInfo;

  /* FIXME: Should this DC request its own surface?  */
  NewDC->Surface = OrigDC->Surface;

  NewDC->DriverFunctions = OrigDC->DriverFunctions;
  /* DriverName is copied in the AllocDC routine  */
  NewDC->DeviceDriver = OrigDC->DeviceDriver;
  NewDC->wndOrgX = OrigDC->wndOrgX;
  NewDC->wndOrgY = OrigDC->wndOrgY;
  NewDC->wndExtX = OrigDC->wndExtX;
  NewDC->wndExtY = OrigDC->wndExtY;
  NewDC->vportOrgX = OrigDC->vportOrgX;
  NewDC->vportOrgY = OrigDC->vportOrgY;
  NewDC->vportExtX = OrigDC->vportExtX;
  NewDC->vportExtY = OrigDC->vportExtY;

  DC_InitDC(NewDC);

  /* Create default bitmap */
  if (!(hBitmap = W32kCreateBitmap( 1, 1, 1, 1, NULL )))
    {
      DC_FreeDC(NewDC);
      DC_UnlockDC(OrigDC);
      
      return NULL;
    }
  NewDC->w.flags        = DC_MEMORY;
  NewDC->w.bitsPerPixel = 1;
  NewDC->w.hBitmap      = hBitmap;
  NewDC->w.hFirstBitmap = hBitmap;

  DC_UnlockDC(NewDC);
  DC_UnlockDC(OrigDC);

  return  DC_PtrToHandle(NewDC);
}

HDC STDCALL  W32kCreateDC(LPCWSTR  Driver,
                  LPCWSTR  Device,
                  LPCWSTR  Output,
                  CONST PDEVMODEW  InitData)
{
  PGD_ENABLEDRIVER  GDEnableDriver;
  PDC  NewDC;
  HDC  hDC;
  DRVENABLEDATA  DED;
  
  /*  Check for existing DC object  */
  if ((NewDC = DC_FindOpenDC(Driver)) != NULL)
    {
      hDC = DC_PtrToHandle(NewDC);
      return  W32kCreateCompatableDC(hDC);
    }
  
  /*  Allocate a DC object  */
  if ((NewDC = DC_AllocDC(Driver)) == NULL)
    {
      return  NULL;
    }

  /*  Open the miniport driver  */
  if ((NewDC->DeviceDriver = DRIVER_FindMPDriver(Driver)) == NULL)
    {
      DbgPrint("FindMPDriver failed\n");
      goto Failure;
    }
  
  /*  Get the DDI driver's entry point  */
  if ((GDEnableDriver = DRIVER_FindDDIDriver(Driver)) == NULL)
    {
      DbgPrint("FindDDIDriver failed\n");
      goto Failure;
    }
  
  /*  Call DDI driver's EnableDriver function  */
  RtlZeroMemory(&DED, sizeof(DED));
  if (!GDEnableDriver(DDI_DRIVER_VERSION, sizeof(DED), &DED))
    {
      DbgPrint("DrvEnableDriver failed\n");
      goto Failure;
    }

  /*  Construct DDI driver function dispatch table  */
  if (!DRIVER_BuildDDIFunctions(&DED, &NewDC->DriverFunctions))
    {
      DbgPrint("BuildDDIFunctions failed\n");
      goto Failure;
    }
  
  /*  Allocate a phyical device handle from the driver  */
  if (Device != NULL)
    {
      wcsncpy(NewDC->DMW.dmDeviceName, Device, DMMAXDEVICENAME);
    }
  NewDC->DMW.dmSize = sizeof(NewDC->DMW);
  NewDC->DMW.dmFields = 0x000fc000;

  /* FIXME: get mode selection information from somewhere  */
  
  NewDC->DMW.dmLogPixels = 96;
  NewDC->DMW.dmBitsPerPel = 8;
  NewDC->DMW.dmPelsWidth = 640;
  NewDC->DMW.dmPelsHeight = 480;
  NewDC->DMW.dmDisplayFlags = 0;
  NewDC->DMW.dmDisplayFrequency = 0;
  NewDC->PDev = NewDC->DriverFunctions.EnablePDev(&NewDC->DMW,
                                                  L"",
                                                  HS_DDI_MAX,
                                                  NewDC->FillPatternSurfaces,
                                                  sizeof(NewDC->GDIInfo),
                                                  (ULONG *) &NewDC->GDIInfo,
                                                  sizeof(NewDC->DevInfo),
                                                  &NewDC->DevInfo,
                                                  NULL,
                                                  L"",
                                                  NewDC->DeviceDriver);
  if (NewDC->PDev == NULL)
    {
      DbgPrint("DrvEnablePDEV failed\n");
      goto Failure;
    }
  
  /*  Complete initialization of the physical device  */
  NewDC->DriverFunctions.CompletePDev(NewDC->PDev, NewDC);

  /*  Enable the drawing surface  */
  NewDC->Surface = NewDC->DriverFunctions.EnableSurface(NewDC->PDev);

  /*  Initialize the DC state  */
  DC_InitDC(NewDC);
  
  return  DC_PtrToHandle(NewDC);

Failure:
  DC_FreeDC(NewDC);
  return  NULL;
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
  
  UNIMPLEMENTED;

  DCToDelete = DC_HandleToPtr(DCHandle);

  /* FIXME: Verify that is is a valid handle */
  
  DCToDelete->DriverFunctions.DisableSurface(DCToDelete->PDev);
  DCToDelete->DriverFunctions.DisablePDev(DCToDelete->PDev);

  DC_FreeDC(DCToDelete);
  
  return  STATUS_SUCCESS;
}

BOOL STDCALL  W32kDeleteObject(HGDIOBJ hObject)
{
  UNIMPLEMENTED;
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
  DC * dc;

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
  DC_UnlockDC (hDC);
  
  return  TRUE;
}

HDC  W32kGetDCState16(HDC  hDC)
{
  PDC  newdc, dc;
    
  dc = DC_HandleToPtr(hDC);
  if (dc == NULL) 
    {
      return 0;
    }
  
  newdc = DC_AllocDC(NULL);
  if (newdc == NULL)
    {
      DC_UnlockDC(hDC);
      return 0;
    }

  newdc->w.flags            = dc->w.flags | DC_SAVED;
#if 0
  newdc->w.devCaps          = dc->w.devCaps;
#endif
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

  newdc->hSelf = DC_PtrToHandle(newdc);
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
  DC_UnlockDC(hDC);
  
  return  newdc->hSelf;
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

  /* Device capabilities for the printer */
  switch (Index)
    {
    case PHYSICALWIDTH:
      if(W32kEscape(hDC, GETPHYSPAGESIZE, 0, NULL, (LPVOID)&pt) > 0)
        {
          return pt.x;
        }
      break;
      
    case PHYSICALHEIGHT:
      if(W32kEscape(hDC, GETPHYSPAGESIZE, 0, NULL, (LPVOID)&pt) > 0)
        {
          return pt.y;
        }
      break;
      
    case PHYSICALOFFSETX:
      if(W32kEscape(hDC, GETPRINTINGOFFSET, 0, NULL, (LPVOID)&pt) > 0)
        {
          return pt.x;
        }
      break;
      
    case PHYSICALOFFSETY:
      if(W32kEscape(hDC, GETPRINTINGOFFSET, 0, NULL, (LPVOID)&pt) > 0)
        {
          return pt.y;
        }
      break;
      
    case SCALINGFACTORX:
      if(W32kEscape(hDC, GETSCALINGFACTOR, 0, NULL, (LPVOID)&pt) > 0)
        {
          return pt.x;
        }
      break;
      
    case SCALINGFACTORY:
      if(W32kEscape(hDC, GETSCALINGFACTOR, 0, NULL, (LPVOID)&pt) > 0)
        {
          return pt.y;
        }
      break;
    }

    if ((Index < 0) || (Index > sizeof(DEVICECAPS) - sizeof(WORD)))
    {
      DC_UnlockDC(hDC);
      return 0;
    }
    
 DPRINT("(%04x,%d): returning %d\n",
        hDC, Index, *(WORD *)(((char *)dc->w.devCaps) + Index));
  ret = *(WORD *)(((char *)dc->w.devCaps) + Index);

  DC_UnlockDC(hDC);
  return ret;
}

DC_GET_VAL( INT, W32kGetMapMode, w.MapMode )
DC_GET_VAL( INT, W32kGetPolyFillMode, w.polyFillMode )

INT STDCALL  W32kGetObject(HGDIOBJ  hGDIObj,
                           INT  BufSize,
                           LPVOID  Object)
{
  UNIMPLEMENTED;
}
 
DWORD STDCALL  W32kGetObjectType(HGDIOBJ  hGDIObj)
{
  UNIMPLEMENTED;
}

DC_GET_VAL( INT, W32kGetRelAbs, w.relAbsMode )
DC_GET_VAL( INT, W32kGetROP2, w.ROPmode )
DC_GET_VAL( INT, W32kGetStretchBltMode, w.stretchBltMode )

HGDIOBJ STDCALL  W32kGetStockObject(INT  Object)
{
  UNIMPLEMENTED;
}

DC_GET_VAL( UINT, W32kGetTextAlign, w.textAlign )
DC_GET_VAL( COLORREF, W32kGetTextColor, w.textColor )
DC_GET_VAL_EX( W32kGetViewportExtEx, vportExtX, vportExtY, SIZE )
DC_GET_VAL_EX( W32kGetViewportOrgEx, vportOrgX, vportOrgY, POINT )
DC_GET_VAL_EX( W32kGetWindowExtEx, wndExtX, wndExtY, SIZE )
DC_GET_VAL_EX( W32kGetWindowOrgEx, wndOrgX, wndOrgY, POINT )

HDC STDCALL W32kResetDC(HDC  hDC, CONST DEVMODE  *InitData)
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
      DC_UnlockDC(hDC);
      
      return FALSE;
    }
    
  success = TRUE;
  while (dc->saveLevel >= SaveLevel)
    {
      HDC hdcs = dc->header.hNext;
      
      dcs = DC_HandleToPtr(hdcs);
      if (dcs == NULL)
        {
          DC_UnlockDC(hDC);
          
          return FALSE;
        }
      dc->header.hNext = dcs->header.hNext;
      if (--dc->saveLevel < SaveLevel)
        {
          W32kSetDCState16(hDC, hdcs);
#if 0
          if (!PATH_AssignGdiPath( &dc->w.path, &dcs->w.path ))
            {
              /* FIXME: This might not be quite right, since we're
               * returning FALSE but still destroying the saved DC state */
              success = FALSE;
            }
#endif
        }
      W32kDeleteDC(hdcs);
    }
  DC_UnlockDC(hDC);
  
  return  success;
}

INT STDCALL W32kSaveDC(HDC  hDC)
{
  HDC  hdcs;
  PDC  dc, dcs;
  INT  ret;

  dc = DC_HandleToPtr(hDC);
  if (dc == NULL)
    {
      return 0;
    }

  if (!(hdcs = W32kGetDCState16(hDC)))
    {
      DC_UnlockDC(hDC);
      
      return 0;
    }
  dcs = DC_HandleToPtr(hdcs);

#if 0
    /* Copy path. The reason why path saving / restoring is in SaveDC/
     * RestoreDC and not in GetDCState/SetDCState is that the ...DCState
     * functions are only in Win16 (which doesn't have paths) and that
     * SetDCState doesn't allow us to signal an error (which can happen
     * when copying paths).
     */
  if (!PATH_AssignGdiPath(&dcs->w.path, &dc->w.path))
    {
      DC_UnlockDC(hdc);
      DC_UnlockDC(hdcs);
      W32kDeleteDC(hdcs);
      return 0;
    }
#endif
    
  dcs->header.hNext = dc->header.hNext;
  dc->header.hNext = hdcs;
  ret = ++dc->saveLevel;
  DC_UnlockDC(hdcs);
  DC_UnlockDC(hDC);

  return  ret;
}

HGDIOBJ STDCALL W32kSelectObject(HDC  hDC, HGDIOBJ  GDIObj)
{
  UNIMPLEMENTED;
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
  DC_UnlockDC(hDC);
  
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
      DC_UnlockDC(hDC);
      
      return;
    }
  if (!dcs->w.flags & DC_SAVED)
    {
      DC_UnlockDC(hDC);
      DC_UnlockDC(hDCSave);
      
      return;
    }

  dc->w.flags            = dcs->w.flags & ~DC_SAVED;

#if 0
  dc->w.devCaps          = dcs->w.devCaps;
#endif

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
  
  DC_UnlockDC(hDC);
  DC_UnlockDC(hDCSave);
}

//  ----------------------------------------------------  Private Interface

PDC  DC_AllocDC(LPCWSTR  Driver)
{
  PDC  NewDC;
  
  NewDC = (PDC) GDIOBJ_AllocObject(sizeof(DC), GO_DC_MAGIC);
  if (NewDC == NULL)
    {
      return  NULL;
    }
  if (Driver != NULL)
    {
      NewDC->DriverName = ExAllocatePool(NonPagedPool, 
                                         wcslen(Driver) * sizeof(WCHAR));
      wcscpy(NewDC->DriverName, Driver);
    }

  return  NewDC;
}

PDC  DC_FindOpenDC(LPCWSTR  Driver)
{
  /*  FIXME  */
  return  NULL;
}

void  DC_InitDC(PDC  DCToInit)
{
  HDC  DCHandle;
  
  DCHandle = DC_PtrToHandle(DCToInit);
//  W32kRealizeDefaultPalette(DCHandle);
  W32kSetTextColor(DCHandle, DCToInit->w.textColor);
  W32kSetBkColor(DCHandle, DCToInit->w.backgroundColor);
  W32kSelectObject(DCHandle, DCToInit->w.hPen);
  W32kSelectObject(DCHandle, DCToInit->w.hBrush);
  W32kSelectObject(DCHandle, DCToInit->w.hFont);
//  CLIPPING_UpdateGCRegion(DCToInit);
}

void  DC_FreeDC(PDC  DCToFree)
{
  PDC  Tmp;
  
  ExFreePool(DCToFree->DriverName);
  ExFreePool(DCToFree);
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
  xformWnd2Vport.eDx  = (FLOAT)dc->vportOrgX -
    scaleX * (FLOAT)dc->wndOrgX;
  xformWnd2Vport.eDy  = (FLOAT)dc->vportOrgY -
    scaleY * (FLOAT)dc->wndOrgY;
  
  /* Combine with the world transformation */
  W32kCombineTransform(&dc->w.xformWorld2Vport, 
                       &dc->w.xformWorld2Wnd,
                       &xformWnd2Vport );
  
  /* Create inverse of world-to-viewport transformation */
  dc->w.vport2WorldValid = DC_InvertXform(&dc->w.xformWorld2Vport,
                                          &dc->w.xformVport2World);
}

BOOL 
DC_InvertXform(const XFORM *xformSrc, 
               XFORM *xformDest)
{
  FLOAT  determinant;
  
  determinant = xformSrc->eM11*xformSrc->eM22 -
    xformSrc->eM12*xformSrc->eM21;
  if (determinant > -1e-12 && determinant < 1e-12)
    {
      return  FALSE;
    }
  
  xformDest->eM11 =  xformSrc->eM22 / determinant;
  xformDest->eM12 = -xformSrc->eM12 / determinant;
  xformDest->eM21 = -xformSrc->eM21 / determinant;
  xformDest->eM22 =  xformSrc->eM11 / determinant;
  xformDest->eDx  = -xformSrc->eDx * xformDest->eM11 -
    xformSrc->eDy * xformDest->eM21;
  xformDest->eDy  = -xformSrc->eDx * xformDest->eM12 -
    xformSrc->eDy * xformDest->eM22;
  
  return  TRUE;
}


