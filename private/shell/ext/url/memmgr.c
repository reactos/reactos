/*
 * memmgr.c - Memory manager module.
 */

/*

   The memory manager implementation in this module uses the process heap.  The
debug implementation of this memory manager keeps track of memory blocks
allocated from the heap using a doubly-linked list of heap element nodes.  Each
node describes one allocated heap element.

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


#ifdef DEBUG

/* heap element byte fill values */

#define UNINITIALIZED_BYTE_VALUE    (0xcc)
#define FREED_BYTE_VALUE            (0xdd)

#endif   /* DEBUG */


/* Macros
 *********/

/* paranoid heap assertion */

#ifdef DEBUG
#define PARANOID_ASSERT(exp) \
   if (IS_FLAG_SET(s_dwMemoryManagerModuleFlags, MEMMGR_DFL_PARANOID_HEAP_VALIDATION)) \
      ASSERT(exp);
#else
#define PARANOID_ASSERT(exp)
#endif


/* Types
 ********/

#ifdef DEBUG

/* heap element descriptor structure */

typedef struct _heapelemdesc
{
   PCSTR pcszSize;
   PCSTR pcszFile;
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
   /*
    * This HEAPNODE must be the first HEAP structure field.  Sometimes a PHEAP
    * is used as a PHEAPNODE.
    */
   HEAPNODE hnHead;

   CRITICAL_SECTION cs;
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
   MEMMGR_DFL_NO_ELEMENT_FILL          = 0x0001,

   MEMMGR_DFL_VALIDATE_HEAP_ON_ENTRY   = 0x0002,

   MEMMGR_DFL_VALIDATE_HEAP_ON_EXIT    = 0x0004,

   MEMMGR_DFL_PARANOID_HEAP_VALIDATION = 0x0008,

   ALL_MEMMGR_DFLAGS                   = (MEMMGR_DFL_NO_ELEMENT_FILL |
                                          MEMMGR_DFL_VALIDATE_HEAP_ON_ENTRY |
                                          MEMMGR_DFL_VALIDATE_HEAP_ON_EXIT |
                                          MEMMGR_DFL_PARANOID_HEAP_VALIDATION)
}
MEMMGRDEBUGFLAGS;

#endif   /* DEBUG */


/* Global Variables
 *******************/

#ifdef DEBUG

#pragma data_seg(DATA_SEG_PER_INSTANCE)

/* parameters used by debug operator new() macro */

PUBLIC_DATA PCSTR g_pcszElemHdrSize = NULL;
PUBLIC_DATA PCSTR g_pcszElemHdrFile = NULL;
PUBLIC_DATA ULONG g_ulElemHdrLine = 0;

#pragma data_seg()

#endif   /* DEBUG */


/* Module Variables
 *******************/

#ifdef DEBUG

#pragma data_seg(DATA_SEG_PER_INSTANCE)

/* heap */

PRIVATE_DATA PHEAP s_pheap = NULL;

#pragma data_seg(DATA_SEG_SHARED)

/* debug flags */

PRIVATE_DATA DWORD s_dwMemoryManagerModuleFlags = 0;

#pragma data_seg(DATA_SEG_READ_ONLY)

/* heap element sentinels */

PRIVATE_DATA CONST struct
{
   BYTE rgbyte[4];
}
s_chsPrefix =
{
   { 'H', 'E', 'A', 'D' }
};

PRIVATE_DATA CONST struct
{
   BYTE rgbyte[4];
}
s_chsSuffix =
{
   { 'T', 'A', 'I', 'L' }
};

/* .ini file switch descriptions */

PRIVATE_DATA CBOOLINISWITCH s_cbisNoBufferFill =
{
   IST_BOOL,
   "DoNotFillHeapElements",
   &s_dwMemoryManagerModuleFlags,
   MEMMGR_DFL_NO_ELEMENT_FILL
};

PRIVATE_DATA CBOOLINISWITCH s_cbisValidateHeapOnEntry =
{
   IST_BOOL,
   "ValidateHeapOnEntry",
   &s_dwMemoryManagerModuleFlags,
   MEMMGR_DFL_VALIDATE_HEAP_ON_ENTRY
};

