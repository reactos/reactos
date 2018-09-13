//+------------------------------------------------------------------------
//
//  File:       SAFETY.CXX
//
//  Contents:   Test safety options for embedded objects.
//
//  Notes:      Contains functions to determine safety of scripting and
//              initialization operations for embedded objects.
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_COMCAT_H_
#define X_COMCAT_H_
#include "comcat.h"
#endif

#ifndef X_OBJSAFE_H_
#define X_OBJSAFE_H_
#include "objsafe.h"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_SAFETY_HXX_
#define X_SAFETY_HXX_
#include "safety.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_CLSTAB_HXX_
#define X_CLSTAB_HXX_
#include "clstab.hxx"
#endif

DeclareTag(tagObjectSafety, "OleSite", "Object safety information");

EXTERN_C const CLSID CLSID_AppletOCX;
HRESULT GetSIDOfDispatch(IDispatch *pDisp, BYTE *pbSID, DWORD *pcbSID);
HRESULT GetCategoryManager(ICatInformation **ppCat);


//+-------------------------------------------------------------------------
//
//  Function:   DeinitCategoryInfo
//
//  Synopsis:   Clear out the thread local cached component category mgr.
//
//--------------------------------------------------------------------------

void 
DeinitCategoryInfo(THREADSTATE *pts)
{
    Assert(pts);
    ClearInterface(&pts->pCatInfo);
}


//+-------------------------------------------------------------------------
//
//  Function:   GetCategoryManager
//
//  Synopsis:   Retrieve thread local cached component category mgr.
//              or create a new one and put it in there if not. 
//
//--------------------------------------------------------------------------

