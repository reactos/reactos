//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       htmctx.cxx
//
//  Contents:   CHtmParseCtx derivatives for parsing ROOT, HEAD, etc
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

#ifndef X_COMMENT_HXX_
#define X_COMMENT_HXX_
#include "comment.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_TEXTAREA_HXX_
#define X_TEXTAREA_HXX_
#include "textarea.hxx"
#endif

#ifndef X_TABLE_HXX_
#define X_TABLE_HXX_
#include "table.hxx"
#endif

#ifndef X_ESCRIPT_HXX_
#define X_ESCRIPT_HXX_
#include "escript.hxx"
#endif

#ifndef X_ESTYLE_HXX_
#define X_ESTYLE_HXX_
#include "estyle.hxx"
#endif

#ifndef X_ESELECT_HXX_
#define X_ESELECT_HXX_
#include "eselect.hxx"
#endif

#ifndef X_EOPTION_HXX_
#define X_EOPTION_HXX_
#include "eoption.hxx"
#endif

#ifndef X_PROPBAG_HXX_
#define X_PROPBAG_HXX_
#include "propbag.hxx"
#endif

#ifndef X_EOBJECT_HXX_
#define X_EOBJECT_HXX_
#include "eobject.hxx"
#endif

#ifndef X_GENERIC_HXX_
#define X_GENERIC_HXX_
#include "generic.hxx"
#endif

#ifndef X_ENOSHOW_HXX_
#define X_ENOSHOW_HXX_
#include "enoshow.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_BUFFER_HXX_
#define X_BUFFER_HXX_
#include "buffer.hxx"
#endif

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif

ExternTag(tagParse);

MtDefine(CHtmParseCtx, Dwn, "CHtmParseCtx")
MtDefine(CHtmParseCtx_apNodeStack_pv, Dwn, "CHtmParseCtx::EndElement apNodeStack::_pv")
MtDefine(CHtmSelectHackBaseCtx_apNodeStack_pv, Dwn, "CHtmSelectHackBaseCtx::EndElement apNodeStack::_pv")
MtDefine(CHtmOutsideParseCtx, Dwn, "CHtmOutsideParseCtx")
MtDefine(CHtmCommentParseCtx, Dwn, "CHtmCommentParseCtx")
MtDefine(CHtmNoShowParseCtx, Dwn, "CHtmNoShowParseCtx")
MtDefine(CHtmTitleParseCtx, Dwn, "CHtmTitleParseCtx")
MtDefine(CHtmTextareaParseCtx, Dwn, "CHtmTextareaParseCtx")
MtDefine(CHtmStyleParseCtx, Dwn, "CHtmStyleParseCtx")
MtDefine(CHtmScriptParseCtx, Dwn, "CHtmScriptParseCtx")
MtDefine(CHtmGenericParseCtx, Dwn, "CHtmGenericParseCtx")
MtDefine(CHtmFramesetParseCtx, Dwn, "CHtmFramesetParseCtx")
MtDefine(CHtmSelectParseCtx, Dwn, "CHtmSelectParseCtx")
MtDefine(CHtmIframeParseCtx, Dwn, "CHtmIframeParseCtx")
MtDefine(CHtmFrameParseCtx, Dwn, "CHtmFrameParseCtx")
MtDefine(CHtmObjectParseCtx, Dwn, "CHtmObjectParseCtx")
MtDefine(CHtmObjectParseCtxAddTag, Dwn, "CObjectElement::PARAMBINDING strings")

ELEMENT_TAG s_atagEmpty[] = {ETAG_NULL};

//+------------------------------------------------------------------------
//
//  CHtmParseCtx::AddSource
//
//  The contract for AddSource: we must extract the specified number of
//  chars from the source stream.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParseCtx::AddSource(CHtmTag *pht)
{
    HRESULT hr;
    
    hr = THR(pht->GetHtmTagStm()->SkipSource(pht->GetSourceCch()));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  CHtmCommentParseCtx
//
//  The top-level context for the COMMENT element
//
//-------------------------------------------------------------------------

class CHtmCommentParseCtx : public CHtmParseCtx
{
public:
    typedef CHtmParseCtx super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmCommentParseCtx))
    CHtmCommentParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop);
    virtual ~CHtmCommentParseCtx();

    virtual HRESULT AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii);
    virtual HRESULT Init();

#if DBG == 1
    virtual HRESULT BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty) { Assert(0); return super::BeginElement(ppNodeNew, pel, pNodeCur, fEmpty); }
    virtual HRESULT EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd) { Assert(0); return super::EndElement(ppNodeNew, pNodeCur, pNodeEnd); }
#endif

private:

    CCommentElement *_pelComment;
};

