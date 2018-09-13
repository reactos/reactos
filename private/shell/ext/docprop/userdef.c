////////////////////////////////////////////////////////////////////////////////
//
// UserDef.c
//
// MS Office User Defined Property Information
//
// Notes:
//  To make this file useful for OLE objects, define OLE_PROPS.
//
//  The macro lpDocObj must be used for all methods to access the
//  object data to ensure that this will compile with OLE_PROPS defined.
//
// Data structures:
//  The dictionary is stored internally as a map, mapping PIDs
//  to the string names.
//
//  The properties themselves are stored internally as a linked list
//
// Change history:
//
// Date         Who             What
// --------------------------------------------------------------------------
// 06/27/94     B. Wentz        Created file
// 07/03/94     B. Wentz        Added Iterator support
// 07/20/94              M. Jansson               Updated include statements, due to changes in PDK
// 07/26/94     B. Wentz        Changed Load & Save to use Document Summary stream
// 07/08/96     MikeHill        Ignore UserDef properties that aren't UDTYPEs.
//
////////////////////////////////////////////////////////////////////////////////

#include "priv.h"
#pragma hdrstop


static void PASCAL RemoveFromList (LPUDOBJ lpUDObj, LPUDPROP lpudprop);
static void PASCAL DeallocNode (LPUDOBJ lpUDObj, LPUDPROP lpudp);

static void PASCAL VUdpropFreeString (LPUDPROP lpudp, BOOL fName);
static BOOL PASCAL FUdpropUpdate (LPUDPROP lpudp, LPUDOBJ  lpUDObj, LPTSTR lpszPropName, LPTSTR lpszLinkMonik, LPVOID lpvValue, UDTYPES udtype, BOOL fLink);
static BOOL PASCAL FUdpropSetString (LPUDPROP lpudp, LPTSTR lptstr, BOOL fLimitLength, BOOL fName);
static BOOL PASCAL FUdpropMakeVisible (LPUDPROP lpudprop);
static BOOL PASCAL FUserDefMakeHidden (LPUDOBJ lpUDObj, LPTSTR lpsz);
static BOOL PASCAL FUdpropMakeHidden (LPUDPROP lpudprop);

#define lpDocObj  (lpUDObj)
#define lpData    ((LPUDINFO) lpUDObj->m_lpData)

////////////////////////////////////////////////////////////////////////////////
//
// OfficeDirtyUDObj
//
// Purpose:
//  Sets object state to dirty or clean.
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT VOID OfficeDirtyUDObj
    (LPUDOBJ lpUDObj,             // The object
     BOOL fDirty)                 // Flag indicating if the object is dirty.
{
    Assert(lpUDObj != NULL);
    lpUDObj->m_fObjChanged = fDirty;
} // OfficeDirtyUDObj


////////////////////////////////////////////////////////////////////////////////
//
// FreeUDData
//
// Purpose:
//  Deallocates all the member data for the object
//
// Note:
//  Assumes object is valid.
//
////////////////////////////////////////////////////////////////////////////////
void PASCAL
FreeUDData
    (LPUDOBJ lpUDObj,                   // Pointer to valid object
     BOOL fTmp)                         // Indicates tmp data should be freed
{
    LPUDPROP lpudp;
    LPUDPROP lpudpT;


    lpudp = (fTmp) ? lpData->lpudpTmpHead : lpData->lpudpHead;

    while (lpudp != NULL)
    {
        lpudpT = lpudp;
        lpudp = (LPUDPROP) lpudp->llist.lpllistNext;
        VUdpropFree(&lpudpT);
    }

    if (fTmp)
    {
        lpData->lpudpTmpCache = NULL;
        lpData->lpudpTmpHead = NULL;
        lpData->dwcTmpProps = 0;
        lpData->dwcTmpLinks = 0;
    }
    else
    {
        lpData->lpudpCache = NULL;
        lpData->lpudpHead = NULL;
        lpData->dwcProps = 0;
        lpData->dwcLinks = 0;
    }

} // FreeUDData

////////////////////////////////////////////////////////////////////////////////
//
// FUserDefCreate
//
// Purpose:
//  Create a User-defined property exchange object.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
FUserDefCreate
    (LPUDOBJ FAR *lplpUDObj,              // Pointer to pointer to object
     void *prglpfn[])                     // Pointer to functions
{
    LPUDOBJ lpUDObj;  // Hack - a temp, must call it "lpUdObj" for macros to work!

    if (lplpUDObj == NULL)
        return(TRUE);

      // Make sure we get valid args before we start alloc'ing
    if ((prglpfn == NULL) || (prglpfn[ifnCPConvert] == NULL) ||
        ((prglpfn[ifnFSzToNum] == NULL) && (prglpfn[ifnFNumToSz] != NULL)) ||
        ((prglpfn[ifnFSzToNum] != NULL) && (prglpfn[ifnFNumToSz] == NULL))
       )
    {
        return FALSE;
    }

    if ((*lplpUDObj = PvMemAlloc(sizeof(USERPROP))) == NULL)
    {
        // REVIEW: Add alert
        return FALSE;
    }

    lpDocObj = *lplpUDObj;

    //
    // If alloc fails, free the original object too.
    //
    if ((lpData = PvMemAlloc(sizeof (UDINFO))) == NULL)
    {
        //
        // REVIEW: Add alert
        //
        VFreeMemP(*lplpUDObj, sizeof(USERPROP));
        return FALSE;
    }

    FillBuf ((void *) lpData, (int) 0, sizeof(UDINFO) );

    //
    // Save the fnc's for code page convert, SzToNum, NumToSz
    //
    lpData->lpfnFCPConvert = (BOOL (*)(LPTSTR, DWORD, DWORD, BOOL)) prglpfn[ifnCPConvert];
    lpData->lpfnFSzToNum = (BOOL (*)(NUM *, LPTSTR)) prglpfn[ifnFSzToNum];
    lpData->lpfnFNumToSz = (BOOL (*)(NUM *, LPTSTR, DWORD)) prglpfn[ifnFNumToSz];

    OfficeDirtyUDObj (*lplpUDObj, FALSE);
    (*lplpUDObj)->m_hPage = NULL;

    return TRUE;

} // FUserDefCreate


////////////////////////////////////////////////////////////////////////////////
//
// FUserDefDestroy
//
// Purpose:
//  Destroy a User-defined property exchange object.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
FUserDefDestroy
    (LPUDOBJ FAR *lplpUDObj)              // Pointer to pointer to object
{
#define lpUDData    ((LPUDINFO)(((LPUDOBJ) *lplpUDObj)->m_lpData))

    DWORD irg;

    if ((lplpUDObj == NULL) || (*lplpUDObj == NULL))
        return TRUE;

    if (lpUDData != NULL)
    {
        FreeUDData (*lplpUDObj, FALSE);
        FreeUDData (*lplpUDObj, TRUE);

        //
        // Invalidate any OLE Automation DocumentProperty objects we might have
        //
        InvalidateVBAObjects(NULL, NULL, *lplpUDObj);

        VFreeMemP((*lplpUDObj)->m_lpData, sizeof(UDINFO));
    }

    VFreeMemP(*lplpUDObj, sizeof(USERPROP));
    *lplpUDObj = NULL;
    return TRUE;

#undef lpUDData
} // FUserDefDestroy


////////////////////////////////////////////////////////////////////////////////
//
// FUserDefClear
//
// Purpose:
//  Clears a User-defined property object without destroying it.  All stored
//  data is lost.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
FUserDefClear(LPUDOBJ lpUDObj)                     // Pointer to object
{

    if ((lpDocObj == NULL) || (lpData == NULL))
        return TRUE;

    FreeUDData (lpDocObj, FALSE);
    FreeUDData (lpDocObj, TRUE);

    //
    // Invalidate any OLE Automation DocumentProperty objects we might have
    //
    InvalidateVBAObjects(NULL, NULL, lpUDObj);

    //
    // Clear the data, don't blt over the fn's stored at the end.
    //
    FillBuf ((void *) lpData, (int) 0, (sizeof (UDINFO) - ifnUDMax*(sizeof (void *))));

    OfficeDirtyUDObj (lpUDObj, TRUE);
    return TRUE;

} // FUserDefClear


