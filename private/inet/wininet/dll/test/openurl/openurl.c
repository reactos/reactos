/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    openurl.c

Abstract:

    Tests InternetOpenUrl()

Author:

    Richard L Firth (rfirth) 29-May-1995

Revision History:

    29-May-1995 rfirth
        Created

--*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <wininet.h>
#include <catlib.h>

#ifndef _CRTAPI1
#define _CRTAPI1
#endif

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))

#define VERSION_STRING  "1.0"

#define URL_CONTEXT             0x55785875  // UxXu
#define DEFAULT_BUFFER_LENGTH   1021        // odd number for fun!

//
// prototypes
//

void _CRTAPI1 main(int, char**);
void usage(void);
void _CRTAPI1 my_cleanup(void);
void my_callback(HINTERNET, DWORD, DWORD, LPVOID, DWORD);
void default_url_test(LPSTR, DWORD, DWORD);
void open_urls(LPSTR*, int, LPSTR, DWORD, DWORD);
void get_url_data(HINTERNET);
void ftp_find(HINTERNET);
void gopher_find(HINTERNET);
void read_data(HINTERNET);
void get_request_flags(HINTERNET);

//
// data
//

BOOL Verbose = FALSE;
HINTERNET InternetHandle = NULL;
INTERNET_STATUS_CALLBACK PreviousCallback;
HINTERNET hCancel;
BOOL AsyncMode = FALSE;
HANDLE AsyncEvent = NULL;
DWORD AsyncResult;
DWORD AsyncError;
BOOL UseQueryData = FALSE;
BOOL NoDump = FALSE;
DWORD BufferLength = DEFAULT_BUFFER_LENGTH;
DWORD ReadLength = DEFAULT_BUFFER_LENGTH;

LPSTR default_urls[] = {

    //
    // WEB
    //

    "http://www.microsoft.com",
    "http://www.microsoft.com/pages/misc/whatsnew.htm",

    //
    // gopher
    //

    "gopher://gopher.microsoft.com",
    "gopher://gopher.microsoft.com/11/msft/",
    "gopher://gopher.microsoft.com/00\\welcome.txt",
    "gopher://gopher.tc.umn.edu/11Information%20About%20Gopher%09%09%2B",
    "gopher://spinaltap.micro.umn.edu/11/computer",
    "gopher://mudhoney.micro.umn.edu:4325/7",
    "gopher://mudhoney.micro.umn.edu:4325/7%09gopher",
    "gopher://spinaltap.micro.umn.edu/7mindex:lotsoplaces%09gopher%09%2b",

    //
    // FTP
    //

    "ftp://ftp.microsoft.com",
    "ftp://ftp.microsoft.com/MSNBRO.TXT",
    "ftp://ftp.microsoft.com/Services/"
};

#define NUMBER_OF_DEFAULT_URLS  (sizeof(default_urls)/sizeof(default_urls[0]))

//
// functions
//

