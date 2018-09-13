/*
 * hndtrans.c - Handle translation module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "sortsrch.h"


/* Macros
 *********/

#define ARRAY_ELEMENT(pht, ai)   ((((PHANDLETRANS)(hht))->hpHandlePairs)[(ai)])


/* Types
 ********/

/* handle translation unit */

typedef struct _handlepair
{
   HGENERIC hgenOld;
   HGENERIC hgenNew;
}
HANDLEPAIR;
DECLARE_STANDARD_TYPES(HANDLEPAIR);

/* handle translation structure */

typedef struct _handletrans
{
   /* pointer to array of handle translation units */

   HANDLEPAIR *hpHandlePairs;

   /* number of handle pairs in array */

   LONG lcTotalHandlePairs;

   /* number of used handle pairs in array */

   LONG lcUsedHandlePairs;
}
HANDLETRANS;
DECLARE_STANDARD_TYPES(HANDLETRANS);


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE COMPARISONRESULT CompareHandlePairs(PCVOID, PCVOID);

#ifdef VSTF

PRIVATE_CODE BOOL IsValidPCHANDLETRANS(PCHANDLETRANS);
PRIVATE_CODE BOOL IsValidPCHANDLEPAIR(PCHANDLEPAIR);

#endif


/*
** CompareHandlePairs()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE COMPARISONRESULT CompareHandlePairs(PCVOID pchp1, PCVOID pchp2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pchp1, CHANDLEPAIR));
   ASSERT(IS_VALID_STRUCT_PTR(pchp2, CHANDLEPAIR));

   if (((PHANDLEPAIR)pchp1)->hgenOld < ((PHANDLEPAIR)pchp2)->hgenOld)
      cr = CR_FIRST_SMALLER;
   else if (((PHANDLEPAIR)pchp1)->hgenOld > ((PHANDLEPAIR)pchp2)->hgenOld)
      cr = CR_FIRST_LARGER;
   else
      cr = CR_EQUAL;

   return(cr);
}


#ifdef VSTF

/*
** IsValidPCHANDLETRANS()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCHANDLETRANS(PCHANDLETRANS pcht)
{
   BOOL bResult;

   if (IS_VALID_READ_PTR(pcht, CHANDLETRANS) &&
       EVAL(pcht->lcTotalHandlePairs >= 0) &&
       (EVAL(pcht->lcUsedHandlePairs >= 0) &&
        EVAL(pcht->lcUsedHandlePairs <= pcht->lcTotalHandlePairs)) &&
       IS_VALID_READ_BUFFER_PTR(pcht->hpHandlePairs, HANDLEPAIR, (UINT)(pcht->lcTotalHandlePairs)))
      bResult = TRUE;
   else
      bResult = FALSE;

   return(bResult);
}


/*
** IsValidPCHANDLEPAIR()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCHANDLEPAIR(PCHANDLEPAIR pchp)
{
   return(IS_VALID_READ_PTR(pchp, CHANDLEPAIR));
}

#endif


/****************************** Public Functions *****************************/


/*
** CreateHandleTranslator()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CreateHandleTranslator(LONG lcHandles, PHHANDLETRANS phht)
{
   PHANDLEPAIR hpHandlePairs;

   ASSERT(IS_VALID_WRITE_PTR(phht, HHANDLETRANS));

   *phht = NULL;

#ifdef DBLCHECK
   ASSERT((double)sizeof(HANDLEPAIR) * (double)lcHandles <= DWORD_MAX);
#endif

   if (AllocateMemory(sizeof(HANDLEPAIR) * lcHandles, &hpHandlePairs))
   {
      PHANDLETRANS phtNew;

      if (AllocateMemory(sizeof(*phtNew), &phtNew))
      {
         /* Success!  Fill in HANDLETRANS fields. */

         phtNew->hpHandlePairs = hpHandlePairs;
         phtNew->lcTotalHandlePairs = lcHandles;
         phtNew->lcUsedHandlePairs = 0;

         *phht = (HHANDLETRANS)phtNew;

         ASSERT(IS_VALID_HANDLE(*phht, HANDLETRANS));
      }
      else
         FreeMemory(hpHandlePairs);
   }

   return(*phht != NULL);
}


/*
** DestroyHandleTranslator()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE void DestroyHandleTranslator(HHANDLETRANS hht)
{
   ASSERT(IS_VALID_HANDLE(hht, HANDLETRANS));

   ASSERT(((PHANDLETRANS)hht)->hpHandlePairs);

   FreeMemory(((PHANDLETRANS)hht)->hpHandlePairs);

   FreeMemory((PHANDLETRANS)hht);

   return;
}


/*
** AddHandleToHandleTranslator()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL AddHandleToHandleTranslator(HHANDLETRANS hht,
                                               HGENERIC hgenOld,
                                               HGENERIC hgenNew)
{
   BOOL bRet;

   ASSERT(IS_VALID_HANDLE(hht, HANDLETRANS));

   if (((PHANDLETRANS)hht)->lcUsedHandlePairs < ((PHANDLETRANS)hht)->lcTotalHandlePairs)
   {
      ARRAY_ELEMENT((PHANDLETRANS)hht, ((PHANDLETRANS)hht)->lcUsedHandlePairs).hgenOld = hgenOld;
      ARRAY_ELEMENT((PHANDLETRANS)hht, ((PHANDLETRANS)hht)->lcUsedHandlePairs).hgenNew = hgenNew;

      ((PHANDLETRANS)hht)->lcUsedHandlePairs++;

      bRet = TRUE;
   }
   else
      bRet = FALSE;

   return(bRet);
}


/*
** PrepareForHandleTranslation()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE void PrepareForHandleTranslation(HHANDLETRANS hht)
{
   HANDLEPAIR hpTemp;

   ASSERT(IS_VALID_HANDLE(hht, HANDLETRANS));

   HeapSort(((PHANDLETRANS)hht)->hpHandlePairs,
            ((PHANDLETRANS)hht)->lcUsedHandlePairs,
            sizeof((((PHANDLETRANS)hht)->hpHandlePairs)[0]),
            &CompareHandlePairs,
            &hpTemp);

   return;
}


/*
** TranslateHandle()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL TranslateHandle(HHANDLETRANS hht, HGENERIC hgenOld,
                                   PHGENERIC phgenNew)
{
   BOOL bFound;
   HANDLEPAIR hpTemp;
   LONG liTarget;

   ASSERT(IS_VALID_HANDLE(hht, HANDLETRANS));
   ASSERT(IS_VALID_WRITE_PTR(phgenNew, HGENERIC));

   hpTemp.hgenOld = hgenOld;

   bFound = BinarySearch(((PHANDLETRANS)hht)->hpHandlePairs,
                         ((PHANDLETRANS)hht)->lcUsedHandlePairs,
                         sizeof((((PHANDLETRANS)hht)->hpHandlePairs)[0]),
                         &CompareHandlePairs,
                         &hpTemp,
                         &liTarget);

   if (bFound)
   {
      ASSERT(liTarget < ((PHANDLETRANS)hht)->lcUsedHandlePairs);

      *phgenNew = ARRAY_ELEMENT((PHANDLETRANS)hht, liTarget).hgenNew;
   }

   return(bFound);
}


#ifdef DEBUG

/*
** IsValidHHANDLETRANS()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHHANDLETRANS(HHANDLETRANS hht)
{
   return(IS_VALID_STRUCT_PTR((PHANDLETRANS)hht, CHANDLETRANS));
}

#endif

