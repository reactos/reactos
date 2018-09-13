/*
 * memmgr.c - Memory manager module.
 */

/*

   The memory manager implementation in this module uses either a private
shared heap (if PRIVATE_HEAP is #defined) or the non-shared process heap (if
PRIVATE_HEAP is not #defined).  Thde debug implementation of this memory
manager keeps track of memory blocks allocated from the heap using a
doubly-linked list of heap element nodes.  Each node describes one allocated
heap element.

   Debug heap elements are allocated with extra space at the beginning and end
of the element.  Prefix and suffix sentinels surround each allocated heap
element.  New heap elements are filled with UNINITIALIZED_BYTE_VALUE.  Freed
heap elements are filled with FREED_BYTE_VALUE.  The new tails of grown heap
elements are filled with UNINITIALIZED_BYTE_VALUE.

*/


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/* Constants
 ************/

#ifdef PRIVATE_HEAP

/* undocumented flag for HeapCreate() from kernel32.h */


#define HEAP_SHARED                 (0x04000000)

/*
 * Set maximum shared heap size used for CreateHeap() to 0 since we don't know
 * how big the heap may get, and we don't want to restrict its size
 * artificially.  BrianSm says this is ok.
 */

#define MAX_SHARED_HEAP_SIZE        (0)

#endif   /* PRIVATE_HEAP */

#ifdef DEBUG

/* heap element byte fill values */

#define UNINITIALIZED_BYTE_VALUE    (0xcc)
#define FREED_BYTE_VALUE            (0xdd)

#endif   /* DEBUG */


/* Macros
 *********/

/* atomic memory management function wrappers for translation */

#ifdef PRIVATE_HEAP

#define GetHeap()                   (Mhheap)
#define MEMALLOCATE(size)           HeapAlloc(GetHeap(), 0, (size))
#define MEMREALLOCATE(pv, size)     HeapReAlloc(GetHeap(), 0, (pv), (size))
#define MEMFREE(pv)                 HeapFree(GetHeap(), 0, (pv))
#define MEMSIZE(pv)                 (DWORD)HeapSize(GetHeap(), 0, (pv))

#else

#define MEMALLOCATE(size)           LocalAlloc(LMEM_FIXED, (size))
#define MEMREALLOCATE(pv, size)     LocalReAlloc((pv), (size), 0)
#define MEMFREE(pv)                 (! LocalFree(pv))
#define MEMSIZE(pv)                 (DWORD)LocalSize(pv)

#endif


/* Types
 ********/

#ifdef DEBUG

/* heap element descriptor structure */

typedef struct _heapelemdesc
{
   TCHAR rgchSize[6];       /* enough for 99,999 lines */
   TCHAR rgchFile[24];      
   ULONG ulLine;
}
HEAPELEMDESC;
DECLARE_STANDARD_TYPES(HEAPELEMDESC);

/* heap node */

typedef struct _heapnode
{
   PCVOID pcv;
   DWORD dwcbSize;
   struct _heapnode *phnPrev;
   struct _heapnode *phnNext;
   HEAPELEMDESC hed;
}
HEAPNODE;
DECLARE_STANDARD_TYPES(HEAPNODE);

/* heap */

typedef struct _heap
{
   HEAPNODE hnHead;
}
HEAP;
DECLARE_STANDARD_TYPES(HEAP);

/* heap summary filled in by AnalyzeHeap() */

typedef struct _heapsummary
{
   ULONG ulcUsedElements;
   DWORD dwcbUsedSize;
}
HEAPSUMMARY;
DECLARE_STANDARD_TYPES(HEAPSUMMARY);

/* debug flags */

typedef enum _memmgrdebugflags
{
   MEMMGR_DFL_VALIDATE_HEAP_ON_ENTRY   = 0x0001,

   MEMMGR_DFL_VALIDATE_HEAP_ON_EXIT    = 0x0002,

   ALL_MEMMGR_DFLAGS                   = (MEMMGR_DFL_VALIDATE_HEAP_ON_ENTRY |
                                          MEMMGR_DFL_VALIDATE_HEAP_ON_EXIT)
}
MEMMGRDEBUGFLAGS;

