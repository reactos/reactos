#include <precomp.h>

#define NDEBUG
#include <debug.h>

HGDIOBJ stock_objects[NB_STOCK_OBJECTS]; // temp location.

HDC
FASTCALL
IntCreateDICW(
    LPCWSTR lpwszDriver,
    LPCWSTR lpwszDevice,
    LPCWSTR lpwszOutput,
    PDEVMODEW lpInitData,
    ULONG iType)
{
    UNICODE_STRING Device, Output;
    HDC hdc = NULL;
    BOOL Display = FALSE, Default = FALSE;
    ULONG UMdhpdev = 0;

    HANDLE hspool = NULL;

    if ( !ghSpooler && !LoadTheSpoolerDrv())
    {
        DPRINT1("WinSpooler.Drv Did not load!\n");
    }
    else
    {
        DPRINT("WinSpooler.Drv Loaded! hMod -> 0x%p\n", ghSpooler);
    }

    if ((!lpwszDevice) && (!lpwszDriver))
    {
        Default = TRUE;  // Ask Win32k to set Default device.
        Display = TRUE;   // Most likely to be DISPLAY.
    }
    else
    {
        if ((lpwszDevice) && (wcslen(lpwszDevice) != 0))  // First
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

    hdc = NtGdiOpenDCW((Default ? NULL : &Device),
                       (PDEVMODEW) lpInitData,
                       (lpwszOutput ? &Output : NULL),
                       iType,             // DCW 0 and ICW 1.
                       Display,
                       hspool,
                       (PVOID) NULL,       // NULL for now.
                       (PVOID) &UMdhpdev );
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        PDC_ATTR Dc_Attr;
        PLDC pLDC;

        GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&Dc_Attr);

        pLDC = LocalAlloc(LMEM_ZEROINIT, sizeof(LDC));

        Dc_Attr->pvLDC = pLDC;
        pLDC->hDC = hdc;
        pLDC->iType = LDC_LDC; // 1 (init) local DC, 2 EMF LDC
        DbgPrint("DC_ATTR Allocated -> 0x%x\n",Dc_Attr);
    }
#endif
    return hdc;
}


/*
 * @implemented
 */
