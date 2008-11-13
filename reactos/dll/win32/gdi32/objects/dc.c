#include "precomp.h"

#define NDEBUG
#include <debug.h>

HGDIOBJ stock_objects[NB_STOCK_OBJECTS]; // temp location.

HDC
FASTCALL
IntCreateDICW ( LPCWSTR   lpwszDriver,
                LPCWSTR   lpwszDevice,
                LPCWSTR   lpwszOutput,
                PDEVMODEW lpInitData,
                ULONG     iType )
{
 UNICODE_STRING Device, Output;
 HDC hDC = NULL;
 BOOL Display = FALSE, Default = TRUE;
 ULONG UMdhpdev = 0;

 HANDLE hspool = NULL;

 if ((!lpwszDevice) && (!lpwszDriver))
 {
     Default = FALSE;  // Ask Win32k to set Default device.
     Display = TRUE;   // Most likely to be DISPLAY.
 }
 else
 {
    if (lpwszDevice) // First
    {
      if (!_wcsnicmp(lpwszDevice, L"\\\\.\\DISPLAY",11)) Display = TRUE;
      RtlInitUnicodeString(&Device, lpwszDevice);
    }
    else
    {
      if (lpwszDriver) // Second
      {
        if ((!_wcsnicmp(lpwszDriver, L"DISPLAY",7)) ||
              (!_wcsnicmp(lpwszDriver, L"\\\\.\\DISPLAY",11))) Display = TRUE;
        RtlInitUnicodeString(&Device, lpwszDriver);
      }
    }
 }

 if (lpwszOutput) RtlInitUnicodeString(&Output, lpwszOutput);

 if (!Display)
 {
    //Handle Print device or something else.
    DPRINT1("Not a DISPLAY device! %wZ\n", &Device);
 }

 hDC = NtGdiOpenDCW( (Default ? &Device : NULL),
                     (PDEVMODEW) lpInitData,
                     (lpwszOutput ? &Output : NULL),
                      iType,             // DCW 0 and ICW 1.
                      hspool,
                     (PVOID) NULL,       // NULL for now.
                     (PVOID) &UMdhpdev );
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    PDC_ATTR Dc_Attr;
    PLDC pLDC;

    GdiGetHandleUserData((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr);

    pLDC = LocalAlloc(LMEM_ZEROINIT, sizeof(LDC));

    Dc_Attr->pvLDC = pLDC;
    pLDC->hDC = hDC;
    pLDC->iType = LDC_LDC; // 1 (init) local DC, 2 EMF LDC
    DbgPrint("DC_ATTR Allocated -> 0x%x\n",Dc_Attr);
 }
#endif
 return hDC;
}


/*
 * @implemented
 */
HDC
STDCALL
CreateCompatibleDC ( HDC hdc)
{
 HDC rhDC;
// PDC_ATTR Dc_Attr;

 rhDC = NtGdiCreateCompatibleDC(hdc);
#if 0
 if ( hdc && rhDC)
 {
    if (GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr))
    {
       if ( Dc_Attr->pvLIcm ) IcmCompatibleDC(rhDC, hdc, Dc_Attr);
    }
 }
#endif
 return rhDC;
}

/*
 * @implemented
 */
HDC
STDCALL
CreateDCA (
	LPCSTR		lpszDriver,
	LPCSTR		lpszDevice,
	LPCSTR		lpszOutput,
	CONST DEVMODEA	* lpdvmInit
	)
{
 ANSI_STRING DriverA, DeviceA, OutputA;
 UNICODE_STRING DriverU, DeviceU, OutputU;
 LPDEVMODEW dvmInitW = NULL;
 HDC hDC;

/*
 * If needed, convert to Unicode
 * any string parameter.
 */

 if (NULL != lpszDriver)
 {
    RtlInitAnsiString(&DriverA, (LPSTR)lpszDriver);
    RtlAnsiStringToUnicodeString(&DriverU, &DriverA, TRUE);
 } else
   DriverU.Buffer = NULL;
 if (NULL != lpszDevice)
 {
   RtlInitAnsiString(&DeviceA, (LPSTR)lpszDevice);
   RtlAnsiStringToUnicodeString(&DeviceU, &DeviceA, TRUE);
 } else
   DeviceU.Buffer = NULL;
 if (NULL != lpszOutput)
 {
   RtlInitAnsiString(&OutputA, (LPSTR)lpszOutput);
   RtlAnsiStringToUnicodeString(&OutputU, &OutputA, TRUE);
 } else
   OutputU.Buffer = NULL;

 if ( lpdvmInit )
   dvmInitW = GdiConvertToDevmodeW((LPDEVMODEA)lpdvmInit);

 hDC = IntCreateDICW ( DriverU.Buffer,
                       DeviceU.Buffer,
                       OutputU.Buffer,
                            lpdvmInit ? dvmInitW : NULL,
                                    0 );
 HEAP_free (dvmInitW);
/*
 * Free Unicode parameters.
 */
 RtlFreeUnicodeString(&DriverU);
 RtlFreeUnicodeString(&DeviceU);
 RtlFreeUnicodeString(&OutputU);

/*
 * Return the possible DC handle.
 */
 return hDC;
}


/*
 * @implemented
 */