//+------------------------------------------------------------------------
//
//  Function:   CreateHtmCommentParseCtx
//
//  Synopsis:   Factory
//
//-------------------------------------------------------------------------
HRESULT CreateHtmCommentParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent)
{
    CHtmParseCtx *phpx = new CHtmCommentParseCtx(phpxParent, pel);
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmCommentParseCtx::ctor
//
//  Synopsis:   first-phase construction
//
//-------------------------------------------------------------------------
CHtmCommentParseCtx::CHtmCommentParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop)
    : CHtmParseCtx(phpxParent)
{
    Assert(pelTop->Tag() == ETAG_COMMENT);

    _pelComment = DYNCAST(CCommentElement, pelTop);
    _pelComment->AddRef();
    _atagReject = s_atagEmpty;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmCommentParseCtx::dtor
//
//  Synopsis:   last-phase destruction
//
//-------------------------------------------------------------------------
CHtmCommentParseCtx::~CHtmCommentParseCtx()
{
    Assert(_pelComment);
    _pelComment->Release();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmCommentParseCtx::Init
//
//  Synopsis:   Gets Comment element ready
//
//-------------------------------------------------------------------------
HRESULT
CHtmCommentParseCtx::Init()
{
    RRETURN(_pelComment->_cstrText.Set(_T("")));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmCommentParseCtx::AddText
//
//  Synopsis:   store Comment text
//
//-------------------------------------------------------------------------
HRESULT
CHtmCommentParseCtx::AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii)
{
    HRESULT hr;

    Assert(pNode->Element() == _pelComment);
    Assert(cch && *pch);

    hr = THR(_pelComment->_cstrText.Append(pch, cch));

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  CHtmNoShowParseCtx
//
//  The top-level context for the HEAD element
//
//-------------------------------------------------------------------------

class CHtmNoShowParseCtx : public CHtmParseCtx
{
public:
    typedef CHtmParseCtx super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmNoShowParseCtx))
    CHtmNoShowParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop);
    ~CHtmNoShowParseCtx();
    
    virtual HRESULT AddSource(CHtmTag *pht);
    virtual HRESULT Finish();
    
private:
    ELEMENT_TAG _atagAccept2[2];
    CBuffer2 _cbuf2Contents;
    CNoShowElement *_pelNoshow;
};

//+------------------------------------------------------------------------
//
//  Function:   CreateHtmlNoShowCtx
//
//  Synopsis:   Factory
//
//-------------------------------------------------------------------------
HRESULT CreateHtmNoShowParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent)
{
    CHtmParseCtx *phpx = new CHtmNoShowParseCtx(phpxParent, pel);
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmNoShowParseCtx::ctor
//
//  Synopsis:   first-phase construction
//
//-------------------------------------------------------------------------
CHtmNoShowParseCtx::CHtmNoShowParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop)
    : CHtmParseCtx(phpxParent)
{
    Assert(pelTop->Tag() == ETAG_NOFRAMES ||
           pelTop->Tag() == ETAG_NOSCRIPT ||
           pelTop->Tag() == ETAG_NOEMBED);
    _atagAccept2[0] = pelTop->Tag();
    _atagAccept2[1] = ETAG_NULL;
    _atagAccept = _atagAccept2;
    _fDropUnknownTags = TRUE;
    
    _pelNoshow = DYNCAST(CNoShowElement, pelTop);
    _pelNoshow->AddRef();
    
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmNoShowParseCtx::dtor
//
//  Synopsis:   destructor
//
//-------------------------------------------------------------------------
CHtmNoShowParseCtx::~CHtmNoShowParseCtx()
{
    Assert(_pelNoshow);
    _pelNoshow->Release();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmNoShowParseCtx::AddSource
//
//  Synopsis:   To keep track of stuff between <NOFRAMES> and </NOFRAMES>
//
//-------------------------------------------------------------------------
HRESULT
CHtmNoShowParseCtx::AddSource(CHtmTag *pht)
{
    HRESULT hr;
    
    hr = THR(pht->GetHtmTagStm()->ReadSource(&_cbuf2Contents, pht->GetSourceCch()));
    
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmNoShowParseCtx::Finish
//
//  Synopsis:   Transfer contents to element
//
//-------------------------------------------------------------------------
HRESULT
CHtmNoShowParseCtx::Finish()
{
    HRESULT hr = S_OK;
    
    _pelNoshow->SetContents(&_cbuf2Contents);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  CHtmTitleParseCtx
//
//  The top-level context for the TITLE element
//
//-------------------------------------------------------------------------

class CHtmTitleParseCtx : public CHtmSpaceParseCtx
{
public:
    typedef CHtmSpaceParseCtx super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmTitleParseCtx))
    CHtmTitleParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop);
    ~CHtmTitleParseCtx();

    virtual HRESULT AddWord(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii);
    virtual HRESULT AddSpace(CTreeNode *pNode);
    virtual HRESULT Init();
    virtual HRESULT Execute();
    virtual CElement *GetMergeElement();

#if DBG == 1
    virtual HRESULT BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty) { Assert(0); return super::BeginElement(ppNodeNew, pel, pNodeCur, fEmpty); }
    virtual HRESULT EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd) { Assert(0); return super::EndElement(ppNodeNew, pNodeCur, pNodeEnd); }
#endif

private:

    CMarkup * _pMarkup;

    enum SPACESTATE
    {
        SS_NOSPACE,
        SS_NEEDSPACE,
        SS_DIDSPACE
    };

    SPACESTATE _spacestate;

    CStr _cstrTitle;
};

