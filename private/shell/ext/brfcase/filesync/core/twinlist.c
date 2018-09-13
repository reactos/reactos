/*
 * twinlist.c - Twin list ADT module.
 */

/*



*/


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "stub.h"


/* Constants
 ************/

/* twin list pointer array allocation parameters */

#define NUM_START_TWIN_HANDLES      (1)
#define NUM_TWIN_HANDLES_TO_ADD     (16)


/* Types
 ********/

/* twin list */

typedef struct _twinlist
{
   /* handle to array of HTWINs in list */

   /* A NULL hpa implies that all twins in the briefcase are in the list. */

   HPTRARRAY hpa;

   /* handle to briefcase that twin list is associated with */

   HBRFCASE hbr;
}
TWINLIST;
DECLARE_STANDARD_TYPES(TWINLIST);


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE COMPARISONRESULT TwinListSortCmp(PCVOID, PCVOID);
PRIVATE_CODE TWINRESULT MyAddTwinToTwinList(PCTWINLIST, HTWIN);
PRIVATE_CODE TWINRESULT MyRemoveTwinFromTwinList(PCTWINLIST, HTWIN);
PRIVATE_CODE void MyRemoveAllTwinsFromTwinList(PCTWINLIST);
PRIVATE_CODE BOOL AddTwinToTwinListProc(HTWIN, LPARAM);

#ifdef VSTF

PRIVATE_CODE BOOL IsValidPCTWINLIST(PCTWINLIST);

#endif


/*
** TwinListSortCmp()
**
** Handle comparison function used to sort twin lists.
**
** Arguments:     htwin1 - first twin handle
**                htwin2 - second twin handle
**
** Returns:
**
** Side Effects:  none
**
** Twin handles are sorted by:
**    1) handle value
*/
PRIVATE_CODE COMPARISONRESULT TwinListSortCmp(PCVOID htwin1, PCVOID htwin2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_HANDLE((HTWIN)htwin1, TWIN));
   ASSERT(IS_VALID_HANDLE((HTWIN)htwin2, TWIN));

   if (htwin1 < htwin2)
      cr = CR_FIRST_SMALLER;
   else if (htwin1 > htwin2)
      cr = CR_FIRST_LARGER;
   else
      cr = CR_EQUAL;

   return(cr);
}


/*
** MyAddTwinToTwinList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT MyAddTwinToTwinList(PCTWINLIST pctl, HTWIN htwin)
{
   TWINRESULT tr;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_STRUCT_PTR(pctl, CTWINLIST));
   ASSERT(IS_VALID_HANDLE(htwin, TWIN));

   if (! SearchSortedArray(pctl->hpa, &TwinListSortCmp, htwin, &ai))
   {
      if (InsertPtr(pctl->hpa, TwinListSortCmp, ai, htwin))
      {
         LockStub((PSTUB)htwin);

         tr = TR_SUCCESS;
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
   {
      ASSERT(GetPtr(pctl->hpa, ai) == htwin);
      ASSERT(GetTwinBriefcase(htwin) == pctl->hbr);

      WARNING_OUT((TEXT("MyAddTwinToTwinList(): Twin %#lx has already been added to twin list %#lx."),
                   htwin,
                   pctl));

      tr = TR_DUPLICATE_TWIN;
   }

   return(tr);
}


/*
** MyRemoveTwinFromTwinList()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT MyRemoveTwinFromTwinList(PCTWINLIST pctl,
                                                    HTWIN htwin)
{
   TWINRESULT tr;
   ARRAYINDEX ai;

   if (SearchSortedArray(pctl->hpa, &TwinListSortCmp, htwin, &ai))
   {
      ASSERT(GetPtr(pctl->hpa, ai) == htwin);
      ASSERT(GetTwinBriefcase(htwin) == pctl->hbr);

      DeletePtr(pctl->hpa, ai);

      UnlockStub((PSTUB)htwin);

      tr = TR_SUCCESS;
   }
   else
   {
      WARNING_OUT((TEXT("MyRemoveTwinFromTwinList(): Twin %#lx is not in twin list %#lx."),
                   htwin,
                   pctl));

      tr = TR_INVALID_PARAMETER;
   }

   return(tr);
}


/*
** MyRemoveAllTwinsFromTwinList()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void MyRemoveAllTwinsFromTwinList(PCTWINLIST pctl)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   /* Unlock all twins in array. */

   aicPtrs = GetPtrCount(pctl->hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      UnlockStub(GetPtr(pctl->hpa, ai));

   /* Now wipe out the array. */

   DeleteAllPtrs(pctl->hpa);

   return;
}


