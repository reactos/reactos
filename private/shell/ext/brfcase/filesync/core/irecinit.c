/*
 * irecinit.c - CReconcileInitiator implementation.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "oleutil.h"
#include "irecinit.h"


/* Types
 ********/

/* ReconcileInitiator class */

typedef struct _creconcileinitiator
{
   /* IReconcileInitiator */

   IReconcileInitiator irecinit;

   /* IBriefcaseInitiator */

   IBriefcaseInitiator ibcinit;

   /* reference count */

   ULONG ulcRef;

   /* handle to parent briefcase */

   HBRFCASE hbr;

   /* status callback function */

   RECSTATUSPROC rsp;

   /* status callback function data */

   LPARAM lpCallbackData;

   /* IUnknown to release to abort reconciliation. */

   PIUnknown piunkForAbort;
}
CReconcileInitiator;
DECLARE_STANDARD_TYPES(CReconcileInitiator);


/* Module Prototypes
 ********************/

PRIVATE_CODE HRESULT ReconcileInitiator_QueryInterface(PCReconcileInitiator, REFIID, PVOID *);
PRIVATE_CODE ULONG ReconcileInitiator_AddRef(PCReconcileInitiator);
PRIVATE_CODE ULONG ReconcileInitiator_Release(PCReconcileInitiator);
PRIVATE_CODE HRESULT ReconcileInitiator_SetAbortCallback(PCReconcileInitiator, PIUnknown);
PRIVATE_CODE HRESULT ReconcileInitiator_SetProgressFeedback(PCReconcileInitiator, ULONG, ULONG);

PRIVATE_CODE HRESULT AbortReconciliation(PCReconcileInitiator);

PRIVATE_CODE HRESULT RI_IReconcileInitiator_QueryInterface(PIReconcileInitiator, REFIID, PVOID *);
PRIVATE_CODE ULONG RI_IReconcileInitiator_AddRef(PIReconcileInitiator);
PRIVATE_CODE ULONG RI_IReconcileInitiator_Release(PIReconcileInitiator);
PRIVATE_CODE HRESULT RI_IReconcileInitiator_SetAbortCallback(PIReconcileInitiator, PIUnknown);
PRIVATE_CODE HRESULT RI_IReconcileInitiator_SetProgressFeedback( PIReconcileInitiator, ULONG, ULONG);

PRIVATE_CODE HRESULT RI_IBriefcaseInitiator_QueryInterface(PIBriefcaseInitiator, REFIID, PVOID *);
PRIVATE_CODE ULONG RI_IBriefcaseInitiator_AddRef(PIBriefcaseInitiator);
PRIVATE_CODE ULONG RI_IBriefcaseInitiator_Release(PIBriefcaseInitiator);
PRIVATE_CODE HRESULT RI_IBriefcaseInitiator_IsMonikerInBriefcase(PIBriefcaseInitiator, PIMoniker);

#ifdef DEBUG

PRIVATE_CODE BOOL IsValidPCCReconcileInitiator(PCCReconcileInitiator);
PRIVATE_CODE BOOL IsValidPCIBriefcaseInitiator(PCIBriefcaseInitiator);

#endif


/* Module Variables
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

/* IReconcileInitiator vtable */

PRIVATE_DATA IReconcileInitiatorVtbl Mcirecinitvtbl =
{
   &RI_IReconcileInitiator_QueryInterface,
   &RI_IReconcileInitiator_AddRef,
   &RI_IReconcileInitiator_Release,
   &RI_IReconcileInitiator_SetAbortCallback,
   &RI_IReconcileInitiator_SetProgressFeedback
};

/* IBriefcaseInitiator vtable */

PRIVATE_DATA IBriefcaseInitiatorVtbl Mcibcinitvtbl =
{
   &RI_IBriefcaseInitiator_QueryInterface,
   &RI_IBriefcaseInitiator_AddRef,
   &RI_IBriefcaseInitiator_Release,
   &RI_IBriefcaseInitiator_IsMonikerInBriefcase
};

#pragma data_seg()


/***************************** Private Functions *****************************/


