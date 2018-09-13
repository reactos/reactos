/*****************************************************************************
 *
 *  nntp.c
 *
 *  Copyright (c) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Dumb layer that opens the socket to the server.
 *
 *****************************************************************************/

#include "msnspa.h"

#define PROXY_PORT1         119                     /* I listen to this */
#define PROXY_DEST1         119                     /* And talk to this */
#define PROXY_HOST1         "msnnews.msn.com"       /* And I talk to him */
#define PROTOCOL            "MSN"                   /* with this protocol */

#define PROXY_PORT2         120                     /* I listen to this */
#define PROXY_DEST2         119                     /* And talk to this */
#define PROXY_HOST2         "netnews.msn.com"       /* And I talk to him */

BOOL CALLBACK NNTP_Negotiate(SOCKET s);

/*****************************************************************************
 *
 *      PROXYINFO for NNTP
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

PROXYINFO g_proxyNntp1 = {
    PROXY_PORT1,                        /* localport */
    PROXY_DEST1,                        /* serverport */
    PROXY_HOST1,                        /* remote server */
    &g_cNewsUsers,                      /* Usage counter */
    NNTP_Negotiate,                     /* Negotiation function */
    "400 Server inaccessible\r\n",      /* Failed to connect */
    "400 Authentication failed\r\n",    /* Password problem */
    "200 MSN Secure Password Authentication Proxy\r\n", /* Generic happy */
    "",                                 /* First 4-char command to ignore */
    "",                                 /* Second 4-char command to ignore */
};

PROXYINFO g_proxyNntp2 = {
    PROXY_PORT2,                        /* localport */
    PROXY_DEST2,                        /* serverport */
    PROXY_HOST2,                        /* remote server */
    &g_cNewsUsers,                      /* Usage counter */
    NNTP_Negotiate,                     /* Negotiation function */
    "400 Server inaccessible\r\n",      /* Failed to connect */
    "400 Authentication failed\r\n",    /* Password problem */
    "200 MSN Secure Password Authentication Proxy\r\n", /* Generic happy */
    "",                                 /* First 4-char command to ignore */
    "",                                 /* Second 4-char command to ignore */
};

/*****************************************************************************
 *
 *  NNTP_Negotiate
 *
 *      Perform an authenticated MSN logon.
 *
 *****************************************************************************/

BOOL CALLBACK
NNTP_Negotiate(SOCKET ssfd)
{
    WIN32AUTHSTATE was;
    char buf[BUFSIZE+1];
    int cb;

    /*
     *  Wait for the greeting.
     */
    cb = recv(ssfd, buf, BUFSIZE, 0);   /* read a hunk */
#ifdef CHATTY
    if (cb >= 0) {
        buf[cb] = 0;
        Squirt("<%s\r\n", buf);
    }
#endif

    if (cb <= 0 || buf[0] != '2') {
        return FALSE;
    }

    /*
     *  Tell the server to go into authentication mode.
     */
    sendsz(ssfd, "AUTHINFO TRANSACT " PROTOCOL "\r\n");
#ifdef CHATTY
    Squirt(">AUTHINFO TRANSACT " PROTOCOL "\r\n");
#endif


    /*
     *  Wait for the Proceed.
     */
    cb = recv(ssfd, buf, BUFSIZE, 0);   /* read a hunk */

#ifdef CHATTY
    if (cb >= 0) {
        buf[cb] = 0;
        Squirt("<%s\r\n", buf);
    }
#endif

    if (cb <= 0 || buf[0] != '3') {
        return FALSE;
    }

    if (!Security_AcquireCredentials(&was, TEXT(PROTOCOL))) {
        Die(TEXT("Cannot acquire credentials handle"));
    }

    if (!Security_GetNegotiation(&was)) {
        Die(TEXT("Cannot get negotiation string"));
    }

    /*
     *  Now send the initial cookie.
     */
    sendsz(ssfd, "AUTHINFO TRANSACT ");
    sendsz(ssfd, was.szBuffer);
    sendsz(ssfd, "\r\n");

#ifdef CHATTY
    Squirt(">AUTHINFO TRANSACT %s\r\n", was.szBuffer);
#endif

    /*
     *  Response should be
     *
     *  381 <challenge>
     */
    cb = recv(ssfd, buf, BUFSIZE, 0);

    if (cb <= 0 || buf[0] != '3') {
        return FALSE;
    }

#ifdef CHATTY
    buf[cb] = 0;
    Squirt("<%s", buf);
#endif

    /*
     *  Parse the server response and build our response.
     */
    if (!Security_GetResponse(&was, buf + 4)) {
        Die(TEXT("Cannot build response"));
    }

    /*
     *  Now send our response.
     */
    sendsz(ssfd, "AUTHINFO TRANSACT ");
    sendsz(ssfd, was.szBuffer);
    sendsz(ssfd, "\r\n");
#ifdef CHATTY
    Squirt(">AUTHINFO TRANSACT %s\r\n", was.szBuffer);
#endif

    Security_ReleaseCredentials(&was);

    /*
     *  Now see how that worked.  Response should be
     *
     *  281 OK
     */
    cb = recv(ssfd, buf, BUFSIZE, 0);

    if (cb <= 0 || buf[0] != '2') {
        return FALSE;
    }

    return TRUE;
}
