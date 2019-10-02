/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Client-side program to establish an SSPI socket connection.
 *                  with a server and exchange messages.
 * PROGRAMMERS:     Hermes Belusca-Maito
 *                  Andreas Maier <staubim@quantentunnel.de> (2019)
 */

#include "client_server.h"

#define cbMaxMessage 12000

BOOL
cli_DoAuthentication(
    IN PCLI_PARAMS pcp,
    IN SOCKET s,
    IN PCredHandle hCred,
    IN PSecHandle  hcText);

BOOL
cli_GenContext(
    PBYTE pIn,
    DWORD cbIn,
    PBYTE pOut,
    DWORD *pcbOut,
    BOOL  *pfDone,
    PCLI_PARAMS pcp,
    PCredHandle hCred,
    PSecHandle  hcText);

//--------------------------------------------------------------------
//  ConnectAuthSocket establishes an authenticated socket connection
//  with a server and initializes needed security package resources.

BOOL
cli_ConnectAuthSocket(
    IN  PCLI_PARAMS pcp,
    OUT SOCKET     *s,
    OUT PCredHandle hCred,
    OUT PSecHandle  hcText)
{
    unsigned long  ulAddress;
    struct hostent *pHost;
    SOCKADDR_IN    sin;
    char AnsiServerName[256];

    WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
        pcp->ServerName, -1, AnsiServerName, _countof(AnsiServerName), NULL, NULL);

    /* Lookup the server's address */
    ulAddress = inet_addr(AnsiServerName);

    if (ulAddress == INADDR_NONE)
    {
        pHost = gethostbyname(AnsiServerName);
        if (!pHost)
        {
            sync_err("Unable to resolve host name.\n");
            goto failed;
        }

        memcpy((char *)&ulAddress, pHost->h_addr, pHost->h_length);
    }

#ifndef UNICODE
#undef AnsiServerName
#endif

    /* Create the socket */
    *s = socket(PF_INET, SOCK_STREAM, 0);
    if (*s == INVALID_SOCKET)
    {
        sync_err("Unable to create socket\n");
        goto failed;
    }

    sync_trace("client socket %x created, port %i.\n", *s, pcp->ServerPort);

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ulAddress;
    sin.sin_port = htons(pcp->ServerPort);

    /* Connect to the server */
    if (connect(*s, (LPSOCKADDR)&sin, sizeof(sin)) != 0)
    {
        sync_err("Connect failed.\n");
        printerr(errno);
        goto failed;
    }

    if (!cli_DoAuthentication(pcp, *s, hCred, hcText))
    {
        sync_err("Authentication failed\n");
        goto failed;
    }

    return TRUE;

failed:
    if (*s != INVALID_SOCKET)
        closesocket(*s);
    return FALSE;
}

BOOL
cli_DoAuthentication(
    PCLI_PARAMS pcp,
    IN SOCKET s,
    IN PCredHandle hCred,
    IN PSecHandle  hcText)
{
    BOOL Success;
    BOOL    fDone = FALSE;
    DWORD   cbOut = 0;
    DWORD   cbIn = 0;
    PBYTE   pInBuf;
    PBYTE   pOutBuf;

    if (!(pInBuf = (PBYTE)malloc(cbMaxMessage)))
    {
        sync_err("Memory allocation ");
        return FALSE;
    }

    if (!(pOutBuf = (PBYTE)malloc(cbMaxMessage)))
    {
        sync_err("Memory allocation ");
        return FALSE;
    }

    cbOut = cbMaxMessage;
    Success = cli_GenContext(NULL,
                             0,
                             pOutBuf,
                             &cbOut,
                             &fDone,
                             pcp,
                             hCred,
                             hcText);
    sync_ok(Success, "cli_GenContext failed\n");
    if (!Success)
    {
        sync_err("cli_GenContext failed!\n");
        return FALSE;
    }
    NtlmCheckSecBuffer(TESTSEC_CLI_AUTH_INIT, pOutBuf, pcp, NULL);

    if (!SendMsg(s, pOutBuf, cbOut))
    {
        sync_err("Send message failed.\n");
        return FALSE;
    }

    while (!fDone)
    {
        if (!ReceiveMsg(s, pInBuf, cbMaxMessage, &cbIn))
        {
            sync_err("Receive message failed ");
            return FALSE;
        }

        cbOut = cbMaxMessage;
        Success = cli_GenContext(pInBuf,
                                 cbIn,
                                 pOutBuf,
                                 &cbOut,
                                 &fDone,
                                 pcp,
                                 hCred,
                                 hcText);
        sync_ok(Success, "cli_GenContext failed.\n");
        if (!Success)
        {
            sync_err("cli_GenContext failed.\n");
            return FALSE;
        }
        NtlmCheckSecBuffer(TESTSEC_CLI_AUTH_FINI, pOutBuf, pcp, NULL);

        if (!SendMsg(s, pOutBuf, cbOut))
        {
            sync_err("Send message 2 failed.\n");
            return FALSE;
        }
    }

    sync_trace("DoAuthentication end\n");
    free(pInBuf);
    free(pOutBuf);
    return TRUE;
}

