#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wininet.h>
#include <catlib.h>

#ifndef _CRTAPI1
#define _CRTAPI1
#endif

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))

void _CRTAPI1 main(int, char**);
void usage(void);

BOOL Verbose = FALSE;

void _CRTAPI1 main(int argc, char** argv) {

    LPSTR lpszUrl = NULL;
    HINTERNET hInternet;
    HINTERNET hRequest;
    char buf[256];
    DWORD buflen = sizeof(buf);

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (*++*argv) {
            case 'v':
                Verbose = TRUE;
                break;

            default:
                printf("error: unrecognized command line flag: '%c'\n", **argv);
                usage();
                break;
            }
        } else if (!lpszUrl) {
            lpszUrl = *argv;
        } else {
            printf("error: unrecognized command line argument: \"%s\"\n", *argv);
            usage();
        }
    }

    hInternet = InternetOpen("contype", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        print_error("contype()", "InternetOpen()");
        exit(1);
    }
    hRequest = InternetOpenUrl(hInternet, lpszUrl, "Accept: */*", -1, 0, 0);
    if (!hRequest) {
        print_error("contype()", "InternetOpenUrl()");
        exit(1);
    }

    if (!HttpQueryInfo(hRequest, HTTP_QUERY_CONTENT_TYPE, buf, &buflen, NULL)) {
        print_error("contype()", "HttpQueryInfo()");
        exit(1);
    }

    printf("URL \"%s\": Content-Type: \"%s\"\n", lpszUrl, buf);

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hInternet);
    exit(0);
}

void usage() {
    printf("usage: contype\n"
           );
    exit(1);
}
