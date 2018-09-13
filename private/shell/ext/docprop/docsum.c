////////////////////////////////////////////////////////////////////////////////
//
// DocSum.c
//
// Notes:
//  To make this file useful for OLE objects, define OLE_PROPS.
//
//  The macro lpDocObj must be used for all methods to access the
//  object data to ensure that this will compile with OLE_PROPS defined.
//
// Change history:
//
// Date         Who             What
// --------------------------------------------------------------------------
// 06/01/94     B. Wentz        Created file
// 06/25/94     B. Wentz        Converted to lean & mean API's
// 07/26/94     B. Wentz        Added code to merge DocumentSummary and UserDefined streams.
// 08/03/94     B. Wentz        Added Manager & Company Properties
//
////////////////////////////////////////////////////////////////////////////////

#include "priv.h"
#pragma hdrstop

#ifndef WINNT

#include <windows.h>
#include "offglue.h"
#include <objbase.h>
#include <objerror.h>

#include "proptype.h"
#include "internal.h"
#include "propmisc.h"
#include "debug.h"

#endif

#ifndef _WIN2000_DOCPROP_

  // Internal prototypes
static void PASCAL FreeData (LPDSIOBJ lpDSIObj);
static BOOL FFreeHeadPart(LPPLXHEADPART lpplxheadpart, SHORT iPlex);
static SHORT ILookupHeading(LPPLXHEADPART lpplxheadpart, LPTSTR lpsz);


  // Do nothing for non-OLE code....
#define lpDocObj  lpDSIObj
#define lpData ((LPDSINFO) lpDSIObj->m_lpData)


////////////////////////////////////////////////////////////////////////////////
//
// OfficeDirtyDSIObj
//
// Purpose:
//  Sets object state to dirty or clean.
//
////////////////////////////////////////////////////////////////////////////////
 VOID OfficeDirtyDSIObj
  (LPDSIOBJ lpDSIObj,           // The object
   BOOL fDirty)                 // Flag indicating if the object is dirty.
{

  Assert(lpDSIObj != NULL);
  lpDocObj->m_fObjChanged = fDirty;

} // OfficeDirtyDSIObj


////////////////////////////////////////////////////////////////////////////////
//
// FDocSumCreate
//
// Purpose:
//  Create the object and return it.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
FDocSumCreate
  (LPDSIOBJ FAR *lplpDSIObj,            // Pointer to pointer to object
   void *prglpfn[])                     // Pointer to function array
{
  LPDSIOBJ lpDSIObj;  // Hack - a temp, must call it "lpDSIObj" for macros to work!

  if (lplpDSIObj == NULL)
          return(TRUE);

    // Make sure we get valid args before we start alloc'ing

  if ((prglpfn == NULL) || (prglpfn[ifnCPConvert] == NULL))
    return FALSE;

  if ((*lplpDSIObj = (LPDSIOBJ) PvMemAlloc(sizeof (DOCSUMINFO))) == NULL)
  {
// REVIEW: Add alert
    return FALSE;
  }

  lpDSIObj = *lplpDSIObj; // Save us some indirecting & let us use the "LP" macros

    // If alloc fails, free the original object too.
  if ((lpData =
       PvMemAlloc(sizeof (DSINFO))) == NULL)
  {
// REVIEW: Add alert
    VFreeMemP(*lplpDSIObj, sizeof(DOCSUMINFO));
    return FALSE;
  }

  FillBuf ((void *) lpData, (int) 0, (sizeof (DSINFO) - ifnDSIMax*(sizeof (void *))));

    // Save the fnc's for Code Page conversions
  lpData->lpfnFCPConvert = (BOOL (*)(LPTSTR, DWORD, DWORD, BOOL)) prglpfn[ifnCPConvert];

  OfficeDirtyDSIObj (*lplpDSIObj, FALSE);
  (*lplpDSIObj)->m_hPage = NULL;

  return TRUE;

} // FDocSumCreate


