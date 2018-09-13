/*
 * @(#)EnumVariant.hxx 1.0 8/28/98
 * 
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _ENUMVARIANT_HXX
#define _ENUMVARIANT_HXX

class NOVTABLE EnumVariant
{
public:
    virtual IDispatch * enumGetIDispatch(Node *) = 0;
    virtual Node * enumGetNext(void **) = 0;
    virtual bool enumValidate(void *) { return true; };
    virtual HRESULT STDMETHODCALLTYPE get__newEnum(IUnknown __RPC_FAR *__RPC_FAR *ppUnk) = 0;
};

class NOVTABLE NodeIteratorState
{
public:
    void init() { _pvPos = _pvLastPos = null; _pNextNode = _pLastNode = null; }
    Node * getNext(EnumVariant * pEnum);
    IDispatch * enumGetNext(EnumVariant * pEnum);

private:
    void *                     _pvPos;
    _reference<Node>           _pNextNode;
    void *                     _pvLastPos;
    _reference<Node>           _pLastNode;
};

DEFINE_CLASS(DOMNode);

class IEnumVARIANTWrapper: public _unknown<IEnumVARIANT, &IID_IEnumVARIANT>
{
protected:

    IEnumVARIANTWrapper() { _State.init(); };
    ~IEnumVARIANTWrapper();

public:

    static IEnumVARIANTWrapper * newIEnumVARIANTWrapper(IUnknown * pUnk, EnumVariant * pEnumVariant, Mutex * pMutex);

    // IUnknown interface
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppv);

    // IEnumVARIANT interface

    HRESULT STDMETHODCALLTYPE Next( 
        /* [in] */ ULONG cnodes,
        /* [length_is][size_is][out] */ VARIANT __RPC_FAR *rgVar,
        /* [out] */ ULONG __RPC_FAR *pCnodesFetched);
    
    HRESULT STDMETHODCALLTYPE Skip( 
        /* [in] */ ULONG cnodes);
    
    HRESULT STDMETHODCALLTYPE Reset( void);
    
    HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IEnumVARIANT __RPC_FAR *__RPC_FAR *ppEnum)
    {
        return _pEnumVariant->get__newEnum((IUnknown**)ppEnum);
    }

protected:
    NodeIteratorState          _State;

    EnumVariant *              _pEnumVariant;
    _reference<IUnknown>       _pUnk; 
    _reference<Mutex>          _pMutex;
};

#endif //_ENUMVARIANT_HXX