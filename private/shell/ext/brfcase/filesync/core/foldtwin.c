/*
 * foldtwin.c - Folder twin ADT module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "stub.h"
#include "subcycle.h"


/* Constants
 ************/

/* pointer array allocation constants */

#define NUM_START_FOLDER_TWIN_PTRS     (16)
#define NUM_FOLDER_TWIN_PTRS_TO_ADD    (16)


/* Types
 ********/

/* internal new folder twin description */

typedef struct _inewfoldertwin
{
   HPATH hpathFirst;
   HPATH hpathSecond;
   HSTRING hsName;
   DWORD dwAttributes;
   HBRFCASE hbr;
   DWORD dwFlags;
}
INEWFOLDERTWIN;
DECLARE_STANDARD_TYPES(INEWFOLDERTWIN);

/* database folder twin list header */

typedef struct _dbfoldertwinlistheader
{
   LONG lcFolderPairs;
}
DBFOLDERTWINLISTHEADER;
DECLARE_STANDARD_TYPES(DBFOLDERTWINLISTHEADER);

/* database folder twin structure */

typedef struct _dbfoldertwin
{
   /* shared stub flags */

   DWORD dwStubFlags;

   /* old handle to first folder path */

   HPATH hpath1;

   /* old handle to second folder path */

   HPATH hpath2;

   /* old handle to name string */

   HSTRING hsName;

   /* attributes to match */

   DWORD dwAttributes;
}
DBFOLDERTWIN;
DECLARE_STANDARD_TYPES(DBFOLDERTWIN);


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE TWINRESULT MakeINewFolderTwin(HBRFCASE, PCNEWFOLDERTWIN, PINEWFOLDERTWIN);
PRIVATE_CODE void ReleaseINewFolderTwin(PINEWFOLDERTWIN);
PRIVATE_CODE TWINRESULT TwinFolders(PCINEWFOLDERTWIN, PFOLDERPAIR *);
PRIVATE_CODE BOOL FindFolderPair(PCINEWFOLDERTWIN, PFOLDERPAIR *);
PRIVATE_CODE BOOL CreateFolderPair(PCINEWFOLDERTWIN, PFOLDERPAIR *);
PRIVATE_CODE BOOL CreateHalfOfFolderPair(HPATH, HBRFCASE, PFOLDERPAIR *);
PRIVATE_CODE void DestroyHalfOfFolderPair(PFOLDERPAIR);
PRIVATE_CODE BOOL CreateSharedFolderPairData(PCINEWFOLDERTWIN, PFOLDERPAIRDATA *);
PRIVATE_CODE void DestroySharedFolderPairData(PFOLDERPAIRDATA);
PRIVATE_CODE COMPARISONRESULT FolderPairSortCmp(PCVOID, PCVOID);
PRIVATE_CODE COMPARISONRESULT FolderPairSearchCmp(PCVOID, PCVOID);
PRIVATE_CODE BOOL RemoveSourceFolderTwin(POBJECTTWIN, PVOID);
PRIVATE_CODE void UnlinkHalfOfFolderPair(PFOLDERPAIR);
PRIVATE_CODE BOOL FolderTwinIntersectsFolder(PCFOLDERPAIR, HPATH);
PRIVATE_CODE TWINRESULT CreateListOfFolderTwins(HBRFCASE, ARRAYINDEX, HPATH, PFOLDERTWIN *, PARRAYINDEX);
PRIVATE_CODE void DestroyListOfFolderTwins(PFOLDERTWIN);
PRIVATE_CODE TWINRESULT AddFolderTwinToList(PFOLDERPAIR, PFOLDERTWIN, PFOLDERTWIN *);
PRIVATE_CODE TWINRESULT TransplantFolderPair(PFOLDERPAIR, HPATH, HPATH);
PRIVATE_CODE TWINRESULT WriteFolderPair(HCACHEDFILE, PFOLDERPAIR);
PRIVATE_CODE TWINRESULT ReadFolderPair(HCACHEDFILE, HBRFCASE, HHANDLETRANS, HHANDLETRANS);

#ifdef VSTF

PRIVATE_CODE BOOL IsValidPCNEWFOLDERTWIN(PCNEWFOLDERTWIN);
PRIVATE_CODE BOOL IsValidPCFOLDERTWINLIST(PCFOLDERTWINLIST);
PRIVATE_CODE BOOL IsValidPCFOLDERTWIN(PCFOLDERTWIN);
PRIVATE_CODE BOOL IsValidFolderPairHalf(PCFOLDERPAIR);
PRIVATE_CODE BOOL IsValidPCFOLDERPAIRDATA(PCFOLDERPAIRDATA);

#endif

#ifdef DEBUG

PRIVATE_CODE BOOL IsValidPCINEWFOLDERTWIN(PCINEWFOLDERTWIN);
PRIVATE_CODE BOOL AreFolderPairsValid(HPTRARRAY);

#endif


/*
** MakeINewFolderTwin()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT MakeINewFolderTwin(HBRFCASE hbr,
                                           PCNEWFOLDERTWIN pcnftSrc,
                                           PINEWFOLDERTWIN pinftDest)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_STRUCT_PTR(pcnftSrc, CNEWFOLDERTWIN));
   ASSERT(IS_VALID_WRITE_PTR(pinftDest, CINEWFOLDERTWIN));

   if (AddString(pcnftSrc->pcszName, GetBriefcaseNameStringTable(hbr),
                 GetHashBucketIndex, &(pinftDest->hsName)))
   {
      HPATHLIST hpl;

      hpl = GetBriefcasePathList(hbr);

      tr = TranslatePATHRESULTToTWINRESULT(
            AddPath(hpl, pcnftSrc->pcszFolder1, &(pinftDest->hpathFirst)));

      if (tr == TR_SUCCESS)
      {
         tr = TranslatePATHRESULTToTWINRESULT(
               AddPath(hpl, pcnftSrc->pcszFolder2, &(pinftDest->hpathSecond)));

         if (tr == TR_SUCCESS)
         {
            pinftDest->dwAttributes = pcnftSrc->dwAttributes;
            pinftDest->dwFlags = pcnftSrc->dwFlags;
            pinftDest->hbr = hbr;
         }
         else
         {
            DeletePath(pinftDest->hpathFirst);
MAKEINEWFOLDERTWIN_BAIL:
            DeleteString(pinftDest->hsName);
         }
      }
      else
         goto MAKEINEWFOLDERTWIN_BAIL;
   }
   else
      tr = TR_OUT_OF_MEMORY;

   ASSERT(tr != TR_SUCCESS ||
          IS_VALID_STRUCT_PTR(pinftDest, CINEWFOLDERTWIN));

   return(tr);
}


/*
** ReleaseINewFolderTwin()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void ReleaseINewFolderTwin(PINEWFOLDERTWIN pinft)
{
   ASSERT(IS_VALID_STRUCT_PTR(pinft, CINEWFOLDERTWIN));

   DeletePath(pinft->hpathFirst);
   DeletePath(pinft->hpathSecond);
   DeleteString(pinft->hsName);

   return;
}


/*
** TwinFolders()
**
** Twins two folders.
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT TwinFolders(PCINEWFOLDERTWIN pcinft, PFOLDERPAIR *ppfp)
{
   PFOLDERPAIR pfp;
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(pcinft, CINEWFOLDERTWIN));
   ASSERT(IS_VALID_WRITE_PTR(ppfp, PFOLDERPAIR));

   /* Are the two folders the same? */

   if (ComparePaths(pcinft->hpathFirst, pcinft->hpathSecond) != CR_EQUAL)
   {
      /* Look for the two folders in existing folder pairs. */

      if (FindFolderPair(pcinft, &pfp))
      {
         /* Found a existing matching folder pair.  Complain. */

         *ppfp = pfp;

         tr = TR_DUPLICATE_TWIN;
      }
      else
      {
         /*
          * No existing matching folder pairs found.  Only allowing twinning to
          * paths whose roots are available.
          */

         if (IsPathVolumeAvailable(pcinft->hpathFirst) &&
             IsPathVolumeAvailable(pcinft->hpathSecond))
         {
            /*
             * If this is a new folder subtree pair, check to see if it would
             * create a cycle.
             */

            if (IS_FLAG_SET(pcinft->dwFlags, NFT_FL_SUBTREE))
               tr = CheckForSubtreeCycles(
                        GetBriefcaseFolderPairPtrArray(pcinft->hbr),
                        pcinft->hpathFirst, pcinft->hpathSecond,
                        pcinft->hsName);
            else
               tr = TR_SUCCESS;

            if (tr == TR_SUCCESS)
            {
               if (CreateFolderPair(pcinft, &pfp))
               {
                  *ppfp = pfp;

                  TRACE_OUT((TEXT("TwinFolders(): Creating %s twin pair %s and %s, files %s."),
                             IS_FLAG_SET(pcinft->dwFlags, NFT_FL_SUBTREE) ? TEXT("subtree") : TEXT("folder"),
                             DebugGetPathString(pcinft->hpathFirst),
                             DebugGetPathString(pcinft->hpathSecond),
                             GetString(pcinft->hsName)));
               }
               else
                  tr = TR_OUT_OF_MEMORY;
            }
         }
         else
            tr = TR_UNAVAILABLE_VOLUME;
      }
   }
   else
      tr = TR_SAME_FOLDER;

   return(tr);
}