////////////////////////////////////////////////////////////////////////////////
//
// FDocSumDestroy
//
// Purpose:
//  Destroy the given object.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
FDocSumDestroy
  (LPDSIOBJ FAR *lplpDSIObj)            // Pointer to pointer to object
{
  if ((lplpDSIObj == NULL)    ||
      (*lplpDSIObj == NULL))
    return TRUE;

  if (((LPDSIOBJ) *lplpDSIObj)->m_lpData != NULL)
  {
    // Free data in the DSINFO structure.
    FreeData (*lplpDSIObj);

    // Invalidate any OLE Automation DocumentProperty objects we might have
    InvalidateVBAObjects(NULL, *lplpDSIObj, NULL);

    // Free the DSINFO buffer itself.
    VFreeMemP((*lplpDSIObj)->m_lpData, sizeof(DSINFO));
  }

  // Free the DOCSUMINFO structure.
  VFreeMemP(*lplpDSIObj, sizeof(DOCSUMINFO));
  *lplpDSIObj = NULL;
  return TRUE;

} // FDocSumDestroy


////////////////////////////////////////////////////////////////////////////////
//
// FreeData
//
// Purpose:
//  Deallocates all the member data for the object
//
// Note:
//  Assumes object is valid.
//
////////////////////////////////////////////////////////////////////////////////
static void PASCAL
FreeData
  (LPDSIOBJ lpDSIObj)                   // Pointer to valid object
{

    // Free any buffers held by the PropVariants.

    FreePropVariantArray (NUM_DSI_PROPERTIES, GETDSINFO(lpDSIObj)->rgpropvar);

} // FreeData


////////////////////////////////////////////////////////////////////////////////
//
// FDocSumClear
//
// Purpose:
//  Clear the data stored in the object, but do not destroy the object
//
////////////////////////////////////////////////////////////////////////////////
BOOL
FDocSumClear
  (LPDSIOBJ lpDSIObj)                   // Pointer to object
{
  if ((lpDocObj == NULL) ||
      (lpData == NULL))
    return TRUE;

  FreeData (lpDocObj);

  // Invalidate any OLE Automation DocumentProperty objects we might have
  InvalidateVBAObjects(NULL, lpDSIObj, NULL);

  // Clear the data, don't blt over the fn's stored at the end.
  FillBuf ((void *) lpData, (int) 0, (sizeof (DSINFO) - ifnDSIMax*(sizeof (void *))));

  OfficeDirtyDSIObj (lpDSIObj, TRUE);
  return TRUE;

} // FDocSumClear


////////////////////////////////////////////////////////////////////////////////
//
// FDocSumShouldSave
//
// Purpose:
//  Indicates if the data has changed, meaning a write is needed.
//
////////////////////////////////////////////////////////////////////////////////
 BOOL
FDocSumShouldSave
  (LPDSIOBJ lpDSIObj)                   // Pointer to object
{
  if ((lpDocObj == NULL) ||
      (lpData == NULL))
    return FALSE;

  return lpDocObj->m_fObjChanged;

} // FDocSumShouldSave


////////////////////////////////////////////////////////////////////////////////
//
// FLinkValsChanged
//
// Purpose:
//              Determine if the link values changed
//
////////////////////////////////////////////////////////////////////////////////
 BOOL
FLinkValsChanged
  (LPDSIOBJ lpDSIObj)                   // Pointer to object
{
  BOOL f = FALSE;

  if ((lpDocObj == NULL) ||
      (lpData == NULL))
    return FALSE;

  // If the PID_LINKSDIRTY property is extant, valid, & true,
  // then return TRUE and clear the property.

  if (GETDSINFO(lpDSIObj)->rgpropvar[ PID_LINKSDIRTY ].vt == VT_BOOL
      &&
      GETDSINFO(lpDSIObj)->rgpropvar[ PID_LINKSDIRTY ].boolVal)
  {
      f = TRUE;
      GETDSINFO(lpDSIObj)->rgpropvar[ PID_LINKSDIRTY ].boolVal = VARIANT_FALSE;
  }


  return(f);

} // FLinkValsChanged

#endif //_WIN2000_DOCPROP_