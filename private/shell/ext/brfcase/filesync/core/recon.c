/*
 * recon.c - Reconciliation routines.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "stub.h"
#include "oleutil.h"


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE void GenerateShellEvents(PCRECITEM);
PRIVATE_CODE TWINRESULT MyReconcileItem(PCRECITEM, RECSTATUSPROC, LPARAM, DWORD, HWND, HWND);
PRIVATE_CODE void UpdateObjectTwinStates(PCRECITEM);
PRIVATE_CODE TWINRESULT CopyFolder(PCRECITEM, RECSTATUSPROC, LPARAM);
PRIVATE_CODE TWINRESULT DeleteFolder(PCRECITEM, RECSTATUSPROC, LPARAM);
PRIVATE_CODE TWINRESULT DealWithCopy(PCRECITEM, RECSTATUSPROC, LPARAM, DWORD, HWND, HWND);
PRIVATE_CODE TWINRESULT DealWithMerge(PCRECITEM, RECSTATUSPROC, LPARAM, DWORD, HWND, HWND);
PRIVATE_CODE TWINRESULT DealWithDelete(PCRECITEM, RECSTATUSPROC, LPARAM);
PRIVATE_CODE ULONG CountRECNODEs(PCRECITEM, RECNODEACTION);
PRIVATE_CODE TWINRESULT UpdateRecNodeFileStamps(PCRECITEM);
PRIVATE_CODE BOOL DeletedTwinsInRecItem(PCRECITEM);


/*
** GenerateShellEvents()
**
** Notifies the Shell about reconciliation events for a RECITEM.
**
** Arguments:     pcri - reconciled RECITEM to notify Shell about
**
** Returns:       void
**
** Side Effects:  None
*/
PRIVATE_CODE void GenerateShellEvents(PCRECITEM pcri)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));

   /* Any reconciliation events to report? */

   if (pcri->riaction == RIA_NOTHING ||
       pcri->riaction == RIA_COPY ||
       pcri->riaction == RIA_MERGE ||
       pcri->riaction == RIA_DELETE)
   {
      PRECNODE prn;

      /*
       * Yes.  Send an appropriate notification to the Shell about the file
       * operations assumed carried out during reconciliation.  The file system
       * is lame, and does not support notifications for some of the
       * interesting reconciliation operations.  We also generate a specious
       * update notification for the source file in a copy operation to cause
       * the Briefcase ui to recalculate the status string for that file, even
       * though the file itself has not changed.
       */

      for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
      {
         BOOL bNotify;
         NOTIFYSHELLEVENT nse;
         LPCTSTR pcszPath;
         TCHAR rgchPath[MAX_PATH_LEN];

         /* How shall I notify you?  Let me enumerate the ways. */

         bNotify = TRUE;

         if (IsFolderObjectTwinName(pcri->pcszName))
         {
            nse = NSE_UPDATE_FOLDER;

            pcszPath = prn->pcszFolder;

            switch (prn->rnaction)
            {
               /*
                * Notifications about folders that were copied or deleted
                * during reconciliation were sent during reconciliation.  Don't
                * send redundant notifications.
                */
               case RNA_COPY_TO_ME:
               case RNA_DELETE_ME:
                  bNotify = FALSE;
                  break;

               default:
                  ASSERT(prn->rnaction == RNA_NOTHING ||
                         prn->rnaction == RNA_COPY_FROM_ME);
                  break;
            }
         }
         else
         {
            nse = NSE_UPDATE_ITEM;

            ComposePath(rgchPath, prn->pcszFolder, pcri->pcszName);
            pcszPath = rgchPath;

            switch (prn->rnaction)
            {
               case RNA_COPY_TO_ME:
                  if (prn->rnstate == RNS_DOES_NOT_EXIST ||
                      prn->rnstate == RNS_DELETED)
                     nse = NSE_CREATE_ITEM;
                  break;

               case RNA_DELETE_ME:
                  nse = NSE_DELETE_ITEM;
                  break;

               default:
                  ASSERT(prn->rnaction == RNA_NOTHING ||
                         prn->rnaction == RNA_COPY_FROM_ME ||
                         prn->rnaction == RNA_MERGE_ME);
                  break;
            }
         }

         if (bNotify)
            NotifyShell(pcszPath, nse);
      }
   }
}


/*
** MyReconcileItem()
**
** Reconciles a reconciliation item.
**
** Arguments:     pcri - pointer to reconciliation item to be reconciled
**
** Side Effects:
*/
PRIVATE_CODE TWINRESULT MyReconcileItem(PCRECITEM pcri, RECSTATUSPROC rsp,
                                        LPARAM lpCallbackData, DWORD dwFlags,
                                        HWND hwndOwner,
                                        HWND hwndProgressFeedback)
{
   TWINRESULT tr;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));
   ASSERT(! rsp ||
          IS_VALID_CODE_PTR(rsp, RECSTATUSPROC));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_RI_FLAGS));
   ASSERT(IS_FLAG_CLEAR(dwFlags, RI_FL_ALLOW_UI) ||
          IS_VALID_HANDLE(hwndOwner, WND));
   ASSERT(IS_FLAG_CLEAR(dwFlags, RI_FL_FEEDBACK_WINDOW_VALID) ||
          IS_VALID_HANDLE(hwndProgressFeedback, WND));

