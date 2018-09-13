//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       sholder.hxx
//
//  Contents:   Script holder classes (private)
//
//-------------------------------------------------------------------------

#ifndef I_SHOLDER_HXX_
#define I_SHOLDER_HXX_
#pragma INCMSG("--- Beg 'sholder.hxx'")

#ifndef X_ACTIVDBG_H_
#define X_ACTIVDBG_H_
#pragma INCMSG("--- Beg <activdbg.h>")
#include <activdbg.h>
#pragma INCMSG("--- End <activdbg.h>")
#endif

//
// Forward decls
//

class CScriptCollection;
class CScriptConst;

MtExtern(CScriptHolder)

//+------------------------------------------------------------------------
//
//  Class:      CScriptHolder
//
//  Purpose:    The primary holder/site for a script engine
//
//-------------------------------------------------------------------------

class CScriptHolder :
    public IActiveScriptSite, 
    public IActiveScriptSiteWindow,
#ifndef NO_SCRIPT_DEBUGGER
    public IActiveScriptSiteDebug,
#endif
    public IActiveScriptSiteInterruptPoll,
    public IServiceProvider,
    public IOleCommandTarget
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CScriptHolder))
    CScriptHolder(CScriptCollection *pCollection);
    ~CScriptHolder();

    HRESULT Close();
    HRESULT Init(
        CBase *pBase,
        IActiveScript *pScript,
        IActiveScriptParse *pScriptParse,
        CLSID *pclsid);
    HRESULT SetScriptState(SCRIPTSTATE ss);

    HRESULT GetDebug(IActiveScriptDebug **ppDebug);
    // IUnknown methods

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID, void **);

    // IActiveScriptSite methods

    STDMETHOD(GetLCID)(LCID *plcid);
    STDMETHOD(GetItemInfo)(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti);
    STDMETHOD(GetDocVersionString)(BSTR *pszVersion);
    STDMETHOD(RequestItems)(void);
    STDMETHOD(RequestTypeLibs)(void);
    STDMETHOD(OnScriptTerminate)(const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo);
    STDMETHOD(OnStateChange)(SCRIPTSTATE ssScriptState);
    STDMETHOD(OnScriptError)(IActiveScriptError *pscripterror);
    STDMETHOD(OnEnterScript)(void);
    STDMETHOD(OnLeaveScript)(void);
    STDMETHOD(OnScriptErrorDebug)(IActiveScriptErrorDebug* pErrorDebug, BOOL *pfEnterDebugger, BOOL *pfCallOnScriptErrorWhenContinuing);

#ifndef NO_SCRIPT_DEBUGGER
    // IActiveScriptSiteDebug methods
    STDMETHOD(GetDocumentContextFromPosition)(DWORD dwSourceContext, ULONG uCharacterOffset, ULONG uNumChars,IDebugDocumentContext **ppsc);
    STDMETHOD(GetRootApplicationNode)( IDebugApplicationNode **ppdan );
    STDMETHOD(EnterBreakPoint)( void );
    STDMETHOD(LeaveBreakPoint)( void );
    STDMETHOD(GetApplication)( IDebugApplication **ppda );
#endif
    // IActiveScriptSiteWindow methods
    STDMETHOD(GetWindow)(HWND *phwnd);
    STDMETHOD(EnableModeless)(BOOL fEnable);

    // IActiveScriptSiteInterruptPoll methods
    STDMETHOD(QueryContinue)(void);

    // IServiceProvider methods
    STDMETHOD(QueryService)             (
                REFGUID sid, 
                REFIID iid, 
                LPVOID * ppv);

    STDMETHOD(QueryStatus)(
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext);

    STDMETHOD(Exec)(
        const GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut);

    BOOL    TestLock(WORD wLockFlags)
        {   return wLockFlags & _wLockFlags; }
    BOOL    IllegalCall();

    HRESULT DoYouWishToDebug(IActiveScriptError * pScriptError, BOOL * pfEnterDebugger);

    enum SCRIPTLOCK_FLAG
    {
        SCRIPTLOCK_FIREONERROR  =   1,  // In the middle of fire_onerror
        SCRIPTLOCK_SCRIPTERROR  =   SCRIPTLOCK_FIREONERROR << 1, // In OnScriptError
	SCRIPTLOCK_FLAG_Last_Enum
    };
    
    class CLock
    {
    public:
        CLock(CScriptHolder *pHolder, WORD wLock);
        ~CLock();

        CScriptHolder * _pHolder;
        WORD            _wLock;
    };
    
    // Data members
    ULONG                           _ulRefs;
    CBase *                         _pBase;     
    CScriptCollection *             _pCollection;
    IActiveScript *                 _pScript;
    IActiveScriptParse *            _pScriptParse;
    IActiveScriptDebug *            _pScriptDebug;
    CLSID                           _clsid;
    IActiveScriptParseProcedure *   _pParseProcedure;
    WORD                            _wLockFlags;
    ULONG                           _ulDocSize;                 // _ulOffset + length of script

    // Bitfields
    unsigned                        _fCaseSensitive:1;

private:
    void TurnOnFastSinking();
    void SetConvertionLocaleToENU();
};

#pragma INCMSG("--- End 'sholder.hxx'")
#else
#pragma INCMSG("*** Dup 'sholder.hxx'")
#endif