HDC
WINAPI
CreateCompatibleDC(
    _In_ HDC hdc)
{
    HDC hdcNew;
// PDC_ATTR pdcattr;

    hdcNew = NtGdiCreateCompatibleDC(hdc);
#if 0
    if ( hdc && hdcNew)
    {
        if (GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
        {
            if (pdcattr->pvLIcm) IcmCompatibleDC(hdcNew, hdc, pdcattr);
        }
    }
#endif

    return hdcNew;
}

/*
 * @implemented
 */
HDC
WINAPI
CreateDCA (
    LPCSTR lpszDriver,
    LPCSTR lpszDevice,
    LPCSTR lpszOutput,
    CONST DEVMODEA * lpdvmInit)
{
    ANSI_STRING DriverA, DeviceA, OutputA;
    UNICODE_STRING DriverU, DeviceU, OutputU;
    LPDEVMODEW dvmInitW = NULL;
    HDC hdc;

    /*
     * If needed, convert to Unicode
     * any string parameter.
     */

    if (lpszDriver != NULL)
    {
        RtlInitAnsiString(&DriverA, (LPSTR)lpszDriver);
        RtlAnsiStringToUnicodeString(&DriverU, &DriverA, TRUE);
    }
    else
    {
        DriverU.Buffer = NULL;
    }

    if (lpszDevice != NULL)
    {
        RtlInitAnsiString(&DeviceA, (LPSTR)lpszDevice);
        RtlAnsiStringToUnicodeString(&DeviceU, &DeviceA, TRUE);
    }
    else
    {
        DeviceU.Buffer = NULL;
    }

    if (lpszOutput != NULL)
    {
        RtlInitAnsiString(&OutputA, (LPSTR)lpszOutput);
        RtlAnsiStringToUnicodeString(&OutputU, &OutputA, TRUE);
    }
    else
    {
        OutputU.Buffer = NULL;
    }

    if (lpdvmInit != NULL)
        dvmInitW = GdiConvertToDevmodeW((LPDEVMODEA)lpdvmInit);

    hdc = IntCreateDICW(DriverU.Buffer,
                        DeviceU.Buffer,
                        OutputU.Buffer,
                        lpdvmInit ? dvmInitW : NULL,
                        0);
    HEAP_free(dvmInitW);

    /* Free Unicode parameters. */
    RtlFreeUnicodeString(&DriverU);
    RtlFreeUnicodeString(&DeviceU);
    RtlFreeUnicodeString(&OutputU);

    /* Return the DC handle. */
    return hdc;
}


/*
 * @implemented
 */
HDC
WINAPI
CreateDCW (
    LPCWSTR lpwszDriver,
    LPCWSTR lpwszDevice,
    LPCWSTR lpwszOutput,
    CONST DEVMODEW *lpInitData)
{
    return IntCreateDICW(lpwszDriver,
                         lpwszDevice,
                         lpwszOutput,
                         (PDEVMODEW)lpInitData,
                         0);
}


/*
 * @implemented
 */
HDC
WINAPI
CreateICW(
    LPCWSTR lpszDriver,
    LPCWSTR lpszDevice,
    LPCWSTR lpszOutput,
    CONST DEVMODEW *lpdvmInit)
{
    return IntCreateDICW(lpszDriver,
                         lpszDevice,
                         lpszOutput,
                         (PDEVMODEW)lpdvmInit,
                         1);
}


/*
 * @implemented
 */
HDC
WINAPI
CreateICA(
    LPCSTR lpszDriver,
    LPCSTR lpszDevice,
    LPCSTR lpszOutput,
    CONST DEVMODEA *lpdvmInit)
{
    NTSTATUS Status;
    LPWSTR lpszDriverW, lpszDeviceW, lpszOutputW;
    LPDEVMODEW dvmInitW = NULL;
    HDC hdc = 0;

    Status = HEAP_strdupA2W(&lpszDriverW, lpszDriver);
    if (!NT_SUCCESS(Status))
        SetLastError(RtlNtStatusToDosError(Status));
    else
    {
        Status = HEAP_strdupA2W(&lpszDeviceW, lpszDevice);
        if (!NT_SUCCESS(Status))
            SetLastError(RtlNtStatusToDosError(Status));
        else
        {
            Status = HEAP_strdupA2W(&lpszOutputW, lpszOutput);
            if (!NT_SUCCESS(Status))
                SetLastError(RtlNtStatusToDosError(Status));
            else
            {
                if (lpdvmInit)
                    dvmInitW = GdiConvertToDevmodeW((LPDEVMODEA)lpdvmInit);

                hdc = IntCreateDICW(lpszDriverW,
                                    lpszDeviceW,
                                    lpszOutputW,
                                    lpdvmInit ? dvmInitW : NULL,
                                    1 );
                HEAP_free(dvmInitW);
                HEAP_free(lpszOutputW);
            }
            HEAP_free(lpszDeviceW);
        }
        HEAP_free(lpszDriverW);
    }

    return hdc;
}


/*
 * @implemented
 */
BOOL
WINAPI
DeleteDC(HDC hdc)
{
    BOOL bResult = TRUE;
    PLDC pLDC = NULL;
    HANDLE hPrinter = NULL;
    ULONG hType = GDI_HANDLE_GET_TYPE(hdc);

    pLDC = GdiGetLDC(hdc);

    if (hType != GDILoObjType_LO_DC_TYPE)
    {

        if ( !pLDC || hType == GDILoObjType_LO_METADC16_TYPE)
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
        if (pLDC->Flags & LDC_INIT_DOCUMENT) AbortDoc(hdc);
        if (pLDC->hPrinter)
        {
            DocumentEventEx(NULL, pLDC->hPrinter, hdc, DOCUMENTEVENT_DELETEDC, 0, NULL, 0, NULL);
            hPrinter = pLDC->hPrinter;
            pLDC->hPrinter = NULL;
        }
    }

    bResult = NtGdiDeleteObjectApp(hdc);

    if (bResult && pLDC)
    {
        DPRINT1("Delete the Local DC structure\n");
        LocalFree( pLDC );
    }

    if (hPrinter)
        fpClosePrinter(hPrinter);

    return bResult;
}

/*
 * @implemented
 */
BOOL
WINAPI
DeleteObject(HGDIOBJ hObject)
{
    DWORD dwType = 0;

    /* From Wine: DeleteObject does not SetLastError() on a null object */
    if(!hObject) return FALSE;

    if ((DWORD)hObject & GDI_HANDLE_STOCK_MASK)
    {
        // Relax! This is a normal return!
        DPRINT("Trying to delete system object 0x%p\n", hObject);
        return TRUE;
    }

    // If you dont own it?! Get OUT!
    if(!GdiIsHandleValid(hObject)) return FALSE;

    dwType = GDI_HANDLE_GET_TYPE(hObject);

    if ((dwType == GDI_OBJECT_TYPE_METAFILE) ||
        (dwType == GDI_OBJECT_TYPE_ENHMETAFILE))
        return FALSE;

    switch (dwType)
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
        PGDIBSOBJECT pgO;

        if ((!GdiGetHandleUserData(hObject, dwType, (PVOID*)&Brh_Attr)) ||
            (Brh_Attr == NULL)) break;

        pTeb = NtCurrentTeb();

        if (pTeb->Win32ThreadInfo == NULL) break;

        pgO = GdiAllocBatchCommand(NULL, GdiBCDelObj);
        if (pgO)
        {
            pgO->hgdiobj = hObject;
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
WINAPI
GetArcDirection(
    _In_ HDC hdc)
{
    return GetDCDWord( hdc, GdiGetArcDirection, 0);
}


INT
WINAPI
SetArcDirection(
   _In_ HDC hdc,
   _In_ INT nDirection)
{
    return GetAndSetDCDWord(hdc, GdiGetSetArcDirection, nDirection, 0, 0, 0);
}

/*
 * @implemented
 *
 */
HGDIOBJ
WINAPI
GetCurrentObject(
    _In_ HDC hdc,
    _In_ UINT uObjectType)
{
    PDC_ATTR pdcattr = NULL;

    /* Check if this is a user mode object */
    if ((uObjectType == OBJ_PEN) ||
        (uObjectType == OBJ_EXTPEN) ||
        (uObjectType == OBJ_BRUSH) ||
        (uObjectType == OBJ_COLORSPACE))
    {
        /* Get the dc attribute */
        pdcattr = GdiGetDcAttr(hdc);
        if (!pdcattr) return NULL;
    }

    /* Check what object was requested */
    switch (uObjectType)
    {
        case OBJ_EXTPEN:
        case OBJ_PEN:
            return pdcattr->hpen;

        case OBJ_BRUSH:
            return pdcattr->hbrush;

        case OBJ_COLORSPACE:
            return pdcattr->hColorSpace;

        case OBJ_PAL:
            uObjectType = GDI_OBJECT_TYPE_PALETTE;
            break;

        case OBJ_FONT:
            uObjectType = GDI_OBJECT_TYPE_FONT;
            break;

        case OBJ_BITMAP:
            uObjectType = GDI_OBJECT_TYPE_BITMAP;
            break;

        /* All others are invalid */
        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
    }

    /* Pass the request to win32k */
    return NtGdiGetDCObject(hdc, uObjectType);
}

/*
 * @implemented
 *
 */
int
WINAPI
GetDeviceCaps(
    _In_ HDC hdc,
    _In_ int nIndex)
{
    PDC_ATTR pdcattr;
    PLDC pLDC;
    PDEVCAPS pDevCaps = GdiDevCaps; // Primary display device capabilities.
    DPRINT("Device CAPS1\n");

    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
        {
            if (nIndex == TECHNOLOGY) return DT_METAFILE;
            return 0;
        }
        else
        {
            pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return 0;
            }
            if (!(pLDC->Flags & LDC_DEVCAPS))
            {
                if (!NtGdiGetDeviceCapsAll(hdc, &pLDC->DevCaps))
                    SetLastError(ERROR_INVALID_PARAMETER);
                pLDC->Flags |= LDC_DEVCAPS;
            }
            pDevCaps = &pLDC->DevCaps;
        }
    }
    else
    {
        if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
            return 0;
        if (!(pdcattr->ulDirty_ & DC_PRIMARY_DISPLAY) )
            return NtGdiGetDeviceCaps(hdc, nIndex);
    }
    DPRINT("Device CAPS2\n");

    switch (nIndex)
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
WINAPI
GetRelAbs(
    _In_ HDC hdc,
    _In_ DWORD dwIgnore)
{
    return GetDCDWord(hdc, GdiGetRelAbs, 0);
}


/*
 * @implemented
 */
INT
WINAPI
SetRelAbs(
    HDC hdc,
    INT Mode)
{
    return GetAndSetDCDWord(hdc, GdiGetSetRelAbs, Mode, 0, 0, 0);
}


/*
 * @implemented
 */
DWORD
WINAPI
GetAndSetDCDWord(
    _In_ HDC hdc,
    _In_ UINT u,
    _In_ DWORD dwIn,
    _In_ ULONG ulMFId,
    _In_ USHORT usMF16Id,
    _In_ DWORD dwError)
{
    DWORD dwResult;
    BOOL Ret = TRUE;

    /* Handle something other than a normal dc object. */
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return 0; //call MFDRV
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
                Ret = TRUE; //call EMFDRV
                if (Ret)
                    return u;
                return 0;
            }
        }
    }

    if (!NtGdiGetAndSetDCDword(hdc, u, dwIn, &dwResult))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    return dwResult;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetDCDWord(
    _In_ HDC hdc,
    _In_ UINT u,
    _In_ DWORD dwError)
{
    DWORD dwResult;

    if (!NtGdiGetDCDword(hdc, u, &dwResult))
    {
        return dwError;
    }

    return dwResult;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetAspectRatioFilterEx(
    HDC hdc,
    LPSIZE lpAspectRatio)
{
    return NtGdiGetDCPoint(hdc, GdiGetAspectRatioFilter, (PPOINTL)lpAspectRatio );
}


/*
 * @implemented
 */
BOOL
WINAPI
GetDCOrgEx(
    HDC hdc,
    LPPOINT lpPoint)
{
    return NtGdiGetDCPoint( hdc, GdiGetDCOrg, (PPOINTL)lpPoint );
}


/*
 * @implemented
 */
LONG
WINAPI
GetDCOrg(
    HDC hdc)
{
    // Officially obsolete by Microsoft
    POINT Pt;
    if (!GetDCOrgEx(hdc, &Pt))
        return 0;
    return(MAKELONG(Pt.x, Pt.y));
}


/*
 * @implemented
 */
int
WINAPI
GetObjectW(
    _In_ HGDIOBJ hGdiObj,
    _In_ int cbSize,
    _Out_ LPVOID lpBuffer)
{
    DWORD dwType;
    INT cbResult = 0;

    /* Fixup handles with upper 16 bits masked */
    hGdiObj = GdiFixUpHandle(hGdiObj);

    /* Get the object type */
    dwType = GDI_HANDLE_GET_TYPE(hGdiObj);

    /* Check what kind of object we have */
    switch (dwType)
    {
        case GDI_OBJECT_TYPE_PEN:
            if (!lpBuffer) return sizeof(LOGPEN);
            break;

        case GDI_OBJECT_TYPE_BRUSH:
            if (!lpBuffer || !cbSize) return sizeof(LOGBRUSH);
            break;

        case GDI_OBJECT_TYPE_BITMAP:
            if (!lpBuffer) return sizeof(BITMAP);
            break;

        case GDI_OBJECT_TYPE_PALETTE:
            if (!lpBuffer) return sizeof(WORD);
            break;

        case GDI_OBJECT_TYPE_FONT:
            if (!lpBuffer) return sizeof(LOGFONTW);
            break;

        case GDI_OBJECT_TYPE_EXTPEN:
            /* we don't know the size, ask win32k */
            break;

        case GDI_OBJECT_TYPE_COLORSPACE:
            if ((cbSize < 328) || !lpBuffer)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            break;

        case GDI_OBJECT_TYPE_DC:
        case GDI_OBJECT_TYPE_REGION:
        case GDI_OBJECT_TYPE_EMF:
        case GDI_OBJECT_TYPE_METAFILE:
        case GDI_OBJECT_TYPE_ENHMETAFILE:
            SetLastError(ERROR_INVALID_HANDLE);
        default:
            return 0;
    }

    /* Call win32k */
    cbResult = NtGdiExtGetObjectW(hGdiObj, cbSize, lpBuffer);

    /* Handle error */
    if (cbResult == 0)
    {
        if (!GdiIsHandleValid(hGdiObj))
        {
            if ((dwType == GDI_OBJECT_TYPE_PEN) ||
                (dwType == GDI_OBJECT_TYPE_EXTPEN) ||
                (dwType == GDI_OBJECT_TYPE_BRUSH) ||
                (dwType == GDI_OBJECT_TYPE_COLORSPACE))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
            }
        }
        else
        {
            if ((dwType == GDI_OBJECT_TYPE_PEN) ||
                (dwType == GDI_OBJECT_TYPE_BRUSH) ||
                (dwType == GDI_OBJECT_TYPE_COLORSPACE) ||
                ( (dwType == GDI_OBJECT_TYPE_EXTPEN) &&
                    ( (cbSize >= sizeof(EXTLOGPEN)) || (cbSize == 0) ) ) ||
                ( (dwType == GDI_OBJECT_TYPE_BITMAP) && (cbSize >= sizeof(BITMAP)) ))
            {
                SetLastError(ERROR_NOACCESS);
            }
        }
    }

    return cbResult;
}


