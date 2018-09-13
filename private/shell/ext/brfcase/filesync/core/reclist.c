/*
 * reclist.c - Reconciliation list ADT module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "stub.h"
#include "oleutil.h"


/* Constants
 ************/

/* RECITEMACTION weights for folders returned by WeighFolderAction(). */

/* (RECITEMACTION weights for files are the RECITEMACTION values.) */

#define RIA_WT_COPY        (-2)
#define RIA_WT_NOTHING     (-1)
#define RIA_WT_DELETE      (+5)


/* Types
 ********/

/* used to count the number of RECNODEs of each RECNODESTATE in a RECITEM */

typedef struct _recnodestatecounter
{
   ULONG ulcUnavailable;
   ULONG ulcDoesNotExist;
   ULONG ulcDeleted;
   ULONG ulcNotReconciled;
   ULONG ulcUpToDate;
   ULONG ulcChanged;
   ULONG ulcNeverReconciled;
}
RECNODESTATECOUNTER;
DECLARE_STANDARD_TYPES(RECNODESTATECOUNTER);

/* DoesTwinFamilyNeedRec() callback structure */

typedef struct _twinfamilyrecinfo
{
   TWINRESULT tr;

   BOOL bNeedsRec;
}
TWINFAMILYRECINFO;
DECLARE_STANDARD_TYPES(TWINFAMILYRECINFO);


/***************************** Private Functions *****************************/


/* Module Prototypes
 ********************/

PRIVATE_CODE RECNODESTATE DetermineRecNodeState(PCRECNODE);
PRIVATE_CODE void AddRecNodeState(RECNODESTATE, PRECNODESTATECOUNTER);
PRIVATE_CODE void CountRecNodeStates(PCRECITEM, PRECNODESTATECOUNTER, PULONG);
PRIVATE_CODE void DetermineRecActions(PRECITEM);
PRIVATE_CODE void BreakMergeIfNecessary(PRECITEM);
PRIVATE_CODE TWINRESULT AddRecItemsToRecList(HTWINLIST, CREATERECLISTPROC, LPARAM, PRECLIST);
PRIVATE_CODE void LinkUpRecList(PRECLIST, HPTRARRAY);
PRIVATE_CODE int WeighFileAction(RECITEMACTION);
PRIVATE_CODE int WeighFolderAction(RECITEMACTION);
PRIVATE_CODE COMPARISONRESULT RecItemSortCmp(PCVOID, PCVOID);
PRIVATE_CODE void DestroyArrayOfRecItems(HPTRARRAY);
PRIVATE_CODE BOOL MarkTwinFamilyUsed(POBJECTTWIN, PVOID);
PRIVATE_CODE ULONG MarkIntersectingTwinFamiliesUsed(HTWIN);
PRIVATE_CODE void DestroyRecItem(PRECITEM);
PRIVATE_CODE void DestroyRecNode(PRECNODE);
PRIVATE_CODE void DestroyListOfRecItems(PRECITEM);
PRIVATE_CODE void DestroyListOfRecNodes(PRECNODE);
PRIVATE_CODE void MyDestroyRecList(PRECLIST);
PRIVATE_CODE BOOL DeleteDeletedObjectTwins(PCRECITEM, PBOOL);
PRIVATE_CODE BOOL FindAGeneratedObjectTwinProc(POBJECTTWIN, PVOID);
PRIVATE_CODE BOOL FolderTwinShouldBeImplicitlyDeleted(PFOLDERPAIR);
PRIVATE_CODE BOOL DeleteDeletedFolderTwins(HPTRARRAY);
PRIVATE_CODE TWINRESULT CreateRecItem(PTWINFAMILY, PRECITEM *);
PRIVATE_CODE TWINRESULT AddObjectTwinRecNode(PRECITEM, POBJECTTWIN);
PRIVATE_CODE BOOL DoesTwinFamilyNeedRec(POBJECTTWIN, PVOID);
PRIVATE_CODE TWINRESULT GetFolderPairStatus(PFOLDERPAIR, CREATERECLISTPROC, LPARAM, PFOLDERTWINSTATUS);

#ifdef DEBUG

PRIVATE_CODE BOOL IsValidCreateRecListProcMsg(UINT);
PRIVATE_CODE BOOL IsValidFOLDERTWINSTATUS(FOLDERTWINSTATUS);
PRIVATE_CODE BOOL IsValidPCRECNODESTATECOUNTER(PCRECNODESTATECOUNTER);

#endif

#if defined(DEBUG) || defined(VSTF)

PRIVATE_CODE BOOL IsValidRECNODESTATE(RECNODESTATE);
PRIVATE_CODE BOOL IsValidRECNODEACTION(RECNODEACTION);
PRIVATE_CODE BOOL IsValidRECITEMACTION(RECITEMACTION);
PRIVATE_CODE BOOL IsValidPCRECLIST(PCRECLIST);

#endif


/*
** DetermineRecNodeState()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE RECNODESTATE DetermineRecNodeState(PCRECNODE pcrn)
{
   RECNODESTATE rnstate;

   ASSERT(IS_VALID_WRITE_PTR(pcrn, RECNODE));

   if (pcrn->fsCurrent.fscond != FS_COND_UNAVAILABLE)
   {
      if (IsReconciledFileStamp(&(pcrn->fsLast)))
      {
         if (pcrn->fsCurrent.fscond == FS_COND_EXISTS)
         {
            BOOL bReconciledLastTime;

            bReconciledLastTime = (IsStubFlagClear(&(((PCOBJECTTWIN)(pcrn->hObjectTwin))->stub),
                                                   STUB_FL_NOT_RECONCILED));

            if (MyCompareFileStamps(&(pcrn->fsLast), &(pcrn->fsCurrent))
                == CR_EQUAL)
            {
               if (bReconciledLastTime)
                  rnstate = RNS_UP_TO_DATE;
               else
                  rnstate = RNS_NOT_RECONCILED;
            }
            else
            {
               if (bReconciledLastTime)
                  rnstate = RNS_CHANGED;
               else
                  /* Divergent version. */
                  rnstate = RNS_NEVER_RECONCILED;
            }
         }
         else
         {
            ASSERT(pcrn->fsCurrent.fscond == FS_COND_DOES_NOT_EXIST);

            rnstate = RNS_DELETED;
         }
      }
      else
      {
         if (pcrn->fsCurrent.fscond == FS_COND_EXISTS)
            rnstate = RNS_NEVER_RECONCILED;
         else
         {
            ASSERT(pcrn->fsCurrent.fscond == FS_COND_DOES_NOT_EXIST);
            rnstate = RNS_DOES_NOT_EXIST;
         }
      }
   }
   else
   {
      /* Deleted wins over unavailable. */

      if (pcrn->fsLast.fscond == FS_COND_DOES_NOT_EXIST)
         rnstate = RNS_DELETED;
      else
         rnstate = RNS_UNAVAILABLE;
   }

   /* Collapse folder RECNODE states. */

   if (IsFolderObjectTwinName(pcrn->priParent->pcszName))
   {
      switch (rnstate)
      {
         case RNS_NEVER_RECONCILED:
         case RNS_NOT_RECONCILED:
         case RNS_CHANGED:
            rnstate = RNS_UP_TO_DATE;
            break;
      }
   }

   ASSERT(IsValidRECNODESTATE(rnstate));

   return(rnstate);
}


