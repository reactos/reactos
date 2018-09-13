/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    autoprox.cxx

Abstract:

    Contains class implementation for auto-proxy DLLs which can extent WININET's
        abilities (logic) for deciding what proxies to use.

    How auto-proxy works:
        By offloading requests to a specialized Win32 Thread which picks
        up queued up message requests for Queries, Shutdown, and Initialization


    Contents:
        InternetInitializeAutoProxyDll
        AUTO_PROXY_DLLS::CheckForTimerConfigChanges
        AUTO_PROXY_DLLS::DoNestedProxyInfoDownload
        AUTO_PROXY_DLLS::SelectAutoProxyByMime
        AUTO_PROXY_DLLS::SelectAutoProxyByFileExtension
        AUTO_PROXY_DLLS::SelectAutoProxyByDefault
        AUTO_PROXY_DLLS::ReadAutoProxyRegistrySettings
        AUTO_PROXY_DLLS::IsAutoProxyEnabled
        AUTO_PROXY_DLLS::IsAutoProxyGetProxyInfoCallNeeded
        AUTO_PROXY_DLLS::QueueAsyncAutoProxyRequest
        AUTO_PROXY_DLLS::ProcessProxyQueryForInfo
        AUTO_PROXY_DLLS::SafeThreadShutdown
        AUTO_PROXY_DLLS::DoThreadProcessing
        AUTO_PROXY_DLLS::ProcessAsyncAutoProxyRequest
        AUTO_PROXY_DLLS::SignalAsyncRequestCompleted
        AUTO_PROXY_DLLS::FreeAutoProxyInfo

        AUTO_PROXY_LIST_ENTRY::LoadEntry
        AUTO_PROXY_LIST_ENTRY::GetProxyInfoEx
        AUTO_PROXY_LIST_ENTRY::ProxyInfoInvalid
        AUTO_PROXY_LIST_ENTRY::ProxyDllInit
        AUTO_PROXY_LIST_ENTRY::ProxyDllDeInit

        (MatchFileExtensionWithUrl)
        (AutoProxyThreadFunc)

Author:

    Arthur L Bierer (arthurbi) 17-Dec-1996

Revision History:

    17-Dec-1996 arthurbi
        Created

--*/

#include <wininetp.h>
#include "autodial.h"
#include "apdetect.h"

//
// definitions
//

#define MAX_RELOAD_DELAY 45000 // in mins
#define DEFAULT_SCRIPT_BUFFER_SIZE 4000 // bytes.

//
// BUGBUG [arthurbi] This structures are shared between
//  wininet.dll && jsproxy.dll, shouldn't we move them 
//  into a central file???
//

#define AUTO_PROXY_REG_FLAG_ALLOW_STRUC  0x0001

typedef struct {

    //
    // Size of struct
    //

    DWORD dwStructSize;

    //
    // Buffer to Pass
    //

    LPSTR lpszScriptBuffer;

    //
    // Size of buffer above
    //

    DWORD dwScriptBufferSize;

}  AUTO_PROXY_EXTERN_STRUC, *LPAUTO_PROXY_EXTERN_STRUC;


//
// private templates
//

PRIVATE
BOOL
MatchFileExtensionWithUrl(
    IN LPCSTR lpszFileExtensionList,
    IN LPCSTR lpszUrl
    );


//
// private vars
//


//
// functions
//

BOOL
CALLBACK
InternetInitializeAutoProxyDll(
    DWORD dwReserved
    )
/*++

Routine Description:

    Stub to make INETCPL work.  Since they expect to call this function.

Arguments:

    none.

Return Value:

    BOOL
        TRUE    - success

--*/

{

    BOOL fSuccess;
    DWORD error = ERROR_SUCCESS;

    DEBUG_ENTER((DBG_PROXY,
                Bool,
                "InternetInitializeAutoProxyDll",
                ""
                ));

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
    }
    if (error == ERROR_SUCCESS) {
        GlobalProxyInfo.ClearBadProxyList();
        FixProxySettingsForCurrentConnection(TRUE);
        GlobalProxyInfo.RefreshProxySettings(TRUE);        
    }

    fSuccess = (error == ERROR_SUCCESS ) ? TRUE : FALSE;

    DEBUG_LEAVE(fSuccess);

    return fSuccess;

}


DWORD
WINAPI
AutoProxyThreadFunc(
    LPVOID lpAutoProxyObject
    )
/*++

Routine Description:

    Initialization Win32 Proc called when the auto-proxy thread is started.

Arguments:

    none.

Return Value:

    DWORD
        ERROR_SUCCESS    - success

        Win32 Error code - failure

--*/

{
    return GlobalProxyInfo.DoThreadProcessing(lpAutoProxyObject);
}


//
// methods
//

DWORD
AUTO_PROXY_DLLS::CheckForTimerConfigChanges(
    IN DWORD dwMinsToPoll
    )
/*++

Routine Description:

    Exames the registry to see if a timer has been turned so
      that we poll auto-proxy every so often to check for updates

Arguments:

    none.

Return Value:

    DWORD
        ERROR_SUCCESS    - success

        Win32 Error code - failure

--*/

{
    DWORD error = ERROR_SUCCESS;
    DWORD dwReloadMins = dwMinsToPoll;
    BOOL fEnableTimer = FALSE;

    //
    // Now check to determine if we have to enable the auto-proxy e
    //   this is used to force a re-download every "X" minutes.
    //

    if ( dwReloadMins != 0 )
    {
        if ( dwReloadMins >  MAX_RELOAD_DELAY  )  // too big?...
        {
            dwReloadMins = MAX_RELOAD_DELAY;
        }

        dwReloadMins *= ( 1000 * 60 ); // convert mins to miliseconds
        fEnableTimer = TRUE;
    }

    //
    // enable/disable timer for auto-proxy refresh.
    //

    ResetTimerCounter(
        fEnableTimer, // TRUE/FALSE enable/disable
        dwReloadMins  // conv-ed already to msecs
        );

    error = ERROR_SUCCESS;

    return error;
}

DWORD
AUTO_PROXY_DLLS::StartDownloadOfProxyInfo(
    IN BOOL fForceRefresh
    )

/*++

Routine Description:

    Does the Overall download of proxy-information from an internally accessable Web Server.
        Handles the case where we may have to redownload a secondary script also takes
        care of launching detection for proxies.

Arguments:

    fForceRefresh - TRUE if we are to attempt to refresh the auto-proxy URLs, and force a redection

Return Value:

    DWORD
        ERROR_SUCCESS    - success

        Win32 Error code - failure

--*/

{
    DWORD error = ERROR_SUCCESS;
    BOOL fLocked = FALSE;
    DWORD dwcbAutoConfigProxy = 0;
    LPCSTR lpszAutoProxyUrl = NULL;
    INTERNET_PROXY_INFO_EX  ProxySettings;
    BOOL fProxySettingsAlloced = FALSE;
    BOOL fNeedRegWrite = FALSE;
    BOOL fCachedDialupDetection = FALSE;
    BOOL fNeedHostIPChk = FALSE;


    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "AUTO_PROXY_DLLS::StartDownloadOfProxyInfo",
                "%B",
                fForceRefresh
                ));


    if ( IsOffline() ) 
    {
        error = ERROR_INTERNET_OFFLINE;
        goto quit;
    }

    LockAutoProxy();
    fLocked = TRUE;    

    error = GetProxySettings(&ProxySettings);

    if (error != ERROR_SUCCESS ) {
        goto quit;
    }

    fProxySettingsAlloced = TRUE;

    //
    // Now read from the registry, closing any stale stuff that we may still have
    //   around.  Do we still really need this?  Lets only do this for Refresh cases.
    //

    if ( fForceRefresh )
    {
        DestroyAutoProxyDll(TRUE); // unloads DLLs and frees up reg vars.
        SelectAutoProxy(NULL);

        _Error = ReadAutoProxyRegistrySettings();
        error = _Error;

        if (error != ERROR_SUCCESS ||
           !IsConfigValidForAutoProxyThread() )
        {
            SetState(AUTO_PROXY_DISABLED);
            goto quit;
        }

        if ( GetState() == AUTO_PROXY_DISABLED )
        {
            INET_ASSERT(FALSE);  // do we need this code?
            goto quit;
        }
    }

    //INET_ASSERT(GetState() == AUTO_PROXY_BLOCKED ||
    //            GetState() == AUTO_PROXY_PENDING);

    fLocked = FALSE;
    UnlockAutoProxy();

    //
    // Now begin the work, we should no longer need to touch the Proxy_DLL object
    //   settings until we grab the AutoProxy Lock again.
    //

    do
    {
        if ( IsProxyAutoDetectEnabled(&ProxySettings) )
        {
            CHAR szUrl[1024];
            BOOL fRet;
            DWORD dwDetectFlags = PROXY_AUTO_DETECT_TYPE_DNS_A;


            if ( fForceRefresh ||
                 IsProxyAutoDetectNeeded(&ProxySettings) )
            {

                //
                // Release Old Auto-config URL
                //

                if ( ProxySettings.lpszLastKnownGoodAutoConfigUrl) {
                    FREE_MEMORY(ProxySettings.lpszLastKnownGoodAutoConfigUrl);
                    ProxySettings.lpszLastKnownGoodAutoConfigUrl = NULL;
                }

                // only do dhcp on net connections
                if ( ProxySettings.lpszConnectionName == NULL ) {
                    dwDetectFlags |= PROXY_AUTO_DETECT_TYPE_DHCP;
                }

                //
                // Save out the Host IP addresses, before we start the detection,
                //  after the detection is complete, we confirm that we're still
                //  on the same set of Host IPs, in case the user switched connections.
                //

                error = GetHostAddresses(&(ProxySettings.pdwDetectedInterfaceIp),
                                         &(ProxySettings.dwDetectedInterfaceIpCount));

                if ( error != ERROR_SUCCESS) {
                    goto quit;
                }

                fNeedHostIPChk = TRUE; // because we've saved our IPs

                //
                // Do the actual Detection work
                //

                fRet = DetectAutoProxyUrl(
                         szUrl,
                         ARRAY_ELEMENTS(szUrl),
                         dwDetectFlags
                         );

                GetCurrentGmtTime(&ProxySettings.ftLastKnownDetectTime); // mark when detection was run.

                //
                // Process the Results of detection.
                //

                if ( fRet )
                {
                    ProxySettings.dwAutoDiscoveryFlags |= AUTO_PROXY_FLAG_DETECTION_RUN;

                    ProxySettings.lpszLastKnownGoodAutoConfigUrl = NewString(szUrl);
                    if ( ProxySettings.lpszLastKnownGoodAutoConfigUrl == NULL )
                    {
                        error = ERROR_NOT_ENOUGH_MEMORY;
                        goto quit;
                    }

                    SetExpiredUrl(szUrl);
                    fNeedRegWrite = TRUE;
                }
                else
                {
                    //
                    // Disable auto-detection, if we failed
                    //

                    if ( ! (ProxySettings.dwAutoDiscoveryFlags & (AUTO_PROXY_FLAG_DETECTION_RUN | AUTO_PROXY_FLAG_USER_SET)) )
                    {
                        ProxySettings.dwFlags &= ~(PROXY_TYPE_AUTO_DETECT);
                    }

                    ProxySettings.dwAutoDiscoveryFlags |= AUTO_PROXY_FLAG_DETECTION_RUN;
                    fNeedRegWrite = TRUE;
                }
            } 
            else if ( ProxySettings.lpszLastKnownGoodAutoConfigUrl != NULL &&
                      ProxySettings.lpszConnectionName             != NULL )
                      
            {   
                //
                // If we're not doing any detection, and we're on a dial-up adapter,
                //   then we should remember that fact, since we may be called upon
                //   to actually force a detect, in case the cached URL was stale/bad.
                //

                INET_ASSERT(! fForceRefresh );

                fCachedDialupDetection = TRUE;
            }

            lpszAutoProxyUrl = ProxySettings.lpszLastKnownGoodAutoConfigUrl;
        }

        //
        // Falback if we are unable to detect something
        //

        if ( lpszAutoProxyUrl == NULL &&
             IsProxyAutoConfigEnabled(&ProxySettings))
        {
            lpszAutoProxyUrl = ProxySettings.lpszAutoconfigUrl;
        }

        //
        // Do the actual download of the file
        //

        if (lpszAutoProxyUrl != NULL)
        {
            error = DoNestedProxyInfoDownload(lpszAutoProxyUrl, &ProxySettings, fForceRefresh);

            if ( error != ERROR_SUCCESS )
            {

                //
                // If we're cached + on a dialup, and we fail with the URL,
                //  then perhaps the URL/net is really expired.
                //

                if ( fCachedDialupDetection &&
                     ! fForceRefresh )
                {
                    fForceRefresh = TRUE;
                    continue;
                }

                //
                // Fallback to autoconfig if we failed and we were using
                //  auto-detect
                //

                if ( IsProxyAutoConfigEnabled(&ProxySettings) &&                 
                     lpszAutoProxyUrl != ProxySettings.lpszAutoconfigUrl &&
                     ProxySettings.lpszAutoconfigUrl != NULL )
                {
                    lpszAutoProxyUrl = ProxySettings.lpszAutoconfigUrl;
                    error = DoNestedProxyInfoDownload(lpszAutoProxyUrl, &ProxySettings, fForceRefresh);
                }
            }
        }
    } while (FALSE);

    LockAutoProxy();
    fLocked = TRUE;

    if (lpszAutoProxyUrl == NULL)
    {
        SetState(AUTO_PROXY_DISABLED);
        goto quit;
    }
    else
    {
        // stamp version so we know we just ran detection
        _dwUpdatedProxySettingsVersion = 
            ProxySettings.dwCurrentSettingsVersion;
        SetState(AUTO_PROXY_ENABLED);
    }

    CheckForTimerConfigChanges(ProxySettings.dwAutoconfigReloadDelayMins);

quit:

    if ( error != ERROR_SUCCESS )
    {
        if (!fLocked)
        {
            LockAutoProxy();
            fLocked = TRUE;
        }

        SetState(AUTO_PROXY_DISABLED);
    }

    //
    // Unlock the AutoProxy 
    //

    if ( fLocked ) {
        UnlockAutoProxy();
    }

    if ( fProxySettingsAlloced )
    {
        //
        // We need to save results to registry,
        //   then stamp the version in our global we now we detected for this,
        //   and then finally clean up any allocated stuff.
        //

        SaveDetectedProxySettings(&ProxySettings, fNeedHostIPChk);        

        WipeProxySettings(&ProxySettings);
    }

    DEBUG_LEAVE(error);

    return error;
}

