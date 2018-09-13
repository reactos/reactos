#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define CRTTONT(fh) (HANDLE)_get_osfhandle(fh)

PrintPair(
    LPWSTR pwch,
    UCHAR *pch,
    DWORD cch)
{
    DWORD cchWritten;

    SetConsoleMode(CRTTONT(STDOUT), ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);

    printf("%04x-%04x: ", pwch[0], pwch[cch-1]);
    WriteConsoleW(CRTTONT(STDOUT), pwch, cch, &cchWritten, NULL);
    printf("  ");
    printf("%02x-%02x: ", pch[0], pch[cch-1]);
    WriteConsoleA(CRTTONT(STDOUT), pch, cch, &cchWritten, NULL);

    printf("\n");
    SetConsoleMode(CRTTONT(STDOUT), ENABLE_WRAP_AT_EOL_OUTPUT);

    printf("%04x-%04x: ", pwch[0], pwch[cch-1]);
    WriteConsoleW(CRTTONT(STDOUT), pwch, cch, &cchWritten, NULL);
    printf("  ");
    printf("%02x-%02x: ", pch[0], pch[cch-1]);
    WriteConsoleA(CRTTONT(STDOUT), pch, cch, &cchWritten, NULL);

    SetConsoleMode(CRTTONT(STDOUT), ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
    printf("\n\n");
}

int _cdecl main(int argc, char *argv[]) {
    WCHAR awch[100];
    CHAR  ach[100];
    CHAR  ch;
    DWORD i, j;

    printf("The first and second line of each pair should be identical:\n");
    printf("Unicode                       ANSI\n");
    for (i = 0xA0; i < 0xFF; i += 0x10) {
        for (j = 0; j < 16; j++) {
            awch[j] = (WCHAR)(i + j);
            ach[j] = (CHAR)(i+j);
        }
        PrintPair(awch, ach, 16);
    }

    printf("Now test line wrapping:\n");
    ch = 0x80;
    for (j = 0; j < 100; j++) {
        MultiByteToWideChar(CP_ACP, 0, &ch, 1, &awch[j], 1);
        ach[j] = ch;
        ch++;
    }
    PrintPair(awch, ach, 100);

}