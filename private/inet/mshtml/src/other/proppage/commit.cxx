//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Micros oft Corporation, 1996
//
//  File:       commit.cxx
//
//  Contents:   Implementation of the proppage commit engine
//
//  History:    07-05-96  AnandRa   Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_COMMIT_HXX_
#define X_COMMIT_HXX_
#include "commit.hxx"
#endif

#ifndef X_PROPUTIL_HXX_
#define X_PROPUTIL_HXX_
#include "proputil.hxx"
#endif

MtDefine(EnsureCommitHolder_paryHolder, Dialogs, "EnsureCommitHolder paryHolder")
MtDefine(EnsureCommitHolder_paryHolder_pv, Dialogs, "EnsureCommitHolder paryHolder::_pv")
MtDefine(CCommitHolder, Dialogs, "CCommitHolder")
MtDefine(CCommitHolder_aryEngine_pv, CCommitHolder, "CCommitHolder::_aryEngine::_pv")
MtDefine(CCommitEngine, Dialogs, "CCommitEngine")
MtDefine(CCommitEngine_aryObjs_pv, CCommitEngine, "CCommitEngine::_aryObjs::_pv")
MtDefine(CCommitEngine_aryDPD_pv, CCommitEngine, "CCommitEngine::_aryDPD::_pv")
MtDefine(CCommitEngine_dpdBool_pAryEVAL, CCommitEngine, "CCommitEngine::_dpdBool.pAryEVAL")
MtDefine(CCommitEngine_dpdBool_pAryEVAL_pv, CCommitEngine, "CCommitEngine::_dpdBool.pAryEVAL_pv")
MtDefine(CCommitEngine_dpdColor_pAryEVAL, CCommitEngine, "CCommitEngine::_dpdColor.pAryEVAL")
MtDefine(CCommitEngine_dpdColor_pAryEVAL_pv, CCommitEngine, "CCommitEngine::_dpdColor.pAryEVAL_pv")
MtDefine(CCommitEngineParseUserDefined_pDPD_pAryEVAL, Dialogs, "CCommitEngine::ParseUserDefined pDPD->pAryEVAL")
MtDefine(CCommitEngineParseUserDefined_pDPD_pAryEVAL_pv, Dialogs, "CCommitEngine::ParseUserDefined pDPD->pAryEVAL::_pv")

