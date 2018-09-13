/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    gc.c

Abstract:

    Gopher client test program

    Basically a real console-mode (win32) gopher client that uses the gopher
    client APIs

    Contents:

Author:

    Richard L Firth (rfirth) 08-Nov-1994

Environment:

    Win32 console mode user executable

Revision History:

    08-Nov-1994 rfirth
        Created

--*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wininet.h>
#include <wininetd.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <catlib.h>

#ifndef _CRTAPI1
#define _CRTAPI1
#endif

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))

//
// manifests
//

#define HOME    -1

#define GC_CONNECT_CONTEXT  0x47474747
#define GC_FIND_CONTEXT     0x47474644
#define GC_FILE_CONTEXT     0x47474645

//
// prototypes
//

void _CRTAPI1 main(int, char**);
void usage(void);
void _CRTAPI1 my_cleanup(void);
void gopher(LPSTR, WORD, CHAR, LPSTR);
BOOL get_dir(LPSTR, LPSTR);
void get_file(LPSTR);
int get_user_request(LPSTR);
void clear_items(void);
void my_callback(HINTERNET, DWORD, DWORD, LPVOID, DWORD);
void print_error(char*, char*, ...);
char hex_to_char(char);
char* decontrol(char*, char*);
void toodle_pip(void);

//
// global data
//

BOOL Verbose = FALSE;
BOOL MakeRequestGopherPlus = FALSE;
HINTERNET InetHandle = NULL;
HINTERNET hGopherSession = NULL;
BOOL NewHome = FALSE;
INTERNET_STATUS_CALLBACK PreviousCallback;
BOOL AsyncMode = FALSE;
HANDLE AsyncEvent = NULL;
DWORD AsyncResult;
DWORD AsyncError;
DWORD CacheFlags = 0;
BOOL UseQueryData = FALSE;
DWORD UserContext = 0;
BOOL UseUserContext = FALSE;

//
// functions
//

void _CRTAPI1 main(int argc, char** argv) {

    LPSTR server = NULL;
    LPSTR selector = NULL;
    WORD port = 70;
    BOOL fCallback = FALSE;
    char selectorType = '1';
    DWORD accessMethod = PRE_CONFIG_INTERNET_ACCESS;
    BOOL expectingProxy = FALSE;
    LPSTR proxyServer = NULL;

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (*++*argv) {
            case '?':
                usage();
                break;

            case '+':
                MakeRequestGopherPlus = TRUE;
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

            case 'c':
                fCallback = TRUE;
                break;

            case 'n':
                CacheFlags |= INTERNET_FLAG_DONT_CACHE;
                break;

            case 'p':
                port = atoi(++*argv);
                break;

            case 'q':
                UseQueryData = TRUE;
                break;

            case 't':
                selectorType = *++*argv;
                break;

            case 'v':
                Verbose = TRUE;
                break;

            case 'x':
                UseUserContext = TRUE;
                if (*++*argv) {
                    UserContext = (DWORD)strtoul(*argv, NULL, 0);
                }
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
        } else if (!server) {
            server = *argv;
        } else if (!selector) {
            selector = *argv;
        } else {
            printf("unknown command line argument: \"%s\"\n", *argv);
            usage();
        }
    }

    if (!server) {
        usage();
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
            print_error("gc", "CreateEvent()");
            exit(1);
        }
    }

    //
    // open gateway
    //

    InetHandle = InternetOpen("gc",
                              accessMethod,
                              proxyServer,
                              NULL,
                              AsyncMode ? INTERNET_FLAG_ASYNC : 0
                              );

    if (InetHandle == NULL) {
        print_error("gc", "InternetOpen()");
        exit(1);
    }

    if (fCallback) {

        //
        // let's have a status callback
        //

        PreviousCallback = InternetSetStatusCallback(InetHandle, my_callback);
        if (Verbose) {
            printf("previous Internet callback = %x\n", PreviousCallback);
        }
    }

    hGopherSession = InternetConnect(InetHandle,
                                     server,
                                     0,
                                     NULL,
                                     NULL,
                                     INTERNET_SERVICE_GOPHER,
                                     0,
                                     UseUserContext ? UserContext : GC_CONNECT_CONTEXT
                                     );
    if (AsyncMode && (hGopherSession == NULL)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            print_error("gc", "InternetConnect()");
            exit(1);
        } else {
            if (Verbose) {
                printf("Waiting for async InternetConnect()\n");
            }
            WaitForSingleObject(AsyncEvent, INFINITE);
            hGopherSession = (HINTERNET)AsyncResult;
            SetLastError(AsyncError);
        }
    }

    if (hGopherSession == NULL) {
        print_error("gc", "InternetConnect()");
        exit(1);
    }

    gopher(server, port, selectorType, selector);

    if (Verbose) {
        printf("closing InternetConnect handle %x\n", hGopherSession);
    }
    if (!InternetCloseHandle(hGopherSession)) {
        print_error("gc", "InternetCloseHandle(%#x)", hGopherSession);
    } else {
        hGopherSession = NULL;
    }

    if (Verbose) {
        printf("closing InternetOpen handle %x\n", InetHandle);
    }
    if (!InternetCloseHandle(InetHandle)) {
        print_error("gc", "InternetCloseHandle(%#x)", InetHandle);
    } else {
        InetHandle = NULL;
    }

    toodle_pip();
    exit(0);
}