////////////////////////////////////////////////////////////////////////////////
//
// FUserDefShouldSave
//
// Purpose:
//  Indicates if the data has changed, meaning a write is needed.
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT BOOL
FUserDefShouldSave
    (LPUDOBJ lpUDObj)             // Pointer to object
{
    if (lpUDObj == NULL)
        return FALSE;

    return lpDocObj->m_fObjChanged;

} // FUserDefShouldSave




////////////////////////////////////////////////////////////////////////////////
//
// UdtypesUserDefType
//
// Purpose:
//  Returns the type of the given Property Value from the string
//
// Returns wUDInvalid on error
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT UDTYPES
UdtypesUserDefType
    (LPUDOBJ lpUDObj,
     LPTSTR lpsz)
{
    LPUDPROP lpudprop;

    if ((lpUDObj == NULL)   ||
        (lpData == NULL)    ||
        (lpsz == NULL)
       )
    {
        return wUDinvalid;
    }

    //
    // Find the node that has this name.
    //
    lpudprop = LpudpropFindMatchingName (lpUDObj, lpsz);
    if (lpudprop == NULL || lpudprop->lppropvar == NULL)
        return wUDinvalid;

    // Return the VarType (which is a UDTYPE)

    return (lpudprop->lppropvar->vt);

} // UdtypesUserDefType


////////////////////////////////////////////////////////////////////////////////
//
// LpvoidUserDefGetPropVal
//
// Purpose:
//   This will return the Property Value for the given Property String.
//
////////////////////////////////////////////////////////////////////////////////

DLLEXPORT LPVOID
LpvoidUserDefGetPropVal
    (LPUDOBJ lpUDObj,             // Pointer to object
     LPTSTR lpszProp,             // Property string
     DWORD cbMax,                 // Size of lpv
     LPVOID lpv,                  // Buffer for prop val
     DWORD dwMask,                // Mask for what value is needed
     BOOL *pfLink,                // Indicates a link
     BOOL *pfLinkInvalid)         // Is the link invalid
{
    LPUDPROP lpudprop;

    if ((lpUDObj == NULL)           ||
        (lpData == NULL)            ||
        (lpszProp == NULL)          ||
        (cbMax == 0 && lpv != NULL) ||
        (pfLink == NULL)            ||
        (pfLinkInvalid == NULL)     ||
        ((lpv == NULL) && (!(dwMask & UD_PTRWIZARD)))
       )
    {
        return NULL;
    }

    //
    // Find the node that has this name.
    //

    lpudprop = LpudpropFindMatchingName (lpUDObj, lpszProp);
    if (lpudprop == NULL)
        return NULL;
    Assert (lpudprop->lppropvar != NULL);

    *pfLink = (lpudprop->lpstzLink != NULL);

    // Links are always invalid in the shell (there's no app to update the data).

#ifdef SHELL
    *pfLinkInvalid = lpudprop->fLinkInvalid = TRUE;
#else
    *pfLinkInvalid = lpudprop->fLinkInvalid;
#endif

    //
    // Return based on the type and flags
    //

    if (dwMask & UD_LINK)
    {
        if (dwMask & UD_PTRWIZARD)
        {
            if (lpudprop->lpstzLink != NULL)
                return((LPVOID) (PSTR (lpudprop->lpstzLink)));
            return(NULL);
        }

        if (lpudprop->lpstzLink != NULL)
        {
            PbSzNCopy (lpv, lpudprop->lpstzLink, cbMax);
            return (lpv);
        }
        else
          return(NULL);
    }

    if (dwMask & UD_PTRWIZARD)
    {
        // If this is a string, return it's pointer from the
        // PropVariant.  Otherwise, return a pointer into
        // the data of the PropVariant.

        return (lpudprop->lppropvar->vt == VT_LPTSTR) ?
               (LPVOID) lpudprop->lppropvar->pszVal :
               (LPVOID) &lpudprop->lppropvar->lVal;
    }

    // Copy the property from the PropVariant to the caller-provided
    // buffer.

    return( FPropVarCopyToBuf( lpudprop->lppropvar,
                               cbMax,
                               lpv
                             ) ? lpv : NULL
           );

} // LpvoidUserDefGetPropVal


////////////////////////////////////////////////////////////////////////////////
//
// LppropvarUserDefAddProp
//
// Purpose:
//  This will add a new Property to the set, with the given
//  Property string, type, and data.
//
////////////////////////////////////////////////////////////////////////////////

DLLEXPORT LPPROPVARIANT
LppropvarUserDefAddProp
    (LPUDOBJ lpUDObj,             // Pointer to object
     LPTSTR lpszPropName,         // Property string
     LPVOID lpvVal,               // Property value
     UDTYPES udtype,              // Property type
     LPTSTR lpszLinkMonik,        // Link name
     BOOL fLink,                  // Indicates the property is a link
     BOOL fHidden)                // Indicates the property is hidden
{
    LPUDPROP lpudprop;
    LPUDPROP lpudpropMatch;
    BOOL     fCreated;

    if ((lpUDObj == NULL)   ||
        (lpData == NULL)    ||
        (lpszPropName == NULL) ||
        (*lpszPropName == 0) ||
        (lpvVal == NULL) ||
        (!ISUDTYPE(udtype)) ||
        (fLink && (lpszLinkMonik == NULL))
       )
    {
      return FALSE;
    }

    // Create a UDPROP to be added to the linked-list.

    lpudprop = LpudpropCreate();
    if (lpudprop == NULL)
        return FALSE;

    // Put the data into the UDPROP.

    if (!FUdpropUpdate( lpudprop,
                        lpUDObj,
                        lpszPropName,
                        lpszLinkMonik,
                        lpvVal,
                        udtype,
                        fLink)
        )
    {
        VUdpropFree (&lpudprop);
        return(FALSE);
    }

    //
    // Find this node
    //

    lpudpropMatch = LpudpropFindMatchingName (lpUDObj, lpszPropName);
    if (lpudpropMatch==NULL)
    {
        //
        // Create a node and put it in the list
        // If a new node was created, it must be added to the list...
        //

        if (fLink)
           lpData->dwcLinks++;

        lpData->dwcProps++;
        AddNodeToList (lpUDObj, lpudprop);

    }   // if (lpudpropMatch==NULL)

    else
    {
        // We must replace the existing UDPROP with the new
        // value.

        // Free any existing property name and link name in this
        // UDPROP, and free its value.

        VUdpropFreeString (lpudpropMatch, TRUE);
        VUdpropFreeString (lpudpropMatch, FALSE);
        PropVariantClear (lpudpropMatch->lppropvar);
        CoTaskMemFree (lpudpropMatch->lppropvar);
        lpudpropMatch->lppropvar = NULL;

        // Put the linked-list pointer in the existing UDPROP into
        // the new UDPROP, then copy the new UDPROP back over
        // the matching PROP (this way, we don't have to
        // update the UDPROP that points to the match).

        lpudprop->llist=lpudpropMatch->llist;
        PbMemCopy(lpudpropMatch, lpudprop, sizeof(UDPROP));

        // Clear out the caller-provided UDPROP, free it, but
        // then set the pointer to the matching entry and clear
        // the match pointer.  Thus, after we're done and whether
        // there was a match or not, lpudprop will point to the
        // correct UDPROP.

        FillBuf (lpudprop, 0, sizeof(UDPROP));
        VUdpropFree (&lpudprop);
        lpudprop = lpudpropMatch;
        lpudpropMatch = NULL;

    }   // if (lpudpropMatch==NULL) ... else

    //
    // If the client asked for a hidden property, do it if
    // the name was the real name, not a link
    //

    if (fHidden && !fLink)
    {
        fCreated=FUserDefMakeHidden (lpUDObj, lpszPropName);      // Should never return false
        Assert(fCreated);
    }

    OfficeDirtyUDObj (lpUDObj, TRUE);

    // If successful, return a pointer to the PropVariant with the value.
    if (lpudprop)
        return lpudprop->lppropvar;
    else
        return NULL;

} // LppropvarUserDefAddProp


