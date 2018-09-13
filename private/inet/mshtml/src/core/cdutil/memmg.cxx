/*
	History
	07/01/94        Lajosf  Write
	07/08/94        Lajosf  Introducing DEBUGHEADERSIZE constant
	07/11/94        Lajosf  Preparing multiple Address Spaces (just handle it, but do not allocate)
	12/29/94                Lajosf  Making memory exception conditional (by USE_EXCEPTION)
*/

#define __MEMMG__CXX__  1

#include "headers.hxx"
EXTERN_C HANDLE g_hProcessHeap;

#if 0 // change to #if 1 to use slot manager

#if DBG == 1
BOOL    g_fHeapOpAllowed = TRUE;
#endif

#undef MEM_DEBUG
//#define MEM_DEBUG 1

#ifdef MEM_DEBUG
    #include <tchar.h>
    #include <fstream.h>
#endif

static int              const s_kFIRSTSIZE              = 16;   // smallest slotsize
static int              const s_kSIZEDELTA              = 16;   // difference between two slotsizes
static int              const s_kLASTSIZE               = 1024; // biggest slotsize
static BYTE             const s_kCHECKVALUE             = 0xF1; // value before and after the user area
static DWORD                    const s_kDEFMEMFORPOOL  = 0x00100000;   // try to allocate this number for 1 pool
static DWORD                    const s_kMINMEMFORPOOL  = 0x00010000;   // min. mem. needed for 1 pool
static DWORD                    const s_kTRUNCATION             = 0x000FFFFF;   // add this value to the address space
static DWORD                    const s_kTRUNCATIONMASK = 0xFFF00000;   // use this to make address space to
														// begin on a particular boundary

static DWORD                    const s_kADDRESSSPACEFORPOOL    = 0x00100000;   
static int              const s_kNRPOOL                                 = s_kLASTSIZE / s_kSIZEDELTA;   // number of pools
static DWORD                    const s_kADDRESSSPACESIZE               = s_kADDRESSSPACEFORPOOL * s_kNRPOOL;
static DWORD                    const s_kCOMMITSIZE                             = 0x00008000;   // commit block size
static DWORD                    const s_kNRCOMMITBLOCKS                 = (s_kADDRESSSPACEFORPOOL / s_kCOMMITSIZE);
static DWORD                    const s_kNRCOMMITLONGS                  = ((s_kNRCOMMITBLOCKS / 4 / 8) + 1);
static DWORD                    const s_kMAXADDRESSSPACES               = (2048/((s_kADDRESSSPACESIZE+1)/0x00100000));
static DWORD                    const s_kTOTALADDRESSSPACE              = 2048; // 2 gigabytes 

#ifdef MEM_DEBUG
#       define MAXFILENAME     32
typedef enum states {state_free,state_deleted,state_allocated};
typedef struct sMemDescr {
	int                     iSize;
	TCHAR                   chFileName[MAXFILENAME];
	int                     iLineNum;
	DWORD                   dwAllocCounter;
	states                  stState;
} sMemDescr;
#       define DEBUGHEADERSIZE sizeof(sMemDescr)
#else
#       define DEBUGHEADERSIZE 0
#endif


typedef struct {
	int                     iNrCommitted;
	int                     iSize;
	int                     iFreeBlockIndex;
	BYTE                            *apchFirstFree[s_kNRCOMMITBLOCKS];      
#ifdef MEM_DEBUG
	ULONG                           ulAllocCounter;
	ULONG                           ulDeleteCounter;
#endif
} sPoolDescr;

typedef struct {
	sPoolDescr              PoolDescr[s_kNRPOOL];   
	BYTE*                   pchAddressSpace;
	BYTE*                   pchAddressSpaceMasked;          // truncated to "Truncation" boundary
} sAddressSpaceDescr;


static DWORD                    s_dwPageSize;
static DWORD                    s_dwMemForPool;
static DWORD                    s_dwTruncation;
static DWORD                    s_dwTruncationMask;                             // mask for truncation to "Truncation" boundary
static BOOL                     s_bInitialized=FALSE;
static DWORD                    s_dwTotalAllocCounter=0;
static int                      s_iNrAddressSpaces=0;
static int                      s_iFreeAddressSpace=-1;
static DWORD                    s_dwRequestsPassedToSystem=0;
static UINT                     s_uiSmallestPassedToSystem=0xFFFF;
static UINT                     s_uiBiggestPassedToSystem=0;
static DWORD                    s_dwTotalPassedToSystem=0;
static DWORD                    s_dwTotalRequests=0;
static DWORD                    s_dwTotalMemoryRequests=0;

static  sAddressSpaceDescr      *s_apasdAddressSpaces[s_kMAXADDRESSSPACES];
static  BYTE                    s_uchAddressSpaceIndexes[s_kTOTALADDRESSSPACE];

