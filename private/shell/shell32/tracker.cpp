//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       tracker.cxx
//
//  Contents:   Implementation of Cairo link tracking search.
//
//  Classes:    CTracker
//
//  Functions:  Tracker_InitFromHandle
//              Tracker_GetSize
//              Tracker_Save
//              Tracker_Load
//              Tracker_IsDirty
//              Tracker_Search
//
//  History:    1-Mar-95   BillMo      Created.
//              21-Sep-95  MikeHill    Added Tracker_SetCreationFlags()
//                                     and Tracker_GetCreationFlags()
//              21-Nov-95  MikeHill    Moved dwTickCountDeadline into the
//                                     ResolveSearchData and used new
//                                     TimeoutExpired & DeltaTickCount routines.
//              01-Dec-96  MikeHill    Converted to new NT5 implementation.
//
//  Codework:
//
//--------------------------------------------------------------------------


#ifdef WINNT

#include "shellprv.h"
#pragma hdrstop

#define LINKDATA_AS_CLASS
#include <linkdata.hxx>
#include "tracker.h"

#include "shelllnk.h"


// Private interface used for testing

EXTERN_C
const IID IID_ISLTracker = { /* 7c9e512f-41d7-11d1-8e2e-00c04fb9386d */
    0x7c9e512f,
    0x41d7,
    0x11d1,
    {0x8e, 0x2e, 0x00, 0xc0, 0x4f, 0xb9, 0x38, 0x6d}
  };



EXTERN_C DWORD GetTimeOut(DWORD uFlags);
EXTERN_C WORD wDebugMask;

// BUGBUG:  These four inlines are copied from private\net\svcdlls\trksvcs\common\trklib.hxx
// They should be moved to linkdata.hxx

inline
CDomainRelativeObjId::operator == (const CDomainRelativeObjId &Other) const
{
    return(_volume == Other._volume && _object == Other._object);
}

inline
CDomainRelativeObjId::operator != (const CDomainRelativeObjId &Other) const
{
    return !(*this == Other);
}

inline
CVolumeId:: operator == (const CVolumeId & Other) const
{
    return( 0 == memcmp( &_volume, &Other._volume, sizeof(_volume) ) );
}

inline
CVolumeId:: operator != (const CVolumeId & Other) const
{
    return ! (Other == *this);
}



//+----------------------------------------------------------------------------
//
//  Function:   Thunk Implementations
//
//  Synopsis:   These C routines thunk from C code to the CTracker class.
//
//+----------------------------------------------------------------------------


STDAPI Tracker_Search(struct CTracker *pThis,
                       DWORD dwTickCountDeadline,
                       const WIN32_FIND_DATA *pfdIn,
                       WIN32_FIND_DATA *pfdOut, 
                       UINT uShlinkFlags,
                       DWORD TrackerRestrictions )
{
    return pThis->Search(dwTickCountDeadline, pfdIn, pfdOut, uShlinkFlags, TrackerRestrictions);
}

STDAPI Tracker_CancelSearch(struct CTracker *pThis )
{
    return pThis->CancelSearch();
}


//+----------------------------------------------------------------------------
//
//  Function:   RPC free/alloc routines
//
//  Synopsis:   CTracker uses MIDL-generated code to call an RPC server,
//              and MIDL-generated code assumes that the following routines
//              be provided.
//
//+----------------------------------------------------------------------------

void __RPC_USER MIDL_user_free( void __RPC_FAR *pv ) 
{ 
    LocalFree(pv); 
}


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t s) 
{ 
    return (void __RPC_FAR *) LocalAlloc(LMEM_FIXED, s); 
}



//+----------------------------------------------------------------------------
//
//  Method:     IUnknown methods
//
//  Synopsis:   IUnknown methods for the ISLTracker interface.
//
//+----------------------------------------------------------------------------

HRESULT CTracker::QueryInterface(REFIID riid, void **ppvObj)
{
    return _pshlink->QueryInterface(riid, ppvObj);
}

ULONG CTracker::AddRef()
{
    return _pshlink->AddRef();
}

ULONG CTracker::Release()
{
    return _pshlink->Release();
}



//+----------------------------------------------------------------------------
//
//  Method:     ISLTracker custom methods
//
//  Synopsis:   This interface is private and is only used for testing.
//              This provides test programs the ability to specify the
//              TrackerRestrictions (from the TrkMendRestrictions enum)
//              and the ability to get the internal IDs.
//
//+----------------------------------------------------------------------------

HRESULT CTracker::Resolve(HWND hwnd, DWORD fFlags, DWORD TrackerRestrictions)
{
    return CShellLink::ResolveCallback(_pshlink, hwnd, fFlags, TrackerRestrictions);
}

HRESULT CTracker::GetIDs(CDomainRelativeObjId *pdroidBirth, CDomainRelativeObjId *pdroidLast, CMachineId *pmcid)
{
    if( !_fLoaded )
        return E_UNEXPECTED;

    *pdroidBirth = _droidBirth;
    *pdroidLast = _droidLast;
    *pmcid = _mcidLast;

    return S_OK;
}