BOOL
cli_GenContext(
    PBYTE pIn,
    DWORD cbIn,
    PBYTE pOut,
    DWORD *pcbOut,
    BOOL  *pfDone,
    PCLI_PARAMS pcp,
    PCredHandle hCred,
    PSecHandle  hcText)
{
    SECURITY_STATUS ss;
    TimeStamp       Lifetime;
    SecBufferDesc   OutBuffDesc;
    SecBuffer       OutSecBuff;
    SecBufferDesc   InBuffDesc;
    SecBuffer       InSecBuff;
    ULONG           ContextAttributes;
    PAUTH_TEST_DATA_CLI ptest = pcp->ptest;

    if (pIn)
    {
        sync_trace("===INPUT===\n");
        PrintHexDump(cbIn, (PBYTE)pIn);
    } else
    {
        sync_trace("===INPUT (EMPTY)===\n");
    }

    if (!pIn)
    {
        PSEC_WINNT_AUTH_IDENTITY pAuthData = NULL;
        SEC_WINNT_AUTH_IDENTITY AuthData;
        if (((ptest->pass != NULL) && (ptest->pass[0] != 0)) ||
            ((ptest->user != NULL) && (ptest->user[0] != 0)))
        {
            /* works in ROS but not in W2k ... dont commit! */
            AuthData.User = ptest->user;
            AuthData.UserLength = wcslen(ptest->user);
            AuthData.Password = ptest->pass;
            AuthData.PasswordLength = wcslen(ptest->pass);
            AuthData.Domain = ptest->userdom;
            AuthData.DomainLength = wcslen(ptest->userdom);
            AuthData.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
            pAuthData = &AuthData;
        }
        ss = AcquireCredentialsHandle(NULL,
                                      pcp->PackageName,
                                      SECPKG_CRED_OUTBOUND,
                                      NULL,
                                      (PVOID)pAuthData,
                                      NULL,
                                      NULL,
                                      hCred,
                                      &Lifetime);
        sync_ok(SEC_SUCCESS(ss), "AcquireCredentialsHandle failed with error 0x%08lx\n", ss);
        if (!SEC_SUCCESS(ss))
        {
            sync_err("AcquireCreds failed ");
            return FALSE;
        }
    }

    //--------------------------------------------------------------------
    //  Prepare the buffers.

    OutBuffDesc.ulVersion = 0;
    OutBuffDesc.cBuffers  = 1;
    OutBuffDesc.pBuffers  = &OutSecBuff;

    OutSecBuff.cbBuffer   = *pcbOut;
    OutSecBuff.BufferType = SECBUFFER_TOKEN;
    OutSecBuff.pvBuffer   = pOut;

    /*
     * The input buffer is created only if a message has been received
     * from the server.
     */
    if (pIn)
    {
        InBuffDesc.ulVersion = 0;
        InBuffDesc.cBuffers  = 1;
        InBuffDesc.pBuffers  = &InSecBuff;

        InSecBuff.cbBuffer   = cbIn;
        InSecBuff.BufferType = SECBUFFER_TOKEN;
        InSecBuff.pvBuffer   = pIn;

        PrintISCReqAttr(ptest->MessageAttribute);
        ss = InitializeSecurityContext(hCred,
                                       hcText,
                                       pcp->ServerName,
                                       ptest->MessageAttribute,
                                       0,
                                       SECURITY_NATIVE_DREP,
                                       &InBuffDesc,
                                       0,
                                       hcText,
                                       &OutBuffDesc,
                                       &ContextAttributes,
                                       &Lifetime);
        sync_ok(ContextAttributes == pcp->ptest->ISCContextRETAttr2,
                "ContextAttributes are 0x%x, expected 0x%x\n",
                ContextAttributes, pcp->ptest->ISCContextRETAttr2);
        PrintISCRetAttr(ContextAttributes);
    }
    else
    {
        PrintISCReqAttr(ptest->MessageAttribute);
        ss = InitializeSecurityContext(hCred,
                                       NULL,
                                       pcp->ServerName,
                                       ptest->MessageAttribute,
                                       0,
                                       SECURITY_NATIVE_DREP,
                                       NULL,
                                       0,
                                       hcText,
                                       &OutBuffDesc,
                                       &ContextAttributes,
                                       &Lifetime);
        sync_ok(ContextAttributes == pcp->ptest->ISCContextRETAttr1,
                "ContextAttributes are 0x%x, expected 0x%x\n",
                ContextAttributes, pcp->ptest->ISCContextRETAttr1);
        PrintISCRetAttr(ContextAttributes);
    }

    sync_ok(SEC_SUCCESS(ss), "InitializeSecurityContext failed with error 0x%08lx\n", ss);
    if (!SEC_SUCCESS(ss))
    {
        sync_err("InitializeSecurityContext failed ");
        return 0;
    }

    /* If necessary, complete the token */
    if ((ss == SEC_I_COMPLETE_NEEDED) ||
        (ss == SEC_I_COMPLETE_AND_CONTINUE))
    {
        ss = CompleteAuthToken(hcText, &OutBuffDesc);
        sync_ok(SEC_SUCCESS(ss), "CompleteAuthToken failed with error 0x%08lx\n", ss);
        if (!SEC_SUCCESS(ss))
        {
            sync_err("complete failed: 0x%08lx\n", ss);
            return FALSE;
        }
    }

    *pcbOut = OutSecBuff.cbBuffer;

    *pfDone = !((ss == SEC_I_CONTINUE_NEEDED) ||
                (ss == SEC_I_COMPLETE_AND_CONTINUE));

    sync_trace("Token buffer generated (%lu bytes):\n", OutSecBuff.cbBuffer);
    sync_trace("===OUTPUT===\n");
    PrintSecBuffer(&OutSecBuff);

    return TRUE;

}

