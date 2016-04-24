/*
 * WineCalc (magnifier.h)
 *
 * Copyright 2007 Marc Piulachs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _MAGNIFIER_H_
#define _MAGNIFIER_H_

#include <stdarg.h>
#include <windef.h>

extern int iZoom;

struct _AppBarConfig_t {
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