/*
** FindFolderPair()
**
** Looks for a folder pair matching the given description.
**
** Arguments:     pcinft - pointer to INEWFOLDERTWIN describing folder pair to
**                          search for
**
** Returns:       Pointer to PFOLDERPAIR if found.  NULL if not found.
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL FindFolderPair(PCINEWFOLDERTWIN pcinft, PFOLDERPAIR *ppfp)
{
   ARRAYINDEX aiFirst;

   ASSERT(IS_VALID_STRUCT_PTR(pcinft, CINEWFOLDERTWIN));
   ASSERT(IS_VALID_WRITE_PTR(ppfp, PFOLDERPAIR));

   /*
    * Search all folder pairs containing the first folder.  Then scan all these
    * folder pairs for the second folder.
    */

   *ppfp = NULL;

   if (SearchSortedArray(GetBriefcaseFolderPairPtrArray(pcinft->hbr),
                         &FolderPairSearchCmp, pcinft->hpathFirst, &aiFirst))
   {
      ARRAYINDEX aicPtrs;
      HPTRARRAY hpaFolderPairs;
      LONG ai;
      PFOLDERPAIR pfp;

      /*
       * aiFirst holds the index of the first folder pair that
       * contains the first folder name.
       */

      /*
       * Now search each of these folder pairs for all paired folders
       * using the second folder name.
       */

      hpaFolderPairs = GetBriefcaseFolderPairPtrArray(pcinft->hbr);

      aicPtrs = GetPtrCount(hpaFolderPairs);
      ASSERT(aicPtrs > 0);
      ASSERT(! (aicPtrs % 2));
      ASSERT(aiFirst >= 0);
      ASSERT(aiFirst < aicPtrs);

      for (ai = aiFirst; ai < aicPtrs; ai++)
      {
         pfp = GetPtr(hpaFolderPairs, ai);

         ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

         /* Does this folder pair match the proposed folder pair? */

         if (ComparePaths(pfp->hpath, pcinft->hpathFirst) == CR_EQUAL)
         {
            /*
             * An existing pair of folder twins is considered the same as a
             * proposed new pair of folder twins when the two pairs of folder
             * twins share the same:
             *    1) pair of PATHs
             *    2) name specification
             *    3) file attributes
             *    4) subtree flag setting
             */

            if (ComparePaths(pfp->pfpOther->hpath, pcinft->hpathSecond) == CR_EQUAL &&
                CompareNameStringsByHandle(pfp->pfpd->hsName, pcinft->hsName) == CR_EQUAL &&
                pfp->pfpd->dwAttributes == pcinft->dwAttributes &&
                ((IS_FLAG_SET(pfp->stub.dwFlags, STUB_FL_SUBTREE) &&
                  IS_FLAG_SET(pcinft->dwFlags, NFT_FL_SUBTREE)) ||
                 (IS_FLAG_CLEAR(pfp->stub.dwFlags, STUB_FL_SUBTREE) &&
                  IS_FLAG_CLEAR(pcinft->dwFlags, NFT_FL_SUBTREE))))
            {
               /* Yes. */

               *ppfp = pfp;
               break;
            }
         }
         else
            break;
      }
   }

   return(*ppfp != NULL);
}


/*
** CreateFolderPair()
**
** Creates a new folder pair, and adds them to a briefcase's list of folder
** pairs.
**
** Arguments:     pcinft - pointer to INEWFOLDERTWIN describing folder pair to
**                         create
**                ppfp - pointer to PFOLDERPAIR to be filled in with pointer to
**                       half of new folder pair representing
**                       pcnft->pcszFolder1
**
** Returns:
**
** Side Effects:  Adds the new folder pair to the global array of folder pairs.
**
** N.b., this function does not first check to see if the folder pair already
** exists in the global list of folder pairs.
*/
PRIVATE_CODE BOOL CreateFolderPair(PCINEWFOLDERTWIN pcinft, PFOLDERPAIR *ppfp)
{
   BOOL bResult = FALSE;
   PFOLDERPAIRDATA pfpd;

   ASSERT(IS_VALID_STRUCT_PTR(pcinft, CINEWFOLDERTWIN));
   ASSERT(IS_VALID_WRITE_PTR(ppfp, PFOLDERPAIR));

   /* Try to create the shared folder data structure. */

   if (CreateSharedFolderPairData(pcinft, &pfpd))
   {
      PFOLDERPAIR pfpNew1;
      BOOL bPtr1Loose = TRUE;

      if (CreateHalfOfFolderPair(pcinft->hpathFirst, pcinft->hbr, &pfpNew1))
      {
         PFOLDERPAIR pfpNew2;

         if (CreateHalfOfFolderPair(pcinft->hpathSecond, pcinft->hbr,
                                    &pfpNew2))
         {
            HPTRARRAY hpaFolderPairs;
            ARRAYINDEX ai1;

            /* Combine the two folder pair halves. */

            pfpNew1->pfpd = pfpd;
            pfpNew1->pfpOther = pfpNew2;

            pfpNew2->pfpd = pfpd;
            pfpNew2->pfpOther = pfpNew1;

            /* Set flags. */

            if (IS_FLAG_SET(pcinft->dwFlags, NFT_FL_SUBTREE))
            {
               SetStubFlag(&(pfpNew1->stub), STUB_FL_SUBTREE);
               SetStubFlag(&(pfpNew2->stub), STUB_FL_SUBTREE);
            }

            /*
             * Try to add the two folder pairs to the global list of folder
             * pairs.
             */

            hpaFolderPairs = GetBriefcaseFolderPairPtrArray(pcinft->hbr);

            if (AddPtr(hpaFolderPairs, FolderPairSortCmp, pfpNew1, &ai1))
            {
               ARRAYINDEX ai2;

               bPtr1Loose = FALSE;

               if (AddPtr(hpaFolderPairs, FolderPairSortCmp, pfpNew2, &ai2))
               {
                  ASSERT(IS_VALID_STRUCT_PTR(pfpNew1, CFOLDERPAIR));
                  ASSERT(IS_VALID_STRUCT_PTR(pfpNew2, CFOLDERPAIR));

                  if (ApplyNewFolderTwinsToTwinFamilies(pfpNew1))
                  {
                     *ppfp = pfpNew1;
                     bResult = TRUE;
                  }
                  else
                  {
                     DeletePtr(hpaFolderPairs, ai2);

CREATEFOLDERPAIR_BAIL1:
                     DeletePtr(hpaFolderPairs, ai1);

CREATEFOLDERPAIR_BAIL2:
                     /*
                      * Don't try to remove pfpNew2 from the global list of
                      * folder pairs here since it was never added
                      * successfully.
                      */
                     DestroyHalfOfFolderPair(pfpNew2);

CREATEFOLDERPAIR_BAIL3:
                     /*
                      * Don't try to remove pfpNew1 from the global list of
                      * folder pairs here since it was never added
                      * successfully.
                      */
                     DestroyHalfOfFolderPair(pfpNew1);

CREATEFOLDERPAIR_BAIL4:
                     DestroySharedFolderPairData(pfpd);
                  }
               }
               else
                  goto CREATEFOLDERPAIR_BAIL1;
            }
            else
               goto CREATEFOLDERPAIR_BAIL2;
         }
         else
            goto CREATEFOLDERPAIR_BAIL3;
      }
      else
         goto CREATEFOLDERPAIR_BAIL4;
   }

   return(bResult);
}


/*
** CreateHalfOfFolderPair()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CreateHalfOfFolderPair(HPATH hpathFolder, HBRFCASE hbr,
                                    PFOLDERPAIR *ppfp)
{
   BOOL bResult = FALSE;
   PFOLDERPAIR pfpNew;

   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_WRITE_PTR(ppfp, PFOLDERPAIR));

   /* Try to create a new FOLDERPAIR structure. */

   if (AllocateMemory(sizeof(*pfpNew), &pfpNew))
   {
      /* Try to add the folder string to the folder string table. */

      if (CopyPath(hpathFolder, GetBriefcasePathList(hbr), &(pfpNew->hpath)))
      {
         /* Fill in the fields of the new FOLDERPAIR structure. */

         InitStub(&(pfpNew->stub), ST_FOLDERPAIR);

         *ppfp = pfpNew;
         bResult = TRUE;
      }
      else
         FreeMemory(pfpNew);
   }

   return(bResult);
}


/*
** DestroyHalfOfFolderPair()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroyHalfOfFolderPair(PFOLDERPAIR pfp)
{
   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   TRACE_OUT((TEXT("DestroyHalfOfFolderPair(): Destroying folder twin %s."),
              DebugGetPathString(pfp->hpath)));

   /* Has the other half of this folder pair already been destroyed? */

   if (IsStubFlagClear(&(pfp->stub), STUB_FL_BEING_DELETED))
      /* No.  Indicate that this half has already been deleted. */
      SetStubFlag(&(pfp->pfpOther->stub), STUB_FL_BEING_DELETED);

   /* Destroy FOLDERPAIR fields. */

   DeletePath(pfp->hpath);
   FreeMemory(pfp);

   return;
}


