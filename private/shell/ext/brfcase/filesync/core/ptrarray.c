/*
 * ptrarray.c - Pointer array ADT module.
 */

/*

   Pointer array structures are allocated by AllocateMemory().  Handles to
pointer arrays are pointers to ARRAYs.  Each ARRAY contains a pointer to an
rray of pointers in the pointer array.  Each array of pointers is allocated by
GlobalAlloc() to allow it to grow to greater than 64 Kb in size.  Pointer
arrays are 0-based.  Array elements are accessed using pointers.  If this
proves to be too slow, we'll go to pointers with a 64 Kb limit on total array
size.

   Pointer arrays are created with pointer comparison functions for sorting and
searching.  The sorting comparison function is used to insert new pointers into
the pointer array in sorted order.  The searching comparison function is used
to search the sorted pointer array for a pointer.  The sorting comparison
function is passed the pointer to be added to the pointer array and a pointer
from the pointer array for comparison.  The searching comparison function is
passed a pointer to the information being searched for and a pointer from the
pointer array for comparison.

*/


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/* Macros
 *********/

/* extract array element */

#define ARRAY_ELEMENT(ppa, ai)            (((ppa)->ppcvArray)[(ai)])

/* Add pointers to array in sorted order? */

#define ADD_PTRS_IN_SORTED_ORDER(ppa)     IS_FLAG_SET((ppa)->dwFlags, PA_FL_SORTED_ADD)


/* Types
 ********/

/* pointer array flags */

typedef enum _ptrarrayflags
{
   /* Insert elements in sorted order. */

   PA_FL_SORTED_ADD        = 0x0001,

   /* flag combinations */

   ALL_PA_FLAGS            = PA_FL_SORTED_ADD
}
PTRARRAYFLAGS;

/* pointer array structure */

/*
 * Free elements in the ppcvArray[] array lie between indexes (aicPtrsUsed)
 * and (aiLast), inclusive.
 */

typedef struct _ptrarray
{
   /* elements to grow array by after it fills up */

   ARRAYINDEX aicPtrsToGrowBy;

   /* array flags */

   DWORD dwFlags;

   /* pointer to base of array */

   PCVOID *ppcvArray;

   /* index of last element allocated in array */

   ARRAYINDEX aicPtrsAllocated;

   /*
    * (We keep a count of the number of elements used instead of the index of
    * the last element used so that this value is 0 for an empty array, and not
    * some non-zero sentinel value.)
    */

   /* number of elements used in array */

   ARRAYINDEX aicPtrsUsed;
}
PTRARRAY;
DECLARE_STANDARD_TYPES(PTRARRAY);


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE BOOL AddAFreePtrToEnd(PPTRARRAY);
PRIVATE_CODE void PtrHeapSwap(PPTRARRAY, ARRAYINDEX, ARRAYINDEX);
PRIVATE_CODE void PtrHeapSift(PPTRARRAY, ARRAYINDEX, ARRAYINDEX, COMPARESORTEDPTRSPROC);

#ifdef VSTF

PRIVATE_CODE BOOL IsValidPCNEWPTRARRAY(PCNEWPTRARRAY);
PRIVATE_CODE BOOL IsValidPCPTRARRAY(PCPTRARRAY);

#endif

#if defined(DEBUG) || defined(VSTF)

PRIVATE_CODE BOOL IsPtrArrayInSortedOrder(PCPTRARRAY, COMPARESORTEDPTRSPROC);

#endif


