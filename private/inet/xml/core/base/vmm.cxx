/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "vmm.hxx"
#ifdef PRFDATA
#include "core/prfdata/msxmlprfcounters.h"
#endif

#if DBG == 1
// for outputting error msgs
#include "core/io/cstdio.hxx"
#endif

/********* class VMManager **********************************************/ 

DeclareTag(tagVMM, "VMManager", "VM Manager/Allocator");
DeclareTag(tagVMMOOM, "VMManager-OOM", "VM Manager/Allocator OutOfMemory");

// TraceTag calls can fail (even for disabled tags) in OOM situations!
#if 1
#define NormalTraceTag(x)
#else
#define NormalTraceTag(x)   TraceTag(x)
#endif

const VMManager::SIZET VMManager::s_cbBlockSize = _VMM_BLOCKSIZE*1024; // 64k
const VMManager::SIZET VMManager::s_uBlockMask = ~(s_cbBlockSize-1); // requires that page size be 2^x
const VMManager::SIZET VMManager::s_cbPageSize = _VMM_PAGESIZE*1024; // 4k
const VMManager::SIZET VMManager::s_uPageMask = ~(s_cbPageSize-1); // requires that block size be 2^x
const VMManager::SIZET VMManager::s_cPagesPerBlock = _VMM_BLOCKSIZE/_VMM_PAGESIZE;

VMManager * s_pDefaultVMM = null;
extern CSMutex * g_pMutex;

void VMManager::getDefaultVMM( VMManager ** ppVMM)
{
    if (!s_pDefaultVMM)
    {
        MutexLock lock(g_pMutex);

        TRY
        {
            if (!s_pDefaultVMM)
                s_pDefaultVMM = new VMManager();
        }
        CATCH
        {
            lock.Release();
            Exception::throwAgain();
        }
        ENDTRY
    }
    s_pDefaultVMM->AddRef();    // caller will have to release it when done
    NormalTraceTag((tagVMM, "getDefaultVMM() => %P", s_pDefaultVMM));
    *ppVMM = s_pDefaultVMM;
}

void VMManager::classExit()
{
    NormalTraceTag((tagVMM, "classExit()"));
    if (s_pDefaultVMM != null)
        s_pDefaultVMM->ReleaseAndDelete();
}

VMManager::VMManager() : SimpleIUnknown(MultiThread)
{
    TraceTag((tagVMM, "new (%P) VMM()", this));
    _pBlocks = null;
    _pFastAllocBlock = null;
    InitializeCriticalSection( &_cs);
#if DBG == 1
    _lBlocks = _lMaxPages = _lTotalPages = _lPagesReused = 0;
#endif
}

VMManager::~VMManager()
{
    // decommit all pages
    EnterCriticalSection( &_cs);
    TraceTag((tagVMM, "delete (%P) VMM", this));

    VMBlock * pBlock;
    while ( pBlock = _pBlocks) {
        RemoveBlockFromChain( pBlock);
        FreeBlock( pBlock);
    }

    LeaveCriticalSection( &_cs);
    DeleteCriticalSection( &_cs);
}

#if DBG == 1
extern bool MemAllocFailTest();
#endif

// Get a new page.
VMBlock * VMManager::AllocBlock()
{
    VMBlock * pBlock = new_ne VMBlock( this);
    if (pBlock)
    {
        void * pBase;
#if DBG == 1
        if (MemAllocFailTest())
            pBase = null;
        else
#endif
        // get a new page
            pBase = VirtualAlloc( NULL,               /* system selects address   */ 
                                     s_cbBlockSize,     /* size of allocation       */ 
                                     MEM_RESERVE,        /* allocates reserved pages */ 
                                     PAGE_NOACCESS);     /* protection = no access   */
        if ( pBase == null) {
            // ReportError();
            // Assert( 0 && "Failed to reserve VM block");
            TraceTag((tagVMM, "VMM::AllocBlock() failed to reserve mem"));
            return null;
        }

#ifdef SPECIAL_OBJECT_ALLOCATION
        addObjectRegion(pBase, s_cbBlockSize); 
#endif // SPECIAL_OBJECT_ALLOCATION

        NormalTraceTag((tagVMM, "VMM::AllocBlock() => %P (Block:%P)", pBase, pBlock));
        pBlock->_pBase = pBase;

    #if DBG == 1
        _lBlocks++;
    #endif
    }

    return pBlock;
}

void VMManager::RemoveBlockFromChain( VMBlock * pBlock)
{
    NormalTraceTag((tagVMM, "VMM::RemoveBlockFromChain(%P)", pBlock));
    Assert( pBlock != _pFastAllocBlock);
    // remove block from chain
    if ( pBlock->_pPrevBlock) {
        pBlock->_pPrevBlock->_pNextBlock = pBlock->_pNextBlock;
        Assert( pBlock != _pBlocks);
    } else { // first on list...
        Assert( pBlock == _pBlocks);
        _pBlocks = pBlock->_pNextBlock;
    }

    if ( pBlock->_pNextBlock)
        pBlock->_pNextBlock->_pPrevBlock = pBlock->_pPrevBlock;

    pBlock->_pPrevBlock = pBlock->_pNextBlock = null;
}

