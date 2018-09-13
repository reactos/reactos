/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    font.h

Abstract:

    This module contains the data structures, data types,
    and procedures related to fonts.

Author:

    Therese Stowell (thereses) 15-Jan-1991

Revision History:

--*/

#ifndef FONT_H
#define FONT_H

#define INITIAL_FONTS 20
#define FONT_INCREMENT 3
#define CONSOLE_MAX_FONT_NAME_LENGTH 256

#define EF_NEW         0x0001 // a newly available face
#define EF_OLD         0x0002 // a previously available face
#define EF_ENUMERATED  0x0004 // all sizes have been enumerated
#define EF_OEMFONT     0x0008 // an OEM face
#define EF_TTFONT      0x0010 // a TT face
#define EF_DEFFACE     0x0020 // the default face
#if defined(FE_SB)
#define EF_DBCSFONT    0x0040 // the DBCS font
#endif

/*
 * FONT_INFO
 *
 * The distinction between the desired and actual font dimensions obtained
 * is important in the case of TrueType fonts, in which there is no guarantee
 * that what you ask for is what you will get.
 *
 * Note that the correspondence between "Desired" and "Actual" is broken
 * whenever the user changes his display driver, because GDI uses driver
 * parameters to control the font rasterization.
 *
 * The SizeDesired is {0, 0} if the font is a raster font.
 */
typedef struct _FONT_INFO {
    HFONT hFont;
    COORD Size;      // font size obtained
    COORD SizeWant;  // 0;0 if Raster font
    LONG  Weight;
    LPTSTR FaceName;
    BYTE  Family;
#if defined(FE_SB)
    BYTE  tmCharSet;
#endif
} FONT_INFO, *PFONT_INFO;

typedef struct tagFACENODE {
     struct tagFACENODE *pNext;
     DWORD  dwFlag;
     TCHAR  atch[];
} FACENODE, *PFACENODE;

#define TM_IS_TT_FONT(x)     (((x) & TMPF_TRUETYPE) == TMPF_TRUETYPE)
#define IS_BOLD(w)           ((w) >= FW_SEMIBOLD)
#define SIZE_EQUAL(s1, s2)   (((s1).X == (s2).X) && ((s1).Y == (s2).Y))
#define POINTS_PER_INCH 72
#define MIN_PIXEL_HEIGHT 5
#define MAX_PIXEL_HEIGHT 72


//
// Function prototypes
//

VOID
InitializeFonts(VOID);

VOID
DestroyFonts(VOID);

NTSTATUS
EnumerateFonts(DWORD Flags);

#if !defined(FE_SB)
int
FindCreateFont(
    DWORD Family,
    LPTSTR ptszFace,
    COORD Size,
    LONG Weight);
#else
int
FindCreateFont(
    DWORD Family,
    LPTSTR ptszFace,
    COORD Size,
    LONG Weight,
    UINT CodePage);
#endif

BOOL
DoFontEnum(
    HDC hDC,
    LPTSTR ptszFace,
    PSHORT pTTPoints,
    UINT nTTPoints);

#endif /* !FONT_H */