/*
** AddAFreePtrToEnd()
**
** Adds a free element to the end of an array.
**
** Arguments:     pa - pointer to array
**
** Returns:       TRUE if successful, or FALSE if not.
**
** Side Effects:  May grow the array.
*/
PRIVATE_CODE BOOL AddAFreePtrToEnd(PPTRARRAY pa)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(pa, CPTRARRAY));

   /* Are there any free elements in the array? */

   if (pa->aicPtrsUsed < pa->aicPtrsAllocated)
      /* Yes.  Return the next free pointer. */
      bResult = TRUE;
   else
   {
      ARRAYINDEX aicNewPtrs = pa->aicPtrsAllocated + pa->aicPtrsToGrowBy;
      PCVOID *ppcvArray;

      bResult = FALSE;

      /* Try to grow the array. */

      /* Blow off unlikely overflow conditions as ASSERT()s. */

      ASSERT(pa->aicPtrsAllocated <= ARRAYINDEX_MAX + 1);
      ASSERT(ARRAYINDEX_MAX + 1 - pa->aicPtrsToGrowBy >= pa->aicPtrsAllocated);
#ifdef DBLCHECK
      ASSERT((double)aicNewPtrs * (double)(sizeof(PVOID)) <= (double)DWORD_MAX);
#endif

      /* Try to grow the array. */

      if (ReallocateMemory((PVOID)(pa->ppcvArray), aicNewPtrs * sizeof(*ppcvArray), (PVOID *)(&ppcvArray)))
      {
         /*
          * Array reallocated successfully.  Set up PTRARRAY fields, and return
          * the first free index.
          */

         pa->ppcvArray = ppcvArray;
         pa->aicPtrsAllocated = aicNewPtrs;

         bResult = TRUE;
      }
   }

   return(bResult);
}


/*
** PtrHeapSwap()
**
** Swaps two elements in an array.
**
** Arguments:     pa - pointer to array
**                aiFirst - index of first element
**                aiSecond - index of second element
**
** Returns:       void
**
** Side Effects:  none
*/
PRIVATE_CODE void PtrHeapSwap(PPTRARRAY pa, ARRAYINDEX ai1, ARRAYINDEX ai2)
{
   PCVOID pcvTemp;

   ASSERT(IS_VALID_STRUCT_PTR(pa, CPTRARRAY));
   ASSERT(ai1 >= 0);
   ASSERT(ai1 < pa->aicPtrsUsed);
   ASSERT(ai2 >= 0);
   ASSERT(ai2 < pa->aicPtrsUsed);

   pcvTemp = ARRAY_ELEMENT(pa, ai1);
   ARRAY_ELEMENT(pa, ai1) = ARRAY_ELEMENT(pa, ai2);
   ARRAY_ELEMENT(pa, ai2) = pcvTemp;

   return;
}


/*
** PtrHeapSift()
**
** Sifts an element down in an array until the partially ordered tree property
** is retored.
**
** Arguments:     pa - pointer to array
**                aiFirst - index of element to sift down
**                aiLast - index of last element in subtree
**                cspp - element comparison callback function to be called to
**                      compare elements
**
** Returns:       void
**
** Side Effects:  none
*/
PRIVATE_CODE void PtrHeapSift(PPTRARRAY pa, ARRAYINDEX aiFirst, ARRAYINDEX aiLast,
                         COMPARESORTEDPTRSPROC cspp)
{
   ARRAYINDEX ai;
   PCVOID pcvTemp;

   ASSERT(IS_VALID_STRUCT_PTR(pa, CPTRARRAY));
   ASSERT(IS_VALID_CODE_PTR(cspp, COMPARESORTEDPTRSPROC));

   ASSERT(aiFirst >= 0);
   ASSERT(aiFirst < pa->aicPtrsUsed);
   ASSERT(aiLast >= 0);
   ASSERT(aiLast < pa->aicPtrsUsed);

   ai = aiFirst * 2;

   pcvTemp = ARRAY_ELEMENT(pa, aiFirst);

   while (ai <= aiLast)
   {
      if (ai < aiLast &&
          (*cspp)(ARRAY_ELEMENT(pa, ai), ARRAY_ELEMENT(pa, ai + 1)) == CR_FIRST_SMALLER)
         ai++;

      if ((*cspp)(pcvTemp, ARRAY_ELEMENT(pa, ai)) != CR_FIRST_SMALLER)
         break;

      ARRAY_ELEMENT(pa, aiFirst) = ARRAY_ELEMENT(pa, ai);

      aiFirst = ai;

      ai *= 2;
   }

   ARRAY_ELEMENT(pa, aiFirst) = pcvTemp;

   return;
}


