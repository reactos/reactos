//---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       olesitdb.cxx
//
//  Contents:   databinding functions for COleSite class
//
//  Classes:    COleSite
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include <binder.hxx>       // for cdatasourceprovider
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>     // for safetylevel in safety.hxx (via olesite.hxx)
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include <evntprm.hxx>      // for eventparam (needed by fire_ondata*)
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include <elemdb.hxx>       // for DBSPEC
#endif

#ifndef NO_DATABINDING
#ifndef X_VBCURSOR_VBDSC_H_
#define X_VBCURSOR_VBDSC_H_
#include <vbcursor/vbdsc.h> // for iid_ivbdsc
#endif

#ifndef X_SIMPDATA_H_
#define X_SIMPDATA_H_
#include <simpdata.h>
#endif

#ifndef X_MSDATSRC_H_
#define X_MSDATSRC_H_
#include <msdatsrc.h>
#endif

#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include <tearoff.hxx>
#endif

#endif // ndef NO_DATABINDING


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::GetInterfaceProperty (protected member)
//
//  Synopsis:   Get an interface-valued property of the control.
//
//----------------------------------------------------------------------------

HRESULT
COleSite::GetInterfaceProperty(UINT uGetOffset, DISPID dispid, REFIID riid,
                                IUnknown** ppunk)
{
    HRESULT hr;
    VARIANT var;

    // get the control's property
    hr = GetProperty(uGetOffset, dispid, VT_UNKNOWN, &var);
    if (hr || (var.vt != VT_UNKNOWN && var.vt != VT_DISPATCH) || var.punkVal==0)
    {
        if (hr)
            VariantInit(&var);      // control might mess with var before failing
        goto Cleanup;
    }

    // get the desired interface
    hr = var.punkVal->QueryInterface(riid, (void**)ppunk);

Cleanup:
    VariantClear(&var);
    return hr;
}


#define VALID_VTABLE_OFFSET(off) ((off) && (off)!=~0UL)

//+-------------------------------------------------------------------------
// Function:    Get Property (protected helper)
//
// Synopsis:    fetch a property value
//
// Arguments:   uVTableOffsetGet    VTable offset for Get method (or "invalid")
//              dispidGet           dispid for property
//              vtType              type of property
//              pvar                variant into which value is placed
//
// Returns:     HRESULT

HRESULT
COleSite::GetProperty(UINT uVTableOffsetGet, DISPID dispidGet, VARTYPE vtType,
                        VARIANT* pVar)
{
    Assert(pVar);
    HRESULT hr;

    VariantInit(pVar);

    if (VALID_VTABLE_OFFSET(uVTableOffsetGet) && IsVTableValid())
    {   // use the VTable binding to read from the control.
        hr = VTableDispatch(_pDisp, vtType, VTBL_PROPGET,
                            (vtType == VT_VARIANT) ?
                                (void *) pVar :
                                (void *) &pVar->iVal,
                            uVTableOffsetGet);
        if (!hr && vtType != VT_VARIANT)
        {
            pVar->vt = vtType;
        }
    }
    
    else
    {   // use Invoke to read from the control
        EXCEPINFO   except;

        // If the dispatch isn't valid then we can't even begin to bind.
        CacheDispatch();
        if (!_pDisp)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
        
        InitEXCEPINFO(&except);
        hr = THR(GetDispProp(_pDisp,
                             dispidGet,
                             g_lcidUserDefault,
                             pVar,
                             &except));
        FreeEXCEPINFO(&except);
    }

Cleanup:
    return hr;
}


//+-------------------------------------------------------------------------
// Function:    Set Property (public helper)
//
// Synopsis:    store a property value
//
// Arguments:   uVTableOffsetSet    VTable offset for Set method (or "invalid")
//              dispidSet           dispid for property
//              var                 variant into which value is placed
//
// Returns:     HRESULT

