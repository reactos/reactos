/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    hinet.cxx

Abstract:

    contains methods for INTERNET_HANDLE_OBJECT class

    Contents:
        ContainingHandleObject
        CancelActiveSyncRequests
        HANDLE_OBJECT::HANDLE_OBJECT()
        HANDLE_OBJECT::HANDLE_OBJECT()
        HANDLE_OBJECT::Reference()
        HANDLE_OBJECT::Dereference()
        HANDLE_OBJECT::IsValid()
        INTERNET_HANDLE_OBJECT::INTERNET_HANDLE_OBJECT(LPCSTR, ...)
        INTERNET_HANDLE_OBJECT::INTERNET_HANDLE_OBJECT(INTERNET_HANDLE_OBJECT*)
        INTERNET_HANDLE_OBJECT::~INTERNET_HANDLE_OBJECT()
        INTERNET_HANDLE_OBJECT::SetAbortHandle(ICSocket)
        INTERNET_HANDLE_OBJECT::ResetAbortHandle()
        INTERNET_HANDLE_OBJECT::AbortSocket()
        INTERNET_HANDLE_OBJECT::CheckGlobalProxyUpdated()
        INTERNET_HANDLE_OBJECT::SetProxyInfo()
        INTERNET_HANDLE_OBJECT::GetProxyInfo(LPVOID, LPDWORD)
        INTERNET_HANDLE_OBJECT::GetProxyInfo(INTERNET_SCHEME, LPINTERNET_SCHEME, LPSTR *, LPDWORD, LPINTERNET_PORT)
        INTERNET_HANDLE_OBJECT::IndicateStatus()

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:

   Sophia Chung (sophiac) 14-Feb-1995 (added FTP and Archie class impl.)
   (code adopted from madana)

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include "autodial.h"

//
// private manifests
//

#define PROXY_REGISTRY_STRING_LENGTH    (4 K)

//
// functions
//


HANDLE_OBJECT *
ContainingHandleObject(
    IN LPVOID lpAddress
    )

/*++

Routine Description:

    Returns address of containing HANDLE_OBJECT from address of _List member

Arguments:

    lpAddress   - address of _List in this object

Return Value:

    HANDLE_OBJECT *

--*/

{
    return CONTAINING_RECORD(lpAddress, HANDLE_OBJECT, _List);
}


VOID
CancelActiveSyncRequests(
    IN DWORD dwError
    )