PBYTE
VerifyThis(
    PBYTE   pBuffer,
    LPDWORD pcbMessage,
    PSecHandle hCtxt,
    ULONG   cbMaxSignature)
{
    SECURITY_STATUS   ss;
    SecBufferDesc     BuffDesc;
    SecBuffer         SecBuff[2];
    ULONG             ulQop = 0;
    PBYTE             pSigBuffer;
    PBYTE             pDataBuffer;

    /*
     * The global cbMaxSignature is the size of the signature
     * in the message received.
     */
    sync_trace("data before verifying (including signature):\n");
    PrintHexDump(*pcbMessage, pBuffer);

    /*
     * By agreement with the server,
     * the signature is at the beginning of the message received,
     * and the data that was signed comes after the signature.
     */
    pSigBuffer = pBuffer;
    pDataBuffer = pBuffer + cbMaxSignature;

    /* The size of the message is reset to the size of the data only */
    *pcbMessage = *pcbMessage - cbMaxSignature;

    /*
     * Prepare the buffers to be passed to the signature verification function
     */

    BuffDesc.ulVersion    = 0;
    BuffDesc.cBuffers     = ARRAYSIZE(SecBuff);
    BuffDesc.pBuffers     = SecBuff;

    SecBuff[0].cbBuffer   = cbMaxSignature;
    SecBuff[0].BufferType = SECBUFFER_TOKEN;
    SecBuff[0].pvBuffer   = pSigBuffer;

    SecBuff[1].cbBuffer   = *pcbMessage;
    SecBuff[1].BufferType = SECBUFFER_DATA;
    SecBuff[1].pvBuffer   = pDataBuffer;

    ss = VerifySignature(hCtxt, &BuffDesc, 0, &ulQop);
    sync_ok(SEC_SUCCESS(ss), "VerifySignature failed with error 0x%08lx\n", ss);
    if (!SEC_SUCCESS(ss))
        sync_err("VerifyMessage failed");
    else
        sync_trace("Message was properly signed.\n");

    return pDataBuffer;
}