DWORD
AUTO_PROXY_DLLS::DoNestedProxyInfoDownload(
    IN LPCSTR lpszAutoProxy,
    IN LPINTERNET_PROXY_INFO_EX lpProxySettings,
    IN BOOL fForceRefresh
    )
/*++

Routine Description:

    Does the download of proxy-information from an internally accessable Web Server.
        Handles the case where we may have to redownload a secondary script and then
        calls on to DownloadProxyInfo to actually do the dirty work.

Arguments:

    lpszAutoProxy - URL to download

    lpProxySettings - active Proxy Settings Copy for auto-proxy thread.

    fForceRefresh - force refresh of downloaded file

Return Value:

    DWORD
        ERROR_SUCCESS    - success

        Win32 Error code - failure

--*/

{
    LPCSTR lpszNestedAutoProxyUrl = NULL;
    DWORD error;
   
    _pTempProxySettings = lpProxySettings; // in case they reset proxy settings    

    if ( _fInAutoProxyThreadShutDown )
    {
        error = ERROR_INTERNET_SHUTDOWN;
        goto quit;

    }

    //
    // IEAK flag, don't retry INS file unless either one of the following:
    //      1) INS URL is expired
    //      2) caching INS run is disabled
    //      3) we're not forcing a refresh
    //

    if ( !(lpProxySettings->dwAutoDiscoveryFlags & AUTO_PROXY_FLAG_CACHE_INIT_RUN ) ||
         // !(ProxySettings.dwAutoDiscoveryFlags & AUTO_PROXY_FLAG_DETECTION_RUN ) ||
         !fForceRefresh ||         
         IsExpiredUrl(lpszAutoProxy))
    {

        //
        // Now do the actual download of the proxy info.
        //

        error =
            DownloadProxyInfo(lpszAutoProxy, fForceRefresh);

        if ( error != ERROR_SUCCESS )
        {
            goto quit;
        }
    }

    //
    // Now check to determine if we have an overriding Auto-Proxy that
    //   supersides the one we just download.  If we do, we will
    //   need to redownload auto-proxy
    //
    lpszNestedAutoProxyUrl = lpProxySettings->lpszAutoconfigSecondaryUrl;

    if ( lpszNestedAutoProxyUrl ) 
    {
        //
        // Success - now restart the download process for this special Url
        //

        error = DownloadProxyInfo(
                    lpszNestedAutoProxyUrl,
                    fForceRefresh
                    );
    }

    // fall through even in error

    error = ERROR_SUCCESS;

quit:

    INET_ASSERT(_pTempProxySettings);
    _pTempProxySettings = NULL;

    return error;
}



DWORD
AUTO_PROXY_DLLS::DownloadProxyInfo(
    IN LPCSTR lpszAutoProxy,
    IN BOOL fForceRefresh
    )

/*++

Routine Description:

    Does the download of proxy-information from an internally accessable Web Server.
        The data will be checked, written to a temp file, and then an associated
        DLL will be called to handle it.

Arguments:

    lpszUrl - The URL to download.

    fForceRefresh - force file to be reloaded from the wire

Return Value:

    DWORD
        ERROR_SUCCESS    - success

        Win32 Error code - failure

--*/

{
    HINTERNET hInternet = NULL;
    HINTERNET hRequest = NULL;
    HANDLE hFile = NULL;
    CHAR szTemporyFile[MAX_PATH+1];
    DWORD dwTempFileSize;

    HANDLE hLockHandle = NULL;

    BOOL fSuccess;
    DWORD error = ERROR_SUCCESS;

    CHAR szBuffer[MAX_PATH+1];
    DWORD dwBytesRead;

    BOOL fCleanupFile = FALSE;
    BOOL fLocked = FALSE;
    BOOL fBuffering = FALSE; 

    LPSTR lpszScriptBuffer = NULL;
    DWORD dwScriptBufferSize;

    DWORD dwStatusCode = ERROR_SUCCESS;
    DWORD cbSize = sizeof(DWORD);

    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "AUTO_PROXY_DLLS::DownloadProxyInfo",
                "%q",
                lpszAutoProxy
                ));

    INET_ASSERT(lpszAutoProxy);

    //
    // Fire up a mini InternetOpen/InternetOpenUrl to download a
    //  config file found on some internal server.
    //

    hInternet = InternetOpen(
                    (_lpszUserAgent) ?
                    _lpszUserAgent :
#ifndef unix
                    "Mozilla/4.0 (compatible; MSIE 5.0; Win32)"
#else
                    "Mozilla/4.0 (compatible; MSIE 5.0; X11)"
#endif /* !unix */
                    ,
                    INTERNET_OPEN_TYPE_DIRECT,
                    NULL,
                    NULL,
                    0
                    );


    if ( !hInternet )
    {
        error = GetLastError();
        INET_ASSERT(error != ERROR_SUCCESS);
            goto quit;
    }


    hRequest = InternetOpenUrl(
                            hInternet,
                            lpszAutoProxy,
                            "Accept: */*\r\n",
                            (DWORD) -1,
                            (fForceRefresh ?
                                INTERNET_FLAG_RELOAD :
                                0) |
                            INTERNET_FLAG_NEED_FILE,
                            INTERNET_NO_CALLBACK
                            );


    if ( !hRequest )
    {
        //error = ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT;
        error = GetLastError();

        if(GlobalDisplayScriptDownloadFailureUI)
        {
            InternetErrorDlg(
                GetDesktopWindow(),
                hRequest,
                ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT,
                FLAGS_ERROR_UI_FILTER_FOR_ERRORS,
                NULL);
        }
        goto quit;
    }


    if (HttpQueryInfo(
            hRequest,
            HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
            (LPVOID) &dwStatusCode,
            &cbSize,
            NULL
            ) && HTTP_STATUS_NOT_FOUND == dwStatusCode)
    {
        error = ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT;
        if(GlobalDisplayScriptDownloadFailureUI)
        {
            InternetErrorDlg(
                GetDesktopWindow(),
                hRequest,
                ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT,
                FLAGS_ERROR_UI_FILTER_FOR_ERRORS,
                NULL);
        }
        goto quit;
    }


    DWORD dwIndex;
    DWORD dwTempSize;

    dwIndex = 0;
    dwTempSize = sizeof(dwScriptBufferSize);

    dwScriptBufferSize = 0;
    
    if ( ! HttpQueryInfo(hRequest, 
                       (HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER),
                       (LPVOID) &dwScriptBufferSize,
                       &dwTempSize,
                       &dwIndex) )
    {
        // failure, just defaults 
        dwScriptBufferSize = DEFAULT_SCRIPT_BUFFER_SIZE;
        fBuffering = TRUE;
    }
    else if ( dwScriptBufferSize > DEFAULT_SCRIPT_BUFFER_SIZE )
    {
        // success, but too big.
        dwScriptBufferSize = DEFAULT_SCRIPT_BUFFER_SIZE;
        fBuffering = FALSE;
    }
    else
    {
        // success, and this one is just right.
        fBuffering = TRUE; 
    }
   
    lpszScriptBuffer = (LPSTR) 
                        ALLOCATE_MEMORY(LMEM_FIXED, ((dwScriptBufferSize+1)
                            * sizeof(CHAR)));

    if ( lpszScriptBuffer == NULL ) 
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    //
    // Suck down bytes, and write to file
    //

    DWORD dwBytes;
    dwBytesRead = 0;

    do
    {
        DWORD dwBytesLeft;
        LPSTR lpszDest;

        dwBytes = 0;
        lpszDest = lpszScriptBuffer;
        dwBytesLeft = dwScriptBufferSize;

        fSuccess = InternetReadFile(
                        hRequest,
                        lpszDest,
                        dwBytesLeft,
                        &dwBytes
                        );

        if ( ! fSuccess )
        {
            error = GetLastError();
            goto quit;
        }

        if ( _fInAutoProxyThreadShutDown )
        {
            error = ERROR_INTERNET_SHUTDOWN;
            goto quit;
        }

        if ( dwBytes > 0 )
        {
            if ( dwBytesRead > 0 ) 
            {
                fBuffering = FALSE;
            }

            dwBytesRead += dwBytes;
        }
    } while ( dwBytes != 0 );

    //
    // Figure out what kind of file we're dealing with.
    //  ONLY allow files with the correct extension or the correct MIME type.
    //

    szBuffer[0] = '\0';
    dwBytes = ARRAY_ELEMENTS(szBuffer)-1;

    fSuccess = HttpQueryInfo( hRequest,
                                 HTTP_QUERY_CONTENT_TYPE,
                                 szBuffer,
                                 &dwBytes,
                                 NULL );

    fLocked = TRUE;
    LockAutoProxy();

    if ( !(fSuccess
            && SelectAutoProxyByMime(szBuffer)) )
    {
        if ( ! SelectAutoProxyByFileExtension(lpszAutoProxy) )
        {
            if ( ! SelectAutoProxyByDefault() )
            {
                //
                // Could not find a registered handler for this data.
                //

                error = ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT;
                goto quit;
            }
        }
    }

    //
    // Now, launch the handler to tell them about the file.
    //  we downloaded it first.
    //

    AUTO_PROXY_LIST_ENTRY * papleAutoProxy;
    AUTO_PROXY_EXTERN_STRUC apeStruct;
    LPAUTO_PROXY_EXTERN_STRUC pExtraStruct;
    

    papleAutoProxy = GetSelectedAutoProxyEntry();
    pExtraStruct = NULL;

    //
    // Get a temp file from cache if we need it.
    //

    if ( fBuffering &&
         (papleAutoProxy->_dwFlags & AUTO_PROXY_REG_FLAG_ALLOW_STRUC) &&
         dwBytesRead > 0 )
    {
        // slam a \0 terminator
        INET_ASSERT(dwBytesRead <= dwScriptBufferSize);
        lpszScriptBuffer[dwBytesRead] = '\0';

        pExtraStruct = &apeStruct;
        apeStruct.dwStructSize = sizeof(AUTO_PROXY_EXTERN_STRUC);
        apeStruct.lpszScriptBuffer = lpszScriptBuffer;
        apeStruct.dwScriptBufferSize = dwBytesRead + 1;
    }
    else 
    {
        fCleanupFile = TRUE;

        //Qfe 3430: When parsing ins file, with reference to pac file, wininet needs to 
        //know the connectoid name in order to set the pac file correctly. Currently there
        //is no way for wininet to pass the connectoid name to branding dll. To workaround
        //this, we use the AUTO_PROXY_EXTERN_STRUC to pass the connectoid name in lpszScriptBuffer
        //variable. 

        pExtraStruct = &apeStruct;
        apeStruct.dwStructSize = sizeof(AUTO_PROXY_EXTERN_STRUC);
        apeStruct.lpszScriptBuffer = GlobalProxyInfo.GetConnectionName();

        if ( ! InternetLockRequestFile(hRequest, &hLockHandle) ) 
        {
            fCleanupFile = FALSE;
        }
        
        dwTempFileSize = MAX_PATH;

        if ( ! InternetQueryOption(hRequest, INTERNET_OPTION_DATAFILE_NAME, 
                    szTemporyFile, &dwTempFileSize ) ||
               dwTempFileSize == 0 )
        {
            error = ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT;
            goto quit;
        }
    }

    //
    // The loading and unloading of DLLs is owned/allowed
    //  only by the auto-proxy thread, ONCE THIS THREAD IS RUNNING
    //  so once we're here, we can be assured its safe to
    //  make the following calls without holding the crit sec.
    //

    INET_ASSERT(fLocked);
    UnlockAutoProxy();
    fLocked = FALSE;
    fSuccess = TRUE;

    //
    // If we're not configured, or cannot match the URL
    //  with a Handler to actually run the script,
    //  then we need to error out
    //

    if ( papleAutoProxy == NULL)
    {
        error = ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT;
        goto quit;
    }

    if ( ! papleAutoProxy->_hAutoConfigDLL )
    {
        if ( _fInAutoProxyThreadShutDown )
        {
            error = ERROR_INTERNET_SHUTDOWN;
            goto quit;
        }

        error = papleAutoProxy->LoadEntry();

        if ( error != ERROR_SUCCESS )
        {
            goto quit;
        }
    }
    else if ( papleAutoProxy->_fInitializedSuccessfully &&
              papleAutoProxy->_pProxyDllInit &&
              papleAutoProxy->_pProxyDllDeInit )
    {
        //
        // If this DLL has already been loaded,
        //   then unitialize it before re-initalizeing with
        //   new data, otherwise we risk leaking some its
        //   objects
        //

        papleAutoProxy->ProxyDllDeInit(
            szBuffer,
            0
            );
        INET_ASSERT(papleAutoProxy->_fUnInited);
    }

    //
    // Make the call into the external DLL,
    //  and let it run, possibly initilization and doing a bunch
    //  of stuff.
    //


    fSuccess = papleAutoProxy->ProxyDllInit (
                            AUTO_PROXY_VERSION,
                            szTemporyFile,  // temp file we down loaded
                            szBuffer,       // mime
                            &_aphAutoProxyAPIs,
                            (DWORD_PTR) pExtraStruct
                            );

    if ( _fInAutoProxyThreadShutDown )
    {
        error = ERROR_INTERNET_SHUTDOWN;
        goto quit;
    }

    if ( ! fSuccess )
    {
        error = ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT;
        goto quit;
    }


    if ( papleAutoProxy->IsGetProxyInfoEx() ||
         papleAutoProxy->IsGetProxyInfo()  )
    {
        SetState(AUTO_PROXY_ENABLED);
    }
    else
    {
        SetState(AUTO_PROXY_DISABLED);
    }

quit:

    if ( error != ERROR_SUCCESS )
    {
        if (!fLocked)
        {
            LockAutoProxy();
            fLocked = TRUE;
        }

        SetState(AUTO_PROXY_DISABLED);
    }

    if ( fLocked )
    {
        UnlockAutoProxy();
    }

    if ( fCleanupFile )
    {
        InternetUnlockRequestFile(hLockHandle);
    }

    if ( lpszScriptBuffer )
    {
        FREE_MEMORY(lpszScriptBuffer);
    }

    if ( hRequest )
    {
        InternetCloseHandle(hRequest);
    }

    if ( hInternet )
    {
        InternetCloseHandle(hInternet);
    }

    SetAbortHandle(NULL);

    DEBUG_LEAVE(error);

    return error;
}



