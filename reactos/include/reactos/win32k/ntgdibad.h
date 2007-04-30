#ifndef WIN32K_NTGDI_BAD_INCLUDED
#define WIN32K_NTGDI_BAD_INCLUDED

/*
 *
 * If you want to help, please read this:
 *
 * This file contains NtGdi APIs which are specific to ROS, including
 * a short comment describing the solution on how to use the actual NtGdi
 * call documented in ntgdi.h. Here are the main cases and information on
 * how to remove them from this header.
 *
 * - Simple rename. This deals with an API simply having a different name,
 *                  with absolutely no other changes needed.
 * - Rename and new parameters. This deals with a case similar to the one
 *                              above, except that new parameters have now
 *                              been added. This is also usually extremly
 *                              simple to fix. Either pass the right params
 *                              or pass null/0 values that you ignore.
 * - Rename and new structure. This is similar to the above, except that the
 *                             information is now passed in a differently
 *                             named and organized structure. Ask Alex for
 *                             the structure you need and he will add it to
 *                             ntgdityp.h
 * - Rename and different semantics. Similar to the previous examples, except
 *                                   that parameters have usually been removed
 *                                   or need to be converted in user-mode in
 *                                   one form of another.
 * - Does not exist: user-mode. This means that the API can be fully done in
 *                              user mode. In 80% of cases, our API was already
 *                              returning NOT_IMPLEMENTED in kernel-mode, so
 *                              the work to be done is minimal. A good example
 *                              are the ICM and Metafile APIs, which can simply
 *                              be removed and added into gdi32.
 * - Does not exist: GDI Shared Objects. This is by far the hardest case. This
 *                                       class cannot be fixed until ReactOS
 *                                       has a working Shared GDI Object table
 *                                       and a DC_ATTR structure in which the
 *                                       attributes, selection and deletion of
 *                                       objects can be quickly done from user-
 *                                       mode without requiring a kernel mode
 *                                       call.
 */
/* Should be using ENUMFONTDATAW */
typedef struct tagFONTFAMILYINFO
{
  ENUMLOGFONTEXW EnumLogFontEx;
  NEWTEXTMETRICEXW NewTextMetricEx;
  DWORD FontType;
} FONTFAMILYINFO, *PFONTFAMILYINFO;

/* Should be using NtGdiEnumFontChunk */
INT
NTAPI
NtGdiGetFontFamilyInfo(
    HDC Dc,
    LPLOGFONTW LogFont,
    PFONTFAMILYINFO Info,
    DWORD Size
);

/* Should be using NtGdiEnumFontChunk */
BOOL
NTAPI
NtGdiTranslateCharsetInfo(
    PDWORD Src,
    LPCHARSETINFO CSI,
    DWORD Flags
);

/* The gdi32 call does all the work in user-mode, save for NtGdiMakeFontDir */
BOOL
NTAPI
NtGdiCreateScalableFontResource(
    DWORD Hidden,
    LPCWSTR FontRes,
    LPCWSTR FontFile,
    LPCWSTR CurrentPath
);

/* The gdi32 call Should Use NtGdiGetRandomRgn and nothing else */
HRGN
NTAPI
NtGdiGetClipRgn(HDC hDC);

/* The gdi32 call Should Use NtGdiGetTextExtent */
BOOL
NTAPI
NtGdiGetTextExtentPoint32(
    HDC hDC,
    LPCWSTR String,
    int Count,
    LPSIZE   
);

BOOL
STDCALL
NtGdiGetCharWidth32(
    HDC hDC,
    UINT FirstChar,
    UINT LastChar,
    LPINT Buffer
);

/* Use NtGdiAddFontResourceW */
int
STDCALL
NtGdiAddFontResource(PUNICODE_STRING Filename,
					 DWORD fl);

/* Use NtGdiDoPalette with GdiPalAnimate */
BOOL
STDCALL
NtGdiAnimatePalette (
	HPALETTE		hpal,
	UINT			StartIndex,
	UINT			Entries,
	CONST PPALETTEENTRY	ppe
	);

/* Use NtGdiArcInternal with GdiTypeArc */
BOOL
STDCALL
NtGdiArc(HDC  hDC,
              int  LeftRect,
              int  TopRect,
              int  RightRect, 
              int  BottomRect,
              int  XStartArc,
              int  YStartArc,
              int  XEndArc,  
              int  YEndArc);

