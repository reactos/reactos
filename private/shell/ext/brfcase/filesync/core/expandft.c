/*
 * expandft.c - Routines for expanding folder twins to object twins.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "stub.h"


/* Constants
 ************/

/* for subtree folder searching */

#define STAR_DOT_STAR            TEXT("*.*")


/* Macros
 *********/

/* name component macros used by NameComponentsIntersect() */

#define COMPONENT_CHARS_MATCH(ch1, ch2)   (CharLower((PTSTR)(DWORD)ch1) == CharLower((PTSTR)(DWORD)ch2) || (ch1) == QMARK || (ch2) == QMARK)

#define IS_COMPONENT_TERMINATOR(ch)       (! (ch) || (ch) == PERIOD || (ch) == ASTERISK)


/* Types
 ********/

/* find structure used by ExpandSubtree() */

typedef struct _findstate
{
   HANDLE hff;

   WIN32_FIND_DATA wfd;
}
FINDSTATE;
DECLARE_STANDARD_TYPES(FINDSTATE);

/* information structure passed to GenerateObjectTwinFromFolderTwinProc() */

typedef struct _expandsubtreetwininfo
{
   PFOLDERPAIR pfp;

   UINT ucbSubtreeRootPathLen;

   HCLSIFACECACHE hcic;

   CREATERECLISTPROC crlp;

   LPARAM lpCallbackData;

   TWINRESULT tr;
}
EXPANDSUBTREETWININFO;
DECLARE_STANDARD_TYPES(EXPANDSUBTREETWININFO);


/* Module Variables
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

/*
 * folder names to be avoided during subtree expansion (comparison is
 * case-insensitive)
 */

PRIVATE_DATA CONST LPCTSTR MrgcpcszFoldersToAvoid[] =
{
   TEXT("."),
   TEXT("..")
};

#pragma data_seg()


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE BOOL SetObjectTwinFileStamp(POBJECTTWIN, PVOID);
PRIVATE_CODE void MarkFolderTwinDeletionPending(PFOLDERPAIR);
PRIVATE_CODE void UnmarkFolderTwinDeletionPending(PFOLDERPAIR);
PRIVATE_CODE TWINRESULT ExpandFolderTwin(PFOLDERPAIR, HCLSIFACECACHE, CREATERECLISTPROC, LPARAM);
PRIVATE_CODE BOOL GenerateObjectTwinFromFolderTwinProc(LPCTSTR, PCWIN32_FIND_DATA, PVOID);
PRIVATE_CODE TWINRESULT ExpandSubtreeTwin(PFOLDERPAIR, HCLSIFACECACHE, CREATERECLISTPROC, LPARAM);
PRIVATE_CODE BOOL IsFolderToExpand(LPCTSTR);
PRIVATE_CODE TWINRESULT FakeObjectTwinFromFolderTwin(PCFOLDERPAIR, LPCTSTR, LPCTSTR, HCLSIFACECACHE, POBJECTTWIN *, POBJECTTWIN *);
PRIVATE_CODE TWINRESULT AddFolderObjectTwinFromFolderTwin(PCFOLDERPAIR, LPCTSTR, HCLSIFACECACHE);
PRIVATE_CODE TWINRESULT AddFileObjectTwinFromFolderTwin(PCFOLDERPAIR, LPCTSTR, PCWIN32_FIND_DATA, HCLSIFACECACHE);
PRIVATE_CODE BOOL NameComponentsIntersect(LPCTSTR, LPCTSTR);
PRIVATE_CODE BOOL AttributesMatch(DWORD, DWORD);
PRIVATE_CODE void PrepareForFolderTwinExpansion(HBRFCASE);
PRIVATE_CODE TWINRESULT MyExpandIntersectingFolderTwins(PFOLDERPAIR, HCLSIFACECACHE, CREATERECLISTPROC, LPARAM);
PRIVATE_CODE TWINRESULT HalfExpandIntersectingFolderTwins(PFOLDERPAIR, HCLSIFACECACHE, CREATERECLISTPROC, LPARAM);

#ifdef DEBUG

PRIVATE_CODE BOOL IsValidPCEXPANDSUBTREETWININFO(PCEXPANDSUBTREETWININFO);

#endif