BOOL
AUTO_PROXY_DLLS::IsAutoProxyEnabled(
    VOID
    )

/*++

Routine Description:

    Determines whether the auto-proxy thread is enabled and ready
        to accept async proxy requests.  This is needed to prevent
        senceless calls to the auto-proxy thread when the request
        could made more directly to the PROXY_INFO object.

Return Value:

    BOOL
        TRUE - the AutoProxy thread is accepting async requests

        FALSE - the AutoProxy thread is refusung async requests.

--*/

{
    BOOL fIsGetProxyCallProxyNeeded = FALSE;

    DEBUG_ENTER((DBG_PROXY,
                Bool,
                "AUTO_PROXY_DLLS::IsAutoProxyEnabled",
                ""
                ));


    LockAutoProxy();

    //
    // If we're downloading new information, OR we have a async thread ready to process
    //   autoproxy queries, then go async and do the request.  The worst case will
    //   result in the async thread being created and then returning the request
    //   unprocessed ( the FSM will resubmit it to the general proxy code )
    //

    if ( GetState() == AUTO_PROXY_ENABLED ||
         GetState() == AUTO_PROXY_BLOCKED ||
         GetState() == AUTO_PROXY_PENDING )
    {
        fIsGetProxyCallProxyNeeded = TRUE;
    }
    else
    {
        fIsGetProxyCallProxyNeeded = FALSE;
    }

    UnlockAutoProxy();

    DEBUG_LEAVE(fIsGetProxyCallProxyNeeded);

    return fIsGetProxyCallProxyNeeded;
}


BOOL
AUTO_PROXY_DLLS::IsAutoProxyGetProxyInfoCallNeeded(
    IN AUTO_PROXY_ASYNC_MSG *pQueryForInfo
    )

/*++

Routine Description:

    Wrapper for IsAutoProxyEnabled, verifies that the message object (pQueryForInfo)
        in question is capible of going async.

Arguments:

    pQueryForInfo - Pointer to Message object that contains state information
                        used in the query.

Return Value:

    BOOL
        TRUE - the AutoProxy thread is accepting async requests

        FALSE - the AutoProxy thread is refusung async requests.

--*/

{

    INET_ASSERT(pQueryForInfo);

    //
    // The caller has explicitly banned auto-proxy calls.
    //

    if ( pQueryForInfo->IsAvoidAsyncCall() )
    {
        return FALSE;
    }

    //
    // We cannot process this request to auto-proxy unless we have an URL
    //

    if ( ! pQueryForInfo->IsUrl() )
    {
        return FALSE;
    }

    //
    // Now check to see if its really enabled.
    //

    return IsAutoProxyEnabled();

}

DWORD
AUTO_PROXY_DLLS::QueueAsyncAutoProxyRequest(
    IN OUT AUTO_PROXY_ASYNC_MSG **ppQueryForInfo
    )

/*++

Routine Description:

    Submits the *ppQueryForInfo Object on the async queue for processing
     by the async auto-proxy thread.

Arguments:

    ppQueryForInfo - Pointer to pointer to Message object that contains state information
                        used in the query.   If the object needs to be allocated on the heap
                        then the pointer will change to reflect the new object ptr.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/


{
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    BOOL fLocked = FALSE;
    DWORD error = ERROR_SUCCESS;

    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "AUTO_PROXY_DLLS::QueueAsyncAutoProxyRequest",
                "%x",
                ppQueryForInfo
                ));


    //
    // First allocate an object, if its not allocated,
    //  cause we can't pass local stack vars to another thread (duh..)
    //

    if ( ! (*ppQueryForInfo)->IsAlloced() )
    {
        *ppQueryForInfo = new AUTO_PROXY_ASYNC_MSG(*ppQueryForInfo);

        if ( *ppQueryForInfo == NULL )
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }
    }

    LockAutoProxy();  // lock object to prevent anyone messing with thread handles
    fLocked = TRUE;

    //
    // Busy wait for thread to be shutdown before restarting.
    //

    while ( _fInAutoProxyThreadShutDown )
    {
        UnlockAutoProxy();
        Sleep(100);
        LockAutoProxy();
    }

    if ( _hAutoProxyThread == NULL )
    {

        _hAutoProxyThreadEvent = CreateEvent(
                                    NULL,   // pointer to security attributes
                                    FALSE,  // flag for manual-reset event
                                    TRUE,  // flag for initial state
                                    NULL    // event-object name
                                    );


        if ( _hAutoProxyThreadEvent == NULL )
        {
            error = GetLastError();
            goto quit;
        }

        //
        // We block on this event until the thread is actually running,
        //   otherwise we may PROCESS_DETEACH while the auto-proxy
        //   thread is in this quasi-state of having been created, but not actually run.
        //

        _hAutoProxyStartEvent = CreateEvent(
                                    NULL,   // pointer to security attributes
                                    TRUE,  // flag for manual-reset event
                                    FALSE,  // flag for initial state
                                    NULL    // event-object name
                                    );


        if ( _hAutoProxyStartEvent == NULL )
        {
            error = GetLastError();
            goto quit;
        }

        ResetEvent(_hAutoProxyStartEvent);

        _hAutoProxyThread = CreateThread(
                                NULL, // pointer to thread security attributes
                                0,    // starting stack size
                                AutoProxyThreadFunc,
                                this,  // argument
                                0,     // flags
                                &_dwAutoProxyThreadId
                                );


        if ( _hAutoProxyThread == NULL )
        {
            error = GetLastError();
            CloseHandle(_hAutoProxyThreadEvent);
            CloseHandle(_hAutoProxyStartEvent);
            _hAutoProxyThreadEvent = NULL;
            _hAutoProxyStartEvent = NULL;
            goto quit;
        }

        error = WaitForSingleObject(_hAutoProxyStartEvent,
                             10000
                             );

        if ( error == WAIT_TIMEOUT )
        {
            INET_ASSERT(FALSE);
            TerminateThread(_hAutoProxyThread, ERROR_SUCCESS);

            CloseHandle(_hAutoProxyThreadEvent);
            CloseHandle(_hAutoProxyStartEvent);
            CloseHandle(_hAutoProxyThread);
            _hAutoProxyStartEvent = NULL;
            _hAutoProxyThread = NULL;
            _hAutoProxyThreadEvent = NULL;
            _dwAutoProxyThreadId = NULL;

            error = ERROR_INTERNET_INTERNAL_ERROR;
            goto quit;
        }
        else
        {
            CloseHandle(_hAutoProxyStartEvent);
            _hAutoProxyStartEvent = NULL;
        }

    }

    //
    // Now Block the Thread we're on...
    //

    if ( (*ppQueryForInfo)->IsBlockUntilCompletetion() )
    {
        if ( lpThreadInfo &&
             lpThreadInfo->Fsm &&
             lpThreadInfo->IsAsyncWorkerThread &&
             !(lpThreadInfo->Fsm->IsBlocking()))
        {
            //
            // In a FSM, use FSM - thread pool handler to block this.
            //

            (*ppQueryForInfo)->SetBlockedOnFsm(TRUE);

            lpThreadInfo->Fsm->SetState(FSM_STATE_CONTINUE);
            lpThreadInfo->Fsm->SetNextState(FSM_STATE_CONTINUE);

            error = BlockWorkItem(
                lpThreadInfo->Fsm,
                (DWORD_PTR) *ppQueryForInfo,    // block the FSM on ourselves
                INFINITE                        // we block foreever
                );


            if ( error != ERROR_SUCCESS )
            {
                goto quit;
            }

            InsertAtTailOfSerializedList(&_AsyncQueueList, &(*ppQueryForInfo)->_List);
            SetEvent(_hAutoProxyThreadEvent);

            error = ERROR_IO_PENDING;
        }
        else
        {
            AcquireBlockedRequestQueue();

            InsertAtTailOfSerializedList(&_AsyncQueueList, &(*ppQueryForInfo)->_List);
            SetEvent(_hAutoProxyThreadEvent);

            UnlockAutoProxy();
            fLocked = FALSE;

            error = BlockThreadOnEvent(
                (DWORD_PTR) *ppQueryForInfo,
                INFINITE,   // we need to block forever !
                TRUE        // release BlockedRequestQueue
                );

            INET_ASSERT(error == ERROR_SUCCESS);
        }
    }
    else
    {
        InsertAtTailOfSerializedList(&_AsyncQueueList, &(*ppQueryForInfo)->_List);
        SetEvent(_hAutoProxyThreadEvent);
    }

quit:

    if ( fLocked )
    {
        UnlockAutoProxy();
        fLocked = FALSE;
    }

    DEBUG_LEAVE(error);

    return error;
}

DWORD
AUTO_PROXY_DLLS::ProcessProxyQueryForInfo(
    IN OUT AUTO_PROXY_ASYNC_MSG **ppQueryForInfo
    )

/*++

Routine Description:

    Performs a query for proxy information using an external DLL's entry points to
      anwser our query.  If we are not on the correct thread, we call QueueAsyncAutoProxyRequest
      to marshall our call across to it.

Arguments:

    ppQueryForInfo - Pointer to pointer to Message object that contains state information
                        used in the query.   If the object needs to be allocated on the heap
                        then the pointer will change to reflect the new object ptr.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    AUTO_PROXY_LIST_ENTRY * papleAutoProxy;
    DWORD error = ERROR_SUCCESS;
    BOOL fUnlocked = FALSE;

    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "AUTO_PROXY_DLLS::ProcessProxyQueryForInfo",
                "%x",
                ppQueryForInfo
                ));


    INET_ASSERT(ppQueryForInfo);
    INET_ASSERT(*ppQueryForInfo);
    INET_ASSERT((*ppQueryForInfo)->QueryForInfoMessage() == PROXY_MSG_GET_PROXY_INFO);
    INET_ASSERT(IsOnAsyncAutoProxyThread());

    LockAutoProxy();

    //
    // this function should only ever be executed on the auto-proxy thread
    //

    if ( GetState() == AUTO_PROXY_DISABLED )
    {
        //
        // Fall back to a normal proxy query.
        //

        (*ppQueryForInfo)->SetAvoidAsyncCall(TRUE);
        goto quit;
    }

    papleAutoProxy =
            GetSelectedAutoProxyEntry();

    //
    // We should be the only ones to create or destroy
    //  auto-proxy info, therefore we release the holly
    //  lock of auto-proxy.  If I'm wrong another thread
    //  could screw us ...
    //

    UnlockAutoProxy();
    fUnlocked = TRUE;

    if ( papleAutoProxy )
    {
        //
        // If GetProxyInfoEx is supported, we defer to it to handle
        //  everything.
        //

        if ( papleAutoProxy->IsGetProxyInfoEx() )
        {
            error = papleAutoProxy->GetProxyInfoEx(
                *ppQueryForInfo
                );

            goto quit;
        }
        else if ( papleAutoProxy->IsGetProxyInfo() )
        {
            error = papleAutoProxy->GetProxyInfo(
                *ppQueryForInfo
                );

            goto quit;
        }
        else
        {
            //
            // Fall back to a normal proxy query.
            //

            (*ppQueryForInfo)->SetAvoidAsyncCall(TRUE);
        }
    }
    else
    {
        //
        // Fall back to a normal proxy query.
        //

        (*ppQueryForInfo)->SetAvoidAsyncCall(TRUE);
    }


quit:

    if ( !fUnlocked )
    {
        UnlockAutoProxy();
    }

    DEBUG_LEAVE(error);

    return error;
}

VOID
AUTO_PROXY_DLLS::SafeThreadShutdown(
    BOOL fItsTheFinalShutDown
    )
/*++

Routine Description:

    Performs a shutdown of the auto-proxy thread by the auto-proxy thread itself.

Arguments:

    fItsTheFinalShutDown - TRUE, if we are shutting down at the end of a process

Return Value:

    none.

--*/
{
    DEBUG_ENTER((DBG_PROXY,
                None,
                "AUTO_PROXY_DLLS::SafeThreadShutdown",
                "%B",
                fItsTheFinalShutDown
                ));


    //
    // Attempt to Shut down thread due to no-activity OR
    //  due to process shutdown.
    //

    LockAutoProxy();

    if ( IsSerializedListEmpty(&_AsyncQueueList) ||
         fItsTheFinalShutDown )
    {
        //HANDLE hAutoProxyThreadEvent = _hAutoProxyThreadEvent;
        //HANDLE hAutoProxyThread = _hAutoProxyThread;

        //_hAutoProxyThread = NULL;
        //_hAutoProxyThreadEvent = NULL;
        _dwAutoProxyThreadId = 0;

        //CloseHandle(hAutoProxyThreadEvent);
                //CloseHandle(hAutoProxyThread);

        UnlockAutoProxy();

        if ( fItsTheFinalShutDown )
        {
            //DestroyAutoProxyMsgQueue();
            DestroyAutoProxyDll(TRUE); // unloads DLLs and frees up reg vars
        }

        DEBUG_LEAVE(0);

        ExitThread(ERROR_SUCCESS);

        INET_ASSERT(FALSE);  //we will never get here...
    }

    UnlockAutoProxy();

    DEBUG_LEAVE(0);

}


DWORD
AUTO_PROXY_DLLS::DoThreadProcessing(
    VOID
    )
