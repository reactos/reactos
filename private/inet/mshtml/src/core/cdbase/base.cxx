//
//  Microsoft Forms³
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       base.cxx
//
//  Contents:   CBase implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ATOMTBL_HXX_
#define X_ATOMTBL_HXX_
#include "atomtbl.hxx"
#endif

#ifndef X_TYPENAV_HXX_
#define X_TYPENAV_HXX_
#include "typenav.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif

#ifndef X_ACTIVSCP_H_
#define X_ACTIVSCP_H_
#include <activscp.h>
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

#if TREE_SYNC
#ifndef X_SYNCMKP_HXX_
#define X_SYNCMKP_HXX_
#include "..\..\site\base\syncmkp.hxx" // baaad hack, I must fix this
#endif
#endif // TREE_SYNC

// This files is generated from the pdlparse /G to produce the table of handlers
// and the handler functions themselves.
#ifndef X_FUNCSIG_HXX_
#define X_FUNCSIG_HXX_
#include "funcsig.hxx"
#endif

// Include headers from that PDLParser generated for DISPIDs and IHTMLDocument2
#include "document.h"
#include "window.h"

#pragma warning(disable: 4189)  /* local variable is initialized but not referenced */
#pragma warning(disable: 4701)  /* local variable may be used without having been initialized */
#include <funcsig.cxx>
#pragma warning(default: 4701)
#pragma warning(default: 4189)

DeclareTag(tagOleAutomation, "OleAuto", "Enable OLE automation Invoke");
DeclareTag(tagInvokeTrace, "OleAuto", "Trace custom invoke calls (slow...)");
DeclareTag(tagDisableLockAR, "Lock", "Disable AddRef/Release in locks")


MtDefine(CFunctionPointer, ObjectModel, "CFunctionPointer")
MtDefine(CBaseFireEventAry_pv, Locals, "CBase::FireEvent stack ptr array")
MtDefine(CBaseFireTypeAry_pv, Locals, "CBase::FireType stack data array")
MtDefine(CBaseFirePropertyNotify_pv, Locals, "CBase::FirePropertyNotify ptr array")
MtDefine(CDispParams, ObjectModel, "CDispParms")
MtDefine(CDispParams_rgvarg, CDispParams, "CDispParams::rgvarg")
MtDefine(CDispParams_rgdispidNamedArgs, CDispParams, "CDispParams::rgdispidNamedArgs")

const IID * const g_apIID_IDispatchEx[] = { &IID_IDispatchEx, &IID_IDispatch, NULL };

BEGIN_TEAROFF_TABLE(CBase, IDispatchEx)
    //  IDispatch methods
    TEAROFF_METHOD(CBase, GetTypeInfoCount, gettypeinfocount, (UINT *pcTinfo))
    TEAROFF_METHOD(CBase, GetTypeInfo, gettypeinfo, (UINT itinfo, ULONG lcid, ITypeInfo ** ppTI))
    TEAROFF_METHOD(CBase, GetIDsOfNames, getidsofnames, (REFIID riid,
                                   LPOLESTR *prgpsz,
                                   UINT cpsz,
                                   LCID lcid,
                                   DISPID *prgid))
    TEAROFF_METHOD(CBase, Invoke, invoke, (DISPID dispidMember,
                            REFIID riid,
                            LCID lcid,
                            WORD wFlags,
                            DISPPARAMS * pdispparams,
                            VARIANT * pvarResult,
                            EXCEPINFO * pexcepinfo,
                            UINT * puArgErr))

    TEAROFF_METHOD(CBase, GetDispID, getdispid, (BSTR bstrName,
                               DWORD grfdex,
                               DISPID *pid))

    TEAROFF_METHOD(CBase, InvokeEx, invokeex, (DISPID id,
                        LCID lcid,
                        WORD wFlags,
                        DISPPARAMS *pdp,
                        VARIANT *pvarRes,
                        EXCEPINFO *pei,
                        IServiceProvider *pSrvProvider)) 
            
    TEAROFF_METHOD(CBase, DeleteMemberByName, deletememberbyname, (BSTR bstr,DWORD grfdex))
    TEAROFF_METHOD(CBase, DeleteMemberByDispID, deletememberbydispid, (DISPID id))    
    TEAROFF_METHOD(CBase, GetMemberProperties, getmemberproperties, (DISPID id,
                                         DWORD grfdexFetch,
                                         DWORD *pgrfdex))
    TEAROFF_METHOD(CBase, GetMemberName, getmembername, (DISPID id,
                                   BSTR *pbstrName))
    TEAROFF_METHOD(CBase, GetNextDispID, getnextdispid, (DWORD grfdex,
                                   DISPID id,
                                   DISPID *pid))
    TEAROFF_METHOD(CBase, GetNameSpaceParent, getnamespaceparent, (IUnknown **ppunk))
END_TEAROFF_TABLE()


BEGIN_TEAROFF_TABLE(CBase, IProvideMultipleClassInfo)
    TEAROFF_METHOD(CBase, GetClassInfo, getclassinfo, (ITypeInfo ** ppTI))
    TEAROFF_METHOD(CBase, GetGUID, getguid, (DWORD dwGuidKind, GUID * pGUID))
    TEAROFF_METHOD(CBase, GetMultiTypeInfoCount, getmultitypeinfocount, (ULONG *pcti))
    TEAROFF_METHOD(CBase, GetInfoOfIndex, getinfoofindex, (
            ULONG iti,
            DWORD dwFlags,
            ITypeInfo** pptiCoClass,
            DWORD* pdwTIFlags,
            ULONG* pcdispidReserved,
            IID* piidPrimary,
            IID* piidSource))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CBase, ISupportErrorInfo)
    TEAROFF_METHOD(CBase, InterfaceSupportsErrorInfo, interfacesupportserrorinfo, (REFIID iid))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CBase, IOleCommandTarget)
    TEAROFF_METHOD(CBase, QueryStatus, querystatus, (GUID * pguidCmdGroup, ULONG cCmds, MSOCMD rgCmds[], MSOCMDTEXT * pcmdtext))
    TEAROFF_METHOD(CBase, Exec, exec, (GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG * pvarargIn, VARIANTARG * pvarargOut))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CBase, ISpecifyPropertyPages)
    TEAROFF_METHOD(CBase, GetPages, getpages, (CAUUID * pPages))
END_TEAROFF_TABLE()


BEGIN_TEAROFF_TABLE(CBase, IObjectIdentity)
    TEAROFF_METHOD(CBase, IsEqualObject, isequalobject, (IUnknown*))
END_TEAROFF_TABLE()


// NOTE: Tearoff table for CFunctionPointer IDispatchEx located in types.hdl
// is included in element.cxx for other tearoff tables.

