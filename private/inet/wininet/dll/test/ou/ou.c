/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    openurl.c

Abstract:

    Tests InternetOpenUrl()/InternetReadFile()

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

//
// functions
//

void _CRTAPI1 main(int argc, char** argv) {

    BOOL ok;
    LPSTR urls[64];
    int numberOfUrls = 0;
    BOOL fCallback = FALSE;

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (tolower(*++*argv)) {
            case 'c':
                fCallback = TRUE;
                break;

            case 'v':
                Verbose = TRUE;
                break;

            default:
                printf("unknown command line flag: '%c'\n", **argv);
                usage();
            }
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

    InternetHandle = InternetOpen("ou",
                                  PRE_CONFIG_INTERNET_ACCESS,
                                  NULL,
                                  0,
                                  0
                                  );
    if (InternetHandle == NULL) {
        printf("error: openurl: InternetOpen() returns %d\n", GetLastError());
        exit(1);
    }

    if (numberOfUrls == 0) {
        printf("error: you must supply an URL\n");
        usage();
    } else {
        open_urls(urls, numberOfUrls);
    }

    ok = InternetCloseHandle(InternetHandle);
    if (!ok) {
        printf("error: openurl: InternetClose(InternetHandle) returns %d\n", GetLastError());
        exit(1);
    }

    printf("Done.\n");
    exit(0);
}

