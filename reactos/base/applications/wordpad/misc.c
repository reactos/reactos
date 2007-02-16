#include "precomp.h"

static INT
LengthOfStrResource(IN HINSTANCE hInst,
                    IN UINT uID)
{
    HRSRC hrSrc;
    HGLOBAL hRes;
    LPWSTR lpName, lpStr;

    if (hInst == NULL)
    {
        return -1;
    }

    /* There are always blocks of 16 strings */
    lpName = (LPWSTR)MAKEINTRESOURCE((uID >> 4) + 1);

    /* Find the string table block */
    if ((hrSrc = FindResourceW(hInst, lpName, (LPWSTR)RT_STRING)) &&
        (hRes = LoadResource(hInst, hrSrc)) &&
        (lpStr = (WCHAR*) LockResource(hRes)))
    {
        UINT x;

        /* Find the string we're looking for */
        uID &= 0xF; /* position in the block, same as % 16 */
        for (x = 0; x < uID; x++)
        {
            lpStr += (*lpStr) + 1;
        }

        /* Found the string */
        return (int)(*lpStr);
    }
    return -1;
}

INT
AllocAndLoadString(OUT LPTSTR *lpTarget,
                   IN HINSTANCE hInst,
                   IN UINT uID)
{
    INT ln;

    ln = LengthOfStrResource(hInst,
                             uID);
    if (ln++ > 0)
    {
        (*lpTarget) = (LPTSTR)LocalAlloc(LMEM_FIXED,
                                         ln * sizeof(TCHAR));
        if ((*lpTarget) != NULL)
        {
            INT Ret;
            if (!(Ret = LoadString(hInst, uID, *lpTarget, ln)))
            {
                LocalFree((HLOCAL)(*lpTarget));
            }
            return Ret;
        }
    }
    return 0;
}

DWORD
LoadAndFormatString(IN HINSTANCE hInstance,
                    IN UINT uID,
                    OUT LPTSTR *lpTarget,
                    ...)
{
    DWORD Ret = 0;
    LPTSTR lpFormat;
    va_list lArgs;

    if (AllocAndLoadString(&lpFormat,
                           hInstance,
                           uID) > 0)
    {
        va_start(lArgs, lpTarget);
        /* let's use FormatMessage to format it because it has the ability to allocate
           memory automatically */
        Ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                            lpFormat,
                            0,
                            0,
                            (LPTSTR)lpTarget,
                            0,
                            &lArgs);
        va_end(lArgs);

        LocalFree((HLOCAL)lpFormat);
    }

    return Ret;
}

BOOL
StatusBarLoadAndFormatString(IN HWND hStatusBar,
                             IN INT PartId,
                             IN HINSTANCE hInstance,
                             IN UINT uID,
                             ...)
{
    BOOL Ret = FALSE;
    LPTSTR lpFormat, lpStr;
    va_list lArgs;

    if (AllocAndLoadString(&lpFormat,
                           hInstance,
                           uID) > 0)
    {
        va_start(lArgs, uID);
        /* let's use FormatMessage to format it because it has the ability to allocate
           memory automatically */
        Ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                            lpFormat,
                            0,
                            0,
                            (LPTSTR)&lpStr,
                            0,
                            &lArgs);
        va_end(lArgs);

        if (lpStr != NULL)
        {
            Ret = (BOOL)SendMessage(hStatusBar,
                                    SB_SETTEXT,
                                    (WPARAM)PartId,
                                    (LPARAM)lpStr);
            LocalFree((HLOCAL)lpStr);
        }

        LocalFree((HLOCAL)lpFormat);
    }

    return Ret;
}

BOOL
StatusBarLoadString(IN HWND hStatusBar,
                    IN INT PartId,
                    IN HINSTANCE hInstance,
                    IN UINT uID)
{
    BOOL Ret = FALSE;
    LPTSTR lpStr;

    if (AllocAndLoadString(&lpStr,
                           hInstance,
                           uID) > 0)
    {
        Ret = (BOOL)SendMessage(hStatusBar,
                                SB_SETTEXT,
                                (WPARAM)PartId,
                                (LPARAM)lpStr);
        LocalFree((HLOCAL)lpStr);
    }

    return Ret;
}


INT
GetTextFromEdit(OUT LPTSTR lpString,
                IN HWND hDlg,
                IN UINT Res)
{
    INT len = GetWindowTextLength(GetDlgItem(hDlg, Res));
    if(len > 0)
    {
        GetDlgItemText(hDlg,
                       Res,
                       lpString,
                       len + 1);
    }
    else
        lpString = NULL;

    return len;
}


VOID GetError(DWORD err)
{
    LPVOID lpMsgBuf;

    if (err == 0)
        err = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  err,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMsgBuf,
                  0,
                  NULL );

    MessageBox(NULL, (LPCTSTR) lpMsgBuf, _T("Error!"), MB_OK | MB_ICONERROR);

    LocalFree(lpMsgBuf);
}