/* Use NtGdiArcInternal with GdiTypeArcTo */
BOOL
STDCALL
NtGdiArcTo(HDC  hDC,
                int  LeftRect,
                int  TopRect,
                int  RightRect,
                int  BottomRect,
                int  XRadial1,
                int  YRadial1,
                int  XRadial2,
                int  YRadial2);

/* Use NtGdiArcInternal with GdiTypeChord */
BOOL
STDCALL
NtGdiChord(HDC  hDC,
                int  LeftRect,
                int  TopRect,
                int  RightRect,
                int  BottomRect,
                int  XRadial1,
                int  YRadial1,
                int  XRadial2,
                int  YRadial2);

/* Metafiles are user mode */
HENHMETAFILE
STDCALL
NtGdiCloseEnhMetaFile (
	HDC	hDC
	);

/* Metafiles are user mode */
HMETAFILE
STDCALL
NtGdiCloseMetaFile (
	HDC	hDC
	);

/* Does not exist */
BOOL
STDCALL
NtGdiColorMatchToTarget(HDC  hDC,
                             HDC  hDCTarget, 
                             DWORD  Action);

/* Metafiles are user mode */
HENHMETAFILE
STDCALL
NtGdiCopyEnhMetaFile (
	HENHMETAFILE	Src,
	LPCWSTR		File
	);

/* Metafiles are user mode */
HMETAFILE
STDCALL
NtGdiCopyMetaFile (
	HMETAFILE	Src,
	LPCWSTR		File
	);

/* Use NtGdiCreateBitmap and expand the pbm-> */
HBITMAP
STDCALL
NtGdiCreateBitmapIndirect (
	CONST BITMAP	* BM
	);

/* Use NtGdiCreateDIBitmapInternal */
HBITMAP
STDCALL
NtGdiCreateDIBitmap (
	HDC			hDC,
	CONST BITMAPINFOHEADER	* bmih,
	DWORD			Init,
	CONST VOID		* bInit,
	CONST BITMAPINFO	* bmi,
	UINT			Usage
	);

/* Use NtGdiCreateCompatibleBitmap */
HBITMAP
STDCALL
NtGdiCreateDiscardableBitmap (
	HDC	hDC,
	INT	Width,
	INT	Height
	);

/* Use NtGdiCreateEllipticRgn and expand the lprect-> */
HRGN
STDCALL
NtGdiCreateEllipticRgnIndirect(CONST PRECT  rc);

/* Metafiles are user mode */
HDC
STDCALL
NtGdiCreateEnhMetaFile (
	HDC		hDCRef,
	LPCWSTR		File,
	CONST LPRECT	Rect,
	LPCWSTR		Description
	);

/* Metafiles are user mode */
HDC
STDCALL
NtGdiCreateMetaFile (
	LPCWSTR		File
	);

/* Use NtGdiCreatePaletteInternal with palNumEntries at the end. */
HPALETTE
STDCALL
NtGdiCreatePalette (
	CONST PLOGPALETTE	lgpl
	);

/* Use NtGdiCreatePen with -> as parameters. */
HPEN STDCALL
NtGdiCreatePenIndirect(
   CONST PLOGPEN LogBrush);

/* Use NtGdiPolyPolyDraw with PolyPolyRgn. */
HRGN
STDCALL
NtGdiCreatePolygonRgn(CONST PPOINT  pt,
                           INT  Count,
                           INT  PolyFillMode);

/* Use NtGdiPolyPolyDraw with PolyPolyRgn. */
HRGN
STDCALL
NtGdiCreatePolyPolygonRgn(CONST PPOINT  pt,
                               CONST PINT  PolyCounts,
                               INT  Count,
                               INT  PolyFillMode);

/* Use NtGdiCreateRectRgn with expanded paraemters. */
HRGN
STDCALL
NtGdiCreateRectRgnIndirect(CONST PRECT  rc);

/* Use NtGdiTransformPoints with GdiDpToLp. */
BOOL
STDCALL
NtGdiDPtoLP (
	HDC	hDC,
	LPPOINT	Points,
	int	Count
	);

/* Meta are user-mode. */
BOOL
STDCALL
NtGdiDeleteEnhMetaFile (
	HENHMETAFILE	emf
	);

