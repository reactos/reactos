/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ierrui.cxx

Abstract:

    Contains immplimentation of generic Error UI API.

    Contents:
        InternetErrorDlg
        InternetConfirmZoneCrossing
        (wErrorUIInvalidPassword)

Author:

    Arthur L Bierer (arthurbi) 04-Apr-1996

Revision History:

    04-Apr-1996 arthurbi
        Created

--*/

#include <wininetp.h>
#include <persist.h>
#include "ierrui.hxx"


//
// private prototypes
//



PRIVATE
DWORD
wErrorUIInvalidPassword(
    IN HWND hWnd,
    IN OUT HINTERNET hRequest,
    IN DWORD dwError,
    IN DWORD dwFlags,
    IN OUT LPVOID *lppvData
    );

DWORD
ConfirmCookie(
            IN HWND hwnd,
            IN HTTP_REQUEST_HANDLE_OBJECT *lpRequest,
            IN DWORD dwFlags,
            IN LPVOID *lppvData,
            OUT DWORD *pdwStopWarning
            );


//
// public functions
//

INTERNETAPI
DWORD
WINAPI
InternetErrorDlg(IN HWND hWnd,
                 IN OUT HINTERNET hRequest,
                 IN DWORD dwError,
                 IN DWORD dwFlags,
                 IN OUT LPVOID *lppvData
                 )
/*++

Routine Description:

    Creates an Error Dialog informing the User of a problem with a WinINet HttpSendRequest.
    Can optionally store the results of dialog in the HTTP handle.
    Data may be specified in the lppvData pointer, or optionally InternetErrorDlg
    will gather the information from the HTTP handle

Arguments:

    hWnd -        Window handle to be used as the parent window of a Error Dialog.

    hRequest -    An open HTTP request handle
                  where headers will be added if needed.

    dwError -     The Error code to which the appropriate UI will be shown for.

    dwFlags -     Can be any of all of the following Flags:

                    FLAGS_ERROR_UI_FLAGS_GENERATE_DATA - generates lppvData, and queries
                    internet handle for information.

                    FLAGS_ERROR_UI_FILTER_FOR_ERRORS - scans the returned headers,
                    and determines if there is a "hidden" error, such as an authentication
                    failure.

                    FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS - On successful return of the dialog,
                    this flag will causes the results to be stored in the handle.

                    FLAGS_ERROR_UI_SERIALIZE_DIALOGS - display only one auth dlg and
                    notify subsequent threads when the dialog has been dismissed.

    lppvData -    Contains a Pointer to a Pointer where a stucture is stored containing
                  error specific data, and Dialog results.


Return Value:

    DWORD
    Success - ERROR_SUCCESS

    Failure - One of Serveral Error codes defined in winerror.h or wininet.w

Comments:

    BUGBUG - need to look into multiple authentication scheme handling, and
    how it relates to the UI

--*/