PRIVATE_DATA CBOOLINISWITCH s_cbisValidateHeapOnExit =
{
   IST_BOOL,
   "ValidateHeapOnExit",
   &s_dwMemoryManagerModuleFlags,
   MEMMGR_DFL_VALIDATE_HEAP_ON_EXIT
};

PRIVATE_DATA CBOOLINISWITCH s_cbisParanoidHeapValidation =
{
   IST_BOOL,
   "ParanoidHeapValidation",
   &s_dwMemoryManagerModuleFlags,
   MEMMGR_DFL_PARANOID_HEAP_VALIDATION
};

PRIVATE_DATA const PCVOID s_rgcpcvisMemoryManagerModule[] =
{
   &s_cbisNoBufferFill,
   &s_cbisValidateHeapOnEntry,
   &s_cbisValidateHeapOnExit,
   &s_cbisParanoidHeapValidation
};

#pragma data_seg()

#endif   /* DEBUG */


/***************************** Private Functions *****************************/


#ifdef DEBUG

PRIVATE_CODE BOOL IsValidHeapBlock(PCBYTE pcbyte, DWORD dwcbRequestedSize,
                                   DWORD dwcbAllocatedSize)
{
   /* dwcbRequestedSize may be any value. */

   return(EVAL(dwcbAllocatedSize >= sizeof(s_chsPrefix) + dwcbRequestedSize + sizeof(s_chsSuffix)) &&
          IS_VALID_READ_PTR(pcbyte, dwcbAllocatedSize) &&
          EVAL(MyMemComp(pcbyte, &s_chsPrefix, sizeof(s_chsPrefix)) == CR_EQUAL) &&
          EVAL(MyMemComp(pcbyte + sizeof(s_chsPrefix) + dwcbRequestedSize, &s_chsSuffix, sizeof(s_chsSuffix)) == CR_EQUAL));
}


PRIVATE_CODE BOOL IsValidPCHEAPELEMDESC(PCHEAPELEMDESC pched)
{
   /* ulLine may be any value. */

   return(IS_VALID_READ_PTR(pched, CHEAPELEMDESC) &&
          IS_VALID_STRING_PTR(pched->pcszSize, CSTR) &&
          IS_VALID_STRING_PTR(pched->pcszFile, CSTR));
}


PRIVATE_CODE BOOL IsValidPCHEAPNODE(PCHEAPNODE pchn)
{
   return(IS_VALID_READ_PTR(pchn, CHEAPNODE) &&
          IS_VALID_READ_PTR(pchn->phnPrev, CHEAPNODE) &&
          EVAL(pchn->phnPrev->phnNext == pchn) &&
          EVAL(! pchn->phnNext ||
               (IS_VALID_READ_PTR(pchn->phnNext, CHEAPNODE) &&
                EVAL(pchn->phnNext->phnPrev == pchn))) &&
          EVAL(IsValidHeapBlock(pchn->pcv, pchn->dwcbSize, IMemorySize((PVOID)(pchn->pcv)))) &&
          IS_VALID_STRUCT_PTR(&(pchn->hed), CHEAPELEMDESC));
}


PRIVATE_CODE BOOL IsValidPCHEAP(PCHEAP pch)
{
   return(IS_VALID_READ_PTR(pch, CHEAP) &&
          EVAL(! pch->hnHead.pcv) &&
          EVAL(! pch->hnHead.dwcbSize) &&
          EVAL(! pch->hnHead.phnPrev) &&
          EVAL(! pch->hnHead.phnNext ||
               IS_VALID_STRUCT_PTR(pch->hnHead.phnNext, CHEAPNODE)) &&
          EVAL(! pch->hnHead.hed.pcszSize) &&
          EVAL(! pch->hnHead.hed.pcszFile) &&
          EVAL(! pch->hnHead.hed.ulLine));
}


PRIVATE_CODE BOOL IsValidPrivatePtr(PCVOID pcvPrivate, DWORD dwcbPublicSize)
{
   /* dwcbPublicSize may be any value. */

   return(IS_VALID_READ_BUFFER_PTR(pcvPrivate, VOID, sizeof(s_chsPrefix) + dwcbPublicSize + sizeof(s_chsSuffix)));
}


