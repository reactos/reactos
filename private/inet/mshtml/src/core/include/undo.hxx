//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       undo.hxx
//
//  Contents:   Form Undo classes
//
//----------------------------------------------------------------------------

#ifndef I_UNDO_HXX_
#define I_UNDO_HXX_
#pragma INCMSG("--- Beg 'undo.hxx'")

ExternTag(tagUndo);

MtExtern(CUndoManager)
MtExtern(CParentUndoUnit)
MtExtern(CParentUndo)
MtExtern(CUndoPropChange)
MtExtern(CUndoUnitAry)
MtExtern(CUndoUnitAry_pv)

DECLARE_CPtrAry(CUndoUnitAry, IOleUndoUnit *, Mt(CUndoUnitAry), Mt(CUndoUnitAry_pv))

enum UNDOSTATE
{
    UNDO_BASESTATE = 0,  // Undo manager is in the base state
    UNDO_UNDOSTATE = 1,  // Undo manager is in the undo state (doing an undo)
    UNDO_REDOSTATE = 2   // Undo manager is in the redo state (doing a redo)
};


//+---------------------------------------------------------------------------
//
//  Class:      CBlockedParentUnit (BCA)
//
//  Purpose:    Dummy class that we use to block new undo units on the stack.
//
//              NOTE: This class is only intended to be created as a static
//              object. It should never be created on the heap!
//
//----------------------------------------------------------------------------

class CBlockedParentUnit : public IOleParentUndoUnit
{
private:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))
public:
    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppv)  { return E_NOINTERFACE; }
    STDMETHOD_(ULONG, AddRef)(void)                       { return 0; }
    STDMETHOD_(ULONG, Release)(void)                      { return 0; }

    //
    // IOleParentUndoUnit methods
    //
    STDMETHOD(Open)(IOleParentUndoUnit *pPUU)
              { return S_OK; }
    STDMETHOD(Close)(IOleParentUndoUnit *pPUU, BOOL fCommit);
    STDMETHOD(Add)(IOleUndoUnit * pUU)
              { return S_OK; }
    STDMETHOD(FindUnit)(IOleUndoUnit *pUU)
              { return S_FALSE; }
    STDMETHOD(Do)(IOleUndoManager *pUndoManager)
              { return S_OK; }
    STDMETHOD(GetParentState)(DWORD * pdwState)
              { *pdwState = UAS_BLOCKED; return S_OK; }
    STDMETHOD(GetDescription)(BSTR *pbstr)
              { return E_NOTIMPL; }
    STDMETHOD(GetUnitType)(CLSID *pclsid, LONG *plID)
              { return E_NOTIMPL; }
    STDMETHOD(OnNextAdd)(void)
              { return S_OK; }
};

//+---------------------------------------------------------------------------
//
//  Class:      CComposeUndo (CU)
//
//  Purpose:    A class which implements the parent undo methods that are
//              used by the CUndoManager and CUndoParentUnitBase
//              classes.  This consolodates the implementation of the
//              common methods for those two classes.
//
//----------------------------------------------------------------------------

class NOVTABLE CComposeUndo
{
    friend CDoc;
    
private:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))
public:
    CComposeUndo(void);
   ~CComposeUndo(void);

    HRESULT Open(IOleParentUndoUnit *pPUU);
    HRESULT Close(IOleParentUndoUnit *pPUU, BOOL fCommit);
    HRESULT Add(IOleUndoUnit * pUU);

    void    SetBlockedState(BOOL fBlocked);
    void    SetNonEnableState(BOOL fNonEnable);

    //
    // Override OnClose if you need notification when the parent unit
    // is closed.
    //
    virtual HRESULT OnClose() { return S_OK; }

    CUndoUnitAry _aryUndo; // The first element in these arrays is
    CUndoUnitAry _aryRedo; //  the bottom of the stack.

protected:
    HRESULT DoTo(IOleUndoManager *          pUM,
                 CUndoUnitAry *             paryUnit,
                 IOleUndoUnit *             pUU,
                 BOOL                       fDoRollback);

    HRESULT AddUnit(IOleUndoUnit *pUU);
    int     FindChild(CUndoUnitAry & aryUnit, IOleUndoUnit * pUU);

    IOleUndoUnit * GetTopUndoUnit();
    IOleUndoUnit * GetTopRedoUnit();