/*
** AddRecNodeState()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void AddRecNodeState(RECNODESTATE rnstate,
                                  PRECNODESTATECOUNTER prnscntr)
{
   ASSERT(IsValidRECNODESTATE(rnstate));
   ASSERT(IS_VALID_STRUCT_PTR(prnscntr, CRECNODESTATECOUNTER));

   switch (rnstate)
   {
      case RNS_UNAVAILABLE:
         ASSERT(prnscntr->ulcUnavailable < ULONG_MAX);
         prnscntr->ulcUnavailable++;
         break;

      case RNS_DOES_NOT_EXIST:
         ASSERT(prnscntr->ulcDoesNotExist < ULONG_MAX);
         prnscntr->ulcDoesNotExist++;
         break;

      case RNS_DELETED:
         ASSERT(prnscntr->ulcDeleted < ULONG_MAX);
         prnscntr->ulcDeleted++;
         break;

      case RNS_NOT_RECONCILED:
         ASSERT(prnscntr->ulcNotReconciled < ULONG_MAX);
         prnscntr->ulcNotReconciled++;
         break;

      case RNS_UP_TO_DATE:
         ASSERT(prnscntr->ulcUpToDate < ULONG_MAX);
         prnscntr->ulcUpToDate++;
         break;

      case RNS_CHANGED:
         ASSERT(prnscntr->ulcChanged < ULONG_MAX);
         prnscntr->ulcChanged++;
         break;

      default:
         ASSERT(rnstate == RNS_NEVER_RECONCILED);
         ASSERT(prnscntr->ulcNeverReconciled < ULONG_MAX);
         prnscntr->ulcNeverReconciled++;
         break;
   }

   return;
}


/*
** CountRecNodeStates()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void CountRecNodeStates(PCRECITEM pcri,
                                     PRECNODESTATECOUNTER prnscntr,
                                     PULONG pulcToDelete)
{
   PCRECNODE pcrn;

   ASSERT(IS_VALID_READ_PTR(pcri, CRECITEM));
   ASSERT(IS_VALID_STRUCT_PTR(prnscntr, CRECNODESTATECOUNTER));
   ASSERT(IS_VALID_WRITE_PTR(pulcToDelete, ULONG));

   ZeroMemory(prnscntr, sizeof(*prnscntr));
   *pulcToDelete = 0;

   for (pcrn = pcri->prnFirst; pcrn; pcrn = pcrn->prnNext)
   {
      AddRecNodeState(pcrn->rnstate, prnscntr);

      if (pcrn->rnstate == RNS_UP_TO_DATE &&
          IsStubFlagClear(&(((PCOBJECTTWIN)(pcrn->hObjectTwin))->stub),
                          STUB_FL_KEEP))
      {
         ASSERT(*pulcToDelete < ULONG_MAX);
         (*pulcToDelete)++;
      }
   }

   ASSERT(IS_VALID_STRUCT_PTR(prnscntr, CRECNODESTATECOUNTER));
   ASSERT(prnscntr->ulcUnavailable +
          prnscntr->ulcDoesNotExist +
          prnscntr->ulcDeleted +
          prnscntr->ulcNotReconciled +
          prnscntr->ulcUpToDate +
          prnscntr->ulcChanged +
          prnscntr->ulcNeverReconciled == pcri->ulcNodes);
   ASSERT(*pulcToDelete <= prnscntr->ulcUpToDate);

   return;
}


/*
** DetermineRecActions()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE void DetermineRecActions(PRECITEM pri)
{
   RECNODESTATECOUNTER rnscntr;
   ULONG ulcToDelete;
   RECITEMACTION riaSummary = RIA_NOTHING;
   RECNODEACTION rnaDoesNotExist = RNA_NOTHING;
   RECNODEACTION rnaNotReconciled = RNA_NOTHING;
   RECNODEACTION rnaUpToDateSrc = RNA_NOTHING;
   RECNODEACTION rnaUpToDate = RNA_NOTHING;
   RECNODEACTION rnaChanged = RNA_NOTHING;
   RECNODEACTION rnaNeverReconciled = RNA_NOTHING;
   BOOL bNeedUpToDateCopySrc = FALSE;
   BOOL bNeedNotReconciledCopySrc = FALSE;
   PRECNODE prn;

   ASSERT(IS_VALID_WRITE_PTR(pri, RECITEM));

   ZeroMemory(&rnscntr, sizeof(rnscntr));
   CountRecNodeStates(pri, &rnscntr, &ulcToDelete);

   if (rnscntr.ulcNeverReconciled > 0)
   {
      if (rnscntr.ulcChanged > 0)
      {
         riaSummary = RIA_MERGE;

         rnaNeverReconciled = RNA_MERGE_ME;
         rnaChanged = RNA_MERGE_ME;

         rnaUpToDate = RNA_COPY_TO_ME;
         rnaNotReconciled = RNA_COPY_TO_ME;
         rnaDoesNotExist = RNA_COPY_TO_ME;
      }
      else if (rnscntr.ulcUpToDate > 0)
      {
         riaSummary = RIA_MERGE;

         rnaNeverReconciled = RNA_MERGE_ME;
         rnaUpToDate = RNA_MERGE_ME;

         rnaNotReconciled = RNA_COPY_TO_ME;
         rnaDoesNotExist = RNA_COPY_TO_ME;
      }
      else if (rnscntr.ulcNotReconciled > 0)
      {
         riaSummary = RIA_MERGE;

         rnaNeverReconciled = RNA_MERGE_ME;
         rnaNotReconciled = RNA_MERGE_ME;

         rnaDoesNotExist = RNA_COPY_TO_ME;
      }
      else if (rnscntr.ulcNeverReconciled >= 2)
      {
         riaSummary = RIA_MERGE;

         rnaNeverReconciled = RNA_MERGE_ME;

         rnaDoesNotExist = RNA_COPY_TO_ME;
      }
      else if (rnscntr.ulcDoesNotExist > 0)
      {
         ASSERT(rnscntr.ulcNeverReconciled == 1);

         riaSummary = RIA_COPY;

         rnaNeverReconciled = RNA_COPY_FROM_ME;

         rnaDoesNotExist = RNA_COPY_TO_ME;
      }
   }
   else if (rnscntr.ulcChanged >= 2)
   {
      riaSummary = RIA_MERGE;

      rnaChanged = RNA_MERGE_ME;

      rnaUpToDate = RNA_COPY_TO_ME;
      rnaNotReconciled = RNA_COPY_TO_ME;
      rnaDoesNotExist = RNA_COPY_TO_ME;
   }
   else if (rnscntr.ulcChanged == 1)
   {
      if (rnscntr.ulcUpToDate > 0 ||
          rnscntr.ulcNotReconciled > 0 ||
          rnscntr.ulcDoesNotExist > 0)
      {
         riaSummary = RIA_COPY;

         rnaChanged = RNA_COPY_FROM_ME;

         rnaUpToDate = RNA_COPY_TO_ME;
         rnaNotReconciled = RNA_COPY_TO_ME;
         rnaDoesNotExist = RNA_COPY_TO_ME;
      }
   }
   else if (IsTwinFamilyDeletionPending((PCTWINFAMILY)(pri->hTwinFamily)))
   {
      if (ulcToDelete > 0)
      {
         riaSummary = RIA_DELETE;

         rnaNotReconciled = RNA_DELETE_ME;
         rnaUpToDate = RNA_DELETE_ME;
      }
   }
   else if (rnscntr.ulcUpToDate > 0)
   {
      if (rnscntr.ulcNotReconciled > 0 ||
          rnscntr.ulcDoesNotExist > 0)
      {
         riaSummary = RIA_COPY;

         bNeedUpToDateCopySrc = TRUE;

         rnaNotReconciled = RNA_COPY_TO_ME;
         rnaDoesNotExist = RNA_COPY_TO_ME;
      }
   }
   else if (rnscntr.ulcNotReconciled > 0)
   {
      if (rnscntr.ulcDoesNotExist > 0)
      {
         riaSummary = RIA_COPY;

         bNeedNotReconciledCopySrc = TRUE;

         rnaDoesNotExist = RNA_COPY_TO_ME;
      }
   }

   /* Apply determined actions. */

   ASSERT(! (bNeedUpToDateCopySrc && bNeedNotReconciledCopySrc));

   for (prn = pri->prnFirst; prn; prn = prn->prnNext)
   {
      switch (prn->rnstate)
      {
         case RNS_NEVER_RECONCILED:
            prn->rnaction = rnaNeverReconciled;
            break;

         case RNS_DOES_NOT_EXIST:
            prn->rnaction = rnaDoesNotExist;
            break;

         case RNS_NOT_RECONCILED:
            if (bNeedNotReconciledCopySrc)
            {
               prn->rnaction = RNA_COPY_FROM_ME;
               bNeedNotReconciledCopySrc = FALSE;
            }
            else
               prn->rnaction = rnaNotReconciled;
            break;

         case RNS_UP_TO_DATE:
            if (bNeedUpToDateCopySrc)
            {
               prn->rnaction = RNA_COPY_FROM_ME;
               bNeedUpToDateCopySrc = FALSE;
            }
            else
               prn->rnaction = rnaUpToDate;
            break;

         case RNS_CHANGED:
            prn->rnaction = rnaChanged;
            break;

         default:
            ASSERT(prn->rnstate == RNS_UNAVAILABLE ||
                   prn->rnstate == RNS_DELETED);
            prn->rnaction = RNA_NOTHING;
            break;
      }

      if (prn->rnaction == RNA_DELETE_ME)
      {
         if (IsStubFlagClear(&(((PCOBJECTTWIN)(prn->hObjectTwin))->stub),
             STUB_FL_KEEP))
            SET_FLAG(prn->dwFlags, RN_FL_DELETION_SUGGESTED);
         else
            prn->rnaction = RNA_NOTHING;
      }
   }

   pri->riaction = riaSummary;

   /* Break a merge if no reconciliation handler is registered. */

   if (pri->riaction == RIA_MERGE)
      BreakMergeIfNecessary(pri);

   ASSERT(IS_VALID_STRUCT_PTR(pri, CRECITEM));

   return;
}


/*
** BreakMergeIfNecessary()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void BreakMergeIfNecessary(PRECITEM pri)
{
   PRECNODE prnMergeDest;
   TCHAR rgchPath[MAX_PATH_LEN];
   CLSID clsidReconciler;

   ASSERT(IS_VALID_STRUCT_PTR(pri, CRECITEM));

   ASSERT(pri->riaction == RIA_MERGE);

   /* Is a class reconciler registered for this RECITEM? */

   ChooseMergeDestination(pri, &prnMergeDest);
   ASSERT(prnMergeDest->priParent == pri);

   ComposePath(rgchPath, prnMergeDest->pcszFolder, prnMergeDest->priParent->pcszName);
   ASSERT(lstrlen(rgchPath) < ARRAYSIZE(rgchPath));

   if (FAILED(GetReconcilerClassID(rgchPath, &clsidReconciler)))
   {
      pri->riaction = RIA_BROKEN_MERGE;

      TRACE_OUT((TEXT("MassageMergeCase(): Breaking merge RECITEM for %s.  No registered reconciliation handler."),
                 pri->pcszName));
   }

   return;
}


/*
** AddRecItemsToRecList()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT AddRecItemsToRecList(HTWINLIST htl,
                                             CREATERECLISTPROC crlp,
                                             LPARAM lpCallbackData,
                                             PRECLIST prl)
{
   TWINRESULT tr = TR_SUCCESS;
   HBRFCASE hbr;
   HPTRARRAY hpaTwinFamilies;
   ARRAYINDEX aicTwins;
   ARRAYINDEX ai;
   ULONG ulcMarkedTwinFamilies = 0;
   NEWPTRARRAY npa;
   HPTRARRAY hpaRecItems;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_HANDLE(htl, TWINLIST));
   ASSERT(! crlp ||
          IS_VALID_CODE_PTR(crlp, CREATERECLISTPROC));
   ASSERT(IS_VALID_STRUCT_PTR(prl, CRECLIST));

   /*
    * "Used" twin families are twin families that are to be added to the
    * reconciliation list as RECITEMs.
    */

   /* Mark all twin families unused. */

   hbr = GetTwinListBriefcase(htl);

   hpaTwinFamilies = GetBriefcaseTwinFamilyPtrArray(hbr);

   ClearFlagInArrayOfStubs(hpaTwinFamilies, STUB_FL_USED);

   /* Mark twin families that intersect twins in twin list as used. */

   aicTwins = GetTwinListCount(htl);

   for (ai = 0; ai < aicTwins; ai++)
   {
      ULONG ulcNewlyMarked;

      ulcNewlyMarked = MarkIntersectingTwinFamiliesUsed(GetTwinFromTwinList(htl, ai));

      ASSERT(ulcMarkedTwinFamilies <= ULONG_MAX - ulcNewlyMarked);
      ulcMarkedTwinFamilies += ulcNewlyMarked;
   }

   /* Create a PTRARRAY to keep track of the RECITEMs created. */

   npa.aicInitialPtrs = ulcMarkedTwinFamilies;
   npa.aicAllocGranularity = 1;
   npa.dwFlags = 0;

   if (CreatePtrArray(&npa, &hpaRecItems))
   {
      ARRAYINDEX aicPtrs;

      /* Add the marked twin families to the RECLIST as RECITEMS. */

      aicPtrs = GetPtrCount(hpaTwinFamilies);

      ai = 0;

      while (ai < aicPtrs && ulcMarkedTwinFamilies > 0)
      {
         PTWINFAMILY ptf;

         ptf = GetPtr(hpaTwinFamilies, ai);

         ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));

         if (IsStubFlagSet(&(ptf->stub), STUB_FL_USED))
         {
            PRECITEM priNew;

            ulcMarkedTwinFamilies--;

            tr = CreateRecItem(ptf, &priNew);

            if (tr == TR_SUCCESS)
            {
               ARRAYINDEX ai;

               if (AddPtr(hpaRecItems, NULL, priNew, &ai))
               {
                  if (! NotifyCreateRecListStatus(crlp, CRLS_DELTA_CREATE_REC_LIST,
                                                  0, lpCallbackData))
                     tr = TR_ABORT;
               }
               else
                  tr = TR_OUT_OF_MEMORY;
            }

            if (tr != TR_SUCCESS)
               break;
         }

         ai++;
      }

      if (tr == TR_SUCCESS)
      {
         ASSERT(! ulcMarkedTwinFamilies);
         LinkUpRecList(prl, hpaRecItems);
      }
      else
         DestroyArrayOfRecItems(hpaRecItems);

      DestroyPtrArray(hpaRecItems);
   }
   else
      tr = TR_OUT_OF_MEMORY;

   return(tr);
}