void _CRTAPI1 main(int argc, char** argv) {

    BOOL ok;
    LPSTR urls[64];
    int numberOfUrls = 0;
    BOOL fCallback = FALSE;
    LPSTR headers = NULL;
    BOOL expectingHeaders = FALSE;
    DWORD accessMethod = INTERNET_OPEN_TYPE_PRECONFIG;
    LPSTR proxyServer = NULL;
    BOOL expectingProxy = FALSE;
    DWORD context = 0;
    DWORD flags = 0;
    LPSTR endptr;

    printf("\n"
           "OpenUrl  Version " VERSION_STRING "  " __DATE__ "\n"
           "\n"
           );

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (tolower(*++*argv)) {
            case '?':
                usage();
                break;

            case 'a':
                ++*argv;
                if (**argv == 'l') {
                    accessMethod = INTERNET_OPEN_TYPE_DIRECT;
                } else if (**argv == 'p') {
                    accessMethod = INTERNET_OPEN_TYPE_PROXY;
                    if (*++*argv) {
                        proxyServer = *argv;
                    } else {
                        expectingProxy = TRUE;
                    }
                } else {
                    printf("error: unrecognised access type: '%c'\n", **argv);
                    usage();
                }
                break;

            case 'b':
                BufferLength = (DWORD)strtol(++*argv, &endptr, 0);
                if (*endptr == 'K' || *endptr == 'k') {
                    BufferLength *= 1024;
                }
                break;

            case 'c':
                fCallback = TRUE;
                break;

            case 'd':
                NoDump = TRUE;
                break;

            case 'e':
                flags |= INTERNET_FLAG_EXISTING_CONNECT;
                break;

            case 'h':
                if (*++*argv) {
                    headers = *argv;
                } else {
                    expectingHeaders = TRUE;
                }
                break;

            case 'l':
                ReadLength = (DWORD)strtol(++*argv, &endptr, 0);
                if (*endptr == 'K' || *endptr == 'k') {
                    ReadLength *= 1024;
                }
                break;

            case 'n':
                flags |= INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE;
                break;

            case 'p':
                flags |= INTERNET_FLAG_PASSIVE;
                break;

            case 'q':
                UseQueryData = TRUE;
                break;

            case 'r':
                flags |= INTERNET_FLAG_RAW_DATA;
                break;

            case 'v':
                Verbose = TRUE;
                break;

            case 'x':
                context = URL_CONTEXT;
                break;

            case 'y':
                AsyncMode = TRUE;
                break;

            default:
                printf("unknown command line flag: '%c'\n", **argv);
                usage();
            }
        } else if (expectingProxy) {
            proxyServer = *argv;
            expectingProxy = FALSE;
        } else if (expectingHeaders) {
            headers = *argv;
            expectingHeaders = FALSE;
        } else {
            if (numberOfUrls == sizeof(urls)/sizeof(urls[0]) - 1) {
                break;
            }
            urls[numberOfUrls++] = *argv;
        }
    }

    if (BufferLength < ReadLength) {
        BufferLength = ReadLength;
    }

    //
    // exit function
    //

    atexit(my_cleanup);

    if (AsyncMode) {

        //
        // create an auto-reset, initially unsignalled event
        //

        AsyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!AsyncEvent) {
            print_error("OpenUrl", "CreateEvent()");
            exit(1);
        }
    }

    //
    // get a handle to the internet - local, gateway or CERN proxy
    //

    InternetHandle = InternetOpen("OpenUrl",
                                  accessMethod,
                                  proxyServer,
                                  NULL,
                                  AsyncMode ? INTERNET_FLAG_ASYNC : 0
                                  );
    if (InternetHandle == NULL) {
        print_error("openurl()", "InternetOpen()");
        exit(1);
    }

    if (Verbose) {
        printf("InternetOpen() returns handle %x\n", InternetHandle);
    }

    //
    // let's have a status callback
    //

    if (fCallback) {
        PreviousCallback = InternetSetStatusCallback(InternetHandle, my_callback);
        if (Verbose) {
            printf("previous Internet callback = %x\n", PreviousCallback);
        }
    }

    if (numberOfUrls == 0) {
        default_url_test(headers, flags, context);
    } else {
        open_urls(urls, numberOfUrls, headers, flags, context);
    }

    if (Verbose) {
        printf("closing InternetHandle (%x)\n", InternetHandle);
    }
    ok = InternetCloseHandle(InternetHandle);
    if (!ok) {
        print_error("openurl()", "InternetClose(%x)", InternetHandle);
        exit(1);
    } else {
        InternetHandle = NULL;
    }

    printf("Done.\n");
    exit(0);
}

