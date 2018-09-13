#ifndef __SVROM_HXX__
#define __SVROM_HXX__

interface IScriptletHandler;

MtExtern(CSvrOM)


//+------------------------------------------------------------------------
//
//  Class:  CSvrOM
//
//-------------------------------------------------------------------------

class CSvrOM;
class CSvrOMDisp;

//+------------------------------------------------------------------------
//
//  Class:  CSvrOMDisp
//
//-------------------------------------------------------------------------

class CSvrOMDisp : public IDispatchEx
{
public:
    DECLARE_SUBOBJECT_IUNKNOWN(CSvrOM, SvrOM)
    
    //
    // IDispatch
    //

    STDMETHODIMP GetTypeInfoCount(UINT *pctinfo)
        { return E_NOTIMPL; }
    STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
        { return E_NOTIMPL; }
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
    STDMETHODIMP DeleteMemberByName(BSTR bstr, DWORD grfdex)
        { return S_OK; }
    STDMETHODIMP DeleteMemberByDispID(DISPID id)
        { return S_OK; }
    STDMETHODIMP GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
        { return DISP_E_MEMBERNOTFOUND; }
    STDMETHODIMP GetMemberName(DISPID id, BSTR *pbstrName);
    STDMETHODIMP GetNextDispID(DWORD grfdex, DISPID id, DISPID *pid)
        { return S_FALSE; }
    STDMETHODIMP GetNameSpaceParent(IUnknown **ppunk)
        { *ppunk = NULL; return S_OK; }

    //
    // data
    //

    // the following 2 tables are required to be in the same order
    enum {
        DISPID_SVROM_FIRST          = 5000000,
        DISPID_SVROM_ISAVAILABLE    = DISPID_SVROM_FIRST, 
        DISPID_SVROM_BEGINLOG       = DISPID_SVROM_ISAVAILABLE + 1,
        DISPID_SVROM_TRACELOG       = DISPID_SVROM_BEGINLOG + 1,
        DISPID_SVROM_ENDLOG         = DISPID_SVROM_TRACELOG + 1,
        
        DISPID_COUNT
    };

    inline LPTSTR * GetNamesTable()
    {
        static LPTSTR s_Names[] = {
            _T("isAvailable"),
            _T("beginLog"),
            _T("traceLog"),
            _T("endLog"),
            NULL
        };

        return s_Names;
    };

};


class CSvrOM : public CBase
{
    DECLARE_CLASS_TYPES(CSvrOM, CBase)

public: 

    //
    // construction and destruction
    //

    CSvrOM(IUnknown *pUnkContext, IUnknown *pUnkOuter);

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSvrOM))

    //
    // classdescs
    //

    static const CBase::CLASSDESC s_classdesc;
    const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    virtual void    Passivate();
    
    //
    // IUnknown and IDispatch declarations
    //

    DECLARE_AGGREGATED_IUNKNOWN(CSvrOM)
    DECLARE_PRIVATE_QI_FUNCS(CSvrOM)
    DECLARE_DERIVED_DISPATCH(CSvrOM)

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
	NV_DECLARE_TEAROFF_METHOD(SetScriptNameSpace, setscriptnamespace, (IUnknown *punkNameSpace))
	    { return S_OK; }

    //
    // data
    //

    IUnknown *                  _pUnkOuter;     // Outer unknown
    CSvrOMDisp                  _SvrOMDisp;     // Dispatch for server OM.
    IUnknown *                  _pContext;      // Host context
    IDispatch *                 _pDispSvr;      // IDispatch to the svr obj.
    HANDLE                      _hFileLog;      // Handle to the log file                       
};



#endif  // __HIFITIME_HXX__