HDC
STDCALL
CreateDCW (
	LPCWSTR		lpwszDriver,
	LPCWSTR		lpwszDevice,
	LPCWSTR		lpwszOutput,
	CONST DEVMODEW	*lpInitData
	)
{

 return  IntCreateDICW ( lpwszDriver,
                         lpwszDevice,
                         lpwszOutput,
              (PDEVMODEW) lpInitData,
                                   0 );
}


/*
 * @implemented
 */
HDC
STDCALL
CreateICW(
	LPCWSTR		lpszDriver,
	LPCWSTR		lpszDevice,
	LPCWSTR		lpszOutput,
	CONST DEVMODEW *lpdvmInit
	)
{
 return IntCreateDICW ( lpszDriver,
                        lpszDevice,
                        lpszOutput,
             (PDEVMODEW) lpdvmInit,
                                 1 );
}


/*
 * @implemented
 */
HDC
STDCALL
CreateICA(
	LPCSTR		lpszDriver,
	LPCSTR		lpszDevice,
	LPCSTR		lpszOutput,
	CONST DEVMODEA *lpdvmInit
	)
{
 NTSTATUS Status;
 LPWSTR lpszDriverW, lpszDeviceW, lpszOutputW;
 LPDEVMODEW dvmInitW = NULL;
 HDC rc = 0;

 Status = HEAP_strdupA2W ( &lpszDriverW, lpszDriver );
 if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
 else
  {
    Status = HEAP_strdupA2W ( &lpszDeviceW, lpszDevice );
    if (!NT_SUCCESS (Status))
      SetLastError (RtlNtStatusToDosError(Status));
    else
      {
	Status = HEAP_strdupA2W ( &lpszOutputW, lpszOutput );
	if (!NT_SUCCESS (Status))
	  SetLastError (RtlNtStatusToDosError(Status));
	else
	  {
	    if ( lpdvmInit )
               dvmInitW = GdiConvertToDevmodeW((LPDEVMODEA)lpdvmInit);

               rc = IntCreateDICW ( lpszDriverW,
                                    lpszDeviceW,
                                    lpszOutputW,
                                      lpdvmInit ? dvmInitW : NULL,
                                              1 );
            HEAP_free (dvmInitW);
	    HEAP_free ( lpszOutputW );
	  }
	HEAP_free ( lpszDeviceW );
      }
    HEAP_free ( lpszDriverW );
  }
 return rc;
}


/*
 * @implemented
 */
