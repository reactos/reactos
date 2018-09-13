#ifndef __PEERHAND_HXX__
#define __PEERHAND_HXX__

#ifndef X_ATOMTBL_HXX_
#define X_ATOMTBL_HXX_
#include "atomtbl.hxx"
#endif

interface IScriptletXML;
interface IScriptletHandler;
interface IElementBehavior;
interface IElementBehaviorSite;
interface IElementBehaviorUI;
interface IElementBehaviorSiteOM;
interface ISubDivisionProvider;

class CPeerDispatch;

struct PNODE;

MtExtern(CPeerHandler)
MtExtern(CPeerHandlerConstructor)
MtExtern(CPeerHandlerConstructor_aryEventFlags)

//+------------------------------------------------------------------------
//
//  Class:  CPeerHandlerConstructor
//
//-------------------------------------------------------------------------

class CPeerHandlerConstructor : public CBase
{
#ifdef UNIX // IEUNIX needs this to resolve PFNTEAROFF
    DECLARE_CLASS_TYPES(CPeerHandlerConstructor, CBase)
#endif

public: 

    //
    // constructor / destructor
    //

    CPeerHandlerConstructor();
    ~CPeerHandlerConstructor();

    //
    // IUnknown
    //

    DECLARE_PRIVATE_QI_FUNCS(CPeerHandlerConstructor)

    //
    // IScriptletHandlerConstructor
    //

    DECLARE_TEAROFF_TABLE(IScriptletHandlerConstructor)

    NV_DECLARE_TEAROFF_METHOD(Load,         load, (WORD wFlags, PNODE * pnode));
	NV_DECLARE_TEAROFF_METHOD(Create,       create, (IUnknown *punkContext, IUnknown * punkOuter, IUnknown ** ppunkHandler));
	NV_DECLARE_TEAROFF_METHOD(Register,     register, (LPCOLESTR pstrPath, REFCLSID rclisid, LPCOLESTR pstrProgId)) { RRETURN(E_NOTIMPL); }
	NV_DECLARE_TEAROFF_METHOD(Unregister,   unregister, (REFCLSID rclsid, LPCOLESTR pstrProgId)) { RRETURN(E_NOTIMPL); }
	NV_DECLARE_TEAROFF_METHOD(AddInterfaceTypeInfo, addinterfacetypeinfo, (ICreateTypeLib *ptclib,  ICreateTypeInfo *pctiCoclass, UINT *puiImplIndex)) { RRETURN(E_NOTIMPL); }

    //
    // wiring
    //

    DECLARE_CLASS_TYPES(CPeerHandlerConstructor, CBase)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerHandlerConstructor))

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // helpers
    //

    HRESULT ProcessTree(PNODE * pNode);

    //
    // data
    //

    DECLARE_CPtrAry(CFlagsArray, DWORD, Mt(Mem), Mt(CPeerHandlerConstructor_aryEventFlags));

    CAtomTable  _aryEvents;
    CFlagsArray _aryEventFlags;
    CStr        _cstrName;
};

//+------------------------------------------------------------------------
//
//  Class:  CPeerHandler
//
//-------------------------------------------------------------------------

class CPeerHandler : public CBase
{
    DECLARE_CLASS_TYPES(CPeerHandler, CBase)

public: 

    //
    // construction and destruction
    //

    CPeerHandler(IUnknown *pUnkOuter);
    ~CPeerHandler ();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerHandler))

    //
    // classdescs
    //

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // helpers
    //

    void Passivate();

    inline IElementBehaviorSite * GetSite () {  return _pPeerSite; }

    //
    // IUnknown and IDispatch declarations
    //

    DECLARE_AGGREGATED_IUNKNOWN(CPeerHandler)
    DECLARE_PRIVATE_QI_FUNCS(CPeerHandler)
    DECLARE_DERIVED_DISPATCH(CPeerHandler)

    //
    // IScriptletHandler
    //

    DECLARE_TEAROFF_TABLE(IScriptletHandler)

	NV_DECLARE_TEAROFF_METHOD(GetNameSpaceObject, getnamespaceobject, (IUnknown **ppunk));
	NV_DECLARE_TEAROFF_METHOD(SetScriptNameSpace, setscriptnamespace, (IUnknown *punkNameSpace));

    //
    // IElementBehavior
    //

    DECLARE_TEAROFF_TABLE(IElementBehavior)

	NV_DECLARE_TEAROFF_METHOD(Init, init, (IElementBehaviorSite *pPeerSite));
    NV_DECLARE_TEAROFF_METHOD(Notify, notify, (DWORD, VARIANT *));
    NV_DECLARE_TEAROFF_METHOD(Detach, detach, ()) { return S_OK; };

    //
    // IElementBehaviorUI
    //

    DECLARE_TEAROFF_TABLE(IElementBehaviorUI)

	NV_DECLARE_TEAROFF_METHOD_(BOOL, CanTakeFocus, cantakefocus, ());
	NV_DECLARE_TEAROFF_METHOD(GetSubDivisionProvider, getsubdivisionprovider, (ISubDivisionProvider **pp));
    NV_DECLARE_TEAROFF_METHOD(OnReceiveFocus, onreceivefocus, (BOOL fFocus, long lSubDivision));

    //
    // IPersistPropertyBag2
    //
    
    DECLARE_TEAROFF_TABLE(IPersistPropertyBag2)
    NV_DECLARE_TEAROFF_METHOD(InitNew, initnew, ())
        { return S_OK; }
    NV_DECLARE_TEAROFF_METHOD(Load, load, (IPropertyBag2 *pBag2, IErrorLog *pErrLog));
    NV_DECLARE_TEAROFF_METHOD(Save, save, (IPropertyBag2 *pBag2, BOOL fClearDirty, BOOL fSaveAll))
        { return S_OK; }
    NV_DECLARE_TEAROFF_METHOD(IsDirty, isdirty, ())
        { return S_FALSE; }

    //
    // data
    //

    IUnknown *                  _pUnkOuter;         // Outer unknown
    IDispatch *                 _pScript;           // Both of these are kept around if possible,  
    CPeerDispatch *             _pPeerDisp;         // Event and element object.
    IElementBehaviorSite *      _pPeerSite;         // peer site
    IElementBehaviorSiteOM *    _pPeerSiteOM;       // peer site object model
    IDispatch *                 _pDispNotification; // IDispatch of function in jscript ssyncing IElementBehavior::Notify;
                                                    // initialized by Peer.attachNotification
    BOOL                        _fCanTakeFocus;     // Whether this peer can take focus
};


#endif  // __PEERHAND_HXX__
