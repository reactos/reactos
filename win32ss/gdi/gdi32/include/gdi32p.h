/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            win32ss/gdi/gdi32/include/gdi32p.h
 * PURPOSE:         User-Mode Win32 GDI Library Private Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#pragma once

/* DATA **********************************************************************/

extern PGDI_TABLE_ENTRY GdiHandleTable;
extern PGDI_SHARED_HANDLE_TABLE GdiSharedHandleTable;
extern HANDLE hProcessHeap;
extern HANDLE CurrentProcessId;
extern DWORD GDI_BatchLimit;
extern PDEVCAPS GdiDevCaps;
extern BOOL gbLpk;          // Global bool LanguagePack
extern HANDLE ghSpooler;
extern RTL_CRITICAL_SECTION semLocal;

typedef INT
(CALLBACK* EMFPLAYPROC)(
    HDC hdc,
    INT iFunction,
    HANDLE hPageQuery
);

typedef BOOL
(WINAPI* LPKETO)(
    HDC hdc,
    int x,
    int y,
    UINT fuOptions,
    const RECT *lprc,
    LPCWSTR lpString,
    UINT uCount,
    const INT *lpDx,
    INT unknown
);

typedef DWORD
(WINAPI* LPKGCP)(
    HDC hdc,
    LPCWSTR lpString,
    INT uCount,
    INT nMaxExtent,
    LPGCP_RESULTSW lpResults,
    DWORD dwFlags,
    DWORD dwUnused
);

typedef BOOL
(WINAPI* LPKGTEP)(
    HDC hdc,
    LPCWSTR lpString,
    INT cString,
    INT nMaxExtent,
    LPINT lpnFit,
    LPINT lpnDx,
    LPSIZE lpSize,
    DWORD dwUnused,
    int unknown
);

extern HINSTANCE hLpk;
extern LPKETO LpkExtTextOut;
extern LPKGCP LpkGetCharacterPlacement;
extern LPKGTEP LpkGetTextExtentExPoint;

/* DEFINES *******************************************************************/

#define HANDLE_LIST_INC 20

#define METAFILE_MEMORY 1
#define METAFILE_DISK   2

#define SAPCALLBACKDELAY 244

#define LPK_INIT 1
#define LPK_ETO  2
#define LPK_GCP  3
#define LPK_GTEP 4

/* MACRO ********************************************************************/

#define ROP_USES_SOURCE(Rop)   (((Rop) << 2 ^ Rop) & 0xCC0000)
#define RCAST(_Type, _Value)   (*((_Type*)&_Value))


/* TYPES *********************************************************************/

// Based on wmfapi.h and Wine.
typedef struct tagMETAFILEDC
{
    PVOID       pvMetaBuffer;
    HANDLE      hFile;
    DWORD       Size;
    DWORD       dwWritten;
    METAHEADER  mh;
    WORD        reserved;
    HLOCAL      MFObjList;
    HPEN        hPen;
    HBRUSH      hBrush;
    HDC         hDc;
    HGDIOBJ     hMetaDc;
    HPALETTE    hPalette;
    HFONT       hFont;
    HBITMAP     hBitmap;
    HRGN        hRegion;
    HGDIOBJ     hMetafile;
    HGDIOBJ     hMemDc;
    HPEN        hExtPen;
    HGDIOBJ     hEnhMetaDc;
    HGDIOBJ     hEnhMetaFile;
    HCOLORSPACE hColorSpace;
    WCHAR       Filename[MAX_PATH+2];
} METAFILEDC,*PMETAFILEDC;

// Metafile Entry handle
typedef struct tagMF_ENTRY
{
    LIST_ENTRY   List;
    HGDIOBJ      hmDC;             // Handle return from NtGdiCreateClientObj.
    PMETAFILEDC pmfDC;
} MF_ENTRY, *PMF_ENTRY;

typedef struct tagENHMETAFILE
{
    PVOID      pvMetaBuffer;
    HANDLE     hFile;      /* Handle for disk based MetaFile */
    DWORD      Size;
    INT        iType;
    PENHMETAHEADER emf;
    UINT       handles_size, cur_handles;
    HGDIOBJ   *handles;
    INT        horzres, vertres;
    INT        horzsize, vertsize;
    INT        logpixelsx, logpixelsy;
    INT        bitspixel;
    INT        textcaps;
    INT        rastercaps;
    INT        technology;
    INT        planes;
} ENHMETAFILE,*PENHMETAFILE;


