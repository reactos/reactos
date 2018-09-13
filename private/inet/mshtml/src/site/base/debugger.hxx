//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1999
//
//  File:       debugger.hxx
//
//  Contents:   Script Debugger related code
//
//----------------------------------------------------------------------------

#ifndef I_DEBUGGER_HXX_
#define I_DEBUGGER_HXX_
#pragma INCMSG("--- Beg 'debugger.hxx'")

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

/////////////////////////////////////////////////////////////////////////////////
//
// misc
//
/////////////////////////////////////////////////////////////////////////////////

MtExtern(CScriptCookieTable)
MtExtern(CScriptCookieTable_CItemsArray)

MtExtern(CScriptDebugDocument)

/////////////////////////////////////////////////////////////////////////////////
//
// Stateless helpers
//
/////////////////////////////////////////////////////////////////////////////////

HRESULT GetScriptDebugDocument (CBase * pSourceObject, CScriptDebugDocument ** ppScriptDebugDocument);

/////////////////////////////////////////////////////////////////////////////////
//
// Class:   CScriptCookieTable
//
/////////////////////////////////////////////////////////////////////////////////

class CScriptCookieTable : public CVoid
{
    DECLARE_CLASS_TYPES(CScriptCookieTable, CVoid)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CScriptCookieTable))

    //
    // data definitions
    //

    enum ITEMTYPE
    {
        ITEMTYPE_NULL = 0,
        ITEMTYPE_SCRIPTELEMENT,
        ITEMTYPE_MARKUP,
        ITEMTYPE_REF
    };

    class CItem
    {
    public:
    
        inline BOOL IsMatch(DWORD dwCookie, CBase * pSourceObject = NULL)
        {
            if (pSourceObject)
            {
                return (_dwCookie == dwCookie &&
                        ITEMTYPE_NULL != _type &&
                        ITEMTYPE_REF  != _type &&
                        _pSourceObject == pSourceObject);
            }
            else
            {
                return (_dwCookie == dwCookie &&
                        ITEMTYPE_NULL != _type);
            }
        };

        DWORD                   _dwCookie;
        ITEMTYPE                _type;
        union
        {
            CBase *             _pSourceObject;
            CMarkup *           _pMarkup;
            CScriptElement *    _pScriptElement;
            DWORD               _dwCookieRef;
        };
    };

    DECLARE_CDataAry(CItemsArray, CItem, Mt(Mem), Mt(CScriptCookieTable_CItemsArray));

    //
    // methods
    //

    HRESULT  CreateCookieForSourceObject (DWORD * pdwCookie, CBase * pSourceObject);
    HRESULT  MapCookieToSourceObject     (DWORD dwCookie, CBase * pSourceObject);
    CItem *  FindItem                    (DWORD dwCookie, CBase * pSourceObject = NULL);
    CItem *  FindItemDerefed             (DWORD dwCookie, CBase * pSourceObject = NULL);
    HRESULT  RevokeSourceObject          (DWORD dwCookie, CBase * pSourceObject);

    HRESULT  GetSourceObjects            (DWORD dwCookie, CMarkup **              ppMarkup,
                                                          CScriptElement **       ppScriptElement = NULL,
                                                          CScriptDebugDocument ** ppScriptDebugDocument = NULL);

    HRESULT  GetScriptDebugDocument      (DWORD dwCookie, CScriptDebugDocument ** ppScriptDebugDocument);

    HRESULT  GetScriptDebugDocument      (CItem * pItem,  CScriptDebugDocument ** ppScriptDebugDocument);

    HRESULT  GetScriptDebugDocumentContext(
                DWORD                       dwCookie,
                ULONG                       uCharacterOffset,
                ULONG                       uNumChars,
                IDebugDocumentContext **    ppsc);

    //
    // data
    //

    CItemsArray     _aryItems;
};

/////////////////////////////////////////////////////////////////////////////////
//
// Class:   CScriptDebugDocument
//
// NOTE:    the class has to be derived from CBaseFT because some of it methods
//          are free-threaded (examples: GetDeferredText; CHost::AddRef/CHost::Release)
//
/////////////////////////////////////////////////////////////////////////////////

