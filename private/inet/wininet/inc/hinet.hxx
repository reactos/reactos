/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    handle.hxx

Abstract:

    Contains client-side internet handle class

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:


    Sophia Chung (sophiac) 12-Feb-1995 (added FTP handle obj. class defs)

--*/

#ifndef _HINET_
#define _HINET_

//
// manifests
//

#define OBJECT_SIGNATURE            0x41414141  // "AAAA"
#define DESTROYED_OBJECT_SIGNATURE  0x5A5A5A5A  // "ZZZZ"
#define LOCAL_INET_HANDLE           (HINTERNET)-2
#define INET_INVALID_HANDLE_VALUE   (NULL)
#define MAX_PROXY_BYPASS_BUCKETS    256

//
// types
//

typedef
DWORD
(*URLGEN_FUNC)(
    INTERNET_SCHEME,
    LPSTR,
    LPSTR,
    LPSTR,
    LPSTR,
    DWORD,
    LPSTR *,
    LPDWORD
    );

//
// typedef virtual close function
//

typedef BOOL (*CLOSE_HANDLE_FUNC)(HINTERNET);
typedef BOOL (*CONNECT_CLOSE_HANDLE_FUNC)(HINTERNET, DWORD);

//
// forward references
//

class ICSocket;

//
// class implementations
//

/*++

Class Description:

    This is generic HINTERNET class definition.

Private Member functions:

    None.

Public Member functions:

    IsValid : Validates handle pointer.

    GetStatus : Gets status of the object.

    GetInternetHandle : Virtual function that gets the internet handle
        value form the generic object handle.

    GetHandle : Virtual function that gets the service handle value from
        the generic object handle.

--*/

class HANDLE_OBJECT {

private:

    //
    // _List - doubly-linked list of all handle objects
    //

    LIST_ENTRY _List;

    //
    // _Children - serialized list of all child handle objects
    //

    SERIALIZED_LIST _Children;

    //
    // _Siblings - doubly-linked list of all handle objects at the current level
    // and descended from a particular parent - e.g. all InternetConnect handles
    // belonging to InternetOpen handle. Linked to the parent's _Children list
    //

    LIST_ENTRY _Siblings;

    //
    // _Parent - this member has the address of the parent object. Mainly used
    // when we create an INTERNET_CONNECT_HANDLE_OBJECT on behalf of the object
    // created with InternetOpenUrl().
    // We also need this field to locate the INTERNET_CONNECT_HANDLE_OBJECT that
    // is the parent of a HTTP request object: if the caller requests Keep-Alive
    // then the parent INTERNET_CONNECT_HANDLE_OBJECT will have the socket that
    // we must use
    //

    HANDLE_OBJECT* _Parent;

    //
    // _DeleteWithChild - used in conjunction with _Parent. In the case of an
    // INTERNET_CONNECT_HANDLE_OBJECT created on behalf of a HTTP, gopher or
    // FTP object created by InternetOpenUrl(), we need to delete the parent
    // handle when the child is closed via InternetCloseHandle(). If we don't
    // do this then the connect handle object will be orphaned (i.e. we will
    // have a memory leak)
    //

    //
    // BUGBUG - combine into bitfield
    //

    BOOL _DeleteWithChild;

    //
    // _Handle - the non-address pseudo-handle value returned at the API
    //

    HINTERNET _Handle;

    //
    // _ObjectType - type of handle object (mainly for debug purposes)
    //

    HINTERNET_HANDLE_TYPE _ObjectType;

    //
    // _ReferenceCount - number of references of this object. Used to protect
    // object against multi-threaded operations and can be used to delete
    // object when count is decremented to 0
    //

    LONG _ReferenceCount;

    //
    // _Invalid - when this is TRUE the handle has been closed although it may
    // still be alive. The app cannot perform any further actions on this
    // handle object - it will soon be destroyed
    //

    //
    // BUGBUG - combine into bitfield
    //

    BOOL _Invalid;

