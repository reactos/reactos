////////////////////////////////////////////////////////////////////////////////
//
// SInfo.c
//
// Summary Information API implementation
//
// Notes:
//  To make this file useful for OLE objects, define OLE_PROPS.
//
//  The macro lpDocObj must be used for all methods to access the
//  object data to ensure that this will compile with OLE_PROPS defined.
//  The macro lpData must also be used for all access to the m_lpData
//  member of the object.  These macros only work when the LPSIOBJ
//  parameter is named lpSIObj!
//
//  All strings stored in objects are in the format described in proptype.h
//
// Change history:
//
// Date         Who             What
// --------------------------------------------------------------------------
// 06/03/94     B. Wentz        Created file
// 06/08/94     B. Wentz        Updated to new string format
// 06/25/94     B. Wentz        Updated to lean & mean API
// 07/20/94             M. Jansson              Updated include statemes, due to changes in PDK
//
////////////////////////////////////////////////////////////////////////////////

#include "priv.h"
#pragma hdrstop
#include "reg.h"

#ifndef _WIN2000_DOCPROP_

 // Internal prototypes
void PASCAL FreeData (LPSIOBJ lpSIObj);

  // Do nothing for non-OLE code....
#define lpDocObj  lpSIObj
#define lpData  ((LPSINFO) lpSIObj->m_lpData)


////////////////////////////////////////////////////////////////////////////////
//
// OfficeDirtySIObj
//
// Purpose:
//  Sets object state to dirty or clean.
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT VOID OfficeDirtySIObj (
   LPSIOBJ lpSIObj,             // The object
   BOOL fDirty)                 // Flag indicating if the object is dirty.
{
    Assert(lpSIObj != NULL);
    lpDocObj->m_fObjChanged = fDirty;

} // OfficeDirtySIObj


////////////////////////////////////////////////////////////////////////////////
//
// FSumInfoCreate
//
// Purpose:
//   Create the object and return it.  Caller responsible for destruction.
//
////////////////////////////////////////////////////////////////////////////////
BOOL FSumInfoCreate (
   LPSIOBJ FAR *lplpSIObj,           // Pointer to object
   const void *prglpfn[])            // Pointer to functions
{
    LPSIOBJ lpSIObj;  // Hack - a temp, must call it "lpSIObj" for macros to work!
    DWORD         cb;
    TCHAR  szValue[10];

    if (lplpSIObj == NULL)
        return(TRUE);

    // Make sure we get valid args before we start alloc'ing
    if ((prglpfn == NULL) || (prglpfn[ifnCPConvert] == NULL) ||
        ((prglpfn[ifnFSzToNum] == NULL) && (prglpfn[ifnFNumToSz] != NULL)) ||
        ((prglpfn[ifnFSzToNum] != NULL) && (prglpfn[ifnFNumToSz] == NULL)))
        return FALSE;

    if ((*lplpSIObj = (LPSIOBJ) PvMemAlloc(sizeof (OFFICESUMINFO))) == NULL)
    {
// REVIEW: Add alert
        return FALSE;
    }

    lpSIObj = *lplpSIObj; // Save us some indirecting & let us use the "LP" macros

    // If alloc fails, free the original object too.
    if ((lpData = (LPSINFO) PvMemAlloc(sizeof (SINFO))) == NULL)
    {
// REVIEW: Add alert
        VFreeMemP(*lplpSIObj, sizeof(OFFICESUMINFO));
        return FALSE;
    }

    FillBuf ((void *) lpData, (int) 0, (sizeof (SINFO) - ifnSIMax*(sizeof (void *))));

  // Save the fnc for code page conversion, SzToNum, NumToSz
    lpData->lpfnFCPConvert = (BOOL (*)(LPTSTR, DWORD, DWORD, BOOL)) prglpfn[ifnCPConvert];
    lpData->lpfnFSzToNum = (BOOL (*)(NUM *, LPTSTR)) prglpfn[ifnFSzToNum];
    lpData->lpfnFNumToSz = (BOOL (*)(NUM *, LPTSTR, DWORD)) prglpfn[ifnFNumToSz];
    lpData->lpfnFUpdateStats = (BOOL (*)(HWND, LPSIOBJ, LPDSIOBJ)) prglpfn[ifnFUpdateStats];

  // Check the registry to see if we should disable Total Editing tracking
    cb = sizeof(szValue);
    if (RegQueryValue(HKEY_CURRENT_USER, vcszNoTracking,
                      (LPTSTR)&szValue, &cb) == ERROR_SUCCESS
        &&  cb < sizeof(szValue))
        lpData->fNoTimeTracking = (lstrcmpi(szValue,TEXT("0")) != 0); // lstrcmpi returns 0 if equal

    OfficeDirtySIObj (*lplpSIObj, FALSE);
    (*lplpSIObj)->m_hPage = NULL;

    return TRUE;

} // FSumInfoCreate


//////////////////////////////////////////////////////////////////////////////
//
// FreeData
//
// Purpose:
//  Deallocates all the member data for the object
//
// Note:
//  Assumes object is valid.
//
//////////////////////////////////////////////////////////////////////////////
void PASCAL FreeData (
  LPSIOBJ lpSIObj)                     // Pointer to valid object
{
 
    // Free any buffers held by PropVariants.

    FreePropVariantArray (NUM_SI_PROPERTIES, GETSINFO(lpSIObj)->rgpropvar);
    
}    
    
    
    