#define MAX_BLOCK_COUNT   7
#define MAX_STACK_ENTRIES 150

    IOleParentUndoUnit *     _pPUUOpen;

    unsigned                 _UndoState:3;
    unsigned                 _BlockCount:3;
    unsigned                 _NonEnableCount:3;
    unsigned                 _fDisabled:1;
    unsigned                 _fRollbackNeeded:1;
    unsigned                 _fUnitSucceeded:1;
    unsigned                 _fRespectMaxEntries:1;
};

//+---------------------------------------------------------------------------
//
//  Class:      CUndoManager (UM)
//
//  Purpose:    Class which provides the undo manager behavior for the form
//              when the container does not provide it.
//
//----------------------------------------------------------------------------

EXTERN_C const CLSID CLSID_CUndoManager;

class CUndoManager : public IOleUndoManager,
                     public CComposeUndo
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CUndoManager))

    CUndoManager::CUndoManager();

    DECLARE_FORMS_STANDARD_IUNKNOWN(CUndoManager)

    //
    // IOleUndoManager methods (delegated to CComposeUndo)
    //
    STDMETHOD(Open)(IOleParentUndoUnit *pPUU)
             { return CComposeUndo::Open(pPUU); }
    STDMETHOD(Close)(IOleParentUndoUnit *pPUU, BOOL fCommit)
             { return CComposeUndo::Close(pPUU, fCommit); }
    STDMETHOD(Add)(IOleUndoUnit * pUU)
             { return CComposeUndo::Add(pUU); }

    //
    // IOleUndoManager methods
    //
    STDMETHOD(GetOpenParentState)(DWORD * pdwState);
    STDMETHOD(DiscardFrom)(IOleUndoUnit * pUU);
    STDMETHOD(UndoTo)(IOleUndoUnit * pUU);
    STDMETHOD(RedoTo)(IOleUndoUnit * pUU);
    STDMETHOD(EnumUndoable)(IEnumOleUndoUnits **ppEnum);
    STDMETHOD(EnumRedoable)(IEnumOleUndoUnits **ppEnum);
    STDMETHOD(GetLastUndoDescription)(BSTR *pbstr);
    STDMETHOD(GetLastRedoDescription)(BSTR *pbstr);
    STDMETHOD(Enable)(BOOL fEnable);

protected:
    HRESULT GetDescr(IOleUndoUnit *pUU, BSTR *pbstr);
};

//+---------------------------------------------------------------------------
//
//  Class:      CUndoUnit (UA)
//
//  Purpose:    A class which implements the common methods of IOleUndoUnit
//              and IOleParentUndoUnit for which we want the same
//              implementation.  CUndoUnitBase and CParentUnitBase
//              inherit from this class.
//
//----------------------------------------------------------------------------

class CUndoUnit
{
private:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))
public:
    CUndoUnit(CBase * pBase, UINT uiStringID);
    CUndoUnit(CBase * pBase, BSTR bstrDescription);
    
    ~CUndoUnit();

    //
    // IOleUndoUnit methods (not virtual)
    //

    HRESULT GetDescription(BSTR *pbstr);
    HRESULT GetUnitType(CLSID *pclsid, LONG *plID);

protected:
    CBase *  _pBase;            // Un-addref'd pointer
    UINT     _uiResID;          // Resource ID for descr. Also used as identifier
                                // for GetUnitType.
    TCHAR *  _pchDescription;   // Undo Description String set by BeginUndoUnit()
};


//+---------------------------------------------------------------------------
//
//  Class:      CUndoUnitBase (UAB)
//
//  Purpose:    Base class for all simple undoable units.
//
//----------------------------------------------------------------------------

class NOVTABLE CUndoUnitBase : public IOleUndoUnit,
                        public CUndoUnit
{
private:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))
public:
    CUndoUnitBase(CBase * pBase, UINT uiStringID);
    virtual ~CUndoUnitBase(void) {}

    DECLARE_FORMS_STANDARD_IUNKNOWN(CUndoUnitBase)

    // This is the method to be overridden.
    virtual HRESULT PrivateDo(IOleUndoManager *pUndoManager) = 0;

    //
    // IOleUndoUnit methods
    //
    STDMETHOD(Do)(IOleUndoManager *pUndoManager);

    STDMETHOD(GetDescription)(BSTR *pbstr)
              { return CUndoUnit::GetDescription(pbstr); }
    STDMETHOD(GetUnitType)(CLSID *pclsid, LONG *plID)
              { return CUndoUnit::GetUnitType(pclsid, plID); }
    STDMETHOD(OnNextAdd)(void)
              { return S_OK; }

