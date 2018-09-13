///////////////////////////////////////////////////////////////////////////////
/*  File: FreeStore.cpp

    Description: Declaration for class FreeStore.  This class provides 
        a replacement for global new and delete.  It adds value by 
        recording allocations in DEBUG builds to help find memory leaks.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/12/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "precomp.hxx" // PCH
#pragma hdrstop

FreeStore g_FreeStore;  // Global heap FreeStore object.


///////////////////////////////////////////////////////////////////////////////
/*  Function: new

    Description: Replacement for global new.

    Arguments:
        cb - Number of bytes to allocate.

    Returns:
        On success, returns pointer to allocated block.
        Throws OutOfMemory if block can't be allocated.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/12/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
void * __cdecl 
operator new(
    unsigned int cb
    ) throw(OutOfMemory)
{
    void *pv = g_FreeStore.Alloc(cb);

    DebugMsg(DM_ALLOC, TEXT("new: %6d bytes allocated at 0x%08X"), cb, pv);
    return pv;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: delete

    Description: Replacement for global delete.

    Arguments:
        pv - Address of block (previously returned by "new") to deallocate.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/12/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
void __cdecl 
operator delete(
    void *pv
    )
{
    DebugMsg(DM_ALLOC, TEXT("delete: block at 0x%08X"), pv);
    g_FreeStore.Free(pv);
}





#ifdef DEBUG_ALLOC
///////////////////////////////////////////////////////////////////////////////
/*  Function: FreeStore::Stats::~Stats

    Description: Constructor for FreeStore statistics object.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
FreeStore::Stats::Stats(
    VOID
    ) : m_pBlockList(NULL),
        m_cCurrentBlocks(0),
        m_cCummulativeBlocks(0),
        m_cCurrentBytes(0),
        m_cCummulativeBytes(0),
        m_hMutex(NULL)
{ 

}


///////////////////////////////////////////////////////////////////////////////
/*  Function: FreeStore::Stats::~Stats

    Description: Destructor for FreeStore statistics.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
FreeStore::Stats::~Stats(
    VOID
    )
{
    Lock();
    Block *pThis = m_pBlockList;
    while(NULL != pThis)
    {
        m_pBlockList = pThis->m_pNext;
		LocalFree(pThis);
        pThis = m_pBlockList;
	}
    ReleaseLock();
    if (NULL != m_hMutex)
        CloseHandle(m_hMutex);
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: FreeStore::Stats::Initialize

    Description: Initializes the FreeStore statistics object.

    Arguments: None.

    Returns:
        NO_ERROR  - Success
        E_FAIL    - Couldn't create mutex.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
FreeStore::Stats::Initialize(
    VOID
    )
{
    m_hMutex = CreateMutex(NULL, FALSE, NULL);
    return NULL != m_hMutex ? NO_ERROR : E_FAIL;
}
    
///////////////////////////////////////////////////////////////////////////////
/*  Function: FreeStore::Stats::Add

    Description: Adds a new entry to the FreeStore's statistics pool.

    Arguments:
        pBlock - Address of block allocated.

        cbBlock - Number of bytes in block.

    Returns:
        NO_ERROR      - Success
        E_OUTOFMEMORY - Insufficient memory to allocate stats node.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
FreeStore::Stats::Add(
    LPVOID pBlock, 
    DWORD cbBlock
    )
{
    HRESULT hResult = NO_ERROR;
    Lock();
    //
    // Create a new node and insert at head of list.
    //
    Block *pInfo = (Block *)LocalAlloc(LPTR, sizeof(Block));
    if (NULL != pInfo)
    {
        pInfo->m_pBlock  = pBlock;
        pInfo->m_cbBlock = cbBlock;

        pInfo->m_pNext = m_pBlockList;
        m_pBlockList   = pInfo;

        m_cCurrentBlocks++;
        m_cCurrentBytes += cbBlock;
        m_cCummulativeBlocks++;
        m_cCummulativeBytes += cbBlock;
        if (m_cCurrentBlocks > m_cMaxBlocks)
            m_cMaxBlocks = m_cCurrentBlocks;
        if (m_cCurrentBytes > m_cMaxBytes)
            m_cMaxBytes = m_cCurrentBytes;
    }
    else
        hResult = E_OUTOFMEMORY;

    ReleaseLock();
    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: FreeStore::Stats::Delete

    Description: Remove an entry from the FreeStore's statistics pool.

    Arguments: 
        pBlock - Address of allocated block to remove.

    Returns:
        NO_ERROR    - Success.
        E_FAIL      - pBlock not found.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
FreeStore::Stats::Delete(
    LPVOID pBlock
    )
{
    Lock();
    HRESULT hResult = NO_ERROR;
    Block *pThis = m_pBlockList;
    Block *pPrev = NULL;

    while(NULL != pThis && pThis->m_pBlock != pBlock)
    {
        pPrev = pThis;
        pThis = pThis->m_pNext;
    }

    if (NULL != pThis)
    {
        m_cCurrentBlocks--;
        m_cCurrentBytes -= pThis->m_cbBlock;

        if (pThis == m_pBlockList)
            m_pBlockList = pThis->m_pNext;
        else
            pPrev->m_pNext = pThis->m_pNext;

        LocalFree(pThis);
    }
    else
        hResult = E_FAIL;

    ReleaseLock();
    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: FreeStore::Stats::Realloc

    Description: Change the accounting information for a block to reflect
        a re-allocation of the block.

    Arguments: 
        pOldBlock - Address of allocated block to be reallocated.
        
        pNewBlock - New address of reallocated block.  May be the same as
            pOldBlock.

        cbNewBlock - Number of bytes in new block.

    Returns:
        NO_ERROR    - Success.
        E_FAIL      - pBlock not found.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
FreeStore::Stats::Realloc(
    LPVOID pOldBlock,
    LPVOID pNewBlock,
    DWORD cbNewBlock
    )
{
    Lock();
    HRESULT hResult = NO_ERROR;
    Block *pThis = m_pBlockList;
    Block *pPrev = NULL;

    while(NULL != pThis && pThis->m_pBlock != pOldBlock)
    {
        pPrev = pThis;
        pThis = pThis->m_pNext;
    }

    if (NULL != pThis)
    {
        m_cCurrentBlocks--;
        m_cCurrentBytes += (cbNewBlock - pThis->m_cbBlock);
        if (m_cCurrentBytes > m_cMaxBytes)
            m_cMaxBytes = m_cCurrentBytes;

        pThis->m_pBlock  = pNewBlock;
        pThis->m_cbBlock = cbNewBlock;        
    }
    else
        hResult = E_FAIL;

    ReleaseLock();
    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: FreeStore::Stats::Dump

    Description: Dumps contents of FreeStore's statistics object to the debug
        terminal.  

    Arguments:
        bDumpAll - If TRUE, data for each node is dumped.  If FALSE, only
            summary information is output.

    Returns:
        NO_ERROR - Always returns NO_ERROR.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
FreeStore::Stats::Dump(
    BOOL bDumpAll
    )
{
    DWORD dwDebugMask = DM_ALLOC | DM_ALLOCSUM;

    Lock();
    Block *pThis = m_pBlockList;

    DebugMsg(dwDebugMask, 
             TEXT("[blks/bytes] - Cur [%d/%d]  Cumm [%d/%d]  Max [%d/%d]"),
                           m_cCurrentBlocks,
                           m_cCurrentBytes,
                           m_cCummulativeBlocks,
                           m_cCummulativeBytes,
                           m_cMaxBlocks,
                           m_cMaxBytes);

    if (bDumpAll && m_cCurrentBlocks > 0)
    {
        DebugMsg(dwDebugMask, TEXT("List of blocks currently allocated:"));

        while(NULL != pThis)
        {
            DebugMsg(dwDebugMask, TEXT("\tBlock 0x%08X, %4d bytes"), 
                     pThis->m_pBlock, pThis->m_cbBlock);
            pThis = pThis->m_pNext;
        }
    }
    ReleaseLock();
    return NO_ERROR;
}

#endif // DEBUG_ALLOC