/*----------------------------------------------------------------------------------------------------
	Name                            InitNewAddressSpace

	Synopsis                        Initializes a newly reserved address space

	Effects                         Sets the s_uchAddressSpaceIndexes corresponding elements to the index number
									of the new address space

	Arguments                       [pASD]                                  Pointer to the new address space descriptor
									[iAddressSpaceIndex]    Index number of the new Address Space

	Requires                        Nothing

	Returns                         Nothing

	Signals                         Nothing

	Modifies                        Certain elements of s_uchAddressSpaceIndexes and members of the
									sAddressSpaceDescr structure.

	Derivation                      None

	Algorithm                       --

	History                         07/11/94        Lajosf  Create

	Notes                           None

----------------------------------------------------------------------------------------------------*/
void InitNewAddressSpace(sAddressSpaceDescr *pASD,int iAddressSpaceIndex)
{
	DWORD   dwPoolStart;
	DWORD   dwAddressSpaceSize;

	// truncate the start address of the address space to s_dwTruncation;
	pASD->pchAddressSpaceMasked = pASD->pchAddressSpace + s_dwTruncation;
	pASD->pchAddressSpaceMasked = (BYTE*)((DWORD)pASD->pchAddressSpaceMasked & s_dwTruncationMask);

	// set the corresponding elements of the s_uchAddressSpaceIndexes array to the new 
	// address space index
	dwAddressSpaceSize = s_kNRPOOL * s_dwMemForPool;
	dwAddressSpaceSize = dwAddressSpaceSize >> 20;
	dwPoolStart = (DWORD)pASD->pchAddressSpaceMasked;
	dwPoolStart = dwPoolStart >> 20;
	for (DWORD i=dwPoolStart;i<dwPoolStart+dwAddressSpaceSize;i++) 
	  s_uchAddressSpaceIndexes[i] = iAddressSpaceIndex;     

	// increment the address space counter
	s_iNrAddressSpaces++;

	// set the s_iFreeAddressSpace index to the number of the new Address Space
	s_iFreeAddressSpace = iAddressSpaceIndex;
};





/*----------------------------------------------------------------------------------------------------
	Name                            MemInit

	Synopsis                        Init

	Effects                         Inititalizes static variables, and reserves address space

	Arguments                       None

	Requires                        Nothing

	Returns                         Nothing

	Signals                         Memory exception

	Modifies                        All elements of PoolDescr array

	Derivation                      None

	Algorithm                       --

	History                         07/01/94        Lajosf  Create

	Notes                           Instead of returning error code it throws a memory exception

----------------------------------------------------------------------------------------------------*/
void MemInit(void)
{
	sPoolDescr                      *pAD;
	UINT                            i,j;    
	int                                     iSize;
	SYSTEM_INFO                     sysInfo;
	DWORD                           dwAllMem;
	DWORD                           dwTruncation;
	sAddressSpaceDescr      *pASD;

	s_bInitialized = FALSE; // if anything goes wrong it will remain FALSE

	GetSystemInfo(&sysInfo);
	s_dwPageSize = sysInfo.dwPageSize;      // for later use

	// init address space descriptors to be NULL
	for (i = 0;i < s_kMAXADDRESSSPACES;i++)
		{
		s_apasdAddressSpaces[i] = NULL;
		};

	// init address space indexes to be 0xFF
	for (i = 0;i < s_kTOTALADDRESSSPACE;i++)
		{
		s_uchAddressSpaceIndexes[i] = 0xFF;
		};

	// Allocate memory for the 0th address space descriptor
	HGLOBAL hlgl;
	hlgl = GlobalAlloc(GMEM_MOVEABLE,sizeof(sAddressSpaceDescr));
	if (hlgl == NULL)
#ifdef USE_EXCEPTION
		throw CMemoryException();
#else
		return;
#endif
	s_apasdAddressSpaces[0] = (sAddressSpaceDescr*)GlobalLock(hlgl); 
	pASD = s_apasdAddressSpaces[0];
	iSize = s_kFIRSTSIZE;

	// init the pool description array for the first (0) address space
	for (i=0,pAD=&pASD->PoolDescr[0];i<s_kNRPOOL;i++,pAD++)
		{
#ifdef MEM_DEBUG
		// in the debug version every slot must contain a structure called sMemDescr
		// for debug informations and 2 checkbytes, 1 before the slot, and 1 after the
		// slot, containing 0xF1        
		pAD->iSize                                      = iSize+sizeof(sMemDescr)+2;
		pAD->ulAllocCounter                     = 0;
		pAD->ulDeleteCounter            = 0;
#else
		pAD->iSize                                      = iSize;
#endif
		pAD->iNrCommitted                       = 0;
		pAD->iFreeBlockIndex            = -1;

		// init the first free pointer for every commit blocks
		for (j=0;j<s_kNRCOMMITBLOCKS;j++)
			{
			pAD->apchFirstFree[j] = NULL;
			}

		iSize += s_kSIZEDELTA;
		};
	s_dwMemForPool = s_kDEFMEMFORPOOL;
	pASD->pchAddressSpace = NULL;
	dwTruncation = s_kTRUNCATION;
	s_dwTruncationMask = s_kTRUNCATIONMASK;

	// Now we try to reserve s_kNRPOOL(32) * DEFMEMFORPOOL(1 meg) + 1 meg
	// if not successful we divide the memory for 1 pool by 16 and try again
	// the cycle ends when the reservation is successful or the memory for
	// pool decreases below MINMEMFORPOOL (64K)
	while ( (pASD->pchAddressSpace == NULL) && (s_dwMemForPool >= s_kMINMEMFORPOOL))
		{
		dwAllMem = s_kNRPOOL * s_dwMemForPool + dwTruncation;
		pASD->pchAddressSpace = (BYTE*)VirtualAlloc(NULL,dwAllMem,MEM_RESERVE,PAGE_READWRITE);
		if (pASD->pchAddressSpace == NULL)
			{
			s_dwMemForPool >>= 4;
			dwTruncation >>= 4;
			s_dwTruncationMask = dwTruncation;
			s_dwTruncationMask = ~s_dwTruncationMask;
			};
		};
	if (pASD->pchAddressSpace == NULL)
		{
#ifdef USE_EXCEPTION
		throw CMemoryException();
#endif
		return;
		};
	// Now we got an addressspace + 1 meg, therefore we can mask the address to 1 meg boundary
	// and still we can be sure we have enough space
	s_dwTruncation = dwTruncation;
	InitNewAddressSpace(pASD,0);
	s_bInitialized = TRUE;
};





