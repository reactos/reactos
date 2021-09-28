#include <precomp.h>

#define NDEBUG
#include <debug.h>

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
    HANDLE UMdhpdev = 0;

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

    // Handle Print device or something else.
    if (!Display)
    {
        // WIP - GDI Print Commit coming in soon.
        DPRINT1("Not a DISPLAY device! %wZ\n", &Device);
        return NULL; // Return NULL until then.....
    }

    hdc = NtGdiOpenDCW((Default ? NULL : &Device),
                       (PDEVMODEW) lpInitData,
                       (lpwszOutput ? &Output : NULL),
                       iType,             // DCW 0 and ICW 1.
                       Display,
                       hspool,
                       &UMdhpdev );
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
    ULONG hType = GDI_HANDLE_GET_TYPE(hdc);

    if (hType != GDILoObjType_LO_DC_TYPE)
    {
        return METADC_RosGlueDeleteDC(hdc);
    }

    //if ( ghICM || pdcattr->pvLIcm )
    //    IcmDeleteLocalDC( hdc, pdcattr, NULL );

    return NtGdiDeleteObjectApp(hdc);
}


/*
 * @implemented
 */
INT
WINAPI
SaveDC(IN HDC hdc)
{
    HANDLE_METADC1P(INT, SaveDC, 0, hdc);
    return NtGdiSaveDC(hdc);
}


/*
 * @implemented
 */
BOOL
WINAPI
RestoreDC(IN HDC hdc,
          IN INT iLevel)
{
    HANDLE_METADC(BOOL, RestoreDC, FALSE, hdc, iLevel);
    return NtGdiRestoreDC(hdc, iLevel);
}


/*
 * @implemented
 */
BOOL
WINAPI
CancelDC(HDC hDC)
{
    PDC_ATTR pDc_Attr;

    if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC &&
            GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_METADC )
    {
        PLDC pLDC = GdiGetLDC(hDC);
        if ( !pLDC )
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
        /* If a document has started set it to die. */
        if (pLDC->Flags & LDC_INIT_DOCUMENT) pLDC->Flags |= LDC_KILL_DOCUMENT;

        return NtGdiCancelDC(hDC);
    }

    if (GdiGetHandleUserData((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC, (PVOID) &pDc_Attr))
    {
        pDc_Attr->ulDirty_ &= ~DC_PLAYMETAFILE;
        return TRUE;
    }

    return FALSE;
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
    return GetAndSetDCDWord(hdc, GdiGetSetArcDirection, nDirection, EMR_SETARCDIRECTION, 0, 0);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiReleaseDC(HDC hdc)
{
    return 0;
}


/*
 * @implemented
 */
BOOL
WINAPI
GdiCleanCacheDC(HDC hdc)
{
    if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_DC_TYPE)
        return TRUE;
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
}

/*
 * @implemented
 */
