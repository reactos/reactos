/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for client/server authentication via secur32 API.
 * PROGRAMMERS:     Samuel Serapión
 */

#include "client_server.h"

#define PackageName L"NTLM"

PSecurityFunctionTable client1_SecFuncTable = NULL;

void auth(
    IN  SOCKET s,
    OUT PCredHandle pCred,
    OUT PCtxtHandle pCliCtx,
    IN  LPCWSTR tokenSource,
    IN  LPCWSTR name OPTIONAL,
    IN  LPCWSTR pwd OPTIONAL,
    IN  LPCWSTR domain OPTIONAL)
{
    int rc, rcISC;
    SecPkgInfo *secPackInfo;
    SEC_WINNT_AUTH_IDENTITY_W *nameAndPwd = NULL;
    int bytesReceived = 0, bytesSent = 0;
    LPWSTR myTokenSource;

    TimeStamp useBefore;

    /* Input and output buffers */
    SecBufferDesc obd, ibd;
    SecBuffer ob, ib;

    DWORD ctxReq, ctxAttr;

    BOOL haveInbuffer = FALSE;
    BOOL haveContext = FALSE;

    trace("auth() entered\n");

    // the arguments to ISC() is not const ... for once, I decided
    // on creating writable copies instead of using a brutalizing cast.
    myTokenSource = _wcsdup(tokenSource);

    if (name != NULL)
    {
        nameAndPwd = (SEC_WINNT_AUTH_IDENTITY_W *)malloc(sizeof(*nameAndPwd));
        memset(nameAndPwd, '\0', sizeof(*nameAndPwd));
        nameAndPwd->Domain = (unsigned short *)_wcsdup(domain? domain: L"");
        nameAndPwd->DomainLength = domain? wcslen(domain): 0;
        nameAndPwd->User = (unsigned short *)_wcsdup(name? name: L"");
        nameAndPwd->UserLength = name? wcslen(name): 0;
        nameAndPwd->Password = (unsigned short *)_wcsdup(pwd? pwd: L"");
        nameAndPwd->PasswordLength = pwd? wcslen(pwd): 0;
        nameAndPwd->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    }

    rc = client1_SecFuncTable->QuerySecurityPackageInfo(PackageName, &secPackInfo);
    trace("QuerySecurityPackageInfo(NTLM): returned %08xh\n", rc);

    rc = client1_SecFuncTable->AcquireCredentialsHandle(NULL, PackageName, SECPKG_CRED_OUTBOUND,
        NULL, nameAndPwd, NULL, NULL, pCred, &useBefore);
    trace("AcquireCredentialsHandle(NTLM): returned %08xh\n", rc);

    ctxReq = ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT |
        ISC_REQ_CONFIDENTIALITY | ISC_REQ_DELEGATE;

    ib.pvBuffer = NULL;

    while (1)
    {
        char* p;
        int n;

        obd.ulVersion = SECBUFFER_VERSION;
        obd.cBuffers = 1;
        obd.pBuffers = &ob; // just one buffer
        ob.BufferType = SECBUFFER_TOKEN; // preping a token here
        ob.cbBuffer = secPackInfo->cbMaxToken;
        ob.pvBuffer = LocalAlloc(0, ob.cbBuffer);

        rcISC = client1_SecFuncTable->InitializeSecurityContext(pCred, haveContext? pCliCtx: NULL,
            myTokenSource, ctxReq, 0, SECURITY_NATIVE_DREP, haveInbuffer? &ibd: NULL,
            0, pCliCtx, &obd, &ctxAttr, &useBefore);
        trace("InitializeSecurityContext(): returned %08xh\n", rcISC);

        if (ib.pvBuffer != NULL)
        {
            LocalFree(ib.pvBuffer);
            ib.pvBuffer = NULL;
        }

        if (rcISC == SEC_I_COMPLETE_AND_CONTINUE || rcISC == SEC_I_COMPLETE_NEEDED)
        {
            if (client1_SecFuncTable->CompleteAuthToken != NULL) // only if implemented
                client1_SecFuncTable->CompleteAuthToken(pCliCtx, &obd);
            if (rcISC == SEC_I_COMPLETE_NEEDED)
                rcISC = SEC_E_OK;
            else if (rcISC == SEC_I_COMPLETE_AND_CONTINUE)
                rcISC = SEC_I_CONTINUE_NEEDED;
            trace("SEC_I_COMPLETE_AND_CONTINUE or SEC_I_COMPLETE_NEEDED\n");
        }

        /* Send the output buffer off to the server */
        // WARNING -- this is machine-dependent! FIX IT!
        if (ob.cbBuffer != 0)
        {
            send(s, (char*)&ob.cbBuffer, sizeof(ob.cbBuffer), 0);
            bytesSent += sizeof(ob.cbBuffer);
            send(s, (char*)ob.pvBuffer, ob.cbBuffer, 0);
            bytesSent += ob.cbBuffer;
        }
        LocalFree(ob.pvBuffer);
        ob.pvBuffer = NULL;

        if (rcISC != SEC_I_CONTINUE_NEEDED)
            break;

        /* Prepare to get the server's response */
        ibd.ulVersion = SECBUFFER_VERSION;
        ibd.cBuffers = 1;
        ibd.pBuffers = &ib; // just one buffer
        ib.BufferType = SECBUFFER_TOKEN; // preping a token here

        /* Receive the server's response */
        // MACHINE-DEPENDENT CODE! (Besides, we assume that we
        // get the length with a single read, which is not guaranteed)
        recv(s, (char*)&ib.cbBuffer, sizeof(ib.cbBuffer), 0);
        bytesReceived += sizeof(ib.cbBuffer);
        ib.pvBuffer = LocalAlloc(0, ib.cbBuffer);

        p = (char*)ib.pvBuffer;
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

        /* By now we have an input buffer and a client context */
        haveInbuffer = TRUE;
        haveContext = TRUE;

        /* Loop back for another round */
        trace("looping");
    }

    /*
     * We arrive here as soon as InitializeSecurityContext()
     * returns != SEC_I_CONTINUE_NEEDED.
     */
    if (rcISC != SEC_E_OK)
        err("Oops! InitializeSecurityContext() returned %08xh!\n", rcISC);

    client1_SecFuncTable->FreeContextBuffer(secPackInfo);
    trace("auth() exiting (%d sent, %d received)\n", bytesSent, bytesReceived);
    free(myTokenSource);
    if (nameAndPwd != 0)
    {
        if (nameAndPwd->Domain != 0)
            free(nameAndPwd->Domain);
        if (nameAndPwd->User != 0)
            free(nameAndPwd->User);
        if (nameAndPwd->Password != 0)
            free(nameAndPwd->Password);
        free(nameAndPwd);
    }
}