/*----------------------------------------------------------------------------------------------------
	Name                            MemTerm

	Synopsis                        Release allocated memory

	Effects                         elements of m_AllocDescr array will be NULL

	Arguments                       None

	Requires                        Nothing

	Returns                         Nothing

	Signals                         Memory exception

	Modifies                        pchAddressSpace

	Derivation                      None

	Algorithm                       --

	History                         07/01/94        Lajosf  Create

	Notes                           Instead of returning error code it throws a memory exception

----------------------------------------------------------------------------------------------------*/
void MemTerm(void)
{
	int i;

	for (i=0;i<s_iNrAddressSpaces;i++)
		{
		if (s_apasdAddressSpaces[i] != NULL)
			{
			if (s_apasdAddressSpaces[i]->pchAddressSpace != NULL)
				{
				if (!VirtualFree(s_apasdAddressSpaces[i]->pchAddressSpace,0,MEM_RELEASE))
					{
#ifdef MEM_DEBUG
					// only in the debug version send error message
					DWORD   dErrorCode;
					TCHAR   chBuffer[80];
					dErrorCode = GetLastError();
					wsprintf((TCHAR*)&chBuffer,_T("Virtual Free error code : %x"),dErrorCode);
					MessageBox(NULL, (TCHAR*)&chBuffer, NULL, MB_OK);
#endif
#ifdef USE_EXCEPTION
					throw CMemoryException();
#else
					return;
#endif
					};
				s_apasdAddressSpaces[i]->pchAddressSpace = NULL;
				};
			HGLOBAL hlgl;
			hlgl = GlobalHandle((void*)s_apasdAddressSpaces[i]);
			if (hlgl == NULL)
				{
#ifdef USE_EXCEPTION
				throw CMemoryException();
#else
				return;
#endif
				};
			GlobalUnlock(hlgl);
			GlobalFree(hlgl);
			};
		};
	s_bInitialized = FALSE;
};





/*----------------------------------------------------------------------------------------------------
	Name                            PrepareFreeList

	Synopsis                        Initializes a newly allocated block

	Effects                         Divide the block N part each iSize size. First four bytes of every part
						contains pointer to the next part. Last contains NULL.

	Arguments                       [N]                     Number of parts in block
						[iSize]         Size of one part
						[pBegin]        points to the beginning of the block    

	Requires                        Block should be committed

	Returns                         Nothing

	Signals                         Assert if pBegin == NULL OR 
								  N < 1          OR
								  iSize < 1

	Modifies                        Contents of the block

	Derivation                      None

	Algorithm                       --

	History                         07/01/94        Lajosf  Create

	Notes                           None

----------------------------------------------------------------------------------------------------*/
void PrepareFreeList(int N,int iSize,BYTE *pchBegin)
{
	Assert(pchBegin != NULL);
	Assert(N > 0);
	Assert(iSize > 0);

	// Slots should point to the next slot
#ifdef MEM_DEBUG
	BYTE                    *pchWork;
	int                     i;
	LONG                    *plLong;


	for (i=0,pchWork=pchBegin;i<N-1;i++,pchWork+=iSize)
		{
		sMemDescr               *pMD;
		pMD = (sMemDescr*)pchWork;
		pMD->stState = state_free;

		plLong = (LONG*)(pchWork + DEBUGHEADERSIZE);
		*plLong = (LONG)(pchWork+iSize);
		};
	// Last slot should point to nowhere
	plLong = (LONG*)(pchWork+DEBUGHEADERSIZE);
	*plLong = NULL;
#else
__asm
	{
	mov             ecx,N
	dec             ecx
	mov             edi,pchBegin
	mov             eax,edi
	mov             edx,iSize
	add             eax,edx
	}
LoopStart:
__asm
	{
	stosd
	mov             edi,eax
	add                         eax,edx
	loop                    LoopStart
	xor             eax,eax
	stosd
	}
#endif
};




/*----------------------------------------------------------------------------------------------------
	Name                            FindFreeBlock

	Synopsis                        Gives back the iFreeBlockIndex

	Effects                         None

	Arguments                       [pAD]           pointer to the pool description structure

	Requires                        Nothing

	Returns                         pAD->iFreeBlockIndex

	Signals                         Assert if pAD == NULL

	Modifies                        Nothing

	Derivation                      None

	Algorithm                       --

	History                         07/01/94        Lajosf  Create

	Notes                           None

----------------------------------------------------------------------------------------------------*/
inline int FindFreeBlock(sPoolDescr *pAD)
{
	Assert(pAD != NULL);

	return pAD->iFreeBlockIndex;
};


/*----------------------------------------------------------------------------------------------------
	Name                            SetPageAsNotFree

	Synopsis                        iBlockIndex block no longer contains free slots. If it is the same
						as the iFreeBlockIndex in the PoolDescription structure then we
						should find another free block (if any)

	Effects                         None


	Arguments                       [pAD]                   pointer to the pool description structure
						[iBlockIndex]   index of block to be set as totally allocated

	Requires                        Nothing

	Returns                         Nothing

	Signals                         Assert if pAD == NULL or
								iBlockIndex outside of the valid range

	Modifies                        iFreeBlockIndex

	Derivation                      None

	Algorithm                       --

	History                         07/01/94        Lajosf  Create

	Notes                           It looks slow. If we are unfortunate we should run two cycles to
						find the new free block. It is true, but this procedure return 
						immediately without doing anything if the iBlockIndex not the same
						as iFreeBlockIndex. The are the same only when the user allocates
						the very last slot in a block;
						Procedure has three return point !
----------------------------------------------------------------------------------------------------*/
void SetPageAsNotFree(sPoolDescr *pAD,int iBlockIndex)
{
	Assert(pAD != NULL);
	Assert(iBlockIndex >= 0);
	Assert(iBlockIndex <= pAD->iNrCommitted-1);

	if (iBlockIndex == pAD->iFreeBlockIndex)
		{
		int             i;

		for (i=0;i<pAD->iFreeBlockIndex;i++)
			{
			if (pAD->apchFirstFree[i] != NULL)
				{
				pAD->iFreeBlockIndex = i;
				return;
				};
			};
		for (i=pAD->iFreeBlockIndex+1;i<pAD->iNrCommitted;i++)
			{
			if (pAD->apchFirstFree[i] != NULL)
				{
				pAD->iFreeBlockIndex = i;
				return;
				};
			};
		pAD->iFreeBlockIndex = -1;
	};
};