#ifdef VSTF

/*
** IsValidPCNEWPTRARRAY()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCNEWPTRARRAY(PCNEWPTRARRAY pcnpa)
{
   BOOL bResult;

   /*
    * Given the current value of ARRAYINDEX_MAX (ULONG_MAX - 1), we don't
    * really need a check like:
    *
    *    (pcna->aicInitialPtrs - 1 <= ARRAYINDEX_MAX)
    *
    * since the maximum value of the aicInitialPtrs field (ULONG_MAX) still
    * yields a valid top index:
    *
    *    (ULONG_MAX) - 1 == (ULONG_MAX - 1)
    *
    *    ARRAYINDEX_MAX == (ULONG_MAX - 1)
    *
    * But we'll leave the clause here anyway in case things change.
    */

   if (IS_VALID_READ_PTR(pcnpa, CNEWPTRARRAY) &&
       EVAL(pcnpa->aicInitialPtrs >= 0) &&
       EVAL(pcnpa->aicInitialPtrs < ARRAYINDEX_MAX) &&
       EVAL(pcnpa->aicAllocGranularity > 0) &&
       FLAGS_ARE_VALID(pcnpa->dwFlags, ALL_NPA_FLAGS))
      bResult = TRUE;
   else
      bResult = FALSE;

   return(bResult);
}


/*
** IsValidPCPTRARRAY()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCPTRARRAY(PCPTRARRAY pcpa)
{
   BOOL bResult;

   if (IS_VALID_READ_PTR(pcpa, CPTRARRAY) &&
       EVAL(pcpa->aicPtrsToGrowBy > 0) &&
       FLAGS_ARE_VALID(pcpa->dwFlags, ALL_PA_FLAGS) &&
       EVAL(pcpa->aicPtrsAllocated >= 0) &&
       IS_VALID_READ_BUFFER_PTR(pcpa->ppcvArray, PCVOID, (pcpa->aicPtrsAllocated) * sizeof(*(pcpa->ppcvArray))) &&
       (EVAL(pcpa->aicPtrsUsed >= 0) &&
        EVAL(pcpa->aicPtrsUsed <= pcpa->aicPtrsAllocated)))
      bResult = TRUE;
   else
      bResult = FALSE;

   return(bResult);
}

#endif


#if defined(DEBUG) || defined(VSTF)

/*
** IsPtrArrayInSortedOrder()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsPtrArrayInSortedOrder(PCPTRARRAY pcpa,
                                          COMPARESORTEDPTRSPROC cspp)
{
   BOOL bResult = TRUE;

   /* Don't validate pcpa here. */

   ASSERT(IS_VALID_CODE_PTR(cspp, COMPARESORTEDPTRSPROC));

   if (pcpa->aicPtrsUsed > 1)
   {
      ARRAYINDEX ai;

      for (ai = 0; ai < pcpa->aicPtrsUsed - 1; ai++)
      {
         if ((*cspp)(ARRAY_ELEMENT(pcpa, ai), ARRAY_ELEMENT(pcpa, ai + 1))
             == CR_FIRST_LARGER)
         {
            bResult = FALSE;
            ERROR_OUT((TEXT("IsPtrArrayInSortedOrder(): Element [%ld] %#lx > following element [%ld] %#lx."),
                       ai,
                       ARRAY_ELEMENT(pcpa, ai),
                       ai + 1,
                       ARRAY_ELEMENT(pcpa, ai + 1)));
            break;
         }
      }
   }

   return(bResult);
}

#endif


/****************************** Public Functions *****************************/


