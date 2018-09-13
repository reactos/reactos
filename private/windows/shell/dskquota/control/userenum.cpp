///////////////////////////////////////////////////////////////////////////////
/*  File: enumuser.cpp

    Description: Contains member function definitions for class DiskQuotaUserEnum.
        The DiskQuotaUserEnum object is provided to enumerate the users in a 
        volume's quota information file.  The caller instantiates an enumerator
        through IDiskQuotaControl::CreateDiskQuotaUserEnum().  The enumerator's
        interface IEnumDiskQuotaUsers supports the normal OLE 2 enumeration
        functions Next(), Skip(), Reset() and Clone().

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h" // PCH
#pragma hdrstop

#include "user.h"
#include "userenum.h"

//
// Verify that build is UNICODE.
//
#if !defined(UNICODE)
#   error This module must be compiled UNICODE.
#endif

///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserEnum::DiskQuotaUserEnum

    Description: Constructor.

    Arguments:
        pFSObject - Pointer to an existing "file system" object.  
            It is through this object that the enumerator accesses the ntioapi 
            functions.  A pointer to this file system object is also passed 
            on to contained user objects so they may refresh their data when 
            required.  

        pQuotaController - Pointer to an IDiskQuotaControl interface that we'll
            AddRef().  The control object is who provides the "name changed"
            notification mechanism.  It needs to stay around as long as the
            enumerator is alive.

        pSidNameResolver - Pointer to an ISidNameResolver interface that will
            be used to resolve user SIDs to account names.  The resolver object
            is initially instantiated by the quota controller.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/15/97    Moved pQuotaControl, pSidNameResolver and pFSObject  BrianAu
                arguments from Initialize() to ctor.  Needed so
                that ref counting is correct.
*/
///////////////////////////////////////////////////////////////////////////////
DiskQuotaUserEnum::DiskQuotaUserEnum(
    PDISKQUOTA_CONTROL pQuotaController,
    PSID_NAME_RESOLVER pSidNameResolver,
    FSObject *pFSObject
    ) : m_cRef(0),
        m_pbBuffer(NULL),
        m_pbCurrent(NULL),
        m_cbBuffer(0),
        m_pSidList(NULL),
        m_cbSidList(0),
        m_bEOF(FALSE),
        m_bSingleUser(FALSE),
        m_bInitialized(FALSE),
        m_bRestartScan(TRUE),
        m_fNameResolution(DISKQUOTA_USERNAME_RESOLVE_NONE),
        m_pFSObject(pFSObject),
        m_pQuotaController(pQuotaController),
        m_pSidNameResolver(pSidNameResolver)
{
    DBGTRACE((DM_USER, DL_HIGH, TEXT("DiskQuotaUserEnum::DiskQuotaUserEnum")));

    if (NULL != m_pQuotaController)
        m_pQuotaController->AddRef();

    if (NULL != m_pSidNameResolver)
        m_pSidNameResolver->AddRef();

    if (NULL != m_pFSObject)
        m_pFSObject->AddRef();

    InterlockedIncrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserEnum::~DiskQuotaUserEnum

    Description: Destructor.  Destroys the enumerator's internal buffers and
        releases any held interface pointers.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DiskQuotaUserEnum::~DiskQuotaUserEnum(void)
{
    DBGTRACE((DM_USER, DL_HIGH, TEXT("DiskQuotaUserEnum::~DiskQuotaUserEnum")));

    if (NULL != m_pFSObject)
        m_pFSObject->Release();

    //
    // Order is important here.  Release the resolver before the controller.
    //
    if (NULL != m_pSidNameResolver)
        m_pSidNameResolver->Release();

    if (NULL != m_pQuotaController)
        m_pQuotaController->Release();

    delete [] m_pbBuffer;
    delete [] m_pSidList;

    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserEnum::QueryInterface

    Description: Obtain pointer to IUnknown or IEnumDiskQuotaUser.  Note that
        referenced object is uninitialized.  Recipient of interface pointer
        must call Initialize() member function before object is usable.

    Arguments:
        riid - Reference to requested interface ID.  IID_IUnknown and 
            IID_IEnumDiskQuotaUser are recognized.

        ppvOut - Address of interface pointer variable to accept the
            returned interface pointer.

    Returns:
        NO_ERROR        - Success.
        E_NOINTERFACE   - Requested interface not supported.
        E_INVALIDARG    - ppvOut argument is NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaUserEnum::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    HRESULT hResult = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IID_IUnknown == riid || IID_IEnumDiskQuotaUsers == riid)
    {
        //
        // Interface supported.
        //
        *ppvOut = this;
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hResult = NOERROR;
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserEnum::AddRef

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
DiskQuotaUserEnum::AddRef(
    VOID
    )
{
    ULONG ulReturn = m_cRef + 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("DiskQuotaUserEnum::AddRef, 0x%08X  %d -> %d"),
                     this, ulReturn - 1, ulReturn));

    InterlockedIncrement(&m_cRef);

    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserEnum::Release

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
DiskQuotaUserEnum::Release(
    VOID
    )
{
    ULONG ulReturn = m_cRef - 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("DiskQuotaUserEnum::Release, 0x%08X  %d -> %d"),
                     this, ulReturn + 1, ulReturn));

    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserEnum::Initialize

    Description: Initializes a new enumerator object.
        This member function is overloaded to provide two
        implementations.  The first accepts explicit arguments for
        initialization.  This member is intended for creating a new unique
        enumerator through IDiskQuotaControl::CreateEnumUsers.  The
        second implementation merely accepts a reference to an existing 
        EnumUsers object.  This member is intended to support the function
        IEnumDiskQuotaUser::Clone().

    Arguments:
        fNameResolution - Method of SID-to-name resolution. Can be one of the 
            following:
                    DISKQUOTA_USERNAME_RESOLVE_NONE
                    DISKQUOTA_USERNAME_RESOLVE_SYNC
                    DISKQUOTA_USERNAME_RESOLVE_ASYNC
 
        cbBuffer [optional] - Size in bytes of the internal buffer used in 
            calls to the NTIOAPI functions.  Default is ENUMUSER_BUF_LEN.

        rgpSids [optional] - Pointer to a list of SID pointers.  If 
            provided, only those users with SIDs included in the list are 
            returned.  This argument may be NULL in which case ALL users are
            included.  Any element containing a NULL pointer will terminate
            the list.

        cpSids [optional] - If pSidList is not NULL, this arg contains
            the count of entries in rgpSids.  If rgpSids is not NULL and this 
            argument contains 0, rgpSids is assumed to contain a terminating
            NULL pointer entry.

        UserEnum - Reference to an existing DiskQuotaUserEnum object.  The new
            object opens a connection to the same volume as the object being
            cloned.  The new object maintains a separate buffer for transfer
            of data from the NTIOAPI system.

    Returns:
        NO_ERROR          - Success.
        S_FALSE           - Already initialized.
        E_OUTOFMEMORY     - Insufficient memory.
        ERROR_INVALID_SID (hr) - A SID in rgpSids was invalid.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
