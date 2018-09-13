/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    makeconn.cxx

Abstract:

    This file contains the MakeConnection method

    Contents:
        CFsm_MakeConnection::RunSM
        HTTP_REQUEST_HANDLE_OBJECT::MakeConnection_Fsm

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

      29-Apr-97 rfirth
        Conversion to FSM

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include "httpp.h"

//
// HTTP Request Handle Object methods
//


DWORD
CFsm_MakeConnection::RunSM(
    IN CFsm * Fsm
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    Fsm -

Return Value:

    DWORD

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_MakeConnection::RunSM",
                 "%#x",
                 Fsm
                 ));

    START_SENDREQ_PERF();

    CFsm_MakeConnection * stateMachine = (CFsm_MakeConnection *)Fsm;
    HTTP_REQUEST_HANDLE_OBJECT * pRequest;
    DWORD error;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)Fsm->GetContext();

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pRequest->MakeConnection_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    STOP_SENDREQ_PERF();

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::MakeConnection_Fsm(
    IN CFsm_MakeConnection * Fsm
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    Fsm -

Return Value:

    DWORD

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::MakeConnection_Fsm",
                 "%#x",
                 Fsm
                 ));

    PERF_ENTER(MakeConnection_Fsm);

    CFsm_MakeConnection & fsm = *Fsm;
    FSM_STATE state = fsm.GetState();
    DWORD error = fsm.GetError();

    if (state == FSM_STATE_INIT) {
        if (GetAuthState() == AUTHSTATE_NEEDTUNNEL) {
            state = FSM_STATE_1;
        } else if (IsTalkingToSecureServerViaProxy()) {
            state = FSM_STATE_3;
        } else {
            state = FSM_STATE_6;
        }
    } else {
        state = fsm.GetFunctionState();
    }
    switch (state) {
    case FSM_STATE_1:

        //
        // If we're attempting to do NTLM authentication using Proxy tunnelling
        // and we don't have a keep-alive socket to use, then create one
        //

        if (!(IsWantKeepAlive() && (_Socket != NULL) && _Socket->IsOpen())) {
            fsm.SetFunctionState(FSM_STATE_2);
            error = OpenProxyTunnel();
            if ((error != ERROR_SUCCESS)
            || ((GetStatusCode() != HTTP_STATUS_OK) && (GetStatusCode() != 0))) {
                goto quit;
            }
        } else {
            goto quit;
        }

        //
        // fall through
        //

    case FSM_STATE_2:
        if ((error != ERROR_SUCCESS)
        || ((GetStatusCode() != HTTP_STATUS_OK) && (GetStatusCode() != 0))) {
            goto quit;
        }

        //
        // Bind Socket Object with Proper HostName,
        //  so we can check for valid common name
        //  in the handshake.
        //

        if (_Socket->IsSecure()) {
            /* SCLE ref */
            error = ((ICSecureSocket *)_Socket)->SetHostName(GetHostName());
            if (error != ERROR_SUCCESS) {
                goto quit;
            }
        }

        //
        // Undo the proxy-ified info found in this Request Object, make it seem like
        //  we're doing a connect connection, since we're about to do something like it
        //  ( a tunnelled connection through the firewall )
        //

        error = SetServerInfo(INTERNET_SCHEME_HTTP, FALSE);
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        LPSTR urlPath;
        DWORD urlPathLength;

        //
        // get URL-path again if it was changed during tunnel creation
        //

        error = CrackUrl(GetURL(),
                     lstrlen(GetURL()),
                     FALSE, // don't escape URL-path
                     NULL,  // don't care about scheme type
                     NULL,  // or scheme name
                     NULL,  // or scheme name length
                     NULL,  // or host name
                     NULL,  // or host name length
                     NULL,  // or port
                     NULL,  // or user name
                     NULL,  // or user name length
                     NULL,  // or password
                     NULL,  // or password length
                     &urlPath,
                     &urlPathLength,
                     NULL,  // don't care about extra
                     NULL,  // or extra length
                     NULL
                     );
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        LockHeaders();

        ModifyRequest(HTTP_METHOD_TYPE_GET,
                      urlPath,
                      urlPathLength,
                      NULL,
                      0
                      );

        UnlockHeaders();

        //SetProxyNTLMTunnelling(FALSE);
        SetRequestUsingProxy(FALSE);             // don't generate proxy stuff.
        break;

    case FSM_STATE_3:

        //
        // Hack for SSL2 Client Hello bug in IIS Servers.
        //  Need to ReOpen connection after failure with
        //  a Client Hello Message.
        //

        if (_Socket != NULL) {
            ((ICSecureSocket *)_Socket)->SetProviderIndex(0);
        }

attempt_ssl_connect:

        //
        // Attempt to do the connect
        //

        fsm.SetFunctionState(FSM_STATE_4);
        error = OpenProxyTunnel();
        if (error == ERROR_IO_PENDING) {
            goto quit;
        }

        //
        // fall through
        //

    case FSM_STATE_4:
        if ((error != ERROR_SUCCESS) || (GetStatusCode() != HTTP_STATUS_OK)) {
            goto quit;
        }

        //
        // Bind Socket Object with Proper HostName,
        //  so we can check for valid common name
        //  in the handshake.
        //

        INET_ASSERT(_Socket->IsSecure());

        /* SCLE ref */
        error = ((ICSecureSocket *)_Socket)->SetHostName(GetHostName());
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        //
        // if the app wants a secure channel (PCT/SSL) then we must negotiate
        // the security here
        //

        //
        // dwProviderIndex will be managed by SecureHandshakeWithServer,
        // And will be set to 0 when we can't try anymore.
        //

        DWORD asyncFlags;

        //
        // find out if we're async. N.B. see Assumes
        //

        asyncFlags = IsAsyncHandle() ? SF_NON_BLOCKING : 0;

        //
        // If we're Posting or sending data, make sure
        //  the SSL connection knows about it, for the
        //  purposes of generating errors.
        //

        if ((GetMethodType() == HTTP_METHOD_TYPE_POST)
        || (GetMethodType() == HTTP_METHOD_TYPE_PUT)) {
            asyncFlags |= SF_SENDING_DATA;
        }

        fsm.SetFunctionState(FSM_STATE_5);
        error = ((ICSecureSocket *)_Socket)->SecureHandshakeWithServer(
                                                (asyncFlags | SF_ENCRYPT),
                                                &fsm.m_bAttemptReconnect);
        if (error == ERROR_IO_PENDING) {
            goto quit;
        }

        //
        // fall through
        //

    case FSM_STATE_5:
        if (error != ERROR_SUCCESS) {

            if (error == ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED)
            {
                if (_Socket->IsSecure())
                {
                    if(m_pSecurityInfo)
                    {
                        /* SCLE ref */
                        m_pSecurityInfo->Release();
                    }
                    /* SCLE ref */
                    m_pSecurityInfo = ((ICSecureSocket *)_Socket)->GetSecurityEntry();
                }
            }

            _Socket->Disconnect(); // close socket.

            //
            // we disconnected the socket and we won't attempt to reconnect. We
            // need to release the connection to balance the connection limiter
            //

            if (!fsm.m_bAttemptReconnect) {
                ReleaseConnection(TRUE,     // bClose
                                  FALSE,    // bIndicate
                                  TRUE      // bDispose
                                  );
            }
        }

        //
        // SSL2 hack for old IIS servers.
        //  We re-open the socket, and call again.
        //

        if (fsm.m_bAttemptReconnect) {
            goto attempt_ssl_connect;
        }
        break;

    case FSM_STATE_6:
        fsm.SetFunctionState(FSM_STATE_7);
        error = OpenConnection(FALSE);
        if (error == ERROR_IO_PENDING) {
            break;
        }

    case FSM_STATE_7:
//dprintf("HTTP connect took %d msec\n", GetTickCount() - _dwQuerySetCookieHeader);
        //hack
        if (error == ERROR_SUCCESS &&
            _Socket &&
            _Socket->IsSecure() &&
            m_pSecurityInfo == NULL
            )
        {
            /* SCLE ref */
            m_pSecurityInfo = ((ICSecureSocket *)_Socket)->GetSecurityEntry();
        }
        break;

    default:

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        break;
    }

quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
//        PERF_LEAVE(MakeConnection_Fsm);

    }
        PERF_LEAVE(MakeConnection_Fsm);

    DEBUG_LEAVE(error);

    return error;
}