/* Meta are user-mode. */
BOOL
STDCALL
NtGdiDeleteMetaFile (
	HMETAFILE	mf
	);

/* Should be done in user-mode. */
BOOL STDCALL  NtGdiDeleteObject(HGDIOBJ hObject);

/* Meta are user-mode. */
BOOL
STDCALL
NtGdiEnumEnhMetaFile (
	HDC		hDC,
	HENHMETAFILE	emf,
	ENHMFENUMPROC	EnhMetaFunc,
	LPVOID		Data,
	CONST LPRECT	Rect
	);

/* Should be done in user-mode. */
int
STDCALL
NtGdiEnumFonts(HDC  hDC,
                   LPCWSTR FaceName,
                   FONTENUMPROCW  FontFunc,
                   LPARAM  lParam);

/* Should be done in user-mode. */
INT
STDCALL
NtGdiEnumICMProfiles(HDC    hDC,
                    LPWSTR lpstrBuffer,
                    UINT   cch );

/* Meta are user-mode. */
BOOL
STDCALL
NtGdiEnumMetaFile (
	HDC		hDC,
	HMETAFILE	mf,
	MFENUMPROC	MetaFunc,
	LPARAM		lParam
	);

/* Should be done in user-mode. */
INT
STDCALL
NtGdiEscape(HDC  hDC,
                INT  Escape,
                INT  InSize,
                LPCSTR  InData,
                LPVOID  OutData);

/* Use NtGdiExtTextOutW with 0, 0 at the end. */
BOOL
STDCALL
NtGdiExtTextOut(HDC  hdc,
                     int  X,
                     int  Y,
                     UINT  fuOptions,
                     CONST RECT  *lprc,
                     LPCWSTR  lpString,
                     UINT  cbCount,
                     CONST INT  *lpDx);

/* Use NtGdiExtFloodFill with FLOODFILLBORDER. */
BOOL
STDCALL
NtGdiFloodFill (
	HDC		hDC,
	INT		XStart,
	INT		YStart,
	COLORREF	Fill
	);

/* Should be done in user-mode. */
BOOL
STDCALL
NtGdiGdiComment (
	HDC		hDC,
	UINT		Size,
	CONST LPBYTE	Data
	);

/* Should be done in user-mode. */
DWORD STDCALL NtGdiGdiGetBatchLimit (VOID);

/* Should be done in user-mode. */
DWORD STDCALL NtGdiGdiSetBatchLimit (DWORD  Limit);

/* Use NtGdiGetDCDword with GdiGetArcDirection. */
INT
STDCALL
NtGdiGetArcDirection ( HDC hDC );

/* Should be done in user-mode. */
BOOL
STDCALL
NtGdiGetAspectRatioFilterEx(HDC  hDC,
                                 LPSIZE  AspectRatio);

/* Should be done in user-mode using shared GDI Objects. */
BOOL
STDCALL
NtGdiGetBitmapDimensionEx (
	HBITMAP	hBitmap,
	LPSIZE	Dimension
	);

/* Should be done in user-mode using shared GDI Objects. */
COLORREF STDCALL  NtGdiGetBkColor(HDC  hDC);

/* Should be done in user-mode using shared GDI Objects. */
INT STDCALL  NtGdiGetBkMode(HDC  hDC);

/* Should be done in user-mode using shared GDI Objects. */
BOOL STDCALL  NtGdiGetBrushOrgEx(HDC  hDC, LPPOINT brushOrg);

/* Use NtGdiGetCharABCWidthsW */
BOOL
STDCALL
NtGdiGetCharABCWidths(HDC  hDC,
                           UINT  FirstChar,
                           UINT  LastChar,
                           LPABC  abc);

/* Should be done in user mode. */
BOOL
STDCALL
NtGdiGetCharABCWidthsFloat(HDC  hDC,
                                UINT  FirstChar,
                                UINT  LastChar,
                                LPABCFLOAT  abcF);

/* Should be done in user mode. */
DWORD
STDCALL
NtGdiGetCharacterPlacement(HDC  hDC,
                                 LPCWSTR  String,
                                 int  Count,
                                 int  MaxExtent,
                                 LPGCP_RESULTSW Results,
                                 DWORD  Flags);

