/*
 * @(#)IObjectWithSite.hxx 1.0 11/18/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _PARSER_OM_IOBJECTWITHSITE
#define _PARSER_OM_IOBJECTWITHSITE



class NOVTABLE ObjectWithSite : public Object
{
    public: virtual void setSite(IUnknown* u) = 0;

    public: virtual void getSite(REFIID riid, void** pUnk) = 0;
};

class IObjectWithSiteWrapper : public _comexport<ObjectWithSite, IObjectWithSite, &IID_IObjectWithSite>
{
    public: IObjectWithSiteWrapper(ObjectWithSite * p, Mutex * pMutex)
                : _comexport<ObjectWithSite, IObjectWithSite, &IID_IObjectWithSite>(p)
            {
                _pMutex = pMutex;
            }

    public:
        virtual HRESULT STDMETHODCALLTYPE SetSite( 
            /* [in] */ IUnknown __RPC_FAR *pUnkSite);
        
        virtual HRESULT STDMETHODCALLTYPE GetSite( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvSite);

    private:
        RMutex _pMutex;
};



#endif _PARSER_OM_IOBJECTWITHSITE