void * VMManager::GrabFreePage( VMBlock * pBlock, long * plOffset, unsigned long * pulMask)
{
    NormalTraceTag((tagVMM, "VMM::GrabFreePage(%P,,)", pBlock));

    if ( !pBlock) 
        return 0;

    long lOffset = 0;
    unsigned long ulMask = 1;
    while ( lOffset < pBlock->_cCommittedPages)
    {
        if ( !( pBlock->_ulUsageMap & ulMask) )
        { // found unused block
            if ( plOffset)
                *plOffset = lOffset;
            if ( pulMask)
                *pulMask = ulMask;

            pBlock->_ulUsageMap |= ulMask; // mark it as used...
            Assert( ((unsigned __int64)pBlock->_ulUsageMap >> pBlock->_cCommittedPages) == 0);
            pBlock->_cFreePages--;

            void * pPage = (void *)((char *)pBlock->_pBase + lOffset*s_cbPageSize);

            NormalTraceTag((tagVMM, "VMM::GrabFreePage(%P,,) [%x|%x] grabbed page %d (%P)", pBlock, pBlock->_ulUsageMap, ulMask, lOffset, pPage));

#if DBG == 1
            _lPagesReused++;
#endif
            return pPage;
        }
        lOffset++;
        ulMask <<= 1;
    }
    return null;
}

void VMManager::ReportError()
{
#if 0
    DWORD dwErr = GetLastError();
#if DBG == 1
    LPVOID lpMsgBuf;
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
    );
    StdIO::getOut()->print(_T("Failed to commit VM page!: "));
    StdIO::getOut()->println((TCHAR *)lpMsgBuf);
    // Assert( 0 && "Failed to commit VM page!");
    // Free the buffer.
    LocalFree( lpMsgBuf );
#endif
#endif
}

void * VMManager::CommitPages( VMBlock * pBlock)
{
#if DBG == 1
    InterlockedIncrement( &_lTotalPages);
#endif
    long cCommitted = pBlock->_cCommittedPages;
    NormalTraceTag((tagVMM, "VMM::CommitPages(%P,) committing page %d", pBlock, cCommitted));
    // mark section to be commited as used (so noone tries to steal it out from under us)
    pBlock->_ulUsageMap |= (1 << cCommitted);
    pBlock->_cCommittedPages++;

    // actually commit it
    void * pPage = (void *)((char *)pBlock->_pBase + cCommitted*s_cbPageSize);

    void * pMem;
#if DBG == 1
    if (MemAllocFailTest())
        pMem = null;
    else
#endif
        pMem = VirtualAlloc( pPage,
                                s_cbPageSize,
                                MEM_COMMIT,
                                PAGE_READWRITE);
    // Assert( pPage == pMem);
    if ( pMem == null) { // failed
        TraceTag((tagVMM, "VMM::CommitPages(%P,) committing page %d failed!", pBlock, cCommitted));
#if DBG == 1
        ReportError();
#endif
        // back out update to bitmap (since it failed!!)

        // this should not happen, but we back it any changes we made, so that things work
        // in retail when memory runs out
        pBlock->_ulUsageMap &= ~(1 << cCommitted);
        pBlock->_cCommittedPages--;
    } 
#ifdef PRFDATA
    else
        PrfCountCommittedPages(s_cbPageSize);
#endif

    Assert( ((unsigned __int64)pBlock->_ulUsageMap >> pBlock->_cCommittedPages) == 0);

    return pMem;
}

// _pBlock is a a block to start looking in first
void * VMManager::Alloc( VMBlock ** ppBlock)
{

    EnterCriticalSection( &_cs);

    VMBlock * pBlock = _pBlocks;

    unsigned long ulMask = 0;
    long lOffset = 0;
    void * pPage = null;

    if ( _pFastAllocBlock &&
         _pFastAllocBlock->_cFreePages > 0 &&
         (pPage = GrabFreePage( _pFastAllocBlock, &lOffset, null)) != 0 ) {

        pBlock = _pFastAllocBlock;
        LeaveCriticalSection( &_cs);
        memset( pPage, 0, s_cbPageSize); // all returned pages must be zero'd

        goto ReturnPage;

    }

    _pFastAllocBlock = null;

    // look for a block with a commited page, which is not used
    while (pBlock && ( pBlock->_cFreePages == 0))
        pBlock = pBlock->_pNextBlock;
    if ( pBlock) { // grab free block
        pPage = GrabFreePage( pBlock, &lOffset, null);
        if ( pPage) {
            LeaveCriticalSection( &_cs);

            memset( pPage, 0, s_cbPageSize); // all returned pages must be zero'd

            goto ReturnPage;
        } else {
            Assert( 0 && "cFreePages count doesn't match usage bitmap!!");
            pPage = null;
            pBlock = null;
            goto ReturnPage;
        }
    }

#if DBG == 1
    _lMaxPages++;
#endif

    // start back at the beginning of the block list looking for a block with uncommited pages
    pBlock = _pBlocks;
    while (pBlock && !( pBlock->_cCommittedPages < s_cPagesPerBlock))
        pBlock = pBlock->_pNextBlock;
    if ( pBlock) {
        pPage = CommitPages( pBlock);
        // previous function does a: LeaveCriticalSection()
        LeaveCriticalSection( &_cs);
        goto ReturnPage;
    }

    LeaveCriticalSection( &_cs);

    // get a new page
    pBlock = AllocBlock();
    if (pBlock)
    {
        // commit the first block on the new page
        pPage = CommitPages( pBlock);

        EnterCriticalSection( &_cs);
        pBlock->_pNextBlock = _pBlocks;
        if ( _pBlocks)
            _pBlocks->_pPrevBlock = pBlock;
        _pBlocks = pBlock;
        LeaveCriticalSection( &_cs);

        // Assert( pPage);
    }

  ReturnPage:
    if ( ppBlock)
        *ppBlock = pBlock;

    NormalTraceTag((tagVMM, "VMM::Alloc() => %P (Block:%P)", pPage, pBlock));

    return pPage;
}

