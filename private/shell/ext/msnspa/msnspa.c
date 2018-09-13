/*****************************************************************************
 *
 *  MSNSPA.c
 *
 *  Copyright (c) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      MSN SPA Proxy.
 *
 *      Proxies POP and NNTP for clients that don't speak them natively.
 *
 *      Runs as app that minimizes to nowhere.  Get it back by Alt+Tab'ing
 *      to it.
 *
 *****************************************************************************/

#include "msnspa.h"

/*****************************************************************************
 *
 *      Globals
 *
 *****************************************************************************/

HINSTANCE g_hinst;
HINSTANCE g_hinstSecur;
PSecurityFunctionTable g_psft;

#ifdef DBG
/*****************************************************************************
 *
 *  Squirt - Print a message
 *
 *****************************************************************************/

void __cdecl
Squirt(LPCTSTR ptszMsg, ...)
{
    TCHAR tsz[1024 + PLENTY_BIG];
    va_list ap;
    va_start(ap, ptszMsg);
    wvsprintf(tsz, ptszMsg, ap);

    OutputDebugString(tsz);
}
#endif

/*****************************************************************************
 *
 *  Die - Death
 *
 *****************************************************************************/

void __cdecl
Die(LPCTSTR ptszMsg, ...)
{
    TCHAR tsz[1024];
    va_list ap;
    va_start(ap, ptszMsg);
    wvsprintf(tsz, ptszMsg, ap);

    OutputDebugString(tsz);
    OutputDebugString(TEXT("\r\n"));
    ExitProcess(1);
}

/*****************************************************************************
 *
 *  IsNT
 *
 *****************************************************************************/

BOOL
IsNT(void)
{
    return (int)GetVersion() >= 0;
}

/*****************************************************************************
 *
 *  RFC1113 translation tables
 *
 *****************************************************************************/

