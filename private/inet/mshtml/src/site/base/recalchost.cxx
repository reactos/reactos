// ---------------------------------------------------------
//
// Microsoft Trident
// Copyright Microsoft corporation 1998
//
// File: recalchost.cxx
//
// Recalc engine hosting code
//
// ---------------------------------------------------------

#include <headers.hxx>

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include "mshtmdid.h"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_SITECNST_HXX_
#define X_SITECNST_HXX_
#include "sitecnst.hxx"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include <treepos.hxx>
#endif

#ifndef X_ATOM_HXX
#define X_ATOM_HXX
#include "atomtbl.hxx"
#endif

#ifndef X_SHOLDER_HXX_
#define X_SHOLDER_HXX_
#include "sholder.hxx"
#endif

#ifndef X_RECALC_HXX_
#define X_RECALC_HXX_
#include "recalc.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif

ExternTag(tagRecalcInfo);
ExternTag(tagRecalcDetail);
ExternTag(tagRecalcStyle);

DeclareTag(tagRecalcSync, "Recalc Sync", "Do recalc synchronously")
DeclareTag(tagRecalcHost, "Recalc Host", "Trace recalc host support")
DeclareTag(tagRecalcInfo, "Recalc", "Trace recalc behaviour")
DeclareTag(tagRecalcGetInfo, "Recalc", "Enable GetObjectInfo to get object id and prop name")
DeclareTag(tagRecalcDetail, "Recalc detail", "Detailed recalc trace")
DeclareTag(tagRecalcDisable, "Recalc", "Disable all recalc")
DeclareTag(tagRecalcDisableCSS, "Recalc", "Disable CSS expressions");
DeclareTag(tagRecalcDumpBefore, "Recalc", "Dump before every recalc");
DeclareTag(tagRecalcDumpAfter, "Recalc", "Dump after every recalc")
//------------------------------------
//
// CDoc::CRecalcHost::QueryInterface
//
// Description: IUnknown::QueryInterface
//
//------------------------------------

STDMETHODIMP
CDoc::CRecalcHost::QueryInterface(REFIID iid, LPVOID *ppv)
{
    if (ppv == 0)
        RRETURN(E_INVALIDARG);
    *ppv = 0;
    
    switch (iid.Data1)
    {
        QI_INHERITS((IRecalcHost *)this, IUnknown)
        QI_INHERITS(this, IRecalcHost)
        QI_INHERITS(this, IServiceProvider)
#if DBG == 1
        QI_INHERITS(this, IRecalcHostDebug)
#endif
    }
    if (*ppv == NULL)
        RRETURN(E_NOINTERFACE);

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}

//------------------------------------
//
// CDoc::CRecalcHost::AddRef
//
// Description: IUnknown::AddRef
//
//------------------------------------

STDMETHODIMP_(ULONG)
CDoc::CRecalcHost::AddRef()
{
    return MyDoc()->SubAddRef();
}

//------------------------------------
//
// CDoc::CRecalcHost::Release
//
// Description: IUnknown::Release
//
//------------------------------------

STDMETHODIMP_(ULONG)
CDoc::CRecalcHost::Release()
{
    return MyDoc()->SubRelease();
}

//------------------------------------
//
// CDoc::CRecalcHost::QueryService
//
// Description: IServiceProvider::QueryService
//
//------------------------------------

STDMETHODIMP
CDoc::CRecalcHost::QueryService(REFGUID guidService, REFIID riid, LPVOID *ppv)
{
    RRETURN(MyDoc()->QueryService(guidService, riid, ppv));
}

//------------------------------------
//
// CDoc::CRecalcHost::Init
//
// Description: Actually creates the recalc engine
//
//------------------------------------

HRESULT
CDoc::CRecalcHost::Init()
{
    if (_pEngine)
        return S_OK;

    HRESULT hr;
    IRecalcEngine *pEngine = 0;
    IObjectWithSite *pObject = 0;

    hr = THR(CoCreateInstance(CLSID_CRecalcEngine, NULL, CLSCTX_INPROC_SERVER, IID_IRecalcEngine, (LPVOID *)&pEngine));
    if (hr)
        goto Cleanup;

    hr = THR(pEngine->QueryInterface(IID_IObjectWithSite, (LPVOID *)&pObject));
    if (hr)
        goto Cleanup;

    hr = THR(pObject->SetSite((IRecalcHost *)this));
    if (hr)
        goto Cleanup;

    _pEngine = pEngine;
    _pEngine->AddRef();

Cleanup:
    TraceTag((tagRecalcHost, "Creating recalc engine %08x", pEngine));
    ReleaseInterface(pObject);
    ReleaseInterface(pEngine);

    RRETURN(hr);
}