/*
** CreateSharedFolderPairData()
**
** Creates a shared folder pair data structure.
**
** Arguments:     pcinft - pointer to INEWFOLDERTWIN describing folder pair
**                          being created
**
** Returns:       Pointer to new folder pair data structure if successful.
**                NULL if unsuccessful.
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CreateSharedFolderPairData(PCINEWFOLDERTWIN pcinft,
                                        PFOLDERPAIRDATA *ppfpd)
{
   PFOLDERPAIRDATA pfpd;

   ASSERT(IS_VALID_STRUCT_PTR(pcinft, CINEWFOLDERTWIN));
   ASSERT(IS_VALID_WRITE_PTR(ppfpd, PFOLDERPAIRDATA));

   /* Try to allocate a new shared folder pair data data structure. */

   *ppfpd = NULL;

   if (AllocateMemory(sizeof(*pfpd), &pfpd))
   {
      /* Fill in the FOLDERPAIRDATA structure fields. */

      LockString(pcinft->hsName);
      pfpd->hsName = pcinft->hsName;

      pfpd->dwAttributes = pcinft->dwAttributes;
      pfpd->hbr = pcinft->hbr;

      ASSERT(! IS_ATTR_DIR(pfpd->dwAttributes));

      CLEAR_FLAG(pfpd->dwAttributes, FILE_ATTRIBUTE_DIRECTORY);

      *ppfpd = pfpd;

      ASSERT(IS_VALID_STRUCT_PTR(*ppfpd, CFOLDERPAIRDATA));
   }

   return(*ppfpd != NULL);
}


/*
** DestroySharedFolderPairData()
**
** Destroys shared folder pair data.
**
** Arguments:     pfpd - pointer to shared folder pair data to destroy
**
** Returns:       void
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroySharedFolderPairData(PFOLDERPAIRDATA pfpd)
{
   ASSERT(IS_VALID_STRUCT_PTR(pfpd, CFOLDERPAIRDATA));

   /* Destroy FOLDERPAIRDATA fields. */

   DeleteString(pfpd->hsName);
   FreeMemory(pfpd);

   return;
}


/*
** FolderPairSortCmp()
**
** Pointer comparison function used to sort the global array of folder pairs.
**
** Arguments:     pcfp1 - pointer to FOLDERPAIR describing first folder pair
**                pcfp2 - pointer to FOLDERPAIR describing second folder pair
**
** Returns:
**
** Side Effects:  none
**
** Folder pairs are sorted by:
**    1) path
**    2) pointer value
*/
PRIVATE_CODE COMPARISONRESULT FolderPairSortCmp(PCVOID pcfp1, PCVOID pcfp2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pcfp1, CFOLDERPAIR));
   ASSERT(IS_VALID_STRUCT_PTR(pcfp2, CFOLDERPAIR));

   cr = ComparePaths(((PCFOLDERPAIR)pcfp1)->hpath,
                     ((PCFOLDERPAIR)pcfp2)->hpath);

   if (cr == CR_EQUAL)
      cr = ComparePointers(pcfp1, pcfp2);

   return(cr);
}


/*
** FolderPairSearchCmp()
**
** Pointer comparison function used to search the global array of folder pairs
** for the first folder pair for a given folder.
**
** Arguments:     hpath - folder pair to search for
**                pcfp - pointer to FOLDERPAIR to examine
**
** Returns:
**
** Side Effects:  none
**
** Folder pairs are searched by:
**    1) path
*/
PRIVATE_CODE COMPARISONRESULT FolderPairSearchCmp(PCVOID hpath, PCVOID pcfp)
{
   ASSERT(IS_VALID_HANDLE((HPATH)hpath, PATH));
   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));

   return(ComparePaths((HPATH)hpath, ((PCFOLDERPAIR)pcfp)->hpath));
}


/*
** RemoveSourceFolderTwin()
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

PRIVATE_CODE BOOL RemoveSourceFolderTwin(POBJECTTWIN pot, PVOID pv)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(! pv);

   if (EVAL(pot->ulcSrcFolderTwins > 0))
      pot->ulcSrcFolderTwins--;

   /*
    * If there are no more source folder twins for this object twin, and this
    * object twin is not a separate "orphan" object twin, wipe it out.
    */

   if (! pot->ulcSrcFolderTwins &&
       IsStubFlagClear(&(pot->stub), STUB_FL_FROM_OBJECT_TWIN))
      EVAL(DestroyStub(&(pot->stub)) == TR_SUCCESS);

   return(TRUE);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


/*
** UnlinkHalfOfFolderPair()
**
** Unlinks one half of a pair of folder twins.
**
** Arguments:     pfp - pointer to folder pair half to unlink
**
** Returns:       void
**
** Side Effects:  Removes a source folder twin from each of the object twin's
**                in the folder pair's list of generated object twins.  May
**                cause object twins and twin families to be destroyed.
*/
PRIVATE_CODE void UnlinkHalfOfFolderPair(PFOLDERPAIR pfp)
{
   HPTRARRAY hpaFolderPairs;
   ARRAYINDEX aiUnlink;

   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   TRACE_OUT((TEXT("UnlinkHalfOfFolderPair(): Unlinking folder twin %s."),
              DebugGetPathString(pfp->hpath)));

   /* Search for the folder pair to be unlinked. */

   hpaFolderPairs = GetBriefcaseFolderPairPtrArray(pfp->pfpd->hbr);

   if (EVAL(SearchSortedArray(hpaFolderPairs, &FolderPairSortCmp, pfp,
                              &aiUnlink)))
   {
      /* Unlink folder pair. */

      ASSERT(GetPtr(hpaFolderPairs, aiUnlink) == pfp);

      DeletePtr(hpaFolderPairs, aiUnlink);

      /*
       * Don't mark folder pair stub unlinked here.  Let caller do that after
       * both folder pair halves have been unlinked.
       */

      /* Remove a source folder twin from all generated object twins. */

      EVAL(EnumGeneratedObjectTwins(pfp, &RemoveSourceFolderTwin, NULL));
   }

   return;
}


/*
** FolderTwinIntersectsFolder()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL FolderTwinIntersectsFolder(PCFOLDERPAIR pcfp, HPATH hpathFolder)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));
   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));

   if (IsStubFlagSet(&(pcfp->stub), STUB_FL_SUBTREE))
      bResult = IsPathPrefix(hpathFolder, pcfp->hpath);
   else
      bResult = (ComparePaths(hpathFolder, pcfp->hpath) == CR_EQUAL);

   return(bResult);
}


/*
** CreateListOfFolderTwins()
**
** Creates a list of folder twins from a block of folder pairs.
**
** Arguments:     aiFirst - index of first folder pair in the array of folder
**                          pairs
**                hpathFolder - folder that list of folder twins is to be
**                              created for
**                ppftHead - pointer to PFOLDERTWIN to be filled in with
**                           pointer to first folder twin in new list
**                paic - pointer to ARRAYINDEX to be filled in with number of
**                       folder twins in new list
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT CreateListOfFolderTwins(HBRFCASE hbr, ARRAYINDEX aiFirst,
                                           HPATH hpathFolder,
                                           PFOLDERTWIN *ppftHead,
                                           PARRAYINDEX paic)
{
   TWINRESULT tr;
   PFOLDERPAIR pfp;
   HPATH hpath;
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;
   PFOLDERTWIN pftHead;
   HPTRARRAY hpaFolderTwins;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));
   ASSERT(IS_VALID_WRITE_PTR(ppftHead, PFOLDERTWIN));
   ASSERT(IS_VALID_WRITE_PTR(paic, ARRAYINDEX));

   /*
    * Get the handle to the common folder that the list of folder twins is
    * being prepared for.
    */

   hpaFolderTwins = GetBriefcaseFolderPairPtrArray(hbr);

   pfp = GetPtr(hpaFolderTwins, aiFirst);

   hpath = pfp->hpath;

   /*
    * Add the other half of each matching folder pair to the folder twin list
    * as a folder twin.
    */

   aicPtrs = GetPtrCount(hpaFolderTwins);
   ASSERT(aicPtrs > 0);
   ASSERT(! (aicPtrs % 2));
   ASSERT(aiFirst >= 0);
   ASSERT(aiFirst < aicPtrs);

   /* Start with an empty list of folder twins. */

   pftHead = NULL;

   /*
    * A pointer to the first folder pair is already in pfp, but we'll look it
    * up again.
    */

   TRACE_OUT((TEXT("CreateListOfFolderTwins(): Creating list of folder twins of folder %s."),
              DebugGetPathString(hpath)));

   tr = TR_SUCCESS;

   for (ai = aiFirst; ai < aicPtrs && tr == TR_SUCCESS; ai++)
   {
      pfp = GetPtr(hpaFolderTwins, ai);

      if (ComparePaths(pfp->hpath, hpathFolder) == CR_EQUAL)
         tr = AddFolderTwinToList(pfp, pftHead, &pftHead);
      else
         break;
   }

   TRACE_OUT((TEXT("CreateListOfFolderTwins(): Finished creating list of folder twins of folder %s."),
              DebugGetPathString(hpath)));

   if (tr == TR_SUCCESS)
   {
      /* Success!  Fill in the result parameters. */

      *ppftHead = pftHead;
      *paic = ai - aiFirst;
   }
   else
      /* Free any folder twins that have been added to the list. */
      DestroyListOfFolderTwins(pftHead);

   return(tr);
}


