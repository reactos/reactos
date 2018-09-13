/*  MARKUP.HXX
 *
 *  Purpose:
 *      Basic data structure for markup storage
 *
 *  Authors:
 *      Joe Beda
 *      (CTxtEdit) Christian Fortini
 *      (CTxtEdit) Murray Sargent
 *
 *  Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */
#ifndef I_MARKUP_HXX_
#define I_MARKUP_HXX_
#pragma INCMSG("--- Beg 'markup.hxx'")

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

#ifndef X_TEXTEDIT_H_
#define X_TEXTEDIT_H_
#include "textedit.h"
#endif

#ifndef X_SELRENSV_HXX_
#define X_SELRENSV_HXX_
#include "selrensv.hxx"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef  __mlang_h__
#include <mlang.h>
#endif

typedef BYTE SCRIPT_ID;

#ifndef X_NOTIFY_HXX_
#define X_NOTIFY_HXX_
#include "notify.hxx"
#endif

#define _hxx_
#include "markup.hdl"

// The initial size of the the CTreePos pool.  This
// has to be at least 1.
#define INITIAL_TREEPOS_POOL_SIZE   1

class CHtmRootParseCtx;
class CMetaElement;
class CTitleElement;
class CHtmlElement;
class CHeadElement;
class CNotification;
class CTable;
class CFlowLayout;
class CMarkupPointer;
class CTreePosGap;
class CHtmlComponent;
class CSpliceRecordList;
class CMarkup;
class CXmlNamespaceTable;
class CScriptDebugDocument;
class CAutoRange;

enum XMLNAMESPACETYPE;

struct HTMLOADINFO;
struct HTMPASTEINFO;

#if DBG==1
    extern CMarkup *    g_pDebugMarkup;
    void   DumpTreeOverwrite(void);
#endif

#ifdef MARKUP_STABLE

enum    UNSTABLE_CODE
{
    UNSTABLE_NOT = 0,           // No violation (means tree/html is stable)
    UNSTABLE_NESTED = 1,        // violation of NESTED containers rule
    UNSTABLE_TEXTSCOPE = 2,     // violation of TEXTSCOPE rule
    UNSTABLE_OVERLAPING = 3,    // violation of OVERLAPING tags rule
    UNSTABLE_MASKING = 4,       // violation of MASKING container rule
    UNSTABLE_PROHIBITED = 5,    // violation of PROHIBITED container rule
    UNSTABLE_REQUIRED = 6,      // violation of REQUIRED container rule
    UNSTABLE_IMPLICITCHILD = 7, // violation of IMPLICITCHILD rule
    UNSTABLE_LITERALTAG = 8,    // violation of LITERALTAG rule
    UNSTABLE_TREE = 9,          // violation of TREE rules
    UNSTABLE_EMPTYTAG = 10,     // vialotion of EMPTY TAG rule
    UNSTABLE_MARKUPPOINTER = 11,// vialotion of the markup pointer rule
    UNSTABLE_CANTDETERMINE = 12,// can not determine if stable or not due to the other probelms (OUT_OF_MEMEORY)
};

#endif

MtExtern(CMarkup)
MtExtern(CMarkupBehaviorContext)
MtExtern(CAryNotify_aryANotification_pv)
MtExtern(CAryNotify_aryDNotification_pv)
MtExtern(CSpliceRecordList)
MtExtern(CSpliceRecordList_pv)
MtExtern(CDocFrag)
MtExtern(CMarkupScriptContext);

MtExtern(CMarkup_aryXmlDeclElements_pv)
MtExtern(CMarkupDirtyRangeContext)
MtExtern(CMarkupDirtyRangeContext_aryMarkupDirtyRange_pv)
MtExtern(CMarkupTextFragContext)
MtExtern(CMarkupTextFragContext_aryMarkupTextFrag_pv)
MtExtern(CMarkup_aryScriptEnqueued)

//---------------------------------------------------------------------------
//
//  Class:   CMarkupScriptContext
//
//---------------------------------------------------------------------------

class CMarkupScriptContext
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupScriptContext))

    CMarkupScriptContext();
    ~CMarkupScriptContext();

    CStr                    _cstrNamespace;             // namespace of scripts in this markup:
                                                        //      for primary markup:         "window"
                                                        //      for non-primary markups:    "ms__idX", X is in {1, 2, ... }

    CScriptMethodsTable     _ScriptMethodsTable;        // table mapping dispids of dispatch items exposed by script
                                                        // engines to dispids we expose on window object.
                                                        // Primary purpose: gluing script engines in single namespace.

    CStr                    _cstrUrl;                   // used to report script within this markup

    ULONG                   _cInlineNesting;            // counts nesting of Enter/Leave inline script
    ULONG                   _cScriptDownloading;        // counts scripts being downloaded
    DWORD                   _dwScriptDownloadingCookie;

    DWORD                   _dwScriptCookie;

    DECLARE_CPtrAry(CAryScriptEnqueued, CScriptElement *, Mt(Mem), Mt(CMarkup_aryScriptEnqueued))
    CAryScriptEnqueued      _aryScriptEnqueued;         // queue of scripts to be run (see wscript.cxx)

    CScriptDebugDocument *  _pScriptDebugDocument;      // script debug document (manages _pDebugHelper)

    LONG                    _idxDefaultScriptHolder;    // index of script holder of default script engine within this markup

    BOOL                    _fWaitScript:1;             // TRUE when the parser is waiting on the message loop to execute scripts
};

//---------------------------------------------------------------------------
//
//  Class:   CDocFrag
//
//---------------------------------------------------------------------------

class CDocFrag : public CBase
{
public:
    DECLARE_CLASS_TYPES(CDocFrag, CBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDocFrag))

    STDMETHOD(PrivateQueryInterface)(REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, PrivateAddRef)();
    STDMETHOD_(ULONG, PrivateRelease)();

    //
    // methods
    //

    ~CDocFrag() { super::Passivate(); }

    CMarkup *Markup();

    //
    // IDispatchEx
    //

    NV_DECLARE_TEAROFF_METHOD(GetNameSpaceParent, getnamespaceparent, (IUnknown ** ppunk));

    //
    // wiring
    //

    #define _CDocFrag_
    #include "markup.hdl"

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

};

//---------------------------------------------------------------------------
//
//  Class:   CMarkupBehaviorContext
//
//---------------------------------------------------------------------------

class CMarkupBehaviorContext
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupBehaviorContext))

    CMarkupBehaviorContext();
    ~CMarkupBehaviorContext();
    
    CHtmlComponent *            _pHtmlComponent;
    CXmlNamespaceTable *        _pXmlNamespaceTable;
};

#if MARKUP_DIRTYRANGE
//---------------------------------------------------------------------------
//
//  Class:   CMarkupDirtyRangeContext
//
//---------------------------------------------------------------------------

struct MarkupDirtyRange
{
    CDirtyTextRegion    _dtr;
    DWORD               _dwCookie;
};

class CMarkupDirtyRangeContext
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupDirtyRangeContext))

    DECLARE_CStackDataAry(CAryMarkupDirtyRange, MarkupDirtyRange, 2, Mt(Mem), Mt(CMarkupDirtyRangeContext_aryMarkupDirtyRange_pv))
    CAryMarkupDirtyRange    _aryMarkupDirtyRange;
};
#endif

//---------------------------------------------------------------------------
//
//  Class:   CMarkupTextFragContext
//
//---------------------------------------------------------------------------

struct MarkupTextFrag
{
    CTreePos *  _ptpTextFrag;
    TCHAR *     _pchTextFrag;
};

class CMarkupTextFragContext
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupTextFragContext))

    ~CMarkupTextFragContext();

    DECLARE_CDataAry(CAryMarkupTextFrag, MarkupTextFrag, Mt(Mem), Mt(CMarkupTextFragContext_aryMarkupTextFrag_pv))
    CAryMarkupTextFrag      _aryMarkupTextFrag;

    long        FindTextFragAtCp( long cp, BOOL * pfFragFound );
    HRESULT     AddTextFrag( CTreePos * ptpTextFrag, TCHAR* pchTextFrag, ULONG cchTextFrag, long iTextFrag );
    HRESULT     RemoveTextFrag( long iTextFrag, CMarkup * pMarkup );