/*
** CreatePtrArray()
**
** Creates a pointer array.
**
** Arguments:     pcna - pointer to NEWPTRARRAY describing the array to be
**                        created
**
** Returns:       Handle to the new array if successful, or NULL if
**                unsuccessful.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CreatePtrArray(PCNEWPTRARRAY pcna, PHPTRARRAY phpa)
{
   PCVOID *ppcvArray;

   ASSERT(IS_VALID_STRUCT_PTR(pcna, CNEWPTRARRAY));
   ASSERT(IS_VALID_WRITE_PTR(phpa, HPTRARRAY));

   /* Try to allocate the initial array. */

   *phpa = NULL;

   if (AllocateMemory(pcna->aicInitialPtrs * sizeof(*ppcvArray), (PVOID *)(&ppcvArray)))
   {
      PPTRARRAY pa;

      /* Try to allocate PTRARRAY structure. */

      if (AllocateMemory(sizeof(*pa), &pa))
      {
         /* Initialize PTRARRAY fields. */

         pa->aicPtrsToGrowBy = pcna->aicAllocGranularity;
         pa->ppcvArray = ppcvArray;
         pa->aicPtrsAllocated = pcna->aicInitialPtrs;
         pa->aicPtrsUsed = 0;

         /* Set flags. */

         if (IS_FLAG_SET(pcna->dwFlags, NPA_FL_SORTED_ADD))
            pa->dwFlags = PA_FL_SORTED_ADD;
         else
            pa->dwFlags = 0;

         *phpa = (HPTRARRAY)pa;

         ASSERT(IS_VALID_HANDLE(*phpa, PTRARRAY));
      }
      else
         /* Unlock and free array (ignoring return values). */
         FreeMemory((PVOID)(ppcvArray));
   }

   return(*phpa != NULL);
}


/*
** DestroyPtrArray()
**
** Destroys an array.
**
** Arguments:     hpa - handle to array to be destroyed
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void DestroyPtrArray(HPTRARRAY hpa)
{
   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));

   /* Free the array. */

   ASSERT(((PCPTRARRAY)hpa)->ppcvArray);

   FreeMemory((PVOID)(((PCPTRARRAY)hpa)->ppcvArray));

   /* Free PTRARRAY structure. */

   FreeMemory((PPTRARRAY)hpa);

   return;
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

/*
** InsertPtr()
**
** Adds an element to an array at a given index.
**
** Arguments:     hpa - handle to array that element is to be added to
**                aiInsert - index where new element is to be inserted
**                pcvNew - pointer to element to add to array
**
** Returns:       TRUE if the element was inserted successfully, or FALSE if
**                not.
**
** Side Effects:  The array may be grown.
**
** N.b., for an array marked PA_FL_SORTED_ADD, this index should only be
** retrieved using SearchSortedArray(), or the sorted order will be destroyed.
*/
PUBLIC_CODE BOOL InsertPtr(HPTRARRAY hpa, COMPARESORTEDPTRSPROC cspp, ARRAYINDEX aiInsert, PCVOID pcvNew)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));
   ASSERT(aiInsert >= 0);
   ASSERT(aiInsert <= ((PCPTRARRAY)hpa)->aicPtrsUsed);

#ifdef DEBUG

   /* Make sure the correct index was given for insertion. */

   if (ADD_PTRS_IN_SORTED_ORDER((PCPTRARRAY)hpa))
   {
      ARRAYINDEX aiNew;

      EVAL(! SearchSortedArray(hpa, cspp, pcvNew, &aiNew));

      ASSERT(aiInsert == aiNew);
   }

