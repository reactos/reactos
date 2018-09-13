// ---------------------------------------------------------
//
// Microsoft Trident
// Copyright Microsoft corporation 1998
//
// File: recalc.cxx
//
// The Trident recalc engine: CRecalcEngine
//
// ---------------------------------------------------------

#include "headers.hxx"

#ifndef X_RECALC_H
#define X_RECALC_H
#include "recalc.h"
#endif

#ifndef X_RECALC_HXX_
#define X_RECALC_HXX_
#include "recalc.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include <mshtmdid.h>
#endif

MtDefine(Recalc, Mem, "Recalc")
MtDefine(CRecalcEngine, Recalc, "CRecalcEngine")
MtDefine(CRecalcObject, Recalc, "CRecalcObject")
MtDefine(CRecalcProperty, Recalc, "CRecalcProperty")

#ifndef RECALC_USE_SCRIPTDEBUG
MtDefine(CScriptAuthorHolder, Recalc, "CScriptAuthorHolder")
#endif

DeclareTag(tagRecalcEngine, "Recalc Engine", "Recalc Engine trace")
DeclareTag(tagRecalcMemory, "Recalc Memory", "Track recalc memory use")
DeclareTag(tagRecalcDump, "Recalc Dump", "Enable dumping of recalc graph")
DeclareTag(tagRecalcEval, "Recalc Eval", "Trace expression evaluation in recalc")

ExternTag(tagRecalcStyle);

#if DBG == 1
unsigned CRecalcProperty::s_serialNumber = 0;
unsigned CRecalcObject::s_serialNumber = 0;
unsigned CRecalcEngine::s_serialNumber = 0;
#ifndef RECALC_USE_SCRIPTDEBUG
unsigned CScriptAuthorHolder::s_serialNumber = 0;
#endif
#endif

#ifndef RECALC_USE_SCRIPTDEBUG
//---------------------------------------------------------------
//
// Method:      CScriptAuthorHolder::CScriptAuthorHolder
//
// Description: the constructor
//
//---------------------------------------------------------------
CScriptAuthorHolder::CScriptAuthorHolder()
{
#if DBG == 1
    _serialNumber = s_serialNumber++;
    TraceTag((tagRecalcMemory, "Constructing CScriptAuthorHolder # %d p=%08x", _serialNumber, this));
#endif
}

//---------------------------------------------------------------
//
// Function:    CScriptAuthorHolder::~CScriptAuthorHolder
//
// Description: the destroyer
//
//---------------------------------------------------------------}
CScriptAuthorHolder::~CScriptAuthorHolder()
{
    Assert(_pScriptAuthor == 0);
    TraceTag((tagRecalcMemory, "Destroying CScriptAuthorHolder # %d p=%08x", _serialNumber, this));
}

//---------------------------------------------------------------
//
// Function:    CScriptAuthorHolder::Init
//
// Description: Initialize the holder, actually CoCreates the engine
//
//---------------------------------------------------------------
HRESULT
CScriptAuthorHolder::Init(LPOLESTR szLanguage)
{
    CLSID clsid;
    HRESULT hr;
    CStr strTemp;

    hr = THR(_sLanguage.Set(szLanguage));
    if (hr)
        goto Cleanup;

    hr = THR(strTemp.Set(_sLanguage));
    if (hr)
        goto Cleanup;

    hr = THR(strTemp.Append(_T(" Author")));
    if (hr)
        goto Cleanup;

    hr = THR(CLSIDFromProgID(strTemp, &clsid));
    if (hr)
        goto Cleanup;

    hr = THR(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IActiveScriptAuthor, (LPVOID *)&_pScriptAuthor));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------
//
// Function:    CScriptAuthorHolder::Compare
//
// Description: Are you the right engine for me?
//
//---------------------------------------------------------------
BOOL
CScriptAuthorHolder::Compare(LPOLESTR szLanguage)
{
    return (_tcsicmp(szLanguage, _sLanguage) == 0);
}

//---------------------------------------------------------------
//
// Function:    CScriptAuthorHolder::Detach
//
// Description: 'Hallo, my name is Diego Montoya, you killed my
//              father, prepare to die.'  Time to free all resources
//
//---------------------------------------------------------------
void
CScriptAuthorHolder::Detach()
{
    TraceTag((tagRecalcMemory, "Destroying CScriptAuthorHolder # %d p=%08x", _serialNumber, this));

    ClearInterface(&_pScriptAuthor);
}

#endif