ULONG
WINAPI
GetFontObjectA(
    _In_ HGDIOBJ hfont,
    _In_ ULONG cbSize,
    _Out_ LPVOID lpBuffer)
{
    ENUMLOGFONTEXDVW elfedvW;
    ENUMLOGFONTEXDVA elfedvA;
    ULONG cbResult;

    /* Check if size only is requested */
    if (!lpBuffer) return sizeof(LOGFONTA);

    /* Check for size 0 */
    if (cbSize == 0)
    {
        /* Windows does not SetLastError() */
        return 0;
    }

    /* Windows does this ... */
    if (cbSize == sizeof(LOGFONTW)) cbSize = sizeof(LOGFONTA);

    /* Call win32k to get the logfont (widechar) */
    cbResult = NtGdiExtGetObjectW(hfont, sizeof(ENUMLOGFONTEXDVW), &elfedvW);
    if (cbResult == 0)
    {
        return 0;
    }

    /* Convert the logfont from widechar to ansi */
    EnumLogFontExW2A(&elfedvA.elfEnumLogfontEx, &elfedvW.elfEnumLogfontEx);
    elfedvA.elfDesignVector = elfedvW.elfDesignVector;

    /* Don't copy more than maximum */
    if (cbSize > sizeof(ENUMLOGFONTEXDVA)) cbSize = sizeof(ENUMLOGFONTEXDVA);

    /* Copy the number of bytes requested */
    memcpy(lpBuffer, &elfedvA, cbSize);

    /* Return the number of bytes copied */
    return cbSize;
}