////////////////////////////////////////////////////////////////////////////////
//
// FSumInfoClear
//
// Purpose:
//   Clear the data stored in the object, but do not destroy the object.
//
////////////////////////////////////////////////////////////////////////////////
BOOL FSumInfoClear (
  LPSIOBJ lpSIObj)                     // Pointer to object
{
    BOOL fNoTimeTracking;

    if ((lpDocObj == NULL) ||
        (lpData == NULL))
        return TRUE;

    // Free data in the SINFO structure.
    FreeData (lpDocObj);

    // Invalidate any OLE Automation DocumentProperty objects we might have
    InvalidateVBAObjects(lpSIObj, NULL, NULL);

    // Clear the data, don't blt over the fn's stored at the end.
    fNoTimeTracking = lpData->fNoTimeTracking;
    FillBuf ((void *) lpData, (int) 0, (sizeof (SINFO) - ifnSIMax*(sizeof (void *))));
    lpData->fNoTimeTracking = fNoTimeTracking;

    OfficeDirtySIObj (lpSIObj, TRUE);
    return TRUE;

} // FSumInfoClear


////////////////////////////////////////////////////////////////////////////////
//
// FSumInfoDestroy
//
// Purpose:
//   Destroy the object
//
////////////////////////////////////////////////////////////////////////////////
BOOL FSumInfoDestroy (LPSIOBJ *lplpSIObj)              // Pointer to pointer to object
{
    if ((lplpSIObj == NULL)    ||
        (*lplpSIObj == NULL))
        return TRUE;

    if ((*lplpSIObj)->m_lpData != NULL)
    {
        // Free data held by the SINFO structure.
        FreeData (*lplpSIObj);

        // Invalidate any OLE Automation DocumentProperty objects we might have
        InvalidateVBAObjects(*lplpSIObj, NULL, NULL);

        // Free the SINFO structure itself.
        VFreeMemP((*lplpSIObj)->m_lpData, sizeof(SINFO));
    }

    // Free the OFFICESUMINFO buffer.
    VFreeMemP(*lplpSIObj, sizeof(OFFICESUMINFO));

    *lplpSIObj=NULL;
    return TRUE;

} // FSumInfoDestroy


////////////////////////////////////////////////////////////////////////////////
//
// FSumInfoShouldSave
//
// Purpose:
//  Indicates if the data has changed, meaning a write is needed.
//
////////////////////////////////////////////////////////////////////////////////
DLLEXPORT BOOL FSumInfoShouldSave (LPSIOBJ lpSIObj)                     // Pointer to object
{
    if (lpDocObj == NULL)
        return FALSE;

    return lpDocObj->m_fObjChanged;

} // FSumInfoShouldSave


//
// VSumInfoSetPropBit
//
// Set the bit that indicates that a filetime has been set/loaded
//
VOID PASCAL VSumInfoSetPropBit(LONG pid, BYTE *pbPropSet)
{
    switch (pid)
    {
    case PID_EDITTIME:
        *pbPropSet |= bEditTime;
        break;
    case PID_LASTPRINTED:
        *pbPropSet |= bLastPrint;
        break;
    case PID_CREATE_DTM:
        *pbPropSet |= bCreated;
        break;
    case PID_LASTSAVE_DTM:
        *pbPropSet |= bLastSave;
        break;
    case PID_PAGECOUNT:
        *pbPropSet |= bPageCount;
        break;
    case PID_WORDCOUNT:
        *pbPropSet |= bWordCount;
        break;
    case PID_CHARCOUNT:
        *pbPropSet |= bCharCount;
        break;
    case PID_DOC_SECURITY:
        *pbPropSet |= bSecurity;
        break;
#ifdef DEBUG
    default:
        Assert(FALSE);
        break;
#endif
    }
}

//
// FSumInfoPropBitIsSet
//
// Check the bit that indicates that a filetime has been set/loaded
//
BOOL PASCAL FSumInfoPropBitIsSet(LONG pid, BYTE bPropSet)
{
    switch (pid)
    {
    case PID_EDITTIME:
        return (bPropSet & bEditTime);
        break;
    case PID_LASTPRINTED:
        return(bPropSet & bLastPrint);
        break;
    case PID_CREATE_DTM:
        return(bPropSet & bCreated);
        break;
    case PID_LASTSAVE_DTM:
        return(bPropSet & bLastSave);
        break;
    case PID_PAGECOUNT:
        return(bPropSet & bPageCount);
        break;
    case PID_WORDCOUNT:
        return(bPropSet & bWordCount);
        break;
    case PID_CHARCOUNT:
        return(bPropSet & bCharCount);
        break;
    case PID_DOC_SECURITY:
        return(bPropSet & bSecurity);
        break;
    default:
#ifdef DEBUG
        Assert(FALSE);
#endif
        return(FALSE);
        break;
    }
}

#endif // _WIN2000_DOCPROP_ 