#endif

   /* Get a free element in the array. */

   bResult = AddAFreePtrToEnd((PPTRARRAY)hpa);

   if (bResult)
   {
      ASSERT(((PCPTRARRAY)hpa)->aicPtrsUsed < ARRAYINDEX_MAX);

      /* Open a slot for the new element. */

      MoveMemory((PVOID)& ARRAY_ELEMENT((PPTRARRAY)hpa, aiInsert + 1),
                 & ARRAY_ELEMENT((PPTRARRAY)hpa, aiInsert),
                 (((PCPTRARRAY)hpa)->aicPtrsUsed - aiInsert) * sizeof(ARRAY_ELEMENT((PCPTRARRAY)hpa, 0)));

      /* Put the new element in the open slot. */

      ARRAY_ELEMENT((PPTRARRAY)hpa, aiInsert) = pcvNew;

      ((PPTRARRAY)hpa)->aicPtrsUsed++;
   }

   return(bResult);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


/*
** AddPtr()
**
** Adds an element to an array, in sorted order if so specified at
** CreatePtrArray() time.
**
** Arguments:     hpa - handle to array that element is to be added to
**                pcvNew - pointer to element to be added to array
**                pai - pointer to ARRAYINDEX to be filled in with index of
**                      new element, may be NULL
**
** Returns:       TWINRESULT
**
** Side Effects:  The array may be grown.
*/
PUBLIC_CODE BOOL AddPtr(HPTRARRAY hpa, COMPARESORTEDPTRSPROC cspp, PCVOID pcvNew, PARRAYINDEX pai)
{
   BOOL bResult;
   ARRAYINDEX aiNew;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));
   ASSERT(! pai || IS_VALID_WRITE_PTR(pai, ARRAYINDEX));

   /* Find out where the new element should go. */

   if (ADD_PTRS_IN_SORTED_ORDER((PCPTRARRAY)hpa))
      EVAL(! SearchSortedArray(hpa, cspp, pcvNew, &aiNew));
   else
      aiNew = ((PCPTRARRAY)hpa)->aicPtrsUsed;

   bResult = InsertPtr(hpa, cspp, aiNew, pcvNew);

   if (bResult && pai)
      *pai = aiNew;

   return(bResult);
}


/*
** DeletePtr()
**
** Removes an element from an element array.
**
** Arguments:     ha - handle to array
**                aiDelete - index of element to be deleted
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE void DeletePtr(HPTRARRAY hpa, ARRAYINDEX aiDelete)
{
   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));
   ASSERT(aiDelete >= 0);
   ASSERT(aiDelete < ((PCPTRARRAY)hpa)->aicPtrsUsed);

   /*
    * Compact the element array by moving down all elements past the one being
    * deleted.
    */

   MoveMemory((PVOID)& ARRAY_ELEMENT((PPTRARRAY)hpa, aiDelete),
              & ARRAY_ELEMENT((PPTRARRAY)hpa, aiDelete + 1),
              (((PCPTRARRAY)hpa)->aicPtrsUsed - aiDelete - 1) * sizeof(ARRAY_ELEMENT((PCPTRARRAY)hpa, 0)));

   /* One less element used. */

   ((PPTRARRAY)hpa)->aicPtrsUsed--;

   return;
}


