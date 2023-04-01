/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/common.h
 * PURPOSE:     Commonly used functions and definitions
 * PROGRAMMERS: Benedikt Freisen
 *              Stanislav Motylkov
 *              Katayama Hirofumi MZ
 */

#pragma once

#define GRIP_SIZE   3
#define MIN_ZOOM    125
#define MAX_ZOOM    8000

#define MAX_LONG_PATH 512

#define WM_TOOLSMODELTOOLCHANGED         (WM_APP + 0)
#define WM_TOOLSMODELSETTINGSCHANGED     (WM_APP + 1)
#define WM_TOOLSMODELZOOMCHANGED         (WM_APP + 2)
#define WM_PALETTEMODELCOLORCHANGED      (WM_APP + 3)
#define WM_PALETTEMODELPALETTECHANGED    (WM_APP + 4)
#define WM_IMAGEMODELDIMENSIONSCHANGED   (WM_APP + 5)
#define WM_IMAGEMODELIMAGECHANGED        (WM_APP + 6)
#define WM_SELECTIONMODELREFRESHNEEDED   (WM_APP + 7)

/* width of the rectangle defined by a RECT structure */
#define RECT_WIDTH(a)  ((a).right - (a).left)

/* height of the rectangle defined by a RECT structure */
#define RECT_HEIGHT(a)  ((a).bottom - (a).top)

/* this simplifies checking and unchecking menu items */
#define CHECKED_IF(a) ((a) ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND))

/* this simplifies enabling or graying menu items */
#define ENABLED_IF(a) ((a) ? (MF_ENABLED | MF_BYCOMMAND) : (MF_GRAYED | MF_BYCOMMAND))

enum CANVAS_HITTEST // hit
{
    HIT_NONE = 0, // Nothing hit or outside
    HIT_UPPER_LEFT,
    HIT_UPPER_CENTER,
    HIT_UPPER_RIGHT,
    HIT_MIDDLE_LEFT,
    HIT_MIDDLE_RIGHT,
    HIT_LOWER_LEFT,
    HIT_LOWER_CENTER,
    HIT_LOWER_RIGHT,
    HIT_BORDER,
    HIT_INNER,
};

/* FUNCTIONS ********************************************************/

BOOL zoomTo(int newZoom, int mouseX, int mouseY);
BOOL nearlyEqualPoints(INT x0, INT y0, INT x1, INT y1);
void updateStartAndLast(LONG x, LONG y);
void updateLast(LONG x, LONG y);