/*++

Routine Description:

    Main function for the auto-proxy thread, maintains a generic loop
      that dispatchs events/messages sent to our thread for processing

Arguments:

    none.

Return Value:

    none.

--*/
{
    DWORD error  = ERROR_SUCCESS;
    LPINTERNET_THREAD_INFO lpThreadInfo;

    INET_ASSERT(IsOnAsyncAutoProxyThread());
    INET_ASSERT(_hAutoProxyStartEvent);

    SetEvent(_hAutoProxyStartEvent);

    lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo == NULL) {
        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    //
    // Mark ourselves as the Auto-proxy thread...
    //

    _InternetSetAutoProxy(lpThreadInfo);

    while ( TRUE )
    {
        //
        // 1. Check for shut down.
        //

        if ( _fInAutoProxyThreadShutDown )
        {
            SafeThreadShutdown(GlobalDynaUnload);
        }

        //
        // 2. Check for timer causing a refresh of registry settings,
        //      thus causing a redownload of settings
        //

        if ( ChkForAndUpdateTimerCounter() )
        {

            //
            // Finally, start a download of the new stuff.
            //

            error = StartDownloadOfProxyInfo(FALSE /*full refresh*/);
        }

        //
        // 3. Wait for new messages to come in,
        //      for shutdown we should pass right past it
        //


        error = WaitForSingleObject(_hAutoProxyThreadEvent,
                                    _dwWaitTimeOut
                                    );


        //
        // 4. Check to see if we're ready to shut down the thread,
        //    due to a process termination or whatnot.
        //

        if ( _fInAutoProxyThreadShutDown )
        {
            SafeThreadShutdown(TRUE);
        }

        GlobalProxyInfo.CheckForExpiredEntries();

        //
        // 5. If we've idled let us shut down until we're needed again
        //

        //
        // BUGBUG [arthurbi] Theoredically this should work,
        //   and the thread should shutdown on idle, too risky?
        //

        //if ( error == WAIT_FAILED && GetLastError() == WAIT_TIMEOUT)
        //{
            //SafeThreadShutdown(FALSE);
        //}

        //
        // 6. Walk and process the list the messages to our thread asking for
        //     information or reinitalization
        //

        error = ProcessAsyncAutoProxyRequest();

        INET_ASSERT( (error == ERROR_INTERNET_SHUTDOWN) ? _fInAutoProxyThreadShutDown : TRUE );
    }

quit:

    return error;
}

DWORD
AUTO_PROXY_DLLS::ProcessAsyncAutoProxyRequest(
    VOID
    )

/*++

Routine Description:

    Walks the list of queued messages and processes them one by one by
      either rerunning the download/initalization or executing a query for
      proxy information

Arguments:

    none.

Return Value:

    none.

--*/

{

    DWORD error = ERROR_SUCCESS;
    BOOL fForceRefresh;

    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "AUTO_PROXY_DLLS::ProcessAsyncAutoProxyRequest",
                ""
                ));

    while (!IsSerializedListEmpty(&_AsyncQueueList)) {

        //
        // If we're shuting down, then quit right away
        //

        if ( _fInAutoProxyThreadShutDown )
        {
            error = ERROR_INTERNET_SHUTDOWN;
            goto quit;

        }

        LPVOID entry = SlDequeueHead(&_AsyncQueueList);
        AUTO_PROXY_ASYNC_MSG *pQueryForInfo =
            CONTAINING_RECORD(entry, AUTO_PROXY_ASYNC_MSG, _List);


        //
        // If the request has been unblocked, then destroy
        //   the request. ( this is typically due to cancel )
        //

        if ( pQueryForInfo->IsBlockUntilCompletetion() )
        {
            DWORD dwBlockCnt;

            if ( pQueryForInfo->IsBlockedOnFsm() )
            {

                dwBlockCnt = CheckForBlockedWorkItems(
                                1,
                                (DWORD_PTR) pQueryForInfo // blocked on message
                                );

                if ( dwBlockCnt == 0 )
                {
                    delete pQueryForInfo;
                    continue;
                }
            }
        }

        switch ( pQueryForInfo->QueryForInfoMessage() )
        {
            case PROXY_MSG_INIT:

                fForceRefresh = pQueryForInfo->IsForceRefresh();

                //
                // First peak ahead to make sure we're not asked to download
                //  the same darn thing over and over again.
                //

                while (!IsSerializedListEmpty(&_AsyncQueueList))
                {

                    LPVOID entry = SlDequeueHead(&_AsyncQueueList);
                    AUTO_PROXY_ASYNC_MSG *pQueryForInfo =
                        CONTAINING_RECORD(entry, AUTO_PROXY_ASYNC_MSG, _List);

                    if ( pQueryForInfo->QueryForInfoMessage() == PROXY_MSG_INIT )
                    {
                        INET_ASSERT(!pQueryForInfo->IsBlockUntilCompletetion());
                        if ( pQueryForInfo->IsForceRefresh() ) {
                            fForceRefresh = TRUE;
                        }
                        delete pQueryForInfo;
                    }
                    else
                    {
                        InsertAtHeadOfSerializedList(&_AsyncQueueList, &pQueryForInfo->_List);
                        break;
                    }
                }

                //
                // Finally, start a download of the new stuff.
                //

                if ( error == ERROR_SUCCESS )
                {
                    error = StartDownloadOfProxyInfo(fForceRefresh);
                }

                if ( error == ERROR_INTERNET_SHUTDOWN )
                {
                    goto quit;
                }

                break;

            case PROXY_MSG_SELF_DESTRUCT:

                //
                // Destroy ourselves and AUTO_PROXY_DLLS object.
                //
                //

                SafeThreadShutdown(TRUE);

                INET_ASSERT(FALSE); // never returns...

                break;


            case PROXY_MSG_GET_PROXY_INFO:

                //
                // If we've been updated then we need refresh settings from 
                //   the net.
                //

                if ( (_dwUpdatedProxySettingsVersion != _ProxySettings.dwCurrentSettingsVersion) ) 
                {
                    error = StartDownloadOfProxyInfo(FALSE /* no full refresh*/);
                }


                //
                // The strait call into GetProxyInfo
                //

                error = ProcessProxyQueryForInfo(
                    &pQueryForInfo
                    );

                if ( error == ERROR_INTERNET_SHUTDOWN )
                {
                    goto quit;
                }

                break;


            case PROXY_MSG_SET_BAD_PROXY:
            case PROXY_MSG_DEINIT:
            case PROXY_MSG_INVALID:
            default:

                INET_ASSERT(FALSE);
                break;
        }

        //
        // Wake up the caller if they are blocking on this completeion
        //

        if ( pQueryForInfo->IsBlockUntilCompletetion() )
        {
            SignalAsyncRequestCompleted(pQueryForInfo); // the thread that gets woken up now owns the object
        }
        else
        {
            delete pQueryForInfo;
        }
    }

quit:

    DEBUG_LEAVE(error);

    return error;
}

DWORD
AUTO_PROXY_DLLS::SignalAsyncRequestCompleted(
    IN AUTO_PROXY_ASYNC_MSG *pQueryForInfo
    )
/*++

Routine Description:

    Notifies a blocked thread or FSM that we have completed its message based request and
      it now can continue.

Arguments:

    pQueryForInfo - the orginating request

Return Value:

    none.

--*/

{
    DWORD dwCntUnBlocked;

    INET_ASSERT ( pQueryForInfo );

    if ( pQueryForInfo->IsBlockedOnFsm() )
    {
        dwCntUnBlocked = UnblockWorkItems(
                            1,
                            (DWORD_PTR) pQueryForInfo, // blocked on message
                            ERROR_SUCCESS,
                            TP_NO_PRIORITY_CHANGE
                            );

        //
        // If we were unable to unblock it, then we need to free it.
        //

        if ( dwCntUnBlocked == 0 )
        {
            delete pQueryForInfo;
            pQueryForInfo = NULL;
        }

    }
    else
    {

        dwCntUnBlocked = SignalThreadOnEvent(
                            (DWORD_PTR) pQueryForInfo,  // blocked on message
                            1,                          // should only be one item thats blocked by us
                            ERROR_SUCCESS
                            );

        //if ( dwCntUnBlocked == 0 )
        //{
        //    delete pQueryForInfo;
        //    pQueryForInfo = NULL;
        //}

    }

    return ERROR_SUCCESS;
}

VOID
AUTO_PROXY_DLLS::WipeProxySettings(
    LPINTERNET_PROXY_INFO_EX lpProxySettings
    )

/*++

Routine Description:

    Frees proxy settings

Arguments:

    lpProxySettings - pointer to listing of proxy settings

Return Value:

    none.

--*/

{
    if ( lpProxySettings )
    {
        if ( lpProxySettings->lpszConnectionName ) {
            FREE_MEMORY(lpProxySettings->lpszConnectionName);
        }

        if ( lpProxySettings->lpszProxy ) {
            FREE_MEMORY(lpProxySettings->lpszProxy);
        }

        if ( lpProxySettings->lpszProxyBypass ) {
            FREE_MEMORY(lpProxySettings->lpszProxyBypass);
        }

        if ( lpProxySettings->lpszLastKnownGoodAutoConfigUrl ) {
            FREE_MEMORY(lpProxySettings->lpszLastKnownGoodAutoConfigUrl);
        }

        if (lpProxySettings->lpszAutoconfigUrl) {
            FREE_MEMORY(lpProxySettings->lpszAutoconfigUrl);
        }

        if (lpProxySettings->lpszAutoconfigSecondaryUrl) {
            FREE_MEMORY(lpProxySettings->lpszAutoconfigSecondaryUrl);
        }

        if (lpProxySettings->pdwDetectedInterfaceIp) {
            FREE_MEMORY(lpProxySettings->pdwDetectedInterfaceIp);
        }
    }
}


DWORD
AUTO_PROXY_DLLS::SetProxySettings(
    IN LPINTERNET_PROXY_INFO_EX  lpProxySettings,
    IN BOOL fModifiedInProcess,
    IN BOOL fAllowOverwrite
    )

/*++

Routine Description:

    Write Auto-proxy Settings on the current auto-proxy object,
        this writes the settings into the object, but does not
        refresh it.

Arguments:

    lpProxySettings - List of new settings that we'd like to write

    fAllowOverwrite - Allow update of the settings in the object even if the version counter
                    hasn't changed

Return Value:

    DWORD
        ERROR_SUCCESS - if we have a valid proxy config

        other-errors - if we were not able to make the settings

--*/


{
    DWORD error = ERROR_SUCCESS;
    BOOL fGoPending = FALSE;

    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "AUTO_PROXY_DLLS::SetProxySettings",
                "%x %B, %B",
                lpProxySettings,
                fModifiedInProcess,
                fAllowOverwrite
                ));

    LockAutoProxy();

    if ( fAllowOverwrite ||
         lpProxySettings->dwCurrentSettingsVersion == _ProxySettings.dwCurrentSettingsVersion )
    {
        BOOL fConnectionNameChange = FALSE;

        _fModifiedInProcess = fModifiedInProcess;

        UPDATE_GLOBAL_PROXY_VERSION();

        //
        // Check to see if we're changing the connection name,
        //  if so than this means we should plan on resetting
        //  the version below, so that we can redetect the new 
        //  connection.
        //

        if ( ! IsConnectionMatch( 
                _ProxySettings.lpszConnectionName,
                lpProxySettings->lpszConnectionName))
        {
            fConnectionNameChange = TRUE;
        }

        WipeProxySettings();

        _ProxySettings = *lpProxySettings;

        if ( fModifiedInProcess ) {
            // update version, only if we don't plan to write out these settings
            _ProxySettings.dwCurrentSettingsVersion++;
        }

        //
        // minus -1 so that when we get to the auto-proxy thread,
        //  we can reload/detect or do what's needed to keep settings current
        //

        if ( _dwUpdatedProxySettingsVersion != _ProxySettings.dwCurrentSettingsVersion ||             
             _dwUpdatedProxySettingsVersion == 0 /* init state */ ||
             fConnectionNameChange /* connection switch */ ) 
        {
            _dwUpdatedProxySettingsVersion = (DWORD) (_ProxySettings.dwCurrentSettingsVersion - 1);
            fGoPending = TRUE;
        }

        //
        // we shouldn't care about these settings,
        //   but we need to copy them anyway in
        //   case we save the stuff to the registry store
        //

        _ProxySettings.lpszProxy =
            lpProxySettings->lpszProxy ?
            NewString(lpProxySettings->lpszProxy) :
            NULL;

        _ProxySettings.lpszProxyBypass =
            lpProxySettings->lpszProxyBypass ?
            NewString(lpProxySettings->lpszProxyBypass) :
            NULL;

        _ProxySettings.lpszConnectionName =
            lpProxySettings->lpszConnectionName ?
            NewString(lpProxySettings->lpszConnectionName) :
            NULL;


        //
        // Copy strings, cause we may be on another thread
        //

        _ProxySettings.lpszAutoconfigUrl =
            lpProxySettings->lpszAutoconfigUrl ?
            NewString(lpProxySettings->lpszAutoconfigUrl) :
            NULL;

        _ProxySettings.lpszAutoconfigSecondaryUrl = 
            lpProxySettings->lpszAutoconfigSecondaryUrl ?
            NewString(lpProxySettings->lpszAutoconfigSecondaryUrl) :
            NULL;

        _ProxySettings.lpszLastKnownGoodAutoConfigUrl =
            lpProxySettings->lpszLastKnownGoodAutoConfigUrl ?
            NewString(lpProxySettings->lpszLastKnownGoodAutoConfigUrl) :
            NULL;

        //
        // Copy of IP host addresses from last detection
        //

        if ( lpProxySettings->dwDetectedInterfaceIpCount > 0 &&
             lpProxySettings->pdwDetectedInterfaceIp != NULL )
        {
            _ProxySettings.pdwDetectedInterfaceIp = (LPDWORD)
                ALLOCATE_MEMORY(LMEM_FIXED, lpProxySettings->dwDetectedInterfaceIpCount
                                * sizeof(DWORD));

            if (_ProxySettings.pdwDetectedInterfaceIp == NULL )
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }

            memcpy(_ProxySettings.pdwDetectedInterfaceIp, lpProxySettings->pdwDetectedInterfaceIp,
                        lpProxySettings->dwDetectedInterfaceIpCount * sizeof(DWORD));
        }


        if (fGoPending &&
            IsConfigValidForAutoProxyThread())            
        {
            //
            // Enable a forced refresh if the user orders it through the UI,
            //   for unknown new connections we don't block the user on auto-detect,
            //   since he didn't really order it
            //

            if ( IsStaticFallbackEnabled() ) {
                SetState(AUTO_PROXY_PENDING);
            } else {
                SetState(AUTO_PROXY_BLOCKED);
            }
        }
    }

quit:

    UnlockAutoProxy();

    DEBUG_LEAVE(error);

    return error;
}


DWORD
AUTO_PROXY_DLLS::GetProxySettings(
    OUT LPINTERNET_PROXY_INFO_EX  lpProxySettings,
    IN BOOL fCheckVersion
    )

