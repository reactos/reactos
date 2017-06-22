/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for client/server authentication via secur32 API.
 * PROGRAMMERS:     Samuel Serapión
 */

#include "client_server.h"

#define PackageName L"NTLM"

PSecurityFunctionTable server1_SecFuncTable = NULL;

BOOL auth(
    IN  SOCKET s,
    OUT PCredHandle pCred,
    OUT PCtxtHandle pSrvCtx)
{
    int rc;
    BOOL haveToken = TRUE;
    SecPkgInfoW *secPackInfo;
    int bytesReceived = 0, bytesSent = 0;
    TimeStamp useBefore;

    /* Input and output buffers */
    SecBufferDesc obd, ibd;
    SecBuffer ob, ib;

    DWORD ctxAttr;
    BOOL haveContext = FALSE;

    trace("auth() entered");

    rc = server1_SecFuncTable->QuerySecurityPackageInfoW(PackageName, &secPackInfo);
    trace("QSPI(): %08xh\n", rc);
    if (rc != SEC_E_OK)
        haveToken = FALSE;

    rc = server1_SecFuncTable->AcquireCredentialsHandleW(NULL, PackageName, SECPKG_CRED_INBOUND,
        NULL, NULL, NULL, NULL, pCred, &useBefore);
    trace("ACH(): %08xh\n", rc);
    if (rc != SEC_E_OK)
        haveToken = FALSE;

    while (1)
    {
        char *p;
        int n;

        /* Prepare to get the server's response */
        ibd.ulVersion = SECBUFFER_VERSION;
        ibd.cBuffers = 1;
        ibd.pBuffers = &ib; // just one buffer
        ib.BufferType = SECBUFFER_TOKEN; // preping a token here

        /* Receive the client's POD */
        // MACHINE-DEPENDENT CODE! (Besides, we assume that we
        // get the length with a single read, which is not guaranteed)
        recv(s, (char *) &ib.cbBuffer, sizeof(ib.cbBuffer), 0);
        bytesReceived += sizeof(ib.cbBuffer);
        ib.pvBuffer = LocalAlloc(0, ib.cbBuffer);

        p = (char *)ib.pvBuffer;
        n = ib.cbBuffer;
        while (n)
        {
            rc = recv(s, p, n, 0);
            if (rc == SOCKET_ERROR)
                wserr(rc, L"recv");
            if (rc == 0)
                wserr(999, L"recv");
            bytesReceived += rc;
            n -= rc;
            p += rc;
        }

        /* By now we have an input buffer */

        obd.ulVersion = SECBUFFER_VERSION;
        obd.cBuffers = 1;
        obd.pBuffers = &ob; // just one buffer
        ob.BufferType = SECBUFFER_TOKEN; // preping a token here
        ob.cbBuffer = secPackInfo->cbMaxToken;
        ob.pvBuffer = LocalAlloc(0, ob.cbBuffer);

        rc = server1_SecFuncTable->AcceptSecurityContext(pCred, haveContext? pSrvCtx: NULL,
            &ibd, 0, SECURITY_NATIVE_DREP, pSrvCtx, &obd, &ctxAttr, &useBefore);
        trace("ASC(): %08xh\n", rc);

        if (ib.pvBuffer != NULL)
        {
            LocalFree(ib.pvBuffer);
            ib.pvBuffer = NULL;
        }

        if (rc == SEC_I_COMPLETE_AND_CONTINUE || rc == SEC_I_COMPLETE_NEEDED)
        {
            if (server1_SecFuncTable->CompleteAuthToken != NULL) // only if implemented
                server1_SecFuncTable->CompleteAuthToken(pSrvCtx, &obd);
            if (rc == SEC_I_COMPLETE_NEEDED)
                rc = SEC_E_OK;
            else if (rc == SEC_I_COMPLETE_AND_CONTINUE)
                rc = SEC_I_CONTINUE_NEEDED;
        }

        /* Send the output buffer off to the server */
        // WARNING -- this is machine-dependent! FIX IT!
        if (rc == SEC_E_OK || rc == SEC_I_CONTINUE_NEEDED)
        {
            if (ob.cbBuffer != 0)
            {
                send(s, (const char *) &ob.cbBuffer, sizeof(ob.cbBuffer), 0);
                bytesSent += sizeof(ob.cbBuffer);
                send(s, (const char *) ob.pvBuffer, ob.cbBuffer, 0);
                bytesSent += ob.cbBuffer;
            }
            LocalFree(ob.pvBuffer);
            ob.pvBuffer = NULL;
        }

        if (rc != SEC_I_CONTINUE_NEEDED)
            break;

        haveContext = TRUE;

        /* Loop back for another round */
        puts("looping");
    }

    /*
     * We arrive here as soon as InitializeSecurityContext()
     * returns != SEC_I_CONTINUE_NEEDED.
     */

    if (rc != SEC_E_OK)
    {
        err("Oops! ASC() returned %08xh!\n", rc);
        haveToken = FALSE;
    }

    /* Now we try to use the context */
    rc = server1_SecFuncTable->ImpersonateSecurityContext(pSrvCtx);
    trace("ImpSC(): %08xh\n", rc);
    if (rc != SEC_E_OK)
    {
        err("Oops! ImpSC() returns %08xh!\n", rc);
        haveToken = FALSE;
    }
    else
    {
        WCHAR buf[256];
        DWORD bufsiz = ARRAYSIZE(buf);
        GetUserNameW(buf, &bufsiz);
        trace("user name: \"%s\"\n", buf);
        server1_SecFuncTable->RevertSecurityContext(pSrvCtx);
        trace("RSC(): %08xh\n", rc);
    }

    server1_SecFuncTable->FreeContextBuffer(secPackInfo);

    trace("auth() exiting (%d received, %d sent)\n", bytesReceived, bytesSent);
    return haveToken;
}

