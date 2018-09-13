//+---------------------------------------------------------------------
//
//  File:       generic.cxx
//
//  Contents:   Extensible tags classes
//
//  Classes:    CGenericElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_GENERIC_HXX_
#define X_GENERIC_HXX_
#include "generic.hxx"
#endif

#ifndef X_XMLNS_HXX_
#define X_XMLNS_HXX_
#include "xmlns.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx" // for CStreamWriteBuf
#endif

#ifndef X_DMEMBMGR_HXX_
#define X_DMEMBMGR_HXX_
#include "dmembmgr.hxx"       // for CDataMemberMgr
#endif

#ifndef X_DBTASK_HXX_
#define X_DBTASK_HXX_
#include "dbtask.hxx"       // for CDataBindTask
#endif

#define _cxx_
#include "generic.hdl"

MtDefine(CGenericElement, Elements, "CGenericElement")

///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

const CElement::CLASSDESC CGenericElement::s_classdesc =
{
    {
        &CLSID_HTMLGenericElement,          // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_XTAG,                   // _dwFlags
        &IID_IHTMLGenericElement,           // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnIHTMLGenericElement,      //_apfnTearOff

    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

///////////////////////////////////////////////////////////////////////////
//
// CGenericElement methods
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Method:     CGenericElement::CreateElement
//
//-------------------------------------------------------------------------

HRESULT CGenericElement::CreateElement(
    CHtmTag *  pht,
    CDoc *      pDoc,
    CElement ** ppElement)
{
    Assert(ppElement);

    *ppElement = new CGenericElement(pht, pDoc);

    return *ppElement ? S_OK : E_OUTOFMEMORY;
}

//+------------------------------------------------------------------------
//
//  Method:     CGenericElement constructor
//
//-------------------------------------------------------------------------

CGenericElement::CGenericElement (CHtmTag * pht, CDoc * pDoc)
  : CElement(pht->GetTag(), pDoc)
{
#ifndef VSTUDIO7

    LPTSTR  pchColon;
    LPTSTR  pchStart;

    Assert(IsGenericTag(pht->GetTag()));
    Assert(pht->GetPch());

    if (pht->GetPch())
    {
        pchColon = StrChr(pht->GetPch(), _T(':'));
        if (pchColon)
        {
            pchStart = pht->GetPch();

            IGNORE_HR(_cstrNamespace.Set(pchStart, PTR_DIFF(pchColon, pchStart)));
            IGNORE_HR(_cstrTagName.Set(pchColon + 1));
        }
        else
        {
            IGNORE_HR(_cstrTagName.Set(pht->GetPch()));
        }
    }
#endif //VSTUDIO7
}

//+------------------------------------------------------------------------
//
//  Method:     CGenericElement::Init2
//
//-------------------------------------------------------------------------

HRESULT
CGenericElement::Init2(CInit2Context * pContext)
{
    HRESULT     hr;
    LPTSTR      pchNamespace;

#ifdef VSTUDIO7
    if (pContext && pContext->_pht)
    {
        hr = THR(SetTagNameAndScope(pContext->_pht));
        if (hr)
            goto Cleanup;
    }
#endif //VSTUDIO7

    hr = THR(super::Init2(pContext));
    if (hr)
        goto Cleanup;

    if (pContext)
    {
        pchNamespace = (LPTSTR) Namespace();

        if (pchNamespace && pContext->_pTargetMarkup)
        {
            CXmlNamespaceTable *    pNamespaceTable = pContext->_pTargetMarkup->GetXmlNamespaceTable();
            LONG                    urnAtom;

            if (pNamespaceTable) // (we might not have pNamespaceTable if the namespace if "PUBLIC:")
            {
                hr = THR(pNamespaceTable->GetUrnAtom(pchNamespace, &urnAtom));
                if (hr)
                    goto Cleanup;

                if (-1 != urnAtom)
                {
                    hr = THR(PutUrnAtom(urnAtom));
                }
            }
        }
    }

Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Method:     CGenericElement::Notify
//
//-------------------------------------------------------------------------

void
CGenericElement::Notify(CNotification *pnf)
{
    Assert(pnf);

    super::Notify(pnf);

    switch (pnf->Type())
    {
    case NTYPE_ELEMENT_ENTERTREE:
        // the <XML> tag can act as a data source for databinding.  Whenever such
        // a tag is added to the document, we should tell the databinding task
        // to try again.  Something might work now that didn't before.
        if (0 == FormsStringCmp(TagName(), _T("xml")))
        {
            Doc()->GetDataBindTask()->SetWaiting();
        }
        break;

    default:
        break;
    }
}

//+------------------------------------------------------------------------
//
//  Method:     CGenericElement::Save
//
//-------------------------------------------------------------------------

HRESULT
CGenericElement::Save (CStreamWriteBuff * pStreamWriteBuff, BOOL fEnd)
{
    HRESULT     hr;
    DWORD       dwOldFlags;

    if (ETAG_GENERIC_LITERAL != Tag())
    {
        hr = THR(super::Save(pStreamWriteBuff, fEnd));
        if (hr)
            goto Cleanup;
    }
    else // if (ETAG_GENERIC_LITERAL == Tag())
    {
        Assert (ETAG_GENERIC_LITERAL == Tag());

        dwOldFlags = pStreamWriteBuff->ClearFlags(WBF_ENTITYREF);
        pStreamWriteBuff->SetFlags(WBF_SAVE_VERBATIM | WBF_NO_WRAP);
        pStreamWriteBuff->BeginPre();

        hr = THR(super::Save(pStreamWriteBuff, fEnd));
        if (hr)
            goto Cleanup;

        if (!fEnd &&
            !pStreamWriteBuff->TestFlag(WBF_SAVE_PLAINTEXT) &&
            ETAG_GENERIC_LITERAL == Tag())
        {
            hr = THR(pStreamWriteBuff->Write(_cstrContents));
            if (hr)
                goto Cleanup;
        }

        pStreamWriteBuff->EndPre();
        pStreamWriteBuff->RestoreFlags(dwOldFlags);
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericElement::namedRecordset
//
//  Synopsis:   returns an ADO Recordset for the named data member.  Tunnels
//              into the hierarchy using the path, if given.
//
//  Arguments:  bstrDataMember  name of data member (NULL for default)
//              pvarHierarchy   BSTR path through hierarchy (optional)
//              pRecordSet      where to return the recordset.
//
//
//----------------------------------------------------------------------------

HRESULT
CGenericElement::namedRecordset(BSTR bstrDatamember,
                               VARIANT *pvarHierarchy,
                               IDispatch **ppRecordSet)
{
    HRESULT hr;
    CDataMemberMgr *pdmm;

#ifndef NO_DATABINDING
    EnsureDataMemberManager();
    pdmm = GetDataMemberManager();
    if (pdmm)
    {
        hr = pdmm->namedRecordset(bstrDatamember, pvarHierarchy, ppRecordSet);
        if (hr == S_FALSE)
            hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }
    
#else
    *pRecordSet = NULL;
    hr = S_OK;
#endif NO_DATABINDING

    RRETURN (SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericElement::getRecordSet
//
//  Synopsis:   returns an ADO Recordset pointer if this site is a data
//              source control
//
//  Arguments:  IDispatch **    pointer to a pointer to a record set.
//
//
//----------------------------------------------------------------------------

HRESULT
CGenericElement::get_recordset(IDispatch **ppRecordSet)
{
    return namedRecordset(NULL, NULL, ppRecordSet);
}

