///////////////////////////////////////////////////////////////////////////////
/*  File: sidname.cpp

    Description: Implements the SID-to-NAME resolver.  It is anticipated that
        resolving a user's SID to it's corresponding name can be a lengthy
        process.  We don't want the quota controller's client to experience
        slow user enumerations just because it takes a long time to resolve
        a name.  Therefore, this object was created to perform the 
        SID-to-name resolutions on a background thread and notify the
        client whenever a name has been resolved.  That way, the client
        can display a list of users then fill in the names as names
        are resolved.  


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/12/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h" // PCH
#pragma hdrstop

#include "control.h"
#include "guidsp.h"    // Private GUIDs.
#include "registry.h"
#include "sidname.h"
#include "sidcache.h"

//
// Verify that build is UNICODE.
//
#if !defined(UNICODE)
#   error This module must be compiled UNICODE.
#endif


//
// SID/Name resolver messages (SNRM_XXXXXXXX).
//
#define SNRM_CLEAR_INPUT_QUEUE   (WM_USER + 1)
#define SNRM_EXIT_THREAD         (WM_USER + 2)

///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::SidNameResolver

    Description: SidNameResolver constructor.

    Arguments:
        rQuotaController - Reference to quota controller that this resolver is
            working for.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/12/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
SidNameResolver::SidNameResolver(
    DiskQuotaControl& rQuotaController)
    : m_cRef(0),
      m_rQuotaController(rQuotaController),
      m_hsemQueueNotEmpty(NULL),
      m_hMutex(NULL),
      m_dwResolverThreadId(0),
      m_hResolverThread(NULL),
      m_bCacheCreationFailed(FALSE),
      m_heventResolverThreadReady(NULL)
{
    DBGTRACE((DM_RESOLVER, DL_MID, TEXT("SidNameResolver::SidNameResolver")));
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::~SidNameResolver

    Description: SidNameResolver destructor.

    Arguments: None.

    Returns: Nothing.
 
    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/12/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
SidNameResolver::~SidNameResolver(void)
{
    DBGTRACE((DM_RESOLVER, DL_MID, TEXT("SidNameResolver::~SidNameResolver")));

    if (NULL != m_hsemQueueNotEmpty)
        CloseHandle(m_hsemQueueNotEmpty);
    if (NULL != m_hMutex)
        CloseHandle(m_hMutex);
    if (NULL != m_hResolverThread)
        CloseHandle(m_hResolverThread);
    if (NULL != m_heventResolverThreadReady)
        CloseHandle(m_heventResolverThreadReady);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::QueryInterface

    Description: Returns an interface pointer to the object's IUnknown or
        ISidNameResolver interface.  
        Only IID_IUnknown, IID_ISidNameResolver are recognized.  
        The object referenced by the returned interface pointer is uninitialized.  
        The recipient of the pointer must call Initialize() before the object 
        is usable.

    Arguments:
        riid - Reference to requested interface ID.

        ppvOut - Address of interface pointer variable to accept interface ptr.

    Returns:
        NO_ERROR        - Success.
        E_NOINTERFACE   - Requested interface not supported.
        E_INVALIDARG    - ppvOut argument was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/07/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
SidNameResolver::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_RESOLVER, DL_MID, TEXT("SidNameResolver::QueryInterface")));
    DBGPRINTIID(DM_RESOLVER, DL_MID, riid);

    HRESULT hResult = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IID_IUnknown == riid || IID_ISidNameResolver == riid)
    {
        *ppvOut = this;
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hResult = NOERROR;
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::AddRef

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
SidNameResolver::AddRef(
    VOID
    )
{
    DBGTRACE((DM_RESOLVER, DL_LOW, TEXT("SidNameResolver::AddRef")));
    DBGPRINT((DM_RESOLVER, DL_LOW, TEXT("\t0x%08X %d -> %d"),
             this, m_cRef, m_cRef + 1));

    ULONG ulReturn = m_cRef + 1;
    InterlockedIncrement(&m_cRef);
    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::Release

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
SidNameResolver::Release(
    VOID
    )
{
    DBGTRACE((DM_RESOLVER, DL_LOW, TEXT("SidNameResolver::Release")));
    DBGPRINT((DM_RESOLVER, DL_LOW, TEXT("\t0x%08X %d -> %d"),
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
/*  Function: SidNameResolver::Initialize

    Description: Initializes a SidNameResolver object.
        This function performs lot's of initialization steps so I chose to 
        use the "jump to label on failure" approach.  It avoids a lot of
        deeply nested "ifs".

    Arguments: None.

    Returns:
        NO_ERROR        - Success.
        E_FAIL          - Initialization failed.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/11/96    Initial creation.                                    BrianAu
    08/14/96    Moved SID/Name cache initialization to               BrianAu
                FindCachedUserName() method.  Only initialize cache
                when it is truly needed.
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
SidNameResolver::Initialize(
    VOID
    ) 
{
    DBGTRACE((DM_RESOLVER, DL_HIGH, TEXT("SidNameResolver::Initialize")));
    HRESULT hResult  = NO_ERROR;
    DWORD dwThreadId = 0;

    //
    // Configure the user queue so it grows in chunks of 100.
    //
    m_UserQueue.SetSize(100);
    m_UserQueue.SetGrow(100);

    //
    // IMPORTANT:  There is code in the QuotaControl object that
    //             counts on the thread being created LAST in this function.
    //             See DiskQuotaControl::CreateEnumUsers.  
    //             Thread creation must be the last thing performed in this
    //             function.  The caller assumes that if this function returns
    //             E_FAIL, no thread was created.
    //
    m_hMutex = CreateMutex(NULL, FALSE, NULL);
    if (NULL == m_hMutex)
        goto InitFailed;

    m_hsemQueueNotEmpty = CreateSemaphore(NULL,        // No security.
                                          0,           // Initially empty queue.
                                          0x7FFFFFFF,  // Max count (a lot).
                                          NULL);       // No name.
    if (NULL == m_hsemQueueNotEmpty)
        goto InitFailed;

    m_heventResolverThreadReady = CreateEvent(NULL,   // No security.
                                              TRUE,   // Manual reset.
                                              FALSE,  // Initially non-signaled.
                                              NULL);  // No name.
    if (NULL == m_heventResolverThreadReady)
        goto InitFailed;


    hResult = CreateResolverThread(&m_hResolverThread, &m_dwResolverThreadId);
    DBGPRINT((DM_RESOLVER, DL_MID, TEXT("Resolver thread. hThread = 0x%08X, idThread = %d"),
             m_hResolverThread, m_dwResolverThreadId));

    if (FAILED(hResult))
        goto InitFailed;
InitFailed:

    //
    // Failure only returns E_FAIL.
    //
    if (FAILED(hResult))
        hResult = E_FAIL;

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::Shutdown

    Description: Commands the resolver to terminate its activities.
        When the resolver's client is through with the resolver's services,
        it should call Shutdown() followed by IUnknown::Release().  The function
        sends a WM_QUIT message to the resolver thread.

    Arguments: None.

    Returns:
        NO_ERROR    - Success
        E_FAIL      - Failed to send WM_QUIT message.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/29/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
SidNameResolver::Shutdown(
    BOOL bWait
    )
{
    DBGTRACE((DM_RESOLVER, DL_HIGH, TEXT("SidNameResolver::Shutdown")));
    DBGPRINT((DM_RESOLVER, DL_HIGH, TEXT("\tThread ID = %d"), m_dwResolverThreadId));

    BOOL bResult = FALSE;
    if (0 != m_dwResolverThreadId)
    {
        bResult = PostThreadMessage(m_dwResolverThreadId, WM_QUIT, 0, 0);
        if (bResult && bWait && NULL != m_hResolverThread)
        {
            //
            // Wait for thread to terminate normally.
            //
            DBGPRINT((DM_RESOLVER, DL_HIGH, TEXT("Resolver waiting for thread to exit...")));
            WaitForSingleObject(m_hResolverThread, INFINITE);
        }
    }

    return bResult ? NO_ERROR : E_FAIL;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::GetUserSid

    Description: Private method that allocates a SID buffer and retrieves
        the user's SID into that buffer.  The caller must free the returned
        buffer when done with it.

    Arguments:
        pUser - Pointer to user's IDiskQuotaUser interface.

        ppSid - Address of a PSID variable to receive the address of the
            allocated SID buffer.

    Returns:
        NO_ERROR        - Success.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/08/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
SidNameResolver::GetUserSid(
    PDISKQUOTA_USER pUser, 
    PSID *ppSid
    )
{
    HRESULT hResult   = NO_ERROR;
    DWORD cbSid = 0;

    DBGASSERT((NULL != pUser));
    hResult = pUser->GetSidLength(&cbSid);
    if (SUCCEEDED(hResult))
    {
        PSID pUserSid = NULL;

        pUserSid = (PSID) new BYTE[cbSid];

        hResult = pUser->GetSid((PBYTE)pUserSid, cbSid);
        if (SUCCEEDED(hResult))
        {
            DBGASSERT((IsValidSid(pUserSid)));
            DBGASSERT((NULL != ppSid));
            *ppSid = pUserSid; // Return address of buffer to caller.
                               // Caller must free it when done.
        }
        else
        {
            DBGERROR((TEXT("RESOLVER - GetSid failed.")));
            delete[] pUserSid; // Failed to get SID.  Free buffer.
        }
    }
    else
    {
        DBGERROR((TEXT("RESOLVER - GetUserSid failed.")));
    }
    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::AddUserToResolverQueue

    Description: Adds a user pointer to the resolver's input queue.

    Arguments:
        pUser - Address of IDiskQuotaUser ptr.

    Returns:
        NO_ERROR        - Success.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/09/96    Initial creation.                                    BrianAu
    09/03/96    Added exception handling.                            BrianAu
    12/10/96    Removed interface marshaling. Using free-threading   BrianAu
                model.
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
SidNameResolver::AddUserToResolverQueue(
    PDISKQUOTA_USER pUser
    )
{
    DBGTRACE((DM_RESOLVER, DL_MID, TEXT("SidNameResolver::AddUserToResolverQueue")));
    DBGASSERT((NULL != pUser));
    HRESULT hResult = NO_ERROR;

    //
    // Add user object pointer to resolver input queue.
    // This can throw OutOfMemory.
    //
    pUser->AddRef();
    try
    {
        m_UserQueue.Add(pUser);
    }
    catch(...)
    {
        pUser->Release();
    }

    //
    // Increment queue's semaphore count.
    // Means there's something in queue to process.
    //
    ReleaseSemaphore(m_hsemQueueNotEmpty, 1, NULL);

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::RemoveUserFromResolverQueue

    Description: Removes a user pointer from the resolver's input queue.

    Arguments:
        ppUser - Address of pointer variable to receive IDiskQuotaUser ptr.

    Returns:
        NO_ERROR     - Success.
        E_UNEXPECTED - Resolver queue was empty.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/09/96    Initial creation.                                    BrianAu
    12/10/96    Removed interface marshaling. Using free-threading   BrianAu
                model.
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
SidNameResolver::RemoveUserFromResolverQueue(
    PDISKQUOTA_USER *ppUser
    )
{
    DBGTRACE((DM_RESOLVER, DL_MID, TEXT("SidNameResolver::RemoveUserFromResolverQueue")));
    DBGASSERT((NULL != ppUser));
    HRESULT hResult = E_UNEXPECTED;

    *ppUser = NULL;

    if (!m_UserQueue.IsEmpty() && 
        m_UserQueue.Remove(*ppUser))
    {
        hResult = NO_ERROR;
    }
    else
    {
        DBGERROR((TEXT("RESOLVER - Input queue unexpectedly empty.")));
    }


    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::PromoteUserToQueueHead

    Description: Promotes a user object to the head of the resolver queue
                 if the user object is in the queue.  This can be used to
                 give specific user objects higher priority if desired.
                 In particular, the initial requirement behind this feature
                 is so that user objects highlighted in the details list view
                 get higher name-resolution priority so that the user 
                 (app user) feels the UI is responsive to their inputs.

    Arguments:
        pUser - Address of IDiskQuotaUser interface for user object.

    Returns:
        NO_ERROR      - Success.
        S_FALSE       - User record not in queue.
        E_OUTOFMEMORY - Insufficient memory adding item.
        E_UNEXPECTED  - Exception caught.  User record not promoted.
        E_INVALIDARG  - pUser argument was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/18/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
SidNameResolver::PromoteUserToQueueHead(
    PDISKQUOTA_USER pUser
    )
{
    DBGTRACE((DM_RESOLVER, DL_MID, TEXT("SidNameResolver::PromoteUserToQueueHead")));
    HRESULT hResult = S_FALSE;

    if (NULL == pUser)
        return E_INVALIDARG;

    m_UserQueue.Lock();
    try
    {
        //
        // Find the user in the resolver's queue.
        //
        INT iUser = m_UserQueue.Find(pUser);
        if (-1 != iUser)
        {
            //
            // Note we don't mess with the ref count of the
            // user object.  We're merely deleting a user and re
            // inserting it into the queue.  The queue's original
            // AddRef() is retained.
            //
            m_UserQueue.Delete(iUser);
            m_UserQueue.Add(pUser);
        }
    }
    catch(CAllocException& e)
    {
        hResult = E_OUTOFMEMORY;
    }
    catch(...)
    {
        //
        // Do nothing.  Catch exceptions so we release user list lock.
        // This promotion operation isn't critical enough to throw
        // the exception up the call chain.  Just silently fail
        // to promote the user object to the head of the queue.  
        // The return value will notify callers who are interested.
        // There is a chance that if the Insert() function throws an
        // exception, the user record will be REMOVED from the list
        // instead of promoted to the head.  Oh well...  If we're to the
        // point of throwing exceptions (i.e. OutOfMemory), SID-Name
        // resolution for quota records will not be the major concern
        // for the quota UI user.  
        //
        hResult = E_UNEXPECTED;
    }
    m_UserQueue.ReleaseLock();

    return hResult;            
}

 

///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::ResolveSidToName

    Description: Finds the name corresponding to a user's SID.
        Once the name is found, it is sent to the user object for storage.

    Arguments:
        pUser - Pointer to user objects's IDiskQuotaUser interface.

    Returns:
        NO_ERROR        - Success.
        E_FAIL          - Couldn't resolve SID to a name.
        ERROR_NONE_MAPPED (hr) - No SID-to-Name mapping available.
            No need to try again.


    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/11/96    Initial creation.                                    BrianAu
    08/09/96    Set hResult to E_FAIL when LookupAccountSid fails.   BrianAu
    09/05/96    Added user domain name string.                       BrianAu
    05/18/97    Changed to report deleted SIDs.                      BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
SidNameResolver::ResolveSidToName(
    PDISKQUOTA_USER pUser
    )
{
    DBGTRACE((DM_RESOLVER, DL_HIGH, TEXT("SidNameResolver::ResolveSidToName")));
    HRESULT hResult     = NO_ERROR;
    array_autoptr<BYTE> ptrUserSid;

    DBGASSERT((NULL != pUser));
    
    hResult = GetUserSid(pUser, (PSID *)(ptrUserSid.getaddr()));
    if (SUCCEEDED(hResult))
    {
        SID_NAME_USE SidNameUse = SidTypeUnknown;
        CString strContainer;
        CString strLogonName;
        CString strDisplayName;

        DBGPRINT((DM_RESOLVER, DL_MID, TEXT("RESOLVER - Calling LookupAccountBySid.")));

        hResult = m_rQuotaController.m_NTDS.LookupAccountBySid(
                                                         NULL,
                                                         (PSID)(ptrUserSid.get()),
                                                         &strContainer,
                                                         &strLogonName,
                                                         &strDisplayName,
                                                         &SidNameUse);
        if (SUCCEEDED(hResult))
        {                                         
            switch(SidNameUse)
            {
                case SidTypeDeletedAccount:
                    static_cast<DiskQuotaUser *>(pUser)->SetAccountStatus(DISKQUOTA_USER_ACCOUNT_DELETED);
                    break;

                case SidTypeInvalid:
                    static_cast<DiskQuotaUser *>(pUser)->SetAccountStatus(DISKQUOTA_USER_ACCOUNT_INVALID);
                    break;

                case SidTypeUnknown:
                    static_cast<DiskQuotaUser *>(pUser)->SetAccountStatus(DISKQUOTA_USER_ACCOUNT_UNKNOWN);
                    break;

                default:
                    //
                    // Valid account.
                    //
                    if (NULL != g_pSidCache) // Cache creation may have failed.
                    {
                        //
                        // Add SID/Name pair to cache.  
                        // Indicate failure only with a debug msg.  If cache
                        // addition fails, we'll still work OK, just slower.
                        // This can throw OutOfMemory.
                        //
                        HRESULT hr = g_pSidCache->Add((PSID)(ptrUserSid.get()), 
                                                      strContainer, 
                                                      strLogonName, 
                                                      strDisplayName);
                        if (FAILED(hr))
                        {
                            DBGERROR((TEXT("SIDNAME - Addition of %s\\%s failed."), strLogonName.Cstr()));
                        }
                    }

                    //
                    // Set the user object's account name strings.
                    //
                    hResult = static_cast<DiskQuotaUser *>(pUser)->SetName(
                                                                        strContainer,
                                                                        strLogonName, 
                                                                        strDisplayName);
                    if (SUCCEEDED(hResult))
                    {
                        static_cast<DiskQuotaUser *>(pUser)->SetAccountStatus(DISKQUOTA_USER_ACCOUNT_RESOLVED);
                    }
                    else
                    {
                        DBGERROR((TEXT("SIDNAME - SetName failed in ResolveSidToName. hResult = 0x%08X"),
                                 hResult));
                    }

                    break;
            }
        }
        else
        {
            //
            // Failed asynch name resolution.
            //
            static_cast<DiskQuotaUser *>(pUser)->SetAccountStatus(DISKQUOTA_USER_ACCOUNT_UNAVAILABLE);
            if (ERROR_NONE_MAPPED == GetLastError())
                hResult = HRESULT_FROM_WIN32(ERROR_NONE_MAPPED);
            else
                hResult = E_FAIL; 
        }
    }
    else
    {
        DBGERROR((TEXT("SIDNAME - GetUserSid failed in ResolveSidToName, hResult = 0x%08X"),
                 hResult));
    }

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::FindCachedUserName

    Description: Accepts a user object's IDiskQuotaUser interface pointer and
        looks for it's SID/Name pair in the SID/Name cache.  If found, the 
        name is set in the user object and the function returns NO_ERROR.

    Arguments:
        pUser - Pointer to user objects's IDiskQuotaUser interface.

    Returns:
        NO_ERROR             - Success.  User's SID found in cache.
        ERROR_FILE_NOT_FOUND (hr) - User's SID not in cache.

    Exceptions: OutOfMemory

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/27/96    Initial creation.                                    BrianAu
    08/14/96    Moved initialization of SID/Name cache from          BrianAu
                SidNameResolver::Initialize().
    09/05/96    Added user domain name string.                       BrianAu
    09/21/96    New cache design.                                    BrianAu
    03/18/98    Replaced "domain", "name" and "full name" with       BrianAu
                "container", "logon name" and "display name" to
                better match the actual contents.  This was in 
                reponse to making the quota UI DS-aware.  The 
                "logon name" is now a unique key as it contains
                both account name and domain-like information.
                i.e. "REDMOND\brianau" or "brianau@microsoft.com".
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
SidNameResolver::FindCachedUserName(
    PDISKQUOTA_USER pUser
    )
{
    DBGTRACE((DM_RESOLVER, DL_MID, TEXT("SidNameResolver::FindCachedUserName")));
    HRESULT hResult = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND); // Assume not found.

    //
    // No use continuing if we've failed to create a cache object.
    //
    if (!m_bCacheCreationFailed)
    {
        PSID pUserSid = NULL;

        hResult = GetUserSid(pUser, &pUserSid);
        if (SUCCEEDED(hResult))
        {
            LPTSTR pszContainer   = NULL;
            LPTSTR pszLogonName   = NULL;
            LPTSTR pszDisplayName = NULL;

            try
            {
                //
                // Can throw OutOfMemory.
                //
                if (NULL == g_pSidCache)
                {
                    hResult = SidNameCache::CreateNewCache(&g_pSidCache);
                }

                if (NULL != g_pSidCache)
                {
                    DBGASSERT((SUCCEEDED(hResult)));

                    //
                    // Check cache for SID/Name pair.
                    // This can throw OutOfMemory.
                    //
                    DBGPRINT((DM_RESOLVER, DL_MID, TEXT("RESOLVER - Query cache for user 0x%08X."), pUser));
                    hResult = g_pSidCache->Lookup(pUserSid, 
                                           &pszContainer,
                                           &pszLogonName,
                                           &pszDisplayName);

                    if (SUCCEEDED(hResult))
                    {
                        //
                        // Name was cached.  Set it in the user object and return NO_ERROR.
                        //
                        hResult = static_cast<DiskQuotaUser *>(pUser)->SetName(
                                                                            pszContainer, 
                                                                            pszLogonName, 
                                                                            pszDisplayName);
                        if (SUCCEEDED(hResult))
                        {
                            static_cast<DiskQuotaUser *>(pUser)->SetAccountStatus(DISKQUOTA_USER_ACCOUNT_RESOLVED);
                        }
                    }
                }
                else
                {
                    //
                    // Remember that we tried but failed to create a cache.
                    //
                    m_bCacheCreationFailed = TRUE;
                    //
                    // Set the return value so the caller knows to resolve
                    // the user name.
                    //
                    hResult = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                    DBGERROR((TEXT("RESOLVER - SID/Name cache not available.")));
                }
            }
            catch(...)
            {
                delete[] pszContainer;
                delete[] pszLogonName;
                delete[] pszDisplayName;
                delete[] pUserSid;
                throw;
            }

            delete[] pszContainer;
            delete[] pszLogonName;
            delete[] pszDisplayName;
            delete[] pUserSid;  
        }
    }
    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::FindUserName

    Description: Accepts a user object's IDiskQuotaUser interface pointer and
        looks for it's SID/Name pair in the SID/Name cache.  If the information
        is not cached, the function calls ResolveSidToName to synchronously
        determine the SID's account name.  The function blocks until the name
        is retrieved.

    Arguments:
        pUser - Pointer to user objects's IDiskQuotaUser interface.

    Returns:
        NO_ERROR             - Success.
        E_FAIL               - Couldn't resolve SID to name.
        ERROR_NONE_MAPPED (hr) - No SID-to-Name mapping found.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/27/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
SidNameResolver::FindUserName(
    PDISKQUOTA_USER pUser
    )
{
    DBGTRACE((DM_RESOLVER, DL_MID, TEXT("SidNameResolver::FindUserName")));
    HRESULT hResult = NO_ERROR;
    
    DBGASSERT((NULL != pUser));
    hResult = FindCachedUserName(pUser); // Can throw OutOfMemory.
    if (ERROR_FILE_NOT_FOUND == HRESULT_CODE(hResult))
    {
        DBGPRINT((DM_RESOLVER, DL_MID, TEXT("RESOLVER - User 0x%08X not cached.  Resolving..."),
                 pUser));
        hResult = ResolveSidToName(pUser); // Can throw OutOfMemory.
    }
    else
    {
        DBGPRINT((DM_RESOLVER, DL_MID, TEXT("RESOLVER - User 0x%08X found in cache."), pUser));
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::FindUserNameAsync

    Description: Accepts a user object's IDiskQuotaUser interface pointer and
        looks for it's SID/Name pair in the SID/Name cache.  If the information
        is not cached, the user object is submitted to the resolver for 
        background processing and asynchronous client notification when the 
        operation is complete.

    Arguments:
        pUser - Pointer to user objects's IDiskQuotaUser interface.

    Returns:
        NO_ERROR        - Success.
        E_FAIL          - Resolver thread not active.  Can't resolve Async.
        S_FALSE         - User name not in cache.  Submitted for background
                          processing.  Client will be notified when name is
                          found and set in user object.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/11/96    Initial creation.                                    BrianAu
    06/25/96    Added SID/Name caching.                              BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
SidNameResolver::FindUserNameAsync(
    PDISKQUOTA_USER pUser
    )
{
    DBGTRACE((DM_RESOLVER, DL_MID, TEXT("SidNameResolver::FindUserNameAsync")));
    HRESULT hResult = NO_ERROR;
    
    DBGASSERT((NULL != pUser));

    hResult = FindCachedUserName(pUser);

    if (ERROR_FILE_NOT_FOUND == HRESULT_CODE(hResult))
    {
        if (0 != m_dwResolverThreadId)
        {
            DBGPRINT((DM_RESOLVER, DL_MID, 
                     TEXT("RESOLVER - User 0x%08X not cached.  Submitting to Resolver."),
                     pUser));

            //
            // Name was not cached.  Add the user object to the resolver's input queue
            // so that the name can be located by the resolver's background thread.
            //
            hResult = AddUserToResolverQueue(pUser);
        }
        else
        {
            DBGERROR((TEXT("RESOLVER - Thread not active.  Can't resolve user 0x%08X async."),
                     pUser));
            hResult = E_FAIL;
        }
    }
    else
    {
        DBGPRINT((DM_RESOLVER, DL_MID, TEXT("RESOLVER - User 0x%08X found in cache."),
                 pUser));
    }

    return hResult;
}

    

///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::ClearInputQueue.

    Description: Called by a SidNameResolver thread after the thread receives
        a WM_QUIT message.  This function removes all user object pointers
        from the input queue before the thread exists.

    Arguments: None.

    Returns:
        NO_ERROR    - Always returns NO_ERROR.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/96    Initial creation.                                    BrianAu
    12/10/96    Moved Semaphore reduction from method                BrianAu
                HandleShutdownMessages then deleted that method.
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
SidNameResolver::ClearInputQueue(
    void
    )
{
    DBGTRACE((DM_RESOLVER, DL_HIGH, TEXT("SidNameResolver::ClearInputQueue")));
    PDISKQUOTA_USER pUser = NULL;

    //
    // Decrement the queue-not-empty semaphore to 0 so that the thread
    // doesn't try to remove any more queue entries.
    // Set the resolver's thread ID to 0 so that FindNameAsync will not
    // submit any more users to the resolver.
    //
    m_dwResolverThreadId = 0;
    while(WAIT_OBJECT_0 == WaitForSingleObject(m_hsemQueueNotEmpty, 0))
        NULL;
    //
    // Remove all remaining items from input queue
    // Remove will return E_FAIL if list is empty.
    //
    m_UserQueue.Lock();
    while(m_UserQueue.Count() > 0)
    {
        HRESULT hResult = RemoveUserFromResolverQueue(&pUser);
        if (SUCCEEDED(hResult) && NULL != pUser)
        {
            pUser->Release(); // Release user object.
        }
    }
    m_UserQueue.ReleaseLock();

    return NO_ERROR;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::ThreadOnQueueNotEmpty

    Description: Called by a SidNameResolver thread when the resolver's input
        queue is not empty.  This function removes the next entry
        from the queue, resolves the user's SID to its name, sets the name
        in the user object and notifies the client of the name change.

    Arguments: None.

    Returns:
        NO_ERROR    - Always returns NO_ERROR.

    Exceptions: OutOfMemory

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
SidNameResolver::ThreadOnQueueNotEmpty(
    void
    )
{
    DBGTRACE((DM_RESOLVER, DL_LOW, TEXT("SidNameResolver::ThreadOnQueueNotEmpty")));
    HRESULT hResult       = NO_ERROR;
    PDISKQUOTA_USER pUser = NULL;
    LPSTREAM pstm         = NULL;

    //
    // Remove item from queue
    // RemoveFirst() will return E_FAIL if list is empty.
    //
    try
    {
        hResult = RemoveUserFromResolverQueue(&pUser);
        if (SUCCEEDED(hResult) && NULL != pUser)
        {
            ResolveSidToName(pUser);

            //
            // If successful or not, notify client event sink.
            // Even if we couldn't resolve the name, the object's account
            // status has changed.  The client will want to respond to this.
            // 
            // Don't bother with return value.  We don't care if it fails.
            // The client will but we don't.
            //
            m_rQuotaController.NotifyUserNameChanged(pUser);
            pUser->Release(); // Release pointer obtained from resolver queue.
        }
        else
        {
            DBGERROR((TEXT("RESOLVER - Error removing stream ptr from input queue.")));
        }
    }
    catch(...)
    {
        if (NULL != pUser)
            pUser->Release();
        throw;
    }
    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::ThreadProc

    Description: This thread procedure sits in a loop handling events and
        thread messages.

        Conditions that cause the thread to exit.

        1. OleInitalize() fails.
        2. Thread receives a WM_QUIT message.
        3. Wait function failure or timeout.

    Arguments:
        dwParam - "this" pointer for the SidNameResolver object.
            Required since ThreadProc must be static to be a THREADPROC.

    Returns:

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/11/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DWORD 
SidNameResolver::ThreadProc(
    DWORD dwParam
    )
{
    DBGTRACE((DM_RESOLVER, DL_HIGH, TEXT("SidNameResolver::ThreadProc")));
    DBGPRINT((DM_RESOLVER, DL_HIGH, TEXT("\tThreadID = %d"), GetCurrentThreadId()));
    SidNameResolver *pThis = (SidNameResolver *)dwParam;
    BOOL bExitThread       = FALSE;

    //
    // Make sure DLL stays loaded while this thread is active.
    //
    InterlockedIncrement(&g_cRefThisDll);

    //
    // Must call CoInitializeEx() for each thread.
    //
    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        BOOL bReadyToReceiveMsgs = FALSE;

        DBGPRINT((DM_RESOLVER, DL_MID, TEXT("RESOLVER: Thread %d entering msg loop."), GetCurrentThreadId()));
        while(!bExitThread)
        {
            MSG msg;
            try
            {
                //
                // Allow blocked thread to respond to sent messages.
                //
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && !bExitThread)
                {
                    if ( WM_QUIT != msg.message )
                    {
                        DBGPRINT((DM_RESOLVER, DL_MID, TEXT("RESOLVER: Thread %d dispatching msg %d"),
                                 GetCurrentThreadId(), msg.message));
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                    else
                    {
                        //
                        // Rcvd WM_QUIT.  Clear the resolver's input queue and
                        // exit the msg loop.
                        //
                        DBGPRINT((DM_RESOLVER, DL_MID, TEXT("RESOLVER: Thread %d received WM_QUIT"),
                                 GetCurrentThreadId()));
                        pThis->ClearInputQueue();
                        bExitThread = TRUE;
                    }
                }

                if (!bExitThread)
                {
                    DWORD dwWaitResult = 0;

                    if (!bReadyToReceiveMsgs)
                    {
                        //
                        // Tell the thread creation function that it can
                        // now return.  The thread is ready to accept messages.
                        //
                        SetEvent(pThis->m_heventResolverThreadReady);
                        bReadyToReceiveMsgs = TRUE;
                    }

                    //
                    // Sleep if the queue is empty.
                    // Wake up on queue-not-empty or any thread messages.
                    //
                    DBGPRINT((DM_RESOLVER, DL_MID, TEXT("RESOLVER - Thread %d waiting for messages..."),
                              GetCurrentThreadId()));
                    dwWaitResult = MsgWaitForMultipleObjects(
                                           1,
                                           &(pThis->m_hsemQueueNotEmpty),
                                           FALSE,
                                           INFINITE,
                                           QS_ALLINPUT);

                    switch(dwWaitResult)
                    {
                        case WAIT_OBJECT_0:
                            //
                            // Have data in input queue. Process one user.
                            //
                            DBGPRINT((DM_RESOLVER, DL_MID, TEXT("RESOLVER - Something added to input queue.")));
                            pThis->ThreadOnQueueNotEmpty();
                            break;

                        case WAIT_OBJECT_0 + 1:
                            //
                            // Received input message(s).
                            // Loop back around and handle them.
                            //
                            DBGPRINT((DM_RESOLVER, DL_MID, TEXT("RESOLVER -  Thread %d rcvd message(s)."),
                                     GetCurrentThreadId()));
                            break;

                        case WAIT_FAILED:
                        case WAIT_TIMEOUT:
                        default:
                            //
                            // Something bad happened.
                            //
                            DBGPRINT((DM_RESOLVER, DL_MID, TEXT("RESOLVER - Thread %d wait failed."),
                                     GetCurrentThreadId()));

                            DBGASSERT((FALSE));
                            bExitThread = TRUE;
                            break;
                    }
                }
            }
            catch(...)
            {
                //
                // Exit thread in a controlled manner so we do required cleanup stuff.
                //
                DBGERROR((TEXT("RESOLVER - C++ Exception.")));
                bExitThread = TRUE;
            }                
        }
        CoUninitialize();
    }
    else
    {
        DBGERROR((TEXT("RESOLVER - OleInitialize failed for thread %d."),
                 GetCurrentThreadId()));
    }

    DBGPRINT((DM_RESOLVER, DL_HIGH, TEXT("RESOLVER - Exit thread %d"), GetCurrentThreadId()));

    pThis->m_dwResolverThreadId = 0;

    InterlockedDecrement(&g_cRefThisDll);
    CoFreeUnusedLibraries();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: SidNameResolver::CreateResolverThread

    Description: Creates a thread that will process user objects and resolve
        their SIDs to account names.  

    Arguments:
        phThread [optional] - Address of handle variable to receive handle of 
            new thread.  Can be NULL.

        pdwThreadId [optional] - Address of DWORD to receive thread ID.
            Can be NULL.

    Returns:
        NO_ERROR    - Started thread.
        E_FAIL      - Couldn't start thread.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/27/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
SidNameResolver::CreateResolverThread(
    PHANDLE phThread,
    LPDWORD pdwThreadId
    )
{
    DBGTRACE((DM_RESOLVER, DL_HIGH, TEXT("SidNameResolver::CreateResolverThread")));
    HRESULT hResult  = NO_ERROR;
    DWORD dwThreadId = 0;
    HANDLE hThread   = NULL;

    //
    // Launch new thread.
    //
    hThread = CreateThread(NULL,        // No security attributes.
                           0,           // Default stack size.
                           (LPTHREAD_START_ROUTINE)&ThreadProc,
                           this,        // Static thread proc needs this.
                           0,           // Not suspended.
                           &dwThreadId);
    if (NULL != hThread)
    {
        if (NULL != phThread)
            *phThread = hThread;  // Caller want's handle value.
        else
            CloseHandle(hThread); // Caller doesn't want it.

        if (NULL != pdwThreadId)
            *pdwThreadId = dwThreadId; // Caller want's thread ID.

        //
        // Wait here until the thread is ready to receive thread messages.
        // This event is set in ThreadProc.
        //
        WaitForSingleObject(m_heventResolverThreadReady, INFINITE);
    }
    else
    {
        hResult = E_FAIL;
    }

    return hResult;
}



