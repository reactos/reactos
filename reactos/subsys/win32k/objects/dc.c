/*
 * DC.C - Device context functions
 * 
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/driver.h>
#include <win32k/dc.h>

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
    DC_Unlock(dc);                          \
    return prevMode;                        \
}

//  ---------------------------------------------------------  File Statics


//  -----------------------------------------------------  Public Functions

BOOL STDCALL  W32kCancelDC(HDC  hDC)
{
  UNIMPLEMENTED;
}

HDC STDCALL  W32kCreateCompatableDC(HDC  hDC)
{
  UNIMPLEMENTED;

  PDC  NewDC, OrigDC;
  HBITMAP  hBitmap;

  OrigDC = DC_HandleToPtr(hDC);

  NewDC = DC_AllocDC(OrigDC->Driver);
  if (NewDC == NULL) 
    {
      DC_UnlockDC(OrigDC);
      
      return  NULL;
    }
  if ((NewDC->DeviceDriver = DRIVER_FindMPDriver(Driver)) == NULL)
    {
      DC_FreeDC(NewDC);
      DC_UnlockDC(OrigDC);
      
      return  NULL;
    }
  if ((GDEnableDriver = DRIVER_FindDDIDriver(Driver)) == NULL)
    {
      DC_FreeDC(NewDC);
      DC_UnlockDC(OrigDC);
      
      return  NULL;
    }

  /* Create default bitmap */
  if (!(hBitmap = CreateBitmap( 1, 1, 1, 1, NULL )))
    {
      DC_FreeDC(NewDC);
      DC_UnlockDC(OrigDC);
      
      return NULL;
    }
  dc->w.flags        = DC_MEMORY;
  dc->w.bitsPerPixel = 1;
  dc->w.hBitmap      = hbitmap;
  dc->w.hFirstBitmap = hbitmap;

  /* Copy the driver-specific physical device info into
   * the new DC. The driver may use this read-only info
   * while creating the compatible DC below. */
  if (origDC)
    {
      dc->physDev = origDC->physDev;
    }

  DC_InitDC(NewDC);
  DC_Unlock(NewDC);
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
  DRVENABLEDATA  DED;
  
  /*  Check for existing DC object  */
  if ((NewDC = DC_FindOpenDC(Driver)) != NULL)
    {
      return  DC_PtrToHandle(NewDC);
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
                                                  &NewDC->GDIInfo,
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
  return  W32kCreateDC(Driver, Device, Output DevMode);
}

BOOL STDCALL W32kDeleteDC(HDC  DCHandle)
{
  PDC  DCToDelete;
  
  UNIMPLEMENTED;

  DCToDelete = DC_HandleToPtr(DCHandle);

  /* FIXME: Call driver shutdown/deallocate routines here  */
  
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
  UNIMPLEMENTED;
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
      DC_UnlockDC(dc);
      return 0;
    }

  newdc->w.flags            = dc->w.flags | DC_SAVED;
  newdc->w.devCaps          = dc->w.devCaps;
  newdc->w.hPen             = dc->w.hPen;       
  newdc->w.hBrush           = dc->w.hBrush;     
  newdc->w.hFont            = dc->w.hFont;      
  newdc->w.hBitmap          = dc->w.hBitmap;    
  newdc->w.hFirstBitmap     = dc->w.hFirstBitmap;
  newdc->w.hDevice          = dc->w.hDevice;
  newdc->w.hPalette         = dc->w.hPalette;   
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
  newdc->w.xformWorld2Wnd   = dc->w.xformWorld2Wnd;
  newdc->w.xformWorld2Vport = dc->w.xformWorld2Vport;
  newdc->w.xformVport2World = dc->w.xformVport2World;
  newdc->w.vport2WorldValid = dc->w.vport2WorldValid;
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

  PATH_InitGdiPath( &newdc->w.path );
    
  /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */

  newdc->w.hGCClipRgn = newdc->w.hVisRgn = 0;
  if (dc->w.hClipRgn)
    {
      newdc->w.hClipRgn = CreateRectRgn( 0, 0, 0, 0 );
      CombineRgn( newdc->w.hClipRgn, dc->w.hClipRgn, 0, RGN_COPY );
    }
  else
    {
      newdc->w.hClipRgn = 0;
    }
  DC_UnlockDC(dc);
  
  return  newdc->hSelf;
}

