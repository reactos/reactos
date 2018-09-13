/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ftpcat.c

Abstract:

    Windows Internet API FTP test program

    Provides the same functionality as a cut-down version of the venerable
    (console-mode) ftp program

Author:

    Richard L Firth (rfirth) 05-Jun-1995

Environment:

    Win32 user-mode console app

Revision History:

    05-Jun-1995 rfirth
        Created

--*/

#include "ftpcatp.h"

#undef tolower

//
// macros
//

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))

//
// manifests
//

#define MAX_COMMAND_LENGTH 100

//
// external data
//

extern BOOL fQuit;
extern DWORD CacheFlags;

//
// data
//

DWORD Verbose = 0;
INTERNET_STATUS_CALLBACK PreviousCallback;
HINTERNET hCancel = NULL;
BOOL AsyncMode = FALSE;
BOOL fOffline = FALSE;
DWORD Context = 0;
DWORD AsyncResult = 0;
DWORD AsyncError = 0;
HANDLE AsyncEvent = NULL;
BOOL UseQueryData = FALSE;

#if DBG
BOOL CheckHandleLeak = FALSE;
#endif

//
// external functions
//

extern BOOL DispatchCommand(LPCTSTR, HANDLE);

//
// prototypes
//

void __cdecl main(int, char**);
void __cdecl control_c_handler(int);
void usage(void);
BOOL Prompt(LPCTSTR, LPTSTR*);

//
// functions
//

void __cdecl main(int argc, char** argv) {

    LPTSTR ptszSite = NULL;
    LPTSTR ptszUser = NULL;
    LPTSTR ptszPass = NULL;
    HINTERNET hInternet;
    HINTERNET hFtpSession;
    DWORD dwLocalAccessFlags;
    LPTSTR lpszCmd;
    DWORD lastError;
    DWORD bufLen;
    BOOL enableCallbacks = FALSE;
    DWORD flags = 0;
    DWORD accessMethod = INTERNET_OPEN_TYPE_PRECONFIG;
    BOOL expectingProxy = FALSE;
    LPSTR proxyServer = NULL;

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (*++*argv) {
            case '?':
                usage();
                break;

            case 'a':
                ++*argv;
                if ((**argv == 'l') || (**argv == 'd')) {
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
                enableCallbacks = TRUE;
                break;

#if DBG
            case 'l':
                CheckHandleLeak = TRUE;
                break;
#endif

            case 'n':
                CacheFlags |= INTERNET_FLAG_DONT_CACHE;
                break;

            case 'p':
                flags |= INTERNET_FLAG_PASSIVE;
                break;

            case 'q':
                UseQueryData = TRUE;
                break;

            case 'v':
                if (*++*argv == '\0') {
                    Verbose = 1;
                } else {
                    Verbose = atoi(*argv);
                }
                break;

            case 'x':
                Context = (DWORD)atoi(++*argv);
                break;

            case 'y':
                AsyncMode = TRUE;
                break;
            case 'o':
                fOffline = TRUE;
                break;
            default:
                printf("error: unrecognized command line flag: '%c'\n", **argv);
                usage();
            }
        } else if (expectingProxy) {
            proxyServer = *argv;
            expectingProxy = FALSE;
        } else if (ptszSite == NULL) {
            ptszSite = *argv;
        } else if (ptszUser == NULL) {
            ptszUser = *argv;
        } else if (ptszPass == NULL) {
            ptszPass = *argv;
        } else {
            printf("error: unrecognized command line argument: \"%s\"\n", *argv);
            usage();
        }
    }

    if (ptszSite == NULL) {
        printf("error: required server name argument missing\n");
        exit(1);
    }

    if (AsyncMode) {

        //
        // create auto-reset, initially unsignalled event
        //

        AsyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (AsyncEvent == NULL) {
            print_error("ftpcat", "CreateEvent()");
            exit(1);
        }
    }

    //
    // add a control-c handler
    //

    signal(SIGINT, control_c_handler);

#if DBG
    if (CheckHandleLeak) {
        printf("initial handle count = %d\n", GetProcessHandleCount());
    }
