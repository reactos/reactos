/*
 * copy.c - File copy handler module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "stub.h"
#include "oleutil.h"


/* Constants
 ************/

/* size of file copy buffer in bytes */

#define COPY_BUF_SIZE               (64 * 1024)


/* Module Variables
 *******************/

#pragma data_seg(DATA_SEG_SHARED)

/* lock count for file copy buffer */

PRIVATE_DATA ULONG MulcCopyBufLock = 0;

/* buffer for file copying */

PRIVATE_DATA PBYTE MpbyteCopyBuf = NULL;

/* length of file copy buffer in bytes */

PRIVATE_DATA UINT MucbCopyBufLen = 0;

#pragma data_seg()


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE TWINRESULT SimpleCopy(PRECNODE, RECSTATUSPROC, LPARAM);
PRIVATE_CODE TWINRESULT CreateDestinationFolders(PCRECNODE);
PRIVATE_CODE TWINRESULT CreateCopyBuffer(void);
PRIVATE_CODE void DestroyCopyBuffer(void);
PRIVATE_CODE TWINRESULT CopyFileByHandle(HANDLE, HANDLE, RECSTATUSPROC, LPARAM, ULONG, PULONG);
PRIVATE_CODE TWINRESULT CopyFileByName(PCRECNODE, PRECNODE, RECSTATUSPROC, LPARAM, ULONG, PULONG);
PRIVATE_CODE ULONG DetermineCopyScale(PCRECNODE);
PRIVATE_CODE BOOL IsCopyDestination(PCRECNODE);
PRIVATE_CODE BOOL SetDestinationTimeStamps(PCRECNODE);
PRIVATE_CODE BOOL DeleteFolderProc(LPCTSTR, PCWIN32_FIND_DATA, PVOID);

#ifdef DEBUG

PRIVATE_CODE BOOL CopyBufferIsOk(void);
PRIVATE_CODE BOOL VerifyRECITEMAndSrcRECNODE(PCRECNODE);

#endif


/*
** SimpleCopy()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT SimpleCopy(PRECNODE prnSrc, RECSTATUSPROC rsp,
                              LPARAM lpCallbackData)
{
   TWINRESULT tr;
   ULONG ulScale;
   PRECNODE prnDest;
   ULONG ulCurrent = 0;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(prnSrc, CRECNODE));
   ASSERT(! rsp ||
          IS_VALID_CODE_PTR(rsp, RECSTATUSPROC));

   ulScale = DetermineCopyScale(prnSrc);

   /* Copy the source file to each destination file. */

   tr = TR_SUCCESS;

   BeginCopy();

   for (prnDest = prnSrc->priParent->prnFirst;
        prnDest;
        prnDest = prnDest->prnNext)
   {
      if (prnDest != prnSrc)
      {
         if (IsCopyDestination(prnDest))
         {
            tr = CopyFileByName(prnSrc, prnDest, rsp, lpCallbackData,
                                ulScale, &ulCurrent);

            if (tr != TR_SUCCESS)
               break;

            ASSERT(ulCurrent <= ulScale);
         }
      }
   }

   EndCopy();

   return(tr);
}


/*
** CreateDestinationFolders()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT CreateDestinationFolders(PCRECNODE pcrnSrc)
{
   TWINRESULT tr = TR_SUCCESS;
   PCRECNODE pcrnDest;

   for (pcrnDest = pcrnSrc->priParent->prnFirst;
        pcrnDest;
        pcrnDest = pcrnDest->prnNext)
   {
      if (pcrnDest->rnaction == RNA_COPY_TO_ME)
      {
         tr = CreateFolders(pcrnDest->pcszFolder,
                            ((PCOBJECTTWIN)(pcrnDest->hObjectTwin))->hpath);

         if (tr != TR_SUCCESS)
            break;
      }
   }

   return(tr);
}


/*
** CreateCopyBuffer()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT CreateCopyBuffer(void)
{
   TWINRESULT tr;

   ASSERT(CopyBufferIsOk());

   /* Has the copy buffer already been allocated? */

   if (MpbyteCopyBuf)
      /* Yes. */
      tr = TR_SUCCESS;
   else
   {
      /* No.  Allocate it. */

      if (AllocateMemory(COPY_BUF_SIZE, &MpbyteCopyBuf))
      {
         MucbCopyBufLen = COPY_BUF_SIZE;
         tr = TR_SUCCESS;

         TRACE_OUT((TEXT("CreateCopyBuffer(): %u byte file copy buffer allocated."),
                    MucbCopyBufLen));
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }

   ASSERT(CopyBufferIsOk());

   return(tr);
}