HDC
WINAPI
GdiConvertAndCheckDC(HDC hdc)
{
    PLDC pldc;
    ULONG hType = GDI_HANDLE_GET_TYPE(hdc);
    if (hType == GDILoObjType_LO_DC_TYPE || hType == GDILoObjType_LO_METADC16_TYPE)
        return hdc;
    pldc = GdiGetLDC(hdc);
    if (pldc)
    {
        if (pldc->Flags & LDC_SAPCALLBACK) GdiSAPCallback(pldc);
        if (pldc->Flags & LDC_KILL_DOCUMENT) return NULL;
        if (pldc->Flags & LDC_STARTPAGE) StartPage(hdc);
        return hdc;
    }
    SetLastError(ERROR_INVALID_HANDLE);
    return NULL;
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
        /* Get the DC attribute */
        pdcattr = GdiGetDcAttr(hdc);
        if (pdcattr == NULL)
        {
            return NULL;
        }
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
 */
int
WINAPI
EnumObjects(HDC hdc,
            int nObjectType,
            GOBJENUMPROC lpObjectFunc,
            LPARAM lParam)
{
    ULONG ObjectsCount;
    ULONG Size;
    PVOID Buffer = NULL;
    DWORD_PTR EndOfBuffer;
    int Result = 0;

    switch (nObjectType)
    {
    case OBJ_BRUSH:
        Size = sizeof(LOGBRUSH);
        break;

    case OBJ_PEN:
        Size = sizeof(LOGPEN);
        break;

    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    ObjectsCount = NtGdiEnumObjects(hdc, nObjectType, 0, NULL);
    if (!ObjectsCount) return 0;

    Buffer = HeapAlloc(GetProcessHeap(), 0, ObjectsCount * Size);
    if (!Buffer)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    if (!NtGdiEnumObjects(hdc, nObjectType, ObjectsCount * Size, Buffer))
    {
        HeapFree(GetProcessHeap(), 0, Buffer);
        return 0;
    }

    EndOfBuffer = (DWORD_PTR)Buffer + (ObjectsCount * Size);
    while ((DWORD_PTR)Buffer < EndOfBuffer)
    {
        Result = lpObjectFunc(Buffer, lParam);
        if (!Result) break;
        Buffer = (PVOID)((DWORD_PTR)Buffer + Size);
    }

    HeapFree(GetProcessHeap(), 0, Buffer);
    return Result;
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
    PLDC pldc;
    ULONG hType = GDI_HANDLE_GET_TYPE(hdc);
    PDEVCAPS pDevCaps = GdiDevCaps; // Primary display device capabilities.
    DPRINT("Device CAPS1\n");

    HANDLE_METADC16(INT, GetDeviceCaps, 0, hdc, nIndex);

    if ( hType != GDILoObjType_LO_DC_TYPE && hType != GDILoObjType_LO_METADC16_TYPE )
    {
        pldc = GdiGetLDC(hdc);
        if ( !pldc )
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return 0;
        }
        if (!(pldc->Flags & LDC_DEVCAPS) )
        {
            if (!NtGdiGetDeviceCapsAll(hdc, &pldc->DevCaps) )
                SetLastError(ERROR_INVALID_PARAMETER);

            pldc->Flags |= LDC_DEVCAPS;
        }
        pDevCaps = &pldc->DevCaps;
    }
    else
    {
        /* Get the DC attribute */
        pdcattr = GdiGetDcAttr(hdc);
        if ( pdcattr == NULL )
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }

        if (!(pdcattr->ulDirty_ & DC_PRIMARY_DISPLAY))
            return NtGdiGetDeviceCaps(hdc, nIndex);
    }

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
    return GetAndSetDCDWord(hdc, GdiGetSetRelAbs, Mode, 0, META_SETRELABS, 0);
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
    PLDC pldc;

    if ( GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_DC_TYPE &&
         ulMFId != EMR_MAX + 1 )
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE)
        {
            return METADC_SetD( hdc, dwIn, usMF16Id );
        }
        pldc = GdiGetLDC(hdc);
        if ( pldc->iType == LDC_EMFLDC)
        {
            if (!EMFDC_SetD( pldc, dwIn, ulMFId ))
                return 0;
        }
    }

    /* Call win32k to do the real work */
    if (!NtGdiGetAndSetDCDword(hdc, u, dwIn, &dwResult))
    {
        return dwError;
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
UINT
WINAPI
GetBoundsRect(
    HDC	hdc,
    LPRECT	lprcBounds,
    UINT	flags
)
{
    return NtGdiGetBoundsRect(hdc,lprcBounds,flags & ~DCB_WINDOWMGR);
}


/*
 * @implemented
 */
UINT
WINAPI
SetBoundsRect(HDC hdc,
              CONST RECT *prc,
              UINT flags)
{
    /* FIXME add check for validate the flags */
    return NtGdiSetBoundsRect(hdc, (LPRECT)prc, flags & ~DCB_WINDOWMGR);
}


/*
 * @implemented
 *
 */
int
WINAPI
GetClipBox(HDC hdc,
           LPRECT lprc)
{
    return NtGdiGetAppClipBox(hdc, lprc);
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
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return CLR_INVALID;
    }

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

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return CLR_INVALID;
    }

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
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return CLR_INVALID;
    }

    /* We handle only enhanced meta DCs here */
    HANDLE_EMETAFDC(COLORREF, SetDCBrushColor, CLR_INVALID, hdc, crColor);

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

    /* We handle only enhanced meta DCs here */
    HANDLE_EMETAFDC(COLORREF, SetDCPenColor, CLR_INVALID, hdc, crColor);

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
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Don't set LastError here! */
        return CLR_INVALID;
    }

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

    HANDLE_METADC(COLORREF, SetBkColor, CLR_INVALID, hdc, crColor);

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return CLR_INVALID;
    }

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
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Don't set LastError here! */
        return 0;
    }

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

    HANDLE_METADC(INT, SetBkMode, 0, hdc, iBkMode);

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

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
GetROP2(
    _In_ HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Do not set LastError here! */
        return 0;
    }

    return pdcattr->jROP2;
}

