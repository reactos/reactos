/*
 * PROJECT:   ReactOS Task Manager
 * LICENSE:   LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * COPYRIGHT: 1999-2001 Brian Palmer <brianp@reactos.org>
 */

#pragma once

INT_PTR CALLBACK ProcessListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

extern WNDPROC OldProcessListWndProc;
