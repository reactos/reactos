/* $Id: stubs.c,v 1.48 2004/02/08 21:37:52 weiden Exp $
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
#include <ddentry.h>

/*
 * @unimplemented
 */
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



/*
 * @unimplemented
 */
BOOL
STDCALL
CancelDC(
	HDC	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
HMETAFILE
STDCALL
CloseMetaFile(
	HDC	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteMetaFile(
	HMETAFILE	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
UINT
STDCALL
GetSystemPaletteUse(
	HDC	hDc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetTextCharacterExtra(
	HDC	hDc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetTextCharset(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
DWORD
STDCALL
GetFontLanguageInfo(
	HDC 	hDc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
int
STDCALL
SetMetaRgn(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
BOOL
STDCALL
UpdateColors(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
HENHMETAFILE
STDCALL
CloseEnhMetaFile(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteEnhMetaFile(
	HENHMETAFILE	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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

/*
 * @unimplemented 
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
int
STDCALL
EndDoc(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
StartPage(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
EndPage(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
AbortDoc(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
BOOL
STDCALL
AbortPath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
BeginPath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
CloseFigure(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EndPath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
FillPath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
FlattenPath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
HRGN
STDCALL
PathToRegion(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
BOOL
STDCALL
StrokeAndFillPath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
StrokePath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
WidenPath(
	HDC	hdc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
BOOL
STDCALL
UnrealizeObject(
	HGDIOBJ	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GdiFlush()
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GdiSetBatchLimit(
	DWORD	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GdiGetBatchLimit()
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
HANDLE
STDCALL
GetColorSpace(
	HDC	hDc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteColorSpace(
	HCOLORSPACE	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
HGLRC
STDCALL
wglCreateContext(
	HDC	hDc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
BOOL
STDCALL
wglDeleteContext(
	HGLRC	a
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
HGLRC
STDCALL
wglGetCurrentContext(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
HDC
STDCALL
wglGetCurrentDC(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
PROC
STDCALL
wglGetProcAddress(
	LPCSTR		a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
DWORD
STDCALL
SetFontEnumeration(
	DWORD	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
HANDLE 
STDCALL 
AddFontMemResourceEx(
	PVOID pbFont,
	DWORD cbFont,
	PVOID pdv,
	DWORD *pcFonts
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int 
STDCALL 
AddFontResourceTracking(
	LPCSTR lpString,
	int unknown
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL 
AnyLinkedFonts(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBITMAP 
STDCALL
ClearBitmapAttributes(HBITMAP hbm, DWORD dwFlags)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBRUSH 
STDCALL
ClearBrushAttributes(HBRUSH hbm, DWORD dwFlags)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
ColorCorrectPalette(HDC hDC,HPALETTE hPalette,DWORD dwFirstEntry,DWORD dwNumOfEntries)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
EnableEUDC(BOOL enable)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int
STDCALL
EndFormPage(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
EudcLoadLinkW(LPCWSTR pBaseFaceName,LPCWSTR pEudcFontPath,INT iPriority,INT iFontLinkType)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
EudcUnloadLinkW(LPCWSTR pBaseFaceName,LPCWSTR pEudcFontPath)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
FontIsLinked(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int 
STDCALL
GdiAddFontResourceW(LPCWSTR filename,FLONG f,DESIGNVECTOR *pdv)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GdiAddGlsBounds(HDC hdc,LPRECT prc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GdiAlphaBlend(HDC hdcDst,LONG DstX,LONG DstY,LONG DstCx,LONG DstCy,HDC hdcSrc,LONG SrcX,LONG SrcY,LONG SrcCx,LONG SrcCy,BLENDFUNCTION BlendFunction)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GdiArtificialDecrementDriver(LPWSTR pDriverName,BOOL unknown)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiCleanCacheDC(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GdiConsoleTextOut(HDC hdc, POLYTEXTW *lpto,UINT nStrings, RECTL *prclBounds)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HDC
STDCALL
GdiConvertAndCheckDC(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBITMAP 
STDCALL
GdiConvertBitmap(HBITMAP hbm)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBRUSH
STDCALL
GdiConvertBrush(HBRUSH hbr)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HDC 
STDCALL
GdiConvertDC(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HFONT 
STDCALL
GdiConvertFont(HFONT hfont)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HPALETTE 
STDCALL
GdiConvertPalette(HPALETTE hpal)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HRGN
STDCALL
GdiConvertRegion(HRGN hregion)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HENHMETAFILE 
STDCALL
GdiConvertEnhMetaFile(HENHMETAFILE hmf)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiDeleteLocalDC(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int 
STDCALL
GdiDescribePixelFormat(HDC hdc,int ipfd,UINT cjpfd,PPIXELFORMATDESCRIPTOR ppfd)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiDrawStream(HDC dc, ULONG l, VOID *v)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HGDIOBJ 
STDCALL
GdiFixUpHandle(HGDIOBJ hobj)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GdiGetCodePage(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBRUSH 
STDCALL
GdiGetLocalBrush(HBRUSH hbr)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HDC 
STDCALL
GdiGetLocalDC(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HFONT 
STDCALL
GdiGetLocalFont(HFONT hfont)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiIsMetaFileDC(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiIsMetaPrintDC(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiIsPlayMetafileDC(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiReleaseDC(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiReleaseLocalDC(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiSetAttrs(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
GdiSetLastError(DWORD dwErrCode)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiSetPixelFormat(HDC hdc,int ipfd)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiTransparentBlt(HDC hdcDst, int xDst, int yDst, int cxDst, int cyDst,HDC hdcSrc, int xSrc, int ySrc, int cxSrc, int cySrc,COLORREF TransColor)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiValidateHandle(HGDIOBJ hobj)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiSwapBuffers(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID 
STDCALL
GdiSetServerAttr(HDC hdc,DWORD attr)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GetBitmapAttributes(HBITMAP hbm)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GetBrushAttributes(HBRUSH hbr)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GetCharABCWidthsI(
	HDC hdc,
	UINT giFirst,
	UINT cgi,
	LPWORD pgi,
	LPABC lpabc
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GetCharWidthI(
	HDC hdc,
	UINT giFirst,
	UINT cgi,
	LPWORD pgi,
	LPINT lpBuffer
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
COLORREF 
STDCALL
GetDCBrushColor(
	HDC hdc
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
COLORREF 
STDCALL
GetDCPenColor(
	HDC hdc
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GetFontUnicodeRanges(
	HDC hdc,
	LPGLYPHSET lpgs
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG 
STDCALL
GetEUDCTimeStamp(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GetEUDCTimeStampExW(LPCWSTR str)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG 
STDCALL
GetFontAssocStatus(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HFONT 
STDCALL
GetHFONT(HDC dc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GetLayout(
	HDC hdc
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetTextExtentExPointWPri(HDC hdc,LPWSTR lpwsz,ULONG cwc,ULONG dxMax,ULONG *pcCh,PULONG pdxOut,LPSIZE psize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int 
STDCALL
GetTextFaceAliasW(HDC hdc,int cChar,LPWSTR pszOut)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GetTransform(HDC hdc, DWORD iXform, LPXFORM pxf)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
LONG 
STDCALL
HT_Get8BPPFormatPalette(LPPALETTEENTRY pPaletteEntry, USHORT RedGamma,USHORT GreenGamma, USHORT BlueGamma)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
LONG 
STDCALL
HT_Get8BPPMaskPalette(LPPALETTEENTRY pPaletteEntry, BOOL Use8BPPMaskPal,BYTE CMYMask, USHORT RedGamma, USHORT GreenGamma, USHORT BlueGamma)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
MirrorRgn(HWND hwnd,HRGN hrgn)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int 
STDCALL
NamedEscape(HDC hdc,PWCHAR pDriver,int nDriver,int iEsc,int cjIn,LPSTR pjIn,int cjOut,LPSTR pjOut)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
QueryFontAssocStatus(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
RemoveFontMemResourceEx(
	HANDLE fh
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
RemoveFontResourceExA(
	LPCSTR lpFileName,
	DWORD fl,
	PVOID pdv
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
RemoveFontResourceExW(
	LPCWSTR lpFileName,
	DWORD fl,
	PVOID pdv
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int 
STDCALL
RemoveFontResourceTracking(LPCSTR lpString,int unknown)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBITMAP 
STDCALL
SetBitmapAttributes(HBITMAP hbm, DWORD dwFlags)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBRUSH 
STDCALL
SetBrushAttributes(HBRUSH hbm, DWORD dwFlags)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
COLORREF 
STDCALL
SetDCBrushColor(
	HDC hdc,
	COLORREF crColor
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
COLORREF 
STDCALL
SetDCPenColor(
	HDC hdc,
	COLORREF crColor
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
SetLayout(
	HDC hdc,
	DWORD dwLayout
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
SetLayoutWidth(HDC hdc,LONG wox,DWORD dwLayout)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
SetMagicColors(HDC hdc,PALETTEENTRY peMagic,ULONG Index)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetVirtualResolution(HDC hdc, int cxVirtualDevicePixel,int cyVirtualDevicePixel,int cxVirtualDeviceMm, int cyVirtualDeviceMm)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int 
STDCALL
StartFormPage(HDC hdc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID 
STDCALL
UnloadNetworkFonts(DWORD unknown)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
bInitSystemAndFontsDirectoriesW(LPWSTR *SystemDir,LPWSTR *FontsDir)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
bMakePathNameW(LPWSTR lpBuffer,LPCWSTR lpFileName,LPWSTR *lpFilePart,DWORD unknown)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HFONT 
STDCALL
CreateFontIndirectExA(const ENUMLOGFONTEXDVA *elfexd)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GetGlyphIndicesA(
	HDC hdc,
	LPCSTR lpstr,
	int c,
	LPWORD pgi,
	DWORD fl
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
UINT 
STDCALL
GetStringBitmapA(HDC hdc,LPSTR psz,BOOL unknown,UINT cj,BYTE *lpSB)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GetTextExtentExPointI(
	HDC hdc,
	LPWORD pgiIn,
	int cgi,
	int nMaxExtent,
	LPINT lpnFit,
	LPINT alpDx,
	LPSIZE lpSize
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HFONT
STDCALL
CreateFontIndirectExW(const ENUMLOGFONTEXDVW *elfexd)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GetGlyphIndicesW(
	HDC hdc,
	LPCWSTR lpstr,
	int c,
	LPWORD pgi,
	DWORD fl
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
UINT 
STDCALL
GetStringBitmapW(HDC hdc,LPWSTR pwsz,BOOL unknown,UINT cj,BYTE *lpSB)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GetTextExtentPointI(
	HDC hdc,
	LPWORD pgiIn,
	int cgi,
	LPSIZE lpSize
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
GdiFullscreenControl(FULLSCREENCONTROL FullscreenCommand,PVOID FullscreenInput,
					DWORD FullscreenInputLength,PVOID FullscreenOutput,
					PULONG FullscreenOutputLength)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
INT 
STDCALL
GdiQueryFonts(PUNIVERSAL_FONT_ID pufiFontList,ULONG nBufferSize,PLARGE_INTEGER pTimeStamp )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GdiRealizationInfo(HDC hdc, PREALIZATION_INFO pri)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GetCharWidthInfo(HDC hdc,PCHWIDTHINFO pChWidthInfo)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GetETM(HDC hdc,EXTTEXTMETRIC *petm)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiAddGlsRecord(HDC hdc,DWORD unknown1,LPCSTR unknown2,LPRECT unknown3)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
GdiConvertMetaFilePict(HGLOBAL hMem)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DEVMODEW *
STDCALL
GdiConvertToDevmodeW(DEVMODEA *dm)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HENHMETAFILE
STDCALL
GdiCreateLocalEnhMetaFile(HENHMETAFILE hmo)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
METAFILEPICT *
STDCALL
GdiCreateLocalMetaFilePict(HENHMETAFILE hmo)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GdiGetCharDimensions(HDC hdc,LPTEXTMETRICW lptm,BOOL unk)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
PSHAREDHANDLETABLE
STDCALL
GdiQueryTable(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HANDLE 
STDCALL
GdiGetSpoolFileHandle(
	LPWSTR		pwszPrinterName,
	LPDEVMODEW	pDevmode,
	LPWSTR		pwszDocName)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiDeleteSpoolFileHandle(
	HANDLE	SpoolFileHandle)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GdiGetPageCount(
	HANDLE	SpoolFileHandle)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HDC
STDCALL
GdiGetDC(
	HANDLE	SpoolFileHandle)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HANDLE 
STDCALL
GdiGetPageHandle(
	HANDLE	SpoolFileHandle,
	DWORD	Page,
	LPDWORD	pdwPageType)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiStartDocEMF(
	HANDLE		SpoolFileHandle,
	DOCINFOW	*pDocInfo)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiStartPageEMF(
	HANDLE	SpoolFileHandle)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiPlayPageEMF(
	HANDLE	SpoolFileHandle,
	HANDLE	hemf,
	RECT	*prectDocument,
	RECT	*prectBorder,
	RECT	*prectClip)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiEndPageEMF(
	HANDLE	SpoolFileHandle,
	DWORD	dwOptimization)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiEndDocEMF(
	HANDLE	SpoolFileHandle)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiGetDevmodeForPage(
	HANDLE		SpoolFileHandle,
	DWORD		dwPageNumber,
	PDEVMODEW	*pCurrDM,
	PDEVMODEW	*pLastDM)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiResetDCEMF(
	HANDLE		SpoolFileHandle,
	PDEVMODEW	pCurrDM)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HANDLE STDCALL
BRUSHOBJ_hGetColorTransform(BRUSHOBJ *pbo)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
PVOID STDCALL
BRUSHOBJ_pvAllocRbrush(IN PBRUSHOBJ BrushObj,
		       IN ULONG ObjSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
PVOID STDCALL
BRUSHOBJ_pvGetRbrush(IN PBRUSHOBJ BrushObj)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG STDCALL
BRUSHOBJ_ulGetBrushColor(BRUSHOBJ *pbo)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
CLIPOBJ_bEnum(IN PCLIPOBJ ClipObj,
	      IN ULONG ObjSize,
	      OUT ULONG *EnumRects)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG STDCALL
CLIPOBJ_cEnumStart(IN PCLIPOBJ ClipObj,
		   IN BOOL ShouldDoAll,
		   IN ULONG ClipType,
		   IN ULONG BuildOrder,
		   IN ULONG MaxRects)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
PPATHOBJ STDCALL
CLIPOBJ_ppoGetPath(PCLIPOBJ ClipObj)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
EngAcquireSemaphore ( IN HSEMAPHORE hsem )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngAlphaBlend(SURFOBJ *psoDest,SURFOBJ *psoSrc,CLIPOBJ *pco,XLATEOBJ *pxlo,RECTL *prclDest,RECTL *prclSrc,BLENDOBJ *pBlendObj)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngAssociateSurface(IN HSURF Surface,
		    IN HDEV Dev,
		    IN ULONG Hooks)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngBitBlt(SURFOBJ *Dest,
	  SURFOBJ *Source,
	  SURFOBJ *Mask,
	  CLIPOBJ *ClipRegion,
	  XLATEOBJ *ColorTranslation,
	  RECTL *DestRect,
	  POINTL *SourcePoint,
	  POINTL *MaskRect,
	  BRUSHOBJ *Brush,
	  POINTL *BrushOrigin,
	  ROP4 rop4)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngCheckAbort(SURFOBJ *pso)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
FD_GLYPHSET* STDCALL
EngComputeGlyphSet(INT nCodePage,INT nFirstChar,INT cChars)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngCopyBits(SURFOBJ *Dest,
	    SURFOBJ *Source,
	    CLIPOBJ *Clip,
	    XLATEOBJ *ColorTranslation,
	    RECTL *DestRect,
	    POINTL *SourcePoint)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBITMAP STDCALL
EngCreateBitmap(IN SIZEL Size,
		IN LONG Width,
		IN ULONG Format,
		IN ULONG Flags,
		IN PVOID Bits)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
PCLIPOBJ STDCALL
EngCreateClip(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBITMAP STDCALL
EngCreateDeviceBitmap(IN DHSURF Surface,
		      IN SIZEL Size,
		      IN ULONG Format)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HSURF STDCALL
EngCreateDeviceSurface(IN DHSURF Surface,
		       IN SIZEL Size,
		       IN ULONG FormatVersion)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HPALETTE STDCALL
EngCreatePalette(IN ULONG Mode,
		 IN ULONG NumColors,
		 IN ULONG *Colors,
		 IN ULONG Red,
		 IN ULONG Green,
		 IN ULONG Blue)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HSEMAPHORE
STDCALL
EngCreateSemaphore ( VOID )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL
EngDeleteClip(CLIPOBJ *ClipRegion)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngDeletePalette(IN HPALETTE Palette)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL
EngDeletePath(PATHOBJ *ppo)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
VOID
STDCALL
EngDeleteSemaphore ( IN HSEMAPHORE hsem )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngDeleteSurface(IN HSURF Surface)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngEraseSurface(SURFOBJ *Surface,
		RECTL *Rect,
		ULONG iColor)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngFillPath(SURFOBJ *pso,PATHOBJ *ppo,CLIPOBJ *pco,BRUSHOBJ *pbo,POINTL *pptlBrushOrg,MIX mix,FLONG flOptions)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
PVOID STDCALL
EngFindResource(HANDLE h,int iName,int iType,PULONG pulSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL 
EngFreeModule(HANDLE h)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
VOID STDCALL
EngGetCurrentCodePage(OUT PUSHORT OemCodePage,
		      OUT PUSHORT AnsiCodePage)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
LPWSTR STDCALL
EngGetDriverName(HDEV hdev)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
LPWSTR STDCALL
EngGetPrinterDataFileName(HDEV hdev)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngGradientFill(SURFOBJ *psoDest,CLIPOBJ *pco,XLATEOBJ *pxlo,TRIVERTEX *pVertex,ULONG nVertex,PVOID pMesh,ULONG nMesh,RECTL *prclExtents,POINTL *pptlDitherOrg,ULONG ulMode)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngLineTo(SURFOBJ *Surface,
	  CLIPOBJ *Clip,
	  BRUSHOBJ *Brush,
	  LONG x1,
	  LONG y1,
	  LONG x2,
	  LONG y2,
	  RECTL *RectBounds,
	  MIX mix)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HANDLE STDCALL 
EngLoadModule(LPWSTR pwsz)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
SURFOBJ * STDCALL
EngLockSurface(IN HSURF Surface)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngMarkBandingSurface(HSURF hsurf)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL
EngMultiByteToUnicodeN(OUT LPWSTR UnicodeString,
		       IN ULONG MaxBytesInUnicodeString,
		       OUT PULONG BytesInUnicodeString,
		       IN PCHAR MultiByteString,
		       IN ULONG BytesInMultiByteString)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
INT STDCALL 
EngMultiByteToWideChar(UINT CodePage,LPWSTR WideCharString,INT BytesInWideCharString,LPSTR MultiByteString,INT BytesInMultiByteString)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngPaint(IN SURFOBJ *Surface,
	 IN CLIPOBJ *ClipRegion,
	 IN BRUSHOBJ *Brush,
	 IN POINTL *BrushOrigin,
	 IN MIX  Mix)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngPlgBlt(SURFOBJ *psoTrg,SURFOBJ *psoSrc,SURFOBJ *psoMsk,CLIPOBJ *pco,XLATEOBJ *pxlo,COLORADJUSTMENT *pca,POINTL *pptlBrushOrg,POINTFIX *pptfx,RECTL *prcl,POINTL *pptl,ULONG iMode)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngQueryEMFInfo(HDEV hdev,EMFINFO *pEMFInfo)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL 
EngQueryLocalTime(PENG_TIME_FIELDS etf)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
VOID
STDCALL
EngReleaseSemaphore ( IN HSEMAPHORE hsem )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngStretchBlt(SURFOBJ *psoDest,SURFOBJ *psoSrc,SURFOBJ *psoMask,CLIPOBJ *pco,XLATEOBJ *pxlo,COLORADJUSTMENT *pca,POINTL *pptlHTOrg,RECTL *prclDest,RECTL *prclSrc,POINTL *pptlMask,ULONG iMode)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngStretchBltROP(SURFOBJ *psoDest,SURFOBJ *psoSrc,SURFOBJ *psoMask,CLIPOBJ *pco,XLATEOBJ *pxlo,COLORADJUSTMENT *pca,POINTL *pptlHTOrg,RECTL *prclDest,RECTL *prclSrc,POINTL *pptlMask,ULONG iMode,BRUSHOBJ *pbo,DWORD rop4)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngStrokeAndFillPath(SURFOBJ *pso,PATHOBJ *ppo,CLIPOBJ *pco,XFORMOBJ *pxo,BRUSHOBJ *pboStroke,LINEATTRS *plineattrs,BRUSHOBJ *pboFill,POINTL *pptlBrushOrg,MIX mixFill,FLONG flOptions)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngStrokePath(SURFOBJ *pso,PATHOBJ *ppo,CLIPOBJ *pco,XFORMOBJ *pxo,BRUSHOBJ *pbo,POINTL *pptlBrushOrg,LINEATTRS *plineattrs,MIX mix)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngTextOut(SURFOBJ *pso,STROBJ *pstro,FONTOBJ *pfo,CLIPOBJ *pco,RECTL *prclExtra,RECTL *prclOpaque,BRUSHOBJ *pboFore,BRUSHOBJ *pboOpaque,POINTL *pptlOrg,MIX mix)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngTransparentBlt(IN PSURFOBJ Dest,
		  IN PSURFOBJ Source,
		  IN PCLIPOBJ Clip,
		  IN PXLATEOBJ ColorTranslation,
		  IN PRECTL DestRect,
		  IN PRECTL SourceRect,
		  IN ULONG TransparentColor,
		  IN ULONG Reserved)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL
EngUnicodeToMultiByteN(OUT PCHAR MultiByteString,
		       IN ULONG  MaxBytesInMultiByteString,
		       OUT PULONG  BytesInMultiByteString,
		       IN PWSTR  UnicodeString,
		       IN ULONG  BytesInUnicodeString)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
VOID STDCALL 
EngUnlockSurface(SURFOBJ *pso)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
INT STDCALL 
EngWideCharToMultiByte(UINT CodePage,LPWSTR WideCharString,INT BytesInWideCharString,LPSTR MultiByteString,INT BytesInMultiByteString)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG
STDCALL
FONTOBJ_cGetAllGlyphHandles(IN PFONTOBJ  FontObj,
                            IN HGLYPH  *Glyphs)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG
STDCALL
FONTOBJ_cGetGlyphs(IN PFONTOBJ FontObj,
                   IN ULONG    Mode,
                   IN ULONG    NumGlyphs,
                   IN HGLYPH  *GlyphHandles,
                   IN PVOID   *OutGlyphs)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
PFD_GLYPHATTR STDCALL
FONTOBJ_pQueryGlyphAttrs(FONTOBJ *pfo,ULONG iMode)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
FD_GLYPHSET *STDCALL
FONTOBJ_pfdg(FONTOBJ *pfo)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
IFIMETRICS*
STDCALL
FONTOBJ_pifi(IN PFONTOBJ  FontObj)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
PVOID
STDCALL
FONTOBJ_pvTrueTypeFontFile(IN PFONTOBJ  FontObj,
                           IN ULONG    *FileSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
XFORMOBJ*
STDCALL
FONTOBJ_pxoGetXform(IN PFONTOBJ  FontObj)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
FONTOBJ_vGetInfo(IN  PFONTOBJ   FontObj,
                 IN  ULONG      InfoSize,
                 OUT PFONTINFO  FontInfo)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL
PATHOBJ_bEnum(PATHOBJ *ppo,PATHDATA *ppd)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL 
PATHOBJ_bEnumClipLines(PATHOBJ *ppo,ULONG cb,CLIPLINE *pcl)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL 
PATHOBJ_vEnumStart(PATHOBJ *ppo)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
VOID STDCALL
PATHOBJ_vEnumStartClipLines(PATHOBJ *ppo,CLIPOBJ *pco,SURFOBJ *pso,LINEATTRS *pla)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
VOID STDCALL
PATHOBJ_vGetBounds(PATHOBJ *ppo,PRECTFX prectfx)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL
STROBJ_bEnum(STROBJ *pstro,ULONG *pc,PGLYPHPOS *ppgpos)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
STROBJ_bEnumPositionsOnly(STROBJ *pstro,ULONG *pc,PGLYPHPOS *ppgpos)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
STROBJ_bGetAdvanceWidths(STROBJ *pso,ULONG iFirst,ULONG c,POINTQF *pptqD)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD STDCALL
STROBJ_dwGetCodePage(STROBJ  *pstro)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL
STROBJ_vEnumStart(STROBJ *pstro)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL
XFORMOBJ_bApplyXform(XFORMOBJ *pxo,ULONG iMode,ULONG cPoints,PVOID pvIn,PVOID pvOut)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG STDCALL
XFORMOBJ_iGetXform(XFORMOBJ *pxo,XFORML *pxform)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG STDCALL
XLATEOBJ_cGetPalette(XLATEOBJ *XlateObj,
		     ULONG PalOutType,
		     ULONG cPal,
		     ULONG *OutPal)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HANDLE STDCALL
XLATEOBJ_hGetColorTransform(XLATEOBJ *pxlo)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG STDCALL
XLATEOBJ_iXlate(XLATEOBJ *XlateObj,
		ULONG Color)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG * STDCALL
XLATEOBJ_piVector(XLATEOBJ *XlateObj)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL DdCreateDirectDrawObject( 
LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
HDC hdc
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL DdQueryDirectDrawObject( 
LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
LPDDHALINFO pHalInfo,
LPDDHAL_DDCALLBACKS pDDCallbacks,
LPDDHAL_DDSURFACECALLBACKS pDDSurfaceCallbacks,
LPDDHAL_DDPALETTECALLBACKS pDDPaletteCallbacks,
LPD3DHAL_CALLBACKS pD3dCallbacks,
LPD3DHAL_GLOBALDRIVERDATA pD3dDriverData,
LPDDHAL_DDEXEBUFCALLBACKS pD3dBufferCallbacks,
LPDDSURFACEDESC pD3dTextureFormats,
LPDWORD pdwFourCC,
LPVIDMEM pvmList
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL DdDeleteDirectDrawObject( 
LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL DdCreateSurfaceObject( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
BOOL bPrimarySurface
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL DdDeleteSurfaceObject( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL DdResetVisrgn( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
HWND hWnd
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL DdGetDC( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
LPPALETTEENTRY pColorTable
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL DdReleaseDC( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBITMAP STDCALL DdCreateDIBSection( 
HDC hdc,
CONST BITMAPINFO *pbmi,
UINT iUsage,
VOID **ppvBits,
HANDLE hSectionApp,
DWORD dwOffset
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL DdReenableDirectDrawObject( 
LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
BOOL *pbNewMode
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL DdAttachSurface( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceFrom,
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceTo
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL DdUnattachSurface( 
LPDDRAWI_DDRAWSURFACE_LCL pSurface,
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceAttached
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
ULONG STDCALL DdQueryDisplaySettingsUniqueness(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HANDLE STDCALL DdGetDxHandle( 
LPDDRAWI_DIRECTDRAW_LCL pDDraw,
LPDDRAWI_DDRAWSURFACE_LCL pSurface,
BOOL bRelease
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL DdSetGammaRamp( 
LPDDRAWI_DIRECTDRAW_LCL pDDraw,
HDC hdc,
LPVOID lpGammaRamp
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD STDCALL DdSwapTextureHandles( 
LPDDRAWI_DIRECTDRAW_LCL pDDraw,
LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl1,
LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl2
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GdiPlayEMF
(
	LPWSTR     pwszPrinterName,
	LPDEVMODEW pDevmode,
	LPWSTR     pwszDocName,
	EMFPLAYPROC pfnEMFPlayFn,
	HANDLE     hPageQuery
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiInitSpool(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiPlayPrivatePageEMF
(
	HANDLE	SpoolFileHandle,
	DWORD	unknown,
	RECT	*prectDocument
)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL GdiInitializeLanguagePack(DWORD InitParam)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}
