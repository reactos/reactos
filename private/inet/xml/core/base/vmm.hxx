/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _CORE_BASE_VMM
#define _CORE_BASE_VMM

/*****************************************************
 *************** Virtual Memory Manager *************/

#define _VMM_BLOCKSIZE 128 // 64k

#if defined(UNIX) && defined(ux10)
#define _VMM_PAGESIZE 4 // 4k
#else
#define _VMM_PAGESIZE 8 // 8k
#endif

// there must never be more than 32 pages per block, 
// because we use a 32bit word for our usage map!
#define _VMM_PAGES_PER_BLOCK  (_VMM_BLOCKSIZE / _VMM_PAGESIZE)

class VMManager;

struct VMBlock
{
public:
    VMBlock( VMManager * pVMM) 
        : _pBase( null), _pPrevBlock( null), _pNextBlock( null),
          _cCommittedPages( 0), _cFreePages( 0), _ulUsageMap( 0), _pVMM( pVMM)
    {}

public:
    void * _pBase;   // pointer to "base" of memory block
    VMBlock * _pPrevBlock, * _pNextBlock;
    long _cCommittedPages;   // number of (sequentially) allocated pages (from this block)
    long _cFreePages;    // number of allocated, but unused, pages
    unsigned long _ulUsageMap;   // bitmap of which pages are used
    VMManager * _pVMM;   // pointer to the VMManager, for deletes
};

DEFINE_CLASS(VMManager);

class VMManager : public SimpleIUnknown
{
public:
#ifdef _ALPHA_
typedef unsigned __int64 SIZET;
#else
typedef unsigned __int32 SIZET;
#endif

public:
    // constants
    static const SIZET s_cbBlockSize;      // BlockSIZE*1024;
    static const SIZET s_uBlockMask;      // ~(cbBlockSize-1); // requires that Block size be 2^x
    static const SIZET s_cbPageSize;     // PAGESIZE*1024;
    static const SIZET s_uPageMask;      // ~(cbPageSize-1); // requires that block size be 2^x
    static const SIZET s_cPagesPerBlock;  // BLOCKSIZE/PAGESIZE

    VMManager();

    static void getDefaultVMM( VMManager **);

    static void classExit();

protected:
    ~VMManager();

public:
    // Get a new page.
    //  on return *_pBlock points to the VMBlock object which manages
    //      the returned page. (for use by Free())
	void * Alloc( /* [out] */ VMBlock ** ppBlock = null);

    // return a block.  if the block is given, it is _much_ faster...
    void Free( void * _pPage, VMBlock * pBlock = null);

    static void s_Free( void * pPage, VMBlock * pBlock) 
    {
        pBlock->_pVMM->Free( pPage, pBlock);
    }

    static inline void * PAGE(void * p) { return (void *)((SIZET)p & s_uPageMask); }

#if DBG == 1
    bool ValidatePagePtr( void *);

    long _lBlocks;       // (total) number of blocks allocted
    long _lMaxPages;     // number of calls to Alloc()
    long _lPagesReused;  // number of previously committed pages which were reused
#endif
#if DBG == 1
    long _lTotalPages;   // number of pages currently committed
#endif

protected:
    // Get a new Block.
	VMBlock * AllocBlock();

    // return a Block.
	void FreeBlock( VMBlock *);

    void * GrabFreePage( VMBlock * pBlock, long * plOffset, unsigned long * pulMask);
    void * CommitPages( VMBlock * pBlock);
    void RemoveBlockFromChain( VMBlock * pBlock);

    void ReportError();

private:
	CRITICAL_SECTION _cs;

    VMBlock * _pFastAllocBlock;
	VMBlock * _pBlocks;
};


#endif // _CORE_BASE_VMM
