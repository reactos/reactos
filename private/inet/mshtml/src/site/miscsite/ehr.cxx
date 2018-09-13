//+---------------------------------------------------------------------
//
//   File:      eli.cxx
//
//  Contents:   HR element class
//
//  Classes:    CHRElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_EHR_HXX_
#define X_EHR_HXX_
#include "ehr.hxx"
#endif

#ifndef X_HRLYT_HXX_
#define X_HRLYT_HXX_
#include "hrlyt.hxx"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#define _cxx_
#include "hr.hdl"

MtDefine(CHRElement, Elements, "CHRElement")



const CElement::CLASSDESC CHRElement::s_classdesc =
{
    {
        &CLSID_HTMLHRElement,                // _pclsid
        0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                      // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                              // _pcpi
        ELEMENTDESC_NEVERSCROLL     |
        ELEMENTDESC_CARETINS_DL,             // _dwFlags
        &IID_IHTMLHRElement,                 // _piidDispinterface
        &s_apHdlDescs,                       // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLHRElement,          //_apfnTearOff
    NULL,                                    // _pAccelsDesign
    NULL                                     // _pAccelsRun
};

IMPLEMENT_LAYOUT_FNS(CHRElement, CHRLayout)

HRESULT
CHRElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_HR));
    Assert(ppElement);

    *ppElement = new CHRElement(pDoc);

    RRETURN ( (*ppElement) ? S_OK : E_OUTOFMEMORY );
}

//+------------------------------------------------------------------------
//
//  Member:     CHRElement::Save
//
//  Synopsis:   Save the element to the stream
//
//-------------------------------------------------------------------------

HRESULT
CHRElement::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    DWORD   dwOldFlags;
    HRESULT hr = S_OK;

    dwOldFlags = pStreamWrBuff->ClearFlags(WBF_ENTITYREF);

    // Supress CRLF at start when saving (bug 66743) (jbeda)
    if (! pStreamWrBuff->TestFlag(WBF_NO_PRETTY_CRLF))
    {
        hr = pStreamWrBuff->NewLine();
        if (hr)
            goto Cleanup;
    }

    if(!fEnd)
    {
        if(pStreamWrBuff->TestFlag(WBF_SAVE_PLAINTEXT))
        {
            if (pStreamWrBuff->TestFlag(WBF_FORMATTED_PLAINTEXT))
            {
                hr = pStreamWrBuff->WriteRule();
                if(hr)
                    goto Cleanup;
            }
        }
        else
        {
            // do not write end tags for P, etc
            hr = WriteTag(pStreamWrBuff, fEnd);
            if(hr)
                goto Cleanup;
        }
    }

Cleanup:
    pStreamWrBuff->RestoreFlags(dwOldFlags);
    RRETURN(hr);
}

//
// BUGBUG marka - HandleMessage for HR has been removed. We used to bubble the message to it's
// parent in edit mode. We don't think we need this anymore ( or can't see why ).
//


HRESULT
CHRElement::ApplyDefaultFormat ( CFormatInfo * pCFI )
{
    HRESULT hr;

    // Override the inherited text color
    if (pCFI->_pcf->_ccvTextColor.IsDefined())
    {
        pCFI->PrepareCharFormat();
        pCFI->_cf()._ccvTextColor.Undefine();
        pCFI->UnprepareForDebug();
    }

    if (pCFI->_bBlockAlign == htmlBlockAlignNotSet)
    {
        pCFI->_bBlockAlign     = htmlBlockAlignCenter;
        pCFI->_bCtrlBlockAlign = htmlBlockAlignCenter;
    }

    hr = THR(super::ApplyDefaultFormat ( pCFI ));

    // Default to percent width.
    if (!pCFI->_pff->_fWidthPercent && pCFI->_pff->_cuvWidth.IsNullOrEnum())
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fWidthPercent = TRUE;
        pCFI->UnprepareForDebug();
    }

    RRETURN(hr);
}