PRIVATE_CODE BOOL IsValidPublicPtr(PCVOID pcvPublic, DWORD dwcbPublicSize)
{
   /* dwcbPublicSize may be any value. */

   return(IS_VALID_READ_BUFFER_PTR(pcvPublic, VOID, dwcbPublicSize + sizeof(s_chsSuffix)) &&
          EVAL((ULONG_PTR)pcvPublic >= sizeof(s_chsPrefix)));
}


PRIVATE_CODE DWORD CalculatePrivateSize(DWORD dwcbPublicSize)
{
   ASSERT(dwcbPublicSize <= DWORD_MAX - sizeof(s_chsPrefix) - sizeof(s_chsSuffix));

   return(dwcbPublicSize + sizeof(s_chsPrefix) + sizeof(s_chsSuffix));
}


PRIVATE_CODE PVOID GetPrivatePtr(PVOID pvPublic)
{
   PVOID pvPrivate;

   ASSERT(IsValidPublicPtr(pvPublic, 0));

   pvPrivate = (PBYTE)pvPublic - sizeof(s_chsPrefix);

   ASSERT(IsValidPrivatePtr(pvPrivate, 0));

   return(pvPrivate);
}


PRIVATE_CODE PVOID GetPublicPtr(PVOID pvPrivate)
{
   PVOID pvPublic;

   ASSERT(IsValidPrivatePtr(pvPrivate, 0));

   pvPublic = (PBYTE)pvPrivate + sizeof(s_chsPrefix);

   ASSERT(IsValidPublicPtr(pvPublic, 0));

   return(pvPublic);
}


PRIVATE_CODE BOOL FindHeapElement(PCVOID pcvTarget, PHEAPNODE *pphn)
{
   PHEAPNODE phn;

   /* pcvTarget may have been reallocated and may be any value here. */
   ASSERT(IS_VALID_WRITE_PTR(pphn, PHEAPNODE));

   *pphn = NULL;

   if (s_pheap)
   {
      for (phn = s_pheap->hnHead.phnNext; phn; phn = phn->phnNext)
      {
         /*
          * Verify each HEAPNODE structure carefully.  We may be in the middle
          * of a ModifyHeapElement() call, in which case just the target
          * HEAPNODE may be invalid, e.g., after IReallocateMemory() in
          * ReallocateMemory().
          */

         PARANOID_ASSERT((IS_VALID_READ_PTR(phn, CHEAPNODE) &&
                          phn->pcv == pcvTarget) ||
                         IS_VALID_STRUCT_PTR(phn, CHEAPNODE));

         if (phn->pcv == pcvTarget)
         {
            *pphn = phn;
            break;
         }
      }
   }

   return(*pphn != NULL);
}


PRIVATE_CODE DWORD GetPublicMemorySize(PCVOID pcv)
{
   PHEAPNODE phn;
   DWORD dwcbSize;

   ASSERT(IsValidPrivatePtr(pcv, 0));

   if (EVAL(FindHeapElement(pcv, &phn)))
      dwcbSize = phn->dwcbSize;
   else
      dwcbSize = 0;

   return(dwcbSize);
}


PRIVATE_CODE BOOL IsValidPublicHeapElement(PVOID pvPublic,
                                           DWORD dwcbPublicSize)
{
   PHEAPNODE phnUnused;

   /* dwcbPublicSize may be any value. */

   return(EVAL(IsValidPublicPtr(pvPublic, dwcbPublicSize)) &&
          EVAL(FindHeapElement(GetPrivatePtr(pvPublic), &phnUnused)));
}