/*
** DestroyListOfFolderTwins()
**
** Wipes out the folder twins in a folder twin list.
**
** Arguments:     pftHead - pointer to first folder twin in list
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroyListOfFolderTwins(PFOLDERTWIN pftHead)
{
   while (pftHead)
   {
      PFOLDERTWIN pftOldHead;

      ASSERT(IS_VALID_STRUCT_PTR(pftHead, CFOLDERTWIN));

      UnlockStub(&(((PFOLDERPAIR)(pftHead->hftSrc))->stub));
      UnlockStub(&(((PFOLDERPAIR)(pftHead->hftOther))->stub));

      pftOldHead = pftHead;
      pftHead = (PFOLDERTWIN)(pftHead->pcftNext);

      FreeMemory((LPTSTR)(pftOldHead->pcszSrcFolder));
      FreeMemory((LPTSTR)(pftOldHead->pcszOtherFolder));

      FreeMemory(pftOldHead);
   }

   return;
}


/*
** AddFolderTwinToList()
**
** Adds a folder twin to a list of folder twins.
**
** Arguments:     pfpSrc - pointer to source folder pair to be added
**                pftHead - pointer to head of folder twin list, may be NULL
**                ppft - pointer to PFOLDERTWIN to be filled in with pointer
**                         to new folder twin, ppft may be &pftHead
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT AddFolderTwinToList(PFOLDERPAIR pfpSrc,
                                            PFOLDERTWIN pftHead,
                                            PFOLDERTWIN *ppft)
{
   TWINRESULT tr = TR_OUT_OF_MEMORY;
   PFOLDERTWIN pftNew;

   ASSERT(IS_VALID_STRUCT_PTR(pfpSrc, CFOLDERPAIR));
   ASSERT(! pftHead || IS_VALID_STRUCT_PTR(pftHead, CFOLDERTWIN));
   ASSERT(IS_VALID_WRITE_PTR(ppft, PFOLDERTWIN));

   /* Try to create a new FOLDERTWIN structure. */

   if (AllocateMemory(sizeof(*pftNew), &pftNew))
   {
      LPTSTR pszFirstFolder;

      if (AllocatePathString(pfpSrc->hpath, &pszFirstFolder))
      {
         LPTSTR pszSecondFolder;

         if (AllocatePathString(pfpSrc->pfpOther->hpath, &pszSecondFolder))
         {
            /* Fill in FOLDERTWIN structure fields. */

            pftNew->pcftNext = pftHead;
            pftNew->hftSrc = (HFOLDERTWIN)pfpSrc;
            pftNew->hvidSrc = (HVOLUMEID)(pfpSrc->hpath);
            pftNew->pcszSrcFolder = pszFirstFolder;
            pftNew->hftOther = (HFOLDERTWIN)(pfpSrc->pfpOther);
            pftNew->hvidOther = (HVOLUMEID)(pfpSrc->pfpOther->hpath);
            pftNew->pcszOtherFolder = pszSecondFolder;
            pftNew->pcszName = GetString(pfpSrc->pfpd->hsName);

            pftNew->dwFlags = 0;

            if (IsStubFlagSet(&(pfpSrc->stub), STUB_FL_SUBTREE))
               pftNew->dwFlags = FT_FL_SUBTREE;

            LockStub(&(pfpSrc->stub));
            LockStub(&(pfpSrc->pfpOther->stub));

            *ppft = pftNew;
            tr = TR_SUCCESS;

            TRACE_OUT((TEXT("AddFolderTwinToList(): Added folder twin %s of folder %s matching objects %s."),
                       pftNew->pcszSrcFolder,
                       pftNew->pcszOtherFolder,
                       pftNew->pcszName));
         }
         else
         {
            FreeMemory(pszFirstFolder);
ADDFOLDERTWINTOLIST_BAIL:
            FreeMemory(pftNew);
         }
      }
      else
         goto ADDFOLDERTWINTOLIST_BAIL;
   }

   ASSERT(tr != TR_SUCCESS ||
          IS_VALID_STRUCT_PTR(*ppft, CFOLDERTWIN));

   return(tr);
}


/*
** TransplantFolderPair()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT TransplantFolderPair(PFOLDERPAIR pfp,
                                             HPATH hpathOldFolder,
                                             HPATH hpathNewFolder)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));
   ASSERT(IS_VALID_HANDLE(hpathOldFolder, PATH));
   ASSERT(IS_VALID_HANDLE(hpathNewFolder, PATH));

   /* Is this folder pair rooted in the renamed folder's subtree? */

   if (IsPathPrefix(pfp->hpath, hpathOldFolder))
   {
      TCHAR rgchPathSuffix[MAX_PATH_LEN];
      LPCTSTR pcszSubPath;
      HPATH hpathNew;

      /* Yes.  Change the folder pair's root. */

      pcszSubPath = FindChildPathSuffix(hpathOldFolder, pfp->hpath,
                                        rgchPathSuffix);

      if (AddChildPath(GetBriefcasePathList(pfp->pfpd->hbr), hpathNewFolder,
                       pcszSubPath, &hpathNew))
      {
         if (IsStubFlagSet(&(pfp->stub), STUB_FL_SUBTREE))
         {
            ASSERT(IsStubFlagSet(&(pfp->pfpOther->stub), STUB_FL_SUBTREE));

            BeginTranslateFolder(pfp);

            tr = CheckForSubtreeCycles(
                     GetBriefcaseFolderPairPtrArray(pfp->pfpd->hbr), hpathNew,
                     pfp->pfpOther->hpath, pfp->pfpd->hsName);

            EndTranslateFolder(pfp);
         }
         else
            tr = TR_SUCCESS;

         if (tr == TR_SUCCESS)
         {
            TRACE_OUT((TEXT("TransplantFolderPair(): Transplanted folder twin %s to %s."),
                       DebugGetPathString(pfp->hpath),
                       DebugGetPathString(hpathNew)));

            DeletePath(pfp->hpath);
            pfp->hpath = hpathNew;
         }
         else
            DeletePath(hpathNew);
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
      tr = TR_SUCCESS;

   return(tr);
}


/*
** WriteFolderPair()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT WriteFolderPair(HCACHEDFILE hcf, PFOLDERPAIR pfp)
{
   TWINRESULT tr;
   DBFOLDERTWIN dbft;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   /* Set up folder pair database structure. */

   dbft.dwStubFlags = (pfp->stub.dwFlags & DB_STUB_FLAGS_MASK);
   dbft.hpath1 = pfp->hpath;
   dbft.hpath2 = pfp->pfpOther->hpath;
   dbft.hsName = pfp->pfpd->hsName;
   dbft.dwAttributes = pfp->pfpd->dwAttributes;

   /* Save folder pair database structure. */

   if (WriteToCachedFile(hcf, (PCVOID)&dbft, sizeof(dbft), NULL))
      tr = TR_SUCCESS;
   else
      tr = TR_BRIEFCASE_WRITE_FAILED;

   return(tr);
}


/*
** ReadFolderPair()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT ReadFolderPair(HCACHEDFILE hcf, HBRFCASE hbr,
                                  HHANDLETRANS hhtFolderTrans,
                                  HHANDLETRANS hhtNameTrans)
{
   TWINRESULT tr = TR_CORRUPT_BRIEFCASE;
   DBFOLDERTWIN dbft;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hhtFolderTrans, HANDLETRANS));
   ASSERT(IS_VALID_HANDLE(hhtNameTrans, HANDLETRANS));

   if (ReadFromCachedFile(hcf, &dbft, sizeof(dbft), &dwcbRead) &&
       dwcbRead == sizeof(dbft))
   {
      INEWFOLDERTWIN inft;

      if (TranslateHandle(hhtFolderTrans, (HGENERIC)(dbft.hpath1), (PHGENERIC)&(inft.hpathFirst)))
      {
         if (TranslateHandle(hhtFolderTrans, (HGENERIC)(dbft.hpath2), (PHGENERIC)&(inft.hpathSecond)))
         {
            if (TranslateHandle(hhtNameTrans, (HGENERIC)(dbft.hsName), (PHGENERIC)&(inft.hsName)))
            {
               PFOLDERPAIR pfp;

               inft.dwAttributes = dbft.dwAttributes;
               inft.hbr = hbr;

               if (IS_FLAG_SET(dbft.dwStubFlags, STUB_FL_SUBTREE))
                  inft.dwFlags = NFT_FL_SUBTREE;
               else
                  inft.dwFlags = 0;

               if (CreateFolderPair(&inft, &pfp))
                  tr = TR_SUCCESS;
               else
                  tr = TR_OUT_OF_MEMORY;
            }
         }
      }
   }

   return(tr);
}


#ifdef VSTF

/*
** IsValidPCNEWFOLDERTWIN()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCNEWFOLDERTWIN(PCNEWFOLDERTWIN pcnft)
{
   return(IS_VALID_READ_PTR(pcnft, CNEWFOLDERTWIN) &&
          EVAL(pcnft->ulSize == sizeof(*pcnft)) &&
          IS_VALID_STRING_PTR(pcnft->pcszFolder1, CSTR) &&
          IS_VALID_STRING_PTR(pcnft->pcszFolder2, CSTR) &&
          IS_VALID_STRING_PTR(pcnft->pcszName, CSTR) &&
          FLAGS_ARE_VALID(pcnft->dwAttributes, ALL_FILE_ATTRIBUTES) &&
          FLAGS_ARE_VALID(pcnft->dwFlags, ALL_NFT_FLAGS));
}


/*
** IsValidPCFOLDERTWINLIST()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCFOLDERTWINLIST(PCFOLDERTWINLIST pcftl)
{
   BOOL bResult = FALSE;

   if (IS_VALID_READ_PTR(pcftl, CFOLDERTWINLIST) &&
       IS_VALID_HANDLE(pcftl->hbr, BRFCASE))
   {
      PCFOLDERTWIN pcft;
      ULONG ulcFolderTwins = 0;

      for (pcft = pcftl->pcftFirst;
           pcft && IS_VALID_STRUCT_PTR(pcft, CFOLDERTWIN);
           pcft = pcft->pcftNext)
      {
         ASSERT(ulcFolderTwins < ULONG_MAX);
         ulcFolderTwins++;
      }

      if (! pcft && EVAL(ulcFolderTwins == pcftl->ulcItems))
         bResult = TRUE;
   }

   return(bResult);
}


/*
** IsValidPCFOLDERTWIN()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCFOLDERTWIN(PCFOLDERTWIN pcft)
{
   /* dwUser may be any value. */

   return(IS_VALID_READ_PTR(pcft, CFOLDERTWIN) &&
          IS_VALID_HANDLE(pcft->hftSrc, FOLDERTWIN) &&
          IS_VALID_HANDLE(pcft->hvidSrc, VOLUMEID) &&
          IS_VALID_STRING_PTR(pcft->pcszSrcFolder, CSTR) &&
          IS_VALID_HANDLE(pcft->hftOther, FOLDERTWIN) &&
          IS_VALID_HANDLE(pcft->hvidOther, VOLUMEID) &&
          IS_VALID_STRING_PTR(pcft->pcszOtherFolder, CSTR) &&
          IS_VALID_STRING_PTR(pcft->pcszName, CSTR) &&
          FLAGS_ARE_VALID(pcft->dwFlags, ALL_FT_FLAGS));
}