/*
** DestroyCopyBuffer()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroyCopyBuffer(void)
{
   ASSERT(CopyBufferIsOk());

   /* Has the copy buffer already been allocated? */

   if (MpbyteCopyBuf)
   {
      /* Yes.  Free it. */

      FreeMemory(MpbyteCopyBuf);
      MpbyteCopyBuf = NULL;
      TRACE_OUT((TEXT("DestroyCopyBuffer(): %u byte file copy buffer freed."),
                 MucbCopyBufLen));
      MucbCopyBufLen = 0;
   }

   ASSERT(CopyBufferIsOk());

   return;
}


/*
** CopyFileByHandle()
**
** Copies one file to another.
**
** Arguments:     hfSrc - file handle to open source file
**                hfDest - file handle to open destination file
**
** Returns:       TWINRESULT
**
** Side Effects:  Leaves the file pointer of each file at the end of the file.
*/
PRIVATE_CODE TWINRESULT CopyFileByHandle(HANDLE hfSrc, HANDLE hfDest,
                                    RECSTATUSPROC rsp, LPARAM lpCallbackData,
                                    ULONG ulScale, PULONG pulcbTotal)
{
   TWINRESULT tr;

   /* lpCallbackData may be any value. */
   /* ulScale may be any value. */

   ASSERT(IS_VALID_HANDLE(hfSrc, FILE));
   ASSERT(IS_VALID_HANDLE(hfDest, FILE));
   ASSERT(! rsp ||
          IS_VALID_CODE_PTR(rsp, RECSTATUSROC));
   ASSERT(IS_VALID_WRITE_PTR(pulcbTotal, ULONG));

   /* Make sure the copy buffer has been created. */

   tr = CreateCopyBuffer();

   if (tr == TR_SUCCESS)
   {
      BeginCopy();

      /* Move to the beginning of the files. */

      if (SetFilePointer(hfSrc, 0, NULL, FILE_BEGIN) != INVALID_SEEK_POSITION)
      {
         if (SetFilePointer(hfDest, 0, NULL, FILE_BEGIN) != INVALID_SEEK_POSITION)
         {
            do
            {
               DWORD dwcbRead;

               if (ReadFile(hfSrc, MpbyteCopyBuf, MucbCopyBufLen, &dwcbRead,
                            NULL))
               {
                  if (dwcbRead)
                  {
                     DWORD dwcbWritten;

                     if (WriteFile(hfDest, MpbyteCopyBuf, dwcbRead,
                                   &dwcbWritten, NULL) &&
                         dwcbWritten == dwcbRead)
                     {
                        RECSTATUSUPDATE rsu;

                        ASSERT(*pulcbTotal <= ULONG_MAX - dwcbRead);

                        *pulcbTotal += dwcbRead;

                        rsu.ulProgress = *pulcbTotal;
                        rsu.ulScale = ulScale;

                        if (! NotifyReconciliationStatus(rsp, RS_DELTA_COPY,
                                                         (LPARAM)&rsu,
                                                         lpCallbackData))
                           tr = TR_ABORT;
                     }
                     else
                        tr = TR_DEST_WRITE_FAILED;
                  }
                  else
                     /* Hit EOF.  Stop. */
                     break;
               }
               else
                  tr = TR_SRC_READ_FAILED;
            } while (tr == TR_SUCCESS);
         }
         else
            tr = TR_DEST_WRITE_FAILED;
      }
      else
         tr = TR_SRC_READ_FAILED;

      EndCopy();
   }

   return(tr);
}

// MakeAnsiPath
//
// Copys path pszIn to pszOut, ensuring that pszOut has a valid ANSI mapping

void MakeAnsiPath(LPTSTR pszIn, LPTSTR pszOut)
{
#ifdef UNICODE
    CHAR szAnsi[MAX_PATH];
    pszOut[0] = L'\0';

    WideCharToMultiByte(CP_ACP, 0, pszIn, -1, szAnsi, ARRAYSIZE(szAnsi), NULL, NULL);
    MultiByteToWideChar(CP_ACP, 0, szAnsi,   -1, pszOut, MAX_PATH);
    if (lstrcmp(pszOut, pszIn))
    {
        // Cannot convert losslessly from Unicode -> Ansi, so get the short path

        lstrcpyn(pszOut, pszIn, MAX_PATH);
        SheShortenPath(pszOut, TRUE);
    }
#else
    lstrcpyn(pszOut, pszIn, MAX_PATH);
#endif
}

