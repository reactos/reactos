//+---------------------------------------------------------------------
//
//   File:      inputtxt.cxx
//
//  Contents:   InputTxt element class, etc..
//
//  Classes:    CInput, etc..
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_CGUID_H_
#define X_CGUID_H_
#include <cguid.h>
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_POSTDATA_HXX_
#define X_POSTDATA_HXX_
#include "postdata.hxx"
#endif

#ifndef X_IMGHLPER_HXX_
#define X_IMGHLPER_HXX_
#include "imghlper.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_INPUTBTN_HXX_
#define X_INPUTBTN_HXX_
#include "inputbtn.hxx"
#endif

#ifndef X_IMGHLPER_HXX_
#define X_IMGHLPER_HXX_
#include "imghlper.hxx"
#endif

#ifndef X_INPUTLYT_HXX_
#define X_INPUTLYT_HXX_
#include "inputlyt.hxx"
#endif

#ifndef X_CKBOXLYT_HXX_
#define X_CKBOXLYT_HXX_
#include "ckboxlyt.hxx"
#endif

#ifndef X_IMGLYT_HXX_
#define X_IMGLYT_HXX_
#include "imglyt.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include <treepos.hxx>
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_TXTSLAVE_HXX_
#define X_TXTSLAVE_HXX_
#include "txtslave.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

#if defined(_M_ALPHA)
#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif


#define _cxx_
#include "inputtxt.hdl"

DeclareTag(tagAllowMorphing, "CInput", "Allow input one time morphing");

MtDefine(CInput, Elements, "CInput")
MtDefine(CInputSetValueHelperReal, Locals, "CInput::SetValueHelperReal (temp)")
MtDefine(CRadioGroupAry, Elements, "CRadioGroupAry")
MtDefine(CRadioGroupAry_pv, CRadioGroupAry, "CRadioGroupAry::_pv")

HRESULT GetCallerCommandTarget (CBase *pBase, IServiceProvider *pSP, BOOL fFirstScriptSite, IOleCommandTarget **ppCommandTarget);

//
// This is a generic creation function for our input controls
// parser.cpp should point to this function for ETAG_INPUT
//

HRESULT
CreateInputElement(CHtmTag *pht, CDoc *pDoc, CElement **ppElement)
{
    HRESULT         hr = S_OK;
    long            iType;
    TCHAR *         pchType;

    Assert(pht->Is(ETAG_INPUT));

    // Fetch type from the attributes and map to enum. Default to input text.

    if (!pht->ValFromName(_T("type"), &pchType) ||
            FAILED(s_enumdeschtmlInput.EnumFromString(pchType, &iType, FALSE)))
    {
        iType = htmlInputText;
    }

    // Map input type enum to create function.

    switch (iType)
    {
        default:
            // Convert bogus Input types to default. These enum values exist only
            // because TEXTAREA, HTMLAREA and SELECT support the type attribute
            // and use these values to specify the type. These values have no
            // meaning when used with an INPUT tag and are mapped to the default
            // value of htmlInputText.
            iType= htmlInputText;
            // fall through
        case htmlInputReset:
        case htmlInputSubmit:
        case htmlInputButton:
        case htmlInputHidden:
        case htmlInputPassword:
        case htmlInputText:
        case htmlInputFile:
        case htmlInputImage:
        case htmlInputRadio:
        case htmlInputCheckbox:
            hr = THR(CInput::CreateElement(pht, pDoc, ppElement, (htmlInput)iType));
            DYNCAST(CInput, *ppElement)->_fScriptCreated = pht->IsScriptCreated();
            break;
    }

    RRETURN(hr);
}


CElement::ACCELS CInput::s_AccelsInputTxtDesign = CElement::ACCELS (NULL, IDR_ACCELS_INPUTTXT_DESIGN);
CElement::ACCELS CInput::s_AccelsInputTxtRun    = CElement::ACCELS (NULL, IDR_ACCELS_INPUTTXT_RUN);

static const TCHAR s_achUtf8[] = TEXT("utf-8");