#endif   /* DEBUG */


/* Global Variables
 *******************/

#ifdef DEBUG

#pragma data_seg(DATA_SEG_PER_INSTANCE)

/* parameters used by debug AllocateMemory() macro */

PUBLIC_DATA LPCTSTR GpcszElemHdrSize = NULL;
PUBLIC_DATA LPCTSTR GpcszElemHdrFile = NULL;
PUBLIC_DATA ULONG GulElemHdrLine = 0;

#pragma data_seg()

#endif   /* DEBUG */


/* Module Variables
 *******************/

#ifdef PRIVATE_HEAP

#pragma data_seg(DATA_SEG_SHARED)

/* handle to global shared heap */

PRIVATE_DATA HANDLE Mhheap = NULL;

#pragma data_seg()

#endif   /* PRIVATE_HEAP */

#ifdef DEBUG

#ifdef PRIVATE_HEAP
#pragma data_seg(DATA_SEG_SHARED)
#else
#pragma data_seg(DATA_SEG_PER_INSTANCE)
#endif   /* PRIVATE_HEAP */

/* heap */

PRIVATE_DATA PHEAP Mpheap = NULL;

#pragma data_seg(DATA_SEG_SHARED)

/* debug flags */

PRIVATE_DATA DWORD MdwMemoryManagerModuleFlags = 0;

#pragma data_seg(DATA_SEG_READ_ONLY)

/* heap element sentinels */

PRIVATE_DATA CONST struct
{
   BYTE rgbyte[4];
}
MchsPrefix =
{
   { TEXT('H'), TEXT('E'), TEXT('A'), TEXT('D') }
};

PRIVATE_DATA CONST struct
{
   BYTE rgbyte[4];
}
MchsSuffix =
{
   { TEXT('T'), TEXT('A'), TEXT('I'), TEXT('L') }
};

/* .ini file switch descriptions */

PRIVATE_DATA CBOOLINISWITCH cbisValidateHeapOnEntry =
{
   IST_BOOL,
   TEXT("ValidateHeapOnEntry"),
   &MdwMemoryManagerModuleFlags,
   MEMMGR_DFL_VALIDATE_HEAP_ON_ENTRY
};

PRIVATE_DATA CBOOLINISWITCH cbisValidateHeapOnExit =
{
   IST_BOOL,
   TEXT("ValidateHeapOnExit"),
   &MdwMemoryManagerModuleFlags,
   MEMMGR_DFL_VALIDATE_HEAP_ON_EXIT
};

PRIVATE_DATA const PCVOID MrgcpcvisMemoryManagerModule[] =
{
   &cbisValidateHeapOnEntry,
   &cbisValidateHeapOnExit
};

#pragma data_seg()

#endif   /* DEBUG */


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

#ifdef DEBUG

PRIVATE_CODE DWORD CalculatePrivateSize(DWORD);
PRIVATE_CODE PVOID GetPrivateHeapPtr(PVOID);
PRIVATE_CODE PVOID GetPublicHeapPtr(PVOID);
PRIVATE_CODE DWORD GetHeapSize(PCVOID);
PRIVATE_CODE BOOL AddHeapElement(PCVOID, DWORD);
PRIVATE_CODE void RemoveHeapElement(PCVOID);
PRIVATE_CODE void ModifyHeapElement(PCVOID, PCVOID, DWORD);
PRIVATE_CODE BOOL FindHeapElement(PCVOID, PHEAPNODE *);
PRIVATE_CODE void FillNewMemory(PBYTE, DWORD, DWORD);
PRIVATE_CODE void FillFreedMemory(PBYTE, DWORD);
PRIVATE_CODE void FillGrownMemory(PBYTE, DWORD, DWORD, DWORD);
PRIVATE_CODE void FillShrunkenMemory(PBYTE, DWORD, DWORD, DWORD);
PRIVATE_CODE BOOL IsValidHeapPtr(PCVOID);
PRIVATE_CODE BOOL IsHeapOK(void);
PRIVATE_CODE BOOL IsValidPCHEAPNODE(PCHEAPNODE);
PRIVATE_CODE BOOL IsValidPCHEAPELEMDESC(PCHEAPELEMDESC);
PRIVATE_CODE BOOL IsValidHeapElement(PCBYTE, DWORD, DWORD);
PRIVATE_CODE void SpewHeapElementInfo(PCHEAPNODE);
PRIVATE_CODE void AnalyzeHeap(PHEAPSUMMARY, DWORD);

