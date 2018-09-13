/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    thh.c

Abstract:

    Test program for handle hierarchy

Author:

    Richard L Firth (rfirth) 12-Jan-1996

Revision History:

    12-Jan-1996 rfirth
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

#define OPEN_CONTEXT_VALUE      0x11
#define CONNECT_CONTEXT_VALUE   0x22

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))

void _CRTAPI1 main(int, char**);
void usage(void);
void my_callback(HINTERNET, DWORD, DWORD, LPVOID, DWORD);

BOOL Verbose = FALSE;
DWORD AsyncResult;
DWORD AsyncError;


void _CRTAPI1 main(int argc, char** argv) {

    HINTERNET hInternet;
    HINTERNET hConnect;
    HINTERNET hRequest;
    BOOL callbacks = FALSE;
    INTERNET_STATUS_CALLBACK cbres;

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (*++*argv) {
            case 'c':
                callbacks = TRUE;
                break;

            case 'v':
                Verbose = TRUE;
                break;

            default:
                printf("error: unrecognized command line flag: '%c'\n", **argv);
                usage();
                break;
            }
        } else {
            printf("error: unrecognized command line argument: \"%s\"\n", *argv);
            usage();
        }
    }

    hInternet = InternetOpen("Handle Hierarchy Test Program (thh)",
                             LOCAL_INTERNET_ACCESS,
                             NULL,
                             0,
                             0
                             );
    if (!hInternet) {
        print_error("thh()", "InternetOpen()");
        exit(1);
    }

    if (Verbose) {
        printf("InternetOpen() returns handle %x\n", hInternet);
    }

    if (callbacks) {
        if (Verbose) {
            printf("installing callback\n");
        }

        cbres = InternetSetStatusCallback(hInternet, my_callback);
        if (cbres == INTERNET_INVALID_STATUS_CALLBACK) {
            print_error("thh()", "InternetSetStatusCallback()");
            exit(1);
        } else if (Verbose) {
            printf("previous callback = %x\n", cbres);
        }
    }

    if (Verbose) {
        printf("calling InternetConnect()...\n");
    }

    hConnect = InternetConnect(hInternet,
                               "foo.bar.com",
                               0,
                               "albert einstein",
                               "e=mc2",
                               INTERNET_SERVICE_HTTP,
                               0,
                               callbacks ? CONNECT_CONTEXT_VALUE : 0
                               );
    if (!hInternet) {
        print_error("thh()", "InternetConnect()");
        exit(1);
    } else if (Verbose) {
        printf("InternetConnect() returns handle %x\n", hConnect);
    }

    if (Verbose) {
        printf("calling HttpOpenRequest()...\n");
    }

    hRequest = HttpOpenRequest(hConnect,
                               NULL,    // verb
                               NULL,    // object
                               NULL,    // version
                               NULL,    // referrer
                               NULL,    // accept types
                               0,       // flags
                               callbacks ? OPEN_CONTEXT_VALUE : 0
                               );
    if (!hRequest) {
        print_error("thh()", "HttpOpenRequest()");
        exit(1);
    } else if (Verbose) {
        printf("HttpOpenRequest() returns handle %x\n", hRequest);
    }

    if (Verbose) {
        printf("closing InternetOpen() handle %x\n", hInternet);
    }

    if (InternetCloseHandle(hInternet)) {
        if (Verbose) {
            printf("closed Internet handle %x OK\n", hInternet);
        }
    } else {
        print_error("thh()", "InternetCloseHandle()");
        exit(1);
    }

    printf("Done.\n");
    exit(0);
}

void usage() {
    printf("usage: thh [-c] [-v]\n"
           "where: -c = use callbacks\n"
           "       -v = verbose mode\n"
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
        break;

    case INTERNET_STATUS_HANDLE_CLOSING:
        type$ = "HANDLE CLOSING";
        break;

    case INTERNET_STATUS_REQUEST_COMPLETE:
        type$ = "REQUEST COMPLETE";
        AsyncResult = ((LPINTERNET_ASYNC_RESULT)Info)->dwResult;
        AsyncError = ((LPINTERNET_ASYNC_RESULT)Info)->dwError;
        break;

    default:
        type$ = "???";
        break;
    }
    if (Verbose) {
        printf("callback: H=%x [C=%x [%s]] %s ",
                Handle,
                Context,
                (Context == CONNECT_CONTEXT_VALUE)
                    ? "Connect"
                    : (Context == OPEN_CONTEXT_VALUE)
                        ? "Open   "
                        : "???",
                type$
                );
        if (Info) {
            if ((Status == INTERNET_STATUS_HANDLE_CREATED)
            || (Status == INTERNET_STATUS_HANDLE_CLOSING)) {

                DWORD handleType;
                DWORD handleTypeSize = sizeof(handleType);

                if (InternetQueryOption(*(LPHINTERNET)Info,
                                        INTERNET_OPTION_HANDLE_TYPE,
                                        (LPVOID)&handleType,
                                        &handleTypeSize
                                        )) {
                    switch (handleType) {
                    case INTERNET_HANDLE_TYPE_INTERNET:
                        type$ = "Internet";
                        break;

                    case INTERNET_HANDLE_TYPE_CONNECT_FTP:
                        type$ = "FTP Connect";
                        break;

                    case INTERNET_HANDLE_TYPE_CONNECT_GOPHER:
                        type$ = "Gopher Connect";
                        break;

                    case INTERNET_HANDLE_TYPE_CONNECT_HTTP:
                        type$ = "HTTP Connect";
                        break;

                    case INTERNET_HANDLE_TYPE_FTP_FIND:
                        type$ = "FTP Find";
                        break;

                    case INTERNET_HANDLE_TYPE_FTP_FIND_HTML:
                        type$ = "FTP Find HTML";
                        break;

                    case INTERNET_HANDLE_TYPE_FTP_FILE:
                        type$ = "FTP File";
                        break;

                    case INTERNET_HANDLE_TYPE_FTP_FILE_HTML:
                        type$ = "FTP File HTML";
                        break;

                    case INTERNET_HANDLE_TYPE_GOPHER_FIND:
                        type$ = "Gopher Find";
                        break;

                    case INTERNET_HANDLE_TYPE_GOPHER_FIND_HTML:
                        type$ = "Gopher Find HTML";
                        break;

                    case INTERNET_HANDLE_TYPE_GOPHER_FILE:
                        type$ = "Gopher File";
                        break;

                    case INTERNET_HANDLE_TYPE_GOPHER_FILE_HTML:
                        type$ = "Gopher File HTML";
                        break;

                    case INTERNET_HANDLE_TYPE_HTTP_REQUEST:
                        type$ = "HTTP Request";
                        break;

                    default:
                        type$ = "???";
                        break;
                    }
                } else {
                    type$ = "<error>";
                }
                printf("%x [%s]", *(LPHINTERNET)Info, type$);
            } else if (Status == INTERNET_STATUS_REQUEST_COMPLETE) {

                //
                // nothing
                //

            } else if (Length == sizeof(DWORD)) {
                printf("%d", *(LPDWORD)Info);
            } else {
                printf(Info);
            }
        }
        putchar('\n');
    }
}
