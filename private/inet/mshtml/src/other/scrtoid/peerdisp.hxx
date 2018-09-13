#ifndef __PEERDISP_HXX__
#define __PEERDISP_HXX__

class CPeerHandler;

#define DISPID_PEERHAND_BASE    (10000 - 10)


//+------------------------------------------------------------------------
//
//  Class:  CPeerDispatch
//
//-------------------------------------------------------------------------

class CPeerDispatch : public IDispatchEx
{
public:
    
    //
    // construction / destruction / lifetime
    //

    CPeerDispatch(CPeerHandler *pPeerHandler);
    virtual ~CPeerDispatch();

    void Disconnect(); // disconnects CPeerHandler

    //
    // helpers
    //

    HRESULT GetElementDispatchEx(IDispatchEx ** ppDisp);

    HRESULT InternalGetDispID(
        BSTR        bstrName,
        DWORD       grfdex,
        DISPID *    pdispid);

    HRESULT InternalInvoke(
        DISPID              dispid,
        WORD                wFlags,
        DISPPARAMS *        pDispParams,
        VARIANT *           pvarRes,
        EXCEPINFO *         pExcepInfo);

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
    // om
    //

    // the following 2 tables are required to be in the same order
    enum {
        DISPID_PEERELEMENT = 0, 
        DISPID_CREATEEVENTOBJECT,
        DISPID_FIREEVENT,
        DISPID_ATTACHNOTIFICATION,
        DISPID_CANTAKEFOCUS,
        
        DISPID_COUNT
    };

    inline LPTSTR * GetNamesTable()
    {
        static LPTSTR s_Names[] = {
            _T("element"),
            _T("createEventObject"),
            _T("fireEvent"),
            _T("attachNotification"),
            _T("canTakeFocus"),
            NULL
        };

        return s_Names;
    };

    //
    // data
    //

    ULONG           _ulRefs;
    CPeerHandler *  _pPeerHandler;
};

#endif  // __PEERDISP_HXX__
