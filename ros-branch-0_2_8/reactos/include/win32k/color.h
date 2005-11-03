#ifndef __WIN32K_COLOR_H
#define __WIN32K_COLOR_H

#ifndef CLR_INVALID
#define CLR_INVALID         0xffffffff
#endif
#define PC_SYS_USED     0x80		/* palentry is used (both system and logical) */
#define PC_SYS_RESERVED 0x40		/* system palentry is not to be mapped to */
#define PC_SYS_MAPPED   0x10		/* logical palentry is a direct alias for system palentry */

BOOL
STDCALL
NtGdiAnimatePalette (
	HPALETTE		hpal,
	UINT			StartIndex,
	UINT			Entries,
	CONST PPALETTEENTRY	ppe
	);
HPALETTE
STDCALL
NtGdiCreateHalftonePalette (
	HDC	hDC
	);
HPALETTE
STDCALL
NtGdiCreatePalette (
	CONST PLOGPALETTE	lgpl
	);
BOOL
STDCALL
NtGdiGetColorAdjustment (
	HDC			hDC,
	LPCOLORADJUSTMENT	ca
	);
COLORREF
STDCALL
NtGdiGetNearestColor (
	HDC		hDC,
	COLORREF	Color
	);
UINT
STDCALL
NtGdiGetNearestPaletteIndex (
	HPALETTE	hpal,
	COLORREF	Color
	);
UINT
STDCALL
NtGdiGetPaletteEntries (
	HPALETTE	hpal,
	UINT		StartIndex,
	UINT		Entries,
	LPPALETTEENTRY	pe
	);
UINT
STDCALL
NtGdiGetSystemPaletteEntries (
	HDC		hDC,
	UINT		StartIndex,
	UINT		Entries,
	LPPALETTEENTRY	pe
	);
UINT
STDCALL
NtGdiGetSystemPaletteUse (
	HDC	hDC
	);
UINT
STDCALL
NtGdiRealizePalette (
	HDC	hDC
	);
BOOL
STDCALL
NtGdiResizePalette (
	HPALETTE	hpal,
	UINT		Entries
	);
HPALETTE
STDCALL
NtGdiSelectPalette (
	HDC		hDC,
	HPALETTE	hpal,
	BOOL		ForceBackground
	);
BOOL
STDCALL
NtGdiSetColorAdjustment (
	HDC			hDC,
	CONST LPCOLORADJUSTMENT	ca
	);
UINT
STDCALL
NtGdiSetPaletteEntries (
	HPALETTE		hpal,
	UINT			Start,
	UINT			Entries,
	CONST LPPALETTEENTRY	pe
	);
UINT
STDCALL
NtGdiSetSystemPaletteUse (
	HDC	hDC,
	UINT	Usage
	);
BOOL
STDCALL
NtGdiUnrealizeObject (
	HGDIOBJ	hgdiobj
	);
BOOL
STDCALL
NtGdiUpdateColors (
	HDC	hDC
	);
#endif