void toodle_pip() {

    static LPSTR intl[] = {
        "Goodbye",
        "Au revoir",
        "Auf wiederzehen",
        "Cheers",
        "Ciao"
    };

    srand(GetTickCount());
    printf("%s.\n", intl[rand() % (sizeof(intl) / sizeof(intl[0]))]);
}

void usage() {
    printf("\n"
           "usage: gc [-+] [-a{l|p[ ]proxy}] [-c] [-p#] [-n] [-v] [-y] [-q] [-t<char>]\n"
           "          [-x#] [selector] <server>\n"
           "\n"
           "where: -+ = First request is gopher+\n"
           "       -a = Access type. Default is pre-configured:\n"
           "        l = direct internet access\n"
           "        p = proxy access\n"
           "       -c = Enable status callbacks\n"
           "       -n = Don't cache\n"
           "       -p = Port number to connect to at <server>. Default is 70\n"
           "       -q = use InternetQueryDataAvailable\n"
           "       -t = Selector type character, e.g. -t9 for binary file. Default is dir\n"
           "       -v = Verbose mode\n"
           "       -x = Context value. # is number to use as context\n"
           "       -y = Asynchronous APIs\n"
          );
    exit(1);
}

void _CRTAPI1 my_cleanup() {
    if (hGopherSession != NULL) {
        printf("closing InternetConnect handle %x\n", hGopherSession);
        if (!InternetCloseHandle(hGopherSession)) {
            print_error("my_cleanup", "InternetCloseHandle(%#x)", hGopherSession);
        }
    }
    if (InetHandle != NULL) {
        printf("closing InternetOpen handle %x\n", InetHandle);
        if (!InternetCloseHandle(InetHandle)) {
            print_error("my_cleanup", "InternetCloseHandle(%#x)", InetHandle);
        }
    }
}

char HomeLocator[MAX_GOPHER_SELECTOR_TEXT];

typedef struct {
    LPSTR display_string;
    LPSTR locator;
} GINFO;

GINFO items[4096];
int nitems = 0;