//+------------------------------------------------------------------------
//
//  Function:   CreateHtmTitleParseCtx
//
//  Synopsis:   Factory
//
//-------------------------------------------------------------------------
HRESULT CreateHtmTitleParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent)
{
    CHtmParseCtx *phpx = new CHtmTitleParseCtx(phpxParent, pel);

    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmTitleParseCtx::ctor
//
//  Synopsis:   first-phase construction
//
//-------------------------------------------------------------------------
CHtmTitleParseCtx::CHtmTitleParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop)
    : CHtmSpaceParseCtx(phpxParent)
{
    Assert(pelTop->Tag() == ETAG_ROOT);

    _pMarkup = pelTop->GetMarkup();

    _pMarkup->AddRef();
    _fNeedExecute = TRUE;
    _atagReject = s_atagEmpty;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmTitleParseCtx::~CHtmTitleParseCtx
//
//  Synopsis:   dtor
//
//-------------------------------------------------------------------------
CHtmTitleParseCtx::~CHtmTitleParseCtx()
{
    _pMarkup->Release();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmTitleParseCtx::Init
//
//  Synopsis:   Gets title element ready
//
//-------------------------------------------------------------------------
HRESULT
CHtmTitleParseCtx::Init()
{
    HRESULT hr;

    hr = THR(_cstrTitle.Set(_T("")));
    if (hr)
        goto Cleanup;

    hr = THR(RFill(FILL_EAT, NULL));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmTitleParseCtx::AddText
//
//  Synopsis:   store title text
//
//-------------------------------------------------------------------------
HRESULT
CHtmTitleParseCtx::AddWord(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii)
{
    HRESULT hr;

    Assert(cch && *pch);

    hr = THR(_cstrTitle.Append(pch, cch));

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmTitleParseCtx::AddSpace
//
//  Synopsis:   store title text
//
//-------------------------------------------------------------------------
HRESULT
CHtmTitleParseCtx::AddSpace(CTreeNode *pNode)
{
    HRESULT hr;

    hr = THR(_cstrTitle.Append(_T(" ")));

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmTitleParseCtx::GetMergeElement
//
//  Synopsis:   Element with which to merge the found tag
//
//-------------------------------------------------------------------------
CElement *
CHtmTitleParseCtx::GetMergeElement()
{
    return _pMarkup->GetTitleElement();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmTitleParseCtx::Finish
//
//  Synopsis:   Cause UI to be updated after TITLE is complete
//
//-------------------------------------------------------------------------
HRESULT
CHtmTitleParseCtx::Execute()
{
    HRESULT hr = S_OK;
    
    CTitleElement * pElementTitle = _pMarkup->GetTitleElement();

    if (pElementTitle && !pElementTitle->GetTitle())
    {
        // BUGBUG: SetTitle will always make the document dirty.
        // This screws up VB.  Will fix this for IE5 RTM (jbeda).
        CDoc * pDoc = pElementTitle->Doc();
        BOOL fDirty = !!pDoc->_lDirtyVersion;

        hr = THR( pElementTitle->SetTitle( _cstrTitle ) );

        if (    !fDirty
            &&  pDoc->_lDirtyVersion)
        {
            pDoc->_lDirtyVersion = 0;
        }

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}

//+------------------------------------------------------------------------
//
//  CHtmStyleParseCtx
//
//  The top-level context for the STYLE element
//
//-------------------------------------------------------------------------

class CHtmStyleParseCtx : public CHtmParseCtx
{
public:
    typedef CHtmParseCtx super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmStyleParseCtx))
    CHtmStyleParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop);
    ~CHtmStyleParseCtx();

    virtual HRESULT AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii);
    virtual HRESULT Init();
    virtual HRESULT Finish();

#if DBG == 1
    virtual HRESULT BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty) { Assert(0); return super::BeginElement(ppNodeNew, pel, pNodeCur, fEmpty); }
    virtual HRESULT EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd) { Assert(0); return super::EndElement(ppNodeNew, pNodeCur, pNodeEnd); }
#endif

private:

    CStyleElement *_pelStyle;
    CBuffer _cbufText;
};

//+------------------------------------------------------------------------
//
//  Function:   CreateHtmStyleCtx
//
//  Synopsis:   Factory
//
//-------------------------------------------------------------------------
HRESULT CreateHtmStyleParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent)
{
    CHtmParseCtx *phpx = new CHtmStyleParseCtx(phpxParent, pel);
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmStyleParseCtx::ctor
//
//  Synopsis:   first-phase construction
//
//-------------------------------------------------------------------------
CHtmStyleParseCtx::CHtmStyleParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop)
    : CHtmParseCtx(phpxParent)
{
    Assert(pelTop->Tag() == ETAG_STYLE);

    _pelStyle = DYNCAST(CStyleElement, pelTop);
    _pelStyle->AddRef();
    _pelStyle->_fParseFinished = FALSE;
    _atagReject = s_atagEmpty;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmStyleParseCtx::~CHtmStyleParseCtx
//
//  Synopsis:   dtor
//
//-------------------------------------------------------------------------
CHtmStyleParseCtx::~CHtmStyleParseCtx()
{
    Assert(_pelStyle);
    _pelStyle->Release();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmStyleParseCtx::Init
//
//  Synopsis:   Gets ourselves ready
//
//-------------------------------------------------------------------------
HRESULT
CHtmStyleParseCtx::Init()
{
    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmStyleParseCtx::AddText
//
//  Synopsis:   store style text
//
//-------------------------------------------------------------------------
HRESULT
CHtmStyleParseCtx::AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii)
{
    Assert(pNode->Element() == _pelStyle);

    _cbufText.Append(pch, cch);

    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmStyleParseCtx::Finish
//
//  Synopsis:   Fill in Style once text is complete
//
//-------------------------------------------------------------------------
HRESULT
CHtmStyleParseCtx::Finish()
{
    HRESULT hr = S_OK;

    _pelStyle->_fParseFinished = TRUE;
    if (_pelStyle->_fEnterTreeCalled)
    {
        hr = THR(_pelStyle->SetText((LPTSTR)_cbufText));
    }
    else
    {
        _pelStyle->_cstrText.Set((LPTSTR)_cbufText);
    }

    _cbufText.Clear();

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  CHtmScriptParseCtx
//
//  The top-level context for the SCRIPT element
//
//-------------------------------------------------------------------------

class CHtmScriptParseCtx : public CHtmParseCtx
{
public:
    typedef CHtmParseCtx super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmScriptParseCtx))
    CHtmScriptParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop);
    virtual ~CHtmScriptParseCtx();

    virtual HRESULT AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii);
    virtual HRESULT Init();
    virtual HRESULT Finish();
    virtual HRESULT Execute();

#if DBG == 1
    virtual HRESULT BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty) { Assert(0); return super::BeginElement(ppNodeNew, pel, pNodeCur, fEmpty); }
    virtual HRESULT EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd) { Assert(0); return super::EndElement(ppNodeNew, pNodeCur, pNodeEnd); }