/*
** SetObjectTwinFileStampCondition()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL SetObjectTwinFileStampCondition(POBJECTTWIN pot,
                                                  PVOID fscond)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(IsValidFILESTAMPCONDITION((FILESTAMPCONDITION)PtrToUlong(fscond)));

   ZeroMemory(&(pot->fsCurrent), sizeof(pot->fsCurrent));
   pot->fsCurrent.fscond = (FILESTAMPCONDITION)PtrToUlong(fscond);

   SetStubFlag(&(pot->stub), STUB_FL_FILE_STAMP_VALID);

   return(TRUE);
}


/*
** MarkFolderTwinDeletionPending()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void MarkFolderTwinDeletionPending(PFOLDERPAIR pfp)
{
   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   if (IsStubFlagClear(&(pfp->stub), STUB_FL_DELETION_PENDING))
   {
      TCHAR rgchRootPath[MAX_PATH_LEN];

      GetPathRootString(pfp->hpath, rgchRootPath);

      if (PathExists(rgchRootPath))
      {
         SetStubFlag(&(pfp->stub), STUB_FL_DELETION_PENDING);

         TRACE_OUT((TEXT("MarkFolderTwinDeletionPending(): Folder twin deletion pending for deleted folder %s."),
                    DebugGetPathString(pfp->hpath)));
      }
      else
         WARNING_OUT((TEXT("MarkFolderTwinDeletionPending(): Root path %s of folder %s does not exist."),
                      rgchRootPath,
                      DebugGetPathString(pfp->hpath)));
   }

   return;
}


/*
** UnmarkFolderTwinDeletionPending()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void UnmarkFolderTwinDeletionPending(PFOLDERPAIR pfp)
{
   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   if (IsStubFlagSet(&(pfp->stub), STUB_FL_DELETION_PENDING))
      WARNING_OUT((TEXT("UnmarkFolderTwinDeletionPending(): Folder twin %s was deleted but has been recreated."),
                   DebugGetPathString(pfp->hpath)));

   ClearStubFlag(&(pfp->stub), STUB_FL_DELETION_PENDING);

   return;
}


/*
** ExpandFolderTwin()
**
** Expands a single folder of half of a folder pair into object twins.
**
** Arguments:     pfp - pointer to folder pair whose folder is to be expanded
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT ExpandFolderTwin(PFOLDERPAIR pfp, HCLSIFACECACHE hcic,
                                         CREATERECLISTPROC crlp,
                                         LPARAM lpCallbackData)
{
   TWINRESULT tr = TR_SUCCESS;
   TCHAR rgchSearchSpec[MAX_PATH_LEN];

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));
   ASSERT(IS_VALID_HANDLE(hcic, CLSIFACECACHE));
   ASSERT(! crlp ||
          IS_VALID_CODE_PTR(crlp, CREATERECLISTPROC));

   ASSERT(IsPathVolumeAvailable(pfp->hpath));
   ASSERT(IsStubFlagClear(&(pfp->stub), STUB_FL_SUBTREE));
   ASSERT(IsStubFlagClear(&(pfp->stub), STUB_FL_USED));

   /* Build search specification. */

   GetPathString(pfp->hpath, rgchSearchSpec);

   if (PathExists(rgchSearchSpec))
   {
      WIN32_FIND_DATA wfd;
      HANDLE hff;

      UnmarkFolderTwinDeletionPending(pfp);

      TRACE_OUT((TEXT("ExpandFolderTwin(): Expanding folder %s for objects matching %s."),
                 rgchSearchSpec,
                 GetString(pfp->pfpd->hsName)));

      tr = AddFolderObjectTwinFromFolderTwin(pfp, EMPTY_STRING, hcic);

      if (tr == TR_SUCCESS)
      {
         CatPath(rgchSearchSpec, GetString(pfp->pfpd->hsName));

         hff = FindFirstFile(rgchSearchSpec, &wfd);

         /* Did we find a matching object? */

         if (hff != INVALID_HANDLE_VALUE)
         {
            /* Yes. */

            do
            {
               /* Ping. */

               if (NotifyCreateRecListStatus(crlp, CRLS_DELTA_CREATE_REC_LIST,
                                             0, lpCallbackData))
               {
                  if (AttributesMatch(pfp->pfpd->dwAttributes,
                                      wfd.dwFileAttributes))
                  {
                     TRACE_OUT((TEXT("ExpandFolderTwin(): Found matching object %s."),
                                &(wfd.cFileName)));

                     tr = AddFileObjectTwinFromFolderTwin(pfp, EMPTY_STRING,
                                                          &wfd, hcic);

                     if (tr != TR_SUCCESS)
                        break;
                  }
               }
               else
                  tr = TR_ABORT;

            } while (FindNextFile(hff, &wfd));
         }

         if (hff != INVALID_HANDLE_VALUE)
            FindClose(hff);
      }

      TRACE_OUT((TEXT("ExpandFolderTwin(): Folder expansion complete.")));
   }
   else
      MarkFolderTwinDeletionPending(pfp);

   return(tr);
}


/*
** GenerateObjectTwinFromFolderTwinProc()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL GenerateObjectTwinFromFolderTwinProc(LPCTSTR pcszFolder,
                                                       PCWIN32_FIND_DATA pcwfd,
                                                       PVOID pvpesti)
{
   TWINRESULT tr;
   PEXPANDSUBTREETWININFO pesti = pvpesti;

   ASSERT(IsCanonicalPath(pcszFolder));
   ASSERT(IS_VALID_READ_PTR(pcwfd, CWIN32_FIND_DATA));
   ASSERT(IS_VALID_STRUCT_PTR(pesti, CEXPANDSUBTREETWININFO));

   /* Ping. */

   if (NotifyCreateRecListStatus(pesti->crlp, CRLS_DELTA_CREATE_REC_LIST, 0,
                                 pesti->lpCallbackData))
   {
      if (IS_ATTR_DIR(pcwfd->dwFileAttributes))
      {
         TCHAR rgchFolder[MAX_PATH_LEN];

         /* Add any folder as a folder object twin. */

         ComposePath(rgchFolder, pcszFolder, pcwfd->cFileName);
         ASSERT(lstrlen(rgchFolder) < ARRAYSIZE(rgchFolder));

         tr = AddFolderObjectTwinFromFolderTwin(
                                    pesti->pfp,
                                    rgchFolder + (pesti->ucbSubtreeRootPathLen / sizeof(TCHAR)),
                                    pesti->hcic);
      }
      else
      {
         /* Does this file match the requested attributes? */

         if (NamesIntersect(pcwfd->cFileName,
                            GetString(pesti->pfp->pfpd->hsName)) &&
             AttributesMatch(pesti->pfp->pfpd->dwAttributes,
                             pcwfd->dwFileAttributes))
         {
            /* Yes.  Twin it. */

            TRACE_OUT((TEXT("GenerateObjectTwinFromFolderTwinProc(): Found matching object %s in subfolder %s."),
                       pcwfd->cFileName,
                       pcszFolder));

            tr = AddFileObjectTwinFromFolderTwin(
                                    pesti->pfp,
                                    pcszFolder + (pesti->ucbSubtreeRootPathLen / sizeof(TCHAR)),
                                    pcwfd, pesti->hcic);
         }
         else
         {
            TRACE_OUT((TEXT("GenerateObjectTwinFromFolderTwinProc(): Skipping unmatched object %s in subfolder %s."),
                       pcwfd->cFileName,
                       pcszFolder));

            tr = TR_SUCCESS;
         }
      }
   }
   else
      tr = TR_ABORT;

   pesti->tr = tr;

   ASSERT(IS_VALID_STRUCT_PTR(pvpesti, CEXPANDSUBTREETWININFO));

   return(tr == TR_SUCCESS);
}