/*
** AddTwinToTwinListProc()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL AddTwinToTwinListProc(HTWIN htwin, LPARAM htl)
{
   BOOL bResult;
   TWINRESULT tr;

   ASSERT(IS_VALID_HANDLE(htwin, TWIN));
   ASSERT(IS_VALID_HANDLE((HTWINLIST)htl, TWINLIST));

   tr = MyAddTwinToTwinList((PCTWINLIST)htl, htwin);

   switch (tr)
   {
      case TR_SUCCESS:
      case TR_DUPLICATE_TWIN:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         break;
   }

   return(bResult);
}


#ifdef VSTF

/*
** IsValidPCTWINLIST()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCTWINLIST(PCTWINLIST pctl)
{
   BOOL bResult;

   if (IS_VALID_READ_PTR(pctl, CTWINLIST) &&
       (! pctl->hpa || IS_VALID_HANDLE(pctl->hpa, PTRARRAY)) &&
       IS_VALID_HANDLE(pctl->hbr, BRFCASE))
      bResult = TRUE;
   else
      bResult = FALSE;

   return(bResult);
}

#endif


/****************************** Public Functions *****************************/


/*
** GetTwinListBriefcase()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HBRFCASE GetTwinListBriefcase(HTWINLIST htl)
{
   ASSERT(IS_VALID_HANDLE(htl, TWINLIST));

   return(((PCTWINLIST)htl)->hbr);
}


/*
** GetTwinListCount()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE ARRAYINDEX GetTwinListCount(HTWINLIST htl)
{
   ASSERT(IS_VALID_HANDLE(htl, TWINLIST));

   return(GetPtrCount(((PCTWINLIST)htl)->hpa));
}


/*
** GetTwinFromTwinList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HTWIN GetTwinFromTwinList(HTWINLIST htl, ARRAYINDEX ai)
{
   HTWIN htwin;

   ASSERT(IS_VALID_HANDLE(htl, TWINLIST));

   ASSERT(ai < GetPtrCount(((PCTWINLIST)htl)->hpa));

   htwin = GetPtr(((PCTWINLIST)htl)->hpa, ai);

   ASSERT(IS_VALID_HANDLE(htwin, TWIN));

   return(htwin);
}


/*
** IsValidHTWINLIST()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHTWINLIST(HTWINLIST htl)
{
   return(IS_VALID_STRUCT_PTR((PCTWINLIST)htl, CTWINLIST));
}


/***************************** Exported Functions ****************************/


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | CreateTwinList | Creates a new empty twin list.

@parm HBRFCASE | hbr | A handle to the open briefcase that the twin list is to
be associated with.

@parm PHTWINLIST | phtl | A pointer to an HTWINLIST to be filled in with a
handle to the new twin list.  *phtl is only valid if TR_SUCCESS is returned.

@rdesc If the twin list was created successfully, TR_SUCCESS is returned, and
*phtl contains a handle to the new twin list.  Otherwise, the twin list was
not created successfully, the return value indicates the error that occurred,
and *phtl is undefined.

@xref DeleteTwinList

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI CreateTwinList(HBRFCASE hbr, PHTWINLIST phtl)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(CreateTwinList);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbr, BRFCASE) &&
          IS_VALID_WRITE_PTR(phtl, HTWINLIST))