//+-------------------------------------------------------------------------
//
//  Method:     CHRElement::QueryStatus
//
//  Synopsis:   Called to discover if a given command is supported
//              and if it is, what's its state.  (disabled, up or down)
//
//--------------------------------------------------------------------------

HRESULT
CHRElement::QueryStatus(GUID * pguidCmdGroup,
                        ULONG cCmds,
                        MSOCMD rgCmds[],
                        MSOCMDTEXT * pcmdtext)
{
    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));
    Assert(cCmds == 1);

    MSOCMD   * pCmd = & rgCmds[0];
    ULONG      cmdID;
    HRESULT    hr;

    Assert(!pCmd->cmdf);

    cmdID = CBase::IDMFromCmdID( pguidCmdGroup, pCmd->cmdID );
    switch (cmdID)
    {
    case IDM_FORECOLOR:
        pCmd->cmdf = IsEditable(TRUE) ?
                (MSOCMDSTATE_UP) : (MSOCMDSTATE_DISABLED);
        hr = S_OK;
        break;

    default:
        hr = THR_NOTRACE(super::QueryStatus(pguidCmdGroup,
                                            1,
                                            pCmd,
                                            pcmdtext));
        break;
    }

    RRETURN_NOTRACE( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CHRElement::Exec
//
//  Synopsis:   execute the commands from QueryStatus
//
//--------------------------------------------------------------------------

HRESULT
CHRElement::Exec(GUID * pguidCmdGroup,
                 DWORD nCmdID,
                 DWORD nCmdexecopt,
                 VARIANTARG * pvarargIn,
                 VARIANTARG * pvarargOut)
{
    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));

    int      idm = CBase::IDMFromCmdID(pguidCmdGroup, nCmdID);
    HRESULT  hr  = MSOCMDERR_E_NOTSUPPORTED;

    switch (idm)
    {
    case IDM_FORECOLOR:
        if (pvarargOut)
        {
            // its a property get
            hr = THR(get_PropertyHelper(pvarargOut, (PROPERTYDESC *)&s_propdescCHRElementcolor));
            if (hr)
                goto Cleanup;

            if (VT_BSTR == V_VT(pvarargOut))
            {
                // we need to convert to I4 to return consistent with what
                // comes in from a set
                CColorValue cvColor;

                hr = THR(cvColor.FromString(V_BSTR(pvarargOut)));
                if (hr)
                    goto Cleanup;

                VariantClear(pvarargOut);
                V_I4(pvarargOut) = cvColor.GetRawValue();
                V_VT(pvarargOut) = VT_I4;
            }
        }
        else if (!pvarargIn)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        else
        {
#ifndef NO_EDIT
            CParentUndoUnit *pCPUU = OpenParentUnit(this, IDS_UNDOPROPCHANGE);
#endif // NO_EDIT

            // property set. we need to flip the colors, for OLE
            // compatability
            CColorValue cvValue;
            CVariant varColor;

            hr = THR(varColor.CoerceVariantArg(pvarargIn, VT_I4));
            if (hr)
                goto Cleanup;

            cvValue.SetFromRGB(V_I4(&varColor));

            V_I4(pvarargIn) = (DWORD)cvValue.GetRawValue();
            V_VT(pvarargIn) = VT_I4;

            //its a property put
            hr = THR(put_VariantHelper(*pvarargIn, (PROPERTYDESC *)&s_propdescCHRElementcolor));

#ifndef NO_EDIT
            CloseParentUnit(pCPUU, hr);
#endif // NO_EDIT
        }
        break;
    }

    if (hr == MSOCMDERR_E_NOTSUPPORTED)
    {
        hr = super::Exec(
                pguidCmdGroup,
                nCmdID,
                nCmdexecopt,
                pvarargIn,
                pvarargOut);
    }

Cleanup:
    RRETURN_NOTRACE(hr);
}