void usage() {
    printf("\n"
           "usage: openurl [-a{l|p}[[ ]{server[:port]}] [-b#] [-c] [-d] [-h[ ]{headers}]\n"
           "               [-l#] [-n] [-p] [-q] [-r] [-v] [-x] [-y] [url]*\n"
           "\n"
           "where:  -al = local internet access\n"
           "        -ap = CERN proxy internet access\n"
           "         -b = buffer size\n"
           "         -c = enable status callbacks\n"
           "         -d = don't dump data\n"
           "         -e = use existing connection\n"
           "         -h = headers\n"
           "         -l = read length\n"
           "         -n = don't use cache\n"
           "         -p = PASSIVE mode (FTP transfers)\n"
           "         -q = use InternetQueryDataAvailable\n"
           "         -r = raw data. Default is HTML for FTP and gopher directories\n"
           "         -v = Verbose mode\n"
           "         -x = use context value when calling InternetOpenUrl()\n"
           "         -y = Async mode\n"
           "\n"
           "     server = gateway server or proxy server name\n"
           "      :port = (optional) CERN proxy port\n"
           "        url = one or more URLs to open\n"
           "\n"
           "Default internet access is pre-configured (i.e. use settings in registry)\n"
           );
    exit(1);
}

void _CRTAPI1 my_cleanup() {
    if (InternetHandle != NULL) {
        if (Verbose) {
            printf("closing Internet handle %x\n", InternetHandle);
        }
        if (!InternetCloseHandle(InternetHandle)) {
            print_error("my_cleanup", "InternetCloseHandle(%x)", InternetHandle);
        }
    }
}

VOID
my_callback(
    HINTERNET hInternet,
    DWORD Context,
    DWORD Status,
    LPVOID Info,
    DWORD Length
    )
{
    char* type$;
    DWORD handleType;
    DWORD size;

    size = sizeof(handleType);
    if (!InternetQueryOption(hInternet,
                             INTERNET_OPTION_HANDLE_TYPE,
                             (LPVOID)&handleType,
                             &size)) {
        print_error("my_callback", "InternetQueryOption(HANDLE_TYPE)");
    } else {
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
            type$ = "Internet";
            break;

        case INTERNET_HANDLE_TYPE_HTTP_REQUEST:
            type$ = "HTTP Request";
            break;

        default:
            type$ = "???";
        }
        printf("callback: handle %x = %s\n", hInternet, type$);
    }

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
        hCancel = *(LPHINTERNET)Info;
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
        printf("callback: handle %x [context %x [%s]] %s ",
                hInternet,
                Context,
                (Context == URL_CONTEXT) ? "UrlContext" : "???",
                type$
                );
        if (Info) {
            if ((Status == INTERNET_STATUS_HANDLE_CREATED)
            || (Status == INTERNET_STATUS_HANDLE_CLOSING)) {
                printf("%x", *(LPHINTERNET)Info);
            } else if (Length == sizeof(DWORD)) {
                printf("%d", *(LPDWORD)Info);
            } else if (Status != INTERNET_STATUS_REQUEST_COMPLETE) {
                printf(Info);
            }
        }
        putchar('\n');
    }
    if (Status == INTERNET_STATUS_REQUEST_COMPLETE) {
        if (AsyncMode) {
            SetEvent(AsyncEvent);
        } else {
            printf("error: INTERNET_STATUS_REQUEST_COMPLETE received when not async\n");
        }
    }
}

void default_url_test(LPSTR headers, DWORD flags, DWORD context) {
    open_urls(default_urls, NUMBER_OF_DEFAULT_URLS, headers, flags, context);
}

void open_urls(LPSTR* purls, int nurls, LPSTR headers, DWORD flags, DWORD context) {

    HINTERNET handle;

    if (headers) {

        LPSTR h;

        for (h = headers; *h; ++h) {
            if (*h == '\\' && *(h + 1) == 'n') {
                *h++ = '\r';
                *h = '\n';
            }
        }
    }
    while (nurls--) {
        if (Verbose) {
            printf("\nopening URL \"%s\"\n\n", *purls);
        }
        handle = InternetOpenUrl(InternetHandle,
                                 *purls,
                                 headers,
                                 headers ? -1 : 0,
                                 flags,
                                 context
                                 );
        if ((handle == NULL) && AsyncMode) {

            DWORD err;

            err = GetLastError();
            if (err == ERROR_IO_PENDING) {
                if (Verbose) {
                    printf("waiting for async InternetOpenUrl()...\n");
                }
                WaitForSingleObject(AsyncEvent, INFINITE);
                handle = (HINTERNET)AsyncResult;
                SetLastError(AsyncError);
            }
        } else if (AsyncMode && Verbose) {
            printf("async InternetOpenUrl() returns sync result\n");
        }

        if (handle == NULL) {
            print_error("open_urls", "InternetOpenUrl(%s)", *purls);
        } else {
            if (Verbose) {
                printf("InternetOpenUrl() returns handle %x\n", handle);
            }
            get_request_flags(handle);
            get_url_data(handle);
        }

        ++purls;
    }
}

