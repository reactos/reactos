/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cernprox.c

Abstract:

    Tests CERN proxy support

Author:

    Richard L Firth (rfirth) 28-Jun-1995

Revision History:

    28-Jun-1995 rfirth
        Created

--*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <wininet.h>
#include <winsock.h>

#ifndef _CRTAPI1
#define _CRTAPI1
#endif

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))

//
// prototypes
//

void _CRTAPI1 main(int, char**);
void usage(void);
void _CRTAPI1 my_cleanup(void);
void my_callback(DWORD, DWORD, LPVOID, DWORD);
void default_url_test(void);
void open_urls(LPSTR*, int);
void get_url_data(HINTERNET);
void ftp_find(HINTERNET);
void gopher_find(HINTERNET);
void read_data(HINTERNET);
void print_error(char*, char*, ...);
char* map_error(DWORD);
void get_last_internet_error(void);

//
// data
//

BOOL Verbose = FALSE;
HINTERNET InternetHandle = NULL;
INTERNET_STATUS_CALLBACK PreviousCallback;
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
    INTERNET_PORT proxyPort = 0;
    LPSTR proxy = NULL;

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (tolower(*++*argv)) {
            case 'c':
                fCallback = TRUE;
                break;

            case 'p':
                proxyPort = (INTERNET_PORT)atoi(++*argv);
                break;

            case 'v':
                Verbose = TRUE;
                break;

            default:
                printf("unknown command line flag: '%c'\n", **argv);
                usage();
            }
        } else if (proxy == NULL) {
            proxy = *argv;
        } else {
            if (numberOfUrls == sizeof(urls)/sizeof(urls[0]) - 1) {
                break;
            }
            urls[numberOfUrls++] = *argv;
        }
    }

    //
    // exit function
    //

    atexit(my_cleanup);

    //
    // let's have a status callback
    //

    if (fCallback) {
        PreviousCallback = InternetSetStatusCallback(my_callback);
        if (Verbose) {
            printf("previous Internet callback = %x\n", PreviousCallback);
        }
    }

    //
    // open gateway
    //

    InternetHandle = InternetOpen("cernprox",
                                  CERN_PROXY_INTERNET_ACCESS,
                                  proxy,
                                  proxyPort,
                                  0
                                  );
    if (InternetHandle == NULL) {
        printf("error: cernprox: InternetOpen() returns %d\n", GetLastError());
        exit(1);
    }

    if (numberOfUrls == 0) {
        default_url_test();
    } else {
        open_urls(urls, numberOfUrls);
    }

    ok = InternetCloseHandle(InternetHandle);
    if (!ok) {
        printf("error: cernprox: InternetClose(InternetHandle) returns %d\n", GetLastError());
        exit(1);
    } else {
        InternetHandle = NULL;
    }

    printf("Done.\n");
    exit(0);
}

void usage() {
    printf("usage: cernprox [-c] [-v] <proxy> [-p#] [url]*\n"
           "where:  -c = enable status call-backs\n"
           "        -p = port for CERN proxy\n"
           "        -v = Verbose mode\n"
           "     proxy = CERN proxy server\n"
           "       url = one or more URLs to open\n"
           );
    exit(1);
}

void _CRTAPI1 my_cleanup() {
    if (InternetHandle != NULL) {
        printf("closing Internet handle %x\n", InternetHandle);
        if (!InternetCloseHandle(InternetHandle)) {
            print_error("my_cleanup", "InternetCloseHandle()");
        }
    }
}

VOID
my_callback(
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

    default:
        type$ = "???";
        break;
    }
    printf("callback: %s ", type$);
    if (Info) {
        printf(Info);
    }
    putchar('\n');
}

void default_url_test() {
    open_urls(default_urls, NUMBER_OF_DEFAULT_URLS);
}