//---------------------------------------------------------------
//
// Function:    CreateRecalcEngine(IUnknown
//
// Description: A helper used by our class factory to get things going
//
//---------------------------------------------------------------
STDMETHODIMP
CreateRecalcEngine(IUnknown *pUnkOuter, IUnknown **ppUnk)
{
    HRESULT hr;

    Assert(ppUnk);

    if (pUnkOuter != NULL)
    {
        hr = CLASS_E_NOAGGREGATION;
        *ppUnk = 0;
    }
    else
    {
        *ppUnk = static_cast<IUnknown *>(static_cast<IRecalcEngine *>(new CRecalcEngine()));
        hr = *ppUnk ? S_OK : E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::CRecalcEngine
//
// Description: The constructor
//
//---------------------------------------------------------------}
CRecalcEngine::CRecalcEngine() : _ulRefs(1)
{
    _iFoundLast = -1;
    _pUnkFoundLast = 0;

#if DBG == 1
    _serialNumber = s_serialNumber++;
    TraceTag((tagRecalcDetail, "Constructing CRecalcEngine # %d p=%08x", _serialNumber, this));
#endif
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::~CRecalcEngine
//
// Description: The destructor
//
//---------------------------------------------------------------}
CRecalcEngine::~CRecalcEngine()
{
    TraceTag((tagRecalcDetail, "Destroying CRecalcEngine # %d p=%08x", _serialNumber, this));
    Assert(_objects.Size() == 0);
    Assert(_ulRefs == 0);
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::QueryInterface
//
// Description: IUnknown::QueryInterface
//
//---------------------------------------------------------------
STDMETHODIMP
CRecalcEngine::QueryInterface(REFIID iid, LPVOID *ppv)
{
    if (ppv == NULL)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IRecalcEngine *)this, IUnknown)
        QI_INHERITS((IRecalcEngine *)this, IRecalcEngine)
        QI_INHERITS(this, IObjectWithSite)
    }
    if (*ppv == NULL)
        RRETURN(E_NOINTERFACE);

    (*(IRecalcEngine **)ppv)->AddRef();
    return S_OK;
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::AddRef
//
// Description: IUnknown::AddRef
//
//---------------------------------------------------------------
STDMETHODIMP_(ULONG)
CRecalcEngine::AddRef()
{
    return ++_ulRefs;
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::Release
//
// Description: IUnknown::Release
//
//---------------------------------------------------------------
STDMETHODIMP_(ULONG)
CRecalcEngine::Release()
{
    Assert(_ulRefs > 0);

    unsigned long ulRefs = --_ulRefs;
    if (ulRefs == 0)
    {
        Assert(_pHost == 0 && "Recalc Engine Released before SetSite(NULL)");
        Assert(_objects.Size() == 0);
#ifndef RECALC_USE_SCRIPTDEBUG
        Assert(_authors.Size() == 0);
#endif
        delete this;
    }
    return ulRefs;
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::Detach
//
// Description: Blows away all objects, unwinding any ref loops as we go
//
//---------------------------------------------------------------
void
CRecalcEngine::Detach()
{
    CRecalcObject **ppObject;
#ifndef RECALC_USE_SCRIPTDEBUG
    CScriptAuthorHolder **ppAuthor;
#endif

    int i;

    for (i = _objects.Size() , ppObject = _objects; i > 0 ; i-- , ppObject++)
    {
        (*ppObject)->Detach();
        (*ppObject)->Release();
    }
    _objects.DeleteAll();

#ifndef RECALC_USE_SCRIPTDEBUG
    for (i = _authors.Size() , ppAuthor = _authors ; i > 0 ; i-- , ppAuthor++)
    {
        (*ppAuthor)->Detach();
        delete (*ppAuthor);
    }
    _authors.DeleteAll();
#endif
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::SetSite
//
// Description: IObjectWithSite::SetSite
//
//---------------------------------------------------------------
STDMETHODIMP
CRecalcEngine::SetSite(IUnknown *pUnk)
{
    HRESULT hr;

    // We only expect to be called twice:
    // once at setup (!_pUnk && pUnk) and
    // once at teardown (_pUnk && !pUnk).
    //
    Assert((!_pHost && pUnk) || (_pHost && !pUnk));

    if (_pHost)
    {
        Detach();
        ClearInterface(&_pHost);
        WHEN_DBG(ClearInterface(&_pDebugHost);)
    }

    Assert(_objects.Size() == 0);
#ifndef RECALC_USE_SCRIPTDEBUG
    Assert(_authors.Size() == 0);
#endif

    if (pUnk)
    {
        hr = THR(pUnk->QueryInterface(IID_IRecalcHost, (LPVOID *)&_pHost));
#if DBG == 1
        if (SUCCEEDED(hr))
            pUnk->QueryInterface(IID_IRecalcHostDebug, (LPVOID *)&_pDebugHost);
#endif
    }
    else
        hr = S_OK;

    RRETURN(hr);
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::GetSite
//
// Description: IObjectWithSite::GetSite
//
//---------------------------------------------------------------
STDMETHODIMP
CRecalcEngine::GetSite(REFIID riid, LPVOID *ppUnk)
{
    if (ppUnk == 0)
        RRETURN(E_INVALIDARG);
    *ppUnk = 0;
    if (!_pHost)
        RRETURN(E_FAIL);

    RRETURN(_pHost->QueryInterface(riid, ppUnk));
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::RemoveObject
//
// Description: Removes an object from the recalc engine
//
//---------------------------------------------------------------
void
CRecalcEngine::RemoveObject(CRecalcObject *pObject)
{
    int i = _objects.Find(pObject);
    if (i >= 0)
    {
        _objects[i]->Detach();
        _objects.ReleaseAndDelete(i);
    }
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::RecalcAll
//
// Description: IRecalcEngine::RecalcAll
//
//              Recalculates all dirty expressions.  Ignores dirty
//              state if fForce is set.
//
//---------------------------------------------------------------
STDMETHODIMP
CRecalcEngine::RecalcAll(BOOL fForce)
{
    HRESULT hr = S_OK;
    int i;

    TraceTag((tagRecalcEngine, "CRecalcEngine::RecalcAll(%d)", fForce));

    // Find any properties with dirty dependencies and update them.
    // Although resolving dependencies can change the graph (by adding
    // new property objects), none of the new objects should require
    // dependency checking

    WHEN_DBG(int cDirtyDeps = _dirtyDeps.Size());

    _fInDeps = TRUE;
    for (i = 0 ; i < _dirtyDeps.Size() ; i++)
    {
        _dirtyDeps[i]->UpdateDependencies();
    }

    _fInDeps = FALSE;

    Assert(cDirtyDeps == _dirtyDeps.Size());

    _dirtyDeps.DeleteAll();

    _fInRecalc = TRUE;

    // Now find anyone who needs recalc
    for (i = 0; i < _objects.Size(); i++)
    {
        hr = THR(_objects[i]->RecalcAll(fForce));
        if (hr)
            break;
    }

    _fInRecalc = FALSE;

    RRETURN(hr);
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::SetExpression
//
// Description: IRecalcEngine::SetExpression
//
//---------------------------------------------------------------
STDMETHODIMP
CRecalcEngine::SetExpression(IUnknown *pUnk, DISPID dispid, LPOLESTR szExpression, LPOLESTR szLanguage)
{
    CRecalcProperty *pProperty = 0;
    HRESULT hr;

    if (_fInRecalc)
        RRETURN(E_UNEXPECTED);

#if DBG == 1
    if (_fInRecalc)
        TraceTag((tagRecalcEngine, "SetExpression while in recalc: %ls", szExpression));

    if (_fInDeps)
        TraceTag((tagRecalcEngine, "SetExpression while in recalc deps: %ls", szExpression));
#endif

    hr = THR(FindProperty(pUnk, dispid, TRUE, &pProperty));
    if (hr)
        goto Cleanup;

    hr = THR(pProperty->SetExpression(szExpression, szLanguage));
    if (hr)
        goto Cleanup;

Cleanup:
    if (hr)
    {
        if (pProperty)
            pProperty->DeleteIfEmpty();
    }

    RRETURN(hr);
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::GetExpression
//
// Description: IRecalcEngine::GetExpression
//
// Returns:     S_OK or S_FALSE
//---------------------------------------------------------------
STDMETHODIMP
CRecalcEngine::GetExpression(IUnknown *pUnk, DISPID dispid, BSTR *pstrExpression, BSTR *pstrLanguage)
{
    CRecalcProperty *pProperty;

    HRESULT hr = THR(FindProperty(pUnk, dispid, FALSE, &pProperty));
    if (!hr)
    {
        hr = THR(pProperty->GetExpression(pstrExpression, pstrLanguage));
    }

    if (hr == S_FALSE)
    {
        *pstrExpression = 0;
        *pstrLanguage = 0;
    }
    RRETURN1(hr, S_FALSE);
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::ClearExpression
//
// Description: IRecalcEngine::ClearExpression
//
//---------------------------------------------------------------
STDMETHODIMP
CRecalcEngine::ClearExpression(IUnknown *pUnk, DISPID dispid)
{
    CRecalcProperty *pProperty;

    if (_fInRecalc)
        RRETURN(E_UNEXPECTED);

    HRESULT hr = THR(FindProperty(pUnk, dispid, FALSE, &pProperty));
    if (!hr)
    {
        hr = THR(pProperty->ClearExpression());
    }

    RRETURN1(hr, S_FALSE);
}


//---------------------------------------------------------------
//
// Function:    CRecalcEngine::BeginStyle
//
// Description: IRecalcEngine::BeginStyle
//
//---------------------------------------------------------------
STDMETHODIMP
CRecalcEngine::BeginStyle(IUnknown *pUnk)
{
    HRESULT hr = E_INVALIDARG;
    CRecalcObject *pObject;

    Assert(!_fInRecalc);

    hr = THR(FindObject(pUnk, TRUE, &pObject));
    if (hr)
        goto Cleanup;

    hr = THR(pObject->BeginStyle());
    if (hr)
        goto Cleanup;
Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::BeginStyle
//
// Description: IRecalcEngine::EndStyle
//
//---------------------------------------------------------------
STDMETHODIMP
CRecalcEngine::EndStyle(IUnknown *pUnk)
{
    HRESULT hr = E_INVALIDARG;
    CRecalcObject *pObject;

    hr = THR(FindObject(pUnk, FALSE, &pObject));
    if (hr)
        goto Cleanup;

    hr = THR(pObject->EndStyle());
    if (hr)
        goto Cleanup;
Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------
//
// Function:    CRecalcEngine::Find
//
// Description: Find an object by pUnk (returns the index)
//
//---------------------------------------------------------------
int
CRecalcEngine::Find(IUnknown *pUnk)
{
    TraceTag((tagRecalcDetail, "Looking for %08x", pUnk));

    CRecalcObject **ppObject;
    int i;

    if ((pUnk == _pUnkFoundLast) && (_iFoundLast < _objects.Size()) && (_objects[_iFoundLast]->GetUnknown() == pUnk))
        return _iFoundLast;

    for (i = _objects.Size() , ppObject = _objects ; i > 0 ; i-- , ppObject++)
    {
        if ((*ppObject)->GetUnknown() == pUnk)
        {
            TraceTag((tagRecalcDetail, "Found at index %d", ppObject - _objects));
            _iFoundLast = i;
            _pUnkFoundLast = pUnk;

            return ppObject - _objects;
        }
    }

    return -1;
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::FindObject
//
// Description: Finds an object (by _pUnk), optionally creating it if not found
//
//---------------------------------------------------------------
HRESULT
CRecalcEngine::FindObject(IUnknown *pUnk, BOOL fCreate, CRecalcObject **ppObject, unsigned *pIndex)
{
    HRESULT hr = S_FALSE;

    CRecalcObject *pObject = 0;
    int i = Find(pUnk);
    
    if (i >= 0)
    {
        pObject = _objects[i];
        hr = S_OK;
    }
    else if (fCreate)
    {
        pObject = new CRecalcObject(this, pUnk);
        if (!pObject)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_objects.Append(pObject));
        if (hr)
        {
            ClearInterface(&pObject);
            goto Cleanup;
        }

        Assert(_objects.Size() > 0);

        i = _objects.Size() - 1;

        Assert(_objects[i] == pObject);
    }

Cleanup:
    *ppObject = pObject;
    if (pIndex)
        *pIndex = i;
    RRETURN1(hr, S_FALSE);
}

//---------------------------------------------------------------
//
// Function:    CRecalcEngine::FindProperty
//
// Description: Find a property (optionally creating the object
//              the property along the way
//
//---------------------------------------------------------------
HRESULT
CRecalcEngine::FindProperty(IUnknown *pUnk, DISPID dispid, BOOL fCreate, CRecalcProperty **ppProperty)
{
    Assert(ppProperty);
    HRESULT hr;

    CRecalcProperty *pProperty = 0;
    CRecalcObject *pObject = 0;

    hr = THR(FindObject(pUnk, fCreate, &pObject));
    if (!hr)
    {
        Assert(pObject);
        hr = THR(pObject->FindProperty(dispid, fCreate, &pProperty));
        if (!hr)
        {
            *ppProperty = pProperty;

            hr = S_OK;
        }
    }

    RRETURN1(hr, S_FALSE);
}

#ifndef RECALC_USE_SCRIPTDEBUG
//---------------------------------------------------------------
//
// Function:    CRecalcEngine::GetScriptAuthor
//
// Description: A helper to get the IActiveScriptAuthor interface
//
//---------------------------------------------------------------
HRESULT
CRecalcEngine::GetScriptAuthor(LPOLESTR szLanguage, IActiveScriptAuthor **ppScriptAuthor)
{
    Assert(ppScriptAuthor);

    HRESULT hr ;

    CScriptAuthorHolder **ppScriptHolder;
    int i;

    for (i = _authors.Size() , ppScriptHolder = _authors ; i > 0 ; i-- , ppScriptHolder++)
    {
        if ((*ppScriptHolder)->Compare(szLanguage))
        {
            *ppScriptAuthor = (*ppScriptHolder)->ScriptAuthor();
            RRETURN(S_OK);
        }
    }

    CScriptAuthorHolder *psh = new CScriptAuthorHolder();
    if (!psh)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    hr = THR(psh->Init(szLanguage));
    if (hr)
        goto Error;

    hr = THR(_authors.Append(psh));
    if (hr)
        goto Error;

    *ppScriptAuthor = psh->ScriptAuthor();

Cleanup:
    RRETURN(hr);
Error:
    if (psh)
    {
        psh->Detach();
        delete psh;
    }
    goto Cleanup;
}

#endif

HRESULT
CRecalcEngine::AddDependencyUpdateRequest(CRecalcProperty *pProperty)
{
    Assert(!_fInDeps);
    RRETURN(_dirtyDeps.Append(pProperty));
}

HRESULT
CRecalcEngine::RemoveDependencyUpdateRequest(CRecalcProperty *pProperty)
{
    RRETURN(_dirtyDeps.DeleteByValue(pProperty) ? S_OK : E_INVALIDARG);
}


//---------------------------------------------------------------------------------------
//
// CRecalcObject
//

//---------------------------------------------------------------
//
// Function:    CRecalcObject::CRecalcObject
//
// Description: The constructor
//
// Notes:       CRecalcObject lifetime is controlled by refcount
//              However, its lifetime in the recalc engine is
//              controlled by the number of properties.  When the
//              number of properties drops to zero, the object
//              removes itself from the engine.  When the engine
//              releases the object it should go away.  Errant
//              holders of IPropertyNotifySink will cause the
//              CRecalcObject to live on.
//
//---------------------------------------------------------------
CRecalcObject::CRecalcObject(CRecalcEngine *pEngine, IUnknown *pUnk) : _pEngine(pEngine) , _pUnk(pUnk) , _ulRefs(1)
{
    Assert(_pUnk);
    Assert(_pEngine);

#if DBG == 1
    _serialNumber = s_serialNumber++;
    TraceTag((tagRecalcMemory, "Constructing CRecalcObject # %d p=%08x", _serialNumber, this));

    GetDebugHost()->GetObjectInfo(_pUnk, 0, &_bstrID, 0, &_bstrTag);

    if (_bstrID == 0)
        _bstrID = SysAllocString(_T("{no-id}"));

    if (_bstrTag == 0)
        _bstrTag = SysAllocString(_T("{no-tag}"));

#endif

    _iFoundLast = -1;
    _dispidFoundLast = DISPID_UNKNOWN;

    _pUnk->AddRef();
    if (THR(ConnectSink(_pUnk, IID_IPropertyNotifySink, (IPropertyNotifySink *)this, &_dwCookie)) == S_OK)
        _fGotSink = TRUE;

}

//---------------------------------------------------------------
//
// Function:    CRecalcObject::~CRecalcObject
//
// Description: The destructor
//
//---------------------------------------------------------------}
CRecalcObject::~CRecalcObject()
{
    TraceTag((tagRecalcMemory, "Destroying CRecalcObject # %d p=%08x", _serialNumber, this));
    Assert(_properties.Size() == 0);
    Assert(_pUnk == 0);
    Assert(!_fGotSink);
    Assert(_ulRefs == 0);
#if DBG == 1
    FormsFreeString(_bstrID);
    FormsFreeString(_bstrTag);
#endif
}

//---------------------------------------------------------------
//
// Function:    CRecalcObject::Detach
//
// Description: Time to cleanup and release all resources.
//
//---------------------------------------------------------------
void
CRecalcObject::Detach()
{
    TraceTag((tagRecalcMemory, "Detaching CRecalcObject # %d p=%08x", _serialNumber, this));

    Assert(_pUnk);
    Assert(_pEngine);

    int i;
    CRecalcProperty **ppProperty;

    for (i = _properties.Size() , ppProperty = _properties ; i > 0 ; i-- , ppProperty++)
    {
        delete (*ppProperty);
    }

    _properties.DeleteAll();

    if (_fGotSink)
    {
        IGNORE_HR(DisconnectSink(_pUnk, IID_IPropertyNotifySink, &_dwCookie));
        Assert(_dwCookie == 0);
        _fGotSink = FALSE;
    }

    ClearInterface(&_pUnk);
}

//---------------------------------------------------------------
//
// Function:    CRecalcObject::QueryInterface
//
// Description: IUnknown::QueryInterface
//
//---------------------------------------------------------------
STDMETHODIMP
CRecalcObject::QueryInterface(REFIID iid, LPVOID *ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPropertyNotifySink *)this, IUnknown)
        QI_INHERITS(this, IPropertyNotifySink)
    }
    if (*ppv == NULL)
        RRETURN(E_NOINTERFACE);

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}

//---------------------------------------------------------------
//
// Function:    CRecalcObject::AddRef
//
// Description: IUnknown::AddRef
//
//---------------------------------------------------------------
STDMETHODIMP_(ULONG)
CRecalcObject::AddRef()
{
    return ++_ulRefs;
}

//---------------------------------------------------------------
//
// Function:    CRecalcObject::Release
//
// Description: IUnknown::Release
//
//---------------------------------------------------------------
STDMETHODIMP_(ULONG)
CRecalcObject::Release()
{
    Assert(_ulRefs > 0);

    unsigned long ulRefs = --_ulRefs;
    if (ulRefs == 0)
    {
        delete this;
    }
    return ulRefs;
}

//---------------------------------------------------------------
//
// Function:    CRecalcObject::OnChanged
//
// Description: IPropertyNotifySink::OnChanged
//
//---------------------------------------------------------------
STDMETHODIMP
CRecalcObject::OnChanged(DISPID dispid)
{
    Assert(_properties.Size());

    CRecalcProperty *pProperty = 0;
    if (FindProperty(dispid, FALSE, &pProperty) == S_OK)
    {
        Assert(pProperty);
        pProperty->OnChanged();
    }
    RRETURN(S_OK);
}

//---------------------------------------------------------------
//
// Function:    CRecalcObject::OnRequestEdit
//
// Description: IPropertyNotifySink::OnRequestEdit
//
//---------------------------------------------------------------
STDMETHODIMP
CRecalcObject::OnRequestEdit(DISPID dispid)
{
    // Don't care
    return S_OK;
}

//---------------------------------------------------------------
//
// Function:    CRecalcObject::FindProperty
//
// Description: Find a property by dispid, optionally create it
//
//---------------------------------------------------------------
HRESULT
CRecalcObject::FindProperty(DISPID dispid, BOOL fCreate, CRecalcProperty **ppProperty)
{
    HRESULT hr = S_FALSE;

    CRecalcProperty *pProperty = 0;
    int i = Find(dispid);

    if (i >= 0)
    {
        pProperty = _properties[i];
        hr = S_OK;
    }
    else if (fCreate)
    {
        hr = THR(CRecalcProperty::CreateProperty(this, dispid, &pProperty));
        if (hr)
            goto Cleanup;

        Assert(pProperty);

        hr = THR(_properties.Append(pProperty));
        if (hr)
            goto Cleanup;
Cleanup:
        if (hr)
        {
            pProperty->DeleteIfEmpty();
            pProperty = 0;
        }

    }

    *ppProperty = pProperty;
    RRETURN1(hr, S_FALSE);
}

//---------------------------------------------------------------
//
// Function:    CRecalcObject::Find
//
// Description: Find a property by dispid
//
//---------------------------------------------------------------
int
CRecalcObject::Find(DISPID dispid)
{
    CRecalcProperty **ppProperty;
    unsigned i;

    if ((dispid == _dispidFoundLast) && (_iFoundLast < _properties.Size()) && (_properties[_iFoundLast]->GetDispid() == dispid))
        return _iFoundLast;

    for (i = _properties.Size() , ppProperty = _properties ; i > 0 ; i-- , ppProperty++)
    {
        if ((*ppProperty)->GetDispid() == dispid)
        {
            return ppProperty - _properties;
        }
    }
    return -1;
}

//---------------------------------------------------------------
//
// Function:    CRecalcObject::RecalcAll
//
// Description: Recalc all of your properties
//              Only properties with no dependents are recalc'd.
//              They will in turn call their dependencies before
//              trying to eval.  Depth first traversal of a tree
//---------------------------------------------------------------
HRESULT
CRecalcObject::RecalcAll(BOOL fForce)
{
    int i;

    TraceTag((tagRecalcEngine, "RecalcAll: object: %ls", GetID()));

    //
    // Because SetExpression (or worse, removeExpression) could
    // be called during recalc, we iterate the old fashioned way.
    // At worst we'll 
    // Iterating using the index ensures that additions to the
    // recalc dep graph doesn't cause any problems.  (SetExpression can 
    // be called during recalc).  This 
    //
    //
    for (i = 0 ; i < _properties.Size() ; i++)
    {
        CRecalcProperty *pProperty = _properties[i];

        if (pProperty->IsTopLevel())
        {
            // Just because one expression fails doesn't mean we should
            // stop trying others.
            //
            // BUGBUG (michaelw) It would be nice if I know if the user
            // has said "No" when asked if they want to keep running
            // scripts.
            //
            IGNORE_HR(pProperty->Eval(fForce));
        }
    }

    return S_OK;
}

//---------------------------------------------------------------
//
// Function:    CRecalcObject::RemoveProperty
//
// Description: This property is no longer needed
//
//---------------------------------------------------------------
void
CRecalcObject::RemoveProperty(CRecalcProperty *pProperty)
{
    int i = _properties.Find(pProperty);

    if (i >= 0)
    {
        delete _properties[i];
        _properties.Delete(i);

        if (!_fDoingStyle && (_properties.Size() == 0))
            _pEngine->RemoveObject(this);
    }
}

//---------------------------------------------------------------
//
// Function:    CRecalcObject::BeginStyle
//
// Description: Any calls to SetExpression are for style props
//
//---------------------------------------------------------------
HRESULT
CRecalcObject::BeginStyle()
{
    Assert(!_fDoingStyle);

    _fDoingStyle = TRUE;
#if DBG == 1
    for (int i = 0 ; i < _properties.Size() ; i++)
        if (_properties[i]->IsStyleProp())
            Assert(!_properties[i]->CheckAndClearStyleSet());
#endif
    return S_OK;
}

//---------------------------------------------------------------
//
// Function:    CRecalcObject::EndStyle
//
// Description: After all the style expressions have been set,
//              remove anything left over
//
//---------------------------------------------------------------
HRESULT
CRecalcObject::EndStyle()
{
    Assert(_fDoingStyle);

    // Because we are removing elements from the arraw as we go,
    // This array is enumerated using an index only.

    for (int i = _properties.Size() - 1 ; i >= 0 ; i--)
    {
        CRecalcProperty *pProp = _properties[i];

        if (pProp->IsStyleProp() && !pProp->CheckAndClearStyleSet())
        {
            TraceTag((tagRecalcStyle, "Removing style expression: _pUnk: %08x _dispid: %08x _expr: %ls", pProp->GetUnknown(), pProp->GetDispid(), pProp->GetExpression()));
            IGNORE_HR(pProp->ClearExpression());
        }
    }

    _fDoingStyle = FALSE;

    if (_properties.Size() == 0)
        _pEngine->RemoveObject(this);

    return S_OK;
}


//---------------------------------------------------------------------------------------------------
//
// CRecalcProperty::CreateProperty
//
// The only place where property objects get created.
//
// This function ensures that the property is properly constructed and initialized.
//
// It may need to create the canonical property at the same time.
//
//---------------------------------------------------------------------------------------------------


HRESULT CRecalcProperty::CreateProperty(CRecalcObject *pObject, DISPID dispid, CRecalcProperty **ppProperty)
{
    CRecalcProperty *pProperty = 0;
    CRecalcProperty *pPropertyCanonical = 0;
    IUnknown *pUnkCanonical = NULL;
    DISPID dispidCanonical;

    HRESULT hr;

    pProperty = new CRecalcProperty(pObject, dispid);
    if (!pProperty)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(GetCanonicalProperty(pObject->GetUnknown(), dispid, &pUnkCanonical, &dispidCanonical));
    if (hr == S_FALSE)
    {
        // This property is not aliased
        hr = S_OK;
    }
    else
    {
        // The property is aliased, find and create it's canonical property

        pProperty->_fAlias = TRUE;

        hr = THR(pObject->GetEngine()->FindProperty(pUnkCanonical, dispidCanonical, TRUE, &pPropertyCanonical));
        if (hr)
            goto Cleanup;

        hr = THR(pPropertyCanonical->AddAlias(pProperty));
        if (hr)
            goto Cleanup;

        pProperty->_pPropertyCanonical = pPropertyCanonical;
    }

Cleanup:
    if (hr)
    {
        if (pPropertyCanonical)
            pPropertyCanonical->DeleteIfEmpty();
        if (pProperty)
            pProperty->DeleteIfEmpty();

        *ppProperty = 0;
    }
    else
        *ppProperty = pProperty;

    ReleaseInterface(pUnkCanonical);
    RRETURN(hr);
}


//-----------------------------------------------
//
// Method:      CRecalcProperty::CRecalcProperty
//
// Description: Constructor
//
//-----------------------------------------------
CRecalcProperty::CRecalcProperty(CRecalcObject *pObject, DISPID dispid) : _pObject(pObject) , _dispid(dispid)
{
#if DBG == 1
    _serialNumber = s_serialNumber++;
    TraceTag((tagRecalcMemory, "Constructing CRecalcProperty # %d p=%08x", _serialNumber, this));
    GetDebugHost()->GetObjectInfo(GetUnknown(), _dispid, NULL, &_bstrName, NULL);

    if (_bstrName == 0)
        _bstrName = SysAllocString(_T("{no-name}"));

#endif

    _fNoNotify = !_pObject->GotSink();

}

//-----------------------------------------------
//
// Method:      CRecalcProperty::
//
// Description: Destructor
//
//-----------------------------------------------
CRecalcProperty::~CRecalcProperty()
{
    TraceTag((tagRecalcMemory, "Destroying CRecalcProperty # %d p=%08x", _serialNumber, this));

    if (_fDirtyDeps)
        GetEngine()->RemoveDependencyUpdateRequest(this);

    ClearInterface(&_pdispThis);
    ClearInterface(&_pdispExpression);
#if DBG == 1
    FormsFreeString(_bstrName);
#endif
}


//-----------------------------------------------
//
// Method:      CRecalcProperty::OnChanged
//
// Description: IPropertyNotifySink::OnChanged (as delegated by CRecalcObject)
//
//-----------------------------------------------

void
CRecalcProperty::OnChanged()
{
    TraceTag((tagRecalcDetail, "OnChange() this: %08x _pUnk: %08x _dispid: %08x has changed", this, _pObject->GetUnknown(), _dispid));

    if (_fSetValue)
    {
        // This is an expected prop change.
#if DBG == 1
        _cChanged++;
#endif
    }
/*
 * No more automatic removal of expressions!
 *
 * Although this is the "right" way, it isn't
 * practical to make it work for all cases.
 * The case where a property is set to its 
 * current value doesn't always fire a prop
 * change so we would behave very inconsistently.
 *
    else if (HasExpression())
    {
        ClearExpression();
    }
*/
    else
    {
        SetDirty(TRUE);
    }
}

//-----------------------------------------------
//
// Method:      CRecalcProperty::SetDirty
//
// Description: We're dirty.  Tell our dependents and request a recalc
//
//-----------------------------------------------
void
CRecalcProperty::SetDirty(BOOL fDirty, BOOL fRequestRecalc)
{
    if (_fSetDirty)
    {
        TraceTag((tagRecalcEngine, "SetDirty detected circularity"));
        _fCircular = TRUE;

        return;
    }

    if (IsAlias())
    {
        _pPropertyCanonical->SetDirty(fDirty, fRequestRecalc);

        return;
    }

    // Canonicalize the incoming BOOL and make it an unsigned
    unsigned ufDirty = !!fDirty;

    if (ufDirty != _fDirty)
    {
        _fDirty = ufDirty;

        if (ufDirty)
        {
            Assert(!_fSetDirty);
            _fSetDirty = TRUE;

            NotifyDependents();

            Assert(_fSetDirty);
            _fSetDirty = FALSE;

            if (fRequestRecalc)
                GetHost()->RequestRecalc();
        }
    }
}

//---------------------------------------------------------------
//
// Function:    CRecalcProperty::NotifyDependents
//
// Description: Tell our dependents that we're dirty
//
//---------------------------------------------------------------
void
CRecalcProperty::NotifyDependents()
{
    int i;
    CRecalcProperty **ppProperty;

    Assert(IsCanonical());

    for (i = _dependents.Size() , ppProperty = _dependents ; i > 0 ; i-- , ppProperty++)
        (*ppProperty)->SetDirty(TRUE, FALSE);
}

//-----------------------------------------------
//
// Method:      CRecalcProperty::
//
// Description: Are we needed anymore?
//
//-----------------------------------------------
void
CRecalcProperty::DeleteIfEmpty()
{
    //
    // Any dependents?  Any dependencies?  Any aliases?  An Expression?
    //
    // It is possible for _dependencies.Size() == 0 and _dependencyNames.Size() != 0
    // This happens when there are unresolved names.  We will stick around hoping, waiting...
    //
    if (_dependencyNames.Size() + _dependents.Size() + _dependencies.Size() + _aliases.Size() + _sExpression.Length() == 0)
    {
        if (IsAlias())
        {
            _pPropertyCanonical->RemoveAlias(this);
        }

        _pObject->RemoveProperty(this);
    }
}

//-----------------------------------------------
//
// Method:      CRecalcProperty::AddDependent
//
// Description: Add's a property to the list of dependents.
//              Dependents will be notified whenever this
//              property changes.
//
// Returns:     S_OK        everything is fine
//              S_FALSE     added ok but can't notify on prop change
//
//-----------------------------------------------
HRESULT
CRecalcProperty::AddDependent(CRecalcProperty *pProperty, BOOL *pfNoNotify)
{
    Assert(pProperty);
    Assert(pfNoNotify);

    HRESULT hr;

    *pfNoNotify = _fNoNotify;

    Assert(IsCanonical());

    if (_dependents.Find(pProperty) >= 0)
        hr = S_OK;
    else
    {
        hr = _dependents.Append(pProperty);
    }

    RRETURN(hr);
}

//-----------------------------------------------
//
// Method:      CRecalcProperty::RemoveDependent
//
// Description: Someone doesn't depend on us anymore
//
//-----------------------------------------------
HRESULT
CRecalcProperty::RemoveDependent(CRecalcProperty *pProperty)
{
    HRESULT hr;

    Assert(IsCanonical());

    if (_dependents.DeleteByValue(pProperty))
    {
        hr = S_OK;
        DeleteIfEmpty();
    }
    else
    {
        hr = E_INVALIDARG;
    }

    RRETURN(hr);
}

//---------------------------------------------------
//
// Method:      CRecalcProperty::AddAlias
//
// Description: Add an alias to the list of known aliases
//
//---------------------------------------------------
HRESULT
CRecalcProperty::AddAlias(CRecalcProperty *pProperty)
{
    Assert(pProperty);

   if (_aliases.Find(pProperty) >= 0)
        return S_OK;

    RRETURN(_aliases.Append(pProperty));
}


//---------------------------------------------------
//
// Method:      CRecalcProperty::RemoveAlias
//
// Description: Remove an alias to the list of known aliases
//
//---------------------------------------------------
HRESULT
CRecalcProperty::RemoveAlias(CRecalcProperty *pProperty)
{
    Assert(pProperty);

    if (!_aliases.DeleteByValue(pProperty))
        RRETURN(E_INVALIDARG);

    DeleteIfEmpty();

    return S_OK;
}


//-----------------------------------------------
//
// Method:      CRecalcProperty::AddDependency
//
// Description: We depend on some other property.
//
//-----------------------------------------------
HRESULT
CRecalcProperty::AddDependency(IDispatch *pDispatch, DISPID dispid)
{
    CRecalcProperty *pProperty;
    IUnknown *pUnkDepend = 0;
    DISPID dispidDepend;
    BOOL fNoNotify;
    HRESULT hr;
    int i;

    // First try to get the canonical property
    hr = THR(GetCanonicalProperty((IUnknown *)pDispatch, dispid, &pUnkDepend, &dispidDepend));
    if (FAILED(hr))
        goto Cleanup;
    else if (hr == S_FALSE)
    {
        // This property isn't aliased, use the pUnk
        dispidDepend = dispid;
        hr = THR(pDispatch->QueryInterface(IID_IUnknown, (LPVOID *)&pUnkDepend));
        if (hr)
            goto Cleanup;
    }

    // Do we already have this dependency?

    for (i = 0 ; i < _dependencies.Size() ; i++)
    {
        if ((_dependencies[i]->GetUnknown() == pUnkDepend) && (_dependencies[i]->_dispid == dispidDepend))
        {
            hr = S_OK;
            goto Cleanup;
        }
    }

    // We don't already have this dependency, time to add it.

    hr = THR(GetEngine()->FindProperty(pUnkDepend, dispidDepend, TRUE, &pProperty));
    if (hr)
        goto Cleanup;

    hr = THR(_dependencies.Append(pProperty));
    if (hr)
        goto Cleanup;

    hr = THR(pProperty->AddDependent(this, &fNoNotify));
    if (hr)
        goto Cleanup;

    if (fNoNotify)
        _fNoNotify = TRUE;

Cleanup:
    ReleaseInterface(pUnkDepend);

    RRETURN(hr);
}

//---------------------------------------------------------------
//
// Function:    CRecalcProperty::RemoveAllDependencies
//
// Description: Clean out the bunch of them, we depend on nobody!
//
//---------------------------------------------------------------
HRESULT
CRecalcProperty::RemoveAllDependencies()
{
    HRESULT hr = S_OK;
    int i;
    CRecalcProperty **ppProperty;

    for (i = _dependencies.Size() , ppProperty = _dependencies ; i > 0 ; i-- , ppProperty++)
    {
        hr = THR((*ppProperty)->RemoveDependent(this));
        if (hr)
            goto Cleanup;
    }

    _dependencies.DeleteAll();

    _fNoNotify = !_pObject->GotSink();
Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------
//
// Method:      CRecalcProperty::ClearExpression
//
// Description: Clear's the expression, may delete the object
//
//-----------------------------------------------
HRESULT
CRecalcProperty::ClearExpression()
{
    Assert(!GetEngine()->InRecalc());
    // Tell the host to remove the attribute value
    GetHost()->RemoveValue(GetUnknown(), _dispid);
    HRESULT hr = clearExpressionHelper();
    if (hr)
        goto Cleanup;

    DeleteIfEmpty();

Cleanup:
    RRETURN(hr);
}

HRESULT
CRecalcProperty::clearExpressionHelper()
{
    Assert(!_fSetDirty);
    Assert(!_fSetValue);
    Assert(!_fInEval);
    Assert(!GetEngine()->InRecalc());

    HRESULT hr = THR(RemoveAllDependencies());
    if (hr)
        goto Cleanup;

    _dependencyNames.DeleteAll();

    _sExpression.Free();
    _sLanguage.Free();
    _sExpressionNames.Free();

    _fError = FALSE;

    ClearInterface(&_pdispExpression);
    ClearInterface(&_pdispThis);

    _fCircular = FALSE;

    if (IsAlias())
        _pPropertyCanonical->_pPropertyExpression = NULL;
    else
        _pPropertyExpression = NULL;

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------
//
// Method:      CRecalcProperty::EvalExpression
//
// Description: The real McCoy.  This is where all
//              the magic happens.  This is it.  Hold on
//              to your socks.
//
//-----------------------------------------------
HRESULT
CRecalcProperty::EvalExpression(VARIANT *pv)
{
    HRESULT hr;
    CExcepInfo  excepinfo;
    DISPPARAMS dispParams;

    Assert((LPOLESTR)_sExpression);

    if (_pdispExpression)
    {
        if (_pdispThis)     // We actually have a _pDEXExpression
        {
            DISPID dispidThis = DISPID_THIS;
            VARIANTARG  vThis;

            dispParams.rgvarg = &vThis;
            dispParams.rgdispidNamedArgs = &dispidThis;
            dispParams.cArgs = 1;
            dispParams.cNamedArgs = 1;

            V_VT(&vThis) = VT_DISPATCH;
            V_DISPATCH(&vThis) = _pdispThis;

            hr = THR(_pDEXExpression->InvokeEx(DISPID_VALUE,
                                          0,
                                          DISPATCH_METHOD,
                                          &dispParams,
                                          pv,
                                          &excepinfo,
                                          NULL));
        }
        else
        {
            dispParams = g_Zero.dispparams;
            hr = THR(_pdispExpression->Invoke(DISPID_VALUE,
                                          IID_NULL,
                                          0,
                                          DISPATCH_METHOD,
                                          &dispParams,
                                          pv,
                                          &excepinfo,
                                          NULL));
        }
    }
    else
    {
        hr = THR(GetHost()->EvalExpression(_pObject->GetUnknown(), _dispid, _sExpression, _sLanguage, pv));
    }
#if DBG == 1
    CVariant vStr;

    vStr.CoerceVariantArg(pv, VT_BSTR);

    TraceTag((tagRecalcEval, "%ls returned %ls", (LPOLESTR)_sExpression, V_BSTR(&vStr)));
#endif
    RRETURN(hr);
}

//---------------------------------------------------
//
// Method:      CRecalcProperty::Eval
//
// Description: This ensures that a property is up to date
//              This method should be called for any dependencies
//              Before actuallying evaluating the expression on a property
//
//              This method will also ensure that the computed value is
//              put into the OM
//
//              Circularity is detected but tolerated
//
//---------------------------------------------------
HRESULT
CRecalcProperty::Eval(BOOL fForce)
{
    HRESULT hr = S_OK;
    CVariant v;
    int i;

    TraceTag((tagRecalcEval, "%ls.%ls", _pObject->GetID(), _bstrName));

    //
    // Catch circularity
    //
    // For the cases where the circularlity is implicit (inner relationships between
    // properties) or hidden (global references to properties inside function calls), _fInEval
    // is used to dynamically detect circularity.
    //
    if (_fInEval)
    {
        TraceTag((tagRecalcEval, "bailing due to circularity", _pObject->GetID(), _bstrName));
        hr = S_OK;
        goto Cleanup;
    }

    // If we're trying to evaluate a property that doesn't have an expression
    // it must be canonical (because we only put dependencies on canonical properties)
    // In some cases, the real expression is on another object, in which case we eval
    // that object.  Otherwise we're done.
    if (_sExpression.IsNull())
    {
        // Aliases shouldn't be dependencies or top level evals.
        Assert(IsCanonical());

        // If we're looking at this object and we're not the one with the expression, go do the expression 
        if (_pPropertyExpression)
        {
            TraceTag((tagRecalcEval, "evaluating alias"));
            hr = THR(_pPropertyExpression->Eval(fForce));
        }

        goto Cleanup;
    }

    _fInEval = TRUE;

    Assert(!_sExpression.IsNull());

    //
    // This property has an expression.  Now we need to make sure all its dependencies
    // have been evaluated first.
    //
    // This loop needs to handle the possibility that some properties will go away
    // I actually think this is a pretty rare case but if all expressions are somehow
    // cleared as a result of evaluating one of them then the entire recalc graph could
    // be destroyed.

#if DBG == 1
    if (_dependencies.Size())
        TraceTag((tagRecalcEval, "Evaluating dependencies"));
#endif
    for (i = 0 ; i < _dependencies.Size() ; i++)
    {
        // Just because a dependency fails doesn't mean we should
        IGNORE_HR(_dependencies[i]->Eval(fForce));
    }

    if (_fDirtyDeps || fForce || (IsAlias() ? _pPropertyCanonical->_fDirty : _fDirty) || _fNoNotify || _fUnresolved)
    {
        // Now actually evaluate our expression

        hr = THR(EvalExpression(&v));

        if (hr)
        {
            // The expression eval failed for some reason.
            _fError = TRUE;
            hr = S_OK;          // gobble the error
            goto Cleanup;
        }

        else
        {
            _fError = FALSE;
            hr = THR(SetValue(&v, fForce));
            if (hr)
                goto Cleanup;
        }
    }

    TraceTag((tagRecalcEval, "%ls.%ls done\n", _pObject->GetID(), _bstrName));
Cleanup:
    SetDirty(FALSE);

    _fInEval = FALSE;
    RRETURN(hr);
}

//-----------------------------------------------
//
// Method:      CRecalcProperty::SetExpression
//
// Description: Assigns an expression to a property
//
// REVIEW michaelw  Needs more error handling
//
//-----------------------------------------------
HRESULT
CRecalcProperty::SetExpression(LPCOLESTR szExpression, LPCOLESTR szLanguage)
{
    HRESULT hr;
    IDispatchEx *pDEXExpression = 0;
    CRecalcProperty *pPropertyExpression = 0;

    Assert(!GetEngine()->InRecalc());

    if (!szExpression || !szLanguage)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pPropertyExpression = IsAlias() ? _pPropertyCanonical->_pPropertyExpression : _pPropertyExpression;

    if (pPropertyExpression && pPropertyExpression != this)
    {
        // There is already an expression on this property group and it isn't on this property

        TraceTag((tagRecalcEngine, "SetExpression: you can't have two expressions on a property group"));
        hr = E_FAIL;
        goto Cleanup;
    }

    _fStyleProp = _pObject->InStyle();
    if (_fStyleProp)
    {
        _fStyleSet = TRUE;
    }

    // Quickly ignore the case where the expression and language are the same
    if ((_tcscmp(szExpression, _sExpression) == 0) && (_tcscmp(szLanguage, _sLanguage) == 0))
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = clearExpressionHelper();
    if (hr)
        goto Cleanup;

    hr = THR(_sExpression.Set(szExpression));
    if (hr)
        goto Cleanup;

    hr = THR(_sLanguage.Set(szLanguage));
    if (hr)
        goto Cleanup;

    hr = THR(GetHost()->CompileExpression(_pObject->GetUnknown(), _dispid, _sExpression, _sLanguage, &_pdispExpression, &_pdispThis));
    if (hr == S_OK)
    {
        hr = THR(_pdispExpression->QueryInterface(IID_IDispatchEx, (LPVOID *)&pDEXExpression));
        if (hr == E_NOINTERFACE)
        {
            ClearInterface(&_pdispThis);
        }
        else if (hr)
            goto Cleanup;
        else
        {
            ClearInterface(&_pdispExpression);
            _pDEXExpression = pDEXExpression;
        }
    }

    hr = THR(ParseExpressionDependencies());
    if (hr)
        goto Cleanup;

    _fDirtyDeps = TRUE;

    hr = THR(GetEngine()->AddDependencyUpdateRequest(this));
    if (hr)
        goto Cleanup;

    if (IsAlias())
        _pPropertyCanonical->_pPropertyExpression = this;
    else
        _pPropertyExpression = this;

Cleanup:
    if (hr)
        IGNORE_HR(clearExpressionHelper());
    RRETURN(hr);
}

HRESULT
CRecalcProperty::GetExpression(BSTR *pstrExpression, BSTR *pstrLanguage)
{
    Assert(!_fSetDirty);
    Assert(!_fSetValue);
    Assert(!_fInEval);

    HRESULT hr;

    if (_sExpression.IsNull())
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    hr = THR(_sExpression.AllocBSTR(pstrExpression));
    if (hr)
        goto Cleanup;

    hr = THR(_sLanguage.AllocBSTR(pstrLanguage));
    if (hr)
    {
        SysFreeString(*pstrExpression);
        *pstrExpression = 0;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//---------------------------------------------------------------
//
// Function:    CRecalcProperty::ParseExpressionDependencies
//
// Description: Extract out identifiers from an expression.  Cool stuff.
//
//---------------------------------------------------------------
HRESULT
CRecalcProperty::ParseExpressionDependencies()
{
    SOURCE_TEXT_ATTR *pAttr = NULL, *pA;
    WCHAR *pch = 0;
    WCHAR *pchEnd = 0;
    HRESULT hr;
#ifndef RECALC_USE_SCRIPTDEBUG
    IActiveScriptAuthor *pScriptAuthor = 0;
#endif

    TraceTag((tagRecalcInfo, "ParseExpressionDependencies %08x (%d) \"%ls\"", this, _serialNumber, (LPOLESTR)_sExpression));
    if (_sExpression.Length() == 0)
        return S_OK;

    hr = THR(_sExpressionNames.Set(_sExpression));
    if (hr)
        goto Cleanup;

    pAttr = new SOURCE_TEXT_ATTR[_sExpressionNames.Length()];
    if (!pAttr)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

#ifndef RECALC_USE_SCRIPTDEBUG
    hr = THR(GetEngine()->GetScriptAuthor(_sLanguage, &pScriptAuthor));
    if (hr)
        goto Cleanup;

    hr = THR(pScriptAuthor->GetScriptTextAttributes(_sExpressionNames, _sExpressionNames.Length(), NULL, GETATTRTYPE_DEPSCAN, pAttr));
    if (hr)
        goto Cleanup;

#else
    hr = THR(GetHost()->GetScriptTextAttributes(_sLanguage, _sExpressionNames, _sExpressionNames.Length(), NULL, GETATTRTYPE_DEPSCAN | GETATTRFLAG_THIS, pAttr));
    if (hr)
        goto Cleanup;
#endif

    //
    // Start finding identifiers.  Only simple names (no array refs) are allowed
    //
    pch = _sExpressionNames;
    pchEnd = pch + _sExpressionNames.Length();
    pA = pAttr;
    while (pch < pchEnd)
    {
        if ((*pA == SOURCETEXT_ATTR_IDENTIFIER) || (*pA == SOURCETEXT_ATTR_THIS))
        {
            WCHAR *pchBegin = pch;

            while (pch < pchEnd && (*pA == SOURCETEXT_ATTR_IDENTIFIER || *pA == SOURCETEXT_ATTR_THIS || *pA == SOURCETEXT_ATTR_MEMBERLOOKUP))
            {
                if (*pA == SOURCETEXT_ATTR_MEMBERLOOKUP)
                    *pch = _T('.');
                pch++;
                pA++;
            }

            *pch = 0;

            TraceTag((tagRecalcDetail, "Got identifier: \"%ls\"", pchBegin));
            hr = THR(_dependencyNames.Append(pchBegin));
            if (hr)
                goto Cleanup;
        }
        pch++;
        pA++;
    }
Cleanup:
    if (pAttr)
        delete [] pAttr;
    return hr;
}

//---------------------------------------------------------------
//
// Function:    CRecalcProperty::UpdateDependencies
//
// Description: We've got a bunch of identifiers, resolve them to
//              actual object/dispid pairs and then setup the
//              dependencies.
//
//---------------------------------------------------------------
HRESULT
CRecalcProperty::UpdateDependencies()
{
    IDispatch *pDispatch = 0;
    DISPID dispid;
    HRESULT hr = S_OK;
    int i;
    LPOLESTR *psz;

    _fUnresolved = FALSE;

    Assert(_fDirtyDeps);
    _fDirtyDeps = FALSE;

    for (i = _dependencyNames.Size() , psz = _dependencyNames ; i > 0 ; i-- , psz++)
    {
        hr = THR(GetHost()->ResolveNames(_pdispThis ? _pdispThis : _pObject->GetUnknown(), _dispid, 1, psz, &pDispatch, &dispid));
        if (!hr)
        {
            hr = THR(AddDependency(pDispatch, dispid));

            ClearInterface(&pDispatch);

            if (hr)
                goto Cleanup;
        }
        else
        {
            _fUnresolved = TRUE;
            hr = S_OK;
            // Silently swallow the error
        }
    }

    _fCircular = FALSE;

    SetDirty(TRUE);
    if (_fCircular)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------
//
// Function:    CRecalcProperty::SetValue
//
// Description: Stuff a value into a property without blowing the
//              expression away.
//
//---------------------------------------------------------------
HRESULT
CRecalcProperty::SetValue(VARIANT *pv, BOOL fForce)
{
    Assert(pv);
    Assert(_fSetValue == FALSE);

    HRESULT hr = S_OK;

    if (fForce || !IsVariantEqual(&_vCurrent, pv))
    {
        _fSetValue = TRUE;

#if DBG == 1
        _cChanged = 0;
#endif

        hr = THR(GetHost()->SetValue(_pObject->GetUnknown(), _dispid, pv, _fStyleProp));
        Assert(_fSetValue);
        _fSetValue = FALSE;

        if (hr)
            goto Cleanup;

#if DBG == 1
        if (_cChanged != 1)
            TraceTag((tagRecalcDetail, "\n\n!!!!!!!!!!     Received %d change notifications after put\n\n", _cChanged));
        _cChanged = 0;
#endif

        hr = THR(VariantCopy(&_vCurrent, pv));
        if (hr)
        {
            VariantClear(&_vCurrent);
            goto Cleanup;
        }
    }

Cleanup:
    Assert(_fSetValue == FALSE);
    RRETURN(hr);
}

//---------------------------------------------------------------
//
// Function:    CRecalcProperty::CheckAndClearStyleSet
//
// Description: Checks to see if the styleSet bit has been set
//              Clears after the check
//
//---------------------------------------------------------------
BOOL
CRecalcProperty::CheckAndClearStyleSet()
{
    BOOL fStyleSet = _fStyleSet;
    _fStyleSet = FALSE;
    return fStyleSet;
}


//---------------------------------------------------
//
// Function:        GetCanonicalProperty
//
// Description:     Helper function to get the canonical
//                  property from a particular punk/dispid.
//                  Does "all" the work of QI'ing and so forth
//
//---------------------------------------------------
HRESULT
GetCanonicalProperty(IUnknown *pUnk, DISPID dispid, IUnknown **ppUnkCanonical, DISPID *pdispidCanonical)
{
    HRESULT hr;
    IRecalcProperty *pRecalcProperty;


    hr = pUnk->QueryInterface(IID_IRecalcProperty, (LPVOID *)&pRecalcProperty);
    if (hr == E_NOINTERFACE)
    {
        *ppUnkCanonical = 0;
        *pdispidCanonical = DISPID_UNKNOWN;
        hr = S_FALSE;
    }
    else if (SUCCEEDED(hr))
    {
        hr = THR(pRecalcProperty->GetCanonicalProperty(dispid, ppUnkCanonical, pdispidCanonical));
        pRecalcProperty->Release();
    }

    RRETURN1(hr, S_FALSE);
}

#if DBG == 1
//---------------------------------------------------------------
//
// Function:    int CRecalcEngine::
//
// Description: Debug dump
//
//---------------------------------------------------------------

void RecalcDumpVariant(LPOLESTR szName, VARIANT *pv)
{
    CVariant v;

    v.CoerceVariantArg(pv, VT_BSTR);

    if (V_VT(&v) == VT_ERROR)
        RecalcDumpFormat(_T("<0s>=[error]"), szName);
    else if (V_VT(&v) == VT_EMPTY)
        RecalcDumpFormat(_T("<0s>=[null]"), szName);
    else if (V_VT(&v) == VT_BSTR)
        RecalcDumpFormat(_T("<0s>=<1s> "), szName, V_BSTR(&v));
    else
        RecalcDumpFormat(_T("<0s>=[???] "), szName);
}

HANDLE g_hfileRecalcDump = INVALID_HANDLE_VALUE;

BOOL RecalcDumpOpen()
{
    if (g_hfileRecalcDump == INVALID_HANDLE_VALUE)
    {
        g_hfileRecalcDump = CreateFile(_T("c:\\recalcdump.htm"),
                                            GENERIC_WRITE | GENERIC_READ,
                                            FILE_SHARE_WRITE | FILE_SHARE_READ,
                                            NULL,
                                            OPEN_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL,
                                            NULL);

        if (g_hfileRecalcDump == INVALID_HANDLE_VALUE)
            return FALSE;

        DWORD dwSize = GetFileSize(g_hfileRecalcDump, 0);

        if (dwSize == 0)
            r_p(_T("<html><head><link rel=stylesheet type=text/css href=recalcdump.css></head>\n"));
        else
            SetFilePointer( g_hfileRecalcDump, GetFileSize( g_hfileRecalcDump, 0 ), 0, 0 );
    }

    return TRUE;
}

void RecalcDumpFormat(LPOLESTR szFormat, ...)
{
    va_list arg;

    va_start(arg, szFormat);

    WriteHelpV(g_hfileRecalcDump, szFormat, &arg);
}

int CRecalcEngine::Dump(DWORD dwFlags)
{
    int i;

    r_pf((_T("\t<<div id=e<0d>>Recalc Engine\n"), _serialNumber));
    r_p(_T("\t\t<div class=recalcmembers>"));
    r_pb(_fInRecalc);
    r_pb(_fRecalcRequested);
    r_pb(_fInDeps);
    r_p(_T(">\n"));

    for (i = 0 ; i < _objects.Size() ; i++)
    {
        _objects[i]->Dump(dwFlags);
    }

    r_p(_T("</div></BODY></HTML>\n"));

    return 0;
}

//---------------------------------------------------------------
//
// Function:    CRecalcObject::
//
// Description: Dump interesting stuff for debugging
//
//---------------------------------------------------------------
int
CRecalcObject::Dump(DWORD dwFlags)
{
    int i;

    r_pf((_T("\t\t<<div class=recalcobject id=o<0d>>Object o<0d> <1s>\n "), _serialNumber, _bstrID));
    r_p(_T("\t\t\t<div class=recalcmembers>Members"));
    r_p(_T("\t\t\t\t<div class=recalcvalues>"));
    r_ps(_bstrID);
    r_ps(_bstrTag);
    r_pb(_fGotSink);
    r_pb(_fDoingStyle);
    r_pb(_fInRecalc);
    r_p(_T("</div>\n"));
    r_p(_T("\t\t\t</div>\n"));

    for (i = 0 ; i < _properties.Size() ; i++)
    {
        _properties[i]->Dump(dwFlags);
    }
    r_p(_T("</div>"));
    return 0;
}

//---------------------------------------------------------------
//
// Function:    CRecalcProperty::TraceDump
//
// Description: Dump stuff
//
//---------------------------------------------------------------
int CRecalcProperty::Dump(DWORD dwFlags)
{
    r_pf((_T("\t\t\t<<div class=recalcproperty id=p<0d>>Property p<0d> <1s>.<2s>"), _serialNumber, _pObject->GetID(), _bstrName));
    r_p(_T("\t\t\t\t<div class=recalcmembers>Members\n"));
    r_p(_T("\t\t\t\t\t<div class=recalcvalues>"));

    r_ps(_bstrName);
    r_pn(_dispid);

    r_pb(_fDirty);
    r_pb(_fInEval);
    r_pb(_fSetValue);
    r_pb(_fCircular);
    r_pb(_fSetDirty);
    r_pb(_fStyleSet);
    r_pb(_fAlias);

    if (!_sExpression.IsNull())
    {
        r_ps(_sExpression);
        r_pv(&_vCurrent);
        r_ps(_sLanguage);
    }

    if (IsAlias())
    {
        r_pf((_T("canonical=p<0d> "), _pPropertyCanonical->_serialNumber));
    }
    else if (_pPropertyExpression)
    {
        r_pf((_T("aliasexp=p<0d> "), _pPropertyExpression->_serialNumber));
    }
    r_p(_T("\t\t\t\t\t</div>\n"));
    r_p(_T("\t\t\t\t</div>\n"));

    int i;

    if (_dependencyNames.Size())
    {
        r_pf((_T("\t\t\t\t<<div class=recalcdependencies>DependencyNames: <0d>\n"), _dependencyNames.Size()));

        for (i = 0 ; i < _dependencyNames.Size() ; i++)
            r_pf((_T("\t\t\t\t<<span class=recalcdependencyName><0s><</span><<br>\n"), (LPOLESTR)_dependencyNames[i]));

        r_p(_T("\t\t\t\t</div>\n"));
    }

    if (_dependencies.Size())
    {
        r_pf((_T("\t\t\t\t<<div class=recalcdependencies>Dependencies: <0d>\n"), _dependencies.Size()));

        for (i = 0 ; i < _dependencies.Size() ; i++)
            r_pf((_T("\t\t\t\t\t<<a class=recalcdependency href=#p'<0d>'>p<0d><</a><<br>\n"), (LPOLESTR)UIntToPtr(_dependencies[i]->_serialNumber)));

        r_p(_T("\t\t\t\t</div>\n"));
    }

    if (_dependents.Size())
    {
        r_pf((_T("\t\t\t\t<<div class=recalcdependents>Dependents: <0d>\n"), _dependents.Size()));

        for (i = 0 ; i < _dependents.Size() ; i++)
            r_pf((_T("\t\t\t\t\t<<a class=recalcdependent href=#p'<0d>'>p<0d><</a><<br>\n"), (LPOLESTR)UIntToPtr(_dependents[i]->_serialNumber)));
        r_p(_T("\t\t\t\t</div>\n"));
    }

    if (_aliases.Size())
    {
        r_pf((_T("\t\t\t\t<<div class=recalcaliases>Aliases: <0d>\n"), _aliases.Size()));

        for (i = 0 ; i < _aliases.Size() ; i++)
            r_pf((_T("\t\t\t\t\t<<a class=recalcalias href=#p'<0d>'>p<0d><</a><<br>\n"), (LPOLESTR)UIntToPtr(_aliases[i]->_serialNumber)));

        r_p(_T("\t\t\t\t</div>\n"));
    }

    r_p(_T("</div>\n"));
    return 0;
}
#endif