/*----------------------------------------------------------------------------------------------------
	Name                            SetPageAsFree

	Synopsis                        iBlockIndex block contains at least 1 free slot.
						If we have any other free block then the procedure do nothing.
						If there was no free block then the procedure sets iFreeBlockIndex
						to be equal the block just freed;
							
	Effects                         None


	Arguments                       [pAD]                   pointer to the pool description structure
						[iBlockIndex]   index of block to be set as free

	Requires                        Nothing

	Returns                         Nothing

	Signals                         Assert if pAD == NULL or
								iBlockIndex outside of the valid range

	Modifies                        iFreeBlockIndex

	Derivation                      None

	Algorithm                       --

	History                         07/01/94        Lajosf  Create

	Notes                           None

----------------------------------------------------------------------------------------------------*/
inline void SetPageAsFree(sPoolDescr *pAD,int iBlockIndex)
{
	Assert(pAD != NULL);
	Assert(iBlockIndex >= 0);
	Assert(iBlockIndex <= pAD->iNrCommitted-1);

	if (pAD->iFreeBlockIndex == -1)
		pAD->iFreeBlockIndex = iBlockIndex;
};


/*----------------------------------------------------------------------------------------------------
	Name                            CommitANewBlock

	Synopsis                        It commites a new block

	Effects                         A new block for a given pool will be committed

	Arguments                       [pPD]                                   PoolDescription
						[iAddressSpaceIndex]    Index to the address space we want to commit into
						[iPoolIndex]                    Index to the pool we want to commit into
						[piFreeBlockIndex]              Index of the newly committed block

	Requires                        Nothing

	Returns                         Pointer to the newly committed block

	Signals                         Assert if pPD == NULL
							   if iAddressSpaceIndex < 0
							   if iPoolIndex < 0
							   if piFreeBlockIndex == NULL
						Memory exception if memory couldn't be committed

	Modifies                        Number of committed blocks

	Derivation                      None

	Algorithm                       VirtualAlloc(...,MEM_COMMIT,PAGE_READWRITE);

	History                         07/11/94        Lajosf  Create

	Notes                           None

----------------------------------------------------------------------------------------------------*/
BYTE *CommitANewBlock(sPoolDescr *pPD,int iAddressSpaceIndex,int iPoolIndex,int *piFreeBlockIndex)
{
	BYTE    *pch;
	DWORD   dw;
	int     iNrSlots;
	BYTE    *pchMem;

	Assert(pPD != NULL);
	Assert(iAddressSpaceIndex >= 0);
	Assert(iPoolIndex >= 0);
	Assert(piFreeBlockIndex != NULL);

	pch = s_apasdAddressSpaces[iAddressSpaceIndex]->pchAddressSpaceMasked;  // Megabyte boundary start
																			// address of Address Space     
	dw = iPoolIndex * s_kADDRESSSPACEFORPOOL;               // Size of previous pools                       
	pch += dw;                                                                              // Start address of the pool
	dw = pPD->iNrCommitted * s_kCOMMITSIZE;                 // Size of already committed blocks     
	pch += dw;                                                                              // Start address of the block to be committed
	pPD->apchFirstFree[pPD->iNrCommitted] = (BYTE*)VirtualAlloc(pch,s_kCOMMITSIZE,MEM_COMMIT,PAGE_READWRITE);
	if (pPD->apchFirstFree[pPD->iNrCommitted] == NULL) 
	{
#ifdef USE_EXCEPTION
		throw CMemoryException();
#else
		return NULL;
#endif
	};
	iNrSlots = s_kCOMMITSIZE / pPD->iSize;                  // Number of slots in the committed block
	PrepareFreeList(iNrSlots,pPD->iSize,pPD->apchFirstFree[pPD->iNrCommitted]);
	pchMem = pPD->apchFirstFree[pPD->iNrCommitted]; // start address of committed block
	*piFreeBlockIndex = pPD->iNrCommitted;                  // the new block is free for sure
	pPD->iNrCommitted++;                                                    // Increment the Commit counter
	SetPageAsFree(pPD,*piFreeBlockIndex);                   // New block, we should mark it as free
	return pchMem;
};