/*
** ExpandSubtreeTwin()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT ExpandSubtreeTwin(PFOLDERPAIR pfp, HCLSIFACECACHE hcic,
                                          CREATERECLISTPROC crlp,
                                          LPARAM lpCallbackData)
{
   TWINRESULT tr = TR_SUCCESS;
   TCHAR rgchPath[MAX_PATH_LEN];

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));
   ASSERT(IS_VALID_HANDLE(hcic, CLSIFACECACHE));
   ASSERT(! crlp ||
          IS_VALID_CODE_PTR(crlp, CREATERECLISTPROC));

   ASSERT(IsPathVolumeAvailable(pfp->hpath));
   ASSERT(IsStubFlagSet(&(pfp->stub), STUB_FL_SUBTREE));
   ASSERT(IsStubFlagClear(&(pfp->stub), STUB_FL_USED));

   GetPathString(pfp->hpath, rgchPath);

   if (PathExists(rgchPath))
   {
      UnmarkFolderTwinDeletionPending(pfp);

      tr = AddFolderObjectTwinFromFolderTwin(pfp, EMPTY_STRING, hcic);

      if (tr == TR_SUCCESS)
      {
         EXPANDSUBTREETWININFO esti;

         esti.pfp = pfp;
         esti.ucbSubtreeRootPathLen = lstrlen(rgchPath) * sizeof(TCHAR); // UNICODE really cb?
         esti.hcic = hcic;
         esti.crlp = crlp;
         esti.lpCallbackData = lpCallbackData;
         esti.tr = TR_SUCCESS;

         tr = ExpandSubtree(pfp->hpath, &GenerateObjectTwinFromFolderTwinProc,
                            &esti);

         ASSERT(tr != TR_SUCCESS ||
                esti.tr == TR_SUCCESS);

         if (tr == TR_SUCCESS ||
             tr == TR_ABORT)
            tr = esti.tr;
      }
   }
   else
      MarkFolderTwinDeletionPending(pfp);

   return(tr);
}


/*
** IsFolderToExpand()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsFolderToExpand(LPCTSTR pcszFolder)
{
   BOOL bExpandMe = TRUE;
   int i;

   for (i = 0; i < ARRAY_ELEMENTS(MrgcpcszFoldersToAvoid); i++)
   {
      if (ComparePathStrings(pcszFolder, MrgcpcszFoldersToAvoid[i])
          == CR_EQUAL)
      {
         bExpandMe = FALSE;
         break;
      }
   }

   return(bExpandMe);
}


/*
** FakeObjectTwinFromFolderTwin()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT FakeObjectTwinFromFolderTwin(PCFOLDERPAIR pcfp,
                                                     LPCTSTR pcszSubPath,
                                                     LPCTSTR pcszName,
                                                     HCLSIFACECACHE hcic,
                                                     POBJECTTWIN *ppot1,
                                                     POBJECTTWIN *ppot2)
{
   TWINRESULT tr = TR_OUT_OF_MEMORY;
   HPATHLIST hpl;
   HPATH hpath1;

   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));
   ASSERT(IS_VALID_STRING_PTR(pcszSubPath, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));
   ASSERT(IS_VALID_HANDLE(hcic, CLSIFACECACHE));
   ASSERT(IS_VALID_WRITE_PTR(ppot1, POBJECTTWIN));
   ASSERT(IS_VALID_WRITE_PTR(ppot2, POBJECTTWIN));

   /* If the common sub path is non-empty, append it to the path strings. */

   hpl = GetBriefcasePathList(pcfp->pfpd->hbr);

   if (AddChildPath(hpl, pcfp->hpath, pcszSubPath, &hpath1))
   {
      HPATH hpath2;

      if (AddChildPath(hpl, pcfp->pfpOther->hpath, pcszSubPath, &hpath2))
      {
         /* Add the two object twins. */

         tr = TwinObjects(pcfp->pfpd->hbr, hcic, hpath1, hpath2, pcszName,
                          ppot1, ppot2);

         DeletePath(hpath2);
      }

      DeletePath(hpath1);
   }

   return(tr);
}