BOOL
STDCALL
DeleteDC(HDC hDC)
{
  BOOL Ret = TRUE;
#if 0
  PDC_ATTR Dc_Attr;
  PLDC pLDC;

  if (!GdiGetHandleUserData((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;

  if ( Dc_Attr )
    {
      pLDC = Dc_Attr->pvLDC;

      if ( pLDC )
        {
          DPRINT1("Delete the Local DC structure\n");
          LocalFree( pLDC );
        }
    }
#endif
  Ret = NtGdiDeleteObjectApp(hDC);

  return Ret;
}

/*
 * @implemented
 */
BOOL
STDCALL
DeleteObject(HGDIOBJ hObject)
{
  UINT Type = 0;
    
  /* From Wine: DeleteObject does not SetLastError() on a null object */
  if(!hObject) return FALSE;

  if (0 != ((DWORD) hObject & GDI_HANDLE_STOCK_MASK))
  { // Relax! This is a normal return!
     DPRINT("Trying to delete system object 0x%x\n", hObject);
     return TRUE;
  }
  // If you dont own it?! Get OUT!
  if(!GdiIsHandleValid(hObject)) return FALSE;

  Type = GDI_HANDLE_GET_TYPE(hObject);

  if ((Type == GDI_OBJECT_TYPE_METAFILE) || 
      (Type == GDI_OBJECT_TYPE_ENHMETAFILE))
     return FALSE;

  switch (Type)
  {
     case GDI_OBJECT_TYPE_DC:
       return DeleteDC((HDC) hObject);
     case GDI_OBJECT_TYPE_COLORSPACE:
       return NtGdiDeleteColorSpace((HCOLORSPACE) hObject);
     case GDI_OBJECT_TYPE_REGION:
       return DeleteRegion((HRGN) hObject);
#if 0
     case GDI_OBJECT_TYPE_METADC:
       return MFDRV_DeleteObject( hObject );
     case GDI_OBJECT_TYPE_EMF:
     {          
       PLDC pLDC = GdiGetLDC(hObject);
       if ( !pLDC ) return FALSE;
       return EMFDRV_DeleteObject( hObject );
     }
#endif
     case GDI_OBJECT_TYPE_FONT:
       break;

     case GDI_OBJECT_TYPE_BRUSH:
     case GDI_OBJECT_TYPE_EXTPEN:
     case GDI_OBJECT_TYPE_PEN:
       {
          PBRUSH_ATTR Brh_Attr;
          PTEB pTeb;

          if ((!GdiGetHandleUserData(hObject, (DWORD)Type, (PVOID) &Brh_Attr)) ||
              (Brh_Attr == NULL) ) break;

          pTeb = NtCurrentTeb();

          if (pTeb->Win32ThreadInfo == NULL) break;

          if ((pTeb->GdiTebBatch.Offset + sizeof(GDIBSOBJECT)) <= GDIBATCHBUFSIZE)
          {
             PGDIBSOBJECT pgO = (PGDIBSOBJECT)(&pTeb->GdiTebBatch.Buffer[0] +
                                                      pTeb->GdiTebBatch.Offset);
             pgO->gbHdr.Cmd = GdiBCDelObj;
             pgO->gbHdr.Size = sizeof(GDIBSOBJECT);
             pgO->hgdiobj = hObject;

             pTeb->GdiTebBatch.Offset += sizeof(GDIBSOBJECT);
             pTeb->GdiBatchCount++;
             if (pTeb->GdiBatchCount >= GDI_BatchLimit) NtGdiFlush();
             return TRUE;
          }
       break;
       }
     case GDI_OBJECT_TYPE_BITMAP:
     default:
       break;
  }
  return NtGdiDeleteObjectApp(hObject);
}

INT
STDCALL
GetArcDirection( HDC hdc )
{
  return GetDCDWord( hdc, GdiGetArcDirection, 0);
}


INT
STDCALL
SetArcDirection( HDC hdc, INT nDirection )
{
  return GetAndSetDCDWord( hdc, GdiGetSetArcDirection, nDirection, 0, 0, 0 );
}


HGDIOBJ
STDCALL
GetDCObject( HDC hDC, INT iType)
{
 if((iType == GDI_OBJECT_TYPE_BRUSH) ||
    (iType == GDI_OBJECT_TYPE_EXTPEN)||
    (iType == GDI_OBJECT_TYPE_PEN)   ||
    (iType == GDI_OBJECT_TYPE_COLORSPACE))
 {
   HGDIOBJ hGO = NULL;
   PDC_ATTR Dc_Attr;

   if (!hDC) return hGO;

   if (!GdiGetHandleUserData((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return NULL;

   switch (iType)
   {
     case GDI_OBJECT_TYPE_BRUSH:
          hGO = Dc_Attr->hbrush;
          break;

     case GDI_OBJECT_TYPE_EXTPEN:
     case GDI_OBJECT_TYPE_PEN:
          hGO = Dc_Attr->hpen;
          break;

     case GDI_OBJECT_TYPE_COLORSPACE:
          hGO = Dc_Attr->hColorSpace;
          break;
   }
   return hGO;
 }
 return NtGdiGetDCObject( hDC, iType );
}


/*
 * @implemented
 *
 */
HGDIOBJ
STDCALL
GetCurrentObject(HDC hdc,
                 UINT uObjectType)
{
    switch(uObjectType)
    {
      case OBJ_EXTPEN:
      case OBJ_PEN:
        uObjectType = GDI_OBJECT_TYPE_PEN;
        break;
      case OBJ_BRUSH:
        uObjectType = GDI_OBJECT_TYPE_BRUSH;
        break;
      case OBJ_PAL:
        uObjectType = GDI_OBJECT_TYPE_PALETTE;
        break;
      case OBJ_FONT:
        uObjectType = GDI_OBJECT_TYPE_FONT;
        break;
      case OBJ_BITMAP:
        uObjectType = GDI_OBJECT_TYPE_BITMAP;
        break;
      case OBJ_COLORSPACE:
        uObjectType = GDI_OBJECT_TYPE_COLORSPACE;
        break;
      /* tests show that OBJ_REGION is explicitly ignored */
      case OBJ_REGION:
        return NULL;
      /* the SDK only mentions those above */
      default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    return  GetDCObject(hdc, uObjectType);
}

/*
 * @implemented
 *
 */
int
STDCALL
GetDeviceCaps(HDC hDC,
              int i)
{
  PDC_ATTR Dc_Attr;
  PLDC pLDC;
  PDEVCAPS pDevCaps = GdiDevCaps; // Primary display device capabilities.
  DPRINT("Device CAPS1\n");

  if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
  {
     if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
     {
        if ( i == TECHNOLOGY) return DT_METAFILE;
        return 0;
     }
     else
     {
        pLDC = GdiGetLDC(hDC);
        if ( !pLDC )
        {
           SetLastError(ERROR_INVALID_HANDLE);
           return 0;
        }
        if (!(pLDC->Flags & LDC_DEVCAPS))
        {
           if (!NtGdiGetDeviceCapsAll(hDC, &pLDC->DevCaps))
              SetLastError(ERROR_INVALID_PARAMETER);
           pLDC->Flags |= LDC_DEVCAPS;
        }
        pDevCaps = &pLDC->DevCaps;
     }
  }
  else
  {
     // HAX!!!!
     // Due to winlogon process/thread mapping issues we have this hax!
     //
     return NtGdiGetDeviceCaps(hDC,i);

     if (!GdiGetHandleUserData((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr))
        return 0;
     if (!(Dc_Attr->ulDirty_ & DC_PRIMARY_DISPLAY) )
        return NtGdiGetDeviceCaps(hDC,i);
  }
  DPRINT("Device CAPS2\n");
  
  switch (i)
  {
    case DRIVERVERSION:
      return pDevCaps->ulVersion;

    case TECHNOLOGY:
      return pDevCaps->ulTechnology;

    case HORZSIZE:
      return pDevCaps->ulHorzSize;

    case VERTSIZE:
      return pDevCaps->ulVertSize;

    case HORZRES:
      return pDevCaps->ulHorzRes;

    case VERTRES:
      return pDevCaps->ulVertRes;

    case LOGPIXELSX:
      return pDevCaps->ulLogPixelsX;

    case LOGPIXELSY:
      return pDevCaps->ulLogPixelsY;

    case BITSPIXEL:
      return pDevCaps->ulBitsPixel;

    case PLANES:
      return pDevCaps->ulPlanes;

    case NUMBRUSHES:
      return -1;

    case NUMPENS:
      return pDevCaps->ulNumPens;

    case NUMFONTS:
      return pDevCaps->ulNumFonts;

    case NUMCOLORS:
      return pDevCaps->ulNumColors;

    case ASPECTX:
      return pDevCaps->ulAspectX;

    case ASPECTY:
      return pDevCaps->ulAspectY;

    case ASPECTXY:
      return pDevCaps->ulAspectXY;

    case CLIPCAPS:
      return CP_RECTANGLE;

    case SIZEPALETTE:
      return pDevCaps->ulSizePalette;

    case NUMRESERVED:
      return 20;

    case COLORRES:
      return pDevCaps->ulColorRes;

    case DESKTOPVERTRES:
      return pDevCaps->ulVertRes;

    case DESKTOPHORZRES:
      return pDevCaps->ulHorzRes;

    case BLTALIGNMENT:
      return pDevCaps->ulBltAlignment;

    case SHADEBLENDCAPS:
      return pDevCaps->ulShadeBlend;

    case COLORMGMTCAPS:
      return pDevCaps->ulColorMgmtCaps;

    case PHYSICALWIDTH:
      return pDevCaps->ulPhysicalWidth;

    case PHYSICALHEIGHT:
      return pDevCaps->ulPhysicalHeight;

    case PHYSICALOFFSETX:
      return pDevCaps->ulPhysicalOffsetX;

    case PHYSICALOFFSETY:
      return pDevCaps->ulPhysicalOffsetY;

    case VREFRESH:
      return pDevCaps->ulVRefresh;

    case RASTERCAPS:
      return pDevCaps->ulRasterCaps;

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
      return pDevCaps->ulTextCaps;

    case PDEVICESIZE:
    case SCALINGFACTORX:
    case SCALINGFACTORY:
    default:
      return 0;
  }
  return 0;
}

/*
 * @implemented
 */
DWORD
STDCALL
GetRelAbs(
         HDC  hdc,
         DWORD dwIgnore
           )
{
  return GetDCDWord( hdc, GdiGetRelAbs, 0);
}


/*
 * @implemented
 */
DWORD
STDCALL
SetRelAbs(
	HDC hdc,
	INT Mode
	)
{
  return GetAndSetDCDWord( hdc, GdiGetSetRelAbs, Mode, 0, 0, 0 );
}


/*
 * @implemented
 */
DWORD
STDCALL
GetAndSetDCDWord( HDC hDC, INT u, DWORD dwIn, DWORD Unk1, DWORD Unk2, DWORD Unk3 )
{
  BOOL Ret = TRUE;
// Handle something other than a normal dc object.
  if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
       return 0; //call MFDRV
    else
    {
       PLDC pLDC = GdiGetLDC(hDC);
       if ( !pLDC )
       {
           SetLastError(ERROR_INVALID_HANDLE);
           return 0;
       }
       if (pLDC->iType == LDC_EMFLDC)
       {
          Ret = TRUE; //call EMFDRV
          if (Ret)
             return u;
          return 0;
       }
    }
  }
  Ret = NtGdiGetAndSetDCDword( hDC, u, dwIn, (DWORD*) &u );
  if (Ret)
     return u;
  else
     SetLastError(ERROR_INVALID_HANDLE);
  return 0;
}


/*
 * @implemented
 */
DWORD
STDCALL
GetDCDWord( HDC hDC, INT u, DWORD Result )
{
  BOOL Ret = NtGdiGetDCDword( hDC, u, (DWORD*) &u );
  if (!Ret) return Result;
  else return u;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetAspectRatioFilterEx(
                HDC hdc,
                LPSIZE lpAspectRatio
                      )
{
  return NtGdiGetDCPoint( hdc, GdiGetAspectRatioFilter, (LPPOINT) lpAspectRatio );
}


/*
 * @implemented
 */
BOOL
STDCALL
GetDCOrgEx(
    HDC hdc,
    LPPOINT lpPoint
    )
{
  return NtGdiGetDCPoint( hdc, GdiGetDCOrg, lpPoint );
}


/*
 * @implemented
 */
LONG
STDCALL
GetDCOrg(
    HDC hdc
    )
{
  // Officially obsolete by Microsoft
  POINT Pt;
  if (!GetDCOrgEx(hdc, &Pt))
    return 0;
  return(MAKELONG(Pt.x, Pt.y));
}


int
GetNonFontObject(HGDIOBJ hGdiObj, int cbSize, LPVOID lpBuffer)
{
  INT dwType;

  hGdiObj = (HANDLE)GdiFixUpHandle(hGdiObj);
  dwType = GDI_HANDLE_GET_TYPE(hGdiObj);

  if (!lpBuffer) // Should pass it all to Win32k and let god sort it out. ;^)
  {
    switch(dwType)
    {
      case GDI_OBJECT_TYPE_PEN:
        return sizeof(LOGPEN);
      case GDI_OBJECT_TYPE_BRUSH:
        return sizeof(LOGBRUSH);
      case GDI_OBJECT_TYPE_BITMAP:
        return sizeof(BITMAP);
      case GDI_OBJECT_TYPE_PALETTE:
        return sizeof(WORD);
      case GDI_OBJECT_TYPE_EXTPEN: /* we don't know the size, ask win32k */
        break;
    }
  }

  switch(dwType)
  {
    case GDI_OBJECT_TYPE_PEN: //Check the structures and see if A & W are the same.
    case GDI_OBJECT_TYPE_EXTPEN:
    case GDI_OBJECT_TYPE_BRUSH: // Mixing Apples and Oranges?
    case GDI_OBJECT_TYPE_BITMAP:
    case GDI_OBJECT_TYPE_PALETTE:
      return NtGdiExtGetObjectW(hGdiObj, cbSize, lpBuffer);

    case GDI_OBJECT_TYPE_DC:
    case GDI_OBJECT_TYPE_REGION:
    case GDI_OBJECT_TYPE_METAFILE:
    case GDI_OBJECT_TYPE_ENHMETAFILE:
    case GDI_OBJECT_TYPE_EMF:
      SetLastError(ERROR_INVALID_HANDLE);
  }
  return 0;
}


/*
 * @implemented
 */
int
STDCALL
GetObjectA(HGDIOBJ hGdiObj, int cbSize, LPVOID lpBuffer)
{
  ENUMLOGFONTEXDVW LogFont;
  DWORD dwType;
  INT Result = 0;

  dwType = GDI_HANDLE_GET_TYPE(hGdiObj);;

  if(dwType == GDI_OBJECT_TYPE_COLORSPACE) //Stays here, processes struct A
  {
     SetLastError(ERROR_NOT_SUPPORTED);
     return 0;
  }

  if (dwType == GDI_OBJECT_TYPE_FONT)
  {
    if (!lpBuffer)
    {
      return sizeof(LOGFONTA);
    }
    if (cbSize == 0)
    {
      /* Windows does not SetLastError() */
      return 0;
    }
    // ENUMLOGFONTEXDVW is the default size and should be the structure for
    // Entry->KernelData for Font objects.
    Result = NtGdiExtGetObjectW(hGdiObj, sizeof(ENUMLOGFONTEXDVW), &LogFont);

    if (0 == Result)
    {
      return 0;
    }

    switch (cbSize)
      {
         case sizeof(ENUMLOGFONTEXDVA):
         // need to move more here.
         case sizeof(ENUMLOGFONTEXA):
            EnumLogFontExW2A( (LPENUMLOGFONTEXA) lpBuffer, &LogFont.elfEnumLogfontEx );
            break;

         case sizeof(ENUMLOGFONTA):
         // Same here, maybe? Check the structures.
         case sizeof(EXTLOGFONTA):
         // Same here
         case sizeof(LOGFONTA):
            LogFontW2A((LPLOGFONTA) lpBuffer, &LogFont.elfEnumLogfontEx.elfLogFont);
            break;

         default:
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return 0;
      }
    return cbSize;
  }

  return GetNonFontObject(hGdiObj, cbSize, lpBuffer);
}


/*
 * @implemented
 */
int
STDCALL
GetObjectW(HGDIOBJ hGdiObj, int cbSize, LPVOID lpBuffer)
{
  DWORD dwType = GDI_HANDLE_GET_TYPE(hGdiObj);
  INT Result = 0;

/*
  Check List:
  MSDN, "This can be a handle to one of the following: logical bitmap, a brush,
  a font, a palette, a pen, or a device independent bitmap created by calling
  the CreateDIBSection function."
 */
  if(dwType == GDI_OBJECT_TYPE_COLORSPACE) //Stays here, processes struct W
  {
     SetLastError(ERROR_NOT_SUPPORTED); // Not supported yet.
     return 0;
  }

  if (dwType == GDI_OBJECT_TYPE_FONT)
  {
    if (!lpBuffer)
    {
      return sizeof(LOGFONTW);
    }

    if (cbSize == 0)
    {
      /* Windows does not SetLastError() */
      return 0;
    }
    // Poorly written apps are not ReactOS problem!
    // We fix it here if the size is larger than the default size.
    if( cbSize > sizeof(ENUMLOGFONTEXDVW) ) cbSize = sizeof(ENUMLOGFONTEXDVW);

    Result = NtGdiExtGetObjectW(hGdiObj, cbSize, lpBuffer); // Should handle the copy.

    if (0 == Result)
    {
      return 0;
    }
    return cbSize;
  }

  return GetNonFontObject(hGdiObj, cbSize, lpBuffer);
}


/*
 * @implemented
 */
COLORREF
STDCALL
GetDCBrushColor(
	HDC hdc
)
{
  PDC_ATTR Dc_Attr;

  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return CLR_INVALID;
  return (COLORREF) Dc_Attr->ulPenClr;
}

/*
 * @implemented
 */
COLORREF
STDCALL
GetDCPenColor(
	HDC hdc
)
{
  PDC_ATTR Dc_Attr;

  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return CLR_INVALID;
  return (COLORREF) Dc_Attr->ulPenClr;
}

/*
 * @implemented
 */
COLORREF
STDCALL
SetDCBrushColor(
	HDC hdc,
	COLORREF crColor
)
{
  PDC_ATTR Dc_Attr;
  COLORREF OldColor = CLR_INVALID;

  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return OldColor;
  else
  {
    OldColor = (COLORREF) Dc_Attr->ulBrushClr;
    Dc_Attr->ulBrushClr = (ULONG) crColor;

    if ( Dc_Attr->crBrushClr != crColor ) // if same, don't force a copy.
    {
       Dc_Attr->ulDirty_ |= DIRTY_FILL;
       Dc_Attr->crBrushClr = crColor;
    }
  }
  return OldColor;
}

/*
 * @implemented
 */
COLORREF
STDCALL
SetDCPenColor(
	HDC hdc,
	COLORREF crColor
)
{
  PDC_ATTR Dc_Attr;
  COLORREF OldColor = CLR_INVALID;

  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return OldColor;
  else
  {
     OldColor = (COLORREF) Dc_Attr->ulPenClr;
     Dc_Attr->ulPenClr = (ULONG) crColor;

    if ( Dc_Attr->crPenClr != crColor )
    {
       Dc_Attr->ulDirty_ |= DIRTY_LINE;
       Dc_Attr->crPenClr = crColor;
    }
  }
  return OldColor;
}

/*
 * @implemented
 *
 */
COLORREF
STDCALL
GetBkColor(HDC hdc)
{
  PDC_ATTR Dc_Attr;
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;
  return Dc_Attr->ulBackgroundClr;
}

/*
 * @implemented
 */
COLORREF
STDCALL
SetBkColor(
	HDC hdc,
	COLORREF crColor
)
{
  PDC_ATTR Dc_Attr;
  COLORREF OldColor = CLR_INVALID;

  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return OldColor;
#if 0
  if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_SetBkColor( hDC, crColor );
    else
    {
      PLDC pLDC = Dc_Attr->pvLDC;
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_SetBkColor( hDC, crColor );
      }
    }
  }
#endif
  OldColor = (COLORREF) Dc_Attr->ulBackgroundClr;
  Dc_Attr->ulBackgroundClr = (ULONG) crColor;

  if ( Dc_Attr->crBackgroundClr != crColor )
  {
     Dc_Attr->ulDirty_ |= (DIRTY_BACKGROUND|DIRTY_LINE|DIRTY_FILL);
     Dc_Attr->crBackgroundClr = crColor;
  }
  return OldColor;
}

/*
 * @implemented
 *
 */
int
STDCALL
GetBkMode(HDC hdc)
{
  PDC_ATTR Dc_Attr;
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;
  return Dc_Attr->lBkMode;
}

/*
 * @implemented
 *
 */
int
STDCALL
SetBkMode(HDC hdc,
              int iBkMode)
{
  PDC_ATTR Dc_Attr;
  INT OldMode = 0;

  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return OldMode;
#if 0
  if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_SetBkMode( hdc, iBkMode )
    else
    {
      PLDC pLDC = Dc_Attr->pvLDC;
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_SetBkMode( hdc, iBkMode )
      }
    }
  }
#endif
  OldMode = Dc_Attr->lBkMode;
  Dc_Attr->jBkMode = iBkMode; // Processed
  Dc_Attr->lBkMode = iBkMode; // Raw
  return OldMode;
}

/*
 * @implemented
 *
 */
int
STDCALL
GetPolyFillMode(HDC hdc)
{
  PDC_ATTR Dc_Attr;
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;
  return Dc_Attr->lFillMode;
}

/*
 * @unimplemented
 */
int
STDCALL
SetPolyFillMode(HDC hdc,
                int iPolyFillMode)
{
  INT fmode;
  PDC_ATTR Dc_Attr;
#if 0
  if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_SetPolyFillMode( hdc, iPolyFillMode )
    else
    {
      PLDC pLDC = GdiGetLDC(hdc);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_SetPolyFillMode( hdc, iPolyFillMode )
      }
    }
  }
#endif
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;

  if (NtCurrentTeb()->GdiTebBatch.HDC == hdc)
  {
     if (Dc_Attr->ulDirty_ & DC_MODE_DIRTY)
     {
       NtGdiFlush(); // Sync up Dc_Attr from Kernel space.
       Dc_Attr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
     }
  }

  fmode = Dc_Attr->lFillMode;
  Dc_Attr->lFillMode = iPolyFillMode;

  return fmode;
}

/*
 * @implemented
 *
 */
int
STDCALL
GetGraphicsMode(HDC hdc)
{
  PDC_ATTR Dc_Attr;
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;
  return Dc_Attr->iGraphicsMode;
}

/*
 * @unimplemented
 */
int
STDCALL
SetGraphicsMode(HDC hdc,
                int iMode)
{
  INT oMode;
  PDC_ATTR Dc_Attr;
  if ((iMode < GM_COMPATIBLE) || (iMode > GM_ADVANCED))
  {
     SetLastError(ERROR_INVALID_PARAMETER);
     return 0;
  }
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;

  if (iMode == Dc_Attr->iGraphicsMode) return iMode;

  if (NtCurrentTeb()->GdiTebBatch.HDC == hdc)
  {
     if (Dc_Attr->ulDirty_ & DC_MODE_DIRTY)
     {
       NtGdiFlush(); // Sync up Dc_Attr from Kernel space.
       Dc_Attr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
     }
  }
/* One would think that setting the graphics mode to GM_COMPATIBLE
 * would also reset the world transformation matrix to the unity
 * matrix. However, in Windows, this is not the case. This doesn't
 * make a lot of sense to me, but that's the way it is.
 */
  oMode = Dc_Attr->iGraphicsMode;
  Dc_Attr->iGraphicsMode = iMode;

  return oMode;
}

/*
 * @implemented
 */
HDC
STDCALL
ResetDCW(
	HDC		hdc,
	CONST DEVMODEW	*lpInitData
	)
{
  NtGdiResetDC ( hdc, (PDEVMODEW)lpInitData, NULL, NULL, NULL);
  return hdc;
}


/*
 * @implemented
 */
HDC
STDCALL
ResetDCA(
	HDC		hdc,
	CONST DEVMODEA	*lpInitData
	)
{
  LPDEVMODEW InitDataW;

  InitDataW = GdiConvertToDevmodeW((LPDEVMODEA)lpInitData);

  NtGdiResetDC ( hdc, InitDataW, NULL, NULL, NULL);
  HEAP_free(InitDataW);
  return hdc;
}


/*
 * @implemented
 */
int
STDCALL
StartDocW(
	HDC		hdc,
	CONST DOCINFOW	*a1
	)
{
	return NtGdiStartDoc ( hdc, (DOCINFOW *)a1, NULL, 0);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetObjectType(
	HGDIOBJ h
	)
{
  DWORD Ret = 0;

  if(GdiIsHandleValid(h))
  {
    LONG Type = GDI_HANDLE_GET_TYPE(h);
    switch(Type)
    {
      case GDI_OBJECT_TYPE_PEN:
        Ret = OBJ_PEN;
        break;
      case GDI_OBJECT_TYPE_BRUSH:
        Ret = OBJ_BRUSH;
        break;
      case GDI_OBJECT_TYPE_BITMAP:
        Ret = OBJ_BITMAP;
        break;
      case GDI_OBJECT_TYPE_FONT:
        Ret = OBJ_FONT;
        break;
      case GDI_OBJECT_TYPE_PALETTE:
        Ret = OBJ_PAL;
        break;
      case GDI_OBJECT_TYPE_REGION:
        Ret = OBJ_REGION;
        break;
      case GDI_OBJECT_TYPE_DC:
        if ( GetDCDWord( h, GdiGetIsMemDc, 0))
        {
           Ret = OBJ_MEMDC;
        }
        else
           Ret = OBJ_DC;
        break;
      case GDI_OBJECT_TYPE_COLORSPACE:
        Ret = OBJ_COLORSPACE;
        break;
      case GDI_OBJECT_TYPE_METAFILE:
        Ret = OBJ_METAFILE;
        break;
      case GDI_OBJECT_TYPE_ENHMETAFILE:
        Ret = OBJ_ENHMETAFILE;
        break;
      case GDI_OBJECT_TYPE_METADC:
        Ret = OBJ_METADC;
        break;
      case GDI_OBJECT_TYPE_EXTPEN:
        Ret = OBJ_EXTPEN;
        break;

      default:
        DPRINT1("GetObjectType: Magic 0x%08x not implemented\n", Type);
        break;
    }
  }
  else
    /* From Wine: GetObjectType does SetLastError() on a null object */
    SetLastError(ERROR_INVALID_HANDLE);
  return Ret;
}


/*
 * @implemented
 */
HGDIOBJ
WINAPI
GetStockObject(
              INT h
              )
{
  HGDIOBJ Ret = NULL;
  if ((h < 0) || (h >= NB_STOCK_OBJECTS)) return Ret;
  Ret = stock_objects[h];
  if (!Ret)
  {
      HGDIOBJ Obj = NtGdiGetStockObject( h );

      if (GdiIsHandleValid(Obj))
      {
         stock_objects[h] = Obj;
         return Obj;
      }// Returns Null anyway.
  }
  return Ret;
}

/* FIXME: include correct header */
HPALETTE STDCALL NtUserSelectPalette(HDC  hDC,
                            HPALETTE  hpal,
                            BOOL  ForceBackground);

HPALETTE
STDCALL
SelectPalette(
    HDC hDC,
    HPALETTE hPal,
    BOOL bForceBackground)
{
#if 0
  if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_SelectPalette( hDC, hPal, bForceBackground);
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return NULL;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        if return EMFDRV_SelectPalette( hDC, hPal, bForceBackground);
      }
    }
  }
#endif
    return NtUserSelectPalette(hDC, hPal, bForceBackground);
}