#define PDEV_UMPD_ID  0xFEDCBA98
// UMPDEV flags
#define UMPDEV_NO_ESCAPE      0x0002
#define UMPDEV_SUPPORT_ESCAPE 0x0004
typedef struct _UMPDEV
{
    DWORD_PTR       Sig;            // Init with PDEV_UMPD_ID
    struct _UMPDEV *pumpdNext;
    PDRIVER_INFO_5W pdi5Info;
    HMODULE         hModule;
    DWORD           dwFlags;
    DWORD           dwDriverAttributes;
    DWORD           dwConfigVersion; // Number of times the configuration
    // file for this driver has been upgraded
    // or downgraded since the last spooler restart.
    DWORD           dwDriverCount;   // After init should be 2
    DWORD           WOW64_UMPDev;
    DWORD           WOW64_hMod;
    DWORD           Unknown;
    PVOID           apfn[INDEX_LAST]; // Print Driver pfn
} UMPDEV, *PUMPDEV;

#define LOCALFONT_COUNT 10
typedef struct _LOCALFONT
{
    FONT_ATTR  lfa[LOCALFONT_COUNT];
} LOCALFONT, *PLOCALFONT;

// sdk/winspool.h
typedef BOOL (WINAPI *ABORTPRINTER) (HANDLE);
typedef BOOL (WINAPI *CLOSEPRINTER) (HANDLE);
typedef BOOL (WINAPI *CLOSESPOOLFILEHANDLE) (HANDLE, HANDLE); // W2k8
typedef HANDLE (WINAPI *COMMITSPOOLDATA) (HANDLE,HANDLE,DWORD); // W2k8
typedef LONG (WINAPI *DOCUMENTPROPERTIESW) (HWND,HANDLE,LPWSTR,PDEVMODEW,PDEVMODEW,DWORD);
typedef BOOL (WINAPI *ENDDOCPRINTER) (HANDLE);
typedef BOOL (WINAPI *ENDPAGEPRINTER) (HANDLE);
typedef BOOL (WINAPI *GETPRINTERW) (HANDLE,DWORD,LPBYTE,DWORD,LPDWORD);
typedef BOOL (WINAPI *GETPRINTERDRIVERW) (HANDLE,LPWSTR,DWORD,LPBYTE,DWORD,LPDWORD);
typedef HANDLE (WINAPI *GETSPOOLFILEHANDLE) (HANDLE); // W2k8
typedef BOOL (WINAPI *ISVALIDDEVMODEW) (PDEVMODEW,size_t);
typedef BOOL (WINAPI *OPENPRINTERW) (LPWSTR,PHANDLE,LPPRINTER_DEFAULTSW);
typedef BOOL (WINAPI *READPRINTER) (HANDLE,PVOID,DWORD,PDWORD);
typedef BOOL (WINAPI *RESETPRINTERW) (HANDLE,LPPRINTER_DEFAULTSW);
typedef LPWSTR (WINAPI *STARTDOCDLGW) (HANDLE,DOCINFOW *);
typedef DWORD (WINAPI *STARTDOCPRINTERW) (HANDLE,DWORD,PBYTE);
typedef BOOL (WINAPI *STARTPAGEPRINTER) (HANDLE);
// ddk/winsplp.h
typedef BOOL (WINAPI *SEEKPRINTER) (HANDLE,LARGE_INTEGER,PLARGE_INTEGER,DWORD,BOOL);
typedef BOOL (WINAPI *SPLREADPRINTER) (HANDLE,LPBYTE *,DWORD);
// Same as ddk/winsplp.h DriverUnloadComplete?
typedef BOOL (WINAPI *SPLDRIVERUNLOADCOMPLETE) (LPWSTR);
// Driver support:
// DrvDocumentEvent api/winddiui.h not W2k8 DocumentEventAW
typedef INT (WINAPI *DOCUMENTEVENT) (HANDLE,HDC,INT,ULONG,PVOID,ULONG,PVOID);
// DrvQueryColorProfile
typedef BOOL (WINAPI *QUERYCOLORPROFILE) (HANDLE,PDEVMODEW,ULONG,VOID*,ULONG,FLONG);
// Unknown:
typedef DWORD (WINAPI *QUERYSPOOLMODE) (HANDLE,DWORD,DWORD);
typedef DWORD (WINAPI *QUERYREMOTEFONTS) (DWORD,DWORD,DWORD);

extern CLOSEPRINTER fpClosePrinter;
extern OPENPRINTERW fpOpenPrinterW;

/* FUNCTIONS *****************************************************************/

PVOID
HEAP_alloc(DWORD len);

NTSTATUS
HEAP_strdupA2W(
    LPWSTR* ppszW,
    LPCSTR lpszA
);

