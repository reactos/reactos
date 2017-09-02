/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/dib.cpp
 * PURPOSE:     Some DIB related functions
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

HBITMAP
CreateDIBWithProperties(int width, int height)
{
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    return CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
}

int
GetDIBWidth(HBITMAP hBitmap)
{
    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);
    return bm.bmWidth;
}

int
GetDIBHeight(HBITMAP hBitmap)
{
    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);
    return bm.bmHeight;
}

void
SaveDIBToFile(HBITMAP hBitmap, LPTSTR FileName, HDC hDC, LPSYSTEMTIME time, int *size, int hRes, int vRes)
{
    CImage img;
    img.Attach(hBitmap);
    img.Save(FileName);  // TODO: error handling
    img.Detach();

    // update time and size

    HANDLE hFile =
        CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    if (time)
    {
        FILETIME ft;
        GetFileTime(hFile, NULL, NULL, &ft);
        FileTimeToSystemTime(&ft, time);
    }
    if (size)
        *size = GetFileSize(hFile, NULL);

    // TODO: update hRes and vRes

    CloseHandle(hFile);
}

void ShowFileLoadError(LPCTSTR name)
{
    CString strText;
    strText.Format(IDS_LOADERRORTEXT, (LPCTSTR) name);
    CString strProgramName;
    strProgramName.LoadString(IDS_PROGRAMNAME);
    mainWindow.MessageBox(strText, strProgramName, MB_OK | MB_ICONEXCLAMATION);
}

void
LoadDIBFromFile(HBITMAP * hBitmap, LPCTSTR name, LPSYSTEMTIME time, int *size, int *hRes, int *vRes)
{
    using namespace Gdiplus;
    Bitmap img(CStringW(name), FALSE);  // always use WCHAR string

    if (!hBitmap)
    {
        ShowFileLoadError(name);
        return;
    }

    img.GetHBITMAP(Color(255, 255, 255), hBitmap);

    // update time and size
    HANDLE hFile =
        CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ShowFileLoadError(name);
        return;
    }

    if (time)
    {
        FILETIME ft;
        GetFileTime(hFile, NULL, NULL, &ft);
        FileTimeToSystemTime(&ft, time);
    }
    if (size)
        *size = GetFileSize(hFile, NULL);

    // update hRes and vRes
    *hRes = (int) (img.GetHorizontalResolution() * 1000 / 25.4);
    *vRes = (int) (img.GetVerticalResolution() * 1000 / 25.4);

    CloseHandle(hFile);
}