/*
** AddFolderObjectTwinFromFolderTwin()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT AddFolderObjectTwinFromFolderTwin(PCFOLDERPAIR pcfp,
                                                          LPCTSTR pcszSubPath,
                                                          HCLSIFACECACHE hcic)
{
   TWINRESULT tr;
   POBJECTTWIN pot1;
   POBJECTTWIN pot2;

   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));
   ASSERT(IS_VALID_STRING_PTR(pcszSubPath, CSTR));
   ASSERT(IS_VALID_HANDLE(hcic, CLSIFACECACHE));

   /* Add the two object twins. */

   tr = FakeObjectTwinFromFolderTwin(pcfp, pcszSubPath, EMPTY_STRING, hcic,
                                     &pot1, &pot2);

   /* An attempted redundant add is ok. */

   if (tr == TR_DUPLICATE_TWIN)
      tr = TR_SUCCESS;

   if (tr == TR_SUCCESS)
      /* Cache folder object twin file stamps. */
      SetObjectTwinFileStampCondition(pot1, (PVOID)FS_COND_EXISTS);

   return(tr);
}


/*
** AddFileObjectTwinFromFolderTwin()
**
** Adds a pair of object twins generated by a folder twin.
**
** Arguments:     pfp - pointer to folder pair that generated the two object
**                      twins
**                pcszSubPath - common path off of folder pair roots describing
**                              object's location
**                pcszName - name of object twins
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT AddFileObjectTwinFromFolderTwin(PCFOLDERPAIR pcfp,
                                                    LPCTSTR pcszSubPath,
                                                    PCWIN32_FIND_DATA pcwfd,
                                                    HCLSIFACECACHE hcic)
{
   TWINRESULT tr;
   POBJECTTWIN pot1;
   POBJECTTWIN pot2;

   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));
   ASSERT(IS_VALID_STRING_PTR(pcszSubPath, CSTR));
   ASSERT(IS_VALID_READ_PTR(pcwfd, CWIN32_FIND_DATA));
   ASSERT(IS_VALID_HANDLE(hcic, CLSIFACECACHE));

   /* Add the two object twins. */

   tr = FakeObjectTwinFromFolderTwin(pcfp, pcszSubPath, pcwfd->cFileName, hcic,
                                     &pot1, &pot2);

   /* An attempted redundant add is ok. */

   if (tr == TR_DUPLICATE_TWIN)
      tr = TR_SUCCESS;

   if (tr == TR_SUCCESS)
   {
      /* Cache object twin file stamp. */

      CopyFileStampFromFindData(pcwfd, &(pot1->fsCurrent));

      SetStubFlag(&(pot1->stub), STUB_FL_FILE_STAMP_VALID);
   }

   return(tr);
}


/*
** NameComponentsIntersect()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL NameComponentsIntersect(LPCTSTR pcszComponent1,
                                          LPCTSTR pcszComponent2)
{
   BOOL bIntersect;

   ASSERT(IS_VALID_STRING_PTR(pcszComponent1, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszComponent2, CSTR));

   while (! IS_COMPONENT_TERMINATOR(*pcszComponent1) && ! IS_COMPONENT_TERMINATOR(*pcszComponent2) &&
          COMPONENT_CHARS_MATCH(*pcszComponent1, *pcszComponent2))
   {
      pcszComponent1 = CharNext(pcszComponent1);
      pcszComponent2 = CharNext(pcszComponent2);
   }

   if (*pcszComponent1 == ASTERISK ||
       *pcszComponent2 == ASTERISK ||
       *pcszComponent1 == *pcszComponent2)
      bIntersect = TRUE;
   else
   {
      LPCTSTR pcszTrailer;

      if (! *pcszComponent1 || *pcszComponent1 == PERIOD)
         pcszTrailer = pcszComponent2;
      else
         pcszTrailer = pcszComponent1;

      while (*pcszTrailer == QMARK)
         pcszTrailer++;

      if (IS_COMPONENT_TERMINATOR(*pcszTrailer))
         bIntersect = TRUE;
      else
         bIntersect = FALSE;
   }

   return(bIntersect);
}


/*
** AttributesMatch()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** An object's attributes match the master attributes iff the object's
** attributes do not contain any set bits that are not also set in the master
** attributes.
*/
PRIVATE_CODE BOOL AttributesMatch(DWORD dwMasterAttributes,
                                  DWORD dwObjectAttributes)
{
   // We don't consider a difference in compression to be enough to call
   // the file different, especially since that attribute is impossible
   // to reconcile in some cases.

   dwObjectAttributes &= ~(FILE_ATTRIBUTE_COMPRESSED);

   return(! (dwObjectAttributes & (~dwMasterAttributes)));
}


/*
** PrepareForFolderTwinExpansion()
**
**
**
** Arguments:
**
** Returns:       void
**
** Side Effects:  none
**
** N.b., this function should be called before the outermost call to
** MyExpandIntersectingFolderTwins().
*/
PRIVATE_CODE void PrepareForFolderTwinExpansion(HBRFCASE hbr)
{
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));

   ClearFlagInArrayOfStubs(GetBriefcaseFolderPairPtrArray(hbr), STUB_FL_USED);

   EVAL(EnumObjectTwins(hbr,
                        (ENUMGENERATEDOBJECTTWINSPROC)&ClearStubFlagWrapper,
                        (PVOID)STUB_FL_FILE_STAMP_VALID));

   return;
}