//+----------------------------------------------------------------------------
//
//  Method:     CTracker::InitRPC
//
//  Synopsis:   Initializes the data members used for RPC.  This should be
//              called either by InitNew or Load.
//
//  Arguments:  None
//
//  Returns:    [HRESULT]
//
//+----------------------------------------------------------------------------


HRESULT CTracker::InitRPC()
{
    HRESULT hr = S_OK;

    if( !_fCritsecInitialized )
    {
        InitializeCriticalSection( &_cs );
        _fCritsecInitialized = TRUE;
    }

    if( NULL == _pRpcAsyncState )
    {
        _pRpcAsyncState = reinterpret_cast<PRPC_ASYNC_STATE>( new BYTE[ sizeof(RPC_ASYNC_STATE) ] );
        if( NULL == _pRpcAsyncState )
        {
            hr = HRESULT_FROM_WIN32( E_OUTOFMEMORY );
            goto Exit;
        }
    }

    if( NULL == _hEvent )
    {
        _hEvent = CreateEvent( NULL, FALSE, FALSE, NULL ); // Manual reset, not initially signaled
        if( NULL == _hEvent )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Exit;
        }
    }

Exit:

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Method:     CTracker::InitNew
//
//  Synopsis:   Initializes the CTracker object.  This method may be called
//              repeatedly, i.e. it may be called to clear/reinit the object.
//              This method need not be called before calling the Load method.
//
//  Arguments:  None
//
//  Returns:    [HRESULT]
//
//+----------------------------------------------------------------------------

HRESULT
CTracker::InitNew(  )
{
    HRESULT hr = InitRPC();
    if (SUCCEEDED(hr)) 
    {
        _mcidLast = CMachineId();
        _droidLast = CDomainRelativeObjId();
        _droidBirth = CDomainRelativeObjId();

        _fDirty = FALSE;
        _fLoaded = FALSE;
        _fMendInProgress = FALSE;
    }
    return hr;
}   // CTracker::InitNew()


//+----------------------------------------------------------------------------
//
//  Member:     CTracker::InitFromHandle
//
//  Synopsis:   Get tracking state from the given file handle.  Note that this
//              is expected to fail if the referrent file isn't on an
//              NTFS5 volume.
//          
//
//  Arguments:  [hFile]
//                  The file to track
//              [ptszFile]
//                  The name of the file
//
//  Returns:    [HRESULT]
//
//-----------------------------------------------------------------------------