////////////////////////////////////////////////////////////////////////////////
//
// FUserDefDeleteProp
//
// Purpose:
//  This will delete a Property from the set given a Property string.
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT BOOL
FUserDefDeleteProp
  (LPUDOBJ lpUDObj,             // Pointer to object
   LPTSTR lpsz)                  // String to delete
{
  LPUDPROP lpudprop;

  if ((lpUDObj == NULL)   ||
      (lpData == NULL)    ||
      (lpsz == NULL))
    return FALSE;

    // Find the node
  lpudprop = LpudpropFindMatchingName (lpUDObj, lpsz);
  if (lpudprop == NULL)
    return FALSE;

  lpData->dwcProps--;
  if (lpudprop->lpstzLink != NULL)
    lpData->dwcLinks--;

  RemoveFromList (lpUDObj, lpudprop);
  VUdpropFree (&lpudprop);

  OfficeDirtyUDObj (lpUDObj, TRUE);
  return TRUE;

} // FUserDefDeleteProp


////////////////////////////////////////////////////////////////////////////////
//
// LpudiUserDefCreateIterator
//
// Purpose:
//  Create a User-defined Properties iterator
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT LPUDITER
LpudiUserDefCreateIterator
  (LPUDOBJ lpUDObj)                     // Pointer to object
{
  LPUDITER lpudi;

  if ((lpUDObj == NULL) ||
      (lpData == NULL) ||
                (lpData->lpudpHead == NULL))            // No custom props
    return NULL;


    // Create & Init the iterator
  lpudi = PvMemAlloc(sizeof(UDITER));
  if (lpudi == NULL)
     return(NULL);

  FillBuf (lpudi, 0, sizeof (UDITER));
  lpudi->lpudp = lpData->lpudpHead;

  return lpudi;

} // LpudiUserDefCreateIterator


////////////////////////////////////////////////////////////////////////////////
//
// FUserDefDestroyIterator
//
// Purpose:
//  Destroy a User-defined Properties iterator
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT BOOL
FUserDefDestroyIterator
  (LPUDITER *lplpUDIter)                   // Pointer to iterator
{
    if ((lplpUDIter == NULL) || (*lplpUDIter == NULL))
        return TRUE;

      VFreeMemP(*lplpUDIter, sizeof(UDITER));
      *lplpUDIter = NULL;

      return TRUE;

} // FUserDefDestroyIterator


////////////////////////////////////////////////////////////////////////////////
//
// FUserDefIteratorValid
//
// Purpose:
//  Determine if an iterator is still valid
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT BOOL
FUserDefIteratorValid
  (LPUDITER lpUDIter)                   // Pointer to iterator
{
    if (lpUDIter == NULL)
        return FALSE;

    return (lpUDIter->lpudp != NULL);

} // FUserDefIteratorValid


////////////////////////////////////////////////////////////////////////////////
//
// FUserDefIteratorNext
//
// Purpose:
//  Iterate to the next element
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT BOOL
FUserDefIteratorNext
  (LPUDITER lpUDIter)                   // Pointer to iterator
{
    if (lpUDIter == NULL)
        return FALSE;

    // Move to the next node, if possible.
#ifdef OLD
    if (lpUDIter->lpudp != NULL)
        lpUDIter->lpudp = (LPUDPROP) lpUDIter->lpudp->llist.lpllistNext;

    return TRUE;
#endif

    if (lpUDIter->lpudp == NULL)
        return FALSE;

    lpUDIter->lpudp = (LPUDPROP) lpUDIter->lpudp->llist.lpllistNext;

    return(lpUDIter->lpudp != NULL);

} // FUserDefIteratorNext

////////////////////////////////////////////////////////////////////////////////
//
// FUserDefIteratorIsLink
//
// Purpose:
//  Returns TRUE if the iterator is a link, FALSE otherwise
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT BOOL
FUserDefIteratorIsLink
  (LPUDITER lpUDIter)                   // Pointer to iterator
{
  if ((lpUDIter == NULL) || (lpUDIter->lpudp == NULL))
    return FALSE;

  return(lpUDIter->lpudp->lpstzLink != NULL);

} // FUserDefIteratorIsLink

////////////////////////////////////////////////////////////////////////////////
//
// FUserDefIteratorIsLinkInvalid
//
// Purpose:
//  Returns TRUE if the iterator is an invalid link, FALSE otherwise
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT BOOL
FUserDefIteratorIsLinkInvalid
  (LPUDITER lpUDIter)                   // Pointer to iterator
{
  if ((lpUDIter == NULL) || (lpUDIter->lpudp == NULL))
    return(FALSE);

  if (lpUDIter->lpudp->lpstzLink == NULL)
    return(FALSE);

  return(lpUDIter->lpudp->fLinkInvalid);

} // FUserDefIteratorIsLinkInvalid

////////////////////////////////////////////////////////////////////////////////
//
// UdtypesUserDefIteratorType
//
// Purpose:
//  Returns the type of the given Property Value from the iterator
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT UDTYPES
UdtypesUserDefIteratorType
  (LPUDITER lpUDIter)                   // Pointer to iterator
{
   if ((lpUDIter == NULL)  ||
       (lpUDIter->lpudp == NULL) ||
       (lpUDIter->lpudp->lppropvar == NULL))
    return wUDinvalid;

  return (lpUDIter->lpudp->lppropvar->vt);

} // UdtypesUserDefIteratorType


////////////////////////////////////////////////////////////////////////////////
//
// LpvoidUserDefGetIteratorVal
//
// Purpose:
//  This will return the Property Value for the given iterator
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT LPVOID
LpvoidUserDefGetIteratorVal
  (LPUDITER lpUDIter,                   // Pointer to iterator
   DWORD cbMax,                         // Max size of lpv
   LPVOID lpv,                          // Buffer to copy data value to
   DWORD dwMask,                        // Mask indicating the value to get
   BOOL *pfLink,                        // Flag indicating the link is desired
   BOOL *pfLinkInvalid)                 // Flag indicating if the link is invalid
{
  if ((cbMax == 0)        ||
      ((lpv == NULL) && (!(dwMask & UD_PTRWIZARD))) ||
      (lpUDIter == NULL)  ||
      (pfLink == NULL)    ||
      (pfLinkInvalid == NULL) ||
      (lpUDIter->lpudp == NULL))
    return NULL;

  *pfLink = (lpUDIter->lpudp->lpstzLink != NULL);
  *pfLinkInvalid = lpUDIter->lpudp->fLinkInvalid;

      // Return based on the type and flags
  if (dwMask & UD_LINK)
  {
    if (dwMask & UD_PTRWIZARD)
    {
      if (lpUDIter->lpudp->lpstzLink != NULL)
         return((LPVOID) (PSTR (lpUDIter->lpudp->lpstzLink)));
      return(NULL);
    }

    if (lpUDIter->lpudp->lpstzLink != NULL)
    {
       PbSzNCopy (lpv, lpUDIter->lpudp->lpstzLink, cbMax);
       return (lpv);
    }
    else
      return(NULL);
  }

  if (dwMask & UD_PTRWIZARD)
  {
    Assert (lpUDIter->lpudp->lppropvar != NULL);
    Assert (lpUDIter->lpudp->lppropvar->vt == VT_LPTSTR
            ||
            lpUDIter->lpudp->lppropvar->vt == VT_I4);

    // If this is a string property, return a pointer to it.
    // Otherwise it is a Long, and we return its value.

    return ( lpUDIter->lpudp->lppropvar->vt == VT_LPTSTR
             ? (LPVOID) lpUDIter->lpudp->lppropvar->pszVal
             : (LPVOID) lpUDIter->lpudp->lppropvar->lVal );
  }

  // Copy the value of this property into a caller-provided buffer.

  return(FPropVarCopyToBuf(lpUDIter->lpudp->lppropvar, cbMax, lpv) ? lpv : NULL);

} // LpvoidUserDefGetIteratorVal