#ifdef DEBUG

   {
      LPCTSTR pcszGerund;

      switch (pcri->riaction)
      {
         case RIA_NOTHING:
            pcszGerund = TEXT("Taking no action on");
            break;

         case RIA_COPY:
            pcszGerund = TEXT("Copying");
            break;

         case RIA_MERGE:
            pcszGerund = TEXT("Merging");
            break;

         case RIA_BROKEN_MERGE:
            pcszGerund = TEXT("Broken merge for");
            break;

         case RIA_DELETE:
            pcszGerund = TEXT("Deleting");
            break;

         default:
            pcszGerund = TEXT("Unknown action specifed for");
            break;
      }

      TRACE_OUT((TEXT("MyReconcileItem(): %s %s."),
                 pcszGerund,
                 *(pcri->pcszName) ? pcri->pcszName : TEXT("folder")));
   }

#endif

   switch (pcri->riaction)
   {
      case RIA_NOTHING:
         tr = TR_SUCCESS;
         break;

      case RIA_COPY:
         if (*(pcri->pcszName))
            tr = DealWithCopy(pcri, rsp, lpCallbackData, dwFlags, hwndOwner,
                              hwndProgressFeedback);
         else
            tr = CopyFolder(pcri, rsp, lpCallbackData);

         if (tr == TR_SUCCESS)
            tr = UpdateRecNodeFileStamps(pcri);
         break;

      case RIA_MERGE:
         tr = DealWithMerge(pcri, rsp, lpCallbackData, dwFlags, hwndOwner,
                            hwndProgressFeedback);

         if (tr == TR_SUCCESS || tr == TR_MERGE_INCOMPLETE)
            tr = UpdateRecNodeFileStamps(pcri);
         break;

      case RIA_DELETE:
         if (*(pcri->pcszName))
            tr = DealWithDelete(pcri, rsp, lpCallbackData);
         else
         {
            tr = DeleteFolder(pcri, rsp, lpCallbackData);

            if (tr == TR_DEST_WRITE_FAILED)
               tr = TR_SUCCESS;
         }

         if (tr == TR_SUCCESS)
            tr = UpdateRecNodeFileStamps(pcri);
         break;

      default:
         ASSERT(pcri->riaction == RIA_BROKEN_MERGE);
         tr = TR_NO_MERGE_HANDLER;
         break;
   }

   /*
    * Only update briefcase time stamps if the entire reconciliation operation
    * on this RECITEM is successful.  However, the RECNODE time stamps in the
    * given RECITEM have been updated as they were changed.
    */

   if (tr == TR_SUCCESS)
   {
      UpdateObjectTwinStates(pcri);

      DetermineDeletionPendingState(pcri);

      DeleteTwinsFromRecItem(pcri);
   }
   else if (tr == TR_MERGE_INCOMPLETE)
      tr = TR_SUCCESS;

   if (tr == TR_SUCCESS)
      GenerateShellEvents(pcri);

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));

   return(tr);
}