#endif   /* DEBUG */


#ifdef PRIVATE_HEAP

/*
** InitPrivateHeapModule()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL InitPrivateHeapModule(void)
{
   BOOL bResult;
   SYSTEM_INFO si;

   ASSERT(! Mhheap);

   /* Create shared heap. */

   GetSystemInfo(&si);

   #ifdef WINNT
   Mhheap = HeapCreate(0, si.dwPageSize, MAX_SHARED_HEAP_SIZE);
#else      
   Mhheap = HeapCreate(HEAP_SHARED, si.dwPageSize, MAX_SHARED_HEAP_SIZE);
#endif

   if (Mhheap)
   {

#ifdef DEBUG

      ASSERT(! Mpheap);

      Mpheap = MEMALLOCATE(sizeof(*Mpheap));
      
      if (Mpheap)
      {
         FillMemory(Mpheap, sizeof(*Mpheap), 0);
         bResult = TRUE;

         TRACE_OUT((TEXT("InitMemoryManagerModule(): Created shared heap, initial size == %lu, maximum size == %lu."),
                    si.dwPageSize,
                    MAX_SHARED_HEAP_SIZE));
      }
      else
      {
         EVAL(HeapDestroy(Mhheap));
         Mhheap = NULL;
         bResult = FALSE;

         WARNING_OUT((TEXT("InitMemoryManagerModule(): Failed to create shared heap head.")));
      }

#else    /* DEBUG */

      bResult = TRUE;

#endif   /* DEBUG */
         
   }
   else
   {
      bResult = FALSE;

      WARNING_OUT((TEXT("InitMemoryManagerModule(): Failed to create shared heap.")));
   }

   return(bResult);
}


#else    /* PRIVATE_HEAP */


/*
** InitHeapModule()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL InitHeapModule(void)
{
   BOOL bResult;

#ifdef DEBUG

   ASSERT(! Mpheap);

   Mpheap = MEMALLOCATE(sizeof(*Mpheap));
   
   if (Mpheap)
   {
      FillMemory(Mpheap, sizeof(*Mpheap), 0);

      TRACE_OUT((TEXT("InitMemoryManagerModule(): Created heap.")));
   }
   else
      WARNING_OUT((TEXT("InitMemoryManagerModule(): Failed to create heap head.")));
         
   bResult = (Mpheap != NULL);

#else

   bResult = TRUE;

#endif

   return(bResult);
}


#endif   /* PRIVATE_HEAP */


#ifdef DEBUG