/*
** ReconcileInitiator_QueryInterface()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT ReconcileInitiator_QueryInterface(
                                                PCReconcileInitiator precinit,
                                                REFIID riid, PVOID *ppvObject)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRUCT_PTR(precinit, CCReconcileInitiator));
   //ASSERT(IsValidREFIID(riid));
   ASSERT(IS_VALID_WRITE_PTR(ppvObject, PVOID));

   if (IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid, &IID_IReconcileInitiator))
   {
      *ppvObject = &(precinit->irecinit);
      precinit->irecinit.lpVtbl->AddRef(&(precinit->irecinit));
      hr = S_OK;
   }
   else if (IsEqualIID(riid, &IID_IBriefcaseInitiator))
   {
      *ppvObject = &(precinit->ibcinit);
      precinit->ibcinit.lpVtbl->AddRef(&(precinit->ibcinit));
      hr = S_OK;
   }
   else
      hr = E_NOINTERFACE;

   return(hr);
}


/*
** ReconcileInitiator_AddRef()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE ULONG ReconcileInitiator_AddRef(PCReconcileInitiator precinit)
{
   ASSERT(IS_VALID_STRUCT_PTR(precinit, CCReconcileInitiator));

   ASSERT(precinit->ulcRef < ULONG_MAX);
   return(++(precinit->ulcRef));
}


/*
** ReconcileInitiator_Release()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE ULONG ReconcileInitiator_Release(PCReconcileInitiator precinit)
{
   ULONG ulcRef;

   ASSERT(IS_VALID_STRUCT_PTR(precinit, CCReconcileInitiator));

   if (EVAL(precinit->ulcRef > 0))
      precinit->ulcRef--;

   ulcRef = precinit->ulcRef;

   if (! precinit->ulcRef)
      FreeMemory(precinit);

   return(ulcRef);
}


/*
** ReconcileInitiator_SetAbortCallback()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT ReconcileInitiator_SetAbortCallback(
                                                PCReconcileInitiator precinit,
                                                PIUnknown piunkForAbort)
{
   ASSERT(IS_VALID_STRUCT_PTR(precinit, CCReconcileInitiator));
   /* piunkForAbort can be legally NULL */
   ASSERT(NULL == piunkForAbort || IS_VALID_STRUCT_PTR(piunkForAbort, CIUnknown));

   precinit->piunkForAbort = piunkForAbort;

   return(S_OK);
}


/*
** ReconcileInitiator_SetProgressFeedback()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT ReconcileInitiator_SetProgressFeedback(
                                                PCReconcileInitiator precinit,
                                                ULONG ulProgress,
                                                ULONG ulProgressMax)
{
   RECSTATUSUPDATE rsu;

   /* ulProgress may be any value. */
   /* ulProgressMax may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(precinit, CCReconcileInitiator));

   rsu.ulScale = ulProgressMax;
   rsu.ulProgress = ulProgress;

   if (! NotifyReconciliationStatus(precinit->rsp, RS_DELTA_MERGE,
                                    (LPARAM)&rsu, precinit->lpCallbackData))
      AbortReconciliation(precinit);

   return(S_OK);
}


/*
** ReconcileInitiator_IsMonikerInBriefcase()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT ReconcileInitiator_IsMonikerInBriefcase(
                                          PCReconcileInitiator precinit,
                                          PIMoniker pimk)
{
   HRESULT hr;
   PIMoniker pimkBriefcase;

   ASSERT(IS_VALID_STRUCT_PTR(precinit, CCReconcileInitiator));
   ASSERT(IS_VALID_STRUCT_PTR(pimk, CIMoniker));

   hr = GetBriefcaseRootMoniker(precinit->hbr, &pimkBriefcase);

   if (SUCCEEDED(hr))
   {
      PIMoniker pimkCommonPrefix;

      hr = pimk->lpVtbl->CommonPrefixWith(pimk, pimkBriefcase,
                                          &pimkCommonPrefix);

      if (SUCCEEDED(hr))
      {
         switch (hr)
         {
            case MK_S_US:
               WARNING_OUT(((TEXT("ReconcileInitiator_IsMonikerInBriefcase(): Called on briefcase root."))));
               /* Fall through... */
            case MK_S_HIM:
               hr = S_OK;
               break;

            default:
               ASSERT(hr == S_OK ||
                      hr == MK_S_ME);
               hr = S_FALSE;
               break;
         }

