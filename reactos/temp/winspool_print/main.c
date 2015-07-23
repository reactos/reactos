#include <stdio.h>
#include <windows.h>

int main()
{
    int ReturnValue = 1;
    DWORD dwRead;
    DWORD dwWritten;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hPrinter = NULL;
    DOC_INFO_1W docInfo;
    BYTE Buffer[20000];

    hFile = CreateFileW(L"testfile", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("CreateFileW failed, last error is %lu!\n", GetLastError());
        goto Cleanup;
    }

    if (!ReadFile(hFile, Buffer, sizeof(Buffer), &dwRead, NULL))
    {
        printf("ReadFile failed, last error is %lu!\n", GetLastError());
        goto Cleanup;
    }

    if (!OpenPrinterW(L"Dummy Printer On LPT1", &hPrinter, NULL))
    {
        printf("OpenPrinterW failed, last error is %lu!\n", GetLastError());
        goto Cleanup;
    }

    ZeroMemory(&docInfo, sizeof(docInfo));
    docInfo.pDocName = L"winspool_print";

    if (!StartDocPrinterW(hPrinter, 1, (LPBYTE)&docInfo))
    {
        printf("StartDocPrinterW failed, last error is %lu!\n", GetLastError());
        goto Cleanup;
    }

    if (!StartPagePrinter(hPrinter))
    {
        printf("StartPagePrinter failed, last error is %lu!\n", GetLastError());
        goto Cleanup;
    }

    if (!WritePrinter(hPrinter, Buffer, dwRead, &dwWritten))
    {
        printf("WritePrinter failed, last error is %lu!\n", GetLastError());
        goto Cleanup;
    }

    if (!EndPagePrinter(hPrinter))
    {
        printf("EndPagePrinter failed, last error is %lu!\n", GetLastError());
        goto Cleanup;
    }

    if (!EndDocPrinter(hPrinter))
    {
        printf("EndDocPrinter failed, last error is %lu!\n", GetLastError());
        goto Cleanup;
    }

    ReturnValue = 0;

Cleanup:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    if (hPrinter)
        ClosePrinter(hPrinter);

    return ReturnValue;
}
