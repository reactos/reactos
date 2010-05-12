
#include <precomp.h>

/* update NUM_SETTINGS in precomp.h */
LPWSTR lpSettings[NUM_SETTINGS] =
{
    L"desktopwidth",
    L"desktopheight",
    L"session bpp",
    L"full address",
};

VOID
SaveAllSettings(PINFO pInfo)
{
    INT ret;
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
                if (pRdpSettings->pSettings[i].Type == 0)
                    pRdpSettings->pSettings[i].Type = L'i';

                pRdpSettings->pSettings[i].Value.i = Value;
                bRet = TRUE;
                break;
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
                if (pRdpSettings->pSettings[i].Type == 0)
                    pRdpSettings->pSettings[i].Type = L's';

                wcscpy(pRdpSettings->pSettings[i].Value.s, lpValue);
                bRet = TRUE;
                break;
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
    WCHAR szSeps[] = L":\r\n";
    LPWSTR lpToken;
    BOOL bFound;
    INT i;

    /* move past unicode byte order */
    if (lpStr[0] == 0xFEFF || lpStr[0] == 0xFFFE)
        lpStr += 1;

    lpToken = wcstok(lpStr, szSeps);
    while (lpToken)
    {
        bFound = FALSE;

        for (i = 0; i < pRdpSettings->NumSettings && !bFound; i++)
        {
            if (wcscmp(lpToken, pRdpSettings->pSettings[i].Key) == 0)
            {
                lpToken = wcstok(NULL, szSeps);
                if (lpToken[0] == L'i')
                {
                    pRdpSettings->pSettings[i].Type = lpToken[0];
                    lpToken = wcstok(NULL, szSeps);
                    if (lpToken != NULL)
                        pRdpSettings->pSettings[i].Value.i = _wtoi(lpToken);
                }
                else if (lpToken[0] == L's')
                {
                    pRdpSettings->pSettings[i].Type = lpToken[0];
                    lpToken = wcstok(NULL, szSeps);
                    if (lpToken != NULL)
                        wcscpy(pRdpSettings->pSettings[i].Value.s, lpToken);
                }
                bFound = TRUE;
            }
        }

        /* move past the type and value */
        if (!bFound)
        {
            lpToken = wcstok(NULL, szSeps);
            lpToken = wcstok(NULL, szSeps);
        }

        /* move to next key */
        lpToken = wcstok(NULL, szSeps);
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
                            bWrite ? CREATE_ALWAYS : OPEN_EXISTING,
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


BOOL
LoadRdpSettingsFromFile(PRDPSETTINGS pRdpSettings,
                        LPWSTR lpFile)
{
    WCHAR pszPath[MAX_PATH];
    HANDLE hFile;
    BOOL bRet = FALSE;

    /* use default file */
    if (lpFile == NULL)
    {
#ifndef __REACTOS__  // remove when this is working
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
                
                bRet = TRUE;
            }

            CloseRdpFile(hFile);
        }
    }

    return bRet;
}


BOOL
InitRdpSettings(PRDPSETTINGS pRdpSettings)
{
    BOOL bRet = FALSE;

    pRdpSettings->pSettings = HeapAlloc(GetProcessHeap(),
                                        0,
                                        sizeof(SETTINGS) * NUM_SETTINGS);
    if (pRdpSettings->pSettings)
    {
        INT i;

        for (i = 0; i < NUM_SETTINGS; i++)
        {
            wcscpy(pRdpSettings->pSettings[i].Key, lpSettings[i]);
            pRdpSettings->pSettings[i].Type = (WCHAR)0;
            pRdpSettings->pSettings[i].Value.i = 0;
        }

        pRdpSettings->NumSettings = NUM_SETTINGS;

        bRet = TRUE;
    }

    return bRet;
}
