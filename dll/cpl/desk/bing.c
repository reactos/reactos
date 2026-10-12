/*
 * PROJECT:     ReactOS Display Control Panel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     "Bing - picture of the day" wallpaper
 *
 * The picture is listed by Bing's public feed:
 *   https://www.bing.com/HPImageArchive.aspx?format=js&idx=0&n=1
 * which returns JSON with "url":"/th?id=OHR.Name_1920x1080.jpg&...".
 * The image is cached in %LOCALAPPDATA%\BingWallpaper.jpg and, while the option
 * is selected, refreshed at every logon by "rundll32 desk.cpl,UpdateBingWallpaper".
 */

#include "desk.h"

#include <wininet.h>
#include <shlwapi.h>

#define BING_FEED_URL   L"https://www.bing.com/HPImageArchive.aspx?format=js&idx=0&n=1"
#define BING_HOST       L"https://www.bing.com"
#define BING_REG_VALUE  L"ReactOSBingWallpaper"
#define BING_RUN_VALUE  L"ReactOSBingWallpaper"
#define BING_RUN_CMD    L"rundll32.exe desk.cpl,UpdateBingWallpaper"

BOOL
Bing_GetCachePath(LPWSTR pszPath, DWORD cchPath)
{
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, pszPath)))
        return FALSE;
    return SUCCEEDED(StringCchCatW(pszPath, cchPath, L"\\BingWallpaper.jpg"));
}

/* Read a whole (small) URL into a zero terminated buffer */
static PSTR
ReadUrl(HINTERNET hInternet, LPCWSTR pszUrl, DWORD cbMax)
{
    HINTERNET hUrl = InternetOpenUrlW(hInternet, pszUrl, NULL, 0,
                                      INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    PSTR pBuffer;
    DWORD cbTotal = 0, cbRead;

    if (!hUrl)
        return NULL;
    pBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbMax + 1);
    while (pBuffer && cbTotal < cbMax &&
           InternetReadFile(hUrl, pBuffer + cbTotal, cbMax - cbTotal, &cbRead) && cbRead)
    {
        cbTotal += cbRead;
    }
    InternetCloseHandle(hUrl);
    return pBuffer;
}

/* Download today's picture to the cache file */
BOOL
Bing_Download(VOID)
{
    WCHAR szPath[MAX_PATH], szUrl[1024];
    HINTERNET hInternet, hUrl;
    PSTR pszJson, pszStart, pszEnd;
    BOOL bOk = FALSE;
    INT i, j;

    if (!Bing_GetCachePath(szPath, _countof(szPath)))
        return FALSE;

    hInternet = InternetOpenW(L"ReactOS Display Properties", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet)
        return FALSE;

    pszJson = ReadUrl(hInternet, BING_FEED_URL, 64 * 1024);
    pszStart = pszJson ? strstr(pszJson, "\"url\":\"") : NULL;
    if (pszStart)
    {
        pszStart += 7;
        pszEnd = strchr(pszStart, '"');
        if (pszEnd)
        {
            /* host + path, with the JSON escape "&" turned back into "&" */
            StringCchCopyW(szUrl, _countof(szUrl), BING_HOST);
            j = (INT)wcslen(szUrl);
            for (i = 0; pszStart + i < pszEnd && j < (INT)_countof(szUrl) - 1; i++)
            {
                if (!strncmp(pszStart + i, "\\u0026", 6))
                {
                    szUrl[j++] = L'&';
                    i += 5;
                }
                else
                {
                    szUrl[j++] = (WCHAR)(UCHAR)pszStart[i];
                }
            }
            szUrl[j] = UNICODE_NULL;

            hUrl = InternetOpenUrlW(hInternet, szUrl, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
            if (hUrl)
            {
                HANDLE hFile = CreateFileW(szPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    BYTE Buffer[16 * 1024];
                    DWORD cbRead, cbWritten, cbTotal = 0;
                    while (InternetReadFile(hUrl, Buffer, sizeof(Buffer), &cbRead) && cbRead)
                    {
                        WriteFile(hFile, Buffer, cbRead, &cbWritten, NULL);
                        cbTotal += cbRead;
                    }
                    CloseHandle(hFile);
                    bOk = (cbTotal > 1024);
                    if (!bOk)
                        DeleteFileW(szPath);
                }
                InternetCloseHandle(hUrl);
            }
        }
    }

    if (pszJson)
        HeapFree(GetProcessHeap(), 0, pszJson);
    InternetCloseHandle(hInternet);
    return bOk;
}

BOOL
Bing_IsEnabled(VOID)
{
    WCHAR szValue[8] = L"";
    DWORD cb = sizeof(szValue);
    return SHGetValueW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", BING_REG_VALUE, NULL, szValue, &cb) == ERROR_SUCCESS &&
           szValue[0] == L'1';
}

/* Remember the choice, and refresh the picture at every logon while it is chosen */
VOID
Bing_SetEnabled(BOOL bEnable)
{
    static const WCHAR szRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    SHSetValueW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", BING_REG_VALUE, REG_SZ,
                bEnable ? L"1" : L"0", 2 * sizeof(WCHAR));
    if (bEnable)
        SHSetValueW(HKEY_CURRENT_USER, szRunKey, BING_RUN_VALUE, REG_SZ, BING_RUN_CMD, sizeof(BING_RUN_CMD));
    else
        SHDeleteValueW(HKEY_CURRENT_USER, szRunKey, BING_RUN_VALUE);
}

/* The desktop only displays BMP files: convert and set it */
static BOOL
SetAsWallpaper(LPCWSTR pszImage)
{
    WCHAR szBmp[MAX_PATH];
    GpImage *image = NULL;
    UINT num = 0, size = 0, i;
    ImageCodecInfo *codecs;
    BOOL bOk = FALSE;

    if (FAILED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, szBmp)) ||
        FAILED(StringCchCatW(szBmp, _countof(szBmp), L"\\Wallpaper1.bmp")))
    {
        return FALSE;
    }

    GdipLoadImageFromFile(pszImage, &image);
    if (!image)
        return FALSE;

    GdipGetImageEncodersSize(&num, &size);
    codecs = HeapAlloc(GetProcessHeap(), 0, size);
    if (codecs && GdipGetImageEncoders(num, size, codecs) == Ok)
    {
        for (i = 0; i < num; i++)
        {
            if (!wcscmp(codecs[i].MimeType, L"image/bmp"))
            {
                bOk = GdipSaveImageToFile(image, szBmp, &codecs[i].Clsid, NULL) == Ok;
                break;
            }
        }
    }
    if (codecs)
        HeapFree(GetProcessHeap(), 0, codecs);
    GdipDisposeImage(image);

    if (bOk)
        SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, szBmp, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    return bOk;
}

/* rundll32 desk.cpl,UpdateBingWallpaper (runs at logon while the option is chosen) */
VOID
WINAPI
UpdateBingWallpaper(HWND hwnd, HINSTANCE hInstance, LPSTR pszCmdLine, INT nCmdShow)
{
    WCHAR szPath[MAX_PATH];
    struct GdiplusStartupInput gdipStartup = { 1 };
    ULONG_PTR token;

    if (!Bing_IsEnabled())
        return;

    /* Keep yesterday's picture if the download fails (offline...) */
    Bing_Download();
    if (!Bing_GetCachePath(szPath, _countof(szPath)) || GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
        return;

    if (GdiplusStartup(&token, &gdipStartup, NULL) == Ok)
    {
        SetAsWallpaper(szPath);
        GdiplusShutdown(token);
    }
}