#ifdef DEBUG

         {
            PIBindCtx pibindctx;
            BOOL bGotMoniker = FALSE;
            BOOL bGotBriefcase = FALSE;
            PWSTR pwszMoniker;
            PWSTR pwszBriefcase;
            PIMalloc pimalloc;

            if (SUCCEEDED(CreateBindCtx(0, &pibindctx)))
            {
               bGotMoniker = SUCCEEDED(pimk->lpVtbl->GetDisplayName(
                                                               pimk, pibindctx,
                                                               NULL,
                                                               &pwszMoniker));

               bGotBriefcase = SUCCEEDED(pimkBriefcase->lpVtbl->GetDisplayName(
                                                            pimkBriefcase,
                                                            pibindctx, NULL,
                                                            &pwszBriefcase));

               pibindctx->lpVtbl->Release(pibindctx);
            }

            if (! bGotMoniker)
               pwszMoniker = (PWSTR)L"UNAVAILABLE DISPLAY NAME";

            if (! bGotBriefcase)
               pwszBriefcase = (PWSTR)L"UNAVAILABLE DISPLAY NAME";

            TRACE_OUT(((TEXT("ReconcileInitiator_IsMonikerInBriefcase(): Moniker %ls is %s briefcase %ls.")),
                       pwszMoniker,
                       (hr == S_OK) ? "in" : "not in",
                       pwszBriefcase));

            if (EVAL(GetIMalloc(&pimalloc)))
            {
               if (bGotMoniker)
                  pimalloc->lpVtbl->Free(pimalloc, pwszMoniker);

               if (bGotBriefcase)
                  pimalloc->lpVtbl->Free(pimalloc, pwszBriefcase);

               /* Do not release pimalloc. */
            }
         }

#endif

         /* Do not release pimkBriefcase. */
      }
   }

   return(hr);
}


/*
** AbortReconciliation()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT AbortReconciliation(PCReconcileInitiator precinit)
{
   ASSERT(IS_VALID_STRUCT_PTR(precinit, CCReconcileInitiator));

   if (precinit->piunkForAbort)
      precinit->piunkForAbort->lpVtbl->Release(precinit->piunkForAbort);

   return(S_OK);
}


/*
** RI_IReconcileInitiator_QueryInterface()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT RI_IReconcileInitiator_QueryInterface(
                                                PIReconcileInitiator pirecinit,
                                                REFIID riid, PVOID *ppvObject)
{
   ASSERT(IS_VALID_STRUCT_PTR(pirecinit, CIReconcileInitiator));
   //ASSERT(IsValidREFIID(riid));
   ASSERT(IS_VALID_WRITE_PTR(ppvObject, PVOID));

   return(ReconcileInitiator_QueryInterface(
            ClassFromIface(CReconcileInitiator, irecinit, pirecinit),
            riid, ppvObject));
}


/*
** RI_IReconcileInitiator_AddRef()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE ULONG RI_IReconcileInitiator_AddRef(
                                                PIReconcileInitiator pirecinit)
{
   ASSERT(IS_VALID_STRUCT_PTR(pirecinit, CIReconcileInitiator));

   return(ReconcileInitiator_AddRef(
            ClassFromIface(CReconcileInitiator, irecinit, pirecinit)));
}


/*
** RI_IReconcileInitiator_Release()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE ULONG RI_IReconcileInitiator_Release(
                                                PIReconcileInitiator pirecinit)
{
   ASSERT(IS_VALID_STRUCT_PTR(pirecinit, CIReconcileInitiator));

   return(ReconcileInitiator_Release(
            ClassFromIface(CReconcileInitiator, irecinit, pirecinit)));
}


/*
** RI_IReconcileInitiator_SetAbortCallback()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT RI_IReconcileInitiator_SetAbortCallback(
                                                PIReconcileInitiator pirecinit,
                                                PIUnknown piunkForAbort)
{
   ASSERT(IS_VALID_STRUCT_PTR(pirecinit, CIReconcileInitiator));
   /* piunkForAbort can be legally NULL */
   ASSERT(NULL == piunkForAbort || IS_VALID_STRUCT_PTR(piunkForAbort, CIUnknown));

   return(ReconcileInitiator_SetAbortCallback(
            ClassFromIface(CReconcileInitiator, irecinit, pirecinit),
            piunkForAbort));
}


