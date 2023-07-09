/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ftpclose.c

Abstract:

    Tests FTP open/read/close. Main purpose is to make sure we do the right
    thing with ABOR

Author:

    Richard L Firth (rfirth) 11-Oct-1995

Revision History:

    11-Oct-1995 rfirth
        Created

--*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wininet.h>

#ifndef _CRTAPI1
#define _CRTAPI1
#endif

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))

#define DEFAULT_BUFFER_SIZE 1024

void _CRTAPI1 main(int, char**);
void usage(void);

BOOL Verbose = FALSE;

void _CRTAPI1 main(int argc, char** argv) {

    HINTERNET hInternet;
    HINTERNET hFtpSession;
    HINTERNET hFile;
    LPSTR server = NULL;
    LPSTR username = NULL;
    LPSTR password = NULL;
    LPSTR filename = NULL;
    LPBYTE buffer;
    int buflen = DEFAULT_BUFFER_SIZE;
    DWORD bytesRead;
    DWORD totalBytes;
    DWORD closeAfter = 0xffffffff;
    DWORD accessMethod = INTERNET_OPEN_TYPE_PRECONFIG;
    BOOL expectingProxyServer = FALSE;
    LPSTR proxyServer = NULL;

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (*++*argv) {
            case '?':
                usage();
                break;

            case 'a':
                ++*argv;
                if (**argv == 'p') {
                    accessMethod = INTERNET_OPEN_TYPE_PROXY;
                    if (*++*argv) {
                        proxyServer = *argv;
                    } else {
                        expectingProxyServer = TRUE;
                    }
                } else if (**argv == 'l') {
                    accessMethod = INTERNET_OPEN_TYPE_DIRECT;
                } else {
                    if (**argv) {
                        printf("error: unrecognised access type: '%c'\n", **argv);
                    } else {
                        printf("error: missing access type\n");
                    }
                    usage();
                }
                break;

            case 'b':
                buflen = atoi(++*argv);
                break;

            case 'f':
                filename = ++*argv;
                break;

            case 'n':
                closeAfter = (DWORD)atoi(++*argv);
                break;

            case 'p':
                password = ++*argv;
                break;

            case 's':
                server = ++*argv;
                break;

            case 'u':
                username = ++*argv;
                break;

            case 'v':
                Verbose = TRUE;
                break;

            default:
                printf("error: unrecognized command line flag: '%c'\n", **argv);
                usage();
            }
        } else if (expectingProxyServer) {
            proxyServer = *argv;
            expectingProxyServer = FALSE;
        }
    }

    if (!server) {
        printf("error: must supply server name\n");
        usage();
    }

    if (!filename) {
        printf("error: must supply file name\n");
        usage();
    }

    hInternet = InternetOpen("ftpclose",
                             accessMethod,
                             proxyServer,
                             NULL,
                             0);
    if (!hInternet) {
        printf("error: InternetOpen() returns %d\n", GetLastError());
        exit(1);
    } else if (Verbose) {
        printf("opened Internet handle %x\n", hInternet);
    }

    hFtpSession = InternetConnect(hInternet,
                                  server,
                                  INTERNET_INVALID_PORT_NUMBER,
                                  username,
                                  password,
                                  INTERNET_SERVICE_FTP,
                                  0,
                                  0
                                  );
    if (!hFtpSession) {
        printf("error: InternetConnect() returns %d\n", GetLastError());
        InternetCloseHandle(hInternet);
        exit(1);
    } else if (Verbose) {
        printf("opened FTP connect handle %x\n", hFtpSession);
    }

    hFile = FtpOpenFile(hFtpSession,
                        filename,
                        GENERIC_READ,
                        FTP_TRANSFER_TYPE_BINARY,
                        0,
                        0
                        );
    if (!hFile) {
        printf("error: FtpOpenFile(%s) returns %d\n", filename, GetLastError());
        InternetCloseHandle(hFtpSession);
        InternetCloseHandle(hInternet);
        exit(1);
    } else if (Verbose) {
        printf("opened FTP File handle %x\n", hFile);
    }

    buffer = (LPBYTE)malloc(buflen);
    if (!buffer) {
        printf("error: failed to allocate %u bytes\n", buflen);
        InternetCloseHandle(hFile);
        InternetCloseHandle(hFtpSession);
        InternetCloseHandle(hInternet);
        exit(1);
    }

    if (closeAfter == 0) {
        if (Verbose) {
            printf("not reading file (close after 0)\n");
        }
    } else {
        if (Verbose) {
            printf("reading file %s", filename);
            if (closeAfter != 0xffffffff) {
                printf(" closing after %d bytes", closeAfter);
            }
            putchar('\n');
        }

        totalBytes = 0;
        while (InternetReadFile(hFile, buffer, buflen, &bytesRead)) {
            if (Verbose) {
                printf("read %d bytes\n", bytesRead);
            }
            if (bytesRead == 0) {
                break;
            }
            totalBytes += bytesRead;
            if (totalBytes >= closeAfter) {
                break;
            }
        }

        if (GetLastError() != ERROR_SUCCESS) {
            printf("error: InternetReadFile() returns %d\n", GetLastError());
            InternetCloseHandle(hFile);
            InternetCloseHandle(hFtpSession);
            InternetCloseHandle(hInternet);
            exit(1);
        }

        if (Verbose) {
            printf("%d bytes read\n", totalBytes);
        }
    }

    InternetCloseHandle(hFile);
    InternetCloseHandle(hFtpSession);
    InternetCloseHandle(hInternet);

    exit(0);
}

void usage() {
    printf("\n"
           "usage: ftpclose [-a{l|p}] [-b#] [-v] [-n#] <-sserver> [-uuser] [-ppassword]\n"
           "                <-ffile>\n"
           "\n"
           "where:  -a = Access mode: l = local; p = proxy\n"
           "        -b = Buffer size. Default is %d\n"
           "        -n = Number of bytes to read before closing file. Default is all\n"
           "        -p = Password (with -u)\n"
           "        -s = FTP server\n"
           "        -u = User name\n"
           "        -v = Verbose mode\n",
           DEFAULT_BUFFER_SIZE
           );
    exit(1);
}
