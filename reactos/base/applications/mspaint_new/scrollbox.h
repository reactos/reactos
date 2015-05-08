/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/scrollbox.h
 * PURPOSE:     Functionality surrounding the scroll box window class
 * PROGRAMMERS: Benedikt Freisen
 */

void RegisterWclScrollbox();

void UpdateScrollbox();

LRESULT CALLBACK ScrollboxWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
