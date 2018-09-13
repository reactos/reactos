/*
 * twin.c - Twin ADT module.
 */

/*



*/


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "stub.h"
#include "oleutil.h"


/* Constants
 ************/

/* twin family pointer array allocation constants */

#define NUM_START_TWIN_FAMILY_PTRS        (16)
#define NUM_TWIN_FAMILY_PTRS_TO_ADD       (16)


/* Types
 ********/

/* twin families database structure header */

typedef struct _twinfamiliesdbheader
{
   /* number of twin families */

   LONG lcTwinFamilies;
}
TWINFAMILIESDBHEADER;
DECLARE_STANDARD_TYPES(TWINFAMILIESDBHEADER);

/* individual twin family database structure header */

typedef struct _twinfamilydbheader
{
   /* stub flags */

   DWORD dwStubFlags;

   /* old string handle of name */

   HSTRING hsName;

   /* number of object twins in family */

   LONG lcObjectTwins;
}
TWINFAMILYDBHEADER;
DECLARE_STANDARD_TYPES(TWINFAMILYDBHEADER);

/* object twin database structure */

typedef struct _dbobjecttwin
{
   /* stub flags */

   DWORD dwStubFlags;

   /* old handle to folder string */

   HPATH hpath;

   /* time stamp at last reconciliation */

   FILESTAMP fsLastRec;
}
DBOBJECTTWIN;
DECLARE_STANDARD_TYPES(DBOBJECTTWIN);

/* GenerateSpinOffObjectTwin() callback structure */

typedef struct _spinoffobjecttwininfo
{
   PCFOLDERPAIR pcfp;

   HLIST hlistNewObjectTwins;
}
SPINOFFOBJECTTWININFO;
DECLARE_STANDARD_TYPES(SPINOFFOBJECTTWININFO);

typedef void (CALLBACK *COPYOBJECTTWINPROC)(POBJECTTWIN, PCDBOBJECTTWIN);


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE TWINRESULT TwinJustTheseTwoObjects(HBRFCASE, HPATH, HPATH, LPCTSTR, POBJECTTWIN *, POBJECTTWIN *, HLIST);
PRIVATE_CODE BOOL CreateTwinFamily(HBRFCASE, LPCTSTR, PTWINFAMILY *);
PRIVATE_CODE void CollapseTwinFamilies(PTWINFAMILY, PTWINFAMILY);
PRIVATE_CODE BOOL GenerateSpinOffObjectTwin(PVOID, PVOID);
PRIVATE_CODE BOOL BuildBradyBunch(PVOID, PVOID);
PRIVATE_CODE BOOL CreateObjectTwinAndAddToList(PTWINFAMILY, HPATH, HLIST, POBJECTTWIN *, PHNODE);
PRIVATE_CODE BOOL CreateListOfGeneratedObjectTwins(PCFOLDERPAIR, PHLIST);
PRIVATE_CODE void NotifyNewObjectTwins(HLIST, HCLSIFACECACHE);
PRIVATE_CODE HRESULT NotifyOneNewObjectTwin(PINotifyReplica, PCOBJECTTWIN, LPCTSTR);
PRIVATE_CODE HRESULT CreateOtherReplicaMonikers(PCOBJECTTWIN, PULONG, PIMoniker **);
PRIVATE_CODE COMPARISONRESULT TwinFamilySortCmp(PCVOID, PCVOID);
PRIVATE_CODE COMPARISONRESULT TwinFamilySearchCmp(PCVOID, PCVOID);
PRIVATE_CODE BOOL ObjectTwinSearchCmp(PCVOID, PCVOID);
PRIVATE_CODE TWINRESULT WriteTwinFamily(HCACHEDFILE, PCTWINFAMILY);
PRIVATE_CODE TWINRESULT WriteObjectTwin(HCACHEDFILE, PCOBJECTTWIN);
PRIVATE_CODE TWINRESULT ReadTwinFamily(HCACHEDFILE, HBRFCASE, PCDBVERSION, HHANDLETRANS, HHANDLETRANS);
PRIVATE_CODE TWINRESULT ReadObjectTwin(HCACHEDFILE, PCDBVERSION, PTWINFAMILY, HHANDLETRANS);
PRIVATE_CODE void CopyTwinFamilyInfo(PTWINFAMILY, PCTWINFAMILYDBHEADER);
PRIVATE_CODE void CopyObjectTwinInfo(POBJECTTWIN, PCDBOBJECTTWIN);
PRIVATE_CODE void CopyM8ObjectTwinInfo(POBJECTTWIN, PCDBOBJECTTWIN);
PRIVATE_CODE BOOL DestroyObjectTwinStubWalker(PVOID, PVOID);
PRIVATE_CODE BOOL MarkObjectTwinNeverReconciledWalker(PVOID, PVOID);
PRIVATE_CODE BOOL LookForSrcFolderTwinsWalker(PVOID, PVOID);
PRIVATE_CODE BOOL IncrementSrcFolderTwinsWalker(PVOID, PVOID);
PRIVATE_CODE BOOL ClearSrcFolderTwinsWalker(PVOID, PVOID);
PRIVATE_CODE BOOL SetTwinFamilyWalker(PVOID, PVOID);
PRIVATE_CODE BOOL InsertNodeAtFrontWalker(POBJECTTWIN, PVOID);

#ifdef VSTF

PRIVATE_CODE BOOL IsValidObjectTwinWalker(PVOID, PVOID);
PRIVATE_CODE BOOL IsValidPCNEWOBJECTTWIN(PCNEWOBJECTTWIN);
PRIVATE_CODE BOOL IsValidPCSPINOFFOBJECTTWININFO(PCSPINOFFOBJECTTWININFO);

#endif

#ifdef DEBUG

PRIVATE_CODE BOOL AreTwinFamiliesValid(HPTRARRAY);

#endif


/*
** TwinJustTheseTwoObjects()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT TwinJustTheseTwoObjects(HBRFCASE hbr, HPATH hpathFolder1,
                                           HPATH hpathFolder2, LPCTSTR pcszName,
                                           POBJECTTWIN *ppot1,
                                           POBJECTTWIN *ppot2,
                                           HLIST hlistNewObjectTwins)
{
   TWINRESULT tr = TR_OUT_OF_MEMORY;
   HNODE hnodeSearch;
   BOOL bFound1;
   BOOL bFound2;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hpathFolder1, PATH));
   ASSERT(IS_VALID_HANDLE(hpathFolder2, PATH));
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppot1, POBJECTTWIN));
   ASSERT(IS_VALID_WRITE_PTR(ppot2, POBJECTTWIN));
   ASSERT(IS_VALID_HANDLE(hlistNewObjectTwins, LIST));

   /* Determine twin families of existing object twins. */

   bFound1 = FindObjectTwin(hbr, hpathFolder1, pcszName, &hnodeSearch);

   if (bFound1)
      *ppot1 = (POBJECTTWIN)GetNodeData(hnodeSearch);

   bFound2 = FindObjectTwin(hbr, hpathFolder2, pcszName, &hnodeSearch);

   if (bFound2)
      *ppot2 = (POBJECTTWIN)GetNodeData(hnodeSearch);

   /* Take action based upon existence of two object twins. */

   if (! bFound1 && ! bFound2)
   {
      PTWINFAMILY ptfNew;

      /* Neither object is already present.  Create a new twin family. */

      if (CreateTwinFamily(hbr, pcszName, &ptfNew))
      {
         HNODE hnodeNew1;

         if (CreateObjectTwinAndAddToList(ptfNew, hpathFolder1,
                                          hlistNewObjectTwins, ppot1,
                                          &hnodeNew1))
         {
            HNODE hnodeNew2;

            if (CreateObjectTwinAndAddToList(ptfNew, hpathFolder2,
                                             hlistNewObjectTwins, ppot2,
                                             &hnodeNew2))
            {
               TRACE_OUT((TEXT("TwinJustTheseTwoObjects(): Created a twin family for object %s in folders %s and %s."),
                          pcszName,
                          DebugGetPathString(hpathFolder1),
                          DebugGetPathString(hpathFolder2)));

               ASSERT(IsStubFlagClear(&(ptfNew->stub), STUB_FL_DELETION_PENDING));

               tr = TR_SUCCESS;
            }
            else
            {
               DeleteNode(hnodeNew1);
               DestroyStub(&((*ppot1)->stub));
TWINJUSTTHESETWOOBJECTS_BAIL:
               DestroyStub(&(ptfNew->stub));
            }
         }
         else
            goto TWINJUSTTHESETWOOBJECTS_BAIL;
      }
   }
   else if (bFound1 && bFound2)
   {
      /*
       * Both objects are already present.  Are they members of the same twin
       * family?
       */

      if ((*ppot1)->ptfParent == (*ppot2)->ptfParent)
      {
         /* Yes, same twin family.  Complain that these twins already exist. */

         TRACE_OUT((TEXT("TwinJustTheseTwoObjects(): Object %s is already twinned in folders %s and %s."),
                    pcszName,
                    DebugGetPathString(hpathFolder1),
                    DebugGetPathString(hpathFolder2)));

         tr = TR_DUPLICATE_TWIN;
      }
      else
      {
         /*
          * No, different twin families.  Collapse the two families.
          *
          * "That's the way they became the Brady bunch..."
          *
          * *ppot1 and *ppot2 remain valid across this call.
          */

         TRACE_OUT((TEXT("TwinJustTheseTwoObjects(): Collapsing separate twin families for object %s in folders %s and %s."),
                    pcszName,
                    DebugGetPathString(hpathFolder1),
                    DebugGetPathString(hpathFolder2)));

         CollapseTwinFamilies((*ppot1)->ptfParent, (*ppot2)->ptfParent);

         tr = TR_SUCCESS;
      }
   }
   else
   {
      PTWINFAMILY ptfParent;
      HNODE hnodeUnused;

      /*
       * Only one of the two objects is present.  Add the new object twin
       * to the existing object twin's family.
       */

      if (bFound1)
      {
         /* First object is already a twin. */

         ptfParent = (*ppot1)->ptfParent;

         if (CreateObjectTwinAndAddToList(ptfParent, hpathFolder2,
                                          hlistNewObjectTwins, ppot2,
                                          &hnodeUnused))
         {
            TRACE_OUT((TEXT("TwinJustTheseTwoObjects(): Adding twin of object %s\\%s to existing twin family including %s\\%s."),
                       DebugGetPathString(hpathFolder2),
                       pcszName,
                       DebugGetPathString(hpathFolder1),
                       pcszName));

            tr = TR_SUCCESS;
         }
      }
      else
      {
         /* Second object is already a twin. */

         ptfParent = (*ppot2)->ptfParent;

         if (CreateObjectTwinAndAddToList(ptfParent, hpathFolder1,
                                          hlistNewObjectTwins, ppot1,
                                          &hnodeUnused))
         {
            TRACE_OUT((TEXT("TwinJustTheseTwoObjects(): Adding twin of object %s\\%s to existing twin family including %s\\%s."),
                       DebugGetPathString(hpathFolder1),
                       pcszName,
                       DebugGetPathString(hpathFolder2),
                       pcszName));

            tr = TR_SUCCESS;
         }
      }
   }

   ASSERT((tr != TR_SUCCESS && tr != TR_DUPLICATE_TWIN) ||
          IS_VALID_STRUCT_PTR(*ppot1, COBJECTTWIN) && IS_VALID_STRUCT_PTR(*ppot2, COBJECTTWIN));

   return(tr);
}


