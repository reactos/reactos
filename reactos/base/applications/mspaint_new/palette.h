/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/palette.h
 * PURPOSE:     Window procedure of the palette window
 * PROGRAMMERS: Benedikt Freisen
 */

void RegisterWclPal();

LRESULT CALLBACK PalWinProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
