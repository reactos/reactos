//+----------------------------------------------------------------------------
//
// File:        qdocglue.CXX
//
// Contents:    Implementation of CQDocGlue and related classes
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
// @doc INTERNAL
//-----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X__LINE_H_
#define X__LINE_H_
#include "_line.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_QUILGLUE_HXX_
#define X_QUILGLUE_HXX_
#include "quilglue.hxx"
#endif

#ifndef X_IID_QUILLSITE_H_
#define X_IID_QUILLSITE_H_
#include "iid_quillsite.h"
#endif

#ifndef X_IID_TREESYNC_H_
#define X_IID_TREESYNC_H_
#include "iid_treesync.h"
#endif

#ifndef X_QDOCGLUE_HXX_
#define X_QDOCGLUE_HXX_
#include "qdocglue.hxx"
#endif

MtDefine(CQDocGlue, CDoc, "CQDocGlue")
MtDefine(CQIdleTask, Quill, "CQIdleTask")

//+---------------------------------------------------------------------------
//
//  Method:     CQDocGlue::CQDocGlue
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------
CQDocGlue::CQDocGlue(IUnknown *pUnkOuter)
{
    Assert(pUnkOuter);
    _pUnkOuter = pUnkOuter ? pUnkOuter : PunkInner();
    _pDoc = (CDoc *)(IHTMLDocument *)pUnkOuter;
    // HRESULT hr = pUnkOuter->QueryInterface(CLSID_CDoc,(void**)&_pDoc); // review(tomlaw) didn't work, CLSID_CDoc not defined!!!
    //Assert(hr == S_OK);
}

//+---------------------------------------------------------------------------
//
//  Method:     CQDocGlue::~CQDocGlue
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------
CQDocGlue::~CQDocGlue()
{
    Assert(!m_pITextLayoutGroup);
    Assert(!m_pIdleTask);
}

//+---------------------------------------------------------------------------
//
//  Method:     CQDocGlue::Passivate
//
//  Synopsis:   shutdown
//
//----------------------------------------------------------------------------
void
CQDocGlue::Passivate()
{
    if (m_pIdleTask)
    {
        m_pIdleTask->Release();
        m_pIdleTask = NULL;
    }

    ClearInterface(&m_pITextLayoutGroup);
}

//+------------------------------------------------------------------------
//
//  Member:     CQDocGlue::InitTextLayoutGroup
//
//  Synopsis:   Create external layout engine if necessary.
//
//-------------------------------------------------------------------------
HRESULT
CQDocGlue::InitTextLayoutGroup()
{
    HRESULT hr;

    if (m_pITextLayoutGroup)
        return S_OK;

   // load the Quill DLL, and fail if it's not found
    hr = InitQuillLayout();
    if (!TLS(_pQLM))
        return hr;

    hr = TLS(_pQLM)->CreateTextLayoutGroup(&m_pITextLayoutGroup);

    if (hr)
        goto LExit;

    m_pIdleTask = new CQIdleTask(m_pITextLayoutGroup);
    if (!m_pIdleTask)
    {
        ClearInterface(&m_pITextLayoutGroup);
        hr = E_OUTOFMEMORY;
        goto LExit;
    }

    hr = S_OK;

LExit:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CQDocGlue::Init
//
//  Synopsis:   Check registry settings, create external layout engine.
//
//-------------------------------------------------------------------------
HRESULT
CQDocGlue::Init()
{
    HRESULT hr = S_OK;

    // TEMPORARY (sidda): use Quill based on registry settings
    HKEY hkey = NULL;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"software\\debug\\quill",
                                      0, KEY_READ, &hkey))
    {
        long value = 0;
        DWORD type, size;
        size = sizeof(long);

        if (ERROR_SUCCESS == RegQueryValueEx(hkey, L"Layout", 0, &type,
                                             (LPBYTE) &value, &size))
        {
            if (value)
                m_dwOptions |= LAYOUTOPTIONS_EXTERNAL;
            else
                m_dwOptions &= ~LAYOUTOPTIONS_EXTERNAL;
        }

        RegCloseKey(hkey);
    }

    if (FExternalLayout())
        hr = InitTextLayoutGroup();

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CQDocGlue::PrivateQueryInterfaceNonIUnknown
//
//  Synopsis:   QI. However, because of the *weird* way in which CDoc
//              aggregates this object, do NOT succeed on a QI for IUnknown!!
//
//-------------------------------------------------------------------------
HRESULT
CQDocGlue::PrivateQueryInterfaceNonIUnknown(REFIID riid, LPVOID * ppv)
{
    HRESULT hr = S_OK;

    *ppv = NULL;

    if(riid == IID_ITextLayoutAccess)
    {
        *ppv = (ITextLayoutAccess *)this;
    }
#if TREE_SYNC
    else if (riid == IID_ITreeSyncServices)
    {
        *ppv = (ITreeSyncServices *)this;
    }
    else if (riid == IID_ITreeSyncLogSource)
    {
        *ppv = (ITreeSyncLogSource *)this;
    }
#endif // TREE_SYNC

    if(*ppv == NULL)
    {
        hr = E_NOINTERFACE;
    }
    else
    {
        ((LPUNKNOWN)* ppv)->AddRef();
    }
    RRETURN(hr);
}