/*
** CreateTwinFamily()
**
** Creates a new empty twin family, and adds it to a briefcase.
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CreateTwinFamily(HBRFCASE hbr, LPCTSTR pcszName, PTWINFAMILY *pptf)
{
   BOOL bResult = FALSE;
   PTWINFAMILY ptfNew;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pptf, PTWINFAMILY));

   /* Try to create a new TWINFAMILY structure. */

   if (AllocateMemory(sizeof(*ptfNew), &ptfNew))
   {
      NEWLIST nl;
      HLIST hlistObjectTwins;

      /* Create a list of object twins for the new twin family. */

      nl.dwFlags = 0;

      if (CreateList(&nl, &hlistObjectTwins))
      {
         HSTRING hsName;

         /* Add the object name to the name string table. */

         if (AddString(pcszName, GetBriefcaseNameStringTable(hbr), 
            GetHashBucketIndex, &hsName))
         {
            ARRAYINDEX aiUnused;

            /* Fill in TWINFAMILY fields. */

            InitStub(&(ptfNew->stub), ST_TWINFAMILY);

            ptfNew->hsName = hsName;
            ptfNew->hlistObjectTwins = hlistObjectTwins;
            ptfNew->hbr = hbr;

            MarkTwinFamilyNeverReconciled(ptfNew);

            /* Add the twin family to the briefcase's list of twin families. */

            if (AddPtr(GetBriefcaseTwinFamilyPtrArray(hbr), TwinFamilySortCmp, ptfNew, &aiUnused))
            {
               *pptf = ptfNew;
               bResult = TRUE;

               ASSERT(IS_VALID_STRUCT_PTR(*pptf, CTWINFAMILY));
            }
            else
            {
               DeleteString(hsName);
CREATETWINFAMILY_BAIL1:
               DestroyList(hlistObjectTwins);
CREATETWINFAMILY_BAIL2:
               FreeMemory(ptfNew);
            }
         }
         else
            goto CREATETWINFAMILY_BAIL1;
      }
      else
         goto CREATETWINFAMILY_BAIL2;
   }

   return(bResult);
}


/*
** CollapseTwinFamilies()
**
** Collapses two twin families into one.  N.b., this function should only be
** called on two twin families with the same object name!
**
** Arguments:     ptf1 - pointer to destination twin family
**                ptf2 - pointer to source twin family
**
** Returns:       void
**
** Side Effects:  Twin family *ptf2 is destroyed.
*/
PRIVATE_CODE void CollapseTwinFamilies(PTWINFAMILY ptf1, PTWINFAMILY ptf2)
{
   ASSERT(IS_VALID_STRUCT_PTR(ptf1, CTWINFAMILY));
   ASSERT(IS_VALID_STRUCT_PTR(ptf2, CTWINFAMILY));

   ASSERT(CompareNameStringsByHandle(ptf1->hsName, ptf2->hsName) == CR_EQUAL);

   /* Use the first twin family as the collapsed twin family. */

   /*
    * Change the parent twin family of the object twins in the second twin
    * family to the first twin family.
    */

   EVAL(WalkList(ptf2->hlistObjectTwins, &SetTwinFamilyWalker, ptf1));

   /* Append object list from second twin family on to first. */

   AppendList(ptf1->hlistObjectTwins, ptf2->hlistObjectTwins);

   MarkTwinFamilyNeverReconciled(ptf1);

   /* Wipe out the old twin family. */

   DestroyStub(&(ptf2->stub));

   ASSERT(IS_VALID_STRUCT_PTR(ptf1, CTWINFAMILY));

   return;
}


/*
** GenerateSpinOffObjectTwin()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL GenerateSpinOffObjectTwin(PVOID pot, PVOID pcsooti)
{
   BOOL bResult;
   HPATH hpathMatchingFolder;
   HNODE hnodeUnused;

   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(IS_VALID_STRUCT_PTR(pcsooti, CSPINOFFOBJECTTWININFO));

   /*
    * Append the generated object twin's subpath to the matching folder twin's
    * base path for subtree twins.
    */

   if (BuildPathForMatchingObjectTwin(
                     ((PCSPINOFFOBJECTTWININFO)pcsooti)->pcfp, pot,
                     GetBriefcasePathList(((POBJECTTWIN)pot)->ptfParent->hbr),
                     &hpathMatchingFolder))
   {
      /*
       * Does this generated object twin's twin family already contain an
       * object twin generated by the other half of the pair of folder twins?
       */

      if (! SearchUnsortedList(((POBJECTTWIN)pot)->ptfParent->hlistObjectTwins,
                               &ObjectTwinSearchCmp, hpathMatchingFolder,
                               &hnodeUnused))
      {
         /*
          * No.  Does the other object twin already exist in a different twin
          * family?
          */

         if (FindObjectTwin(((POBJECTTWIN)pot)->ptfParent->hbr,
                            hpathMatchingFolder,
                            GetString(((POBJECTTWIN)pot)->ptfParent->hsName),
                            &hnodeUnused))
         {
            /* Yes. */

            ASSERT(((PCOBJECTTWIN)GetNodeData(hnodeUnused))->ptfParent != ((POBJECTTWIN)pot)->ptfParent);

            bResult = TRUE;
         }
         else
         {
            POBJECTTWIN potNew;

            /*
             * No.  Create a new object twin, and add it to the bookkeeping
             * list of new object twins.
             */

            bResult = CreateObjectTwinAndAddToList(
                     ((POBJECTTWIN)pot)->ptfParent, hpathMatchingFolder,
                     ((PCSPINOFFOBJECTTWININFO)pcsooti)->hlistNewObjectTwins,
                     &potNew, &hnodeUnused);

#ifdef DEBUG

            if (bResult)
            {
               TRACE_OUT((TEXT("GenerateSpinOffObjectTwin(): Generated spin-off object twin for object %s\\%s."),
                          DebugGetPathString(potNew->hpath),
                          GetString(potNew->ptfParent->hsName)));
            }

#endif

         }
      }
      else
         bResult = TRUE;

      DeletePath(hpathMatchingFolder);
   }
   else
      bResult = FALSE;

   return(bResult);
}


/*
** BuildBradyBunch()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL BuildBradyBunch(PVOID pot, PVOID pcfp)
{
   BOOL bResult;
   HPATH hpathMatchingFolder;
   HNODE hnodeOther;

   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));

   /*
    * Append the generated object twin's subpath to the matching folder twin's
    * base path for subtree twins.
    */

   bResult = BuildPathForMatchingObjectTwin(
                     pcfp, pot,
                     GetBriefcasePathList(((POBJECTTWIN)pot)->ptfParent->hbr),
                     &hpathMatchingFolder);

   if (bResult)
   {
      /*
       * Does this generated object twin's twin family already contain an object
       * twin generated by the other half of the pair of folder twins?
       */

      if (! SearchUnsortedList(((POBJECTTWIN)pot)->ptfParent->hlistObjectTwins,
                               &ObjectTwinSearchCmp, hpathMatchingFolder,
                               &hnodeOther))
      {
         /*
          * The other object twin should already exist in a different twin family.
          */

         if (EVAL(FindObjectTwin(((POBJECTTWIN)pot)->ptfParent->hbr,
                                 hpathMatchingFolder,
                                 GetString(((POBJECTTWIN)pot)->ptfParent->hsName),
                                 &hnodeOther)))
         {
            PCOBJECTTWIN pcotOther;

            pcotOther = (PCOBJECTTWIN)GetNodeData(hnodeOther);

            if (EVAL(pcotOther->ptfParent != ((POBJECTTWIN)pot)->ptfParent))
            {
               /* It does.  Crush them. */

               CollapseTwinFamilies(((POBJECTTWIN)pot)->ptfParent,
                                    pcotOther->ptfParent);

               TRACE_OUT((TEXT("BuildBradyBunch(): Collapsed separate twin families for object %s\\%s and %s\\%s."),
                          DebugGetPathString(((POBJECTTWIN)pot)->hpath),
                          GetString(((POBJECTTWIN)pot)->ptfParent->hsName),
                          DebugGetPathString(pcotOther->hpath),
                          GetString(pcotOther->ptfParent->hsName)));
            }
         }
      }

      DeletePath(hpathMatchingFolder);
   }

   return(bResult);
}


/*
** CreateObjectTwinAndAddToList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CreateObjectTwinAndAddToList(PTWINFAMILY ptf, HPATH hpathFolder,
                                          HLIST hlistObjectTwins,
                                          POBJECTTWIN *ppot, PHNODE phnode)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));
   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));
   ASSERT(IS_VALID_HANDLE(hlistObjectTwins, LIST));
   ASSERT(IS_VALID_WRITE_PTR(ppot, POBJECTTWIN));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   if (CreateObjectTwin(ptf, hpathFolder, ppot))
   {
      if (InsertNodeAtFront(hlistObjectTwins, NULL, *ppot, phnode))
         bResult = TRUE;
      else
         DestroyStub(&((*ppot)->stub));
   }

   return(bResult);
}


/*
** CreateListOfGeneratedObjectTwins()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CreateListOfGeneratedObjectTwins(PCFOLDERPAIR pcfp,
                                             PHLIST phlistGeneratedObjectTwins)
{
   NEWLIST nl;
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));
   ASSERT(IS_VALID_WRITE_PTR(phlistGeneratedObjectTwins, HLIST));

   nl.dwFlags = 0;

   if (CreateList(&nl, phlistGeneratedObjectTwins))
   {
      if (EnumGeneratedObjectTwins(pcfp, &InsertNodeAtFrontWalker, *phlistGeneratedObjectTwins))
         bResult = TRUE;
      else
         DestroyList(*phlistGeneratedObjectTwins);
   }

   return(bResult);
}


/*
** NotifyNewObjectTwins()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void NotifyNewObjectTwins(HLIST hlistNewObjectTwins,
                                       HCLSIFACECACHE hcic)
{
   BOOL bContinue;
   HNODE hnode;

   ASSERT(IS_VALID_HANDLE(hlistNewObjectTwins, LIST));
   ASSERT(IS_VALID_HANDLE(hcic, CLSIFACECACHE));

   for (bContinue = GetFirstNode(hlistNewObjectTwins, &hnode);
        bContinue;
        bContinue = GetNextNode(hnode, &hnode))
   {
      PCOBJECTTWIN pcot;
      TCHAR rgchPath[MAX_PATH_LEN];
      CLSID clsidReplicaNotification;

      pcot = (PCOBJECTTWIN)GetNodeData(hnode);

      GetPathString(pcot->hpath, rgchPath);
      CatPath(rgchPath, GetString(pcot->ptfParent->hsName));
      ASSERT(lstrlen(rgchPath) < ARRAYSIZE(rgchPath));

      if (SUCCEEDED(GetReplicaNotificationClassID(rgchPath,
                                                  &clsidReplicaNotification)))
      {
         PINotifyReplica pinr;

         if (SUCCEEDED(GetClassInterface(hcic, &clsidReplicaNotification,
                                         &IID_INotifyReplica, &pinr)))
            /* Ignore return value. */
            NotifyOneNewObjectTwin(pinr, pcot, rgchPath);
         else
            TRACE_OUT((TEXT("NotifyNewObjectTwins(): Failed to get INotifyReplica for replica %s."),
                       rgchPath));
      }
      else
         TRACE_OUT((TEXT("NotifyNewObjectTwins(): Failed to get replica notification class ID for replica %s."),
                    rgchPath));
   }

   return;
}


/*
** NotifyOneNewObjectTwin()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT NotifyOneNewObjectTwin(PINotifyReplica pinr, PCOBJECTTWIN pcot,
                                       LPCTSTR pcszPath)
{
   HRESULT hr;
   HSTGIFACE hstgi;

   ASSERT(IS_VALID_STRUCT_PTR(pinr, CINotifyReplica));
   ASSERT(IS_VALID_STRUCT_PTR(pcot, COBJECTTWIN));
   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));

   hr = GetStorageInterface((PIUnknown)pinr, &hstgi);

   if (SUCCEEDED(hr))
   {
      hr = LoadFromStorage(hstgi, pcszPath);

      if (SUCCEEDED(hr))
      {
         ULONG ulcOtherReplicas;
         PIMoniker *ppimkOtherReplicas;

         /*
          * RAIDRAID: (16270) (Performance) We may create a file moniker for
          * the same object twin multiple times here.
          */

         hr = CreateOtherReplicaMonikers(pcot, &ulcOtherReplicas,
                                         &ppimkOtherReplicas);

         if (SUCCEEDED(hr))
         {
            hr = pinr->lpVtbl->YouAreAReplica(pinr, ulcOtherReplicas,
                                              ppimkOtherReplicas);

            if (SUCCEEDED(hr))
            {
               hr = SaveToStorage(hstgi);

               if (SUCCEEDED(hr))
                  TRACE_OUT((TEXT("NotifyOneNewObjectTwin(): Replica %s successfully notified."),
                             pcszPath));
               else
                  WARNING_OUT((TEXT("NotifyOneNewObjectTwin(): Failed to save replica %s to storage."),
                               pcszPath));
            }
            else
               WARNING_OUT((TEXT("NotifyOneNewObjectTwin(): Failed to notify replica %s."),
                            pcszPath));

            ReleaseIUnknowns(ulcOtherReplicas,
                             (PIUnknown *)ppimkOtherReplicas);
         }
         else
            WARNING_OUT((TEXT("NotifyOneNewObjectTwin(): Failed to create monikers for other replicas of replica %s."),
                         pcszPath));
      }
      else
         WARNING_OUT((TEXT("NotifyOneNewObjectTwin(): Failed to load replica %s from storage."),
                      pcszPath));

      ReleaseStorageInterface(hstgi);
   }
   else
      WARNING_OUT((TEXT("NotifyOneNewObjectTwin(): Failed to get storage interface for replica %s."),
                   pcszPath));

   return(hr);
}