/*----------------------------------------------------------------------------------------------------
	Name                            Allocate

	Synopsis                        Gives back memory pointer to the caller

	Effects                         Reserve a new chunk of memory for the user

	Arguments                       [iSize]                 Size of requested memory
						[pFileName]             Name of the source file where the call happened
						[iLineNum]              Line number in the source file where the call happened

	Requires                        Nothing

	Returns                         Pointer to an area in a block

	Signals                         Assert if iSize < 1
						Memory exception if no more free memory

	Modifies                        Free chain

	Derivation                      None

	Algorithm                       First determines the slot based on the requested size.
						If the requested size is bigger than the biggest slot passes
						the request to the system and returns.
						Otherwise checks if there is at least 1 block containing at least
						1 free slot, by checking if iFreeBlockIndex is not equal -1.
						If we do not have free block then we commit a new block if there
						is space in the reserved pool. If no space, i.e. all the reserved
						memory alredy is committed, we try the next pool (i.e. the next
						slot size). If we cannot get any free slot from any pool we pass
						the request to the system.
						If the system cannot give memory we throw a memory exception

	History                         07/01/94        Lajosf  Create
						07/08/94        Lajosf  Instead of setting 1 byte before and 1 byte after the user area
											to CHECKVALUE, I move CHECKVALUE into the whole user area.

	Notes                           None

----------------------------------------------------------------------------------------------------*/
BYTE* Allocate
(
    int iSize
#ifdef MEM_DEBUG
    ,const TCHAR *pFileName=NULL,
    int iLineNum=0
#endif
)
{
	int                     iIndex;
	register sPoolDescr     *pPD;
	BYTE                            *pchMem;
	int                     iFreeBlockIndex;
	ULONG                           *pLong;
	int                     iAddressSpaceIndex;

	Assert(iSize > 0);

	if (!s_bInitialized) MemInit(); // if something wrong MemInit throws a memory exception

	iIndex = (iSize - 1)/ s_kSIZEDELTA;

	Assert(iIndex >= 0);

	iAddressSpaceIndex = s_iFreeAddressSpace;

	s_dwTotalRequests++;
	s_dwTotalMemoryRequests += (DWORD)iSize;

TryNextPool:    // if there is no more reserved space for the pool we jump here

	if ((iIndex > (s_kNRPOOL - 1)) || !s_bInitialized)
    {               // The requested size too big, ask the system
        void * pv = HeapAlloc(g_hProcessHeap, 0, iSize);
        if (!pv)
#ifdef USE_EXCEPTION
            throw CMemoryException();
#else
            return NULL;
#endif
        s_dwRequestsPassedToSystem++;
        if ((UINT)iSize > s_uiBiggestPassedToSystem)
            s_uiBiggestPassedToSystem = (UINT)iSize;
        if ((UINT)iSize < s_uiSmallestPassedToSystem)
            s_uiSmallestPassedToSystem = (UINT)iSize;
        s_dwTotalPassedToSystem += (DWORD)iSize;
        return (BYTE*)pv;
    };
    pPD = &s_apasdAddressSpaces[iAddressSpaceIndex]->PoolDescr[iIndex];     // Get the appropriate pool description
    iFreeBlockIndex = FindFreeBlock(pPD);   // which block has free slot
    if (iFreeBlockIndex > -1)
        {       // There is free slot in iFreeBlockIndex block
        pchMem = pPD->apchFirstFree[iFreeBlockIndex];
        }
    else
        {       // There is no free block
        if (pPD->iNrCommitted < s_kNRCOMMITBLOCKS)      // Can we commit from the reserved memory ?
            {               // Yes, we did not use all
            pchMem = CommitANewBlock(pPD,iAddressSpaceIndex,iIndex,&iFreeBlockIndex);
            // if no memory, CommitANewBlock throws MemoryException
            if (pchMem == NULL) return NULL;
            }
        else
            {       // No, all reserved space for this pool used, try next pool
            iIndex++;
            goto TryNextPool;
            };
        };

    // Now we move the address in the first 4 bytes of the slot into apchFirstFree
    // i.e. we give back the head of the free list to the user and remove this element
    // from the free list.

    pLong = (ULONG*)(pchMem+DEBUGHEADERSIZE);
    pPD->apchFirstFree[iFreeBlockIndex] = (BYTE*)(*pLong);
    if (pPD->apchFirstFree[iFreeBlockIndex] == NULL)
        {               // that was the very last free slot in the block, we mark it as not free
        SetPageAsNotFree(pPD,iFreeBlockIndex);
        };
#ifdef MEM_DEBUG
    // we move the chFileName and iLineNum parameters into the sMemDescr structure in the slot
    // (and the checkbytes too)
    sMemDescr       *pMD;
    s_dwTotalAllocCounter++;
    pPD->ulAllocCounter++;
    pMD = (sMemDescr*)pchMem;
    if (pFileName == NULL)
        {
        _tcscpy((TCHAR*)pMD->chFileName,_T(""));
        pMD->iLineNum = 0;
        }
    else
        {
        int     iFileNameLimit;
        iFileNameLimit = _tcslen((TCHAR*)pFileName);
        if (iFileNameLimit > MAXFILENAME-1)
            iFileNameLimit = MAXFILENAME-1;
        _tcsncpy((TCHAR*)pMD->chFileName,pFileName,iFileNameLimit+1);
//              lstrcpyn(pMD->chFileName,pFileName,iFileNameLimit+1);
        pMD->iLineNum = iLineNum;
        };
    pMD->stState = state_allocated;
    pMD->dwAllocCounter = s_dwTotalAllocCounter;
    pMD->iSize = iSize;
    BYTE   *pchWork;
    pchWork = (BYTE*)pMD;
    pchWork += sizeof(sMemDescr);
    // Fill the slot with checkvalue
    memset(pchWork,s_kCHECKVALUE,iSize+2);
    pchMem++;               // skip the first checkbyte
#endif
    pchMem += DEBUGHEADERSIZE;
    return pchMem;                          // that is the address the user can use
};


