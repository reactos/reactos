/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ftpszcls.cxx

Abstract:

    Tests FTP abort during FtpGetFileSize(). Ensure we overcome IE5 bug #71219

Author:

    Richard L Firth (rfirth) 09-Feb-1999

Revision History:

    09-Feb-1999 rfirth
        Created

--*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wininet.h>
#include <catlib.h>

#ifndef _CRTAPI1
#define _CRTAPI1
#endif

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))
#define SESSION_CONTEXT 0x01010101
#define FILE_CONTEXT    0x02020202

void _CRTAPI1 main(int, char**);
void usage(void);
VOID
my_callback(
    HINTERNET Handle,
    DWORD Context,
    DWORD Status,
    LPVOID Info,
    DWORD Length
    );

BOOL Verbose = FALSE;

void _CRTAPI1 main(int argc, char** argv) {

    HINTERNET hInternet;
    HINTERNET hFtpSession;
    HINTERNET hFile;
    char server[INTERNET_MAX_HOST_NAME_LENGTH + 1];
    char username[INTERNET_MAX_USER_NAME_LENGTH];
    char password[INTERNET_MAX_PASSWORD_LENGTH];
    LPSTR filename = NULL;
    DWORD accessMethod = INTERNET_OPEN_TYPE_PRECONFIG;
    BOOL expectingProxyServer = FALSE;
    LPSTR proxyServer = NULL;
    LPSTR pszUrl = NULL;

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (*++*argv) {
            case '?':
                usage();

            case 'a':
                ++*argv;
                if (**argv == 'p') {
                    accessMethod = INTERNET_OPEN_TYPE_PROXY;
                } else if (**argv == 'd') {
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
        } else if (!pszUrl) {
            pszUrl = *argv;
        }
    }

    if (!pszUrl) {
        printf("error: must supply file URL\n");
        usage();
    }

    URL_COMPONENTS urlbits;

    memset(&urlbits, 0, sizeof(urlbits));
    urlbits.dwStructSize = sizeof(urlbits);
    urlbits.lpszHostName = (LPSTR)server;
    urlbits.dwHostNameLength = (DWORD)-1;
    urlbits.lpszUserName = (LPSTR)username;
    urlbits.dwUserNameLength = (DWORD)-1;
    urlbits.lpszPassword = (LPSTR)password;
    urlbits.dwPasswordLength = (DWORD)-1;
    urlbits.dwUrlPathLength = (DWORD)-1;

    BOOL ok;

    ok = InternetCrackUrl(pszUrl, (DWORD)-1, 0, &urlbits);
    if (!ok) {
        print_error("ftpszcls()", "InternetCrackUrl()");
        exit(1);
    }

    hInternet = InternetOpen("ftpszcls",
                             accessMethod,
                             proxyServer,
                             NULL,
                             0);
    if (!hInternet) {
        print_error("ftpszcls()", "InternetOpen()");
        exit(1);
    } else if (Verbose) {
        printf("opened Internet handle %#x\n", hInternet);
    }

    hFtpSession = InternetConnect(hInternet,
                                  server,
                                  INTERNET_INVALID_PORT_NUMBER,
                                  username,
                                  password,
                                  INTERNET_SERVICE_FTP,
                                  0,
                                  SESSION_CONTEXT
                                  );
    if (!hFtpSession) {
        print_error("ftpszcls()", "InternetConnect()");
        InternetCloseHandle(hInternet);
        exit(1);
    } else if (Verbose) {
        printf("opened FTP connect handle %#x\n", hFtpSession);
    }

    filename = urlbits.lpszUrlPath;

    hFile = FtpOpenFile(hFtpSession,
                        filename,
                        GENERIC_READ,
                        FTP_TRANSFER_TYPE_BINARY,
                        FILE_CONTEXT
                        );
    if (!hFile) {
        print_error("ftpszcls()", "FtpOpenFile(%s)", filename);
        InternetCloseHandle(hFtpSession);
        InternetCloseHandle(hInternet);
        exit(1);
    } else if (Verbose) {
        printf("opened FTP File handle %#x\n", hFile);
    }

    InternetSetStatusCallback(hInternet, my_callback);

    DWORD dwFileSizeLow, dwFileSizeHigh;

    //
    // force TLS handle value to be not FTP file handle object
    //

    HINTERNET hInternet2;

    hInternet2 = InternetOpen("ftpszcls",
                              INTERNET_OPEN_TYPE_PRECONFIG,
                              NULL,
                              NULL,
                              0
                              );
    InternetCloseHandle(hInternet2);

    //
    // get the size of the FTP file. Buggy wininet now has TLS handle and
    // mapped handle values set to hInternet2
    //

    dwFileSizeLow = FtpGetFileSize(hFile, &dwFileSizeHigh);

    if (dwFileSizeLow == (DWORD)-1) {
        print_error("ftpszcls()", "FtpGetFileSize()");
        exit(1);
    } else if (Verbose) {
        printf("size of \"%s\" is %d\n", pszUrl, dwFileSizeLow);
    }

    InternetCloseHandle(hFile);
    InternetCloseHandle(hFtpSession);
    InternetCloseHandle(hInternet);

    printf("Done.\n");
    exit(0);
}