#endif

private:

    CScriptElement *_pelScript;
    CTreeNode *_pNodeScript;
    CBuffer _cbufText;
};


//+------------------------------------------------------------------------
//
//  Function:   CreateHtmScriptCtx
//
//  Synopsis:   Factory
//
//-------------------------------------------------------------------------
HRESULT CreateHtmScriptParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent)
{
    CHtmParseCtx *phpx = new CHtmScriptParseCtx(phpxParent, pel);
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmScriptParseCtx::ctor
//
//  Synopsis:   first-phase construction
//
//-------------------------------------------------------------------------
CHtmScriptParseCtx::CHtmScriptParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop)
    : CHtmParseCtx(phpxParent)
{
    Assert(pelTop->Tag() == ETAG_SCRIPT);

    _pelScript = DYNCAST(CScriptElement, pelTop);
    _pNodeScript = _pelScript->GetFirstBranch();
    _pNodeScript->NodeAddRef();  // ref needed for execute
    _atagReject = s_atagEmpty;
    _fNeedExecute = TRUE;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmScriptParseCtx::dtor
//
//  Synopsis:   destructor
//
//-------------------------------------------------------------------------
CHtmScriptParseCtx::~CHtmScriptParseCtx()
{
    Assert(_pelScript);
    _pNodeScript->NodeRelease();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmScriptParseCtx::Init
//
//  Synopsis:   Gets ourselves ready
//
//-------------------------------------------------------------------------
HRESULT
CHtmScriptParseCtx::Init()
{
    HRESULT hr;

    hr = THR(_pelScript->_cstrText.Set(_T("")));
    if (hr)
        RRETURN(hr);

    _pelScript->SetParserWillExecute();

    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmScriptParseCtx::AddText
//
//  Synopsis:   store script text
//
//-------------------------------------------------------------------------
HRESULT
CHtmScriptParseCtx::AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii)
{
    Assert(pNode->Element() == _pelScript);

    _cbufText.Append(pch, cch);

    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmScriptParseCtx::Finish
//
//  Synopsis:   Fill in Script once text is complete
//
//-------------------------------------------------------------------------
HRESULT
CHtmScriptParseCtx::Finish()
{
    _pelScript->_cstrText.Set((LPTSTR)_cbufText);

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmScriptParseCtx::Execute
//
//  Synopsis:   Execute inline script
//
//-------------------------------------------------------------------------
HRESULT
CHtmScriptParseCtx::Execute()
{
    Assert(_pelScript);
    
    RRETURN(_pelScript->Execute());
}

//+------------------------------------------------------------------------
//
//  CHtmGenericParseCtx
//
//  The top-level context for the Generic element
//
//-------------------------------------------------------------------------

class CHtmGenericParseCtx : public CHtmParseCtx
{
public:
    typedef CHtmParseCtx super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmGenericParseCtx))
    CHtmGenericParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop);
    virtual ~CHtmGenericParseCtx();

    virtual HRESULT AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii);
    virtual HRESULT Init();
    virtual HRESULT Finish();

private:

    CGenericElement * _pelGeneric;
    CBuffer           _cbufText;
};

//+------------------------------------------------------------------------
//
//  Function:   CreateHtmGenericCtx
//
//-------------------------------------------------------------------------

