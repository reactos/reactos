//
// MSDN Example "Using SSPI with a Windows Sockets Client"
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa380536(v=vs.85).aspx
//

//--------------------------------------------------------------------
//  Client-side program to establish an SSPI socket connection
//  with a server and exchange messages.

//--------------------------------------------------------------------
//  Define macros and constants.

#include "client_server.h"

#define g_usPort 2000

#define BIG_BUFF    2048

#define cbMaxMessage 12000
#define MessageAttribute ISC_REQ_CONFIDENTIALITY

BOOL
client2_DoAuthentication(
    IN LPCTSTR TargetName,
    IN LPCTSTR PackageName,
    IN SOCKET s,
    IN PCredHandle hCred,
    IN PSecHandle  hcText);

BOOL
GenClientContext(
    PBYTE pIn,
    DWORD cbIn,
    PBYTE pOut,
    DWORD *pcbOut,
    BOOL  *pfDone,
    LPCTSTR pszTarget,
    LPCTSTR PackageName,
    PCredHandle hCred,
    PSecHandle  hcText);

//--------------------------------------------------------------------
//  ConnectAuthSocket establishes an authenticated socket connection
//  with a server and initializes needed security package resources.

BOOL
client2_ConnectAuthSocket(
    IN  LPCTSTR ServerName,
    IN  LPCTSTR TargetName,
    IN  LPCTSTR PackageName,
    OUT SOCKET     *s,
    OUT PCredHandle hCred,
    OUT PSecHandle  hcText)
{
    unsigned long  ulAddress;
    struct hostent *pHost;
    SOCKADDR_IN    sin;

#ifdef UNICODE
    char AnsiServerName[256];

    WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
        ServerName, -1, AnsiServerName, _countof(AnsiServerName), NULL, NULL);
#else
#define AnsiServerName ServerName
#endif

    /* Lookup the server's address */
    ulAddress = inet_addr(AnsiServerName);

    if (ulAddress == INADDR_NONE)
    {
        pHost = gethostbyname(AnsiServerName);
        if (!pHost)
            fatal_error("Unable to resolve host name ");

        memcpy((char *)&ulAddress, pHost->h_addr, pHost->h_length);
    }

#ifndef UNICODE
#undef AnsiServerName
#endif

    /* Create the socket */
    *s = socket(PF_INET, SOCK_STREAM, 0);
    if (*s == INVALID_SOCKET)
        fatal_error("Unable to create socket\n");

    trace("client socket %x created.\n",*s);

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ulAddress;
    sin.sin_port = htons(g_usPort);

    /* Connect to the server */
    if (connect(*s, (LPSOCKADDR)&sin, sizeof(sin)))
    {
        closesocket(*s);
        fatal_error("Connect failed\n");
    }

    /* Authenticate the connection */
    if (!client2_DoAuthentication(TargetName, PackageName,
                                  *s, hCred, hcText))
    {
        closesocket(*s);
        fatal_error("Authentication failed\n");
    }

    return TRUE;
}

BOOL
client2_DoAuthentication(
    IN LPCTSTR TargetName,
    IN LPCTSTR PackageName,
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
        fatal_error("Memory allocation ");

    if (!(pOutBuf = (PBYTE)malloc(cbMaxMessage)))
        fatal_error("Memory allocation ");

    cbOut = cbMaxMessage;
    Success = GenClientContext(NULL,
                               0,
                               pOutBuf,
                               &cbOut,
                               &fDone,
                               TargetName,
                               PackageName,
                               hCred,
                               hcText);
    ok(Success, "GenClientContext failed\n");
    if (!Success)
    {
        err("GenClientContext failed!\n");
        return FALSE;
    }

    if (!SendMsg(s, pOutBuf, cbOut))
        fatal_error("Send message failed ");

    while (!fDone)
    {
        if (!ReceiveMsg(s, pInBuf, cbMaxMessage, &cbIn))
            fatal_error("Receive message failed ");

        cbOut = cbMaxMessage;
        Success = GenClientContext(pInBuf,
                                   cbIn,
                                   pOutBuf,
                                   &cbOut,
                                   &fDone,
                                   TargetName,
                                   PackageName,
                                   hCred,
                                   hcText);
        ok(Success, "GenClientContext failed\n");
        if (!Success)
        {
            fatal_error("GenClientContext failed");
        }
        if (!SendMsg(s, pOutBuf, cbOut))
            fatal_error("Send message 2 failed ");
    }

    trace("DoAuthentication end\n");
    free(pInBuf);
    free(pOutBuf);
    return TRUE;
}

