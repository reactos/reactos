/*
 * @(#)ospwrap.hxx 1.0 8/11/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _OSPWRAP_HXX
#define _OSPWRAP_HXX

#include <simpdata.h>
#include "xmlrowset.hxx"

class OSPWrapper : public _comexport<XMLRowsetProvider, OLEDBSimpleProvider, &IID_OLEDBSimpleProvider>
{
    typedef _comexport<XMLRowsetProvider, OLEDBSimpleProvider, &IID_OLEDBSimpleProvider> super;
public:
    OSPWrapper(XMLRowsetProvider * p, Mutex * pMutex) : super(p)
    { _pMutex = pMutex; }

    XMLRowsetProvider * getProvider() { return getWrapped(); }

    /////////////////////////////////////////////////////////////////
    // IUnknown methods
    
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppv);


    /////////////////////////////////////////////////////////////////
    // OSP methods
    
    virtual HRESULT STDMETHODCALLTYPE getRowCount( 
        /* [retval][out] */ DBROWCOUNT __RPC_FAR *pcRows);
    
    virtual HRESULT STDMETHODCALLTYPE getColumnCount( 
        /* [retval][out] */ DB_LORDINAL __RPC_FAR *pcColumns);
    
    virtual HRESULT STDMETHODCALLTYPE getRWStatus( 
        /* [in] */ DBROWCOUNT iRow,
        /* [in] */ DB_LORDINAL iColumn,
        /* [retval][out] */ OSPRW __RPC_FAR *prwStatus);
    
    virtual HRESULT STDMETHODCALLTYPE getVariant( 
        /* [in] */ DBROWCOUNT iRow,
        /* [in] */ DB_LORDINAL iColumn,
        /* [in] */ OSPFORMAT format,
        /* [retval][out] */ VARIANT __RPC_FAR *pVar);
    
    virtual HRESULT STDMETHODCALLTYPE setVariant( 
        /* [in] */ DBROWCOUNT iRow,
        /* [in] */ DB_LORDINAL iColumn,
        /* [in] */ OSPFORMAT format,
        /* [in] */ VARIANT Var);
    
    virtual HRESULT STDMETHODCALLTYPE getLocale( 
        /* [retval][out] */ BSTR __RPC_FAR *pbstrLocale);
    
    virtual HRESULT STDMETHODCALLTYPE deleteRows( 
        /* [in] */ DBROWCOUNT iRow,
        /* [in] */ DBROWCOUNT cRows,
        /* [retval][out] */ DBROWCOUNT __RPC_FAR *pcRowsDeleted);
    
    virtual HRESULT STDMETHODCALLTYPE insertRows( 
        /* [in] */ DBROWCOUNT iRow,
        /* [in] */ DBROWCOUNT cRows,
        /* [retval][out] */ DBROWCOUNT __RPC_FAR *pcRowsInserted);
    
    virtual HRESULT STDMETHODCALLTYPE find( 
        /* [in] */ DBROWCOUNT iRowStart,
        /* [in] */ DB_LORDINAL iColumn,
        /* [in] */ VARIANT val,
        /* [in] */ OSPFIND findFlags,
        /* [in] */ OSPCOMP compType,
        /* [retval][out] */ DBROWCOUNT __RPC_FAR *piRowFound);
    
    virtual HRESULT STDMETHODCALLTYPE addOLEDBSimpleProviderListener( 
        /* [in] */ OLEDBSimpleProviderListener __RPC_FAR *pospIListener);
    
    virtual HRESULT STDMETHODCALLTYPE removeOLEDBSimpleProviderListener( 
        /* [in] */ OLEDBSimpleProviderListener __RPC_FAR *pospIListener);
    
    virtual HRESULT STDMETHODCALLTYPE isAsync( 
        /* [retval][out] */ BOOL __RPC_FAR *pbAsynch);
    
    virtual HRESULT STDMETHODCALLTYPE getEstimatedRows( 
        /* [retval][out] */ DBROWCOUNT __RPC_FAR *piRows);
    
    virtual HRESULT STDMETHODCALLTYPE stopTransfer( void);

private:
    RMutex _pMutex;
};


#endif _OSPWRAP_HXX