    //
    // _Error - optionally set when invalidating the handle. If set (non-zero)
    // then this is the preferred error to return from an invalidated request
    //

    DWORD _Error;

    //
    // _Signature - used to perform sanity test of object
    //

    DWORD _Signature;

protected:

    //
    // _Context - the context value specified in the API that created this
    // object. This member is inherited by all derived objects
    //

    DWORD_PTR _Context;

    //
    // _Status - used to store return codes whilst creating the object. If not
    // ERROR_SUCCESS when new() returns, the object is deleted
    //

    DWORD _Status;

public:

    HANDLE_OBJECT(
        HANDLE_OBJECT * Parent
        );

    virtual
    ~HANDLE_OBJECT(
        VOID
        );

    DWORD
    Reference(
        VOID
        );

    BOOL
    Dereference(
        VOID
        );

    VOID Invalidate(VOID) {

        //
        // just mark the object as invalidated
        //

        _Invalid = TRUE;
    }

    BOOL IsInvalidated(VOID) const {
        return _Invalid;
    }

    VOID InvalidateWithError(DWORD dwError) {
        _Error = dwError;
        Invalidate();
    }

    DWORD GetError(VOID) const {
        return _Error;
    }

    DWORD ReferenceCount(VOID) const {
        return _ReferenceCount;
    }

    VOID AddChild(PLIST_ENTRY Child) {

        DEBUG_ENTER((DBG_OBJECTS,
                    None,
                    "AddChild",
                    "%#x",
                    Child
                    ));

        InsertAtTailOfSerializedList(&_Children, Child);

        //
        // each time we add a child object, we increase the reference count, and
        // correspondingly decrease it when we remove the child. This stops us
        // leaving orphaned child objects
        //

        Reference();

        DEBUG_LEAVE(0);

    }

    VOID RemoveChild(PLIST_ENTRY Child) {

        DEBUG_ENTER((DBG_OBJECTS,
                    None,
                    "RemoveChild",
                    "%#x",
                    Child
                    ));

        RemoveFromSerializedList(&_Children, Child);

        INET_DEBUG_ASSERT((Child->Flink == NULL) && (Child->Blink == NULL));

        //
        // if this object was previously invalidated but could not be deleted
        // in case we orphaned child objects, then this dereference is going to
        // close this object
        //

        Dereference();

        DEBUG_LEAVE(0);

    }

    BOOL HaveChildren(VOID) {
        return IsSerializedListEmpty(&_Children) ? FALSE : TRUE;
    }

    HINTERNET NextChild(VOID) {

        PLIST_ENTRY link = HeadOfSerializedList(&_Children);

        INET_ASSERT(link != (PLIST_ENTRY)&_Children.List.Flink);

        //
        // link points at _Siblings.Flink in the child object
        //

        HANDLE_OBJECT * pHandle;

        pHandle = CONTAINING_RECORD(link, HANDLE_OBJECT, _Siblings.Flink);

        //
        // return the pseudo handle of the child object
        //

        return pHandle->GetPseudoHandle();
    }

    HINTERNET GetPseudoHandle(VOID) {
        return _Handle;
    }

    DWORD_PTR GetContext(VOID) {
        return _Context;
    }

    //
    // BUGBUG - rfirth 04/05/96 - remove virtual functions
    //
    //          See similar BUGBUG in INTERNET_HANDLE_OBJECT
    //

    virtual HINTERNET GetInternetHandle(VOID) {
        return INET_INVALID_HANDLE_VALUE;
    }