/*
 * @implemented
 */
int
WINAPI
SetROP2(
    _In_ HDC hdc,
    _In_ int rop2)
{
    PDC_ATTR pdcattr;
    INT rop2Old;

    HANDLE_METADC(INT, SetROP2, 0, hdc, rop2);

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (NtCurrentTeb()->GdiTebBatch.HDC == hdc)
    {
        if (pdcattr->ulDirty_ & DC_MODE_DIRTY)
        {
            NtGdiFlush();
            pdcattr->ulDirty_ &= ~DC_MODE_DIRTY;
        }
    }

    rop2Old = pdcattr->jROP2;
    pdcattr->jROP2 = (BYTE)rop2;

    return rop2Old;
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
        /* Don't set LastError here! */
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

    HANDLE_METADC(INT, SetPolyFillMode, 0, hdc, iPolyFillMode);

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (NtCurrentTeb()->GdiTebBatch.HDC == hdc)
    {
        if (pdcattr->ulDirty_ & DC_MODE_DIRTY)
        {
            NtGdiFlush(); // Sync up pdcattr from Kernel space.
            pdcattr->ulDirty_ &= ~DC_MODE_DIRTY;
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

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Don't set LastError here! */
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

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
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
            pdcattr->ulDirty_ &= ~DC_MODE_DIRTY;
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
    if (GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_DC_TYPE)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE)
        {
           return (HPALETTE)((ULONG_PTR)METADC_SelectPalette(hdc, hpal));
        }
        else
        {
           PLDC pLDC = GdiGetLDC(hdc);
           if ( !pLDC )
           {
              SetLastError(ERROR_INVALID_HANDLE);
              return NULL;
           }
           if ( pLDC->iType == LDC_EMFLDC && !(EMFDC_SelectPalette(pLDC, hpal)) )
           {
              return NULL;
           }
        }
    }
    return NtUserSelectPalette(hdc, hpal, bForceBackground);
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
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Don't set LastError here! */
        return 0;
    }

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

    HANDLE_METADC(INT, SetStretchBltMode, 0, hdc, iStretchMode);

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

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
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Don't set LastError here! */
        return NULL;
    }

    /* Return the current font */
    return pdcattr->hlfntNew;
}



HBITMAP
WINAPI
GdiSelectBitmap(
    _In_ HDC hdc,
    _In_ HBITMAP hbmp)
{
    return NtGdiSelectBitmap(hdc, hbmp);
}

HBRUSH
WINAPI
GdiSelectBrush(
    _In_ HDC hdc,
    _In_ HBRUSH hbr)
{
    PDC_ATTR pdcattr;
    HBRUSH hbrOld;

    HANDLE_METADC(HBRUSH, SelectObject, NULL, hdc, hbr);

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Get the current brush. If it matches the new brush, we're done */
    hbrOld = pdcattr->hbrush;
    if (hbrOld == hbr)
        return hbrOld;

    /* Set the new brush and update dirty flags */
    pdcattr->hbrush = hbr;
    pdcattr->ulDirty_ |= DC_BRUSH_DIRTY;
    return hbrOld;
}

