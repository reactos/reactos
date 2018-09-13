// File memmgr.cpp
#define OLDNEW
#include "stdafx.h"

#undef MYnew
#undef new

#if defined(_DEBUG)

#include "assert.h"
#include "memmgr.hpp"
#include "memblk.hpp"
#include <time.h>


CMemMgr::CMemMgr()
{
    _uTotalAllocNum = 0l;
    _uMaxAllocNum = 0l;
    _uTotalAllocSize = 0l;
    _uTotalRequestedSize = 0l;
    _uTotalUsedSize = 0l;
    _uTotalMaxRequestedSize = 0l;
    _uMaxAllocBlockSize = 0l;
    _uMinAllocBlockSize = LONG_MAX; 
    _uTotalNewCalls = 0l;
    srand( (unsigned)time( NULL ));
}



void *CMemMgr::AllocMem(UINT uSize, LPCSTR szFile, UINT uLine)
{
    CMemBlk *pCurBlk;
    void    *pMem;


    // Check if we have lots of memory used, free the released memory
    if(_uTotalUsedSize > COMPACTMEMORYTHRESHOLD)
    {
        ULONG uReleased = _MemPtrList.ReleaseFreeMemBlocks();
        char  szBuf[100];

        sprintf(szBuf, "   MEMMGR TRACE: Compacting memory, %lu bytes released\r\n", uReleased);
        OutputDebugStringA(szBuf);

        _uTotalUsedSize -= uReleased;
    }

    // Allocate the class
    pCurBlk = new CMemBlk(uSize, szFile, uLine);
    if(!pCurBlk)
        return NULL;

    // Get the external memory address from the class
    pMem = pCurBlk->GetAddress();

    // Add the class to the list of memory blocks
    _MemPtrList.Add(pCurBlk);

    // Update the statistics
    //************************

    // Increment number of allocated blocks
    _uTotalNewCalls++;

    _uTotalAllocNum++;
    if(_uTotalAllocNum > _uMaxAllocNum)
        _uMaxAllocNum = _uTotalAllocNum;

    // Increment the size of allocated blocks (includin padding)
    _uTotalAllocSize += pCurBlk->GetRealSize();
    _uTotalUsedSize  += pCurBlk->GetRealSize();

    // Increment total requested size
    _uTotalRequestedSize += uSize;

    // Keep the larges amoun of memory used value
    if(_uTotalMaxRequestedSize < _uTotalRequestedSize)
    {
        _uTotalMaxRequestedSize = _uTotalRequestedSize;
    }

    // Largest single block allocated
    if(_uMaxAllocBlockSize < uSize)
        _uMaxAllocBlockSize = uSize;
    
    // Smallest single block allocated
    if(_uMinAllocBlockSize > uSize) 
        _uMinAllocBlockSize = uSize;
    return pMem;
}



void CMemMgr::FreeMem(void *pMem)
{
    CMemBlk *pMemBlk;

    if(pMem == NULL)
        return;

    pMemBlk = CMemBlk::GetCMemBlockFromMemory(pMem);
    assert(!IsBadCodePtr((FARPROC)pMemBlk));
	if(IsBadCodePtr((FARPROC)pMemBlk))
    {
        OutputDebugString(_T("MEMMGR ERROR: The memory is corrupt, or the pointer is not allocated by memory manager\r\n"));
		// Try to search in the list
		pMemBlk = _MemPtrList.FindMemBlk(CMemBlk::GetCMemBlockFromMemory(pMem));
		if(pMemBlk == NULL)
		{
			free(pMem);
			return;
		}
    }

    if(!KEEPRELEASEDMEMORY)
    {
        if(!_MemPtrList.Remove(pMemBlk))
        {
            OutputDebugString(_T("MEMMGR ERROR: Releasing a memory that was already released:\r\n"));
            pMemBlk->Dump();
            pMemBlk->~CMemBlk();
            free(pMemBlk);
            return;
        }
    }

    if(pMemBlk->IsFree())
    {
        OutputDebugString(_T("MEMMGR ERROR: Releasing a memory that was already released:\r\n"));
        pMemBlk->Dump();
        return;
    }

    _uTotalAllocNum--;
    _uTotalAllocSize -= pMemBlk->GetRealSize();
    _uTotalRequestedSize -= pMemBlk->GetSize();

    pMemBlk->FreeMem();
    if(!KEEPRELEASEDMEMORY)
    {
        // I should have used structures instead of classes, when I use delete
        //  operator my deleted is called. So I try to free the block myself
        // hoping that free() and delete use the same allocator
        //  (Dont do this at home)
        pMemBlk->~CMemBlk();
        free(pMemBlk);
    }
};