/*
** UpdateObjectTwinStates()
**
** Updates the last reconciliation time stamp of the twin family and the object
** twins associated with a RECITEM that has just been successfully reconciled.
**
** Arguments:     pri - pointer to reconciliation item that has just been
**                      reconciled
**
** Returns:       TWINRESULT
**
** Side Effects:  Implicitly deletes twin family if the last known state of
**                every component object twins is non-existence.
**
** N.b., this function assumes that the actions specified in the RECNODEs of
** the RECITEM were carried out successfully.
**
** This function assumes that all available RECNODES were reconciled.
**
** This function assumes that the time stamp fields of the RECNODEs associated
** with objects that were modified during reconciliation were filled in
** immediately after each of those RECNODEs was reconciled.  I.e., that all
** time stamp fields in reconciled RECNODEs are up-to-date with respect to any
** modifications that may have been made to them during reconciliation.
*/
PRIVATE_CODE void UpdateObjectTwinStates(PCRECITEM pcri)
{
   PRECNODE prn;
   BOOL bNewVersion = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));

   /*
    * There is a new version if any changed or never reconciled RECNODEs were
    * reconciled as RNA_NOTHING, RNA_COPY_FROM_ME, or RNA_MERGE_ME.
    */

   for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
   {
      if ((prn->rnstate == RNS_NEVER_RECONCILED ||
           prn->rnstate == RNS_CHANGED) &&
          (prn->rnaction == RNA_NOTHING ||
           prn->rnaction == RNA_COPY_FROM_ME ||
           prn->rnaction == RNA_MERGE_ME))
         bNewVersion = TRUE;
   }

   /*
    * Save file stamps of available files.  Mark unavailable object twins not
    * reconciled if any new versions exist in the reconciled set of files.
    */

   for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
   {
      POBJECTTWIN pot;

      pot = (POBJECTTWIN)(prn->hObjectTwin);

      if (prn->fsCurrent.fscond != FS_COND_UNAVAILABLE)
      {
         ClearStubFlag(&(pot->stub), STUB_FL_NOT_RECONCILED);
         pot->fsLastRec = prn->fsCurrent;

         /*
          * Remember not to delete object twins as requested.  Treat any folder
          * that could not be deleted as implicitly kept as well.
          */

         if (IS_FLAG_SET(prn->dwFlags, RN_FL_DELETION_SUGGESTED) &&
             IsTwinFamilyDeletionPending((PCTWINFAMILY)(pcri->hTwinFamily)) &&
             (pcri->riaction == RIA_NOTHING ||
              pcri->riaction == RIA_DELETE) &&
             (prn->rnaction != RNA_DELETE_ME ||
              IsFolderObjectTwinName(pcri->pcszName)))
         {
            SetStubFlag(&(pot->stub), STUB_FL_KEEP);

            TRACE_OUT((TEXT("UpdateObjectTwinStates(): Object twin %s\\%s will be kept and not deleted."),
                       prn->pcszFolder,
                       prn->priParent->pcszName));
         }
      }
      else if (bNewVersion &&
               IsReconciledFileStamp(&(prn->fsLast)))
      {
         SetStubFlag(&(pot->stub), STUB_FL_NOT_RECONCILED);

         WARNING_OUT((TEXT("UpdateObjectTwinStates(): Marked %s\\%s not reconciled."),
                      prn->pcszFolder,
                      pcri->pcszName));
      }
   }

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));

   return;
}


/*
** CopyFolder()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT CopyFolder(PCRECITEM pcri, RECSTATUSPROC rsp,
                                   LPARAM lpCallbackData)
{
   TWINRESULT tr;
   RECSTATUSUPDATE rsu;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));
   ASSERT(! rsp ||
          IS_VALID_CODE_PTR(rsp, RECSTATUSPROC));

   ASSERT(IsFolderObjectTwinName(pcri->pcszName));

   rsu.ulScale = CountRECNODEs(pcri, RNA_COPY_TO_ME);
   ASSERT(rsu.ulScale > 0);
   rsu.ulProgress = 0;

   if (NotifyReconciliationStatus(rsp, RS_BEGIN_COPY, (LPARAM)&rsu,
                                  lpCallbackData))
   {
      PRECNODE prn;

      tr = TR_SUCCESS;

      for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
      {
         if (prn->rnaction == RNA_COPY_TO_ME)
            tr = CreateFolders(prn->pcszFolder, (HPATH)(prn->hvid));
      }

      if (tr == TR_SUCCESS)
      {
         /* 100% complete. */

         rsu.ulProgress = rsu.ulScale;

         /* Don't allow abort. */

         NotifyReconciliationStatus(rsp, RS_END_COPY, (LPARAM)&rsu,
                                    lpCallbackData);
      }
   }
   else
      tr = TR_ABORT;

   return(tr);
}


/*
** DeleteFolder()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT DeleteFolder(PCRECITEM pcri, RECSTATUSPROC rsp,
                                     LPARAM lpCallbackData)
{
   TWINRESULT tr;
   RECSTATUSUPDATE rsu;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));
   ASSERT(! rsp ||
          IS_VALID_CODE_PTR(rsp, RECSTATUSPROC));

   ASSERT(IsFolderObjectTwinName(pcri->pcszName));

   rsu.ulScale = CountRECNODEs(pcri, RNA_DELETE_ME);
   ASSERT(rsu.ulScale > 0);
   rsu.ulProgress = 0;

   if (NotifyReconciliationStatus(rsp, RS_BEGIN_DELETE, (LPARAM)&rsu,
                                  lpCallbackData))
   {
      PRECNODE prn;

      tr = TR_SUCCESS;

      for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
      {
         if (prn->rnaction == RNA_DELETE_ME)
            tr = DestroySubtree(prn->pcszFolder, (HPATH)(prn->hvid));
      }

      if (tr == TR_SUCCESS)
      {
         /* 100% complete. */

         rsu.ulProgress = rsu.ulScale;

         /* Don't allow abort. */

         NotifyReconciliationStatus(rsp, RS_END_DELETE, (LPARAM)&rsu,
                                    lpCallbackData);
      }
   }
   else
      tr = TR_ABORT;

   return(tr);
}