{
    DEBUG_ENTER_API((DBG_HTTP,
                     Dword,
                     "InternetErrorDlg",
                     "%#x, %#x, %s (%d), %#x, %#x",
                     hWnd,
                     hRequest,
                     InternetMapError(dwError),
                     dwError,
                     dwFlags,
                     lppvData
                     ));

    DWORD dwStatusCode = HTTP_STATUS_OK;
    DWORD error = ERROR_SUCCESS;
    HINTERNET hMappedRequestHandle = NULL;
    HINTERNET_HANDLE_TYPE handleType;
    HTTP_REQUEST_HANDLE_OBJECT *lpRequest;

    LPVOID lpVoidData;

    //
    // Check Parameters
    //

    if ( !(dwFlags & FLAGS_ERROR_UI_FLAGS_NO_UI) &&
         !IsWindow(hWnd) )
    {
        error = ERROR_INVALID_HANDLE;
        goto quit;
    }

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }

    if ( hRequest )
    {
        error = MapHandleToAddress(hRequest, (LPVOID *)&hMappedRequestHandle, FALSE);

        if ( error != ERROR_SUCCESS )
            goto quit;

        //
        //  We only support HttpRequestHandles for this form of UI.
        //


        error = RGetHandleType(hMappedRequestHandle, &handleType);

        if ( error != ERROR_SUCCESS || handleType != TypeHttpRequestHandle )
            goto quit;

        lpRequest = (HTTP_REQUEST_HANDLE_OBJECT *) hMappedRequestHandle;

        //
        // See if we can detect an error if needed
        //

        if ( dwFlags & FLAGS_ERROR_UI_FILTER_FOR_ERRORS )
        {
            dwStatusCode = lpRequest->GetStatusCode();

            if ( dwStatusCode == HTTP_STATUS_DENIED || dwStatusCode == HTTP_STATUS_PROXY_AUTH_REQ )
                dwError = ERROR_INTERNET_INCORRECT_PASSWORD;

            if ( lpRequest->GetBlockingFilter())
                dwError = ERROR_INTERNET_NEED_UI;
        }
    }
    else
    {
        if( ERROR_INTERNET_NEED_UI                  == dwError ||
            ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED  == dwError ||
            ERROR_INTERNET_INSERT_CDROM             == dwError ||
            ERROR_INTERNET_INCORRECT_PASSWORD       == dwError )
        {

            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }
    }

    //
    // If the Caller didn't pass us anything, then we create our own
    // structure. Lets NULL it, and then allocate it later
    //
    //

    if ( !lppvData )
    {
        lppvData = &lpVoidData;
        *lppvData = NULL;
    }

    if  (   dwFlags & FLAGS_ERROR_UI_FLAGS_GENERATE_DATA
       && !(dwFlags & FLAGS_ERROR_UI_SERIALIZE_DIALOGS))
    {
        *lppvData = NULL;
    }

    //
    // Determine what Function to handle this based on what error where given
    //

    BOOL fRet;

    switch ( dwError )
    {
        case ERROR_INTERNET_NEED_UI:
            fRet = HttpFiltOnBlockingOps
                (lpRequest, hRequest, hWnd);
            error = fRet? ERROR_SUCCESS : GetLastError();
            break;

        case ERROR_INTERNET_INCORRECT_PASSWORD:
            error = wErrorUIInvalidPassword(hWnd,
                                    hMappedRequestHandle,
                                    dwError,
                                    dwFlags,
                                    lppvData);

            break;

        case ERROR_HTTP_COOKIE_NEEDS_CONFIRMATION:
            // If we are in silent mode, we will have to
            // decline the cookie without throwing up a
            // dialog
            if((dwFlags & FLAGS_ERROR_UI_FLAGS_NO_UI))
            {
                error = ERROR_HTTP_COOKIE_DECLINED;
            }
            else
            {
                DWORD dwStopWarning = 0;
                error = ConfirmCookie(hWnd,
                                       lpRequest,
                                       dwFlags,
                                       lppvData,
                                       &dwStopWarning);
                if( dwStopWarning )
                {
                    // zone mgr to set stop warning
                    INTERNET_COOKIE* pic = (INTERNET_COOKIE*)(*lppvData);
                    ::SetStopWarning(
                        lpRequest->GetURL(), 
                        URLPOLICY_ALLOW,
                        pic->dwFlags & INTERNET_COOKIE_IS_SESSION
                    );
                }
            }

            break;

        case ERROR_INTERNET_FORTEZZA_LOGIN_NEEDED:
            //
            // Call the function which gets the cert context from the
            // Fortezza CSP.

            error = FortezzaLogOn(hWnd);
            break;

        case ERROR_INTERNET_SEC_CERT_REVOKED:
            {
                static WCHAR szMsgBoxText[128]  = L"\0";
                static WCHAR szCaption[64]      = L"\0";
                if (!szCaption[0])
                {
                    LoadStringWrapW(GlobalDllHandle, IDS_CERT_REVOKED, szMsgBoxText, sizeof(szMsgBoxText));
                    LoadStringWrapW(GlobalDllHandle, IDS_SECURITY_CAPTION, szCaption, sizeof(szCaption));
                }
                MessageBoxWrapW(hWnd, szMsgBoxText, szCaption, MB_OK | MB_ICONWARNING);
                error = ERROR_CANCELLED;
            }
            break;

        case ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR:
        case ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR:

            //
            // Read the registry, and check to make sure
            //  the user hasn't overriden this.
            //

            GlobalDataReadWarningUIFlags();

            if ( ! GlobalWarnOnZoneCrossing )
                break;


            //
            // fall through to launch dlg code.
            //

            goto l_launchdlg;

        case ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION:
            //
            // Read the registry, and check to make sure
            //  the user hasn't overriden this.
            //

            GlobalDataReadWarningUIFlags();

            if ( ! GlobalWarnOnPostRedirect )
                break;


            //
            // fall through to launch dlg code.
            //

            goto l_launchdlg;

        case ERROR_INTERNET_POST_IS_NON_SECURE:
        case ERROR_INTERNET_CHG_POST_IS_NON_SECURE:

            //
            // Read the registry, and check to make sure
            //  the user hasn't overriden this.
            //

            GlobalDataReadWarningUIFlags();

            if ( ! GlobalWarnOnPost )
                break;

            //
            // If we're not warning always, ie only warning
            //  if the user actually changed some fields, then
            //  bail out, and don't warn.
            //

            if ( ! GlobalWarnAlways &&
                   dwError == ERROR_INTERNET_POST_IS_NON_SECURE )
                break;

            //
            // fall through to launch dlg code.
            //

            goto l_launchdlg;

        case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
        case ERROR_INTERNET_SEC_CERT_CN_INVALID:
        case ERROR_INTERNET_MIXED_SECURITY:
        case ERROR_INTERNET_INVALID_CA:
        case ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED:
        case ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT:
        case ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT:
        case ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR:
        case ERROR_INTERNET_INSERT_CDROM:
        case ERROR_INTERNET_SEC_CERT_ERRORS:
        case ERROR_INTERNET_SEC_CERT_REV_FAILED:

l_launchdlg:
            // if silent flag is set, we can just pass
            // ERROR_CANCELLED back without throwing up
            // a dialog
            if ((dwFlags & FLAGS_ERROR_UI_FLAGS_NO_UI))
            {
                error = ERROR_CANCELLED;
            }
            else
            {
                ERRORINFODLGTYPE ErrorInfoDlgInfo;
                DLGPROC          pDlgProc;


                MapWininetErrorToDlgId(
                                       dwError,
                                       &ErrorInfoDlgInfo.dwDlgId,
                                       &ErrorInfoDlgInfo.dwDlgFlags,
                                       &pDlgProc
                                       );


                INET_ASSERT(pDlgProc);

                ErrorInfoDlgInfo.hInternetMapped =
                    hMappedRequestHandle;

                if (ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED == dwError)
                {
                    CERT_CONTEXT_ARRAY *pCertContextArray =
                        ((HTTP_REQUEST_HANDLE_OBJECT *)hMappedRequestHandle)->GetCertContextArray();

                    if (pCertContextArray &&
                        pCertContextArray->GetArraySize()==1 &&
                        GlobalAutoSelectSingleCert)
                    {
                        pCertContextArray->SelectCertContext(0);
                        error = ERROR_SUCCESS;
                        break;
                    }
                }
                error = LaunchDlg(
                          hWnd,
                          (LPVOID) &ErrorInfoDlgInfo,
                          ErrorInfoDlgInfo.dwDlgId,
                          pDlgProc
                          );


            }

            break;
        default:
            //
            // if we're filtering for errors, then its not an error
            // since we're intented to process all errors
            //

            if ( ! (dwFlags & FLAGS_ERROR_UI_FILTER_FOR_ERRORS) )
                error = ERROR_NOT_SUPPORTED;
            break;
    }