PRIVATE_CODE BOOL AddHeapElement(PCVOID pcvNew, DWORD dwcbSize, PCSTR pcszSize,
                                 PCSTR pcszFile, ULONG ulLine)
{
   PHEAPNODE phnNew = NULL;
   PHEAPNODE phnUnused;

   /* ulLine may be any value. */

   ASSERT(IsValidPrivatePtr(pcvNew, dwcbSize));
   ASSERT(IS_VALID_STRING_PTR(pcszSize, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));

   /* Is the new heap element already in the list? */

   ASSERT(! FindHeapElement(pcvNew, &phnUnused));

   if (s_pheap)
   {
      /* Create new heap node. */

      if (IAllocateMemory(sizeof(*phnNew), &phnNew))
      {
         /* Fill in heap node fields. */

         phnNew->pcv = pcvNew;
         phnNew->dwcbSize = dwcbSize;

         /* Insert heap node at front of list. */

         phnNew->phnNext = s_pheap->hnHead.phnNext;
         phnNew->phnPrev = &(s_pheap->hnHead);
         s_pheap->hnHead.phnNext = phnNew;

         if (phnNew->phnNext)
            phnNew->phnNext->phnPrev = phnNew;

         /* Fill in heap element descriptor fields. */

         phnNew->hed.pcszSize = pcszSize;
         phnNew->hed.pcszFile = pcszFile;
         phnNew->hed.ulLine = ulLine;

         ASSERT(IS_VALID_STRUCT_PTR(phnNew, CHEAPNODE));
      }
   }

   ASSERT(FindHeapElement(pcvNew, &phnUnused));

   return(phnNew != NULL);
}


PRIVATE_CODE void RemoveHeapElement(PCVOID pcvOld)
{
   PHEAPNODE phnOld;

   ASSERT(IsValidPrivatePtr(pcvOld, 0));

   if (EVAL(FindHeapElement(pcvOld, &phnOld)))
   {
      /* Remove heap node from list. */

      phnOld->phnPrev->phnNext = phnOld->phnNext;

      if (phnOld->phnNext)
         phnOld->phnNext->phnPrev = phnOld->phnPrev;

      IFreeMemory(phnOld);
      phnOld = NULL;
   }

   ASSERT(! FindHeapElement(pcvOld, &phnOld));

   return;
}


PRIVATE_CODE void ModifyHeapElement(PCVOID pcvOld, PCVOID pcvNew,
                                    DWORD dwcbNewSize)
{
   PHEAPNODE phn;

   /* pcvOld has been reallocated and may be any value here. */
   ASSERT(IsValidPrivatePtr(pcvNew, dwcbNewSize));

   if (EVAL(FindHeapElement(pcvOld, &phn)))
   {
      phn->pcv = pcvNew;
      phn->dwcbSize = dwcbNewSize;

      ASSERT(IS_VALID_STRUCT_PTR(phn, CHEAPNODE));
   }

   return;
}


PRIVATE_CODE void FillNewMemory(PBYTE pbyte, DWORD dwcbRequestedSize,
                                DWORD dwcbAllocatedSize)
{
   /* dwcbRequestedSize may be any value. */

   ASSERT(dwcbAllocatedSize >= sizeof(s_chsPrefix) + sizeof(s_chsSuffix));
   ASSERT(dwcbAllocatedSize >= dwcbRequestedSize + sizeof(s_chsPrefix) + sizeof(s_chsSuffix));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pbyte, BYTE, (UINT)dwcbAllocatedSize));

   /* Fill new heap element with the uninitialized byte value. */

   if (IS_FLAG_CLEAR(s_dwMemoryManagerModuleFlags, MEMMGR_DFL_NO_ELEMENT_FILL))
      FillMemory(pbyte, dwcbAllocatedSize, UNINITIALIZED_BYTE_VALUE);

   /* Copy prefix and suffix heap element sentinels. */

   CopyMemory(pbyte, &s_chsPrefix, sizeof(s_chsPrefix));
   CopyMemory(pbyte + sizeof(s_chsPrefix) + dwcbRequestedSize, &s_chsSuffix,
              sizeof(s_chsSuffix));

   ASSERT(IsValidHeapBlock(pbyte, dwcbRequestedSize, dwcbAllocatedSize));

   return;
}


PRIVATE_CODE void FillFreedMemory(PBYTE pbyte, DWORD dwcbAllocatedSize)
{
   ASSERT(dwcbAllocatedSize >= sizeof(s_chsPrefix) + sizeof(s_chsSuffix));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pbyte, BYTE, (UINT)dwcbAllocatedSize));

   /* Fill old heap element with the freed byte value. */

   if (IS_FLAG_CLEAR(s_dwMemoryManagerModuleFlags, MEMMGR_DFL_NO_ELEMENT_FILL))
      FillMemory(pbyte, dwcbAllocatedSize, FREED_BYTE_VALUE);

   return;
}


