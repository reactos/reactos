/*****************************************************************************
 *
 *  PROXYcc
 *
 *  Copyright (c) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Does all the bookkeeping part of proxying.
 *
 *****************************************************************************/

#include "msnspa.h"

/*****************************************************************************
 *
 *      init_send_socket
 *
 *      Create a socket that talks to the real world.
 *
 *****************************************************************************/

SOCKET INTERNAL
init_send_socket(SOCKET scfd, LPCSTR pszHost, u_short port, LPCSTR pszErrMsg)
{
    SOCKET s;
    struct hostent *phe;
    struct sockaddr_in saddr;

    /*
     * Find out who the target is.
     */
    ZeroMemory(&saddr, sizeof(saddr));
    phe = gethostbyname(pszHost);

    if (!phe) {
        Squirt("Couldn't build address of gateway");
        send(scfd, pszErrMsg, lstrlen(pszErrMsg), 0);
        s = INVALID_SOCKET;
        goto done;
    }

    /*
     * Build the socket address packet for the open.
     */
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    CopyMemory(&saddr.sin_addr, phe->h_addr, phe->h_length);

    /*
     * Open sesame.
     */
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        Squirt("Couldn't create send socket\r\n");
        send(scfd, pszErrMsg, lstrlen(pszErrMsg), 0);
        s = INVALID_SOCKET;
        goto done;
    }

    /*
     * One ringy-dingy...
     */
    if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr))) {
        Squirt("Couldn't connect");
        closesocket(s);
        send(scfd, pszErrMsg, lstrlen(pszErrMsg), 0);
        s = INVALID_SOCKET;
        goto done;
    }

done:;
    return s;

}

/*****************************************************************************
 *
 *      set_sock_opt_int
 *
 *  Set an integer socket option or die trying.
 *
 *****************************************************************************/

void
set_sock_opt_int(SOCKET s, int optname, int val)
{
    if (setsockopt(s, SOL_SOCKET, optname, (PV)&val, sizeof(val)) == -1) {
        Die("set sock opt");
    }
}

/*****************************************************************************
 *
 *      create_listen_socket
 *
 *      Start listening on a port.
 *
 *****************************************************************************/

SOCKET INTERNAL
create_listen_socket(u_short port)
{
    SOCKET isckdes;
    struct hostent *phe;                /* my host entry table */
    struct sockaddr_in saddr;           /* my socket address */
    char hostname[64];

    /*
     * Find out who I am.
     */
    gethostname(hostname, 64);
    phe = gethostbyname(hostname);	/* Get my own hostent */

    if (!phe) {
        Die("Couldn't build address of localhost");
        return INVALID_SOCKET;
    }

    /*
     * Build the socket address packet for the open.
     */
    ZeroMemory(&saddr, sizeof(saddr));  /* start fresh */
    CopyMemory(&saddr.sin_addr, phe->h_addr, phe->h_length); /* Copy the IP */

    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(port); /* Listen on this port */

    /*
     * Open sesame.
     */
    isckdes = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (isckdes == INVALID_SOCKET) {
        Die("Couldn't create listen socket");
        return INVALID_SOCKET;
    }

    /*
     * Set some socket options.
     */
    set_sock_opt_int(isckdes, SO_REUSEADDR, 1);
    set_sock_opt_int(isckdes, SO_KEEPALIVE, 1);

    /*
     * All right, let's bind to it already.
     */
    if (bind(isckdes, (struct sockaddr *)&saddr, sizeof(saddr)) == -1) {
        Die("Couldn't bind to recv. socket");
    }

    return isckdes;
}

/*****************************************************************************
 *
 *      ProxyPeekCommand
 *
 *      Study the incoming command to see if it is something we
 *      have a canned response to.
 *
 *****************************************************************************/