/*
** CopyFileByName()
**
** Copies one file over another.
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  Copies source file's time stamp to destination file.
*/
PRIVATE_CODE TWINRESULT CopyFileByName(PCRECNODE pcrnSrc, PRECNODE prnDest,
                                  RECSTATUSPROC rsp, LPARAM lpCallbackData,
                                  ULONG ulScale, PULONG pulcbTotal)
{
   TWINRESULT tr;
   TCHAR rgchSrcPath[MAX_PATH_LEN];
   TCHAR rgchDestPath[MAX_PATH_LEN];

   /* lpCallbackData may be any value. */
   /* ulScale may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pcrnSrc, CRECNODE));
   ASSERT(IS_VALID_STRUCT_PTR(prnDest, CRECNODE));
   ASSERT(! rsp ||
          IS_VALID_CODE_PTR(rsp, RECSTATUSROC));
   ASSERT(IS_VALID_WRITE_PTR(pulcbTotal, ULONG));

   /* Create source path string. */

   ComposePath(rgchSrcPath, pcrnSrc->pcszFolder, pcrnSrc->priParent->pcszName);
   ASSERT(lstrlen(rgchSrcPath) < ARRAYSIZE(rgchSrcPath));

   /* Create destination path string. */

   ComposePath(rgchDestPath, prnDest->pcszFolder, prnDest->priParent->pcszName);
   ASSERT(lstrlen(rgchDestPath) < ARRAYSIZE(rgchDestPath));

   /* Check volumes. */

   if (MyIsPathOnVolume(rgchSrcPath, (HPATH)(pcrnSrc->hvid)) &&
       MyIsPathOnVolume(rgchDestPath, (HPATH)(prnDest->hvid)))
   {
      FILESTAMP fsSrc;
      FILESTAMP fsDest;

      /* Compare current file stamps with recorded file stamps. */

      MyGetFileStamp(rgchSrcPath, &fsSrc);
      MyGetFileStamp(rgchDestPath, &fsDest);

      if (! MyCompareFileStamps(&(pcrnSrc->fsCurrent), &fsSrc) &&
          ! MyCompareFileStamps(&(prnDest->fsCurrent), &fsDest))
      {
         HANDLE hfSrc;

         /* Open source file.  Assume source file will be read sequentially. */

         hfSrc = CreateFile(rgchSrcPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

         if (hfSrc != INVALID_HANDLE_VALUE)
         {
            HANDLE hfDest;

            /*
             * Create destination file.  Assume destination file will be
             * written sequentially.
             */

            TCHAR szAnsiPath[MAX_PATH];
            MakeAnsiPath(rgchDestPath, szAnsiPath);

            hfDest = CreateFile(szAnsiPath, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS,
                        (FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN),
                        NULL);

            if (hfDest != INVALID_HANDLE_VALUE)
            {
               /* Everything is cool.  Copy the file. */

               tr = CopyFileByHandle(hfSrc, hfDest, rsp,
                                     lpCallbackData, ulScale,
                                     pulcbTotal);

               if (tr == TR_SUCCESS)
               {
                  /*
                   * Set the destination file's time stamp to the source
                   * file's time stamp to assist clients that don't maintain
                   * a persistent briefcase database, like MPR.  Failure to
                   * set the time stamp is not fatal.
                   */

                  if (! SetFileTime(hfDest, NULL, NULL,
                                    &(pcrnSrc->fsCurrent.ftMod)))
                     WARNING_OUT((TEXT("CopyFileByName(): Failed to set last modification time stamp of destination file %s to match source file %s."),
                                  rgchDestPath,
                                  rgchSrcPath));
               }

               /* Failing to close the destination file is fatal here. */

               if (! CloseHandle(hfDest) && tr == TR_SUCCESS)
                  tr = TR_DEST_WRITE_FAILED;
            }
            else
               tr = TR_DEST_OPEN_FAILED;

            /* Failing to close the source file successfully is not fatal. */

            CloseHandle(hfSrc);
         }
         else
            tr = TR_SRC_OPEN_FAILED;
      }
      else
         tr = TR_FILE_CHANGED;
   }
   else
      tr = TR_UNAVAILABLE_VOLUME;

#ifdef DEBUG

   if (tr == TR_SUCCESS)
      TRACE_OUT((TEXT("CopyFileByName(): %s\\%s copied to %s\\%s."),
                 pcrnSrc->pcszFolder,
                 pcrnSrc->priParent->pcszName,
                 prnDest->pcszFolder,
                 prnDest->priParent->pcszName));

   else
      TRACE_OUT((TEXT("CopyFileByName(): Failed to copy %s\\%s to %s\\%s."),
                 pcrnSrc->pcszFolder,
                 pcrnSrc->priParent->pcszName,
                 prnDest->pcszFolder,
                 prnDest->priParent->pcszName));

#endif

   return(tr);
}


/*
** DetermineCopyScale()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE ULONG DetermineCopyScale(PCRECNODE pcrnSrc)
{
   DWORD dwcbSrcFileLen;
   PCRECNODE pcrn;
   ULONG ulScale = 0;

   ASSERT(IS_VALID_STRUCT_PTR(pcrnSrc, CRECNODE));

   /*
    * RAIDRAID: (16257) If anyone tries to copy more than 4 Gb of files, this
    * scaling calculation is broken.
    */

   ASSERT(! pcrnSrc->fsCurrent.dwcbHighLength);
   dwcbSrcFileLen = pcrnSrc->fsCurrent.dwcbLowLength;

   for (pcrn = pcrnSrc->priParent->prnFirst; pcrn; pcrn = pcrn->prnNext)
   {
      if (pcrn != pcrnSrc)
      {
         if (IsCopyDestination(pcrn))
         {
            ASSERT(ulScale < ULONG_MAX - dwcbSrcFileLen);
            ulScale += dwcbSrcFileLen;
         }
      }
   }

   TRACE_OUT((TEXT("DetermineCopyScale(): Scale for %s is %lu."),
              pcrnSrc->priParent->pcszName,
              ulScale));

   return(ulScale);
}


