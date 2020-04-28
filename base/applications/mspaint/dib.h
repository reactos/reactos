/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/dib.h
 * PURPOSE:     Some DIB related functions
 * PROGRAMMERS: Benedikt Freisen
 */

#pragma once

HBITMAP CreateDIBWithProperties(int width, int height);
HBITMAP CreateColorDIB(int width, int height, COLORREF rgb);

int GetDIBWidth(HBITMAP hbm);

int GetDIBHeight(HBITMAP hbm);

void SaveDIBToFile(HBITMAP hBitmap, LPTSTR FileName, HDC hDC);

BOOL DoLoadImageFile(HWND hwnd, HBITMAP *phBitmap, LPCTSTR name, BOOL fIsMainFile);

void ShowFileLoadError(LPCTSTR name);

BOOL SetBitmapAndInfo(HBITMAP *phBitmap, LPCTSTR name, DWORD dwFileSize, BOOL isFile);
