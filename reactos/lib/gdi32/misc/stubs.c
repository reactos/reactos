/* $Id: stubs.c,v 1.20 2003/06/26 21:52:40 gvg Exp $
 *
 * reactos/lib/gdi32/misc/stubs.c
 *
 * GDI32.DLL Stubs
 *
 * When you implement one of these functions,
 * remove its stub from this file.
 *
 */
#ifdef UNICODE
#undef UNICODE
#endif
#include <windows.h>


BOOL
STDCALL
AnimatePalette(
	HPALETTE		a0,
	UINT			a1,
	UINT			a2,
	CONST PALETTEENTRY	*a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
Arc(
	HDC	a0,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	int	a5,
	int	a6,
	int	a7,
	int	a8
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
CancelDC(
	HDC	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
Chord(
	HDC	a0,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	int	a5,
	int	a6,
	int	a7,
	int	a8
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



int
STDCALL
ChoosePixelFormat(
	HDC				a0,
	CONST PIXELFORMATDESCRIPTOR	*a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HMETAFILE
STDCALL
CloseMetaFile(
	HDC	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HBRUSH
STDCALL
CreateBrushIndirect(
	CONST LOGBRUSH	*a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HBRUSH
STDCALL
CreateDIBPatternBrush(
	HGLOBAL			a0,
	UINT			a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HBRUSH
STDCALL
CreateDIBPatternBrushPt(
	CONST VOID		*a0,
	UINT			a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HRGN
STDCALL
CreateEllipticRgn(
	int			a0,
	int			a1,
	int			a2,
	int			a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HRGN
STDCALL
CreateEllipticRgnIndirect(
	CONST RECT		*a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HBRUSH
STDCALL
CreateHatchBrush(
	int		a0,
	COLORREF	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HPALETTE
STDCALL
CreatePalette(
	CONST LOGPALETTE	*a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

BOOL
STDCALL
DeleteMetaFile(
	HMETAFILE	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



int
STDCALL
DescribePixelFormat(
	HDC			a0,
	int			a1,
	UINT			a2,
	LPPIXELFORMATDESCRIPTOR	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
DrawEscape(
	HDC		a0,
	int		a1,
	int		a2,
	LPCSTR		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
Ellipse(
	HDC		a0,
	int		a1,
	int		a2,
	int		a3,
	int		a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



int
STDCALL
EnumObjects(
	HDC		a0,
	int		a1,
	ENUMOBJECTSPROC	a2,
	LPARAM		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
Escape(
	HDC		a0,
	int		a1,
	int		a2,
	LPCSTR		a3,
	LPVOID		a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}




int
STDCALL
ExtEscape(
	HDC		a0,
	int		a1,
	int		a2,
	LPCSTR		a3,
	int		a4,
	LPSTR		a5
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
ExcludeClipRect(
	HDC	a0,
	int	a1,
	int	a2,
	int	a3,
	int	a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HRGN
STDCALL
ExtCreateRegion(
	CONST XFORM *	a0,
	DWORD		a1,
	CONST RGNDATA *	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
ExtFloodFill(
	HDC		a0,
	int		a1,
	int		a2,
	COLORREF	a3,
	UINT		a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
FillRgn(
	HDC	a0,
	HRGN	a1,
	HBRUSH	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
FloodFill(
	HDC		a0,
	int		a1,
	int		a2,
	COLORREF	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
FrameRgn(
	HDC	a0,
	HRGN	a1,
	HBRUSH	a2,
	int	a3,
	int	a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



int
STDCALL
GetROP2(
	HDC	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
GetAspectRatioFilterEx(
	HDC	a0,
	LPSIZE	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



COLORREF
STDCALL
GetBkColor(
	HDC	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
GetBkMode(
	HDC	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
GetBoundsRect(
	HDC	a0,
	LPRECT	a1,
	UINT	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
GetBrushOrgEx(
	HDC	a0,
	LPPOINT	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
GetClipRgn(
	HDC	a0,
	HRGN	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
GetMetaRgn(
	HDC	a0,
	HRGN	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HGDIOBJ
STDCALL
GetCurrentObject(
	HDC	a0,
	UINT	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
GetCurrentPositionEx(
	HDC	a0,
	LPPOINT	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}




DWORD
STDCALL
GetFontData(
	HDC	a0,
	DWORD	a1,
	DWORD	a2,
	LPVOID	a3,
	DWORD	a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
GetGraphicsMode(
	HDC	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}




int
STDCALL
GetMapMode(
	HDC	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
GetMetaFileBitsEx(
	HMETAFILE	a0,
	UINT		a1,
	LPVOID		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



COLORREF
STDCALL
GetNearestColor(
	HDC		a0,
	COLORREF	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
GetNearestPaletteIndex(
	HPALETTE	a0,
	COLORREF	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD
STDCALL
GetObjectType(
	HGDIOBJ		a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
STDCALL
GetPaletteEntries(
	HPALETTE	a0,
	UINT		a1,
	UINT		a2,
	LPPALETTEENTRY	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



COLORREF
STDCALL
GetPixel(
	HDC	a0,
	int	a1,
	int	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
GetPixelFormat(
	HDC	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}




BOOL
STDCALL
GetRasterizerCaps(
	LPRASTERIZER_STATUS	a0,
	UINT			a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}






int
STDCALL
GetStretchBltMode(
	HDC	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
GetSystemPaletteEntries(
	HDC		a0,
	UINT		a1,
	UINT		a2,
	LPPALETTEENTRY	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
GetSystemPaletteUse(
	HDC	hDc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
GetTextCharacterExtra(
	HDC	hDc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
GetTextAlign(
	HDC	hDc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



COLORREF
STDCALL
GetTextColor(
	HDC	hDc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetTextCharset(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
GetTextCharsetInfo(
	HDC		hdc,
	LPFONTSIGNATURE	lpSig,
	DWORD		dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
TranslateCharsetInfo(
	DWORD FAR	*lpSrc,
	LPCHARSETINFO	lpCs,
	DWORD		dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



DWORD
STDCALL
GetFontLanguageInfo(
	HDC 	hDc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
GetViewportExtEx(
	HDC	hDc,
	LPSIZE	lpSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
GetViewportOrgEx(
	HDC		hDc,
	LPPOINT		lpPoint
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
GetWindowExtEx(
	HDC		hDc,
	LPSIZE		lpSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
GetWindowOrgEx(
	HDC		hDc,
	LPPOINT		lpPoint
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



int
STDCALL
IntersectClipRect(
	HDC		hDc,
	int		a1,
	int		a2,
	int		a3,
	int		a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
InvertRgn(
	HDC	hDc,
	HRGN	hRgn
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
LineDDA(
	int		a0,
	int		a1,
	int		a2,
	int		a3,
	LINEDDAPROC	a4,
	LPARAM		a5
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}




int
STDCALL
OffsetClipRgn(
	HDC	a0,
	int	a1,
	int	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}





BOOL
STDCALL
Pie(
	HDC	a0,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	int	a5,
	int	a6,
	int	a7,
	int	a8
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
PlayMetaFile(
	HDC		a0,
	HMETAFILE	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}






BOOL
STDCALL
PolyPolygon(
	HDC		a0,
	CONST POINT	*a1,
	CONST INT	*a2,
	int		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
PtInRegion(
	HRGN	a0,
	int	a1,
	int	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
PtVisible(
	HDC	a0,
	int	a1,
	int	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
RectInRegion(
	HRGN		a0,
	CONST RECT	*a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
RectVisible(
	HDC		a0,
	CONST RECT	*a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
RestoreDC(
	HDC	a0,
	int	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
RoundRect(
	HDC	a0,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	int	a5,
	int	a6
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
ResizePalette(
	HPALETTE	a0,
	UINT		a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



int
STDCALL
SaveDC(
	HDC	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
ExtSelectClipRgn(
	HDC	a0,
	HRGN	a1,
	int	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
SetMetaRgn(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



COLORREF
STDCALL
SetBkColor(
	HDC		a0,
	COLORREF	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
SetBkMode(
	HDC	a0,
	int	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
SetBoundsRect(
	HDC		a0,
	CONST RECT	*a1,
	UINT		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD
STDCALL
SetMapperFlags(
	HDC	a0,
	DWORD	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
SetGraphicsMode(
	HDC	hdc,
	int	iMode
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HMETAFILE
STDCALL
SetMetaFileBitsEx(
	UINT		a0,
	CONST BYTE	*a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
SetPaletteEntries(
	HPALETTE		a0,
	UINT			a1,
	UINT			a2,
	CONST PALETTEENTRY	*a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
SetPixelV(
	HDC		a0,
	int		a1,
	int		a2,
	COLORREF	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
SetPixelFormat(
	HDC				a0,
	int				a1,
	CONST PIXELFORMATDESCRIPTOR	*a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
SetROP2(
	HDC	a0,
	int	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
SetStretchBltMode(
	HDC	a0,
	int	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
SetSystemPaletteUse(
	HDC	a0,
	UINT	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
SetTextCharacterExtra(
	HDC	a0,
	int	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
SetTextAlign(
	HDC	a0,
	UINT	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
SetTextJustification(
	HDC	a0,
	int	a1,
	int	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
UpdateColors(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
PlayMetaFileRecord(
	HDC		a0,
	LPHANDLETABLE	a1,
	LPMETARECORD	a2,
	UINT		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
EnumMetaFile(
	HDC			a0,
	HMETAFILE		a1,
	ENUMMETAFILEPROC	a2,
	LPARAM			a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



HENHMETAFILE
STDCALL
CloseEnhMetaFile(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
DeleteEnhMetaFile(
	HENHMETAFILE	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
EnumEnhMetaFile(
	HDC		a0,
	HENHMETAFILE	a1,
	ENHMETAFILEPROC	a2,
	LPVOID		a3,
	CONST RECT	*a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



UINT
STDCALL
GetEnhMetaFileBits(
	HENHMETAFILE	a0,
	UINT		a1,
	LPBYTE		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
GetEnhMetaFileHeader(
	HENHMETAFILE	a0,
	UINT		a1,
	LPENHMETAHEADER	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
GetEnhMetaFilePaletteEntries(
	HENHMETAFILE	a0,
	UINT		a1,
	LPPALETTEENTRY	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
GetEnhMetaFilePixelFormat(
	HENHMETAFILE			a0,
	DWORD				a1,
	CONST PIXELFORMATDESCRIPTOR	*a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
GetWinMetaFileBits(
	HENHMETAFILE	a0,
	UINT		a1,
	LPBYTE		a2,
	INT		a3,
	HDC		a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
PlayEnhMetaFile(
	HDC		a0,
	HENHMETAFILE	a1,
	CONST RECT	*a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
PlayEnhMetaFileRecord(
	HDC			a0,
	LPHANDLETABLE		a1,
	CONST ENHMETARECORD	*a2,
	UINT			a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



HENHMETAFILE
STDCALL
SetEnhMetaFileBits(
	UINT		a0,
	CONST BYTE	*a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HENHMETAFILE
STDCALL
SetWinMetaFileBits(
	UINT			a0,
	CONST BYTE		*a1,
	HDC			a2,
//	CONST METAFILEPICT	*a3
		   PVOID a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
GdiComment(
	HDC		a0,
	UINT		a1,
	CONST BYTE	*a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
AngleArc(
	HDC	hdc,
	int	a1,
	int	a2,
	DWORD	a3,
	FLOAT	a4,
	FLOAT	a5
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
PolyPolyline(
	HDC		hdc,
	CONST POINT	*a1,
	CONST DWORD	*a2,
	DWORD		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
GetWorldTransform(
	HDC		hdc,
	LPXFORM		a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
SetWorldTransform(
	HDC		a0,
	CONST XFORM	*a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
ModifyWorldTransform(
	HDC		a0,
	CONST XFORM	*a1,
	DWORD		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
CombineTransform(
	LPXFORM		a0,
	CONST XFORM	*a1,
	CONST XFORM	*a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



UINT
STDCALL
GetDIBColorTable(
	HDC		hdc,
	UINT		a1,
	UINT		a2,
	RGBQUAD		*a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
SetDIBColorTable(
	HDC		hdc,
	UINT		a1,
	UINT		a2,
	CONST RGBQUAD	*a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
SetColorAdjustment(
	HDC			hdc,
	CONST COLORADJUSTMENT	*a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
GetColorAdjustment(
	HDC			hdc,
	LPCOLORADJUSTMENT	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



HPALETTE
STDCALL
CreateHalftonePalette(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
EndDoc(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
StartPage(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
EndPage(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
AbortDoc(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
SetAbortProc(
	HDC		hdc,
	ABORTPROC	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
AbortPath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
ArcTo(
	HDC	hdc,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	int	a5,
	int	a6,
	int	a7,
	int	a8
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
BeginPath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
CloseFigure(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
EndPath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
FillPath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
FlattenPath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



int
STDCALL
GetPath(
	HDC		hdc,
	LPPOINT		a1,
	LPBYTE		a2,
	int		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HRGN
STDCALL
PathToRegion(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
PolyDraw(
	HDC		hdc,
	CONST POINT	*a1,
	CONST BYTE	*a2,
	int		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
SelectClipPath(
	HDC	hdc,
	int	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



int
STDCALL
SetArcDirection(
	HDC	hdc,
	int	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
SetMiterLimit(
	HDC	hdc,
	FLOAT	a1,
	PFLOAT	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
StrokeAndFillPath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
StrokePath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
WidenPath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



HPEN
STDCALL
ExtCreatePen(
	DWORD		a0,
	DWORD		a1,
	CONST LOGBRUSH	*a2,
	DWORD		a3,
	CONST DWORD	*a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
GetMiterLimit(
	HDC	hdc,
	PFLOAT	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



int
STDCALL
GetArcDirection(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HRGN
STDCALL
CreatePolygonRgn(
	CONST POINT	*a0,
	int		a1,
	int		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
DPtoLP(
	HDC	a0,
	LPPOINT	a1,
	int	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
PolyBezier(
	HDC		a0,
	CONST POINT	*a1,
	DWORD		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
PolyBezierTo(
	HDC		a0,
	CONST POINT	*a1,
	DWORD		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
PolylineTo(
	HDC		a0,
	CONST POINT	*a1,
	DWORD		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
SetViewportExtEx(
	HDC	a0,
	int	a1,
	int	a2,
	LPSIZE	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
SetWindowExtEx(
	HDC	a0,
	int	a1,
	int	a2,
	LPSIZE	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
OffsetWindowOrgEx(
	HDC	a0,
	int	a1,
	int	a2,
	LPPOINT	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
ScaleViewportExtEx(
	HDC	a0,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	LPSIZE	a5
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
ScaleWindowExtEx(
	HDC	a0,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	LPSIZE	a5
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
SetBitmapDimensionEx(
	HBITMAP	a0,
	int	a1,
	int	a2,
	LPSIZE	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
SetBrushOrgEx(
	HDC	a0,
	int	a1,
	int	a2,
	LPPOINT	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
GetDCOrgEx(
	HDC	a0,
	LPPOINT	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
FixBrushOrgEx(
	HDC	a0,
	int	a1,
	int	a2,
	LPPOINT	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
UnrealizeObject(
	HGDIOBJ	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
GdiFlush()
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



DWORD
STDCALL
GdiSetBatchLimit(
	DWORD	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD
STDCALL
GdiGetBatchLimit()
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
SetICMMode(
	HDC	a0,
	int	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
CheckColorsInGamut(
	HDC	a0,
	LPVOID	a1,
	LPVOID	a2,
	DWORD	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


HANDLE
STDCALL
GetColorSpace(
	HDC	hDc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
SetColorSpace(
	HDC		a0,
	HCOLORSPACE	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
DeleteColorSpace(
	HCOLORSPACE	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
GetDeviceGammaRamp(
	HDC	a0,
	LPVOID	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
SetDeviceGammaRamp(
	HDC	a0,
	LPVOID	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
ColorMatchToTarget(
	HDC	a0,
	HDC	a1,
	DWORD	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
wglCopyContext(
	HGLRC	a0,
	HGLRC	a1,
	UINT	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



HGLRC
STDCALL
wglCreateContext(
	HDC	hDc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HGLRC
STDCALL
wglCreateLayerContext(
	HDC	hDc,
	int	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
wglDeleteContext(
	HGLRC	a
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



HGLRC
STDCALL
wglGetCurrentContext(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HDC
STDCALL
wglGetCurrentDC(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



PROC
STDCALL
wglGetProcAddress(
	LPCSTR		a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



BOOL
STDCALL
wglMakeCurrent(
	HDC	a0,
	HGLRC	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
wglShareLists(
	HGLRC	a0,
	HGLRC	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
SwapBuffers(
	HDC	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



BOOL
STDCALL
wglDescribeLayerPlane(
	HDC			a0,
	int			a1,
	int			a2,
	UINT			a3,
	LPLAYERPLANEDESCRIPTOR	a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
wglSetLayerPaletteEntries(
	HDC		a0,
	int		a1,
	int		a2,
	int		a3,
	CONST COLORREF	*a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
wglGetLayerPaletteEntries(
	HDC		a0,
	int		a1,
	int		a2,
	int		a3,
	CONST COLORREF	*a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
wglRealizeLayerPalette(
	HDC		a0,
	int		a1,
	BOOL		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
wglSwapLayerBuffers(
	HDC		a0,
	UINT		a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* === AFTER THIS POINT I GUESS... =========
 * (based on stack size in Norlander's .def)
 * === WHERE ARE THEY DEFINED? =============
 */


DWORD
STDCALL
GdiPlayDCScript(
	DWORD	a0,
	DWORD	a1,
	DWORD	a2,
	DWORD	a3,
	DWORD	a4,
	DWORD	a5
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GdiPlayJournal(
	DWORD	a0,
	DWORD	a1,
	DWORD	a2,
	DWORD	a3,
	DWORD	a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GdiPlayScript(
	DWORD	a0,
	DWORD	a1,
	DWORD	a2,
	DWORD	a3,
	DWORD	a4,
	DWORD	a5,
	DWORD	a6
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
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
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetRandomRgn(
	DWORD	a0,
	DWORD	a1,
	DWORD	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetRelAbs(
	DWORD	a0,
	DWORD	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD
STDCALL
SelectBrushLocal(
	DWORD	a0,
	DWORD	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD
STDCALL
SelectFontLocal(
	DWORD	a0,
	DWORD	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
SetFontEnumeration(
	DWORD	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
SetRelAbs(
	DWORD	a0,
	DWORD	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
gdiPlaySpoolStream(
	DWORD	a0,
	DWORD	a1,
	DWORD	a2,
	DWORD	a3,
	DWORD	a4,
	DWORD	a5
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetFontResourceInfo(
	DWORD	a0,
	DWORD	a1,
	DWORD	a2,
	DWORD	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/* EOF */
