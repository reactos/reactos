#ifndef __WIN32K_COLOR_H
#define __WIN32K_COLOR_H

#ifndef CLR_INVALID
#define CLR_INVALID         0xffffffff
#endif
#define PC_SYS_USED     0x80		// palentry is used (both system and logical)
#define PC_SYS_RESERVED 0x40		// system palentry is not to be mapped to
#define PC_SYS_MAPPED   0x10		// logical palentry is a direct alias for system palentry

BOOL
STDCALL
W32kAnimatePalette (
	HPALETTE		hpal,
	UINT			StartIndex,
	UINT			Entries,
	CONST PPALETTEENTRY	ppe
	);
HPALETTE
STDCALL
W32kCreateHalftonePalette (
	HDC	hDC
	);
HPALETTE
STDCALL
W32kCreatePalette (
	CONST PLOGPALETTE	lgpl
	);
BOOL
STDCALL
W32kGetColorAdjustment (
	HDC			hDC,
	LPCOLORADJUSTMENT	ca
	);
COLORREF
STDCALL
W32kGetNearestColor (
	HDC		hDC,
	COLORREF	Color
	);
UINT
STDCALL
W32kGetNearestPaletteIndex (
	HPALETTE	hpal,
	COLORREF	Color
	);
UINT
STDCALL
W32kGetPaletteEntries (
	HPALETTE	hpal,
	UINT		StartIndex,
	UINT		Entries,
	LPPALETTEENTRY	pe
	);
UINT
STDCALL
W32kGetSystemPaletteEntries (
	HDC		hDC,
	UINT		StartIndex,
	UINT		Entries,
	LPPALETTEENTRY	pe
	);
UINT
STDCALL
W32kGetSystemPaletteUse (
	HDC	hDC
	);
UINT
STDCALL
W32kRealizePalette (
	HDC	hDC
	);
BOOL
STDCALL
W32kResizePalette (
	HPALETTE	hpal,
	UINT		Entries
	);
HPALETTE
STDCALL
W32kSelectPalette (
	HDC		hDC,
	HPALETTE	hpal,
	BOOL		ForceBackground
	);
BOOL
STDCALL
W32kSetColorAdjustment (
	HDC			hDC,
	CONST LPCOLORADJUSTMENT	ca
	);
UINT
STDCALL
W32kSetPaletteEntries (
	HPALETTE		hpal,
	UINT			Start,
	UINT			Entries,
	CONST LPPALETTEENTRY	pe
	);
UINT
STDCALL
W32kSetSystemPaletteUse (
	HDC	hDC,
	UINT	Usage
	);
BOOL
STDCALL
W32kUnrealizeObject (
	HGDIOBJ	hgdiobj
	);
BOOL
STDCALL
W32kUpdateColors (
	HDC	hDC
	);
#endif
