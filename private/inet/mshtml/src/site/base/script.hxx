//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       script.hxx
//
//  Contents:   Script classes (public)
//
//-------------------------------------------------------------------------

#ifndef I_SCRIPT_HXX_
#define I_SCRIPT_HXX_
#pragma INCMSG("--- Beg 'script.hxx'")

#ifndef X_ACTIVDBG_H_
#define X_ACTIVDBG_H_
#pragma INCMSG("--- Beg <activdbg.h>")
#include <activdbg.h>
#pragma INCMSG("--- End <activdbg.h>")
#endif

//
// Forward decls.
//

class CDoc;
class CHtmCtx;
class CScriptCollection;
class CScriptHolder;
class CScriptDebugDocument;

#define DEFAULT_OM_SCOPE              _T("window")
#define NO_SOURCE_CONTEXT 0xffffffff

/*
#ifndef NO_SCRIPT_DEBUGGER
//+------------------------------------------------------------------------
//
//  Class:      CScriptDebugHost
//
//  Purpose:    Implements the interfaces used by the script debugger.
//              This is a secondary object of CScriptCollection.
//
//-------------------------------------------------------------------------

class CScriptDebugHost :
    public IDebugDocumentHost,
    public IRemoteDebugApplicationEvents
{
public:
    DECLARE_SUBOBJECT_IUNKNOWN(CScriptCollection, ScriptCollection);

    // IDebugDocumentHost methods
    STDMETHOD(GetDeferredText)( DWORD dwTextStartCookie, WCHAR *pcharText,SOURCE_TEXT_ATTR *pstaTextAttr, ULONG *pcNumChars, ULONG cMaxChars );
    STDMETHOD(GetScriptTextAttributes)( LPCOLESTR pstrCode,ULONG uNumCodeChars,LPCOLESTR pstrDelimiter, DWORD dwFlags, SOURCE_TEXT_ATTR *pattr )  { return E_NOTIMPL; }
    STDMETHOD(OnCreateDocumentContext)( IUnknown** ppunkOuter )  { return E_NOTIMPL; }
    STDMETHOD(GetPathName)( BSTR *pbstrLongName, BOOL *pfIsOriginalFile);
    STDMETHOD(GetFileName)( BSTR *pbstrShortName);
    STDMETHOD(NotifyChanged)() { return E_NOTIMPL; }

    // IRemoteDebugApplicationEvents methods
    STDMETHOD(OnConnectDebugger)( IApplicationDebugger *pad );
    STDMETHOD(OnDisconnectDebugger)();
    STDMETHOD(OnSetName)( LPCOLESTR pstrName );
    STDMETHOD(OnDebugOutput)( LPCOLESTR pstr );
    STDMETHOD(OnClose)();
    STDMETHOD(OnEnterBreakPoint)( IRemoteDebugApplicationThread *prdat );
    STDMETHOD(OnLeaveBreakPoint)( IRemoteDebugApplicationThread *prdat );
    STDMETHOD(OnCreateThread)( IRemoteDebugApplicationThread *prdat );
    STDMETHOD(OnDestroyThread)( IRemoteDebugApplicationThread *prdat );
    STDMETHOD(OnBreakFlagChange)( APPBREAKFLAGS abf, IRemoteDebugApplicationThread *prdatSteppingThread );
};
#endif // ndef NO_SCRIPT_DEBUGGER
*/

//+------------------------------------------------------------------------
//
//  Class:      CNamedItemsTable, CNamedItem
//
//-------------------------------------------------------------------------

MtExtern(CNamedItemsTable_CItemsArray);
  
class CNamedItem
{
public:

    CNamedItem(LPTSTR pchName, IUnknown * pUnkItem)
    {
        _cstrName.Set(pchName);
        _pUnkItem = pUnkItem;
        _pUnkItem->AddRef();
    }
    ~CNamedItem()
    {
        _pUnkItem->Release();
    }

    CStr        _cstrName;
    IUnknown *  _pUnkItem;
};

DECLARE_CPtrAry(CNamedItemsArray, CNamedItem*,  Mt(Mem), Mt(CNamedItemsTable_CItemsArray));

class CNamedItemsTable : public CNamedItemsArray
{
public:
    DECLARE_CLASS_TYPES(CNamedItemsTable, CNamedItemsArray)

    HRESULT AddItem(LPTSTR pchName, IUnknown * pUnkItem);
    HRESULT GetItem(LPTSTR pchName, IUnknown ** ppUnkItem);
    HRESULT FreeAll();
};

//+------------------------------------------------------------------------
//
//  Class:      CScriptMethodsTable, SCRIPTMETHOD
//
//-------------------------------------------------------------------------

MtExtern(CScriptCollection_CScriptMethodsArray);

struct SCRIPTMETHOD
{
    DISPID          dispid;
    CScriptHolder * pHolder;
    CStr            cstrName;
};

DECLARE_CDataAry(CScriptMethodsArray, SCRIPTMETHOD, Mt(Mem), Mt(CScriptCollection_CScriptMethodsArray))

class CScriptMethodsTable : public CScriptMethodsArray
{
public:
    CScriptMethodsTable() {};
    ~CScriptMethodsTable();
};

//+------------------------------------------------------------------------
//
//  Class:      CScriptCollection
//
//  Purpose:    Contains a number of script holders for different langs
//              There is one of these per doc.
//
//-------------------------------------------------------------------------

MtExtern(CScriptCollection)

class CScriptCollection
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CScriptCollection))
    CScriptCollection();
    HRESULT     Init( CDoc *pDoc );
    
    void        Deinit();
#ifndef NO_SCRIPT_DEBUGGER
    void        DeinitDebugger();