////////////////////////////////////////////////////////////////////////////////
//
// LpszUserDefIteratorName
//
// Purpose:
//  This will return the Property String (name) for the property
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT LPTSTR
LpszUserDefIteratorName
  (LPUDITER lpUDIter,                   // Pointer to iterator
   DWORD cbMax,                         // Max size of lpsz
   LPTSTR lpsz)                         // Buffer to copy into, or UD_PTRWIZARD
{
    if ((cbMax == 0)        ||
        (lpsz == NULL)      ||
        (lpUDIter == NULL)  ||
        (lpUDIter->lpudp == NULL))
    {
        return NULL;
    }

    // If the lpsz input is not actually a pointer, but the
    // UD_PTRWIZARD value, then the caller doesn't want
    // us to copy the name into a buffer, they just want
    // us to return a pointer to the name.

    if ((INT_PTR) lpsz == UD_PTRWIZARD)
    {
        AssertSz ((IsBadReadPtr (lpsz, sizeof(LPTSTR))), TEXT("UD_PTRWIZARD should be a bogus pointer value!"));
        return (lpUDIter->lpudp->lpstzName);
    }

    // The caller gave us a real buffer in lpsz.
    // Copy the name into it, and return it.

    PbSzNCopy (lpsz, lpUDIter->lpudp->lpstzName, cbMax-1);
    lpsz[cbMax-1] = TEXT('\0');

    return lpsz;

} // LpszUserDefIteratorName


////////////////////////////////////////////////////////////////////////////////
//
// FUserDefIteratorSetPropString
//
// Purpose:
//  Sets the name of the iterator.
//
////////////////////////////////////////////////////////////////////////////////
DLLFUNC BOOL OFC_CALLTYPE
FUserDefIteratorSetPropString
  (LPUDOBJ lpUDObj,                     // Pointer to object
   LPUDITER lpUDIter,                   // Pointer to iterator
   LPTSTR lpszNew)                       // Pointer to new name
{
  if ((lpUDObj == NULL)   ||
      (lpData == NULL)    ||
      (lpszNew == NULL)   ||
      (lpUDIter == NULL)  ||
      (lpUDIter->lpudp == NULL))
    return FALSE;

    // Update the node
  if (!FUdpropSetString (lpUDIter->lpudp, lpszNew, TRUE, TRUE))
    return FALSE;

  OfficeDirtyUDObj (lpUDObj, TRUE);
  return TRUE;

} // FUserDefIteratorSetPropString


////////////////////////////////////////////////////////////////////////////////
//
// FUserDefIteratorChangeVal
//
// Purpose:
//  Changes the value of the data stored.
//
////////////////////////////////////////////////////////////////////////////////
DLLFUNC BOOL OFC_CALLTYPE
FUserDefIteratorChangeVal
  (LPUDOBJ lpUDObj,                     // Pointer to object
   LPUDITER lpUDIter,                   // Pointer to iterator
   UDTYPES udtype,                      // The type
   LPVOID lpv,                          // New value.
   BOOL fLinkInvalid)                   // Is the link still valid?
{
  if ((lpUDObj == NULL)   ||
      (lpData == NULL)    ||
      (lpUDIter == NULL)  ||
      (!ISUDTYPE (udtype))   ||
      (lpUDIter->lpudp == NULL))
    return FALSE;

  if (fLinkInvalid)
  {
      if (lpUDIter->lpudp->lpstzLink == NULL)
          return FALSE;
      lpUDIter->lpudp->fLinkInvalid = TRUE;
      return TRUE;
  }
  else
      lpUDIter->lpudp->fLinkInvalid = FALSE;


  if (!FPropVarLoad (lpUDIter->lpudp->lppropvar, (VARTYPE)udtype, lpv))
      return FALSE;


  OfficeDirtyUDObj (lpUDObj, TRUE);
  return TRUE;

} // FUserDefIteratorChangeVal


////////////////////////////////////////////////////////////////////////////////
//
// FUserDefIteratorSetLink
//
// Purpose:
//  Sets the link value for a property.  This is NOT a public API.
//
////////////////////////////////////////////////////////////////////////////////
BOOL PASCAL
FUserDefIteratorSetLink
  (LPUDOBJ lpUDObj,                     // Pointer to object
   LPUDITER lpUDIter,                   // Pointer to iterator
   LPTSTR lpszLink)                      // New link name
{
  if ((lpUDObj == NULL)   ||
      (lpData == NULL)    ||
      (lpUDIter == NULL)  ||
      (lpUDIter->lpudp == NULL))
    return FALSE;

   Assert(lpszLink != NULL);
   Assert(*lpszLink != TEXT('\0'));
   // Should already be a link
   Assert(lpUDIter->lpudp->lpstzLink != NULL);

   if (!FUdpropSetString (lpUDIter->lpudp, lpszLink, FALSE, FALSE))
       return FALSE;

   return TRUE;

} // FUserDefIteratorSetLink


////////////////////////////////////////////////////////////////////////////////
//
// LpudiUserDefCreateIterFromLpudp
//
// Purpose:
//  Creates an iterator object from a node.  This is not a public API.
//
////////////////////////////////////////////////////////////////////////////////
LPUDITER PASCAL
LpudiUserDefCreateIterFromLpudp
  (LPUDOBJ lpUDObj,                     // Pointer to object
   LPUDPROP lpudp)                      // Pointer to node
{
  LPUDITER lpudi;

  if ((lpUDObj == NULL) ||
      (lpData == NULL)  ||
      (lpudp == NULL))
    return NULL;

  lpudi = LpudiUserDefCreateIterator (lpUDObj);
  if (lpudi != NULL)
    lpudi->lpudp = lpudp;

  return lpudi;

} // LpudiUserDefCreateIterFromLpudp


////////////////////////////////////////////////////////////////////////////////
//
// FUserDefIsHidden
//
// Purpose:
//  Determine if a Property string is hidden.
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT BOOL
FUserDefIsHidden
  (LPUDOBJ lpUDObj,             // Pointer to object
   LPTSTR lpsz)                  // Property string
{
   if (lpsz == NULL)
    return FALSE;

  // We don't really need the object, we can tell from the name
  return (lpsz[0] == HIDDENPREFIX);

} // FUserDefIsHidden


////////////////////////////////////////////////////////////////////////////////
//
// FUserDefMakeVisible
//
// Purpose:
//  Make a property visible based on the Property string
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT BOOL
FUserDefMakeVisible
  (LPUDOBJ lpUDObj,             // Pointer to object
   LPTSTR lpsz)                  // String to hide.
{
  LPUDPROP lpudprop;
  ULONG    cb;

  if ((lpUDObj == NULL)   ||
      (lpData == NULL)    ||
      (lpsz == NULL))
    return FALSE;

    // Find the name
  lpudprop = LpudpropFindMatchingName (lpUDObj, lpsz);
  if (lpudprop == NULL)
    return FALSE;


  if (!FUdpropMakeVisible (lpudprop))
    return FALSE;

  OfficeDirtyUDObj (lpUDObj, TRUE);
  return TRUE;

} // FUserDefMakeVisible


////////////////////////////////////////////////////////////////////////////////
//
// FUserDefMakeHidden
//
// Purpose:
//  Hide a Property based on the Property string.
//
////////////////////////////////////////////////////////////////////////////////
static BOOL PASCAL
FUserDefMakeHidden
  (LPUDOBJ lpUDObj,             // Pointer to object
   LPTSTR lpsz)                  // String to hide
{
  LPUDPROP lpudprop;
  LPTSTR lpstzT;

  if ((lpUDObj == NULL)   ||
      (lpData == NULL)    ||
      (lpsz == NULL))
    return FALSE;

    // Find the name
  lpudprop = LpudpropFindMatchingName (lpUDObj, lpsz);
  if (lpudprop == NULL)
    return FALSE;

  if (!FUdpropMakeHidden (lpudprop))
      return FALSE;

  OfficeDirtyUDObj (lpUDObj, TRUE);
  return TRUE;

} // FUserDefMakeHidden


