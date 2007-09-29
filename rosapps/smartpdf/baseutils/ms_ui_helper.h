/*++

Copyright (c) 2003 Microsoft Corporation

Module Name:

    uihelper.h

Abstract:

    Include file for HIDPI / orientation / font change helper functions.

--*/

#ifndef __UIHELPER_H__
#define __UIHELPER_H__

#include <windows.h>
#include <commctrl.h>
#include "ms_ui_shguim.h"

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////
// HIDPI functions and constants.
////////////////////////////////////////////////////////////////////////////////

#ifndef ILC_COLORMASK
#define ILC_COLORMASK 0x00FE
#endif

//
// The two macros HIDPISIGN and HIDPIABS are there to ensure correct rounding
// for negative numbers passed into HIDPIMulDiv as x (we want -1.5 to round
// to -1, 2.5 to round to 2, etc).  So we use the absolute value of x, and then
// multiply the result by the sign of x.  Y and z should never be negative, as
// y is the dpi of the device (presumably 192 or 96), and z is always 96, as
// that is our original dpi we developed on.
//

#define HIDPISIGN(x) (((x)<0)?-1:1)
#define HIDPIABS(x) (((x)<0)?-(x):x)
#define HIDPIMulDiv(x,y,z) ((((HIDPIABS(x)*(y))+((z)>>1))/(z))*HIDPISIGN(x))

//
// Cached values of GetDeviceCaps(LOGPIXELSX/Y) for the screen DC.
//
EXTERN_C int g_HIDPI_LogPixelsX;
EXTERN_C int g_HIDPI_LogPixelsY;

//
// You need to define these somewhere in your .c files only if you make use of
// the scaling macros.  (Defined in UIHelper.cpp).
//
#define HIDPI_ENABLE \
    int g_HIDPI_LogPixelsX; \
    int g_HIDPI_LogPixelsY;

//
// Scaling macros.
//
#define SCALEX(argX) (HIDPIMulDiv(argX,g_HIDPI_LogPixelsX,96))
#define SCALEY(argY) (HIDPIMulDiv(argY,g_HIDPI_LogPixelsY,96))

#define UNSCALEX(argX) (HIDPIMulDiv(argX,96,g_HIDPI_LogPixelsX))
#define UNSCALEY(argY) (HIDPIMulDiv(argY,96,g_HIDPI_LogPixelsY))

