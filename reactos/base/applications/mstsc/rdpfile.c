#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>
#include <shlobj.h>
#include "resource.h"

#define MAXKEY 256
#define MAXVALUE 256

typedef struct _Settings
{
    WCHAR Key[MAXKEY];
    WCHAR Type; // holds 'i' or 's'
    union {
        INT i;
        WCHAR s[MAXVALUE];
    } Value;
} SETTINGS, *PSETTINGS;

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


static BOOL
WriteRdpFile(HANDLE hFile,
             PSETTINGS pSettings)
{

}


static PSETTINGS
ParseSettings(LPWSTR lpBuffer)
{
    PSETTINGS pSettings;
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
            return NULL;

        /* move past unicode byte order */
        if (lpStr[0] == 0xFEFF || lpStr[0] == 0xFFFE)
            lpStr += 1;

        pSettings = HeapAlloc(GetProcessHeap(),
                              0,
                              sizeof(SETTINGS) * NumSettings);
        if (pSettings)
        {
            for (s = 0; s < NumSettings; s++)
            {
                INT i = 0, k, temp;

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
                        wcscpy(pSettings[s].Key, lpKey);

                        /* get the type */
                        lpStr++;
                        if (*lpStr == L'i' || *lpStr == L's')
                            pSettings[s].Type = *lpStr;

                        lpStr += 2;

                        /* get a value */
                        i = 0;
                        while (*lpStr != L'\r')
                        {
                            lpValue[i++] = *lpStr++;
                        }
                        lpValue[i] = 0;

                        if (pSettings[s].Type == L'i')
                        {
                            pSettings[s].Value.i = _wtoi(lpValue);
                            pSettings[s].Value.s[0] = 0;
                        }
                        else if (pSettings[s].Type == L's')
                        {
                            pSettings[s].Value.i = 0;
                            wcscpy(pSettings[s].Value.s, lpValue);
                        }
                        else
                            pSettings[s].Type = 0;
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

    return pSettings;
}

static LPWSTR
ReadRdpFile(HANDLE hFile)
{
    LPWSTR lpBuffer;
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
    HANDLE hFile;

    if (path)
    {
        hFile = CreateFileW(path,
                            bWrite ? GENERIC_WRITE : GENERIC_READ,
                            0,
                            NULL,
                            bWrite ? CREATE_ALWAYS : OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
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


PSETTINGS
LoadRdpSettingsFromFile(LPWSTR lpFile)
{
    PSETTINGS pSettings = NULL;
    WCHAR pszPath[MAX_PATH];
    HANDLE hFile;

    /* use default file */
    if (lpFile == NULL)
    {
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
            }
        }
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
                pSettings = ParseSettings(lpBuffer);

                HeapFree(GetProcessHeap(),
                         0,
                         lpBuffer);
            }

            CloseRdpFile(hFile);
        }
    }

    return pSettings;
}