VOID
HEAP_free(LPVOID memory);

VOID
FASTCALL
FONT_TextMetricWToA(
    const TEXTMETRICW *ptmW,
    LPTEXTMETRICA ptmA
);

VOID
FASTCALL
NewTextMetricW2A(
    NEWTEXTMETRICA *tma,
    NEWTEXTMETRICW *tmw
);

VOID
FASTCALL
NewTextMetricExW2A(
    NEWTEXTMETRICEXA *tma,
    NEWTEXTMETRICEXW *tmw
);

BOOL
FASTCALL
DeleteRegion( HRGN );

BOOL
WINAPI
GdiValidateHandle(HGDIOBJ);

BOOL
GdiGetHandleUserData(
    HGDIOBJ hGdiObj,
    DWORD ObjectType,
    PVOID *UserData
);

PLDC
FASTCALL
GdiGetLDC(HDC hDC);

BOOL
FASTCALL
GdiSetLDC(HDC hdc, PVOID pvLDC);

HGDIOBJ
WINAPI
GdiFixUpHandle(HGDIOBJ hGO);

BOOL
WINAPI
CalculateColorTableSize(
    CONST BITMAPINFOHEADER *BitmapInfoHeader,
    UINT *ColorSpec,
    UINT *ColorTableSize
);

LPBITMAPINFO
WINAPI
ConvertBitmapInfo(
    CONST BITMAPINFO *BitmapInfo,
    UINT ColorSpec,
    UINT *BitmapInfoSize,
    BOOL FollowedByData
);

DWORD
WINAPI
GetAndSetDCDWord(
    _In_ HDC hdc,
    _In_ UINT u,
    _In_ DWORD dwIn,
    _In_ ULONG ulMFId,
    _In_ USHORT usMF16Id,
    _In_ DWORD dwError);

DWORD
WINAPI
GetDCDWord(
    _In_ HDC hdc,
    _In_ UINT u,
    _In_ DWORD dwError);

HGDIOBJ
WINAPI
GetDCObject( HDC, INT);

VOID
NTAPI
LogFontA2W(
    LPLOGFONTW pW,
    CONST LOGFONTA *pA
);

VOID
NTAPI
LogFontW2A(
    LPLOGFONTA pA,
    CONST LOGFONTW *pW
);

VOID
WINAPI
EnumLogFontExW2A(
    LPENUMLOGFONTEXA fontA,
    CONST ENUMLOGFONTEXW *fontW );

BOOL
WINAPI
LoadLPK(
    INT LpkFunctionID
);

VOID
WINAPI
GdiInitializeLanguagePack(
    _In_ DWORD InitParam);

VOID
WINAPI
InitializeLpkHooks(
    _In_ FARPROC *hookfuncs);

BOOL
WINAPI
GetETM(HDC hdc,
       EXTTEXTMETRIC *petm);

/* FIXME: Put in some public header */
UINT
WINAPI
UserRealizePalette(HDC hDC);

int
WINAPI
GdiAddFontResourceW(LPCWSTR lpszFilename,FLONG fl,DESIGNVECTOR *pdv);

VOID
WINAPI
GdiSetLastError( DWORD dwErrCode );

DWORD WINAPI GdiGetCodePage(HDC);

int
WINAPI
GdiGetBitmapBitsSize(BITMAPINFO *lpbmi);

VOID GdiSAPCallback(PLDC pldc);
HGDIOBJ FASTCALL hGetPEBHandle(HANDLECACHETYPE,COLORREF);

int FASTCALL DocumentEventEx(PVOID,HANDLE,HDC,int,ULONG,PVOID,ULONG,PVOID);
BOOL FASTCALL EndPagePrinterEx(PVOID,HANDLE);
BOOL FASTCALL LoadTheSpoolerDrv(VOID);

