#include "precomp.h"


#define USAGE(x)                        \
    "Usage:\n"\
    "   %s {s|c} name\n"\
    "   where\n"\
    "       s       -   run as the server (listen);\n"\
    "       c [machine pid number] -   run as the client (connect).\n"\
    ,x

HANDLE      hHelper;
OVERLAPPED  ovlp;

VOID WINAPI WsCompletion (
    DWORD           dwErrorCode,
    DWORD           dwNumberOfBytesTransfered,
    LPOVERLAPPED    lpOverlapped,
    DWORD           Flags
    );

int _cdecl
main (int argc, CHAR **argv) {
    DWORD   rc;
    BOOL    Server;

    if (argc<2) {
        printf (USAGE(argv[0]));
        return 0;
    }

    if (_stricmp (argv[1], "s")==0)
        Server = TRUE;
    else if (_stricmp (argv[1], "c")==0)
        Server = FALSE;
    else {
        printf ("Must specify s(erver) or c(lient).\n");
        printf (USAGE(argv[0]));
        return -1;
    }

    if ((rc=WahOpenNotificationHandleHelper (&hHelper))!=NO_ERROR) {
        printf ("WahOpenNotificationHandleHelper failed, err: %ld.\n", rc);
        return -1;
    }

    if (Server) {
        HANDLE  hSync, hApc, hAEvent, hAPort, hANoPort;
        OVERLAPPED  oApc, oEvent, oPort, oNoPort;
        LPOVERLAPPED lpo;
        HANDLE  hPort;
        HANDLE  hEvent;
        DWORD   count, key;

        hPort = CreateIoCompletionPort (
                    INVALID_HANDLE_VALUE,
                    NULL,
                    0,
                    0
                    );

        if (hPort==INVALID_HANDLE_VALUE) {
            printf ("CreateIoCompletionPort (create) failed, err: %ld.\n", GetLastError ());
            return -1;
        }

        hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
        if (hEvent==NULL) {
            printf ("CreateEvent failed, err: %ld.\n", GetLastError ());
            return -1;
        }

        rc = WahCreateNotificationHandle (hHelper, &hSync);
        if (rc!=0) {
            printf ("WahCreateNotificationHandle failed, err: %ld.\n", rc);
            return -1;
        }

        rc = WahCreateNotificationHandle (hHelper, &hApc);
        if (rc!=0) {
            printf ("WahCreateNotificationHandle failed, err: %ld.\n", rc);
            return -1;
        }
        oApc.hEvent = NULL;

        rc = WahCreateNotificationHandle (hHelper, &hAEvent);
        if (rc!=0) {
            printf ("WahCreateNotificationHandle failed, err: %ld.\n", rc);
            return -1;
        }
        oEvent.hEvent = hEvent;

        rc = WahCreateNotificationHandle (hHelper, &hAPort);
        if (rc!=0) {
            printf ("WahCreateNotificationHandle failed, err: %ld.\n", rc);
            return -1;
        }
        if (CreateIoCompletionPort (hAPort, hPort, (DWORD)hAPort, 0)
               == INVALID_HANDLE_VALUE) {
            printf ("CreateIoCompletionPort (associate) failed, err: %ld.\n", GetLastError ());
            return -1;
        }
        oPort.hEvent = NULL;

        rc = WahCreateNotificationHandle (hHelper, &hANoPort);
        if (rc!=0) {
            printf ("WahCreateNotificationHandle failed, err: %ld.\n", rc);
            return -1;
        }
        if (CreateIoCompletionPort (hANoPort, hPort, (DWORD)hANoPort, 0)
               == INVALID_HANDLE_VALUE) {
            printf ("CreateIoCompletionPort (associate) failed, err: %ld.\n", GetLastError ());
            return -1;
        }
        oNoPort.hEvent = (HANDLE)1;

        while (1) {
            printf ("Posting apc wait.\n");
            rc = WahWaitForNotification (hHelper, hApc, &oApc, WsCompletion);
            if ((rc!=0) && (rc!=WSA_IO_PENDING)) {
                printf ("WahWaitForNotification(apc) failed, err: %ld.\n", rc);
                return -1;
            }

            printf ("Posting event wait.\n");
            rc = WahWaitForNotification (hHelper, hAEvent, &oEvent, NULL);
            if ((rc!=0) && (rc!=WSA_IO_PENDING)) {
                printf ("WahWaitForNotification(event) failed, err: %ld.\n", rc);
                return -1;
            }

            printf ("Posting port wait.\n");
            rc = WahWaitForNotification (hHelper, hAPort, &oPort, NULL);
            if ((rc!=0) && (rc!=WSA_IO_PENDING)) {
                printf ("WahWaitForNotification(port) failed, err: %ld.\n", rc);
                return -1;
            }

            printf ("Posting noport wait.\n");
            rc = WahWaitForNotification (hHelper, hANoPort, &oNoPort, NULL);
            if ((rc!=0) && (rc!=WSA_IO_PENDING)) {
                printf ("WahWaitForNotification(port) failed, err: %ld.\n", rc);
                return -1;
            }

            printf ("Posting sync wait.\n");
            memset (&ovlp, 0, sizeof (ovlp));
            rc = WahWaitForNotification (hHelper, hSync, NULL, NULL);
            if (rc!=0) {
                printf ("WahWaitForNotification failed, err: %ld.\n", rc);
                return -1;
            }

            printf ("Waiting for apc\n");
            rc = SleepEx (INFINITE, TRUE);
            if (rc!=WAIT_IO_COMPLETION) {
                printf ("Unexpected result on wait for apc: %ld.\n", rc);
                return rc;
            }

            printf ("Waiting for port\n");
            if (GetQueuedCompletionStatus (hPort, &count, &key, &lpo, INFINITE)) {
                rc = 0;
            }
            else if (lpo) {
                rc = GetLastError ();
                if (rc==0)
                    printf ("No error code for failed GetQueuedCompletionStatus.\n");
            }
            else {
                printf ("GetQueuedCompletionStatus failed, err: %ld.\n", GetLastError ());
                return -1;
            }
            if (key!=(DWORD)hAPort)
                printf ("Wrong completion key: %lx (expected : %lx)", key, hAPort);
            WsCompletion (rc, count, lpo, 0);

            printf ("Waiting for noport\n");
            oNoPort.hEvent = NULL;
            if (GetOverlappedResult (hANoPort, &oNoPort, &count, TRUE)) {
                rc = 0;
            }
            else {
                rc = GetLastError ();
                if (rc==0)
                    printf ("No error code for failed GetOverlappedResult.\n");
            }
            WsCompletion (rc, count, &oNoPort, 0);

            printf ("Waiting for event\n");
            if (GetOverlappedResult (hAEvent, &oEvent, &count, TRUE)) {
                rc = 0;
            }
            else {
                rc = GetLastError ();
                if (rc==0)
                    printf ("No error code for failed GetOverlappedResult.\n");
            }
            WsCompletion (rc, count, &oEvent, 0);
        }
        CloseHandle (hSync);
        CloseHandle (hANoPort);
        CloseHandle (hAPort);
        CloseHandle (hAEvent);
        CloseHandle (hApc);

        CloseHandle (hPort);
        CloseHandle (hEvent);
    }
    else if (argc<3) {
        rc = WahNotifyAllProcesses (hHelper);
        if (rc==0) {
            printf ("Notified all ok.\n");
        }
        else {
            printf ("WahNotifyAllProcesses failed, err: %ld.\n", rc);
        }
    }
    else if (argc>=5) {
        HANDLE          hPipe;
        DWORD           id = GetCurrentProcessId (), count;
        WCHAR           name[MAX_PATH];


        //
        // Create client and of the pipe that matched the
        // pattern
        //
        wsprintfW (name, L"\\\\%hs\\pipe\\Winsock2\\CatalogChangeListener-%hs-%hs", 
                    argv[2], argv[3], argv[4]);

        hPipe =  CreateFileW (name, 
                                GENERIC_WRITE, 
                                FILE_SHARE_READ, 
                                (LPSECURITY_ATTRIBUTES) NULL, 
                                OPEN_EXISTING, 
                                FILE_ATTRIBUTE_NORMAL, 
                                (HANDLE) NULL);
        if (hPipe!=INVALID_HANDLE_VALUE) {
            printf ("Opened pipe %ls.\n", name);
            CloseHandle (hPipe);
        }
        else {
            printf ("Could not open pipe %ls, err: %ld\n",
                name, GetLastError ());
        }
    }
    else
        printf (USAGE(argv[0]));

    WahCloseNotificationHandleHelper (hHelper);
    return 0;
}

VOID WINAPI WsCompletion (
    DWORD           dwErrorCode,
    DWORD           dwNumberOfBytesTransfered,
    LPOVERLAPPED    lpOverlapped,
    DWORD           dwFlags
    ) {
    if (dwErrorCode!=0) {
        printf ("Falure in WsCompletion, err: %ld.\n", dwErrorCode);
        ExitProcess (-1);
    }
    else {
        printf ("WsComletion: %d bytes transferred.\n", dwNumberOfBytesTransfered);
    }
}