/*
** CalculatePrivateSize()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE DWORD CalculatePrivateSize(DWORD dwcbPublicSize)
{
   ASSERT(dwcbPublicSize <= DWORD_MAX - sizeof(MchsPrefix) - sizeof(MchsSuffix));

   return(dwcbPublicSize + sizeof(MchsPrefix) + sizeof(MchsSuffix));
}


/*
** GetPrivateHeapPtr()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE PVOID GetPrivateHeapPtr(PVOID pvPublic)
{
   ASSERT((ULONG_PTR)pvPublic > sizeof(MchsPrefix));

   return((PBYTE)pvPublic - sizeof(MchsPrefix));
}


/*
** GetPublicHeapPtr()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE PVOID GetPublicHeapPtr(PVOID pvPrivate)
{
   ASSERT((PCBYTE)pvPrivate <= (PCBYTE)PTR_MAX - sizeof(MchsPrefix));

   return((PBYTE)pvPrivate + sizeof(MchsPrefix));
}


/*
** GetHeapSize()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE DWORD GetHeapSize(PCVOID pcv)
{
   PHEAPNODE phn;
   DWORD dwcbSize;

   if (EVAL(FindHeapElement(pcv, &phn)))
      dwcbSize = phn->dwcbSize;
   else
      dwcbSize = 0;

   return(dwcbSize);
}


/*
** AddHeapElement()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** Assumes that the global variables GpcszElemHdrSize, GpcszElemHdrFile, and
** GulElemHdrLine are filled in.
*/
PRIVATE_CODE BOOL AddHeapElement(PCVOID pcvNew, DWORD dwcbSize)
{
   PHEAPNODE phnNew;

   /* Is the new heap element already in the list? */

   ASSERT(! FindHeapElement(pcvNew, &phnNew));

   if (Mpheap)
   {
      /* Create new heap node. */

      phnNew = MEMALLOCATE(sizeof(*phnNew));

      if (phnNew)
      {
         /* Fill in heap node fields. */

         phnNew->pcv = pcvNew;
         phnNew->dwcbSize = dwcbSize;

         /* Insert heap node at front of list. */

         phnNew->phnNext = Mpheap->hnHead.phnNext;
         phnNew->phnPrev = &(Mpheap->hnHead);
         Mpheap->hnHead.phnNext = phnNew;

         if (phnNew->phnNext)
            phnNew->phnNext->phnPrev = phnNew;

         /* Fill in heap element descriptor fields. */

         MyLStrCpyN(phnNew->hed.rgchSize, GpcszElemHdrSize, ARRAYSIZE(phnNew->hed.rgchSize));
         MyLStrCpyN(phnNew->hed.rgchFile, GpcszElemHdrFile, ARRAYSIZE(phnNew->hed.rgchFile));
         phnNew->hed.ulLine = GulElemHdrLine;

         ASSERT(IS_VALID_STRUCT_PTR(phnNew, CHEAPNODE));
      }
   }
   else
      phnNew = NULL;

   return(phnNew != NULL);
}


/*
** RemoveHeapElement()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void RemoveHeapElement(PCVOID pcvOld)
{
   PHEAPNODE phnOld;

   if (EVAL(FindHeapElement(pcvOld, &phnOld)))
   {
      /* Remove heap node from list. */

      phnOld->phnPrev->phnNext = phnOld->phnNext;

      if (phnOld->phnNext)
         phnOld->phnNext->phnPrev = phnOld->phnPrev;

      MEMFREE(phnOld);
   }

   return;
}


/*
** ModifyHeapElement()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void ModifyHeapElement(PCVOID pcvOld, PCVOID pcvNew, DWORD dwcbNewSize)
{
   PHEAPNODE phn;

   if (EVAL(FindHeapElement(pcvOld, &phn)))
   {
      phn->pcv = pcvNew;
      phn->dwcbSize = dwcbNewSize;
   }

   return;
}


/*
** FindHeapElement()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL FindHeapElement(PCVOID pcvTarget, PHEAPNODE *pphn)
{
   BOOL bFound = FALSE;
   PHEAPNODE phn;

   ASSERT(IS_VALID_WRITE_PTR(pphn, PHEAPNODE));

   if (Mpheap)
   {
      for (phn = Mpheap->hnHead.phnNext;
           phn;
           phn = phn->phnNext)
      {
         /*
          * Verify each HEAPNODE structure carefully.  We may be in the middle of
          * a ModifyHeapElement() call, in which case just the target HEAPNODE may
          * be invalid, e.g., after MEMREALLOCATE() in ReallocateMemory().
          */

         ASSERT((IS_VALID_READ_PTR(phn, CHEAPNODE) && phn->pcv == pcvTarget) ||
                IS_VALID_STRUCT_PTR(phn, CHEAPNODE));

         if (phn->pcv == pcvTarget)
         {
            *pphn = phn;
            bFound = TRUE;
            break;
         }
      }
   }

   return(bFound);
}


/*
** FillNewMemory()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void FillNewMemory(PBYTE pbyte, DWORD dwcbRequestedSize,
                           DWORD dwcbAllocatedSize)
{
   ASSERT(dwcbRequestedSize >= sizeof(MchsPrefix) + sizeof(MchsSuffix));
   ASSERT(dwcbAllocatedSize >= dwcbRequestedSize);
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pbyte, BYTE, (UINT)dwcbAllocatedSize));

   /* Fill new heap element with the uninitialized byte value. */

   FillMemory(pbyte, dwcbAllocatedSize, UNINITIALIZED_BYTE_VALUE);

   /* Copy prefix and suffix heap element sentinels. */

   CopyMemory(pbyte, &MchsPrefix, sizeof(MchsPrefix));
   CopyMemory(pbyte + dwcbRequestedSize - sizeof(MchsSuffix), &MchsSuffix,
              sizeof(MchsSuffix));

   return;
}