void VMManager::FreeBlock( VMBlock * pBlock)
{
    TraceTag((tagVMM, "VMM::FreeBlock(%P) comitted:%x free:%x", pBlock, pBlock->_cCommittedPages, pBlock->_cFreePages));
    Assert( pBlock);
    Assert( pBlock->_cCommittedPages == pBlock->_cFreePages);
    Assert (pBlock->_pPrevBlock == null &&
            pBlock->_pNextBlock == null);
    Assert( pBlock->_pBase != null);

#ifdef SPECIAL_OBJECT_ALLOCATION
    delObjectRegion(pBlock->_pBase, s_cbBlockSize);
#endif // SPECIAL_OBJECT_ALLOCATION

    long size = (long)(s_cbPageSize * pBlock->_cCommittedPages);
    if (!VirtualFree( pBlock->_pBase, size, MEM_DECOMMIT)) {
#if DBG == 1
        ReportError();
#endif
    }
#ifdef PRFDATA
    else
        PrfCountCommittedPages(-size);
#endif

    if (!VirtualFree( pBlock->_pBase, 0, MEM_RELEASE)) {
#if DBG == 1
        ReportError();
#endif
    }
#if DBG == 1
    memset( (void *)pBlock, 0, sizeof( VMBlock));
#endif
    delete pBlock;
}

void VMManager::Free( void * pPage, VMBlock * _pBlock)
{
    VMBlock * pBlock = _pBlock;
    EnterCriticalSection( &_cs);
    if ( ! pBlock) {
        pBlock = _pBlocks;
        void * pTmp = (void *)((char *)pPage - s_cbBlockSize);
        // for pPage to be in a given block, the block's base pointer 
        //  must be between pTmp and pPage.
        while ( pBlock && ((pPage < pBlock->_pBase) || (pBlock->_pBase < pTmp)) )
            pBlock = pBlock->_pNextBlock;
        Assert( pBlock);
    }

    long index = (long)(((char *)pPage - (char *)pBlock->_pBase) / s_cbPageSize);
    Assert( index < pBlock->_cCommittedPages);
    Assert( (((char *)pBlock->_pBase) + s_cbPageSize * index) == (char *)pPage );
    Assert( (pBlock->_ulUsageMap & (1 << index)) != 0 );

    NormalTraceTag((tagVMM, "VMM::Free(%P, %P) page: %d", pPage, pBlock, index));

    pBlock->_ulUsageMap &= ~(1 << index); // mark as free
    Assert( ((unsigned __int64)pBlock->_ulUsageMap >> pBlock->_cCommittedPages) == 0);
    pBlock->_cFreePages++;

    // to help avoid walking the block chain to find free spaces
    if ( !_pFastAllocBlock ||
         _pFastAllocBlock->_cFreePages < pBlock->_cFreePages)
        _pFastAllocBlock = pBlock;

    NormalTraceTag((tagVMM, "VMM::Free(%P, %P) [%x] freePages:%d committedPages: %d", pPage, pBlock, pBlock->_ulUsageMap, pBlock->_cFreePages, pBlock->_cCommittedPages));

    if ( pBlock->_cFreePages == pBlock->_cCommittedPages) {
        if ( _pFastAllocBlock == pBlock)
            _pFastAllocBlock = null;
        RemoveBlockFromChain( pBlock);
        LeaveCriticalSection( &_cs);
        FreeBlock( pBlock);
    } else {
        LeaveCriticalSection( &_cs);
    }
}

#if DBG == 1
bool VMManager::ValidatePagePtr( void * pv)
{
    EnterCriticalSection( &_cs);

    VMBlock * pBlock = _pBlocks;
    void * pTmp = (void *)((char *)pv - s_cbBlockSize);
    // for pPage to be in a given block, the block's base pointer 
    //  must be between pTmp and pPage.
    while ( pBlock && ((pv < pBlock->_pBase) || (pBlock->_pBase < pTmp)) )
        pBlock = pBlock->_pNextBlock;

    LeaveCriticalSection( &_cs);

    return (pBlock != null);
}
#endif
