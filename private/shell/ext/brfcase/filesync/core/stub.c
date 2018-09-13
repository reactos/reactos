/*
 * stub.c - Stub ADT module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "stub.h"


/* Macros
 *********/

/* get a pointer to the stub type descriptor for a STUB */

#define GetStubTypeDescriptor(pcs)     (&(Mrgcstd[pcs->st]))


/* Types
 ********/

/* stub functions */

typedef TWINRESULT (*UNLINKSTUBPROC)(PSTUB);
typedef void (*DESTROYSTUBPROC)(PSTUB);
typedef void (*LOCKSTUBPROC)(PSTUB);
typedef void (*UNLOCKSTUBPROC)(PSTUB);

/* stub type descriptor */

typedef struct _stubtypedescriptor
{
   UNLINKSTUBPROC UnlinkStub;

   DESTROYSTUBPROC DestroyStub;

   LOCKSTUBPROC LockStub;

   UNLOCKSTUBPROC UnlockStub;
}
STUBTYPEDESCRIPTOR;
DECLARE_STANDARD_TYPES(STUBTYPEDESCRIPTOR);


/* Module Prototypes
 ********************/

PRIVATE_CODE void LockSingleStub(PSTUB);
PRIVATE_CODE void UnlockSingleStub(PSTUB);

#if defined(DEBUG) || defined(VSTF)

PRIVATE_CODE BOOL IsValidStubType(STUBTYPE);

#endif

#ifdef DEBUG

PRIVATE_CODE LPCTSTR GetStubName(PCSTUB);

#endif


/* Module Variables
 *******************/

#pragma data_seg(DATA_SEG_READ_ONLY)

/* stub type descriptors */

/* Cast off compiler complaints about pointer argument mismatch. */

PRIVATE_DATA CONST STUBTYPEDESCRIPTOR Mrgcstd[] =
{
   /* object twin STUB descriptor */

   {
      (UNLINKSTUBPROC)UnlinkObjectTwin,
      (DESTROYSTUBPROC)DestroyObjectTwin,
      LockSingleStub,
      UnlockSingleStub
   },

   /* twin family STUB descriptor */

   {
      (UNLINKSTUBPROC)UnlinkTwinFamily,
      (DESTROYSTUBPROC)DestroyTwinFamily,
      LockSingleStub,
      UnlockSingleStub
   },

   /* folder pair STUB descriptor */

   {
      (UNLINKSTUBPROC)UnlinkFolderPair,
      (DESTROYSTUBPROC)DestroyFolderPair,
      (LOCKSTUBPROC)LockFolderPair,
      (UNLOCKSTUBPROC)UnlockFolderPair
   }
};

#pragma data_seg()


/***************************** Private Functions *****************************/


/*
** LockSingleStub()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void LockSingleStub(PSTUB ps)
{
   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));

   ASSERT(IsStubFlagClear(ps, STUB_FL_UNLINKED));

   ASSERT(ps->ulcLock < ULONG_MAX);
   ps->ulcLock++;

   return;
}


/*
** UnlockSingleStub()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void UnlockSingleStub(PSTUB ps)
{
   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));

   if (EVAL(ps->ulcLock > 0))
   {
      ps->ulcLock--;

      if (! ps->ulcLock &&
          IsStubFlagSet(ps, STUB_FL_UNLINKED))
         DestroyStub(ps);
   }

   return;
}


#if defined(DEBUG) || defined(VSTF)

/*
** IsValidStubType()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidStubType(STUBTYPE st)
{
   BOOL bResult;

   switch (st)
   {
      case ST_OBJECTTWIN:
      case ST_TWINFAMILY:
      case ST_FOLDERPAIR:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("IsValidStubType(): Invalid STUB type %d."),
                    st));
   }

   return(bResult);
}

#endif


#ifdef DEBUG

/*
** GetStubName()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE LPCTSTR GetStubName(PCSTUB pcs)
{
   LPCTSTR pcszStubName;

   ASSERT(IS_VALID_STRUCT_PTR(pcs, CSTUB));

   switch (pcs->st)
   {
      case ST_OBJECTTWIN:
         pcszStubName = TEXT("object twin");
         break;

      case ST_TWINFAMILY:
         pcszStubName = TEXT("twin family");
         break;

      case ST_FOLDERPAIR:
         pcszStubName = TEXT("folder twin");
         break;

      default:
         ERROR_OUT((TEXT("GetStubName() called on unrecognized stub type %d."),
                    pcs->st));
         pcszStubName = TEXT("UNKNOWN");
         break;
   }

   ASSERT(IS_VALID_STRING_PTR(pcszStubName, CSTR));

   return(pcszStubName);
}

#endif


/****************************** Public Functions *****************************/


