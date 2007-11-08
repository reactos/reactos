
#include <precomp.h>

#define NUM_SETTINGS 6
LPWSTR lpSettings[NUM_SETTINGS] =
{
    L"screen mode id",
    L"desktopwidth",
    L"desktopheight",
    L"session bpp",
    L"full address",
    L"compression",
};

VOID
SaveAllSettings(PINFO pInfo)
{
    INT ret;
    WCHAR szKey[MAXKEY];
    WCHAR szValue[MAXVALUE];

    /* server */
    if (GetDlgItemText(pInfo->hGeneralPage,
                       IDC_SERVERCOMBO,
                       szValue,
                       MAXVALUE))
    {
        SetStringToSettings(pInfo->pRdpSettings,
                            L"full address",
                            szValue);
    }

    /* resolution */
    ret = SendDlgItemMessage(pInfo->hDisplayPage,
                             IDC_GEOSLIDER,
                             TBM_GETPOS,
                             0,
                             0);
    if (ret != -1)
    {
        SetIntegerToSettings(pInfo->pRdpSettings,
                             L"desktopwidth",
                             pInfo->DisplayDeviceList->Resolutions[ret].dmPelsWidth);
        SetIntegerToSettings(pInfo->pRdpSettings,
                             L"desktopheight",
                             pInfo->DisplayDeviceList->Resolutions[ret].dmPelsHeight);
    }

    /* bpp */
    ret = SendDlgItemMessage(pInfo->hDisplayPage,
                             IDC_BPPCOMBO,
                             CB_GETCURSEL,
                             0,
                             0);
    if (ret != CB_ERR)
    {
        ret = SendDlgItemMessage(pInfo->hDisplayPage,
                                 IDC_BPPCOMBO,
                                 CB_GETITEMDATA,
                                 ret,
                                 0);
        if (ret != CB_ERR)
        {
            SetIntegerToSettings(pInfo->pRdpSettings,
                                 L"session bpp",
                                 ret);
        }
    }
}


BOOL
SetIntegerToSettings(PRDPSETTINGS pRdpSettings,
                     LPWSTR lpKey,
                     INT Value)
{
    BOOL bRet = FALSE;

    if (pRdpSettings)
    {
        INT i;

        for (i = 0; i < pRdpSettings->NumSettings; i++)
        {
            if (wcscmp(pRdpSettings->pSettings[i].Key, lpKey) == 0)
            {
                if (pRdpSettings->pSettings[i].Type == L'i')
                {
                    pRdpSettings->pSettings[i].Value.i = Value;
                    bRet = TRUE;
                    break;
                }
            }
        }
    }

    return bRet;
}


BOOL
SetStringToSettings(PRDPSETTINGS pRdpSettings,
                    LPWSTR lpKey,
                    LPWSTR lpValue)
{
    BOOL bRet = FALSE;

    if (pRdpSettings)
    {
        INT i;

        for (i = 0; i < pRdpSettings->NumSettings; i++)
        {
            if (wcscmp(pRdpSettings->pSettings[i].Key, lpKey) == 0)
            {
                if (pRdpSettings->pSettings[i].Type == L's')
                {
                    wcscpy(pRdpSettings->pSettings[i].Value.s, lpValue);
                    bRet = TRUE;
                    break;
                }
            }
        }
    }

    return bRet;
}


INT
GetIntegerFromSettings(PRDPSETTINGS pRdpSettings,
                       LPWSTR lpKey)
{
    INT Value = -1;

    if (pRdpSettings)
    {
        INT i;

        for (i = 0; i < pRdpSettings->NumSettings; i++)
        {
            if (wcscmp(pRdpSettings->pSettings[i].Key, lpKey) == 0)
            {
                if (pRdpSettings->pSettings[i].Type == L'i')
                {
                    Value = pRdpSettings->pSettings[i].Value.i;
                    break;
                }
            }
        }
    }

    return Value;
}


LPWSTR
GetStringFromSettings(PRDPSETTINGS pRdpSettings,
                      LPWSTR lpKey)
{
    LPWSTR lpValue = NULL;

    if (pRdpSettings)
    {
        INT i;

        for (i = 0; i < pRdpSettings->NumSettings; i++)
        {
            if (wcscmp(pRdpSettings->pSettings[i].Key, lpKey) == 0)
            {
                if (pRdpSettings->pSettings[i].Type == L's')
                {
                    lpValue = pRdpSettings->pSettings[i].Value.s;
                    break;
                }
            }
        }
    }

    return lpValue;
}