quit:

    if ( hMappedRequestHandle != NULL )
        DereferenceObject((LPVOID)hMappedRequestHandle);

    DEBUG_LEAVE_API(error);

    return error;
}


INTERNETAPI
DWORD
WINAPI
InternetConfirmZoneCrossingA(
    IN HWND hWnd,
    IN LPSTR szUrlPrev,
    IN LPSTR szUrlNew,
    BOOL bPost
    )
/*++

Routine Description:

    Creates an Error Dialog informing the User of a zone crossing ( going
    between HTTPS to HTTP or HTTPS to HTTP ) when one is detected.

Arguments:

    hWnd -        Window handle to be used as the parent window of a Error Dialog.

    szUrlPrev -   Previous URL string.

    szUrlNew  -   New URL string that the User is about to go to.

    bPost     -   TRUE if a POST is being done, FALSE otherwise.

Return Value:

    DWORD
    Success - ERROR_SUCCESS
               ( user either confirmed OK to continue, or there was no
                 UI needed )

    Failure - ERROR_CANCELLED ( user canceled, we want to stop )
              ERROR_NOT_ENOUGH_MEMORY

Comments:

    none.

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Dword,
                     "InternetConfirmZoneCrossingA",
                     "%#x, %q, %q, %d",
                     hWnd,
                     szUrlPrev,
                     szUrlNew,
                     bPost));

    DWORD error = ERROR_SUCCESS;
    INTERNET_SCHEME ustPrevScheme;
    INTERNET_SCHEME ustNewScheme;

    INET_ASSERT(szUrlNew);


    //
    // If the Previous one is NULL, then we could be starting out
    //  for the first time
    //

    if ( szUrlPrev == NULL )
        goto quit;


    error = CrackUrl(szUrlPrev,
             lstrlen(szUrlPrev),
             FALSE,
             &ustPrevScheme,
             NULL,          //  Scheme Name
             NULL,          //  Scheme Length
             NULL,          //  Host Name
             NULL,          //  Host Length
             NULL,          //  Internet Port
             NULL,          //  UserName
             NULL,          //  UserName Length
             NULL,          //  Password
             NULL,          //  Password Length
             NULL,          //  Path
             NULL,          //  Path Length
             NULL,          //  Extra
             NULL,          //  Extra Length
             NULL
             );

    if ( error != ERROR_SUCCESS )
    {
        error = ERROR_SUCCESS;
        ustPrevScheme = INTERNET_SCHEME_UNKNOWN;
    }


    error = CrackUrl(szUrlNew,
             lstrlen(szUrlNew),
             FALSE,
             &ustNewScheme,
             NULL,          //  Scheme Name
             NULL,          //  Scheme Length
             NULL,          //  Host Name
             NULL,          //  Host Length
             NULL,          //  Internet Port
             NULL,          //  UserName
             NULL,          //  UserName Length
             NULL,          //  Password
             NULL,          //  Password Length
             NULL,          //  Path
             NULL,          //  Path Length
             NULL,          //  Extra
             NULL,          //  Extra Length
             NULL
             );

    if ( error != ERROR_SUCCESS )
    {
        error = ERROR_SUCCESS;
        ustNewScheme = INTERNET_SCHEME_UNKNOWN;
    }

    //
    // First Eliminate the obvious.
    //

    if ( ustPrevScheme == ustNewScheme )
        goto quit;


    //
    // Next, if it wasn't HTTPS, and it is now
    //  we've caught one zone cross.
    //

    if ( ustPrevScheme != INTERNET_SCHEME_HTTPS &&
         ustNewScheme  == INTERNET_SCHEME_HTTPS   )
    {
        error = ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR;
    }


    //
    // Otherwise, if it was HTTPS, and it is now
    //  no longer HTTPS, we also flag it.
    //

    else if ((ustPrevScheme == INTERNET_SCHEME_HTTPS)
             && ((ustNewScheme != INTERNET_SCHEME_HTTPS)
                 && (ustNewScheme != INTERNET_SCHEME_JAVASCRIPT)
                 && (ustNewScheme != INTERNET_SCHEME_VBSCRIPT))) {
        error = ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR;
    }


    if ( error != ERROR_SUCCESS &&
         hWnd )
    {
        error = InternetErrorDlg(
                 hWnd,
                 NULL,  // hRequest
                 error,
                 0,
                 NULL   // non Data structure to pass.
                 );
    }

quit:
    DEBUG_LEAVE_API(error);
    return error;
}


//
// private functions
//


PRIVATE
DWORD
wErrorUIInvalidPassword(IN HWND hWnd,
                        IN HINTERNET hRequest,
                        IN DWORD dwError,
                        IN DWORD dwFlags,
                        IN OUT LPVOID *lppvData
                        )
/*++

Routine Description:

    Creates an Error Dialog asking the User for his Username and Password.

Arguments:

    hWnd -        Window handle to be used as the parent window of a Error Dialog.

    hRequest -    An open HTTP request handle, that is MAPPED,
                  where headers will be added if needed.

    dwError -     The Error code to which the appropriate UI will be shown for.

    dwFlags -     Can be any of all of the following Flags:

                    FLAGS_ERROR_UI_FLAGS_GENERATE_DATA - generates lppvData, and queries
                    internet handle for information.

                    FLAGS_ERROR_UI_FILTER_FOR_ERRORS - scans the returned headers,
                    and determines if there is a "hidden" error, such as an authentication
                    failure.

                    FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS - On successful return of the dialog,
                    this flag will causes the results to be stored in the handle.

                    FLAGS_ERROR_UI_FLAGS_NO_UI - Don't show any User interface. Silently,
                    accepts the Username, and Password that is passed in.

    lppvData -    Contains a Pointer to a Pointer where a stucture is stored containing
                  error specific data, and Dialog results.


Return Value:

    DWORD
    Success - ERROR_SUCCESS

    Failure - One of Serveral Error codes defined in winerror.h or wininet.w

Comments:

    BUGBUG - need to look into multiple authentication scheme handling, and
    how it relates to the UI
    BUGBUG - need to handle case where one thread calls into this function,
    and a second threads also calls in to put up UI on the same User-Pass Info
    as the first.

--*/