FORCEINLINE
PVOID
GdiAllocBatchCommand(
    HDC hdc,
    USHORT Cmd)
{
    PTEB pTeb;
    USHORT cjSize;
    PGDIBATCHHDR pHdr;

    /* Get a pointer to the TEB */
    pTeb = NtCurrentTeb();

    /* Check if we have a valid environment */
    if (!pTeb || !pTeb->Win32ThreadInfo) return NULL;

    /* Get the size of the entry */
    if      (Cmd == GdiBCPatBlt) cjSize = sizeof(GDIBSPATBLT);
    else if (Cmd == GdiBCPolyPatBlt) cjSize = sizeof(GDIBSPPATBLT);
    else if (Cmd == GdiBCTextOut) cjSize = sizeof(GDIBSTEXTOUT);
    else if (Cmd == GdiBCExtTextOut) cjSize = sizeof(GDIBSEXTTEXTOUT);
    else if (Cmd == GdiBCSetBrushOrg) cjSize = sizeof(GDIBSSETBRHORG);
    else if (Cmd == GdiBCExtSelClipRgn) cjSize = sizeof(GDIBSEXTSELCLPRGN);
    else if (Cmd == GdiBCSelObj) cjSize = sizeof(GDIBSOBJECT);
    else if (Cmd == GdiBCDelRgn) cjSize = sizeof(GDIBSOBJECT);
    else if (Cmd == GdiBCDelObj) cjSize = sizeof(GDIBSOBJECT);
    else cjSize = 0;

    /* Unsupported operation */
    if (cjSize == 0) return NULL;

    /* Do we use a DC? */
    if (hdc)
    {
        /* If the batch DC is NULL, we set this one as the new one */
        if (!pTeb->GdiTebBatch.HDC) pTeb->GdiTebBatch.HDC = hdc;

        /* If not, check if the batch DC equal to our DC */
        else if (pTeb->GdiTebBatch.HDC != hdc) return NULL;
    }

    /* Check if the buffer is full */
    if ((pTeb->GdiBatchCount >= GDI_BatchLimit) ||
        ((pTeb->GdiTebBatch.Offset + cjSize) > GDIBATCHBUFSIZE))
    {
        /* Call win32k, the kernel will call NtGdiFlushUserBatch to flush
           the current batch */
        NtGdiFlush();

        // If Flushed, lose the hDC for this batch job! See CORE-15839.
        if (hdc)
        {
            if (!pTeb->GdiTebBatch.HDC) pTeb->GdiTebBatch.HDC = hdc;
        }
    }

    /* Get the head of the entry */
    pHdr = (PVOID)((PUCHAR)pTeb->GdiTebBatch.Buffer + pTeb->GdiTebBatch.Offset);

    /* Update Offset and batch count */
    pTeb->GdiTebBatch.Offset += cjSize;
    pTeb->GdiBatchCount++;

    /* Fill in the core fields */
    pHdr->Cmd = Cmd;
    pHdr->Size = cjSize;

    return pHdr;
}

FORCEINLINE
PDC_ATTR
GdiGetDcAttr(HDC hdc)
{
    GDILOOBJTYPE eDcObjType;
    PDC_ATTR pdcattr;

    /* Check DC object type */
    eDcObjType = GDI_HANDLE_GET_TYPE(hdc);
    if ((eDcObjType != GDILoObjType_LO_DC_TYPE) &&
        (eDcObjType != GDILoObjType_LO_ALTDC_TYPE))
    {
        return NULL;
    }

    /* Get the DC attribute */
    if (!GdiGetHandleUserData((HGDIOBJ)hdc, eDcObjType, (PVOID*)&pdcattr))
    {
        return NULL;
    }

    return pdcattr;
}

FORCEINLINE
PRGN_ATTR
GdiGetRgnAttr(HRGN hrgn)
{
    PRGN_ATTR prgnattr;

    /* Get the region attribute */
    if (!GdiGetHandleUserData(hrgn, GDILoObjType_LO_REGION_TYPE, (PVOID*)&prgnattr))
    {
        return NULL;
    }

    return prgnattr;
}

#ifdef _M_IX86
FLOATL FASTCALL EFtoF(EFLOAT_S * efp);
#define FOtoF(pfo) EFtoF((EFLOAT_S*)pfo)
#else
#define FOtoF(pfo) (*(pfo))
#endif

/* This is an inlined version of lrintf. */
FORCEINLINE
int
_lrintf(float f)
{
#if defined(_M_IX86) && defined(__GNUC__)
    int result;
    __asm__ __volatile__ ("fistpl %0" : "=m" (result) : "t" (f) : "st");
    return result;
#elif defined(_M_IX86) && defined(_MSC_VER)
    int result;
    __asm
    {
        fld f;
        fistp result;
    }
#else
    /* slow, but portable */
    return (int)(f >= 0 ? f+0.5 : f-0.5);
#endif
}

HBRUSH
WINAPI
GdiSelectBrush(
    _In_ HDC hdc,
    _In_ HBRUSH hbr);

HPEN
WINAPI
GdiSelectPen(
    _In_ HDC hdc,
    _In_ HPEN hpen);

HFONT
WINAPI
GdiSelectFont(
    _In_ HDC hdc,
    _In_ HFONT hfont);

HGDIOBJ
WINAPI
GdiCreateClientObj(
    _In_ PVOID pvObject,
    _In_ GDILOOBJTYPE eObjType);