#if DBG==1
    void        TextFragAssertOrder();
#endif
};

//---------------------------------------------------------------------------
//
//  Class:   CMarkupTopElemCache
//
//---------------------------------------------------------------------------

class CMarkupTopElemCache
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupTextFragContext))
            
    CHtmlElement  * __pHtmlElementCached;
    CHeadElement  * __pHeadElementCached;
    CTitleElement * __pTitleElementCached;
    CElement *      __pElementClientCached;
};

//---------------------------------------------------------------------------
//
//  Class:   CMarkup
//
//---------------------------------------------------------------------------

class CMarkup : public CBase
{
    friend class CTxtPtr;
    friend class CTreePos;
    friend class CMarkupPointer;

    DECLARE_CLASS_TYPES( CMarkup, CBase )
            
public:
    DECLARE_TEAROFF_TABLE(ISelectionRenderingServices)
    DECLARE_TEAROFF_TABLE(IMarkupContainer)
    DECLARE_TEAROFF_TABLE(IMarkupTextFrags)

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkup))

    CMarkup( CDoc *pDoc, CElement * pElementMaster = NULL );
    ~CMarkup();

    HRESULT Init ( CRootElement * pElementRoot );

    HRESULT CreateInitialMarkup( CRootElement * pElementRoot );

    HRESULT UnloadContents( BOOL fForPassivate = FALSE );

    HRESULT DestroySplayTree( BOOL fReinit );

    void Passivate();

    HRESULT CreateElement (
        ELEMENT_TAG etag, CElement * * ppElementNew,
        TCHAR * pch = NULL, long cch = 0 );

    #define _CMarkup_
    #include "markup.hdl"

    // IDispatchEx methods:
    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (BSTR bstrName, DWORD grfdex, DISPID *pid));
    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (DISPID dispidMember, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, IServiceProvider *pSrvProvider));
    NV_DECLARE_TEAROFF_METHOD(GetNextDispID, getnextdispid, (DWORD grfdex, DISPID id, DISPID *pid));
    NV_DECLARE_TEAROFF_METHOD(GetMemberName, getmembername, (DISPID id, BSTR *pbstrName));
    NV_DECLARE_TEAROFF_METHOD(GetNameSpaceParent, getnamespaceparent, (IUnknown **ppunk));

    // Document props\methods:
    NV_DECLARE_TEAROFF_METHOD(get_Script, GET_Script, (IDispatch**p));
    NV_DECLARE_TEAROFF_METHOD(get_all, GET_all, (IHTMLElementCollection**p));
    NV_DECLARE_TEAROFF_METHOD(get_body, GET_body, (IHTMLElement**p));
    NV_DECLARE_TEAROFF_METHOD(get_activeElement, GET_activeElement, (IHTMLElement**p));
    NV_DECLARE_TEAROFF_METHOD(get_anchors, GET_anchors, (IHTMLElementCollection**p));
    NV_DECLARE_TEAROFF_METHOD(get_applets, GET_applets, (IHTMLElementCollection**p));
    NV_DECLARE_TEAROFF_METHOD(get_links, GET_links, (IHTMLElementCollection**p));
    NV_DECLARE_TEAROFF_METHOD(get_forms, GET_forms, (IHTMLElementCollection**p));
    NV_DECLARE_TEAROFF_METHOD(get_images, GET_images, (IHTMLElementCollection**p));
    NV_DECLARE_TEAROFF_METHOD(put_title, PUT_title, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_title, GET_title, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_scripts, GET_scripts, (IHTMLElementCollection**p));
    NV_DECLARE_TEAROFF_METHOD(put_designMode, PUT_designMode, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_designMode, GET_designMode, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_embeds, GET_embeds, (IHTMLElementCollection**p));
    NV_DECLARE_TEAROFF_METHOD(get_selection, GET_selection, (IHTMLSelectionObject**p));
    NV_DECLARE_TEAROFF_METHOD(get_readyState, GET_readyState, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_frames, GET_frames, (IHTMLFramesCollection2**p));
    NV_DECLARE_TEAROFF_METHOD(get_plugins, GET_plugins, (IHTMLElementCollection**p));
    NV_DECLARE_TEAROFF_METHOD(put_URL, PUT_URL, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_URL, GET_URL, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_location, GET_location, (IHTMLLocation**p));
    NV_DECLARE_TEAROFF_METHOD(get_referrer, GET_referrer, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_lastModified, GET_lastModified, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(put_domain, PUT_domain, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_domain, GET_domain, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(put_cookie, PUT_cookie, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_cookie, GET_cookie, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(put_expando, PUT_expando, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(get_expando, GET_expando, (VARIANT_BOOL*p));
    NV_DECLARE_TEAROFF_METHOD(put_charset, PUT_charset, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_charset, GET_charset, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(put_defaultCharset, PUT_defaultCharset, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_defaultCharset, GET_defaultCharset, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_parentWindow, GET_parentWindow, (IHTMLWindow2**p));
    NV_DECLARE_TEAROFF_METHOD(put_dir, PUT_dir, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_dir, GET_dir, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_mimeType, GET_mimeType, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_fileSize, GET_fileSize, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_fileCreatedDate, GET_fileCreatedDate, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_fileModifiedDate, GET_fileModifiedDate, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_fileUpdatedDate, GET_fileUpdatedDate, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_security, GET_security, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_protocol, GET_protocol, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_nameProp, GET_nameProp, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, write, wRITE, (SAFEARRAY* psarray));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, writeln, wRITELN, (SAFEARRAY* psarray));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, open, oPEN, (BSTR url,VARIANT name,VARIANT features,VARIANT replace,IDispatch** pomWindowResult));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, close, cLOSE, ());
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, clear, cLEAR, ());
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, queryCommandSupported, querycommandsupported, (BSTR cmdID,VARIANT_BOOL* pfRet));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, queryCommandEnabled, querycommandenabled, (BSTR cmdID,VARIANT_BOOL* pfRet));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, queryCommandState, querycommandstate, (BSTR cmdID,VARIANT_BOOL* pfRet));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, queryCommandIndeterm, querycommandindeterm, (BSTR cmdID,VARIANT_BOOL* pfRet));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, queryCommandText, querycommandtext, (BSTR cmdID,BSTR* pcmdText));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, queryCommandValue, querycommandvalue, (BSTR cmdID,VARIANT* pcmdValue));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, execCommand, execcommand, (BSTR cmdID,VARIANT_BOOL showUI,VARIANT value,VARIANT_BOOL* pfRet));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, execCommandShowHelp, execcommandshowhelp, (BSTR cmdID,VARIANT_BOOL* pfRet));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, createElement, createelement, (BSTR eTag,IHTMLElement** newElem));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, elementFromPoint, elementfrompoint, (long x,long y,IHTMLElement** elementHit));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, toString, tostring, (BSTR* String));
    NV_DECLARE_TEAROFF_METHOD(get_styleSheets, GET_styleSheets, (IHTMLStyleSheetsCollection**p));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, createStyleSheet, createstylesheet, (BSTR bstrHref,long lIndex,IHTMLStyleSheet** ppnewStyleSheet));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, releaseCapture, releasecapture, ());
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, recalc, rECALC, (VARIANT_BOOL fForce));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, createTextNode, createtextnode, (BSTR text,IHTMLDOMNode** newTextNode));
    NV_DECLARE_TEAROFF_METHOD(get_documentElement, GET_documentelement, (IHTMLElement**pRootElem));
    NV_DECLARE_TEAROFF_METHOD(get_uniqueID, GET_uniqueID, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, attachEvent, attachevent, (BSTR event,IDispatch* pDisp,VARIANT_BOOL* pfResult));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, detachEvent, detachevent, (BSTR event,IDispatch* pDisp));
    NV_DECLARE_TEAROFF_METHOD(get_bgColor, GET_bgColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_bgColor, PUT_bgColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD(get_fgColor, GET_fgColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_fgColor, PUT_fgColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD(get_linkColor, GET_linkColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_linkColor, PUT_linkColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD(get_alinkColor, GET_alinkColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_alinkColor, PUT_alinkColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD(get_vlinkColor, GET_vlinkColor, (VARIANT * p));
    NV_DECLARE_TEAROFF_METHOD(put_vlinkColor, PUT_vlinkColor, (VARIANT p));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, createDocumentFragment, createdocumentfragment, (IHTMLDocument2** pNewDoc));
    NV_DECLARE_TEAROFF_METHOD(get_parentDocument, GET_parentDocument, (IHTMLDocument2**p));
    NV_DECLARE_TEAROFF_METHOD(put_enableDownload, PUT_enableDownload, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(get_enableDownload, GET_enableDownload, (VARIANT_BOOL*p));
    NV_DECLARE_TEAROFF_METHOD(put_baseUrl, PUT_baseUrl, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_baseUrl, GET_baseUrl, (BSTR*p));
    NV_DECLARE_TEAROFF_METHOD(get_childNodes, GET_childNodes, (IDispatch**p));
    NV_DECLARE_TEAROFF_METHOD(put_inheritStyleSheets, PUT_inheritStyleSheets, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(get_inheritStyleSheets, GET_inheritStyleSheets, (VARIANT_BOOL*p));
    NV_DECLARE_TEAROFF_METHOD(getElementsByName, getelementsbyname, (BSTR v, IHTMLElementCollection** p));
    NV_DECLARE_TEAROFF_METHOD(getElementsByTagName, getelementsbytagname, (BSTR v, IHTMLElementCollection** p));
    NV_DECLARE_TEAROFF_METHOD(getElementById, getelementbyid, (BSTR v, IHTMLElement** p));
#ifdef IE5_ZOOM
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, zoom, zoom, (long, long));
    NV_DECLARE_TEAROFF_METHOD(get_zoomNumerator, GET_zoomNumerator, (long *));
    NV_DECLARE_TEAROFF_METHOD(get_zoomDenominator, GET_zoomDenominator, (long *));
#endif  // IE5_ZOOM

    //
    // IServiceProvider
    //

    // DECLARE_TEAROFF_TABLE(IServiceProvider)

    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID rguidService, REFIID riid, void ** ppvObject));

    //
    // script context
    //

    HRESULT EnsureScriptContext(
                CMarkupScriptContext **     ppScriptContext = NULL,
                LPTSTR                      pchUrl = NULL);

    BOOL    IsInInline();
    HRESULT EnterInline();
    HRESULT LeaveInline();

    BOOL    AllowInlineExecution();
    BOOL    AllowImmediateExecution();
    HRESULT WaitInlineExecution();
    HRESULT WakeUpExecution();

    void    EnterScriptDownload(DWORD * pdwCookie);
    HRESULT LeaveScriptDownload(DWORD * pdwCookie);
    BOOL    IsScriptDownload();

    HRESULT EnqueueScriptToCommit(CScriptElement *pScriptElement);
    HRESULT CommitQueuedScripts();
    HRESULT CommitQueuedScriptsInline();

    //
    // behaviors
    //

    HRESULT EnsureBehaviorContext(CMarkupBehaviorContext ** ppBehaviorContext = NULL);

    HRESULT RecomputePeers();

    HRESULT ProcessPeerTask(PEERTASK task);

    //
    // xml namespaces
    //

    HRESULT     EnsureXmlNamespaceTable(CXmlNamespaceTable ** ppXmlNamespaceTable = NULL);
    inline CXmlNamespaceTable * GetXmlNamespaceTable()
    {
        CMarkupBehaviorContext *    pBehaviorContext = BehaviorContext();
        return pBehaviorContext ? pBehaviorContext->_pXmlNamespaceTable : NULL;
    };

#ifdef VSTUDIO7
    ELEMENT_TAG IsGenericElement    (LPTSTR pchFullName, BOOL fAllSprinklesGeneric, BOOL *pfDerived = NULL);
#else
    ELEMENT_TAG IsGenericElement    (LPTSTR pchFullName, BOOL fAllSprinklesGeneric);
#endif //VSTUDIO7
    ELEMENT_TAG IsXmlSprinkle       (LPTSTR pchNamespace);
    HRESULT     RegisterXmlNamespace(LPTSTR pchNamespace, LPTSTR pchUrn, XMLNAMESPACETYPE namespaceType);
    HRESULT     SaveXmlNamespaceAttrs (CStreamWriteBuff * pStreamWriteBuff);

    //
    // Data Access
    //

    CDoc *                    Doc()             { return _pDoc;             }
    CRootElement *            Root()            { return _pElementRoot;     }

    BOOL                      HasCollectionCache() { return HasLookasidePtr(LOOKASIDE_COLLECTIONCACHE); }
    CCollectionCache *           CollectionCache() { return (CCollectionCache *) GetLookasidePtr(LOOKASIDE_COLLECTIONCACHE); }
    CCollectionCache *        DelCollectionCache() { return (CCollectionCache *) DelLookasidePtr(LOOKASIDE_COLLECTIONCACHE); }
    HRESULT                   SetCollectionCache(CCollectionCache * pCollectionCache) { return SetLookasidePtr(LOOKASIDE_COLLECTIONCACHE, pCollectionCache); }

    BOOL                      HasParentMarkup()    { return HasLookasidePtr(LOOKASIDE_PARENTMARKUP); }
    CMarkup *                    ParentMarkup()    { return (CMarkup *)GetLookasidePtr(LOOKASIDE_PARENTMARKUP); }
    CMarkup *                 DelParentMarkup()    { return (CMarkup *) DelLookasidePtr(LOOKASIDE_PARENTMARKUP); }
    HRESULT                   SetParentMarkup(CMarkup * pMarkup) { return SetLookasidePtr(LOOKASIDE_PARENTMARKUP, pMarkup); }

    BOOL                      HasScriptContext()   { return HasLookasidePtr(LOOKASIDE_SCRIPTCONTEXT);                               }
    CMarkupScriptContext *       ScriptContext()   { return (CMarkupScriptContext *) GetLookasidePtr(LOOKASIDE_SCRIPTCONTEXT);      }
    CMarkupScriptContext *    DelScriptContext()   { return (CMarkupScriptContext *) DelLookasidePtr(LOOKASIDE_SCRIPTCONTEXT);      }
    HRESULT                   SetScriptContext(CMarkupScriptContext * pScriptContext) { return SetLookasidePtr(LOOKASIDE_SCRIPTCONTEXT, pScriptContext); }

    BOOL                      HasBehaviorContext() { return HasLookasidePtr(LOOKASIDE_BEHAVIORCONTEXT);                             }
    CMarkupBehaviorContext *     BehaviorContext() { return (CMarkupBehaviorContext *) GetLookasidePtr(LOOKASIDE_BEHAVIORCONTEXT);  }
    CMarkupBehaviorContext *  DelBehaviorContext() { return (CMarkupBehaviorContext *) DelLookasidePtr(LOOKASIDE_BEHAVIORCONTEXT);  }
    HRESULT                   SetBehaviorContext(CMarkupBehaviorContext * pBehaviorContext) { return SetLookasidePtr(LOOKASIDE_BEHAVIORCONTEXT, pBehaviorContext); }

    BOOL                      HasStyleSheetArray() { return HasLookasidePtr(LOOKASIDE_STYLESHEETS); }
    CStyleSheetArray *        GetStyleSheetArray() { return (CStyleSheetArray *) GetLookasidePtr(LOOKASIDE_STYLESHEETS); }
    CStyleSheetArray *        DelStyleSheetArray() { return (CStyleSheetArray *) DelLookasidePtr(LOOKASIDE_STYLESHEETS); }
    HRESULT                   SetStyleSheetArray(CStyleSheetArray * pStyleSheets) { return SetLookasidePtr(LOOKASIDE_STYLESHEETS, pStyleSheets); }

    BOOL                      HasTextRangeListPtr() { return HasLookasidePtr(LOOKASIDE_TEXTRANGE); }
    CAutoRange *              GetTextRangeListPtr() { return (CAutoRange *) GetLookasidePtr(LOOKASIDE_TEXTRANGE); }
    CAutoRange *              DelTextRangeListPtr() { return (CAutoRange *) DelLookasidePtr(LOOKASIDE_TEXTRANGE); }
    HRESULT                   SetTextRangeListPtr(CAutoRange * pAutoRange) { return SetLookasidePtr(LOOKASIDE_TEXTRANGE, pAutoRange); }

    CElement *      Master()        { return _pElementMaster; }
    void            ClearMaster()   { _pElementMaster = NULL; }

    CBase *         GetDD();

    LOADSTATUS      LoadStatus();
    CProgSink *     GetProgSinkC();
    IProgSink *     GetProgSink();
    CHtmCtx *       HtmCtx()        { return _pHtmCtx; }

    CElement *      FirstElement();

    // Remove 'Assert(Master())', once the root element is gone and these functions
    // become meaningful for non-slave markups as well.
    long            GetContentFirstCp()     { Assert(Master()); return 2; }
    long            GetContentLastCp()      { Assert(Master()); return GetTextLength() - 2; }
    long            GetContentTextLength()  { Assert(Master()); return GetTextLength() - 4; }
    void            GetContentTreeExtent(CTreePos ** pptpStart, CTreePos ** pptpEnd);

    CHtmRootParseCtx * GetRootParseCtx() { return _pRootParseCtx; }
    void            SetRootParseCtx( CHtmRootParseCtx * pCtx )  { _pRootParseCtx = pCtx; }

    BOOL        GetAutoWordSel() const          { return TRUE;            }
    BOOL        GetModified() const             { return _fModified;      }
    void        SetModified();

    BOOL        GetReadOnly() const             { return _fReadOnly;      }

    BOOL        GetOverstrike() const           { return _fOverstrike;    }
    void        SetOverstrike(BOOL fSet)        { _fOverstrike = (fSet) ? 1 : 0;}

    BOOL        GetLoaded() const               { return _fLoaded;     }
    void        SetLoaded(BOOL fLoaded)         { _fLoaded = fLoaded;  }

    void        SetStreaming(BOOL flag)         { _fStreaming = (flag) ? 1 : 0; }
    BOOL        IsStreaming() const             { return _fStreaming; }

#ifdef XMV_PARSE    
    void        SetXML(BOOL flag);
    BOOL        IsXML() const                   { return _fXML; }
#endif    

    // BUGBUG: need to figure this function out...
    BOOL        IsEditable(BOOL fCheckContainerOnly = FALSE) const;
    
    //
    // Top element cache
    //
    
    void            EnsureTopElems();
    CElement *      GetElementTop();
    CElement *      GetElementClient() { EnsureTopElems(); return HasTopElemCache() ? GetTopElemCache()->__pElementClientCached : NULL;  }
    CHtmlElement *  GetHtmlElement()   { EnsureTopElems(); return HasTopElemCache() ? GetTopElemCache()->__pHtmlElementCached   : NULL;  }
    CHeadElement *  GetHeadElement()   { EnsureTopElems(); return HasTopElemCache() ? GetTopElemCache()->__pHeadElementCached   : NULL;  }
    CTitleElement * GetTitleElement()  { EnsureTopElems(); return HasTopElemCache() ? GetTopElemCache()->__pTitleElementCached  : NULL;  }
    
    //
    // Head elements manipulation
    //
    HRESULT AddHeadElement(CElement * pElement, long lIndex = -1);
    HRESULT LocateHeadMeta (BOOL (*pfnCompare) (CMetaElement *), CMetaElement**);
    HRESULT LocateOrCreateHeadMeta (BOOL (*pfnCompare) (CMetaElement *), CMetaElement**, BOOL fInsertAtEnd = TRUE);
    BOOL    MetaPersistEnabled(long eState);
    HRESULT EnsureTitle();

#ifdef MARKUP_STABLE
    //
    // Tree services stability inetrface
    //
    BOOL    IsStable();         // returns TRUE if tree is stable.
    HRESULT MakeItStable();     // fixup the tree.
    void    UpdateStableTreeVersionNumber(); // update stable tree version number with the new tree version number
    UNSTABLE_CODE ValidateParserRules();
#endif

    //
    // Other helpers
    //

    BOOL        IsPrimaryMarkup()               { return this == Doc()->_pPrimaryMarkup; }
    long        GetTextLength()                 { return _TxtArray._cchText; }

    // overrides
    virtual CAtomTable * GetAtomTable (BOOL *pfExpando = NULL)
       { if (pfExpando) *pfExpando = _pDoc->_fExpando; return &_pDoc->_AtomTable; }

    // GetRunOwner is an abomination that should be eliminated -- or at least moved off of the markup
    CLayout *   GetRunOwner ( CTreeNode * pNode, CLayout * pLayoutParent = NULL );
    CTreeNode * GetRunOwnerBranch ( CTreeNode *, CLayout * pLayoutParent = NULL );

    //
    // Markup manipulation functions
    //

    void CompactStory() { _TxtArray.ShrinkBlocks(); }

    HRESULT RemoveElementInternal (
        CElement *      pElementRemove,
        DWORD           dwFlags = NULL );

    HRESULT InsertElementInternal(
        CElement *      pElementInsertThis,
        CTreePosGap *   ptpgBegin,
        CTreePosGap *   ptpgEnd,
        DWORD           dwFlags = NULL);

    HRESULT SpliceTreeInternal(
        CTreePosGap *   ptpgStartSource,
        CTreePosGap *   ptpgEndSource,
        CMarkup *       pMarkupTarget  = NULL,
        CTreePosGap *   ptpgTarget  = NULL,
        BOOL            fRemove  = TRUE,
        DWORD           dwFlags = NULL );
    
    HRESULT InsertTextInternal(
        CTreePos *      ptpAfterInsert,
        const TCHAR *   pch,
        long            cch,
        DWORD           dwFlags = NULL );

    HRESULT FastElemTextSet(
        CElement * pElem, 
        const TCHAR * psz, 
        int cch, 
        BOOL fAsciiOnly );

    // Undo only operations
    HRESULT UndoRemoveSplice(
        CMarkupPointer *    pmpBegin,
        CSpliceRecordList * paryRegion,
        long                cchRemove,
        TCHAR *             pchRemove,
        long                cchNodeReinsert,
        DWORD               dwFlags );


    // Markup operation helpers
    HRESULT ReparentDirectChildren (
        CTreeNode *     pNodeParentNew,
        CTreePosGap *   ptpgStart = NULL,
        CTreePosGap *   ptpgEnd = NULL);

    HRESULT CreateInclusion(
        CTreeNode *     pNodeStop,
        CTreePosGap*    ptpgLocation,
        CTreePosGap*    ptpgInclusion,
        long *          pcchNeeded = NULL,
        CTreeNode *     pNodeAboveLocation = NULL,
        BOOL            fFullReparent = TRUE,
        CTreeNode **    ppNodeLastAdded = NULL);

    HRESULT CloseInclusion(
        CTreePosGap *   ptpgMiddle,
        long *          pcchRemove = NULL );

    HRESULT RangeAffected ( CTreePos *ptpStart, CTreePos *ptpFinish );
    HRESULT ClearCaches( CTreePos *  ptpStart, CTreePos * ptpFinish );
    HRESULT ClearRunCaches( DWORD dwFlags, CElement * pElement );

    //
    // Markup pointers
    //

    HRESULT EmbedPointers ( ) { return _pmpFirst ? DoEmbedPointers() : S_OK; }

    HRESULT DoEmbedPointers ( );

    BOOL HasUnembeddedPointers ( ) { return !!_pmpFirst; }

    //
    // TextID manipulation
    //

    // Give a unique text id to every text chunk in
    // the range given
    HRESULT SetTextID(
        CTreePosGap *   ptpgStart,
        CTreePosGap *   ptpgEnd,
        long *plNewTextID );

    // Get text ID for text to right
    // -1 --> no text to right
    // 0  --> no assigned TextID
    long GetTextID( 
        CTreePosGap * ptpg );

    // Starting with ptpgStart, look for
    // the extent of lTextID
    HRESULT FindTextID(
        long            lTextID,
        CTreePosGap *   ptpgStart,
        CTreePosGap *   ptpgEnd );


    // If text to the left of ptpLeft has
    // the same ID as text to the right of
    // ptpRight, give the right fragment a
    // new ID
    void SplitTextID(
        CTreePos *   ptpLeft,
        CTreePos *   ptpRight );

#if 0
    This routine is not currently needed and may not even be correct!
    //
    // Script ID Helper
    //

    HRESULT EnsureSidAfterCharsInserted(
        CTreeNode * pNodeNotify,
        CTreePos *  ptpText,
        long        ichStart,
        long        cch );
#endif

    //
    // Splay Tree Primitives
    //
    CTreePos *  NewTextPos(long cch, SCRIPT_ID sid = sidAsciiLatin, long lTextID = 0);
    CTreePos *  NewPointerPos(CMarkupPointer *pPointer, BOOL fRight, BOOL fStick);

    typedef CTreePos SUBLIST;

    HRESULT Append(CTreePos *ptp);
    HRESULT Insert(CTreePos *ptpNew, CTreePosGap *ptpgInsert);
    HRESULT Insert(CTreePos *ptpNew, CTreePos *ptpInsert, BOOL fBefore);
    HRESULT Move(CTreePos *ptpMove, CTreePosGap *ptpgDest);
    HRESULT Move(CTreePos *ptpMove, CTreePos *ptpDest, BOOL fBefore);
    HRESULT Remove(CTreePosGap *ptpgStart, CTreePosGap *ptpgFinish);
    HRESULT Remove(CTreePos *ptpStart, CTreePos *ptpFinish);
    HRESULT Remove(CTreePos *ptp) { return Remove(ptp, ptp); }
    HRESULT Split(CTreePos *ptpSplit, long cchLeft, SCRIPT_ID sidNew = sidNil );
    HRESULT Join(CTreePos *ptpJoin);
    HRESULT ReplaceTreePos(CTreePos * ptpOld, CTreePos * ptpNew); 
    HRESULT MergeText ( CTreePos * ptpMerge );
    HRESULT SetTextPosID( CTreePos ** pptpText, long lTextID ); // DANGER: may realloc the text pos if old textID == 0
    HRESULT RemovePointerPos ( CTreePos * ptp, CTreePos * * pptpUpdate, long * pichUpdate );
    
    HRESULT SpliceOut(CTreePos *ptpStart, CTreePos *ptpFinish, SUBLIST *pSublistSplice);
    HRESULT SpliceIn(SUBLIST *pSublistSplice, CTreePos *ptpSplice);
#if 0 // not used and useless
    HRESULT CloneOut(CTreePos *ptpStart, CTreePos *ptpFinish, SUBLIST *pSublistClone, CMarkup *pMarkupOwner);
    HRESULT CopyFrom(CMarkup *pMarkpSource, CTreePos *ptpStart, CTreePos *ptpFinish, CTreePos *ptpInsert);
#endif
    HRESULT InsertPosChain( CTreePos * ptpChainHead, CTreePos * ptpChainTail, CTreePos * ptpInsertBefore );

    // splay tree search routines
    CTreePos *  FirstTreePos() const;
    CTreePos *  LastTreePos() const;
#if DBG==1
    CTreePos *  TreePosAtCp(long cp, long *pcchOffset, BOOL fNoTrace=FALSE) const;
    CTreePos *  TreePosAtCp_NoTrace(long cp, long *pcchOffset) const
                { return TreePosAtCp(cp, pcchOffset, TRUE); }
#else
    CTreePos *  TreePosAtCp(long cp, long *pcchOffset) const;
#endif
    CTreePos *  TreePosAtSourceIndex(long iSourceIndex);

    // General splay information
    
    long NumElems() const { return _tpRoot.GetElemLeft(); }
    long Cch() const { return _tpRoot._cchLeft; }
    long CchInRange(CTreePos *ptpFirst, CTreePos *ptpLast);

    // Force a splay
    void FocusAt(CTreePos *ptp) { ptp->Splay(); }

    // splay tree primitive helpers
protected:
    CTreePos * AllocData1Pos();
    CTreePos * AllocData2Pos();
    void    FreeTreePos(CTreePos *ptp);
    void    ReleaseTreePos(CTreePos *ptp, BOOL fLastRelease = FALSE );
    BOOL    ShouldSplay(long cDepth) const;
    HRESULT MergeTextHelper(CTreePos *ptpMerge);

public:
    //
    // Text Story
    //
    LONG GetTextLength ( ) const    { return _TxtArray._cchText; }

    //
    // Notifications
    //
    void    Notify(CNotification * pnf);
    void    Notify(CNotification & rnf)         { Notify(&rnf); }

    //  notification helpers
protected:
    void SendNotification(CNotification * pnf, CDataAry<CNotification> * paryNotification);
    void SendAntiNotification(CNotification * pnf);

    BOOL ElementWantsNotification(CElement * pElement, CNotification * pnf);

    void NotifyElement(CElement * pElement, CNotification * pnf);
    void NotifyAncestors(CNotification * pnf);
    void NotifyDescendents(CNotification * pnf);
    void NotifyTreeLevel(CNotification * pnf);

    WHEN_DBG( void ValidateChange(CNotification * pnf) ); 
    WHEN_DBG( BOOL AreChangesValid() ); 

public:
    //
    // Branch searching functions - I'm sure some of these aren't needed
    //
    // Note: All of these (except InStory versions) implicitly stop searching at a FlowLayout
    CTreeNode * FindMyListContainer ( CTreeNode * pNodeStartHere );
    CTreeNode * SearchBranchForChildOfScope ( CTreeNode * pNodeStartHere, CElement * pElementFindChildOfMyScope );
    CTreeNode * SearchBranchForChildOfScopeInStory ( CTreeNode * pNodeStartHere, CElement * pElementFindChildOfMyScope );
    CTreeNode * SearchBranchForScopeInStory ( CTreeNode * pNodeStartHere, CElement * pElementFindMyScope );
    CTreeNode * SearchBranchForScope ( CTreeNode * pNodeStartHere, CElement * pElementFindMyScope );
    CTreeNode * SearchBranchForNode ( CTreeNode * pNodeStartHere, CTreeNode * brFindMe );
    CTreeNode * SearchBranchForNodeInStory ( CTreeNode * pNodeStartHere, CTreeNode * brFindMe );
    CTreeNode * SearchBranchForTag ( CTreeNode * pNodeStartHere, ELEMENT_TAG );
    CTreeNode * SearchBranchForTagInStory ( CTreeNode * pNodeStartHere, ELEMENT_TAG );
    CTreeNode * SearchBranchForBlockElement ( CTreeNode * pNodeStartHere, CFlowLayout * pFLContext = NULL );
    CTreeNode * SearchBranchForNonBlockElement ( CTreeNode * pNodeStartHere, CFlowLayout * pFLContext = NULL );
    CTreeNode * SearchBranchForAnchor ( CTreeNode * pNodeStartHere );
    CTreeNode * SearchBranchForAnchorLink ( CTreeNode * pNodeStartHere );
    CTreeNode * SearchBranchForCriteria ( CTreeNode * pNodeStartHere, BOOL (* pfnSearchCriteria) ( CTreeNode * ) );
    CTreeNode * SearchBranchForCriteriaInStory ( CTreeNode * pNodeStartHere, BOOL (* pfnSearchCriteria) ( CTreeNode * ) );

    //
    // Loading
    //

    HRESULT Load (HTMLOADINFO * phtmloadinfo);
    HRESULT Load (IMoniker * pMoniker, IBindCtx * pBindCtx = NULL);
    HRESULT Load (IStream * pStream, HTMPASTEINFO * phtmpasteinfo = NULL);
    void    OnLoadStatus(LOADSTATUS LoadStatus);
    HRESULT StopDownload();
    void    EnsureFormats();

#if MARKUP_DIRTYRANGE
    //
    // Dirty Range Service
    //  Listeners can register for the markup to accumulate a dirty range
    //  for them.
    //
    HRESULT RegisterForDirtyRange( DWORD * pdwCookie );
    HRESULT UnRegisterForDirtyRange( DWORD dwCookie );
    HRESULT GetAndClearDirtyRange( DWORD dwCookie, CMarkupPointer * pmpBegin, CMarkupPointer * pmpEnd );
    HRESULT GetAndClearDirtyRange( DWORD dwCookie, IMarkupPointer * pIPointerBegin, IMarkupPointer * pIPointerEnd );
    NV_DECLARE_ONCALL_METHOD(OnDirtyRangeChange, ondirtyrangechange, (DWORD_PTR dwParam));

    CMarkupDirtyRangeContext *  EnsureDirtyRangeContext();

    BOOL                        HasDirtyRangeContext()    { return HasLookasidePtr(LOOKASIDE_DIRTYRANGE); }
    CMarkupDirtyRangeContext *  GetDirtyRangeContext()    { return (CMarkupDirtyRangeContext *) GetLookasidePtr(LOOKASIDE_DIRTYRANGE); }
    HRESULT                     SetDirtyRangeContext(CMarkupDirtyRangeContext * pdrc) { return SetLookasidePtr(LOOKASIDE_DIRTYRANGE, pdrc); }
    CMarkupDirtyRangeContext *  DelDirtyRangeContext()    { return (CMarkupDirtyRangeContext *) DelLookasidePtr(LOOKASIDE_DIRTYRANGE); }
#endif

    //
    // Markup TextFrag services
    //  Markup TextFrags are used to store arbitrary string data in the markup.  Mostly
    //  this is used to persist and edit conditional comment tags
    //
    CMarkupTextFragContext *    EnsureTextFragContext();

    BOOL                        HasTextFragContext()    { return HasLookasidePtr(LOOKASIDE_TEXTFRAGCONTEXT); }
    CMarkupTextFragContext *    GetTextFragContext()    { return (CMarkupTextFragContext *) GetLookasidePtr(LOOKASIDE_TEXTFRAGCONTEXT); }
    HRESULT                     SetTextFragContext(CMarkupTextFragContext * ptfc) { return SetLookasidePtr(LOOKASIDE_TEXTFRAGCONTEXT, ptfc); }
    CMarkupTextFragContext *    DelTextFragContext()    { return (CMarkupTextFragContext *) DelLookasidePtr(LOOKASIDE_TEXTFRAGCONTEXT); }
    
    //
    // Markup TopElems services
    //  Stores the cached values for the HTML/HEAD/TITLE and client elements
    //
    CMarkupTopElemCache *       EnsureTopElemCache();

    BOOL                        HasTopElemCache()    { return HasLookasidePtr(LOOKASIDE_TOPELEMCACHE); }
    CMarkupTopElemCache *       GetTopElemCache()    { return (CMarkupTopElemCache *) GetLookasidePtr(LOOKASIDE_TOPELEMCACHE); }
    HRESULT                     SetTopElemCache(CMarkupTopElemCache * ptec) { return SetLookasidePtr(LOOKASIDE_TOPELEMCACHE, ptec); }
    CMarkupTopElemCache *       DelTopElemCache()    { return (CMarkupTopElemCache *) DelLookasidePtr(LOOKASIDE_TOPELEMCACHE); }
    
    //
    // Misc editing stuff...can't this go anywhere else?
    //
#ifdef UNIX_NOTYET
    HRESULT     PasteUnixQuickTextToRange( 
                        CTxtRange *prg,
                        VARIANTARG *pvarTextHandle );
#endif
    HRESULT     createTextRange(
                        IHTMLTxtRange * * ppDisp, 
                        CElement * pElemContainer );

    HRESULT     createTextRange(
                        IHTMLTxtRange   ** ppDisp, 
                        CElement        * pElemContainer,
                        IMarkupPointer  * pLeft,
                        IMarkupPointer  * pRight,
                        BOOL            fAdjustPointers);


    HRESULT     PasteClipboard();

    //
    // Selection Methods
    //
    
    VOID        GetSelectionChunksForLayout( CFlowLayout* pFlowLayout, CPtrAry<HighlightSegment*> *paryHighlight, int* piCpMin, int * piCpMax );           
    HRESULT        EnsureSelRenSvc();                             
    BOOL IsElementSelected( CElement* pElement );
    CElement* GetSelectedElement( int iElementIndex );
    HRESULT GetFlattenedSelection( int iSegmentIndex, int & cpStart, int & cpEnd, SELECTION_TYPE&  eType )    ;
    VOID HideSelection();
    VOID ShowSelection();
    VOID InvalidateSelection( BOOL fFireOM );    
    //
    // Collections support
    //

    enum
    {
        ELEMENT_COLLECTION = 0,
        FORMS_COLLECTION,
        ANCHORS_COLLECTION,
        LINKS_COLLECTION,
        IMAGES_COLLECTION,
        APPLETS_COLLECTION,
        SCRIPTS_COLLECTION,
        MAPS_COLLECTION,
        WINDOW_COLLECTION,
        EMBEDS_COLLECTION,
        REGION_COLLECTION,
        LABEL_COLLECTION,
        NAVDOCUMENT_COLLECTION,
        FRAMES_COLLECTION,
        NUM_DOCUMENT_COLLECTIONS
    };
    // DISPID range for FRAMES_COLLECTION
    enum
    {
        FRAME_COLLECTION_MIN_DISPID = ((DISPID_COLLECTION_MIN+DISPID_COLLECTION_MAX)*2)/3+1,
        FRAME_COLLECTION_MAX_DISPID = DISPID_COLLECTION_MAX
    };

    HRESULT         EnsureCollectionCache(long lCollectionIndex);
    HRESULT         AddToCollections(CElement * pElement, CCollectionBuildContext * pWalk);

    HRESULT         InitCollections ( void );

    NV_DECLARE_ENSURE_METHOD(EnsureCollections, ensurecollections, (long lIndex, long * plCollectionVersion));
    HRESULT         GetCollection(int iIndex, IHTMLElementCollection ** ppdisp);
    HRESULT         GetElementByNameOrID(LPTSTR szName, CElement **ppElement);
    HRESULT         GetDispByNameOrID(LPTSTR szName, IDispatch **ppDisp, BOOL fAlwaysCollection = FALSE);

    CTreeNode *     InFormCollection(CTreeNode * pNode);


    //
    //
    // Lookaside pointers
    //

    enum
    {
        LOOKASIDE_COLLECTIONCACHE   = 0,
        LOOKASIDE_PARENTMARKUP      = 1,
        LOOKASIDE_SCRIPTCONTEXT     = 2,
        LOOKASIDE_BEHAVIORCONTEXT   = 3,
        LOOKASIDE_TEXTFRAGCONTEXT   = 4,
        LOOKASIDE_TOPELEMCACHE      = 5,
        LOOKASIDE_STYLESHEETS       = 6,
        LOOKASIDE_TEXTRANGE         = 7,

        // *** There are only 8 bits reserved in the markup.
        // *** if you add more lookasides you have to make sure 
        // *** that you make room for those bits.

#if MARKUP_DIRTYRANGE
        LOOKASIDE_DIRTYRANGE        = 7,
#endif
        LOOKASIDE_MARKUP_NUMBER     = 8
    };

    BOOL            HasLookasidePtr(int iPtr)                   { return(_fHasLookasidePtr & (1 << iPtr)); }
    void *          GetLookasidePtr(int iPtr);
    HRESULT         SetLookasidePtr(int iPtr, void * pv);
    void *          DelLookasidePtr(int iPtr);

    void ClearLookasidePtrs ( );

    // Undo helper
    //

    virtual BOOL AcceptingUndo();

    //
    // IUnknown
    // 

    DECLARE_PLAIN_IUNKNOWN(CMarkup);

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    //
    // ISegmentList
    //
    NV_DECLARE_TEAROFF_METHOD(
        MovePointersToSegment, movepointerstosegment, (    
            int iSegmentIndex, 
            IMarkupPointer* pStart, 
            IMarkupPointer* pEnd ) ) ;
    NV_DECLARE_TEAROFF_METHOD(
        GetSegmentCount, getsegmentcount, ( 
            int* piSegmentCount,
            SELECTION_TYPE * peType ) ) ;

    //
    // ISelectionRenderingServices
    //
    NV_DECLARE_TEAROFF_METHOD(
        AddSegment, addsegment, ( 
            IMarkupPointer* pStart, 
            IMarkupPointer* pEnd,
            HIGHLIGHT_TYPE HighlightType,
            int * iSegmentIndex ) ) ;
    NV_DECLARE_TEAROFF_METHOD(
        AddElementSegment, addelementsegment, ( 
            IHTMLElement* pElement , 
            int * iSegmentIndex ) ) ;
    NV_DECLARE_TEAROFF_METHOD(
        MoveSegmentToPointers, movesegmenttopointers, ( 
            int iSegmentIndex, 
            IMarkupPointer* pStart, 
            IMarkupPointer* pEnd,
            HIGHLIGHT_TYPE HighlightType ) ) ;
    NV_DECLARE_TEAROFF_METHOD(
        GetElementSegment, getelementsegment, ( 
            int iSegmentIndex, 
            IHTMLElement** ppElement ) ) ;
    NV_DECLARE_TEAROFF_METHOD(
        SetElementSegment, setelementsegment, ( 
            int iSegmentIndex,    
            IHTMLElement* pElement ) ) ;
    NV_DECLARE_TEAROFF_METHOD(
        ClearSegment, clearsegment, (
            int iSegmentIndex,    
            BOOL fInvalidate ) ) ;
    NV_DECLARE_TEAROFF_METHOD(
        ClearSegments, clearsegments, (BOOL fInvalidate) ) ;
    NV_DECLARE_TEAROFF_METHOD(
        ClearElementSegments, clearelementsegments, () ) ;

    //
    // IMarkupContainer methods
    //

    NV_DECLARE_TEAROFF_METHOD( 
        OwningDoc, owningdoc, (
            IHTMLDocument2 * * ppDoc ) );

    //
    // IMarkupTextFrags methods
    //
    NV_DECLARE_TEAROFF_METHOD(
        GetTextFragCount, gettextfragcount, (long *pcFrags) );
    NV_DECLARE_TEAROFF_METHOD(
        GetTextFrag, gettextfrag, (
            long iFrag,
            BSTR* pbstrFrag,
            IMarkupPointer* pPointerFrag ) );
    NV_DECLARE_TEAROFF_METHOD(
        RemoveTextFrag, removetextfrag, (long iFrag) );
    NV_DECLARE_TEAROFF_METHOD(
        InsertTextFrag, inserttextfrag, (
            long iFrag,
            BSTR bstrInsert,
            IMarkupPointer* pPointerInsert ) );
    NV_DECLARE_TEAROFF_METHOD(
        FindTextFragFromMarkupPointer, findtextfragfrommarkuppointer, (
            IMarkupPointer* pPointerFind,
            long* piFrag,
            BOOL* pfFragFound ) );
    //
    // Ref counting helpers
    //

    static void ReplacePtr ( CMarkup ** pplhs, CMarkup * prhs );
    static void SetPtr     ( CMarkup ** pplhs, CMarkup * prhs );
    static void ClearPtr   ( CMarkup ** pplhs );
    static void StealPtrSet     ( CMarkup ** pplhs, CMarkup * prhs );
    static void StealPtrReplace ( CMarkup ** pplhs, CMarkup * prhs );
    static void ReleasePtr      ( CMarkup *  pMarkup );

    //
    // Debug stuff
    //

#if DBG==1 || defined(DUMPTREE)
    void DumpTree ( );
    void DumpTreeOverwrite ( );
    void SetDebugMarkup ( );

    void DumpClipboardText( );
    void DumpTreeWithMessage ( TCHAR * szMessage = NULL );
    void DumpTreeInXML ( );
    void PrintNode ( CTreeNode * pNode , BOOL fSimple = FALSE, int type = 0 );
    void PrintNodeTag ( CTreeNode * pNode, int type = 0 );
    void DumpTextChanges ( );
    void DumpNodeTreePos (CTreePos *ptpCurr);
    void DumpPointerTreePos (CTreePos *ptpCurr);
    void DumpTextTreePos (CTreePos *ptpCurr);

    void DumpDisplayTree();

    void DumpSplayTree(CTreePos *ptpStart=NULL, CTreePos *ptpFinish=NULL);
#endif

    WHEN_DBG( BOOL IsSplayValid() const; )
    WHEN_DBG( BOOL IsNodeValid(); )

    //
    // CMarkup::CLock
    //
    class CLock
    {
    public:
        DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
        CLock(CMarkup *pMarkup);
        ~CLock();

    private:
        CMarkup *     _pMarkup;
    };

protected:

    static const CLASSDESC s_classdesc;

    virtual const CBase::CLASSDESC *GetClassDesc() const { return & s_classdesc; }

    //
    // Data
    //

public:
    // loading
    LOADSTATUS      _LoadStatus;
    CHtmCtx *       _pHtmCtx;
    CProgSink *     _pProgSink;
    
    // the om document's default dispatch exposed by doc frags to script.
    CDocFrag        _OmDoc;

    CDoc * _pDoc;

    // BUGBUG!!! Adding members to CMarkup!  THe ones I'm adding are rather important,
    //           while others are less so.  Remove the others to bring the size back
    //           to normal.

    //
    // The following are similar to the CDoc's equivalent, but pertain to this
    // markup alone.
    //
    
    long GetMarkupTreeVersion() { return __lMarkupTreeVersion; }
    long GetMarkupContentsVersion() { return __lMarkupContentsVersion; }

    //
    // Do NOT modify these version numbers unless the document structure
    // or content is being modified.
    //
    // In particular, incrementing these to get a cache to rebuild is
    // BAD because it causes all sorts of other stuff to rebuilt.
    //
    
    long __lMarkupTreeVersion;         // Element structure
    long __lMarkupContentsVersion;     // Any content

#if DBG == 1
    BOOL __fDbgLockTree;
    void DbgLockTree(BOOL f) { __fDbgLockTree = f; }
    void UpdateMarkupTreeVersion ( );
    void UpdateMarkupContentsVersion ( );
#else
    void UpdateMarkupTreeVersion ( )
    {
        CDoc * pDoc = Doc();
        
        __lMarkupTreeVersion++;
        __lMarkupContentsVersion++;
        
        pDoc->__lDocTreeVersion++;
        pDoc->__lDocContentsVersion++;
    }
    
    void UpdateMarkupContentsVersion ( )
    {
        __lMarkupContentsVersion++;
        Doc()->UpdateDocContentsVersion();
    }
    
#endif

private:
    
    CRootElement *  _pElementRoot;
    CElement *      _pElementMaster;

    // Story data
    CTxtArray       _TxtArray;

    // Parsing context
    CHtmRootParseCtx *  _pRootParseCtx;     // Root parse context

    long _lTopElemsVersion;

    // Selection state
    CSelectionRenderingServiceProvider * _pSelRenSvcProvider; // Object to Delegate to.

#ifdef MARKUP_STABLE
    // Tree stability - move this?
    long _lStableTreeVersion;
#endif

    // Notification data
    DECLARE_CDataAry(CAryANotify, CNotification, Mt(Mem), Mt(CAryNotify_aryANotification_pv))
    CAryANotify _aryANotification;

#if DBG == 1
public:
    
    // This long keeps track of the total number of characters
    // in the tree, as reported by the notifactions.  This is
    // validated against the real count.  There is also
    // one for the number of elements in the tree
    
    long _cchTotalDbg;
    long _cElementsTotalDbg;
    
#endif
    
private:

    //
    // This is the list of pointers positioned in this markup
    // which do not have an embedding.
    //
    
    CMarkupPointer * _pmpFirst;

    // Splay Tree Data
    
    CTreePos        _tpRoot;       // dummy root node
    CTreePos *      _ptpFirst;     // cached first (leftmost) node
    void *          _pvPool;       // list of pool blocks (so they can be freed)
    CTreeDataPos *  _ptdpFree;     // head of free list
    BYTE            _abPoolInitial [ sizeof( void * ) + TREEDATA1SIZE * INITIAL_TREEPOS_POOL_SIZE ]; // The initial pool of TreePos objects

public:
    struct
    {
        DWORD   _fReadOnly         : 1; //  0 Control is read only
        DWORD   _fOverstrike       : 1; //  1 Overstrike mode vs insert mode
        DWORD   _fModified         : 1; //  2 Control text has been modified
        DWORD   _fLoaded           : 1; //  3 Is the markup completely parsed
        DWORD   _fNoUndoInfo       : 1; //  4 Don't store any undo info for this markup
        DWORD   _fIncrementalAlloc : 1; //  5 The text array should slow start
        DWORD   _fStreaming        : 1; //  6 True during parsing
        DWORD   _fUnstable         : 1; //  7 the tree is unstable because of the tree services/DOM operations 
                                        //    were performed on the tree and nobody call to validate the tree
        DWORD   _fInSendAncestor   : 1; //  8 Notification - We're sending something to ancestors
        DWORD   _fUnused1          : 1; //  9 
        DWORD   _fEnableDownload   : 1; //  10 Allows content to be downloaded in this markup
#if MARKUP_DIRTYRANGE
        DWORD   _fOnDirtyRangeChangePosted : 1; // NO NUMBER True when we have a posted call for the dirty range
#endif

#ifdef XMV_PARSE
        DWORD   _fXML              : 1; //  11 file contains native xml tags
        DWORD   _fPad              : 4; //  12-15 Padding to align lookaside flags on byte
#else
        DWORD   _fPad              : 5; //  11-15 Padding to align lookaside flags on byte
#endif
        DWORD   _fHasLookasidePtr  : 8; //  16-23 Lookaside flags
        DWORD   _fUnused2          : 8; //  24-31
    };

    // Style sheets moved from CDoc
    HRESULT         EnsureStyleSheets();
    HRESULT ApplyStyleSheets(
        CStyleInfo *    pStyleInfo,
        ApplyPassType   passType = APPLY_All,
        EMediaType      eMediaType = MEDIA_All,
        BOOL *          pfContainsImportant = NULL);

    HRESULT OnCssChange(BOOL fStable, BOOL fRecomputePeers);

    BOOL            HasStyleSheets()
    {   
        CStyleSheetArray * pStyleSheets = GetStyleSheetArray();
        return (pStyleSheets && pStyleSheets->Size()); 
    }

private:
    static DWORD   s_dwDirtyRangeServiceCookiePool;

#if DBG==1 || defined(DUMPTREE)
public:
    int     _nSerialNumber;
    int     SN () const     { return _nSerialNumber; }
#endif


#if DBG==1
private:
    // Debug only mirror of lookaside information, for convenience
    union
    {
        void *              _apLookAside[LOOKASIDE_MARKUP_NUMBER];
        struct
        {
            CCollectionCache *          _pCollectionCacheDbg;
            CMarkup *                   _pParentMarkupDbg;
            CMarkupScriptContext *      _pScriptContextDbg;
            CMarkupBehaviorContext *    _pBehaviorContextDbg;
            CMarkupTextFragContext *    _ptfcDbg;
            CMarkupTopElemCache *       _ptecDbg;
            CStyleSheetArray *          _pStyleSheets;
            CAutoRange *                _pAutoRangeDbg;
#if MARKUP_DIRTYRANGE
            CMarkupDirtyRangeContext *  _pdrcDbg;
#endif
        };
    };
#endif

private:
    NO_COPY(CMarkup);
};

inline HRESULT
CDoc::CreateElement (
    ELEMENT_TAG etag, CElement * * ppElementNew, TCHAR * pch, long cch )
{
    return PrimaryMarkup()->CreateElement( etag, ppElementNew, pch, cch );
}

inline
CMarkup *CDocFrag::Markup()
{
    return CONTAINING_RECORD(this, CMarkup, _OmDoc);
}

inline
CMarkup::CLock::CLock(CMarkup *pMarkup)
{
    Assert(pMarkup);
    _pMarkup = pMarkup;
    pMarkup->AddRef();
}

inline
CMarkup::CLock::~CLock()
{
    _pMarkup->Release();
}

inline CRootElement *
CDoc::PrimaryRoot()
{
    Assert( _pPrimaryMarkup );
    return _pPrimaryMarkup->Root();
}

inline CLayout *
CMarkup::GetRunOwner ( CTreeNode * pNode, CLayout * pLayoutParent /*= NULL*/ )
{
    CTreeNode * pNodeRet = GetRunOwnerBranch(pNode, pLayoutParent);
    if(pNodeRet)
    {
        Assert( pNodeRet->GetUpdatedLayout());

        return pNodeRet->GetUpdatedLayout();
    }
    else
        return NULL;
}

//
// Splay Tree inlines
//

inline CTreePos *
CMarkup::NewTextPos(long cch, SCRIPT_ID sid /* = sidAsciiLatin */, long lTextID /* = 0 */)
{
    CTreePos * ptp;
    if( !lTextID )
        ptp = AllocData1Pos();
    else
        ptp = AllocData2Pos();

    if (ptp)
    {
        Assert( ptp->IsDataPos() );
        ptp->SetType(CTreePos::Text);
        ptp->DataThis()->t._cch = cch;
        ptp->DataThis()->t._sid = sid;
        if( lTextID )
            ptp->DataThis()->t._lTextID = lTextID;
        WHEN_DBG( ptp->_pOwner = this; )
    }

    return ptp;
}

inline CTreePos *
CMarkup::NewPointerPos(CMarkupPointer *pPointer, BOOL fRight, BOOL fStick)
{
#ifdef _WIN64
    CTreePos *ptp = AllocData2Pos();
#else
    CTreePos *ptp = AllocData1Pos();
#endif

    if (ptp)
    {
        Assert( ptp->IsDataPos() );
        ptp->SetType(CTreePos::Pointer);
        Assert( (DWORD_PTR( pPointer ) & 0x3) == 0 );
        ptp->DataThis()->p._dwPointerAndGravityAndCling = (DWORD_PTR)(pPointer) | !!fRight | (fStick ? 0x2 : 0);
        WHEN_DBG( ptp->_pOwner = this; )
    }

    return ptp;
}

inline long
CMarkup::CchInRange(CTreePos *ptpFirst, CTreePos *ptpLast)
{
    Assert(ptpFirst && ptpLast);
    return  ptpLast->GetCp() + ptpLast->GetCch() - ptpFirst->GetCp();
}


inline LOADSTATUS
CDoc::LoadStatus()
{
    Assert (PrimaryMarkup());
    return PrimaryMarkup()->LoadStatus();
}

//+---------------------------------------------------------------------------
//
//  Struct:     CSpliceRecord
//
//  Synposis:   we use this structure to store poslist information from the
//              area that is being spliced out.  This is used internally
//              in SpliceTree and also by undo
//
//----------------------------------------------------------------------------
struct CSpliceRecord
{
    CTreePos::EType _type;

    union
    {
        // Begin/end edge
        struct
        {
            CElement *  _pel;
            long        _cIncl;
            BOOL        _fSkip;
        };
        
        // Text
        struct
        {
            unsigned long   _cch:26;        // [Text] number of characters I own directly
            unsigned long   _sid:6;         // [Text] the script id for this run
            long            _lTextID;       // [Text] Text ID for DOM text nodes
        };

        // Pointer
        struct
        {
            CMarkupPointer *_pPointer;
        };
    };
};

//+---------------------------------------------------------------------------
//
//  Class:      CSpliceRecordList
//
//  Synposis:   A list of above records
//
//----------------------------------------------------------------------------
class CSpliceRecordList : public CDataAry<CSpliceRecord>
{
public:
    DECLARE_MEMALLOC_NEW_DELETE( Mt(CSpliceRecordList) );

    CSpliceRecordList() : CDataAry<CSpliceRecord> ( Mt(CSpliceRecordList_pv) )
        {}

    ~CSpliceRecordList();
};

#pragma INCMSG("--- End 'markup.hxx'")
#else
#pragma INCMSG("*** Dup 'markup.hxx'")
#endif