void gopher(LPSTR server, WORD port, CHAR selectorType, LPSTR selector) {

    LPSTR request = "";
    char locator[MAX_GOPHER_SELECTOR_TEXT];
    DWORD len;
    BOOL done = FALSE;
    HINTERNET h;
    BOOL unknownType;
    DWORD gopherType;

    //
    // if the user supplied a gopher type character then create a default
    // text locator, then change the type (evil!). Otherwise, the default
    // is directory
    //

    gopherType = selector ? GOPHER_TYPE_TEXT_FILE : GOPHER_TYPE_DIRECTORY;
    if (MakeRequestGopherPlus) {
        gopherType |= GOPHER_TYPE_GOPHER_PLUS;
    }

    len = sizeof(HomeLocator);
    if (!GopherCreateLocator(server,
                             port,
                             NULL,
                             selector,
                             gopherType,
                             locator,
                             &len
                             )) {
        print_error("gopher", "GopherCreateLocator()");
        return;
    }

    if (selector) {
        *locator = selectorType;
    }

    NewHome = TRUE;

    while (!done) {

        DWORD gopherType;

        unknownType = FALSE;
        if (!GopherGetLocatorType(locator, &gopherType)) {
            print_error("gopher", "GopherGetLocatorType()");
            exit(1);
        }
        if (gopherType & GOPHER_TYPE_DIRECTORY) {
            if (get_dir(locator, NULL)) {
                if (NewHome) {
                    strcpy(HomeLocator, locator);
                    NewHome = FALSE;
                }
            }
        } else if (gopherType & GOPHER_TYPE_FILE_MASK) {
            get_file(locator);
        } else {
            if (gopherType & GOPHER_TYPE_INDEX_SERVER) {

                char searchBuf[256];

                printf("\nEnter Text To Search For: ");
                gets(searchBuf);
                get_dir(locator, searchBuf);
            } else {
                unknownType = TRUE;
            }
        }
        if (unknownType) {

            char dcbuf[1024];

            printf("error: gopher: locator %s is unknown type\n",
                    decontrol(locator, dcbuf)
                    );
            return;
        }
        done = get_user_request(locator) == 0;
    }
}

char CurrentDirLocator[256];

BOOL get_dir(LPSTR locator, LPSTR search) {

    HINTERNET h;
    GOPHER_FIND_DATA data;
    DWORD error;

    h = GopherFindFirstFile(hGopherSession,
                            locator,
                            search,
                            &data,
                            CacheFlags, // dwFlags
                            UseUserContext ? UserContext : GC_FIND_CONTEXT
                            );

    if (AsyncMode && (h == NULL)) {
        error = GetLastError();
        if (error == ERROR_IO_PENDING) {
            if (Verbose) {
                printf("waiting for async GopherFindFirstFile()...\n");
            }
            WaitForSingleObject(AsyncEvent, INFINITE);
            h = (HINTERNET)AsyncResult;
            error = AsyncError;
        }
        if (h == NULL) {
            SetLastError(error);
        }
    }

    if (h != NULL) {

        LPGOPHER_FIND_DATA p = (LPGOPHER_FIND_DATA)&data;
        SYSTEMTIME systemTime;
        int i = 0;
        char timeBuf[9];
        char sizeBuf[32];
        BOOL ok;

        clear_items();
        strcpy(CurrentDirLocator, locator);
        do {
            items[i].display_string = _strdup(p->DisplayString);
            items[i].locator = _strdup(p->Locator);
            ++i;
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

            if (UseQueryData) {

                DWORD avail;

                ok = InternetQueryDataAvailable(h, &avail, 0, 0);
                if (!ok) {
                    error = GetLastError();
                    if (error == ERROR_IO_PENDING) {
                        if (Verbose) {
                            printf("waiting for async InternetQueryDataAvailable()...\n");
                        }
                        WaitForSingleObject(AsyncEvent, INFINITE);
                        ok = (BOOL)AsyncResult;
                        SetLastError(AsyncError);
                    }
                }
                if (ok) {
                    if (Verbose) {
                        printf("%sSYNC IQDA(): %d available\n", AsyncMode ? "A" : "", avail);
                    }
                } else {
                    print_error("get_dir()", "InternetQueryDataAvailable()");
                    break;
                }
            }

            ok = InternetFindNextFile(h, (LPGOPHER_FIND_DATA)&data);

            if (AsyncMode && !ok) {
                error = GetLastError();
                if (error == ERROR_IO_PENDING) {
                    if (Verbose) {
                        printf("waiting for async InternetFindNextFile()...\n");
                    }
                    WaitForSingleObject(AsyncEvent, INFINITE);
                    ok = (BOOL)AsyncResult;
                    error = AsyncError;
                }
                SetLastError(error);
            }
        } while (ok);

        if (GetLastError() != ERROR_NO_MORE_FILES) {
            print_error("get_dir", "InternetFindNextFile()");
        }

        nitems = i;

        if (Verbose) {
            printf("closing Find handle %x\n", h);
        }
        if (!InternetCloseHandle(h)) {
            print_error("get_dir", "InternetCloseHandle(%#x)", h);
        }

        return TRUE;
    } else {
        print_error("get_dir", "GopherFindFirstFile()");
        return FALSE;
    }
}