/*
** DealWithCopy()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT DealWithCopy(PCRECITEM pcri, RECSTATUSPROC rsp,
                                     LPARAM lpCallbackData, DWORD dwInFlags,
                                     HWND hwndOwner, HWND hwndProgressFeedback)
{
   TWINRESULT tr;
   PRECNODE prnCopySrc;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));
   ASSERT(! rsp ||
          IS_VALID_CODE_PTR(rsp, RECSTATUSPROC));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_RI_FLAGS));
   ASSERT(IS_FLAG_CLEAR(dwInFlags, RI_FL_ALLOW_UI) ||
          IS_VALID_HANDLE(hwndOwner, WND));
   ASSERT(IS_FLAG_CLEAR(dwInFlags, RI_FL_FEEDBACK_WINDOW_VALID) ||
          IS_VALID_HANDLE(hwndProgressFeedback, WND));

   tr = FindCopySource(pcri, &prnCopySrc);

   if (EVAL(tr == TR_SUCCESS))
      tr = CopyHandler(prnCopySrc, rsp, lpCallbackData, dwInFlags, hwndOwner,
                       hwndProgressFeedback);

   return(tr);
}


/*
** DealWithMerge()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT DealWithMerge(PCRECITEM pcri, RECSTATUSPROC rsp,
                                      LPARAM lpCallbackData, DWORD dwInFlags,
                                      HWND hwndOwner,
                                      HWND hwndProgressFeedback)
{
   TWINRESULT tr;
   HRESULT hr;
   PRECNODE prnMergeDest;
   PRECNODE prnMergedResult;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));
   ASSERT(! rsp ||
          IS_VALID_CODE_PTR(rsp, RECSTATUSPROC));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_RI_FLAGS));
   ASSERT(IS_FLAG_CLEAR(dwInFlags, RI_FL_ALLOW_UI) ||
          IS_VALID_HANDLE(hwndOwner, WND));
   ASSERT(IS_FLAG_CLEAR(dwInFlags, RI_FL_FEEDBACK_WINDOW_VALID) ||
          IS_VALID_HANDLE(hwndProgressFeedback, WND));

   ChooseMergeDestination(pcri, &prnMergeDest);

   hr = MergeHandler(prnMergeDest, rsp, lpCallbackData, dwInFlags, hwndOwner,
                     hwndProgressFeedback, &prnMergedResult);

   if (hr == S_OK ||
       hr == REC_S_NOTCOMPLETEBUTPROPAGATE)
   {
      tr = CopyHandler(prnMergedResult, rsp, lpCallbackData, dwInFlags,
                       hwndOwner, hwndProgressFeedback);

      if (tr == TR_SUCCESS)
         TRACE_OUT((TEXT("DealWithMerge(): Propagated merged result %s\\%s successfully."),
                    prnMergedResult->pcszFolder,
                    pcri->pcszName));
      else
         WARNING_OUT((TEXT("DealWithMerge(): Propagating merged result %s\\%s failed."),
                      prnMergedResult->pcszFolder,
                      pcri->pcszName));
   }
   else
      tr = TR_SUCCESS;

   return((tr == TR_SUCCESS) ? TranslateHRESULTToTWINRESULT(hr)
                             : tr);
}


/*
** DealWithDelete()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT DealWithDelete(PCRECITEM pcri, RECSTATUSPROC rsp,
                                       LPARAM lpCallbackData)
{
   TWINRESULT tr;
   RECSTATUSUPDATE rsu;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));
   ASSERT(! rsp ||
          IS_VALID_CODE_PTR(rsp, RECSTATUSPROC));

   rsu.ulScale = CountRECNODEs(pcri, RNA_DELETE_ME);
   ASSERT(rsu.ulScale > 0);
   rsu.ulProgress = 0;

   if (NotifyReconciliationStatus(rsp, RS_BEGIN_DELETE, (LPARAM)&rsu,
                                  lpCallbackData))
   {
      PRECNODE prn;

      tr = TR_SUCCESS;

      for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
      {
         if (prn->rnaction == RNA_DELETE_ME)
         {
            TCHAR rgchPath[MAX_PATH_LEN];

            ComposePath(rgchPath, prn->pcszFolder, prn->priParent->pcszName);
            ASSERT(lstrlen(rgchPath) < ARRAYSIZE(rgchPath));

            if (MyIsPathOnVolume(rgchPath, (HPATH)(prn->hvid)))
            {
               if (DeleteFile(rgchPath))
                  WARNING_OUT((TEXT("DealWithDelete(): Deleted file %s."),
                               rgchPath));
               else
               {
                  tr = TR_DEST_OPEN_FAILED;

                  WARNING_OUT((TEXT("DealWithDelete(): Failed to delete file %s."),
                               rgchPath));
               }
            }
            else
               tr = TR_UNAVAILABLE_VOLUME;
         }
      }

      if (tr == TR_SUCCESS)
      {
         /* 100% complete. */

         rsu.ulProgress = rsu.ulScale;

         /* Don't allow abort. */

         NotifyReconciliationStatus(rsp, RS_END_DELETE, (LPARAM)&rsu,
                                    lpCallbackData);
      }
   }
   else
      tr = TR_ABORT;

   return(tr);
}