HRESULT
CFunctionPointer::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
    }

    if (*ppv)
    {
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


HRESULT
CFunctionPointer::InvokeEx(DISPID id,
                           LCID lcid,
                           WORD wFlags,
                           DISPPARAMS *pdp,
                           VARIANT *pvarRes,
                           EXCEPINFO *pei,
                           IServiceProvider *pSrvProvider)
{
    HRESULT     hr;

    // Value property on the function object returns the name of the function.
    if (id == DISPID_VALUE && !(wFlags & DISPATCH_METHOD))
    {
        TCHAR       pchFuncName[255];

        if (pvarRes)
        {
            PROPERTYDESC   *pDesc;
            WORD            wEntry;
            WORD            wIIDIndex;

            V_VT(pvarRes) = VT_BSTR;

            hr = _pThis->FindPropDescFromDispID(_dispid, (PROPERTYDESC **)&pDesc, &wEntry, &wIIDIndex);
            if (!hr)
            {
                Format(0, &pchFuncName, 255, _T("\nfunction <0s>() {\n    [native code]\n}\n"),
                       pDesc->pstrExposedName ? pDesc->pstrExposedName :
                                                pDesc->pstrName);
                if (!hr)
                {
                    hr = FormsAllocString(pchFuncName, &V_BSTR(pvarRes));
                }
            }

            // Any error return null string.
            if (hr)
                V_BSTR(pvarRes) = NULL;
        }

        hr = S_OK;
    }
    else
    {
        hr = _pThis->InvokeEx(_dispid, lcid, wFlags, pdp, pvarRes, pei, pSrvProvider);
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CDispParams::Create, public
//
//  Synopsis:   Allocated argument and named argument arrays.
//              Initial values are VT_NULL for argument array
//              and DISPID_UNKNOWN for named argument array.
//
//----------------------------------------------------------------
HRESULT
CDispParams::Create (DISPPARAMS *pOrgDispParams)
{
    HRESULT     hr = S_OK;
    UINT        i;

    // Nothing should exist yet.
    if (rgvarg || rgdispidNamedArgs)
    {
        hr = E_FAIL;
        goto Error;
    }

    if (cArgs + cNamedArgs)
    {
        rgvarg = new(Mt(CDispParams_rgvarg)) VARIANTARG[cArgs + cNamedArgs];
        if (!rgvarg)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        // cArgs is now total count of args including named args.
        cArgs = cArgs + cNamedArgs;

        // Initialize all parameters to VT_NULL.
        for (i = 0; i < cArgs; i++)
        {
            rgvarg[i].vt = VT_NULL;
        }

        // Any arguments to copy over?
        if (pOrgDispParams->cArgs)
        {
            if (cArgs >= pOrgDispParams->cArgs)
            {
                UINT    iStartIndex;

                iStartIndex = cArgs - pOrgDispParams->cArgs;

                if (cArgs >= iStartIndex + pOrgDispParams->cArgs)
                {
                    memcpy(&rgvarg[iStartIndex],
                           pOrgDispParams->rgvarg,
                           sizeof(VARIANTARG) * pOrgDispParams->cArgs);
                }
            }
            else
            {
                hr = E_UNEXPECTED;
                goto Cleanup;
            }
        }

        if (cNamedArgs)
        {
            rgdispidNamedArgs = new(Mt(CDispParams_rgdispidNamedArgs)) DISPID[cNamedArgs];
            if (!rgdispidNamedArgs)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            }

            // Initialize all named args to the unknown dispid.
            for (i = 0; i < cNamedArgs; i++)
            {
                rgdispidNamedArgs[i] = DISPID_UNKNOWN;
            }

            if (pOrgDispParams->cNamedArgs)
            {
                if (cNamedArgs >= pOrgDispParams->cNamedArgs)
                {
                    UINT    iStartIndex;

                    iStartIndex = cNamedArgs - pOrgDispParams->cNamedArgs;
                    if (cNamedArgs >= (iStartIndex + pOrgDispParams->cNamedArgs))
                    {
                        memcpy(&rgdispidNamedArgs[iStartIndex],
                               pOrgDispParams->rgdispidNamedArgs,
                               sizeof(VARIANTARG) * pOrgDispParams->cNamedArgs);

                    }
                }
                else
                {
                    hr = E_UNEXPECTED;
                    goto Cleanup;
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);

Error:
    delete [] rgvarg;
    delete [] rgdispidNamedArgs;
    goto Cleanup;
}


//+---------------------------------------------------------------
//
//  Member:     CDispParams::MoveArgsToDispParams, public
//
//  Synopsis:   Move arguments from arguments array to pOutDispParams.
//              Notice, I said move not copy so both this
//              object and the pOutDispParams hold the identical
//              VARIANTS.  So be careful to only release these
//              variants ONCE.  The fFromEnd parameter specifies
//              how the arguments are moved from our rgvar array.
//
//----------------------------------------------------------------
HRESULT
CDispParams::MoveArgsToDispParams (DISPPARAMS *pOutDispParams, UINT cNumArgs, BOOL fFromEnd /*= TRUE*/)
{
    HRESULT hr = S_OK;
    UINT     iStartIndex;

    if (rgvarg && cNumArgs)
    {
        if (cArgs >= cNumArgs && pOutDispParams->cArgs >= cNumArgs)
        {
            iStartIndex = fFromEnd ? cArgs - cNumArgs : 0;

            memcpy(pOutDispParams->rgvarg,
                   &rgvarg[iStartIndex],
                   sizeof(VARIANTARG) * cNumArgs);
            goto Cleanup;
        }

        hr = E_FAIL;
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CDispParams::ReleaseVariants, public
//
//  Synopsis:   Release any variants in out arguments array.
//              Again, if the values were moved (exist in 2
//              places) be careful you may have just screwed
//              yourself.
//
//----------------------------------------------------------------
void
CDispParams::ReleaseVariants ()
{
    for (UINT i = 0; i < cArgs; i++)
        VariantClear(rgvarg + i);
}


//+---------------------------------------------------------------
//
//  Member:     CBaseCF::CreateInstance, public
//
//  Synopsis:   Method of IClassFactory interface.
//
//----------------------------------------------------------------

STDMETHODIMP
CBaseCF::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID iid,
    void **ppv)
{
    HRESULT     hr;                         
    CBase       *pBase  = NULL;
    IUnknown    *pUnk;

    CEnsureThreadState ets;
    hr = ets._hr;
    if (FAILED(hr))
        goto Error;

    if (pUnkOuter && iid != IID_IUnknown)
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    // Initialize the class
    // (Ensure only one thread performs initialization)

    if (_pfnInitClass)
    {
        LOCK_GLOBALS;

        if (_pfnInitClass)
        {
            hr = (*_pfnInitClass)();
            if (hr)
                goto Error;

            _pfnInitClass = NULL;
        }
    }

    // Create the object in two steps.  The first
    // step calls the constructor.

    pBase = (*_pfnCreate)(pUnkOuter);
    if (!pBase)
        goto MemoryError;

    // Check whether aggregation is supported by the object.

    if (pUnkOuter && pBase->PunkOuter() != pUnkOuter)
    {
        hr = CLASS_E_NOAGGREGATION;
        goto Error;
    }

    // Call the second step initialization.

    hr = pBase->Init();
    if (hr)
        goto Error;

    pUnk = pBase->PunkInner();

    if (pUnkOuter)
    {
        *ppv = pUnk;
    }
    else
    {
        hr = pUnk->QueryInterface(iid, ppv);
        pUnk->Release();
    }

Cleanup:
    RRETURN(hr);

MemoryError:
    hr = E_OUTOFMEMORY;

Error:
    *ppv = NULL;
    if (pBase)
        pBase->PrivateRelease();
    goto Cleanup;
}

//+---------------------------------------------------------------
//
//  Member:     CBaseLockCF::LockServer, public
//
//  Synopsis:   Method of IClassFactory interface.
//             
//              Allows client to lock thread state.
//
//----------------------------------------------------------------

STDMETHODIMP
CBaseLockCF::LockServer (BOOL fLock)
{
    HRESULT hr;

#ifdef OBJCNTCHK
    DWORD dwObjCnt;
#endif

    if (fLock)
    {
        CEnsureThreadState ets;
        hr = ets._hr;
        if (FAILED(hr))
        {
            goto Cleanup;
        }

        TLS(dll.cLockServer) += 1;
        IncrementObjectCount(&dwObjCnt);
    }
    else
    {
#ifdef OBJCNTCHK
        THREADSTATE * pts = GetThreadState();
        dwObjCnt = GetCurrentThreadId();

        if (!pts)
        {
#if DBG==1
            AssertSz(0, "CBaseLockCF::LockServer(FALSE) called but there is no thread state");
#else
            F3DebugBreak();
#endif
            hr = E_FAIL;
        }
        else if (pts->dll.cLockServer <= 0)
        {
#if DBG==1
            AssertSz(0, "CBaseLockCF::LockServer(FALSE) called too many times on this thread");
#else
            F3DebugBreak();
#endif
            hr = E_FAIL;
        }
        else
#endif
        {
            TLS(dll.cLockServer) -= 1;
            DecrementObjectCount(&dwObjCnt);
            hr = S_OK;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CBase::CBase, public
//
//  Synopsis:   Constructor.
//
//----------------------------------------------------------------

#if DBG == 1
ULONG CBase::s_ulLastSSN = 0;
#endif

CBase::CBase()
{
    _pAA = NULL;

    IncrementSecondaryObjectCount( 0 );
    _ulRefs = 1;
    _ulAllRefs = 1;

#if DBG == 1
    _ulSSN = ++s_ulLastSSN;
    MemSetName((this, "SSN=%d", _ulSSN));
#endif
#ifdef WIN16
    m_baseOffset = 0;
#endif
#ifdef OBJCNTCHK
    _dwTidDbg = GetCurrentThreadId();
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CBase::CBaseCheckThread
//
//  Synopsis:   Verifies that the current thread is the same as the one this
//              object was created on.
//
//----------------------------------------------------------------------------

#ifdef OBJCNTCHK

void
CBase::CBaseCheckThread()
{
    if (_dwTidDbg != GetCurrentThreadId())
    {
        char ach[512];
        wsprintfA(ach, "Attempt to access CBase object on the wrong thread.  The object was "
                 "created on thread 0x%08lX; attemping access on thread 0x%08lX.",
                 _dwTidDbg, GetCurrentThreadId());
#if DBG==1
        AssertSz(0, ach);
#else
        F3DebugBreak();
#endif
    }
}

#endif

//+---------------------------------------------------------------------------
//
//  Member:     CBase::Init
//
//  Synopsis:   Nothing happens.
//
//----------------------------------------------------------------------------

HRESULT
CBase::Init()
{
    //
    // Assert that the cpi classdesc is setup correctly.  It should either
    // be NULL or have at least two entries.  The second entry can be
    // CPI_ENTRY_NULL.
    //

    Assert(!BaseDesc()->_pcpi || BaseDesc()->_pcpi[0].piid);
            
    return S_OK;
}

CBase::~CBase()
{
    CBaseCheckThread();

    Assert("Ref count messed up in derived dtor?" &&
            (_ulAllRefs == ULREF_IN_DESTRUCTOR || _ulAllRefs == 1) &&
            (_ulRefs == ULREF_IN_DESTRUCTOR || _ulRefs == 1));

    Assert(_pAA == NULL);

    Assert( ! GWHasPostedMethod( this ) );

    DecrementSecondaryObjectCount(0);
}


//+---------------------------------------------------------------
//
//  Member:     CBase::Passivate
//
//  Synopsis:   Shutdown main object by releasing references to
//              other objects and generally cleaning up.  This
//              function is called when the main reference count
//              goes to zero.  The destructor is called when
//              the reference count for the main object and all
//              embedded sub-objects goes to zero.
//
//---------------------------------------------------------------

void
CBase::Passivate()
{
    CBaseCheckThread();

    Assert("CBase::Passivate called unexpectedly or refcnt "
            "messed up in derived Passivate" &&
            (_ulRefs == ULREF_IN_DESTRUCTOR || _ulAllRefs == 1));

    //
    // The attr array destructor will free up any stuff left inside it.
    // This includes anyone sinking events or prop notifies from us.
    //
    // WARNING!!!  Be very careful to not delete the _pAA anywhere but
    // here - CStyle and its children do not manage their AAs, so _pAA
    // is changed to NULL in their Passivate() (before CBase::Passivate()
    // is called).  If you want _pAA to be destroyed elsewhere, consider
    // the CStyle case carefully.  -CWilso
    //
    
    delete _pAA;
    _pAA = NULL;
}

//+---------------------------------------------------------------
//
//  member: CBase ::IsEqualObject
//
//  synopsis : default IObjectIdentity implementation.
//
//----------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CBase::IsEqualObject(IUnknown * pUnk)
{
    HRESULT    hr = E_POINTER;
    IUnknown * pUnkThis = NULL;
    IUnknown * pUnkTarget=NULL;

    if (!pUnk)
        goto Cleanup;

    hr = THR_NOTRACE(PrivateQueryInterface(IID_IUnknown, (void**)&pUnkThis));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pUnk->QueryInterface(IID_IUnknown, (void**)&pUnkTarget));
    if (hr)
        goto Cleanup;

    hr = (pUnkTarget == pUnkThis) ? S_OK : S_FALSE;

Cleanup:
    ReleaseInterface(pUnkThis);
    ReleaseInterface(pUnkTarget);
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------
//
//  Member:     CBase::PrivateQueryInterface, IPrivateUnknown
//
//---------------------------------------------------------------

HRESULT CBase::PrivateQueryInterface(REFIID iid, void **ppvObj)
{
#if DBG==1
    // If we ever get to CBase::PrivateQueryInterface with an IID of one of the
    // non-stacked interfaces then the most derived class needs to handle this
    // IID in it's PrivateQueryInterface.
    for (int i = 0; i++; i < MAX_IIDS)
    {
        if (IsEqualIID(iid, *(_IIDTable[i])))
            break;
    }            

    Assert("Non stack interface not handled in PrivateQueryInterface" && DispNonDualDIID(iid) || i < MAX_IIDS); 
#endif

    *ppvObj = NULL;
    return E_NOINTERFACE;
}


#if DBG == 1
BOOL        g_fDisableBaseTrace;
BOOL        g_fDisableBaseSubTrace;
CBase *     g_pBaseTrace;
CBase *     g_pSubBaseTrace;
ULONG       g_ulSSNTrace = 0xFFFFFFFF;
ULONG       g_ulSSNSubTrace = 0xFFFFFFFF;
#endif


//+---------------------------------------------------------------
//
//  Member:     CBase::PrivateAddRef, IPrivateUnknown
//
//---------------------------------------------------------------

ULONG CBase::PrivateAddRef()
{
    CBaseCheckThread();

    Assert("CBase::PrivateAddRef called after CBase::Passivate." &&
            _ulRefs != 0);

    _ulRefs += 1;

#if DBG==1
    if (!g_fDisableBaseTrace && (this == g_pBaseTrace || _ulSSN == g_ulSSNTrace))
    {
        TraceTag((0, "base %x AR %d", this, _ulRefs));
        TraceCallers(0, 0, 12);
    }

    return _ulRefs;
#else
    return 0;
#endif
}

//+---------------------------------------------------------------
//
//  Member:     CBase::PrivateRelease, IPrivateUnknown
//
//---------------------------------------------------------------

ULONG CBase::PrivateRelease()
{
    CBaseCheckThread();

    ULONG ulRefs = --_ulRefs;

#if DBG==1
    if (!g_fDisableBaseTrace && (this == g_pBaseTrace || _ulSSN == g_ulSSNTrace))
    {
        TraceTag((0, "base %x Rel %d", this, ulRefs));
        TraceCallers(0, 0, 12);
    }
#endif

    if (ulRefs == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        Passivate();
        Assert("Unexpected refcnt on return from CBase::Passivate" &&
                _ulRefs == ULREF_IN_DESTRUCTOR);
        _ulRefs = 0;

        SubRelease();
    }

    return ulRefs;
}

#if DBG == 1

ULONG CBase::SubAddRef( )
{
    CBaseCheckThread();

    if (!g_fDisableBaseTrace && 
        !g_fDisableBaseSubTrace && (
            this == g_pBaseTrace || 
            this == g_pSubBaseTrace ||
            _ulSSN == g_ulSSNTrace ||
            _ulSSN == g_ulSSNSubTrace))
    {
        TraceTag((0, "base %x SubAR %d", this, _ulAllRefs + 1));
        TraceCallers(0, 0, 12);
    }
    return ++_ulAllRefs;
}

#endif

//+---------------------------------------------------------------
//
//  Member:     CBase::SubRelease
//
//---------------------------------------------------------------

ULONG CBase::SubRelease()
{
    CBaseCheckThread();

#if DBG==1
    ULONG ulAllRefs = _ulAllRefs;

    if (!g_fDisableBaseTrace &&
        !g_fDisableBaseSubTrace && (
            this == g_pBaseTrace || 
            this == g_pSubBaseTrace ||
            _ulSSN == g_ulSSNTrace ||
            _ulSSN == g_ulSSNSubTrace))
    {
        TraceTag((0, "base %x SubRel %d", this, ulAllRefs - 1));
        TraceCallers(0, 0, 12);
    }
#endif

    if (--_ulAllRefs == 0)
    {
        _ulAllRefs = ULREF_IN_DESTRUCTOR;
        _ulRefs = ULREF_IN_DESTRUCTOR;
        delete this;
    }

#if DBG==1
    return ulAllRefs - 1;
#else
    return 0;
#endif
}


//+------------------------------------------------------------------------
//
//  Member:     CBase::CLock::CLock
//
//  Synopsis:   Lock resources in CBase object.
//
//-------------------------------------------------------------------------

CBase::CLock::CLock(CBase *pBase)
{
#if DBG==1
    g_fDisableBaseTrace = TRUE;
#endif

    _pBase = pBase;

#if DBG==1
    if (!IsTagEnabled(tagDisableLockAR))
#endif
    {
        pBase->PunkOuter()->AddRef();
    }
    
#if DBG==1
    g_fDisableBaseTrace = FALSE;
#endif
}

//+------------------------------------------------------------------------
//
//  Member:     CServer::CLock::~CLock
//
//  Synopsis:   Unlock resources in CBase object.
//
//-------------------------------------------------------------------------

CBase::CLock::~CLock()
{
#if DBG==1
    g_fDisableBaseTrace = TRUE;
#endif

#if DBG==1
    if (!IsTagEnabled(tagDisableLockAR))
#endif
    {
        _pBase->PunkOuter()->Release();
    }
    
#if DBG==1
    g_fDisableBaseTrace = FALSE;
#endif
}


//+------------------------------------------------------------------------
//
//  Member:     CBase::QueryService
//
//  Synopsis:   Get service from host.  Derived classes should override.
//
//  Arguments:  guidService     id of service
//              iid             id of interface on service
//              ppv             the service
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CBase::QueryService(REFGUID guidService, REFIID iid, void ** ppv)
{
    if ( ppv )
        *ppv = NULL;
    RRETURN(E_NOINTERFACE);
}


//+------------------------------------------------------------------------
//
//  Member:     CBase::GetPages, ISpecifyPropertyPages
//
//  Synopsis:   Specifies the property pages supported by this object.
//
//  Arguments:  pPages  CLSID's for the pages are returned via a counted array
//
//  Returns:    HRESULT
//
//  Notes:      The array of CLSIDs is divided into two groups, each group
//              ending in a NULL pointer. The first group of CLSIDs is used
//              for when we are in run-mode (browse mode), and the second for
//              when we are in edit mode.
//
//-------------------------------------------------------------------------

STDMETHODIMP
CBase::GetPages(CAUUID * pPages)
{
#ifdef NO_PROPERTY_PAGE
    pPages->pElems = NULL;
    pPages->cElems = 0;
    return S_OK;
#else
    int         i, c;
    int         iStart;
    BOOL        fDesign = DesignMode();
    const CLSID ** apclsid = BaseDesc()->_apclsidPages;

    pPages->pElems = NULL;
    pPages->cElems = 0;

    if (apclsid)
    {
        if (fDesign)
        {   
            for (c = 0; apclsid[c]; c++)
                ;

            iStart = c + 1;
        }
        else
        {
            iStart = 0;
        }

        for (i = iStart; apclsid[i]; i++)
            ;

        c = i - iStart;

        if (c != 0)
        {
            pPages->pElems = (GUID *) CoTaskMemAlloc(c * sizeof(CLSID));
            if (!pPages->pElems)
            {
                pPages->cElems = 0;
                RRETURN(E_OUTOFMEMORY);
            }

            for (i = iStart; i < iStart + c; i++)
            {
                memcpy(pPages->pElems + i - iStart,
                       apclsid[i],
                       sizeof(CLSID));
            }
            pPages->cElems = c;
        }
    }

    return S_OK;
#endif // NO_PROPERTY_PAGE
}


//+------------------------------------------------------------------------
//
//  Member:     CBase::HasPages
//
//  Synopsis:   Checks if any property pages are supported by this object.
//
//  Returns:    BOOL
//
//  Notes:      The array of CLSIDs is divided into two groups, each group
//              ending in a NULL pointer. The first group of CLSIDs is used
//              for when we are in run-mode (browse mode), and the second for
//              when we are in edit mode.
//
//-------------------------------------------------------------------------

BOOL
CBase::HasPages()
{
#ifdef NO_PROPERTY_PAGE
    return FALSE;
#else
    int         i = 0;
    const CLSID ** apclsid = BaseDesc()->_apclsidPages;


    if (!apclsid)
        return FALSE;

    if (DesignMode())
    {   
        for (; apclsid[i]; i++)
            ;

        i++;
    }

    return (apclsid[i] != NULL);
#endif // NO_PROPERTY_PAGE
}

//+---------------------------------------------------------------------------
//
//  Member:     CBase:GetClassInfo, IProvideMultipleClassInfo
//
//  Synopsis:   Returns the control's coclass typeinfo.
//
//  Arguments:  ppTI    Resulting typeinfo.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CBase::GetClassInfo(ITypeInfo ** ppTI)
{
    RRETURN(THR(LoadF3TypeInfo(*BaseDesc()->_pclsid, ppTI)));
}


//+---------------------------------------------------------------------------
//
//  Member:     CBase:GetGUID, IProvideMultipleClassInfo
//
//  Synopsis:   Returns some type of requested guid
//
//  Arguments:  dwGuidKind      The type of guid requested
//              pGUID           Resultant
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CBase::GetGUID(DWORD dwGuidKind, GUID *pGUID)
{
    if (!pGUID)
        RRETURN(E_INVALIDARG);

    *pGUID = g_Zero.guid;

    if (dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID)
    {
        *pGUID= (BaseDesc()->_pcpi) ? 
                    *(BaseDesc()->_pcpi[CPI_OFFSET_PRIMARYDISPEVENT].piid) :
                    IID_NULL;
        return S_OK;
    }

    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Member:     CBase:GetMultiTypeInfoCount, IProvideMultipleClassInfo
//
//  Synopsis:   Returns the count of type infos on this object
//
//  Arguments:  pcti    Resultant of count of ti's.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CBase::GetMultiTypeInfoCount(ULONG *pcti)
{
    HRESULT hr = S_OK;

    if (!pcti)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // The default is we support one typeinfo
    *pcti = 1;

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CBase:GetAggMultiTypeInfoCount
//
//  Synopsis:   Helper for aggregators for IProvideMultipleClassInfo
//
//  Arguments:  pcti    Resultant of count of ti's.
//              pUnkAgg The aggregate IUnknown
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CBase::GetAggMultiTypeInfoCount(ULONG *pcti, IUnknown *pUnkAgg)
{
    HRESULT                     hr = S_OK;
    IProvideMultipleClassInfo * pPMCI = NULL;

    if (!pcti)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pcti = 0;

    if (pUnkAgg)
    {
        // Determine how many ITypeInfos are available from the contained object
        hr = THR_NOTRACE(pUnkAgg->QueryInterface(
                IID_IProvideMultipleClassInfo,
                (void **)&pPMCI));
        if (hr)
        {
            //
            // Aggregate does not support MCI, just give it one TI.
            //

            *pcti = 1;
        }
        else
        {
            hr = THR(pPMCI->GetMultiTypeInfoCount(pcti));
            if (hr)
                goto Cleanup;
        }
    }

    //
    // Finally append our own TI
    //

    (*pcti)++;
    hr = S_OK;

Cleanup:
    ReleaseInterface(pPMCI);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CBase:GetInfoOfIndex, IProvideMultipleClassInfo
//
//  Synopsis:   Get info on the type-info of some index
//
//  Arguments:  iti         Index of type info
//              dwFlags
//              pptiCoClass
//              pdwTIFlags
//              pcdispidReserved
//              piidPrimary
//              piidSource
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CBase::GetInfoOfIndex(
        ULONG iTI,
        DWORD dwFlags,
        ITypeInfo** ppTICoClass,
        DWORD* pdwTIFlags,
        ULONG* pcdispidReserved,
        IID* piidPrimary,
        IID* piidSource)
{
    HRESULT             hr = S_OK;

    if (ppTICoClass)
        *ppTICoClass = NULL;
    if (pdwTIFlags)
        *pdwTIFlags = 0;
    if (pcdispidReserved)
        *pcdispidReserved = 0;
    if (piidPrimary)
        *piidPrimary = IID_NULL;
    if (piidSource)
        *piidSource = IID_NULL;

    if (((dwFlags & MULTICLASSINFO_GETTYPEINFO) && !ppTICoClass)                ||
        ((dwFlags & MULTICLASSINFO_GETNUMRESERVEDDISPIDS) && !pcdispidReserved) ||
        ((dwFlags & MULTICLASSINFO_GETIIDPRIMARY) && !piidPrimary)              ||
        ((dwFlags & MULTICLASSINFO_GETIIDSOURCE) && !piidSource))
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (iTI != 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (dwFlags & MULTICLASSINFO_GETTYPEINFO)
    {
        hr = THR(GetClassInfo(ppTICoClass));
        if (hr)
            goto Cleanup;
    }

    if (dwFlags & MULTICLASSINFO_GETNUMRESERVEDDISPIDS)
    {
        //
        // NOTE: (anandra) For our default objects, the reserved
        // dispids have been verified in the code.  Other aggregators
        // will need to ensure this or implement their own.
        //
        
        *pcdispidReserved = 0;
    }

    if (dwFlags & MULTICLASSINFO_GETIIDPRIMARY)
    {
        *piidPrimary = (BaseDesc()->_piidDispinterface) ?
                            *(BaseDesc()->_piidDispinterface) : 
                            IID_NULL;
    }

    if (dwFlags & MULTICLASSINFO_GETIIDSOURCE)
    {
        *piidSource = (BaseDesc()->_pcpi && 
                       BaseDesc()->_pcpi[CPI_OFFSET_PRIMARYDISPEVENT].piid) ? 
                        *(BaseDesc()->_pcpi[CPI_OFFSET_PRIMARYDISPEVENT].piid) :
                        IID_NULL;
    }

    if (pdwTIFlags)
    {
        *pdwTIFlags = TIFLAGS_EXTENDDISPATCHONLY;
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CBase:GetAggInfoOfIndex
//
//  Synopsis:   Helper for getting info on the type-info of some index
//              on an aggregator
//
//  Arguments:  iti         Index of type info
//              dwFlags
//              pptiCoClass
//              pdwTIFlags
//              pcdispidReserved
//              piidPrimary
//              piidSource
//              pUnkAgg
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CBase::GetAggInfoOfIndex(
            ULONG iTI,
            DWORD dwFlags,
            ITypeInfo** ppTICoClass,
            DWORD* pdwTIFlags,
            ULONG* pcdispidReserved,
            IID* piidPrimary,
            IID* piidSource,
            IUnknown *pUnkAgg)
{
    IProvideMultipleClassInfo * pPMCI = NULL;
    IProvideClassInfo *         pPCI = NULL;
    ITypeInfo *                 pTICoClass = NULL;
    HRESULT                     hr = S_OK;

    if (iTI == 0)
    {
        // Call default implementation
        hr = THR(CBase::GetInfoOfIndex(
                iTI,
                dwFlags,
                ppTICoClass,
                pdwTIFlags,
                pcdispidReserved,
                piidPrimary,
                piidSource));
    }
    else
    {
        //
        // If the aggregate supports multiple ITypeInfos, pass the request on
        // (Decrement the index by one to account for our extender ITypeInfo)
        //

        hr = THR_NOTRACE(pUnkAgg->QueryInterface(
                IID_IProvideMultipleClassInfo,
                (void **)&pPMCI));
        if (!hr)
        {
            hr = THR(pPMCI->GetInfoOfIndex(iTI - 1,
                                           dwFlags,
                                           ppTICoClass,
                                           pdwTIFlags,
                                           pcdispidReserved,
                                           piidPrimary,
                                           piidSource));
        }
        else if (iTI != 1)
        {
            // If the caller requested an ITypeInfo for something other
            // than the us or that of the immediate aggregate and the
            // aggregate does not support multiple ITypeInfos, it is an error
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        else
        {
            //
            // If the aggregate does not support multiple ITypeInfos and the
            // request is for the aggregate's ITypeInfo, return it
            // using traditional methods
            //

            if (pdwTIFlags)
            {
                *pdwTIFlags = TIFLAGS_EXTENDDISPATCHONLY;
            }

            if (dwFlags & (MULTICLASSINFO_GETTYPEINFO   |
                           MULTICLASSINFO_GETIIDPRIMARY |
                           MULTICLASSINFO_GETIIDSOURCE))
            {
                hr = THR(pUnkAgg->QueryInterface(
                        IID_IProvideClassInfo,
                        (void **)&pPCI));
                if (hr)
                    goto Cleanup;

                if (!ppTICoClass)
                    ppTICoClass = &pTICoClass;

                hr = THR(pPCI->GetClassInfo(ppTICoClass));
                if (hr)
                    goto Cleanup;

                if ((dwFlags & MULTICLASSINFO_GETIIDPRIMARY) ||
                    (dwFlags & MULTICLASSINFO_GETIIDSOURCE))
                {
                    hr = THR(GetTypeInfoFromCoClass(
                                *ppTICoClass,
                                (dwFlags & MULTICLASSINFO_GETIIDPRIMARY
                                        ? FALSE
                                        : TRUE),
                                NULL,
                                (dwFlags & MULTICLASSINFO_GETIIDPRIMARY
                                        ? piidPrimary
                                        : piidSource)));
                    if (hr)
                        goto Cleanup;
                }
            }
        }
    }

    hr = S_OK;

Cleanup:
    ReleaseInterface(pPMCI);
    ReleaseInterface(pPCI);
    ReleaseInterface(pTICoClass);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CBase:InterfaceSupportsErrorInfo, ISupportErrorInfo
//
//  Synopsis:   Return true if given interface supports error info.
//
//  Arguments:  iid the interface
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CBase::InterfaceSupportsErrorInfo(REFIID iid)
{
    const CLASSDESC * pcd;

    pcd = BaseDesc();
    return (pcd->_piidDispinterface &&
            *pcd->_piidDispinterface == iid) ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------
//
//  Member:     CBase::GetClassID, IPersist
//
//  Synopsis:   Method of IPersist interface
//
//  Returns:    HRESULT
//
//---------------------------------------------------------------

STDMETHODIMP
CBase::GetClassID(LPCLSID pclsid)
{
    *pclsid = *BaseDesc()->_pclsid;
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CBase::InvokeDispatchWithThis
//
//  Synopsis:   invokes pDisp adding IUnknown of this
//              named parameter to args list
//
//---------------------------------------------------------------

HRESULT
CBase::InvokeDispatchWithThis (
    IDispatch *     pDisp,
    VARIANT *       pExtraArg,
    REFIID          riid,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pdispparams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pexcepinfo,
    IServiceProvider *pSrvProvider) 
{
    HRESULT         hr;
    IDispatchEx  *  pDEX = NULL;
    DISPPARAMS *    pPassedParams = pdispparams;
    IUnknown *      pUnk = NULL;
    CDispParams     dispParams((pExtraArg ? pdispparams->cArgs + 1 : pdispparams->cArgs),
                                pdispparams->cNamedArgs + 1);

    hr = THR_NOTRACE(pDisp->QueryInterface(IID_IDispatchEx, (void **)&pDEX));
    // Does the object support IDispatchEx then we'll need to pass
    // the this pointer of this object
    if (!hr)
    {
        Verify (!THR(PrivateQueryInterface(IID_IDispatch, (void **)&pUnk)));

        hr = dispParams.Create(pdispparams);
        if (hr)
            goto Cleanup;

        // Create the named this parameter
        Assert(dispParams.cNamedArgs > 0);
        V_VT(&dispParams.rgvarg[0]) = VT_DISPATCH;
        V_UNKNOWN(&dispParams.rgvarg[0]) = pUnk;
        dispParams.rgdispidNamedArgs[0] = DISPID_THIS;

        if (pExtraArg)
        {
            UINT     uIdx = dispParams.cArgs - 1;

            memcpy(&(dispParams.rgvarg[uIdx]), pExtraArg, sizeof(VARIANT));
        }

        pPassedParams = &dispParams;
    }

    //
    // Call the function
    //
    hr = pDEX ? THR(pDEX->InvokeEx(DISPID_VALUE,
                                 lcid,
                                 wFlags,
                                 pPassedParams,
                                 pvarResult,
                                 pexcepinfo,
                                 pSrvProvider))         :
                THR(pDisp->Invoke(DISPID_VALUE,
                                  riid,
                                  lcid,
                                  wFlags,
                                  pPassedParams,
                                  pvarResult,
                                  pexcepinfo,
                                  NULL));

    // If we had to pass the this parameter and we had more than
    // 1 parameter then copy the orginal back over (the args could have
    // been byref).
    if (pPassedParams != pdispparams && pPassedParams->cArgs > 1)
    {
        hr = dispParams.MoveArgsToDispParams(pdispparams, pdispparams->cArgs);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pDEX);
    ReleaseInterface(pUnk);

    RRETURN (hr);
}


//+---------------------------------------------------------------
//
//  Member:     CBase::InvokeDispatchExtraParam
//
//  Synopsis:   invokes pDisp adding pExtraParam as the last
//              parameter to args list
//
//---------------------------------------------------------------

HRESULT
CBase::InvokeDispatchExtraParam (
    IDispatch *     pDisp,
    REFIID          riid,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pdispparams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pexcepinfo,
    IServiceProvider *pSrvProvider,
    VARIANT        *pExtraParam) 
{
    HRESULT         hr;
    IDispatchEx  *  pDEX = NULL;
    DISPPARAMS *    pPassedParams = pdispparams;
    CDispParams     dispParams(pdispparams->cArgs, 1);

    Assert(pdispparams->cNamedArgs == 0);

    IGNORE_HR(pDisp->QueryInterface(IID_IDispatchEx, (void **)&pDEX));

    hr = dispParams.Create(pdispparams);
    if (hr)
        goto Cleanup;

    // Add the extra parameter
    dispParams.cNamedArgs = 0;
    delete [] dispParams.rgdispidNamedArgs;
    dispParams.rgdispidNamedArgs = 0;

    // Move the extra parameter.
    memcpy(&dispParams.rgvarg[0], pExtraParam, sizeof(VARIANTARG));

    pPassedParams = &dispParams;

    //
    // Call the function
    //
    hr = pDEX ? THR_NOTRACE(pDEX->InvokeEx(DISPID_VALUE,
                                 lcid,
                                 wFlags,
                                 pPassedParams,
                                 pvarResult,
                                 pexcepinfo,
                                 pSrvProvider))         :
                THR_NOTRACE(pDisp->Invoke(DISPID_VALUE,
                                  riid,
                                  lcid,
                                  wFlags,
                                  pPassedParams,
                                  pvarResult,
                                  pexcepinfo,
                                  NULL));

    // If we had to pass the this parameter and we had more than
    // 1 parameter then copy the orginal back over (the args could have
    // been byref).
    if (pPassedParams != pdispparams && pPassedParams->cArgs > 1)
    {
        hr = dispParams.MoveArgsToDispParams(pdispparams, pdispparams->cArgs - 1);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pDEX);

    RRETURN (hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::Invoke
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

STDMETHODIMP
CBase::Invoke(
        DISPID dispidMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS * pdispparams,
        VARIANT * pvarResult,
        EXCEPINFO * pexcepinfo,
        UINT *)
{
    HRESULT         hr;
    IDispatchEx    *pDispEx;

    if (!IsEqualIID(riid, IID_NULL))
        RRETURN(E_INVALIDARG);

    hr = THR_NOTRACE(PrivateQueryInterface(IID_IDispatchEx, (void **)&pDispEx));
    if (hr)
    {
        // Object doesn't support IDispatchEx use CBase::InvokeEx
        hr = InvokeEx(dispidMember, lcid, wFlags, pdispparams,pvarResult, pexcepinfo, NULL);
    }
    else
    {
        // Object supports IDispatchEx call InvokeEx thru it's v-table.
        hr = pDispEx->InvokeEx(dispidMember, lcid, wFlags, pdispparams,pvarResult, pexcepinfo, NULL);
        ReleaseInterface(pDispEx);
    }

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::GetIDsOfNames
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

STDMETHODIMP
CBase::GetIDsOfNames(REFIID riid,
                     LPOLESTR *  rgszNames,
                     UINT,
                     LCID,
                     DISPID *    rgdispid)
{
    HRESULT         hr;
    IDispatchEx    *pDispEx;

    if (!IsEqualIID(riid, IID_NULL))
        RRETURN(E_INVALIDARG);

    hr = THR_NOTRACE(PrivateQueryInterface(IID_IDispatchEx, (void **)&pDispEx));
    if (hr)
    {
        // Object doesn't support IDispatchEx use CBase::InvokeEx
        hr = GetDispID(rgszNames[0], fdexFromGetIdsOfNames, rgdispid);
    }
    else
    {
        // Object supports IDispatchEx call InvokeEx thru it's v-table.
        hr = pDispEx->GetDispID(rgszNames[0], fdexFromGetIdsOfNames, rgdispid);
        ReleaseInterface(pDispEx);
    }

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Function:   GetArgsActual
//
//  Synopsis:   helper
//
//--------------------------------------------------------------------------

LONG GetArgsActual(DISPPARAMS * pdispparams)
{
    LONG cArgsActual;
    // If the parameters passed in has the named DISPID_THIS paramter, then we
    // don't want to include this parameter in the total number of parameter
    // count.  This named parameter is an additional parameter tacked on by the
    // script engine to handle scoping rules.
    cArgsActual = pdispparams->cArgs;
    if (pdispparams->cNamedArgs && *pdispparams->rgdispidNamedArgs == DISPID_THIS)
    {
        cArgsActual--;  // Don't include DISPID_THIS in argument count.
    }

    return cArgsActual;
}

//+-------------------------------------------------------------------------
//
//  Method:     CBase::InvokeAA
//
//  Synopsis:   helper
//
//--------------------------------------------------------------------------

HRESULT
CBase::InvokeAA(
    DISPID              dispidMember,
    CAttrValue::AATYPE  aaType,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pdispparams,
    VARIANT *           pvarResult,
    EXCEPINFO *         pexcepinfo,
    IServiceProvider *  pSrvProvider)
{
    HRESULT     hr = DISP_E_MEMBERNOTFOUND;
    LONG        cArgsActual = GetArgsActual(pdispparams);

    if ((cArgsActual == 0) && (wFlags & DISPATCH_PROPERTYGET))
    {
        if (pvarResult)
        {
            hr = GetVariantAt(FindAAIndex(dispidMember, aaType),
                                pvarResult, /* fAllowNullVariant = */FALSE);
            if (!hr)
                goto Cleanup;
        }
    }
    else if (wFlags & DISPATCH_PROPERTYPUT)
    {
        if (pdispparams && cArgsActual == 1)
        {
            AAINDEX aaIdx = FindAAIndex(dispidMember, aaType);
            if (aaIdx == AA_IDX_UNKNOWN)
            {
                hr = AddVariant(dispidMember,
                                pdispparams->rgvarg,
                                aaType);
            }
            else
            {
                hr = ChangeVariantAt(aaIdx, pdispparams->rgvarg);
            }
            if (hr)
                goto Cleanup;
            hr = OnPropertyChange(dispidMember, 0);
            goto Cleanup;
        }
        else
        {
            // Missing value to set.
            hr = DISP_E_BADPARAMCOUNT;
            goto Cleanup;
        }
    }

    // If the wFlags was marked as a Get/Method and the get failed then try
    // this as a method invoke.  Of if the wFlags was only a method invoke
    // then the hr is set to DISP_E_MEMBERNOTFOUND at the entry ExpandoInvoke.
    if (hr && (wFlags & DISPATCH_METHOD))
    {
        AAINDEX aaidx = FindAAIndex(dispidMember, aaType);
        if (AA_IDX_UNKNOWN == aaidx)
        {
            if (pvarResult)
            {
                VariantClear(pvarResult);
                pvarResult->vt = VT_NULL;
                hr = S_OK;
            }
        }
        else
        {
            IDispatch * pDisp = NULL;
        
            hr = THR(GetDispatchObjectAt(aaidx, &pDisp));
            if (!hr && pDisp)
            {
                hr = THR(InvokeDispatchWithThis(
                    pDisp,
                    NULL,
                    IID_NULL,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    pSrvProvider));
                ReleaseInterface(pDisp);
           }
        }
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }

Cleanup:
    RRETURN (hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CBase::ContextInvokeEx, IDispatchEx
//
//  Synopsis:   Real implementation of InvokeEx.  Uses context passed
//              in for actual calls
//
//--------------------------------------------------------------------------

STDMETHODIMP CBase::ContextInvokeEx(DISPID dispidMember,
                                    LCID lcid,
                                    WORD wFlags,
                                    DISPPARAMS * pdispparams,
                                    VARIANT * pvarResult,
                                    EXCEPINFO * pexcepinfo,
                                    IServiceProvider *pSrvProvider,
                                    IUnknown *pUnkContext)
{
    HRESULT         hr = S_OK;
    long            cArgsActual = GetArgsActual(pdispparams);
    DISPPARAMS     *pOldDispParams = NULL;
    CDispParams     dpMissingArgs;

    ITypeInfo * pTI = NULL;
    IUnknown *  pUnk = NULL;

    // clear the per thread EI object to check again after custom Invoke to see if 
    // it was set from a prop\method on failure
    IErrorInfo *pErrorInfo = NULL;

#if DBG == 1
    hr = GetErrorInfo(0, &pErrorInfo);
    if (!hr)
    {
        ReleaseInterface(pErrorInfo);
        pErrorInfo = NULL;
    }
#endif

    //
    // Handle special performance critical dispids without loading typeinfo.
    //

    if (wFlags & DISPATCH_PROPERTYGET)
    {
        switch (dispidMember)
        {
        case DISPID_ENABLED:
        case DISPID_VALID:
            if (pvarResult == NULL)
                RRETURN(E_INVALIDARG);

            V_VT(pvarResult) = VT_BOOL;
            switch (dispidMember)
            {
            case DISPID_ENABLED:
                return THR_NOTRACE(GetEnabled(&V_BOOL(pvarResult)));

            case DISPID_VALID:
                return THR_NOTRACE(GetValid(&V_BOOL(pvarResult)));

            default:
                Assert(0 && "Logic error");
                break;
            }
            break;

        case DISPID_UNKNOWN:
            // Special case handling if we ever have a condition were
            // something can't be found but the invoke caller expect us
            // to return data (pvarResult) then instead of returning the
            // error DISP_EMEMBERNOTFOUND we'll return DISPID_UNKNOWN with
            // a hr result of S_OK (inn GetIDsOfNamesEx) when the Invoke
            // is called and if we ever get a DISPID_UNKNOWN we'll simply
            // return VT_NULL.  This will JavaScript to compare the result
            // to null and VBScript to detect isnull()
            if (pvarResult)
            {
                VariantClear(pvarResult);
                pvarResult->vt = VT_NULL;
                hr = S_OK;
                goto Cleanup;
            }
            break;

        default:
            break;
        }
    }

    // Are we an expando?
    if (IsExpandoDISPID ( dispidMember ) )
    {

ExpandoInvoke:
        hr = THR(InvokeAA (dispidMember, CAttrValue::AA_Expando,
                lcid, wFlags, pdispparams, pvarResult, pexcepinfo, pSrvProvider));

    }
    else
    {
        PROPERTYDESC_METHOD   *pDesc;
        WORD                   wEntry;
        WORD                   wIIDIndex;

#if DBG == 1
        if (IsTagEnabled(tagOleAutomation))
        {
            goto OLEInvoke;
        }
#endif

        // Can we find the properydesc for this dispid, if so then use our
        // custom invoke.
        hr = FindPropDescFromDispID(dispidMember, (PROPERTYDESC **)&pDesc, &wEntry, &wIIDIndex);
        if (hr)
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }
        else
        {
            CustomHandler   pHandler;
            IDispatch      *pDisp;
            CVariant        varTemp;

#if DBG == 1
            if (IsTagEnabled(tagInvokeTrace))
            {
                OutputDebugString(_T("Invoke: "));
                OutputDebugString((pDesc->a.pstrExposedName) ? pDesc->a.pstrExposedName :
                                                               pDesc->a.pstrName);
                OutputDebugString(_T("\r\n"));
            }
#endif  // Output what's happening.

            if (!pvarResult)
            {
                pvarResult = &varTemp;
            }

            Assert(pDesc);

            if ((cArgsActual == 0)                              &&
                (wFlags & DISPATCH_PROPERTYGET)                 &&
                (pDesc->b.dwPPFlags & PROPPARAM_INVOKEGet))
            {
                // If the property supports both get/set then the vtable offset
                // in the propdesc is for the set adding +sizeof(void *) gets us to the get
                // property function.
                if (pDesc->b.dwPPFlags & PROPPARAM_INVOKESet)
                {
                    wEntry += sizeof(void *);
                }
                goto CustomInvoke;
            }
            else if ((cArgsActual == 1)                         &&
                     (wFlags & DISPATCH_PROPERTYPUT)            &&
                     (pDesc->b.dwPPFlags & PROPPARAM_INVOKESet))
            {
                goto CustomInvoke;
            }
            // object put or get value property of an object?
            else if ((wFlags & DISPATCH_PROPERTYPUT ||
                      wFlags & DISPATCH_PROPERTYGET)                &&
                     pDesc->b.dwPPFlags & PROPPARAM_INVOKEGet)
            {
                if (cArgsActual > 0 && 
                    (pDesc->b.wInvFunc == IDX_G_IDispatchp ||
                     pDesc->b.wInvFunc == IDX_GS_IDispatchp))
                {
                    // Possible indexed collection access
                    CVariant    varTmp;

                    // Must be QI not PrivateQI, location, navigator, etc. are wrapped
                    // we must delegate to the wrappers.  If the index into the IIDTable
                    // is zero then the wEntry is on the primary default interface otherwise
                    // use the interface in the IIDTable.
                    hr = pUnkContext->QueryInterface(_IIDTable[wIIDIndex] 
                                                     ? *_IIDTable[wIIDIndex] 
                                                     : *BaseDesc()->_piidDispinterface,
                                               (void **) &pDisp);
                    if (hr)
                        goto Cleanup;

                    // If the property is both a get/set then adjust offset to
                    // point to the get property function.
                    if (pDesc->b.dwPPFlags & PROPPARAM_INVOKESet)
                    {
                        wEntry += sizeof(void *);
                    }

                    hr = G_IDispatchp(this,
                                      pSrvProvider,
                                      pDisp,
                                      wEntry,
                                      (PROPERTYDESC_BASIC_ABSTRACT *)pDesc,
                                      wFlags,
                                      NULL,
                                      &varTmp);
                    if (!hr)
                    {
                        if (V_VT(&varTmp) == VT_DISPATCH)
                        {
                            IDispatch  *pDisp = V_DISPATCH(&varTmp);

        				    IDispatchEx *pDispEx;

                            if ( !pDisp )
                            {
                                hr = DISP_E_MEMBERNOTFOUND;
                            }
                            else
                            {
                                hr = pDisp->QueryInterface ( IID_IDispatchEx, (void **)&pDispEx );
                                if ( !hr )
                                {
                                    hr = pDispEx->InvokeEx(DISPID_VALUE,
                                        lcid, wFlags, pdispparams,
                                        pvarResult,pexcepinfo,pSrvProvider);
                                    ReleaseInterface(pDispEx);
                                    SetErrorInfo(hr);
                                }
                                else
                                {
                                    hr = pDisp->Invoke(DISPID_VALUE,
        				                IID_NULL,lcid,wFlags,pdispparams,
                        				pvarResult,pexcepinfo,NULL);
                                    SetErrorInfo(hr);
                                }                            
                                if (FAILED(hr))
                                {
                                    GetErrorInfo(0, &pErrorInfo);
                                    Assert(pErrorInfo && "Property or Method did not SetErrorInfo");
                                }
                            }
                        }
                        else
                        {
                            hr = DISP_E_NOTACOLLECTION;
                        }
                    }

                    ReleaseInterface(pDisp);

                    goto Cleanup2;
                }
            }
            else if (wFlags & DISPATCH_METHOD)
            {
                // Check if method and args are at least minimum #.  Note, I
                // longer check the maximum count, JavaScript (Navigator)
                // ignores any arguments greater than max and we will as well.
                // So we wont check:
                //
                //   cArgsActual <= pDesc->d   // #args <= max # of args?
                //
                if (!((pDesc->b.dwPPFlags & PROPPARAM_INVOKEGet)    ||
                       (pDesc->b.dwPPFlags & PROPPARAM_INVOKESet))      &&
                    cArgsActual >= pDesc->e)      // #args >= required minimum args?
                {
                    // It's a method invocation.
                    goto CustomInvoke;
                }
                // Check to see if scriptlet flag is on.
                else if (pDesc->b.dwPPFlags & PROPPARAM_SCRIPTLET)
                {
                    hr = DISP_E_MEMBERNOTFOUND;
                    
                    // Invoke with dispid this, it's a property.
                    AAINDEX aaidx = FindAAIndex(dispidMember, CAttrValue::AA_Internal);
                    if (AA_IDX_UNKNOWN != aaidx)
                    {
                        IDispatch * pDisp = NULL;
                        
                        hr = THR(GetDispatchObjectAt(aaidx, &pDisp));
                        if (!hr && pDisp)
                        {
                            hr = THR(InvokeDispatchWithThis(
                                pDisp,
                                NULL,
                                IID_NULL,
                                lcid,
                                wFlags,
                                pdispparams,
                                pvarResult,
                                pexcepinfo,
                                pSrvProvider));
                            ReleaseInterface(pDisp);
                            if (FAILED(hr))
                            {
                                GetErrorInfo(0, &pErrorInfo);
                                Assert(pErrorInfo && "Property or Method did not SetErrorInfo");
                            }
                         }
                    }
                    goto Cleanup;
                }
                else if (!((pDesc->b.dwPPFlags & PROPPARAM_INVOKEGet)    ||
                           (pDesc->b.dwPPFlags & PROPPARAM_INVOKESet))      &&
                    cArgsActual < pDesc->e)      // #args < less than minimum args?
                {
                    // Construct null parameters for each missing parameter.
                    dpMissingArgs.Initialize(pDesc->e, pdispparams->cNamedArgs);

                    hr = dpMissingArgs.Create(pdispparams);
                    if (hr)
                        goto Cleanup;

                    pOldDispParams = pdispparams;
                    pdispparams = &dpMissingArgs;

                    // It's a method invocation.
                    goto CustomInvoke;
                }
            }
            else if ((cArgsActual == 1)                  &&
                     (wFlags & DISPATCH_PROPERTYPUTREF)         &&
                     (pDesc->b.dwPPFlags & PROPPARAM_INVOKESet))
            {
                // Passing argument by reference. 
                goto CustomInvoke;
            }
            else if ((wFlags & DISPATCH_PROPERTYGET) && dispidMember == DISPID_VALUE &&
                pvarResult != NULL && cArgsActual == 0 )
            {
                // PROPGET For default property
                hr = DISP_E_MEMBERNOTFOUND;
                goto Cleanup;
            }
            // Accessing a method w/o arguments (e.g, document.open;).  This
            // would invoke the open as a get property which we'll return as
            // undefined.  Nav3 would do nothing but error if alert(doc.open)
            // we'll return undefined.
            else if (wFlags & DISPATCH_PROPERTYGET &&
                     ((pDesc->b.dwPPFlags & PROPPARAM_INVOKESet) == 0))
            {
                // Compute open expando and get the expando value.
                DISPID  dispIDExpando;
                TCHAR  *pName = const_cast <TCHAR *> (pDesc->a.pstrExposedName ?
                                                pDesc->a.pstrExposedName :
                                                pDesc->a.pstrName);

                // Compute the expando dispid.
                hr = GetExpandoDispID(pName, &dispIDExpando, fdexNameCaseSensitive);
                if (!hr)
                {
                    // Found the expando so get the expando value.
                    dispidMember = dispIDExpando;
                    goto ExpandoInvoke;
                }
                else
                {
                    // Create the CFunctionPointer object and store it in the
                    // attrArray.
                    AAINDEX     aIdx;

                    aIdx = FindAAIndex(dispidMember, CAttrValue::AA_Attribute);
                    if (aIdx == AA_IDX_UNKNOWN)
                    {
                        V_DISPATCH(pvarResult) = (IDispatch *)new CFunctionPointer(this, dispidMember);

                        hr = V_DISPATCH(pvarResult) ?
                                    AddDispatchObject(dispidMember,
                                                      V_DISPATCH(pvarResult),
                                                      CAttrValue::AA_Attribute)
                                    : E_OUTOFMEMORY;
                    }
                    else
                    {
                        // Object already exist.
                        hr = GetDispatchObjectAt(aIdx, &V_DISPATCH(pvarResult));
                    }

                    V_VT(pvarResult) = VT_DISPATCH;

                    // Any error then cleanup the any objects created?
                    if (hr)
                    {
                        VariantClear(pvarResult);
                        V_VT(pvarResult) = VT_NULL;

                        // No expando/function object, return undefined.
                        hr = S_OK;
                    }

                    goto Cleanup;
                }            
            }

            // Catch all error for anything not customed invoked.
            hr = E_NOTIMPL;
            goto Cleanup;

CustomInvoke:
#if 0
if (StrCmpIC(_T("parent"), pDesc->a.pstrExposedName ?
                            pDesc->a.pstrExposedName :
                            pDesc->a.pstrName) == 0)
    DebugBreak(); 
#endif
            pHandler = _HandlerTable[pDesc->b.wInvFunc];
            if (!pHandler)
            {
                hr = DISP_E_MEMBERNOTFOUND;
                goto Cleanup;
            }

            // Must be QI not PrivateQI, location, navigator, etc. are wrapped
            // we must delegate to the wrappers.  If the index into the IIDTable
            // is zero then the wEntry is on the primary default interface otherwise
            // use the interface in the IIDTable.
            hr = pUnkContext->QueryInterface(_IIDTable[wIIDIndex] 
                                                ? *_IIDTable[wIIDIndex] 
                                                : *BaseDesc()->_piidDispinterface,
                                             (void **) &pDisp);
            if (hr)
                goto Cleanup;

            // Before we Invoke, cache the Invoke Context in the Attr Array. Store it for the duration of the
            // Invoke so that we can establish script context

            // Can optimize this by knowing what dispids need to know this info
            // Adding it into the attr array automaticaly AddRef's it
            if ( pSrvProvider )
                AddUnknownObject ( DISPID_INTERNAL_INVOKECONTEXT, pSrvProvider, CAttrValue::AA_Internal );

            hr = (*pHandler)(this, pSrvProvider, pDisp, wEntry, (PROPERTYDESC_BASIC_ABSTRACT *)pDesc, wFlags, pdispparams, pvarResult);

            // Did we make a method call where we fabricated arguments which were
            // missing?
            if (pOldDispParams)
            {
                // Yes, so move the args back to original dispparams and reset
                // out dispparam pointers.
                hr = dpMissingArgs.MoveArgsToDispParams(pOldDispParams, pOldDispParams->cArgs);

                pdispparams = pOldDispParams;
                pOldDispParams = NULL;

                if (hr)
                    goto Cleanup;
            }

            if ( pSrvProvider )
                FindAAIndexAndDelete ( DISPID_INTERNAL_INVOKECONTEXT, CAttrValue::AA_Internal );
            
            if (FAILED(hr))
            {
                GetErrorInfo(0, &pErrorInfo);
                // BUGWIN16: we might want to uncomment it after getting the
                // midl compiler fix.
#ifndef WIN16
                Assert(pErrorInfo && "Property or Method did not SetErrorInfo");
#endif //ndef WIN16                
            }

            ReleaseInterface(pDisp);
        }

#if DBG==1
        goto Cleanup;

OLEInvoke:
        // Problem, so use ITypeInfo::Invoke
        hr = GetTypeInfo(0, lcid, &pTI);
        if (hr)
            goto Cleanup;

        hr = pUnkContext->QueryInterface(
                            *BaseDesc()->_piidDispinterface,
                            (void **) &pUnk);
        if (hr)
            goto Cleanup;

        hr = pTI->Invoke(
                    pUnk,
                    dispidMember,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    NULL);
#endif
    }

Cleanup:
    // Nav support for coercing object ptrs to Strings
    // BUGBUG rgardner need to confirm this works for OBJECT's
    if ( hr == DISP_E_MEMBERNOTFOUND && (wFlags & DISPATCH_PROPERTYGET) && dispidMember == DISPID_VALUE &&
                pvarResult != NULL && cArgsActual == 0)
    {
        V_VT(pvarResult) = VT_BSTR;
        hr = THR(FormsAllocString ( _T("[object]"),&V_BSTR(pvarResult) ) );
    }

Cleanup2:
    if (FAILED(hr) && pexcepinfo)
    {
        pexcepinfo->wCode = 0;
        pexcepinfo->scode = hr;
        if (pErrorInfo)
        {
            pErrorInfo->GetSource(&(pexcepinfo->bstrSource));
            pErrorInfo->GetDescription(&(pexcepinfo->bstrDescription));
            pErrorInfo->GetHelpFile(&(pexcepinfo->bstrHelpFile));
            pErrorInfo->GetHelpContext(&(pexcepinfo->dwHelpContext));
            hr = DISP_E_EXCEPTION;
        }
    }
        
    ReleaseInterface(pErrorInfo);
    ReleaseInterface(pTI);
    ReleaseInterface(pUnk);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::GetDispID, IDispatchEx
//
//  Synopsis:   Delegates to ITypeInfo::GetIDsOfName first if this calls fails
//              then check if the name exist in the propertyDesc array and then
//              the expando list (don't do yet).  If the name doesn't exist in
//              any of those list then create an expando property if grfdex
//              parameter says to and return the dispid of the new expando.
//--------------------------------------------------------------------------

STDMETHODIMP
CBase::GetDispID(BSTR bstrName,
                 DWORD grfdex,
                 DISPID *pid)
{
    HRESULT     hr;

    if (!pid)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = CBase::GetInternalDispID(bstrName, pid, grfdex);
    if (!hr)
    {
        // We found the name, however we'll need to check if the name is a method
        // if so then the builtin method could have been overriden.  So check
        // the attr array for this expando.
        PROPERTYDESC_METHOD    *pDesc;
        WORD                    wEntry;
        WORD                    wIIDIndex;

        hr = FindPropDescFromDispID(*pid,
                                    (PROPERTYDESC **)&pDesc,
                                    &wEntry,
                                    &wIIDIndex);
        if (!hr)
        {
            // Accessing a method, could have changed so it might be expando.
            if ((pDesc->b.dwPPFlags & PROPPARAM_INVOKESet) == 0)
            {
                DISPID  dispIDExpando;

                // Compute the expando dispid.
                hr = GetExpandoDispID(bstrName, &dispIDExpando, grfdex);
                if (!hr)
                {
                    if ((*GetAttrArray())->FindAAIndex(dispIDExpando,
                                                       CAttrValue::AA_Expando) != AA_IDX_UNKNOWN)
                    {
                        // Found the expando so get the expando value.
                        *pid = dispIDExpando;
                    }
                }
            }
        }

        hr = S_OK;
        goto Cleanup;
    }

    // Try expandos
    hr = GetExpandoDispID(bstrName, pid, grfdex);
    if (hr == S_FALSE)
    {
        // If we're returning false then the rgdispid is DISPID_UNKNOWN expando
        // wasn't found but it wasn't added either we'll return VT_NULL on the
        // invoke.
        Assert(*pid == DISPID_UNKNOWN);
        hr = S_OK;
    }
#if DBG == 1
    else if (hr == S_OK)
    {
        if (IsTagEnabled(tagInvokeTrace))
        {
            OutputDebugString(_T("NamesEx expando: "));
            OutputDebugString(bstrName);
            OutputDebugString(_T("\r\n"));
        }
    }
#endif  // Output what's happening.

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CBase::DeleteMember, IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
CBase::DeleteMemberByName(BSTR psz,DWORD grfdex)
{
    return E_NOTIMPL;
}

HRESULT
CBase::DeleteMemberByDispID(DISPID id)
{
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CBase::GetMemberProperties, IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
CBase::GetMemberProperties(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex)
{
    return E_NOTIMPL;
}


HRESULT
CBase::GetMemberName(DISPID id, BSTR *pbstrName)
{
    LPCTSTR pszName;
    PROPERTYDESC *pPropDesc;
    const TCHAR *pstr;
    DISPID expDispid;

    if (!pbstrName)
        return E_INVALIDARG;

    *pbstrName = NULL;
    
    if (IsExpandoDISPID(id, &expDispid))
    {
        GetExpandoName(expDispid, &pszName);
        FormsAllocString(pszName, pbstrName);
    }
    else
    {
        if (FindPropDescFromDispID(id, &pPropDesc, NULL, NULL))
            goto Cleanup;

        pstr = pPropDesc->pstrExposedName ?
                                 pPropDesc->pstrExposedName :
                                 pPropDesc->pstrName;

        FormsAllocString(pstr, pbstrName);
    }

Cleanup:
    return *pbstrName ? S_OK : DISP_E_MEMBERNOTFOUND;
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::GetNextDispID, IDispatchEx
//
//  Synopsis:   Loop through all property entries in ITypeInfo then enumerate
//              through all expando properties.  Any DISPID handed out here
//              must stay constant (GetDispID should hand out the
//              same).  This routines enumerates regular properties and expando.
//
//  Results:
//              S_OK        - returns next DISPID
//              S_FALSE     - enumeration is done
//              E_nnnn      - call failed.
//--------------------------------------------------------------------------

HRESULT
CBase::GetNextDispID(DWORD grfdex,
                     DISPID id,
                     DISPID *prgid)
{
    HRESULT     hr;
    BSTR        bstr = NULL;

    hr = GetInternalNextDispID(grfdex, id, prgid, &bstr);
    FormsFreeString( bstr );
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::GetNameSpaceParent, IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
CBase::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT     hr;

    if (!ppunk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppunk = NULL;
    hr = S_OK;

Cleanup:
    RRETURN(hr);
}


HRESULT
CBase::GetInternalDispID(BSTR           bstrName,
                         DISPID       * pid,
                         DWORD          grfdex, 
                         IDispatch    * pDisp /*= NULL*/,
                         IDispatchEx  * pDispEx /*= NULL*/)
{
    HRESULT                     hr = DISP_E_UNKNOWNNAME;
    const VTABLEDESC           *pVTblDesc;

    pVTblDesc = FindVTableEntryForName (bstrName, 
                                        (!!(grfdex & fdexNameCaseSensitive)));
    if (pVTblDesc)
    {
        // Variable name usage on a method?
        if ((!!(grfdex & fdexNameEnsure)) &&
            ((pVTblDesc->pPropDesc->GetPPFlags() & PROPPARAM_INVOKEGet) ||
            (pVTblDesc->pPropDesc->GetPPFlags() & PROPPARAM_INVOKESet)) == 0)
        {
            // Yes, so if we've got a function here then error we
            // want this as an expando.
            goto Cleanup;
        }
        else
        {
            // Either it's a real method wanted or we're working on a
            // property.
            *pid = pVTblDesc->pPropDesc->GetDispid();

            if (!_pAA)
            {
                _pAA = new CAttrArray;
                // If new doesn't succeed then no cache is used.
            }

            // Set cache of last found DISPID and vtable desc this cache is
            // used by custom invoke to eliminate search.
            if (_pAA)
            {
                _pAA->SetGINCache(*pid, pVTblDesc);
            }

            hr = S_OK;
            goto Cleanup;           // We're done we can leave now.
        }
    }

    // They better both not be set.
    Assert(!(pDisp && pDispEx));

    // Is there another typeinfo we should look at?
    if (pDisp || pDispEx)
    {
        if (pDispEx) 
        {
            hr = THR_NOTRACE(pDispEx->GetDispID(bstrName, 
                                                grfdex, 
                                                pid));
        }
        else if (pDisp)
        {
            // If control is IDispatch then no case sensitive match.
            ITypeInfo   *pTI = NULL;

            hr = THR(pDisp->GetTypeInfo(0, g_lcidUserDefault, &pTI));
            if (!hr)
            {
                hr = THR_NOTRACE(MatchExactGetIDsOfNames(pTI,
                                                         IID_NULL,
                                                         &bstrName,
                                                         1,
                                                         0,
                                                         pid,
                                                         !!(grfdex & fdexNameCaseSensitive)));
            }

            // We'll enter here if we didn't have a typelib or if
            // MatchExactGetIDsOfNames failed.
            if (hr)
            {
                // Do it the hardway no typelib at all or the typelib didn't
                // expose the prop/metho so lets try it this way.
                hr = THR_NOTRACE(pDisp->GetIDsOfNames(IID_NULL,
                                                      &bstrName,
                                                      1,
                                                      0,
                                                      pid));
            }

            ReleaseInterface(pTI);
        }
    }

Cleanup:
    RRETURN(hr);
}



HRESULT
#ifdef VSTUDIO7
CBase::GetExpandoDispID(LPOLESTR pchName, DISPID *pid, DWORD grfdex, CAttrArray *pAA /* = NULL */)
#else
CBase::GetExpandoDispID(BSTR bstrName, DISPID *pid, DWORD grfdex, CAttrArray *pAA /* = NULL */)
#endif //VSTUDIO7
{
    HRESULT         hr = S_OK;
    CAtomTable     *pat;
    BOOL            fCaseSensitive = (grfdex & fdexNameCaseSensitive) != 0;
    LONG            lIndex;
    BOOL            fFound = FALSE;

    // Assume failure.
    *pid = DISPID_UNKNOWN;
    hr = DISP_E_UNKNOWNNAME;

    // If pAA is not specified or is NULL use the calss memeber value
    if(!pAA) pAA = _pAA;

    // Look in Expando for this property.
    pat = GetAtomTable();
    if(!pat)
        goto Cleanup;

    lIndex = -1;
    do
    {
        lIndex++;
#ifdef VSTUDIO7
        hr = THR_NOTRACE(pat->GetAtomFromName(pchName, &lIndex, fCaseSensitive, TRUE));
#else
        hr = THR_NOTRACE(pat->GetAtomFromName(bstrName, &lIndex, fCaseSensitive, TRUE));
#endif //VSTUDIO7
        if (!hr)
        {
            *pid = lIndex;
            Assert(lIndex >= 0);

            // Make it an expando range.
            *pid += DISPID_EXPANDO_BASE;
            // Outside of range then we've got too many expandos.
            if (*pid > DISPID_EXPANDO_MAX)
            {
                *pid = DISPID_UNKNOWN;
                hr = DISP_E_UNKNOWNNAME;
                goto Cleanup;
            }

            // Verify that this expando truly does exist, a name in the atom
            // table doesn't guarantee that the name is expando.  Consider the
            // case of a named element.  During the event firing the implicit
            // this is the element which when called thru GetDispID will
            // find that name in this routine and assume that it was expando
            // unless we verify that the attrArray for this element truly has
            // this expando.
            if (FindAAIndex(*pid, CAttrValue::AA_Expando, AA_IDX_UNKNOWN, FALSE, pAA) != AA_IDX_UNKNOWN)
            {
                // It's an expando...we found it
                fFound = TRUE;
            }
            else
            {
                // Check if it could be an ActiveX expando?
                DISPID  dispIDactX;

                // remapped to ActiveX expando?
                dispIDactX = (*pid - DISPID_EXPANDO_BASE) + DISPID_ACTIVEX_EXPANDO_BASE;
                // Too many activeX expandos?
                if (dispIDactX <= DISPID_ACTIVEX_EXPANDO_MAX)
                {
                    if (FindAAIndex(dispIDactX, CAttrValue::AA_Expando, AA_IDX_UNKNOWN, FALSE, pAA) != AA_IDX_UNKNOWN)
                    {
                        fFound = TRUE;
                    }
                }
            }
        }
        // Atom tables are alway built case sensitive so if we did not found the name
        //  when doing a case insensitive search that may be because there different
        //  spellings of the same name in the atom table. We need to try them all
    } while(!fCaseSensitive && !fFound && hr == S_OK);

    if(!fFound)
    {
        if((grfdex & fdexNameEnsure) != 0)
        {
            // Add expando.
#ifdef VSTUDIO7
            hr = AddExpando(pchName, pid);
#else
            hr = AddExpando(bstrName, pid);
#endif //VSTUDIO7
        }
        else
        {
    // BUGBUG: ***TLL*** VT_NULL handling wont happen unless we can return S_OK here.
    //         temporary work around to allow foo() to return not defined.
            // Don't error just return success, however the
            // dispid returned is DISPID_UNKNOWN. CBase::Invoke
            // will take care of r-value case to return a
            // VT_NULL for l-value an error will occur.
            hr = DISP_E_UNKNOWNNAME;
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


HRESULT
CBase::GetInternalNextDispID(DWORD grfdex,
                             DISPID id,
                             DISPID *prgid,
                             BSTR *prgbstr,
                             IDispatch *pDisp /* = NULL */)
{
    HRESULT hr = S_FALSE;

    // check the outgoing parameters.
    if (prgid == NULL || prgbstr == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // set to the same as lastid
    *prgid = id;
    *prgbstr = NULL;

    if (IsExpandoDISPID(id))
    {
        goto ExpandoRange;
    }

    // Try to get element properties
    if (!pDisp || (pDisp && ((id & 0xFFFF0000) == DISPID_XOBJ_BASE || id == DISPID_STARTENUM)))
    {
        hr = NextProperty(id, prgbstr, prgid);

        // If we found some properties in NextProperty then we want to start
        // enumerating properties in the activeX control starting at the beginning.
        // The function NextProperty will return S_FALSE and *prgid == id if the
        // last property from NextProperty is id otherwise *prgid == DISPID_UNKNOWN.
        if (hr == S_FALSE && pDisp && *prgid == id)
        {
            id = DISPID_STARTENUM;
        }
    }

    // If we didn't get the current dispid asked for then try the pDisp typeinfo
    // if that was passed.
    if (hr == S_FALSE && pDisp)
    {
        IDispatchEx *pDispEx = NULL;

        hr = THR(pDisp->QueryInterface(IID_IDispatchEx, (void **)&pDispEx));
        if (S_OK == hr && pDispEx)
        {
            hr = THR(pDispEx->GetNextDispID(grfdex, id, prgid));
            ReleaseInterface(pDispEx);
        }
        else
            hr = NextTypeInfoProperty(pDisp, id, prgbstr, prgid);
    }

    // If we still don't have a current dispid asked for then try the expandos.
    if (hr == S_FALSE)
    {
        id = DISPID_STARTENUM;

ExpandoRange:
        hr = GetNextDispIDExpando(id, prgbstr, prgid);
    }

Cleanup:
    RRETURN1(hr, S_FALSE);

}


HRESULT
CBase::NextTypeInfoProperty(
            IDispatch *pDisp,
            DISPID id,
            BSTR *pbstrName,
            DISPID *pid)
{
    HRESULT         hr = S_OK;
    ITypeInfo      *pTI = NULL;
    CTypeInfoNav    tin;
    BOOL            ffound = FALSE;

    *pid = DISPID_UNKNOWN;

    // BUGBUG:  ***TLL*** Need optimization to have a special range for the
    //          object tag properties/attributes so we can ignore traversing
    //          activeX control properties and jump right to the CBase
    //          enumeration of expando or object tag/properties.

    if (pDisp)
    {
        // Loop thru ActiveX properties.
        hr = tin.InitIDispatch(pDisp, &pTI, 0);
        if (hr)
        {
            if (E_NOTIMPL == hr)
            {
                hr = S_FALSE;
            }
            goto Cleanup;
        }

        while ((hr = tin.Next()) == S_OK)
        {
            VARDESC    *pVar;
            FUNCDESC   *pFunc;

            if ((pVar = tin.getVarD()) != NULL)
            {
                if (!ffound)
                {
                    // Match or enumerating at beginning.
                    ffound = (pVar->memid == id) || (id == DISPID_STARTENUM);
                    if (ffound)
                    {
                        continue;
                    }
                }
                else
                {
                    // fill in the outgoing parameters
                    *pid = pVar->memid;
                    break;
                }
            }
            else if ((pFunc = tin.getFuncD()) != NULL)
            {
                if (!ffound)
                {
                    // Match or enumerating at beginning.
                    ffound = (pFunc->memid == id) || (id == DISPID_STARTENUM);
                    if (ffound)
                    {
                        continue;
                    }
                }
                else
                {
                    // Is the next a property get/put?
                    if ((pFunc->invkind == INVOKE_PROPERTYGET) ||
                        (pFunc->invkind == INVOKE_PROPERTYPUT))
                    {
                        // Might be 2 properties get/put
                        if (pFunc->memid != id)
                        {
                            // fill in the outgoing parameters
                            *pid = pFunc->memid;
                            break;
                        }
                        else
                        {
                            continue;
                        }
                    }
                    else
                    {
                        // No, continue until we find a property.
                        continue;
                    }
                }
            }
        }

        // If we succeede on getting a property from ITypeInfo then get the name.
        if (!hr && *pid != DISPID_UNKNOWN && pbstrName)
        {
            UINT         cNames;

            hr = pTI->GetNames(*pid, pbstrName, 1, &cNames);
            if (hr)
                goto Cleanup;

            Assert(cNames == 1);
            goto Cleanup;   // Everything is okay just return.
        }
        else
        {
            if (hr && *pid == DISPID_UNKNOWN)
            {
                // Nothing more to find.
                hr = S_FALSE;
            }
        }
    }

Cleanup:
    ReleaseInterface(pTI);

    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::AddExpando, helper
//
//  Synopsis:   Add an expando property to the attr array.
//
//  Results:    S_OK        - return dispid of found name
//              E_          - unable to add expando
//
//--------------------------------------------------------------------------

HRESULT
CBase::AddExpando (LPOLESTR pszExpandoName, DISPID *pdispID)
{
    HRESULT             hr;
    CAtomTable         *pat;
    BOOL                fExpando;

    pat = GetAtomTable(&fExpando);
    if (!pat || !fExpando)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    hr = pat->AddNameToAtomTable(pszExpandoName, pdispID);
    if (hr)
        goto Cleanup;

    // Make it an expando range.
    *pdispID += DISPID_EXPANDO_BASE;
    if (*pdispID > DISPID_EXPANDO_MAX)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

Cleanup:
    if (hr)
    {
        *pdispID = DISPID_UNKNOWN;
    }

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::SetExpando, helper
//
//  Synopsis:   Set an expando property to the attr array.
//
//--------------------------------------------------------------------------

HRESULT
CBase::SetExpando(LPOLESTR pszExpandoName, VARIANT * pvarPropertyValue)
{
    HRESULT hr;
    DISPID dispIDExpando;
    AAINDEX aaIdx;

    hr = AddExpando(pszExpandoName, &dispIDExpando);
    if (hr)
        goto Cleanup;

    aaIdx = FindAAIndex(dispIDExpando, CAttrValue::AA_Expando);
    if (aaIdx == AA_IDX_UNKNOWN)
        hr = AddVariant(dispIDExpando, pvarPropertyValue, CAttrValue::AA_Expando);
    else
        hr = ChangeVariantAt(aaIdx, pvarPropertyValue);

Cleanup:
    RRETURN(hr);
}


HRESULT
CBase::GetNextDispIDExpando(DISPID currDispID, BSTR *pStrNextName, DISPID *pNextDispID)
{
    HRESULT     hr = S_FALSE;
    AAINDEX     aaNextIdx;

    // Assumption at this point is that expandos are always unique.  Multiple
    // DISPIDs for AA_Expando is not valid.  If the currDispID is 0 then get
    // first occurrance otherwise get the next attribute after the currDispID.

    // Are we looking to return a name if so then currDispID needs to be
    // valid.
    if (currDispID == DISPID_STARTENUM)
    {
        // We need index of first expando.
        aaNextIdx = FindAAIndex(DISPID_UNKNOWN, CAttrValue::AA_Expando);
    }
    else
    {
        // Get index of current dispid.
        aaNextIdx = FindAAIndex(currDispID, CAttrValue::AA_Expando, AA_IDX_UNKNOWN);
        // Compute next expando.
        aaNextIdx = FindAAIndex(DISPID_UNKNOWN, CAttrValue::AA_Expando, aaNextIdx);
    }

    *pNextDispID = GetDispIDAt(aaNextIdx);

    if (*pNextDispID != DISPID_UNKNOWN)
    {
        // Look in Expando for this property.
        CAtomTable     *pat;
        LPCTSTR         pch = NULL;
        DISPID          didOffset;

        pat = GetAtomTable();
        if (!pat)
            goto Cleanup;

        // Compute which kind of expando it is ActiveX or element.
        didOffset = (*pNextDispID >= DISPID_ACTIVEX_EXPANDO_BASE &&
                     *pNextDispID <= DISPID_ACTIVEX_EXPANDO_MAX ) ?
                            DISPID_ACTIVEX_EXPANDO_BASE :
                            DISPID_EXPANDO_BASE;

        hr = THR_NOTRACE(pat->GetNameFromAtom(
                        *pNextDispID - didOffset, &pch));
        if (hr)
            goto Cleanup;

        if (pStrNextName)
        {
            hr = FormsAllocString(pch, pStrNextName);
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        // End of expando.
        hr = S_FALSE;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


HRESULT CBase::GetExpandoName ( DISPID expandoDISPID, LPCTSTR *lpPropName )
{
    HRESULT hr;

    Assert ( expandoDISPID >= DISPID_EXPANDO_BASE );

    // Get the name associated with the expando DISPID
    CAtomTable *pat = GetAtomTable();

    if (!pat)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }
    // When we put the dispid in the attr array we offset it by the expando base
    hr = THR(pat->GetNameFromAtom ( expandoDISPID-DISPID_EXPANDO_BASE, lpPropName ));
Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CBase::NextProperty, helper
//
//  Synopsis:   Loop through all property entries in ITypeInfo then enumerate
//              through all expando properties.  Any DISPID handed out here
//              must stay constant (GetIDsOfNamesEx should hand out the
//              same).  This routines enumerates regular properties and expando.
//
//  Results:
//              S_OK        - returns next DISPID after currDispID
//              S_FALSE     - enumeration is done
//              E_nnnn      - call failed.
//--------------------------------------------------------------------------

HRESULT
CBase::NextProperty (DISPID currDispID, BSTR *pStrNextName, DISPID *pNextDispID)
{
    HRESULT     hr = S_FALSE;

    Assert(pNextDispID);

    // Initialize to not found.  If result is S_FALSE and pNextDispID is
    // currDispID then the last known item was a property found in this routine,
    // however there isn't another property after this one.  If pNextDispID is
    // DISPID_UNKNOWN then the last known property wasn't fetched from this
    // routine.  This is important for activex controls so we know when to ask
    // for first property versus next property.
    *pNextDispID = DISPID_UNKNOWN;

    if (IsExpandoDISPID(currDispID))
    {
        goto Cleanup;
    }

    // Enumerate thru attributes and properties.
    if (pStrNextName)
    {
        const VTABLEDESC       *pVTblArray;
        const PROPERTYDESC     *ppropdesc;
        BOOL                    fPropertyFound = FALSE;
        DWORD                   dwppFlags;

        pVTblArray = GetVTableArray();
        if (!pVTblArray)
            goto Cleanup;

        while (pVTblArray->pPropDesc)
        {
            ppropdesc = pVTblArray->pPropDesc;

            dwppFlags = ppropdesc->GetPPFlags();

            // Only iterate over properties and items which are not hidden.
            if ((dwppFlags & (PROPPARAM_INVOKEGet | PROPPARAM_INVOKESet)) == 0 ||
                (dwppFlags & PROPPARAM_HIDDEN))
                 goto NextEntry;

            // Get the first property.
            if (currDispID == DISPID_STARTENUM)
            {
                fPropertyFound = TRUE;
            }
            else if (currDispID == ppropdesc->GetDispid())
            {
                // Found a match get the next property.
                fPropertyFound = TRUE;
                *pNextDispID = currDispID;  // Signal we found last one now we
                                            // we need to look for next property.
                 goto NextEntry;
            }

            // Look at this propertydesc?
            if (fPropertyFound)
            {
                // Yes, we're either the first one or this is the dispid we're
                // looking for.

                const TCHAR * pstr = ppropdesc->pstrExposedName ?
                                         ppropdesc->pstrExposedName :
                                         ppropdesc->pstrName;

                // If property has a leading underscore then don't display.
                if (pstr[0] == _T('_'))
                    goto NextEntry;

                // allocate the result string
                *pStrNextName = SysAllocString(pstr);
                if (*pStrNextName == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                // We're returning this ID.
                *pNextDispID = ppropdesc->GetDispid();

                hr = S_OK;
                break;
            }

NextEntry:
            pVTblArray++;;
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


AAINDEX
CBase::FindNextAttach(int idx, DISPID dispID)
{
    AAINDEX     aaidx = AA_IDX_UNKNOWN;
    int         cAttachEvts = 0;

    for (;;)
    {
        aaidx = FindNextAAIndex(dispID, CAttrValue::AA_AttachEvent, aaidx);
        if (aaidx == AA_IDX_UNKNOWN || idx == cAttachEvts)
            break;

        cAttachEvts++;
    };

    return aaidx;
}


HRESULT
CBase::GetTheDocument(IHTMLDocument2 **ppDoc)
{
    HRESULT         hr;
    IDispatchEx    *pDex = NULL;
    BSTR            bstr = NULL;
    CVariant        retVariant;
    DISPPARAMS      dp = g_Zero.dispparams;
    DISPID          dispid;

    Assert(ppDoc);

    hr = PrivateQueryInterface(IID_IHTMLDocument2, (void **)ppDoc);
    if (hr)
    {
        hr = PrivateQueryInterface(IID_IDispatchEx, (void **)&pDex);
        if (hr)
            goto Cleanup;

        hr = FormsAllocString(_T("document"), &bstr);
        if (hr)
            goto Cleanup;

        hr = pDex->GetDispID(bstr, GETMEMBER_CASE_SENSITIVE, &dispid);
        if (hr)
            goto Cleanup;

        hr = pDex->InvokeEx(dispid,
                            g_lcidUserDefault,
                            DISPATCH_PROPERTYGET,
                            &dp,
                            &retVariant,
                            NULL,
                            NULL);
        if (hr)
            goto Cleanup;

        *ppDoc = (IHTMLDocument2 *)(V_DISPATCH(&retVariant));

        (*ppDoc)->AddRef();       // Take ownership.
    }

Cleanup:
    FormsFreeString(bstr);
    ReleaseInterface(pDex);

    RRETURN(hr);
}


HRESULT 
CBase::FindEventName(ITypeInfo *pTISrc, DISPID dispid, BSTR *pBSTR)
{
    int         ievt;
    int         cbEvents;
    UINT        cbstr;
    HRESULT     hr;
    FUNCDESC   *pFDesc = NULL;
    TYPEATTR   *pTAttr = NULL;

    Assert(pTISrc);
    Assert(pBSTR);

    *pBSTR = NULL;

    hr = pTISrc->GetTypeAttr(&pTAttr);
    if (hr)
        goto Cleanup;

    if (pTAttr->typekind != TKIND_DISPATCH)
    {
        hr = E_NOINTERFACE;
        goto Cleanup;
    }

    // Total events.
    cbEvents = pTAttr->cFuncs;
    if (cbEvents == 0)
    {
        hr = E_NOINTERFACE;
        goto Cleanup;
    }

    // Populate the event table.
    for (ievt = 0; ievt < cbEvents; ++ievt)
    {
        hr = pTISrc->GetFuncDesc(ievt, &pFDesc);
        if (hr)
            goto Cleanup;

        // Did we find the event that fired?
        if (dispid == pFDesc->memid)
        {
            // Yes, so return the name.
            hr = pTISrc->GetNames(dispid, pBSTR, 1, &cbstr);
            goto Cleanup;
        }

        pTISrc->ReleaseFuncDesc(pFDesc);
        pFDesc = NULL;
    }

    hr = S_OK;

Cleanup:
    if (pFDesc)
        pTISrc->ReleaseFuncDesc(pFDesc);
    if (pTAttr)
        pTISrc->ReleaseTypeAttr(pTAttr);

    RRETURN(hr);
}


#ifdef WIN16
#pragma code_seg( "BASE_2_TEXT" )
#endif // WIN16

//+---------------------------------------------------------------------------
//
//  Member:     CBase::FireEvent, public
//
//  Synopsis:   Fires an event out the primary dispatch event connection point
//              and using the corresponding code pointers living in attr array
//
//  Arguments:  dispidEvent     Dispid of the event to fire
//              dispidProp      Dispid of property where event func is stored
//              pvarResult      Where to store the return value
//              pdispparams     Parameters for Invoke
//              pexcepinfo      Any exception info
//              puArgErr        Error argument
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

enum EVENTTYPE {
    EVENTTYPE_EMPTY = 0,
    EVENTTYPE_OLDSTYLE = 1,
    EVENTTYPE_VBHOOKUP = 2
};

enum EVENT_TYPE { VB_EVENTS, CPC_EVENTS };

HRESULT 
CBase::FireEvent(
    DISPID dispidEvent, 
    DISPID dispidProp, 
    VARIANT *pvarResult, 
    DISPPARAMS *pdispparams,
    EXCEPINFO *pexcepinfo,
    UINT *puArgErr,
    ITypeInfo *pTIEventSrc/* = NULL*/,
    IDispatch *pEventObject/* = NULL*/)
{
    AAINDEX                         aaidx;
    HRESULT                         hr = S_OK;
    IDispatch *                     pDisp = NULL;
    CVariant                        Var;
    CStackPtrAry<IDispatch *, 2>    arySinks(Mt(CBaseFireEventAry_pv));
    CStackDataAry<WORD, 2>          arySinkType(Mt(CBaseFireTypeAry_pv)); 
    long                            i;
    VARIANT                         varEO;                  // Don't release.
    VARIANT                        *pVarEventObject;
    CDispParams                     dispParams;
    BOOL                            f_CPCEvents = FALSE;
    EVENT_TYPE                      whichEventType;

    if (pvarResult)
    {
        VariantInit(pvarResult);
    }
    
    //
    //
    // First fire off any properties bound to this event.
    //

    if (dispidProp != DISPID_UNKNOWN)   // Skip property events
    {
        aaidx = FindAAIndex(dispidProp, CAttrValue::AA_Internal);
        if (AA_IDX_UNKNOWN != aaidx)
        {
            hr = THR(GetDispatchObjectAt(aaidx, &pDisp));
            if (!hr && pDisp)
            {
                CAttrValue *pAV = GetAttrValueAt(aaidx);

                pVarEventObject = NULL;

                if (pAV)
                {
                    // If not old style event, and we have an eventObject and we're not firing the onError
                    // event which already has arguments then prepare to pass the event object as an
                    // argument.
                    if (!pAV->IsOldEventStyle() && pEventObject && dispidProp != DISPID_EVPROP_ONERROR)
                    {
                        V_VT(&varEO) = VT_DISPATCH;
                        V_DISPATCH(&varEO) = pEventObject;
                        pVarEventObject = &varEO;;
                    }
                }            

                hr = THR(InvokeDispatchWithThis(
                    pDisp,
                    pVarEventObject,
                    IID_NULL,
                    g_lcidUserDefault,
                    DISPATCH_METHOD,
                    pdispparams,
                    &Var,
                    pexcepinfo,
                    NULL));

                ClearInterface(&pDisp);
        
                if (pvarResult)
                {
                    hr = THR(VariantCopy(pvarResult, &Var));
                    if (hr)
                        goto Cleanup;
                }
            }
        }

        // Fire any attach events.
        IGNORE_HR(FireAttachEvents(dispidProp, pdispparams, pvarResult, this, pexcepinfo, puArgErr, pEventObject));
    }

    //
    // Now fire off to anyone listening through the normal connection
    // points.  Remember that these are also just stored in the attr
    // array with a special internal dispid.
    //

    aaidx = AA_IDX_UNKNOWN;
    
    for (;;)
    {
        CAttrValue  *pAV;

        aaidx = FindNextAAIndex(
            DISPID_A_EVENTSINK, 
            CAttrValue::AA_Internal, 
            aaidx);
        if (aaidx == AA_IDX_UNKNOWN)
            break;

        Assert(!pDisp);

        pAV = GetAttrValueAt(aaidx);
        if (pAV)
        {
            if (OK(FetchObject(pAV, VT_UNKNOWN, (void **)&pDisp)) && pDisp)
            {
                WORD    wET;

                hr = THR(arySinks.Append(pDisp));
                if (hr)
                    goto Cleanup;

                wET  = pAV->IsOldEventStyle() ? EVENTTYPE_OLDSTYLE : EVENTTYPE_EMPTY;
                wET |= pAV->IsTridentSink()   ? EVENTTYPE_VBHOOKUP : EVENTTYPE_EMPTY;

                hr = THR(arySinkType.AppendIndirect(&wET, NULL));
                if (hr)
                    goto Cleanup;

                // Upon success arySinks takes over the ref of pDisp
                pDisp = NULL;
            }
        }
    }

    whichEventType = VB_EVENTS;

NextEventSet:

    for (i = 0; i < arySinks.Size(); i++)
    {
        DISPPARAMS *pPassedParams;

        VariantClear(&Var);
        
        // If not old style event, and we have an eventObject and we're not firing the onError
        // event which already has arguments then prepare to pass the event object as an
        // argument.
        if ((!(arySinkType[i] & EVENTTYPE_OLDSTYLE)) && pEventObject && dispidProp != DISPID_EVPROP_ONERROR)
        {
            // Only compute this once.
            if (dispParams.cArgs == 0)
            {
                UINT     uIdx;

                // Fix up pdispparam
                dispParams.Initialize(pdispparams->cArgs + 1, pdispparams->cNamedArgs);
                
                hr = dispParams.Create(pdispparams);
                if (hr)
                    goto Cleanup;

                // Now dispParams.cArgs is > 0.
                uIdx = dispParams.cArgs - 1;

                V_VT(&varEO) = VT_DISPATCH;
                V_DISPATCH(&varEO) = pEventObject;

                memcpy(&(dispParams.rgvarg[uIdx]), &varEO, sizeof(VARIANT));
            }

            pPassedParams = &dispParams;
        }
       else
            pPassedParams = pdispparams;

        if (arySinkType[i] & EVENTTYPE_VBHOOKUP)
        {
            // Are we firing VB_EVENTS?
            if (whichEventType == VB_EVENTS)
            {
                // Yes, so fire the event.
                PROPERTYDESC      *pDesc;
                ITridentEventSink *pTriSink = (ITridentEventSink *)(arySinks[i]);

                if (!FindPropDescFromDispID(dispidProp, (PROPERTYDESC **)&pDesc, NULL, NULL))
                {
                    LPCOLESTR pEventString = (LPCOLESTR)(pDesc->pstrExposedName ? pDesc->pstrExposedName
                                                                      : pDesc->pstrName);
                    IGNORE_HR(pTriSink->FireEvent(pEventString, pPassedParams, &Var, pexcepinfo));
                }
                else if (pTIEventSrc)
                {
                    BSTR    eventName;

                    hr = FindEventName(pTIEventSrc, dispidEvent, &eventName);

                    if (hr == S_OK && eventName)
                    {
                        // ActiveX events don't pass the eventObject argument.
                        IGNORE_HR(pTriSink->FireEvent(eventName, pdispparams, &Var, pexcepinfo));
                    }

                    FormsFreeString(eventName);
                }
            }
        }
        else
        {
            // Firing CPC events yet?
            if (whichEventType == CPC_EVENTS)
            {
                IGNORE_HR(arySinks[i]->Invoke(dispidEvent,
                                              IID_NULL,
                                              g_lcidUserDefault,
                                              DISPATCH_METHOD,
                                              pPassedParams,
                                              &Var,
                                              pexcepinfo,
                                              puArgErr));
            }
            else
            {
                // No, just flag we got some CPC events to fire.
                f_CPCEvents = TRUE;
            }
        }

        // If we had to pass the this parameter and we had more than
        // 1 parameter then copy the orginal back over (the args could have
        // been byref).
        if (pPassedParams != pdispparams && pPassedParams->cArgs > 1)
        {
            hr = dispParams.MoveArgsToDispParams(pdispparams, pdispparams->cArgs);
            if (hr)
                goto Cleanup;
        }

        //
        // Copy return value into pvarResult if there was one.
        //

        if (pvarResult && V_VT(&Var) != VT_EMPTY)
        {
            hr = THR(VariantCopy(pvarResult, &Var));
            if (hr)
                goto Cleanup;
        }
    }

    // Any CPC events?
    if (f_CPCEvents && whichEventType == VB_EVENTS)
    {
        // Yes, and we just finished VB Events so fire all CPC events.
        whichEventType = CPC_EVENTS;
        f_CPCEvents = FALSE;
        goto NextEventSet;
    }

    Assert((whichEventType == CPC_EVENTS && !f_CPCEvents) || whichEventType == VB_EVENTS);

Cleanup:
    ReleaseInterface(pDisp);
    arySinks.ReleaseAll();
    
    RRETURN(hr);
}


HRESULT 
CBase::FireAttachEvents(
    DISPID          dispidProp, 
    DISPPARAMS *    pdispparams,
    VARIANT *       pvarResult,
    CBase *         pDocAccessObject,
    EXCEPINFO *     pexcepinfo,
    UINT *          puArgErr,
    IDispatch      *pEventObj)
{
    int                             iNextIdx;
    IDispatch                      *pDisp = NULL;
    CVariant                        Var;
    AAINDEX                         aaidx;
    HRESULT                         hr = S_OK;
    VARIANT                         varEventObj;
    EXCEPINFO                       excepinfo;
    UINT                            uArgErr;
    CStackPtrAry<IDispatch *, 2>    arySinks(Mt(CBaseFireEventAry_pv));

    if (!pDocAccessObject)
        pDocAccessObject = this;

    if (!pexcepinfo)
        pexcepinfo = &excepinfo;

    if (!puArgErr)
        puArgErr = &uArgErr;

    if (dispidProp != DISPID_UNKNOWN)   // Skip property events
    {
        // Fire all functions hooked via attachEvent.
        iNextIdx = 0;

        for (;;)
        {
            aaidx = FindNextAttach(iNextIdx, dispidProp);
            if (aaidx == AA_IDX_UNKNOWN)
                break;

            hr = THR(GetDispatchObjectAt(aaidx, &pDisp));
            if (hr)
                goto Cleanup;

            hr = THR(arySinks.Append(pDisp));
            if (hr)
                goto Cleanup;

            iNextIdx++;
        }

        V_DISPATCH(&varEventObj) = pEventObj;
        V_VT(&varEventObj) = VT_DISPATCH;

        // Fire the events:
        for (int i = 0; i < arySinks.Size(); i++)
        {
            if (arySinks[i])
            {
                hr = THR_NOTRACE(InvokeDispatchExtraParam(
                    arySinks[i],
                    IID_NULL,
                    g_lcidUserDefault,
                    DISPATCH_METHOD,
                    pdispparams,
                    &Var,
                    pexcepinfo,
                    NULL,
                    &varEventObj));

                if (pvarResult && V_VT(&Var) != VT_EMPTY)
                {
                    hr = THR(VariantCopy(pvarResult, &Var));
                    if (hr)
                        goto Cleanup;
                }
            }

            Var.Clear();
        }
    }

Cleanup:
    arySinks.ReleaseAll();

    RRETURN(hr);
}


HRESULT
CBase::FireAttachEventV(
    DISPID          dispidEvent,
    DISPID          dispidProp,
    IDispatch *     pEventObj,
    VARIANT *       pvRes,
    CBase *         pDocAccessObject,
    BYTE *          pbTypes,
    va_list         valParms)
{
    HRESULT         hr;
    DISPPARAMS      dp;
    VARIANTARG      av[EVENTPARAMS_MAX];

    dp.rgvarg = av;
    CParamsToDispParams(&dp, pbTypes, valParms);

    hr = THR(FireAttachEvents(dispidProp, &dp, pvRes, pDocAccessObject, NULL, NULL, pEventObj));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CBase::FireEventV, public
//
//  Synopsis:   Fires an event out the primary dispatch event connection point
//              and using the corresponding code pointers living in attr array
//
//  Arguments:  [dispidEvent]  -- DISPID of event to fire
//              [dispidProp]   -- dispid of property where event ptr is stored
//              [pvbRes]       -- Resultant boolean return value
//              [pbTypes]      -- Pointer to array giving the types of params
//              [va_list]      -- Parameters
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CBase::FireEventV(
    DISPID          dispidEvent,
    DISPID          dispidProp,
    IDispatch    *  pEventObject,
    VARIANT      *  pvRes,
    BYTE *          pbTypes,
    va_list         valParms)
{
    HRESULT         hr;
    DISPPARAMS      dp;
    VARIANTARG      av[EVENTPARAMS_MAX];
    CExcepInfo      ei;
    UINT            uArgErr;

    dp.rgvarg = av;
    CParamsToDispParams(&dp, pbTypes, valParms);

    hr = THR(FireEvent(dispidEvent, dispidProp, pvRes, &dp, &ei, &uArgErr, NULL, pEventObject));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CBase::FirePropertyNotify, public
//
//----------------------------------------------------------------------------

HRESULT
CBase::FirePropertyNotify(DISPID dispid, BOOL fOnChanged)
{
    IPropertyNotifySink *   pPNS = NULL;
    HRESULT                 hr = S_OK;
    AAINDEX                 aaidx = AA_IDX_UNKNOWN;
    CStackPtrAry<IPropertyNotifySink *, 2>  arySinks(Mt(CBaseFirePropertyNotify_pv));
    long                    i;

    //
    // Take sinks local first, and then fire to avoid reentrancy problems.
    //
    
    for (;;)
    {
        aaidx = FindNextAAIndex(
            DISPID_A_PROPNOTIFYSINK, 
            CAttrValue::AA_Internal, 
            aaidx);
        if (aaidx == AA_IDX_UNKNOWN)
            break;
            
        Assert(!pPNS);
        if (OK(GetUnknownObjectAt(aaidx, (IUnknown **)&pPNS)) && pPNS)
        {
            hr = THR(arySinks.Append(pPNS));
            if (hr)
                goto Cleanup;

            pPNS = NULL;   // arySinks has taken over ref.
        }
    }

    for (i = 0; i < arySinks.Size(); i++)
    {
        if (fOnChanged)
        {
            IGNORE_HR(arySinks[i]->OnChanged(dispid));
        }
        else
        {
            hr = THR_NOTRACE(arySinks[i]->OnRequestEdit(dispid));
            if (!hr)
            {
                //
                // Somebody doesn't want the change.
                //
                break;
            }
        }
    }
    
Cleanup:
    ReleaseInterface(pPNS);
    arySinks.ReleaseAll();
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::FireCancelableEvent
//
//  Synopsis:   Tries to fire the event and returns the script return value that
//              is a combination of returned value and varReturnValue (if present)
//              If one of them is TRUE (that means no default action for onerrer)
//              TRUE is returned. If no value is returned from the script the
//               *pfRet is not changed.
//--------------------------------------------------------------------------

HRESULT
CBase::FireCancelableEvent(DISPID dispidMethod,
                           DISPID dispidProp,
                           IDispatch *pEventObject,
                           VARIANT_BOOL *pfRetVal,
                           BYTE * pbTypes,
                           ...)
{
    va_list     valParms;
    CVariant    Var;
    HRESULT     hr;

    va_start(valParms, pbTypes);
    hr = THR(FireEventV(dispidMethod, dispidProp, pEventObject, &Var, pbTypes, valParms));
    va_end(valParms);

    if (pfRetVal != NULL)
    {
        if (V_VT(&Var) == VT_BOOL) 
            *pfRetVal = V_BOOL(&Var);
    }

    RRETURN(hr);
}



//+-------------------------------------------------------------------------
//
//  Method:     CBase::GetEnabled
//
//  Synopsis:   Helper method.  Many objects will simply override this.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CBase::GetEnabled(VARIANT_BOOL * pfEnabled)
{
    if (!pfEnabled)
        RRETURN(E_INVALIDARG);

    *pfEnabled = VB_TRUE;
    return DISP_E_MEMBERNOTFOUND;
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::GetValid
//
//  Synopsis:   Helper method.  Many objects will simply override this.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CBase::GetValid(VARIANT_BOOL * pfValid)
{
    if (!pfValid)
        RRETURN(E_INVALIDARG);

    *pfValid = VB_TRUE;
    return DISP_E_MEMBERNOTFOUND;
}


////////////////////////////////////////////////////////////////////////////////
//
//  AttrArray helpers:
//
////////////////////////////////////////////////////////////////////////////////


AAINDEX
CBase::FindAAType(CAttrValue::AATYPE aaType, AAINDEX lastIndex)
{
    AAINDEX newIdx = AA_IDX_UNKNOWN;

    if (_pAA)
    {
        int nPos;

        if (lastIndex == AA_IDX_UNKNOWN)
        {
            nPos = 0;
        }
        else
        {
            nPos = ++lastIndex;
        }

        while (nPos < _pAA->Size())
        {
            CAttrValue *pAV = (CAttrValue *)(*_pAA) + nPos;
            // Logical & so we can find more than one type
            if (pAV->AAType() == aaType)
            {
                newIdx = (AAINDEX)nPos;
                break;
            }
            else
            {
                nPos++;
            }
        }
    }
    return newIdx;
}


BOOL
CBase::DidFindAAIndexAndDelete (DISPID dispID, CAttrValue::AATYPE aaType)
{
    BOOL    fRetValue = TRUE;
    AAINDEX aaidx;

    aaidx = FindAAIndex (dispID, aaType);

    if (AA_IDX_UNKNOWN != aaidx)
        DeleteAt (aaidx);
    else
        fRetValue = FALSE;

    return fRetValue;
}


//+------------------------------------------------------------------------
//
//  Member:     CBase::FindNextAAIndex
//
//  Synopsis:   Find next AAIndex with given dispid and aatype
//
//  Arguments:  dispid      Dispid to look for
//              aatype      AAType to look for
//              paaIdx      The aaindex of the last entry, if AA_IDX_UNKNOWN
//                          will return the 
//
//-------------------------------------------------------------------------

AAINDEX
CBase::FindNextAAIndex(
    DISPID              dispid, 
    CAttrValue::AATYPE  aaType, 
    AAINDEX             aaIndex)
{
    CAttrValue *    pAV;

    if (AA_IDX_UNKNOWN == aaIndex)
    {
        return FindAAIndex(dispid, aaType, AA_IDX_UNKNOWN, TRUE);
    }

    aaIndex++;
    if (!_pAA || aaIndex >= (ULONG)_pAA->Size())
        return AA_IDX_UNKNOWN;

    pAV = _pAA->FindAt(aaIndex);
    if (!pAV || pAV->GetDISPID() != dispid || pAV->GetAAType() != aaType)
        return AA_IDX_UNKNOWN;

    return aaIndex;
}


HRESULT
CBase::GetStringAt(AAINDEX aaIdx, LPCTSTR *ppStr)
{
    HRESULT       hr = DISP_E_MEMBERNOTFOUND;
    CAttrValue   *pAV;

    Assert(ppStr);

    *ppStr = NULL;

    // Any attr array?
    if (_pAA)
    {
        pAV = _pAA->FindAt(aaIdx);
        // Found AttrValue?
        if (pAV)
        {
            *ppStr = pAV->GetLPWSTR();
            hr = S_OK;
        }
    }

    RRETURN(hr);
}

HRESULT
CBase::GetIntoBSTRAt(AAINDEX aaIdx, BSTR *pbStr )
{
    HRESULT             hr = DISP_E_MEMBERNOTFOUND;
    CAttrValue   *pAV;

    Assert(pbStr);

    *pbStr = NULL;

    // Any attr array?
    if (_pAA)
    {
        pAV = _pAA->FindAt(aaIdx);
        // Found AttrValue?
        if (pAV)
        {
            hr = S_OK;
            switch ( pAV->GetAVType() )
            {
            case VT_LPWSTR:
                hr = THR(FormsAllocString ( pAV->GetLPWSTR(), pbStr ));
                break;

            default:
                {
                    VARIANT varDest;
                    VARIANT varSrc;
                    varDest.vt = VT_EMPTY;
                    pAV->GetAsVariantNC(&varSrc);
                    hr = THR(VariantChangeTypeSpecial ( &varDest, &varSrc, VT_BSTR ));
                    if ( hr )
                    {
                        VariantClear ( &varDest );
                        if ( hr == DISP_E_TYPEMISMATCH )
                            hr = S_FALSE;
                        goto Cleanup;
                    }
                    *pbStr = V_BSTR ( &varDest );
                }
                break;
            }
        }
    }
Cleanup:
    RRETURN(hr);
}


HRESULT
CBase::GetIntoStringAt(AAINDEX aaIdx, BSTR *pbStr, LPCTSTR *ppStr)
{
    HRESULT             hr = DISP_E_MEMBERNOTFOUND;
    CAttrValue   *pAV;

    Assert(ppStr);
    Assert (pbStr);

    *ppStr = NULL;
    *pbStr = NULL;

    // Any attr array?
    if (_pAA)
    {
        pAV = _pAA->FindAt(aaIdx);
        // Found AttrValue?
        if (pAV)
            hr = pAV->GetIntoString( pbStr, ppStr );
    }

    RRETURN(hr);
}


HRESULT
CBase::GetPointerAt(AAINDEX aaIdx, void **ppv)
{
    HRESULT       hr = DISP_E_MEMBERNOTFOUND;
    CAttrValue   *pAV;

    Assert(ppv);

    *ppv = NULL;

    // Any attr array?
    if (_pAA)
    {
        pAV = _pAA->FindAt(aaIdx);
        // Found AttrValue?
        if (pAV)
        {
            *ppv = pAV->GetPointer();
            hr = S_OK;
        }
    }

    RRETURN(hr);
}

HRESULT
CBase::GetVariantAt(AAINDEX aaIdx, VARIANT *pVar, BOOL fAllowNullVariant)
{
    HRESULT             hr = S_OK;
    const CAttrValue   * pAV;

    Assert(pVar);

    // Don't have an attr array or attr value will return
    // NULL variant.  Note, this will result in the return result
    // being a null.

    // Any attr array?
    if (_pAA &&  ( ( pAV = _pAA->FindAt(aaIdx) ) != NULL ) )
    {
        hr = pAV->GetIntoVariant(pVar);
        if (hr && fAllowNullVariant)
        {
            pVar->vt = VT_NULL;
        }
    }
    else
    {
        if (fAllowNullVariant)
        {
            pVar->vt = VT_NULL;
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
    }

    RRETURN(hr);
}


HRESULT
CBase::FetchObject(CAttrValue *pAV, VARTYPE vt, void **ppvoid)
{
    HRESULT       hr = DISP_E_MEMBERNOTFOUND;

    Assert(ppvoid);

    // Found AttrValue?
    if (pAV)
    {
        if (pAV->GetAVType() == vt && vt == VT_UNKNOWN)
        {
            *ppvoid = (void *)pAV->GetUnknown();
            pAV->GetUnknown()->AddRef();
        }
        else if (pAV->GetAVType() == vt && vt == VT_DISPATCH)
        {
            *ppvoid = (void *)pAV->GetDispatch();
            pAV->GetDispatch()->AddRef();
        }

        hr = S_OK;
    }

    RRETURN(hr);
}


HRESULT
CBase::GetObjectAt(AAINDEX aaIdx, VARTYPE vt, void **ppvoid)
{
    HRESULT       hr = DISP_E_MEMBERNOTFOUND;
    CAttrValue   *pAV;

    Assert(ppvoid);

    // Don't have an attr array or attr value will return
    // NULL IUnknown *.
    *ppvoid = NULL;

    // Any attr array?
    if (_pAA)
    {
        pAV = _pAA->FindAt(aaIdx);
        hr = FetchObject(pAV, vt, ppvoid);
    }

    RRETURN(hr);
}

#ifdef _WIN64
HRESULT
CBase::GetCookieAt(AAINDEX aaIdx, DWORD * pdwCookie)
{
    HRESULT       hr = DISP_E_MEMBERNOTFOUND;
    CAttrValue   *pAV;

    Assert(pdwCookie);

    // Don't have an attr array or attr value will return
    // NULL IUnknown *.
    *pdwCookie = 0;

    // Any attr array?
    if (_pAA)
    {
        pAV = _pAA->FindAt(aaIdx);
        if (pAV)
        {
            *pdwCookie = pAV->GetCookie();
            hr = S_OK;
        }
    }

    RRETURN(hr);
}

HRESULT
CBase::SetCookieAt(AAINDEX aaIdx, DWORD dwCookie)
{
    HRESULT       hr = DISP_E_MEMBERNOTFOUND;
    CAttrValue   *pAV;

    // Any attr array?
    if (_pAA)
    {
        pAV = _pAA->FindAt(aaIdx);
        if (pAV)
        {
            pAV->SetCookie(dwCookie);
            hr = S_OK;
        }
    }

    RRETURN(hr);
}
#endif

CAttrValue::AATYPE
CBase::GetAAtypeAt(AAINDEX aaIdx)
{
    CAttrValue::AATYPE aaType = CAttrValue::AA_Undefined;

    // Any attr array?
    if (_pAA)
    {
        CAttrValue     *pAV;

        pAV = _pAA->FindAt(aaIdx);
        // Found AttrValue?
        if (pAV)
        {
            aaType = pAV->GetAAType();
        }
    }

    return aaType;
}

UINT
CBase::GetVariantTypeAt(AAINDEX aaIdx)
{
    if (_pAA)
    {
        CAttrValue     *pAV;

        pAV = _pAA->FindAt(aaIdx);
        // Found AttrValue?
        if (pAV)
        {
            return pAV->GetAVType();
        }
    }

    return VT_EMPTY;
}


HRESULT
CBase::AddSimple(DISPID dispID, DWORD dwSimple, CAttrValue::AATYPE aaType)
{
    VARIANT varNew;

    varNew.vt = VT_I4;
    varNew.lVal = (long)dwSimple;

    RRETURN(CAttrArray::Set(&_pAA, dispID, &varNew, NULL, aaType));
}

HRESULT
CBase::AddAttrArray(DISPID dispID,
                  CAttrArray *pAttrArray,
                  CAttrValue::AATYPE aaType )
{
    VARIANT varNew;

    varNew.vt = CAttrValue::VT_ATTRARRAY;
    varNew.byref = pAttrArray;

    RRETURN(CAttrArray::Set(&_pAA, dispID, &varNew, NULL, aaType));
}

HRESULT
CBase::AddString(DISPID dispID, LPCTSTR pch, CAttrValue::AATYPE aaType)
{
    VARIANT varNew;

    varNew.vt = VT_LPWSTR;
    varNew.byref = (LPVOID)pch;

    RRETURN(CAttrArray::Set(&_pAA, dispID, &varNew, NULL, aaType));
}

HRESULT
CBase::AddPointer(DISPID dispID, void *pValue, CAttrValue::AATYPE aaType)
{
    VARIANT varNew;

    varNew.vt = VT_PTR;
    varNew.byref = pValue;

    RRETURN(CAttrArray::Set(&_pAA, dispID, &varNew, NULL, aaType));
}

HRESULT
CBase::AddBSTR (DISPID dispID, LPCTSTR pch, CAttrValue::AATYPE aaType)
{
    VARIANT varNew;
    HRESULT hr;

    varNew.vt = VT_BSTR;
    hr = THR(FormsAllocString ( pch, &V_BSTR(&varNew) ));
    if ( hr )
        goto Cleanup;

    hr = THR(CAttrArray::Set(&_pAA, dispID, &varNew, NULL, aaType));
    if ( hr )
    {
        FormsFreeString ( V_BSTR(&varNew) );
    }
Cleanup:
    RRETURN(hr);
}

HRESULT
CBase::AddVariant(DISPID dispID, VARIANT *pVar, CAttrValue::AATYPE aaType)
{
    RRETURN(CAttrArray::Set(&_pAA, dispID, pVar, NULL, aaType));
}


HRESULT
CBase::AddUnknownObject(DISPID dispID, IUnknown *pUnk, CAttrValue::AATYPE aaType)
{
    VARIANT varNew;

    varNew.vt = VT_UNKNOWN;
    varNew.punkVal = pUnk;

    RRETURN(CAttrArray::Set(&_pAA, dispID, &varNew, NULL, aaType));
}


//+---------------------------------------------------------------------------------
//
//  Member:     CBase::AddUnknownObjectMultiple
//
//  Synopsis:   Add an object to the attr array allowing for multiple
//              entries at this dispid.
//
//----------------------------------------------------------------------------------

HRESULT
CBase::AddUnknownObjectMultiple(
    DISPID dispID, 
    IUnknown *pUnk, 
    CAttrValue::AATYPE aaType,
    CAttrValue::AAExtraBits wFlags/* = CAttrValue::AA_Extra_Empty*/)
{
    VARIANT var;
    
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = pUnk;
    
    RRETURN(CAttrArray::Set(&_pAA, dispID, &var, NULL, aaType, wFlags, TRUE));
}


//+---------------------------------------------------------------------------------
//
//  Member:     CBase::AddDispatchObjectMultiple
//
//  Synopsis:   Add an object to the attr array allowing for multiple
//              entries at this dispid.
//
//----------------------------------------------------------------------------------

HRESULT
CBase::AddDispatchObjectMultiple(
    DISPID dispID, 
    IDispatch *pDisp, 
    CAttrValue::AATYPE aaType)
{
    VARIANT var;
    
    V_VT(&var) = VT_DISPATCH;
    V_UNKNOWN(&var) = pDisp;
    
    RRETURN(CAttrArray::Set(&_pAA, dispID, &var, NULL, aaType, 0, TRUE));
}


HRESULT
CBase::AddDispatchObject(DISPID dispID, IDispatch *pDisp, CAttrValue::AATYPE aaType)
{
    VARIANT varNew;

    varNew.vt = VT_DISPATCH;
    varNew.pdispVal = pDisp;

    RRETURN(CAttrArray::Set(&_pAA, dispID, &varNew, NULL, aaType));
}


HRESULT
CBase::StoreEventsToHook(InlineEvts *pInlineEvts)
{
    HRESULT     hr = S_OK;
    CAttrValue *pAAHeader;

    if (!_pAA)
    {
        _pAA = new CAttrArray;
        if (!_pAA)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    pAAHeader = _pAA->EnsureHeader();
    if (pAAHeader)
    {
        pAAHeader->SetEventsToHook(pInlineEvts);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}


InlineEvts *
CBase::GetEventsToHook()
{
    InlineEvts     *pEvts = NULL;
    CAttrValue     *pAAHeader;

    if (_pAA)
    {
        pAAHeader = _pAA->EnsureHeader(FALSE);
        if (pAAHeader)
        {
            pEvts = pAAHeader->GetEventsToHook();
        }
    }

    return pEvts;
}


HRESULT
CBase::ChangeSimpleAt(AAINDEX aaIdx, DWORD dwSimple)
{
    VARIANT varNew;

    varNew.vt = VT_I4;
    varNew.lVal = (long)dwSimple;

    RRETURN(_pAA ? _pAA->SetAt(aaIdx, &varNew) : DISP_E_MEMBERNOTFOUND);
}


HRESULT
CBase::ChangeStringAt(AAINDEX aaIdx, LPCTSTR pch)
{
    VARIANT varNew;

    varNew.vt = VT_LPWSTR;
    varNew.byref = (LPVOID)pch;

    RRETURN(_pAA ? _pAA->SetAt(aaIdx, &varNew) : DISP_E_MEMBERNOTFOUND);
}


HRESULT
CBase::ChangeUnknownObjectAt(AAINDEX aaIdx, IUnknown *pUnk)
{
    VARIANT varNew;

    varNew.vt = VT_UNKNOWN;
    varNew.punkVal = pUnk;

    RRETURN(_pAA ? _pAA->SetAt(aaIdx, &varNew) : DISP_E_MEMBERNOTFOUND);
}


HRESULT
CBase::ChangeDispatchObjectAt(AAINDEX aaIdx, IDispatch *pDisp)
{
    VARIANT varNew;

    varNew.vt = VT_DISPATCH;
    varNew.pdispVal = pDisp;

    RRETURN(_pAA ? _pAA->SetAt(aaIdx, &varNew) : DISP_E_MEMBERNOTFOUND);
}


HRESULT
CBase::ChangeVariantAt(AAINDEX aaIdx, VARIANT *pVar)
{
    RRETURN(_pAA ? _pAA->SetAt(aaIdx, pVar) : DISP_E_MEMBERNOTFOUND);
}


HRESULT
CBase::ChangeAATypeAt(AAINDEX aaIdx, CAttrValue::AATYPE aaType)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;

    // Any attr array?
    if (_pAA)
    {
        CAttrValue     *pAV;

        pAV = _pAA->FindAt(aaIdx);
        // Found AttrValue?
        if (pAV)
        {
            pAV->SetAAType ( aaType );
            hr = S_OK;
        }
    }

    RRETURN(hr);
}

const VTABLEDESC *
CBase::FindVTableEntryForName (LPCTSTR szName, BOOL fCaseSensitive, WORD *pVTblOffset)
{
    const VTABLEDESC * found = NULL;
    int nVTblEntries = GetVTableCount();

    if (nVTblEntries)
    {
        int r;
        const VTABLEDESC *pLow = GetVTableArray();
        const VTABLEDESC *pHigh;
        const VTABLEDESC *pMid;

        pHigh = pLow + nVTblEntries - 1;

        // Binary search for property name
        while (pHigh >= pLow)
        {
            if (pHigh != pLow)
            {
                pMid = pLow + (((pHigh - pLow) + 1) >> 1);
                r = StrCmpIC(szName, pMid->pPropDesc->pstrExposedName ?
                                            pMid->pPropDesc->pstrExposedName :
                                            pMid->pPropDesc->pstrName);
                if (r < 0)
                {
                    pHigh = pMid - 1;
                }
                else if (r > 0)
                {
                    pLow = pMid + 1;
                }
                else
                {
                    // If case sensitive then make sure it really matches.
                    if (fCaseSensitive)
                    {
                        if (StrCmpC(szName, pMid->pPropDesc->pstrExposedName ?
                                              pMid->pPropDesc->pstrExposedName :
                                              pMid->pPropDesc->pstrName))
                        {
                            // Didn't match exactly, return not found.
                            found = NULL;
                            break;
                        }
                    }

                    found = pMid;
                    break;
                }
            }
            else 
            {
                STRINGCOMPAREFN pfnCompareString = fCaseSensitive
                                                   ? StrCmpC : StrCmpIC;

                found = pfnCompareString( szName,
                                          pLow->pPropDesc->pstrExposedName ?
                                          pLow->pPropDesc->pstrExposedName :
                                          pLow->pPropDesc->pstrName)
                        ? NULL : pLow;
                break;
            }
        }
    }

    return found;
}


#define GETMEMBER_CASE_SENSITIVE    0x00000001
#define GETMEMBER_AS_SOURCE         0x00000002
#define GETMEMBER_ABSOLUTE          0x00000004

HRESULT STDMETHODCALLTYPE
CBase::getAttribute(BSTR bstrPropertyName, LONG lFlags, VARIANT *pvarPropertyValue)
{
    HRESULT hr;
    DISPID dispID;
    DISPPARAMS dp = g_Zero.dispparams;
    CVariant varNULL(VT_NULL);
    PROPERTYDESC *propDesc = NULL;
    IDispatchEx *pDEX = NULL;

    if ( !bstrPropertyName || !pvarPropertyValue )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pvarPropertyValue->vt = VT_NULL;

    hr = THR(PrivateQueryInterface ( IID_IDispatchEx, (void**)&pDEX ));
    if ( hr )
        goto Cleanup;

    hr = pDEX->GetDispID(bstrPropertyName, lFlags & GETMEMBER_CASE_SENSITIVE ?
                                                fdexNameCaseSensitive : 0,
                         &dispID);
    if ( hr )
        goto Cleanup;

    // If we're asked for the SaveAs value - our best guess is to grab the attr array value. This won't
    // always work - but it's the best we can do!
    if ( lFlags & GETMEMBER_AS_SOURCE || lFlags & GETMEMBER_ABSOLUTE )
    {
        // Here we pretty much do what we do at Save time. The only difference is we don't do the GetBaseObject call
        // so certain re-directed properties won't work - e.g. the BODY onXXX properties
        if (IsExpandoDISPID(dispID, &dispID))
        {
            hr = GetIntoBSTRAt ( FindAAIndex ( dispID, CAttrValue::AA_Expando ),
                &V_BSTR(pvarPropertyValue) );
        }
        else
        {
            // Try the Unknown value first...
            hr = GetIntoBSTRAt ( FindAAIndex ( dispID, CAttrValue::AA_UnknownAttr ),
                &V_BSTR(pvarPropertyValue) );
            if ( hr == DISP_E_MEMBERNOTFOUND )
            {
                // No Unknown - try getting the attribute as it would be saved
                hr = FindPropDescFromDispID ( dispID, &propDesc, NULL, NULL );
                Assert(propDesc);
                if (!hr && propDesc->pfnHandleProperty)
                {
                    if (propDesc->pfnHandleProperty) 
                    {
                        hr = propDesc->HandleGetIntoAutomationVARIANT(this, pvarPropertyValue);

                        // This flag only works for URL properties we want to
                        // return the fully expanded URL.

                        if (lFlags & GETMEMBER_ABSOLUTE &&
#ifdef _MAC
                            propDesc->pfnHandleProperty == &PROPERTYDESC::HandleUrlProperty &&
#else
                            propDesc->pfnHandleProperty == PROPERTYDESC::HandleUrlProperty &&
#endif
                            !hr)
                        {
                            hr = ExpandedRelativeUrlInVariant(pvarPropertyValue);
                        }

                        goto Cleanup;
                    }
                    else
                        hr = DISP_E_UNKNOWNNAME;
                }
            }
        }
        
        if (!hr)
        {
            pvarPropertyValue->vt = VT_BSTR;
        }
    }
    else
    {
        // Need to check for an unknown first, because the regular get_'s will return
        // a default. No point in doing this if we're looking at an expando
        if (!IsExpandoDISPID(dispID))
        {
            hr = GetIntoBSTRAt(FindAAIndex(dispID, CAttrValue::AA_UnknownAttr),
                               &V_BSTR(pvarPropertyValue));
            // If this worked - we're done.
            if (!hr)
            {
                pvarPropertyValue->vt = VT_BSTR;
                goto Cleanup;
            }
        } 

        // See if we can get a regular property or expando
        hr = THR(pDEX->InvokeEx(dispID,
                                g_lcidUserDefault,
                                DISPATCH_PROPERTYGET,
                                &dp,
                                pvarPropertyValue,
                                NULL,
                                NULL));
    }


Cleanup:
    ReleaseInterface ( pDEX );
    if ( hr == DISP_E_UNKNOWNNAME || hr == DISP_E_MEMBERNOTFOUND)
    {
        // Couldn't find property - return a Null rather than an error
        hr = S_OK;
    }
    RRETURN ( SetErrorInfo ( hr ) );
}


HRESULT STDMETHODCALLTYPE
CBase::removeAttribute(BSTR strPropertyName, LONG lFlags, VARIANT_BOOL *pfSuccess)
{
    DISPID dispID;
    IDispatchEx *pDEX = NULL;

    if (pfSuccess)
        *pfSuccess = VB_FALSE;

#if TREE_SYNC
    // this is a big HACK, to temporarily get markup-sync working for netdocs.  it
    // will totally change in the future.
    // trident team: do not bother trying to maintain this code, delete if it causes problems
    THR(CMkpSyncLogger::StaticSetAttributeHandler(this,strPropertyName,NULL/*remove*/,lFlags));
#endif // TREE_SYNC

    if (THR_NOTRACE(PrivateQueryInterface ( IID_IDispatchEx, (void**)&pDEX )))
        goto Cleanup;

    if (THR_NOTRACE(pDEX->GetDispID(strPropertyName, lFlags & GETMEMBER_CASE_SENSITIVE ?
                                                    fdexNameCaseSensitive : 0, &dispID)))
        goto Cleanup;

    if (!removeAttributeDispid(dispID))
        goto Cleanup;

    if (pfSuccess)
        *pfSuccess = VB_TRUE;

Cleanup:
    ReleaseInterface ( pDEX );

    RRETURN ( SetErrorInfo ( S_OK ) );
}

BOOL
CBase::removeAttributeDispid(DISPID dispid, const PROPERTYDESC *pPropDesc /*=NULL*/)
{
    AAINDEX aaIx;
    BOOL fExpando;
    CVariant varNULL(VT_NULL);

    fExpando = IsExpandoDISPID(dispid);

    if(dispid != STDPROPID_XOBJ_STYLE)
    {
        aaIx = FindAAIndex ( dispid, fExpando ?
                CAttrValue::AA_Expando :
                CAttrValue::AA_Attribute);
    }
    else
    {
        // We cannot delete the inline style object. It is created "on demand".
        // We need to remove it's attrarray that stored on the element.
        aaIx = FindAAIndex ( DISPID_INTERNAL_INLINESTYLEAA, CAttrValue::AA_AttrArray );
    }

    if (aaIx == AA_IDX_UNKNOWN)
        return FALSE;

#ifndef NO_EDIT
    // BUGBUG: handle expando undo (jbeda)
    if (!fExpando && QueryCreateUndo(TRUE, FALSE))
    {
        HRESULT     hr;
        CVariant    varOld;
        IDispatch * pDisp = NULL;

        hr = THR(PunkOuter()->QueryInterface(IID_IDispatch, (LPVOID*)&pDisp));
        if (hr)
            goto UndoCleanup;

        hr = THR(GetDispProp(
                       pDisp,
                       dispid,
                       g_lcidUserDefault,
                       &varOld));
        if (hr)
            goto UndoCleanup;

        hr = THR(CreatePropChangeUndo(dispid, &varOld, NULL));

UndoCleanup:
        ReleaseInterface( pDisp ); 
    }
#endif // NO_EDIT

    DeleteAt(aaIx);

    if ( !fExpando )
    {
        CLock Lock (this); // For OnPropertyChange

        if (!pPropDesc)
        {
            const VTABLEDESC * pVtDesc = _pAA->FindInGINCache(dispid);
            AssertSz(pVtDesc, "And I thought we were guaranteed to get a nonzero VTABLEDESC because of a recent call to GetDisp");
            Assert(pVtDesc->pPropDesc);
            pPropDesc = pVtDesc->pPropDesc;
        }

        // Need to fire a property change event
        if (THR(OnPropertyChange(dispid, pPropDesc->GetdwFlags())))
            return FALSE;
    }

    return TRUE;
}


HRESULT STDMETHODCALLTYPE
CBase::setAttribute(BSTR strPropertyName, VARIANT varPropertyValue, LONG lFlags)
{
    HRESULT hr;
    DISPID dispID;
    DISPPARAMS dp;
    EXCEPINFO   excepinfo;
    UINT uArgError;
    DISPID  dispidPut = DISPID_PROPERTYPUT; // Dispid of prop arg.
    CVariant varNULL(VT_NULL);
    IDispatchEx *pDEX = NULL;
    VARIANT *pVar;

#if TREE_SYNC
    // this is a big HACK, to temporarily get markup-sync working for netdocs.  it
    // will totally change in the future.
    // trident team: do not bother trying to maintain this code, delete if it causes problems
    hr = THR(CMkpSyncLogger::StaticSetAttributeHandler(this,strPropertyName,&varPropertyValue,lFlags));
    if (hr != S_OK)
    {
        goto Cleanup;
    }
#endif // TREE_SYNC

    // Implementation leverages the existing Invoke mechanism

    InitEXCEPINFO(&excepinfo);

    if ( !strPropertyName  )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(PrivateQueryInterface ( IID_IDispatchEx, (void**)&pDEX ));
    if ( hr )
        goto Cleanup;

    hr = THR ( pDEX->GetDispID (strPropertyName, lFlags & GETMEMBER_CASE_SENSITIVE ?
                                    fdexNameCaseSensitive | fdexNameEnsure: fdexNameEnsure, &dispID) );
    if ( hr )
        goto Cleanup;

    pVar = V_ISBYREF(&varPropertyValue) ? V_VARIANTREF(&varPropertyValue) :
                                          &varPropertyValue;

    dp.rgvarg = pVar;
    dp.rgdispidNamedArgs = &dispidPut;
    dp.cArgs = 1;
    dp.cNamedArgs = 1;

    // See if it's accepted as a regular property or expando
    hr = THR ( pDEX->Invoke ( dispID,
                              IID_NULL,
                              g_lcidUserDefault,
                              DISPATCH_PROPERTYPUT,
                              &dp,
                              NULL,
                              &excepinfo,
                              &uArgError ) );

    if ( hr )
    {
        // Failed to parse .. make an unknown
        CVariant varBSTR;
        const PROPERTYDESC *ppropdesc;
        const BASICPROPPARAMS *bpp;

        CLock Lock (this); // For OnPropertyChange

        ppropdesc = FindPropDescForName ( strPropertyName );
        if ( !ppropdesc )
            goto Cleanup;

        // It seems sensible to only allow string values for unknowns so we can always
        // persist them.
        // Try to see if it parses as a valid value
        // Coerce it to a string...
        hr = THR(VariantChangeTypeSpecial ( &varBSTR, pVar, VT_BSTR, NULL, VARIANT_NOVALUEPROP ));
        if ( hr )
            goto Cleanup;

        // Add it as an unknown (invalid) value
        // SetString with fIsUnkown set to TRUE
        hr = CAttrArray::SetString ( &_pAA, ppropdesc,
            (LPTSTR)varBSTR.bstrVal, TRUE );
        if ( hr )
            goto Cleanup;

        bpp = (const BASICPROPPARAMS*)(ppropdesc+1);

        // Need to fire a property change event
        hr = THR(OnPropertyChange(bpp->dispid, bpp->dwFlags));
    }

Cleanup:
    ReleaseInterface ( pDEX );
    FreeEXCEPINFO(&excepinfo);
    RRETURN ( SetErrorInfo ( hr ) );
}


BOOL
CBase::IsExpandoDISPID (DISPID dispid, DISPID *pOLESiteExpando /*= NULL*/)
{
    DISPID  tmpDispID;

    if (!pOLESiteExpando)
        pOLESiteExpando = &tmpDispID;

    if (dispid >= DISPID_EXPANDO_BASE &&
        dispid <= DISPID_EXPANDO_MAX)
    {
         *pOLESiteExpando = dispid;
    }
    else if (dispid >= DISPID_ACTIVEX_EXPANDO_BASE &&
             dispid <= DISPID_ACTIVEX_EXPANDO_MAX)
    {
        *pOLESiteExpando = (dispid - DISPID_ACTIVEX_EXPANDO_BASE) + DISPID_EXPANDO_BASE;
    }
    else
    {
        *pOLESiteExpando = dispid;
        return FALSE;
    }

    return TRUE;
}


HRESULT
CBase::toString(BSTR *bstrString)
{
    HRESULT    hr;
    DISPPARAMS dp = g_Zero.dispparams;
    CVariant   var;

    if ( !bstrString)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *bstrString = NULL;

     hr = THR ( Invoke ( DISPID_VALUE, IID_NULL, g_lcidUserDefault,
            DISPATCH_PROPERTYGET,  &dp, &var, NULL, NULL ) );
    if(hr)
        goto Cleanup;

    hr = var.CoerceVariantArg(VT_BSTR);
    if(hr)
        goto Cleanup;

    *bstrString = V_BSTR(&var);
    // Don't allow deletion of the BSTR
    V_VT(&var) = VT_EMPTY;
     
Cleanup:
    RRETURN ( SetErrorInfo ( hr ) );
}


//+---------------------------------------------------------------
//
//  Member:     FindPropDescFromDispID (private)
//
//  Synopsis:   Find a PROPERTYDESC based on the dispid.
//
//  Arguments:  dispidMember    - PROPERTY/METHOD to find
//              ppDesc          - PROPERTYDESC/METHODDESC found
//              pwEntry         - Byte offset in v-table of virtual function
//
//  Returns:    S_OK            everything is fine
//              E_INVALIDARG    ppDesc is NULL
//              S_FALSE         dispidMember not found in PROPDESC array
//--------------------------------------------------------------------------

#define AUTOMATION_VTBL_ENTRIES   7     // Includes IUnknown + IDispatch functions

HRESULT
CBase::FindPropDescFromDispID(DISPID dispidMember, PROPERTYDESC **ppDesc, WORD *pwEntry, WORD *pwIIDIndex)
{
    HRESULT                     hr = S_OK;
    const VTABLEDESC           *pVTblArray;

    if (!ppDesc)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppDesc = NULL;
    if (pwEntry)
        *pwEntry = 0;
    if (pwIIDIndex)
        *pwIIDIndex = 0;

    // Check cache before we try a linear search?
    pVTblArray = _pAA ? _pAA->FindInGINCache(dispidMember) : NULL;
    if (!pVTblArray)
    {
        // Not in cache, do it the hard way -- linear.
        pVTblArray = GetVTableArray();

        // Look for known property or method in vtable interface array.
        if (pVTblArray && pVTblArray->pPropDesc)
        {
            while (pVTblArray->pPropDesc)
            {
                if (dispidMember == pVTblArray->pPropDesc->GetDispid())
                    goto Found;

                pVTblArray++;
            }

            goto NotFound;
        }
    }

Found:
    if (pVTblArray)
    {
        *ppDesc = const_cast<PROPERTYDESC *>(pVTblArray->pPropDesc);

        if (pwIIDIndex) 
            *pwIIDIndex = pVTblArray->uVTblEntry >> 8;
        if (pwEntry)
            *pwEntry = ((pVTblArray->uVTblEntry & 0xff) + AUTOMATION_VTBL_ENTRIES) * sizeof(void *);  // Adjust for IDispatch methods
    }
    else
    {
        hr = S_FALSE;
        goto Cleanup;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);

NotFound:
    // No match found.
    hr = S_FALSE;
    goto Cleanup;
}

//+---------------------------------------------------------------------------------
//
//  Member:     DefaultMembers
//
//  Synopsis:   sets all class members defined in pdl as members (not attr array)
//              to default value specified in pdl
//
//----------------------------------------------------------------------------------

HRESULT
CBase::DefaultMembers()
{
    HRESULT                         hr = S_OK;
    BASICPROPPARAMS *               pbpp;
    const PROPERTYDESC * const *    ppPropDescs;

    if (GetPropDescArray())
    {
        for (ppPropDescs = GetPropDescArray(); *ppPropDescs; ppPropDescs++)
        {
            pbpp = (BASICPROPPARAMS *)(*ppPropDescs + 1);

            if (!(pbpp->dwPPFlags & PROPPARAM_ATTRARRAY))
            {
                hr = (*ppPropDescs)->Default(this);
                if (hr)
                    goto Cleanup;
            }
        }
    }

Cleanup:
    RRETURN (hr);
}



const ENUMDESC g_enumdescFalseTrue = 
{ 2, 3, {
    { _T("False"),0},
    { _T("True"),-1},
} };


//+---------------------------------------------------------------------------------
//  Member:     GetEnumDescFromDispID - Helper for IPerPropertyBrowsing
//
//  Synopsis:   Returns the enumdesc for given property. It also returns an enumDesk
//                  for booleans
//--------------------------------------------------------------------------------

HRESULT 
CBase::GetEnumDescFromDispID(DISPID dispID, const ENUMDESC **ppEnumDesc)
{
    PROPERTYDESC  * pPropDesc;
    HRESULT         hr;

    // Get the propertydesc by property dispid
    hr = THR(FindPropDescFromDispID(dispID, &pPropDesc, NULL, NULL));
    if(hr)
    {
        if(hr == S_FALSE)
            hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Check if the property is a boolean
    if(pPropDesc->GetBasicPropParams()->wInvFunc == IDX_GS_VARIANTBOOL || 
       pPropDesc->GetBasicPropParams()->wInvFunc == IDX_G_VARIANTBOOL)
    {
        // we have a boolean, return the special enumDesc for Booleans
        *ppEnumDesc = &g_enumdescFalseTrue;
    }
    else
    {    
         // Get the enumDesc
        *ppEnumDesc =  pPropDesc->GetEnumDescriptor();
        if(!(*ppEnumDesc) || !(*ppEnumDesc)->cEnums)
        {
            // No enum description and the property is not boolean, we need to fail
            hr = E_NOTIMPL;
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------------
//  Member:     IPerPropertyBrowsing::GetDisplayString
//
//  Synopsis:   Returns the value of the property as a string
//--------------------------------------------------------------------------------

HRESULT 
CBase::GetDisplayString(DISPID dispID, BSTR *pBstr)
{
    HRESULT          hr;
    const ENUMDESC * pEnumDesc;
    AAINDEX          idx;
    CAttrValue     * pAV = NULL;
    
    if(!pBstr)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *pBstr = NULL;

    idx = FindAAIndex(dispID, CAttrValue::AA_Attribute);
    if(idx == AA_IDX_UNKNOWN)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    if(_pAA)
    {
        pAV = _pAA->FindAt(idx);
    }

    if(!pAV)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    hr = THR(GetEnumDescFromDispID(dispID, &pEnumDesc));
    if(hr)
        goto Cleanup;

    // Return the corresponding enum string
    hr = THR(pEnumDesc->StringFromEnum(pAV->GetLong(), pBstr));
    if(hr)
        goto Cleanup;
  
Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------------
//  Member:     IPerPropertyBrowsing::MapPropertyToPage
//
//  Synopsis:   We do not support this call
//--------------------------------------------------------------------------------

HRESULT 
CBase::MapPropertyToPage(DISPID dispID, CLSID *pClsid)
{
    if(pClsid)
        *pClsid = CLSID_NULL;
    RRETURN(SetErrorInfo(PERPROP_E_NOPAGEAVAILABLE));
}


//+---------------------------------------------------------------------------------
//  Member:     IPerPropertyBrowsing::GetPredefinedStrings
//
//  Synopsis:   Returns a counted array of strings (LPOLESTR pointers) listing the
//                description of the allowable values that the specified property
//                can accept.
//              dispID - the property to use
//              pCaStringsOut - counted array of the possible property values
//              pCaCookiesOut - counted array of cookies that allow to get the
//                 associated string by calling GetPredefinedValue
//--------------------------------------------------------------------------------

HRESULT 
CBase::GetPredefinedStrings(DISPID dispID, CALPOLESTR  *pCaStringsOut, CADWORD *pCaCookiesOut)
{
    HRESULT          hr;
    const ENUMDESC * pEnumDesc;
    BSTR           * pStrings = NULL;
    DWORD          * pCookies = NULL;
    LPOLESTR         szName;
    TCHAR          * tchName;
    int              nEnums = 0;
    int              nIndex = 0;
    
    if(!pCaStringsOut || !pCaCookiesOut)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetEnumDescFromDispID(dispID, &pEnumDesc));
    if(hr)
        goto Cleanup;

    nEnums = pEnumDesc ->cEnums;

    // Allocate the memory for the array of strings    
    pStrings = (BSTR *)CoTaskMemAlloc(sizeof(BSTR) * nEnums);
    if(!pStrings)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Allocate memory for the array of cookies
    pCookies = (DWORD *)CoTaskMemAlloc(sizeof(DWORD) * nEnums);
    if(!pCookies)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Fill the arrays. We use enum indexes as cookies, so it is faster to look
    //  the string up using the cookies later
    for(nIndex = 0; nIndex < nEnums; nIndex++)
    {
        pCookies[nIndex] = nIndex;
        // For enums get the value from the enumdesc
        tchName = pEnumDesc->aenumpairs[nIndex].pszName;
        if(!(*tchName))
            tchName = _T("not set");

        hr = THR(TaskAllocString(tchName, &szName));
        if(hr)
            goto Cleanup;
        pStrings[nIndex] = szName;
    }
    
    // Set the number of values into the return structures
    pCaStringsOut->cElems = pCaCookiesOut->cElems = nEnums;
    // Set the array pointers
    pCaStringsOut->pElems = pStrings;
    pCaCookiesOut->pElems = pCookies;

Cleanup:
    if(hr)
    {   
        for(int i = 0; i < nIndex; i++)
            TaskFreeString(pStrings[i]);
        CoTaskMemFree(pStrings);
        CoTaskMemFree(pCookies);
    }

    RRETURN1(SetErrorInfo(hr), S_FALSE);
}



//+---------------------------------------------------------------------------------
//  Member:     IPerPropertyBrowsing::GetPredefinedValue
//
//  Synopsis:   Returns a Variant containing the value of a property identified with 
//                dispID that is associated with a predefined string (enum) name
//                as returned from GetPredefinedStrings
//              dispID - the property to use
//              dwCookie - the value from GetPredefinedStrings
//              pVarOut - the return value
//--------------------------------------------------------------------------------

HRESULT 
CBase::GetPredefinedValue(DISPID dispID, DWORD dwCookie, VARIANT *pVarOut)
{
    HRESULT          hr;
    const ENUMDESC * pEnumDesc;
    int              nEnums;
    
    if(!pVarOut)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetEnumDescFromDispID(dispID, &pEnumDesc));
    if(hr)
        goto Cleanup;

    nEnums = pEnumDesc ->cEnums;

    if(dwCookie >= (DWORD)nEnums)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if(&g_enumdescFalseTrue != pEnumDesc)
    {
        // Return string values for enums
        hr = THR(FormsAllocString(pEnumDesc->aenumpairs[dwCookie].pszName, &V_BSTR(pVarOut)));
        if(hr)
            goto Cleanup;
        V_VT(pVarOut) = VT_BSTR;
    }
    else
    {
        // For booleans return a VARIANT_BOOL
        Assert(dwCookie == 0 || dwCookie == 1);
        V_BOOL(pVarOut) = pEnumDesc->aenumpairs[dwCookie].iVal;
        V_VT(pVarOut) = VT_BOOL;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}



//+-------------------------------------------------------------------------
//
//  Method:     MatchExactGetIDsOfNames, (exported helper used by shdocvw)
//
//  Synopsis:   Loop through all property entries in ITypeInfo does a case
//              sensitive match (GetIDsofNames is case insensitive).
//
//  Results:    S_OK                - return dispid of found name
//              DISP_E_UNKNOWNNAME  - name not found
//
//--------------------------------------------------------------------------
STDAPI
MatchExactGetIDsOfNames(ITypeInfo *pTI,
                        REFIID riid,
                        LPOLESTR *rgszNames,
                        UINT cNames,
                        LCID lcid,
                        DISPID *rgdispid,
                        BOOL fCaseSensitive)
{
    HRESULT         hr = S_OK;
    CTypeInfoNav    tin;
    STRINGCOMPAREFN pfnCompareString;

    if (cNames == 0)
        goto Cleanup;

    if (!IsEqualIID(riid, IID_NULL) || !pTI || !rgszNames || !rgdispid)
        RRETURN(E_INVALIDARG);

    pfnCompareString = fCaseSensitive ? StrCmpC : StrCmpI;

    rgdispid[0] = DISPID_UNKNOWN;

    // Loop thru properties.
    hr = tin.InitITypeInfo(pTI);
    while ((hr = tin.Next()) == S_OK)
    {
        VARDESC        *pVar;
        FUNCDESC       *pFunc;
        DISPID          memid = DISPID_UNKNOWN;

        if ((pVar = tin.getVarD()) != NULL)
        {
            memid = pVar->memid;
        }
        else if ((pFunc = tin.getFuncD()) != NULL)
        {
            memid = pFunc->memid;
        }

        // Got a property?
        if (memid != DISPID_UNKNOWN)
        {
            BSTR            bstrName;
            unsigned int    cNameRet;

            // Get the name.
            hr = THR(pTI->GetNames(memid, &bstrName, 1, &cNameRet));
            if (hr)
                break;

            if (cNameRet == 1)
            {
                // Does it match?
                if (pfnCompareString(rgszNames[0], bstrName) == 0)
                {
                    rgdispid[0] = memid;
                    SysFreeString(bstrName);
                    break;
                }

                SysFreeString(bstrName);
           }
        }
    }

Cleanup:
    if (hr || rgdispid[0] == DISPID_UNKNOWN)
        hr = DISP_E_UNKNOWNNAME;

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     attachEvent
//
//  Synopsis:   Adds an AA_AttachEvent entry to attrarray to support multi-
//              casting of onNNNNN events.
//
//--------------------------------------------------------------------------

HRESULT
CBase::attachEvent(BSTR bstrEvent, IDispatch* pDisp, VARIANT_BOOL *pResult)
{
    HRESULT         hr = S_OK;
    DISPID          dispid;
    IDispatchEx *   pDEXMe = NULL;
    
    if (!pDisp || !bstrEvent)
        goto Cleanup;

    hr = THR_NOTRACE(PrivateQueryInterface(IID_IDispatchEx, (void **)&pDEXMe));
    if (hr)
        goto Cleanup;
        
    //
    // BUGBUG: (anandra) Always being case sensitive here.  Need to
    // check for VBS and be insensitive then.
    //
    
    hr = THR_NOTRACE(pDEXMe->GetDispID(bstrEvent, fdexNameCaseSensitive | fdexNameEnsure, &dispid));
    if (hr)
        goto Cleanup;

    hr = THR(AddDispatchObjectMultiple(dispid,
                                       pDisp,
                                       CAttrValue::AA_AttachEvent));

    // let opp know that an event was attached.
    IGNORE_HR(OnPropertyChange(DISPID_EVPROP_ONATTACHEVENT, 0));

Cleanup:
    if (pResult)
    {
        *pResult = hr ? VARIANT_FALSE : VARIANT_TRUE;
    }
    ReleaseInterface(pDEXMe);
    RRETURN(SetErrorInfo(hr));
}


//+-------------------------------------------------------------------------
//
//  Method:     detachEvent
//
//  Synopsis:   Loops through all AA_AttachEvent entries in the attrarray
//              and removes first entry who's COM identity is the same as
//              the pDisp passed in.
//
//--------------------------------------------------------------------------

HRESULT
CBase::detachEvent(BSTR bstrEvent, IDispatch *pDisp)
{
    AAINDEX         aaidx = AA_IDX_UNKNOWN;
    DISPID          dispid;
    IDispatch *     pThisDisp = NULL;
    HRESULT         hr = S_OK;
    IDispatchEx *   pDEXMe = NULL;
    
    if (!pDisp || !bstrEvent)
        goto Cleanup;

    hr = THR_NOTRACE(PrivateQueryInterface(IID_IDispatchEx, (void **)&pDEXMe));
    if (hr)
        goto Cleanup;
        
    //
    // BUGBUG: (anandra) Always being case sensitive here.  Need to
    // check for VBS and be insensitive then.
    //
    
    hr = THR_NOTRACE(pDEXMe->GetDispID(bstrEvent, fdexNameCaseSensitive, &dispid));
    if (hr)
        goto Cleanup;

    // Find event that has this function pointer.
    for (;;)
    {
        aaidx = FindNextAAIndex(dispid, CAttrValue::AA_AttachEvent, aaidx);
        if (aaidx == AA_IDX_UNKNOWN)
            break;

        ClearInterface(&pThisDisp);
        if (GetDispatchObjectAt(aaidx, &pThisDisp))
            continue;

        if (IsSameObject(pDisp, pThisDisp))
            break;
    };

    // Found item to delete?
    if (aaidx != AA_IDX_UNKNOWN)
    {
        DeleteAt(aaidx);
    }

Cleanup:
    ReleaseInterface(pThisDisp);
    ReleaseInterface(pDEXMe);
    RRETURN(SetErrorInfo(S_OK));
}