HRESULT CreateHtmGenericParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent)
{
    CHtmParseCtx *phpx = new CHtmGenericParseCtx(phpxParent, pel);
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmGenericParseCtx::constructor
//
//-------------------------------------------------------------------------

CHtmGenericParseCtx::CHtmGenericParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop)
    : CHtmParseCtx(phpxParent)
{
    Assert(ETAG_GENERIC_LITERAL == pelTop->Tag());

    _pelGeneric = DYNCAST(CGenericElement, pelTop);
    _pelGeneric->AddRef();
    _atagReject = s_atagEmpty;
    _fNeedExecute = TRUE;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmGenericParseCtx::destructor
//
//-------------------------------------------------------------------------

CHtmGenericParseCtx::~CHtmGenericParseCtx()
{
    Assert(_pelGeneric);
    _pelGeneric->Release();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmGenericParseCtx::Init
//
//-------------------------------------------------------------------------

HRESULT
CHtmGenericParseCtx::Init()
{
    HRESULT hr;

    hr = THR(_pelGeneric->_cstrContents.Set(_T("")));
    if (hr)
        RRETURN(hr);

    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmGenericParseCtx::AddText
//
//-------------------------------------------------------------------------
HRESULT
CHtmGenericParseCtx::AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii)
{
    Assert(pNode->Element() == _pelGeneric);

    _cbufText.Append(pch, cch);

    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmGenericParseCtx::Finish
//
//-------------------------------------------------------------------------

HRESULT
CHtmGenericParseCtx::Finish()
{
    _pelGeneric->_cstrContents.Set((LPTSTR)_cbufText);

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  CHtmFramesetParseCtx
//
//  The top-level context for the HEAD element
//
//-------------------------------------------------------------------------

class CHtmFramesetParseCtx : public CHtmParseCtx
{
public:
    typedef CHtmParseCtx super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmFramesetParseCtx))
    CHtmFramesetParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop);
    ~CHtmFramesetParseCtx();

    virtual HRESULT Finish();

private:

    CFrameSetSite *_psiteFrameset;
    BOOL _fTop;
};

//+------------------------------------------------------------------------
//
//  Function:   CreateHtmlFramesetCtx
//
//  Synopsis:   Factory
//
//-------------------------------------------------------------------------
HRESULT CreateHtmFramesetParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent)
{
    CHtmParseCtx *phpx = new CHtmFramesetParseCtx(phpxParent, pel);
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Constant:   s_atagFramesetAccept
//
//  Synopsis:   The set of tags processed normally within a frameset.
//
//              To imitate NS, we ignore _everything_
//              except <FRAMESET> and <FRAME> tags
//              (issue: NS ignores SCRIPT. Should we?)
//
//-------------------------------------------------------------------------
ELEMENT_TAG s_atagFramesetAccept[] = {
    ETAG_EMBED,
    ETAG_FRAME,
    ETAG_FRAMESET,
    ETAG_NOFRAMES,
//    ETAG_NOSCRIPT, // commented to fix bug 58528
//    ETAG_NOEMBED,  // commented to fix bug 58528
//    ETAG_SCRIPT,   // ignore the script tag (NS compat)
    ETAG_UNKNOWN,
//    ETAG_FORM,     // Hidden inputs in the HEAD are now handled in a new way
//    ETAG_INPUT,    //
    ETAG_NULL,
};

ELEMENT_TAG s_atagFramesetIgnoreEnd[] = {
    ETAG_HTML,
    ETAG_NULL,
};


//+------------------------------------------------------------------------
//
//  Member:     CHtmFramesetParseCtx::ctor
//
//  Synopsis:   first-phase construction
//
//-------------------------------------------------------------------------
CHtmFramesetParseCtx::CHtmFramesetParseCtx(CHtmParseCtx *phpxParent, CElement *pelTop)
    : CHtmParseCtx(phpxParent)
{
    Assert(pelTop->Tag() == ETAG_FRAMESET);

    CLayout *pParent = pelTop->GetCurParentLayout();

    // no parent layout means TOP frameset - so ignore the rest of the file after my </FRAMESET>
    
    if (!pParent)
    {
        _fIgnoreSubsequent = TRUE;
    }
    
    _atagIgnoreEnd  = s_atagFramesetIgnoreEnd;
    _atagAccept     = s_atagFramesetAccept;
    _fDropUnknownTags = TRUE;
    
    _psiteFrameset = DYNCAST(CFrameSetSite, pelTop);
    _psiteFrameset->AddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmFramesetParseCtx::~CHtmFramesetParseCtx
//
//  Synopsis:   dtor
//
//-------------------------------------------------------------------------
CHtmFramesetParseCtx::~CHtmFramesetParseCtx()
{
    Assert(_psiteFrameset);
    _psiteFrameset->Release();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmFramesetParseCtx::Finish
//
//  Synopsis:   do the frameset thing...
//
//-------------------------------------------------------------------------
HRESULT
CHtmFramesetParseCtx::Finish()
{
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Frame context
//
//  The parser context for the FRAME element
//
//-------------------------------------------------------------------------

class CHtmFrameParseCtx : public CHtmParseCtx
{
public:
    typedef CHtmParseCtx super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmFrameParseCtx))
    CHtmFrameParseCtx(CHtmParseCtx *phpxParent, CElement *pel);
    ~CHtmFrameParseCtx();
    virtual HRESULT Execute();

#if DBG == 1
    virtual HRESULT BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty) { Assert(0); return super::BeginElement(ppNodeNew, pel, pNodeCur, fEmpty); }
    virtual HRESULT EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd) { Assert(0); return super::EndElement(ppNodeNew, pNodeCur, pNodeEnd); }
#endif
    
    CFrameElement *_psiteFrame;
};

//+------------------------------------------------------------------------
//
//  Constant:   s_atagFrameAccept
//
//  Synopsis:   The set of tags processed normally within a object.
//
//-------------------------------------------------------------------------
ELEMENT_TAG s_atagFrameAccept[] = {
    ETAG_NULL,
};

//+------------------------------------------------------------------------
//
//  Function:   CreateHtmFrameParseCtx
//
//  Synopsis:   Factory
//
//-------------------------------------------------------------------------
HRESULT CreateHtmFrameParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent)
{
    CHtmParseCtx *phpx = new CHtmFrameParseCtx(phpxParent, pel);
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Function:   CHtmFrameParseCtx::CHtmFrameParseCtx
//
//  Synopsis:   ctor
//
//-------------------------------------------------------------------------
CHtmFrameParseCtx::CHtmFrameParseCtx(CHtmParseCtx *phpxParent, CElement *pel)
    : CHtmParseCtx(phpxParent)
{
    _psiteFrame = DYNCAST(CFrameElement, pel);
    _psiteFrame->AddRef();
    _fNeedExecute   = TRUE;
    _fExecuteOnEof  = TRUE;

    _atagAccept     = s_atagFrameAccept;
}

//+------------------------------------------------------------------------
//
//  Function:   CHtmFrameParseCtx::~CHtmFrameParseCtx
//
//  Synopsis:   dtor
//
//-------------------------------------------------------------------------
CHtmFrameParseCtx::~CHtmFrameParseCtx()
{
    Assert(_psiteFrame);
    _psiteFrame->Release();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmFrameParseCtx::Execute
//
//  Synopsis:   instantiate
//
//-------------------------------------------------------------------------
HRESULT
CHtmFrameParseCtx::Execute()
{
    THR(_psiteFrame->CreateObject());

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Select context
//
//  The parser context for the SELECT element
//
//-------------------------------------------------------------------------

class CHtmSelectParseCtx : public CHtmSpaceParseCtx
{
public:
    typedef CHtmParseCtx super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmSelectParseCtx))
    CHtmSelectParseCtx(CHtmParseCtx *phpxParent, CElement *pel);
    ~CHtmSelectParseCtx();        
    virtual HRESULT Init();
    virtual HRESULT AddWord(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii);
    virtual HRESULT AddSpace(CTreeNode *pNode);
    virtual HRESULT BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty);
    virtual HRESULT EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd);

    CSelectElement *_psiteSelect;
    CHtmParseCtx * _pRootCtx;
};