void get_url_data(HINTERNET handle) {

    DWORD handleType;
    DWORD handleTypeLen;

    handleTypeLen = sizeof(handleType);
    if (InternetQueryOption(handle,
                            INTERNET_OPTION_HANDLE_TYPE,
                            (LPVOID)&handleType,
                            &handleTypeLen
                            )) {
        switch (handleType) {
        case INTERNET_HANDLE_TYPE_INTERNET:
            printf("error: get_url_data: HANDLE_TYPE_INTERNET?\n");
            break;

        case INTERNET_HANDLE_TYPE_CONNECT_FTP:
            printf("error: get_url_data: INTERNET_HANDLE_TYPE_CONNECT_FTP?\n");
            break;

        case INTERNET_HANDLE_TYPE_CONNECT_GOPHER:
            printf("error: get_url_data: INTERNET_HANDLE_TYPE_CONNECT_GOPHER?\n");
            break;

        case INTERNET_HANDLE_TYPE_CONNECT_HTTP:
            printf("error: get_url_data: INTERNET_HANDLE_TYPE_CONNECT_HTTP?\n");
            break;

        case INTERNET_HANDLE_TYPE_FTP_FIND:
            ftp_find(handle);
            break;

        case INTERNET_HANDLE_TYPE_GOPHER_FIND:
            gopher_find(handle);
            break;

        case INTERNET_HANDLE_TYPE_FTP_FIND_HTML:
        case INTERNET_HANDLE_TYPE_FTP_FILE:
        case INTERNET_HANDLE_TYPE_FTP_FILE_HTML:
        case INTERNET_HANDLE_TYPE_GOPHER_FIND_HTML:
        case INTERNET_HANDLE_TYPE_GOPHER_FILE:
        case INTERNET_HANDLE_TYPE_GOPHER_FILE_HTML:
        case INTERNET_HANDLE_TYPE_HTTP_REQUEST:
            read_data(handle);
            break;

        default:
            printf("error: get_url_data: handleType == %d?\n", handleType);
            break;
        }
        if (Verbose) {
            printf("closing Internet handle %x\n", handle);
        }
        if (!InternetCloseHandle(handle)) {
            print_error("get_url_data", "InternetCloseHandle(%x)", handle);
        }
    } else {
        print_error("get_url_data", "InternetQueryOption()");
    }
}

