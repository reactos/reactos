/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/winproc.h
 * PURPOSE:     Window procedure of the main window and all children apart from
 *              hPalWin, hToolSettings and hSelection
 * PROGRAMMERS: Benedikt Freisen
 */

void RegisterWclMain();

LRESULT CALLBACK MainWindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
