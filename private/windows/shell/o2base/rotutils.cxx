//+---------------------------------------------------------------------
//
//  File:       rotutils.cxx
//
//  Contents:   Running Object Table helper functions
//
//----------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//+---------------------------------------------------------------
//
//  Function:   RegisterAsRunning
//
//  Synopsis:   Registers the object in the Running Object Table
//
//  Arguments:  [lpUnk] -- the object being registered
//              [lpmkFull] -- the full moniker to the object
//              [lpdwRegister] -- where the registration value will be
//                                  returned.
//
//  Notes:      c.f. RevokeAsRunning
//
//----------------------------------------------------------------

void
RegisterAsRunning(LPUNKNOWN lpUnk,
        LPMONIKER lpmkFull,
        DWORD FAR* lpdwRegister)
{
    LPRUNNINGOBJECTTABLE pROT;
    HRESULT r;

    if (OK(r = GetRunningObjectTable(0,(LPRUNNINGOBJECTTABLE FAR*)&pROT)))
    {
        // if already registered, revoke
        if (*lpdwRegister != NULL)
        {
            pROT->Revoke(*lpdwRegister);
            *lpdwRegister = NULL;
        }

        // register as running if a valid moniker is passed
        if (lpmkFull)
        {
            pROT->Register(NULL, lpUnk, lpmkFull, lpdwRegister);
        }

        pROT->Release();
    }
}


//+---------------------------------------------------------------
//
//  Function:   RevokeAsRunning
//
//  Synopsis:   Revokes an objects registration in the Running Object Table
//
//  Arguments:  [lpdwRegister] -- points to where the registration value is
//                                  for the object.  Will be set to NULL.
//
//  Notes:      c.f. RegisterAsRunning
//
//----------------------------------------------------------------

void
RevokeAsRunning(DWORD FAR* lpdwRegister)
{
    LPRUNNINGOBJECTTABLE pROT;
    HRESULT r;

    // if still registered, then revoke
    if (*lpdwRegister != NULL)
    {
        if (OK(r = GetRunningObjectTable(0,(LPRUNNINGOBJECTTABLE FAR*)&pROT)))
        {
            pROT->Revoke(*lpdwRegister);
            *lpdwRegister = NULL;
            pROT->Release();
        }
    }
}

