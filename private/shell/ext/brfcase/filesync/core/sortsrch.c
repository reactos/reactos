/*
 * sortsrch.c - Generic array sorting and searching module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "sortsrch.h"


/* Macros
 *********/

#define ARRAY_ELEMENT(hpa, ai, es)     (((PBYTE)hpa)[(ai) * (es)])


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE void HeapSwap(PVOID, LONG, LONG, size_t, PVOID);
PRIVATE_CODE void HeapSift(PVOID, LONG, LONG, size_t, COMPARESORTEDELEMSPROC, PVOID);


/*
** HeapSwap()
**
** Swaps two elements of an array.
**
** Arguments:     pvArray - pointer to array
**                li1 - index of first element
**                li2 - index of second element
**                stElemSize - length of element in bytes
**                pvTemp - pointer to temporary buffer of at least stElemSize
**                          bytes used for swapping
**
** Returns:       void
**
** Side Effects:  none
*/
PRIVATE_CODE void HeapSwap(PVOID pvArray, LONG li1, LONG li2,
                           size_t stElemSize, PVOID pvTemp)
{
   ASSERT(li1 >= 0);
   ASSERT(li2 >= 0);
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pvArray, VOID, (max(li1, li2) + 1) * stElemSize));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pvTemp, VOID, stElemSize));

   CopyMemory(pvTemp, & ARRAY_ELEMENT(pvArray, li1, stElemSize), stElemSize);
   CopyMemory(& ARRAY_ELEMENT(pvArray, li1, stElemSize), & ARRAY_ELEMENT(pvArray, li2, stElemSize), stElemSize);
   CopyMemory(& ARRAY_ELEMENT(pvArray, li2, stElemSize), pvTemp, stElemSize);

   return;
}


/*
** HeapSift()
**
** Sifts an element down in an array until the partially ordered tree property
** is restored.
**
** Arguments:     hppTable - pointer to array
**                liFirst - index of first element to sift down
**                liLast - index of last element in subtree
**                cep - pointer comparison callback function to be called to
**                      compare elements
**
** Returns:       void
**
** Side Effects:  none
*/
PRIVATE_CODE void HeapSift(PVOID pvArray, LONG liFirst, LONG liLast,
                           size_t stElemSize, COMPARESORTEDELEMSPROC cep, PVOID pvTemp)
{
   LONG li;

   ASSERT(liFirst >= 0);
   ASSERT(liLast >= 0);
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pvArray, VOID, (max(liFirst, liLast) + 1) * stElemSize));
   ASSERT(IS_VALID_CODE_PTR(cep, COMPARESORTEDELEMSPROC));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pvTemp, VOID, stElemSize));

   li = liFirst * 2;

   CopyMemory(pvTemp, & ARRAY_ELEMENT(pvArray, liFirst, stElemSize), stElemSize);

   while (li <= liLast)
   {
      if (li < liLast &&
          (*cep)(& ARRAY_ELEMENT(pvArray, li, stElemSize), & ARRAY_ELEMENT(pvArray, li + 1, stElemSize)) == CR_FIRST_SMALLER)
         li++;

      if ((*cep)(pvTemp, & ARRAY_ELEMENT(pvArray, li, stElemSize)) != CR_FIRST_SMALLER)
         break;

      CopyMemory(& ARRAY_ELEMENT(pvArray, liFirst, stElemSize), & ARRAY_ELEMENT(pvArray, li, stElemSize), stElemSize);

      liFirst = li;

      li *= 2;
   }

   CopyMemory(& ARRAY_ELEMENT(pvArray, liFirst, stElemSize), pvTemp, stElemSize);

   return;
}


#ifdef DEBUG

/*
** InSortedOrder()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL InSortedOrder(PVOID pvArray, LONG lcElements,
                                size_t stElemSize, COMPARESORTEDELEMSPROC cep)
{
   BOOL bResult = TRUE;

   ASSERT(lcElements >= 0);
   ASSERT(IS_VALID_READ_BUFFER_PTR(pvArray, VOID, lcElements * stElemSize));
   ASSERT(IS_VALID_CODE_PTR(cep, COMPARESORTEDELEMSPROC));

   if (lcElements > 1)
   {
      LONG li;

      for (li = 0; li < lcElements - 1; li++)
      {
         if ((*cep)(& ARRAY_ELEMENT(pvArray, li, stElemSize),
                    & ARRAY_ELEMENT(pvArray, li + 1, stElemSize))
             == CR_FIRST_LARGER)
         {
            bResult = FALSE;
            ERROR_OUT((TEXT("InSortedOrder(): Element [%ld] %#lx > following element [%ld] %#lx."),
                       li,
                       & ARRAY_ELEMENT(pvArray, li, stElemSize),
                       li + 1,
                       & ARRAY_ELEMENT(pvArray, li + 1, stElemSize)));
            break;
         }
      }
   }

   return(bResult);
}

#endif


/****************************** Public Functions *****************************/


