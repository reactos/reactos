/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    connect.cxx

Abstract:

    Contains methods for INTERNET_CONNECT_HANDLE_OBJECT class

    Contents:
        RMakeInternetConnectObjectHandle
        FindExistingConnectObject
        FlushExistingConnectObjects
        INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT
        INTERNET_CONNECT_HANDLE_OBJECT::~INTERNET_CONNECT_HANDLE_OBJECT
        INTERNET_CONNECT_HANDLE_OBJECT::AttachLastResponseInfo
        INTERNET_CONNECT_HANDLE_OBJECT::SetOriginServer
        INTERNET_CONNECT_HANDLE_OBJECT::SetServerInfo(INTERNET_SCHEME, BOOL, BOOL)
        INTERNET_CONNECT_HANDLE_OBJECT::SetServerInfo(LPSTR, DWORD)

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:

   Sophia Chung (sophiac) 14-Feb-1995 (added FTP and Archie class impl.)
   (code adopted from madana)

--*/

#include <wininetp.h>

#define DEFAULT_VARIABLE_CACHE_INFO_SIZE    512

//
// data
//
LONG GlobalExistingConnectHandles = 0;

extern DWORD dwCacheWriteBufferSize;

BOOL
GetCanonicalizedParentUrl(
    LPSTR   lpszChildUrl,
    LPSTR   lpszParentUrlBuff,
    DWORD   dwBuffSize);

//
// functions
//