protected:
    BOOL _fUndo;    // vs. Redo
};

//+---------------------------------------------------------------------------
//
//  Class:      CParentUnitBase (CUAB)
//
//  Purpose:    Base class for all parent undo units
//
//  Notes:      The _aryRedo array is not used by this class, and can in fact
//              be used as a scratch array by derived classes. The _UndoState
//              member should never be changed from UNDO_BASESTATE, however.
//
//----------------------------------------------------------------------------

class CParentUnitBase : public IOleParentUndoUnit,
                                public CUndoUnit,
                                public CComposeUndo
{
private:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))
public:
    CParentUnitBase(CBase * pBase, UINT uiStringID);
    CParentUnitBase(CBase * pBase, BSTR bstrDescription);
    virtual ~CParentUnitBase(void);

    DECLARE_FORMS_STANDARD_IUNKNOWN(CParentUnitBase)

    //
    // Parent undo methods (delegated to CComposeUndo)
    //
    STDMETHOD(Open)(IOleParentUndoUnit *pPUU)
              { return CComposeUndo::Open(pPUU); }
    STDMETHOD(Close)(IOleParentUndoUnit *pPUU, BOOL fCommit)
              { return CComposeUndo::Close(pPUU, fCommit); }
    STDMETHOD(Add)(IOleUndoUnit * pUU)
              { return CComposeUndo::Add(pUU); }
    STDMETHOD(FindUnit)(IOleUndoUnit *pUU);

    //
    // IOleParentUndoUnit methods
    //

    STDMETHOD(Do)(IOleUndoManager *pUndoManager) = 0;

    STDMETHOD(GetParentState)(DWORD * pdwState);

    STDMETHOD(GetDescription)(BSTR *pbstr)
              { return CUndoUnit::GetDescription(pbstr); }
    STDMETHOD(GetUnitType)(CLSID *pclsid, LONG *plID)
              { return CUndoUnit::GetUnitType(pclsid, plID); }
    STDMETHOD(OnNextAdd)(void)
              { return S_OK; }
};

//+---------------------------------------------------------------------------
//
//  Class:      CParentUndoUnit (PUU)
//
//  Purpose:    A basic parent unit that does nothing but contain other
//              undo units and delegates to their Do() methods.
//
//----------------------------------------------------------------------------

class CParentUndoUnit : public CParentUnitBase
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CParentUndoUnit))
    CParentUndoUnit(CBase * pBase, UINT uiStringID);
    CParentUndoUnit(CBase * pBase, BSTR bstrDescription);

    STDMETHOD(Do)(IOleUndoManager *pUndoManager);
};

//+---------------------------------------------------------------------------
//
//  Class:      CParentUndo (PU)
//
//  Purpose:    A helper class to put a parent undo unit on the undo stack
//
//----------------------------------------------------------------------------

class CParentUndo
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CParentUndo))

    CParentUndo(CBase * pDoc) : _pBase( pDoc ), _puu( NULL ) {}
#if DBG==1
    ~CParentUndo() { AssertSz( !_puu, "CParentUndo dtor without Finish called" ); }
#endif

    HRESULT Start(UINT uiStringID);
    HRESULT Start(TCHAR * pchDescription);

    HRESULT Finish(HRESULT hrCommit);

private:
    CBase *           _pBase;
    CParentUndoUnit * _puu;
};

//+---------------------------------------------------------------------------
//
//  Class:      CUndoPropChange (UPC)
//
//  Purpose:    Generic class to undo most property changes.  Property
//              changes not supported by this class will need custom undo
//              unit objects.
//
//----------------------------------------------------------------------------

class CUndoPropChange : public CUndoUnitBase
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CUndoPropChange))
    CUndoPropChange(CBase * pBase,
                    UINT    uiStringID);

   ~CUndoPropChange();

    HRESULT Init(DISPID dispidProp, VARIANT* pVar);

    HRESULT PrivateDo(IOleUndoManager *pUndoManager);

protected:

    DISPID     _dispid;     // DispID for property.
    VARIANT    _varData;    // Current Data.
};

#pragma INCMSG("--- End 'undo.hxx'")
#else
#pragma INCMSG("*** Dup 'undo.hxx'")
#endif
