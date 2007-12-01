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

/* The gdi32 call does all the work in user-mode, save for NtGdiMakeFontDir */
BOOL
NTAPI
NtGdiCreateScalableFontResource(
    DWORD Hidden,
    LPCWSTR FontRes,
    LPCWSTR FontFile,
    LPCWSTR CurrentPath
);

/* The gdi32 call Should Use NtGdiGetTextExtent */
BOOL
NTAPI
NtGdiGetTextExtentPoint32(
    HDC hDC,
    LPCWSTR String,
    int Count,
    LPSIZE
);

/* Use NtGdiAddFontResourceW */
int
STDCALL
NtGdiAddFontResource(PUNICODE_STRING Filename,
					 DWORD fl);

/* Metafiles are user mode */
HENHMETAFILE
STDCALL
NtGdiCloseEnhMetaFile (
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

/* Metafiles are user mode */
HDC
STDCALL
NtGdiCreateEnhMetaFile (
	HDC		hDCRef,
	LPCWSTR		File,
	CONST LPRECT	Rect,
	LPCWSTR		Description
	);

/* Use NtGdiPolyPolyDraw with PolyPolyRgn. */
HRGN
STDCALL
NtGdiCreatePolygonRgn(CONST PPOINT  pt,
                           INT  Count,
                           INT  PolyFillMode);

/* Meta are user-mode. */
BOOL
STDCALL
NtGdiDeleteEnhMetaFile (
	HENHMETAFILE	emf
	);

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

/* Should be done in user-mode. */
BOOL
STDCALL
NtGdiGdiComment (
	HDC		hDC,
	UINT		Size,
	CONST LPBYTE	Data
	);

/* Should be done in user-mode. */
BOOL
STDCALL
NtGdiGetAspectRatioFilterEx(HDC  hDC,
                                 LPSIZE  AspectRatio);

/* Use NtGdiGetColorSpaceforBitmap. */
HCOLORSPACE
STDCALL
NtGdiGetColorSpace(HDC  hDC);

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
INT
STDCALL
NtGdiGetPixelFormat(HDC  hDC);

/* Should be done in user-mode using shared GDI Objects. */
UINT
STDCALL
NtGdiGetTextCharset(HDC  hDC);

/* Use NtGdiGetDCPoint with GdiGetViewPortExt */
BOOL STDCALL  NtGdiGetViewportExtEx(HDC  hDC, LPSIZE viewportExt);

/* Needs to be done in user-mode. */
BOOL STDCALL  NtGdiGetViewportOrgEx(HDC  hDC, LPPOINT viewportOrg);

/* Needs to be done in user-mode. */
BOOL STDCALL  NtGdiGetWindowExtEx(HDC  hDC, LPSIZE windowExt);

/* Needs to be done in user-mode. */
BOOL STDCALL  NtGdiGetWindowOrgEx(HDC  hDC, LPPOINT windowOrg);

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

/* Use NtGdiPolyTextOutW with 0 at the end. */
BOOL
STDCALL
NtGdiPolyTextOut(HDC  hDC,
                      CONST LPPOLYTEXTW txt,
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

/* Metafiles are user-mode. */
HENHMETAFILE
STDCALL
NtGdiSetEnhMetaFileBits (
	UINT		BufSize,
	CONST PBYTE	Data
	);

/* Should be done in user-mode. */
BOOL
STDCALL
NtGdiSetICMProfile(HDC  hDC,
                        LPWSTR  Filename);

/* Needs to be done in user-mode, using shared GDI Object Attributes. */
DWORD
STDCALL
NtGdiSetMapperFlags(HDC  hDC,
                          DWORD  Flag);

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

/* Needs to be done in user-mode. */
BOOL
STDCALL
NtGdiUpdateICMRegKey(DWORD  Reserved,
                          LPWSTR  CMID,
                          LPWSTR  Filename,
                          UINT  Command);


#endif /* WIN32K_NTGDI_BAD_INCLUDED */


/* Follow thing need be rewriten
 *
 * Opengl icd are complete hacked in reactos and are using own way, this need be rewriten and be setup with the correct syscall
 * and the opengl32 shall using correct syscall to optain then driver interface or using the correct version in gdi32.
 * it mean whole icd are hacked in frist place and need be rewtiten from scrash. and it need enum the opengl correct way and
 * export the driver correct
 *
 * DirectX aka ReactX alot api that have been implement in reactos win32k for ReactX shall move to a file call dxg.sys
 * there from it will really doing the stuff. And we should setup loading of dxg.sys
 *
 *  The Init of Gdi subsystem shall move into NtGdiInit()
 *
 *  The Init of spooler are done in NtGdiInitSpool()
 *
 *  The Init of the User subsystem shall move into NtUserInit()
 */