DiskQuotaUserEnum::Initialize(
    DWORD fNameResolution,
    DWORD cbBuffer,
    PSID *rgpSids,
    DWORD cpSids
    )
{
    HRESULT hResult = NO_ERROR;

    if (m_bInitialized)
    {
        hResult = S_FALSE;
    }
    else
    {
        try
        {
            //
            // Create an internal buffer for data transfer from the ntioapi.
            //
            m_pbBuffer = new BYTE [cbBuffer];

            m_cbBuffer = cbBuffer;

            if (NULL != rgpSids)
            {
                //
                // A list of SID pointers was provided.
                // Initialize the SID list structure.
                // Can throw OutOfMemory.
                //
                m_bSingleUser = (cpSids == 1);
                hResult = InitializeSidList(rgpSids, cpSids);
            }

            if (SUCCEEDED(hResult))
            {
                //
                // Must have an independent instance of the controller's file system 
                // object.  The NTIOAPI functions maintain an enumeration context
                // for each open handle.  Therefore, each user enumerator must have a 
                // unique file handle to the NTIOAPI object.
                // I say this because it appears tempting to just keep a copy of the 
                // controller's FSObject pointer and AddRef it.
                //
                // This create-n-swap is sort of slimy.  We originally got a 
                // ptr to the caller's FSObject in the ctor.  However, now we want
                // to create our own FSObject.  Create a copy and release the original.
                //
                FSObject *pFsoTemp = m_pFSObject;
                m_pFSObject        = NULL;
                hResult = FSObject::Create(*pFsoTemp, &m_pFSObject);
                pFsoTemp->Release();

                if (SUCCEEDED(hResult))
                {
                    m_fNameResolution  = fNameResolution;
                    m_bInitialized     = TRUE;
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
    }

    return hResult;
}


HRESULT 
DiskQuotaUserEnum::Initialize(
    const DiskQuotaUserEnum& UserEnum
    )
{
    HRESULT hResult = NO_ERROR;

    try
    {
        //
        // Initialize the new enumerator without a SID list.
        // If the enumerator being copied has a SID list, we 
        // don't want to re-create a list of SID pointers for Initialize()
        // so we defer this for InitializeSidList.  InitializeSidList
        // has an overloaded version that accepts a pointer to an existing
        // SIDLIST structure and merely copies the bytes.
        //
        hResult = Initialize(UserEnum.m_fNameResolution,
                             UserEnum.m_cbBuffer,
                             NULL,
                             0);

        if (SUCCEEDED(hResult) && NULL != UserEnum.m_pSidList)
        {
            m_bSingleUser = UserEnum.m_bSingleUser;
            hResult = InitializeSidList(UserEnum.m_pSidList,
                                        UserEnum.m_cbSidList);
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



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserEnum::InitializeSidList

    Description: Initializes the m_pSidList member of the enumerator.
        The method comes in two overloaded forms.  The first accepts a pointer
        to an existing SIDLIST structure and merely creates a new copy.
        The second form accepts the address of an array of SID pointers and
        generates a new SIDLIST structure.

    Arguments:
        pSidList - Address of an existing SIDLIST structure to be copied.

        cbSidList - Number of bytes in the SIDLIST structure.

        rgpSids  - Pointer to a list of SID pointers.  If provided, only those 
            users with SIDs included in the list are returned.  This argument 
            may be NULL in which case ALL users are included.  Any element 
            containing a NULL pointer will terminate the list.

        cpSids - If pSidList is not NULL, this arg contains the count of 
            entries in rgpSids.  If rgpSids is not NULL and this argument 
            contains 0, rgpSids is assumed to contain a terminating NULL 
            pointer entry.

    Returns:
        NO_ERROR            - Success.
        ERROR_INVALID_SID (hr) - A SID in rgpSids was invalid.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/13/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DiskQuotaUserEnum::InitializeSidList(
    PSIDLIST pSidList,
    DWORD cbSidList
    )
{
    HRESULT hResult = NO_ERROR;

    DBGASSERT((NULL != pSidList));
    DBGASSERT((0 < cbSidList));

    //
    // Create a buffer for the SID list copy.
    //
    m_pSidList = (PSIDLIST)new BYTE[cbSidList];

    //
    // Copy the enumerator's SID list.
    //
    CopyMemory(m_pSidList, pSidList, cbSidList);
    m_cbSidList = cbSidList;

    return hResult;
}


HRESULT
DiskQuotaUserEnum::InitializeSidList(
    PSID *rgpSids,
    DWORD cpSids
    )
{
    HRESULT hResult = NO_ERROR;
    
    DBGASSERT((NULL != rgpSids));
    DBGASSERT((0 < cpSids));

    //
    // Create a SIDLIST structure from the array of SID pointers.
    // Can throw OutOfMemory.
    //
    hResult = CreateSidList(rgpSids, cpSids, &m_pSidList, &m_cbSidList);

    if (FAILED(hResult))
    {
        DBGASSERT((NULL == m_pSidList));
        DBGASSERT((0 == m_cbSidList));
    }

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserEnum::QueryQuotaInformation

    Description: Provides a simple wrapper around the NTIOAPI function
        QueryQuotaInformationFile. The function adds value by providing
        the address and size of the enumerator's data buffer.
        Note that the QueryQuotaInformationFile interface in NTIOAPI 
        functions as an enumerator itself.  Repeated calls enumerate
        user quota data much the same way the Next() function does in an OLE
        enumerator interface.  Data is returned in a byte buffer as a series
        of variable-length quota records.

    Arguments:
        bReturnSingleEntry [optional] - TRUE if only a single entry is
            desired.  FALSE if multiple records are desired.  Default is
            FALSE.

        pSidList [optional] - Pointer to a list of SIDs.  If provided, the
            data returned is only for those users included in the SID list.
            Default is NULL.

        cbSidList [optional] - If SidList is not NULL, contains length
            of SidList in bytes.  Default is 0.

        pStartSid [optional] - Pointer to SID in SID list where scan is to 
            start if bRestartScan is TRUE.  Default is NULL.

        bRestartScan [optional] - TRUE = restart enumeration at first user
            or user pointed to by StartSid in SidList (if provided).  
            FALSE = continue enumeration from current point.
            Default is FALSE.

    Returns:
        NO_ERROR             - Success.
        ERROR_NO_MORE_ITEMS  - No more user records.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
DiskQuotaUserEnum::QueryQuotaInformation(
    BOOL bReturnSingleEntry, 
    PVOID pSidList,           
    ULONG cbSidList,     
    PSID pStartSid,           
    BOOL bRestartScan        
    )
{
    HRESULT hResult = NO_ERROR;

    if (bRestartScan)
    {
        //
        // Reset EOF flag if restarting enumerator scan.
        //
        m_bEOF = FALSE;
    }

    if (!m_bEOF)
    {
        hResult = m_pFSObject->QueryUserQuotaInformation(
                        m_pbBuffer,
                        m_cbBuffer,
                        bReturnSingleEntry,
                        pSidList,
                        cbSidList,
                        pStartSid,
                        bRestartScan);

        if (ERROR_NO_MORE_ITEMS == HRESULT_CODE(hResult))
        {
            //
            // NtQueryQuotaInformationFile returns STATUS_NO_MORE_ENTRIES
            // when it has read the last entry, not when there are no more 
            // entries at the time of the call as you might expect.
            // We use the m_bEOF flag to indicate that we're at the end of the
            // quota information file but we return NO_ERROR so that the
            // caller will continue processing any records we just read.
            // Next time the caller requests quota information, we known that
            // we're at EOF so we return ERROR_NO_MORE_ITEMS.
            //
            m_bEOF  = TRUE;
            hResult = NO_ERROR;
        }
    }
    else
    {
        //
        // There REALLY are no more entries.
        //
        hResult = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
    }

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserEnum::CreateUserObject

    Description: Creates a new DiskQuotaUser object from the quota information
        retrieved through QueryQuotaInformation.  The caller provides a pointer
        to the start of the desired quota info record to be used for 
        initialization.

    Arguments:
        pfqi - Pointer to the quota information record used for initialization.

        ppOut - Address of interface pointer variable to accept the user object's
            IDiskQuotaUser interface pointer.

    Returns:
        NO_ERROR                - Success.
        E_INVALIDARG            - pfqi or ppOut arg is NULL.
        E_UNEXPECTED            - Unexpected error.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    09/05/96    Added exception handling.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
DiskQuotaUserEnum::CreateUserObject(
    PFILE_QUOTA_INFORMATION pfqi, 
    PDISKQUOTA_USER *ppOut
    )
{
    HRESULT hResult = NO_ERROR;

    if (NULL == pfqi || NULL == ppOut)
        return E_INVALIDARG;

    //
    // Create the user object and get the IDiskQuotaUser interface pointer.
    // This pointer is what is given to the caller.
    //
    m_pFSObject->AddRef();
    DiskQuotaUser *pUser = new DiskQuotaUser(m_pFSObject);

    //
    // Initialize the new user object using the buffered quota data pointed to
    // by pfqi.
    //
    hResult = pUser->Initialize(pfqi);

    if (SUCCEEDED(hResult))
    {
        hResult = pUser->QueryInterface(IID_IDiskQuotaUser, (LPVOID *)ppOut);
    }

    if (FAILED(hResult))
    {
        //
        // Either Initialize or QueryInterface failed.  Delete the object.
        //
        delete pUser;
        pUser = NULL;
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserEnum::GetNextUser

    Description: Creates a new user object from the "current" record in the
        quota information buffer. A pointer to the object's IDiskQuotaUser
        interface is returned and the "current" record pointer in the 
        quota information buffer is advanced to the next user record.

    Arguments:
        ppOut [optional] - Address of interface pointer variable to receive 
            address of user object's IDiskQuotaUserInterface pointer.  If this
            argument is NULL, the new user object is not created.  This is 
            useful for skipping items in the enumeration.

    Returns:
        NO_ERROR                 - Success.
        E_DISKQUOTA_INVALID_SID  - User record's SID is invalid.
        ERROR_NO_MORE_ITEMS      - No more users.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
DiskQuotaUserEnum::GetNextUser(
    PDISKQUOTA_USER *ppOut
    )
{
    PFILE_QUOTA_INFORMATION pfqi = (PFILE_QUOTA_INFORMATION)m_pbCurrent;
    HRESULT hResult              = NO_ERROR;

    //
    // If m_pbCurrent is NULL, this is the first request for data.
    // If pfqi->NextEntryOffset is 0, we need to read another buffer of data.
    //
    if (NULL == m_pbCurrent)
    {
        //
        // No more user entries in buffer.
        // Read quota information into m_pbBuffer.
        // Use SID list if we have one.
        //
        hResult = QueryQuotaInformation(m_bSingleUser,   // Single user?
                                        m_pSidList,      // SID list.
                                        m_cbSidList,     // SID list length.
                                        0,               // Start SID,
                                        m_bRestartScan); // Restart scan?
        if (SUCCEEDED(hResult))
        {
            //
            // New information in buffer. Reset record pointers.
            //
            m_pbCurrent    = m_pbBuffer;
            m_bRestartScan = FALSE;
            pfqi = (PFILE_QUOTA_INFORMATION)m_pbCurrent;
        }
    }

    if (SUCCEEDED(hResult))
    {
        //
        // We have a valid pointer into the buffer of user quota data.
        //
        if (NULL != ppOut)
        {
            // 
            // Caller provided a user interface pointer variable.
            // Create a new user record.  This can throw OutOfMemory.
            //
            hResult = CreateUserObject(pfqi, ppOut);
        }

        if (0 != pfqi->NextEntryOffset)
            m_pbCurrent += pfqi->NextEntryOffset; // Advance to next user.
        else
            m_pbCurrent = NULL;  // Reset to trigger quota file read.

    }
    return hResult;
}    


    
///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserEnum::Next

    Description: Retrieve the next cUsers records from the volume's quota
        information file.  If the enumerator was created with a SidList,
        only those users contained in the SidList are included in the 
        enumeration.  Repeated calls to Next() continue to enumerate
        successive quota users.   The Reset() function may be used to 
        reset the enumerator to the start of the enumeration.  

    Arguments:
        cUsers - Number of elements in paUsers array.

        pUser - Array of IDiskQuotaUser pointers.  Must provide space for 
            cUsers pointers. Upon return, each element of this array contains 
            an interface pointer to a DiskQuotaUser object.

        pcCreated [optional] - Address of DWORD to accept the count of user 
            object interface pointers returned in pUser.  Note that any 
            array locations equal to or beyond the value returned in 
            pcCreated are invalid and set to NULL.

    Returns:
        S_OK            - Success.  Enumerated number of requested users.
        S_FALSE         - End of enumeration encountered.  Returning less than
                          cUsers records.
        E_INVALIDARG    - pUser arg is NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
DiskQuotaUserEnum::Next(
    DWORD cUsers,                        // Number of elements in array.
    PDISKQUOTA_USER *pUser,              // Dest array for quota user interface ptrs.
    DWORD *pcCreated                     // Return number created.
    )
{
    HRESULT hResult = S_OK;
    UINT i          = 0;                // Index into user caller's array.
    UINT cCreated   = 0;

    if (NULL == pUser)
        return E_INVALIDARG;

    if (NULL != pcCreated)
        *pcCreated = 0;

    try
    {
        IDiskQuotaUser *pNextUser = NULL;   // Ptr to new user.

        //
        // Enumerate user records until one of the following:
        // 1. Failure.
        // 2. No more users.
        // 3. Enumerated requested count.
        //
        while(SUCCEEDED(hResult) && cUsers > 0)
        {
            //
            // Create new user object.  This can throw OutOfMemory.
            //
            hResult = GetNextUser(&pNextUser);
            if (SUCCEEDED(hResult))
            {
                //
                // User records come from the quota file containing only a SID.
                // We must ask the SidNameResolver to locate the corresponding
                // account name.  If client wants names synchronously, we block
                // here until account name is found.  User object will contain
                // account name.
                // If client wants names asynchronously, the user object is handed
                // off to the resolver for background processing.  We continue on.
                // If the client implemented the IDiskQuotaEvents interface and
                // called IConnectionPoint::Advise, it will receive a 
                // OnUserNameChange notification when the name is finally resolved.
                // If user doesn't want user name resolved, don't do either. 
                // This would be the case if the client already has the SID/Name
                // pair and just wants user objects.
                //
                switch(m_fNameResolution)
                {
                    case DISKQUOTA_USERNAME_RESOLVE_ASYNC:
                        m_pSidNameResolver->FindUserNameAsync(pNextUser);
                        break;
                    case DISKQUOTA_USERNAME_RESOLVE_SYNC:
                        m_pSidNameResolver->FindUserName(pNextUser);
                        break;
                    case DISKQUOTA_USERNAME_RESOLVE_NONE:
                    default:
                        break;
                }

                //
                // Note: Ref count for pUser already incremented in 
                // DiskQuotaUser::QueryInterface.
                //
                *pUser = pNextUser;
                pUser++;
                cUsers--;
                cCreated++;
            }
        }

        if (NULL != pcCreated)
            *pcCreated = cCreated; // If requested, return number of users created.

        if (cUsers > 0)
        {
            //
            // Less than requested number of users were retrieved.
            // 
            hResult = S_FALSE;
            while(cUsers > 0)
            {
                //
                // Set any un-filled array elements to NULL.
                //
                *pUser = NULL;
                pUser++;
                cUsers--;
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

    if (FAILED(hResult))
    {
        //
        // Release any user objects already created.
        //
        for (i = 0; i < cCreated; i++)
        {
            PDISKQUOTA_USER pu = *(pUser + i);
            if (NULL != pu)
            {
                pu->Release();
                *(pUser + i) = NULL;
            }
        }

        *pcCreated = 0;        
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserEnum::Skip

    Description: Skips a specified number of users in the user enumeration.

    Arguments:
        cUsers - Number of users to skip.

    Returns:
        S_OK            - Success.  Skipped number of requested users.
        S_FALSE         - End of enumeration encountered.  Skipped less than
                          cUsers records.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
DiskQuotaUserEnum::Skip(
    DWORD cUsers
    )
{
    while(cUsers > 0 && SUCCEEDED(GetNextUser(NULL)))
    {
        cUsers--;
    }

    return cUsers == 0 ? S_OK : S_FALSE;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserEnum::Reset

    Description: Resets the enumerator object so that the next call to Next()
        starts enumerating at the beginning of the enumeration.

    Arguments: None.

    Returns:
        Always returns S_OK.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    02/09/99    Changed so we just reset m_pbCurrent and             BrianAu
                m_bRestartScan.
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaUserEnum::Reset(
    VOID
    )
{
    m_pbCurrent    = NULL;
    m_bRestartScan = TRUE;
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserEnum::Clone

    Description: Creates a duplicate of the enumerator object and returns
        a pointer to the new object's IEnumDiskQuotaUser interface.

    Arguments:
        ppOut - Address of interface pointer variable to accept the pointer
            to the new object's IEnumDiskQuotaUser interface.

    Returns:
        NO_ERROR        - Success.
        E_OUTOFMEMORY   - Insufficient memory to create new enumerator.
        E_INVALIDARG    - ppOut arg was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaUserEnum::Clone(
    PENUM_DISKQUOTA_USERS *ppOut
    )
{
    HRESULT hResult = NO_ERROR;

    if (NULL == ppOut)
        return E_INVALIDARG;

    try
    {
        DiskQuotaUserEnum *pUserEnum = new DiskQuotaUserEnum(
                                                m_pQuotaController,
                                                m_pSidNameResolver,
                                                m_pFSObject);

        hResult = pUserEnum->Initialize(*this);

        if (SUCCEEDED(hResult))
        {
            hResult = pUserEnum->QueryInterface(IID_IEnumDiskQuotaUsers, 
                                               (LPVOID *)ppOut);
        }

        if (FAILED(hResult))
        {
            //
            // Either Initialize or QueryInterface failed.
            //
            delete pUserEnum;
            pUserEnum = NULL;
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

    

DiskQuotaUserCollection::DiskQuotaUserCollection(
    PDISKQUOTA_CONTROL pController,
    DWORD fNameResolution

    ) : m_cRef(0),
        m_pController(pController),
        m_pEnum(NULL),
        m_fNameResolution(fNameResolution)
{
    if (NULL != m_pController)
    {
        m_pController->AddRef();
    }
}

DiskQuotaUserCollection::~DiskQuotaUserCollection(
    VOID
    )
{
    if (NULL != m_pEnum)
    {
        m_pEnum->Release();
    }
    if (NULL != m_pController)
    {
        m_pController->Release();
    }
}


HRESULT
DiskQuotaUserCollection::Initialize(
    VOID
    )
{
    HRESULT hr = S_FALSE; // Assume already initialized.

    if (NULL == m_pEnum)
    {
        if (NULL == m_pController)
        {
            hr = E_UNEXPECTED;
        }
        else
        {
            hr = m_pController->CreateEnumUsers(NULL,
                                                0,
                                                m_fNameResolution,
                                                &m_pEnum);
        }
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserCollection::QueryInterface

    Description: Obtain pointer to IUnknown or IEnumDiskQuotaUserVARIANTs.  

    Arguments:
        riid - Reference to requested interface ID.  

        ppvOut - Address of interface pointer variable to accept the
            returned interface pointer.

    Returns:
        NO_ERROR        - Success.
        E_NOINTERFACE   - Requested interface not supported.
        E_INVALIDARG    - ppvOut argument is NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaUserCollection::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    HRESULT hResult = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IID_IUnknown == riid || 
        IID_IEnumVARIANT == riid)
    {
        //
        // Interface supported.
        //
        *ppvOut = this;
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hResult = NOERROR;
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserCollection::AddRef

    Description: Increments object reference count.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
DiskQuotaUserCollection::AddRef(
    VOID
    )
{
    ULONG ulReturn = m_cRef + 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("DiskQuotaUserCollection::AddRef, 0x%08X  %d -> %d"),
                     this, ulReturn - 1, ulReturn));

    InterlockedIncrement(&m_cRef);

    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserCollection::Release

    Description: Decrements object reference count.  If count drops to 0,
        object is deleted.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
DiskQuotaUserCollection::Release(
    VOID
    )
{
    ULONG ulReturn = m_cRef - 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("DiskQuotaUserCollection::Release, 0x%08X  %d -> %d"),
                     this, ulReturn + 1, ulReturn));

    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}


STDMETHODIMP
DiskQuotaUserCollection::Next(
    DWORD cUsers,
    VARIANT *rgvar,
    DWORD *pcUsersFetched
    )
{
    HRESULT hr = E_UNEXPECTED;
    try
    {
        if (NULL == pcUsersFetched && 1 < cUsers)
        {
            //
            // If pcUsersFetched is NULL, cUsers must be 1.
            //
            hr = E_INVALIDARG;
        }
        else
        {
            DWORD cEnumerated = 0;
            PDISKQUOTA_USER *prgUsers = new PDISKQUOTA_USER[cUsers];
            if (NULL != prgUsers)
            {
                hr = m_pEnum->Next(cUsers, prgUsers, &cEnumerated);
                if (SUCCEEDED(hr))
                {
                    for (INT i = 0; i < (INT)cEnumerated; i++)
                    {
                        VariantInit(&rgvar[i]);

                        IDispatch *pIDisp = NULL;
                        hr = prgUsers[i]->QueryInterface(IID_IDispatch, (LPVOID *)&pIDisp);
                        if (SUCCEEDED(hr))
                        {
                            V_VT(&rgvar[i])       = VT_DISPATCH;
                            V_DISPATCH(&rgvar[i]) = pIDisp;
                        }
                        prgUsers[i]->Release();
                    }
                }
                delete[] prgUsers;
            }
            if (NULL != pcUsersFetched)
            {
                *pcUsersFetched = cEnumerated;
            }
        }
    }
    catch(CAllocException& e)
    {
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        hr = E_UNEXPECTED;
    }
    return hr;
}


STDMETHODIMP
DiskQuotaUserCollection::Skip(
    DWORD cUsers
    )
{
    return m_pEnum->Skip(cUsers);
}

STDMETHODIMP
DiskQuotaUserCollection::Reset(
    void
    )
{
    return m_pEnum->Reset();
}


STDMETHODIMP
DiskQuotaUserCollection::Clone(
    IEnumVARIANT **ppEnum
    )
{
    HRESULT hr = E_FAIL;
    try
    {
        DiskQuotaUserCollection *pEnum = new DiskQuotaUserCollection(m_pController,
                                                                     m_fNameResolution);
        if (NULL != pEnum)
        {
            hr = pEnum->Initialize();
            if (SUCCEEDED(hr))
            {
                hr = pEnum->QueryInterface(IID_IEnumVARIANT, (LPVOID *)ppEnum);
            }
        }
    }
    catch(CAllocException& me)
    {
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        hr = E_UNEXPECTED;
    }
    return hr;
}