HRESULT
CTracker::InitFromHandle( const HANDLE hFile, const TCHAR* ptszFile )
{
    HRESULT hr = E_FAIL;
    NTSTATUS status = STATUS_SUCCESS;

    FILE_OBJECTID_BUFFER fobOID;
    DWORD cbReturned;

    CDomainRelativeObjId droidLast;
    CDomainRelativeObjId droidBirth;
    CMachineId           mcidLast;

    // Initialize the RPC members

    hr = InitRPC();
    if( FAILED(hr) ) goto Exit;

    //  -----------------------------------
    //  Get the Object ID Buffer (64 bytes)
    //  -----------------------------------

    // Use the file handle to get the file's Object ID.  Tell the filesystem to give us the
    // existing object ID if the file already has one, or to create a new one otherwise.

    memset( &fobOID, 0, sizeof(fobOID) );

    if( !DeviceIoControl( hFile, FSCTL_CREATE_OR_GET_OBJECT_ID,
                          NULL, 0,                      // No input buffer
                          &fobOID, sizeof(fobOID),      // Output buffer
                          &cbReturned, NULL ))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

    //  ----------------------
    //  Load the Droids & MCID
    //  ----------------------

    status = droidLast.InitFromFile( hFile, fobOID );
    if( !NT_SUCCESS(status) )
    {
        hr = HRESULT_FROM_WIN32( RtlNtStatusToDosError(status) );
        goto Exit;
    }

    droidBirth.InitFromFOB( fobOID );
    droidBirth.GetVolumeId().Normalize();

    if( FAILED(mcidLast.InitFromPath( ptszFile, hFile )) )
        mcidLast = CMachineId();

    //  ----
    //  Exit
    //  ----

    if( _mcidLast   != mcidLast
        ||
        _droidLast  != droidLast
        ||
        _droidBirth != droidBirth
      )
    {
        _mcidLast   = mcidLast;
        _droidLast  = droidLast;
        _droidBirth = droidBirth;
        _fDirty = TRUE;
    }

    _fLoaded = TRUE;            // Cleared in InitNew
    _fLoadedAtLeastOnce = TRUE; // Not cleared in InitNew

    hr = S_OK;

Exit:

    if( FAILED(hr) )
        DebugMsg( DM_TRACE, TEXT("CTracker::InitFromHandle failed (%08X)"), hr );

    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CTracker::Load
//
//  Synopsis:   Load the tracker from the memory buffer.  The InitNew
//              method need not be called before calling this method.
//
//  Arguments:  [pb] -- buffer to load from
//              [cb] -- size of pb buffer
//
//  Returns:    [HRESULT]
//
//--------------------------------------------------------------------

HRESULT
CTracker::Load( BYTE *pb, ULONG cb )
{
    HRESULT hr = E_FAIL;
    DWORD dwLength;

    // Initialize RPC if it hasn't been already.

    hr = InitRPC();
    if( FAILED(hr) ) goto Exit;

    // Check the length

    dwLength = *reinterpret_cast<DWORD*>(pb);
    if( dwLength < GetSize() )
    {
        hr = E_INVALIDARG;
        goto Exit;
    }
    pb += sizeof(dwLength);

    // Check the version number

    if( CTRACKER_VERSION != *reinterpret_cast<DWORD*>(pb) )
    {
        hr = HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
        goto Exit;
    }

    pb += sizeof(DWORD);    // Skip past the version

    // Get the machine ID & droids

    _mcidLast = *reinterpret_cast<CMachineId*>(pb);
    pb += sizeof(_mcidLast );

    _droidLast = *reinterpret_cast<CDomainRelativeObjId*>(pb);
    pb += sizeof(_droidLast);

    _droidBirth = *reinterpret_cast<CDomainRelativeObjId*>(pb);
    pb += sizeof(_droidBirth);

    _fLoaded = TRUE;            // Cleared in InitNew
    _fLoadedAtLeastOnce = TRUE; // Not cleared in InitNew


    hr = S_OK;

Exit:

    if( FAILED(hr) )
        DebugMsg( DM_TRACE, TEXT("CTracker::Load failed (%08X)"), hr );

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CTracker::Save
//
//  Synopsis:   Save tracker to the given buffer.
//
//  Arguments:  [pb]     -- buffer for tracker.
//              [cbSize] -- size of buffer in pb
//
//  Returns:    None
//
//--------------------------------------------------------------------

VOID
CTracker::Save( BYTE *pb, ULONG cbSize )
{
    // Save the length
    *reinterpret_cast<DWORD*>(pb) = GetSize();
    pb += sizeof(DWORD);

    // Save a version number
    *reinterpret_cast<DWORD*>(pb) = CTRACKER_VERSION;
    pb += sizeof(DWORD);

    // Save the machine & DROIDs

    *reinterpret_cast<CMachineId*>(pb) = _mcidLast;
    pb += sizeof(_mcidLast);

    *reinterpret_cast<CDomainRelativeObjId*>(pb) = _droidLast;
    pb += sizeof(_droidLast);

    *reinterpret_cast<CDomainRelativeObjId*>(pb) = _droidBirth;
    pb += sizeof(_droidBirth);

    _fDirty = FALSE;

    return;

}   // CTracker::Save()






//+-------------------------------------------------------------------
//
//  Member:     CTracker::Search
//
//  Synopsis:   Search for the object referred to by the tracker.
//
//  Arguments:  [dwTickCountDeadline] -- absolute tick count for deadline
//              [pfdIn]               -- may not be NULL
//              [pfdOut]              -- may not be NULL
//                                       will contain updated data on success
//              [uShlinkFlags]       -- SLR_ flags
//              [TrackerRestrictions] -- TrkMendRestrictions enumeration
//
//  Returns:    [HRESULT]
//               S_OK
//                  found (pfd contains new info)
//              E_UNEXPECTED
//                  CTracker::InitNew hasn't bee called.
//              HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED)
//                  Restrictions (set in registry) are set such that
//                  this operation isn't to be performed.
//
//--------------------------------------------------------------------


HRESULT
CTracker::Search( const DWORD dwTickCountDeadline,
                  const WIN32_FIND_DATA *pfdIn,
                  WIN32_FIND_DATA *pfdOut,
                  UINT  uShlinkFlags,
                  DWORD TrackerRestrictions)
{
    HRESULT hr = S_OK;
    TCHAR ptszError = NULL;
    WIN32_FILE_ATTRIBUTE_DATA fadNew;
    WIN32_FIND_DATA fdNew = *pfdIn;
    DWORD cbFileName;
    BOOL fPotentialFileFound = FALSE;
    BOOL fLocked = FALSE;
    DWORD dwCurrentTickCount = 0;

    RPC_TCHAR          *ptszStringBinding = NULL;
    RPC_BINDING_HANDLE  BindingHandle;
    RPC_STATUS          rpcstatus;

    CDomainRelativeObjId droidBirth, droidLast, droidCurrent;
    CMachineId mcidCurrent;

    // Initialize the output

    memset( pfdOut, 0, sizeof(*pfdOut) );

    // Abort if restrictions don't allow this operation

    if (SHRestricted(REST_NORESOLVETRACK) ||
        (SLR_NOTRACK & uShlinkFlags))
    {
        hr = HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED);
        goto Exit;
    }

    // Ensure that we've been loaded first

    else if( !_fLoaded )
    {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    // Capture the current tick count

    dwCurrentTickCount = GetTickCount();

    if( (long) dwTickCountDeadline <= (long) dwCurrentTickCount )
    {
        hr = HRESULT_FROM_WIN32( ERROR_SERVICE_REQUEST_TIMEOUT );
        goto Exit;
    }


    //
    // Create an RPC binding
    //

    rpcstatus = RpcStringBindingCompose(NULL,
                                        TEXT("ncalrpc"),
                                        NULL,
                                        TRKWKS_LRPC_ENDPOINT_NAME,
                                        NULL,
                                        &ptszStringBinding);

    if( RPC_S_OK == rpcstatus )
        rpcstatus = RpcBindingFromStringBinding(ptszStringBinding, &BindingHandle);

    if( RPC_S_OK != rpcstatus )
    {
        hr = HRESULT_FROM_WIN32( rpcstatus );
        goto Exit;
    }

    //
    // Initialize an RPC Async handle
    //

    //  Take the lock
    EnterCriticalSection( &_cs );  fLocked = TRUE;

    // Verify that we were initialized properly
    if( NULL == _hEvent || NULL == _pRpcAsyncState )
    {
        hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
        goto Exit;
    }
    
    rpcstatus = RpcAsyncInitializeHandle( _pRpcAsyncState, RPC_ASYNC_VERSION_1_0 );
    if( RPC_S_OK != rpcstatus )
    {
        hr = HRESULT_FROM_WIN32( rpcstatus );
        goto Exit;
    }

    _pRpcAsyncState->NotificationType = RpcNotificationTypeEvent;
    _pRpcAsyncState->u.hEvent = _hEvent;
    _pRpcAsyncState->UserInfo = NULL;


    //
    // Call the tracking service to find the file
    //

    cbFileName = sizeof(fdNew.cFileName);
    droidLast = _droidLast;
    droidBirth = _droidBirth;
    mcidCurrent = _mcidLast;

    __try
    {
        SYSTEMTIME stNow;
        FILETIME ftDeadline;
        DWORD dwDeltaMillisecToDeadline;

        // Convert the tick-count deadline into a UTC filetime.

        dwDeltaMillisecToDeadline = (DWORD)( (long)dwTickCountDeadline - (long)dwCurrentTickCount );
        GetSystemTime( &stNow );
        SystemTimeToFileTime( &stNow, &ftDeadline );
        *reinterpret_cast<LONGLONG*>(&ftDeadline) += ( dwDeltaMillisecToDeadline * 10*1000 );

        // Start the async RPC call to the tracking service

        _fMendInProgress = TRUE;
        LnkMendLink( _pRpcAsyncState,
                     BindingHandle,
                     ftDeadline,
                     TrackerRestrictions,
                     const_cast<CDomainRelativeObjId*>(&droidBirth),
                     const_cast<CDomainRelativeObjId*>(&droidLast),
                     const_cast<CMachineId*>(&_mcidLast),
                     &droidCurrent,
                     &mcidCurrent,
                     &cbFileName,
                     fdNew.cFileName );

        // Wait for the call to return.  Release the lock first, though, so that
        // the UI thread can come in and cancel.

        LeaveCriticalSection( &_cs ); fLocked = FALSE;
        DWORD dwWaitReturn = WaitForSingleObject( _hEvent, dwDeltaMillisecToDeadline );

        // Now take the lock back and see what happenned.

        EnterCriticalSection( &_cs ); fLocked = TRUE;
        _fMendInProgress = FALSE;

        if( WAIT_TIMEOUT == dwWaitReturn )
        {
            // We timed out waiting for a response.  Cancel the call.
            // If the call should complete between the time
            // we exited the WaitForSingleObject and the cancel call below,
            // then the cancel will be ignored by RPC.

            rpcstatus = RpcAsyncCancelCall( _pRpcAsyncState, TRUE ); // fAbort
            if( RPC_S_OK != rpcstatus )
            {
                hr = HRESULT_FROM_WIN32(rpcstatus);
                __leave;
            }
        }
        else if( WAIT_OBJECT_0 != dwWaitReturn )
        {
            // There was an error of some kind.
            hr = HRESULT_FROM_WIN32( GetLastError() );
            __leave;
        }

        // Now we find out how the LnkMendLink call completed.  If we get
        // RPC_S_OK, then it completed normally, and the result is
        // in hr.

        rpcstatus = RpcAsyncCompleteCall( _pRpcAsyncState, &hr );
        if( RPC_S_OK != rpcstatus )
        {
            // The call either failed or was cancelled (the reason for the
            // cancel would be that the UI thread called CTracker::CancelSearch,
            // or because we timed out above and called RpcAsyncCancelCall).

            hr = HRESULT_FROM_WIN32(rpcstatus);
            __leave;
        }

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        _fMendInProgress = FALSE;
        hr = HRESULT_FROM_WIN32( RpcExceptionCode() );
    }


    // free the binding
    RpcBindingFree(&BindingHandle);

    if( HRESULT_FROM_WIN32(ERROR_POTENTIAL_FILE_FOUND) == hr )
    {
        fPotentialFileFound = TRUE;
        hr = S_OK;
    }

    if( FAILED(hr) ) goto Exit;

    //
    // See if this is in the recycle bin
    //

    if( IsFileInBitBucket( fdNew.cFileName ))
    {
        hr = E_FAIL;
        goto Exit;
    }


    //
    // Now that we know what the new filename is, let's get all
    // the FindData info.
    //

    if( !GetFileAttributesEx( fdNew.cFileName, GetFileExInfoStandard, &fadNew ))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

    // Ensure that the file we found has the same "directory-ness"
    // as the last known link source (either they're both a directory
    // or they're both a file).  Also ensure that the file we found
    // isn't itself a link client (a shell shortcut).

    if( ( (fadNew.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
          ^
          (pfdIn->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        )
        ||
        PathIsLnk( fdNew.cFileName )
      )
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto Exit;
    }

    // Copy the file attributes into the WIN32_FIND_DATA structure.

    fdNew.dwFileAttributes = fadNew.dwFileAttributes;
    fdNew.ftCreationTime = fadNew.ftCreationTime;
    fdNew.ftLastAccessTime = fadNew.ftLastAccessTime;
    fdNew.ftLastWriteTime = fadNew.ftLastWriteTime;
    fdNew.nFileSizeLow = fadNew.nFileSizeLow;

    // Return the new finddata to the caller.

    *pfdOut = fdNew;

    // Update our local state

    if( _droidLast != droidCurrent
        ||
        _droidBirth != droidBirth
        ||
        _mcidLast != mcidCurrent
      )
    {
        _droidLast = droidCurrent;
        _droidBirth = droidBirth;
        _mcidLast = mcidCurrent;

        _fDirty = TRUE;
    }


    //  ----
    //  Exit
    //  ----

Exit:

    if( fLocked )
        LeaveCriticalSection( &_cs );

    if(ptszStringBinding)
        RpcStringFree(&ptszStringBinding);

    if( FAILED(hr) )
        DebugMsg( DM_TRACE, TEXT("CTracker::Search failed (hr=0x%08X)"), hr );
    else if( fPotentialFileFound )
        hr = HRESULT_FROM_WIN32(ERROR_POTENTIAL_FILE_FOUND);

    return(hr);

}   // CTracker::Search()




//+----------------------------------------------------------------------------
//
//  Member:     CTracker::CancelSearch
//
//  Synopsis:   This method is called on a thread signal another thread
//              which is in CTracker::Search to abort the LnkMendLink
//              call.
//
//  Returns:    [HRESULT]
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CTracker::CancelSearch()
{
    HRESULT hr = S_OK;
    RPC_STATUS rpcstatus = RPC_S_OK;

    EnterCriticalSection( &_cs );
    
    // If a search is in progress, cancel it.

    if( _fMendInProgress && NULL != _pRpcAsyncState )
    {
        rpcstatus = RpcAsyncCancelCall( _pRpcAsyncState, TRUE ); // fAbort

        // This should never fail, because the critsec ensures we have a valid
        // async handle.

        if( RPC_S_OK != rpcstatus )
            hr = HRESULT_FROM_WIN32( rpcstatus );
    }

    LeaveCriticalSection( &_cs );

    return hr;
}



//+----------------------------------------------------------------------------
//
//  Function:   GetServerComputerName
//              GetRemoteServerComputerName
//              ScanForComputerName (helper function)
//              ConvertDfsPath (helper function)
//
//  Look at a path and determine the computer name of the host machine.
//  In the future, we should remove this code, and add the capbility to query
//  handles for their computer name.
//
//  GetServerComputer name uses ScanForComputerName and ConvertDfsPath
//  as helper functions.
//
//  The name can only be obtained for NetBios paths - if the path is IP or DNS
//  an error is returned.  (If the NetBios name has a "." in it, it will
//  cause an error because it will be misinterpreted as a DNS path.  This case
//  becomes less and less likely as the NT5 UI doesn't allow such computer names.)
//  For DFS paths, the leaf server's name is returned, as long as it wasn't
//  joined to its parent with an IP or DNS path name.
//
//+----------------------------------------------------------------------------

const UNICODE_STRING NtUncPathNamePrefix = { 16, 18, L"\\??\\UNC\\"};
#define cchNtUncPathNamePrefix  8

const UNICODE_STRING NtDrivePathNamePrefix = { 8, 10, L"\\??\\" };
#define cchNtDrivePathNamePrefix  4

const WCHAR RedirectorMappingPrefix[] = { L"\\Device\\LanmanRedirector\\;" };
const WCHAR LocalVolumeMappingPrefix[] = { L"\\Device\\Volume" };
const WCHAR CDRomMappingPrefix[] = { L"\\Device\\CDRom" };
const WCHAR FloppyMappingPrefix[] = { L"\\Device\\Floppy" };
const WCHAR DfsMappingPrefix[] = { L"\\Device\\WinDfs\\" };


//
//  ScanForComputerName:
//
//  Scan the path in ServerFileName (which is a UNICODE_STRING with
//  a full NT path name), searching for the computer name.  If it's
//  found, point to it with UnicodeComputerName.Buffer, and set
//  *AvailableLength to show how much readable memory is after that
//  point.
//

HRESULT
ScanForComputerName( HANDLE hFile,
                     const UNICODE_STRING &ServerFileName,
                     UNICODE_STRING *UnicodeComputerName,
                     ULONG *AvailableLength,
                     WCHAR *DosDeviceMapping, ULONG cchDosDeviceMapping,
                     PFILE_NAME_INFORMATION FileNameInfo, ULONG cbFileNameInfo,
                     BOOL *CheckForDfs )
{

    HRESULT hr = S_OK;

    // Is this a UNC path?

    if( RtlPrefixString( (PSTRING)&NtUncPathNamePrefix, (PSTRING)&ServerFileName, TRUE )) {

        // Make sure there's some more to this path than just the prefix
        if( ServerFileName.Length <= NtUncPathNamePrefix.Length )
        {
            hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
            goto Exit;
        }

        // It appears to be a valid UNC path.  Point to the beginning of the computer
        // name, and calculate how much room is left in ServerFileName after that.

        UnicodeComputerName->Buffer = &ServerFileName.Buffer[ NtUncPathNamePrefix.Length/sizeof(WCHAR) ];
        *AvailableLength = ServerFileName.Length - NtUncPathNamePrefix.Length;

    }

    // If it's not a UNC path, then is it a drive-letter path?

    else if( RtlPrefixString( (PSTRING)&NtDrivePathNamePrefix, (PSTRING)&ServerFileName, TRUE )
             &&
             ServerFileName.Buffer[ cchNtDrivePathNamePrefix + 1 ] == L':' ) {

        // Get the correct, upper-cased, drive letter into DosDevice.

        WCHAR DosDevice[3] = { L"A:" };

        DosDevice[0] = ServerFileName.Buffer[ cchNtDrivePathNamePrefix ];
        if( L'a' <= DosDevice[0] && DosDevice[0] <= L'z' )
            DosDevice[0] = L'A' + (DosDevice[0] - L'a');

        // Map the drive letter to its symbolic link under \??.  E.g., say D: & R:
        // are DFS/rdr drives, you would then see something like:
        //
        //   D: => \Device\WinDfs\G
        //   R: => \Device\LanmanRedirector\;R:0\scratch\scratch

        if( !QueryDosDevice( DosDevice, DosDeviceMapping, cchDosDeviceMapping ))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        // Now that we have the DosDeviceMapping, we can check ... Is this a rdr drive?

        if( // Does it begin with "\Device\LanmanRedirector\;" ?
            0 == wcsncmp( DosDeviceMapping, RedirectorMappingPrefix, lstrlenW(RedirectorMappingPrefix) )
            &&
            // Are the next letters the correct drive letter, a colon, and a whack?
            ( DosDevice[0] == DosDeviceMapping[ sizeof(RedirectorMappingPrefix)/sizeof(WCHAR) - 1 ]
              &&
              L':' == DosDeviceMapping[ sizeof(RedirectorMappingPrefix)/sizeof(WCHAR) ]
              &&
              (UnicodeComputerName->Buffer = StrChrW(&DosDeviceMapping[ sizeof(RedirectorMappingPrefix)/sizeof(WCHAR) + 1 ], L'\\'))
            ))
        {
            // We have a valid rdr drive.  Point to the beginning of the computer
            // name, and calculate how much room is availble in DosDeviceMapping after that.

            UnicodeComputerName->Buffer += 1;
            *AvailableLength = sizeof(DosDeviceMapping) - sizeof(DosDeviceMapping[0]) * (ULONG)(UnicodeComputerName->Buffer - DosDeviceMapping);

            // We know now that it's not a DFS path
            *CheckForDfs = FALSE;

        }

        // Or, is it a DFS drive?

        else if( 0 == wcsncmp( DosDeviceMapping, DfsMappingPrefix, lstrlenW(DfsMappingPrefix) ))
        {

            // Get the full UNC name of this DFS path.  Later, we'll call the DFS
            // driver to find out what the actual server name is.

            NTSTATUS NtStatus;
            IO_STATUS_BLOCK IoStatusBlock;

            NtStatus = NtQueryInformationFile(
                        hFile,
                        &IoStatusBlock,
                        FileNameInfo,
                        cbFileNameInfo, 
                        FileNameInformation
                        );
            if( !NT_SUCCESS(NtStatus) ) {
                hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
                goto Exit;
            }

            UnicodeComputerName->Buffer = FileNameInfo->FileName + 1;
            *AvailableLength = FileNameInfo->FileNameLength;
        }

        // We don't recognize this path type

        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
            goto Exit;
        }

    }   // else if( RtlPrefixString( (PSTRING)&NtDrivePathNamePrefix, (PSTRING)&ServerFileName, TRUE ) ...

    else {
        hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
        goto Exit;
    }


Exit:

    return hr;
}


//
//  ConvertDfsPath:
//
//  Try to convert the path name pointed to by UnicodeComputerName.Buffer
//  into a DFS path name.  The caller provides DfsServerPathName as a buffer
//  for the converted name.  If it's a DFS path, then update UnicodeComputerName.Buffer
//  to point to the conversion, otherwise leave it unchanged.
//

HRESULT
ConvertDfsPath( HANDLE hFile, UNICODE_STRING *UnicodeComputerName, ULONG *AvailableLength,
                WCHAR *DfsServerPathName, ULONG cbDfsServerPathName )
{
    HRESULT hr = S_OK;
    HANDLE hDFS = INVALID_HANDLE_VALUE;
    UNICODE_STRING DfsDriverName;
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;

    WCHAR *DfsPathName = UnicodeComputerName->Buffer - 1;    // Back up to the whack
    ULONG DfsPathNameLength = *AvailableLength + sizeof(WCHAR);

    // Open the DFS driver

    RtlInitUnicodeString( &DfsDriverName, L"\\Dfs" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &DfsDriverName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                            );

    NtStatus = NtCreateFile(
                    &hDFS,
                    SYNCHRONIZE,
                    &ObjectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
                );

    if( !NT_SUCCESS(NtStatus) ) {
        hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
        goto Exit;
    }

    // Query DFS's cache for the server name.  The name is guaranteed to
    // remain in the cache as long as the file is open.

    if( L'\\' != DfsPathName[0] ) {
        NtClose( hDFS );
        hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
        goto Exit;
    }

    NtStatus = NtFsControlFile(
                    hDFS,
                    NULL,       // Event,
                    NULL,       // ApcRoutine,
                    NULL,       // ApcContext,
                    &IoStatusBlock,
                    FSCTL_DFS_GET_SERVER_NAME,
                    DfsPathName,
                    DfsPathNameLength,
                    DfsServerPathName,
                    cbDfsServerPathName );
    NtClose( hDFS );

    // STATUS_OBJECT_NAME_NOT_FOUND means that it's not a DFS path
    if( !NT_SUCCESS(NtStatus) ) {

        if( STATUS_OBJECT_NAME_NOT_FOUND != NtStatus  ) {
            hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
            goto Exit;
        }
    }
    else if( L'\0' != DfsServerPathName[0] ) {

        // The previous DFS call returns the server-specific path to the file in UNC form.
        // Point UnicodeComputerName to just past the two whacks.

        *AvailableLength = lstrlenW(DfsServerPathName) * sizeof(WCHAR);
        if( 3*sizeof(WCHAR) > *AvailableLength
            ||
            L'\\' != DfsServerPathName[0]
            ||
            L'\\' != DfsServerPathName[1] )
        {
            hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
            goto Exit;
        }

        UnicodeComputerName->Buffer = DfsServerPathName + 2;
        *AvailableLength -= 2 * sizeof(WCHAR);
    }

Exit:

    return hr;

}


//
//  GetRemoteServerComputerName:
//
//  Take pwszFile, which is a path to a remote machine, and get the 
//  server machine's computer name.
//

HRESULT
GetRemoteServerComputerName(
    const WCHAR *pwszFile,
    HANDLE hFile,
    WCHAR *pwszComputer )
{
    HRESULT hr = S_OK;
    ULONG cbComputer = 0;
    ULONG AvailableLength = 0;
    PWCHAR PathCharacter = NULL;
    BOOL CheckForDfs = TRUE;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    WCHAR FileNameInfoBuffer[MAX_PATH+sizeof(FILE_NAME_INFORMATION)];
    PFILE_NAME_INFORMATION FileNameInfo = (PFILE_NAME_INFORMATION)FileNameInfoBuffer;
    WCHAR DfsServerPathName[ MAX_PATH + 1 ];
    WCHAR DosDeviceMapping[ MAX_PATH + 1 ];

    UNICODE_STRING UnicodeComputerName;
    UNICODE_STRING ServerFileName;

    // Canonicalize the file name into the NT object directory namespace.

    RtlInitUnicodeString( &UnicodeComputerName, NULL );
    RtlInitUnicodeString( &ServerFileName, NULL );
    if( !RtlDosPathNameToNtPathName_U( pwszFile, &ServerFileName, NULL, NULL ))
    {
        hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
        goto Exit;
    }

    // Point UnicodeComputerName.Buffer at the beginning of the computer name.

    hr = ScanForComputerName( hFile, ServerFileName, &UnicodeComputerName, &AvailableLength,
                              DosDeviceMapping, ARRAYSIZE(DosDeviceMapping),
                              FileNameInfo, sizeof(FileNameInfoBuffer), &CheckForDfs );
    if( FAILED(hr) ) goto Exit;

    // If there was no error but we don't have a computer name, then the file is on 
    // the local computer.

    if( NULL == UnicodeComputerName.Buffer )
    {
        DWORD cchName = MAX_COMPUTERNAME_LENGTH + 1;
        hr = S_OK;

        if( !GetComputerNameW( pwszComputer, &cchName) )
            hr = HRESULT_FROM_WIN32(GetLastError());

        goto Exit;
    }

    // If we couldn't determine above whether or not this is a DFS path, let the
    // DFS driver decide now.

    if( CheckForDfs && INVALID_HANDLE_VALUE != hFile ) {

        // On return, UnicodeComputerName.Buffer points to the leaf machine's
        // UNC name if it's a DFS path.  If it's not a DFS path, 
        // .Buffer is left unchanged.

        hr = ConvertDfsPath( hFile, &UnicodeComputerName, &AvailableLength,
                             DfsServerPathName, sizeof(DfsServerPathName) );
        if( FAILED(hr) ) goto Exit;
    }

    // If we get here, then the computer name\share is pointed to by UnicodeComputerName.Buffer.
    // But the Length is currently zero, so we search for the whack that separates
    // the computer name from the share, and set the Length to include just the computer name.

    PathCharacter = UnicodeComputerName.Buffer;

    while( ( (ULONG) ((PCHAR)PathCharacter - (PCHAR)UnicodeComputerName.Buffer) < AvailableLength)
           &&
           *PathCharacter != L'\\' ) {

        // If we found a '.', we fail because this is probably a DNS or IP name.
        if( L'.' == *PathCharacter ) {
            hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
            goto Exit;
        }

        PathCharacter++;
    }

    // Set the computer name length

    UnicodeComputerName.Length = UnicodeComputerName.MaximumLength
        = (USHORT) ((PCHAR)PathCharacter - (PCHAR)UnicodeComputerName.Buffer);

    // Fail if the computer name exceeded the length of the input ServerFileName,
    // or if the length exceeds that allowed.

    if( UnicodeComputerName.Length >= AvailableLength
        ||
        UnicodeComputerName.Length > MAX_COMPUTERNAME_LENGTH*sizeof(WCHAR) ) {
        goto Exit;
    }

    // Copy the computer name into the caller's buffer, as long as there's enough
    // room for the name & a terminating '\0'.

    if( UnicodeComputerName.Length + sizeof(WCHAR) > (MAX_COMPUTERNAME_LENGTH+1)*sizeof(WCHAR) ) {
        hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
        goto Exit;
    }

    RtlCopyMemory( pwszComputer, UnicodeComputerName.Buffer, UnicodeComputerName.Length );
    pwszComputer[ UnicodeComputerName.Length / sizeof(WCHAR) ] = L'\0';

    hr = S_OK;


Exit:

    RtlFreeHeap(RtlProcessHeap(), 0, ServerFileName.Buffer);
    return hr;

}


//
//  GetServerComputerName:
//
//  Give a file's path & handle, determine the computer name of the server
//  on which that file resides (which could just be this machine).
//

HRESULT
GetServerComputerName(
    const WCHAR *pwszFile,
    HANDLE hFile,
    WCHAR *pwszComputer )
{
    HRESULT hr = S_OK;
    WCHAR wszAbsoluteName[ MAX_PATH + 1 ];
    WCHAR *pwszFilePart = NULL;
    BOOL fRemote = FALSE;
    UINT DriveType = 0;

    // pwszFile may be a local path name.  Convert it into an absolute
    // name.

    if( 0 == GetFullPathName( pwszFile, ARRAYSIZE(wszAbsoluteName),
                              wszAbsoluteName, &pwszFilePart ))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

    // Check to see if this points to a local or remote drive.  Terminate
    // the path at the beginning of the file name, so that the path ends in
    // a whack.  This allows GetDriveType to determine the type without being
    // give a root path.

    *pwszFilePart = L'\0';
    DriveType = GetDriveType( wszAbsoluteName );

    if( DRIVE_REMOTE == DriveType )
    {
        // We have a remote drive (could be a UNC path or a redirected drive).
        hr = GetRemoteServerComputerName( wszAbsoluteName, hFile, pwszComputer );
    }
    else if( DRIVE_UNKNOWN == DriveType
             ||
             DRIVE_NO_ROOT_DIR == DriveType )
    {
        // We have an unsupported type
        hr = HRESULT_FROM_WIN32( ERROR_BAD_PATHNAME );
    }
    else
    {
        // We have a path to the local machine.

        DWORD cchName = MAX_COMPUTERNAME_LENGTH + 1;
        hr = S_OK;

        if( !GetComputerNameW( pwszComputer, &cchName) )
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    if( FAILED(hr) )
        goto Exit;

Exit:

    return hr;

}





#endif // #ifdef WINNT