/*
** CountRECNODEs()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE ULONG CountRECNODEs(PCRECITEM pcri, RECNODEACTION rnaction)
{
   ULONG ulc = 0;
   PRECNODE prn;

   for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
   {
      if (prn->rnaction == rnaction)
      {
         ASSERT(ulc < ULONG_MAX);
         ulc++;
      }
   }

   return(ulc);
}


/*
** UpdateRecNodeFileStamps()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT UpdateRecNodeFileStamps(PCRECITEM pcri)
{
   TWINRESULT tr;
   PRECNODE prn;

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));

   tr = TR_SUCCESS;

   for (prn = pcri->prnFirst; prn; prn = prn->prnNext)
   {
      ASSERT(IS_VALID_HANDLE(prn->hObjectTwin, OBJECTTWIN));

      /* Was the RECNODE supposed to be reconciled? */

      /*
       * BUGBUG: We should avoid updating file stamps of copy sources here in
       * the SimpleCopy() case.
       */

      if (prn->rnaction != RNA_NOTHING)
      {
         ASSERT(prn->fsCurrent.fscond != FS_COND_UNAVAILABLE);

         /* Leave prn->fsLast as pot->fsLastRec here. */

         MyGetFileStampByHPATH(((PCOBJECTTWIN)(prn->hObjectTwin))->hpath,
               GetString(((PCOBJECTTWIN)(prn->hObjectTwin))->ptfParent->hsName),
               &(prn->fsCurrent));
      }
   }

   return(tr);
}


/*
** DeletedTwinsInRecItem()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL DeletedTwinsInRecItem(PCRECITEM pcri)
{
   BOOL bResult = TRUE;

   ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));

   /* Has the associated twin family been deleted? */

   if (IsStubFlagClear(&(((PTWINFAMILY)(pcri->hTwinFamily))->stub), STUB_FL_UNLINKED))
   {
      PRECNODE prn;

      /* No.  Have any of the associated object twins been deleted? */

      for (prn = pcri->prnFirst;
           prn && IsStubFlagClear(&(((PCOBJECTTWIN)(prn->hObjectTwin))->stub), STUB_FL_UNLINKED);
           prn = prn->prnNext)
         ;

      if (! prn)
         bResult = FALSE;
   }

   return(bResult);
}


/****************************** Public Functions *****************************/


/*
** CopyFileStampFromFindData()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void CopyFileStampFromFindData(PCWIN32_FIND_DATA pcwfdSrc,
                                           PFILESTAMP pfsDest)
{
   ASSERT(IS_VALID_READ_PTR(pcwfdSrc, CWIN32_FIND_DATA));
   ASSERT(IS_VALID_WRITE_PTR(pfsDest, FILESTAMP));

   pfsDest->dwcbHighLength = pcwfdSrc->nFileSizeHigh;
   pfsDest->dwcbLowLength = pcwfdSrc->nFileSizeLow;

   /* Convert to local time and save that too */

   if ( !FileTimeToLocalFileTime(&pcwfdSrc->ftLastWriteTime, &pfsDest->ftModLocal) )
   {
      /* Just copy the time if FileTimeToLocalFileTime failed */

      pfsDest->ftModLocal = pcwfdSrc->ftLastWriteTime;
   }
   pfsDest->ftMod = pcwfdSrc->ftLastWriteTime;
   pfsDest->fscond = FS_COND_EXISTS;

   ASSERT(IS_VALID_STRUCT_PTR(pfsDest, CFILESTAMP));

   return;
}


/*
** MyGetFileStamp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void MyGetFileStamp(LPCTSTR pcszFile, PFILESTAMP pfs)
{
   WIN32_FIND_DATA wfd;
   HANDLE hff;

   ASSERT(IS_VALID_STRING_PTR(pcszFile, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pfs, FILESTAMP));

   ZeroMemory(pfs, sizeof(*pfs));

   hff = FindFirstFile(pcszFile, &wfd);

   if (hff != INVALID_HANDLE_VALUE)
   {
      if (! IS_ATTR_DIR(wfd.dwFileAttributes))
         CopyFileStampFromFindData(&wfd, pfs);
      else
         pfs->fscond = FS_COND_EXISTS;

      EVAL(FindClose(hff));
   }
   else
      pfs->fscond = FS_COND_DOES_NOT_EXIST;

   ASSERT(IS_VALID_STRUCT_PTR(pfs, CFILESTAMP));

   return;
}


/*
** MyGetFileStampByHPATH()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:
*/
PUBLIC_CODE void MyGetFileStampByHPATH(HPATH hpath, LPCTSTR pcszSubPath,
                                       PFILESTAMP pfs)
{
   ASSERT(IS_VALID_HANDLE(hpath, PATH));
   ASSERT(! pcszSubPath ||
          IS_VALID_STRING_PTR(pcszSubPath, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pfs, FILESTAMP));

   if (IsPathVolumeAvailable(hpath))
   {
      TCHAR rgchPath[MAX_PATH_LEN];

      /* The root of the file's path is accessible. */

      GetPathString(hpath, rgchPath);
      if (pcszSubPath)
         CatPath(rgchPath, pcszSubPath);
      ASSERT(lstrlen(rgchPath) < ARRAYSIZE(rgchPath));

      MyGetFileStamp(rgchPath, pfs);
   }
   else
   {
      ZeroMemory(pfs, sizeof(*pfs));
      pfs->fscond = FS_COND_UNAVAILABLE;
   }

   ASSERT(IS_VALID_STRUCT_PTR(pfs, CFILESTAMP));

   return;
}


