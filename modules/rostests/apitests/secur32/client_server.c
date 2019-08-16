/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for client/server authentication via secur32 API.
 * PROGRAMMERS:     Samuel Serapión
 *                  Hermes Belusca-Maito
 */

#include "client_server.h"
#include <strsafe.h>

/*
 * Params:
 * IN LPCTSTR PackageName); // Example: _T("NTLM"), or _T("Negotiate")
 */
int server2_main(int argc, WCHAR** argv);

/*
 * Params:
 * IN LPCTSTR ServerName,   // ServerName must be defined as the name of the computer running the server sample.    Example: _T("127.0.0.1")
 * IN LPCTSTR TargetName,   // TargetName must be defined as the logon name of the user running the server program. Example: _T("")
 * IN LPCTSTR PackageName); // Example: _T("NTLM"), or _T("Negotiate")
 */
int client2_main(int argc, WCHAR** argv);

typedef int (*TESTPROC)(int argc, WCHAR** argv);

typedef struct _TESTDATA
{
    TESTPROC server_main;
    int svr_argc;
    WCHAR* svr_argv[2];
    TESTPROC client_main;
    int cli_argc;
    WCHAR* cli_argv[6];
} TESTDATA, *PTESTDATA;

#define TEST_AUTO
//#define TEST_NO_AUTO1
//#define TEST_NO_AUTO2

TESTDATA testdata[] =
{
#ifdef TEST_AUTO
    {   // Test 0
        server2_main, 2, { L"2000", L"NTLM" },
        // ip, port, targetname, package, usr, pwd
        client2_main, 6, { L"127.0.0.1", L"2000", L"", L"NTLM", L"", L"" }
    }
#endif
#ifdef TEST_NO_AUTO1
    {   // Test 0
        server2_main, 2, { L"2000", L"NTLM" },
        // ip, port, targetname, package, usr, pwd
        client2_main, 6, { L"<ip>", L"2000", L"", L"NTLM", L"<usr>", L"<pwd>" }
    },
#endif
#ifdef TEST_NO_AUTO2
    {   // Test 1 - not for automation
        // give params per cmd line <ip> <usr> <pwd>
        NULL, 0, {0},
        // Port 139 / 445 (smb)
        client2_main, 6, { L"<ip>", L"445", L"", L"NTLM", L"<usr>", L"<pwd>" }
    }
#endif
};

CRITICAL_SECTION sync_msg_cs;

void sync_msg_enter()
{
    EnterCriticalSection(&sync_msg_cs);
}

void sync_msg_leave()
{
    LeaveCriticalSection(&sync_msg_cs);
}

static DWORD WINAPI server_thread(void *param)
{
    PTESTDATA td = (PTESTDATA)param;

    td->server_main(td->svr_argc, td->svr_argv);

    return 0;
}

void test_runner(USHORT testidx)
{
    HANDLE hThreadServer = INVALID_HANDLE_VALUE;
    WCHAR FileName[MAX_PATH];
    LPWSTR *argv, parIp, parUsr, parPwd;
    int argc, nextarg;
    PTESTDATA td = &testdata[testidx];
    BOOL runCLI = (td->client_main != NULL);
    BOOL runSVR = (td->server_main != NULL);

    /* optionen
     * secur32_apitest ClientServer [cli] <ip> <usr> <pwd>
     * secur32_apitest ClientServer [svr]
     */

    /* replace <pwd> client-param if given .... */
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    nextarg = 2;
    if (argc >= nextarg+1)
    {
        if (wcscmp(L"cli", argv[nextarg]) == 0)
        {
            runSVR = FALSE;
            nextarg++;
        }
        else if (wcscmp(L"svr", argv[nextarg]) == 0)
        {
            runCLI = FALSE;
            nextarg++;
        }
    }
    if ((runCLI) &&
        (argc >= nextarg+3))
    {
        int i1;
        parIp = argv[nextarg];
        parUsr = argv[nextarg+1];
        parPwd = argv[nextarg+2];
        for (i1 = 0; i1 < td->cli_argc; i1++)
        {
            if (wcscmp(td->cli_argv[i1], L"<ip>") == 0)
                td->cli_argv[i1] = parIp;
            if (wcscmp(td->cli_argv[i1], L"<usr>") == 0)
                td->cli_argv[i1] = parUsr;
            if (wcscmp(td->cli_argv[i1], L"<pwd>") == 0)
                td->cli_argv[i1] = parPwd;
        }
        sync_trace("ip %S; user %S\n", parIp, parUsr);
    }

    sync_trace("runCli %i, runSVR %i\n", runCLI, runSVR);

    /* Retrieve our full path */
    if (!GetModuleFileNameW(NULL, FileName, _countof(FileName)))
    {
        skip("GetModuleFileNameW failed with error %lu!\n", GetLastError());
        return;
    }

    if (runSVR)
    {
        hThreadServer = CreateThread(NULL, 0, server_thread, td, 0, NULL);
        ok(hThreadServer != NULL, "CreateThread failed: %lu\n", GetLastError());
        if (!hThreadServer)
            goto Quit;
    }

    /* Wait 500 millisecs for server initialization */
    Sleep(500);

    /*
     * Run client
     */

    if (runCLI)
        td->client_main(td->cli_argc, td->cli_argv);

    if (hThreadServer != INVALID_HANDLE_VALUE)
        ok(WaitForSingleObject(hThreadServer, 10000) == WAIT_OBJECT_0, "Timeout waiting for thread\n");

Quit:
    if (hThreadServer != INVALID_HANDLE_VALUE)
    {
        /* Be sure to kill the thread if it did not already terminate */
        TerminateThread(hThreadServer, 0);
        CloseHandle(hThreadServer);
    }
}