/*
** MyExpandIntersectingFolderTwins()
**
** Expands all folder twins intersecting a pair of folder twins.
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  Marks expanded folder pairs used.
**
** N.b., PrepareForFolderTwinExpansion(pfp->pfpd->hbr) should be called before
** the first time this function is called.
*/
PRIVATE_CODE TWINRESULT MyExpandIntersectingFolderTwins(PFOLDERPAIR pfp,
                                                        HCLSIFACECACHE hcic,
                                                        CREATERECLISTPROC crlp,
                                                        LPARAM lpCallbackData)
{
   TWINRESULT tr;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));
   ASSERT(IS_VALID_HANDLE(hcic, CLSIFACECACHE));
   ASSERT(! crlp ||
          IS_VALID_CODE_PTR(crlp, CREATERECLISTPROC));

   /*
    * N.b., pfp may already be marked used here, but may intersect folder twins
    * that have not yet been expanded.
    */

   tr = HalfExpandIntersectingFolderTwins(pfp, hcic, crlp, lpCallbackData);

   if (tr == TR_SUCCESS)
   {
      ASSERT(IsStubFlagSet(&(pfp->stub), STUB_FL_USED));

      tr = HalfExpandIntersectingFolderTwins(pfp->pfpOther, hcic, crlp,
                                             lpCallbackData);
   }

   return(tr);
}


/*
** HalfExpandIntersectingFolderTwins()
**
** Expands all folder twins intersecting one half of a pair of folder twins.
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  Marks expanded folder pairs used.
**
** N.b., this function is only meant to be called from
** MyExpandIntersectingFolderTwins().
*/
PRIVATE_CODE TWINRESULT HalfExpandIntersectingFolderTwins(
                                                      PFOLDERPAIR pfp,
                                                      HCLSIFACECACHE hcic,
                                                      CREATERECLISTPROC crlp,
                                                      LPARAM lpCallbackData)
{
   TWINRESULT tr = TR_SUCCESS;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));
   ASSERT(IS_VALID_HANDLE(hcic, CLSIFACECACHE));
   ASSERT(! crlp ||
          IS_VALID_CODE_PTR(crlp, CREATERECLISTPROC));

   if (IsStubFlagClear(&(pfp->stub), STUB_FL_UNLINKED))
   {
      BOOL bArgIsSubtree;
      HPTRARRAY hpaFolderPairs;
      ARRAYINDEX ai;
      ARRAYINDEX aicFolderPairs;

      bArgIsSubtree = IsStubFlagSet(&(pfp->stub), STUB_FL_SUBTREE);

      hpaFolderPairs = GetBriefcaseFolderPairPtrArray(pfp->pfpd->hbr);
      aicFolderPairs = GetPtrCount(hpaFolderPairs);

      for (ai = 0; ai < aicFolderPairs; ai++)
      {
         PFOLDERPAIR pfpCur;

         pfpCur = (PFOLDERPAIR)GetPtr(hpaFolderPairs, ai);

         ASSERT(IS_VALID_STRUCT_PTR(pfpCur, CFOLDERPAIR));

         if (IsStubFlagClear(&(pfpCur->stub), STUB_FL_USED) &&
             NamesIntersect(GetString(pfp->pfpd->hsName),
                            GetString(pfpCur->pfpd->hsName)))
         {
            BOOL bCurIsSubtree;
            BOOL bExpand = FALSE;

            bCurIsSubtree = IsStubFlagSet(&(pfpCur->stub), STUB_FL_SUBTREE);

            if (bCurIsSubtree)
            {
               if (bArgIsSubtree)
                  bExpand = SubtreesIntersect(pfp->hpath, pfpCur->hpath);
               else
                  bExpand = IsPathPrefix(pfp->hpath, pfpCur->hpath);
            }
            else
            {
               if (bArgIsSubtree)
                  bExpand = IsPathPrefix(pfpCur->hpath, pfp->hpath);
               else
                  bExpand = (ComparePaths(pfp->hpath, pfpCur->hpath) == CR_EQUAL);
            }

            /* Expand folder twin and mark it used. */

            if (bExpand)
            {
               /*
                * Mark all generated object twins as non-existent or unavailable.
                * Expand available folder twin.
                */

               if (IsPathVolumeAvailable(pfp->hpath))
               {
                  EVAL(EnumGeneratedObjectTwins(pfp,
                                                &SetObjectTwinFileStampCondition,
                                                (PVOID)FS_COND_DOES_NOT_EXIST));

                  if (bCurIsSubtree)
                     tr = ExpandSubtreeTwin(pfpCur, hcic, crlp, lpCallbackData);
                  else
                     tr = ExpandFolderTwin(pfpCur, hcic, crlp, lpCallbackData);

                  if (tr != TR_SUCCESS)
                     break;
               }
               else
               {
                  EVAL(EnumGeneratedObjectTwins(pfp, &SetObjectTwinFileStampCondition,
                                                (PVOID)FS_COND_UNAVAILABLE));

                  WARNING_OUT((TEXT("HalfExpandIntersectingFolderTwins(): Unavailable folder %s skipped."),
                               DebugGetPathString(pfp->hpath)));
               }

               SetStubFlag(&(pfp->stub), STUB_FL_USED);
            }
         }
      }
   }

   return(tr);
}


