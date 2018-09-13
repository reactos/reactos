///////////////////////////////////////////////////////////////////////////////
/*  File: userbat.cpp

    Description: Contains member function definitions for class 
        DiskQuotaUserBatch.
        The DiskQuotaUserBatch object represents a batch update mechanism for
        rapid update of multiple-user-object quota information.  This class
        takes advantage of the batching capabilities built into the NTIOAPI.
        A user batch object is obtained through 
        IDiskQuotaControl::CreateUserBatch().

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/06/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h" // PCH
#pragma hdrstop

#include "userbat.h"

//
// Verify that build is UNICODE.
//
#if !defined(UNICODE)
#   error This module must be compiled UNICODE.
#endif

//
// The NTFS quota write function can only handle a max of 64K of data
// in any one write operation.  IssacHe recommended 60K as a comfortable
// limit.
//
const INT MAX_BATCH_BUFFER_BYTES = (1 << 10) * 60;

///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserBatch::DiskQuotaUserBatch

    Description: Constructor.

    Arguments: 
        pFSObject - Address of File System object to be utilized by the
            batching operations.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/03/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DiskQuotaUserBatch::DiskQuotaUserBatch(
    FSObject *pFSObject
    ) : m_cRef(0),
        m_pFSObject(pFSObject)
{      
    DBGASSERT((NULL != m_pFSObject));

    m_pFSObject->AddRef();
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserBatch::~DiskQuotaUserBatch

    Description: Destructor.

    Arguments: Destroys the batch object.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/26/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DiskQuotaUserBatch::~DiskQuotaUserBatch(
    VOID
    )
{
    Destroy();
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserBatch::QueryInterface

    Description: Returns an interface pointer to the object's IUnknown or 
        IDiskQuotaUserBatch interface.  Only IID_IUnknown and 
        IID_IDiskQuotaUserBatch are recognized.  The object referenced by the 
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
    06/06/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaUserBatch::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    HRESULT hResult = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IID_IUnknown == riid || IID_IDiskQuotaUserBatch == riid)
    {
        *ppvOut = this;
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hResult = NOERROR;
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserBatch::AddRef

    Description: Increments object reference count.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/06/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
DiskQuotaUserBatch::AddRef(
    VOID
    )
{
    ULONG ulReturn = m_cRef + 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("DiskQuotaUserBatch::AddRef, 0x%08X  %d -> %d\n"),
             this, ulReturn - 1, ulReturn));

    InterlockedIncrement(&m_cRef);

    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserBatch::Release

    Description: Decrements object reference count.  If count drops to 0,
        object is deleted.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/06/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
DiskQuotaUserBatch::Release(
    VOID
    )
{
    ULONG ulReturn = m_cRef - 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("DiskQuotaUserBatch::Release, 0x%08X  %d -> %d\n"),
             this, ulReturn + 1, ulReturn));

    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserBatch::Destroy

    Description: Destroys the contents of a user batch object and releases
        its FSObject pointer.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/06/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
DiskQuotaUserBatch::Destroy(VOID)
{
    //
    // Remove and release all user object pointers from the batch list.
    //
    RemoveAll();

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
/*  Function: DiskQuotaUserBatch::Add

    Description: Adds an IDiskQuotaUser interface pointer to the batch list.

    Arguments:
        pUser - Address of IDiskQuotaUser interface.

    Returns:
        NOERROR         - Success.
        E_INVALIDARG    - pUser arg is NULL. 
        E_OUTOFMEMORY   - Couldn't create new node in batch list.
        E_UNEXPECTED    - Unexpected exception.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/06/96    Initial creation.                                    BrianAu
    09/03/96    Add exception handling.                              BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaUserBatch::Add(
    PDISKQUOTA_USER pUser
    )
{
    HRESULT hResult = NOERROR;
        
    if (NULL == pUser)
        return E_INVALIDARG;

    try
    {
        m_UserList.Append(pUser);
        //
        // Success.  Increment ref count on object.
        //
        pUser->AddRef();
    }
    catch(CAllocException& e)
    {
        hResult = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("C++ exception adding user to batch.")));
        hResult = E_UNEXPECTED;  // Throwing stops here!
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserBatch::Remove

    Description: Removes a user pointer from the batch queue.  

    Arguments:
        pUser - Address of IDiskQuotaUser interface for the user object to
            be removed.

    Returns:
        S_OK         - Success.
        S_FALSE      - User not found in batch object.
        E_INVALIDARG - pUser argument is NULL.
        E_UNEXPECTED - Unexpected exception.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/06/96    Initial creation.                                    BrianAu
    09/03/96    Add exception handling.                              BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DiskQuotaUserBatch::Remove(
    PDISKQUOTA_USER pUser
    )
{
    HRESULT hResult = S_FALSE;  // Assume user not present.
    PDISKQUOTA_USER pRemoved = NULL;

    if (NULL == pUser)
        return E_INVALIDARG;

    m_UserList.Lock();
    INT iUser = m_UserList.Find(pUser);
    if (-1 != iUser)
    {
        try
        {
            DBGASSERT((NULL != m_UserList[iUser]));
            m_UserList[iUser]->Release();
            m_UserList.Delete(iUser);
            hResult = S_OK;
        }
        catch(...)
        {
            DBGERROR((TEXT("C++ exception removing user from batch.")));
            hResult = E_UNEXPECTED;  // Throwing stops here!
        }
    }
    m_UserList.ReleaseLock();

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserBatch::RemoveAll

    Description: Removes all user pointers from the batch
        list and calling Release() through the removed pointer.  

    Arguments: None.

    Returns:
        NOERROR      - Success.
        E_UNEXPECTED - Unexpected exception.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/06/96    Initial creation.                                    BrianAu
    09/03/96    Add exception handling.                              BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaUserBatch::RemoveAll(
    VOID
    )
{
    HRESULT hResult = NOERROR;

    m_UserList.Lock();
    INT cUsers = m_UserList.Count();
    for (INT i = 0; i < cUsers; i++)
    {
        try
        {
            DBGASSERT((NULL != m_UserList[i]));
            m_UserList[i]->Release();
        }
        catch(...)
        {
            DBGERROR((TEXT("C++ exception removing user from batch.")));
            hResult = E_UNEXPECTED;  // Throwing stops here!
        }
    }
    m_UserList.Clear();
    m_UserList.ReleaseLock();

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUserBatch::FlushToDisk

    Description: Writes data for all batched user objects to disk in a single
        NTIOAPI call.  This is the real worker function for the batch object.

    Arguments: None.

    Returns:
        NOERROR       - Success.
        E_OUTOFMEMORY - Insufficient memory.
        E_UNEXPECTED  - Unexpected exception.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/06/96    Initial creation.                                    BrianAu
    09/03/96    Add exception handling.                              BrianAu
    02/27/97    Divided NTFS writes into max 60KB pieces.            BrianAu
                The quota code in NTFS couldn't handle larger
                buffers.  It got into an infinite loop condition
                due to filling of the log.
    07/01/97    Replaced use of PointerList with CArray<>.           BrianAu
                Now use indexes instead of iterators.
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
DiskQuotaUserBatch::FlushToDisk(
    VOID
    )
{
    HRESULT hResult                   = NOERROR;
    PFILE_QUOTA_INFORMATION pUserInfo = NULL;
    PDISKQUOTA_USER pUser             = NULL;
    PBYTE pbBatchBuffer               = NULL;
    DWORD cbMinimumSid = FIELD_OFFSET(SID, SubAuthority) + sizeof(LONG);
    INT iOuter = 0;

    //
    // Do nothing if the batch object is empty.
    //
    if (0 == m_UserList.Count())
        return NOERROR;

    m_UserList.Lock();

    try
    {
        //
        // Process the data in 60K chunks using a nested loop.
        //
        while(iOuter < m_UserList.UpperBound())
        {
            //
            // Clone the outer iterator so we can process the next 60K of data.
            // Need two new iterators.  One for counting the bytes and
            // one for transferring data to the write buffer.  They're very small
            // objects and cheap to create.
            //
            INT iCount    = iOuter;
            INT iTransfer = iOuter;

            DWORD cbBatchBuffer = 0;
            DWORD cItemsThisBatch = 0;

            while(cbBatchBuffer < MAX_BATCH_BUFFER_BYTES &&
                  iCount <= m_UserList.UpperBound())

            {
                DWORD cbSid = 0;
                pUser = m_UserList[iCount++];
                pUser->GetSidLength(&cbSid);

                //
                // Total size required for user records.
                //
                cbBatchBuffer += FIELD_OFFSET(FILE_QUOTA_INFORMATION, Sid) + cbSid;

                //
                // Ensure it's quad-word aligned.
                //
                if (cbBatchBuffer & 0x00000007)
                    cbBatchBuffer = (cbBatchBuffer & 0xFFFFFFF8) + 8;

                cItemsThisBatch++;
            }

            //
            // Allocate the buffer.
            //
            pbBatchBuffer = new BYTE[cbBatchBuffer];

            PBYTE pbBatchBufferItem = pbBatchBuffer;
            DWORD cbNextEntryOffset = 0;
            //
            // Now fill in the batch transaction buffer with data from 
            // all of the users in the batch list.
            //
            while(0 != cItemsThisBatch-- &&
                  iTransfer <= m_UserList.UpperBound())
            {
                pUser = m_UserList[iTransfer++];
                pUserInfo = (PFILE_QUOTA_INFORMATION)pbBatchBufferItem;

                pUser->GetSidLength(&pUserInfo->SidLength);

                cbNextEntryOffset = FIELD_OFFSET(FILE_QUOTA_INFORMATION, Sid) + pUserInfo->SidLength;
                //
                // Ensure quad-word alignment.
                //
                if (cbNextEntryOffset & 0x00000007)
                    cbNextEntryOffset = (cbNextEntryOffset & 0xFFFFFFF8) + 8;

                pUserInfo->NextEntryOffset = cbNextEntryOffset;

                pUser->GetQuotaThreshold(&pUserInfo->QuotaThreshold.QuadPart);
                pUser->GetQuotaLimit(&pUserInfo->QuotaLimit.QuadPart);
                pUser->GetSid((PBYTE)&pUserInfo->Sid, pUserInfo->SidLength);

                //
                // These two don't get set but let's provide a known value anyway.
                //
                pUserInfo->ChangeTime.QuadPart = 0;
                pUserInfo->QuotaUsed.QuadPart  = 0;

                pbBatchBufferItem += cbNextEntryOffset;
            }
            pUserInfo->NextEntryOffset = 0;  // Last entry needs a 0 here.
            //
            // Submit the batch to the NTIOAPI for update.
            //
            hResult = m_pFSObject->SetUserQuotaInformation(pbBatchBuffer, cbBatchBuffer);

            //
            // Delete the data buffer.
            //
            delete[] pbBatchBuffer;
            pbBatchBuffer = NULL;

            //
            // Advance the outer iterator to where the transfer iterator left off.
            //
            iOuter = iTransfer;
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
        // Something failed.  Invalid data cached in user objects.
        // Next request for user data will have to read from disk.
        //
        iOuter = 0;

        while(iOuter <= m_UserList.UpperBound())
        {
            pUser = m_UserList[iOuter++];
            pUser->Invalidate();
        }
    }

    m_UserList.ReleaseLock();

    delete[] pbBatchBuffer;

    return hResult;
}