int client1_main(
    char *server,
    char *portstr,
    char *tokenSource,
    char *user,
    char *pwd,
    char *domain)
{
    int rc, port;
    unsigned long naddr;
    SOCKET sock;
    WSADATA wsadata;
    PHOSTENT phe;
    PSERVENT pse;
    SOCKADDR_IN addr;
    HINSTANCE hSecLib;

    CredHandle cred;
    CtxtHandle cliCtx;

    struct sockaddr name;
    int namelen = sizeof(name);

    size_t converted, strsize;
    LPWSTR tokenW, userW, pwdW, domainW;
    BOOL haveToken = FALSE;

    initSecLib(&hSecLib, &client1_SecFuncTable);

    rc = WSAStartup(2, &wsadata);
    wserr(rc, L"WSAStartup");

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
        wserr(999, L"socket");

    addr.sin_family = AF_INET;
    // try numeric IP address first (inet_addr)
    naddr = inet_addr(server);
    if (naddr != INADDR_NONE)
    {
        addr.sin_addr.s_addr = naddr;
    }
    else
    {
        phe = gethostbyname(server);
        if (phe == NULL)
            wserr(1, L"gethostbyname");
        addr.sin_addr.s_addr = *((unsigned long *)(phe->h_addr));
        memcpy((LPWSTR)&addr.sin_addr, phe->h_addr, phe->h_length);
    }

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

    rc = connect(sock, (SOCKADDR *)&addr, sizeof(addr));
    wserr(rc, L"connect");

    rc = getsockname(sock, &name, &namelen);
    wserr(rc, L"getsockname()");
    wprintf(L"I am %u.%u.%u.%u\n",
            (unsigned int)(unsigned char)name.sa_data[2],
            (unsigned int)(unsigned char)name.sa_data[3],
            (unsigned int)(unsigned char)name.sa_data[4],
            (unsigned int)(unsigned char)name.sa_data[5]);

    strsize = strlen(tokenSource)+1;
    tokenW = (LPWSTR)malloc(strsize*sizeof(WCHAR));
    mbstowcs_s(&converted,tokenW,strsize,tokenSource,_TRUNCATE);

    strsize = strlen(user)+1;
    userW = (LPWSTR)malloc(strsize*sizeof(WCHAR));
    mbstowcs_s(&converted,userW,strsize,user,_TRUNCATE);

    strsize = strlen(user)+1;
    pwdW = (LPWSTR)malloc(strsize*sizeof(WCHAR));
    mbstowcs_s(&converted,pwdW,strsize,user,_TRUNCATE);

    strsize = strlen(user)+1;
    domainW = (LPWSTR)malloc(strsize*sizeof(WCHAR));
    mbstowcs_s(&converted,domainW,strsize,user,_TRUNCATE);

    auth(sock, &cred, &cliCtx, tokenW, userW, pwdW, domainW); // this does the real work

    free(tokenW);
    free(userW);
    free(pwdW);
    free(domainW);


    /* Use the authenticated connection here */
    haveToken = FALSE;
    rc = recv(sock, (char*)&haveToken, sizeof(haveToken), 0);
    if (rc != sizeof(haveToken))
        wserr(999, L"result-recv");

    if (haveToken)
        wprintf(L"That seems to have worked.");
    else
        wprintf(L"Oops!  Wrong user name or password?");

    /*
     * The server is probably impersonating us by now.
     * This is where the client and server talk business.
     */

    /* Clean up */
    client1_SecFuncTable->DeleteSecurityContext(&cliCtx);
    client1_SecFuncTable->FreeCredentialHandle(&cred);

    rc = closesocket(sock);
    wserr(rc, L"closesocket");

    rc = WSACleanup();
    wserr(rc, L"WSACleanup");

    FreeLibrary(hSecLib);

    return 0;
}