/*
** IsValidFolderPairHalf()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidFolderPairHalf(PCFOLDERPAIR pcfp)
{
   return(IS_VALID_READ_PTR(pcfp, CFOLDERPAIR) &&
          IS_VALID_STRUCT_PTR(&(pcfp->stub), CSTUB) &&
          FLAGS_ARE_VALID(GetStubFlags(&(pcfp->stub)), ALL_FOLDER_TWIN_FLAGS) &&
          IS_VALID_HANDLE(pcfp->hpath, PATH) &&
          IS_VALID_STRUCT_PTR(pcfp->pfpd, CFOLDERPAIRDATA) &&
          (IsStubFlagSet(&(pcfp->stub), STUB_FL_BEING_DELETED) ||
           IS_VALID_READ_PTR(pcfp->pfpOther, CFOLDERPAIR)));
}


/*
** IsValidPCFOLDERPAIRDATA()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCFOLDERPAIRDATA(PCFOLDERPAIRDATA pcfpd)
{
   /* Don't validate hbr. */

   return(IS_VALID_READ_PTR(pcfpd, CFOLDERPAIRDATA) &&
          IS_VALID_HANDLE(pcfpd->hsName, STRING));
}

#endif


#ifdef DEBUG

/*
** IsValidPCINEWFOLDERTWIN()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCINEWFOLDERTWIN(PCINEWFOLDERTWIN pcinft)
{
   return(IS_VALID_READ_PTR(pcinft, CINEWFOLDERTWIN) &&
          IS_VALID_HANDLE(pcinft->hpathFirst, PATH) &&
          IS_VALID_HANDLE(pcinft->hpathSecond, PATH) &&
          IS_VALID_HANDLE(pcinft->hsName, STRING) &&
          FLAGS_ARE_VALID(pcinft->dwAttributes, ALL_FILE_ATTRIBUTES) &&
          FLAGS_ARE_VALID(pcinft->dwFlags, ALL_NFT_FLAGS) &&
          IS_VALID_HANDLE(pcinft->hbr, BRFCASE));
}


/*
** AreFolderPairsValid()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL AreFolderPairsValid(HPTRARRAY hpaFolderPairs)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hpaFolderPairs, PTRARRAY));

   aicPtrs = GetPtrCount(hpaFolderPairs);
   ASSERT(! (aicPtrs % 2));

   for (ai = 0;
        ai < aicPtrs && IS_VALID_STRUCT_PTR(GetPtr(hpaFolderPairs, ai), CFOLDERPAIR);
        ai++)
      ;

   return(ai == aicPtrs);
}

#endif


/****************************** Public Functions *****************************/


/*
** CreateFolderPairPtrArray()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CreateFolderPairPtrArray(PHPTRARRAY phpa)
{
   NEWPTRARRAY npa;

   ASSERT(IS_VALID_WRITE_PTR(phpa, HPTRARRAY));

   /* Try to create a folder pair pointer array. */

   npa.aicInitialPtrs = NUM_START_FOLDER_TWIN_PTRS;
   npa.aicAllocGranularity = NUM_FOLDER_TWIN_PTRS_TO_ADD;
   npa.dwFlags = NPA_FL_SORTED_ADD;

   return(CreatePtrArray(&npa, phpa));
}


/*
** DestroyFolderPairPtrArray()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DestroyFolderPairPtrArray(HPTRARRAY hpa)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));

   /* Free all folder pairs in pointer array. */

   aicPtrs = GetPtrCount(hpa);
   ASSERT(! (aicPtrs % 2));

   for (ai = 0; ai < aicPtrs; ai++)
   {
      PFOLDERPAIR pfp;
      PFOLDERPAIR pfpOther;
      PFOLDERPAIRDATA pfpd;
      BOOL bDeleteFolderPairData;

      pfp = GetPtr(hpa, ai);

      ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

      /* Copy fields needed after folder pair half's demise. */

      pfpOther = pfp->pfpOther;
      pfpd = pfp->pfpd;
      bDeleteFolderPairData = IsStubFlagSet(&(pfp->stub), STUB_FL_BEING_DELETED);

      DestroyHalfOfFolderPair(pfp);

      /* Has the other half of this folder pair already been destroyed? */

      if (bDeleteFolderPairData)
         /* Yes.  Destroy the pair's shared data. */
         DestroySharedFolderPairData(pfpd);
   }

   /* Now wipe out the pointer array. */

   DestroyPtrArray(hpa);

   return;
}


/*
** LockFolderPair()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void LockFolderPair(PFOLDERPAIR pfp)
{
   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   ASSERT(IsStubFlagClear(&(pfp->stub), STUB_FL_UNLINKED));
   ASSERT(IsStubFlagClear(&(pfp->pfpOther->stub), STUB_FL_UNLINKED));

   ASSERT(pfp->stub.ulcLock < ULONG_MAX);
   pfp->stub.ulcLock++;

   ASSERT(pfp->pfpOther->stub.ulcLock < ULONG_MAX);
   pfp->pfpOther->stub.ulcLock++;

   return;
}


/*
** UnlockFolderPair()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void UnlockFolderPair(PFOLDERPAIR pfp)
{
   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   if (EVAL(pfp->stub.ulcLock > 0))
      pfp->stub.ulcLock--;

   if (EVAL(pfp->pfpOther->stub.ulcLock > 0))
      pfp->pfpOther->stub.ulcLock--;

   if (! pfp->stub.ulcLock &&
       IsStubFlagSet(&(pfp->stub), STUB_FL_UNLINKED))
   {
      ASSERT(! pfp->pfpOther->stub.ulcLock);
      ASSERT(IsStubFlagSet(&(pfp->pfpOther->stub), STUB_FL_UNLINKED));

      DestroyFolderPair(pfp);
   }

   return;
}


/*
** UnlinkFolderPair()
**
** Unlinks a folder pair.
**
** Arguments:     pfp - pointer to folder pair to be unlinked
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT UnlinkFolderPair(PFOLDERPAIR pfp)
{
   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   ASSERT(IsStubFlagClear(&(pfp->stub), STUB_FL_UNLINKED));
   ASSERT(IsStubFlagClear(&(pfp->pfpOther->stub), STUB_FL_UNLINKED));

   /* Unlink both halves of the folder pair. */

   UnlinkHalfOfFolderPair(pfp);
   UnlinkHalfOfFolderPair(pfp->pfpOther);

   SetStubFlag(&(pfp->stub), STUB_FL_UNLINKED);
   SetStubFlag(&(pfp->pfpOther->stub), STUB_FL_UNLINKED);

   return(TR_SUCCESS);
}


/*
** DestroyFolderPair()
**
** Destroys a folder pair.
**
** Arguments:     pfp - pointer to folder pair to destroy
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void DestroyFolderPair(PFOLDERPAIR pfp)
{
   PFOLDERPAIRDATA pfpd;

   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   /* Destroy both FOLDERPAIR halves, and shared data. */

   pfpd = pfp->pfpd;

   DestroyHalfOfFolderPair(pfp->pfpOther);
   DestroyHalfOfFolderPair(pfp);

   DestroySharedFolderPairData(pfpd);

   return;
}


/*
** MyTranslateFolder()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT MyTranslateFolder(HBRFCASE hbr, HPATH hpathOld,
                                         HPATH hpathNew)
{
   TWINRESULT tr = TR_SUCCESS;
   HPTRARRAY hpaFolderPairs;
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hpathOld, PATH));
   ASSERT(IS_VALID_HANDLE(hpathNew, PATH));

   /*
    * Change folders of all folder pairs rooted in pcszOldFolder's subtree to
    * being rooted in pcszNewFolder's subtree.
    */

   hpaFolderPairs = GetBriefcaseFolderPairPtrArray(hbr);
   aicPtrs = GetPtrCount(hpaFolderPairs);
   ASSERT(! (aicPtrs % 2));

   for (ai = 0; ai < aicPtrs; ai++)
   {
      tr = TransplantFolderPair(GetPtr(hpaFolderPairs, ai), hpathOld,
                                hpathNew);

      if (tr != TR_SUCCESS)
         break;
   }

   if (tr == TR_SUCCESS)
   {
      HPTRARRAY hpaTwinFamilies;

      /* Restore folder pair array to sorted order. */

      SortPtrArray(hpaFolderPairs, &FolderPairSortCmp);

      TRACE_OUT((TEXT("MyTranslateFolder(): Sorted folder pair array after folder translation.")));

      /*
       * Change folders of all object twins in pcszOldFolder's old subtree to
       * being in pcszNewFolder's subtree.
       */

      hpaTwinFamilies = GetBriefcaseTwinFamilyPtrArray(hbr);

      aicPtrs = GetPtrCount(hpaTwinFamilies);

      for (ai = 0; ai < aicPtrs; ai++)
      {
         PTWINFAMILY ptf;
         BOOL bContinue;
         HNODE hnode;

         ptf = GetPtr(hpaTwinFamilies, ai);

         ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));

         /*
          * Walk each twin family's list of object twins looking for object
          * twins in the translated folder's subtree.
          */

         for (bContinue = GetFirstNode(ptf->hlistObjectTwins, &hnode);
              bContinue;
              bContinue = GetNextNode(hnode, &hnode))
         {
            POBJECTTWIN pot;

            pot = (POBJECTTWIN)GetNodeData(hnode);

            tr = TransplantObjectTwin(pot, hpathOld, hpathNew);

            if (tr != TR_SUCCESS)
               break;
         }

         if (tr != TR_SUCCESS)
            break;
      }

      /* Twin family array is still in sorted order. */
   }

   return(tr);
}