/*
** DeleteAllPtrs()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DeleteAllPtrs(HPTRARRAY hpa)
{
   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));

   ((PPTRARRAY)hpa)->aicPtrsUsed = 0;

   return;
}


/*
** GetPtrCount()
**
** Retrieves the number of elements in an element array.
**
** Arguments:     hpa - handle to array
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE ARRAYINDEX GetPtrCount(HPTRARRAY hpa)
{
   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));

   return(((PCPTRARRAY)hpa)->aicPtrsUsed);
}


/*
** GetPtr()
**
** Retrieves an element from an array.
**
** Arguments:     hpa - handle to array
**                ai - index of element to be retrieved
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE PVOID GetPtr(HPTRARRAY hpa, ARRAYINDEX ai)
{
   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));
   ASSERT(ai >= 0);
   ASSERT(ai < ((PCPTRARRAY)hpa)->aicPtrsUsed);

   return((PVOID)ARRAY_ELEMENT((PCPTRARRAY)hpa, ai));
}


/*
** SortPtrArray()
**
** Sorts an array.
**
** Arguments:     hpa - handle to element list to be sorted
**                cspp - pointer comparison callback function
**
** Returns:       void
**
** Side Effects:  none
**
** Uses heap sort.
*/
PUBLIC_CODE void SortPtrArray(HPTRARRAY hpa, COMPARESORTEDPTRSPROC cspp)
{
   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));

   /* Are there any elements to sort (2 or more)? */

   if (((PCPTRARRAY)hpa)->aicPtrsUsed > 1)
   {
      ARRAYINDEX ai;
      ARRAYINDEX aiLastUsed = ((PCPTRARRAY)hpa)->aicPtrsUsed - 1;

      /* Yes.  Create partially ordered tree. */

      for (ai = aiLastUsed / 2; ai >= 0; ai--)
         PtrHeapSift((PPTRARRAY)hpa, ai, aiLastUsed, cspp);

      for (ai = aiLastUsed; ai >= 1; ai--)
      {
         /* Remove minimum from front of heap. */

         PtrHeapSwap((PPTRARRAY)hpa, 0, ai);

         /* Reestablish partially ordered tree. */

         PtrHeapSift((PPTRARRAY)hpa, 0, ai - 1, cspp);
      }
   }

   ASSERT(IsPtrArrayInSortedOrder((PCPTRARRAY)hpa, cspp));

   return;
}


/*
** SearchSortedArray()
**
** Searches an array for a target element using binary search.  If several
** adjacent elements match the target element, the index of the first matching
** element is returned.
**
** Arguments:     hpa - handle to array to be searched
**                cspp - element comparison callback function to be called to
**                      compare the target element with an element from the
**                      array, the callback function is called as:
**
**                         (*cspp)(pcvTarget, pcvPtrFromList)
**
**                pcvTarget - pointer to target element to search for
**                pbFound - pointer to BOOL to be filled in with TRUE if the
**                          target element is found, or FALSE if not
**                paiTarget - pointer to ARRAYINDEX to be filled in with the
**                            index of the first element matching the target
**                            element if found, otherwise filled in with the
**                            index where the target element should be
**                            inserted
**
** Returns:       TRUE if target element is found.  FALSE if not.
**
** Side Effects:  none
**
** We use a private version of SearchSortedArray() instead of the CRT bsearch()
** function since we want it to return the insertion index of the target
** element if the target element is not found.
*/
PUBLIC_CODE BOOL SearchSortedArray(HPTRARRAY hpa, COMPARESORTEDPTRSPROC cspp,
                                   PCVOID pcvTarget, PARRAYINDEX paiTarget)
{
   BOOL bFound;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));
   ASSERT(IS_VALID_CODE_PTR(cspp, COMPARESORTEDPTRSPROC));
   ASSERT(IS_VALID_WRITE_PTR(paiTarget, ARRAYINDEX));

   ASSERT(ADD_PTRS_IN_SORTED_ORDER((PCPTRARRAY)hpa));
#if 0
   ASSERT(IsPtrArrayInSortedOrder((PCPTRARRAY)hpa, ((PCPTRARRAY)hpa)->cspp));