#if 0
int main(int argc, _TCHAR* argv[])
{
    int i, errors;

    char *tokenSource = "Authsamp";
    char *server = "127.0.0.1";
    char *portstr = "55", *user = "", *pwd = "", *domain = "";

    errors = 0;
    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] != '-' && argv[i][0] != '/')
        {
            wprintf(L"\"%s\" is not a valid switch.\n", argv[i]);
            ++errors;
            continue;
        }

        switch (argv[i][1])
        {
        case L's':
            //if (i >= argc - 1)
            //{
            //	wprintf(L"\"%s\" requires an argument.\n", argv[i]);
            //	++errors;
            //}
            //else if (server != 0)
            //{
            //	wprintf(L"\"%s\" has already been used.\n", argv[i++]);
            //	++errors;
            //}
            //else
            //	server = argv[++i];
            break;
        case 'p':
            //if (i >= argc - 1)
            //{
            //	wprintf(L"\"%s\" requires an argument.\n", argv[i]);
            //	++errors;
            //}
            //else if (portstr != 0)
            //{
            //	wprintf(L"\"%s\" has already been used.\n", argv[i++]);
            //	++errors;
            //}
            //else
            //	portstr = argv[++i];
            break;
        case 't':
            if (i >= argc - 1)
            {
                wprintf(L"\"%s\" requires an argument.\n", argv[i]);
                ++errors;
            }
            else if (tokenSource != NULL)
            {
                wprintf(L"\"%s\" has already been used.\n", argv[i++]);
                ++errors;
            }
            else
            {
                tokenSource = argv[++i];
            }
            break;
        case 'd':
            if (i >= argc - 1)
            {
                wprintf(L"\"%s\" requires an argument.\n", argv[i]);
                ++errors;
            }
            else if (domain != NULL)
            {
                wprintf(L"\"%s\" has already been used.\n", argv[i++]);
                ++errors;
            }
            else
            {
                domain = argv[++i];
            }
            break;
        case 'u':
            if (i >= argc - 2)
            {
                wprintf(L"\"%s\" requires two arguments.\n", argv[i++]);
                ++errors;
            }
            else if (user != NULL)
            {
                wprintf(L"\"%s\" has already been used.\n", argv[i]);
                i += 2;
                ++errors;
            }
            else
            {
                user = argv[++i];
                pwd = argv[++i];
            }
            break;
        default:
            wprintf(L"\"%s\" is not a valid switch.\n", argv[i]);
            ++errors;
            break;
        }
    }

    if (server == NULL)
    {
        wprintf(L"A server name or IP address must be specified.\n");
        ++errors;
    }

    if (portstr == NULL)
    {
        wprintf(L"A port name or port number must be specified.\n");
        ++errors;
    }

    if (user == NULL && domain != NULL)
        wprintf(L"No user name was specified, ignoring the domain.\n");

    if (errors)
    {
        wprintf(L"\nusage: client -s your.server.com -p serverport");
        wprintf(L" [-t token-source] [-u user pwd [-d domain]]\n");
        wprintf(L"Token-source is _required_ for Kerberos and should be your");
        wprintf(L" current logon name (e.g., \"MYDOMAIN\\mypc\").");
        wprintf(L"If -u is absent, your current credentials will be used.\n");
        return 1;
    }

    return client1_main(server, portstr, tokenSource, user, pwd, domain);
}
#endif