/*
 * CRYPT
 */

VOID
CodeCalcAndAllocBuffer(
    IN ULONG cbMessage,
    IN PSecPkgContext_Sizes pSecSizes,
    OUT PBYTE* ppBuffer,
    OUT PULONG pBufLen)
{
    *pBufLen = pSecSizes->cbSecurityTrailer + cbMessage + sizeof(DWORD);
    if (ppBuffer)
        *ppBuffer = (PBYTE)malloc(*pBufLen);
}

BOOL
CodeEncrypt(
    IN PSecHandle hCtxt,
    IN PBYTE pbMessage,
    IN ULONG cbMessage,
    IN PSecPkgContext_Sizes pSecSizes,
    IN ULONG cbBufLen,
    OUT PBYTE pOutBuf)
{
    SECURITY_STATUS ss;
    SecBufferDesc BuffDesc;
    //SecBuffer SecBuff[4];
    SecBuffer SecBuff[4];
    ULONG /*cbIoBufLen, */neededBufSize, fQOP = 0;
    //PBYTE pbIoBuffer;

    /* cbSecurityTrailer:
     * The size of the trailer (signature + padding) block is
     * determined from the global cbSecurityTrailer.
     */

    sync_trace("Data (%ld bytes) before encryption: %.*s\n",
                cbMessage, (int)cbMessage/sizeof(char), pbMessage);

    /*
     * Allocate a buffer to hold the a DWORD, signature,
     * and encrypted data
     * that specifies the size of the trailer block.
     */
    CodeCalcAndAllocBuffer(cbMessage, pSecSizes, NULL, &neededBufSize);
    printf("need size %ld\n", neededBufSize);
    if (neededBufSize > cbBufLen)
    {
        sync_err("Buffer to small\n");
        return FALSE;
    }
    //cbIoBufLen = pSecSizes->cbHeader +
    //             /*pSecSizes->cbMaximumMessage * */2*cbMessage +
    //             pSecSizes->cbTrailer;
    /*if(!(pOutBuf = (PBYTE)LocalAlloc(LMEM_FIXED, neededBufSize)))
    {
        sync_err("Out of memory");
        return FALSE;
    }*/
//DebugBreak();
    /* Prepare buffers */
    memset(SecBuff, 0, sizeof(SecBuff));
    memset(pOutBuf, 0, neededBufSize);
    SecBuff[0].cbBuffer = pSecSizes->cbSecurityTrailer;
    SecBuff[0].BufferType = SECBUFFER_TOKEN;
    SecBuff[0].pvBuffer = pOutBuf + sizeof(DWORD);

    SecBuff[1].cbBuffer = cbMessage;
    SecBuff[1].BufferType = SECBUFFER_DATA;
    SecBuff[1].pvBuffer = pOutBuf + sizeof(DWORD) +
                          pSecSizes->cbSecurityTrailer;
    SecBuff[2].BufferType = SECBUFFER_EMPTY;
    SecBuff[3].BufferType = SECBUFFER_EMPTY;

    /* header */
    /*SecBuff[0].pvBuffer     = pbIoBuffer;
    SecBuff[0].cbBuffer     = pSecSizes->cbHeader;
    SecBuff[0].BufferType   = SECBUFFER_STREAM_HEADER;
    / * message * /
    SecBuff[1].pvBuffer     = pbIoBuffer +
                              pSecSizes->cbHeader;
    SecBuff[1].cbBuffer     = cbMessage;
    SecBuff[1].BufferType   = SECBUFFER_DATA;
    / * trailer * /
    SecBuff[2].pvBuffer     = pbIoBuffer +
                              pSecSizes->cbHeader +
                              cbMessage;
    SecBuff[2].cbBuffer     = pSecSizes->cbTrailer;
    SecBuff[2].BufferType   = SECBUFFER_STREAM_TRAILER;
    / * empty * /
    SecBuff[3].BufferType   = SECBUFFER_EMPTY;*/

    BuffDesc.ulVersion = SECBUFFER_VERSION;
    BuffDesc.cBuffers = ARRAYSIZE(SecBuff);
    BuffDesc.pBuffers = SecBuff;

    //sync_trace("make sign\n");
    //PrintHexDumpMax(neededBufSize, pOutBuf, neededBufSize);

    /*ss = MakeSignature(hCtxt, 0, &BuffDesc, 0);
    sync_ok(SEC_SUCCESS(ss), "MakeSignature failed with error 0x%08lx\n", ss);
    if (!SEC_SUCCESS(ss))
        return FALSE;
    sync_trace("after make sign\n");
    PrintHexDumpMax(neededBufSize, pOutBuf, neededBufSize);*/

    /* the data will be encrypted in place ... so we have to copy it
       first to the buffer */
    memset(SecBuff[0].pvBuffer, 0x00, SecBuff[0].cbBuffer);
    memcpy(SecBuff[1].pvBuffer, pbMessage, cbMessage);

    //sync_trace("SecBuff (before encrypt)!\n");
    //PrintHexDumpMax(sizeof(SecBuff), (PBYTE)&SecBuff, sizeof(SecBuff));
    //PrintHexDumpMax(SecBuff[0].cbBuffer, (PBYTE)SecBuff[0].pvBuffer, SecBuff[0].cbBuffer);
    //PrintHexDumpMax(SecBuff[1].cbBuffer, (PBYTE)SecBuff[1].pvBuffer, SecBuff[1].cbBuffer);

    //ss = MakeSignature(hCtxt, 0, &BuffDesc, 0);
    //sync_ok(SEC_SUCCESS(ss), "MakeSignature failed with error 0x%08lx\n", ss);
    //if (!SEC_SUCCESS(ss))
    //    return FALSE;

    //ss = VerifySignature(hCtxt, &BuffDesc, 0, NULL);
    //sync_ok(SEC_SUCCESS(ss), "VerifySignature failed with error 0x%08lx\n", ss);
    //if (!SEC_SUCCESS(ss))
    //    return FALSE;

    ss = EncryptMessage(hCtxt, fQOP, &BuffDesc, 0);
    sync_ok(SEC_SUCCESS(ss), "EncryptMessage failed with error 0x%08lx\n", ss);
    if (!SEC_SUCCESS(ss))
        return FALSE;

    //sync_trace("after encrypt sign\n");
    //PrintHexDumpMax(neededBufSize, pOutBuf, neededBufSize);

    //sync_trace("SecBuff (after encrypt)!\n");
    //PrintHexDumpMax(sizeof(SecBuff), (PBYTE)&SecBuff, sizeof(SecBuff));
    //PrintHexDumpMax(SecBuff[0].cbBuffer, (PBYTE)SecBuff[0].pvBuffer, SecBuff[0].cbBuffer);
    //PrintHexDumpMax(SecBuff[1].cbBuffer, (PBYTE)SecBuff[1].pvBuffer, SecBuff[1].cbBuffer);

    sync_trace("The message has been encrypted.\n");
    /* set Byte 0-3 (DWORD) Trailer-Size  */
    *((DWORD*)pOutBuf) = pSecSizes->cbSecurityTrailer;


    /*ss = MakeSignature(hCtxt, 0, &BuffDesc, 0);
    sync_ok(SEC_SUCCESS(ss), "MakeSignature failed with error 0x%08lx\n", ss);
    if (!SEC_SUCCESS(ss))
        return FALSE;
    sync_trace("after make sign\n");
    PrintHexDumpMax(neededBufSize, pOutBuf, neededBufSize);
*/
    /*if (FALSE) {
        //SEC_E_MESSAGE_ALTERED
        //ULONG ulQop;
        //BuffDesc.pBuffers[0].cbBuffer--;
    BuffDesc.cBuffers = 2;
    SecBuff[2].BufferType = SECBUFFER_PADDING;
    SecBuff[3].BufferType = SECBUFFER_EMPTY;
    ss = DecryptMessage(hCtxt, &BuffDesc, 0, &fQOP);
    //ss = DecryptMessage(hCtxt, &BuffDesc, 0, &ulQop);
    sync_ok(SEC_SUCCESS(ss), "DecryptMessage (A) failed with error 0x%08lx\n", ss);
    if (!SEC_SUCCESS(ss))
        return FALSE;
    }*/
    return TRUE;
}

/******************************************************************************/

START_TEST(ClientServer)
{
    InitializeCriticalSection(&sync_msg_cs);
    NtlmCheckInit();

    test_runner(0);

    NtlmCheckFini();
    DeleteCriticalSection(&sync_msg_cs);
}