/*
** InitStub()
**
** Initializes a stub.
**
** Arguments:     ps - pointer to stub to be initialized
**                st - type of stub
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void InitStub(PSTUB ps, STUBTYPE st)
{
   ASSERT(IS_VALID_WRITE_PTR(ps, STUB));
   ASSERT(IsValidStubType(st));

   ps->st = st;
   ps->ulcLock = 0;
   ps->dwFlags = 0;

   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));

   return;
}


/*
** DestroyStub()
**
** Destroys a stub.
**
** Arguments:     ps - pointer to stub to be destroyed
**
** Returns:       TWINRESULT
**
** Side Effects:  Depends upon stub type.
*/
PUBLIC_CODE TWINRESULT DestroyStub(PSTUB ps)
{
   TWINRESULT tr;
   PCSTUBTYPEDESCRIPTOR pcstd;

   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));

#ifdef DEBUG

   if (IsStubFlagSet(ps, STUB_FL_UNLINKED) &&
       ps->ulcLock > 0)
      WARNING_OUT((TEXT("DestroyStub() called on unlinked locked %s stub %#lx."),
                   GetStubName(ps),
                   ps));

#endif

   pcstd = GetStubTypeDescriptor(ps);

   /* Is the stub already unlinked? */

   if (IsStubFlagSet(ps, STUB_FL_UNLINKED))
      /* Yes. */
      tr = TR_SUCCESS;
   else
      /* No.  Unlink it. */
      tr = (*(pcstd->UnlinkStub))(ps);

   /* Is the stub still locked? */

   if (tr == TR_SUCCESS && ! ps->ulcLock)
      /* No.  Wipe it out. */
      (*(pcstd->DestroyStub))(ps);

   return(tr);
}


/*
** LockStub()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void LockStub(PSTUB ps)
{
   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));

   (*(GetStubTypeDescriptor(ps)->LockStub))(ps);

   return;
}


/*
** UnlockStub()
**
** Unlocks a stub.  Carries out any pending deletion on the stub.
**
** Arguments:     ps - pointer to stub to be unlocked
**
** Returns:       void
**
** Side Effects:  If the stub is unlinked and the lock count decreases to 0
**                after unlocking, the stub is deleted.
*/
PUBLIC_CODE void UnlockStub(PSTUB ps)
{
   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));

   (*(GetStubTypeDescriptor(ps)->UnlockStub))(ps);

   return;
}


/*
** GetStubFlags()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE DWORD GetStubFlags(PCSTUB pcs)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcs, CSTUB));

   return(pcs->dwFlags);
}


/*
** SetStubFlag()
**
** Sets given flag in a stub.  Other flags in stub are not affected.
**
** Arguments:     ps - pointer to stub whose flags are to be set
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void SetStubFlag(PSTUB ps, DWORD dwFlags)
{
   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_STUB_FLAGS));

   SET_FLAG(ps->dwFlags, dwFlags);

   return;
}


/*
** ClearStubFlag()
**
** Clears given flag in a stub.  Other flags in stub are not affected.
**
** Arguments:     ps - pointer to stub whose flags are to be set
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void ClearStubFlag(PSTUB ps, DWORD dwFlags)
{
   ASSERT(IS_VALID_STRUCT_PTR(ps, CSTUB));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_STUB_FLAGS));

   CLEAR_FLAG(ps->dwFlags, dwFlags);

   return;
}


/*
** IsStubFlagSet()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsStubFlagSet(PCSTUB pcs, DWORD dwFlags)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcs, CSTUB));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_STUB_FLAGS));

   return(IS_FLAG_SET(pcs->dwFlags, dwFlags));
}


/*
** IsStubFlagClear()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsStubFlagClear(PCSTUB pcs, DWORD dwFlags)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcs, CSTUB));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_STUB_FLAGS));

   return(IS_FLAG_CLEAR(pcs->dwFlags, dwFlags));
}


#ifdef VSTF

/*
** IsValidPCSTUB()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCSTUB(PCSTUB pcs)
{
   BOOL bResult;

   if (IS_VALID_READ_PTR(pcs, CSTUB) &&
       IsValidStubType(pcs->st) &&
       FLAGS_ARE_VALID(pcs->dwFlags, ALL_STUB_FLAGS))
      bResult = TRUE;
   else
      bResult = FALSE;

   return(bResult);
}

#endif