{
    DEBUG_ENTER((DBG_INET,
                Dword,
                "wErrorUIInvalidPassword",
                "%#x, %#x, %d (%s), %#x, %#x",
                hWnd,
                hRequest,
                dwError,
                InternetMapError(dwError),
                dwFlags,
                lppvData
                ));

    DWORD error = ERROR_SUCCESS;
    BOOL fMustLock = FALSE;
    
    // Get the request handle and connect handle.
    HTTP_REQUEST_HANDLE_OBJECT *pRequest =
        (HTTP_REQUEST_HANDLE_OBJECT *) hRequest;
    INTERNET_CONNECT_HANDLE_OBJECT *pConnect =
        (INTERNET_CONNECT_HANDLE_OBJECT *) pRequest->GetParent();

    // Validate parameters.
    if (!pConnect || (!(dwFlags & FLAGS_ERROR_UI_FLAGS_NO_UI) && !IsWindow(hWnd)))
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    // Get the authentication context.
    AUTHCTX *pAuthCtx;
    pAuthCtx = pRequest->GetAuthCtx();
    if (!pAuthCtx)
    {
        pAuthCtx = pRequest->GetTunnelAuthCtx();
        if (!pAuthCtx)
        {
            error = ERROR_SUCCESS;
            goto quit;
        }
    }

    PWC *pwc;
    pwc = pAuthCtx->_pPWC;
    INET_ASSERT (pwc && pwc->nLockCount);

    //
    //  Initialize InvalidPassType struct
    //
    char szUserBuf[MAX_FIELD_LENGTH], szPassBuf[MAX_FIELD_LENGTH];
    InvalidPassType ipt;
    ipt.ulMaxField = MAX_FIELD_LENGTH - 1;
    ipt.lpszPassword = szUserBuf;
    ipt.lpszUsername = szPassBuf;
    ipt.lpszHost = pAuthCtx->_pPWC->lpszHost;
    ipt.fIsProxy = pAuthCtx->_fIsProxy;


    //
    // Transfer password cache entry fields to UI structure.
    //
    AuthLock();
    if (pwc->lpszUser)
        lstrcpyn (ipt.lpszUsername, pwc->lpszUser, MAX_FIELD_LENGTH);
    else
        ipt.lpszUsername[0] = 0;
    if (pwc->lpszPass)
        lstrcpyn (ipt.lpszPassword, pwc->lpszPass, MAX_FIELD_LENGTH);
    else
        ipt.lpszPassword[0] = 0;
    ipt.lpszRealm = pwc->lpszRealm;
    AuthUnlock();

    if (dwFlags & FLAGS_ERROR_UI_SERIALIZE_DIALOGS)
    {
        // Queue this thread if we're already in a dialog.
        INET_ASSERT (lppvData);
        INTERNET_AUTH_NOTIFY_DATA *pNotify =
            (INTERNET_AUTH_NOTIFY_DATA *) *lppvData;
        if (IsBadReadPtr (pNotify, sizeof(*pNotify)))
        {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }
        if (AuthInDialog (pAuthCtx, pNotify, &fMustLock))
        {
            // The auth context maintains a context handle
            // to correctly process the 'stale' subheader.
            // Setting _nRetries to 0 will cause no context handle to
            // be passed into the package and restart auth for this context
            // from scratch. Therefore, no attempt will be made to parse the
            // stale header which is the correct behavior.
            if (pAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_DIGEST)
            {
                ((DIGEST_CTX*)pAuthCtx)->_nRetries = 0;
            }
            error = ERROR_INTERNET_DIALOG_PENDING;
            goto quit;
        }
    }

    if ((dwFlags & FLAGS_ERROR_UI_FLAGS_NO_UI))
    {
        // No UI - retrieve any persisted credentials.
        error = InetGetCachedCredentials(pwc->lpszHost, pwc->lpszRealm,
            ipt.lpszUsername, ipt.lpszPassword);
    }
    else // Launch the appropriate dialog.
    {
        if ((pAuthCtx->GetFlags() & PLUGIN_AUTH_FLAGS_CAN_HANDLE_UI))
        {
            // If auth package handles its own UI, let it.
            error = LaunchAuthPlugInDlg
                (pAuthCtx, hWnd, dwError, dwFlags, &ipt);
        }
        else
        {
            if (pAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_BASIC
                || pAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_DIGEST)
            ipt.eAuthType = REALM_AUTH;
            else
            {
                INET_ASSERT(pAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_NTLM 
                    || pAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_KERBEROS
                    || pAuthCtx->GetSchemeType() == AUTHCTX::SCHEME_NEGOTIATE);
                ipt.eAuthType = NTLM_AUTH;
            }

            if (ipt.eAuthType == REALM_AUTH)
                error = LaunchDlg (hWnd,(LPVOID) &ipt, IDD_REALM_AUTH, NULL);
            else
                error = LaunchDlg (hWnd,(LPVOID) &ipt, IDD_NTLM_AUTH, NULL);
        }
    }
        
    // If dialog was OK, return as follows.
    if (error == ERROR_SUCCESS)
    {
        AuthLock();

        // Update user/pass which would overwrite PWC.
        pRequest->SetUserOrPass (ipt.lpszUsername, IS_USER, pAuthCtx->_fIsProxy);
        pRequest->SetUserOrPass (ipt.lpszPassword, IS_PASS, pAuthCtx->_fIsProxy);

        // Update PWC so related requests will be retried.
        pwc->SetUser (ipt.lpszUsername);
        pwc->SetPass (ipt.lpszPassword);

        AuthUnlock();

        // Retry this request.
        error = ERROR_INTERNET_FORCE_RETRY;
    }

    // Notify any waiting threads.

    if (dwFlags & FLAGS_ERROR_UI_SERIALIZE_DIALOGS)
    {
        if (fMustLock)
        {
            // fMustLock will have been set by AuthInDialog if this is
            // a reentrant call to InternetErrorDlg by AuthNotify. If this
            // is the case, acquire the lock and do not call AuthNotify.
            // so that when control is returned to AuthNotify the lock will
            // already be correctly acquired.
            AuthLock();
        }
        else
        {
            AuthNotify (pwc, error);
        }
    }

quit:

    //
    // IE30 compat: must return ERROR_SUCCESS to avoid bogus dialog.
    //
    if ( error == ERROR_CANCELLED )
        error = ERROR_SUCCESS;

    DEBUG_LEAVE(error);
    return error;
}

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