/*
** FillFreedMemory()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void FillFreedMemory(PBYTE pbyte, DWORD dwcbAllocatedSize)
{
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pbyte, BYTE, (UINT)dwcbAllocatedSize));

   /* Fill old heap element with the freed byte value. */

   FillMemory(pbyte, dwcbAllocatedSize, FREED_BYTE_VALUE);

   return;
}


/*
** FillGrownMemory()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void FillGrownMemory(PBYTE pbyte, DWORD dwcbOldRequestedSize,
                             DWORD dwcbNewRequestedSize,
                             DWORD dwcbNewAllocatedSize)
{
   ASSERT(dwcbOldRequestedSize >= sizeof(MchsPrefix) + sizeof(MchsSuffix));
   ASSERT(dwcbNewRequestedSize > dwcbOldRequestedSize);
   ASSERT(dwcbNewAllocatedSize >= dwcbNewRequestedSize);
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pbyte, BYTE, (UINT)dwcbNewAllocatedSize));

   ASSERT(MyMemComp(pbyte, &MchsPrefix, sizeof(MchsPrefix)) == CR_EQUAL);

   /* Fill new heap element tail with the uninitialized byte value. */

   FillMemory(pbyte + dwcbOldRequestedSize - sizeof(MchsSuffix),
              dwcbNewRequestedSize - dwcbOldRequestedSize,
              UNINITIALIZED_BYTE_VALUE);

   /* Copy suffix heap element sentinel. */

   CopyMemory(pbyte + dwcbNewRequestedSize - sizeof(MchsSuffix), &MchsSuffix,
              sizeof(MchsSuffix));

   return;
}


/*
** FillShrunkenMemory()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void FillShrunkenMemory(PBYTE pbyte, DWORD dwcbOldRequestedSize,
                                DWORD dwcbNewRequestedSize,
                                DWORD dwcbNewAllocatedSize)
{
   ASSERT(dwcbNewRequestedSize >= sizeof(MchsPrefix) + sizeof(MchsSuffix));
   ASSERT(dwcbNewRequestedSize < dwcbOldRequestedSize);
   ASSERT(dwcbNewAllocatedSize >= dwcbNewRequestedSize);
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pbyte, BYTE, (UINT)dwcbNewAllocatedSize));

   ASSERT(MyMemComp(pbyte, &MchsPrefix, sizeof(MchsPrefix)) == CR_EQUAL);

   /* Fill old heap element tail with the freed byte value. */

   FillMemory(pbyte + dwcbNewRequestedSize,
              dwcbOldRequestedSize - dwcbNewRequestedSize, FREED_BYTE_VALUE);

   /* Copy suffix heap element sentinel. */

   CopyMemory(pbyte + dwcbNewRequestedSize - sizeof(MchsSuffix), &MchsSuffix,
              sizeof(MchsSuffix));

   return;
}