/*
 * @implemented
 */
int
WINAPI
GetObjectA(
    _In_ HGDIOBJ hGdiObj,
    _In_ int cbSize,
    _Out_ LPVOID lpBuffer)
{
    DWORD dwType = GDI_HANDLE_GET_TYPE(hGdiObj);

    /* Chjeck if this is anything else but a font */
    if (dwType == GDI_OBJECT_TYPE_FONT)
    {
        return GetFontObjectA(hGdiObj, cbSize, lpBuffer);
    }
    else
    {
        /* Simply pass it to the widechar version */
        return GetObjectW(hGdiObj, cbSize, lpBuffer);
    }
}


/*
 * @implemented
 */
COLORREF
WINAPI
GetDCBrushColor(
    _In_ HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
        return CLR_INVALID;

    return pdcattr->ulBrushClr;
}

/*
 * @implemented
 */
COLORREF
WINAPI
GetDCPenColor(
    _In_ HDC hdc)
{
    PDC_ATTR pdcattr;

    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
        return CLR_INVALID;

    return pdcattr->ulPenClr;
}

/*
 * @implemented
 */
COLORREF
WINAPI
SetDCBrushColor(
    _In_ HDC hdc,
    _In_ COLORREF crColor)
{
    PDC_ATTR pdcattr;
    COLORREF crOldColor;

    /* Get the DC attribute */
    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
        return CLR_INVALID;

    /* Get old color and store the new */
    crOldColor = pdcattr->ulBrushClr;
    pdcattr->ulBrushClr = crColor;

    if (pdcattr->crBrushClr != crColor)
    {
        pdcattr->ulDirty_ |= DIRTY_FILL;
        pdcattr->crBrushClr = crColor;
    }

    return crOldColor;
}