DWORD
ConfirmCookie(
            HWND hwnd,
            HTTP_REQUEST_HANDLE_OBJECT *lpRequest,
            DWORD dwFlags,
            LPVOID *lppvData,
            DWORD  *pdwStopWarning
            )
{
    COOKIE_DLG_INFO cdi = {0};
    DWORD dw;
    BYTE *buf = NULL;

    INET_ASSERT(lppvData);
    INET_ASSERT(pdwStopWarning);

    cdi.pic = (PINTERNET_COOKIE) *lppvData;

    //if there is a cookie in *lppvData then we use it
    if (!cdi.pic || (cdi.pic->cbSize != sizeof(INTERNET_COOKIE)))
    {
        //
        //  BUGBUG TODO we need to get the cookie structure from the lpRequest
#if 0   //  TODO implement cookie retrieval
        //

        buf = (LPBYTE) ALLOCATE_MEMORY(LPTR, MAX_INTERNET_COOKIE_SIZE);

        if (buf)
        {
            //
            //  BUGBUG  HTTP_QUERY_FLAG_STRUCT should be used to indicate we want this as an ic
            //  this is really a superset of SYSTEMTIME & NUMBER - ZekeL 27Aug96
            //

            lpRequest->QueryInfo(HTTP_QUERY_SET_COOKIE | HTTP_QUERY_FLAG_STRUCT, buf,...);

            cdi.pic = (PINTERNET_COOKIE) buf;
            //  or we would have to use call something like SetCookieToInternetCookie()
        }

#endif  //  TODO implement cookie retrieval

        //  assert to show that we need to do this
        dprintf(TEXT("Need to implement cookie retrieval from HTTP_REQUEST handle"));
        INET_ASSERT(FALSE);

    }

    //
    //  Get the server name
    //
    WCHAR wszServer[INTERNET_MAX_HOST_NAME_LENGTH + 1];
    if (lpRequest)
    {
        DEBUG_ONLY(lpRequest->GetHostName(&dw));
        INET_ASSERT(dw <= ARRAYSIZE(wszServer));
        CHAR szServerTmp[INTERNET_MAX_HOST_NAME_LENGTH + 1];
        lpRequest->CopyHostName(szServerTmp);
        if(SHAnsiToUnicode(szServerTmp, wszServer, ARRAYSIZE(wszServer))) 
            cdi.pszServer = wszServer;
    }
    else
    {
        if(SHAnsiToUnicode(cdi.pic->pszDomain, wszServer, ARRAYSIZE(wszServer))) 
            cdi.pszServer = wszServer;
    }
    
    // Can typecast as long as "CookieDialogProc" only returns true or false
    dw = (BOOL ) DialogBoxParamWrapW(GetModuleHandle("WININET.DLL"),
                                MAKEINTRESOURCEW(IDD_CONFIRM_COOKIE),
                                hwnd,
                                (DLGPROC) CookieDialogProc,
                                (LPARAM) &cdi);
    *pdwStopWarning = cdi.dwStopWarning;

    return dw;
}