HPEN
WINAPI
GdiSelectPen(
    _In_ HDC hdc,
    _In_ HPEN hpen)
{
    PDC_ATTR pdcattr;
    HPEN hpenOld;

    HANDLE_METADC(HPEN, SelectObject, NULL, hdc, hpen);

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    /* Get the current pen. If it matches the new pen, we're done */
    hpenOld = pdcattr->hpen;
    if (hpenOld == hpen)
        return hpenOld;

    /* Set the new pen and update dirty flags */
    pdcattr->ulDirty_ |= DC_PEN_DIRTY;
    pdcattr->hpen = hpen;
    return hpenOld;
}

HFONT
WINAPI
GdiSelectFont(
    _In_ HDC hdc,
    _In_ HFONT hfont)
{
    PDC_ATTR pdcattr;
    HFONT hfontOld;

    HANDLE_METADC(HFONT, SelectObject, NULL, hdc, hfont);

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Get the current font. If it matches the new font, we're done */
    hfontOld = pdcattr->hlfntNew;
    if (hfontOld == hfont)
        return hfontOld;

    /* Set the new font and update dirty flags */
    pdcattr->hlfntNew = hfont;
    pdcattr->ulDirty_ &= ~SLOW_WIDTHS;
    pdcattr->ulDirty_ |= DIRTY_CHARSET;

    /* If the DC does not have a DIB section selected, try a batch command */
    if (!(pdcattr->ulDirty_ & DC_DIBSECTION))
    {
        PGDIBSOBJECT pgO;

        pgO = GdiAllocBatchCommand(hdc, GdiBCSelObj);
        if (pgO)
        {
            pgO->hgdiobj = hfont;
            return hfontOld;
        }
    }

    /* We could not use the batch command, call win32k */
    return NtGdiSelectFont(hdc, hfont);
}


/*
 * @implemented
 *
 */
HGDIOBJ
WINAPI
SelectObject(
    _In_ HDC hdc,
    _In_ HGDIOBJ hobj)
{
    /* Fix up 16 bit handles */
    hobj = GdiFixUpHandle(hobj);
    if (!GdiValidateHandle(hobj))
    {
        return NULL;
    }

    /* Call the appropriate select function */
    switch (GDI_HANDLE_GET_TYPE(hobj))
    {
        case GDILoObjType_LO_REGION_TYPE:
            return (HGDIOBJ)UlongToHandle(ExtSelectClipRgn(hdc, hobj, RGN_COPY));

        case GDILoObjType_LO_BITMAP_TYPE:
        case GDILoObjType_LO_DIBSECTION_TYPE:
            return GdiSelectBitmap(hdc, hobj);

        case GDILoObjType_LO_BRUSH_TYPE:
            return GdiSelectBrush(hdc, hobj);

        case GDILoObjType_LO_PEN_TYPE:
        case GDILoObjType_LO_EXTPEN_TYPE:
            return GdiSelectPen(hdc, hobj);

        case GDILoObjType_LO_FONT_TYPE:
            return GdiSelectFont(hdc, hobj);

        case GDILoObjType_LO_ICMLCS_TYPE:
            return SetColorSpace(hdc, hobj);

        case GDILoObjType_LO_PALETTE_TYPE:
            SetLastError(ERROR_INVALID_FUNCTION);

        default:
            return NULL;
    }

    return NULL;
}

/***********************************************************************
 *           D3DKMTCreateDCFromMemory    (GDI32.@)
 */
DWORD WINAPI D3DKMTCreateDCFromMemory( D3DKMT_CREATEDCFROMMEMORY *desc )
{
    return NtGdiDdDDICreateDCFromMemory( desc );
}

DWORD WINAPI D3DKMTDestroyDCFromMemory( const D3DKMT_DESTROYDCFROMMEMORY *desc )
{
    return NtGdiDdDDIDestroyDCFromMemory( desc );
}