/*
** WeighFileAction()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE int WeighFileAction(RECITEMACTION riaction)
{
   ASSERT(IsValidRECITEMACTION(riaction));

   return(riaction);
}


/*
** WeighFolderAction()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE int WeighFolderAction(RECITEMACTION riaction)
{
   int nWeight;

   ASSERT(IsValidRECITEMACTION(riaction));

   switch (riaction)
   {
      case RIA_COPY:
         nWeight = RIA_WT_COPY;
         break;

      case RIA_NOTHING:
         nWeight = RIA_WT_NOTHING;
         break;

      default:
         ASSERT(riaction == RIA_DELETE);
         nWeight = RIA_WT_DELETE;
         break;
   }

   return(nWeight);
}


/*
** RecItemSortCmp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** RECITEMs are sorted in the following order:
**    1) create folder
**    2) do nothing to folder
**    3) do nothing to file
**    4) delete file
**    5) copy file
**    6) merge file
**    7) broken merge file
**    8) delete folder
** and then by:
**    1) name
*/
PRIVATE_CODE COMPARISONRESULT RecItemSortCmp(PCVOID pcriFirst,
                                             PCVOID pcriSecond)
{
   COMPARISONRESULT cr;
   BOOL bFirstFile;
   BOOL bSecondFile;
   int nFirstWeight;
   int nSecondWeight;

   ASSERT(IS_VALID_STRUCT_PTR(pcriFirst, CRECITEM));
   ASSERT(IS_VALID_STRUCT_PTR(pcriSecond, CRECITEM));

   bFirstFile = (*(((PCRECITEM)pcriFirst)->pcszName) != TEXT('\0'));
   bSecondFile = (*(((PCRECITEM)pcriSecond)->pcszName) != TEXT('\0'));

   if (bFirstFile)
      nFirstWeight = WeighFileAction(((PCRECITEM)pcriFirst)->riaction);
   else
      nFirstWeight = WeighFolderAction(((PCRECITEM)pcriFirst)->riaction);

   if (bSecondFile)
      nSecondWeight = WeighFileAction(((PCRECITEM)pcriSecond)->riaction);
   else
      nSecondWeight = WeighFolderAction(((PCRECITEM)pcriSecond)->riaction);

   cr = CompareInts(nFirstWeight, nSecondWeight);

   if (cr == CR_EQUAL)
      cr = CompareNameStrings(((PCRECITEM)pcriFirst)->pcszName,
                              ((PCRECITEM)pcriSecond)->pcszName);

   return(cr);
}


/*
** LinkUpRecList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void LinkUpRecList(PRECLIST prl, HPTRARRAY hpaRecItems)
{
   ARRAYINDEX ai;
   ARRAYINDEX aicPtrs;

   ASSERT(IS_VALID_STRUCT_PTR(prl, CRECLIST));
   ASSERT(IS_VALID_HANDLE(hpaRecItems, PTRARRAY));

   SortPtrArray(hpaRecItems, &RecItemSortCmp);

   aicPtrs = GetPtrCount(hpaRecItems);

   for (ai = aicPtrs; ai > 0; ai--)
   {
      PRECITEM pri;

      pri = GetPtr(hpaRecItems, ai - 1);

      pri->priNext = prl->priFirst;
      prl->priFirst = pri;
   }

   prl->ulcItems = aicPtrs;

   ASSERT(IS_VALID_STRUCT_PTR(prl, CRECLIST));

   return;
}


/*
** DestroyArrayOfRecItems()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroyArrayOfRecItems(HPTRARRAY hpaRecItems)
{
   ARRAYINDEX ai;
   ARRAYINDEX aicPtrs;

   ASSERT(IS_VALID_HANDLE(hpaRecItems, PTRARRAY));

   aicPtrs = GetPtrCount(hpaRecItems);

   for (ai = 0; ai < aicPtrs; ai++)
      DestroyRecItem(GetPtr(hpaRecItems, ai));

   return;
}


/*
** MarkTwinFamilyUsed()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/

#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

PRIVATE_CODE BOOL MarkTwinFamilyUsed(POBJECTTWIN pot, PVOID pulcNewlyMarked)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(IS_VALID_WRITE_PTR(pulcNewlyMarked, ULONG));

   if (IsStubFlagClear(&(pot->ptfParent->stub), STUB_FL_USED))
   {
      /* Mark the twin family used. */

      SetStubFlag(&(pot->ptfParent->stub), STUB_FL_USED);

      ASSERT(*(PULONG)pulcNewlyMarked < ULONG_MAX);
      (*(PULONG)pulcNewlyMarked)++;
   }

   return(TRUE);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


/*
** MarkIntersectingTwinFamiliesUsed()
**
** Marks the twin families that intersect a twin as used.
**
** Arguments:     htwin - handle to intersecting twin
**
** Returns:       Number of twin families newly marked used.
**
** Side Effects:  none
*/
PRIVATE_CODE ULONG MarkIntersectingTwinFamiliesUsed(HTWIN htwin)
{
   ULONG ulcNewlyMarked = 0;

   ASSERT(IS_VALID_HANDLE(htwin, TWIN));

   /* Skip deleted twins. */

   if (IsStubFlagClear((PCSTUB)htwin, STUB_FL_UNLINKED))
   {
      /* Determine intersecting twin families based upon type of twin. */

      switch (((PSTUB)htwin)->st)
      {
         case ST_OBJECTTWIN:
            if (IsStubFlagClear(&(((POBJECTTWIN)htwin)->ptfParent->stub),
                                STUB_FL_USED))
            {
               /* Mark the twin family of the object twin as used. */

               SetStubFlag(&(((POBJECTTWIN)htwin)->ptfParent->stub),
                           STUB_FL_USED);

               ulcNewlyMarked++;
            }
            break;

         case ST_TWINFAMILY:
            if (IsStubFlagClear(&(((PTWINFAMILY)htwin)->stub), STUB_FL_USED))
            {
               /* Mark the twin family used. */

               SetStubFlag(&(((PTWINFAMILY)htwin)->stub), STUB_FL_USED);

               ulcNewlyMarked++;
            }
            break;

         default:
            /*
             * Mark the twin families of the generated object twins from one of
             * the folder twins as used.  (Only one of the two lists of object
             * twins needs to be added since the other list should contain
             * object twins in exactly the same twin families as the first.)
             */
            ASSERT(((PSTUB)htwin)->st == ST_FOLDERPAIR);
            EVAL(EnumGeneratedObjectTwins((PCFOLDERPAIR)htwin,
                                          &MarkTwinFamilyUsed,
                                          &ulcNewlyMarked));
            break;
      }
   }

   return(ulcNewlyMarked);
}


/*
** DestroyRecItem()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroyRecItem(PRECITEM pri)
{
   ASSERT(IS_VALID_STRUCT_PTR(pri, CRECITEM));

   /* Destroy the RECITEM's list of RECNODES. */

   DestroyListOfRecNodes(pri->prnFirst);

   /* Now unlock the twin family stub associated with the RECITEM. */

   UnlockStub(&(((PTWINFAMILY)(pri->hTwinFamily))->stub));

   FreeMemory(pri);

   return;
}


/*
** DestroyRecNode()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroyRecNode(PRECNODE prn)
{
   ASSERT(IS_VALID_STRUCT_PTR(prn, CRECNODE));

   /* Unlock the object twin stub associated with the RECNODE. */

   UnlockStub(&(((POBJECTTWIN)(prn->hObjectTwin))->stub));

   FreeMemory((LPTSTR)(prn->pcszFolder));
   FreeMemory(prn);

   return;
}


