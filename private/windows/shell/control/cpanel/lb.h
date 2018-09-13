/*----------------------------------------------------------------------------*\
|   lb.h - a simple owner draw list box                                        |
|                                                                              |
|   History:                                                                   |
|	01/15/89 toddla     Created					       |
|                                                                              |
 *  10:30 on Tues  04 Feb 1992	-by-	Steve Cathcart   [stevecat]
 *	    Updated code to latest Win 3.1 sources
 *
 *  Copyright (C) 1990-1992 Microsoft Corporation
 *
\*----------------------------------------------------------------------------*/

BOOL lbInit(HANDLE hInst);

#define LBCLASS         TEXT("lb")

// flags
#define LB_FOCUS    0x0001
#define LB_CAPTURE  0x0002
#define LB_REDRAW   0x0003

typedef struct {
    HWND    hwnd;           // list box window
    HWND    hwndOwner;      // parent of list box window
    int     id;             // list box id
    int     nItems;         // actual number of items
    int     nx;             // number of items in the x dimension
    int     ny;
    int     dx;             // size of a item
    int     dy;
    int     dyScroll;       // current scoll pos
    int     nyScroll;       // current scoll range
    WORD    flb;            // flags
    int     iCurSel;        // current selected item
    DWORD   lData[1];
}   LB;

typedef LB     * PLB;

