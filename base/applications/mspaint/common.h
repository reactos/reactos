/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/common.h
 * PURPOSE:     Commonly used functions
 * PROGRAMMERS: Benedikt Freisen
 *              Stanislav Motylkov
 */

#pragma once

/* FUNCTIONS ********************************************************/

BOOL zoomTo(int newZoom, int mouseX, int mouseY);
int Zoomed(int xy);
int UnZoomed(int xy);