/*
** MyCompareFileStamps()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** Any FS_COND_UNAVAILABLE == any FS_COND_UNAVAILABLE.
** Any FS_COND_UNAVAILABLE < any FS_COND_DOES_NOT_EXIST.
** Any FS_COND_DOES_NOT_EXIST == any FS_COND_DOES_NOT_EXIST.
** Any FS_COND_DOES_NOT_EXIST < any FS_COND_EXISTS.
** Two FS_COND_EXISTS are compared by date and time.
**
** Hack Warning: This function depends upon the constant values of
** FS_COND_UNAVAILABLE, FS_COND_DOES_NOT_EXIST, and FS_COND_EXISTS being in
** increasing order, i.e.,
**
** FS_COND_UNAVAILABLE < FS_COND_DOES_NOT_EXIST < FS_COND_EXISTS
*/
PUBLIC_CODE COMPARISONRESULT MyCompareFileStamps(PCFILESTAMP pcfs1, PCFILESTAMP pcfs2)
{
   int nResult;

   ASSERT(IS_VALID_STRUCT_PTR(pcfs1, CFILESTAMP));
   ASSERT(IS_VALID_STRUCT_PTR(pcfs2, CFILESTAMP));

   nResult = (int)(pcfs1->fscond - pcfs2->fscond);

   if (! nResult && pcfs1->fscond == FS_COND_EXISTS)
   {
      /* File times are stored as UTC times.  However, files on FAT
      ** file systems only store the local time.  This means the UTC
      ** is derived from the local time, and fudged depending on the
      ** current timezone info.  This means that the UTC time will
      ** differ between timezone changes.
      **
      ** For remote files, the time's derivation depends on the server.
      ** NTFS servers provide the absolute UTC time, regardless of timezone.
      ** These are the best.  Likewise, NWServer keeps track of the 
      ** timezone and puts the UTC time on the wire like NTFS.  FAT
      ** systems convert the local time to UTC time based on the server's
      ** timezone, and places the UTC time on the wire.  Netware 3.31
      ** and some SMB servers put the local time on the wire and have
      ** the client convert to UTC time, so it uses the client's timezone.
      **
      ** One way to cover most of the holes that occur due to timezone
      ** changes is store both the UTC time and the local time.  If either
      ** are the same, then the file has not changed.
      */

      BOOL bModEqual = (pcfs1->ftMod.dwHighDateTime == pcfs2->ftMod.dwHighDateTime);
      BOOL bModLocalEqual = (pcfs1->ftModLocal.dwHighDateTime == pcfs2->ftModLocal.dwHighDateTime);

      if (bModEqual || bModLocalEqual)
      {
         if (bModEqual && pcfs1->ftMod.dwLowDateTime == pcfs2->ftMod.dwLowDateTime ||
            bModLocalEqual && pcfs1->ftModLocal.dwLowDateTime == pcfs2->ftModLocal.dwLowDateTime)
         {
            if (pcfs1->dwcbHighLength == pcfs2->dwcbHighLength)
            {
               if (pcfs1->dwcbLowLength == pcfs2->dwcbLowLength)
                  nResult = CR_EQUAL;
               else if (pcfs1->dwcbLowLength < pcfs2->dwcbLowLength)
                  nResult = CR_FIRST_SMALLER;
               else
                  nResult = CR_FIRST_LARGER;
            }
            else if (pcfs1->dwcbHighLength < pcfs2->dwcbHighLength)
               nResult = CR_FIRST_SMALLER;
            else
               nResult = CR_FIRST_LARGER;
         }
         else if (pcfs1->ftMod.dwLowDateTime < pcfs2->ftMod.dwLowDateTime)
            nResult = CR_FIRST_SMALLER;
         else
            nResult = CR_FIRST_LARGER;
      }
      else if (pcfs1->ftMod.dwHighDateTime < pcfs2->ftMod.dwHighDateTime)
         nResult = CR_FIRST_SMALLER;
      else
         nResult = CR_FIRST_LARGER;
   }

   return(MapIntToComparisonResult(nResult));
}


/***************************** Exported Functions ****************************/

/* RAIDRAID: (16205) AutoDoc RECSTATUSPROC messages below. */

/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | ReconcileItem | Reconciles a reconciliation item created by
CreateRecList().

@parm PCRECITEM | pcri | A pointer to a reconciliation item to be reconciled.