/*
** IsCopyDestination()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsCopyDestination(PCRECNODE pcrn)
{
   BOOL bDest = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(pcrn, CRECNODE));

   switch (pcrn->priParent->riaction)
   {
      case RIA_COPY:
         switch (pcrn->rnaction)
         {
            case RNA_COPY_TO_ME:
               bDest = TRUE;
               break;

            default:
               break;
         }
         break;

      case RIA_MERGE:
         switch (pcrn->rnaction)
         {
            case RNA_COPY_TO_ME:
            case RNA_MERGE_ME:
               bDest = TRUE;
               break;

            default:
               break;
         }
         break;

      default:
         ERROR_OUT((TEXT("IsCopyDestination(): Bad RECITEM action %d."),
                    pcrn->priParent->riaction));
         break;
   }

   return(bDest);
}


/*
** SetDestinationTimeStamps()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL SetDestinationTimeStamps(PCRECNODE pcrnSrc)
{
   BOOL bResult = TRUE;
   PCRECNODE pcrn;

   ASSERT(IS_VALID_STRUCT_PTR(pcrnSrc, CRECNODE));

   for (pcrn = pcrnSrc->priParent->prnFirst;
        pcrn;
        pcrn = pcrn->prnNext)
   {
      if (pcrn->rnaction == RNA_COPY_TO_ME)
      {
         TCHAR rgchPath[MAX_PATH_LEN];
         HANDLE hfDest;

         ComposePath(rgchPath, pcrn->pcszFolder, pcrn->priParent->pcszName);
         ASSERT(lstrlen(rgchPath) < ARRAYSIZE(rgchPath));

         hfDest = CreateFile(rgchPath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, NULL);

         if (hfDest != INVALID_HANDLE_VALUE)
         {
            if (! SetFileTime(hfDest, NULL, NULL, &(pcrnSrc->fsCurrent.ftMod)))
               bResult = FALSE;

            if (! CloseHandle(hfDest))
               bResult = FALSE;
         }
         else
            bResult = FALSE;

         if (bResult)
            TRACE_OUT((TEXT("SetDestinationTimeStamps(): Set last modification time stamp of %s to match last modification time stamp of %s\\%s."),
                       rgchPath,
                       pcrnSrc->pcszFolder,
                       pcrnSrc->priParent->pcszName));
         else
            WARNING_OUT((TEXT("SetDestinationTimeStamps(): Failed to set last modification time stamp of %s to match last modification time stamp of %s\\%s."),
                         rgchPath,
                         pcrnSrc->pcszFolder,
                         pcrnSrc->priParent->pcszName));
      }
   }

   return(bResult);
}


/*
** DeleteFolderProc()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL DeleteFolderProc(LPCTSTR pcszFolder, PCWIN32_FIND_DATA pcwfd,
                                   PVOID ptr)
{
   ASSERT(IsCanonicalPath(pcszFolder));
   ASSERT(IS_VALID_READ_PTR(pcwfd, CWIN32_FIND_DATA));
   ASSERT(IS_VALID_WRITE_PTR(ptr, TWINRESULT));

   if (IS_ATTR_DIR(pcwfd->dwFileAttributes))
   {
      TCHAR rgchPath[MAX_PATH_LEN];

      ComposePath(rgchPath, pcszFolder, pcwfd->cFileName);
      ASSERT(lstrlen(rgchPath) < ARRAYSIZE(rgchPath));

      if (RemoveDirectory(rgchPath))
      {
         WARNING_OUT((TEXT("DeleteFolderProc(): Removed folder %s."),
                      rgchPath));

         NotifyShell(rgchPath, NSE_DELETE_FOLDER);
      }
      else
      {
         WARNING_OUT((TEXT("DeleteFolderProc(): Failed to remove folder %s."),
                      rgchPath));

         *(PTWINRESULT)ptr = TR_DEST_WRITE_FAILED;
      }
   }
   else
      TRACE_OUT((TEXT("DeleteFolderProc(): Skipping file %s\\%s."),
                 pcszFolder,
                 pcwfd->cFileName));

   return(TRUE);
}


#ifdef DEBUG

/*
** CopyBufferIsOk()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CopyBufferIsOk(void)
{
   /* Are the module copy buffer variables in a correct state? */

   return((! MucbCopyBufLen &&
           ! MpbyteCopyBuf) ||
          (MucbCopyBufLen > 0 &&
           IS_VALID_WRITE_BUFFER_PTR(MpbyteCopyBuf, BYTE, MucbCopyBufLen)));
}