////////////////////////////////////////////////////////////////////////////////
//
// LpudpropFindMatchingName
//
// Purpose:
//  Returns a node with a matching name, NULL otherwise.
//
////////////////////////////////////////////////////////////////////////////////
LPUDPROP PASCAL
LpudpropFindMatchingName
  (LPUDOBJ lpUDObj,             // Pointer to object
   LPTSTR lpsz)                  // String to search for
{
  LPUDPROP lpudprop;
  TCHAR sz[256];
  BOOL fCopy = FALSE;

  if ((lpUDObj == NULL) || (lpData == NULL))
     return(NULL);

  if (CchTszLen(lpsz) > 255)
     {
     PbSzNCopy(sz, lpsz, 255);
     sz[255] = 0;
     fCopy = TRUE;
     }

    // Check the cache first
  if (lpData->lpudpCache != NULL)
  {
    Assert ((lpData->lpudpCache->lpstzName != NULL));

      // lstrcmpi returns 0 if 2 strings are equal.....
    if (!(lstrcmpi (fCopy ? sz : lpsz, (LPTSTR)PSTR (lpData->lpudpCache->lpstzName))))
      return lpData->lpudpCache;
  }

  lpudprop = lpData->lpudpHead;

  while (lpudprop != NULL)
  {
    Assert ((lpudprop->lpstzName != NULL));

      // lstrcmpi returns 0 if 2 strings are equal.....
    if (!(lstrcmpi (fCopy ? sz : lpsz, (LPTSTR)PSTR (lpudprop->lpstzName))))
    {
        // Set the cache to the last node found
      lpData->lpudpCache = lpudprop;
      return lpudprop;
    }

    lpudprop = (LPUDPROP) lpudprop->llist.lpllistNext;

  } // while

  return NULL;

} // LpudpropFindMatchingName


////////////////////////////////////////////////////////////////////////////////
//
//  LpudpropFindMatchingPID
//
//  Purpose:
//      Searches the linked-list in the caller-provided UDINFO structure
//      for a UDPROP with the requested PropID.
//
//  Inputs:
//      LPUDOBJ - The UDINFO structure
//      PROPID  - The PID to search for.
//
//  Output:
//      The requested LPUDPROP, or NULL if not found.
//
////////////////////////////////////////////////////////////////////////////////
LPUDPROP PASCAL
LpudpropFindMatchingPID
  (LPUDOBJ lpUDObj,
   PROPID propid)
{
    //  ------
    //  Locals
    //  ------

    LPUDPROP lpudprop = NULL;
    BOOL fCopy = FALSE;

    //  -----
    //  Begin
    //  -----

    // Validate the inputs.

    if ((lpUDObj == NULL) || (lpData == NULL))
    {
        AssertSz (0, TEXT("Invalid inputs"));
        goto Exit;
    }


    // Check the cache first

    if (lpData->lpudpCache != NULL
        &&
        lpData->lpudpCache->propid == propid)
    {
        lpudprop = lpData->lpudpCache;
        goto Exit;
    }

    // Search the linked-list.

    lpudprop = lpData->lpudpHead;
    while (lpudprop != NULL)
    {
        if (lpudprop->propid == propid)
        {
            lpData->lpudpCache = lpudprop;
            goto Exit;
        }

        lpudprop = (LPUDPROP) lpudprop->llist.lpllistNext;

    }

    //  ----
    //  Exit
    //  ----

Exit:

  return lpudprop;


} // LpudpropFindMatchingPID



////////////////////////////////////////////////////////////////////////////////
//
// FAddPropToList
//
// Purpose:
//  Adds the given object to the list.  The type and value must
//  be filled in before calling this.
//
// The linked-list we're adding to has one entry for each of
// the user-defined properties.  Each entry has the property
// value, it's PID, and it's name.  If the property is linked
// to document content, the link name (e.g. a Bookmark name
// in Word) is also in this entry.  Note that the property set
// stores the property value as one property, with its name in the
// dictionary, and it stores the link name as a second property.
// Consequently, this routine will be called twice for such
// properties:  on the first call we'll create a new entry in the
// linked-list, adding the property ID, name, and value; on the
// second call we'll pull out that entry, and add the link name.
//
// On success, the input lppropvar & lpstatpropstg are cleared.
// On error, all inputs are left unmodified.
//
////////////////////////////////////////////////////////////////////////////////
BOOL PASCAL
FAddPropToList
  (LPUDOBJ lpUDObj,
   LPPROPVARIANT lppropvar,
   STATPROPSTG   *lpstatpropstg,
   LPUDPROP lpudprop)           // Property to add
{
    //  ------
    //  Locals
    //  ------

    BOOL                fSuccess = FALSE;
    LPTSTR              lpstz;
    LPUDPROP            lpudpT;
    BOOL                fLink;
    USES_CONVERSION ;

    Assert(lpUDObj != NULL);
    Assert(lpudprop != NULL);      // Is this a bogus assert?
    Assert(lppropvar != NULL && lpstatpropstg != NULL);

    // If the PId has one of the special masks, strip it off
    // so the PId will match the normal value.

    fLink = lpstatpropstg->propid & PID_LINKMASK;
    lpstatpropstg->propid &= ~PID_LINKMASK;


    //  ------------------------------------------------------------
    //  See if we can find this property already in the linked-list.
    //  If we have a name, use that, otherwise use the PID.
    //  ------------------------------------------------------------

    if (lpstatpropstg->lpwstrName != NULL)
    {
        // Search by name.

		//	[scotthan] Re: bogus cast to TCHAR in propio.c when this thing
        //  was read out of the file.  If this is an ANSI build, it's going to store
        //  a TCHAR* value!.  So we need to reciprocate the cast...
        lpudpT = LpudpropFindMatchingName (lpUDObj, (LPTSTR)lpstatpropstg->lpwstrName );
    }
    else
    {
        // Search by PID
        lpudpT = LpudpropFindMatchingPID (lpUDObj, lpstatpropstg->propid);
    }

    //  --------------------------------------------------------------
    //  If this property isn't already in the linked-list, add it now.
    //  --------------------------------------------------------------

    if (lpudpT == NULL)
    {
        // This should be a named property.  If it's not
        // named, then it should be a link, and the property
        // it links should have been in the linked-list already
        // (i.e., the lpudpT should have been non-NULL).

        if (lpstatpropstg->lpwstrName == NULL)
        {
            AssertSz (0, TEXT("Missing name in User-Defined properties"));
            goto Exit;
        }

        // Allocate memory for the property value.

        lpudprop->lppropvar = CoTaskMemAlloc (sizeof(PROPVARIANT));
        if (lpudprop->lppropvar == NULL)
        {
            AssertSz (0, TEXT("Couldn't allocate lpudprop->lppropvar"));
            goto Exit;
        }

        // Load the property ID, name, and value.
        // Note that if we had an error before here, we left
        // the caller's inputs un-touched.  Since no more errors
        // can occur, we'll never have half-modified data in an
        // error case.

        lpudprop->propid = lpstatpropstg->propid;

		//	[scotthan] Re: bogus cast to TCHAR in propio.c when this thing
        //  was read out of the file.  If this is an ANSI build, it's going to store
        //  a TCHAR* value!.  So we need to reciprocate the cast...
        lpudprop->lpstzName = (LPTSTR)lpstatpropstg->lpwstrName;
        lpstatpropstg->lpwstrName = NULL;

        *lpudprop->lppropvar = *lppropvar;
        PropVariantInit (lppropvar);

        lpData->dwcProps++;
        AddNodeToList (lpUDObj, lpudprop);

    } // if ((lpudpT = LpudpropFindMatchingName (lpUDInfo, lpstatpropstg->lpwsz)) == NULL)


    //  --------------------------------------------------------
    //  Otherwise (this property is already in the linked-list),
    //  add this new link name or value to the UDPROP.
    //  --------------------------------------------------------

    else
    {
        // If this is a link being added, then update the link-name in the
        // extant property.

        if (fLink)
        {
            // lpudpT points to the entry in our linked-list for this
            // property.  But it shouldn't already have a link-name (there
            // can only be one link-name per property).

            if (lpudpT->lpstzLink != NULL)
            {
                AssertSz (0, TEXT("Invalid property set - link name defined twice"));
                goto Exit;
            }

            // Since this is a link-name, it should be a string.

            if (lppropvar->vt != VT_LPTSTR)
            {
                AssertSz (0, TEXT("Invalid property set - link name isn't a string"));
                goto Exit;
            }

            // Point the UDPROP to the link name, and take ownership
            // of it by clearing the caller's pointer.

            Assert (lppropvar->pszVal != NULL);

            lpudpT->lpstzLink = (LPTSTR) lppropvar->pszVal;
            PropVariantInit (lppropvar);

            lpData->dwcLinks++;

        }   // if (fLink)

        // Otherwise, this isn't a link name, it's a value.  So point the
        // UDPROP to it's data.

        else
        {
            *lpudpT->lppropvar = *lppropvar;
            PropVariantInit (lppropvar);

        }   // if (fLink) ... else

    } // if ((lpudpT = LpudpropFindMatchingName ... else

    fSuccess = TRUE;

Exit:

    // Just in case we were given a name that we didn't
    // need, clear it now so that the caller knows that
    // on success, they needn't worry about the buffers
    // pointed to by lppropvar & lpstatpropstg.

    if (fSuccess)
    {
        if (lpstatpropstg->lpwstrName != NULL)
        {
            CoTaskMemFree (lpstatpropstg->lpwstrName);
            lpstatpropstg->lpwstrName = NULL;
        }
    }

    return(fSuccess);

} // FAddPropToList


