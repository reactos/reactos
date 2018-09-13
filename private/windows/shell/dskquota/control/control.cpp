///////////////////////////////////////////////////////////////////////////////
/*  File: control.cpp

    Description: Contains member function definitions for class DiskQuotaControl.
        This class is the main point of focus for managing disk quota information
        through the DSKQUOTA library.  The user creates an instance of a 
        DiskQuotaControl object through CoCreateInstance and manages quota
        information through it's IDiskQuotaControl interface.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h" // PCH
#pragma hdrstop

#include "connect.h"
#include "control.h"
#include "guidsp.h"    // Private GUIDs.
#include "registry.h"
#include "sidcache.h"
#include "userbat.h"
#include "userenum.h"
#include "resource.h"  // For IDS_NO_LIMIT.
#include <oleauto.h>   // OLE automation
#include <comutil.h>
#include <sddl.h>

//
// Verify that build is UNICODE.
//
#if !defined(UNICODE)
#   error This module must be compiled UNICODE.
#endif


//
// Size of user enumerator's buffer.  Thought about this being a reg entry.
// Didn't make a lot of sense.
//
const UINT ENUMUSER_BUF_LEN = 2048;

//
// To add support for a new connection point type, just add a new IID to this
// array.  Also add a corresponding enumeration constant in the DiskQuotaControl
// class declaration that identifies the location of the conn pt IID in 
// m_rgpIConnPtsSupported[].
//
const IID * const DiskQuotaControl::m_rgpIConnPtsSupported[] = { &IID_IDiskQuotaEvents,
                                                                 &IID_DIDiskQuotaControlEvents };


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::DiskQuotaControl

    Description: Constructor.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/15/97    Added m_bInitialized member.                         BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DiskQuotaControl::DiskQuotaControl(
    VOID
    ) : m_cRef(0),
        m_bInitialized(FALSE),
        m_pFSObject(NULL),
        m_dwFlags(0),
        m_pSidNameResolver(NULL),
        m_rgConnPts(NULL),
        m_cConnPts(0),
        m_llDefaultQuotaThreshold(0),
        m_llDefaultQuotaLimit(0)
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("DiskQuotaControl::DiskQuotaControl")));
    InterlockedIncrement(&g_cRefThisDll);
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::~DiskQuotaControl

    Description: Destructor. Releases FSObject pointer.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DiskQuotaControl::~DiskQuotaControl(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("DiskQuotaControl::~DiskQuotaControl")));

    //
    // See the comment in NotifyUserNameChanged for a discussion on the
    // use of this mutex.  In short, it prevents a deadlock between
    // the resolver's thread and a client receiving a name-change
    // notification.  The wait here is INFINITE while the corresponding
    // wait in NotifyUserNameChanged is limited.
    //
    AutoLockMutex lock(m_mutex, INFINITE);

    if (NULL != m_pFSObject)
    {
        m_pFSObject->Release();
        m_pFSObject = NULL;
    }

    ShutdownNameResolution();

    if (NULL != m_rgConnPts)
    {
        for (UINT i = 0; i < m_cConnPts; i++)
        {
            if (NULL != m_rgConnPts[i])
            {
                m_rgConnPts[i]->Release();
                m_rgConnPts[i] = NULL;
            }
        }
        delete[] m_rgConnPts;
    }

    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::QueryInterface

    Description: Returns an interface pointer to the object's IUnknown,
        IDiskQuotaControl or IConnectionPointContainer interface.  The object 
        referenced by the returned interface pointer is uninitialized.  The 
        recipient of the pointer must call Initialize() before the object is 
        usable.

    Arguments:
        riid - Reference to requested interface ID.

        ppvOut - Address of interface pointer variable to accept interface ptr.

    Returns:
        NOERROR       - Success.
        E_NOINTERFACE - Requested interface not supported.
        E_INVALIDARG  - ppvOut argument was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaControl::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("DiskQuotaControl::QueryInterface")));
    DBGPRINTIID(DM_CONTROL, DL_MID, riid);

    if (NULL == ppvOut)
        return E_INVALIDARG;

    HRESULT hr = E_NOINTERFACE;

    try
    {
        *ppvOut = NULL;

        if (IID_IUnknown == riid || 
            IID_IDiskQuotaControl == riid)
        {
            *ppvOut = this;
        }
        else if (IID_IConnectionPointContainer == riid)
        {
            hr = InitConnectionPoints();
            if (SUCCEEDED(hr))
            {
                *ppvOut = static_cast<IConnectionPointContainer *>(this);
            }
        }
        else if (IID_IDispatch == riid ||
                 IID_DIDiskQuotaControl == riid)
        {
            DiskQuotaControlDisp *pQCDisp = new DiskQuotaControlDisp(static_cast<PDISKQUOTA_CONTROL>(this));
            *ppvOut = static_cast<DIDiskQuotaControl *>(pQCDisp);
        }
        if (NULL != *ppvOut)
        {
            ((LPUNKNOWN)*ppvOut)->AddRef();
            hr = NOERROR;
        }
    }
    catch(CAllocException& e)
    {   
        DBGERROR((TEXT("Insufficient memory exception")));
        *ppvOut = NULL;
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        *ppvOut = NULL;
        hr = E_UNEXPECTED;
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::AddRef

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
DiskQuotaControl::AddRef(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_LOW, TEXT("DiskQuotaControl::AddRef")));
    DBGPRINT((DM_CONTROL, DL_LOW, TEXT("\t0x%08X  %d -> %d"),
                     this, m_cRef, m_cRef + 1));

    ULONG ulReturn = m_cRef + 1;

    InterlockedIncrement(&m_cRef);

    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::Release

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
DiskQuotaControl::Release(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_LOW, TEXT("DiskQuotaControl::Release")));
    DBGPRINT((DM_CONTROL, DL_LOW, TEXT("\t0x%08X  %d -> %d"),
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
/*  Function: DiskQuotaControl::Initialize

    Description: Initializes a quota controller object by opening the NTFS 
        "device" associated with the quota information. The caller passes the
        name of an NTFS volume device to open.  A C++ object is created which
        encapsulates the required NTFS functionality.  This object is known
        as a "file system object" or FSObject.

        Currently, NTFS only supports quotas on volumes.  However, there is
        talk of providing quotas for directories in the future.  This library
        has been designed with this expansion in mind.
        By using an object hierarchy to represent the FSObject,
        we are able to shield the quota control object from differences
        in NTIO API functions dealing with volumes, directories and both
        local and remote flavors of both.  


    Arguments:
        pszPath - Name of NTFS path to open.

        bReadWrite - TRUE  = Read/write.
                     FALSE = Read only.
    Returns:
        NOERROR         - Success.
        E_INVALIDARG    - pszPath arg was NULL.
        E_OUTOFMEMORY   - Insufficient memory.
        E_UNEXPECTED    - Unexpected exception.
        E_FAIL          - Error getting volume information.
        ERROR_ACCESS_DENIED (hr)  - Insufficient access to open FS object.
        ERROR_FILE_NOT_FOUND (hr) - Specified volume doesn't exist.
        ERROR_PATH_NOT_FOUND (hr) - Specified volume doesn't exist.
        ERROR_BAD_PATHNAME (hr)   - Invalid path name provided.
        ERROR_INVALID_NAME (hr)   - Invalid path name provided.
        ERROR_NOT_SUPPORTED (hr)  - Quotas not supported by volume.
        ERROR_ALREADY_INITIALIZED (hr) - Controller is already initialized.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    06/06/96    Added ansi-unicode thunk.                            BrianAu
    06/11/96    Added return of access granted value.                BrianAu
    09/05/96    Added exception handling.                            BrianAu
    09/23/96    Take a "lazy" position on creating the               BrianAu
                SidNameResolver object.  Should only create it when
                it will be needed (user enumeration).  Moved
                creation from here to CreateEnumUsers. 
    07/03/97    Added dwAccess argument.                             BrianAu
    08/15/97    Added "already initialized" check.                   BrianAu
                Removed InitializeA().  
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaControl::Initialize(
    LPCWSTR pszPath,
    BOOL bReadWrite
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::Initialize")));
    DBGPRINT((DM_CONTROL, DL_MID, TEXT("\tpath = \"%s\", bReadWrite = %d"),
              pszPath ? pszPath : TEXT("<null>"), bReadWrite));
    HRESULT hr = NOERROR;

    if (m_bInitialized)
    {
        //
        // Controller has already been initialized.
        // Re-initialization is not allowed.
        //
        hr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }
    else
    {
        if (NULL == pszPath)
            return E_INVALIDARG;

        try
        {
            DWORD dwAccess = GENERIC_READ | (bReadWrite ? GENERIC_WRITE : 0);
            hr = FSObject::Create(pszPath, 
                                  dwAccess,
                                  &m_pFSObject);

            m_bInitialized = SUCCEEDED(hr);
        }
        catch(CAllocException& e)
        {
            DBGERROR((TEXT("Insufficient memory exception")));
            hr = E_OUTOFMEMORY;
        }
        catch(...)
        {
            DBGERROR((TEXT("Unexpected C++ exception")));
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::CreateEnumUsers

    Description: Create a new enumerator object for enumerating over the users
        in a volume's quota information file.  The returned interface supports
        the normal OLE 2 enumeration members Next(), Reset(), Skip() and Clone().

    Arguments:
        rgpSids [optional] - Pointer to a list of SID pointers.  If 
            provided, only those users with SIDs included in the list are 
            returned.  This argument may be NULL in which case ALL users are
            included.  Any element containing a NULL pointer will terminate
            the list.

        cpSids [optional] - If pSidList is not NULL, this arg contains
            the count of entries in rgpSids.  If rgpSids is not NULL and this 
            argument contains 0, rgpSids is assumed to contain a terminating
            NULL pointer entry.

        fNameResolution - Can be one of the following:
        
            DISKQUOTA_USERNAME_RESOLVE_NONE
            DISKQUOTA_USERNAME_RESOLVE_SYNC
            DISKQUOTA_USERNAME_RESOLVE_ASYNC

        ppEnum - Address of interface variable to accept the IEnumDiskQuotaUser
            interface pointer.


    Returns:
        NOERROR        - Success.
        E_INVALIDARG   - ppEnum arg is NULL.
        E_OUTOFMEMORY  - Insufficient memory to create enumerator object.
        ERROR_ACCESS_DENIED (hr) - Need READ access to create enumerator.
        ERROR_NOT_READY (hr)     - Object not initialized.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/11/96    Added access control.                                BrianAu
    09/05/96    Added exception handling.                            BrianAu
    09/23/96    Added lazy creation of SidNameResolver object.       BrianAu
                Moved it from InitializeW().
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
DiskQuotaControl::CreateEnumUsers(
    PSID *rgpSids,
    DWORD cpSids,
    DWORD fNameResolution,
    IEnumDiskQuotaUsers **ppEnum
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::CreateEnumUsers")));

    HRESULT hr = E_FAIL;

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    if (NULL == ppEnum)
        return E_INVALIDARG;

    if (!m_pFSObject->GrantedAccess(GENERIC_READ))
    {
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        DiskQuotaUserEnum *pEnumUsers = NULL;
        try
        {
            if (NULL == m_pSidNameResolver)
            {
                //
                // If there's no SID/Name resolver object, create one.
                // We do this "as needed" because user enumeration is
                // the only controller function that requires a resolver.
                // If the client doesn't need a resolver, why create one?
                //
                SidNameResolver *pResolver = NULL;

                //
                // Create user SID/Name resolver object.
                //
                pResolver = new SidNameResolver(*this);

                hr = pResolver->QueryInterface(IID_ISidNameResolver,
                                               (LPVOID *)&m_pSidNameResolver);
                if (SUCCEEDED(hr))
                {
                    hr = m_pSidNameResolver->Initialize();
                    if (FAILED(hr))
                    {
                        //
                        // If resolver initialization fails, we can assume
                        // that the resolver's thread hasn't been created so
                        // it's OK to just call Release() instead of 
                        // Shutdown() followed by Release().  This is strongly 
                        // dependent on the initialization logic in the resolver's
                        // Initialize method.  There's a comment there also.
                        //
                        m_pSidNameResolver->Release();
                        m_pSidNameResolver = NULL;
                        pResolver          = NULL;
                    }
                }
            }
            if (NULL != m_pSidNameResolver)
            {
                //
                // Create and initialize the enumerator object.
                //
                pEnumUsers = new DiskQuotaUserEnum(static_cast<IDiskQuotaControl *>(this),
                                                   m_pSidNameResolver,
                                                   m_pFSObject);
                //
                // This can throw OutOfMemory.
                //
                hr = pEnumUsers->Initialize(fNameResolution,
                                            ENUMUSER_BUF_LEN,
                                            rgpSids, 
                                            cpSids);

                if (SUCCEEDED(hr))
                {
                    hr = pEnumUsers->QueryInterface(IID_IEnumDiskQuotaUsers, 
                                                    (LPVOID *)ppEnum);
                }
                else
                {
                    //
                    // Something failed after enumerator object was created.
                    // 
                    delete pEnumUsers;
                }
            }
        }
        catch(CAllocException& e)
        {
            DBGERROR((TEXT("Insufficient memory exception")));
            delete pEnumUsers;
            hr = E_OUTOFMEMORY;
        }
        catch(...)
        {
            DBGERROR((TEXT("Unexpected C++ exception")));
            delete pEnumUsers;
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::CreateUserBatch

    Description: Create a new user batch control object.  Batch control is
        provided to take advantage of the inherent batching properties of the
        NTIOAPI.  If many user records are being altered at one time, it is
        much more efficient to mark each of the users for "deferred update",
        submit each user object to the batch and then flush the batch to disk.

    Arguments:
        ppUserBatch - Address of interface variable to accept the IDiskQuotaUserBatch
            interface pointer.

    Returns:
        NOERROR        - Success.
        E_INVALIDARG   - ppOut arg is NULL.
        E_OUTOFMEMORY  - Insufficient memory to create batch object.
        ERROR_ACCESS_DENIED (hr) - Need WRITE access to create batch.
        ERROR_NOT_READY (hr)     - Object not initialized.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/06/96    Initial creation.                                    BrianAu
    08/11/96    Added access control.                                BrianAu
    09/05/96    Added exception handling.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaControl::CreateUserBatch(
    PDISKQUOTA_USER_BATCH *ppUserBatch
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::CreateUserBatch")));

    HRESULT hr = NOERROR;

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    if (NULL == ppUserBatch)
        return E_INVALIDARG;

    if (!m_pFSObject->GrantedAccess(GENERIC_WRITE))
    {
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        try
        {
            DiskQuotaUserBatch *pUserBatch = new DiskQuotaUserBatch(m_pFSObject);

            hr = pUserBatch->QueryInterface(IID_IDiskQuotaUserBatch, 
                                           (LPVOID *)ppUserBatch);
        }
        catch(CAllocException& e)        // From new or m_UserList ctor.
        {
            DBGERROR((TEXT("Insufficient memory exception")));
            hr = E_OUTOFMEMORY;
        }
        catch(...)
        {
            DBGERROR((TEXT("Unexpected C++ exception")));
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::AddUserSid

    Description: Adds a new user to the volume's quota information file.
        If successful, returns an interface to the new user object.  When
        the caller is finished with the interface, they must call Release()
        through that interface pointer.  Uses the default limit and threshold.

    Arguments:
        pSid - Pointer to single SID structure.

        fNameResolution - Method of SID-to-name resolution. Can be one of the 
            following:
                    DISKQUOTA_USERNAME_RESOLVE_NONE
                    DISKQUOTA_USERNAME_RESOLVE_SYNC
                    DISKQUOTA_USERNAME_RESOLVE_ASYNC

        ppUser - Address of interface pointer variable to accept 
            pointer to the new user object's IDiskQuotaUser interface.

    Returns:
        SUCCESS       - Success.
        S_FALSE       - User already exists.  Not added.
        E_OUTOFMEMORY - Insufficient memory.
        E_UNEXPECTED  - Unexpected exception.
        E_INVALIDARG  - pSid or ppUser were NULL.
        ERROR_NOT_READY (hr) - Object not initialized.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    09/30/96    Added implementation.  Was E_NOTIMPL.                BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaControl::AddUserSid(
    PSID pSid, 
    DWORD fNameResolution,
    PDISKQUOTA_USER *ppUser
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::AddUserSid")));

    HRESULT hr = E_FAIL;
    PDISKQUOTA_USER pIUser = NULL;

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    if (NULL == pSid || NULL == ppUser)
        return E_INVALIDARG;

    LONGLONG llLimit     = 0;
    LONGLONG llThreshold = 0;
    LONGLONG llUsed      = 0;

    *ppUser = NULL;

    //
    // Check to see if the user already exists in the quota file.
    //
    try
    {
        hr = FindUserSid(pSid,
                         DISKQUOTA_USERNAME_RESOLVE_NONE,
                         &pIUser);

        if (SUCCEEDED(hr))
        {
            //
            // The NTIOAPI says the user exists.  
            // We'll need the quota info to determine if we
            // still allow addition of the "new" user.  This is needed because
            // of the weird way the NTIOAPI enumerates users.  If you ask it to
            // enumerate specified user(s), the returned information will include
            // info for users that do not have (but could have) a record in the
            // quota file.  Since the quota system allows automatic addition of
            // users, it considers any users with write access to have a record
            // in the quota file.  Such users are returned with a quota threshold
            // and limit of 0.  Therefore, we treat records marked for deletion
            // or those with 0 used, 0 limit and 0 threshold as "non existing".
            // I use the term "ghost" for these users.
            //
            pIUser->GetQuotaLimit(&llLimit);
            pIUser->GetQuotaThreshold(&llThreshold);
            pIUser->GetQuotaUsed(&llUsed);

            ULARGE_INTEGER a,b,c;
            a.QuadPart = llLimit;
            b.QuadPart = llThreshold;
            c.QuadPart = llUsed;
            DBGPRINT((DM_CONTROL, DL_LOW, TEXT("Found user: Limit = 0x%08X 0x%08X, Threshold = 0x%08X 0x%08X, Used = 0x%08X 0x%08X"),
                      a.HighPart, a.LowPart, b.HighPart, b.LowPart, c.HighPart, c.LowPart));


            BOOL bIsGhost = ((MARK4DEL == llLimit) ||
                            ( 0 == llLimit && 
                              0 == llThreshold && 
                              0 == llUsed));

            if (!bIsGhost)
            {
                //
                // User already exists.  
                //
                hr = S_FALSE;
            }
            else
            {
                DWORD cbSid = GetLengthSid(pSid);

                //
                // User not in quota file OR in quota file but marked for deletion.  
                // Just set it's limit and threshold to the volume defaults.
                //
                pIUser->SetQuotaThreshold(m_llDefaultQuotaThreshold, TRUE);
                hr = pIUser->SetQuotaLimit(m_llDefaultQuotaLimit, TRUE);

                if (SUCCEEDED(hr) && NULL != m_pSidNameResolver)
                {
                    //
                    // We have a good user object and have set the quota parameters.
                    // Get the user's domain, name and full name from the network DC
                    // using the resolution type specified by the caller.
                    //
                    switch(fNameResolution)
                    {
                        case DISKQUOTA_USERNAME_RESOLVE_ASYNC:
                            m_pSidNameResolver->FindUserNameAsync(pIUser);
                            break;
                        case DISKQUOTA_USERNAME_RESOLVE_SYNC:
                            m_pSidNameResolver->FindUserName(pIUser);
                            break;
                        case DISKQUOTA_USERNAME_RESOLVE_NONE:
                        default:
                            break;
                    }
                }
            }
            *ppUser = pIUser;
        }
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        hr = E_UNEXPECTED;
    }
    if (FAILED(hr))
    {
        *ppUser = NULL;
        if (NULL != pIUser)
        {
            pIUser->Release();
        }
    }

    return hr;
}


STDMETHODIMP 
DiskQuotaControl::AddUserName(
    LPCWSTR pszLogonName,
    DWORD fNameResolution,
    PDISKQUOTA_USER *ppUser
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::AddUserName")));

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    if (NULL == pszLogonName || NULL == ppUser)
        return E_INVALIDARG;

    HRESULT hr = E_FAIL;
    try
    {
        BYTE Sid[MAX_SID_LEN];
        DWORD cbSid = sizeof(Sid);
        SID_NAME_USE eSidUse;

        if (SUCCEEDED(m_NTDS.LookupAccountByName(NULL,         // system
                                                 pszLogonName, // key
                                                 NULL,         // no container ret
                                                 NULL,         // no display name ret
                                                 &Sid[0],
                                                 &cbSid,
                                                 &eSidUse)))
        {
            hr = AddUserSid(&Sid[0], fNameResolution, ppUser);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
        }
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        hr = E_UNEXPECTED;
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::ShutdownNameResolution

    Description: Release the SID/Name resolver.  This terminates the
        resolver thread for clients who don't want to wait for the controller
        object to be destroyed.  Note that subsequent calls to CreateEnumUsers,
        AddUserSid, AddUserName, FindUserSid or FindUserName can restart
        the resolver.

    Arguments: None.

    Returns: Always returns NOERROR

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/29/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaControl::ShutdownNameResolution(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::ShutdownNameResolution")));
    if (NULL != m_pSidNameResolver)
    {
        //
        // Shutdown and release the resolver.
        // Since it's running on it's own thread, we must wait for the thread
        // to exit.
        // Note that if the thread is off resolving a name from the DC, this could
        // take a bit.
        //
        m_pSidNameResolver->Shutdown(TRUE); // TRUE == Wait for thread exit.

        m_pSidNameResolver->Release();  
        m_pSidNameResolver = NULL;
    }
    return NOERROR;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::GiveUserNameResolutionPriority

    Description: A very long name for a very simple function.
        This function merely finds the user object in the name resolver's
        input queue and moves it to the head of the queue.

    Arguments:
        pUser - Address of interface pointer for the user object's 
                IDiskQuotaUser interface.

    Returns:
        NOERROR       - Success.
        S_FALSE       - User object not in resolver queue.
        E_OUTOFMEMORY - Insufficient memory.
        E_INVALIDARG  - pUser is NULL.
        E_UNEXPECTED  - Unexpected error.  Caught an exception or the 
                        Sid-Name resolver hasn't been created.
        ERROR_NOT_READY (hr) - Object not initialized.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaControl::GiveUserNameResolutionPriority(
    PDISKQUOTA_USER pUser
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::GiveUserNameResolutionPriority")));
    HRESULT hr = E_UNEXPECTED;

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    if (NULL == pUser)
        return E_INVALIDARG;

    //
    // SidNameResolver::PromoteUserToQueueHeader catches exceptions and
    // converts them to HRESULTs.  No need for try-catch block here.
    //
    if (NULL != m_pSidNameResolver)
    {
        hr = m_pSidNameResolver->PromoteUserToQueueHead(pUser);
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::FindUserSid

    Description: Finds a single user record in a volume's quota information
        file.  Returns an interface to the corresponding user object.  When
        the caller is finished with the interface, they must call Release()
        through that interface pointer.


                   >>>>>>>>> IMPORTANT NOTE <<<<<<<<<

        This method will return a user object even if there is no quota
        record for the user in the quota file.  While that may sound
        strange, it is consistent with the idea of automatic user addition
        and default quota settings.  If there is currently no user record
        for the requested user, and the user would be added to the quota
        file if they were to request disk space, the returned user object
        will have a quota threshold of 0 and a quota limit of 0.

    Arguments:
        pSid - Pointer to single SID structure identifying the user.

        fNameResolution -  Can be one of the following:

            DISKQUOTA_USERNAME_RESOLVE_NONE
            DISKQUOTA_USERNAME_RESOLVE_SYNC
            DISKQUOTA_USERNAME_RESOLVE_ASYNC

        ppUser - Address of interface pointer variable to accept pointer to 
            the user object's IDiskQuotaUser interface.

    Returns:
        NOERROR       - Success.
        E_INVALIDARG  - Either pSid or ppUser were NULL.
        E_OUTOFMEMORY - Insufficient memory.
        E_UNEXPECTED  - Unexpected exception.
        ERROR_INVALID_SID (hr)   - Invalid SID.
        ERROR_ACCESS_DENIED (hr) - No READ access to quota device.
        ERROR_NO_SUCH_USER (hr)  - User not found in volume's quota information.
        ERROR_NOT_READY (hr)     - Object not initialized.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/14/96    Changed name from FindUser to FindUserSid to         BrianAu
                accomodate the addition of the FindUserName
                methods.  No change in functionality.
    09/05/96    Added exception handling.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaControl::FindUserSid(
    PSID pSid, 
    DWORD fNameResolution,
    PDISKQUOTA_USER *ppUser
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::FindUserSid")));
    HRESULT hr = NOERROR;

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    if (NULL == pSid || NULL == ppUser)
        return E_INVALIDARG;


    if (!m_pFSObject->GrantedAccess(GENERIC_READ))
    {
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        if (!IsValidSid(pSid))
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_SID);
        }
        else
        {
            PENUM_DISKQUOTA_USERS pEnumUsers = NULL;
            try
            {
                DWORD cbSid = GetLengthSid(pSid);

                *ppUser = NULL;

                //
                // Create a user enumerator for the user's SID.
                // Can throw OutOfMemory.
                //
                hr = CreateEnumUsers(&pSid, 1, fNameResolution, &pEnumUsers);
                if (SUCCEEDED(hr))
                {
                    DWORD dwUsersFound    = 0;
                    PDISKQUOTA_USER pUser = NULL;
                    //
                    // Enumerate 1 record to get the user's info.
                    // Only one record required since the enumerator object
                    // was created from a single SID. Can throw OutOfMemory.
                    //
                    hr = pEnumUsers->Next(1, &pUser, &dwUsersFound);
                    if (S_OK == hr)
                    {
                        //
                        // Return user object interface to caller.
                        //
                        *ppUser = pUser;
                    }
                    else if (S_FALSE == hr)
                    {
                        //
                        // Note:  We should never hit this.
                        //        The quota system always returns a user record
                        //        for a user SID.  If the record doesn't currently
                        //        exist, one with default limit and threshold is
                        //        returned.  This is consistent with the idea
                        //        of automatic user record addition implemented
                        //        by the NTFS quota system.  Just in case we do,
                        //        I want to return something intelligent.
                        //
                        hr = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
                    }
                }
            }
            catch(CAllocException& e)
            {
                DBGERROR((TEXT("Insufficient memory exception")));
                hr = E_OUTOFMEMORY;
            }
            catch(...)
            {
                DBGERROR((TEXT("Unexpected C++ exception")));
                hr = E_UNEXPECTED;
            }
            if (NULL != pEnumUsers)
            {
                //
                // Release the enumerator.
                //
                pEnumUsers->Release();
            }
        }
    }

    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::FindUserName

    Description: Finds a single user record in a volume's quota information
        file.  Returns an interface to the corresponding user object.  When
        the caller is finished with the interface, they must call Release()
        through that interface pointer.  
        If the name is not already cached in the SidNameCache, the function
        queries the network domain controller.  This operation may take some
        time (on the order of 0 - 10 seconds).

    Arguments:
        pszLogonName - Address of user's logon name string.
            i.e. "REDMOND\brianau" or "brianau@microsoft.com"

        ppUser - Address of interface pointer variable to accept pointer to 
            the user object's IDiskQuotaUser interface.

    Returns:
        NOERROR       - Success.
        E_INVALIDARG  - Name string is blank or NUL ptr was passed.
        E_OUTOFMEMORY - Insufficient memory.
        E_UNEXPECTED  - Unexpected exception.
        ERROR_ACCESS_DENIED (hr) - No READ access to quota device.
        ERROR_NO_SUCH_USER (hr)  - User not found in quota file.
        ERROR_NOT_READY (hr)     - Object not initialized.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/14/96    Initial creation.                                    BrianAu
    09/05/96    Added domain name string.                            BrianAu
                Added exception handling.
    08/15/97    Removed ANSI version.                                BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaControl::FindUserName(
    LPCWSTR pszLogonName,
    PDISKQUOTA_USER *ppUser
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::FindUserName")));

    HRESULT hr                = E_FAIL; // Assume failure.
    BOOL bAskDomainController = TRUE;

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    if (NULL == pszLogonName || NULL == ppUser)
        return E_INVALIDARG;

    if (TEXT('\0') == *pszLogonName)
        return E_INVALIDARG;

    //
    // Check for client's access to quota file before we do any 
    // time-expensive operations.
    //
    if (!m_pFSObject->GrantedAccess(GENERIC_READ))
    {
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED); 
    }
    else
    {
        PSID pSid = NULL;      // For cache query.
        SID Sid[MAX_SID_LEN];  // For DC query.
        //
        // These nested try-catch blocks look really gross and may 
        // be unnecessary.  I should probably just punt if one of the
        // inner blocks really does throw an exception and return
        // E_UNEXPECTED instead of trying to continue. [brianau]
        //
        try
        {
            LOCK_GLOBAL_DATA;
            try
            {
                if (NULL == g_pSidCache)
                {
                    //
                    // This can throw OutOfMemory.
                    //
                    hr = SidNameCache::CreateNewCache(&g_pSidCache);
                }
            }
            catch(...)
            {
                //
                // Couldn't create SID/Name cache.
                // Don't panic.  We'll just have to wait and resolve the name.
                //
                DBGERROR((TEXT("C++ exception during SID cache creation in FindUserName")));
                g_pSidCache = NULL;
            }

            SidNameCache *pSidCache = g_pSidCache; // Local copy.
            RELEASE_GLOBAL_DATA;

            if (NULL != pSidCache)
            {
                //
                // See if the SID/Name pair is in the cache.
                //
                try
                {
                    hr = pSidCache->Lookup(pszLogonName, &pSid);
                    if (SUCCEEDED(hr))
                    {
                        //
                        // We have a SID.  No need to ask DC.
                        //
                        bAskDomainController = FALSE;
                    }
                }
                catch(...)
                {
                    //
                    // Just catch the exception.
                    // This will cause us to go to the DC for the SID.
                    //
                    DBGERROR((TEXT("C++ exception during SID cache lookup in FindUserName")));
                    pSid = &Sid[0];
                }
            }

            if (bAskDomainController)
            {
                DBGASSERT((FAILED(hr)));

                //
                // Still don't have a SID.  Ask the DC.
                // This can take some time (Ho Hum.........)
                //
                CString strDisplayName;
                CString strContainerName;
                SID_NAME_USE eUse;
                DWORD cbSid = sizeof(Sid);

                if (m_NTDS.LookupAccountByName(NULL,
                                               pszLogonName,
                                               &strContainerName,
                                               &strDisplayName,
                                               &Sid[0],
                                               &cbSid,
                                               &eUse))
                {
                    //
                    // Add it to the cache for later use.
                    //
                    if (NULL != pSidCache)
                    {
                        pSidCache->Add(&Sid[0], 
                                       strContainerName,
                                       pszLogonName,
                                       strDisplayName);
                    }

                    hr = NOERROR;
                }
            }

            if (SUCCEEDED(hr))
            {
                //
                // We have a SID.
                // Now create the actual user object using FindUserSid().
                //
                hr = FindUserSid(pSid, DISKQUOTA_USERNAME_RESOLVE_SYNC, ppUser);
            }
        }
        catch(CAllocException& e)
        {
            DBGERROR((TEXT("Insufficient memory exception")));
            hr = E_OUTOFMEMORY;
        }
        catch(...)
        {
            DBGERROR((TEXT("Unexpected C++ exception")));
            hr = E_UNEXPECTED;
        }
        if (&Sid[0] != pSid)
        {
            //
            // We received a heap-allocated SID from SidNameCache::Lookup.
            // Need to free the buffer.
            //
            delete[] pSid;
        }
    }

    return hr;
}




///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::DeleteUser

    Description: Deletes a user from a volume's quota information and quota
        tracking.  The IDiskQuotaUser pointer may be obtained either through
        enumeration or DiskQuotaControl::FindUser().

        NOTE:  At this time, we're not sure how (or if) deletion will be done.
               This function remains un-implemented until we figure it out.

    Arguments:
        pUser - Pointer to quota user object's IDiskQuotaUser interface.

    Returns:
        NOERROR              - Success.
        E_OUTOFMEMORY        - Insufficient memory.
        E_UNEXPECTED         - Unexpected exception.
        E_FAIL               - NTIO error writing user data.
        E_INVALIDARG         - pUser argument was NULL.
        ERROR_FILE_EXISTS (hr)   - Couldn't delete.  User still has bytes charge.
        ERROR_ACCESS_DENIED (hr) - Insufficient access.
        ERROR_NOT_READY (hr)     - Object not initialized.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    09/28/96    Added implementation.  Was E_NOTIMPL.                BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaControl::DeleteUser(
    PDISKQUOTA_USER pUser
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::DeleteUser")));

    HRESULT hr = NOERROR;

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    if (NULL == pUser)
        return E_INVALIDARG;

    try
    {
        LONGLONG llValue;
        //
        // Invalidate user object to force a refresh of data from the
        // quota file.  Want to make sure this is current information before
        // we tell the caller that the user can't be deleted.
        //
        pUser->Invalidate();
        hr = pUser->GetQuotaUsed(&llValue);

        if (SUCCEEDED(hr))
        {
            if (0 == llValue)
            {
                //
                // User has 0 bytes in use.  OK to delete.
                // Write -2 to user's limit and threshold.
                //
                pUser->SetQuotaThreshold(MARK4DEL, TRUE);
                hr = pUser->SetQuotaLimit(MARK4DEL, TRUE);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
            }
        }
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        hr = E_UNEXPECTED;
    }

    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::QueryQuotaInformation

    Description: Read quota information from disk to member variables.

    Arguments: None.

    Returns:
        NOERROR            - Success.
        E_FAIL             - Any other error.
        ERROR_ACCESS_DENIED (hr) - No READ access to quota device.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/23/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DiskQuotaControl::QueryQuotaInformation(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_LOW, TEXT("DiskQuotaControl::QueryQuotaInformation")));

    HRESULT hr = NOERROR;
    DISKQUOTA_FSOBJECT_INFORMATION Info;

    hr = m_pFSObject->QueryObjectQuotaInformation(&Info);
    if (SUCCEEDED(hr))
    {
        m_llDefaultQuotaThreshold = Info.DefaultQuotaThreshold;
        m_llDefaultQuotaLimit     = Info.DefaultQuotaLimit;
        
        m_dwFlags = (Info.FileSystemControlFlags & DISKQUOTA_FLAGS_MASK);
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::SetQuotaInformation

    Description: Writes quota information from member variables to disk.

    Arguments: 
        dwChangeMask - A bit mask with one or more of the following bits set:
        
                FSObject::ChangeState
                FSObject::ChangeLogFlags
                FSObject::ChangeThreshold
                FSObject::ChangeLimit

    Returns:
        NOERROR            - Success.
        ERROR_ACCESS_DENIED (hr) - No WRITE access to quota device.
        E_FAIL              - Any other error.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/23/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DiskQuotaControl::SetQuotaInformation(
    DWORD dwChangeMask
    )
{
    DBGTRACE((DM_CONTROL, DL_LOW, TEXT("DiskQuotaControl::SetQuotaInformation")));

    HRESULT hr = NOERROR;
    DISKQUOTA_FSOBJECT_INFORMATION Info;

    Info.DefaultQuotaThreshold  = m_llDefaultQuotaThreshold;
    Info.DefaultQuotaLimit      = m_llDefaultQuotaLimit;
    Info.FileSystemControlFlags = m_dwFlags;

    hr = m_pFSObject->SetObjectQuotaInformation(&Info, dwChangeMask);
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::SetDefaultQuotaThreshold

    Description: Sets the default quota threshold value applied to new user 
        quota records.  Value is in bytes.

    Arguments:
        llThreshold - Threshold value.

    Returns:
        NOERROR    - Success.
        ERROR_ACCESS_DENIED (hr) - No WRITE access to quota device.
        ERROR_NOT_READY (hr)     - Object not initialized.
        ERROR_INVALID_PARAMETER  - llThreshold was less than -2.
        E_FAIL                   - Any other error.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/23/96    Initial creation.                                    BrianAu
    11/11/98    Added check for value < -2.                          BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaControl::SetDefaultQuotaThreshold(
    LONGLONG llThreshold
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::SetDefaultQuotaThreshold")));

    HRESULT hr = NOERROR;

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    if (MARK4DEL > llThreshold)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    m_llDefaultQuotaThreshold  = llThreshold;
    hr = SetQuotaInformation(FSObject::ChangeThreshold);

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::SetDefaultQuotaLimit

    Description: Sets the default quota limit value applied to new user 
        quota records.  Value is in bytes.

    Arguments:
        llThreshold - Limit value.

    Returns:
        NOERROR    - Success.
        ERROR_ACCESS_DENIED (hr) - No WRITE access to quota device.
        ERORR_NOT_READY (hr)     - Object not initialized.
        ERROR_INVALID_PARAMETER  - llLimit was less than -2.
        E_FAIL                   - Any other error.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/23/96    Initial creation.                                    BrianAu
    11/11/98    Added check for value < -2.                          BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaControl::SetDefaultQuotaLimit(
    LONGLONG llLimit
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::SetDefaultQuotaLimit")));

    HRESULT hr = NOERROR;

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    if (MARK4DEL > llLimit)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    m_llDefaultQuotaLimit  = llLimit;
    hr = SetQuotaInformation(FSObject::ChangeLimit);

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::GetDefaultQuotaItem

    Description: Retrieves one of the default quota items (limit, threshold)
        applied to new user quota records.  Value is in bytes.

    Arguments:
        pllItem - Address of item (limit, threshold, used) value item to 
            retrieve (member variable).

        pllValueOut - Address of LONGLONG variable to receive value.

    Returns:
        NOERROR             - Success.
        E_INVALIDARG        - pdwLowPart or pdwHighPart is NULL.
        E_UNEXPECTED        - Unexpected exception.
        E_OUTOFMEMORY       - Insufficient memory.
        ERROR_ACCESS_DENIED (hr) - No READ access to quota device.
        E_FAIL              - Any other error.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/23/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DiskQuotaControl::GetDefaultQuotaItem(
    PLONGLONG pllItem,
    PLONGLONG pllValueOut
    )
{
    DBGTRACE((DM_CONTROL, DL_LOW, TEXT("DiskQuotaControl::GetDefaultQuotaItem")));

    HRESULT hr = NOERROR;

    if (NULL == pllItem || NULL == pllValueOut)
        return E_INVALIDARG;

    try
    {
        hr = QueryQuotaInformation();
        if (SUCCEEDED(hr))
        {
            *pllValueOut = *pllItem;
        }
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        hr = E_UNEXPECTED;
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::GetDefaultQuotaThreshold

    Description: Retrieves the default quota threshold applied to new user
        quota records.  Value is in bytes.

    Arguments:
        pllThreshold - Address of LONGLONG to receive threshold value.

    Returns:
        NOERROR             - Success.
        E_INVALIDARG        - pdwLowPart or pdwHighPart is NULL.
        E_UNEXPECTED        - Unexpected exception.
        E_OUTOFMEMORY       - Insufficient memory.
        ERROR_ACCESS_DENIED (hr) - No READ access to quota device.
        ERROR_NOT_READY (hr)     - Object not initialized.
        E_FAIL                   - Any other error.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/23/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaControl::GetDefaultQuotaThreshold(
    PLONGLONG pllThreshold
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::GetDefaultQuotaThreshold")));

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    return GetDefaultQuotaItem(&m_llDefaultQuotaThreshold, pllThreshold);
}


STDMETHODIMP
DiskQuotaControl::GetDefaultQuotaThresholdText(
    LPWSTR pszText,
    DWORD cchText
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::GetDefaultQuotaThresholdText")));

    if (NULL == pszText)
        return E_INVALIDARG;

    LONGLONG llValue;
    HRESULT hr = GetDefaultQuotaThreshold(&llValue);
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


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::GetDefaultQuotaLimit

    Description: Retrieves the default quota limit applied to new user
        quota records.  Value is in bytes.

    Arguments:
        pllThreshold - Address of LONGLONG to receive limit value.

    Returns:
        NOERROR             - Success.
        E_INVALIDARG        - pdwLowPart or pdwHighPart is NULL.
        E_UNEXPECTED        - Unexpected exception.
        E_OUTOFMEMORY       - Insufficient memory.
        ERROR_ACCESS_DENIED (hr) - No READ access to quota device.
        ERROR_NOT_READY (hr)     - Object not initialized.
        E_FAIL              - Any other error. // BUBUG: conflict?

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/23/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaControl::GetDefaultQuotaLimit(
    PLONGLONG pllLimit
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::GetDefaultQuotaLimit")));

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    return GetDefaultQuotaItem(&m_llDefaultQuotaLimit, pllLimit);
}


STDMETHODIMP
DiskQuotaControl::GetDefaultQuotaLimitText(
    LPWSTR pszText,
    DWORD cchText
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::GetDefaultQuotaLimitText")));

    if (NULL == pszText)
        return E_INVALIDARG;

    LONGLONG llValue;
    HRESULT hr = GetDefaultQuotaLimit(&llValue);
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


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::GetQuotaState

    Description: Retrieve the state of the quota system.
        
    Arguments:
        pdwState - Address of DWORD to accept the quota state value.
            Returned value is formatted as follows:
            
            Bit(s)   Definition
            -------  ---------------------------------------------------
            00-01    0 = Disabled  (DISKQUOTA_STATE_DISABLED)
                     1 = Tracking  (DISKQUOTA_STATE_TRACK)
                     2 = Enforcing (DISKQUOTA_STATE_ENFORCE)
                     3 = Invalid value.
            02-07    Reserved
            08       1 = Quota file incomplete.
            09       1 = Rebuilding quota file.
            10-31    Reserved.

            Use the macros defined in dskquota.h to query the 
            values and bits in this state DWORD.

    Returns:
        NOERROR             - Success.
        E_INVALIDARG        - pdwState arg is NULL.
        E_UNEXPECTED        - Unexpected exception.
        E_OUTOFMEMORY       - Insufficient memory.
        ERROR_ACCESS_DENIED (hr) - No READ access to quota device.
        ERROR_NOT_READY (hr)     - Object not initialized.
        E_FAIL                   - Any other error.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/02/96    Initial creation.                                    BrianAu
    08/19/96    Added DISKQUOTA_FILEFLAG_MASK.                       BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaControl::GetQuotaState(
    LPDWORD pdwState
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::GetQuotaState")));
    HRESULT hr = NOERROR;

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    if (NULL == pdwState)
        return E_INVALIDARG;

    try
    {
        hr = QueryQuotaInformation();
        if (SUCCEEDED(hr))
        {
            DWORD dwMask = DISKQUOTA_STATE_MASK | DISKQUOTA_FILEFLAG_MASK;
            *pdwState = (m_dwFlags & dwMask);
        }
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        hr = E_UNEXPECTED;
    }
    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::SetQuotaState

    Description: Sets the quota state flags in the volume's quota info file.
        The quota state may be one of the following:
            - Disabled.
            - Tracking quotas (no enforcement).
            - Enforcing quota limits.

    Arguments:
        dwState - New state of quota system.  The bits in this DWORD are
            defined as follows:
            
            Bit(s)   Definition
            -------  ---------------------------------------------------
            00-01    0 = Disabled  (DISKQUOTA_STATE_DISABLED)
                     1 = Tracking  (DISKQUOTA_STATE_TRACK)
                     2 = Enforcing (DISKQUOTA_STATE_ENFORCE)
                     3 = Invalid value.
            02-07    Reserved
            08       1 = Quota file incomplete (read only)
            09       1 = Rebuilding quota file (read only)
            10-31    Reserved.

            Use the macros defined in dskquota.h to set the 
            values and bits in this state DWORD.

    Returns:
        NOERROR             - Success.
        E_INVALIDARG        - Invalid state value.
        E_UNEXPECTED        - Unexpected exception.
        E_OUTOFMEMORY       - Insufficient memory.
        ERROR_ACCESS_DENIED (hr) - No WRITE access to quota device.
        ERROR_NOT_READY (hr)     - Object not initialized.
        E_FAIL                   - Any other error.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/02/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaControl::SetQuotaState(
    DWORD dwState
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::SetQuotaState")));

    HRESULT hr = NOERROR;

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    try
    {
        if (dwState <= DISKQUOTA_STATE_MASK)
        {

            m_dwFlags &= ~DISKQUOTA_STATE_MASK; // Clear current state bits.
            m_dwFlags |= dwState;               // Set new state bits.

            hr = SetQuotaInformation(FSObject::ChangeState);
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        hr = E_UNEXPECTED;
    }
    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::GetQuotaLogFlags

    Description: Retrieve the state of the quota logging system.
        
    Arguments:
        pdwFlags - Address of DWORD to accept the quota logging flags.
            The bits in the flags DWORD are defined as follows:

            Bit(s)   Definition
            -------  ---------------------------------------------------
            00       1 = Logging user threshold violations.
            01       1 = Logging user limit violations.
            02       1 = Logging volume threshold violations.
            03       1 = Logging volume limit violations.
            04-31    Reserved.

            Use the macros defined in dskquota.h to query the 
            values and bits in this flags DWORD.

    Returns:
        NOERROR             - Success.
        E_INVALIDARG        - pdwFlags arg is NULL.
        E_UNEXPECTED        - Unexpected exception.
        E_OUTOFMEMORY       - Insufficient memory.
        ERROR_ACCESS_DENIED (hr) - No READ access to quota device.
        ERROR_NOT_READY (hr)     - Object not initialized.
        E_FAIL                   - Any other error.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/02/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaControl::GetQuotaLogFlags(
    LPDWORD pdwFlags
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::GetQuotaLogFlags")));

    HRESULT hr = NOERROR;

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    if (NULL == pdwFlags)
        return E_INVALIDARG;

    try
    {
        hr = QueryQuotaInformation();
        if (SUCCEEDED(hr))
        {
            *pdwFlags = ((m_dwFlags & DISKQUOTA_LOGFLAG_MASK) >> DISKQUOTA_LOGFLAG_SHIFT);
        }
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        hr = E_UNEXPECTED;
    }
    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::SetQuotaLogFlags

    Description: Sets the quota logging state flags in the volume's quota 
        info file.

    Arguments:
        dwFlags - New state of quota logging. 
            The bits in the flags DWORD are defined as follows:

            Bit(s)   Definition
            -------  ---------------------------------------------------
            00       1 = Logging user threshold violations.
            01       1 = Logging user limit violations.
            02       1 = Logging volume threshold violations.
            03       1 = Logging volume limit violations.
            04-31    Reserved.

            Use the macros defined in dskquota.h to set the 
            values and bits in this flags DWORD.

    Returns:
        NOERROR             - Success.
        E_UNEXPECTED        - Unexpected exception.
        E_OUTOFMEMORY       - Insufficient memory.
        ERROR_ACCESS_DENIED (hr) - No WRITE access to quota device.
        ERROR_NOT_READY (hr)     - Object not initialized.
        E_FAIL                   - Any other error.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/02/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaControl::SetQuotaLogFlags(
    DWORD dwFlags
    )     
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::SetQuotaLogFlags")));

    HRESULT hr = NOERROR;

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);

    try
    {
        m_dwFlags &= ~DISKQUOTA_LOGFLAG_MASK;
        m_dwFlags |= (dwFlags << DISKQUOTA_LOGFLAG_SHIFT);
        hr = SetQuotaInformation(FSObject::ChangeLogFlags);
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        hr = E_UNEXPECTED;
    }
    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::InitConnectionPoints

    Description: Private function for initializing the connection point
        objects supported by IConnectionPointContainer.  Called from
        DiskQuotaControl::Initialize().  To add a new connection point
        type, merely add a new record to the m_rgConnPtDesc[] array in the
        DiskQuotaControl class declaration.  All of the other related code
        in DiskQuotaControl will adjust to it automatically.

    Arguments: None.

    Returns:
        NOERROR        - Success.
        E_UNEXPECTED   - A connection point pointer was non-NULL.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
    09/05/96    Added exception handling.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DiskQuotaControl::InitConnectionPoints(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("DiskQuotaControl::InitConnectionPoints")));

    if (NULL != m_rgConnPts)
    {
        //
        // Already initialized.  
        //
        return NOERROR;
    }

    HRESULT hr = NOERROR;
    ConnectionPoint *pConnPt = NULL;

    m_cConnPts = ARRAYSIZE(m_rgpIConnPtsSupported);

    try
    {
        m_rgConnPts = new PCONNECTIONPOINT[m_cConnPts];

        //
        // For each of the connection point IIDs in m_rgpIConnPtsSupported[]...
        //
        for (UINT i = 0; i < m_cConnPts && SUCCEEDED(hr); i++)
        {
            m_rgConnPts[i] = NULL;

            //
            // Create connection point object and query for IConnectionPoint interface.
            //
            pConnPt = new ConnectionPoint(static_cast<IConnectionPointContainer *>(this), 
                                          *m_rgpIConnPtsSupported[i]);

            hr = pConnPt->QueryInterface(IID_IConnectionPoint, (LPVOID *)&m_rgConnPts[i]);

            if (FAILED(hr))
            {
                // 
                // Either Initialize or QI failed.
                //
                delete pConnPt;
                pConnPt = NULL;
            }
        }
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        delete pConnPt;
        throw;
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::FindConnectionPoint

    Description: Queries the quota control object for a specific connection
        point type.  If that type is supported, a pointer to the connection
        point's IConnectionPoint interface is returned.

    Arguments:
        riid - Interface ID of desired connection point interface.
            Supported interfaces:
                IID_IDiskQuotaUserEvents
                    - OnNameChanged()


    Returns:
        NOERROR         - Success.
        E_INVALIDARG    - ppConnPtOut arg is NULL.
        E_NOINTERFACE   - Requested interface is not supported.
        E_UNEXPECTED    - Connection point object pointer is NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaControl::FindConnectionPoint(
    REFIID riid,
    IConnectionPoint **ppConnPtOut
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::FindConnectionPoint")));
    DBGPRINTIID(DM_CONTROL, DL_HIGH, riid);

    HRESULT hr = E_NOINTERFACE;

    if (NULL == ppConnPtOut)
        return E_INVALIDARG;

    *ppConnPtOut = NULL;
    for (UINT i = 0; i < m_cConnPts && NULL == *ppConnPtOut; i++)
    {
        if (*m_rgpIConnPtsSupported[i] == riid)
        {
            if (NULL != m_rgConnPts[i])
            {
                //
                // We have an IID match.  
                // Now get the conn pt interface pointer.
                //
                hr = m_rgConnPts[i]->QueryInterface(IID_IConnectionPoint, (LPVOID *)ppConnPtOut);
            }
            else
            {
                hr = E_UNEXPECTED;
                break;
            }
        }
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::EnumConnectionPoints

    Description: Creates a connection point enumerator object.
        Using this object, the client can enumerate through all of the
        connection point interfaces supported by the quota controller.

    Arguments:
        ppEnum - Address of interface pointer variable to receive the 
            IEnumConnectionPoints interface.

    Returns:
        NOERROR         - Success.
        E_OUTOFMEMORY   - Insufficient memory to create object(s).
        E_INVALIDARG    - ppEnum arg was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaControl::EnumConnectionPoints(
    IEnumConnectionPoints **ppEnum
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::EnumConnectionPoints")));

    HRESULT hr = NOERROR;

    if (NULL == ppEnum)
        return E_INVALIDARG;

    PCONNECTIONPOINT rgCP[ARRAYSIZE(m_rgpIConnPtsSupported)];
    ConnectionPointEnum *pEnum = NULL;

    *ppEnum = NULL;

    for (UINT i = 0; i < m_cConnPts; i++)
    {
        //
        // Make a copy of each connection point pointer
        // to give to the enumerator's Initialize() method.
        //
        m_rgConnPts[i]->AddRef();
        rgCP[i] = m_rgConnPts[i];
    }

    try
    {
        pEnum = new ConnectionPointEnum(static_cast<IConnectionPointContainer *>(this), 
                                        m_cConnPts, rgCP);

        hr = pEnum->QueryInterface(IID_IEnumConnectionPoints, (LPVOID *)ppEnum);
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        hr = E_UNEXPECTED;
    }
    if (FAILED(hr) && NULL != pEnum)
    {
        delete pEnum;
        *ppEnum = NULL;
    }

    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControl::InvalidateSidNameCache

    Description: Invalidates the contents of the SidNameCache so that future
        requests for account names from the cache must be resolved through
        the DC.  As names are resolved, they are again added to the cache.

    Arguments: None.

    Returns:
        NOERROR       - Cache invalidated.
        E_OUTOFMEMORY - Insufficient memory.
        E_UNEXPECTED  - Unexpected exception.
        E_FAIL        - No cache object available or couldn't get lock on
                        cache files.  Either way, cache wasn't invalidated.
        ERROR_NOT_READY (hr)  - Object not initialized.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/24/96    Initial creation.                                    BrianAu
    09/20/96    Updated for new cache design.                        BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaControl::InvalidateSidNameCache(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::InvalidateSidNameCache")));

    if (!m_bInitialized)
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);


    HRESULT hr = E_FAIL;
    LOCK_GLOBAL_DATA;
    try
    {
        if (NULL == g_pSidCache)
        {
            hr = SidNameCache::CreateNewCache(&g_pSidCache);
        }
        if (NULL != g_pSidCache)
        {
            if (g_pSidCache->Clear())
                hr = NOERROR;
        }
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        hr = E_UNEXPECTED;
    }
    RELEASE_GLOBAL_DATA;

    return hr;
}
    

///////////////////////////////////////////////////////////////////////////////
/*  Function:  DiskQuotaControl::NotifyUserNameChanged

    Description: Notify all user IDiskQuotaControl Event connections that a 
        user's name has changed.

    Arguments:
        pUser - Address of user object's IDiskQuotaUser interface.

    Returns:
        NOERROR       - Success.
        E_OUTOFMEMORY - Insufficient memory to create enumerator.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/22/96    Initial creation.                                    BrianAu
    08/25/97    Added support for IPropertyNotifySink                BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
DiskQuotaControl::NotifyUserNameChanged(
    PDISKQUOTA_USER pUser
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControl::NotifyUserNameChanged")));

    HRESULT hr = NOERROR;
    PCONNECTIONPOINT pConnPt = NULL;
    bool bAbort = false;

    INT rgiConnPt[] = { ConnPt_iQuotaEvents,
                        ConnPt_iQuotaEventsDisp };

    for (INT i = 0; i < ARRAYSIZE(rgiConnPt) && !bAbort; i++)
    {
        if (NULL != (pConnPt = m_rgConnPts[ rgiConnPt[i] ]))
        {
            PENUMCONNECTIONS pEnum = NULL;

            pConnPt->AddRef();
            hr = pConnPt->EnumConnections(&pEnum);
            if (SUCCEEDED(hr))
            {
                CONNECTDATA cd;

                while(!bAbort && NOERROR == pEnum->Next(1, &cd, NULL))
                {
                    DBGASSERT((NULL != cd.pUnk));

                    LPUNKNOWN pEventSink = NULL;
                    hr = cd.pUnk->QueryInterface(*(m_rgpIConnPtsSupported[ rgiConnPt[i] ]),
                                                 (LPVOID *)&pEventSink);
                    //
                    // Guard with a critical section mutex.  The NT5 quota UI
                    // may deadlock after closing the details view window
                    // without this critical section.  It's possible, other
                    // clients of the quota controller could do the same.  
                    // Here's what happens:
                    //      The controller calls OnUserNameChanged which
                    //      is implemented by the DetailsView object.  This
                    //      function updates the details view for the specified
                    //      quota user.  Update involves sending/posting 
                    //      messages to the listview object.  On destruction
                    //      of the listview window (user closing the window), 
                    //      the quota controller is released.  If the 
                    //      DetailsView held the last ref count to the 
                    //      controller, the controller commands the SID/Name
                    //      resolver to shutdown.  The resolver's Shutdown
                    //      command posts a WM_QUIT to the resolver's input
                    //      queue and blocks until the resolver's thread
                    //      exits normally.  The problem is that the DetailsView
                    //      thread is blocked waiting for the resolver's thread
                    //      to exit but the resolver's thread is blocked 
                    //      because the DetailsView thread can't process it's
                    //      listview update messages.  This results in deadlock.
                    // This critical section prevents this.
                    //
                    if (SUCCEEDED(hr))
                    {
                        if (WAIT_OBJECT_0 != m_mutex.Wait(2000))
                        {
                            //
                            // DiskQuotaControl dtor must own this mutex.  
                            // Since the control is being destroyed, no sense in 
                            // continuing.
                            //
                            DBGERROR((TEXT("Mutex timeout in DiskQuotaControl::NotifyUserNameChanged")));
                            bAbort = true;
                        }
                        else
                        {
                            AutoLockMutex lock(m_mutex); // Exception-safe release.
                            try
                            {
                                //
                                // Calling client code.  Handle any exceptions.
                                //
                                switch(rgiConnPt[i])
                                {
                                    case ConnPt_iQuotaEvents:
                                        hr = ((PDISKQUOTA_EVENTS)pEventSink)->OnUserNameChanged(pUser);
                                        break;

                                    case ConnPt_iQuotaEventsDisp:
                                    {
                                        IDispatch *pEventDisp = NULL;
                                        hr = pEventSink->QueryInterface(IID_IDispatch, (LPVOID *)&pEventDisp);
                                        if (SUCCEEDED(hr))
                                        {
                                            IDispatch *pUserDisp = NULL;
                                            hr = pUser->QueryInterface(IID_IDispatch, (LPVOID *)&pUserDisp);
                                            if (SUCCEEDED(hr))
                                            {
                                                UINT uArgErr;
                                                VARIANTARG va;
                                                DISPPARAMS params;

                                                VariantClear(&va);
                                                V_VT(&va)       = VT_DISPATCH;
                                                V_DISPATCH(&va) = pUserDisp;

                                                params.rgvarg            = &va;
                                                params.rgdispidNamedArgs = NULL;
                                                params.cArgs             = 1;
                                                params.cNamedArgs        = 0;

                                                hr = pEventDisp->Invoke(DISPID_DISKQUOTAEVENTS_USERNAMECHANGED,
                                                                             IID_NULL,
                                                                             GetThreadLocale(),
                                                                             DISPATCH_METHOD,
                                                                             &params,
                                                                             NULL,
                                                                             NULL,
                                                                             &uArgErr);
                                                if (FAILED(hr))
                                                {
                                                    DBGERROR((TEXT("Error 0x%08X firing async notification event with IDispatch::Invoke"), hr));
                                                }
                                                pUserDisp->Release();
                                            }
                                            else
                                            {
                                                DBGERROR((TEXT("Error 0x%08X getting IDispatch interface from user object for async notification."), hr));
                                            }
                                            pEventDisp->Release();
                                        }
                                        else
                                        {
                                            DBGERROR((TEXT("Error 0x%08X getting IDispatch interface from connection point for async notification."), hr));
                                        }
                                        break;
                                    }

                                    default:
                                        //
                                        // Shouldn't hit this.
                                        //
                                        DBGERROR((TEXT("Invalid connection point ID")));
                                        break;
                                }
                            }
                            catch(...)
                            {
                                DBGERROR((TEXT("C++ exception in OnUserNameChanged")));
                            }
                        }
                        pEventSink->Release();
                    }
                    cd.pUnk->Release();
                }
                pEnum->Release();
            }
            pConnPt->Release();
        }
    }
    return hr;
}


DiskQuotaControlDisp::DiskQuotaControlDisp(
    PDISKQUOTA_CONTROL pQC
    ) : m_cRef(0),
        m_pQC(pQC),
        m_pUserEnum(NULL)
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("DiskQuotaControlDisp::DiskQuotaControlDisp")));

    if (NULL != m_pQC)
    {
        m_pQC->AddRef();
    }

    //
    // This is the default resolution style for OLE automation.
    // I've used ASYNC as the default so it won't hang the caller
    // on enumeration if many of the names aren't resolved.
    // If they want sync resolution, they can set the 
    // UserNameResolution property.
    //
    m_fOleAutoNameResolution = DISKQUOTA_USERNAME_RESOLVE_ASYNC;

    m_Dispatch.Initialize(static_cast<IDispatch *>(this),
                          LIBID_DiskQuotaTypeLibrary,
                          IID_DIDiskQuotaControl,
                          L"DSKQUOTA.DLL");
}

DiskQuotaControlDisp::~DiskQuotaControlDisp(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("DiskQuotaControlDisp::~DiskQuotaControlDisp")));

    if (NULL != m_pUserEnum)
    {
        m_pUserEnum->Release();
    }
    if (NULL != m_pQC)
    {
        m_pQC->Release();
    }
}


STDMETHODIMP_(ULONG) 
DiskQuotaControlDisp::AddRef(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_LOW, TEXT("DiskQuotaControlDisp::AddRef")));
    DBGPRINT((DM_CONTROL, DL_LOW, TEXT("\t0x%08X  %d -> %d"),
                     this, m_cRef, m_cRef + 1));

    ULONG ulReturn = m_cRef + 1;
    InterlockedIncrement(&m_cRef);
    return ulReturn;
}


STDMETHODIMP_(ULONG) 
DiskQuotaControlDisp::Release(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_LOW, TEXT("DiskQuotaControlDisp::Release")));
    DBGPRINT((DM_CONTROL, DL_LOW, TEXT("\t0x%08X  %d -> %d"),
                     this, m_cRef, m_cRef - 1));

    ULONG ulReturn = m_cRef - 1;
    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}



STDMETHODIMP 
DiskQuotaControlDisp::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("DiskQuotaControlDisp::QueryInterface")));
    DBGPRINTIID(DM_CONTROL, DL_MID, riid);

    HRESULT hr = E_NOINTERFACE;

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
    else if (IID_DIDiskQuotaControl == riid)
    {
        *ppvOut = static_cast<DIDiskQuotaControl *>(this);
    }
    else if (IID_IDiskQuotaControl == riid ||
             IID_IConnectionPointContainer == riid)
    {
        //
        // Return the quota controller's vtable interface.
        // This allows code to "typecast" (COM-style) between
        // the dispatch interface and vtable interface.
        //
        return m_pQC->QueryInterface(riid, ppvOut);
    }
    if (NULL != *ppvOut)
    {
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hr = NOERROR;
    }

    return hr;
}

//
// IDispatch::GetIDsOfNames
//
STDMETHODIMP
DiskQuotaControlDisp::GetIDsOfNames(
    REFIID riid,  
    OLECHAR **rgszNames,  
    UINT cNames,  
    LCID lcid,  
    DISPID *rgDispId
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::GetIDsOfNames")));
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
DiskQuotaControlDisp::GetTypeInfo(
    UINT iTInfo,  
    LCID lcid,  
    ITypeInfo **ppTypeInfo
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::GetTypeInfo")));
    //
    // Let our dispatch object handle this.
    //
    return m_Dispatch.GetTypeInfo(iTInfo, lcid, ppTypeInfo);
}


//
// IDispatch::GetTypeInfoCount
//
STDMETHODIMP
DiskQuotaControlDisp::GetTypeInfoCount(
    UINT *pctinfo
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::GetTypeInfoCount")));
    //
    // Let our dispatch object handle this.
    //
    return m_Dispatch.GetTypeInfoCount(pctinfo);
}


//
// IDispatch::Invoke
//
STDMETHODIMP
DiskQuotaControlDisp::Invoke(
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
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::Invoke")));
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
// Dispatch property "QuotaState" (put)
//
// Sets the state of the quota system on the volume.
// See DiskQuotaControl::SetQuotaState for details.
//
// Valid states:    0 = Disabled.
//                  1 = Tracking
//                  2 = Enforcing
STDMETHODIMP 
DiskQuotaControlDisp::put_QuotaState(
    QuotaStateConstants State
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::put_QuotaState")));
    if (dqStateMaxValue < State)
    {
        //
        // State can only be 0, 1 or 2.
        //
        return E_INVALIDARG;
    }
    //
    // No exception handling required.
    // DiskQuotaControl::SetQuotaState handles exceptions.
    //
    return m_pQC->SetQuotaState(State);
}

//
// Dispatch property "QuotaState" (get)
//
// Retrieves the state of the quota system on the volume.
// See DiskQuotaControl::GetQuotaState for details.
//
// State returned:  0 = Disabled.
//                  1 = Tracking
//                  2 = Enforcing
//
STDMETHODIMP 
DiskQuotaControlDisp::get_QuotaState(
    QuotaStateConstants *pState
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::get_QuotaState")));
    DWORD dwState;
    //
    // No exception handling required.
    // DiskQuotaControl::GetQuotaState handles exceptions.
    //
    HRESULT hr = m_pQC->GetQuotaState(&dwState);
    if (SUCCEEDED(hr))
    {
        *pState = (QuotaStateConstants)(dwState & DISKQUOTA_STATE_MASK);
    }
    return hr;
}


//
// Dispatch property "QuotaFileIncomplete" (get)
//
// Determines if the state of the quota file is "incomplete".
//
STDMETHODIMP 
DiskQuotaControlDisp::get_QuotaFileIncomplete(
    VARIANT_BOOL *pbIncomplete
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::get_QuotaFileIncomplete")));
    DWORD dwState;
    //
    // No exception handling required.
    // DiskQuotaControl::GetQuotaState handles exceptions.
    //
    HRESULT hr = m_pQC->GetQuotaState(&dwState);
    if (SUCCEEDED(hr))
    {
        *pbIncomplete = DISKQUOTA_FILE_INCOMPLETE(dwState) ? VARIANT_TRUE : VARIANT_FALSE;
    }

    return hr;
}


//
// Dispatch property "QuotaFileRebuilding" (get)
//
// Determines if the state of the quota file is "rebuilding".
//
STDMETHODIMP 
DiskQuotaControlDisp::get_QuotaFileRebuilding(
    VARIANT_BOOL *pbRebuilding
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::get_QuotaFileRebuilding")));
    DWORD dwState;
    //
    // No exception handling required.
    // DiskQuotaControl::GetQuotaState handles exceptions.
    //
    HRESULT hr = m_pQC->GetQuotaState(&dwState);
    if (SUCCEEDED(hr))
    {
        *pbRebuilding = DISKQUOTA_FILE_REBUILDING(dwState) ? VARIANT_TRUE : VARIANT_FALSE;
    }

    return hr;
}


//
// Dispatch property "LogQuotaThreshold" (put)
//
// Sets the "log warning threshold" flag on the volume.
//
STDMETHODIMP 
DiskQuotaControlDisp::put_LogQuotaThreshold(
    VARIANT_BOOL bLog
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::put_LogQuotaThreshold")));
    DWORD dwFlags;
    //
    // No exception handling required.
    // DiskQuotaControl::GetQuotaLogFlags and SetQuotaLogFlags handle 
    // exceptions.
    //
    HRESULT hr = m_pQC->GetQuotaLogFlags(&dwFlags);
    if (SUCCEEDED(hr))
    {
        hr = m_pQC->SetQuotaLogFlags(DISKQUOTA_SET_LOG_USER_THRESHOLD(dwFlags, VARIANT_TRUE == bLog));
    }
    return hr;
}


//
// Dispatch property "LogQuotaThreshold" (get)
//
// Retrieves the "log warning threshold" flag on the volume.
//
STDMETHODIMP 
DiskQuotaControlDisp::get_LogQuotaThreshold(
    VARIANT_BOOL *pbLog
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::get_LogQuotaThreshold")));
    DWORD dwFlags;
    //
    // No exception handling required.
    // DiskQuotaControl::GetQuotaLogFlags handles exceptions.
    //
    HRESULT hr = m_pQC->GetQuotaLogFlags(&dwFlags);
    if (SUCCEEDED(hr))
    {
        *pbLog = DISKQUOTA_IS_LOGGED_USER_THRESHOLD(dwFlags) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return hr;
}


//
// Dispatch property "LogQuotaLimit" (put)
//
// Sets the "log quota limit" flag on the volume.
//
STDMETHODIMP 
DiskQuotaControlDisp::put_LogQuotaLimit(
    VARIANT_BOOL bLog
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::put_LogQuotaLimit")));
    DWORD dwFlags;
    //
    // No exception handling required.
    // DiskQuotaControl::GetQuotaLogFlags handles exceptions.
    //
    HRESULT hr = m_pQC->GetQuotaLogFlags(&dwFlags);
    if (SUCCEEDED(hr))
    {
        hr = m_pQC->SetQuotaLogFlags(DISKQUOTA_SET_LOG_USER_LIMIT(dwFlags, VARIANT_TRUE == bLog));
    }
    return hr;
}


//
// Dispatch property "LogQuotaLimit" (get)
//
// Retrieves the "log quota limit" flag on the volume.
//
STDMETHODIMP 
DiskQuotaControlDisp::get_LogQuotaLimit(
    VARIANT_BOOL *pbLog
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::get_LogQuotaLimit")));
    DWORD dwFlags;
    //
    // No exception handling required.
    // DiskQuotaControl::GetQuotaLogFlags handles exceptions.
    //
    HRESULT hr = m_pQC->GetQuotaLogFlags(&dwFlags);
    if (SUCCEEDED(hr))
    {
        *pbLog = DISKQUOTA_IS_LOGGED_USER_LIMIT(dwFlags) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return hr;
}



//
// Dispatch property "DefaultQuotaThreshold" (put)
//
// Sets the default quota threshold on the volume.
//
STDMETHODIMP 
DiskQuotaControlDisp::put_DefaultQuotaThreshold(
    double Threshold
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::put_DefaultQuotaThreshold")));
 
    if (MAXLONGLONG < Threshold)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
 
    //
    // No exception handling required.
    // DiskQuotaControl::GetDefaultQuotaThreshold handles exceptions.
    //
    return m_pQC->SetDefaultQuotaThreshold((LONGLONG)Threshold);
}


//
// Dispatch property "DefaultQuotaThreshold" (get)
//
// Retrieves the default quota threshold on the volume.
//
STDMETHODIMP 
DiskQuotaControlDisp::get_DefaultQuotaThreshold(
    double *pThreshold
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::get_DefaultQuotaThreshold")));
    LONGLONG llTemp;
    //
    // No exception handling required.
    // DiskQuotaControl::GetDefaultQuotaThreshold handles exceptions.
    //
    HRESULT hr = m_pQC->GetDefaultQuotaThreshold(&llTemp);
    if (SUCCEEDED(hr))
    {
        *pThreshold = (double)llTemp;
    }
    return hr;
}


STDMETHODIMP 
DiskQuotaControlDisp::get_DefaultQuotaThresholdText(
    BSTR *pThresholdText
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::get_DefaultQuotaThresholdText")));
    TCHAR szValue[40];
    HRESULT hr;
    hr = m_pQC->GetDefaultQuotaThresholdText(szValue, ARRAYSIZE(szValue));
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

//
// Dispatch property "DefaultQuotaLimit" (put)
//
// Sets the default quota limit on the volume.
//
STDMETHODIMP 
DiskQuotaControlDisp::put_DefaultQuotaLimit(
    double Limit
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::put_DefaultQuotaLimit")));

    if (MAXLONGLONG < Limit)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    //
    // No exception handling required.
    // DiskQuotaControl::SetDefaultQuotaLimit handles exceptions.
    //
    return m_pQC->SetDefaultQuotaLimit((LONGLONG)Limit);
}


//
// Dispatch property "DefaultQuotaLimit" (get)
//
// Retrieves the default quota limit on the volume.
//
STDMETHODIMP 
DiskQuotaControlDisp::get_DefaultQuotaLimit(
    double *pLimit
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::get_DefaultQuotaLimit")));
    LONGLONG llTemp;
    //
    // No exception handling required.
    // DiskQuotaControl::GetDefaultQuotaLimit handles exceptions.
    //
    HRESULT hr = m_pQC->GetDefaultQuotaLimit(&llTemp);
    if (SUCCEEDED(hr))
    {
        *pLimit = (double)llTemp;
    }
    return hr;
}


STDMETHODIMP 
DiskQuotaControlDisp::get_DefaultQuotaLimitText(
    BSTR *pLimitText
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::get_DefaultQuotaLimitText")));
    TCHAR szValue[40];
    HRESULT hr = m_pQC->GetDefaultQuotaLimitText(szValue, ARRAYSIZE(szValue));
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
DiskQuotaControlDisp::put_UserNameResolution(
    UserNameResolutionConstants ResolutionType
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::put_UserNameResolution")));
    if (dqResolveMaxValue < ResolutionType)
    {
        return E_INVALIDARG;
    }
    m_fOleAutoNameResolution = (DWORD)ResolutionType;    
    return NOERROR;
}


STDMETHODIMP 
DiskQuotaControlDisp::get_UserNameResolution(
    UserNameResolutionConstants *pResolutionType
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::get_UserNameResolution")));
    if (NULL == pResolutionType)
        return E_INVALIDARG;

    *pResolutionType = (UserNameResolutionConstants)m_fOleAutoNameResolution;
    return NOERROR;
}


//
// Dispatch method "Initialize"
//
// Initializes the quota control object for a given path and
// access mode.  See DiskQuotaControl::Initialize for details.
//
STDMETHODIMP 
DiskQuotaControlDisp::Initialize(
    BSTR path, 
    VARIANT_BOOL bReadWrite
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::Initialize")));
    //
    // No exception handling required.
    // DiskQuotaControl::Initialize handles exceptions.
    //
    return m_pQC->Initialize(reinterpret_cast<LPCWSTR>(path), VARIANT_TRUE == bReadWrite);
}

//
// Dispatch method "AddUser"
//
// Adds new user quota record.
// See DiskQuotaControl::AddUserName for details.
//
STDMETHODIMP
DiskQuotaControlDisp::AddUser(
    BSTR LogonName,
    DIDiskQuotaUser **ppUser
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::AddUser")));
    //
    // No exception handling required.
    // DiskQuotaControl::AddUserName handles exceptions.
    //
    PDISKQUOTA_USER pUser = NULL;
    HRESULT hr = m_pQC->AddUserName(reinterpret_cast<LPCWSTR>(LogonName),
                                    m_fOleAutoNameResolution,
                                    &pUser);

    if (SUCCEEDED(hr))
    {
        //
        // Retrieve the user object's IDispatch interface.
        //
        hr = pUser->QueryInterface(IID_IDispatch, (LPVOID *)ppUser);
        pUser->Release();
    }
    return hr;
}



//
// Dispatch method "DeleteUser"
//
// Marks a user quota record for deletion.
// See DiskQuotaControl::DeleteUser for details.
//
STDMETHODIMP 
DiskQuotaControlDisp::DeleteUser(
    DIDiskQuotaUser *pUser
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::DeleteUser")));
    HRESULT hr = E_INVALIDARG;
    if (NULL != pUser)
    {
        //
        // No exception handling required.
        // DiskQuotaControl::DeleteUser handles exceptions.
        //
        PDISKQUOTA_USER pUserToDelete = NULL;
        hr = pUser->QueryInterface(IID_IDiskQuotaUser, (LPVOID *)&pUserToDelete);
        if (SUCCEEDED(hr))
        {
            hr = m_pQC->DeleteUser(pUserToDelete);
            pUserToDelete->Release();
        }
    }
    return hr;
}


//
// Dispatch method "FindUser"
//
// Locates a user quota entry based on the user's name strings.
// Creates a corresponding user object and returns it to the caller.
// See DiskQuotaControl::FindUserName for details.
//
STDMETHODIMP 
DiskQuotaControlDisp::FindUser(
    BSTR LogonName,
    DIDiskQuotaUser **ppUser
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::FindUser")));

    //
    // No exception handling required.
    // DiskQuotaControl::FindUserName handles exceptions.
    //
    HRESULT hr = NOERROR;
    LPCWSTR pszName = reinterpret_cast<LPCWSTR>(LogonName);
    PSID psid = NULL;
    PDISKQUOTA_USER pUser = NULL;
    if (ConvertStringSidToSid(pszName, &psid))
    {
        hr = m_pQC->FindUserSid(psid,
                                m_fOleAutoNameResolution,
                                &pUser);
        LocalFree(psid);
        psid = NULL;
    }
    else
    {
        hr = m_pQC->FindUserName(pszName, &pUser);
    }
    
    if (SUCCEEDED(hr))
    {
        DBGASSERT((NULL != pUser));
        //
        // Query for the user's IDispatch interface and release the pointer
        // we received from FindUserName.
        //
        hr = pUser->QueryInterface(IID_IDispatch, (LPVOID *)ppUser);
        pUser->Release();
    }
    return hr;
}

//
// Dispatch method "InvalidateSidNameCache"
//
// Invalidates the SID/Name cache.
// See DiskQuotaControl::InvalidateSidNameCache for details.
//
STDMETHODIMP 
DiskQuotaControlDisp::InvalidateSidNameCache(
    void
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::InvalidateSidNameCache")));
    //
    // No exception handling required.
    // DiskQuotaControl::InvalidateSidNameCache handles exceptions.
    //
    return m_pQC->InvalidateSidNameCache();
}

        
//
// Dispatch method "GiveUserNameResolutionPriority"
//
// Promotes a user object to the front of the SID/Name resolver's input queue.
// See DiskQuotaControl::GiveUserNameResolutionPriority.
//
STDMETHODIMP 
DiskQuotaControlDisp::GiveUserNameResolutionPriority(
    DIDiskQuotaUser *pUser
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::GiveUserNameResolutionPriority")));

    HRESULT hr = E_INVALIDARG;
    if (NULL != pUser)
    {
        //
        // No exception handling required.
        // DiskQuotaControl::GiveUserNameResolutionPriority handles exceptions.
        //
        PDISKQUOTA_USER pUserToPromote = NULL;
        hr = pUser->QueryInterface(IID_IDiskQuotaUser, (LPVOID *)&pUserToPromote);
        if (SUCCEEDED(hr))
        {
            hr = m_pQC->GiveUserNameResolutionPriority(pUserToPromote);
            pUserToPromote->Release();
        }
    }
    return hr;
}


//
// This function is called by an automation controller when a new enumerator is
// required.  In particular, Visual Basic calls it when it encounters a
// "for each" loop.  The name "_NewEnum" is hard-wired and can't change.
//
STDMETHODIMP
DiskQuotaControlDisp::_NewEnum(
    IDispatch **ppEnum
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::_NewEnum")));
    HRESULT hr = E_INVALIDARG;

    if (NULL != ppEnum)
    {
        try
        {
            //
            // Create a Collection object using the current settings for
            // name resolution.
            //
            DiskQuotaUserCollection *pCollection = new DiskQuotaUserCollection(m_pQC,
                                                                               m_fOleAutoNameResolution);
            hr = pCollection->Initialize();
            if (SUCCEEDED(hr))
            {
                //
                // The caller of _NewEnum (probably VB) wants the IEnumVARIANT
                // interface.
                //
                hr = pCollection->QueryInterface(IID_IEnumVARIANT, (LPVOID *)ppEnum);
            }
        }
        catch(CAllocException& e)
        {
            DBGERROR((TEXT("Insufficient memory exception")));
            hr = E_OUTOFMEMORY;
        }
        catch(...)
        {
            DBGERROR((TEXT("Unexpected C++ exception")));
            hr = E_UNEXPECTED;
        }
    }
    return hr;
}

//
// Shutdown the SID/Name resolver.  Note that this happens automatically
// when the control object is destroyed.
//
STDMETHODIMP
DiskQuotaControlDisp::ShutdownNameResolution(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlDisp::ShutdownNameResolution")));
    return m_pQC->ShutdownNameResolution();
}


//
// Given a logon name in SAM-compatible or UPN format, translate
// the name to the corresponding account's SID.  The returned SID is 
// formatted as a string.
//
STDMETHODIMP 
DiskQuotaControlDisp::TranslateLogonNameToSID(
    BSTR LogonName,
    BSTR *psid
    )
{
    NTDS ntds;
    BYTE sid[MAX_SID_LEN];
    SID_NAME_USE eSidUse;
    DWORD cbSid = ARRAYSIZE(sid);
    
    HRESULT hr = ntds.LookupAccountByName(NULL,
                                          reinterpret_cast<LPCWSTR>(LogonName),
                                          NULL,
                                          NULL,
                                          sid,
                                          &cbSid,
                                          &eSidUse);

    if (SUCCEEDED(hr))
    {
        LPTSTR pszSid = NULL;
        if (ConvertSidToStringSid((PSID)sid, &pszSid))
        {
            *psid = SysAllocString(pszSid);
            if (NULL == *psid)
            {
                hr = E_OUTOFMEMORY;
            }
            LocalFree(pszSid);
            pszSid = NULL;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return hr;
}