// Input Class Decriptor
CElement::CLASSDESC CInput::s_classdescHidden =
{
    {
        &CLSID_HTMLInputElement,    // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_TEXTSITE |          // _dwFlags
        ELEMENTDESC_DONTINHERITSTYLE |
        ELEMENTDESC_SHOWTWS |
        ELEMENTDESC_VPADDING |
        ELEMENTDESC_NOTIFYENDPARSE |
        ELEMENTDESC_NOANCESTORCLICK |
        ELEMENTDESC_HASDEFDESCENT,
        &IID_IHTMLInputElement,   // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLInputElement, // _pfnTearOff
    &s_AccelsInputTxtDesign,            // _pAccelsDesign
    &s_AccelsInputTxtRun                // _pAccelsRun
};

CElement::CLASSDESC CInput::s_classdescPassword =
{
    {
        &CLSID_HTMLInputElement,    // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_TEXTSITE |          // _dwFlags
        ELEMENTDESC_DONTINHERITSTYLE |
        ELEMENTDESC_SHOWTWS |
        ELEMENTDESC_VPADDING |
        ELEMENTDESC_CANSCROLL |
        ELEMENTDESC_NOTIFYENDPARSE |
        ELEMENTDESC_NOANCESTORCLICK |
        ELEMENTDESC_HASDEFDESCENT,
        &IID_IHTMLInputElement,     // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLInputElement, // _pfnTearOff
    &s_AccelsInputTxtDesign,            // _pAccelsDesign
    &s_AccelsInputTxtRun                // _pAccelsRun
};

CElement::CLASSDESC CInput::s_classdescText =
{
    {
        &CLSID_HTMLInputElement,    // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_TEXTSITE |          // _dwFlags
        ELEMENTDESC_DONTINHERITSTYLE |
        ELEMENTDESC_SHOWTWS |
        ELEMENTDESC_VPADDING |
        ELEMENTDESC_CANSCROLL |
        ELEMENTDESC_NOTIFYENDPARSE |
        ELEMENTDESC_NOANCESTORCLICK |
        ELEMENTDESC_HASDEFDESCENT,
        &IID_IHTMLInputElement,     // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLInputElement, // _pfnTearOff
    &s_AccelsInputTxtDesign,            // _pAccelsDesign
    &s_AccelsInputTxtRun                // _pAccelsRun
};

CElement::CLASSDESC CInput::s_classdescBtn =
{
    {
        &CLSID_HTMLInputElement,      // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_TEXTSITE |
        ELEMENTDESC_DONTINHERITSTYLE |
        ELEMENTDESC_SHOWTWS |
        ELEMENTDESC_NOTIFYENDPARSE |
        ELEMENTDESC_NOANCESTORCLICK|
        ELEMENTDESC_HASDEFDESCENT,
        &IID_IHTMLInputElement,       // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLInputElement,  // _pfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

// Submit Class Decriptor

CElement::CLASSDESC CInput::s_classdescSubmit =
{
    {
        &CLSID_HTMLInputElement,    // _pclsid
        0,                                // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                           // _pcpi
        ELEMENTDESC_TEXTSITE |
        ELEMENTDESC_DONTINHERITSTYLE |
        ELEMENTDESC_DEFAULT |             // input/submit is the default button
        ELEMENTDESC_SHOWTWS |
        ELEMENTDESC_NOTIFYENDPARSE |
        ELEMENTDESC_NOANCESTORCLICK |
        ELEMENTDESC_HASDEFDESCENT,
        &IID_IHTMLInputElement,     // _piidDispinterface
        &s_apHdlDescs,                    // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLInputElement,  // _pfnTearOff
    NULL,                                 // _pAccelsDesign
    NULL                                  // _pAccelsRun
};

// Reset Class Decriptor

CElement::CLASSDESC CInput::s_classdescReset =
{
    {
        &CLSID_HTMLInputElement,     // _pclsid
        0,                                 // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                    // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                            // _pcpi
        ELEMENTDESC_TEXTSITE |
        ELEMENTDESC_DONTINHERITSTYLE |
        ELEMENTDESC_CANCEL |               // input/reset is the cancel button
        ELEMENTDESC_SHOWTWS |
        ELEMENTDESC_NOTIFYENDPARSE |
        ELEMENTDESC_NOANCESTORCLICK |
        ELEMENTDESC_HASDEFDESCENT,
        &IID_IHTMLInputElement,      // _piidDispinterface
        &s_apHdlDescs,                     // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLInputElement,  // _pfnTearOff
    NULL,                                  // _pAccelsDesign
    NULL                                   // _pAccelsRun
};

CElement::CLASSDESC CInput::s_classdescFile =
{
    {
        &CLSID_HTMLInputElement,        // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_TEXTSITE |          // _dwFlags
        ELEMENTDESC_DONTINHERITSTYLE |
        ELEMENTDESC_SHOWTWS |
        ELEMENTDESC_CANSCROLL |
        ELEMENTDESC_VPADDING |
        ELEMENTDESC_NOTIFYENDPARSE |
        ELEMENTDESC_NOANCESTORCLICK |
        ELEMENTDESC_OMREADONLY |        // block OM from accessing value
        ELEMENTDESC_HASDEFDESCENT,
        &IID_IHTMLInputElement,     // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLInputElement,  // _apfnTearOff
    &s_AccelsInputTxtDesign,            // _pAccelsDesign
    &s_AccelsInputTxtRun                // _pAccelsRun
};

CElement::CLASSDESC CInput::s_classdescCheckbox =
{
    {
        &CLSID_HTMLInputElement,      // _pclsid
        0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                      // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                              // _pcpi
        ELEMENTDESC_NEVERSCROLL     |
        ELEMENTDESC_NOANCESTORCLICK |
        ELEMENTDESC_HASDEFDESCENT,          // _dwFlags
        &IID_IHTMLInputElement,       // _piidDispinterface
        &s_apHdlDescs,                       // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLInputElement,  //_apfnTearOff
    NULL,                                    // _pAccelsDesign
    NULL                                     // _pAccelsRun
};


LONG CInput::cxButtonSpacing = 2;
static TCHAR s_achUploadCaption[] = TEXT("Browse...");
CElement::CLASSDESC CInput::s_classdescImage =
{
    {
        &CLSID_HTMLInputElement,        // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_NEVERSCROLL     |
        ELEMENTDESC_NOANCESTORCLICK |
        ELEMENTDESC_DEFAULT |           // input/image can act as default button -#3397
        ELEMENTDESC_EXBORDRINMOV,       // _dwFlags
        &IID_IHTMLInputElement,    // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLInputElement,  // _pfnTearOff
    NULL,                               // _pAccelsDesign
    NULL                                // _pAccelsRun
};
   
static const IID * s_aInputInterface[] =   {
            &IID_IHTMLInputTextElement,         //    htmlInputNotSet = 0,
            &IID_IHTMLInputButtonElement,       //    htmlInputButton = 1,
            &IID_IHTMLOptionButtonElement,      //    htmlInputCheckbox = 2,
            &IID_IHTMLInputFileElement,         //    htmlInputFile = 3,
            &IID_IHTMLInputHiddenElement,       //    htmlInputHidden = 4,
            &IID_IHTMLInputImage,               //    htmlInputImage = 5,
            &IID_IHTMLInputTextElement,         //    htmlInputPassword = 6,
            &IID_IHTMLOptionButtonElement,      //    htmlInputRadio = 7,
            &IID_IHTMLInputButtonElement,       //    htmlInputReset = 8,
            NULL,                               //    htmlInputSelectOne = 9,
            NULL,                               //    htmlInputSelectMultiple = 10,
            &IID_IHTMLInputButtonElement,       //    htmlInputSubmit = 11,
            &IID_IHTMLInputTextElement,         //    htmlInputText = 12,
            NULL,                               //    htmlInputTextarea = 13,
            NULL,                               //    htmlInputRichtext = 14,
            };

HRESULT 
CInput::CreateLayout()
{
    Assert(!HasLayoutPtr());

    CLayout * pLayout = NULL;

    switch (GetType())
    {
    case htmlInputButton:
    case htmlInputReset:
    case htmlInputSubmit:
        pLayout = new CInputButtonLayout(this);
        break;
    case htmlInputFile:
        pLayout = new CInputFileLayout(this);
        break;
    case htmlInputText:
    case htmlInputPassword:
    case htmlInputHidden:
        pLayout = new CInputTextLayout(this);
        break;
    case htmlInputCheckbox:
    case htmlInputRadio:
        pLayout = new CCheckboxLayout(this);
        break;
    case htmlInputImage:
        pLayout = new CInputImageLayout(this);
        break;
    default:
        AssertSz(FALSE, "Illegal Input Type");
    }

    if (!pLayout)
        return(E_OUTOFMEMORY);

    SetLayoutPtr(pLayout);

    return (S_OK);
} 

CInput::CInput (ELEMENT_TAG etag, CDoc *pDoc, htmlInput type)
: CSite(etag, pDoc)
{
    _fHasInitValue  = FALSE;
    _fInSave        = FALSE;
#ifdef WIN16
    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
    m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
    SetType(type);
    SetTypeAtCreate(type);
    switch (type)
    {
    case htmlInputButton:
    case htmlInputReset:
    case htmlInputSubmit:
    case htmlInputImage:
        _fActsLikeButton = TRUE;
        break;
    }
    _icfButton = -1;
}

CLayout* 
CInput::Layout()                    
{
    return GetLayoutPtr();
}


#ifndef NO_PROPERTY_PAGE
const CLSID * CInput::s_apclsidPages[] =
{
    // Browse-time pages
    NULL,
    // Edit-time pages
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,
    &CLSID_CInlineStylePropertyPage,
#endif // DBG==1    
    NULL
};
#endif // NO_PROPERTY_PAGE

extern class CFontCache & fc();

BOOL
IsTypeMultiline(htmlInput type)
{
    switch (type)
    {
    case htmlInputButton:
    case htmlInputSubmit:
    case htmlInputReset:
    case htmlInputHidden:
        return TRUE;
    default:
        return FALSE;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     Init
//
//  Synopsis:
//
//-------------------------------------------------------------------------

HRESULT
CInput::Init()
{
    HRESULT hr = S_OK;

    if (GetType() == htmlInputImage)
    {
        _pImage = new CImgHelper(Doc(), this, TRUE);

        if (!_pImage)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = super::Init();

Cleanup:
    RRETURN(hr);
}

HRESULT
CInput::Init2(CInit2Context * pContext)
{
    HRESULT hr = S_OK;
    CMarkup * pSlaveMarkup;

    htmlInput           type = GetType();

    // Convert bogus Input types to default. These enum values exist only
    // because TEXTAREA, HTMLAREA and SELECT support the type attribute
    // and use these values to specify the type. These values have no
    // meaning when used with an INPUT tag and are mapped to the default
    // value of htmlInputText.
    if (GetAAtype() != type)
    {
        Assert(type == htmlInputText);
        hr = THR(SetAAtype(type));
        if (hr)
            goto Cleanup;
    }

    switch(type)
    {
    case    htmlInputRadio:

        // Defer clearing the other radio buttons in the group. This would
        // prevent the collections from being accessed prematurely in
        // SetChecked() called from super::Init2(). The clearing would
        // happen in AddToCollections().

        _fDeferClearGroup = TRUE;

        // fall through

    case    htmlInputCheckbox:

        hr = THR(super::Init2(pContext));
        if (type == htmlInputRadio)
        {
            _fDeferClearGroup = FALSE;
        }
        goto Cleanup;

    case    htmlInputImage:
            RRETURN1 (super::Init2(pContext), S_INCOMPLETE);
    }

    // create inner element
    hr = THR(CreateSlave());
    if (hr)
        goto Cleanup;

    pSlaveMarkup = GetSlaveMarkupPtr();
    if (pSlaveMarkup)
        pSlaveMarkup->_fNoUndoInfo = TRUE;

    hr = THR(super::Init2(pContext));
    if (hr)
        goto Cleanup;

    // Set the default value for the control, if any. The default value
    // is specified using the 'value' attribute. If none was specified,
    // submit and reset buttons have their own implicit default values.

    hr = PrivateInit2();
    if (hr)
        goto Cleanup;

    if (pSlaveMarkup)
        pSlaveMarkup->_fNoUndoInfo = FALSE;

    _iHistoryIndex = (unsigned short)Doc()->_dwHistoryIndex++;

#ifdef  NEVER
    if (_fHasInitValue)
    {
        hr = SetValueHelper((TCHAR *) _cstrDefaultValue, _cstrDefaultValue.Length(), FALSE);
        if (hr)
            goto Cleanup;
    }
    else
    {
        UINT    uiBtnDefault    = IDS_BUTTONCAPTION_SUBMIT;
        TCHAR   pszCaption[128];
        int     c;
        
        switch (type)
        {
        case htmlInputReset:
            uiBtnDefault = IDS_BUTTONCAPTION_RESET;
            // fall through
        case htmlInputSubmit:
            c = LoadString(GetResourceHInst(),
                           uiBtnDefault, pszCaption, ARRAY_SIZE(pszCaption));
            if (c)
            {
                hr = THR(SetValueHelper(pszCaption, c));
                if (hr)
                    goto Cleanup;
            }
            break;
        }
    }

    _fTextChanged = FALSE;
#endif

Cleanup:

    RRETURN1(hr, S_INCOMPLETE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CInput::QueryStatus
//
//  Synopsis:   Called to discover if a given command is supported
//              and if it is, what's its state.  (disabled, up or down)
//
//--------------------------------------------------------------------------

HRESULT
CInput::QueryStatus(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));
    Assert(cCmds == 1);

    HRESULT     hr = S_OK;
    MSOCMD *    pCmd = & rgCmds[0];
    ULONG       cmdID;
    htmlInput   type = GetType();
    
    Assert(Doc());
    Assert(!pCmd->cmdf);

    cmdID = CBase::IDMFromCmdID( pguidCmdGroup, pCmd->cmdID );

    if (type == htmlInputImage)
    {
        Assert(_pImage);
        hr = _pImage->QueryStatus(
                    pguidCmdGroup,
                    1,
                    pCmd,
                    pcmdtext);
        if (!pCmd->cmdf)
        {
            hr = super::QueryStatus(pguidCmdGroup,
                                     1,
                                     pCmd,
                                     pcmdtext);
        }
        RRETURN_NOTRACE(hr);
    }
    else if (IsOptionButton())
    {
        RRETURN_NOTRACE(super::QueryStatus(
            pguidCmdGroup,
            1,
            pCmd,
            pcmdtext));
    }

    switch (cmdID)
    {
    case IDM_COPY:
    case IDM_CUT:
        if (type == htmlInputPassword)
        {
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
            return S_OK;
        }
        break;

    case IDM_INSERTOBJECT:
        // Don't allow objects to be inserted in input controls
        pCmd->cmdf = MSOCMDSTATE_DISABLED;
        return S_OK;

    case IDM_SELECTALL:
        // Button text should not be selectable in browse mode
        if (!IsEditable(TRUE) && IsButton())
        {
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
            return S_OK;
        }
        break;

    case IDM_PASTE:
        if (type == htmlInputFile)
        {
            IOleCommandTarget * pCT = NULL;
            
            GetCallerCommandTarget(this, NULL, FALSE, &pCT);
            if (pCT)
            {
                CVariant    Var;

                //
                // If this is a trusted doc, allow it to go through.
                //
                
                pCT->Exec(
                        &CGID_ScriptSite,
                        CMDID_SCRIPTSITE_TRUSTEDDOC,
                        0,
                        NULL,
                        &Var);
                ReleaseInterface(pCT);

                if (V_VT(&Var) == VT_BOOL && V_BOOL(&Var) == VARIANT_TRUE)
                    break;
                
                // Eat up command if being called through om
                hr = S_OK;  
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
            }
        }
        break;
    }

    if (!pCmd->cmdf)
    {
        hr = super::QueryStatus(pguidCmdGroup,
                                     1,
                                     pCmd,
                                     pcmdtext);
    }

    RRETURN_NOTRACE(hr);
}


HRESULT
CInput::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));

    int      idm = CBase::IDMFromCmdID(pguidCmdGroup, nCmdID);
    HRESULT  hr  = MSOCMDERR_E_NOTSUPPORTED;

    if (hr == MSOCMDERR_E_NOTSUPPORTED)
    {
        htmlInput type = GetType();

        switch(type)
        {
        case htmlInputImage:
            Assert(_pImage);
            hr = _pImage->Exec(
                pguidCmdGroup,
                nCmdID,
                nCmdexecopt,
                pvarargIn,
                pvarargOut);
            break;
        case htmlInputFile:
            if (idm == IDM_PASTE)
            {
                IOleCommandTarget * pCT = NULL;
                CDoc *pDoc = Doc();

                if (    pDoc->TestLock(FORMLOCK_QSEXECCMD)
                    || (pDoc->_pDocParent && pDoc->_pDocParent->TestLock(FORMLOCK_QSEXECCMD)))
                {
                    hr = E_ACCESSDENIED;
                }
                else
                {                
                    GetCallerCommandTarget(this, NULL, FALSE, &pCT);
                    if (pCT)
                    {
                        CVariant    Var;
                    
                        //
                        // If this is a trusted doc, allow it to go through.
                        //
                    
                        pCT->Exec(
                                &CGID_ScriptSite,
                                CMDID_SCRIPTSITE_TRUSTEDDOC,
                                0,
                                NULL,
                                &Var);
                        ReleaseInterface(pCT);
                    
                        if (V_VT(&Var) == VT_BOOL && V_BOOL(&Var) == VARIANT_TRUE)
                            break;
                    
                        // Eat up command if being called through om
                        hr = E_ACCESSDENIED;  
                    }
                }
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
    }

    RRETURN_NOTRACE(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CInput::Save
//
//  Synopsis:   Save the element to the stream
//
//-------------------------------------------------------------------------

HRESULT
CInput::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    HRESULT hr = S_OK;
    BOOL    fIsButton = IsButton();

    if (GetType() == htmlInputImage)
    {
        RRETURN (super::Save(pStreamWrBuff, fEnd));
    }

    // We do not want to call super here since that would dump what
    // is in the runs.

    Assert(!_fInSave);
    _fInSave = TRUE;

    if (IsOptionButton())
    {
        hr = SaveCheckbox(pStreamWrBuff, fEnd);
        goto Cleanup;
    }

    if (!fIsButton && !fEnd && !pStreamWrBuff->TestFlag(WBF_SAVE_PLAINTEXT))
    {
        DWORD dwOldFlags = pStreamWrBuff->ClearFlags(WBF_ENTITYREF);

        hr = WriteTag(pStreamWrBuff, fEnd);

        pStreamWrBuff->RestoreFlags(dwOldFlags);
    }

    if (fIsButton)
    {
        if (!fEnd)
        {
            pStreamWrBuff->BeginPre();
        }

        hr = super::Save(pStreamWrBuff, fEnd);
        if (hr)
            goto Cleanup;

        if(fEnd)
        {
            pStreamWrBuff->EndPre();
        }
    }

Cleanup:

    Assert(_fInSave); // this will catch recursion
    _fInSave = FALSE;

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Template:   CCharFilter
//
//  Synopsis:   This is a template class that will pre-process an input string
//              character class out of an input string without disturbing
//              the original string. It makes a copy of the string if
//              needed and copies to the stack if possible, allocating
//              only if the string is longer than a certain length.
//
//              Pre-processing will either filter out CR/LF (if !fMultiLine),
//              or converts each of LF, CR-LF and LF-CR to a CR. Also, presence
//              of non-ASCII characters is checked for.
//-----------------------------------------------------------------------------

MtDefine(CCharPreprocessPchAlloc, Locals, "CCharPreprocess::_pchAlloc");

#define BASELEN 64
class CCharPreprocess
{
public:
    CCharPreprocess()  { _pchAlloc = NULL; _chOR = 0; _fPrevIsCR = _fPrevIsLF = FALSE;}
    ~CCharPreprocess() { MemFree(_pchAlloc); }

    HRESULT         Preprocess(const TCHAR *pchIn, ULONG cchIn, BOOL fMultiLine);
    const TCHAR *   Pch() { return const_cast<TCHAR*>(_pchResult); }
    ULONG           Cch() { return _cchResult; }
    BOOL            FAsciiOnly() { return _chOR <= 0x7F; }  

private:
    TCHAR   _ach[BASELEN];
    TCHAR * _pchAlloc;
    TCHAR * _pchResult;
    ULONG   _cchResult;
    TCHAR   _chOR;
    BOOL    _fPrevIsCR;
    BOOL    _fPrevIsLF;
};

HRESULT
CCharPreprocess::Preprocess(const TCHAR *pchIn, ULONG cchIn, BOOL fMultiLine)
{
    const TCHAR *pch;
    TCHAR *pchTo;
    ULONG cch;
    HRESULT hr;

    for (pch = pchIn, cch = cchIn; cch; pch++, cch--)
    {
        _chOR |= *pch;

        if (*pch == _T('\r') || *pch == _T('\n'))
        {
            if (cchIn > BASELEN)
            {
                hr = THR(MemRealloc(Mt(CCharPreprocessPchAlloc), (void**)&_pchAlloc, cchIn * sizeof(TCHAR)));
                if (hr)
                    RRETURN(hr);

                _pchResult = _pchAlloc;
            }
            else
            {
                _pchResult = _ach;
            }

            pchTo = _pchResult;
            memcpy(pchTo, pchIn, (cchIn - cch) * sizeof(TCHAR));
            pchTo += (cchIn - cch);
            _cchResult = (cchIn - cch);

            for (; cch; pch++, cch--)
            {
                _chOR |= *pch;

                if (fMultiLine)
                {
                    if (*pch == _T('\r'))
                    {
                        if (!_fPrevIsLF)
                        {
                            *pchTo++ = _T('\r');
                            _cchResult++;
                            _fPrevIsCR = TRUE;
                        }
                        else
                        {
                            _fPrevIsCR = FALSE;
                        }
                        _fPrevIsLF = FALSE;
                    }
                    else if (*pch == _T('\n'))
                    {
                        if (!_fPrevIsCR)
                        {
                            *pchTo++ = _T('\r');
                            _cchResult++;
                            _fPrevIsLF = TRUE;
                        }
                        else
                        {
                            _fPrevIsLF = FALSE;
                        }
                        _fPrevIsCR = FALSE;
                    }
                    else
                    {
                        *pchTo++ = *pch;
                        _cchResult++;
                        _fPrevIsCR = _fPrevIsLF = FALSE;
                    }
                }
                else
                {
                    if (!(*pch == _T('\r') || *pch == _T('\n')))
                    {
                        *pchTo++ = *pch;
                        _cchResult++;
                    }
                }

            }
            return S_OK;
        }
    }

    _pchResult = const_cast<TCHAR*>(pchIn);
    _cchResult = cchIn;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Method:     SetValueHelper
//
//  Synopsis:   Removes forbidden characters from the string before
//              setting the value.
//
//-------------------------------------------------------------------------

HRESULT
CInput::SetValueHelper(const TCHAR *psz, int c, BOOL fOM /* = TRUE */)
{
    //  This is a no-op for <input type=file>
    //  When the file control needs to update its contents
    //  for the file pick dialog, it will call
    //  SetValueHelperReal directly.

    HRESULT             hr;
    BOOL                fAsciiOnly = FALSE;
    CCharPreprocess     Charf;

    if (TestClassFlag(ELEMENTDESC_OMREADONLY))
        return S_OK;

#ifdef  NEVER   // look at bug 24311 
    if (IsTextOrFile())
    {
        int l = GetAAmaxLength();
        c = min(c, l);
    }
#endif

    // Type is not set until Init2() is called
    Assert (GetType() != htmlInputNotSet);
    hr = Charf.Preprocess(psz, c, IsTypeMultiline(GetType()));
    if (hr)
        RRETURN(hr);

    psz = Charf.Pch();
    c = Charf.Cch();
    fAsciiOnly = Charf.FAsciiOnly();

    RRETURN(THR(SetValueHelperReal(psz, c, fAsciiOnly, fOM)));
}



HRESULT
CInput::SetValueHelperReal(const TCHAR *psz, int c, BOOL fAsciiOnly, BOOL fOM /* = TRUE */)
{
    HRESULT     hr = S_OK;

    // Temp. code. Needs to be re-visited
    CMarkup *       pMarkupSlave = GetSlaveMarkupPtr();
    if (!pMarkupSlave)
    {
        _cstrDefaultValue.Set(psz, c);
        _fTextChanged = FALSE;
        _fHasInitValue = TRUE;
    }
    else
    {
        hr = THR(pMarkupSlave->FastElemTextSet(pMarkupSlave->FirstElement(), psz, c, fAsciiOnly));

        if (hr)
            goto Cleanup;
        // Set this to prevent OnPropertyChange(_Value_) from firing twice
        // when value is set through OM. This flag is cleared in OnTextChange().
        _fFiredValuePropChange = fOM;
        hr = THR(_cstrLastValue.Set(psz, c));
        _fLastValueSet = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CInput::SetValueHelperInternal(CStr *pstr, BOOL fOM /* = TRUE */)
{
    HRESULT hr = S_OK;
    htmlInput   type = GetType();

    switch (type)
    {
    case    htmlInputText:
    case    htmlInputHidden:
    case    htmlInputPassword:
    case    htmlInputButton:
    case    htmlInputReset:
    case    htmlInputSubmit:
    case    htmlInputFile:
        hr = THR(SetValueHelper((LPTSTR) *pstr, pstr->Length(), fOM));

#ifndef NO_DATABINDING
        if (SUCCEEDED(hr))
        {
            // if the value is changed by script (or any way besides user
            // typing, or databinding), try to save the value into the
            // database.  But if user cancels this, leave the new value
            // in place.
            hr = SaveDataIfChanged(ID_DBIND_DEFAULT);
            if (SUCCEEDED(hr) || hr == E_ABORT)
                hr = S_OK;
        }
#endif

        _fTextChanged = FALSE;
        break;
    default:
        _fHasInitValue = TRUE;
        _cstrDefaultValue.Set(*pstr);
    }

    RRETURN(hr);
}

HRESULT
CInput::SetValueHelper(CStr *pstr)
{
    return SetValueHelperInternal(pstr);
}


HRESULT
CInput::GetValueHelper(CStr *pstr, BOOL fIsSubmit)
{
    HRESULT hr          = S_OK;

    switch (GetType())
    {
    case    htmlInputText:
    case    htmlInputHidden:
    case    htmlInputPassword:
    case    htmlInputFile:
    case    htmlInputButton:
    case    htmlInputReset:
    case    htmlInputSubmit:
        // Do not save password contents
        if (_fInSave && !IsEditable(TRUE) && GetType() == htmlInputPassword)
        {
            Assert(!fIsSubmit);
            pstr->Set(_T(""));
            goto Cleanup;
        }

        if (HasSlaveMarkupPtr())
        {
            hr = THR(GetSlaveMarkupPtr()->FirstElement()->GetPlainTextInScope(pstr));
        }
        else
        {
            pstr->Set(*GetLastValue());
        }
        break;
    case    htmlInputCheckbox:
    case    htmlInputRadio:
        if (!_fHasInitValue && !_fInSave)
        {
            pstr->Set(_T("on"));
            goto Cleanup;
        }
        //fall through
    default:
        pstr->Set(_cstrDefaultValue);
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CInput::GetValueHelper(CStr *pstr)
{
    HRESULT     hr = S_OK;
    hr = GetValueHelper(pstr, FALSE);
    RRETURN (hr);
}


const CBase::CLASSDESC *
CInput::GetClassDesc() const
{
    switch (GetType())
    {
        case htmlInputReset:
            return (CBase::CLASSDESC *)&s_classdescReset;

        case htmlInputSubmit:
            return (CBase::CLASSDESC *)&s_classdescSubmit;

        case htmlInputButton:
            return (CBase::CLASSDESC *)&s_classdescBtn;

        case htmlInputPassword:
            return (CBase::CLASSDESC *)&s_classdescPassword;

        case htmlInputHidden:
            return (CBase::CLASSDESC *)&s_classdescHidden;

        case htmlInputFile:
            return (CBase::CLASSDESC *)&s_classdescFile;

        case htmlInputCheckbox:
        case htmlInputRadio:
            return (CBase::CLASSDESC *)&s_classdescCheckbox;

        case htmlInputImage:
            return (CBase::CLASSDESC *)&s_classdescImage;

        default:
            return (CBase::CLASSDESC *)&s_classdescText;
    }
}


HRESULT
CInput::CreateElement(CHtmTag *pht, CDoc *pDoc, CElement **ppElement, htmlInput type)
{
    Assert(ppElement);

    *ppElement = new CInput(pht->GetTag(), pDoc, type);

    RRETURN ( (*ppElement) ? S_OK : E_OUTOFMEMORY);
}


//+---------------------------------------------------------------------------
//
//  Member: CInput::GetInfo
//
//  Params: [gi]: The GETINFO enumeration.
//
//  Descr:  Returns the information requested in the enum
//
//----------------------------------------------------------------------------
DWORD
CInput::GetInfo(GETINFO gi)
{
    switch (gi)
    {
    case GETINFO_HISTORYCODE:
        return MAKELONG(GetType(), Tag());
    }

    return super::GetInfo(gi);
}

//+------------------------------------------------------------------------
//
//  Member:     CInput::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------
                
HRESULT
CInput::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;

    if (!_fScriptCreated)
    {
        // if the type (or interface) has not been changed
        // we only support the old interface
        if (*s_aInputInterface[_typeAtCreate] == iid)
        {
            switch (iid.Data1)
            {
                QI_HTML_TEAROFF(this, IHTMLInputImage, NULL);
                QI_HTML_TEAROFF(this, IHTMLInputHiddenElement, NULL);
                QI_HTML_TEAROFF(this, IHTMLInputTextElement, NULL);
                QI_HTML_TEAROFF(this, IHTMLOptionButtonElement, NULL);
                QI_HTML_TEAROFF(this, IHTMLInputButtonElement, NULL);
                QI_HTML_TEAROFF(this, IHTMLInputFileElement, NULL);

                default:
                    AssertSz(FALSE, "Invalid interface IID");
            }
            goto Cleanup;
        }
        else
        {
            int i;

            // all possible interfaces are in between htmlInputNotSet
            // and htmlInputImage
            for (i =  (int)htmlInputNotSet; i <= (int)htmlInputImage; i++)
            {
                if (*s_aInputInterface[i] == iid)
                {
                    RRETURN (E_NOINTERFACE);
                }
            }
        }
    }

    switch(GetType())
    {
    case htmlInputImage:
        switch (iid.Data1)
        {
            QI_TEAROFF(this, IDispatchEx, NULL);
            QI_HTML_TEAROFF(this, IHTMLElement2, NULL);
            QI_HTML_TEAROFF(this, IHTMLInputElement, NULL);
        }
        break;
    default:
        switch (iid.Data1)
        {
            QI_HTML_TEAROFF(this, IHTMLInputElement, NULL);
        }
        break;
    }

Cleanup:

    if (*ppv)
    {
        ((IUnknown *) *ppv)->AddRef();
        RRETURN(S_OK);
    }

    RRETURN(super::PrivateQueryInterface(iid, ppv));
}

HRESULT
CInput::EnterTree()
{
    htmlInput   type    = GetType();
    HRESULT     hr      = S_OK;

#if DBG==1
    if (!IsTagEnabled(tagAllowMorphing))
#endif
    
    // no morphing allowed once the element is entering the tree
    _fHasMorphed = TRUE;
    
    switch (type)
    {
    case    htmlInputFile:
        {
            CInputFileLayout *pLayout;
            HRESULT hr;
        
            //  Load the caption string
            pLayout = DYNCAST(CInputFileLayout, Layout());
            hr = LoadString(GetResourceHInst(),
                            IDS_BUTTONCAPTION_UPLOAD,
                            &pLayout->_cchButtonCaption,
                            &pLayout->_pchButtonCaption);
            if (FAILED(hr))
            {
                pLayout->_pchButtonCaption = s_achUploadCaption;
                pLayout->_cchButtonCaption = ARRAY_SIZE(s_achUploadCaption) - 1;
            }
    
            hr = S_OK;
        }
    case    htmlInputText:
    case    htmlInputHidden:
        hr = THR(LoadHistoryValue());
        if (!OK(hr))
        {
            // See comments for _cstrLastValue in inputtxt.hxx
            if (_cstrLastValue == NULL && htmlInputHidden == type)
                SetValueHelper(_T("  "), 2);

            hr = S_OK;
        }
        // fall through
    case    htmlInputPassword:
        DYNCAST(CInputLayout, Layout())->GetDisplay()->SetWordWrap(FALSE);
        GetCurLayout()->_fAllowSelectionInDialog = TRUE;
        break;

    case    htmlInputSubmit:
        SetDefaultElem();
        // fall through
    case    htmlInputButton:
    case    htmlInputReset:
        DYNCAST(CInputLayout, Layout())->GetDisplay()->SetWordWrap(FALSE);
        break;
    case    htmlInputRadio:
        hr = THR(LoadHistoryValue());
        _fDeferClearGroup = FALSE;
        break;
    case    htmlInputCheckbox:
        hr = THR(LoadHistoryValue());
        break;
    case    htmlInputImage :
        SetDefaultElem();
        break;
    }

    RRETURN(hr);
}

HRESULT
CInput::LoadHistoryValue()
{
    HRESULT         hr      = S_OK;
    CStr            cstrVal;
    DWORD           dwTemp;
    IStream        *pStream = NULL;
    BOOL            fHistorySet = FALSE;
    VARIANT_BOOL    checked  = VARIANT_FALSE;
    CDoc           *pDoc = Doc();
    DWORD           dwHistoryIndex = 0x80000000 | (DWORD)_iHistoryIndex & 0x0FFFF;

    hr = THR(pDoc->GetLoadHistoryStream(dwHistoryIndex, HistoryCode(), &pStream));
    if (hr)
    {
        if (IsOptionButton())
            goto NoHistory;
        else
            goto Cleanup;
    }

    if (pStream)
    {
        CDataStream ds(pStream);

        if (IsOptionButton())
        {
            if (OK(THR(ds.LoadDword(&dwTemp))))
            {
                checked = (VARIANT_BOOL)(BOOL)dwTemp;
                fHistorySet = TRUE;
            }
        }
        else
        {
            Assert(IsTextOrFile());
            DWORD   dwEncoding;

            // Load encoding changing history
            hr = THR(ds.LoadDword(&dwEncoding));
            if (hr)
                goto Cleanup;

            if (!dwEncoding)
            {
                // load value
                hr = THR(ds.LoadCStr(&cstrVal));
                if (hr)
                    goto Cleanup;

                hr = THR(SetValueHelperInternal(&cstrVal));
                if (hr)
                    goto Cleanup;

                // load _fTextChanged
                hr = THR(ds.LoadDword(&dwTemp));
                if (hr)
                    goto Cleanup;

                _fTextChanged = dwTemp ? TRUE : FALSE;
            }
        }
    }
NoHistory:
    if (IsOptionButton())
    {
        hr = S_OK;

        if (!fHistorySet)
            get_PropertyHelper(&checked, (PROPERTYDESC *)&s_propdescCInputdefaultChecked);

        SetChecked(checked);

        _fLastValue = !!checked;
    }

Cleanup:
    ReleaseInterface(pStream);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     DoReset
//
//  Synopsis:   Called from CForm::Reset(). this helper assistes the form reset
//      operation by restoreing the default value to the value of this input
//      element
//
//----------------------------------------------------------------------------

HRESULT
CInput::DoReset(void)
{
    HRESULT hr = S_OK;

    switch(GetType())
    {
    case    htmlInputFile:
        hr = THR(SetValueHelperReal(NULL, 0));

        if (hr == S_OK)
            IGNORE_HR(OnPropertyChange(DISPID_A_VALUE, 0));
        break;
    case    htmlInputCheckbox:
    case    htmlInputRadio:
        VARIANT_BOOL bCheck;
        hr = THR(get_PropertyHelper(&bCheck, (PROPERTYDESC *)&s_propdescCInputdefaultChecked));

        if (hr)
            goto Cleanup;

        hr = THR(put_checked(bCheck));
        break;
    case    htmlInputText:
    case    htmlInputHidden:
    case    htmlInputPassword:
        hr = SetValueHelperInternal( &_cstrDefaultValue);
        break;
    }

Cleanup:
    RRETURN (hr);
}

HRESULT
CInput::GetSubmitInfoForImg(CPostData * pSubmitData)
{
    HRESULT     hr;
    LPCTSTR     pchName = GetAAsubmitname();
    CDoc *      pDoc = Doc();

    // Write x coord
    if (pchName)
    {
        hr = THR(pSubmitData->AppendEscaped(pchName, pDoc));
        if (hr)
            goto Cleanup;
        hr = THR(pSubmitData->Append(".x"));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(pSubmitData->Append("x"));
        if (hr)
            goto Cleanup;
    }
    hr = THR(pSubmitData->AppendValueSeparator());
    if (hr)
        goto Cleanup;

// BUGBUG: Ensure that _pt is stored in local coordinates (brendand)
    hr = THR(pSubmitData->Append(_pt.x));
    if (hr)
        goto Cleanup;

    hr = THR(pSubmitData->AppendItemSeparator());
    if (hr)
        goto Cleanup;

    // Write y coord
    if (pchName)
    {
        hr = THR(pSubmitData->AppendEscaped(pchName, pDoc));
        if (hr)
            goto Cleanup;
        hr = THR(pSubmitData->Append(".y"));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(pSubmitData->Append("y"));
        if (hr)
            goto Cleanup;
    }
    hr = THR(pSubmitData->AppendValueSeparator());
    if (hr)
        goto Cleanup;

// BUGBUG: Ensure that _pt is stored in local coordinates (brendand)
    hr = THR(pSubmitData->Append(_pt.y));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------------------
//
//  Method:     GetSubmitInfo
//
//  Synopsis:   returns the submit info string if there is a value
//              (name && value pair)
//
//  Returns:    S_OK if successful
//              E_NOTIMPL if not applicable for current element
//
//-----------------------------------------------------------------------------
HRESULT
CInput::GetSubmitInfo(CPostData * pSubmitData)
{
    htmlInput   type = GetType();
    LPCTSTR     pstrName;
    CFormElement    *pForm;
    ULONG       cElements;
    CStr        cstrValue;
    HRESULT     hr = S_FALSE;
    CDoc        *pDoc = Doc();

    if (type == htmlInputImage)
    {
        RRETURN(GetSubmitInfoForImg(pSubmitData));
    }

    pstrName = GetAAsubmitname();

    //  no name --> no submit!
    if ( ! pstrName )
        return S_FALSE;

    hr = GetValueHelper(&cstrValue, TRUE);

    if (hr)
        goto Cleanup;

    if (type == htmlInputFile)
    {
        if (_fDirtiedByOM && !pDoc->_fTrustedDoc)
        {
            // Null out the contents, set focus back to the control and cancel the submit operation
            AssertSz(FALSE, "Attempted security breach - trying to submit InputFile dirtied through OM");
            _fDirtiedByOM = FALSE;
            IGNORE_HR(SetValueHelperReal(NULL, 0, TRUE, FALSE));
            IGNORE_HR(BecomeCurrent(0));
            // BUGBUG (MohanB) bring up error alert here?
            hr = E_ACCESSDENIED;
            goto Cleanup;
        }
        else
        {
            RRETURN1(THR(pSubmitData->AppendNameFilePair(pstrName, cstrValue, pDoc)), S_FALSE);
        }
    }

    pForm = GetParentForm();

    if (!pForm)
        goto Cleanup;

    cElements  = pForm->_pCollectionCache->SizeAry(CFormElement::FORM_SUBMIT_COLLECTION);

    if (IsOptionButton())
    {
        VARIANT_BOOL  bCheck;
        HRESULT hr = THR(get_checked(&bCheck));

        if (hr || !bCheck)
        {
            hr = S_FALSE;
            goto Cleanup;
        }
    }

    if ( _tcsiequal( pstrName, TEXT("_charset_")) && htmlInputHidden == type)
    {
        TCHAR   achCharset[MAX_MIMECSET_NAME];

        if (pSubmitData->_fCharsetNotDefault && pForm->Utf8InAcceptCharset())
        {
            Assert(sizeof(s_achUtf8) <= sizeof(achCharset));
            memcpy(achCharset, s_achUtf8, sizeof(s_achUtf8));
        }
        else
        {
            hr = THR(GetMlangStringFromCodePage(
                pSubmitData->_fCharsetNotDefault && !pSubmitData->_fCodePageError ? 
                pSubmitData->_cpInit :
                pSubmitData->GetCP(pDoc), 
                achCharset,
                ARRAY_SIZE(achCharset)));
            if (hr)
                goto Cleanup;
        }

        hr = THR(pSubmitData->AppendNameValuePair(pstrName, 
            achCharset, pDoc));
    }    
    else if ( type != htmlInputText || _tcsicmp( pstrName, TEXT("isindex")) )
    {
        hr = THR(pSubmitData->AppendNameValuePair(pstrName, cstrValue, pDoc));
    }
    else
    {
        Assert (htmlInputText == type);
        // For <ISINDEX ...> we don't submit the "name=" part.
        /* NASA fix (bug 27359)
            If the ISINDEX textbox is the first element we're submitting
            (not necessarily the first element on the form), we don't
            submit the name-value pair.
        */
        if (!pSubmitData->Size())
        {
            hr = THR(pSubmitData->AppendEscaped(cstrValue, pDoc));
        }
        else
        {
            hr = THR(pSubmitData->AppendNameValuePair(pstrName, cstrValue, pDoc));
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


HRESULT CInput::DoClick(CMessage * pMessage, CTreeNode *pNodeContext,
                              BOOL fFromLabel)
{
    HRESULT hr = S_OK;

    if (IsOptionButton())
    {
        hr = DoClickCheckbox(pMessage, pNodeContext, fFromLabel);
    }
    else if (!IsEditable(TRUE) || !IsButton())
    {
        hr = super::DoClick(pMessage, pNodeContext, fFromLabel);
    }
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CInput::GetEnabled
//
//  Synopsis:   return not disabled
//
//----------------------------------------------------------------------------

STDMETHODIMP
CInput::GetEnabled(VARIANT_BOOL * pfEnabled)
{
    if (!pfEnabled)
        RRETURN(E_INVALIDARG);

    *pfEnabled = !GetAAdisabled();
    return S_OK;
}

HRESULT BUGCALL
CInput::select(void)
{
    HRESULT             hr          = S_OK;
    CMarkup *           pMarkup     = GetSlaveMarkupPtr();
    CDoc *              pDoc        = Doc();
    CMarkupPointer      ptrStart(pDoc);
    CMarkupPointer      ptrEnd(pDoc); 
    IMarkupPointer *    pIStart; 
    IMarkupPointer *    pIEnd; 

    if (!pMarkup || !IsInMarkup())
        goto Cleanup;
#if 0
    hr = pDoc->SetEditContext(this, TRUE, FALSE);
#else
    // BUGBUG (MohanB) We need to make this current because that's the only
    // way selection works right now. GetCurrentSelectionRenderingServices()
    // looks for the current element. MarkA should fix this.
    hr = BecomeCurrent(0);
#endif
    if (hr)
        goto Cleanup;

    hr = ptrStart.MoveToCp(2, pMarkup);
    if (hr)
        goto Cleanup;
    hr = ptrEnd.MoveToCp(pMarkup->GetTextLength()-2, pMarkup);
    if (hr)
        goto Cleanup;
    Verify(S_OK == ptrStart.QueryInterface(IID_IMarkupPointer, (void**)&pIStart));
    Verify(S_OK == ptrEnd.QueryInterface(IID_IMarkupPointer, (void**)&pIEnd));
    hr = pDoc->Select(pIStart, pIEnd, SELECTION_TYPE_Selection);
    pIStart->Release();
    pIEnd->Release();
Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange
//
//  Note:       Called after a property has changed to notify derived classes
//              of the change.  All properties (except those managed by the
//              derived class) are consistent before this call.
//
//              Also, fires a property change notification to the site.
//
//-------------------------------------------------------------------------

HRESULT
CInput::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    htmlInput type = GetType();

    switch (dispid)
    {
    case DISPID_CInput_type:
        {
           htmlInput typeNew = GetAAtype();

           // NOTE: (krisma) if we should ever want to turn on infinite morphing,
           // just remove the next bit of code. You can then probably get rid of
           // the _fHasMorphed flag.
           if (_fHasMorphed || type != htmlInputText)
           {
                // We need to set the type back to it's old value.
                hr = SetAAtype(type);
                if (hr)
                    goto Cleanup;

                hr = OLECMDERR_E_NOTSUPPORTED;
                goto Cleanup;
           }
           // End of morph-limiting code.
           else
           {

           // The following types are valid, but we realy just want a textbox.
           if (typeNew == htmlInputNotSet         ||
               typeNew == htmlInputSelectOne      ||
               typeNew == htmlInputSelectMultiple ||
               typeNew == htmlInputTextarea       ||
               typeNew == htmlInputRichtext)
           {
               typeNew = htmlInputText;
               hr = SetAAtype(typeNew);
               if (hr)
                   goto Cleanup;
           }

           // Do we realy have to do anything?
           if (typeNew == type)
               break;

           if ((typeNew == htmlInputCheckbox  ||
                typeNew == htmlInputRadio     ||
                typeNew == htmlInputImage)    &&
               (type    != htmlInputCheckbox  &&
                type    != htmlInputRadio     &&
                type    != htmlInputImage))
           {
               Assert (HasSlaveMarkupPtr());
               CMarkup * pMarkupSlave = DelSlaveMarkupPtr();

               pMarkupSlave->ClearMaster();
               pMarkupSlave->Release();
           }

           // delete the image helper
           if (type == htmlInputImage)
           {
               Assert(_pImage);
               _pImage->CleanupImage();
               delete _pImage;
               _pImage = NULL;
           }

           CLayout * pLayout;
           
           pLayout = DelLayoutPtr();
           pLayout->Detach();
           Assert(pLayout->_fDetached);
           pLayout->Release();

           SetType(typeNew);

           hr = THR(CreateLayout());
           if (hr)
               goto Cleanup;

           hr = THR(Layout()->Init());
           if (hr)
               goto Cleanup;

           // If we're changing from a password, we have to remove the value
           if (type == htmlInputPassword)
           {
               hr = SetValueHelperReal(_T(""), 0, TRUE, FALSE);
           }

           if (typeNew == htmlInputImage)
           {
               // if the new type is input image, create the imbedded input image instance
               _pImage = new CImgHelper(Doc(), this, TRUE);
           }

           dwFlags |= ELEMCHNG_CLEARCACHES | ELEMCHNG_SITEREDRAW | ELEMCHNG_REMEASUREINPARENT;
           _fHasMorphed = TRUE;

           hr = PrivateInit2();
           if (hr)
               goto Cleanup;
           }
        }
        break;
    case DISPID_CElement_submitName:
        {
            // DISPID_CInput_name gets changes to DISPID_CElement_submitName before we get here
            // and that prevents onpropertychange from firing for the name property (bug 30267)
            // This call gets us to fire onpropertychage if the name is changed.
            hr = OnPropertyChange(DISPID_CInput_name, dwFlags);
            if (hr)
                goto Cleanup;
        }
        break;
    case DISPID_CInput_readOnly:
        if (IsTextOrFile())
            _fEditAtBrowse = !GetAAreadOnly();
        break;

    case DISPID_CInput_src:
        if (_pImage && type == htmlInputImage)
        {
            hr = _pImage->SetImgSrc(IMGF_REQUEST_RESIZE | IMGF_INVALIDATE_FRAME);
        }
        break;
    case DISPID_CInput_lowsrc:
        if (_pImage && type == htmlInputImage)
        {
            LPCTSTR szUrl = GetAAsrc();

            if (!szUrl)
            {
                Assert(_pImage);
                hr = _pImage->FetchAndSetImgCtx(GetAAlowsrc(), IMGF_REQUEST_RESIZE | IMGF_INVALIDATE_FRAME);
            }

        }
        break;

#ifndef NO_AVI
    case DISPID_CInput_dynsrc:
        if (_pImage && GetType() == htmlInputImage)
        {
            hr = _pImage->SetImgDynsrc();
        }
        break;
#endif
    }

    hr = (THR(super::OnPropertyChange(dispid, dwFlags)));

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CInput::BecomeUIActive
//
//  Synopsis:   Check imeMode to set state of IME.
//
//  Notes:      This is the method that external objects should call
//              to force sites to become ui active.
//
//--------------------------------------------------------------------------

HRESULT
CInput::BecomeUIActive()
{
    HRESULT hr = S_OK;
    htmlInput   type = GetType();

    hr = THR(super::BecomeUIActive());
    if (hr)
        goto Cleanup;

    if (type == htmlInputText)
    {
        hr = THR(SetImeState());
        if (hr)
            goto Cleanup;
    }

Cleanup:    
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CInput::GetFocusShape
//
//  Synopsis:   Returns the shape of the focus outline that needs to be drawn
//              when this element has focus. This function creates a new
//              CShape-derived object. It is the caller's responsibility to
//              release it.
//
//----------------------------------------------------------------------------

HRESULT
CInput::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    HRESULT     hr;
    htmlInput   type = GetType();

    Assert(ppShape);

    switch(type)
    {
    case htmlInputCheckbox:
    case htmlInputRadio:
        hr = DYNCAST(CCheckboxLayout, Layout())->GetFocusShape(lSubDivision, pdci, ppShape);
        break;
    case htmlInputButton:
    case htmlInputReset:
    case htmlInputSubmit:
        hr = DYNCAST(CInputButtonLayout, Layout())->GetFocusShape(lSubDivision, pdci, ppShape);
        break;
    case htmlInputFile:
        hr = DYNCAST(CInputFileLayout, Layout())->GetFocusShape(lSubDivision, pdci, ppShape);
        break;
    case htmlInputImage:
        hr = DYNCAST(CInputImageLayout, Layout())->GetFocusShape(lSubDivision, pdci, ppShape);
        break;
    default:
        // Never want focus rcet for text/password
        *ppShape = NULL;
        hr = S_FALSE;
        break;
    }
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CInput::RequestYieldCurrency
//
//  Synopsis:   Check if OK to Relinquish currency
//
//  Arguments:  BOOl fForce -- if TRUE, force change and ignore user cancelling the
//                             onChange event
//
//  Returns:    S_OK: ok to yield currency
//
//--------------------------------------------------------------------------

HRESULT
CInput::RequestYieldCurrency(BOOL fForce)
{
    CStr    cstr;
    HRESULT hr = S_OK;
    BOOL    fIsText = IsTextOrFile();
    BOOL    fIsOptionButton = IsOptionButton();

    if(IsButton())
        RRETURN1(super::RequestYieldCurrency(fForce), S_FALSE);

    if ((fIsText && (hr = GetValueHelper(&cstr)) == S_OK) ||
        (fIsOptionButton && _fLastValue != _fChecked))
    {
        BOOL fFire =  fIsText ? (FormsStringCmp(cstr, *GetLastValue()) != 0)
            : (_fLastValue != _fChecked);

        if (!fFire)
            goto Cleanup;

        if (!Fire_onchange())   //JS event
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    
        if (!IsInMarkup())
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = super::RequestYieldCurrency(fForce);
        if (hr == S_OK)
        {
            if (fIsText)
            {
                _cstrLastValue.Set(cstr);
                _fLastValueSet = TRUE;
            }
            else 
            {
                Assert(fIsOptionButton);
                _fLastValue = _fChecked;
            }
        }
    }

Cleanup:
    if (fForce && FAILED(hr))
    {
        hr = S_OK;
    }

    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CInput::YieldCurrency
//
//  Synopsis:   Relinquish currency
//
//  Arguments:  pSiteNew    New site that wants currency
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CInput::YieldCurrency(CElement *pElemNew)
{
    HRESULT hr;

    _fDoReset = FALSE;
    hr = THR(super::YieldCurrency(pElemNew));
    if (hr)
        goto Cleanup;

    switch(GetType())
    {
    case    htmlInputFile:
        if (_fButtonHasFocus)
        {
            BtnHelperKillFocus();
        }
        break;
    case    htmlInputButton:
    case    htmlInputSubmit:
    case    htmlInputReset:
            _wBtnStatus = BTN_RESSTATUS(_wBtnStatus, FLAG_HASFOCUS);
            GetCurLayout()->Invalidate();
            break;
    case    htmlInputCheckbox:
    case    htmlInputRadio:
            _wBtnStatus = BTN_RESSTATUS(_wBtnStatus, FLAG_HASFOCUS);
            break;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT BUGCALL
CInput::HandleMessage(CMessage * pMessage)
{
    HRESULT     hr = S_FALSE;
    htmlInput   type = GetType();

    switch(type)
    {
    case    htmlInputButton:
    case    htmlInputReset:
    case    htmlInputSubmit:
        hr = HandleButtonMessage(pMessage);
        break;
    case    htmlInputText:
    case    htmlInputHidden:
    case    htmlInputPassword:
        hr = HandleTextMessage (pMessage);
        break;
    case    htmlInputFile:
        hr = HandleFileMessage (pMessage);
        break;
    case    htmlInputCheckbox:
    case    htmlInputRadio:
        hr = HandleCheckboxMessage (pMessage);
        break;
    case    htmlInputImage:
        hr = HandleImageMessage (pMessage);
        break;
    default:
        AssertSz(FALSE, "Invalid input element");
    }

    RRETURN1(hr, S_FALSE);
}

HRESULT BUGCALL
CInput::HandleFileMessage(CMessage * pMessage)
{
    HRESULT hr = S_FALSE;
    BOOL fButtonMessage = FALSE;
    BOOL fTextEditMessage = FALSE;
    CRect rc;
    CInputFileLayout * pLayout = DYNCAST(CInputFileLayout, GetCurLayout());

    if (pLayout)
    {
        pLayout->GetButtonRect(&rc);
    }
    else
    {
        rc = g_Zero.rc;
    }

    switch ( pMessage->message )
    {
    case WM_SETFOCUS:
        if ( _fButtonHasFocus )
        {
            BtnHelperSetFocus();
            fButtonMessage = TRUE;
        }
        else
        {
            fTextEditMessage = TRUE;
        }

        break;

    case WM_KILLFOCUS:
        if ( _fButtonHasFocus )
        {
            fButtonMessage = TRUE;
            hr = S_OK;
        }
        else
        {
            fTextEditMessage = TRUE;
        }

        break;

    case WM_KEYDOWN:
        if ( pMessage->wParam == VK_TAB )
        {
            if ( pMessage->dwKeyState & FSHIFT )
            {
                //  BackTAB
                if ( _fButtonHasFocus )
                {
                    CDoc    *pDoc = Doc();
                    BtnHelperKillFocus();
                    // set caret
                    if (pDoc->_pCaret)
                    {
                        pDoc->_pCaret->Show( FALSE );
                    }
                    hr = S_OK;
                }
                else
                {
                    fTextEditMessage = TRUE;
                }
            }
            else
            {
                //  Forward TAB
                if ( _fButtonHasFocus )
                {
                    fTextEditMessage = TRUE;
                    fButtonMessage = TRUE;
                }
                else
                {
                    BtnHelperSetFocus();
                    hr = S_OK;
                }
            }
        }
        else
        {
            fButtonMessage = _fButtonHasFocus;
            fTextEditMessage = ! fButtonMessage;
        }

        break;

    default:

        if ( HasCapture() )
        {
            fButtonMessage = _fButtonHasCapture;
        }
        else if ( pMessage->message >= WM_MOUSEFIRST &&
                  pMessage->message <= WM_MOUSELAST )
        {
            fButtonMessage = rc.Contains(pMessage->ptContent);

            if ( !fButtonMessage &&
                 pMessage->message == WM_LBUTTONDOWN )
            {
                BtnHelperKillFocus();
            }
        }
        else if ( rc.Contains(pMessage->ptContent) )
        {
            fButtonMessage = rc.Contains(pMessage->ptContent);
        }
        else if ( pMessage->message >= WM_KEYFIRST &&
                  pMessage->message <= WM_KEYLAST )
        {
            fButtonMessage = _fButtonHasFocus;
        }

        fTextEditMessage = ! fButtonMessage;

        break;
    }


    if ( fButtonMessage )
    {
        _fBtnHelperRequestsCurrency = TRUE;
        if (    pMessage->message != WM_LBUTTONDOWN
            ||  !BecomeCurrent(pMessage->lSubDivision, NULL, pMessage))
        {
            hr = THR(BtnHandleMessage(pMessage));
        }
        _fBtnHelperRequestsCurrency = FALSE;
    }
    if ( fTextEditMessage )
    {
        hr = THR(HandleTextMessage(pMessage));
    }

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
// Member:      CInputImage::HandleMessage
//
// Synopsis:    Handle window message
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
CInput::HandleImageMessage(CMessage * pMessage)
{
    HRESULT hr = S_FALSE;

    // Use hyperlink cursor (IE3 compat.)
    if (WM_SETCURSOR == pMessage->message)
    {
        SetCursorStyle(MAKEINTRESOURCE(IDC_HYPERLINK));
        hr = S_OK;
        goto Cleanup;
    }
    // WM_CONTEXTMENU message should always be handled.
    else if (WM_CONTEXTMENU == pMessage->message)
    {
        Assert(_pImage);
        hr = THR(_pImage->ShowImgContextMenu(pMessage));
    }
    else if (!IsEditable(TRUE))
    {
        hr = BtnHandleMessage(pMessage);
    }
    if (hr == S_FALSE)
    {
        // image does not have their own handlemessage
        hr = super::HandleMessage(pMessage);
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

HRESULT BUGCALL
CInput::HandleButtonMessage(CMessage * pMessage)
{
    HRESULT hr = S_FALSE;
    BOOL    fEditable = IsEditable( TRUE );

    if (!CanHandleMessage())
        goto Cleanup;

    if (!fEditable)
    {
        if (!IsEnabled())
            goto Cleanup;

        hr = BtnHandleMessage(pMessage);
        if (hr == S_FALSE)
        {
            hr = super::HandleMessage(pMessage);
        }
    }
    else
    {
        if (pMessage->message == WM_CONTEXTMENU)
        {
            hr = THR(OnContextMenu(
                    (short)LOWORD(pMessage->lParam),
                    (short)HIWORD(pMessage->lParam),
                    CONTEXT_MENU_CONTROL));
        }
        if (hr == S_FALSE)
        {
            hr = super::HandleMessage(pMessage);
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

HRESULT BUGCALL
CInput::HandleTextMessage(CMessage * pMessage)
{
    HRESULT         hr = S_FALSE;
    CFormElement *  pForm;
    BOOL            fEditable = IsEditable(TRUE);
    BOOL            fEnabled  = IsEnabled();

    Assert(IsTextOrFile());

    if ( !CanHandleMessage() ||
         (!fEditable && !fEnabled) )
    {
        goto Cleanup;
    }

    if (!fEditable && _fDoReset)
    {
        if (pMessage->message == WM_KEYDOWN)
        {
            if (pMessage->wParam == VK_ESCAPE)
            {
                pForm = GetParentForm();
                if (pForm)
                {
                    _fDoReset = FALSE;
                    hr = THR(pForm->DoReset(TRUE));
                    if (hr != S_FALSE)
                        goto Cleanup;
                }
            }
            else
            {
                _fDoReset = FALSE;
            }
        }
    }

    switch (pMessage->message)
    {
        case WM_CHAR:
            if (pMessage->wParam == VK_RETURN)
            {
                BOOL    fOnlyTextbox;

                pForm = GetParentForm();
                if (pForm)
                {
                    hr = THR(pForm->FOnlyTextbox(this, &fOnlyTextbox));
                    if (FAILED(hr))
                        goto Cleanup;
                    if (fOnlyTextbox)
                    {
                        IGNORE_HR(pForm->DoSubmit(this, TRUE));
                        hr = S_OK;
                        goto Cleanup;
                    }
                    else
                    {
                        hr = S_FALSE;
                    }
                }
            }
            break;


        // We handle all WM_CONTEXTMENUs
        case WM_CONTEXTMENU:
            hr = THR(OnContextMenu(
                    (short) LOWORD(pMessage->lParam),
                    (short) HIWORD(pMessage->lParam),
                    CONTEXT_MENU_CONTROL));
            goto Cleanup;
    }

    if (!fEditable  &&
        pMessage->message == WM_KEYDOWN &&
        pMessage->wParam == VK_ESCAPE)
    {
        _fDoReset = TRUE;
        SetValueHelperInternal(GetLastValue(), FALSE);

        // BUGBUG (MohanB) Why return S_FALSE instead of S_OK?
        hr = S_FALSE;
        goto Cleanup;
    }

    // Since we let TxtEdit handle messages we do JS events after
    // it comes back
    Assert(S_FALSE == hr);
    hr = super::HandleMessage(pMessage);

Cleanup:

    RRETURN1(hr, S_FALSE);
}


HRESULT
CInput::ClickAction(CMessage * pMessage)
{
    HRESULT         hr = S_OK;
    htmlInput       type = GetType();

    switch(type)
    {
    case    htmlInputButton:
        // Do nothing;
        break;
    case    htmlInputReset:
    case    htmlInputSubmit:
        hr = ClickActionButton(pMessage);
        break;
    case    htmlInputFile:
        hr = ClickActionFile(pMessage);
        break;
    case    htmlInputCheckbox:
        if (BTN_GETSTATUS(_wBtnStatus, FLAG_TRISTATE))
        {
            _wBtnStatus = BTN_RESSTATUS(_wBtnStatus, FLAG_TRISTATE);
            DYNCAST(CCheckboxLayout, GetCurLayout())->Invalidate();
        }
        else
        {
            put_checked(_fChecked ? VB_FALSE : VB_TRUE);
        }
        break;
    case    htmlInputRadio:
        // Do nothing if unnamed (Netscape Compat.)
        if (!_fChecked && GetAAname())
            put_checked(VB_TRUE);
        break;
    case    htmlInputImage:
        hr = ClickActionImage(pMessage);
        goto Cleanup;
        break;
    default:
        hr = super::ClickAction(pMessage);
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

HRESULT
CInput::ClickActionButton(CMessage * pMessage)
{
    HRESULT         hr = S_OK;
    CFormElement *  pForm;

    pForm = GetParentForm();
    if (pForm)
    {
        switch (GetType())
        {
            case htmlInputReset:
                hr = THR(pForm->DoReset(TRUE));
                break;

            case htmlInputSubmit:
                hr = THR(pForm->DoSubmit(this, TRUE));
                break;
        }
        if (hr == S_FALSE)
            hr = S_OK;
    }
    RRETURN1(hr, S_FALSE);
}

HRESULT
CInput::ClickActionImage(CMessage * pMessage)
{
    HRESULT         hr = S_OK;
    CFormElement *  pForm;

    if (pMessage &&
        pMessage->message >= WM_MOUSEFIRST &&
        pMessage->message <= WM_MOUSELAST)
    {
        _pt.x = pMessage->ptContent.x;
        _pt.y = pMessage->ptContent.y;
    }
    else
    {
        GetCurLayout()->GetPosition(&_pt, COORDSYS_CONTENT);
    }

    pForm = GetParentForm();
    if (pForm)
    {
        pForm->DoSubmit(this, TRUE);
    }
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CInput::ClickActionFile
//
//  Synopsis:   Display the File Open... common dialog here.
//
//----------------------------------------------------------------------------

HRESULT
CInput::ClickActionFile(CMessage * pMessage)
{
    HRESULT hr                      = S_OK;
    TCHAR   achBuf[FORMS_BUFLEN + 1];
    CStr    cstr;
    BSTR    bstr                    = NULL;

    // Do nothing unless the click is in the button portion (indicated
    // by non-zero click data)
    if (pMessage && 0 == pMessage->dwClkData)
        goto Cleanup;

    hr = THR(GetValueHelper(&cstr));
    if ( hr )
        goto Cleanup;

    if ( cstr )
    {
        _tcsncpy(achBuf, cstr, FORMS_BUFLEN);
        achBuf[FORMS_BUFLEN] = 0;
    }
    else
    {
        *achBuf = 0;
    }

    hr = THR(FormsGetFileName(FALSE,
                              Doc()->_pInPlace->_hwnd,
                              IDS_UPLOADFILE,
                              achBuf,
                              ARRAY_SIZE(achBuf), (LPARAM)0));

    if ( FAILED(hr) )
        goto Cleanup;

    if ( S_OK == hr )
    {
        hr = THR(SetValueHelperReal(achBuf, _tcslen(achBuf), FALSE));
        if ( hr )
            goto Cleanup;

        IGNORE_HR(OnPropertyChange(DISPID_A_VALUE, 0));
    }
    else
    {
        //  S_FALSE means the user cancelled the dialog
        hr = S_OK;
    }

Cleanup:
    SysFreeString(bstr);

    // Do not want this to bubble, so..
    if (S_FALSE == hr)
        hr = S_OK;

    RRETURN(hr);
}

HRESULT
CInput::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT         hr = S_OK;
    enum _htmlInput type = GetType();
    CDoc *          pDoc;
    LOGFONT         lf;
    CUnitValue      uvBorder;
    DWORD           dwRawValue;
    int             i;
    BOOL            fIsButton;

    switch(type)
    {
    case    htmlInputImage:
    case    htmlInputCheckbox:
    case    htmlInputRadio:
        hr = super::ApplyDefaultFormat(pCFI);
        goto Cleanup;
    }

    pDoc        = Doc();
    fIsButton   = IsButton();

    pCFI->PrepareCharFormat();
    pCFI->PrepareFancyFormat();
    pCFI->PrepareParaFormat();

    uvBorder.SetValue( 2, CUnitValue::UNIT_PIXELS );
    dwRawValue = uvBorder.GetRawValue();

    DefaultFontInfoFromCodePage( pDoc->GetCodePage(), &lf );

    if (fIsButton)
    {
        // Set default color and let super override it with the use style
        pCFI->_cf()._ccvTextColor.SetSysColor(COLOR_BTNTEXT);
        pCFI->_ff()._ccvBackColor.SetSysColor(COLOR_BTNFACE);

        pCFI->_cf()._fBold = FALSE;
        pCFI->_cf()._wWeight = 400;
        pCFI->_cf()._yHeight = 200; // 10 * 20 twips NS compatibility
        // pCFI->_cf()._yHeight = 160; // 8 * 20 twips IE3 compatibility

        pCFI->_bBlockAlign     = htmlBlockAlignCenter;

        pCFI->_ff()._bBorderSoftEdges = TRUE;

        pCFI->_ff()._ccvBorderColorLight.SetSysColor(COLOR_BTNFACE);
        pCFI->_ff()._ccvBorderColorDark.SetSysColor(COLOR_3DDKSHADOW);
        pCFI->_ff()._ccvBorderColorHilight.SetSysColor(COLOR_BTNHIGHLIGHT);
        pCFI->_ff()._ccvBorderColorShadow.SetSysColor(COLOR_BTNSHADOW);

        if (pCFI->_cf()._fVisibilityHidden || pCFI->_cf()._fDisplayNone)
            _fButtonWasHidden = TRUE;
    }
    else
    {
        // No vertical spacing between para's
        pCFI->_ff()._cuvSpaceBefore.SetPoints(0);
        pCFI->_ff()._cuvSpaceAfter.SetPoints(0);

        pCFI->_cf()._ccvTextColor.SetSysColor(COLOR_WINDOWTEXT);
        pCFI->_ff()._ccvBackColor.SetSysColor(COLOR_WINDOW);

        pCFI->_cf()._fBold = FALSE;
        pCFI->_cf()._wWeight = FW_NORMAL; //FW_NORMAL = 400
        pCFI->_cf()._yHeight = 200; // 10 * 20, 10 points

        // border
        pCFI->_ff()._ccvBorderColorLight.SetSysColor(COLOR_3DLIGHT);
        pCFI->_ff()._ccvBorderColorDark.SetSysColor(COLOR_3DDKSHADOW);
        pCFI->_ff()._ccvBorderColorHilight.SetSysColor(COLOR_BTNHIGHLIGHT);
        pCFI->_ff()._ccvBorderColorShadow.SetSysColor(COLOR_BTNSHADOW);

        //
        // Add default padding
        //
        uvBorder.SetValue(TEXT_INSET_DEFAULT_LEFT, CUnitValue::UNIT_PIXELS);

        pCFI->_ff()._cuvPaddingLeft   =
        pCFI->_ff()._cuvPaddingRight  =
        pCFI->_ff()._cuvPaddingTop    =
        pCFI->_ff()._cuvPaddingBottom = uvBorder;
    }

    // our intrinsics shouldn't inherit the cursor property. they have a 'default'
    pCFI->_cf()._bCursorIdx = styleCursorAuto;
    pCFI->_pf()._cuvTextIndent.SetPoints(0);

#ifdef  NEVER
    if (pDoc->IsPrintDoc())
    {
        CODEPAGESETTINGS * pCS = pDoc->_pCodepageSettings;

        CODEPAGE cp = pDoc->GetCodePage();
        // Thai does not have a fixed pitch font. Leave it as proportional
        if (cp != CP_THAI)
        {
            pCFI->_cf()._bPitchAndFamily = FIXED_PITCH;
            // pCFI->_cf._fBumpSizeDown = TRUE;         // not necessary?
            pCFI->_cf()._latmFaceName = pCS->latmFixedFontFace;
        }
        pCFI->_cf()._bCharSet = pCS->bCharSet;
    }
    else
#endif
    {
        pCFI->_cf()._bCharSet = lf.lfCharSet;
        pCFI->_cf().SetFaceName( lf.lfFaceName);
    }
    pCFI->_cf()._fNarrow = IsNarrowCharSet(pCFI->_cf()._bCharSet);

    for ( i = BORDER_TOP; i <= BORDER_LEFT; i++ )
    {
        pCFI->_ff()._cuvBorderWidths[i].SetRawValue( dwRawValue );
        if (!fIsButton)
            pCFI->_ff()._bBorderStyles[i] = fmBorderStyleSunken;
    }

    pCFI->UnprepareForDebug();

    hr = THR(super::ApplyDefaultFormat(pCFI));
    if(hr)
        goto Cleanup;

    pCFI->PrepareParaFormat();
    pCFI->PrepareCharFormat();

    pCFI->_pf()._fPreInner = TRUE;

    if (type == htmlInputHidden && !IsEditable(TRUE))
    {
        pCFI->_cf()._fDisplayNone = TRUE;
    }

    // font height in CharFormat is already nonscaling size in twips
    pCFI->_cf().SetHeightInNonscalingTwips( pCFI->_pcf->_yHeight );

    if (type == htmlInputPassword)
    {
        pCFI->_cf()._fPassword = TRUE;
    }

    pCFI->UnprepareForDebug();

Cleanup:
    RRETURN(hr);
}

DWORD
CInput::GetBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll)
{
    DWORD   nBorders;
    SIZE    sizeButton;

    switch (GetType())
    {
    case htmlInputButton:
    case htmlInputReset:
    case htmlInputSubmit:
        pborderinfo->abStyles[BORDER_TOP]    =
        pborderinfo->abStyles[BORDER_RIGHT]  =
        pborderinfo->abStyles[BORDER_BOTTOM] =
        pborderinfo->abStyles[BORDER_LEFT]   = BTN_PRESSED(_wBtnStatus)
                                                    ? fmBorderStyleSunken
                                                    : fmBorderStyleRaised;
        nBorders = super::GetBorderInfo( pdci, pborderinfo, fAll );
        pborderinfo->aiWidths[BORDER_TOP]    += 1;  // xyFlat space
        pborderinfo->aiWidths[BORDER_RIGHT]  += 1;  // xyFlat space
        pborderinfo->aiWidths[BORDER_BOTTOM] += 1;  // xyFlat space
        pborderinfo->aiWidths[BORDER_LEFT]   += 1;  // xyFlat space
        break;
    case htmlInputFile:
        nBorders = super::GetBorderInfo( pdci, pborderinfo, fAll );
        if (!_fRealBorderSize)
       {
            BOOL fRightToLeft = GetFirstBranch()->GetParaFormat()->HasRTL(FALSE);
            UINT uBorder = (!fRightToLeft ? BORDER_RIGHT : BORDER_LEFT);

            // We reserve space for the fake button in the extra right border
            sizeButton = DYNCAST(CInputFileLayout, GetCurLayout())->_sizeButton;
            pborderinfo->aiWidths[uBorder] = (pdci ?
                                        pdci->WindowXFromDocPixels(sizeButton.cx)
                                        : sizeButton.cx)
                                        + cxButtonSpacing
                                        + pborderinfo->aiWidths[uBorder];
        }
        nBorders = DISPNODEBORDER_COMPLEX;
        break;
    default:
        nBorders = super::GetBorderInfo( pdci, pborderinfo, fAll );
    }

    return nBorders;
}

void
CInput::Notify(CNotification *pNF)
{
    IStream *       pStream = NULL;
    HRESULT         hr = S_OK;
    CStr            cstrVal;
    htmlInput       type = GetType();
    
    // Defer clearing the other radio buttons in the group. This would
    // prevent the collections from being accessed prematurely in
    // SetChecked() called from super::Init2(). The clearing would
    // happen in AddToCollections().
    if (type == htmlInputRadio 
        && pNF->IsType(NTYPE_ELEMENT_ENTERTREE))
    {
        _fDeferClearGroup = TRUE;
    }

    super::Notify(pNF);
    if (type == htmlInputImage)
    {
        Assert(_pImage);
        _pImage->Notify(pNF);
    }

    switch (pNF->Type())
    {

    case NTYPE_ELEMENT_GOTMNEMONIC:
    {
        if (! Doc()->_fDesignMode && type == htmlInputText )
        {
            hr = THR( select());
        }                 
    }        
    break;

    case NTYPE_ELEMENT_LOSTMNEMONIC:
    {
        if (! Doc()->_fDesignMode && type == htmlInputText )
        {
            Doc()->NotifySelection( SELECT_NOTIFY_DESTROY_ALL_SELECTION, NULL );
        }                 
    }        
    break;
    
    case NTYPE_ELEMENT_QUERYTABBABLE:
        if (GetType() == htmlInputRadio)
        {
            CElement * pElemCurrent = Doc()->_pElemCurrent;

            //
            // this is the current element or this is a radio of different group
            // AND this radio is checked or there is no checked radio
            //
            ((CQueryFocus *)pNF->DataAsPtr())->_fRetVal = 
                                            IsEditable(TRUE)
                                            ||
                                            ((      this == pElemCurrent 
                                                ||  !FInSameGroup(pElemCurrent))
                                            && 
                                             (      _fChecked
                                                ||  ChkRadioGroup(GetAAname()) == S_FALSE)
                                            );
        }
        break;

    case NTYPE_ELEMENT_SETFOCUS:
        switch (type)
        {
        case    htmlInputRadio:
        case    htmlInputCheckbox:
            _fLastValue = _fChecked;
            // fall through
        case    htmlInputButton:
        case    htmlInputReset:
        case    htmlInputSubmit:
            _wBtnStatus = BTN_SETSTATUS(_wBtnStatus, FLAG_HASFOCUS);
            break;
        case    htmlInputFile:
            {
                CSetFocus * pSetFocus       = (CSetFocus *)pNF->DataAsPtr();
                CMessage *  pMessage        = pSetFocus->_pMessage;
                unsigned    fButtonHadFocus = _fButtonHasFocus;

                if (pMessage && pMessage->message == WM_LBUTTONDOWN)
                {
                    CRect rc;
                    DYNCAST(CInputFileLayout, GetCurLayout())->GetButtonRect(&rc);
                    _fButtonHasFocus = rc.Contains(pMessage->ptContent);
                }
                //  if the button helper is requesting or we come in with a BackTab
                if ( _fBtnHelperRequestsCurrency ||
                     pMessage &&
                     pMessage->message == WM_KEYDOWN &&
                     pMessage->wParam == VK_TAB &&
                     pMessage->dwKeyState & FSHIFT )
                {
                    BtnHelperSetFocus();
                }
                else if ( fButtonHadFocus && !_fButtonHasFocus)
                {
                    BtnHelperKillFocus();
                }
            }
            break;
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        switch (type)
        {
        case htmlInputImage :
        case htmlInputSubmit :
            if( pNF->DataAsDWORD() & EXITTREE_DESTROY )
            {
                Doc()->_pElemDefault = NULL;
            }
            else
            {
                SetDefaultElem(TRUE);
            }
            break;
        case htmlInputFile :
            if (_icfButton != -1)
            {
                TLS(_pCharFormatCache)->ReleaseData( _icfButton ); _icfButton = -1;
            }
            break;
        case htmlInputRadio :
            if (_fChecked && !(pNF->DataAsDWORD() & EXITTREE_DESTROY))
            {
                DelRadioGroup(GetAAname());
            }
            break;
        }
        break;

    case NTYPE_SAVE_HISTORY_1:
        switch (type)
        {
        case htmlInputText :
        case htmlInputHidden :
        case htmlInputFile :
        case htmlInputCheckbox :
        case htmlInputRadio :
            pNF->SetSecondChanceRequested();
        }

        break;

    case NTYPE_SAVE_HISTORY_2:
        Assert(    type == htmlInputText || type == htmlInputHidden || type == htmlInputFile
                || type == htmlInputCheckbox || type == htmlInputRadio );
        {
            CDataStream         ds;
            CHistorySaveCtx *   phsc;
            DWORD               dwHistoryIndex =    0x80000000
                                                |   (DWORD)_iHistoryIndex
                                                &   0x0FFFF;

            pNF->Data((void **)&phsc);
            hr = THR(phsc->BeginSaveStream(dwHistoryIndex, HistoryCode(), &pStream));
            if (hr)
                goto Cleanup;

            ds.Init(pStream);

            if (!IsOptionButton())
            {
                Assert (IsTextOrFile());

                hr = THR(ds.SaveDword(_fChangingEncoding ? 1 : 0));
                if (hr)
                    goto Cleanup;
                if (!_fChangingEncoding)
                {
                    // save value
                    hr = THR(GetValueHelper(&cstrVal));
                    if (hr)
                        goto Cleanup;
                    hr = THR(ds.SaveCStr(&cstrVal));
                    if (hr)
                        goto Cleanup;
                    // save _fTextChanged
                    hr = THR(ds.SaveDword(_fTextChanged ? 1 : 0));
                    if (hr)
                        goto Cleanup;
                }
            }
            else
            {
                hr = THR(ds.SaveDword((DWORD)_fChecked));
                if (hr)
                    goto Cleanup;
            }

            hr = THR(phsc->EndSaveStream());
            if (hr)
                goto Cleanup;
        }
        break;

    case NTYPE_ELEMENT_ENTERTREE:
        EnterTree();
        break;

    case NTYPE_BASE_URL_CHANGE:
        // send onpropertychange calls for all properties that are
        // effected by this change
        OnPropertyChange( DISPID_CInput_src, ((PROPERTYDESC *)&s_propdescCInputsrc)->GetdwFlags() );
        OnPropertyChange( DISPID_CInput_dynsrc, ((PROPERTYDESC *)&s_propdescCInputdynsrc)->GetdwFlags() );
        OnPropertyChange( DISPID_CInput_lowsrc, ((PROPERTYDESC *)&s_propdescCInputlowsrc)->GetdwFlags() );
        break;        

    case NTYPE_SET_CODEPAGE:
        if (IsTextOrFile())
        {
            _fChangingEncoding = TRUE;
        }
        break;
    }

Cleanup:
    ReleaseInterface(pStream);
    return;
}


#ifndef NO_DATABINDING
class CDBindMethodsRadio : public CDBindMethodsSimple
{
    typedef CDBindMethodsSimple super;
    
public:
    CDBindMethodsRadio() : super(VT_BSTR) {}
    ~CDBindMethodsRadio()   {}

    virtual HRESULT BoundValueToElement(CElement *pElem, LONG id,
                    BOOL fHTML, LPVOID pvData) const;
                    
    virtual HRESULT BoundValueFromElement(CElement *pElem, LONG id,
                     BOOL fHTML, LPVOID pvData) const;

};

class CDBindMethodsInputTxtBase : public CDBindMethodsText
{
    typedef CDBindMethodsText super;
public:
    CDBindMethodsInputTxtBase() : super(0)   {}
    ~CDBindMethodsInputTxtBase()    {}

    virtual HRESULT BoundValueToElement(CElement *pElem, LONG id,
                                        BOOL fHTML, LPVOID pvData) const;
    virtual HRESULT BoundValueFromElement(CElement *pElem, LONG id,
                                         BOOL fHTML, LPVOID pvData) const;
};

class CDBindMethodsCheckbox : public CDBindMethodsSimple
{
    typedef CDBindMethodsSimple super;

public:
    CDBindMethodsCheckbox() : super(VT_BOOL) {}
    ~CDBindMethodsCheckbox()    {}

    virtual HRESULT BoundValueToElement(CElement *pElem, LONG id,
                                        BOOL fHTML, LPVOID pvData) const;

    virtual HRESULT BoundValueFromElement(CElement *pElem, LONG id,
                                          BOOL fHTML, LPVOID pvData) const;

};


//+----------------------------------------------------------------------------
//
//  Function: BoundValueToElement, CDBindMethods
//
//  Synopsis: Transfer data into bound checkbox.  Only called if DBindKind
//            allows databinding.
//
//  Arguments:
//            [id]      - ID of binding point.  For the checkbox, is always
//                        DISPID_VALUE.
//            [pvData]  - pointer to data to transfer, in this case a boolean.
//
//-----------------------------------------------------------------------------
HRESULT
CDBindMethodsCheckbox::BoundValueToElement(CElement *pElem,
                                           LONG,
                                           BOOL,
                                           LPVOID pvData) const
{
    CInput *pCheckbox = DYNCAST(CInput, pElem);

    HRESULT hr = THR(pCheckbox->SetChecked(*(VARIANT_BOOL *)pvData,
                                  FALSE /* don't SaveData */));
    if (!hr)
        Verify(!pElem->OnPropertyChange(DISPID_CInput_checked, 0));
    RRETURN(hr);
}

HRESULT
CDBindMethodsCheckbox::BoundValueFromElement(CElement *pElem,
                                             LONG,
                                             BOOL,
                                             LPVOID pvData) const
{
    CInput *pCheckbox = DYNCAST(CInput, pElem);

    RRETURN(pCheckbox->GetChecked((VARIANT_BOOL *)pvData));
}

static const CDBindMethodsInputTxtBase DBindMethodsInputTxtBase;
static const CDBindMethodsCheckbox DBindMethodsCheckbox;
static const CDBindMethodsRadio DBindMethodsRadio;

const CDBindMethods *
CInput::GetDBindMethods()
{
    htmlInput type = GetType();

    switch(type)
    {
    case htmlInputPassword:
    case htmlInputHidden:
    case htmlInputText:
    case htmlInputFile:
    case htmlInputReset:
    case htmlInputSubmit:
    case htmlInputButton:
        return (CDBindMethods *)&DBindMethodsInputTxtBase;
    case htmlInputCheckbox:
        return (CDBindMethods *)&DBindMethodsCheckbox;
    case htmlInputRadio:
        return (CDBindMethods *)&DBindMethodsRadio; 
    default:
        return NULL;
    }
}


//+----------------------------------------------------------------------------
//
//  Function: BoundValueToElement, CDBindMethods
//
//  Synopsis: Transfer data into bound radio button group.  Only called if 
//            DBindKind allows databinding.
//
//  Arguments:
//            [id]      - ID of binding point.  For the checkbox, is always
//                        DISPID_VALUE.
//            [pvData]  - pointer to data to transfer, in this case a bstr
//
//-----------------------------------------------------------------------------
HRESULT
CDBindMethodsRadio::BoundValueToElement(CElement *pElem,
                    LONG,
                    BOOL,
                    LPVOID pvData) const
{
    // Search for a radio button with value equal to pcData
    HRESULT hr;
    CInput *pRadio = DYNCAST(CInput, pElem);
    LPCTSTR lpName=pRadio->GetAAname();         // NAME= field from INPUT tag

    if (lpName)
    {
        // Clear the currently set radiobutton.
        IGNORE_HR(pRadio->TraverseGroup( 
                    lpName, 
                    VISIT_METHOD(CInput, Clear, clear),
                    0,
                    TRUE));

        // check the button whose value appears in the data
        hr = THR(pRadio->TraverseGroup( lpName,
                    VISIT_METHOD(CInput, SetByValue, setbyvalue),
                    (DWORD_PTR) pvData,
                    TRUE));
    }
    else
    {
        // the user forgot the NAME field, bind to just this button.
        hr = pRadio->SetByValue((DWORD_PTR)pvData);
        
        // If there's no NAME field, then this is just a single button, so
        // if binding doesn't match, we must shut the button off ourselves.
        // Can't depend on mutual exclusion to do it.
        if (hr == S_FALSE)
        {
            IGNORE_HR(pRadio->SetChecked(VB_FALSE, FALSE /* don't SaveData */));
            Verify(!pElem->OnPropertyChange(DISPID_CInput_checked, 0));
        }
    }

    // failure to find a matching NAME is not a failure
    if (hr == S_FALSE)
        hr = S_OK;
    
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Function: BoundValueFromElement, CDBindMethods
//
//  Synopsis: Transfer data from bound radio button group.  Only called if 
//            DBindKind allows databinding.
//
//  Arguments:
//            [id]      - ID of binding point.  For the checkbox, is always
//                        DISPID_VALUE.
//            [pvData]  - pointer to data to transfer, in this case a bstr
//
//  Returns:  S_OK      no problems
//            S_FALSE   No radio button was checked!
//                      BEWARE! This is a very possible state for a bound
//                      radio button to be in, if the value in the database
//                      did not match any of the Value fields of radio buttons.
//
//-----------------------------------------------------------------------------
HRESULT
CDBindMethodsRadio::BoundValueFromElement(CElement *pElem,
                      LONG,
                      BOOL,
                      LPVOID pvData) const
{
    // Get the value form the radio button in this group that is set.
    HRESULT hr;
    CInput *pRadio = DYNCAST(CInput, pElem);
    LPCTSTR lpName=pRadio->GetAAname();         // NAME= field from INPUT tag
    * (BSTR *) pvData = NULL;

    if (lpName)
    {
        hr = THR(pRadio->TraverseGroup( lpName,
                    VISIT_METHOD(CInput, GetValue, getvalue),
                    (DWORD_PTR) pvData,
                    TRUE));
    }
    else
    {
        // the user forgot the NAME field, look at just this button
        hr = pRadio->GetValue((DWORD_PTR)pvData);
    }

    RRETURN1(hr, S_FALSE);
}


HRESULT
CDBindMethodsInputTxtBase::BoundValueFromElement(CElement * pElem,
                                                 LONG       id,
                                                 BOOL       fHTML,
                                                 LPVOID     pvData) const
{
    HRESULT     hr;
    CStr        cstr;
    BSTR *      pBstr   = (BSTR *) pvData;

    Assert(pBstr);
    // shouldn't be called for one-way bindings
    Assert((_dwTransfer & DBIND_ONEWAY) == 0);

#if DBG==1
    {
        DBINFO dbi;

        Assert(id == ID_DBIND_DEFAULT);
        Assert(DBindKind(pElem, id, &dbi) > DBIND_NONE);
        Assert(dbi._vt == VT_BSTR);
    }
#endif // DBG == 1

    hr = THR(DYNCAST(CInput, pElem)->GetValueHelper(&cstr));
    if (hr)
        goto Cleanup;

    hr = cstr.AllocBSTR(pBstr);

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function: BoundValueToElement, CDBindMethods
//
//  Synopsis: transfer bound data to input text element.  We need to override
//            the CElement implementation to remember the value being
//            transferred as the "last value".  This keeps the onchange event
//            from firing simply because the value is databound.
//
//  Arguments:
//            [id]      - ID of binding point.
//            [fHTML]   - is HTML-formatted data called for?
//            [pvData]  - pointer to data to transfer, in the expected data
//                        type.  For text-based transfers, must be BSTR.
//
//-----------------------------------------------------------------------------

HRESULT
CDBindMethodsInputTxtBase::BoundValueToElement ( CElement *pElem,
                                                 LONG id,
                                                 BOOL fHTML,
                                                 LPVOID pvData ) const
{
    CInput *    pInput = DYNCAST(CInput, pElem);
    BSTR *      pBstr = (BSTR *) pvData;
    HRESULT     hr;


    hr = THR(pInput->SetValueHelper((TCHAR *) *pBstr, FormsStringLen(*pBstr)));
    if (hr)
        goto Cleanup;

    // Remember the value
    // Call GetValueHelper instead of using pvData directly. This is because
    // the control may modify the given value (e.g. CR-LF munging - bug 3763).
    hr = THR(pInput->GetValueHelper(&pInput->_cstrLastValue));

    // We should call pInput->SetValueHelper with fOM set to FALSE
    // in order to fire onpropertychange, since we have to
    // call onpropertychange anyway for the cases where layout is not
    // created or not listening, we prefer do it here.
    Verify(!pElem->OnPropertyChange(DISPID_A_VALUE, 0));

Cleanup:
    RRETURN(hr);
}
#endif // ndef NO_DATABINDING


//+---------------------------------------------------------------------------
//
//  Member:     CInput::createTextRange
//
//----------------------------------------------------------------------------
HRESULT
CInput::createTextRange( IHTMLTxtRange * * ppDisp )
{
    HRESULT         hr = S_OK;
    CAutoRange *    pAutoRange = NULL;
    CMarkup *       pMarkupSlave;

    if (!ppDisp)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!HasSlaveMarkupPtr())
        goto Cleanup;
    
    pMarkupSlave = GetSlaveMarkupPtr();
    pAutoRange = new CAutoRange( pMarkupSlave, pMarkupSlave->FirstElement() );

    if (!pAutoRange)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Set the lookaside entry for this range
    pAutoRange->SetNext( pMarkupSlave->DelTextRangeListPtr() );
    hr = THR( pMarkupSlave->SetTextRangeListPtr( pAutoRange ) );
    if (hr)
        goto Cleanup;

    hr = THR( pAutoRange->Init() );

    if (hr)
        goto Cleanup;

    Assert( SlaveMarkup() && SlaveMarkup()->Root() );

    hr = THR_NOTRACE( pAutoRange->SetTextRangeToElement( pMarkupSlave->FirstElement() ) );

    Assert( hr != S_FALSE );

    if (hr)
        goto Cleanup;

    *ppDisp = pAutoRange;
    pAutoRange->AddRef();

Cleanup:
    if (pAutoRange)
    {
        pAutoRange->Release();
    }

    RRETURN( SetErrorInfo( hr ) );
}


void
CInput::BtnHelperSetFocus(void)
{
    CDoc *  pDoc = Doc();

    Assert (GetType() == htmlInputFile);

    _fButtonHasFocus = TRUE;

    _wBtnStatus = BTN_SETSTATUS(_wBtnStatus, FLAG_HASFOCUS);
    ChangePressedLook();

    pDoc->GetView()->SetFocus(this, 0);

    // Hide the caret
    if (pDoc->_pCaret)
    {
        pDoc->_pCaret->Hide();
    }
}

void
CInput::BtnHelperKillFocus(void)
{
    Assert (GetType() == htmlInputFile);
    BTN_RESETSTATUS(_wBtnStatus);
    ChangePressedLook();
    _fButtonHasFocus = FALSE;
    Doc()->GetView()->SetFocus(this, 0);
}


HRESULT
CInput::DoClickCheckbox(CMessage * pMessage /*=NULL*/, CTreeNode *pNodeContext /*=NULL*/,
                          BOOL fFromLabel /*=FALSE*/)
{
    HRESULT hr          = S_OK;
    BOOL    fOldChecked = !!_fChecked;
    WORD    wOldStatus  = _wBtnStatus;

    if (!IsEnabled() && !IsEditable(TRUE))
        goto Cleanup;

    if(!pNodeContext)
        pNodeContext = GetFirstBranch();

    if(!pNodeContext)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    Assert(pNodeContext && pNodeContext->Element() == this);

    if (!TestLock(ELEMENTLOCK_CLICK))
    {
        CLock     Lock(this, ELEMENTLOCK_CLICK);
        CTreeNode::CLock NodeLock(pNodeContext);

        // Set check before firing onclick (Netscape Compat.)
        IGNORE_HR(ClickAction(pMessage));
        if (Fire_onclick(pNodeContext))
            goto Cleanup;

        // Click event was cancelled, so restore old value (Netscape Compat.)
        if (fOldChecked == !!_fChecked)
        {
            _wBtnStatus = wOldStatus;
            DYNCAST(CCheckboxLayout, GetCurLayout())->Invalidate();
        }
        else
        {
            put_checked(_fChecked ? VB_FALSE : VB_TRUE);
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CInput::Getchecked, Setchecked
//
//  Synopsis:   Functions that are called by the vtable when properties
//              are requested.
//
//--------------------------------------------------------------------------

HRESULT
CInput::put_checked(VARIANT_BOOL checked)
{
    HRESULT hr;

    hr = SetErrorInfo(SetChecked(checked));
    if (FAILED(hr))
        goto Cleanup;

    if (!_fChecked)
    {
        CFormElement    *pForm = GetParentForm();

        if (!(pForm ? pForm->_fInTraverseGroup : Doc()->_fInTraverseGroup))
            DelRadioGroup(GetAAname());
    }

    Verify(!OnPropertyChange(DISPID_CInput_checked, 0));

Cleanup:
    RRETURN(hr);
}

HRESULT
CInput::get_checked(VARIANT_BOOL * pchecked)
{
    if ( !pchecked )
        return ( SetErrorInfo(E_INVALIDARG));

    return(SetErrorInfo(GetChecked(pchecked)));
}

HRESULT
CInput::put_status(VARIANT status)
{
    switch(status.vt)
    {
        case VT_NULL:
        _vStatus.vt = VT_NULL;
        break;
        case VT_BOOL:
        _vStatus.vt = VT_BOOL;
        V_BOOL(&_vStatus) = V_BOOL(&status);
        break;
        default:
        _vStatus.vt = VT_BOOL;
        V_BOOL(&_vStatus) = VB_TRUE;
    }

    Verify(S_OK==OnPropertyChange(DISPID_CInput_status, 0));

    RRETURN(S_OK);
}

HRESULT
CInput::get_status(VARIANT * pStatus)
{
    if (_vStatus.vt==VT_NULL)
    {
        pStatus->vt = VT_NULL;
    }
    else
    {
        pStatus->vt = VT_BOOL;
        V_BOOL(pStatus) = V_BOOL(&_vStatus);
    }
RRETURN(S_OK);
}


// status is an altternative NS name for "checked"
HRESULT
CInput::put_status(VARIANT_BOOL checked)
{
   RRETURN(SetErrorInfo(put_checked(checked)));
}

HRESULT
CInput::get_status(VARIANT_BOOL * pchecked)
{
    if ( !pchecked )
        return ( SetErrorInfo(E_INVALIDARG));
    return(GetChecked(pchecked));
}

//+-------------------------------------------------------------------------
//
//  Method:     CInput::GetChecked, SetChecked
//
//  Synopsis:   Helper properties that are implemented for CBtnHelper
//
//--------------------------------------------------------------------------


HRESULT
CInput::SetChecked(VARIANT_BOOL checked, BOOL fSaveData)
{
    HRESULT hr          = S_OK;
    unsigned fCheckedOrig   = _fChecked;
    BOOL fIsRadio       = (GetType() == htmlInputRadio);
    BOOL fIsInMarkup    = IsInMarkup();

    if (fIsRadio)
    {
        // if there a state change or the setting is to Off...
        //  just call the super. otherwise we have to turn off
        //  any other radiobuttons in our group which are on.
        if (checked == -(VARIANT_BOOL)_fChecked)
            goto Cleanup;

        // if fSaveData is FALSE, we're being called on behalf of BoundValueToElement,
        // and so the current group is already cleared.

        if (checked && fSaveData)
        {
            // if only if multiple default checked radios need traversegroup
            // if (AddRadioGroup(GetAAname()) == S_FALSE && !_fDeferClearGroup)
            if (AddRadioGroup(GetAAname()) == S_FALSE || !_fDeferClearGroup)
            {
                // Clear the currently set radiobutton.
                IGNORE_HR(TraverseGroup( 
                                        GetAAname(), 
                                        VISIT_METHOD(CInput, Clear, clear),
                                        0,
                                        TRUE));
            }
        }
    }

    _fChecked = checked;

#ifndef NO_DATABINDING
    if (fSaveData && fIsInMarkup)   // BUGBUG - krisma - Do we need to check fIsInMarkup here?
    {
        hr = SaveDataIfChanged(ID_DBIND_DEFAULT);
        if (SUCCEEDED(hr))
        {
            hr = S_OK;
        }
        else                    // if we can't save bound value, ignore change
        {
            DBMEMBERS * pdbm = GetDBMembers();
            if (fIsRadio && pdbm)
            {
                // we have to set all the buttons in the group back to their
                // original state (bug 61432).  The simplest way to do this
                // is to just reread the value from the database.
                IGNORE_HR(pdbm->TransferFromSrc(this, ID_DBIND_DEFAULT));
            }
            else
            {
                _fChecked = fCheckedOrig;
            }
        }
    }
#endif

    if (IsEditable(TRUE))
        put_BoolHelper(-(VARIANT_BOOL)_fChecked, (PROPERTYDESC *)&s_propdescCInputdefaultChecked);

    if (_fChecked != fCheckedOrig && fIsInMarkup && IsOptionButton())
    {
        CLayout * pLayout   = DYNCAST(CCheckboxLayout, GetCurLayout());

        if (pLayout)
            pLayout->Invalidate();
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CInput::GetChecked(VARIANT_BOOL * pchecked)
{
    *pchecked = -(VARIANT_BOOL)_fChecked;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Member:      CInput::SaveCheckbox
//
// Synopsis:    saves the checkbox/radiobutton
//

//----------------------------------------------------------------------------
HRESULT CInput::SaveCheckbox(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd )
{
    // in run mode the CHECKED and DEFAULTCHECKED properties are
    // not insync, which is intentional to allow the reset functionallity
    // if we save in run we allways want to save the current state
    // (e.g. for printing) therefore we temporarly set the default checked
    // value and then reset it again after saving...
    HRESULT hr;
    VARIANT_BOOL bCurstate = GetAAdefaultChecked();

    SetAAdefaultChecked(_fChecked);

    hr = super::Save(pStreamWrBuff, fEnd);

    SetAAdefaultChecked(bCurstate);

    RRETURN(hr);

}

//+---------------------------------------------------------------------------
//
// Member:      CInput::HandleCheckboxMessage
//
// Synopsis:    Handle window message
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
CInput::HandleCheckboxMessage(CMessage * pMessage)
{
    HRESULT hr = S_FALSE;
    BOOL    fRunMode = !IsEditable(TRUE);

    if (GetType() == htmlInputRadio)
    {
        BOOL    fAlt = pMessage->dwKeyState & FALT;
        BOOL    fCtrl = pMessage->dwKeyState & FCONTROL;

        if (fRunMode && !fAlt && !fCtrl && pMessage->message == WM_KEYDOWN)
        {
            // try to handle the message right here
            switch(pMessage->wParam)
            {
            case VK_LEFT:
            case VK_UP:
                BTN_RESETSTATUS(_wBtnStatus);
                NavigateToNext(pMessage, FALSE);
                hr = S_OK;
                break;

            case VK_RIGHT:
            case VK_DOWN:
                BTN_RESETSTATUS(_wBtnStatus);
                NavigateToNext(pMessage, TRUE);
                hr = S_OK;
                break;
            }
        }

        if (hr != S_FALSE)
            goto Cleanup;
    }

    if (IsEnabled())
    {
        hr = BtnHandleMessage(pMessage);
        if (hr != S_FALSE)
            goto Cleanup;
    }

    hr = super::HandleMessage(pMessage);

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CInput::RenderGlyph
//
//  Synopsis:   renders the glyph for the button
//
//  Arguments:  [hdc] -- HDC to render into
//              [prc] -- rect to render info
//
//  Returns:    HRESULT
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT
CInput::RenderGlyph(CFormDrawInfo * pDI, LPRECT prc)
{
    //[TRISTATE]
    //OLE_TRISTATE    triValue = BtnValue();
    UINT            dfcs     = 0;

    VARIANT_BOOL    checked;

#ifdef NEVER
//[TRISTATE]
    if (triValue == TRISTATE_TRUE)
        dfcs |= DFCS_CHECKED;
    else if (triValue == TRISTATE_MIXED)
        dfcs |= DFCS_CHECKED | DFCS_BUTTON3STATE;
#endif

    Assert(IsOptionButton());

    GetChecked(&checked);
    if (checked)
        dfcs |= DFCS_CHECKED;

    if(BTN_PRESSED(_wBtnStatus))
        dfcs |= DFCS_PUSHED;

    if (!IsEnabled())
        dfcs |= DFCS_INACTIVE;

    switch (BtnStyle())
    {
    case GLYPHSTYLE_NONE:
        Assert(0);
        break;

    case GLYPHSTYLE_CHECK:
        dfcs |= DFCS_BUTTONCHECK;
        if (BTN_GETSTATUS(_wBtnStatus, FLAG_TRISTATE))
        {
            dfcs |= DFCS_CHECKED | DFCS_BUTTON3STATE;
        }

        break;

    case GLYPHSTYLE_OPTION:
        dfcs |= DFCS_BUTTONRADIO;
        break;
    }

#ifdef WIN16
    GDIRECT r; // short rect.
    CopyRect(&r, prc);
    FormsDrawGlyph(pDI, &r, DFC_BUTTON, dfcs);
#else
    FormsDrawGlyph(pDI, prc, DFC_BUTTON, dfcs);
#endif

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CInput::FInSameGroup
//
//  Synopsis:   Check if the given site another radio button in the same
//              group as this one.
//
//-------------------------------------------------------------------------

BOOL
CInput::FInSameGroup(CElement * pElem)
{
    // Make sure pSite is a radio button
    Assert(pElem);
    if (!(pElem->Tag() == ETAG_INPUT &&
        DYNCAST(CInput, pElem)->GetAAtype() == htmlInputRadio))
    {
        return FALSE;
    }

    // Make sure they are in the same scope
    if (GetParentForm() != pElem->GetParentForm())
        return FALSE;

    LPCTSTR pnameThis = GetAAname();
    LPCTSTR pnameOther = DYNCAST(CInput, pElem)->GetAAname();

    return (pnameThis
        &&  pnameOther
        &&  FormsStringICmp(pnameThis, pnameOther) == 0);
}

//+------------------------------------------------------------------------
//
//  Member:     CInput::ClearGroup
//
//  Synopsis:   Clear the other radio buttons in the same group (if deferred
//              in Init2()).
//
//-------------------------------------------------------------------------
HRESULT
CInput::ClearGroup ( CRadioGroupAry * pRadioGroupArray )
{
    HRESULT             hr          = S_OK;
    LPCTSTR             pchName     = GetAAname();
    CFormElement *      pForm       = GetParentForm();
    RadioGroupEntry *   pEntry;
    int                 i;
    BOOL                fAddEntry;

    Assert(GetType() == htmlInputRadio);

    if (!pchName)
    goto Cleanup;

    Assert( pRadioGroupArray );

    // search for this group
    fAddEntry = TRUE;
    for (i = pRadioGroupArray->Size(), pEntry = *pRadioGroupArray; i > 0; i--, pEntry++)
    {
        if (pEntry->_pParentForm == pForm &&
            0 == FormsStringICmp(pEntry->_pRadio->GetAAname(), pchName))
        {
            if (_fChecked)
            {
                // Clear the previous button
                if (pEntry->_pRadio->_fChecked)
                    pEntry->_pRadio->put_checked(FALSE);

                pEntry->_pRadio = this;
            }
            else
            {
            }
            fAddEntry = FALSE;
                break;
        }
    }

    if (fAddEntry)
    {
        hr = THR(pRadioGroupArray->Grow(pRadioGroupArray->Size() + 1));
        if (hr)
            goto Cleanup;
        pEntry = &pRadioGroupArray->Item(pRadioGroupArray->Size() - 1);
        pEntry->_pRadio = this;
        pEntry->_pParentForm = pForm;
        if (hr)
            goto Cleanup;     
    }
Cleanup:
    RRETURN(hr);            
}


//+--------------------------------------------------------
//
//  Member:     CInput::Clear
//
//  Synopsis:   CCheckbox override, since CRadioElements are mutually exclusive
//
//-------------------------------------------------------------------------

HRESULT 
CInput::Clear(DWORD_PTR dw)
{
    HRESULT hr = S_FALSE;

    if (BaseDesc() == (const CBase::CLASSDESC *) &s_classdescCheckbox)
    {
        put_checked(FALSE);
    }
    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------------------
//
//  Function: GetElementDataBound, CElement
//
//  Synopsis: For the given site, find whichever site handles its databinding.
//            (Note that the returned site might not have any databinding
//            specified at all.)  This method allows one bound radio button
//            to act as a binding proxy for all of the buttons in a group.
//
//  Return:   either site itself, or else some other bound radiobutton in the
//            same group.
//
//-----------------------------------------------------------------------------
CElement *
CInput::GetElementDataBound()
{
    CElement *pElement = this;

    if (GetType() == htmlInputRadio)
    {
        IGNORE_HR(TraverseGroup(
            GetAAname(),
            VISIT_METHOD(CInput, FindBoundRadioElement, findboundradioelement),
            (DWORD_PTR) &pElement,
            TRUE));
    }
    else
    {
        pElement = super::GetElementDataBound();
    }

    return pElement;
}

HRESULT
CInput::FindBoundRadioElement(DWORD_PTR dw)
{
    HRESULT hr = S_FALSE;

#ifndef NO_DATABINDING   
    if (BaseDesc() == (const CBase::CLASSDESC *) &s_classdescCheckbox)
    {
        if (GetDBMembers() != NULL)
        {
            * (CElement **) dw = this;
            hr = S_OK;
        }
    }
#endif // ndef NO_DATABINDING

    RRETURN1(hr, S_FALSE);
}
    

//+--------------------------------------------------------
//
//  Member:     CInput::SetByValue
//
//  Synopsis:   CCheckbox override, sice CRadioElements are mutually exclusive
//
//-------------------------------------------------------------------------

HRESULT 
CInput::SetByValue(DWORD_PTR dw)
{
    HRESULT hr = S_FALSE;

    if (BaseDesc() == (const CBase::CLASSDESC *) &s_classdescCheckbox)
    {
        CStr    cstrValue;
        HRESULT hr1 = GetValueHelper(&cstrValue);

        if (hr1)
        {
            hr = hr1;
            goto Cleanup;
        }
        
        if (FormsStringCmp(*(BSTR *) dw, cstrValue) == 0)
        {
            // (sambent) SetByValue is only called from BoundValueToElement
            // so call SetChecked with the flag that suppresses the call to
            // SaveDataIfChanged.  This avoids redundant fetching from the database.
            hr = THR(SetChecked(VB_TRUE, FALSE /* don't SaveData */));
            Verify(!OnPropertyChange(DISPID_CInput_checked, 0));
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+--------------------------------------------------------
//
//  Member:     CInput::GetValue
//
//  Synopsis:   CCheckbox override, sice CRadioElements are mutually exclusive
//
//-------------------------------------------------------------------------

HRESULT
CInput::GetValue(DWORD_PTR dw)
{
    HRESULT hr = S_FALSE;

    if (BaseDesc() == (const CBase::CLASSDESC *) &s_classdescCheckbox)
    {
        if (_fChecked)
            hr = THR(get_PropertyHelper((BSTR *) dw, (PROPERTYDESC *)&s_propdescCInputvalue));
    }
    RRETURN1(hr, S_FALSE);
}


//+--------------------------------------------------------
//
//  Member:     CInput::NavigateToNext
//
//  Synopsis:   Make the radio element after this one current.
//
//-------------------------------------------------------------------------

void
CInput::NavigateToNext(CMessage * pMessage, BOOL fForward)
{
    CElement *     pElement = NULL;

    TraverseGroup( 
        GetAAname(), 
        VISIT_METHOD(CInput, GetNext, getnext),
        (DWORD_PTR) &pElement,
        fForward);

    if (pElement)
    {
        IGNORE_HR(pElement->BecomeCurrentAndActive());
        IGNORE_HR(pElement->ScrollIntoView());
        IGNORE_HR(pElement->DoClick(pMessage));
    }
}


HRESULT
CInput::GetNext(DWORD_PTR dw)
{
    HRESULT hr = S_FALSE;

    if (BaseDesc() == (const CBase::CLASSDESC *) &s_classdescCheckbox)
    {
        CElement ** ppElement = (CElement **) dw;

        if (*ppElement)
        {
            *ppElement = this;
            hr = S_OK;
        }            
        else if (this == Doc()->_pElemCurrent)
            *ppElement = this;
    }
    RRETURN1(hr, S_FALSE);
}


HRESULT
CInput::GetTabStop(DWORD dw)
{
    HRESULT hr = S_FALSE;

    if (BaseDesc() == (const CBase::CLASSDESC *) &s_classdescCheckbox)
    {
        if (IsTabbable(0))
            return S_OK;
    }
    RRETURN1(hr, S_FALSE);
}

HRESULT CInput::GetHeight(long *pl)
{
    VARIANT v;
    HRESULT hr;

    Assert(GetType() == htmlInputImage);
    hr = THR(s_propdescCInputheight.a.HandleUnitValueProperty(
            HANDLEPROP_VALUE | (PROPTYPE_VARIANT << 16),
            &v,
            this,
            CVOID_CAST(GetAttrArray())));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&v) == VT_I4);
    Assert(pl);

    *pl = V_I4(&v);

Cleanup:
    RRETURN(hr);
}

HRESULT CInput::putHeight(long l)
{
    VARIANT v;

    Assert(GetType() == htmlInputImage);
    if ( l < 0 )
        l = 0;

    V_VT(&v) = VT_I4;
    V_I4(&v) = l;

    RRETURN(s_propdescCInputheight.a.HandleUnitValueProperty(
            HANDLEPROP_SET | HANDLEPROP_AUTOMATION | HANDLEPROP_DONTVALIDATE | (PROPTYPE_VARIANT << 16),
            &v,
            this,
            CVOID_CAST(GetAttrArray())));
}

HRESULT CInput::GetWidth(long *pl)
{
    VARIANT v;
    HRESULT hr;

    Assert(GetType() == htmlInputImage);
    hr = THR(s_propdescCInputwidth.a.HandleUnitValueProperty(
            HANDLEPROP_VALUE | (PROPTYPE_VARIANT << 16),
            &v,
            this,
            CVOID_CAST(GetAttrArray())));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&v) == VT_I4);
    Assert(pl);

    *pl = V_I4(&v);

Cleanup:
    RRETURN(hr);
}

HRESULT CInput::putWidth(long l)
{
    VARIANT v;

    Assert(GetType() == htmlInputImage);
    if ( l < 0 )
        l = 0;

    V_VT(&v) = VT_I4;
    V_I4(&v) = l;

    RRETURN(s_propdescCInputwidth.a.HandleUnitValueProperty(
            HANDLEPROP_SET | HANDLEPROP_DONTVALIDATE | HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16),
            &v,
            this,
            CVOID_CAST(GetAttrArray())));
}


STDMETHODIMP CInput::put_height(long l)
{
    RRETURN(SetErrorInfoPSet(putHeight(l), DISPID_CInput_height));
}

STDMETHODIMP CInput::get_height(long *p)
{
    HRESULT hr = S_OK;

    if ( !p )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ((GetType() == htmlInputImage && _pImage))
        RRETURN (_pImage->get_height(p));

Cleanup:
    RRETURN(SetErrorInfoPGet(hr, DISPID_CInput_height));
}

STDMETHODIMP CInput::put_align(BSTR bstrAlign)
{
    RRETURN(s_propdescCInputalign.b.SetEnumStringProperty(
                bstrAlign,
                this,
                (CVoid *)(void *)(GetAttrArray()) ));
}

STDMETHODIMP CInput::get_align(BSTR *p)
{
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = NULL;

    if (GetType() != htmlInputImage)
        goto Cleanup;

    hr = s_propdescCInputalign.b.GetEnumStringProperty(
                p,
                this,
                (CVoid *)(void *)(GetAttrArray()) );

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP CInput::put_width(long l)
{
    RRETURN(SetErrorInfoPSet(putWidth(l), DISPID_CInput_width));
}

STDMETHODIMP CInput::get_width(long *p)
{
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = NULL;

    if ((GetType() == htmlInputImage && _pImage))
        hr = _pImage->get_width(p);

Cleanup:
    RRETURN(SetErrorInfoPGet(hr, DISPID_CInput_width));
}

STDMETHODIMP
CInput::get_src(BSTR * pstrFullSrc)
{
    HRESULT hr = S_OK;

    if (!pstrFullSrc)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *pstrFullSrc = NULL;

    if ((GetType() == htmlInputImage && _pImage))
    {
        hr = _pImage->get_src(pstrFullSrc);
    }
    else
    {
        hr = s_propdescCInputsrc.b.GetUrlProperty(pstrFullSrc, this, CVOID_CAST(GetAttrArray()));
    }

Cleanup:
    RRETURN(SetErrorInfoPGet(hr, DISPID_CInput_src));
}

//+------------------------------------------------------------------
//
//  member : put_src
//
//  sysnopsis : impementation of the interface src property set
//          since this is a URL property we want the crlf striped out
//
//-------------------------------------------------------------------

STDMETHODIMP
CInput::put_src(BSTR bstrSrc)
{
    RRETURN(SetErrorInfo(s_propdescCInputsrc.b.SetUrlProperty(bstrSrc,
                    this,
                    (CVoid *)(void *)(GetAttrArray()))));
}

/*

HRESULT STDMETHODCALLTYPE CInput::put_ie4vtblslot(BSTR)
{
    Assert(GetType() == htmlInputImage);
    return (E_ACCESSDENIED);
}
*/

//+----------------------------------------------------------------------------
//
// Methods:     get/set_hspace
//
// Synopsis:    hspace for aligned images is 3 pixels by default, so we need
//              a method to identify if a default value is specified.
//
//-----------------------------------------------------------------------------

STDMETHODIMP CInput::put_hspace(long v)
{
    RRETURN(s_propdescCInputhspace.b.SetNumberProperty(v, this, CVOID_CAST(GetAttrArray())));
}

STDMETHODIMP CInput::get_hspace(long * p)
{
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = NULL;

    if ((GetType() == htmlInputImage && _pImage))
    {
        HRESULT hr = s_propdescCInputhspace.b.GetNumberProperty(p, this, CVOID_CAST(GetAttrArray()));

        if(!hr)
            *p = *p == -1 ? 0 : *p;
    }

Cleanup:
    RRETURN(SetErrorInfoPGet(hr, DISPID_CInput_hspace));
}

//+----------------------------------------------------------------------------
//
//  Member : [get_/put_] onload
//
//  synopsis : store in this element's propdesc
//
//+----------------------------------------------------------------------------

HRESULT
CInput:: put_onload(VARIANT v)
{
    HRESULT hr = S_OK;

    if ((GetType() == htmlInputImage && _pImage))
    {
        hr = THR(s_propdescCInputonload .a.HandleCodeProperty(
                HANDLEPROP_SET | HANDLEPROP_AUTOMATION |
                (PROPTYPE_VARIANT << 16),
                &v,
                this,
                CVOID_CAST(GetAttrArray())));
    }

    RRETURN( SetErrorInfo( hr ));
}

HRESULT
CInput:: get_onload(VARIANT *p)
{
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    p->vt = VT_NULL;

    if ((GetType() == htmlInputImage && _pImage))
    {
        hr = THR(s_propdescCInputonload.a.HandleCodeProperty(
                    HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16),
                    p,
                    this,
                    CVOID_CAST(GetAttrArray())));
    }
Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//+----------------------------------------------------------------------------
//
//  Member:     CInput:get_strReadyState
//
//
//+------------------------------------------------------------------------------

HRESULT
CInput::get_readyState(BSTR * p)
{
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = NULL;

    if (GetType() != htmlInputImage)
    {
        goto Cleanup;
    }

    hr=THR(s_enumdeschtmlReadyState.StringFromEnum(_pImage->_readyStateFired, p));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CInput::get_readyState(VARIANT * pVarRes)
{
    HRESULT hr = S_OK;

    if (!pVarRes)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    hr = get_readyState(&V_BSTR(pVarRes));
    if (!hr)
        V_VT(pVarRes) = VT_BSTR;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CInput::get_readyStateValue(long *plRetValue)
{
    HRESULT     hr = S_OK;

    if (!plRetValue)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plRetValue = _pImage->_readyStateFired;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     InvokeExReady
//
//  Synopsis  :this is only here to handle readyState queries, everything
//      else is passed on to the super
//
//+------------------------------------------------------------------------

#ifdef USE_STACK_SPEW
#pragma check_stack(off)
#endif

STDMETHODIMP
CInput::ContextThunk_InvokeExReady(DISPID dispid,
                        LCID lcid,
                        WORD wFlags,
                        DISPPARAMS *pdispparams,
                        VARIANT *pvarResult,
                        EXCEPINFO *pexcepinfo,
                        IServiceProvider *pSrvProvider)
{
    IUnknown * pUnkContext;

    // Magic macro which pulls context out of nowhere (actually eax)
    CONTEXTTHUNK_SETCONTEXT

    HRESULT  hr = S_OK;

    if (GetType() == htmlInputImage)
    {
        Assert(_pImage);

        hr = THR(ValidateInvoke(pdispparams, pvarResult, pexcepinfo, NULL));
        if (hr)
            goto Cleanup;

        hr = THR_NOTRACE(ReadyStateInvoke(
                                            dispid,
                                            wFlags,
                                            _pImage->_readyStateFired,
                                            pvarResult));
    }
    hr = THR_NOTRACE(super::ContextInvokeEx(
                                dispid,
                                lcid,
                                wFlags,
                                pdispparams,
                                pvarResult,
                                pexcepinfo,
                                pSrvProvider,
                                pUnkContext ? pUnkContext : (IUnknown*)this));

Cleanup:
    RRETURN(hr);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(on)
#endif

//+-------------------------------------------------------------------------
//
//  Method:     CInput::ShowTooltip
//
//  Synopsis:   Displays the tooltip for the site.
//
//  Arguments:  [pt]    Mouse position in container window coordinates
//              msg     Message passed to tooltip for Processing
//
//--------------------------------------------------------------------------

HRESULT
CInput::ShowTooltip(CMessage *pmsg, POINT pt)
{
    HRESULT hr = S_FALSE;
    CDoc *pDoc = Doc();

    if (pDoc->_pInPlace == NULL)
        goto Cleanup;

    // check to see if tooltip should display the title property
    //
    hr = THR(super::ShowTooltip(pmsg, pt));
    if (hr == S_OK || GetType() != htmlInputImage)
        goto Cleanup;

    Assert(_pImage);
    hr = _pImage->ShowTooltip(pmsg, pt);

Cleanup:
    RRETURN1 (hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//  Member :    CImgHelper::IsHSpaceDefined
//
//  Synopsis:   if hspace is defined on the image
//
//+---------------------------------------------------------------------------
BOOL
CInput::IsHSpaceDefined() const
{
    DWORD v;
    CAttrArray::FindSimple( *GetAttrArray(), &s_propdescCInputhspace.a, &v);

    return v != -1;
}

void
CInput::Passivate()
{
    if (_pImage)
    {
        _pImage->Passivate();
        delete _pImage;
        _pImage = NULL;
    }
    super::Passivate();
}

HRESULT
CInput::PrivateInit2()
{
    HRESULT     hr = S_OK;
    htmlInput   type = GetType();
    UINT    uiBtnDefault    = IDS_BUTTONCAPTION_SUBMIT;
    TCHAR   pszCaption[128];
    int     c;

    _fEditAtBrowse = FALSE;
    switch (type)
    {
    case htmlInputReset:
        uiBtnDefault = IDS_BUTTONCAPTION_RESET;
        // fall through
    case htmlInputSubmit:
        if (_fHasInitValue || TLS(nUndoState) != UNDO_BASESTATE)
            break;
        c = LoadString(GetResourceHInst(),
                   uiBtnDefault, pszCaption, ARRAY_SIZE(pszCaption));
        if (c)
        {
            hr = THR(SetValueHelper(pszCaption, c));
            if (hr)
                goto Cleanup;
        }
        break;
    case htmlInputFile:
        {
            CInputFileLayout *pLayout;

            //  Load the caption string
            pLayout = DYNCAST(CInputFileLayout, Layout());
            hr = LoadString(GetResourceHInst(),
                            IDS_BUTTONCAPTION_UPLOAD,
                            &pLayout->_cchButtonCaption,
                            &pLayout->_pchButtonCaption);
            if (FAILED(hr))
            {
                pLayout->_pchButtonCaption = s_achUploadCaption;
                pLayout->_cchButtonCaption = ARRAY_SIZE(s_achUploadCaption) - 1;
            }

            //remove initital value from input file
            // set value to a empty string
            // ascii only
            // not OM
            if (TLS(nUndoState) == UNDO_BASESTATE)
                hr = SetValueHelperReal(_T(""), 0, TRUE, FALSE);
        }
        // fall through
    case htmlInputText:
    case htmlInputPassword:
        if (!GetAAreadOnly())
        {
            _fEditAtBrowse = TRUE;
        }
        break;
    }

    // Do we need to create a inner element?
    if (!IsOptionButton() && type != htmlInputImage && !HasSlaveMarkupPtr())
    {
        hr = THR(CreateSlave());
        if (hr)
            goto Cleanup;
    }

    // If we are in the middle of an undo operation, trust that the undo
    // queue has enough information to restore the value.
    if (!_fHasMorphed && _fHasInitValue && TLS(nUndoState) == UNDO_BASESTATE)
    {
        hr = SetValueHelper((TCHAR *) _cstrDefaultValue,
                            _cstrDefaultValue.Length(), FALSE);
        if (hr)
            goto Cleanup;
    }

    _fTextChanged = FALSE;

Cleanup:

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CInput::CreateSlave
//
//  Synopsis:   Creates the slave element for the input
//
//--------------------------------------------------------------------------
HRESULT
CInput::CreateSlave()
{
    CMarkup *       pInputMarkup = NULL;
    CTxtSlave *     pElemSlave = NULL;
    CDoc *          pDoc = Doc();
    HRESULT         hr = S_OK;

    Assert(!HasSlaveMarkupPtr());

    hr = THR( pDoc->CreateElement( ETAG_TXTSLAVE, (CElement * *) & pElemSlave ) );
    if(hr)
        goto Cleanup;

    // Might want to make a call here to tell the inner input it now has a valid
    // _pInput pointer

    hr = THR( pDoc->CreateMarkupWithElement( &pInputMarkup, pElemSlave, this ) );
    if (hr)
        goto Cleanup;

    SetSlaveMarkupPtr(pInputMarkup);
    pInputMarkup = NULL;

    // Transfer ownership of the owns runs bit from element-owner to element-content
    pElemSlave->_fOwnsRuns = _fOwnsRuns;
    _fOwnsRuns = FALSE;
    
Cleanup:

    if( pInputMarkup )
        pInputMarkup->Release();

    if( pElemSlave )
        pElemSlave->Release();

    RRETURN(hr);
}

//HACKHACK 47681 : ignore height/width for inputs/no image
//+-------------------------------------------------------------------------
//
//  Method:     CInput::GetDispID
//
//  Synopsis:   need to ignore height and width
//
//--------------------------------------------------------------------------

STDMETHODIMP
CInput::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT         hr = S_OK;
    STRINGCOMPAREFN pfnCompareString = (grfdex & fdexNameCaseSensitive)
                                                   ? StrCmpC : StrCmpIC;

    if (GetType() != htmlInputImage &&
        (pfnCompareString(s_propdescCInputheight.a.pstrName, bstrName) == 0 ||
        pfnCompareString(s_propdescCInputwidth.a.pstrName, bstrName) == 0))
    {
        // ignore height and width
        if (grfdex & fdexNameEnsure)
        {
            hr = AddExpando(bstrName, pid);
        }
        else
        {
            *pid = DISPID_UNKNOWN;
            hr  = DISP_E_UNKNOWNNAME;
        }
    }
    else
    {
        hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pid));
    }
    
    RRETURN(hr);
}

// end of HACKHACK 47681 -> remmeber removing map to in inputtxt.pdl

//+--------------------------------------------------------------
//
//  Member:     CInput::AddRadioGroup(LPCTSTR lpstrGrpName);
//
//  Synopsis:   Add a new radio group name,
//
//              S_OK:       name added
//              S_FALSE:    the name is already there
//
//---------------------------------------------------------------
HRESULT
CInput::AddRadioGroup(LPCTSTR lpstrGrpName)
{
    CFormElement *pForm = GetParentForm();
    CDoc         *pDoc  = Doc();
    RADIOGRPNAME *pGroupName = pForm ? pForm->_pRadioGrpName : pDoc->_pRadioGrpName;
    RADIOGRPNAME *pNew;
    RADIOGRPNAME *pPrev = 0;
    int          iCompResult = 1;

    if (!lpstrGrpName || !lpstrGrpName[0])
        return S_FALSE;

    while (pGroupName)
    {
        iCompResult = FormsStringICmp(lpstrGrpName, pGroupName->lpstrName);
        if (iCompResult == 0)
            return S_FALSE;

        // the new one should be inserted in the order
        if (iCompResult < 0)
            break;

        pPrev = pGroupName;
        pGroupName = pGroupName->_pNext;
    }

    pNew = new RADIOGRPNAME();
    if (!pNew)
        return E_OUTOFMEMORY;

    pNew->lpstrName =SysAllocString(lpstrGrpName);
    if (!pNew->lpstrName)
    {
        delete pNew;
        return E_OUTOFMEMORY;
    }

    if (pPrev)
    {
        pPrev->_pNext = pNew;
    }
    else
    {
        if (pForm)
        {
            pForm->_pRadioGrpName = pNew;
        }
        else
        {
            pDoc->_pRadioGrpName = pNew;
        }
    }

    pNew->_pNext = pGroupName;

    return S_OK;
}

//+--------------------------------------------------------------
//
//  Member:     CInput::DelRadioGroup(LPCTSTR lpstrGrpName);
//
//  Synopsis:   delete a radio group name,
//
//              S_OK:       name deleted
//              S_FALSE:    name not found
//
//---------------------------------------------------------------
HRESULT
CInput::DelRadioGroup(LPCTSTR lpstrGrpName)
{
    CDoc         *pDoc  = Doc();
    CFormElement *pForm = GetParentForm();
    RADIOGRPNAME *pGroupName = pForm ? pForm->_pRadioGrpName : pDoc->_pRadioGrpName;
    RADIOGRPNAME *pPrev = 0;
    int          iCompResult = 1;

    if (!lpstrGrpName || !lpstrGrpName[0])
        return S_FALSE;

    while (pGroupName)
    {
        iCompResult = FormsStringICmp(lpstrGrpName, pGroupName->lpstrName);
        if (iCompResult == 0)
        {
            if (pPrev)
            {
                pPrev->_pNext = pGroupName->_pNext;
            }
            else
            {
                if (pForm)
                {
                    pForm->_pRadioGrpName = pGroupName->_pNext;
                }
                else
                {
                    pDoc->_pRadioGrpName = pGroupName->_pNext;
                }
            }
            SysFreeString((BSTR)pGroupName->lpstrName);
            delete pGroupName;
            return S_OK;
        }

        if (iCompResult < 0)
            return S_FALSE;

        pPrev = pGroupName;
        pGroupName = pGroupName->_pNext;
    }

    return S_FALSE;
}


//+--------------------------------------------------------------
//
//  Member:     CInput::ChkRadioGroup(LPCTSTR lpstrGrpName);
//
//  Synopsis:   Check to see if the radio group is already there
//
//              S_OK:       yes
//              S_FALSE:    no
//
//---------------------------------------------------------------
HRESULT
CInput::ChkRadioGroup(LPCTSTR lpstrGrpName)
{
    CFormElement *pForm = GetParentForm();
    RADIOGRPNAME *pGroupName = pForm ? pForm->_pRadioGrpName : Doc()->_pRadioGrpName;
    int          iCompResult = 1;

    if (!lpstrGrpName || !lpstrGrpName[0])
        return S_OK;

    while (pGroupName)
    {
        iCompResult = FormsStringICmp(lpstrGrpName, pGroupName->lpstrName);
        if (iCompResult == 0)
            return S_OK;

        if (iCompResult < 0)
            return S_FALSE;
        pGroupName = pGroupName->_pNext;
    }

    return S_FALSE;
}


HRESULT
CInput::TraverseGroup(LPCTSTR strGroupName, PFN_VISIT pfn, DWORD_PTR dw, BOOL fForward)
{
    HRESULT hr;

    // get the form this lives on
    //
    CFormElement * pForm = GetParentForm();

    if (pForm)
    {
        hr = THR(pForm->FormTraverseGroup(GetAAname(), pfn, dw, fForward));
    }
    else
    {
        // Let the document handle it
        //
        hr = THR(Doc()->DocTraverseGroup(GetAAname(), pfn, dw, fForward));
    }
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CInput::SetIndeterminateHelper, GetIndeterminateHelper
//
//  Synopsis:   Helper function to get/set Indeterminate
//
//--------------------------------------------------------------------------

HRESULT
CInput::SetIndeterminateHelper(long indeterminate)
{
    _wBtnStatus = (indeterminate==VB_TRUE)?
                    BTN_SETSTATUS(_wBtnStatus, FLAG_TRISTATE) :
                    BTN_RESSTATUS(_wBtnStatus, FLAG_TRISTATE);

    if ((GetType() == htmlInputCheckbox) && !TLS(fInInitAttrBag))
    {
        DYNCAST(CCheckboxLayout, GetCurLayout())->Invalidate();
    }

    return S_OK;
}

HRESULT
CInput::GetIndeterminateHelper(long * pindeterminate)
{
    *pindeterminate = BTN_GETSTATUS(_wBtnStatus, FLAG_TRISTATE)?VB_TRUE:VB_FALSE;

    return S_OK;
}