/*++

Routine Description:

    Reads Auto-proxy Settings off the current auto-proxy object,
        this allocates individual fields as needed to store the result

Arguments:

    lpProxySettings - Returns the result of the auto-proxy settings

Return Value:

    DWORD
        ERROR_SUCCESS - if we have a valid proxy config

        other-errors - if we were not able to make the settings

--*/

{
    DWORD error = ERROR_SUCCESS;

    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "AUTO_PROXY_DLLS::GetProxySettings",
                "%x %B",
                lpProxySettings
                ));

    LockAutoProxy();

    if ( fCheckVersion &&
         lpProxySettings->dwCurrentSettingsVersion == _ProxySettings.dwCurrentSettingsVersion )
    {
        goto quit; // no change
    }


    *lpProxySettings = _ProxySettings;

    //
    // Copy strings, cause we may be on another thread
    //

    lpProxySettings->lpszProxy =
        _ProxySettings.lpszProxy ?
        NewString(_ProxySettings.lpszProxy) :
        NULL;

    lpProxySettings->lpszProxyBypass =
        _ProxySettings.lpszProxyBypass ?
        NewString(_ProxySettings.lpszProxyBypass) :
        NULL;

    lpProxySettings->lpszConnectionName =
        _ProxySettings.lpszConnectionName ?
        NewString(_ProxySettings.lpszConnectionName) :
        NULL;

    lpProxySettings->lpszAutoconfigUrl =
        _ProxySettings.lpszAutoconfigUrl ?
        NewString(_ProxySettings.lpszAutoconfigUrl) :
        NULL;

    lpProxySettings->lpszAutoconfigSecondaryUrl = 
        _ProxySettings.lpszAutoconfigSecondaryUrl ?
        NewString(_ProxySettings.lpszAutoconfigSecondaryUrl) :
        NULL;

    lpProxySettings->lpszLastKnownGoodAutoConfigUrl =
        _ProxySettings.lpszLastKnownGoodAutoConfigUrl ?
        NewString(_ProxySettings.lpszLastKnownGoodAutoConfigUrl) :
        NULL;

    //
    // Copy of IP host addresses from last detection
    //

    if ( _ProxySettings.dwDetectedInterfaceIpCount > 0 &&
         _ProxySettings.pdwDetectedInterfaceIp != NULL )
    {
        lpProxySettings->pdwDetectedInterfaceIp = (LPDWORD)
            ALLOCATE_MEMORY(LMEM_FIXED, _ProxySettings.dwDetectedInterfaceIpCount
                            * sizeof(DWORD));

        if (lpProxySettings->pdwDetectedInterfaceIp == NULL )
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }

        memcpy(lpProxySettings->pdwDetectedInterfaceIp, _ProxySettings.pdwDetectedInterfaceIp,
                    _ProxySettings.dwDetectedInterfaceIpCount * sizeof(DWORD));
    }

quit:

    UnlockAutoProxy();

    DEBUG_LEAVE(error);

    return error;
}


DWORD
AUTO_PROXY_DLLS::RefreshProxySettings(
    IN BOOL fForceRefresh
    )

/*++

Routine Description:

    Syncronizes the Auto-Proxy engine with the state of the various settings,
      and updates the various state keepers of the results.

        - If there is no proxy settings, or auto-proxy is not needed or not detected,
          we disable the auto-proxy system.

        - If we have auto-proxy information or need to detect for some, then we fire up
            the auto-proxy thread and send it a message to initalize itself.

Arguments:

    none.

Return Value:

    DWORD
        ERROR_SUCCESS    - success

        Win32 Error code - failure

--*/


{
    DWORD error = ERROR_SUCCESS;

    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "AUTO_PROXY_DLLS::RefreshProxySettings",
                "%B",
                fForceRefresh
                ));

    LockAutoProxy();

    if (! IsOnAsyncAutoProxyThread() )
    {
        AUTO_PROXY_ASYNC_MSG *pQueryForInfo;

        error = _Error;
        if ( error != ERROR_SUCCESS)
        {            
            goto quit; // obj is not initalized properly
        }

        if (!IsConfigValidForAutoProxyThread() )
        {
            goto quit; // disable & bail, we're not setup for this
        }

        pQueryForInfo = new AUTO_PROXY_ASYNC_MSG(PROXY_MSG_INIT);

        if (pQueryForInfo == NULL )
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }

        //
        // Enabled a forced refresh if the user orders it through the UI,
        //   for unknown new connections we don't block the user on auto-detect,
        //   since he didn't really order it
        //

        pQueryForInfo->SetForceRefresh(fForceRefresh);

        if ( !fForceRefresh && IsStaticFallbackEnabled() ) {
            SetState(AUTO_PROXY_PENDING);
        } else {
            SetState(AUTO_PROXY_BLOCKED);
        }

        error = QueueAsyncAutoProxyRequest(
                &pQueryForInfo  // don't worry, this request won't block us.
                );
    }

quit:

    if ( error != ERROR_SUCCESS  && 
         error != ERROR_IO_PENDING )
    {
        SetState(AUTO_PROXY_DISABLED); // disable in case something critical happens
    }

    UnlockAutoProxy();

    DEBUG_LEAVE(error);

    return error;
}

DWORD
AUTO_PROXY_DLLS::StartBackroundDetectionIfNeeded(
    VOID
    )
{
    DWORD error = ERROR_SUCCESS;

    LockAutoProxy();
    if ( _hAutoProxyThread == NULL ) {
        error = RefreshProxySettings(FALSE);
    }
    UnlockAutoProxy();

    return error;
}

DWORD
AUTO_PROXY_DLLS::QueryProxySettings(
    IN OUT AUTO_PROXY_ASYNC_MSG **ppQueryForInfo
    )

/*++

Routine Description:

    Performs a query for proxy information using auto-proxy to anwser our query.

    Assumes this is not called from auto-proxy thread/

Arguments:

    ppQueryForInfo - Pointer to pointer to Message object that contains state information
                        used in the query.   If the object needs to be allocated on the heap
                        then the pointer will change to reflect the new object ptr.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    AUTO_PROXY_LIST_ENTRY * papleAutoProxy;
    DWORD error = ERROR_SUCCESS;
    BOOL fUnlocked = FALSE;

    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "AUTO_PROXY_DLLS::QueryProxySettings",
                "%x",
                ppQueryForInfo
                ));


    INET_ASSERT(ppQueryForInfo);
    INET_ASSERT(*ppQueryForInfo);
    INET_ASSERT((*ppQueryForInfo)->QueryForInfoMessage() == PROXY_MSG_GET_PROXY_INFO);
    //INET_ASSERT(!IsOnAsyncAutoProxyThread());

    LockAutoProxy();

    //
    // ALWAYS force this function to exec it auto-proxy calls on the
    //   async auto-proxy thread.
    //

    if ( IsAutoProxy() &&
        !IsOnAsyncAutoProxyThread() &&
         IsAutoProxyGetProxyInfoCallNeeded(*ppQueryForInfo))
    {
        if ( GetState() == AUTO_PROXY_PENDING )
        {

            //
            // If we're doing a pending detection, then
            //  fallback to standby settings, and if we fail
            //  with standby settings, we should get re-called to here
            //  and if we're then still detecting, then
            //  we'll block on the detection result
            //

            if ( ! (*ppQueryForInfo)->IsBackroundDetectionPending() )
            {
                error = StartBackroundDetectionIfNeeded();
                if (error != ERROR_SUCCESS ) {
                    goto quit;
                }
                (*ppQueryForInfo)->SetBackroundDetectionPending(TRUE);
                if ( ! (*ppQueryForInfo)->IsAlloced() )
                {
                    *ppQueryForInfo = new AUTO_PROXY_ASYNC_MSG(*ppQueryForInfo);

                    if ( *ppQueryForInfo == NULL ) {
                        error = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
                goto quit;
            }
        }
        else if (!(_ProxySettings.dwAutoDiscoveryFlags & AUTO_PROXY_FLAG_DONT_CACHE_PROXY_RESULT) &&
                   GlobalAutoProxyCacheEnable)
        {
            (*ppQueryForInfo)->SetCanCacheResult(TRUE);
        }

        //
        // If we need to show indication during detection,
        //   then do so now before we block
        //

        if ( (*ppQueryForInfo)->IsShowIndication() &&
                (GetState() == AUTO_PROXY_PENDING ||
                 GetState() == AUTO_PROXY_BLOCKED))
        {
            UnlockAutoProxy();
            fUnlocked = TRUE;

            InternetIndicateStatus(INTERNET_STATUS_DETECTING_PROXY, NULL, 0);
        }
        else
        {
            UnlockAutoProxy();
            fUnlocked = TRUE;
        }

        // always disable unless we're ready to renter on a failure
        (*ppQueryForInfo)->SetBackroundDetectionPending(FALSE);

        error = QueueAsyncAutoProxyRequest(ppQueryForInfo);
        goto quit;
    }

    // always disabled it unless we're ready to reenter on a failure
    (*ppQueryForInfo)->SetBackroundDetectionPending(FALSE);

quit:

    if ( !fUnlocked )
    {
        UnlockAutoProxy();
    }

    DEBUG_LEAVE(error);

    return error;
}

BOOL
AUTO_PROXY_DLLS::IsConfigValidForAutoProxyThread(
    VOID
    )
/*++

Routine Description:

    Decide if we need the auto-thread to either download auto-proxy ot
      go off and detect for auto-proxy on the network

Arguments:

    none.

Return Value:

    BOOL
        TRUE - if we have a valid proxy config

        FALSE - if we not valid

--*/

{
    LPINTERNET_PROXY_INFO_EX lpProxySettings = &_ProxySettings;

    if ( (lpProxySettings->dwFlags & PROXY_TYPE_AUTO_PROXY_URL) &&
          lpProxySettings->lpszAutoconfigUrl != NULL &&
         *lpProxySettings->lpszAutoconfigUrl != '\0' )
    {
        return TRUE; // old behavior for auto-proxy URL config
    }

    if ( IsProxyAutoDetectEnabled(lpProxySettings))
    {
        return TRUE;
    }

    return FALSE; // do nothing
}

BOOL
AUTO_PROXY_DLLS::IsStaticFallbackEnabled(
    VOID
    )
/*++

Routine Description:

    Decide if we need to block on the auto-proxy information,
     or if we can fallback to static settings when auto-proxy is initalizing/detecting.

Arguments:

    none.

Return Value:

    BOOL
        TRUE - if we keep going with static settings

        FALSE - we'll need to block requests until auto-proxy is intialized

--*/

{

    if ( (_ProxySettings.dwFlags & PROXY_TYPE_AUTO_PROXY_URL) &&
          _ProxySettings.lpszAutoconfigUrl != NULL &&
         *_ProxySettings.lpszAutoconfigUrl != '\0' )
    {
        return FALSE; // block, don't bypass, this is old behavior for Auto-proxy URLs
    }

    if (  IsProxyAutoDetectEnabled() )

    {
        if ( !(_ProxySettings.dwAutoDiscoveryFlags & (AUTO_PROXY_FLAG_DETECTION_RUN | AUTO_PROXY_FLAG_USER_SET)) ) {
            return TRUE; // detection SHOULD NOT BLOCK THE FIRST TIME, in case it doesn't work
        }

        if ( _ProxySettings.lpszConnectionName != NULL )
        {
            if ( _ProxySettings.lpszLastKnownGoodAutoConfigUrl == NULL ) 
            {
                return TRUE; // detection SHOULD NOT BE DEPENDED upon with Dialup, unless it has something
            }

            if ( _ProxySettings.dwAutoDiscoveryFlags & AUTO_PROXY_FLAG_DETECTION_SUSPECT)
            {
                return TRUE; // detection should not block when we're in a hosed state.
            }
        }
    }



    return FALSE; // block
}

VOID
AUTO_PROXY_DLLS::SetExpiredUrl(
    LPCSTR lpszUrl
    )
/*++

Routine Description:

    Sets default expiry time on the if none is specified on the cached URL. 

Arguments:

    lpszUrl -

Return Value:

    DWORD
        ERROR_SUCCESS    - success

        Win32 Error code - failure

--*/
{
    CACHE_ENTRY_INFOEX Cei;
    DWORD dwCeiSize = sizeof(INTERNET_CACHE_ENTRY_INFOA);
    BOOL fRet;

    fRet = GetUrlCacheEntryInfoExA(
            lpszUrl,
            (INTERNET_CACHE_ENTRY_INFOA *) &Cei,
            &dwCeiSize,
            NULL,
            NULL,
            NULL,
            INTERNET_CACHE_FLAG_GET_STRUCT_ONLY
            );


    if ( fRet && 
         (FT2LL(Cei.ExpireTime) == LONGLONG_ZERO))
    {
        //
        // Set default expiry as: current time + some default expiry
        //

        GetCurrentGmtTime(&Cei.ExpireTime);

        *(LONGLONG *) &(Cei.ExpireTime) += (12 * (24 * ONE_HOUR_DELTA));

        //
        // Re-Save Cache entry with updated default expiry time for PAC/INS file.
        //

        SetUrlCacheEntryInfoA(
                lpszUrl,
                (INTERNET_CACHE_ENTRY_INFOA *) &Cei,
                CACHE_ENTRY_EXPTIME_FC
                );

    }

}

BOOL
AUTO_PROXY_DLLS::IsExpiredUrl(
    LPCSTR lpszUrl
    )
/*++

Routine Description:

    Is Url Expired?  If it isn't don't force a reload/update of data

Arguments:

    lpszUrl -

Return Value:

    DWORD
        ERROR_SUCCESS    - success

        Win32 Error code - failure

--*/
{
    CACHE_ENTRY_INFOEX Cei;
    DWORD dwCeiSize = sizeof(INTERNET_CACHE_ENTRY_INFOA);
    BOOL fRet;

    fRet = GetUrlCacheEntryInfoExA(
            lpszUrl,
            (INTERNET_CACHE_ENTRY_INFOA *) &Cei,
            &dwCeiSize,
            NULL,
            NULL,
            NULL,
            INTERNET_CACHE_FLAG_GET_STRUCT_ONLY 
            );

    if (!fRet) {
        return TRUE; // expired, not in the cache
    }

    if ( IsExpired(&Cei, 0, &fRet) )
    {
        return TRUE; // expired, like really it is
    }

    return FALSE; // not expired
}


BOOL
AUTO_PROXY_DLLS::IsProxyAutoDetectNeeded(
    LPINTERNET_PROXY_INFO_EX lpProxySettings
    )