/* Should be done in user mode. */
BOOL
STDCALL
NtGdiGetCharWidthFloat(HDC  hDC,
                            UINT  FirstChar,
                            UINT  LastChar,
                            PFLOAT  Buffer);

/* Use NtGdiGetAppClipBox. */
int
STDCALL
NtGdiGetClipBox (
	HDC	hDC,
	LPRECT	rc
	);

/* Use NtGdiGetColorSpaceforBitmap. */
HCOLORSPACE
STDCALL
NtGdiGetColorSpace(HDC  hDC);

/* Should be done in user-mode and/or NtGdiGetDCObject. */
HGDIOBJ STDCALL  NtGdiGetCurrentObject(HDC  hDC, UINT  ObjectType);

/* Should be done in user mode. */
BOOL STDCALL  NtGdiGetCurrentPositionEx(HDC  hDC, LPPOINT currentPosition);

/* Use NtGdiGetDCPoint with GdiGetDCOrg. */
BOOL STDCALL  NtGdiGetDCOrgEx(HDC  hDC, LPPOINT  Point);

/* Use NtGdiDoPalette with GdiPalGetColorTable. */
UINT
STDCALL
NtGdiGetDIBColorTable (
	HDC	hDC,
	UINT	StartIndex,
	UINT	Entries,
	RGBQUAD	* Colors
	);

/* Use NtGdiGetDIBitsInternal. */
INT
STDCALL
NtGdiGetDIBits (
	HDC		hDC,
	HBITMAP		hBitmap,
	UINT		StartScan,
	UINT		ScanLines,
	LPVOID		Bits,
	LPBITMAPINFO	bi,
	UINT		Usage
	);


/* Meta are user-mode. */
HENHMETAFILE
STDCALL
NtGdiGetEnhMetaFile (
	LPCWSTR	MetaFile
	);

/* Meta are user-mode. */
UINT
STDCALL
NtGdiGetEnhMetaFileBits (
	HENHMETAFILE	hemf,
	UINT		BufSize,
	LPBYTE		Buffer
	);

/* Meta are user-mode. */
UINT
STDCALL
NtGdiGetEnhMetaFileDescription (
	HENHMETAFILE	hemf,
	UINT		BufSize,
	LPWSTR		Description
	);

/* Meta are user-mode. */
UINT
STDCALL
NtGdiGetEnhMetaFileHeader (
	HENHMETAFILE	hemf,
	UINT		BufSize,
	LPENHMETAHEADER	emh
	);

/* Meta are user-mode. */
UINT
STDCALL
NtGdiGetEnhMetaFilePaletteEntries (
	HENHMETAFILE	hemf,
	UINT		Entries,
	LPPALETTEENTRY	pe
	);

/* Meta are user-mode. */
UINT
STDCALL
NtGdiGetEnhMetaFilePixelFormat(HENHMETAFILE  hEMF,
                                    DWORD  BufSize, 
                                    CONST PPIXELFORMATDESCRIPTOR  pfd);

/* Should be done in user-mode. */
DWORD
STDCALL
NtGdiGetFontLanguageInfo(HDC  hDC);

/* Should be done in user-mode. */
int
STDCALL
NtGdiGetGraphicsMode ( HDC hDC );

/* Should be done in user-mode. */
BOOL
STDCALL
NtGdiGetICMProfile(HDC  hDC,  
                        LPDWORD  NameSize,
                        LPWSTR  Filename);

/* Should be done in user-mode. */
BOOL
STDCALL
NtGdiGetLogColorSpace(HCOLORSPACE  hColorSpace,
                           LPLOGCOLORSPACEW  Buffer,
                           DWORD  Size);

/* Should be done in user-mode using shared GDI Objects. */
INT STDCALL  NtGdiGetMapMode(HDC  hDC);

/* Meta files are user-mode. */
HMETAFILE
STDCALL
NtGdiGetMetaFile (
	LPCWSTR	MetaFile
	);

/* Meta files are user-mode. */
UINT
STDCALL
NtGdiGetMetaFileBitsEx (
	HMETAFILE	hmf,
	UINT		Size,
	LPVOID		Data
	);

/* Meta files are user-mode. */
int
STDCALL
NtGdiGetMetaRgn (
	HDC	hDC,
	HRGN	hrgn
	);