/*
 * @implemented
 */
COLORREF
WINAPI
SetDCPenColor(
    _In_ HDC hdc,
    _In_ COLORREF crColor)
{
    PDC_ATTR pdcattr;
    COLORREF crOldColor;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return CLR_INVALID;
    }

    /* Get old color and store the new */
    crOldColor = pdcattr->ulPenClr;
    pdcattr->ulPenClr = (ULONG)crColor;

    if (pdcattr->crPenClr != crColor)
    {
        pdcattr->ulDirty_ |= DIRTY_LINE;
        pdcattr->crPenClr = crColor;
    }

    return crOldColor;
}

/*
 * @implemented
 *
 */
COLORREF
WINAPI
GetBkColor(
    _In_ HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
        return 0;

    return pdcattr->ulBackgroundClr;
}

/*
 * @implemented
 */
COLORREF
WINAPI
SetBkColor(
    _In_ HDC hdc,
    _In_ COLORREF crColor)
{
    PDC_ATTR pdcattr;
    COLORREF crOldColor;

    /* Get the DC attribute */
    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
        return CLR_INVALID;

#if 0
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_SetBkColor(hdc, crColor );
        else
        {
            PLDC pLDC = pdcattr->pvLDC;
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_SetBkColor(hdc, crColor );
            }
        }
    }