/*++

Routine Description:

  Detects whether we need to actually run a detection on the network,
    or whether we can resuse current results from previous runs

Arguments:

    lpProxySettings - structure to fill

Return Value:

    DWORD
        ERROR_SUCCESS    - success

        Win32 Error code - failure

--*/

{
    DWORD addressCount;
    LPDWORD * addressList;
    LPHOSTENT lpHostent;
    BOOL fSuspectBadDetect = FALSE;

    INET_ASSERT(IsProxyAutoDetectEnabled(lpProxySettings));

    // we haven't detected before on this connection, so we need to do it.
    if ( !(lpProxySettings->dwAutoDiscoveryFlags & AUTO_PROXY_FLAG_DETECTION_RUN) ) {
        return TRUE; // detect needed
    }

    // if we're flagged to ALWAYS force detection, then do this
    if (lpProxySettings->dwAutoDiscoveryFlags & AUTO_PROXY_FLAG_ALWAYS_DETECT) {
        return TRUE; // detect needed
    }

    //
    // Check for an expired detected Url, detect it its expired
    //  Since this is RAS we can't rely on the host IP staying 
    //  the same everytime.
    //

    if (  lpProxySettings->lpszLastKnownGoodAutoConfigUrl && 
          lpProxySettings->lpszConnectionName &&          
        ! IsExpiredUrl(lpProxySettings->lpszLastKnownGoodAutoConfigUrl))
    {
        // if we're suspecting bad settings, make sure to redirect.
        if ( ! (lpProxySettings->dwAutoDiscoveryFlags & AUTO_PROXY_FLAG_DETECTION_SUSPECT) ) {
            return FALSE; 
        }

        //otherwise, we'll be careful
        fSuspectBadDetect = TRUE;
    }

    //
    // Check for IP addresses that no longer match, indicating a network change
    //

    __try
    {
        lpHostent = _I_gethostbyname(NULL);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        g_fGetHostByNameNULLFails = TRUE;
        lpHostent = NULL;
    }
    ENDEXCEPT

    if (lpHostent &&
        lpProxySettings->pdwDetectedInterfaceIp == NULL)
    {
        return TRUE; // detect needed, no current IPs saved from last run
    }

    if ( lpHostent != NULL &&
         lpProxySettings->pdwDetectedInterfaceIp != NULL)
    {

        for ( addressCount = 0;
              lpHostent->h_addr_list[addressCount] != NULL;
              addressCount++ );  // gather count

        if ( addressCount != lpProxySettings->dwDetectedInterfaceIpCount ) {
            return TRUE; // detect needed, the IP count is different
        }

        if ( fSuspectBadDetect) {
            return FALSE; // detect NOT needed, because the IP addresses may change from dialup/to dialup
        }

        for (DWORD i = 0; i < addressCount; i++)
        {
            //dwAddress[iCount] = *((LPDWORD)(ph->h_addr_list[iCount]));

            if ( *((LPDWORD)(lpHostent->h_addr_list[i])) != lpProxySettings->pdwDetectedInterfaceIp[i] ) {
                return TRUE; // detect needed, mismatched values
            }
        }
    }


    return FALSE; // default, do not need to redetect
}


DWORD
AUTO_PROXY_DLLS::GetHostAddresses(
    DWORD ** ppdwDetectedInterfaceIp,
    DWORD *  pdwDetectedInterfaceIpCount
    )
/*++

Routine Description:

  Copies out the current host information into an
    a array/ProxyInfoStruct for later comparision.

Arguments:

    lpProxySettings - structure to fill

Return Value:

    DWORD
        ERROR_SUCCESS    - success

        Win32 Error code - failure

--*/

{
    DWORD addressCount;
    LPDWORD * addressList;
    LPHOSTENT lpHostent;
    DWORD error = ERROR_SUCCESS;

    *pdwDetectedInterfaceIpCount = 0;

    if ( *ppdwDetectedInterfaceIp )
    {
        FREE_MEMORY(*ppdwDetectedInterfaceIp);
        *ppdwDetectedInterfaceIp = NULL;
    }

    //
    // Gather IP addresses and start copying them over
    //

    __try
    {
        lpHostent = _I_gethostbyname(NULL);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        g_fGetHostByNameNULLFails = TRUE;
        lpHostent = NULL;
    }
    ENDEXCEPT

    if (lpHostent == NULL ) {
        goto quit;
    }

    for ( addressCount = 0;
          lpHostent->h_addr_list[addressCount] != NULL;
          addressCount++ );  // gather count

    *ppdwDetectedInterfaceIp = (LPDWORD)
        ALLOCATE_MEMORY(LMEM_FIXED, addressCount
                        * sizeof(DWORD));

    if ( *ppdwDetectedInterfaceIp == NULL )
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }
    
    if ( *ppdwDetectedInterfaceIp != NULL)
    {
        *pdwDetectedInterfaceIpCount = addressCount;

        for (DWORD i = 0; i < addressCount; i++)
        {
            (*ppdwDetectedInterfaceIp)[i] =
                *((LPDWORD)(lpHostent->h_addr_list[i]));
        }
    }

quit:
    return error;
}


VOID
AUTO_PROXY_DLLS::FreeAutoProxyInfo(
    VOID
    )

/*++

Routine Description:

    Attempts to shutdown the auto-proxy thread (from outside of it), by first signalling
      it with an event and boolean, and then if that fails, forcibly forcing a shutdown
      with TerminateThread.

Arguments:

    none.

Return Value:

    none.

--*/

{
    DWORD dwError;

    LockAutoProxy();

    if ( _hAutoProxyThread != NULL &&
         ! IsOnAsyncAutoProxyThread() )
    {
        HANDLE hAutoProxyThread;
        BOOL fAlreadyInShutDown;

        fAlreadyInShutDown =
            InterlockedExchange((LPLONG)&_fInAutoProxyThreadShutDown, TRUE);

        INET_ASSERT ( ! fAlreadyInShutDown );

        SetEvent(_hAutoProxyThreadEvent);

        if ( _hInternetAbortHandle != NULL )
        {
            InternetCloseHandle(_hInternetAbortHandle);
        }

        hAutoProxyThread = _hAutoProxyThread;

        UnlockAutoProxy();

        //
        // Wait for shutdown of auto-proxy thread.
        //

        if ( hAutoProxyThread )
        {
            dwError = WaitForSingleObject(hAutoProxyThread,
                                            2000
                                            );

            if ( dwError == STATUS_TIMEOUT )
            {
                INET_ASSERT(FALSE);
                TerminateThread(hAutoProxyThread,ERROR_SUCCESS);

                //
                // If we've gotten here than we need to LEAK THE CRITICAL
                //  SECTION, because Terminate thread may have left the
                //  auto-proxy thread with ownership of it
                //

                memset((LPVOID) &_CritSec, 0, sizeof(CRITICAL_SECTION));
                InitializeCriticalSection(&_CritSec);

                _hAutoProxyThreadEvent = NULL;
                _hAutoProxyThread = NULL;

                InterlockedExchange((LPLONG)&_fInAutoProxyThreadShutDown, FALSE);
                return;
            }
        }

        LockAutoProxy();

        CloseHandle(_hAutoProxyThread);
        CloseHandle(_hAutoProxyThreadEvent);

        _hAutoProxyThreadEvent = NULL;
        _hAutoProxyThread = NULL;

        InterlockedExchange((LPLONG)&_fInAutoProxyThreadShutDown, FALSE);

        UnlockAutoProxy();
    }
    else
    {
        UnlockAutoProxy();
    }
}

BOOL
AUTO_PROXY_LIST_ENTRY::ProxyInfoInvalid(
    IN LPSTR lpszMime,
    IN LPSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN LPSTR lpszProxyHostName,
    IN DWORD dwProxyHostNameLength
    )
{
    BOOL success = TRUE; // don't care if it doesn't succeed

    if ( ! _hAutoConfigDLL )
    {
        if ( LoadEntry() != ERROR_SUCCESS )
            return FALSE;
    }

    if ( ! _fInitializedSuccessfully )
    {
        return FALSE;
    }

    if ( _pProxyInfoInvalid )
    {

        INET_ASSERT(_hAutoConfigDLL);
        INET_ASSERT(!IsBadCodePtr((FARPROC)_pProxyInfoInvalid));

        success = (_pProxyInfoInvalid) ( lpszMime,
                                          lpszUrl,
                                          dwUrlLength,
                                          lpszProxyHostName,
                                          dwProxyHostNameLength
                                          );
    }

    return success;
}



BOOL
AUTO_PROXY_LIST_ENTRY::ProxyDllDeInit(
    IN LPSTR lpszMime,
    IN DWORD dwReserved
    )
{
    BOOL success = TRUE; // don't care if it doesn't succeed

    DEBUG_ENTER((DBG_PROXY,
                Bool,
                "AUTO_PROXY_LIST_ENTRY::ProxyDllDeInit",
                "%s, %u",
                lpszMime,
                dwReserved
                ));


    INET_ASSERT(_hAutoConfigDLL);

    if ( !_hAutoConfigDLL  )
    {
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }

    if ( ! _fInitializedSuccessfully )
    {
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }

    if ( _pProxyDllDeInit )
    {

        INET_ASSERT(_hAutoConfigDLL);
        INET_ASSERT(!IsBadCodePtr((FARPROC)_pProxyDllDeInit));
#ifdef INET_DEBUG
        INET_ASSERT(!_fUnInited);

        _fUnInited = TRUE;
#endif

        success = (_pProxyDllDeInit) ( lpszMime,
                                       dwReserved
                                       );
    }

    DEBUG_LEAVE(success);
    return success;
}



DWORD
AUTO_PROXY_LIST_ENTRY::GetProxyInfoEx(
    IN AUTO_PROXY_ASYNC_MSG *pQueryForProxyInfo
    )
{
    BOOL success = FALSE;
    DWORD error = ERROR_SUCCESS;

    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "AUTO_PROXY_LIST_ENTRY::GetProxyInfoEx",
                "%x [%s, %s, %u, %s, %d, %x, %x]",
                pQueryForProxyInfo,
                InternetMapScheme(pQueryForProxyInfo->_tUrlProtocol),
                pQueryForProxyInfo->_lpszUrl,
                pQueryForProxyInfo->_dwUrlLength,
                pQueryForProxyInfo->_lpszUrlHostName,
                pQueryForProxyInfo->_dwUrlHostNameLength,
                pQueryForProxyInfo->_lpszProxyHostName,
                &pQueryForProxyInfo->_dwProxyHostNameLength
                ));

    //if ( ! _hAutoConfigDLL )
    //{
    //    if ( LoadEntry() != ERROR_SUCCESS )
    //    {
    //        DEBUG_LEAVE(FALSE);
    //        return FALSE;
    //    }
    //}

    if ( ! _fInitializedSuccessfully )
    {
        DEBUG_LEAVE(FALSE);
        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    INET_ASSERT(_hAutoConfigDLL);
    INET_ASSERT(_pGetProxyInfoEx);
    INET_ASSERT(!IsBadCodePtr((FARPROC)_pGetProxyInfoEx));

    success = (_pGetProxyInfoEx) ( pQueryForProxyInfo->_tUrlProtocol,
                                    pQueryForProxyInfo->_lpszUrl,
                                    pQueryForProxyInfo->_dwUrlLength,
                                    pQueryForProxyInfo->_lpszUrlHostName,
                                    pQueryForProxyInfo->_dwUrlHostNameLength,
                                    pQueryForProxyInfo->_nUrlPort,
                                    &(pQueryForProxyInfo->_tProxyScheme),
                                    &(pQueryForProxyInfo->_lpszProxyHostName),
                                    &(pQueryForProxyInfo->_dwProxyHostNameLength),
                                    &(pQueryForProxyInfo->_nProxyHostPort)
                                    );

quit:

    pQueryForProxyInfo->_dwQueryResult = (DWORD) success;

    DEBUG_LEAVE(error);
    return error;

}



DWORD
AUTO_PROXY_LIST_ENTRY::GetProxyInfo(
    IN AUTO_PROXY_ASYNC_MSG *pQueryForProxyInfo
    )
{
    BOOL success = FALSE;
    DWORD error = ERROR_SUCCESS;
    LPSTR lpszAutoProxyReturnInfo;
    DWORD dwAutoProxyReturnInfoSize;

    INET_ASSERT(pQueryForProxyInfo);

    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "AUTO_PROXY_LIST_ENTRY::GetProxyInfo",
                "%x [%s, %u, %s, %d, %x, %x]",
                pQueryForProxyInfo,
                pQueryForProxyInfo->_lpszUrl,
                pQueryForProxyInfo->_dwUrlLength,
                pQueryForProxyInfo->_lpszUrlHostName,
                pQueryForProxyInfo->_dwUrlHostNameLength,
                pQueryForProxyInfo->_lpszProxyHostName,
                &pQueryForProxyInfo->_dwProxyHostNameLength
                ));

    //if ( ! _hAutoConfigDLL )
    //{
    //    if ( LoadEntry() != ERROR_SUCCESS )
    //    {
    //        DEBUG_LEAVE(FALSE);
    //        return FALSE;
    //    }
    //}

    if ( ! _fInitializedSuccessfully )
    {
        DEBUG_LEAVE(FALSE);
        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    INET_ASSERT ( _pGetProxyInfo );
    INET_ASSERT(_hAutoConfigDLL);
    INET_ASSERT(!IsBadCodePtr((FARPROC)_pGetProxyInfo));

    success = (_pGetProxyInfo) (    pQueryForProxyInfo->_lpszUrl,
                                    pQueryForProxyInfo->_dwUrlLength,
                                    pQueryForProxyInfo->_lpszUrlHostName,
                                    pQueryForProxyInfo->_dwUrlHostNameLength,
                                    &lpszAutoProxyReturnInfo,
                                    &dwAutoProxyReturnInfoSize
                                    );


    if ( success )
    {
        if ( pQueryForProxyInfo->_tUrlProtocol == INTERNET_SCHEME_HTTPS )
        {
            pQueryForProxyInfo->_tProxyScheme = INTERNET_SCHEME_HTTPS;
        }
        else
        {
            pQueryForProxyInfo->_tProxyScheme = INTERNET_SCHEME_HTTP;
        }

        INET_ASSERT(pQueryForProxyInfo->_pProxyState == NULL);

        pQueryForProxyInfo->_pProxyState = new PROXY_STATE(lpszAutoProxyReturnInfo,
                                                          dwAutoProxyReturnInfoSize,
                                                          TRUE,  // parse netscape-style proxy list
                                                          pQueryForProxyInfo->_tProxyScheme,
                                                          pQueryForProxyInfo->_nProxyHostPort
                                                          );


        INET_ASSERT(lpszAutoProxyReturnInfo);
        GlobalFree(lpszAutoProxyReturnInfo);  // clean up jsproxy.dll return string

        if ( pQueryForProxyInfo->_pProxyState  == NULL )
        {
           error = ERROR_NOT_ENOUGH_MEMORY;
           goto quit;
        }

        error = pQueryForProxyInfo->_pProxyState->GetError();

        if ( error != ERROR_SUCCESS )
        {
            goto quit;
        }
    }


quit:

    pQueryForProxyInfo->_dwQueryResult = (DWORD) success;

    DEBUG_LEAVE(error);
    return error;

}