HRESULT
COleSite::SetProperty(UINT uVTableOffsetSet, DISPID dispidSet,
                     VARTYPE vtType, VARIANT *pVar)
{
    HRESULT hr;

    if (VALID_VTABLE_OFFSET(uVTableOffsetSet) && IsVTableValid())
    {   // use the VTable binding to write to the control.
        VARIANT     vCopy;
        VARIANT *   pVarPass = pVar;

        // Ensure the value in the variant (pVar) is the same type as the
        // property expects.
        if ((vtType != VT_VARIANT) && (vtType != pVarPass->vt))
        {
            vCopy.vt = VT_EMPTY;
            pVarPass = &vCopy;
            VariantChangeTypeEx(pVarPass, pVar, g_lcidUserDefault, 0, vtType);
        }

        // send the value to the control
        hr = VTableDispatch(_pDisp, vtType, VTBL_PROPSET,
                            (vtType == VT_VARIANT) ?
                                (void *) pVarPass :
                                (void *) &pVarPass->iVal,
                            uVTableOffsetSet);

         // free variant if we made a copy.
        if (pVarPass != pVar)
        {
            THR(VariantClear(pVarPass));
        }
    }
    
    else
    {   // use Invoke to read from the control
        EXCEPINFO   except;

        // If the dispatch isn't valid then we can't even begin to bind.
        CacheDispatch();
        if (!_pDisp)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
        
        InitEXCEPINFO(&except);
        hr = THR(SetDispProp(_pDisp,
                             dispidSet,
                             g_lcidUserDefault,
                             pVar,
                             &except));
        FreeEXCEPINFO(&except);
    }

Cleanup:
    return hr;
}


//+------------------------------------------------------------------------
//
//  Member:     COleSite::IsVTableValid
//
//  Synopsis:   If the control offers a default (dual) interface, replace
//              my _pDisp with a pointer to the default interface.
//
//  Returns:    TRUE        _pDisp can be used for vtable-style interface
//              FALSE       _pDisp cannot be used for vtable, only for IDispatch
//
//  Note:       This function loads the control's typeinfo.  Don't call it unless
//              you really plan to use vtable offsets.
//
//-------------------------------------------------------------------------

BOOL
COleSite::IsVTableValid()
{
    HRESULT     hr;
    CLASSINFO * pci = GetClassInfo();
    IDispatch * pDisp;

    if (pci == NULL)
        goto Cleanup;
    
    if (!_fVTableCached && _pUnkCtrl)
    {
        hr = QueryControlInterface(pci->iidDefault, (void **)&pDisp );
        if (SUCCEEDED(hr))
        {
            ReplaceInterface(&_pDisp, pDisp);
            ReleaseInterface(pDisp);
            _fDispatchCached = TRUE;
        }
        else
        {
            // If we can't actually get the default interface, then we shouldn't
            // consider this control to be DUAL interface anymore.
            // This is probably because we're dealing with a control that's not
            // threadsafe.
            pci->ClearFDualInterface();
        }
    }

    _fVTableCached = TRUE;

Cleanup:
    return pci ? pci->FDualInterface() : FALSE;
}


//+---------------------------------------------------------------
//
//  Member:     VTableDispatch
//
//  Synopsis:   Set/Get the control property of the current object
//              via dual interface VTable binding.
//
//  Arguments:  propType        - the VT_nnn property type -- this had better
//                                be the correct proptype for this control
//                                property.
//              propDirection   - set the property or get the property
//              pData           - pointer to Data of a type that better match
//                                the propType argument
//                                assumed to have inspected CLASSINFO).
//              uVTblOffset     - v-table offset from pDispatch in bytes
//
//  Returns:    S_OK                    everything is fine
//              E_xxxx                  other errors from the property function
//                                      being called
//

typedef HRESULT (STDMETHODCALLTYPE *OLEVTblFunc)(IDispatch *, void*);