PRIVATE_CODE void FillReallocatedMemory(PBYTE pbyte,
                                        DWORD dwcbOldRequestedSize,
                                        DWORD dwcbOldAllocatedSize,
                                        DWORD dwcbNewRequestedSize,
                                        DWORD dwcbNewAllocatedSize)
{
   ASSERT(dwcbOldAllocatedSize >= sizeof(s_chsPrefix) + dwcbOldRequestedSize + sizeof(s_chsSuffix));
   ASSERT(dwcbNewAllocatedSize >= sizeof(s_chsPrefix) + dwcbNewRequestedSize + sizeof(s_chsSuffix));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pbyte, BYTE, (UINT)dwcbNewAllocatedSize));

   /* Fill new heap element tail, if any, with uninitialized byte value. */

   ASSERT(MyMemComp(pbyte, &s_chsPrefix, sizeof(s_chsPrefix)) == CR_EQUAL);

   if (IS_FLAG_CLEAR(s_dwMemoryManagerModuleFlags, MEMMGR_DFL_NO_ELEMENT_FILL) &&
       dwcbNewAllocatedSize > sizeof(s_chsPrefix) + dwcbOldRequestedSize + sizeof(s_chsSuffix))
   {
      FillMemory(pbyte + sizeof(s_chsPrefix) + dwcbOldRequestedSize,
                 dwcbNewAllocatedSize - dwcbOldRequestedSize - sizeof(s_chsPrefix),
                 UNINITIALIZED_BYTE_VALUE);

      /*
       * Don't fill any freed heap element tail with the freed byte value since
       * that memory is no longer valid.
       */
   }

   /* Copy suffix heap element sentinel. */

   CopyMemory(pbyte + sizeof(s_chsPrefix) + dwcbNewRequestedSize, &s_chsSuffix,
              sizeof(s_chsSuffix));

   ASSERT(IsValidHeapBlock(pbyte, dwcbNewRequestedSize, dwcbNewAllocatedSize));

   return;
}


PRIVATE_CODE BOOL IsHeapOK(void)
{
   BOOL bResult;

   if (s_pheap)
   {
      PHEAPNODE phn;

      for (phn = s_pheap->hnHead.phnNext;
           phn && IS_VALID_STRUCT_PTR(phn, CHEAPNODE);
           phn = phn->phnNext)
         ;

      bResult = (phn == NULL);
   }
   else
   {
      bResult = TRUE;

      WARNING_OUT(("IsHeapOK(): No heap."));
   }

   return(bResult);
}


PRIVATE_CODE void HeapEntry(void)
{
   EnterCriticalSection(&(s_pheap->cs));

   if (IS_FLAG_SET(s_dwMemoryManagerModuleFlags, MEMMGR_DFL_VALIDATE_HEAP_ON_ENTRY))
      ASSERT(IsHeapOK());

   return;
}


PRIVATE_CODE void HeapExit(void)
{
   if (IS_FLAG_SET(s_dwMemoryManagerModuleFlags, MEMMGR_DFL_VALIDATE_HEAP_ON_EXIT))
      ASSERT(IsHeapOK());

   LeaveCriticalSection(&(s_pheap->cs));

   return;
}



PRIVATE_CODE void SpewHeapElementInfo(PCHEAPNODE pchn)
{
   char buffer[400];
   ASSERT(IS_VALID_STRUCT_PTR(pchn, CHEAPNODE));

#ifdef VERBOSE_MEMMGR
   TRACE_OUT(("Used heap element at %#lx:\r\n"
              "     %lu bytes requested\r\n"
              "     %lu bytes allocated\r\n"
              "     originally allocated as '%s' bytes in file %s at line %lu",
              pchn->pcv,
              pchn->dwcbSize,
              IMemorySize((PVOID)(pchn->pcv)),
              pchn->hed.pcszSize,
              pchn->hed.pcszFile,
              pchn->hed.ulLine));
#else
    wsprintf(buffer, ",%lu, %lu, %s, %lu, %#lx, %s", pchn->dwcbSize,
                                                    IMemorySize((PVOID)(pchn->pcv)),
                                                    pchn->hed.pcszFile,
                                                    pchn->hed.ulLine,
                                                    pchn->pcv,
                                                    pchn->hed.pcszSize );
    buffer[70] = 0;


   TRACE_OUT(("%s", buffer));
#endif


   return;
}