/* Use NtGdiDoPalette with GdiPalGetEntries. */
UINT
STDCALL
NtGdiGetPaletteEntries (
	HPALETTE	hpal,
	UINT		StartIndex,
	UINT		Entries,
	LPPALETTEENTRY	pe
	);

/* Should be done in user-mode using shared GDI Objects. */
INT
STDCALL
NtGdiGetPixelFormat(HDC  hDC);

/* Should be done in user-mode using shared GDI Objects. */
INT STDCALL  NtGdiGetPolyFillMode(HDC  hDC);

/* Should be done in user-mode using shared GDI Objects. */
INT STDCALL  NtGdiGetROP2(HDC  hDC);

/* Should be done in user-mode using shared GDI Objects. */
INT STDCALL  NtGdiGetRelAbs(HDC  hDC);

/* Should be done in user-mode using shared GDI Objects. */
INT
STDCALL
NtGdiGetSetTextCharExtra( HDC hDC, INT CharExtra, BOOL Set);

/* Should be done in user-mode using shared GDI Objects. */
INT STDCALL  NtGdiGetStretchBltMode(HDC  hDC);

/* Use NtGdiDoPalette with GdiPalSetSystemEntries. */
UINT
STDCALL
NtGdiGetSystemPaletteEntries (
	HDC		hDC,
	UINT		StartIndex,
	UINT		Entries,
	LPPALETTEENTRY	pe
	);

/* Should be done in user-mode using shared GDI Objects. */
UINT STDCALL  NtGdiGetTextAlign(HDC  hDC);

/* Should be done in user-mode using shared GDI Objects. */
UINT
STDCALL
NtGdiGetTextCharset(HDC  hDC);

/* Needs to be done in user-mode, using shared GDI Object Attributes. */
COLORREF STDCALL  NtGdiGetTextColor(HDC  hDC);

/* Rename to NtGdiGetTextExtentExW. Add 0 at the end. */
BOOL
STDCALL
NtGdiGetTextExtentExPoint(HDC  hDC,
                               LPCWSTR String,
                               int  Count,
                               int  MaxExtent,
                               LPINT  Fit,
                               LPINT  Dx,
                               LPSIZE  Size);

/* Rename to NtGdiGetTextFaceW, add FALSE at the end. */
int
STDCALL
NtGdiGetTextFace(HDC  hDC,
                     int  Count,
                     LPWSTR  FaceName);

/* Use NtGdiGetTextMetricsW with 0 at the end */
BOOL
STDCALL
NtGdiGetTextMetrics(HDC  hDC,
                         LPTEXTMETRICW  tm);

/* Use NtGdiGetDCPoint with GdiGetViewPortExt */
BOOL STDCALL  NtGdiGetViewportExtEx(HDC  hDC, LPSIZE viewportExt);

/* Needs to be done in user-mode. */
BOOL STDCALL  NtGdiGetViewportOrgEx(HDC  hDC, LPPOINT viewportOrg);

/* Metafiles are user-mode. */
UINT
STDCALL
NtGdiGetWinMetaFileBits (
	HENHMETAFILE	hemf,
	UINT		BufSize,
	LPBYTE		Buffer,
	INT		MapMode,
	HDC		Ref
	);

/* Needs to be done in user-mode. */
BOOL STDCALL  NtGdiGetWindowExtEx(HDC  hDC, LPSIZE windowExt);

/* Needs to be done in user-mode. */
BOOL STDCALL  NtGdiGetWindowOrgEx(HDC  hDC, LPPOINT windowOrg);

/* Use NtGdiGetTransform with GdiWorldSpaceToPageSpace */
BOOL
STDCALL
NtGdiGetWorldTransform (
	HDC	hDC,
	LPXFORM	Xform
	);

/* Use NtGdiTransformPoints with GdiDpToLp */
BOOL
STDCALL
NtGdiLPtoDP (
	HDC	hDC,
	LPPOINT	Points,
	int	Count
	);

/* Needs to be done in user-mode. */
BOOL
STDCALL
NtGdiMoveToEx(HDC  hDC,
                   int  X,
                   int  Y,
                   LPPOINT  Point);

/* Needs to be done in user-mode. */
BOOL
STDCALL
NtGdiOffsetViewportOrgEx (
	HDC	hDC,
	int	XOffset,
	int	YOffset,
	LPPOINT	Point
	);