/*
** CreateOtherReplicaMonikers()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE HRESULT CreateOtherReplicaMonikers(PCOBJECTTWIN pcotMaster,
                                           PULONG pulcOtherReplicas,
                                           PIMoniker **pppimk)
{
   HRESULT hr;
   HLIST hlist;
   ULONG ulcOtherReplicas;

   ASSERT(IS_VALID_STRUCT_PTR(pcotMaster, COBJECTTWIN));
   ASSERT(IS_VALID_WRITE_PTR(pulcOtherReplicas, ULONG));
   ASSERT(IS_VALID_WRITE_PTR(pppimk, PIMoniker *));

   hlist = pcotMaster->ptfParent->hlistObjectTwins;

   ulcOtherReplicas = GetNodeCount(hlist);
   ASSERT(ulcOtherReplicas > 0);
   ulcOtherReplicas--;

   if (AllocateMemory(ulcOtherReplicas * sizeof(**pppimk), (PVOID *)pppimk))
   {
      BOOL bContinue;
      HNODE hnode;

      hr = S_OK;
      *pulcOtherReplicas = 0;

      for (bContinue = GetFirstNode(hlist, &hnode);
           bContinue;
           bContinue = GetNextNode(hnode, &hnode))
      {
         PCOBJECTTWIN pcot;

         pcot = (PCOBJECTTWIN)GetNodeData(hnode);

         if (pcot != pcotMaster)
         {
            TCHAR rgchPath[MAX_PATH_LEN];

            GetPathString(pcot->hpath, rgchPath);

            hr = MyCreateFileMoniker(rgchPath,
                                     GetString(pcot->ptfParent->hsName),
                                     &((*pppimk)[*pulcOtherReplicas]));

            if (SUCCEEDED(hr))
            {
               ASSERT(*pulcOtherReplicas < ulcOtherReplicas);
               (*pulcOtherReplicas)++;
            }
            else
               break;
         }
      }

      if (FAILED(hr))
         ReleaseIUnknowns(*pulcOtherReplicas, *(PIUnknown **)pppimk);
   }
   else
      hr = E_OUTOFMEMORY;

   return(hr);
}


/*
** TwinFamilySortCmp()
**
** Pointer comparison function used to sort the global array of twin families.
**
** Arguments:     pctf1 - pointer to TWINFAMILY describing first twin family
**                pctf2 - pointer to TWINFAMILY describing second twin family
**
** Returns:
**
** Side Effects:  none
**
** Twin families are sorted by:
**    1) name string
**    2) pointer value
*/
PRIVATE_CODE COMPARISONRESULT TwinFamilySortCmp(PCVOID pctf1, PCVOID pctf2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pctf1, CTWINFAMILY));
   ASSERT(IS_VALID_STRUCT_PTR(pctf2, CTWINFAMILY));

   cr = CompareNameStringsByHandle(((PCTWINFAMILY)pctf1)->hsName, ((PCTWINFAMILY)pctf2)->hsName);

   if (cr == CR_EQUAL)
      /* Same name strings.  Now sort by pointer value. */
      cr = ComparePointers(pctf1, pctf2);

   return(cr);
}


/*
** TwinFamilySearchCmp()
**
** Pointer comparison function used to search the global array of twin families
** for the first twin family for a given name.
**
** Arguments:     pcszName - name string to search for
**                pctf - pointer to TWINFAMILY to examine
**
** Returns:
**
** Side Effects:  none
**
** Twin families are searched by:
**    1) name string
*/
PRIVATE_CODE COMPARISONRESULT TwinFamilySearchCmp(PCVOID pcszName, PCVOID pctf)
{
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));
   ASSERT(IS_VALID_STRUCT_PTR(pctf, CTWINFAMILY));

   return(CompareNameStrings(pcszName, GetString(((PCTWINFAMILY)pctf)->hsName)));
}


/*
** ObjectTwinSearchCmp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL ObjectTwinSearchCmp(PCVOID hpath, PCVOID pcot)
{
   ASSERT(IS_VALID_HANDLE((HPATH)hpath, PATH));
   ASSERT(IS_VALID_STRUCT_PTR(pcot, COBJECTTWIN));

   return(ComparePaths((HPATH)hpath, ((PCOBJECTTWIN)pcot)->hpath));
}


/*
** WriteTwinFamily()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT WriteTwinFamily(HCACHEDFILE hcf, PCTWINFAMILY pctf)
{
   TWINRESULT tr = TR_BRIEFCASE_WRITE_FAILED;
   DWORD dwcbTwinFamilyDBHeaderOffset;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(pctf, CTWINFAMILY));

   /* Save initial file poisition. */

   dwcbTwinFamilyDBHeaderOffset = GetCachedFilePointerPosition(hcf);

   if (dwcbTwinFamilyDBHeaderOffset != INVALID_SEEK_POSITION)
   {
      TWINFAMILYDBHEADER tfdbh;

      /* Leave space for the twin family's header. */

      ZeroMemory(&tfdbh, sizeof(tfdbh));

      if (WriteToCachedFile(hcf, (PCVOID)&tfdbh, sizeof(tfdbh), NULL))
      {
         BOOL bContinue;
         HNODE hnode;
         LONG lcObjectTwins = 0;

         /* Save twin family's object twins. */

         ASSERT(GetNodeCount(pctf->hlistObjectTwins) >= 2);

         tr = TR_SUCCESS;

         for (bContinue = GetFirstNode(pctf->hlistObjectTwins, &hnode);
              bContinue;
              bContinue = GetNextNode(hnode, &hnode))
         {
            POBJECTTWIN pot;

            pot = (POBJECTTWIN)GetNodeData(hnode);

            tr = WriteObjectTwin(hcf, pot);

            if (tr == TR_SUCCESS)
            {
               ASSERT(lcObjectTwins < LONG_MAX);
               lcObjectTwins++;
            }
            else
               break;
         }

         /* Save twin family's database header. */

         if (tr == TR_SUCCESS)
         {
            ASSERT(lcObjectTwins >= 2);

            tfdbh.dwStubFlags = (pctf->stub.dwFlags & DB_STUB_FLAGS_MASK);
            tfdbh.hsName = pctf->hsName;
            tfdbh.lcObjectTwins = lcObjectTwins;

            tr = WriteDBSegmentHeader(hcf, dwcbTwinFamilyDBHeaderOffset, &tfdbh, sizeof(tfdbh));
         }
      }
   }

   return(tr);
}


/*
** WriteObjectTwin()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT WriteObjectTwin(HCACHEDFILE hcf, PCOBJECTTWIN pcot)
{
   TWINRESULT tr;
   DBOBJECTTWIN dbot;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(pcot, COBJECTTWIN));

   /* Set up object twin database structure. */

   dbot.dwStubFlags = (pcot->stub.dwFlags & DB_STUB_FLAGS_MASK);
   dbot.hpath = pcot->hpath;
   dbot.hpath = pcot->hpath;
   dbot.fsLastRec = pcot->fsLastRec;

   /* Save object twin database structure. */

   if (WriteToCachedFile(hcf, (PCVOID)&dbot, sizeof(dbot), NULL))
      tr = TR_SUCCESS;
   else
      tr = TR_BRIEFCASE_WRITE_FAILED;

   return(tr);
}


