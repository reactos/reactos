/*
 * @(#)IObjectSafety.hxx 1.0 11/18/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _PARSER_OM_IOBJECTSAFETY
#define _PARSER_OM_IOBJECTSAFETY

class NOVTABLE ObjectSafety : public Object
{
    public: virtual void getInterfaceSafetyOptions(REFIID riid, DWORD* dwOptionSetMask, DWORD* dwEnabledOptions) = 0;

    public: virtual void setInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions) = 0;
};

class IObjectSafetyWrapper : public _comexport<ObjectSafety, IObjectSafety, &IID_IObjectSafety>
{
    public: IObjectSafetyWrapper(ObjectSafety * p, Mutex * pMutex)
                : _comexport<ObjectSafety, IObjectSafety, &IID_IObjectSafety>(p)
            {
                _pMutex = pMutex;
            }

    public:
        virtual HRESULT STDMETHODCALLTYPE GetInterfaceSafetyOptions( 
            /* [in] */ REFIID riid,
            /* [out] */ DWORD __RPC_FAR *pdwSupportedOptions,
            /* [out] */ DWORD __RPC_FAR *pdwEnabledOptions);
        
        virtual HRESULT STDMETHODCALLTYPE SetInterfaceSafetyOptions( 
            /* [in] */ REFIID riid,
            /* [in] */ DWORD dwOptionSetMask,
            /* [in] */ DWORD dwEnabledOptions);

    private:
        RMutex _pMutex;
};



#endif _PARSER_OM_IOBJECTSAFETY

