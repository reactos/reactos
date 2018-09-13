#ifndef I_SAFETY_HXX_
#define I_SAFETY_HXX_
#pragma INCMSG("--- Beg 'safety.hxx'")

//+---------------------------------------------------------------------------
//
//  Enumeration:    SAFETYOPERATION
//
//  Synopsis:       Indicates which operation is being validated for safety
//
//----------------------------------------------------------------------------
enum SAFETYOPERATION
{
    SAFETY_INIT,
    SAFETY_SCRIPT,
    SAFETY_SCRIPTENGINE
};

// public API to this module

void NotifyHaveProtectedUserFromUnsafeContent(CDoc *pDoc, UINT uResId);
BOOL IsSafeTo(
    SAFETYOPERATION sOperation, 
    REFIID riid, 
    CLSID clsid, 
    IUnknown *pUnk, 
    CDoc *pDoc);

#pragma INCMSG("--- End 'safety.hxx'")
#else
#pragma INCMSG("*** Dup 'safety.hxx'")
#endif