void    
DeinitCommitHolder(THREADSTATE *pts)
{
    if (pts->prop.paryCommitHolder)
    {
        delete pts->prop.paryCommitHolder;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     EnsureCommitHolder
//
//  Synopsis:   Creator of commit holder
//
//  Arguments:  dwID    Some identification of the object
//                      requesting the ensure.  This id is the same
//                      across all objects wanting to link to the 
//                      same commit holder.
//
//  Notes:      Property pages that want to sync contents need to call
//              this function very early on.  This is because some frames
//              may delay calling functions such as SetObjects and
//              Activate until late.  Typically this is done as soon
//              as the pagesite is set on the proppage.
//
//-------------------------------------------------------------------------

HRESULT 
EnsureCommitHolder(DWORD_PTR dwID, CCommitHolder **ppHolder)
{
    HRESULT                     hr = S_OK;
    CPtrAry<CCommitHolder *> *  paryHolder = TLS(prop.paryCommitHolder);
    CCommitHolder *             pHolderNew = NULL;
    long                        i;

    if (!paryHolder)
    {
        paryHolder = new(Mt(EnsureCommitHolder_paryHolder)) CPtrAry<CCommitHolder *>(Mt(EnsureCommitHolder_paryHolder_pv));
        if (!paryHolder)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        TLS(prop.paryCommitHolder) = paryHolder;
    }
    
    Assert(paryHolder);

    //
    // Check the global holders first and see if the id's match.
    // If so, then we can bail out right here.
    //

    for (i = 0; i < paryHolder->Size(); i++)
    {
        if ((*paryHolder)[i]->_dwID == dwID)
        {
            (*paryHolder)[i]->AddRef();
            *ppHolder = (*paryHolder)[i];
            goto Cleanup;
        }
    }

    //
    // If we're here then either there is no holder in global memory or
    // there is a global holder but someone wants to 
    // create another holder.  Either way, we can just forget about
    // the existing global commit holder.
    //
    
    pHolderNew = new CCommitHolder(dwID);
    if (!pHolderNew)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    hr = THR(paryHolder->Append(pHolderNew));
    if (hr)
        goto Error;
        
    *ppHolder = pHolderNew;
    
Cleanup:
    RRETURN(hr);

Error:
    delete pHolderNew;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     CCommitHolder::CCommitHolder
//
//  Synopsis:   ctor
//
//-------------------------------------------------------------------------

CCommitHolder::CCommitHolder(DWORD_PTR dwID)
    : _aryEngine(Mt(CCommitHolder_aryEngine_pv))
{
    _aryEngine.SetSize(0);
    _ulRefs = 1;
    _dwID = dwID;
}


//+------------------------------------------------------------------------
//
//  Member:     CCommitHolder::~CCommitHolder
//
//  Synopsis:   dtor
//
//-------------------------------------------------------------------------

CCommitHolder::~CCommitHolder()
{
    CPtrAry<CCommitHolder *> *  paryHolder = TLS(prop.paryCommitHolder);
    long                        i;
    
    Assert(paryHolder);

    for (i = 0; i < _aryEngine.Size(); i++)
    {
        delete _aryEngine[i];
    }
    
    _aryEngine.DeleteAll();
    paryHolder->DeleteByValue(this);
}


//+---------------------------------------------------------------------------
//
//  Member:     CCommitHolder::AddRef
//
//  Synopsis:   Ref counting
//
//----------------------------------------------------------------------------

ULONG
CCommitHolder::AddRef()
{
    return ++_ulRefs;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCommitHolder::Release
//
//  Synopsis:   Ref counting
//
//----------------------------------------------------------------------------

void
CCommitHolder::Release()
{
    if (--_ulRefs == 0)
    {
        delete this;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CCommitHolder::GetEngine
//
//  Synopsis:   Get a commit engine based on objects being set.
//
//  Arguments:  cObj        Count of objects
//              ppDisp      Array of dispatch ptrs
//              ppEngine    Engine that is returned.
//
//-------------------------------------------------------------------------

HRESULT
CCommitHolder::GetEngine(
    long cObj, 
    IDispatch **ppDisps, 
    CCommitEngine **ppEngine)
{
    CCommitEngine **    ppEng = NULL;
    long                i;
    IUnknown *          pUnk = NULL;
    IUnknown *          pUnkObj = NULL;
    long                cEngObjs;
    HRESULT             hr = S_OK;
    
    Assert( ppEngine != NULL );
    *ppEngine = NULL;  // for safety.

    // 
    // First search among existing engines to see if one fits the bill
    //

    for (i = _aryEngine.Size(), ppEng = _aryEngine; i > 0; i--, ppEng++)
    {
        cEngObjs = (*ppEng)->_aryObjs.Size();
        if (cEngObjs > 0)
        {
            //
            // It is sufficient to just check only the first dispatch
            // coming in because some set of objects can only be
            // in one commit engine.  
            //
            
            Verify(!(*ppEng)->_aryObjs[0]->QueryInterface(
                IID_IUnknown, 
                (void **)&pUnk));
            Verify(!(*ppDisps)->QueryInterface(IID_IUnknown, (void **)&pUnkObj));
            if (pUnk == pUnkObj)
            {
                //
                // Yay!  We found a match, bail out.
                //
                
                *ppEngine = *ppEng;
                goto Cleanup;
            }
        }
        ClearInterface(&pUnk);
        ClearInterface(&pUnkObj);
    }

    //
    // The fact that we got here implies that no suitable commit engine
    // was found.  So create one up and return it.
    //

    Assert(!*ppEngine);
    *ppEngine = new CCommitEngine();
    if (!*ppEngine)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    hr = THR(_aryEngine.Append(*ppEngine));
    if (hr)
        goto Error;

    hr = THR((*ppEngine)->SetObjects(cObj, ppDisps));
    if (hr)
        goto Error;
        
Cleanup:
    ClearInterface(&pUnk);
    ClearInterface(&pUnkObj);
    RRETURN(hr);

Error:
    if (_aryEngine.Find(*ppEngine) != -1)
        _aryEngine.DeleteByValue(*ppEngine);
    delete *ppEngine;
    *ppEngine = NULL;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     DPD::AppendEnumValue
//
//  Synopsis:   Adds a new enumerated value to the list maintained by
//              this property descriptor.
//
//  Arguments:  [pstr]      Friendly name
//              [value]     Value
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
DPD::AppendEnumValue(TCHAR * pstr, int value)
{
    HRESULT hr;
    EVAL    eval;

    Assert(pAryEVAL);
    Assert(fOwnEVAL);

    hr = THR(FormsAllocString(pstr, &eval.bstr));
    if (!hr)
    {
        eval.value = value;

        hr = THR(pAryEVAL->AppendIndirect(&eval));
        if (hr)
            FormsFreeString(eval.bstr);
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     DPD::Free
//
//  Synopsis:   Frees any resources attached to this property descriptor
//
//-------------------------------------------------------------------------

void
DPD::Free( )
{
    int     i;
    EVAL *  pEVAL;

    FormsFreeString(bstrName);
    FormsFreeString(bstrType);

    VariantClear(&var);
    if (fOwnEVAL && pAryEVAL)
    {
        for (i = pAryEVAL->Size(), pEVAL = *pAryEVAL;
             i > 0;
             i--, pEVAL++)
        {
            FormsFreeString(pEVAL->bstr);
        }

        delete pAryEVAL;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CCommitEngine::CCommitEngine
//
//  Synopsis:   ctor
//
//-------------------------------------------------------------------------

CCommitEngine::CCommitEngine()
    : _aryObjs(Mt(CCommitEngine_aryObjs_pv)),
      _aryDPD(Mt(CCommitEngine_aryDPD_pv))
{
    long    i;
    
    //  BUGBUG may want to do this in SetObjects, so we can
    //    return errors...

    static EVALINIT s_aevalBool[] =
    {
        { _T("True"), -1 },
        { _T("False"), 0 },
    };

    static EVALINIT s_aevalColor[] =
    {
        { _T("Scrollbar"), OLECOLOR_FROM_SYSCOLOR(0) },
        { _T("Background"), OLECOLOR_FROM_SYSCOLOR(1) },
        { _T("Active Caption"), OLECOLOR_FROM_SYSCOLOR(2) },
        { _T("Inactive Caption"), OLECOLOR_FROM_SYSCOLOR(3) },
        { _T("Menu"), OLECOLOR_FROM_SYSCOLOR(4) },
        { _T("Window"), OLECOLOR_FROM_SYSCOLOR(5) },
        { _T("Window Frame"), OLECOLOR_FROM_SYSCOLOR(6) },
        { _T("Menu Text"), OLECOLOR_FROM_SYSCOLOR(7) },
        { _T("Window Text"), OLECOLOR_FROM_SYSCOLOR(8) },
        { _T("Caption Text"), OLECOLOR_FROM_SYSCOLOR(9) },
        { _T("Active Border"), OLECOLOR_FROM_SYSCOLOR(10) },
        { _T("Inactive Border"), OLECOLOR_FROM_SYSCOLOR(11) },
        { _T("App Work Space"), OLECOLOR_FROM_SYSCOLOR(12) },
        { _T("Highlight"), OLECOLOR_FROM_SYSCOLOR(13) },
        { _T("Highlight Text"), OLECOLOR_FROM_SYSCOLOR(14) },
        { _T("Button Face"), OLECOLOR_FROM_SYSCOLOR(15) },
        { _T("Button Shadow"), OLECOLOR_FROM_SYSCOLOR(16) },
        { _T("Gray Text"), OLECOLOR_FROM_SYSCOLOR(17) },
        { _T("Button Text"), OLECOLOR_FROM_SYSCOLOR(18) },
        { _T("Inactive Caption Text"), OLECOLOR_FROM_SYSCOLOR(19) },
        { _T("Button Highlight"), OLECOLOR_FROM_SYSCOLOR(20) },
        { _T("3D Dark"), OLECOLOR_FROM_SYSCOLOR(21) },
        { _T("3D Light"), OLECOLOR_FROM_SYSCOLOR(22) },
        { _T("Info Text"), OLECOLOR_FROM_SYSCOLOR(23) },
        { _T("Info Background"), OLECOLOR_FROM_SYSCOLOR(24) },
    };

    _aryObjs.SetSize(0);
    _aryDPD.SetSize(0);
    memset(&_dpdBool, 0, sizeof(DPD));
    _dpdBool.pAryEVAL = new(Mt(CCommitEngine_dpdBool_pAryEVAL)) CDataAry<EVAL>(Mt(CCommitEngine_dpdBool_pAryEVAL_pv));
    _dpdBool.fOwnEVAL = TRUE;

    for (i = 0; i < ARRAY_SIZE(s_aevalBool); i++)
    {
        IGNORE_HR(_dpdBool.AppendEnumValue(
                s_aevalBool[i].pstr,
                s_aevalBool[i].value));
    }

    memset(&_dpdColor, 0, sizeof(DPD));
    _dpdColor.pAryEVAL = new(Mt(CCommitEngine_dpdColor_pAryEVAL)) CDataAry<EVAL>(Mt(CCommitEngine_dpdColor_pAryEVAL_pv));
    _dpdColor.fOwnEVAL = TRUE;

    for (i = 0; i < ARRAY_SIZE(s_aevalColor); i++)
    {
        IGNORE_HR(_dpdColor.AppendEnumValue(
                s_aevalColor[i].pstr,
                s_aevalColor[i].value));
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CCommitEngine::~CCommitEngine
//
//  Synopsis:   dtor
//
//-------------------------------------------------------------------------

CCommitEngine::~CCommitEngine()
{
    ReleaseObjects();
    _dpdBool.Free();
    _dpdColor.Free();
}


//+------------------------------------------------------------------------
//
//  Member:     CCommitEngine::SetObjects
//
//  Synopsis:   Set the objects for this commit engine
//
//  Arguments:  cObj        Count of objects
//              ppDisp      Array of dispatch ptrs
//
//-------------------------------------------------------------------------

HRESULT
CCommitEngine::SetObjects(long cObj, IDispatch **ppDisps)
{
    long        i;
    IDispatch **ppDisp;
    HRESULT     hr;
    
    if (_aryObjs.Size() != 0)
    {
#if DBG == 1
        //
        // The incoming objects better be the same as the ones
        // we have.
        //

        IDispatch **    ppDisp2;
        IUnknown *      pUnk;
        IUnknown *      pUnk2;
        
        Assert(cObj == _aryObjs.Size());
        for (i = cObj, ppDisp = _aryObjs, ppDisp2 = ppDisps;
                i > 0; i--, ppDisp++, ppDisp2++)
        {
            Verify(!(*ppDisp)->QueryInterface(IID_IUnknown, (void **)&pUnk));
            Verify(!(*ppDisp2)->QueryInterface(IID_IUnknown, (void **)&pUnk2));
            Assert(pUnk == pUnk2);
            ReleaseInterface(pUnk);
            ReleaseInterface(pUnk2);
        }
#endif

        return S_OK;
    }

    //
    // First cache a ptr to the dispatches handed in
    //
   
    hr = THR(_aryObjs.EnsureSize(cObj));
    if (hr)
        goto Cleanup;
        
    for (i = cObj, ppDisp = ppDisps; i > 0; i--, ppDisp++)
    {
        hr = THR(_aryObjs.Append(*ppDisp));
        if (hr)
            goto Error;

        (*ppDisp)->AddRef();
    }

    //
    // Now interrogate the dispatches to fill up the property
    // descriptor list.
    //
    
    hr = THR(CreatePropertyDescriptor());
    if (hr)
        goto Error;

    UpdateValues();
        
Cleanup:
    RRETURN(hr);

Error:
    _aryObjs.ReleaseAll();
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     CCommitEngine::ReleaseObjects
//
//  Synopsis:   Frees the objects for this commit engine
//
//-------------------------------------------------------------------------

HRESULT
CCommitEngine::ReleaseObjects()
{
    _aryObjs.ReleaseAll();
    RRETURN(ReleasePropertyDescriptor());
}


//+------------------------------------------------------------------------
//
//  Member:     CCommitEngine::Commit
//
//  Synopsis:   Commit any dirty properties into the object(s) below.
//
//-------------------------------------------------------------------------

HRESULT
CCommitEngine::Commit()
{
    long        i;
    DPD *       pDPD;
    HRESULT     hr = S_OK;
    
    for (i = _aryDPD.Size(), pDPD = _aryDPD; i > 0; i--, pDPD++)
    {
        if (pDPD->fReadOnly)
            continue;

        if (pDPD->fDirty)
        {
            if ( pDPD -> fSpecialCaseUnitMeasurement )
            {
                hr = THR(SetCommonSubObjectPropertyValue(
                        pDPD->dispid,
                        3, // Should be dispid of unit meas sub-object Text property
                        _aryObjs.Size(),
                        _aryObjs,
                        &(pDPD->var)));
            }
            else
            {
                hr = THR(SetCommonPropertyValue(
                        pDPD->dispid,
                        _aryObjs.Size(),
                        _aryObjs,
                        &(pDPD->var)));
            }
            if (hr)
                goto Cleanup;

            pDPD->fDirty = 0;
        }
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CCommitEngine::GetProperty
//
//  Synopsis:   Get the value of some property
//
//  Arguments:  dispid      Dispid to query
//              pvar        Resultant variant
//
//-------------------------------------------------------------------------

HRESULT
CCommitEngine::GetProperty(DISPID dispid, VARIANT *pvar)
{
    long        i;
    DPD *       pDPD;
    HRESULT     hr = S_OK;

    //
    // First find property descriptor
    //
    
    for (i = _aryDPD.Size(), pDPD = _aryDPD; i > 0; i--, pDPD++)
    {
        if (pDPD->dispid == dispid)
            break;
    }

    if (i <= 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Now get property
    //

    VariantInit(pvar);
    hr = THR(VariantCopy(pvar, &(pDPD->var)));
    
Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CCommitEngine::SetProperty
//
//  Synopsis:   Set the value of some property
//
//  Arguments:  dispid      Dispid to query
//              pvar        Variant coming in
//
//-------------------------------------------------------------------------

HRESULT
CCommitEngine::SetProperty(DISPID dispid, VARIANT *pvar)
{
    long        i;
    DPD *       pDPD;
    HRESULT     hr = S_OK;
    
    //
    // First find property descriptor
    //
    
    for (i = _aryDPD.Size(), pDPD = _aryDPD; i > 0; i--, pDPD++)
    {
        if (pDPD->dispid == dispid)
            break;
    }

    if (i <= 0 || pDPD->fReadOnly)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Now set property and mark as dirty
    //

    VariantClear(&(pDPD->var));
    hr = THR(VariantCopy(&(pDPD->var), pvar));
    if (hr)
        goto Cleanup;

    pDPD->fDirty = 1;
    
Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CCommitEngine::CreatePropertyDescriptor
//
//  Synopsis:   Create property descriptor list based on set objects
//
//-------------------------------------------------------------------------

HRESULT
CCommitEngine::CreatePropertyDescriptor()
{
    BSTR                        bstr;
    DPD *                       pDPD = NULL;
    DPD *                       pDPD2;
    UINT                        ucNames;
    int                         i, j, k;
    ULONG                       iTI, cTI;
    IDispatch **                ppDisp;
    IProvideMultipleClassInfo * pPMCI   = NULL;
    ITypeInfo *                 pTI     = NULL;
    TYPEATTR *                  pTA     = NULL;
    VARDESC *                   pVD     = NULL;
    FUNCDESC *                  pFD     = NULL;
    HRESULT                     hr      = S_OK;
    BOOL                        fGot;
    BOOL                        fReadOnly = FALSE;

    Assert(_aryDPD.Size() == 0);

    for (i = 0, ppDisp = _aryObjs;
         i < _aryObjs.Size();
         i++, ppDisp++)
    {
        // Determine if the object supports multiple ITypeInfos
        hr = THR_NOTRACE((*ppDisp)->QueryInterface(
                                       IID_IProvideMultipleClassInfo,
                                       (void **)&pPMCI));
        if (!hr)
        {
            hr = THR(pPMCI->GetMultiTypeInfoCount(&cTI));
            if (hr)
                goto Error;
        }
        else
        {
            cTI = 1;
        }

        // If this is not the first object (in the selection), mark each property
        // as "not seen" so those which are not common between all objects may
        // be removed
        if (i > 0)
        {
            for (k = _aryDPD.Size(), pDPD = _aryDPD;
                 k > 0;
                 k--, pDPD++)
            {
                pDPD->fVisit = FALSE;
            }
        }

        // For each ITypeInfo, retrieve the appropriate members
        for (iTI=0; iTI < cTI; iTI++)
        {
            // If the object supports multiple ITypeInfos, retrieve the ITypeInfo by index
            if (pPMCI)
            {
                ITypeInfo * pTICoClass;

                hr = THR(pPMCI->GetInfoOfIndex(iTI,
                                               MULTICLASSINFO_GETTYPEINFO,
                                               &pTICoClass,
                                               NULL, NULL, NULL, NULL));
                if (hr)
                {
                    hr = S_OK;
                    continue;
                }

                hr = THR(GetTypeInfoFromCoClass(pTICoClass, FALSE, &pTI, NULL));
                ReleaseInterface(pTICoClass);
                if (hr)
                    goto Error;
            }

            // Otherwise, obtain the ITypeInfo available through IDispatch
            else
            {
                Assert(cTI == 1);
                Assert(iTI == 0);
                hr = THR((*ppDisp)->GetTypeInfo(0, g_lcidUserDefault, &pTI));
                if (hr)
                {
                    hr = S_OK;
                    continue;
                }
            }

            hr = THR(pTI->GetTypeAttr(&pTA));
            if (hr)
                goto Error;

            if ((pTA->cVars == 0) && (pTA->cFuncs == 0))
                continue;

            // If this is on the first object (in the selection), ensure the array
            // is large enough to hold all the elements
            // (If the object supports multiple ITypeInfos, the array will be grown
            //  by the amount each ITypeInfo contributes)
            if (i == 0)
            {
                int cDPD = _aryDPD.Size();

                hr = THR(_aryDPD.EnsureSize(cDPD + pTA->cVars + pTA->cFuncs));
                if (hr)
                    goto Error;

                pDPD = (DPD *)_aryDPD + cDPD;
            }

            // First, obtain all property descriptions
            for (j = 0; j < pTA->cVars; j++)
            {
                hr = THR(pTI->GetVarDesc(j, &pVD));
                if (hr)
                    goto Error;

                // Do not display non-browsable, or hidden properties
                if (!(pVD->wVarFlags & (VARFLAG_FNONBROWSABLE |
                                        VARFLAG_FHIDDEN)))
                {
                    hr = THR(pTI->GetNames(pVD->memid, &bstr, 1, &ucNames));
                    if (hr)
                        goto Error;

                    if (i == 0)
                    {
                        memset(pDPD, 0, sizeof(DPD));

                        pDPD->dispid = pVD->memid;
                        pDPD->vt = (VARENUM) pVD->elemdescVar.tdesc.vt;
                        pDPD->bstrName = bstr;
                        pDPD->fReadOnly = (pVD->wVarFlags & VARFLAG_FREADONLY) ?
                                (TRUE) : (FALSE);
                        VariantInit(&pDPD->var);

                        if (pDPD->vt == VT_BOOL)
                        {
                            pDPD->pAryEVAL = _dpdBool.pAryEVAL;
                        }
                        else if (pDPD->vt == VT_USERDEFINED || pDPD->vt == VT_PTR)
                        {
                            hr = THR(ParseUserDefined(pTI, pVD, pDPD, TRUE));
                            if (hr)
                            {
                                pDPD->Free();
                                goto Cleanup;
                            }
                        }
                        else if (pDPD->vt == VT_UNKNOWN)
                        {
                            hr = THR ( ParseUnknown(pTI, pDPD) );
                            if ( hr )
                            {
                                pTI->ReleaseVarDesc(pVD);
                                continue;
                            }
                        }

                        pDPD++;
                        _aryDPD.SetSize(_aryDPD.Size() + 1);
                    }
                    else
                    {
                        for (k = _aryDPD.Size(), pDPD = _aryDPD;
                             k > 0;
                             k--, pDPD++)
                        {
                            if (pDPD->dispid == pVD->memid &&
                                pDPD->vt == (VARENUM) pVD->elemdescVar.tdesc.vt &&
                                !FormsStringCmp(pDPD->bstrName, bstr))
                            {
                                pDPD->fVisit = TRUE;
                                break;
                            }
                        }

                        FormsFreeString(bstr);
                    }
                }
                pTI->ReleaseVarDesc(pVD);
                pVD = NULL;
            }

            // Next, obtain all property function descriptions
            for ( ; j < pTA->cVars + pTA->cFuncs; j++)
            {
                hr = THR(pTI->GetFuncDesc(j - pTA->cVars, &pFD));
                if (hr)
                    goto Error;

                fReadOnly = (pFD->invkind == INVOKE_PROPERTYGET) ?
                        (TRUE) : (FALSE);
                fGot = FALSE;

                // Do We have a sub-object
				if (!(pFD->wFuncFlags & 
                        (FUNCFLAG_FRESTRICTED|FUNCFLAG_FNONBROWSABLE|FUNCFLAG_FHIDDEN)) &&
					pFD->funckind == FUNC_DISPATCH &&
					pFD->invkind  == INVOKE_PROPERTYGET &&
					pFD->elemdescFunc.tdesc.vt == VT_PTR &&
					pFD->elemdescFunc.tdesc.lptdesc->vt == VT_USERDEFINED )
                {
                    fGot = TRUE;
                }
                else if// Do not display non-browsable or hidden properties
                    ((pFD->invkind == INVOKE_PROPERTYPUT   ||
                        pFD->invkind == INVOKE_PROPERTYGET ||
                        pFD->invkind == INVOKE_PROPERTYPUTREF ) &&
                        !(pFD->wFuncFlags & (FUNCFLAG_FNONBROWSABLE |
                                         FUNCFLAG_FHIDDEN)))
                {
                    DISPID  dispid = pFD->memid;

                    pTI->ReleaseFuncDesc(pFD);
                    pFD = NULL;

                    // Locate the "get" method for the property
                    hr = FindReadPropFuncDesc(
                                    pTI,
                                    dispid,
                                    j - pTA->cVars + 1,
                                    pTA->cFuncs,
                                    &pFD);
                    // If no "get" method exists, skip the property
                    if (!hr)
                    {
                        fGot = TRUE;
                    }
                }

                if ( fGot )
                {
                    hr = THR(pTI->GetNames(pFD->memid, &bstr, 1, &ucNames));
                    if (hr)
                        goto Error;

                    if (i == 0)
                    {
                        memset(pDPD, 0, sizeof(DPD));

                        pDPD->dispid = pFD->memid;
                        pDPD->fReadOnly = fReadOnly;
#if 1
                        pDPD->vt = (VARENUM) pFD->elemdescFunc.tdesc.vt;
#else
                        if(pFD->funckind == FUNC_PUREVIRTUAL)
                        {
                            pDPD->vt = (VARENUM) pFD->lprgelemdescParam->tdesc.lptdesc->vt;
                        }
                        else
                        {
                            pDPD->vt = (VARENUM) pFD->elemdescFunc.tdesc.vt;
                        }
#endif
                        pDPD->bstrName = bstr;
                        VariantInit(&pDPD->var);

                        if (pDPD->dispid == DISPID_FONT)
                            i = 0;

                        if (pDPD->vt == VT_BOOL)
                        {
                            pDPD->pAryEVAL = _dpdBool.pAryEVAL;
                        }
                        else if (pDPD->vt == VT_USERDEFINED || pDPD->vt == VT_PTR)
                        {
                            hr = THR(ParseUserDefined(pTI, pFD, pDPD, FALSE));
                            if (hr)
                            {
                                pDPD->Free();
                                goto Cleanup;
                            }
                        }
                        else if (pDPD->vt == VT_UNKNOWN)
                        {
                            ParseUnknown(pTI, pDPD);
                        }

                        pDPD++;
                        _aryDPD.SetSize(_aryDPD.Size() + 1);
                    }
                    else
                    {
                        for (k = _aryDPD.Size(), pDPD = _aryDPD;
                             k > 0;
                             k--, pDPD++)
                        {
                            if (pDPD->dispid == pFD->memid &&
                                pDPD->vt == (VARENUM) pFD->elemdescFunc.tdesc.vt &&
                                !FormsStringCmp(pDPD->bstrName, bstr))
                            {
                                pDPD->fVisit = TRUE;
                                break;
                            }
                        }

                        FormsFreeString(bstr);
                    }
                }

                pTI->ReleaseFuncDesc(pFD);
                pFD = NULL;
            }

            pTI->ReleaseTypeAttr(pTA);
            pTA = NULL;

            ClearInterface(&pTI);
        }

        // Remove entries which do not occur on all objects in the selection
        if (i > 0)
        {
            for (k = _aryDPD.Size(), pDPD2 = pDPD = _aryDPD;
                 k > 0;
                 k--, pDPD++)
            {
                if (pDPD->fVisit)
                {
                    if (pDPD != pDPD2)
                        memcpy(pDPD2, pDPD, sizeof(DPD));

                    pDPD2++;
                }
                else
                {
                    pDPD->Free();
                    _aryDPD.SetSize(_aryDPD.Size() - 1);
                }
            }
        }

        ClearInterface(&pPMCI);
    }

Cleanup:
    if (pTA)
        pTI->ReleaseTypeAttr(pTA);
    if (pVD)
        pTI->ReleaseVarDesc(pVD);
    if (pFD)
        pTI->ReleaseFuncDesc(pFD);
    ReleaseInterface(pTI);
    ReleaseInterface(pPMCI);

    RRETURN(hr);

Error:
    ReleasePropertyDescriptor();
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     CCommitEngine::ParseUserDefined
//
//  Synopsis:   Parses a user-defined type in an object's TypeInfo.  Called
//              from ExamineObjects.
//
//  Arguments:  [pTIObject]     Object's TypeInfo
//              [pVDObject]     VARDESC for user-defined prop
//              [pDPD]          Descriptor to fill in
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CCommitEngine::ParseUserDefined(
        ITypeInfo * pTIObject,
        void * pv,
        DPD * pDPD,
        BOOL fVar)
{
    HRESULT             hr;
    TYPEDESC *          ptdesc;
    ITypeInfo *         pTI     = NULL;
    TYPEATTR *          pTA     = NULL;
    VARDESC *           pVD     = NULL;
    BSTR                bstr    = NULL;
    CDataAry<EVAL> *    pAryEVAL;
    int                 i;
    EVAL                eval;

//#ifndef _MAC
#if 1
    ptdesc = fVar ? &((VARDESC *) pv)->elemdescVar.tdesc :
                    &((FUNCDESC *) pv)->elemdescFunc.tdesc;
    if (ptdesc->vt == VT_PTR)
#else
    ptdesc = fVar ? &((VARDESC *) pv)->elemdescVar.tdesc :
                    &((FUNCDESC *) pv)->lprgelemdescParam->tdesc;
    while (ptdesc->vt == VT_PTR)

#endif
    {
        ptdesc = ptdesc->lptdesc;
        pDPD->fIndirect = TRUE;
    }
    if (ptdesc->vt != VT_USERDEFINED)
        return S_OK;

    hr = THR(pTIObject->GetRefTypeInfo(
            ptdesc->hreftype,
            &pTI));
    if (hr)
        goto Cleanup;

    hr = THR(pTI->GetTypeAttr(&pTA));
    if (hr)
        goto Cleanup;

    //
    // Skip aliases here except for the color
    //

    while (TKIND_ALIAS == pTA->typekind && pTA->guid != GUID_COLOR
        && pTA->tdescAlias.vt == VT_USERDEFINED && pTA->tdescAlias.hreftype != NULL)
    {
        ITypeInfo * pTIAlias = NULL;
        
        Assert(pTA->tdescAlias.vt == VT_USERDEFINED);
        
        hr = THR(pTI->GetRefTypeInfo(pTA->tdescAlias.hreftype, &pTIAlias));
        if (hr)
            goto Cleanup;

        pTI->ReleaseTypeAttr(pTA);
        
        hr = THR(pTIAlias->GetTypeAttr(&pTA));

        ReleaseInterface(pTI);
        pTI = pTIAlias;
        if (hr)
            goto Cleanup;
    }
        
    hr = THR(pTI->GetDocumentation(-1, &bstr, NULL, NULL, NULL));
    if (hr)
        goto Cleanup;

    Assert(pDPD->bstrType == NULL);
    pDPD->bstrType = bstr;
    bstr = NULL;

    switch (pTA->typekind)
    {
    case TKIND_ENUM:
        {
            pAryEVAL = pDPD->pAryEVAL = new(Mt(CCommitEngineParseUserDefined_pDPD_pAryEVAL)) CDataAry<EVAL>(Mt(CCommitEngineParseUserDefined_pDPD_pAryEVAL_pv));
            if (!pAryEVAL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            pDPD->fOwnEVAL = TRUE;

            for (i = 0; i < pTA->cVars; i++)
            {
                hr = THR(pTI->GetVarDesc(i, &pVD));
                if (hr)
                    goto Cleanup;
                Assert(pVD->varkind == VAR_CONST);

                hr = THR(pTI->GetDocumentation(pVD->memid, NULL, &bstr, NULL, NULL));
                if (hr)
                    goto Cleanup;
#ifdef _MAC
                // BUGBUG   pTI->GetDocumentation returns a bogus BSTR that is not
                //      NULL terminated and with a byteswapped length.  Temporarily
                //      we get around this by making our own BSTR...
                UINT len = SysStringLen(bstr);
                if(len > 256)  // we don't have any helpstrings bigger than 256 bytes
                {
                    eval.bstr = SysAllocStringLen(bstr,len));
                    SysFreeString(bstr);
                }
                else
                {
                    eval.bstr = bstr;
                }
#else

                eval.bstr = bstr;
#endif
                eval.value = V_I4(pVD->lpvarValue);

                hr = THR(pAryEVAL->AppendIndirect(&eval));
                if (hr)
                    goto Cleanup;

                bstr = NULL;

                pTI->ReleaseVarDesc(pVD);
                pVD = NULL;
            }
        }
        break;

    case TKIND_DISPATCH:
        IGNORE_HR( ParseUnknown(pTI, pDPD));
        break;

    case TKIND_ALIAS:
        if (pTA->guid == GUID_COLOR)
        {
            pDPD->fSpecialCaseColor = TRUE;
            pDPD->pAryEVAL = _dpdColor.pAryEVAL;
        }
        break;
    }

Cleanup:
    if (pTA)
        pTI->ReleaseTypeAttr(pTA);
    if (pVD)
        pTI->ReleaseVarDesc(pVD);
    ReleaseInterface(pTI);
    FormsFreeString(bstr);

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CCommitEngine::ParseUnknown
//
//  Synopsis:   Tests for support of IPicture/IFont.
//
//--------------------------------------------------------------------------

HRESULT
CCommitEngine::ParseUnknown(ITypeInfo *pTypeInfo, DPD * pDPD)
{
    ITypeLib *pTypeLib = NULL;
    ITypeInfo *pInterfaceInfo = NULL;
    HRESULT hr = S_OK;
    USHORT uFound = 1; // Find the first matching name
    MEMBERID memID; // Not used
    TYPEATTR *pTypeAttr = NULL;
    UINT nTypeInfoIndex;

    switch (pDPD->dispid)
    {
    case DISPID_FONT:
        pDPD->fSpecialCaseFont = TRUE;
        break;

    case DISPID_MOUSEICON:
        pDPD->fSpecialCaseMouseIcon = TRUE;
        // Intentional fallthrough.
    case DISPID_PICTURE:
        pDPD->fSpecialCasePicture = TRUE;
        break;

    default:
        // The pDPD->bstrType contains the arg name for the property
        // go to the typelibrary, find the interface type info corresponding
        // to this name, and see if its IID is the unit measurement subobject
        hr = pTypeInfo->GetContainingTypeLib ( &pTypeLib, &nTypeInfoIndex );
        if ( hr )
            goto Cleanup;

        hr = pTypeLib->FindName ( pDPD->bstrType, 0, 
            &pInterfaceInfo, &memID, &uFound );
        if ( hr )
            goto Cleanup;

        hr = pInterfaceInfo->GetTypeAttr ( &pTypeAttr );
        if ( hr )
            goto Cleanup;

        hr = S_FALSE;
        break;

    }
Cleanup:
    if ( pInterfaceInfo && pTypeAttr )
        pInterfaceInfo->ReleaseTypeAttr ( pTypeAttr );
    ReleaseInterface ( pInterfaceInfo ); 
    ReleaseInterface ( pTypeLib ); 
    RRETURN1 ( hr, S_FALSE );
}


//+------------------------------------------------------------------------
//
//  Member:     CCommitEngine::FindReadPropFuncDesc
//
//  Synopsis:   Find FUNCDESC for get property with specified DISPID
//              This is helper function for ExamineObjects().
//
//-------------------------------------------------------------------------
HRESULT
CCommitEngine::FindReadPropFuncDesc(
        ITypeInfo * pTI,
        DISPID      dispid,
        int         iStart,         // Start search point for optimis
        int         iCount,         // Total number of functions in TypeInfo
        FUNCDESC **  ppFD)
{
    HRESULT     hr = E_FAIL;
    int         i;

    //
    // Start at the current location and move forward to the end,
    // If not found, start at the beginning of the FUNCDESCs and search to
    // the current location
    //

    for (i = iStart; i < iCount; i++)
    {
        hr = THR(pTI->GetFuncDesc(i, ppFD));
        if (hr)
            goto Cleanup;

        if ((*ppFD)->invkind == INVOKE_PROPERTYGET &&
                (*ppFD)->memid == dispid)

        {
            // Find the funcion desc for the read property
            return S_OK;
        }
        pTI->ReleaseFuncDesc(*ppFD);
        *ppFD = NULL;
    }

    for (i = 0; i < iStart; i++)
    {
        hr = THR(pTI->GetFuncDesc(i, ppFD));
        if (hr)
            goto Cleanup;

        if ((*ppFD)->invkind == INVOKE_PROPERTYGET &&
                (*ppFD)->memid == dispid)

        {
            // Find the funcion desc for the read property
            return S_OK;
        }
        pTI->ReleaseFuncDesc(*ppFD);
        *ppFD = NULL;
    }

Cleanup:
    pTI->ReleaseFuncDesc(*ppFD);
    *ppFD = NULL;
    RRETURN_NOTRACE(E_FAIL);
}


//+------------------------------------------------------------------------
//
//  Member:     CCommitEngine::ReleasePropertyDescriptor
//
//  Synopsis:   Release the created property descriptor list
//
//-------------------------------------------------------------------------

HRESULT
CCommitEngine::ReleasePropertyDescriptor()
{
    long    i;
    DPD *   pDPD;

    for (i = _aryDPD.Size(), pDPD = _aryDPD; i > 0; i--, pDPD++)
    {
        pDPD->Free();
    }
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CCommitEngine::UpdateValues
//
//  Synopsis:   Fill in values into _aryDPD
//
//-------------------------------------------------------------------------

void
CCommitEngine::UpdateValues()
{
    HRESULT     hr;
    int         i;
    DPD *       pDPD;
    
    for (i = _aryDPD.Size(), pDPD = _aryDPD;
         i > 0;
         i--, pDPD++)
    {
        Assert(pDPD->var.vt == VT_EMPTY);

        // Clear down the current value
        VariantClear(&pDPD->var);

        hr = THR_NOTRACE(GetCommonPropertyValue(
                pDPD->dispid,
                _aryObjs.Size(),
                _aryObjs,
                &pDPD->var));

        if (hr == S_FALSE)
        {
            pDPD->fNoMatch = TRUE;
            pDPD->fMemberNotFound = FALSE;
        }
        else
        {
            pDPD->fNoMatch = FALSE;
            pDPD->fMemberNotFound = (BOOL) hr;
        }
    }
}