/*
** ApplyNewObjectTwinsToFolderTwins()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  Adds new spin-off object twins to hlistNewObjectTwins as they
**                are created.
**
** N.b., new object twins may have been added to hlistNewObjectTwins even if
** FALSE is returned.  Clean-up of these new object twins in case of failure is
** the caller's responsibility.
**
** A new object twin may generate more new object twins, implied by existing
** folder twins.  E.g., consider the following scenario:
**
** 1) Folder twins (c:\, d:\, *.*) and (d:\, e:\, *.*) exist.
** 2) Files c:\foo, d:\foo, and e:\foo do not exist.
** 3) File e:\foo is created.
** 4) New object twin e:\foo is added.
** 5) d:\foo must be added as a new object twin from the e:\foo object twin
**    because of the (d:\, e:\, *.*) folder twin.
** 6) c:\foo must be added as a new object twin from the d:\foo object twin
**    because of the (c:\, d:\, *.*) folder twin.
**
** So we see that new object twin e:\foo must generate two more new object
** twins, d:\foo and c:\foo, implied by the two existing folder twins,
** (c:\, d:\, *.*) and (d:\, e:\, *.*).
*/
PUBLIC_CODE BOOL ApplyNewObjectTwinsToFolderTwins(HLIST hlistNewObjectTwins)
{
   BOOL bResult = TRUE;
   BOOL bContinue;
   HNODE hnode;

   ASSERT(IS_VALID_HANDLE(hlistNewObjectTwins, LIST));

   /*
    * Don't use WalkList() here because we want to insert new nodes in
    * hlistNewObjectTwins after the current node.
    */

   for (bContinue = GetFirstNode(hlistNewObjectTwins, &hnode);
        bContinue && bResult;
        bContinue = GetNextNode(hnode, &hnode))
   {
      POBJECTTWIN pot;
      HPATHLIST hpl;
      HPTRARRAY hpaFolderPairs;
      ARRAYINDEX aicPtrs;
      ARRAYINDEX ai;

      pot = GetNodeData(hnode);

      ASSERT(! pot->ulcSrcFolderTwins);

      TRACE_OUT((TEXT("ApplyNewObjectTwinsToFolderTwins(): Applying new object twin %s\\%s."),
                 DebugGetPathString(pot->hpath),
                 GetString(pot->ptfParent->hsName)));

      /*
       * Assume that hpl, hpaFolderPairs, and aicPtrs don't change during this
       * loop.  Calculate them outside the loop.
       */

      hpl = GetBriefcasePathList(pot->ptfParent->hbr);
      hpaFolderPairs = GetBriefcaseFolderPairPtrArray(pot->ptfParent->hbr);

      aicPtrs = GetPtrCount(hpaFolderPairs);
      ASSERT(! (aicPtrs % 2));

      for (ai = 0; ai < aicPtrs; ai++)
      {
         PFOLDERPAIR pfp;

         pfp = GetPtr(hpaFolderPairs, ai);

         if (FolderTwinGeneratesObjectTwin(pfp, pot->hpath,
                                           GetString(pot->ptfParent->hsName)))
         {
            HPATH hpathMatchingFolder;
            HNODE hnodeUnused;

            ASSERT(pot->ulcSrcFolderTwins < ULONG_MAX);
            pot->ulcSrcFolderTwins++;

            /*
             * Append the generated object twin's subpath to the matching
             * folder twin's base path for subtree twins.
             */

            if (BuildPathForMatchingObjectTwin(pfp, pot, hpl,
                                               &hpathMatchingFolder))
            {
               /*
                * We don't want to collapse any twin families if the matching
                * object twin is found in a different twin family.  This will
                * be done by ApplyNewFolderTwinsToTwinFamilies() for spin-off
                * object twins generated by new folder twins.
                *
                * Spin-off object twins created by new object twins never
                * require collapsing twin families.  For a spin-off object twin
                * generated by a new object twin to collapse twin families,
                * there would have to have been separate twin families
                * connected by a folder twin.  But if those twin families were
                * already connected by a folder twin, they would not be
                * separate because they would already have been collapsed by
                * ApplyNewFolderTwinsToTwinFamilies() when the connecting
                * folder twin was added.
                */

               if (! FindObjectTwin(pot->ptfParent->hbr, hpathMatchingFolder,
                                    GetString(pot->ptfParent->hsName),
                                    &hnodeUnused))
               {
                  POBJECTTWIN potNew;

                  /*
                   * CreateObjectTwin() ASSERT()s that an object twin for
                   * hpathMatchingFolder is not found, so we don't need to do
                   * that here.
                   */

                  if (CreateObjectTwin(pot->ptfParent, hpathMatchingFolder,
                                       &potNew))
                  {
                     /*
                      * Add the new object twin to hlistNewObjectTwins after
                      * the new object twin currently being processed to make
                      * certain that it gets processed in the outside loop
                      * through hlistNewObjectTwins.
                      */

                     if (! InsertNodeAfter(hnode, NULL, potNew, &hnodeUnused))
                     {
                        DestroyStub(&(potNew->stub));
                        bResult = FALSE;
                        break;
                     }
                  }
               }

               DeletePath(hpathMatchingFolder);
            }
            else
            {
               bResult = FALSE;
               break;
            }
         }
      }
   }

   return(bResult);
}


/*
** BuildPathForMatchingObjectTwin()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  Path is added to object twin's briefcase's path list.
*/
PUBLIC_CODE BOOL BuildPathForMatchingObjectTwin(PCFOLDERPAIR pcfp,
                                                PCOBJECTTWIN pcot,
                                                HPATHLIST hpl, PHPATH phpath)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));
   ASSERT(IS_VALID_STRUCT_PTR(pcot, COBJECTTWIN));
   ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));
   ASSERT(IS_VALID_WRITE_PTR(phpath, HPATH));

   ASSERT(FolderTwinGeneratesObjectTwin(pcfp, pcot->hpath, GetString(pcot->ptfParent->hsName)));

   /* Is the generating folder twin a subtree twin? */

   if (IsStubFlagSet(&(pcfp->stub), STUB_FL_SUBTREE))
   {
      TCHAR rgchPathSuffix[MAX_PATH_LEN];
      LPCTSTR pcszSubPath;

      /*
       * Yes.  Append the object twin's subpath to the subtree twin's base
       * path.
       */

      pcszSubPath = FindChildPathSuffix(pcfp->hpath, pcot->hpath,
                                        rgchPathSuffix);

      bResult = AddChildPath(hpl, pcfp->pfpOther->hpath, pcszSubPath, phpath);
   }
   else
      /* No.  Just use the matching folder twin's folder. */
      bResult = CopyPath(pcfp->pfpOther->hpath, hpl, phpath);

   return(bResult);
}


/*
** EnumGeneratedObjectTwins()
**
**
**
** Arguments:
**
** Returns:       FALSE if callback aborted.  TRUE if not.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL EnumGeneratedObjectTwins(PCFOLDERPAIR pcfp,
                                     ENUMGENERATEDOBJECTTWINSPROC egotp,
                                     PVOID pvRefData)
{
   BOOL bResult = TRUE;
   HPTRARRAY hpaTwinFamilies;
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   /* pvRefData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));
   ASSERT(IS_VALID_CODE_PTR(egotp, ENUMGENERATEDOBJECTTWINPROC));

   /*
    * Walk the array of twin families, looking for twin families whose names
    * intersect the given folder twin's name specification.
    */

   hpaTwinFamilies = GetBriefcaseTwinFamilyPtrArray(pcfp->pfpd->hbr);

   aicPtrs = GetPtrCount(hpaTwinFamilies);
   ai = 0;

   while (ai < aicPtrs)
   {
      PTWINFAMILY ptf;
      LPCTSTR pcszName;

      ptf = GetPtr(hpaTwinFamilies, ai);

      ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));
      ASSERT(IsStubFlagClear(&(ptf->stub), STUB_FL_UNLINKED));

      /*
       * Does the twin family's name match the folder twin's name
       * specification?
       */

      pcszName = GetString(ptf->hsName);

      if (IsFolderObjectTwinName(pcszName) ||
          NamesIntersect(pcszName, GetString(pcfp->pfpd->hsName)))
      {
         BOOL bContinue;
         HNODE hnodePrev;

         /* Yes.  Look for a matching folder. */

         /* Lock the twin family so it isn't deleted out from under us. */

         LockStub(&(ptf->stub));

         /*
          * Walk each twin family's list of object twins looking for object
          * twins in the given folder twin's subtree.
          */

         bContinue = GetFirstNode(ptf->hlistObjectTwins, &hnodePrev);

         while (bContinue)
         {
            HNODE hnodeNext;
            POBJECTTWIN pot;

            bContinue = GetNextNode(hnodePrev, &hnodeNext);

            pot = (POBJECTTWIN)GetNodeData(hnodePrev);

            ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));

            if (FolderTwinIntersectsFolder(pcfp, pot->hpath))
            {
               /*
                * A given object twin should only be generated by one of the
                * folder twins in a pair of folder twins.
                */

               ASSERT(! FolderTwinGeneratesObjectTwin(pcfp->pfpOther, pot->hpath, GetString(pot->ptfParent->hsName)));

               bResult = (*egotp)(pot, pvRefData);

               if (! bResult)
                  break;
            }

            hnodePrev = hnodeNext;
         }

         /* Was the twin family unlinked? */

         if (IsStubFlagClear(&(ptf->stub), STUB_FL_UNLINKED))
            /* No. */
            ai++;
         else
         {
            /* Yes. */

            aicPtrs--;
            ASSERT(aicPtrs == GetPtrCount(hpaTwinFamilies));

            TRACE_OUT((TEXT("EnumGeneratedObjectTwins(): Twin family for object %s unlinked by callback."),
                       GetString(ptf->hsName)));
         }

         UnlockStub(&(ptf->stub));

         if (! bResult)
            break;
      }
      else
         /* No.  Skip it. */
         ai++;
   }

   return(bResult);
}


