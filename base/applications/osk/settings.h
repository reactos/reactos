/*
 * PROJECT:         ReactOS On-Screen Keyboard
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Settings header file with function prototypes
 * COPYRIGHT:       Copyright 2018 Bișoc George (fraizeraust99 at gmail dot com)
 */

#ifndef SETTINGS_OSK_H
#define SETTINGS_OSK_H

BOOL LoadDataFromRegistry(VOID);
BOOL SaveDataToRegistry(VOID);
INT_PTR CALLBACK OSK_WarningProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

#endif // SETTINGS_OSK_H