#endif
      {
         PTWINLIST ptl;

         tr = TR_OUT_OF_MEMORY;

         if (AllocateMemory(sizeof(*ptl), &ptl))
         {
            NEWPTRARRAY npa;

            /* Try to create a twin list pointer array. */

            npa.aicInitialPtrs = NUM_START_TWIN_HANDLES;
            npa.aicAllocGranularity = NUM_TWIN_HANDLES_TO_ADD;
            npa.dwFlags = NPA_FL_SORTED_ADD;

            if (CreatePtrArray(&npa, &(ptl->hpa)))
            {
               ptl->hbr = hbr;

               *phtl = (HTWINLIST)ptl;
               tr = TR_SUCCESS;

               ASSERT(IS_VALID_HANDLE(*phtl, TWINLIST));
            }
            else
               FreeMemory(ptl);
         }
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(CreateTwinList, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | DestroyTwinList | Destroys a twin list.

@parm HTWINLIST | htl | A handle to the twin list to be destroyed.

@rdesc If the twin list was destroyed successfully, TR_SUCCESS is returned.
Otherwise, the twin list was not destroyed successfully, and the return value
indicates the error that occurred.

@xref CreateTwinList

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI DestroyTwinList(HTWINLIST htl)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(DestroyTwinList);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(htl, TWINLIST))
#endif
      {
         /* Unlock all twins. */

         MyRemoveAllTwinsFromTwinList((PCTWINLIST)htl);

         DestroyPtrArray(((PCTWINLIST)htl)->hpa);

         FreeMemory((PTWINLIST)htl);

         tr = TR_SUCCESS;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(DestroyTwinList, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | AddTwinToTwinList | Adds a twin to a twin list.

@parm HTWINLIST | htl | A handle to the twin list that the twin is to be added
to.

@parm HTWIN | htwin | A handle to the twin to be added to the twin list.

@rdesc If the twin was added to the twin list successfully, TR_SUCCESS is
returned.  Otherwise, the twin was not added to the twin list successfully, and
the return value indicates the error that occurred.

@comm If the twin associated with htwin is part of an open briefcase other than
the open briefcase associated with htl, TR_INVALID_PARAMETER is returned.  If
htwin has already been added to the twin list, TR_DUPLICATE_TWIN is returned.
If htwin refers to a deleted twin, TR_DELETED_TWIN is returned.

@xref RemoveTwinFromTwinList

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI AddTwinToTwinList(HTWINLIST htl, HTWIN htwin)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(AddTwinToTwinList);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(htl, TWINLIST) &&
          IS_VALID_HANDLE(htwin, TWIN))
#endif
      {
         if (IsStubFlagClear((PCSTUB)htwin, STUB_FL_UNLINKED))
         {
            if (GetTwinBriefcase(htwin) == ((PCTWINLIST)htl)->hbr)
               tr = MyAddTwinToTwinList((PCTWINLIST)htl, htwin);
            else
               tr = TR_INVALID_PARAMETER;
         }
         else
            tr = TR_DELETED_TWIN;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(AddTwinToTwinList, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | AddAllTwinsToTwinList | Adds all the twins in an open
briefcase to a twin list.

@parm HTWINLIST | htl | A handle to the twin list that the twins are to be
added to.

@rdesc If the twins were added to the twin list successfully, TR_SUCCESS is
returned.  Otherwise, the twins were not added to the twin list successfully,
and the return value indicates the error that occurred.

@xref RemoveAllTwinsFromTwinList

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI AddAllTwinsToTwinList(HTWINLIST htl)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(AddAllTwinsToTwinList);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(htl, TWINLIST))
#endif
      {
         HTWIN htwinUnused;

         if (! EnumTwins(((PCTWINLIST)htl)->hbr, &AddTwinToTwinListProc, (LPARAM)htl, &htwinUnused))
            tr = TR_SUCCESS;
         else
            tr = TR_OUT_OF_MEMORY;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(AddAllTwinsToTwinList, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | RemoveTwinFromTwinList | Removes a twin from a twin list.

@parm HTWINLIST | htl | A handle to the twin list that the twin is to be
removed from.

@parm HTWIN | htwin | A handle to the twin to be removed from the twin list.

@rdesc If the twin was removed from the twin list successfully, TR_SUCCESS is
returned.  Otherwise, the twin was not removed from the twin list successfully,
and the return value indicates the error that occurred.

@comm If the twin associated with htwin is not in the twin list,
TR_INVALID_PARAMETER is returned.

@xref AddTwinToTwinList

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI RemoveTwinFromTwinList(HTWINLIST htl, HTWIN htwin)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(RemoveTwinFromTwinList);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(htl, TWINLIST) &&
          IS_VALID_HANDLE(htwin, TWIN))
#endif
      {
         tr = MyRemoveTwinFromTwinList((PCTWINLIST)htl, htwin);
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(RemoveTwinFromTwinList, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | RemoveAllTwinsFromTwinList | Removes all the twins from a
twin list.

@parm HTWINLIST | htl | A handle to the twin list to be emptied.

@rdesc If the twins were removed from the twin list successfully, TR_SUCCESS is
returned.  Otherwise, the twins were not removed from the twin list
successfully, and the return value indicates the error that occurred.

@xref AddAllTwinsToTwinList

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI RemoveAllTwinsFromTwinList(HTWINLIST htl)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(RemoveAllTwinsFromTwinList);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(htl, TWINLIST))
#endif
      {
         MyRemoveAllTwinsFromTwinList((PCTWINLIST)htl);

         tr = TR_SUCCESS;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(RemoveAllTwinsFromTwinList, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}