BOOL
GenClientContext(
    PBYTE pIn,
    DWORD cbIn,
    PBYTE pOut,
    DWORD *pcbOut,
    BOOL  *pfDone,
    LPCTSTR pszTarget,
    LPCTSTR PackageName,
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

    if (!pIn)
    {
        ss = AcquireCredentialsHandle(NULL,
                                      (LPTSTR)PackageName,
                                      SECPKG_CRED_OUTBOUND,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      hCred,
                                      &Lifetime);
        ok(SEC_SUCCESS(ss), "AcquireCredentialsHandle failed with error 0x%08x\n", ss);
        if (!SEC_SUCCESS(ss))
            fatal_error("AcquireCreds failed ");
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

        ss = InitializeSecurityContext(hCred,
                                       hcText,
                                       (LPTSTR)pszTarget,
                                       MessageAttribute,
                                       0,
                                       SECURITY_NATIVE_DREP,
                                       &InBuffDesc,
                                       0,
                                       hcText,
                                       &OutBuffDesc,
                                       &ContextAttributes,
                                       &Lifetime);
    }
    else
    {
        ss = InitializeSecurityContext(hCred,
                                       NULL,
                                       (LPTSTR)pszTarget,
                                       MessageAttribute,
                                       0,
                                       SECURITY_NATIVE_DREP,
                                       NULL,
                                       0,
                                       hcText,
                                       &OutBuffDesc,
                                       &ContextAttributes,
                                       &Lifetime);
    }

    ok(SEC_SUCCESS(ss), "InitializeSecurityContext failed with error 0x%08x\n", ss);
    if (!SEC_SUCCESS(ss))
        fatal_error("InitializeSecurityContext failed ");

    /* If necessary, complete the token */
    if ((ss == SEC_I_COMPLETE_NEEDED) ||
        (ss == SEC_I_COMPLETE_AND_CONTINUE))
    {
        ss = CompleteAuthToken(hcText, &OutBuffDesc);
        ok(SEC_SUCCESS(ss), "CompleteAuthToken failed with error 0x%08x\n", ss);
        if (!SEC_SUCCESS(ss))
        {
            err("complete failed: 0x%08x\n", ss);
            return FALSE;
        }
    }

    *pcbOut = OutSecBuff.cbBuffer;

    *pfDone = !((ss == SEC_I_CONTINUE_NEEDED) ||
                (ss == SEC_I_COMPLETE_AND_CONTINUE));

    trace("Token buffer generated (%lu bytes):\n", OutSecBuff.cbBuffer);
    PrintHexDump(OutSecBuff.cbBuffer, (PBYTE)OutSecBuff.pvBuffer);
    return TRUE;

}

PBYTE
DecryptThis(
    PBYTE   pBuffer,
    LPDWORD pcbMessage,
    PSecHandle hCtxt,
    ULONG   cbSecurityTrailer)
{
    SECURITY_STATUS   ss;
    SecBufferDesc     BuffDesc;
    SecBuffer         SecBuff[2];
    ULONG             ulQop = 0;
    PBYTE             pSigBuffer;
    PBYTE             pDataBuffer;
    DWORD             SigBufferSize;

    /*
     * By agreement, the server encrypted the message and set the size
     * of the trailer block to be just what it needed. DecryptMessage
     * needs the size of the trailer block.
     * The size of the trailer is in the first DWORD of the
     * message received.
     */
    SigBufferSize = *((DWORD*)pBuffer);
    trace("data before decryption including trailer (%lu bytes):\n",
          *pcbMessage);
    PrintHexDump(*pcbMessage, (PBYTE)pBuffer);

    /*
     * By agreement, the server placed the trailer at the beginning
     * of the message that was sent immediately following the trailer
     * size DWORD.
     */
    pSigBuffer = pBuffer + sizeof(DWORD);

    /* The data comes after the trailer */
    pDataBuffer = pSigBuffer + SigBufferSize;

    /* *pcbMessage is reset to the size of just the encrypted bytes */
    *pcbMessage = *pcbMessage - SigBufferSize - sizeof(DWORD);

    /*
     * Prepare the buffers to be passed to the DecryptMessage function
     */

    BuffDesc.ulVersion    = 0;
    BuffDesc.cBuffers     = ARRAYSIZE(SecBuff);
    BuffDesc.pBuffers     = SecBuff;

    SecBuff[0].cbBuffer   = SigBufferSize;
    SecBuff[0].BufferType = SECBUFFER_TOKEN;
    SecBuff[0].pvBuffer   = pSigBuffer;

    SecBuff[1].cbBuffer   = *pcbMessage;
    SecBuff[1].BufferType = SECBUFFER_DATA;
    SecBuff[1].pvBuffer   = pDataBuffer;

    ss = DecryptMessage(hCtxt, &BuffDesc, 0, &ulQop);
    ok(SEC_SUCCESS(ss), "DecryptMessage failed");

    /* Return a pointer to the decrypted data. The trailer data is discarded. */
    return pDataBuffer;
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
    trace("data before verifying (including signature):\n");
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
    ok(SEC_SUCCESS(ss), "VerifySignature failed with error 0x%08x\n", ss);
    if (!SEC_SUCCESS(ss))
        err("VerifyMessage failed");
    else
        trace("Message was properly signed.\n");

    return pDataBuffer;
}

