/*
 * @(#)datasrc.hxx 1.0 8/13/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _DATASRC_HXX
#define _DATASRC_HXX

#include <msdatsrc.h>

class NOVTABLE CDataSource : public Object
{
public:
    virtual IUnknown*   getDataMember(DataMember bstrDM, REFIID riid) = 0;
// NOTIMPL    virtual DataMember  getDataMemberName(long lIndex) = 0;
// NOTIMPL    virtual long        getDataMemberCount() = 0;
    virtual void        addDataSourceListener(DataSourceListener *pDSL) = 0;
    virtual void        removeDataSourceListener(DataSourceListener *pDSL) = 0;
};


class DataSourceWrapper : public _comexport<CDataSource, DataSource, &IID_DataSource>
{
    typedef _comexport<CDataSource, DataSource, &IID_DataSource> super;
public:
    DataSourceWrapper(CDataSource * p, Mutex * pMutex) : super(p)
    { _pMutex = pMutex; }


    /////////////////////////////////////////////////////////////////
    // DataSource methods
    
    virtual /* [restricted][hidden] */ HRESULT STDMETHODCALLTYPE getDataMember( 
        /* [in] */ DataMember bstrDM,
        /* [in] */ REFIID riid,
        /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunk);
    
    virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE getDataMemberName( 
        /* [in] */ long lIndex,
        /* [retval][out] */ DataMember __RPC_FAR *pbstrDM);
    
    virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE getDataMemberCount( 
        /* [retval][out] */ long __RPC_FAR *plCount);
    
    virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE addDataSourceListener( 
        /* [in] */ DataSourceListener __RPC_FAR *pDSL);
    
    virtual /* [hidden] */ HRESULT STDMETHODCALLTYPE removeDataSourceListener( 
        /* [in] */ DataSourceListener __RPC_FAR *pDSL);

private:
    RMutex _pMutex;
};


#endif _DATASRC_HXX
