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
{ \
    PDC  dc = DC_HandleToPtr( hdc ); \
    if (!dc) return 0; \
    return dc->dc_field; \
}

/* DC_GET_VAL_EX is used to define functions returning a POINT or a SIZE. It is 
 * important that the function has the right signature, for the implementation 
 * we can do whatever we want.
 */
#define DC_GET_VAL_EX( func_name, ret_x, ret_y, type ) \
BOOL STDCALL  func_name( HDC hdc, LP##type pt ) \
{ \
    PDC  dc = DC_HandleToPtr( hdc ); \
    if (!dc) return FALSE; \
    ((LPPOINT)pt)->x = dc->ret_x; \
    ((LPPOINT)pt)->y = dc->ret_y; \
    return TRUE; \
}

#define DC_SET_MODE( func_name, dc_field, min_val, max_val ) \
INT STDCALL  func_name( HDC hdc, INT mode ) \
{ \
    INT prevMode; \
    PDC  dc = DC_HandleToPtr( hdc ); \
    if(!dc) return 0; \
    if ((mode < min_val) || (mode > max_val)) return 0; \
    prevMode = dc->dc_field; \
    dc->dc_field = mode; \
    return prevMode; \
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
  UNIMPLEMENTED;
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

BOOL STDCALL W32kRestoreDC(HDC  hDC, INT  SavedDC)
{
  UNIMPLEMENTED;
}

INT STDCALL W32kSaveDC(HDC  hDC)
{
  UNIMPLEMENTED;
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

  return  oldColor;
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

  return  oldColor;
}

//  ----------------------------------------------------  Private Interface

PDC  DCList = NULL;

PDC  DC_AllocDC(LPCWSTR  Driver)
{
  PDC  NewDC;
  
  NewDC = ExAllocatePool(NonPagedPool, sizeof(DC));
  if (NewDC == NULL)
    {
      return  NULL;
    }

  RtlZeroMemory(NewDC, sizeof(DC));
  NewDC->Type = GDI_DC_TYPE;
  NewDC->DriverName = ExAllocatePool(NonPagedPool, 
                                     wcslen(Driver) * sizeof(WCHAR));
  wcscpy(NewDC->DriverName, Driver);
  NewDC->NextDC = DCList;
  DCList = NewDC;

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

PDC  DC_FindOpenDC(LPCWSTR  Driver)
{
  PDC  DCToReturn = DCList;
  
  if (Driver == NULL)
    {
      return  NULL;
    }
  
  while (DCToReturn != NULL)
    {
      if (DCToReturn->DriverName != NULL && 
          !wcscmp(DCToReturn->DriverName, Driver))
        {
          break;
        }
      DCToReturn = DCToReturn->NextDC;
    }
  
  return DCToReturn;
}

void  DC_FreeDC(PDC  DCToFree)
{
  PDC  Tmp;
  
  if (DCList == DCToFree)
    {
      DCList = DCList->NextDC;
    }
  else
    {
      Tmp = DCList;
      while (Tmp->NextDC != NULL && Tmp->NextDC != DCToFree)
        {
          Tmp = Tmp->NextDC;
        }
      Tmp->NextDC = Tmp->NextDC->NextDC;
    }
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


