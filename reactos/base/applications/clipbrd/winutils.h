/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Clipboard Viewer
 * FILE:            base/applications/clipbrd/winutils.c
 * PURPOSE:         Miscellaneous helper functions.
 * PROGRAMMERS:     Ricardo Hanke
 */

void ShowLastWin32Error(HWND hwndParent);
void BringWindowToFront(HWND hWnd);
int DrawTextFromResource(HINSTANCE hInstance, UINT uID, HDC hDC, LPRECT lpRect, UINT uFormat);
int MessageBoxRes(HWND hWnd, HINSTANCE hInstance, UINT uText, UINT uCaption, UINT uType);
void DrawTextFromClipboard(HDC hDC, LPRECT lpRect, UINT uFormat);
void BitBltFromClipboard(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, int nXSrc, int nYSrc, DWORD dwRop);
void SetDIBitsToDeviceFromClipboard(UINT uFormat, HDC hdc, int XDest, int YDest, int XSrc, int YSrc, UINT uStartScan, UINT fuColorUse);
void PlayMetaFileFromClipboard(HDC hdc, const RECT *lpRect);
void PlayEnhMetaFileFromClipboard(HDC hdc, const RECT *lpRect);