/*
** ReadTwinFamily()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT ReadTwinFamily(HCACHEDFILE hcf, HBRFCASE hbr,
                                  PCDBVERSION pcdbver,
                                  HHANDLETRANS hhtFolderTrans,
                                  HHANDLETRANS hhtNameTrans)
{
   TWINRESULT tr = TR_CORRUPT_BRIEFCASE;
   TWINFAMILYDBHEADER tfdbh;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_READ_PTR(pcdbver, DBVERSION));
   ASSERT(IS_VALID_HANDLE(hhtFolderTrans, HANDLETRANS));
   ASSERT(IS_VALID_HANDLE(hhtNameTrans, HANDLETRANS));

   if (ReadFromCachedFile(hcf, &tfdbh, sizeof(tfdbh), &dwcbRead) &&
       dwcbRead == sizeof(tfdbh))
   {
      if (tfdbh.lcObjectTwins >= 2)
      {
         HSTRING hsName;

         if (TranslateHandle(hhtNameTrans, (HGENERIC)(tfdbh.hsName), (PHGENERIC)&hsName))
         {
            PTWINFAMILY ptfParent;

            if (CreateTwinFamily(hbr, GetString(hsName), &ptfParent))
            {
               LONG l;

               CopyTwinFamilyInfo(ptfParent, &tfdbh);

               tr = TR_SUCCESS;

               for (l = tfdbh.lcObjectTwins;
                    l > 0 && tr == TR_SUCCESS;
                    l--)
                  tr = ReadObjectTwin(hcf, pcdbver, ptfParent, hhtFolderTrans);

               if (tr != TR_SUCCESS)
                  DestroyStub(&(ptfParent->stub));
            }
            else
               tr = TR_OUT_OF_MEMORY;
         }
      }
   }

   return(tr);
}


/*
** ReadObjectTwin()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT ReadObjectTwin(HCACHEDFILE hcf, 
                                  PCDBVERSION pcdbver,
                                  PTWINFAMILY ptfParent,
                                  HHANDLETRANS hhtFolderTrans)
{
   TWINRESULT tr;
   DBOBJECTTWIN dbot;
   DWORD dwcbRead;
   HPATH hpath;
   DWORD dwcbSize;
   COPYOBJECTTWINPROC pfnCopy;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_READ_PTR(pcdbver, DBVERSION));
   ASSERT(IS_VALID_STRUCT_PTR(ptfParent, CTWINFAMILY));
   ASSERT(IS_VALID_HANDLE(hhtFolderTrans, HANDLETRANS));

   if (HEADER_M8_MINOR_VER == pcdbver->dwMinorVer)
   {
      /* The M8 database does not have the ftModLocal in the FILESTAMP
      ** structure.
      */

      dwcbSize = sizeof(dbot) - sizeof(FILETIME);
      pfnCopy = CopyM8ObjectTwinInfo;
   }
   else
   {
      ASSERT(HEADER_MINOR_VER == pcdbver->dwMinorVer);
      dwcbSize = sizeof(dbot);
      pfnCopy = CopyObjectTwinInfo;
   }

   if ((ReadFromCachedFile(hcf, &dbot, dwcbSize, &dwcbRead) &&
        dwcbRead == dwcbSize) &&
       TranslateHandle(hhtFolderTrans, (HGENERIC)(dbot.hpath), (PHGENERIC)&hpath))
   {
      POBJECTTWIN pot;

      /* Create the new object twin and add it to the twin family. */

      if (CreateObjectTwin(ptfParent, hpath, &pot))
      {
          pfnCopy(pot, &dbot);

          tr = TR_SUCCESS;
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


/*
** CopyTwinFamilyInfo()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE void CopyTwinFamilyInfo(PTWINFAMILY ptf,
                                PCTWINFAMILYDBHEADER pctfdbh)
{
   ASSERT(IS_VALID_WRITE_PTR(ptf, TWINFAMILY));
   ASSERT(IS_VALID_READ_PTR(pctfdbh, CTWINFAMILYDBHEADER));

   ptf->stub.dwFlags = pctfdbh->dwStubFlags;

   return;
}


/*
** CopyObjectTwinInfo()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE void CopyObjectTwinInfo(POBJECTTWIN pot, PCDBOBJECTTWIN pcdbot)
{
   ASSERT(IS_VALID_WRITE_PTR(pot, OBJECTTWIN));
   ASSERT(IS_VALID_READ_PTR(pcdbot, CDBOBJECTTWIN));

   pot->stub.dwFlags = pcdbot->dwStubFlags;
   pot->fsLastRec = pcdbot->fsLastRec;

   return;
}


/*
** CopyM8ObjectTwinInfo()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE void CopyM8ObjectTwinInfo(POBJECTTWIN pot, PCDBOBJECTTWIN pcdbot)
{
   ASSERT(IS_VALID_WRITE_PTR(pot, OBJECTTWIN));
   ASSERT(IS_VALID_READ_PTR(pcdbot, CDBOBJECTTWIN));

   pot->stub.dwFlags = pcdbot->dwStubFlags;
   pot->fsLastRec = pcdbot->fsLastRec;

   /* The pot->fsLastRec.ftModLocal field is invalid, so fill it in */

   if ( !FileTimeToLocalFileTime(&pot->fsLastRec.ftMod, &pot->fsLastRec.ftModLocal) )
   {
      /* Just copy the time if FileTimeToLocalFileTime failed */

      pot->fsLastRec.ftModLocal = pot->fsLastRec.ftMod;
   }

   return;
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

/*
** DestroyObjectTwinStubWalker()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL DestroyObjectTwinStubWalker(PVOID pot, PVOID pvUnused)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(! pvUnused);

   /*
    * Set ulcSrcFolderTwins to 0 so UnlinkObjectTwin() succeeds.
    * DestroyStub() will unlink and destroy any new twin family created.
    */

   ((POBJECTTWIN)pot)->ulcSrcFolderTwins = 0;
   DestroyStub(&(((POBJECTTWIN)pot)->stub));

   return(TRUE);
}


/*
** MarkObjectTwinNeverReconciledWalker()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL MarkObjectTwinNeverReconciledWalker(PVOID pot, PVOID pvUnused)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(! pvUnused);

   MarkObjectTwinNeverReconciled(pot);

   return(TRUE);
}


/*
** LookForSrcFolderTwinsWalker()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL LookForSrcFolderTwinsWalker(PVOID pot, PVOID pvUnused)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(! pvUnused);

   return(! ((POBJECTTWIN)pot)->ulcSrcFolderTwins);
}


/*
** IncrementSrcFolderTwinsWalker()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IncrementSrcFolderTwinsWalker(PVOID pot, PVOID pvUnused)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(! pvUnused);

   ASSERT(((POBJECTTWIN)pot)->ulcSrcFolderTwins < ULONG_MAX);
   ((POBJECTTWIN)pot)->ulcSrcFolderTwins++;

   return(TRUE);
}


/*
** ClearSrcFolderTwinsWalker()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL ClearSrcFolderTwinsWalker(PVOID pot, PVOID pvUnused)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(! pvUnused);

   ((POBJECTTWIN)pot)->ulcSrcFolderTwins = 0;

   return(TRUE);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


/*
** SetTwinFamilyWalker()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL SetTwinFamilyWalker(PVOID pot, PVOID ptfParent)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(IS_VALID_STRUCT_PTR(ptfParent, CTWINFAMILY));

   ((POBJECTTWIN)pot)->ptfParent = ptfParent;

   return(TRUE);
}


/*
** InsertNodeAtFrontWalker()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL InsertNodeAtFrontWalker(POBJECTTWIN pot, PVOID hlist)
{
   HNODE hnodeUnused;

   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(IS_VALID_HANDLE(hlist, LIST));

   return(InsertNodeAtFront(hlist, NULL, pot, &hnodeUnused));
}


#ifdef VSTF

/*
** IsValidObjectTwinWalker()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidObjectTwinWalker(PVOID pcot, PVOID pctfParent)
{
   return(IS_VALID_STRUCT_PTR(pcot, COBJECTTWIN) &&
          EVAL(((PCOBJECTTWIN)pcot)->ptfParent == pctfParent) &&
          EVAL(IsStubFlagClear(&(((PCOBJECTTWIN)pcot)->stub), STUB_FL_KEEP) ||
               IsStubFlagSet(&(((PCTWINFAMILY)pctfParent)->stub),
                             STUB_FL_DELETION_PENDING)));
}


/*
** IsValidPCNEWOBJECTTWIN()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCNEWOBJECTTWIN(PCNEWOBJECTTWIN pcnot)
{
   return(IS_VALID_READ_PTR(pcnot, CNEWOBJECTTWIN) &&
          EVAL(pcnot->ulSize == sizeof(*pcnot)) &&
          IS_VALID_STRING_PTR(pcnot->pcszFolder1, CSTR) &&
          IS_VALID_STRING_PTR(pcnot->pcszFolder2, CSTR) &&
          IS_VALID_STRING_PTR(pcnot->pcszName, CSTR));
}


/*
** IsValidPCSPINOFFOBJECTTWININFO()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCSPINOFFOBJECTTWININFO(PCSPINOFFOBJECTTWININFO pcsooti)
{
   return(IS_VALID_READ_PTR(pcsooti, CSPINOFFOBJECTTWININFO) &&
          IS_VALID_STRUCT_PTR(pcsooti->pcfp, CFOLDERPAIR) &&
          IS_VALID_HANDLE(pcsooti->hlistNewObjectTwins, LIST));
}

#endif


#ifdef DEBUG

/*
** AreTwinFamiliesValid()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL AreTwinFamiliesValid(HPTRARRAY hpaTwinFamilies)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hpaTwinFamilies, PTRARRAY));

   aicPtrs = GetPtrCount(hpaTwinFamilies);

   for (ai = 0; ai < aicPtrs; ai++)
   {
      PCTWINFAMILY pctf;

      pctf = GetPtr(hpaTwinFamilies, ai);

      if (! IS_VALID_STRUCT_PTR(pctf, CTWINFAMILY) ||
          ! EVAL(GetNodeCount(pctf->hlistObjectTwins) >= 2))
         break;
   }

   return(ai == aicPtrs);
}

#endif


/****************************** Public Functions *****************************/


/*
** CompareNameStrings()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE COMPARISONRESULT CompareNameStrings(LPCTSTR pcszFirst, LPCTSTR pcszSecond)
{
   ASSERT(IS_VALID_STRING_PTR(pcszFirst, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszSecond, CSTR));

   return(MapIntToComparisonResult(lstrcmpi(pcszFirst, pcszSecond)));
}


/*
** CompareNameStringsByHandle()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE COMPARISONRESULT CompareNameStringsByHandle(HSTRING hsFirst,
                                                        HSTRING hsSecond)
{
   ASSERT(IS_VALID_HANDLE(hsFirst, STRING));
   ASSERT(IS_VALID_HANDLE(hsSecond, STRING));

   return(CompareStringsI(hsFirst, hsSecond));
}


/*
** TranslatePATHRESULTToTWINRESULT()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT TranslatePATHRESULTToTWINRESULT(PATHRESULT pr)
{
   TWINRESULT tr;

   switch (pr)
   {
      case PR_SUCCESS:
         tr = TR_SUCCESS;
         break;

      case PR_UNAVAILABLE_VOLUME:
         tr = TR_UNAVAILABLE_VOLUME;
         break;

      case PR_OUT_OF_MEMORY:
         tr = TR_OUT_OF_MEMORY;
         break;

      default:
         ASSERT(pr == PR_INVALID_PATH);
         tr = TR_INVALID_PARAMETER;
         break;
   }

   return(tr);
}


/*
** CreateTwinFamilyPtrArray()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CreateTwinFamilyPtrArray(PHPTRARRAY phpa)
{
   NEWPTRARRAY npa;

   ASSERT(IS_VALID_WRITE_PTR(phpa, HPTRARRAY));

   /* Try to create a twin family pointer array. */

   npa.aicInitialPtrs = NUM_START_TWIN_FAMILY_PTRS;
   npa.aicAllocGranularity = NUM_TWIN_FAMILY_PTRS_TO_ADD;
   npa.dwFlags = NPA_FL_SORTED_ADD;

   return(CreatePtrArray(&npa, phpa));
}


/*
** DestroyTwinFamilyPtrArray()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DestroyTwinFamilyPtrArray(HPTRARRAY hpa)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hpa, PTRARRAY));

   /* First free all twin families in pointer array. */

   aicPtrs = GetPtrCount(hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      DestroyTwinFamily(GetPtr(hpa, ai));

   /* Now wipe out the pointer array. */

   DestroyPtrArray(hpa);

   return;
}


/*
** GetTwinBriefcase()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HBRFCASE GetTwinBriefcase(HTWIN htwin)
{
   HBRFCASE hbr;

   ASSERT(IS_VALID_HANDLE(htwin, TWIN));

   switch (((PSTUB)htwin)->st)
   {
      case ST_OBJECTTWIN:
         hbr = ((PCOBJECTTWIN)htwin)->ptfParent->hbr;
         break;

      case ST_TWINFAMILY:
         hbr = ((PCTWINFAMILY)htwin)->hbr;
         break;

      case ST_FOLDERPAIR:
         hbr = ((PCFOLDERPAIR)htwin)->pfpd->hbr;
         break;

      default:
         ERROR_OUT((TEXT("GetTwinBriefcase() called on unrecognized stub type %d."),
                    ((PSTUB)htwin)->st));
         hbr = NULL;
         break;
   }

   return(hbr);
}


/*
** FindObjectTwinInList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL FindObjectTwinInList(HLIST hlist, HPATH hpath, PHNODE phnode)
{
   ASSERT(IS_VALID_HANDLE(hlist, LIST));
   ASSERT(IS_VALID_HANDLE(hpath, PATH));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   return(SearchUnsortedList(hlist, &ObjectTwinSearchCmp, hpath, phnode));
}


/*
** EnumTwins()
**
** Enumerates folder twins and twin families in a briefcase.
**
** Arguments:
**
** Returns:       TRUE if halted.  FALSE if not.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL EnumTwins(HBRFCASE hbr, ENUMTWINSPROC etp, LPARAM lpData,
                           PHTWIN phtwinStop)
{
   HPTRARRAY hpa;
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_CODE_PTR(etp, ENUMTWINSPROC));
   ASSERT(IS_VALID_WRITE_PTR(phtwinStop, HTWIN));

   /* Enumerate folder pairs. */

   *phtwinStop = NULL;

   hpa = GetBriefcaseFolderPairPtrArray(hbr);

   aicPtrs = GetPtrCount(hpa);

   for (ai = 0; ai < aicPtrs; ai++)
   {
      PCFOLDERPAIR pcfp;

      pcfp = GetPtr(hpa, ai);

      ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));

      if (! (*etp)((HTWIN)pcfp, lpData))
      {
         *phtwinStop = (HTWIN)pcfp;
         break;
      }
   }

   if (! *phtwinStop)
   {
      /* Enumerate twin families. */

      hpa = GetBriefcaseTwinFamilyPtrArray(hbr);

      aicPtrs = GetPtrCount(hpa);

      for (ai = 0; ai < aicPtrs; ai++)
      {
         PCTWINFAMILY pctf;

         pctf = GetPtr(hpa, ai);

         ASSERT(IS_VALID_STRUCT_PTR(pctf, CTWINFAMILY));

         if (! (*etp)((HTWIN)pctf, lpData))
         {
            *phtwinStop = (HTWIN)pctf;
            break;
         }
      }
   }

   return(*phtwinStop != NULL);
}


