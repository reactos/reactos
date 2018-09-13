/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WGFONT.H
 *  WOW32 16-bit GDI API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


/* Enumeration handler data
 */
typedef struct _FNTDATA {	/* fntdata */
    VPPROC  vpfnEnumFntProc;    // 16-bit enumeration function
    DWORD   dwUserFntParam;	// user param, if any
    HMEM16  hLogFont;		//
    VPVOID  vpLogFont;		// 16-bit storage for logical font
    HMEM16  hTextMetric;	//
    VPVOID  vpTextMetric;	// 16-bit storage for textmetric structure
    VPVOID  vpFaceName;     // 16bit far ptr - input to Enum Fonts & Families
} FNTDATA, *PFNTDATA;


/* Function prototypes
 */
ULONG FASTCALL WG32AddFontResource(PVDMFRAME pFrame);
ULONG FASTCALL WG32CreateFont(PVDMFRAME pFrame);
ULONG FASTCALL WG32CreateFontIndirect(PVDMFRAME pFrame);

INT	W32FontFunc(LPLOGFONT pLogFont,
		    LPTEXTMETRIC pTextMetrics, INT nFontType, PFNTDATA pFntData);

ULONG FASTCALL WG32EnumFonts(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetAspectRatioFilter(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetCharWidth(PVDMFRAME pFrame);
ULONG FASTCALL WG32RemoveFontResource(PVDMFRAME pFrame);

ULONG W32EnumFontHandler( PVDMFRAME pFrame, BOOL fEnumFontFamilies );