/*
 * @implemented
 *
 */
int
STDCALL
GetMapMode(HDC hdc)
{
  PDC_ATTR Dc_Attr;
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;
  return Dc_Attr->iMapMode;
}

/*
 * @implemented
 */
INT
STDCALL
SetMapMode(
	HDC hdc,
	INT Mode
	)
{
  PDC_ATTR Dc_Attr;
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;
#if 0
  if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_SetMapMode(hdc, Mode);
    else
    {
      SetLastError(ERROR_INVALID_HANDLE);
      return 0;
    }
#endif
  // Force change if Isotropic is set for recompute.
  if ((Mode != Dc_Attr->iMapMode) || (Mode == MM_ISOTROPIC))
  {
      Dc_Attr->ulDirty_ &= ~SLOW_WIDTHS;
      return GetAndSetDCDWord( hdc, GdiGetSetMapMode, Mode, 0, 0, 0 );
  }
  return Dc_Attr->iMapMode;
}

/*
 * @implemented
 *
 */
int
STDCALL
GetStretchBltMode(HDC hdc)
{
  PDC_ATTR Dc_Attr;
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;
  return Dc_Attr->lStretchBltMode;
}

/*
 * @implemented
 */
int
STDCALL
SetStretchBltMode(HDC hdc, int iStretchMode)
{
  INT oSMode;
  PDC_ATTR Dc_Attr;
#if 0
  if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_SetStretchBltMode( hdc, iStretchMode);
    else
    {
      PLDC pLDC = GdiGetLDC(hdc);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return 0;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_SetStretchBltMode( hdc, iStretchMode);
      }
    }
  }
