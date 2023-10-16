/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Commonly used functions and definitions
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2018 Stanislav Motylkov <x86corez@gmail.com>
 *             Copyright 2021-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

/* this simplifies checking and unchecking menu items */
#define CHECKED_IF(a) ((a) ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND))

/* this simplifies enabling or graying menu items */
#define ENABLED_IF(a) ((a) ? (MF_ENABLED | MF_BYCOMMAND) : (MF_GRAYED | MF_BYCOMMAND))

enum HITTEST // hit
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

BOOL nearlyEqualPoints(INT x0, INT y0, INT x1, INT y1);
BOOL OpenMailer(HWND hWnd, LPCWSTR pszPathName);

#define DEG2RAD(degree) (((degree) * M_PI) / 180)
#define RAD2DEG(radian) ((LONG)(((radian) * 180) / M_PI))
