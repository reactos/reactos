/*
 * subcycle.c - Subtree cycle detection routines module.
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

#define NUM_CYCLE_PTRS_TO_ADD          (16)


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE TWINRESULT CheckHalfForSubtreeCycle(HPTRARRAY, HPATH, HPATH, LPCTSTR);


/*
** CheckHalfForSubtreeCycle()
**
** Checks to see if half of a proposed new folder subtree twin would create one
** or more cycles of folder subtree twins.
**
** Arguments:     hpaFolderPairs - handle to PTRARRAY containing pointers to
**                                 folder pairs
**                hpathStartFolder - root folder of initial half of proposed
**                                   new folder pair
**                hpathEndFolder - root folder of other half of proposed new
**                                 folder pair
**                pcszName - name specification of matching objects to be
**                           included in proposed new folder subtree pair
**
** Returns:       TWINRESULT
**
** Side Effects:  none
**
** N.b., this function should be called twice for each proposed new folder
** subtree pair.
*/
PRIVATE_CODE TWINRESULT CheckHalfForSubtreeCycle(HPTRARRAY hpaFolderPairs,
                                                 HPATH hpathStartFolder,
                                                 HPATH hpathEndFolder,
                                                 LPCTSTR pcszName)
{
   TWINRESULT tr;
   ARRAYINDEX aicFolderPairs;
   NEWPTRARRAY npa;
   HPTRARRAY hpaFolders;

   ASSERT(IS_VALID_HANDLE(hpaFolderPairs, PTRARRAY));
   ASSERT(IS_VALID_HANDLE(hpathStartFolder, PATH));
   ASSERT(IS_VALID_HANDLE(hpathEndFolder, PATH));
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));

   aicFolderPairs = GetPtrCount(hpaFolderPairs);

   /*
    * Try to create an unsorted pointer array to be used in checking for
    * cycles.
    */

   npa.aicInitialPtrs = aicFolderPairs;
   npa.aicAllocGranularity = NUM_CYCLE_PTRS_TO_ADD;
   npa.dwFlags = 0;

   if (CreatePtrArray(&npa, &hpaFolders))
   {
      ARRAYINDEX aicFolders;
      ARRAYINDEX aiCurFolder;
      HPATH hpathCurFolderRoot;

      /* Search all folder pairs connected to the first new folder twin. */

      /*
       * Mark all folder twins unused.  A "used" folder twin is one that has
       * already been visited while searching for subtree cycles.  I.e., a
       * used folder subtree pair half intersected the first folder of the
       * proposed new folder twin, and its other half was added to the list for
       * later comparison.
       */

      ClearFlagInArrayOfStubs(hpaFolderPairs, STUB_FL_USED);

      /*
       * Loop to process entire graph of folder subtree twins connected to the
       * first new folder twin.  Folder twins are only added to the hpaFolders
       * array if they don't already intersect the second of the two proposed
       * new folder subtree twins.
       */

      tr = TR_SUCCESS;
      aicFolders = 0;
      aiCurFolder = 0;

      /* Begin with start folder. */

      hpathCurFolderRoot = hpathStartFolder;

      FOREVER
      {
         ARRAYINDEX aiCheckFolderRoot;

         /*
          * Loop to find all subtree folder pairs that intersect
          * hpaFolders[aiCurFolder]'s subtree.
          */

         for (aiCheckFolderRoot = 0;
              aiCheckFolderRoot < aicFolderPairs;
              aiCheckFolderRoot++)
         {
            PFOLDERPAIR pfpCheck;

            /* Get this subtree folder pair's root folder. */

            pfpCheck = GetPtr(hpaFolderPairs, aiCheckFolderRoot);

            ASSERT(IS_VALID_STRUCT_PTR(pfpCheck, CFOLDERPAIR));

            /* Have we already visited this folder pair? */

            if (IsStubFlagSet(&(pfpCheck->stub), STUB_FL_SUBTREE) &&
                IsStubFlagClear(&(pfpCheck->stub), STUB_FL_BEING_TRANSLATED) &&
                IsStubFlagClear(&(pfpCheck->stub), STUB_FL_USED) &&
                IsStubFlagClear(&(pfpCheck->pfpOther->stub), STUB_FL_USED))
            {
               /*
                * No.  Does this subtree folder pair intersect the current
                * folder pair node's subtree, and the objects named in the
                * proposed new folder subtree twin?
                */

               ASSERT(IsStubFlagSet(&(pfpCheck->pfpOther->stub), STUB_FL_SUBTREE));
               ASSERT(IsStubFlagClear(&(pfpCheck->pfpOther->stub), STUB_FL_BEING_TRANSLATED));

               if (SubtreesIntersect(hpathCurFolderRoot, pfpCheck->hpath) &&
                   NamesIntersect(GetString(pfpCheck->pfpd->hsName), pcszName))
               {
                  HPATH hpathOtherCheckFolderRoot;

                  /* Yes.  Get the other side of the folder subtree pair. */

                  hpathOtherCheckFolderRoot = pfpCheck->pfpOther->hpath;

                  /*
                   * Does this pair connect back to the other side of the
                   * proposed new folder pair?
                   */

                  if (SubtreesIntersect(hpathOtherCheckFolderRoot,
                                        hpathEndFolder))
                  {
                     /*
                      * Yes.  Are the roots different parts of the common
                      * subtree?
                      */

                     if (ComparePaths(hpathEndFolder,
                                      hpathOtherCheckFolderRoot)
                         != CR_EQUAL)
                     {
                        /* Yes.  Found a cycle.  Bail out. */

                        WARNING_OUT((TEXT("CheckHalfForSubtreeCycle(): Subtree cycle found connecting folders %s and %s."),
                                     DebugGetPathString(hpathStartFolder),
                                     DebugGetPathString(hpathEndFolder)));

                        tr = TR_SUBTREE_CYCLE_FOUND;
                        break;
                     }

                     /*
                      * We don't need to include this root in the search if it
                      * is the same as the other side of the proposed new
                      * folder pair since it will be covered during the other
                      * call to CheckHalfForSubtreeCycle().
                      */
                  }
                  else
                  {
                     /* Add this subtree as another node to be examined. */

                     if (! InsertPtr(hpaFolders, NULL, aicFolders++,
                                     (PCVOID)(pfpCheck->pfpOther)))
                        tr = TR_OUT_OF_MEMORY;
                  }

                  /* Mark this folder twin as already visited. */

                  if (tr == TR_SUCCESS)
                     SetStubFlag(&(pfpCheck->stub), STUB_FL_USED);
                  else
                     break;
               }
            }
         }

         /* Any folder subtree twins left to investigate? */

         if (aiCurFolder < aicFolders)
         {
            PFOLDERPAIR pfpCur;

            /* Yes. */

            pfpCur = GetPtr(hpaFolders, aiCurFolder++);

            hpathCurFolderRoot = pfpCur->hpath;
         }
         else
            /* No. */
            break;
      }

      DestroyPtrArray(hpaFolders);
   }
   else
      tr = TR_OUT_OF_MEMORY;

   return(tr);
}