void get_file(LPSTR locator) {

    HINTERNET h;
    char buf[4096];
    DWORD error;

    h = GopherOpenFile(hGopherSession,
                       locator,
                       NULL,
                       CacheFlags,
                       UseUserContext ? UserContext : GC_FILE_CONTEXT
                       );

    if (AsyncMode && (h == NULL)) {
        error = GetLastError();
        if (error == ERROR_IO_PENDING) {
            if (Verbose) {
                printf("waiting for async GopherOpenFile()...\n");
            }
            WaitForSingleObject(AsyncEvent, INFINITE);
            h = (HINTERNET)AsyncResult;
            error = AsyncError;
        }
        SetLastError(error);
    }

    if (h == NULL) {
        print_error("get_file", "GopherOpenFile()");
    } else {

        DWORD nread;
        BOOL ok;
        DWORD avail;

        do {
            if (UseQueryData) {
                ok = InternetQueryDataAvailable(h, &avail, 0, 0);
                if (!ok) {
                    error = GetLastError();
                    if (error == ERROR_IO_PENDING) {
                        if (Verbose) {
                            printf("waiting for async InternetQueryDataAvailable()...\n");
                        }
                        WaitForSingleObject(AsyncEvent, INFINITE);
                        ok = (BOOL)AsyncResult;
                        SetLastError(AsyncError);
                    }
                }
                if (ok) {
                    if (Verbose) {
                        printf("%sSYNC IQDA(): %d available\n", AsyncMode ? "A" : "", avail);
                    }
                } else {
                    print_error("get_dir()", "InternetQueryDataAvailable()");
                    break;
                }
            } else {
                avail = sizeof(buf);
            }

            avail = min(avail, sizeof(buf));
            if (avail == 0) {
                break;
            }

            ok = InternetReadFile(h, buf, avail, &nread);

            if (!ok && AsyncMode) {
                error = GetLastError();
                if (error == ERROR_IO_PENDING) {
                    if (Verbose) {
                        printf("waiting for async InternetReadFile()...\n");
                    }
                    WaitForSingleObject(AsyncEvent, INFINITE);
                    ok = (BOOL)AsyncResult;
                    error = AsyncError;
                }
                SetLastError(error);
            }

            if (ok) {
                if (!nread) {
                    printf("=== end of file ===\n");
                    break;
                } else {
                    _setmode(1, _O_BINARY);
                    _write(1, buf, nread);
                }
            }
        } while (ok);

        if (!ok) {
            error = GetLastError();
            if (error != ERROR_SUCCESS) {
                print_error("get_file", "InternetReadFile()");
            }
        }
        if (Verbose) {
            printf("closing File handle %x\n", h);
        }
        if (!InternetCloseHandle(h)) {
            print_error("get_file", "InternetCloseHandle(%#x)", h);
        }
    }
}