/*
** DestroyListOfRecItems()
**
** Destroys a list of reconciliation items.
**
** Arguments:     priHead - pointer to first reconciliation item in list
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroyListOfRecItems(PRECITEM priHead)
{
   while (priHead)
   {
      PRECITEM priPrev;

      ASSERT(IS_VALID_STRUCT_PTR(priHead, CRECITEM));

      priPrev = priHead;
      priHead = priHead->priNext;

      DestroyRecItem(priPrev);
   }

   return;
}


/*
** DestroyListOfRecNodes()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroyListOfRecNodes(PRECNODE prnHead)
{
   while (prnHead)
   {
      PRECNODE prnPrev;

      ASSERT(IS_VALID_STRUCT_PTR(prnHead, CRECNODE));

      prnPrev = prnHead;
      prnHead = prnHead->prnNext;

      DestroyRecNode(prnPrev);
   }

   return;
}


/*
** MyDestroyRecList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void MyDestroyRecList(PRECLIST prl)
{
   ASSERT(IS_VALID_STRUCT_PTR(prl, CRECLIST));

   DestroyListOfRecItems(prl->priFirst);
   FreeMemory(prl);

   return;
}


/*
** DeleteDeletedObjectTwins()
**
** Performs garbage collection of obsolete object twins.
**
** Arguments:
**
** Returns:
**
** Side Effects:  May implicitly delete twin family.  Marks generating folder
**                twins used.
**
** Deletes the following:
**    1) Any reconciled object twin last known non-existent whose twin family
**       is not in the deletion pending state.  This may cause the parent twin
**       family to be implicitly deleted as a result.
**    2) Any twin family all of whose object twins are last known non-existent.
*/
PRIVATE_CODE BOOL DeleteDeletedObjectTwins(PCRECITEM pcri,
                                           PBOOL pbAnyFolderTwinsMarked)
{
   BOOL bAnyDeleted = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));
   ASSERT(IS_VALID_WRITE_PTR(pbAnyFolderTwinsMarked, BOOL));

   *pbAnyFolderTwinsMarked = FALSE;

   if (! IsTwinFamilyDeletionPending((PCTWINFAMILY)(pcri->hTwinFamily)))
   {
      ULONG ulcNonExistent = 0;
      PRECNODE prn;
      PTWINFAMILY ptf;

      for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
      {
         if (LastKnownNonExistent(&(prn->fsLast), &(prn->fsCurrent)))
         {
            ASSERT(ulcNonExistent < ULONG_MAX);
            ulcNonExistent++;

            if (IsReconciledFileStamp(&(prn->fsLast)))
            {
               POBJECTTWIN pot;

               pot = (POBJECTTWIN)(prn->hObjectTwin);

               if (! pot->ulcSrcFolderTwins)
               {
                  DestroyStub(&(pot->stub));

                  TRACE_OUT((TEXT("DeleteDeletedObjectTwins(): Implicitly removed object twin for deleted file %s\\%s."),
                             prn->pcszFolder,
                             prn->priParent->pcszName));
               }
               else
               {
                  ULONG ulcFolderTwins;

                  ClearStubFlag(&(pot->stub), STUB_FL_FROM_OBJECT_TWIN);

                  EVAL(EnumGeneratingFolderTwins(
                           pot,
                           (ENUMGENERATINGFOLDERTWINSPROC)&SetStubFlagWrapper,
                           (PVOID)STUB_FL_USED, &ulcFolderTwins));

                  *pbAnyFolderTwinsMarked = (ulcFolderTwins > 0);
               }

               bAnyDeleted = TRUE;
            }
         }
      }

      ptf = (PTWINFAMILY)(pcri->hTwinFamily);

      if (ulcNonExistent == pcri->ulcNodes &&
          IsStubFlagClear(&(ptf->stub), STUB_FL_UNLINKED))
      {
         ClearTwinFamilySrcFolderTwinCount(ptf);

         EVAL(DestroyStub(&(ptf->stub)) == TR_SUCCESS);

         TRACE_OUT((TEXT("DeleteDeletedObjectTwins(): Implicitly removed twin family for %s since all members are last known non-existent."),
                    pcri->pcszName));
      }
   }

   return(bAnyDeleted);
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

/*
** FindAGeneratedObjectTwinProc()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** An obsolete generated object twin is an object twin that is last known
** non-existent, and whose twin family is not in the deletion pending state.
*/
PRIVATE_CODE BOOL FindAGeneratedObjectTwinProc(POBJECTTWIN pot, PVOID pvUnused)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(! pvUnused);

   return(pot->fsLastRec.fscond == FS_COND_DOES_NOT_EXIST &&
          ! IsTwinFamilyDeletionPending(pot->ptfParent));
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


/*
** FolderTwinShouldBeImplicitlyDeleted()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** A folder twin should be implicitly deleted when it meets all the following
** conditions:
**    1) The folder root is last known non-existent.
**    2) One or more of its generated object twins has just been implicitly
**       deleted.
**    3) It no longer generates any non-obsolete object twins.
*/
PRIVATE_CODE BOOL FolderTwinShouldBeImplicitlyDeleted(PFOLDERPAIR pfp)
{
   BOOL bDelete;

   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   if (IsStubFlagSet(&(pfp->stub), STUB_FL_DELETION_PENDING) &&
       IsStubFlagSet(&(pfp->stub), STUB_FL_USED) &&
       EnumGeneratedObjectTwins(pfp, &FindAGeneratedObjectTwinProc, NULL))
   {
      TRACE_OUT((TEXT("FolderTwinShouldBeImplicitlyDeleted(): Folder twin %s should be implicitly deleted."),
                 DebugGetPathString(pfp->hpath)));

      bDelete = TRUE;
   }
   else
      bDelete = FALSE;

   return(bDelete);
}


/*
** DeleteDeletedFolderTwins()
**
** Performs garbage collection of obsolete folder twins.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL DeleteDeletedFolderTwins(HPTRARRAY hpaFolderPairs)
{
   BOOL bAnyDeleted = FALSE;
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hpaFolderPairs, PTRARRAY));

   aicPtrs = GetPtrCount(hpaFolderPairs);
   ASSERT(! (aicPtrs % 2));

   ai = 0;

   while (ai < aicPtrs)
   {
      PFOLDERPAIR pfp;

      pfp = GetPtr(hpaFolderPairs, ai);

      if (FolderTwinShouldBeImplicitlyDeleted(pfp) ||
          FolderTwinShouldBeImplicitlyDeleted(pfp->pfpOther))
      {
         TRACE_OUT((TEXT("DeleteDeletedFolderTwins(): Implicitly deleting %s twin pair %s and %s, files %s."),
                    IsStubFlagSet(&(pfp->stub), STUB_FL_SUBTREE) ? TEXT("subtree") : TEXT("folder"),
                    DebugGetPathString(pfp->hpath),
                    DebugGetPathString(pfp->pfpOther->hpath),
                    GetString(pfp->pfpd->hsName)));

         DestroyStub(&(pfp->stub));

         aicPtrs -= 2;
         ASSERT(! (aicPtrs % 2));
         ASSERT(aicPtrs == GetPtrCount(hpaFolderPairs));

         bAnyDeleted = TRUE;
      }
      else
      {
         /* Don't check this pair of folder twins again. */

         ClearStubFlag(&(pfp->stub), STUB_FL_USED);
         ClearStubFlag(&(pfp->pfpOther->stub), STUB_FL_USED);

         ai++;
      }
   }

   return(bAnyDeleted);
}


/*
** CreateRecItem()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT CreateRecItem(PTWINFAMILY ptf, PRECITEM *ppri)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));
   ASSERT(IS_VALID_WRITE_PTR(ppri, PRECITEM));

   /* Create a new RECITEM for the twin family. */

   if (AllocateMemory(sizeof(**ppri), ppri))
   {
      LPCTSTR pcszName;
      BOOL bContinue;
      HNODE hnode;

      /* Get twin family's object name. */

      tr = TR_SUCCESS;

      pcszName = GetString(ptf->hsName);

      /* Fill in the fields required for adding new RECNODEs. */

      /* N.b., SYNCUI depends on dwUser to be initialized to 0. */

      (*ppri)->pcszName = pcszName;
      (*ppri)->ulcNodes = 0;
      (*ppri)->prnFirst = NULL;
      (*ppri)->hTwinFamily = (HTWINFAMILY)ptf;
      (*ppri)->dwUser = 0;

      TRACE_OUT((TEXT("CreateRecItem(): Creating a RECITEM for %s."),
                 pcszName));

      /* Add object twins to the RECITEM one at a time as RECNODEs. */

      for (bContinue = GetFirstNode(ptf->hlistObjectTwins, &hnode);
           bContinue;
           bContinue = GetNextNode(hnode, &hnode))
      {
         POBJECTTWIN pot;

         pot = (POBJECTTWIN)GetNodeData(hnode);

         tr = AddObjectTwinRecNode(*ppri, pot);

         if (tr != TR_SUCCESS)
         {
            DestroyRecItem(*ppri);
            break;
         }
      }

      if (tr == TR_SUCCESS)
      {
         DetermineDeletionPendingState(*ppri);

         DetermineRecActions(*ppri);

         LockStub(&(ptf->stub));

#ifdef DEBUG

         {
            LPCTSTR pcszAction;

            switch ((*ppri)->riaction)
            {
               case RIA_COPY:
                  pcszAction = TEXT("Copy");
                  break;

               case RIA_MERGE:
                  pcszAction = TEXT("Merge");
                  break;

               case RIA_BROKEN_MERGE:
                  pcszAction = TEXT("Broken merge for");
                  break;

               case RIA_DELETE:
                  pcszAction = TEXT("Delete");
                  break;

               default:
                  ASSERT((*ppri)->riaction == RIA_NOTHING);
                  pcszAction = TEXT("Do nothing to");
                  break;
            }

            TRACE_OUT((TEXT("CreateRecItem(): %s %s."),
                       pcszAction,
                       (*ppri)->pcszName));
         }

#endif

      }
   }
   else
      tr = TR_OUT_OF_MEMORY;

   ASSERT(tr != TR_SUCCESS ||
          IS_VALID_READ_PTR(*ppri, CRECITEM));

   return(tr);
}


/*
** AddObjectTwinRecNode()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT AddObjectTwinRecNode(PRECITEM pri, POBJECTTWIN pot)
{
   TWINRESULT tr = TR_OUT_OF_MEMORY;
   PRECNODE prnNew;

   ASSERT(IS_VALID_READ_PTR(pri, CRECITEM));
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));

   ASSERT(pri->ulcNodes < ULONG_MAX);

   /* Try to allocate a new reconciliation node. */

   if (AllocateMemory(sizeof(*prnNew), &prnNew))
   {
      LPTSTR pszFolder;

      if (AllocatePathString(pot->hpath, &pszFolder))
      {
         /* Fill in RECNODE fields. */

         /* N.b., we don't touch the dwUser field. */

         /*
          * The rnaction field may be changed later during the call to
          * DetermineRecActions().
          */

         prnNew->hvid = (HVOLUMEID)(pot->hpath);
         prnNew->pcszFolder = pszFolder;
         prnNew->hObjectTwin = (HOBJECTTWIN)pot;
         prnNew->priParent = pri;
         prnNew->fsLast = pot->fsLastRec;
         prnNew->rnaction = RNA_NOTHING;
         prnNew->dwFlags = 0;

         /* Set flags. */

         if (IsStubFlagSet(&(pot->stub), STUB_FL_FROM_OBJECT_TWIN))
            SET_FLAG(prnNew->dwFlags, RN_FL_FROM_OBJECT_TWIN);

         if (pot->ulcSrcFolderTwins > 0)
            SET_FLAG(prnNew->dwFlags, RN_FL_FROM_FOLDER_TWIN);

         /* Determine RECNODE file stamp. */

         if (IsStubFlagSet(&(pot->stub), STUB_FL_FILE_STAMP_VALID))
         {
            prnNew->fsCurrent = pot->fsCurrent;

            TRACE_OUT((TEXT("AddObjectTwinRecNode(): Used cached file stamp for object twin %s\\%s."),
                       prnNew->pcszFolder,
                       prnNew->priParent->pcszName));
         }
         else
         {
            MyGetFileStampByHPATH(pot->hpath, prnNew->priParent->pcszName,
                                  &(prnNew->fsCurrent));

            TRACE_OUT((TEXT("AddObjectTwinRecNode(): Determined uncached file stamp for object twin %s\\%s."),
                       prnNew->pcszFolder,
                       prnNew->priParent->pcszName));
         }

         prnNew->rnstate = DetermineRecNodeState(prnNew);

         /* Tie the new RECNODE in to the parent RECITEM's list of RECNODEs. */

         prnNew->prnNext = pri->prnFirst;
         pri->prnFirst = prnNew;

         ASSERT(pri->ulcNodes < ULONG_MAX);
         pri->ulcNodes++;

         LockStub(&(pot->stub));

         tr = TR_SUCCESS;

         ASSERT(IS_VALID_STRUCT_PTR(prnNew, CRECNODE));

         TRACE_OUT((TEXT("AddObjectTwinRecNode(): Adding a RECNODE for object %s\\%s.  RECNODE state is %s."),
                    pszFolder,
                    pri->pcszName,
                    GetRECNODESTATEString(prnNew->rnstate)));
      }
      else
         FreeMemory(prnNew);
   }

   return(tr);
}