BOOL
CQDocGlue::FExternalLayout()
{
    return (m_dwOptions & LAYOUTOPTIONS_EXTERNAL ? TRUE : FALSE);
};

// @doc QTAPI

//+------------------------------------------------------------------------
//
//  ITextLayoutAccess implementation.
//
//-------------------------------------------------------------------------

/*----------------------------------------------------------------------------
@interface ITextLayoutAccess |

    Exposes the external layout engine provided by Quill.

@meth    HRESULT | GetTextLayoutGroup | Retrieve a pointer to the layout group
                    associated with this document.
@meth    HRESULT | GetLayoutOptions | Retrieve the layout options.
@meth    HRESULT | SetLayoutOptions | Set the layout options.

@comm    This interface is supported by the MSHTML document object. It
        exposes access to an externally-supplied HTML layout engine such
        as QuillSite.

@supby    <o CDoc>
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutAccess | GetTextLayoutGroup |

    Retrieve a pointer to the layout group associated with this document.

@rvalue    S_OK | Success.
@rvalue    E_INVALIDARG | <p ppgrp> is NULL.

@xref    <i ITextLayoutGroup>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQDocGlue::GetTextLayoutGroup(
    ITextLayoutGroup **ppgrp)    // @parm Pointer to the layout group associated
                                // with this document is returned in *<p ppgrp>.
{
    if (!ppgrp)
        return E_INVALIDARG;

    *ppgrp = m_pITextLayoutGroup;
    if (*ppgrp)
        (*ppgrp)->AddRef();

    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutAccess | GetLayoutOptions |

    Retrieve the layout options.

@rvalue    S_OK | Success.
@rvalue    E_INVALIDARG | <p pdwOptions> is NULL.

@xref    <om .SetLayoutOptions>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQDocGlue::GetLayoutOptions(
    DWORD *pdwOptions)    // @parm Options returned in *<p pdwOptions>. See
                        // <om .SetLayoutOptions> for a description.
{
    if (!pdwOptions)
        return E_INVALIDARG;

    *pdwOptions = m_dwOptions;
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutAccess | SetLayoutOptions |

    Set the layout options.

@rvalue    S_OK | Success.
@rvalue E_FAIL | Cannot load external layout engine.

@comm    <p dwOptions> can consist of one or more of the following flags:

@flag    LAYOUTOPTIONS_EXTERNAL | Use the external layout engine.

@comm    By default, Trident's internal layout engine is used.

@xref    <om .GetLayoutOptions>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQDocGlue::SetLayoutOptions(
    DWORD dwOptions)    // @parm Options to be set. See below for a description.
{
    if (dwOptions & LAYOUTOPTIONS_EXTERNAL)
    {
        // make sure we can access the external layout engine
        if (InitTextLayoutGroup())
            return E_FAIL;
    }

    m_dwOptions = dwOptions;
    return S_OK;
}

////////////////////////////////////////
// CQIdleTask implementation

CQIdleTask::CQIdleTask(ITextLayoutGroup* pITextLayoutGroup)
{
    Assert(pITextLayoutGroup);
    m_pITextLayoutGroup = pITextLayoutGroup;
    m_pITextLayoutGroup->AddRef();
    SetInterval(250);
}

/*virtual*/void CQIdleTask::OnRun (DWORD dwTimeOut)
{
    if (m_pITextLayoutGroup)
        m_pITextLayoutGroup->Idle(dwTimeOut);
}

/*virtual*/void CQIdleTask::OnTerminate() 
{ 
    if (m_pITextLayoutGroup) 
    {
        m_pITextLayoutGroup->Release();
        m_pITextLayoutGroup = NULL; 
    }
}

