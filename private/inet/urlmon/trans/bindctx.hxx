//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       bindctx.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9-16-96   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#ifndef BINDCTX_HXX
#define BINDCTX_HXX


// These two structures are used to pass data from the inloader callbacks
// to the wndproc of the hidden window in the main thread.

class CTransaction;
class CTransData;
class CBindCtx;

class CBindCtx : public IBindCtx, public IMarshal
{
private:
    CBindCtx(IBindCtx * pbcRem);

public:
    static HRESULT Create(CBindCtx **ppCBCtx, IBindCtx *pbc = NULL);
    ~CBindCtx();

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** IBindCtx methods ***
    STDMETHODIMP RegisterObjectBound(IUnknown *punk)
    {   return _pbcLocal->RegisterObjectBound(punk); }

    STDMETHODIMP RevokeObjectBound(IUnknown *punk)
    {   return _pbcLocal->RevokeObjectBound(punk); }

    STDMETHODIMP ReleaseBoundObjects(void)
    {   return _pbcLocal->ReleaseBoundObjects(); }

    STDMETHODIMP SetBindOptions(BIND_OPTS *pbindopts)
    {   return _pbcLocal->SetBindOptions(pbindopts); }

    STDMETHODIMP GetBindOptions(BIND_OPTS *pbindopts)
    {   return _pbcLocal->GetBindOptions(pbindopts); }

    STDMETHODIMP GetRunningObjectTable(IRunningObjectTable **pprot)
    {   return _pbcLocal->GetRunningObjectTable(pprot); }

    STDMETHODIMP RegisterObjectParam(LPOLESTR pszKey, IUnknown *punk)
    {   return _pbcLocal->RegisterObjectParam(pszKey, punk); }

    STDMETHODIMP GetObjectParam(LPOLESTR pszKey, IUnknown **ppunk);
    STDMETHODIMP EnumObjectParam(IEnumString **ppenum);
    STDMETHODIMP RevokeObjectParam(LPOLESTR pszKey);


    // *** IMarshal methods ***
    STDMETHODIMP GetUnmarshalClass(REFIID riid, void *pvInterface, DWORD dwDestContext,
                        void *pvDestContext, DWORD mshlflags, CLSID *pCid);
    STDMETHODIMP GetMarshalSizeMax(REFIID riid, void *pvInterface, DWORD dwDestContext,
                        void *pvDestContext, DWORD mshlflags, DWORD *pSize);
    STDMETHODIMP MarshalInterface(IStream *pStm, REFIID riid, void *pvInteface, DWORD dwDestContext,
                        void *pvDestContext, DWORD mshlflags);
    STDMETHODIMP UnmarshalInterface(IStream *pStm, REFIID riid, void **ppv);
    STDMETHODIMP ReleaseMarshalData(IStream *pStm);
    STDMETHODIMP DisconnectObject(DWORD dwReserved);

    // private method
    STDMETHODIMP SetTransactionObject(CTransaction *pCTrans);
    STDMETHODIMP GetTransactionObject(CTransaction **ppCTrans);

    STDMETHODIMP SetTransactionObjects(CTransaction *pCTrans,  CTransData *pCTransData);
    STDMETHODIMP GetTransactionObjects(CTransaction **ppCTrans,CTransData **ppCTransData);

    STDMETHODIMP SetTransData(CTransData  *pCTransData);
    STDMETHODIMP GetTransData(CTransData **ppCTransData);
    
    IBindCtx *GetRemBindCtx()
    {
        return _pbcRem;
    }

    IBindCtx *GetLocalBindCtx()
    {
        return _pbcLocal;
    }


private:

    inline BOOL CanMarshalIID(REFIID riid);
    HRESULT ValidateMarshalParams(REFIID riid, void *pvInterface, DWORD dwDestContext,
                        void *pvDestContext,DWORD mshlflags);
    CRefCount       _CRefs;     // refcount class
    IBindCtx *      _pbcRem;    // the remote bind context
    IBindCtx *      _pbcLocal;  // the local bind context
    CTransaction  *_pCTrans;
    CTransData     *_pCTransData;
    DWORD           _dwThreadId;

};


#endif  // BINDCTX_HXX