/*++

Routine Description:

    For all currently active synchronous requests, cancels them with the error
    code supplied

Arguments:

    dwError - error code to complete requests

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_INET,
                 None,
                 "CancelActiveSyncRequests",
                 "%s",
                 InternetMapError(dwError)
                 ));

    LockSerializedList(&GlobalObjectList);

    for (PLIST_ENTRY pEntry = (PLIST_ENTRY)HeadOfSerializedList(&GlobalObjectList);
         pEntry != (PLIST_ENTRY)SlSelf(&GlobalObjectList);
         pEntry = pEntry->Flink) {

        HANDLE_OBJECT * pObject = ContainingHandleObject(pEntry);
        HINTERNET_HANDLE_TYPE objectType = pObject->GetObjectType();

        //
        // check handle types in decreasing order of expectation for IE
        //

        if ((objectType == TypeHttpRequestHandle)
        || (objectType == TypeFtpFindHandleHtml)
        || (objectType == TypeFtpFindHandle)
        || (objectType == TypeFtpFileHandle)
        || (objectType == TypeGopherFindHandleHtml)
        || (objectType == TypeGopherFindHandle)
        || (objectType == TypeGopherFileHandle)) {

            //
            // all these handle types are descended from INTERNET_HANDLE_OBJECT
            // which in turn is descended from HANDLE_OBJECT
            //

            if (!((INTERNET_HANDLE_OBJECT *)pObject)->IsAsyncHandle()) {

                //
                // sync request
                //

                DEBUG_PRINT(INET,
                            INFO,
                            ("cancelling %s sync request on handle %#x (%#x) \n",
                            InternetMapHandleType(objectType),
                            pObject->GetPseudoHandle(),
                            pObject
                            ));

                pObject->InvalidateWithError(dwError);
            }
        }
    }

    UnlockSerializedList(&GlobalObjectList);

    DEBUG_LEAVE(0);
}

//
// methods
//


HANDLE_OBJECT::HANDLE_OBJECT(
    IN HANDLE_OBJECT * Parent
    )

/*++

Routine Description:

    HANDLE_OBJECT constructor

Arguments:

    Parent  - pointer to parent HANDLE_OBJECT

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                None,
                "HANDLE_OBJECT",
                "%#x",
                this
                ));

    //InitializeListHead(&_List);
    InitializeSerializedList(&_Children);
    //InitializeListHead(&_Siblings);
    _Parent = Parent;
    if (_Parent != NULL) {
        _Parent->AddChild(&_Siblings);
    } else {
        InitializeListHead(&_Siblings);
    }
    _DeleteWithChild = FALSE;
    _Status = AllocateHandle(this, &_Handle);
    _ObjectType = TypeGenericHandle;
    _ReferenceCount = 1;
    _Invalid = FALSE;
    _Error = ERROR_SUCCESS;
    _Signature = OBJECT_SIGNATURE;
    _Context = INTERNET_NO_CALLBACK;
    InsertAtTailOfSerializedList(&GlobalObjectList, &_List);

    //
    // if AllocateHandle() failed then we cannot create this handle object.
    // Invalidate it ready for the destructor
    //

    if (_Status != ERROR_SUCCESS) {
        _Invalid = TRUE;
        _ReferenceCount = 0;
    }

    DEBUG_PRINT(OBJECTS,
                INFO,
                ("handle %#x created; address %#x; %d objects\n",
                _Handle,
                this,
                ElementsOnSerializedList(&GlobalObjectList)
                ));

    DEBUG_LEAVE(0);
}


HANDLE_OBJECT::~HANDLE_OBJECT(VOID)

/*++

Routine Description:

    HANDLE_OBJECT destructor. Virtual function

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                None,
                "~HANDLE_OBJECT",
                "%#x",
                this
                ));

    //
    // remove this object from global object list
    //

    LockSerializedList(&GlobalObjectList);
    RemoveFromSerializedList(&GlobalObjectList, &_List);
    if (IsSerializedListEmpty(&GlobalObjectList)) {
        OnLastHandleDestroyed();
    }
    UnlockSerializedList(&GlobalObjectList);

    INET_DEBUG_ASSERT((_List.Flink == NULL) && (_List.Blink == NULL));

    //
    // inform the app that this handle is completely closed, but only if we
    // can make callbacks at all
    //

    if (_Context != INTERNET_NO_CALLBACK) {

        LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

        HINTERNET hCurrent = _InternetGetObjectHandle(lpThreadInfo);
        HINTERNET hCurrentMapped = _InternetGetMappedObjectHandle(lpThreadInfo);
        DWORD_PTR currentContext = _InternetGetContext(lpThreadInfo);

        _InternetSetObjectHandle(lpThreadInfo, _Handle, (HINTERNET)this);
        _InternetSetContext(lpThreadInfo, _Context);

        InternetIndicateStatus(INTERNET_STATUS_HANDLE_CLOSING,
                               (LPVOID)&_Handle,
                               sizeof(_Handle)
                               );

        _InternetSetObjectHandle(lpThreadInfo, hCurrent, hCurrentMapped);
        _InternetSetContext(lpThreadInfo, currentContext);
    } else {

        DEBUG_PRINT(OBJECTS,
                    WARNING,
                    ("handle %#x [%#x] no context: no callback\n",
                    _Handle,
                    this
                    ));

    }

    //
    // remove object from parent's child list (if we have a parent object)
    //

    if (_Parent != NULL) {
        _Parent->RemoveChild(&_Siblings);

        INET_DEBUG_ASSERT((_Siblings.Flink == NULL) && (_Siblings.Blink == NULL));

    }

    //
    // now we can free up the API handle value
    //

    if (_Handle != NULL) {
        _Status = FreeHandle(_Handle);

        INET_ASSERT(_Status == ERROR_SUCCESS);

    }

    //
    // there should be no child objects
    //

    INET_ASSERT(IsSerializedListEmpty(&_Children));

    TerminateSerializedList(&_Children);

    //
    // set the signature to a value that indicates the handle has been
    // destroyed (not useful in debug builds)
    //

    _Signature = DESTROYED_OBJECT_SIGNATURE;

    INET_ASSERT((_ReferenceCount == 0) && _Invalid);

    DEBUG_PRINT(OBJECTS,
                INFO,
                ("handle %#x destroyed; type %s; address %#x; %d objects\n",
                _Handle,
                InternetMapHandleType(_ObjectType),
                this,
                ElementsOnSerializedList(&GlobalObjectList)
                ));

    DEBUG_LEAVE(0);
}


DWORD
HANDLE_OBJECT::Reference(
    VOID
    )

/*++

Routine Description:

    Increases the reference count on the HANDLE_OBJECT

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE
                    Handle has already been invalidated
                  ERROR_ACCESS_DENIED
                    Handle object is being destroyed, cannot use it

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 Dword,
                 "HANDLE_OBJECT::Reference",
                 "{%#x}",
                 _Handle
                 ));

    DWORD error;

    if (_Invalid) {

        DEBUG_PRINT(OBJECTS,
                    INFO,
                    ("handle object %#x [%#x] is invalid\n",
                    _Handle,
                    this
                    ));

        error = ERROR_INVALID_HANDLE;
    } else {
        error = ERROR_SUCCESS;
    }

    //
    // even if the handle has been invalidated (i.e. closed), we allow it
    // to continue to be referenced. The caller should return the fact
    // that the handle has been invalidated, but may require information
    // from the object in order to do so (e.g. in async thread)
    //

    do
    {
        LONG lRefCountBeforeIncrement = _ReferenceCount;

        //
        // refcount is > 0 means that the object's destructor has not been called yet
        //
        if (lRefCountBeforeIncrement > 0)
        {
            //
            // try to increment the refcount using compare-exchange
            //
#ifndef _WIN64
            LONG lRefCountCurrent = (LONG)SHInterlockedCompareExchange((LPVOID*)&_ReferenceCount,
                                                                       (LPVOID)(lRefCountBeforeIncrement + 1),
                                                                       (LPVOID)lRefCountBeforeIncrement);
#else
            //
            // can't use SHInterlockedCompareExchange on win64 because the values are really LONG's (32-bits) but they
            // are treated as pointers (64-bits) because SHInterlockedCompareExchange should really be called 
            // SHInterlockedCompareExchangePointer (sigh...).
            //
            LONG lRefCountCurrent = InterlockedCompareExchange(&_ReferenceCount,
                                                               lRefCountBeforeIncrement + 1,
                                                               lRefCountBeforeIncrement);
#endif        
            if (lRefCountCurrent == lRefCountBeforeIncrement)
            {
                //
                // since SHInterlockedCompareExchange returns the value in _ReferenceCount 
                // before the exchange, we know the exchange sucessfully took place (i.e. we 
                // sucessfully incremented the refrence count of the object by one)
                //
                INET_ASSERT(lRefCountCurrent > 0);
                break;
            }
        }
        else
        {
            //
            // the refcount dropped to zero before we could increment it,
            // so the object is being destroyed. 
            //
            error = ERROR_ACCESS_DENIED;
            break;
        }

    } while (TRUE);

    DEBUG_PRINT(REFCOUNT,
                INFO,
                ("handle object %#x [%#x] ReferenceCount = %d\n",
                _Handle,
                this,
                _ReferenceCount
                ));

    DEBUG_LEAVE(error);

    return error;
}


BOOL
HANDLE_OBJECT::Dereference(
    VOID
    )

/*++

Routine Description:

    Reduces the reference count on the HANDLE_OBJECT, and if it goes to zero,
    the object is deleted

Arguments:

    None.

Return Value:

    BOOL
        TRUE    - this object was deleted

        FALSE   - this object is still valid

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 Bool,
                 "HANDLE_OBJECT::Dereference",
                 "{%#x}",
                 _Handle
                 ));

    //
    // by the time we get here, the reference count should not be 0. There
    // should be 1 call to Dereference() for each call to Reference()
    //

    INET_ASSERT(_ReferenceCount != 0);

    BOOL deleted = FALSE;

    if (InterlockedDecrement(&_ReferenceCount) == 0)
    {
        deleted = TRUE;
    }


    if (deleted)
    {
        //
        // if we are calling the destructor, the handle had better be invalid!
        //
        INET_ASSERT(_Invalid);
        
        //
        // this handle has now been closed. If there is no activity on it
        // then it will be destroyed
        //

        DEBUG_PRINT(REFCOUNT,
                    INFO,
                    ("handle object %#x [%#x] ReferenceCount = %d\n",
                    _Handle,
                    this,
                    _ReferenceCount
                    ));

        delete this;
    } else {

        DEBUG_PRINT(REFCOUNT,
                    INFO,
                    ("handle object %#x [%#x] ReferenceCount = %d\n",
                    _Handle,
                    this,
                    _ReferenceCount
                    ));
    }

    DEBUG_LEAVE(deleted);

    return deleted;
}


DWORD
HANDLE_OBJECT::IsValid(
    IN HINTERNET_HANDLE_TYPE ExpectedHandleType
    )

/*++

Routine Description:

    Checks a HANDLE_OBJECT for validity

Arguments:

    ExpectedHandleType  - type of object we are testing for. Can be
                          TypeWildHandle which matches any valid handle

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE
                    The handle object is invalid

                  ERROR_INTERNET_INCORRECT_HANDLE_TYPE
                    The handle object is valid, but not the type we want

--*/

