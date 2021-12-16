/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Performance Graph Meters.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#define BRIGHT_GREEN	RGB(0, 255, 0)
#define MEDIUM_GREEN	RGB(0, 190, 0)
#define DARK_GREEN	RGB(0, 130, 0)
#define RED		RGB(255, 0, 0)

extern	WNDPROC		OldGraphWndProc;

INT_PTR CALLBACK	Graph_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


#ifdef __cplusplus
};
#endif
