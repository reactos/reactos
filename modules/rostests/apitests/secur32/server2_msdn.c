//
// MSDN Example "Using SSPI with a Windows Sockets Server"
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa380537(v=vs.85).aspx
//

//--------------------------------------------------------------------
//  This is a server-side SSPI Windows Sockets program.

#include "client_server.h"

#define g_usPort 2000

CredHandle hcred;
SecHandle hctxt;
PBYTE g_pInBuf;
PBYTE g_pOutBuf;
DWORD g_cbMaxMessage;

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
    IN SOCKET AuthSocket,
    IN LPCTSTR PackageName);

BOOL AcceptAuthSocket(
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
        err("Failed to create socket: %u\n", GetLastError());
        return FALSE;
    }
    else
    {
        trace("server socket created.\n",sockListen);
    }

    /* Bind to local port */
    sockIn.sin_family = AF_INET;
    sockIn.sin_addr.s_addr = 0;
    sockIn.sin_port = htons(g_usPort);

    if (SOCKET_ERROR == bind(sockListen,
                             (LPSOCKADDR)&sockIn,
                             sizeof(sockIn)))
    {
        err("bind failed: %u\n", GetLastError());
        return FALSE;
    }

    //-----------------------------------------------------------------
    //  Listen for client, maximum 1 connection.

    if (SOCKET_ERROR == listen(sockListen, 1))
    {
        err("Listen failed: %u\n", GetLastError());
        return FALSE;
    }
    else
    {
        trace("Listening !\n");
    }

    /* Accept client */
    sockClient = accept(sockListen,
                        NULL,
                        NULL);
    if (sockClient == INVALID_SOCKET)
    {
        err("accept failed: %u\n", GetLastError());
        return FALSE;
    }

    /* Stop listening */
    closesocket(sockListen);

    trace("connection accepted on socket %x!\n",sockClient);
    *ServerSocket = sockClient;

    return server2_DoAuthentication(sockClient, PackageName);
}