BOOL INTERNAL
ProxyPeekCommand(PCONNECTIONSTATE pcxs)
{
    PPROXYINFO pproxy = pcxs->pproxy;

    /*
     *  Now peek to see if we got a specific ignorable
     *  four-letter command from
     *  the client.  If so, then spit back the canned response.
     */
    if (pcxs->nread > 4 && pcxs->buf[4] == ' ') {
        char szWord[5];
        szWord[0] = pcxs->buf[0];
        szWord[1] = pcxs->buf[1];
        szWord[2] = pcxs->buf[2];
        szWord[3] = pcxs->buf[3];
        szWord[4] = 0;

        /*
         *  Are the first four letters an ignored command?
         */
        if (lstrcmpi(szWord, pproxy->szIgnore1) == 0 ||
            lstrcmpi(szWord, pproxy->szIgnore2) == 0) {

            /*
             *  Then spit back the canned response.
             */


            if (sendsz(pcxs->scfd, pproxy->pszResponse) == SOCKET_ERROR) {
                Squirt("Write failed" EOL);
            }
            return TRUE;

        }
    }

    return FALSE;
}

/*****************************************************************************
 *
 *      PROXYTHREADSTATE
 *
 *      Tiny chunk of memory used to transfer proxy state between
 *      the ProxyThread() and the ProxyWorkerThread().
 *
 *****************************************************************************/

typedef struct PROXYTHREADSTATE {
    PPROXYINFO pproxy;                  /* Who we are */
    SOCKET scfd;                        /* Newly-accepted socket to client */
} PROXYTHREADSTATE, *PPROXYTHREADSTATE;

/*****************************************************************************
 *
 *      ProxyWorkerThread
 *
 *      Hold two phones together.
 *
 *****************************************************************************/

DWORD WINAPI
ProxyWorkerThread(LPVOID pvRef)
{
    PPROXYTHREADSTATE ppts = pvRef;
    CONNECTIONSTATE cxs;

    cxs.scfd = ppts->scfd;
    cxs.pproxy = ppts->pproxy;

    LocalFree(ppts);

    Squirt("Connection %d..." EOL, GetCurrentThreadId());

    ++*cxs.pproxy->piUsers;
    UI_UpdateCounts();

    /* open the target socket */
    cxs.ssfd = init_send_socket(cxs.scfd,
                                cxs.pproxy->pszHost,
                                cxs.pproxy->serverport,
                                cxs.pproxy->pszError);

    if (cxs.ssfd != INVALID_SOCKET) {
#if 0
        Squirt("ssfd = %d; scfd = %d, &ssfd = %08x" EOL,
               cxs.ssfd, cxs.scfd, &cxs.ssfd);
#endif

        if (!cxs.pproxy->Negotiate(cxs.ssfd)) {
            sendsz(cxs.scfd, cxs.pproxy->pszErrorPwd);
            goto byebye;
        }

        sendsz(cxs.scfd, cxs.pproxy->pszResponse);

        for (;;) {
            fd_set fdrd, fder;
            SOCKET sfrom, sto;

            fdrd.fd_count = 2;
            fdrd.fd_array[0] = cxs.ssfd;
            fdrd.fd_array[1] = cxs.scfd;

            fder.fd_count = 2;
            fder.fd_array[0] = cxs.ssfd;
            fder.fd_array[1] = cxs.scfd;

            cxs.nread = select(32, &fdrd, 0, &fder, 0);

            if (cxs.nread != SOCKET_ERROR) {
                char *ptszSrc;
                char *ptszDst;

                if (fder.fd_count) {        /* error on a socket, e.g., EOF */
                    break;                  /* outta here */
                }

                if (fdrd.fd_count == 0) {   /* Huh?? */
                    continue;
                }

                if (fdrd.fd_array[0] == cxs.scfd) {
                    sfrom = cxs.scfd; sto = cxs.ssfd;
                } else if (fdrd.fd_array[0] == cxs.ssfd) {
                    sfrom = cxs.ssfd; sto = cxs.scfd;
                } else {
                    continue;
                }

                cxs.nread = recv(sfrom, cxs.buf, BUFSIZE, 0); /* read a hunk */

                if (cxs.nread > 0) {

                    /*
                     *  If it's from the client, then peek at it
                     *  in case we need to munge it.
                     */
                    if (sfrom == cxs.scfd) {
                        if (ProxyPeekCommand(&cxs)) {
                            continue;
                        }
                    }

                    if (send(sto, cxs.buf, cxs.nread, 0) == SOCKET_ERROR) {
                        Squirt("Write failed" EOL);
                    }
#ifdef DBG
                    cxs.buf[cxs.nread] = 0;
                    if (sto == cxs.scfd) {
                        /*
                         *  Walk the buffer studying each line.
                         */
                        int ich = 0;
                        while (ich < cxs.nread) {
                            int ichEnd;
                            DWORD dwFirst;

                            for (ichEnd = ich;
                                 ichEnd < cxs.nread &&
                                 cxs.buf[ichEnd] != '\n'; ichEnd++) {
                            }

                            dwFirst = *(LPDWORD)&cxs.buf[ich];
                            #define PLUSOK      0x004B4F2B
                            #define DASHERR     0x5252452D
                            #define SUBJECT     0x6A627553

                            if ((dwFirst & 0x00FFFFFF) == PLUSOK ||
                                dwFirst == DASHERR ||
                                dwFirst == SUBJECT) {

                                cxs.buf[ichEnd] = 0;
                                Squirt("<%s\n", &cxs.buf[ich]);
                            }

                            ich = ichEnd + 1;
                        }

                    } else {
                        Squirt(">%s", cxs.buf);
                    }

#endif
                } else {                /* EOF */
                    break;
                }
            } else {                    /* Panic */
                Squirt("select %d", WSAGetLastError());
                break;
            }
        }
    byebye:;
        Sleep(250);                     /* wait for socket to drain */
        closesocket(cxs.ssfd);
    }
    closesocket(cxs.scfd);
    Squirt("End connection %d..." EOL, GetCurrentThreadId());

    --*cxs.pproxy->piUsers;
    UI_UpdateCounts();

    return 0;
}

