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

/* width of the rectangle defined by a RECT structure */
#define RECT_WIDTH(a)  ((a).right - (a).left)

/* height of the rectangle defined by a RECT structure */
#define RECT_HEIGHT(a)  ((a).bottom - (a).top)

/* this simplifies checking and unchecking menu items */
#define CHECKED_IF(a) ((a) ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND))

/* this simplifies enabling or graying menu items */
#define ENABLED_IF(a) ((a) ? (MF_ENABLED | MF_BYCOMMAND) : (MF_GRAYED | MF_BYCOMMAND))

/* FUNCTIONS ********************************************************/

BOOL zoomTo(int newZoom, int mouseX, int mouseY);
BOOL nearlyEqualPoints(INT x0, INT y0, INT x1, INT y1);
void placeSelWin(void);
void updateStartAndLast(LONG x, LONG y);
void updateLast(LONG x, LONG y);

static inline int Zoomed(int xy)
{
    return xy * toolsModel.GetZoom() / 1000;
}

static inline int UnZoomed(int xy)
{
    return xy * 1000 / toolsModel.GetZoom();
}