void usage() {
    printf("\n"
           "usage: ftpszcls [-a{d|p}] [-v] fileURL\n"
           "\n"
           "where:  -a = Access mode: d = direct; p = proxy. Default is pre-config\n"
           "        -v = Verbose mode\n"
           );
    exit(1);
}

VOID
my_callback(
    HINTERNET Handle,
    DWORD Context,
    DWORD Status,
    LPVOID Info,
    DWORD Length
    )
{
    char* type$;

    switch (Status) {
    case INTERNET_STATUS_RESOLVING_NAME:
        type$ = "RESOLVING NAME";
        break;

    case INTERNET_STATUS_NAME_RESOLVED:
        type$ = "NAME RESOLVED";
        break;

    case INTERNET_STATUS_CONNECTING_TO_SERVER:
        type$ = "CONNECTING TO SERVER";
        break;

    case INTERNET_STATUS_CONNECTED_TO_SERVER:
        type$ = "CONNECTED TO SERVER";
        break;

    case INTERNET_STATUS_SENDING_REQUEST:
        type$ = "SENDING REQUEST";
        break;

    case INTERNET_STATUS_REQUEST_SENT:
        type$ = "REQUEST SENT";
        break;

    case INTERNET_STATUS_RECEIVING_RESPONSE:
        type$ = "RECEIVING RESPONSE";
        break;

    case INTERNET_STATUS_RESPONSE_RECEIVED:
        type$ = "RESPONSE RECEIVED";
        break;

    case INTERNET_STATUS_CLOSING_CONNECTION:
        type$ = "CLOSING CONNECTION";
        break;

    case INTERNET_STATUS_CONNECTION_CLOSED:
        type$ = "CONNECTION CLOSED";
        break;

    case INTERNET_STATUS_HANDLE_CREATED:
        type$ = "HANDLE CREATED";
        //hCancel = *(LPHINTERNET)Info;
        break;

    case INTERNET_STATUS_HANDLE_CLOSING:
        type$ = "HANDLE CLOSING";
        break;

    case INTERNET_STATUS_REQUEST_COMPLETE:
        type$ = "REQUEST COMPLETE";
        //AsyncResult = ((LPINTERNET_ASYNC_RESULT)Info)->dwResult;
        //AsyncError = ((LPINTERNET_ASYNC_RESULT)Info)->dwError;
        break;

    default:
        type$ = "???";
        break;
    }
    if (Verbose) {
        printf("callback: handle %x [context %x [%s]] %s ",
                Handle,
                Context,
                (Context == SESSION_CONTEXT) ? "Session"
                : (Context == FILE_CONTEXT) ? "File"
                : "???",
                type$
                );
        if (Info) {
            if ((Status == INTERNET_STATUS_HANDLE_CREATED)
            || (Status == INTERNET_STATUS_HANDLE_CLOSING)) {
                printf("%x", *(LPHINTERNET)Info);
            } else if (Length == sizeof(DWORD)) {
                printf("%d", *(LPDWORD)Info);
            } else {
                printf((LPSTR)Info);
            }
        }
        putchar('\n');
    }
    //if (Status == INTERNET_STATUS_REQUEST_COMPLETE) {
    //    get_response(Handle);
    //    if (AsyncMode) {
    //        SetEvent(AsyncEvent);
    //    } else {
    //        printf("error: INTERNET_STATUS_REQUEST_COMPLETE returned. Not async\n");
    //    }
    //}
}
