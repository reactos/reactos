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

HBITMAP
CreateWhiteDIB(int width, int height)
{
    HBITMAP ret = CreateDIBWithProperties(width, height);
    if (!ret)
        return NULL;

    HDC hdc = CreateCompatibleDC(NULL);
    HGDIOBJ hbmOld = SelectObject(hdc, ret);
    RECT rc;
    SetRect(&rc, 0, 0, width, height);
    FillRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
    SelectObject(hdc, hbmOld);
    DeleteDC(hdc);

    return ret;
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

    registrySettings.SetMostRecentFile(FileName);
}

void ShowFileLoadError(LPCTSTR name)
{
    CString strText;
    strText.Format(IDS_LOADERRORTEXT, (LPCTSTR) name);
    CString strProgramName;
    strProgramName.LoadString(IDS_PROGRAMNAME);
    mainWindow.MessageBox(strText, strProgramName, MB_OK | MB_ICONEXCLAMATION);
}

BOOL DoLoadImageFile(HWND hwnd, HBITMAP *phBitmap, LPCTSTR name, BOOL fIsMainFile)
{
    HBITMAP hBitmap = NULL;
    if (phBitmap)
        *phBitmap = hBitmap;

    // find the file
    WIN32_FIND_DATAW find;
    HANDLE hFind = FindFirstFileW(name, &find);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        // does not exist
        CStringW strText;
        strText.Format(IDS_LOADERRORTEXT, name);
        MessageBoxW(hwnd, strText, NULL, MB_ICONERROR);
        return FALSE;
    }
    DWORD dwFileSize = find.nFileSizeLow; // get file size
    FindClose(hFind);

    // is file empty?
    if (dwFileSize == 0)
    {
        if (fIsMainFile)
        {
            hBitmap = CreateWhiteDIB(registrySettings.BMPWidth, registrySettings.BMPHeight);
            if (phBitmap)
                *phBitmap = hBitmap;

            // update image
            imageModel.Insert(hBitmap);
            imageModel.ClearHistory();

            // update fileSize
            fileSize = dwFileSize;

            // update PPMs
            fileHPPM = fileVPPM = 0;

            // get full path
            GetFullPathName(name, SIZEOF(filepathname), filepathname, NULL);

            // set title
            CString strTitle;
            strTitle.Format(IDS_WINDOWTITLE, PathFindFileName(filepathname));
            mainWindow.SetWindowText(strTitle);

            // update file info
            isAFile = TRUE;
            registrySettings.SetMostRecentFile(filepathname);

            return TRUE;
        }
    }

    // load the image
    CImage img;
    img.Load(name);
    hBitmap = img.Detach();
    if (hBitmap == NULL)
    {
        // cannot open
        CStringW strText;
        strText.Format(IDS_LOADERRORTEXT, name);
        MessageBoxW(hwnd, strText, NULL, MB_ICONERROR);
        return FALSE;
    }
    if (phBitmap)
        *phBitmap = hBitmap;

    if (fIsMainFile)
    {
        // open the saved file to get fileTime and PPMs
        HANDLE hFile = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL,
                                  OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            ShowFileLoadError(name);

            DeleteObject(hBitmap);
            if (phBitmap)
                *phBitmap = NULL;

            return FALSE;
        }

        // update image
        imageModel.Insert(hBitmap);
        imageModel.ClearHistory();

        // update fileSize
        fileSize = dwFileSize;

        // update fileTime
        FILETIME ft;
        GetFileTime(hFile, NULL, NULL, &ft);
        FileTimeToSystemTime(&ft, &fileTime);

        // update PPMs
        HDC hScreenDC = GetDC(NULL);
        fileHPPM = (int)(GetDeviceCaps(hScreenDC, LOGPIXELSX) * 1000 / 25.4);
        fileVPPM = (int)(GetDeviceCaps(hScreenDC, LOGPIXELSY) * 1000 / 25.4);
        ReleaseDC(NULL, hScreenDC);

        CloseHandle(hFile);

        // get full path
        GetFullPathName(name, SIZEOF(filepathname), filepathname, NULL);

        // set title
        CString strTitle;
        strTitle.Format(IDS_WINDOWTITLE, PathFindFileName(filepathname));
        mainWindow.SetWindowText(strTitle);

        // update file info
        isAFile = TRUE;
        registrySettings.SetMostRecentFile(filepathname);
    }

    return TRUE;
}