CMemMgr::~CMemMgr()
{
    CMemBlk *pMemBlk;

    // TODO dump all the leaked memory blocks
    if(_uTotalAllocNum != 0)
    {
        char szBuf[150];
        sprintf(szBuf, "\r\nMEMMGR ERROR: %d MEMORY LEAK(S) DETECTED\r\n*****************************************************\r\n",
            _uTotalAllocNum);
        OutputDebugStringA(szBuf);
        lstrcatA(szBuf, "Do you want to debug?");
        int nRet = MessageBoxA(NULL, szBuf, "Memory leaks", MB_YESNO);
        while((pMemBlk = _MemPtrList.PopNext()) != 0)
        {
            // I should have used structures instead of classes, when I use delete
            //  operator my deleted is called. So I try to free the block myself
            // hoping that free() and delete use the same allocator
            //  (Dont do this at home)
            pMemBlk->~CMemBlk();
            free(pMemBlk);
        }
        if(nRet == IDYES)
        {
           #ifdef _M_IX86
              _asm int 3;
           #else
               DebugBreak();
           #endif // _M_IX86
        }
    }
    DumpStatistics();
}


void CMemMgr::DumpStatistics(void)
{
    char szBuf[120];
    OutputDebugString(_T("\r\n\r\nMemory Usage Statistics\r\n***********************\r\n"));
    sprintf(szBuf, "Peak number of  memory blocks -  %lu\r\n", _uMaxAllocNum);
    OutputDebugStringA(szBuf);
    sprintf(szBuf, "Peak size of  memory blocks -  %lu\r\n", _uTotalMaxRequestedSize);
    OutputDebugStringA(szBuf);
    if(_uTotalRequestedSize > 0)
    {
        assert(_uTotalAllocNum > 0);
        sprintf(szBuf, "Number of leaked memory blocks -  %lu\r\n", _uTotalAllocNum);
        OutputDebugStringA(szBuf);
        sprintf(szBuf, "Total size of leaked memory blocks -  %lu\r\n", _uTotalRequestedSize);
        OutputDebugStringA(szBuf);
    }

    sprintf(szBuf, "Largest memory block allocated -  %lu\r\n", _uMaxAllocBlockSize);
    OutputDebugStringA(szBuf);
    if(_uMinAllocBlockSize != LONG_MAX)
    {
        sprintf(szBuf, "Smallest memory block allocated -  %lu\r\n", _uMinAllocBlockSize);
        OutputDebugStringA(szBuf);
    }
    
    sprintf(szBuf, "Number of times memory was allocated -  %lu\r\n***********************\r\n\r\n", _uTotalNewCalls);
    OutputDebugStringA(szBuf);

}



CMemMgr gMemMgr;


CMemBlkList::CMemBlkList()
{
    _pHead = NULL;
}

BOOL CMemBlkList::Add(CMemBlk *pBlk)
{
    MemBlkListElem *pNew;

    pNew = (MemBlkListElem *)malloc(sizeof(*pNew));
    if(pNew == NULL)
    {
        assert(pNew != 0);
        return FALSE;
    }

    pNew->pMemBlk = pBlk;

    pNew->_pNext = _pHead;
    _pHead = pNew;

    return TRUE;
}


CMemBlkList::~CMemBlkList()
{
    MemBlkListElem *pCur;

    while(_pHead != NULL)
    {
        pCur = _pHead;
        _pHead = _pHead->_pNext;
        free(pCur);
    }
}

BOOL CMemBlkList::Remove(CMemBlk *pBlk)
{
    MemBlkListElem *pCur, *pPrev;
    
    if(!_pHead)
        return FALSE;

    pPrev = _pHead;
    while(pPrev != NULL)
    {
        pCur = pPrev->_pNext;
        if(pCur == NULL)
            break;
        if(pCur->pMemBlk->GetAddress() == pBlk)
        {
            // Found, remove it from the list
            pPrev->_pNext = pCur->_pNext;
            free(pCur);
            return TRUE;
        }
        pPrev = pCur;
    }

    return FALSE;
}


ULONG CMemBlkList::ReleaseFreeMemBlocks()
{
    MemBlkListElem *pCur, *pPrev;
    ULONG uSize = 0l;
    
    if(_pHead == NULL)
        return FALSE;

    pPrev = _pHead;
    
    while(pPrev != NULL)
    {
        pCur = pPrev->_pNext;
        if(pCur == NULL)
            break;
        if(pCur->pMemBlk->IsFree())
        {
            // Found, remove it from the list
            pPrev->_pNext = pCur->_pNext;
            uSize += pCur->pMemBlk->GetRealSize();
            pCur->pMemBlk->~CMemBlk();
            free(pCur->pMemBlk);
            free(pCur);
            continue;
        }
        pPrev = pCur;
    }

    return uSize;
}

CMemBlk *CMemBlkList::PopNext()
{
    MemBlkListElem *pCur;
    CMemBlk *pRet;

    if(_pHead == NULL)
        return NULL;

    pCur = _pHead;
    pRet = pCur->pMemBlk;
    _pHead = _pHead->_pNext;
    free(pCur);
    return pRet;
}

// Find the memory block class given memory pointer
CMemBlk *CMemBlkList::FindMemBlk(void *pMem)
{
    MemBlkListElem *pCur;
	
	if(pMem == NULL || _pHead == NULL)
		return NULL;

    
    pCur = _pHead;

    while(pCur != NULL)
    {
        if(pCur->pMemBlk->GetAddress() == pMem)
        {
            return pCur->pMemBlk;
        }
        pCur = pCur->_pNext;
    }

	return NULL;
}


#endif
