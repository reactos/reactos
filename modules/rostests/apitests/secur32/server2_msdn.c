//
// MSDN Example "Using SSPI with a Windows Sockets Server"
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa380537(v=vs.85).aspx
//

//--------------------------------------------------------------------
//  This is a server-side SSPI Windows Sockets program.

#include "client_server.h"

typedef struct _SERVER_DATA
{
    CredHandle hcred;
    SecHandle hctxt;
    PBYTE pInBuf;
    PBYTE pOutBuf;
    DWORD cbMaxMessage;
} SERVER_DATA;

SERVER_DATA g_sd;

BOOL
GenServerContext(
    CredHandle hcred,
    PBYTE pIn,
    DWORD cbIn,
    PBYTE pOut,
    DWORD *pcbOut,
    BOOL  *pfDone,
    BOOL  fNewConversation);

BOOL server2_DoAuthentication(
    IN int ServerPort,
    IN SOCKET AuthSocket,
    IN LPCTSTR PackageName);

BOOL AcceptAuthSocket(
    IN int ServerPort,
    OUT SOCKET *ServerSocket,
    IN  LPCTSTR PackageName)
{
    SOCKET sockListen;
    SOCKET sockClient;
    SOCKADDR_IN sockIn;

    /* Create listening socket */
    sockListen = socket(PF_INET, SOCK_STREAM, 0);
    if (sockListen == INVALID_SOCKET)
    {
        sync_err("Failed to create socket: %lu\n", (int)GetLastError());
        return FALSE;
    }
    else
    {
        sync_trace("server socket created.\n");//,sockListen);
    }

    /* Bind to local port */
    sockIn.sin_family = AF_INET;
    sockIn.sin_addr.s_addr = 0;
    sockIn.sin_port = htons(ServerPort);

    if (SOCKET_ERROR == bind(sockListen,
                             (LPSOCKADDR)&sockIn,
                             sizeof(sockIn)))
    {
        sync_err("bind failed: %lu\n", GetLastError());
        return FALSE;
    }

    //-----------------------------------------------------------------
    //  Listen for client, maximum 1 connection.

    if (SOCKET_ERROR == listen(sockListen, 1))
    {
        sync_err("Listen failed: %lu\n", GetLastError());
        return FALSE;
    }
    else
    {
        sync_trace("Listening !\n");
    }

    /* Accept client */
    sockClient = accept(sockListen,
                        NULL,
                        NULL);

    if (sockClient == INVALID_SOCKET)
    {
        sync_err("accept failed: %lu\n", GetLastError());
        return FALSE;
    }

    /* Stop listening */
    closesocket(sockListen);

    sync_trace("connection accepted on socket %x!\n",sockClient);
    *ServerSocket = sockClient;

    return server2_DoAuthentication(ServerPort, sockClient, PackageName);
}