/*
** DoesTwinFamilyNeedRec()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL DoesTwinFamilyNeedRec(POBJECTTWIN pot, PVOID ptfri)
{
   BOOL bContinue = FALSE;
   TWINRESULT tr;
   PRECITEM priTemp;

   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(IS_VALID_WRITE_PTR((PTWINFAMILYRECINFO)ptfri, TWINFAMILYRECINFO));

   /*
    * Create a temporary RECITEM for this object twin's twin family to
    * determine whether or not the twin family requires reconciliation.
    */

   tr = CreateRecItem(pot->ptfParent, &priTemp);

   if (tr == TR_SUCCESS)
   {
      if (priTemp->riaction == RIA_NOTHING)
      {
         ((PTWINFAMILYRECINFO)ptfri)->bNeedsRec = FALSE;

         bContinue = TRUE;

         TRACE_OUT((TEXT("DoesTwinFamilyNeedRec(): Twin family for object %s is up-to-date."),
                    priTemp->pcszName));
      }
      else
      {
         ((PTWINFAMILYRECINFO)ptfri)->bNeedsRec = TRUE;

         TRACE_OUT((TEXT("DoesTwinFamilyNeedRec(): Twin family for object %s needs to be reconciled."),
                    priTemp->pcszName));
      }

      DestroyRecItem(priTemp);
   }

   ((PTWINFAMILYRECINFO)ptfri)->tr = tr;

   return(bContinue);
}


/*
** GetFolderPairStatus()
**
** Determines the status of a folder pair.
**
** Arguments:     pfp - pointer to folder pair whose status is to be
**                      determined
**                pfts - pointer to FOLDERTWINSTATUS to be filled in with
**                       reconciliation action that should be taken on the
**                       folder pair, *pfts is filled in with one of the
**                       following values:
**
**                         FTS_DO_NOTHING - no reconciliation required
**                         FTS_DO_SOMETHING - reconciliation required
**                         FTS_UNAVAILABLE - one or both of the folders is
**                                           unavailable
**
** Returns:       TWINRESULT
**
** Side Effects:  Expands intersecting folder twins to object twins.  This may
**                be S-L-O-W.
*/
PRIVATE_CODE TWINRESULT GetFolderPairStatus(PFOLDERPAIR pfp,
                                            CREATERECLISTPROC crlp,
                                            LPARAM lpCallbackData,
                                            PFOLDERTWINSTATUS pfts)
{
   TWINRESULT tr;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));
   ASSERT(! crlp ||
          IS_VALID_CODE_PTR(crlp, CREATERECLISTPROC));
   ASSERT(IS_VALID_WRITE_PTR(pfts, UINT));

   if (IsPathVolumeAvailable(pfp->hpath))
   {
      tr = ExpandIntersectingFolderTwins(pfp, crlp, lpCallbackData);

      if (tr == TR_SUCCESS)
      {
         TWINFAMILYRECINFO tfri;

         /*
          * Walk the list of generated object twins for one half of this folder
          * pair.  Prepare a RECITEM for each object twin's twin family.
          * Continue until one of the RECITEMs requires reconciliation, or we
          * run out of object twins.
          *
          * Both of the lists of object twins in this folder pair should hit
          * exactly the same twin families.
          */

         /* Set defaults in case there are no generated object twins. */

         tfri.tr = TR_SUCCESS;
         tfri.bNeedsRec = FALSE;

         /*
          * EnumGeneratedObjectTwins() returns TRUE if enumeration finished
          * without being stopped by the callback function.
          */

         if (EnumGeneratedObjectTwins(pfp, &DoesTwinFamilyNeedRec, &tfri))
            ASSERT(tfri.tr == TR_SUCCESS && ! tfri.bNeedsRec);
         else
            ASSERT((tfri.tr != TR_SUCCESS) ||
                   (tfri.tr == TR_SUCCESS && tfri.bNeedsRec));

         tr = tfri.tr;

         if (tr == TR_SUCCESS)
         {
            if (tfri.bNeedsRec)
               *pfts = FTS_DO_SOMETHING;
            else
               *pfts = FTS_DO_NOTHING;
         }
      }
   }
   else
   {
      *pfts = FTS_UNAVAILABLE;
      tr = TR_SUCCESS;
   }

   ASSERT(tr != TR_SUCCESS ||
          IsValidFOLDERTWINSTATUS(*pfts));

   return(tr);
}


#ifdef DEBUG

/*
** IsValidCreateRecListProcMsg()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidCreateRecListProcMsg(UINT uMsg)
{
   BOOL bResult;

   switch (uMsg)
   {
      case CRLS_BEGIN_CREATE_REC_LIST:
      case CRLS_DELTA_CREATE_REC_LIST:
      case CRLS_END_CREATE_REC_LIST:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("IsValidCreateRecListProcMsg(): Invalid CreateRecListProc() message %u."),
                    uMsg));
         break;
   }

   return(bResult);
}


/*
** IsValidFOLDERTWINSTATUS()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidFOLDERTWINSTATUS(FOLDERTWINSTATUS fts)
{
   BOOL bResult;

   switch (fts)
   {
      case FTS_DO_NOTHING:
      case FTS_DO_SOMETHING:
      case FTS_UNAVAILABLE:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("IsValidFOLDERTWINSTATUS(): Invalid FOLDERTWINSTATUS %d."),
                    fts));
         break;
   }

   return(bResult);
}


/*
** IsValidPCRECNODESTATECOUNTER()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCRECNODESTATECOUNTER(PCRECNODESTATECOUNTER pcrnscntr)
{
   /* The fields may be any values. */

   return(IS_VALID_READ_PTR(pcrnscntr, CRECNODESTATECOUNTER));
}

#endif


#if defined(DEBUG) || defined(VSTF)

/*
** IsValidRECNODESTATE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidRECNODESTATE(RECNODESTATE rnstate)
{
   BOOL bResult;

   switch (rnstate)
   {
      case RNS_NEVER_RECONCILED:
      case RNS_UNAVAILABLE:
      case RNS_DOES_NOT_EXIST:
      case RNS_DELETED:
      case RNS_NOT_RECONCILED:
      case RNS_UP_TO_DATE:
      case RNS_CHANGED:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("IsValidRECNODESTATE(): Invalid RECNODESTATE %d."),
                    rnstate));
         break;
   }

   return(bResult);
}


/*
** IsValidRECNODEACTION()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidRECNODEACTION(RECNODEACTION rnaction)
{
   BOOL bResult;

   switch (rnaction)
   {
      case RNA_NOTHING:
      case RNA_COPY_FROM_ME:
      case RNA_COPY_TO_ME:
      case RNA_MERGE_ME:
      case RNA_DELETE_ME:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("IsValidRECNODEACTION(): Invalid RECNODEACTION %d."),
                    rnaction));
         break;
   }

   return(bResult);
}


/*
** IsValidRECITEMACTION()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidRECITEMACTION(RECITEMACTION riaction)
{
   BOOL bResult;

   switch (riaction)
   {
      case RIA_NOTHING:
      case RIA_DELETE:
      case RIA_COPY:
      case RIA_MERGE:
      case RIA_BROKEN_MERGE:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("IsValidRECITEMACTION(): Invalid RECITEMACTION %d."),
                    riaction));
         break;
   }

   return(bResult);
}


/*
** IsValidPCRECLIST()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCRECLIST(PCRECLIST pcrl)
{
   BOOL bResult = FALSE;

   if (IS_VALID_READ_PTR(pcrl, CRECLIST) &&
       IS_VALID_HANDLE(pcrl->hbr, BRFCASE))
   {
      PRECITEM pri;
      ULONG ulcRecItems = 0;

      for (pri = pcrl->priFirst;
           pri && IS_VALID_STRUCT_PTR(pri, CRECITEM);
           pri = pri->priNext)
      {
         ASSERT(ulcRecItems < ULONG_MAX);
         ulcRecItems++;
      }

      if (! pri && EVAL(ulcRecItems == pcrl->ulcItems))
         bResult = TRUE;
   }

   return(bResult);
}

#endif


/****************************** Public Functions *****************************/


