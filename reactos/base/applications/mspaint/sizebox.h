/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/paint/sizebox.h
 * PURPOSE:     Window procedure of the size boxes
 * PROGRAMMERS: Benedikt Freisen
 */

void RegisterWclSizebox();

LRESULT CALLBACK SizeboxWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