#ifdef MEM_DEBUG
void MemoryOverwriteCheck(BYTE *pSlotStart)
{
    // check if memory was overwritten, i.e. the checkbytes are holding the original value
    BYTE            *pcCheckPtr;
    BYTE            bcheckByte;
    TCHAR           Buffer[80];
    sMemDescr       *pMD;

    // check the byte before the user area
    pcCheckPtr = pSlotStart + DEBUGHEADERSIZE;
    pMD = (sMemDescr*)pSlotStart;
    pMD->stState = state_deleted;
    bcheckByte = *pcCheckPtr;
    if (bcheckByte != s_kCHECKVALUE)
        {
        wsprintf((TCHAR*)&Buffer,_T("Memory was overwritten at location %lx\nAllocated : %s at %i\nAllocCounter : %lu"),
                pcCheckPtr,pMD->chFileName,pMD->iLineNum,pMD->dwAllocCounter);
        MessageBox(NULL, (TCHAR*)&Buffer, NULL, MB_OK);
#ifdef USE_EXCEPTION
        throw CMemoryException();
#else
        return;
#endif
        };

    // check the byte after the user area
    pcCheckPtr += (pMD->iSize + 1);
    bcheckByte = *pcCheckPtr;
    if (bcheckByte != s_kCHECKVALUE)
        {
        wsprintf((TCHAR*)&Buffer,_T("Memory was overwritten at location %lx\nAllocated : %s at %inAllocCounter : %lu"),
                pcCheckPtr,pMD->chFileName,pMD->iLineNum,pMD->dwAllocCounter);
        MessageBox(NULL, (TCHAR*)&Buffer, NULL, MB_OK);
#ifdef USE_EXCEPTION
        throw CMemoryException();
#else
        return;
#endif
        };
};
#endif




/*----------------------------------------------------------------------------------------------------
    Name                            FindOutAddressSpaceAndPool

    Synopsis                        

    Effects                         

    Arguments                       [*pch]                                  Pointer to the memory we want to find
                        [*iAddressSpaceIndex]   Index to address space ( -1 if not found)
                        [*lPoolIndex]                   Points to pool
                        {*iBlockIndex]                  Points to committed block

    Requires                        

    Returns                         Nothing

    Signals                         Assert if ptr == NULL

    Modifies                        Nothing

    Derivation                      None

    Algorithm                       

    History                         08/18/94        Lajosf  Create

    Notes                           None

----------------------------------------------------------------------------------------------------*/
void FindOutAddressSpaceAndPool(BYTE **pch,int *iAddressSpaceIndex,LONG *lPoolIndex,int *iBlockIndex)
{
    Assert(pch != NULL);

    BYTE            *pchMasked;
    DWORD           dwShiftedAddress;

    *pch -= DEBUGHEADERSIZE;
#ifdef MEM_DEBUG
    pch--;         // skip first checkbyte
#endif
    dwShiftedAddress = (DWORD)*pch >> 20;
    *iAddressSpaceIndex = s_uchAddressSpaceIndexes[dwShiftedAddress];
    if ((s_uchAddressSpaceIndexes[dwShiftedAddress] != 255) && s_bInitialized)
        { // the pointer was given by the memory manager
        pchMasked = (BYTE*)((DWORD)*pch & s_dwTruncationMask);
        *iBlockIndex = ((int)(*pch - pchMasked) / s_kCOMMITSIZE);
        pchMasked -= (DWORD)s_apasdAddressSpaces[*iAddressSpaceIndex]->pchAddressSpaceMasked;
        *lPoolIndex = (DWORD)pchMasked;
        *lPoolIndex /= s_dwMemForPool;
        }
    else *iAddressSpaceIndex = -1; // the pointer was given by the system

};


/*----------------------------------------------------------------------------------------------------
    Name                            Delete

    Synopsis                        Makes a chunk of memory free

    Effects                         Freed memory will go to the Free chain

    Arguments                       [ptr]           Pointer to the memory the user wants to free

    Requires                        The memory had to be allocated by the memory manager

    Returns                         Nothing

    Signals                         Assert if ptr == NULL OR CheckBytes != s_kCHECKVALUE

    Modifies                        Free chain

    Derivation                      None

    Algorithm                       The memory pointed by ptr will go to the beginning of the free chain.
                        If Debug version, the checkbytes will be checked if they still contain
                        CHECKVALUE. if not -> Assert.

    History                         07/01/94        Lajosf  Create
                        07/08/94        Lajosf  Move the memory overwrite checking after checking if
                                            it is our own pointer   

    Notes                           None

----------------------------------------------------------------------------------------------------*/
void Delete(BYTE *pch)
{
    if (pch == NULL) return;

    BYTE            *pchPtr;
    LONG            lPoolIndex;
    int             iBlockIndex;
    sPoolDescr      *pPD;
    DWORD           *pdw;
    int                     iAddressSpaceIndex = -1;


    pchPtr = pch;           // save original pointer value
        
    FindOutAddressSpaceAndPool(&pch,&iAddressSpaceIndex,&lPoolIndex,&iBlockIndex);
    if (iAddressSpaceIndex < 0)
    {       // this memory was asked from the system, we should pass it back without
            // modifying it.
        HeapFree(g_hProcessHeap, 0, pch);
        return;
    };

    pPD = &s_apasdAddressSpaces[iAddressSpaceIndex]->PoolDescr[lPoolIndex];
    // put the slot to be deleted at the beginning of the free list
#ifdef MEM_DEBUG
    pPD->ulDeleteCounter++;
    MemoryOverwriteCheck(pch);
    // MemoryOverwriteCheck throws Memory Exception
#endif
    pdw = (DWORD*)(pch+DEBUGHEADERSIZE);
    *pdw = (DWORD)pPD->apchFirstFree[iBlockIndex];
    pdw = (DWORD*)&pPD->apchFirstFree[iBlockIndex];         
    *pdw = (DWORD)pch;
    SetPageAsFree(pPD,iBlockIndex); // It is sure that there is at least 1 free slot in that block
};