PVOID
WINAPI
GdiDeleteClientObj(
    _In_ HGDIOBJ hobj);

BOOL
WINAPI
GdiCreateClientObjLink(
    _In_ HGDIOBJ hobj,
    _In_ PVOID pvObject);

PVOID
WINAPI
GdiGetClientObjLink(
    _In_ HGDIOBJ hobj);

PVOID
WINAPI
GdiRemoveClientObjLink(
    _In_ HGDIOBJ hobj);

extern ULONG gcClientObj;

VOID
WINAPI
METADC_RosGlueDeleteObject(HGDIOBJ hobj);

BOOL
WINAPI
METADC_RosGlueDeleteDC(
    _In_ HDC hdc);

BOOL METADC_DeleteDC( HDC hdc );

BOOL
WINAPI
METADC_ExtTextOutW(
    HDC hdc,
    INT x,
    INT y,
    UINT fuOptions,
    const RECT *lprc,
    LPCWSTR lpString,
    UINT cchString,
    const INT *lpDx);


/* The following METADC_* functions follow this pattern: */
#define HANDLE_METADC(_RetType, _Func, dwError, hdc, ...) \
    if (GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_DC_TYPE) \
    { \
        if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE) \
        { \
           return (_RetType)METADC_##_Func(hdc, __VA_ARGS__); \
        } \
        else \
        { \
           PLDC pLDC = GdiGetLDC(hdc); \
           _RetType _Ret = dwError; \
           if ( !pLDC ) \
           { \
              SetLastError(ERROR_INVALID_HANDLE); \
              return (_RetType)_Ret; \
           } \
           if ( pLDC->iType == LDC_EMFLDC && !(EMFDC_##_Func(pLDC, __VA_ARGS__)) ) \
           { \
              return (_RetType)_Ret; \
           } \
           /*  Fall through to support information DC's.*/ \
        } \
    }

#define HANDLE_METADC16(_RetType, _Func, dwError, hdc, ...) \
    if (GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_DC_TYPE) \
    { \
        if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE) \
        { \
           return METADC_##_Func(hdc, __VA_ARGS__); \
        } \
    }