#ifdef DEBUG

/*
** IsValidPCEXPANDSUBTREETWININFO()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCEXPANDSUBTREETWININFO(PCEXPANDSUBTREETWININFO pcesi)
{
   /* lpCallbackData may be any value. */

   return(IS_VALID_READ_PTR(pcesi, CEXPANDSUBTREETWININFO) &&
          IS_VALID_STRUCT_PTR(pcesi->pfp, CFOLDERPAIR) &&
          EVAL(pcesi->ucbSubtreeRootPathLen > 0) &&
          IS_VALID_HANDLE(pcesi->hcic, CLSIFACECACHE) &&
          IsValidTWINRESULT(pcesi->tr));
}

#endif


/****************************** Public Functions *****************************/


/*
** ExpandSubtree()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT ExpandSubtree(HPATH hpathRoot, EXPANDSUBTREEPROC esp,
                               PVOID pvRefData)
{
   TWINRESULT tr;
   PFINDSTATE pfs;

   /* pvRefData may be any value. */

   ASSERT(IS_VALID_HANDLE(hpathRoot, PATH));
   ASSERT(IS_VALID_CODE_PTR(esp, EXPANDSUBTREEPROC));

   ASSERT(IsPathVolumeAvailable(hpathRoot));

   if (AllocateMemory(MAX_FOLDER_DEPTH * sizeof(pfs[0]), &pfs))
   {
      /* Copy subtree root folder to beginning of search path buffer. */

      TCHAR rgchSearchSpec[MAX_PATH_LEN];
      LPTSTR pszPathSuffix;
      int iFind;
      LPTSTR pszStartOfSubPath;
      BOOL bFound;
#ifdef DEBUG
      /* Are we leaking WIN32_FIND_DATA structures? */
      ULONG ulcOpenFinds = 0;
#endif

      GetPathRootString(hpathRoot, rgchSearchSpec);
      pszPathSuffix = rgchSearchSpec + lstrlen(rgchSearchSpec);
      GetPathSuffixString(hpathRoot, pszPathSuffix);

      pszStartOfSubPath = rgchSearchSpec + lstrlen(rgchSearchSpec);

      TRACE_OUT((TEXT("ExpandSubtree(): Expanding subtree rooted at %s."),
                 rgchSearchSpec));

      /* Append *.* file specification. */

      CatPath(rgchSearchSpec, STAR_DOT_STAR);

      /* Begin search at subtree root. */

      iFind = 0;

      pfs[iFind].hff = FindFirstFile(rgchSearchSpec, &(pfs[iFind].wfd));

#ifdef DEBUG
      if (pfs[iFind].hff != INVALID_HANDLE_VALUE)
         ulcOpenFinds++;
#endif

      bFound = (pfs[iFind].hff != INVALID_HANDLE_VALUE);

      /* Rip off *.*. */

      DeleteLastPathElement(pszPathSuffix);

      /* Search subtree depth first. */

      tr = TR_SUCCESS;

      while (bFound && tr == TR_SUCCESS)
      {
         /* Did we find a directory to expand? */

         if (IS_ATTR_DIR(pfs[iFind].wfd.dwFileAttributes))
         {
            if (IsFolderToExpand(pfs[iFind].wfd.cFileName))
            {
               /* Yes.  Dive down into it. */

               /* Append the new directory to the current search path. */

               CatPath(rgchSearchSpec, pfs[iFind].wfd.cFileName);

               TRACE_OUT((TEXT("ExpandSubtree(): Diving into subfolder %s."),
                          rgchSearchSpec));

               /* Append *.* file specification. */

               CatPath(rgchSearchSpec, STAR_DOT_STAR);

               /* Start search in the new directory. */

               ASSERT(iFind < INT_MAX);
               iFind++;
               pfs[iFind].hff = FindFirstFile(rgchSearchSpec, &(pfs[iFind].wfd));

               bFound = (pfs[iFind].hff != INVALID_HANDLE_VALUE);

#ifdef DEBUG
               if (bFound)
                  ulcOpenFinds++;
#endif

               /* Rip off *.*. */

               DeleteLastPathElement(pszPathSuffix);
            }
            else
               /* Continue search in this directory. */
               bFound = FindNextFile(pfs[iFind].hff, &(pfs[iFind].wfd));
         }
         else
         {
            /* Found a file. */

            TRACE_OUT((TEXT("ExpandSubtree(): Found file %s\\%s."),
                       rgchSearchSpec,
                       pfs[iFind].wfd.cFileName));

            if ((*esp)(rgchSearchSpec, &(pfs[iFind].wfd), pvRefData))
               bFound = FindNextFile(pfs[iFind].hff, &(pfs[iFind].wfd));
            else
               tr = TR_ABORT;
         }

         if (tr == TR_SUCCESS)
         {
            while (! bFound)
            {
               /* Find failed.  Climb back up one directory level. */

               if (pfs[iFind].hff != INVALID_HANDLE_VALUE)
               {
                  FindClose(pfs[iFind].hff);
#ifdef DEBUG
                  ulcOpenFinds--;
#endif
               }

               if (iFind > 0)
               {
                  DeleteLastPathElement(pszPathSuffix);
                  iFind--;

                  if (IsFolderToExpand(pfs[iFind].wfd.cFileName))
                  {
                     TRACE_OUT((TEXT("ExpandSubtree(): Found folder %s\\%s."),
                                rgchSearchSpec,
                                pfs[iFind].wfd.cFileName));

                     if (! (*esp)(rgchSearchSpec, &(pfs[iFind].wfd), pvRefData))
                     {
                        tr = TR_ABORT;
                        break;
                     }
                  }

                  bFound = FindNextFile(pfs[iFind].hff, &(pfs[iFind].wfd));
               }
               else
               {
                  ASSERT(! iFind);
                  break;
               }
            }
         }
      }

      if (tr != TR_SUCCESS)
      {
         /* Close all open find operations on failure. */

         while (iFind >= 0)
         {
            if (pfs[iFind].hff != INVALID_HANDLE_VALUE)
            {
               FindClose(pfs[iFind].hff);
               iFind--;
#ifdef DEBUG
               ulcOpenFinds--;
#endif
            }
         }
      }

      ASSERT(! ulcOpenFinds);

      FreeMemory(pfs);

      TRACE_OUT((TEXT("ExpandSubtree(): Subtree expansion complete.")));
   }
   else
      tr = TR_OUT_OF_MEMORY;

   return(tr);
}