#endif
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;

  oSMode = Dc_Attr->lStretchBltMode;
  Dc_Attr->lStretchBltMode = iStretchMode;

  // Wine returns an error here. We set the default.
  if ((iStretchMode <= 0) || (iStretchMode > MAXSTRETCHBLTMODE)) iStretchMode = WHITEONBLACK;

  Dc_Attr->jStretchBltMode = iStretchMode;

  return oSMode;
}

/*
 * @implemented
 */
HFONT
STDCALL
GetHFONT(HDC hdc)
{
  PDC_ATTR Dc_Attr;
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return NULL;
  return Dc_Attr->hlfntNew;
}


/*
 * @implemented
 *
 */
HGDIOBJ
STDCALL
SelectObject(HDC hDC,
             HGDIOBJ hGdiObj)
{
    PDC_ATTR pDc_Attr;
    HGDIOBJ hOldObj = NULL;
//    PTEB pTeb;

    if(!GdiGetHandleUserData(hDC, GDI_OBJECT_TYPE_DC, (PVOID)&pDc_Attr))
    {
    	SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    hGdiObj = GdiFixUpHandle(hGdiObj);
    if (!GdiIsHandleValid(hGdiObj))
    {
        return NULL;
    }

    UINT uType = GDI_HANDLE_GET_TYPE(hGdiObj);

    switch (uType)
    {
        case GDI_OBJECT_TYPE_REGION:
            return (HGDIOBJ)ExtSelectClipRgn(hDC, hGdiObj, RGN_COPY);

        case GDI_OBJECT_TYPE_BITMAP:
            return NtGdiSelectBitmap(hDC, hGdiObj);

        case GDI_OBJECT_TYPE_BRUSH:
#if 0 // enable this when support is ready in win32k
            hOldObj = pDc_Attr->hbrush;
            pDc_Attr->ulDirty_ |= DC_BRUSH_DIRTY;
            pDc_Attr->hbrush = hGdiObj;
            return hOldObj;
#endif
            return NtGdiSelectBrush(hDC, hGdiObj);

        case GDI_OBJECT_TYPE_PEN:
        case GDI_OBJECT_TYPE_EXTPEN:
#if 0 // enable this when support is ready in win32k
            hOldObj = pDc_Attr->hpen;
            pDc_Attr->ulDirty_ |= DC_PEN_DIRTY;
            pDc_Attr->hpen = hGdiObj;
            return hOldObj;
#endif
            return NtGdiSelectPen(hDC, hGdiObj);

        case GDI_OBJECT_TYPE_FONT:
            hOldObj = pDc_Attr->hlfntNew;
            if (hOldObj == hGdiObj) return hOldObj;
#if 0
            pDc_Attr->ulDirty_ &= ~SLOW_WIDTHS;
            pDc_Attr->ulDirty_ |= DIRTY_CHARSET;
            pDc_Attr->hlfntNew = hGdiObj;
            pTeb = NtCurrentTeb();
            if (((pTeb->GdiTebBatch.HDC == 0) ||
                 (pTeb->GdiTebBatch.HDC == hDC)) &&
                ((pTeb->GdiTebBatch.Offset + sizeof(GDIBSOBJECT)) <= GDIBATCHBUFSIZE) &&
               (!(pDc_Attr->ulDirty_ & DC_DIBSECTION)))
            {
              PGDIBSOBJECT pgO = (PGDIBSOBJECT)(&pTeb->GdiTebBatch.Buffer[0] +
                                                pTeb->GdiTebBatch.Offset);
              pgO->gbHdr.Cmd = GdiBCSelObj;
              pgO->gbHdr.Size = sizeof(GDIBSOBJECT);
              pgO->hgdiobj = hGdiObj;

              pTeb->GdiTebBatch.Offset += sizeof(GDIBSOBJECT);
              pTeb->GdiTebBatch.HDC = hDC;
              pTeb->GdiBatchCount++;
              if (pTeb->GdiBatchCount >= GDI_BatchLimit) NtGdiFlush();
              return hOldObj;
            }
#endif
            // default for select object font
            return NtGdiSelectFont(hDC, hGdiObj);

#if 0
        case GDI_OBJECT_TYPE_METADC:
            return MFDRV_SelectObject( hDC, hGdiObj); 
        case GDI_OBJECT_TYPE_EMF:
            PLDC pLDC = GdiGetLDC(hDC);
            if ( !pLDC ) return NULL;
            return EMFDRV_SelectObject( hDC, hGdiObj);
#endif
        case GDI_OBJECT_TYPE_COLORSPACE:
            SetColorSpace(hDC, (HCOLORSPACE) hGdiObj);
            return NULL;

        case GDI_OBJECT_TYPE_PALETTE:
        default:
            SetLastError(ERROR_INVALID_FUNCTION);
            return NULL;
    }

    return NULL;
}