/*
** IsReconciledFileStamp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsReconciledFileStamp(PCFILESTAMP pcfs)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcfs, CFILESTAMP));

   return(pcfs->fscond != FS_COND_UNAVAILABLE);
}


/*
** LastKnownNonExistent()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL LastKnownNonExistent(PCFILESTAMP pcfsLast,
                                      PCFILESTAMP pcfsCurrent)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcfsLast, CFILESTAMP));
   ASSERT(IS_VALID_STRUCT_PTR(pcfsCurrent, CFILESTAMP));

   return(pcfsCurrent->fscond == FS_COND_DOES_NOT_EXIST ||
          (pcfsCurrent->fscond == FS_COND_UNAVAILABLE &&
           pcfsLast->fscond == FS_COND_DOES_NOT_EXIST));
}


/*
** DetermineDeletionPendingState()
**
**
**
** Arguments:
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void DetermineDeletionPendingState(PCRECITEM pcri)
{
   PCRECNODE pcrn;
   ULONG ulcDeleted = 0;
   ULONG ulcToDelete = 0;
   ULONG ulcChanged = 0;
   ULONG ulcJustDeleted = 0;
   ULONG ulcNeverReconciledTotal = 0;

   /*
    * Don't fully validate *pcri here since we may be called from
    * CreateRecItem() with an incomplete RECITEM.
    */

   ASSERT(IS_VALID_READ_PTR(pcri, CRECITEM));

   /* Count RECNODE states. */

   for (pcrn = pcri->prnFirst; pcrn; pcrn= pcrn->prnNext)
   {
      if (LastKnownNonExistent(&(pcrn->fsLast), &(pcrn->fsCurrent)))
      {
         ASSERT(ulcDeleted < ULONG_MAX);
         ulcDeleted++;

         TRACE_OUT((TEXT("DetermineDeletionPendingState(): %s\\%s last known non-existent."),
                    pcrn->pcszFolder,
                    pcri->pcszName));
      }
      else if (IsStubFlagClear(&(((PCOBJECTTWIN)(pcrn->hObjectTwin))->stub),
                               STUB_FL_KEEP))
      {
         ASSERT(ulcToDelete < ULONG_MAX);
         ulcToDelete++;

         TRACE_OUT((TEXT("DetermineDeletionPendingState(): %s\\%s not explicitly kept."),
                    pcrn->pcszFolder,
                    pcri->pcszName));
      }

      if (IsReconciledFileStamp(&(pcrn->fsLast)))
      {
         if (pcrn->fsCurrent.fscond == FS_COND_EXISTS &&
             MyCompareFileStamps(&(pcrn->fsLast), &(pcrn->fsCurrent)) != CR_EQUAL)
         {
            ASSERT(ulcChanged < ULONG_MAX);
            ulcChanged++;

            TRACE_OUT((TEXT("DetermineDeletionPendingState(): %s\\%s changed."),
                       pcrn->pcszFolder,
                       pcri->pcszName));
         }

         if (pcrn->fsLast.fscond == FS_COND_EXISTS &&
             pcrn->fsCurrent.fscond == FS_COND_DOES_NOT_EXIST)
         {
            ASSERT(ulcJustDeleted < ULONG_MAX);
            ulcJustDeleted++;

            TRACE_OUT((TEXT("DetermineDeletionPendingState(): %s\\%s just deleted."),
                       pcrn->pcszFolder,
                       pcri->pcszName));
         }
      }
      else
      {
         ASSERT(ulcNeverReconciledTotal < ULONG_MAX);
         ulcNeverReconciledTotal++;

         TRACE_OUT((TEXT("DetermineDeletionPendingState(): %s\\%s never reconciled."),
                    pcrn->pcszFolder,
                    pcri->pcszName));
      }
   }

   /*
    * Take twin family out of the deletion pending state if any object twin
    * has changed, or if no object twins are awaiting deletion (i.e., all
    * object twins are deleted or kept).
    *
    * Take twin family in to the deletion pending state if any object twin has
    * just been deleted.
    */

   if (ulcNeverReconciledTotal > 0 ||
       ulcChanged > 0 ||
       ! ulcDeleted ||
       ! ulcToDelete)
   {
      UnmarkTwinFamilyDeletionPending((PTWINFAMILY)(pcri->hTwinFamily));

      if (ulcJustDeleted > 0)
         TRACE_OUT((TEXT("DetermineDeletionPendingState(): One or more object twins of %s deleted, but deletion not pending (%lu never reconciled, %lu changed, %lu deleted, %lu to delete, %lu just deleted)."),
                    pcri->pcszName,
                    ulcNeverReconciledTotal,
                    ulcChanged,
                    ulcDeleted,
                    ulcToDelete,
                    ulcJustDeleted));
   }
   else if (ulcJustDeleted > 0)
      MarkTwinFamilyDeletionPending((PTWINFAMILY)(pcri->hTwinFamily));

   return;
}


/*
** DeleteTwinsFromRecItem()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  May implicitly delete object twins, twin families, and pairs
**                of folder twins.
*/
PUBLIC_CODE BOOL DeleteTwinsFromRecItem(PCRECITEM pcri)
{
   BOOL bObjectTwinsDeleted;
   BOOL bCheckFolderTwins;
   BOOL bFolderTwinsDeleted;
   HPTRARRAY hpaFolderPairs;

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));

   /*
    * DetermineDeletionPendingState() has already been performed by
    * MyReconcileItem() for the twin family of this RECITEM.
    */

   hpaFolderPairs = GetBriefcaseFolderPairPtrArray(((PCTWINFAMILY)(pcri->hTwinFamily))->hbr);

   ClearFlagInArrayOfStubs(hpaFolderPairs, STUB_FL_USED);

   bObjectTwinsDeleted = DeleteDeletedObjectTwins(pcri, &bCheckFolderTwins);

   if (bObjectTwinsDeleted)
      TRACE_OUT((TEXT("DeleteTwinsFromRecItem(): One or more object twins implicitly deleted from twin family for %s."),
                 pcri->pcszName));

   if (bCheckFolderTwins)
   {
      TRACE_OUT((TEXT("DeleteTwinsFromRecItem(): Checking for folder twins to implicitly delete.")));

      bFolderTwinsDeleted = DeleteDeletedFolderTwins(hpaFolderPairs);

      if (bFolderTwinsDeleted)
         TRACE_OUT((TEXT("DeleteTwinsFromRecItem(): One or more pairs of folder twins implicitly deleted.")));
   }
   else
      bFolderTwinsDeleted = FALSE;

   return(bObjectTwinsDeleted || bFolderTwinsDeleted);
}


/*
** DeleteTwinsFromRecList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  May implicitly delete object twins, twin families, and pairs
**                of folder twins.
*/
PUBLIC_CODE BOOL DeleteTwinsFromRecList(PCRECLIST pcrl)
{
   PCRECITEM pcri;
   BOOL bObjectTwinsDeleted = FALSE;
   BOOL bCheckFolderTwins = FALSE;
   BOOL bFolderTwinsDeleted;
   HPTRARRAY hpaFolderPairs;

   ASSERT(IS_VALID_STRUCT_PTR(pcrl, CRECLIST));

   /*
    * DetermineDeletionPendingState() has already been performed by
    * CreateRecItem() for the twin family of each RECITEM in the RECLIST.
    */

   hpaFolderPairs = GetBriefcaseFolderPairPtrArray(pcrl->hbr);

   ClearFlagInArrayOfStubs(hpaFolderPairs, STUB_FL_USED);

   for (pcri = pcrl->priFirst; pcri; pcri = pcri->priNext)
   {
      BOOL bLocalCheckFolderTwins;

      if (DeleteDeletedObjectTwins(pcri, &bLocalCheckFolderTwins))
      {
         TRACE_OUT((TEXT("DeleteTwinsFromRecList(): One or more object twins implicitly deleted from twin family for %s."),
                    pcri->pcszName));

         bObjectTwinsDeleted = TRUE;
      }

      if (bLocalCheckFolderTwins)
         bCheckFolderTwins = TRUE;
   }

   if (bCheckFolderTwins)
   {
      TRACE_OUT((TEXT("DeleteTwinsFromRecList(): Checking for folder twins to implicitly delete.")));

      bFolderTwinsDeleted = DeleteDeletedFolderTwins(hpaFolderPairs);

      if (bFolderTwinsDeleted)
         TRACE_OUT((TEXT("DeleteTwinsFromRecItem(): One or more pairs of folder twins implicitly deleted.")));
   }
   else
      bFolderTwinsDeleted = FALSE;

   return(bObjectTwinsDeleted || bFolderTwinsDeleted);
}


/*
** FindCopySource()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT FindCopySource(PCRECITEM pcri, PRECNODE *pprnCopySrc)
{
   TWINRESULT tr = TR_INVALID_PARAMETER;
   PRECNODE prn;

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));
   ASSERT(IS_VALID_WRITE_PTR(pprnCopySrc, PRECNODE));

   ASSERT(pcri->riaction == RIA_COPY);

   for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
   {
      if (prn->rnaction == RNA_COPY_FROM_ME)
      {
         *pprnCopySrc = prn;
         tr = TR_SUCCESS;
         break;
      }
   }

   ASSERT(tr != TR_SUCCESS ||
          (*pprnCopySrc)->rnaction == RNA_COPY_FROM_ME);

   return(tr);
}


/*
** ChooseMergeDestination()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void ChooseMergeDestination(PCRECITEM pcri,
                                        PRECNODE *pprnMergeDest)
{
   PRECNODE prn;

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));
   ASSERT(IS_VALID_WRITE_PTR(pprnMergeDest, PRECNODE));

   ASSERT(pcri->riaction == RIA_MERGE);

   for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
   {
      if (prn->rnaction == RNA_MERGE_ME)
      {
         *pprnMergeDest = prn;
         break;
      }
   }

   ASSERT(IS_VALID_STRUCT_PTR(*pprnMergeDest, CRECNODE));
   ASSERT((*pprnMergeDest)->rnaction == RNA_MERGE_ME);

   return;
}


/*
** ClearFlagInArrayOfStubs()
**
** Clears flags in all the stubs pointed to by an array of pointers to stubs.
**
** Arguments:     hpa - handle to array of pointers to stubs
**                dwClearFlags - flags to be cleared
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void ClearFlagInArrayOfStubs(HPTRARRAY hpa, DWORD dwClearFlags)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));
   ASSERT(FLAGS_ARE_VALID(dwClearFlags, ALL_STUB_FLAGS));

   aicPtrs = GetPtrCount(hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      ClearStubFlag(GetPtr(hpa, ai), dwClearFlags);

   return;
}


/*
** NotifyCreateRecListStatus()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL NotifyCreateRecListStatus(CREATERECLISTPROC crlp, UINT uMsg,
                                           LPARAM lp, LPARAM lpCallbackData)
{
   BOOL bContinue;

   /* lpCallbackData may be any value. */

   ASSERT(! crlp ||
          IS_VALID_CODE_PTR(crlp, CREATERECLISTPROC));
   ASSERT(IsValidCreateRecListProcMsg(uMsg));
   ASSERT(! lp);

   if (crlp)
   {
      TRACE_OUT((TEXT("NotifyReconciliationStatus(): Calling CREATERECLISTPROC with message %s, LPARAM %#lx, callback data %#lx."),
                 GetCREATERECLISTPROCMSGString(uMsg),
                 lp,
                 lpCallbackData));

      bContinue = (*crlp)(uMsg, lp, lpCallbackData);
   }
   else
   {
      TRACE_OUT((TEXT("NotifyReconciliationStatus(): Not calling NULL CREATERECLISTPROC with message %s, LPARAM %#lx, callback data %#lx."),
                 GetCREATERECLISTPROCMSGString(uMsg),
                 lp,
                 lpCallbackData));

      bContinue = TRUE;
   }

   if (! bContinue)
      WARNING_OUT((TEXT("NotifyCreateRecListStatus(): Client callback aborted RecList creation.")));

   return(bContinue);
}