HRESULT
COleSite::VTableDispatch (IDispatch *pDisp,
                            VARTYPE propType,
                            VTBL_PROP propDirection,
                            void *pvData,
                            unsigned int uVTblOffset)
{
    OLEVTblFunc             pVTbl;
    HRESULT                 hr = E_FAIL;

    if (pDisp==NULL)
        goto Cleanup;

    Assert((propDirection == VTBL_PROPSET) || (propDirection == VTBL_PROPGET));
    Assert(VALID_VTABLE_OFFSET(uVTblOffset));
    Assert(pvData);
    if (!pvData)                         // in case we proceed past asssertion.
        goto Cleanup;

    pVTbl = *(OLEVTblFunc *)(((BYTE *)(*(DWORD_PTR *)pDisp)) + uVTblOffset);

    if (propDirection == VTBL_PROPSET)
    {
        AssertSz(!(propType & ~VT_TYPEMASK) ||
                    ((propType & VT_BYREF) && (VT_VARIANT == (propType & VT_TYPEMASK))),
                "Unsupported by-ref property put - base type isn't VARIANT");

        switch (propType & VT_TYPEMASK)
        {
            case VT_I2:
                hr = (* (HRESULT (STDMETHODCALLTYPE *)(IDispatch*, short)) pVTbl)
                    (pDisp, *(short*)pvData);
                break;
            case VT_I4:
                hr = (* (HRESULT (STDMETHODCALLTYPE *)(IDispatch*, long)) pVTbl)
                    (pDisp, *(long *)pvData);
                break;
            case VT_R4:
                hr = (* (HRESULT (STDMETHODCALLTYPE *)(IDispatch*, float)) pVTbl)
                    (pDisp, *(float *)pvData);
                break;
            case VT_R8:
                hr = (* (HRESULT (STDMETHODCALLTYPE *)(IDispatch*, double)) pVTbl)
                    (pDisp, *(double *)pvData);
                break;
            case VT_CY:
                hr = (* (HRESULT (STDMETHODCALLTYPE *)(IDispatch*, CY)) pVTbl)
                    (pDisp, *(CY *)pvData);
                break;
            case VT_DATE:
                hr = (* (HRESULT (STDMETHODCALLTYPE *)(IDispatch*, DATE)) pVTbl)
                    (pDisp, *(DATE *)pvData);
                break;
            case VT_BSTR:
                hr = (* (HRESULT (STDMETHODCALLTYPE *)(IDispatch*, BSTR)) pVTbl)
                    (pDisp, *(BSTR *)pvData);
                break;
            case VT_BOOL:
                hr = (* (HRESULT (STDMETHODCALLTYPE *)(IDispatch*, boolean)) pVTbl)
                    (pDisp, *(boolean *)pvData);
                break;
            case VT_VARIANT:
                if (propType & VT_BYREF)
                {
                    hr = (* (HRESULT (STDMETHODCALLTYPE *)(IDispatch*, VARIANT*)) pVTbl)
                        (pDisp, (VARIANT *)pvData);
                }
                else
                {
                    hr = (* (HRESULT (STDMETHODCALLTYPE *)(IDispatch*, VARIANT)) pVTbl)
                        (pDisp, *(VARIANT *)pvData);
                }
                break;
            case VT_UNKNOWN:
                hr = (* (HRESULT (STDMETHODCALLTYPE *)(IDispatch*, IUnknown*)) pVTbl)
                    (pDisp, *(IUnknown **)pvData);
                break;
            case VT_DISPATCH:
                hr = (* (HRESULT (STDMETHODCALLTYPE *)(IDispatch*, IDispatch*)) pVTbl)
                    (pDisp, *(IDispatch **)pvData);
                break;
            case VT_ERROR:
            case VT_EMPTY:
            case VT_NULL:
            default:
                Assert(!"Unsupported dual interface type.");
                hr = E_FAIL;
                break;
        }
    }
    else    // propDirection == VTBL_PROPGET
    {
        switch (propType & VT_TYPEMASK)
        {
        default:
            hr = (*pVTbl) (pDisp, pvData);
            break;
            
        // Following types not supported.
        case VT_ERROR:
        case VT_EMPTY:
        case VT_NULL:
            hr = E_FAIL;
            break;
        }
    }

Cleanup:
    RRETURN(hr);
}
        

        