/*
** VerifyRECITEMAndSrcRECNODE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL VerifyRECITEMAndSrcRECNODE(PCRECNODE pcrnSrc)
{
   /* Do the RECITEM and source RECNODE actions match? */

   return((pcrnSrc->priParent->riaction == RIA_COPY &&
           pcrnSrc->rnaction == RNA_COPY_FROM_ME) ||
          (pcrnSrc->priParent->riaction == RIA_MERGE &&
           pcrnSrc->rnaction == RNA_MERGE_ME));
}

#endif


/****************************** Public Functions *****************************/


/*
** BeginCopy()
**
** Increments copy buffer lock count.
**
** Arguments:
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void BeginCopy(void)
{
   ASSERT(CopyBufferIsOk());

   ASSERT(MulcCopyBufLock < ULONG_MAX);
   MulcCopyBufLock++;

   ASSERT(CopyBufferIsOk());

   return;
}


/*
** EndCopy()
**
** Decrements copy buffer lock count.
**
** Arguments:
**
** Returns:       void
**
** Side Effects:  Frees copy buffer if lock count goes to 0.
*/
PUBLIC_CODE void EndCopy(void)
{
   ASSERT(CopyBufferIsOk());

   /* Is the copy buffer still locked? */

   if (! --MulcCopyBufLock)
      DestroyCopyBuffer();

   ASSERT(CopyBufferIsOk());

   return;
}