#endif // NO_SCRIPT_DEBUGGER

    CDoc *      Doc() { return(_pDoc); }

    // IUnknown methods
    STDMETHOD_(ULONG, AddRef)() { SubAddRef(); return ++_ulRefs; }

    STDMETHOD_(ULONG, Release)();

    ULONG       SubAddRef() { return ++_ulAllRefs; }
    ULONG       SubRelease();
    ULONG       GetObjectRefs() { return _ulRefs; }

    HRESULT     AddNamedItem(CElement *pElement);
    HRESULT     AddNamedItem(LPTSTR pchName, IUnknown * pUnkItem, BOOL fExcludeParseProcedureEngines = FALSE);
    
    HRESULT     SetState(SCRIPTSTATE ss);
    HRESULT     AddHolderForObject(CBase *pBase, IActiveScript *pScript, CLSID *pClsid);
    HRESULT     RemoveHolderForObject(CBase *pBase);

    HRESULT     GetHolderForLanguageHelper(
        LPTSTR              pchLanguage, 
        CMarkup *           pMarkup,
        LPTSTR              pchType,
        CScriptHolder **    ppHolder);

    HRESULT     GetHolderForLanguage(
        LPTSTR              pchLanguage, 
        CMarkup *           pMarkup,
        LPTSTR              pchType,
        LPTSTR              pchCode, 
        CScriptHolder **    ppHolder,
        LPTSTR  *           ppchCleanCode = NULL);

    HRESULT     AddScriptlet(
        LPTSTR              pchLanguage,
        CMarkup *           pMarkup,
        LPTSTR              pchType,
        LPTSTR              pchCode,
        LPTSTR              pchItemName,
        LPTSTR              pchSubItemName,
        LPTSTR              pchEventName,
        LPTSTR              pchDelimiter,
        ULONG               ulOffset,
        ULONG               ulStartingLine,
        CBase *             pSourceObject,
        DWORD               dwFlags,
        BSTR *              pbstrName);

    HRESULT     ParseScriptText(
        LPTSTR              pchLanguage,
        CMarkup *           pMarkup,
        LPTSTR              pchType,
        LPTSTR              pchCode,
        LPTSTR              pchItemName,
        LPTSTR              pchDelimiter,
        ULONG               ulOffset,
        ULONG               ulStartingLine,
        CBase *             pSourceObject,
        DWORD               dwFlags,
        VARIANT *           pvarResult,
        EXCEPINFO *         pexcepinfo);

    HRESULT     ConstructCode (
        LPTSTR              pchScope,
        LPTSTR              pchCode,
        LPTSTR              pchFormalParams,
        LPTSTR              pchLanguage,
        CMarkup *           pMarkup,
        LPTSTR              pchType,
        ULONG               ulOffset,
        ULONG               ulStartingLine,
        CBase *             pSourceObject,
        DWORD               dwFlags,
        IDispatch **        pDispCode,
        BOOL                fSingleLine);

    //
    // Misc helpers
    //
    
    long GetScriptDispatch(TCHAR *pchName, CPtrAry<IDispatch *> *pary);

    HRESULT GetDispID(
        CMarkup *               pMarkup,
        BSTR                    bstrName,
        DWORD                   grfdex,
        DISPID *                pdispid);

    HRESULT InvokeEx(
        CMarkup *               pMarkup,
        DISPID                  dispid,
        LCID                    lcid,
        WORD                    wFlags,
        DISPPARAMS *            pDispParams,
        VARIANT *               pvarRes,
        EXCEPINFO *             pExcepInfo,
        IServiceProvider *      pServiceProvider);
    
    HRESULT InvokeName(
        CMarkup *               pMarkup,
        LPTSTR                  pchName,
        LCID                    lcid,
        WORD                    wFlags,
        DISPPARAMS *            pDispParams,
        VARIANT *               pvarRes,
        EXCEPINFO *             pExcepInfo,
        IServiceProvider *      pSrvProvider);

    //
    // handling script errors with or without script debugger
    //

    // (this method is used even if script debugger is not installed)
    HRESULT CreateSourceContextCookie(
        IActiveScript *     pActiveScript, 
        LPTSTR              pchCode,
        ULONG               ulOffset, 
        BOOL                fScriptlet, 
        CBase *             pSourceObject,
        DWORD               dwFlags,
        DWORD *             pdwSourceContextCookie);

#ifndef NO_SCRIPT_DEBUGGER

    HRESULT ViewSourceInDebugger(
        ULONG               ulLine = 0, 
        ULONG               ulOffsetInLine = 0);

#endif // NO_SCRIPT_DEBUGGER

    //
    // misc
    //

    BOOL        IsSafeToRunScripts(CLSID *pClsid, IUnknown *pUnk);
    
    //
    //  class CDebugDocumentStack
    //

    class CDebugDocumentStack
    {
    public:
        CDebugDocumentStack(CScriptCollection * pScriptCollection);
        ~CDebugDocumentStack();

        CScriptCollection *         _pScriptCollection;
        CScriptDebugDocument *      _pDebugDocumentPrevious;
    };

    //
    // Data Members
    //

    ULONG                       _ulRefs;
    ULONG                       _ulAllRefs;
    CDoc *                      _pDoc;
    SCRIPTSTATE                 _ss;
    CPtrAry<CScriptHolder *>    _aryHolder;
    CNamedItemsTable            _NamedItemsTable;

    ULONG                       _cIncludes;

    CScriptDebugDocument *      _pCurrentDebugDocument;
    
    // Bitflags
    BOOL                        _fDebuggerAttached : 1;
    BOOL                        _fInEnginesGetDispID : 1;

private:
    ~CScriptCollection();
};

#pragma INCMSG("--- End 'script.hxx'")
#else
#pragma INCMSG("*** Dup 'script.hxx'")
#endif