/* Needs to be done in user-mode. */
BOOL
STDCALL
NtGdiOffsetWindowOrgEx (
	HDC	hDC,
	int	XOffset,
	int	YOffset,
	LPPOINT	Point
	);

/* Use NtGdiFillRgn. Add 0 at the end. */
BOOL
STDCALL
NtGdiPaintRgn(HDC  hDC,
                   HRGN  hRgn);

/* Use NtGdiArcInternal with GdiTypePie. */
BOOL
STDCALL
NtGdiPie(HDC  hDC,
              int  LeftRect,
              int  TopRect,
              int  RightRect,
              int  BottomRect,
              int  XRadial1,
              int  YRadial1,
              int  XRadial2,
              int  YRadial2);

/* Metafiles are user-mode. */
BOOL
STDCALL
NtGdiPlayEnhMetaFile (
	HDC		hDC,
	HENHMETAFILE	hemf,
	CONST PRECT	Rect
	);

/* Metafiles are user-mode. */
BOOL
STDCALL
NtGdiPlayEnhMetaFileRecord (
	HDC			hDC,
	LPHANDLETABLE		Handletable,
	CONST ENHMETARECORD	* EnhMetaRecord,
	UINT			Handles
	);

/* Metafiles are user-mode. */
BOOL
STDCALL
NtGdiPlayMetaFile (
	HDC		hDC,
	HMETAFILE	hmf
	);

/* Metafiles are user-mode. */
BOOL
STDCALL
NtGdiPlayMetaFileRecord (
	HDC		hDC,
	LPHANDLETABLE	Handletable,
	LPMETARECORD	MetaRecord,
	UINT		Handles
	);

/* Use NtGdiPolyPolyDraw with GdiPolyBezier. */
BOOL
STDCALL
NtGdiPolyBezier(HDC  hDC,
                     CONST LPPOINT  pt,
                     DWORD  Count);

/* Use NtGdiPolyPolyDraw with GdiPolyBezierTo. */
BOOL
STDCALL
NtGdiPolyBezierTo(HDC  hDC,
                       CONST LPPOINT  pt,
                       DWORD  Count);

/* Use NtGdiPolyPolyDraw with GdiPolyPolyLine. */
BOOL
STDCALL
NtGdiPolyline(HDC  hDC,
                   CONST LPPOINT  pt,
                   int  Count);

/* Use NtGdiPolyPolyDraw with GdiPolyLineTo. */
BOOL
STDCALL
NtGdiPolylineTo(HDC  hDC,
                     CONST LPPOINT  pt,
                     DWORD  Count);

/* Use NtGdiPolyPolyDraw with GdiPolyPolyLine. */
BOOL
STDCALL
NtGdiPolyPolyline(HDC  hDC,
                       CONST LPPOINT  pt,
                       CONST LPDWORD  PolyPoints,
                       DWORD  Count);

/* Use NtGdiPolyTextOutW with 0 at the end. */
BOOL
STDCALL
NtGdiPolyTextOut(HDC  hDC,
                      CONST LPPOLYTEXTW txt,
                      int  Count);

/* Use NtGdiPolyPolyDraw with GdiPolyPolygon. */
BOOL
STDCALL
NtGdiPolygon(HDC  hDC,
                  CONST PPOINT  Points,
                  int  Count);

/* Use NtGdiPolyPolyDraw with GdiPolyPolygon. */
BOOL
STDCALL
NtGdiPolyPolygon(HDC  hDC,
                      CONST LPPOINT  Points,
                      CONST LPINT  PolyCounts,
                      int  Count);

/* Call UserRealizePalette. */
UINT
STDCALL
NtGdiRealizePalette (
	HDC	hDC
	);

/* Should be done in user-mode. */
BOOL
STDCALL
NtGdiRemoveFontResource(LPCWSTR  FileName);

/* Use NtGdiExtSelectClipRgn with RGN_COPY. */
int
STDCALL
NtGdiSelectClipRgn (
	HDC	hDC,
	HRGN	hrgn
	);

/* Should be done in user-mode. */
HGDIOBJ STDCALL  NtGdiSelectObject(HDC  hDC, HGDIOBJ  hGDIObj);

/* Use NtUserSelectPalette. */
HPALETTE
STDCALL
NtGdiSelectPalette (
	HDC		hDC,
	HPALETTE	hpal,
	BOOL		ForceBackground
	);

