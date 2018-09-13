/*****************************************************************************
 *
 *  pop3.c
 *
 *  Copyright (c) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Prepares for and waits for client connections.
 *
 *****************************************************************************/

#include "msnspa.h"

#define PROXY_PORT          110                     /* I listen to this */
#define PROXY_DEST          110                     /* And talk to this */
#define PROXY_HOST          "pop3.email.msn.com"    /* And I talk to him */
#define PROTOCOL            "MSN"                   /* with this protocol */

BOOL CALLBACK POP3_Negotiate(SOCKET s);

/*****************************************************************************
 *
 *      PROXYINFO for POP3
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

PROXYINFO g_proxyPop = {
    PROXY_PORT,                         /* localport */
    PROXY_DEST,                         /* serverport */
    PROXY_HOST,                         /* remote server */
    &g_cMailUsers,                      /* Usage counter */
    POP3_Negotiate,                     /* Negotiation function */
    "-ERR Server inaccessible\r\n",     /* Failed to connect */
    "-ERR Authentication failed\r\n",   /* Password problem */
    "+OK MSN Secure Password Authentication Proxy\r\n", /* Generic happy */
    "USER",                             /* First 4-char command to ignore */
    "PASS",                             /* Second 4-char command to ignore */

};

/*****************************************************************************
 *
 *  POP3_Negotiate
 *
 *      Perform an authenticated MSN logon.
 *
 *****************************************************************************/

BOOL CALLBACK
POP3_Negotiate(SOCKET ssfd)
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

    if (cb <= 0 || buf[0] != '+') {
        return FALSE;
    }

    /*
     *  Tell the server to go into authentication mode.
     */
    sendsz(ssfd, "AUTH " PROTOCOL "\r\n");
#ifdef CHATTY
    Squirt(">AUTH " PROTOCOL "\r\n");
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

    if (cb <= 0 || buf[0] != '+') {
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
    sendsz(ssfd, was.szBuffer);
    sendsz(ssfd, "\r\n");

#ifdef CHATTY
    Squirt(">%s\r\n", was.szBuffer);
#endif

    /*
     *  Response should be
     *
     *  + <challenge>
     */
    cb = recv(ssfd, buf, BUFSIZE, 0);

    if (cb <= 0 || buf[0] != '+') {
        return FALSE;
    }

#ifdef CHATTY
    buf[cb] = 0;
    Squirt("<%s", buf);
#endif

    /*
     *  Parse the server response and build our response.
     */
    if (!Security_GetResponse(&was, buf + 2)) {
        Die(TEXT("Cannot build response"));
    }

    /*
     *  Now send our response.
     */
    sendsz(ssfd, was.szBuffer);
    sendsz(ssfd, "\r\n");
#ifdef CHATTY
    Squirt(">%s\r\n", was.szBuffer);
#endif

    Security_ReleaseCredentials(&was);

    /*
     *  Now see how that worked.  Response should be
     *
     *  + OK
     */
    cb = recv(ssfd, buf, BUFSIZE, 0);

    if (cb <= 0 || buf[0] != '+') {
        return FALSE;
    }

    return TRUE;
}
