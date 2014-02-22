/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/paint/textedit.h
 * PURPOSE:     Text editor and font chooser for the text tool
 * PROGRAMMERS: Benedikt Freisen
 */

void RegisterWclTextEdit();

LRESULT CALLBACK TextEditWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