HRESULT
GetCategoryManager(ICatInformation **ppCat)
{
    HRESULT hr = S_OK;

    Assert(GetThreadState());
    
    if (!TLS(pCatInfo))
    {
        ICatInformation *   pCatInfo = NULL;
        
        hr = THR(CoCreateInstance(
                CLSID_StdComponentCategoriesMgr, 
                NULL, 
                CLSCTX_INPROC_SERVER, 
                IID_ICatInformation, 
                (void **) &pCatInfo));
        if (hr) // couldn't get the category manager--
            goto Cleanup;

        Assert(pCatInfo);
        TLS(pCatInfo) = pCatInfo;   // Take over the ref of pCatInfo
    }

    *ppCat = TLS(pCatInfo);

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     COleSite::AccessAllowed
//
// Synopsis:    Return TRUE if it's ok to access the object model
//              of the dispatch passed in.
//
//---------------------------------------------------------------

BOOL
COleSite::AccessAllowed(IDispatch *pDisp)
{
    BOOL            fAllowed = FALSE;
    HRESULT         hr = S_OK;
    BYTE            abSID[MAX_SIZE_SECURITY_ID];
    DWORD           cbSID = ARRAY_SIZE(abSID);
    
    if (OK(THR_NOTRACE(GetSIDOfDispatch(pDisp, abSID, &cbSID))))
    {
        CDoc *  pDoc = Doc();

        hr = THR(pDoc->EnsureOmWindow());
        if (hr)
            goto Cleanup;

        fAllowed = cbSID == pDoc->_pOmWindow->_cbSID &&
                    !memcmp(abSID, pDoc->_pOmWindow->_pbSID, cbSID);
    }
    else
    {
        fAllowed = TRUE;
    }
    
Cleanup:
    return fAllowed;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::IsSafeToScript, COleSite::IsSafeToInitialize, 
//
//  Returns:    BOOL      TRUE if operation is safe, otherwise FALSE.
//
//----------------------------------------------------------------------------

BOOL
COleSite::IsSafeToScript()
{
    BOOL    fSafe;
    BOOL    fAllow = TRUE;
    INSTANTCLASSINFO * pici;
    
    if (_fKnowSafeToScript)
    {
        fSafe = !!_fSafeToScript;
    }
    else
    {
        HRESULT hr;
        CDoc *  pDoc = Doc();

        _fKnowSafeToScript = TRUE;
        
        hr = THR(pDoc->ProcessURLAction(
                URLACTION_ACTIVEX_OVERRIDE_SCRIPT_SAFETY, 
                &fSafe));
        if (hr)
            goto Cleanup;

        if (fSafe)
        {
            _fSafeToScript = !!fSafe;
            goto Cleanup;
        }
        
        if (!fSafe && OlesiteTag() == OSTAG_APPLET)
        {
            //
            // If we're an applet, determine if we're even allowed
            // to script to them.
            //

            hr = THR(pDoc->ProcessURLAction(
                    URLACTION_SCRIPT_JAVA_USE,
                    &fAllow));
            if (hr)
                goto Cleanup;
        }

        //
        // If not allowed to proceed further, don't bother with 
        // the IObjectSafety stuff, just return unsafe.
        //
        
        if (!fAllow)
        {
            fSafe = FALSE;
        }
        else
        {
            pici = GetInstantClassInfo();

            if (pici)
            {
                fSafe = IsSafeTo(
                            SAFETY_SCRIPT, 
                            IID_IDispatch, 
                            pici->clsid, 
                            _pUnkCtrl, 
                            pDoc);
            }
            else
            {
                fSafe = FALSE;
            }
        }

        _fSafeToScript = !!fSafe;

        if (!fSafe)
        {
            NotifyHaveProtectedUserFromUnsafeContent(pDoc, IDS_PROTECTEDFROMUNSAFEOCX);
        }
    }
    
Cleanup:
    return fSafe;
}    


BOOL
COleSite::IsSafeToInitialize(REFIID riid)
{
    BOOL    fSafe;
    HRESULT hr;
    CDoc *  pDoc = Doc();
    INSTANTCLASSINFO * pici;

#ifdef NO_SECURITY
    return TRUE;
#else

    hr = THR(pDoc->ProcessURLAction(
            URLACTION_ACTIVEX_OVERRIDE_DATA_SAFETY, 
            &fSafe));
    if (hr)
        goto Cleanup;

    if (!fSafe)
    {
        pici = GetInstantClassInfo();

        if (pici)
        {
            fSafe = IsSafeTo(
                        SAFETY_INIT, 
                        riid, 
                        pici->clsid, 
                        _pUnkCtrl, 
                        pDoc);
        }
    }

    if (!fSafe)
    {
        NotifyHaveProtectedUserFromUnsafeContent(pDoc, IDS_PROTECTEDFROMOCXINIT);
    }
    
Cleanup:
    return fSafe;

#endif // NO_SECURITY
}


//+---------------------------------------------------------------------------
//
//  Synopsis:   Brings up the unsafe content protection dialog.
//
//----------------------------------------------------------------------------

void 
NotifyHaveProtectedUserFromUnsafeContent(CDoc *pDoc, UINT uResId)
{
    // CHROME
    // Perform the notification even if we have no inplace HWND.
    if (!pDoc->_fSafetyInformed && 
        !(pDoc->_dwLoadf & DLCTL_SILENT) &&
        pDoc->_pInPlace &&
        (pDoc->_pInPlace->_hwnd || pDoc->IsChromeHosted()))
    {
        CDoc::CLock Lock(pDoc);

        pDoc->_fSafetyInformed = TRUE;
        IGNORE_HR(pDoc->ShowMessage(NULL, MB_OK | MB_ICONWARNING, 0, uResId));
    }
}


//+---------------------------------------------------------------------------
//
//  Synopsis:   Verifies safety for a particular action (on a particular 
//              interface).
//
//  Arguments:  sOperation:     The operation being validated for safety.
//              riid:           The interface this operation will use.
//
//  Returns:    BOOL            TRUE if operation is safe, otherwise FALSE.
//
//----------------------------------------------------------------------------

BOOL 
IsSafeTo(
    SAFETYOPERATION sOperation, 
    REFIID          riid, 
    CLSID           clsid, 
    IUnknown *      pUnk, 
    CDoc *          pDoc)       // if NULL, we don't confirm with user
{
    BOOL    fSafe = FALSE;
    HRESULT hr = E_FAIL;
    DWORD   dwCompat = 0;
    DWORD   dwMisc;
    CATID   catid = GUID_NULL;  // category of safety
    DWORD   dwXSetMask = 0; // options to set
    DWORD   dwXOptions = 0; // options to make safe for
                            // (either INTERFACESAFE_FOR_UNTRUSTED_CALLER or
                            // INTERFACESAFE_FOR_UNTRUSTED_DATA)

#if defined(WINCE) || defined(UNIX)
	// WINCEBUGBUG - temp overide any security on CE (BUGBUG)
	return TRUE;
#else

    IObjectSafety *     posafe = NULL;
    ICatInformation *   pCatInfo = NULL;

    // Only these three operations are handled here. That's why we
    // can get away with not initializing catid.
    AssertSz(sOperation==SAFETY_INIT ||
             sOperation==SAFETY_SCRIPT ||
             sOperation==SAFETY_SCRIPTENGINE, "Illegal operation param to IsSafeTo.");

    switch (sOperation)
    {
    case SAFETY_INIT:
        catid = CATID_SafeForInitializing;
        dwXSetMask = INTERFACESAFE_FOR_UNTRUSTED_DATA;
        dwXOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA;
        break;

    case SAFETY_SCRIPT:
        catid = CATID_SafeForScripting;
        dwXSetMask = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
        dwXOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
        break;
        
    case SAFETY_SCRIPTENGINE:
        catid = GUID_NULL;  // Registry check is not sufficient for script engines
        dwXOptions = dwXSetMask =   INTERFACESAFE_FOR_UNTRUSTED_DATA
                                  | INTERFACE_USES_DISPEX 
                                  | INTERFACE_USES_SECURITY_MANAGER;
    }

    // 
    // Check compat flags first.  If no object safety is allowed, go 
    // straight to confirm because we don't believe what this control
    // says about it's safety.  Effectively the control is lying to us,
    // or implemented IObjectSafety incorrectly.
    //

    hr = THR(CompatFlagsFromClsid(clsid, &dwCompat, &dwMisc));
    if (!OK(hr))
        goto Cleanup;

    if (dwCompat & COMPAT_EVIL_DONT_LOAD)
    {
        Assert (!fSafe);
        goto Cleanup;
    }
        
    if (dwCompat & COMPAT_NO_OBJECTSAFETY)
        goto Confirm;
        
    hr = pUnk->QueryInterface(IID_IObjectSafety, (void **) &posafe);

    // If IObjectSafety is supported, ask the object to make itself safe
    if (posafe)
    {
        //
        // If we're trying to make it safe to script, ask object
        // if it knows about dispex2 & sec mgr.  If not, then bail out
        // if it's a script engine, continue otherwise.
        //

        if (sOperation == SAFETY_SCRIPTENGINE)
        {
            DWORD   dwMask = 0;
            DWORD   dwEnabled;
            
            hr = THR(posafe->GetInterfaceSafetyOptions(
                    riid,
                    &dwMask,
                    &dwEnabled));
            if (hr || !(dwMask & INTERFACE_USES_DISPEX))
                goto Cleanup;
        }

        //
        // If we're going for safe for scripting, try making the object
        // safe on IDispatchEx first, then drop to IDispatch.
        //

        hr = E_FAIL;
        if (sOperation == SAFETY_SCRIPT)
        {
            hr = THR(posafe->
                    SetInterfaceSafetyOptions(IID_IDispatchEx, dwXSetMask, dwXOptions));
        }
        if (!OK(hr))
        {
            hr = THR(posafe->
                    SetInterfaceSafetyOptions(riid, dwXSetMask, dwXOptions));
        }
        
        if (!OK(hr))
        {
            // Give user an opportunity to override.
            goto Confirm;
        }

        // Don't check registry if object supports IObjectSafety.
        goto EnsureSafeForScripting;
    }

    // otherwise looking in the registry to see if the object
    // belongs to the appropriate component category

    hr = THR(GetCategoryManager(&pCatInfo));
    if (hr)
        goto Cleanup;
        
    CATID rgcatid[1];
    rgcatid[0] = catid;

    // Ask if the object belongs to the specified category
    hr = THR(pCatInfo->IsClassOfCategories(clsid, 1, rgcatid, 0, NULL));
    if (hr)
        goto Confirm;

    // Object is safe on this interface!

EnsureSafeForScripting:

    //
    // Though object appears to be safe, we still need to
    // see if scripting to objects is allowed at all.
    //

    if (sOperation == SAFETY_SCRIPT && pDoc)
    {
        IGNORE_HR(pDoc->ProcessURLAction(
            URLACTION_SCRIPT_SAFE_ACTIVEX,
            &fSafe));
    }
    else
    {
        fSafe = TRUE;
    }

    goto Cleanup;

Cleanup:
#if DBG==1
    if (!fSafe)
        TraceTag((tagObjectSafety, "Safety check failed!!"));
#endif

    ReleaseInterface(posafe);
    return fSafe;

Confirm:
    if (pDoc) {
        if (pDoc->IsPrintDoc())
        {
            fSafe = pDoc->TrustSecurityUI();
        }
        else
        {
            IGNORE_HR(pDoc->ProcessURLAction(
                URLACTION_ACTIVEX_CONFIRM_NOOBJECTSAFETY, 
                &fSafe));
        }
    }
    goto Cleanup;

#endif // WINCE
}
