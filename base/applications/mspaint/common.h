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

static inline int Zoomed(int xy)
{
    return xy * toolsModel.GetZoom() / 1000;
}

static inline int UnZoomed(int xy)
{
    return xy * 1000 / toolsModel.GetZoom();
}

#define GRIP_SIZE 3