/* Should be done in user-mode. */
INT
STDCALL
NtGdiSetAbortProc(HDC  hDC,
                      ABORTPROC  AbortProc);

/* Use NtGdiGetAndSetDCDword with GdiGetSetArcDirection. */
int
STDCALL
NtGdiSetArcDirection(HDC  hDC,
                         int  ArcDirection);

/* Use NtGdiSetBitmapDimension. */
BOOL
STDCALL
NtGdiSetBitmapDimensionEx (
	HBITMAP	hBitmap,
	INT	Width,
	INT	Height,
	LPSIZE	Size
	);

/* Needs to be done in user-mode, using shared GDI Object Attributes. */
COLORREF STDCALL NtGdiSetBkColor (HDC hDC, COLORREF Color);

/* Needs to be done in user-mode, using shared GDI Object Attributes. */
INT STDCALL  NtGdiSetBkMode(HDC  hDC, INT  backgroundMode);

/* Use NtGdiSetBrushOrg. */
BOOL STDCALL
NtGdiSetBrushOrgEx(
   HDC hDC,
   INT XOrg,
   INT YOrg,
   LPPOINT Point);

/* Use NtGdiDoPalette with GdiPalSetColorTable, TRUE. */
UINT
STDCALL
NtGdiSetDIBColorTable (
	HDC		hDC,
	UINT		StartIndex,
	UINT		Entries,
	CONST RGBQUAD	* Colors
	);

/* Use SetDIBitsToDevice in gdi32. */
INT
STDCALL
NtGdiSetDIBits (
	HDC			hDC,
	HBITMAP			hBitmap,
	UINT			StartScan,
	UINT			ScanLines,
	CONST VOID		* Bits,
	CONST BITMAPINFO	* bmi,
	UINT			ColorUse
	);

/* Use NtGdiSetDIBitsToDeviceInternal. */
INT
STDCALL
NtGdiSetDIBitsToDevice (
	HDC			hDC,
	INT			XDest,
	INT			YDest,
	DWORD			Width,
	DWORD			Height,
	INT			XSrc,
	INT			YSrc,
	UINT			StartScan,
	UINT			ScanLines,
	CONST VOID		* Bits,
	CONST BITMAPINFO	* bmi,
	UINT			ColorUse
	);

/* Metafiles are user-mode. */
HENHMETAFILE
STDCALL
NtGdiSetEnhMetaFileBits (
	UINT		BufSize,
	CONST PBYTE	Data
	);

/* Needs to be done in user-mode, using shared GDI Object Attributes. */
int
STDCALL
NtGdiSetGraphicsMode (
	HDC	hDC,
	int	Mode
	);

/* Use NtGdiSetIcmMode. */
INT
STDCALL
NtGdiSetICMMode(HDC  hDC,
                    INT  EnableICM);

/* Should be done in user-mode. */
BOOL
STDCALL
NtGdiSetICMProfile(HDC  hDC,
                        LPWSTR  Filename);

/* Needs to be done in user-mode, using shared GDI Object Attributes. */
int
STDCALL
NtGdiSetMapMode (
	HDC	hDC,
	int	MapMode
	);

/* Needs to be done in user-mode, using shared GDI Object Attributes. */
DWORD
STDCALL
NtGdiSetMapperFlags(HDC  hDC,
                          DWORD  Flag);

/* Metafiles are user-mode. */
HMETAFILE
STDCALL
NtGdiSetMetaFileBitsEx (
	UINT		Size,
	CONST PBYTE	Data
	);

/* Use NtGdiDoPalette with GdiPalSetEntries, TRUE. */
UINT
STDCALL
NtGdiSetPaletteEntries (
	HPALETTE		hpal,
	UINT			Start,
	UINT			Entries,
	CONST LPPALETTEENTRY	pe
	);

/* Use NtGdiSetPixel(hdc, x, y, color) != CLR_INVALID; */
BOOL
STDCALL
NtGdiSetPixelV (
	HDC		hDC,
	INT		X,
	INT		Y,
	COLORREF	Color
	);

/* Needs to be done in user-mode, using shared GDI Object Attributes. */
INT STDCALL  NtGdiSetPolyFillMode(HDC  hDC, INT polyFillMode);