void * ReAllocate
(
    void * pv,
    int iSize
)
{
    LONG            lPoolIndex;
    int             iBlockIndex;
    sPoolDescr      *pPD;
    int             iAddressSpaceIndex = -1;
    int             iPoolSize;
    int             iLimit;

    FindOutAddressSpaceAndPool((BYTE **)&pv,&iAddressSpaceIndex,&lPoolIndex,&iBlockIndex);
    if (iAddressSpaceIndex == -1)
    {
        pv = HeapReAlloc(g_hProcessHeap, 0, pv, iSize);
    }
    else
    {
        pPD = &s_apasdAddressSpaces[iAddressSpaceIndex]->PoolDescr[lPoolIndex];
        iPoolSize = pPD->iSize;
        if ((iSize <= iPoolSize) && (iSize >= (iPoolSize - s_kSIZEDELTA)))
            ;
        else
        {
            void * pn = Allocate(iSize);
            if (pn != NULL)
               {
                iLimit = iPoolSize;
                if (iLimit > iSize)
                    iLimit = iSize;
                memcpy(pn,pv,iLimit);
                Delete((BYTE *)pv);
               };
            pv = pn;
        }
    }
    return pv;
}


#ifdef MEM_DEBUG
void ConvStateToChar(states stState,TCHAR *pchState)
{
    switch (stState)
        {
        case state_free          : _tcscpy(pchState,_T("FREE")); break;
        case state_allocated     : _tcscpy(pchState,_T("ALLOCATED")); break;
        case state_deleted       : _tcscpy(pchState,_T("DELETED")); break;
        default                  : _tcscpy(pchState,_T("UNKNOWN")); break;
        };
};
#endif

/*----------------------------------------------------------------------------------------------------
    Name                            ShowResult

    Synopsis                        Opens tmp.tmp file (append mode) and writes informations about the
                        state of the memory manager.

    Effects                         None

    Arguments                       None

    Requires                        Nothing

    Returns                         Nothing

    Signals                         Nothing

    Modifies                        Tmp.tmp file

    Derivation                      None

    Algorithm                       --

    History                         07/01/94        Lajosf  Create

    Notes                           Only in Debug version

----------------------------------------------------------------------------------------------------*/

void ShowMemResult(void)
{
#ifdef MEM_DEBUG 
    ofstream        fOFile;
    TCHAR           chBuffer[160];
    TCHAR           chState[20];
    int             i;
    int             j;
    DWORD           k;
    sPoolDescr      *pAD;
    BYTE            *pch1;
    BYTE            *pch2;
    BYTE            *pch3;
    DWORD           dwSaveK;
    BYTE            *pchSavePtr;
    BYTE            *pchPrevPtr;
    BYTE            *pchCheckByte;
    BYTE            bCheckByte;
    int             iAddressSpaceIndex;

    fOFile.open("tmp.tmp",ios::out);
    if (!fOFile.is_open())
        {
        MessageBox(NULL,_T("Click on OK to acknowledge"),_T("Could not open tmp.tmp for output"),MB_OK);
        return;
        };
    sMemDescr       *pMD;
    states          stSaveState;
    wsprintf(chBuffer,_T("Number of requests passed to the system : %lu\n\n"),s_dwRequestsPassedToSystem);
    fOFile << chBuffer;
    if (s_dwRequestsPassedToSystem > 0)
        {
        wsprintf(chBuffer,_T("Smallest memory asked from the system : %i\n"),s_uiSmallestPassedToSystem);
    fOFile << chBuffer;
        wsprintf(chBuffer,_T("Biggest memory asked from the system : %i\n\n"),s_uiBiggestPassedToSystem);
    fOFile << chBuffer;
        wsprintf(chBuffer,_T("All memory asked from the system : %lu\n"),s_dwTotalPassedToSystem);
    fOFile << chBuffer;
        };
    wsprintf(chBuffer,_T("Total number of requests : %lu\n"),s_dwTotalRequests);
    fOFile << chBuffer;
    wsprintf(chBuffer,_T("All memory requests : %lu\n\n"),s_dwTotalMemoryRequests);
    fOFile << chBuffer;
    for (iAddressSpaceIndex=0;iAddressSpaceIndex<s_iNrAddressSpaces;iAddressSpaceIndex++)
        {
        pch1 = s_apasdAddressSpaces[iAddressSpaceIndex]->pchAddressSpaceMasked;
        for (i=0,pAD=&s_apasdAddressSpaces[iAddressSpaceIndex]->PoolDescr[0];i<s_kNRPOOL;i++,pAD++)
            {
            if (pAD->iNrCommitted > 0)
                {
                wsprintf(chBuffer,_T("%3i. pool for size %3i (nr of committed blocks : %4i) Allocated : %6lu Deleted : %6lu\n"),
                        i,pAD->iSize-sizeof(sMemDescr)-2,pAD->iNrCommitted,pAD->ulAllocCounter,pAD->ulDeleteCounter);
                }
            else
                {
                wsprintf(chBuffer,_T("%3i. pool for size %3i ---------------------------------------------------------------------------\n"),
                        i,pAD->iSize-sizeof(sMemDescr)-2);
                };
        fOFile << chBuffer;
            pch2 = pch1;
            for (j=0;j<pAD->iNrCommitted;j++)
                {
                wsprintf(chBuffer,_T("  %3i. block first free : %lx\n"),
                            j,pAD->apchFirstFree[j]);
        fOFile << chBuffer;
                pch3 = pch2;
                pchSavePtr = pch3;
                pchPrevPtr = pch3;
                pMD = (sMemDescr*)pch3;
                stSaveState = pMD->stState;
                dwSaveK = 0;
                for (k=0;k<s_kCOMMITSIZE/pAD->iSize;k++)
                    {
                    pMD = (sMemDescr*)pch3;
                    if (pMD->stState == state_allocated)
                        {
                        pchCheckByte = pch3+sizeof(sMemDescr);
                        bCheckByte = *pchCheckByte;
                        if (bCheckByte != s_kCHECKVALUE)
                            {
                            wsprintf(chBuffer,_T("******    Checkbyte overwritten (before) at %lx %s %i %lu\n"),
                                     pchCheckByte,pMD->chFileName,pMD->iLineNum,pMD->dwAllocCounter);
                fOFile << chBuffer;
                            };
                        pchCheckByte += (pMD->iSize+1);
                        bCheckByte = *pchCheckByte;
                        if (bCheckByte != s_kCHECKVALUE)
                            {
                            wsprintf(chBuffer,_T("*****    Checkbyte overwritten (after) at %lx %s %i %lu\n"),
                                     pchCheckByte,pMD->chFileName,pMD->iLineNum,pMD->dwAllocCounter);
                fOFile << chBuffer;
                            };                              
                        };
                    if (stSaveState != pMD->stState)
                        {               
                        ConvStateToChar(stSaveState,chState);
                        wsprintf(chBuffer,_T("    %6lu.-%6lu %8lx-%8lx  %s\n"),
                                          dwSaveK,k-1,pchSavePtr,pchPrevPtr,chState);
            fOFile << chBuffer;
                        stSaveState = pMD->stState;
                        pchSavePtr = pch3;
                        dwSaveK = k;
                        };
                    pchPrevPtr = pch3;
                    pch3 += pAD->iSize;
                    };
                ConvStateToChar(stSaveState,chState);
                wsprintf(chBuffer,_T("    %6lu.-%6lu %8lx-%8lx  %s\n"),
                          dwSaveK,s_kCOMMITSIZE/pAD->iSize,pchSavePtr,pchPrevPtr,chState);
        fOFile << chBuffer;
                pch2 += s_kCOMMITSIZE;
                };
            pch1 += s_dwMemForPool;
            };
        };
    fOFile.close();
#endif
};