BOOL
AUTO_PROXY_LIST_ENTRY::ProxyDllInit (
    IN DWORD                  dwVersion,
    IN LPSTR                  lpszDownloadedTempFile,
    IN LPSTR                  lpszMime,
    IN AUTO_PROXY_HELPER_APIS *pAutoProxyCallbacks,
    IN DWORD_PTR              dwReserved
    )
{
    BOOL success = FALSE;

    DEBUG_ENTER((DBG_PROXY,
                Bool,
                "AUTO_PROXY_LIST_ENTRY::ProxyDllInit",
                "%u, %s, %s, %x, %u",
                dwVersion,
                lpszDownloadedTempFile,
                lpszMime,
                pAutoProxyCallbacks,
                dwReserved
                ));


    INET_ASSERT ( _hAutoConfigDLL );

    if ( _pProxyDllInit )
    {

        INET_ASSERT(_hAutoConfigDLL);
        INET_ASSERT(!IsBadCodePtr((FARPROC)_pProxyDllInit));

        success = (_pProxyDllInit) ( dwVersion,
                                     lpszDownloadedTempFile,
                                     lpszMime,
                                     pAutoProxyCallbacks,
                                     dwReserved
                                     );

#ifdef INET_DEBUG
        _fUnInited = FALSE;
#endif
        _fInitializedSuccessfully = success;

    }

    DEBUG_LEAVE(success);
    return success;
}

VOID
AUTO_PROXY_LIST_ENTRY::UnloadEntry(
    VOID
    )

/*++

Routine Description:

    Unloads the DLL function entry points for an Auto-Proxy DLL.

    WARNING: Must be called only on Auto-proxy thread context!

Arguments:

    none.

Return Value:

    none.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                None,
                "AUTO_PROXY_LIST_ENTRY::UnloadEntry",
                NULL
                ));


    if ( _fInitializedSuccessfully &&
         _pProxyDllDeInit )
    {
        ProxyDllDeInit(
            _lpszMimeType,
            0
            );
    }

    if ( _hAutoConfigDLL )
    {
        // HACK HACK This call will block an dll unload time because of the 
        // loader critical section. Ideally we should move this free to the 
        // main thread, but since we are in the midst of IE5 RC's we are doing
        // the least risky change i.e. leaking jsproxy.dll in the process. 
        if (!GlobalDynaUnload)
        {
            FreeLibrary(_hAutoConfigDLL);
        }

        _pGetProxyInfo      = NULL;
        _pGetProxyInfoEx    = NULL;
        _pProxyDllInit      = NULL;
        _pProxyDllDeInit    = NULL;
        _pProxyInfoInvalid  = NULL;
    }

    DEBUG_LEAVE(0);
}


DWORD
AUTO_PROXY_LIST_ENTRY::LoadEntry(
    VOID
    )

/*++

Routine Description:

    Loads the DLL function entry points for an Auto-Proxy DLL.

Arguments:

    none.

Return Value:

    DWORD
        ERROR_SUCCESS    - success

        Win32 Error code - failure

--*/

{
    DWORD error = ERROR_SUCCESS;

    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "AUTO_PROXY_LIST_ENTRY::LoadEntry",
                ""
                ));

    if ( _lpszFileExtensions == NULL  ||
         _lpszDllFilePath == NULL )
    {
        error =  ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    _pGetProxyInfo      = NULL;
    _pProxyInfoInvalid  = NULL;
    _pProxyDllDeInit    = NULL;
    _pProxyDllInit      = NULL;
    _pGetProxyInfoEx    = NULL;

    if ( _hAutoConfigDLL )
    {
        FreeLibrary(_hAutoConfigDLL);
        _hAutoConfigDLL  = NULL;
    }

    _hAutoConfigDLL = LoadLibrary(_lpszDllFilePath);

    if ( _hAutoConfigDLL == NULL )
    {
        error = GetLastError();
        goto quit;
    }



    _pGetProxyInfo     = (GET_PROXY_INFO_FN)
                        GetProcAddress(_hAutoConfigDLL, GET_PROXY_INFO_FN_NAME);


    _pGetProxyInfoEx   = (GET_PROXY_INFO_EX_FN)
                        GetProcAddress(_hAutoConfigDLL, GET_PROXY_INFO_EX_FN_NAME);


    _pProxyInfoInvalid = (PROXY_INFO_INVALID_FN)
                        GetProcAddress(_hAutoConfigDLL, PROXY_INFO_INVALID_FN_NAME);


    _pProxyDllDeInit   = (PROXY_DLL_DEINIT_FN)
                        GetProcAddress(_hAutoConfigDLL, PROXY_DLL_DEINIT_FN_NAME);


    _pProxyDllInit     = (PROXY_DLL_INIT_FN)
                        GetProcAddress(_hAutoConfigDLL, PROXY_DLL_INIT_FN_NAME );


    if ( !_pProxyDllInit && !_pProxyDllDeInit && !_pProxyInfoInvalid && !_pGetProxyInfo && !_pGetProxyInfoEx)
    {
        error = GetLastError();
        goto quit;
    }

    if ( !_pProxyDllInit )
    {
        //
        // If they don't export this entry point, than we can't initialize them, so
        //  we pretend to have initialized them.
        //

        _fInitializedSuccessfully = TRUE;
    }

quit:

    DEBUG_LEAVE(error);

    return error;
}


BOOL
AUTO_PROXY_DLLS::SelectAutoProxyByMime(
    IN LPSTR lpszMimeType
    )

/*++

Routine Description:

    Sets an internal pointer inside the object to point to a found auto-proxy DLL.
        The MIME type given is used to find the match.

Arguments:

    lpszMimeType - The Mime type to search on.

Return Value:

    BOOL
        TRUE    - success

        FALSE - failure

--*/


{

    DEBUG_ENTER((DBG_PROXY,
                Bool,
                "AUTO_PROXY_DLLS::SelectAutoProxyByMime",
                "%s",
                lpszMimeType
                ));

    BOOL found = FALSE;
    AUTO_PROXY_LIST_ENTRY * info = NULL;

    LockAutoProxy();

    LockSerializedList(&_List);

    for (PLIST_ENTRY entry = HeadOfSerializedList(&_List);
        entry != (PLIST_ENTRY)SlSelf(&_List);
        entry = entry->Flink)
    {
        info = CONTAINING_RECORD(entry, AUTO_PROXY_LIST_ENTRY, _List);

        if (info->_lpszMimeType && lstrcmpi(lpszMimeType, info->_lpszMimeType) == 0 )
        {
            found = TRUE;
            break;
        }
    }

    UnlockSerializedList(&_List);

    if ( found )
    {
        SelectAutoProxy(info);
    }

    UnlockAutoProxy();

    DEBUG_LEAVE(found);

    return found;

}

BOOL
AUTO_PROXY_DLLS::SelectAutoProxyByDefault(
    VOID
    )
{

    DEBUG_ENTER((DBG_PROXY,
                Bool,
                "AUTO_PROXY_DLLS::SelectAutoProxyByDefault",
                NULL
                ));

    BOOL found = FALSE;
    AUTO_PROXY_LIST_ENTRY * info = NULL;

    LockAutoProxy();

    LockSerializedList(&_List);

    for (PLIST_ENTRY entry = HeadOfSerializedList(&_List);
        entry != (PLIST_ENTRY)SlSelf(&_List);
        entry = entry->Flink)
    {
        info = CONTAINING_RECORD(entry, AUTO_PROXY_LIST_ENTRY, _List);

        if (info->IsDefault() )
        {
            found = TRUE;
            break;
        }
    }

    UnlockSerializedList(&_List);

    if ( found )
    {
        SelectAutoProxy(info);
    }

    DEBUG_LEAVE(found);

    UnlockAutoProxy();

    return found;
}

BOOL
AUTO_PROXY_DLLS::SelectAutoProxyByFileExtension(
    LPCSTR lpszAutoProxyPath
    )
{

    DEBUG_ENTER((DBG_PROXY,
                Bool,
                "AUTO_PROXY_DLLS::SelectAutoProxyByFileExtension",
                "%s",
                lpszAutoProxyPath
                ));

    BOOL found = FALSE;
    AUTO_PROXY_LIST_ENTRY * info = NULL;

    LockAutoProxy();

    LockSerializedList(&_List);

    for (PLIST_ENTRY entry = HeadOfSerializedList(&_List);
        entry != (PLIST_ENTRY)SlSelf(&_List);
        entry = entry->Flink)
    {
        info = CONTAINING_RECORD(entry, AUTO_PROXY_LIST_ENTRY, _List);

        if (info->_lpszFileExtensions &&
                MatchFileExtensionWithUrl(info->_lpszFileExtensions, lpszAutoProxyPath) )
        {
            found = TRUE;
            break;
        }
    }

    UnlockSerializedList(&_List);

    if ( found )
    {
        SelectAutoProxy(info);
    }

    UnlockAutoProxy();

    DEBUG_LEAVE(found);

    return found;
}

DWORD
AUTO_PROXY_DLLS::GetAutoProxyStringEntry(
    IN LPSTR lpszRegName,
    IN OUT LPSTR * lplpszAllocatedRegValue
    )
{
    DWORD dwcbNewValue = 0;
    DWORD error = ERROR_SUCCESS;

    if ( *lplpszAllocatedRegValue )
    {
        FREE_MEMORY(*lplpszAllocatedRegValue);
        *lplpszAllocatedRegValue = NULL;
    }

    error =
        InternetReadRegistryString(
            lpszRegName,
            NULL,
            &dwcbNewValue
            );

    if ( error == ERROR_SUCCESS )
    {
        dwcbNewValue++;
        *lplpszAllocatedRegValue = (LPSTR) ALLOCATE_MEMORY(LMEM_FIXED, dwcbNewValue);

        if ( *lplpszAllocatedRegValue == NULL )
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }

        error =
            InternetReadRegistryString(
                lpszRegName,
                *lplpszAllocatedRegValue,
                &dwcbNewValue
                );

        if ( error != ERROR_SUCCESS )
        {
            INET_ASSERT((error == ERROR_SUCCESS));
            FREE_MEMORY(*lplpszAllocatedRegValue);
            *lplpszAllocatedRegValue = NULL;
        }
    }

quit:

    return error;
}



DWORD
AUTO_PROXY_DLLS::ReadAutoProxyRegistrySettings(
    VOID
    )

/*++

Routine Description:

    Scans Registry for, and builds linked list of AutoProxy DLLs.

Return Value:

  Success - ERROR_SUCCESS

  Failure -

--*/

{
    HKEY hkSecurity = NULL; // main security key
    DWORD error = ERROR_SUCCESS;
    DWORD dwcbAutoConfigProxy = 0;
    HKEY hkSPMKey = NULL;
    const static CHAR cszMainSecKey[]
        = CSZMAINSECKEY;
    DEBUG_ENTER((DBG_PROXY,
                Dword,
                "AUTO_PROXY_DLLS::ReadAutoProxyRegistrySettings",
                ""
                ));

    LockAutoProxy();

    //
    // QFE 1169:  Use the HKCU value as the user agent string if told to do so.
    //            Yes this looks ugly, but it's just to prevent a whole bunch
    //            of ifs and handlers for failures.

    DWORD dwcbBufLen = 0;
    
    if (ERROR_SUCCESS == InternetReadRegistryDwordKey(
            HKEY_CURRENT_USER,
            "AutoConfigCustomUA",
            &dwcbBufLen) &&
        dwcbBufLen)
    {       
        if (NULL == _hInstUrlmon)
        {
            _hInstUrlmon = LoadLibrary("Urlmon.dll");
        }       
        if (_hInstUrlmon)
        {        
            typedef HRESULT (*PFNOBTAINUA)(DWORD, LPSTR, DWORD*);
            CHAR lpszUserAgent[MAX_PATH];
            DWORD cbSize = MAX_PATH;

            PFNOBTAINUA pfnUA = (PFNOBTAINUA)GetProcAddress(_hInstUrlmon,"ObtainUserAgentString");
            if (pfnUA)
            {
                HRESULT hr = (*pfnUA)(0, lpszUserAgent, &cbSize);
                if(S_OK == hr)
                {
                    if ( _lpszUserAgent && (lstrcmpi(_lpszUserAgent, lpszUserAgent) != 0) )
                    {
                        _lpszUserAgent = (LPSTR)
                            FREE_MEMORY(_lpszUserAgent);
                    }
                    if ( _lpszUserAgent == NULL )
                    {
                        _lpszUserAgent = NewString(lpszUserAgent);
                    }
                }
            }
        }
        //strcpy(szBuffer, "Mozilla/4.0 (compatible; MSIE 4.01; Win32)");
    }

    if (REGOPENKEY( HKEY_CLASSES_ROOT, cszMainSecKey, &hkSecurity ) != ERROR_SUCCESS)
    {
        error = GetLastError();
        goto quit;
    }


    DWORD dwIndex;
    dwIndex = 0;

    do
    {
        CHAR szMime[256];
#ifdef unix
        CHAR szUnixHackMime[256];
#endif /* unix */
        if ( hkSPMKey != NULL )
        {
            REGCLOSEKEY(hkSPMKey);
            hkSPMKey = NULL;
        }

        //
        // Enumerate a List of MIME types, that we accept for auto-proxy
        //

        if (RegEnumKey( hkSecurity, dwIndex, szMime, sizeof(szMime)) != ERROR_SUCCESS )
        {
            goto quit;
        }

        dwIndex++;

        //
        // Open a potential MIME type entry.
        //

        if (REGOPENKEY( hkSecurity, szMime, &hkSPMKey ) != ERROR_SUCCESS)
        {
            INET_ASSERT(0);
            continue;
        }

        //
        // Grab the DLL file name
        //

        DWORD dwType, cbBuf;

        char szDll[MAX_PATH];
        cbBuf = sizeof(szDll);
        if (ERROR_SUCCESS != RegQueryValueEx
                (hkSPMKey, "DllFile", NULL, &dwType, (LPBYTE) szDll, &cbBuf)
            ||  ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ)) )
        {
            continue; // no DLL name
        }


        if ( dwType == REG_EXPAND_SZ )
        {
            DWORD dwSize;
            char szDllPathBeforeExpansion[MAX_PATH];

            lstrcpy(szDllPathBeforeExpansion, szDll);

            dwSize = ExpandEnvironmentStrings(szDllPathBeforeExpansion, szDll, ARRAY_ELEMENTS(szDll));

            if (dwSize > ARRAY_ELEMENTS(szDll) || dwSize == 0 )
            {
                INET_ASSERT(FALSE);
                continue;  // not enough room to expand vars?
            }
        }

        //
        // Grab the list of File extensions that are permitted for this Mime Type
        //


        char szFileExtensions[MAX_PATH];
        cbBuf = sizeof(szFileExtensions);
        if (ERROR_SUCCESS != RegQueryValueEx
                (hkSPMKey, "FileExtensions", NULL, &dwType, (LPBYTE) szFileExtensions, &cbBuf)
            || (dwType != REG_SZ))
        {
            continue; // no DLL name
        }


        //
        // Determine whether its the defaut entry.
        //

        DWORD fIsDefault;
        cbBuf = sizeof(fIsDefault);
        if (ERROR_SUCCESS != RegQueryValueEx
                (hkSPMKey, "Default", NULL, &dwType, (LPBYTE) &fIsDefault, &cbBuf)
            || ((dwType != REG_DWORD) && (dwType != REG_BINARY)))
        {
            INET_ASSERT (cbBuf == sizeof(DWORD));
            fIsDefault = 0;
        }

        //
        // Determine whether we have any Flags that need to be read
        //  from the registry.
        //

        DWORD dwFlags;

        cbBuf = sizeof(dwFlags);
        if (ERROR_SUCCESS != RegQueryValueEx
                (hkSPMKey, "Flags", NULL, &dwType, (LPBYTE) &dwFlags, &cbBuf)
            || ((dwType != REG_DWORD) && (dwType != REG_BINARY)))
        {
            dwFlags = 0;
        }


        //
        // Now build an entry for it, and add it to our list.
        //