void open_urls(LPSTR* purls, int nurls) {

    HINTERNET handle;

    while (nurls--) {
        printf("\nopening URL \"%s\"\n\n", *purls);
        handle = InternetOpenUrl(InternetHandle,
                                 *purls,
                                 NULL,
                                 0,
                                 0,
                                 0
                                 );
        if (handle == NULL) {
            print_error("open_urls", "InternetOpenUrl(%s)", *purls);
        } else {
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

        if (handleType != INTERNET_HANDLE_TYPE_HTTP_REQUEST) {
            printf("error: get_url_data: handle type %d returned, should be %d\n",
                    handleType,
                    INTERNET_HANDLE_TYPE_HTTP_REQUEST
                    );
        }

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
        if (!InternetCloseHandle(handle)) {
            print_error("get_url_data", "InternetCloseHandle()");
        }
    } else {
        print_error("get_url_data", "InternetQueryOption()");
    }
}

void ftp_find(HINTERNET handle) {

    WIN32_FIND_DATA ffd;

    while (InternetFindNextFile(handle, (LPVOID)&ffd)) {

        SYSTEMTIME stDbg;

        if (!FileTimeToSystemTime(&ffd.ftLastWriteTime, &stDbg)) {
            printf("| ftLastWriteTime = ERROR\n");
        }

        printf("%2d-%02d-%04d %2d:%02d:%02d  %15d bytes %-s%-s%-s %s\n",
               stDbg.wMonth, stDbg.wDay, stDbg.wYear,
               stDbg.wHour, stDbg.wMinute, stDbg.wSecond,
               ffd.nFileSizeLow,
               (ffd.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)    ? "Normal    " : "",
               (ffd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)  ? "ReadOnly  " : "",
               (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "Directory " : "",
               ffd.cFileName
               );
    }
    if (GetLastError() != ERROR_NO_MORE_FILES) {
        print_error("ftp_find", "InternetFindNextFile()");
    }
}

void gopher_find(HINTERNET handle) {

    GOPHER_FIND_DATA data;
    int i;

    i = 0;
    while (InternetFindNextFile(handle, (LPVOID)&data)) {

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
    if (GetLastError() != ERROR_NO_MORE_FILES) {
        print_error("gopher_find", "InternetFindNextFile()");
    }
}

void read_data(HINTERNET handle) {

    char buf[1021]; // odd number for fun!
    DWORD nread;

    while (InternetReadFile(handle, buf, sizeof(buf), &nread)) {
        if (!nread) {
            printf("=== end of file ===\n");
            break;
        } else {
            setmode(1, _O_BINARY);
            write(1, buf, nread);
        }
    }
    if (GetLastError() != ERROR_SUCCESS) {
        print_error("read_file", "InternetReadFile()");
    }
}

void print_error(char* func, char* format, ...) {

    va_list argptr;
    char buf[256];
    DWORD error;

    error = GetLastError();
    va_start(argptr, format);
    vsprintf(buf, format, argptr);
    printf("error: %s: %s returns %d [%s]\n", func, buf, error, map_error(error));
    va_end(argptr);
    if (error == ERROR_INTERNET_EXTENDED_ERROR) {
        get_last_internet_error();
    }
}

#define ERROR_CASE(error)  case error: return # error

char* map_error(DWORD error) {
    switch (error) {

    //
    // Windows base errors
    //

    ERROR_CASE(ERROR_SUCCESS);
    ERROR_CASE(ERROR_INVALID_FUNCTION);
    ERROR_CASE(ERROR_FILE_NOT_FOUND);
    ERROR_CASE(ERROR_PATH_NOT_FOUND);
    ERROR_CASE(ERROR_ACCESS_DENIED);
    ERROR_CASE(ERROR_INVALID_HANDLE);
    ERROR_CASE(ERROR_NOT_ENOUGH_MEMORY);
    ERROR_CASE(ERROR_NO_MORE_FILES);
    ERROR_CASE(ERROR_INVALID_PASSWORD);
    ERROR_CASE(ERROR_INVALID_PARAMETER);
    ERROR_CASE(ERROR_BUFFER_OVERFLOW);
    ERROR_CASE(ERROR_NO_MORE_SEARCH_HANDLES);
    ERROR_CASE(ERROR_INVALID_TARGET_HANDLE);
    ERROR_CASE(ERROR_CALL_NOT_IMPLEMENTED);
    ERROR_CASE(ERROR_INSUFFICIENT_BUFFER);
    ERROR_CASE(ERROR_INVALID_NAME);
    ERROR_CASE(ERROR_INVALID_LEVEL);
    ERROR_CASE(ERROR_BAD_PATHNAME);
    ERROR_CASE(ERROR_BUSY);
    ERROR_CASE(ERROR_ALREADY_EXISTS);
    ERROR_CASE(ERROR_FILENAME_EXCED_RANGE);
    ERROR_CASE(ERROR_MORE_DATA);
    ERROR_CASE(ERROR_NO_MORE_ITEMS);
    ERROR_CASE(ERROR_INVALID_ADDRESS);
    ERROR_CASE(ERROR_OPERATION_ABORTED);
    ERROR_CASE(RPC_S_INVALID_STRING_BINDING);
    ERROR_CASE(RPC_S_WRONG_KIND_OF_BINDING);
    ERROR_CASE(RPC_S_INVALID_BINDING);
    ERROR_CASE(RPC_S_PROTSEQ_NOT_SUPPORTED);
    ERROR_CASE(RPC_S_INVALID_RPC_PROTSEQ);
    ERROR_CASE(RPC_S_INVALID_STRING_UUID);
    ERROR_CASE(RPC_S_INVALID_ENDPOINT_FORMAT);
    ERROR_CASE(RPC_S_INVALID_NET_ADDR);
    ERROR_CASE(RPC_S_NO_ENDPOINT_FOUND);
    ERROR_CASE(RPC_S_INVALID_TIMEOUT);
    ERROR_CASE(RPC_S_OBJECT_NOT_FOUND);
    ERROR_CASE(RPC_S_ALREADY_REGISTERED);
    ERROR_CASE(RPC_S_TYPE_ALREADY_REGISTERED);
    ERROR_CASE(RPC_S_ALREADY_LISTENING);
    ERROR_CASE(RPC_S_NO_PROTSEQS_REGISTERED);
    ERROR_CASE(RPC_S_NOT_LISTENING);
    ERROR_CASE(RPC_S_UNKNOWN_MGR_TYPE);
    ERROR_CASE(RPC_S_UNKNOWN_IF);
    ERROR_CASE(RPC_S_NO_BINDINGS);
    ERROR_CASE(RPC_S_NO_PROTSEQS);
    ERROR_CASE(RPC_S_CANT_CREATE_ENDPOINT);
    ERROR_CASE(RPC_S_OUT_OF_RESOURCES);
    ERROR_CASE(RPC_S_SERVER_UNAVAILABLE);
    ERROR_CASE(RPC_S_SERVER_TOO_BUSY);
    ERROR_CASE(RPC_S_INVALID_NETWORK_OPTIONS);
    ERROR_CASE(RPC_S_NO_CALL_ACTIVE);
    ERROR_CASE(RPC_S_CALL_FAILED);
    ERROR_CASE(RPC_S_CALL_FAILED_DNE);
    ERROR_CASE(RPC_S_PROTOCOL_ERROR);
    ERROR_CASE(RPC_S_UNSUPPORTED_TRANS_SYN);
    ERROR_CASE(RPC_S_UNSUPPORTED_TYPE);
    ERROR_CASE(RPC_S_INVALID_TAG);
    ERROR_CASE(RPC_S_INVALID_BOUND);
    ERROR_CASE(RPC_S_NO_ENTRY_NAME);
    ERROR_CASE(RPC_S_INVALID_NAME_SYNTAX);
    ERROR_CASE(RPC_S_UNSUPPORTED_NAME_SYNTAX);
    ERROR_CASE(RPC_S_UUID_NO_ADDRESS);
    ERROR_CASE(RPC_S_DUPLICATE_ENDPOINT);
    ERROR_CASE(RPC_S_UNKNOWN_AUTHN_TYPE);
    ERROR_CASE(RPC_S_MAX_CALLS_TOO_SMALL);
    ERROR_CASE(RPC_S_STRING_TOO_LONG);
    ERROR_CASE(RPC_S_PROTSEQ_NOT_FOUND);
    ERROR_CASE(RPC_S_PROCNUM_OUT_OF_RANGE);
    ERROR_CASE(RPC_S_BINDING_HAS_NO_AUTH);
    ERROR_CASE(RPC_S_UNKNOWN_AUTHN_SERVICE);
    ERROR_CASE(RPC_S_UNKNOWN_AUTHN_LEVEL);
    ERROR_CASE(RPC_S_INVALID_AUTH_IDENTITY);
    ERROR_CASE(RPC_S_UNKNOWN_AUTHZ_SERVICE);
    ERROR_CASE(EPT_S_INVALID_ENTRY);
    ERROR_CASE(EPT_S_CANT_PERFORM_OP);
    ERROR_CASE(EPT_S_NOT_REGISTERED);
    ERROR_CASE(RPC_S_NOTHING_TO_EXPORT);
    ERROR_CASE(RPC_S_INCOMPLETE_NAME);
    ERROR_CASE(RPC_S_INVALID_VERS_OPTION);
    ERROR_CASE(RPC_S_NO_MORE_MEMBERS);
    ERROR_CASE(RPC_S_NOT_ALL_OBJS_UNEXPORTED);
    ERROR_CASE(RPC_S_INTERFACE_NOT_FOUND);
    ERROR_CASE(RPC_S_ENTRY_ALREADY_EXISTS);
    ERROR_CASE(RPC_S_ENTRY_NOT_FOUND);
    ERROR_CASE(RPC_S_NAME_SERVICE_UNAVAILABLE);
    ERROR_CASE(RPC_S_INVALID_NAF_ID);
    ERROR_CASE(RPC_S_CANNOT_SUPPORT);
    ERROR_CASE(RPC_S_NO_CONTEXT_AVAILABLE);
    ERROR_CASE(RPC_S_INTERNAL_ERROR);
    ERROR_CASE(RPC_S_ZERO_DIVIDE);
    ERROR_CASE(RPC_S_ADDRESS_ERROR);
    ERROR_CASE(RPC_S_FP_DIV_ZERO);
    ERROR_CASE(RPC_S_FP_UNDERFLOW);
    ERROR_CASE(RPC_S_FP_OVERFLOW);
    ERROR_CASE(RPC_X_NO_MORE_ENTRIES);
    ERROR_CASE(RPC_X_SS_CHAR_TRANS_OPEN_FAIL);
    ERROR_CASE(RPC_X_SS_CHAR_TRANS_SHORT_FILE);
    ERROR_CASE(RPC_X_SS_IN_NULL_CONTEXT);
    ERROR_CASE(RPC_X_SS_CONTEXT_DAMAGED);
    ERROR_CASE(RPC_X_SS_HANDLES_MISMATCH);
    ERROR_CASE(RPC_X_SS_CANNOT_GET_CALL_HANDLE);
    ERROR_CASE(RPC_X_NULL_REF_POINTER);
    ERROR_CASE(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
    ERROR_CASE(RPC_X_BYTE_COUNT_TOO_SMALL);
    ERROR_CASE(RPC_X_BAD_STUB_DATA);


    //
    // WinInet errors
    //

    ERROR_CASE(ERROR_INTERNET_OUT_OF_HANDLES);
    ERROR_CASE(ERROR_INTERNET_TIMEOUT);
    ERROR_CASE(ERROR_INTERNET_EXTENDED_ERROR);
    ERROR_CASE(ERROR_INTERNET_INTERNAL_ERROR);
    ERROR_CASE(ERROR_INTERNET_INVALID_URL);
    ERROR_CASE(ERROR_INTERNET_UNRECOGNIZED_SCHEME);
    ERROR_CASE(ERROR_INTERNET_NAME_NOT_RESOLVED);
    ERROR_CASE(ERROR_INTERNET_PROTOCOL_NOT_FOUND);
    ERROR_CASE(ERROR_INTERNET_INVALID_OPTION);
    ERROR_CASE(ERROR_INTERNET_BAD_OPTION_LENGTH);
    ERROR_CASE(ERROR_INTERNET_OPTION_NOT_SETTABLE);
    ERROR_CASE(ERROR_INTERNET_SHUTDOWN);
    ERROR_CASE(ERROR_INTERNET_INCORRECT_USER_NAME);
    ERROR_CASE(ERROR_INTERNET_INCORRECT_PASSWORD);
    ERROR_CASE(ERROR_INTERNET_LOGIN_FAILURE);
    ERROR_CASE(ERROR_INTERNET_INVALID_OPERATION);
    ERROR_CASE(ERROR_INTERNET_OPERATION_CANCELLED);
    ERROR_CASE(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
    ERROR_CASE(ERROR_INTERNET_NOT_LOCAL_HANDLE);
    ERROR_CASE(ERROR_INTERNET_NOT_PROXY_REQUEST);
    ERROR_CASE(ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND);
    ERROR_CASE(ERROR_INTERNET_BAD_REGISTRY_PARAMETER);
    ERROR_CASE(ERROR_FTP_TRANSFER_IN_PROGRESS);
    ERROR_CASE(ERROR_FTP_CONNECTED);
    ERROR_CASE(ERROR_FTP_DROPPED);
    ERROR_CASE(ERROR_GOPHER_PROTOCOL_ERROR);
    ERROR_CASE(ERROR_GOPHER_NOT_FILE);
    ERROR_CASE(ERROR_GOPHER_DATA_ERROR);
    ERROR_CASE(ERROR_GOPHER_END_OF_DATA);
    ERROR_CASE(ERROR_GOPHER_INVALID_LOCATOR);
    ERROR_CASE(ERROR_GOPHER_INCORRECT_LOCATOR_TYPE);
    ERROR_CASE(ERROR_GOPHER_NOT_GOPHER_PLUS);
    ERROR_CASE(ERROR_GOPHER_ATTRIBUTE_NOT_FOUND);
    ERROR_CASE(ERROR_GOPHER_UNKNOWN_LOCATOR);
    ERROR_CASE(ERROR_HTTP_HEADER_NOT_FOUND);
    ERROR_CASE(ERROR_HTTP_DOWNLEVEL_SERVER);
    ERROR_CASE(ERROR_HTTP_INVALID_SERVER_RESPONSE);


    //
    // Windows sockets errors
    //

    ERROR_CASE(WSAEINTR);
    ERROR_CASE(WSAEBADF);
    ERROR_CASE(WSAEACCES);
    ERROR_CASE(WSAEFAULT);
    ERROR_CASE(WSAEINVAL);
    ERROR_CASE(WSAEMFILE);
    ERROR_CASE(WSAEWOULDBLOCK);
    ERROR_CASE(WSAEINPROGRESS);
    ERROR_CASE(WSAEALREADY);
    ERROR_CASE(WSAENOTSOCK);
    ERROR_CASE(WSAEDESTADDRREQ);
    ERROR_CASE(WSAEMSGSIZE);
    ERROR_CASE(WSAEPROTOTYPE);
    ERROR_CASE(WSAENOPROTOOPT);
    ERROR_CASE(WSAEPROTONOSUPPORT);
    ERROR_CASE(WSAESOCKTNOSUPPORT);
    ERROR_CASE(WSAEOPNOTSUPP);
    ERROR_CASE(WSAEPFNOSUPPORT);
    ERROR_CASE(WSAEAFNOSUPPORT);
    ERROR_CASE(WSAEADDRINUSE);
    ERROR_CASE(WSAEADDRNOTAVAIL);
    ERROR_CASE(WSAENETDOWN);
    ERROR_CASE(WSAENETUNREACH);
    ERROR_CASE(WSAENETRESET);
    ERROR_CASE(WSAECONNABORTED);
    ERROR_CASE(WSAECONNRESET);
    ERROR_CASE(WSAENOBUFS);
    ERROR_CASE(WSAEISCONN);
    ERROR_CASE(WSAENOTCONN);
    ERROR_CASE(WSAESHUTDOWN);
    ERROR_CASE(WSAETOOMANYREFS);
    ERROR_CASE(WSAETIMEDOUT);
    ERROR_CASE(WSAECONNREFUSED);
    ERROR_CASE(WSAELOOP);
    ERROR_CASE(WSAENAMETOOLONG);
    ERROR_CASE(WSAEHOSTDOWN);
    ERROR_CASE(WSAEHOSTUNREACH);
    ERROR_CASE(WSAENOTEMPTY);
    ERROR_CASE(WSAEPROCLIM);
    ERROR_CASE(WSAEUSERS);
    ERROR_CASE(WSAEDQUOT);
    ERROR_CASE(WSAESTALE);
    ERROR_CASE(WSAEREMOTE);
    ERROR_CASE(WSAEDISCON);
    ERROR_CASE(WSASYSNOTREADY);
    ERROR_CASE(WSAVERNOTSUPPORTED);
    ERROR_CASE(WSANOTINITIALISED);
    ERROR_CASE(WSAHOST_NOT_FOUND);
    ERROR_CASE(WSATRY_AGAIN);
    ERROR_CASE(WSANO_RECOVERY);
    ERROR_CASE(WSANO_DATA);

    default:
        return "?";
    }
}

void get_last_internet_error() {

    DWORD bufLength;
    char buffer[256];
    DWORD category;

    bufLength = sizeof(buffer);
    if (InternetGetLastResponseInfo(&category, buffer, &bufLength)) {
        printf("InternetGetLastResponseInfo() returns %d bytes\n", bufLength);
        if (bufLength != 0) {
            printf("Text = \"%s\"\n", buffer);
        }
        if (strlen(buffer) != bufLength) {
            printf("\aerror: get_last_internet_error: InternetGetLastResponseInfo() returns %d bytes; strlen(buffer) = %d\n",
                    bufLength,
                    strlen(buffer)
                    );
        }
    } else {

        LPSTR errbuf;

        printf("InternetGetLastResponseInfo() returns error %d (bufLength = %d)\n",
               GetLastError(),
               bufLength
               );
        if ((errbuf = malloc(bufLength)) == NULL) {
            printf("error: get_last_internet_error: malloc(%d) failed\n", bufLength);
            return;
        }
        if (InternetGetLastResponseInfo(&category, errbuf, &bufLength)) {
            printf("InternetGetLastResponseInfo() returns %d bytes\n", bufLength);
            if (bufLength != 0) {
                printf("Text = \"%s\"\n", errbuf);
            }
            if (strlen(buffer) != bufLength) {
                printf("\aerror: get_last_internet_error: InternetGetLastResponseInfo() returns %d bytes; strlen(buffer) = %d\n",
                        bufLength,
                        strlen(buffer)
                        );
            }
        } else {
            printf("error: get_last_internet_error: InternetGetLastResponseInfo() returns error %d (bufLength = %d)\n",
               GetLastError(),
               bufLength
               );
        }
        free(errbuf);
    }
}
