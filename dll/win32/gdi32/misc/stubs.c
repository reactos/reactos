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
STDCALL
CancelDC(
	HDC	a0
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	GOBJENUMPROC	a2,
	LPARAM		a3
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
STDCALL
Escape(HDC hdc, INT escape, INT in_count, LPCSTR in_data, LPVOID out_data)
{
    return NtGdiEscape(hdc,escape,in_count,in_data,out_data);
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetSystemPaletteUse(HDC hDc)
{
    return NtGdiGetSystemPaletteUse(hDc);
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
HENHMETAFILE
STDCALL
CloseEnhMetaFile(
	HDC	hdc
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
STDCALL
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
STDCALL
EnumEnhMetaFile(
	HDC		a0,
	HENHMETAFILE	a1,
	ENHMFENUMPROC	a2,
	LPVOID		a3,
	CONST RECT	*a4
	)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
STDCALL
GdiComment(
	HDC		a0,
	UINT		a1,
	CONST BYTE	*a2
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @implemented
 */
BOOL
STDCALL
UnrealizeObject(
	HGDIOBJ	a0
	)
{
	return NtGdiUnrealizeObject(a0);
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GdiFlush()
{
        /*
         * Although GdiFlush is unimplemented, it's safe to return
         * TRUE, because we don't have GDI engine surface caching
         * implemented yet.
         */
	return TRUE;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
HCOLORSPACE
STDCALL
GetColorSpace(
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
HCOLORSPACE
STDCALL
SetColorSpace(
	HDC		a0,
	HCOLORSPACE	a1
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
STDCALL
DeleteColorSpace(
	HCOLORSPACE	a0
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
STDCALL
GetDeviceGammaRamp(
	HDC	a0,
	LPVOID	a1
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
STDCALL
SetDeviceGammaRamp(
	HDC	a0,
	LPVOID	a1
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
STDCALL
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
STDCALL
wglCopyContext(
	HGLRC	a0,
	HGLRC	a1,
	UINT	a2
	)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
STDCALL
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
STDCALL
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
INT
STDCALL
GetRandomRgn(
	HDC	a0,
	HRGN	a1,
	INT	a2
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
STDCALL
SelectBrushLocal(
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
STDCALL
SelectFontLocal(
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
STDCALL
SetFontEnumeration(
	DWORD	a0
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
STDCALL
SetRelAbs(
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiSetPixelFormat(HDC hdc,int ipfd)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
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
STDCALL
GetBrushAttributes(HBRUSH hbr)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @implemented
 */
DEVMODEW *
STDCALL
GdiConvertToDevmodeW(DEVMODEA *dm)
{
  LPDEVMODEW dmw;
  
  dmw = HEAP_alloc(sizeof(DEVMODEW));
#define COPYS(f,len) MultiByteToWideChar ( CP_THREAD_ACP, 0, (LPSTR)dm->f, len, dmw->f, len )
#define COPYN(f) dmw->f = dm->f
  COPYS(dmDeviceName, CCHDEVICENAME );
  COPYN(dmSpecVersion);
  COPYN(dmDriverVersion);
  switch ( dm->dmSize )
    {
    case SIZEOF_DEVMODEA_300:
      dmw->dmSize = SIZEOF_DEVMODEW_300;
      break;
    case SIZEOF_DEVMODEA_400:
      dmw->dmSize = SIZEOF_DEVMODEW_400;
      break;
    case SIZEOF_DEVMODEA_500:
    default: /* FIXME what to do??? */
      dmw->dmSize = SIZEOF_DEVMODEW_500;
      break;
    }
  COPYN(dmDriverExtra);
  COPYN(dmFields);
  COPYN(dmPosition.x);
  COPYN(dmPosition.y);
  COPYN(dmScale);
  COPYN(dmCopies);
  COPYN(dmDefaultSource);
  COPYN(dmPrintQuality);
  COPYN(dmColor);
  COPYN(dmDuplex);
  COPYN(dmYResolution);
  COPYN(dmTTOption);
  COPYN(dmCollate);
  COPYS(dmFormName,CCHFORMNAME);
  COPYN(dmLogPixels);
  COPYN(dmBitsPerPel);
  COPYN(dmPelsWidth);
  COPYN(dmPelsHeight);
  COPYN(dmDisplayFlags); // aka dmNup
  COPYN(dmDisplayFrequency);

  if ( dm->dmSize <= SIZEOF_DEVMODEA_300 )
    return dmw; // we're done with 0x300 fields

  COPYN(dmICMMethod);
  COPYN(dmICMIntent);
  COPYN(dmMediaType);
  COPYN(dmDitherType);
  COPYN(dmReserved1);
  COPYN(dmReserved2);

  if ( dm->dmSize <= SIZEOF_DEVMODEA_400 )
    return dmw; // we're done with 0x400 fields

  COPYN(dmPanningWidth);
  COPYN(dmPanningHeight);

  return dmw;

#undef COPYN
#undef COPYS
}

/*
 * @unimplemented
 */
HENHMETAFILE
STDCALL
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
STDCALL
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
STDCALL
GdiGetSpoolFileHandle(
	LPWSTR		pwszPrinterName,
	LPDEVMODEW	pDevmode,
	LPWSTR		pwszDocName)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HANDLE STDCALL
BRUSHOBJ_hGetColorTransform(BRUSHOBJ *pbo)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
PVOID STDCALL
BRUSHOBJ_pvAllocRbrush(IN BRUSHOBJ *BrushObj,
		       IN ULONG ObjSize)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
PVOID STDCALL
BRUSHOBJ_pvGetRbrush(IN BRUSHOBJ *BrushObj)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG STDCALL
BRUSHOBJ_ulGetBrushColor(BRUSHOBJ *pbo)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
CLIPOBJ_bEnum(IN CLIPOBJ *ClipObj,
	      IN ULONG ObjSize,
	      OUT ULONG *EnumRects)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG STDCALL
CLIPOBJ_cEnumStart(IN CLIPOBJ *ClipObj,
		   IN BOOL ShouldDoAll,
		   IN ULONG ClipType,
		   IN ULONG BuildOrder,
		   IN ULONG MaxRects)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
PATHOBJ* STDCALL
CLIPOBJ_ppoGetPath(CLIPOBJ *ClipObj)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngAlphaBlend(SURFOBJ *psoDest,SURFOBJ *psoSrc,CLIPOBJ *pco,XLATEOBJ *pxlo,RECTL *prclDest,RECTL *prclSrc,BLENDOBJ *pBlendObj)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngCheckAbort(SURFOBJ *pso)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
FD_GLYPHSET* STDCALL
EngComputeGlyphSet(INT nCodePage,INT nFirstChar,INT cChars)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
CLIPOBJ* STDCALL
EngCreateClip(VOID)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL
EngDeleteClip(CLIPOBJ *ClipRegion)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngDeletePalette(IN HPALETTE Palette)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL
EngDeletePath(PATHOBJ *ppo)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
VOID
STDCALL
EngDeleteSemaphore ( IN HSEMAPHORE hsem )
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngDeleteSurface(IN HSURF Surface)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngFillPath(SURFOBJ *pso,PATHOBJ *ppo,CLIPOBJ *pco,BRUSHOBJ *pbo,POINTL *pptlBrushOrg,MIX mix,FLONG flOptions)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
PVOID STDCALL
EngFindResource(HANDLE h,int iName,int iType,PULONG pulSize)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL 
EngFreeModule(HANDLE h)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
VOID STDCALL
EngGetCurrentCodePage(OUT PUSHORT OemCodePage,
		      OUT PUSHORT AnsiCodePage)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
LPWSTR STDCALL
EngGetDriverName(HDEV hdev)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
LPWSTR STDCALL
EngGetPrinterDataFileName(HDEV hdev)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngGradientFill(SURFOBJ *psoDest,CLIPOBJ *pco,XLATEOBJ *pxlo,TRIVERTEX *pVertex,ULONG nVertex,PVOID pMesh,ULONG nMesh,RECTL *prclExtents,POINTL *pptlDitherOrg,ULONG ulMode)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HANDLE STDCALL 
EngLoadModule(LPWSTR pwsz)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
SURFOBJ * STDCALL
EngLockSurface(IN HSURF Surface)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngMarkBandingSurface(HSURF hsurf)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
INT STDCALL 
EngMultiByteToWideChar(UINT CodePage,LPWSTR WideCharString,INT BytesInWideCharString,LPSTR MultiByteString,INT BytesInMultiByteString)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngPlgBlt(SURFOBJ *psoTrg,SURFOBJ *psoSrc,SURFOBJ *psoMsk,CLIPOBJ *pco,XLATEOBJ *pxlo,COLORADJUSTMENT *pca,POINTL *pptlBrushOrg,POINTFIX *pptfx,RECTL *prcl,POINTL *pptl,ULONG iMode)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngQueryEMFInfo(HDEV hdev,EMFINFO *pEMFInfo)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL 
EngQueryLocalTime(PENG_TIME_FIELDS etf)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
VOID
STDCALL
EngReleaseSemaphore ( IN HSEMAPHORE hsem )
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngStretchBlt(SURFOBJ *psoDest,SURFOBJ *psoSrc,SURFOBJ *psoMask,CLIPOBJ *pco,XLATEOBJ *pxlo,COLORADJUSTMENT *pca,POINTL *pptlHTOrg,RECTL *prclDest,RECTL *prclSrc,POINTL *pptlMask,ULONG iMode)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngStretchBltROP(SURFOBJ *psoDest,SURFOBJ *psoSrc,SURFOBJ *psoMask,CLIPOBJ *pco,XLATEOBJ *pxlo,COLORADJUSTMENT *pca,POINTL *pptlHTOrg,RECTL *prclDest,RECTL *prclSrc,POINTL *pptlMask,ULONG iMode,BRUSHOBJ *pbo,DWORD rop4)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngStrokeAndFillPath(SURFOBJ *pso,PATHOBJ *ppo,CLIPOBJ *pco,XFORMOBJ *pxo,BRUSHOBJ *pboStroke,LINEATTRS *plineattrs,BRUSHOBJ *pboFill,POINTL *pptlBrushOrg,MIX mixFill,FLONG flOptions)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngStrokePath(SURFOBJ *pso,PATHOBJ *ppo,CLIPOBJ *pco,XFORMOBJ *pxo,BRUSHOBJ *pbo,POINTL *pptlBrushOrg,LINEATTRS *plineattrs,MIX mix)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL 
EngTextOut(SURFOBJ *pso,STROBJ *pstro,FONTOBJ *pfo,CLIPOBJ *pco,RECTL *prclExtra,RECTL *prclOpaque,BRUSHOBJ *pboFore,BRUSHOBJ *pboOpaque,POINTL *pptlOrg,MIX mix)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
EngTransparentBlt(IN SURFOBJ *Dest,
		  IN SURFOBJ *Source,
		  IN CLIPOBJ *Clip,
		  IN XLATEOBJ *ColorTranslation,
		  IN PRECTL DestRect,
		  IN PRECTL SourceRect,
		  IN ULONG TransparentColor,
		  IN ULONG Reserved)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
VOID STDCALL 
EngUnlockSurface(SURFOBJ *pso)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
INT STDCALL 
EngWideCharToMultiByte(UINT CodePage,LPWSTR WideCharString,INT BytesInWideCharString,LPSTR MultiByteString,INT BytesInMultiByteString)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG
STDCALL
FONTOBJ_cGetAllGlyphHandles(IN FONTOBJ *FontObj,
                            IN HGLYPH  *Glyphs)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG
STDCALL
FONTOBJ_cGetGlyphs(IN FONTOBJ *FontObj,
                   IN ULONG    Mode,
                   IN ULONG    NumGlyphs,
                   IN HGLYPH  *GlyphHandles,
                   IN PVOID   *OutGlyphs)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
PFD_GLYPHATTR STDCALL
FONTOBJ_pQueryGlyphAttrs(FONTOBJ *pfo,ULONG iMode)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
FD_GLYPHSET *STDCALL
FONTOBJ_pfdg(FONTOBJ *pfo)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
IFIMETRICS*
STDCALL
FONTOBJ_pifi(IN FONTOBJ  *FontObj)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
PVOID
STDCALL
FONTOBJ_pvTrueTypeFontFile(IN FONTOBJ  *FontObj,
                           IN ULONG    *FileSize)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
XFORMOBJ*
STDCALL
FONTOBJ_pxoGetXform(IN FONTOBJ  *FontObj)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
FONTOBJ_vGetInfo(IN  FONTOBJ   *FontObj,
                 IN  ULONG      InfoSize,
                 OUT PFONTINFO  FontInfo)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL
PATHOBJ_bEnum(PATHOBJ *ppo,PATHDATA *ppd)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL 
PATHOBJ_bEnumClipLines(PATHOBJ *ppo,ULONG cb,CLIPLINE *pcl)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL 
PATHOBJ_vEnumStart(PATHOBJ *ppo)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
VOID STDCALL
PATHOBJ_vEnumStartClipLines(PATHOBJ *ppo,CLIPOBJ *pco,SURFOBJ *pso,LINEATTRS *pla)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
VOID STDCALL
PATHOBJ_vGetBounds(PATHOBJ *ppo,PRECTFX prectfx)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL
STROBJ_bEnum(STROBJ *pstro,ULONG *pc,PGLYPHPOS *ppgpos)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
STROBJ_bEnumPositionsOnly(STROBJ *pstro,ULONG *pc,PGLYPHPOS *ppgpos)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
STROBJ_bGetAdvanceWidths(STROBJ *pso,ULONG iFirst,ULONG c,POINTQF *pptqD)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD STDCALL
STROBJ_dwGetCodePage(STROBJ  *pstro)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL
STROBJ_vEnumStart(STROBJ *pstro)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL
XFORMOBJ_bApplyXform(XFORMOBJ *pxo,ULONG iMode,ULONG cPoints,PVOID pvIn,PVOID pvOut)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG STDCALL
XFORMOBJ_iGetXform(XFORMOBJ *pxo,XFORML *pxform)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HANDLE STDCALL
XLATEOBJ_hGetColorTransform(XLATEOBJ *pxlo)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG * STDCALL
XLATEOBJ_piVector(XLATEOBJ *XlateObj)
{
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL GdiInitializeLanguagePack(DWORD InitParam)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}
