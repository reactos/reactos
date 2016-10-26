#include <stdio.h>
#include <windows.h>

void Usage(WCHAR* name)
{
    wprintf(L"Usage: %s testfile\n", name);
}

int wmain(int argc, WCHAR* argv[])
{
    int ReturnValue = 1;
    DWORD dwFileSize;
    DWORD dwRead, dwWritten;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hPrinter = NULL;
    DOC_INFO_1W docInfo;
    BYTE Buffer[4096];

    if (argc <= 1)
    {
        Usage(argv[0]);
        return 0;
    }

    hFile = CreateFileW(argv[1], GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("CreateFileW failed, last error is %lu!\n", GetLastError());
        goto Cleanup;
    }

    dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == INVALID_FILE_SIZE)
    {
        printf("File is too big, or GetFileSize failed; last error is %lu!\n", GetLastError());
        goto Cleanup;
    }

    if (!OpenPrinterW(L"Dummy Printer On LPT1", &hPrinter, NULL))
    {
        printf("OpenPrinterW failed, last error is %lu!\n", GetLastError());
        goto Cleanup;
    }

    /* Print to a printer, with the "RAW" datatype (pDatatype == NULL or "RAW") */
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

    while (dwFileSize > 0)
    {
        dwRead = min(sizeof(Buffer), dwFileSize);
        if (!ReadFile(hFile, Buffer, dwRead, &dwRead, NULL))
        {
            printf("ReadFile failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
        dwFileSize -= dwRead;

        if (!WritePrinter(hPrinter, Buffer, dwRead, &dwWritten))
        {
            printf("WritePrinter failed, last error is %lu!\n", GetLastError());
            goto Cleanup;
        }
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
    if (hPrinter)
        ClosePrinter(hPrinter);

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return ReturnValue;
}