void usage() {
    printf("usage: ou [-c] [-v] [url]*\n"
           "where:  -c = enable status call-backs\n"
           "        -v = Verbose mode\n"
           "        url = one or more URLs to open\n"
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

        case INTERNET_HANDLE_TYPE_FTP_FILE:
            read_data(handle);
            break;

        case INTERNET_HANDLE_TYPE_GOPHER_FIND:
            gopher_find(handle);
            break;

        case INTERNET_HANDLE_TYPE_GOPHER_FILE:
            read_data(handle);
            break;

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
    DWORD nRead;

    while (InternetReadFile(handle, (LPVOID)&ffd, sizeof(ffd), &nRead)) {

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
        print_error("ftp_find", "InternetReadFile()");
    }
}

void gopher_find(HINTERNET handle) {

    GOPHER_FIND_DATA data;
    int i;
    DWORD nRead;

    i = 0;
    while (InternetReadFile(handle, (LPVOID)&data, sizeof(data), &nRead)) {

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
        print_error("gopher_find", "InternetReadFile()");
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

char* map_error(DWORD error) {
    switch (error) {
    case ERROR_FILE_NOT_FOUND:
        return "ERROR_FILE_NOT_FOUND";

    case ERROR_PATH_NOT_FOUND:
        return "ERROR_PATH_NOT_FOUND";

    case ERROR_ACCESS_DENIED:
        return "ERROR_ACCESS_DENIED";

    case ERROR_INVALID_HANDLE:
        return "ERROR_INVALID_HANDLE";

    case ERROR_NOT_ENOUGH_MEMORY:
        return "ERROR_NOT_ENOUGH_MEMORY";

    case ERROR_NO_MORE_FILES:
        return "ERROR_NO_MORE_FILES";

    case ERROR_INVALID_PASSWORD:
        return "ERROR_INVALID_PASSWORD";

    case ERROR_INVALID_PARAMETER:
        return "ERROR_INVALID_PARAMETER";

    case ERROR_BUFFER_OVERFLOW:
        return "ERROR_BUFFER_OVERFLOW";

    case ERROR_NO_MORE_SEARCH_HANDLES:
        return "ERROR_NO_MORE_SEARCH_HANDLES";

    case ERROR_INVALID_TARGET_HANDLE:
        return "ERROR_INVALID_TARGET_HANDLE";

    case ERROR_CALL_NOT_IMPLEMENTED:
        return "ERROR_CALL_NOT_IMPLEMENTED";

    case ERROR_INSUFFICIENT_BUFFER:
        return "ERROR_INSUFFICIENT_BUFFER";

    case ERROR_INVALID_NAME:
        return "ERROR_INVALID_NAME";

    case ERROR_INVALID_LEVEL:
        return "ERROR_INVALID_LEVEL";

    case ERROR_BAD_PATHNAME:
        return "ERROR_BAD_PATHNAME";

    case ERROR_BUSY:
        return "ERROR_BUSY";

    case ERROR_ALREADY_EXISTS:
        return "ERROR_ALREADY_EXISTS";

    case ERROR_FILENAME_EXCED_RANGE:
        return "ERROR_FILENAME_EXCED_RANGE";

    case ERROR_MORE_DATA:
        return "ERROR_MORE_DATA";

    case ERROR_NO_MORE_ITEMS:
        return "ERROR_NO_MORE_ITEMS";

    case ERROR_INVALID_ADDRESS:
        return "ERROR_INVALID_ADDRESS";

    case ERROR_OPERATION_ABORTED:
        return "ERROR_OPERATION_ABORTED";

    case ERROR_INTERNET_OUT_OF_HANDLES:
        return "ERROR_INTERNET_OUT_OF_HANDLES";

    case ERROR_INTERNET_TIMEOUT:
        return "ERROR_INTERNET_TIMEOUT";

    case ERROR_INTERNET_EXTENDED_ERROR:
        return "ERROR_INTERNET_EXTENDED_ERROR";

    case ERROR_INTERNET_INTERNAL_ERROR:
        return "ERROR_INTERNET_INTERNAL_ERROR";

    case ERROR_INTERNET_INVALID_URL:
        return "ERROR_INTERNET_INVALID_URL";

    case ERROR_INTERNET_UNRECOGNIZED_SCHEME:
        return "ERROR_INTERNET_UNRECOGNIZED_SCHEME";

    case ERROR_INTERNET_NAME_NOT_RESOLVED:
        return "ERROR_INTERNET_NAME_NOT_RESOLVED";

    case ERROR_INTERNET_PROTOCOL_NOT_FOUND:
        return "ERROR_INTERNET_PROTOCOL_NOT_FOUND";

    case ERROR_INTERNET_INVALID_OPTION:
        return "ERROR_INTERNET_INVALID_OPTION";

    case ERROR_FTP_TRANSFER_IN_PROGRESS:
        return "ERROR_FTP_TRANSFER_IN_PROGRESS";

    case ERROR_FTP_CONNECTED:
        return "ERROR_FTP_CONNECTED";

    case ERROR_FTP_DROPPED:
        return "ERROR_FTP_DROPPED";

    case ERROR_GOPHER_PROTOCOL_ERROR:
        return "ERROR_GOPHER_PROTOCOL_ERROR";

    case ERROR_GOPHER_NOT_FILE:
        return "ERROR_GOPHER_NOT_FILE";

    case ERROR_GOPHER_DATA_ERROR:
        return "ERROR_GOPHER_DATA_ERROR";

    case ERROR_GOPHER_END_OF_DATA:
        return "ERROR_GOPHER_END_OF_DATA";

    case ERROR_GOPHER_INVALID_LOCATOR:
        return "ERROR_GOPHER_INVALID_LOCATOR";

    case ERROR_GOPHER_INCORRECT_LOCATOR_TYPE:
        return "ERROR_GOPHER_INCORRECT_LOCATOR_TYPE";

    case ERROR_GOPHER_NOT_GOPHER_PLUS:
        return "ERROR_GOPHER_NOT_GOPHER_PLUS";

    case ERROR_GOPHER_ATTRIBUTE_NOT_FOUND:
        return "ERROR_GOPHER_ATTRIBUTE_NOT_FOUND";

    case ERROR_GOPHER_UNKNOWN_LOCATOR:
        return "ERROR_GOPHER_UNKNOWN_LOCATOR";

    case ERROR_HTTP_HEADER_NOT_FOUND:
        return "ERROR_HTTP_HEADER_NOT_FOUND";

    case ERROR_HTTP_DOWNLEVEL_SERVER:
        return "ERROR_HTTP_DOWNLEVEL_SERVER";

    case ERROR_HTTP_INVALID_SERVER_RESPONSE:
        return "ERROR_HTTP_INVALID_SERVER_RESPONSE";

    case WSAEINTR:
        return "WSAEINTR";

    case WSAEBADF:
        return "WSAEBADF";

    case WSAEACCES:
        return "WSAEACCES";

    case WSAEFAULT:
        return "WSAEFAULT";

    case WSAEINVAL:
        return "WSAEINVAL";

    case WSAEMFILE:
        return "WSAEMFILE";

    case WSAEWOULDBLOCK:
        return "WSAEWOULDBLOCK";

    case WSAEINPROGRESS:
        return "WSAEINPROGRESS";

    case WSAEALREADY:
        return "WSAEALREADY";

    case WSAENOTSOCK:
        return "WSAENOTSOCK";

    case WSAEDESTADDRREQ:
        return "WSAEDESTADDRREQ";

    case WSAEMSGSIZE:
        return "WSAEMSGSIZE";

    case WSAEPROTOTYPE:
        return "WSAEPROTOTYPE";

    case WSAENOPROTOOPT:
        return "WSAENOPROTOOPT";

    case WSAEPROTONOSUPPORT:
        return "WSAEPROTONOSUPPORT";

    case WSAESOCKTNOSUPPORT:
        return "WSAESOCKTNOSUPPORT";

    case WSAEOPNOTSUPP:
        return "WSAEOPNOTSUPP";

    case WSAEPFNOSUPPORT:
        return "WSAEPFNOSUPPORT";

    case WSAEAFNOSUPPORT:
        return "WSAEAFNOSUPPORT";

    case WSAEADDRINUSE:
        return "WSAEADDRINUSE";

    case WSAEADDRNOTAVAIL:
        return "WSAEADDRNOTAVAIL";

    case WSAENETDOWN:
        return "WSAENETDOWN";

    case WSAENETUNREACH:
        return "WSAENETUNREACH";

    case WSAENETRESET:
        return "WSAENETRESET";

    case WSAECONNABORTED:
        return "WSAECONNABORTED";

    case WSAECONNRESET:
        return "WSAECONNRESET";

    case WSAENOBUFS:
        return "WSAENOBUFS";

    case WSAEISCONN:
        return "WSAEISCONN";

    case WSAENOTCONN:
        return "WSAENOTCONN";

    case WSAESHUTDOWN:
        return "WSAESHUTDOWN";

    case WSAETOOMANYREFS:
        return "WSAETOOMANYREFS";

    case WSAETIMEDOUT:
        return "WSAETIMEDOUT";

    case WSAECONNREFUSED:
        return "WSAECONNREFUSED";

    case WSAELOOP:
        return "WSAELOOP";

    case WSAENAMETOOLONG:
        return "WSAENAMETOOLONG";

    case WSAEHOSTDOWN:
        return "WSAEHOSTDOWN";

    case WSAEHOSTUNREACH:
        return "WSAEHOSTUNREACH";

    case WSAENOTEMPTY:
        return "WSAENOTEMPTY";

    case WSAEPROCLIM:
        return "WSAEPROCLIM";

    case WSAEUSERS:
        return "WSAEUSERS";

    case WSAEDQUOT:
        return "WSAEDQUOT";

    case WSAESTALE:
        return "WSAESTALE";

    case WSAEREMOTE:
        return "WSAEREMOTE";

    case WSAEDISCON:
        return "WSAEDISCON";

    case WSASYSNOTREADY:
        return "WSASYSNOTREADY";

    case WSAVERNOTSUPPORTED:
        return "WSAVERNOTSUPPORTED";

    case WSANOTINITIALISED:
        return "WSANOTINITIALISED";

    case WSAHOST_NOT_FOUND:
        return "WSAHOST_NOT_FOUND";

    case WSATRY_AGAIN:
        return "WSATRY_AGAIN";

    case WSANO_RECOVERY:
        return "WSANO_RECOVERY";

    case WSANO_DATA:
        return "WSANO_DATA";

    default:
        return "???";
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