/*
** CopyHandler()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT CopyHandler(PRECNODE prnSrc, RECSTATUSPROC rsp,
                                   LPARAM lpCallbackData, DWORD dwFlags,
                                   HWND hwndOwner, HWND hwndProgressFeedback)
{
   TWINRESULT tr;
   RECSTATUSUPDATE rsu;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(prnSrc, CRECNODE));
   ASSERT(! rsp ||
          IS_VALID_CODE_PTR(rsp, RECSTATUSPROC));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_RI_FLAGS));
   ASSERT(IS_FLAG_CLEAR(dwFlags, RI_FL_ALLOW_UI) ||
          IS_VALID_HANDLE(hwndOwner, WND));
   ASSERT(IS_FLAG_CLEAR(dwFlags, RI_FL_FEEDBACK_WINDOW_VALID) ||
          IS_VALID_HANDLE(hwndProgressFeedback, WND));

   ASSERT(VerifyRECITEMAndSrcRECNODE(prnSrc));

   /* 0% complete. */

   rsu.ulScale = 1;
   rsu.ulProgress = 0;

   if (NotifyReconciliationStatus(rsp, RS_BEGIN_COPY, (LPARAM)&rsu,
                                  lpCallbackData))
   {
      tr = CreateDestinationFolders(prnSrc);

      if (tr == TR_SUCCESS)
      {
         TCHAR rgchPath[MAX_PATH_LEN];
         CLSID clsidReconciler;
         HRESULT hr;

         ComposePath(rgchPath, prnSrc->pcszFolder, prnSrc->priParent->pcszName);
         ASSERT(lstrlen(rgchPath) < ARRAYSIZE(rgchPath));

         if (SUCCEEDED(GetCopyHandlerClassID(rgchPath, &clsidReconciler)))
         {
            hr = OLECopy(prnSrc, &clsidReconciler, rsp, lpCallbackData,
                         dwFlags, hwndOwner, hwndProgressFeedback);

            if (SUCCEEDED(hr))
            {
               if (hr != S_FALSE)
               {
                  /*
                   * Set the destination files' time stamps to the source
                   * file's time stamp to assist clients that don't maintain
                   * a persistent briefcase database, like MPR.  Failure to
                   * set the time stamps is not fatal.
                   */

                  ASSERT(hr == REC_S_IDIDTHEUPDATES);
                  TRACE_OUT((TEXT("CopyHandler(): OLECopy() on %s returned %s."),
                             rgchPath,
                             GetHRESULTString(hr)));

                  if (! SetDestinationTimeStamps(prnSrc))
                     WARNING_OUT((TEXT("CopyHandler(): SetDestinationTimeStamps() failed.  Not all destination files have been marked with source file's time stamp.")));

                  tr = TR_SUCCESS;
               }
               else
               {
                  WARNING_OUT((TEXT("CopyHandler(): OLECopy() on %s returned %s.  Resorting to internal copy routine."),
                               rgchPath,
                               GetHRESULTString(hr)));

                  /*
                   * Update the source RECNODE's file stamp in case it was
                   * changed by the reconciler.
                   */

                  MyGetFileStampByHPATH(((PCOBJECTTWIN)(prnSrc->hObjectTwin))->hpath,
                     GetString(((PCOBJECTTWIN)(prnSrc->hObjectTwin))->ptfParent->hsName),
                     &(prnSrc->fsCurrent));

                  tr = SimpleCopy(prnSrc, rsp, lpCallbackData);
               }
            }
            else
               tr = TranslateHRESULTToTWINRESULT(hr);
         }
         else
            tr = SimpleCopy(prnSrc, rsp, lpCallbackData);

         if (tr == TR_SUCCESS)
         {
            /* 100% complete. */

            rsu.ulScale = 1;
            rsu.ulProgress = 1;

            /* Don't allow abort here. */

            NotifyReconciliationStatus(rsp, RS_END_COPY, (LPARAM)&rsu,
                                       lpCallbackData);
         }
      }
   }
   else
      tr = TR_ABORT;

   return(tr);
}


/*
** NotifyReconciliationStatus()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL NotifyReconciliationStatus(RECSTATUSPROC rsp, UINT uMsg, LPARAM lp,
                                       LPARAM lpCallbackData)
{
   BOOL bContinue;

   /* lp may be any value. */
   /* lpCallbackData may be any value. */

   ASSERT(! rsp ||
          IS_VALID_CODE_PTR(rsp, RECSTATUSROC));
   ASSERT(IsValidRecStatusProcMsg(uMsg));

   if (rsp)
   {
      TRACE_OUT((TEXT("NotifyReconciliationStatus(): Calling RECSTATUSPROC with message %s, ulProgress %lu, ulScale %lu, callback data %#lx."),
                 GetRECSTATUSPROCMSGString(uMsg),
                 ((PCRECSTATUSUPDATE)lp)->ulProgress,
                 ((PCRECSTATUSUPDATE)lp)->ulScale,
                 lpCallbackData));

      bContinue = (*rsp)(uMsg, lp, lpCallbackData);
   }
   else
   {
      TRACE_OUT((TEXT("NotifyReconciliationStatus(): Not calling NULL RECSTATUSPROC with message %s, ulProgress %lu, ulScale %lu, callback data %#lx."),
                 GetRECSTATUSPROCMSGString(uMsg),
                 ((PCRECSTATUSUPDATE)lp)->ulProgress,
                 ((PCRECSTATUSUPDATE)lp)->ulScale,
                 lpCallbackData));

      bContinue = TRUE;
   }

   if (! bContinue)
      WARNING_OUT((TEXT("NotifyReconciliationStatus(): Client callback aborted reconciliation.")));

   return(bContinue);
}