INT STDCALL W32kGetDeviceCaps(HDC  hDC,
                       INT  Index)
{
  UNIMPLEMENTED;
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
      DC_UnlockDC(dc);
      
      return FALSE;
    }
    
  success = TRUE;
  while (dc->saveLevel >= SaveLevel)
    {
      HDC hdcs = dc->header.hNext;
      
      dcs = DC_HandleToPtr(hdcs);
      if (dcs == NULL)
        {
          DC_UnlockDC(dc);
          
          return FALSE;
        }
      dc->header.hNext = dcs->header.hNext;
      if (--dc->saveLevel < level)
        {
          W32kSetDCState16(hdc, hdcs);
          if (!PATH_AssignGdiPath( &dc->w.path, &dcs->w.path ))
            {
              /* FIXME: This might not be quite right, since we're
               * returning FALSE but still destroying the saved DC state */
              success = FALSE;
            }
        }
      W32kDeleteDC(hdcs);
    }
  DC_UnlockDC(hdc);
  
  return  success;
}

INT STDCALL W32kSaveDC(HDC  hDC)
{
  HDC  hdcs;
  DC  dc, dcs;
  INT  ret;

  dc = DC_HandleToPtr(hdc);
  if (dc == NULL)
    {
      return 0;
    }

  if (!(hdcs = W32kGetDCState16(hdc)))
    {
      DC_UnlockDC(dc);
      
      return 0;
    }
  dcs = DC_HandleToPtr(hdcs);

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
    
  dcs->header.hNext = dc->header.hNext;
  dc->header.hNext = hdcs;
  ret = ++dc->saveLevel;
  DC_UnlockDC(hdcs);
  DC_UnlockDC(hdc);

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
  DC_UnlockDC(dc);
  
  return  oldColor;
}

void  W32kSetDCState16(HDC  hDC, HDC  hDCSave)
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
      DC_UnlockDC(dc);
      
      return;
    }
  if (!dcs->w.flags & DC_SAVED)
    {
      DC_UnlockDC(dc);
      DC_UnlockDC(dcs);
      
      return;
    }

  dc->w.flags            = dcs->w.flags & ~DC_SAVED;
  dc->w.devCaps          = dcs->w.devCaps;
  dc->w.hFirstBitmap     = dcs->w.hFirstBitmap;
  dc->w.hDevice          = dcs->w.hDevice;
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
  dc->w.xformWorld2Wnd   = dcs->w.xformWorld2Wnd;
  dc->w.xformWorld2Vport = dcs->w.xformWorld2Vport;
  dc->w.xformVport2World = dcs->w.xformVport2World;
  dc->w.vport2WorldValid = dcs->w.vport2WorldValid;
  
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
  
  W32kSelectObject( hdc, dcs->w.hBitmap );
  W32kSelectObject( hdc, dcs->w.hBrush );
  W32kSelectObject( hdc, dcs->w.hFont );
  W32kSelectObject( hdc, dcs->w.hPen );
  W32kSetBkColor( hdc, dcs->w.backgroundColor);
  W32kSetTextColor( hdc, dcs->w.textColor);

  GDISelectPalette16( hdc, dcs->w.hPalette, FALSE );
  
  DC_UnlockDC(dc);
  DC_UnlockDC(dcs);
}

COLORREF STDCALL  W32kSetTextColor(HDC hDC, COLORREF color)
{
  COLORREF  oldColor;
  PDC  dc = DC_HandleToPtr(hDC);
  
  if (!dc) 
    {
      return 0x80000000;
    }

  oldColor = dc->w.textColor;
  dc->w.textColor = color;
  DC_UnlockDC(dc);

  return  oldColor;
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

HDC  DC_PtrToHandle(PDC  pDC)
{
  /* FIXME: this should actually return a handle obtained from the pointer  */
  return (HDC) pDC;
}


PDC  DC_HandleToPtr(HDC  hDC)
{
  /* FIXME: this should actually return a pointer obtained from the handle  */
  return (PDC) hDC;
}

BOOL DC_LockDC(HDC  hDC)
{
  /* FIXME */
  return  TRUE;
}

BOOL DC_UnlockDC(HDC  hDC)
{
  /* FIXME */
  return  TRUE;
}