int get_user_request(LPSTR locator) {

    int n;
    char buf[80];
    BOOL got = FALSE;
    char newLocator[256];
    char serverBuf[80];
    char portBuf[32];
    DWORD len = sizeof(newLocator);
    int i;
    BOOL ok;
    DWORD handles;
    DWORD size_handles;

    while (!got) {
        printf("\nEnter selection: ");
        gets(buf);
        if (isdigit(buf[0])) {
            n = atoi(buf);
            if (n >= 1 && n <= nitems) {
                strcpy(locator, items[n - 1].locator);
                got = TRUE;
            } else {
                printf("\n"
                       "error: must enter number in the range 1 to %d\n", nitems);
            }
        } else {
            switch (buf[0]) {
            case '+':
                printf("NYI\n");
                break;

            case '.':
                strcpy(locator, CurrentDirLocator);
                got = TRUE;
                break;

            case 'g':
                for (i = 1; buf[i] && isspace(buf[i]); ) {
                    ++i;
                }
                if (buf[i]) {

                    int j = 0;

                    while (buf[i] && !isspace(buf[i])) {
                        serverBuf[j++] = buf[i++];
                    }
                    serverBuf[j] = 0;
                    while (buf[i] && isspace(buf[i])) {
                        ++i;
                    }
                } else {
                    printf("server: ");
                    gets(serverBuf);
                }
                if (buf[i]) {

                    int j = 0;

                    while (buf[i] && !isspace(buf[i])) {
                        portBuf[j++] = buf[i++];
                    }
                    portBuf[j] = 0;
                } else {
                    printf("port:   ");
                    gets(portBuf);
                }
                if (!GopherCreateLocator(serverBuf,
                                         (WORD)atoi(portBuf),
                                         NULL,
                                         NULL,
                                         GOPHER_TYPE_DIRECTORY,
                                         newLocator,
                                         &len
                                         )) {
                    print_error("get_user_request", "GopherCreateLocator()");
                } else {
                    strcpy(locator, newLocator);
                    got = TRUE;
                }
                NewHome = TRUE;
                break;

            case 'h':
                n = HOME;
                strcpy(locator, HomeLocator);
                got = TRUE;
                break;

            case 'l':
                size_handles = sizeof(handles);
                ok = InternetQueryOption(NULL,
                                         INTERNET_OPTION_GET_HANDLE_COUNT,
                                         (LPVOID)&handles,
                                         &size_handles
                                         );
                if (!ok) {
                    print_error("get_user_request", "InternetQueryOption(handle count)");
                } else {
                    printf("current handle count = %d\n", handles);
                }
                break;

            case 'q':
                toodle_pip();
                exit(0);

            case 's':
                PreviousCallback = InternetSetStatusCallback(InetHandle,
                                                             PreviousCallback
                                                             );
                if (Verbose) {
                    printf("previous Internet callback = %x\n", PreviousCallback);
                }
                if ((PreviousCallback != NULL) && (PreviousCallback != my_callback)) {
                    printf("error: get_gopher_request: previous callback not recognised\n");
                }
                got = TRUE;
                break;

            case 'v':
                Verbose = !Verbose;
                printf("verbose mode %s\n", Verbose ? "on" : "off");
                break;

            default:
                printf("\n"
                       "enter the number of your selection or one of the following:\n"
                       "\n"
                       "\t+ = toggle gopher+\n"
                       "\t. = list current directory\n"
                       "\tg = go to new server\n"
                       "\th = list home directory\n"
                       "\t1 = display handle usage\n"
                       "\tq = quit\n"
                       "\ts = toggle status callback\n"
                       "\tv = toggle verbose mode\n"
                       );
            }
        }
    }
    putchar('\n');

    return n;
}

void clear_items() {
    while (nitems) {
        --nitems;
        free(items[nitems].display_string);
        free(items[nitems].locator);
    }
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
        printf("callback: handle %x [context %x [%s]] %s ",
                Handle,
                Context,
                (Context == GC_CONNECT_CONTEXT) ? "Connect"
                : (Context == GC_FIND_CONTEXT) ? "Find"
                : (Context == GC_FILE_CONTEXT) ? "File"
                : "???",
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

char hex_to_char(char b) {
    return (b <= 9) ? (b + '0') : ((b - 10) + 'a');
}

char* decontrol(char* instr, char* outstr) {

    char* outp;

    for (outp = outstr; *instr; ++instr) {
        if (*instr < 0x20) {
            *outp++ = '\\';
            switch (*instr) {
            case '\t':
                *outp++ = 't';
                break;

            case '\r':
                *outp++ = 'r';
                break;

            case '\n':
                *outp++ = 'n';
                break;

            default:
                *outp++ = 'x';
                *outp++ = hex_to_char((char)(*instr >> 4));
                *outp++ = hex_to_char((char)(*instr & 15));
            }
        } else {
            *outp++ = *instr;
        }
    }
    *outp = 0;
    return outstr;
}
