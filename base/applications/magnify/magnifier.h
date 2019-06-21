/*
 * PROJECT:     ReactOS Magnify
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Magnification of parts of the screen.
 * COPYRIGHT:   Copyright 2007-2019 Marc Piulachs <marc.piulachs@codexchange.net>
 *              Copyright 2015-2019 David Quintana <gigaherz@gmail.com>
 */

#ifndef _MAGNIFIER_H_
#define _MAGNIFIER_H_

#include <stdarg.h>
#include <windef.h>

extern UINT uiZoom;

struct _AppBarConfig_t
{
    DWORD cbSize;
    INT   uEdge;
    DWORD value3;
    DWORD value4;
    RECT appBarSizes;
    RECT rcFloating;
};
extern struct _AppBarConfig_t AppBarConfig;

extern BOOL bShowWarning;

extern BOOL bFollowMouse;
extern BOOL bFollowFocus;
extern BOOL bFollowCaret;

extern BOOL bInvertColors;
extern BOOL bStartMinimized;
extern BOOL bShowMagnifier;

void LoadSettings(void);
void SaveSettings(void);

#endif /* _MAGNIFIER_H_ */