////////////////////////////////////////////////////////////////////////////////
//
// AddNodeToList
//
// Purpose:
//  Adds the given node to the list.
//
////////////////////////////////////////////////////////////////////////////////

void PASCAL
AddNodeToList
  (LPUDOBJ lpUDObj,             // Pointer to object
   LPUDPROP lpudprop)           // Node to add
{
    // Put the new node at the end

    if (lpData->lpudpHead != NULL)
    {
        if (lpData->lpudpHead->llist.lpllistPrev != NULL)
        {
            ((LPUDPROP) lpData->lpudpHead->llist.lpllistPrev)->llist.lpllistNext = (LPLLIST) lpudprop;
            lpudprop->llist.lpllistPrev = lpData->lpudpHead->llist.lpllistPrev;
        }
        else
        {
            lpData->lpudpHead->llist.lpllistNext = (LPLLIST) lpudprop;
            lpudprop->llist.lpllistPrev = (LPLLIST) lpData->lpudpHead;
        }
        lpData->lpudpHead->llist.lpllistPrev = (LPLLIST) lpudprop;
    }
    else
    {
        lpData->lpudpHead = lpudprop;
        lpudprop->llist.lpllistPrev = NULL;
    }

    lpudprop->llist.lpllistNext = NULL;
    lpData->lpudpCache = lpudprop;

} // AddNodeToList


////////////////////////////////////////////////////////////////////////////////
//
// RemoveFromList
//
// Purpose:
//  Removes the given node from the list
//
////////////////////////////////////////////////////////////////////////////////
static void PASCAL
RemoveFromList
  (LPUDOBJ lpUDObj,                     // Pointer to object
   LPUDPROP lpudprop)                   // The node itself.
{
  AssertSz ((lpData->lpudpHead != NULL), TEXT("List is corrupt"));

    // If we're removing the cached node, invalidate the cache
  if (lpudprop == lpData->lpudpCache)
  {
    lpData->lpudpCache = NULL;
  }

    // Be sure the head gets updated, if the node is at the front
  if (lpudprop == lpData->lpudpHead)
  {
    lpData->lpudpHead = (LPUDPROP) lpudprop->llist.lpllistNext;

    if (lpData->lpudpHead != NULL)
    {
      lpData->lpudpHead->llist.lpllistPrev = lpudprop->llist.lpllistPrev;
    }
    return;
  }

    // Update the links
  if (lpudprop->llist.lpllistNext != NULL)
  {
    ((LPUDPROP) lpudprop->llist.lpllistNext)->llist.lpllistPrev = lpudprop->llist.lpllistPrev;
  }

  if (lpudprop->llist.lpllistPrev != NULL)
  {
    ((LPUDPROP) lpudprop->llist.lpllistPrev)->llist.lpllistNext = lpudprop->llist.lpllistNext;
  }

    // If it is the last node in the list, be sure the head is updated
  if (lpudprop == (LPUDPROP) lpData->lpudpHead->llist.lpllistPrev)
  {
    lpData->lpudpHead->llist.lpllistPrev = lpudprop->llist.lpllistPrev;
  }

} // RemoveFromList


////////////////////////////////////////////////////////////////////////////////
//
// DeallocValue
//
// Purpose:
//  Deallocates the value in the buffer.
//
////////////////////////////////////////////////////////////////////////////////
void PASCAL
DeallocValue
  (LPVOID *lplpvBuf,                    // Pointer to buffer to dealloc
   UDTYPES udtype)                      // Type stored in buffer
{
  DWORD cb=0;

  if (*lplpvBuf == NULL)
      return;

  switch (udtype)
  {
    case wUDdate  :
    case wUDfloat :
      Assert(sizeof(FILETIME) == sizeof(NUM));
                cb = sizeof(NUM);
                break;

    case wUDlpsz  :
      cb =  CBTSTR(*lplpvBuf);
                break;

         case   wUDbool:
         case   wUDdw:
                 AssertSz (((sizeof(LPVOID) >= sizeof(DWORD)) && (sizeof(LPVOID) >= sizeof(WORD))),
                TEXT("Sizes of basic types have changed!"));
                 return;

    default:
                Assert(fFalse);
                return;
  }


   VFreeMemP(*lplpvBuf,cb);
        *lplpvBuf = NULL;

} // DeallocValue


////////////////////////////////////////////////////////////////////////////////
//
//  VUdpropFree
//
//  Purpose:
//      Free a UDPROP (which is in a linked-list).
//
//  Inputs:
//      LPUDPROP * - A pointer-to-pointer-to a UDPROP object.
//
//  Output:
//      None.
//
////////////////////////////////////////////////////////////////////////////////
VOID
VUdpropFree
  (LPUDPROP *lplpudp)
{
    // Validate the inputs.

    if (lplpudp == NULL
        ||
        *lplpudp == NULL)
    {
        goto Exit;
    }

    // If this property has a name, free that buffer.

    if ((*lplpudp)->lpstzName)
    {
        CoTaskMemFree ((*lplpudp)->lpstzName);
    }

    // If this property has a link-name, free it too.

    if ((*lplpudp)->lpstzLink)
    {
        CoTaskMemFree ((*lplpudp)->lpstzLink);
    }

    // Clear the property value, which will free any associated
    // buffer.  Then free the PropVariant itself.

    PropVariantClear ((*lplpudp)->lppropvar);
    CoTaskMemFree ((*lplpudp)->lppropvar);
    CoTaskMemFree (*lplpudp);

    *lplpudp = NULL;

Exit:

    return;

} // VUdpropFree


////////////////////////////////////////////////////////////////////////////////
//
//  FUdpropUpdate
//
//  Purpose:
//      Updates the given node with the given data
//
//      It's the caller's responsibility to free lpudp if this function
//      fails.
//
//  Inputs:
//      LPUDPROP    - The node in the linked-list for this property.
//      LPUDOBJ     - All User-Defined data (including the properties)
//      LPTSTR      - The property name.
//      LPTSTR      - The link-name
//      LPVOID      - The new value
//      UDTYPES     - The type of the value.
//      BOOL        - TRUE if this is a link.
//
////////////////////////////////////////////////////////////////////////////////
static BOOL PASCAL
FUdpropUpdate
  (LPUDPROP lpudp,
   LPUDOBJ  lpUDObj,
   LPTSTR   lpszPropName,
   LPTSTR   lpszLinkMonik,
   LPVOID   lpvValue,
   UDTYPES  udtype,
   BOOL     fLink)
{
    //  ------
    //  Locals
    //  ------

    BOOL fSuccess = FALSE;

    //  -----
    //  Begin
    //  -----


    // Validate the inputs.

    if ((lpudp == NULL)  ||
        (lpszPropName == NULL) ||
        (lpvValue == NULL) ||
        (fLink && (lpszLinkMonik == NULL)) ||
        (!ISUDTYPE(udtype)))
    {
        goto Exit;
    }

    // Update the property name

    if (!FUdpropSetString (lpudp, lpszPropName, TRUE, TRUE))
        goto Exit;

    // If necessary, allocate a PropVariant for the UDPROPS

    if (lpudp->lppropvar == NULL)
    {
        lpudp->lppropvar = CoTaskMemAlloc (sizeof(PROPVARIANT));
        if (lpudp->lppropvar == NULL)
        {
            goto Exit;
        }
    }

    // Put the property value into the PropVariant

    PropVariantClear (lpudp->lppropvar);
    if (!FPropVarLoad (lpudp->lppropvar, (VARTYPE)udtype, lpvValue))
    {
        goto Exit;
    }

    // Update the link name if this is a link, otherwise
    // free any existing link name.

    if (fLink)
    {
        if(!FUdpropSetString (lpudp, lpszLinkMonik, FALSE, FALSE))
            goto Exit;
    }
    else
    {
        VUdpropFreeString (lpudp, FALSE);
        lpData->dwcLinks--;
    }

    //  ----
    //  Exit
    //  ----

    fSuccess = TRUE;

Exit:

    return(fSuccess);

} // FUdpropUpdate


