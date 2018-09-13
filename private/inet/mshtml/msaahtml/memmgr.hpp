// memmgr.hpp
#if !defined(MEMMGR_HPP_INCLUDED)
#define MEMMGR_HPP_INCLUDED
#if defined(_DEBUG)

#include "windows.h"
#include "limits.h"



class CMemBlk;


typedef struct tagMemBlkListElem
{
    CMemBlk *pMemBlk;
    struct tagMemBlkListElem * _pNext;
} MemBlkListElem;


class CMemBlkList
{
public:
    CMemBlkList();
    ~CMemBlkList();
    BOOL Add(CMemBlk *pBlk);
    BOOL Remove(CMemBlk *pBlk);
    CMemBlk *PopNext();
    ULONG ReleaseFreeMemBlocks();
	// Find the memory block class given memory pointer
	CMemBlk *FindMemBlk(void *pMem);
private:
    MemBlkListElem *_pHead;
};



class CMemMgr
{
public:
    CMemMgr();
    ~CMemMgr();
    void *AllocMem(UINT uSize, LPCSTR szFile, UINT uLine);
    void FreeMem(void *pMem);

private:
    CMemBlkList    _MemPtrList;                     // List of memory block classes

    ULONG          _uTotalAllocNum;                 // Total number of memory allocations
    ULONG          _uMaxAllocNum;                   //
    ULONG          _uTotalUsedSize;                 // memory currently used by us (allocated of free)
    ULONG          _uTotalAllocSize;                // Total memory used (including pading)
    ULONG          _uTotalRequestedSize;            // Total memory requested
    ULONG          _uTotalNewCalls;                 // Total number of memory allocations

    ULONG          _uTotalMaxRequestedSize;         // Total maximum memory requested
    ULONG          _uMaxAllocBlockSize;             // Largest memory block allocated
    ULONG          _uMinAllocBlockSize;             // Smallest block allocated

    void DumpStatistics();
    ULONG ReleaseFreememBlocks();
};

#endif
#endif
