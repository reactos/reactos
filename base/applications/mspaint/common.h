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

BOOL zoomTo(int, int, int);
int Zoomed(int xy);
int UnZoomed(int xy);