BOOL WINAPI
cli_start(
    IN PCLI_PARAMS pcp)
{
    SOCKET          Client_Socket = INVALID_SOCKET;
    CredHandle      hCred;
    SecHandle       hCtxt;
    SECURITY_STATUS ss;
    SecPkgContext_Sizes SecPkgSizes;
    SecPkgContext_NegotiationInfo SecPkgNegInfo;
    DWORD bRet = FALSE;

    /* Connect to a server */
    if (!cli_ConnectAuthSocket(pcp, &Client_Socket, &hCred, &hCtxt))
    {
        /* do not free garbage (in done) */
        hCtxt.dwLower = 0;
        hCred.dwLower = 0;
        sync_err("Unable to authenticate server connection.\n");
        goto done;
    }

    /*
     * An authenticated session with a server has been established.
     * Receive and manage a message from the server.
     * First, find and display the name of the negotiated
     * SSP and the size of the signature and the encryption
     * trailer blocks for this SSP.
     */
    ss = QueryContextAttributes(&hCtxt,
                                SECPKG_ATTR_NEGOTIATION_INFO,
                                &SecPkgNegInfo);
    sync_ok(SEC_SUCCESS(ss), "QueryContextAttributes failed with error 0x%08lx\n", ss);
    if (!SEC_SUCCESS(ss))
    {
        sync_err("QueryContextAttributes failed.\n");
        goto done;
    }
    else
    {
        sync_trace("Negotiation State: 0x%x\n", SecPkgNegInfo.NegotiationState);
        sync_trace("fCapabilities: 0x%x\n", SecPkgNegInfo.PackageInfo->fCapabilities);
        sync_trace("wVersion/wRPCID: %d/%d\n",
                    SecPkgNegInfo.PackageInfo->wVersion,
                    SecPkgNegInfo.PackageInfo->wRPCID);
        sync_trace("cbMaxToken: %d\n", SecPkgNegInfo.PackageInfo->cbMaxToken);
        sync_trace("Package Name: %S\n", SecPkgNegInfo.PackageInfo->Name);
        sync_trace("Package Comment: %S\n", SecPkgNegInfo.PackageInfo->Comment);
        FreeContextBuffer(SecPkgNegInfo.PackageInfo);
    }

    ss = QueryContextAttributes(&hCtxt,
                                SECPKG_ATTR_SIZES,
                                &SecPkgSizes);
    sync_ok(SEC_SUCCESS(ss), "Querycontext2 failed with error 0x%x!", ss);

    bRet = msgtest_recv(Client_Socket, &hCtxt, &SecPkgSizes, pcp->ownServer,
                        L"This is your server speaking.") &&
           msgtest_send(Client_Socket, &hCtxt, &SecPkgSizes,
                        L"Greetings from client.") &&
           msgtest_recv(Client_Socket, &hCtxt, &SecPkgSizes, pcp->ownServer,
                        L"2nd message from server.") &&
           msgtest_send(Client_Socket, &hCtxt, &SecPkgSizes,
                        L"Client got a 2nd message.");

done:
    /* Terminate socket and security package */
    if (hCtxt.dwLower != 0)
        DeleteSecurityContext(&hCtxt);
    if (hCred.dwLower != 0)
        FreeCredentialHandle(&hCred);
    if (Client_Socket != INVALID_SOCKET)
    {
        shutdown(Client_Socket, 2);
        closesocket(Client_Socket);
    }

    return bRet;
}

int cli_auth_main(int argc, WCHAR** argv)
{
    DWORD dwRet = 1;//FAILED
    DWORD testDataIdx;
    WSADATA wsaData;
    CLI_PARAMS cp;
    
    sync_ok(argc == 3, "argumentcount mismatched - aborting\n");
    if (argc != 3)
        goto done;

    cp.ServerName = argv[0];
    cp.ServerPort = _wtoi(argv[1]);
    testDataIdx = _wtoi(argv[2]);
    if (testDataIdx > AUTH_TEST_DATA_SIZE)
    {
        sync_err("internal error - testindex out of bounds!\n");
        goto done;
    }
    cp.PackageName = authtestdata[testDataIdx].PackageName;
    cp.ptest = &authtestdata[testDataIdx].cli;
    cp.ownServer = (cp.ServerPort != 445);

    /* Startup WSA */
    if (WSAStartup(0x0101, &wsaData))
    {
        sync_err("Could not initialize winsock.\n");
        goto done;
    }

    /* Start the client */
    dwRet = cli_start(&cp);

done:
    /* Shutdown WSA and return */
    WSACleanup();

    //client_exit();
    return dwRet;
}