PRIVATE_CODE void AnalyzeHeap(PHEAPSUMMARY phs, DWORD dwFlags)
{
   PCHEAPNODE pchn;
   ULONG ulcHeapElements = 0;
   DWORD dwcbUsed = 0;

   ASSERT(IS_VALID_WRITE_PTR(phs, HEAPSUMMARY));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_SHS_FLAGS));

   ASSERT(IsHeapOK());

   TRACE_OUT(("Starting heap analysis."));

   if (s_pheap)
   {
      for (pchn = s_pheap->hnHead.phnNext;
           pchn;
           pchn = pchn->phnNext)
      {
         PARANOID_ASSERT(IS_VALID_STRUCT_PTR(pchn, CHEAPNODE));

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
      WARNING_OUT(("AnalyzeHeap(): No heap."));

   TRACE_OUT(("Heap analysis complete."));

   return;
}

#endif   /* DEBUG */


/****************************** Public Functions *****************************/


PUBLIC_CODE BOOL InitMemoryManagerModule(void)
{
   BOOL bResult;

#ifdef DEBUG

   ASSERT(! s_pheap);

   bResult = IAllocateMemory(sizeof(*s_pheap), &s_pheap);

   if (bResult)
   {
      FillMemory(s_pheap, sizeof(*s_pheap), 0);
      InitializeCriticalSection(&(s_pheap->cs));

      TRACE_OUT(("InitMemoryManagerModule(): Created debug heap."));
   }
   else
      WARNING_OUT(("InitHeapModule(): Failed to create debug heap."));

   /*
    * Don't take debug heap critical section here.  Assume this is the only
    * thread using the heap.
    */

   ASSERT(IsHeapOK());

   SpewHeapSummary(SHS_FL_SPEW_USED_INFO);

   ASSERT((bResult &&
           IS_VALID_STRUCT_PTR(s_pheap, CHEAP)) ||
          (! bResult &&
           EVAL(! s_pheap)));

#endif   /* DEBUG */

   bResult = TRUE;

   return(bResult);
}


PUBLIC_CODE void ExitMemoryManagerModule(void)
{

#ifdef DEBUG

   /*
    * Don't take debug heap critical section here.  Assume this is the only
    * thread using the heap.
    */

   ASSERT(IsHeapOK());

   SpewHeapSummary(SHS_FL_SPEW_USED_INFO);

   if (s_pheap)
   {
      DeleteCriticalSection(&(s_pheap->cs));
      IFreeMemory(s_pheap);
      s_pheap = NULL;

      TRACE_OUT(("ExitMemoryManagerModule(): Destroyed debug heap."));
   }
   else
      WARNING_OUT(("ExitMemoryManagerModule(): No heap."));

   ASSERT(! s_pheap);

#endif

   return;
}


PUBLIC_CODE COMPARISONRESULT MyMemComp(PCVOID pcv1, PCVOID pcv2,
                                       DWORD dwcbSize)
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


#ifdef DEBUG

PUBLIC_CODE BOOL DebugAllocateMemory(DWORD dwcbSize, PVOID *ppvNew,
                                     PCSTR pcszSize, PCSTR pcszFile,
                                     ULONG ulLine)
{
   BOOL bResult;
   DWORD dwcbRequestedSize;

   HeapEntry();

   /* dwcbSize may be any value. */
   /* ulLine may be any value. */

   ASSERT(IS_VALID_WRITE_PTR(ppvNew, PVOID));
   ASSERT(IS_VALID_STRING_PTR(pcszSize, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));

   dwcbRequestedSize = dwcbSize;
   dwcbSize = CalculatePrivateSize(dwcbSize);

   bResult = IAllocateMemory(dwcbSize, ppvNew);

   if (bResult)
   {
      FillNewMemory(*ppvNew, dwcbRequestedSize, IMemorySize(*ppvNew));

      if (AddHeapElement(*ppvNew, dwcbRequestedSize, pcszSize, pcszFile, ulLine))
         *ppvNew = GetPublicPtr(*ppvNew);
      else
      {
         IFreeMemory(*ppvNew);
         *ppvNew = NULL;
      }
   }

   ASSERT((bResult &&
           EVAL(IsValidPublicHeapElement(*ppvNew, dwcbRequestedSize))) ||
          (! bResult &&
           EVAL(! *ppvNew)));

   HeapExit();

   return(bResult);
}


PUBLIC_CODE BOOL DebugReallocateMemory(PVOID pvOld, DWORD dwcbNewSize,
                                       PVOID *ppvNew)
{
   BOOL bResult;
   DWORD dwcbNewRequestedSize;
   DWORD dwcbOldRequestedSize;
   DWORD dwcbOldSize;

   HeapEntry();

   /* dwcbNewSize may be any value. */

   ASSERT(IsValidPublicHeapElement(pvOld, 0));
   ASSERT(IS_VALID_WRITE_PTR(ppvNew, PVOID));

   dwcbNewRequestedSize = dwcbNewSize;
   dwcbNewSize = CalculatePrivateSize(dwcbNewSize);

   pvOld = GetPrivatePtr(pvOld);
   dwcbOldSize = IMemorySize(pvOld);

   if (dwcbNewSize == dwcbOldSize)
      WARNING_OUT(("ReallocateMemory(): Size of heap element %#lx is already %lu bytes.",
                   GetPublicPtr(pvOld),
                   dwcbNewRequestedSize));

   dwcbOldRequestedSize = GetPublicMemorySize(pvOld);

   bResult = IReallocateMemory(pvOld, dwcbNewSize, ppvNew);

   if (bResult)
   {
      FillReallocatedMemory(*ppvNew, dwcbOldRequestedSize, dwcbOldSize,
                            dwcbNewRequestedSize, IMemorySize(*ppvNew));

      ModifyHeapElement(pvOld, *ppvNew, dwcbNewRequestedSize);

      *ppvNew = GetPublicPtr(*ppvNew);
   }

   ASSERT((bResult &&
           EVAL(IsValidPublicHeapElement(*ppvNew, dwcbNewRequestedSize))) ||
          (! bResult &&
           EVAL(! *ppvNew)));

   HeapExit();

   return(bResult);
}


PUBLIC_CODE void DebugFreeMemory(PVOID pvOld)
{
   HeapEntry();

   ASSERT(IsValidPublicHeapElement(pvOld, 0));

   pvOld = GetPrivatePtr(pvOld);

   RemoveHeapElement(pvOld);

   FillFreedMemory(pvOld, IMemorySize(pvOld));

   IFreeMemory(pvOld);
   pvOld = NULL;

   HeapExit();

   return;
}


PUBLIC_CODE DWORD DebugMemorySize(PVOID pv)
{
   DWORD dwcbSize;

   HeapEntry();

   ASSERT(IsValidPublicHeapElement(pv, 0));

   pv = GetPrivatePtr(pv);

   dwcbSize = GetPublicMemorySize(pv);

   HeapExit();

   return(dwcbSize);
}


PUBLIC_CODE BOOL SetMemoryManagerModuleIniSwitches(void)
{
   BOOL bResult;

   bResult = SetIniSwitches(s_rgcpcvisMemoryManagerModule,
                            ARRAY_ELEMENTS(s_rgcpcvisMemoryManagerModule));

   ASSERT(FLAGS_ARE_VALID(s_dwMemoryManagerModuleFlags, ALL_MEMMGR_DFLAGS));

   return(bResult);
}


PUBLIC_CODE void SpewHeapSummary(DWORD dwFlags)
{
   HEAPSUMMARY hs;

   ASSERT(FLAGS_ARE_VALID(dwFlags, SHS_FL_SPEW_USED_INFO));

   AnalyzeHeap(&hs, dwFlags);

   TRACE_OUT(("Heap summary: %lu bytes in %lu used elements.",
              hs.dwcbUsedSize,
              hs.ulcUsedElements));

   return;
}

#endif   /* DEBUG */