/*
** IsValidHeapPtr()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidHeapPtr(PCVOID pcv)
{
   PHEAPNODE phnUnused;

   return(FindHeapElement(pcv, &phnUnused));
}


/*
** IsHeapOK()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsHeapOK(void)
{
   PHEAPNODE phn;

   if (Mpheap)
   {
      for (phn = Mpheap->hnHead.phnNext;
           phn && IS_VALID_STRUCT_PTR(phn, CHEAPNODE);
           phn = phn->phnNext)
         ;
   }
   else
      phn = (PHEAPNODE)0xFFFF;

   return(phn == NULL);
}


/*
** IsValidPCHEAPNODE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCHEAPNODE(PCHEAPNODE pchn)
{
   BOOL bResult;

   if (IS_VALID_READ_PTR(pchn, CHEAPNODE) &&
       IS_VALID_READ_PTR(pchn->phnPrev, CHEAPNODE) &&
       EVAL(pchn->phnPrev->phnNext == pchn) &&
       EVAL(! pchn->phnNext ||
            (IS_VALID_READ_PTR(pchn->phnNext, CHEAPNODE) &&
             EVAL(pchn->phnNext->phnPrev == pchn))) &&
       EVAL(IsValidHeapElement(pchn->pcv, pchn->dwcbSize, MEMSIZE((PVOID)(pchn->pcv)))) &&
       IS_VALID_STRUCT_PTR(&(pchn->hed), CHEAPELEMDESC))
      bResult = TRUE;
   else
      bResult = FALSE;

   return(bResult);
}


/*
** IsValidPCHEAPELEMDESC()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCHEAPELEMDESC(PCHEAPELEMDESC pched)
{
   BOOL bResult;

   /* Any value for pched->ulLine is valid. */

   if (IS_VALID_READ_PTR(pched, CHEAPELEMDESC))
      bResult = TRUE;
   else
      bResult = FALSE;

   return(bResult);
}


/*
** IsValidHeapElement()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidHeapElement(PCBYTE pcbyte, DWORD dwcbRequestedSize,
                                DWORD dwcbAllocatedSize)
{
   BOOL bResult;

   if (EVAL(dwcbRequestedSize >= sizeof(MchsPrefix) + sizeof(MchsSuffix)) &&
       EVAL(dwcbAllocatedSize >= dwcbRequestedSize) &&
       IS_VALID_READ_PTR(pcbyte, dwcbAllocatedSize) &&
       EVAL(MyMemComp(pcbyte, &MchsPrefix, sizeof(MchsPrefix)) == CR_EQUAL) &&
       EVAL(MyMemComp(pcbyte + dwcbRequestedSize - sizeof(MchsSuffix), &MchsSuffix, sizeof(MchsSuffix)) == CR_EQUAL))
      bResult = TRUE;
   else
      bResult = FALSE;

   return(bResult);
}


/*
** SpewHeapElementInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void SpewHeapElementInfo(PCHEAPNODE pchn)
{
   ASSERT(IS_VALID_STRUCT_PTR(pchn, CHEAPNODE));

   TRACE_OUT((TEXT("Used heap element at %#lx:\r\n")
              TEXT("     %lu bytes requested\r\n")
              TEXT("     %lu bytes allocated\r\n")
              TEXT("     originally allocated as '%s' bytes in file %s at line %lu"),
              pchn->pcv,
              pchn->dwcbSize,
              MEMSIZE((PVOID)(pchn->pcv)),
              pchn->hed.rgchSize,
              pchn->hed.rgchFile,
              pchn->hed.ulLine));

   return;
}


/*
** AnalyzeHeap()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void AnalyzeHeap(PHEAPSUMMARY phs, DWORD dwFlags)
{
   PCHEAPNODE pchn;
   ULONG ulcHeapElements = 0;
   DWORD dwcbUsed = 0;

   ASSERT(IS_VALID_WRITE_PTR(phs, HEAPSUMMARY));
   ASSERT(FLAGS_ARE_VALID(dwFlags, SHS_FL_SPEW_USED_INFO));

   ASSERT(IsHeapOK());

   TRACE_OUT((TEXT("Starting private heap analysis.")));

   if (Mpheap)
   {
      for (pchn = Mpheap->hnHead.phnNext;
           pchn;
           pchn = pchn->phnNext)
      {
         ASSERT(IS_VALID_STRUCT_PTR(pchn, CHEAPNODE));

         ASSERT(ulcHeapElements < ULONG_MAX);
         ulcHeapElements++;

         ASSERT(dwcbUsed < DWORD_MAX - pchn->dwcbSize);
         dwcbUsed += pchn->dwcbSize;

         if (IS_FLAG_SET(dwFlags, SHS_FL_SPEW_USED_INFO))
            SpewHeapElementInfo(pchn);
      }

      phs->ulcUsedElements = ulcHeapElements;
      phs->dwcbUsedSize = dwcbUsed;
   }
   else
      WARNING_OUT((TEXT("Private heap not allocated!")));

   TRACE_OUT((TEXT("Private heap analysis complete.")));

   return;
}

#endif   /* DEBUG */