@parm RECSTATUSPROC | rsp | A procedure instance address of a callback function
to be called with status information during the reconciliation of the given
RECITEM.  rsp may be NULL to indicate that no reconciliation status callback
function is to be called.  (See the reconciliation handler SPI documentation
for details.)

@parm LPARAM | lpCallbackData | Callback data to be supplied to the
reconciliation status callback function.  If rsp is NULL, lpCallbackData is
ignored.

@parm DWORD | dwFlags | A bit mask of flags.  This parameter may be any
combination of the following values:
   RI_FL_ALLOW_UI - Allow interaction with the user during reconciliation.
   RI_FL_FEEDBACK_WINDOW_VALID - hwndProgressFeedback is valid, and may be used
   to communicate reconciliation progress information to the user during
   reconciliation.

@parm HWND | hwndOwner | A handle to the parent window to be used when
requesting user interaction.  This parameter is ignored if the RI_FL_ALLOW_UI
flag is clear.

@parm HWND | hwndProgressFeedback | A handle to the window to be used to
provide progress information to the user during reconciliation.  This parameter
is ignored if the RI_FL_FEEDBACK_WINDOW_VALID flag is clear.

@rdesc If the reconciliation item was reconciled successfully, TR_SUCCESS is
returned.  Otherwise, the reconciliation item was not reconciled successfully,
and the return value indicates the error that occurred.

@comm All the fields in the given RECITEM and its child structures are left
unchanged by ReconcileItem(), except for the fsCurrent fields of RECNODEs
associated with objects that are overwritten during reconciliation.  The
fsCurrent field of each RECNODE associated with an object that is overwritten
during reconciliation (i.e., RECNODEs with rnaction set to RNA_COPY_TO_ME or
RNA_MERGE_ME) is updated to reflect the current time stamp of the object after
it is overwritten.  If ReconcileItem() returns TR_SUCCESS, all the available
RECNODEs (i.e., all RECNODEs whose uState field is not RNS_UNAVAILABLE) in the
RECITEM may be assumed to be up-to-date.  If ReconcileItem() does not return
TR_SUCCESS, no assumption may be made about the states of the RECNODEs in the
RECITEM.  If ReconcileItem() is called on a RECITEM that references a twin
family that has been deleted or one or more object twins that have been
deleted, TR_DELETED_TWIN is returned.  In this case, no assumption may be made
about what reconciliation actions have been carried out on the RECITEM.  If
TR_DELETED_TWIN is returned, the client may attempt to create a RECLIST for the
twin family associated with the RECITEM in order to retry the reconciliation
operation.  (The client would call MarkTwin(), followed by CreateRecList().) If
TR_DELETED_TWIN is returned by MarkTwin(), the entire twin family has been
deleted.  If TR_SUCCESS is returned by MarkTwin(), the client should be able to
call CreateRecList() to create a RECLIST containing a more up-to-date RECITEM
for the twin family.

@xref CreateRecList

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI ReconcileItem(PCRECITEM pcri, RECSTATUSPROC rsp,
                                           LPARAM lpCallbackData,
                                           DWORD dwFlags, HWND hwndOwner,
                                           HWND hwndProgressFeedback)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(ReconcileItem);

#ifdef EXPV
      /* Verify parameters. */

      /* lpCallbackData may be any value. */

      if (IS_VALID_STRUCT_PTR(pcri, CRECITEM) &&
          (! rsp ||
           IS_VALID_CODE_PTR(rsp, RECSTATUSPROC)) &&
          FLAGS_ARE_VALID(dwFlags, ALL_RI_FLAGS) &&
          (IS_FLAG_CLEAR(dwFlags, RI_FL_ALLOW_UI) ||
           IS_VALID_HANDLE(hwndOwner, WND)) &&
          (IS_FLAG_CLEAR(dwFlags, RI_FL_FEEDBACK_WINDOW_VALID) ||
           IS_VALID_HANDLE(hwndProgressFeedback, WND)))
