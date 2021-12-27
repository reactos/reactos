/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/common.h
 * PURPOSE:     Commonly used functions
 * PROGRAMMERS: Benedikt Freisen
 *              Stanislav Motylkov
 *              Katayama Hirofumi MZ
 */

#pragma once

/* FUNCTIONS ********************************************************/

BOOL zoomTo(int newZoom, int mouseX, int mouseY);
BOOL nearlyEqualPoints(INT x0, INT y0, INT x1, INT y1);

static inline int Zoomed(int xy)
{
    return xy * toolsModel.GetZoom() / 1000;
}

static inline int UnZoomed(int xy)
{
    return xy * 1000 / toolsModel.GetZoom();
}

#define GRIP_SIZE 3
#define MIN_ZOOM 125
#define MAX_ZOOM 8000