    virtual HINTERNET GetHandle(VOID) {
        return INET_INVALID_HANDLE_VALUE;
    }

    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID) {
        return TypeGenericHandle;
    }

    virtual VOID SetHtml(VOID) {
    }

    virtual VOID SetHtmlState(HTML_STATE) {
    }

    virtual HTML_STATE GetHtmlState(VOID) {
        return HTML_STATE_INVALID;
    }

    virtual LPSTR GetUrl(VOID) {
        return NULL;
    }

    virtual VOID SetUrl(LPSTR Url) {
        UNREFERENCED_PARAMETER(Url);
    }

    virtual VOID SetDirEntry(LPSTR DirEntry) {
        UNREFERENCED_PARAMETER(DirEntry);
    }

    virtual LPSTR GetDirEntry(VOID) {
        return NULL;
    }

    DWORD IsValid(HINTERNET_HANDLE_TYPE ExpectedHandleType);

    DWORD GetStatus(VOID) {
        return _Status;
    }

    VOID SetParent(HINTERNET Handle, BOOL DeleteWithChild) {
        _Parent = (HANDLE_OBJECT *)Handle;
        _DeleteWithChild = DeleteWithChild;
    }

    HINTERNET GetParent() {
        return _Parent;
    }

    BOOL GetDeleteWithChild(VOID) {
        return _DeleteWithChild;
    }

    VOID SetObjectType(HINTERNET_HANDLE_TYPE Type) {
        _ObjectType = Type;
    }

    HINTERNET_HANDLE_TYPE GetObjectType(VOID) const {
        return _ObjectType;
    }

    void OnLastHandleDestroyed (void)
    {
        HttpFiltClose();
    }

    virtual VOID AbortSocket(VOID) {

    }

    //
    // friend functions
    //

    //
    // these friend functions are here so that they can get access to _List for
    // CONTAINING_RECORD()
    //

    friend
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
        );

    friend
    INT
    FlushExistingConnectObjects(
        IN HINTERNET hInternet
        );

    friend
    HANDLE_OBJECT *
    ContainingHandleObject(
        IN LPVOID lpAddress
        );
};



// The following enables unicode-receiving callbacks.
VOID UnicodeStatusCallbackWrapper(IN HINTERNET hInternet, IN DWORD_PTR dwContext,
        IN DWORD dwInternetStatus, IN LPVOID lpvStatusInformation OPTIONAL,
        IN DWORD dwStatusInformationLength );

/*++

Class Description:

    This class defines the INTERNET_HANDLE_OBJECT.

Private Member functions:

    None.

Public Member functions:

    GetInternetHandle : Virtual function that gets the Internet handle
        value from the generic object handle.

    GetHandle : Virtual function that gets the service handle value from
        the generic object handle.

--*/

class INTERNET_HANDLE_OBJECT : public HANDLE_OBJECT {

private:


    //
    // _INetHandle - BUGBUG - why is this still here?
    //

    //
    // BUGBUG - post-beta cleanup - remove
    //

    HINTERNET _INetHandle;

    //
    // _IsCopy - TRUE if this is part of a derived object handle (e.g. a
    // connect handle object)
    //

    //
    // BUGBUG - post-beta cleanup - combine into bitfield
    //

    BOOL _IsCopy;

    //
    // _UserAgent - name by why which the application wishes to be known to
    // HTTP servers. Provides the User-Agent header unless overridden by
    // specific User-Agent header from app
    //

    ICSTRING _UserAgent;

    //
    // _ProxyInfo - maintains the proxy server and bypass lists
    //

    PROXY_INFO * _ProxyInfo;

    //
    // _ProxyInfoResourceLock - must acquire for exclusive access in order to
    // modify the proxy info
    //

    RESOURCE_LOCK _ProxyInfoResourceLock;

    VOID AcquireProxyInfo(BOOL bExclusiveMode) {

        INET_ASSERT(!IsCopy());

        _ProxyInfoResourceLock.Acquire(bExclusiveMode);
    }

    VOID ReleaseProxyInfo(VOID) {

        INET_ASSERT(!IsCopy());

        _ProxyInfoResourceLock.Release();
    }