static BOOL
WriteRdpFile(HANDLE hFile,
             PRDPSETTINGS pRdpSettings)
{
    WCHAR line[MAXKEY + MAXVALUE + 4];
    DWORD BytesToWrite, BytesWritten;
    BOOL bRet;
    INT i, k;

    for (i = 0; i < pRdpSettings->NumSettings; i++)
    {
        /* only write out values in the lpSettings struct */
        for (k = 0; k < NUM_SETTINGS; k++)
        {
            if (wcscmp(lpSettings[k], pRdpSettings->pSettings[i].Key) == 0)
            {
                if (pRdpSettings->pSettings[i].Type == L'i')
                {
                    _snwprintf(line, MAXKEY + MAXVALUE + 4, L"%s:i:%d\r\n",
                               pRdpSettings->pSettings[i].Key,
                               pRdpSettings->pSettings[i].Value.i);
                }
                else
                {
                    _snwprintf(line, MAXKEY + MAXVALUE + 4, L"%s:s:%s\r\n",
                               pRdpSettings->pSettings[i].Key,
                               pRdpSettings->pSettings[i].Value.s);
                }

                BytesToWrite = wcslen(line) * sizeof(WCHAR);

                bRet = WriteFile(hFile,
                                 line,
                                 BytesToWrite,
                                 &BytesWritten,
                                 NULL);
                if (!bRet || BytesWritten == 0)
                    return FALSE;
            }
        }
    }

    return TRUE;
}


static VOID
ParseSettings(PRDPSETTINGS pRdpSettings,
              LPWSTR lpBuffer)
{
    LPWSTR lpStr = lpBuffer;
    WCHAR lpKey[MAXKEY];
    WCHAR lpValue[MAXVALUE];
    INT NumSettings = 0;
    INT s;

    if (lpStr)
    {
        /* get number of settings */
        while (*lpStr)
        {
            if (*lpStr == L'\n')
                NumSettings++;
            lpStr++;
        }
        lpStr = lpBuffer;

        if (NumSettings == 0)
            return;

        /* move past unicode byte order */
        if (lpStr[0] == 0xFEFF || lpStr[0] == 0xFFFE)
            lpStr += 1;

        pRdpSettings->pSettings = HeapAlloc(GetProcessHeap(),
                                            0,
                                            sizeof(SETTINGS) * NumSettings);
        if (pRdpSettings->pSettings)
        {
            pRdpSettings->NumSettings = NumSettings;

            for (s = 0; s < NumSettings; s++)
            {
                INT i = 0, k;

                /* get a key */
                while (*lpStr != L':')
                {
                    lpKey[i++] = *lpStr++;
                }
                lpKey[i] = 0;

                for (k = 0; k < NUM_SETTINGS; k++)
                {
                    if (wcscmp(lpSettings[k], lpKey) == 0)
                    {
                        wcscpy(pRdpSettings->pSettings[s].Key, lpKey);

                        /* get the type */
                        lpStr++;
                        if (*lpStr == L'i' || *lpStr == L's')
                            pRdpSettings->pSettings[s].Type = *lpStr;

                        lpStr += 2;

                        /* get a value */
                        i = 0;
                        while (*lpStr != L'\r')
                        {
                            lpValue[i++] = *lpStr++;
                        }
                        lpValue[i] = 0;

                        if (pRdpSettings->pSettings[s].Type == L'i')
                        {
                            pRdpSettings->pSettings[s].Value.i = _wtoi(lpValue);
                        }
                        else if (pRdpSettings->pSettings[s].Type == L's')
                        {
                            wcscpy(pRdpSettings->pSettings[s].Value.s, lpValue);
                        }
                        else
                            pRdpSettings->pSettings[s].Type = 0;
                    }
                }

                /* move onto next setting */
                while (*lpStr != L'\n')
                {
                    lpStr++;
                }
                lpStr++;
            }
        }
    }
}


