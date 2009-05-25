/* $Id$
 *
 * reactos/lib/gdi32/misc/stubs.c
 *
 * GDI32.DLL Stubs
 *
 * When you implement one of these functions,
 * remove its stub from this file.
 *
 */

#include "precomp.h"

#define SIZEOF_DEVMODEA_300 124
#define SIZEOF_DEVMODEA_400 148
#define SIZEOF_DEVMODEA_500 156
#define SIZEOF_DEVMODEW_300 188
#define SIZEOF_DEVMODEW_400 212
#define SIZEOF_DEVMODEW_500 220

#define UNIMPLEMENTED DbgPrint("GDI32: %s is unimplemented, please try again later.\n", __FUNCTION__);


/*
 * @unimplemented
 */
BOOL
WINAPI
PtInRegion(IN HRGN hrgn,
           int x,
           int y)
{
    /* FIXME some stuff at user mode need be fixed */
    return NtGdiPtInRegion(hrgn,x,y);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
RectInRegion(HRGN hrgn,
             LPCRECT prcl)
{
    /* FIXME some stuff at user mode need be fixed */
    return NtGdiRectInRegion(hrgn, (LPRECT) prcl);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
RestoreDC(IN HDC hdc,
          IN INT iLevel)
{
    /* FIXME Sharememory */
    return NtGdiRestoreDC(hdc, iLevel);
}

/*
 * @unimplemented
 */
INT
WINAPI
SaveDC(IN HDC hdc)
{
    /* FIXME Sharememory */
    return NtGdiSaveDC(hdc);
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


/*
 * @implemented
 */
int
WINAPI
DrawEscape(HDC  hDC,
           INT nEscape,
           INT cbInput,
           LPCSTR lpszInData)
{
  if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_DC)
     return NtGdiDrawEscape(hDC, nEscape, cbInput, (LPSTR) lpszInData);

  if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_METADC)
  {
     PLDC pLDC = GdiGetLDC(hDC);
     if ( pLDC )
     {
        if (pLDC->Flags & LDC_META_PRINT)
        {
//           if (nEscape != QUERYESCSUPPORT)
//              return EMFDRV_WriteEscape(hDC, nEscape, cbInput, lpszInData, EMR_DRAWESCAPE);

           return NtGdiDrawEscape(hDC, nEscape, cbInput, (LPSTR) lpszInData);
        }
     }
     SetLastError(ERROR_INVALID_HANDLE);
  }
  return 0;
}


/*
 * @unimplemented
 */
int
WINAPI
EnumObjects(HDC hdc,
            int nObjectType,
            GOBJENUMPROC lpObjectFunc,
            LPARAM lParam)
{
    switch (nObjectType)
    {
        case OBJ_BRUSH:
        case OBJ_PEN:
            break;

        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
    }

    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
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
    return NtGdiGetBoundsRect(hdc,lprcBounds,flags & DCB_RESET);
}


/*
 * @unimplemented
 */
UINT
WINAPI
GetMetaFileBitsEx(
	HMETAFILE	a0,
	UINT		a1,
	LPVOID		a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
PlayMetaFile(
	HDC		a0,
	HMETAFILE	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
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
    return NtGdiSetBoundsRect(hdc, (LPRECT)prc, flags);
}

/*
 * @unimplemented
 */
HMETAFILE
WINAPI
SetMetaFileBitsEx(
	UINT		size,
	CONST BYTE	*lpData
	)
{
    const METAHEADER *mh_in = (const METAHEADER *)lpData;

    if (size & 1) return 0;

    if (!size || mh_in->mtType != METAFILE_MEMORY || mh_in->mtVersion != 0x300 ||
        mh_in->mtHeaderSize != sizeof(METAHEADER) / 2)
    {
        SetLastError(ERROR_INVALID_DATA);
        return 0;
    }

	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
PlayMetaFileRecord(
	HDC		a0,
	LPHANDLETABLE	a1,
	LPMETARECORD	a2,
	UINT		a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
EnumMetaFile(
	HDC			a0,
	HMETAFILE		a1,
	MFENUMPROC		a2,
	LPARAM			a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
DeleteEnhMetaFile(
	HENHMETAFILE	a0
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
EnumEnhMetaFile(
	HDC		hdc,
	HENHMETAFILE	hmf,
	ENHMFENUMPROC	callback,
	LPVOID		data,
	CONST RECT	*lpRect
	)
{
    if(!lpRect && hdc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @unimplemented
 */
UINT
WINAPI
GetEnhMetaFileBits(
	HENHMETAFILE	a0,
	UINT		a1,
	LPBYTE		a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
UINT
WINAPI
GetEnhMetaFileHeader(
	HENHMETAFILE	a0,
	UINT		a1,
	LPENHMETAHEADER	a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
UINT
WINAPI
GetEnhMetaFilePaletteEntries(
	HENHMETAFILE	a0,
	UINT		a1,
	LPPALETTEENTRY	a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
UINT
WINAPI
GetWinMetaFileBits(
	HENHMETAFILE	a0,
	UINT		a1,
	LPBYTE		a2,
	INT		a3,
	HDC		a4
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
PlayEnhMetaFile(
	HDC		a0,
	HENHMETAFILE	a1,
	CONST RECT	*a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
PlayEnhMetaFileRecord(
	HDC			a0,
	LPHANDLETABLE		a1,
	CONST ENHMETARECORD	*a2,
	UINT			a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
HENHMETAFILE
WINAPI
SetEnhMetaFileBits(
	UINT		a0,
	CONST BYTE	*a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
HENHMETAFILE
WINAPI
SetWinMetaFileBits(
	UINT			a0,
	CONST BYTE		*a1,
	HDC			a2,
	CONST METAFILEPICT	*a3)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GdiComment(
	HDC		hDC,
	UINT		bytes,
	CONST BYTE	*buffer
	)
{
#if 0
  if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_EMF)
  {
     PLDC pLDC = GdiGetLDC(hDC);
     if ( !pLDC )
     {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
     }
     if (pLDC->iType == LDC_EMFLDC)
     {          // Wine port
         return EMFDRV_GdiComment( hDC, bytes, buffer );
     }
  }
#endif
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
SetColorAdjustment(
	HDC			hdc,
	CONST COLORADJUSTMENT	*a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
UnrealizeObject(HGDIOBJ  hgdiobj)
{
    BOOL retValue = TRUE;
/*
   Win 2k Graphics API, Black Book. by coriolis.com
   Page 62, Note that Steps 3, 5, and 6 are not required for Windows NT(tm)
   and Windows 2000(tm).

   Step 5. UnrealizeObject(hTrackBrush);
 */
/*
    msdn.microsoft.com,
    "Windows 2000/XP: If hgdiobj is a brush, UnrealizeObject does nothing,
    and the function returns TRUE. Use SetBrushOrgEx to set the origin of
    a brush."
 */
    if (GDI_HANDLE_GET_TYPE(hgdiobj) != GDI_OBJECT_TYPE_BRUSH)
    {
        retValue = NtGdiUnrealizeObject(hgdiobj);
    }

    return retValue;
}


/*
 * @implemented
 */
BOOL
WINAPI
GdiFlush()
{
    NtGdiFlush();
    return TRUE;
}


/*
 * @unimplemented
 */
int
WINAPI
SetICMMode(
	HDC	hdc,
	int	iEnableICM
	)
{
    /*FIXME:  Assume that ICM is always off, and cannot be turned on */
    if (iEnableICM == ICM_OFF) return ICM_OFF;
    if (iEnableICM == ICM_ON) return 0;
    if (iEnableICM == ICM_QUERY) return ICM_OFF;

	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
CheckColorsInGamut(
	HDC	a0,
	LPVOID	a1,
	LPVOID	a2,
	DWORD	a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetDeviceGammaRamp( HDC hdc,
                    LPVOID lpGammaRamp)
{
    BOOL retValue = FALSE;
    if (lpGammaRamp == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        retValue = NtGdiGetDeviceGammaRamp(hdc,lpGammaRamp);
    }

    return retValue;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetDeviceGammaRamp(HDC hdc,
                   LPVOID lpGammaRamp)
{
    BOOL retValue = FALSE;

    if (lpGammaRamp)
    {
        retValue = NtGdiSetDeviceGammaRamp(hdc, lpGammaRamp);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return  retValue;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
ColorMatchToTarget(
	HDC	a0,
	HDC	a1,
	DWORD	a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
wglCopyContext(
	HGLRC	hglrcSrc,
	HGLRC	hglrcDst,
	UINT	mask
	)
{
    if(!hglrcSrc || !hglrcDst)
        return FALSE;

	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
HGLRC
WINAPI
wglCreateContext(
	HDC	hDc
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
HGLRC
WINAPI
wglCreateLayerContext(
	HDC	hDc,
	int	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
wglDeleteContext(
	HGLRC	hglrc
	)
{
    if (hglrc == NULL) return FALSE;

	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
HGLRC
WINAPI
wglGetCurrentContext(VOID)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
HDC
WINAPI
wglGetCurrentDC(VOID)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
PROC
WINAPI
wglGetProcAddress(
	LPCSTR		func
	)
{
    if(!func) return NULL;

	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
wglMakeCurrent(
	HDC	a0,
	HGLRC	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
wglShareLists(
	HGLRC	hglrc1,
	HGLRC	hglrc2
	)
{
    if (hglrc1 == NULL) return FALSE;
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
wglDescribeLayerPlane(
	HDC			a0,
	int			a1,
	int			a2,
	UINT			a3,
	LPLAYERPLANEDESCRIPTOR	a4
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
int
WINAPI
wglSetLayerPaletteEntries(
	HDC		a0,
	int		a1,
	int		a2,
	int		a3,
	CONST COLORREF	*a4
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
WINAPI
wglGetLayerPaletteEntries(
	HDC		a0,
	int		a1,
	int		a2,
	int		a3,
	COLORREF	*a4
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
wglRealizeLayerPalette(
	HDC		a0,
	int		a1,
	BOOL		a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
wglSwapLayerBuffers(
	HDC		a0,
	UINT		a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* === AFTER THIS POINT I GUESS... =========
 * (based on stack size in Norlander's .def)
 * === WHERE ARE THEY DEFINED? =============
 */

/*
 * @unimplemented
 */
DWORD
WINAPI
IsValidEnhMetaRecord(
	DWORD	a0,
	DWORD	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;

}

/*
 * @unimplemented
 */
DWORD
WINAPI
IsValidEnhMetaRecordOffExt(
	DWORD	a0,
	DWORD	a1,
	DWORD	a2,
	DWORD	a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;

}

/*
 * @unimplemented
 */
DWORD
WINAPI
GetGlyphOutlineWow(
	DWORD	a0,
	DWORD	a1,
	DWORD	a2,
	DWORD	a3,
	DWORD	a4,
	DWORD	a5,
	DWORD	a6
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
gdiPlaySpoolStream(
	DWORD	a0,
	DWORD	a1,
	DWORD	a2,
	DWORD	a3,
	DWORD	a4,
	DWORD	a5
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @implemented
 */
HANDLE
WINAPI
AddFontMemResourceEx(
	PVOID pbFont,
	DWORD cbFont,
	PVOID pdv,
	DWORD *pcFonts
)
{
  if ( pbFont && cbFont && pcFonts)
  {
     return NtGdiAddFontMemResourceEx(pbFont, cbFont, NULL, 0, pcFonts);
  }
  SetLastError(ERROR_INVALID_PARAMETER);
  return NULL;
}

/*
 * @unimplemented
 */
int
WINAPI
AddFontResourceTracking(
	LPCSTR lpString,
	int unknown
)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



/*
 * @unimplemented
 */
HBITMAP
WINAPI
ClearBitmapAttributes(HBITMAP hbm, DWORD dwFlags)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBRUSH
WINAPI
ClearBrushAttributes(HBRUSH hbm, DWORD dwFlags)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
ColorCorrectPalette(HDC hDC,HPALETTE hPalette,DWORD dwFirstEntry,DWORD dwNumOfEntries)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiArtificialDecrementDriver(LPWSTR pDriverName,BOOL unknown)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiCleanCacheDC(HDC hdc)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
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
 * @unimplemented
 */
HENHMETAFILE
WINAPI
GdiConvertEnhMetaFile(HENHMETAFILE hmf)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiDrawStream(HDC dc, ULONG l, VOID *v)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiIsMetaFileDC(HDC hDC)
{
  if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
  {
     if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
        return TRUE;
     else
     {
        PLDC pLDC = GdiGetLDC(hDC);
        if ( !pLDC )
        {
           SetLastError(ERROR_INVALID_HANDLE);
           return FALSE;
        }
        if ( pLDC->iType == LDC_EMFLDC) return TRUE;
     }
  }
  return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiIsMetaPrintDC(HDC hDC)
{

  if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
  {
     if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
        return FALSE;
     else
     {
        PLDC pLDC = GdiGetLDC(hDC);
        if ( !pLDC )
        {
           SetLastError(ERROR_INVALID_HANDLE);
           return FALSE;
        }
        if ( pLDC->Flags & LDC_META_PRINT) return TRUE;
     }
  }
  return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiIsPlayMetafileDC(HDC hDC)
{
  PLDC pLDC = GdiGetLDC(hDC);
  if ( pLDC )
  {
     if ( pLDC->Flags & LDC_PLAY_MFDC ) return TRUE;
  }
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiValidateHandle(HGDIOBJ hobj)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
GetBitmapAttributes(HBITMAP hbm)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
GetBrushAttributes(HBRUSH hbr)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @implemented
 */
ULONG
WINAPI
GetEUDCTimeStamp(VOID)
{
    return NtGdiGetEudcTimeStampEx(NULL,0,TRUE);
}

/*
 * @implemented
 */
ULONG
WINAPI
GetFontAssocStatus(HDC hdc)
{
    ULONG retValue = 0;

    if (hdc)
    {
        retValue = NtGdiQueryFontAssocInfo(hdc);
    }

    return retValue;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetTextExtentExPointWPri(HDC hdc,
                         LPWSTR lpwsz,
                         ULONG cwc,
                         ULONG dxMax,
                         ULONG *pcCh,
                         PULONG pdxOut,
                         LPSIZE psize)
{
    return NtGdiGetTextExtentExW(hdc,lpwsz,cwc,dxMax,pcCh,pdxOut,psize,0);
}

/*
 * @unimplemented
 */
DWORD
WINAPI
QueryFontAssocStatus(VOID)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
RemoveFontMemResourceEx(HANDLE fh)
{
  if (fh)
  {
     return NtGdiRemoveFontMemResourceEx(fh);
  }
  SetLastError(ERROR_INVALID_PARAMETER);
  return FALSE;
}

/*
 * @unimplemented
 */
int
WINAPI
RemoveFontResourceTracking(LPCSTR lpString,int unknown)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBITMAP
WINAPI
SetBitmapAttributes(HBITMAP hbm, DWORD dwFlags)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBRUSH
WINAPI
SetBrushAttributes(HBRUSH hbm, DWORD dwFlags)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @implemented
 */
int
WINAPI
StartFormPage(HDC hdc)
{
    return StartPage(hdc);
}

/*
 * @unimplemented
 */
VOID
WINAPI
UnloadNetworkFonts(DWORD unknown)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiRealizationInfo(HDC hdc,
                   PREALIZATION_INFO pri)
{
    // ATM we do not support local font data and Language Pack.
    return NtGdiGetRealizationInfo(hdc, pri, (HFONT) NULL);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetETM(HDC hdc,
       EXTTEXTMETRIC *petm)
{
  BOOL Ret = NtGdiGetETM(hdc, petm);

  if (Ret && petm)
    petm->emKernPairs = GetKerningPairsA(hdc, 0, 0);

  return Ret;
}

/*
 * @unimplemented
 */
int
WINAPI
Escape(HDC hdc, INT nEscape, INT cbInput, LPCSTR lpvInData, LPVOID lpvOutData)
{
    int retValue = SP_ERROR;    
    HGDIOBJ hObject = hdc;
    UINT Type = 0;
    LPVOID pUserData = NULL;

    Type = GDI_HANDLE_GET_TYPE(hObject);

    if (Type == GDI_OBJECT_TYPE_METADC)
    {
        /* FIXME we do not support metafile */
        UNIMPLEMENTED;
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    }
    else
    {
        switch (nEscape)
        {
            case ABORTDOC:        
                /* Note Winodws check see if the handle have any user data for ABORTDOC command 
                 * ReactOS copy this behavior to be compatible with windows 2003 
                 */
                if ( (!GdiGetHandleUserData(hObject, (DWORD)Type, (PVOID) &pUserData)) ||  
                      (pUserData == NULL) ) 
                 {
                     GdiSetLastError(ERROR_INVALID_HANDLE);
                     retValue = FALSE;
                 }
                 else
                 {
                    retValue = AbortDoc(hdc);
                 }
                break;

            case DRAFTMODE:
            case FLUSHOUTPUT:
            case SETCOLORTABLE:
                /* Note 1: DRAFTMODE, FLUSHOUTPUT, SETCOLORTABLE is outdated and been replace with other api */
                /* Note 2: Winodws check see if the handle have any user data for DRAFTMODE, FLUSHOUTPUT, SETCOLORTABLE command 
                 * ReactOS copy this behavior to be compatible with windows 2003 
                 */
                if ( (!GdiGetHandleUserData(hObject, (DWORD)Type, (PVOID) &pUserData)) ||  
                     (pUserData == NULL) ) 
                {
                    GdiSetLastError(ERROR_INVALID_HANDLE);
                }
                retValue = FALSE;
                break;

            case SETABORTPROC:
                /* Note : Winodws check see if the handle have any user data for DRAFTMODE, FLUSHOUTPUT, SETCOLORTABLE command 
                 * ReactOS copy this behavior to be compatible with windows 2003 
                 */
                if ( (!GdiGetHandleUserData(hObject, (DWORD)Type, (PVOID) &pUserData)) ||  
                     (pUserData == NULL) ) 
                {
                    GdiSetLastError(ERROR_INVALID_HANDLE);
                    retValue = FALSE;
                }
                retValue = SetAbortProc(hdc, (ABORTPROC)lpvInData);
                break;

            case GETCOLORTABLE:
                retValue = GetSystemPaletteEntries(hdc, (UINT)*lpvInData, 1, (LPPALETTEENTRY)lpvOutData);
                if ( !retValue )
                {
                    retValue = SP_ERROR;        
                }            
                break;

            case ENDDOC:
                /* Note : Winodws check see if the handle have any user data for DRAFTMODE, FLUSHOUTPUT, SETCOLORTABLE command 
                 * ReactOS copy this behavior to be compatible with windows 2003 
                 */
                if ( (!GdiGetHandleUserData(hObject, (DWORD)Type, (PVOID) &pUserData)) ||  
                     (pUserData == NULL) ) 
                {
                    GdiSetLastError(ERROR_INVALID_HANDLE);
                    retValue = FALSE;
                }
                retValue = EndDoc(hdc);
                break;


            case GETSCALINGFACTOR:
                /* Note GETSCALINGFACTOR is outdated have been replace by GetDeviceCaps */
                if ( Type == GDI_OBJECT_TYPE_DC )
                {                
                    if ( lpvOutData )
                    {
                        PPOINT ptr = (PPOINT) lpvOutData;
                        ptr->x = 0;
                        ptr->y = 0;                            
                    }
                }                                
                retValue = FALSE;
                break;

            case GETEXTENDEDTEXTMETRICS:
                retValue = (int) GetETM( hdc, (EXTTEXTMETRIC *) lpvOutData) != 0;
                break;

            case  STARTDOC:
                {
                    DOCINFOA *pUserDatalpdi;
                    DOCINFOA lpdi;

                    /* Note : Winodws check see if the handle have any user data for STARTDOC command 
                     * ReactOS copy this behavior to be compatible with windows 2003 
                     */
                    if ( (!GdiGetHandleUserData(hObject, (DWORD)Type, (PVOID) &pUserDatalpdi)) ||  
                         (pUserData == NULL) ) 
                    {
                        GdiSetLastError(ERROR_INVALID_HANDLE);
                        retValue = FALSE;
                    }

                    lpdi.cbSize = sizeof(DOCINFOA);

                    /* NOTE lpszOutput will be store in handle userdata */
                    lpdi.lpszOutput = 0;

                    lpdi.lpszDatatype = 0;
                    lpdi.fwType = 0;
                    lpdi.lpszDocName = lpvInData;

                    /* NOTE : doc for StartDocA/W at msdn http://msdn2.microsoft.com/en-us/library/ms535793(VS.85).aspx */
                    retValue = StartDocA(hdc, &lpdi);  

                    /* StartDocA fail */
                    if (retValue < 0)
                    {                                        
                        /* check see if outbuffer contain any data, if it does abort */ 
                        if  ( (pUserDatalpdi->lpszOutput != 0) && 
                              ( (*(WCHAR *)pUserDatalpdi->lpszOutput) != UNICODE_NULL) )
                        {
                            retValue = SP_APPABORT;
                        }
                        else
                        {
                            retValue = GetLastError();
                         
                            /* Translate StartDocA error code to STARTDOC error code 
                             * see msdn http://msdn2.microsoft.com/en-us/library/ms535472.aspx 
                             */
                            switch(retValue)
                            {
                                case ERROR_NOT_ENOUGH_MEMORY:
                                    retValue = SP_OUTOFMEMORY;
                                    break;

                                case ERROR_PRINT_CANCELLED:
                                    retValue = SP_USERABORT;
                                    break;

                                case ERROR_DISK_FULL:
                                    retValue = SP_OUTOFDISK;
                                    break;

                                default:
                                    retValue = SP_ERROR;
                                    break;
                            }                         
                        }                                                  
                    }
                }
                break;
                
            


            default:
                UNIMPLEMENTED;
                SetLastError(ERROR_CALL_NOT_IMPLEMENTED);                
        }
    }
    
    return retValue;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiAddGlsRecord(HDC hdc,
                DWORD unknown1,
                LPCSTR unknown2,
                LPRECT unknown3)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
GdiConvertMetaFilePict(HGLOBAL hMem)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @implemented
 */
DEVMODEW *
WINAPI
GdiConvertToDevmodeW(DEVMODEA *dmA)
{
    DEVMODEW *dmW;
    WORD dmW_size, dmA_size;

    dmA_size = dmA->dmSize;

    /* this is the minimal dmSize that XP accepts */
    if (dmA_size < FIELD_OFFSET(DEVMODEA, dmFields))
        return NULL;

    if (dmA_size > sizeof(DEVMODEA))
        dmA_size = sizeof(DEVMODEA);

    dmW_size = dmA_size + CCHDEVICENAME;
    if (dmA_size >= FIELD_OFFSET(DEVMODEA, dmFormName) + CCHFORMNAME)
        dmW_size += CCHFORMNAME;

    dmW = HeapAlloc(GetProcessHeap(), 0, dmW_size + dmA->dmDriverExtra);
    if (!dmW) return NULL;

    MultiByteToWideChar(CP_ACP, 0, (const char*) dmA->dmDeviceName, CCHDEVICENAME,
                                   dmW->dmDeviceName, CCHDEVICENAME);
    /* copy slightly more, to avoid long computations */
    memcpy(&dmW->dmSpecVersion, &dmA->dmSpecVersion, dmA_size - CCHDEVICENAME);

    if (dmA_size >= FIELD_OFFSET(DEVMODEA, dmFormName) + CCHFORMNAME)
    {
        MultiByteToWideChar(CP_ACP, 0, (const char*) dmA->dmFormName, CCHFORMNAME,
                                       dmW->dmFormName, CCHFORMNAME);
        if (dmA_size > FIELD_OFFSET(DEVMODEA, dmLogPixels))
            memcpy(&dmW->dmLogPixels, &dmA->dmLogPixels, dmA_size - FIELD_OFFSET(DEVMODEA, dmLogPixels));
    }

    if (dmA->dmDriverExtra)
        memcpy((char *)dmW + dmW_size, (const char *)dmA + dmA_size, dmA->dmDriverExtra);

    dmW->dmSize = dmW_size;

    return dmW;
}

/*
 * @unimplemented
 */
HENHMETAFILE
WINAPI
GdiCreateLocalEnhMetaFile(HENHMETAFILE hmo)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
METAFILEPICT *
WINAPI
GdiCreateLocalMetaFilePict(HENHMETAFILE hmo)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
HANDLE
WINAPI
GdiGetSpoolFileHandle(LPWSTR pwszPrinterName,
                      LPDEVMODEW pDevmode,
                      LPWSTR pwszDocName)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiDeleteSpoolFileHandle(HANDLE SpoolFileHandle)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
GdiGetPageCount(HANDLE SpoolFileHandle)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HDC
WINAPI
GdiGetDC(HANDLE SpoolFileHandle)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
GdiGetPageHandle(HANDLE SpoolFileHandle,
                 DWORD Page,
                 LPDWORD pdwPageType)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiStartDocEMF(HANDLE SpoolFileHandle,
               DOCINFOW *pDocInfo)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiStartPageEMF(HANDLE SpoolFileHandle)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiPlayPageEMF(HANDLE SpoolFileHandle,
               HANDLE hemf,
               RECT *prectDocument,
               RECT *prectBorder,
               RECT *prectClip)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiEndPageEMF(HANDLE SpoolFileHandle,
              DWORD dwOptimization)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiEndDocEMF(HANDLE SpoolFileHandle)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiGetDevmodeForPage(HANDLE SpoolFileHandle,
                     DWORD dwPageNumber,
                     PDEVMODEW *pCurrDM,
                     PDEVMODEW *pLastDM)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiResetDCEMF(HANDLE SpoolFileHandle,
              PDEVMODEW pCurrDM)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
INT
WINAPI
CombineRgn(HRGN  hDest,
           HRGN  hSrc1,
           HRGN  hSrc2,
           INT  CombineMode)
{
    /* FIXME some part should be done in user mode */
    return NtGdiCombineRgn(hDest, hSrc1, hSrc2, CombineMode);
}

/*
 * @unimplemented
 */
ULONG WINAPI
XLATEOBJ_iXlate(XLATEOBJ *XlateObj,
                ULONG Color)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
ULONG *
WINAPI
XLATEOBJ_piVector(XLATEOBJ *XlateObj)
{
    return XlateObj->pulXlate;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiPlayEMF(LPWSTR pwszPrinterName,
           LPDEVMODEW pDevmode,
           LPWSTR pwszDocName,
           EMFPLAYPROC pfnEMFPlayFn,
           HANDLE hPageQuery
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}



/*
 * @unimplemented
 */
BOOL
WINAPI
GdiPlayPrivatePageEMF(HANDLE SpoolFileHandle,
                      DWORD unknown,
                      RECT *prectDocument)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
VOID WINAPI GdiInitializeLanguagePack(DWORD InitParam)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}


/*
 * @implemented
 */
INT
WINAPI
ExcludeClipRect(IN HDC hdc, IN INT xLeft, IN INT yTop, IN INT xRight, IN INT yBottom)
{
    /* FIXME some part need be done on user mode size */
    return NtGdiExcludeClipRect(hdc, xLeft, yTop, xRight, yBottom);
}

/*
 * @implemented
 */
INT
WINAPI
ExtSelectClipRgn( IN HDC hdc, IN HRGN hrgn, IN INT iMode)
{
    /* FIXME some part need be done on user mode size */
    return NtGdiExtSelectClipRgn(hdc,hrgn, iMode);
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiGradientFill(
    IN HDC hdc,
    IN PTRIVERTEX pVertex,
    IN ULONG nVertex,
    IN PVOID pMesh,
    IN ULONG nMesh,
    IN ULONG ulMode)
{
    /* FIXME some part need be done in user mode */
    return NtGdiGradientFill(hdc, pVertex, nVertex, pMesh, nMesh, ulMode);
}


/*
 * @implemented
 */
BOOL
WINAPI
GdiTransparentBlt(IN HDC hdcDst,
                  IN INT xDst,
                  IN INT yDst,
                  IN INT cxDst,
                  IN INT cyDst,
                  IN HDC hdcSrc,
                  IN INT xSrc,
                  IN INT ySrc,
                  IN INT cxSrc,
                  IN INT cySrc,
                  IN COLORREF TransColor
)
{
    /* FIXME some part need be done in user mode */
    return NtGdiTransparentBlt(hdcDst, xDst, yDst, cxDst, cyDst, hdcSrc, xSrc, ySrc, cxSrc, cySrc, TransColor);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiPrinterThunk(
    IN HUMPD humpd,
    DWORD *status,
    DWORD unuse)
{
    /* FIXME figout the protypes, the HUMPD are a STRUCT or COM object */
    /* status contain some form of return value that being save, what it is I do not known */
    /* unsue seam have zero effect, what it is for I do not known */

    // ? return NtGdiSetPUMPDOBJ(humpd->0x10,TRUE, humpd, ?) <- blackbox, OpenRCE info, and api hooks for anylaysing;
    return FALSE;
}

/*
 * @unimplemented
 *
 */
HBITMAP
WINAPI
GdiConvertBitmapV5(
    HBITMAP in_format_BitMap,
    HBITMAP src_BitMap,
    INT bpp,
    INT unuse)
{
    /* FIXME guessing the prototypes */

    /*
     * it have create a new bitmap with desired in format,
     * then convert it src_bitmap to new format
     * and return it as HBITMAP
     */

    return FALSE;
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
    return  NtGdiGetAppClipBox(hdc, lprc);
}

/*
 * @implemented
 *
 */
DWORD
WINAPI
GetFontData(HDC hdc,
            DWORD dwTable,
            DWORD dwOffset,
            LPVOID lpvBuffer,
            DWORD cbData)
{
    if (!lpvBuffer)
    {
       cbData = 0;
    }
    return NtGdiGetFontData(hdc, dwTable, dwOffset, lpvBuffer, cbData);
}


/*
 * @implemented
 *
 */
DWORD
WINAPI
GetRegionData(HRGN hrgn,
              DWORD nCount,
              LPRGNDATA lpRgnData)
{
    if (!lpRgnData)
    {
        nCount = 0;
    }

    return NtGdiGetRegionData(hrgn,nCount,lpRgnData);
}


/*
 * @implemented
 *
 */
INT
WINAPI
GetRgnBox(HRGN hrgn,
          LPRECT prcOut)
{
#if 0
  PRGN_ATTR Rgn_Attr;
  if (!GdiGetHandleUserData((HGDIOBJ) hRgn, GDI_OBJECT_TYPE_REGION, (PVOID) &Rgn_Attr))
     return NtGdiGetRgnBox(hrgn, prcOut);
  if (Rgn_Attr->Flags == NULLREGION)
  {
     prcOut->left   = 0;
     prcOut->top    = 0;
     prcOut->right  = 0;
     prcOut->bottom = 0;
  }
  else
  {
     if (Rgn_Attr->Flags != SIMPLEREGION) return NtGdiGetRgnBox(hrgn, prcOut);
     *prcOut = Rgn_Attr->Rect;
  }
  return Rgn_Attr->Flags;
#endif
  return NtGdiGetRgnBox(hrgn, prcOut);
}


/*
 * @implemented
 *
 */
INT
WINAPI
OffsetRgn( HRGN hrgn,
          int nXOffset,
          int nYOffset)
{
    /* FIXME some part are done in user mode */
    return NtGdiOffsetRgn(hrgn,nXOffset,nYOffset);
}

/*
 * @implemented
 */
INT
WINAPI
IntersectClipRect(HDC hdc,
                  int nLeftRect,
                  int nTopRect,
                  int nRightRect,
                  int nBottomRect)
{
#if 0
// Handle something other than a normal dc object.
  if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_IntersectClipRect( hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
    else
    {
      PLDC pLDC = GdiGetLDC(hdc);
      if ( pLDC )
      {
         if (pLDC->iType != LDC_EMFLDC || EMFDRV_IntersectClipRect( hdc, nLeftRect, nTopRect, nRightRect, nBottomRect))
             return NtGdiIntersectClipRect(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
      }
      else
        SetLastError(ERROR_INVALID_HANDLE);
      return 0;
    }
  }
#endif
    return NtGdiIntersectClipRect(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
}

/*
 * @implemented
 */
INT
WINAPI
OffsetClipRgn(HDC hdc,
              int nXOffset,
              int nYOffset)
{
#if 0
// Handle something other than a normal dc object.
  if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_OffsetClipRgn( hdc, nXOffset, nYOffset );
    else
    {
      PLDC pLDC = GdiGetLDC(hdc);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return 0;
      }
      if (pLDC->iType == LDC_EMFLDC && !EMFDRV_OffsetClipRgn( hdc, nXOffset, nYOffset ))
         return 0;
      return NtGdiOffsetClipRgn( hdc,  nXOffset,  nYOffset);
    }
  }
#endif
  return NtGdiOffsetClipRgn( hdc,  nXOffset,  nYOffset);
}


INT
WINAPI
NamedEscape(HDC hdc,
            PWCHAR pDriver,
            INT iEsc,
            INT cjIn,
            LPSTR pjIn,
            INT cjOut,
            LPSTR pjOut)
{
    /* FIXME metadc, metadc are done most in user mode, and we do not support it
     * Windows 2000/XP/Vista ignore the current hdc, that are being pass and always set hdc to NULL
     * when it calls to NtGdiExtEscape from NamedEscape
     */
    return NtGdiExtEscape(NULL,pDriver,wcslen(pDriver),iEsc,cjIn,pjIn,cjOut,pjOut);
}



/*
 * @unimplemented
 */

/* FIXME wrong protypes, it is a fastcall api */
DWORD
WINAPI
cGetTTFFromFOT(DWORD x1 ,DWORD x2 ,DWORD x3, DWORD x4, DWORD x5, DWORD x6, DWORD x7)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

