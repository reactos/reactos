#ifndef __FREE_STORE_H
#define __FREE_STORE_H
///////////////////////////////////////////////////////////////////////////////
/*  File: FreeStore.h

    Description: Declaration for class FreeStore.  This class provides 
        a replacement for global new and delete.  It adds value by 
        recording allocations in DEBUG builds to help find memory leaks.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/12/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "except.h"

//
// Overload global new and delete.
//
void *__cdecl operator new(unsigned int cb) throw(OutOfMemory);
void  __cdecl operator delete(void *pv) throw(OutOfMemory);

#ifdef DEBUG
// 
// Defining DEBUG_ALLOC causes the allocator to keep track of each allocation
// and to generate a report of memory leaks when the allocator is destroyed.
//
#   define DEBUG_ALLOC 1
#endif

class FreeStore
{
    private:

#ifdef DEBUG_ALLOC 
        class Stats
        {
            public:
                class Block
                {
                    public:
                        LPVOID m_pBlock;
                        DWORD  m_cbBlock;
                        Block *m_pNext;

                        Block(LPVOID pBlock = NULL, DWORD cbBlock = 0)
                            : m_cbBlock(cbBlock),
                              m_pBlock(pBlock),
                              m_pNext(NULL) { }

                };

                Block *m_pBlockList;
                DWORD m_cCurrentBlocks;
                DWORD m_cCummulativeBlocks;
                DWORD m_cMaxBlocks;
                DWORD m_cCurrentBytes;
                DWORD m_cCummulativeBytes;
                DWORD m_cMaxBytes;
                HANDLE m_hMutex;

                Stats(VOID);
                ~Stats(VOID);

                HRESULT Initialize(VOID);
                HRESULT Add(LPVOID pBlock, DWORD cbBlock);
                HRESULT Delete(LPVOID pBlock);
                HRESULT Realloc(LPVOID pOldBlock, LPVOID pNewBlock, DWORD cbNewBlock);
                HRESULT Dump(BOOL bAll);
                VOID Lock(VOID)
                    { WaitForSingleObject(m_hMutex, INFINITE); }

                VOID ReleaseLock(VOID)
                    { ReleaseMutex(m_hMutex); }

        };

        Stats m_Stats;

#endif // DEBUG_ALLOC

        //
        // Prevent copying.
        //
        FreeStore(const FreeStore&);
        FreeStore& operator = (const FreeStore&);

    public:
        FreeStore(VOID) { }

        inline void *Alloc(ULONG cb) throw(OutOfMemory);

        inline void Free(void *pv);

#ifdef DEBUG_ALLOC
        VOID DumpStatistics(VOID)
            { m_Stats.Dump(TRUE); }
#endif
};

extern FreeStore  g_FreeStore;    // Global heap allocator.


///////////////////////////////////////////////////////////////////////////////
/*  Function: FreeStore::Alloc [inline]

    Description: Allocates a block of memory.
        CAUTION:  This function is declared "inline" because it is currently
                  only used in the override of global new.  If it is ever used
                  in multiple locations, the impact of this inlining should
                  be reassessed.

    Arguments:
        cb - Number of bytes requested.

    Returns:
        On success, returns pointer to allocated block.
        On failure, throws OutOfMemory.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/12/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
inline 
void *
FreeStore::Alloc(
    ULONG cb
    ) throw (OutOfMemory)
{ 
    void *pvNew = (void *)LocalAlloc(LPTR, cb);

    if (NULL == pvNew)
        throw OutOfMemory();
#ifdef DEBUG_ALLOC
    else
        m_Stats.Add(pvNew, cb);
#endif

    return pvNew;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: FreeStore::Free [inline]

    Description: Frees a block of memory.

    Arguments:
        pv - Pointer to block to remove.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/12/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
inline 
void 
FreeStore::Free(
    void *pv
    )
{ 
    LocalFree(pv); 

#ifdef DEBUG_ALLOC
    m_Stats.Delete(pv);
#endif

}


#endif // __FREE_STORE_H