static LPWSTR
ReadRdpFile(HANDLE hFile)
{
    LPWSTR lpBuffer = NULL;
    DWORD BytesToRead, BytesRead;
    BOOL bRes;

    if (hFile)
    {
        BytesToRead = GetFileSize(hFile, NULL);
        if (BytesToRead)
        {
            lpBuffer = HeapAlloc(GetProcessHeap(),
                                 0,
                                 BytesToRead + 2);
            if (lpBuffer)
            {
                bRes = ReadFile(hFile,
                                lpBuffer,
                                BytesToRead,
                                &BytesRead,
                                NULL);
                if (bRes)
                {
                    lpBuffer[BytesRead / 2] = 0;
                }
                else
                {
                    HeapFree(GetProcessHeap(),
                             0,
                             lpBuffer);

                    lpBuffer = NULL;
                }
            }
        }
    }

    return lpBuffer;
}


static HANDLE
OpenRdpFile(LPWSTR path, BOOL bWrite)
{
    HANDLE hFile = NULL;

    if (path)
    {
        hFile = CreateFileW(path,
                            bWrite ? GENERIC_WRITE : GENERIC_READ,
                            0,
                            NULL,
                            bWrite ? OPEN_EXISTING: CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN,
                            NULL);
    }

    return hFile;
}


static VOID
CloseRdpFile(HANDLE hFile)
{
    if (hFile)
        CloseHandle(hFile);
}


BOOL
SaveRdpSettingsToFile(LPWSTR lpFile,
                      PRDPSETTINGS pRdpSettings)
{
    WCHAR pszPath[MAX_PATH];
    HANDLE hFile;
    BOOL bRet = FALSE;

    /* use default file */
    if (lpFile == NULL)
    {
#ifndef __REACTOS__
        HRESULT hr;
        LPITEMIDLIST lpidl= NULL;

        hr = SHGetFolderLocation(NULL,
                                 CSIDL_PERSONAL,
                                 NULL,
                                 0,
                                 &lpidl);
        if (hr == S_OK)
        {
            if (SHGetPathFromIDListW(lpidl, pszPath))
            {
                wcscat(pszPath, L"\\Default.rdp");
                lpFile = pszPath;
                CoTaskMemFree(lpidl);
            }
        }
#else
        wcscpy(pszPath, L"C:\\Default.rdp");
        lpFile = pszPath;
#endif
    }

    if (lpFile)
    {
        hFile = OpenRdpFile(lpFile, TRUE);
        if (hFile)
        {
            if (WriteRdpFile(hFile, pRdpSettings))
            {
                bRet = TRUE;
            }

            CloseRdpFile(hFile);
        }
    }

    return bRet;
}


PRDPSETTINGS
LoadRdpSettingsFromFile(LPWSTR lpFile)
{
    PRDPSETTINGS pRdpSettings = NULL;
    WCHAR pszPath[MAX_PATH];
    HANDLE hFile;

    /* use default file */
    if (lpFile == NULL)
    {
#ifndef __REACTOS__
        HRESULT hr;
        LPITEMIDLIST lpidl= NULL;

        hr = SHGetFolderLocation(NULL,
                                 CSIDL_PERSONAL,
                                 NULL,
                                 0,
                                 &lpidl);
        if (hr == S_OK)
        {
            if (SHGetPathFromIDListW(lpidl, pszPath))
            {
                wcscat(pszPath, L"\\Default.rdp");
                lpFile = pszPath;
                CoTaskMemFree(lpidl);
            }
        }
#else
        wcscpy(pszPath, L"C:\\Default.rdp");
        lpFile = pszPath;
#endif
    }

    if (lpFile)
    {
        LPWSTR lpBuffer = NULL;

        pRdpSettings = HeapAlloc(GetProcessHeap(),
                                 0,
                                 sizeof(RDPSETTINGS));
        if (pRdpSettings)
        {
            hFile = OpenRdpFile(lpFile, FALSE);
            if (hFile)
            {
                lpBuffer = ReadRdpFile(hFile);
                if (lpBuffer)
                {
                    ParseSettings(pRdpSettings, lpBuffer);

                    HeapFree(GetProcessHeap(),
                             0,
                             lpBuffer);
                }

                CloseRdpFile(hFile);
            }
        }
    }

    return pRdpSettings;
}