/*
** FindObjectTwin()
**
** Looks for a twin family containing a specified object twin.
**
** Arguments:     hpathFolder - folder containing object
**                pcszName - name of object
**
** Returns:       Handle to list node containing pointer to object twin if
**                found, or NULL if not found.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL FindObjectTwin(HBRFCASE hbr, HPATH hpathFolder,
                                LPCTSTR pcszName, PHNODE phnode)
{
   BOOL bFound = FALSE;
   HPTRARRAY hpaTwinFamilies;
   ARRAYINDEX aiFirst;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   /* Search for a matching twin family. */

   *phnode = NULL;

   hpaTwinFamilies = GetBriefcaseTwinFamilyPtrArray(hbr);

   if (SearchSortedArray(hpaTwinFamilies, &TwinFamilySearchCmp, pcszName,
                         &aiFirst))
   {
      ARRAYINDEX aicPtrs;
      ARRAYINDEX ai;
      PTWINFAMILY ptf;

      /*
       * aiFirst holds the index of the first twin family with a common object
       * name matching pcszName.
       */

      /*
       * Now search each of these twin families for a folder matching
       * pcszFolder.
       */

      aicPtrs = GetPtrCount(hpaTwinFamilies);

      ASSERT(aicPtrs > 0);
      ASSERT(aiFirst >= 0);
      ASSERT(aiFirst < aicPtrs);

      for (ai = aiFirst; ai < aicPtrs; ai++)
      {
         ptf = GetPtr(hpaTwinFamilies, ai);

         ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));

         /* Is this a twin family of objects of the given name? */

         if (CompareNameStrings(GetString(ptf->hsName), pcszName) == CR_EQUAL)
         {
            bFound = SearchUnsortedList(ptf->hlistObjectTwins,
                                        &ObjectTwinSearchCmp, hpathFolder,
                                        phnode);

            if (bFound)
               break;
         }
         else
            /* No.  Stop searching. */
            break;
      }
   }

   return(bFound);
}


/*
** TwinObjects()
**
** Twins two objects.
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
**
** N.b., *ppot1 and *ppot2 are valid if TR_SUCCESS or TR_DUPLICATE_TWIN is
** returned.
*/
PUBLIC_CODE TWINRESULT TwinObjects(HBRFCASE hbr, HCLSIFACECACHE hcic,
                                   HPATH hpathFolder1, HPATH hpathFolder2,
                                   LPCTSTR pcszName, POBJECTTWIN *ppot1,
                                   POBJECTTWIN *ppot2)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hcic, CLSIFACECACHE));
   ASSERT(IS_VALID_HANDLE(hpathFolder1, PATH));
   ASSERT(IS_VALID_HANDLE(hpathFolder2, PATH));
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppot1, POBJECTTWIN));
   ASSERT(IS_VALID_WRITE_PTR(ppot2, POBJECTTWIN));

   /* Fail twinning a file to itself. */

   if (ComparePaths(hpathFolder1, hpathFolder2) != CR_EQUAL)
   {
      NEWLIST nl;
      HLIST hlistNewObjectTwins;

      nl.dwFlags = 0;

      if (CreateList(&nl, &hlistNewObjectTwins))
      {
         /* Twin 'em. */

         tr = TwinJustTheseTwoObjects(hbr, hpathFolder1, hpathFolder2,
                                      pcszName, ppot1, ppot2,
                                      hlistNewObjectTwins);

         /*
          * Add any new object twins to the lists of generated object twins for
          * all intersecting folder twins.  Create new spin-off object twins
          * from the other folder twin connected to each intersecting folder
          * twin.  Spin-off object twins are added to the twin family as they
          * are created.
          */

         if (tr == TR_SUCCESS)
         {
            if (ApplyNewObjectTwinsToFolderTwins(hlistNewObjectTwins))
            {
               /*
                * Notify new object twins that they are object twins.  Don't
                * notify folder object twins.
                */

               if (*pcszName)
                  NotifyNewObjectTwins(hlistNewObjectTwins, hcic);
            }
            else
               tr = TR_OUT_OF_MEMORY;
         }

         if (tr != TR_SUCCESS)
            /*
             * We must maintain a consistent internal state by deleting any new
             * twin family and object twins on failure, independent of source
             * folder twin count.
             */
            EVAL(WalkList(hlistNewObjectTwins, &DestroyObjectTwinStubWalker,
                          NULL));

         DestroyList(hlistNewObjectTwins);
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
      tr = TR_SAME_FOLDER;

   ASSERT((tr != TR_SUCCESS && tr != TR_DUPLICATE_TWIN) ||
          IS_VALID_STRUCT_PTR(*ppot1, COBJECTTWIN) && IS_VALID_STRUCT_PTR(*ppot2, COBJECTTWIN));

   return(tr);
}


/*
** CreateObjectTwin()
**
** Creates a new object twin, and adds it to a twin family.
**
** Arguments:     ptf - pointer to parent twin family
**                hpathFolder - folder of new object twin
**
** Returns:       Pointer to new object twin if successful, or NULL if
**                unsuccessful.
**
** Side Effects:  none
**
** N.b., this function does not first check to see if the object twin already
** exists in the family.
*/
PUBLIC_CODE BOOL CreateObjectTwin(PTWINFAMILY ptf, HPATH hpathFolder,
                             POBJECTTWIN *ppot)
{
   BOOL bResult = FALSE;
   POBJECTTWIN potNew;

   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));
   ASSERT(IS_VALID_HANDLE(hpathFolder, PATH));
   ASSERT(IS_VALID_WRITE_PTR(ppot, POBJECTTWIN));

#ifdef DEBUG

   {
      HNODE hnodeUnused;

      /* Is this object twin already in a twin family? */

      if (FindObjectTwin(ptf->hbr, hpathFolder, GetString(ptf->hsName), &hnodeUnused))
         ERROR_OUT((TEXT("CreateObjectTwin(): An object twin for %s\\%s already exists."),
                    DebugGetPathString(hpathFolder),
                    GetString(ptf->hsName)));
   }

#endif

   /* Create a new OBJECTTWIN structure. */

   if (AllocateMemory(sizeof(*potNew), &potNew))
   {
      if (CopyPath(hpathFolder, GetBriefcasePathList(ptf->hbr), &(potNew->hpath)))
      {
         HNODE hnodeUnused;

         /* Fill in new OBJECTTWIN fields. */

         InitStub(&(potNew->stub), ST_OBJECTTWIN);

         potNew->ptfParent = ptf;
         potNew->ulcSrcFolderTwins = 0;

         MarkObjectTwinNeverReconciled(potNew);

         /* Add the object twin to the twin family's list of object twins. */

         if (InsertNodeAtFront(ptf->hlistObjectTwins, NULL, potNew, &hnodeUnused))
         {
            *ppot = potNew;
            bResult = TRUE;

            ASSERT(IS_VALID_STRUCT_PTR(*ppot, COBJECTTWIN));
         }
         else
         {
            DeletePath(potNew->hpath);
CREATEOBJECTTWIN_BAIL:
            FreeMemory(potNew);
         }
      }
      else
         goto CREATEOBJECTTWIN_BAIL;
   }

   return(bResult);
}


/*
** UnlinkObjectTwin()
**
** Unlinks an object twin.
**
** Arguments:     pot - pointer to object twin to unlink
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT UnlinkObjectTwin(POBJECTTWIN pot)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));

   ASSERT(IsStubFlagClear(&(pot->stub), STUB_FL_UNLINKED));

   TRACE_OUT((TEXT("UnlinkObjectTwin(): Unlinking object twin for folder %s."),
              DebugGetPathString(pot->hpath)));

   /* Is the object twin's twin family being deleted? */

   if (IsStubFlagSet(&(pot->ptfParent->stub), STUB_FL_BEING_DELETED))
      /* Yes.  No need to unlink the object twin. */
      tr = TR_SUCCESS;
   else
   {
      /* Are there any folder twin sources left for this object twin? */

      if (! pot->ulcSrcFolderTwins)
      {
         HNODE hnode;

         /*
          * Search the object twin's parent's list of object twins for the
          * object twin to be unlinked.
          */

         if (EVAL(FindObjectTwinInList(pot->ptfParent->hlistObjectTwins, pot->hpath, &hnode)) &&
             EVAL(GetNodeData(hnode) == pot))
         {
            ULONG ulcRemainingObjectTwins;

            /* Unlink the object twin. */

            DeleteNode(hnode);

            SetStubFlag(&(pot->stub), STUB_FL_UNLINKED);

            /*
             * If we have just unlinked the second last object twin in a twin
             * family, destroy the twin family.
             */

            ulcRemainingObjectTwins = GetNodeCount(pot->ptfParent->hlistObjectTwins);

            if (ulcRemainingObjectTwins < 2)
            {

#ifdef DEBUG

               TCHAR rgchName[MAX_NAME_LEN];

               lstrcpy(rgchName, GetString(pot->ptfParent->hsName));

#endif

               /* It's the end of the family line. */

               tr = DestroyStub(&(pot->ptfParent->stub));

#ifdef DEBUG

               if (tr == TR_SUCCESS)
                  TRACE_OUT((TEXT("UnlinkObjectTwin(): Implicitly destroyed twin family for object %s."),
                             rgchName));

#endif

               if (ulcRemainingObjectTwins == 1 &&
                   tr == TR_HAS_FOLDER_TWIN_SRC)
                  tr = TR_SUCCESS;
            }
            else
               tr = TR_SUCCESS;
         }
         else
            tr = TR_INVALID_PARAMETER;

         ASSERT(tr == TR_SUCCESS);
      }
      else
         tr = TR_HAS_FOLDER_TWIN_SRC;
   }

   return(tr);
}


/*
** DestroyObjectTwin()
**
** Destroys an object twin.
**
** Arguments:     pot - pointer to object twin to destroy
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void DestroyObjectTwin(POBJECTTWIN pot)
{
   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));

   TRACE_OUT((TEXT("DestroyObjectTwin(): Destroying object twin for folder %s."),
              DebugGetPathString(pot->hpath)));

   DeletePath(pot->hpath);
   FreeMemory(pot);

   return;
}


/*
** UnlinkTwinFamily()
**
** Unlinks a twin family.
**
** Arguments:     ptf - pointer to twin family to unlink
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT UnlinkTwinFamily(PTWINFAMILY ptf)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));

   ASSERT(IsStubFlagClear(&(ptf->stub), STUB_FL_UNLINKED));
   ASSERT(IsStubFlagClear(&(ptf->stub), STUB_FL_BEING_DELETED));

   /*
    * A twin family containing object twins generated by folder twins may not
    * be deleted, since those object twins may not be directly deleted.
    */

   if (WalkList(ptf->hlistObjectTwins, &LookForSrcFolderTwinsWalker, NULL))
   {
      HPTRARRAY hpaTwinFamilies;
      ARRAYINDEX aiUnlink;

      TRACE_OUT((TEXT("UnlinkTwinFamily(): Unlinking twin family for object %s."),
                 GetString(ptf->hsName)));

      /* Search for the twin family to be unlinked. */

      hpaTwinFamilies = GetBriefcaseTwinFamilyPtrArray(ptf->hbr);

      if (EVAL(SearchSortedArray(hpaTwinFamilies, &TwinFamilySortCmp, ptf,
                                 &aiUnlink)))
      {
         /* Unlink the twin family. */

         ASSERT(GetPtr(hpaTwinFamilies, aiUnlink) == ptf);

         DeletePtr(hpaTwinFamilies, aiUnlink);

         SetStubFlag(&(ptf->stub), STUB_FL_UNLINKED);
      }

      tr = TR_SUCCESS;
   }
   else
      tr = TR_HAS_FOLDER_TWIN_SRC;

   return(tr);
}


/*
** DestroyTwinFamily()
**
** Destroys a twin family.
**
** Arguments:     ptf - pointer to twin family to destroy
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void DestroyTwinFamily(PTWINFAMILY ptf)
{
   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));

   ASSERT(IsStubFlagClear(&(ptf->stub), STUB_FL_BEING_DELETED));

   TRACE_OUT((TEXT("DestroyTwinFamily(): Destroying twin family for object %s."),
              GetString(ptf->hsName)));

   SetStubFlag(&(ptf->stub), STUB_FL_BEING_DELETED);

   /*
    * Destroy the object twins in the family one by one.  Be careful not to use
    * an object twin after it has been destroyed.
    */

   EVAL(WalkList(ptf->hlistObjectTwins, &DestroyObjectTwinStubWalker, NULL));

   /* Destroy TWINFAMILY fields. */

   DestroyList(ptf->hlistObjectTwins);
   DeleteString(ptf->hsName);
   FreeMemory(ptf);

   return;
}


