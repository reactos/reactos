#ifndef __HIFITIME_HXX__
#define __HIFITIME_HXX__

#ifndef X_OCMM_H_
#define X_OCMM_H_
#include "ocmm.h"
#endif

interface IScriptletHandler;

MtExtern(CHiFiUses)
MtExtern(CHiFiSink)
MtExtern(CHiFiList__aryHiFi_pv)


//+------------------------------------------------------------------------
//
//  Class:  CHiFiUses
//
//-------------------------------------------------------------------------

class CHiFiDispatch;

class CHiFiUses : public CBase
{
    DECLARE_CLASS_TYPES(CHiFiUses, CBase)

public: 

    //
    // construction and destruction
    //

    CHiFiUses(IUnknown *pUnkContext, IUnknown *pUnkOuter);
    ~CHiFiUses ();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHiFiUses))

    //
    // classdescs
    //

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    //
    // helpers
    //
    HRESULT GetHiFiTimer(IUnknown **ppTimerServices);

    HRESULT Init();
    void Passivate();

    //
    // IUnknown and IDispatch declarations
    //

    DECLARE_AGGREGATED_IUNKNOWN(CHiFiUses)
    DECLARE_PRIVATE_QI_FUNCS(CHiFiUses)
    DECLARE_DERIVED_DISPATCH(CHiFiUses)

    //
    // IScriptletHandlerConstructor
    //

    DECLARE_TEAROFF_TABLE(IScriptletHandlerConstructor)

    NV_DECLARE_TEAROFF_METHOD(Load,         load, (WORD wFlags, IScriptletXML *pxmlElement)) { return S_OK;}
	NV_DECLARE_TEAROFF_METHOD(Create,       create, (IUnknown *punkContext, IUnknown *punkOuter, IUnknown **ppunkHandler));
	NV_DECLARE_TEAROFF_METHOD(Register,     register, (LPCOLESTR pstrPath)) { RRETURN(E_NOTIMPL); }
	NV_DECLARE_TEAROFF_METHOD(Unregister,   unregister, ()) { RRETURN(E_NOTIMPL); }
	NV_DECLARE_TEAROFF_METHOD(AddInterfaceTypeInfo, addinterfacetypeinfo, (ICreateTypeLib *ptclib, ICreateTypeInfo *pctiCoclass))
	    { RRETURN(E_NOTIMPL); }

    //
    // IScriptletHandler
    //

    DECLARE_TEAROFF_TABLE(IScriptletHandler)

	NV_DECLARE_TEAROFF_METHOD(GetNameSpaceObject, getnamespaceobject, (IUnknown **ppunk));
	NV_DECLARE_TEAROFF_METHOD(SetScriptNameSpace, setscriptnamespace, (IUnknown *punkNameSpace));

    //
    // data
    //

    IUnknown *                  _pUnkOuter;     // Outer unknown
    CHiFiDispatch *             _pHiFiDisp;     // Dispatch of HiFiSetTimeout.
    IUnknown *                  _pContext;      // Host context
};


class CHiFiSink : public ITimerSink
{
public :

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CHiFiSink))
    CHiFiSink(IDispatch *pFuncPtr);
    ~CHiFiSink();

    // IUnknown methods
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface (REFIID iid, void **ppvObj);

    // ITimerSink methods
    STDMETHODIMP OnTimer        ( VARIANT vtimeAdvise );

private :
    ULONG                   _ulRefs;
    IDispatch              *_pFuncPtr;
};


//+------------------------------------------------------------------------
//
//  Class:  CHiFiDispatch
//
//-------------------------------------------------------------------------

class CHiFiDispatch : public IDispatchEx
{
public:
    
    //
    // construction / destruction / lifetime
    //

    CHiFiDispatch(CHiFiUses *pHiFiUses);
    virtual ~CHiFiDispatch();

    void Disconnect(); // disconnects CHiFiUses

    //
    // IUnknown
    //

    STDMETHODIMP QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //
    // IDispatch
    //

    STDMETHODIMP GetTypeInfoCount(UINT *pctinfo);
    STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid,
                                     ITypeInfo **ppTInfo);
    STDMETHODIMP GetIDsOfNames(
        REFIID      riid,
        LPOLESTR *  rgszNames,
        UINT        cNames,
        LCID        lcid,
        DISPID *    rgDispId);

    STDMETHODIMP Invoke(
        DISPID          dispid,
        REFIID          riid,
        LCID            lcid,
        WORD            wFlags,
        DISPPARAMS *    pDispParams,
        VARIANT *       pvarResult,
        EXCEPINFO *     pExcepInfo,
        UINT *          puArgErr);

    //
    // IDispatchEx
    //

    STDMETHODIMP GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid);
    STDMETHODIMP InvokeEx(
        DISPID              dispid,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS *        pDispParams,
        VARIANT *           pvarRes,
        EXCEPINFO *         pei,
        IServiceProvider *  pspCaller);
    STDMETHODIMP DeleteMemberByName(BSTR bstr, DWORD grfdex);
    STDMETHODIMP DeleteMemberByDispID(DISPID id);
    STDMETHODIMP GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD *pgrfdex);
    STDMETHODIMP GetMemberName(DISPID id, BSTR *pbstrName);
    STDMETHODIMP GetNextDispID(DWORD grfdex, DISPID id, DISPID *pid);
    STDMETHODIMP GetNameSpaceParent(IUnknown **ppunk);    

    //
    // data
    //

    // the following 2 tables are required to be in the same order
    enum {
        DISPID_ISAVAILABLE  = 0, 
        DISPID_INTERVAL     = 1, 
        DISPID_DELETE       = 2, 
        DISPID_TIME         = 3, 

        DISPID_COUNT
    };

    inline LPTSTR * GetNamesTable()
    {
        static LPTSTR s_Names[] = {
            _T("isAvailable"),
            _T("HiFiInterval"),
            _T("HiFiDelete"),
            _T("HiFiTime"),
            NULL
        };

        return s_Names;
    };

    ITimer * getTimer()
      { return _pTimer; }

private:
    HRESULT SetHiFiTimer(unsigned int interval, IDispatch *pFuncPtr, DWORD *dwCookie);
    HRESULT GetHiFiCurrentTime(VARIANT *pVar);
    void DeleteHiFi(DWORD dwCookie);

    ULONG           _ulRefs;
    CHiFiUses      *_pHiFiUses;

    struct HIFIINFO
    {
        CHiFiSink      *_pSink;
        DWORD           _dwCookie;
        CHiFiDispatch  *_pHiFiDispatch;
    public:
        HIFIINFO(CHiFiDispatch * pHiFiDispatch) :
          _pSink(NULL), _dwCookie(0), _pHiFiDispatch(pHiFiDispatch)
          {  }
        ~HIFIINFO();
    };

    ITimer     *_pTimer;

    DECLARE_CPtrAry(CAryHiFiTimers, HIFIINFO *, Mt(Mem), Mt(CHiFiList__aryHiFi_pv))
    CAryHiFiTimers  _aryHiFi;
};

#endif  // __HIFITIME_HXX__