#define HANDLE_METADC0P(_RetType, _Func, dwError, hdc, ...) \
    if (GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_DC_TYPE) \
    { \
       PLDC pLDC = NULL; \
       _RetType _Ret = dwError; \
       if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE) \
       { \
          return (_RetType)_Ret; \
       } \
       pLDC = GdiGetLDC(hdc); \
       if ( !pLDC ) \
       { \
          SetLastError(ERROR_INVALID_HANDLE); \
          return (_RetType)_Ret; \
       } \
       if ( pLDC->iType == LDC_EMFLDC && !(EMFDC_##_Func(pLDC)) ) \
       { \
          return (_RetType)_Ret; \
       } \
       /*  Fall through to support information DC's.*/ \
    }

#define HANDLE_EMETAFDC(_RetType, _Func, dwError, hdc, ...) \
    if (GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_DC_TYPE) \
    { \
       PLDC pLDC = NULL; \
       _RetType _Ret = dwError; \
       if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE) \
       { \
          return (_RetType)_Ret; \
       } \
       pLDC = GdiGetLDC(hdc); \
       if ( !pLDC ) \
       { \
          SetLastError(ERROR_INVALID_HANDLE); \
          return (_RetType)_Ret; \
       } \
       if ( pLDC->iType == LDC_EMFLDC && !(EMFDC_##_Func(pLDC, __VA_ARGS__)) ) \
       { \
          return (_RetType)_Ret; \
       } \
       /*  Fall through to support information DC's.*/ \
    }

#define HANDLE_METADC1P(_RetType, _Func, dwError, hdc, ...) \
    if (GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_DC_TYPE) \
    { \
        if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE) \
        { \
           return (_RetType)METADC_##_Func(hdc); \
        } \
        else \
        { \
           PLDC pLDC = GdiGetLDC(hdc); \
           _RetType _Ret = dwError; \
           if ( !pLDC ) \
           { \
              SetLastError(ERROR_INVALID_HANDLE); \
              return (_RetType)_Ret; \
           } \
           if ( pLDC->iType == LDC_EMFLDC && !(EMFDC_##_Func(pLDC)) ) \
           { \
              return (_RetType)_Ret; \
           } \
           /*  Fall through to support information DC's.*/ \
        } \
    }


BOOL WINAPI METADC_SetD(_In_ HDC hdc,_In_ DWORD dwIn,_In_ USHORT usMF16Id);
BOOL WINAPI EMFDC_SetD(_In_ PLDC pldc,_In_ DWORD dwIn,_In_ ULONG ulMFId);

HDC WINAPI GdiConvertAndCheckDC(HDC hdc);

HENHMETAFILE WINAPI SetEnhMetaFileBitsAlt( PDWORD pdw, LPWSTR FilePart, HANDLE hFile, LARGE_INTEGER li);

/* meta dc files */
extern BOOL METADC_Arc( HDC hdc, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL METADC_BitBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width, INT height,
                           HDC hdc_src, INT x_src, INT y_src, DWORD rop );
extern BOOL METADC_Chord( HDC hdc, INT left, INT top, INT right, INT bottom, INT xstart,
                          INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL METADC_Ellipse( HDC hdc, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL METADC_ExcludeClipRect( HDC hdc, INT left, INT top, INT right,
                                    INT bottom ) DECLSPEC_HIDDEN;
extern BOOL METADC_ExtEscape( HDC hdc, INT escape, INT input_size, LPCSTR input, INT output_size, LPVOID output ) DECLSPEC_HIDDEN;
extern BOOL METADC_ExtFloodFill( HDC hdc, INT x, INT y, COLORREF color,
                                 UINT fill_type ) DECLSPEC_HIDDEN;
extern BOOL METADC_ExtSelectClipRgn( HDC hdc, HRGN hrgn, INT mode ) DECLSPEC_HIDDEN;
extern BOOL METADC_ExtTextOut( HDC hdc, INT x, INT y, UINT flags, const RECT *rect,
                               const WCHAR *str, UINT count, const INT *dx ) DECLSPEC_HIDDEN;
extern BOOL METADC_FillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern BOOL METADC_FrameRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT  METADC_GetDeviceCaps( HDC hdc, INT cap );
extern BOOL METADC_IntersectClipRect( HDC hdc, INT left, INT top, INT right,
                                      INT bottom ) DECLSPEC_HIDDEN;
extern BOOL METADC_InvertRgn( HDC hdc, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL METADC_LineTo( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_MoveTo( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_OffsetClipRgn( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_OffsetViewportOrgEx( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_OffsetWindowOrgEx( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_PaintRgn( HDC hdc, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL METADC_PatBlt( HDC hdc, INT left, INT top, INT width, INT height, DWORD rop );
extern BOOL METADC_Pie( HDC hdc, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL METADC_PolyPolygon( HDC hdc, const POINT *points, const INT *counts,
                                UINT polygons ) DECLSPEC_HIDDEN;
extern BOOL METADC_Polygon( HDC hdc, const POINT *points, INT count ) DECLSPEC_HIDDEN;
extern BOOL METADC_Polyline( HDC hdc, const POINT *points,INT count) DECLSPEC_HIDDEN;
extern BOOL METADC_RealizePalette( HDC hdc ) DECLSPEC_HIDDEN;
extern BOOL METADC_Rectangle( HDC hdc, INT left, INT top, INT right, INT bottom) DECLSPEC_HIDDEN;
extern BOOL METADC_RestoreDC( HDC hdc, INT level ) DECLSPEC_HIDDEN;
extern BOOL METADC_RoundRect( HDC hdc, INT left, INT top, INT right, INT bottom,
                              INT ell_width, INT ell_height ) DECLSPEC_HIDDEN;
extern BOOL METADC_SaveDC( HDC hdc ) DECLSPEC_HIDDEN;
extern BOOL METADC_ScaleViewportExtEx( HDC hdc, INT x_num, INT x_denom, INT y_num,
                                       INT y_denom ) DECLSPEC_HIDDEN;
extern BOOL METADC_ScaleWindowExtEx( HDC hdc, INT x_num, INT x_denom, INT y_num,
                                     INT y_denom ) DECLSPEC_HIDDEN;
extern HGDIOBJ METADC_SelectObject( HDC hdc, HGDIOBJ obj ) DECLSPEC_HIDDEN;
extern BOOL METADC_SelectPalette( HDC hdc, HPALETTE palette ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetBkColor( HDC hdc, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetBkMode( HDC hdc, INT mode ) DECLSPEC_HIDDEN;
extern INT  METADC_SetDIBitsToDevice( HDC hdc, INT x_dest, INT y_dest, DWORD width, DWORD height,
                                      INT x_src, INT y_src, UINT startscan, UINT lines,
                                      const void *bits, const BITMAPINFO *info,
                                      UINT coloruse ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetLayout( HDC hdc, DWORD layout ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetTextCharacterExtra( HDC hdc, INT extra ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetMapMode( HDC hdc, INT mode ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetMapperFlags( HDC hdc, DWORD flags ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetPixel( HDC hdc, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetPolyFillMode( HDC hdc, INT mode ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetRelAbs( HDC hdc, INT mode ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetROP2( HDC hdc, INT rop ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetStretchBltMode( HDC hdc, INT mode ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetTextAlign( HDC hdc, UINT align ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetTextColor( HDC hdc, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetTextJustification( HDC hdc, INT extra, INT breaks ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetViewportExtEx( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetViewportOrgEx( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetWindowExtEx( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_SetWindowOrgEx( HDC, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_StretchBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                               HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                               DWORD rop );
extern BOOL METADC_StretchDIBits( HDC hdc, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                                  INT x_src, INT y_src, INT width_src, INT height_src,
                                  const void *bits, const BITMAPINFO *info, UINT coloruse,
                                  DWORD rop ) DECLSPEC_HIDDEN;
/* enhanced metafiles */
extern BOOL EMFDC_AbortPath( LDC *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_AlphaBlend( LDC *dc_attr, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                              HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                              BLENDFUNCTION blend_function );
extern BOOL EMFDC_AngleArc( LDC *dc_attr, INT x, INT y, DWORD radius, FLOAT start,
                            FLOAT sweep ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ArcChordPie( LDC *dc_attr, INT left, INT top, INT right,
                               INT bottom, INT xstart, INT ystart, INT xend,
                               INT yend, DWORD type ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_BeginPath( LDC *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_BitBlt( LDC *dc_attr, INT x_dst, INT y_dst, INT width, INT height,
                          HDC hdc_src, INT x_src, INT y_src, DWORD rop );
extern BOOL EMFDC_CloseFigure( LDC *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_DeleteDC( LDC *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_Ellipse( LDC *dc_attr, INT left, INT top, INT right,
                           INT bottom ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_EndPath( LDC *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ExcludeClipRect( LDC *dc_attr, INT left, INT top, INT right,
                                   INT bottom ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ExtFloodFill( LDC *dc_attr, INT x, INT y, COLORREF color,
                                UINT fill_type ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ExtSelectClipRgn( LDC *dc_attr, HRGN hrgn, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ExtTextOut( LDC *dc_attr, INT x, INT y, UINT flags, const RECT *rect,
                              const WCHAR *str, UINT count, const INT *dx ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_FillPath( LDC *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_FillRgn( LDC *dc_attr, HRGN hrgn, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_FlattenPath( LDC *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_FrameRgn( LDC *dc_attr, HRGN hrgn, HBRUSH hbrush, INT width,
                            INT height ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_GradientFill( LDC *dc_attr, TRIVERTEX *vert_array, ULONG nvert,
                                void *grad_array, ULONG ngrad, ULONG mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_IntersectClipRect( LDC *dc_attr, INT left, INT top, INT right,
                                     INT bottom ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_InvertRgn( LDC *dc_attr, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_LineTo( LDC *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ModifyWorldTransform( LDC *dc_attr, const XFORM *xform,
                                        DWORD mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_MoveTo( LDC *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_OffsetClipRgn( LDC *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PaintRgn( LDC *dc_attr, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PatBlt( LDC *dc_attr, INT left, INT top, INT width, INT height, DWORD rop );
extern BOOL EMFDC_PolyBezier( LDC *dc_attr, const POINT *points, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PolyBezierTo( LDC *dc_attr, const POINT *points, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PolyDraw( LDC *dc_attr, const POINT *points, const BYTE *types,
                            DWORD count ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PolyPolyline( LDC *dc_attr, const POINT *points, const DWORD *counts,
                                DWORD polys ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PolyPolygon( LDC *dc_attr, const POINT *points, const INT *counts,
                               UINT polys ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_Polygon( LDC *dc_attr, const POINT *points, INT count ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_Polyline( LDC *dc_attr, const POINT *points, INT count) DECLSPEC_HIDDEN;
extern BOOL EMFDC_PolylineTo( LDC *dc_attr, const POINT *points, INT count ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_Rectangle( LDC *dc_attr, INT left, INT top, INT right,
                             INT bottom) DECLSPEC_HIDDEN;
extern BOOL EMFDC_RestoreDC( LDC *dc_attr, INT level ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_RoundRect( LDC *dc_attr, INT left, INT top, INT right, INT bottom,
                             INT ell_width, INT ell_height ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SaveDC( LDC *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ScaleViewportExtEx( LDC *dc_attr, INT x_num, INT x_denom, INT y_num,
                                      INT y_denom ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_ScaleWindowExtEx( LDC *dc_attr, INT x_num, INT x_denom, INT y_num,
                                    INT y_denom ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SelectClipPath( LDC *dc_attr, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SelectObject( LDC *dc_attr, HGDIOBJ obj ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SelectPalette( LDC *dc_attr, HPALETTE palette ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetArcDirection( LDC *dc_attr, INT dir ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetBkColor( LDC *dc_attr, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetBkMode( LDC *dc_attr, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetDCBrushColor( LDC *dc_attr, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetDCPenColor( LDC *dc_attr, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetDIBitsToDevice( LDC *dc_attr, INT x_dest, INT y_dest, DWORD width,
                                     DWORD height, INT x_src, INT y_src, UINT startscan,
                                     UINT lines, const void *bits, const BITMAPINFO *info,
                                     UINT coloruse ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetLayout( LDC *dc_attr, DWORD layout ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetMapMode( LDC *dc_attr, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetMapperFlags( LDC *dc_attr, DWORD flags ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetPixel( LDC *dc_attr, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetPolyFillMode( LDC *dc_attr, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetROP2( LDC *dc_attr, INT rop ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetStretchBltMode( LDC *dc_attr, INT mode ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetTextAlign( LDC *dc_attr, UINT align ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetTextColor( LDC *dc_attr, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetTextJustification( LDC *dc_attr, INT extra, INT breaks ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetViewportExtEx( LDC *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetViewportOrgEx( LDC *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetWindowExtEx( LDC *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetWindowOrgEx( LDC *dc_attr, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_SetWorldTransform( LDC *dc_attr, const XFORM *xform ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_StretchBlt( LDC *dc_attr, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                              HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                              DWORD rop );
extern BOOL EMFDC_StretchDIBits( LDC *dc_attr, INT x_dst, INT y_dst, INT width_dst,
                                 INT height_dst, INT x_src, INT y_src, INT width_src,
                                 INT height_src, const void *bits, const BITMAPINFO *info,
                                 UINT coloruse, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_StrokeAndFillPath( LDC *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_StrokePath( LDC *dc_attr ) DECLSPEC_HIDDEN;
extern BOOL EMFDC_WidenPath( LDC *dc_attr ) DECLSPEC_HIDDEN;


BOOL EMFDC_MaskBlt( LDC *dc_attr, INT xDest, INT yDest, INT cx, INT cy, HDC hdcSrc, INT xSrc, INT ySrc, HBITMAP hbmMask, INT xMask, INT yMask, DWORD dwRop);
BOOL EMFDC_PlgBlt( LDC *dc_attr, const POINT * ppt, HDC hdcSrc, INT xSrc, INT ySrc, INT cx, INT cy, HBITMAP hbmMask, INT xMask, INT yMask);
BOOL EMFDC_TransparentBlt( LDC *dc_attr, INT xDst, INT yDst, INT cxDst, INT cyDst, HDC hdcSrc, INT xSrc, INT ySrc, INT cxSrc, INT cySrc, UINT crTransparent);
BOOL EMFDC_SetBrushOrg( LDC *dc_attr, INT x, INT y);
BOOL EMFDC_SetMetaRgn( LDC *dc_attr );
BOOL EMFDC_WriteNamedEscape( LDC *dc_attr, PWCHAR pDriver, INT nEscape, INT cbInput, LPCSTR lpszInData);
BOOL EMFDC_WriteEscape( LDC *dc_attr, INT nEscape, INT cbInput, LPSTR lpszInData, DWORD emrType);


FORCEINLINE BOOL EMFDC_Arc( PLDC dc_attr, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend )
{
    return EMFDC_ArcChordPie( dc_attr, left, top, right, bottom, xstart, ystart, xend, yend, EMR_ARC );
}

FORCEINLINE BOOL EMFDC_ArcTo( PLDC dc_attr, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend )
{
return EMFDC_ArcChordPie( dc_attr, left, top, right, bottom, xstart, ystart, xend, yend, EMR_ARCTO );
}
FORCEINLINE BOOL EMFDC_Chord( PLDC dc_attr, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend )
{
return EMFDC_ArcChordPie( dc_attr, left, top, right, bottom, xstart, ystart, xend, yend, EMR_CHORD );
}

FORCEINLINE BOOL EMFDC_Pie( PLDC dc_attr, INT left, INT top, INT right, INT bottom, INT xstart, INT ystart, INT xend, INT yend )
{
return EMFDC_ArcChordPie( dc_attr, left, top, right, bottom, xstart, ystart, xend, yend, EMR_PIE );
}

BOOL WINAPI EMFDC_GdiComment( HDC hdc, UINT bytes, const BYTE *buffer );

/* EOF */