/*
** EnumGeneratingFolderTwins()
**
**
**
** Arguments:
**
** Returns:       FALSE if callback aborted.  TRUE if not.
**
** Side Effects:  none
**
** N.b., if the egftp callback removes a pair of folder twins, it must remove
** the pair from the first folder twin encountered.  If it removes the pair of
** folder twins from the second folder twin encountered, a folder twin will be
** skipped.
*/
PUBLIC_CODE BOOL EnumGeneratingFolderTwins(PCOBJECTTWIN pcot,
                                           ENUMGENERATINGFOLDERTWINSPROC egftp,
                                           PVOID pvRefData,
                                           PULONG pulcGeneratingFolderTwins)
{
   BOOL bResult = TRUE;
   HPTRARRAY hpaFolderPairs;
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   /* pvRefData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pcot, COBJECTTWIN));
   ASSERT(IS_VALID_CODE_PTR(egftp, ENUMGENERATINGFOLDERTWINSPROC));
   ASSERT(IS_VALID_WRITE_PTR(pulcGeneratingFolderTwins, ULONG));

   *pulcGeneratingFolderTwins = 0;

   hpaFolderPairs = GetBriefcaseFolderPairPtrArray(pcot->ptfParent->hbr);

   aicPtrs = GetPtrCount(hpaFolderPairs);
   ASSERT(! (aicPtrs % 2));

   ai = 0;

   while (ai < aicPtrs)
   {
      PFOLDERPAIR pfp;

      pfp = GetPtr(hpaFolderPairs, ai);

      if (FolderTwinGeneratesObjectTwin(pfp, pcot->hpath,
                                        GetString(pcot->ptfParent->hsName)))
      {
         ASSERT(! FolderTwinGeneratesObjectTwin(pfp->pfpOther, pcot->hpath, GetString(pcot->ptfParent->hsName)));

         ASSERT(*pulcGeneratingFolderTwins < ULONG_MAX);
         (*pulcGeneratingFolderTwins)++;

         /*
          * Lock the pair of folder twins so they don't get deleted out from
          * under us.
          */

         LockStub(&(pfp->stub));

         bResult = (*egftp)(pfp, pvRefData);

         if (IsStubFlagSet(&(pfp->stub), STUB_FL_UNLINKED))
         {
            WARNING_OUT((TEXT("EnumGeneratingFolderTwins(): Folder twin pair unlinked during callback.")));

            aicPtrs -= 2;
            ASSERT(! (aicPtrs % 2));
            ASSERT(aicPtrs == GetPtrCount(hpaFolderPairs));
         }
         else
            ai++;

         UnlockStub(&(pfp->stub));

         if (! bResult)
            break;
      }
      else
         ai++;
   }

   return(bResult);
}


/*
** FolderTwinGeneratesObjectTwin()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** A folder twin or subtree twin is said to generate an object twin when the
** following conditions are met:
**
** 1) The folder twin or subtree twin is on the same volume as the object twin.
**
** 2) The name of the object twin (literal) intersects the objects matched by
**    the folder twin or subtree twin (literal or wildcard).
**
** 3) The folder twin's folder exactly matches the object twin's folder, or the
**    subtree twin's root folder is a path prefix of the object twin's folder.
*/
PUBLIC_CODE BOOL FolderTwinGeneratesObjectTwin(PCFOLDERPAIR pcfp,
                                               HPATH hpathFolder,
                                               LPCTSTR pcszName)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));
   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));

   return(FolderTwinIntersectsFolder(pcfp, hpathFolder) &&
          (IsFolderObjectTwinName(pcszName) ||
           NamesIntersect(pcszName, GetString(pcfp->pfpd->hsName))));
}


/*
** IsValidHFOLDERTWIN()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHFOLDERTWIN(HFOLDERTWIN hft)
{
   return(IS_VALID_STRUCT_PTR((PFOLDERPAIR)hft, CFOLDERPAIR));
}


#ifdef VSTF

/*
** IsValidPCFOLDERPAIR()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCFOLDERPAIR(PCFOLDERPAIR pcfp)
{
   BOOL bResult = FALSE;

   /* All the fields of an unlinked folder pair should be valid. */

   if (EVAL(IsValidFolderPairHalf(pcfp)))
   {
      if (IsStubFlagSet(&(pcfp->stub), STUB_FL_BEING_DELETED))
         bResult = TRUE;
      else if (EVAL(IsValidFolderPairHalf(pcfp->pfpOther)) &&
               EVAL(pcfp->pfpOther->pfpOther == pcfp) &&
               EVAL(pcfp->pfpd == pcfp->pfpOther->pfpd) &&
               EVAL(pcfp->stub.ulcLock == pcfp->pfpOther->stub.ulcLock))
      {
         BOOL bUnlinked;
         BOOL bOtherUnlinked;

         /*
          * Neither or both folder pair halves may be unlinked, but not only
          * one.
          */

         bUnlinked = IsStubFlagSet(&(pcfp->stub), STUB_FL_UNLINKED);
         bOtherUnlinked = IsStubFlagSet(&(pcfp->pfpOther->stub), STUB_FL_UNLINKED);

         if (EVAL((bUnlinked && bOtherUnlinked) ||
                  (! bUnlinked && ! bOtherUnlinked)))
            bResult = TRUE;
      }
   }

   return(bResult);
}

#endif


/*
** WriteFolderPairList()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT WriteFolderPairList(HCACHEDFILE hcf,
                                      HPTRARRAY hpaFolderPairs)
{
   TWINRESULT tr = TR_BRIEFCASE_WRITE_FAILED;
   DWORD dwcbDBFolderTwinListHeaderOffset;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hpaFolderPairs, PTRARRAY));

   /* Save initial file position. */

   dwcbDBFolderTwinListHeaderOffset = GetCachedFilePointerPosition(hcf);

   if (dwcbDBFolderTwinListHeaderOffset != INVALID_SEEK_POSITION)
   {
      DBFOLDERTWINLISTHEADER dbftlh;

      /* Leave space for folder twin data header. */

      ZeroMemory(&dbftlh, sizeof(dbftlh));

      if (WriteToCachedFile(hcf, (PCVOID)&dbftlh, sizeof(dbftlh), NULL))
      {
         ARRAYINDEX aicPtrs;
         ARRAYINDEX ai;

         tr = TR_SUCCESS;

         /* Mark all folder pairs unused. */

         ClearFlagInArrayOfStubs(hpaFolderPairs, STUB_FL_USED);

         aicPtrs = GetPtrCount(hpaFolderPairs);
         ASSERT(! (aicPtrs % 2));

         /* Write all folder pairs. */

         for (ai = 0; ai < aicPtrs; ai++)
         {
            PFOLDERPAIR pfp;

            pfp = GetPtr(hpaFolderPairs, ai);

            ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

            if (IsStubFlagClear(&(pfp->stub), STUB_FL_USED))
            {
               ASSERT(IsStubFlagClear(&(pfp->pfpOther->stub), STUB_FL_USED));

               tr = WriteFolderPair(hcf, pfp);

               if (tr == TR_SUCCESS)
               {
                  SetStubFlag(&(pfp->stub), STUB_FL_USED);
                  SetStubFlag(&(pfp->pfpOther->stub), STUB_FL_USED);
               }
               else
                  break;
            }
         }

         /* Save folder twin data header. */

         if (tr == TR_SUCCESS)
         {
            ASSERT(! (aicPtrs % 2));

            dbftlh.lcFolderPairs = aicPtrs / 2;

            tr = WriteDBSegmentHeader(hcf, dwcbDBFolderTwinListHeaderOffset,
                                      &dbftlh, sizeof(dbftlh));

            if (tr == TR_SUCCESS)
               TRACE_OUT((TEXT("WriteFolderPairList(): Wrote %ld folder pairs."),
                          dbftlh.lcFolderPairs));
         }
      }
   }

   return(tr);
}