/****************************** Public Functions *****************************/


/*
** InitMemoryManagerModule()
**
** When PRIVATE_HEAP is defined, this function should be called only
** once, when the DLL is being first initialized.  When PRIVATE_HEAP
** is not defined, this function should be called for every 
** DLL_PROCESS_ATTACH.
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL InitMemoryManagerModule(void)
{
   BOOL bResult;

#ifdef PRIVATE_HEAP

   bResult = InitPrivateHeapModule();

#else  /* PRIVATE_HEAP */

   bResult = InitHeapModule();

#endif

   return(bResult);
}

/*
** ExitMemoryManagerModule()
**
** When PRIVATE_HEAP is defined, this function should be called only
** once, when the DLL is finally being terminated.  When PRIVATE_HEAP
** is not defined, this function should be called for every 
** DLL_PROCESS_DETACH.
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void ExitMemoryManagerModule(void)
{

#ifdef DEBUG

   if (Mpheap)
   {
      MEMFREE(Mpheap);
      Mpheap = NULL;
   }
   else
      WARNING_OUT((TEXT("ExitMemoryManagerModule() called when Mpheap is NULL.")));

#endif

#ifdef PRIVATE_HEAP

   if (Mhheap)
   {
      EVAL(HeapDestroy(Mhheap));
      Mhheap = NULL;
   }
   else
      WARNING_OUT((TEXT("ExitMemoryManagerModule() called when Mhheap is NULL.")));

#endif

   return;
}


/*
** MyMemComp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE COMPARISONRESULT MyMemComp(PCVOID pcv1, PCVOID pcv2, DWORD dwcbSize)
{
   int nResult = 0;
   PCBYTE pcbyte1 = pcv1;
   PCBYTE pcbyte2 = pcv2;

   ASSERT(IS_VALID_READ_BUFFER_PTR(pcv1, BYTE, (UINT)dwcbSize));
   ASSERT(IS_VALID_READ_BUFFER_PTR(pcv2, BYTE, (UINT)dwcbSize));

   while (dwcbSize > 0 &&
          ! (nResult = *pcbyte1 - *pcbyte2))
   {
      pcbyte1++;
      pcbyte2++;
      dwcbSize--;
   }

   return(MapIntToComparisonResult(nResult));
}


/*
** MyAllocateMemory()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL MyAllocateMemory(DWORD dwcbSize, PVOID *ppvNew)
{

#ifdef DEBUG

   DWORD dwcbRequestedSize = dwcbSize;

   ASSERT(dwcbSize >= 0);
   ASSERT(IS_VALID_WRITE_PTR(ppvNew, PVOID));

   dwcbSize = CalculatePrivateSize(dwcbSize);

   if (IS_FLAG_SET(MdwMemoryManagerModuleFlags, MEMMGR_DFL_VALIDATE_HEAP_ON_ENTRY))
      ASSERT(IsHeapOK());

#endif

   *ppvNew = MEMALLOCATE(dwcbSize);

#ifdef DEBUG

   if (*ppvNew)
   {
      FillNewMemory(*ppvNew, dwcbSize, MEMSIZE(*ppvNew));

      if (AddHeapElement(*ppvNew, dwcbSize))
      {
         *ppvNew = GetPublicHeapPtr(*ppvNew);

         ASSERT(IS_VALID_WRITE_BUFFER_PTR(*ppvNew, BYTE, (UINT)dwcbRequestedSize));
      }
      else
      {
         EVAL(MEMFREE(*ppvNew));
         *ppvNew = NULL;
      }
   }

   if (IS_FLAG_SET(MdwMemoryManagerModuleFlags, MEMMGR_DFL_VALIDATE_HEAP_ON_EXIT))
      ASSERT(IsHeapOK());

#endif

   return(*ppvNew != NULL);
}


/*
** FreeMemory()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void FreeMemory(PVOID pvOld)
{

#ifdef DEBUG

   if (IS_FLAG_SET(MdwMemoryManagerModuleFlags, MEMMGR_DFL_VALIDATE_HEAP_ON_ENTRY))
      ASSERT(IsHeapOK());

   pvOld = GetPrivateHeapPtr(pvOld);

   ASSERT(IsValidHeapPtr(pvOld));

   RemoveHeapElement(pvOld);

   FillFreedMemory(pvOld, MEMSIZE(pvOld));

#endif

   EVAL(MEMFREE(pvOld));

#ifdef DEBUG

   if (IS_FLAG_SET(MdwMemoryManagerModuleFlags, MEMMGR_DFL_VALIDATE_HEAP_ON_EXIT))
      ASSERT(IsHeapOK());

#endif   /* DEBUG */

   return;
}