INTERNETAPI
DWORD
WINAPI
InternetConfirmZoneCrossingW(
    IN HWND hWnd,
    IN LPWSTR szUrlPrev,
    IN LPWSTR szUrlNew,
    BOOL bPost
    )
/*++

Routine Description:

    Creates an Error Dialog informing the User of a zone crossing ( going
    between HTTPS to HTTP or HTTPS to HTTP ) when one is detected.

Arguments:

    hWnd -        Window handle to be used as the parent window of a Error Dialog.

    szUrlPrev -   Previous URL string.

    szUrlNew  -   New URL string that the User is about to go to.

    bPost     -   TRUE if a POST is being done, FALSE otherwise.

Return Value:

    DWORD
    Success - ERROR_SUCCESS
               ( user either confirmed OK to continue, or there was no
                 UI needed )

    Failure - ERROR_CANCELLED ( user canceled, we want to stop )
              ERROR_NOT_ENOUGH_MEMORY

Comments:

    none.

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Dword,
                     "InternetConfirmZoneCrossingW",
                     "%wq, %wq, %d",
                     szUrlPrev,
                     szUrlNew,
                     bPost));

    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpNew, mpPrev;

    if (!(szUrlPrev && szUrlNew))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(szUrlPrev,0,mpPrev);
    ALLOC_MB(szUrlNew,0,mpNew);
    if (!(mpPrev.psStr && mpNew.psStr))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(szUrlPrev,mpPrev);
    UNICODE_TO_ANSI(szUrlNew,mpNew);
    dwErr = InternetConfirmZoneCrossingA(hWnd, mpPrev.psStr, mpNew.psStr, bPost);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(dwErr);
    return dwErr;
}


