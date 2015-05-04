#include <stdio.h>
#include <windows.h>

int main()
{
    int ReturnValue = 1;
    DWORD dwWritten;
    HANDLE hPrinter = NULL;
    DOC_INFO_1W docInfo;
    char szString[] = "winspool_print Test\f";

    if (!OpenPrinterW(L"Generic / Text Only", &hPrinter, NULL))
    {
        printf("OpenPrinterW failed\n");
        goto Cleanup;
    }

    ZeroMemory(&docInfo, sizeof(docInfo));
    docInfo.pDocName = L"winspool_print";

    if (!StartDocPrinterW(hPrinter, 1, (LPBYTE)&docInfo))
    {
        printf("StartDocPrinterW failed, last error is %u!\n", GetLastError());
        goto Cleanup;
    }

    if (!StartPagePrinter(hPrinter))
    {
        printf("StartPagePrinter failed, last error is %u!\n", GetLastError());
        goto Cleanup;
    }

    if (!WritePrinter(hPrinter, szString, strlen(szString), &dwWritten))
    {
        printf("WritePrinter failed, last error is %u!\n", GetLastError());
        goto Cleanup;
    }

    if (!EndPagePrinter(hPrinter))
    {
        printf("EndPagePrinter failed, last error is %u!\n", GetLastError());
        goto Cleanup;
    }

    if (!EndDocPrinter(hPrinter))
    {
        printf("EndDocPrinter failed, last error is %u!\n", GetLastError());
        goto Cleanup;
    }

    ReturnValue = 0;

Cleanup:
    if (hPrinter)
        ClosePrinter(hPrinter);

    return ReturnValue;
}
