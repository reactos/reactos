#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>

#include "resource.h"

static void PrintError(void)
{
    DWORD dwError;
    LPWSTR lpMsgBuf = NULL;

    dwError = GetLastError();
    if (dwError == NO_ERROR)
        return;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL, dwError, 0, (LPWSTR)&lpMsgBuf, 0,  NULL);
    wprintf(L"%s", lpMsgBuf);
    LocalFree(lpMsgBuf);
}

static void PrintResourceString(UINT uID)
{
    WCHAR buff[500];

    if (LoadStringW(GetModuleHandle(NULL), uID, buff, ARRAYSIZE(buff)))
    {
        wprintf(L"%s", buff);
    }
}

static BOOL IsDataUnicode(HGLOBAL hGlobal)
{
    BOOL bReturn;
    LPVOID lpBuffer;

    lpBuffer = GlobalLock(hGlobal);
    bReturn = IsTextUnicode(lpBuffer, GlobalSize(hGlobal), NULL);
    GlobalUnlock(hGlobal);

    return bReturn;
}

int wmain(int argc, wchar_t** argv)
{
    HANDLE hInput;
    DWORD dwBytesRead;
    BOOL bStatus;
    HGLOBAL hBuffer;
    HGLOBAL hTemp;
    LPBYTE lpBuffer;
    SIZE_T dwSize = 0;

    hInput = GetStdHandle(STD_INPUT_HANDLE);

    /* Check for usage */
    if (argc > 1 && wcsncmp(argv[1], L"/?", 2) == 0)
    {
        PrintResourceString(IDS_HELP);
        return 0;
    }

    if (GetFileType(hInput) == FILE_TYPE_CHAR)
    {
        PrintResourceString(IDS_USAGE);
        return 0;
    }

    /* Initialize a growable buffer for holding clipboard data */
    hBuffer = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 4096);
    if (!hBuffer)
    {
        PrintError();
        return -1;
    }

    /*
     * Read data from the input stream by chunks of 4096 bytes
     * and resize the buffer each time when needed.
     */
    do
    {
        lpBuffer = GlobalLock(hBuffer);
        if (!lpBuffer)
            goto cleanup;

        bStatus = ReadFile(hInput, lpBuffer + dwSize, 4096, &dwBytesRead, NULL);
        dwSize += dwBytesRead;
        GlobalUnlock(hBuffer);

        hTemp = GlobalReAlloc(hBuffer, GlobalSize(hBuffer) + 4096, GMEM_ZEROINIT);
        if (!hTemp)
            goto cleanup;

        hBuffer = hTemp;
    }
    while (dwBytesRead > 0 && bStatus);

    /*
     * Resize the buffer to the total size of data read.
     * Note that, if the call fails, we still have the old buffer valid.
     * The old buffer would be larger than the actual size of data it contains,
     * but this is not a problem for us.
     */
    hTemp = GlobalReAlloc(hBuffer, dwSize + sizeof(WCHAR), GMEM_ZEROINIT);
    if (hTemp)
        hBuffer = hTemp;

    /* Attempt to open the clipboard */
    if (!OpenClipboard(NULL))
        goto cleanup;

    /* Empty it, copy our data, then close it */

    EmptyClipboard();

    if (IsDataUnicode(hBuffer))
    {
        SetClipboardData(CF_UNICODETEXT, hBuffer);
    }
    else
    {
        // TODO: Convert text from current console page to standard ANSI.
        // Alternatively one can use CF_OEMTEXT as done here.
        SetClipboardData(CF_OEMTEXT, hBuffer);
    }

    CloseClipboard();
    return 0;

cleanup:
    GlobalFree(hBuffer);
    PrintError();
    return -1;
}
