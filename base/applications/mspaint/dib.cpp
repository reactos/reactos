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
CreateColorDIB(int width, int height, COLORREF rgb)
{
    HBITMAP ret = CreateDIBWithProperties(width, height);
    if (!ret)
        return NULL;

    if (rgb)
    {
        HDC hdc = CreateCompatibleDC(NULL);
        HGDIOBJ hbmOld = SelectObject(hdc, ret);
        RECT rc;
        SetRect(&rc, 0, 0, width, height);
        HBRUSH hbr = CreateSolidBrush(rgb);
        FillRect(hdc, &rc, hbr);
        DeleteObject(hbr);
        SelectObject(hdc, hbmOld);
        DeleteDC(hdc);
    }

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

BOOL SaveDIBToFile(HBITMAP hBitmap, LPTSTR FileName, HDC hDC)
{
    CImage img;
    img.Attach(hBitmap);
    img.Save(FileName);  // TODO: error handling
    img.Detach();

    WIN32_FIND_DATA find;
    HANDLE hFind = FindFirstFile(FileName, &find);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        ShowFileLoadError(FileName);
        return FALSE;
    }
    FindClose(hFind);

    // update time and size
    FILETIME ft;
    FileTimeToLocalFileTime(&find.ftLastWriteTime, &ft);
    FileTimeToSystemTime(&ft, &fileTime);
    fileSize = find.nFileSizeLow;

    // TODO: update hRes and vRes

    registrySettings.SetMostRecentFile(FileName);

    isAFile = TRUE;
    imageSaved = TRUE;
    return TRUE;
}

void ShowFileLoadError(LPCTSTR name)
{
    CString strText;
    strText.Format(IDS_LOADERRORTEXT, (LPCTSTR) name);
    CString strProgramName;
    strProgramName.LoadString(IDS_PROGRAMNAME);
    mainWindow.MessageBox(strText, strProgramName, MB_OK | MB_ICONEXCLAMATION);
}

HBITMAP SetBitmapAndInfo(HBITMAP hBitmap, LPCTSTR name, DWORD dwFileSize, BOOL isFile)
{
    if (hBitmap == NULL)
    {
        COLORREF white = RGB(255, 255, 255);
        hBitmap = CreateColorDIB(registrySettings.BMPWidth,
                                 registrySettings.BMPHeight, white);
        if (hBitmap == NULL)
            return FALSE;

        fileHPPM = fileVPPM = 2834;
        ZeroMemory(&fileTime, sizeof(fileTime));
        isFile = FALSE;
    }
    else
    {
        // update PPMs
        HDC hScreenDC = GetDC(NULL);
        fileHPPM = (int)(GetDeviceCaps(hScreenDC, LOGPIXELSX) * 1000 / 25.4);
        fileVPPM = (int)(GetDeviceCaps(hScreenDC, LOGPIXELSY) * 1000 / 25.4);
        ReleaseDC(NULL, hScreenDC);
    }

    // update image
    imageModel.Insert(hBitmap);
    imageModel.ClearHistory();

    // update fileSize
    fileSize = dwFileSize;

    // update filepathname
    if (name && name[0])
        GetFullPathName(name, _countof(filepathname), filepathname, NULL);
    else
        LoadString(hProgInstance, IDS_DEFAULTFILENAME, filepathname, _countof(filepathname));

    // set title
    CString strTitle;
    strTitle.Format(IDS_WINDOWTITLE, PathFindFileName(filepathname));
    mainWindow.SetWindowText(strTitle);

    // update file info and recent
    isAFile = isFile;
    if (isAFile)
        registrySettings.SetMostRecentFile(filepathname);

    imageSaved = TRUE;

    return hBitmap;
}

HBITMAP DoLoadImageFile(HWND hwnd, LPCTSTR name, BOOL fIsMainFile)
{
    // find the file
    WIN32_FIND_DATA find;
    HANDLE hFind = FindFirstFile(name, &find);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        // does not exist
        CStringW strText;
        strText.Format(IDS_LOADERRORTEXT, name);
        MessageBoxW(hwnd, strText, NULL, MB_ICONERROR);
        return NULL;
    }
    DWORD dwFileSize = find.nFileSizeLow; // get file size
    FindClose(hFind);

    // is file empty?
    if (dwFileSize == 0)
    {
        if (fIsMainFile)
        {
            FILETIME ft;
            FileTimeToLocalFileTime(&find.ftLastWriteTime, &ft);
            FileTimeToSystemTime(&ft, &fileTime);
            return SetBitmapAndInfo(NULL, name, dwFileSize, TRUE);
        }
    }

    // load the image
    CImage img;
    img.Load(name);
    HBITMAP hBitmap = img.Detach();

    if (hBitmap == NULL)
    {
        // cannot open
        CStringW strText;
        strText.Format(IDS_LOADERRORTEXT, name);
        MessageBoxW(hwnd, strText, NULL, MB_ICONERROR);
        return NULL;
    }

    if (fIsMainFile)
    {
        FILETIME ft;
        FileTimeToLocalFileTime(&find.ftLastWriteTime, &ft);
        FileTimeToSystemTime(&ft, &fileTime);
        SetBitmapAndInfo(hBitmap, name, dwFileSize, TRUE);
    }

    return hBitmap;
}