void *
MemAlloc(size_t cb)
{
    Assert(g_fHeapOpAllowed);
    return DbgPostAlloc(Allocate(DbgPreAlloc(cb)));
};



void *
MemAllocClear(size_t cb)
{
    Assert(g_fHeapOpAllowed);
    void *pv;

    pv = MemAlloc(cb);
    if (pv != NULL)
    memset(pv, 0, cb);
    return pv;
}



void 
MemFree(void *pv)
{
    Assert(g_fHeapOpAllowed);
    Delete((BYTE *)DbgPreFree(pv));
    DbgPostFree();
};



HRESULT 
MemRealloc(void **ppv, size_t cb)
{
    Assert(g_fHeapOpAllowed);
    void *pv;

    if (cb == 0)
    {
        Delete((BYTE *)DbgPreFree(*ppv));
        DbgPostFree();
        *ppv = 0;
    } 
    else if (*ppv == NULL)
    {
        *ppv = DbgPostAlloc(Allocate(DbgPreAlloc(cb)));
        if (*ppv == NULL)
            return E_OUTOFMEMORY;
    }
    else
    {       
        #if DBG == 1
            cb = DbgPreRealloc(*ppv, cb, &pv);
            pv = DbgPostRealloc(ReAllocate(pv, cb));
        #else
            pv = ReAllocate(*ppv, cb);
        #endif
        if (pv == NULL)
            return E_OUTOFMEMORY;
        *ppv = pv;
    }
    return NOERROR;
};



size_t
MemGetSize(void *pv)
{
    Assert(g_fHeapOpAllowed);
    if (pv == NULL)
    return 0;

    pv = DbgPreGetSize(pv);

    LONG            lPoolIndex;
    int             iBlockIndex;
    int             iAddressSpaceIndex = -1;

        
    FindOutAddressSpaceAndPool((BYTE**)&pv,&iAddressSpaceIndex,&lPoolIndex,&iBlockIndex);
    if (iAddressSpaceIndex == -1)
    {
        return DbgPostGetSize(HeapSize(g_hProcessHeap, 0, pv));
    }
    else
    {
        return DbgPostGetSize(s_apasdAddressSpaces[iAddressSpaceIndex]->PoolDescr[lPoolIndex].iSize);
    }
};


/*
LPVOID
WINAPI
SlotLock(
    HLOCAL hMem
    )
{
    LPVOID  p = NULL;

    p = (LPVOID)hMem;
    if (p == NULL) return p;

    LONG            lPoolIndex;
    int             iBlockIndex;
    int             iAddressSpaceIndex = -1;

        
    FindOutAddressSpaceAndPool((BYTE**)&p,&iAddressSpaceIndex,&lPoolIndex,&iBlockIndex);
    if (iAddressSpaceIndex == -1)
        {
        return LocalLock(hMem);
        };
    return p;
};



HLOCAL
WINAPI
SlotHandle(
    LPCVOID pMem
    )
{
    LONG            lPoolIndex;
    int             iBlockIndex;
    int             iAddressSpaceIndex = -1;

        
    FindOutAddressSpaceAndPool((BYTE**)&pMem,&iAddressSpaceIndex,&lPoolIndex,&iBlockIndex);
    if (iAddressSpaceIndex == -1)
        {
        return LocalHandle(pMem);
        };
    return (HLOCAL)pMem;

};



BOOL
WINAPI
SlotUnlock(
    HLOCAL hMem
    )
{
    LPVOID  pMem;
    pMem = (LPVOID)hMem;

    LONG            lPoolIndex;
    int             iBlockIndex;
    int             iAddressSpaceIndex = -1;

        
    FindOutAddressSpaceAndPool((BYTE**)&pMem,&iAddressSpaceIndex,&lPoolIndex,&iBlockIndex);
    if (iAddressSpaceIndex == -1)
        {
        return LocalUnlock(hMem);
        };
    return TRUE;

};
*/

#endif SLOTMANAGER