#endif
      {
         /* Check for any deleted twins referenced by this RECITEM. */

         if (! DeletedTwinsInRecItem(pcri))
         {
            InvalidatePathListInfo(GetBriefcasePathList(((PCTWINFAMILY)(pcri->hTwinFamily))->hbr));

            tr = MyReconcileItem(pcri, rsp, lpCallbackData, dwFlags,
                                 hwndOwner, hwndProgressFeedback);

            ASSERT(IS_VALID_STRUCT_PTR(pcri, CRECITEM));
         }
         else
            tr = TR_DELETED_TWIN;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(ReconcileItem, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | BeginReconciliation | Indicates to the synchronization engine
that the caller is about to make multiple calls to ReconcileItem().

@parm HBRFCASE | hbr | A handle to the open briefcase about to be reconciled.

@rdesc If reconciliation for the given briefcase was initialized successfully,
TR_SUCCESS is returned.  Otherwise, reconciliation for the given briefcase was
not initialized successfully, and the return value indicates the error that
occurred.

@comm Synchronization engine clients need not call BeginReconciliation() before
calling ReconcileItem().  BeginReconciliation() is simply provided to allow
synchronization engine clients to give the synchronization engine a hint that
multiple calls to ReconcileItem() are about to occur.  Each call to
EndReconciliation() should be followed by a call to EndReconciliation().

@xref EndReconciliation ReconcileItem

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI BeginReconciliation(HBRFCASE hbr)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(BeginReconciliation);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbr, BRFCASE))
#endif
      {
         BeginCopy();
         BeginMerge();

         tr = TR_SUCCESS;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(BeginReconciliation, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | EndReconciliation | Indicates to the synchronization engine
that the client has finished making multiple calls to ReconcileItem(),
preceded by a call to BeginReconciliation().

@parm HBRFCASE | hbr | A handle to the open briefcase whose reconciliation has
been completed.

@rdesc If reconciliation for the given briefcase was terminaterd successfully,
TR_SUCCESS is returned.  Otherwise, reconciliation for the given briefcase was
not terminated successfully, and the return value indicates the error that
occurred.

@comm EndReconciliation() should only be called after a call to
BeginReconciliation().  Each call to BeginReconciliation() should be followed
by a matching call to EndReconciliation().

@xref BeginReconciliation ReconcileItem

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI EndReconciliation(HBRFCASE hbr)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(EndReconciliation);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbr, BRFCASE))
#endif
      {
         EndMerge();
         EndCopy();

         tr = TR_SUCCESS;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(EndReconciliation, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | GetFileStamp | Retrieves the file stamp for an open file.

@parm PCSTR | pcszFile | A pointer to a string indicating the file whose file
stamp is to be retrieved.

@parm PFILESTAMP | pcr | A pointer to a FILESTAMP to be filled in with the
file stamp of the given open file.

@rdesc If the comparison was successful, TR_SUCCESS is returned.  Otherwise,
the comparison was not successful, and the return value indicates the error
that occurred.

@xref CompareFileStamps

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI GetFileStamp(LPCTSTR pcszFile, PFILESTAMP pfs)
{
   TWINRESULT tr;

   /* No need for exclusive access here. */

   DebugEntry(GetFileStamp);

#ifdef EXPV
   /* Verify parameters. */

   if (IS_VALID_STRING_PTR(pcszFile, CSTR) &&
       IS_VALID_WRITE_PTR(pfs, FILESTAMP))
#endif
   {
      MyGetFileStamp(pcszFile, pfs);

      tr = TR_SUCCESS;
   }
#ifdef EXPV
   else
      tr = TR_INVALID_PARAMETER;
#endif

   DebugExitTWINRESULT(GetFileStamp, tr);

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api COMPARISONRESULT | CompareFileStamps | Compares two file stamps.

@parm PCFILESTAMP | pcfs1 | A pointer to the first FILESTAMP to be compared.

@parm PCFILESTAMP | pcfs2 | A pointer to the second FILESTAMP to be compared.

@parm PCOMPARISONRESULT | pcr | A pointer to a COMPARISONRESULT to be filled in
with the result of the file stamp comparison.  *pcr is only valid if TR_SUCCESS
is returned.

@rdesc If the file stamp was retrieved successfully, TR_SUCCESS is returned.
Otherwise, the file stamp was not retrieved successfully, and the return value
indicates the error that occurred.

@comm File stamps are compared by fields as follows:
1) condition
   Any FS_COND_UNAVAILABLE equals any FS_COND_UNAVAILABLE.
   Any FS_COND_UNAVAILABLE is less than any FS_COND_DOES_NOT_EXIST.
   Any FS_COND_DOES_NOT_EXIST equals any FS_COND_DOES_NOT_EXIST.
   Any FS_COND_DOES_NOT_EXIST is less than any FS_COND_EXISTS.
   Two FS_COND_EXISTS are compared by date and time.
2) date and time of last modification
3) length

@xref GetFileStamp

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI CompareFileStamps(PCFILESTAMP pcfs1,
                                               PCFILESTAMP pcfs2,
                                               PCOMPARISONRESULT pcr)
{
   TWINRESULT tr;

   /* No need for exclusive access here. */

   DebugEntry(CompareFileStamps);

#ifdef EXPV
   /* Verify parameters. */

   if (IS_VALID_STRUCT_PTR(pcfs1, CFILESTAMP) &&
       IS_VALID_STRUCT_PTR(pcfs2, CFILESTAMP) &&
       IS_VALID_WRITE_PTR(pcr, COMPARISONRESULT))
#endif
   {
      *pcr = MyCompareFileStamps(pcfs1, pcfs2);
      tr = TR_SUCCESS;
   }
#ifdef EXPV
   else
      tr = TR_INVALID_PARAMETER;
#endif

   DebugExitTWINRESULT(CompareFileStamps, tr);

   return(tr);
}