/*
** RI_IReconcileInitiator_SetProgressFeedback()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT RI_IReconcileInitiator_SetProgressFeedback(
                                                PIReconcileInitiator pirecinit,
                                                ULONG ulProgress,
                                                ULONG ulProgressMax)
{
   /* ulProgress may be any value. */
   /* ulProgressMax may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pirecinit, CIReconcileInitiator));

   return(ReconcileInitiator_SetProgressFeedback(
            ClassFromIface(CReconcileInitiator, irecinit, pirecinit),
            ulProgress, ulProgressMax));
}


/*
** RI_IBriefcaseInitiator_QueryInterface()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT RI_IBriefcaseInitiator_QueryInterface(
                                                PIBriefcaseInitiator pibcinit,
                                                REFIID riid, PVOID *ppvObject)
{
   ASSERT(IS_VALID_STRUCT_PTR(pibcinit, CIBriefcaseInitiator));
   //ASSERT(IsValidREFIID(riid));
   ASSERT(IS_VALID_WRITE_PTR(ppvObject, PVOID));

   return(ReconcileInitiator_QueryInterface(
            ClassFromIface(CReconcileInitiator, ibcinit, pibcinit),
            riid, ppvObject));
}


/*
** RI_IBriefcaseInitiator_AddRef()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE ULONG RI_IBriefcaseInitiator_AddRef(PIBriefcaseInitiator pibcinit)
{
   ASSERT(IS_VALID_STRUCT_PTR(pibcinit, CIBriefcaseInitiator));

   return(ReconcileInitiator_AddRef(
            ClassFromIface(CReconcileInitiator, ibcinit, pibcinit)));
}


/*
** RI_IBriefcaseInitiator_Release()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE ULONG RI_IBriefcaseInitiator_Release(
                                                PIBriefcaseInitiator pibcinit)
{
   ASSERT(IS_VALID_STRUCT_PTR(pibcinit, CIBriefcaseInitiator));

   return(ReconcileInitiator_Release(
            ClassFromIface(CReconcileInitiator, ibcinit, pibcinit)));
}


/*
** RI_IBriefcaseInitiator_IsMonikerInBriefcase()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT RI_IBriefcaseInitiator_IsMonikerInBriefcase(
                                                PIBriefcaseInitiator pibcinit,
                                                PIMoniker pmk)
{
   ASSERT(IS_VALID_STRUCT_PTR(pibcinit, CIBriefcaseInitiator));
   ASSERT(IS_VALID_STRUCT_PTR(pmk, CIMoniker));

   return(ReconcileInitiator_IsMonikerInBriefcase(
            ClassFromIface(CReconcileInitiator, ibcinit, pibcinit),
            pmk));
}


#ifdef DEBUG

/*
** IsValidPCCReconcileInitiator()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCCReconcileInitiator(PCCReconcileInitiator pcrecinit)
{
   /* ulcRef may be any value. */
   /* lpCallbackData may be any value. */

   return(IS_VALID_READ_PTR(pcrecinit, CCReconcileInitiator) &&
          IS_VALID_STRUCT_PTR(&(pcrecinit->irecinit), CIReconcileInitiator) &&
          IS_VALID_STRUCT_PTR(&(pcrecinit->ibcinit), CIBriefcaseInitiator) &&
          IS_VALID_HANDLE(pcrecinit->hbr, BRFCASE) &&
          (! pcrecinit->rsp ||
           IS_VALID_CODE_PTR(pcrecinit->rsp, RECSTATUSPROC)) &&
          (! pcrecinit->piunkForAbort ||
           IS_VALID_STRUCT_PTR(pcrecinit->piunkForAbort, CIUnknown)));
}


/*
** IsValidPCIBriefcaseInitiator()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCIBriefcaseInitiator(PCIBriefcaseInitiator pcibcinit)
{
   return(IS_VALID_READ_PTR(pcibcinit, CIBriefcaseInitiator) &&
          IS_VALID_READ_PTR(pcibcinit->lpVtbl, sizeof(*(pcibcinit->lpVtbl))) &&
          IS_VALID_STRUCT_PTR((PCIUnknown)pcibcinit, CIUnknown) &&
          IS_VALID_CODE_PTR(pcibcinit->lpVtbl->IsMonikerInBriefcase, IsMonikerInBriefcase));
}

#endif


/****************************** Public Functions *****************************/


/*
** IReconcileInitiator_Constructor()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HRESULT IReconcileInitiator_Constructor(
                                             HBRFCASE hbr, RECSTATUSPROC rsp,
                                             LPARAM lpCallbackData,
                                             PIReconcileInitiator *ppirecinit)
{
   HRESULT hr;
   PCReconcileInitiator precinit;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(! rsp ||
          IS_VALID_CODE_PTR(rsp, RECSTATUSPROC));
   ASSERT(IS_VALID_WRITE_PTR(ppirecinit, PIReconcileInitiator));

   if (AllocateMemory(sizeof(*precinit), &precinit))
   {
      precinit->irecinit.lpVtbl = &Mcirecinitvtbl;
      precinit->ibcinit.lpVtbl = &Mcibcinitvtbl;
      precinit->ulcRef = 0;
      precinit->hbr = hbr;
      precinit->rsp = rsp;
      precinit->lpCallbackData = lpCallbackData;
      precinit->piunkForAbort = NULL;

      ASSERT(IS_VALID_STRUCT_PTR(precinit, CCReconcileInitiator));

      hr = precinit->irecinit.lpVtbl->QueryInterface(
               &(precinit->irecinit), &IID_IReconcileInitiator, ppirecinit);

      ASSERT(hr == S_OK);
   }
   else
      hr = E_OUTOFMEMORY;

   ASSERT(FAILED(hr) ||
          IS_VALID_STRUCT_PTR(*ppirecinit, CIReconcileInitiator));

   return(hr);
}