/****************************** Public Functions *****************************/


/*
** BeginTranslateFolder()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void BeginTranslateFolder(PFOLDERPAIR pfp)
{
   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   ASSERT(IsStubFlagClear(&(pfp->stub), STUB_FL_BEING_TRANSLATED));
   ASSERT(IsStubFlagClear(&(pfp->pfpOther->stub), STUB_FL_BEING_TRANSLATED));

   SetStubFlag(&(pfp->stub), STUB_FL_BEING_TRANSLATED);
   SetStubFlag(&(pfp->pfpOther->stub), STUB_FL_BEING_TRANSLATED);

   return;
}


/*
** EndTranslateFolder()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void EndTranslateFolder(PFOLDERPAIR pfp)
{
   ASSERT(IS_VALID_STRUCT_PTR(pfp, CFOLDERPAIR));

   ASSERT(IsStubFlagSet(&(pfp->stub), STUB_FL_BEING_TRANSLATED));
   ASSERT(IsStubFlagSet(&(pfp->pfpOther->stub), STUB_FL_BEING_TRANSLATED));

   ClearStubFlag(&(pfp->stub), STUB_FL_BEING_TRANSLATED);
   ClearStubFlag(&(pfp->pfpOther->stub), STUB_FL_BEING_TRANSLATED);

   return;
}


/*
** CheckForSubtreeCycles()
**
** Checks to see if a proposed new folder subtree twin would create one or more
** cycles of folder subtree twins.
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
**
** N.b., TR_SUBTREE_CYCLE_FOUND is returned if the folder subtree roots of the
** proposed new folder subtree twin are the same.
*/
PUBLIC_CODE TWINRESULT CheckForSubtreeCycles(HPTRARRAY hpaFolderPairs,
                                             HPATH hpathFirstFolder,
                                             HPATH hpathSecondFolder,
                                             HSTRING hsName)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_HANDLE(hpaFolderPairs, PTRARRAY));
   ASSERT(IS_VALID_HANDLE(hpathFirstFolder, PATH));
   ASSERT(IS_VALID_HANDLE(hpathSecondFolder, PATH));
   ASSERT(IS_VALID_HANDLE(hsName, STRING));

   /* Are the folder twins cyclical on their own? */

   if (SubtreesIntersect(hpathFirstFolder, hpathSecondFolder))
   {
      /* Yes. */

      tr = TR_SUBTREE_CYCLE_FOUND;

      WARNING_OUT((TEXT("CheckForSubtreeCycles(): Subtree cycle found connecting folders %s and %s."),
                   DebugGetPathString(hpathFirstFolder),
                   DebugGetPathString(hpathSecondFolder)));
   }
   else
   {
      LPCTSTR pcszName;

      /* No.  Check for any indirect subtree cycle.  */

      pcszName = GetString(hsName);

      tr = CheckHalfForSubtreeCycle(hpaFolderPairs, hpathFirstFolder,
                                    hpathSecondFolder, pcszName);

      if (tr == TR_SUCCESS)
         tr = CheckHalfForSubtreeCycle(hpaFolderPairs, hpathSecondFolder,
                                       hpathFirstFolder, pcszName);
   }

   return(tr);
}