{
    DWORD error;
    BOOL IsOkHandle = TRUE;

    //
    // test handle object within try..except in case we are given a bad address
    //

    __try {
        if (_Signature == OBJECT_SIGNATURE) {

            error = ERROR_SUCCESS;

            //
            // check handle type if we are asked to do so.
            //

            if (ExpectedHandleType != TypeWildHandle) {
                if (ExpectedHandleType != this->GetHandleType()) {
                    error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
                }
            }
        } else {
            error = ERROR_INVALID_HANDLE;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        error = ERROR_INVALID_HANDLE;
    }
    ENDEXCEPT
    return error;
}


INTERNET_HANDLE_OBJECT::INTERNET_HANDLE_OBJECT(
    LPCSTR UserAgent,
    DWORD AccessMethod,
    LPSTR ProxyServerList,
    LPSTR ProxyBypassList,
    DWORD Flags
    ) : HANDLE_OBJECT(NULL)

/*++

Routine Description:

    Creates the handle object for InternetOpen()

Arguments:

    UserAgent       - name of agent (user-agent string for HTTP)

    AccessMethod    - DIRECT, PROXY or PRECONFIG

    ProxyServerList - one or more proxy servers. The string has the form:

                        [<scheme>=][<scheme>"://"]<server>[":"<port>][";"*]

    ProxyBypassList - zero or more addresses which if matched will result in
                      requests NOT going via the proxy (only if PROXY access).
                      The string has the form:

                        bp_entry ::= [<scheme>"://"]<server>[":"<port>]
                        bp_macro ::= "<local>"
                        bp_list ::= [<> | bp_entry bp_macro][";"*]

    Flags           - various open flags:

                        INTERNET_FLAG_ASYNC

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_HANDLE_OBJECT::INTERNET_HANDLE_OBJECT",
                 NULL
                 ));


    //
    // if the HANDLE_OBJECT constructor failed then bail out now
    //

    if (_Status != ERROR_SUCCESS) {

        DEBUG_PRINT(OBJECTS,
                    ERROR,
                    ("early-exit: _Status = %d\n",
                    _Status
                    ));

        DEBUG_LEAVE(0);

        return;
    }

    //
    // BUGBUG - remove _INetHandle
    //

    _INetHandle = INET_INVALID_HANDLE_VALUE;

    _IsCopy = FALSE;
    _UserAgent = (LPSTR)UserAgent;
    _ProxyInfo = NULL;
    _dwInternetOpenFlags = Flags;
    _WinsockLoaded = FALSE;

    //
    // BUGBUG - post-beta: move to HANDLE_OBJECT
    //

    _Context = INTERNET_NO_CALLBACK;

    //
    // initialize the timeout/retry values for this object from the
    // global (DLL) values
    //

    _ConnectTimeout = GlobalConnectTimeout;
    _ConnectRetries = GlobalConnectRetries;
    _SendTimeout = GlobalSendTimeout;
    _DataSendTimeout = GlobalDataSendTimeout;
    _ReceiveTimeout = GlobalReceiveTimeout;
    _DataReceiveTimeout = GlobalDataReceiveTimeout;
    _FromCacheTimeout = GlobalFromCacheTimeout;
    _SocketSendBufferLength = GlobalSocketSendBufferLength;
    _SocketReceiveBufferLength = GlobalSocketReceiveBufferLength;

    //
    // set _Async based on the INTERNET_FLAG_ASYNC supplied to InternetOpen()
    //

    _Async = (Flags & INTERNET_FLAG_ASYNC) ? TRUE : FALSE;

    //
    // no data available yet
    //

    SetAvailableDataLength(0);

    //
    // not yet end of file
    //

    ResetEndOfFile();

    //
    // no status callback by default
    //

    _StatusCallback = NULL;
    _StatusCallbackType = FALSE;

    //
    // the number of pending async requests is 0. The clash test variable is
    // used to test for ownership using InterlockedIncrement()
    //

    //
    // BUGBUG - RLF 03/16/98. See hinet.hxx
    //

    //_PendingAsyncRequests = 0;
    //_AsyncClashTest = -1;

    InitializeCriticalSection(&_UiCritSec);
    _dwUiBlocked = FALSE;
    SetObjectType(TypeInternetHandle);

    _ProxyInfoResourceLock.Initialize();

    _Status = SetProxyInfo(AccessMethod, ProxyServerList, ProxyBypassList);

    //
    // if _pICSocket is not NULL then this is the socket that this object handle
    // is currently working on. We close it to cancel the operation
    //

    _pICSocket = NULL;

    //
    // load winsock now. We always want to go via winsock since the demise of
    // catapult
    //

    if (_Status == ERROR_SUCCESS) {
        _INetHandle = LOCAL_INET_HANDLE;
        _Status = LoadWinsock();
        _WinsockLoaded = (_Status == ERROR_SUCCESS);

        if ( _Status == ERROR_SUCCESS )
        {
             LONG lOpenHandleCnt;

             LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

             if ( lpThreadInfo && ! lpThreadInfo->IsAutoProxyProxyThread )
             {
                 lOpenHandleCnt = InterlockedIncrement((LPLONG)&GlobalInternetOpenHandleCount);

                 if ( lOpenHandleCnt == 0 )
                 {
                    DWORD fAlreadyInInit = (DWORD) InterlockedExchange((LPLONG) &GlobalAutoProxyInInit, TRUE);

                    INET_ASSERT (! fAlreadyInInit );

                    GlobalProxyInfo.ReleaseQueuedRefresh();

                    InterlockedExchange((LPLONG)&GlobalAutoProxyInInit, FALSE);
                 }
             }
        }
    }

    DEBUG_LEAVE(0);
}


INTERNET_HANDLE_OBJECT::INTERNET_HANDLE_OBJECT(
    INTERNET_HANDLE_OBJECT *INetObj
    ) : HANDLE_OBJECT((HANDLE_OBJECT*)INetObj)

/*++

Routine Description:

    Constructor for derived handle object. We are creating this handle as part
    of an INTERNET_CONNECT_HANDLE_OBJECT

Arguments:

    INetObj - pointer to INTERNET_HANDLE_OBJECT to copy

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_HANDLE_OBJECT::INTERNET_HANDLE_OBJECT",
                 "{IsCopy}"
                 ));

    _INetHandle = INetObj->_INetHandle;
    _IsCopy = TRUE;

    //
    // copy user agent string
    //

    //
    // BUGBUG - compiler generated copy constructor (no new string)
    //

    _UserAgent = INetObj->_UserAgent;

    //
    // do not inherit the proxy info - code must go to parent handle
    //

    _ProxyInfo = NULL;

    _dwInternetOpenFlags = INetObj->_dwInternetOpenFlags;

    //
    // creating this handle didn't load winsock
    //

    _WinsockLoaded = FALSE;

    //
    // inherit the context, timeout values, async flag and status callback from
    // the parent object handle
    //

    _Context = INetObj->_Context;

    _ConnectTimeout = INetObj->_ConnectTimeout;
    _ConnectRetries = INetObj->_ConnectRetries;
    _SendTimeout = INetObj->_SendTimeout;
    _DataSendTimeout = INetObj->_DataSendTimeout;
    _ReceiveTimeout = INetObj->_ReceiveTimeout;
    _DataReceiveTimeout = INetObj->_DataReceiveTimeout;
    _FromCacheTimeout = INetObj->_FromCacheTimeout;

    //
    // inherit the async I/O mode and callback function
    //

    _Async = INetObj->_Async;
    SetAvailableDataLength(0);
    ResetEndOfFile();
    _StatusCallback = INetObj->_StatusCallback;
    _StatusCallbackType = INetObj->_StatusCallbackType;

    //
    // this is a new object: we need a new pending async request count and clash
    // test variable
    //

    //
    // BUGBUG - RLF 03/16/98. See hinet.hxx
    //

    //_PendingAsyncRequests = 0;
    //_AsyncClashTest = -1;

    //
    // no socket operation to abort yet
    //

    _pICSocket = NULL;

    //
    // BUGBUG - this overwrites status set above?
    //

    _Status = INetObj->_Status;
    _dwUiBlocked = FALSE;

    DEBUG_LEAVE(0);
}


INTERNET_HANDLE_OBJECT::~INTERNET_HANDLE_OBJECT(
    VOID
    )

/*++

Routine Description:

    INTERNET_HANDLE_OBJECT destructor

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_HANDLE_OBJECT::~INTERNET_HANDLE_OBJECT",
                 ""
                 ));

    //
    // if this handle is not a copy then delete proxy information if we are not
    // using the global proxy info, and unload the sockets package if we loaded
    // it in the first place
    //


    if (!IsCopy()) {

        DEBUG_PRINT(OBJECTS,
                    INFO,
                    ("Not a Copy...\n"
                    ));

        DeleteCriticalSection(&_UiCritSec);

        if (IsProxy()) {

            DEBUG_PRINT(OBJECTS,
                        INFO,
                        ("A Proxy is enabled\n"
                        ));


            if (!IsProxyGlobal()) {

                DEBUG_PRINT(OBJECTS,
                            INFO,
                            ("Free-ing ProxyInfo\n"
                            ));

                delete _ProxyInfo;
                _ProxyInfo = NULL;
            }
        }

        //
        // don't unload winsock. There really is no need to unload separately
        // from process detach and if we do unload, we first have to terminate
        // async support. Dynaloading and unloading winsock is vestigial
        //

        //if (_WinsockLoaded) {
        //    UnloadWinsock();
        //}

//        if ( _Status == ERROR_SUCCESS )
//        {
//             LONG lOpenHandleCnt;
//
//             LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
//
//             if ( lpThreadInfo && ! lpThreadInfo->IsAutoProxyProxyThread )
//             {
//                 lOpenHandleCnt = InterlockedDecrement((LPLONG)&GlobalInternetOpenHandleCount);
//
//                 if ( lOpenHandleCnt < 0 )
//                 {
//                     GlobalProxyInfo.FreeAutoProxyInfo();                     
//                     GlobalProxyInfo.SetRefreshDisabled(TRUE);
//                 }
//             }
//        }
    }

    DEBUG_LEAVE(0);
}

HINTERNET
INTERNET_HANDLE_OBJECT::GetInternetHandle(
    VOID
    )
{
    return _INetHandle;
}

HINTERNET
INTERNET_HANDLE_OBJECT::GetHandle(
    VOID
    )
{
    return _INetHandle;
}

VOID
INTERNET_HANDLE_OBJECT::SetTimeout(
    IN DWORD TimeoutOption,
    IN DWORD TimeoutValue
    )
{
    switch (TimeoutOption) {
    case INTERNET_OPTION_SEND_TIMEOUT:
        _SendTimeout = TimeoutValue;
        break;

    case INTERNET_OPTION_RECEIVE_TIMEOUT:
        _ReceiveTimeout = TimeoutValue;
        break;

    case INTERNET_OPTION_DATA_SEND_TIMEOUT:
        _DataSendTimeout = TimeoutValue;
        break;

    case INTERNET_OPTION_DATA_RECEIVE_TIMEOUT:
        _DataReceiveTimeout = TimeoutValue;
        break;

    case INTERNET_OPTION_CONNECT_TIMEOUT:
        _ConnectTimeout = TimeoutValue;
        break;

    case INTERNET_OPTION_CONNECT_RETRIES:
        _ConnectRetries = TimeoutValue;
        break;

    case INTERNET_OPTION_FROM_CACHE_TIMEOUT:
        _FromCacheTimeout = TimeoutValue;
        break;
    }
}

DWORD
INTERNET_HANDLE_OBJECT::GetTimeout(
    IN DWORD TimeoutOption
    )
{
    switch (TimeoutOption) {
    case INTERNET_OPTION_SEND_TIMEOUT:
        return _SendTimeout;

    case INTERNET_OPTION_RECEIVE_TIMEOUT:
        return _ReceiveTimeout;

    case INTERNET_OPTION_DATA_SEND_TIMEOUT:
        return _DataSendTimeout;

    case INTERNET_OPTION_DATA_RECEIVE_TIMEOUT:
        return _DataReceiveTimeout;

    case INTERNET_OPTION_CONNECT_TIMEOUT:
        return _ConnectTimeout;

    case INTERNET_OPTION_CONNECT_RETRIES:
        return _ConnectRetries;

    case INTERNET_OPTION_CONNECT_BACKOFF:
        return 0;   // Backoff no longer used

    case INTERNET_OPTION_FROM_CACHE_TIMEOUT:
        return _FromCacheTimeout;

    case INTERNET_OPTION_LISTEN_TIMEOUT:

        //
        // BUGBUG - not per-object
        //

        return GlobalFtpAcceptTimeout;
    }

    INET_ASSERT(FALSE);

    //
    // we should not be here, but in case we are, return a random timeout
    //

    return DEFAULT_CONNECT_TIMEOUT;
}

//VOID INTERNET_HANDLE_OBJECT::AcquireAsyncSpinLock(VOID) {
//
//    //
//    // wait until we're the exclusive owner of the async info
//    //
//
//    while (TRUE) {
//        if (InterlockedIncrement(&_AsyncClashTest) == 0) {
//            return;
//        } else {
//            InterlockedDecrement(&_AsyncClashTest);
//            Sleep(0);
//        }
//    }
//}
//
//VOID INTERNET_HANDLE_OBJECT::ReleaseAsyncSpinLock(VOID) {
//    InterlockedDecrement(&_AsyncClashTest);
//}

DWORD
INTERNET_HANDLE_OBJECT::ExchangeStatusCallback(
    LPINTERNET_STATUS_CALLBACK lpStatusCallback,
    BOOL fType
    )
{
    DWORD error;

    //
    // we can only change the status callback if there are no async requests
    // pending
    //

    //AcquireAsyncSpinLock();

    //
    // BUGBUG - RFirth 03/16/98 - _PendingAsyncRequests is no longer being
    //          updated. It is always 0, hence always safe to change. Since
    //          no-one (that we know of) does this, we can let it go for
    //          now, but it needs to be fixed by RTM
    //
    //          (R)AddAsyncRequest() and (R)RemoveAsyncRequest() have been
    //          commented-out until this is fixed
    //

    //if (_PendingAsyncRequests == 0) {

        INTERNET_STATUS_CALLBACK callback;

        //
        // exchange new and current callbacks
        //

        callback = _StatusCallback;
        _StatusCallback = *lpStatusCallback;
        *lpStatusCallback = callback;
        _StatusCallbackType = fType;
        error = ERROR_SUCCESS;
    //} else {
    //    error = ERROR_INTERNET_REQUEST_PENDING;
    //}
    //
    //ReleaseAsyncSpinLock();

    return error;
}

//DWORD INTERNET_HANDLE_OBJECT::AddAsyncRequest(BOOL fNoCallbackOK) {
//    DWORD error;
//
//    AcquireAsyncSpinLock();
//
//    if (fNoCallbackOK || _StatusCallback != NULL) {
//        ++_PendingAsyncRequests;
//
//        INET_ASSERT(_PendingAsyncRequests > 0);
//
//        error = ERROR_SUCCESS;
//    } else {
//
//        INET_ASSERT(_PendingAsyncRequests == 0);
//
//        error = ERROR_INTERNET_NO_CALLBACK;
//    }
//
//    ReleaseAsyncSpinLock();
//
//    return error;
//}
//
//VOID INTERNET_HANDLE_OBJECT::RemoveAsyncRequest(VOID) {
//
//    INET_ASSERT(_PendingAsyncRequests > 0);
//
//    InterlockedDecrement(&_PendingAsyncRequests);
//}


VOID
INTERNET_HANDLE_OBJECT::SetAbortHandle(
    IN ICSocket * Socket
    )

/*++

Routine Description:

    Associates with this request handle the ICSocket object currently being used
    for network I/O

Arguments:

    Socket  - pointer to ICSocket

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS | DBG_SOCKETS,
                 None,
                 "INTERNET_HANDLE_OBJECT::SetAbortHandle",
                 "{%#x} %#x [sock=%#x ref=%d]",
                 GetPseudoHandle(),
                 Socket,
                 Socket ? Socket->GetSocket() : 0,
                 Socket ? Socket->ReferenceCount() : 0
                 ));

    INET_ASSERT(Socket != NULL);

    //
    // first off, increase the socket reference count to stop any other threads
    // killing it whilst we are performing the socket operation. The only way
    // another thread can dereference the socket is by calling our AbortSocket()
    // method
    //

    Socket->Reference();

    //
    // now associate the socket object with this handle object. We should not
    // have a current association
    //

    ICSocket * pSocket;

    pSocket = (ICSocket *) InterlockedExchangePointer((PVOID*)&_pICSocket, Socket);

    //
    // because ConnectSocket() can call this method multiple times without
    // intervening calls to ResetAbortHandle(), pSocket can legitimately be
    // non-NULL at this point
    //

    //INET_ASSERT(pSocket == NULL);

    //
    // if the handle was invalidated on another thread before we got
    // chance to set the socket to close, then abort the request now
    //

    //
    // BUGBUG - screws up normal FTP close handle processing - we
    //          have to communicate with the server in order to
    //          drop the connection
    //

    //if (IsInvalidated()) {
    //    AbortSocket();
    //}

    DEBUG_LEAVE(0);
}


VOID
INTERNET_HANDLE_OBJECT::ResetAbortHandle(
    VOID
    )

/*++

Routine Description:

    Disassociates this request handle and the ICSocket object when the network
    operation has completed

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS | DBG_SOCKETS,
                 None,
                 "INTERNET_HANDLE_OBJECT::ResetAbortHandle",
                 "{%#x}",
                 GetPseudoHandle()
                 ));

    //
    // there really should be a ICSocket associated with this object, otherwise
    // our handle close/invalidation logic is broken
    //

    //
    // however, we can call ResetAbortHandle() from paths where we completed
    // early, not having called SetAbortHandle()
    //

    //INET_ASSERT(pSocket != NULL);

    //
    // so if there was a ICSocket associated with this object then remove the
    // reference added in SetAbortHandle()
    //


    ICSocket * pICSocket;

    pICSocket = (ICSocket *)InterlockedExchangePointer((PVOID*)&_pICSocket, NULL);
    if (pICSocket != NULL) {

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("socket=%#x ref=%d\n",
                    pICSocket->GetSocket(),
                    pICSocket->ReferenceCount()
                    ));

        pICSocket->Dereference();
    }

    DEBUG_LEAVE(0);
}


VOID
INTERNET_HANDLE_OBJECT::AbortSocket(
    VOID
    )

/*++

Routine Description:

    If there is a ICSocket associated with this handle object then abort it. This
    forces the current network operation aborted and the request to complete
    with ERROR_INTERNET_OPERATION_CANCELLED

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS | DBG_SOCKETS,
                 None,
                 "INTERNET_HANDLE_OBJECT::AbortSocket",
                 "{%#x, %#x [sock=%#x, ref=%d]}",
                 GetPseudoHandle(),
                 (_pICSocket != NULL)
                    ? (LPVOID)_pICSocket
                    : (LPVOID)_pICSocket,
                 _pICSocket
                    ? _pICSocket->GetSocket()
                    : (_pICSocket
                        ? _pICSocket->GetSocket()
                        : 0),
                 _pICSocket
                    ? _pICSocket->ReferenceCount()
                    : (_pICSocket
                        ? _pICSocket->ReferenceCount()
                        : 0)
                 ));

    //
    // get the associated ICSocket. It may have already been removed by a call
    // to ResetAbortHandle()
    //

    //
    // if there is an associated ICSocket then abort it (close the socket handle)
    // which will complete the current network I/O (if active) with an error.
    // Once the ICSocket is aborted, we reduce the reference count that was added
    // in SetAbortHandle(). This may cause the ICSocket to be deleted
    //

    LPVOID pAddr;

    pAddr = (LPVOID)InterlockedExchangePointer((PVOID*)&_pICSocket, NULL);
    if (pAddr != NULL) {

        ICSocket * pSocket = (ICSocket *)pAddr;
//dprintf(">>>>>>>> %#x AbortSocket %#x [%#x]\n", GetCurrentThreadId(), pSocket, pSocket->GetSocket());
        pSocket->Abort();
        pSocket->Dereference();
    }

    DEBUG_LEAVE(0);
}


VOID
INTERNET_HANDLE_OBJECT::CheckGlobalProxyUpdated(
    VOID
    )

/*++

Routine Description:

    Tests whether we need to update the global proxy info structure from the registry

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_INET,
                None,
                "INTERNET_HANDLE_OBJECT::CheckGlobalProxyUpdated",
                NULL
                ));

    if (IsProxyGlobal() && InternetSettingsChanged()) {

        //
        // acquire the pointer for exclusive access
        //

        AcquireProxyInfo(TRUE);

        //
        // check to make sure we are still using the global proxy info
        //

        if (IsProxyGlobal() && !GlobalProxyInfo.IsModifiedInProcess()) {
            //GlobalProxyInfo.SetProxyInfo(INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL);
        }
        ReleaseProxyInfo();

//dprintf("CheckGlobalProxyUpdated()\n");
        ChangeGlobalSettings();
    }

    DEBUG_LEAVE(0);
}

DWORD
INTERNET_HANDLE_OBJECT::Refresh(
    IN DWORD dwInfoLevel
    ) 
/*++

Routine Description:

    Refreshes the proxy info on an InternetOpen() HINTERNET based on the parameters

    Assumes: 1. The parameters have already been validated in the API that calls
                this method (i.e. InternetOpen(), InternetSetOption())

Arguments:

    dwInfoLevel     -

Return Value:

    DWORD
        Success - ERROR_SUCCESS

--*/
{

    if (dwInfoLevel == 0) {

        DWORD error;

        //
        // this refresh value means reload the proxy info from registry,
        // but we ONLY do this if we're using the global proxy info AND
        // we haven't set it to something other than the registry contents
        //

        if (IsProxyGlobal() && !GlobalProxyInfo.IsModifiedInProcess()) {

            FixProxySettingsForCurrentConnection(TRUE);
            return ERROR_SUCCESS;

        } else {

            //
            // not using global proxy or it has been set to something other
            // than the registry contents. Just return success
            //

            return ERROR_SUCCESS;
        }
    } else {
        return ERROR_INVALID_PARAMETER;
    }
}



DWORD
INTERNET_HANDLE_OBJECT::SetProxyInfo(
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL
    )

/*++

Routine Description:

    Sets the proxy info on an InternetOpen() HINTERNET based on the parameters

    Assumes: 1. The parameters have already been validated in the API that calls
                this method (i.e. InternetOpen(), InternetSetOption())

Arguments:

    dwAccessType    - type of proxy access required

    lpszProxy       - pointer to proxy server list

    lpszProxyBypass - pointer to proxy bypass list

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER
                    The lpszProxy or lpszProxyBypass list was bad

                  ERROR_NOT_ENOUGH_MEMORY
                    Failed to create an object or allocate space for a list,
                    etc.

--*/

{
    DEBUG_ENTER((DBG_INET,
                Dword,
                "INTERNET_HANDLE_OBJECT::SetProxyInfo",
                "%s (%d), %#x (%q), %#x (%q)",
                InternetMapOpenType(dwAccessType),
                dwAccessType,
                lpszProxy,
                lpszProxy,
                lpszProxyBypass,
                lpszProxyBypass
                ));

    //
    // we only set proxy information on the top-level InternetOpen() handle
    //

    INET_ASSERT(!IsCopy());

/*

    We are setting the proxy information for an InternetOpen() handle. Based on
    the current and new settings we do the following (Note: the handle is
    initialized to DIRECT operation):

                                        current access
                +---------------------------------------------------------------
        new     |      DIRECT        |       PROXY        |      PRECONFIG
       access   |                    |                    |
    +-----------+--------------------+--------------------+---------------------
    | DIRECT    | No action          | Delete proxy info  | Remove reference to
    |           |                    |                    | global proxy info
    +-----------+--------------------+--------------------+---------------------
    | PROXY     | Set new proxy info | Delete proxy info. | Remove reference to
    |           |                    | Set new proxy info | global proxy info.
    |           |                    |                    | Set new proxy info
    +-----------+--------------------+--------------------+---------------------
    | PRECONFIG | Set proxy info to  | Delete proxy info. | No action
    |           | global proxy info  | Set proxy info to  |
    |           |                    | global proxy info  |
    +-----------+--------------------+--------------------+---------------------
*/

    DWORD error = ERROR_SUCCESS;
    PROXY_INFO * proxyInfo = NULL;

    //
    // acquire proxy info for exclusive access
    //

    AcquireProxyInfo(TRUE);

    if (IsProxy()) {

        //
        // delete private proxy info, or unlink from global proxy info
        //

        SafeDeleteProxyInfo();
    }

    //
    // Map Various Proxy types to their internal counterparts,
    //   note that I've ordered them in what I think is their 
    //   use frequency (how often each one is most likely to get hit).
    //

    switch (dwAccessType)
    {
        case INTERNET_OPEN_TYPE_PRECONFIG:
            proxyInfo = &GlobalProxyInfo;
            break;

        case INTERNET_OPEN_TYPE_DIRECT:
            proxyInfo = NULL;
            break;

        case INTERNET_OPEN_TYPE_PROXY:     
            {
                INET_ASSERT(!IsProxy());

                proxyInfo = new PROXY_INFO;
                if (proxyInfo != NULL) {
                    proxyInfo->InitializeProxySettings();
                    error = proxyInfo->GetError();
                    if (error == ERROR_SUCCESS &&
                        lpszProxy ) 
                    {

                        INTERNET_PROXY_INFO_EX info;
                        memset(&info, 0, sizeof(info));
                        info.dwStructSize = sizeof(info);
                        info.dwFlags = (PROXY_TYPE_DIRECT | PROXY_TYPE_PROXY);

                        info.lpszProxy = lpszProxy;
                        info.lpszProxyBypass = lpszProxyBypass;

                        error = proxyInfo->SetProxySettings(&info, TRUE /*modified*/);

                    }
                    if (error != ERROR_SUCCESS) {
                        delete proxyInfo;
                        proxyInfo = NULL;
                    }
                } else {
                    error = ERROR_NOT_ENOUGH_MEMORY;
                }

                break;
            }

        case INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY:
            {
                proxyInfo = new PROXY_INFO_GLOBAL_WRAPPER;
                if (proxyInfo == NULL) {
                    error = ERROR_NOT_ENOUGH_MEMORY;
                }
                break;
            }

        default:
            proxyInfo = NULL;
            break;
    }

    SetProxyInfo(proxyInfo);

    ReleaseProxyInfo();

    DEBUG_LEAVE(error);

    return error;
}


DWORD
INTERNET_HANDLE_OBJECT::GetProxyStringInfo(
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Returns the current proxy information for this INTERNET_HANDLE_OBJECT

Arguments:

    lpBuffer            - pointer to buffer where INTERNET_PROXY_INFO will be
                          written, and any proxy strings (if sufficient space)

    lpdwBufferLength    - IN: number of bytes in lpBuffer
                          OUT: number of bytes returned in lpBuffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    lpBuffer doesn't have enough space to hold the proxy
                    information. *lpdwBufferLength has the required size

--*/

{
    DEBUG_ENTER((DBG_INET,
                Dword,
                "INTERNET_HANDLE_OBJECT::GetProxyStringInfo",
                "%#x, %#x [%d]",
                lpBuffer,
                lpdwBufferLength,
                lpdwBufferLength ? *lpdwBufferLength : 0
                ));

    INET_ASSERT(!IsCopy());

    AcquireProxyInfo(FALSE);

    DWORD error;

    if (IsProxy()) {
        error = _ProxyInfo->GetProxyStringInfo(lpBuffer, lpdwBufferLength);
    } else {
        if (*lpdwBufferLength >= sizeof(INTERNET_PROXY_INFO)) {

            LPINTERNET_PROXY_INFO lpInfo = (LPINTERNET_PROXY_INFO)lpBuffer;

            lpInfo->dwAccessType = INTERNET_OPEN_TYPE_DIRECT;
            lpInfo->lpszProxy = NULL;
            lpInfo->lpszProxyBypass = NULL;
            error = ERROR_SUCCESS;
        } else {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        *lpdwBufferLength = sizeof(INTERNET_PROXY_INFO);
    }

    ReleaseProxyInfo();

    DEBUG_LEAVE(error);

    return error;
}


DWORD
INTERNET_HANDLE_OBJECT::GetProxyInfo(
    IN AUTO_PROXY_ASYNC_MSG **ppQueryForProxyInfo
    )

/*++

Routine Description:

    Returns all proxy information based on a protocol scheme

Arguments:

    tProtocol           - protocol to get proxy info for

    lptScheme           - returned scheme

    lplpszHostName      - returned proxy name

    lpdwHostNameLength  - returned length of proxy name

    lpHostPort          - returned proxy port

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE

--*/

{
    DEBUG_ENTER((DBG_INET,
                 Dword,
                 "INTERNET_HANDLE_OBJECT::GetProxyInfo",
                 "%#x",
                 ppQueryForProxyInfo
                 ));

    INET_ASSERT(!IsCopy());

    DWORD error;
    BOOL rc;

    //if (IsProxyGlobal()) {
    //    CheckGlobalProxyUpdated();
    //}

    AcquireProxyInfo(FALSE);

    if ( _ProxyInfo )
    {
        error = _ProxyInfo->QueryProxySettings(ppQueryForProxyInfo);
    }
    else
    {
        error = ERROR_SUCCESS;
        (*ppQueryForProxyInfo)->SetUseProxy(FALSE);
    }

    ReleaseProxyInfo();

    DEBUG_LEAVE(error);

    return error;
}


BOOL
INTERNET_HANDLE_OBJECT::RedoSendRequest(
    IN OUT LPDWORD lpdwError,
    IN AUTO_PROXY_ASYNC_MSG *pQueryForProxyInfo,
    IN CServerInfo *pOriginServer,
    IN CServerInfo *pProxyServer
    )
{

    INET_ASSERT(!IsCopy());

    BOOL rc;

   AcquireProxyInfo(FALSE);

    if ( _ProxyInfo )
    {
        rc = _ProxyInfo->RedoSendRequest(
                                         lpdwError,
                                         pQueryForProxyInfo,
                                         pOriginServer,
                                         pProxyServer
                                         );
    }
    else
    {
        rc = FALSE;
    }

    ReleaseProxyInfo();

    return rc;
}


DWORD
INTERNET_HANDLE_OBJECT::IndicateStatus(
    IN DWORD dwStatus,
    IN LPVOID lpBuffer,
    IN DWORD dwLength
    )

/*++

Routine Description:

    Object version of InternetIndicateStatus

Arguments:

    dwStatus    - status code

    lpBuffer    - pointer to information to indicate

    dwLength    - length of information

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_OPERATION_CANCELLED
                    The app closed the object handle during the callback

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 Dword,
                 "HANDLE_OBJECT::IndicateStatus",
                 "{%#x} %s, %#x, %d",
                 GetPseudoHandle(),
                 InternetMapStatus(dwStatus),
                 lpBuffer,
                 dwLength
                 ));

    DWORD error = ERROR_SUCCESS;
    INTERNET_STATUS_CALLBACK appCallback = GetStatusCallback();
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD_PTR context = GetContext();
    BOOL bCopyFailed = FALSE;

    if (appCallback == NULL) {

        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("%#x: no callback\n",
                    GetPseudoHandle()
                    ));

        INET_ASSERT(dwStatus != INTERNET_STATUS_REQUEST_COMPLETE);

        goto quit;
    }

    if (lpThreadInfo == NULL) {

        INET_ASSERT(dwStatus != INTERNET_STATUS_REQUEST_COMPLETE);
        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    IF_DEBUG(OBJECTS) {

        if (dwStatus == INTERNET_STATUS_REQUEST_COMPLETE) {

            DEBUG_PRINT(THRDINFO,
                        INFO,
                        ("REQUEST_COMPLETE: dwResult = %#x, dwError = %d [%s]\n",
                        ((LPINTERNET_ASYNC_RESULT)lpBuffer)->dwResult,
                        ((LPINTERNET_ASYNC_RESULT)lpBuffer)->dwError,
                        InternetMapError(((LPINTERNET_ASYNC_RESULT)lpBuffer)->dwError)
                        ));

        }
    }

    LPVOID pInfo;
    DWORD infoLength;

    //
    // we make a copy of the info to remove the app's opportunity to change it.
    // E.g. if we were about to resolve host name "foo" and passed the pointer
    // to our buffer containing "foo", the app could change the name to "bar",
    // changing the intended server
    //

    if (lpBuffer != NULL) {
        pInfo = (LPVOID)ALLOCATE_FIXED_MEMORY(dwLength);
        if (pInfo != NULL) {
            memcpy(pInfo, lpBuffer, dwLength);
            infoLength = dwLength;
        } else {

            DEBUG_PRINT(THRDINFO,
                        ERROR,
                        ("Failed to allocate %d bytes for info\n",
                        dwLength
                        ));

            //
            // if this is a request complete indication, then we CANNOT fail it
            // because we cannot make a copy of the parameters. In this case,
            // we'll trust the app & pass in a pointer to our (stack) data
            //

            if (dwStatus == INTERNET_STATUS_REQUEST_COMPLETE) {
                pInfo = lpBuffer;
                infoLength = dwLength;
                bCopyFailed = TRUE; // don't free pInfo
            } else {
                infoLength = 0;
            }
        }
    } else {
        pInfo = NULL;
        infoLength = 0;
    }

    //
    // protect the object from being dereferenced & deleted inside the
    // callback
    //

    Reference();

    //
    // we're about to call into the app. We may be in the context of an async
    // worker thread, and if the callback submits an async request then we'll
    // execute it synchronously. To avoid this, we will reset the async worker
    // thread indicator in the per-thread info block and restore it when the
    // app returns control to us. This way, if the app makes an API request
    // during the callback, on a handle that has async I/O semantics, then we
    // will simply queue it, and not try to execute it synchronously
    //

    BOOL isAsyncWorkerThread;

    isAsyncWorkerThread = lpThreadInfo->IsAsyncWorkerThread;
    lpThreadInfo->IsAsyncWorkerThread = FALSE;
    lpThreadInfo->InCallback = TRUE;

    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "(*callback)",
                 "%#x, %#x, %s (%d), %#x [%#x], %d",
                 GetPseudoHandle(),
                 context,
                 InternetMapStatus(dwStatus),
                 dwStatus,
                 pInfo,
                 (infoLength == sizeof(DWORD))
                    ? *(LPDWORD)pInfo
                    : 0,
                 infoLength
                 ));

    INET_ASSERT(!IsBadCodePtr((FARPROC)appCallback));

    PERF_LOG(PE_APP_CALLBACK_START,
             dwStatus,
             lpThreadInfo->ThreadId,
             lpThreadInfo->hObject
             );

    appCallback(GetPseudoHandle(),
                context,
                dwStatus,
                pInfo,
                infoLength
                );

    PERF_LOG(PE_APP_CALLBACK_END,
             dwStatus,
             lpThreadInfo->ThreadId,
             lpThreadInfo->hObject
             );

    DEBUG_LEAVE(0);

    //
    // restore the app/worker thread state in our per-thread info
    //

    lpThreadInfo->InCallback = FALSE;
    lpThreadInfo->IsAsyncWorkerThread = isAsyncWorkerThread;

    //
    // reduce the reference count on the object (it *may* be destroyed)
    //

    BOOL ok;

    ok = Dereference();

    //
    // free the buffer if we created a copy
    //

    if ((pInfo != NULL)&& !bCopyFailed) {
        FREE_FIXED_MEMORY(pInfo);
    }

    //
    // if the object is now invalid then the app closed the handle in
    // the callback, and the entire operation is cancelled
    //

    if (ok || IsInvalidated()) {
        error = ERROR_INTERNET_OPERATION_CANCELLED;
    }

quit:

    DEBUG_LEAVE(error);

    return error;
}


VOID
UnicodeStatusCallbackWrapper(
    IN HINTERNET hInternet,
    IN DWORD_PTR dwContext,
    IN DWORD dwInternetStatus,
    IN LPVOID lpvStatusInformation OPTIONAL,
    IN DWORD dwStatusInformationLength
)
{
    DWORD dwErr = ERROR_SUCCESS;
    INTERNET_STATUS_CALLBACK iscCallback;
    INET_ASSERT(hInternet != NULL);
    MEMORYPACKET mpBuffer;
    HINTERNET hInternetMapped = NULL;

    dwErr = MapHandleToAddress(hInternet, (LPVOID*)&hInternetMapped, FALSE);
    if (dwErr!=ERROR_SUCCESS)
        goto Cleanup;

    dwErr = ((HANDLE_OBJECT *)hInternetMapped)->IsValid(TypeWildHandle);
    if (dwErr==ERROR_SUCCESS)
    {
        iscCallback = ((INTERNET_HANDLE_OBJECT *)hInternetMapped)->GetTrueStatusCallback();
        INET_ASSERT(iscCallback);

        switch (dwInternetStatus)
        {
        case INTERNET_STATUS_RESOLVING_NAME:
        case INTERNET_STATUS_NAME_RESOLVED:
        case INTERNET_STATUS_REDIRECT:
        case INTERNET_STATUS_CONNECTING_TO_SERVER:
        case INTERNET_STATUS_CONNECTED_TO_SERVER:
            mpBuffer.dwSize = dwStatusInformationLength;
            if (lpvStatusInformation)
            {
                mpBuffer.dwAlloc = (MultiByteToWideChar(CP_ACP,0,(LPSTR)lpvStatusInformation,-1,NULL,0)+1)
                                    *sizeof(WCHAR);
                mpBuffer.psStr = (LPSTR)ALLOC_BYTES(mpBuffer.dwAlloc);
                if (!mpBuffer.psStr)
                {
                    dwErr = ERROR_INSUFFICIENT_BUFFER;
                    goto Cleanup;
                }
                mpBuffer.dwSize = MultiByteToWideChar(CP_ACP,0,(LPSTR)lpvStatusInformation,-1,
                                    (LPWSTR)mpBuffer.psStr, mpBuffer.dwAlloc/sizeof(WCHAR));
            }
            iscCallback(hInternet, dwContext, dwInternetStatus, (LPVOID)mpBuffer.psStr,
                    mpBuffer.dwSize);
            break;

        default:
            iscCallback(hInternet, dwContext, dwInternetStatus, lpvStatusInformation,
                dwStatusInformationLength);
        }
    }

Cleanup:
    if (hInternetMapped)
    {
        DereferenceObject(hInternetMapped);
    }
    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
}