/*
** ClearStubFlagWrapper()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL ClearStubFlagWrapper(PSTUB pstub, PVOID dwFlags)
{
   ASSERT(IS_VALID_STRUCT_PTR(pstub, CSTUB));
   ASSERT(FLAGS_ARE_VALID(PtrToUlong(dwFlags), ALL_STUB_FLAGS));

   ClearStubFlag(pstub, PtrToUlong(dwFlags));

   return(TRUE);
}


/*
** SetStubFlagWrapper()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL SetStubFlagWrapper(PSTUB pstub, PVOID dwFlags)
{
   ASSERT(IS_VALID_STRUCT_PTR(pstub, CSTUB));
   ASSERT(FLAGS_ARE_VALID(PtrToUlong(dwFlags), ALL_STUB_FLAGS));

   SetStubFlag(pstub, PtrToUlong(dwFlags));

   return(TRUE);
}


/*
** ExpandIntersectingFolderTwins()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  Leaves only the folder pairs expanded marked used.
*/
PUBLIC_CODE TWINRESULT ExpandIntersectingFolderTwins(PFOLDERPAIR pfp,
                                                     CREATERECLISTPROC crlp,
                                                     LPARAM lpCallbackData)
{
   TWINRESULT tr;
   HCLSIFACECACHE hcic;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));
   ASSERT(! crlp ||
          IS_VALID_CODE_PTR(crlp, CREATERECLISTPROC));

   ASSERT(IsStubFlagClear(&(pfp->stub), STUB_FL_UNLINKED));

   if (CreateClassInterfaceCache(&hcic))
   {
      /* Prepare for call to MyExpandIntersectingFolderTwins(). */

      PrepareForFolderTwinExpansion(pfp->pfpd->hbr);

      tr = MyExpandIntersectingFolderTwins(pfp, hcic, crlp, lpCallbackData);

      DestroyClassInterfaceCache(hcic);
   }
   else
      tr = TR_OUT_OF_MEMORY;

   return(tr);
}


/*
** ExpandFolderTwinsIntersectingTwinList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  Leaves only the folder pairs expanded marked used.
*/
PUBLIC_CODE TWINRESULT ExpandFolderTwinsIntersectingTwinList(
                                                      HTWINLIST htl,
                                                      CREATERECLISTPROC crlp,
                                                      LPARAM lpCallbackData)
{
   TWINRESULT tr;
   HCLSIFACECACHE hcic;

   /* lpCallbackData may be any value. */

   ASSERT(IS_VALID_HANDLE(htl, TWINLIST));
   ASSERT(! crlp ||
          IS_VALID_CODE_PTR(crlp, CREATERECLISTPROC));

   if (CreateClassInterfaceCache(&hcic))
   {
      ARRAYINDEX aicTwins;
      ARRAYINDEX ai;

      tr = TR_SUCCESS;

      /* Prepare for calls to MyExpandIntersectingFolderTwins(). */

      PrepareForFolderTwinExpansion(GetTwinListBriefcase(htl));

      aicTwins = GetTwinListCount(htl);

      for (ai = 0; ai < aicTwins; ai++)
      {
         HTWIN htwin;

         htwin = GetTwinFromTwinList(htl, ai);

         /* Expand only live folder twins. */

         if (((PCSTUB)htwin)->st == ST_FOLDERPAIR)
         {
            tr = MyExpandIntersectingFolderTwins((PFOLDERPAIR)htwin, hcic,
                                                 crlp, lpCallbackData);

            if (tr != TR_SUCCESS)
               break;
         }
      }

      DestroyClassInterfaceCache(hcic);
   }
   else
      tr = TR_OUT_OF_MEMORY;

   return(tr);
}