/*****************************************************************************
 *
 *      ProxyThread
 *
 *      Thread procedure for proxies.
 *
 *****************************************************************************/

DWORD CALLBACK
ProxyThread(LPVOID pvRef)
{
    PPROXYINFO pproxy = pvRef;
    SOCKET ic_sck;
    SOCKET scfd;

    ic_sck = create_listen_socket(pproxy->localport);

    for (;;) {
        HANDLE hThread;
        DWORD dwThid;
        PPROXYTHREADSTATE ppts;

        Squirt("listening..." EOL);
        if (listen(ic_sck, SOMAXCONN) == -1) {
            Squirt("listen failed %d" EOL, WSAGetLastError());
            // BUGBUG -- Win95 sucks.  Close the socket and try again
            closesocket(ic_sck);
            ic_sck = create_listen_socket(pproxy->localport);
            continue;
        }

        /*
         *  OLD COMMENT
         *
         * We ought to put a timeout in here, and then
         * if there are no connections, reap any zombies
         * and go back to listening... so if we've blocked some
         * sockets other people don't get refused... -- mikeg
         */

        Squirt("accept waiting..." EOL);
        scfd = accept(ic_sck, NULL, NULL); /* wait for a connection */

        if (scfd == INVALID_SOCKET) {
            Squirt("accept failed %d" EOL, WSAGetLastError());
            break;
        }

        ppts = LocalAlloc(LMEM_FIXED, sizeof(PROXYTHREADSTATE));
        if (ppts) {
            ppts->pproxy = pproxy;
            ppts->scfd = scfd;

            hThread = CreateThread(0, 0, ProxyWorkerThread, ppts, 0, &dwThid);
            if (hThread) {
                CloseHandle(hThread);
            } else {
                Squirt("Can't spawn worker thread; tossing connection" EOL);
                closesocket(scfd);
            }
        } else {
            Squirt("Out of memory; tossing connection" EOL);
            closesocket(scfd);
        }
    }
    closesocket(ic_sck);
    return 0;
}