/* Needs to be done in user-mode, using shared GDI Object Attributes. */
INT STDCALL  NtGdiSetROP2(HDC  hDC, INT  ROPmode);

/* Needs to be done in user-mode, using shared GDI Object Attributes. */
INT STDCALL  NtGdiSetStretchBltMode(HDC  hDC, INT  stretchBltMode);

/* Needs to be done in user-mode, using shared GDI Object Attributes. */
UINT
STDCALL
NtGdiSetTextAlign(HDC  hDC,
                       UINT  Mode);

/* Needs to be done in user-mode, using shared GDI Object Attributes. */
COLORREF STDCALL  NtGdiSetTextColor(HDC hDC, COLORREF color);

/* Needs to be done in user-mode. */
BOOL
STDCALL
NtGdiSetWindowExtEx (
	HDC	hDC,
	int	XExtent,
	int	YExtent,
	LPSIZE	Size
	);

/* Needs to be done in user-mode. */
BOOL
STDCALL
NtGdiSetViewportOrgEx (
	HDC	hDC,
	int	X,
	int	Y,
	LPPOINT	Point
	);

/* Needs to be done in user-mode. */
BOOL
STDCALL
NtGdiSetViewportExtEx (
	HDC	hDC,
	int	XExtent,
	int	YExtent,
	LPSIZE	Size
	);

/* Needs to be done in user-mode. */
BOOL
STDCALL
NtGdiSetWindowOrgEx (
	HDC	hDC,
	int	X,
	int	Y,
	LPPOINT	Point
	);

/* Use NtGdiModifyWorldTransform with MWT_MAX + 1; */
BOOL
STDCALL
NtGdiSetWorldTransform (
	HDC		hDC,
	CONST LPXFORM	Xform
	);

/* Use NtGdiStretchDIBitsInternal. */
INT
STDCALL
NtGdiStretchDIBits (
	HDC			hDC,
	INT			XDest,
	INT			YDest,
	INT			DestWidth,
	INT			DestHeight,
	INT			XSrc,
	INT			YSrc,
	INT			SrcWidth,
	INT			SrcHeight,
	CONST VOID		* Bits,
	CONST BITMAPINFO	* BitsInfo,
	UINT			Usage,
	DWORD			ROP
	);

/* Use NtGdiExtTextOutW with 0, 0 at the end. */
BOOL
STDCALL
NtGdiTextOut(HDC  hDC,
                  int  XStart,
                  int  YStart,
                  LPCWSTR  String,
                  int  Count);

/* Needs to be done in user-mode. */
BOOL
STDCALL
NtGdiUpdateICMRegKey(DWORD  Reserved,  
                          LPWSTR  CMID, 
                          LPWSTR  Filename,
                          UINT  Command);

/* These shouldn't even be called NtGdi */
HDC STDCALL  NtGdiGetDCState(HDC  hDC);
WORD STDCALL NtGdiSetHookFlags(HDC hDC, WORD Flags);
INT
STDCALL
NtGdiSelectVisRgn(HDC hdc,
                     HRGN hrgn);
VOID STDCALL NtGdiSetDCState ( HDC hDC, HDC hDCSave );

/* All this Should be in user-mode, not NtUser calls. Especially not in GDI! */
DWORD
NTAPI
NtUserCallTwoParam(
  DWORD Param1,
  DWORD Param2,
  DWORD Routine);

#define TWOPARAM_ROUTINE_SETDCPENCOLOR      0x45
#define TWOPARAM_ROUTINE_SETDCBRUSHCOLOR    0x46
#define TWOPARAM_ROUTINE_GETDCCOLOR         0x47

#define NtUserGetDCBrushColor(hbr) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), OBJ_BRUSH, TWOPARAM_ROUTINE_GETDCCOLOR)

#define NtUserGetDCPenColor(hbr) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), OBJ_PEN, TWOPARAM_ROUTINE_GETDCCOLOR)

#define NtUserSetDCBrushColor(hbr, crColor) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), (DWORD)crColor, TWOPARAM_ROUTINE_SETDCBRUSHCOLOR)

#define NtUserSetDCPenColor(hbr, crColor) \
  (COLORREF)NtUserCallTwoParam((DWORD)(hbr), (DWORD)crColor, TWOPARAM_ROUTINE_SETDCPENCOLOR)

#endif /* WIN32K_NTGDI_BAD_INCLUDED */
