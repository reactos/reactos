/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    tcputil.cxx

Abstract:

    Contains functions to connect to an FTP server

    Contents:
        FtpOpenServer
        ResetSocket

Author:

    Heath Hunnicutt (t-hheath) 21-Jun-1994

Environment:

    Win32 user-level DLL

Revision History:

--*/

#include <wininetp.h>
#include "ftpapih.h"

//
// functions
//

DWORD
FtpOpenServer(
    IN LPFTP_SESSION_INFO SessionInfo
    )

/*++

Routine Description:

    Resolves a host name and makes a connection to the FTP server at that host.
    If successful, the controlSocket field of the FTP_SESSION_INFO object will
    contain an opened socket handle

Arguments:

    SessionInfo - pointer to FTP_SESSION_INFO describing host to connect to

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_NAME_NOT_RESOLVED
                  WSA error

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "FtpOpenServer",
                "%#x",
                SessionInfo
                ));

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error;
    BOOL  fUseSocksProxy = FALSE;
    INTERNET_CONNECT_HANDLE_OBJECT *pConnect;
    AUTO_PROXY_ASYNC_MSG *pProxyInfoQuery = NULL;

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    DWORD asyncFlags;

    asyncFlags = 0;
    //asyncFlags = lpThreadInfo->IsAsyncWorkerThread ? SF_NON_BLOCKING : 0;

    //
    // attempt to resolve the host name, server port, and possibly service GUID
    // to socket address(es)
    //

    SessionInfo->socketControl->SetPort(SessionInfo->Port);


    //
    // Using the object handle, check to see if we have a socks proxy.
    //  If so, use it to do our connections.
    //

    INTERNET_HANDLE_OBJECT * pInternet;
    HINTERNET hConnectMapped;

    INET_ASSERT(lpThreadInfo != NULL);
    INET_ASSERT(lpThreadInfo->hObjectMapped != NULL);
    INET_ASSERT(SessionInfo->Host != NULL);

    //
    // Get the Mapped Connect Handle Object
    //

    hConnectMapped = lpThreadInfo->hObjectMapped;

    INET_ASSERT(hConnectMapped);

    //
    // Finally get the Internet Object, so we can query proxy information
    //  out of it.
    //

    pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *) hConnectMapped;


    pInternet = (INTERNET_HANDLE_OBJECT *)
                    ((INTERNET_CONNECT_HANDLE_OBJECT *)hConnectMapped)->GetParent();

    INET_ASSERT(pInternet);

    {
        CHAR szUrl[INTERNET_MAX_URL_LENGTH + sizeof("ftp:// /")];

        PROXY_STATE * pProxyState = NULL;

        INTERNET_SCHEME scheme = INTERNET_SCHEME_DEFAULT;

        if (lstrlen(SessionInfo->Host)>INTERNET_MAX_URL_LENGTH)
        {
            error = ERROR_INSUFFICIENT_BUFFER;
            goto quit;
        }
        wsprintf(szUrl, "ftp://%s/", SessionInfo->Host);

        AUTO_PROXY_ASYNC_MSG proxyInfoQuery(
                                    INTERNET_SCHEME_FTP,
                                    szUrl,
                                    lstrlen(szUrl),
                                    SessionInfo->Host,
                                    lstrlen(SessionInfo->Host),
                                    (INTERNET_PORT) SessionInfo->Port
                                    );

        proxyInfoQuery.SetBlockUntilCompletetion(TRUE);

        pProxyInfoQuery = &proxyInfoQuery;

        error = pInternet->GetProxyInfo(
                                &pProxyInfoQuery
                                );

        if ( error != ERROR_SUCCESS)
        {
            goto quit2;
        }


        if ( pProxyInfoQuery->IsUseProxy() &&
             pProxyInfoQuery->GetProxyScheme() == INTERNET_SCHEME_SOCKS &&
             pProxyInfoQuery->_lpszProxyHostName )
        {

            //
            //  If Socks is enabled, then turn it on.
            //

            error = SessionInfo->socketControl->EnableSocks(
                                                             pProxyInfoQuery->_lpszProxyHostName,
                                                             pProxyInfoQuery->_nProxyHostPort
                                                             );

            if ( error != ERROR_SUCCESS)
            {
                goto quit2;
            }

            error = SessionInfo->socketData->EnableSocks(
                                                             pProxyInfoQuery->_lpszProxyHostName,
                                                             pProxyInfoQuery->_nProxyHostPort
                                                             );


            if ( error != ERROR_SUCCESS)
            {
                goto quit2;
            }

            //
            // Force Passive Mode, since Socks Firewalls may not
            //  support connections from the outside in.
            //

            SessionInfo->Flags |= FFTP_PASSIVE_MODE;

        }

        pConnect->SetServerInfo(SessionInfo->Host,
                                lstrlen(SessionInfo->Host));



        //
        // name was resolved ok, now let's try to connect to the server. Here
        // we create the control socket
        //

        error = SessionInfo->socketControl->Connect(
                              GetTimeoutValue(INTERNET_OPTION_CONNECT_TIMEOUT),
                              GetTimeoutValue(INTERNET_OPTION_CONNECT_RETRIES),
                              SF_INDICATE | asyncFlags
                              );

    quit2:

        if ( pProxyInfoQuery && pProxyInfoQuery->IsAlloced() )
        {
            delete pProxyInfoQuery;
            pProxyInfoQuery = NULL;
        }
    }

quit:


    DEBUG_LEAVE(error);

    return error;
}


BOOL
ResetSocket(
    IN ICSocket * Socket
    )

/*++

Routine Description:

    Sets linger time to zero on a socket, then closes the socket, causing a
    "hard" close

Arguments:

    Socket  - The socket to reset the connection on

Return Value:

    BOOL
        Success - TRUE
        Failure - FALSE

--*/

{
    //
    // ignore return code from linger - if error, socket is closed or aborted
    //

    Socket->SetLinger(TRUE, 0);
    return (Socket->Close() == ERROR_SUCCESS) ? TRUE : FALSE;
}