void ftp_find(HINTERNET handle) {

    WIN32_FIND_DATA data;
    BOOL ok;

    do {

        SYSTEMTIME systemTime;

        ok = InternetFindNextFile(handle, (LPVOID)&data);
        if (!ok) {
            if (AsyncMode) {
                if (GetLastError() == ERROR_IO_PENDING) {
                    if (Verbose) {
                        printf("waiting for async InternetFindNextFile()...\n");
                    }
                    WaitForSingleObject(AsyncEvent, INFINITE);
                    ok = (BOOL)AsyncResult;
                    SetLastError(AsyncError);
                }
            }
        }
        if (ok && !NoDump) {
            if (!FileTimeToSystemTime(&data.ftLastWriteTime, &systemTime)) {
                print_error("ftp_find", "FileTimeToSystemTime()");
            }

            printf("%2d-%02d-%04d %2d:%02d:%02d  %15d bytes %-s%-s%-s %s\n",
                   systemTime.wMonth,
                   systemTime.wDay,
                   systemTime.wYear,
                   systemTime.wHour,
                   systemTime.wMinute,
                   systemTime.wSecond,
                   data.nFileSizeLow,
                   (data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
                    ? "Normal    " : "",
                   (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
                    ? "ReadOnly  " : "",
                   (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    ? "Directory " : "",
                   data.cFileName
                   );
        }
    } while (ok);

    if (GetLastError() != ERROR_NO_MORE_FILES) {
        print_error("ftp_find", "InternetFindNextFile()");
    }
}

void gopher_find(HINTERNET handle) {

    GOPHER_FIND_DATA data;
    BOOL ok;
    int i;

    i = 0;
    do {
        ok = InternetFindNextFile(handle, (LPVOID)&data);
        if (!ok) {
            if (AsyncMode) {
                if (GetLastError() == ERROR_IO_PENDING) {
                    if (Verbose) {
                        printf("waiting for async InternetFindNextFile()...\n");
                    }
                    WaitForSingleObject(AsyncEvent, INFINITE);
                    ok = (BOOL)AsyncResult;
                    SetLastError(AsyncError);
                }
            }
        }
        if (ok && !NoDump) {

            LPGOPHER_FIND_DATA p;
            SYSTEMTIME systemTime;
            char timeBuf[9];
            char sizeBuf[32];

            p = (LPGOPHER_FIND_DATA)&data;
            if ((p->LastModificationTime.dwLowDateTime != 0)
            && (p->LastModificationTime.dwHighDateTime != 0)) {
                FileTimeToSystemTime(&p->LastModificationTime, &systemTime);
                sprintf(timeBuf,
                        "%02d-%02d-%02d",
                        systemTime.wMonth,
                        systemTime.wDay,
                        systemTime.wYear % 100
                        );
                sprintf(sizeBuf, "%d", p->SizeLow);
            } else {
                timeBuf[0] = '\0';
                sizeBuf[0] = '\0';
            }
            printf("%5d %c %7s %10s %8s %s\n",
                    i,
                    (p->GopherType & GOPHER_TYPE_GOPHER_PLUS) ? '+' : ' ',
                    (p->GopherType & GOPHER_TYPE_TEXT_FILE)         ? "Text"
                    : (p->GopherType & GOPHER_TYPE_DIRECTORY)       ? "Dir"
                    : (p->GopherType & GOPHER_TYPE_CSO)             ? "Phone"
                    : (p->GopherType & GOPHER_TYPE_ERROR)           ? "Error"
                    : (p->GopherType & GOPHER_TYPE_MAC_BINHEX)      ? "MAC"
                    : (p->GopherType & GOPHER_TYPE_DOS_ARCHIVE)     ? "Archive"
                    : (p->GopherType & GOPHER_TYPE_UNIX_UUENCODED)  ? "UNIX"
                    : (p->GopherType & GOPHER_TYPE_INDEX_SERVER)    ? "Index"
                    : (p->GopherType & GOPHER_TYPE_TELNET)          ? "Telnet"
                    : (p->GopherType & GOPHER_TYPE_BINARY)          ? "Binary"
                    : (p->GopherType & GOPHER_TYPE_REDUNDANT)       ? "Backup"
                    : (p->GopherType & GOPHER_TYPE_TN3270)          ? "TN3270"
                    : (p->GopherType & GOPHER_TYPE_GIF)             ? "GIF"
                    : (p->GopherType & GOPHER_TYPE_IMAGE)           ? "Image"
                    : (p->GopherType & GOPHER_TYPE_BITMAP)          ? "Bitmap"
                    : (p->GopherType & GOPHER_TYPE_MOVIE)           ? "Movie"
                    : (p->GopherType & GOPHER_TYPE_SOUND)           ? "Sound"
                    : (p->GopherType & GOPHER_TYPE_HTML)            ? "HTML"
                    : (p->GopherType & GOPHER_TYPE_PDF)             ? "PDF"
                    : (p->GopherType & GOPHER_TYPE_CALENDAR)        ? "Cal"
                    : (p->GopherType & GOPHER_TYPE_INLINE)          ? "Inline"
                    : (p->GopherType & GOPHER_TYPE_UNKNOWN)         ? "Unknown"
                    : "\a????",
                    sizeBuf,
                    timeBuf,
                    p->DisplayString
                    );
            ++i;
        }
    } while (ok);

    if (GetLastError() != ERROR_NO_MORE_FILES) {
        print_error("gopher_find", "InternetFindNextFile()");
    }
}

void read_data(HINTERNET handle) {

    char* buf;
    DWORD nread;
    BOOL ok;
    int mode;

    buf = (char*)malloc(BufferLength);
    if (!buf) {
        printf("error: failed to allocate %d bytes for buffer\n", BufferLength);
        return;
    }

    mode = _setmode(1, _O_BINARY);

    do {

        DWORD avail;
        int i;

        if (UseQueryData) {
            ok = InternetQueryDataAvailable(handle, &avail, 0, 0);
            if (!ok) {
                if (GetLastError() == ERROR_IO_PENDING) {
                    if (Verbose) {
                        printf("waiting for async InternetQueryDataAvailable()...\n");
                    }
                    WaitForSingleObject(AsyncEvent, INFINITE);
                    ok = (BOOL)AsyncResult;
                    SetLastError(AsyncError);
                }
            }
            if (!ok) {
                print_error("read_file", "InternetQueryDataAvailable()");
                break;
            }

            if (Verbose) {
                printf("InternetQueryDataAvailable() returns %d bytes\n", avail);
            }
        } else {
            avail = BufferLength;
        }

        avail = min(avail, ReadLength);

        for (i = 0; i < 2; ++i) {
            memset(buf, '@', avail);
            ok = InternetReadFile(handle, buf, avail, &nread);
            if (!ok) {
                if (AsyncMode) {
                    if (GetLastError() == ERROR_IO_PENDING) {
                        if (Verbose) {
                            printf("waiting for async InternetFindNextFile()...\n");
                        }
                        WaitForSingleObject(AsyncEvent, INFINITE);
                        ok = (BOOL)AsyncResult;
                        SetLastError(AsyncError);
                    }
                } else {
                    break;
                }
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                    if (i == 1) {
                        printf("error: failed to read %d bytes in 2 attempts\n",
                            nread);
                        goto quit;
                    }

                    //
                    // second attempt with all buffer
                    //

                    avail = BufferLength;
                } else {
                    break;
                }
            }
        }
        if (ok && !NoDump) {
            if (!nread) {
                printf("=== end of file ===\n");
            } else {
                _write(1, buf, nread);
            }
        } else if (ok && NoDump && Verbose) {
            printf("InternetReadFile() returns %d bytes\n", nread);
        }
    } while (ok && nread);

    if (GetLastError() != ERROR_SUCCESS) {
        print_error("read_file", "InternetReadFile()");
    }

quit:

    free(buf);

    _setmode(1, mode);
}

void get_request_flags(HINTERNET hInternet) {

    DWORD dwFlags;
    DWORD len = sizeof(dwFlags);

    if (InternetQueryOption(hInternet,
                            INTERNET_OPTION_REQUEST_FLAGS,
                            &dwFlags,
                            &len)) {

        char buf[256];
        char * p = buf;

        p += sprintf(p, "REQUEST_FLAGS = %08x\n", dwFlags);
        p += sprintf(p, "\tRetrieved from:  %s\n",
            (dwFlags & INTERNET_REQFLAG_FROM_CACHE) ? "Cache" : "Network");
        p += sprintf(p, "\tNo Headers:      %s\n",
            (dwFlags & INTERNET_REQFLAG_NO_HEADERS) ? "TRUE" : "FALSE");
        p += sprintf(p, "\tVia Proxy:       %s\n",
            (dwFlags & INTERNET_REQFLAG_VIA_PROXY) ? "YES" : "NO");
        printf(buf);
    } else {
        print_error("get_request_flags()", "InternetQueryOption()");
    }
}
