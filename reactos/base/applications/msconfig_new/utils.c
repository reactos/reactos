#include "precomp.h"
#include "utils.h"
#include "stringutils.h"

// HANDLE g_hHeap = GetProcessHeap();
#define g_hHeap GetProcessHeap()

#if 0
VOID
MemInit(IN HANDLE Heap)
{
    /* Save the heap handle */
    g_hHeap = Heap;
}
#endif

BOOL
MemFree(IN PVOID lpMem)
{
    /* Free memory back into the heap */
    return HeapFree(g_hHeap, 0, lpMem);
}

PVOID
MemAlloc(IN DWORD dwFlags,
         IN DWORD dwBytes)
{
    /* Allocate memory from the heap */
    return HeapAlloc(g_hHeap, dwFlags, dwBytes);
}

LPWSTR
FormatDateTime(IN LPSYSTEMTIME pDateTime)
{
    LPWSTR lpszDateTime = NULL;

    if (pDateTime)
    {
        int iDateBufSize = 0, iTimeBufSize = 0;

        iDateBufSize = GetDateFormatW(LOCALE_USER_DEFAULT,
                                      /* Only for Windows 7 : DATE_AUTOLAYOUT | */ DATE_SHORTDATE,
                                      pDateTime,
                                      NULL,
                                      NULL,
                                      0);
        iTimeBufSize = GetTimeFormatW(LOCALE_USER_DEFAULT,
                                      0,
                                      pDateTime,
                                      NULL,
                                      NULL,
                                      0);

        if ( (iDateBufSize > 0) && (iTimeBufSize > 0) )
        {
            lpszDateTime = (LPWSTR)MemAlloc(0, (iDateBufSize + iTimeBufSize) * sizeof(TCHAR));

            GetDateFormatW(LOCALE_USER_DEFAULT,
                           /* Only for Windows 7 : DATE_AUTOLAYOUT | */ DATE_SHORTDATE,
                           pDateTime,
                           NULL,
                           lpszDateTime,
                           iDateBufSize);
            if (iDateBufSize > 0) lpszDateTime[iDateBufSize-1] = L' ';
            GetTimeFormatW(LOCALE_USER_DEFAULT,
                           0,
                           pDateTime,
                           NULL,
                           lpszDateTime + iDateBufSize,
                           iTimeBufSize);
        }
    }

    return lpszDateTime;
}

VOID
FreeDateTime(IN LPWSTR lpszDateTime)
{
    if (lpszDateTime)
        MemFree(lpszDateTime);
}

LPWSTR
LoadResourceStringEx(IN HINSTANCE hInstance,
                     IN UINT uID,
                     OUT size_t* pSize OPTIONAL)
{
    LPWSTR lpszDestBuf = NULL, lpszResourceString = NULL;
    size_t iStrSize    = 0;

    // When passing a zero-length buffer size, LoadString(...) returns a
    // read-only pointer buffer to the program's resource string.
    iStrSize = LoadString(hInstance, uID, (LPTSTR)&lpszResourceString, 0);

    if ( lpszResourceString && ( (lpszDestBuf = (LPWSTR)MemAlloc(0, (iStrSize + 1) * sizeof(WCHAR))) != NULL ) )
    {
        _tcsncpy(lpszDestBuf, lpszResourceString, iStrSize);
        lpszDestBuf[iStrSize] = L'\0'; // NULL-terminate the string

        if (pSize)
            *pSize = iStrSize + 1;
    }
    else
    {
        if (pSize)
            *pSize = 0;
    }

    return lpszDestBuf;
}

LPWSTR
LoadConditionalResourceStringEx(IN HINSTANCE hInstance,
                                IN BOOL bCondition,
                                IN UINT uIDifTrue,
                                IN UINT uIDifFalse,
                                IN size_t* pSize OPTIONAL)
{
    return LoadResourceStringEx(hInstance,
                                (bCondition ? uIDifTrue : uIDifFalse),
                                pSize);
}

LPWSTR
GetExecutableVendor(IN LPCWSTR lpszFilename)
{
    LPWSTR lpszVendor = NULL;
    DWORD dwHandle = 0;
    DWORD dwLen;

    LPVOID lpData;

    LPVOID pvData = NULL;
    UINT BufLen = 0;
    WORD wCodePage = 0, wLangID = 0;
    LPTSTR lpszStrFileInfo = NULL;
    
    LPWSTR lpszData = NULL;

    if (lpszFilename == NULL) return NULL;

    dwLen = GetFileVersionInfoSizeW(lpszFilename, &dwHandle);
    if (dwLen == 0) return NULL;

    lpData = MemAlloc(0, dwLen);
    if (!lpData) return NULL;

    GetFileVersionInfoW(lpszFilename, dwHandle, dwLen, lpData);

    if (VerQueryValueW(lpData, L"\\VarFileInfo\\Translation", &pvData, &BufLen))
    {
        wCodePage = LOWORD(*(DWORD*)pvData);
        wLangID   = HIWORD(*(DWORD*)pvData);

        lpszStrFileInfo = FormatString(L"StringFileInfo\\%04X%04X\\CompanyName",
                                       wCodePage,
                                       wLangID);
    }

    VerQueryValueW(lpData, lpszStrFileInfo, (LPVOID*)&lpszData, &BufLen);
    if (lpszData)
    {
        lpszVendor = (LPWSTR)MemAlloc(0, BufLen * sizeof(WCHAR));
        if (lpszVendor)
            wcscpy(lpszVendor, lpszData);
    }
    else
    {
        lpszVendor = NULL;
    }

    MemFree(lpszStrFileInfo);
    MemFree(lpData);

    return lpszVendor;
}