/*
** ReallocateMemory()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL ReallocateMemory(PVOID pvOld, DWORD dwcbNewSize, PVOID *ppvNew)
{

#ifdef DEBUG

   DWORD dwcbRequestedSize = dwcbNewSize;
   DWORD dwcbOldSize;

   ASSERT(IS_VALID_WRITE_PTR(ppvNew, PVOID));

   if (IS_FLAG_SET(MdwMemoryManagerModuleFlags, MEMMGR_DFL_VALIDATE_HEAP_ON_ENTRY))
      ASSERT(IsHeapOK());

   pvOld = GetPrivateHeapPtr(pvOld);

   ASSERT(IsValidHeapPtr(pvOld));

   dwcbNewSize = CalculatePrivateSize(dwcbNewSize);

   dwcbOldSize = GetHeapSize(pvOld);

   if (dwcbNewSize == dwcbOldSize)
      WARNING_OUT((TEXT("ReallocateMemory(): Size of heap element %#lx is already %lu bytes."),
                   GetPublicHeapPtr(pvOld),
                   dwcbNewSize));

#endif

   *ppvNew = MEMREALLOCATE(pvOld, dwcbNewSize);

#ifdef DEBUG

   if (*ppvNew)
   {
      /* Bigger or smaller? */

      if (dwcbNewSize > dwcbOldSize)
         /* Bigger. */
         FillGrownMemory(*ppvNew, dwcbOldSize, dwcbNewSize, MEMSIZE(*ppvNew));
      else
         /* Smaller. */
         FillShrunkenMemory(*ppvNew, dwcbOldSize, dwcbNewSize, MEMSIZE(*ppvNew));

      ModifyHeapElement(pvOld, *ppvNew, dwcbNewSize);

      *ppvNew = GetPublicHeapPtr(*ppvNew);

      ASSERT(IS_VALID_WRITE_BUFFER_PTR(*ppvNew, BYTE, (UINT)dwcbRequestedSize));
   }

   if (IS_FLAG_SET(MdwMemoryManagerModuleFlags, MEMMGR_DFL_VALIDATE_HEAP_ON_EXIT))
      ASSERT(IsHeapOK());

#endif

   return(*ppvNew != NULL);
}


/*
** GetMemorySize()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE DWORD GetMemorySize(PVOID pv)
{
   ASSERT(IsValidHeapPtr(GetPrivateHeapPtr(pv)));

   return(MEMSIZE(pv));
}


#ifdef DEBUG

/*
** SetMemoryManagerModuleIniSwitches()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL SetMemoryManagerModuleIniSwitches(void)
{
   BOOL bResult;

   bResult = SetIniSwitches(MrgcpcvisMemoryManagerModule,
                            ARRAY_ELEMENTS(MrgcpcvisMemoryManagerModule));

   ASSERT(FLAGS_ARE_VALID(MdwMemoryManagerModuleFlags, ALL_MEMMGR_DFLAGS));

   return(bResult);
}


/*
** SpewHeapSummary()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void SpewHeapSummary(DWORD dwFlags)
{
   HEAPSUMMARY hs;

   ASSERT(FLAGS_ARE_VALID(dwFlags, SHS_FL_SPEW_USED_INFO));

   AnalyzeHeap(&hs, dwFlags);

   TRACE_OUT((TEXT("Heap summary: %lu bytes in %lu used elements."),
              hs.dwcbUsedSize,
              hs.ulcUsedElements));

   return;
}

#endif   /* DEBUG */