/*
** TryToGenerateObjectTwin()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT TryToGenerateObjectTwin(HBRFCASE hbr, HPATH hpathFolder,
                                               LPCTSTR pcszName,
                                               PBOOL pbGenerated,
                                               POBJECTTWIN *ppot)
{
   TWINRESULT tr;
   HCLSIFACECACHE hcic;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pbGenerated, BOOL));
   ASSERT(IS_VALID_WRITE_PTR(ppot, POBJECTTWIN));

   if (CreateClassInterfaceCache(&hcic))
   {
      HPTRARRAY hpaFolderPairs;
      ARRAYINDEX aicPtrs;
      ARRAYINDEX ai;

      tr = TR_SUCCESS;
      *pbGenerated = FALSE;

      hpaFolderPairs = GetBriefcaseFolderPairPtrArray(hbr);

      aicPtrs = GetPtrCount(hpaFolderPairs);
      ASSERT(! (aicPtrs % 2));

      for (ai = 0; ai < aicPtrs; ai++)
      {
         PCFOLDERPAIR pcfp;

         pcfp = GetPtr(hpaFolderPairs, ai);

         if (FolderTwinGeneratesObjectTwin(pcfp, hpathFolder, pcszName))
         {
            TCHAR rgchSubPath[MAX_PATH_LEN];
            LPCTSTR pcszSubPath;
            POBJECTTWIN potOther;

            if (IsStubFlagSet(&(pcfp->stub), STUB_FL_SUBTREE))
               pcszSubPath = FindChildPathSuffix(pcfp->hpath, hpathFolder,
                                                 rgchSubPath);
            else
               pcszSubPath = EMPTY_STRING;

            tr = FakeObjectTwinFromFolderTwin(pcfp, pcszSubPath, pcszName,
                                              hcic, ppot, &potOther);

            if (tr == TR_SUCCESS)
               *pbGenerated = TRUE;
            else
               ASSERT(tr != TR_DUPLICATE_TWIN);

            break;
         }
      }

      DestroyClassInterfaceCache(hcic);
   }
   else
      tr = TR_OUT_OF_MEMORY;

   ASSERT(tr != TR_SUCCESS ||
          ! *pbGenerated ||
          IS_VALID_STRUCT_PTR(*ppot, COBJECTTWIN));

   return(tr);
}


/*
** NamesIntersect()
**
** Determines whether or not two names may refer to the same object.  Both
** names may contain wildcards ('*' or '?').
**
** Arguments:     pcszName1 - first name
**                pcszName2 - second name
**
** Returns:       TRUE if the two names intersect.  FALSE if not.
**
** Side Effects:  none
**
** A "name" is broken up into two components: a "base" and an optional
** "extension", e.g., "BASE" or "BASE.EXT".
**
** "Intersecting names" are defined as follows:
**
**    1) An asterisk matches 0 or more characters in the base or extension.
**    2) Any characters after an asterisk in the base or extension are ignored.
**    3) A question mark matches exactly one character, or no character if it
**       appears at the end of the base or extension.
**
** N.b., this function does not perform any checking on the validity of the two
** names.
*/
PUBLIC_CODE BOOL NamesIntersect(LPCTSTR pcszName1, LPCTSTR pcszName2)
{
   BOOL bIntersect = FALSE;

   ASSERT(IS_VALID_STRING_PTR(pcszName1, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszName2, CSTR));

   if (NameComponentsIntersect(pcszName1, pcszName2))
   {
      LPCTSTR pcszExt1;
      LPCTSTR pcszExt2;

      /* Get extensions, skipping leading periods. */

      pcszExt1 = ExtractExtension(pcszName1);
      if (*pcszExt1 == PERIOD)
         pcszExt1 = CharNext(pcszExt1);

      pcszExt2 = ExtractExtension(pcszName2);
      if (*pcszExt2 == PERIOD)
         pcszExt2 = CharNext(pcszExt2);

      bIntersect = NameComponentsIntersect(pcszExt1, pcszExt2);
   }

   return(bIntersect);
}


#ifdef DEBUG

/*
** IsValidTWINRESULT()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidTWINRESULT(TWINRESULT tr)
{
   BOOL bResult;

   switch (tr)
   {
      case TR_SUCCESS:
      case TR_RH_LOAD_FAILED:
      case TR_SRC_OPEN_FAILED:
      case TR_SRC_READ_FAILED:
      case TR_DEST_OPEN_FAILED:
      case TR_DEST_WRITE_FAILED:
      case TR_ABORT:
      case TR_UNAVAILABLE_VOLUME:
      case TR_OUT_OF_MEMORY:
      case TR_FILE_CHANGED:
      case TR_DUPLICATE_TWIN:
      case TR_DELETED_TWIN:
      case TR_HAS_FOLDER_TWIN_SRC:
      case TR_INVALID_PARAMETER:
      case TR_REENTERED:
      case TR_SAME_FOLDER:
      case TR_SUBTREE_CYCLE_FOUND:
      case TR_NO_MERGE_HANDLER:
      case TR_MERGE_INCOMPLETE:
      case TR_TOO_DIFFERENT:
      case TR_BRIEFCASE_LOCKED:
      case TR_BRIEFCASE_OPEN_FAILED:
      case TR_BRIEFCASE_READ_FAILED:
      case TR_BRIEFCASE_WRITE_FAILED:
      case TR_CORRUPT_BRIEFCASE:
      case TR_NEWER_BRIEFCASE:
      case TR_NO_MORE:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("IsValidTWINRESULT(): Invalid TWINRESULT %d."),
                    tr));
         break;
   }

   return(bResult);
}

#endif
