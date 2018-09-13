/*++

Copyright (c) 1990-1995,  Microsoft Corporation  All rights reserved.

Module Name:

    color.h

Abstract:

    This module contains the header information for the Win32 color dialogs.

Revision History:

--*/



//
//  Include Files.
//

#include "colordlg.h"




//
//  Constant Declarations.
//

#define COLORBOXES           64
#define NUM_X_BOXES          8
#define BOX_X_MARGIN         5
#define BOX_Y_MARGIN         5
//
//  Range of values for HLS scrollbars.
//  HLS-RGB conversions work best when RANGE is divisible by 6.
//
#define RANGE                240

#define HLSMAX               RANGE
#define RGBMAX               255

#define HUEINC               4
#define SATINC               8
#define LUMINC               8

#define CC_RGBINIT           0x00000001
#define CC_FULLOPEN          0x00000002
#define CC_PREVENTFULLOPEN   0x00000004

//
//  This used to be in wingdi.h, but recently moved to wingdip.h
//  Including wingdip.h causes too many compiler errors, so define
//  the one constant we need here.
//
#define HS_DITHEREDTEXTCLR   9

#define COLORPROP  (LPCTSTR) 0xA000L




//
//  Typedef Declarations.
//

typedef struct {
    UINT           ApiType;
    LPCHOOSECOLOR  pCC;
    HANDLE         hLocal;
    HANDLE         hDialog;
    DWORD          currentRGB;
    WORD           currentHue;
    WORD           currentSat;
    WORD           currentLum;
    WORD           nHueWidth;
    WORD           nSatHeight;
    WORD           nLumHeight;
    WORD           nCurMix;
    WORD           nCurDsp;
    WORD           nCurBox;
    WORD           nHuePos;
    WORD           nSatPos;
    WORD           nLumPos;
    RECT           rOriginal;
    RECT           rRainbow;
    RECT           rLumScroll;
    RECT           rLumPaint;
    RECT           rCurrentColor;
    RECT           rNearestPure;
    RECT           rColorSamples;
    BOOL           bFoldOut;
    DWORD          rgbBoxColor[COLORBOXES];
#ifdef UNICODE
    LPCHOOSECOLORA pCCA;
#endif
} COLORINFO;

typedef COLORINFO *PCOLORINFO;

#define LPDIS LPDRAWITEMSTRUCT




//
//  Extern Declarations.
//

extern HDC hDCFastBlt;
extern DWORD rgbClient;
extern WORD H,S,L;
extern HBITMAP hRainbowBitmap;
extern BOOL bMouseCapture;
extern WNDPROC lpprocStatic;
extern SHORT nDriverColors;
extern DWORD rgbBoxColor[COLORBOXES];

extern TCHAR szOEMBIN[];

extern RECT rColorBox[COLORBOXES];
extern SHORT nBoxHeight, nBoxWidth;
extern HWND hSave;
extern WNDPROC qfnColorDlg;




//
// Function Prototypes.
//

//
//  color.c
//
BOOL
ChooseColorX(
    PCOLORINFO pCI);

BOOL WINAPI
ColorDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam);

BOOL
ChangeColorBox(
    register PCOLORINFO pCI,
    DWORD dwRGBcolor);

VOID
HiLiteBox(
    HDC hDC,
    SHORT nBox,
    SHORT fStyle);

VOID
ChangeBoxSelection(
    PCOLORINFO pCI,
    SHORT nNewBox);

VOID
ChangeBoxFocus(
    PCOLORINFO pCI,
    SHORT nNewBox);

BOOL
ColorKeyDown(
    WPARAM wParam,
    INT FAR *id,
    PCOLORINFO pCI);

VOID
PaintBox(
    PCOLORINFO pCI,
    register HDC hDC,
    SHORT i);

BOOL
InitScreenCoords(
    HWND hDlg,
    PCOLORINFO pCI);

VOID
SetupRainbowCapture(
    PCOLORINFO pCI);

BOOL
InitColor(
    HWND hDlg,
    WPARAM wParam,
    PCOLORINFO pCI);

VOID
ColorPaint(
    HWND hDlg,
    PCOLORINFO pCI,
    HDC hDC,
    LPRECT lpPaintRect);

LONG WINAPI
WantArrows(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

VOID
TermColor();


#ifdef UNICODE
  VOID
  ThunkChooseColorA2W(
      PCOLORINFO pCI);

  VOID
  ThunkChooseColorW2A(
      PCOLORINFO pCI);
#endif


//
//  color2.c
//
VOID
ChangeColorSettings(
    register PCOLORINFO pCI);

VOID
LumArrowPaint(
    HDC hDC,
    SHORT y,
    PCOLORINFO pCI);

VOID
EraseLumArrow(
    HDC hDC,
    PCOLORINFO pCI);

VOID
EraseCrossHair(
    HDC hDC,
    PCOLORINFO pCI);

VOID
CrossHairPaint(
    register HDC hDC,
    SHORT x,
    SHORT y,
    PCOLORINFO pCI);

VOID
NearestSolid(
    register PCOLORINFO pCI);

VOID
HLSPostoHLS(
    SHORT nHLSEdit,
    register PCOLORINFO pCI);

VOID
HLStoHLSPos(
    SHORT nHLSEdit,
    register PCOLORINFO pCI);

VOID
SetHLSEdit(
    SHORT nHLSEdit,
    register PCOLORINFO pCI);

VOID
SetRGBEdit(
    SHORT nRGBEdit,
    PCOLORINFO pCI);

BOOL
InitRainbow(
    register PCOLORINFO pCI);

VOID
PaintRainbow(
    HDC hDC,
    LPRECT lpRect,
    register PCOLORINFO pCI);

void
RainbowPaint(
    register PCOLORINFO pCI,
    HDC hDC,
    LPRECT lpPaintRect);

VOID
RGBtoHLS(
    DWORD lRGBColor);

WORD
HueToRGB(
    WORD n1,
    WORD n2,
    WORD hue);

DWORD
HLStoRGB(
    WORD hue,
    WORD lum,
    WORD sat);

SHORT
RGBEditChange(
    SHORT nDlgID,
    PCOLORINFO pCI);