BOOL server2_DoAuthentication(
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
                                  &hcred,
                                  &Lifetime);
    ok(SEC_SUCCESS(ss), "AcquireCredentialsHandle failed with error 0x%08x\n", ss);
    if (!SEC_SUCCESS(ss))
    {
        err("AcquireCreds failed: 0x%08x\n", ss);
        printerr(ss);
        return FALSE;
    }

    while (!done)
    {
        if (!ReceiveMsg(AuthSocket,
                        g_pInBuf,
                        g_cbMaxMessage,
                        &cbIn))
        {
            return FALSE;
        }

        cbOut = g_cbMaxMessage;

        if (!GenServerContext(hcred,
                              g_pInBuf,
                              cbIn,
                              g_pOutBuf,
                              &cbOut,
                              &done,
                              fNewConversation))
        {
            err("GenServerContext failed.\n");
            return FALSE;
        }

        fNewConversation = FALSE;
        if (!SendMsg(AuthSocket, g_pOutBuf, cbOut))
        {
            err("Sending message failed.\n");
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
    SecBuffer         InSecBuff;
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
    InBuffDesc.cBuffers = 1;
    InBuffDesc.pBuffers = &InSecBuff;

    InSecBuff.cbBuffer = cbIn;
    InSecBuff.BufferType = SECBUFFER_TOKEN;
    InSecBuff.pvBuffer = pIn;

    trace("Token buffer received (%lu bytes):\n", InSecBuff.cbBuffer);
    PrintHexDump(InSecBuff.cbBuffer, (PBYTE)InSecBuff.pvBuffer);

    ss = AcceptSecurityContext(&hcred,
                               fNewConversation ? NULL : &hctxt,
                               &InBuffDesc,
                               Attribs,
                               SECURITY_NATIVE_DREP,
                               &hctxt,
                               &OutBuffDesc,
                               &Attribs,
                               &Lifetime);
    ok(SEC_SUCCESS(ss), "AcceptSecurityContext failed with error 0x%08x\n", ss);
    if (!SEC_SUCCESS(ss))
    {
        err("AcceptSecurityContext failed: 0x%08x\n", ss);
        printerr(ss);
        return FALSE;
    }

    /* Complete token if applicable */
    if ((ss == SEC_I_COMPLETE_NEEDED) ||
        (ss == SEC_I_COMPLETE_AND_CONTINUE))
    {
        ss = CompleteAuthToken(&hctxt, &OutBuffDesc);
        ok(SEC_SUCCESS(ss), "CompleteAuthToken failed with error 0x%08x\n", ss);
        if (!SEC_SUCCESS(ss))
        {
            err("complete failed: 0x%08x\n", ss);
            printerr(ss);
            return FALSE;
        }
    }

    *pcbOut = OutSecBuff.cbBuffer;

    //  fNewConversation equals FALSE.

    trace("Token buffer generated (%lu bytes):\n",
        OutSecBuff.cbBuffer);
    PrintHexDump(OutSecBuff.cbBuffer,
                 (PBYTE)OutSecBuff.pvBuffer);

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

    trace("Data before encryption: %.*S\n", cbMessage/sizeof(TCHAR), pMessage);
    trace("Length of data before encryption: %d\n", cbMessage);

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

    ss = EncryptMessage(&hctxt, ulQop, &BuffDesc, 0);
    ok(SEC_SUCCESS(ss), "EncryptMessage failed with error 0x%08x\n", ss);
    if (!SEC_SUCCESS(ss))
    {
        err("EncryptMessage failed: 0x%08x\n", ss);
        return FALSE;
    }
    else
    {
        trace("The message has been encrypted.\n");
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

    trace("data after encryption including trailer (%lu bytes):\n", *pcbOutput);
    PrintHexDump(*pcbOutput, *ppOutput);

    return TRUE;
}

void server2_cleanup(void)
{
    trace("called server2_cleanup!\n");

    if (g_pInBuf)
        free(g_pInBuf);

    if (g_pOutBuf)
        free(g_pOutBuf);

    WSACleanup();
}

DWORD WINAPI
server2_start(
    IN LPCTSTR PackageName)
{
    BOOL Success;
    PBYTE pDataToClient = NULL;
    DWORD cbDataToClient = 0;
    TCHAR* pUserName = NULL;
    DWORD cchUserName = 0;
    SOCKET Server_Socket;
    SECURITY_STATUS ss;
    PSecPkgInfo pkgInfo;
    SecPkgContext_Sizes SecPkgContextSizes;
    SecPkgContext_NegotiationInfo SecPkgNegInfo;
    ULONG cbMaxSignature;
    ULONG cbSecurityTrailer;

    /*
     * Initialize the security package
     */
    ss = QuerySecurityPackageInfo((LPTSTR)PackageName, &pkgInfo);
    ok(SEC_SUCCESS(ss), "QuerySecurityPackageInfo failed with error 0x%08x\n", ss);
    if (!SEC_SUCCESS(ss))
    {
        skip("Could not query package info for %S, error 0x%08x\n",
             PackageName, ss);
        printerr(ss);
        server2_cleanup();
        return 0;
    }

    g_cbMaxMessage = pkgInfo->cbMaxToken;
    trace("max token = %d\n", g_cbMaxMessage);

    FreeContextBuffer(pkgInfo);

    g_pInBuf = (PBYTE)malloc(g_cbMaxMessage);
    g_pOutBuf = (PBYTE)malloc(g_cbMaxMessage);

    if (!g_pInBuf || !g_pOutBuf)
    {
        server2_cleanup();
        fatal_error("Memory allocation error.\n");
    }

    /* Start looping for clients */

    //while (TRUE)
    {
        trace("Waiting for client to connect...\n");

        /* Make an authenticated connection with client */
        Success = AcceptAuthSocket(&Server_Socket, PackageName);
        ok(Success, "AcceptAuthSocket failed\n");
        if (!Success)
        {
            server2_cleanup();
            fatal_error("Could not authenticate the socket.\n");
        }

        ss = QueryContextAttributes(&hctxt,
                                    SECPKG_ATTR_SIZES,
                                    &SecPkgContextSizes);
        ok(SEC_SUCCESS(ss), "QueryContextAttributes failed with error 0x%08x\n", ss);
        if (!SEC_SUCCESS(ss))
        {
            server2_cleanup();
            fatal_error("QueryContextAttributes failed: 0x%08x\n", ss);
        }

        /* The following values are used for encryption and signing */
        cbMaxSignature = SecPkgContextSizes.cbMaxSignature;
        cbSecurityTrailer = SecPkgContextSizes.cbSecurityTrailer;

        ss = QueryContextAttributes(&hctxt,
                                    SECPKG_ATTR_NEGOTIATION_INFO,
                                    &SecPkgNegInfo);
        ok(SEC_SUCCESS(ss), "QueryContextAttributes failed with error 0x%08x\n", ss);
        if (!SEC_SUCCESS(ss))
        {
            err("QueryContextAttributes failed: 0x%08x\n", ss);
            printerr(ss);
            server2_cleanup();
            fatal_error("aborting\n");
        }
        else
        {
            trace("Package Name: %S\n", SecPkgNegInfo.PackageInfo->Name);
        }

        /* Free the allocated buffer */
        FreeContextBuffer(SecPkgNegInfo.PackageInfo);

        /* Impersonate the client */
        ss = ImpersonateSecurityContext(&hctxt);
        ok(SEC_SUCCESS(ss), "ImpersonateSecurityContext failed with error 0x%08x\n", ss);
        if (!SEC_SUCCESS(ss))
        {
            err("Impersonate failed: 0x%08x\n", ss);
            server2_cleanup();
            printerr(ss);
            fatal_error("aborting\n");
        }
        else
        {
            trace("Impersonation worked.\n");
        }

        GetUserName(NULL, &cchUserName);
        pUserName = (TCHAR*)malloc(cchUserName*sizeof(TCHAR));
        if (!pUserName)
        {
            err("Memory allocation error.\n");
            server2_cleanup();
            fatal_error("aborting\n");
        }

        if (!GetUserName(pUserName, &cchUserName))
        {
            err("Could not get the client name.\n");
            server2_cleanup();
            printerr(GetLastError());
            fatal_error("aborting\n");
        }
        else
        {
            trace("Client connected as :  %S\n", pUserName);
        }

        /* Revert to self */
        ss = RevertSecurityContext(&hctxt);
        ok(SEC_SUCCESS(ss), "RevertSecurityContext failed with error 0x%08x\n", ss);
        if (!SEC_SUCCESS(ss))
        {
            err("Revert failed: 0x%08x\n", ss);
            server2_cleanup();
            printerr(GetLastError());
            fatal_error("aborting\n");
        }
        else
        {
            trace("Reverted to self.\n");
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
            err("send message failed.\n");
            server2_cleanup();
            printerr(GetLastError());
            fatal_error("aborting\n");
        }

        trace(" %d encrypted bytes sent.\n", cbDataToClient);

        if (Server_Socket)
        {
            DeleteSecurityContext(&hctxt);
            FreeCredentialHandle(&hcred);
            shutdown(Server_Socket, 2);
            closesocket(Server_Socket);
            Server_Socket = 0;
        }

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

    trace("Server ran to completion without error.\n");
    server2_cleanup();
    return 0;
}

DWORD
server2_main(
    IN LPCTSTR PackageName)  // Example: _T("NTLM"), or _T("Negotiate")
{
    DWORD dwRet;
    WSADATA wsaData;

    /* Startup WSA */
    if (WSAStartup(0x0101, &wsaData))
        fatal_error("Could not initialize winsock.\n");

    /* Start the server */
    dwRet = server2_start(PackageName);

    /* Shutdown WSA and return */
    WSACleanup();
    return dwRet;
}