DWORD
RMakeInternetConnectObjectHandle(
    IN HINTERNET ParentHandle,
    IN OUT HINTERNET * ChildHandle,
    IN CONNECT_CLOSE_HANDLE_FUNC wCloseFunc,
    IN LPSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPSTR lpszUserName OPTIONAL,
    IN LPSTR lpszPassword OPTIONAL,
    IN DWORD ServiceType,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Creates an INTERNET_CONNECT_HANDLE_OBJECT. Wrapper function callable from
    C code

Arguments:

    ParentHandle    - parent InternetOpen() handle

    ChildHandle     - IN: protocol-specific child handle
                      OUT: address of handle object

    wCloseFunc      - pointer to function to close when object deleted

    lpszServerName  - pointer to server name

    nServerPort     - server port to connect to

    lpszUserName    - optional user name

    lpszPassword    - optional password

    ServiceType     - type of service required, e.g. INTERNET_SERVICE_HTTP

    dwFlags         - various open flags from InternetConnect()

    dwContext       - app-supplied context value to associate with the handle

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DWORD error;
    INTERNET_CONNECT_HANDLE_OBJECT * hConnect;

    hConnect = new INTERNET_CONNECT_HANDLE_OBJECT(
                                (INTERNET_HANDLE_OBJECT *)ParentHandle,
                                *ChildHandle,
                                wCloseFunc,
                                lpszServerName,
                                nServerPort,
                                lpszUserName,
                                lpszPassword,
                                ServiceType,
                                dwFlags,
                                dwContext
                                );

    if (hConnect != NULL) {
        error = hConnect->GetStatus();
        if (error == ERROR_SUCCESS) {

            //
            // inform the app of the new handle
            //

            error = InternetIndicateStatusNewHandle((LPVOID)hConnect);

            //
            // ERROR_INTERNET_OPERATION_CANCELLED is the only error that we are
            // expecting here. If we get this error then the app has cancelled
            // the operation. Either way, the handle we just generated will be
            // already deleted
            //

            if (error != ERROR_SUCCESS) {

                INET_ASSERT(error == ERROR_INTERNET_OPERATION_CANCELLED);

                hConnect = NULL;
            }
        } else {
            delete hConnect;
            hConnect = NULL;
        }
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    *ChildHandle = (HINTERNET)hConnect;

    return error;
}


HINTERNET
FindExistingConnectObject(
    IN HINTERNET hInternet,
    IN LPSTR lpHostName,
    IN INTERNET_PORT nPort,
    IN LPSTR lpszUserName,
    IN LPSTR lpszPassword,
    IN DWORD dwServiceType,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Attempts to find an existing INTERNET_CONNECT_HANDLE_OBJECT with the
    desired attributes

Arguments:

    hInternet       - required parent handle

    lpHostName      - pointer to host name to connect to

    nPort           - port at server to connect to

    lpszUserName    - name of user making requests

    lpszPassword    - password required to establish connection

    dwServiceType   - type of service required

    dwFlags         - extra control information

    dwContext       - required context value

Return Value:

    HINTERNET
        Success - handle of found object

        Failure - NULL

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 Handle,
                 "FindExistingConnectObject",
                 "%#x, %q, %d, %q, %q, %s (%d), %#x, %#x",
                 hInternet,
                 lpHostName,
                 nPort,
                 lpszUserName,
                 lpszPassword,
                 InternetMapService(dwServiceType),
                 dwServiceType,
                 dwFlags,
                 dwContext
                 ));

    HINTERNET hConnect = NULL;
    HINTERNET_HANDLE_TYPE handleType;
    INTERNET_PORT defaultPort;

    switch (dwServiceType) {
    case INTERNET_SERVICE_FTP:
        handleType = TypeFtpConnectHandle;
        defaultPort = INTERNET_DEFAULT_FTP_PORT;
        break;

    case INTERNET_SERVICE_GOPHER:
        handleType = TypeGopherConnectHandle;
        defaultPort = INTERNET_DEFAULT_GOPHER_PORT;
        break;

    case INTERNET_SERVICE_HTTP:
        handleType = TypeHttpConnectHandle;
        defaultPort = (dwFlags & INTERNET_FLAG_SECURE)
                        ? INTERNET_DEFAULT_HTTPS_PORT
                        : INTERNET_DEFAULT_HTTP_PORT;
        break;

    default:

        INET_ASSERT(FALSE);

        break;
    }

    if (nPort == INTERNET_INVALID_PORT_NUMBER) {
        nPort = defaultPort;
    }

    LockSerializedList(&GlobalObjectList);

    PLIST_ENTRY entry;

    for (entry = HeadOfSerializedList(&GlobalObjectList);
        entry != (PLIST_ENTRY)SlSelf(&GlobalObjectList);
        entry = entry->Flink) {

        HANDLE_OBJECT * pObject = CONTAINING_RECORD(entry, HANDLE_OBJECT, _List);

        //
        // check elements of the HANDLE_OBJECT first (DWORDs)
        //

        HANDLE_OBJECT * pParent = (HANDLE_OBJECT *)pObject->GetParent();

        if ((pObject->GetHandleType() == handleType)
        && ((pParent != NULL) && (pParent->GetPseudoHandle() == hInternet))
        && (pObject->GetContext() == dwContext)

        //
        // the handle may be invalidated - its been closed, but is still alive
        // because it has children which are in the process of being closed
        //

        && !pObject->IsInvalidated()) {

            //
            // handle is correct type & has the right parent & context values.
            // Next, check if its reusable and currently unused, and has the
            // correct destination attributes
            //

            INTERNET_CONNECT_HANDLE_OBJECT * pConnect;

            pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)pObject;
            if (!pConnect->IsInUse() && (pConnect->GetHostPort() == nPort)) {

                LPSTR hostName = pConnect->GetHostName();

                if (((lpHostName == NULL) && (hostName == NULL))
                || ((lpHostName != NULL) && (hostName != NULL)
                    && !stricmp(lpHostName, hostName))) {

                    //
                    // must have same name and password, or no name and/or
                    // password
                    //

                    LPSTR userName = pConnect->GetUserOrPass (TRUE, IS_SERVER);

                    if (((lpszUserName == NULL) && (userName == NULL))
                    || ((lpszUserName != NULL) && (userName != NULL)
                        && !strcmp(lpszUserName, userName))) {

                        LPSTR password = pConnect->GetUserOrPass (FALSE, IS_SERVER);

                        if (((lpszPassword == NULL) && (password == NULL))
                        || ((lpszPassword != NULL) && (password != NULL)
                            && !strcmp(lpszPassword, password))) {

                            //
                            // this one will do - should be no other users
                            //

                            //INET_ASSERT(pConnect->ReferenceCount() == 1);
//{
//    if (pConnect->ReferenceCount() != 1) {
//        dprintf("handle %#x [%#x]: refcount = %d\n",
//            pConnect,
//            pConnect->GetPseudoHandle(),
//            pConnect->ReferenceCount()
//            );
//    }
//}
                            if (pConnect->ReferenceCount() == 1) {

                                //
                                // reset the CWD - ignore any error
                                //

                                DWORD error = pConnect->SetCurrentWorkingDirectory("/");

                                INET_ASSERT(error == ERROR_SUCCESS);

                                //
                                // this handle is back in use
                                //

                                pConnect->SetInUse();
                                hConnect = (HINTERNET)pConnect;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    UnlockSerializedList(&GlobalObjectList);

    DEBUG_LEAVE(hConnect);

    return hConnect;
}


INT
FlushExistingConnectObjects(
    IN HINTERNET hInternet
    )

/*++

Routine Description:

    Closes all unused EXISTING_CONNECT objects that are children of hInternet

Arguments:

    hInternet   - parent handle

Return Value:

    INT
        Number of handles flushed

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 Int,
                 "FlushExistingConnectObjects",
                 "%#x",
                 hInternet
                 ));

    INT nFlushed = 0;

    if (GlobalExistingConnectHandles > 0) {

        LockSerializedList(&GlobalObjectList);

        PLIST_ENTRY previous = (PLIST_ENTRY)SlSelf(&GlobalObjectList);
        PLIST_ENTRY entry = HeadOfSerializedList(&GlobalObjectList);

        while (entry != (PLIST_ENTRY)SlSelf(&GlobalObjectList)) {

            HANDLE_OBJECT * pObject = CONTAINING_RECORD(entry, HANDLE_OBJECT, _List);
            HANDLE_OBJECT * pParent = (HANDLE_OBJECT *)pObject->GetParent();
            BOOL flushed = FALSE;

            if ((pParent != NULL) && (pParent->GetPseudoHandle() == hInternet)) {

                HINTERNET_HANDLE_TYPE handleType = pObject->GetHandleType();

                if ((handleType == TypeFtpConnectHandle)
                || (handleType == TypeGopherConnectHandle)
                || (handleType == TypeHttpConnectHandle)) {

                    INTERNET_CONNECT_HANDLE_OBJECT * pConnect;

                    pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)pObject;
                    if (!pConnect->IsInUse() && !pConnect->IsInvalidated()) {

                        //INET_ASSERT(pConnect->ReferenceCount() == 1);

                        IF_DEBUG_CONTROL(ANY) {

                            if (pConnect->ReferenceCount() != 1) {

                                DEBUG_PRINT(OBJECTS,
                                            WARNING,
                                            ("handle %#x [%#x]: refcount = %d\n",
                                            pConnect,
                                            pConnect->GetPseudoHandle(),
                                            pConnect->ReferenceCount()
                                            ));

                                //dprintf("handle %#x [%#x]: refcount = %d\n",
                                //        pConnect,
                                //        pConnect->GetPseudoHandle(),
                                //        pConnect->ReferenceCount()
                                //        );
                            }

                        }

                        //
                        // invalidate the object (stops an assert - we normally
                        // expect the handle to be invalidated by
                        // MapHandleToAddress(), but we're simply dereferencing
                        // the object, which would usually be done by the second
                        // of 2 calls to DereferenceObject(), so we save ourselves
                        // from jumping through hoops just to destroy the object)
                        //

                        pConnect->Invalidate();
                        flushed = pConnect->Dereference();

                        //
                        // the entry was unused; it should have been destroyed
                        //

                        //INET_ASSERT(flushed);

                        DEBUG_PRINT(OBJECTS,
                                    INFO,
                                    ("flushed object %#x\n",
                                    pConnect
                                    ));

                    }
                }
            }

            //
            // if we just destroyed the object pointed at by entry then we need
            // to dereference the previous pointer for the next object
            //

            if (flushed) {
                ++nFlushed;
                entry = previous->Flink;
            } else {
                previous = entry;
                entry = entry->Flink;
            }
        }

        UnlockSerializedList(&GlobalObjectList);
    }

    DEBUG_LEAVE(nFlushed);

//dprintf("*** flushed %d objects\n", nFlushed);
    return nFlushed;
}

//
// INTERNET_CONNECT_HANDLE_OBJECT class implementation
//


INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT(
    INTERNET_CONNECT_HANDLE_OBJECT *InternetConnectObj
    ) : INTERNET_HANDLE_OBJECT((INTERNET_HANDLE_OBJECT *)InternetConnectObj)

/*++

Routine Description:

    Constructor that creates a copy of an INTERNET_CONNECT_HANDLE_OBJECT when
    generating a derived handle object, such as a HTTP_REQUEST_HANDLE_OBJECT

Arguments:

    InternetConnectObj  - INTERNET_CONNECT_HANDLE_OBJECT to copy

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT",
                 "%#x",
                 InternetConnectObj
                 ));

    _InternetConnectHandle = InternetConnectObj->_InternetConnectHandle;
    _wCloseFunction = InternetConnectObj->_wCloseFunction;
    _HandleType = InternetConnectObj->_HandleType;
    _ServiceType = InternetConnectObj->_ServiceType;
    _IsCopy = TRUE;

    //
    // copy the flags except EXISTING_CONNECT - we don't want to influence the
    // number of flushable handles just by closing a request handle
    //

    _Flags = InternetConnectObj->_Flags & ~INTERNET_FLAG_EXISTING_CONNECT;

    //
    // in this case, we are not dealing with a real connect handle object, but a
    // derived object, hence _InUse is FALSE
    //

    _InUse = FALSE;

    _ReadBufferSize = InternetConnectObj->_ReadBufferSize;
    _WriteBufferSize = InternetConnectObj->_WriteBufferSize;

    //
    // copy the name objects and server port
    //

    _HostName = InternetConnectObj->_HostName;
    _HostPort = InternetConnectObj->_HostPort;

    //
    // _SchemeType is actual scheme we use. May be different than original
    // object type when going via CERN proxy. Initially set to default (HTTP)
    //

    _SchemeType = InternetConnectObj->_SchemeType;

    //
    // _LastResponseInfo points to a buffer containing the last response info
    // from an FTP URL operation
    //

    _LastResponseInfo = NULL;

    InitCacheVariables();
    if (InternetConnectObj->_CacheUrlName != NULL) {
        SetURL (InternetConnectObj->_CacheUrlName);
    }
    if (InternetConnectObj->_CacheCWD != NULL) {
        _CacheCWD = NEW_STRING(InternetConnectObj->_CacheCWD);
    }

    // Inherit the PerUserItem status of the parent
    _CachePerUserItem = InternetConnectObj->_CachePerUserItem;
    
    _bViaProxy = InternetConnectObj->_bViaProxy;
    _bNoHeaders = InternetConnectObj->_bNoHeaders;
    _bNetFailed = InternetConnectObj->_bNetFailed;
    _ServerInfo = InternetConnectObj->_ServerInfo;
    _OriginServer = InternetConnectObj->_OriginServer;
    _dwErrorMask = 0;

    //
    // reference the server info to balance the deref in our destructor
    //

    if (_ServerInfo != NULL) {

        //
        // could be cache-only handle
        //

        _ServerInfo->Reference();
    }
    if (_OriginServer != NULL) {
        _OriginServer->Reference();
    }

    DEBUG_LEAVE(0);
}


INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT(
    INTERNET_HANDLE_OBJECT * Parent,
    HINTERNET Child,
    CONNECT_CLOSE_HANDLE_FUNC wCloseFunc,
    LPTSTR lpszServerName,
    INTERNET_PORT nServerPort,
    LPTSTR lpszUsername OPTIONAL,
    LPTSTR lpszPassword OPTIONAL,
    DWORD SrvType,
    DWORD dwFlags,
    DWORD_PTR dwContext
    ) : INTERNET_HANDLE_OBJECT(Parent)

/*++

Routine Description:

    Constructor for direct-to-net INTERNET_CONNECT_HANDLE_OBJECT

Arguments:

    Parent          - pointer to parent handle (INTERNET_HANDLE_OBJECT as
                      created by InternetOpen())

    Child           - handle of child object - typically an identifying value
                      for the protocol-specific code

    wCloseFunc      - pointer to function that handles closes when
                      InternetCloseHandle() called for this object

    lpszServerName  - name of the server we are connecting to. May also be the
                      IP address expressed as a string

    nServerPort     - the port number at the server to which we connect

    lpszUsername    - user name for logon at server (if required)

    lpszPassword    - password for logon at server (if required)

    SrvType         - Type of service, e.g. INTERNET_SERVICE_HTTP that this
                      object represents

    dwFlags         - creation flags from InternetConnect():

                        - INTERNET_FLAG_PASSIVE

    dwContext       - context value for call-backs

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT",
                 "%#x, %#x, %#x, %q, %d, %q, %q, %s (%d), %#x, %#x",
                 Parent,
                 Child,
                 wCloseFunc,
                 lpszServerName,
                 nServerPort,
                 lpszUsername,
                 lpszPassword,
                 InternetMapService(SrvType),
                 SrvType,
                 dwFlags,
                 dwContext
                 ));

    _InternetConnectHandle = Child;
    _wCloseFunction = wCloseFunc;
    _ServiceType = SrvType;
    _Context = dwContext;
    _IsCopy = FALSE;

    SetHandleType(SrvType);

    //
    // remember the creation flags. Mainly (currently) for HTTP Keep-Alive
    //

    _Flags = dwFlags;

    //
    // setting _InUse to TRUE stops any EXISTING_CONNECT requests from acquiring
    // it
    //

    _InUse = TRUE;

    InitCacheVariables();

    //
    // set the read/write buffer sizes to the default values (4K)
    //

    _ReadBufferSize = (4 K);
    _WriteBufferSize = (4 K);

    //
    // create the string buffer and copy the port number
    //

    _HostName = lpszServerName;
    if (lpszUsername) {
        _xsUser.SetData(lpszUsername);
    }
    if (lpszPassword) {
        _xsPass.SetData(lpszPassword);
    }
    _HostPort = nServerPort;

    //
    // set the scheme and object types based on the service type
    //

    INTERNET_SCHEME schemeType;
    HINTERNET_HANDLE_TYPE handleType;

    switch (_ServiceType) {
    case INTERNET_SERVICE_HTTP:
        schemeType = INTERNET_SCHEME_HTTP;
        handleType = TypeHttpConnectHandle;
        break;

    case INTERNET_SERVICE_FTP:
        schemeType = INTERNET_SCHEME_FTP;
        handleType = TypeFtpConnectHandle;
        break;

    case INTERNET_SERVICE_GOPHER:
        schemeType = INTERNET_SCHEME_GOPHER;
        handleType = TypeGopherConnectHandle;
        break;

    default:
        schemeType = INTERNET_SCHEME_DEFAULT;
        handleType = TypeWildHandle;
        break;
    }
    SetSchemeType(schemeType);
    SetObjectType(handleType);

    //
    // _LastResponseInfo points to a buffer containing the last response info
    // from an FTP URL operation
    //

    _LastResponseInfo = NULL;
    _bViaProxy = FALSE;
    _bNoHeaders = TRUE;
    _bNetFailed = FALSE;
    _HandleFlags.fServerUserPassValid = TRUE;
    _HandleFlags.fProxyUserPassValid = TRUE;

    //
    // we need to get the server info that we are going to connect to. In the
    // HTTPS case, we don't yet have enough info to make a proper decision, so
    // we defer that until HttpOpenRequest() at which point we may get another
    // SERVER_INFO
    //

    _ServerInfo = NULL;
    _OriginServer = NULL;
    _Status = SetServerInfo(schemeType, FALSE);

    DEBUG_LEAVE(0);
}


INTERNET_CONNECT_HANDLE_OBJECT::~INTERNET_CONNECT_HANDLE_OBJECT(VOID)

/*++

Routine Description:

    Destructor for INTERNET_CONNECT_HANDLE_OBJECT

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_CONNECT_HANDLE_OBJECT::~INTERNET_CONNECT_HANDLE_OBJECT",
                 NULL
                 ));

    if ((!_IsCopy) && (_InternetConnectHandle != INET_INVALID_HANDLE_VALUE)) {

        HINTERNET _INetHandle;
        DWORD LocalError = ERROR_SUCCESS;

        _INetHandle = this->GetInternetHandle();

        if (_INetHandle == LOCAL_INET_HANDLE) {
            if (_wCloseFunction != NULL) {
                LocalError = ERROR_SUCCESS;
                if (!(this->GetInternetOpenFlags() & INTERNET_FLAG_OFFLINE)) {
                    LocalError = _wCloseFunction(_InternetConnectHandle,
                                                 _ServiceType
                                                 );
                }
            } else {

                INET_ASSERT(LocalError == ERROR_SUCCESS);

            }
        }

        //INET_ASSERT( LocalError == ERROR_SUCCESS );

    }

    if (_CacheReadInProgress) {

        INET_ASSERT(_CacheWriteInProgress == FALSE);

        EndCacheRetrieval();
    } else if (_CacheWriteInProgress) {

        // Abort cache write operation
        //

        EndCacheWrite(NULL, NULL, NULL, 0xffffffff, 0, NULL, NULL);
    }

    // background update if the flag is set
    if( _fLazyUpdate )
    {
        LazyUpdate();
    }


    if (_hLockRequestInfo) {

        //
        // If the request is locked, the last InternetUnlockRequestFile
        // will clean up so there is no need to check _fDeleteDataFile.
        //

        if (_fDeleteDataFile) {

            // We let InternetUnlockRequestFile know that it doesn't
            // have to do a cache lookup and if DeleteFile fails that
            // it should add the file to the leaked list.

            LPLOCK_REQUEST_INFO pLock = (LPLOCK_REQUEST_INFO) _hLockRequestInfo;
            pLock->fNoCacheLookup = TRUE;

        }

        InternetUnlockRequestFile(_hLockRequestInfo);

    } else if (_fDeleteDataFile) {

        //
        // This flag is set if we are not committing a download file to cache,
        // either because we never intended to or the download was aborted.
        //

        if (!DeleteFile (_CacheFileName)) {

            switch (GetLastError()) {
                case ERROR_SHARING_VIOLATION:
                case ERROR_ACCESS_DENIED:
                 UrlCacheAddLeakFile (_CacheFileName);
            }
        }
    }

    // delete the staled entry (to prevent back/fwd see the staled entry)
    if( _fDeleteDataFile && _CacheUrlName ) {
        DeleteUrlCacheEntry(_CacheUrlName);
    }

    FreeCacheFileName();

    INET_ASSERT(_CacheFileName == NULL);
    INET_ASSERT(_CacheFileHandle == INVALID_HANDLE_VALUE);

    if (_CacheCWD) {
        _CacheCWD = (LPSTR)FREE_MEMORY((HLOCAL)_CacheCWD);

        INET_ASSERT(_CacheCWD == NULL);

    }

    // if there is refcount, then remove it


    FreeURL();

    SetOriginalUrl(NULL);

#ifdef LAZY_WRITE
    if (_CacheScratchBuf) {
        _CacheScratchBuf = (LPBYTE)FREE_MEMORY((HLOCAL)_CacheScratchBuf);

        INET_ASSERT(_CacheScratchBuf == NULL);

    }
#endif

    FreeLastResponseInfo();

    if ((_Flags & INTERNET_FLAG_EXISTING_CONNECT) && !_InUse) {

        //
        // one less handle that can be flushed right now
        //

//dprintf("GlobalExistingConnectHandles = %d\n", GlobalExistingConnectHandles);

        if (InterlockedDecrement(&GlobalExistingConnectHandles) < 0) {

            INET_ASSERT(FALSE);

            GlobalExistingConnectHandles = 0;
        }
    }

    if (_ServerInfo != NULL) {
        _ServerInfo->Dereference();
    }
    if (_OriginServer != NULL) {
        _OriginServer->Dereference();
    }

    DEBUG_LEAVE(0);
}

BOOL INTERNET_CONNECT_HANDLE_OBJECT::SetURL (LPSTR lpszUrl)
{
    LPSTR lpszNew;

    if (!_xsSecondaryCacheKey.GetPtr()) {

        // Make an undecorated copy of the URL.

        lpszNew = NewString(lpszUrl);
        if (!lpszNew) {
            return FALSE;
        }

    } else {

        // Decorate the URL by appending the secondary cache key.

        lpszNew = CatString (lpszUrl, _xsSecondaryCacheKey.GetPtr());
        if (!lpszNew) {
            return FALSE;
        }

        // Restore the undecorated URL as the primary cache key.

        if (!_xsPrimaryCacheKey.SetData (lpszUrl)) {
            FREE_MEMORY (lpszNew);
            return FALSE;
        }

    }

    // Clear any previous cache key and record the new one.

    FreeURL();
    INET_ASSERT (lpszNew);
    _CacheUrlName = lpszNew;
    return TRUE;
}

BOOL INTERNET_CONNECT_HANDLE_OBJECT::SetURLPtr(LPSTR* ppszUrl)
{
    LPSTR lpszNew;

    if (!_xsSecondaryCacheKey.GetPtr()) {

        // Swap in the new URL as the cache key.

        FreeURL();
        _CacheUrlName = *ppszUrl;
        *ppszUrl = NULL;

    } else {

        // Decorate the URL by appending the secondary cache key.

        lpszNew = CatString (*ppszUrl, _xsSecondaryCacheKey.GetPtr());
        if (!lpszNew) {
            return FALSE;
        }

        // Back up the undecorated URL as the primary cache key.

        _xsPrimaryCacheKey.SetPtr (ppszUrl);
        INET_ASSERT (!*ppszUrl);

        // Clear any previous cache key and record the new one.

        FreeURL();
        _CacheUrlName = lpszNew;
    }

    return TRUE;
}


BOOL INTERNET_CONNECT_HANDLE_OBJECT::SetSecondaryCacheKey (LPSTR lpszKey)
{
    LPSTR lpszTemp = NULL;


    if (_CacheUrlName) {

        // Decorate the URL by appending the secondary cache key.

        // BUGBUG: what if it is already decorated?  The app
        // better not set the secondary cache key more than once.

        lpszTemp = CatString (_CacheUrlName, lpszKey);
        if (!lpszTemp)
            return FALSE;
    }

    // Save the secondary cache key in case we later change the URL.

    if (!_xsSecondaryCacheKey.SetData (lpszKey)) {

        if (lpszTemp) {
            FREE_MEMORY (lpszTemp);
        }
        return FALSE;
    }

    if (lpszTemp)
    {
        // Back up the undecorated URL as the primary cache key.

        _xsPrimaryCacheKey.SetPtr (&_CacheUrlName);
        INET_ASSERT (!_CacheUrlName);
        _CacheUrlName = lpszTemp;
    }

    return TRUE;
}

void INTERNET_CONNECT_HANDLE_OBJECT::FreeSecondaryCacheKey (void)
{
    if (_xsSecondaryCacheKey.GetPtr()) {

        // Free the secondary key and the decorated URL.

        _xsSecondaryCacheKey.Free();
        FreeURL();

        // Back up the cache key from the undecorated URL.

        LPSTR lpszOld = _xsPrimaryCacheKey.ClearPtr();
        _CacheUrlName = lpszOld;
    }
}


HINTERNET
INTERNET_CONNECT_HANDLE_OBJECT::GetHandle(
    VOID
    )
{
    return _InternetConnectHandle;
}

//
// Cache methods.
//

char* back_up(char* stopper, char* ptr) {

    INET_ASSERT(stopper <= ptr);

    while ((*ptr != '/') && (ptr >= stopper)) --ptr;
    return ((ptr >= stopper) && (*ptr == '/')) ? ptr : NULL;
}

char* convert_macros(char* path) {

    char* ls = NULL;    // last slash
    char* pls = NULL;   // previous last slash
    char* p = path;

    while (*p) {
        if (*p == '/') {
            pls = ls;
            ls = p;
        }
        if (*p == '.') {
            if (*(p + 1) == '/') {
                p = lstrcpy(ls, p + 1);
            } else if (*(p + 1) == '\0') {
                if (*(p - 1) == '/') {
                    *p = '\0';
                }
            } else if (!strncmp(p, "../", 3)) {
                if ((!pls) || (ls != p - 1)) {
                    return NULL;
                }
                p = lstrcpy(pls, p + 2);
                ls = pls;
                pls = back_up(path, max(path, pls - 1));
            } else if (!lstrcmp(p, "..")) {
                if ((*(p - 1) != '/') || !pls) {
                    return NULL;
                } else {
                    *(pls + 1) = 0;
                    p = pls - 1;
                }
            }
        }
        ++p;
    }
    return path;
}

DWORD
INTERNET_CONNECT_HANDLE_OBJECT::SetCurrentWorkingDirectory(
    IN LPSTR lpszCWD
    )
{
    INET_ASSERT(lpszCWD != NULL);

    //
    // BUGBUG - we assume lpszCWD is clean which might not be true....
    //

    int clen;
    int slen;
    LPSTR cwd;

    if (*lpszCWD == '/') {
        cwd = NULL;
        ++lpszCWD;
    } else {
        cwd = _CacheCWD;
    }

    if (!cwd) {
        clen = 1;
    } else {
        clen = lstrlen(cwd);
    }

    slen = lstrlen(lpszCWD);

    LPSTR buffer = (LPSTR)ALLOCATE_FIXED_MEMORY(clen + 1 + slen + 1);

    if (buffer == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (clen == 1) {
        buffer[0] = '/';
    } else {
        memcpy(buffer, _CacheCWD, clen);
    }

    memcpy(&buffer[clen], lpszCWD, slen);
    clen += slen;
    if ((clen > 1) && (lpszCWD[slen - 1] != '/')) {
        buffer[clen++] = '/';
    }
    buffer[clen] = '\0';

    LPSTR p = convert_macros(buffer);

    if (p) {
        if (_CacheCWD != NULL) {
            FREE_MEMORY(_CacheCWD);
            _CacheCWD = NULL;
        }
        _CacheCWD = NewString(p);
    }

    FREE_MEMORY(buffer);

    return (p == NULL) ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS;
}

DWORD
INTERNET_CONNECT_HANDLE_OBJECT::GetCurrentWorkingDirectory(
    LPSTR   lpszCWD,
    LPDWORD lpdwLen
    )
{
    DWORD   dwlenCWD;

    if (!_CacheCWD) {
        *lpdwLen = 0;
    }
    else {
        // do something if the guy gave us any buffer
        if (*lpdwLen) {
            dwlenCWD = lstrlen(_CacheCWD);

            // if the buffer is not enough, copy the size of the buffer
            if (dwlenCWD >= *lpdwLen) {
                memcpy(lpszCWD, _CacheCWD, *lpdwLen);
            }
            else {
                strcpy(lpszCWD, _CacheCWD);
                *lpdwLen = dwlenCWD;
            }
        }
    }
    return (ERROR_SUCCESS);
}

DWORD
INTERNET_CONNECT_HANDLE_OBJECT::SetObjectName(
    LPSTR lpszObjectName,
    LPSTR lpszExtension,
    URLGEN_FUNC * procProtocolUrl
    )
{
    DWORD   dwLen, dwError;
    INTERNET_SCHEME schemeType;

    // BUGBUG move this to protocol specific object

    //
    // if there is already an object name, then free it. We are replacing it
    //

    //
    // BUGBUG - make _CacheUrlString an ICSTRING
    //

    FreeURL();

    //
    // get protocol specific url
    //

    if (procProtocolUrl) {

        //
        // if we are going via proxy AND this is an FTP object AND the user name
        // consists of <username>@<servername> then <servername> is the real
        // server name, and _HostName is the name of the proxy
        //

        //
        // BUGBUG - this is a bit of a hack(!)
        //

        LPSTR target = _HostName.StringAddress();

        if (IsProxy()
        && (GetSchemeType() == INTERNET_SCHEME_FTP)
        && (_xsUser.GetPtr())) {

            LPSTR at = strchr(_xsUser.GetPtr(), '@');

            if (at != NULL) {
                target = at + 1;

                INET_ASSERT(*target);

            }
        }
        schemeType = GetSchemeType();

        // make the scheme type https if necessary

        schemeType = (((schemeType == INTERNET_SCHEME_DEFAULT)||
                      (schemeType == INTERNET_SCHEME_HTTP)) &&
                      (_dwCacheFlags & INTERNET_FLAG_SECURE))?
                      INTERNET_SCHEME_HTTPS: schemeType;

        LPSTR lpszNewUrl = NULL;

        dwError = (*procProtocolUrl)(schemeType,
                                     target,
                                     _CacheCWD,
                                     lpszObjectName,
                                     lpszExtension,
                                     _HostPort,
                                     &lpszNewUrl,
                                     &dwLen
                                     );

        if (dwError == ERROR_SUCCESS) {

            if (!SetURLPtr (&lpszNewUrl)) {
                FREE_MEMORY (lpszNewUrl);
                dwError = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }

    if (dwError == ERROR_SUCCESS) {

        DEBUG_PRINT(HANDLE,
                    INFO,
                    ("Url: %s\n",
                    _CacheUrlName
                    ));

    }
    return dwError;
}

DWORD
INTERNET_CONNECT_HANDLE_OBJECT::BeginCacheRetrieval(
    LPCACHE_ENTRY_INFO  *lplpCacheEntryInfo
    )
{
    DEBUG_ENTER((DBG_CACHE,
                 Dword,
                 "INTERNET_CONNECT_HANDLE_OBJECT::BeginCacheRetrieval",
                 "{%q} %#x",
                 _CacheUrlName,
                 lplpCacheEntryInfo
                 ));

    DWORD Error = ERROR_NOT_SUPPORTED;
    DWORD dwBufferSize = 0;
    int i;

    INET_ASSERT( _CacheReadInProgress == FALSE );
    INET_ASSERT( _CacheWriteInProgress == FALSE );
    //INET_ASSERT( _CacheFileName == NULL );
    INET_ASSERT( _CacheFileHandle == INVALID_HANDLE_VALUE );
    INET_ASSERT( _hCacheStream == NULL);

    *lplpCacheEntryInfo = NULL;

    if (!_CacheUrlName) {

        DEBUG_PRINT(CACHE,
                    ERROR,
                    ("Cache: No UrlName\n"
                    ));

        DEBUG_LEAVE(ERROR_INVALID_PARAMETER);

        return (ERROR_INVALID_PARAMETER);
    }

    dwBufferSize = sizeof(CACHE_ENTRY_INFO) + DEFAULT_VARIABLE_CACHE_INFO_SIZE;
    for (i=0; i<2; ++i) {

        if (*lplpCacheEntryInfo != NULL) {
            FREE_MEMORY(*lplpCacheEntryInfo);
        }

        *lplpCacheEntryInfo = (LPCACHE_ENTRY_INFO)ALLOCATE_MEMORY(
                                    LPTR
                                    , dwBufferSize);
        if (*lplpCacheEntryInfo) {
            _hCacheStream = RetrieveUrlCacheEntryStream( _CacheUrlName,
                                       *lplpCacheEntryInfo,
                                        &dwBufferSize,
                                        FALSE, // Not Random, sequential
                                        0);
            if (_hCacheStream == NULL) {

                //
                // second time around the buffer must be sufficient
                //

                INET_ASSERT(!((i == 1) && (Error == ERROR_INSUFFICIENT_BUFFER)));

                Error = GetLastError();

                if ((i == 1) || (Error != ERROR_INSUFFICIENT_BUFFER)) {
                    goto Cleanup;
                }
            } else {

                break; // success
            }
        }
    }

    Error = RecordCacheRetrieval (*lplpCacheEntryInfo);

Cleanup:

    if( Error != ERROR_SUCCESS ) {

        if (*lplpCacheEntryInfo) {
            FREE_MEMORY(*lplpCacheEntryInfo);
            *lplpCacheEntryInfo = NULL;
        }
        FreeCacheFileName();
    }

    DEBUG_LEAVE(Error);

    return( Error );
}

DWORD INTERNET_CONNECT_HANDLE_OBJECT::RecordCacheRetrieval
    (LPCACHE_ENTRY_INFO lpCacheEntryInfo)
{
    //
    // save cache file name.
    //
    FreeCacheFileName();
    INET_ASSERT(!_CacheFileName);
    _CacheFileName = NewString((lpCacheEntryInfo)->lpszLocalFileName);
    if (!_CacheFileName) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    _dwStreamRefCount = 0;
    _dwCurrentStreamPosition = 0;

    //
    // we have this much data immediately available to the application
    //

    SetAvailableDataLength((lpCacheEntryInfo)->dwSizeLow);

    //
    // and we automatically have the end-of-file indication
    //

    SetEndOfFile();

    _CacheReadInProgress = TRUE;

    return ERROR_SUCCESS;
}


DWORD
INTERNET_CONNECT_HANDLE_OBJECT::ReadCache(
    LPBYTE lpbBuffer,
    DWORD dwBufferLen,
    LPDWORD lpdwBytesRead
    )
{
    DWORD dwError = ERROR_NOT_SUPPORTED;
    BOOL fOk;

    INET_ASSERT( _CacheReadInProgress == TRUE );

    *lpdwBytesRead = dwBufferLen;

    fOk = ReadUrlCacheEntryStream(
                                    _hCacheStream,
                                    _dwCurrentStreamPosition,
                                    lpbBuffer,
                                    lpdwBytesRead,
                                    0);
    if (fOk) {
        _dwCurrentStreamPosition += *lpdwBytesRead;
    }

    if( !fOk ) {
        dwError = GetLastError() ;
    }
    else {
        dwError =  ERROR_SUCCESS;

        DEBUG_PRINT(CACHE,
                    INFO,
                    ("read %d bytes from cache\n",
                    *lpdwBytesRead
                    ));

    }

    return(dwError);
}

DWORD
INTERNET_CONNECT_HANDLE_OBJECT::EndCacheRetrieval(
    VOID
    )
{
    DEBUG_ENTER((DBG_CACHE,
                 Dword,
                 "EndCacheRetrieval",
                 "Url=%s, File=%s",
                 _CacheUrlName, _CacheFileName
                 ));

    DWORD Error = ERROR_SUCCESS;

    INET_ASSERT( _CacheUrlName != NULL );
    INET_ASSERT( _CacheReadInProgress == TRUE );
    INET_ASSERT( _CacheWriteInProgress == FALSE );


    INET_ASSERT(_hCacheStream != NULL);

    if (!_dwStreamRefCount) {    // if the caller obtained it using GetCacheStream
                                // then it is his responsibility to call
                                // UnlockCacheStream
        DEBUG_PRINT(CACHE,
                        INFO,
                        ("Wininet.EndCacheRetrieval: Calling UnlockUrlCacheEntryStream for %s\n",
                        _CacheUrlName
                        ));
        if (!UnlockUrlCacheEntryStream(_hCacheStream, 0)) {
            Error = GetLastError();
        }
    }

    if (Error == ERROR_SUCCESS) {
        _CacheReadInProgress = FALSE;
        _hCacheStream = NULL;
        _dwCurrentStreamPosition = 0;
        _dwStreamRefCount = 0;
    }

    DEBUG_LEAVE(Error);
    return( Error );
}


DWORD
INTERNET_CONNECT_HANDLE_OBJECT::LazyUpdate()
{
    DWORD dwError = ERROR_INTERNET_INTERNAL_ERROR;
    INET_ASSERT(_CacheUrlName);

    dwError = CreateAndQueueBackgroundWorkItem(_CacheUrlName);
    return dwError;
}

DWORD
INTERNET_CONNECT_HANDLE_OBJECT::GetCacheStream(
    LPBYTE  lpBuffer,
    DWORD   dwLen
    )
{
    DWORD   dwError = ERROR_INVALID_FUNCTION;
    if (_CacheReadInProgress) {
        if (dwLen > sizeof(_hCacheStream)) {
            ++_dwStreamRefCount;
            *(HANDLE *)lpBuffer = _hCacheStream;
            dwError = ERROR_SUCCESS;
        }
        else {
            dwError = ERROR_INSUFFICIENT_BUFFER;
        }
    }
    return (dwError);
}

DWORD
INTERNET_CONNECT_HANDLE_OBJECT::ReleaseCacheStream(
    HANDLE  hStream
    )
{
    DWORD   dwError = ERROR_INVALID_FUNCTION;
    if (_CacheReadInProgress) {
        if (_hCacheStream == hStream) {
            --_dwStreamRefCount;
            dwError = ERROR_SUCCESS;
        }
        else {
            dwError = ERROR_INVALID_PARAMETER;
        }
    }
    return (dwError);
}

DWORD
INTERNET_CONNECT_HANDLE_OBJECT::BeginCacheWrite(
    DWORD   dwExpectedLength,
    LPCSTR  lpszFileExtension,
    LPCSTR  lpszFileName
    )
{
    DEBUG_ENTER((DBG_CACHE,
                 Dword,
                 "BeginCacheWrite",
                 "%d, %q",
                 dwExpectedLength,
                 lpszFileExtension
                 ));

    DWORD Error=ERROR_NOT_SUPPORTED;

    CHAR FileName[MAX_PATH];
    CHAR* pFileName;

    // BUGBUG   uncode version needs to be fixed


    INET_ASSERT( _CacheReadInProgress == FALSE );
    INET_ASSERT( _CacheWriteInProgress == FALSE );
    FreeCacheFileName(); // may be left over from Begin/EndCacheRetrieval
                         // in case of ftp/gopher dir raw/html mismatch
    INET_ASSERT( _CacheFileHandle == INVALID_HANDLE_VALUE);

    if (!_CacheUrlName) {

        DEBUG_PRINT(CACHE,
                    ERROR,
                    ("Invalid parameter\n"
                    ));

        DEBUG_LEAVE(ERROR_INVALID_PARAMETER);

        return (ERROR_INVALID_PARAMETER);
    }

    // lpszFileName passed in indicates that
    // we want to create a filename from scratch.
    if (!lpszFileName)
    {
        *FileName = '\0';
        pFileName = FileName;
    }
    // Otherwise, attempt to use the filename passed in.
    else
        pFileName = (CHAR*) lpszFileName;


    // Create the cache file.

    Error = UrlCacheCreateFile(
                    _CacheUrlName,
                    (CHAR*) lpszFileExtension,
                    pFileName,
                    &_CacheFileHandle);

    if (Error != ERROR_SUCCESS)
    {
        DEBUG_PRINT(CACHE,
                    ERROR,
                    ("Cache: Error %ld createurlcacheentry failed for %s\n",
                    Error,
                    _CacheUrlName
                    ));

        DEBUG_LEAVE(Error);

        return( Error); // BUGBUG refine this error
    } else IF_DEBUG(CACHE) {
        DEBUG_PRINT(CACHE, INFO, ("cache filename = %q\n", pFileName));
    }

//dprintf("caching %s (%s) in %s\n", _CacheUrlName, _OriginalUrl, FileName);

    //
    // save names.
    //

    INET_ASSERT(!_CacheFileName);
    _CacheFileName = NewString(pFileName);
    if (!_CacheFileName) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    Error = ERROR_SUCCESS;

    INET_ASSERT(_CacheFileHandle != INVALID_HANDLE_VALUE);

    _CacheWriteInProgress = TRUE;
    Error =  ERROR_SUCCESS;


Cleanup:

    if( Error != ERROR_SUCCESS ) {

        if( _CacheFileHandle != INVALID_HANDLE_VALUE ) {
            CloseHandle( _CacheFileHandle );
            _CacheFileHandle = INVALID_HANDLE_VALUE;
        }

        FreeCacheFileName();

        //
        // delete file temp file
        //

        BOOL BoolError;

        BoolError = DeleteFile( pFileName );
        INET_ASSERT( BoolError == TRUE );
    }

    DEBUG_LEAVE(Error);

    return( Error );
}

DWORD
INTERNET_CONNECT_HANDLE_OBJECT::WriteCache(
    LPBYTE Buffer,
    DWORD BufferLen
    )
{
    DWORD   dwError = ERROR_NOT_SUPPORTED;
    DWORD   dwSize, dwUsed, dwRemain;
    BOOL    fWriteToDisk = TRUE;

    DWORD BytesWritten;

    INET_ASSERT( _CacheWriteInProgress == TRUE );

/*
    DEBUG_PRINT( CACHE, INFO,
                ("Writecache: _virtualCacheFileSize=%d, realfilesize= %d, inputsize=%d\n",
                        _VirtualCacheFileSize, _RealCacheFileSize, BufferLen));
*/

#ifdef LAZY_WRITE

    LPBYTE   lpScratch, lpBuffer;

    if (_dwCacheFlags & INTERNET_FLAG_NEED_FILE) {

        lpScratch = GetCacheScratchBuf(&dwSize, &dwUsed);

        if (lpScratch) {

            // don't do default writes to disk
            fWriteToDisk = FALSE;

            lpBuffer = Buffer;
            dwRemain = BufferLen;

            INET_ASSERT(dwSize >= dwUsed);


            while ((dwUsed+dwRemain) >= dwSize) {

                DEBUG_PRINT( CACHE, INFO,
                            ("remaining=%d\n",
                             dwRemain));

                // Fill the buffer to the brim

                CopyToScratch(lpBuffer, (dwSize-dwUsed));

                lpBuffer += (dwSize-dwUsed);

                dwRemain -= (dwSize-dwUsed);

                // and write it out
                dwError = WriteToDisk(lpScratch, dwSize, &BytesWritten);

                if( dwError != ERROR_SUCCESS ) {

                    goto bailout;

                }

                INET_ASSERT( BytesWritten == dwSize );

                //mark the buffer as empty
                ResetScratchUseSize();

                // get it's location and new used size
                lpScratch = GetCacheScratchBuf(NULL, &dwUsed);
                INET_ASSERT(dwUsed == 0);

            }

            // if anything remain after our disk-writing frenzy
            // then keep it in the buffer

            if (dwRemain) {

                CopyToScratch(lpBuffer, dwRemain);

            }

        }
    }
#endif //LAZY_WRITE

    if (fWriteToDisk){

        // DEBUG_PRINT( CACHE, INFO, ("no lazy write, flushing to disk\n"));

        dwError = WriteToDisk(Buffer, BufferLen, &BytesWritten);

        if( dwError != ERROR_SUCCESS ) {

            goto bailout;

        }

        INET_ASSERT( BytesWritten == BufferLen );
    }


    _VirtualCacheFileSize += BufferLen;

#ifdef LAZY_WRITE
    INET_ASSERT(_VirtualCacheFileSize == (_RealCacheFileSize+_CacheScratchUsedLen));
#else
    INET_ASSERT(_VirtualCacheFileSize == (_RealCacheFileSize));
#endif

    dwError =  ERROR_SUCCESS;

bailout:

    DEBUG_PRINT( CACHE, INFO,
        ("WriteCache: _CacheFileSize=%d, inputsize=%d, dwError=%d\n",
        _VirtualCacheFileSize, BufferLen, dwError));

    return (dwError);
}


DWORD
INTERNET_CONNECT_HANDLE_OBJECT::WriteToDisk(
    LPBYTE Buffer,
    DWORD BufferLen,
    LPDWORD lpdwBytesWritten
    )
{
    BOOL BoolError;

    BoolError = WriteFile(
                    _CacheFileHandle,
                    Buffer,
                    BufferLen,
                    lpdwBytesWritten,
                    NULL );
    if( !BoolError ) {
        return( GetLastError() );
    }

    _RealCacheFileSize += *lpdwBytesWritten;

    return (ERROR_SUCCESS);
}


DWORD
INTERNET_CONNECT_HANDLE_OBJECT::EndCacheWrite(
    FILETIME    *lpftExpireTime,
    FILETIME    *lpftLastModifiedTime,
    FILETIME    *lpftPostCheckTime,
    DWORD       dwCacheEntryType,
    DWORD       dwHeaderLen,
    LPSTR       lpHeaderInfo,
    LPSTR       lpszFileExtension,
    BOOL        fImage
    )
{
    LPBYTE lpBuff;
    DWORD  dwBytesWritten, dwUsed;
    FILETIME ftCreate;

    DEBUG_ENTER((DBG_CACHE,
                 Dword,
                 "INTERNET_CONNECT_HANDLE_OBJECT::EndCacheWrite",
                 "{%q} %#x, %#x,, %#x, %d, %d, %.32q",
                 _CacheUrlName,
                 lpftExpireTime,
                 lpftLastModifiedTime,
                 lpftPostCheckTime,
                 dwCacheEntryType,
                 dwHeaderLen,
                 lpHeaderInfo
                 ));

    DWORD Error = ERROR_NOT_SUPPORTED;

    INET_ASSERT( _CacheUrlName != NULL );
    INET_ASSERT( _CacheFileName != NULL );
    INET_ASSERT( _CacheReadInProgress == FALSE );
    INET_ASSERT( _CacheWriteInProgress == TRUE );
    INET_ASSERT( _CacheFileHandle != INVALID_HANDLE_VALUE );

    //
    // close the file.
    //

    if( _CacheFileHandle != INVALID_HANDLE_VALUE ) {

        GetFileTime( _CacheFileHandle, &ftCreate, NULL, NULL );
        CloseHandle( _CacheFileHandle );

        _CacheFileHandle = INVALID_HANDLE_VALUE;

    } else {

        DEBUG_PRINT(CACHE,
                    ERROR,
                    ("_CacheFileHandle = %x\n",
                    _CacheFileHandle
                    ));

    }

    if( _CacheFileHandleRead != INVALID_HANDLE_VALUE ) {
        CloseHandle( _CacheFileHandleRead );
        _CacheFileHandleRead = INVALID_HANDLE_VALUE;
    }

    //
    // Cache the file.
    //

    if( (_CacheUrlName != NULL) && (_CacheFileName != NULL) ) {

        //
        // if the cache file is successfully made, cache it, otherwise
        // mark it for deletion.
        //

        if( dwCacheEntryType == 0xffffffff ) {


            _fDeleteDataFile = TRUE;
        }

        if (!_fDeleteDataFile)
        {
            if (((GetHandleType() == TypeFtpConnectHandle) ||
                 (GetHandleType() == TypeFtpFileHandle) ||
                 (GetHandleType() == TypeFtpFileHandleHtml))
                 && IsPerUserItem())
            {
                char buff[256];
                DEBUG_PRINT(CACHE, 
                            INFO,
                            ("EndCacheWrite():FTP:PerUserItem = TRUE\n")
                            //("EndCacheWrite():PerUserItem = TRUE: <pConnect = 0x%x>.\n",pConnect)
                            );
                INET_ASSERT(vdwCurrentUserLen);

                // Store the total length to get copied to the args for AddUrl
                dwHeaderLen = sizeof(vszUserNameHeader) - 1
                                 + vdwCurrentUserLen
                                 + sizeof("\r\n");
                if (sizeof(buff) >= dwHeaderLen)
                {
                    memcpy(buff, vszUserNameHeader, sizeof(vszUserNameHeader) - 1);

                    DWORD dwSize = lstrlen(vszCurrentUser);
                    memcpy(&buff[sizeof(vszUserNameHeader) - 1],
                           vszCurrentUser,
                           dwSize);
                    dwSize += sizeof(vszUserNameHeader) - 1;
                    memcpy(&buff[dwSize], "\r\n", sizeof("\r\n"));

                    // Copy over to lpHeaderInfo which gets copied into the args for AddUrl
                    lpHeaderInfo = buff;
                    DEBUG_PRINT(CACHE, 
                                INFO,
                                ("EndCacheWrite():FTP: lpHeaderInfo = %q dwHeaderLen = %d\n",
                                lpHeaderInfo, dwHeaderLen)
                                );
                }
                else
                {
                    // if it failed, mark it as expired
/*
                    dwUserNameHeader = 0;
                    GetCurrentGmtTime(&_ftExpires);
                    *(LONGLONG *)&_ftExpires -= ONE_HOUR_DELTA;
*/              }
                                
            }
            else
            {
                DEBUG_PRINT(CACHE, 
                            INFO,
                            ("EndCacheWrite():FTP:PerUserItem = FALSE\n")
                            );

            }
        
            AddUrlArg Args;
            Args.pszUrl      = _CacheUrlName;
            Args.pszFilePath = _CacheFileName;
            Args.dwFileSize  = _RealCacheFileSize;
            Args.qwExpires   = *((LONGLONG*)lpftExpireTime);
            Args.qwLastMod   = *((LONGLONG*)lpftLastModifiedTime);
            Args.qwPostCheck = *((LONGLONG*)lpftPostCheckTime);
            Args.ftCreate    = ftCreate;
            Args.dwEntryType = dwCacheEntryType;
            Args.pbHeaders   = lpHeaderInfo;
            Args.cbHeaders   = dwHeaderLen;
            Args.pszFileExt  = lpszFileExtension;
            Args.pszRedirect = _OriginalUrl;
            Args.fImage      = fImage;

            Error = UrlCacheCommitFile(&Args);

            if (Error != ERROR_SUCCESS)
            {
                DEBUG_PRINT(CACHE,
                            ERROR,
                            ("CommitUrlCacheEntry(%q) failed\n",
                            _CacheUrlName
                            ));

                _fDeleteDataFile = TRUE;

                if (Error == ERROR_SHARING_VIOLATION) {

                    // we got new URL data, but the old one is in use.
                    // expire it, so any new user's will go to the net

                    ExpireUrl();

                }
            }
        }
    }

    _CacheWriteInProgress = FALSE;

    DEBUG_LEAVE(Error);

    return( Error );
}

BOOL
INTERNET_CONNECT_HANDLE_OBJECT::ExpireDependents(VOID
    )
{
    char szUrlParent[INTERNET_MAX_URL_LENGTH];
    BOOL fRet = FALSE;

    ExpireUrl();

    if (GetCanonicalizedParentUrl( _CacheUrlName,
                                       szUrlParent,
                                       sizeof(szUrlParent))){

        ExpireUrl(szUrlParent);

        fRet = TRUE;

    }
    return(fRet);

}


#ifdef LAZY_WRITE

LPBYTE
INTERNET_CONNECT_HANDLE_OBJECT::GetCacheScratchBuf(
    LPDWORD Length, LPDWORD lpdwUsed
    )
/*++

Routine Description:

    Get existing scratch buffer for use.

Arguments:

    Length : pointer to a location where the buffer length is returned.

Return Value:

    return scratch buffer pointer.

--*/
{
    //
    // no one else is using this buffer.
    //


    if(( _CacheScratchBuf != NULL )||(Length==NULL)) {


        INET_ASSERT(!((_CacheScratchBuf == NULL)&&(_CacheScratchUsedLen != 0)));

        if (Length) {

            *Length = _CacheScratchBufLen;

        }

        *lpdwUsed = _CacheScratchUsedLen;

        return( _CacheScratchBuf );
    }

    INET_ASSERT( _CacheScratchBufLen == 0 );

    //
    // create a default buffer.
    //

    *lpdwUsed = _CacheScratchUsedLen = 0;

    _CacheScratchBufLen = dwCacheWriteBufferSize;    //  default size;

    INET_ASSERT(dwCacheWriteBufferSize >= 4096);

    _CacheScratchBuf = (LPBYTE)ALLOCATE_MEMORY(LMEM_FIXED | LMEM_ZEROINIT,
                                          _CacheScratchBufLen
                                          );

    if( _CacheScratchBuf == NULL ) {

        //
        // we couldn't make one.
        //

        _CacheScratchBufLen = 0;


        *Length = 0;

        return( NULL );
    }

    *Length = _CacheScratchBufLen;

    return( _CacheScratchBuf );
}

#endif // LAZY_WRITE

BOOL
GetCanonicalizedParentUrl(
    LPSTR   lpszChildUrl,
    LPSTR   lpszParentUrlBuff,
    DWORD   dwBuffSize)
{

    char szUrlT[INTERNET_MAX_URL_LENGTH];
    LPSTR lpT;
    BOOL fRet = FALSE;
    DWORD dwT = dwBuffSize;

    lstrcpy(szUrlT, lpszChildUrl);

    lpT = szUrlT+lstrlen(szUrlT);

    if ( lpT > szUrlT ) {

        --lpT;
        if (*lpT == '/') {
            --lpT;
        }

        for(; lpT >= szUrlT; --lpT){

            if (*lpT == '/') {

                *(lpT+1) = 0;

                if(InternetCanonicalizeUrl(szUrlT, lpszParentUrlBuff, &dwT, 0)) {

                    fRet = TRUE;

                }

                goto done;
            }

        }
    }

done:

    return (fRet);

}


VOID
INTERNET_CONNECT_HANDLE_OBJECT::AttachLastResponseInfo(
    VOID
    )

/*++

Routine Description:

    Called when we are performing an FTP URL operation & we want to display
    the welcome message contained in the last response info as part of the
    generated HTML. We need to keep hold of it in this object in case the
    app calls an API which wipes out the last response info before getting
    the HTML data

Arguments:

    None.

Return Value:

    None.

--*/

{
    LPSTR buffer = NULL;
    DWORD bufferLength = 0;
    DWORD category;
    BOOL ok = InternetGetLastResponseInfo(&category,
                                          buffer,
                                          &bufferLength
                                          );
    if (!ok && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        buffer = (LPSTR)ResizeBuffer(NULL, bufferLength, FALSE);
        if (buffer != NULL) {
            ok = InternetGetLastResponseInfo(&category,
                                             buffer,
                                             &bufferLength
                                             );
            if (ok) {
                SetLastResponseInfo(buffer, bufferLength);
            } else {
                (void)ResizeBuffer((HLOCAL)buffer, 0, FALSE);
            }
        }
    }
}


VOID
INTERNET_CONNECT_HANDLE_OBJECT::SetOriginServer(
    IN CServerInfo * pServerInfo
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    pServerInfo -

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_CONNECT_HANDLE_OBJECT::SetOriginServer",
                 "%#x{%q}",
                 pServerInfo,
                 pServerInfo ? pServerInfo->GetHostName() : ""
                 ));

    if (_OriginServer == NULL) {
        _OriginServer = pServerInfo;
        if (pServerInfo != NULL) {
            pServerInfo->Reference();
        }
    }

    DEBUG_LEAVE(0);
}


DWORD
INTERNET_CONNECT_HANDLE_OBJECT::SetServerInfo(
    IN INTERNET_SCHEME tScheme,
    IN BOOL bDoResolution,
    IN OPTIONAL BOOL fNtlm
    )

/*++

Routine Description:

    Associates a SERVER_INFO with this INTERNET_CONNECT_HANDLE_OBJECT based on
    the host name for which this object was created and an optional scheme
    type

Arguments:

    tScheme         - scheme type we want SERVER_INFO for

    bDoResolution   - TRUE if we are to resolve the host name if creating a new
                      SERVER_INFO object

    fNtlm           - TRUE if we are tunnelling for NTLM

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 Dword,
                 "INTERNET_CONNECT_HANDLE_OBJECT::SetServerInfo",
                 "%s (%d), %B, %B",
                 InternetMapScheme(tScheme),
                 tScheme,
                 bDoResolution,
                 fNtlm
                 ));

    //INTERNET_SCHEME proxyScheme = INTERNET_SCHEME_DEFAULT;
    //INTERNET_HANDLE_OBJECT * lpParent = (INTERNET_HANDLE_OBJECT *)GetParent();
    //
    //INET_ASSERT(lpParent != NULL);
    //
    ////
    //// this may be called from an INTERNET_CONNECT_HANDLE_OBJECT within a
    //// derived handle (HTTP_REQUEST_HANDLE_OBJECT), in which case we need to go
    //// one level higher to the INTERNET_HANDLE_OBJECT
    ////
    //
    //if (lpParent->GetHandleType() != TypeInternetHandle) {
    //    lpParent = (INTERNET_HANDLE_OBJECT *)lpParent->GetParent();
    //
    //    INET_ASSERT(lpParent != NULL);
    //    INET_ASSERT(lpParent->GetHandleType() == TypeInternetHandle);
    //
    //}

    if (_ServerInfo != NULL) {
        ::ReleaseServerInfo(_ServerInfo);
    }

    //
    // use the base service type to find the server info
    //

//dprintf("getting server info for %q (current = %q)\n", hostName, GetHostName());
    DWORD error = ::GetServerInfo(GetHostName(),
                                  _ServiceType,
                                  bDoResolution,
                                  &_ServerInfo
                                  );

    ////
    //// if _ServerInfo is NULL then we didn't find a SERVER_INFO and couldn't
    //// create one, therefore we must be out of memory
    ////
    //
    //if (_ServerInfo != NULL) {
    //    if (proxyScheme == INTERNET_SCHEME_HTTP) {
    //        _ServerInfo->SetCernProxy();
    //    } else if (proxyScheme == INTERNET_SCHEME_FTP) {
    //        _ServerInfo->SetFTPProxy();
    //    }
    //
    //    INET_ASSERT(error == ERROR_SUCCESS);
    //
    //} else {
    //
    //    INET_ASSERT(error != ERROR_SUCCESS);
    //
    //}

    DEBUG_LEAVE(error);

    return error;
}


DWORD
INTERNET_CONNECT_HANDLE_OBJECT::SetServerInfo(
    IN LPSTR lpszServerName,
    IN DWORD dwServerNameLength
    )

/*++

Routine Description:

    Associates a SERVER_INFO with this INTERNET_CONNECT_HANDLE_OBJECT based on
    the host name in the parameters

Arguments:

    lpszServerName      - name of server

    dwServerNameLength  - length of lpszServerName

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 Dword,
                 "INTERNET_CONNECT_HANDLE_OBJECT::SetServerInfo",
                 "%q, %d",
                 lpszServerName,
                 dwServerNameLength
                 ));

    if (_ServerInfo != NULL) {
        ::ReleaseServerInfo(_ServerInfo);
    }

    //
    // use the base service type to find the server info
    //

    char hostName[INTERNET_MAX_HOST_NAME_LENGTH + 1];
    int copyLength = min(sizeof(hostName) - 1, dwServerNameLength);

    memcpy(hostName, lpszServerName, copyLength);
    hostName[copyLength] = '\0';

    DWORD error = ::GetServerInfo(hostName,
                                  _ServiceType,
                                  FALSE,
                                  &_ServerInfo
                                  );

    DEBUG_LEAVE(error);

    return error;
}

BOOL INTERNET_CONNECT_HANDLE_OBJECT::GetUserAndPass (BOOL fProxy, LPSTR *pszUser, LPSTR *pszPass)
{
    // How the credentials (username + password) are retrieved from handles during
    // authentication (see AuthOnRequest)
    //
    //             Connect      Request
    //  Server        4     <-     3
    //  Proxy         2     <-     1
    //
    //
    //
    // When credentials are transferred from the handle to the password cache, they
    // are invalidated on the handle for internal calls from wininet so that they
    // are not inadvertently used therafter. The handle credentials are maintained
    // for external apps which expect these values to be available via InternetQueryOption.
    // When GetUserAndPass is called, if a credential is found on the handle its validity is
    // checked for internal calls. If no credential is found or the credential is no longer
    // valid GetUserAndPass is called recursively on the parent connect handle if it exists.
    //
    // When transferring credentials from a handle to the password cache it is IMPORTANT
    // GetUserAndPass is called to invalidate the credentials. The credentials (both username
    // and password) are re-validated as a pair if either of them is reset via SetUserOrPass.

    if (fProxy)
    {
        // If proxy credentials are valid and exist invalidate and return.
        if (_HandleFlags.fProxyUserPassValid
            && _xsProxyUser.GetPtr()
            && _xsProxyPass.GetPtr())
        {
            *pszUser = _xsProxyUser.GetPtr();
            *pszPass = _xsProxyPass.GetPtr();
            _HandleFlags.fProxyUserPassValid = FALSE;
            return TRUE;
        }
    }
    else
    {
        // If server credentials are valid and exist, invalidate and return.
        if (_HandleFlags.fServerUserPassValid
            && _xsUser.GetPtr()
            && _xsPass.GetPtr())
        {
            *pszUser = _xsUser.GetPtr();
            *pszPass = _xsPass.GetPtr();
            _HandleFlags.fServerUserPassValid = FALSE;
            return TRUE;
        }
    }


    // Either credentials not found or are invalid on this handle.
    // Walk up to any existing connect handle and repeat call.
    if (GetHandleType() == TypeHttpRequestHandle)
    {
        INTERNET_CONNECT_HANDLE_OBJECT * pConnect =
            (INTERNET_CONNECT_HANDLE_OBJECT *) GetParent();

        return pConnect->GetUserAndPass (fProxy, pszUser, pszPass);
    }

    // Connect handle returns FALSE and null values if none/invalid.
    *pszUser = *pszPass = NULL;
    return FALSE;
}