    VOID SafeDeleteProxyInfo(VOID) {

        DEBUG_ENTER((DBG_OBJECTS,
            None,
            "SafeDeleteProxyInfo",
            ""
            ));


        if (_ProxyInfo != NULL) {
            AcquireProxyInfo(TRUE);
            if (!IsProxyGlobal()) {
                if (_ProxyInfo != NULL) {
                    delete _ProxyInfo;
                }
            }
            _ProxyInfo = NULL;
            ReleaseProxyInfo();
        }

        DEBUG_LEAVE(0);
    }

    //
    // _dwInternetOpenFlags - flags from InternetOpen()
    //
    // BUGBUG - there should only be ONE flags DWORD for all handles descended
    //          from this one. This is it
    //          Rename to just _Flags, or _OpenFlags
    //

    DWORD _dwInternetOpenFlags;

#ifdef POST_BETA

    //
    // _Flags - contains all flags from all handle creation functions. Each
    // subsequent derived handle just OR's its flags into the base flags
    //

    DWORD _Flags;

#endif

    //
    // _WinsockLoaded - TRUE if we managed to successfully load winsock
    //

    //
    // BUGBUG - post-beta cleanup - combine into bitfield
    //

    BOOL _WinsockLoaded;

    //
    // _pICSocket - pointer to ICSocket for new HTTP async code
    //

    ICSocket * _pICSocket;

    DWORD _dwBlockedOnError;
    DWORD _dwBlockedOnResult;
    LPVOID _lpBlockedResultData;

    CRITICAL_SECTION _UiCritSec;
                                                                                      
protected:

    //
    // _dwUiBlocked - count of handles blocked on a request waiting to show UI to the User
    //

    DWORD  _dwUiBlocked;

    //
    // _dwBlockId - contains the ID that this handle is blocked on for the purpose of 
    //   showing UI to the user.
    //

    DWORD_PTR _dwBlockId;

    //
    // timout/retry/back-off values. The app can specify timeouts etc. at
    // various levels: global (DLL), Internet handle (from InternetOpen()) and
    // Connect handle (from InternetConnect()). The following fields are
    // inherited by INTERNET_CONNECT_HANDLE_OBJECT (and derived objects)
    //

    DWORD _ConnectTimeout;
    DWORD _ConnectRetries;
    DWORD _SendTimeout;
    DWORD _DataSendTimeout;
    DWORD _ReceiveTimeout;
    DWORD _DataReceiveTimeout;
    DWORD _FromCacheTimeout;
    DWORD _SocketSendBufferLength;
    DWORD _SocketReceiveBufferLength;

    //
    // _Async - TRUE if the InternetOpen() handle, and all handles descended
    // from it, support asynchronous I/O
    //

    //
    // BUGBUG - post-beta cleanup - get from flags
    //

    BOOL _Async;

    //
    // _DataAvailable - the number of bytes that can be read from this handle
    // (i.e. only protocol handles) immediately. This avoids a read request
    // being made asynchronously if it can be satisfied immediately
    //

    DWORD _DataAvailable;

    //
    // _EndOfFile - TRUE when we have received all data for this request. This
    // is used to avoid the API having to perform an extraneous read (possibly
    // asynchronously) just to discover that we reached end-of-file already
    //

    //
    // BUGBUG - post-beta cleanup - combine into bitfield
    //

    BOOL _EndOfFile;

    //
    // _StatusCallback - we now maintain callbacks on a per-handle basis. The
    // callback address comes from the parent handle or the DLL if this is an
    // InternetOpen() handle. The status callback can be changed for an
    // individual handle object using the ExchangeStatusCallback() method
    // (called from InternetSetStatusCallback())
    //

    //
    // BUGBUG - this should go in HANDLE_OBJECT
    //

    INTERNET_STATUS_CALLBACK _StatusCallback;
    BOOL _StatusCallbackType;

    //
    // _PendingAsyncRequests - the number of pending async requests on this
    // handle object. While this is !0, an app cannot reset the callback
    //

    //
    // BUGBUG - RLF 03/16/98. _PendingAsyncRequests is no longer being modified
    //          since fibers were removed. This count is used for precautionary
    //          measure when changing a status callback on a handle that has
    //          outstanding async requests. I am commenting-out all uses of this
    //          and _AsyncClashTest for B1. Should be fixed or re-examined after
    //          B1
    //