const char RFC1113_From[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27,
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

const char RFC1113_To[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

#define chPad   '='

/*****************************************************************************
 *
 *  RFC1113_Encode
 *
 *  Convert a binary blob into an ASCII string using RFC1113 encoding.
 *  (I think it's RFC1113.  Boy would it be embarrassing if it weren't.)
 *
 *  szBuf - output buffer, will be null-terminated
 *  rgbIn - source buffer to be encoded
 *  cbIn  - number of bytes in source buffer
 *
 *****************************************************************************/

void
RFC1113_Encode(LPSTR szBuf, const BYTE *rgbIn, UINT cbIn)
{
   LPSTR psz = szBuf;
   UINT ib;

   unsigned char *outptr;
   unsigned int   i;

   for (ib = 0; ib < cbIn; ib += 3) {
      *psz++ = RFC1113_To[*rgbIn >> 2];
      *psz++ = RFC1113_To[((*rgbIn << 4) & 060) | ((rgbIn[1] >> 4) & 017)];
      *psz++ = RFC1113_To[((rgbIn[1] << 2) & 074) | ((rgbIn[2] >> 6) & 03)];
      *psz++ = RFC1113_To[rgbIn[2] & 077];

      rgbIn += 3;
   }

   /*
    *   If cbIn was not a multiple of 3, then we have encoded too
    *   many characters.  Adjust appropriately.
    */
   if (ib == cbIn + 1) {
      /* There were only 2 bytes in that last group */
      psz[-1] = chPad;
   } else if (ib == cbIn + 2) {
      /* There was only 1 byte in that last group */
      psz[-1] = chPad;
      psz[-2] = chPad;
   }

   *psz = '\0';

}

/*****************************************************************************
 *
 *  RFC1113_Decode
 *
 *  Convert an ASCII string back into a binary blob.
 *
 *  rgbOut - output buffer
 *  szIn   - source buffer
 *
 *  Returns number of bytes converted.
 *
 *****************************************************************************/

#define RFC1113x(ch) (RFC1113_From[(BYTE)(ch)] & 63)

int
RFC1113_Decode(LPBYTE rgbOut, LPSTR szIn)
{
    int nbytesdecoded;
    LPSTR psz;
    BYTE *rgb = rgbOut;
    int cchIn;
    int cbRc;

    /*
     *  Skip leading whitespace, just to be safe.
     */

    while (szIn[0] == ' ' || szIn[0] == '\t') {
        szIn++;
    }

    /*
     *  Figure out how many characters are in the input buffer.
     */
    psz = szIn;
    while (RFC1113_From[(BYTE)*psz] < 64) {
        psz++;
    }

    /*
     *  The caller will pad the input string to a multiple of 4 in
     *  length with chPad's.
     *
     *  Three bytes out for each four chars in.
     */
    cchIn = psz - szIn;

    cbRc = ((cchIn + 3) / 4) * 3;

    /*
     *  Now decode it.
     */
    psz = szIn;

    while (cchIn > 0) {
        *rgb++ =
            (BYTE)(RFC1113x(psz[0]) << 2 | RFC1113x(psz[1]) >> 4);
        *rgb++ =
            (BYTE) (RFC1113x(psz[1]) << 4 | RFC1113x(psz[2]) >> 2);
        *rgb++ =
            (BYTE) (RFC1113x(psz[2]) << 6 | RFC1113x(psz[3]));
        psz += 4;
        cchIn -= 4;
    }

    /*
     *  Now adjust the number of output bytes based on the number of
     *  input equal-signs.
     */
    if (cchIn & 3) {
        if(RFC1113_From[psz[-2]] > 63) {
            rgbOut[cbRc - 2] = 0;
            cbRc -= 2;
        } else {
            rgbOut[cbRc - 1] = 0;
            cbRc -= 1;
        }
    }

    return cbRc;
}

/*****************************************************************************
 *
 *  LoadSecurityManager
 *
 *  Obtain all the entry points into the security DLL.
 *
 *****************************************************************************/

BOOL
LoadSecurityManager(void)
{
    INIT_SECURITY_INTERFACE InitSecurityInterface;

    /*
     *  On NT, the security DLL is named SECURITY.DLL.
     *  On 95, the security DLL is named SECUR32.DLL.
     *
     *  Go figure.
     */
    g_hinstSecur = LoadLibrary(IsNT() ? TEXT("security.dll") :
                                        TEXT("secur32.dll"));
    if (!g_hinstSecur) {
        Squirt(TEXT("Can't load security manager") EOL);
        return FALSE;
    }

    InitSecurityInterface = (INIT_SECURITY_INTERFACE)
                            GetProcAddress(g_hinstSecur, SECURITY_ENTRYPOINT);

    if (!InitSecurityInterface) {
        Squirt(TEXT("Can't find entrypoint") EOL);
        return FALSE;
    }

    g_psft = InitSecurityInterface();
    if (!g_psft) {
        Squirt(TEXT("Unable to init security interface") EOL);
        return FALSE;
    }

    Squirt(TEXT("Security manager successfully loaded") EOL);
    return TRUE;
}

/*****************************************************************************
 *
 *  Security_AcquireCredentials
 *
 *  pwas -> WIN32AUTHSTATE to track the state of this session
 *  ptszPackage - name of security package (e.g., "MSN")
 *
 *****************************************************************************/

BOOL INTERNAL
Security_AcquireCredentials(PWIN32AUTHSTATE pwas, LPTSTR ptszPackage)
{
    TimeStamp       tsExpires;
    SECURITY_STATUS ss;
    char szToken[PLENTY_BIG];

    /*
     *  Clean slate.
     */
    ZeroMemory(pwas, sizeof(*pwas));

    ss = g_psft->AcquireCredentialsHandle(
                NULL,                   /* Use credentials of current user  */
                ptszPackage,            /* Use this security package        */
                SECPKG_CRED_OUTBOUND,   /* I am the untrusted one           */
                NULL,                   /* Not gonna access remote files    */
                NULL,                   /* Additional info                  */
                NULL,                   /* No credential retriever          */
                NULL,                   /* No credential retriever          */
                &pwas->hCred,           /* Receives credentials handle      */
                &tsExpires);            /* Expiration time for hCred        */

    if (ss == SEC_E_OK) {
        pwas->fHCredValid = TRUE;
    }

    return (ss == SEC_E_OK);

}

/*****************************************************************************
 *
 *  Security_BuildOutString
 *
 *  Build a string that will be output.
 *
 *****************************************************************************/

BOOL
Security_BuildOutString(PWIN32AUTHSTATE pwas, PSecBufferDesc pdescIn,
                        PCtxtHandle pctxOld, PCtxtHandle pctxNew,
                        LPTSTR ptszTarget)
{
    SecBuffer       bufOut;
    SecBufferDesc   descOut;
    TimeStamp       tsExpire;
    SECURITY_STATUS ss;
    BYTE            rgbToken[PLENTY_BIG];
    ULONG           fContextAttrib;

    /*
     *  Set up the buffers...
     */
    descOut.ulVersion = SECBUFFER_VERSION;
    descOut.cBuffers  = 1;
    descOut.pBuffers  = &bufOut;

    bufOut.cbBuffer = PLENTY_BIG;
    bufOut.BufferType = SECBUFFER_TOKEN;
    bufOut.pvBuffer = rgbToken;

retry:;
    ss = g_psft->InitializeSecurityContext(
                &pwas->hCred,           /* Remember me?                     */
                pctxOld,                /* Current context                  */
                ptszTarget,             /* Server name                      */
                pwas->fContextReq,      /* Context requiremnents            */
                0,                      /* (reserved)                       */
                SECURITY_NATIVE_DREP,   /* Target data representation       */
                pdescIn,                /* Input buffer descriptor          */
                0,                      /* (reserved)                       */
                pctxNew,                /* New context                      */
                &descOut,               /* Output buffer descriptor         */
                &fContextAttrib,        /* Receives context attributes      */
                &tsExpire);             /* Expiration time                  */

    /*
     *  If we failed to obtain credentials, and we haven't yet prompted
     *  the user, then try again with prompting.
     */
    if (ss == SEC_E_NO_CREDENTIALS &&
        !(pwas->fContextReq & ISC_REQ_PROMPT_FOR_CREDS)) {
        pwas->fContextReq |= ISC_REQ_PROMPT_FOR_CREDS;
        goto retry;
    }

    if (FAILED(ss)) {
        Squirt(TEXT("Logon failed") EOL);
        return FALSE;
    }

    /*
     *  Oh dear, a continuation record?  Ack, I can't handle that
     *  because I'm lazy.
     */
    if (ss == SEC_I_CONTINUE_NEEDED) {
        /* Aigh! */
    }

    /*
     *  Since POP and NNTP are text-based protocols, we need to
     *  RFC1113-encode the binary data before transmitting.
     */

    RFC1113_Encode(pwas->szBuffer, rgbToken, bufOut.cbBuffer);

    return TRUE;
}

/*****************************************************************************
 *
 *  Security_GetNegotiation
 *
 *  Begin the transaction by building a negotiation string
 *
 *****************************************************************************/

BOOL INTERNAL
Security_GetNegotiation(PWIN32AUTHSTATE pwas)
{
    BOOL            fRc;

    /*
     *  We're starting over; throw away any leftover context.
     */
    if (pwas->fHCtxtValid) {
        g_psft->DeleteSecurityContext(&pwas->hCtxt);
        pwas->fHCtxtValid = FALSE;
    }

    /*
     *  Use common worker function to generate an output string.
     */

    fRc = Security_BuildOutString(
                        pwas,           /* Authorization state              */
                        NULL,           /* No input buffer                  */
                        NULL,           /* No source context                */
                        &pwas->hCtxt,   /* Destination context              */
                        NULL);          /* Server name                      */

    /*
     *  If it worked, then the hCtxt is valid and needs to be
     *  deleted when we're done.
     */
    if (fRc) {
        pwas->fHCtxtValid = TRUE;
    }

    return fRc;
}

/*****************************************************************************
 *
 *  Security_GetResponse
 *
 *  Build a reponse to the server's challenge.
 *
 *****************************************************************************/

BOOL INTERNAL
Security_GetResponse(PWIN32AUTHSTATE pwas, LPSTR szChallenge)
{
    BOOL            fRc;
    BYTE            rgbChallenge[PLENTY_BIG];
    int             cb;
    SecBuffer       bufIn;
    SecBufferDesc   descIn;

    cb = RFC1113_Decode(rgbChallenge, szChallenge);

#ifdef CHATTY
    Squirt("Decoded %d bytes" EOL, cb);
#endif

    /*
     *  Set up the buffers...
     */
    descIn.ulVersion = SECBUFFER_VERSION;
    descIn.cBuffers  = 1;
    descIn.pBuffers  = &bufIn;

    bufIn.cbBuffer   = cb;
    bufIn.BufferType = SECBUFFER_TOKEN;
    bufIn.pvBuffer   = rgbChallenge;

    /*
     *  Use common worker function to generate an output string.
     */

    fRc = Security_BuildOutString(
                        pwas,           /* Authorization state              */
                        &descIn,        /* No input buffer                  */
                        &pwas->hCtxt,   /* No source context                */
                        &pwas->hCtxt,   /* Destination context              */
                        NULL);          /* Server name                      */

    return fRc;
}

/*****************************************************************************
 *
 *  Security_ReleaseCredentials
 *
 *  pwas -> WIN32AUTHSTATE to track the state of this session
 *  ptszPackage - name of security package (e.g., "MSN")
 *
 *****************************************************************************/

void INTERNAL
Security_ReleaseCredentials(PWIN32AUTHSTATE pwas)
{
    if (pwas->fHCtxtValid) {
        g_psft->DeleteSecurityContext(&pwas->hCtxt);
        pwas->fHCtxtValid = FALSE;
    }

    if (pwas->fHCredValid) {
        g_psft->FreeCredentialHandle(&pwas->hCred);
        pwas->fHCredValid = FALSE;
    }
}

/*****************************************************************************
 *
 *  sendsz
 *
 *      Send an asciiz string.
 *
 *****************************************************************************/

int
sendsz(SOCKET s, LPCSTR psz)
{
    return send(s, psz, lstrlen(psz), 0);
}

#if 0
/*****************************************************************************
 *
 *  POP3_Negotiate
 *
 *      Perform an authenticated MSN logon.
 *
 *****************************************************************************/

void
POP3_Negotiate(PCONNECTIONSTATE pcxs)
{
    WIN32AUTHSTATE was;
    int cb;

    /*
     *  Tell the server to go into MSN mode.
     */
    sendsz(pcxs->ssfd, "AUTH MSN\r\n");

    /*
     *  Wait for the Proceed.
     */
    cb = recv(pcxs->ssfd, pcxs->buf, BUFSIZE, 0); /* read a hunk */
    if (cb <= 0 || pcxs->buf[0] != '+') {
        sendsz(pcxs->scfd, "-ERR Server lost 1\r\n");
        return;
    }

    pcxs->buf[cb] = 0;
#ifdef CHATTY
    Squirt("<%s", pcxs->buf);
#endif

    if (!Security_AcquireCredentials(&was, TEXT("MSN"), NULL)) {
        Die(TEXT("Cannot acquire credentials handle"));
    }

    if (!Security_GetNegotiation(&was)) {
        Die(TEXT("Cannot get negotiation string"));
    }

    /*
     *  Now send the initial cookie.
     */
    wsprintf(pcxs->buf, "%s\r\n", was.szBuffer);
    sendsz(pcxs->ssfd, pcxs->buf);
#ifdef CHATTY
    Squirt(">%s", pcxs->buf);
#endif

    /*
     *  Response should be
     *
     *  + <challenge>
     */
    cb = recv(pcxs->ssfd, pcxs->buf, BUFSIZE, 0);

    if (cb <= 0 || pcxs->buf[0] != '+') {
        if (cb > 0) {
            pcxs->buf[cb] = 0;
            sendsz(pcxs->scfd, pcxs->buf);
        } else {
            sendsz(pcxs->scfd, "-ERR Server lost 2\r\n");
        }
        return;
    }

#ifdef CHATTY
    pcxs->buf[cb] = 0;
    Squirt("<%s", pcxs->buf);
#endif

    if (!Security_GetResponse(&was, pcxs->buf + 2)) {
        Die(TEXT("Cannot build response"));
    }

    /*
     *  Now send the response.
     */
    wsprintf(pcxs->buf, "%s\r\n", was.szBuffer);
    sendsz(pcxs->ssfd, pcxs->buf);
#ifdef CHATTY
    Squirt(">%s", pcxs->buf);
#endif

    Security_ReleaseCredentials(&was);
}

//
// nntp: authinfo user blah
//       authinfo pass blah
//
#endif

/*****************************************************************************
 *
 *  Proxy_Main
 *
 *****************************************************************************/

void
Proxy_Main(void)
{
    HANDLE hThread;
    DWORD dwThid;

    WSADATA wsad;

    /* BUGBUG -- Lazy!  I should check the return code */
    if (WSAStartup(0x0101, &wsad)) return;

    hThread = CreateThread(0, 0, ProxyThread, &g_proxyPop, 0, &dwThid);

    if (hThread) {

     CloseHandle(hThread);
     hThread = CreateThread(0, 0, ProxyThread, &g_proxyNntp1, 0, &dwThid);
     if (hThread) {
       CloseHandle(hThread);
       hThread = CreateThread(0, 0, ProxyThread, &g_proxyNntp2, 0, &dwThid);
       if (hThread) {

        HWND hdlg;
        MSG msg;

        CloseHandle(hThread);

        /*
         *  Create our UI window.
         */
        hdlg = UI_Init();
        while (GetMessage(&msg, 0, 0, 0)) {
            if (IsDialogMessage(hdlg, &msg)) {
            } else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
       } else {
        Squirt("Can't spawn NNTP socket thread 2; bye-bye" EOL);
       }
      } else {
        Squirt("Can't spawn NNTP socket thread 1; bye-bye" EOL);
      }
    } else {
        Squirt("Can't spawn POP3 socket thread; bye-bye" EOL);
    }
}

/*****************************************************************************
 *
 *  Entry
 *
 *****************************************************************************/

void __cdecl
Entry(void)
{
    g_hinst = GetModuleHandle(0);

    if (LoadSecurityManager()) {
        Proxy_Main();
    }

    if (g_hinstSecur) {
        FreeLibrary(g_hinstSecur);
    }

    ExitProcess(0);
}