/*
** CompareInts()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE COMPARISONRESULT CompareInts(int nFirst, int nSecond)
{
   COMPARISONRESULT cr;

   /* nFirst and nSecond may be any value. */

   if (nFirst < nSecond)
      cr = CR_FIRST_SMALLER;
   else if (nFirst > nSecond)
      cr = CR_FIRST_LARGER;
   else
      cr = CR_EQUAL;

   return(cr);
}


#if defined(DEBUG) || defined(VSTF)

/*
** IsValidFILESTAMPCONDITION()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidFILESTAMPCONDITION(FILESTAMPCONDITION fsc)
{
   BOOL bResult;

   switch (fsc)
   {
      case FS_COND_EXISTS:
      case FS_COND_DOES_NOT_EXIST:
      case FS_COND_UNAVAILABLE:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("IsValidFILESTAMPCONDITION(): Unknown FILESTAMPCONDITION %d."),
                    fsc));
         break;
   }

   return(bResult);
}


/*
** IsValidPCFILESTAMP()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCFILESTAMP(PCFILESTAMP pcfs)
{
   /* dwcbLowLength may be any value. */

   return(IS_VALID_READ_PTR(pcfs, CFILESTAMP) &&
          EVAL(IsValidFILESTAMPCONDITION(pcfs->fscond)) &&
          IS_VALID_STRUCT_PTR(&(pcfs->ftMod), CFILETIME) &&
          IS_VALID_STRUCT_PTR(&(pcfs->ftModLocal), CFILETIME) &&
          ! pcfs->dwcbHighLength);
}


/*
** IsFolderObjectTwinFileStamp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsFolderObjectTwinFileStamp(PCFILESTAMP pcfs)
{
   return(EVAL(! pcfs->ftMod.dwLowDateTime) &&
          EVAL(! pcfs->ftMod.dwHighDateTime) &&
          EVAL(! pcfs->dwcbLowLength) &&
          EVAL(! pcfs->dwcbHighLength));
}


/*
** IsValidPCRECNODE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCRECNODE(PCRECNODE pcrn)
{
   return(IS_VALID_READ_PTR(pcrn, CRECNODE) &&
          IS_VALID_HANDLE(pcrn->hvid, VOLUMEID) &&
          IS_VALID_STRING_PTR(pcrn->pcszFolder, CSTR) &&
          IS_VALID_HANDLE(pcrn->hObjectTwin, OBJECTTWIN) &&
          IS_VALID_STRUCT_PTR(&(pcrn->fsLast), CFILESTAMP) &&
          IS_VALID_STRUCT_PTR(&(pcrn->fsCurrent), CFILESTAMP) &&
          FLAGS_ARE_VALID(pcrn->dwFlags, ALL_RECNODE_FLAGS) &&
          EVAL(IsValidRECNODESTATE(pcrn->rnstate)) &&
          EVAL(IsValidRECNODEACTION(pcrn->rnaction)) &&
          EVAL(*(pcrn->priParent->pcszName) ||
               (IsFolderObjectTwinFileStamp(&(pcrn->fsLast)) &&
                IsFolderObjectTwinFileStamp(&(pcrn->fsCurrent)))) &&
          EVAL(IsReconciledFileStamp(&(pcrn->fsCurrent)) ||
               MyCompareFileStamps(&(pcrn->fsLast), &(((PCOBJECTTWIN)(pcrn->hObjectTwin))->fsLastRec)) == CR_EQUAL));
}


/*
** IsValidPCRECITEM()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCRECITEM(PCRECITEM pcri)
{
   BOOL bResult = FALSE;

   /* Does the twin family associated with this RECITEM still exist? */

   if (IS_VALID_READ_PTR(pcri, CRECITEM) &&
       IS_VALID_STRING_PTR(pcri->pcszName, CSTR) &&
       IS_VALID_HANDLE(pcri->hTwinFamily, TWINFAMILY))
   {
      if (IsStubFlagSet(&(((PTWINFAMILY)(pcri->hTwinFamily))->stub),
                        STUB_FL_UNLINKED))
         bResult = TRUE;
      else
      {
         ULONG ulcRecNodes = 0;
         PRECNODE prn;

         /*
          * Yes.  Verify the parent pointers, node count, and flags in this
          * RECITEM.
          */

         /* All unavailable RECNODEs should contain action RNA_NOTHING. */

         for (prn = pcri->prnFirst;
              prn && IS_VALID_STRUCT_PTR(prn, CRECNODE);
              prn = prn->prnNext)
         {
            /* Does the object twin associated with this RECNODE still exist? */

            if (IsStubFlagClear(&(((PTWINFAMILY)(pcri->hTwinFamily))->stub),
                                STUB_FL_UNLINKED))
            {
               /* Yes.  Verify its parent RECITEM pointer. */

               if (prn->priParent != pcri)
               {
                  ERROR_OUT((TEXT("IsValidPCRECITEM(): Bad parent pointer found in RECNODE - parent pointer (%#lx), actual parent (%#lx)."),
                             prn->priParent,
                             pcri));

                  break;
               }
            }

            ASSERT(ulcRecNodes < ULONG_MAX);
            ulcRecNodes++;
         }

         if (! prn)
         {
            /* Check RECNODE count. */

            if (ulcRecNodes == pcri->ulcNodes)
            {
               if (ulcRecNodes >= 2)
               {
                  /* Now verify the RECITEM's actions. */

                  switch (pcri->riaction)
                  {
                     case RIA_NOTHING:

                        /* All RECNODEs should contain action RNA_NOTHING. */

                        for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
                        {
                           if (prn->rnaction != RNA_NOTHING)
                           {
                              ERROR_OUT((TEXT("IsValidPCRECITEM(): Nop RECITEM with non-nop RECNODE action %d."),
                                         prn->rnaction));
                              break;
                           }
                        }

                        if (! prn)
                           bResult = TRUE;

                        break;

                     case RIA_COPY:
                     {
                        PRECNODE prnSrc = NULL;
                        ULONG ulcCopyDests = 0;

                        /*
                         * There should only be one available RECNODE
                         * containing action RNA_COPY_FROM_ME.
                         *
                         * The other available RECNODEs should contain action
                         * RNA_COPY_TO_ME or RNA_NOTHING.
                         */

                        for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
                        {
                           if (RECNODE_IS_AVAILABLE(prn))
                           {
                              switch (prn->rnaction)
                              {
                                 case RNA_COPY_TO_ME:
                                    ASSERT(ulcCopyDests < ULONG_MAX);
                                    ulcCopyDests++;
                                    break;

                                 case RNA_NOTHING:
                                    break;

                                 case RNA_COPY_FROM_ME:
                                    if (! prnSrc)
                                       prnSrc = prn;
                                    else
                                       ERROR_OUT((TEXT("IsValidPCRECITEM(): Copy RECITEM with multiple source file RECNODEs.")));
                                    break;

                                 case RNA_MERGE_ME:
                                    ERROR_OUT((TEXT("IsValidPCRECITEM(): Copy RECITEM with merge RECNODE.")));
                                    break;

                                 default:
                                    ERROR_OUT((TEXT("IsValidPCRECITEM(): Copy RECITEM with unknown RECNODE action %d."),
                                               prn->rnaction));
                                    break;
                              }
                           }
                        }

                        if (! prn)
                        {
                           /* Did we find a copy source? */

                           if (prnSrc)
                           {
                              /* Yes. */

                              /* Did we find one or more copy destinations? */

                              if (ulcCopyDests > 0)
                                 /* Yes. */
                                 bResult = TRUE;
                              else
                                 /* No. */
                                 ERROR_OUT((TEXT("IsValidPCRECITEM(): Copy RECITEM with no copy destination RECNODEs.")));
                           }
                           else
                              /* No. */
                              ERROR_OUT((TEXT("IsValidPCRECITEM(): Copy RECITEM with no copy source RECNODE.")));
                        }

                        break;
                     }

                     case RIA_MERGE:
                     case RIA_BROKEN_MERGE:
                     {
                        PRECNODE prn;
                        ULONG ulcMergeBuddies = 0;
#ifdef DEBUG
                        LPCTSTR pcszAction = (pcri->riaction == RIA_MERGE) ?
                                           TEXT("merge") :
                                           TEXT("broken merge");
#endif

                        /*
                         * There should be multiple available RECNODEs
                         * containing action RNA_MERGE_ME.
                         *
                         * The other available RECNODEs should contain action
                         * RNA_COPY_TO_ME.
                         */

                        for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
                        {
                           if (RECNODE_IS_AVAILABLE(prn))
                           {
                              switch (prn->rnaction)
                              {
                                 case RNA_COPY_TO_ME:
                                    break;

                                 case RNA_NOTHING:
                                    ERROR_OUT((TEXT("IsValidPCRECITEM(): %s RECITEM with RNA_NOTHING RECNODE."),
                                               pcszAction));
                                    break;

                                 case RNA_COPY_FROM_ME:
                                    ERROR_OUT((TEXT("IsValidPCRECITEM(): %s RECITEM with RNA_COPY_FROM_ME RECNODE."),
                                               pcszAction));
                                    break;

                                 case RNA_MERGE_ME:
                                    ASSERT(ulcMergeBuddies < ULONG_MAX);
                                    ulcMergeBuddies++;
                                    break;

                                 default:
                                    ERROR_OUT((TEXT("IsValidPCRECITEM(): %s RECITEM with unknown RECNODE action %d."),
                                               pcszAction,
                                               prn->rnaction));
                                    break;
                              }
                           }
                        }

                        if (! prn)
                        {
                           /* Are there multiple merge source RECNODEs? */

                           if (ulcMergeBuddies > 1)
                              bResult = TRUE;
                           else
                              ERROR_OUT((TEXT("IsValidPCRECITEM(): %s RECITEM with too few (%lu) merge source RECNODEs."),
                                         pcszAction,
                                         ulcMergeBuddies));
                        }

                        break;
                     }

                     case RIA_DELETE:
                     {
                        BOOL bDelete = FALSE;

                        /*
                         * There should be at least one available RECNODE
                         * marked RNA_DELETE_ME.  All other RECNODEs should be
                         * marked RNA_NOTHING.
                         */

                        for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
                        {
                           if (RECNODE_IS_AVAILABLE(prn) &&
                               prn->rnaction == RNA_DELETE_ME)
                              bDelete = TRUE;
                           else if (prn->rnaction != RNA_NOTHING)
                              ERROR_OUT((TEXT("IsValidPCRECITEM(): Delete RECITEM with RECNODE marked %s."),
                                         GetRECNODEACTIONString(prn->rnaction)));
                        }

                        if (bDelete)
                           bResult = TRUE;
                        else
                           ERROR_OUT((TEXT("IsValidPCRECITEM(): Delete RECITEM with no RECNODEs marked RNA_DELETE_ME.")));

                        break;
                     }

                     default:
                        ERROR_OUT((TEXT("IsValidPCRECITEM(): Unrecognized RECITEMACTION %d."),
                                   pcri->riaction));
                        break;
                  }
               }
               else
                  ERROR_OUT((TEXT("IsValidPCRECITEM(): RECITEM only has %lu RECNODEs."),
                             ulcRecNodes));
            }
            else
               ERROR_OUT((TEXT("IsValidPCRECITEM(): RECITEM has bad RECNODE count.  (%lu actual RECNODEs for RECITEM claiming %lu RECNODEs.)"),
                          ulcRecNodes,
                          pcri->ulcNodes));
         }
      }
   }

   return(bResult);
}