#define SCALERECT(rc) { rc.left = SCALEX(rc.left); rc.right = SCALEX(rc.right); rc.top = SCALEY(rc.top); rc.bottom = SCALEY(rc.bottom);}
#define SCALEPT(pt) { pt.x = SCALEX(pt.x); pt.y = SCALEY(pt.y);}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: HIDPI_InitScaling
//
// PURPOSE: Initializes g_HIDPI_LogPixelsX and g_HIDPI_LogPixelsY.  This
//     should be called once at the beginning of any HIDPI-aware application.
//
__inline void HIDPI_InitScaling()
{
    HDC screen;

    if( g_HIDPI_LogPixelsX )
        return;

    screen = GetDC(NULL);
    g_HIDPI_LogPixelsX = GetDeviceCaps(screen, LOGPIXELSX);
    g_HIDPI_LogPixelsY = GetDeviceCaps(screen, LOGPIXELSY);
    ReleaseDC(NULL, screen);
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: HIDPI_StretchBitmap
//
// PURPOSE: Stretches a bitmap containing a grid of images.  There are
//     cImagesX images per row and cImagesY rows per bitmap.  Each image is
//     scaled individually, so that there are no artifacts with non-integral
//     scaling factors.  If the bitmap contains only one image, set cImagesX
//     and cImagesY to 1.
//
// ON ENTRY:
//     HBITMAP* phbm: a pointer to the bitmap to be scaled.
//     INT cxDstImg: the width of each image after scaling.
//     INT cyDstImg: the height of each image after scaling.
//     INT cImagesX: the number of images per row. This value should
//         evenly divide the width of the bitmap.
//     INT cImagesY: the number of rows in the bitmap. This value should
//         evenly divide the height of the bitmap.
//
// ON EXIT:
//     Returns TRUE on success, FALSE on failure.
//
//     If any scaling has occured, the bitmap pointed to by phbm is deleted
//     and is replaced by a new bitmap handle.
//
BOOL HIDPI_StretchBitmap(
    HBITMAP* phbm,
    int cxDstImg,
    int cyDstImg,
    int cImagesX,
    int cImagesY
    );

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: HIDPI_GetBitmapLogPixels
//
// PURPOSE: retrieves the DPI fields of the specified bitmap.
//
// ON ENTRY:
//     HINSTANCE hinst: the HINSTANCE of the bitmap resource.
//     LPCTSTR lpbmp: the ID of the bitmap resource.  The MAKEINTRESOURCE
//         macro can be used for integer IDs.
//     INT* pnLogPixelsX: the returned value for the horizontal DPI field of
//         the bitmap.  This value is never less than 96.
//     INT* pnLogPixelsY: the returned value for the vertical DPI field of
//         the bitmap.  This value is never less than 96.
//
// ON EXIT:
//     Returns TRUE on success, FALSE on failure.
//
BOOL HIDPI_GetBitmapLogPixels(
    HINSTANCE hinst,
    LPCTSTR lpbmp,
    int* pnLogPixelsX,
    int* pnLogPixelsY
    );

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: HIDPI_StretchIcon
//
// PURPOSE: stretches an icon to the specified size on 4.21 devices and later.
//     On 4.20 and previous revisions of the OS, this is a no-op.
//
// ON ENTRY:
//     HICON* phic: the icon to stretch.
//     INT cxIcon: the desired width of the icon.
//     INT cyIcon: the desired height of the icon.
//
// ON EXIT:
//     Returns TRUE on success, FALSE on failure.
//
//     If any stretching occurred, the icon pointed to by phic is deleted and
//     is replaced by a new icon handle.
//
BOOL HIDPI_StretchIcon(
    HICON* phic,
    int cxIcon,
    int cyIcon
    );

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: HIDPI_ImageList_LoadImage
//
// PURPOSE: This function operates identically to ImageList_LoadImage, except
//     that it first checks the DPI fields of the bitmap (using
//     HIDPI_GetBitmapLogPixels); compares it to the DPI of the screen
//     (using g_HIDPI_LogPixelsX and g_HIDPI_LogPixelsY), and performs scaling
//     (using HIDPI_StretchBitmap) if the values are different.
//
// ON ENTRY:
//     See the MSDN documentation for ImageList_LoadImage.
//
// ON EXIT:
//     See the MSDN documentation for ImageList_LoadImage.
//
HIMAGELIST HIDPI_ImageList_LoadImage(
    HINSTANCE hinst,
    LPCTSTR lpbmp,
    int cx,
    int cGrow,
    COLORREF crMask,
    UINT uType,
    UINT uFlags
    );

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: HIDPI_ImageList_ReplaceIcon
//
// PURPOSE: Replaces an icon in an ImageList, scaling it from its original size
//          to the size of the images in the ImageList.
//
// ON ENTRY:
//     See the MSDN documentation for ImageList_ReplaceIcon.
//
// ON EXIT:
//     See the MSDN documentation for ImageList_ReplaceIcon.
//
int HIDPI_ImageList_ReplaceIcon(
    HIMAGELIST himl,
    int i,
    HICON hicon
    );

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: HIDPI_ImageList_AddIcon
//
// PURPOSE: Adds an icon to an ImageList, scaling it from its original size
//          to the size of the images in the ImageList.
//
// ON ENTRY:
//     See the MSDN documentation for ImageList_AddIcon.
//
// ON EXIT:
//     See the MSDN documentation for ImageList_AddIcon.
//
#define HIDPI_ImageList_AddIcon(himl, hicon) HIDPI_ImageList_ReplaceIcon(himl, -1, hicon)

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: HIDPI_Rectangle
//
// PURPOSE: Draws a rectangle using the currently selected pen.  Drawing occurs
//    completely within the drawing rectangle (the rectangle has an "inside
//    frame" drawing style).
//
// ON ENTRY:
//     HDC hdc: the display context of the drawing surface.
//     INT nLeft: left bound of rectangle
//     INT nTop: top bound of rectangle
//     INT nRight: right bound of rectangle plus one.
//     INT nBottom: bottom bound of rectangle plus one.
//
// ON EXIT:
//     Returns TRUE on success, FALSE on failure.
//
BOOL HIDPI_Rectangle(HDC hdc, int nLeft, int nTop, int nRight, int nBottom);

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: HIDPI_BorderRectangle
//
// PURPOSE: Draws a rectangle with the system border pen.  Drawing occurs
//    completely within the drawing rectangle (the rectangle has an "inside
//    frame" drawing style).
//
// ON ENTRY:
//     HDC hdc: the display context of the drawing surface.
//     INT nLeft: left bound of rectangle
//     INT nTop: top bound of rectangle
//     INT nRight: right bound of rectangle plus one.
//     INT nBottom: bottom bound of rectangle plus one.
//
// ON EXIT:
//     Returns TRUE on success, FALSE on failure.
//
BOOL HIDPI_BorderRectangle(HDC hdc, int nLeft, int nTop, int nRight, int nBottom);

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: HIDPI_Polyline
//
// PURPOSE: Draws a polyline using the currently selected pen.  In addition,
//     this function provides control over how the line will be drawn.
//
// ON ENTRY:
//     HDC hdc: the display context of the drawing surface.
//     const POINT* lppt: array of POINTS that specify line to draw.
//     INT cPoints: number of points in array.
//     INT nStyle: the style the pen should be drawn in.  This may be an
//        existing pen style, such as PS_SOLID, or one of the following styles:
//
//           PS_LEFTBIAS      PS_UPBIAS        PS_UPLEFT
//           PS_RIGHTBIAS     PS_DOWNBIAS      PS_DOWNRIGHT
//
//        These styles indicate how the pen should "hang" from each line
//        segment.  By default, the pen is centered along the line, but with
//        these line styles the developer can draw lines above, below, to the
//        left or to the right of the line segment.
//
// ON EXIT:
//     Returns TRUE on success, FALSE on failure.
//

#define PS_RIGHTBIAS    0x10
#define PS_LEFTBIAS     0x20
#define PS_DOWNBIAS     0x40
#define PS_UPBIAS       0x80
#define PS_DOWNRIGHT    (PS_DOWNBIAS | PS_RIGHTBIAS)
#define PS_UPLEFT       (PS_UPBIAS | PS_LEFTBIAS)
#define PS_BIAS_MASK    (PS_RIGHTBIAS | PS_LEFTBIAS | PS_DOWNBIAS | PS_UPBIAS)

BOOL HIDPI_Polyline(HDC hdc, const POINT *lppt, int cPoints, int nStyle);

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: HIDPI_BorderPolyline
//
// PURPOSE: Draws a polyline, but with the system border pen.  In addition,
//     this function provides control over how the line will be drawn.
//
// ON ENTRY:
//     HDC hdc: the display context of the drawing surface.
//     const POINT* lppt: array of POINTS that specify line to draw.
//     INT cPoints: number of points in array.
//     INT nStyle: the style the pen should be drawn in.  See HIDPI_Polyline
//         for more details.
//
// ON EXIT:
//     Returns TRUE on success, FALSE on failure.
//
BOOL HIDPI_BorderPolyline(HDC hdc, const POINT *lppt, int cPoints, int nStyle);

////////////////////////////////////////////////////////////////////////////////
// Orientation functions.
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: RelayoutDialog
//
// PURPOSE: Re-lays out a dialog based on a dialog template.  This function
//      iterates through all the child window controls and does a SetWindowPos
//      for each.  It also does a SetWindowText for each static text control
//      and updates the selected bitmap or icon in a static image control.
//      This assumes that the current dialog and the new template have all the
//      same controls, with the same IDCs.
//
// ON ENTRY:
//      HINSTANCE hInst: the hInstance of the current module.
//      HWND hDlg: the dialog to layout.
//      LPCWSTR iddTemplate: the new template for the dialog (can use
//          the MAKEINTRESOURCE macro).
//
// ON EXIT: TRUE if success; FALSE if failure (either the iddTemplate is
//      invalid, or there are two or more IDC_STATICs in the template).
//
BOOL RelayoutDialog(HINSTANCE hInst, HWND hDlg, LPCWSTR iddTemplate);

#ifdef __cplusplus
}
#endif

#endif // __UIHELPER_H__