class CScriptDebugDocument : public CBaseFT
{
    DECLARE_CLASS_TYPES(CScriptDebugDocument, CBaseFT)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CScriptDebugDocument))

    //
    // CCreateInfo
    //

    class CCreateInfo
    {
    public:
        CCreateInfo(CDoc * pDoc, LPTSTR pchUrl, CHtmCtx * pMarkupHtmCtx)
        {
            _pDoc                   = pDoc;
            _pchUrl                 = pchUrl;
            _pMarkupHtmCtx          = pMarkupHtmCtx;
            _pchScriptElementCode   = NULL;
        }

        CCreateInfo(CDoc * pDoc, LPTSTR pchUrl, LPTSTR pchScriptElementCode)
        {
            _pDoc                   = pDoc;
            _pchUrl                 = pchUrl;
            _pMarkupHtmCtx          = NULL;
            _pchScriptElementCode   = pchScriptElementCode;
        }

        CDoc *      _pDoc;
        LPTSTR      _pchUrl;
        CHtmCtx *   _pMarkupHtmCtx;
        LPTSTR      _pchScriptElementCode;
    };

    //
    // methods
    //

    CScriptDebugDocument();
    ~CScriptDebugDocument();

    void Passivate();

    static HRESULT Create(CCreateInfo * pInfo, CScriptDebugDocument ** ppScriptDebugDocument);

    HRESULT Init(CCreateInfo * pInfo);
    HRESULT Done();

    BOOL IllegalCall();

    void GetDebugHelper     (IDebugDocumentHelper ** ppDebugHelper);
    void ReleaseDebugHelper (IDebugDocumentHelper *  pDebugHelper);

    HRESULT DefineScriptBlock(
                IActiveScript *     pActiveScript, 
                ULONG               ulOffset, 
                ULONG               ulCodelen, 
                BOOL                fScriptlet, 
                DWORD *             pdwScriptCookie);

    HRESULT GetDocumentContext(
                DWORD                       dwCookie,
                ULONG                       uCharacterOffset,
                ULONG                       uNumChars,
                IDebugDocumentContext **    ppDebugDocumentContext);

    HRESULT ViewSourceInDebugger (ULONG ulLine, ULONG ulOffsetInLine);

    HRESULT SetDocumentSize(ULONG ulSize);
    HRESULT RequestDocumentSize(ULONG ulSize);
    HRESULT UpdateDocumentSize();

    //+--------------------------------------------------------------------------
    //
    // Subclass:    CHost
    //
    //---------------------------------------------------------------------------

    class CHost : public IDebugDocumentHost
    {
    public:

        inline CScriptDebugDocument * SDD()
        {
            return CONTAINING_RECORD(this, CScriptDebugDocument, _Host);
        }

        //
        // IUnknown
        //

        STDMETHOD(QueryInterface) (REFIID riid, void ** ppv);
        STDMETHOD_(ULONG, AddRef) ()  { return SDD()->SubAddRef(); }
        STDMETHOD_(ULONG, Release) () { return SDD()->SubRelease(); }

        //
        // IDebugDocumentHost
        //

        STDMETHOD(GetDeferredText)( DWORD dwTextStartCookie, WCHAR *pcharText,SOURCE_TEXT_ATTR *pstaTextAttr, ULONG *pcNumChars, ULONG cMaxChars );
        STDMETHOD(GetScriptTextAttributes)( LPCOLESTR pstrCode,ULONG uNumCodeChars,LPCOLESTR pstrDelimiter, DWORD dwFlags, SOURCE_TEXT_ATTR *pattr )  { return E_NOTIMPL; }
        STDMETHOD(OnCreateDocumentContext)( IUnknown** ppunkOuter )  { return E_NOTIMPL; }
        STDMETHOD(GetPathName)(BSTR *pbstrLongName, BOOL *pfIsOriginalFile);
        STDMETHOD(GetFileName)(BSTR *pbstrShortName);
        STDMETHOD(NotifyChanged)() { return E_NOTIMPL; }

        //
        // data
        //

#if DBG == 1
        CScriptDebugDocument *  _pScriptDebugDocumentDbg;
#endif
    };

    //
    // data
    //

    CStr                        _cstrUrl;

    CHtmCtx *                   _pMarkupHtmCtx;
    CStr                        _cstrScriptElementCode;

    DWORD                       _dwThreadId;

    CHost                       _Host;

    IDebugDocumentHelper *      _pDebugHelper;
    int                         _cDebugHelperAccesses;
    CCriticalSection            _DebugHelperCriticalSection;

    ULONG                       _ulCurrentSize;
    ULONG                       _ulNewSize;
};

#pragma INCMSG("--- End 'debugger.hxx'")
#else
#pragma INCMSG("*** Dup 'debugger.hxx'")
#endif // I_DEBUGGER_HXX_