#endif


/***************************** Exported Functions ****************************/

/* RAIDRAID: (16203) AutoDoc CREATERECLISTPROC messages below. */

/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | CreateRecList | Creates a reconciliation list for all twins
or for marked twins.

@parm HTWINLIST | htl | A handle to the twin list that the reconciliation list
is to be created from.  All twins in the twin list generate RECITEMs in the
reconciliation list.

@parm CREATERECLISTPROC | crlp | A procedure instance address of a callback
function to be called with status information during the creation of the
RECLIST.  crlp may be NULL to indicate that no RECLIST creation status callback
function is to be called.

@parm LPARAM | lpCallbackData | Callback data to be supplied to the RECLIST
creation status callback function.  If crlp is NULL, lpCallbackData is ignored.

@parm PRECLIST * | pprl | A pointer to a RECLIST to be filled in with a
pointer to the new reconciliation list.  *pprl is only valid if TR_SUCCESS is
returned.

@rdesc If the reconciliation list was created successfully, TR_SUCCESS is
returned.  Otherwise, the reconciliation list was not created successfully, and
the return value indicates the error that occurred.

@xref DestroyRecList MarkTwin

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI CreateRecList(HTWINLIST htl,
                                           CREATERECLISTPROC crlp,
                                           LPARAM lpCallbackData,
                                           PRECLIST *pprl)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(CreateRecList);

#ifdef EXPV
      /* Verify parameters. */

      /* lpCallbackData may be any value. */

      if (IS_VALID_HANDLE(htl, TWINLIST) &&
          (! crlp ||
           IS_VALID_CODE_PTR(crlp, CREATERECLISTPROC)) &&
          IS_VALID_WRITE_PTR(pprl, PRECLIST))
#endif
      {
         if (NotifyCreateRecListStatus(crlp, CRLS_BEGIN_CREATE_REC_LIST, 0,
                                       lpCallbackData))
         {
            HBRFCASE hbr;

            /* Expand the required folder twins to object twins. */

            hbr = GetTwinListBriefcase(htl);

            InvalidatePathListInfo(GetBriefcasePathList(hbr));

            tr = ExpandFolderTwinsIntersectingTwinList(htl, crlp,
                                                       lpCallbackData);

            if (tr == TR_SUCCESS)
            {
               PRECLIST prlNew;

               /* Try to create a new reconciliation list. */

               if (AllocateMemory(sizeof(*prlNew), &prlNew))
               {
                  /* Initialize RECLIST structure fields. */

                  prlNew->ulcItems = 0;
                  prlNew->priFirst = NULL;
                  prlNew->hbr = hbr;

                  tr = AddRecItemsToRecList(htl, crlp, lpCallbackData, prlNew);

                  if (tr == TR_SUCCESS)
                  {
                     if (DeleteTwinsFromRecList(prlNew))
                     {
                        TRACE_OUT((TEXT("CreateRecList(): Twins implicitly deleted.  Recalculating RECLIST.")));

                        DestroyListOfRecItems(prlNew->priFirst);

                        prlNew->ulcItems = 0;
                        prlNew->priFirst = NULL;
                        ASSERT(prlNew->hbr == hbr);

                        tr = AddRecItemsToRecList(htl, crlp, lpCallbackData,
                                                  prlNew);
                     }
                  }

                  if (tr == TR_SUCCESS)
                  {
                     *pprl = prlNew;

                     /* Don't allow abort. */

                     NotifyCreateRecListStatus(crlp, CRLS_END_CREATE_REC_LIST,
                                               0, lpCallbackData);

                     ASSERT(IS_VALID_STRUCT_PTR(*pprl, CRECLIST));
                  }
                  else
                     /*
                      * Destroy RECLIST and any RECITEMs that have been created
                      * so far.
                      */
                     MyDestroyRecList(prlNew);
               }
            }
         }
         else
            tr = TR_ABORT;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(CreateRecList, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | DestroyRecList | Destroys a reconciliation list created by
CreateRecList().

@parm PRECLIST | prl | A pointer to the reconciliation list to be destroyed.
The RECLIST pointed by pRecList is not valid after DestroyRecList() is called.

@rdesc If the specified reconciliation list was freed successfully, TR_SUCCESS
is returned.  Otherwise, the specified reconciliation list was not freed
successfully, and the return value indicates the error that occurred.

@xref CreateRecList

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI DestroyRecList(PRECLIST prl)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(DestroyRecList);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_STRUCT_PTR(prl, CRECLIST))
#endif
      {
         MyDestroyRecList(prl);

         tr = TR_SUCCESS;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(DestroyRecList, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | GetFolderTwinStatus | Determines the reconciliation status of
a folder twin.

@parm HFOLDERTWIN | hFolderTwin | A handle to the folder twin whose
reconciliation status is to be determined.

@parm CREATERECLISTPROC | crlp | A procedure instance address of a callback
function to be called with status information during the creation of the
RECLIST.  crlp may be NULL to indicate that no RECLIST creation status callback
function is to be called.

@parm LPARAM | lpCallbackData | Callback data to be supplied to the RECLIST
creation status callback function.  If crlp is NULL, lpCallbackData is ignored.

@parm PFOLDERTWINSTATUS | pfts | A pointer to a UINT to be filled in with the
reconciliation status of the folder twin.  *pfts is only valid if TR_SUCCESS is
returned.  *pfts may be one of the following values:

@flag FTS_DO_NOTHING | This folder twin is up-to-date.  No reconciliation
action needs to be taken on it.  N.b., the folder may still contain object
twins that were not generated by the folder twin that are out-of-date.

@flag FTS_DO_SOMETHING | This folder twin is out-of-date.  Some reconciliation
action needs to be taken on it.

@flag FTS_UNAVAILABLE | This folder twin is unavailable for reconciliation.

@rdesc If the reconcilation status of the folder twin was determined
successfully, *pfts is filled in with the status of the folder twin, and
TR_SUCCESS is returned.  Otherwise, the reconciliation status of the folder
twin was not determined successfully, *pfts is undefined, and the return
value indicates the error that occurred.

@comm If GetFolderTwinStatus() is called with a valid handle to a folder twin
that has been deleted, TR_DELETED_TWIN will be returned.  N.b., in general,
calling GetFolderTwinStatus() for a folder twin will not be any slower than
calling CreateRecList() for that folder twin, and may be significantly faster
if the folder twin requires reconciliation.

@xref AddFolderTwin CreateFolderTwinList

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI GetFolderTwinStatus(HFOLDERTWIN hFolderTwin,
                                                 CREATERECLISTPROC crlp,
                                                 LPARAM lpCallbackData,
                                                 PFOLDERTWINSTATUS pfts)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(GetFolderTwinStatus);

#ifdef EXPV
      /* Verify parameters. */

      /* lpCallbackData may be any value. */

      if (IS_VALID_HANDLE(hFolderTwin, FOLDERTWIN) &&
          EVAL(! crlp ||
               IS_VALID_CODE_PTR(crlp, CREATERECLISTPROC)) &&
          IS_VALID_WRITE_PTR(pfts, FOLDERTWINSTATUS))
#endif
      {
         /* Has this folder twin already been deleted? */

         if (IsStubFlagClear(&(((PFOLDERPAIR)hFolderTwin)->stub),
                             STUB_FL_UNLINKED))
         {
            if (NotifyCreateRecListStatus(crlp, CRLS_BEGIN_CREATE_REC_LIST, 0,
                                          lpCallbackData))
            {
               /* No. Determine its status. */

               InvalidatePathListInfo(
                  GetBriefcasePathList(((PCFOLDERPAIR)hFolderTwin)->pfpd->hbr));

               tr = GetFolderPairStatus((PFOLDERPAIR)hFolderTwin, crlp,
                                        lpCallbackData, pfts);

               if (tr == TR_SUCCESS)
               {
                  /* Don't allow abort. */

                  NotifyCreateRecListStatus(crlp, CRLS_END_CREATE_REC_LIST, 0,
                                            lpCallbackData);

                  if (IsStubFlagSet(&(((PFOLDERPAIR)hFolderTwin)->stub),
                                    STUB_FL_UNLINKED))
                  {
                     WARNING_OUT((TEXT("GetFolderTwinStatus(): Folder twin deleted during status determination.")));

                     tr = TR_DELETED_TWIN;
                  }

#ifdef DEBUG

                  {
                     LPCTSTR pcszStatus;

                     switch (*pfts)
                     {
                        case FTS_DO_NOTHING:
                           pcszStatus = TEXT("FTS_DO_NOTHING");
                           break;

                        case FTS_DO_SOMETHING:
                           pcszStatus = TEXT("FTS_DO_SOMETHING");
                           break;

                        default:
                           ASSERT(*pfts == FTS_UNAVAILABLE);
                           pcszStatus = TEXT("FTS_UNAVAILABLE");
                           break;
                     }

                     TRACE_OUT((TEXT("GetFolderTwinStatus(): Status of folder %s is %s."),
                                DebugGetPathString(((PFOLDERPAIR)hFolderTwin)->hpath),
                                pcszStatus));
                  }

#endif

               }
            }
            else
               tr = TR_ABORT;
         }
         else
         {
            /* Yes.  Bail. */

            WARNING_OUT((TEXT("GetFolderTwinStatus(): Called on deleted folder twin.")));

            tr = TR_DELETED_TWIN;
         }
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      ASSERT(tr != TR_SUCCESS ||
             IsValidFOLDERTWINSTATUS(*pfts));

      DebugExitTWINRESULT(GetFolderTwinStatus, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}

