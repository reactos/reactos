//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       escript.hxx
//
//  Contents:   CScriptElement
//
//  History:    09-Jul-1996     AnandRa     Created
//
//----------------------------------------------------------------------------

#ifndef I_ESCRIPT_HXX_
#define I_ESCRIPT_HXX_
#pragma INCMSG("--- Beg 'escript.hxx'")

#define _hxx_
#include "script.hdl"

MtExtern(CScriptElement)

// Forward declarations
class CScriptElement;
class CBitsCtx;
class CDwnChan;

//+---------------------------------------------------------------------------
//
// CScriptElement
//
//----------------------------------------------------------------------------


class CScriptElement : public CElement
{
    DECLARE_CLASS_TYPES(CScriptElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CScriptElement))

    // constructor / destructor
    CScriptElement (CHtmTag *pht, CDoc *pDoc);
    void Passivate();

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    // CElement overrides
    static HRESULT  CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);
            
    HRESULT         Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd);
    
    virtual void    Notify(CNotification *pNF);
    
    // Other helpers
    HRESULT         CommitCode();
    HRESULT         CommitFunctionPointersCode(CBase *pelTarget = NULL, BOOL fHookup = TRUE, BOOL fMarkupDestroy = FALSE);
    HRESULT         Execute();
    HRESULT         DownLoadScript();

    HRESULT         EnsureSourceObjectRevoked();
    HRESULT         EnsureScriptDownloadLeft();

    // For download
    void            SetBitsCtx(CBitsCtx * pBitsCtx);
    void            OnDwnChan(CDwnChan * pDwnChan);
    static void CALLBACK OnDwnChanCallback(void * pvObj, void * pvArg)
        { ((CScriptElement *)pvArg)->OnDwnChan((CDwnChan *)pvObj); }

    // Readystate support
    void SetReadyStateScript(long readyStateScript);
    void OnReadyStateChange();
    void BUGCALL FireOnReadyStateChange(DWORD_PTR dwContext);

    // misc

    NV_DECLARE_TEAROFF_METHOD(get_readyState, get_readyState, (BSTR *pBSTR));
    NV_DECLARE_TEAROFF_METHOD(get_readyState, get_readyState, (VARIANT *pReadyState));
    NV_DECLARE_TEAROFF_METHOD(put_src, put_src, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_src, get_src, (BSTR *p));
    NV_DECLARE_TEAROFF_METHOD(put_htmlFor, put_htmlfor, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_htmlFor, get_htmlfor, (BSTR *p));
    NV_DECLARE_TEAROFF_METHOD(put_event, put_event, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_event, get_event, (BSTR *p));
    NV_DECLARE_TEAROFF_METHOD(put_text, put_text, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_text, get_text, (BSTR *p));
    
    HRESULT STDMETHODCALLTYPE SetPropertyHelper(BSTR v, const PROPERTYDESC *pPropDesc);

    BOOL            ParserWillExecute()
        { return _fParserWillExecute; }

    void            SetParserWillExecute(BOOL fParserWillExecute = TRUE)
        { _fParserWillExecute = fParserWillExecute; }

    #define _CScriptElement_
    #include "script.hdl"

    CStr                    _cstrText;      // The source text of the code

    CBitsCtx *              _pBitsCtx;
    DWORD                   _dwScriptCookie;        // cookie identifying the script block uniquely
    DWORD                   _dwScriptDownloadCookie;
    DWORD                   _dwScriptProgressCookie;
    TCHAR *                 _pchSrcCode;    // Downloaded source

    ULONG                   _ulScriptOffset;
    ULONG                   _ulScriptLine;

    CScriptDebugDocument *  _pScriptDebugDocument;  // created if (1) script debugger is on, and (2) _fSrc is on

    IDispatch *             _pDispCode;
    TCHAR *                 _pchEventName;

    unsigned                _fCodeConstructed : 1;  // TRUE iff attempted to construct and hookup code
                                                    // pointer after either parsing or pasting html
    unsigned                _fSrc             : 1;  // TRUE iff source is downloaded using SRC=*
    unsigned                _readyStateScript : 3;
    unsigned                _readyStateFired  : 3;

    unsigned                _fFirstEnteredTree: 1;  // TRUE after first SN_ENTERTREE is recieved
    unsigned                _fScriptDownloaded: 1;  // TRUE if script is downloaded in put_src
    unsigned                _fDeferredExecution: 1; // TRUE if script has been defered by CDoc::DeferScrpt

    unsigned                _fParserWillExecute: 1; // TRUE if the Parser will execute the script
    unsigned                _fScriptCommitted: 1; // TRUE if function pointers code has already been committed

protected:
    DECLARE_CLASSDESC_MEMBERS;


    NO_COPY(CScriptElement);
};

#pragma INCMSG("--- End 'escript.hxx'")
#else
#pragma INCMSG("*** Dup 'escript.hxx'")
#endif