//+------------------------------------------------------------------------
//
//  Constant:   s_atagSelectAccept
//
//  Synopsis:   The set of tags processed normally within a select.
//
//              (issue: NS ignores SCRIPT. Should we?)
//
//-------------------------------------------------------------------------
ELEMENT_TAG s_atagSelectAccept[] = {
    ETAG_OPTION,
    ETAG_SCRIPT,
    ETAG_UNKNOWN,
    ETAG_TC,
    ETAG_TD,
    ETAG_TH,
    ETAG_TR,
    ETAG_TBODY,
    ETAG_THEAD,
    ETAG_TFOOT,
    ETAG_TABLE,
    ETAG_INPUT,
    ETAG_TEXTAREA,

    ETAG_NULL,
};

ELEMENT_TAG s_atagSelectAlwaysEnd[] = { ETAG_SELECT, ETAG_NULL };

//+------------------------------------------------------------------------
//
//  Function:   CreateHtmSelectParseCtx
//
//  Synopsis:   Factory
//
//-------------------------------------------------------------------------
HRESULT CreateHtmSelectParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent)
{
    CHtmParseCtx *phpx = new CHtmSelectParseCtx(phpxParent, pel);
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Function:   CHtmSelectParseCtx::CHtmSelectParseCtx
//
//  Synopsis:   ctor
//
//-------------------------------------------------------------------------
CHtmSelectParseCtx::CHtmSelectParseCtx(CHtmParseCtx *phpxParent, CElement *pel)
    : CHtmSpaceParseCtx(phpxParent)
{
    _psiteSelect = DYNCAST(CSelectElement, pel);
    _psiteSelect->AddRef();

    _atagAccept     = s_atagSelectAccept;
    _atagAlwaysEnd  = s_atagSelectAlwaysEnd;
    _fDropUnknownTags = TRUE;
}

//+------------------------------------------------------------------------
//
//  Function:   CHtmSelectParseCtx::~CHtmSelectParseCtx
//
//  Synopsis:   dtor
//
//-------------------------------------------------------------------------
CHtmSelectParseCtx::~CHtmSelectParseCtx()
{
    Assert(_psiteSelect);

    _psiteSelect->Release();
}


HRESULT
CHtmSelectParseCtx::Init()
{
    _pRootCtx = GetHpxRoot();
    
    RRETURN(_pRootCtx ? S_OK : E_FAIL);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmSelectParseCtx::BeginElement
//
//  Synopsis:   Listen to the beginning of OPTION tags and
//              eat leading whitespace
//
//-------------------------------------------------------------------------
HRESULT
CHtmSelectParseCtx::BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty)
{

    HRESULT hr;
    CTreeNode *pNode = NULL;

    hr = LFill(FILL_PUT);
    if ( hr )
        goto Cleanup;

    hr = THR(super::BeginElement(&pNode, pel, pNodeCur, fEmpty));
    if (hr)
        goto Cleanup;

    *ppNodeNew = pNode;
    pNode = NULL;

    if ( ETAG_OPTION == pel->Tag() )
    {
        hr = RFill(FILL_EAT, NULL);
        if ( hr )
            goto Cleanup;
    }
    else
    {
        hr = RFill(FILL_PUT, *ppNodeNew);
        if ( hr )
            goto Cleanup;
    }


Cleanup:

    CTreeNode::ReleasePtr(pNode);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmSelectParseCtx::EndElement
//
//  Synopsis:   Listen to the end of OPTION tags, sets up the space-eating
//              for symmetry's sake
//
//-------------------------------------------------------------------------
HRESULT
CHtmSelectParseCtx::EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd)
{
    HRESULT hr;


    
    if (ETAG_OPTION == pNodeEnd->Element()->Tag())
    {
        hr = LFill(FILL_EAT);
        if ( hr )
            goto Cleanup;
    }
    else
    {
        hr = LFill(FILL_PUT);
        if ( hr )
            goto Cleanup;
    }

    hr = THR(super::EndElement(ppNodeNew, pNodeCur, pNodeEnd));
    if (hr)
        goto Cleanup;

    hr = RFill(FILL_PUT, *ppNodeNew);
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmSelectParseCtx::AddWord
//
//  Synopsis:   Listen for the space context passing in text,
//              forward it to the root
//
//-------------------------------------------------------------------------
HRESULT
CHtmSelectParseCtx::AddWord(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii)
{
    HRESULT hr;
    
    hr = THR(_pRootCtx->AddText(pNode, pch, cch, fAscii));

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmSelectParseCtx::AddSpace
//
//  Synopsis:   Listen for the space context passing in space,
//              forward a single space to the root
//
//-------------------------------------------------------------------------
HRESULT
CHtmSelectParseCtx::AddSpace(CTreeNode *pNode)
{
    HRESULT hr;
    
    hr = THR(_pRootCtx->AddText(pNode, _T(" "), 1, TRUE));

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Iframe context
//
//  The parser context for the IFRAME element
//
//-------------------------------------------------------------------------

class CHtmIframeParseCtx : public CHtmParseCtx
{
public:
    typedef CHtmParseCtx super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmIframeParseCtx))
    CHtmIframeParseCtx(CHtmParseCtx *phpxParent, CElement *pel);
    ~CHtmIframeParseCtx();
    virtual HRESULT Finish();
    virtual HRESULT Execute();
    virtual HRESULT AddSource(CHtmTag *pht);
    
#if DBG == 1
    virtual HRESULT BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty) { Assert(0); return super::BeginElement(ppNodeNew, pel, pNodeCur, fEmpty); }
    virtual HRESULT EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd) { Assert(0); return super::EndElement(ppNodeNew, pNodeCur, pNodeEnd); }
#endif

    CBuffer2 _cbuf2Contents;
    CIFrameElement *_psiteIframe;
};