////////////////////////////////////////////////////////////////////////////////
//
// FMakeTmpUDProps
//
// Purpose:
//  Create a temporary copy of the User-Defined property data
//
////////////////////////////////////////////////////////////////////////////////
BOOL
FMakeTmpUDProps
  (LPUDOBJ lpUDObj)                     // Pointer to object
{
    //  ------
    //  Locals
    //  ------

    BOOL     fSuccess = FALSE;

    LPUDPROP lpudpCur;
    LPUDPROP lpudpTmpCur;
    DWORD dw;
    LPVOID lpv;

    //  -----
    //  Begin
    //  -----

    // Validate the inputs.

    if ((lpUDObj == NULL) ||
        (lpData == NULL))
    {
        goto Exit;
    }

    FDeleteTmpUDProps (lpUDObj);

    // Move all the original list data to the tmp list
    lpData->dwcTmpLinks = lpData->dwcLinks;
    lpData->dwcTmpProps = lpData->dwcProps;
    lpData->lpudpTmpHead = lpData->lpudpHead;
    lpData->lpudpTmpCache = lpData->lpudpCache;

    // Reinitialize the object data
    lpData->dwcLinks = 0;
    lpData->dwcProps = 0;
    lpData->lpudpCache = NULL;
    lpudpTmpCur = lpData->lpudpHead = NULL;

    // Remember that we just put all the original data in the tmp ptrs.
    lpudpCur = lpData->lpudpTmpHead;

    // Loop through the old data and copy to the temp list

    while (lpudpCur != NULL)
    {
        // Create a new UDPROP

        lpudpTmpCur = LpudpropCreate();
        if (lpudpTmpCur == NULL)
            goto Exit;

        // Set the name in the UDPROP

        if (!FUdpropSetString (lpudpTmpCur, lpudpCur->lpstzName, FALSE, TRUE))
            goto Exit;

        // If we have a link-name, set it too in the UDPROP

        if (lpudpCur->lpstzLink != NULL)
        {
            if (!FUdpropSetString (lpudpTmpCur, lpudpCur->lpstzLink, FALSE, FALSE))
                goto Exit;
            lpData->dwcLinks++;
        }

        // Allocate a PropVariant to hold the property value.

        lpudpTmpCur->lppropvar = CoTaskMemAlloc (sizeof(PROPVARIANT));
        if (lpudpTmpCur->lppropvar == NULL)
        {
            goto Exit;
        }

        // Copy the PropVariant into the temporary UDPROP.

        PropVariantCopy (lpudpTmpCur->lppropvar, lpudpCur->lppropvar);

        // Also show if this is an invalid link or not.

        lpudpTmpCur->fLinkInvalid = lpudpCur->fLinkInvalid;

        // Add this new temporary UDPROP to the linked-list.

        AddNodeToList (lpUDObj, lpudpTmpCur);
        lpData->dwcProps++;

        // Move on to the next property.

        lpudpCur = (LPUDPROP) lpudpCur->llist.lpllistNext;

    }   // while (lpudpCur != NULL)


    //  ----
    //  Exit
    //  ----

    fSuccess = TRUE;

Exit:


    // If there was an error, put everything back and deallocate anything we created

    if (!fSuccess)
    {
        FSwapTmpUDProps (lpUDObj);
        FDeleteTmpUDProps (lpUDObj);
    }


    return fSuccess;

} // FMakeTmpUDProps


////////////////////////////////////////////////////////////////////////////////
//
// FSwapTmpUDProps
//
// Purpose:
//  Swap the "temp" copy with the real copy of User-Defined property data
//
////////////////////////////////////////////////////////////////////////////////
BOOL
FSwapTmpUDProps
  (LPUDOBJ lpUDObj)
{
    DWORD dwT;
    LPUDPROP lpudpT;

    if ((lpUDObj == NULL) ||
        (lpData == NULL))
        return FALSE;

    dwT = lpData->dwcLinks;
    lpData->dwcLinks = lpData->dwcTmpLinks;
    lpData->dwcTmpLinks = dwT;

    dwT = lpData->dwcProps;
    lpData->dwcProps = lpData->dwcTmpProps;
    lpData->dwcTmpProps = dwT;

    lpudpT = lpData->lpudpHead;
    lpData->lpudpHead = lpData->lpudpTmpHead;
    lpData->lpudpTmpHead = lpudpT;

    lpudpT = lpData->lpudpCache;
    lpData->lpudpCache = lpData->lpudpTmpCache;
    lpData->lpudpTmpCache = lpudpT;

    return TRUE;

} // FSwapTmpUDProps


////////////////////////////////////////////////////////////////////////////////
//
// FDeleteTmpUDProps
//
// Purpose:
//  Delete the "temp" copy of the data
//
////////////////////////////////////////////////////////////////////////////////
BOOL
FDeleteTmpUDProps
  (LPUDOBJ lpUDObj)
{
  if ((lpUDObj == NULL) ||
      (lpData == NULL))
    return FALSE;

  FreeUDData (lpUDObj, TRUE);

  return TRUE;

} // FDeleteTmpU


////////////////////////////////////////////////////////////////////////////////
//
//  FUdpropMakeVisible
//
//  Purpose:
//      Make a property in a UDPROP visibile by removing the
//      "_" that prepends the property name.
//
//  Inputs:
//      LPUDPROP - The UDPROP to convert.
//
//  Output:
//      TRUE if successful.
//
////////////////////////////////////////////////////////////////////////////////

static BOOL PASCAL
FUdpropMakeVisible (LPUDPROP lpudprop)
{
    //  ------
    //  Locals
    //  ------

    BOOL    fSuccess = FALSE;
    ULONG   cch;

    //  -----
    //  Begin
    //  -----

    Assert (lpudprop != NULL);

    // Is there anything to process?

    if (lpudprop->lpstzName == NULL)
        goto Exit;


    // Simply move the string up one character, thus overlaying
    // the prefix indicating a hidden property.  If it's already
    // a visible property we don't flag an error.

    if (*lpudprop->lpstzName == HIDDENPREFIX)
    {
        cch = CchTszLen (lpudprop->lpstzName) + 1;
        PbSzNCopy (lpudprop->lpstzName, &lpudprop->lpstzName[1], cch);
    }

    fSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    return (fSuccess);

}   // FUdpropMakeVisible


////////////////////////////////////////////////////////////////////////////////
//
//  FUdpropMakeHidden
//
//  Purpose:
//      Convert a property in a UDPROP so that it is a hidden
//      property.  Properties are considered hidden if the
//      first character in their name is an "_".
//
//  Inputs:
//      LPUDPROP - The UDPROP to convert.
//
//  Output:
//      TRUE if successful.
//
////////////////////////////////////////////////////////////////////////////////

static BOOL PASCAL
FUdpropMakeHidden (LPUDPROP lpudprop)
{
    //  ------
    //  Locals
    //  ------

    BOOL    fSuccess = FALSE;
    ULONG   cch;
    LPTSTR  lpstzOld;

    //  -----
    //  Begin
    //  -----

    // Intialize

    Assert (lpudprop != NULL);

    if (lpudprop->lpstzName == NULL)
        goto Exit;

    // Keep the old name.

    lpstzOld = lpudprop->lpstzName;

    // How many characters do we need in the new string?

    cch = CchTszLen (lpstzOld) + 2; // Includes the NULL & prefix

    // Allocate the memory.

    lpudprop->lpstzName = CoTaskMemAlloc (cch * sizeof(TCHAR));
    if (lpudprop->lpstzName == NULL)
        goto Exit;

    // Set the "_" prefix to indicate this is a hidden property.

    lpudprop->lpstzName[0] = HIDDENPREFIX;

    // Copy the original property name after the prefix in the UDPROP.

    PbSzNCopy (&lpudprop->lpstzName[1],
               lpstzOld,
               cch - 1); // One chacter less than cch to accout for hidden prefix.

    // Free the old buffer

    CoTaskMemFree (lpstzOld);

    //  ----
    //  Exit
    //  ----

    fSuccess = TRUE;

Exit:

    // If there was an error, ensure that the UDPROP is left as
    // we found it.

    if (!fSuccess)
    {
        if (lpstzOld != NULL)
        {
            if (lpudprop->lpstzName != NULL)
            {
                CoTaskMemFree (lpudprop->lpstzName);
            }
            lpudprop->lpstzName = lpstzOld;
        }
    }

    return (fSuccess);

}   // FUdpropMakeHidden