DWORD WINAPI
client2_start(
    IN LPCTSTR ServerName,
    IN LPCTSTR TargetName,
    IN LPCTSTR PackageName)
{
    SOCKET          Client_Socket;
    BYTE            Data[BIG_BUFF];
    PCHAR           pMessage;
    CredHandle      hCred;
    SecHandle       hCtxt;
    SECURITY_STATUS ss;
    DWORD           cbRead;
    ULONG           cbMaxSignature;
    ULONG           cbSecurityTrailer;
    SecPkgContext_Sizes           SecPkgContextSizes;
    SecPkgContext_NegotiationInfo SecPkgNegInfo;

    /* Connect to a server */
    if (!client2_ConnectAuthSocket(ServerName, TargetName, PackageName,
                                   &Client_Socket, &hCred, &hCtxt))
    {
        fatal_error("Unable to authenticate server connection \n");
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
    ok(SEC_SUCCESS(ss), "QueryContextAttributes failed with error 0x%08x\n", ss);
    if (!SEC_SUCCESS(ss))
        fatal_error("QueryContextAttributes failed ");
    else
        trace("Package Name: %S\n", SecPkgNegInfo.PackageInfo->Name);

    ss = QueryContextAttributes(&hCtxt,
                                SECPKG_ATTR_SIZES,
                                &SecPkgContextSizes);
    ok(SEC_SUCCESS(ss), "Querycontext2 failed!");

    cbMaxSignature = SecPkgContextSizes.cbMaxSignature;
    cbSecurityTrailer = SecPkgContextSizes.cbSecurityTrailer;


    /*
     * Decrypt and display the message from the server
     */
    if (!ReceiveBytes(Client_Socket,
                      Data,
                      BIG_BUFF,
                      &cbRead))
    {
        fatal_error("No response from server ");
    }

    ok(cbRead != 0, "Zero bytes received ");

    pMessage = (PCHAR)DecryptThis(Data,
                                  &cbRead,
                                  &hCtxt,
                                  cbSecurityTrailer);
    trace("message len: %d message: %.*S\n", cbRead, cbRead/sizeof(TCHAR), pMessage);

    /* Terminate socket and security package */
    DeleteSecurityContext(&hCtxt);
    FreeCredentialHandle(&hCred);
    shutdown(Client_Socket, 2);
    closesocket(Client_Socket);

    return 0; // EXIT_SUCCESS
}

DWORD
client2_main(
    IN LPCTSTR ServerName,   // ServerName must be defined as the name of the computer running the server sample.    Example: _T("127.0.0.1")
    IN LPCTSTR TargetName,   // TargetName must be defined as the logon name of the user running the server program. Example: _T("")
    IN LPCTSTR PackageName)  // Example: _T("NTLM"), or _T("Negotiate")
{
    DWORD dwRet;
    WSADATA wsaData;

    /* Startup WSA */
    if (WSAStartup(0x0101, &wsaData))
        fatal_error("Could not initialize winsock.\n");

    /* Start the client */
    dwRet = client2_start(ServerName, TargetName, PackageName);

    /* Shutdown WSA and return */
    WSACleanup();
    return dwRet;
}
