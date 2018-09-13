///////////////////////////////////////////////////////////////////////////////
/*  File: user.cpp

    Description: Contains member function definitions for class DiskQuotaUser.
        The DiskQuotaUser object represents a user's record in a volume's
        quota information file.  The holder of a user object's IDiskQuotaUser
        interface can query and modify information for that user as security
        privileges permit.  A user object is obtained through a UserEnumerator
        object (IEnumDiskQuotaUsers) which is itself obtained through
        IDiskQuotaControl::CreateEnumUsers().

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/20/96    Added m_dwID member to DiskQuotaUser.                BrianAu
    09/05/96    Added exception handling.                            BrianAu
    03/18/98    Replaced "domain", "name" and "full name" with       BrianAu
                "container", "logon name" and "display name" to
                better match the actual contents.  This was in
                reponse to making the quota UI DS-aware.  The
                "logon name" is now a unique key as it contains
                both account name and domain-like information.
                i.e. "REDMOND\brianau" or "brianau@microsoft.com".
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h" // PCH
#pragma hdrstop

#include <comutil.h>
#include "user.h"
#include "sidcache.h"  // For g_pSidCache.
#include "resource.h"  // For IDS_NO_LIMIT.

//
// Verify that build is UNICODE.
//
#if !defined(UNICODE)
#   error This module must be compiled UNICODE.
#endif


//
// Only one of these for all users. (static member).
//
LONG            DiskQuotaUser::m_cUsersAlive        = 0;    // Cnt of users alive now.
ULONG           DiskQuotaUser::m_ulNextUniqueId     = 0;
HANDLE          DiskQuotaUser::m_hMutex             = NULL;
DWORD           DiskQuotaUser::m_dwMutexWaitTimeout = 5000; // 5 seconds.
CArray<CString> DiskQuotaUser::m_ContainerNameCache;



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::DiskQuotaUser

    Description: Constructor.

    Arguments:
         pFSObject - Pointer to "file system" object.  It is through this pointer
            that the object accesses the ntioapi functions.  Caller must call
            AddRef() for this pointer prior to calling Initialize().


    Returns: Nothing.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    09/05/96    Added domain name string.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DiskQuotaUser::DiskQuotaUser(
    FSObject *pFSObject
    ) : m_cRef(0),
        m_ulUniqueId(InterlockedIncrement((LONG *)&m_ulNextUniqueId)),
        m_pSid(NULL),
        m_pszLogonName(NULL),
        m_pszDisplayName(NULL),
        m_pFSObject(pFSObject),
        m_bNeedCacheUpdate(TRUE),     // Data cache, not domain name cache.
        m_iContainerName(-1),
        m_dwAccountStatus(DISKQUOTA_USER_ACCOUNT_UNRESOLVED)
{
    DBGTRACE((DM_USER, DL_HIGH, TEXT("DiskQuotaUser::DiskQuotaUser")));
    DBGPRINT((DM_USER, DL_HIGH, TEXT("\tthis = 0x%08X"), this));
    DBGASSERT((NULL != m_pFSObject));

    m_llQuotaUsed      = 0;
    m_llQuotaThreshold = 0;
    m_llQuotaLimit     = 0;

    //
    // Initialize the domain name cache and class-wide locking mutex.
    // These members are static so we only do it once.
    //
    InterlockedIncrement(&m_cUsersAlive);
    if (NULL == DiskQuotaUser::m_hMutex)
    {
        DiskQuotaUser::m_hMutex = CreateMutex(NULL, FALSE, NULL);
        m_ContainerNameCache.SetSize(25);
        m_ContainerNameCache.SetGrow(25);
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::~DiskQuotaUser

    Description: Destructor

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DiskQuotaUser::~DiskQuotaUser(
    VOID
    )
{
    DBGTRACE((DM_USER, DL_HIGH, TEXT("DiskQuotaUser::~DiskQuotaUser")));
    DBGPRINT((DM_USER, DL_HIGH, TEXT("\tthis = 0x%08X"), this));

    Destroy();
    if (InterlockedDecrement(&m_cUsersAlive) == 0)
    {
        //
        // If active user count is 0, destroy the domain name cache and
        // class-wide mutex.
        //
        DestroyContainerNameCache();

        if (NULL != DiskQuotaUser::m_hMutex)
        {
            CloseHandle(DiskQuotaUser::m_hMutex);
            DiskQuotaUser::m_hMutex = NULL;
        }
    }
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::QueryInterface

    Description: Returns an interface pointer to the object's IUnknown or
        IDiskQuotaUser interface.  Only IID_IUnknown and
        IID_IDiskQuotaUser are recognized.  The object referenced by the
        returned interface pointer is uninitialized.  The recipient of the
        pointer must call Initialize() before the object is usable.

    Arguments:
        riid - Reference to requested interface ID.

        ppvOut - Address of interface pointer variable to accept interface ptr.

    Returns:
        NOERROR         - Success.
        E_NOINTERFACE   - Requested interface not supported.
        E_INVALIDARG    - ppvOut argument was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaUser::QueryInterface(
    REFIID riid,
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_USER, DL_MID, TEXT("DiskQuotaUser::QueryInterface")));
    DBGPRINTIID(DM_USER, DL_MID, riid);

    HRESULT hResult = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    try
    {
        *ppvOut = NULL;

        if (IID_IUnknown == riid ||
            IID_IDiskQuotaUser == riid)
        {
            *ppvOut = static_cast<IDiskQuotaUser *>(this);
        }
        else if (IID_IDispatch == riid ||
                 IID_DIDiskQuotaUser == riid)
        {
            //
            // Create a disk quota user "dispatch" object to handle all of
            // the automation duties. This object takes a pointer to the real
            // user object so that it can call the real object to do the real
            // work.  The reason we use a special "dispatch" object is so that
            // we can maintain identical names for dispatch and vtable methods
            // that perform the same function.  Otherwise, if the DiskQuotaUser
            // object implements both IDiskQuotaUser and DIDiskQuotaUser methods,
            // we could not have two methods named Invalidate (one for vtable
            // and one for dispatch.
            //
            DiskQuotaUserDisp *pUserDisp = new DiskQuotaUserDisp(static_cast<PDISKQUOTA_USER>(this));
            *ppvOut = static_cast<DIDiskQuotaUser *>(pUserDisp);
        }
        if (NULL != *ppvOut)
        {
            ((LPUNKNOWN)*ppvOut)->AddRef();
            hResult = NOERROR;
        }
    }
    catch(CAllocException& e)
    {
        *ppvOut = NULL;
        hResult = E_OUTOFMEMORY;
    }
    catch(...)
    {
        *ppvOut = NULL;
        hResult = E_UNEXPECTED;
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::AddRef

    Description: Increments object reference count.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
DiskQuotaUser::AddRef(
    VOID
    )
{
    DBGTRACE((DM_USER, DL_LOW, TEXT("DiskQuotaUser::AddRef")));
    DBGPRINT((DM_USER, DL_LOW, TEXT("\t0x%08X  %d -> %d"),
              this, m_cRef, m_cRef + 1));

    ULONG ulReturn = m_cRef + 1;
    InterlockedIncrement(&m_cRef);
    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::Release

    Description: Decrements object reference count.  If count drops to 0,
        object is deleted.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
DiskQuotaUser::Release(
    VOID
    )
{
    DBGTRACE((DM_USER, DL_LOW, TEXT("DiskQuotaUser::Release")));
    DBGPRINT((DM_USER, DL_LOW, TEXT("\t0x%08X  %d -> %d"),
              this, m_cRef, m_cRef - 1));

    ULONG ulReturn = m_cRef - 1;
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::Initialize

    Description: Initializes a new DiskQuotaUser object from a quota information
        record read from a volume's quota information file.

    Arguments:
        pfqi [optional] - Pointer to a record of type FILE_QUOTA_INFORMATION.  If
            not NULL, the data from this record is used to initialize the new user
            object.

    Returns:
        NOERROR             - Success.
        E_UNEXPECTED        - SID buffer too small (shouldn't happen).
        ERROR_INVALID_SID (hr) - SID in quota information is invalid.
        ERROR_ACCESS_DENIED (hr) - Need READ access to quota device.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/11/96    Added access control.                                BrianAu
    09/05/96    Added exception handling.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DiskQuotaUser::Initialize(
    PFILE_QUOTA_INFORMATION pfqi
    )
{
    HRESULT hResult = NOERROR;

    DBGASSERT((NULL != m_pFSObject));

    //
    // Need READ access to create a user object.
    //
    if (m_pFSObject->GrantedAccess(GENERIC_READ))
    {
        if (NULL != pfqi)  // pfqi is optional.
        {
            if (0 < pfqi->SidLength && IsValidSid(&pfqi->Sid))
            {
                //
                // Allocate space for SID structure.
                //
                m_pSid = (PSID) new BYTE[pfqi->SidLength];

                //
                // Copy SID structure to object.
                //
                if (CopySid(pfqi->SidLength, m_pSid, &pfqi->Sid))
                {
                    //
                    // Initialize user's quota data values.
                    // If error copying SID, don't bother with these.
                    //
                    m_llQuotaUsed      = pfqi->QuotaUsed.QuadPart;
                    m_llQuotaThreshold = pfqi->QuotaThreshold.QuadPart;
                    m_llQuotaLimit     = pfqi->QuotaLimit.QuadPart;
                }
                else
                {
                    //
                    // The only reason CopySid can fail is
                    // STATUS_BUFFER_TOO_SMALL.  Since we allocated the buffer
                    // above, this should never fail.
                    //
                    DBGASSERT((FALSE));
                    hResult = E_UNEXPECTED; // Error copying SID.
                }
            }
            else
            {
                DBGERROR((TEXT("DiskQuotaUser::Initialize - Invalid SID or Bad Sid Length (%d)"), pfqi->SidLength));
                hResult = HRESULT_FROM_WIN32(ERROR_INVALID_SID);
            }
        }
    }
    else
        hResult = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::Destroy

    Description: Destroys a user object by deleting its SID buffer and releasing
        its FSObject pointer.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    09/05/96    Added domain name string.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID DiskQuotaUser::Destroy(
    VOID
    )
{
    //
    // Delete the SID buffer.
    //
    delete [] m_pSid;
    m_pSid = NULL;

    //
    // Delete the logon name buffer.
    //
    delete[] m_pszLogonName;
    m_pszLogonName = NULL;

    //
    // Delete the display name buffer.
    //
    delete[] m_pszDisplayName;
    m_pszDisplayName = NULL;

    if (NULL != m_pFSObject)
    {
        //
        // Release hold on File System object.
        //
        m_pFSObject->Release();
        m_pFSObject = NULL;
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::DestroyContainerNameCache

    Description: Destroys the container name cache.  Should only be called
        when there are not more active user objects.  The container name cache
        is a static member of DiskQuotaUser.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/06/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DiskQuotaUser::DestroyContainerNameCache(
    VOID
    )
{
    //
    // Remove all container name strings from the cache.  No need to lock
    // the cache object before clearing it.  It will handle the locking
    // and unlocking.
    //
    m_ContainerNameCache.Clear();
}


//
// Return user object's unique ID.
//
STDMETHODIMP
DiskQuotaUser::GetID(
    ULONG *pulID
    )
{
    *pulID = m_ulUniqueId;
    return NOERROR;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::GetAccountStatus

    Description: Retrieves the account name resolution status for the
        user object.

    Arguments:
        pdwAccountStatus - Address of variable to recieve status.  The following
            values (see dskquota.h) can be returned in this variable:

            DISKQUOTA_USER_ACCOUNT_RESOLVED
            DISKQUOTA_USER_ACCOUNT_UNAVAILABLE
            DISKQUOTA_USER_ACCOUNT_DELETED
            DISKQUOTA_USER_ACCOUNT_INVALID
            DISKQUOTA_USER_ACCOUNT_UNKNOWN
            DISKQUOTA_USER_ACCOUNT_UNRESOLVED

    Returns:
        NOERROR       - Success.
        E_INVALIDARG  - pdwAccountStatus arg is NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/11/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaUser::GetAccountStatus(
    LPDWORD pdwAccountStatus
    )
{
    if (NULL == pdwAccountStatus)
        return E_INVALIDARG;

    *pdwAccountStatus = m_dwAccountStatus;

    return NOERROR;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::SetName

    Description: Sets the account names of the user object.
        It is intended that the SidNameResolver object will call this member
        when it has resolved a user's SID into an account name.  This function
        is not included in IDiskQuotaUser.  Therefore, it is not for public
        consumption.

    Arguments:
        pszContainer - Address of buffer containing container name string.

        pszLogonName - Address of buffer containing user's logon name string.

        pszDisplayName - Address of buffer containing user's display name string.

    Returns:
        NOERROR        - Success.
        E_INVALIDARG   - pszName or pszDomain arg is NULL.
        E_OUTOFMEMORY  - Insufficient memory.
        ERROR_LOCK_FAILED (hr) - Couldn't lock user object.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/11/96    Initial creation.                                    BrianAu
    09/05/96    Added domain name string.                            BrianAu
    09/22/96    Added full name string.                              BrianAu
    12/10/96    Added class-scope user lock.                         BrianAu
    05/18/97    Removed access token.                                BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaUser::SetName(
    LPCWSTR pszContainer,
    LPCWSTR pszLogonName,
    LPCWSTR pszDisplayName
    )
{
    HRESULT hResult = NOERROR;

    if (NULL == pszContainer || NULL == pszLogonName || NULL == pszDisplayName)
        return E_INVALIDARG;

    if (!DiskQuotaUser::Lock())
    {
        hResult = HRESULT_FROM_WIN32(ERROR_LOCK_FAILED);
    }
    else
    {
        //
        // Delete existing name buffer.
        //
        delete[] m_pszLogonName;
        m_pszLogonName = NULL;
        delete[] m_pszDisplayName;
        m_pszDisplayName = NULL;

        try
        {
            //
            // Save name and full name in user object.
            // Cache container string in container name cache and
            // save cache index in user object.
            //
            INT index     = -1;
            m_pszLogonName   = StringDup(pszLogonName);
            m_pszDisplayName = StringDup(pszDisplayName);
            CacheContainerName(pszContainer, &m_iContainerName);
        }
        catch(CAllocException& e)
        {
            hResult = E_OUTOFMEMORY;
        }
        catch(...)
        {
            hResult = E_UNEXPECTED;   // No re-throw.
        }
        DiskQuotaUser::ReleaseLock();
    }

    return hResult;
}




///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::GetName

    Description: Retrieves the domain and account names from the user object.
        It is intended that the client of the user object will register
        a callback (event sink) with a DiskQuotaControl object.  When the
        resolver has resolved the SID to an account name, the resolver will
        set the user object's name string and the client will be notified.
        The client then calls this method to get the user's name.

    Arguments:
        pszContainerBuffer - Address of destination buffer for container name string.

        cchContainerBuffer - Size of container destination buffer in characters.

        pszLogonNameBuffer - Address of destination buffer for logon name string.

        cchLogonNameBuffer - Size of logon name destination buffer in characters.

        pszDisplayNameBuffer - Address of destination buffer for display name string.

        cchDisplayNameBuffer - Size of display name destination buffer in characters.

    Returns:
        NOERROR                - Success.
        ERROR_LOCK_FAILED (hr) - Failed to lock user object.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/11/96    Initial creation.                                    BrianAu
    09/05/96    Added domain name string.                            BrianAu
    09/22/96    Added full name string.                              BrianAu
    12/10/96    Added class-scope user lock.                         BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaUser::GetName(
    LPWSTR pszContainerBuffer,
    DWORD cchContainerBuffer,
    LPWSTR pszLogonNameBuffer,
    DWORD cchLogonNameBuffer,
    LPWSTR pszDisplayNameBuffer,
    DWORD cchDisplayNameBuffer
    )
{
    HRESULT hResult = NOERROR;
    if (!DiskQuotaUser::Lock())
    {
        hResult = HRESULT_FROM_WIN32(ERROR_LOCK_FAILED);
    }
    else
    {
        if (NULL != pszContainerBuffer)
        {
            if (-1 != m_iContainerName)
            {
                GetCachedContainerName(m_iContainerName,
                                       pszContainerBuffer,
                                       cchContainerBuffer);
            }
            else
                lstrcpyn(pszContainerBuffer, TEXT(""), cchContainerBuffer);
        }

        if (NULL != pszLogonNameBuffer)
        {
            lstrcpyn(pszLogonNameBuffer,
                     (NULL != m_pszLogonName) ? m_pszLogonName : TEXT(""),
                     cchLogonNameBuffer);
        }

        if (NULL != pszDisplayNameBuffer)
        {
            lstrcpyn(pszDisplayNameBuffer,
                     (NULL != m_pszDisplayName) ? m_pszDisplayName : TEXT(""),
                     cchDisplayNameBuffer);
        }
        DiskQuotaUser::ReleaseLock();
    }

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::GetSidLength

    Description: Retrieves the length of the user's SID in bytes.

    Arguments:
        pcbSid - Address of DWORD to accept SID length value.

    Returns:
        NOERROR                - Success.
        E_INVALIDARG           - pcbSid argument is NULL.
        ERROR_INVALID_SID (hr) - Invalid SID.
        ERROR_LOCK_FAILED (hr) - Couldn't lock user object.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    12/10/96    Added class-scope user lock.                         BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaUser::GetSidLength(
    LPDWORD pcbSid
    )
{
    HRESULT hResult = NOERROR;

    if (NULL == pcbSid)
        return E_INVALIDARG;

    if (!DiskQuotaUser::Lock())
    {
        hResult = HRESULT_FROM_WIN32(ERROR_LOCK_FAILED);
    }
    else
    {
        if (NULL != m_pSid && IsValidSid(m_pSid))
        {
            *pcbSid = GetLengthSid(m_pSid);
        }
        else
            hResult = HRESULT_FROM_WIN32(ERROR_INVALID_SID);

        DiskQuotaUser::ReleaseLock();
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::GetSid

    Description: Retrieves the user's SID to a caller-provided buffer.
        The caller should call GetSidLength() to obtain the required buffer
        size before calling GetSid().

    Arguments:
        pSid - Address of destination buffer for SID.  This argument type must
            be PBYTE to work with the MIDL compiler.  Since PSID is really just
            LPVOID and since MIDL doesn't like pointers to void, we have to
            use something other than PSID.

        cbSidBuf - Size of destination buffer in bytes.

    Returns:
        NOERROR                        - Success.
        E_INVALIDARG                   - pSID is NULL.
        ERROR_INVALID_SID (hr)         - User's SID is invalid.
        ERROR_INSUFFICIENT_BUFFER (hr) - Insufficient dest buffer size.
        ERROR_LOCK_FAILED (hr)         - Couldn't lock user object.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    12/10/96    Added class-scope user lock.                         BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaUser::GetSid(
    PBYTE pSid,
    DWORD cbSidBuf
    )
{
    HRESULT hResult = NOERROR;

    if (NULL == pSid)
        return E_INVALIDARG;

    if (!DiskQuotaUser::Lock())
    {
        hResult = HRESULT_FROM_WIN32(ERROR_LOCK_FAILED);
    }
    else
    {
        if (NULL != m_pSid && IsValidSid(m_pSid))
        {
            if (!CopySid(cbSidBuf, (PSID)pSid, m_pSid))
            {
                //
                // The only reason CopySid can fail is STATUS_BUFFER_TOO_SMALL.
                // Force status code to INSUFFICIENT_BUFFER.
                //
                DBGERROR((TEXT("ERROR in DiskQuotaUser::GetSid. CopySid() failed.  Result = 0x%08X."),
                          GetLastError()));
                hResult = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
        }
        else
        {
            DBGERROR((TEXT("ERROR in DiskQuotaUser::GetSid. Invalid SID.")));
            hResult = HRESULT_FROM_WIN32(ERROR_INVALID_SID);
        }
        DiskQuotaUser::ReleaseLock();
    }

    return hResult;

}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::RefreshCachedInfo

    Description: Refreshes a user object's cached quota information from the
        volume's quota information file.

    Arguments: None.

    Returns:
        NOERROR                  - Success.
        ERROR_ACCESS_DENIED (hr) - No READ access to quota device.
        E_FAIL                   - Unexpected NTIOAPI error.

        This function can propagate errors from the NTIOAPI system.  A few
        known ones are mapped to HResults in fsobject.cpp (see HResultFromNtStatus).
        All others are mapped to E_FAIL.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/05/96    Initial creation.                                    BrianAu
    09/05/96    Added exception handling.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DiskQuotaUser::RefreshCachedInfo(
    VOID
    )
{
    HRESULT hResult   = NOERROR;
    DWORD cbBuffer    = FILE_QUOTA_INFORMATION_MAX_LEN;
    PSIDLIST pSidList = NULL;
    DWORD cbSidList   = 0;
    PSID pSids[]      = { m_pSid, NULL };
    PBYTE pbBuffer    = NULL;

    try
    {
        pbBuffer = new BYTE[cbBuffer];

        //
        // This can throw OutOfMemory.
        //
        hResult = CreateSidList(pSids, 0, &pSidList, &cbSidList);
        if (SUCCEEDED(hResult))
        {
            hResult = m_pFSObject->QueryUserQuotaInformation(
                            pbBuffer,               // Buffer to receive data.
                            cbBuffer,               // Buffer size in bytes.
                            TRUE,                   // Single entry requested.
                            pSidList,               // Sid.
                            cbSidList,              // Length of Sid.
                            NULL,                   // Starting Sid
                            TRUE);                  // Start search at first user.

            if (SUCCEEDED(hResult) || ERROR_NO_MORE_ITEMS == HRESULT_CODE(hResult))
            {
                PFILE_QUOTA_INFORMATION pfqi = (PFILE_QUOTA_INFORMATION)pbBuffer;

                m_llQuotaUsed      = pfqi->QuotaUsed.QuadPart;
                m_llQuotaThreshold = pfqi->QuotaThreshold.QuadPart;
                m_llQuotaLimit     = pfqi->QuotaLimit.QuadPart;
                m_bNeedCacheUpdate = FALSE;

                //
                // Don't return ERROR_NO_MORE_ITEMS to caller.
                // They won't care.
                //
                hResult = NOERROR;
            }
            delete[] pSidList;
        }
        delete[] pbBuffer;
    }
    catch(...)
    {
        //
        // Cleanup and throw to caller.
        //
        delete[] pbBuffer;
        delete[] pSidList;

        throw;
    }

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::WriteCachedInfo

    Description: Writes quota information cached in a user object to the
        volume's quota information file.

    Arguments: None.

    Returns:
        NOERROR                  - Success.
        ERROR_ACCESS_DENIED (hr) - No WRITE access to quota device.
        E_FAIL                   - Some other NTIOAPI error.
        E_UNEXPECTED             - CopySid failed.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/31/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DiskQuotaUser::WriteCachedInfo(
    VOID
    )
{
    HRESULT hResult = NOERROR;
    BYTE Buffer[FILE_QUOTA_INFORMATION_MAX_LEN];

    PFILE_QUOTA_INFORMATION pfqi = (PFILE_QUOTA_INFORMATION)Buffer;

    pfqi->NextEntryOffset         = 0;
    pfqi->SidLength               = GetLengthSid(m_pSid);
    pfqi->QuotaUsed.QuadPart      = m_llQuotaUsed;
    pfqi->QuotaLimit.QuadPart     = m_llQuotaLimit;
    pfqi->QuotaThreshold.QuadPart = m_llQuotaThreshold;

    if (CopySid(pfqi->SidLength, &(pfqi->Sid), m_pSid))
        hResult = m_pFSObject->SetUserQuotaInformation(pfqi, sizeof(Buffer));
    else
        hResult = E_UNEXPECTED;

    if (FAILED(hResult))
    {
        //
        // Something failed.
        // Invalidate cached information so next request reads from disk.
        //
        Invalidate();
    }


    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::GetQuotaInformation

    Description: Retrieves a user's quota limit, threshold and used quota
        values in a single method.  Since the user interface is marshaled
        across thread boundaries, this can be a big performance improvement
        if you want all three values.

    Arguments:
        pbInfo - Address of destination buffer.  Should be sized for structure
            DISKQUOTA_USER_INFORMATION.

        cbInfo - Number of bytes in destination buffer.  Should be
            sizeof(DISKQUOTA_USER_INFORMATION).

    Returns:
        NOERROR                        - Success.
        E_INVALIDARG                   - pbInfo argument is NULL.
        E_OUTOFMEMORY                  - Insufficient memory.
        ERROR_INSUFFICIENT_BUFFER (hr) - Destination buffer is too small.
        ERROR_LOCK_FAILED (hr)         - Couldn't lock user object.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/31/96    Initial creation.                                    BrianAu
    12/10/96    Added class-scope user lock.                         BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaUser::GetQuotaInformation(
    LPVOID pbInfo,
    DWORD cbInfo
    )
{
    HRESULT hResult = NOERROR;

    if (NULL == pbInfo)
        return E_INVALIDARG;

    try
    {
        if (cbInfo < sizeof(DISKQUOTA_USER_INFORMATION))
        {
            hResult = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
            if (!DiskQuotaUser::Lock())
            {
                hResult = HRESULT_FROM_WIN32(ERROR_LOCK_FAILED);
            }
            else
            {
                //
                // Refresh cached info from disk if needed.
                // Can throw OutOfMemory.
                //
                if (m_bNeedCacheUpdate)
                    hResult = RefreshCachedInfo();

                if (SUCCEEDED(hResult))
                {
                    PDISKQUOTA_USER_INFORMATION pui = (PDISKQUOTA_USER_INFORMATION)pbInfo;

                    pui->QuotaUsed      = m_llQuotaUsed;
                    pui->QuotaThreshold = m_llQuotaThreshold;
                    pui->QuotaLimit     = m_llQuotaLimit;
                }
                DiskQuotaUser::ReleaseLock();
            }
        }
    }
    catch(CAllocException& e)
    {
        hResult = E_OUTOFMEMORY;
    }
    catch(...)
    {
        hResult = E_UNEXPECTED;
    }

    return hResult;
}


STDMETHODIMP
DiskQuotaUser::GetQuotaUsedText(
    LPWSTR pszText,
    DWORD cchText
    )
{
    if (NULL == pszText)
        return E_INVALIDARG;

    LONGLONG llValue;
    HRESULT hr = GetQuotaUsed(&llValue);
    if (SUCCEEDED(hr))
    {
        if (NOLIMIT == llValue)
        {
            LoadString(g_hInstDll, IDS_NO_LIMIT, pszText, cchText);
        }
        else
        {
            XBytes::FormatByteCountForDisplay(llValue, pszText, cchText);
        }
    }
    return hr;
}

STDMETHODIMP
DiskQuotaUser::GetQuotaThresholdText(
    LPWSTR pszText,
    DWORD cchText
    )
{
    if (NULL == pszText)
        return E_INVALIDARG;

    LONGLONG llValue;
    HRESULT hr = GetQuotaThreshold(&llValue);
    if (SUCCEEDED(hr))
    {
        if (NOLIMIT == llValue)
        {
            LoadString(g_hInstDll, IDS_NO_LIMIT, pszText, cchText);
        }
        else
        {
            XBytes::FormatByteCountForDisplay(llValue, pszText, cchText);
        }
    }
    return hr;
}


STDMETHODIMP
DiskQuotaUser::GetQuotaLimitText(
    LPWSTR pszText,
    DWORD cchText
    )
{
    if (NULL == pszText)
        return E_INVALIDARG;

    LONGLONG llValue;
    HRESULT hr = GetQuotaLimit(&llValue);
    if (SUCCEEDED(hr))
    {
        if (NOLIMIT == llValue)
        {
            LoadString(g_hInstDll, IDS_NO_LIMIT, pszText, cchText);
        }
        else
        {
            XBytes::FormatByteCountForDisplay(llValue, pszText, cchText);
        }
    }
    return hr;
}


STDMETHODIMP
DiskQuotaUser::SetQuotaThreshold(
    LONGLONG llThreshold,
    BOOL bWriteThrough
    )
{
    if (MARK4DEL > llThreshold)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    return SetLargeIntegerQuotaItem(&m_llQuotaThreshold,
                                    llThreshold,
                                    bWriteThrough);
}


STDMETHODIMP
DiskQuotaUser::SetQuotaLimit(
    LONGLONG llLimit,
    BOOL bWriteThrough
    )
{
    if (MARK4DEL > llLimit)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    return SetLargeIntegerQuotaItem(&m_llQuotaLimit,
                                    llLimit,
                                    bWriteThrough);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::GetLargeIntegerQuotaItem

    Description: Retrieves a single quota information item (used, limit,
        threshold) for the user.  If the cached data is invalid, fresh data is
        read in from disk.

    Arguments:
        pllItem - Address of cached member item.

        pllValueOut - Address of LONGLONG to receive item's value.

    Returns:
        NOERROR                - Success.
        E_INVALIDARG           - Either pdwLowPart or pdwHighPart arg was NULL.
        E_OUTOFMEMORY          - Insufficient memory.
        E_UNEXPECTED           - Unexpected exception.
        ERROR_LOCK_FAILED (hr) - Couldn't lock user object.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/05/96    Initial creation.                                    BrianAu
    12/10/96    Added class-scope user lock.                         BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DiskQuotaUser::GetLargeIntegerQuotaItem(
    PLONGLONG pllItem,
    PLONGLONG pllValueOut
    )
{
    HRESULT hResult = NOERROR;

    DBGASSERT((NULL != pllItem));

    if (NULL == pllItem || NULL == pllValueOut)
        return E_INVALIDARG;

    if (!DiskQuotaUser::Lock())
    {
        hResult = HRESULT_FROM_WIN32(ERROR_LOCK_FAILED);
    }
    else
    {
        if (m_bNeedCacheUpdate)
        try
        {
            hResult = RefreshCachedInfo();
        }
        catch(CAllocException& e)
        {
            hResult = E_OUTOFMEMORY;
        }
        catch(...)
        {
            hResult = E_UNEXPECTED;
        }

        if (SUCCEEDED(hResult))
        {
            *pllValueOut = *pllItem;
        }
        DiskQuotaUser::ReleaseLock();
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::SetLargeIntegerQuotaItem

    Description: Sets the quota information for a given quota item (limit or
        threshold).  If the bWriteThrough argument is TRUE, the information is
        also written through to the volume's quota file.  Otherwise, it is
        just cached in the user object.

    Arguments:
        pllItem - Address of cached member item.

        llValue - LONGLONG value to assign to member item.

        bWriteThrough - TRUE  = Write data through to disk.
                        FALSE = Only cache data in user object.

    Returns:
        NOERROR                  - Success.
        ERROR_ACCESS_DENIED (hr) - No WRITE access to quota device.
        ERROR_LOCK_FAILED (hr)   - Couldn't lock user object.
        E_FAIL                   - Some other NTIOAPI error.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/06/96    Initial creation.                                    BrianAu
    12/10/96    Added class-scope user lock.                         BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DiskQuotaUser::SetLargeIntegerQuotaItem(
    PLONGLONG pllItem,
    LONGLONG llValue,
    BOOL bWriteThrough)
{
    DBGASSERT((NULL != pllItem));
    HRESULT hResult = NOERROR;

    if (!DiskQuotaUser::Lock())
    {
        hResult = HRESULT_FROM_WIN32(ERROR_LOCK_FAILED);
    }
    else
    {
        *pllItem = llValue;
        if (bWriteThrough)
            hResult = WriteCachedInfo();

        DiskQuotaUser::ReleaseLock();
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::CacheContainerName

    Description: Class DiskQuotaUser maintains a static member that is
        a cache of account container names.   It is likely that there will be
        few distinct container names in use on a volume.  Therefore, there's
        no need to store a container name for each user object.  We cache the
        names and store only an index into the cache in each user object.

        This method adds a name to the cache and returns the index of the
        name in the cache.  If the name already exists in the cache,
        it is not added.

    Arguments:
        pszContainer - Address of container name string to add to cache.

        pCacheIndex [optional] - Address of integer variable to receive the
            cache index of the container name string.  May be NULL.

    Returns:
        S_OK        - Success.
        S_FALSE     - Name already in cache.
        E_FAIL      - No cache object.

    Exceptions: OutOfMemory.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/05/09    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DiskQuotaUser::CacheContainerName(
    LPCTSTR pszContainer,
    INT *pCacheIndex
    )
{
    DBGASSERT((NULL != pszContainer));

    HRESULT hResult  = S_OK;
    INT iCacheIndex  = -1;
    UINT cItems      = 0;

    m_ContainerNameCache.Lock();

    cItems = m_ContainerNameCache.Count();

    for (UINT i = 0; i < cItems; i++)
    {
        //
        // See if the name is already in the cache.
        //
        if (0 == m_ContainerNameCache[i].Compare(pszContainer))
        {
            iCacheIndex = i;
            hResult     = S_FALSE; // Already cached.
            break;
        }
    }

    if (S_OK == hResult)
    {
        //
        // Not in the cache. Add it.
        //
        try
        {
            m_ContainerNameCache.Append(CString(pszContainer));
            iCacheIndex = m_ContainerNameCache.UpperBound();
        }
        catch(...)
        {
            m_ContainerNameCache.ReleaseLock();
            throw; // throw to caller.
        }
    }
    m_ContainerNameCache.ReleaseLock();

    if (NULL != pCacheIndex)
        *pCacheIndex = iCacheIndex;

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::GetCachedContainerName

    Description: Retrieves an account container name string from the
        container name cache.

    Arguments:
        iCacheIndex - User's index in domain name cache.

        pszContainer - Destination buffer to receive container name string.

        cchContainer - Number of characters in destination buffer.

    Returns:
        NOERROR      - Success.
        E_UNEXPECTED - No name at index iCacheIndex.  Returns "" as name.
        E_FAIL       - No domain name cache object.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/05/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DiskQuotaUser::GetCachedContainerName(
    INT iCacheIndex,
    LPTSTR pszContainer,
    UINT cchContainer
    )
{
    DBGASSERT((NULL != pszContainer));
    DBGASSERT((-1 != iCacheIndex));

    HRESULT hResult  = NOERROR;

    m_ContainerNameCache.Lock();

    DBGASSERT((iCacheIndex < m_ContainerNameCache.Count()));

    lstrcpyn(pszContainer, m_ContainerNameCache[iCacheIndex], cchContainer);

    m_ContainerNameCache.ReleaseLock();

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::Lock

    Description: Call this to obtain an exclusive lock to the user object.
        In actuality, there is only one lock for all user objects so you're
        really getting an exclusive lock to all users.  Since there can be
        a high number of users, it was decided to use a single class-wide
        lock instead of a unique lock for each user object.

    Arguments: None.

    Returns:
        TRUE    = Obtained exclusive lock.
        FALSE   = Couldn't get a lock.  Either mutex hasn't been created or
                  the mutex wait timeout expired, or the wait operation failed.
                  Either way, we couldn't get the lock.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    12/10/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
DiskQuotaUser::Lock(
    VOID
    )
{
    BOOL bResult = FALSE;

    if (NULL != DiskQuotaUser::m_hMutex)
    {
        DWORD dwWaitResult = WaitForSingleObject(DiskQuotaUser::m_hMutex,
                                                 DiskQuotaUser::m_dwMutexWaitTimeout);
        bResult = (WAIT_OBJECT_0 == dwWaitResult);
    }
    return bResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::ReleaseLock

    Description: Call this to release a lock obtained with DiskQuotaUser::Lock.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    12/10/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DiskQuotaUser::ReleaseLock(
    VOID
    )
{
    if (NULL != DiskQuotaUser::m_hMutex)
    {
        ReleaseMutex(DiskQuotaUser::m_hMutex);
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::SetAccountStatus

    Description: Stores the status of the user's account in the user object.
                 User accounts may be "unresolved", "unavailable", "resolved",
                 "deleted", "invalid" or "unknown".
                 These states correspond to the values obtained
                 through LookupAccountSid.

    Arguments:
        dwStatus - DISKQUOTA_USER_ACCOUNT_UNRESOLVED
                   DISKQUOTA_USER_ACCOUNT_UNAVAILABLE
                   DISKQUOTA_USER_ACCOUNT_RESOLVED
                   DISKQUOTA_USER_ACCOUNT_DELETED
                   DISKQUOTA_USER_ACCOUNT_UNKNOWN
                   DISKQUOTA_USER_ACCOUNT_INVALID

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DiskQuotaUser::SetAccountStatus(
    DWORD dwStatus
    )
{
    DBGASSERT((DISKQUOTA_USER_ACCOUNT_UNRESOLVED  == dwStatus ||
           DISKQUOTA_USER_ACCOUNT_UNAVAILABLE == dwStatus ||
           DISKQUOTA_USER_ACCOUNT_RESOLVED    == dwStatus ||
           DISKQUOTA_USER_ACCOUNT_DELETED     == dwStatus ||
           DISKQUOTA_USER_ACCOUNT_INVALID     == dwStatus ||
           DISKQUOTA_USER_ACCOUNT_UNKNOWN     == dwStatus));

    m_dwAccountStatus = dwStatus;
}



//
// The following functions implement the DiskQuotaUser "dispatch" object that
// is created to handle OLE automation duties for the DiskQuotaUser object.
// The functions are all fairly basic and require little explanation.
// Therefore, I'll spare you the function headers.  In most cases,
// the property and method functions call directly through to their
// corresponding functions in class DiskQuotaUser.
//
DiskQuotaUserDisp::DiskQuotaUserDisp(
    PDISKQUOTA_USER pUser
    ) : m_cRef(0),
        m_pUser(pUser)
{
    DBGTRACE((DM_USER, DL_HIGH, TEXT("DiskQuotaUserDisp::DiskQuotaUserDisp")));
    DBGPRINT((DM_USER, DL_HIGH, TEXT("\tthis = 0x%08X"), this));

    if (NULL != m_pUser)
    {
        m_pUser->AddRef();
    }
    m_Dispatch.Initialize(static_cast<IDispatch *>(this),
                          LIBID_DiskQuotaTypeLibrary,
                          IID_DIDiskQuotaUser,
                          L"DSKQUOTA.DLL");
}

DiskQuotaUserDisp::~DiskQuotaUserDisp(
    VOID
    )
{
    DBGTRACE((DM_USER, DL_HIGH, TEXT("DiskQuotaUserDisp::~DiskQuotaUserDisp")));
    DBGPRINT((DM_USER, DL_HIGH, TEXT("\tthis = 0x%08X"), this));

    if (NULL != m_pUser)
    {
        m_pUser->Release();
    }
}


STDMETHODIMP
DiskQuotaUserDisp::QueryInterface(
    REFIID riid,
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_USER, DL_MID, TEXT("DiskQuotaUserDisp::QueryInterface")));
    DBGPRINTIID(DM_USER, DL_MID, riid);

    HRESULT hResult = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IID_IUnknown == riid)
    {
        *ppvOut = this;
    }
    else if (IID_IDispatch == riid)
    {
        *ppvOut = static_cast<IDispatch *>(this);
    }
    else if (IID_DIDiskQuotaUser == riid)
    {
        *ppvOut = static_cast<DIDiskQuotaUser *>(this);
    }
    else if (IID_IDiskQuotaUser == riid)
    {
        //
        // Return the quota user's vtable interface.
        // This allows code to "typecast" (COM-style) between
        // the dispatch interface and vtable interface.
        //
        return m_pUser->QueryInterface(riid, ppvOut);
    }

    if (NULL != *ppvOut)
    {
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hResult = NOERROR;
    }

    return hResult;
}

STDMETHODIMP_(ULONG)
DiskQuotaUserDisp::AddRef(
    VOID
    )
{
    ULONG ulReturn = m_cRef + 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("DiskQuotaUserDisp::AddRef, 0x%08X  %d -> %d"),
             this, ulReturn - 1, ulReturn));

    InterlockedIncrement(&m_cRef);

    return ulReturn;
}


STDMETHODIMP_(ULONG)
DiskQuotaUserDisp::Release(
    VOID
    )
{
    ULONG ulReturn = m_cRef - 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("DiskQuotaUserDisp::Release, 0x%08X  %d -> %d"),
             this, ulReturn + 1, ulReturn));

    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}



//
// IDispatch::GetIDsOfNames
//
STDMETHODIMP
DiskQuotaUserDisp::GetIDsOfNames(
    REFIID riid,
    OLECHAR **rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId
    )
{
    DBGTRACE((DM_USER, DL_LOW, TEXT("DiskQuotaUserDisp::GetIDsOfNames")));
    //
    // Let our dispatch object handle this.
    //
    return m_Dispatch.GetIDsOfNames(riid,
                                    rgszNames,
                                    cNames,
                                    lcid,
                                    rgDispId);
}


//
// IDispatch::GetTypeInfo
//
STDMETHODIMP
DiskQuotaUserDisp::GetTypeInfo(
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTypeInfo
    )
{
    DBGTRACE((DM_USER, DL_LOW, TEXT("DiskQuotaUserDisp::GetTypeInfo")));
    //
    // Let our dispatch object handle this.
    //
    return m_Dispatch.GetTypeInfo(iTInfo, lcid, ppTypeInfo);
}


//
// IDispatch::GetTypeInfoCount
//
STDMETHODIMP
DiskQuotaUserDisp::GetTypeInfoCount(
    UINT *pctinfo
    )
{
    DBGTRACE((DM_USER, DL_LOW, TEXT("DiskQuotaUserDisp::GetTypeInfoCount")));
    //
    // Let our dispatch object handle this.
    //
    return m_Dispatch.GetTypeInfoCount(pctinfo);
}


//
// IDispatch::Invoke
//
STDMETHODIMP
DiskQuotaUserDisp::Invoke(
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr
    )
{
    DBGTRACE((DM_USER, DL_LOW, TEXT("DiskQuotaUserDisp::Invoke")));
    DBGPRINT((DM_USER, DL_LOW, TEXT("DispId = %d"), dispIdMember));
    DBGPRINTIID(DM_USER, DL_LOW, riid);
    //
    // Let our dispatch object handle this.
    //
    return m_Dispatch.Invoke(dispIdMember,
                             riid,
                             lcid,
                             wFlags,
                             pDispParams,
                             pVarResult,
                             pExcepInfo,
                             puArgErr);
}


//
// Return user object's unique ID.
//
STDMETHODIMP
DiskQuotaUserDisp::get_ID(
    long *pID
    )
{
    return m_pUser->GetID((ULONG *)pID);
}


STDMETHODIMP
DiskQuotaUserDisp::get_AccountContainerName(
    BSTR *pContainerName
    )
{
    TCHAR szName[MAX_DOMAIN] = { TEXT('\0') };
    HRESULT hr = m_pUser->GetName(szName, ARRAYSIZE(szName),
                                  NULL,   0,
                                  NULL,   0);
    if (SUCCEEDED(hr))
    {
        *pContainerName = SysAllocString(szName);
        if (NULL == *pContainerName)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


STDMETHODIMP
DiskQuotaUserDisp::get_LogonName(
    BSTR *pLogonName
    )
{
    TCHAR szName[MAX_USERNAME] = { TEXT('\0') };
    HRESULT hr = m_pUser->GetName(NULL,   0,
                                  szName, ARRAYSIZE(szName),
                                  NULL,   0);
    if (SUCCEEDED(hr))
    {
        *pLogonName = SysAllocString(szName);
        if (NULL == *pLogonName)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


STDMETHODIMP
DiskQuotaUserDisp::get_DisplayName(
    BSTR *pDisplayName
    )
{
    TCHAR szName[MAX_FULL_USERNAME] = { TEXT('\0') };
    HRESULT hr = m_pUser->GetName(NULL,   0,
                                  NULL,   0,
                                  szName, ARRAYSIZE(szName));

    if (SUCCEEDED(hr))
    {
        *pDisplayName = SysAllocString(szName);
        if (NULL == *pDisplayName)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


STDMETHODIMP
DiskQuotaUserDisp::get_QuotaThreshold(
    double *pThreshold
    )
{
    LONGLONG llValue;
    HRESULT hr = m_pUser->GetQuotaThreshold(&llValue);
    if (SUCCEEDED(hr))
        *pThreshold = (double)llValue;

    return hr;
}


STDMETHODIMP
DiskQuotaUserDisp::put_QuotaThreshold(
    double Threshold
    )
{
    if (MAXLONGLONG < Threshold)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    return m_pUser->SetQuotaThreshold((LONGLONG)Threshold, TRUE);
}


STDMETHODIMP
DiskQuotaUserDisp::get_QuotaThresholdText(
    BSTR *pThresholdText
    )
{
    TCHAR szValue[40];
    HRESULT hr = m_pUser->GetQuotaThresholdText(szValue, ARRAYSIZE(szValue));
    if (SUCCEEDED(hr))
    {
        *pThresholdText = SysAllocString(szValue);
        if (NULL == *pThresholdText)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}


STDMETHODIMP
DiskQuotaUserDisp::get_QuotaLimit(
    double *pQuotaLimit
    )
{
    LONGLONG llValue;
    HRESULT hr = m_pUser->GetQuotaLimit(&llValue);

    if (SUCCEEDED(hr))
        *pQuotaLimit = (double)llValue;

    return hr;
}


STDMETHODIMP
DiskQuotaUserDisp::put_QuotaLimit(
    double Limit
    )
{
    if (MAXLONGLONG < Limit)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    return m_pUser->SetQuotaLimit((LONGLONG)Limit, TRUE);
}


STDMETHODIMP
DiskQuotaUserDisp::get_QuotaLimitText(
    BSTR *pLimitText
    )
{
    TCHAR szValue[40];
    HRESULT hr = m_pUser->GetQuotaLimitText(szValue, ARRAYSIZE(szValue));
    if (SUCCEEDED(hr))
    {
        *pLimitText = SysAllocString(szValue);
        if (NULL == *pLimitText)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}


STDMETHODIMP
DiskQuotaUserDisp::get_QuotaUsed(
    double *pUsed
    )
{
    LONGLONG llValue;
    HRESULT hr = m_pUser->GetQuotaUsed(&llValue);
    if (SUCCEEDED(hr))
        *pUsed = (double)llValue;

    return hr;
}


STDMETHODIMP
DiskQuotaUserDisp::get_QuotaUsedText(
    BSTR *pUsedText
    )
{
    TCHAR szValue[40];
    HRESULT hr = m_pUser->GetQuotaUsedText(szValue, ARRAYSIZE(szValue));
    if (SUCCEEDED(hr))
    {
        *pUsedText = SysAllocString(szValue);
        if (NULL == *pUsedText)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

STDMETHODIMP
DiskQuotaUserDisp::get_AccountStatus(
    AccountStatusConstants *pStatus
    )
{
    DWORD dwStatus;
    HRESULT hr = m_pUser->GetAccountStatus(&dwStatus);
    if (SUCCEEDED(hr))
    {
        *pStatus = (AccountStatusConstants)dwStatus;
    }
    return hr;
}


//
// Methods.
//
STDMETHODIMP
DiskQuotaUserDisp::Invalidate(
    void
    )
{
    return m_pUser->Invalidate();
}




#ifdef NEVER
// ----------------------------------------------------------------------------
// OBSOLETE CODE
// ----------------------------------------------------------------------------
//
// I originally provided the GetSidString code in the IDiskQuotaUser interface.
// It was useful during development but I decided that it would be
// relatively useless to most users of the interface.  If they want to
// format the SID as a string, they can learn about it on MSDN.  That's
// where I got the original code (See in dskquota\common\utils.cpp).
// I've left it here in case a need is ever identified.
//
// [brianau - 08/05/97]
//
///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUser::GetSidString

    Description: Retrieves the account SID from the user object in character
        format.

    Arguments:
        pszBuffer - Address of destination buffer for account SID string.

        cchBuffer - Size of destination buffer in characters.

    Returns:
        NOERROR                        - Success.
        E_INVALIDARG                   - pszBuffer arg is NULL.
        ERROR_INSUFFICIENT_BUFFER (hr) - Caller's buffer is too small.
        ERROR_LOCK_FAILED (hr)         - Failed to lock user object.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/23/96    Initial creation.                                    BrianAu
    12/10/96    Added class-scope user lock.                         BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaUser::GetSidString(
    LPWSTR pszBuffer,
    DWORD cchBuffer
    )
{
    HRESULT hResult = NOERROR;

    if (NULL == pszBuffer)
        return E_INVALIDARG;

    if (!DiskQuotaUser::Lock())
    {
        hResult = HRESULT_FROM_WIN32(ERROR_LOCK_FAILED);
    }
    else
    {
        DWORD cchSid = cchBuffer;
        if (!SidToString(m_pSid, pszBuffer, &cchSid))
            hResult = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

        DiskQuotaUser::ReleaseLock();
    }

    return hResult;
}


#endif // NEVER