/*
** ReadFolderPairList()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT ReadFolderPairList(HCACHEDFILE hcf, HBRFCASE hbr,
                                     HHANDLETRANS hhtFolderTrans,
                                     HHANDLETRANS hhtNameTrans)
{
   TWINRESULT tr;
   DBFOLDERTWINLISTHEADER dbftlh;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hhtFolderTrans, HANDLETRANS));
   ASSERT(IS_VALID_HANDLE(hhtNameTrans, HANDLETRANS));

   if (ReadFromCachedFile(hcf, &dbftlh, sizeof(dbftlh), &dwcbRead) &&
       dwcbRead == sizeof(dbftlh))
   {
      LONG l;

      tr = TR_SUCCESS;

      TRACE_OUT((TEXT("ReadFolderPairList(): Reading %ld folder pairs."),
                 dbftlh.lcFolderPairs));

      for (l = 0; l < dbftlh.lcFolderPairs && tr == TR_SUCCESS; l++)
         tr = ReadFolderPair(hcf, hbr, hhtFolderTrans, hhtNameTrans);

      ASSERT(tr != TR_SUCCESS || AreFolderPairsValid(GetBriefcaseFolderPairPtrArray(hbr)));
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


/***************************** Exported Functions ****************************/


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | AddFolderTwin | Twins two folders.

@parm HBRFCASE | hbr | A handle to the open briefcase that the new folder twins
are to be added to.

@parm PCNEWFOLDERTWIN | pcnft | A pointer to a CNEWFOLDERTWIN describing the
two folders to be twinned.

@parm PHFOLDERTWIN | phft | A pointer to an HFOLDERTWIN to be filled in with
a handle to the new folder twins.  *phft is only valid if TR_SUCCESS or
TR_DUPLICATE_TWIN is returned.

@rdesc If the folder twins were added successfully, TR_SUCCESS is returned, and
*phFolderTwin contains a handle to the new folder twins.  Otherwise, the
folder twins were not added successfully, the return value indicates the error
that occurred, and *phFolderTwin is undefined.  If one or both of the volumes
specified by the NEWFOLDERTWIN structure is not present, TR_UNAVAILABLE_VOLUME
will be returned, and the folder twin will not be added.


@comm Once the caller is finshed with the twin handle returned by
AddFolderTwin(), ReleaseTwinHandle() should be called to release the twin
handle.  N.b., DeleteTwin() does not release a twin handle returned by
AddFolderTwin().

@xref ReleaseTwinHandle DeleteTwin

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI AddFolderTwin(HBRFCASE hbr, PCNEWFOLDERTWIN pcnft,
                                           PHFOLDERTWIN phft)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(AddFolderTwin);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbr, BRFCASE) &&
          IS_VALID_STRUCT_PTR(pcnft, CNEWFOLDERTWIN) &&
          EVAL(pcnft->ulSize == sizeof(*pcnft)) &&
          IS_VALID_WRITE_PTR(phft, HFOLDERTWIN))
#endif
      {
         INEWFOLDERTWIN inft;

         InvalidatePathListInfo(GetBriefcasePathList(hbr));

         tr = MakeINewFolderTwin(hbr, pcnft, &inft);

         if (tr == TR_SUCCESS)
         {
            PFOLDERPAIR pfp;

            ASSERT(! IS_ATTR_DIR(pcnft->dwAttributes));

            tr = TwinFolders(&inft, &pfp);

            if (tr == TR_SUCCESS ||
                tr == TR_DUPLICATE_TWIN)
            {
               LockStub(&(pfp->stub));

               *phft = (HFOLDERTWIN)pfp;
            }

            ReleaseINewFolderTwin(&inft);
         }
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(AddFolderTwin, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | IsFolderTwin | Determines whether or not a folder is a
   folder twin.

@parm HBRFCASE | hbr | A handle to the open briefcase to check for the folder
twin.

@parm PCSTR | pcszFolder | A pointer to a string indicating the folder in
question.

@parm PBOOL | pbIsFolderTwin | A pointer to a BOOL to be filled in with TRUE
if the folder is a folder twin, or FALSE if not.  *pbIsFolderTwin is only
valid if TR_SUCCESS is returned.

@rdesc If the lookup was successful, TR_SUCCESS is returned.  Otherwise, the
lookup was not successful, and the return value indicates the error that
occurred.

@xref CreateFolderTwinList DestroyFolderTwinList

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI IsFolderTwin(HBRFCASE hbr, LPCTSTR pcszFolder,
                                          PBOOL pbIsFolderTwin)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(IsFolderTwin);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbr, BRFCASE) &&
          IS_VALID_STRING_PTR(pcszFolder, CSTR) &&
          IS_VALID_WRITE_PTR(pbIsFolderTwin, BOOL))
#endif
      {
         HPATH hpath;

         InvalidatePathListInfo(GetBriefcasePathList(hbr));

         tr = TranslatePATHRESULTToTWINRESULT(
               AddPath(GetBriefcasePathList(hbr), pcszFolder, &hpath));

         if (tr == TR_SUCCESS)
         {
            ARRAYINDEX aiFirst;

            /* Search for folder pair referencing given folder. */

            *pbIsFolderTwin = SearchSortedArray(
                                       GetBriefcaseFolderPairPtrArray(hbr),
                                       &FolderPairSearchCmp, hpath, &aiFirst);

            if (*pbIsFolderTwin)
               TRACE_OUT((TEXT("IsFolderTwin(): %s is a folder twin."),
                          DebugGetPathString(hpath)));
            else
               TRACE_OUT((TEXT("IsFolderTwin(): %s is not a folder twin."),
                          DebugGetPathString(hpath)));

            DeletePath(hpath);
         }
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(IsFolderTwin, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | CreateFolderTwinList | Creates a list of the folder twins of
a given folder.

@parm HBRFCASE | hbr | A handle to the open briefcase that the folder twin list
is to be created from.

@parm PCSTR | pcszFolder | A pointer to a string indicating the folder whose
folder twins are to be listed.

@parm PFOLDERTWINLIST | ppftl | A pointer to an PFOLDERTWINLIST to be
filled in with a pointer to the new list of folder twins.  *ppFolderTwinList
is only valid if TR_SUCCESS is returned.

@rdesc If the folder twin list was created successfully, TR_SUCCESS is
returned.  Otherwise, the folder twin list was not created successfully, and
the return value indicates the error that occurred.

@xref DestroyFolderTwinList IsFolderTwin

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI CreateFolderTwinList(HBRFCASE hbr,
                                                  LPCTSTR pcszFolder,
                                                  PFOLDERTWINLIST *ppftl)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(CreateFolderTwinList);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbr, BRFCASE) &&
          IS_VALID_STRING_PTR(pcszFolder, CSTR) &&
          IS_VALID_WRITE_PTR(ppftl, PFOLDERTWINLIST))
#endif
      {
         HPATH hpath;

         InvalidatePathListInfo(GetBriefcasePathList(hbr));

         tr = TranslatePATHRESULTToTWINRESULT(
               AddPath(GetBriefcasePathList(hbr), pcszFolder, &hpath));

         if (tr == TR_SUCCESS)
         {
            PFOLDERTWINLIST pftlNew;

            /* Try to create a new folder twin list. */

            if (AllocateMemory(sizeof(*pftlNew), &pftlNew))
            {
               ARRAYINDEX ai;

               /* Initialize FOLDERTWINLIST structure fields. */

               pftlNew->ulcItems = 0;
               pftlNew->pcftFirst = NULL;
               pftlNew->hbr = hbr;

               /* Search for first folder pair referencing given folder. */

               if (SearchSortedArray(GetBriefcaseFolderPairPtrArray(hbr),
                                     &FolderPairSearchCmp, hpath, &ai))
               {
                  PFOLDERTWIN pftHead;
                  ARRAYINDEX aicFolderTwins;

                  tr = CreateListOfFolderTwins(hbr, ai, hpath, &pftHead, &aicFolderTwins);

                  if (tr == TR_SUCCESS)
                  {
                     /* Success!  Update parent folder twin list fields. */

                     pftlNew->pcftFirst = pftHead;
                     pftlNew->ulcItems = aicFolderTwins;
                  }
                  else
                     /* Free data structure, ignoring return value. */
                     FreeMemory(pftlNew);
               }
               else
                  tr = TR_SUCCESS;

               /* Return pointer to new FOLDERTWINLIST. */

               if (tr == TR_SUCCESS)
               {
                  *ppftl = pftlNew;

                  ASSERT(IS_VALID_STRUCT_PTR(*ppftl, CFOLDERTWINLIST));
               }
            }
            else
               tr = TR_OUT_OF_MEMORY;

            DeletePath(hpath);
         }
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(CreateFolderTwinList, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | DestroyFolderTwinList | Destroys a folder twin list created
by CreateFolderTwinList().

@parm PFOLDERTWINLIST | pftl | A pointer to the folder twin list to be
destroyed.  The FOLDERTWINLIST pointed to by pftl is not valid after
DestroyFolderTwinList() is called.

@rdesc If the folder twin list was deleted successfully, TR_SUCCESS is
returned.  Otherwise, the folder twin list was not deleted successfully, and
the return value indicates the error that occurred.

@xref CreateFolderTwinList IsFolderTwin

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI DestroyFolderTwinList(PFOLDERTWINLIST pftl)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(DestroyFolderTwinList);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_STRUCT_PTR(pftl, CFOLDERTWINLIST))
#endif
      {
         DestroyListOfFolderTwins((PFOLDERTWIN)(pftl->pcftFirst));
         FreeMemory(pftl);

         tr = TR_SUCCESS;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(DestroyFolderTwinList, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}