#endif

#if DBG
    if (CheckHandleLeak && Verbose) {
        printf("Initial handle count = %d\n", GetProcessHandleCount());
    }
#endif

    if (Verbose) {
        printf("calling InternetOpen()...\n");
    }
    hInternet = InternetOpen("ftpcat",
                             accessMethod,
                             proxyServer,
                             NULL,
                             AsyncMode ? INTERNET_FLAG_ASYNC : 0
                             | (fOffline ? INTERNET_FLAG_OFFLINE : 0)
                             );
    if (hInternet == NULL) {
        print_error("ftpcat", "InternetOpen()");
        exit(1);
    } else if (Verbose) {
        printf("Internet handle = %x\n", hInternet);
        hCancel = hInternet;
    }

#if DBG
    if (CheckHandleLeak) {
        printf("after InternetOpen(): handle count = %d\n", GetProcessHandleCount());
    }
#endif

    if (enableCallbacks) {

        //
        // let's have a status callback
        //
        // Note that callbacks can be set even before we have opened a handle
        // to the internet/gateway
        //

        PreviousCallback = InternetSetStatusCallback(hInternet, my_callback);
        if (PreviousCallback == INTERNET_INVALID_STATUS_CALLBACK) {
            print_error("ftpcat", "InternetSetStatusCallback()");
        } else if (Verbose) {
            printf("previous Internet callback = %x\n", PreviousCallback);
        }
    }

    if (Verbose) {
        printf("calling InternetConnect()...\n");
    }
    hFtpSession = InternetConnect(hInternet,
                                  ptszSite,
                                  0,
                                  ptszUser,
                                  ptszPass,
                                  INTERNET_SERVICE_FTP,
                                  flags,
                                  FTPCAT_CONNECT_CONTEXT
                                  );
    if ((hFtpSession == NULL) && AsyncMode) {
        if (Verbose) {
            printf("waiting for async InternetConnect()...\n");
        }
        WaitForSingleObject(AsyncEvent, INFINITE);
        hFtpSession = (HINTERNET)AsyncResult;
        SetLastError(AsyncError);
    }
    if (hFtpSession == NULL) {
        if (AsyncMode) {
            SetLastError(AsyncError);
        }
        print_error("ftpcat",
                    "%sInternetConnect()",
                    AsyncMode ? "async " : ""
                    );
        get_response(hFtpSession);
        close_handle(hInternet);
        exit(1);
    } else if (Verbose) {
        printf("FTP session handle = %x\n", hFtpSession);
    }

#if DBG
    if (CheckHandleLeak) {
        printf("after InternetConnect(): handle count = %d\n", GetProcessHandleCount());
    }
#endif

    printf("Connected to %s.\n", ptszSite);

    get_response(hFtpSession);

#if DBG
    if (CheckHandleLeak) {
        printf("after InternetGetLastResponseInfo(): handle count = %d\n", GetProcessHandleCount());
    }
#endif

    //
    // set the (top level) cancellable handle
    //

    hCancel = hFtpSession;

    while (!fQuit) {
        if (Prompt(TEXT("ftp> "), &lpszCmd)) {
            DispatchCommand(lpszCmd, hFtpSession);
        }
    }

    if (Verbose) {
        printf("Closing %x\n", hFtpSession);
    }

    close_handle(hFtpSession);

#if DBG
    if (CheckHandleLeak) {
        printf("after InternetCloseHandle(): handle count = %d\n", GetProcessHandleCount());
    }
#endif

    get_response(hFtpSession);

#if DBG
    if (CheckHandleLeak) {
        printf("after InternetGetLastResponseInfo(): handle count = %d\n", GetProcessHandleCount());
    }
#endif

    close_handle(hInternet);

#if DBG
    if (CheckHandleLeak) {
        printf("after InternetCloseHandle(): handle count = %d\n", GetProcessHandleCount());
    }

    if (CheckHandleLeak && Verbose) {
        printf("Final handle count = %d\n", GetProcessHandleCount());
    }