/*
** HeapSort()
**
** Sorts an array.  Thanks to Rob's Dad for the cool heap sort algorithm.
**
** Arguments:     pvArray - pointer to base of array
**                lcElements - number of elements in array
**                stElemSize - length of element in bytes
**                cep - element comparison callback function
**                pvTemp - pointer to temporary buffer of at least stElemSize
**                          bytes used for swapping
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void HeapSort(PVOID pvArray, LONG lcElements, size_t stElemSize,
                          COMPARESORTEDELEMSPROC cep, PVOID pvTemp)
{
#ifdef DBLCHECK
   ASSERT((double)lcElements * (double)stElemSize <= (double)LONG_MAX);
#endif

   ASSERT(lcElements >= 0);
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pvArray, VOID, lcElements * stElemSize));
   ASSERT(IS_VALID_CODE_PTR(cep, COMPARESORTEDELEMSPROC));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pvTemp, VOID, stElemSize));

   /* Are there any elements to sort (2 or more)? */

   if (lcElements > 1)
   {
      LONG li;
      LONG liLastUsed = lcElements - 1;

      /* Yes.  Create partially ordered tree. */

      for (li = liLastUsed / 2; li >= 0; li--)
         HeapSift(pvArray, li, liLastUsed, stElemSize, cep, pvTemp);

      for (li = liLastUsed; li >= 1; li--)
      {
         /* Remove minimum from front of heap. */

         HeapSwap(pvArray, 0, li, stElemSize, pvTemp);

         /* Reestablish partially ordered tree. */

         HeapSift(pvArray, 0, li - 1, stElemSize, cep, pvTemp);
      }
   }

   ASSERT(InSortedOrder(pvArray, lcElements, stElemSize, cep));

   return;
}


/*
** BinarySearch()
**
** Searches an array for a given element.
**
** Arguments:     pvArray - pointer to base of array
**                lcElements - number of elements in array
**                stElemSize - length of element in bytes
**                cep - element comparison callback function
**                pvTarget - pointer to target element to search for
**                pliTarget - pointer to LONG to be filled in with index of
**                             target element if found
**
** Returns:       TRUE if target element found, or FALSE if not.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL BinarySearch(PVOID pvArray, LONG lcElements,
                              size_t stElemSize, COMPARESORTEDELEMSPROC cep,
                              PCVOID pcvTarget, PLONG pliTarget)
{
   BOOL bFound = FALSE;

#ifdef DBLCHECK
   ASSERT((double)lcElements * (double)stElemSize <= (double)ULONG_MAX);
#endif

   ASSERT(lcElements >= 0);
   ASSERT(IS_VALID_READ_BUFFER_PTR(pvArray, VOID, lcElements * stElemSize));
   ASSERT(IS_VALID_CODE_PTR(cep, COMPARESORTEDELEMSPROC));
   ASSERT(IS_VALID_READ_BUFFER_PTR(pcvTarget, VOID, stElemSize));
   ASSERT(IS_VALID_WRITE_PTR(pliTarget, LONG));

   /* Are there any elements to search through? */

   if (lcElements > 0)
   {
      LONG liLow = 0;
      LONG liMiddle = 0;
      LONG liHigh = lcElements - 1;
      COMPARISONRESULT cr = CR_EQUAL;

      /* Yes.  Search for the target element. */

      /*
       * At the end of the penultimate iteration of this loop:
       *
       * liLow == liMiddle == liHigh.
       */

      while (liLow <= liHigh)
      {
         liMiddle = (liLow + liHigh) / 2;

         cr = (*cep)(pcvTarget, & ARRAY_ELEMENT(pvArray, liMiddle, stElemSize));

         if (cr == CR_FIRST_SMALLER)
            liHigh = liMiddle - 1;
         else if (cr == CR_FIRST_LARGER)
            liLow = liMiddle + 1;
         else
         {
            *pliTarget = liMiddle;
            bFound = TRUE;
            break;
         }
      }
   }

   return(bFound);
}

