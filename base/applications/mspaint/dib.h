/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Some DIB related functions
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#pragma once

BOOL IsBitmapBlackAndWhite(HBITMAP hbm);
HBITMAP CreateDIBWithProperties(int width, int height);
HBITMAP CreateMonoBitmap(int width, int height, BOOL bWhite);
HBITMAP CreateColorDIB(int width, int height, COLORREF rgb);
HBITMAP CachedBufferDIB(HBITMAP hbm, int minimalWidth, int minimalHeight);
HBITMAP ConvertToBlackAndWhite(HBITMAP hbm);

HBITMAP CopyMonoImage(HBITMAP hbm, INT cx = 0, INT cy = 0);

static inline HBITMAP CopyDIBImage(HBITMAP hbm, INT cx = 0, INT cy = 0)
{
    return (HBITMAP)CopyImage(hbm, IMAGE_BITMAP, cx, cy, LR_COPYRETURNORG | LR_CREATEDIBSECTION);
}

int GetDIBWidth(HBITMAP hbm);
int GetDIBHeight(HBITMAP hbm);

BOOL SaveDIBToFile(HBITMAP hBitmap, LPCWSTR FileName, BOOL fIsMainFile, REFGUID guidFileType = GUID_NULL);

HBITMAP DoLoadImageFile(HWND hwnd, LPCWSTR name, BOOL fIsMainFile);

void SetFileInfo(LPCWSTR name, LPWIN32_FIND_DATAW pFound, BOOL isAFile);

HBITMAP InitializeImage(LPCWSTR name, LPWIN32_FIND_DATAW pFound, BOOL isFile);
HBITMAP SetBitmapAndInfo(HBITMAP hBitmap, LPCWSTR name, LPWIN32_FIND_DATAW pFound, BOOL isFile);

HBITMAP Rotate90DegreeBlt(HDC hDC1, INT cx, INT cy, BOOL bRight, BOOL bMono);

HBITMAP SkewDIB(HDC hDC1, HBITMAP hbm, INT nDegree, BOOL bVertical, BOOL bMono = FALSE);

float PpcmFromDpi(float dpi);

#define ROUND(x) (INT)((x) + 0.5)

HGLOBAL BitmapToClipboardDIB(HBITMAP hBitmap);
HBITMAP BitmapFromClipboardDIB(HGLOBAL hGlobal);
HBITMAP BitmapFromHEMF(HENHMETAFILE hEMF);
HBITMAP getSubImage(HBITMAP hbmWhole, const RECT& rcPartial);
void putSubImage(HBITMAP hbmWhole, const RECT& rcPartial, HBITMAP hbmPart);