#endif

    exit(0);
}

void __cdecl control_c_handler(int sig) {

    //
    // disable signals
    //

    signal(SIGINT, SIG_IGN);

    //
    // cancel the current operation
    //

    if (Verbose) {
        printf("control-c handler\n");
    }
    if (hCancel == NULL) {
        if (Verbose) {
            printf("control-c handler: no Internet operation in progress\n");
        }
    } else {
        close_handle(hCancel);
    }

    //
    // re-enable this signal handler
    //

    signal(SIGINT, control_c_handler);
}

void usage() {
    printf("\n"
           "usage: ftpcat [-a{g{[ ]server}|l|d|p}] [-c] [-d] [-n] [-p] [-v] [-x#] [-y]\n"
           "              {servername} [username] [password]\n"
           "\n"
           "where: -a = access type. Default is pre-configured:\n"
           "            g = gateway access via <gateway server>\n"
           "            l = local internet access\n"
           "            d = local internet access\n"
           "            p = CERN proxy access\n"
           "       -c = Enable callbacks\n"
           "       -n = Don't cache\n"
           "       -p = Use Passive transfer mode\n"
           "       -v = Verbose mode. Default is off\n"
           "       -x = Set context value\n"
           "       -y = Asynchronous APIs\n"
           );
    exit(1);
}

BOOL
Prompt(
    IN LPCTSTR pszPrompt,
    OUT LPTSTR* ppszCommand
    )
{
    static CHAR Command[MAX_COMMAND_LENGTH + sizeof(TEXT('\0'))];

#ifdef UNICODE

    static WCHAR wchBuf[MAX_COMMAND_LENGTH + sizeof(L'\0')];

#endif

    DWORD dwBytesRead;
    PTCHAR pch;

    lprintf(TEXT("%s"), pszPrompt);

    if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE),
                  Command,
                  MAX_COMMAND_LENGTH * sizeof(CHAR),
                  &dwBytesRead,
                  NULL)) {
        return FALSE;
    }

    Command[dwBytesRead] = '\0';

#ifdef UNICODE

    wsprintf(wchBuf, L"%S", Command);
    *ppszCommand = wchBuf;

#else

    *ppszCommand = Command;

#endif

    pch = lstrchr(*ppszCommand, TEXT('\r'));

    if (pch) {
        *pch = TEXT('\0');
    }

    return TRUE;
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
                Handle,
                Context,
                (Context == FTPCAT_CONNECT_CONTEXT) ? "Connect"
                : (Context == FTPCAT_FIND_CONTEXT) ? "Find"
                : (Context == FTPCAT_FILE_CONTEXT) ? "File"
                : (Context == FTPCAT_GET_CONTEXT) ? "Get"
                : (Context == FTPCAT_PUT_CONTEXT) ? "Put"
                : (Context == FTPCAT_COMMAND_CONTEXT) ? "Command"
                : (Context == FTPCAT_OPEN_CONTEXT) ? "Open"
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
                printf(Info);
            }
        }
        putchar('\n');
    }
    if (Status == INTERNET_STATUS_REQUEST_COMPLETE) {
        get_response(Handle);
        if (AsyncMode) {
            SetEvent(AsyncEvent);
        } else {
            printf("error: INTERNET_STATUS_REQUEST_COMPLETE returned. Not async\n");
        }
    }
}

void close_handle(HINTERNET handle) {
    if (Verbose) {
        printf("closing handle %#x\n", handle);
    }
    if (!InternetCloseHandle(handle)) {
        print_error("close_handle", "InternetCloseHandle(%x)", handle);
    }
}

#if DBG

DWORD GetProcessHandleCount() {

    DWORD error;
    DWORD count;
    DWORD countSize;

    countSize = sizeof(count);
    if (!InternetQueryOption(NULL,
                             INTERNET_OPTION_GET_HANDLE_COUNT,
                             &count,
                             &countSize
                             )) {
        print_error("GetProcessHandleCount", "InternetQueryOption()");
        return 0;
    }
    return count;
}

#endif