#endif

    /* Get old color and store the new */
    crOldColor = pdcattr->ulBackgroundClr;
    pdcattr->ulBackgroundClr = crColor;

    if (pdcattr->crBackgroundClr != crColor)
    {
        pdcattr->ulDirty_ |= (DIRTY_BACKGROUND|DIRTY_LINE|DIRTY_FILL);
        pdcattr->crBackgroundClr = crColor;
    }

    return crOldColor;
}

/*
 * @implemented
 *
 */
int
WINAPI
GetBkMode(HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
        return 0;

    return pdcattr->lBkMode;
}

/*
 * @implemented
 *
 */
int
WINAPI
SetBkMode(
    _In_ HDC hdc,
    _In_ int iBkMode)
{
    PDC_ATTR pdcattr;
    INT iOldMode;

    /* Get the DC attribute */
    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
        return 0;
#if 0
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_SetBkMode( hdc, iBkMode )
        else
        {
            PLDC pLDC = pdcattr->pvLDC;
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_SetBkMode(hdc, iBkMode)
            }
        }
    }
#endif

    iOldMode = pdcattr->lBkMode;
    pdcattr->jBkMode = iBkMode; // Processed
    pdcattr->lBkMode = iBkMode; // Raw

    return iOldMode;
}

/*
 * @implemented
 *
 */
int
WINAPI
GetPolyFillMode(HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        return 0;
    }

    /* Return current fill mode */
    return pdcattr->lFillMode;
}

/*
 * @unimplemented
 */
int
WINAPI
SetPolyFillMode(
    _In_ HDC hdc,
    _In_ int iPolyFillMode)
{
    INT iOldPolyFillMode;
    PDC_ATTR pdcattr;

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

    /* Get DC attribute */
    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
    {
        return 0;
    }

    if (NtCurrentTeb()->GdiTebBatch.HDC == hdc)
    {
        if (pdcattr->ulDirty_ & DC_MODE_DIRTY)
        {
            NtGdiFlush(); // Sync up pdcattr from Kernel space.
            pdcattr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
        }
    }

    iOldPolyFillMode = pdcattr->lFillMode;
    pdcattr->lFillMode = iPolyFillMode;

    return iOldPolyFillMode;
}