/*
** MarkTwinFamilyNeverReconciled()
**
** Marks a twin family as never reconciled.
**
** Arguments:     ptf - pointer to twin family to be marked never reconciled
**
** Returns:       void
**
** Side Effects:  Clears the twin family's last reconciliation time stamp.
**                Marks all the object twins in the family never reconciled.
*/
PUBLIC_CODE void MarkTwinFamilyNeverReconciled(PTWINFAMILY ptf)
{
   /*
    * If we're being called from CreateTwinFamily(), the fields we're about to
    * set may currently be invalid.  Don't fully verify the TWINFAMILY
    * structure.
    */

   ASSERT(IS_VALID_WRITE_PTR(ptf, TWINFAMILY));

   /* Mark all object twins in twin family as never reconciled. */

   EVAL(WalkList(ptf->hlistObjectTwins, MarkObjectTwinNeverReconciledWalker, NULL));

   return;
}


/*
** MarkObjectTwinNeverReconciled()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE void MarkObjectTwinNeverReconciled(PVOID pot)
{
   /*
    * If we're being called from CreateObjectTwin(), the fields we're about to
    * set may currently be invalid.  Don't fully verify the OBJECTTWIN
    * structure.
    */

   ASSERT(IS_VALID_WRITE_PTR((PCOBJECTTWIN)pot, COBJECTTWIN));

   ASSERT(IsStubFlagClear(&(((PCOBJECTTWIN)pot)->stub), STUB_FL_NOT_RECONCILED));

   ZeroMemory(&(((POBJECTTWIN)pot)->fsLastRec),
              sizeof(((POBJECTTWIN)pot)->fsLastRec));

   ((POBJECTTWIN)pot)->fsLastRec.fscond = FS_COND_UNAVAILABLE;

   return;
}


/*
** MarkTwinFamilyDeletionPending()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void MarkTwinFamilyDeletionPending(PTWINFAMILY ptf)
{
   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));

   if (IsStubFlagClear(&(ptf->stub), STUB_FL_DELETION_PENDING))
      TRACE_OUT((TEXT("MarkTwinFamilyDeletionPending(): Deletion now pending for twin family for %s."),
                 GetString(ptf->hsName)));

   SetStubFlag(&(ptf->stub), STUB_FL_DELETION_PENDING);

   return;
}


/*
** UnmarkTwinFamilyDeletionPending()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void UnmarkTwinFamilyDeletionPending(PTWINFAMILY ptf)
{
   BOOL bContinue;
   HNODE hnode;

   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));

   if (IsStubFlagSet(&(ptf->stub), STUB_FL_DELETION_PENDING))
   {
      for (bContinue = GetFirstNode(ptf->hlistObjectTwins, &hnode);
           bContinue;
           bContinue = GetNextNode(hnode, &hnode))
      {
         POBJECTTWIN pot;

         pot = GetNodeData(hnode);

         ClearStubFlag(&(pot->stub), STUB_FL_KEEP);
      }

      ClearStubFlag(&(ptf->stub), STUB_FL_DELETION_PENDING);

      TRACE_OUT((TEXT("UnmarkTwinFamilyDeletionPending(): Deletion no longer pending for twin family for %s."),
                 GetString(ptf->hsName)));
   }

   return;
}


/*
** IsTwinFamilyDeletionPending()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsTwinFamilyDeletionPending(PCTWINFAMILY pctf)
{
   ASSERT(IS_VALID_STRUCT_PTR(pctf, CTWINFAMILY));

   return(IsStubFlagSet(&(pctf->stub), STUB_FL_DELETION_PENDING));
}


/*
** ClearTwinFamilySrcFolderTwinCount()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void ClearTwinFamilySrcFolderTwinCount(PTWINFAMILY ptf)
{
   ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));

   EVAL(WalkList(ptf->hlistObjectTwins, &ClearSrcFolderTwinsWalker, NULL));

   return;
}


/*
** EnumObjectTwins()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL EnumObjectTwins(HBRFCASE hbr,
                                 ENUMGENERATEDOBJECTTWINSPROC egotp,
                                 PVOID pvRefData)
{
   BOOL bResult = TRUE;
   HPTRARRAY hpaTwinFamilies;
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   /* pvRefData may be any value. */

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_CODE_PTR(egotp, ENUMGENERATEDOBJECTTWINPROC));

   /* Walk the array of twin families. */

   hpaTwinFamilies = GetBriefcaseTwinFamilyPtrArray(hbr);

   aicPtrs = GetPtrCount(hpaTwinFamilies);
   ai = 0;

   while (ai < aicPtrs)
   {
      PTWINFAMILY ptf;
      BOOL bContinue;
      HNODE hnodePrev;

      ptf = GetPtr(hpaTwinFamilies, ai);

      ASSERT(IS_VALID_STRUCT_PTR(ptf, CTWINFAMILY));
      ASSERT(IsStubFlagClear(&(ptf->stub), STUB_FL_UNLINKED));

      /* Lock the twin family so it isn't deleted out from under us. */

      LockStub(&(ptf->stub));

      /*
       * Walk each twin family's list of object twins, calling the callback
       * function with each object twin.
       */

      bContinue = GetFirstNode(ptf->hlistObjectTwins, &hnodePrev);

      while (bContinue)
      {
         HNODE hnodeNext;
         POBJECTTWIN pot;

         bContinue = GetNextNode(hnodePrev, &hnodeNext);

         pot = (POBJECTTWIN)GetNodeData(hnodePrev);

         ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));

         bResult = (*egotp)(pot, pvRefData);

         if (! bResult)
            break;

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
         TRACE_OUT((TEXT("EnumObjectTwins(): Twin family for object %s unlinked by callback."),
                    GetString(ptf->hsName)));
      }

      UnlockStub(&(ptf->stub));

      if (! bResult)
         break;
   }

   return(bResult);
}


/*
** ApplyNewFolderTwinsToTwinFamilies()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** If FALSE is returned, the array of twin families is in the same state it was
** in before ApplyNewFolderTwinsToTwinFamilies() was called.  No clean-up is
** required by the caller in case of failure.
**
** This function collapses a pair of separate twin families when an object twin
** in one twin family intersects one of the folder twins in the pair of new
** folder twins and an object twin in the other twin family intersects the
** other folder twin in the pair of new folder twins.
**
** This function generates a spinoff object twin when an existing object twin
** intersects one of the folder twins in the pair of new folder twins, and no
** corresponding object twin for the other folder twin in the pair of new
** folder twins exists in the briefcase.  The spinoff object twin is added to
** the generating object twin's twin family.  A spinoff object twins cannot
** cause any existing pairs of twin families to be collapsed because the
** spinoff object twin did not previously exist in a twin family.
**
** A new folder twin may collapse pairs of existing twin families.  E.g.,
** consider the following scenario:
**
** 1) Twin families (c:\, d:\, foo), (e:\, f:\, foo), (c:\, d:\, bar), and
**    (e:\, f:\, bar) exist.
** 2) New folder twin (d:\, e:\, *.*) is added.
** 3) Twin families (c:\, d:\, foo) and (e:\, f:\, foo) must be collpased into
**    a single twin family because of the (d:\, e:\, *.*) folder twin.
** 4) Twin families (c:\, d:\, bar) and (e:\, f:\, bar) must be collpased into
**    a single twin family because of the (d:\, e:\, *.*) folder twin.
**
** So we see that new folder twin (d:\, e:\, *.*) must collapse two pairs of
** existing twin families a single twin family each.  Twin family
** (c:\, d:\, foo) plus twin family (e:\, f:\, foo) becomes twin family
** (c:\, d:\, e:\, f:\, foo).  Twin family (c:\, d:\, bar) plus twin family
** (e:\, f:\, bar) becomes twin family (c:\, d:\, e:\, f:\, bar).
*/
PUBLIC_CODE BOOL ApplyNewFolderTwinsToTwinFamilies(PCFOLDERPAIR pcfp)
{
   BOOL bResult = FALSE;
   HLIST hlistGeneratedObjectTwins;

   ASSERT(IS_VALID_STRUCT_PTR(pcfp, CFOLDERPAIR));

   /*
    * Create lists to contain existing object twins generated by both folder
    * twins.
    */

   if (CreateListOfGeneratedObjectTwins(pcfp, &hlistGeneratedObjectTwins))
   {
      HLIST hlistOtherGeneratedObjectTwins;

      if (CreateListOfGeneratedObjectTwins(pcfp->pfpOther,
                                           &hlistOtherGeneratedObjectTwins))
      {
         NEWLIST nl;
         HLIST hlistNewObjectTwins;

         /* Create list to contain spin-off object twins. */

         nl.dwFlags = 0;

         if (CreateList(&nl, &hlistNewObjectTwins))
         {
            SPINOFFOBJECTTWININFO sooti;

            /*
             * Generate list of new object twins generated by new folder twins
             * to seed ApplyNewObjectTwinToFolderTwins().
             */

            sooti.pcfp = pcfp;
            sooti.hlistNewObjectTwins = hlistNewObjectTwins;

            if (WalkList(hlistGeneratedObjectTwins, &GenerateSpinOffObjectTwin,
                         &sooti))
            {
               sooti.pcfp = pcfp->pfpOther;
               ASSERT(sooti.hlistNewObjectTwins == hlistNewObjectTwins);

               if (WalkList(hlistOtherGeneratedObjectTwins,
                            &GenerateSpinOffObjectTwin, &sooti))
               {
                  /*
                   * ApplyNewObjectTwinsToFolderTwins() sets ulcSrcFolderTwins
                   * for all object twins in hlistNewObjectTwins.
                   */

                  if (ApplyNewObjectTwinsToFolderTwins(hlistNewObjectTwins))
                  {
                     /*
                      * Collapse separate twin families joined by new folder
                      * twin.
                      */

                     EVAL(WalkList(hlistGeneratedObjectTwins, &BuildBradyBunch,
                                   (PVOID)pcfp));

                     /*
                      * We don't need to call BuildBradyBunch() for
                      * pcfp->pfpOther and hlistOtherGeneratedObjectTwins since
                      * one twin family from each collapsed pair of twin
                      * families must come from each list of generated object
                      * twins.
                      */

                     /*
                      * Increment source folder twin count for all pre-existing
                      * object twins generated by the new folder twins.
                      */

                     EVAL(WalkList(hlistGeneratedObjectTwins,
                                   &IncrementSrcFolderTwinsWalker, NULL));
                     EVAL(WalkList(hlistOtherGeneratedObjectTwins,
                                   &IncrementSrcFolderTwinsWalker, NULL));

                     bResult = TRUE;
                  }
               }
            }

            /* Wipe out any new object twins on failure. */

            if (! bResult)
               EVAL(WalkList(hlistNewObjectTwins, &DestroyObjectTwinStubWalker,
                             NULL));

            DestroyList(hlistNewObjectTwins);
         }

         DestroyList(hlistOtherGeneratedObjectTwins);
      }

      DestroyList(hlistGeneratedObjectTwins);
   }

   return(bResult);
}