//------------------------------------
//
// CDoc::CRecalcHost::Detach
//
// Description: Cleanup, release the recalc engine
//
//------------------------------------

void
CDoc::CRecalcHost::Detach()
{
    if (_pEngine)
    {
        IObjectWithSite *pObject;

        if (SUCCEEDED(THR(_pEngine->QueryInterface(IID_IObjectWithSite, (LPVOID *)&pObject))))
        {
            pObject->SetSite(NULL);
            pObject->Release();
        }
        ClearInterface(&_pEngine);
    }
}

HRESULT
CDoc::CRecalcHost::SuspendRecalc(VARIANT_BOOL fSuspend)
{
    if (fSuspend)
    {
        _ulSuspend++;
        Assert(_ulSuspend != 0);
    }
    else if (_ulSuspend > 0)
    {
        _ulSuspend--;
        if (_ulSuspend == 0 && _fRecalcRequested)
        {
            MyDoc()->GetView()->RequestRecalc();
        }
    }
    else
        RRETURN(E_UNEXPECTED);

    return S_OK;
}

//------------------------------------
//
// CDoc::CRecalcHost::setExpression
//
// Description: A helper function for CElement and CStyle
//
//------------------------------------
HRESULT
CDoc::CRecalcHost::setExpression(CBase *pBase, BSTR strPropertyName, BSTR strExpression, BSTR strLanguage)
{
    WHEN_DBG(if (IsTagEnabled(tagRecalcDisable)) return S_OK; )

    HRESULT hr;
    DISPID dispid = 0;

    TraceTag((tagRecalcInfo, "setExpression(%08x, \"%ls\", \"%ls\", \"%ls\")", pBase, strPropertyName, strExpression, strLanguage));

    // Don't do this while we're in recalc!
    if (_fInRecalc)
        RRETURN(E_UNEXPECTED);

    if (!strPropertyName || !strExpression)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // If there are any pending tasks, do them now so that all
    // expressions are either applied or cleared before we actually
    // try to do any work.
    MyDoc()->_view.ExecuteRecalcTasks(0);

    hr = THR(pBase->GetDispID(strPropertyName, fdexNameCaseSensitive | fdexNameEnsure, &dispid));
    if (hr)
        goto Cleanup;

    hr = THR(Init());
    if (hr)
        goto Cleanup;

    if (!strLanguage || (*strLanguage == 0))        // For consistency, the default is always javascript
    {
        strLanguage = _T("javascript");
    }

    hr = THR(_pEngine->SetExpression((IUnknown *)(IPrivateUnknown *)pBase, dispid, strExpression, strLanguage));
Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------
//
// Member:      CDoc::CRecalcHost::setStyleExpression
//
// Description: Called by the stylesheet 
//
//---------------------------------------------------------
HRESULT
CDoc::CRecalcHost::setStyleExpressions(CElement *pElement)
{
    WHEN_DBG(if (IsTagEnabled(tagRecalcDisableCSS)) return S_OK; )

    HRESULT hr = S_OK;
    CStyle *pStyle;

    int iExpandos = pElement->GetFirstBranch()->GetFancyFormat()->_iExpandos;

    // It is possible for the element cache to have been cleared before
    // we get a chance to run.  In this case iExpandos will be negative.
    // Some later pass through ComputeFormats will once again determine
    // the appropriate set of expressions for this element.
    // REVIEW (michaelw) Is it possible that we will never be called
    // again because try to be so lazy?  If so will expressions that are
    // currently applied to the object not be removed when they should?

    if (iExpandos < 0)
        goto Cleanup;

    // If we are in the middle of setting a value then we don't want to do anything
    if (_pElemSetValue)
        goto Cleanup;

    hr = Init();
    if (hr)
        goto Cleanup;

    hr = THR(pElement->GetStyleObject(&pStyle));
    if (hr)
        goto Cleanup;

    if (SUCCEEDED(_pEngine->BeginStyle((IUnknown *)pStyle)))
    {
        CAttrArray *pAA = GetExpandosAttrArrayFromCacheEx(iExpandos);
        CAttrValue *pAV = (CAttrValue *)*pAA;

        Assert(pAA);

        for ( int i = 0 ; i < pAA->Size() ; i++, pAV++ )
        {
            if (pAV->AAType() == CAttrValue::AA_Expression)
            {
                TraceTag((tagRecalcStyle, "\tsetting expression: this: %08x dispid: %08x expression:%ls", this, pAV->GetDISPID(), pAV->GetLPWSTR()));
                IGNORE_HR(_pEngine->SetExpression((IUnknown *)(IPrivateUnknown *)pStyle, pAV->GetDISPID(), pAV->GetLPWSTR(), _T("javascript")));
            }
        }
        IGNORE_HR(_pEngine->EndStyle((IUnknown *)(IPrivateUnknown *)pStyle));
    }

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------
//
//  Member:     CDoc::CRecalcHost::getExpression
//
// REVIEW michaelw : need to add a language parameter to allow
//                   the caller to get the expression language
//
//---------------------------------------------------------------
HRESULT
CDoc::CRecalcHost::getExpression(CBase *pBase, BSTR strPropertyName, VARIANT *pvExpression)
{
    HRESULT hr;
    BSTR strLanguage = 0;
    BSTR strExpression = 0;
    DISPID dispid = 0;

    TraceTag((tagRecalcInfo, "getExpression(%08x, \"%ls\")", pBase, strPropertyName));

    if (!strPropertyName || (*strPropertyName == 0) || (pvExpression == 0))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // If there are any pending tasks, do them now so that all
    // expressions are either applied or cleared before we actually
    // try to do any work.
    MyDoc()->_view.ExecuteRecalcTasks(0);

    hr = THR(pBase->GetDispID(strPropertyName, fdexNameCaseSensitive, &dispid));
    if (hr == DISP_E_UNKNOWNNAME)
    {
        hr = S_FALSE;
        goto Cleanup;
    }
    else if (hr)
        goto Cleanup;

    if (!_pEngine)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    hr = THR(_pEngine->GetExpression((IUnknown *)(IPrivateUnknown *)pBase, dispid, &strExpression, &strLanguage));
    if (!hr)
    {
        V_VT(pvExpression) = VT_BSTR;
        V_BSTR(pvExpression) = strExpression;
    }

Cleanup:
    if (hr == S_FALSE)
    {
        V_VT(pvExpression) = VT_EMPTY;
        hr = S_OK;
    }

    SysFreeString(strLanguage);
    RRETURN(hr);
}

//---------------------------------------------------------------
//
//  Member:     CDoc::CRecalcHost::removeExpression
//
//---------------------------------------------------------------
HRESULT
CDoc::CRecalcHost::removeExpression(CBase *pBase, BSTR strPropertyName, VARIANT_BOOL *pfSuccess)
{
    HRESULT hr = S_OK;
    DISPID dispid = 0;

    if (pfSuccess)
        *pfSuccess = VB_FALSE;

    TraceTag((tagRecalcInfo, "removeExpression(%08x, \"%ls\")", pBase, strPropertyName));

    // Don't do this while we're in recalc
    if (_fInRecalc)
        RRETURN(E_UNEXPECTED);

    if (!strPropertyName || (*strPropertyName == 0))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // If there are any pending tasks, do them now so that all
    // expressions are either applied or cleared before we actually
    // try to do any work.
    MyDoc()->_view.ExecuteRecalcTasks(0);

    // Get the dispid for strPropertyName.  
    hr = THR(pBase->GetDispID(strPropertyName, fdexNameCaseSensitive, &dispid));
    if (hr == DISP_E_UNKNOWNNAME)
    {
        hr = S_OK;
        goto Cleanup;
    }
    else if (hr)
        goto Cleanup;

    if (!_pEngine)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    hr = THR(_pEngine->ClearExpression((IUnknown *)(IPrivateUnknown *)pBase, dispid));
    if (hr)
        goto Cleanup;

    if (pfSuccess)
        *pfSuccess = VB_TRUE;

Cleanup:
    if (hr == S_FALSE)
        hr = S_OK;

    RRETURN(hr);
}

//------------------------------------
//
// CDoc::CRecalcHost::EngineRecalcAll
//
// Description: A little wrapper to RecalcAll on the engine.
//              Does not create the engine if it doesn't exist
//
//------------------------------------

HRESULT
CDoc::CRecalcHost::EngineRecalcAll(BOOL fForce)
{
    HRESULT hr = S_OK;

#if DBG==1
    if (IsTagEnabled(tagRecalcDumpBefore))
        Dump(0);
#endif

    // If someone explicitly calls document.recalc while we are actually
    // doing the recalc, we fail.
    if (_fInRecalc)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if (MyDoc()->_fEngineSuspended)
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (_ulSuspend == 0)
    {
        // Do any pending work for style expressions to be applied.
        MyDoc()->_view.ExecuteRecalcTasks(0);

        if (_pEngine)
        {
            BOOL fRunScript;
            hr = THR(MyDoc()->ProcessURLAction(URLACTION_SCRIPT_RUN, &fRunScript));
            if (hr || !fRunScript)
                goto Cleanup;

            // We clear the flag before calling RecalcAll
            // so that we don't ignore requests generated
            // while we're in recalc.
            _fRecalcRequested = FALSE;
 
            _fInRecalc = TRUE;
            if (SUCCEEDED(hr))
                hr = THR(_pEngine->RecalcAll(fForce));
            _fInRecalc = FALSE;
        }
    }
    else
        hr = S_OK;

Cleanup:
#if DBG==1
    if (IsTagEnabled(tagRecalcDumpBefore))
        Dump(0);
#endif

    RRETURN(hr);
}

//---------------------------------------------------------------
//
// Function:    CRecalcHost::RequestRecalc
//
// Description: The recalc engine has requested a recalc.  We choose
//              to do this async.
//
//---------------------------------------------------------------
STDMETHODIMP
CDoc::CRecalcHost::RequestRecalc()
{
    Assert(_pEngine);
#if DBG == 1
    if (IsTagEnabled(tagRecalcSync))
    {
        EngineRecalcAll(FALSE);
        return S_OK;
    }
#endif

    if (!_fRecalcRequested)
    {
        //
        // If we are suspended then the request to the view will be issued when we unsuspend
        //
        if (!_ulSuspend)
            MyDoc()->GetView()->RequestRecalc();
        _fRecalcRequested = TRUE;
    }

    return _fRecalcRequested ? S_OK : E_FAIL;
}
//------------------------------------
//
// CDoc::CRecalcHost::CompileExpression
//
// Description: IRecaclHost::ParseExpressionText
//
//------------------------------------

STDMETHODIMP
CDoc::CRecalcHost::CompileExpression(IUnknown *pUnk, DISPID dispid, BSTR strExpression, BSTR strLanguage, IDispatch **ppExpressionObject, IDispatch **ppThis)
{
    TraceTag((tagRecalcHost, "CompileExpression %ls %ls", strExpression, strLanguage));
    CStr sExpression;
    CStyle *pStyle = 0;

    // BUGBUG this should be done by the script engine (beta 2)
    if (strLanguage[0] == _T('J') || strLanguage[0] == _T('j'))
    {
        sExpression.Append(_T("return ("));
        sExpression.Append(strExpression);
        sExpression.Append(_T(")"));
    }
    else
        RRETURN(E_INVALIDARG);

    TraceTag((tagRecalcHost, "CompileExpression generated: %ls", sExpression));

	//
	// Even if an expression is applied to the style of an element,
	// we need to make sure that the "this" pointer of the expression 
	// code is the element and not element.style.
	//
    HRESULT hr = pUnk->QueryInterface(CLSID_CStyle, (LPVOID *)&pStyle);         // no addref
    if (hr)
    {
        hr = pUnk->QueryInterface(IID_IDispatch, (LPVOID *)ppThis);
    }
    else
    {
        hr = pStyle->GetElementPtr()->PrivateQueryInterface(IID_IDispatch, (LPVOID *)ppThis);
    }

    if (!hr)
        hr = THR(MyDoc()->_pScriptCollection->ConstructCode(
            NULL,                           // pchScope
            sExpression,                    // pchCode
            NULL,                           // pchFormalParams
            strLanguage,                    // pchLanguage
            NULL,                           // pMarkup
            NULL,                           // pchType
            0,                              // ulOffset
            0,                              // ulStartingLine
            NULL,                           // pSourceObject
            SCRIPTPROC_HOSTMANAGESSOURCE,   // dwFlags
            ppExpressionObject,             // ppDispCode result
            TRUE));                         // fSingleLine

    RRETURN(hr);
}

//------------------------------------
//
// CDoc::CRecalcHost::EvalExpression
//
// Description: IRecalcHost::EvalExpression
//
// Apparently has no support for DISPID_THIS
//
STDMETHODIMP
CDoc::CRecalcHost::EvalExpression(IUnknown *pUnk, DISPID dispid, BSTR strExpression, BSTR strLanguage, VARIANT *pvResult)
{
    HRESULT hr;

    if (!pvResult)
        RRETURN(E_INVALIDARG);

    TraceTag((tagRecalcHost, "Evaluating expression %ls", strExpression));

    VariantInit(pvResult);

    if (MyDoc()->_pScriptCollection)
    {
        CExcepInfo excepinfo;
  
        hr = THR(MyDoc()->_pScriptCollection->ParseScriptText(
            strLanguage,                    // pchLanguage
            NULL,                           // pMarkup
            NULL,                           // pchType
            strExpression,                  // pchCode
            NULL,                           // pchItemName
            _T("\""),                       // pchDelimiter
            0,                              // ulOffset
            0,                              // ulStartingLine
            NULL,                           // pSourceObject
            SCRIPTTEXT_ISEXPRESSION,        // dwFlags
            pvResult,                       // pvarResult
            &excepinfo));                   // pExcepInfo
    }
    else
        hr = E_FAIL;
    RRETURN(hr);
}
	
//------------------------------------
//
// CDoc::CRecalcHost::resolveName
//
// Description: An internal helper to resolve a single name
//
// This function walks the object.sub-object.sub-object.property string
//
// It doesn't handle arrays or anything other than strings
// This is by design.  Names involing arrays are very ambiguous
//
//------------------------------------

HRESULT
CDoc::CRecalcHost::resolveName(IDispatch *pDispatchThis, DISPID dispidThis, LPOLESTR szName, IDispatch **ppDispatch, DISPID *pDispid)
{
    IDispatch *pDispatch = 0;
    CVariant v;
    HRESULT hr = S_OK;

    TraceTag((tagRecalcHost, "resolveName: %ls", szName));

    //
    // parsing a name of the form object.sub-object.sub-object.sub-object.property
    // need to know about scope so that "this" works properly.
    //
    // we tolerate nothing but name.name.name.name
    // no spaces, no nulls, no array references
    //

    CStr sTemp;
    
    sTemp.Set(szName);

    LPTSTR pchTemp = sTemp;

    // Check for "this."
    if (_tcsncmp(pchTemp, sTemp.Length(), _T("this."), 5) == 0)
    {
        //
        // Our initial scope is this
        //
        pDispatch = pDispatchThis;
        pDispatch->AddRef();

        pchTemp += 5;          // skip past "this."
    }
    else
    {
        //
        // The default initial scope is the "window" object
        //
        // REVIEW michaelw: this should also look for form objects
        //
        hr = MyDoc()->EnsureOmWindow();
        if (hr)
            goto Cleanup;
        hr = THR(MyDoc()->_pOmWindow->QueryInterface(IID_IDispatchEx, (LPVOID *)&pDispatch));
        if (hr)
            goto Cleanup;
    }

    //
    // Now work our way through the names until we reach the end or something
    // doesn't map to sub-object
    //
    // REVIEW michaelw: is there any benefit (apart from complexity and case sensitivity)
    // REVIEW michaelw: to using IDispatchEx::GetDispID ?
    //

	while (*pchTemp)
	{
		LPTSTR pchEnd = pchTemp;

		// Skip over token characters
		while ((*pchEnd) && (*pchEnd != _T('.')))
			pchEnd++;

        if (pchEnd == pchTemp)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        BSTR strName = SysAllocStringLen(pchTemp, pchEnd - pchTemp);
        if (!strName)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

		// The token was terminated by a delimiter or the end of the string
		// in the case of a delimiter we need to skip past it.
		if (*pchEnd)
		{
			Assert(*pchEnd == _T('.'));
			pchTemp = pchEnd + 1;
			Assert(*pchTemp != _T('.'));
		}
        else
            pchTemp = pchEnd;

        hr = THR(GetNamedProp(pDispatch, strName, g_lcidUserDefault, &v, pDispid, NULL, FALSE, TRUE));

#if DBG == 1
        if (hr)
            TraceTag((tagRecalcHost, "resolveName: GetNamedProp(%ls) failed", strName));
#endif

        SysFreeString(strName);

        if (hr)
            goto Cleanup;

        if (*pchEnd == 0)
        {
            break;
        }
        else if (V_VT(&v) != VT_DISPATCH)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        else if (V_DISPATCH(&v) == 0)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        // We still have tokens and we still have a valid IDispatch, next token please!
        ReplaceInterface(&pDispatch, V_DISPATCH(&v));
        v.Clear();
    }

    Assert(*pchTemp == 0);

    //
    // CONSIDER (michaelw)
    // This is where we could try to detec if the object we've found is a 
    // builtin function or user-defined function.  In the case of a user-defined
    // function we would look for a ._recalcIgnore property that would allow us
    // to ignore this property for recalc purposes
    //

Cleanup:
    if (!hr)
    {
        TraceTag((tagRecalcHost, "resolveName %ls => %08x dispid: %08x", szName, pDispatch, *pDispid));
        *ppDispatch = pDispatch;
    }
    else
    {
        TraceTag((tagRecalcHost, "resolveName %ls failed", szName));
        *ppDispatch = 0;
        ReleaseInterface(pDispatch);
    }
    RRETURN(hr);
}


//------------------------------------
//
// CDoc::CRecalcHost::ResolveNames
//
// Description: IRecalcHost::ResolveNames
//
//------------------------------------

STDMETHODIMP
CDoc::CRecalcHost::ResolveNames(IUnknown *pUnk, DISPID dispid, unsigned cNames, BSTR *pstrNames, IDispatch **ppObjects, DISPID *pDispids)
{
    HRESULT hr;
    IDispatch *pDispatch = 0;
    unsigned i;

    if (pUnk == 0 || ppObjects == 0 || pDispids == 0)
        RRETURN(E_INVALIDARG);

    hr = THR(pUnk->QueryInterface(IID_IDispatch, (LPVOID *)&pDispatch));
    if (hr)
        goto Cleanup;

    for (i = 0 ; i < cNames ; i++)
    {
        hr = THR(resolveName(pDispatch, dispid, pstrNames[i], &ppObjects[i], &pDispids[i]));
        if (hr)
            goto Cleanup;
    }
Cleanup:
    ReleaseInterface(pDispatch);
    return hr;
}

STDMETHODIMP
CDoc::CRecalcHost::SetValue(IUnknown *pUnk, DISPID dispid, VARIANT *pv, BOOL fStyle)
{
    HRESULT hr;
    IDispatch *pDispatch = 0;
    CStyle *pStyle = 0;

    if (fStyle)
    {

        hr = pUnk->QueryInterface(CLSID_CStyle, (LPVOID *)&pStyle); // no AddRef
        if (hr)
            goto Cleanup;

        _pElemSetValue = pStyle->GetElementPtr();
        _dispidSetValue = dispid;
    }

    hr = THR(pUnk->QueryInterface(IID_IDispatch, (LPVOID *)&pDispatch));
    if (hr)
        goto Cleanup;

    hr = THR(SetDispProp(pDispatch, dispid, g_lcidUserDefault, pv));

    TraceTag((tagRecalcHost, "SetValue: put to %08x.%08x failed", pUnk, dispid));

    // Now we need to mark the value as something put there by an expression
    // so that we know to delete it when the expression goes away
    //
    // We only need to do this for style props (ie expressions that came in via the stylesheet)
    // Any other expression will have to be explicitly removed and the removeExpression will
    // remove the value at that time.
    //
    if (SUCCEEDED(hr) && fStyle)
    {
        CAttrArray **ppAA = 0;
        CAttrValue *pAV = 0;

        ppAA = pStyle->GetAttrArray();

        Assert(ppAA && *ppAA);

        pAV = (*ppAA)->Find(dispid, pStyle->IsExpandoDISPID(dispid) ? CAttrValue::AA_Expando : CAttrValue::AA_Attribute);
        
        if (pAV)
            pAV->SetExpression(TRUE);
    }

    _pElemSetValue = 0;

Cleanup:
    ReleaseInterface(pDispatch);
    RRETURN(hr);
}

//------------------------------------------------------------------
//
// Method:      RemoveValue
//
// Description: If the value in the target was put there by an expression
//              remove it.  If not then the value was explicitly put there
//              by someone else and we don't want to or need to remove it
//
//-------------------------------------------------------------------
STDMETHODIMP
CDoc::CRecalcHost::RemoveValue(IUnknown *pUnk, DISPID dispid)
{
    HRESULT hr = S_OK;
    CBase *pBase = (CBase *)pUnk;   // There really should be a better way!
    CAttrArray *pAA = *(pBase->GetAttrArray());
    if (pAA)
    {
        CAttrValue *pAV = pAA->Find(dispid);
        if (pAV && pAV->IsExpression())
        {
            PROPERTYDESC *pDesc = 0;
            hr = THR(pBase->FindPropDescFromDispID(dispid, &pDesc, 0, 0));
            if (!hr)
                hr = THR(pBase->removeAttributeDispid(dispid, pDesc) ? S_OK : E_FAIL);
        }
    }

    RRETURN(hr);
}

STDMETHODIMP
CDoc::CRecalcHost::GetScriptTextAttributes(LPCOLESTR szLanguage, LPCOLESTR pchCode, ULONG cch, LPCOLESTR szDelim, DWORD dwFlags, WORD *pattr)
{
    CScriptHolder *psholder = 0;
    IActiveScriptDebug *pdebug = 0;
    HRESULT hr;

    hr = THR(MyDoc()->_pScriptCollection->GetHolderForLanguage(const_cast<LPOLESTR>(szLanguage), NULL, NULL, NULL, &psholder, NULL));
    if (hr)
        goto Cleanup;

    hr = THR(psholder->GetDebug(&pdebug));
    if (hr)
        goto Cleanup;

    hr = THR(pdebug->GetScriptTextAttributes(pchCode, cch, szDelim, dwFlags, pattr));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pdebug);
    RRETURN(hr);
}

#if DBG == 1
STDMETHODIMP
CDoc::CRecalcHost::GetObjectInfo(IUnknown *pUnk, DISPID dispid, BSTR *pstrID, BSTR *pstrMember, BSTR *pstrTag)
{
    if (pstrID)
        *pstrID = 0;
    if (pstrMember)
        *pstrMember = 0;
    if (pstrTag)
        *pstrTag = 0;

    if (!IsTagEnabled(tagRecalcGetInfo))
        RRETURN(E_FAIL);

    CElement *pElement = 0;
    IHTMLElement *pHTMLElement = 0;
    CStyle *pStyle = 0;
    HRESULT hr;
    BSTR bstrID = 0;

    // try to get the element
    hr = pUnk->QueryInterface(CLSID_CElement, (LPVOID *)&pElement);
    if (FAILED(hr))
    {
        // See if it's a style object and find the element

        hr = pUnk->QueryInterface(CLSID_CStyle, (LPVOID *)&pStyle);
        if (SUCCEEDED(hr))
        {
            pElement = pStyle->GetElementPtr();
        }
    }

    if (SUCCEEDED(hr))
    {
        // We have the element, now get the id, tag and member names

        if (pstrID)
        {
            hr = pElement->PrivateQueryInterface(IID_IHTMLElement, (LPVOID *)&pHTMLElement);
            Assert(!hr);

            IGNORE_HR(pHTMLElement->get_id(&bstrID));

            // No ID?  Try the name
            if (!bstrID || *bstrID == 0)
            {
                IGNORE_HR(pElement->get_name(&bstrID));
            }

            if (pStyle)
            {
                CBufferedStr cbuf;
                cbuf.Set(bstrID);
                cbuf.QuickAppend(_T(".style"));

                *pstrID = SysAllocString(cbuf);
            }
            else
            {
                *pstrID = bstrID;
                bstrID = 0; // prevent this from being freed
            }
        }

        if (pstrTag)
#ifdef VSTUDIO7
            pElement->GettagName(pstrTag);
#else
            pElement->get_tagName(pstrTag);
#endif //VSTUDIO7


        if (pstrMember)
        {
            if (pStyle)
            {
                DISPID expDispid;
                LPCTSTR pszName = 0;

                PROPERTYDESC *ppropdesc = 0;
                if (pStyle->FindPropDescFromDispID(dispid, &ppropdesc, NULL, NULL) == S_OK)
                {
                    pszName = ppropdesc->pstrExposedName ? ppropdesc->pstrExposedName : ppropdesc->pstrName;
                }
                else if (pStyle->IsExpandoDISPID(dispid, &expDispid))
                {
                    pStyle->GetExpandoName(expDispid, &pszName);
                }

                *pstrMember = SysAllocString(pszName);
            }
            else
            {
                pElement->GetMemberName(dispid, pstrMember);
            }
        }
    }

    if (pHTMLElement)
        pHTMLElement->Release();
    if (bstrID)
        FormsFreeString(bstrID);

    RRETURN(hr);
}

int
CDoc::CRecalcHost::Dump(DWORD dwFlags)
{
    extern BOOL RecalcDumpOpen();
    if (!RecalcDumpOpen())
        return 0;

    r_p(_T("<div class=recalchost>Recalc Host<br>\n"));
    r_p(_T("\t<div class=recalcmembers>Members\n"));
    r_p(_T("\t\t<div class=recalcvalues>"));

    r_pp(_pElemSetValue);
    r_pn(_dispidSetValue);
    r_pb(_fRecalcRequested);
    r_pb(_fInRecalc);
    r_pn(_ulSuspend);
    r_p(_T("\t\t</div>\n"));
    r_p(_T("\t</div>\n"));

    if (_pEngine)
    {
        ((CRecalcEngine *)_pEngine)->Dump(dwFlags);
    }

    r_p(_T("</div>\n"));
    if (g_hfileRecalcDump)
    {
        CloseHandle(g_hfileRecalcDump);
        g_hfileRecalcDump = INVALID_HANDLE_VALUE;
    }
    return 0;
}
#endif


//+----------------------------------------------------------------------------
//
//  Function:   CElement::GetCanonicalProperty
//
//  Synopsis:   Returns the canonical pUnk/dispid pair for a particular dispid
//              Used by the recalc engine to catch aliased properties.
//
//  Parameters: ppUnk will contain the canonical object
//              pdispid will contain the canonical dispid
//
//  Returns:    S_OK if successful
//              S_FALSE if property has no alias
//
//-----------------------------------------------------------------------------

HRESULT
CElement::GetCanonicalProperty(DISPID dispid, IUnknown **ppUnk, DISPID *pdispid)
{
    HRESULT hr = S_OK;

    switch (dispid)
    {
    case DISPID_IHTMLELEMENT2_CLIENTLEFT:
        hr = THR(PrivateQueryInterface(IID_IUnknown, (LPVOID *) ppUnk));
        *pdispid = DISPID_IHTMLELEMENT_OFFSETLEFT;
        break;
    case DISPID_IHTMLELEMENT2_CLIENTTOP:
        hr = THR(PrivateQueryInterface(IID_IUnknown, (LPVOID *) ppUnk));
        *pdispid = DISPID_IHTMLELEMENT_OFFSETTOP;
        break;
    case DISPID_IHTMLELEMENT2_CLIENTWIDTH:
        hr = THR(PrivateQueryInterface(IID_IUnknown, (LPVOID *) ppUnk));
        *pdispid = DISPID_IHTMLELEMENT_OFFSETWIDTH;
        break;
    case DISPID_IHTMLELEMENT2_CLIENTHEIGHT:
        hr = THR(PrivateQueryInterface(IID_IUnknown, (LPVOID *) ppUnk));
        *pdispid = DISPID_IHTMLELEMENT_OFFSETHEIGHT;
        break;
    default:
        *ppUnk = 0;
        *pdispid = 0;
        hr = S_FALSE;
    }
    
    RRETURN1(hr, S_FALSE);
}