////////////////////////////////////////////////////////////////////////////////
//
//  FUdpropSetString
//
//  Purpose:
//      Set the name or link-name string in a UDPROP.
//      If the UDPROP already contains the string, free
//      it.
//
//  Inputs:
//      LPUDPROP    - A UDPROP (a property in the linked-list)
//      LPTSTR      - The new name or link-name
//      BOOL        - True => limit the length of the string to BUFMAX characters
//                            (including the NULL terminator)
//      BOOL        - True => set the (property) name, False => set the link-name
//
////////////////////////////////////////////////////////////////////////////////

static BOOL PASCAL
FUdpropSetString
    (LPUDPROP   lpudp,
     LPTSTR     lptstr,
     BOOL       fLimitLength,
     BOOL       fName)
{
    //  ------
    //  Locals
    //  ------

    BOOL    fSuccess = FALSE;   // Return value
    LPTSTR  lptstrNew = NULL;   // Pointed to be the UDPROP.
    ULONG   cch, cb;            

    //  ----------
    //  Initialize
    //  ----------

    // Validate the inputs.

    if (lpudp == NULL
        ||
        lptstr == NULL)
    {
        goto Exit;
    }

    //  ----------------
    //  Set the new name
    //  ----------------

    // Calculate the sizes.

    cch = CchTszLen(lptstr);
    if (fLimitLength && cch >= BUFMAX)
    {
        cch = BUFMAX - 1;
    }
    cb = (cch + 1) * sizeof(TCHAR); // Leave room for the NULL.

    // Allocate new memory.

    lptstrNew = CoTaskMemAlloc (cb);
    if (lptstrNew == NULL)
    {
        goto Exit;
    }

    // Copy the buffer (the buffer size is cch+1 including the NULL)
    // Also, terminate the target string, since it may be a truncation
    // of the source string.

    PbSzNCopy (lptstrNew, lptstr, cch+1);
    lptstrNew[cch] = TEXT('\0');

    // Put this new buffer in the UDPROP.

    if (fName)
        lpudp->lpstzName = lptstrNew;
    else
        lpudp->lpstzLink = lptstrNew;

    lptstrNew = NULL;

    //  ----
    //  Exit
    //  ----

    fSuccess = TRUE;

Exit:

    if (lptstrNew != NULL)
        CoTaskMemFree (lptstrNew);

    return (fSuccess);

}   // FUdpropSetString



////////////////////////////////////////////////////////////////////////////////
//
//  VUdpropFreeString
//
//  Purpose:
//      Free one of the two strings in a UDPROP - the
//      name string or the link-name string.  It is not
//      considered an error if either the UDPROP or the
//      string doesn't exist.
//
//  Inputs:
//      LPUDPROP    - The UDPROP containing the strings.
//      BOOL        - TRUE indicates we should free the
//                    name, FALSE indicates we should free
//                    the link name.
//
//  Output:
//      None.
//
////////////////////////////////////////////////////////////////////////////////

static void PASCAL
VUdpropFreeString
    (LPUDPROP   lpudp,
     BOOL       fName)
{

    // Is this really a UDPROP?

    if (lpudp != NULL)
    {
        // Should we delete the name?

        if (fName && lpudp->lpstzName)
        {
            CoTaskMemFree (lpudp->lpstzName);
            lpudp->lpstzName = NULL;
        }

        // Should we delete the link name?

        else if (!fName && lpudp->lpstzLink)
        {
            CoTaskMemFree (lpudp->lpstzLink);
            lpudp->lpstzLink = NULL;
        }

    }   // if (lpudp != NULL)

    return;

}   // VUdpropFreeString


/////////////////////////////////////////////////////////////////////////////////////
//
//  LpudpropCreate
//
//  Purpose:
//      Create a new UDPROP structure (an element of a linked-
//      list, and holds information about a single property).
//
//  Inputs:
//      None
//
//  Output:
//      A LPUDPROP if successful, NULL otherwise.
//
/////////////////////////////////////////////////////////////////////////////////////

LPUDPROP
LpudpropCreate ( void )
{
    // Create a buffer for the UDPROP

    LPUDPROP lpudp = CoTaskMemAlloc (sizeof(UDPROP));

    // Zero the buffer.

    if (lpudp != NULL)
    {
        FillBuf (lpudp, 0, sizeof(UDPROP));
    }

    return (lpudp);

}   // LpudpropCreate


////////////////////////////////////////////////////////////////////////////////
//
//  LppropvarUserDefGetPropVal
//
//  Purpose:
//      Return a PropVariant pointer for the requested 
//      property (requested by property name).
//
//  Inputs:
//      LPUDOBJ     - All UD data (including properties)
//      LPTSTR      - The name of the desired property
//      BOOL *      - True if this is a link.
//      BOOL *      - True if this link is invalid.
//
//  Output:
//      An LPPROPVARINT, or NULL if there was an error.
//
////////////////////////////////////////////////////////////////////////////////

DLLEXPORT LPPROPVARIANT
LppropvarUserDefGetPropVal
    (LPUDOBJ lpUDObj,             // Pointer to object
     LPTSTR lpszProp,             // Property string
     BOOL *pfLink,                // Indicates a link
     BOOL *pfLinkInvalid)         // Is the link invalid
{
    //  ------
    //  Locals
    //  ------

    LPUDPROP lpudprop;
    LPPROPVARIANT lppropvar;

    //  --------------
    //  Initialization
    //  --------------

    if ((lpUDObj == NULL)   ||
        (lpData == NULL)    ||
        (lpszProp == NULL))
    {
        return NULL;
    }

    //  ---------------------------------
    //  Find the node that has this name.
    //  ---------------------------------

    lpudprop = LpudpropFindMatchingName (lpUDObj, lpszProp);
    if (lpudprop == NULL)
        return NULL;

    // Is this a link?
    if (pfLink != NULL)
    {
        *pfLink = (lpudprop->lpstzLink != NULL);
    }

    // Is this an invalid link?  (In the Shell, all properties are
    // invalid).

    if (pfLinkInvalid != NULL)
    {
#ifdef SHELL
        *pfLinkInvalid = lpudprop->fLinkInvalid = TRUE;
#else
        *pfLinkInvalid = lpudprop->fLinkInvalid;
#endif
    }

    //  ----
    //  Exit
    //  ----

    return (lpudprop->lppropvar);

} // LppropvarUserDefGetPropVal


////////////////////////////////////////////////////////////////////////////////
//
//  LppropvarUserDefGetIteratorVal
//
//  Purpose:
//      Given an iterator value, get the property value.
//
//  Inputs:
//      LPUDITER    - The Iterator value.
//      BOOL *      - Set to True if this value is a link.
//      BOLL *      - Set to True if this value is invalid link.
//
//  Outputs:
//      LPPROPVARIANT of the property value.
//
////////////////////////////////////////////////////////////////////////////////

DLLEXPORT LPPROPVARIANT
LppropvarUserDefGetIteratorVal
  (LPUDITER lpUDIter,
   BOOL *pfLink,
   BOOL *pfLinkInvalid )
{
  // Validate the inputs

  if ((lpUDIter == NULL)  ||
      (lpUDIter->lpudp == NULL))
    return NULL;

  // Is this a Link?

  if (pfLink != NULL)
  {
    *pfLink = (lpUDIter->lpudp->lpstzLink != NULL);
  }

  // Is this an invalid link?

  if (pfLinkInvalid != NULL)
  {
    *pfLinkInvalid = lpUDIter->lpudp->fLinkInvalid;
  }

  // Return a pointer to the PropVariant

  return (lpUDIter->lpudp->lppropvar);

} // LpvoidUserDefGetIteratorVal