/*
** TransplantObjectTwin()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT TransplantObjectTwin(POBJECTTWIN pot,
                                            HPATH hpathOldFolder,
                                            HPATH hpathNewFolder)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(pot, COBJECTTWIN));
   ASSERT(IS_VALID_HANDLE(hpathOldFolder, PATH));
   ASSERT(IS_VALID_HANDLE(hpathNewFolder, PATH));

   /* Is this object twin rooted in the renamed folder's subtree? */

   if (IsPathPrefix(pot->hpath, hpathOldFolder))
   {
      TCHAR rgchPathSuffix[MAX_PATH_LEN];
      LPCTSTR pcszSubPath;
      HPATH hpathNew;

      /* Yes.  Change the object twin's root. */

      pcszSubPath = FindChildPathSuffix(hpathOldFolder, pot->hpath,
                                        rgchPathSuffix);

      if (AddChildPath(GetBriefcasePathList(pot->ptfParent->hbr),
                       hpathNewFolder, pcszSubPath, &hpathNew))
      {
         TRACE_OUT((TEXT("TransplantObjectTwin(): Transplanted object twin %s\\%s to %s\\%s."),
                    DebugGetPathString(pot->hpath),
                    GetString(pot->ptfParent->hsName),
                    DebugGetPathString(hpathNew),
                    GetString(pot->ptfParent->hsName)));

         DeletePath(pot->hpath);
         pot->hpath = hpathNew;

         tr = TR_SUCCESS;
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
      tr = TR_SUCCESS;

   return(tr);
}


/*
** IsFolderObjectTwinName()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsFolderObjectTwinName(LPCTSTR pcszName)
{
   ASSERT(IS_VALID_STRING_PTR(pcszName, CSTR));

   return(! *pcszName);
}


/*
** IsValidHTWIN()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHTWIN(HTWIN htwin)
{
   BOOL bValid = FALSE;

   if (IS_VALID_STRUCT_PTR((PCSTUB)htwin, CSTUB))
   {
      switch (((PSTUB)htwin)->st)
      {
         case ST_OBJECTTWIN:
            bValid = IS_VALID_HANDLE((HOBJECTTWIN)htwin, OBJECTTWIN);
            break;

         case ST_TWINFAMILY:
            bValid = IS_VALID_HANDLE((HTWINFAMILY)htwin, TWINFAMILY);
            break;

         case ST_FOLDERPAIR:
            bValid = IS_VALID_HANDLE((HFOLDERTWIN)htwin, FOLDERTWIN);
            break;

         default:
            ERROR_OUT((TEXT("IsValidHTWIN() called on unrecognized stub type %d."),
                       ((PSTUB)htwin)->st));
            break;
      }
   }
   else
      ERROR_OUT((TEXT("IsValidHTWIN() called on bad twin handle %#lx."),
                 htwin));

   return(bValid);
}


/*
** IsValidHTWINFAMILY()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHTWINFAMILY(HTWINFAMILY htf)
{
   return(IS_VALID_STRUCT_PTR((PTWINFAMILY)htf, CTWINFAMILY));
}


/*
** IsValidHOBJECTTWIN()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHOBJECTTWIN(HOBJECTTWIN hot)
{
   return(IS_VALID_STRUCT_PTR((POBJECTTWIN)hot, COBJECTTWIN));
}


#ifdef VSTF

/*
** IsValidPCTWINFAMILY()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCTWINFAMILY(PCTWINFAMILY pctf)
{
   BOOL bResult;

   /* All the fields of an unlinked twin family should be valid. */

   /* Don't validate hbr. */

   /*
    * In some cases there may be fewer than two object twins in a twin family,
    * e.g., when two twin families are being collapsed, when a twin family is
    * being deleted, and when a twin family is being read in from a database.
    */

   if (IS_VALID_READ_PTR(pctf, CTWINFAMILY) &&
       IS_VALID_STRUCT_PTR(&(pctf->stub), CSTUB) &&
       FLAGS_ARE_VALID(GetStubFlags(&(pctf->stub)), ALL_TWIN_FAMILY_FLAGS) &&
       IS_VALID_HANDLE(pctf->hsName, STRING) &&
       IS_VALID_HANDLE(pctf->hlistObjectTwins, LIST))
      bResult = WalkList(pctf->hlistObjectTwins, &IsValidObjectTwinWalker, (PVOID)pctf);
   else
      bResult = FALSE;

   return(bResult);
}


/*
** IsValidPCOBJECTTWIN()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCOBJECTTWIN(PCOBJECTTWIN pcot)
{
   /*
    * All the fields of an unlinked object twin should be valid, except
    * possibly ptfParent and fsCurrent.
    */

   /*
    *   Winner of the 1995 "I think the compiler generates better code
    *                       if its takes up less space on the screen" award.
    *
    *   Running up in the  "Make the debugger execute 2K of code as an
    *                       atomic operation while debugger" category.
    *
    */

   return(IS_VALID_READ_PTR(pcot, COBJECTTWIN) &&
          IS_VALID_STRUCT_PTR(&(pcot->stub), CSTUB) &&
          FLAGS_ARE_VALID(GetStubFlags(&(pcot->stub)), ALL_OBJECT_TWIN_FLAGS) &&
          IS_VALID_HANDLE(pcot->hpath, PATH) &&
          (IsStubFlagSet(&(pcot->stub), STUB_FL_UNLINKED) ||
           IS_VALID_READ_PTR(pcot->ptfParent, CTWINFAMILY)) &&
          IS_VALID_STRUCT_PTR(&(pcot->fsLastRec), CFILESTAMP) &&
          (IsStubFlagClear(&(pcot->stub), STUB_FL_FILE_STAMP_VALID) ||
           (IS_VALID_STRUCT_PTR(&(pcot->fsCurrent), CFILESTAMP))) &&
          EVAL(! (! IsReconciledFileStamp(&(pcot->fsLastRec)) &&
                  IsStubFlagSet(&(pcot->stub), STUB_FL_NOT_RECONCILED))));
}

#endif


/*
** WriteTwinFamilies()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT WriteTwinFamilies(HCACHEDFILE hcf, HPTRARRAY hpaTwinFamilies)
{
   TWINRESULT tr = TR_BRIEFCASE_WRITE_FAILED;
   DWORD dwcbTwinFamiliesDBHeaderOffset;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hpaTwinFamilies, PTRARRAY));

   /* Save initial file poisition. */

   dwcbTwinFamiliesDBHeaderOffset = GetCachedFilePointerPosition(hcf);

   if (dwcbTwinFamiliesDBHeaderOffset != INVALID_SEEK_POSITION)
   {
      TWINFAMILIESDBHEADER tfdbh;

      /* Leave space for the twin families' header. */

      ZeroMemory(&tfdbh, sizeof(tfdbh));

      if (WriteToCachedFile(hcf, (PCVOID)&tfdbh, sizeof(tfdbh), NULL))
      {
         ARRAYINDEX aicPtrs;
         ARRAYINDEX ai;

         tr = TR_SUCCESS;

         aicPtrs = GetPtrCount(hpaTwinFamilies);

         for (ai = 0;
              ai < aicPtrs && tr == TR_SUCCESS;
              ai++)
            tr = WriteTwinFamily(hcf, GetPtr(hpaTwinFamilies, ai));

         if (tr == TR_SUCCESS)
         {
            /* Save twin families' header. */

            tfdbh.lcTwinFamilies = aicPtrs;

            tr = WriteDBSegmentHeader(hcf, dwcbTwinFamiliesDBHeaderOffset,
                                      &tfdbh, sizeof(tfdbh));

            if (tr == TR_SUCCESS)
               TRACE_OUT((TEXT("WriteTwinFamilies(): Wrote %ld twin families."),
                          tfdbh.lcTwinFamilies));
         }
      }
   }

   return(tr);
}


/*
** ReadTwinFamilies()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT ReadTwinFamilies(HCACHEDFILE hcf, HBRFCASE hbr,
                                   PCDBVERSION pcdbver,
                                   HHANDLETRANS hhtFolderTrans,
                                   HHANDLETRANS hhtNameTrans)
{
   TWINRESULT tr;
   TWINFAMILIESDBHEADER tfdbh;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_READ_PTR(pcdbver, DBVERSION));
   ASSERT(IS_VALID_HANDLE(hhtFolderTrans, HANDLETRANS));
   ASSERT(IS_VALID_HANDLE(hhtNameTrans, HANDLETRANS));

   if (ReadFromCachedFile(hcf, &tfdbh, sizeof(tfdbh), &dwcbRead) &&
       dwcbRead == sizeof(tfdbh))
   {
      LONG l;

      tr = TR_SUCCESS;

      TRACE_OUT((TEXT("ReadTwinFamilies(): Reading %ld twin families."),
                 tfdbh.lcTwinFamilies));

      for (l = 0;
           l < tfdbh.lcTwinFamilies && tr == TR_SUCCESS;
           l++)
         tr = ReadTwinFamily(hcf, hbr, pcdbver, hhtFolderTrans, hhtNameTrans);

      ASSERT(AreTwinFamiliesValid(GetBriefcaseTwinFamilyPtrArray(hbr)));
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


/***************************** Exported Functions ****************************/


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | AddObjectTwin | Twins two objects.

@parm HBRFCASE | hbr | A handle to the open briefcase that the new object twins
are to be added to.

@parm PCNEWOBJECTTWIN | pcnot | A pointer to a CNEWOBJECTTWIN describing the
objects to be twinned.

@parm PHTWINFAMILY | phtf | A pointer to an HTWINFAMILY to be filled in with
a handle to the twin family to which the object twins were added.  This handle
may refer to a new or existing twin family.  *phtf is only valid if TR_SUCCESS
is returned.

@rdesc If the objects were twinned successfully, TR_SUCCESS is returned, and
*phTwinFamily contains a handle to the associated twin family.  Otherwise, the
objects were not twinned successfully, the return value indicates the error
that occurred, and *phtf is undefined.  If one or both of the volumes
specified by the NEWOBJECTTWIN structure is not present, TR_UNAVAILABLE_VOLUME
will be returned, and the object twin will not be added.

@comm Once the caller is finshed with the twin handle returned by
AddObjectTwin(), ReleaseTwinHandle() should be called to release the twin
handle.  DeleteTwin() does not release a twin handle returned by
AddObjectTwin().

@xref ReleaseTwinHandle DeleteTwin

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI AddObjectTwin(HBRFCASE hbr, PCNEWOBJECTTWIN pcnot,
                                           PHTWINFAMILY phtf)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(AddObjectTwin);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbr, BRFCASE) &&
          IS_VALID_STRUCT_PTR(pcnot, CNEWOBJECTTWIN) &&
          EVAL(pcnot->ulSize == sizeof(*pcnot)) &&
          IS_VALID_WRITE_PTR(phtf, HTWINFAMILY))