/*
 * @implemented
 *
 */
int
WINAPI
GetGraphicsMode(HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get DC attribute */
    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
    {
        return 0;
    }

    /* Return current graphics mode */
    return pdcattr->iGraphicsMode;
}

/*
 * @unimplemented
 */
int
WINAPI
SetGraphicsMode(
    _In_ HDC hdc,
    _In_ int iMode)
{
    INT iOldMode;
    PDC_ATTR pdcattr;

    /* Check parameters */
    if ((iMode < GM_COMPATIBLE) || (iMode > GM_ADVANCED))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Get DC attribute */
    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
    {
        return 0;
    }

    /* Check for trivial case */
    if (iMode == pdcattr->iGraphicsMode)
        return iMode;

    if (NtCurrentTeb()->GdiTebBatch.HDC == hdc)
    {
        if (pdcattr->ulDirty_ & DC_MODE_DIRTY)
        {
            NtGdiFlush(); // Sync up pdcattr from Kernel space.
            pdcattr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
        }
    }

    /* One would think that setting the graphics mode to GM_COMPATIBLE
     * would also reset the world transformation matrix to the unity
     * matrix. However, in Windows, this is not the case. This doesn't
     * make a lot of sense to me, but that's the way it is.
     */
    iOldMode = pdcattr->iGraphicsMode;
    pdcattr->iGraphicsMode = iMode;

    return iOldMode;
}

/*
 * @implemented
 */
HDC
WINAPI
ResetDCW(
    _In_ HDC hdc,
    _In_ CONST DEVMODEW *lpInitData)
{
    NtGdiResetDC ( hdc, (PDEVMODEW)lpInitData, NULL, NULL, NULL);
    return hdc;
}


/*
 * @implemented
 */
HDC
WINAPI
ResetDCA(
    _In_ HDC hdc,
    _In_ CONST DEVMODEA *lpInitData)
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
DWORD
WINAPI
GetObjectType(
    HGDIOBJ h)
{
    DWORD Ret = 0;

    if (GdiIsHandleValid(h))
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

        case GDILoObjType_LO_ALTDC_TYPE:
            // FIXME: could be something else?
            Ret = OBJ_ENHMETADC;
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
    INT fnObject)
{
    HGDIOBJ hobj;

    if ((fnObject < 0) || (fnObject >= NB_STOCK_OBJECTS))
        return NULL;

    hobj = stock_objects[fnObject];
    if (hobj == NULL)
    {
        hobj = NtGdiGetStockObject(fnObject);

        if (!GdiIsHandleValid(hobj))
        {
            return NULL;
        }

        stock_objects[fnObject] = hobj;
    }

    return hobj;
}

/* FIXME: include correct header */
HPALETTE WINAPI NtUserSelectPalette(HDC  hDC,
                                    HPALETTE  hpal,
                                    BOOL  ForceBackground);

HPALETTE
WINAPI
SelectPalette(
    HDC hdc,
    HPALETTE hpal,
    BOOL bForceBackground)
{
#if 0
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_SelectPalette(hdc, hpal, bForceBackground);
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return NULL;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                if return EMFDRV_SelectPalette(hdc, hpal, bForceBackground);
            }
        }
    }
#endif
    return NtUserSelectPalette(hdc, hpal, bForceBackground);
}

/*
 * @implemented
 *
 */
int
WINAPI
GetMapMode(HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &pdcattr))
        return 0;

    return pdcattr->iMapMode;
}

/*
 * @implemented
 */
INT
WINAPI
SetMapMode(
    _In_ HDC hdc,
    _In_ INT iMode)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

#if 0
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_SetMapMode(hdc, iMode);
        else
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return 0;
        }
    }
#endif
    /* Force change if Isotropic is set for recompute. */
    if ((iMode != pdcattr->iMapMode) || (iMode == MM_ISOTROPIC))
    {
        pdcattr->ulDirty_ &= ~SLOW_WIDTHS;
        return GetAndSetDCDWord( hdc, GdiGetSetMapMode, iMode, 0, 0, 0 );
    }

    return pdcattr->iMapMode;
}

