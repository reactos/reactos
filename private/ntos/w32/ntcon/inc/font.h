/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    font.h

Abstract:

    This module contains the data structures, data types,
    and procedures related to fonts.

Author:

    Therese Stowell (thereses) 15-Jan-1991

Revision History:

--*/

#define INITIAL_FONTS 20
#define FONT_INCREMENT 3

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
    LPWSTR FaceName;
    BYTE  Family;
#if defined(FE_SB)
    BYTE tmCharSet;
#endif
} FONT_INFO, *PFONT_INFO;

#define TM_IS_TT_FONT(x)     (((x) & TMPF_TRUETYPE) == TMPF_TRUETYPE)
#define IS_BOLD(w)           ((w) >= FW_SEMIBOLD)
#define SIZE_EQUAL(s1, s2)   (((s1).X == (s2).X) && ((s1).Y == (s2).Y))
#define POINTS_PER_INCH 72
#define MIN_PIXEL_HEIGHT 5
#define MAX_PIXEL_HEIGHT 72