#endif
      {
         HCLSIFACECACHE hcic;

         if (CreateClassInterfaceCache(&hcic))
         {
            HPATHLIST hplBriefcase;
            HPATH hpathFolder1;

            InvalidatePathListInfo(GetBriefcasePathList(hbr));

            hplBriefcase = GetBriefcasePathList(hbr);

            tr = TranslatePATHRESULTToTWINRESULT(AddPath(hplBriefcase,
                                                         pcnot->pcszFolder1,
                                                         &hpathFolder1));

            if (tr == TR_SUCCESS)
            {
               HPATH hpathFolder2;

               tr = TranslatePATHRESULTToTWINRESULT(AddPath(hplBriefcase,
                                                            pcnot->pcszFolder2,
                                                            &hpathFolder2));

               if (tr == TR_SUCCESS)
               {
                  POBJECTTWIN pot1;
                  POBJECTTWIN pot2;

                  tr = TwinObjects(hbr, hcic, hpathFolder1, hpathFolder2,
                                   pcnot->pcszName, &pot1, &pot2);

                  /*
                   * These twins are not really duplicates unless they were already
                   * connected as object twins.
                   */

                  if (tr == TR_DUPLICATE_TWIN &&
                      (IsStubFlagClear(&(pot1->stub), STUB_FL_FROM_OBJECT_TWIN) ||
                       IsStubFlagClear(&(pot2->stub), STUB_FL_FROM_OBJECT_TWIN)))
                     tr = TR_SUCCESS;

                  if (tr == TR_SUCCESS)
                  {
                     /* Success! */

                     ASSERT(pot1->ptfParent == pot2->ptfParent);
                     ASSERT(IS_VALID_HANDLE((HTWINFAMILY)(pot1->ptfParent), TWINFAMILY));

                     LockStub(&(pot1->ptfParent->stub));

                     SetStubFlag(&(pot1->stub), STUB_FL_FROM_OBJECT_TWIN);
                     SetStubFlag(&(pot2->stub), STUB_FL_FROM_OBJECT_TWIN);

                     *phtf = (HTWINFAMILY)(pot1->ptfParent);
                  }

                  DeletePath(hpathFolder2);
               }

               DeletePath(hpathFolder1);
            }

            DestroyClassInterfaceCache(hcic);
         }
         else
            tr = TR_OUT_OF_MEMORY;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(AddObjectTwin, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | ReleaseTwinHandle | Releases a twin handle returned by
AddObjectTwin(), AddFolderTwin(), or GetObjectTwinHandle().

@parm HTWIN | hTwin | The twin handle that is to be released.

@rdesc If the twin handle was released successfully, TR_SUCCESS is returned.
Otherwise, the twin handle was not released successfully, and the return value
indicates the error that occurred.  hTwin is no longer a valid twin handle
after ReleaseTwinHandle() is called.

@comm If the lock count of the twin drops to 0 and deletion is pending against
the twin, the twin is deleted.  If ReleaseTwinHandle() is called with a valid
handle to a twin that has been deleted, TR_SUCCESS will be returned.
DeleteTwin() does not release a twin handle returned by AddObjectTwin(),
AddFolderTwin(), or GetObjectTwinHandle().  ReleaseTwinHandle() should be
called to release a twin handle returned by AddObjectTwin(), AddFolderTwin(),
or GetObjectTwinHandle().  DeleteTwin() should be called before
ReleaseTwinHandle() if the twin is to be deleted.

@xref AddObjectTwin AddFolderTwin DeleteTwin

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI ReleaseTwinHandle(HTWIN hTwin)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(ReleaseTwinHandle);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hTwin, TWIN))
#endif
      {
         UnlockStub((PSTUB)hTwin);

         tr = TR_SUCCESS;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(ReleaseTwinHandle, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | DeleteTwin | Deletes a twin from the synchronization
database.  A twin is added to the synchronization database by AddObjectTwin()
or AddFolderTwin().

@parm HTWIN | htwin | A handle to the twin being deleted.

@rdesc If the twin was deleted successfully, TR_SUCCESS is returned.
Otherwise, the twin was not deleted successfully, and the return value
indicates the error that occurred.

@comm If DeleteTwin() is called with a valid handle to a twin that has been
deleted, TR_SUCCESS will be returned.  DeleteTwin() does not release a twin
handle returned by AddObjectTwin(), AddFolderTwin(), or GetObjectTwinHandle().
ReleaseTwinHandle() should be called to release a twin handle returned by
AddObjectTwin(), AddFolderTwin(), or GetObjectTwinHandle().  DeleteTwin()
should be called before ReleaseTwinHandle() if the twin is to be deleted.
DeleteTwin() will always succeed on a valid HFOLDERTWIN.  DeleteTwin() will
fail on a valid HOBJECTTWIN for any object twin that has source folder twins,
returning TR_HAS_FOLDER_TWIN_SRC.  DeleteTwin() will also fail on a valid
HTWINFAMILY for any twin family that contains two or more object twins with
source folder twins, returning TR_HAS_FOLDER_TWIN_SRC.  A twin family cannot
contain only one object twin with source folder twins.  Twin families can only
contain 0, 2, or more object twins with source folder twins.

@xref AddObjectTwin AddFolderTwin ReleaseTwinHandle IsOrphanObjectTwin
CountSourceFolderTwins

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI DeleteTwin(HTWIN hTwin)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(DeleteTwin);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hTwin, TWIN))
#endif
      {
         tr = DestroyStub((PSTUB)hTwin);
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(DeleteTwin, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | GetObjectTwinHandle | Determines whether or not an object is
a twin.  If the object is a twin, a twin handle for the twinned object is
returned.

@parm HBRFCASE | hbr | A handle to the open briefcase to be checked for the
object twin.

@parm PCSTR | pcszFolder | A pointer to a string indicating the object's
folder.

@parm PCSTR | pcszName | A pointer to a string indicating the object's name.

@parm PHOBJECTTWIN | phot | A pointer to an HOBJECTTWIN to be filled in with
a handle to the object twin or NULL.  If the object is a twin, *phObjectTwin
is filled in with a handle to the object twin.  If the object is not a twin,
*phObjectTwin is filled in with NULL.  *phObjectTwin is only valid if
TR_SUCCESS is returned.

@rdesc If the lookup was successful, TR_SUCCESS is returned.  Otherwise, the
lookup was not successful, and the return value indicates the error that
occurred.

@comm Once the caller is finshed with the twin handle returned by
GetObjectTwinHandle(), ReleaseTwinHandle() should be called to release the twin
handle.  N.b., DeleteTwin() does not release a twin handle returned by
GetObjectTwinHandle().

@xref AddObjectTwin ReleaseTwinHandle DeleteTwin

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI GetObjectTwinHandle(HBRFCASE hbr,
                                                 LPCTSTR pcszFolder,
                                                 LPCTSTR pcszName,
                                                 PHOBJECTTWIN phot)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(GetObjectTwinHandle);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbr, BRFCASE) &&
          IS_VALID_STRING_PTR(pcszFolder, CSTR) &&
          IS_VALID_STRING_PTR(pcszName, CSTR) &&
          IS_VALID_WRITE_PTR(phot, HOBJECTTWIN))
#endif
      {
         HPATH hpath;

         InvalidatePathListInfo(GetBriefcasePathList(hbr));

         tr = TranslatePATHRESULTToTWINRESULT(
               AddPath(GetBriefcasePathList(hbr), pcszFolder, &hpath));

         if (tr == TR_SUCCESS)
         {
            BOOL bFound;
            HNODE hnode;
            POBJECTTWIN pot;

            /* Is this object already an object twin? */

            bFound = FindObjectTwin(hbr, hpath, pcszName, &hnode);

            if (bFound)
               /* Yes. */
               pot = (POBJECTTWIN)GetNodeData(hnode);
            else
               /*
                * No.  Expand folder twins, and check for a generating folder
                * twin.
                */
               tr = TryToGenerateObjectTwin(hbr, hpath, pcszName, &bFound,
                                            &pot);

            if (tr == TR_SUCCESS)
            {
               if (bFound)
               {
                  LockStub(&(pot->stub));

                  TRACE_OUT((TEXT("GetObjectTwinHandle(): %s\\%s is an object twin."),
                             DebugGetPathString(hpath),
                             pcszName));

                  *phot = (HOBJECTTWIN)pot;

                  ASSERT(IS_VALID_HANDLE(*phot, OBJECTTWIN));
               }
               else
               {
                  TRACE_OUT((TEXT("GetObjectTwinHandle(): %s\\%s is not an object twin."),
                             DebugGetPathString(hpath),
                             pcszName));

                  *phot = NULL;
               }
            }

            DeletePath(hpath);
         }
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(GetObjectTwinHandle, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | IsOrphanObjectTwin | Determines whether or not an object twin
was added to the synchronization database through a call to AddObjectTwin().

@parm HOBJECTTWIN | hot | A handle to the object twin whose orphan status is to
be determined.

@parm PBOOL | pbIsOrphanObjectTwin | A pointer to a BOOL to be filled in with
TRUE if the object twin was added through AddObjectTwin().
*pbIsOrphanObjectTwin is only valid if TR_SUCCESS is returned.

@rdesc If the lookup was successful, TR_SUCCESS is returned.  Otherwise, the
lookup was not successful, and the return value indicates the error that
occurred.

@comm If IsOrphanObjectTwin() is called with a valid handle to an object twin
that has been deleted, TR_DELETED_TWIN will be returned.

@xref AddObjectTwin

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI IsOrphanObjectTwin(HOBJECTTWIN hot,
                                                PBOOL pbIsOrphanObjectTwin)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(IsOrphanObjectTwin);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hot, OBJECTTWIN) &&
          IS_VALID_WRITE_PTR(pbIsOrphanObjectTwin, BOOL))
#endif
      {
         /* Has this object twin been deleted? */

         if (IsStubFlagClear(&(((POBJECTTWIN)(hot))->stub), STUB_FL_UNLINKED))
         {
            /* No. */

            if (IsStubFlagSet(&(((POBJECTTWIN)hot)->stub), STUB_FL_FROM_OBJECT_TWIN))
            {
               *pbIsOrphanObjectTwin = TRUE;

               TRACE_OUT((TEXT("IsOrphanObjectTwin(): Object twin %s\\%s is an orphan object twin."),
                          DebugGetPathString(((POBJECTTWIN)hot)->hpath),
                          GetString(((POBJECTTWIN)hot)->ptfParent->hsName)));
            }
            else
            {
               *pbIsOrphanObjectTwin = FALSE;

               TRACE_OUT((TEXT("IsOrphanObjectTwin(): Object twin %s\\%s is not an orphan object twin."),
                          DebugGetPathString(((POBJECTTWIN)hot)->hpath),
                          GetString(((POBJECTTWIN)hot)->ptfParent->hsName)));
            }

            ASSERT(*pbIsOrphanObjectTwin ||
                   ((POBJECTTWIN)hot)->ulcSrcFolderTwins);

            tr = TR_SUCCESS;
         }
         else
            /* Yes. */
            tr = TR_DELETED_TWIN;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(IsOrphanObjectTwin, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | CountSourceFolderTwins | Determines the number of folder
twins that generate an object twin.

@parm HOBJECTTWIN | hot | A handle to the object twin whose folder twin sources
are to be counted.

@parm PULONG | pulcSrcFolderTwins | A pointer to a ULONG to be filled in with
the number of folder twins that generate the object twin.  *pulcSrcFolderTwins
is only valid if TR_SUCCESS is returned.

@rdesc If the lookup was successful, TR_SUCCESS is returned.  Otherwise, the
lookup was not successful, and the return value indicates the error that
occurred.

@comm If CountSourceFolderTwins() is called with a valid handle to a folder
twin that has been deleted, TR_DELETED_TWIN will be returned.

@xref AddFolderTwin

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI CountSourceFolderTwins(HOBJECTTWIN hot,
                                                    PULONG pulcSrcFolderTwins)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(CountSourceFolderTwins);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hot, OBJECTTWIN) &&
          IS_VALID_WRITE_PTR(pulcSrcFolderTwins, ULONG))
#endif
      {
         /* Has this object twin been deleted? */

         if (IsStubFlagClear(&(((POBJECTTWIN)(hot))->stub), STUB_FL_UNLINKED))
         {
            /* No. */

            *pulcSrcFolderTwins = ((POBJECTTWIN)hot)->ulcSrcFolderTwins;

            ASSERT(*pulcSrcFolderTwins > 0 ||
                   IsStubFlagSet(&(((POBJECTTWIN)hot)->stub), STUB_FL_FROM_OBJECT_TWIN));

            TRACE_OUT((TEXT("CountSourceFolderTwins(): Object twin %s\\%s has %lu source folder twins."),
                       DebugGetPathString(((POBJECTTWIN)hot)->hpath),
                       GetString(((POBJECTTWIN)hot)->ptfParent->hsName),
                       *pulcSrcFolderTwins));

            tr = TR_SUCCESS;
         }
         else
            /* Yes. */
            tr = TR_DELETED_TWIN;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(CountSourceFolderTwins, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api BOOL | AnyTwins | Determines whether or not any twins currently exist in a
briefcase.

@parm HBRFCASE | hbr | A handle to the open briefcase to be checked for twins.

@parm PBOOL | pbAnyTwins | A pointer to a BOOL to be filled in with TRUE if
the given briefcase contains any twins or FALSE if not.  *pbAnyTwins is only
valid if TR_SUCCESS is returned.

@rdesc If the lookup was successful, TR_SUCCESS is returned.  Otherwise, the
lookup was not successful, and the return value indicates the error that
occurred.

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI AnyTwins(HBRFCASE hbr, PBOOL pbAnyTwins)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(AnyTwins);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbr, BRFCASE) &&
          IS_VALID_WRITE_PTR(pbAnyTwins, BOOL))
#endif
      {
         if (GetPtrCount(GetBriefcaseTwinFamilyPtrArray(hbr)) ||
             GetPtrCount(GetBriefcaseFolderPairPtrArray(hbr)))
         {
            *pbAnyTwins = TRUE;

            TRACE_OUT((TEXT("AnyTwins(): There are twins in briefcase %#lx."),
                       hbr));
         }
         else
         {
            *pbAnyTwins = FALSE;

            TRACE_OUT((TEXT("AnyTwins(): There are not any twins in briefcase %#lx."),
                       hbr));
         }

         tr = TR_SUCCESS;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(AnyTwins, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}