/*
 * @implemented
 *
 */
int
WINAPI
GetStretchBltMode(HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
        return 0;

    return pdcattr->lStretchBltMode;
}

/*
 * @implemented
 */
int
WINAPI
SetStretchBltMode(
    _In_ HDC hdc,
    _In_ int iStretchMode)
{
    INT iOldMode;
    PDC_ATTR pdcattr;
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
    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
        return 0;

    iOldMode = pdcattr->lStretchBltMode;
    pdcattr->lStretchBltMode = iStretchMode;

    // Wine returns an error here. We set the default.
    if ((iStretchMode <= 0) || (iStretchMode > MAXSTRETCHBLTMODE)) iStretchMode = WHITEONBLACK;

    pdcattr->jStretchBltMode = iStretchMode;

    return iOldMode;
}

/*
 * @implemented
 */
HFONT
WINAPI
GetHFONT(HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    if (!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
    {
        return NULL;
    }

    /* Return the current font */
    return pdcattr->hlfntNew;
}


/*
 * @implemented
 *
 */
HGDIOBJ
WINAPI
SelectObject(
    _In_ HDC hdc,
    _In_ HGDIOBJ hGdiObj)
{
    PDC_ATTR pdcattr;
    HGDIOBJ hOldObj = NULL;
    UINT uType;

    if(!GdiGetHandleUserData(hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    hGdiObj = GdiFixUpHandle(hGdiObj);
    if (!GdiIsHandleValid(hGdiObj))
    {
        return NULL;
    }

    uType = GDI_HANDLE_GET_TYPE(hGdiObj);

    switch (uType)
    {
        case GDI_OBJECT_TYPE_REGION:
            return (HGDIOBJ)ExtSelectClipRgn(hdc, hGdiObj, RGN_COPY);

        case GDI_OBJECT_TYPE_BITMAP:
            return NtGdiSelectBitmap(hdc, hGdiObj);

        case GDI_OBJECT_TYPE_BRUSH:
            hOldObj = pdcattr->hbrush;
            pdcattr->ulDirty_ |= DC_BRUSH_DIRTY;
            pdcattr->hbrush = hGdiObj;
            return hOldObj;
//            return NtGdiSelectBrush(hdc, hGdiObj);

        case GDI_OBJECT_TYPE_PEN:
        case GDI_OBJECT_TYPE_EXTPEN:
            hOldObj = pdcattr->hpen;
            pdcattr->ulDirty_ |= DC_PEN_DIRTY;
            pdcattr->hpen = hGdiObj;
            return hOldObj;
//            return NtGdiSelectPen(hdc, hGdiObj);

        case GDI_OBJECT_TYPE_FONT:
            hOldObj = pdcattr->hlfntNew;
            if (hOldObj == hGdiObj) return hOldObj;

            pdcattr->ulDirty_ &= ~SLOW_WIDTHS;
            pdcattr->ulDirty_ |= DIRTY_CHARSET;
            pdcattr->hlfntNew = hGdiObj;

            if (!(pdcattr->ulDirty_ & DC_DIBSECTION))
            {
                PGDIBSOBJECT pgO;

                pgO = GdiAllocBatchCommand(hdc, GdiBCSelObj);
                if (pgO)
                {
                    pgO->hgdiobj = hGdiObj;
                    return hOldObj;
                }
            }

            // default for select object font
            return NtGdiSelectFont(hdc, hGdiObj);

#if 0
        case GDI_OBJECT_TYPE_METADC:
            return MFDRV_SelectObject(hdc, hGdiObj);
        case GDI_OBJECT_TYPE_EMF:
            PLDC pLDC = GdiGetLDC(hdc);
            if (!pLDC) return NULL;
            return EMFDRV_SelectObject(hdc, hGdiObj);
#endif
        case GDI_OBJECT_TYPE_COLORSPACE:
            SetColorSpace(hdc, (HCOLORSPACE) hGdiObj);
            return NULL;

        case GDI_OBJECT_TYPE_PALETTE:
            SetLastError(ERROR_INVALID_FUNCTION);
        default:
            return NULL;
    }

    return NULL;
}
