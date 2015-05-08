/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/paint/palette.h
 * PURPOSE:     Window procedure of the palette window
 * PROGRAMMERS: Benedikt Freisen
 */

void RegisterWclPal();

LRESULT CALLBACK PalWinProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