#endif

   bFound = FALSE;

   /* Are there any elements to search through? */

   if (((PCPTRARRAY)hpa)->aicPtrsUsed > 0)
   {
      ARRAYINDEX aiLow = 0;
      ARRAYINDEX aiMiddle = 0;
      ARRAYINDEX aiHigh = ((PCPTRARRAY)hpa)->aicPtrsUsed - 1;
      COMPARISONRESULT cr = CR_EQUAL;

      /* Yes.  Search for the target element. */

      /*
       * At the end of the penultimate iteration of this loop:
       *
       * aiLow == aiMiddle == aiHigh.
       */

      ASSERT(aiHigh <= ARRAYINDEX_MAX);

      while (aiLow <= aiHigh)
      {
         aiMiddle = (aiLow + aiHigh) / 2;

         cr = (*cspp)(pcvTarget, ARRAY_ELEMENT((PCPTRARRAY)hpa, aiMiddle));

         if (cr == CR_FIRST_SMALLER)
            aiHigh = aiMiddle - 1;
         else if (cr == CR_FIRST_LARGER)
            aiLow = aiMiddle + 1;
         else
         {
            /*
             * Found a match at index aiMiddle.  Search back for first match.
             */

            bFound = TRUE;

            while (aiMiddle > 0)
            {
               if ((*cspp)(pcvTarget, ARRAY_ELEMENT((PCPTRARRAY)hpa, aiMiddle - 1)) != CR_EQUAL)
                  break;
               else
                  aiMiddle--;
            }

            break;
         }
      }

      /*
       * Return the index of the target if found, or the index where the target
       * should be inserted if not found.
       */

      /*
       * If (cr == CR_FIRST_LARGER), the insertion index is aiLow.
       *
       * If (cr == CR_FIRST_SMALLER), the insertion index is aiMiddle.
       *
       * If (cr == CR_EQUAL), the insertion index is aiMiddle.
       */

      if (cr == CR_FIRST_LARGER)
         *paiTarget = aiLow;
      else
         *paiTarget = aiMiddle;
   }
   else
      /*
       * No.  The target element cannot be found in an empty array.  It should
       * be inserted as the first element.
       */
      *paiTarget = 0;

   ASSERT(*paiTarget <= ((PCPTRARRAY)hpa)->aicPtrsUsed);

   return(bFound);
}


/*
** LinearSearchArray()
**
** Searches an array for a target element using binary search.  If several
** adjacent elements match the target element, the index of the first matching
** element is returned.
**
** Arguments:     hpa - handle to array to be searched
**                cupp - element comparison callback function to be called to
**                       compare the target element with an element from the
**                       array, the callback function is called as:
**
**                         (*cupp)(pvTarget, pvPtrFromList)
**
**                      the callback function should return a value based upon
**                      the result of the element comparison as follows:
**
**                         FALSE, pvTarget == pvPtrFromList
**                         TRUE,  pvTarget != pvPtrFromList
**
**                pvTarget - far element to target element to search for
**                paiTarget - far element to ARRAYINDEX to be filled in with
**                            the index of the first matching element if
**                            found, otherwise filled in with index where
**                            element should be inserted
**
** Returns:       TRUE if target element is found.  FALSE if not.
**
** Side Effects:  none
**
** We use a private version of LinearSearchForPtr() instead of the CRT _lfind()
** function since we want it to return the insertion index of the target
** element if the target element is not found.
**
** If the target element is not found the insertion index returned is the first
** element after the last used element in the array.
*/
PUBLIC_CODE BOOL LinearSearchArray(HPTRARRAY hpa, COMPAREUNSORTEDPTRSPROC cupp,
                                   PCVOID pcvTarget, PARRAYINDEX paiTarget)
{
   BOOL bFound;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY) &&
          (! cupp || IS_VALID_CODE_PTR(cupp, COMPPTRSPROC)) &&
          IS_VALID_WRITE_PTR(paiTarget, ARRAYINDEX));

   bFound = FALSE;

   for (ai = 0; ai < ((PCPTRARRAY)hpa)->aicPtrsUsed; ai++)
   {
      if (! (*cupp)(pcvTarget, ARRAY_ELEMENT((PCPTRARRAY)hpa, ai)))
      {
         bFound = TRUE;
         break;
      }
   }

   if (bFound)
      *paiTarget = ai;

   return(bFound);
}


#if defined(DEBUG) || defined(VSTF)

/*
** IsValidHPTRARRAY()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHPTRARRAY(HPTRARRAY hpa)
{
   return(IS_VALID_STRUCT_PTR((PCPTRARRAY)hpa, CPTRARRAY));
}

#endif


#ifdef VSTF

/*
** IsValidHGLOBAL()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHGLOBAL(HGLOBAL hg)
{
   return(EVAL(hg != NULL));
}

#endif