//+------------------------------------------------------------------------
//
//  Constant:   s_atagIframeAccept
//
//  Synopsis:   The set of tags processed normally within a object.
//
//-------------------------------------------------------------------------
ELEMENT_TAG s_atagIframeAccept[] = {
    ETAG_IFRAME,
    ETAG_NULL
};

//+------------------------------------------------------------------------
//
//  Function:   CreateHtmIframeParseCtx
//
//  Synopsis:   Factory
//
//-------------------------------------------------------------------------
HRESULT CreateHtmIframeParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent)
{
    CHtmParseCtx *phpx = new CHtmIframeParseCtx(phpxParent, pel);
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Function:   CHtmIframeParseCtx::CHtmIframeParseCtx
//
//  Synopsis:   ctor
//
//-------------------------------------------------------------------------
CHtmIframeParseCtx::CHtmIframeParseCtx(CHtmParseCtx *phpxParent, CElement *pel)
    : CHtmParseCtx(phpxParent)
{
    _psiteIframe = DYNCAST(CIFrameElement, pel);
    _psiteIframe->AddRef();
    _fNeedExecute   = TRUE;
    _fExecuteOnEof  = TRUE;

    _atagAccept     = s_atagIframeAccept;
    _fDropUnknownTags = TRUE;
}

//+------------------------------------------------------------------------
//
//  Function:   CHtmIframeParseCtx::~CHtmIframeParseCtx
//
//  Synopsis:   dtor
//
//-------------------------------------------------------------------------
CHtmIframeParseCtx::~CHtmIframeParseCtx()
{
    Assert(_psiteIframe);
    _psiteIframe->Release();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmIframeParseCtx::AddSource
//
//  Synopsis:   To keep track of stuff between <NOFRAMES> and </NOFRAMES>
//
//-------------------------------------------------------------------------
HRESULT
CHtmIframeParseCtx::AddSource(CHtmTag *pht)
{
    HRESULT hr;
    
    hr = THR(pht->GetHtmTagStm()->ReadSource(&_cbuf2Contents, pht->GetSourceCch()));
    
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmIframeParseCtx::Finish
//
//  Synopsis:   Transfer contents to element
//
//-------------------------------------------------------------------------
HRESULT
CHtmIframeParseCtx::Finish()
{
    HRESULT hr = S_OK;
    
    _psiteIframe->SetContents(&_cbuf2Contents);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmIframeParseCtx::Execute
//
//  Synopsis:   instantiate
//
//-------------------------------------------------------------------------
HRESULT
CHtmIframeParseCtx::Execute()
{
    THR(_psiteIframe->CreateObject());

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Object context
//
//  The parser context for the OBJECT element
//
//-------------------------------------------------------------------------

class CHtmObjectParseCtx : public CHtmParseCtx
{
public:
    typedef CHtmParseCtx super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmObjectParseCtx))
    CHtmObjectParseCtx(CHtmParseCtx *phpxParent, CElement *pel);
    ~CHtmObjectParseCtx();
    virtual HRESULT AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii);
    virtual HRESULT AddTag(CHtmTag *pht);
    virtual HRESULT Init();
    virtual HRESULT Finish();
    virtual HRESULT AddSource(CHtmTag *pht);
    virtual HRESULT Execute();

#if DBG == 1
    virtual HRESULT BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty) { Assert(0); return super::BeginElement(ppNodeNew, pel, pNodeCur, fEmpty); }
    virtual HRESULT EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd) { Assert(0); return super::EndElement(ppNodeNew, pNodeCur, pNodeEnd); }
#endif

    CObjectElement *_psiteObject;
    int _cchLastSource;
    CBuffer2 _cbuf2AltHtml;
};

//+------------------------------------------------------------------------
//
//  Constant:   s_atagObjectAccept
//
//  Synopsis:   The set of tags processed normally within a object.
//
//              (issue: NS ignores SCRIPT. Should we?)
//
//-------------------------------------------------------------------------
ELEMENT_TAG s_atagObjectAccept[] = {
    ETAG_APPLET,
    ETAG_OBJECT,
    ETAG_NULL,
};

ELEMENT_TAG s_atagObjectTag[] = { ETAG_PARAM, ETAG_NULL };