    //LONG _PendingAsyncRequests;

    //
    // _AsyncClashTest - used with InterlockedIncrement() to ensure that only
    // one thread is modifying _PendingAsyncRequests
    //

    //LONG _AsyncClashTest;

public:

    INTERNET_HANDLE_OBJECT(
        LPCSTR UserAgent,
        DWORD AccessMethod,
        LPSTR ProxyName,
        LPSTR ProxyBypass,
        DWORD Flags
        );

    INTERNET_HANDLE_OBJECT(INTERNET_HANDLE_OBJECT *INetObj);

    virtual ~INTERNET_HANDLE_OBJECT(VOID);

    virtual HINTERNET GetInternetHandle(VOID);

    virtual HINTERNET GetHandle(VOID);

    //
    // BUGBUG - rfirth 04/05/96 - remove virtual functions
    //
    //          For the most part, these functions aren't required to be
    //          virtual. They should just be moved to the relevant handle type
    //          (e.g. FTP_FILE_HANDLE_OBJECT). Even GetHandleType() is overkill.
    //          Replacing with a method that just returns
    //          HANDLE_OBJECT::_ObjectType would be sufficient
    //

    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID) {
        return TypeInternetHandle;
    }

    virtual VOID SetHtml(VOID) {
    }

    virtual VOID SetHtmlState(HTML_STATE) {
    }

    virtual HTML_STATE GetHtmlState(VOID) {
        return HTML_STATE_INVALID;
    }

    virtual LPSTR GetUrl(VOID) {
        return NULL;
    }

    virtual VOID SetUrl(LPSTR Url) {
        if (Url != NULL) {
            Url = (LPSTR)FREE_MEMORY(Url);

            INET_ASSERT(Url == NULL);

        }
    }

    virtual VOID SetDirEntry(LPSTR DirEntry) {
        UNREFERENCED_PARAMETER(DirEntry);
    }

    virtual LPSTR GetDirEntry(VOID) {
        return NULL;
    }

    BOOL IsCopy(VOID) const {
        return _IsCopy;
    }

    VOID LockPopupInfo(VOID) {
        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("INTERNET_HANDLE_OBJECT::LockPopupInfo()\n"
                    ));
        EnterCriticalSection(&_UiCritSec);
    }

    VOID UnlockPopupInfo(VOID) {
        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("INTERNET_HANDLE_OBJECT::UnlockPopupInfo()\n"
                    ));
        LeaveCriticalSection(&_UiCritSec);
    }

    VOID BlockOnUserInput(DWORD dwError, DWORD_PTR dwBlockId, LPVOID lpResultData) {

        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("INTERNET_HANDLE_OBJECT::BlockOnUserInput(BLOCKID) = %u\n",
                    dwBlockId
                    ));

        INET_ASSERT(!_dwUiBlocked);

        _dwBlockedOnError    = dwError;
        _dwBlockedOnResult   = ERROR_SUCCESS;
        _dwBlockId           = dwBlockId;
        _lpBlockedResultData = lpResultData;
        _dwUiBlocked++;
    }

    VOID UnBlockOnUserInput(LPDWORD lpdwResultCode, LPVOID *lplpResultData) {

        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("INTERNET_HANDLE_OBJECT::UnBlockOnUserInput() = %u\n",
                    _dwBlockedOnResult
                    ));

        INET_ASSERT(_dwUiBlocked);
        _dwUiBlocked--;
        *lpdwResultCode = _dwBlockedOnResult;
        *lplpResultData = _lpBlockedResultData;
    }

    VOID SetBlockedResultCode(DWORD dwResultCode) {

        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("INTERNET_HANDLE_OBJECT::SetBlockedResultCode(result = %u)\n",
                    dwResultCode
                    ));

        INET_ASSERT(_dwUiBlocked);
        _dwBlockedOnResult = dwResultCode;
    }


    BOOL IsBlockedOnUserInput(VOID) {
        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("INTERNET_HANDLE_OBJECT::IsBlockedOnUserInput() = %B\n",
                    _dwUiBlocked
                    ));

        return (BOOL) _dwUiBlocked;
    }

    VOID IncrementBlockedUiCount(VOID) {

        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("INTERNET_HANDLE_OBJECT::IncrementBlockedUiCount()+1 = %u\n",
                    (_dwUiBlocked+1)
                    ));

        _dwUiBlocked++;
    }

    DWORD GetBlockedUiCount(VOID) {

        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("INTERNET_HANDLE_OBJECT::GetBlockedUiCount() = %u\n",
                    _dwUiBlocked
                    ));


        INET_ASSERT(_dwUiBlocked);
        return _dwUiBlocked;
    }

    VOID ClearBlockedUiCount(VOID) {

        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("INTERNET_HANDLE_OBJECT::ClearBlockedUiCount() \n"
                    ));

        INET_ASSERT(_dwUiBlocked);
        _dwUiBlocked = 0;
    }


    DWORD_PTR GetBlockId(VOID) {

        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("INTERNET_HANDLE_OBJECT::GetBlockId() = %u\n",
                    _dwBlockId
                    ));


        INET_ASSERT(_dwUiBlocked);
        return _dwBlockId;
    }

    VOID GetUserAgent(LPSTR Buffer, LPDWORD BufferLength) {
        _UserAgent.CopyTo(Buffer, BufferLength);
    }

    LPSTR GetUserAgent(VOID) {
        return _UserAgent.StringAddress();
    }

    LPSTR GetUserAgent(LPDWORD lpdwLength) {
        *lpdwLength = _UserAgent.StringLength();
        return _UserAgent.StringAddress();
    }

    VOID SetUserAgent(LPSTR lpszUserAgent) {

        INET_ASSERT(lpszUserAgent != NULL);

        _UserAgent = lpszUserAgent;
    }

    BOOL IsProxy(VOID) const {

        //
        // we can return this info without acquiring the critical section
        //

        return (_ProxyInfo != NULL) ? _ProxyInfo->IsProxySettingsConfigured() : FALSE;
    }

    BOOL IsProxyGlobal(VOID) const {
        return (_ProxyInfo == &GlobalProxyInfo) ? TRUE : FALSE;
    }

    VOID
    CheckGlobalProxyUpdated(
        VOID
        );

    PROXY_INFO * GetProxyInfo(VOID) const {

        INET_ASSERT(!IsCopy());

        return _ProxyInfo;
    }

    VOID SetProxyInfo(PROXY_INFO * ProxyInfo) {

        INET_ASSERT(!IsCopy());

        _ProxyInfo = ProxyInfo;
    }

    VOID ResetProxyInfo(VOID) {

        INET_ASSERT(!IsCopy());

        SetProxyInfo(NULL);
    }

    DWORD
    Refresh(
        IN DWORD dwInfoLevel
        ); 

    DWORD
    SetProxyInfo(
        IN DWORD dwAccessType,
        IN LPCSTR lpszProxy OPTIONAL,
        IN LPCSTR lpszProxyBypass OPTIONAL
        );

    DWORD
    GetProxyStringInfo(
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );

    DWORD
    GetProxyInfo(
        IN AUTO_PROXY_ASYNC_MSG **ppQueryForProxyInfo
        );

    BOOL
    RedoSendRequest(
        IN OUT LPDWORD lpdwError,
        IN AUTO_PROXY_ASYNC_MSG *pQueryForProxyInfo,
        IN CServerInfo *pOriginServer,
        IN CServerInfo *pProxyServer
        );

    VOID SetContext(DWORD_PTR NewContext) {
        _Context = NewContext;
    }

    VOID SetTimeout(DWORD TimeoutOption, DWORD TimeoutValue);

    DWORD GetTimeout(DWORD TimeoutOption);

    BOOL IsAsyncHandle(VOID) {
        return _Async;
    }

    VOID ResetAsync(VOID) {
        _Async = FALSE;
    }

    VOID SetAvailableDataLength(DWORD Amount) {

        INET_ASSERT((int)Amount >= 0);

        _DataAvailable = Amount;
    }

    DWORD AvailableDataLength(VOID) const {

        INET_ASSERT((int)_DataAvailable >= 0);

        return _DataAvailable;
    }

    BOOL IsDataAvailable(VOID) {

        INET_ASSERT((int)_DataAvailable >= 0);

        return (_DataAvailable != 0) ? TRUE : FALSE;
    }

    VOID ReduceAvailableDataLength(DWORD Amount) {

        //
        // why would Amount be > _DataAvailable?
        //

        if (Amount > _DataAvailable) {
            _DataAvailable = 0;
        } else {
            _DataAvailable -= Amount;
        }

        INET_ASSERT((int)_DataAvailable >= 0);

    }

    VOID IncreaseAvailableDataLength(DWORD Amount) {
        _DataAvailable += Amount;

        INET_ASSERT((int)_DataAvailable >= 0);

    }

    VOID SetEndOfFile(VOID) {
        _EndOfFile = TRUE;
    }

    VOID ResetEndOfFile(VOID) {
        _EndOfFile = FALSE;
    }

    BOOL IsEndOfFile(VOID) const {
        return _EndOfFile;
    }

    INTERNET_STATUS_CALLBACK GetStatusCallback(VOID) {
        return ((_StatusCallbackType==FALSE) ? _StatusCallback : UnicodeStatusCallbackWrapper);
    }

    INTERNET_STATUS_CALLBACK GetTrueStatusCallback(VOID) {
        return ((_StatusCallbackType==FALSE) ? NULL : _StatusCallback);
    }

    VOID ResetStatusCallback(VOID) {
        _StatusCallback = NULL;
        _StatusCallbackType = FALSE;
    }

    //VOID AcquireAsyncSpinLock(VOID);
    //
    //VOID ReleaseAsyncSpinLock(VOID);

    DWORD ExchangeStatusCallback(LPINTERNET_STATUS_CALLBACK lpStatusCallback, BOOL fType);

    //DWORD AddAsyncRequest(BOOL fNoCallbackOK);
    //
    //VOID RemoveAsyncRequest(VOID);
    //
    //DWORD GetAsyncRequestCount(VOID) {
    //
    //    //
    //    // it doesn't matter about locking this variable - it can change before
    //    // we have returned it to the caller anyway
    //    //
    //
    //    return _PendingAsyncRequests;
    //}

    // random methods on flags

    DWORD GetInternetOpenFlags() {
        return _dwInternetOpenFlags;
    }

#ifdef POST_BETA

    DWORD GetFlags(VOID) const {
        return _Flags;
    }

    VOID SetFlags(DWORD Flags) {
        _Flags |= Flags;
    }

    VOID ResetFlags(DWORD Flags) {
        _Flags &= ~Flags;
    }

#endif

    VOID
    SetAbortHandle(
        IN ICSocket * pSocket
        );

    ICSocket * GetAbortHandle(VOID) const {
        return _pICSocket;
    }

    VOID
    ResetAbortHandle(
        VOID
        );

    virtual
    VOID
    AbortSocket(
        VOID
        );

    DWORD
    IndicateStatus(
        IN DWORD dwStatus,
        IN LPVOID lpBuffer,
        IN DWORD dwLength
        );

    BOOL IsFromCacheTimeoutSet(VOID) const {
        return _FromCacheTimeout != (DWORD)-1;
    }
};

//
// prototypes
//

HANDLE_OBJECT *
ContainingHandleObject(
    IN LPVOID lpAddress
    );

VOID
CancelActiveSyncRequests(
    IN DWORD dwError
    );

#endif // _HINET_