BOOL server2_DoAuthentication(
    IN int ServerPort,
    IN SOCKET AuthSocket,
    IN LPCTSTR PackageName)
{
    SECURITY_STATUS   ss;
    DWORD cbIn,       cbOut;
    BOOL              done = FALSE;
    TimeStamp         Lifetime;
    BOOL              fNewConversation = TRUE;

    ss = AcquireCredentialsHandle(NULL,
                                  (LPTSTR)PackageName,
                                  SECPKG_CRED_INBOUND,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &g_sd.hcred,
                                  &Lifetime);
    sync_ok(SEC_SUCCESS(ss), "AcquireCredentialsHandle failed with error 0x%08lx\n", ss);
    if (!SEC_SUCCESS(ss))
    {
        sync_err("AcquireCreds failed: 0x%08lx\n", ss);
        printerr(ss);
        return FALSE;
    }

    while (!done)
    {
        if (!ReceiveMsg(AuthSocket,
                        g_sd.pInBuf,
                        g_sd.cbMaxMessage,
                        &cbIn))
        {
            return FALSE;
        }

        cbOut = g_sd.cbMaxMessage;

        if (!GenServerContext(g_sd.hcred,
                              g_sd.pInBuf,
                              cbIn,
                              g_sd.pOutBuf,
                              &cbOut,
                              &done,
                              fNewConversation))
        {
            sync_err("GenServerContext failed.\n");
            return FALSE;
        }
        NtlmCheckSecBuffer(TESTSEC_SVR_AUTH, g_sd.pOutBuf);

        fNewConversation = FALSE;
        if (!SendMsg(AuthSocket, g_sd.pOutBuf, cbOut))
        {
            sync_err("Sending message failed.\n");
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
GenServerContext(
    CredHandle hcred,
    PBYTE pIn,
    DWORD cbIn,
    PBYTE pOut,
    DWORD *pcbOut,
    BOOL  *pfDone,
    BOOL  fNewConversation)
{
    SECURITY_STATUS   ss;
    TimeStamp         Lifetime;
    SecBufferDesc     OutBuffDesc;
    SecBuffer         OutSecBuff;
    SecBufferDesc     InBuffDesc;
    SecBuffer         InSecBuff[2];
    ULONG             Attribs = 0;

    //----------------------------------------------------------------
    //  Prepare output buffers.

    OutBuffDesc.ulVersion = 0;
    OutBuffDesc.cBuffers = 1;
    OutBuffDesc.pBuffers = &OutSecBuff;

    OutSecBuff.cbBuffer = *pcbOut;
    OutSecBuff.BufferType = SECBUFFER_TOKEN;
    OutSecBuff.pvBuffer = pOut;

    //----------------------------------------------------------------
    //  Prepare input buffers.

    InBuffDesc.ulVersion = 0;
    InBuffDesc.cBuffers = 2;
    InBuffDesc.pBuffers = InSecBuff;

    InSecBuff[0].cbBuffer = cbIn;
    InSecBuff[0].BufferType = SECBUFFER_TOKEN;
    InSecBuff[0].pvBuffer = pIn;

    /* second buffer should be of SECBUFFER_EMPTY type! */
    InSecBuff[1].cbBuffer = 0;
    InSecBuff[1].BufferType = SECBUFFER_EMPTY;
    InSecBuff[1].pvBuffer = NULL;

    sync_trace("Token buffer received (%lu bytes):\n", InSecBuff[0].cbBuffer);
    PrintSecBuffer(&InSecBuff[0]);
    sync_trace(">>> %p %p\n", OutSecBuff.pvBuffer, pOut);
    PrintASCReqAttr(Attribs);
    ss = AcceptSecurityContext(&hcred,
                               fNewConversation ? NULL : &g_sd.hctxt,
                               &InBuffDesc,
                               Attribs,
                               SECURITY_NATIVE_DREP,
                               &g_sd.hctxt,
                               &OutBuffDesc,
                               &Attribs,
                               &Lifetime);
    PrintASCRetAttr(Attribs);
    if (fNewConversation)
    {
        sync_ok(Attribs == 0x1c, "Attribs 0x%x wrong, expected 0x%x.\n", Attribs, 0x1c);
        sync_ok(OutSecBuff.cbBuffer != 0, "OutSecBuff.cbBuffer == 0\n");
    }
    else
    {
        sync_ok(Attribs == 0x2001c, "Attribs 0x%x wrong, expected 0x%x.\n", Attribs, 0x2001c);
        sync_ok(OutSecBuff.cbBuffer == 0, "OutSecBuff.cbBuffer != 0\n");
    }
    sync_ok(SEC_SUCCESS(ss), "AcceptSecurityContext failed with error 0x%08lx\n", ss);
    if (!SEC_SUCCESS(ss))
        return FALSE;

    /* Complete token if applicable */
    if ((ss == SEC_I_COMPLETE_NEEDED) ||
        (ss == SEC_I_COMPLETE_AND_CONTINUE))
    {
        ss = CompleteAuthToken(&g_sd.hctxt, &OutBuffDesc);
        sync_ok(SEC_SUCCESS(ss), "CompleteAuthToken failed with error 0x%08lx\n", ss);
        if (!SEC_SUCCESS(ss))
        {
            sync_err("complete failed: 0x%08lx\n", ss);
            printerr(ss);
            return FALSE;
        }
    }

    *pcbOut = OutSecBuff.cbBuffer;

    //  fNewConversation equals FALSE.

    sync_trace("Token buffer generated (%lu bytes):\n",
        OutSecBuff.cbBuffer);

    PrintSecBuffer(&OutSecBuff);
    sync_trace(">>> %p %p\n", OutSecBuff.pvBuffer, pOut);

    *pfDone = !((ss == SEC_I_CONTINUE_NEEDED) ||
                (ss == SEC_I_COMPLETE_AND_CONTINUE));

    return TRUE;
}

BOOL
EncryptThis(
    PBYTE pMessage,
    ULONG cbMessage,
    BYTE ** ppOutput,
    ULONG * pcbOutput,
    ULONG cbSecurityTrailer)
{
    SECURITY_STATUS   ss;
    SecBufferDesc     BuffDesc;
    SecBuffer         SecBuff[2];
    ULONG             ulQop = 0;
    ULONG             SigBufferSize;

    /*
     * The size of the trailer (signature + padding) block is
     * determined from the global cbSecurityTrailer.
     */
    SigBufferSize = cbSecurityTrailer;

    sync_trace("Data before encryption: %.*s\n", (int)cbMessage/sizeof(TCHAR), pMessage);
    sync_trace("Length of data before encryption: %ld\n", cbMessage);

    /*
     * Allocate a buffer to hold the signature,
     * encrypted data, and a DWORD
     * that specifies the size of the trailer block.
     */
    *ppOutput = (PBYTE)malloc(SigBufferSize + cbMessage + sizeof(DWORD));

    /* Prepare buffers */
    BuffDesc.ulVersion = 0;
    BuffDesc.cBuffers = ARRAYSIZE(SecBuff);
    BuffDesc.pBuffers = SecBuff;

    SecBuff[0].cbBuffer = SigBufferSize;
    SecBuff[0].BufferType = SECBUFFER_TOKEN;
    SecBuff[0].pvBuffer = *ppOutput + sizeof(DWORD);

    SecBuff[1].cbBuffer = cbMessage;
    SecBuff[1].BufferType = SECBUFFER_DATA;
    SecBuff[1].pvBuffer = pMessage;

    ss = EncryptMessage(&g_sd.hctxt, ulQop, &BuffDesc, 0);
    sync_ok(SEC_SUCCESS(ss), "EncryptMessage failed with error 0x%08lx\n", ss);
    if (!SEC_SUCCESS(ss))
    {
        sync_err("EncryptMessage failed: 0x%08lx\n", ss);
        return FALSE;
    }
    else
    {
        sync_trace("The message has been encrypted.\n");
    }

    /* Indicate the size of the buffer in the first DWORD */
    *((DWORD*)*ppOutput) = SecBuff[0].cbBuffer;

    /*
     * Append the encrypted data to our trailer block
     * to form a single block.
     * Putting trailer at the beginning of the buffer works out
     * better.
     */
    memcpy(*ppOutput+SecBuff[0].cbBuffer + sizeof(DWORD), pMessage, cbMessage);

    *pcbOutput = cbMessage + SecBuff[0].cbBuffer + sizeof(DWORD);

    sync_trace("data after encryption including trailer (%lu bytes):\n", *pcbOutput);
    //PrintHexDump(*pcbOutput, *ppOutput);

    return TRUE;
}

void server2_cleanup(void)
{
    sync_trace("called server2_cleanup!\n");

    if (g_sd.pInBuf)
        free(g_sd.pInBuf);

    if (g_sd.pOutBuf)
        free(g_sd.pOutBuf);

    WSACleanup();
}

BOOL WINAPI
server2_start(
    IN int ServerPort,
    IN LPCTSTR PackageName)
{
    BOOL Success, res = FALSE;
    PBYTE pDataToClient = NULL;
    DWORD cbDataToClient = 0;
    TCHAR* pUserName = NULL;
    DWORD cchUserName = 0;
    SOCKET Server_Socket = INVALID_SOCKET;
    SECURITY_STATUS ss;
    PSecPkgInfo pkgInfo;
    SecPkgContext_Sizes SecPkgContextSizes;
    SecPkgContext_NegotiationInfo SecPkgNegInfo;
    ULONG cbSecurityTrailer;

    // server2_init
    RtlZeroMemory(&g_sd, sizeof(g_sd));

    /*
     * Initialize the security package
     */
    ss = QuerySecurityPackageInfo((LPTSTR)PackageName, &pkgInfo);
    sync_ok(SEC_SUCCESS(ss), "QuerySecurityPackageInfo failed with error 0x%08lx\n", ss);
    if (!SEC_SUCCESS(ss))
    {
        skip("Could not query package info for %S, error 0x%08lx\n",
             PackageName, ss);
        printerr(ss);
        goto done;
    }

    g_sd.cbMaxMessage = pkgInfo->cbMaxToken;
    sync_trace("max token = %ld\n", g_sd.cbMaxMessage);

    FreeContextBuffer(pkgInfo);

    g_sd.pInBuf = (PBYTE)malloc(g_sd.cbMaxMessage);
    g_sd.pOutBuf = (PBYTE)malloc(g_sd.cbMaxMessage);

    if (!g_sd.pInBuf || !g_sd.pOutBuf)
    {
        sync_err("Memory allocation error.\n");
        goto done;
    }

    /* Start looping for clients */

    //while (TRUE)
    {
        sync_trace("Waiting for client to connect...\n");

        /* Make an authenticated connection with client */
        Success = AcceptAuthSocket(ServerPort, &Server_Socket, PackageName);
        sync_ok(Success, "AcceptAuthSocket failed\n");
        if (!Success)
        {
            sync_err("Could not authenticate the socket.\n");
            goto done;
        }

        ss = QueryContextAttributes(&g_sd.hctxt,
                                    SECPKG_ATTR_SIZES,
                                    &SecPkgContextSizes);
        sync_ok(SEC_SUCCESS(ss), "QueryContextAttributes failed with error 0x%08lx\n", ss);
        if (!SEC_SUCCESS(ss))
        {
            sync_err("QueryContextAttributes failed: 0x%08lx\n", ss);
            goto done;
        }

        /* The following values are used for encryption and signing */
        cbSecurityTrailer = SecPkgContextSizes.cbSecurityTrailer;

        ss = QueryContextAttributes(&g_sd.hctxt,
                                    SECPKG_ATTR_NEGOTIATION_INFO,
                                    &SecPkgNegInfo);
        sync_ok(SEC_SUCCESS(ss), "QueryContextAttributes failed with error 0x%08lx\n", ss);
        if (!SEC_SUCCESS(ss))
        {
            sync_err("QueryContextAttributes failed: 0x%08lx\n", ss);
            printerr(ss);
            sync_err("aborting\n");
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
        }

        /* Free the allocated buffer */
        FreeContextBuffer(SecPkgNegInfo.PackageInfo);

        /* Impersonate the client */
        ss = ImpersonateSecurityContext(&g_sd.hctxt);
        sync_ok(SEC_SUCCESS(ss), "ImpersonateSecurityContext failed with error 0x%08lx - skipping.\n", ss);
        if (SEC_SUCCESS(ss))
        {
            sync_trace("Impersonation worked.\n");

            GetUserName(NULL, &cchUserName);
            pUserName = (TCHAR*)malloc(cchUserName*sizeof(TCHAR));
            if (!pUserName)
            {
                sync_err("Memory allocation error.\n");
                sync_err("aborting\n");
                goto done;
            }

            if (!GetUserName(pUserName, &cchUserName))
            {
                sync_err("Could not get the client name.\n");
                printerr(GetLastError());
                sync_err("aborting\n");
                goto done;
            }
            else
            {
                sync_trace("Client connected as :  %S\n", pUserName);
            }

            /* Revert to self */
            ss = RevertSecurityContext(&g_sd.hctxt);
            sync_ok(SEC_SUCCESS(ss), "RevertSecurityContext failed with error 0x%08lx\n", ss);
            if (!SEC_SUCCESS(ss))
            {
                sync_err("Revert failed: 0x%08lx\n", ss);
                printerr(GetLastError());
                sync_err("aborting\n");
                goto done;
            }
            else
            {
                sync_trace("Reverted to self.\n");
            }
        }

        /*
         * Send the client an encrypted message
         */
        {
        TCHAR pMessage[200] = _T("This is your server speaking");
        DWORD cbMessage = _tcslen(pMessage) * sizeof(TCHAR);

        EncryptThis((PBYTE)pMessage,
                    cbMessage,
                    &pDataToClient,
                    &cbDataToClient,
                    cbSecurityTrailer);
        }

        /* Send the encrypted data to client */
        if (!SendBytes(Server_Socket,
                       pDataToClient,
                       cbDataToClient))
        {
            sync_err("send message failed.\n");
            printerr(GetLastError());
            sync_err("aborting\n");
            goto done;
        }

        sync_trace(" %ld encrypted bytes sent.\n", cbDataToClient);

        if (pUserName)
        {
            free(pUserName);
            pUserName = NULL;
            cchUserName = 0;
        }
        if (pDataToClient)
        {
            free(pDataToClient);
            pDataToClient = NULL;
            cbDataToClient = 0;
        }
    }

    sync_trace("Server ran to completion without error.\n");
    res = TRUE;
done:
    if (g_sd.hcred.dwLower != 0)
        DeleteSecurityContext(&g_sd.hctxt);
    if (g_sd.hctxt.dwLower != 0)
        FreeCredentialHandle(&g_sd.hcred);
    if (Server_Socket != INVALID_SOCKET)
    {
        shutdown(Server_Socket, 2);
        closesocket(Server_Socket);
        Server_Socket = 0;
    }

    server2_cleanup();
    return res;
}

int server2_main(int argc, WCHAR** argv)
{
    DWORD dwRet;
    WSADATA wsaData;
    LPCTSTR PackageName;
    int ServerPort;

    if (argc != 2)
    {
        sync_err("arg != 1\n");
        return -1;
    }

    ServerPort = _wtoi(argv[0]);
    PackageName = argv[1];

    /* Startup WSA */
    if (WSAStartup(0x0101, &wsaData))
    {
        sync_err("Could not initialize winsock.\n");
        return 0;
    }

    /* Start the server */
    dwRet = server2_start(ServerPort, PackageName);

    /* Shutdown WSA and return */
    WSACleanup();
    return dwRet;
}