//+------------------------------------------------------------------------
//
//  Function:   CreateHtmObjectParseCtx
//
//  Synopsis:   Factory
//
//-------------------------------------------------------------------------
HRESULT CreateHtmObjectParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent)
{
    CHtmParseCtx *phpx = new CHtmObjectParseCtx(phpxParent, pel);
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Function:   CHtmObjectParseCtx::CHtmObjectParseCtx
//
//  Synopsis:   ctor
//
//-------------------------------------------------------------------------
CHtmObjectParseCtx::CHtmObjectParseCtx(CHtmParseCtx *phpxParent, CElement *pel)
    : CHtmParseCtx(phpxParent)
{
    _psiteObject = DYNCAST(CObjectElement, pel);
    _psiteObject->AddRef();
    _atagAccept     = s_atagObjectAccept;
    _atagTag        = s_atagObjectTag;
    _atagAlwaysEnd  = NULL;
    _fNeedExecute   = TRUE;
    _fExecuteOnEof  = TRUE;
    _fDropUnknownTags = TRUE;
}

//+------------------------------------------------------------------------
//
//  Function:   CHtmObjectParseCtx::~CHtmObjectParseCtx
//
//  Synopsis:   dtor
//
//-------------------------------------------------------------------------
CHtmObjectParseCtx::~CHtmObjectParseCtx()
{
    Assert(_psiteObject);
    _psiteObject->Release();
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmObjectParseCtx::AddTag
//
//  Synopsis:   store Object params
//
//-------------------------------------------------------------------------
HRESULT
CHtmObjectParseCtx::AddTag(CHtmTag *pht)
{
    TCHAR *pchName;
    TCHAR *pchValue;
    TCHAR *pchDataSrc;
    TCHAR *pchDataFld;
    TCHAR *pchDataFormatAs;
    HRESULT hr = S_OK;

    Assert(pht->Is(ETAG_PARAM));

    // Since we don't want Param's in our althtml, blow it away here.
    _cbuf2AltHtml.Chop(_cchLastSource);
    _cchLastSource = 0;

    if (pht->IsEnd())
        return S_OK;

    pht->ValFromName(OBJECTPARAM_NAME, &pchName);
    pht->ValFromName(OBJECTPARAM_VALUE, &pchValue);

    if (pchName && *pchName)
    {
        hr = THR(_psiteObject->EnsureParamBag());
        if (hr)
            RRETURN(hr);

        hr = THR(_psiteObject->_pParamBag->AddProp(pchName, _tcslen(pchName), pchValue, pchValue ? _tcslen(pchValue) : 0));

        // check for param bindings
        pht->ValFromName(OBJECTPARAM_DATASRC, &pchDataSrc);
        pht->ValFromName(OBJECTPARAM_DATAFLD, &pchDataFld);
        pht->ValFromName(OBJECTPARAM_DATAFORMATAS, &pchDataFormatAs);

        if (pchDataSrc || pchDataFld)
        {
            CObjectElement::PARAMBINDING paramBinding;

            hr = MemAllocString(Mt(CHtmObjectParseCtxAddTag), pchName, &paramBinding._strParamName);
            if (hr)
                goto CleanupBinding;

            if (pchDataSrc)
            {
                hr = MemAllocString(Mt(CHtmObjectParseCtxAddTag), pchDataSrc, &paramBinding._strDataSrc);
                if (hr)
                    goto CleanupBinding;
            }

            if (pchDataFld)
            {
                hr = MemAllocString(Mt(CHtmObjectParseCtxAddTag), pchDataFld, &paramBinding._strDataFld);
                if (hr)
                    goto CleanupBinding;
            }

            if (pchDataFormatAs)
            {
                hr = MemAllocString(Mt(CHtmObjectParseCtxAddTag), pchDataFormatAs, &paramBinding._strDataFormatAs);
                if (hr)
                    goto CleanupBinding;
            }
            hr = _psiteObject->_aryParamBinding.AppendIndirect(&paramBinding);

        CleanupBinding:

            if (hr)
            {
                MemFreeString(paramBinding._strDataFormatAs);
                MemFreeString(paramBinding._strDataFld);
                MemFreeString(paramBinding._strDataSrc);
                MemFreeString(paramBinding._strParamName);
            }
        }
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmObjectParseCtx::Execute
//
//  Synopsis:   instantiate object
//
//-------------------------------------------------------------------------
HRESULT
CHtmObjectParseCtx::Execute()
{
    // instantiate object
    THR(_psiteObject->CreateObject());
    
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmObjectParseCtx::Init
//
//  Synopsis:   inits string to store altHtml string
//
//-------------------------------------------------------------------------
HRESULT
CHtmObjectParseCtx::Init()
{
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmObjectParseCtx::Finish
//
//  Synopsis:   saves the altHtml once parsing is complete
//
//-------------------------------------------------------------------------
#define ISSPACE(ch) (((ch) == _T(' ')) || ((unsigned)((ch) - 9)) <= 13 - 9)

HRESULT
CHtmObjectParseCtx::Finish()
{
    HRESULT hr = S_OK;
    CStr cstrAltHtml;
    TCHAR *pch;
    ULONG cch;

    if (_cbuf2AltHtml.Length())
    {
        // Consolidate into one string
        
        hr = THR(_cbuf2AltHtml.SetCStr(&cstrAltHtml));
        if (hr) 
            goto Cleanup;
        
        for (pch = cstrAltHtml, cch = cstrAltHtml.Length(); cch; pch++, cch--)
        {
            // only save alt html if object contains nonspace
            if (!ISSPACE(*pch))
            {
                hr = THR(_psiteObject->SetAAaltHtml(cstrAltHtml));
                break;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmObjectParseCtx::AddText
//
//  Synopsis:   store Object text in the altHtml property
//
//-------------------------------------------------------------------------
HRESULT
CHtmObjectParseCtx::AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii)
{
    _cchLastSource = 0;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmObjectParseCtx::AddSource
//
//  Synopsis:   Stores the original source
//              for the altHtml property
//
//-------------------------------------------------------------------------
HRESULT
CHtmObjectParseCtx::AddSource(CHtmTag *pht)
{
    HRESULT hr = S_OK;
    Assert(pht);

    _cchLastSource = pht->GetSourceCch();
    
    hr = THR(pht->GetHtmTagStm()->ReadSource(&_cbuf2AltHtml, _cchLastSource));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}