/*
** CreateFolders()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT CreateFolders(LPCTSTR pcszPath, HPATH hpath)
{
   TWINRESULT tr;

   ASSERT(IsCanonicalPath(pcszPath));
   ASSERT(IS_VALID_HANDLE(hpath, PATH));

   if (MyIsPathOnVolume(pcszPath, hpath))
   {
      TCHAR rgchPath[MAX_PATH_LEN];
      LPTSTR pszRootEnd;
      LPTSTR pszHackSlash;

      /* Create working copy of path. */

      ASSERT(lstrlen(pcszPath) < ARRAYSIZE(rgchPath));
      lstrcpy(rgchPath, pcszPath);

      pszRootEnd = FindEndOfRootSpec(rgchPath, hpath);

      /*
       * Hack off the path at each successive slash, and check to see if that
       * folder needs to be created.
       */

      tr = TR_SUCCESS;

      pszHackSlash = pszRootEnd;

      while (*pszHackSlash)
      {
         TCHAR chReplaced;

         while (*pszHackSlash && *pszHackSlash != TEXT('\\'))
            pszHackSlash = CharNext(pszHackSlash);

         /* Replace the slash with a null terminator to set the current folder. */

         chReplaced = *pszHackSlash;
         *pszHackSlash = TEXT('\0');

         /* Does the folder exist? */

         if (! PathExists(rgchPath))
         {
            /* No.  Try to create it. */

            TCHAR szAnsiPath[MAX_PATH];
            MakeAnsiPath(rgchPath, szAnsiPath);

            if (CreateDirectory(szAnsiPath, NULL))
            {
               WARNING_OUT((TEXT("CreateFolders(): Created folder %s."),
                            rgchPath));

               NotifyShell(rgchPath, NSE_CREATE_FOLDER);
            }
            else
            {
               WARNING_OUT((TEXT("CreateFolders(): Failed to create folder %s."),
                            rgchPath));

               tr = TR_DEST_OPEN_FAILED;
               break;
            }
         }

         *pszHackSlash = chReplaced;

         if (chReplaced)
            pszHackSlash++;
      }
   }
   else
      tr = TR_UNAVAILABLE_VOLUME;

   return(tr);
}


/*
** DestroySubtree()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT DestroySubtree(LPCTSTR pcszPath, HPATH hpath)
{
   TWINRESULT tr;

   ASSERT(IsCanonicalPath(pcszPath));
   ASSERT(IS_VALID_HANDLE(hpath, PATH));

   if (MyIsPathOnVolume(pcszPath, hpath))
   {
      tr = ExpandSubtree(hpath, &DeleteFolderProc, &tr);

      if (tr == TR_SUCCESS)
      {
         if (RemoveDirectory(pcszPath))
         {
            WARNING_OUT((TEXT("DestroySubtree(): Subtree %s removed successfully."),
                         pcszPath));

            NotifyShell(pcszPath, NSE_DELETE_FOLDER);
         }
         else
         {
            if (PathExists(pcszPath))
            {
               /* Still there. */

               WARNING_OUT((TEXT("DestroySubtree(): Failed to remove subtree root %s."),
                            pcszPath));

               tr = TR_DEST_WRITE_FAILED;
            }
            else
               /* Already gone. */
               tr = TR_SUCCESS;
         }
      }
   }
   else
      tr = TR_UNAVAILABLE_VOLUME;

   return(tr);
}


#ifdef DEBUG

/*
** IsValidRecStatusProcMsg()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidRecStatusProcMsg(UINT uMsg)
{
   BOOL bResult;

   switch (uMsg)
   {
      case RS_BEGIN_COPY:
      case RS_DELTA_COPY:
      case RS_END_COPY:
      case RS_BEGIN_MERGE:
      case RS_DELTA_MERGE:
      case RS_END_MERGE:
      case RS_BEGIN_DELETE:
      case RS_DELTA_DELETE:
      case RS_END_DELETE:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("IsValidRecStatusProcMsg(): Invalid RecStatusProc() message %u."),
                    uMsg));
         break;
   }

   return(bResult);
}

#endif