#ifndef unix
        AUTO_PROXY_LIST_ENTRY *apleAutoProxy
             = new AUTO_PROXY_LIST_ENTRY(szDll, szFileExtensions, szMime, fIsDefault, dwFlags);
#else
        strcpy(szUnixHackMime,"application/");
        strcat(szUnixHackMime,szMime);
        AUTO_PROXY_LIST_ENTRY *apleAutoProxy
             = new AUTO_PROXY_LIST_ENTRY(szDll, szFileExtensions, szUnixHackMime, fIsDefault, dwFlags);
#endif /* unix */
        if (!apleAutoProxy)
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }

        if (fIsDefault)
        {
            InsertAtTailOfSerializedList(&_List, &apleAutoProxy->_List);
        }
        else
        {
            InsertAtHeadOfSerializedList(&_List, &apleAutoProxy->_List);
        }

    } while (1);

    error = ERROR_SUCCESS;

quit:

    if ( hkSPMKey != NULL )
    {
        REGCLOSEKEY(hkSPMKey);
    }

    if ( hkSecurity != NULL )
    {
        REGCLOSEKEY(hkSecurity);
    }

    UnlockAutoProxy();

    DEBUG_LEAVE(error);

    return error;
}

AUTO_PROXY_ASYNC_MSG::AUTO_PROXY_ASYNC_MSG(
    IN INTERNET_SCHEME isUrlScheme,
    IN LPSTR lpszUrl,
    IN LPSTR lpszUrlHostName,
    IN DWORD dwUrlHostNameLength
    )
{
    URL_COMPONENTS urlComponents;

    Initalize();

    if ( lpszUrl )
    {
        _lpszUrl      = lpszUrl;
        _dwUrlLength  = lstrlen(lpszUrl);
        _tUrlProtocol = isUrlScheme;
        _pmProxyQuery = PROXY_MSG_GET_PROXY_INFO;
        _pmaAllocMode = MSG_ALLOC_STACK_ONLY;

        memset(&urlComponents, 0, sizeof(urlComponents));
        urlComponents.dwStructSize = sizeof(urlComponents);
        urlComponents.lpszHostName = lpszUrlHostName;
        urlComponents.dwHostNameLength = dwUrlHostNameLength;

        //
        // parse out the host name and port. The host name will be decoded; the
        // original URL will not be modified
        //

        if (InternetCrackUrl(lpszUrl, 0, ICU_DECODE, &urlComponents))
        {
           _nUrlPort            = urlComponents.nPort;
           _lpszUrlHostName     = urlComponents.lpszHostName;
           _dwUrlHostNameLength = urlComponents.dwHostNameLength;

           if ( _tUrlProtocol == INTERNET_SCHEME_UNKNOWN )
           {
               _tUrlProtocol = urlComponents.nScheme;
           }

        }
        else
        {
            _Error = GetLastError();
        }
    }
    else
    {
        _Error = ERROR_NOT_ENOUGH_MEMORY;
    }
}

AUTO_PROXY_ASYNC_MSG::AUTO_PROXY_ASYNC_MSG(
    IN INTERNET_SCHEME isUrlScheme,
    IN LPSTR lpszUrlHostName,
    IN DWORD dwUrlHostNameLength
    )
{

    Initalize();

    _tUrlProtocol = isUrlScheme;
    _pmProxyQuery = PROXY_MSG_GET_PROXY_INFO;
    _pmaAllocMode = MSG_ALLOC_STACK_ONLY;
    _lpszUrlHostName     = lpszUrlHostName;
    _dwUrlHostNameLength = dwUrlHostNameLength;
}

AUTO_PROXY_ASYNC_MSG::AUTO_PROXY_ASYNC_MSG(
    IN INTERNET_SCHEME isUrlScheme,
    IN LPSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN LPSTR lpszUrlHostName,
    IN DWORD dwUrlHostNameLength,
    IN INTERNET_PORT nUrlPort
    )
{
    Initalize();

    _tUrlProtocol        = isUrlScheme;
    _pmProxyQuery        = PROXY_MSG_GET_PROXY_INFO;
    _pmaAllocMode        = MSG_ALLOC_STACK_ONLY;
    _nUrlPort            = nUrlPort;
    _lpszUrlHostName     = lpszUrlHostName;
    _dwUrlHostNameLength = dwUrlHostNameLength;
    _lpszUrl             = lpszUrl;
    _dwUrlLength         = dwUrlLength;

}

VOID
AUTO_PROXY_ASYNC_MSG::SetProxyMsg(
    IN INTERNET_SCHEME isUrlScheme,
    IN LPSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN LPSTR lpszUrlHostName,
    IN DWORD dwUrlHostNameLength,
    IN INTERNET_PORT nUrlPort
    )
{
    _tUrlProtocol        = isUrlScheme;
    _pmProxyQuery        = PROXY_MSG_GET_PROXY_INFO;
    _pmaAllocMode        = MSG_ALLOC_STACK_ONLY;
    _nUrlPort            = nUrlPort;
    _lpszUrlHostName     = lpszUrlHostName;
    _dwUrlHostNameLength = dwUrlHostNameLength;
    _lpszUrl             = lpszUrl;
    _dwUrlLength         = dwUrlLength;
}


AUTO_PROXY_ASYNC_MSG::AUTO_PROXY_ASYNC_MSG(
    IN PROXY_MESSAGE_TYPE pmProxyQuery
    )
{
    Initalize();

    _pmaAllocMode = MSG_ALLOC_HEAP_MSG_OBJ_OWNS;
    _pmProxyQuery = pmProxyQuery;
}

AUTO_PROXY_ASYNC_MSG::AUTO_PROXY_ASYNC_MSG(
    IN AUTO_PROXY_ASYNC_MSG *pStaticAutoProxy
    )
{
    Initalize();

    _tUrlProtocol          = pStaticAutoProxy->_tUrlProtocol;
    _lpszUrl               = (pStaticAutoProxy->_lpszUrl) ? NewString(pStaticAutoProxy->_lpszUrl) : NULL;
    _dwUrlLength           = pStaticAutoProxy->_dwUrlLength;
    _lpszUrlHostName       =
                (pStaticAutoProxy->_lpszUrlHostName ) ?
                NewString(pStaticAutoProxy->_lpszUrlHostName, pStaticAutoProxy->_dwUrlHostNameLength) :
                NULL;
    _dwUrlHostNameLength   = pStaticAutoProxy->_dwUrlHostNameLength;
    _nUrlPort              = pStaticAutoProxy->_nUrlPort;
    _tProxyScheme          = pStaticAutoProxy->_tProxyScheme;

    //
    // ProxyHostName is something that is generated by the request,
    //   therefore it should not be copied OR freed.
    //

    INET_ASSERT( pStaticAutoProxy->_lpszProxyHostName == NULL );
    //_lpszProxyHostName     = (pStaticAutoProxy->_lpszProxyHostName ) ? NewString(pStaticAutoProxy->_lpszProxyHostName) : NULL;


    _dwProxyHostNameLength = pStaticAutoProxy->_dwProxyHostNameLength;
    _nProxyHostPort        = pStaticAutoProxy->_nProxyHostPort;
    _pmProxyQuery          = pStaticAutoProxy->_pmProxyQuery;
    _pmaAllocMode          = MSG_ALLOC_HEAP_MSG_OBJ_OWNS;
    _pProxyState           = pStaticAutoProxy->_pProxyState;

    INET_ASSERT(_pProxyState == NULL);

    _dwQueryResult         = pStaticAutoProxy->_dwQueryResult;
    _Error                 = pStaticAutoProxy->_Error;
    _MessageFlags.Dword    = pStaticAutoProxy->_MessageFlags.Dword;
    _dwProxyVersion        = pStaticAutoProxy->_dwProxyVersion;
}

AUTO_PROXY_ASYNC_MSG::~AUTO_PROXY_ASYNC_MSG(
    VOID
    )
{
    DEBUG_ENTER((DBG_OBJECTS,
                None,
                "~AUTO_PROXY_ASYNC_MSG",
                NULL
                ));

    if ( IsAlloced() )
    {
        DEBUG_PRINT(OBJECTS,
                    INFO,
                    ("Freeing Allocated MSG ptr=%x\n",
                    this
                    ));


        if ( _lpszUrl )
        {
            //DEBUG_PRINT(OBJECTS,
            //            INFO,
            //            ("Url ptr=%x, %q\n",
            //            _lpszUrl,
            //            _lpszUrl
            //            ));

            FREE_MEMORY(_lpszUrl);
        }

        if ( _lpszUrlHostName )
        {
            FREE_MEMORY(_lpszUrlHostName);
        }


        if ( _pProxyState )
        {
            delete _pProxyState;
        }
    }
    if (_bFreeProxyHostName && (_lpszProxyHostName != NULL)) {
        FREE_MEMORY(_lpszProxyHostName);
    }

    DEBUG_LEAVE(0);
}



//
// functions
//


PRIVATE
BOOL
MatchFileExtensionWithUrl(
    IN LPCSTR lpszFileExtensionList,
    IN LPCSTR lpszUrl
    )

/*++

Routine Description:

    Matches a Url with a string of externsions (looks like ".ins;.jv").
    Confims the Url is using one the approved externsions.

Arguments:

    lpszFileExtensionList - the list of file extensions to check the Url against.

    lpszUrl  - the url to examine.

Return Value:

    BOOL
        TRUE    - success

        FALSE   - the Url does not contain any of the extensions.

--*/


{

    LPCSTR lpszExtension, lpszExtensionOffset;
    LPCSTR lpszQuestion;

    //
    // We need to be careful about checking for a period on the end of an URL
    //   Example: if we have: "http://auto-proxy-srv/fooboo.exe?autogenator.com.ex" ?
    //

    lpszQuestion = strchr( lpszUrl, '?' );

    lpszUrl = ( lpszQuestion ) ? lpszQuestion : lpszUrl;

    lpszExtension = strrchr( lpszUrl, '.' );


    if ( lpszExtension )
    {
        lpszExtensionOffset = lpszExtension;
        BOOL fMatching = FALSE;
        BOOL fCurrentMatch = FALSE;
        BOOL fFirstByte = TRUE;

        while ( *lpszFileExtensionList && *lpszExtensionOffset )
        {

            if ( toupper(*lpszFileExtensionList) == toupper(*lpszExtensionOffset) )
            {
                fCurrentMatch = TRUE;
            }
            else
            {
                fCurrentMatch = FALSE;
            }

            if ( *lpszFileExtensionList == ';')
            {
                lpszExtensionOffset = lpszExtension-1;
                fFirstByte = TRUE;

                if ( fMatching )
                {
                    return TRUE;
                }
            }
            else if ( fFirstByte || fMatching )
            {
                fMatching = fCurrentMatch;
                fFirstByte = FALSE;
            }

            lpszExtensionOffset++;
            lpszFileExtensionList++;

            if ( *lpszExtensionOffset == '\0' )
            {
                lpszExtensionOffset = lpszExtension;
            }
        }

        if ( fMatching )
        {
            return TRUE;
        }

    }

    return FALSE;
}

#ifdef unix
static BOOL fForceAutoProxSync = TRUE;
extern "C"
void unixForceAutoProxSync()
{
    if(fForceAutoProxSync)
    {
       if (!GlobalDataInitialized)
          GlobalDataInitialize();

       if (GlobalDataInitialized)
       {
          GlobalProxyInfo.SetRefreshDisabled(FALSE);
          FixProxySettingsForCurrentConnection(TRUE);
          GlobalProxyInfo.ReleaseQueuedRefresh();
          fForceAutoProxSync = FALSE;
       }
    }
}
#endif /* unix */