int server1_main(
    char *portstr)
{
    int rc, port, addrlen;
    BOOL haveToken;
    SOCKET sock, s;
    WSADATA wsadata;
    PSERVENT pse;
    SOCKADDR_IN addr;
    HINSTANCE hSecLib;
    CredHandle cred;
    CtxtHandle srvCtx;

    initSecLib(&hSecLib, &server1_SecFuncTable);

    rc = WSAStartup(2, &wsadata);
    wserr(rc, L"WSAStartup");

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
        wserr(999, L"socket");

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    /* Try numeric protocol first */
    port = atoi(portstr);
    if (port > 0 && port < 32768)
    {
        addr.sin_port = htons((short)port);
    }
    else
    {
        pse = getservbyname(portstr, "tcp");
        if (pse == NULL)
            wserr(1, L"getservbyname");
        addr.sin_port = pse->s_port;
    }

    rc = bind(sock, (SOCKADDR *) &addr, sizeof(addr));
    wserr(rc, L"bind");

    rc = listen(sock, 2);
    wserr(rc, L"listen");

    while (1)
    {
        addrlen = sizeof(addr);
        s = accept(sock, (SOCKADDR *) &addr, &addrlen);
        if (s == INVALID_SOCKET)
            wserr(s, L"accept");

        haveToken = auth(s, &cred, &srvCtx);

        /* Now we talk to the client */
        trace("haveToken = %s\n\n", haveToken? "true": "false");
        send(s, (const char *) &haveToken, sizeof(haveToken), 0);

        /* Clean up */
        server1_SecFuncTable->DeleteSecurityContext(&srvCtx);
        server1_SecFuncTable->FreeCredentialHandle(&cred);
        closesocket(s);
    }

    FreeLibrary(hSecLib);

    return 0;
}

#if 0
int main(int argc, _TCHAR* argv[])
{
    //if (argc != 1)
    //{
    //  puts("usage: server portnumber");
    //  return 1;
    //}

    return server1_main(argv[1]);
}
#endif
